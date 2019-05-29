// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_VirtualFS.h"
#include "Malterlib_File_VirtualFS_MalterlibFS.h"

namespace NMib::NFile
{
	CFileSystemInterface_VirtualFS::CFileSystemInterface_VirtualFS(NMib::NFile::CVirtualFS &_VirtualFS)
		: mp_VirtualFS(_VirtualFS)
	{
	}

	NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> CFileSystemInterface_VirtualFS::f_OpenStream(const NStr::CStr &_FileName, NMib::NFile::EFileOpen _OpenFlags) const
	{
		NStorage::TCSharedPointer<NFile::TCBinaryStream_VirtualFSFile<NStream::CBinaryStreamDefaultRef>> pRet(fg_Construct());
		mp_VirtualFS.f_OpenFile(pRet->f_GetVFSFile(), _FileName, _OpenFlags);
		return pRet;
	}
	EFileAttrib CFileSystemInterface_VirtualFS::f_GetAttributes(const NStr::CStr &_File) const
	{
		return (EFileAttrib)mp_VirtualFS.f_GetAttributes(_File);
	}

	void CFileSystemInterface_VirtualFS::f_SetAttributes(const NStr::CStr &_File, EFileAttrib _Attribs) const
	{
		return mp_VirtualFS.f_SetAttributes(_File, _Attribs);
	}

	void CFileSystemInterface_VirtualFS::f_SetCreationTime(NStr::CStr const& _File, const NTime::CTime &_Time)
	{
		mp_VirtualFS.f_SetCreationTime(_File, _Time);
	}

	void CFileSystemInterface_VirtualFS::f_SetAccessTime(NStr::CStr const& _File, const NTime::CTime &_Time)
	{
		mp_VirtualFS.f_SetAccessTime(_File, _Time);
	}

	void CFileSystemInterface_VirtualFS::f_SetWriteTime(NStr::CStr const& _File, const NTime::CTime &_Time)
	{
		mp_VirtualFS.f_SetWriteTime(_File, _Time);
	}

	NTime::CTime CFileSystemInterface_VirtualFS::f_GetCreationTime(NStr::CStr const& _File) const
	{
		return mp_VirtualFS.f_GetCreationTime(_File);
	}

	NTime::CTime CFileSystemInterface_VirtualFS::f_GetAccessTime(NStr::CStr const& _File) const
	{
		return mp_VirtualFS.f_GetAccessTime(_File);
	}

	NTime::CTime CFileSystemInterface_VirtualFS::f_GetWriteTime(NStr::CStr const& _File) const
	{
		return mp_VirtualFS.f_GetWriteTime(_File);
	}

	void CFileSystemInterface_VirtualFS::f_DeleteFile(const NStr::CStr &_File) const
	{
		mp_VirtualFS.f_DeleteFile(_File);
	}
	void CFileSystemInterface_VirtualFS::f_DeleteDirectory(const NStr::CStr &_File) const
	{
		mp_VirtualFS.f_DeleteFile(_File);
	}
	void CFileSystemInterface_VirtualFS::f_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo) const
	{
		mp_VirtualFS.f_RenameFile(_FileFrom, _FileTo);
	}
	void CFileSystemInterface_VirtualFS::f_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress) const
	{
		return ICFileSystemInterface::f_RenameFile(_FileFrom, _FileTo, _Progress);
	}
	void CFileSystemInterface_VirtualFS::f_CreateDirectory(const NStr::CStr &_Path) const
	{
		mp_VirtualFS.f_CreateDirectory(_Path);
	}
	NContainer::TCVector<NStr::CStr> CFileSystemInterface_VirtualFS::f_FindFiles(const NStr::CStr &_FindPath, NFile::EFileAttrib _AttribMask, bool _bRecursive, bool _bFollowLinks) const
	{
		return mp_VirtualFS.f_FindFiles(_FindPath, _AttribMask, _bRecursive);
	}
	bool CFileSystemInterface_VirtualFS::f_FileExists(const NStr::CStr &_File, NFile::EFileAttrib _AttribMask) const
	{
		return mp_VirtualFS.f_FileExists(_File, _AttribMask);
	}

	NContainer::CByteVector CFileSystemInterface_VirtualFS::f_ReadFile(NStr::CStr const& _FileFrom) const
	{
		NContainer::CByteVector Ret;
		mp_VirtualFS.f_ReadFileToMemory(_FileFrom, Ret);

		return fg_Move(Ret);
	}

	NMib::NStream::CFilePos CFileSystemInterface_VirtualFS::f_GetFreeSpace(NMib::NStr::CStr const& _Path) const
	{
		return mp_VirtualFS.f_GetFreeSize();
	}

	NMib::NStream::CFilePos CFileSystemInterface_VirtualFS::f_GetUsedSpace(NMib::NStr::CStr const& _Path) const
	{
		return mp_VirtualFS.f_GetTotalSize();
	}

	bool CFileSystemInterface_VirtualFS::f_IsValid() const
	{
		return mp_VirtualFS.f_IsValid();
	}

	void CFileSystemInterface_VirtualFS::f_CreateSymbolicLink(NMib::NStr::CStr const &_FileFrom, NMib::NStr::CStr const &_FileTo, NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags) const
	{
		f_WriteStringToFile(_FileTo, _FileFrom);
		f_SetAttributes(_FileTo, f_GetAttributes(_FileTo) | EFileAttrib_Link);
	}
	NMib::NStr::CStr CFileSystemInterface_VirtualFS::f_ResolveSymbolicLink(NMib::NStr::CStr const &_FileFrom) const
	{
		return f_ReadStringFromFile(_FileFrom);
	}
}
