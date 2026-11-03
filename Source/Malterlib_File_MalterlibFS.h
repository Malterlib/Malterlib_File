// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Mib/Core/Core>
#include <Mib/String/MixedPreserve>

namespace NMib::NFile
{
	struct CVirtualFS_FixFSFileTree;
	struct CVirtualFS
	{
		struct CFile;
		using CClusterID = uint64;
		using CDirectoryID = uint64;
		using CFilePos = uint64;

	private:
		friend struct CVirtualFS_FixFSFileTree;
		friend struct CFile;

		struct CRootData
		{
			CRootData();

			NCryptography::CHashDigest_MD5 m_MD5Checksum; // Validate the data
			uint64 m_ClusterSize;                         // Cluster size minimum cluster size is 128
			CClusterID m_RootFileCluster;                 // Cluster at which the root directory resides
			CClusterID m_FirstFreeCluster;                // First free cluster
			uint64 m_FileSystemGrowSize;                  // We grow the master file with this much
			uint64 m_nFreeClusters;                       // Max Cluster Size
			uint64 m_DirectoryCacheSize;                  // How many directories to cache
			uint64 m_ClusterCacheSize;                    // How many clusters to cache
			CDirectoryID m_CurrentDirectoryID;
			uint32 m_Version;

			void f_WriteInternal(NStream::CBinaryStream &_Stream);
			void f_Write(NStream::CBinaryStream &_Stream);
			void f_Read(NStream::CBinaryStream &_Stream);
			void f_GenerateChecksum();
			bool f_Validate();
		};

		NStream::CBinaryStream *mp_pStorageStream;
		CRootData mp_RootData;

		uint64 mp_ClusterIDsPerCluster;
		bool mp_bOutOfSpace;

		void fp_SaveRootData();

		inline_small CDirectoryID fp_GetDirectoryID();
		inline_small CFilePos fp_GetFilePos(CClusterID _Cluster);
		void fp_SetStreamCluster(CClusterID _Cluster);
		inline_small CClusterID fp_GetClusterID(CFilePos _Position, CFilePos &_Remaining);

		struct CFileRecord
		{
			NCryptography::CHashDigest_MD5 m_MD5Checksum; // Validate the data
			NTime::CTime m_CreationTime;
			NTime::CTime m_AccessTime;
			NTime::CTime m_WriteTime;
			uint64 m_FileSize;
			uint64 m_FileAttributes;
			CClusterID m_FirstFileChainCluster;           // At this cluster we store the cluster chain for the file data with inline linking to other clusters
			CDirectoryID m_ParentDirectory;
			// 96 bytes

			void f_Reset();
			void f_GenerateChecksum(uint64 _ClusterID, uint32 _Version);
			bool f_Validate(uint64 _ClusterID, uint32 _Version);
			void f_WriteInternal(NStream::CBinaryStream &_Stream);
			void f_Write(NStream::CBinaryStream &_Stream);
			void f_Read(NStream::CBinaryStream &_Stream);
		};

		struct CClusterChain
		{
			NCryptography::CHashDigest_MD5 m_MD5Checksum; // Validate the data
			CClusterID m_NextChainCluster;
			CClusterID m_ClusterFirstIDIndex;
			uint64 m_nClusterIDs;
			NContainer::TCVector<CClusterID> m_ClusterIDs;

			void f_GenerateChecksum(uint64 _ClusterID, uint32 _Version);
			bool f_Validate(uint64 _ClusterID, uint32 _Version);
			void f_WriteInternal(NStream::CBinaryStream &_Stream);
			void f_Write(NStream::CBinaryStream &_Stream);
			void f_Read(NStream::CBinaryStream &_Stream);
		};

	public:
		struct CDirectory
		{
			NCryptography::CHashDigest_MD5 m_MD5Checksum; // Validate the data
			CDirectoryID m_DirectoryID;

			struct CFile
			{
				CClusterID m_FileRecordCluster;
				NStr::CMStrPreserve m_FileName;

				void f_FeedPreserve(NStream::CBinaryStream &_Stream) const;
				void f_FeedNormal(NStream::CBinaryStream &_Stream) const;
				void f_Consume(NStream::CBinaryStream &_Stream);
			};

			NContainer::TCVector<CFile> m_Files;

			void f_GenerateChecksum(uint64 _ClusterID, uint32 _Version);
			void f_AddChild(NStr::CStr const &_Path, CClusterID _ClusterID);
			bool f_Validate(uint64 _ClusterID, uint32 _Version);
			void f_WriteInternalValidate(NStream::CBinaryStream &_Stream);
			void f_WriteInternal(NStream::CBinaryStream &_Stream);
			void f_Write(NStream::CBinaryStream &_Stream);
			void f_Read(NStream::CBinaryStream &_Stream);
			CClusterID f_GetChild(NStr::CStr const &_Child);
			void f_RemoveChild(CClusterID _Child);
		};

	private:
		struct CFreeCluster
		{
			NCryptography::CHashDigest_MD5 m_MD5Checksum; // Validate the data
			CClusterID m_NextFreeCluster;                 // Next free cluster

			void f_WriteInternal(NStream::CBinaryStream &_Stream);
			void f_Write(NStream::CBinaryStream &_Stream);
			void f_Read(NStream::CBinaryStream &_Stream);
			void f_GenerateChecksum(uint64 _ClusterID, uint32 _Version);
			bool f_Validate(uint64 _ClusterID, uint32 _Version);
		};

		CClusterID fp_GetNewCluster();
		void fp_ReturnCluster(CVirtualFS::CClusterID _Cluster);

		struct CClusterCacheEntry
		{
			CClusterID m_ClusterID;
			NContainer::CByteVector m_ClusterMemory;

			DMibListLinkD_Link(CClusterCacheEntry, m_AccessLink);
			//DMibListLinkD_Link(CClusterCacheEntry, m_LazyLink);

			umint m_RefCount;

			bool m_bDirty;

			CClusterCacheEntry();

			struct CCompare
			{
				inline_small CClusterID const &operator () (CClusterCacheEntry const &_Node) const;
			};

			NIntrusive::TCAVLLink<> m_CacheLink;

			void f_MakeDirty();
			void f_OpenStream(NStream::TCBinaryStreamMemoryPtr<> &_Stream);
			void f_CloseStream(bool _bWritten);
		};

		struct CClusterCache
		{
			CClusterCache();
			void f_Create(CVirtualFS *_pVirtualFS);
			~CClusterCache();
			void f_Destroy();
			void f_Flush(CClusterCacheEntry *_pCluster);
			void f_Flush();

			CVirtualFS *m_pVirtualFS;

			NIntrusive::TCAVLTree<&CClusterCacheEntry::m_CacheLink, CClusterCacheEntry::CCompare> m_Tree;
			DMibListLinkD_List(CClusterCacheEntry, m_AccessLink) m_Cached;
			DMibListLinkD_List(CClusterCacheEntry, m_AccessLink) m_InUse;
			// DMibListLinkD_List(CClusterCacheEntry, m_LazyLink) m_LazyOrder; Disable lazy writing for now
			umint m_nCached;
			umint m_nCachedMax;

			void f_SetCacheSize(umint _nCachedMax);
			void f_RemoveCacheEntry(CClusterCacheEntry *_pEntry);
			void f_MakeSpace(umint _Space);
			CClusterCacheEntry *f_GetCluster(CClusterID _Cluster);
			void f_ReturnCluster(CClusterCacheEntry *_pCluster);
		};

		CClusterCache mp_ClusterCache;

		struct CDirectoryCacheEntry
		{
			NStr::CStr m_FullPath;
			CDirectory m_Directory;
			CClusterID m_DirectoryFileClusterID;
			DMibListLinkD_Link(CDirectoryCacheEntry, m_AccessLink);
			bool m_bDirty;
			umint m_RefCount;

			CDirectoryCacheEntry();

			struct CCompare
			{
				inline_small NStr::CStr const &operator () (CDirectoryCacheEntry const &_Node) const;
			};

			NIntrusive::TCAVLLink<> m_CacheLink;
		};

		struct CDirectoryCache
		{
			CDirectoryCache();
			~CDirectoryCache();
			void f_Destroy();
			void f_Flush();
			void f_Flush(CDirectoryCacheEntry *_pEntry);
			void f_Create(CVirtualFS *_pVirtualFS);

			CVirtualFS *m_pVirtualFS;

			NIntrusive::TCAVLTree<&CDirectoryCacheEntry::m_CacheLink, CDirectoryCacheEntry::CCompare> m_Tree;
			DMibListLinkD_List(CDirectoryCacheEntry, m_AccessLink) m_Cached;
			DMibListLinkD_List(CDirectoryCacheEntry, m_AccessLink) m_InUse;
			umint m_nCached;
			umint m_nCachedMax;

			CDirectoryCacheEntry *f_GetNewCacheEntry(CDirectoryCacheEntry *_pParent, NStr::CStr const &_Path);
			CDirectoryCacheEntry *f_GetExistingCacheEntry(CClusterID _DirectoryCluster, NStr::CStr const &_Path);

			CDirectoryCacheEntry *f_GetCacheEntry(CDirectoryCacheEntry *_pParentEntry, NStr::CStr const &_CurrentPath);
			void f_ReturnCacheEntry(CDirectoryCacheEntry *_pEntry);
			CDirectoryCacheEntry *f_GetDirectoryCommon(NStr::CStr const &_Path);
			CDirectoryCacheEntry *f_GetDirectory(NStr::CStr const &_Path);
			CDirectoryCacheEntry *f_GetNewDirectory(NStr::CStr const &_Path);
			void f_ReturnDirectory(CDirectoryCacheEntry *_pEntry);
			void f_RemoveCacheEntry(CDirectoryCacheEntry *_pEntry);
			void f_MakeSpace(umint _Space);
			void f_SetCacheSize(umint _Size);
		};

		CDirectoryCache mp_DirectoryCache;

		struct CFileInternal
		{
			CVirtualFS *m_pVirtualFS;

			umint m_RefCount;
			CClusterID m_FileDescriptorID; // This is the guid for the file
			CFileRecord m_FileRecord;      // The file record. The records lives here while the file is open
			CClusterChain m_ClusterChain;  // The chain of data clusters
			CClusterID m_ClusterChainCluster;
			bool m_bValidClusterChain;
			bool m_bClusterChainDirty;
			bool m_bFileRecordDirty;
			bool m_bDestroyException;

			struct CCompare
			{
				inline_small CClusterID const &operator () (CFileInternal const &_Node) const;
			};

			NIntrusive::TCAVLLink<> m_CacheLink;

			CFileInternal();
			~CFileInternal() noexcept(false);

			struct CClusterCache
			{
				CClusterCache(CClusterCacheEntry *_pClusterCacheEntry, CFilePos _FilePosStart);

				DMibListLinkDS_Link(CClusterCache, m_Link);
				CClusterCacheEntry *m_pClusterCacheEntry;
				CFilePos m_FilePosStart;
			};

			DMibListLinkDS_List(CClusterCache, m_Link) m_CachedClusters;
			umint m_nCachedClusters;

			CClusterCacheEntry *f_GetCluster(CFilePos &_Position);
			void f_Read(void *_pDest, CFilePos _Position, umint _nBytes);
			void f_Write(void const *_pSource, CFilePos _Position, umint _nBytes);
			void f_WriteClusterChain();

			void f_SetLength(uint64 _Length);

			void f_Delete();
			void f_ReadClusterChain(CClusterID _ClusterID);
			CClusterID f_GetDataCluster(CFilePos &_FilePosition);
			void f_Create(CVirtualFS *_pVirtualFS, CClusterID _FileDescriptorID, bool _bLoad);
			void f_WriteFileRecord();
			void f_Destroy();
			void f_Flush();
		};

		NIntrusive::TCAVLTree<&CFileInternal::m_CacheLink, CFileInternal::CCompare> mp_InternalFiles;

		struct CBinaryStreamFileInternal : public NStream::CBinaryStreamDefault
		{
			DMibStreamImplementOperators(CBinaryStreamFileInternal);

			CBinaryStreamFileInternal();
			void f_Open(CFileInternal *_pFile);
			void f_Reset();
			void f_FeedBytes(void const *_pMem, umint _nBytes);
			void f_ConsumeBytes(void *_pMem, umint _nBytes);
			bool f_IsValid() const;
			bool f_IsAtEndOfStream() const;
			NStream::CFilePos f_GetPosition() const;
			void f_SetPosition(NStream::CFilePos _Pos);
			void f_SetPositionFromEnd(NStream::CFilePos _Pos);
			void f_AddPosition(NStream::CFilePos _Pos);
			bool f_IsValidReadPosition(NStream::CFilePos _Pos) const;
			void f_Flush(bool _bLocalCacheOnly);
			void f_SetCacheSize(umint _CacheSize);
			NStream::CFilePos f_GetLength() const;
			umint f_ContainerLengthLimit() const;
			void f_SetLength(NStream::CFilePos _Length);

		private:
			CFileInternal *mp_pFile;
			CFilePos mp_Position;

		protected:
			DMibStreamImplementProtected(CBinaryStreamFileInternal);
		};

		void fp_Close();

		void fp_CheckPath(NStr::CStr const &_Path, bool _bCanBeEmpty = false);
		CFileInternal *fp_OpenFile(CClusterID _ClusterID);
		CFileInternal *fp_OpenNewFile();
		void fp_CloseFile(CFileInternal *_pFile);

		CFileInternal *fp_OpenFileFromPath(NStr::CStr const &_Path, bool _bCreate, bool _bRemove = false, bool _bAllowRoot = false);
		void fp_DeleteDirectoryRecursive(NStr::CStr const &_File);

	public:
		CVirtualFS();
		~CVirtualFS();
		bool f_IsValid();

		void f_Flush();

		void f_Open(NStream::CBinaryStream *_pStorageStream);
		void f_Create(NStream::CBinaryStream *_pStorageStream, uint64 _ClusterSize, uint64 _FileSystemGrowSise, uint64 _InitialSize);
		void f_Close();

		void f_SetCacheSizes(umint _nDirectories, umint _nClusterBytes);

		void f_OpenFile(CFile &_File, NStr::CStr const &_FileName, uint32 _OpenFlags);
		void f_DeleteFile(NStr::CStr const &_File);
		void f_RenameFile(NStr::CStr const &_FileFrom, NStr::CStr const &_FileTo);
		void f_DeleteDirectoryRecursive(NStr::CStr const &_File);
		void f_CreateDirectory(NStr::CStr const &_Path);
		bool f_FileExists(NStr::CStr const &_File);
		bool f_FileExists(NStr::CStr const &_File, EFileAttrib _Attributes);
		bool f_ReadFileToMemory(NStr::CStr const &_File, NContainer::CByteVector &_Data);

		struct CCheckReporter
		{
			virtual void f_ReportError(NStr::CStr const &_Error) = 0;
			virtual void f_IncreaseProgress(uint64 _nInc);
			virtual void f_StepDown(uint64 _nMax);
			virtual void f_StepUp();
		};

		enum ECheckFSError
		{
			ECheckFSError_None
			, ECheckFSError_NotCheckableMemoryInsufficient
			, ECheckFSError_InplaceFixableSecondPass
			, ECheckFSError_VersionUpgradeFixable
			, ECheckFSError_InplaceFixable
			, ECheckFSError_NewFSFixable
			, ECheckFSError_NotFixable
		};

		struct CCheckFSContext
		{
			enum EClusterCheckFlag
			{
				EClusterCheckFlag_Free               = DMibBit(0)
				, EClusterCheckFlag_FileRecord       = DMibBit(1)
				, EClusterCheckFlag_FileClusterChain = DMibBit(2)
				, EClusterCheckFlag_FileData         = DMibBit(2)
				, EClusterCheckFlag_Invalid          = DMibBit(3)
				, EClusterCheckFlag_RootData         = DMibBit(4)
			};

			struct CClusterCheck
			{
				CClusterCheck();

				uint64 m_Flags;
				CVirtualFS::CClusterID m_ClusterID;
			};

			CCheckFSContext(NStream::CBinaryStream *_pStorageStream, CCheckReporter *_pReporter, bool _bFix, CVirtualFS *_pFixDestinationFS);

			ECheckFSError f_UpdateFreeClusters();
			void f_FreeCluster(CClusterID _Cluster);
			void f_Seek(CClusterID _Cluster);
			uint64 f_AddClusterID(CClusterID _Cluster, CClusterID _File, uint64 _Flags, bool _bMap);
			ECheckFSError f_AddFreeClusters(CClusterID _ClusterID);
			bool f_WriteFileData(CVirtualFS::CFileRecord &_FileRecord, NContainer::CByteVector &_Data, umint _Length);
			bool f_ReadFileData(CVirtualFS::CFileRecord &_FileRecord, NContainer::CByteVector &_Data);
			static auto fs_ReadFileData
				(
					uint32 _Version
					, uint64 _ClusterSize
					, uint64 _ClusterIDsPerCluster
					, NStream::CBinaryStream *_pStorageStream
					, CVirtualFS::CFileRecord &_FileRecord
					, NContainer::CByteVector &_Data
				)
				-> bool
			;
			static auto fs_CheckClusterChain
				(
					uint32 _Version
					, uint64 _ClusterSize
					, uint64 _ClusterIDsPerCluster
					, NStream::CBinaryStream *_pStorageStream
					, CVirtualFS::CFileRecord &_FileRecord
				)
				-> bool
			;

			struct CCircularChecker
			{
				struct CMember
				{
					DMibListLinkDS_Link(CMember, m_Link);
					CClusterID m_ClusterID;
				};

				DMibListLinkDS_List(CMember, m_Link) m_List;

				bool f_AlreadyInList(CClusterID _ClusterID);
			};

			ECheckFSError f_CheckOrphanClusters();
			ECheckFSError f_CheckFSTree(CClusterID _ClusterID, bool _bMap);
			ECheckFSError f_CheckFSTreeRecursive(CClusterID _ClusterID, CCircularChecker &_CircularChecker, bool &_bRemove, bool _bMap, CDirectoryID _ParentDir, NStr::CStr const &_FileName);

		private:
			friend struct CVirtualFS;

			CVirtualFS::CRootData mp_RootData;
			NContainer::TCVector<CClusterCheck> mp_ClusterMap;
			NStream::CBinaryStream *mp_pStorageStream;
			CCheckReporter *mp_pReporter;
			CVirtualFS *mp_pFixDestinationFS;
			CDirectoryID mp_MaxDirectoryID;
			bool mp_bFix;
			uint64 mp_ClusterIDsPerCluster;

			void fp_SaveRootData();
		};

		static ECheckFSError fs_CheckFS(NStream::CBinaryStream *_pStorageStream, CCheckReporter *_pReporter, bool _bFix, CVirtualFS *_pFixDestinationFS);

		uint64 f_GetClusterSize();
		uint64 f_GetClusterIDsPerCluster();
		uint64 f_GetNumberOfFreeClusters();
		uint64 f_GetFreeSize();
		uint64 f_GetFileSystemGrowSize();
		void f_SetFileSystemGrowSize(uint64 _Size);

		CMibFilePos f_GetTotalSize();

		void f_SetAttributes(NStr::CStr const &_Path, uint64 _Attribs);
		void f_ChangeFileAttributes(NStr::CStr const &_Path, uint64 _AttribsAdd, uint64 _AttribsRemove);
		void f_SetCreationTime(NStr::CStr const &_Path, NTime::CTime const &_Time);
		void f_SetAccessTime(NStr::CStr const &_Path, NTime::CTime const &_Time);
		void f_SetWriteTime(NStr::CStr const &_Path, NTime::CTime const &_Time);
		uint64 f_GetAttributes(NStr::CStr const &_Path);
		NTime::CTime f_GetCreationTime(NStr::CStr const &_Path);
		NTime::CTime f_GetAccessTime(NStr::CStr const &_Path);
		NTime::CTime f_GetWriteTime(NStr::CStr const &_Path);

		uint64 f_GetFileSize(NStr::CStr const &_Path);

		void f_EnumFiles(NStr::CStr const &_Path, NContainer::TCVector<NStr::CStr> &_Files);
		NContainer::TCVector<NStr::CStr> f_FindFiles(NStr::CStr const &_Path, uint32 _AttribMask = EFileAttrib_Directory | EFileAttrib_File, bool _bRecursive = false);
		NCryptography::CHashDigest_MD5 f_GetFileChecksum(NStr::CStr const &_Path);

		struct CFile
		{
			CFile();
			CFile(CVirtualFS &_FileSystem, NStr::CStr const &_FileName, uint32 _OpenFlags);
			~CFile();

			bool f_IsValid() const;
			void f_Close();
			void f_Open(CVirtualFS &_FileSystem, NStr::CStr const &_FileName, uint32 _OpenFlags);
			void f_Read(void *_pDest, umint _nBytes);
			void f_Write(void const *_pSrc, umint _nBytes);
			void f_Flush(bool _bLocalCacheOnly);
			void f_SetCacheSize(umint _CacheSize);
			bool f_IsAtEndOfFile() const;
			void f_SetLength(CMibFilePos const &_Length);
			CMibFilePos f_GetLength() const;
			void f_SetPosition(NStream::CFilePos _Pos);
			void f_SetPositionFromEnd(NStream::CFilePos _Pos);
			void f_AddPosition(NStream::CFilePos _Pos);
			bool f_IsValidReadPosition(NStream::CFilePos _Pos) const;
			NStream::CFilePos f_GetPosition() const;

		private:
			friend struct CVirtualFS;
			CVirtualFS::CFileInternal *mp_pInternalFile;
			uint64 mp_Position;
			void fp_CheckValid() const;
			DMibListLinkDS_Link(CFile, mp_Link);
		};

	private:
		DMibListLinkDS_List(CFile, mp_Link) mp_OpenFiles;
		using COpenFilesIter = DMibListLinkDS_Iter(CFile, mp_Link);

	public:
		void f_CloseOpenFiles();

		void f_CopyFileToFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile);
		void f_CopyFileFromFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile);
		void f_CopyFileToFS(CVirtualFS &_OtherFS, NStr::CStr const &_FromFile, NStr::CStr const &_ToFile);
		void f_CopyFileInFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile);
	};

	using CVirtualFSFile = CVirtualFS::CFile;

	template <typename t_CStreamType = NStream::CBinaryStreamDefault>
	struct TCBinaryStream_VirtualFSFilePtr : public t_CStreamType
	{
		DMibStreamImplementOperators(TCBinaryStream_VirtualFSFilePtr);

		TCBinaryStream_VirtualFSFilePtr();
		TCBinaryStream_VirtualFSFilePtr(CVirtualFSFile *_pFile);
		TCBinaryStream_VirtualFSFilePtr(CVirtualFSFile &_File);
		void f_Open(CVirtualFSFile *_pFile);
		void f_Open(CVirtualFSFile &_File);
		void f_Reset();
		void f_Flush(bool _bLocalCacheOnly);
		void f_SetCacheSize(umint _CacheSize);
		void f_FeedBytes(void const *_pMem, umint _nBytes);
		void f_ConsumeBytes(void *_pMem, umint _nBytes);
		bool f_IsValid() const;
		bool f_IsAtEndOfStream() const;
		NStream::CFilePos f_GetPosition() const;
		void f_SetPosition(NStream::CFilePos _Pos);
		void f_SetPositionFromEnd(NStream::CFilePos _Pos);
		void f_AddPosition(NStream::CFilePos _Pos);
		bool f_IsValidReadPosition(NStream::CFilePos _Pos) const;
		NStream::CFilePos f_GetLength() const;
		umint f_ContainerLengthLimit() const;
		void f_SetLength(NStream::CFilePos _Length);

	private:
		CVirtualFSFile *mp_pFile;

	protected:
		DMibStreamImplementProtected(TCBinaryStream_VirtualFSFilePtr);
	};

	template <typename t_CStreamType = NStream::CBinaryStreamDefault>
	struct TCBinaryStream_VirtualFSFile : public t_CStreamType
	{
		DMibStreamImplementOperators(TCBinaryStream_VirtualFSFile);

		TCBinaryStream_VirtualFSFile();
		~TCBinaryStream_VirtualFSFile();
		CVirtualFSFile &f_GetVFSFile();
		void f_Reset();
		void f_Flush(bool _bLocalCacheOnly);
		void f_SetCacheSize(umint _CacheSize);
		void f_FeedBytes(void const *_pMem, umint _nBytes);
		void f_ConsumeBytes(void *_pMem, umint _nBytes);
		bool f_IsValid() const;
		bool f_IsAtEndOfStream() const;
		NStream::CFilePos f_GetPosition() const;
		void f_SetPosition(NStream::CFilePos _Pos);
		void f_SetPositionFromEnd(NStream::CFilePos _Pos);
		void f_AddPosition(NStream::CFilePos _Pos);
		bool f_IsValidReadPosition(NStream::CFilePos _Pos) const;
		NStream::CFilePos f_GetLength() const;
		umint f_ContainerLengthLimit() const;
		void f_SetLength(NStream::CFilePos _Length);

	private:
		CVirtualFSFile mp_File;

	protected:
		DMibStreamImplementProtected(TCBinaryStream_VirtualFSFile);
	};
}

#ifndef DMibPNoShortCuts
using namespace NMib::NFile;
#endif

#include "Malterlib_File_MalterlibFS.hpp"
