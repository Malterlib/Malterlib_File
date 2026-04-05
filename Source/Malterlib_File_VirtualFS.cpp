// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Malterlib_File_VirtualFS.h"

namespace NMib::NFile
{
	///
	/// ICFileSystemInterface
	/// =====================

	void ICFileSystemInterface::f_CreateSymbolicLink(NMib::NStr::CStr const &_FileFrom, NMib::NStr::CStr const &_FileTo, NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags) const
	{
		DMibError("Not implemented");
	}

	void ICFileSystemInterface::f_CreateHardLink(NMib::NStr::CStr const &_FileFrom, NMib::NStr::CStr const &_FileTo) const
	{
		DMibError("Not implemented");
	}

	NMib::NStr::CStr ICFileSystemInterface::f_ResolveSymbolicLink(NMib::NStr::CStr const &_FileFrom) const
	{
		DMibError("Not implemented");
		return NMib::NStr::CStr();
	}

	bool ICFileSystemInterface::f_CanCreateSymbolicLink(NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags) const
	{
		return false;
	}

	NMib::NStream::CFilePos ICFileSystemInterface::f_GetFreeSpace(const NMib::NStr::CStr &_Path) const
	{
		return 0;
	}
	NMib::NStream::CFilePos ICFileSystemInterface::f_GetUsedSpace(const NMib::NStr::CStr &_Path) const
	{
		return 0;
	}

	void ICFileSystemInterface::fpr_DeleteFilesInDirs(const NStr::CStr &_File, bool _bRemoveWriteProtection, NStr::CStr &_Errors) const
	{
		NContainer::TCVector<NStr::CStr> Files;
		Files = f_FindFiles(CFile::fs_AppendPath(_File, "*"), EFileAttrib_File | EFileAttrib_Directory, false, false);
		// First recurse down
		for (auto iFile = Files.f_GetIterator(); iFile; ++iFile)
		{
			auto Attribs = f_GetAttributes(*iFile);
			if (!(Attribs & EFileAttrib_Directory) || (Attribs & NFile::EFileAttrib_Link))
			{
				try
				{
					if (_bRemoveWriteProtection && (Attribs & EFileAttrib_ReadOnly))
						f_MakeFileWritable(*iFile);
					f_DeleteFile(*iFile);
				}
				catch (CExceptionFile const &_Exception)
				{
					NStr::fg_AddStrSep(_Errors, _Exception.f_GetErrorStr(), DMibNewLine);
				}
			}
			else if (Attribs & EFileAttrib_Directory)
			{
				fpr_DeleteFilesInDirs(*iFile, _bRemoveWriteProtection, _Errors);
			}
		}
		try
		{
			f_DeleteDirectory(_File);
		}
		catch (CExceptionFile const &_Exception)
		{
			NStr::fg_AddStrSep(_Errors, _Exception.f_GetErrorStr(), DMibNewLine);
		}
	}

	ICFileSystemInterface::~ICFileSystemInterface()
	{
	}

	bool ICFileSystemInterface::f_IsValid() const
	{
		return true;
	}

	// Optional interface
	bool ICFileSystemInterface::f_MakeFileWritable(const NStr::CStr &_LocalPath, bool _bWritable) const
	{
		return true;
	};
	bool ICFileSystemInterface::f_IsFileWritable(const NStr::CStr &_LocalPath) const
	{
		return true;
	};


	NContainer::TCVector<NStr::CStr> ICFileSystemInterface::f_FindFilesNoRoot(const NStr::CStr &_FindPath, NFile::EFileAttrib _AttribMask, bool _bRecursive, bool _bFollowLinks) const
	{
		NStr::CStr RootPath = CFile::fs_GetPath(_FindPath);

		NContainer::TCVector<NStr::CStr> Files = f_FindFiles(_FindPath, _AttribMask, _bRecursive, _bFollowLinks);

		NContainer::TCVector<NStr::CStr> RetFiles;

		for (auto iFile = Files.f_GetIterator(); iFile; ++iFile)
		{
			RetFiles.f_Insert(CFile::fs_MakePathRelative(*iFile, RootPath));
		}
		return RetFiles;
	}

	void ICFileSystemInterface::f_DeleteDirectoryRecursive(const NStr::CStr &_File, bool _bRemoveWriteProtection) const
	{
		NStr::CStr Errors;
		fpr_DeleteFilesInDirs(_File, _bRemoveWriteProtection, Errors);

		if (!Errors.f_IsEmpty())
			DMibErrorFile(Errors);
	}
	void ICFileSystemInterface::f_CopyFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress) const
	{
		return f_CopyFile(_FileFrom, _FileTo);
	}
	void ICFileSystemInterface::f_CopyFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo) const
	{
		return f_CopyFileRaw(_FileFrom, _FileTo);
	}
	void ICFileSystemInterface::f_CopyFileRaw(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo) const
	{
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pInFile = f_OpenStream(_FileFrom, EFileOpen_Read | EFileOpen_ShareAll);
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pOutFile = f_OpenStream(_FileTo, EFileOpen_Write | EFileOpen_ShareAll);

		CFileIoTempBuffer Buffer;

		NStream::CFilePos Len = pInFile->f_GetLength();
		while (Len)
		{
			auto BufferResult = Buffer.f_UseBuffer(Len);
			pInFile->f_ConsumeBytes(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			pOutFile->f_FeedBytes(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			Len -= BufferResult.m_nBytes;
		}

	}
	void ICFileSystemInterface::f_CopyFileRaw(const NStr::CStr &_FileFrom, ICFileSystemInterface &_ToFS, const NStr::CStr &_FileTo) const
	{
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pInFile = f_OpenStream(_FileFrom, EFileOpen_Read | EFileOpen_ShareAll);
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pOutFile = _ToFS.f_OpenStream(_FileTo, EFileOpen_Write | EFileOpen_ShareAll);
		NStream::CFilePos Len = pInFile->f_GetLength();

		CFileIoTempBuffer Buffer;

		while (Len)
		{
			auto BufferResult = Buffer.f_UseBuffer(Len);
			pInFile->f_ConsumeBytes(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			pOutFile->f_FeedBytes(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			Len -= BufferResult.m_nBytes;
		}

	}
	NContainer::CByteVector ICFileSystemInterface::f_ReadFile(const NStr::CStr &_FileFrom) const
	{
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pSource = f_OpenStream(_FileFrom, EFileOpen_Read | EFileOpen_ShareAll);
		NContainer::CByteVector Dest;
		auto Length = pSource->f_GetLength();
		Dest.f_SetLen(Length);
		pSource->f_ConsumeBytes(Dest.f_GetArray(), Length);

		return fg_Move(Dest);
	}

	void ICFileSystemInterface::f_WriteFile(NContainer::CByteVector const &_FileFrom, const NStr::CStr &_FileTo) const
	{
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pOutFile = f_OpenStream(_FileTo, EFileOpen_Write | EFileOpen_ShareAll);
		pOutFile->f_FeedBytes(_FileFrom.f_GetArray(), _FileFrom.f_GetLen());
	}
	void ICFileSystemInterface::f_CopyFiles(const NStr::CStr &_FindPath, const NStr::CStr &_ToPath, bool _bRecursive, bool _bRaw, NFile::EFileAttrib _AttribMask) const
	{
		NContainer::TCVector<NStr::CStr> Paths;
		if (_AttribMask & EFileAttrib_Directory)
			Paths = f_FindFiles(_FindPath, EFileAttrib_Directory, _bRecursive);

		NContainer::TCVector<NStr::CStr> Files;
		if (_AttribMask & EFileAttrib_File)
			Files = f_FindFiles(_FindPath, EFileAttrib_File, _bRecursive);

		umint nPaths = Paths.f_GetLen();

		NStr::CStr RootPath = CFile::fs_GetPath(_FindPath);
		if (!RootPath.f_IsEmpty())
			RootPath += "/";

		umint RootPathLen = RootPath.f_GetLen();
		NStr::CStr ToPath = _ToPath;
		if (!ToPath.f_IsEmpty())
			ToPath += "/";

		for (umint i = 0; i < nPaths; ++i)
		{
			NStr::CStr Path = Paths[i].f_Extract(RootPathLen);
			f_CreateDirectory(ToPath + Path);
		}

		umint nFiles = Files.f_GetLen();
		for (umint i = 0; i < nFiles; ++i)
		{
			NStr::CStr Path = Files[i].f_Extract(RootPathLen);
			NStr::CStr ToFile = ToPath + Path;
			f_CreateDirectory(CFile::fs_GetPath(ToFile));

			if (_bRaw)
				f_CopyFileRaw(Files[i], ToFile);
			else
				f_CopyFile(Files[i], ToFile);
		}
	}

	bool ICFileSystemInterface::f_FileIsSame(const NContainer::CByteVector &_SourceData, const NStr::CStr &_ToFileName) const
	{
		bool bChanged = false;

		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pToFile;

		int32 nTimes = 20 * 30; // 30 seconds
		while (1)
		{
			try
			{
				if (!f_FileExists(_ToFileName))
					return false;
				pToFile = f_OpenStream(_ToFileName, EFileOpen_Read | EFileOpen_ShareAll);
				break;
			}
			catch (CExceptionFile)
			{
				if (--nTimes == 0)
					throw;
				NSys::fg_Thread_Sleep(NMisc::fg_GetRandomFloat() * 0.100);
			}

		}
		NStream::CFilePos FileLen = pToFile->f_GetLength();
		if (NStream::CFilePos(_SourceData.f_GetLen()) != FileLen)
			bChanged = true;
		if (!bChanged)
		{
			NContainer::CByteVector DestData;
			DestData.f_SetLen(FileLen);
			pToFile->f_ConsumeBytes(DestData.f_GetArray(), FileLen);

			if (DestData != _SourceData)
				bChanged = true;
		}

		if (bChanged)
			return false;
		return true;
	}

	bool ICFileSystemInterface::f_CopyFileDiff(const NContainer::CByteVector &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs) const
	{
		if (f_FileIsSame(_SourceData, _ToFileName))
			return false;
		bool bChanged = false;

		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pToFile;

		int32 nTimes = 20 * 30; // 30 seconds
		while (1)
		{
			try
			{
				pToFile = f_OpenStream(_ToFileName, EFileOpen_Read | EFileOpen_DontTruncate | EFileOpen_Write | EFileOpen_ShareRead);
				break;
			}
			catch (CExceptionFile)
			{
				if (--nTimes == 0)
					throw;
				NSys::fg_Thread_Sleep(NMisc::fg_GetRandomFloat() * 0.100);
			}

		}
		NStream::CFilePos FileLen = pToFile->f_GetLength();
		if (NStream::CFilePos(_SourceData.f_GetLen()) != FileLen)
			bChanged = true;
		if (!bChanged)
		{
			NContainer::CByteVector DestData;
			DestData.f_SetLen(FileLen);
			pToFile->f_ConsumeBytes(DestData.f_GetArray(), FileLen);

			if (DestData != _SourceData)
				bChanged = true;
		}

		if (bChanged)
		{
			NStream::CFilePos SourceLen = _SourceData.f_GetLen();
			pToFile->f_SetPosition(0);
			pToFile->f_SetLength(SourceLen);
			pToFile->f_FeedBytes(_SourceData.f_GetArray(), SourceLen);
			//pToFile->f_SetWriteTime(_FileTime);
			return true;
		}
		return false;
	}
	bool ICFileSystemInterface::f_CopyFileDiff(const NStr::CStr &_FromFileName, const NStr::CStr &_ToFileName, bool _bCopyDate) const
	{
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pInFile = f_OpenStream(_FromFileName, EFileOpen_Read | EFileOpen_ShareAll);

		NContainer::CByteVector SourceData;
		umint FileLen = pInFile->f_GetLength();
		SourceData.f_SetLen(FileLen);
		pInFile->f_ConsumeBytes(SourceData.f_GetArray(), FileLen);
//				if (_bCopyDate)
//					return f_CopyFileDiff(SourceData, _ToFileName, File.f_GetWriteTime());
//				else
			return f_CopyFileDiff(SourceData, _ToFileName, NTime::CTime::fs_NowUTC());

	}
	void ICFileSystemInterface::f_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress) const
	{
		return f_RenameFile(_FileFrom, _FileTo);
	}

	CMibFilePos ICFileSystemInterface::f_GetFileSize(const NStr::CStr &_Path) const
	{
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pInFile = f_OpenStream(_Path, EFileOpen_Read | EFileOpen_ShareAll);
		return pInFile->f_GetLength();
	}
	NCryptography::CHashDigest_MD5 ICFileSystemInterface::f_GetFileChecksum(const NStr::CStr &_Path) const
	{
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pInFile = f_OpenStream(_Path, EFileOpen_Read | EFileOpen_ShareAll);

		NCryptography::CHash_MD5 Checksum;
		CMibFilePos Length = pInFile->f_GetLength();

		CFileIoTempBuffer Buffer;

		while (Length)
		{
			auto BufferResult = Buffer.f_UseBuffer(Length);
			pInFile->f_ConsumeBytes(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			Checksum.f_AddData(BufferResult.m_pBuffer, BufferResult.m_nBytes);

			Length -= BufferResult.m_nBytes;
		}

		return Checksum;
	}

	void ICFileSystemInterface::f_WriteStringToFile(const NStr::CStr &_Path, const NStr::CStr &_ToWrite, bool _bAddBOM) const
	{
		uint8 BOM[] = {0xEF, 0xBB, 0xBF};
		NStr::CStr UTF8 = _ToWrite;
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pFile = f_OpenStream(_Path, EFileOpen_Write | EFileOpen_ShareAll);
		if (_bAddBOM)
			pFile->f_FeedBytes(BOM, 3);
		pFile->f_FeedBytes(UTF8.f_GetStr(), UTF8.f_GetLen());
	}

	NStr::CStr ICFileSystemInterface::f_ReadStringFromFile(const NStr::CStr &_Path, bool _bAssumeUTF8) const
	{
		NStr::CStr FileData;
		NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pFile = f_OpenStream(_Path, EFileOpen_Read|EFileOpen_ShareAll);
		FileData = NStr::fg_ReadTextStream(*pFile, _bAssumeUTF8);
		return FileData;
	}

	void ICFileSystemInterface::f_CopyFiles(const NStr::CStr &_FindPath, ICFileSystemInterface &_ToFS, const NStr::CStr &_ToPath, bool _bRecursive, NFile::EFileAttrib _AttribMask) const
	{
		NContainer::TCVector<NStr::CStr> Paths;
		if (_AttribMask & EFileAttrib_Directory)
			Paths = f_FindFiles(_FindPath, EFileAttrib_Directory, _bRecursive);

		NContainer::TCVector<NStr::CStr> Files;
		if (_AttribMask & EFileAttrib_File)
			Files = f_FindFiles(_FindPath, EFileAttrib_File, _bRecursive);

		umint nPaths = Paths.f_GetLen();

		NStr::CStr RootPath = CFile::fs_GetPath(_FindPath);
		if (!RootPath.f_IsEmpty())
			RootPath += "/";
		umint RootPathLen = RootPath.f_GetLen();

		for (umint i = 0; i < nPaths; ++i)
		{
			NStr::CStr Path = Paths[i].f_Extract(RootPathLen);
			_ToFS.f_CreateDirectory(NFile::CFile::fs_AppendPath(_ToPath, Path));
		}

		umint nFiles = Files.f_GetLen();
		for (umint i = 0; i < nFiles; ++i)
		{
			NStr::CStr Path = Files[i].f_Extract(RootPathLen);
			NStr::CStr ToFile = NFile::CFile::fs_AppendPath(_ToPath, Path);
			NStr::CStr Dir = CFile::fs_GetPath(ToFile);
			if (!Dir.f_IsEmpty())
				_ToFS.f_CreateDirectory(Dir);

			f_CopyFileRaw(Files[i], _ToFS, ToFile);
		}
	}

	void ICFileSystemInterface::f_CopyFilesWithAttribs(const NStr::CStr &_FindPath, ICFileSystemInterface &_ToFS, const NStr::CStr &_ToPath, bool _bRecursive, NFile::EFileAttrib _AttribMask) const
	{
		NContainer::TCVector<NStr::CStr> Paths;
		if (_AttribMask & EFileAttrib_Directory)
			Paths = f_FindFiles(_FindPath, EFileAttrib_Directory, _bRecursive, false);

		NContainer::TCVector<NStr::CStr> Files;
		if (_AttribMask & EFileAttrib_File)
			Files = f_FindFiles(_FindPath, EFileAttrib_File, _bRecursive, false);

		umint nPaths = Paths.f_GetLen();

		NStr::CStr RootPath = CFile::fs_GetPath(_FindPath);
		if (!RootPath.f_IsEmpty())
			RootPath += "/";
		umint RootPathLen = RootPath.f_GetLen();

		for (umint i = 0; i < nPaths; ++i)
		{
			NStr::CStr Path = Paths[i].f_Extract(RootPathLen);
			_ToFS.f_CreateDirectory(NFile::CFile::fs_AppendPath(_ToPath, Path));
		}

		umint nFiles = Files.f_GetLen();
		for (umint i = 0; i < nFiles; ++i)
		{
			NStr::CStr FromFile = Files[i];
			NStr::CStr Path = FromFile.f_Extract(RootPathLen);
			NStr::CStr ToFile = NFile::CFile::fs_AppendPath(_ToPath, Path);
			NStr::CStr Dir = CFile::fs_GetPath(ToFile);
			if (!Dir.f_IsEmpty())
				_ToFS.f_CreateDirectory(Dir);

			auto Attribs = f_GetAttributes(FromFile);

			if (_ToFS.f_FileExists(ToFile))
			{
				_ToFS.f_MakeFileWritable(ToFile);
				_ToFS.f_DeleteFile(ToFile);
			}

			if (Attribs & EFileAttrib_Link)
			{
				NStr::CStr Link = f_ResolveSymbolicLink(FromFile);
				_ToFS.f_CreateSymbolicLink(Link, ToFile, Attribs, NFile::ESymbolicLinkFlag_None);
			}
			else
			{
				f_CopyFileRaw(FromFile, _ToFS, ToFile);
				_ToFS.f_SetAttributes(ToFile, Attribs);
			}
		}
	}

	bool gfp_CopyFiles_Recursive
		(
			ICFileSystemInterface &_DestFS
			, ICFileSystemInterface &_SourceFS
			, NStr::CStr _Path
			, ICFileSystemInterface::ECopyFilesBackupFlag _Flags
			, NFunction::TCFunction<bool (NStr::CStr const& _Filename)> const& _fShouldBackup
			, ICFileSystemInterface::CCopyFilesProgress * _pProgress
			, NContainer::TCMap<NStr::CStr, NTime::CTime> * _pSourceCleared
		)
	{
		using namespace  NContainer;
		using namespace NStr;

		auto ToFind = CFile::fs_AppendPath(_Path, "*");
		TCVector<CStr> Files = _SourceFS.f_FindFilesNoRoot(ToFind);

		umint nFiles = Files.f_GetLen();

		for (umint i = 0; i < nFiles; ++i)
		{
			CStr FileName;
			FileName = CFile::fs_AppendPath(_Path, Files[i]);

			if (_fShouldBackup && !_fShouldBackup(FileName))
				continue;

			auto SourceAttribs = _SourceFS.f_GetAttributes(FileName);

			if (SourceAttribs & EFileAttrib_Directory)
			{
				if (!gfp_CopyFiles_Recursive(_DestFS, _SourceFS, FileName, _Flags, _fShouldBackup, _pProgress, _pSourceCleared))
					return false;
			}
			else
			{
				if (!_DestFS.f_FileExists(FileName))
				{
					// Create containing directory

					if (!(SourceAttribs & EFileAttrib_BackedUp) || !(_Flags & ICFileSystemInterface::ECopyFilesBackupFlag_Incremental))
					{
						CStr PathToCreate = CFile::fs_GetPath(FileName);
						if (!PathToCreate.f_IsEmpty())
							_DestFS.f_CreateDirectory(PathToCreate);

						{
							CFileIoTempBuffer Buffer;

							auto pSource = _SourceFS.f_OpenStream(FileName, EFileOpen_Read);
							auto & Source = *pSource;
							auto pDestStream = _DestFS.f_OpenStream(FileName, EFileOpen_Write);
							CMibFilePos Size = Source.f_GetLength();
							while (Size)
							{
								auto BufferResult = Buffer.f_UseBuffer(Size);
								Source.f_ConsumeBytes(BufferResult.m_pBuffer, BufferResult.m_nBytes);
								pDestStream->f_FeedBytes(BufferResult.m_pBuffer, BufferResult.m_nBytes);
								Size -= BufferResult.m_nBytes;

								if (_pProgress && !_pProgress->f_IncreaseProgress(BufferResult.m_nBytes))
									return false;
							}
						}

						if (_Flags & ICFileSystemInterface::ECopyFilesBackupFlag_MarkDestBackedUp)
							SourceAttribs = SourceAttribs | EFileAttrib_BackedUp;
						else if (_Flags & ICFileSystemInterface::ECopyFilesBackupFlag_ClearDestBackedUp)
							SourceAttribs = SourceAttribs & ~EFileAttrib_BackedUp;

						_DestFS.f_SetAttributes(FileName, SourceAttribs);
						NTime::CTime Temp = _SourceFS.f_GetCreationTime(FileName);
						_DestFS.f_SetCreationTime(FileName, Temp);
						Temp = _SourceFS.f_GetAccessTime(FileName);
						_DestFS.f_SetAccessTime(FileName, Temp);
						NTime::CTime WriteTime = _SourceFS.f_GetWriteTime(FileName);
						_DestFS.f_SetWriteTime(FileName, WriteTime);

						if (_pSourceCleared)
							(*_pSourceCleared)[FileName] = WriteTime;
					}
					else if (_pProgress && !_pProgress->f_IncreaseProgress(_SourceFS.f_GetFileSize(FileName)))
						return false;
				}
				else if (_pProgress && !_pProgress->f_IncreaseProgress(_SourceFS.f_GetFileSize(FileName)))
						 return false;
			}
		}

		return true;
	}

	bool ICFileSystemInterface::f_CopyFiles_Backup
		(
			NStr::CStr const& _Path
			, ICFileSystemInterface & _ToFS
			, ECopyFilesBackupFlag _Flags
			, NFunction::TCFunction<bool (NStr::CStr const& _Filename)> const& _fShouldBackup
			, CCopyFilesProgress * _pProgress
			, NContainer::TCMap<NStr::CStr, NTime::CTime> * _pSourceCleared
		)
	{
		DMibRequire(!!(_Flags & ECopyFilesBackupFlag_MarkDestBackedUp) != !!(_Flags & ECopyFilesBackupFlag_ClearDestBackedUp));

		if (_pProgress)
			_pProgress->f_StepDown(f_GetUsedSpace(""));

		bool bCompleted = gfp_CopyFiles_Recursive(_ToFS, *this, _Path, _Flags, _fShouldBackup, _pProgress, _pSourceCleared);

		if (_pProgress)
			_pProgress->f_StepUp();

		return bCompleted;
	}

	///
	/// CFileSystemInterface_Disk
	/// =========================

	NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> CFileSystemInterface_Disk::f_OpenStream(const NStr::CStr &_FileName, NMib::NFile::EFileOpen _OpenFlags) const
	{
		NStorage::TCSharedPointer<NFile::TCBinaryStreamFile<NStream::CBinaryStreamDefaultRef>> pRet(fg_Construct());
		pRet->f_Open(_FileName, _OpenFlags);
		return pRet;
	}

	EFileAttrib CFileSystemInterface_Disk::f_GetAttributes(const NStr::CStr &_File) const
	{
		NFile::CFile File;
		File.f_Open(_File, EFileOpen_ShareAll | EFileOpen_ReadAttribs);
		return File.f_GetAttributes();
	}

	void CFileSystemInterface_Disk::f_SetAttributes(const NStr::CStr &_File, EFileAttrib _Attribs) const
	{
		NFile::CFile::fs_SetAttributes(_File, _Attribs);
	}

	void CFileSystemInterface_Disk::f_SetCreationTime(NStr::CStr const& _File, const NTime::CTime &_Time)
	{
		EFileOpen OpenFlags = EFileOpen_ShareAll | EFileOpen_WriteAttribs;
		if (CFile::fs_FileExists(_File, EFileAttrib_Directory))
			OpenFlags |= EFileOpen_Directory;
		NFile::CFile File;
		File.f_Open(_File, OpenFlags);
		File.f_SetCreationTime(_Time);
	}

	void CFileSystemInterface_Disk::f_SetAccessTime(NStr::CStr const& _File, const NTime::CTime &_Time)
	{
		EFileOpen OpenFlags = EFileOpen_ShareAll | EFileOpen_WriteAttribs;
		if (CFile::fs_FileExists(_File, EFileAttrib_Directory))
			OpenFlags |= EFileOpen_Directory;
		NFile::CFile File;
		File.f_Open(_File, OpenFlags);
		File.f_SetAccessTime(_Time);
	}

	void CFileSystemInterface_Disk::f_SetWriteTime(NStr::CStr const& _File, const NTime::CTime &_Time)
	{
		EFileOpen OpenFlags = EFileOpen_ShareAll | EFileOpen_WriteAttribs;
		if (CFile::fs_FileExists(_File, EFileAttrib_Directory))
			OpenFlags |= EFileOpen_Directory;
		NFile::CFile File;
		File.f_Open(_File, OpenFlags);
		File.f_SetWriteTime(_Time);
	}

	NTime::CTime CFileSystemInterface_Disk::f_GetCreationTime(NStr::CStr const& _File) const
	{
		return CFile::fs_GetCreationTime(_File);
	}

	NTime::CTime CFileSystemInterface_Disk::f_GetAccessTime(NStr::CStr const& _File) const
	{
		return CFile::fs_GetAccessTime(_File);
	}

	NTime::CTime CFileSystemInterface_Disk::f_GetWriteTime(NStr::CStr const& _File) const
	{
		return CFile::fs_GetWriteTime(_File);
	}

	void CFileSystemInterface_Disk::f_CreateSymbolicLink(NMib::NStr::CStr const &_FileFrom, NMib::NStr::CStr const &_FileTo, NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags) const
	{
		CFile::fs_CreateSymbolicLink(_FileFrom, _FileTo, _Type, _Flags);
	}

	void CFileSystemInterface_Disk::f_CreateHardLink(NMib::NStr::CStr const &_FileFrom, NMib::NStr::CStr const &_FileTo) const
	{
		CFile::fs_CreateHardLink(_FileFrom, _FileTo);
	}

	NMib::NStr::CStr CFileSystemInterface_Disk::f_ResolveSymbolicLink(NMib::NStr::CStr const &_FileFrom) const
	{
		return CFile::fs_ResolveSymbolicLink(_FileFrom);
	}

	bool CFileSystemInterface_Disk::f_CanCreateSymbolicLink(NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags) const
	{
		return CFile::fs_CanCreateSymbolicLink(_Type, _Flags);
	}

	NMib::NStream::CFilePos CFileSystemInterface_Disk::f_GetFreeSpace(NMib::NStr::CStr const& _Path) const
	{
		return NFile::CFile::fs_GetFreeSpace(_Path);
	}

	NMib::NStream::CFilePos CFileSystemInterface_Disk::f_GetUsedSpace(NMib::NStr::CStr const& _Path) const
	{
		return NFile::CFile::fs_GetUsedSpace(_Path);
	}

	void CFileSystemInterface_Disk::f_DeleteFile(const NStr::CStr &_File) const
	{
		return NFile::CFile::fs_DeleteFile(_File);
	}
	void CFileSystemInterface_Disk::f_DeleteDirectory(const NStr::CStr &_File) const
	{
		return NFile::CFile::fs_DeleteDirectory(_File);
	}
	void CFileSystemInterface_Disk::f_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo) const
	{
		return NFile::CFile::fs_RenameFile(_FileFrom, _FileTo);
	}
	void CFileSystemInterface_Disk::f_CreateDirectory(const NStr::CStr &_Path) const
	{
		return NFile::CFile::fs_CreateDirectory(_Path);
	}
	NContainer::TCVector<NStr::CStr> CFileSystemInterface_Disk::f_FindFiles(const NStr::CStr &_FindPath, NFile::EFileAttrib _AttribMask, bool _bRecursive, bool _bFollowLinks) const
	{
		return NFile::CFile::fs_FindFiles(_FindPath, _AttribMask, _bRecursive, _bFollowLinks);
	}
	bool CFileSystemInterface_Disk::f_FileExists(const NStr::CStr &_File, NFile::EFileAttrib _AttribMask) const
	{
		return NFile::CFile::fs_FileExists(_File, _AttribMask);
	}
	bool CFileSystemInterface_Disk::f_MakeFileWritable(const NStr::CStr &_LocalPath, bool _bWritable) const
	{
		return NFile::CFile::fs_MakeFileWritable(_LocalPath);
	}
	bool CFileSystemInterface_Disk::f_IsFileWritable(const NStr::CStr &_LocalPath) const
	{
		return NFile::CFile::fs_IsFileWritable(_LocalPath);
	}
	void CFileSystemInterface_Disk::f_DeleteDirectoryRecursive(const NStr::CStr &_File, bool _bRemoveWriteProtection) const
	{
		return NFile::CFile::fs_DeleteDirectoryRecursive(_File, _bRemoveWriteProtection);
	}
	void CFileSystemInterface_Disk::f_CopyFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress) const
	{
		return NFile::CFile::fs_CopyFile(_FileFrom, _FileTo, _Progress);
	}
	void CFileSystemInterface_Disk::f_CopyFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo) const
	{
		return NFile::CFile::fs_CopyFile(_FileFrom, _FileTo);
	}
	void CFileSystemInterface_Disk::f_CopyFileRaw(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo) const
	{
		return NFile::CFile::fs_CopyFileRaw(_FileFrom, _FileTo);
	}
	void CFileSystemInterface_Disk::f_CopyFileRaw(const NStr::CStr &_FileFrom, ICFileSystemInterface &_ToFS, const NStr::CStr &_FileTo) const
	{
		return ICFileSystemInterface::f_CopyFileRaw(_FileFrom, _ToFS, _FileTo);
	}
	NContainer::CByteVector CFileSystemInterface_Disk::f_ReadFile(const NStr::CStr &_FileFrom) const
	{
		return NFile::CFile::fs_ReadFile(_FileFrom);
	}

	void CFileSystemInterface_Disk::f_WriteFile(NContainer::CByteVector const &_FileFrom, const NStr::CStr &_FileTo) const
	{
		return NFile::CFile::fs_WriteFile(_FileFrom, _FileTo);
	}
	void CFileSystemInterface_Disk::f_CopyFiles(const NStr::CStr &_FindPath, const NStr::CStr &_ToPath, bool _bRecursive, bool _bRaw, NFile::EFileAttrib _AttribMask) const
	{
		return NFile::CFile::fs_CopyFiles(_FindPath, _ToPath, _bRecursive, _bRaw, _AttribMask);
	}
	bool CFileSystemInterface_Disk::f_CopyFileDiff(const NContainer::CByteVector &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs) const
	{
		return NFile::CFile::fs_CopyFileDiff(_SourceData, _ToFileName, _FileTime, _AddAttribs);
	}
	bool CFileSystemInterface_Disk::f_FileIsSame(const NContainer::CByteVector &_SourceData, const NStr::CStr &_ToFileName) const
	{
		return NFile::CFile::fs_FileIsSame(_SourceData, _ToFileName);
	}
	bool CFileSystemInterface_Disk::f_CopyFileDiff(const NStr::CStr &_FromFileName, const NStr::CStr &_ToFileName, bool _bCopyDate) const
	{
		return NFile::CFile::fs_CopyFileDiff(_FromFileName, _ToFileName, _bCopyDate);
	}
	void CFileSystemInterface_Disk::f_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress) const
	{
		return NFile::CFile::fs_RenameFile(_FileFrom, _FileTo, _Progress);
	}
	CMibFilePos CFileSystemInterface_Disk::f_GetFileSize(const NStr::CStr &_Path) const
	{
		return NFile::CFile::fs_GetFileSize(_Path);
	}
	NCryptography::CHashDigest_MD5 CFileSystemInterface_Disk::f_GetFileChecksum(const NStr::CStr &_Path) const
	{
		return NFile::CFile::fs_GetFileChecksum(_Path);
	}
	void CFileSystemInterface_Disk::f_WriteStringToFile(const NStr::CStr &_Path, const NStr::CStr &_ToWrite, bool _bAddBOM) const
	{
		return NFile::CFile::fs_WriteStringToFile(_Path, _ToWrite, _bAddBOM);
	}
	NStr::CStr CFileSystemInterface_Disk::f_ReadStringFromFile(const NStr::CStr &_Path, bool _bAssumeUTF8) const
	{
		return NFile::CFile::fs_ReadStringFromFile(_Path, _bAssumeUTF8);
	}
}
