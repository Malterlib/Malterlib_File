// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once 

namespace NMib
{
	namespace NFile
	{
		class ICFileSystemInterface
		{
			void fpr_DeleteFilesInDirs(NStr::CStr const& _File, bool _bRemoveWriteProtection, NStr::CStr & _Errors) const;
			
		public:
			virtual ~ICFileSystemInterface();
			// Minimal interface
			virtual NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> f_OpenStream(NStr::CStr const& _FileName, NMib::NFile::EFileOpen _OpenFlags) const = 0;
			virtual void f_DeleteFile(NStr::CStr const& _File) const = 0;
			virtual void f_DeleteDirectory(NStr::CStr const& _File) const = 0;
			virtual void f_RenameFile(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo) const = 0;
			virtual void f_CreateDirectory(NStr::CStr const& _Path) const = 0;
			virtual NContainer::TCVector<NStr::CStr> f_FindFiles(NStr::CStr const& _FindPath, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File, bint _bRecursive = false, bool _bFollowLinks = true) const = 0;
			virtual bint f_FileExists(NStr::CStr const& _File, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File) const = 0;

			virtual EFileAttrib f_GetAttributes(NStr::CStr const& _File) const = 0;
			virtual void f_SetAttributes(NStr::CStr const& _File, EFileAttrib _Attribs) const = 0;

			virtual void f_SetCreationTime(NStr::CStr const& _File, const NTime::CTime &_Time) = 0;
			virtual void f_SetAccessTime(NStr::CStr const& _File, const NTime::CTime &_Time) = 0;
			virtual void f_SetWriteTime(NStr::CStr const& _File, const NTime::CTime &_Time) = 0;
			virtual NTime::CTime f_GetCreationTime(NStr::CStr const& _File) const = 0;
			virtual NTime::CTime f_GetAccessTime(NStr::CStr const& _File) const = 0;
			virtual NTime::CTime f_GetWriteTime(NStr::CStr const& _File) const = 0;

			// Optional interface
			
			virtual void f_CreateSymbolicLink(NMib::NStr::CStr const &_FileFrom, NMib::NStr::CStr const &_FileTo, NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags) const;
			virtual void f_CreateHardLink(NMib::NStr::CStr const &_FileFrom, NMib::NStr::CStr const &_FileTo) const;
			virtual NMib::NStr::CStr f_ResolveSymbolicLink(NMib::NStr::CStr const &_FileFrom) const;
			virtual bool f_CanCreateSymbolicLink(NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags) const;
			
			virtual bint f_MakeFileWritable(NStr::CStr const& _LocalPath, bint _bWritable = true) const;
			virtual bint f_IsFileWritable(NStr::CStr const& _LocalPath) const;
			virtual NContainer::TCVector<NStr::CStr> f_FindFilesNoRoot(NStr::CStr const& _FindPath, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File, bint _bRecursive = false, bool _bFollowLinks = true) const;
			virtual bool f_IsValid() const;
			
			virtual void f_DeleteDirectoryRecursive(NStr::CStr const& _File, bint _bRemoveWriteProtection = false) const;
			virtual void f_CopyFile(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo, CFileProgress & _Progress) const;
			virtual void f_CopyFile(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo) const;
			virtual void f_CopyFileRaw(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo) const;
			virtual void f_CopyFileRaw(NStr::CStr const& _FileFrom, ICFileSystemInterface & _ToFS, NStr::CStr const& _FileTo) const;
			virtual void f_WriteFile(NContainer::TCVector<uint8> const& _FileFrom, NStr::CStr const& _FileTo) const;
			virtual NContainer::TCVector<uint8> f_ReadFile(NStr::CStr const& _FileFrom) const;
			virtual void f_CopyFiles(NStr::CStr const& _FindPath, NStr::CStr const& _ToPath, bint _bRecursive = true, bint _bRaw = false, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File) const;
			virtual bint f_FileIsSame(NContainer::TCVector<uint8> const& _SourceData, NStr::CStr const& _ToFileName) const;
			virtual bint f_CopyFileDiff(NContainer::TCVector<uint8> const& _SourceData, NStr::CStr const& _ToFileName, NTime::CTime const& _FileTime, EFileAttrib _AddAttribs = EFileAttrib_None) const;
			virtual bint f_CopyFileDiff(NStr::CStr const& _FromFileName, NStr::CStr const& _ToFileName, bint _bCopyDate) const;
			virtual void f_RenameFile(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo, CFileProgress & _Progress) const;
			virtual NMib::NStream::CFilePos f_GetFreeSpace(NMib::NStr::CStr const& _Path) const;
			virtual NMib::NStream::CFilePos f_GetUsedSpace(NMib::NStr::CStr const& _Path) const;
			virtual CMibFilePos f_GetFileSize(NStr::CStr const& _Path) const;
			virtual NDataProcessing::CHashDigest_MD5 f_GetFileChecksum(NStr::CStr const& _Path) const;
			virtual void f_WriteStringToFile(NStr::CStr const& _Path, NStr::CStr const& _ToWrite, bool _bAddBOM = true) const;
			virtual NStr::CStr f_ReadStringFromFile(NStr::CStr const& _Path, bool _bAssumeUTF8 = false) const;
			void f_CopyFiles(NStr::CStr const& _FindPath, ICFileSystemInterface & _ToFS, NStr::CStr const& _ToPath, bint _bRecursive = true, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File) const;
			void f_CopyFilesWithAttribs(NStr::CStr const& _FindPath, ICFileSystemInterface & _ToFS, NStr::CStr const& _ToPath, bint _bRecursive = true, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File) const;

			class CCopyFilesProgress
			{
			public:

				//! Reports data copied.
				/*! Return false to abort. */
				virtual bool f_IncreaseProgress(uint64 _nInc) = 0;
				virtual void f_StepDown(uint64 _nMax) = 0;
				virtual void f_StepUp() = 0;
			};

			enum ECopyFilesBackupFlag
			{
				ECopyFilesBackupFlag_None = 0
				, ECopyFilesBackupFlag_Incremental = DMibBit(0)
				, ECopyFilesBackupFlag_MarkDestBackedUp = DMibBit(1)
				, ECopyFilesBackupFlag_ClearDestBackedUp = DMibBit(2)
			};

			//! Copy files recursively.
			/*! Returns true if completed successfully. */
			virtual bool f_CopyFiles_Backup
				(
					NStr::CStr const& _Path
					, ICFileSystemInterface & _ToFS
					, ECopyFilesBackupFlag _Flags
					, NFunction::TCFunction<bool (NStr::CStr const& _Filename)> const& _fShouldBackup
					, CCopyFilesProgress * _pProgress = nullptr
					, NContainer::TCMap<NStr::CStr, NTime::CTime> * _pSourceCleared = nullptr
				)
			;
		};

		class CFileSystemInterface_Disk : public ICFileSystemInterface
		{
		public:

			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> f_OpenStream(NStr::CStr const& _FileName, NMib::NFile::EFileOpen _OpenFlags) const override;
			EFileAttrib f_GetAttributes(NStr::CStr const& _File) const override;
			void f_SetAttributes(NStr::CStr const& _File, EFileAttrib _Attribs) const override;
			void f_DeleteFile(NStr::CStr const& _File) const override;
			void f_DeleteDirectory(NStr::CStr const& _File) const override;
			void f_RenameFile(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo) const override;
			void f_CreateDirectory(NStr::CStr const& _Path) const override;
			NContainer::TCVector<NStr::CStr> f_FindFiles(NStr::CStr const& _FindPath, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File, bint _bRecursive = false, bool _bFollowLinks = true) const override;

			void f_SetCreationTime(NStr::CStr const& _File, const NTime::CTime &_Time) override;
			void f_SetAccessTime(NStr::CStr const& _File, const NTime::CTime &_Time) override;
			void f_SetWriteTime(NStr::CStr const& _File, const NTime::CTime &_Time) override;
			NTime::CTime f_GetCreationTime(NStr::CStr const& _File) const override;
			NTime::CTime f_GetAccessTime(NStr::CStr const& _File) const override;
			NTime::CTime f_GetWriteTime(NStr::CStr const& _File) const override;

			void f_CreateSymbolicLink(NMib::NStr::CStr const &_FileFrom, NMib::NStr::CStr const &_FileTo, NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags) const override;
			void f_CreateHardLink(NMib::NStr::CStr const &_FileFrom, NMib::NStr::CStr const &_FileTo) const override;
			NMib::NStr::CStr f_ResolveSymbolicLink(NMib::NStr::CStr const &_FileFrom) const override;
			bool f_CanCreateSymbolicLink(NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags) const override;

			bint f_FileExists(NStr::CStr const& _File, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File) const override;
			bint f_MakeFileWritable(NStr::CStr const& _LocalPath, bint _bWritable) const override;
			bint f_IsFileWritable(NStr::CStr const& _LocalPath) const override;
			void f_DeleteDirectoryRecursive(NStr::CStr const& _File, bint _bRemoveWriteProtection = false) const override;
			void f_CopyFile(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo, CFileProgress & _Progress) const override;
			void f_CopyFile(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo) const override;
			void f_CopyFileRaw(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo) const override;
			void f_CopyFileRaw(NStr::CStr const& _FileFrom, ICFileSystemInterface & _ToFS, NStr::CStr const& _FileTo) const override;
			void f_WriteFile(NContainer::TCVector<uint8> const& _FileFrom, NStr::CStr const& _FileTo) const override;
			NContainer::TCVector<uint8> f_ReadFile(NStr::CStr const& _FileFrom) const override;
			void f_CopyFiles(NStr::CStr const& _FindPath, NStr::CStr const& _ToPath, bint _bRecursive = true, bint _bRaw = false, NFile::EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File) const override;
			bint f_CopyFileDiff(NContainer::TCVector<uint8> const& _SourceData, NStr::CStr const& _ToFileName, NTime::CTime const& _FileTime, EFileAttrib _AddAttribs = EFileAttrib_None) const override;
			bint f_FileIsSame(NContainer::TCVector<uint8> const& _SourceData, NStr::CStr const& _ToFileName) const override;
			bint f_CopyFileDiff(NStr::CStr const& _FromFileName, NStr::CStr const& _ToFileName, bint _bCopyDate) const override;
			void f_RenameFile(NStr::CStr const& _FileFrom, NStr::CStr const& _FileTo, CFileProgress & _Progress) const override;
			NMib::NStream::CFilePos f_GetFreeSpace(NMib::NStr::CStr const& _Path) const override;
			NMib::NStream::CFilePos f_GetUsedSpace(NMib::NStr::CStr const& _Path) const override;
			CMibFilePos f_GetFileSize(NStr::CStr const& _Path) const override;
			NDataProcessing::CHashDigest_MD5 f_GetFileChecksum(NStr::CStr const& _Path) const override;
			void f_WriteStringToFile(NStr::CStr const& _Path, NStr::CStr const& _ToWrite, bool _bAddBOM) const override;
			NStr::CStr f_ReadStringFromFile(NStr::CStr const& _Path, bool _bAssumeUTF8 = false) const override;
		};
	}
}
