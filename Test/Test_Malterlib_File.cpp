// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/Test/Exception>

using namespace NMib::NContainer;
using namespace NMib::NStr;
using namespace NMib::NFile;
#ifdef DPlatformFamily_Windows
#include "stdlib.h"
extern CWStr fg_ConvertToWindowsPath(const CStr &_Path, bint _bAddCurrentDir = true, aint _MaxPath = -1, bool _bTryShorten = true);
extern CStr fg_ConvertFromWindowsPath(const CWStr &_Path);
extern CStr fg_ConvertFromWindowsPath(const CStr &_Path);
#endif

namespace 
{
	class CFile_Tests : public NMib::NTest::CTest
	{
	public:


		class CProgress : public CFileProgress
		{
		public:
			bool m_bCalled;

			CProgress()
				: m_bCalled(false)
			{}

			void f_Progress(CMibFilePos _Position, CMibFilePos _TotalSize)
			{
				m_bCalled = true;
			}

		};

		static CStr fs_GetFullPath(ch8 const *_pPath, ch8 const *_pBase)
		{
			return CFile::fs_GetFullPath(CStr(_pPath), CStr(_pBase));
		}
		static CStr fs_GetExpandedPath(ch8 const *_pPath, bint _bAddCurrentDir = true)
		{
			return CFile::fs_GetExpandedPath(CStr(_pPath), _bAddCurrentDir);
		}
		static bint fs_IsPathAbsolute(ch8 const *_pPath)
		{
			return CFile::fs_IsPathAbsolute(CStr(_pPath));
		}
		static CStr fs_MakePathRelative(ch8 const *_pAbsolutePath, ch8 const *_pAbsoluteBase)
		{
			return CFile::fs_MakePathRelative(CStr(_pAbsolutePath), CStr(_pAbsoluteBase));
		}
		static CStr fs_GetPath(ch8 const *_pFile)
		{
			return CFile::fs_GetPath(CStr(_pFile));
		}
		static CStr fs_GetMalterlibPath(ch8 const *_pFile)
		{
			return CFile::fs_GetMalterlibPath(CStr(_pFile));
		}
		static CStr fs_AppendPath(ch8 const *_pPath, ch8 const *_pAppend)
		{
			return CFile::fs_AppendPath(CStr(_pPath), CStr(_pAppend));
		}
		static CStr fs_RemovePathTopLevels(ch8 const *_pPath, mint _Levels)
		{
			return CFile::fs_RemovePathTopLevels(CStr(_pPath), _Levels);
		}
		static CStr fs_GetFile(ch8 const *_pFile)
		{
			return CFile::fs_GetFile(CStr(_pFile));
		}
		static CStr fs_GetExtension(ch8 const *_pFile)
		{
			return CFile::fs_GetExtension(CStr(_pFile));
		}
		static CStr fs_GetDrive(ch8 const *_pFile)
		{
			return CFile::fs_GetDrive(CStr(_pFile));
		}
		static CStr fs_GetFileNoExt(ch8 const *_pFile)
		{
			return CFile::fs_GetDrive(CStr(_pFile));
		}

		void f_DoTests()
		{
			DMibTestSuite("Nice filename")
			{
				CStr Error;
				DMibTest(!DMibExpr(CFile::fs_IsValidFilePath("com1", Error)));
				DMibTest(!DMibExpr(CFile::fs_IsValidFilePath("CoM1", Error)));
				DMibTest(!DMibExpr(CFile::fs_IsValidFilePath("COM1", Error)));
				DMibTest(!DMibExpr(CFile::fs_IsValidFilePath("COM2", Error)));
				DMibTest(DMibExpr(CFile::fs_IsValidFilePath("COM", Error)));
				DMibTest(DMibExpr(CFile::fs_IsValidFilePath("com.hansoft", Error)));
				DMibTest(DMibExpr(CFile::fs_IsValidFilePath("hansoft.com", Error)));
			};
			DMibTestSuite("Nice filename")
			{

				CStr FileName = CFile::fs_MakeNiceUniqueFilename("com");
				FileName = CFile::fs_MakeNiceUniqueFilename("com");
				FileName = CFile::fs_MakeNiceUniqueFilename(".com.");
				FileName = CFile::fs_MakeNiceFilename("com");
				FileName = CFile::fs_MakeNiceFilename("com");
				FileName = CFile::fs_MakeNiceFilename(".com.");

			};


			DMibTestSuite("Default paths")
			{
				DMibTest(DMibExpr(CFile::fs_GetProgramPath()) == DMibExpr(CFile::fs_GetProgramPathNonTracked()));
				DMibTest(DMibExpr(CFile::fs_GetProgramDirectory()) == DMibExpr(CFile::fs_GetProgramDirectoryNonTracked()));
				DMibTest(DMibExpr(CFile::fs_GetCurrentDirectory()) == DMibExpr(CFile::fs_GetCurrentDirectoryNonTracked()));
				DMibTest(DMibExpr(CFile::fs_GetUserProgramDirectory()) == DMibExpr(CFile::fs_GetUserProgramDirectoryNonTracked()));
				DMibTest(DMibExpr(CFile::fs_GetUserLocalProgramDirectory()) == DMibExpr(CFile::fs_GetUserLocalProgramDirectoryNonTracked()));
				DMibTest(DMibExpr(CFile::fs_GetUserLocalProgramCacheDirectory()) == DMibExpr(CFile::fs_GetUserLocalProgramCacheDirectoryNonTracked()));
				DMibTest(DMibExpr(CFile::fs_GetTemporaryDirectory()) == DMibExpr(CFile::fs_GetTemporaryDirectoryNonTracked()));
				DMibTest(DMibExpr(CFile::fs_GetModulePath((void *)&CFile_Tests::fs_GetFileNoExt)) == DMibExpr(CFile::fs_GetModulePathNonTracked((void *)&CFile_Tests::fs_GetFileNoExt)));
				DMibTest(DMibExpr(CFile::fs_GetProgramRoot()) == DMibExpr(CFile::fs_GetProgramRootNonTracked()));
				DMibTest(DMibExpr(CFile::fs_GetUserHomeDirectory()) == DMibExpr(CFile::fs_GetUserHomeDirectoryNonTracked()));

				DMibTest(DMibExpr(CFile::fs_GetModulePath((void *)&CFile_Tests::fs_GetFileNoExt)) == DMibExpr(CFile::fs_GetProgramPath()));

			};

			DMibTestSuite("Path utils")
			{
				CStr CurrentDir = CFile::fs_GetCurrentDirectory();
				CStr CurrentDrive = CFile::fs_GetDrive(CurrentDir);

				DMibTest(DMibExpr(fs_GetDrive("./Test")) == DMibExpr(""));
				DMibTest(DMibExpr(fs_GetDrive("//Test")) == DMibExpr("//Test"));
				DMibTest(DMibExpr(fs_GetDrive("//Test/")) == DMibExpr("//Test"));
				DMibTest(DMibExpr(fs_GetDrive("//Test/Testing")) == DMibExpr("//Test"));
				DMibTest(DMibExpr(fs_GetDrive("//Test/Testing/Testing")) == DMibExpr("//Test"));
				DMibTest(DMibExpr(fs_GetDrive("D:/")) == DMibExpr("D:"));
				DMibTest(DMibExpr(fs_GetDrive("D:")) == DMibExpr("D:"));
				DMibTest(DMibExpr(fs_GetDrive("D")) == DMibExpr(""));
				DMibTest(DMibExpr(fs_GetDrive("")) == DMibExpr(""));
				
				DMibTest(DMibExpr(fs_GetExpandedPath("/Test")) == DMibExpr(CurrentDrive + "/Test"));
				DMibTest(DMibExpr(fs_GetExpandedPath("/Test/Test1")) == DMibExpr(CurrentDrive + "/Test/Test1"));
				DMibTest(DMibExpr(fs_GetExpandedPath("../Test")) == DMibExpr(CFile::fs_GetExpandedPath(CurrentDir + "/../Test")));
				DMibTest(DMibExpr(fs_GetExpandedPath("./Test")) == DMibExpr(CurrentDir + "/Test"));
				DMibTest(DMibExpr(fs_GetExpandedPath("D:/./Test")) == DMibExpr("D:/Test"));
				DMibTest(DMibExpr(fs_GetExpandedPath("D:/../Test")) == DMibExpr("D:/Test"));
				DMibTest(DMibExpr(fs_GetExpandedPath("D:/Testing1/../Test")) == DMibExpr("D:/Test"));
				DMibTest(DMibExpr(fs_GetExpandedPath("D:/Testing1/Testing2/../Test")) == DMibExpr("D:/Testing1/Test"));
				DMibTest(DMibExpr(fs_GetExpandedPath("//Testing/./Test")) == DMibExpr("//Testing/Test"));
				DMibTest(DMibExpr(fs_GetExpandedPath("//Testing/../Test")) == DMibExpr("//Testing/Test"));
				DMibTest(DMibExpr(fs_GetExpandedPath("//Testing/Testing1/../Test")) == DMibExpr("//Testing/Test"));
				DMibTest(DMibExpr(fs_GetExpandedPath("//Testing/Testing1/Testing2/../Test")) == DMibExpr("//Testing/Testing1/Test"));
				DMibTest(DMibExpr(fs_GetExpandedPath("//Testing/Testing1/Testing2/")) == DMibExpr("//Testing/Testing1/Testing2"));
				DMibTest(DMibExpr(fs_GetExpandedPath("//Testing/Testing1//Testing2/")) == DMibExpr("//Testing/Testing1/Testing2"));

				DMibTest(DMibExpr(CFile::fs_CondensePath("/Fred/Wilma")) == DMibExpr("/Fred/Wilma"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("Fred/Wilma")) == DMibExpr("Fred/Wilma"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("/Fred/Wilma/")) == DMibExpr("/Fred/Wilma/"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("/Fred/Wilma/../Barney/BamBam")) == DMibExpr("/Fred/Barney/BamBam"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("/Fred/..")) == DMibExpr(""));
				DMibTest(DMibExpr(CFile::fs_CondensePath("/Fred/Wilma/../Barney/../BamBam")) == DMibExpr("/Fred/BamBam"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("/Fred/Wilma/../Barney/../BamBam/")) == DMibExpr("/Fred/BamBam/"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("Projects/Malterlib/Library/Core/Proj/MSVC/../../../GUI/Malterlib_GUI.h")) == DMibExpr("Projects/Malterlib/Library/GUI/Malterlib_GUI.h"));
#ifdef DPlatformFamily_Windows
				DMibTest(DMibExpr(CFile::fs_CondensePath("C:/Fred/Wilma/../Barney/../BamBam/")) == DMibExpr("C:/Fred/BamBam/"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("C:/Fred/Wilma")) == DMibExpr("C:/Fred/Wilma"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("C:\\Fred\\Wilma")) == DMibExpr("C:/Fred/Wilma"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("//?/C:/Fred/Wilma")) == DMibExpr("//?/C:/Fred/Wilma"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("//?/UNC/MyComp/Fred\\Wilma")) == DMibExpr("//?/UNC/MyComp/Fred/Wilma"));
				DMibTest(DMibExpr(CFile::fs_CondensePath("//MyComp/Fred\\Wilma")) == DMibExpr("//MyComp/Fred/Wilma"));
				
#endif

				DMibTest(DMibExpr(fs_MakePathRelative("/Fred/Wilma/BamBam.txt", "/Fred/Barney")) == DMibExpr("../Wilma/BamBam.txt"));
				DMibTest(DMibExpr(fs_MakePathRelative("/Fred/Wilma/Betty/BamBam.txt", "/Fred")) == DMibExpr("Wilma/Betty/BamBam.txt"));
				DMibTest(DMibExpr(fs_MakePathRelative("/Fred/BamBam.txt", "/Fred/Wilma/Betty")) == DMibExpr("../../BamBam.txt"));
				DMibTest(DMibExpr(fs_MakePathRelative("/Fred/BamBam.txt", "/Fred")) == DMibExpr("BamBam.txt"));
#ifdef DPlatformFamily_Windows
				DMibTest(DMibExpr(fs_MakePathRelative("C:/Fred/Wilma/BamBam.txt", "C:/Fred/Barney")) == DMibExpr("../Wilma/BamBam.txt"));
				DMibTest(DMibExpr(fs_MakePathRelative("//?/C:/Fred/Wilma/BamBam.txt", "//?/C:/Fred/Barney")) == DMibExpr("../Wilma/BamBam.txt"));
				DMibTest(DMibExpr(fs_MakePathRelative("//?/C:/Fred/Wilma/BamBam.txt", "C:/Fred/Barney")) == DMibExpr("../Wilma/BamBam.txt"));
				DMibTest(DMibExpr(fs_MakePathRelative("//?/UNC/MyComp/Fred/Wilma/BamBam.txt", "//?/UNC/MyComp/Fred/Barney")) == DMibExpr("../Wilma/BamBam.txt"));
				DMibTest(DMibExpr(fs_MakePathRelative("//MyComp/Fred/Wilma/BamBam.txt", "//MyComp/Fred/Barney")) == DMibExpr("../Wilma/BamBam.txt"));
#endif

			};

	#ifdef DPlatformFamily_Windows
			DMibTestSuite("Windows paths")
			{
				CStr CurrentDir = CFile::fs_GetCurrentDirectory();
				CStr CurrentDrive = CFile::fs_GetDrive(CFile::fs_GetCurrentDirectory());

				if (CurrentDrive[1] == ':')
				{
					DMibTest(DMibExpr(fg_ConvertToWindowsPath("/Test")) == DMibExpr("\\\\?\\" + CurrentDrive.f_ReplaceChar('/', '\\') + "\\Test"));
					DMibTest(DMibExpr(fg_ConvertToWindowsPath("/Test/Test1")) == DMibExpr("\\\\?\\" + CurrentDrive.f_ReplaceChar('/', '\\') + "\\Test\\Test1"));
					DMibTest(DMibExpr(fg_ConvertToWindowsPath("../Test")) == DMibExpr("\\\\?\\" + CFile::fs_GetExpandedPath(CurrentDir + "/../Test").f_ReplaceChar('/', '\\')));
					DMibTest(DMibExpr(fg_ConvertToWindowsPath("./Test")) == DMibExpr("\\\\?\\" + CurrentDir.f_ReplaceChar('/', '\\') + "\\Test"));
				}
				else
				{
					CurrentDrive = CurrentDrive.f_Extract(2);
					DMibTest(DMibExpr(fg_ConvertToWindowsPath("/Test")) == DMibExpr("\\\\?\\UNC\\" + CurrentDrive.f_ReplaceChar('/', '\\') + "\\Test"));
					DMibTest(DMibExpr(fg_ConvertToWindowsPath("/Test/Test1")) == DMibExpr("\\\\?\\UNC\\" + CurrentDrive.f_ReplaceChar('/', '\\') + "\\Test\\Test1"));
					DMibTest(DMibExpr(fg_ConvertToWindowsPath("../Test")) == DMibExpr("\\\\?\\UNC\\" + CFile::fs_GetExpandedPath(CurrentDir + "/../Test").f_Extract(2).f_ReplaceChar('/', '\\')));
					DMibTest(DMibExpr(fg_ConvertToWindowsPath("./Test")) == DMibExpr("\\\\?\\UNC\\" + CurrentDir.f_Extract(2).f_ReplaceChar('/', '\\') + "\\Test"));
				}
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("D:/./Test")) == DMibExpr("\\\\?\\D:\\Test"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("D:/../Test")) == DMibExpr("\\\\?\\D:\\Test"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("D:/Testing1/../Test")) == DMibExpr("\\\\?\\D:\\Test"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("D:/Testing1/Testing2/../Test")) == DMibExpr("\\\\?\\D:\\Testing1\\Test"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//Testing/./Test")) == DMibExpr("\\\\?\\UNC\\Testing\\Test"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//Testing/../Test")) == DMibExpr("\\\\?\\UNC\\Testing\\Test"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//Testing/Testing1/../Test")) == DMibExpr("\\\\?\\UNC\\Testing\\Test"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//Testing/Testing1/Testing2/../Test")) == DMibExpr("\\\\?\\UNC\\Testing\\Testing1\\Test"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//Testing/Testing1/Testing2/")) == DMibExpr("\\\\?\\UNC\\Testing\\Testing1\\Testing2"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//Testing/Testing1//Testing2/")) == DMibExpr("\\\\?\\UNC\\Testing\\Testing1\\Testing2"));

				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//?/UNC/Testing/Testing1/Testing2/")) == DMibExpr("\\\\?\\UNC\\Testing\\Testing1\\Testing2"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//?/D:/Testing1")) == DMibExpr("\\\\?\\D:\\Testing1"));

				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//?/UNC/Testing/Testing1/Testing2", true, _MAX_PATH)) == DMibExpr("\\\\Testing\\Testing1\\Testing2"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//?/D:/Testing1", true, _MAX_PATH)) == DMibExpr("D:\\Testing1"));

				DMibTest(DMibExpr(fg_ConvertToWindowsPath("//Testing/Testing1/Testing2", true, _MAX_PATH)) == DMibExpr("\\\\Testing\\Testing1\\Testing2"));
				DMibTest(DMibExpr(fg_ConvertToWindowsPath("D:/Testing1", true, _MAX_PATH)) == DMibExpr("D:\\Testing1"));

				DMibTest(DMibExpr(fg_ConvertFromWindowsPath(CStr("\\\\?\\UNC\\Testing\\Testing1\\Testing2"))) == DMibExpr("//Testing/Testing1/Testing2"));
				DMibTest(DMibExpr(fg_ConvertFromWindowsPath(CStr("\\\\?\\D:\\Testing1"))) == DMibExpr("D:/Testing1"));
			};

			DMibTestSuite("Windows long paths")
			{
				if (CFile::fs_FileExists(CFile::fs_GetCurrentDirectory() + "/FileTest", EFileAttrib_Directory))
					CFile::fs_DeleteDirectoryRecursive(CFile::fs_GetCurrentDirectory() + "/FileTest", true);
				CStr TestPath =  CFile::fs_GetCurrentDirectory() + "/FileTest/";
				while (TestPath.f_GetLen() < 1024)
				{
					if (CFile::fs_GetFile(TestPath).f_GetLen() > 1)
						TestPath += "/";
					TestPath += "A";
					CStr ExceptionMessage;
					try
					{
						CFile::fs_CreateDirectory(CFile::fs_GetPath(TestPath));
						{
							CFile File;
							if (TestPath.f_GetLen() == 260)
							{
								int x = 0;
							}
							File.f_Open(TestPath, EFileOpen_Write);
							File.f_SetAttributes(EFileAttrib_Executable);
						}
						CFile::fs_DeleteFile(TestPath);
					}
					catch (NMib::NException::CException const &_Exception)
					{
						ExceptionMessage = _Exception.f_GetErrorStr();
					}
					DMibTest(DMibExpr(ExceptionMessage) == DMibExpr(""))(ETestFlag_Aggregated);
				}

				CFile::fs_DeleteDirectoryRecursive(CFile::fs_GetCurrentDirectory() + "/FileTest");
			};
	#endif
			for (mint i = 0; i < 2; ++i)
			{
				CStr TestName = "File functions";
				if (i == 1)
					TestName = "File functions with long names";
				DMibTestSuite(TestName)
				{
					CStr CurrentDir = CFile::fs_GetCurrentDirectory() + "/FileTest";
					if (CFile::fs_FileExists(CurrentDir, EFileAttrib_Directory))
						CFile::fs_DeleteDirectoryRecursive(CurrentDir, true);
					CStr TestFileName;
					CStr FirstTestFileDir;
					if (i == 1)
					{
						CStr SubDir = "/TestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTest";
						TestFileName = CurrentDir;
						FirstTestFileDir = CurrentDir + SubDir;
						for (mint i = 0; i < 32; ++i)
						{

							#if defined(DPlatformFamily_Linux) || defined(DPlatformFamily_OSX) //  TODO: Should max path length be exposed via Malterlib? Probably.
								if ( (TestFileName.f_GetLen() + SubDir.f_GetLen()) > (1024 - 14) )
									break;
							#endif
								
							TestFileName += SubDir;
						}
						TestFileName += "/Testfile.txt";
					}
					else
						TestFileName = CurrentDir + "/Testfile.txt";

					CStr TestFileName2 = CFile::fs_GetPath(TestFileName) + "/Testfile2.txt";
					CStr TestFileName3 = CFile::fs_GetPath(TestFileName) + "/Testfile3.txt";

					CStr TestFileDir = CFile::fs_GetPath(TestFileName);

					if (CFile::fs_FileExists(TestFileName))
					{
						CFile::fs_DeleteFile(TestFileName);
						DMibTest(DMibExpr(!CFile::fs_FileExists(TestFileName))) (NMib::NTest::ETest_FailAndStop);
					}
					if (!CFile::fs_FileExists(TestFileDir, EFileAttrib_Directory))
					{
						CFile::fs_CreateDirectory(TestFileDir);
						DMibTest(DMibExpr(CFile::fs_FileExists(TestFileDir, EFileAttrib_Directory))) (NMib::NTest::ETest_FailAndStop);
					}

					{
						{
							CFile::fs_CreateDirectory(TestFileDir + "/TestDir");
						}
						
						CFileChangeNotification FileChangeNotification;
#ifdef DPlatformFamily_OSX
						if (i == 0)
							FileChangeNotification.f_Open(TestFileDir.f_UpperCase(), EFileChange_Recursive | EFileChange_Write | EFileChange_FileName, nullptr);
						else
#endif
							FileChangeNotification.f_Open(TestFileDir, EFileChange_Recursive | EFileChange_Write | EFileChange_FileName, nullptr);
						FileChangeNotification.f_Close();

						CStr NotificationDir = i == 0 ? TestFileDir : FirstTestFileDir;
						FileChangeNotification.f_Open(NotificationDir, EFileChange_Recursive | EFileChange_Write | EFileChange_FileName, nullptr);

						CFileChangeNotification FileChangeNotificationNonRecursive;
						FileChangeNotificationNonRecursive.f_Open(NotificationDir, EFileChange_Write | EFileChange_FileName, nullptr);
						

						{
							CFile File;
							File.f_Open(TestFileName, EFileOpen_Write | EFileOpen_ShareAll);
							File.f_Write("Testing", 7);
							File.f_SetAttributes(EFileAttrib_ReadOnly);
							DMibTest((DMibExpr(File.f_GetAttributes()) & DMibExpr(EFileAttrib_ReadOnly | EFileAttrib_Executable)) == DMibExpr(EFileAttrib_ReadOnly));
							File.f_SetAttributes(EFileAttrib_Executable);
							DMibTest((DMibExpr(File.f_GetAttributes()) & DMibExpr(EFileAttrib_ReadOnly | EFileAttrib_Executable)) == DMibExpr(EFileAttrib_Executable));
							File.f_SetAttributes(EFileAttrib_Executable | EFileAttrib_ReadOnly);
							DMibTest((DMibExpr(File.f_GetAttributes()) & DMibExpr(EFileAttrib_ReadOnly | EFileAttrib_Executable)) == DMibExpr(EFileAttrib_ReadOnly | EFileAttrib_Executable));
							File.f_SetAttributes(EFileAttrib_None);
							DMibTest((DMibExpr(File.f_GetAttributes()) & DMibExpr(EFileAttrib_ReadOnly | EFileAttrib_Executable)) == DMibExpr(EFileAttrib_None));
							File.f_Close();
						}
						{
							CFile File;
							File.f_Open(TestFileName, EFileOpen_Read | EFileOpen_ShareAll);
							ch8 Temp[8];
							NMib::NMem::fg_MemClear(Temp);
							File.f_Read(Temp, 7);
							File.f_Close();
							DMibTest(DMibExpr(fg_StrCmp(Temp, "Testing") == 0));
						}

						{
							CFileChangeNotification FileChangeNotificationFile;
							DMibTest(DMibExpr(TCThrowsException<NMib::NFile::CExceptionFile>()) == DMibLExpr(FileChangeNotificationFile.f_Open(TestFileName, EFileChange_Write | EFileChange_FileName, nullptr);));
						}

						int32 MaxTries = 10000;
						bint bFoundNotification = false;
						bool bFiredNonRecursive = false;
						while (--MaxTries)
						{
							CFileChangeNotification::CNotification Notification;
							if (FileChangeNotification.f_GetNotification(Notification))
							{
								if (NotificationDir + "/" + Notification.m_Path == TestFileName)
								{
									bFoundNotification = true;
								}
							}

							if (FileChangeNotificationNonRecursive.f_GetNotification(Notification))
							{
								bFiredNonRecursive = true;
							}

							if (bFoundNotification && (i == 1 || bFiredNonRecursive))
								break;
							
							NMib::NSys::fg_Thread_Sleep(0.001f);
						}
						DMibTest(DMibExpr(bFoundNotification));
						if (i == 0)
							DMibTest(DMibExpr(bFiredNonRecursive));
						else
							DMibTest(!DMibExpr(bFiredNonRecursive));
						FileChangeNotification.f_Close();
						FileChangeNotificationNonRecursive.f_Close();
						
						CFile::fs_DeleteDirectory(TestFileDir + "/TestDir");


						DMibTest(DMibExpr(MaxTries > 0));

						TCVector<CStr> Files = CFile::fs_FindFiles(CFile::fs_GetPath(TestFileName) + "/*", EFileAttrib_File, true);

						DMibTest(DMibExpr(Files.f_Contains(TestFileName) >= 0));
					}

					{
						DMibTestPath("File attrib");
						CFile File;
						File.f_Open(TestFileName, EFileOpen_Write);
						File.f_Write("Testing", 7);
						File.f_SetAttributes(EFileAttrib_ReadOnly);
						DMibTest((DMibExpr(File.f_GetAttributes()) & DMibExpr(EFileAttrib_ReadOnly | EFileAttrib_Executable)) == DMibExpr(EFileAttrib_ReadOnly));
						File.f_SetAttributes(EFileAttrib_None);
						DMibTest((DMibExpr(File.f_GetAttributes()) & DMibExpr(EFileAttrib_ReadOnly | EFileAttrib_Executable)) == DMibExpr(EFileAttrib_None));
						File.f_Close();
					}
					{
						DMibTestPath("Read back");
						CFile File;
						File.f_Open(TestFileName, EFileOpen_Read);
						ch8 Temp[8];
						NMib::NMem::fg_MemClear(Temp);
						File.f_Read(Temp, 7);
						File.f_Close();
						DMibTest(DMibExpr(fg_StrCmp(Temp, "Testing") == 0));
					}


					// Copy
					{
						DMibTestPath("Copy");
						CFile::fs_CopyFile(TestFileName, TestFileName2);
						DMibTest(DMibExpr(CFile::fs_FileExists(TestFileName2, EFileAttrib_File)));
						DMibTest(DMibExpr(TCThrowsException<NMib::NFile::CExceptionFile>()) == DMibLExpr(CFile::fs_CopyFile(TestFileName + "NotExists", TestFileName2)));

						CFile::fs_DeleteFile(TestFileName2);
						DMibTest(DMibExpr(!CFile::fs_FileExists(TestFileName2, EFileAttrib_File)));
					}

					// Copy Progress
					{
						DMibTestPath("Copy progress");
						CProgress Progress;
						CFile::fs_CopyFile(TestFileName, TestFileName2, Progress);
						DMibTest(DMibExpr(CFile::fs_FileExists(TestFileName2, EFileAttrib_File)));
#ifdef DPlatformFamily_OSX
						DMibTest(DMibExpr(Progress.m_bCalled) == DMibExpr(true));
#endif
						DMibTest(DMibExpr(TCThrowsException<NMib::NFile::CExceptionFile>()) == DMibLExpr(CFile::fs_CopyFile(TestFileName + "NotExists", TestFileName2, Progress)));
						CFile::fs_DeleteFile(TestFileName2);
						DMibTest(DMibExpr(!CFile::fs_FileExists(TestFileName2, EFileAttrib_File)));
					}

					// Copy Raw
					{
						DMibTestPath("Copy raw");
						CFile::fs_CopyFileRaw(TestFileName, TestFileName2);
						DMibTest(DMibExpr(CFile::fs_FileExists(TestFileName2, EFileAttrib_File)));
						CFile::fs_DeleteFile(TestFileName2);
						DMibTest(DMibExpr(!CFile::fs_FileExists(TestFileName2, EFileAttrib_File)));
					}

					// Copy Diff
					{
						DMibTestPath("Copy diff");
						CFile::fs_CopyFileDiff(TestFileName, TestFileName2, true);
						DMibTest(DMibExpr(CFile::fs_FileExists(TestFileName2, EFileAttrib_File)));
						CFile::fs_DeleteFile(TestFileName2);
						DMibTest(DMibExpr(!CFile::fs_FileExists(TestFileName2, EFileAttrib_File)));
					}

					// Rename file
					{
						DMibTestPath("Rename");
						CFile::fs_CopyFile(TestFileName, TestFileName2);
						DMibTest(DMibExpr(CFile::fs_FileExists(TestFileName2, EFileAttrib_File)));

						CFile::fs_RenameFile(TestFileName2, TestFileName3);
						DMibTest(DMibExpr(CFile::fs_FileExists(TestFileName3, EFileAttrib_File)));

						DMibTest(DMibExpr(TCThrowsException<NMib::NFile::CExceptionFile>()) == DMibLExpr(CFile::fs_RenameFile(TestFileName2 + "NotExist", TestFileName3)));

						CFile::fs_DeleteFile(TestFileName3);
						DMibTest(DMibExpr(!CFile::fs_FileExists(TestFileName3, EFileAttrib_File)));
					}

					// Rename directory
					{
						DMibTestPath("Rename");
						CStr TestDir = TestFileDir + "/RenameDir";
						CStr TestDirRename = TestFileDir + "/RenameDir2";
						CFile::fs_CreateDirectory(TestDir);
						CFile::fs_CopyFile(TestFileName, TestDir + "/TestFile");
						
						DMibTest(DMibExpr(CFile::fs_FileExists(TestDir, EFileAttrib_Directory)));

						CFile::fs_RenameFile(TestDir, TestDirRename);
						DMibTest(!DMibExpr(CFile::fs_FileExists(TestDir)));
						DMibTest(!DMibExpr(CFile::fs_FileExists(TestDir + "/TestFile")));
						DMibTest(DMibExpr(CFile::fs_FileExists(TestDirRename, EFileAttrib_Directory)));
						DMibTest(DMibExpr(CFile::fs_FileExists(TestDirRename + "/TestFile")));

						if (CFile::fs_FileExists(TestDir))
							CFile::fs_DeleteDirectoryRecursive(TestDir);
						
						if (CFile::fs_FileExists(TestDirRename))
							CFile::fs_DeleteDirectoryRecursive(TestDirRename);
						DMibTest(DMibExpr(!CFile::fs_FileExists(TestDirRename)));
					}

					// Rename process
					{
						DMibTestPath("Rename progress");

						CProgress Progress;
						CFile::fs_CopyFile(TestFileName, TestFileName2);
						DMibTest(DMibExpr(CFile::fs_FileExists(TestFileName2, EFileAttrib_File)));

						CFile::fs_RenameFile(TestFileName2, TestFileName3, Progress);
						DMibTest(DMibExpr(CFile::fs_FileExists(TestFileName3, EFileAttrib_File)));
#ifdef DPlatformFamily_OSX
						DMibTest(DMibExpr(Progress.m_bCalled) == DMibExpr(true));
#endif
						
						DMibTest(DMibExpr(TCThrowsException<NMib::NFile::CExceptionFile>()) == DMibLExpr(CFile::fs_RenameFile(TestFileName2 + "NotExist", TestFileName3, Progress)));

						CFile::fs_DeleteFile(TestFileName3);
						DMibTest(DMibExpr(!CFile::fs_FileExists(TestFileName3, EFileAttrib_File)));
					}

					// Free Space
					{
						DMibTest(DMibExpr(CFile::fs_GetFreeSpace(TestFileDir)) != DMibExpr(0));
					}

					// File Size
					{
						DMibTest(DMibExpr(CFile::fs_GetFileSize(TestFileName)) == DMibExpr(7));
					}
					

					{
						DMibTestPath("Delete");
						CFile::fs_DeleteFile(TestFileName);
						DMibTest(DMibExpr(!CFile::fs_FileExists(TestFileName)));
					}

					if (!FirstTestFileDir.f_IsEmpty())
					{
						CFile::fs_DeleteDirectoryRecursive(FirstTestFileDir);
						DMibTest(DMibExpr(!CFile::fs_FileExists(FirstTestFileDir, EFileAttrib_Directory)));
					}
				};
			}

			CStr ProtectedFolder;
			CStr UnprotectedFolder;
			CStr ProtectedButExecutableExe;

			#ifdef DPlatformFamily_Windows
				ProtectedFolder = "C:\\Windows";
				UnprotectedFolder = CFile::fs_GetProgramDirectory();
				ProtectedButExecutableExe = "C:\\Windows\\notepad.exe";
			#else
				//#warning File Permissions tests not implemented for this platform!
			#endif

			DMibTestSuite(CTestCategory("Other volume") << CTestGroup("Manual"))
			{
				CStr TestFileDir = CFile::fs_GetCurrentDirectory() + "/FileTest";
				CStr TestFileName = TestFileDir + "/TestFile";
				CFile::fs_CreateDirectory(TestFileDir);
				{
					CFile File;
					File.f_Open(TestFileName, EFileOpen_Write | EFileOpen_ShareAll);
					File.f_Write("Testing", 7);
					File.f_Close();
				}
				
				CStr RenameDest = CFile::fs_GetTemporaryDirectory();
				
				// Rename directory
				{
					DMibTestPath("Rename Directory");
					CStr TestDir = TestFileDir + "/RenameDir";
					CStr TestDirRename = RenameDest + "/RenameDir2";
					if (CFile::fs_FileExists(TestDirRename))
						CFile::fs_DeleteDirectoryRecursive(TestDirRename);
					
					CFile::fs_CreateDirectory(RenameDest);
					CFile::fs_CreateDirectory(TestDir);
					CFile::fs_CopyFile(TestFileName, TestDir + "/TestFile");
					
					DMibTest(DMibExpr(TCThrowsException<NMib::NFile::CExceptionFile>()) == DMibLExpr(CFile::fs_RenameFile(TestDir, TestDirRename)));

					DMibTest(DMibExpr(!CFile::fs_FileExists(TestDirRename)));
					
					if (CFile::fs_FileExists(TestDir))
						CFile::fs_DeleteDirectoryRecursive(TestDir);
					
					DMibTest(DMibExpr(!CFile::fs_FileExists(TestDir)));
				}
				// Rename File
				{
					DMibTestPath("Rename File");
					CStr TestDir = TestFileDir + "/RenameDir";
					CStr TestDirRename = RenameDest + "/RenameDir2";
					if (CFile::fs_FileExists(TestDirRename))
						CFile::fs_CreateDirectory(TestDirRename);
					
					CFile::fs_CreateDirectory(TestDir);
					CFile::fs_CopyFile(TestFileName, TestDir + "/TestFile");
					CFile::fs_CreateDirectory(TestDirRename);
					CFile::fs_RenameFile(TestDir + "/TestFile", TestDirRename + "/TestFile");
					
					DMibTest(DMibExpr(!CFile::fs_FileExists(TestDir + "/TestFile")));
					DMibTest(DMibExpr(CFile::fs_FileExists(TestDirRename + "/TestFile")));
					
					if (CFile::fs_FileExists(TestDir))
						CFile::fs_DeleteDirectoryRecursive(TestDir);
					if (CFile::fs_FileExists(TestDirRename))
						CFile::fs_DeleteDirectoryRecursive(TestDirRename);
					
					DMibTest(DMibExpr(!CFile::fs_FileExists(TestDir)));
				}
			};
			
			DMibTestSuite("Permissions")
			{

				if (!ProtectedFolder.f_IsEmpty())
				{
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedFolder, EFileRight_Read)) == DMibExpr((bint)true));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedFolder, EFileRight_Write)) == DMibExpr((bint)false));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedFolder, EFileRight_Write | EFileRight_Read)) == DMibExpr((bint)false));
				}

				if (!UnprotectedFolder.f_IsEmpty())
				{
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(UnprotectedFolder, EFileRight_Read)) == DMibExpr((bint)true));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(UnprotectedFolder, EFileRight_Write)) == DMibExpr((bint)true));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(UnprotectedFolder, EFileRight_Write | EFileRight_Read)) == DMibExpr((bint)true));
				}

				if (!ProtectedButExecutableExe.f_IsEmpty())
				{
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedButExecutableExe, EFileRight_Read)) == DMibExpr((bint)true));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedButExecutableExe, EFileRight_Write)) == DMibExpr((bint)false));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedButExecutableExe, EFileRight_Write | EFileRight_Read)) == DMibExpr((bint)false));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedButExecutableExe, EFileRight_Execute)) == DMibExpr((bint)true));
				}

			};
			
			DMibTestCategory("Ownership")
			{
				DMibTestSuite("Get Ownership")
				{
#ifdef DPlatformFamily_OSX
					DMibTest(DMibExpr(CFile::fs_GetOwner("/dev")) == DMibExpr("root"));
					DMibTest(DMibExpr(CFile::fs_GetGroup("/dev")) == DMibExpr("wheel"));
					DMibTest(DMibExpr(TCThrowsException<NMib::NException::CException>()) == DMibLExpr(CFile::fs_GetOwner("/_HansoftPathThatDoesNotExist")));
					DMibTest(DMibExpr(TCThrowsException<NMib::NException::CException>()) == DMibLExpr(CFile::fs_GetGroup("/_HansoftPathThatDoesNotExist")));
#endif
				};

			DMibTestSuite(CTestCategory("Set Ownership") << CTestGroup("Manual"))
			{
				CStr TestFileName = CFile::fs_GetCurrentDirectory() + "/MalterlibSetOwnershipTestFile";
				CStr TestDirectory = CFile::fs_GetCurrentDirectory() + "/TestDirectory";
				CStr TestDirectoryFile = TestDirectory + "/MalterlibSetOwnershipTestFile";
				CStr TestDirectoryLink = TestDirectory + "/MalterlibSetOwnershipTestLink";
				
				if (CFile::fs_FileExists(TestDirectoryLink))
					CFile::fs_DeleteFile(TestDirectoryLink);
				if (CFile::fs_FileExists(TestDirectoryFile))
					CFile::fs_DeleteFile(TestDirectoryFile);
				if (CFile::fs_FileExists(TestDirectory))
					CFile::fs_DeleteDirectory(TestDirectory);

				
#ifdef DPlatformFamily_OSX
				{
					DMibTestPath("Non-Existant");
					DMibTest(DMibExpr(TCThrowsException<NMib::NException::CException>()) == DMibLExpr(CFile::fs_SetOwner("/_HansoftPathThatDoesNotExist", "root")));
					DMibTest(DMibExpr(TCThrowsException<NMib::NException::CException>()) == DMibLExpr(CFile::fs_SetGroup("/_HansoftPathThatDoesNotExist", "wheel")));
					DMibTest(DMibExpr(TCThrowsException<NMib::NException::CException>()) == DMibLExpr(CFile::fs_SetOwner("/dev", "NonExistingHansoftMalterlibTestUser")));
					DMibTest(DMibExpr(TCThrowsException<NMib::NException::CException>()) == DMibLExpr(CFile::fs_SetGroup("/dev", "NonExistingHansoftMalterlibTestUser")));
				}
					
				{
					CFile File;
					File.f_Open(TestFileName, EFileOpen_Write | EFileOpen_ShareAll);
					File.f_Write("Testing", 7);
					File.f_Close();
				}

				{
					DMibTestPath("File");
					DMibTest(DMibExpr(TCThrowsException<>()) == DMibLExpr(CFile::fs_SetOwner(TestFileName, "nobody")));
					DMibTest(DMibExpr(CFile::fs_GetOwner(TestFileName)) == DMibExpr("nobody"));
					DMibTest(DMibExpr(TCThrowsException<>()) == DMibLExpr(CFile::fs_SetGroup(TestFileName, "nobody")));
					DMibTest(DMibExpr(CFile::fs_GetGroup(TestFileName)) == DMibExpr("nobody"));
				}
				
				{
					DMibTestPath("Recursive on File");
					DMibTest(DMibExpr(TCThrowsException<>()) == DMibLExpr(CFile::fs_SetOwnerRecursive(TestFileName, "root")));
					DMibTest(DMibExpr(CFile::fs_GetOwner(TestFileName)) == DMibExpr("root"));
					DMibTest(DMibExpr(TCThrowsException<>()) == DMibLExpr(CFile::fs_SetGroup(TestFileName, "wheel")));
					DMibTest(DMibExpr(CFile::fs_GetGroup(TestFileName)) == DMibExpr("wheel"));
				}

				{
					CFile::fs_DeleteFile(TestFileName);
				}
				
				{
					CFile::fs_CreateDirectory(TestDirectory);
					CFile File;
					File.f_Open(TestDirectoryFile, EFileOpen_Write | EFileOpen_ShareAll);
					File.f_Write("Testing", 7);
					File.f_Close();
					CFile::fs_CreateSymbolicLink(TestDirectoryFile, TestDirectoryLink, EFileAttrib_File, ESymbolicLinkFlag_None);
				}
				
				{
					DMibTestPath("Recursive on Directory");
					DMibTest(DMibExpr(TCThrowsException<>()) == DMibLExpr(CFile::fs_SetOwnerRecursive(TestDirectoryLink, "nobody", true)));
					DMibTest(DMibExpr(CFile::fs_GetOwner(TestDirectoryFile)) == DMibExpr("nobody"));
					DMibTest(DMibExpr(TCThrowsException<>()) == DMibLExpr(CFile::fs_SetGroupRecursive(TestDirectoryLink, "nobody", true)));
					DMibTest(DMibExpr(CFile::fs_GetGroup(TestDirectoryFile)) == DMibExpr("nobody"));
				}			

				{
					CFile::fs_DeleteFile(TestDirectoryLink);
					CFile::fs_DeleteFile(TestDirectoryFile);
					CFile::fs_DeleteDirectory(TestDirectory);
				}

#endif
			};
				
			};


			DMibTestSuite("LockFile_NormalFile")
			{
				if (!UnprotectedFolder.f_IsEmpty())
				{
					{
						CLockFile LockFile(UnprotectedFolder + "/TestLockFile");

						DMibTest(DMibExpr(LockFile.f_Lock()) == DMibExpr( CLockFile::ELockResult_Locked ));

							// Ideally we spawn another process to verify the lock here.

						LockFile.f_Unlock();
					}
				}
			};
			DMibTestSuite("LockFile_ProtectedFile")
			{
				if (!ProtectedFolder.f_IsEmpty())
				{
					{
						CLockFile LockFile(ProtectedFolder + "/TestLockFile");

						DMibTest(DMibExpr(LockFile.f_Lock()) == DMibExpr( CLockFile::ELockResult_NoAccess ));

						LockFile.f_Unlock();
					}
				}
			};
			DMibTestSuite("LockFile_DirectoryLock")
			{
				if (!UnprotectedFolder.f_IsEmpty())
				{
					CStr LockDir = UnprotectedFolder + "/LockDir";

					if (!CFile::fs_FileExists(LockDir, EFileAttrib_Directory))
					{
						CFile::fs_CreateDirectory(LockDir);
					}

					CLockFile LockFile(LockDir, EFileOpen_Write | EFileOpen_Directory);

					DMibTest(DMibExpr(LockFile.f_Lock()) == DMibExpr( CLockFile::ELockResult_Locked ));

						// Ideally we spawn another process to verify the lock here.

					LockFile.f_Unlock();
				}
			};

			DMibTestSuite("Find")
			{
				CStr CurrentDir = CFile::fs_GetCurrentDirectory() + "/FileTestFind";
				if (CFile::fs_FileExists(CurrentDir))
					CFile::fs_DeleteDirectoryRecursive(CurrentDir, true);
				CFile::fs_CreateDirectory(CurrentDir);
				CFile::fs_CreateDirectory(CurrentDir + "/Dir1");
				CFile::fs_CreateDirectory(CurrentDir + "/Dir2");
				CFile::fs_CreateDirectory(CurrentDir + "/Dir3");
				CFile::fs_CreateDirectory(CurrentDir + "/Dir2/Dir1");
				{
					CFile File;
					File.f_Open(CurrentDir + "/Dir2/Dir1/TestFile", EFileOpen_Write | EFileOpen_ShareAll);
					File.f_Write("DATA", 4);
				}
				{
					CFile File;
					File.f_Open(CurrentDir + "/Dir2/TestFile", EFileOpen_Write | EFileOpen_ShareAll);
					File.f_Write("DATA", 4);
				}
				{
					CFile File;
					File.f_Open(CurrentDir + "/Dir1/TestFile", EFileOpen_Write | EFileOpen_ShareAll);
					File.f_Write("DATA", 4);
				}

				auto fGetFiles = [](TCVector<CFile::CFoundFile> const &_Files) -> TCVector<CStr>
					{
						TCVector<CStr> Files;

						for (auto &File : _Files)
							Files.f_Insert(File.m_Path);

						return Files;
					}
				;

				{
					DMibTestPath("Case0");
					CFile::CFindFilesOptions Options(CurrentDir + "/*", true);

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/Dir1/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir3");

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
				{
					DMibTestPath("Case1");
					CFile::CFindFilesOptions Options(CurrentDir + "/*", true);

					Options.m_ExcludePatterns = fg_CreateVector<CStr>("*/Dir2");

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir3");

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
				{
					DMibTestPath("Case2");
					CFile::CFindFilesOptions Options(CurrentDir + "/TestFile", true);

					Options.m_ExcludePatterns = fg_CreateVector<CStr>("*/Dir2");

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1/TestFile");

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
				{
					DMibTestPath("Case3");
					CFile::CFindFilesOptions Options(CurrentDir + "/TestFile", true);

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/Dir1/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/TestFile");

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
				{
					DMibTestPath("Case4");
					CFile::CFindFilesOptions Options(CurrentDir + "/Dir1", true);

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/Dir1");

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
				{
					DMibTestPath("Case5");
					CFile::CFindFilesOptions Options(CurrentDir + "/*", true);

					Options.m_ExcludePatterns = fg_CreateVector<CStr>("*/Dir2/Dir1/*");

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir3");

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
				{
					DMibTestPath("Case6");
					CFile::CFindFilesOptions Options(CurrentDir + "/*", false);

					Options.m_ExcludePatterns = fg_CreateVector<CStr>("*/Dir2/Dir1/*");
					Options.m_bRecursive = true;
					Options.m_AttribMask = EFileAttrib_File;

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/TestFile");

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
				{
					DMibTestPath("Case7");
					CFile::CFindFilesOptions Options(CurrentDir + "/*", true);

					Options.m_ExcludePatterns = fg_CreateVector<CStr>("*/Dir2/Dir1/*");
					Options.m_AttribMask |= EFileAttrib_FindDirectoryLast;

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/TestFile");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir3");

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
			};

			DMibTestSuite("Symlink")
			{
				CStr CurrentDir = CFile::fs_GetCurrentDirectory() + "/FileTest2";
				if (CFile::fs_FileExists(CurrentDir))
					CFile::fs_DeleteDirectoryRecursive(CurrentDir, true);
				CFile::fs_CreateDirectory(CurrentDir);

				CFile::fs_CreateDirectory(CurrentDir + "/TestDir");
				{
					CFile File;
					File.f_Open(CurrentDir + "/TestFile", EFileOpen_Write | EFileOpen_ShareAll);
					File.f_Write("DATA", 4);
				}
				{
					CFile File;
					File.f_Open(CurrentDir + "/TestDir/TestFile", EFileOpen_Write | EFileOpen_ShareAll);
					File.f_Write("DATA", 4);
				}

				auto fl_FileContainData
					= [&](CStr const &_File) -> bool
					{
						if (!CFile::fs_FileExists(_File, EFileAttrib_File))
							return false;
						CFile File;
						File.f_Open(_File, EFileOpen_Read | EFileOpen_ShareAll);
						ch8 Data[5];
						Data[4] = 0;
						File.f_Read(Data, 4);
						return NMib::NStr::fg_StrCmp(Data, "DATA") == 0;
					}
				;

				{
					DMibTestPath("Absolute");
					{
						auto Flags = CFile::fs_CanCreateSymbolicLink(EFileAttrib_Directory, ESymbolicLinkFlag_None) ? ESymbolicLinkFlag_None : ESymbolicLinkFlag_AllowEmulation;
						CStr DirSymLinkDestination = CurrentDir + "/TestDirSym";
						CFile::fs_CreateSymbolicLink(CurrentDir + "/TestDir", DirSymLinkDestination, EFileAttrib_Directory, ESymbolicLinkFlag_None | Flags);
						CStr DirSymLink = CFile::fs_ResolveSymbolicLink(DirSymLinkDestination);
						DMibTest(DMibExpr(DirSymLink) == DMibExpr(CurrentDir + "/TestDir"));
						DMibTest(DMibExpr(CFile::fs_GetAttributes(DirSymLinkDestination)) & DMibExpr(EFileAttrib_Directory));
						auto FoundFiles = CFile::fs_FindFilesEx(DirSymLinkDestination, EFileAttrib_Link, false, false);
						DMibTest(DMibExpr(FoundFiles.f_GetLen()) == DMibExpr(1));
						if (!FoundFiles.f_IsEmpty())
							DMibTest((DMibExpr(FoundFiles[0].m_Attribs) & DMibExpr(EFileAttrib_Link | EFileAttrib_Directory)) == DMibExpr(EFileAttrib_Link | EFileAttrib_Directory));
						
						if (!Flags)
							DMibTest(DMibExpr(fl_FileContainData(DirSymLinkDestination + "/TestFile")));
						
					}

					{
						auto Flags = CFile::fs_CanCreateSymbolicLink(EFileAttrib_File, ESymbolicLinkFlag_None) ? ESymbolicLinkFlag_None : ESymbolicLinkFlag_AllowEmulation;
						CStr FileSymLinkDestination = CurrentDir + "/TestFileSym";
						CFile::fs_CreateSymbolicLink(CurrentDir + "/TestFile", FileSymLinkDestination, EFileAttrib_File, ESymbolicLinkFlag_None | Flags);
						CStr FileSymLink = CFile::fs_ResolveSymbolicLink(FileSymLinkDestination);
						DMibTest(DMibExpr(FileSymLink) == DMibExpr(CurrentDir + "/TestFile"));
						DMibTest(!(DMibExpr(CFile::fs_GetAttributes(FileSymLinkDestination)) & DMibExpr(EFileAttrib_Directory)));
						auto FoundFilesFile = CFile::fs_FindFilesEx(FileSymLinkDestination, EFileAttrib_Link, false, false);
						DMibTest(DMibExpr(FoundFilesFile.f_GetLen()) == DMibExpr(1));
						if (!FoundFilesFile.f_IsEmpty())
							DMibTest((DMibExpr(FoundFilesFile[0].m_Attribs) & DMibExpr(EFileAttrib_Link | EFileAttrib_Directory)) == DMibExpr(EFileAttrib_Link));

						if (!Flags)
							DMibTest(DMibExpr(fl_FileContainData(FileSymLinkDestination)));
					}
				}
				{
					DMibTestPath("Relative");
					{
						auto Flags = CFile::fs_CanCreateSymbolicLink(EFileAttrib_Directory, ESymbolicLinkFlag_Relative) ? ESymbolicLinkFlag_None : ESymbolicLinkFlag_AllowEmulation;
						CStr DirSymLinkDestination = CurrentDir + "/TestDirSymRelative";
						CFile::fs_CreateSymbolicLink("TestDir", DirSymLinkDestination, EFileAttrib_Directory, ESymbolicLinkFlag_Relative | Flags);
						CStr DirSymLink = CFile::fs_ResolveSymbolicLink(DirSymLinkDestination);
						DMibTest(DMibExpr(DirSymLink) == DMibExpr("TestDir"));
						DMibTest(DMibExpr(CFile::fs_GetAttributes(DirSymLinkDestination)) & DMibExpr(EFileAttrib_Directory));
						auto FoundFiles = CFile::fs_FindFilesEx(DirSymLinkDestination, EFileAttrib_Link, false, false);
						DMibTest(DMibExpr(FoundFiles.f_GetLen()) == DMibExpr(1));
						if (!FoundFiles.f_IsEmpty())
							DMibTest((DMibExpr(FoundFiles[0].m_Attribs) & DMibExpr(EFileAttrib_Link | EFileAttrib_Directory)) == DMibExpr(EFileAttrib_Link | EFileAttrib_Directory));
						if (!Flags)
							DMibTest(DMibExpr(fl_FileContainData(DirSymLinkDestination + "/TestFile")));
					}

					{
						auto Flags = CFile::fs_CanCreateSymbolicLink(EFileAttrib_File, ESymbolicLinkFlag_Relative) ? ESymbolicLinkFlag_None : ESymbolicLinkFlag_AllowEmulation;
						CStr FileSymLinkDestination = CurrentDir + "/TestFileSymRelative";
						CFile::fs_CreateSymbolicLink("TestFile", FileSymLinkDestination, EFileAttrib_File, ESymbolicLinkFlag_Relative | Flags);
						CStr FileSymLink = CFile::fs_ResolveSymbolicLink(FileSymLinkDestination);
						DMibTest(DMibExpr(FileSymLink) == DMibExpr("TestFile"));
						DMibTest(!(DMibExpr(CFile::fs_GetAttributes(FileSymLinkDestination)) & DMibExpr(EFileAttrib_Directory)));
						auto FoundFilesFile = CFile::fs_FindFilesEx(FileSymLinkDestination, EFileAttrib_Link, false, false);
						DMibTest(DMibExpr(FoundFilesFile.f_GetLen()) == DMibExpr(1));
						if (!FoundFilesFile.f_IsEmpty())
							DMibTest((DMibExpr(FoundFilesFile[0].m_Attribs) & DMibExpr(EFileAttrib_Link | EFileAttrib_Directory)) == DMibExpr(EFileAttrib_Link));
						if (!Flags)
							DMibTest(DMibExpr(fl_FileContainData(FileSymLinkDestination)));
					}
				}
#ifdef DPlatformFamily_Windows
				{
					DMibTestPath("DevicePath");
					{
						auto Flags 
							= CFile::fs_CanCreateSymbolicLink(EFileAttrib_Directory, ESymbolicLinkFlag_ConvertToDevicePath) 
							? ESymbolicLinkFlag_None : ESymbolicLinkFlag_AllowEmulation
						;
						CStr DirSymLinkDestination = CurrentDir + "/TestDirSymDevicePath";
						CFile::fs_CreateSymbolicLink(CurrentDir + "/TestDir", DirSymLinkDestination, EFileAttrib_Directory, ESymbolicLinkFlag_ConvertToDevicePath | Flags);
						CStr DirSymLink = CFile::fs_ResolveSymbolicLink(DirSymLinkDestination);
						DMibTest(DMibExpr(DirSymLink) != DMibExpr(CurrentDir + "/TestDir"));
						DMibTest(DMibExpr(CFile::fs_GetAttributes(DirSymLinkDestination)) & DMibExpr(EFileAttrib_Directory));
						auto FoundFiles = CFile::fs_FindFilesEx(DirSymLinkDestination, EFileAttrib_Link, false, false);
						DMibTest(DMibExpr(FoundFiles.f_GetLen()) == DMibExpr(1));
						if (!FoundFiles.f_IsEmpty())
							DMibTest((DMibExpr(FoundFiles[0].m_Attribs) & DMibExpr(EFileAttrib_Link | EFileAttrib_Directory)) == DMibExpr(EFileAttrib_Link | EFileAttrib_Directory));
						if (!Flags)
							DMibTest(DMibExpr(fl_FileContainData(DirSymLinkDestination + "/TestFile")));
					}

					{
						auto Flags
							= CFile::fs_CanCreateSymbolicLink(EFileAttrib_File, ESymbolicLinkFlag_ConvertToDevicePath)
							? ESymbolicLinkFlag_None : ESymbolicLinkFlag_AllowEmulation
						;
						CStr FileSymLinkDestination = CurrentDir + "/TestFileSymDevicePath";
						CFile::fs_CreateSymbolicLink(CurrentDir + "/TestFile", FileSymLinkDestination, EFileAttrib_File, ESymbolicLinkFlag_ConvertToDevicePath | Flags);
						CStr FileSymLink = CFile::fs_ResolveSymbolicLink(FileSymLinkDestination);
						if (!Flags)
							DMibTest(DMibExpr(FileSymLink) != DMibExpr(CurrentDir + "/TestFile"));
						DMibTest(!(DMibExpr(CFile::fs_GetAttributes(FileSymLinkDestination)) & DMibExpr(EFileAttrib_Directory)));
						auto FoundFilesFile = CFile::fs_FindFilesEx(FileSymLinkDestination, EFileAttrib_Link, false, false);
						DMibTest(DMibExpr(FoundFilesFile.f_GetLen()) == DMibExpr(1));
						if (!FoundFilesFile.f_IsEmpty())
							DMibTest((DMibExpr(FoundFilesFile[0].m_Attribs) & DMibExpr(EFileAttrib_Link | EFileAttrib_Directory)) == DMibExpr(EFileAttrib_Link));
						if (!Flags)
							DMibTest(DMibExpr(fl_FileContainData(FileSymLinkDestination)));
					}
				}
#endif
				
				CFile::fs_DeleteDirectoryRecursive(CurrentDir);
			};

		}
	};
	
	DMibTestRegister(CFile_Tests, Malterlib::File);
}


