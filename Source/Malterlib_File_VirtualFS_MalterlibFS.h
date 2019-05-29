// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/File/VirtualFS>
#include <Mib/File/MalterlibFS>

namespace NMib::NFile
{
	class CFileSystemInterface_VirtualFS : public ICFileSystemInterface
	{
		NMib::NFile::CVirtualFS &mp_VirtualFS;
	public:

		CFileSystemInterface_VirtualFS(NMib::NFile::CVirtualFS & _VirtualFS);

		bool f_IsValid() const override;

		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> f_OpenStream(NStr::CStr const& _FileName, NMib::NFile::EFileOpen _OpenFlags) const override;
		EFileAttrib f_GetAttributes(NStr::CStr const& _File) const override;
		void f_SetAttributes(NStr::CStr const& _File, EFileAttrib _Attribs) const override;
		void f_DeleteFile(NStr::CStr const& _File) const override;
		void f_DeleteDirectory(NStr::CStr const& _File) const override;
		void f_RenameFile(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo) const override;
		void f_RenameFile(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo, CFileProgress & _Progress) const override;
		void f_CreateDirectory(NStr::CStr const& _Path) const override;
		NContainer::TCVector<NStr::CStr> f_FindFiles(NStr::CStr const& _FindPath, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File, bool _bRecursive = false, bool _bFollowLinks = true) const override;
		bool f_FileExists(NStr::CStr const& _File, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File) const override;
		NContainer::CByteVector f_ReadFile(NStr::CStr const& _FileFrom) const override;
		NMib::NStream::CFilePos f_GetFreeSpace(NMib::NStr::CStr const& _Path) const override;
		NMib::NStream::CFilePos f_GetUsedSpace(NMib::NStr::CStr const& _Path) const override;

		void f_SetCreationTime(NStr::CStr const& _File, const NTime::CTime &_Time) override;
		void f_SetAccessTime(NStr::CStr const& _File, const NTime::CTime &_Time) override;
		void f_SetWriteTime(NStr::CStr const& _File, const NTime::CTime &_Time) override;
		NTime::CTime f_GetCreationTime(NStr::CStr const& _File) const override;
		NTime::CTime f_GetAccessTime(NStr::CStr const& _File) const override;
		NTime::CTime f_GetWriteTime(NStr::CStr const& _File) const override;

		void f_CreateSymbolicLink(NMib::NStr::CStr const &_FileFrom, NMib::NStr::CStr const &_FileTo, NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags) const override;
		NMib::NStr::CStr f_ResolveSymbolicLink(NMib::NStr::CStr const &_FileFrom) const override;
	};
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif
