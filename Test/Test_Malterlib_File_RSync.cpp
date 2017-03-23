// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/File/RSync>

using namespace NMib::NFile;
using namespace NMib;
using namespace NMib::NMem;
using namespace NMib::NStream;
using namespace NMib::NContainer;

namespace
{
	class CRSync_Tests : public NMib::NTest::CTest
	{
	public:

		static void fs_DoRSync(CSecureByteVector const &_Orig, CSecureByteVector const &_New, uint32 _MinChunk, uint32 _MaxChunk, bool _bInPlace)
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
				CBinaryStreamMemory<NStream::CBinaryStreamDefault, CSecureByteVector> ClientNewStream;

				CBinaryStreamMemory<NStream::CBinaryStreamDefault, CSecureByteVector> TemporaryStream;
				CBinaryStream *pTemporaryStream = nullptr;
				if (_bInPlace)
				{
					pTemporaryStream = &TemporaryStream;
					pClientOld = &ClientNewStream;
				}
				
				CRSyncServer RSyncServer(ServerStream, 32*1024*1024);
				CRSyncClient RSyncClient(*pClientOld, ClientNewStream, _MinChunk, _MaxChunk, 32*1024*1024, pTemporaryStream);
				
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
				
				Synced = ClientNewStream.f_MoveVector();
			}
			Timer.f_Stop();
//			DMibTrace("Protocol: C {} + S {} ({} raw)= {} bytes = {fe2} speedup {fe2} ms\r\n", TotalSizeClient << TotalSizeServer << nRawBytes << (TotalSizeClient + TotalSizeServer) << fp64(_New.f_GetLen()) / fp64((TotalSizeClient + TotalSizeServer)) << Timer.f_GetTime() * 1000.0);
			DMibTest(DMibExpr(bServerDone) == DMibExpr(true));
			DMibTest(DMibExpr(Synced) == DMibExpr(_New));
		}
		static CSecureByteVector fs_ToVector(const ch8 *_pStr)
		{
			mint Len = NStr::fg_StrLen(_pStr);
			CSecureByteVector Ret;
			Ret.f_Insert((uint8 const *)_pStr, Len);
			
			return Ret;			
		}
		static void fs_DoTests(uint32 _MinChunk, uint32 _MaxChunk, bool _bInPlace)
		{
			DMibTestCategory(NStr::CStr(NStr::CStr::CFormat("{} -> {}{}") << _MaxChunk << _MinChunk << (_bInPlace ? " In place" : "")))
			{
				DMibTestSuite("Same dual")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("aaa aaa"), CRSync_Tests::fs_ToVector("aaa aaa"), _MinChunk, _MaxChunk, _bInPlace);
				};
				DMibTestSuite("Same")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing 123"), CRSync_Tests::fs_ToVector("Testing 123"), _MinChunk, _MaxChunk, _bInPlace);
				};
				DMibTestSuite("Addition")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing 123"), CRSync_Tests::fs_ToVector("Testing New! 123"), _MinChunk, _MaxChunk, _bInPlace);
				};
				DMibTestSuite("Same longer")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing New! 123"), CRSync_Tests::fs_ToVector("Testing New! 123"), _MinChunk, _MaxChunk, _bInPlace);
				};
				DMibTestSuite("Deletion")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing New! 123"), CRSync_Tests::fs_ToVector("Testing 123"), _MinChunk, _MaxChunk, _bInPlace);
				};
				DMibTestSuite("Long")
				{
					NStr::CStr LongStr;
					LongStr.f_AddChars('a', 1024*1024);
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector(""), CRSync_Tests::fs_ToVector(LongStr), _MinChunk, _MaxChunk, _bInPlace);
				};
				DMibTestSuite("Reverse")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("abcdefghijklmnopqr"), CRSync_Tests::fs_ToVector("qropmnklijghefcdab"), _MinChunk, _MaxChunk, _bInPlace);
				};
			};
		};

		static void fs_TestFiles(NStr::CStr const &_StartFile, NStr::CStr const &_EndFile)
		{
			NStr::CStr StartFileName = NFile::CFile::fs_GetProgramDirectory() + "/" + _StartFile;
			NStr::CStr EndFileName = NFile::CFile::fs_GetProgramDirectory() + "/" + _EndFile;
			if (NFile::CFile::fs_FileExists(StartFileName) && NFile::CFile::fs_FileExists(EndFileName))
			{
				fs_DoRSync(NFile::CFile::fs_ReadFile(StartFileName), NFile::CFile::fs_ReadFile(EndFileName), 128, 1024*1024, false);
			}
		}

		void f_DoTests()
		{
			DMibTestCategory("Basic")
			{
				for (mint i = 0; i < 2; ++i)
				{
					bool bInline = i != 0;
					fs_DoTests(1, 1, bInline);
					fs_DoTests(2, 2, bInline);
					fs_DoTests(1, 2, bInline);
					fs_DoTests(1, 4, bInline);
					fs_DoTests(1, 8, bInline);
					fs_DoTests(1, 16, bInline);
					fs_DoTests(1, 32, bInline);
					fs_DoTests(2, 32, bInline);
					fs_DoTests(4, 32, bInline);
					fs_DoTests(8, 32, bInline);
					fs_DoTests(16, 32, bInline);
					fs_DoTests(32, 32, bInline);
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

