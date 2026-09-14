// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Mib/File/PathGlob>
#include <Mib/File/File>
#include <Mib/Test/Test>
#include <Mib/Test/Exception>

namespace
{
	using namespace NMib;
	using namespace NMib::NFile;
	using namespace NMib::NStr;
	using namespace NMib::NTest;

	struct CPathGlob_Tests : CTest
	{
		static bool fs_Matches(CStr const &_Pattern, CStr const &_Path)
		{
			CPathGlob Glob(_Pattern);
			CPathGlob::CScratch Scratch;

			return Glob.f_Match(CPathGlob::fs_ToUnicode(_Path), CPathGlob::fs_ToUnicode(CFile::fs_GetFile(_Path)), Scratch);
		}

		static EPathGlobCover fs_Below(CStr const &_Pattern, CStr const &_Directory)
		{
			CPathGlob Glob(_Pattern);
			CPathGlob::CScratch Scratch;

			return Glob.f_MatchBelow(CPathGlob::fs_ToUnicode(_Directory), Scratch);
		}

		void f_DoTests()
		{
			DMibTestSuite("Match")
			{
				DMibExpectTrue(fs_Matches("*.cpp", "src/main.cpp"));
				DMibExpectFalse(fs_Matches("*.cpp", "src/main.h"));
				DMibExpectTrue(fs_Matches("*.{cpp,h}", "src/main.h"));
				DMibExpectTrue(fs_Matches("src/*.cpp", "src/main.cpp"));
				DMibExpectFalse(fs_Matches("src/*.cpp", "src/nested/main.cpp"));
				DMibExpectTrue(fs_Matches("src/**/*.cpp", "src/nested/deep/main.cpp"));
				DMibExpectTrue(fs_Matches("**/IC/**", "Qt/IC/W-x64/include/q.h"));
				DMibExpectFalse(fs_Matches("**/IC/**", "Qt/Source/q.h"));
				DMibExpectTrue(fs_Matches("/AGENTS.md", "AGENTS.md"));
				DMibExpectFalse(fs_Matches("/AGENTS.md", "docs/AGENTS.md"));
				DMibExpectTrue(fs_Matches("file?.[ch]", "file1.c"));
				DMibExpectFalse(fs_Matches("file?.[!ch]", "file1.c"));
				DMibExpectTrue(fs_Matches("a\\*b", "a*b"));
				DMibExpectExceptionType(CPathGlob("file{1..10}.cpp"), NException::CException);
			};

			DMibTestSuite("Below")
			{
				// A lone star meets every name; any other name pattern may meet some.
				DMibExpectTrue(fs_Below("*", "src") == EPathGlobCover::mc_All);
				DMibExpectTrue(fs_Below("*.cpp", "src") == EPathGlobCover::mc_Some);
				// A recursive star that can end the pattern covers everything under a directory
				// it has reached; a pattern that cannot reach the directory covers nothing.
				DMibExpectTrue(fs_Below("**/IC/**", "Qt/IC") == EPathGlobCover::mc_All);
				DMibExpectTrue(fs_Below("**/IC/**", "Qt/IC/W-x64") == EPathGlobCover::mc_All);
				DMibExpectTrue(fs_Below("**/IC/**", "Qt") == EPathGlobCover::mc_Some);
				DMibExpectTrue(fs_Below("**/IC/**.MHeader", "Qt/IC") == EPathGlobCover::mc_Some);
				DMibExpectTrue(fs_Below("src/**", "src") == EPathGlobCover::mc_All);
				DMibExpectTrue(fs_Below("src/**", "") == EPathGlobCover::mc_Some);
				DMibExpectTrue(fs_Below("src/**", "docs") == EPathGlobCover::mc_None);
				DMibExpectTrue(fs_Below("src/*.cpp", "src") == EPathGlobCover::mc_Some);
				DMibExpectTrue(fs_Below("/**", "") == EPathGlobCover::mc_All);
				DMibExpectTrue(fs_Below("/AGENTS.md", "docs") == EPathGlobCover::mc_None);
			};
		}
	};

	DMibTestRegister(CPathGlob_Tests, Malterlib::File);
}
