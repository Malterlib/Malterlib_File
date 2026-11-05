// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once

#include <Mib/Core/Core>
#include <Mib/Container/Vector>

namespace NMib::NFile
{
	enum class EFileBeneathType : uint8
	{
		mc_None    // Nothing at the path
		, mc_File
		, mc_Directory
		, mc_Other // A symbolic link, FIFO, socket or device, which these functions never open; in a listing also an entry that can't be checked
	};

	struct CFileBeneathEntry
	{
		NStr::CStr m_Name;
		EFileBeneathType m_Type = EFileBeneathType::mc_None;
	};

	/// File access under a root directory that less trusted processes can write, such as a directory mounted into a container. Every
	/// component of the relative path is opened without following symbolic links, and ".." and absolute paths are refused, so nothing
	/// planted under the root can redirect an access to a file outside it. Files are opened non-blocking and refused unless they are
	/// regular files, so a FIFO or device in place of a file can neither block the caller nor be read. The root itself is trusted.
	struct CFileBeneath
	{
		static EFileBeneathType fs_GetType(NStr::CStr const &_Root, NStr::CStr const &_RelativePath);
		static NContainer::CByteVector fs_ReadFile(NStr::CStr const &_Root, NStr::CStr const &_RelativePath, uint64 _MaxSize);
		static NStr::CStr fs_ReadStringFromFile(NStr::CStr const &_Root, NStr::CStr const &_RelativePath, uint64 _MaxSize);
		static void fs_WriteFile(NStr::CStr const &_Root, NStr::CStr const &_RelativePath, NContainer::CByteVector const &_Data);
		static void fs_WriteStringToFile(NStr::CStr const &_Root, NStr::CStr const &_RelativePath, NStr::CStr const &_Data);
		static void fs_CreateDirectory(NStr::CStr const &_Root, NStr::CStr const &_RelativePath);
		static NContainer::TCVector<CFileBeneathEntry> fs_ListDirectory(NStr::CStr const &_Root, NStr::CStr const &_RelativePath);
		static bool fs_Rename(NStr::CStr const &_Root, NStr::CStr const &_From, NStr::CStr const &_To, bool _bNoReplace);
		static void fs_Delete(NStr::CStr const &_Root, NStr::CStr const &_RelativePath);
		static void fs_DeleteRecursive(NStr::CStr const &_Root, NStr::CStr const &_RelativePath);
		static NContainer::TCVector<NStr::CStr> fs_SplitRelativePath(NStr::CStr const &_RelativePath);
	};
}

#ifndef DMibPNoShortCuts
using namespace NMib::NFile;
#endif
