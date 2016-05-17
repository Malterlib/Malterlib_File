// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/Cryptography/Hashes/MD5>

namespace NMib
{
	namespace NFile
	{
		class CFileProgress
		{
		public:

			virtual void f_Progress(CMibFilePos _Position, CMibFilePos _TotalSize) pure;
		};

		class CFileHandle
		{
		public:
			uint32 m_HandleID;
			uint32 m_ProcessID;
			NStr::CStr m_Process;
		};
	}

	namespace NSys
	{
		namespace NFile
		{
			NMib::NFile::EFileSystemFeature fg_GetFileSystemFeatures();
			inline_always bint fg_FileSystemHasDrives() { return fg_GetFileSystemFeatures() & NMib::NFile::EFileSystemFeature_HasDrives; } 
			inline_always bint fg_FileSystemHasExecuteAttrib() { return fg_GetFileSystemFeatures() & NMib::NFile::EFileSystemFeature_HasExecuteAttrib; }

			bint fg_FileExists(const NMib::NStr::CStr &_FileName, NMib::NFile::EFileAttrib _AttribMask);
			bint fg_FileExists(const NMib::NStr::CStrNonTracked &_FileName, NMib::NFile::EFileAttrib _AttribMask);

			NMib::NFile::ECheckFileRights fg_CheckFileRights( const NMib::NStr::CStr & _File, NMib::NFile::EFileRight _Rights);

			void *fg_Open(const NMib::NStr::CStr &_FileName, NMib::NFile::EFileOpen _OpenFlags, NMib::NFile::EFileAttrib _Attributes);
			void *fg_Open(const NMib::NStr::CStrNonTracked &_FileName, NMib::NFile::EFileOpen _OpenFlags, NMib::NFile::EFileAttrib _Attributes);
			void fg_Close(void *_pFile);
			void *fg_GetOSFile(void *_pFile);
			mint fg_Read(void *_pFile, void *_pData, const CMibFilePos &_Offset, mint _NumBytes); // Returns number of bytes read
			mint fg_Write(void *_pFile, const void *_pData, const CMibFilePos &_Offset, mint _NumBytes); // Returns number of bytes written
			void fg_Flush(void *_pFile); // Returns when all pending bytes are written to disk
			void fg_LockRange(void *_pFile, const CMibFilePos &_Offset, const CMibFilePos &_NumBytes, NMib::NFile::EFileLock _Flags);
			void fg_UnlockRange(void *_pFile, const CMibFilePos &_Offset, const CMibFilePos &_NumBytes);

			NMib::NFile::EFileAttrib fg_GetAttributes(void *_pFile);
			void fg_SetAttributes(void *_pFile, NMib::NFile::EFileAttrib _Attributes);

			NMib::NFile::EFileAttrib fg_GetAttributes(NMib::NStr::CStr const& _FileName);
			void fg_SetAttributes(NMib::NStr::CStr const& _FileName, NMib::NFile::EFileAttrib _Attributes);
			
			NMib::NFile::EFileAttrib fg_GetSupportedAttributes();
			NMib::NFile::EFileAttrib fg_GetValidAttributes();
			

			CMibFilePos fg_GetSize(void *_pFile);
			void fg_SetSize(void *_pFile, const CMibFilePos &_Size);

			CMibFilePos fg_GetSize(const NMib::NStr::CStr &_FileName);

			void fg_FileEnumOtherHandles(void *_pFile, NContainer::TCVector<NMib::NFile::CFileHandle> &_HandleInfo);
			void fg_FileEnumOtherHandles(const NMib::NStr::CStr &_FileName, NContainer::TCVector<NMib::NFile::CFileHandle> &_HandleInfo);

			void fg_SetCreationTime(void *_pFile, const NTime::CTime &_Time);
			void fg_SetAccessTime(void *_pFile, const NTime::CTime &_Time);
			void fg_SetWriteTime(void *_pFile, const NTime::CTime &_Time);
			NTime::CTime fg_GetCreationTime(void *_pFile);
			NTime::CTime fg_GetAccessTime(void *_pFile);
			NTime::CTime fg_GetWriteTime(void *_pFile);

			NTime::CTime fg_GetCreationTime(NMib::NStr::CStr const& _FileName);
			NTime::CTime fg_GetAccessTime(NMib::NStr::CStr const& _FileName);
			NTime::CTime fg_GetWriteTime(NMib::NStr::CStr const& _FileName);
			
			NMib::NStr::CStr fg_GetOwner(const NMib::NStr::CStr &_Path);
			NMib::NStr::CStr fg_GetGroup(const NMib::NStr::CStr &_Path);
			NMib::NStr::CStr fg_GetOwnerOnLink(const NMib::NStr::CStr &_Path);
			NMib::NStr::CStr fg_GetGroupOnLink(const NMib::NStr::CStr &_Path);
			void fg_SetOwner(const NMib::NStr::CStr &_Path, const NMib::NStr::CStr &_Owner);
			void fg_SetOwnerOnLink(const NMib::NStr::CStr &_Path, const NMib::NStr::CStr &_Owner);
			void fg_SetGroup(const NMib::NStr::CStr &_Path, const NMib::NStr::CStr &_Group);
			void fg_SetGroupOnLink(const NMib::NStr::CStr &_Path, const NMib::NStr::CStr &_Group);
			void fg_SetOwner(void *_pFile, const NMib::NStr::CStr &_Owner);
			void fg_SetGroup(void *_pFile, const NMib::NStr::CStr &_Group);

			void *fg_FindOpen(const NMib::NStr::CStr &_FindPattern);
			const NMib::NStr::CStr *fg_FindNext(void *_pFindContext, NMib::NFile::EFileAttrib &_FileAttribs); // Returns the fully qualified path. Returns nullptr when no more files are found
			void fg_FindClose(void *_pFindContext);

			void fg_Copy(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo);
			void fg_Copy(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo, NMib::NFile::CFileProgress &_Progress);
			void fg_Rename(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo);
			void fg_Rename(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo, NMib::NFile::CFileProgress &_Progress);

			void fg_CreateSymbolicLink(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo, NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags);
			void fg_CreateHardLink(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo);
			NMib::NStr::CStr fg_ResolveSymbolicLink(const NMib::NStr::CStr &_FileFrom);
			bool fg_CanCreateSymbolicLink(NMib::NFile::EFileAttrib _Type, NMib::NFile::ESymbolicLinkFlag _Flags);

			void fg_Delete(const NMib::NStr::CStr &_File);
			void fg_DeleteDirectory(const NMib::NStr::CStr &_File);
			void fg_CreateDirectory(const NMib::NStr::CStr &_FileDirectory);
			void fg_Delete(const NMib::NStr::CStrNonTracked &_File);
			void fg_DeleteDirectory(const NMib::NStr::CStrNonTracked &_File);
			void fg_CreateDirectory(const NMib::NStr::CStrNonTracked &_FileDirectory);

			NMib::NStream::CFilePos fg_GetFreeSpace(const NMib::NStr::CStr &_Path);
			NMib::NStream::CFilePos fg_GetUsedSpace(const NMib::NStr::CStr &_Path);

			void fg_SetCurrentDirectory(const NMib::NStr::CStr &_Directory);

			NMib::NStr::CStr fg_GetProgramPath();
			NMib::NStr::CStr fg_GetProgramDirectory();
			NMib::NStr::CStr fg_GetCurrentDirectory();
			NMib::NStr::CStr fg_GetUserProgramDirectory();
			NMib::NStr::CStr fg_GetUserLocalProgramDirectory();
			NMib::NStr::CStr fg_GetUserLocalProgramCacheDirectory();
			NMib::NStr::CStr fg_GetTemporaryDirectory();
			NMib::NStr::CStr fg_GetModulePath(void *_pCode);
            NMib::NStr::CStr fg_GetUserHomeDirectory();
            NMib::NStr::CStr fg_GetLogDirectory();

			NMib::NStr::CStrNonTracked fg_GetProgramPathNonTracked();
			NMib::NStr::CStrNonTracked fg_GetProgramDirectoryNonTracked();
			NMib::NStr::CStrNonTracked fg_GetCurrentDirectoryNonTracked();
			NMib::NStr::CStrNonTracked fg_GetUserProgramDirectoryNonTracked();
			NMib::NStr::CStrNonTracked fg_GetUserLocalProgramDirectoryNonTracked();
			NMib::NStr::CStrNonTracked fg_GetUserLocalProgramCacheDirectoryNonTracked();
			NMib::NStr::CStrNonTracked fg_GetTemporaryDirectoryNonTracked();
			NMib::NStr::CStrNonTracked fg_GetModulePathNonTracked(void *_pCode);
            NMib::NStr::CStrNonTracked fg_GetUserHomeDirectoryNonTracked();
            NMib::NStr::CStrNonTracked fg_GetLogDirectoryNonTracked();
            
			ch8 const *fg_GetDllExtension();

//			NMib::NStr::CStr fg_GetProgramRootDirectory();		// This defaults to the program directory but can be changed via fg_SetProgramRootDirectory.
//			void fg_SetProgramRootDirectory(NMib::NStr::CStr const& _Root);

			void *fg_ChangeNotification_Open(const NMib::NStr::CStr &_FileName, NMib::NFile::EFileChange _OpenFlags, NMib::NThread::CSemaphoreReportableAggregate *_pReportTo);
			void fg_ChangeNotification_Close(void *_pNotification);
			bint fg_ChangeNotification_Changed(void *_pNotification);
			bint fg_ChangeNotification_GetNotification(void *_pNotification, NMib::NStr::CStr &_Path, NMib::NFile::EFileChangeNotification &_Notification);
		}
	}

	namespace NFile
	{
		/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		|	Class:				A file exception										|
		\*_____________________________________________________________________________*/
		class CFileChangeNotification
		{
			void *mp_pNotification;
		public:

			CFileChangeNotification()
			{
				mp_pNotification = nullptr;
			}

			~CFileChangeNotification()
			{
				f_Close();
			}

			void f_Close()
			{
				if (mp_pNotification)
				{
					NSys::NFile::fg_ChangeNotification_Close(mp_pNotification);
					mp_pNotification = nullptr;
				}
			}
			
			void f_Open(const NStr::CStr &_Path, EFileChange _OpenFlags, NMib::NThread::CSemaphoreReportableAggregate *_pReportTo)
			{
				f_Close();
				mp_pNotification = NSys::NFile::fg_ChangeNotification_Open(_Path, _OpenFlags, _pReportTo);
			}

			bint f_IsOpen()
			{
				return mp_pNotification != nullptr;
			}

			bint f_Changed()
			{
				return NSys::NFile::fg_ChangeNotification_Changed(mp_pNotification);
			}

			class CNotification
			{
			public:
				EFileChangeNotification m_Notification;
				NStr::CStr m_Path;
			};

			bint f_GetNotification(CNotification &_ToFill)
			{
				if (!f_IsOpen())
					DMibErrorFile("Notification not open");
				if (NSys::NFile::fg_ChangeNotification_GetNotification(mp_pNotification, _ToFill.m_Path, _ToFill.m_Notification))
					return true;
				return false;
			}
		};
		
		class CFileChangeNotifier : NThread::CThread
		{
			CFileChangeNotification m_Notification;
			NFunction::TCFunction<void (CFileChangeNotification::CNotification const &_Change)> m_Notify;
			aint f_Main();
			NMib::NStr::CStr f_GetThreadName();
		public:
			CFileChangeNotifier(NMib::NStr::CStr const &_Path, EFileChange _OpenFlags, NFunction::TCFunction<void (CFileChangeNotification::CNotification const &_Change)> const &_Notify);			
			~CFileChangeNotifier();
		};

		class CFile
		{
		public:
			struct CFoundFile
			{
				NStr::CStr m_Path;
				EFileAttrib m_Attribs; 
				
				template <typename tf_CStream>
				void f_Feed(tf_CStream &_Stream) const
				{
					_Stream << m_Path;
					_Stream << m_Attribs;
				}
				
				template <typename tf_CStream>
				void f_Consume(tf_CStream &_Stream)
				{
					_Stream >> m_Path;
					_Stream >> m_Attribs;
				}
				
				bool operator == (CFoundFile const &_Other) const
				{
					if (m_Path != _Other.m_Path)
						return false;
					return m_Attribs == _Other.m_Attribs;
				}

				bool operator < (CFoundFile const &_Other) const
				{
					if (m_Path < _Other.m_Path)
						return true;
					if (m_Path > _Other.m_Path)
						return false;
					return m_Attribs < _Other.m_Attribs;
				}
			};

			struct CFindFilesOptions
			{
				NStr::CStr m_Path;
				EFileAttrib m_AttribMask = EFileAttrib_Directory | EFileAttrib_File;
				bool m_bRecursive;
				bool m_bFollowLinks = false;
				NContainer::TCVector<NStr::CStr> m_ExcludePatterns;

				CFindFilesOptions(NStr::CStr const &_Path, bool _bRecursive)
					: m_Path(_Path)
					, m_bRecursive(_bRecursive)
				{
				}
			};

			enum EDiffCopyChange
			{
				EDiffCopyChange_DirectoryDeleted
				, EDiffCopyChange_DirectoryCreated
				, EDiffCopyChange_FileDeleted
				, EDiffCopyChange_FileCreated
				, EDiffCopyChange_FileChanged
				, EDiffCopyChange_LinkDeleted
				, EDiffCopyChange_LinkCreated
				, EDiffCopyChange_NoChange
			};
			enum EDiffCopyChangeAction
			{
				EDiffCopyChangeAction_Perform
				, EDiffCopyChangeAction_Skip
			};
					
		private:
			void *mp_pFile;
			CMibFilePos mp_FilePos;
			EFileOpen mp_OpenFlags;

			CMibFilePos mp_CachePos;
			bint mp_bCacheDirty = false;
			bint mp_bNonTracked = false;
			CMibFilePos mp_CachedFileLen;
			NContainer::TCVector<uint8> mp_CacheBuffer;
			NContainer::TCVector<uint8, NMem::CAllocator_NonTrackedHeap> mp_CacheBufferNonTracked;

			uint8 *fp_GetCacheBuffer();
			mint fp_GetCacheBufferLen();

			enum
			{
				EDefaultCacheSize = 16*1024
			};

			inline_small void fp_CheckOpen() const
			{
				if (!mp_pFile)
					DMibErrorFile("File not open");
			}

			void fp_FlushCache();
			void *fp_GetCache(CMibFilePos _CurrentPos, mint &_nBytesAvailable);
			CMibFilePos fp_GetLength() const;

			static bint fsp_OpenFile(NFile::CFile &_File, const NStr::CStr &_FileName, EFileOpen _Open, const NTime::CTime &_FileTime);
			static NTime::CTimeSpan fsp_GetDefaultFileChangedMargin();

			static bool fsp_DiffCopyFileOrDirectory
				(
					NStr::CStr const &_Source
					, NStr::CStr const &_Destination
					, NFunction::TCFunction<EDiffCopyChangeAction (EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
				)
			;
			static bint fsp_CopyFileDiff
					(
						const NContainer::TCVector<uint8> &_SourceData
						, const NStr::CStr &_FromFileName
						, const NStr::CStr &_ToFileName
						, const NTime::CTime &_FileTime
						, EFileAttrib _AddAttribs
						, NFunction::TCFunction<EDiffCopyChangeAction (EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
						, bool _bRemoveWriteProtection = false
					)
			;
			static bint fsp_CopyFileDiffDate
				(
					const NContainer::TCVector<uint8> &_SourceData
					, const NStr::CStr &_ToFileName
					, const NTime::CTime &_FileTime
					, EFileAttrib _AddAttribs
				)
			;
			static bint fsp_CopyFileDiff
				(
					const NStr::CStr &_FromFileName
					, const NStr::CStr &_ToFileName
					, bool _bCopyDate
					, NFunction::TCFunction<EDiffCopyChangeAction (EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
					, bool _bRemoveWriteProtection = false
				)
			;
			static bint fsp_FileIsSame(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName);

		public:
			CFile();
			CFile(const NStr::CStr &_FileName, EFileOpen _OpenFlags);
			CFile(const NStr::CStrNonTracked &_FileName, EFileOpen _OpenFlags);
			~CFile();

			bint f_IsValid() const;

			void f_Close(bint _bCanThrow = true);
			void f_Open(const NStr::CStr &_FileName, EFileOpen _OpenFlags, EFileAttrib _Attributes = EFileAttrib_None);
			void f_Open(const NStr::CStrNonTracked &_FileName, EFileOpen _OpenFlags, EFileAttrib _Attributes = EFileAttrib_None);
			enum ESysFileTag
			{
			};
			void f_Open(void *_pFile, ESysFileTag, EFileOpen _OpenFlags);
			
			
			void f_Read(void *_pDest, mint _nBytes);
			void f_Write(const void *_pSrc, mint _nBytes);

			void f_FileEnumOtherHandles(NContainer::TCVector<CFileHandle> &_HandleInfo);

			static void fs_FileEnumOtherHandles(const NStr::CStr &_FileName, NContainer::TCVector<CFileHandle> &_HandleInfo);

			void f_FlushCache();

			void f_SetCacheSize(mint _CacheSize);

			void *f_GetOSFile() const;

			void f_Flush(bint _bLocalCacheOnly);
			bint f_IsAtEndOfFile() const;
			void f_SetLength(const CMibFilePos &_Length);
			CMibFilePos f_GetLength() const;
			void f_SetCreationTime(const NTime::CTime &_Time);
			void f_SetAccessTime(const NTime::CTime &_Time);
			void f_SetWriteTime(const NTime::CTime &_Time);
			NTime::CTime f_GetCreationTime();
			NTime::CTime f_GetAccessTime();
			NTime::CTime f_GetWriteTime();
			EFileAttrib f_GetAttributes();
			void f_SetAttributes(EFileAttrib _Attribs);
			void f_SetPosition(NStream::CFilePos _Pos);
			void f_SetPositionFromEnd(NStream::CFilePos _Pos);
			void f_AddPosition(NStream::CFilePos _Pos);
			bint f_IsValidReadPosition(NStream::CFilePos _Pos) const;
			void f_LockRange(const CMibFilePos &_Offset, const CMibFilePos &_NumBytes, EFileLock _Flags);
			void f_UnlockRange(const CMibFilePos &_Offset, const CMibFilePos &_NumBytes);
			NStream::CFilePos f_GetPosition() const;
			void f_SetOwner(NStr::CStr const & _Owner);
			void f_SetGroup(NStr::CStr const & _Group);

			// Static functions
			static bint fs_MakeFileWritable(const NStr::CStr &_LocalPath, bint _bWritable = true);
			static bint fs_IsFileWritable(const NStr::CStr &_LocalPath);
			static void fs_DeleteDirectoryRecursive(const NStr::CStr &_File, bint _bRemoveWriteProtection = false);
			static void fs_CopyFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress);
			static void fs_CopyFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo);
			static void fs_CopyFileRaw(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo);

			static void fs_CreateSymbolicLink(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo, EFileAttrib _Type, ESymbolicLinkFlag _Flags);
			static void fs_CreateHardLink(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo);
			static NMib::NStr::CStr fs_ResolveSymbolicLink(const NMib::NStr::CStr &_FileFrom);
			static bool fs_CanCreateSymbolicLink(EFileAttrib _Type, ESymbolicLinkFlag _Flags);


			static void fs_WriteFile(NContainer::TCVector<uint8> const &_FileFrom, const NStr::CStr &_FileTo);
			static void fs_CopyFiles(const NStr::CStr &_FindPath, const NStr::CStr &_ToPath, bint _bRecursive = true, bint _bRaw = false, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File);
			static bint fs_FileIsSame(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName);
			static NContainer::TCVector<uint8> fs_ReadFileTry(const NStr::CStr &_ToFileName);
			static bint fs_CopyFileDiff(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs = EFileAttrib_None);
			static bint fs_CopyFileDiffDate(const NContainer::TCVector<uint8> &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs = EFileAttrib_None);
			static bint fs_CopyFileDiff(const NStr::CStr &_FromFileName, const NStr::CStr &_ToFileName, bint _bCopyDate);
			static void fs_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo);
			static void fs_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress);
			static void fs_SetCurrentDirectory(const NStr::CStr &_Directory);

			static void fs_CreateDirectory(const NStr::CStr &_Path);
			static void fs_DeleteFile(const NStr::CStr &_File);
			static void fs_DeleteDirectory(const NStr::CStr &_File);

			static void fs_CreateDirectory(const NStr::CStrNonTracked &_Path);
			static void fs_DeleteFile(const NStr::CStrNonTracked &_File);
			static void fs_DeleteDirectory(const NStr::CStrNonTracked &_File);

			static bool fs_RemoveIncompatible
				(
					EFileAttrib _SourceAttribs
					, NStr::CStr const &_Source
					, NStr::CStr const &_Destination
					, NFunction::TCFunction<EDiffCopyChangeAction (CFile::EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
				)
			;
			
			static bool fs_DiffCopyFileOrDirectory
				(
					NStr::CStr const &_Source
					, NStr::CStr const &_Destination
					, NFunction::TCFunction<EDiffCopyChangeAction (EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
					, fp32 _Timeout = 30.0f
				)
			;

			static ch8 const *fs_GetDllExtension();
			
			static NStr::CStr fs_GetProgramPath();
			static NStr::CStr fs_GetProgramDirectory();
			static NStr::CStr fs_GetCurrentDirectory();
			static NStr::CStr fs_GetUserProgramDirectory();
			static NStr::CStr fs_GetUserLocalProgramDirectory();
			static NStr::CStr fs_GetUserLocalProgramCacheDirectory();
			static NStr::CStr fs_GetTemporaryDirectory();
			static NStr::CStr fs_GetModulePath(void *_pCode);
			static NStr::CStr fs_GetProgramRoot(); // This can be set via CSystem::f_SetApplicationRoot(). Defaults to program directory.
			static NStr::CStr fs_GetUserHomeDirectory();
			static NStr::CStr fs_GetLogDirectory();
			
			static NStr::CStrNonTracked fs_GetProgramPathNonTracked();
			static NStr::CStrNonTracked fs_GetProgramDirectoryNonTracked();
			static NStr::CStrNonTracked fs_GetCurrentDirectoryNonTracked();
			static NStr::CStrNonTracked fs_GetUserProgramDirectoryNonTracked();
			static NStr::CStrNonTracked fs_GetUserLocalProgramDirectoryNonTracked();
			static NStr::CStrNonTracked fs_GetUserLocalProgramCacheDirectoryNonTracked();
			static NStr::CStrNonTracked fs_GetTemporaryDirectoryNonTracked();
			static NStr::CStrNonTracked fs_GetModulePathNonTracked(void *_pCode);
			static NStr::CStrNonTracked fs_GetProgramRootNonTracked(); // This can be set via CSystem::f_SetApplicationRoot(). Defaults to program directory.
	        static NStr::CStrNonTracked fs_GetUserHomeDirectoryNonTracked();
			static NStr::CStrNonTracked fs_GetLogDirectoryNonTracked();

			template <typename tf_CStr>
			static typename TCEnableIf<NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetProgramPath()
			{
				return fs_GetProgramPathNonTracked();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<!NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetProgramPath()
			{
				return fs_GetProgramPath();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetProgramDirectory()
			{
				return fs_GetProgramDirectoryNonTracked();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<!NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetProgramDirectory()
			{
				return fs_GetProgramDirectory();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetCurrentDirectory()
			{
				return fs_GetCurrentDirectoryNonTracked();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<!NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetCurrentDirectory()
			{
				return fs_GetCurrentDirectory();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetUserProgramDirectory()
			{
				return fs_GetUserProgramDirectoryNonTracked();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<!NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetUserProgramDirectory()
			{
				return fs_GetUserProgramDirectory();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetUserLocalProgramDirectory()
			{
				return fs_GetUserLocalProgramDirectoryNonTracked();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<!NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetUserLocalProgramDirectory()
			{
				return fs_GetUserLocalProgramDirectory();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetTemporaryDirectory()
			{
				return fs_GetTemporaryDirectoryNonTracked();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<!NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetTemporaryDirectory()
			{
				return fs_GetTemporaryDirectory();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetModulePath(void *_pCode)
			{
				return fs_GetModulePathNonTracked(_pCode);
			}
			template <typename tf_CStr>
			static typename TCEnableIf<!NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetModulePath(void *_pCode)
			{
				return fs_GetModulePath(_pCode);
			}
			template <typename tf_CStr>
			static typename TCEnableIf<NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetProgramRoot()
			{
				return fs_GetProgramRootNonTracked();
			}
			template <typename tf_CStr>
			static typename TCEnableIf<!NTraits::TCIsSame<typename tf_CStr::CAllocator, NMem::CAllocator_NonTrackedHeap>::mc_Value, tf_CStr>::CType fs_GetProgramRoot()
			{
				return fs_GetProgramRoot();
			}


			static NMib::NStream::CFilePos fs_GetFreeSpace(const NMib::NStr::CStr &_Path);
			static NMib::NStream::CFilePos fs_GetUsedSpace(const NMib::NStr::CStr &_Path);
			static void fs_FindFilesGeneral
				(
					const NStr::CStr &_FindPath
					, NFunction::TCFunction<bool (NStr::CStr const &_FileName, EFileAttrib _Attribs, bool _bPreRecurse)> const &_fFoundFile // Return true to recurse into directory
					, bool _bRecursive = false
				)
			;
			static NContainer::TCVector<NStr::CStr>
			fs_FindFiles(const NStr::CStr &_FindPath, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File, bint _bRecursive = false, bool _bFollowLinks = true);
			static NContainer::TCVector<CFoundFile>
			fs_FindFilesEx(const NStr::CStr &_FindPath, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File, bint _bRecursive = false, bool _bFollowLinks = true);
			static NContainer::TCVector<CFoundFile> fs_FindFiles(CFindFilesOptions const &_Options);

			static bint fs_FileExists(const NStr::CStr &_File, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File);
			static bint fs_FileExists(const NStr::CStrNonTracked &_File, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File);
			static ECheckFileRights fs_CheckFileRights(NStr::CStr const& _File, EFileRight _Rights);
			static NDataProcessing::CHashDigest_MD5 fs_GetFileChecksum(const NStr::CStr &_Path);
			static NDataProcessing::CHashDigest_MD5 fs_GetDirectoryChecksum
				(
					const NStr::CStr &_Path
					, NFunction::TCFunction<bint (NStr::CStr const &_File)> const &_ExcludeFile
					, NFunction::TCFunction<NDataProcessing::CHashDigest_MD5 (NStr::CStr const &_File)> const &_GetHash = fg_Default()
				)
			;
			static void fs_WriteFile(const NStr::CStr &_Path, NContainer::TCVector<uint8> const &_Data);
			static NContainer::TCVector<uint8> fs_ReadFile(const NStr::CStr &_Path);
			static const ch8 *fs_GetInvalidFileNameChars(bint _bPath);
			static const ch8 **fs_GetInvalidFileNameNames();
			static void fs_SetAttributes(NStr::CStr const &_FileName, EFileAttrib _Attribs);
			static EFileAttrib fs_GetAttributes(NStr::CStr const &_FileName);
			
			static NTime::CTime fs_GetCreationTime(NStr::CStr const& _FileName);
			static NTime::CTime fs_GetAccessTime(NStr::CStr const& _FileName);
			static NTime::CTime fs_GetWriteTime(NStr::CStr const& _FileName);
			
			static CMibFilePos fs_GetFileSize(const NStr::CStr &_Path);

			static EFileAttrib fs_GetSupportedAttributes();
			static EFileAttrib fs_GetValidAttributes();

			static NStr::CStr fs_GetOwner(NStr::CStr const &_Path);
			static NStr::CStr fs_GetGroup(NStr::CStr const &_Path);
			static NStr::CStr fs_GetOwnerOnLink(NStr::CStr const &_Path);
			static NStr::CStr fs_GetGroupOnLink(NStr::CStr const &_Path);
			static void fs_SetOwner(NStr::CStr const &_Path, NStr::CStr const &_Owner);
			static void fs_SetGroup(NStr::CStr const &_Path, NStr::CStr const &_Group);
			static void fs_SetOwnerOnLink(NStr::CStr const &_Path, NStr::CStr const &_Owner);
			static void fs_SetGroupOnLink(NStr::CStr const &_Path, NStr::CStr const &_Group);
			static bool fs_SetOwnerRecursive(NStr::CStr const &_Path, NStr::CStr const &_Owner, bool _bFollowLinks = false);
			static bool fs_SetGroupRecursive(NStr::CStr const &_Path, NStr::CStr const &_Group, bool _bFollowLinks = false);
			static bool fs_SetOwnerAndGroupRecursive(NStr::CStr const &_Path, NStr::CStr const &_Owner, NStr::CStr const &_Group, bool _bFollowLinks = false);

			static void fs_WriteStringToVector(NContainer::TCVector<uint8> &_File, const NStr::CStr &_ToWrite, bint _bAddBOM = true);
			static NStr::CStr fs_ReadStringFromVector(const NContainer::TCVector<uint8> &_File, bool _bAssumeUTF8 = false);

			static void fs_WriteStringToFile(const NStr::CStr &_Path, const NStr::CStr &_ToWrite, bint _bAddBOM = true, EFileAttrib _Attributes = EFileAttrib_None);
			static NStr::CStr fs_ReadStringFromFile(const NStr::CStr &_Path, bool _bAssumeUTF8 = false);

			static void fs_WriteStringToFile(const NStr::CStrNonTracked &_Path, const NStr::CStrNonTracked &_ToWrite, bint _bAddBOM = true, EFileAttrib _Attributes = EFileAttrib_None);
			static NStr::CStrNonTracked fs_ReadStringFromFile(const NStr::CStrNonTracked &_Path, bool _bAssumeUTF8 = false);

			static NStr::CStr fs_CondensePath(NStr::CStr _Path);
			static NStr::CStr fs_MakeNiceFilename(const NStr::CStr &_CurrentName);

			template <typename tf_CStr>
			static tf_CStr fs_GetFullPath(const tf_CStr &_Path, const tf_CStr &_Base);
			template <typename tf_CStr>
			static tf_CStr fs_GetExpandedPath(const tf_CStr &_Path, bint _bAddCurrentDir = true);
			template <typename tf_CStr>
			static tf_CStr fs_GetExpandedPath(const tf_CStr &_Path, const tf_CStr &_BasePath);

			template <typename tf_CStr>
			static bint fs_IsPathAbsolute(tf_CStr _Path);
			template <typename tf_CStr>
			static tf_CStr fs_MakePathRelative(tf_CStr const& _AbsolutePath, tf_CStr const& _AbsoluteBase);

			template <typename tf_CStr>
			static tf_CStr fs_GetCommonPathAndMakeRelative(tf_CStr & _oPath0, tf_CStr & _oPath1);
			template <typename tf_CStr>
			static tf_CStr fs_GetCommonPath(tf_CStr const& _oPath0, tf_CStr const& _oPath1);

			template <typename tf_CStr>
			static tf_CStr fs_GetPath(const tf_CStr &_File);
			template <typename tf_CStr>
			static tf_CStr fs_GetMalterlibPath(const tf_CStr &_File);
			template <typename tf_CStr, typename tf_CToAppend>
			static tf_CStr fs_AppendPath(const tf_CStr &_Path, tf_CToAppend &&_Append);
			template <typename tf_CStr>
			static tf_CStr fs_RemovePathTopLevels(const tf_CStr &_Path, mint _Levels);
			template <typename tf_CStr>
			static tf_CStr fs_GetFile(const tf_CStr &_File);
			template <typename tf_CStr>
			static tf_CStr fs_GetExtension(const tf_CStr &_File);
			template <typename tf_CStr>
			static tf_CStr fs_GetDrive(const tf_CStr &_File);
			template <typename tf_CStr>
			static tf_CStr fs_GetFileNoExt(const tf_CStr &_File);



		private:
			static auto fsp_EncodeChar(ch32 _Char) -> NStr::CFWStr16;
			static auto fsp_EncodeString(ch32 *_pChars, mint _Len) -> NStr::CUStr;
		public:
			static NStr::CStr fs_MakeNiceUniqueFilename(const NStr::CStr &_CurrentName);

			enum EInvalidPathReason
			{
				EInvalidPathReason_Valid
				, EInvalidPathReason_Empty
				, EInvalidPathReason_ControlCharacters
				, EInvalidPathReason_SpecificCharacter // Character returned separately
				, EInvalidPathReason_SpecificWord // Word returned separately
				, EInvalidPathReason_EndWithSpace
				, EInvalidPathReason_EndWithDot
				, EInvalidPathReason_EndWithSlash
			};

			static bint fs_IsValidFilePath(const NStr::CStr &_File, EInvalidPathReason &_InvalidReason, NStr::CStr &_InvalidPart);
			static bint fs_IsValidFilePath(const NStr::CStr &_File, NStr::CStr &_Error);

		};	

		template <typename t_CStreamType = NStream::CBinaryStreamDefault>
		class TCBinaryStreamFile : public t_CStreamType
		{
		protected:
			DMibStreamImplementProtected(TCBinaryStreamFile);
		public:
			DMibStreamImplementOperators(TCBinaryStreamFile);

			CFile m_File;

			TCBinaryStreamFile()
			{
			}

			template <typename tf_CStr>
			void f_Open(const tf_CStr &_FileName, EFileOpen _OpenFlags)
			{
				m_File.f_Open(_FileName, _OpenFlags);
			}

			void f_Close()
			{
				m_File.f_Close();
			}

			void f_Reset()
			{
				m_File.f_Flush(false);
				m_File.f_SetLength(0);
			}

			void f_FeedBytes(const void *_pMem, mint _nBytes)
			{
				m_File.f_Write(_pMem, _nBytes);
			}

			void f_ConsumeBytes(void *_pMem, mint _nBytes)
			{
				m_File.f_Read(_pMem, _nBytes);
			}

			bint f_IsValid() const
			{
				return m_File.f_IsValid();
			}

			bint f_IsAtEndOfStream() const
			{
				return m_File.f_IsAtEndOfFile();
			}

			NStream::CFilePos f_GetPosition() const
			{
				return m_File.f_GetPosition();
			}

			void f_SetPosition(NStream::CFilePos _Pos)
			{
				m_File.f_SetPosition(_Pos);
			}

			void f_SetPositionFromEnd(NStream::CFilePos _Pos)
			{
				m_File.f_SetPositionFromEnd(_Pos);
			}

			void f_AddPosition(NStream::CFilePos _Pos)
			{
				m_File.f_AddPosition(_Pos);
			}

			bint f_IsValidReadPosition(NStream::CFilePos _Pos) const
			{
				return m_File.f_IsValidReadPosition(_Pos);
			}

			void f_Flush(bint _bLocalCacheOnly)
			{
				return m_File.f_Flush(_bLocalCacheOnly);
			}

			void f_SetCacheSize(mint _CacheSize)
			{
				m_File.f_SetCacheSize(_CacheSize);
			}

			NStream::CFilePos f_GetLength() const
			{
				return m_File.f_GetLength();
			}

			void f_SetLength(NStream::CFilePos _Length) 
			{ 
				return m_File.f_SetLength(_Length);
			}

		};
		
		template <typename t_CStreamType = NStream::CBinaryStreamDefault>
		class TCBinaryStreamFilePtr : public t_CStreamType
		{
			CFile *mp_pFile;
		protected:
			DMibStreamImplementProtected(TCBinaryStreamFilePtr);
		public:
			DMibStreamImplementOperators(TCBinaryStreamFilePtr);

			TCBinaryStreamFilePtr()
			{
				mp_pFile = nullptr;
			}

			TCBinaryStreamFilePtr(CFile *_pFile)
			{
				mp_pFile = _pFile;
			}

			TCBinaryStreamFilePtr(CFile &_File)
			{
				mp_pFile = &_File;
			}

			void f_Open(CFile *_pFile)
			{
				mp_pFile = _pFile;
			}
			
			void f_Open(CFile &_File)
			{
				mp_pFile = &_File;
			}
			void f_Close()
			{
				mp_pFile = nullptr;
			}

			void f_Reset()
			{
				mp_pFile->f_SetLength(0);
				mp_pFile->f_Flush(false);
			}

			void f_FeedBytes(const void *_pMem, mint _nBytes)
			{
				mp_pFile->f_Write(_pMem, _nBytes);
			}

			void f_ConsumeBytes(void *_pMem, mint _nBytes)
			{
				mp_pFile->f_Read(_pMem, _nBytes);
			}

			bint f_IsValid() const
			{
				return mp_pFile->f_IsValid();
			}

			bint f_IsAtEndOfStream() const
			{
				return mp_pFile->f_IsAtEndOfFile();
			}

			NStream::CFilePos f_GetPosition() const
			{
				return mp_pFile->f_GetPosition();
			}

			void f_SetPosition(NStream::CFilePos _Pos)
			{
				mp_pFile->f_SetPosition(_Pos);
			}

			void f_SetPositionFromEnd(NStream::CFilePos _Pos)
			{
				mp_pFile->f_SetPositionFromEnd(_Pos);
			}

			void f_AddPosition(NStream::CFilePos _Pos)
			{
				mp_pFile->f_AddPosition(_Pos);
			}

			bint f_IsValidReadPosition(NStream::CFilePos _Pos) const
			{
				return mp_pFile->f_IsValidReadPosition(_Pos);
			}

			void f_Flush(bint _bLocalCacheOnly)
			{
				mp_pFile->f_Flush(_bLocalCacheOnly);
			}

			void f_SetCacheSize(mint _CacheSize)
			{
				mp_pFile->f_SetCacheSize(_CacheSize);
			}

			NStream::CFilePos f_GetLength() const
			{
				return mp_pFile->f_GetLength();
			}

			void f_SetLength(NStream::CFilePos _Length) 
			{ 
				return mp_pFile->f_SetLength(_Length);
			}

		};

		class CLockFile
		{
		private:			
			NStr::CStr mp_FilePath;
			NFile::CFile mp_LockFile;
			NFile::EFileOpen mp_LockFlags;

			CLockFile(CLockFile const& _ToCopy); // delete
			CLockFile(CLockFile&& _ToMove); // delete
			CLockFile& operator=(CLockFile const& _ToCopy); // delete
			CLockFile& operator=(CLockFile&& _ToMove); // delete

		public:
			enum ELockResult
			{
				ELockResult_Locked,
				ELockResult_TimedOut,
				ELockResult_NoAccess,
				ELockResult_DoesNotExist, 	// Only ever returned if your lockflags do not permit creating the lock file...
											// ...OR if the file can be created but the path does not exist.

				ELockResult_Unlocked,	// Only ever returned by f_GetLockStatus()
			};

			CLockFile();
			CLockFile(NStr::CStr const& _FilePath, NFile::EFileOpen _LockFlags = NFile::EFileOpen_Write);
			~CLockFile();

			void f_SetLockFile(NStr::CStr const& _FilePath, NFile::EFileOpen _LockFlags = NFile::EFileOpen_Write);
			NStr::CStr f_GetLockFile() const;

			ELockResult f_Lock(fp64 _TimeoutSeconds = -1); // _TimeoutSeconds < 0 == Forever
			void f_Unlock();

			bint f_HasLock() const;	// Do I have the lock?
		};


	}
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif

#include "Malterlib_File.hpp"
