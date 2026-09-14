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
				// A '**' in a pattern without a separator crosses directories.
				DMibExpectTrue(fs_Matches("a**z.c", "x/a/nested/z.c"));
				DMibExpectFalse(fs_Matches("a**z.c", "x/b/nested/z.c"));
			};

			DMibTestSuite("GitRanges")
			{
				auto fGitMatches = [](CStr const &_Pattern, CStr const &_Name)
					{
						CPathGlob Glob(_Pattern, EPathGlobDialect::mc_Git);
						CPathGlob::CScratch Scratch;
						auto Name = CPathGlob::fs_ToText(_Name, EPathGlobDialect::mc_Git);

						return Glob.f_Match(Name, Name, Scratch);
					}
				;

				// A descending range matches its first character, and a range's end is one character, a '[' too.
				DMibExpectTrue(fGitMatches("file[9-0]", "file9"));
				DMibExpectTrue(fGitMatches("x[a-[:digit:]]", "xd]"));
				DMibExpectFalse(fGitMatches("x[a-[:digit:]]", "xd"));
				// A class starts no range, and a '-' behind a range is a member.
				DMibExpectTrue(fGitMatches("[[:alpha:]-[:digit:]]", "a"));
				DMibExpectTrue(fGitMatches("[[:alpha:]-[:digit:]]", "5"));
				DMibExpectTrue(fGitMatches("[[:alpha:]-[:digit:]]", "-"));
				DMibExpectTrue(fGitMatches("[a-b-[:digit:]]", "5"));
				DMibExpectFalse(fGitMatches("[[:alpha:]-[:digit:]]", "a]"));
				// A '[:' the first ']' behind it does not close as ':]' is members of the set.
				DMibExpectTrue(fGitMatches("[[:digit]x:]", "dx:]"));
				// A leading ']' starts a range as any first member does.
				DMibExpectTrue(fGitMatches("[]-[:digit:]]", "d]"));
				DMibExpectFalse(fGitMatches("[]-[:digit:]]", "]"));

				// Ignoring case folds the text and ranges; a single member and an escaped character compare as written.
				auto fGitMatchesNoCase = [](CStr const &_Pattern, CStr const &_Name)
					{
						CPathGlob Glob(_Pattern, EPathGlobDialect::mc_Git, true);
						CPathGlob::CScratch Scratch;
						auto Name = CPathGlob::fs_ToText(_Name, EPathGlobDialect::mc_Git);

						return Glob.f_Match(Name, Name, Scratch);
					}
				;

				DMibExpectTrue(fGitMatchesNoCase("[A-C]x", "bx"));
				DMibExpectFalse(fGitMatchesNoCase("[A]x", "ax"));
				DMibExpectTrue(fGitMatchesNoCase("[A-A]x", "ax"));
				DMibExpectFalse(fGitMatchesNoCase("\\Ax", "ax"));
				DMibExpectTrue(fGitMatchesNoCase("Ax", "ax"));

				// Only a separator written as one lets a recursive star match no directory.
				DMibExpectFalse(fGitMatches("a/**\\/b", "a/b"));
				DMibExpectTrue(fGitMatches("a/**\\/b", "a/x/b"));
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
				DMibExpectTrue(fs_Below("/****", "") == EPathGlobCover::mc_All);
				DMibExpectTrue(fs_Below("/****/", "") == EPathGlobCover::mc_Some);
				DMibExpectTrue(fs_Below("/AGENTS.md", "docs") == EPathGlobCover::mc_None);
			};
		}
	};

	DMibTestRegister(CPathGlob_Tests, Malterlib::File);
}
