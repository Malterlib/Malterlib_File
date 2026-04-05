// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Mib/Cryptography/Hashes/MD5>
#include <Mib/Cryptography/Hashes/SHA>

//#	define DMibFileChangeNotificationsDebug

#	if defined(DMibFileChangeNotificationsDebug)
#		define DMibFileChangeNotificationsDebugOut(...) DMibLog(Info, __VA_ARGS__)
#	else
#		define DMibFileChangeNotificationsDebugOut(...)  (void)0
#	endif

namespace NMib::NEncoding
{
	namespace NPrivate
	{
		template <template <typename t_CParent> class t_TCValue, typename t_CTypes, bool t_bOrdered>
		struct TCJsonValueBase;
		struct CEJsonExtraTypesSorted;
	}

	template <typename t_CParent>
	struct TCEJsonValue;

	template <template <typename t_CParent> class t_TCValue, typename t_CTypes, bool t_bOrdered>
	using TCJson = t_TCValue<NPrivate::TCJsonValueBase<t_TCValue, t_CTypes, t_bOrdered>>;

	using CEJsonSorted = TCJson<TCEJsonValue, NPrivate::CEJsonExtraTypesSorted, false>;
}

namespace NMib::NFile
{
	class CFileProgress
	{
	public:

		virtual void f_Progress(CMibFilePos _Position, CMibFilePos _TotalSize) = 0;
	};

	class CFileHandle
	{
	public:
		uint32 m_HandleID;
		uint32 m_ProcessID;
		NStr::CStr m_Process;
	};

	struct CUniqueFileIdentifier
	{
		uint64 m_VolumeID = TCLimitsInt<uint64>::mc_Max;
		uint128 m_FileID = TCLimitsInt<uint64>::mc_Max;

		auto operator <=> (CUniqueFileIdentifier const &_Right) const noexcept = default;
	};

	struct CFileIoTempBuffer
	{
		CFileIoTempBuffer();
		~CFileIoTempBuffer();

		struct CUseBufferResult
		{
			uint8 *m_pBuffer;
			umint m_nBytes;
		};

		template <typename tf_CLength>
		inline_small CUseBufferResult f_UseBuffer(tf_CLength _Length);

	protected:
		inline_never void fp_CreateBuffer();

		NContainer::CByteVector mp_Data;
		umint mp_nUsedBytes = 0;
	};

	struct CFileIoTempBufferSecure : public CFileIoTempBuffer
	{
		CFileIoTempBufferSecure();
		~CFileIoTempBufferSecure();
	};
}

namespace NMib::NSys::NFile
{
	NMib::NFile::EFileSystemFeature fg_GetFileSystemFeatures();
	inline_always bool fg_FileSystemHasDrives() { return fg_GetFileSystemFeatures() & NMib::NFile::EFileSystemFeature_HasDrives; }
	inline_always bool fg_FileSystemHasExecuteAttrib() { return fg_GetFileSystemFeatures() & NMib::NFile::EFileSystemFeature_HasExecuteAttrib; }

	bool fg_FileExists(const NMib::NStr::CStr &_FileName, NMib::NFile::EFileAttrib _AttribMask);
	bool fg_FileExists(const NMib::NStr::CStrNonTracked &_FileName, NMib::NFile::EFileAttrib _AttribMask);

	NMib::NFile::ECheckFileRights fg_CheckFileRights( const NMib::NStr::CStr & _File, NMib::NFile::EFileRight _Rights);

	void *fg_Open(const NMib::NStr::CStr &_FileName, NMib::NFile::EFileOpen _OpenFlags, NMib::NFile::EFileAttrib _Attributes);
	void *fg_Open(const NMib::NStr::CStrNonTracked &_FileName, NMib::NFile::EFileOpen _OpenFlags, NMib::NFile::EFileAttrib _Attributes);
	void fg_Close(void *_pFile);
	void *fg_GetOSFile(void *_pFile);
	umint fg_Read(void *_pFile, void *_pData, const CMibFilePos &_Offset, umint _NumBytes); // Returns number of bytes read
	umint fg_Write(void *_pFile, const void *_pData, const CMibFilePos &_Offset, umint _NumBytes); // Returns number of bytes written
	void fg_Flush(void *_pFile); // Returns when all pending bytes are written to disk
	void fg_LockRange(void *_pFile, const CMibFilePos &_Offset, const CMibFilePos &_NumBytes, NMib::NFile::EFileLock _Flags);
	void fg_UnlockRange(void *_pFile, const CMibFilePos &_Offset, const CMibFilePos &_NumBytes);

	NMib::NFile::EFileAttrib fg_GetAttributes(void *_pFile);
	void fg_SetAttributes(void *_pFile, NMib::NFile::EFileAttrib _Attributes);

	NMib::NFile::EFileAttrib fg_GetAttributes(NMib::NStr::CStr const& _FileName);
	void fg_SetAttributes(NMib::NStr::CStr const& _FileName, NMib::NFile::EFileAttrib _Attributes);

	NMib::NFile::EFileAttrib fg_GetAttributesOnLink(NMib::NStr::CStr const& _FileName);
	void fg_SetAttributesOnLink(NMib::NStr::CStr const& _FileName, NMib::NFile::EFileAttrib _Attributes);

	NMib::NFile::CUniqueFileIdentifier fg_GetUniqueIdentifier(void *_pFile);
	NMib::NFile::CUniqueFileIdentifier fg_GetUniqueIdentifier(NMib::NStr::CStr const& _FileName);
	NMib::NFile::CUniqueFileIdentifier fg_GetUniqueIdentifierOnLink(NMib::NStr::CStr const& _FileName);

	NMib::NFile::EFileAttrib fg_GetSupportedAttributes();
	NMib::NFile::EFileAttrib fg_GetValidAttributes();
	umint fg_MaximumPathLength();

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

	void fg_SetCreationTime(NMib::NStr::CStr const &_FileName, const NTime::CTime &_Time);
	void fg_SetAccessTime(NMib::NStr::CStr const &_FileName, const NTime::CTime &_Time);
	void fg_SetWriteTime(NMib::NStr::CStr const &_FileName, const NTime::CTime &_Time);
	NTime::CTime fg_GetCreationTime(NMib::NStr::CStr const &_FileName);
	NTime::CTime fg_GetAccessTime(NMib::NStr::CStr const& _FileName);
	NTime::CTime fg_GetWriteTime(NMib::NStr::CStr const& _FileName);

	void fg_SetCreationTimeOnLink(NMib::NStr::CStr const &_FileName, const NTime::CTime &_Time);
	void fg_SetAccessTimeOnLink(NMib::NStr::CStr const &_FileName, const NTime::CTime &_Time);
	void fg_SetWriteTimeOnLink(NMib::NStr::CStr const &_FileName, const NTime::CTime &_Time);
	NTime::CTime fg_GetCreationTimeOnLink(NMib::NStr::CStr const &_FileName);
	NTime::CTime fg_GetAccessTimeOnLink(NMib::NStr::CStr const &_FileName);
	NTime::CTime fg_GetWriteTimeOnLink(NMib::NStr::CStr const &_FileName);

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

	void fg_Duplicate(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo);
	bool fg_TryDuplicate(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo);

	void fg_Copy(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo);
	void fg_Copy(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo, NMib::NFile::CFileProgress &_Progress);
	void fg_Rename(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo);
	void fg_Rename(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo, NMib::NFile::CFileProgress &_Progress);
	void fg_AtomicReplace(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo);

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
	NMib::NStream::CFilePos fg_GetTotalSpace(const NMib::NStr::CStr &_Path);

	NContainer::TCVector<NMib::NStr::CStr> fg_GetMounts(NMib::NFile::EFileMountType _Types);

	void fg_SetCurrentDirectory(const NMib::NStr::CStr &_Directory);

	NMib::NStr::CStr fg_GetProgramPath();
	NMib::NStr::CStr fg_GetProgramPathForExecutableContents();
	NMib::NStr::CStr fg_GetProgramDirectory();
	NMib::NStr::CStr fg_GetCurrentDirectory();
	NMib::NStr::CStr fg_GetUserProgramDirectory();
	NMib::NStr::CStr fg_GetUserLocalProgramDirectory();
	NMib::NStr::CStr fg_GetUserLocalProgramCacheDirectory();
	NMib::NStr::CStr fg_GetTemporaryDirectory();
	NMib::NStr::CStr fg_GetRawTemporaryDirectory();
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
	NMib::NStr::CStrNonTracked fg_GetRawTemporaryDirectoryNonTracked();
	NMib::NStr::CStrNonTracked fg_GetModulePathNonTracked(void *_pCode);
	NMib::NStr::CStrNonTracked fg_GetUserHomeDirectoryNonTracked();
	NMib::NStr::CStrNonTracked fg_GetLogDirectoryNonTracked();

	ch8 const *fg_GetDllExtension();

//			NMib::NStr::CStr fg_GetProgramRootDirectory();		// This defaults to the program directory but can be changed via fg_SetProgramRootDirectory.
//			void fg_SetProgramRootDirectory(NMib::NStr::CStr const& _Root);

	void *fg_ChangeNotification_Open(const NMib::NStr::CStr &_FileName, NMib::NFile::EFileChange _OpenFlags, NMib::NThread::CSemaphoreAggregate *_pReportTo);
	void fg_ChangeNotification_Close(void *_pNotification);
	bool fg_ChangeNotification_Changed(void *_pNotification);
	bool fg_ChangeNotification_GetNotification(void *_pNotification, NMib::NStr::CStr &_Path, NMib::NFile::EFileChangeNotification &_Notification, NMib::NStr::CStr &_PathFrom);
	bool fg_ChangeNotification_Supported();
}

namespace NMib::NFile
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
			if (NException::fg_UncaughtExceptions())
			{
				try
				{
					f_Close();
				}
				catch (NException::CException const &)
				{
				}
				return;
			}
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

		void f_Open(const NStr::CStr &_Path, EFileChange _OpenFlags, NMib::NThread::CSemaphoreAggregate *_pReportTo)
		{
			f_Close();
			mp_pNotification = NSys::NFile::fg_ChangeNotification_Open(_Path, _OpenFlags, _pReportTo);
		}

		bool f_IsOpen()
		{
			return mp_pNotification != nullptr;
		}

		bool f_Changed()
		{
			return NSys::NFile::fg_ChangeNotification_Changed(mp_pNotification);
		}

		class CNotification
		{
		public:
			EFileChangeNotification m_Notification = EFileChangeNotification_Undefined;
			NStr::CStr m_Path;
			NStr::CStr m_PathFrom; // Only used for EFileChangeNotification_Renamed

			auto operator <=> (CNotification const &_Right) const noexcept = default;
		};

		bool f_GetNotification(CNotification &_ToFill)
		{
			if (!f_IsOpen())
				DMibErrorFile("Notification not open");
			if (NSys::NFile::fg_ChangeNotification_GetNotification(mp_pNotification, _ToFill.m_Path, _ToFill.m_Notification, _ToFill.m_PathFrom))
				return true;
			return false;
		}

		static bool fs_Supported()
		{
			return NSys::NFile::fg_ChangeNotification_Supported();
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

		template <typename tf_CHash>
		struct TCFileChecksumState
		{
			tf_CHash m_Hash;
			CMibFilePos m_Length;
			NStorage::TCUniquePointer<CFile> m_pFile = fg_Construct();
		};

		using CFileChecksumState_MD5 = TCFileChecksumState<NCryptography::CHash_MD5>;
		using CFileChecksumState_SHA256 = TCFileChecksumState<NCryptography::CHash_SHA256>;
		using CFileChecksumState_SHA512 = TCFileChecksumState<NCryptography::CHash_SHA512>;

		struct CSetAttributeEmulationScope final : public CCoroutineThreadLocalHandler
		{
			CSetAttributeEmulationScope(bool _bEnableEmulation);
			~CSetAttributeEmulationScope();
			void f_Suspend() noexcept override;
			void f_ResumeNoExcept() noexcept override;
			void operator ()();

			static bool fs_IsEmulationEnabled();

		private:
			bool m_bEnableEmulation = false;
			bool m_bPreviousEnableEmulation = false;
		};

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

			auto operator <=> (CFoundFile const &_Other) const noexcept = default;
		};

		struct CFindFilesOptions
		{
			NStr::CStr m_Path;
			EFileAttrib m_AttribMask = EFileAttrib_Directory | EFileAttrib_File;
			bool m_bRecursive;
			bool m_bFollowLinks = false;
			NContainer::TCVector<NStr::CStr> m_ExcludePatterns;
			NFunction::TCFunctionNoAlloc<void ()> m_fCheckAbort;

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

		static NStr::CStr const mc_ExecutableExtension;

	private:
		void *mp_pFile = nullptr;
		CMibFilePos mp_FilePos = 0;
		EFileOpen mp_OpenFlags = EFileOpen_None;

		CMibFilePos mp_CachePos = 0;
		bool mp_bCacheDirty = false;
		bool mp_bNonTracked = false;
		CMibFilePos mp_CachedFileLen = 0;
		NContainer::CByteVector mp_CacheBuffer;
		NContainer::TCVector<uint8, NMemory::CAllocator_NonTrackedHeap> mp_CacheBufferNonTracked;

		uint8 *fp_GetCacheBuffer();
		umint fp_GetCacheBufferLen();

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
		void *fp_GetCache(CMibFilePos _CurrentPos, umint &_nBytesAvailable);
		CMibFilePos fp_GetLength() const;

		static bool fsp_OpenFile(NFile::CFile &_File, const NStr::CStr &_FileName, EFileOpen _Open, const NTime::CTime &_FileTime);
		static NTime::CTimeSpan fsp_GetDefaultFileChangedMargin();

		static bool fsp_DiffCopyFileOrDirectory
			(
				NStr::CStr const &_Source
				, NStr::CStr const &_Destination
				, NFunction::TCFunction<EDiffCopyChangeAction (EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
				, NContainer::TCVector<NStr::CStr> const &_ExcludePatterns
			)
		;
		static bool fsp_CopyFileDiff
				(
					const NContainer::CByteVector &_SourceData
					, const NStr::CStr &_FromFileName
					, const NStr::CStr &_ToFileName
					, const NTime::CTime &_FileTime
					, EFileAttrib _AddAttribs
					, NFunction::TCFunction<EDiffCopyChangeAction (EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
					, bool _bRemoveWriteProtection = false
				)
		;
		static bool fsp_CopyFileDiffDate
			(
				const NContainer::CByteVector &_SourceData
				, const NStr::CStr &_ToFileName
				, const NTime::CTime &_FileTime
				, EFileAttrib _AddAttribs
			)
		;
		static bool fsp_CopyFileDiff
			(
				const NStr::CStr &_FromFileName
				, const NStr::CStr &_ToFileName
				, bool _bCopyDate
				, NFunction::TCFunction<EDiffCopyChangeAction (EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange
				, bool _bRemoveWriteProtection = false
			)
		;
		static bool fsp_FileIsSame(NContainer::CByteVector const &_SourceData, const NStr::CStr &_ToFileName);
		static bool fsp_FileIsSame(NContainer::CSecureByteVector const &_SourceData, const NStr::CStr &_ToFileName);

	public:
		CFile(CFile const &) = delete;
		CFile &operator = (CFile const &) = delete;
		CFile(CFile &&);
		CFile &operator = (CFile &&);
		CFile();
		CFile(const NStr::CStr &_FileName, EFileOpen _OpenFlags);
		CFile(const NStr::CStrNonTracked &_FileName, EFileOpen _OpenFlags);
		~CFile();

		bool f_IsValid() const;

		void f_Close(bool _bCanThrow = true);
		void f_Open(const NStr::CStr &_FileName, EFileOpen _OpenFlags, EFileAttrib _Attributes = EFileAttrib_None);
		void f_Open(const NStr::CStrNonTracked &_FileName, EFileOpen _OpenFlags, EFileAttrib _Attributes = EFileAttrib_None);
		enum ESysFileTag
		{
		};
		void f_Open(void *_pFile, ESysFileTag, EFileOpen _OpenFlags);


		void f_Read(void *_pDest, umint _nBytes);
		void f_ReadNoLocalCache(CMibFilePos _Position, void *_pDest, umint _nBytes);
		void f_Write(const void *_pSrc, umint _nBytes);
		void f_WriteNoLocalCache(CMibFilePos _Position, void const *_pSrc, umint _nBytes);

		void f_FileEnumOtherHandles(NContainer::TCVector<CFileHandle> &_HandleInfo);

		static void fs_FileEnumOtherHandles(const NStr::CStr &_FileName, NContainer::TCVector<CFileHandle> &_HandleInfo);

		void f_FlushCache();

		void f_SetCacheSize(umint _CacheSize);

		void *f_GetOSFile() const;

		void f_Flush(bool _bLocalCacheOnly);
		bool f_IsAtEndOfFile() const;
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
		bool f_IsValidReadPosition(NStream::CFilePos _Pos) const;
		void f_LockRange(const CMibFilePos &_Offset, const CMibFilePos &_NumBytes, EFileLock _Flags);
		void f_UnlockRange(const CMibFilePos &_Offset, const CMibFilePos &_NumBytes);
		NStream::CFilePos f_GetPosition() const;
		void f_SetOwner(NStr::CStr const & _Owner);
		void f_SetGroup(NStr::CStr const & _Group);

		CUniqueFileIdentifier f_GetUniqueIdentifier() const;

		static umint fs_MaximumPathLength();

		// Static functions
		static bool fs_MakeFileWritable(const NStr::CStr &_LocalPath, bool _bWritable = true);
		static bool fs_IsFileWritable(const NStr::CStr &_LocalPath);
		static void fs_DeleteDirectoryRecursive(const NStr::CStr &_File, bool _bRemoveWriteProtection = false);
		static void fs_CopyFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress);
		static void fs_CopyFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo);
		static void fs_CopyFileRaw(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo);
		static void fs_CopyFileRaw(CFile &_FileFrom, CFile &_FileTo);

		static void fs_DuplicateFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo);
		static bool fs_TryDuplicateFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo);

		static void fs_CreateSymbolicLink(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo, EFileAttrib _Type, ESymbolicLinkFlag _Flags);
		static void fs_CreateHardLink(const NMib::NStr::CStr &_FileFrom, const NMib::NStr::CStr &_FileTo);
		static NMib::NStr::CStr fs_ResolveSymbolicLink(const NMib::NStr::CStr &_FileFrom);
		static bool fs_CanCreateSymbolicLink(EFileAttrib _Type, ESymbolicLinkFlag _Flags);


		static void fs_CopyFiles(const NStr::CStr &_FindPath, const NStr::CStr &_ToPath, bool _bRecursive = true, bool _bRaw = false, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File);
		static bool fs_FileIsSame(NContainer::CByteVector const &_SourceData, const NStr::CStr &_ToFileName);
		static bool fs_FileIsSame(NContainer::CSecureByteVector const &_SourceData, const NStr::CStr &_ToFileName);
		static NContainer::CByteVector fs_ReadFileTry(const NStr::CStr &_ToFileName);
		static bool fs_CopyFileDiff(const NContainer::CByteVector &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs = EFileAttrib_None, NFunction::TCFunction<EDiffCopyChangeAction (CFile::EDiffCopyChange _Change, NStr::CStr const &_Source, NStr::CStr const &_Destination, NStr::CStr const &_Link)> const &_OnChange = {});
		static bool fs_CopyFileDiffDate(const NContainer::CByteVector &_SourceData, const NStr::CStr &_ToFileName, const NTime::CTime &_FileTime, EFileAttrib _AddAttribs = EFileAttrib_None);
		static bool fs_CopyFileDiff(const NStr::CStr &_FromFileName, const NStr::CStr &_ToFileName, bool _bCopyDate);
		static void fs_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo);
		static void fs_AtomicReplaceFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo);
		static void fs_RenameFile(const NStr::CStr &_FileFrom, const NStr::CStr &_FileTo, CFileProgress &_Progress);
		static void fs_SetCurrentDirectory(const NStr::CStr &_Directory);

		static void fs_CreateDirectory(const NStr::CStr &_Path);
		static void fs_CreateDirectoryForFile(const NStr::CStr &_Path);
		static void fs_DeleteFile(const NStr::CStr &_File);
		static void fs_DeleteDirectory(const NStr::CStr &_File);

		static void fs_CreateDirectory(const NStr::CStrNonTracked &_Path);
		static void fs_DeleteFile(const NStr::CStrNonTracked &_File);
		static void fs_DeleteDirectory(const NStr::CStrNonTracked &_File);

		static void fs_Touch(const NStr::CStr &_File);

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
				, NContainer::TCVector<NStr::CStr> const &_ExcludePatterns = {}
				, fp32 _Timeout = 30.0f
			)
		;

		static ch8 const *fs_GetDllExtension();

		static NStr::CStr fs_GetProgramPathForExecutabelContents();
		static NStr::CStr fs_GetProgramPath();
		static NStr::CStr fs_GetOriginalProgramPath();
		static NStr::CStr fs_GetProgramDirectory();
		static NStr::CStr fs_GetCurrentDirectory();
		static NStr::CStr fs_GetUserProgramDirectory();
		static NStr::CStr fs_GetUserLocalProgramDirectory();
		static NStr::CStr fs_GetUserLocalProgramCacheDirectory();
		static NStr::CStr fs_GetTemporaryDirectory();
		static NStr::CStr fs_GetRawTemporaryDirectory();
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
		static NStr::CStrNonTracked fs_GetRawTemporaryDirectoryNonTracked();
		static NStr::CStrNonTracked fs_GetModulePathNonTracked(void *_pCode);
		static NStr::CStrNonTracked fs_GetProgramRootNonTracked(); // This can be set via CSystem::f_SetApplicationRoot(). Defaults to program directory.
		static NStr::CStrNonTracked fs_GetUserHomeDirectoryNonTracked();
		static NStr::CStrNonTracked fs_GetLogDirectoryNonTracked();

		template <typename tf_CStr>
		static TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetProgramPath()
		{
			return fs_GetProgramPathNonTracked();
		}
		template <typename tf_CStr>
		static TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetProgramPath()
		{
			return fs_GetProgramPath();
		}
		template <typename tf_CStr>
		static TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetProgramDirectory()
		{
			return fs_GetProgramDirectoryNonTracked();
		}
		template <typename tf_CStr>
		static TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetProgramDirectory()
		{
			return fs_GetProgramDirectory();
		}
		template <typename tf_CStr>
		static TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetCurrentDirectory()
		{
			return fs_GetCurrentDirectoryNonTracked();
		}
		template <typename tf_CStr>
		static TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetCurrentDirectory()
		{
			return fs_GetCurrentDirectory();
		}
		template <typename tf_CStr>
		static TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetUserProgramDirectory()
		{
			return fs_GetUserProgramDirectoryNonTracked();
		}
		template <typename tf_CStr>
		static TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetUserProgramDirectory()
		{
			return fs_GetUserProgramDirectory();
		}
		template <typename tf_CStr>
		static TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetUserLocalProgramDirectory()
		{
			return fs_GetUserLocalProgramDirectoryNonTracked();
		}
		template <typename tf_CStr>
		static TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetUserLocalProgramDirectory()
		{
			return fs_GetUserLocalProgramDirectory();
		}
		template <typename tf_CStr>
		static TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetTemporaryDirectory()
		{
			return fs_GetTemporaryDirectoryNonTracked();
		}
		template <typename tf_CStr>
		static TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetRawTemporaryDirectory()
		{
			return fs_GetRawTemporaryDirectoryNonTracked();
		}
		template <typename tf_CStr>
		static TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetTemporaryDirectory()
		{
			return fs_GetTemporaryDirectory();
		}
		template <typename tf_CStr>
		static TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetRawTemporaryDirectory()
		{
			return fs_GetRawTemporaryDirectory();
		}
		template <typename tf_CStr>
		static TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetModulePath(void *_pCode)
		{
			return fs_GetModulePathNonTracked(_pCode);
		}
		template <typename tf_CStr>
		static TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetModulePath(void *_pCode)
		{
			return fs_GetModulePath(_pCode);
		}
		template <typename tf_CStr>
		static TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetProgramRoot()
		{
			return fs_GetProgramRootNonTracked();
		}
		template <typename tf_CStr>
		static TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> fs_GetProgramRoot()
		{
			return fs_GetProgramRoot();
		}


		static NMib::NStream::CFilePos fs_GetFreeSpace(const NMib::NStr::CStr &_Path);
		static NMib::NStream::CFilePos fs_GetUsedSpace(const NMib::NStr::CStr &_Path);
		static NMib::NStream::CFilePos fs_GetTotalSpace(const NMib::NStr::CStr &_Path);
		static NContainer::TCVector<NStr::CStr> fs_GetMounts(EFileMountType _Types);

		static void fs_FindFilesGeneral
			(
				const NStr::CStr &_FindPath
				, NFunction::TCFunction<bool (NStr::CStr const &_FileName, EFileAttrib _Attribs, bool _bPreRecurse)> const &_fFoundFile // Return true to recurse into directory
				, bool _bRecursive = false
			)
		;
		static NContainer::TCVector<NStr::CStr>
		fs_FindFiles(const NStr::CStr &_FindPath, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File, bool _bRecursive = false, bool _bFollowLinks = true);
		static NContainer::TCVector<CFoundFile>
		fs_FindFilesEx(const NStr::CStr &_FindPath, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File, bool _bRecursive = false, bool _bFollowLinks = true);
		static NContainer::TCVector<CFoundFile> fs_FindFiles(CFindFilesOptions const &_Options);

		static bool fs_FileExists(const NStr::CStr &_File, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File);
		static bool fs_FileExists(const NStr::CStrNonTracked &_File, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File);
		static ECheckFileRights fs_CheckFileRights(NStr::CStr const& _File, EFileRight _Rights);
		static NCryptography::CHashDigest_MD5 fs_GetFileChecksum(const NStr::CStr &_Path, CFileChecksumState_MD5 *o_pState = nullptr, NStream::CFilePos _FileLength = -1);
		static NCryptography::CHashDigest_SHA256 fs_GetFileChecksum_SHA256(const NStr::CStr &_Path, CFileChecksumState_SHA256 *o_pState = nullptr, NStream::CFilePos _FileLength = -1);
		static NCryptography::CHashDigest_SHA512 fs_GetFileChecksum_SHA512(const NStr::CStr &_Path, CFileChecksumState_SHA512 *o_pState = nullptr, NStream::CFilePos _FileLength = -1);
		static NCryptography::CHashDigest_SHA256 fs_GetFileChecksum_SHA256(CFile &_File, NStream::CFilePos _FileLength = -1);
		static NCryptography::CHashDigest_SHA512 fs_GetFileChecksum_SHA512(CFile &_File, NStream::CFilePos _FileLength = -1);
		static NCryptography::CHashDigest_MD5 fs_GetDirectoryChecksum
			(
				const NStr::CStr &_Path
				, NFunction::TCFunction<bool (NStr::CStr const &_File)> const &_ExcludeFile
				, NFunction::TCFunction<NCryptography::CHashDigest_MD5 (NStr::CStr const &_File)> const &_GetHash = fg_Default()
			)
		;
		static void fs_WriteFile(const NStr::CStr &_Path, NContainer::CByteVector const &_Data);
		static void fs_WriteFileSecure(const NStr::CStr &_Path, NContainer::CSecureByteVector const &_Data);
		static NContainer::CByteVector fs_ReadFile(const NStr::CStr &_Path);
		static NContainer::CSecureByteVector fs_ReadFileSecure(const NStr::CStr &_Path);

		static void fs_WriteFile(NContainer::CByteVector const &_FileFrom, const NStr::CStr &_FileTo);
		static void fs_WriteFileSecure(NContainer::CSecureByteVector const &_FileFrom, const NStr::CStr &_FileTo);

		static const ch8 *fs_GetInvalidFileNameChars(bool _bPath);
		static const ch8 **fs_GetInvalidFileNameNames();
		static void fs_SetAttributes(NStr::CStr const &_FileName, EFileAttrib _Attribs);
		static EFileAttrib fs_GetAttributes(NStr::CStr const &_FileName);
		static void fs_SetAttributesOnLink(NStr::CStr const &_FileName, EFileAttrib _Attribs);
		static EFileAttrib fs_GetAttributesOnLink(NStr::CStr const &_FileName);
		static bool fs_SetUnixAttributesRecursive(NStr::CStr const &_Path, EFileAttrib _FileAttributes, EFileAttrib _DirectoryAttributes, bool _bFollowLinks = false);

		static CUniqueFileIdentifier fs_GetUniqueIdentifier(NMib::NStr::CStr const& _FileName);
		static CUniqueFileIdentifier fs_GetUniqueIdentifierOnLink(NMib::NStr::CStr const& _FileName);

		static NTime::CTime fs_GetCreationTime(NStr::CStr const &_FileName);
		static NTime::CTime fs_GetAccessTime(NStr::CStr const &_FileName);
		static NTime::CTime fs_GetWriteTime(NStr::CStr const &_FileName);
		static NTime::CTime fs_GetCreationTimeOnLink(NStr::CStr const &_FileName);
		static NTime::CTime fs_GetAccessTimeOnLink(NStr::CStr const &_FileName);
		static NTime::CTime fs_GetWriteTimeOnLink(NStr::CStr const &_FileName);

		static void fs_SetCreationTime(NStr::CStr const& _FileName, NTime::CTime const &_Time);
		static void fs_SetAccessTime(NStr::CStr const& _FileName, NTime::CTime const &_Time);
		static void fs_SetWriteTime(NStr::CStr const& _FileName, NTime::CTime const &_Time);
		static void fs_SetCreationTimeOnLink(NStr::CStr const& _FileName, NTime::CTime const &_Time);
		static void fs_SetAccessTimeOnLink(NStr::CStr const& _FileName, NTime::CTime const &_Time);
		static void fs_SetWriteTimeOnLink(NStr::CStr const& _FileName, NTime::CTime const &_Time);

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

		static void fs_WriteStringToVector(NContainer::CByteVector &_File, const NStr::CStr &_ToWrite, bool _bAddBOM = true);
		static NStr::CStr fs_ReadStringFromVector(const NContainer::CByteVector &_File, bool _bAssumeUTF8 = false);

		static void fs_WriteStringToFile(const NStr::CStr &_Path, const NStr::CStr &_ToWrite, bool _bAddBOM = true, EFileAttrib _Attributes = EFileAttrib_None);
		static NStr::CStr fs_ReadStringFromFile(const NStr::CStr &_Path, bool _bAssumeUTF8 = false);

		static void fs_WriteStringToFile(const NStr::CStrNonTracked &_Path, const NStr::CStrNonTracked &_ToWrite, bool _bAddBOM = true, EFileAttrib _Attributes = EFileAttrib_None);
		static NStr::CStrNonTracked fs_ReadStringFromFile(const NStr::CStrNonTracked &_Path, bool _bAssumeUTF8 = false);

		static NStr::CStr fs_CondensePath(NStr::CStr _Path);
		static NStr::CStr fs_MakeNiceFilename(const NStr::CStr &_CurrentName);

		template <typename tf_CStr>
		static tf_CStr fs_GetFullPath(const tf_CStr &_Path, const tf_CStr &_Base);
		template <typename tf_CStr>
		static tf_CStr fs_GetExpandedPath(const tf_CStr &_Path, bool _bAddCurrentDir = true);
		template <typename tf_CStr>
		static tf_CStr fs_GetExpandedPath(const tf_CStr &_Path, const tf_CStr &_BasePath);

		template <typename tf_CStr>
		static bool fs_HasRelativeComponents(const tf_CStr &_Path);

		template <typename tf_CStr>
		static bool fs_IsPathAbsolute(tf_CStr _Path);
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
		static tf_CStr fs_RemovePathTopLevels(const tf_CStr &_Path, umint _Levels);
		template <typename tf_CStr>
		static tf_CStr fs_GetFile(const tf_CStr &_File);
		template <typename tf_CStr>
		static tf_CStr fs_GetExtension(const tf_CStr &_File);
		template <typename tf_CStr>
		static tf_CStr fs_GetDrive(const tf_CStr &_File);
		template <typename tf_CStr>
		static tf_CStr fs_GetFileNoExt(const tf_CStr &_File);

		static EFileAttrib fs_AttribFromJson(NEncoding::CEJsonSorted const &_Json);
		static NEncoding::CEJsonSorted fs_AttribToJson(EFileAttrib _Attribs);

	private:
		static auto fsp_EncodeChar(ch32 _Char) -> NStr::CFWStr16;
		static auto fsp_EncodeString(ch32 *_pChars, umint _Len) -> NStr::CUStr;
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

		static bool fs_IsValidFilePath(const NStr::CStr &_File, EInvalidPathReason &_InvalidReason, NStr::CStr &_InvalidPart);
		static bool fs_IsValidFilePath(const NStr::CStr &_File, NStr::CStr &_Error);
		static bool fs_IsSafeRelativePath(const NStr::CStr &_File, NStr::CStr &o_Error);
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

		template <typename tf_CType, typename tf_CStr>
		static tf_CType fs_ReadFile(tf_CStr const &_FileName, EFileOpen _OpenFlags = EFileOpen_Read | EFileOpen_ShareAll)
		{
			TCBinaryStreamFile Stream;
			Stream.f_Open(_FileName, _OpenFlags);
			tf_CType Data;
			Stream >> Data;
			return Data;
		}

		template <typename tf_CType, typename tf_CStr>
		static void fs_WriteFile
			(
				tf_CType const &_Data
				, tf_CStr const &_FileName
				, EFileOpen _OpenFlags = EFileOpen_Write | EFileOpen_ShareAll
				, EFileAttrib _Attributes = EFileAttrib_None
			)
		{
			TCBinaryStreamFile Stream;
			Stream.f_Open(_FileName, _OpenFlags, _Attributes);
			Stream << _Data;
		}

		template <typename tf_CStr>
		void f_Open(const tf_CStr &_FileName, EFileOpen _OpenFlags, EFileAttrib _Attributes = EFileAttrib_None)
		{
			m_File.f_Open(_FileName, _OpenFlags, _Attributes);
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

		void f_FeedBytes(const void *_pMem, umint _nBytes)
		{
			m_File.f_Write(_pMem, _nBytes);
		}

		void f_ConsumeBytes(void *_pMem, umint _nBytes)
		{
			m_File.f_Read(_pMem, _nBytes);
		}

		bool f_IsValid() const
		{
			return m_File.f_IsValid();
		}

		bool f_IsAtEndOfStream() const
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

		bool f_IsValidReadPosition(NStream::CFilePos _Pos) const
		{
			return m_File.f_IsValidReadPosition(_Pos);
		}

		void f_Flush(bool _bLocalCacheOnly)
		{
			return m_File.f_Flush(_bLocalCacheOnly);
		}

		void f_SetCacheSize(umint _CacheSize)
		{
			m_File.f_SetCacheSize(_CacheSize);
		}

		NStream::CFilePos f_GetLength() const
		{
			return m_File.f_GetLength();
		}

		umint f_ContainerLengthLimit() const
		{
			return NStream::fg_CapLengthLimit(f_GetLength() - f_GetPosition());
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

		void f_FeedBytes(const void *_pMem, umint _nBytes)
		{
			mp_pFile->f_Write(_pMem, _nBytes);
		}

		void f_ConsumeBytes(void *_pMem, umint _nBytes)
		{
			mp_pFile->f_Read(_pMem, _nBytes);
		}

		bool f_IsValid() const
		{
			return mp_pFile->f_IsValid();
		}

		bool f_IsAtEndOfStream() const
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

		bool f_IsValidReadPosition(NStream::CFilePos _Pos) const
		{
			return mp_pFile->f_IsValidReadPosition(_Pos);
		}

		void f_Flush(bool _bLocalCacheOnly)
		{
			mp_pFile->f_Flush(_bLocalCacheOnly);
		}

		void f_SetCacheSize(umint _CacheSize)
		{
			mp_pFile->f_SetCacheSize(_CacheSize);
		}

		NStream::CFilePos f_GetLength() const
		{
			return mp_pFile->f_GetLength();
		}

		umint f_ContainerLengthLimit() const
		{
			return NStream::fg_CapLengthLimit(mp_pFile->f_GetLength() - mp_pFile->f_GetPosition());
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
			ELockResult_DoesNotExist,	// Only ever returned if your lockflags do not permit creating the lock file...
										// ...OR if the file can be created but the path does not exist.

			ELockResult_Unlocked,	// Only ever returned by f_GetLockStatus()
		};

		CLockFile();
		CLockFile(NStr::CStr const& _FilePath, NFile::EFileOpen _LockFlags = NFile::EFileOpen_Write);
		~CLockFile();

		void f_SetLockFile(NStr::CStr const& _FilePath, NFile::EFileOpen _LockFlags = NFile::EFileOpen_Write);
		NStr::CStr f_GetLockFile() const;

		ELockResult f_Lock(fp64 _TimeoutSeconds = -1, NStr::CStr *o_pLastError = nullptr); // _TimeoutSeconds < 0 == Forever
		void f_LockWithException(fp64 _TimeoutSeconds = -1); // _TimeoutSeconds < 0 == Forever
		void f_Unlock();

		bool f_HasLock() const;	// Do I have the lock?
	};
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif

#include "Malterlib_File.hpp"
