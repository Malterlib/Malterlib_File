// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Malterlib_File_FileBeneath.h"

#ifndef DPlatformFamily_Windows
#	include <Mib/Core/PlatformSpecific/PosixErrNo>
#	include <errno.h>
#	include <fcntl.h>
#	include <stdio.h>
#	include <string.h>
#	include <unistd.h>
#	include <dirent.h>
#	include <sys/stat.h>
#	ifdef DPlatformFamily_Linux
#		include <sys/syscall.h>
#		include <linux/fs.h>
#	endif
#endif

namespace NMib::NFile
{
	using namespace NMib::NStr;
	using namespace NMib::NContainer;

#ifndef DPlatformFamily_Windows
	namespace
	{
		/// A file descriptor closed with the object
		struct CDescriptor
		{
			CDescriptor() = default;

			explicit CDescriptor(int _Descriptor);

			CDescriptor(CDescriptor &&_Other);

			~CDescriptor();

			CDescriptor &operator = (CDescriptor &&_Other);

			void f_Close();

			int m_Descriptor = -1;
		};

		CDescriptor::CDescriptor(int _Descriptor)
			: m_Descriptor(_Descriptor)
		{
		}

		CDescriptor::CDescriptor(CDescriptor &&_Other)
			: m_Descriptor(fg_Exchange(_Other.m_Descriptor, -1))
		{
		}

		CDescriptor::~CDescriptor()
		{
			f_Close();
		}

		CDescriptor &CDescriptor::operator = (CDescriptor &&_Other)
		{
			if (this != &_Other)
			{
				f_Close();
				m_Descriptor = fg_Exchange(_Other.m_Descriptor, -1);
			}

			return *this;
		}

		void CDescriptor::f_Close()
		{
			if (m_Descriptor >= 0)
				::close(m_Descriptor);

			m_Descriptor = -1;
		}

		// Numbers the temporary files of the writes in this process, so concurrent writes of one target don't share one
		constinit NAtomic::TCAtomic<uint64> g_iTemporaryFile = 0;

		// Writers in other PID namespaces, such as containers sharing a directory, can have the same process id and numbers
		constexpr umint gc_MaxTemporaryFileAttempts = 1000;

		// macOS refuses a read or write of more than INT_MAX bytes in one call
		constexpr umint gc_MaxIoSize = umint(1) << 30;

		EFileBeneathType fg_TypeFromMode(mode_t _Mode)
		{
			if (S_ISREG(_Mode))
				return EFileBeneathType::mc_File;

			if (S_ISDIR(_Mode))
				return EFileBeneathType::mc_Directory;

			return EFileBeneathType::mc_Other;
		}

		/// Opens the root as a directory. The directories above it are trusted, but the root itself can be a name in a directory someone
		/// else writes, such as a checkout in a directory a container has mounted, so a link there isn't followed. O_NOFOLLOW only
		/// applies to the last component, and a trailing slash or a last component of "." or ".." makes that something other than the
		/// root's own name, which would follow a link the root's name is.
		CDescriptor fg_OpenRoot(CStr const &_Root)
		{
			CStr Root = _Root;
			while (Root.f_GetLen() > 1 && Root.f_EndsWith("/"))
				Root = Root.f_Left(Root.f_GetLen() - 1);

			CStr Name = Root.f_GetLen() > 1 ? CFile::fs_GetFile(Root) : CStr();
			if (Name == "." || Name == "..")
				DMibErrorFile("The directory '{}' doesn't end in a name, so it can't be opened without following links"_f << _Root);

			int Descriptor = ::open(Root.f_GetStr(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
			if (Descriptor < 0)
				DMibErrorFile("Failed to open the directory '{}': {}"_f << _Root << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

			return CDescriptor(Descriptor);
		}

		/// Opens each directory of _Components under _Root without following a symbolic link at any step. With _bCreate missing
		/// directories are created, which a planted symbolic link with the same name makes fail instead of being followed.
		CDescriptor fg_OpenDirectory(CStr const &_Root, TCVector<CStr> const &_Components, umint _nComponents, bool _bCreate)
		{
			CDescriptor Current = fg_OpenRoot(_Root);

			for (umint iComponent = 0; iComponent < _nComponents; ++iComponent)
			{
				CStr const &Component = _Components[iComponent];
				int Descriptor = ::openat(Current.m_Descriptor, Component.f_GetStr(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
				if (Descriptor < 0 && errno == ENOENT && _bCreate)
				{
					if (::mkdirat(Current.m_Descriptor, Component.f_GetStr(), 0755) != 0 && errno != EEXIST)
						DMibErrorFile("Failed to create '{}' under '{}': {}"_f << Component << _Root << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

					Descriptor = ::openat(Current.m_Descriptor, Component.f_GetStr(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
				}

				if (Descriptor < 0)
				{
					int Error = errno;
					DMibErrorFile
						(
							"Failed to open the directory '{}' under '{}' without following links: {}"_f
							<< CStr::fs_Join(TCVector<CStr>(_Components.f_GetArray(), iComponent + 1), "/")
							<< _Root
							<< NMib::NPlatform::fg_ErrnoString<CStr>(Error)
						)
					;
				}

				Current = CDescriptor(Descriptor);
			}

			return Current;
		}

		/// Opens an existing regular file for reading without following a link, and opens nothing else for I/O: opening a device can make
		/// its driver act, whatever is checked after. On Linux the name is opened as a path only, checked, and that same inode reopened
		/// through /proc. Elsewhere it is checked before the open, and only root can make a device there.
		CDescriptor fg_OpenExistingFile(int _Directory, CStr const &_Name, CStr const &_Description)
		{
#	ifdef DPlatformFamily_Linux
			int PathDescriptor = ::openat(_Directory, _Name.f_GetStr(), O_PATH | O_NOFOLLOW | O_CLOEXEC);
			if (PathDescriptor < 0)
				DMibErrorFile("Failed to open '{}' without following links: {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

			CDescriptor Path(PathDescriptor);

			struct stat Stat;
			if (::fstat(Path.m_Descriptor, &Stat) != 0)
				DMibErrorFile("Failed to check '{}': {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

			if (!S_ISREG(Stat.st_mode))
				DMibErrorFile("'{}' isn't a regular file"_f << _Description);

			int Descriptor = ::open(CStr("/proc/self/fd/{}"_f << Path.m_Descriptor).f_GetStr(), O_RDONLY | O_CLOEXEC);
			if (Descriptor < 0)
				DMibErrorFile("Failed to open '{}': {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

			return CDescriptor(Descriptor);
#	else
			struct stat Stat;
			if (::fstatat(_Directory, _Name.f_GetStr(), &Stat, AT_SYMLINK_NOFOLLOW) != 0)
				DMibErrorFile("Failed to open '{}' without following links: {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

			if (!S_ISREG(Stat.st_mode))
				DMibErrorFile("'{}' isn't a regular file"_f << _Description);

			// Non-blocking, so a FIFO put there since the check can't hang the open, and checked again on the descriptor
			int Descriptor = ::openat(_Directory, _Name.f_GetStr(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
			if (Descriptor < 0)
				DMibErrorFile("Failed to open '{}' without following links: {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

			CDescriptor File(Descriptor);
			if (::fstat(File.m_Descriptor, &Stat) != 0)
				DMibErrorFile("Failed to check '{}': {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

			if (!S_ISREG(Stat.st_mode))
				DMibErrorFile("'{}' isn't a regular file"_f << _Description);

			// Blocking again for the reads themselves
			int StatusFlags = ::fcntl(File.m_Descriptor, F_GETFL);
			if (StatusFlags >= 0)
				::fcntl(File.m_Descriptor, F_SETFL, StatusFlags & ~O_NONBLOCK);

			return File;
#	endif
		}

		bool fg_RenameAt(int _FromDirectory, CStr const &_FromName, int _ToDirectory, CStr const &_ToName, bool _bNoReplace, CStr const &_Description)
		{
			int Result = 0;
			if (_bNoReplace)
			{
#	if defined(DPlatformFamily_Linux)
				Result = int(::syscall(SYS_renameat2, _FromDirectory, _FromName.f_GetStr(), _ToDirectory, _ToName.f_GetStr(), RENAME_NOREPLACE));
#	elif defined(DPlatformFamily_macOS)
				Result = ::renameatx_np(_FromDirectory, _FromName.f_GetStr(), _ToDirectory, _ToName.f_GetStr(), RENAME_EXCL);
#	else
				DMibErrorFile("Renaming without replacing isn't supported on this platform");
#	endif
			}
			else
				Result = ::renameat(_FromDirectory, _FromName.f_GetStr(), _ToDirectory, _ToName.f_GetStr());

			if (Result == 0)
				return true;

			if (_bNoReplace && errno == EEXIST)
				return false;

			DMibErrorFile("Failed to rename '{}': {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
		}

		void fg_DeleteRecursiveAt(int _Directory, CStr const &_Name, CStr const &_Description)
		{
			struct stat Stat;
			if (::fstatat(_Directory, _Name.f_GetStr(), &Stat, AT_SYMLINK_NOFOLLOW) != 0)
			{
				if (errno == ENOENT)
					return;

				DMibErrorFile("Failed to check '{}': {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
			}

			if (!S_ISDIR(Stat.st_mode))
			{
				if (::unlinkat(_Directory, _Name.f_GetStr(), 0) != 0 && errno != ENOENT)
					DMibErrorFile("Failed to delete '{}': {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

				return;
			}

			int Descriptor = ::openat(_Directory, _Name.f_GetStr(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
			if (Descriptor < 0)
				DMibErrorFile("Failed to open '{}': {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

			TCVector<CStr> Names;
			{
				int ListDescriptor = ::fcntl(Descriptor, F_DUPFD_CLOEXEC, 0);
				DIR *pDirectory = ListDescriptor >= 0 ? ::fdopendir(ListDescriptor) : nullptr;
				if (!pDirectory)
				{
					if (ListDescriptor >= 0)
						::close(ListDescriptor);
					::close(Descriptor);
					DMibErrorFile("Failed to list '{}': {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
				}

				while (auto *pEntry = ::readdir(pDirectory))
				{
					CStr Name = pEntry->d_name;
					if (Name != "." && Name != "..")
						Names.f_Insert(Name);
				}

				::closedir(pDirectory);
			}

			CDescriptor Directory(Descriptor);
			for (auto &Name : Names)
				fg_DeleteRecursiveAt(Directory.m_Descriptor, Name, _Description / Name);

			if (::unlinkat(_Directory, _Name.f_GetStr(), AT_REMOVEDIR) != 0 && errno != ENOENT)
				DMibErrorFile("Failed to delete the directory '{}': {}"_f << _Description << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
		}
	}
#endif

	/// The components of a relative path, refusing anything that could leave the root: an absolute path or "..". "." and empty
	/// components are dropped, so "." is the root, as a repository at the root of a checkout is. What remains must be a safe relative
	/// path by CFile::fs_IsSafeRelativePath, which refuses names that Windows can't hold. No components means the root itself.
	TCVector<CStr> CFileBeneath::fs_SplitRelativePath(CStr const &_RelativePath)
	{
		if (_RelativePath.f_StartsWith("/") || _RelativePath.f_StartsWith("\\"))
			DMibErrorFile("'{}' is an absolute path, not one relative to a root"_f << _RelativePath);

		TCVector<CStr> Components;
		for (auto &Component : _RelativePath.f_Split("/"))
		{
			if (Component.f_IsEmpty() || Component == ".")
				continue;

			if (Component == ".." || Component.f_FindChar('\\') >= 0 || Component.f_FindChar(0) >= 0)
				DMibErrorFile("'{}' has a component that could leave its root"_f << _RelativePath);

			Components.f_Insert(Component);
		}

		CStr Error;
		if (!CFile::fs_IsSafeRelativePath(CStr::fs_Join(Components, "/"), Error))
			DMibErrorFile("'{}' isn't a safe relative path: it can't {}"_f << _RelativePath << Error);

		return Components;
	}

#ifndef DPlatformFamily_Windows
	EFileBeneathType CFileBeneath::fs_GetType(CStr const &_Root, CStr const &_RelativePath)
	{
		auto Components = fs_SplitRelativePath(_RelativePath);
		// The root is a directory only once it opens as one, as for every other operation
		if (Components.f_IsEmpty())
		{
			fg_OpenRoot(_Root);
			return EFileBeneathType::mc_Directory;
		}

		// A missing directory, or one that is a link or a file, has nothing at the path that the other functions would open. A directory
		// that can't be opened for another reason, such as its permissions, hides what is in it, so that is an error.
		CDescriptor Directory = fg_OpenRoot(_Root);
		for (umint iComponent = 0; iComponent + 1 < Components.f_GetLen(); ++iComponent)
		{
			int Descriptor = ::openat(Directory.m_Descriptor, Components[iComponent].f_GetStr(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
			if (Descriptor < 0)
			{
				int Error = errno;
				if (Error == ENOENT || Error == ENOTDIR || Error == ELOOP)
					return EFileBeneathType::mc_None;

				DMibErrorFile
					(
						"Failed to open the directory '{}' under '{}' without following links: {}"_f
						<< CStr::fs_Join(TCVector<CStr>(Components.f_GetArray(), iComponent + 1), "/")
						<< _Root
						<< NMib::NPlatform::fg_ErrnoString<CStr>(Error)
					)
				;
			}

			Directory = CDescriptor(Descriptor);
		}

		struct stat Stat;
		if (::fstatat(Directory.m_Descriptor, Components.f_GetLast().f_GetStr(), &Stat, AT_SYMLINK_NOFOLLOW) != 0)
		{
			if (errno == ENOENT || errno == ENOTDIR)
				return EFileBeneathType::mc_None;

			DMibErrorFile("Failed to check '{}' under '{}': {}"_f << _RelativePath << _Root << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
		}

		return fg_TypeFromMode(Stat.st_mode);
	}

	CByteVector CFileBeneath::fs_ReadFile(CStr const &_Root, CStr const &_RelativePath, uint64 _MaxSize)
	{
		auto Components = fs_SplitRelativePath(_RelativePath);
		if (Components.f_IsEmpty())
			DMibErrorFile("'{}' names the root itself, not a file"_f << _RelativePath);

		auto Directory = fg_OpenDirectory(_Root, Components, Components.f_GetLen() - 1, false);
		auto File = fg_OpenExistingFile(Directory.m_Descriptor, Components.f_GetLast(), _Root / _RelativePath);

		struct stat Stat;
		if (::fstat(File.m_Descriptor, &Stat) != 0)
			DMibErrorFile("Failed to check '{}': {}"_f << _RelativePath << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

		if (uint64(Stat.st_size) > _MaxSize)
			DMibErrorFile("'{}' is {} bytes, more than the {} allowed"_f << (_Root / _RelativePath) << uint64(Stat.st_size) << _MaxSize);

		if (uint64(Stat.st_size) > uint64(TCLimitsInt<umint>::mc_Max))
			DMibErrorFile("'{}' is {} bytes, more than fits in memory here"_f << (_Root / _RelativePath) << uint64(Stat.st_size));

		CByteVector Data;
		Data.f_SetLen(umint(Stat.st_size));
		umint nRead = 0;
		while (nRead < Data.f_GetLen())
		{
			auto Result = ::read(File.m_Descriptor, Data.f_GetArray() + nRead, fg_Min(Data.f_GetLen() - nRead, gc_MaxIoSize));
			if (Result < 0)
			{
				if (errno == EINTR)
					continue;

				DMibErrorFile("Failed to read '{}': {}"_f << (_Root / _RelativePath) << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
			}

			if (Result == 0)
				break;

			nRead += umint(Result);
		}

		Data.f_SetLen(nRead);
		return Data;
	}

	CStr CFileBeneath::fs_ReadStringFromFile(CStr const &_Root, CStr const &_RelativePath, uint64 _MaxSize)
	{
		auto Data = fs_ReadFile(_Root, _RelativePath, _MaxSize);
		CStr String;
		String.f_AddStr((ch8 const *)Data.f_GetArray(), aint(Data.f_GetLen()));
		return String;
	}

	/// Writes a temporary file next to the target and renames it over the target, so a reader never sees half a file and a link
	/// planted at the target's name is replaced rather than followed
	void CFileBeneath::fs_WriteFile(CStr const &_Root, CStr const &_RelativePath, CByteVector const &_Data)
	{
		auto Components = fs_SplitRelativePath(_RelativePath);
		if (Components.f_IsEmpty())
			DMibErrorFile("'{}' names the root itself, not a file"_f << _RelativePath);

		auto Directory = fg_OpenDirectory(_Root, Components, Components.f_GetLen() - 1, true);
		// A write or rename that fails doesn't leave the temporary file behind
		CStr TempName;
		bool bCreated = false;
		auto RemoveTemporary = g_OnScopeExit / [&]
			{
				if (bCreated)
					::unlinkat(Directory.m_Descriptor, TempName.f_GetStr(), 0);
			}
		;

		// Not made from the target's name, which can be as long as a name can be. A name that is there already belongs to another
		// writer, or was left by one, so the next one is tried instead of removing it.
		CDescriptor File;
		for (umint iAttempt = 0; File.m_Descriptor < 0; ++iAttempt)
		{
			TempName = ".FileBeneath.{}-{}.tmp"_f << ::getpid() << g_iTemporaryFile.f_FetchAdd(1);
			int Descriptor = ::openat(Directory.m_Descriptor, TempName.f_GetStr(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644);
			if (Descriptor >= 0)
			{
				File = CDescriptor(Descriptor);
				bCreated = true;
			}
			else if (errno != EEXIST || iAttempt + 1 >= gc_MaxTemporaryFileAttempts)
				DMibErrorFile("Failed to create a temporary file for '{}': {}"_f << (_Root / _RelativePath) << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
		}

		umint nWritten = 0;
		while (nWritten < _Data.f_GetLen())
		{
			auto Result = ::write(File.m_Descriptor, _Data.f_GetArray() + nWritten, fg_Min(_Data.f_GetLen() - nWritten, gc_MaxIoSize));
			if (Result < 0)
			{
				if (errno == EINTR)
					continue;

				DMibErrorFile("Failed to write '{}': {}"_f << (_Root / _RelativePath) << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
			}

			nWritten += umint(Result);
		}

		// Some file systems, such as NFS, only report a failed write when the file is closed
		if (::close(fg_Exchange(File.m_Descriptor, -1)) != 0)
			DMibErrorFile("Failed to write '{}': {}"_f << (_Root / _RelativePath) << NMib::NPlatform::fg_ErrnoString<CStr>(errno));

		fg_RenameAt(Directory.m_Descriptor, TempName, Directory.m_Descriptor, Components.f_GetLast(), false, _Root / _RelativePath);
		RemoveTemporary.f_Clear();
	}

	void CFileBeneath::fs_WriteStringToFile(CStr const &_Root, CStr const &_RelativePath, CStr const &_Data)
	{
		CByteVector Data;
		Data.f_Insert((uint8 const *)_Data.f_GetStr(), _Data.f_GetLen());
		fs_WriteFile(_Root, _RelativePath, Data);
	}

	void CFileBeneath::fs_CreateDirectory(CStr const &_Root, CStr const &_RelativePath)
	{
		auto Components = fs_SplitRelativePath(_RelativePath);
		fg_OpenDirectory(_Root, Components, Components.f_GetLen(), true);
	}

	TCVector<CFileBeneathEntry> CFileBeneath::fs_ListDirectory(CStr const &_Root, CStr const &_RelativePath)
	{
		auto Components = fs_SplitRelativePath(_RelativePath);
		auto Directory = fg_OpenDirectory(_Root, Components, Components.f_GetLen(), false);

		int ListDescriptor = ::fcntl(Directory.m_Descriptor, F_DUPFD_CLOEXEC, 0);
		DIR *pDirectory = ListDescriptor >= 0 ? ::fdopendir(ListDescriptor) : nullptr;
		if (!pDirectory)
		{
			if (ListDescriptor >= 0)
				::close(ListDescriptor);

			DMibErrorFile("Failed to list '{}': {}"_f << (_Root / _RelativePath) << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
		}

		auto CloseDirectory = g_OnScopeExit / [pDirectory]
			{
				::closedir(pDirectory);
			}
		;

		TCVector<CFileBeneathEntry> Entries;
		while (true)
		{
			// readdir returns null both at the end and on an error, which only errno tells apart: a listing that stopped early isn't
			// a complete one
			errno = 0;
			auto *pEntry = ::readdir(pDirectory);
			if (!pEntry)
			{
				if (errno != 0)
					DMibErrorFile("Failed to list '{}': {}"_f << (_Root / _RelativePath) << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
				break;
			}

			CStr Name = pEntry->d_name;
			if (Name == "." || Name == "..")
				continue;

			// Gone since the listing is nothing; one that can't be checked, such as in a directory without search permission, is other
			struct stat Stat;
			EFileBeneathType Type = EFileBeneathType::mc_Other;
			if (::fstatat(Directory.m_Descriptor, Name.f_GetStr(), &Stat, AT_SYMLINK_NOFOLLOW) == 0)
				Type = fg_TypeFromMode(Stat.st_mode);
			else if (errno == ENOENT)
				Type = EFileBeneathType::mc_None;

			Entries.f_Insert(CFileBeneathEntry{.m_Name = Name, .m_Type = Type});
		}

		return Entries;
	}

	/// Renames within the root. With _bNoReplace an existing target makes it return false instead of replacing it, atomically.
	bool CFileBeneath::fs_Rename(CStr const &_Root, CStr const &_From, CStr const &_To, bool _bNoReplace)
	{
		auto FromComponents = fs_SplitRelativePath(_From);
		auto ToComponents = fs_SplitRelativePath(_To);
		if (FromComponents.f_IsEmpty() || ToComponents.f_IsEmpty())
			DMibErrorFile("A rename needs a file or directory, not the root");

		auto FromDirectory = fg_OpenDirectory(_Root, FromComponents, FromComponents.f_GetLen() - 1, false);
		auto ToDirectory = fg_OpenDirectory(_Root, ToComponents, ToComponents.f_GetLen() - 1, true);

		return fg_RenameAt
			(
				FromDirectory.m_Descriptor
				, FromComponents.f_GetLast()
				, ToDirectory.m_Descriptor
				, ToComponents.f_GetLast()
				, _bNoReplace
				, "{} -> {}"_f << (_Root / _From) << (_Root / _To)
			)
		;
	}

	void CFileBeneath::fs_Delete(CStr const &_Root, CStr const &_RelativePath)
	{
		auto Components = fs_SplitRelativePath(_RelativePath);
		if (Components.f_IsEmpty())
			DMibErrorFile("Refusing to delete the root");

		auto Directory = fg_OpenDirectory(_Root, Components, Components.f_GetLen() - 1, false);

		struct stat Stat;
		if (::fstatat(Directory.m_Descriptor, Components.f_GetLast().f_GetStr(), &Stat, AT_SYMLINK_NOFOLLOW) != 0)
		{
			if (errno == ENOENT)
				return;

			DMibErrorFile("Failed to check '{}': {}"_f << (_Root / _RelativePath) << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
		}

		if (::unlinkat(Directory.m_Descriptor, Components.f_GetLast().f_GetStr(), S_ISDIR(Stat.st_mode) ? AT_REMOVEDIR : 0) != 0 && errno != ENOENT)
			DMibErrorFile("Failed to delete '{}': {}"_f << (_Root / _RelativePath) << NMib::NPlatform::fg_ErrnoString<CStr>(errno));
	}

	/// Deletes a file or a directory tree under the root. Links inside are deleted as links, never followed.
	void CFileBeneath::fs_DeleteRecursive(CStr const &_Root, CStr const &_RelativePath)
	{
		auto Components = fs_SplitRelativePath(_RelativePath);
		if (Components.f_IsEmpty())
			DMibErrorFile("Refusing to delete the root");

		CDescriptor Directory;
		try
		{
			Directory = fg_OpenDirectory(_Root, Components, Components.f_GetLen() - 1, false);
		}
		catch (NException::CException const &)
		{
			if (fs_GetType(_Root, CStr::fs_Join(TCVector<CStr>(Components.f_GetArray(), Components.f_GetLen() - 1), "/")) == EFileBeneathType::mc_None)
				return;

			throw;
		}

		fg_DeleteRecursiveAt(Directory.m_Descriptor, Components.f_GetLast(), _Root / _RelativePath);
	}
#else
	EFileBeneathType CFileBeneath::fs_GetType(CStr const &_Root, CStr const &_RelativePath)
	{
		DMibErrorFile("CFileBeneath isn't supported on Windows");
	}

	CByteVector CFileBeneath::fs_ReadFile(CStr const &_Root, CStr const &_RelativePath, uint64 _MaxSize)
	{
		DMibErrorFile("CFileBeneath isn't supported on Windows");
	}

	CStr CFileBeneath::fs_ReadStringFromFile(CStr const &_Root, CStr const &_RelativePath, uint64 _MaxSize)
	{
		DMibErrorFile("CFileBeneath isn't supported on Windows");
	}

	void CFileBeneath::fs_WriteFile(CStr const &_Root, CStr const &_RelativePath, CByteVector const &_Data)
	{
		DMibErrorFile("CFileBeneath isn't supported on Windows");
	}

	void CFileBeneath::fs_WriteStringToFile(CStr const &_Root, CStr const &_RelativePath, CStr const &_Data)
	{
		DMibErrorFile("CFileBeneath isn't supported on Windows");
	}

	void CFileBeneath::fs_CreateDirectory(CStr const &_Root, CStr const &_RelativePath)
	{
		DMibErrorFile("CFileBeneath isn't supported on Windows");
	}

	TCVector<CFileBeneathEntry> CFileBeneath::fs_ListDirectory(CStr const &_Root, CStr const &_RelativePath)
	{
		DMibErrorFile("CFileBeneath isn't supported on Windows");
	}

	bool CFileBeneath::fs_Rename(CStr const &_Root, CStr const &_From, CStr const &_To, bool _bNoReplace)
	{
		DMibErrorFile("CFileBeneath isn't supported on Windows");
	}

	void CFileBeneath::fs_Delete(CStr const &_Root, CStr const &_RelativePath)
	{
		DMibErrorFile("CFileBeneath isn't supported on Windows");
	}

	void CFileBeneath::fs_DeleteRecursive(CStr const &_Root, CStr const &_RelativePath)
	{
		DMibErrorFile("CFileBeneath isn't supported on Windows");
	}
#endif
}
