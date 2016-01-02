// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_VirtualFS.h"

namespace NMib
{
	namespace NFile
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
		bint ICFileSystemInterface::f_MakeFileWritable(const NStr::CStr &_LocalPath, bint _bWritable) const 
		{
			return true;
		};
		bint ICFileSystemInterface::f_IsFileWritable(const NStr::CStr &_LocalPath) const
		{ 
			return true;
		};
		
		
		NContainer::TCVector<NStr::CStr> ICFileSystemInterface::f_FindFilesNoRoot(const NStr::CStr &_FindPath, NFile::EFileAttrib _AttribMask, bint _bRecursive, bool _bFollowLinks) const
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
		
		void ICFileSystemInterface::f_DeleteDirectoryRecursive(const NStr::CStr &_File, bint _bRemoveWriteProtection) const
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
			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pInFile = f_OpenStream(_FileFrom, EFileOpen_Read | EFileOpen_ShareAll);
			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pOutFile = f_OpenStream(_FileTo, EFileOpen_Write | EFileOpen_ShareAll);
			NStream::CFilePos Len = pInFile->f_GetLength();
			while (Len)
			{
				mint ThisTime = fg_Min(Len, 32768);
				ch8 Temp[32768];
				pInFile->f_ConsumeBytes(Temp, ThisTime);
				pOutFile->f_FeedBytes(Temp, ThisTime);
				Len -= ThisTime;
			}

		}
		void ICFileSystemInterface::f_CopyFileRaw(const NStr::CStr &_FileFrom, ICFileSystemInterface &_ToFS, const NStr::CStr &_FileTo) const
		{
			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pInFile = f_OpenStream(_FileFrom, EFileOpen_Read | EFileOpen_ShareAll);
			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pOutFile = _ToFS.f_OpenStream(_FileTo, EFileOpen_Write | EFileOpen_ShareAll);
			NStream::CFilePos Len = pInFile->f_GetLength();
			while (Len)
			{
				mint ThisTime = fg_Min(Len, 32768);
				ch8 Temp[32768];
				pInFile->f_ConsumeBytes(Temp, ThisTime);
				pOutFile->f_FeedBytes(Temp, ThisTime);
				Len -= ThisTime;
			}

		}
		NContainer::TCVector<uint8> ICFileSystemInterface::f_ReadFile(const NStr::CStr &_FileFrom) const
		{
			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pSource = f_OpenStream(_FileFrom, EFileOpen_Read | EFileOpen_ShareAll);
			NContainer::TCVector<uint8> Dest;
			auto Length = pSource->f_GetLength();
			Dest.f_SetLen(Length);
			pSource->f_ConsumeBytes(Dest.f_GetArray(), Length);

			return fg_Move(Dest);
		}

		void ICFileSystemInterface::f_WriteFile(NContainer::TCVector<uint8> const &_FileFrom, const NStr::CStr &_FileTo) const
		{
			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pOutFile = f_OpenStream(_FileTo, EFileOpen_Write | EFileOpen_ShareAll);
			pOutFile->f_FeedBytes(_FileFrom.f_GetArray(), _FileFrom.f_GetLen());
		}
		void ICFileSystemInterface::f_CopyFiles(const NStr::CStr &_FindPath, const NStr::CStr &_ToPath, bint _bRecursive, bint _bRaw, NFile::EFileAttrib _AttribMask) const
		{
			NContainer::TCVector<NStr::CStr> Paths;
			if (_AttribMask & EFileAttrib_Directory)
				Paths = f_FindFiles(_FindPath, EFileAttrib_Directory, _bRecursive);

			NContainer::TCVector<NStr::CStr> Files;
			if (_AttribMask & EFileAttrib_File)
				Files = f_FindFiles(_FindPath, EFileAttrib_File, _bRecursive);

			mint nPaths = Paths.f_GetLen();

			NStr::CStr RootPath = CFile::fs_GetPath(_FindPath);
			if (!RootPath.f_IsEmpty())
				RootPath += "/";
				
			mint RootPathLen = RootPath.f_GetLen();
			NStr::CStr ToPath = _ToPath;
			if (!ToPath.f_IsEmpty())
				ToPath += "/";

			for (mint i = 0; i < nPaths; ++i)
			{
				NStr::CStr Path = Paths[i].f_Extract(RootPathLen);
				f_CreateDirectory(ToPath + Path);
			}

			mint nFiles = Files.f_GetLen();
			for (mint i = 0; i < nFiles; ++i)
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

		bint ICFileSystemInterface::f_FileIsSame(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName) const
		{
			bint bChanged = false;

			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pToFile;

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
				NContainer::TCVector<uint8> DestData;
				DestData.f_SetLen(FileLen);
				pToFile->f_ConsumeBytes(DestData.f_GetArray(), FileLen);

				if (DestData != _SourceData)
					bChanged = true;
			}

			if (bChanged)
				return false;
			return true;
		}

		bint ICFileSystemInterface::f_CopyFileDiff(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs) const
		{
			if (f_FileIsSame(_SourceData, _ToFileName))
				return false;
			bint bChanged = false;

			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pToFile;

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
				NContainer::TCVector<uint8> DestData;
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
		bint ICFileSystemInterface::f_CopyFileDiff(const NStr::CStr &_FromFileName, const NStr::CStr &_ToFileName, bint _bCopyDate) const
		{
			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pInFile = f_OpenStream(_FromFileName, EFileOpen_Read | EFileOpen_ShareAll);

			NContainer::TCVector<uint8> SourceData;
			mint FileLen = pInFile->f_GetLength();
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
			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pInFile = f_OpenStream(_Path, EFileOpen_Read | EFileOpen_ShareAll);
			return pInFile->f_GetLength();
		}
		NDataProcessing::CHashDigest_MD5 ICFileSystemInterface::f_GetFileChecksum(const NStr::CStr &_Path) const
		{
			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pInFile = f_OpenStream(_Path, EFileOpen_Read | EFileOpen_ShareAll);

			NDataProcessing::CHash_MD5 Checksum;
			CMibFilePos Length = pInFile->f_GetLength();

			while (Length)
			{
				mint ThisTime = mint(fg_Min(Length, CMibFilePos(32768)));
				uint8 Temp[32768];
				pInFile->f_ConsumeBytes(Temp, ThisTime);
				Checksum.f_AddData(Temp, ThisTime);

				Length -= ThisTime;
			}

			return Checksum;
		}

		void ICFileSystemInterface::f_WriteStringToFile(const NStr::CStr &_Path, const NStr::CStr &_ToWrite, bool _bAddBOM) const
		{
			uint8 BOM[] = {0xEF, 0xBB, 0xBF};
			NStr::CStr UTF8 = _ToWrite;
 			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pFile = f_OpenStream(_Path, EFileOpen_Write | EFileOpen_ShareAll);
			if (_bAddBOM)
				pFile->f_FeedBytes(BOM, 3);
			pFile->f_FeedBytes(UTF8.f_GetStr(), UTF8.f_GetLen());
		}

		NStr::CStr ICFileSystemInterface::f_ReadStringFromFile(const NStr::CStr &_Path, bool _bAssumeUTF8) const
		{
			NStr::CStr FileData;
			NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> pFile = f_OpenStream(_Path, EFileOpen_Read|EFileOpen_ShareAll);
			FileData = NStr::fg_ReadTextStream(*pFile, _bAssumeUTF8);
			return FileData;
		}

		void ICFileSystemInterface::f_CopyFiles(const NStr::CStr &_FindPath, ICFileSystemInterface &_ToFS, const NStr::CStr &_ToPath, bint _bRecursive, NFile::EFileAttrib _AttribMask) const
		{
			NContainer::TCVector<NStr::CStr> Paths;
			if (_AttribMask & EFileAttrib_Directory)
				Paths = f_FindFiles(_FindPath, EFileAttrib_Directory, _bRecursive);

			NContainer::TCVector<NStr::CStr> Files;
			if (_AttribMask & EFileAttrib_File)
				Files = f_FindFiles(_FindPath, EFileAttrib_File, _bRecursive);

			mint nPaths = Paths.f_GetLen();

			NStr::CStr RootPath = CFile::fs_GetPath(_FindPath);
			if (!RootPath.f_IsEmpty())
				RootPath += "/";
			mint RootPathLen = RootPath.f_GetLen();

			for (mint i = 0; i < nPaths; ++i)
			{
				NStr::CStr Path = Paths[i].f_Extract(RootPathLen);
				_ToFS.f_CreateDirectory(NFile::CFile::fs_AppendPath(_ToPath, Path));
			}

			mint nFiles = Files.f_GetLen();
			for (mint i = 0; i < nFiles; ++i)
			{
				NStr::CStr Path = Files[i].f_Extract(RootPathLen);
				NStr::CStr ToFile = NFile::CFile::fs_AppendPath(_ToPath, Path);
				NStr::CStr Dir = CFile::fs_GetPath(ToFile);
				if (!Dir.f_IsEmpty())
					_ToFS.f_CreateDirectory(Dir);

				f_CopyFileRaw(Files[i], _ToFS, ToFile);
			}
		}

		void ICFileSystemInterface::f_CopyFilesWithAttribs(const NStr::CStr &_FindPath, ICFileSystemInterface &_ToFS, const NStr::CStr &_ToPath, bint _bRecursive, NFile::EFileAttrib _AttribMask) const
		{
			NContainer::TCVector<NStr::CStr> Paths;
			if (_AttribMask & EFileAttrib_Directory)
				Paths = f_FindFiles(_FindPath, EFileAttrib_Directory, _bRecursive, false);

			NContainer::TCVector<NStr::CStr> Files;
			if (_AttribMask & EFileAttrib_File)
				Files = f_FindFiles(_FindPath, EFileAttrib_File, _bRecursive, false);

			mint nPaths = Paths.f_GetLen();

			NStr::CStr RootPath = CFile::fs_GetPath(_FindPath);
			if (!RootPath.f_IsEmpty())
				RootPath += "/";
			mint RootPathLen = RootPath.f_GetLen();

			for (mint i = 0; i < nPaths; ++i)
			{
				NStr::CStr Path = Paths[i].f_Extract(RootPathLen);
				_ToFS.f_CreateDirectory(NFile::CFile::fs_AppendPath(_ToPath, Path));
			}

			mint nFiles = Files.f_GetLen();
			for (mint i = 0; i < nFiles; ++i)
			{
				NStr::CStr FromFile = Files[i];
				NStr::CStr Path = FromFile.f_Extract(RootPathLen);
				NStr::CStr ToFile = NFile::CFile::fs_AppendPath(_ToPath, Path);
				NStr::CStr Dir = CFile::fs_GetPath(ToFile);
				if (!Dir.f_IsEmpty())
					_ToFS.f_CreateDirectory(Dir);
				
				auto Attribs = f_GetAttributes(FromFile);

				if (_ToFS.f_FileExists(ToFile))
					_ToFS.f_DeleteFile(ToFile);
				
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

			mint nFiles = Files.f_GetLen();

			for (mint i = 0; i < nFiles; ++i)
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
								enum {ECopySize = 16384};
								uint8 TempBuffer[ECopySize];
								auto pSource = _SourceFS.f_OpenStream(FileName, EFileOpen_Read);
								auto & Source = *pSource;
								auto pDestStream = _DestFS.f_OpenStream(FileName, EFileOpen_Write);
								CMibFilePos Size = Source.f_GetLength();
								while (Size)
								{
									mint ThisTime = fg_Convert<mint>(fg_Min(Size, CMibFilePos(ECopySize)));
									Source.f_ConsumeBytes(TempBuffer, ThisTime);
									pDestStream->f_FeedBytes(TempBuffer, ThisTime);
									Size -= ThisTime;

									if (_pProgress && !_pProgress->f_IncreaseProgress(ThisTime))
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

		NPtr::TCSharedPointer<NStream::CBinaryStreamDefaultRef> CFileSystemInterface_Disk::f_OpenStream(const NStr::CStr &_FileName, NMib::NFile::EFileOpen _OpenFlags) const
		{
			NPtr::TCSharedPointer<NFile::TCBinaryStreamFile<NStream::CBinaryStreamDefaultRef>> pRet(fg_Construct());
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
			NFile::CFile File;
			File.f_Open(_File, EFileOpen_ShareAll | EFileOpen_WriteAttribs);
			File.f_SetCreationTime(_Time);
		}

		void CFileSystemInterface_Disk::f_SetAccessTime(NStr::CStr const& _File, const NTime::CTime &_Time)
		{
			NFile::CFile File;
			File.f_Open(_File, EFileOpen_ShareAll | EFileOpen_WriteAttribs);
			File.f_SetAccessTime(_Time);
		}

		void CFileSystemInterface_Disk::f_SetWriteTime(NStr::CStr const& _File, const NTime::CTime &_Time)
		{
			NFile::CFile File;
			File.f_Open(_File, EFileOpen_ShareAll | EFileOpen_WriteAttribs);
			File.f_SetWriteTime(_Time);
		}

		NTime::CTime CFileSystemInterface_Disk::f_GetCreationTime(NStr::CStr const& _File) const
		{
			NFile::CFile File;
			File.f_Open(_File, EFileOpen_ShareAll | EFileOpen_ReadAttribs);
			return File.f_GetCreationTime();
		}

		NTime::CTime CFileSystemInterface_Disk::f_GetAccessTime(NStr::CStr const& _File) const
		{
			NFile::CFile File;
			File.f_Open(_File, EFileOpen_ShareAll | EFileOpen_ReadAttribs);
			return File.f_GetAccessTime();
		}

		NTime::CTime CFileSystemInterface_Disk::f_GetWriteTime(NStr::CStr const& _File) const
		{
			NFile::CFile File;
			File.f_Open(_File, EFileOpen_ShareAll | EFileOpen_ReadAttribs);
			return File.f_GetWriteTime();
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
		NContainer::TCVector<NStr::CStr> CFileSystemInterface_Disk::f_FindFiles(const NStr::CStr &_FindPath, NFile::EFileAttrib _AttribMask, bint _bRecursive, bool _bFollowLinks) const
		{
			return NFile::CFile::fs_FindFiles(_FindPath, _AttribMask, _bRecursive, _bFollowLinks);
		}
		bint CFileSystemInterface_Disk::f_FileExists(const NStr::CStr &_File, NFile::EFileAttrib _AttribMask) const
		{
			return NFile::CFile::fs_FileExists(_File, _AttribMask);
		}
		bint CFileSystemInterface_Disk::f_MakeFileWritable(const NStr::CStr &_LocalPath, bint _bWritable) const
		{
			return NFile::CFile::fs_MakeFileWritable(_LocalPath);
		}
		bint CFileSystemInterface_Disk::f_IsFileWritable(const NStr::CStr &_LocalPath) const
		{ 
			return NFile::CFile::fs_IsFileWritable(_LocalPath);
		}
		void CFileSystemInterface_Disk::f_DeleteDirectoryRecursive(const NStr::CStr &_File, bint _bRemoveWriteProtection) const
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
		NContainer::TCVector<uint8> CFileSystemInterface_Disk::f_ReadFile(const NStr::CStr &_FileFrom) const
		{
			return NFile::CFile::fs_ReadFile(_FileFrom);
		}

		void CFileSystemInterface_Disk::f_WriteFile(NContainer::TCVector<uint8> const &_FileFrom, const NStr::CStr &_FileTo) const
		{
			return NFile::CFile::fs_WriteFile(_FileFrom, _FileTo);
		}
		void CFileSystemInterface_Disk::f_CopyFiles(const NStr::CStr &_FindPath, const NStr::CStr &_ToPath, bint _bRecursive, bint _bRaw, NFile::EFileAttrib _AttribMask) const
		{
			return NFile::CFile::fs_CopyFiles(_FindPath, _ToPath, _bRecursive, _bRaw, _AttribMask);
		}
		bint CFileSystemInterface_Disk::f_CopyFileDiff(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs) const
		{
			return NFile::CFile::fs_CopyFileDiff(_SourceData, _ToFileName, _FileTime, _AddAttribs);
		}
		bint CFileSystemInterface_Disk::f_FileIsSame(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName) const
		{
			return NFile::CFile::fs_FileIsSame(_SourceData, _ToFileName);
		}
		bint CFileSystemInterface_Disk::f_CopyFileDiff(const NStr::CStr &_FromFileName, const NStr::CStr &_ToFileName, bint _bCopyDate) const
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
		NDataProcessing::CHashDigest_MD5 CFileSystemInterface_Disk::f_GetFileChecksum(const NStr::CStr &_Path) const
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
		
	};
}
