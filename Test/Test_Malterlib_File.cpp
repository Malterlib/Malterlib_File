// Copyright © 2015 Hansoft AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/Test/Exception>

using namespace NMib::NContainer;
using namespace NMib::NStr;
using namespace NMib::NFile;
using namespace NMib;
#ifdef DPlatformFamily_Windows
#include "stdlib.h"
#include <Mib/Core/PlatformSpecific/WindowsFilePath>
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
		static CStr fs_GetExpandedPath(ch8 const *_pPath, bool _bAddCurrentDir = true)
		{
			return CFile::fs_GetExpandedPath(CStr(_pPath), _bAddCurrentDir);
		}
		static bool fs_IsPathAbsolute(ch8 const *_pPath)
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

		static CStr fs_GetCommonPath(ch8 const *_pPath0, ch8 const *_pPath1)
		{
			return CFile::fs_GetCommonPath(CStr(_pPath0), CStr(_pPath1));
		}

		struct CCommonPathResult
		{
			auto operator <=> (CCommonPathResult const &) const = default;

			template <typename tf_CStr>
			void f_Format(tf_CStr &o_String) const
			{
				o_String += typename tf_CStr::CFormat("Path0: {} Path1: {} Common: {}") << m_Path0 << m_Path1 << m_Common;
			}

			CStr m_Common;
			CStr m_Path0;
			CStr m_Path1;
		};

		static CCommonPathResult fs_GetCommonPathAndMakeRelative(ch8 const *_pPath0, ch8 const *_pPath1)
		{
			CStr Path0 = _pPath0;
			CStr Path1 = _pPath1;
			CStr Common = CFile::fs_GetCommonPathAndMakeRelative(Path0, Path1);

			return CCommonPathResult
				{
					.m_Common = fg_Move(Common)
					, .m_Path0 = fg_Move(Path0)
					, .m_Path1 = fg_Move(Path1)
				}
			;
		}

		void f_DoTests()
		{
			DMibTestSuite("Valid path")
			{
				CStr Error;
				DMibTest(!DMibExpr(CFile::fs_IsValidFilePath("com1", Error)));
				DMibTest(!DMibExpr(CFile::fs_IsValidFilePath("CoM1", Error)));
				DMibTest(!DMibExpr(CFile::fs_IsValidFilePath("COM1", Error)));
				DMibTest(!DMibExpr(CFile::fs_IsValidFilePath("COM2", Error)));
				DMibTest(DMibExpr(CFile::fs_IsValidFilePath("COM", Error)));
				DMibTest(DMibExpr(CFile::fs_IsValidFilePath("com.malterlib", Error)));
				DMibTest(DMibExpr(CFile::fs_IsValidFilePath("malterlib.com", Error)));
				DMibTest(DMibExpr(CFile::fs_IsValidFilePath("C:/malterlib.com", Error)));
				DMibTest(DMibExpr(CFile::fs_IsValidFilePath("//./b:/malterlib.com/malterlib.com", Error)));
				DMibTest(DMibExpr(CFile::fs_IsValidFilePath("//?/a:/malterlib.com/malterlib.com", Error)));
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
				DMibTest(DMibExpr(CFile::fs_GetRawTemporaryDirectory()) == DMibExpr(CFile::fs_GetRawTemporaryDirectoryNonTracked()));
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

				DMibExpect(fs_GetCommonPath("/Fred/Wilma/BamBam.txt", "/Fred/Barney"), ==, "/Fred");
				DMibExpect(fs_GetCommonPath("/Fred/Barney", "/Fred/Wilma/BamBam.txt"), ==, "/Fred");
				DMibExpect(fs_GetCommonPath("/Fred/Wilma/BamBam.txt", "/Fred/Barney/"), ==, "/Fred");
				DMibExpect(fs_GetCommonPath("/Fred/Barney/", "/Fred/Wilma/BamBam.txt"), ==, "/Fred");
				DMibExpect(fs_GetCommonPath("/Fred/Barney\\", "/Fred\\Wilma/BamBam.txt"), ==, "/Fred");

				DMibExpect(fs_GetCommonPath("/Fred/Wilma/BamBam.txt", "/Fred/Wilma"), ==, "/Fred/Wilma");
				DMibExpect(fs_GetCommonPath("/Fred/Wilma", "/Fred/Wilma/BamBam.txt"), ==, "/Fred/Wilma");
				DMibExpect(fs_GetCommonPath("/Fred/Wilma/BamBam.txt", "/Fred/Wilma/"), ==, "/Fred/Wilma");
				DMibExpect(fs_GetCommonPath("/Fred/Wilma/", "/Fred/Wilma/BamBam.txt"), ==, "/Fred/Wilma");
				DMibExpect(fs_GetCommonPath("/Fred\\Wilma/", "/Fred/Wilma\\BamBam.txt"), ==, "/Fred\\Wilma");

				DMibExpect(fs_GetCommonPath("/Fred/Wilma/BamBam.txt", "/Fred/Wilma/BamBam.txt"), ==, "/Fred/Wilma/BamBam.txt");
				DMibExpect(fs_GetCommonPath("/Fred\\Wilma/BamBam.txt", "/Fred/Wilma\\BamBam.txt"), ==, "/Fred\\Wilma/BamBam.txt");

				DMibExpect(fs_GetCommonPath("/Wilma/Wilma/BamBam.txt", "/Fred/Wilma/BamBam.txt"), ==, "");
				DMibExpect(fs_GetCommonPath("/Wilma\\Wilma/BamBam.txt", "/Fred/Wilma\\BamBam.txt"), ==, "");

				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Fred/Wilma/BamBam.txt", "/Fred/Barney")
						, ==
						, (CCommonPathResult{.m_Common = "/Fred", .m_Path0 = "Wilma/BamBam.txt", .m_Path1 = "Barney"})
					)
				;
				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Fred/Barney", "/Fred/Wilma/BamBam.txt")
						, ==
						, (CCommonPathResult{.m_Common = "/Fred", .m_Path0 = "Barney", .m_Path1 = "Wilma/BamBam.txt"})
					)
				;
				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Fred/Wilma/BamBam.txt", "/Fred/Barney/")
						, ==
						, (CCommonPathResult{.m_Common = "/Fred", .m_Path0 = "Wilma/BamBam.txt", .m_Path1 = "Barney/"})
					)
				;
				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Fred/Barney/", "/Fred/Wilma/BamBam.txt")
						, ==
						, (CCommonPathResult{.m_Common = "/Fred", .m_Path0 = "Barney/", .m_Path1 = "Wilma/BamBam.txt"})
					)
				;
				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("\\Fred/Barney/", "/Fred/Wilma\\BamBam.txt")
						, ==
						, (CCommonPathResult{.m_Common = "\\Fred", .m_Path0 = "Barney/", .m_Path1 = "Wilma\\BamBam.txt"})
					)
				;
				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Fred/Barney/", "\\Fred/Wilma\\BamBam.txt")
						, ==
						, (CCommonPathResult{.m_Common = "/Fred", .m_Path0 = "Barney/", .m_Path1 = "Wilma\\BamBam.txt"})
					)
				;

				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Fred/Wilma/BamBam.txt", "/Fred/Wilma")
						, ==
						, (CCommonPathResult{.m_Common = "/Fred/Wilma", .m_Path0 = "BamBam.txt", .m_Path1 = ""})
					)
				;
				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Fred/Wilma", "/Fred/Wilma/BamBam.txt")
						, ==
						, (CCommonPathResult{.m_Common = "/Fred/Wilma", .m_Path0 = "", .m_Path1 = "BamBam.txt"})
					)
				;
				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Fred/Wilma/BamBam.txt", "/Fred/Wilma/")
						, ==
						, (CCommonPathResult{.m_Common = "/Fred/Wilma", .m_Path0 = "BamBam.txt", .m_Path1 = ""})
					)
				;
				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Fred/Wilma/", "/Fred/Wilma/BamBam.txt")
						, ==
						, (CCommonPathResult{.m_Common = "/Fred/Wilma", .m_Path0 = "", .m_Path1 = "BamBam.txt"})
					)
				;

				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Fred/Wilma/BamBam.txt", "/Fred/Wilma/BamBam.txt")
						, ==
						, (CCommonPathResult{.m_Common = "/Fred/Wilma/BamBam.txt", .m_Path0 = "", .m_Path1 = ""})
					)
				;

				DMibExpect
					(
						fs_GetCommonPathAndMakeRelative("/Wilma/Wilma/BamBam.txt", "/Fred/Wilma/BamBam.txt")
						, ==
						, (CCommonPathResult{.m_Common = "", .m_Path0 = "/Wilma/Wilma/BamBam.txt", .m_Path1 = "/Fred/Wilma/BamBam.txt"})
					)
				;
			};

	#ifdef DPlatformFamily_Windows
			DMibTestSuite("Windows paths")
			{
				CStr CurrentDir = CFile::fs_GetCurrentDirectory();
				CStr CurrentDrive = CFile::fs_GetDrive(CFile::fs_GetCurrentDirectory());

				if (CurrentDrive[1] == ':')
				{
					DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("/Test", true, -1))) == DMibExpr("\\\\?\\" + CurrentDrive.f_ReplaceChar('/', '\\') + "\\Test"));
					DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("/Test/Test1", true, -1))) == DMibExpr("\\\\?\\" + CurrentDrive.f_ReplaceChar('/', '\\') + "\\Test\\Test1"));
					DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("../Test", true, -1))) == DMibExpr("\\\\?\\" + CFile::fs_GetExpandedPath(CurrentDir + "/../Test").f_ReplaceChar('/', '\\')));
					DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("./Test", true, -1))) == DMibExpr("\\\\?\\" + CurrentDir.f_ReplaceChar('/', '\\') + "\\Test"));
				}
				else
				{
					CurrentDrive = CurrentDrive.f_Extract(2);
					DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("/Test", true, -1))) == DMibExpr("\\\\?\\UNC\\" + CurrentDrive.f_ReplaceChar('/', '\\') + "\\Test"));
					DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("/Test/Test1", true, -1))) == DMibExpr("\\\\?\\UNC\\" + CurrentDrive.f_ReplaceChar('/', '\\') + "\\Test\\Test1"));
					DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("../Test", true, -1))) == DMibExpr("\\\\?\\UNC\\" + CFile::fs_GetExpandedPath(CurrentDir + "/../Test").f_Extract(2).f_ReplaceChar('/', '\\')));
					DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("./Test", true, -1))) == DMibExpr("\\\\?\\UNC\\" + CurrentDir.f_Extract(2).f_ReplaceChar('/', '\\') + "\\Test"));
				}
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("D:/./Test", true, -1))) == DMibExpr("\\\\?\\D:\\Test"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("D:/../Test", true, -1))) == DMibExpr("\\\\?\\D:\\Test"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("D:/Testing1/../Test", true, -1))) == DMibExpr("\\\\?\\D:\\Test"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("D:/Testing1/Testing2/../Test", true, -1))) == DMibExpr("\\\\?\\D:\\Testing1\\Test"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//Testing/./Test", true, -1))) == DMibExpr("\\\\?\\UNC\\Testing\\Test"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//Testing/../Test", true, -1))) == DMibExpr("\\\\?\\UNC\\Testing\\Test"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//Testing/Testing1/../Test", true, -1))) == DMibExpr("\\\\?\\UNC\\Testing\\Test"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//Testing/Testing1/Testing2/../Test", true, -1))) == DMibExpr("\\\\?\\UNC\\Testing\\Testing1\\Test"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//Testing/Testing1/Testing2/", true, -1))) == DMibExpr("\\\\?\\UNC\\Testing\\Testing1\\Testing2"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//Testing/Testing1//Testing2/", true, -1))) == DMibExpr("\\\\?\\UNC\\Testing\\Testing1\\Testing2"));

				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//?/UNC/Testing/Testing1/Testing2/", true, -1))) == DMibExpr("\\\\?\\UNC\\Testing\\Testing1\\Testing2"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//?/D:/Testing1", true, -1))) == DMibExpr("\\\\?\\d:\\Testing1"));

				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//?/UNC/Testing/Testing1/Testing2", true, _MAX_PATH))) == DMibExpr("\\\\Testing\\Testing1\\Testing2"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//?/D:/Testing1", true, _MAX_PATH))) == DMibExpr("d:\\Testing1"));

				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("//Testing/Testing1/Testing2", true, _MAX_PATH))) == DMibExpr("\\\\Testing\\Testing1\\Testing2"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertToWindowsPath("D:/Testing1", true, _MAX_PATH))) == DMibExpr("D:\\Testing1"));

				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertFromWindowsPath(CStr("\\\\?\\UNC\\Testing\\Testing1\\Testing2")))) == DMibExpr("//Testing/Testing1/Testing2"));
				DMibTest(DMibExpr(NStr::CStr(NFile::NPlatform::fg_ConvertFromWindowsPath(CStr("\\\\?\\D:\\Testing1")))) == DMibExpr("d:/Testing1"));
			};

			DMibTestSuite("Windows long paths")
			{
				CStr RootPath = CFile::fs_GetProgramDirectory() / "FileTestWindowsLongPath";
				if (CFile::fs_FileExists(RootPath + "/FileTest", EFileAttrib_Directory))
					CFile::fs_DeleteDirectoryRecursive(RootPath + "/FileTest", true);
				CStr TestPath = RootPath + "/FileTest/";
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

				CFile::fs_DeleteDirectoryRecursive(RootPath + "/FileTest");
			};
	#endif
			DMibTestSuite("Tests")
			{
				CStr RootPath = CFile::fs_GetProgramDirectory() / "FileTestMain";
				fg_TestAddCleanupPath(RootPath);

				for (mint i = 0; i < 8; ++i)
				{
					CStr TestName = "File functions";
					bool bLongNames = i & 1;
					if (bLongNames)
						TestName = "File functions long names";
					bool bRecursive = i & 2;
					bool bSymlinkDir = i & 4;

					if (bRecursive)
						TestName += " recursive notifications";
					else
						TestName += " non recursive notifications";

					if (bSymlinkDir)
						TestName += " in symlinked dir";

					{
						DMibTestPath(TestName);
						CStr CurrentDir = RootPath;
						if (CFile::fs_FileExists(CurrentDir, EFileAttrib_Directory))
							CFile::fs_DeleteDirectoryRecursive(CurrentDir, true);

						if (bSymlinkDir)
						{
							CFile::fs_CreateDirectory(CurrentDir / "LinkTarget");
							CFile::fs_CreateSymbolicLink(CurrentDir / "LinkTarget", CurrentDir / "SymlinkDir", EFileAttrib_Directory, ESymbolicLinkFlag_None);
							CurrentDir = CurrentDir / "SymlinkDir/Dir";
						}

						CStr TestFileName;
						CStr FirstTestFileDir;
						if (bLongNames)
						{
							CStr SubDir = "/TestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTest";
							TestFileName = CurrentDir;
							FirstTestFileDir = CurrentDir + SubDir;
							for (mint i = 0; i < 32; ++i)
							{
								if (mint(TestFileName.f_GetLen() + SubDir.f_GetLen()) > (CFile::fs_MaximumPathLength() - 32u))
									break;

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
						if (NMib::NFile::CFileChangeNotification::fs_Supported())
						{
							{
								CFile::fs_CreateDirectory(TestFileDir + "/TestDir");
							}

							CFileChangeNotification FileChangeNotification;
	#ifdef DPlatformFamily_macOS
							if (!bLongNames)
								FileChangeNotification.f_Open(TestFileDir.f_UpperCase(), EFileChange_Recursive | EFileChange_Write | EFileChange_FileName, nullptr);
							else
	#endif
								FileChangeNotification.f_Open(TestFileDir, EFileChange_Recursive | EFileChange_Write | EFileChange_FileName, nullptr);
							FileChangeNotification.f_Close();

							CStr NotificationDir = !bLongNames ? TestFileDir : FirstTestFileDir;
							FileChangeNotification.f_Open(NotificationDir, EFileChange_Recursive | EFileChange_Write | EFileChange_FileName, nullptr);

							CFileChangeNotification FileChangeNotificationNonRecursive;
							FileChangeNotificationNonRecursive.f_Open(NotificationDir, EFileChange_Write | EFileChange_FileName, nullptr);


							{
								CFile File;
								File.f_Open(TestFileName, EFileOpen_Write | EFileOpen_ShareAll);
								File.f_Write("Testing", 7);
								DMibTest((DMibExpr(File.f_GetAttributes()) & DMibExpr(EFileAttrib_File)) == DMibExpr(EFileAttrib_File));
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
								NMib::NMemory::fg_MemClear(Temp);
								File.f_Read(Temp, 7);
								File.f_Close();
								DMibTest(DMibExpr(fg_StrCmp(Temp, "Testing") == 0));
							}

							{
								CFileChangeNotification FileChangeNotificationFile;
								DMibTest(DMibExpr(TCThrowsException<NMib::NFile::CExceptionFile>()) == DMibLExpr(FileChangeNotificationFile.f_Open(TestFileName, EFileChange_Write | EFileChange_FileName, nullptr);));
							}

							int32 MaxTries = 10000;
							bool bFoundNotification = false;
							bool bFiredNonRecursive = false;
							bool bFoundAnyNotification = false;
							while (--MaxTries)
							{
								CFileChangeNotification::CNotification Notification;
								if (FileChangeNotification.f_GetNotification(Notification))
								{
									bFoundAnyNotification = true;
									if (NotificationDir + "/" + Notification.m_Path == TestFileName)
									{
										bFoundNotification = true;
									}
								}

								if (FileChangeNotificationNonRecursive.f_GetNotification(Notification))
								{
									bFiredNonRecursive = true;
								}

								if (bFoundNotification && (bLongNames || bFiredNonRecursive))
									break;

								NMib::NSys::fg_Thread_Sleep(0.001f);
							}
							DMibExpectTrue(bFoundAnyNotification);
							DMibExpectTrue(bFoundNotification);
							if (!bLongNames)
								DMibExpectTrue(bFiredNonRecursive);
							else
								DMibExpectFalse(bFiredNonRecursive);
							FileChangeNotification.f_Close();
							FileChangeNotificationNonRecursive.f_Close();

							CFile::fs_DeleteDirectory(TestFileDir + "/TestDir");

							DMibExpect(MaxTries, >, 0);

							TCVector<CStr> Files = CFile::fs_FindFiles(CFile::fs_GetPath(TestFileName) + "/*", EFileAttrib_File, true);

							DMibExpect(Files.f_Contains(TestFileName), >=, 0);
						}

						bool bPollMacOS = false;
	#ifdef DPlatformFamily_macOS
						bPollMacOS = true;
	//					bPollMacOS = CSystem::ms_PlatformVersion < 10'07'00;
	#endif

						if (NMib::NFile::CFileChangeNotification::fs_Supported())
						{
							CStr TestDir = TestFileDir + "/TestDir2";
							if (CFile::fs_FileExists(TestDir))
								CFile::fs_DeleteDirectoryRecursive(TestDir);
							if (CFile::fs_FileExists(CFile::fs_GetPath(TestDir) + "/SubDir2"))
								CFile::fs_DeleteDirectoryRecursive(CFile::fs_GetPath(TestDir) + "/SubDir2");
							CFile::fs_CreateDirectory(TestDir);
	#ifdef DPlatformFamily_macOS
							if (CSystem::ms_PlatformVersion < 10'13'00)
							{
								NSys::fg_Thread_Sleep(1.5); // Wait for old notifications to be unqueued
							}
	#endif

							auto fSleep = [&]
								{
									if (CSystem::ms_PlatformVersion < 10'13'00)
									{
										if (bPollMacOS)
											NSys::fg_Thread_Sleep(1.5f);
									}
	#ifdef DPlatformFamily_Windows
									NSys::fg_Thread_Sleep(0.01f);
	#endif
								}
							;

							fSleep();

							NMib::NThread::CEventAutoReset Event;
							CFileChangeNotification FileChangeNotification;
							FileChangeNotification.f_Open(TestDir, bRecursive ? EFileChange_All : EFileChange_All & ~EFileChange_Recursive, &Event);

							bool bEnableLogs = (fg_TestReportFlags() & ETestReportFlag_EnableLogs) != 0;

							auto fTrace = [&](CStr const &_Change, CFileChangeNotification::CNotification const &_Notification)
								{
									if (!bEnableLogs)
										return;

									CStr NotName;
									switch (_Notification.m_Notification)
									{
									case EFileChangeNotification_Undefined: NotName = "Undefined"; break;
									case EFileChangeNotification_Unknown: NotName = "Unknown"; break;
									case EFileChangeNotification_Added:	NotName = "Added"; break;
									case EFileChangeNotification_Removed: NotName = "Removed"; break;
									case EFileChangeNotification_Modified: NotName = "Modified"; break;
									case EFileChangeNotification_Renamed: NotName = "Renamed"; break;
									}

									DMibConErrOut2("{}: {}: {} -> {}\n", _Change, NotName, _Notification.m_Path, _Notification.m_PathFrom);
								}
							;
							auto fWaitForChange = [&](CStr const &_Desc) -> CFileChangeNotification::CNotification
								{
									CFileChangeNotification::CNotification Notification;
									while (!FileChangeNotification.f_GetNotification(Notification))
									{
										DMibExpectFalse(Event.f_WaitTimeout(10.0))(ETest_FailAndStop)(ETestFlag_Aggregated);
									}

									fTrace(_Desc, Notification);

									return Notification;
								}
							;
							auto fTryWaitForChange = [&](CStr const &_Desc, CFileChangeNotification::CNotification &o_Notification)
								{
									while (!FileChangeNotification.f_GetNotification(o_Notification))
									{
										if (Event.f_WaitTimeout(2.0))
											return false;
									}

									fTrace(_Desc, o_Notification);

									return true;
								}
							;
							auto fHasChange = [&]() -> bool
								{
									CFileChangeNotification::CNotification Notification;
									bool bHasChange = false;
									while (FileChangeNotification.f_GetNotification(Notification))
									{
										fTrace("Extra change", Notification);
										bHasChange = true;
									}
									return bHasChange;
								}
							;

							CStr SubDir;
							if (bRecursive)
								SubDir = "SubDir";

							{
								DMibTestPath("Create directory");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_CreateDirectory(TestDir + "/SubDir");

								TCSet<CFileChangeNotification::CNotification> Notifications;
								Notifications[fWaitForChange("CreateDir: Change0")];
								Notifications[fWaitForChange("CreateDir: Change1")];
								DMibAssert(Notifications.f_GetLen(), ==, 2);

								auto iChange = Notifications.f_GetIterator();
								auto Change0 = *iChange;
								++iChange;
								auto Change1 = *iChange;
								++iChange;

								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Added);
								DMibExpect(Change0.m_Path, ==, "SubDir");

								DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change1.m_Path, ==, "");

								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Create file");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_Touch(TestDir / SubDir / "File.tst");

								TCSet<CFileChangeNotification::CNotification> Notifications;
								Notifications[fWaitForChange("CreateFile: Change0")];
								Notifications[fWaitForChange("CreateFile: Change1")];
								DMibAssert(Notifications.f_GetLen(), ==, 2);

								auto iChange = Notifications.f_GetIterator();
								auto Change0 = *iChange;
								++iChange;
								auto Change1 = *iChange;
								++iChange;

								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Added);
								DMibExpect(Change0.m_Path, ==, SubDir / "File.tst");

								DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change1.m_Path, ==, SubDir);

								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Touch file");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_Touch(TestDir / SubDir / "File.tst");

								auto Change0 = fWaitForChange("Touch file: Change0");
								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change0.m_Path, ==, SubDir / "File.tst");

								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Rename file");
								DMibExpectFalse(fHasChange() && "Before");

								CFile::fs_RenameFile(TestDir / SubDir / "File.tst", TestDir / "File2.tst");

	#ifdef DPlatformFamily_Windows
								if (bRecursive)
								{
									TCSet<CFileChangeNotification::CNotification> Notifications;
									Notifications[fWaitForChange("RenameFile: Change0")];
									Notifications[fWaitForChange("RenameFile: Change1")];
									Notifications[fWaitForChange("RenameFile: Change2")];
									Notifications[fWaitForChange("RenameFile: Change3")];
									DMibAssert(Notifications.f_GetLen(), ==, 4);

									auto iChange = Notifications.f_GetIterator();
									auto Change0 = *iChange;
									++iChange;
									auto Change1 = *iChange;
									++iChange;
									auto Change2 = *iChange;
									++iChange;
									auto Change3 = *iChange;
									++iChange;

									DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Added);
									DMibExpect(Change0.m_Path, ==, "File2.tst");

									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Removed);
									DMibExpect(Change1.m_Path, ==, SubDir / "File.tst");

									DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
									DMibExpect(Change2.m_Path, ==, "");

									DMibExpect(Change3.m_Notification, ==, EFileChangeNotification_Modified);
									DMibExpect(Change3.m_Path, ==, "SubDir");
								}
								else
	#endif
								{
									auto Change0 = fWaitForChange("RenameFile: Change0");
									DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Renamed);
									DMibExpect(Change0.m_Path, ==, "File2.tst");
									DMibExpect(Change0.m_PathFrom, ==, SubDir / "File.tst");
									TCSet<CFileChangeNotification::CNotification> Notifications;

									Notifications[fWaitForChange("RenameFile: Change1")];
									if (bRecursive)
									{
										Notifications[fWaitForChange("RenameFile: Change2")];
										DMibAssert(Notifications.f_GetLen(), ==, 2);
									}
									else
										DMibAssert(Notifications.f_GetLen(), ==, 1);

									auto iChange = Notifications.f_GetIterator();
									auto Change1 = *iChange;
									++iChange;

									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Modified);
									DMibExpect(Change1.m_Path, ==, "");

									if (bRecursive)
									{
										auto Change2 = *iChange;
										++iChange;
										DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(Change2.m_Path, ==, SubDir);
									}
		#ifdef DPlatformFamily_macOS
									if (CSystem::ms_PlatformVersion >= 10'13'00)
									{
										auto Change3 = fWaitForChange("RenameFile: Change3");
										DMibExpect(Change3.m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(Change3.m_Path, ==, "File2.tst");
									}
		#endif
								}

								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Hard link");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_CreateHardLink(TestDir / "File2.tst", TestDir / "File.tst");

								auto Change0 = fWaitForChange("CreateHardLink: Change0");
								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Added);
								DMibExpect(Change0.m_Path, ==, "File.tst");

								auto Change1 = fWaitForChange("CreateHardLink: Change1");
								DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change1.m_Path, ==, "");

								if (bPollMacOS)
								{
									auto Change2 = fWaitForChange("CreateHardLink: Change2");
									DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
									DMibExpect(Change2.m_Path, ==, "File2.tst");

									auto Change3 = fWaitForChange("CreateHardLink: Change3");
									DMibExpect(Change3.m_Notification, ==, EFileChangeNotification_Modified);
									DMibExpect(Change3.m_Path, ==, "File.tst");
								}
								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Touch hard link");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_Touch(TestDir + "/File.tst");
	#ifdef DPlatformFamily_macOS
								auto Change0 = fWaitForChange("TouchHardLink: Change0");
								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change0.m_Path, ==, "File2.tst");
	#endif

								auto Change1 = fWaitForChange("TouchHardLink: Change1");
								DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change1.m_Path, ==, "File.tst");


								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Rename hard link");
								DMibExpectFalse(fHasChange() && "Before");

								CFile::fs_RenameFile(TestDir / "File.tst", TestDir / "File4.tst");

	#ifdef DPlatformFamily_macOS
								if (CSystem::ms_PlatformVersion >= 10'13'00)
								{
									TCSet<CFileChangeNotification::CNotification> Notifications;
									auto Change0 = fWaitForChange("RenameHardLink: Change0");
									auto Change1 = fWaitForChange("RenameHardLink: Change1");
									auto Change2 = fWaitForChange("RenameHardLink: Change2");
									auto Change3 = fWaitForChange("RenameHardLink: Change3");
									Notifications[Change0];
									Notifications[Change1];
									Notifications[Change2];
									Notifications[Change3];
									if (Change0.m_Notification == EFileChangeNotification_Added)
									{
										auto Change4 = fWaitForChange("RenameHardLink: Change3");
										Notifications[Change4];

										auto NotificationsVector = TCVector<CFileChangeNotification::CNotification>::fs_FromContainer(Notifications);
										DMibExpect(NotificationsVector[0].m_Notification, ==, EFileChangeNotification_Added);
										DMibExpect(NotificationsVector[0].m_Path, ==, "File4.tst");
										DMibExpect(NotificationsVector[1].m_Notification, ==, EFileChangeNotification_Removed);
										DMibExpect(NotificationsVector[1].m_Path, ==, "File.tst");
										DMibExpect(NotificationsVector[2].m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(NotificationsVector[2].m_Path, ==, "");
										DMibExpect(NotificationsVector[3].m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(NotificationsVector[3].m_Path, ==, "File2.tst");
										DMibExpect(NotificationsVector[4].m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(NotificationsVector[4].m_Path, ==, "File4.tst");
									}
									else
									{
										auto NotificationsVector = TCVector<CFileChangeNotification::CNotification>::fs_FromContainer(Notifications);

										DMibExpect(NotificationsVector[0].m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(NotificationsVector[0].m_Path, ==, "");
										DMibExpect(NotificationsVector[1].m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(NotificationsVector[1].m_Path, ==, "File2.tst");
										DMibExpect(NotificationsVector[2].m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(NotificationsVector[2].m_Path, ==, "File4.tst");
										DMibExpect(NotificationsVector[3].m_Notification, ==, EFileChangeNotification_Renamed);
										DMibExpect(NotificationsVector[3].m_Path, ==, "File4.tst");
										DMibExpect(NotificationsVector[3].m_PathFrom, ==, "File.tst");
									}
								}
								else
	#endif
								{
									auto Change0 = fWaitForChange("RenameHardLink: Change0");
									DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Renamed);
									DMibExpect(Change0.m_Path, ==, "File4.tst");
									DMibExpect(Change0.m_PathFrom, ==, "File.tst");

									auto Change1 = fWaitForChange("RenameHardLink: Change1");
									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Modified);
									DMibExpect(Change1.m_Path, ==, "");
								}


								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Rename hard link to different dir");
								DMibExpectFalse(fHasChange() && "Before");

								CFile::fs_RenameFile(TestDir / "File2.tst", TestDir / SubDir / "File3.tst");

	#ifdef DPlatformFamily_Windows
								if (bRecursive)
								{
									TCSet<CFileChangeNotification::CNotification> Notifications;
									Notifications[fWaitForChange("RenameHardLink2: Change0")];
									Notifications[fWaitForChange("RenameHardLink2: Change1")];
									Notifications[fWaitForChange("RenameHardLink2: Change2")];
									Notifications[fWaitForChange("RenameHardLink2: Change3")];
									{
										while (true)
										{
											CFileChangeNotification::CNotification Notification;
											if (!fTryWaitForChange("RenameHardLink2: Change4", Notification))
												break;
											if (Notifications.f_FindEqual(Notification))
												continue;
											Notifications[Notification];
											break;
										}
									}

									mint ExpectedLen = 4;
									if (Notifications.f_GetLen() == 5)
										ExpectedLen = 5;

									DMibAssert(Notifications.f_GetLen(), ==, ExpectedLen);

									auto iNotification = Notifications.f_GetIterator();
									auto Change0 = *iNotification;
									++iNotification;
									auto Change1 = *iNotification;
									++iNotification;
									auto Change2 = *iNotification;
									++iNotification;
									auto Change3 = *iNotification;
									++iNotification;

									CFileChangeNotification::CNotification Change4;
									if (ExpectedLen == 5)
									{
										Change4 = *iNotification;
										++iNotification;
									}

									DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Added);
									DMibExpect(Change0.m_Path, ==, "SubDir/File3.tst");

									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Removed);
									DMibExpect(Change1.m_Path, ==, "File2.tst");

									DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
									DMibExpect(Change2.m_Path, ==, "");

									DMibExpect(Change3.m_Notification, == , EFileChangeNotification_Modified);
									DMibTest(DMibExpr(Change3.m_Path) == DMibExpr("SubDir") || DMibExpr(Change3.m_Path) == DMibExpr("File2.tst"));

									if (ExpectedLen == 5)
									{
										DMibExpect(Change4.m_Notification, == , EFileChangeNotification_Modified);
										DMibTest(DMibExpr(Change4.m_Path) == DMibExpr("SubDir/File3.tst") || DMibExpr(Change4.m_Path) == DMibExpr("SubDir"));
									}
								}
								else
								{
									TCSet<CFileChangeNotification::CNotification> Notifications;
									Notifications[fWaitForChange("RenameHardLink2: Change0")];
									Notifications[fWaitForChange("RenameHardLink2: Change1")];
									Notifications[fWaitForChange("RenameHardLink2: Change2")];
									DMibAssert(Notifications.f_GetLen(), ==, 3);
									auto iNotification = Notifications.f_GetIterator();
									auto Change0 = *iNotification;
									++iNotification;
									auto Change1 = *iNotification;
									++iNotification;
									auto Change2 = *iNotification;
									++iNotification;

									DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Modified);
									DMibExpect(Change0.m_Path, ==, "");

									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Modified);
									DMibTest(DMibExpr(Change1.m_Path) == DMibExpr("File3.tst") || DMibExpr(Change1.m_Path) == DMibExpr("File2.tst"));

									DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Renamed);
									DMibExpect(Change2.m_Path, ==, "File3.tst");
									DMibExpect(Change2.m_PathFrom, ==, "File2.tst");
								}
	#else
								{
									auto Change1 = fWaitForChange("RenameHardLink2: Change1");
									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Renamed);
									DMibExpect(Change1.m_Path, ==, SubDir / "File3.tst");
									DMibExpect(Change1.m_PathFrom, ==, "File2.tst");

									TCSet<CFileChangeNotification::CNotification> Notifications;
									Notifications[fWaitForChange("RenameHardLink2: Change2")];
									if (bRecursive)
									{
										Notifications[fWaitForChange("RenameHardLink2: Change3")];
										DMibAssert(Notifications.f_GetLen(), ==, 2);
									}
									else
										DMibAssert(Notifications.f_GetLen(), ==, 1);

									auto iChange = Notifications.f_GetIterator();
									auto Change2 = *iChange;
									++iChange;

									DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
									DMibExpect(Change2.m_Path, ==, "");

									if (bRecursive)
									{
										auto Change3 = *iChange;
										++iChange;
										DMibExpect(Change3.m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(Change3.m_Path, ==, "SubDir");
									}

	#ifdef DPlatformFamily_macOS
									if (CSystem::ms_PlatformVersion >= 10'13'00)
									{
										auto Change4 = fWaitForChange("RenameHardLink2: Change4");
										auto Change5 = fWaitForChange("RenameHardLink2: Change5");
										if (!bRecursive)
											fg_Swap(Change4, Change5);
										DMibExpect(Change4.m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(Change4.m_Path, ==, SubDir / "File3.tst");
										DMibExpect(Change5.m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(Change5.m_Path, ==, "File4.tst");
									}
	#endif
								}
	#endif

								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Rename subdir");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_RenameFile(TestDir + "/SubDir", TestDir + "/SubDir2");

								auto Change0 = fWaitForChange("RenameSubDir: Change0");
								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Renamed);
								DMibExpect(Change0.m_Path, ==, "SubDir2");
								DMibExpect(Change0.m_PathFrom, ==, "SubDir");

								if (bRecursive)
								{
									auto Change1 = fWaitForChange("RenameSubDir: Change1");
									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Renamed);
									DMibExpect(Change1.m_Path, ==, "SubDir2/File3.tst");
									DMibExpect(Change1.m_PathFrom, ==, "SubDir/File3.tst");
								}

								auto Change2 = fWaitForChange("RenameSubDir: Change2");
								DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change2.m_Path, ==, "");

	#ifdef DPlatformFamily_macOS
								if (CSystem::ms_PlatformVersion >= 10'13'00)
								{
									auto Change3 = fWaitForChange("RenameSubDir: Change2");
									DMibExpect(Change3.m_Notification, ==, EFileChangeNotification_Modified);
									DMibExpect(Change3.m_Path, ==, "SubDir2");
								}
	#endif
								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Delete hardlink");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_DeleteFile(TestDir + "/File4.tst");

								auto Change0 = fWaitForChange("Delete hardlink: Change0");
								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Removed);
								DMibExpect(Change0.m_Path, ==, "File4.tst");

								auto Change1 = fWaitForChange("Delete hardlink: Change1");
								DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change1.m_Path, ==, "");

	#ifdef DPlatformFamily_macOS
								if (CSystem::ms_PlatformVersion >= 10'13'00)
								{
									if (!bRecursive)
									{
										auto Change2 = fWaitForChange("Delete hardlink: Change2");
										DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(Change2.m_Path, ==, "File3.tst");
									}
								}
	#endif

								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Move directory outside tree");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_RenameFile(TestDir + "/SubDir2", CFile::fs_GetPath(TestDir) + "/SubDir2");

								auto Change0 = fWaitForChange("Move directory outside tree: Change0");
								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Removed);
								DMibExpect(Change0.m_Path, ==, "SubDir2");

								if (bRecursive)
								{
									auto Change1 = fWaitForChange("Move directory outside tree: Change1");
									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Removed);
									DMibExpect(Change1.m_Path, ==, "SubDir2/File3.tst");
								}

								auto Change2 = fWaitForChange("Move directory outside tree: Change1");
								DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change2.m_Path, ==, "");

								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Move directory into tree");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_RenameFile(CFile::fs_GetPath(TestDir) + "/SubDir2", TestDir + "/SubDir2");

								auto Change0 = fWaitForChange("Move directory into tree: Change0");
								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Added);
								DMibExpect(Change0.m_Path, ==, "SubDir2");

								if (bRecursive)
								{
									auto Change1 = fWaitForChange("Move directory into tree: Change1");
									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Added);
									DMibExpect(Change1.m_Path, ==, "SubDir2/File3.tst");
								}

								auto Change2 = fWaitForChange("Move directory into tree: Change1");
								DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change2.m_Path, ==, "");

								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Move directory outside tree 2");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_RenameFile(TestDir + "/SubDir2", CFile::fs_GetPath(TestDir) + "/SubDir2");

								auto Change0 = fWaitForChange("Move directory outside tree 2: Change0");
								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Removed);
								DMibExpect(Change0.m_Path, ==, "SubDir2");

								if (bRecursive)
								{
									auto Change1 = fWaitForChange("Move directory outside tree 2: Change1");
									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Removed);
									DMibExpect(Change1.m_Path, ==, "SubDir2/File3.tst");
								}

								auto Change2 = fWaitForChange("Move directory outside tree 2: Change1");
								DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
								DMibExpect(Change2.m_Path, ==, "");

								DMibExpectFalse(fHasChange() && "After");
							}
							fSleep();
							{
								DMibTestPath("Delete directory outside tree");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_DeleteDirectoryRecursive(CFile::fs_GetPath(TestDir) + "/SubDir2");
								fSleep();
								DMibExpectFalse(fHasChange() && "After");
							}
							// Test for inotify edge case: when a directory is moved from one location to another
							// within the watched tree, and the destination structure is created first (triggering
							// recursive watching), inotify_add_watch returns the same descriptor for the moved
							// directory because it's the same inode. This tests that notifications still work
							// correctly at the new location.
							if (bRecursive)
							{
								fSleep();
								{
									DMibTestPath("Move directory from SyncTemp to nested destination (inotify edge case)");

									// Create a source directory in a sibling location (like SyncTemp)
									CStr SyncTempDir = TestDir + "/SyncTemp";
									CFile::fs_CreateDirectory(SyncTempDir);
									CStr SourceDir = SyncTempDir + "/SourceDir";
									CFile::fs_CreateDirectory(SourceDir);

									// Create a file inside the source directory
									CStr SourceFile = SourceDir + "/TestFile.txt";
									CFile::fs_WriteStringToFile(SourceFile, "Initial content");

									// Drain notifications from setup
									while (true)
									{
										auto Notification = fWaitForChange("Setup");
										if (Notification.m_Notification == EFileChangeNotification_Added && Notification.m_Path == "SyncTemp/SourceDir/TestFile.txt")
											break;
									}

									while (fHasChange())
										; // The rest of modified

									DMibExpectFalse(fHasChange() && "Before");

									// Write to the source file to create notification backlog
									for (int i = 0; i < 10; ++i)
										CFile::fs_WriteStringToFile(SourceFile, CStr::CFormat("Content {}") << i);

									// Quickly move source directory into nested destination
									// This triggers the "Watch tree mismatch" scenario in inotify
									// Create nested destination structure inside watched tree
									// This will trigger recursive watching BEFORE the move completes
									CStr DestParent = TestDir + "/Dest/Nested/Path";
									CFile::fs_CreateDirectory(DestParent);

									CStr DestDir = DestParent + "/SourceDir";
									CFile::fs_RenameFile(SourceDir, DestDir);

									// Drain all notifications from the move
									// We need to wait for both file notifications AND directory notifications
									TCSet<CFileChangeNotification::CNotification> MoveNotifications;

									mint nFileFound = 0;
									mint nDirFound = 0;
									while (nFileFound < 2 || nDirFound < 2)
									{
										auto Notification = fWaitForChange("MoveFromSyncTemp");
										MoveNotifications[Notification];

										// File notification: need both Removed AND Added, or a single Renamed
										if (Notification.m_Notification == EFileChangeNotification_Removed && Notification.m_Path == "SyncTemp/SourceDir/TestFile.txt")
											++nFileFound;
										if (Notification.m_Notification == EFileChangeNotification_Added && Notification.m_Path == "Dest/Nested/Path/SourceDir/TestFile.txt")
											++nFileFound;
										if (Notification.m_Notification == EFileChangeNotification_Renamed && Notification.m_PathFrom == "SyncTemp/SourceDir/TestFile.txt")
											nFileFound += 2;

										// Directory notification: need both Removed AND Added, or a single Renamed
										if (Notification.m_Notification == EFileChangeNotification_Removed && Notification.m_Path == "SyncTemp/SourceDir")
											++nDirFound;
										if (Notification.m_Notification == EFileChangeNotification_Added && Notification.m_Path == "Dest/Nested/Path/SourceDir")
											++nDirFound;
										if (Notification.m_Notification == EFileChangeNotification_Renamed && Notification.m_PathFrom == "SyncTemp/SourceDir")
											nDirFound += 2;
									}

									while (fHasChange())
										; // The rest of modified

									// Verify we got Added notification for the destination path
									bool bFoundAdded = false;
									for (auto const &Notification : MoveNotifications)
									{
										if (Notification.m_Notification == EFileChangeNotification_Added &&
											Notification.m_Path.f_EndsWith("SourceDir"))
										{
											bFoundAdded = true;
											break;
										}
										if (Notification.m_Notification == EFileChangeNotification_Renamed &&
											Notification.m_Path.f_EndsWith("SourceDir"))
										{
											bFoundAdded = true;
											break;
										}
									}
									DMibExpectTrue(bFoundAdded);

									// Verify we got Removed notification for the source directory
									bool bFoundRemovedDir = false;
									for (auto const &Notification : MoveNotifications)
									{
										if (Notification.m_Notification == EFileChangeNotification_Removed &&
											Notification.m_Path == "SyncTemp/SourceDir")
										{
											bFoundRemovedDir = true;
											break;
										}
										if (Notification.m_Notification == EFileChangeNotification_Renamed &&
											Notification.m_PathFrom == "SyncTemp/SourceDir")
										{
											bFoundRemovedDir = true;
											break;
										}
									}
									DMibExpectTrue(bFoundRemovedDir);

									// Verify we got Removed notification for the child file (recursive)
									bool bFoundRemovedFile = false;
									for (auto const &Notification : MoveNotifications)
									{
										if (Notification.m_Notification == EFileChangeNotification_Removed &&
											Notification.m_Path == "SyncTemp/SourceDir/TestFile.txt")
										{
											bFoundRemovedFile = true;
											break;
										}
										if (Notification.m_Notification == EFileChangeNotification_Renamed &&
											Notification.m_PathFrom == "SyncTemp/SourceDir/TestFile.txt")
										{
											bFoundRemovedFile = true;
											break;
										}
									}
									DMibExpectTrue(bFoundRemovedFile);

									// Critical test: write to file at NEW location
									// Verify we receive modification notification
									fSleep();
									while (fHasChange()) {} // Drain any remaining notifications
									DMibExpectFalse(fHasChange() && "Before new location write");

									CStr NewFile = DestDir + "/TestFile.txt";
									CFile::fs_WriteStringToFile(NewFile, "Modified at new location");

									// Wait for the correct modification notification (skip root directory modifications)
									while (true)
									{
										auto ModifyChange = fWaitForChange("Modify at new location");
										if
										(	ModifyChange.m_Notification == EFileChangeNotification_Modified
											&& ModifyChange.m_Path == "Dest/Nested/Path/SourceDir/TestFile.txt"
										)
										{
											break;
										}
									}

									// Cleanup
									CFile::fs_DeleteDirectoryRecursive(TestDir + "/Dest");
									CFile::fs_DeleteDirectoryRecursive(SyncTempDir);

									// Drain cleanup notifications
									while (true)
									{
										auto Notification = fWaitForChange("Cleanup");
										if (Notification.m_Notification == EFileChangeNotification_Removed && Notification.m_Path == "SyncTemp")
											break;
									}
									while (fHasChange())
										; // The rest of modified
									DMibExpectFalse(fHasChange() && "AfterCleanup");
								}
							}
	#ifndef DPlatformFamily_Windows
							fSleep();
							{
								DMibTestPath("Delete self");
								DMibExpectFalse(fHasChange() && "Before");
								CFile::fs_DeleteDirectoryRecursive(TestDir);
								TCSet<CFileChangeNotification::CNotification> Notifications;
								Notifications[fWaitForChange("Delete self: Change0")];
								if (!bRecursive)
								{
									Notifications[fWaitForChange("Delete self: Change1")];

#ifndef DPlatformFamily_macOS
									CFileChangeNotification::CNotification Notification;
									if (fTryWaitForChange("Delete self: Change2", Notification))
										Notifications[Notification];

									DMibAssert(Notifications.f_GetLen(), >=, 2);
#else
									DMibAssert(Notifications.f_GetLen(), ==, 2);
#endif
								}
								else
									DMibAssert(Notifications.f_GetLen(), ==, 1);

								auto iChange = Notifications.f_GetIterator();
								auto Change0 = *iChange;
								++iChange;

								DMibExpect(Change0.m_Notification, ==, EFileChangeNotification_Removed);
								DMibExpect(Change0.m_Path, ==, "");
								if (!bRecursive)
								{
									auto Change1 = *iChange;
									++iChange;
									DMibExpect(Change1.m_Notification, ==, EFileChangeNotification_Removed);
									DMibExpect(Change1.m_Path, ==, "File3.tst");

									if (iChange)
									{
										auto Change2 = *iChange;
										++iChange;
										DMibExpect(Change2.m_Notification, ==, EFileChangeNotification_Modified);
										DMibExpect(Change2.m_Path, ==, "");
									}
								}
								DMibExpectFalse(fHasChange() && "After");
							}
	#endif
							FileChangeNotification.f_Close();
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

						auto fCheckAttribs = [&](EFileAttrib _SetAttrib, EFileAttrib _ExpectAttribMask, EFileAttrib _ExpectAttrib, EFileAttrib _ClearAttrib)
							{
								CFile File;
								File.f_Open(TestFileName + ".attrib", EFileOpen_Write, _SetAttrib);
								File.f_Write("Testing", 7);
								DMibTest((DMibExpr(File.f_GetAttributes()) & DMibExpr(_ExpectAttribMask)) == DMibExpr(_ExpectAttrib));
								File.f_SetAttributes(_ClearAttrib);
								File.f_Close();
							}
						;

						auto fCheckTestFileOpenAttribs = [&](EFileAttrib _SetAttrib, EFileAttrib _ExpectAttribMask, EFileAttrib _ExpectAttrib, EFileAttrib _ClearAttrib)
							{
								{
									DMibTestPath("New file");
									fCheckAttribs(_SetAttrib, _ExpectAttribMask, _ExpectAttrib, _ClearAttrib);
								}
								{
									DMibTestPath("Existing file");
									fCheckAttribs(_SetAttrib, _ExpectAttribMask, _ExpectAttrib, _ClearAttrib);
								}
								CFile::fs_DeleteFile(TestFileName + ".attrib");
							}
						;

						{
							DMibTestPath("File open attrib");
							fCheckTestFileOpenAttribs(EFileAttrib_ReadOnly, EFileAttrib_ReadOnly | EFileAttrib_Executable, EFileAttrib_ReadOnly, EFileAttrib_None);
						}

						{
							DMibTestPath("File open unix attrib");
							fCheckTestFileOpenAttribs
								(
									EFileAttrib_UnixAttributesValid | EFileAttrib_UserRead | EFileAttrib_UserWrite | EFileAttrib_GroupRead
									, EFileAttrib_AllUnixPermissions
									, EFileAttrib_UserRead | EFileAttrib_UserWrite | EFileAttrib_GroupRead
									, EFileAttrib_UnixAttributesValid | EFileAttrib_UserRead | EFileAttrib_UserWrite
								)
							;
						}

						{
							DMibTestPath("Read back");
							CFile File;
							File.f_Open(TestFileName, EFileOpen_Read);
							ch8 Temp[8];
							NMib::NMemory::fg_MemClear(Temp);
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
	#ifdef DPlatformFamily_macOS
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
							DMibTest(DMibExpr(Progress.m_bCalled) == DMibExpr(false));

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
					}
				}
			};

			CStr ProtectedFolder;
			CStr UnprotectedFolder;
			CStr ProtectedButExecutableExe;

			#ifdef DPlatformFamily_Windows
				ProtectedFolder = "C:\\System Volume Information";
				UnprotectedFolder = CFile::fs_GetProgramDirectory();
				ProtectedButExecutableExe = "C:\\Windows\\notepad.exe";
			#else
				//#warning File Permissions tests not implemented for this platform!
			#endif

			DMibTestSuite(CTestCategory("Other volume") << CTestGroup("Manual"))
			{
				CStr RootPath = CFile::fs_GetProgramDirectory() / "FileTestOtherVolume";

				CStr TestFileDir = RootPath + "/FileTest";
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
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedFolder, EFileRight_Read)) == DMibExpr(ECheckFileRights_NoAccess));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedFolder, EFileRight_Write)) == DMibExpr(ECheckFileRights_NoAccess));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedFolder, EFileRight_Write | EFileRight_Read)) == DMibExpr(ECheckFileRights_NoAccess));
				}

				if (!UnprotectedFolder.f_IsEmpty())
				{
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(UnprotectedFolder, EFileRight_Read)) == DMibExpr(ECheckFileRights_Access));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(UnprotectedFolder, EFileRight_Write)) == DMibExpr(ECheckFileRights_Access));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(UnprotectedFolder, EFileRight_Write | EFileRight_Read)) == DMibExpr(ECheckFileRights_Access));
				}

				if (!ProtectedButExecutableExe.f_IsEmpty())
				{
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedButExecutableExe, EFileRight_Read)) == DMibExpr(ECheckFileRights_Access));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedButExecutableExe, EFileRight_Write)) == DMibExpr(ECheckFileRights_NoAccess));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedButExecutableExe, EFileRight_Write | EFileRight_Read)) == DMibExpr(ECheckFileRights_NoAccess));
					DMibTest(DMibExpr(CFile::fs_CheckFileRights(ProtectedButExecutableExe, EFileRight_Execute)) == DMibExpr(ECheckFileRights_Access));
				}

			};

			DMibTestCategory("Ownership")
			{
				DMibTestSuite("Get Ownership")
				{
#ifdef DPlatformFamily_macOS
					DMibTest(DMibExpr(CFile::fs_GetOwner("/dev")) == DMibExpr("root"));
					DMibTest(DMibExpr(CFile::fs_GetGroup("/dev")) == DMibExpr("wheel"));
					DMibTest(DMibExpr(TCThrowsException<NMib::NException::CException>()) == DMibLExpr(CFile::fs_GetOwner("/_HansoftPathThatDoesNotExist")));
					DMibTest(DMibExpr(TCThrowsException<NMib::NException::CException>()) == DMibLExpr(CFile::fs_GetGroup("/_HansoftPathThatDoesNotExist")));
#endif
				};

			DMibTestSuite(CTestCategory("Set Ownership") << CTestGroup("Manual"))
			{
				CStr RootPath = CFile::fs_GetProgramDirectory() / "FileTestSetOwnership";

				CStr TestFileName = RootPath + "/MalterlibSetOwnershipTestFile";
				CStr TestDirectory = RootPath + "/TestDirectory";
				CStr TestDirectoryFile = TestDirectory + "/MalterlibSetOwnershipTestFile";
				CStr TestDirectoryLink = TestDirectory + "/MalterlibSetOwnershipTestLink";
				CStr TestDirectoryDirectoryLink = TestDirectory + "/MalterlibSetOwnershipTestDirectoryLink";

				if (CFile::fs_FileExists(TestDirectoryDirectoryLink))
					CFile::fs_DeleteFile(TestDirectoryDirectoryLink);
				if (CFile::fs_FileExists(TestDirectoryLink))
					CFile::fs_DeleteFile(TestDirectoryLink);
				if (CFile::fs_FileExists(TestDirectoryFile))
					CFile::fs_DeleteFile(TestDirectoryFile);
				if (CFile::fs_FileExists(TestDirectory))
					CFile::fs_DeleteDirectory(TestDirectory);


#ifdef DPlatformFamily_macOS
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

					DMibTest((DMibExpr(CFile::fs_GetAttributes(TestDirectoryLink)) & DMibExpr(EFileAttrib_File)) == DMibExpr(EFileAttrib_File));
					DMibTest((DMibExpr(CFile::fs_GetAttributes(TestDirectoryLink)) & DMibExpr(EFileAttrib_Directory)) == DMibExpr(EFileAttrib_None));
					DMibTest((DMibExpr(CFile::fs_GetAttributes(TestDirectoryLink)) & DMibExpr(EFileAttrib_Link)) == DMibExpr(EFileAttrib_Link));
				}
				{
					CFile::fs_CreateSymbolicLink("..", TestDirectoryDirectoryLink, EFileAttrib_Directory, ESymbolicLinkFlag_Relative);

					DMibTest((DMibExpr(CFile::fs_GetAttributes(TestDirectoryDirectoryLink)) & DMibExpr(EFileAttrib_File)) == DMibExpr(EFileAttrib_None));
					DMibTest((DMibExpr(CFile::fs_GetAttributes(TestDirectoryDirectoryLink)) & DMibExpr(EFileAttrib_Directory)) == DMibExpr(EFileAttrib_Directory));
					DMibTest((DMibExpr(CFile::fs_GetAttributes(TestDirectoryDirectoryLink)) & DMibExpr(EFileAttrib_Link)) == DMibExpr(EFileAttrib_Link));
				}

				{
					DMibTestPath("Recursive on Directory");
					DMibTest(DMibExpr(TCThrowsException<>()) == DMibLExpr(CFile::fs_SetOwnerRecursive(TestDirectoryLink, "nobody", true)));
					DMibTest(DMibExpr(CFile::fs_GetOwner(TestDirectoryFile)) == DMibExpr("nobody"));
					DMibTest(DMibExpr(TCThrowsException<>()) == DMibLExpr(CFile::fs_SetGroupRecursive(TestDirectoryLink, "nobody", true)));
					DMibTest(DMibExpr(CFile::fs_GetGroup(TestDirectoryFile)) == DMibExpr("nobody"));
				}

				{
					CFile::fs_DeleteFile(TestDirectoryDirectoryLink);
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
				CStr RootPath = CFile::fs_GetProgramDirectory() / "FileTestFind";

				fg_TestAddCleanupPath(RootPath);

				CStr CurrentDir = RootPath + "/FileTestFind";
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

					FoundFiles.f_Sort();
					ExpectedFiles.f_Sort();

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

					FoundFiles.f_Sort();
					ExpectedFiles.f_Sort();

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
				{
					DMibTestPath("Case2");
					CFile::CFindFilesOptions Options(CurrentDir + "/TestFile", true);

					Options.m_ExcludePatterns = fg_CreateVector<CStr>("*/Dir2");

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1/TestFile");

					FoundFiles.f_Sort();
					ExpectedFiles.f_Sort();

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

					FoundFiles.f_Sort();
					ExpectedFiles.f_Sort();

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
				{
					DMibTestPath("Case4");
					CFile::CFindFilesOptions Options(CurrentDir + "/Dir1", true);

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/Dir1");

					FoundFiles.f_Sort();
					ExpectedFiles.f_Sort();

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

					FoundFiles.f_Sort();
					ExpectedFiles.f_Sort();

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

					FoundFiles.f_Sort();
					ExpectedFiles.f_Sort();

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
				{
					DMibTestPath("Case7");
					CFile::CFindFilesOptions Options(CurrentDir + "/*", true);

					Options.m_ExcludePatterns = fg_CreateVector<CStr>("*/Dir2/Dir1/*");
					Options.m_AttribMask |= EFileAttrib_FindDirectoryLast;

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> FoundFiles0;
					TCVector<CStr> ExpectedFiles0;
					ExpectedFiles0.f_Insert(CurrentDir + "/Dir1/TestFile");
					ExpectedFiles0.f_Insert(CurrentDir + "/Dir1");

					TCVector<CStr> FoundFiles1;
					TCVector<CStr> ExpectedFiles1;
					ExpectedFiles1.f_Insert(CurrentDir + "/Dir2/Dir1");
					ExpectedFiles1.f_Insert(CurrentDir + "/Dir2/TestFile");
					ExpectedFiles1.f_Insert(CurrentDir + "/Dir2");

					TCVector<CStr> FoundFiles2;
					TCVector<CStr> ExpectedFiles2;
					ExpectedFiles2.f_Insert(CurrentDir + "/Dir3");

					for (auto &FoundFile : FoundFiles)
					{
						if (FoundFile.f_StartsWith(CurrentDir + "/Dir1"))
							FoundFiles0.f_Insert(FoundFile);
						else if (FoundFile.f_StartsWith(CurrentDir + "/Dir2"))
							FoundFiles1.f_Insert(FoundFile);
						else if (FoundFile.f_StartsWith(CurrentDir + "/Dir3"))
							FoundFiles2.f_Insert(FoundFile);
					}

					FoundFiles0.f_Sort();
					ExpectedFiles0.f_Sort();
					FoundFiles1.f_Sort();
					ExpectedFiles1.f_Sort();
					FoundFiles2.f_Sort();
					ExpectedFiles2.f_Sort();

					DMibExpect(FoundFiles0, ==, ExpectedFiles0);
					DMibExpect(FoundFiles1, ==, ExpectedFiles1);
					DMibExpect(FoundFiles2, ==, ExpectedFiles2);
				}
				{
					DMibTestPath("Case8");
					CFile::CFindFilesOptions Options(CurrentDir + "/*", true);

					Options.m_ExcludePatterns = fg_CreateVector<CStr>("*/Dir2/Dir1/*");

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));

					TCVector<CStr> FoundFiles0;
					TCVector<CStr> ExpectedFiles0;
					ExpectedFiles0.f_Insert(CurrentDir + "/Dir1");
					ExpectedFiles0.f_Insert(CurrentDir + "/Dir1/TestFile");

					TCVector<CStr> FoundFiles1;
					TCVector<CStr> ExpectedFiles1;
					ExpectedFiles1.f_Insert(CurrentDir + "/Dir2");
					ExpectedFiles1.f_Insert(CurrentDir + "/Dir2/Dir1");
					ExpectedFiles1.f_Insert(CurrentDir + "/Dir2/TestFile");

					TCVector<CStr> FoundFiles2;
					TCVector<CStr> ExpectedFiles2;
					ExpectedFiles2.f_Insert(CurrentDir + "/Dir3");

					for (auto &FoundFile : FoundFiles)
					{
						if (FoundFile.f_StartsWith(CurrentDir + "/Dir1"))
							FoundFiles0.f_Insert(FoundFile);
						else if (FoundFile.f_StartsWith(CurrentDir + "/Dir2"))
							FoundFiles1.f_Insert(FoundFile);
						else if (FoundFile.f_StartsWith(CurrentDir + "/Dir3"))
							FoundFiles2.f_Insert(FoundFile);
					}

					FoundFiles0.f_Sort();
					ExpectedFiles0.f_Sort();
					FoundFiles1.f_Sort();
					ExpectedFiles1.f_Sort();
					FoundFiles2.f_Sort();
					ExpectedFiles2.f_Sort();

					DMibExpect(FoundFiles0, ==, ExpectedFiles0);
					DMibExpect(FoundFiles1, ==, ExpectedFiles1);
					DMibExpect(FoundFiles2, ==, ExpectedFiles2);
				}
				{
					DMibTestPath("Symbolic Links");

					{
						auto Flags = CFile::fs_CanCreateSymbolicLink(EFileAttrib_Directory, ESymbolicLinkFlag_None) ? ESymbolicLinkFlag_None : ESymbolicLinkFlag_AllowEmulation;
						CStr DirSymLinkDestination = CurrentDir + "/TestDirSym";
						CFile::fs_CreateSymbolicLink(".", DirSymLinkDestination, EFileAttrib_Directory, ESymbolicLinkFlag_Relative | Flags);
					}

					CFile::CFindFilesOptions Options(CurrentDir + "/*", true);

					Options.m_ExcludePatterns = fg_CreateVector<CStr>("*/Dir2/Dir1/*");
					Options.m_bFollowLinks = false;
					Options.m_AttribMask = EFileAttrib_File | EFileAttrib_Directory;

					auto FoundFiles = fGetFiles(CFile::fs_FindFiles(Options));
					FoundFiles.f_Sort();

					TCVector<CStr> ExpectedFiles;
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir1/TestFile");

					ExpectedFiles.f_Insert(CurrentDir + "/Dir2");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/Dir1");
					ExpectedFiles.f_Insert(CurrentDir + "/Dir2/TestFile");

					ExpectedFiles.f_Insert(CurrentDir + "/Dir3");
					ExpectedFiles.f_Insert(CurrentDir + "/TestDirSym");
					ExpectedFiles.f_Sort();

					DMibExpect(FoundFiles, ==, ExpectedFiles);
				}
			};

			DMibTestSuite("Symlink")
			{
				CStr RootPath = CFile::fs_GetProgramDirectory() / "FileTestSymlink";

				fg_TestAddCleanupPath(RootPath);

				CStr CurrentDir = RootPath + "/FileTest2";
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
			DMibTestSuite("Time")
			{
				CStr RootPath = CFile::fs_GetProgramDirectory() / "FileTestTime";

				fg_TestAddCleanupPath(RootPath);

				CStr CurrentDir = RootPath + "/FileTest3";
				if (CFile::fs_FileExists(CurrentDir))
					CFile::fs_DeleteDirectoryRecursive(CurrentDir, true);
				CFile::fs_CreateDirectory(CurrentDir);

				NTime::CTime CreationTime = NTime::CTimeConvert::fs_CreateTime(2005, 01, 01, 22, 16, 05, 0.75757575757575757575);
				NTime::CTime ModificationTime = NTime::CTimeConvert::fs_CreateTime(2005, 05, 01, 22, 16, 05, 0.75757575757575757575);
				NTime::CTime AccessTime = NTime::CTimeConvert::fs_CreateTime(2005, 06, 01, 22, 16, 05, 0.75757575757575757575);

				NTime::CTimeSpan Precision = NTime::CTimeSpanConvert::fs_CreateSpanFromSeconds(0.000000001);
#if defined(DPlatformFamily_Windows)
				Precision = NTime::CTimeSpanConvert::fs_CreateSpanFromSeconds(0.0000001);
#elif defined(DPlatformFamily_macOS)
				if (CSystem::ms_PlatformVersion < 10'13'00)
					Precision = NTime::CTimeSpanConvert::fs_CreateSpanFromSeconds(2.0);
#endif

				auto fDiffFileTime = [&](NTime::CTime const &_Left, NTime::CTime const &_Right) -> fp64
					{
						if (_Left > _Right)
						{
							auto Diff = (_Left - _Right);
							if (Diff > Precision)
								return Diff.f_GetSecondsFraction();
						}
						else
						{
							auto Diff = (_Right - _Left);
							if (Diff > Precision)
								return Diff.f_GetSecondsFraction();
						}

						return 0.0;
					}
				;

				{
					DMibTestPath("ByFile");
					CStr TestFileName = CFile::fs_AppendPath(CurrentDir, "ByFile.file");
					{
						CFile File;
						File.f_Open(TestFileName, EFileOpen_Write);
						File.f_SetCreationTime(CreationTime);
						File.f_SetWriteTime(ModificationTime);
						File.f_SetAccessTime(AccessTime);
					}
					{
						CFile File;
						File.f_Open(TestFileName, EFileOpen_Read);
#ifndef DPlatformFamily_Linux
						DMibExpectFalse(fDiffFileTime(File.f_GetCreationTime(), CreationTime));
#endif
						DMibExpectFalse(fDiffFileTime(File.f_GetWriteTime(), ModificationTime));
						DMibExpectFalse(fDiffFileTime(File.f_GetAccessTime(), AccessTime));
					}
				}
				{
					DMibTestPath("ByPath");
					CStr TestFileName = CFile::fs_AppendPath(CurrentDir, "ByPath.file");
					{
						CFile File;
						File.f_Open(TestFileName, EFileOpen_Write);
					}
					CFile::fs_SetCreationTime(TestFileName, CreationTime);
					CFile::fs_SetWriteTime(TestFileName, ModificationTime);
					CFile::fs_SetAccessTime(TestFileName, AccessTime);
#ifndef DPlatformFamily_Linux
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetCreationTime(TestFileName), CreationTime));
#endif
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetWriteTime(TestFileName), ModificationTime));
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetAccessTime(TestFileName), AccessTime));

					// Link on non-link file
#ifndef DPlatformFamily_Linux
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetCreationTimeOnLink(TestFileName), CreationTime));
#endif
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetWriteTimeOnLink(TestFileName), ModificationTime));
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetAccessTimeOnLink(TestFileName), AccessTime));
				}
				{
					DMibTestPath("OnLinks");
					CStr TestFileNameLinkedTo = CFile::fs_AppendPath(CurrentDir, "LinkedTo.file");
					{
						CFile File;
						File.f_Open(TestFileNameLinkedTo, EFileOpen_Write);
					}
					CStr TestFileName = CFile::fs_AppendPath(CurrentDir, "OnLink.file");
					CFile::fs_CreateSymbolicLink("LinkedTo.file", TestFileName, EFileAttrib_File, ESymbolicLinkFlag_Relative);

					CFile::fs_SetCreationTime(TestFileName, CreationTime);
					CFile::fs_SetWriteTime(TestFileName, ModificationTime);
					CFile::fs_SetAccessTime(TestFileName, AccessTime);
#ifndef DPlatformFamily_Linux
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetCreationTime(TestFileName), CreationTime));
#endif
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetWriteTime(TestFileName), ModificationTime));
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetAccessTime(TestFileName), AccessTime));
				}
				{
					DMibTestPath("Links");
					CStr TestFileNameLinkedTo = CFile::fs_AppendPath(CurrentDir, "LinkedTo2.file");
					{
						CFile File;
						File.f_Open(TestFileNameLinkedTo, EFileOpen_Write);
					}
					CStr TestFileName = CFile::fs_AppendPath(CurrentDir, "Link.file");
					CFile::fs_CreateSymbolicLink("LinkedTo2.file", TestFileName, EFileAttrib_File, ESymbolicLinkFlag_Relative);
					NTime::CTime LinkedToFileTime = NTime::CTimeConvert::fs_CreateTime(2008, 06, 01, 22, 16, 05, 0.75757575757575757575);;

					CFile::fs_SetCreationTimeOnLink(TestFileName, CreationTime);
					CFile::fs_SetCreationTime(TestFileName, LinkedToFileTime);

					CFile::fs_SetWriteTimeOnLink(TestFileName, ModificationTime);
					CFile::fs_SetWriteTime(TestFileName, LinkedToFileTime);

					CFile::fs_SetAccessTime(TestFileName, LinkedToFileTime);
					CFile::fs_SetAccessTimeOnLink(TestFileName, AccessTime);

#ifndef DPlatformFamily_Linux
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetCreationTimeOnLink(TestFileName), CreationTime));
#endif
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetWriteTimeOnLink(TestFileName), ModificationTime));
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetAccessTimeOnLink(TestFileName), AccessTime));

#ifndef DPlatformFamily_Linux
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetCreationTime(TestFileName), LinkedToFileTime));
#endif
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetWriteTime(TestFileName), LinkedToFileTime));
					DMibExpectFalse(fDiffFileTime(CFile::fs_GetAccessTime(TestFileName), LinkedToFileTime));
				}

				CFile::fs_DeleteDirectoryRecursive(CurrentDir);
			};
			DMibTestSuite("TimePrecision")
			{
				// Tests for the precision of file time conversion
				// On POSIX: tests integer arithmetic conversion between CTime and timespec
				// On Windows: tests conversion between CTime and FILETIME

				CStr RootPath = CFile::fs_GetProgramDirectory() / "TimespecConversionTest";

				fg_TestAddCleanupPath(RootPath);

				if (CFile::fs_FileExists(RootPath))
					CFile::fs_DeleteDirectoryRecursive(RootPath, true);
				CFile::fs_CreateDirectory(RootPath);

				using namespace NMib::NTime;

				// Platform-specific precision for file timestamps
				// POSIX timespec: nanosecond precision (1e-9)
				// Windows FILETIME: 100-nanosecond precision (1e-7)
				NTime::CTimeSpan TimePrecision = NTime::CTimeSpanConvert::fs_CreateSpanFromSeconds(1_ns);
				bool bHasHighPrecision = true;
				bool bHasNanosecondPrecision = true;

#if defined(DPlatformFamily_Windows)
				// Windows FILETIME has 100-nanosecond precision (not nanosecond)
				TimePrecision = NTime::CTimeSpanConvert::fs_CreateSpanFromSeconds(100_ns);
				bHasNanosecondPrecision = false;
#elif defined(DPlatformFamily_macOS)
				// On older macOS, file system might have lower precision
				if (CSystem::ms_PlatformVersion < 10'13'00)
				{
					TimePrecision = NTime::CTimeSpanConvert::fs_CreateSpanFromSeconds(2_seconds);
					bHasHighPrecision = false;
					bHasNanosecondPrecision = false;
				}
#endif

				auto fDiffTime = [&](NTime::CTime const &_Left, NTime::CTime const &_Right) -> fp64
					{
						if (_Left > _Right)
						{
							auto Diff = (_Left - _Right);
							if (Diff > TimePrecision)
								return Diff.f_GetSecondsFraction();
						}
						else
						{
							auto Diff = (_Right - _Left);
							if (Diff > TimePrecision)
								return Diff.f_GetSecondsFraction();
						}

						return 0.0;
					}
				;

				auto fTestTimeRoundTrip = [&](CStr const &_Path, NTime::CTime const &_Time)
					{
						DMibTestPath("{}"_f << _Path);
						CStr TestFileName = CFile::fs_AppendPath(RootPath, _Path + ".file");
						{
							CFile File;
							File.f_Open(TestFileName, EFileOpen_Write);
							File.f_SetWriteTime(_Time);
						}
						{
							CFile File;
							File.f_Open(TestFileName, EFileOpen_Read);
							DMibExpectFalse(fDiffTime(File.f_GetWriteTime(), _Time));
						}
					}
				;

				{
					DMibTestPath("FractionPrecision");
					// Test various fractions to ensure integer arithmetic preserves precision
					fTestTimeRoundTrip("Fraction_0", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.0));
					fTestTimeRoundTrip("Fraction_0_1", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.1));
					fTestTimeRoundTrip("Fraction_0_25", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.25));
					fTestTimeRoundTrip("Fraction_0_5", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.5));
					fTestTimeRoundTrip("Fraction_0_75", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.75));
					fTestTimeRoundTrip("Fraction_0_9", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.9));
					fTestTimeRoundTrip("Fraction_0_99", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.99));
					fTestTimeRoundTrip("Fraction_0_999", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.999));
					fTestTimeRoundTrip("Fraction_0_9999", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.9999));
					fTestTimeRoundTrip("Fraction_0_99999", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.99999));
					fTestTimeRoundTrip("Fraction_0_999999", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.999999));
					fTestTimeRoundTrip("Fraction_0_9999999", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.9999999));
					fTestTimeRoundTrip("Fraction_0_99999999", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.99999999));
					fTestTimeRoundTrip("Fraction_0_999999999", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.999999999));
				}

				if (bHasHighPrecision)
				{
					DMibTestPath("FractionIntPrecision");
					// Test using raw fraction integers for precise control
					constexpr uint64 MaxFrac = NTime::NPrivate::CConst::mc_FractionDividend - 1;

					// Test values near maximum fraction - these should round up to 46 seconds
					// when converted to nanoseconds (since MaxFrac is very close to 1.0)
					auto fTestMaxFracRoundsUp = [&](CStr const &_Path, uint64 _FracInt)
						{
							DMibTestPath("{}"_f << _Path);
							CStr TestFileName = CFile::fs_AppendPath(RootPath, _Path + ".file");
							NTime::CTime OriginalTime = NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, _FracInt);
							NTime::CTime ExpectedRoundedTime = NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 46, 0);
							{
								CFile File;
								File.f_Open(TestFileName, EFileOpen_Write);
								File.f_SetWriteTime(OriginalTime);
							}
							{
								CFile File;
								File.f_Open(TestFileName, EFileOpen_Read);
								NTime::CTime ReadTime = File.f_GetWriteTime();
								// Should round up to exactly 46.0 seconds - use exact comparison
								DMibExpect(ReadTime, ==, ExpectedRoundedTime);
							}
						}
					;

					fTestMaxFracRoundsUp("FractionInt_Max", MaxFrac);
					fTestMaxFracRoundsUp("FractionInt_Max_Minus_1", MaxFrac - 1);
					fTestMaxFracRoundsUp("FractionInt_Max_Minus_10", MaxFrac - 10);
					fTestMaxFracRoundsUp("FractionInt_Max_Minus_100", MaxFrac - 100);
					fTestMaxFracRoundsUp("FractionInt_Max_Minus_1000", MaxFrac - 1000);

					// Test values near zero
					fTestTimeRoundTrip("FractionInt_0", NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, 0));
					fTestTimeRoundTrip("FractionInt_1", NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, 1));
					fTestTimeRoundTrip("FractionInt_10", NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, 10));
					fTestTimeRoundTrip("FractionInt_100", NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, 100));
					fTestTimeRoundTrip("FractionInt_1000", NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, 1000));

					// Test half value
					fTestTimeRoundTrip("FractionInt_Half", NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, NTime::NPrivate::CConst::mc_FractionDividend / 2));
				}

				if (bHasHighPrecision)
				{
					DMibTestPath("NanosecondBoundaries");
					// Test nanosecond boundary values
					// These values are chosen to test the integer arithmetic rounding

					// 1 nanosecond = mc_FractionDividend / 1e9 internal units
					// These constants match fsg_TimespecToCTime in Malterlib_Core_PlatformImp_POSIX_File.hpp
					constexpr uint64 Billion = 1'000'000'000;
					constexpr uint64 Divisor = NTime::NPrivate::CConst::mc_FractionDividend / Billion;
					constexpr uint64 Remainder = NTime::NPrivate::CConst::mc_FractionDividend % Billion;

					// Convert nanoseconds to internal fraction using the same formula as fsg_TimespecToCTime
					auto fNsToFrac = [](uint64 _Nanoseconds) -> uint64
						{
							return _Nanoseconds * Divisor + _Nanoseconds * Remainder / Billion;
						}
					;

					// Test exact nanosecond multiples - these should round-trip exactly
					auto fTestExactNs = [&](CStr const &_Path, uint64 _Nanoseconds)
						{
							DMibTestPath("{}"_f << _Path);
							CStr TestFileName = CFile::fs_AppendPath(RootPath, _Path + ".file");
							NTime::CTime OriginalTime = NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, fNsToFrac(_Nanoseconds));
							{
								CFile File;
								File.f_Open(TestFileName, EFileOpen_Write);
								File.f_SetWriteTime(OriginalTime);
							}
							{
								CFile File;
								File.f_Open(TestFileName, EFileOpen_Read);
								NTime::CTime ReadTime = File.f_GetWriteTime();
								DMibExpect(fg_Abs(ReadTime - OriginalTime), ==, NTime::CTimeSpan::fs_Zero());
							}
						}
					;

					// Tests that require true nanosecond precision (POSIX only)
					// Windows FILETIME has 100-nanosecond precision, so values not divisible by 100 will be rounded
					if (bHasNanosecondPrecision)
					{
						fTestExactNs("Ns_1", 1);
						fTestExactNs("Ns_10", 10);
						fTestExactNs("Ns_999999999", 999'999'999);
					}

					// Tests that work on both POSIX (nanosecond) and Windows (100-nanosecond) precision
					// These values are exact multiples of 100ns
					fTestExactNs("Ns_100", 100);
					fTestExactNs("Ns_1000", 1000);
					fTestExactNs("Ns_1000000", 1'000'000);
					fTestExactNs("Ns_500000000", 500'000'000);

					// Test rounding behavior at nanosecond boundaries (POSIX only)
					// The rounding formula is: (FractionInt + Divisor/2) / Divisor
					// Values >= Divisor/2 round up, values < Divisor/2 round down
					// Note: Divisor is odd, so Divisor/2 truncates to slightly below 0.5ns
					auto fTestRoundsTo = [&](CStr const &_Path, uint64 _FracInt, uint64 _ExpectedNs)
						{
							DMibTestPath("{}"_f << _Path);
							CStr TestFileName = CFile::fs_AppendPath(RootPath, _Path + ".file");
							NTime::CTime OriginalTime = NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, _FracInt);
							NTime::CTime ExpectedTime = NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, fNsToFrac(_ExpectedNs));
							{
								CFile File;
								File.f_Open(TestFileName, EFileOpen_Write);
								File.f_SetWriteTime(OriginalTime);
							}
							{
								CFile File;
								File.f_Open(TestFileName, EFileOpen_Read);
								NTime::CTime ReadTime = File.f_GetWriteTime();
								DMibExpect(fg_Abs(ReadTime - ExpectedTime), ==, NTime::CTimeSpan::fs_Zero());
							}
						}
					;

					// Test rounding down: Divisor/2 rounds to 0 (since Divisor is odd, 2*(Divisor/2) < Divisor)
					fTestRoundsTo("At_Half_RoundsTo_0", Divisor / 2, 0);

					// These tests require nanosecond precision (POSIX only)
					if (bHasNanosecondPrecision)
					{
						// Test rounding up: Divisor/2 + 1 rounds to 1
						fTestRoundsTo("Above_Half_RoundsTo_1", Divisor / 2 + 1, 1);
						// Test rounding from 1.5ns: fNsToFrac(1) + Divisor/2 rounds to 1 (not 2)
						fTestRoundsTo("At_1_5_RoundsTo_1", fNsToFrac(1) + Divisor / 2, 1);
						// Test rounding from just above 1.5ns
						fTestRoundsTo("Above_1_5_RoundsTo_2", fNsToFrac(1) + Divisor / 2 + 1, 2);
					}
					else
					{
						// Test rounding behavior at 100-nanosecond boundaries (Windows)
						// Windows FILETIME has 100ns precision, so we test at those boundaries
						constexpr uint64 TenMillion = 10'000'000;
						constexpr uint64 Divisor100ns = NTime::NPrivate::CConst::mc_FractionDividend / TenMillion;
						constexpr uint64 Remainder100ns = NTime::NPrivate::CConst::mc_FractionDividend % TenMillion;

						// Convert 100-nanosecond units to internal fraction
						auto f100nsToFrac = [](uint64 _Units100ns) -> uint64
							{
								constexpr uint64 TenMillion = 10'000'000;
								constexpr uint64 Divisor100ns = NTime::NPrivate::CConst::mc_FractionDividend / TenMillion;
								constexpr uint64 Remainder100ns = NTime::NPrivate::CConst::mc_FractionDividend % TenMillion;
								return _Units100ns * Divisor100ns + _Units100ns * Remainder100ns / TenMillion;
							}
						;

						auto fTestRoundsTo100ns = [&](CStr const &_Path, uint64 _FracInt, uint64 _Expected100ns)
							{
								DMibTestPath("{}"_f << _Path);
								CStr TestFileName = CFile::fs_AppendPath(RootPath, _Path + ".file");
								NTime::CTime OriginalTime = NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, _FracInt);
								NTime::CTime ExpectedTime = NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, f100nsToFrac(_Expected100ns));
								{
									CFile File;
									File.f_Open(TestFileName, EFileOpen_Write);
									File.f_SetWriteTime(OriginalTime);
								}
								{
									CFile File;
									File.f_Open(TestFileName, EFileOpen_Read);
									NTime::CTime ReadTime = File.f_GetWriteTime();
									DMibExpect(fg_Abs(ReadTime - ExpectedTime), ==, NTime::CTimeSpan::fs_Zero());
								}
							}
						;

						// Test rounding up: Divisor100ns/2 + 1 rounds to 1 (100ns)
						fTestRoundsTo100ns("Above_Half_RoundsTo_100ns", Divisor100ns / 2 + 1, 1);
						// Test rounding from 1.5 * 100ns: f100nsToFrac(1) + Divisor100ns/2 rounds to 1 (not 2)
						fTestRoundsTo100ns("At_1_5_RoundsTo_100ns", f100nsToFrac(1) + Divisor100ns / 2, 1);
						// Test rounding from just above 1.5 * 100ns
						fTestRoundsTo100ns("Above_1_5_RoundsTo_200ns", f100nsToFrac(1) + Divisor100ns / 2 + 1, 2);
					}
				}

				if (bHasHighPrecision)
				{
					DMibTestPath("OverflowHandling");
					// Test values near the overflow boundary where nanoseconds might round to 1 billion
					// The fix handles this by incrementing seconds and setting nanoseconds to 0

					constexpr uint64 Billion = 1'000'000'000;
					constexpr uint64 Divisor = NTime::NPrivate::CConst::mc_FractionDividend / Billion;
					constexpr uint64 MaxNsValue = Divisor * (Billion - 1);

					// Values very close to 1 second
					fTestTimeRoundTrip("NearOverflow_1", NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, MaxNsValue));
					fTestTimeRoundTrip("NearOverflow_2", NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, MaxNsValue + Divisor / 4));
					fTestTimeRoundTrip("NearOverflow_3", NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, MaxNsValue + Divisor / 2));

					// Test that overflow case correctly increments seconds
					// When the fraction is very close to 1, nanoseconds should round up and
					// the second counter should be incremented
					constexpr uint64 MaxFrac = NTime::NPrivate::CConst::mc_FractionDividend - 1;
					NTime::CTime TimeNearOverflow = NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 45, MaxFrac);
					NTime::CTime TimeNextSecond = NTime::CTimeConvert::fs_CreateTimeIntFrac(2024, 6, 15, 12, 30, 46, 0);

					CStr TestFileNearOverflow = CFile::fs_AppendPath(RootPath, "NearOverflow.file");
					CStr TestFileNextSecond = CFile::fs_AppendPath(RootPath, "NextSecond.file");

					{
						CFile File;
						File.f_Open(TestFileNearOverflow, EFileOpen_Write);
						File.f_SetWriteTime(TimeNearOverflow);
					}
					{
						CFile File;
						File.f_Open(TestFileNextSecond, EFileOpen_Write);
						File.f_SetWriteTime(TimeNextSecond);
					}

					NTime::CTime ReadNearOverflow;
					NTime::CTime ReadNextSecond;
					{
						CFile File;
						File.f_Open(TestFileNearOverflow, EFileOpen_Read);
						ReadNearOverflow = File.f_GetWriteTime();
					}
					{
						CFile File;
						File.f_Open(TestFileNextSecond, EFileOpen_Read);
						ReadNextSecond = File.f_GetWriteTime();
					}

					// Both should be exactly the same - the near-overflow time should have
					// rounded up to exactly 46.0 seconds
					DMibExpect(ReadNearOverflow, ==, ReadNextSecond);
					DMibExpect(ReadNearOverflow, ==, TimeNextSecond);
				}

				{
					DMibTestPath("YearRange");
					// Test that precision is preserved across different years
					fTestTimeRoundTrip("Year_1970", NTime::CTimeConvert::fs_CreateTime(1970, 1, 1, 0, 0, 0, 0.123456789));
					fTestTimeRoundTrip("Year_2000", NTime::CTimeConvert::fs_CreateTime(2000, 6, 15, 12, 30, 45, 0.987654321));
					fTestTimeRoundTrip("Year_2024", NTime::CTimeConvert::fs_CreateTime(2024, 12, 31, 23, 59, 59, 0.999999999));
					fTestTimeRoundTrip("Year_2038", NTime::CTimeConvert::fs_CreateTime(2038, 1, 19, 3, 14, 7, 0.5));
					// Note: Year 3000 not tested - many file systems can't represent dates that far in the future
				}

				{
					DMibTestPath("SpecificValues");
					// Test specific problematic values that might have caused precision issues before the fix
					// These are values that would truncate badly with floating-point arithmetic

					// Value that has a repeating decimal in floating-point
					fTestTimeRoundTrip("Repeating_Third", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 1.0 / 3.0));
					fTestTimeRoundTrip("Repeating_Sixth", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 1.0 / 6.0));
					fTestTimeRoundTrip("Repeating_Seventh", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 1.0 / 7.0));

					// Powers of 2 fractions (exact in binary)
					fTestTimeRoundTrip("Binary_0_5", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.5));
					fTestTimeRoundTrip("Binary_0_25", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.25));
					fTestTimeRoundTrip("Binary_0_125", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.125));
					fTestTimeRoundTrip("Binary_0_0625", NTime::CTimeConvert::fs_CreateTime(2024, 6, 15, 12, 30, 45, 0.0625));

					// Value from the original file test that was used
					fTestTimeRoundTrip("OriginalTestValue", NTime::CTimeConvert::fs_CreateTime(2005, 05, 01, 22, 16, 05, 0.75757575757575757575));
				}

				CFile::fs_DeleteDirectoryRecursive(RootPath, true);
			};
#ifdef DPlatformFamily_Windows
			DMibTestSuite("Windows Deletion")
			{
				CStr RootPath = CFile::fs_GetProgramDirectory() / "FileTestWindowsDeletion";
				fg_TestAddCleanupPath(RootPath);
				CFile::fs_CreateDirectory(RootPath);

				CStr TestFile = RootPath / "Test.file";

				CFile::fs_WriteStringToFile(TestFile, "Empty", true);

				CFile File;
				File.f_Open(TestFile, EFileOpen_Read);

				DMibExpectException
					(
						CFile::fs_DeleteFile(TestFile)
						, DMibImpExceptionInstance
						(
							NFile::CExceptionFile
							, "Windows returned an error from CreateFileW(Delete)({0}): 32 The process cannot access the file because it is being used by another process.\n"
							"Windows returned an error from DeleteFile({0}): 32 The process cannot access the file because it is being used by another process."_f
							<< TestFile
						)
					)
				;
			};
#endif
		}
	};

	DMibTestRegister(CFile_Tests, Malterlib::File);
}
