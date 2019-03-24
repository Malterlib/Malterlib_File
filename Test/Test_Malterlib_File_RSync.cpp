// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/File/RSync>
#include <Mib/Cryptography/EncryptedStream>

using namespace NMib::NFile;
using namespace NMib;
using namespace NMib::NMemory;
using namespace NMib::NStream;
using namespace NMib::NContainer;
using namespace NMib::NNetwork;

namespace
{
	template <typename t_CStreamType = NStream::CBinaryStreamDefault, typename t_CVector = NContainer::CByteVector>
	class CBinaryStreamMemorySequential : public CBinaryStreamMemory<t_CStreamType, t_CVector>
	{
		typedef CBinaryStreamMemory<t_CStreamType, t_CVector> CParent;

	private:
		CBinaryStreamMemorySequential(CBinaryStreamMemorySequential const &) = delete;
		CBinaryStreamMemorySequential &operator = (CBinaryStreamMemorySequential const &) = delete;

	public:
		typedef t_CVector CStorage;

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
		static void fs_DoRSync(CSecureByteVector const &_Orig, CSecureByteVector const &_New, uint32 _MinChunk, uint32 _MaxChunk, bool _bInPlace, bool _bEncrypt)
		{
			bool bServerDone = false;
			mint TotalSizeClient = 0;
			mint TotalSizeServer = 0;
			mint nRawBytes;
			NTime::CTimer Timer;
			Timer.f_Start();
			CSecureByteVector Synced;
			{
				CBinaryStreamMemoryConstRef<NStream::CBinaryStreamDefault, CSecureByteVector> ServerStream(_New);
				CBinaryStreamMemoryConstRef<NStream::CBinaryStreamDefault, CSecureByteVector> ClientStream(_Orig);
				CBinaryStream *pClientOld = &ClientStream;
				if (_bInPlace)
					Synced = _Orig;

				CBinaryStreamMemorySequential<NStream::CBinaryStreamDefault, CSecureByteVector> ClientNewStream;
				CBinaryStreamMemorySequential<NStream::CBinaryStreamDefault, CSecureByteVector> TemporaryStream;

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
					pKey = fg_Construct(CEncryptKeyIV::fs_GetRandomKey(ESSLCrypto_AES_256_CBC));
					pIV = fg_Construct(CEncryptKeyIV::fs_GetRandomIV(ESSLCrypto_AES_256_CBC));
					pHMACKey = fg_Construct(CEncryptKeyIV::fs_GetRandomHMACKey(ESSLDigest_SHA256));

					pClientNewStreamEncrypted = fg_Construct<CEncrypted>(CEncryptKeyIV{*pKey, *pIV, ESSLCrypto_AES_256_CBC}, ESSLDigest_SHA256, *pHMACKey);
					pClientNewStreamEncrypted->f_SetBufferSize(16);
					pClientNewStreamEncrypted->f_Open(pClientNewStream, EFileOpen_Write);
					pClientNewStream = &*pClientNewStreamEncrypted;
					if (pTemporaryStream)
					{
						pTemporaryStreamEncrypted = fg_Construct<CEncrypted>(CEncryptKeyIV{*pKey, *pIV}, ESSLDigest_SHA256, *pHMACKey);
						pTemporaryStreamEncrypted->f_SetBufferSize(16);
						pTemporaryStreamEncrypted->f_Open(pTemporaryStream, EFileOpen_Read | EFileOpen_Write);
						pTemporaryStream = &*pTemporaryStreamEncrypted;
					}
				}

				CRSyncServer RSyncServer(ServerStream, 32*1024*1024);
				CRSyncClient RSyncClient(*pClientOld, *pClientNewStream, _MinChunk, _MaxChunk, 32*1024*1024, pTemporaryStream);
				
				TCVector<CSecureByteVector> ServerData;
				TCVector<CSecureByteVector> ClientData;
				while (1)
				{
					bool bDone = false;
					bool bDoOneProcess = true;
					while (!ServerData.f_IsEmpty() || bDoOneProcess)
					{
						CSecureByteVector Vector;
						if (!ServerData.f_IsEmpty())
							Vector = ServerData.f_Pop();
						CSecureByteVector Temp;
						bDone = RSyncClient.f_ProcessPacket(Vector, Temp, bDoOneProcess);
						TotalSizeClient += Temp.f_GetLen();
						if (!Temp.f_IsEmpty())
							ClientData.f_Insert(fg_Move(Temp));
					}
					if (bDone)
						break;

					while (!ClientData.f_IsEmpty())
					{
						CSecureByteVector Vector;
						if (!ClientData.f_IsEmpty())
							Vector = ClientData.f_Pop();
						CSecureByteVector Temp;
						bServerDone = RSyncServer.f_ProcessPacket(Vector, Temp);
						TotalSizeServer += Temp.f_GetLen();
						if (!Temp.f_IsEmpty())
							ServerData.f_Insert(fg_Move(Temp));
					}
					
				}
				nRawBytes = RSyncClient.f_GetRawBytes();
				if (_bEncrypt)
				{
					CSecureByteVector EncryptedData;
					EncryptedData = ClientNewStream.f_MoveVector();
					CBinaryStreamMemoryConstRef<NStream::CBinaryStreamDefault, CSecureByteVector> DataStream(EncryptedData);
					NCryptography::TCBinaryStream_Encrypted<CBinaryStream *> DecryptingStream(CEncryptKeyIV{*pKey, *pIV}, ESSLDigest_SHA256, *pHMACKey);
					DecryptingStream.f_Open(&DataStream, EFileOpen_Read);
					auto Length = DecryptingStream.f_GetLength();
					DecryptingStream.f_ConsumeBytes(Synced.f_GetArray(Length), Length);
				}
				else
					Synced = ClientNewStream.f_MoveVector();
			}
			Timer.f_Stop();
//			DMibTrace("Protocol: C {} + S {} ({} raw)= {} bytes = {fe2} speedup {fe2} ms\r\n", TotalSizeClient << TotalSizeServer << nRawBytes << (TotalSizeClient + TotalSizeServer) << fp64(_New.f_GetLen()) / fp64((TotalSizeClient + TotalSizeServer)) << Timer.f_GetTime() * 1000.0);
			DMibTest(DMibExpr(bServerDone) == DMibExpr(true));

			ETestFlag TestFlags = ETestFlag_NoValuesOnSuccess;
			if (_New.f_GetLen() > 64)
				TestFlags |= ETestFlag_NoValues;
			DMibTest(DMibExpr(Synced) == DMibExpr(_New))(TestFlags);
		}
		static CSecureByteVector fs_ToVector(const ch8 *_pStr)
		{
			mint Len = NStr::fg_StrLen(_pStr);
			CSecureByteVector Ret;
			Ret.f_Insert((uint8 const *)_pStr, Len);
			
			return Ret;			
		}
		static void fs_DoTests(uint32 _MinChunk, uint32 _MaxChunk, bool _bInPlace, bool _bEncrypt)
		{
			DMibTestCategory(NStr::CStr(NStr::CStr::CFormat("{} -> {}{}{}") << _MaxChunk << _MinChunk << (_bInPlace ? " In place" : "") << (_bEncrypt ? "Encrypted stream" : "")))
			{
				DMibTestSuite("Same dual")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("aaa aaa"), CRSync_Tests::fs_ToVector("aaa aaa"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				};
				DMibTestSuite("Same")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing 123"), CRSync_Tests::fs_ToVector("Testing 123"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				};
				DMibTestSuite("Addition")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing 123"), CRSync_Tests::fs_ToVector("Testing New! 123"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				};
				DMibTestSuite("Same longer")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing New! 123"), CRSync_Tests::fs_ToVector("Testing New! 123"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				};
				DMibTestSuite("Deletion")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing New! 123"), CRSync_Tests::fs_ToVector("Testing 123"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				};
				DMibTestSuite("Long")
				{
					NStr::CStr LongStr;
					LongStr.f_AddChars('a', 256*1000);
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector(""), CRSync_Tests::fs_ToVector(LongStr), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				};
				DMibTestSuite("Long Before")
				{
					NStr::CStr LongStr;
					LongStr.f_AddChars('a', 256*1000);
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("456"), CRSync_Tests::fs_ToVector(LongStr + "456"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				};
				DMibTestSuite("Long After")
				{
					NStr::CStr LongStr;
					LongStr.f_AddChars('a', 256*1000);
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing 123"), CRSync_Tests::fs_ToVector("Testing 123" + LongStr), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				};
				DMibTestSuite("Long Middle")
				{
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
				};
				DMibTestSuite("Reverse")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("abcdefghijklmnopqr"), CRSync_Tests::fs_ToVector("qropmnklijghefcdab"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				};
				DMibTestSuite("Non-sequential read for 16 byte blocks")
				{
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
				};
				DMibTestSuite("No intersection")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("abcdefghijklmnopqr"), CRSync_Tests::fs_ToVector("0123456789"), _MinChunk, _MaxChunk, _bInPlace, _bEncrypt);
				};
			};
		};

		static void fs_TestFiles(NStr::CStr const &_StartFile, NStr::CStr const &_EndFile)
		{
			NStr::CStr StartFileName = NFile::CFile::fs_GetProgramDirectory() + "/" + _StartFile;
			NStr::CStr EndFileName = NFile::CFile::fs_GetProgramDirectory() + "/" + _EndFile;
			if (NFile::CFile::fs_FileExists(StartFileName) && NFile::CFile::fs_FileExists(EndFileName))
			{
				fs_DoRSync(NFile::CFile::fs_ReadFile(StartFileName).f_ToSecure(), NFile::CFile::fs_ReadFile(EndFileName).f_ToSecure(), 128, 1024*1024, false, false);
			}
		}

		void f_DoTests()
		{
			DMibTestCategory("Basic")
			{
				for (mint e = 0; e < 2; ++e)
				{
					bool bEncrypt = e != 0;
					for (mint i = 0; i < 2; ++i)
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
					//DMibScopeTraceTimer("Almost same");
					CRSync_Tests::fs_TestFiles("StartFile", "EndFile0");
				};
				DMibTestSuite("Day")
				{
					//DMibScopeTraceTimer("Day");
					CRSync_Tests::fs_TestFiles("StartFile", "EndFile1");
				};
				DMibTestSuite("24 days")
				{
					//DMibScopeTraceTimer("24 days");
					CRSync_Tests::fs_TestFiles("StartFile", "EndFile2");
				};
				DMibTestSuite("Dll")
				{
					//DMibScopeTraceTimer("Dll");
					CRSync_Tests::fs_TestFiles("StartDll", "EndDll");
				};
				DMibTestSuite("Big")
				{
					//DMibScopeTraceTimer("Big");
					CRSync_Tests::fs_TestFiles("StartBig", "EndBig");
				};
				DMibTestSuite("Worst")
				{
					//DMibScopeTraceTimer("Big");
					CRSync_Tests::fs_TestFiles("StartWorst0", "EndWorst0");
				};
				DMibTestSuite("Same")
				{
					//DMibScopeTraceTimer("Big");
					CRSync_Tests::fs_TestFiles("StartWorst0", "StartWorst0");
				};
			};
		}
	};

	DMibTestRegister(CRSync_Tests, Malterlib::File);
}

