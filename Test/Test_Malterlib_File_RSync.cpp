// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Mib/Cryptography/EncryptedStream>
#include <Mib/File/RSync>
#include <Mib/Time/TimeMeasure>

using namespace NMib::NFile;
using namespace NMib;
using namespace NMib::NMemory;
using namespace NMib::NStream;
using namespace NMib::NContainer;
using namespace NMib::NNetwork;
using namespace NMib::NCryptography;

namespace
{
	template <typename t_CStreamType = NStream::CBinaryStreamDefault, typename t_CVector = NContainer::CByteVector>
	class CBinaryStreamMemorySequential : public CBinaryStreamMemory<t_CStreamType, t_CVector>
	{
		using CParent = CBinaryStreamMemory<t_CStreamType, t_CVector>;

	private:
		CBinaryStreamMemorySequential(CBinaryStreamMemorySequential const &) = delete;
		CBinaryStreamMemorySequential &operator = (CBinaryStreamMemorySequential const &) = delete;

	public:
		using CStorage = t_CVector;

		DMibStreamImplementProtected(CBinaryStreamMemorySequential);
	public:
		DMibStreamImplementOperators(CBinaryStreamMemorySequential);

		CBinaryStreamMemorySequential(CStorage const& _Buffer)
			: CParent(_Buffer)
		{
		}

		CBinaryStreamMemorySequential(CStorage && _Buffer)
			: CParent(fg_Move(_Buffer))
		{
		}

		CBinaryStreamMemorySequential()
		{
		}

		void f_SetPosition(CFilePos _Pos)
		{
			if (_Pos != 0 && CParent::f_GetPosition() != _Pos)
				DMibExpect(_Pos, ==, CParent::f_GetPosition());

			CParent::f_SetPosition(_Pos);
		}
	};

	class CRSync_Tests : public NMib::NTest::CTest
	{
	public:
		static void fs_DoRSync(CIOByteVector const &_Orig, CIOByteVector const &_New, uint32 _MinChunk, uint32 _MaxChunk, bool _bInPlace, bool _bEncrypt)
		{
			bool bServerDone = false;
			[[maybe_unused]] umint TotalSizeClient = 0;
			[[maybe_unused]] umint TotalSizeServer = 0;
			[[maybe_unused]] umint nRawBytes;
			NTime::CTimeMeasure Timer;
			Timer.f_Start();
			CIOByteVector Synced;
			{
				CBinaryStreamMemoryConstRef<NStream::CBinaryStreamDefault, CIOByteVector> ServerStream(_New);
				CBinaryStreamMemoryConstRef<NStream::CBinaryStreamDefault, CIOByteVector> ClientStream(_Orig);
				CBinaryStream *pClientOld = &ClientStream;
				if (_bInPlace)
					Synced = _Orig;

				CBinaryStreamMemorySequential<NStream::CBinaryStreamDefault, CIOByteVector> ClientNewStream;
				CBinaryStreamMemorySequential<NStream::CBinaryStreamDefault, CIOByteVector> TemporaryStream;

				using CEncrypted = NCryptography::TCBinaryStream_Encrypted<CBinaryStream *>;
				NStorage::TCUniquePointer<CEncrypted> pClientNewStreamEncrypted;
				NStorage::TCUniquePointer<CEncrypted> pTemporaryStreamEncrypted;
				NStorage::TCUniquePointer<NContainer::CSecureByteVector> pKey;
				NStorage::TCUniquePointer<NContainer::CSecureByteVector> pIV;
				NStorage::TCUniquePointer<NContainer::CSecureByteVector> pHMACKey;
				CBinaryStream *pClientNewStream = &ClientNewStream;
				CBinaryStream *pTemporaryStream = nullptr;
				if (_bInPlace)
				{
					pTemporaryStream = &TemporaryStream;
					pClientOld = &ClientNewStream;
				}

				if (_bEncrypt)
				{
					pKey = fg_Construct(CEncryptKeyIV::fs_GetRandomKey(ECryptoType_AES_256_CBC));
					pIV = fg_Construct(CEncryptKeyIV::fs_GetRandomIV(ECryptoType_AES_256_CBC));
					pHMACKey = fg_Construct(CEncryptKeyIV::fs_GetRandomHMACKey(EDigestType_SHA256));

					pClientNewStreamEncrypted = fg_Construct<CEncrypted>(CEncryptKeyIV{*pKey, *pIV, ECryptoType_AES_256_CBC}, EDigestType_SHA256, *pHMACKey);
					pClientNewStreamEncrypted->f_SetBufferSize(16);
					pClientNewStreamEncrypted->f_Open(pClientNewStream, EFileOpen_Write);
					pClientNewStream = &*pClientNewStreamEncrypted;
					if (pTemporaryStream)
					{
						pTemporaryStreamEncrypted = fg_Construct<CEncrypted>(CEncryptKeyIV{*pKey, *pIV}, EDigestType_SHA256, *pHMACKey);
						pTemporaryStreamEncrypted->f_SetBufferSize(16);
						pTemporaryStreamEncrypted->f_Open(pTemporaryStream, EFileOpen_Read | EFileOpen_Write);
						pTemporaryStream = &*pTemporaryStreamEncrypted;
					}
				}

				CRSyncServer RSyncServer(ServerStream, 32 * 1024 * 1024, ERSyncFlag_UseSHA256);
				CRSyncClient RSyncClient
					(
						*pClientOld
						, *pClientNewStream
						, _MinChunk
						, _MaxChunk
						, 32 * 1024 * 1024
						, 32 * 1024 * 1024
						, pTemporaryStream
						, ERSyncFlag_ClientTruncateOutput | ERSyncFlag_UseSHA256
					)
				;

				TCVector<CIOByteVector> ServerData;
				TCVector<CIOByteVector> ClientData;
				while (1)
				{
					bool bDone = false;
					bool bDoOneProcess = true;
					while (!ServerData.f_IsEmpty() || bDoOneProcess)
					{
						CIOByteVector Vector;
						if (!ServerData.f_IsEmpty())
							Vector = ServerData.f_Pop();
						CIOByteVector Temp;
						bDone = RSyncClient.f_ProcessPacket(Vector, Temp, bDoOneProcess, nullptr);
						TotalSizeClient += Temp.f_GetLen();
						if (!Temp.f_IsEmpty())
							ClientData.f_Insert(fg_Move(Temp));
					}
					if (bDone)
						break;

					while (!ClientData.f_IsEmpty())
					{
						CIOByteVector Vector;
						if (!ClientData.f_IsEmpty())
							Vector = ClientData.f_Pop();
						CIOByteVector Temp;
						bServerDone = RSyncServer.f_ProcessPacket(Vector, Temp, nullptr);
						TotalSizeServer += Temp.f_GetLen();
						if (!Temp.f_IsEmpty())
							ServerData.f_Insert(fg_Move(Temp));
					}

				}
				nRawBytes = RSyncClient.f_GetRawBytes();
				if (_bEncrypt)
				{
					CIOByteVector EncryptedData;
					EncryptedData = ClientNewStream.f_MoveVector();
					CBinaryStreamMemoryConstRef<NStream::CBinaryStreamDefault, CIOByteVector> DataStream(EncryptedData);
					NCryptography::TCBinaryStream_Encrypted<CBinaryStream *> DecryptingStream(CEncryptKeyIV{*pKey, *pIV}, EDigestType_SHA256, *pHMACKey);
					DecryptingStream.f_Open(&DataStream, EFileOpen_Read);
					auto Length = DecryptingStream.f_GetLength();
					DecryptingStream.f_ConsumeBytes(Synced.f_GetArray(Length), Length);
				}
				else
					Synced = ClientNewStream.f_MoveVector();
			}
			Timer.f_Stop();
//			DMibTrace("Protocol: C {} + S {} ({} raw)= {} bytes = {fe2} speedup {fe2} ms\r\n", TotalSizeClient, TotalSizeServer, nRawBytes, (TotalSizeClient + TotalSizeServer), fp64(_New.f_GetLen()) / fp64((TotalSizeClient + TotalSizeServer)), Timer.f_GetTime() * 1000.0);
			DMibTest(DMibExpr(bServerDone) == DMibExpr(true));

			ETestFlag TestFlags = ETestFlag_NoValuesOnSuccess;
			if (_New.f_GetLen() > 64)
				TestFlags |= ETestFlag_NoValues;
			DMibTest(DMibExpr(Synced) == DMibExpr(_New))(TestFlags);
		}
		static CIOByteVector fs_ToVector(const ch8 *_pStr)
		{
			umint Len = NStr::fg_StrLen(_pStr);
			CIOByteVector Ret;
			Ret.f_Insert((uint8 const *)_pStr, Len);

			return Ret;
		}
		static void fs_DoTests(uint32 _MinChunk, uint32 _MaxChunk, bool _bInPlace, bool _bEncrypt)
		{
			DMibTestPath(NStr::CStr(NStr::CStr::CFormat("{} -> {}{}{}") << _MaxChunk << _MinChunk << (_bInPlace ? " In place" : "") << (_bEncrypt ? "Encrypted stream" : "")));
			{
				{
					DMibTestPath("Same dual");
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("aaa aaa"), CRSync_Tests::fs_ToVector("aaa aaa"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				}
				{
					DMibTestPath("Same");
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing 123"), CRSync_Tests::fs_ToVector("Testing 123"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				}
				{
					DMibTestPath("Addition");
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing 123"), CRSync_Tests::fs_ToVector("Testing New! 123"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				}
				{
					DMibTestPath("Same longer");
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing New! 123"), CRSync_Tests::fs_ToVector("Testing New! 123"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				}
				{
					DMibTestPath("Deletion");
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing New! 123"), CRSync_Tests::fs_ToVector("Testing 123"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				}
				{
					DMibTestPath("Long");
					NStr::CStr LongStr;
					LongStr.f_AddChars('a', 256*1000);
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector(""), CRSync_Tests::fs_ToVector(LongStr), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				}
				{
					DMibTestPath("Long Before");
					NStr::CStr LongStr;
					LongStr.f_AddChars('a', 256*1000);
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("456"), CRSync_Tests::fs_ToVector(LongStr + "456"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				}
				{
					DMibTestPath("Long After");
					NStr::CStr LongStr;
					LongStr.f_AddChars('a', 256*1000);
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing 123"), CRSync_Tests::fs_ToVector("Testing 123" + LongStr), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				}
				{
					DMibTestPath("Long Middle");
					NStr::CStr LongStr;
					LongStr.f_AddChars('a', 256*1000);
					CRSync_Tests::fs_DoRSync
						(
							CRSync_Tests::fs_ToVector("Testing 123456"), CRSync_Tests::fs_ToVector("Testing 123" + LongStr + "456")
							, _MinChunk
							, _MaxChunk
							, _bInPlace
							, _bEncrypt
						)
					;
				}
				{
					DMibTestPath("Reverse");
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("abcdefghijklmnopqr"), CRSync_Tests::fs_ToVector("qropmnklijghefcdab"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				}
				{
					DMibTestPath("Non-sequential read for 16 byte blocks");
					CRSync_Tests::fs_DoRSync
						(
							CRSync_Tests::fs_ToVector("0123ABCD9abcdefEFGH56789abcdef0123456789IJKLf01MNOP6789abcdef0123456789abcdQRST23456789abcdef01234UVWXabcdef0123456789abcdef")
							, CRSync_Tests::fs_ToVector("0123456789abcdefUVWXQRSTMNOPEFGHABCDIJKL0123456789abcdef....ABCD1234EFGH4567IJKL5678MNOP7890QRST9876UVWX")
							, _MinChunk
							, _MaxChunk
							, _bInPlace
							, _bEncrypt
						)
					;
				}
				{
					DMibTestPath("No intersection");
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("abcdefghijklmnopqr"), CRSync_Tests::fs_ToVector("0123456789"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				}
			};
		};

		static void fs_TestFiles(NStr::CStr const &_StartFile, NStr::CStr const &_EndFile)
		{
			NStr::CStr StartFileName = NFile::CFile::fs_GetProgramDirectory() + "/" + _StartFile;
			NStr::CStr EndFileName = NFile::CFile::fs_GetProgramDirectory() + "/" + _EndFile;
			if (NFile::CFile::fs_FileExists(StartFileName) && NFile::CFile::fs_FileExists(EndFileName))
			{
				fs_DoRSync
					(
						CIOByteVector::fs_AllowInsecureConversion(NFile::CFile::fs_ReadFile(StartFileName))
						, CIOByteVector::fs_AllowInsecureConversion(NFile::CFile::fs_ReadFile(EndFileName))
						, 128
						, 1024*1024
						, false
						, false
					)
				;
			}
		}

		void f_DoTests()
		{
			DMibTestSuite("Basic")
			{
				for (umint e = 0; e < 2; ++e)
				{
					bool bEncrypt = e != 0;
					for (umint i = 0; i < 2; ++i)
					{
						bool bInline = i != 0;
						fs_DoTests(1, 1, bInline, bEncrypt);
						fs_DoTests(2, 2, bInline, bEncrypt);
						fs_DoTests(1, 2, bInline, bEncrypt);
						fs_DoTests(1, 4, bInline, bEncrypt);
						fs_DoTests(1, 8, bInline, bEncrypt);
						fs_DoTests(1, 16, bInline, bEncrypt);
						fs_DoTests(1, 32, bInline, bEncrypt);
						fs_DoTests(2, 32, bInline, bEncrypt);
						fs_DoTests(4, 32, bInline, bEncrypt);
						fs_DoTests(8, 32, bInline, bEncrypt);
						fs_DoTests(16, 32, bInline, bEncrypt);
						fs_DoTests(32, 32, bInline, bEncrypt);
					}
				}
			};
			DMibTestCategory(CTestCategory("Custom files"))
			{
				DMibTestSuite("Almost same")
				{
					//DMibScopeTraceTimeMeasure("Almost same");
					CRSync_Tests::fs_TestFiles("StartFile", "EndFile0");
				};
				DMibTestSuite("Day")
				{
					//DMibScopeTraceTimeMeasure("Day");
					CRSync_Tests::fs_TestFiles("StartFile", "EndFile1");
				};
				DMibTestSuite("24 days")
				{
					//DMibScopeTraceTimeMeasure("24 days");
					CRSync_Tests::fs_TestFiles("StartFile", "EndFile2");
				};
				DMibTestSuite("Dll")
				{
					//DMibScopeTraceTimeMeasure("Dll");
					CRSync_Tests::fs_TestFiles("StartDll", "EndDll");
				};
				DMibTestSuite("Big")
				{
					//DMibScopeTraceTimeMeasure("Big");
					CRSync_Tests::fs_TestFiles("StartBig", "EndBig");
				};
				DMibTestSuite("Worst")
				{
					//DMibScopeTraceTimeMeasure("Big");
					CRSync_Tests::fs_TestFiles("StartWorst0", "EndWorst0");
				};
				DMibTestSuite("Same")
				{
					//DMibScopeTraceTimeMeasure("Big");
					CRSync_Tests::fs_TestFiles("StartWorst0", "StartWorst0");
				};
			};
		}
	};

	DMibTestRegister(CRSync_Tests, Malterlib::File);
}

