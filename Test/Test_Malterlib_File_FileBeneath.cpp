// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Mib/Core/Core>
#include <Mib/Test/Exception>
#include <Mib/File/File>
#include <Mib/File/FileBeneath>
#include <Mib/Thread/Thread>

#ifndef DPlatformFamily_Windows
#	include <Mib/Core/PlatformSpecific/PosixErrNo>
#	include <Mib/Core/PlatformSpecific/PosixUser>
#	include <sys/stat.h>
#	include <unistd.h>
#endif

using namespace NMib;
using namespace NMib::NStr;
using namespace NMib::NContainer;
using namespace NMib::NFile;

namespace
{
	struct CFileBeneath_Tests : public NMib::NTest::CTest
	{
		void f_DoTests();
	};

	void CFileBeneath_Tests::f_DoTests()
	{
#ifndef DPlatformFamily_Windows
		DMibTestSuite("FileBeneath")
		{
			static umint s_iTest = 0;
			CStr Base = CFile::fs_GetTemporaryDirectory() / CStr("MalterlibFileBeneathTest-{}-{}"_f << ::getpid() << s_iTest++);
			CStr Root = Base / "Root";
			CStr Outside = Base / "Outside";
			CFile::fs_CreateDirectory(Root);
			CFile::fs_CreateDirectory(Outside);
			CFile::fs_WriteStringToFile(Outside / "Secret.txt", "secret", false);

			auto Cleanup = g_OnScopeExit / [&]
				{
					CFile::fs_DeleteDirectoryRecursive(Base);
				}
			;

			// What opening a link as a directory without following it fails with
			int const NotFollowedError = ENOTDIR;

			DMibTestCategory("ReadWrite")
			{
				CFileBeneath::fs_WriteStringToFile(Root, "A/B/File.txt", "Hello");
				DMibExpect(CFileBeneath::fs_ReadStringFromFile(Root, "A/B/File.txt", 100), ==, "Hello");
				DMibExpectTrue(CFileBeneath::fs_GetType(Root, "A/B/File.txt") == EFileBeneathType::mc_File);
				DMibExpectTrue(CFileBeneath::fs_GetType(Root, "A/B") == EFileBeneathType::mc_Directory);
				DMibExpectTrue(CFileBeneath::fs_GetType(Root, "A/Missing") == EFileBeneathType::mc_None);
				DMibExpectException
					(
						CFileBeneath::fs_ReadStringFromFile(Root, "A/B/File.txt", 2)
						, DMibImpErrorInstance(NMib::NFile::CExceptionFile, "'{}' is 5 bytes, more than the 2 allowed"_f << (Root / "A/B/File.txt"))
					)
				;

				auto Entries = CFileBeneath::fs_ListDirectory(Root, "A/B");
				DMibAssert(Entries.f_GetLen(), ==, 1);
				DMibExpect(Entries[0].m_Name, ==, "File.txt");
			};

			DMibTestCategory("Links")
			{
				CFile::fs_CreateDirectory(Root / "Links");
				DMibAssert(::symlink((Outside / "Secret.txt").f_GetStr(), (Root / "Links/ToSecret").f_GetStr()), ==, 0);
				DMibAssert(::symlink(Outside.f_GetStr(), (Root / "Links/ToOutside").f_GetStr()), ==, 0);

				DMibExpectException
					(
						CFileBeneath::fs_ReadFile(Root, "Links/ToSecret", 100)
						, DMibImpErrorInstance(NMib::NFile::CExceptionFile, "'{}' isn't a regular file"_f << (Root / "Links/ToSecret"))
					)
				;
				DMibExpectException
					(
						CFileBeneath::fs_ReadFile(Root, "Links/ToOutside/Secret.txt", 100)
						, DMibImpErrorInstance
						(
							NMib::NFile::CExceptionFile
							, "Failed to open the directory 'Links/ToOutside' under '{}' without following links: {}"_f << Root << NMib::NPlatform::fg_ErrnoString<CStr>(NotFollowedError)
						)
					)
				;
				DMibExpectTrue(CFileBeneath::fs_GetType(Root, "Links/ToSecret") == EFileBeneathType::mc_Other);

				// Writing through a planted link replaces the link and leaves its target alone
				CFileBeneath::fs_WriteStringToFile(Root, "Links/ToSecret", "replaced");
				DMibExpect(CFile::fs_ReadStringFromFile(Outside / "Secret.txt", true), ==, "secret");
				DMibExpectTrue(CFileBeneath::fs_GetType(Root, "Links/ToSecret") == EFileBeneathType::mc_File);

				DMibExpectException
					(
						CFileBeneath::fs_WriteStringToFile(Root, "Links/ToOutside/New.txt", "x")
						, DMibImpErrorInstance
						(
							NMib::NFile::CExceptionFile
							, "Failed to open the directory 'Links/ToOutside' under '{}' without following links: {}"_f << Root << NMib::NPlatform::fg_ErrnoString<CStr>(NotFollowedError)
						)
					)
				;
				DMibExpectFalse(CFile::fs_FileExists(Outside / "New.txt"));

				// Deleting a tree deletes links inside it as links
				CFileBeneath::fs_DeleteRecursive(Root, "Links");
				DMibExpectTrue(CFile::fs_FileExists(Outside / "Secret.txt"));
				DMibExpectTrue(CFileBeneath::fs_GetType(Root, "Links") == EFileBeneathType::mc_None);
			};

			DMibTestCategory("LongName")
			{
				// The temporary file of a write isn't named after the target, so a name as long as a name can be is writable
				CStr Name;
				for (umint iCharacter = 0; iCharacter < 255; ++iCharacter)
					Name += "n";
				CFileBeneath::fs_WriteStringToFile(Root, Name, "long");
				DMibExpect(CFileBeneath::fs_ReadStringFromFile(Root, Name, 100), ==, "long");
			};

			DMibTestCategory("ConcurrentWrites")
			{
				// Writes of one target from several threads each use a temporary file of their own
				NAtomic::TCAtomic<umint> nFailed = 0;
				TCVector<NStorage::TCUniquePointer<NThread::CThreadObject>> Threads;
				for (umint iThread = 0; iThread < 8; ++iThread)
				{
					Threads.f_Insert
						(
							NThread::CThreadObject::fs_StartThread
							(
								[&, iThread](NThread::CThreadObject *_pThread) -> aint
								{
									for (umint iWrite = 0; iWrite < 50; ++iWrite)
									{
										try
										{
											CFileBeneath::fs_WriteStringToFile(Root, "Concurrent.txt", "{}"_f << iThread);
										}
										catch (NException::CException const &)
										{
											nFailed.f_FetchAdd(1);
										}
									}

									return 0;
								}
								, "FileBeneath write"
							)
						)
					;
				}

				Threads.f_Clear();

				DMibExpect(nFailed.f_Load(), ==, 0);
				DMibExpect(CFileBeneath::fs_ReadStringFromFile(Root, "Concurrent.txt", 100).f_GetLen(), ==, 1);
				TCVector<CStr> Temporary;
				for (auto &File : CFile::fs_FindFiles(Root / "*", EFileAttrib_File))
				{
					if (CFile::fs_GetFile(File).f_Find(".tmp") >= 0)
						Temporary.f_Insert(File);
				}

				DMibExpect(Temporary, ==, TCVector<CStr>{});
			};

			DMibTestCategory("RootLink")
			{
				// A root that is a link isn't followed, also when it is given with a trailing slash or ending in "."
				CStr LinkRoot = Base / "LinkRoot";
				DMibAssert(::symlink(Outside.f_GetStr(), LinkRoot.f_GetStr()), ==, 0);

				for (CStr Given : {LinkRoot, CStr(LinkRoot + "/"), CStr(LinkRoot + "//")})
				{
					DMibTestPath(Given);
					DMibExpectException
						(
							CFileBeneath::fs_ReadFile(Given, "Secret.txt", 100)
							, DMibImpErrorInstance(NMib::NFile::CExceptionFile, "Failed to open the directory '{}': {}"_f << Given << NMib::NPlatform::fg_ErrnoString<CStr>(NotFollowedError))
						)
					;
				}

				for (CStr Given : {CStr(LinkRoot + "/."), CStr(Root + "/..")})
				{
					DMibTestPath(Given);
					DMibExpectException
						(
							CFileBeneath::fs_ReadFile(Given, "Secret.txt", 100)
							, DMibImpErrorInstance(NMib::NFile::CExceptionFile, "The directory '{}' doesn't end in a name, so it can't be opened without following links"_f << Given)
						)
					;
				}

				// The root itself is checked like any other path: a link there isn't a directory
				DMibExpectException
					(
						CFileBeneath::fs_GetType(LinkRoot, ".")
						, DMibImpErrorInstance(NMib::NFile::CExceptionFile, "Failed to open the directory '{}': {}"_f << LinkRoot << NMib::NPlatform::fg_ErrnoString<CStr>(NotFollowedError))
					)
				;
				DMibExpectTrue(CFileBeneath::fs_GetType(Root, ".") == EFileBeneathType::mc_Directory);

				CFileBeneath::fs_WriteStringToFile(Root + "/", "TrailingSlash.txt", "x");
				DMibExpect(CFile::fs_ReadStringFromFile(Root / "TrailingSlash.txt", true), ==, "x");
			};

			DMibTestCategory("Permissions")
			{
				// A directory that can't be opened hides what is in it, so checking past it fails instead of finding nothing
				CFileBeneath::fs_WriteStringToFile(Root, "Hidden/Inner/config", "x");
				DMibAssert(::symlink(Outside.f_GetStr(), (Root / "ToOutside").f_GetStr()), ==, 0);
				DMibExpectTrue(CFileBeneath::fs_GetType(Root, "ToOutside/Secret.txt") == EFileBeneathType::mc_None);
				DMibExpectException
					(
						CFileBeneath::fs_ReadFile(Root / "ToOutside", "Secret.txt", 100)
						, DMibImpErrorInstance
						(
							NMib::NFile::CExceptionFile
							, "Failed to open the directory '{}': {}"_f << (Root / "ToOutside") << NMib::NPlatform::fg_ErrnoString<CStr>(NotFollowedError)
						)
					)
				;
				DMibExpectTrue(CFileBeneath::fs_GetType(Root, "Hidden/Inner/config/Below") == EFileBeneathType::mc_None);

				auto RestoreModes = g_OnScopeExit / [&]
					{
						::chmod((Root / "Hidden").f_GetStr(), 0755);
					}
				;

				if (::geteuid() != 0)
				{
					DMibAssert(::chmod((Root / "Hidden").f_GetStr(), 0), ==, 0);
					auto HiddenError = DMibImpErrorInstance
						(
							NMib::NFile::CExceptionFile
							, "Failed to open the directory 'Hidden' under '{}' without following links: {}"_f << Root << NMib::NPlatform::fg_ErrnoString<CStr>(EACCES)
						)
					;
					DMibExpectException(CFileBeneath::fs_GetType(Root, "Hidden/Inner"), HiddenError);
					DMibExpectException(CFileBeneath::fs_GetType(Root, "Hidden/Inner/config"), HiddenError);

					// Listable but without search permission: its entries can't be checked
					DMibAssert(::chmod((Root / "Hidden").f_GetStr(), 0400), ==, 0);
					auto Entries = CFileBeneath::fs_ListDirectory(Root, "Hidden");
					DMibAssert(Entries.f_GetLen(), ==, 1);
					DMibExpectTrue(Entries[0].m_Type == EFileBeneathType::mc_Other);
				}
			};

			DMibTestCategory("Fifo")
			{
				DMibAssert(::mkfifo((Root / "Fifo").f_GetStr(), 0644), ==, 0);
				DMibExpectException(CFileBeneath::fs_ReadFile(Root, "Fifo", 100), DMibImpErrorInstance(NMib::NFile::CExceptionFile, "'{}' isn't a regular file"_f << (Root / "Fifo")));
				DMibExpectTrue(CFileBeneath::fs_GetType(Root, "Fifo") == EFileBeneathType::mc_Other);
			};

			DMibTestCategory("Paths")
			{
				DMibExpectException
					(
						CFileBeneath::fs_ReadFile(Root, "../Outside/Secret.txt", 100)
						, DMibImpErrorInstance(NMib::NFile::CExceptionFile, "'../Outside/Secret.txt' has a component that could leave its root")
					)
				;
				DMibExpectException
					(
						CFileBeneath::fs_ReadFile(Root, "/etc/passwd", 100)
						, DMibImpErrorInstance(NMib::NFile::CExceptionFile, "'/etc/passwd' is an absolute path, not one relative to a root")
					)
				;
				DMibExpect(CFileBeneath::fs_SplitRelativePath("./A//B/"), ==, (TCVector<CStr>{"A", "B"}));
				DMibExpect(CFileBeneath::fs_SplitRelativePath("."), ==, TCVector<CStr>{});

				// Malterlib checks out on Windows too, so a name Windows can't hold is refused
				DMibExpectException
					(
						CFileBeneath::fs_SplitRelativePath("A/aux.txt")
						, DMibImpErrorInstance(NMib::NFile::CExceptionFile, "'A/aux.txt' isn't a safe relative path: it can't contain AUX alone or with dots on both or either sides")
					)
				;
				DMibExpectException
					(
						CFileBeneath::fs_SplitRelativePath("A/b:c")
						, DMibImpErrorInstance(NMib::NFile::CExceptionFile, "'A/b:c' isn't a safe relative path: it can't the character :")
					)
				;
			};

			DMibTestCategory("Owner")
			{
				CFileBeneath::fs_WriteStringToFile(Root, "Owned/Dir/File.txt", "x");
				CStr Owner = CFile::fs_GetOwner(Root / "Owned/Dir");
				CStr Group = CFile::fs_GetGroup(Root / "Owned/Dir");

				CFileBeneath::fs_SetOwnerAndGroup(Root, "Owned/Dir", Owner, Group);
				CFileBeneath::fs_SetOwnerAndGroup(Root, "", Owner, Group);
				CFileBeneath::fs_SetOwnerAndGroup(Root, "Owned/Dir/File.txt", "", Group);

				{
					DMibTestPath("Search only");

					// The directories leading to the path need search permission only, as for chown
					DMibAssert(::chmod((Root / "Owned").f_GetStr(), 0300), ==, 0);
					auto RestoreMode = g_OnScopeExit / [&]
						{
							::chmod((Root / "Owned").f_GetStr(), 0755);
						}
					;

					CFileBeneath::fs_SetOwnerAndGroup(Root, "Owned/Dir/File.txt", "", Group);
				}

				{
					DMibTestPath("Search only root");

					DMibAssert(::chmod((Root / "Owned/Dir").f_GetStr(), 0300), ==, 0);
					auto RestoreMode = g_OnScopeExit / [&]
						{
							::chmod((Root / "Owned/Dir").f_GetStr(), 0755);
						}
					;

					CFileBeneath::fs_SetOwnerAndGroup(Root / "Owned/Dir", "", "", Group);
					CFileBeneath::fs_SetOwnerAndGroup(Root / "Owned/Dir", ".", "", Group);
				}

				// A link in the directories leading to the path isn't followed
				CFile::fs_CreateDirectory(Root / "OwnerLinks");
				DMibAssert(::symlink(Outside.f_GetStr(), (Root / "OwnerLinks/ToOutside").f_GetStr()), ==, 0);
				DMibExpectException
					(
						CFileBeneath::fs_SetOwnerAndGroup(Root, "OwnerLinks/ToOutside/Secret.txt", Owner, Group)
						, DMibImpErrorInstance
						(
							NMib::NFile::CExceptionFile
							, "Failed to open the directory 'OwnerLinks/ToOutside' under '{}' without following links: {}"_f
							<< Root
							<< NMib::NPlatform::fg_ErrnoString<CStr>(NotFollowedError)
						)
					)
				;

				// A link at the path gets the group itself: following a dangling link would fail
				DMibAssert(::symlink((Root / "Missing").f_GetStr(), (Root / "OwnerLinks/Dangling").f_GetStr()), ==, 0);
				CFileBeneath::fs_SetOwnerAndGroup(Root, "OwnerLinks/Dangling", "", CFile::fs_GetGroupOnLink(Root / "OwnerLinks/Dangling"));

				// A link at the path gets the group itself, which shows with a group of this user that its target doesn't have
				struct stat OutsideStat;
				DMibAssert(::stat(Outside.f_GetStr(), &OutsideStat), ==, 0);

				gid_t Groups[256];
				int nGroups = ::getgroups(256, Groups);
				CStr OtherGroup;
				for (int iGroup = 0; iGroup < nGroups && OtherGroup.f_IsEmpty(); ++iGroup)
				{
					if (Groups[iGroup] == OutsideStat.st_gid)
						continue;

					NMib::NPlatform::CGetGrGidState State;
					if (group *pGroup = NMib::NPlatform::fg_Helper_GetGrGid(Groups[iGroup], State))
						OtherGroup = pGroup->gr_name;
				}

				if (!OtherGroup.f_IsEmpty())
				{
					CStr OutsideGroup = CFile::fs_GetGroup(Outside);
					CFileBeneath::fs_SetOwnerAndGroup(Root, "OwnerLinks/ToOutside", "", OtherGroup);
					DMibExpect(CFile::fs_GetGroup(Outside), ==, OutsideGroup);
					DMibExpect(CFile::fs_GetGroupOnLink(Root / "OwnerLinks/ToOutside"), ==, OtherGroup);
				}
			};

			DMibTestCategory("Rename")
			{
				CFileBeneath::fs_WriteStringToFile(Root, "From.txt", "1");
				CFileBeneath::fs_WriteStringToFile(Root, "Existing.txt", "2");
				DMibExpectFalse(CFileBeneath::fs_Rename(Root, "From.txt", "Existing.txt", true));
				DMibExpect(CFileBeneath::fs_ReadStringFromFile(Root, "Existing.txt", 10), ==, "2");
				DMibExpectTrue(CFileBeneath::fs_Rename(Root, "From.txt", "Moved/To.txt", true));
				DMibExpect(CFileBeneath::fs_ReadStringFromFile(Root, "Moved/To.txt", 10), ==, "1");
			};
		};
#endif
	}
}

DMibTestRegister(CFileBeneath_Tests, Malterlib::File);
