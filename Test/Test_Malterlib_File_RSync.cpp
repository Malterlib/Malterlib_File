// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/File/RSync>

using namespace NMib::NDataProcessing;
using namespace NMib;
using namespace NMib::NMem;
using namespace NMib::NStream;
using namespace NMib::NContainer;

namespace
{
	class CRSync_Tests : public NMib::NTest::CTest
	{
	public:

		static void fs_DoRSync(TCVector<uint8> const &_Orig, TCVector<uint8> const &_New, uint32 _MinChunk, uint32 _MaxChunk)
		{
			bint bServerDone = false;
			mint TotalSizeClient = 0;
			mint TotalSizeServer = 0;
			mint nRawBytes;
			NTime::CTimer Timer;
			Timer.f_Start();
			TCVector<uint8> Synced;
			{
				CBinaryStreamMemoryConstRef<> ClientStream(_Orig);
				CBinaryStreamMemoryConstRef<> ServerStream(_New);
				CBinaryStreamMemoryRef<> ClientNewStream(Synced);
				
				CRSyncServer RSyncServer(ServerStream, 32*1024*1024);
				CRSyncClient RSyncClient(ClientStream, ClientNewStream, _MinChunk, _MaxChunk, 32*1024*1024);
				
				TCVector<TCVector<uint8>> ServerData;
				TCVector<TCVector<uint8>> ClientData;
				while (1)
				{
					bint bDone = false;
					bint bDoOneProcess = true;
					while (!ServerData.f_IsEmpty() || bDoOneProcess)
					{
						TCVector<uint8> Vector;
						if (!ServerData.f_IsEmpty())
							Vector = ServerData.f_Pop();
						TCVector<uint8> Temp;
						bDone = RSyncClient.f_ProcessPacket(Vector, Temp, bDoOneProcess);
						TotalSizeClient += Temp.f_GetLen();
						if (!Temp.f_IsEmpty())
							ClientData.f_Insert(fg_Move(Temp));
					}
					if (bDone)
						break;

					bDoOneProcess = true;
					while (!ClientData.f_IsEmpty() || bDoOneProcess)
					{
						TCVector<uint8> Vector;
						if (!ClientData.f_IsEmpty())
							Vector = ClientData.f_Pop();
						TCVector<uint8> Temp;
						bServerDone = RSyncServer.f_ProcessPacket(Vector, Temp, bDoOneProcess);
						TotalSizeServer += Temp.f_GetLen();
						if (!Temp.f_IsEmpty())
							ServerData.f_Insert(fg_Move(Temp));
					}
					
				}
				nRawBytes = RSyncClient.f_GetRawBytes();
			}
			Timer.f_Stop();
//			DMibTrace("Protocol: C {} + S {} ({} raw)= {} bytes = {fe2} speedup {fe2} ms\r\n", TotalSizeClient << TotalSizeServer << nRawBytes << (TotalSizeClient + TotalSizeServer) << fp64(_New.f_GetLen()) / fp64((TotalSizeClient + TotalSizeServer)) << Timer.f_GetTime() * 1000.0);
			DMibTest(DMibExpr(bServerDone) == DMibExpr(bint(true)));
			DMibTest(DMibExpr(Synced) == DMibExpr(_New));
		}
		static TCVector<uint8> fs_ToVector(const ch8 *_pStr)
		{
			mint Len = NStr::fg_StrLen(_pStr);
			TCVector<uint8> Ret;
			Ret.f_Insert((uint8 const *)_pStr, Len);
			
			return Ret;			
		}
		static void fs_DoTests(uint32 _MinChunk, uint32 _MaxChunk)
		{
			DMibTestCategory(NStr::CStr(NStr::CStr::CFormat("{} -> {}") << _MaxChunk << _MinChunk))
			{
				DMibTestSuite("Same dual")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("aaa aaa"), CRSync_Tests::fs_ToVector("aaa aaa"), _MinChunk, _MaxChunk);
				};
				DMibTestSuite("Same")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing 123"), CRSync_Tests::fs_ToVector("Testing 123"), _MinChunk, _MaxChunk);
				};
				DMibTestSuite("Addition")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing 123"), CRSync_Tests::fs_ToVector("Testing New! 123"), _MinChunk, _MaxChunk);
				};
				DMibTestSuite("Same longer")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing New! 123"), CRSync_Tests::fs_ToVector("Testing New! 123"), _MinChunk, _MaxChunk);
				};
				DMibTestSuite("Deletion")
				{
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector("Testing New! 123"), CRSync_Tests::fs_ToVector("Testing 123"), _MinChunk, _MaxChunk);
				};
				DMibTestSuite("Long")
				{
					NStr::CStr LongStr;
					LongStr.f_AddChars('a', 1024*1024);
					CRSync_Tests::fs_DoRSync(CRSync_Tests::fs_ToVector(""), CRSync_Tests::fs_ToVector(LongStr), _MinChunk, _MaxChunk);
				};
			};
		};

		static void fs_TestFiles(NStr::CStr const &_StartFile, NStr::CStr const &_EndFile)
		{
			NStr::CStr StartFileName = NFile::CFile::fs_GetProgramDirectory() + "/" + _StartFile;
			NStr::CStr EndFileName = NFile::CFile::fs_GetProgramDirectory() + "/" + _EndFile;
			if (NFile::CFile::fs_FileExists(StartFileName) && NFile::CFile::fs_FileExists(EndFileName))
			{
				fs_DoRSync(NFile::CFile::fs_ReadFile(StartFileName), NFile::CFile::fs_ReadFile(EndFileName), 128, 1024*1024);
			}
		}

		void f_DoTests()
		{
			DMibTestCategory("Basic")
			{
				fs_DoTests(1,1);
				fs_DoTests(2,2);
				fs_DoTests(1,2);
				fs_DoTests(1,4);
				fs_DoTests(1,8);
				fs_DoTests(1,16);
				fs_DoTests(1,32);
				fs_DoTests(2,32);
				fs_DoTests(4,32);
				fs_DoTests(8,32);
				fs_DoTests(16,32);
				fs_DoTests(32,32);
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

