// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

namespace NMib
{
	namespace NFile
	{
		namespace
		{
			void fg_FindFilesLeaf
				(
					const NStr::CStr &_FindPath
					, NFunction::TCFunctionNoAlloc<bool (NStr::CStr const &_FileName, EFileAttrib _Attribs, bool _bPreRecurse)> const &_fFoundFile
					, bool _bCatchExceptions
					, bool _bIncludeDirectories
				)
			{
				auto fDoFind
					= [&]
					{
						void *pFind = nullptr;
						pFind = NSys::NFile::fg_FindOpen(_FindPath);
						auto Cleanup = g_OnScopeExit > [&]
							{
								NSys::NFile::fg_FindClose(pFind);
							}
						;

						EFileAttrib Attribs;
						const NStr::CStr *pPath = NSys::NFile::fg_FindNext(pFind, Attribs);
						while (pPath)
						{
							if (Attribs & EFileAttrib_Directory)
							{
								 if (_bIncludeDirectories)
								 {
									 _fFoundFile(*pPath, Attribs, true);
									 _fFoundFile(*pPath, Attribs, false);
								 }
							}
							else
								_fFoundFile(*pPath, Attribs, false);
							pPath = NSys::NFile::fg_FindNext(pFind, Attribs);
						}

					}
				;
				if (_bCatchExceptions)
				{
					try
					{
						fDoFind();
					}
					catch (NFile::CExceptionFile const &)
					{
					}
				}
				else
				{
					fDoFind();
				}
			}

			void fg_FindFilesRecurse
				(
					const NStr::CStr &_Path
					, const NStr::CStr &_Pattern
					, NFunction::TCFunctionNoAlloc<bool (NStr::CStr const &_FileName, EFileAttrib _Attribs, bool _bPreRecurse)> const &_fFoundFile
					, bool _bCatchExceptions
				)
			{
				auto fDoFind = [&]()
					{
						{
							NStr::CStr ToFind = _Path + "*";
							void *pFind = nullptr;
							pFind = NSys::NFile::fg_FindOpen(ToFind);
							auto Cleanup = g_OnScopeExit > [&]
								{
									NSys::NFile::fg_FindClose(pFind);
								}
							;
							EFileAttrib Attribs;
							const NStr::CStr *pPath = NSys::NFile::fg_FindNext(pFind, Attribs);

							while (pPath)
							{
								if (Attribs & EFileAttrib_Directory)
								{
									if (_fFoundFile(*pPath, Attribs, true))
									{
										fg_FindFilesRecurse(*pPath + "/", _Pattern, _fFoundFile, _bCatchExceptions);
										_fFoundFile(*pPath, Attribs, false);
									}
								}
								pPath = NSys::NFile::fg_FindNext(pFind, Attribs);
							}
						}

						fg_FindFilesLeaf(_Path + _Pattern, _fFoundFile, false, false);
					}
				;

				if (_bCatchExceptions)
				{
					try
					{
						fDoFind();
					}
					catch (NFile::CExceptionFile const &)
					{
					}
				}
				else
				{
					fDoFind();
				}
			}

			void fg_FindFiles
				(
					const NStr::CStr &_FindPath
					, NFunction::TCFunctionNoAlloc<bool (NStr::CStr const &_FileName, EFileAttrib _Attribs, bool _bPreRecurse)> const &_fFoundFile
					, bool _bCatchExceptions
					, bool _bRecursive
				)
			{
				if (_bRecursive)
				{
					NStr::CStr Whole = _FindPath;
					NStr::CStr BasePath = CFile::fs_GetPath(Whole) + "/";
					NStr::CStr Pattern = Whole.f_Extract(BasePath.f_GetLen());

					fg_FindFilesRecurse
						(
							BasePath
							, Pattern
							, _fFoundFile
							, _bCatchExceptions
						)
					;
				}
				else
					fg_FindFilesLeaf(_FindPath, _fFoundFile, _bCatchExceptions, true);

			}
		}

		void CFile::fs_CopyFiles(const NStr::CStr &_FindPath, const NStr::CStr &_ToPath, bint _bRecursive, bint _bRaw, EFileAttrib _AttribMask)
		{

			NContainer::TCVector<NStr::CStr> Paths;
			if (_AttribMask & EFileAttrib_Directory)
				Paths = fs_FindFiles(_FindPath, EFileAttrib_Directory, _bRecursive);

			NContainer::TCVector<NStr::CStr> Files;
			if (_AttribMask & EFileAttrib_File)
				Files = fs_FindFiles(_FindPath, EFileAttrib_File, _bRecursive);

			mint nPaths = Paths.f_GetLen();

			NStr::CStr RootPath = fs_GetPath(_FindPath) + "/";
			mint RootPathLen = RootPath.f_GetLen();
			NStr::CStr ToPath = _ToPath + "/";

			for (mint i = 0; i < nPaths; ++i)
			{
				NStr::CStr Path = Paths[i].f_Extract(RootPathLen);
				fs_CreateDirectory(ToPath + Path);
			}

			mint nFiles = Files.f_GetLen();
			for (mint i = 0; i < nFiles; ++i)
			{
				NStr::CStr Path = Files[i].f_Extract(RootPathLen);
				NStr::CStr ToFile = ToPath + Path;
				fs_CreateDirectory(fs_GetPath(ToFile));

				if (_bRaw)
					fs_CopyFileRaw(Files[i], ToFile);
				else
					fs_CopyFile(Files[i], ToFile);
			}
		}

		NStr::CStr CFile::fs_ReadStringFromVector(const NContainer::TCVector<uint8> &_File, bool _bAssumeUTF8)
		{
			NStr::CStr FileData;
			NStream::CBinaryStreamMemoryPtr<> Stream;
			Stream.f_OpenRead(_File.f_GetArray(), _File.f_GetLen());
			FileData = NStr::CStr::fs_ReadTextStream(Stream, _bAssumeUTF8);
			return FileData;
		}

		uint8 *CFile::fp_GetCacheBuffer()
		{
			if (mp_bNonTracked)
				return mp_CacheBufferNonTracked.f_GetArray();
			else
				return mp_CacheBuffer.f_GetArray();
		}

		mint CFile::fp_GetCacheBufferLen()
		{
			if (mp_bNonTracked)
				return mp_CacheBufferNonTracked.f_GetLen();
			else
				return mp_CacheBuffer.f_GetLen();
		}


		void CFile::fp_FlushCache()
		{
			if (!(mp_OpenFlags & EFileOpen_NoLocalCache) && mp_bCacheDirty)
			{
				CMibFilePos CacheSize = fg_Min(CMibFilePos(fp_GetCacheBufferLen()), mp_CachedFileLen - mp_CachePos);
				if (CacheSize > 0)
				{
					CMibFilePos nBytes = NSys::NFile::fg_Write(mp_pFile, fp_GetCacheBuffer(), mp_CachePos, CacheSize);
					if (nBytes != CacheSize)
						DMibErrorFile("Not all bytes were written");
				}

				mp_bCacheDirty = false;
			}
		}

		void *CFile::fp_GetCache(CMibFilePos _CurrentPos, mint &_nBytesAvailable)
		{
			CMibFilePos CacheSize = fp_GetCacheBufferLen();
			if (mp_CachePos >= 0 && _CurrentPos >= mp_CachePos && _CurrentPos < mp_CachePos + CacheSize)
			{
				mint Offset = (_CurrentPos - mp_CachePos);
				_nBytesAvailable = CacheSize - Offset;
				return fp_GetCacheBuffer() + (_CurrentPos - mp_CachePos);
			}

			if (mp_bCacheDirty)
				fp_FlushCache();

			mp_CachePos = fg_AlignDown(_CurrentPos, CacheSize);

			CMibFilePos CacheReadSize = fg_Min(CMibFilePos(CacheSize), mp_CachedFileLen - mp_CachePos);
			if (CacheReadSize > 0)
			{
				CMibFilePos nBytes = NSys::NFile::fg_Read(mp_pFile, fp_GetCacheBuffer(), mp_CachePos, CacheReadSize);
				if (nBytes != CacheReadSize)
					DMibErrorFile("Not all bytes were read");
			}

			mint Offset = (_CurrentPos - mp_CachePos);
			_nBytesAvailable = CacheSize - Offset;
			return fp_GetCacheBuffer() + (_CurrentPos - mp_CachePos);
		}

		CMibFilePos CFile::fp_GetLength() const
		{
			if (mp_OpenFlags & EFileOpen_NoLocalCache)
				return NSys::NFile::fg_GetSize(mp_pFile);
			else
				return mp_CachedFileLen;
		}

		void *CFile::f_GetOSFile() const
		{
			fp_CheckOpen();
			return NSys::NFile::fg_GetOSFile(mp_pFile);
		}

		void CFile::f_Read(void *_pDest, mint _nBytes)
		{
			fp_CheckOpen();

			if (!(mp_OpenFlags & EFileOpen_Read))
				DMibErrorFile("Cannot perform read, file not opened for read");

			if (!(mp_OpenFlags & EFileOpen_NoFileLength))
			{
				if ((mp_FilePos + (CMibFilePos)_nBytes) > fp_GetLength())
					DMibErrorFile("Read call would have read past end of file");
			}

			if (mp_OpenFlags & EFileOpen_NoLocalCache)
			{
				mint nBytes = NSys::NFile::fg_Read(mp_pFile, _pDest, mp_FilePos, _nBytes);

				if (nBytes != _nBytes)
					DMibErrorFile("Not all bytes were read");

				mp_FilePos += nBytes;
			}
			else
			{
				mint ToCopy = _nBytes;
				CMibFilePos CurrentPos = mp_FilePos;
				uint8 *pDest = (uint8 *)_pDest;
				while (ToCopy)
				{
					mint nBytesAvailable;
					void *pCache = fp_GetCache(CurrentPos, nBytesAvailable);
					mint nThisTime = fg_Min(nBytesAvailable, ToCopy);
					NMem::fg_MemCopy(pDest, pCache, nThisTime);
					ToCopy -= nThisTime;
					CurrentPos += nThisTime;
					pDest += nThisTime;
				}

				mp_FilePos += _nBytes;
			}
		}

		void CFile::f_Write(const void *_pSrc, mint _nBytes)
		{
			fp_CheckOpen();

			if (!(mp_OpenFlags & EFileOpen_Write))
				DMibErrorFile("Cannot perform write, file not opened for write");

			if (mp_OpenFlags & EFileOpen_NoLocalCache)
			{
				mint nBytes = NSys::NFile::fg_Write(mp_pFile, _pSrc, mp_FilePos, _nBytes);

				if (nBytes != _nBytes)
					DMibErrorFile("Not all bytes were written");

				mp_FilePos += nBytes;
			}
			else
			{
				mint ToCopy = _nBytes;
				const uint8 *pSrc = (uint8 *)_pSrc;
				while (ToCopy)
				{
					mint nBytesAvailable;
					void *pCache = fp_GetCache(mp_FilePos, nBytesAvailable);
					mint nThisTime = fg_Min(nBytesAvailable, ToCopy);
					NMem::fg_MemCopy(pCache, pSrc, nThisTime);
					mp_bCacheDirty = true;
					mp_FilePos += nThisTime;
					mp_CachedFileLen = fg_Max(mp_CachedFileLen, mp_FilePos);
					ToCopy -= nThisTime;
					pSrc += nThisTime;
				}
			}
		}

		CFile::CFile()
		{
		}

		CFile::CFile(CFile &&_Other)
			: mp_pFile(_Other.mp_pFile)
			, mp_FilePos(_Other.mp_FilePos)
			, mp_OpenFlags(_Other.mp_OpenFlags)
			, mp_CachePos(_Other.mp_CachePos)
			, mp_bCacheDirty(_Other.mp_bCacheDirty)
			, mp_bNonTracked(_Other.mp_bNonTracked)
			, mp_CachedFileLen(_Other.mp_CachedFileLen)
			, mp_CacheBuffer(fg_Move(_Other.mp_CacheBuffer))
			, mp_CacheBufferNonTracked(fg_Move(_Other.mp_CacheBufferNonTracked))
		{
			_Other.mp_pFile = nullptr;
		}

		CFile &CFile::operator = (CFile &&_Other)
		{
			mp_pFile = _Other.mp_pFile;
			mp_FilePos = _Other.mp_FilePos;
			mp_OpenFlags = _Other.mp_OpenFlags;
			mp_CachePos = _Other.mp_CachePos;
			mp_bCacheDirty = _Other.mp_bCacheDirty;
			mp_bNonTracked = _Other.mp_bNonTracked;
			mp_CachedFileLen = _Other.mp_CachedFileLen;
			mp_CacheBuffer = fg_Move(_Other.mp_CacheBuffer);
			mp_CacheBufferNonTracked = fg_Move(_Other.mp_CacheBufferNonTracked);
			_Other.mp_pFile = nullptr;
			return *this;
		}

		CFile::CFile(const NStr::CStr &_FileName, NMib::NFile::EFileOpen _OpenFlags)
		{
			mp_pFile = nullptr;
			f_Open(_FileName, _OpenFlags);
		}

		CFile::CFile(const NStr::CStrNonTracked &_FileName, NMib::NFile::EFileOpen _OpenFlags)
		{
			mp_pFile = nullptr;
			f_Open(_FileName, _OpenFlags);
		}

		CFile::~CFile()
		{
			f_Close(false);
		}

		bint CFile::f_IsValid() const
		{
			return mp_pFile != 0;
		}

		void CFile::f_Close(bint _bCanThrow)
		{
			if (mp_pFile)
			{
				try
				{
					fp_FlushCache();
					NSys::NFile::fg_Close(mp_pFile);
					mp_pFile = nullptr;
				}
				catch (const CExceptionFile &)
				{
					NSys::NFile::fg_Close(mp_pFile);
					mp_pFile = nullptr;
					if (_bCanThrow)
						throw;
				}
			}
			mp_CacheBuffer.f_Clear();
			mp_CacheBufferNonTracked.f_Clear();
		}

		void CFile::f_Open(void *_pFile, ESysFileTag, EFileOpen _OpenFlags)
		{
			f_Close();
			mp_FilePos = 0;
			mp_OpenFlags = _OpenFlags;
			mp_bCacheDirty = false;
			mp_bNonTracked = false;
			mp_CachePos = -1;
			mp_pFile = _pFile;
			if (!(mp_OpenFlags & EFileOpen_NoLocalCache))
			{
				mp_CachedFileLen = NSys::NFile::fg_GetSize(mp_pFile);
				mp_CacheBuffer.f_SetLen(EDefaultCacheSize);
			}
			else
				mp_CachedFileLen = -1;
		}
		
		void CFile::f_Open(const NStr::CStr &_FileName, NMib::NFile::EFileOpen _OpenFlags, EFileAttrib _Attributes)
		{
			NMib::NFile::EFileOpen OpenFlags = _OpenFlags;
			if (_OpenFlags & EFileOpen_Directory)
				OpenFlags |= EFileOpen_NoLocalCache;
			if (!(_OpenFlags & EFileOpen_NoLocalCache))
				OpenFlags |= EFileOpen_Read;

			f_Close();
			mp_FilePos = 0;
			mp_OpenFlags = _OpenFlags;
			mp_bCacheDirty = false;
			mp_bNonTracked = false;
			mp_CachePos = -1;
			mp_pFile = NSys::NFile::fg_Open(_FileName, OpenFlags, _Attributes);
			if (!(mp_OpenFlags & EFileOpen_NoLocalCache))
			{
				mp_CachedFileLen = NSys::NFile::fg_GetSize(mp_pFile);
				mp_CacheBuffer.f_SetLen(EDefaultCacheSize);
			}
			else
				mp_CachedFileLen = -1;
		}

		void CFile::f_Open(const NStr::CStrNonTracked &_FileName, NMib::NFile::EFileOpen _OpenFlags, EFileAttrib _Attributes)
		{
			NMib::NFile::EFileOpen OpenFlags = _OpenFlags;
			if (_OpenFlags & EFileOpen_Directory)
				OpenFlags |= EFileOpen_NoLocalCache;
			if (!(_OpenFlags & EFileOpen_NoLocalCache))
				OpenFlags |= EFileOpen_Read;

			f_Close();
			mp_FilePos = 0;
			mp_OpenFlags = _OpenFlags;
			mp_bCacheDirty = false;
			mp_bNonTracked = true;
			mp_CachePos = -1;
			mp_pFile = NSys::NFile::fg_Open(_FileName, OpenFlags, _Attributes);
			if (!(mp_OpenFlags & EFileOpen_NoLocalCache))
			{
				mp_CachedFileLen = NSys::NFile::fg_GetSize(mp_pFile);
				mp_CacheBufferNonTracked.f_SetLen(EDefaultCacheSize);
			}
			else
				mp_CachedFileLen = -1;
		}


		void CFile::f_FileEnumOtherHandles(NContainer::TCVector<NMib::NFile::CFileHandle> &_HandleInfo)
		{
			fp_CheckOpen();
			NSys::NFile::fg_FileEnumOtherHandles(mp_pFile, _HandleInfo);
		}

		void CFile::fs_FileEnumOtherHandles(const NStr::CStr &_FileName, NContainer::TCVector<NMib::NFile::CFileHandle> &_HandleInfo)
		{
			NSys::NFile::fg_FileEnumOtherHandles(_FileName, _HandleInfo);
		}

		void CFile::f_FlushCache()
		{
			fp_CheckOpen();
			fp_FlushCache();
		}

		void CFile::f_SetCacheSize(mint _CacheSize)
		{
			fp_FlushCache();
			if (!(mp_OpenFlags & EFileOpen_NoLocalCache))
			{
				mint NewCacheSize = mint(1) << fg_GetHighestBitSet(_CacheSize);
				mp_CachePos = -1;
				if (mp_bNonTracked)
					mp_CacheBufferNonTracked.f_SetLen(NewCacheSize);
				else
					mp_CacheBuffer.f_SetLen(NewCacheSize);
			}
		}


		void CFile::f_Flush(bint _bLocalCacheOnly)
		{
			fp_CheckOpen();
			fp_FlushCache();
			if (!_bLocalCacheOnly && (mp_OpenFlags & EFileOpen_Write))
				NSys::NFile::fg_Flush(mp_pFile);
		}

		bint CFile::f_IsAtEndOfFile() const
		{
			fp_CheckOpen();
			return mp_FilePos >= fp_GetLength();
		}

		void CFile::f_SetLength(const CMibFilePos &_Length)
		{
			fp_CheckOpen();
			fp_FlushCache();

			NSys::NFile::fg_SetSize(mp_pFile, _Length);

			if (!(mp_OpenFlags & EFileOpen_NoLocalCache))
				mp_CachedFileLen = _Length;
		}

		CMibFilePos CFile::f_GetLength() const
		{
			fp_CheckOpen();
			return fp_GetLength();
		}

		void CFile::f_SetCreationTime(const NTime::CTime &_Time)
		{
			fp_CheckOpen();
			NSys::NFile::fg_SetCreationTime(mp_pFile, _Time);
		}

		void CFile::f_SetAccessTime(const NTime::CTime &_Time)
		{
			fp_CheckOpen();
			NSys::NFile::fg_SetAccessTime(mp_pFile, _Time);
		}

		void CFile::f_SetWriteTime(const NTime::CTime &_Time)
		{
			fp_CheckOpen();
			NSys::NFile::fg_SetWriteTime(mp_pFile, _Time);
		}

		NTime::CTime CFile::f_GetCreationTime()
		{
			fp_CheckOpen();
			return NSys::NFile::fg_GetCreationTime(mp_pFile);
		}

		NTime::CTime CFile::f_GetAccessTime()
		{
			fp_CheckOpen();
			return NSys::NFile::fg_GetAccessTime(mp_pFile);
		}

		NTime::CTime CFile::f_GetWriteTime()
		{
			fp_CheckOpen();
			return NSys::NFile::fg_GetWriteTime(mp_pFile);
		}

		EFileAttrib CFile::f_GetAttributes()
		{				
			fp_CheckOpen();
			return NSys::NFile::fg_GetAttributes(mp_pFile);
		}

		void CFile::f_SetAttributes(EFileAttrib _Attribs)
		{				
			fp_CheckOpen();
			return NSys::NFile::fg_SetAttributes(mp_pFile, _Attribs);
		}

		void CFile::fs_SetAttributes(NStr::CStr const &_FileName, EFileAttrib _Attribs)
		{				
			return NSys::NFile::fg_SetAttributes(_FileName, _Attribs);
		}
		EFileAttrib CFile::fs_GetAttributes(NStr::CStr const &_FileName)
		{				
			return NSys::NFile::fg_GetAttributes(_FileName);
		}
		
		NTime::CTime CFile::fs_GetCreationTime(NStr::CStr const& _FileName)
		{
			return NSys::NFile::fg_GetCreationTime(_FileName);
		}
		
		NTime::CTime CFile::fs_GetAccessTime(NStr::CStr const& _FileName)
		{
			return NSys::NFile::fg_GetAccessTime(_FileName);
		}
		
		NTime::CTime CFile::fs_GetWriteTime(NStr::CStr const& _FileName)
		{
			return NSys::NFile::fg_GetWriteTime(_FileName);
		}
			
		
		EFileAttrib CFile::fs_GetSupportedAttributes()
		{
			return NSys::NFile::fg_GetSupportedAttributes();
		}
		EFileAttrib CFile::fs_GetValidAttributes()
		{
			return NSys::NFile::fg_GetValidAttributes();
		}
		
		void CFile::f_SetOwner(NStr::CStr const &_Owner)
		{
			NSys::NFile::fg_SetOwner(mp_pFile, _Owner);
		}
		void CFile::f_SetGroup(NStr::CStr const &_Group)
		{
			NSys::NFile::fg_SetGroup(mp_pFile, _Group);
		}

		void CFile::f_SetPosition(NStream::CFilePos _Pos)
		{
			fp_CheckOpen();
			if (_Pos < 0)
				DMibErrorFile("Position is negative");

			if (!(mp_OpenFlags & EFileOpen_Write) && _Pos > fp_GetLength())
				DMibErrorFile("Position is past end of file");

			mp_FilePos = _Pos;
		}

		void CFile::f_SetPositionFromEnd(NStream::CFilePos _Pos)
		{
			fp_CheckOpen();
			if (!(mp_OpenFlags & EFileOpen_Write) && _Pos > 0)
				DMibErrorFile("Position is past end of file");

			NStream::CFilePos FileSize = fp_GetLength();
			FileSize -= _Pos;
			if (FileSize < 0)
				DMibErrorFile("Position is negative");

			mp_FilePos = FileSize;
		}

		void CFile::f_AddPosition(NStream::CFilePos _Pos)
		{
			fp_CheckOpen();
			NStream::CFilePos NewPos = mp_FilePos + _Pos;
			if (NewPos < 0)
				DMibErrorFile("New position would negative");

			if (!(mp_OpenFlags & EFileOpen_Write) && NewPos > fp_GetLength())
				DMibErrorFile("New position would past end of file");

			mp_FilePos = NewPos;
		}

		bint CFile::f_IsValidReadPosition(NStream::CFilePos _Pos) const
		{
			fp_CheckOpen();
			return _Pos >= 0 && _Pos < fp_GetLength();
		}

		void CFile::f_LockRange(const CMibFilePos &_Offset, const CMibFilePos &_NumBytes, EFileLock _Flags)
		{
			fp_CheckOpen();
			NSys::NFile::fg_LockRange(mp_pFile, _Offset, _NumBytes, _Flags);
		}

		void CFile::f_UnlockRange(const CMibFilePos &_Offset, const CMibFilePos &_NumBytes)
		{
			fp_CheckOpen();
			NSys::NFile::fg_UnlockRange(mp_pFile, _Offset, _NumBytes);
		}


		NStream::CFilePos CFile::f_GetPosition() const
		{
			fp_CheckOpen();
			return mp_FilePos;
		}

		bint CFile::fs_MakeFileWritable(const NStr::CStr &_LocalPath, bint _bWritable)
		{
			try
			{
				CFile File;
				File.f_Open(_LocalPath, EFileOpen_ReadAttribs | EFileOpen_WriteAttribs | EFileOpen_ShareAll);
				EFileAttrib Attribs = File.f_GetAttributes();
				if (_bWritable)
				{
					if ((Attribs & NMib::NFile::EFileAttrib_ReadOnly))
					{
						File.f_SetAttributes(Attribs & ~NMib::NFile::EFileAttrib_ReadOnly);
					}
				}
				else
				{
					if (!(Attribs & NMib::NFile::EFileAttrib_ReadOnly))
					{
						File.f_SetAttributes(Attribs | NMib::NFile::EFileAttrib_ReadOnly);
					}
				}
			}
			catch (CExceptionFile const &)
			{
				return false;
			}
			return true;
		}

		bint CFile::fs_IsFileWritable(const NStr::CStr &_LocalPath)
		{
			try
			{
				CFile File;
				File.f_Open(_LocalPath, EFileOpen_ReadAttribs | EFileOpen_WriteAttribs | EFileOpen_ShareAll);
				EFileAttrib Attribs = File.f_GetAttributes();
				if ((Attribs & NMib::NFile::EFileAttrib_ReadOnly))
				{
					return false;
				}
				else
					return true;
			}
			catch (CExceptionFile)
			{
				return false;
			}
		}

		void fgr_DeleteFilesInDirs(const NStr::CStr &_File, bool _bRemoveWriteProtection, NStr::CStr &_Errors)
		{
			NContainer::TCVector<CFile::CFoundFile> Files;
			auto Attribs = CFile::fs_GetAttributes(_File);
			try
			{
				if (!((Attribs & EFileAttrib_Directory) && !(Attribs & EFileAttrib_Link)))
				{
					CFile::fs_DeleteFile(_File);
					return;
				}
			}
			catch (CExceptionFile const &_Exception)
			{
				NStr::fg_AddStrSep(_Errors, _Exception.f_GetErrorStr(), DMibNewLine);
				return;
			}
			
			Files = CFile::fs_FindFilesEx(CFile::fs_AppendPath(_File, "*"), EFileAttrib_File | EFileAttrib_Directory, false);
			// First recurse down
			for (auto iFile = Files.f_GetIterator(); iFile; ++iFile)
			{
				if ((iFile->m_Attribs & NFile::EFileAttrib_File) || (iFile->m_Attribs & NFile::EFileAttrib_Link))
				{
					try
					{
						if (_bRemoveWriteProtection && (iFile->m_Attribs & EFileAttrib_ReadOnly))
							CFile::fs_MakeFileWritable(iFile->m_Path);
						CFile::fs_DeleteFile(iFile->m_Path);
					}
					catch (CExceptionFile const &_Exception)
					{
						NStr::fg_AddStrSep(_Errors, _Exception.f_GetErrorStr(), DMibNewLine);
					}
				}
				else if (iFile->m_Attribs & EFileAttrib_Directory)
				{
					fgr_DeleteFilesInDirs(iFile->m_Path, _bRemoveWriteProtection, _Errors);
				}
			}
			try
			{
				if ((Attribs & EFileAttrib_Directory) && !(Attribs & EFileAttrib_Link))
					CFile::fs_DeleteDirectory(_File);
			}
			catch (CExceptionFile const &_Exception)
			{
				NStr::fg_AddStrSep(_Errors, _Exception.f_GetErrorStr(), DMibNewLine);
			}
		}
		
		void CFile::fs_DeleteDirectoryRecursive(const NStr::CStr &_File, bint _bRemoveWriteProtection)
		{
			NStr::CStr Errors;
			fgr_DeleteFilesInDirs(_File, _bRemoveWriteProtection, Errors);

			if (!Errors.f_IsEmpty())
				DMibErrorFile(Errors);
		}

		void CFile::fs_CopyFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress)
		{
			NSys::NFile::fg_Copy(_FileFrom, _FileTo, _Progress);
		}

		void CFile::fs_CopyFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo)
		{
			NSys::NFile::fg_Copy(_FileFrom, _FileTo);
		}

		void CFile::fs_CopyFileRaw(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo)
		{
			CFile InFile;
			CFile OutFile;
			InFile.f_Open(_FileFrom, EFileOpen_Read | EFileOpen_ShareAll);
			OutFile.f_Open(_FileTo, EFileOpen_Write | EFileOpen_ShareAll);
			NStream::CFilePos Len = InFile.f_GetLength();
			while (Len)
			{
				mint ThisTime = fg_Min(Len, 32768);
				ch8 Temp[32768];
				InFile.f_Read(Temp, ThisTime);
				OutFile.f_Write(Temp, ThisTime);
				Len -= ThisTime;
			}
		}

		void CFile::fs_CreateSymbolicLink(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo, EFileAttrib _Type, ESymbolicLinkFlag _Flags)
		{
			return NSys::NFile::fg_CreateSymbolicLink(_FileFrom, _FileTo, _Type, _Flags);
		}
		
		void CFile::fs_CreateHardLink(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo)
		{
			return NSys::NFile::fg_CreateHardLink(_FileFrom, _FileTo);
		}
		
		NMib::NStr::CStr CFile::fs_ResolveSymbolicLink(const NMib::NStr::CStr &_FileFrom)
		{
			return NSys::NFile::fg_ResolveSymbolicLink(_FileFrom);
		}

		bool CFile::fs_CanCreateSymbolicLink(EFileAttrib _Type, ESymbolicLinkFlag _Flags)
		{
			return NSys::NFile::fg_CanCreateSymbolicLink(_Type, _Flags);
		}


		void CFile::fs_WriteFile(NContainer::TCVector<uint8> const &_FileFrom, const NStr::CStr &_FileTo)
		{
			CFile OutFile;
			OutFile.f_Open(_FileTo, EFileOpen_Write | EFileOpen_ShareAll);
			OutFile.f_Write(_FileFrom.f_GetArray(), _FileFrom.f_GetLen());
		}

		bint CFile::fs_FileIsSame(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName)
		{
			int32 nTimes = 20 * 30; // 30 seconds
			while (1)
			{
				try
				{
					return fsp_FileIsSame(_SourceData, _ToFileName);
				}
				catch (CExceptionFile const &)
				{
					if (--nTimes == 0)
						throw;
					NSys::fg_Thread_Sleep(NMisc::fg_GetRandomFloat() * 0.100);
				}
			}
		}
		
		bint CFile::fsp_FileIsSame(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName)
		{
			NFile::CFile File;
			bint bChanged = false;

			if (!fs_FileExists(_ToFileName))
				return false;
			File.f_Open(_ToFileName, EFileOpen_Read | EFileOpen_ShareAll);

			NStream::CFilePos FileLen = File.f_GetLength();
			if (NStream::CFilePos(_SourceData.f_GetLen()) != FileLen)
				bChanged = true;
			if (!bChanged)
			{
				NContainer::TCVector<uint8> DestData;
				DestData.f_SetLen(FileLen);
				File.f_Read(DestData.f_GetArray(), FileLen);

				if (DestData != _SourceData)
					bChanged = true;
			}

			if (bChanged)
				return false;
			return true;
		}

		NContainer::TCVector<uint8> CFile::fs_ReadFileTry(const NStr::CStr &_ToFileName)
		{
			int32 nTimes = 20 * 30; // 30 seconds
			while (1)
			{
				try
				{
					NFile::CFile File;
					File.f_Open(_ToFileName, EFileOpen_Read | EFileOpen_ShareAll);

					mint FileLen = File.f_GetLength();
					NContainer::TCVector<uint8> DestData;
					DestData.f_SetLen(FileLen);
					File.f_Read(DestData.f_GetArray(), FileLen);

					return DestData;
				}
				catch (CExceptionFile const &)
				{
					if (--nTimes == 0)
						throw;
					NSys::fg_Thread_Sleep(NMisc::fg_GetRandomFloat() * 0.100);
				}

			}

		}
		bint CFile::fs_CopyFileDiff(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs)
		{
			int32 nTimes = 20 * 30; // 30 seconds
			while (1)
			{
				try
				{
					return fsp_CopyFileDiff(_SourceData, NStr::CStr(), _ToFileName, _FileTime, _AddAttribs, fg_Default());
				}
				catch (CExceptionFile const &)
				{
					if (--nTimes == 0)
						throw;
					NSys::fg_Thread_Sleep(NMisc::fg_GetRandomFloat() * 0.100);
				}
			}
		}

		
		bint CFile::fsp_CopyFileDiff(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_FromFileName, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs, NFunction::TCFunction<EDiffCopyChangeAction (CFile::EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange, bool _bRemoveWriteProtection)
		{
			if (fsp_FileIsSame(_SourceData, _ToFileName))
			{
				if (_OnChange)
					_OnChange(EDiffCopyChange_NoChange, _FromFileName, _ToFileName, NStr::CStr());
				return false;
			}

			bint bChanged = false;
			bool bExists = true;

			{
				NFile::CFile File;
				if (!CFile::fs_FileExists(_ToFileName, EFileAttrib_File))
				{
					bChanged = true;
					bExists = false;
				}
				else
				{
					File.f_Open(_ToFileName, EFileOpen_Read | EFileOpen_ShareRead);

					NStream::CFilePos FileLen = File.f_GetLength();
					if (NStream::CFilePos(_SourceData.f_GetLen()) != FileLen)
						bChanged = true;
					if (!bChanged)
					{
						NContainer::TCVector<uint8> DestData;
						DestData.f_SetLen(FileLen);
						File.f_Read(DestData.f_GetArray(), FileLen);

						if (DestData != _SourceData)
							bChanged = true;
					}
				}
			}

			if (bChanged)
			{
				EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
				
				if (_OnChange)
				{
					if (bExists)
						Action = _OnChange(EDiffCopyChange_FileChanged, _FromFileName, _ToFileName, NStr::CStr());
					else
						Action = _OnChange(EDiffCopyChange_FileCreated, _FromFileName, _ToFileName, NStr::CStr());
				}
				
				if (Action == EDiffCopyChangeAction_Perform)
				{				
					if (_bRemoveWriteProtection)
					{
						if (CFile::fs_FileExists(_ToFileName, EFileAttrib_File))
							CFile::fs_MakeFileWritable(_ToFileName, true);
					}
					
					NStream::CFilePos SourceLen = _SourceData.f_GetLen();
					NFile::CFile File;
					
					File.f_Open(_ToFileName, EFileOpen_Write | EFileOpen_ShareRead);
					File.f_Write(_SourceData.f_GetArray(), SourceLen);
					
					if (_AddAttribs != EFileAttrib_None)
					{
						EFileAttrib CurrentAttribs = File.f_GetAttributes();
						File.f_SetAttributes(CurrentAttribs | _AddAttribs);
					}
					File.f_SetWriteTime(_FileTime);
					return true;
				}
			}
			else if (_OnChange)
				_OnChange(EDiffCopyChange_NoChange, _FromFileName, _ToFileName, NStr::CStr());
			return false;
		}

		NTime::CTimeSpan CFile::fsp_GetDefaultFileChangedMargin()
		{
			return NTime::CTimeSpanConvert::fs_CreateSpanFromSeconds(2.05);
		}

		bint CFile::fsp_OpenFile(NFile::CFile &_File, const NStr::CStr &_FileName, EFileOpen _Open, const NTime::CTime &_FileTime)
		{
			_File.f_Open(NStr::CStr(_FileName), _Open);
			if (_File.f_GetLength() > 0 && (_File.f_GetWriteTime() - _FileTime) >= -fsp_GetDefaultFileChangedMargin())
				return false;
			return true;
		};

		bint CFile::fs_CopyFileDiffDate(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs)
		{
			int32 nTimes = 20 * 30; // 30 seconds
			while (1)
			{
				try
				{
					return fsp_CopyFileDiffDate(_SourceData, _ToFileName, _FileTime, _AddAttribs);
				}
				catch (CExceptionFile const &)
				{
					if (--nTimes == 0)
						throw;
					NSys::fg_Thread_Sleep(NMisc::fg_GetRandomFloat() * 0.100);
				}
			}
		}
		bint CFile::fsp_CopyFileDiffDate(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs)
		{
			bint bChanged = false;

			NFile::CFile File;
			NTime::CTime OldTime;
			if (NFile::CFile::fs_FileExists(_ToFileName))
			{
				File.f_Open(_ToFileName, EFileOpen_ReadAttribs | EFileOpen_ShareAll);

				OldTime = File.f_GetWriteTime();
				if (File.f_GetLength() > 0 && (OldTime - _FileTime) >= -fsp_GetDefaultFileChangedMargin())
					return false;
				File.f_Close();
			}


			if (fsp_FileIsSame(_SourceData, _ToFileName))
			{
				if ((_FileTime - OldTime) >= -fsp_GetDefaultFileChangedMargin())
				{
					if (!fsp_OpenFile(File, _ToFileName, EFileOpen_ReadAttribs | EFileOpen_WriteAttribs, _FileTime))
						return false;
					File.f_SetWriteTime(_FileTime);
					return true;
				}
				return false;
			}

			if (!fsp_OpenFile(File, _ToFileName, EFileOpen_Read | EFileOpen_DontTruncate | EFileOpen_Write | EFileOpen_ShareRead, _FileTime))
				return false;

			NStream::CFilePos FileLen = File.f_GetLength();
			if (NStream::CFilePos(_SourceData.f_GetLen()) != FileLen)
				bChanged = true;
			if (!bChanged)
			{
				NContainer::TCVector<uint8> DestData;
				DestData.f_SetLen(FileLen);
				File.f_Read(DestData.f_GetArray(), FileLen);

				if (DestData != _SourceData)
					bChanged = true;
			}

			if (bChanged)
			{
				NStream::CFilePos SourceLen = _SourceData.f_GetLen();
				File.f_SetPosition(0);
				File.f_SetLength(SourceLen);
				File.f_Write(_SourceData.f_GetArray(), SourceLen);
				if (_AddAttribs != EFileAttrib_None)
				{
					EFileAttrib CurrentAttribs = File.f_GetAttributes();
					File.f_SetAttributes(CurrentAttribs | _AddAttribs);
				}
				File.f_SetWriteTime(_FileTime);
				return true;
			}
			if (_FileTime > OldTime)
			{
				File.f_SetWriteTime(_FileTime);
				return true;
			}

			return false;
		}

		bint CFile::fs_CopyFileDiff(const NStr::CStr &_FromFileName, const NStr::CStr &_ToFileName, bint _bCopyDate)
		{
			int32 nTimes = 20 * 30; // 30 seconds
			while (1)
			{
				try
				{
					return fsp_CopyFileDiff(_FromFileName, _ToFileName, _bCopyDate, fg_Default());
				}
				catch (CExceptionFile const &)
				{
					if (--nTimes == 0)
						throw;
					NSys::fg_Thread_Sleep(NMisc::fg_GetRandomFloat() * 0.100);
				}
			}
		}
		bint CFile::fsp_CopyFileDiff(const NStr::CStr &_FromFileName, const NStr::CStr &_ToFileName, bool _bCopyDate, NFunction::TCFunction<EDiffCopyChangeAction (EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange, bool _bRemoveWriteProtection)
		{
			NFile::CFile File;
			File.f_Open(_FromFileName, EFileOpen_Read | EFileOpen_ShareAll);
			NContainer::TCVector<uint8> SourceData;
			mint FileLen = File.f_GetLength();
			SourceData.f_SetLen(FileLen);
			File.f_Read(SourceData.f_GetArray(), FileLen);
			EFileAttrib Attribs = File.f_GetAttributes() & EFileAttrib_Executable;
			if (_bCopyDate)
				return fsp_CopyFileDiff(SourceData, _FromFileName, _ToFileName, File.f_GetWriteTime(), Attribs, _OnChange, _bRemoveWriteProtection);
			else
				return fsp_CopyFileDiff(SourceData, _FromFileName, _ToFileName, NTime::CTime::fs_NowUTC(), Attribs, _OnChange, _bRemoveWriteProtection);
		}


		void CFile::fs_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo)
		{
			NSys::NFile::fg_Rename(_FileFrom, _FileTo);
		}

		void CFile::fs_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress)
		{
			NSys::NFile::fg_Rename(_FileFrom, _FileTo, _Progress);
		}

		void CFile::fs_CreateDirectory(const NStr::CStr &_Path)
		{
			NSys::NFile::fg_CreateDirectory(_Path);
		}

		void CFile::fs_DeleteFile(const NStr::CStr &_File)
		{
			NSys::NFile::fg_Delete(_File);
		}

		void CFile::fs_DeleteDirectory(const NStr::CStr &_File)
		{
			NSys::NFile::fg_DeleteDirectory(_File);
		}

		void CFile::fs_CreateDirectory(const NStr::CStrNonTracked &_Path)
		{
			NSys::NFile::fg_CreateDirectory(_Path);
		}
		void CFile::fs_DeleteFile(const NStr::CStrNonTracked &_File)
		{
			NSys::NFile::fg_Delete(_File);
		}
		void CFile::fs_DeleteDirectory(const NStr::CStrNonTracked &_File)
		{
			NSys::NFile::fg_DeleteDirectory(_File);
		}


		void CFile::fs_SetCurrentDirectory(const NStr::CStr &_Directory)
		{
			return NSys::NFile::fg_SetCurrentDirectory(_Directory);
		}

		ch8 const *CFile::fs_GetDllExtension()
		{
			return NSys::NFile::fg_GetDllExtension();
		}

		NStr::CStr CFile::fs_GetProgramPath()
		{
			return NSys::NFile::fg_GetProgramPath();
		}

		NStr::CStr CFile::fs_GetProgramDirectory()
		{
			return NSys::NFile::fg_GetProgramDirectory();
		}

		NStr::CStr CFile::fs_GetCurrentDirectory()
		{
			return NSys::NFile::fg_GetCurrentDirectory();
		}

		NStr::CStr CFile::fs_GetUserProgramDirectory()
		{
			return NSys::NFile::fg_GetUserProgramDirectory();
		}

		NStr::CStr CFile::fs_GetUserLocalProgramDirectory()
		{
			return NSys::NFile::fg_GetUserLocalProgramDirectory();
		}

		NMib::NStr::CStr CFile::fs_GetUserLocalProgramCacheDirectory()
		{
			return NSys::NFile::fg_GetUserLocalProgramCacheDirectory();
		}
		
		NStr::CStr CFile::fs_GetTemporaryDirectory()
		{
			return NSys::NFile::fg_GetTemporaryDirectory();
		}

		NStr::CStr CFile::fs_GetModulePath(void *_pCode)
		{
			return NSys::NFile::fg_GetModulePath(_pCode);
		}
        
        NStr::CStr CFile::fs_GetUserHomeDirectory()
        {
            return NSys::NFile::fg_GetUserHomeDirectory();
        }
        
        NStr::CStr CFile::fs_GetLogDirectory()
        {
            return NSys::NFile::fg_GetLogDirectory();
        }
		
		NStr::CStr CFile::fs_GetProgramRoot()
		{
			return fg_GetSys()->f_GetProgramRoot();
		}

		NStr::CStrNonTracked CFile::fs_GetProgramRootNonTracked()
		{
			return fg_GetSys()->f_GetProgramRootNonTracked();
		}

		NStr::CStrNonTracked CFile::fs_GetProgramPathNonTracked()
		{
			return NSys::NFile::fg_GetProgramPathNonTracked();
		}

		NStr::CStrNonTracked CFile::fs_GetProgramDirectoryNonTracked()
		{
			return NSys::NFile::fg_GetProgramDirectoryNonTracked();
		}

		NStr::CStrNonTracked CFile::fs_GetCurrentDirectoryNonTracked()
		{
			return NSys::NFile::fg_GetCurrentDirectoryNonTracked();
		}

		NStr::CStrNonTracked CFile::fs_GetUserProgramDirectoryNonTracked()
		{
			return NSys::NFile::fg_GetUserProgramDirectoryNonTracked();
		}

		NStr::CStrNonTracked CFile::fs_GetUserLocalProgramDirectoryNonTracked()
		{
			return NSys::NFile::fg_GetUserLocalProgramDirectoryNonTracked();
		}

		NStr::CStrNonTracked CFile::fs_GetUserLocalProgramCacheDirectoryNonTracked()
		{
			return NSys::NFile::fg_GetUserLocalProgramCacheDirectoryNonTracked();
		}

		NStr::CStrNonTracked CFile::fs_GetTemporaryDirectoryNonTracked()
		{
			return NSys::NFile::fg_GetTemporaryDirectoryNonTracked();
		}

		NStr::CStrNonTracked CFile::fs_GetModulePathNonTracked(void *_pCode)
		{
			return NSys::NFile::fg_GetModulePathNonTracked(_pCode);
		}
		
        NStr::CStrNonTracked CFile::fs_GetUserHomeDirectoryNonTracked()
        {
            return NSys::NFile::fg_GetUserHomeDirectoryNonTracked();
        }
		
        NStr::CStrNonTracked CFile::fs_GetLogDirectoryNonTracked()
        {
            return NSys::NFile::fg_GetLogDirectoryNonTracked();
        }

		
		NMib::NStream::CFilePos CFile::fs_GetFreeSpace(const NMib::NStr::CStr &_Path)
		{
			return NSys::NFile::fg_GetFreeSpace(_Path);
		}

		NMib::NStream::CFilePos CFile::fs_GetUsedSpace(const NMib::NStr::CStr &_Path)
		{
			return NSys::NFile::fg_GetUsedSpace(_Path);
		}

		bint CFile::fs_FileExists(const NStr::CStr &_File, EFileAttrib _AttribMask)
		{
			return NSys::NFile::fg_FileExists(_File, _AttribMask);
		}
		bint CFile::fs_FileExists(const NStr::CStrNonTracked &_File, EFileAttrib _AttribMask)
		{
			return NSys::NFile::fg_FileExists(_File, _AttribMask);
		}

		ECheckFileRights CFile::fs_CheckFileRights(NStr::CStr const& _File, EFileRight _Rights)
		{
			return NSys::NFile::fg_CheckFileRights(_File, _Rights);
		}

		namespace
		{
			NFunction::TCFunctionNoAlloc<bool (NStr::CStr const &_FileName, EFileAttrib _Attribs, bool _bPreRecurse)> fg_DefaultFindFilter
				(
					CFile::CFindFilesOptions const *_pOptions
					, NFunction::TCFunctionNoAlloc<void (NStr::CStr const& _Path, EFileAttrib _Attribs)> const *_pOnAdd
				)
			{
				NStr::CStr Pattern = CFile::fs_GetFile(_pOptions->m_Path);
				return
					[=](NStr::CStr const &_FileName, EFileAttrib _Attribs, bool _bPreRecurse) -> bool
					{
						if (!_pOptions->m_ExcludePatterns.f_IsEmpty())
						{
							for (auto const &Pattern : _pOptions->m_ExcludePatterns)
							{
								if (NStr::fg_StrMatchWildcard(_FileName.f_GetStr(), Pattern.f_GetStr()) == NStr::EMatchWildcardResult_WholeStringMatchedAndPatternExhausted)
									return false;
							}
						}
						if (_Attribs & EFileAttrib_Directory)
						{
							if ((_Attribs & _pOptions->m_AttribMask) || ((_pOptions->m_AttribMask & EFileAttrib_File) && !(_Attribs & EFileAttrib_Directory)))
							{
								if (_bPreRecurse == !(_pOptions->m_AttribMask & EFileAttrib_FindDirectoryLast))
								{
									if
										(
											NStr::fg_StrMatchWildcard(CFile::fs_GetFile(_FileName).f_GetStr(), Pattern.f_GetStr())
											== NStr::EMatchWildcardResult_WholeStringMatchedAndPatternExhausted
										)
									{
										(*_pOnAdd)(_FileName, _Attribs);
									}
								}
							}
							return _pOptions->m_bFollowLinks || !(_Attribs & EFileAttrib_Link);
						}
						else
						{
							if ((_Attribs & _pOptions->m_AttribMask) || ((_pOptions->m_AttribMask & EFileAttrib_File) && !(_Attribs & EFileAttrib_Directory)))
								(*_pOnAdd)(_FileName, _Attribs);
							return false;
						}
					}
				;
			}
		}

		NContainer::TCVector<NStr::CStr> CFile::fs_FindFiles(const NStr::CStr &_FindPath, EFileAttrib _AttribMask, bint _bRecursive, bool _bFollowLinks)
		{
			NContainer::TCVector<NStr::CStr> Files;

			NFunction::TCFunctionNoAlloc<void (NStr::CStr const& _Path, EFileAttrib _Attribs)> fFoundFile = [&](NStr::CStr const& _Path, EFileAttrib _Attribs)
				{
					Files.f_Insert(_Path);
				}
			;

			CFindFilesOptions Options(_FindPath, _bRecursive);
			Options.m_AttribMask = _AttribMask;
			Options.m_bFollowLinks = _bFollowLinks;

			fg_FindFiles
				(
					_FindPath
					, fg_DefaultFindFilter(&Options, &fFoundFile)
					, true
					, _bRecursive
				)
			;

			return Files;
		}

		NContainer::TCVector<CFile::CFoundFile> CFile::fs_FindFiles(CFindFilesOptions const &_Options)
		{
			NContainer::TCVector<CFoundFile> Files;

			NFunction::TCFunctionNoAlloc<void (NStr::CStr const& _Path, EFileAttrib _Attribs)> fFoundFile = [&](NStr::CStr const& _Path, EFileAttrib _Attribs)
				{
					CFoundFile& Found = Files.f_Insert();
					Found.m_Path = _Path;
					Found.m_Attribs = EFileAttrib(_Attribs);
				}
			;

			fg_FindFiles
				(
					_Options.m_Path
					, fg_DefaultFindFilter(&_Options, &fFoundFile)
					, true
					, _Options.m_bRecursive
				)
			;

			return Files;
		}


		NContainer::TCVector<CFile::CFoundFile> CFile::fs_FindFilesEx(const NStr::CStr &_FindPath, EFileAttrib _AttribMask, bint _bRecursive, bool _bFollowLinks)
		{
			NContainer::TCVector<CFoundFile> Files;

			NFunction::TCFunctionNoAlloc<void (NStr::CStr const& _Path, EFileAttrib _Attribs)> fFoundFile = [&](NStr::CStr const& _Path, EFileAttrib _Attribs)
				{
					CFoundFile& Found = Files.f_Insert();
					Found.m_Path = _Path;
					Found.m_Attribs = EFileAttrib(_Attribs);
				}
			;

			CFindFilesOptions Options(_FindPath, _bRecursive);
			Options.m_AttribMask = _AttribMask;
			Options.m_bFollowLinks = _bFollowLinks;

			fg_FindFiles
				(
					_FindPath
					, fg_DefaultFindFilter(&Options, &fFoundFile)
					, true
					, _bRecursive
				)
			;

			return Files;
		}

		void CFile::fs_FindFilesGeneral
			(
				const NStr::CStr &_FindPath
				, NFunction::TCFunction<bool (NStr::CStr const &_FileName, EFileAttrib _Attribs, bool _bPreRecurse)> const &_fFoundFile
				, bool _bRecurse
			)
		{
			fg_FindFiles
				(
					_FindPath
					, [&_fFoundFile](NStr::CStr const &_FileName, EFileAttrib _Attribs, bool _bPreRecurse) -> bool
					{
						return _fFoundFile(_FileName, _Attribs, _bPreRecurse);
					}
					, false
					, _bRecurse
				)
			;
		}


		NStr::CStr CFile::fs_CondensePath(NStr::CStr _Path)
		{
			if (_Path.f_IsEmpty())
				return _Path;

			NStr::CStr Path = _Path;
			fg_StrReplaceChar(Path, '\\', '/');

			NStr::CStr Prefix;
			NStr::CStr Suffix;

			Prefix = fs_ExtractPathRoot(Path);

			if (Path[Path.f_GetLen() - 1] == '/')
				Suffix = "/";

			// TODO: When we have static substring objects this can be boiled down to a couple of allocs.

			NStr::CStr Portion;
			mint iHead = 0;
			mint SectionsCharCount = 0;
			NContainer::TCVector<NStr::CStr> lSections;

			auto fl_Push = [&](NStr::CStr && _Section)
			{
				if (lSections.f_GetLen() == iHead)
					lSections.f_Insert(fg_Move(_Section));
				else
					lSections[iHead] = fg_Move(_Section);

				SectionsCharCount += lSections[iHead].f_GetLen();

				++iHead;
			};

			auto fl_Pop = [&]()
			{
				if (iHead)
				{
					--iHead;
					SectionsCharCount -= lSections[iHead].f_GetLen();
				}
			};

			while(!Path.f_IsEmpty())
			{
				Portion = NStr::fg_GetStrSep(Path, "/");

				if (Portion == "..")
				{
					if (lSections.f_IsEmpty())
						fl_Push(fg_Move(Portion));
					else
						fl_Pop();
				}
				else if (Portion == "." || Portion.f_IsEmpty())
				{
				}
				else
				{
					fl_Push(fg_Move(Portion));
				}
			}

			if (iHead == 0)
				return NStr::CStr();
			
			NStr::CStr Out;
			mint const nOutChars = Prefix.f_GetLen() + SectionsCharCount + (iHead - 1) + Suffix.f_GetLen();
			NStr::CStr::CChar* pOut = Out.f_GetStr(nOutChars);
			NStr::CStr::CChar* pEnd = pOut + nOutChars;
			(void)pEnd;

			auto fl_WriteStr = [&](NStr::CStr const& _Str)
			{
				mint const nChars = _Str.f_GetLen();
				mint const nBytes = nChars * sizeof(NStr::CStr::CChar);
				DMibSafeCheck((pOut + nChars) <= (pOut + nOutChars), "Buffer overrun.");
				NMem::fg_MemCopy(pOut, _Str.f_GetStr(), nBytes);
				pOut += nChars;
			};

			if (!Prefix.f_IsEmpty())
				fl_WriteStr(Prefix);

			mint iS = 0;
			fl_WriteStr(lSections[iS]);
			++iS;

			for (; iS < iHead; ++iS)
			{
				*pOut = '/';
				++pOut;
				fl_WriteStr(lSections[iS]);
			}

			if (!Suffix.f_IsEmpty())
				fl_WriteStr(Suffix);

			DMibSafeCheck(pOut == pEnd, "Incorrect string length calculation");
			*pOut = 0;

			return fg_Move(Out);
		}


		CMibFilePos CFile::fs_GetFileSize(const NStr::CStr &_Path)
		{
			return NSys::NFile::fg_GetSize(_Path);
		}
		
		NStr::CStr CFile::fs_GetOwner(NStr::CStr const &_Path)
		{
			return NSys::NFile::fg_GetOwner(_Path);
		}
		
		NStr::CStr CFile::fs_GetGroup(NStr::CStr const &_Path)
		{
			return NSys::NFile::fg_GetGroup(_Path);
		}
		
		NStr::CStr CFile::fs_GetOwnerOnLink(NStr::CStr const &_Path)
		{
			return NSys::NFile::fg_GetOwnerOnLink(_Path);
		}
		
		NStr::CStr CFile::fs_GetGroupOnLink(NStr::CStr const &_Path)
		{
			return NSys::NFile::fg_GetGroupOnLink(_Path);
		}
		
		void CFile::fs_SetOwner(NStr::CStr const &_Path, NStr::CStr const &_Owner)
		{
			NSys::NFile::fg_SetOwner(_Path, _Owner);
		}
		
		void CFile::fs_SetGroup(NStr::CStr const &_Path, NStr::CStr const &_Group)
		{
			NSys::NFile::fg_SetGroup(_Path, _Group);
		}
		
		void CFile::fs_SetOwnerOnLink(NStr::CStr const &_Path, NStr::CStr const &_Owner)
		{
			NSys::NFile::fg_SetOwnerOnLink(_Path, _Owner);
		}
		
		void CFile::fs_SetGroupOnLink(NStr::CStr const &_Path, NStr::CStr const &_Group)
		{
			NSys::NFile::fg_SetGroupOnLink(_Path, _Group);
		}

		bool CFile::fs_SetOwnerAndGroupRecursive(NStr::CStr const &_Path, NStr::CStr const &_Owner, NStr::CStr const &_Group, bool _bFollowLinks)
		{
			auto SourceAttribs = CFile::fs_GetAttributes(_Path);
			bool bIsDirectory = (SourceAttribs & EFileAttrib_Directory) != 0;
			bool bIsLink = (SourceAttribs & EFileAttrib_Link) != 0;
			
			bool bRet = false;
			
			auto fl_GetOwner 
				= [&](NStr::CStr const &_Path) -> NStr::CStr
				{
					try
					{
						return fs_GetOwner(_Path);
					}
					catch (NMib::NException::CException const &)
					{
					}
					
					return NStr::CStr();
				}
			;
			auto fl_GetGroup
				= [&](NStr::CStr const &_Path) -> NStr::CStr
				{
					try
					{
						return fs_GetGroup(_Path);
					}
					catch (NMib::NException::CException const &)
					{
					}
					
					return NStr::CStr();
				}
			;
			auto fl_GetOwnerOnLink
				= [&](NStr::CStr const &_Path) -> NStr::CStr
				{
					try
					{
						return fs_GetOwnerOnLink(_Path);
					}
					catch (NMib::NException::CException const &)
					{
					}
					
					return NStr::CStr();
				}
			;
			auto fl_GetGroupOnLink
				= [&](NStr::CStr const &_Path) -> NStr::CStr
				{
					try
					{
						return fs_GetGroupOnLink(_Path);
					}
					catch (NMib::NException::CException const &)
					{
					}
					
					return NStr::CStr();
				}
			;
			
			auto fl_SetOwnerAndGroup =
				[&](NStr::CStr const &_Path)
				{
					if (!_Owner.f_IsEmpty())
					{
						if (fl_GetOwner(_Path) != _Owner)
						{
							bRet = true;
							fs_SetOwner(_Path, _Owner);
						}
					}
					if (!_Group.f_IsEmpty())
					{
						if (fl_GetGroup(_Path) != _Group)
						{
							bRet = true;
							fs_SetGroup(_Path, _Group);
						}
					}
				}
			;
			
			auto fl_SetOwnerAndGroupOnLink =
				[&](NStr::CStr const &_Path)
				{
					if (!_Owner.f_IsEmpty())
					{
						if (fl_GetOwnerOnLink(_Path) != _Owner)
						{
							bRet = true;
							fs_SetOwnerOnLink(_Path, _Owner);
						}
					}
					if (!_Group.f_IsEmpty())
					{
						if (fl_GetGroupOnLink(_Path) != _Group)
						{
							bRet = true;
							fs_SetGroupOnLink(_Path, _Group);
						}
					}
				}
			;

			if (bIsDirectory && !bIsLink)
			{
				NContainer::TCVector<CFile::CFoundFile> SourceFiles = CFile::fs_FindFilesEx(_Path + "/*", EFileAttrib_File, true, false);
				NContainer::TCVector<CFile::CFoundFile> SourceDirectories = CFile::fs_FindFilesEx(_Path + "/*", EFileAttrib_Directory, true, false);
				
				NContainer::TCMap<NStr::CStr, CFile::CFoundFile> SourceFilesSet;
				for (auto iFile = SourceFiles.f_GetIterator(); iFile; ++iFile)
				{
					NStr::CStr Key = iFile->m_Path.f_Extract(_Path.f_GetLen() + 1);
					SourceFilesSet[Key] = *iFile;
				}
				NContainer::TCMap<NStr::CStr, CFile::CFoundFile> SourceDirectoriesSet;
				for (auto iFile = SourceDirectories.f_GetIterator(); iFile; ++iFile)
				{
					NStr::CStr Key = iFile->m_Path.f_Extract(_Path.f_GetLen() + 1);
					SourceDirectoriesSet[Key] = *iFile;
				}
				
				fl_SetOwnerAndGroup(_Path);

				for (auto iFile = SourceDirectoriesSet.f_GetIterator(); iFile; ++iFile)
				{
					if (iFile->m_Attribs & EFileAttrib_Link && !_bFollowLinks)
						fl_SetOwnerAndGroupOnLink(iFile->m_Path);
					else
						fl_SetOwnerAndGroup(iFile->m_Path);
				}
				
				for (auto iFile = SourceFilesSet.f_GetIterator(); iFile; ++iFile)
				{
					if (iFile->m_Attribs & EFileAttrib_Link && !_bFollowLinks)
						fl_SetOwnerAndGroupOnLink(iFile->m_Path);
					else
						fl_SetOwnerAndGroup(iFile->m_Path);
				}
			}
			else
			{
				if (SourceAttribs & EFileAttrib_Link && !_bFollowLinks)
					fl_SetOwnerAndGroupOnLink(_Path);
				else
				{
					if (CFile::fs_FileExists(_Path, EFileAttrib_File))
						fl_SetOwnerAndGroup(_Path);

				}
			}
			return bRet;
		}
		
		bool CFile::fs_SetOwnerRecursive(NStr::CStr const &_Path, NStr::CStr const &_Owner, bool _bFollowLinks)
		{
			return fs_SetOwnerAndGroupRecursive(_Path, _Owner, "", _bFollowLinks);
		}
		
		bool CFile::fs_SetGroupRecursive(NStr::CStr const &_Path, NStr::CStr const &_Group, bool _bFollowLinks)
		{
			return fs_SetOwnerAndGroupRecursive(_Path, "", _Group, _bFollowLinks);
		}

		NDataProcessing::CHashDigest_MD5 CFile::fs_GetFileChecksum(const NStr::CStr &_Path)
		{
			CFile File;
			File.f_Open(_Path, EFileOpen_Read | EFileOpen_ShareAll);

			CMibFilePos Length = File.f_GetLength();
			NDataProcessing::CHash_MD5 Checksum;

			while (Length)
			{
				mint ThisTime = mint(fg_Min(Length, CMibFilePos(32768)));
				uint8 Temp[32768];
				File.f_Read(Temp, ThisTime);
				Checksum.f_AddData(Temp, ThisTime);

				Length -= ThisTime;
			}

			return Checksum;
		}

		NDataProcessing::CHashDigest_MD5 CFile::fs_GetDirectoryChecksum
			(
				const NStr::CStr &_Path
				, NFunction::TCFunction<bint (NStr::CStr const &_File)> const &_ExcludeFile
				, NFunction::TCFunction<NDataProcessing::CHashDigest_MD5 (NStr::CStr const &_File)> const &_GetHash
			)
		{
			NContainer::TCVector<NStr::CStr> Files = fs_FindFiles(_Path + "/*", EFileAttrib_File|EFileAttrib_Directory, true);
			NDataProcessing::CHash_MD5 Checksum;
            
			for (auto FIter = Files.f_GetIterator(); FIter; ++FIter)
			{
				if (_ExcludeFile(*FIter))
					continue;
				
				if (fs_FileExists(*FIter, EFileAttrib_File))
				{
					NDataProcessing::CHashDigest_MD5 FileHash;
					if (_GetHash)
						FileHash = _GetHash(*FIter);
					else
						FileHash = fs_GetFileChecksum(*FIter);
					
					Checksum.f_AddData(FileHash.f_GetData(), FileHash.fs_GetSize());
				}
                
				NStr::CStr FileName = CFile::fs_GetFile(*FIter);
				Checksum.f_AddData((uint8*)FileName.f_GetStr(), FileName.f_GetLen());
			}
            
			return Checksum;
		}
		
		void CFile::fs_WriteStringToFile(const NStr::CStr &_Path, const NStr::CStr &_ToWrite, bint _bAddBOM, EFileAttrib _Attributes)
		{
			uint8 BOM[] = {0xEF, 0xBB, 0xBF};
			NStr::CStr UTF8 = _ToWrite;
			CFile File;
			File.f_Open(_Path, EFileOpen_Write | EFileOpen_ShareAll, _Attributes);
			if (_bAddBOM)
				File.f_Write(BOM, 3);
			File.f_Write(UTF8.f_GetStr(), UTF8.f_GetLen());
		}

		NStr::CStr CFile::fs_ReadStringFromFile(const NStr::CStr &_Path, bool _bAssumeUTF8)
		{
			NStr::CStr FileData;
			TCBinaryStreamFile<> Stream;
			Stream.f_Open(_Path, EFileOpen_Read|EFileOpen_ShareAll);
			FileData = NStr::CStr::fs_ReadTextStream(Stream, _bAssumeUTF8);
			return FileData;
		}


		void CFile::fs_WriteStringToFile(const NStr::CStrNonTracked &_Path, const NStr::CStrNonTracked &_ToWrite, bint _bAddBOM, EFileAttrib _Attributes)
		{
			uint8 BOM[] = {0xEF, 0xBB, 0xBF};

			NStr::CStrNonTracked UTF8 = _ToWrite;

			CFile File;
			File.f_Open(_Path, EFileOpen_Write | EFileOpen_ShareAll | EFileOpen_NoLocalCache, _Attributes);
			if (_bAddBOM)
				File.f_Write(BOM, 3);
			File.f_Write(UTF8.f_GetStr(), UTF8.f_GetLen());
		}


		NStr::CStrNonTracked CFile::fs_ReadStringFromFile(const NStr::CStrNonTracked &_Path, bool _bAssumeUTF8)
		{
			NStr::CStrNonTracked FileData;
			TCBinaryStreamFile<> Stream;
			Stream.f_Open(_Path, EFileOpen_Read|EFileOpen_ShareAll);
			FileData = NStr::CStrNonTracked::fs_ReadTextStream(Stream, _bAssumeUTF8);
			return FileData;
		}

		void CFile::fs_WriteStringToVector(NContainer::TCVector<uint8> &_File, const NStr::CStr &_ToWrite, bint _bAddBOM)
		{
			uint8 BOM[] = {0xEF, 0xBB, 0xBF};
			NStr::CStr UTF8 = _ToWrite;
			if (_bAddBOM)
				_File.f_Insert(BOM, 3);
			_File.f_Insert((const uint8 *)UTF8.f_GetStr(), UTF8.f_GetLen());
		}

		void CFile::fs_WriteFile(const NStr::CStr &_Path, NContainer::TCVector<uint8> const &_Data)
		{
			CFile File;
			File.f_Open(_Path, EFileOpen_Write | EFileOpen_ShareAll);
			File.f_Write(_Data.f_GetArray(), _Data.f_GetLen());
		}

		NContainer::TCVector<uint8> CFile::fs_ReadFile(const NStr::CStr &_Path)
		{
			NContainer::TCVector<uint8> FileData;
			CFile File;
			File.f_Open(_Path, EFileOpen_Read|EFileOpen_ShareAll);
			FileData.f_SetLen(File.f_GetLength());
			File.f_Read(FileData.f_GetArray(), FileData.f_GetLen());
			return fg_Move(FileData);
		}

		const ch8 *CFile::fs_GetInvalidFileNameChars(bint _bPath)
		{
			if (_bPath)
			{
				static const ch8 s_pInvalidChars[] = {'<', '>',':','"','\\','|', '*', '?', 0};
				return s_pInvalidChars;
			}
			else
			{
				static const ch8 s_pInvalidChars[] = {'<', '>',':','"','/', '\\','|', '*', '?', 0};
				return s_pInvalidChars;
			}
		}

		const ch8 **CFile::fs_GetInvalidFileNameNames()
		{
			static const ch8 *s_pInvalidNames[] = {"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9", nullptr};

			return s_pInvalidNames;
		}

		NStr::CStr CFile::fs_MakeNiceFilename(const NStr::CStr &_CurrentName)
		{
			const ch8 *InvalidChars = fs_GetInvalidFileNameChars(false);

			NStr::CUStr Output;
			Output = _CurrentName;
			NTraits::TCUnsigned<ch32>::CType *pStr = (NTraits::TCUnsigned<ch32>::CType *)Output.f_GetStrUniqueWritable();
			const NTraits::TCUnsigned<ch8>::CType *pInvalidChars = (const NTraits::TCUnsigned<ch8>::CType *)InvalidChars;
			while (*pStr)
			{
				if (*pStr >= 1 && *pStr <= 31)
				{
					*pStr = '_';
				}
				else
				{
					const NTraits::TCUnsigned<ch8>::CType *pInv = pInvalidChars;
					while (*pInv)
					{
						if (*pStr == *pInv)
						{
							*pStr = '_';
							break;
						}
						++pInv;
					}
				}

				++pStr;
			}
				
			const ch8 **pInvalidNames = fs_GetInvalidFileNameNames();
				
			pStr = (NTraits::TCUnsigned<ch32>::CType *)Output.f_GetStrUniqueWritable();
			NStr::CUStr Output2;
			while (*pStr)
			{
				const NTraits::TCUnsigned<ch32>::CType *pParseStart = pStr;

				while (*pStr && *pStr != '.' && *pStr != '/')
				{
					++pStr;
				}
				mint Len = pStr - pParseStart;
				bint bValid = true;
				if (Len)
				{
					const ch8 **pInv = pInvalidNames;
					while (*pInv)
					{
						if (NStr::fg_StrCmpNoCase(pParseStart, *pInv, Len) == 0)
						{
							bValid = false;
						}
						++pInv;
					}
				}

				if (bValid)
					Output2.f_AddStr(pParseStart, Len);
				else
				{
					Output2.f_AddStr(pParseStart, Len);
					Output2.f_AddChar('_');
				}

				if (*pStr)
				{
					Output2.f_AddChar(*pStr);
					++pStr;						
				}
			}

			if (Output2.f_IsEmpty())
				Output2 = "_";
			else 
			{
				ch32 LastChar = Output2[Output2.f_GetLen()-1];
				if (LastChar == '.' || LastChar == ' ')
					Output2.f_AddChar('_');
			}

			Output = Output2;
			return Output;
		}

		auto CFile::fsp_EncodeChar(ch32 _Char) -> NStr::CFWStr16
		{
			if (_Char == '$')
				return str_utf16("$$");
			return NStr::CFWStr16::CFormat(str_utf16("${nfh}")) << _Char;
		}
		auto CFile::fsp_EncodeString(ch32 *_pChars, mint _Len) -> NStr::CUStr
		{
			NStr::CUStr Ret;

			for (mint i = 0; i < _Len; ++i)
				Ret += fsp_EncodeChar(_pChars[i]);

			return Ret;
		}
		NStr::CStr CFile::fs_MakeNiceUniqueFilename(const NStr::CStr &_CurrentName)
		{
			const ch8 *InvalidChars = fs_GetInvalidFileNameChars(false);

			NStr::CUStr Input = _CurrentName;
			NStr::CUStr Output;
			NTraits::TCUnsigned<ch32>::CType const *pStr = (NTraits::TCUnsigned<ch32>::CType const *)Input.f_GetStr();
			const NTraits::TCUnsigned<ch8>::CType *pInvalidChars = (const NTraits::TCUnsigned<ch8>::CType *)InvalidChars;
			while (*pStr)
			{
				if ((*pStr >= 1 && *pStr <= 31) || *pStr == '$')
				{
					Output += fsp_EncodeChar(*pStr);
				}
				else
				{
					bint bFound = false;
					const NTraits::TCUnsigned<ch8>::CType *pInv = pInvalidChars;
					while (*pInv)
					{
						if (*pStr == *pInv)
						{
							Output += fsp_EncodeChar(*pStr);
							bFound = true;
							break;
						}
						++pInv;
					}
					if (!bFound)
						Output.f_AddChar(*pStr);
				}

				++pStr;
			}
				
			const ch8 **pInvalidNames = fs_GetInvalidFileNameNames();
				
			pStr = (NTraits::TCUnsigned<ch32>::CType const *)Output.f_GetStr();
			NStr::CUStr Output2;
			while (*pStr)
			{
				const NTraits::TCUnsigned<ch32>::CType *pParseStart = pStr;

				while (*pStr && *pStr != '.' && *pStr != '/')
				{
					++pStr;
				}
				mint Len = pStr - pParseStart;
				bint bValid = true;
				if (Len)
				{
					const ch8 **pInv = pInvalidNames;
					while (*pInv)
					{
						if (NStr::fg_StrCmpNoCase(pParseStart, *pInv, Len) == 0)
						{
							bValid = false;
						}
						++pInv;
					}
				}

				if (bValid)
					Output2.f_AddStr(pParseStart, Len);
				else
				{
					Output2.f_AddStr(pParseStart, Len);
					Output2.f_AddChar('$');
				}

				if (*pStr)
				{
					Output2.f_AddChar(*pStr);
					++pStr;						
				}
			}

			if (Output2.f_IsEmpty())
				Output2 = "_";
			else 
			{
				ch32 LastChar = Output2[Output2.f_GetLen()-1];
				if (LastChar == '.' || LastChar == ' ')
					Output2.f_AddChar('_');
			}

			Output = Output2;

			return Output;
		}
		bint CFile::fs_IsValidFilePath(const NStr::CStr &_File, EInvalidPathReason &_InvalidReason, NStr::CStr &_InvalidPart)
		{
			_InvalidReason = EInvalidPathReason_Valid;
			const ch8 *pInvalidChars = fs_GetInvalidFileNameChars(true);

			NStr::CStr File = _File;

			if (File == "")
			{
				_InvalidReason = EInvalidPathReason_Empty;
				return false;
			}

			const ch8 *pParse = File;
			while (*pParse)
			{
				if (*pParse >= 1 && *pParse <= 31)
				{
					_InvalidReason = EInvalidPathReason_ControlCharacters;
					return false;
				}
				const ch8 *pInv = pInvalidChars;
				while (*pInv)
				{
					if (*pParse == *pInv)
					{
						ch8 Temp[2];
						Temp[0] = *pInv;
						Temp[1] = 0;
						_InvalidReason = EInvalidPathReason_SpecificCharacter;
						_InvalidPart = Temp;
						return false;
					}
					++pInv;
				}
				++pParse;
			}
			const ch8 **pInvalidNames = fs_GetInvalidFileNameNames();

			pParse = File;
			while (*pParse)
			{
				const ch8 *pParseStart = pParse;

				while (*pParse && *pParse != '.' && *pParse != '/')
				{
					++pParse;
				}
				mint Len = pParse - pParseStart;
				if (Len)
				{
					const ch8 **pInv = pInvalidNames;
					while (*pInv)
					{
						if (NStr::fg_StrLen(*pInv) == Len && NStr::fg_StrCmpNoCase(pParseStart, *pInv, Len) == 0)
						{
							_InvalidReason = EInvalidPathReason_SpecificWord;
							_InvalidPart = *pInv;
							return false;
						}
						++pInv;
					}
				}

				if (*pParse)
					++pParse;
			}

			pParse = File;
			while (*pParse)
			{
				const ch8 *pParseStart = pParse;

				while (*pParse && *pParse != '/')
				{
					++pParse;
				}

				mint Len = pParse - pParseStart;
				if (Len)
				{
					if (pParse[-1] == ' ')
					{
						_InvalidReason = EInvalidPathReason_EndWithSpace;
						return false;
					}

					if (pParse[-1] == '.')
					{
						_InvalidReason = EInvalidPathReason_EndWithDot;
						return false;
					}

				}
				else
				{
					_InvalidReason = EInvalidPathReason_Empty;
					return false;
				}

				if (*pParse)
					++pParse;
			}

			if (pParse[-1] == '/')
			{			
				_InvalidReason = EInvalidPathReason_EndWithSlash;
				return false;
			}

			return true;
		}

		bint CFile::fs_IsValidFilePath(const NStr::CStr &_File, NStr::CStr &_Error)
		{
			EInvalidPathReason Reason;
			NStr::CStr InvalidPart;
			if (!fs_IsValidFilePath(_File, Reason, InvalidPart))
			{
				switch (Reason)
				{
				case EInvalidPathReason_Empty:
					{
						_Error = "be empty";
					}
					break;
				case EInvalidPathReason_ControlCharacters:
					{
						_Error = "contain characters between 1 and 31";
					}
					break;
				case EInvalidPathReason_SpecificCharacter:
					{
						_Error = NStr::CStr(NStr::CStr::CFormat("the character {}") << InvalidPart);
					}
					break;
				case EInvalidPathReason_SpecificWord:
					{
						_Error = NStr::CStr(NStr::CStr::CFormat("contain {} alone or with dots on both or either sides") << InvalidPart);
					}
					break;
				case EInvalidPathReason_EndWithSpace:
					{
						_Error = "end with a space";
					}
					break;
				case EInvalidPathReason_EndWithDot:
					{
						_Error = "end with a dot";
					}
					break;
				case EInvalidPathReason_EndWithSlash:
					{
						_Error = "end with a /";
					}
					break;
				default:
					DMibNeverGetHere;
					break;
				}
				return false;
			}
			return true;
		}
		
		bool CFile::fs_RemoveIncompatible
			(
				EFileAttrib _SourceAttribs
				, NStr::CStr const &_Source
				, NStr::CStr const &_Destination
				, NFunction::TCFunction<EDiffCopyChangeAction (CFile::EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
			)
		{
			bool bChanged = false;
			auto Attribs = CFile::fs_GetAttributes(_Destination);
			if (_SourceAttribs & EFileAttrib_Link)
			{
				if (!(Attribs & EFileAttrib_Link))
				{
					if (Attribs & EFileAttrib_Directory)
					{
						EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
						
						if (_OnChange)
							Action = _OnChange(CFile::EDiffCopyChange_DirectoryDeleted, _Source, _Destination, NStr::CStr());
						
						if (Action == EDiffCopyChangeAction_Perform)
						{
							bChanged = true;
							CFile::fs_DeleteDirectoryRecursive(_Destination, true);
						}
					}
					else
					{
						EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
						
						if (_OnChange)
							Action = _OnChange(CFile::EDiffCopyChange_FileDeleted, _Source, _Destination, NStr::CStr());
						
						if (Action == EDiffCopyChangeAction_Perform)
						{
							bChanged = true;
							CFile::fs_MakeFileWritable(_Destination, true);
							CFile::fs_DeleteFile(_Destination);
						}
					}
				}
				else
				{
					NStr::CStr SourceLink = CFile::fs_ResolveSymbolicLink(_Source);
					NStr::CStr DestinationLink = CFile::fs_ResolveSymbolicLink(_Destination);
					if ((Attribs & EFileAttrib_Directory) != (_SourceAttribs & EFileAttrib_Directory) || SourceLink != DestinationLink)
					{
						EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
						
						if (_OnChange)
							Action = _OnChange(CFile::EDiffCopyChange_LinkDeleted, _Source, _Destination, NStr::CStr());
						
						if (Action == EDiffCopyChangeAction_Perform)
						{
							bChanged = true;
							CFile::fs_MakeFileWritable(_Destination, true);
							CFile::fs_DeleteFile(_Destination);
						}
					}
				}
			}
			else
			{
				if (Attribs & EFileAttrib_Link)
				{
					EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
					
					if (_OnChange)
						Action = _OnChange(CFile::EDiffCopyChange_LinkDeleted, _Source, _Destination, NStr::CStr());
					
					if (Action == EDiffCopyChangeAction_Perform)
					{
						bChanged = true;
						CFile::fs_DeleteFile(_Destination);
					}
				}
				else if ((Attribs & EFileAttrib_Directory) != (_SourceAttribs & EFileAttrib_Directory))
				{
					if (Attribs & EFileAttrib_Directory)
					{
						EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
						
						if (_OnChange)
							Action = _OnChange(CFile::EDiffCopyChange_DirectoryDeleted, _Source, _Destination, NStr::CStr());
						
						if (Action == EDiffCopyChangeAction_Perform)
						{
							bChanged = true;
							CFile::fs_DeleteDirectoryRecursive(_Destination, true);
						}
					}
					else
					{
						EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
						
						if (_OnChange)
							Action = _OnChange(CFile::EDiffCopyChange_FileDeleted, _Source, _Destination, NStr::CStr());
						
						if (Action == EDiffCopyChangeAction_Perform)
						{
							bChanged = true;
							CFile::fs_MakeFileWritable(_Destination, true);
							CFile::fs_DeleteFile(_Destination);
						}
					}
				}
			}
			
			return bChanged;
		}
		
		bool CFile::fsp_DiffCopyFileOrDirectory
			(
				NStr::CStr const &_Source
				, NStr::CStr const &_Destination
				, NFunction::TCFunction<EDiffCopyChangeAction (EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
			)
		{
			auto SourceAttribs = CFile::fs_GetAttributes(_Source);
			bool bIsDirectory = (SourceAttribs & EFileAttrib_Directory) != 0;
			bool bIsLink = (SourceAttribs & EFileAttrib_Link) != 0;
			bool bChanged = false;
			if (bIsDirectory && !bIsLink)
			{
				if (CFile::fs_FileExists(_Destination, EFileAttrib_File))
				{
					EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
					
					if (_OnChange)
						Action = _OnChange(EDiffCopyChange_FileDeleted, _Source, _Destination, NStr::CStr());
					
					if (Action == EDiffCopyChangeAction_Perform)
					{
						CFile::fs_DeleteFile(_Destination);
						bChanged = true;
					}
				}
				CFile::fs_CreateDirectory(_Destination);
				
				NContainer::TCVector<CFile::CFoundFile> SourceFiles = CFile::fs_FindFilesEx(_Source + "/*", EFileAttrib_File, true, false);
				NContainer::TCVector<CFile::CFoundFile> SourceDirectories = CFile::fs_FindFilesEx(_Source + "/*", EFileAttrib_Directory, true, false);
				NContainer::TCVector<CFile::CFoundFile> ExistingFiles = CFile::fs_FindFilesEx(_Destination + "/*", EFileAttrib_File, true, false);
				NContainer::TCVector<CFile::CFoundFile> ExistingDirectories = CFile::fs_FindFilesEx(_Destination + "/*", EFileAttrib_Directory, true, false);
				
				NContainer::TCMap<NStr::CStr, CFile::CFoundFile> SourceFilesSet;
				for (auto iFile = SourceFiles.f_GetIterator(); iFile; ++iFile)
				{
					NStr::CStr Key = iFile->m_Path.f_Extract(_Source.f_GetLen() + 1);
					SourceFilesSet[Key] = *iFile;
				}
				NContainer::TCMap<NStr::CStr, CFile::CFoundFile> SourceDirectoriesSet;
				for (auto iFile = SourceDirectories.f_GetIterator(); iFile; ++iFile)
				{
					NStr::CStr Key = iFile->m_Path.f_Extract(_Source.f_GetLen() + 1);
					SourceDirectoriesSet[Key] = *iFile;
				}
				
				for (auto iFile = ExistingFiles.f_GetIterator(); iFile; ++iFile)
				{
					NStr::CStr File = iFile->m_Path.f_Extract(_Destination.f_GetLen() + 1);
					if (!SourceFilesSet.f_FindEqual(File))
					{
						EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
						if (_OnChange)
							Action = _OnChange(EDiffCopyChange_FileDeleted, NStr::CStr(), iFile->m_Path, NStr::CStr());
						
						if (Action == EDiffCopyChangeAction_Perform)
						{
							bChanged = true;
							CFile::fs_MakeFileWritable(iFile->m_Path, true);
							CFile::fs_DeleteFile(iFile->m_Path);
						}
					}
				}
				
				for (auto iFile = ExistingDirectories.f_GetIterator(); iFile; ++iFile)
				{
					NStr::CStr File = iFile->m_Path.f_Extract(_Destination.f_GetLen() + 1);
					if (!SourceDirectoriesSet.f_FindEqual(File))
					{
						if (CFile::fs_FileExists(iFile->m_Path, EFileAttrib_Directory))
						{
							EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
							
							if (_OnChange)
								Action = _OnChange(EDiffCopyChange_DirectoryDeleted, NStr::CStr(), iFile->m_Path, NStr::CStr());
							
							if (Action == EDiffCopyChangeAction_Perform)
							{
								bChanged = true;
								CFile::fs_DeleteDirectoryRecursive(iFile->m_Path, true);
							}
						}
					}
				}
				
				for (auto iFile = SourceDirectoriesSet.f_GetIterator(); iFile; ++iFile)
				{
					NStr::CStr Dest = CFile::fs_AppendPath(_Destination, iFile.f_GetKey());
					CFile::fs_CreateDirectory(CFile::fs_GetPath(Dest));
					if (CFile::fs_FileExists(Dest))
					{
						if (CFile::fs_RemoveIncompatible(iFile->m_Attribs, iFile->m_Path, Dest, _OnChange))
							bChanged = true;
					}
					
					if (iFile->m_Attribs & EFileAttrib_Link)
					{
						if (!CFile::fs_FileExists(Dest))
						{
							NStr::CStr Link = CFile::fs_ResolveSymbolicLink(iFile->m_Path);
							EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
							
							if (_OnChange)
								Action = _OnChange(EDiffCopyChange_LinkCreated, iFile->m_Path, Dest, Link);
							
							if (Action == EDiffCopyChangeAction_Perform)
							{
								bChanged = true;
								CFile::fs_CreateSymbolicLink(Link, Dest, (iFile->m_Attribs & (EFileAttrib_Directory | EFileAttrib_File)), ESymbolicLinkFlag_Relative);
							}
						}
					}
					else
					{
						if (!CFile::fs_FileExists(Dest, EFileAttrib_Directory))
						{
							EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
							
							if (_OnChange)
								Action = _OnChange(EDiffCopyChange_DirectoryCreated, iFile->m_Path, Dest, NStr::CStr());
							
							if (Action == EDiffCopyChangeAction_Perform)
							{
								bChanged = true;
								CFile::fs_CreateDirectory(Dest);
							}
						}
					}
				}
				
				for (auto iFile = SourceFilesSet.f_GetIterator(); iFile; ++iFile)
				{
					NStr::CStr Dest = CFile::fs_AppendPath(_Destination, iFile.f_GetKey());
					CFile::fs_CreateDirectory(CFile::fs_GetPath(Dest));
					if (CFile::fs_FileExists(Dest))
					{
						if (CFile::fs_RemoveIncompatible(iFile->m_Attribs, iFile->m_Path, Dest, _OnChange))
							bChanged = true;
					}
					
					if (iFile->m_Attribs & EFileAttrib_Link)
					{
						if (!CFile::fs_FileExists(Dest))
						{
							NStr::CStr Link = CFile::fs_ResolveSymbolicLink(iFile->m_Path);
							EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
							
							if (_OnChange)
								Action = _OnChange(EDiffCopyChange_LinkCreated, iFile->m_Path, Dest, Link);
							
							if (Action == EDiffCopyChangeAction_Perform)
							{
								bChanged = true;
								CFile::fs_CreateSymbolicLink(Link, Dest, (iFile->m_Attribs & (EFileAttrib_Directory | EFileAttrib_File)), ESymbolicLinkFlag_Relative);
							}
						}
					}
					else
					{
						if (CFile::fsp_CopyFileDiff(iFile->m_Path, Dest, true, _OnChange, true))
							bChanged = true;
					}
				}
			}
			else
			{
				// Remove old files
				if (CFile::fs_FileExists(_Destination))
				{
					if (CFile::fs_RemoveIncompatible(SourceAttribs, _Source, _Destination, _OnChange))
						bChanged = true;
				}
				
				CFile::fs_CreateDirectory(CFile::fs_GetPath(_Destination));
				if (SourceAttribs & EFileAttrib_Link)
				{
					if (!CFile::fs_FileExists(_Destination))
					{
						NStr::CStr Link = CFile::fs_ResolveSymbolicLink(_Source);
						EDiffCopyChangeAction Action = EDiffCopyChangeAction_Perform;
						
						if (_OnChange)
							Action = _OnChange(EDiffCopyChange_LinkCreated, _Source, _Destination, Link);
						
						if (Action == EDiffCopyChangeAction_Perform)
						{
							bChanged = true;
							CFile::fs_CreateSymbolicLink(Link, _Destination, (SourceAttribs & (EFileAttrib_Directory | EFileAttrib_File)), ESymbolicLinkFlag_Relative);
						}
					}
				}
				else
				{
					if (CFile::fsp_CopyFileDiff(_Source, _Destination, true, _OnChange, true))
						bChanged = true;
				}
			}
			
			return bChanged;
		}

		bool CFile::fs_DiffCopyFileOrDirectory
			(
				NStr::CStr const &_Source
				, NStr::CStr const &_Destination
				, NFunction::TCFunction<EDiffCopyChangeAction (EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
				, fp32 _Timeout
			)
		{
			NTime::CClock Clock;
			Clock.f_Start();
			while (1)
			{
				try
				{
					return fsp_DiffCopyFileOrDirectory(_Source, _Destination, _OnChange);
				}
				catch (CExceptionFile const &)
				{
					if (Clock.f_GetTime() >= _Timeout)
						throw;
					NSys::fg_Thread_Sleep(NMisc::fg_GetRandomFloat() * 0.100);
				}
			}
		}

		CLockFile::CLockFile()
		{
		}


		CLockFile::CLockFile(NStr::CStr const& _FilePath, NFile::EFileOpen _LockFlags)
		{
			f_SetLockFile(_FilePath, _LockFlags);
		}

		CLockFile::~CLockFile()
		{
			f_Unlock();
		}

		void CLockFile::f_SetLockFile(NStr::CStr const& _FilePath, NFile::EFileOpen _LockFlags)
		{
			f_Unlock();
			mp_FilePath = _FilePath;
			mp_LockFlags = _LockFlags;
		}

		NStr::CStr CLockFile::f_GetLockFile() const
		{
			return mp_FilePath;
		}

		CLockFile::ELockResult CLockFile::f_Lock(fp64 _TimeoutSeconds)
		{
			if (mp_FilePath.f_IsEmpty())
				DMibErrorFile("No lock file specified!");

			EFileRight RightsToCheck = EFileRight_None;

			if (mp_LockFlags & EFileOpen_Read)
				RightsToCheck |= EFileRight_Read;
			else if (mp_LockFlags & EFileOpen_Write)
				RightsToCheck |= EFileRight_Write;

			ECheckFileRights RightResult = CFile::fs_CheckFileRights(mp_FilePath, RightsToCheck);
			if (RightResult == ECheckFileRights_NoAccess)
				return ELockResult_NoAccess;
			else if (RightResult == ECheckFileRights_DoesNotExist)
			{
				if ( (mp_LockFlags & EFileOpen_Write) && !(mp_LockFlags & EFileOpen_DontCreate) )
				{
					// So we are going to attempt to create the file.
					RightResult = CFile::fs_CheckFileRights( CFile::fs_GetPath(mp_FilePath), EFileRight_Read | EFileRight_Write);
					if (RightResult == ECheckFileRights_NoAccess)
						return ELockResult_NoAccess;
					else if (RightResult == ECheckFileRights_DoesNotExist)
						return ELockResult_DoesNotExist;
				}
				else
				{
					return ELockResult_DoesNotExist;
				}					
			}


			NTime::CStopWatch Timer(true);

			while(1)
			{
				try
				{
					mp_LockFile.f_Open(mp_FilePath, mp_LockFlags);
					break;
				}
				catch(NException::CException)
				{
				}

				if (_TimeoutSeconds < 0.0)
					{}
				else if (_TimeoutSeconds == 0.0)
					break;
				else if (Timer.f_Mark().f_GetSecondsFraction() < _TimeoutSeconds)
					{}
				else
					break;

				NSys::fg_Thread_Sleep(fp64(0.1) + fp64(0.5) * NMisc::fg_GetRandomFloat());
			}

			return mp_LockFile.f_IsValid() ? ELockResult_Locked : ELockResult_TimedOut;
		}

		void CLockFile::f_Unlock()
		{
			if (mp_LockFile.f_IsValid())
				mp_LockFile.f_Close();
		}
		
		bint CLockFile::f_HasLock() const	// Do I have the lock?
		{
			return mp_LockFile.f_IsValid();			
		}

		aint CFileChangeNotifier::f_Main()
		{
			while (true)
			{
				if (m_Notification.f_IsOpen())
				{
					NMib::NFile::CFileChangeNotification::CNotification Notification;
					while (m_Notification.f_GetNotification(Notification))
					{
						m_Notify(Notification);
					}
				}
				if (f_GetState() == NThread::EThreadState_EventWantQuit)
					break;
				m_EventWantQuit.f_Wait();
			}
			return 0;
		}
		NMib::NStr::CStr CFileChangeNotifier::f_GetThreadName()
		{
			return "CFileChangeNotifier";
		}
		CFileChangeNotifier::CFileChangeNotifier(NMib::NStr::CStr const &_Path, NMib::NFile::EFileChange _OpenFlags, NFunction::TCFunction<void (CFileChangeNotification::CNotification const &_Change)> const &_Notify)
			: m_Notify(_Notify)
		{
			m_Notification.f_Open(_Path, _OpenFlags, &m_EventWantQuit);
			f_Start();
		}
		
		CFileChangeNotifier::~CFileChangeNotifier()
		{
			f_Stop();
		}	
	}
}
