// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/Core/Core>
#include <Mib/String/MixedPreserve>

namespace NMib::NFile
{
	class CVirtualFS_FixFSFileTree;
	class CVirtualFS
	{
	public:

		class CFile;
		typedef uint64 CClusterID;
		typedef uint64 CDirectoryID;
		typedef uint64 CFilePos;
	private:

		friend class CVirtualFS_FixFSFileTree;

		friend class CFile;


		class CRootData
		{
		public:
			CRootData()
			{
				m_ClusterSize = 2048;			// Cluster size minimum cluster size is 128
				m_RootFileCluster = 0;		// Cluster at which the root directory resides
				m_FirstFreeCluster = 0;		// First free cluster
				m_FileSystemGrowSize = 512*1024;	// We grow the master file with this much
				m_nFreeClusters = 0;			// Max Cluster Size
				m_DirectoryCacheSize = 128; // How many directories to cache
				m_ClusterCacheSize = 128; // How many clusters to cache
				m_CurrentDirectoryID = 0;
				m_Version = 0x103;
			}
			NCryptography::CHashDigest_MD5 m_MD5Checksum;		// Validate the data
			uint64 m_ClusterSize;			// Cluster size minimum cluster size is 128
			CClusterID m_RootFileCluster;		// Cluster at which the root directory resides
			CClusterID m_FirstFreeCluster;		// First free cluster
			uint64 m_FileSystemGrowSize;	// We grow the master file with this much
			uint64 m_nFreeClusters;			// Max Cluster Size
			uint64 m_DirectoryCacheSize; // How many directories to cache
			uint64 m_ClusterCacheSize; // How many clusters to cache
			CDirectoryID m_CurrentDirectoryID;
			uint32 m_Version;

			void f_WriteInternal(NStream::CBinaryStream &_Stream)
			{
				_Stream << m_ClusterSize;
				_Stream << m_RootFileCluster;
				_Stream << m_FirstFreeCluster;
				_Stream << m_FileSystemGrowSize;
				_Stream << m_nFreeClusters;
				_Stream << m_DirectoryCacheSize;
				_Stream << m_ClusterCacheSize;
				_Stream << m_CurrentDirectoryID;
			}

			void f_Write(NStream::CBinaryStream &_Stream)
			{
				_Stream << ch8('I');
				_Stream << ch8('D');
				_Stream << ch8('S');
				_Stream << ch8('F');
				_Stream << ch8('S');
				_Stream << ch8('0' + ((m_Version >> 8) & 0xf));
				_Stream << ch8('0' + ((m_Version >> 4) & 0xf));
				_Stream << ch8('0' + ((m_Version >> 0) & 0xf));
				_Stream << m_MD5Checksum;
				f_WriteInternal(_Stream);
			}

			void f_Read(NStream::CBinaryStream &_Stream)
			{
				try
				{
					ch8 Signature[8];
					_Stream >> Signature[0];
					_Stream >> Signature[1];
					_Stream >> Signature[2];
					_Stream >> Signature[3];
					_Stream >> Signature[4];
					_Stream >> Signature[5];
					_Stream >> Signature[6];
					_Stream >> Signature[7];
					m_Version = Signature[5] - '0';
					m_Version <<= 4;
					m_Version |= Signature[6] - '0';
					m_Version <<= 4;
					m_Version |= Signature[7] - '0';
					_Stream >> m_MD5Checksum;
					_Stream >> m_ClusterSize;
					_Stream >> m_RootFileCluster;
					_Stream >> m_FirstFreeCluster;
					_Stream >> m_FileSystemGrowSize;
					_Stream >> m_nFreeClusters;
					_Stream >> m_DirectoryCacheSize;
					_Stream >> m_ClusterCacheSize;
					_Stream >> m_CurrentDirectoryID;
				}
				catch (NException::CException)
				{
					m_MD5Checksum = NCryptography::CHashDigest_MD5();
				}
			}

			void f_GenerateChecksum()
			{
				NCryptography::CHash_MD5 Checksum;
				NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
				Stream.f_Open(Checksum);

				Stream << (uint32)0xa042132f;
				f_WriteInternal(Stream);

				m_MD5Checksum = Checksum;
			}

			bool f_Validate()
			{
				NCryptography::CHash_MD5 Checksum;
				NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
				Stream.f_Open(Checksum);

				Stream << (uint32)0xa042132f;
				f_WriteInternal(Stream);

				return m_MD5Checksum == Checksum;
			}
		};

		NStream::CBinaryStream *mp_pStorageStream;
		CRootData mp_RootData;

		uint64 mp_ClusterIDsPerCluster;
		bool mp_bOutOfSpace;

		void fp_SaveRootData();

		inline_small CDirectoryID fp_GetDirectoryID()
		{
			CDirectoryID DirectoryID = ++mp_RootData.m_CurrentDirectoryID;
			fp_SaveRootData();
			return DirectoryID;
		}


		inline_small CFilePos fp_GetFilePos(CClusterID _Cluster)
		{
			return mp_RootData.m_ClusterSize * _Cluster;
		}

		void fp_SetStreamCluster(CClusterID _Cluster)
		{
			mp_pStorageStream->f_SetPosition(fp_GetFilePos(_Cluster));
		}

		inline_small CClusterID fp_GetClusterID(CFilePos _Position, CFilePos &_Remaining)
		{
			CClusterID Cluster = _Position / mp_RootData.m_ClusterSize;
			_Remaining = _Position - Cluster * mp_RootData.m_ClusterSize;
			return Cluster;
		}
		class CFileRecord
		{
		public:
			NCryptography::CHashDigest_MD5 m_MD5Checksum;		// Validate the data
			NTime::CTime m_CreationTime;
			NTime::CTime m_AccessTime;
			NTime::CTime m_WriteTime;
			uint64 m_FileSize;
			uint64 m_FileAttributes;
			CClusterID m_FirstFileChainCluster;	// At this cluster we store the cluster chain for the file data with inline linking to other clusters
			CDirectoryID m_ParentDirectory;
			// 96 bytes

			void f_Reset()
			{
				m_WriteTime = m_AccessTime = m_CreationTime = NTime::CTime::fs_NowUTC();
				m_FileSize = 0;
				m_FileAttributes = 0;
				m_FirstFileChainCluster = 0;
				m_ParentDirectory = 0;
			}

			void f_GenerateChecksum(uint64 _ClusterID, uint32 _Version)
			{
				NCryptography::CHash_MD5 Checksum;
				NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
				Stream.f_Open(Checksum);

				Stream << (uint32)0x54EB5A03;
				if (_Version >= 0x103)
				{
					Stream << _ClusterID;
					Stream << _Version;
				}
				f_WriteInternal(Stream);

				m_MD5Checksum = Checksum;
			}

			bool f_Validate(uint64 _ClusterID, uint32 _Version)
			{
				NCryptography::CHash_MD5 Checksum;
				NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
				Stream.f_Open(Checksum);

				Stream << (uint32)0x54EB5A03;
				if (_Version >= 0x103)
				{
					Stream << _ClusterID;
					Stream << _Version;
				}
				f_WriteInternal(Stream);

				return m_MD5Checksum == Checksum;
			}

			void f_WriteInternal(NStream::CBinaryStream &_Stream)
			{
				_Stream << m_CreationTime;
				_Stream << m_AccessTime;
				_Stream << m_WriteTime;
				_Stream << m_FileSize;
				_Stream << m_FileAttributes;
				_Stream << m_FirstFileChainCluster;
				_Stream << m_ParentDirectory;
			}

			void f_Write(NStream::CBinaryStream &_Stream)
			{
				_Stream << m_MD5Checksum;
				f_WriteInternal(_Stream);
			}

			void f_Read(NStream::CBinaryStream &_Stream)
			{
				try
				{
					_Stream >> m_MD5Checksum;
					_Stream >> m_CreationTime;
					_Stream >> m_AccessTime;
					_Stream >> m_WriteTime;
					_Stream >> m_FileSize;
					_Stream >> m_FileAttributes;
					_Stream >> m_FirstFileChainCluster;
					_Stream >> m_ParentDirectory;
				}
				catch (NException::CException)
				{
					m_MD5Checksum = NCryptography::CHashDigest_MD5();
				}
			}
		};

		class CClusterChain
		{
		public:
			NCryptography::CHashDigest_MD5 m_MD5Checksum;		// Validate the data
			CClusterID m_NextChainCluster;
			CClusterID m_ClusterFirstIDIndex;
			uint64 m_nClusterIDs;
			NContainer::TCVector<CClusterID> m_ClusterIDs;

			void f_GenerateChecksum(uint64 _ClusterID, uint32 _Version)
			{
				NCryptography::CHash_MD5 Checksum;
				NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
				Stream.f_Open(Checksum);

				Stream << (uint32)0x8d8610e6;
				if (_Version >= 0x103)
				{
					Stream << _ClusterID;
					Stream << _Version;
				}
				f_WriteInternal(Stream);

				m_MD5Checksum = Checksum;
			}

			bool f_Validate(uint64 _ClusterID, uint32 _Version)
			{
				NCryptography::CHash_MD5 Checksum;
				NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
				Stream.f_Open(Checksum);

				Stream << (uint32)0x8d8610e6;
				if (_Version >= 0x103)
				{
					Stream << _ClusterID;
					Stream << _Version;
				}
				f_WriteInternal(Stream);

				return m_MD5Checksum == Checksum;
			}

			void f_WriteInternal(NStream::CBinaryStream &_Stream)
			{
				_Stream << m_NextChainCluster;
				_Stream << m_ClusterFirstIDIndex;
				_Stream << m_nClusterIDs;
				if (m_nClusterIDs > m_ClusterIDs.f_GetLen())
					return;
				for (mint i = 0; i < m_nClusterIDs; ++i)
				{
					_Stream << m_ClusterIDs[i];
				}
			}

			void f_Write(NStream::CBinaryStream &_Stream)
			{
				_Stream << m_MD5Checksum;
				f_WriteInternal(_Stream);
			}

			void f_Read(NStream::CBinaryStream &_Stream)
			{
				try
				{
					_Stream >> m_MD5Checksum;
					_Stream >> m_NextChainCluster;
					_Stream >> m_ClusterFirstIDIndex;
					_Stream >> m_nClusterIDs;
					if (m_nClusterIDs > m_ClusterIDs.f_GetLen())
					{
						m_MD5Checksum = NCryptography::CHashDigest_MD5();
						return;
					}

					for (mint i = 0; i < m_nClusterIDs; ++i)
					{
						_Stream >> m_ClusterIDs[i];
					}
				}
				catch (NException::CException)
				{
					m_MD5Checksum = NCryptography::CHashDigest_MD5();
				}
			}
		};

		public:
		class CDirectory
		{
		public:
			NCryptography::CHashDigest_MD5 m_MD5Checksum;		// Validate the data
			CDirectoryID m_DirectoryID;

			class CFile
			{
			public:
				CClusterID m_FileRecordCluster;
				NStr::CMStrPreserve m_FileName;

				void f_FeedPreserve(NStream::CBinaryStream &_Stream) const
				{
					_Stream << m_FileRecordCluster;
					_Stream << m_FileName;
				}

				void f_FeedNormal(NStream::CBinaryStream &_Stream) const
				{
					_Stream << m_FileRecordCluster;
					NStr::CMStrPreserve ToSave(m_FileName);
					ToSave.m_MixedType = NStr::EMStr_Type_CStr;
					ToSave.m_OriginalType = NStr::EStrType_UTF;
					_Stream << ToSave;
				}

				void f_Consume(NStream::CBinaryStream &_Stream)
				{
					_Stream >> m_FileRecordCluster;
					_Stream >> m_FileName;
				}
			};
			NContainer::TCVector<CFile> m_Files;

			void f_GenerateChecksum(uint64 _ClusterID, uint32 _Version)
			{
				NCryptography::CHash_MD5 Checksum;
				NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
				Stream.f_Open(Checksum);

				Stream << (uint32)0x012F3862;
				if (_Version >= 0x103)
				{
					Stream << _ClusterID;
					Stream << _Version;
				}
				f_WriteInternal(Stream);

				m_MD5Checksum = Checksum;
			}

			void f_AddChild(NStr::CStr const &_Path, CClusterID _ClusterID)
			{
				CFile File;
				File.m_FileRecordCluster = _ClusterID;
				File.m_FileName = _Path;
				m_Files.f_Insert(File);
			}

			bool f_Validate(uint64 _ClusterID, uint32 _Version)
			{
				NCryptography::CHash_MD5 Checksum;
				NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
				Stream.f_Open(Checksum);

				Stream << (uint32)0x012F3862;
				if (_Version >= 0x103)
				{
					Stream << _ClusterID;
					Stream << _Version;
				}
				if (_Version == 0x101)
				{
					uint32 nFiles = m_Files.f_GetLen();
					for (mint i = 0; i < nFiles; ++i)
					{
						m_Files[i].m_FileName.m_OriginalType = NStr::EStrType_Ansi;
					}
					f_WriteInternalValidate(Stream);
					for (mint i = 0; i < nFiles; ++i)
					{
						m_Files[i].m_FileName.m_OriginalType = NStr::EStrType_Unicode;
					}
				}
				else
					f_WriteInternalValidate(Stream);

				return m_MD5Checksum == Checksum;
			}

			void f_WriteInternalValidate(NStream::CBinaryStream &_Stream)
			{
				_Stream << m_DirectoryID;
				uint32 nFiles = m_Files.f_GetLen();
				_Stream << nFiles;
				for (auto &File : m_Files)
					File.f_FeedPreserve(_Stream);
			}

			void f_WriteInternal(NStream::CBinaryStream &_Stream)
			{
				_Stream << m_DirectoryID;
				uint32 nFiles = m_Files.f_GetLen();
				_Stream << nFiles;
				for (auto &File : m_Files)
					File.f_FeedNormal(_Stream);
			}

			void f_Write(NStream::CBinaryStream &_Stream)
			{
				_Stream << m_MD5Checksum;
				f_WriteInternal(_Stream);
			}

			void f_Read(NStream::CBinaryStream &_Stream)
			{
				try
				{
					_Stream >> m_MD5Checksum;
					_Stream >> m_DirectoryID;

					uint32 nFiles;
					_Stream >> nFiles;
					mint MaxFiles = ((_Stream.f_GetLength() - _Stream.f_GetPosition()) / (sizeof(CClusterID) + sizeof(uint32)));
					if (nFiles > MaxFiles)
					{
						m_Files.f_Clear();
						m_MD5Checksum = NCryptography::CHashDigest_MD5();
						return ;
					}
					m_Files.f_SetLen(nFiles);
					for (mint i = 0; i < nFiles; ++i)
					{
						_Stream >> m_Files[i];
					}
				}
				catch (NException::CException)
				{
					m_Files.f_Clear();
					m_MD5Checksum = NCryptography::CHashDigest_MD5();
				}
			}

			CClusterID f_GetChild(NStr::CStr const &_Child)
			{
				for (mint i = 0; i < m_Files.f_GetLen(); ++i)
				{
					if (m_Files[i].m_FileName == _Child)
						return m_Files[i].m_FileRecordCluster;
				}

				return 0;
			}

			void f_RemoveChild(CClusterID _Child)
			{
				for (mint i = 0; i < m_Files.f_GetLen(); ++i)
				{
					if (m_Files[i].m_FileRecordCluster == _Child)
					{
						m_Files.f_Remove(i);
						return;
					}
				}
			}

		};
		private:

		class CFreeCluster
		{
		public:
			NCryptography::CHashDigest_MD5 m_MD5Checksum; // Validate the data
			CClusterID m_NextFreeCluster; // Next free cluster

			void f_WriteInternal(NStream::CBinaryStream &_Stream)
			{
				_Stream << m_NextFreeCluster;
			}

			void f_Write(NStream::CBinaryStream &_Stream)
			{
				_Stream << m_MD5Checksum;
				f_WriteInternal(_Stream);
			}

			void f_Read(NStream::CBinaryStream &_Stream)
			{
				try
				{
					_Stream >> m_MD5Checksum;
					_Stream >> m_NextFreeCluster;
				}
				catch (NException::CException)
				{
					m_MD5Checksum = NCryptography::CHashDigest_MD5();
				}
			}

			void f_GenerateChecksum(uint64 _ClusterID, uint32 _Version)
			{
				NCryptography::CHash_MD5 Checksum;
				NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
				Stream.f_Open(Checksum);

				Stream << (uint32)0x052e5468;
				if (_Version >= 0x103)
				{
					Stream << _ClusterID;
					Stream << _Version;
				}
				f_WriteInternal(Stream);

				m_MD5Checksum = Checksum;
			}

			bool f_Validate(uint64 _ClusterID, uint32 _Version)
			{
				NCryptography::CHash_MD5 Checksum;
				NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
				Stream.f_Open(Checksum);

				Stream << (uint32)0x052e5468;
				if (_Version >= 0x103)
				{
					Stream << _ClusterID;
					Stream << _Version;
				}
				f_WriteInternal(Stream);

				return m_MD5Checksum == Checksum;
			}

		};

		CClusterID fp_GetNewCluster();
		void fp_ReturnCluster(CVirtualFS::CClusterID _Cluster);

		class CClusterCacheEntry
		{
		public:
			CClusterID m_ClusterID;
			NContainer::CByteVector m_ClusterMemory;

			DMibListLinkD_Link(CClusterCacheEntry, m_AccessLink);
			//DMibListLinkD_Link(CClusterCacheEntry, m_LazyLink);

			mint m_RefCount;

			bool m_bDirty;

			CClusterCacheEntry()
			{
				m_bDirty = false;
				m_RefCount = 0;
			}

			class CCompare
			{
			public:
				inline_small const CClusterID &operator () (CClusterCacheEntry const &_Node) const
				{
					return _Node.m_ClusterID;
				}
			};

			NIntrusive::TCAVLLink<> m_CacheLink;

			void f_MakeDirty()
			{
				m_bDirty = true;
			}

			void f_OpenStream(NStream::CBinaryStreamMemoryPtr<> &_Stream)
			{
				mint Len = m_ClusterMemory.f_GetLen();
				_Stream.f_OpenReadWrite(m_ClusterMemory.f_GetArray(), Len, Len);
			}

			void f_CloseStream(bool _bWritten)
			{
				if (_bWritten)
					f_MakeDirty();
			}
		};

		class CClusterCache
		{
		public:

			CClusterCache()
			{
				m_nCached = 0;
				m_nCachedMax = 0;
			}


			void f_Create(CVirtualFS * _pVirtualFS)
			{
				m_pVirtualFS = _pVirtualFS;
			}

			~CClusterCache()
			{
				f_Destroy();
			}

			void f_Destroy()
			{
				DMibSafeCheck(m_InUse.f_IsEmpty(), "This should not happed. Did you close all files?");

				f_Flush();
				m_Tree.f_DeleteAllDefiniteType();
			}

			void f_Flush(CClusterCacheEntry *_pCluster)
			{
				if (_pCluster->m_bDirty)
				{
					m_pVirtualFS->fp_SetStreamCluster(_pCluster->m_ClusterID);
					m_pVirtualFS->mp_pStorageStream->f_FeedBytes(_pCluster->m_ClusterMemory.f_GetArray(), _pCluster->m_ClusterMemory.f_GetLen());
					m_pVirtualFS->mp_pStorageStream->f_Flush(true);
					_pCluster->m_bDirty = false;
				}
			}

			void f_Flush()
			{
				auto Iter = m_InUse.f_GetIterator();

				while (Iter)
				{
					if (Iter->m_bDirty)
						f_Flush(Iter);

					++Iter;
				}
			}

			CVirtualFS *m_pVirtualFS;

			NIntrusive::TCAVLTree<&CClusterCacheEntry::m_CacheLink, CClusterCacheEntry::CCompare> m_Tree;
			DMibListLinkD_List(CClusterCacheEntry, m_AccessLink) m_Cached;
			DMibListLinkD_List(CClusterCacheEntry, m_AccessLink) m_InUse;
			// DMibListLinkD_List(CClusterCacheEntry, m_LazyLink) m_LazyOrder; Disable lazy writing for now
			mint m_nCached;
			mint m_nCachedMax;

			void f_SetCacheSize(mint _nCachedMax)
			{
				m_nCachedMax = _nCachedMax;
				f_MakeSpace(0);
			}

			void f_RemoveCacheEntry(CClusterCacheEntry *_pEntry)
			{
				m_Tree.f_Remove(_pEntry);
				--m_nCached;
				fg_DeleteObject(NMemory::CDefaultAllocator(), _pEntry);
			}

			void f_MakeSpace(mint _Space)
			{
				while ((m_nCached + _Space) > m_nCachedMax)
				{
					CClusterCacheEntry *pEntry = m_Cached.f_GetLast();

					if (!pEntry)
						return; // nothing we can do no more unused cache entries to delete

					f_RemoveCacheEntry(pEntry);
				}
			}

			CClusterCacheEntry *f_GetCluster(CClusterID _Cluster)
			{
				CClusterCacheEntry *pCache = m_Tree.f_FindEqual(_Cluster);

				if (!pCache)
				{
					f_MakeSpace(1);
					NStorage::TCUniquePointer<CClusterCacheEntry> pCacheNew(fg_Construct());
					pCacheNew->m_ClusterID = _Cluster;
					pCacheNew->m_ClusterMemory.f_SetLen(m_pVirtualFS->mp_RootData.m_ClusterSize);
					m_pVirtualFS->fp_SetStreamCluster(_Cluster);
					m_pVirtualFS->mp_pStorageStream->f_ConsumeBytes(pCacheNew->m_ClusterMemory.f_GetArray(), pCacheNew->m_ClusterMemory.f_GetLen());
					pCache = pCacheNew.f_Detach();
					m_Tree.f_Insert(pCache);
					++m_nCached;
				}

				m_InUse.f_InsertFirst(pCache);
				++pCache->m_RefCount;
				return pCache;
			}

			void f_ReturnCluster(CClusterCacheEntry *_pCluster)
			{
				if (_pCluster->m_RefCount == 1)
				{
					if (_pCluster->m_bDirty)
						f_Flush(_pCluster);

					--_pCluster->m_RefCount;
					m_Cached.f_InsertFirst(_pCluster);
				}
			}
		};

		CClusterCache mp_ClusterCache;

		class CDirectoryCacheEntry
		{
		public:

			NStr::CStr m_FullPath;
			CDirectory m_Directory;
			CClusterID m_DirectoryFileClusterID;
			DMibListLinkD_Link(CDirectoryCacheEntry, m_AccessLink);
			bool m_bDirty;
			mint m_RefCount;

			CDirectoryCacheEntry()
			{
				m_bDirty = false;
				m_RefCount = 0;
			}

			class CCompare
			{
			public:
				inline_small NStr::CStr const &operator () (CDirectoryCacheEntry const &_Node) const
				{
					return _Node.m_FullPath;
				}
			};

			NIntrusive::TCAVLLink<> m_CacheLink;
		};

		class CDirectoryCache
		{
		public:

			CDirectoryCache()
			{
				m_nCached = 0;
				m_nCachedMax = 0;
			}

			~CDirectoryCache()
			{
				f_Destroy();
			}

			void f_Destroy()
			{
				DMibSafeCheck(m_InUse.f_IsEmpty(), "This should not happed. Did you close all files?");
				f_Flush();
				m_Tree.f_DeleteAllDefiniteType();
			}

			void f_Flush()
			{
				auto Iter = m_InUse.f_GetIterator();

				while (Iter)
				{
					if (Iter->m_bDirty)
						f_Flush(Iter);
					++Iter;
				}
			}

			void f_Flush(CDirectoryCacheEntry *_pEntry)
			{
				if (_pEntry->m_bDirty)
				{
					CVirtualFS::CFileInternal *pFile = m_pVirtualFS->fp_OpenFile(_pEntry->m_DirectoryFileClusterID);
					if (!pFile)
						DMibErrorFile("Could not open directory file");

					{
						CBinaryStreamFileInternal Stream;
						Stream.f_Open(pFile);
						_pEntry->m_Directory.f_GenerateChecksum(_pEntry->m_DirectoryFileClusterID, m_pVirtualFS->mp_RootData.m_Version);
						_pEntry->m_Directory.f_Write(Stream);
						CMibFilePos Length = Stream.f_GetPosition();
						pFile->f_SetLength(Length);
					}

					m_pVirtualFS->fp_CloseFile(pFile);
					_pEntry->m_bDirty = false;
				}
			}

			void f_Create(CVirtualFS * _pVirtualFS)
			{
				m_pVirtualFS = _pVirtualFS;
			}

			CVirtualFS *m_pVirtualFS;

			NIntrusive::TCAVLTree<&CDirectoryCacheEntry::m_CacheLink, CDirectoryCacheEntry::CCompare> m_Tree;
			DMibListLinkD_List(CDirectoryCacheEntry, m_AccessLink) m_Cached;
			DMibListLinkD_List(CDirectoryCacheEntry, m_AccessLink) m_InUse;
			mint m_nCached;
			mint m_nCachedMax;

			CDirectoryCacheEntry *f_GetNewCacheEntry(CDirectoryCacheEntry *_pParent, NStr::CStr const &_Path);
			CDirectoryCacheEntry *f_GetExistingCacheEntry(CClusterID _DirectoryCluster, NStr::CStr const &_Path);

			CDirectoryCacheEntry *f_GetCacheEntry(CDirectoryCacheEntry *_pParentEntry, NStr::CStr const &_CurrentPath)
			{
				NStr::CStr CurrentPath = _CurrentPath;

				CClusterID DirectoryCluster;
				CDirectoryCacheEntry *pCache = nullptr;

				if (_pParentEntry)
				{
					NStr::CStr ToFind;
					int iPath = CurrentPath.f_FindChar('/');

					if (iPath >= 0)
					{
						ToFind = CurrentPath.f_Left(iPath);
						CurrentPath = CurrentPath.f_Extract(iPath + 1);
					}
					else
					{
						ToFind = CurrentPath;
						CurrentPath.f_Clear();
					}
					DirectoryCluster = _pParentEntry->m_Directory.f_GetChild(ToFind);
					if (DirectoryCluster)
					{
						if (_pParentEntry->m_FullPath != "")
							pCache = f_GetExistingCacheEntry(DirectoryCluster, _pParentEntry->m_FullPath + "/" + ToFind);
						else
							pCache = f_GetExistingCacheEntry(DirectoryCluster, ToFind);
					}
				}
				else
				{
					DirectoryCluster = m_pVirtualFS->mp_RootData.m_RootFileCluster;
					pCache = f_GetExistingCacheEntry(DirectoryCluster, "");
				}

				if (!pCache)
					return _pParentEntry;

				if (CurrentPath.f_IsEmpty())
					return pCache;

				return f_GetCacheEntry(pCache, CurrentPath);
			}

			void f_ReturnCacheEntry(CDirectoryCacheEntry *_pEntry)
			{
				if ((--_pEntry->m_RefCount) == 0)
				{
					if (_pEntry->m_bDirty)
						f_Flush(_pEntry);

					m_Cached.f_InsertFirst(_pEntry);
				}
			}

			CDirectoryCacheEntry *f_GetDirectoryCommon(NStr::CStr const &_Path)
			{
				// First try to get the closest directory we have in cache
				CDirectoryCacheEntry *pCache = nullptr;

				NStr::CStr CurrentPath = _Path;

				NStr::CStr Find = _Path;
				while (1)
				{
					pCache = m_Tree.f_FindEqual(Find);

					if (pCache)
					{
						if (Find != "")
							CurrentPath = _Path.f_Extract(Find.f_GetLen()+1);
						else
							CurrentPath = _Path;

						break;
					}
					aint iFind = Find.f_FindCharReverse('/');
					if (iFind >= 0)
						Find = Find.f_Left(iFind);
					else
					{
						if (Find == "")
							break;
						Find = "";
					}
				}

				pCache = f_GetCacheEntry(pCache, CurrentPath);
				++pCache->m_RefCount;
				m_InUse.f_InsertFirst(pCache);

				return pCache;
			}

			CDirectoryCacheEntry *f_GetDirectory(NStr::CStr const &_Path)
			{
				CDirectoryCacheEntry *pCache = f_GetDirectoryCommon(_Path);

				if (pCache->m_FullPath != _Path)
				{
					f_ReturnCacheEntry(pCache);
					return nullptr;
				}
				return pCache;
			}

			CDirectoryCacheEntry *f_GetNewDirectory(NStr::CStr const &_Path)
			{
				CDirectoryCacheEntry *pCache = f_GetDirectoryCommon(_Path);

				if (pCache->m_FullPath.f_GetLen() < _Path.f_GetLen())
				{
					NStr::CStr CurrentPath;
					if (pCache->m_FullPath != "")
						CurrentPath = _Path.f_Extract(pCache->m_FullPath.f_GetLen()+1);
					else
						CurrentPath = _Path;

					while (pCache && !CurrentPath.f_IsEmpty() )
					{
						NStr::CStr ToFind;
						int iPath = CurrentPath.f_FindChar('/');

						if (iPath >= 0)
						{
							ToFind = CurrentPath.f_Left(iPath);
							CurrentPath = CurrentPath.f_Extract(iPath + 1);
						}
						else
						{
							ToFind = CurrentPath;
							CurrentPath.f_Clear();
						}
						CClusterID Cluster = pCache->m_Directory.f_GetChild(ToFind);

						if (Cluster)
						{
							// This must have been a file
							f_ReturnCacheEntry(pCache);
							pCache = nullptr;
							break;
						}

						CDirectoryCacheEntry *pNewCache = f_GetNewCacheEntry(pCache, ToFind);
						f_ReturnCacheEntry(pCache);
						pCache = pNewCache;
					}
				}

				return pCache;
			}

			void f_ReturnDirectory(CDirectoryCacheEntry *_pEntry)
			{
				f_ReturnCacheEntry(_pEntry);
			}

			void f_RemoveCacheEntry(CDirectoryCacheEntry *_pEntry)
			{
				DMibSafeCheck(!_pEntry->m_bDirty, "Should not be dirty");
				DMibSafeCheck(_pEntry->m_RefCount == 0, "Should not be dirty");

				m_Tree.f_Remove(_pEntry);
				--m_nCached;
				fg_DeleteObject(NMemory::CDefaultAllocator(), _pEntry);
			}

			void f_MakeSpace(mint _Space)
			{
				while ((m_nCached + _Space) > m_nCachedMax)
				{
					CDirectoryCacheEntry *pEntry = m_Cached.f_GetLast();

					if (!pEntry)
						return; // nothing we can do no more unused cache entries to delete

					f_RemoveCacheEntry(pEntry);
				}
			}

			void f_SetCacheSize(mint _Size)
			{
				m_nCachedMax = _Size;
				f_MakeSpace(0);
			}
		};

		CDirectoryCache mp_DirectoryCache;


		class CFileInternal
		{
		public:
			CVirtualFS *m_pVirtualFS;

			mint m_RefCount;
			CClusterID m_FileDescriptorID; // This is the guid for the file
			CFileRecord m_FileRecord; // The file record. The records lives here while the file is open
			CClusterChain m_ClusterChain; // The chain of data clusters
			CClusterID m_ClusterChainCluster;
			bool m_bValidClusterChain;
			bool m_bClusterChainDirty;
			bool m_bFileRecordDirty;
			bool m_bDestroyException;

			class CCompare
			{
			public:
				inline_small const CClusterID &operator () (CFileInternal const &_Node) const
				{
					return _Node.m_FileDescriptorID;
				}
			};

			NIntrusive::TCAVLLink<> m_CacheLink;

			CFileInternal()
			{
				m_ClusterChain.m_ClusterFirstIDIndex = 0;
				m_ClusterChain.m_nClusterIDs = 0;
				m_bValidClusterChain = false;
				m_nCachedClusters = 0;
				m_bClusterChainDirty = false;
				m_ClusterChainCluster = 0;
				m_bFileRecordDirty = false;
				m_RefCount = 0;
				m_bDestroyException = false;
			}

			~CFileInternal() noexcept(false)
			{
				try
				{
					CClusterCache *pCluster = m_CachedClusters.f_GetLast();
					while (pCluster)
					{
						m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster->m_pClusterCacheEntry);
						fg_DeleteObject(NMemory::CDefaultAllocator(), pCluster);
						pCluster = m_CachedClusters.f_GetLast();
					}
				}
				catch (...)
				{
					if (!m_bDestroyException && !m_pVirtualFS->mp_bOutOfSpace)
					{
						m_bDestroyException = true;
						throw;
					}
					else
					{
						CClusterCache *pCluster = m_CachedClusters.f_GetLast();
						while (pCluster)
						{
							pCluster->m_pClusterCacheEntry->m_bDirty = false;
							m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster->m_pClusterCacheEntry);
							fg_DeleteObject(NMemory::CDefaultAllocator(), pCluster);
							pCluster = m_CachedClusters.f_GetLast();
						}
					}
				}
			}

			class CClusterCache
			{
			public:
				CClusterCache(CClusterCacheEntry *_pClusterCacheEntry, CFilePos _FilePosStart)
				{
					m_pClusterCacheEntry = _pClusterCacheEntry;
					m_FilePosStart = _FilePosStart;
				}

				DMibListLinkDS_Link(CClusterCache, m_Link);
				CClusterCacheEntry *m_pClusterCacheEntry;
				CFilePos m_FilePosStart;
			};

			DMibListLinkDS_List(CClusterCache, m_Link) m_CachedClusters;
			mint m_nCachedClusters;

			CClusterCacheEntry *f_GetCluster(CFilePos &_Position)
			{
				uint64 ClusterSize = m_pVirtualFS->mp_RootData.m_ClusterSize;
				DMibListLinkDS_Iter(CClusterCache, m_Link) Iter = m_CachedClusters;

				while (Iter)
				{
					CClusterCache *pEntry = Iter;
					uint64 End = pEntry->m_FilePosStart + ClusterSize;
					if (_Position >= pEntry->m_FilePosStart && _Position < End)
					{
						m_CachedClusters.f_InsertFirst(pEntry);
						_Position -= pEntry->m_FilePosStart;
						return pEntry->m_pClusterCacheEntry;
					}
					++Iter;
				}

				CFilePos OrgPos = _Position;
				CClusterID ClusterID = f_GetDataCluster(_Position);
				if (!ClusterID)
					return nullptr;
				OrgPos -= _Position;

				if (m_nCachedClusters >= 8) // Cache max 8 clustes
				{
					--m_nCachedClusters;
					CClusterCache *pCluster = m_CachedClusters.f_GetLast();
					m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster->m_pClusterCacheEntry);
					fg_DeleteObject(NMemory::CDefaultAllocator(), pCluster);
				}

				CClusterCacheEntry *pEntry = m_pVirtualFS->mp_ClusterCache.f_GetCluster(ClusterID);

				CClusterCache *pNewEntry = fg_ConstructObject<CClusterCache>(NMemory::CDefaultAllocator(), pEntry, OrgPos);
				m_CachedClusters.f_InsertFirst(pNewEntry);
				++m_nCachedClusters;

				return pEntry;
			}

			void f_Read(void *_pDest, CFilePos _Position, mint _nBytes)
			{
				CFilePos CurrentPosition = _Position;
				uint8 *pDest = (uint8 *)_pDest;

				uint64 ClusterSize = m_pVirtualFS->mp_RootData.m_ClusterSize;

				if (_Position + _nBytes > m_FileRecord.m_FileSize)
				{
					DMibErrorFile("Read would read past end of file");
				}

				while (_nBytes)
				{
					CFilePos Pos = CurrentPosition;
					CClusterCacheEntry *pEntry = f_GetCluster(Pos);
					mint MaxBytes = fg_Min(ClusterSize - Pos, _nBytes);
					NMemory::fg_MemCopy(pDest, pEntry->m_ClusterMemory.f_GetArray() + Pos, MaxBytes);
					pDest += MaxBytes;
					_nBytes -= MaxBytes;
					CurrentPosition += MaxBytes;
				}
			}

			void f_Write(const void *_pSource, CFilePos _Position, mint _nBytes)
			{
				CFilePos CurrentPosition = _Position;
				uint8 *pSrc = (uint8 *)_pSource;

				uint64 ClusterSize = m_pVirtualFS->mp_RootData.m_ClusterSize;

				while (_nBytes)
				{
					CFilePos Pos = CurrentPosition;
					CClusterCacheEntry *pEntry = f_GetCluster(Pos);
					mint MaxBytes = fg_Min(ClusterSize - Pos, _nBytes);
					NMemory::fg_MemCopy(pEntry->m_ClusterMemory.f_GetArray() + Pos, pSrc, MaxBytes);
					pEntry->m_bDirty = true;
					pSrc += MaxBytes;
					_nBytes -= MaxBytes;
					CurrentPosition += MaxBytes;
				}

				if (CurrentPosition > m_FileRecord.m_FileSize)
				{
					m_FileRecord.m_FileSize = CurrentPosition;
					m_bFileRecordDirty = true;
				}
			}

			void f_WriteClusterChain()
			{
				if (!m_bClusterChainDirty)
					return;
				m_bClusterChainDirty = false;

				m_ClusterChain.f_GenerateChecksum(m_ClusterChainCluster, m_pVirtualFS->mp_RootData.m_Version);
				CClusterCacheEntry *pCluster = m_pVirtualFS->mp_ClusterCache.f_GetCluster(m_ClusterChainCluster);
				NStream::CBinaryStreamMemoryPtr<> Stream;
				pCluster->f_OpenStream(Stream);
				m_ClusterChain.f_Write(Stream);
				pCluster->f_CloseStream(true);
				m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster);
			}

			void f_SetLength(uint64 _Length);

			void f_Delete()
			{
				f_Destroy();

				if (m_FileRecord.m_FirstFileChainCluster)
				{
					f_ReadClusterChain(m_FileRecord.m_FirstFileChainCluster);
					m_pVirtualFS->fp_ReturnCluster(m_FileRecord.m_FirstFileChainCluster);
					while (1)
					{
						for (uint64 i = 0; i < m_ClusterChain.m_nClusterIDs; ++i)
						{
							m_pVirtualFS->fp_ReturnCluster(m_ClusterChain.m_ClusterIDs[i]);
						}
						if (m_ClusterChain.m_NextChainCluster)
						{
							CClusterID ToReturn = m_ClusterChain.m_NextChainCluster;
							f_ReadClusterChain(ToReturn);
							m_pVirtualFS->fp_ReturnCluster(ToReturn);
						}
						else
							break;
					}
				}
				m_pVirtualFS->fp_ReturnCluster(m_FileDescriptorID);
			}

			void f_ReadClusterChain(CClusterID _ClusterID)
			{
				if (m_bClusterChainDirty)
					f_Flush();

				m_ClusterChainCluster = _ClusterID;
				CClusterCacheEntry *pCluster = m_pVirtualFS->mp_ClusterCache.f_GetCluster(_ClusterID);
				NStream::CBinaryStreamMemoryPtr<> Stream;
				pCluster->f_OpenStream(Stream);
				m_ClusterChain.f_Read(Stream);
				pCluster->f_CloseStream(false);
				m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster);
				m_bValidClusterChain = true;

				if (!m_ClusterChain.f_Validate(_ClusterID, m_pVirtualFS->mp_RootData.m_Version))
				{
					DMibErrorFile("Cluster chain record checksum failure");
				}
			}

			CClusterID f_GetDataCluster(CFilePos &_FilePosition)
			{
				CClusterID ClusterIndex = m_pVirtualFS->fp_GetClusterID(_FilePosition, _FilePosition);

				uint64 ClusterIDsPerCluster = m_pVirtualFS->mp_ClusterIDsPerCluster;
//					CClusterID ClusterDataLocation = ClusterIndex / ClusterIDsPerCluster;
				CClusterID IndexInChunk = ClusterIndex - ((ClusterIndex / ClusterIDsPerCluster) * ClusterIDsPerCluster);

				CClusterID LastIndex = m_ClusterChain.m_ClusterFirstIDIndex + m_ClusterChain.m_nClusterIDs;
				if (ClusterIndex >= m_ClusterChain.m_ClusterFirstIDIndex && ClusterIndex < LastIndex)
				{
					// We already have the cluster in question in memory
					return m_ClusterChain.m_ClusterIDs[IndexInChunk];
				}
				else if ((ClusterIndex < m_ClusterChain.m_ClusterFirstIDIndex || !m_bValidClusterChain) && m_FileRecord.m_FirstFileChainCluster)
				{
					// We need to start from beginning and work our way through some clusters
					f_ReadClusterChain(m_FileRecord.m_FirstFileChainCluster);
				}
				else if (!m_FileRecord.m_FirstFileChainCluster)
				{
					CClusterID NewCluster = m_pVirtualFS->fp_GetNewCluster();
					m_FileRecord.m_FirstFileChainCluster = NewCluster;
					m_ClusterChain.m_NextChainCluster = 0;
					m_ClusterChain.m_nClusterIDs = 0;
					m_ClusterChain.m_ClusterFirstIDIndex = 0;
					m_bValidClusterChain = true;
					m_bClusterChainDirty = true;
					m_bFileRecordDirty = true;
					m_ClusterChainCluster = NewCluster;
				}

				while (ClusterIndex >= (m_ClusterChain.m_ClusterFirstIDIndex + ClusterIDsPerCluster))
				{
					if (!m_ClusterChain.m_NextChainCluster)
					{
						CClusterID NewCluster = m_pVirtualFS->fp_GetNewCluster();
						m_ClusterChain.m_NextChainCluster = NewCluster;
						for (uint64 i = m_ClusterChain.m_nClusterIDs; i < ClusterIDsPerCluster; ++i)
						{
							m_ClusterChain.m_ClusterIDs[i] = m_pVirtualFS->fp_GetNewCluster();
						}
						m_ClusterChain.m_nClusterIDs = ClusterIDsPerCluster;
						m_bClusterChainDirty = true;
						f_WriteClusterChain();

						m_ClusterChainCluster = NewCluster;
						m_ClusterChain.m_nClusterIDs = 0;
						m_ClusterChain.m_NextChainCluster = 0;
						m_ClusterChain.m_ClusterFirstIDIndex += ClusterIDsPerCluster;
						m_bClusterChainDirty = true;
					}
					else
						f_ReadClusterChain(m_ClusterChain.m_NextChainCluster);
				}

				if (IndexInChunk >= m_ClusterChain.m_nClusterIDs)
				{
					for (uint64 i = m_ClusterChain.m_nClusterIDs; i <= IndexInChunk; ++i)
					{
						m_ClusterChain.m_ClusterIDs[i] = m_pVirtualFS->fp_GetNewCluster();
					}
					m_ClusterChain.m_nClusterIDs = IndexInChunk+1;
					m_bClusterChainDirty = true;
				}

				LastIndex = m_ClusterChain.m_ClusterFirstIDIndex + m_ClusterChain.m_nClusterIDs;
				if (!(ClusterIndex >= m_ClusterChain.m_ClusterFirstIDIndex && ClusterIndex < LastIndex))
					DMibErrorFile("Cluster chain error");

				f_Flush();

				return m_ClusterChain.m_ClusterIDs[IndexInChunk];
			}

			void f_Create(CVirtualFS *_pVirtualFS, CClusterID _FileDescriptorID, bool _bLoad)
			{
				m_FileDescriptorID = _FileDescriptorID;
				m_pVirtualFS = _pVirtualFS;

				m_ClusterChain.m_ClusterIDs.f_SetLen(m_pVirtualFS->mp_ClusterIDsPerCluster);

				if (_bLoad)
				{
					CClusterCacheEntry *pCluster = m_pVirtualFS->mp_ClusterCache.f_GetCluster(m_FileDescriptorID);
					NStream::CBinaryStreamMemoryPtr<> Stream;
					pCluster->f_OpenStream(Stream);
					m_FileRecord.f_Read(Stream);
					pCluster->f_CloseStream(false);
					m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster);

					if (!m_FileRecord.f_Validate(_FileDescriptorID, m_pVirtualFS->mp_RootData.m_Version))
					{
						DMibErrorFile("File record checksum failure");
					}
				}
			}

			void f_WriteFileRecord()
			{
				if (m_bFileRecordDirty)
				{
					m_bFileRecordDirty = false;
					m_FileRecord.f_GenerateChecksum(m_FileDescriptorID, m_pVirtualFS->mp_RootData.m_Version);
					CClusterCacheEntry *pCluster = m_pVirtualFS->mp_ClusterCache.f_GetCluster(m_FileDescriptorID);
					NStream::CBinaryStreamMemoryPtr<> Stream;
					pCluster->f_OpenStream(Stream);
					m_FileRecord.f_Write(Stream);
					pCluster->f_CloseStream(true);
					m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster);
				}
			}

			void f_Destroy()
			{
				f_Flush();
				m_bValidClusterChain = false;
			}

			void f_Flush()
			{
				f_WriteClusterChain();
				f_WriteFileRecord();
			}
		};

		NIntrusive::TCAVLTree<&CFileInternal::m_CacheLink, CFileInternal::CCompare> mp_InternalFiles;

		class CBinaryStreamFileInternal : public NStream::CBinaryStreamDefault
		{
			CFileInternal *mp_pFile;
			CFilePos mp_Position;

		protected:
			DMibStreamImplementProtected(CBinaryStreamFileInternal);
		public:
			DMibStreamImplementOperators(CBinaryStreamFileInternal);

			CBinaryStreamFileInternal()
			{
				mp_Position = 0;
				mp_pFile = nullptr;
			}

			void f_Open(CFileInternal *_pFile)
			{
				mp_pFile = _pFile;
			}

			void f_Reset()
			{
				mp_Position = 0;
				mp_pFile = nullptr;
			}

			void f_FeedBytes(const void *_pMem, mint _nBytes)
			{
				mp_pFile->f_Write(_pMem, mp_Position, _nBytes);
				mp_Position += _nBytes;
			}

			void f_ConsumeBytes(void *_pMem, mint _nBytes)
			{
				mp_pFile->f_Read(_pMem, mp_Position, _nBytes);
				mp_Position += _nBytes;
			}

			bool f_IsValid() const
			{
				return mp_pFile != nullptr;
			}

			bool f_IsAtEndOfStream() const
			{
				return mp_Position == mp_pFile->m_FileRecord.m_FileSize;
			}

			NStream::CFilePos f_GetPosition() const
			{
				return mp_Position;
			}

			void f_SetPosition(NStream::CFilePos _Pos)
			{
				mp_Position = _Pos;
			}

			void f_SetPositionFromEnd(NStream::CFilePos _Pos)
			{
				mp_Position = mp_pFile->m_FileRecord.m_FileSize - _Pos;
			}

			void f_AddPosition(NStream::CFilePos _Pos)
			{
				mp_Position += _Pos;
			}

			bool f_IsValidReadPosition(NStream::CFilePos _Pos) const
			{
				return _Pos >= 0 && _Pos < NStream::CFilePos(mp_pFile->m_FileRecord.m_FileSize);
			}

			void f_Flush(bool _bLocalCacheOnly)
			{
				return mp_pFile->f_Flush();
			}

			void f_SetCacheSize(mint _CacheSize)
			{
			}

			NStream::CFilePos f_GetLength() const
			{
				return mp_pFile->m_FileRecord.m_FileSize;
			}

			mint f_ContainerLengthLimit() const
			{
				return NStream::fg_CapLengthLimit(f_GetLength() - f_GetPosition());
			}

			void f_SetLength(NStream::CFilePos _Length)
			{
				return mp_pFile->f_SetLength(_Length);
			}

		};


		void fp_Close();

		void fp_CheckPath(const NStr::CStr &_Path, bool _bCanBeEmpty = false);
		CFileInternal *fp_OpenFile(CClusterID _ClusterID);
		CFileInternal *fp_OpenNewFile();
		void fp_CloseFile(CFileInternal *_pFile);


		CFileInternal *fp_OpenFileFromPath(NStr::CStr const &_Path, bool _bCreate, bool _bRemove = false, bool _bAllowRoot = false);
		void fpr_DeleteDirectoryRecursive(NStr::CStr const &_File);

	public:

		CVirtualFS()
		{
			mp_bOutOfSpace = false;
			mp_pStorageStream = nullptr;
			mp_ClusterCache.f_Create(this);
			mp_DirectoryCache.f_Create(this);
		}

		~CVirtualFS()
		{
			fp_Close();
		}

		bool f_IsValid()
		{
			return mp_pStorageStream != nullptr;
		}


		void f_Flush();

		void f_Open(NStream::CBinaryStream *_pStorageStream);
		void f_Create(NStream::CBinaryStream *_pStorageStream, uint64 _ClusterSize, uint64 _FileSystemGrowSise, uint64 _InitialSize);
		void f_Close();

		void f_SetCacheSizes(mint _nDirectories, mint _nClusterBytes);


		void f_OpenFile(CFile &_File, NStr::CStr const &_FileName, uint32 _OpenFlags);
		void f_DeleteFile(NStr::CStr const &_File);
		void f_RenameFile(NStr::CStr const &_FileFrom, NStr::CStr const &_FileTo);
		void f_DeleteDirectoryRecursive(NStr::CStr const &_File);
		void f_CreateDirectory(NStr::CStr const &_Path);
		bool f_FileExists(NStr::CStr const &_File);
		bool f_FileExists(NStr::CStr const &_File, EFileAttrib _Attributes);
		bool f_ReadFileToMemory(NStr::CStr const &_File, NContainer::CByteVector &_Data);

		class CCheckReporter
		{
		public:
			virtual void f_ReportError(NStr::CStr _Error) = 0;
			virtual void f_IncreaseProgress(uint64 _nInc) {};
			virtual void f_StepDown(uint64 _nMax) {};
			virtual void f_StepUp() {};
		};

		enum ECheckFSError
		{
			ECheckFSError_None,
			ECheckFSError_NotCheckableMemoryInsufficient,
			ECheckFSError_InplaceFixableSecondPass,
			ECheckFSError_VersionUpgradeFixable,
			ECheckFSError_InplaceFixable,
			ECheckFSError_NewFSFixable,
			ECheckFSError_NotFixable,
		};

		class CCheckFSContext
		{
			friend class CVirtualFS;
		public:

			enum
			{
				EClusterCheckFlag_Free = DMibBit(0),
				EClusterCheckFlag_FileRecord = DMibBit(1),
				EClusterCheckFlag_FileClusterChain = DMibBit(2),
				EClusterCheckFlag_FileData = DMibBit(2),
				EClusterCheckFlag_Invalid = DMibBit(3),
				EClusterCheckFlag_RootData = DMibBit(4),
			};

			class CClusterCheck
			{
			public:
				CClusterCheck()
				{
					m_Flags = 0;
					m_ClusterID = 0;
				}

				uint64 m_Flags;
				CVirtualFS::CClusterID m_ClusterID;
			};

			CCheckFSContext(NStream::CBinaryStream *_pStorageStream, CCheckReporter *_pReporter, bool _bFix, CVirtualFS *_pFixDestinationFS)
			{
				mp_pStorageStream = _pStorageStream;
				mp_pReporter = _pReporter;
				mp_bFix = _bFix;
				mp_pFixDestinationFS = _pFixDestinationFS;
				mp_MaxDirectoryID = 0;
			}

			ECheckFSError f_UpdateFreeClusters();
			void f_FreeCluster(CClusterID _Cluster);
			void f_Seek(CClusterID _Cluster);
			uint64 f_AddClusterID(CClusterID _Cluster, CClusterID _File, uint64 _Flags, bool _bMap);
			ECheckFSError f_AddFreeClusters(CClusterID _ClusterID);
			bool f_WriteFileData(CVirtualFS::CFileRecord &_FileRecord, NContainer::CByteVector &_Data, mint _Length);
			bool f_ReadFileData(CVirtualFS::CFileRecord &_FileRecord, NContainer::CByteVector &_Data);
			static bool fs_ReadFileData(uint32 _Version, uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream, CVirtualFS::CFileRecord &_FileRecord, NContainer::CByteVector &_Data);
			static bool fs_CheckClusterChain(uint32 _Version, uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream, CVirtualFS::CFileRecord &_FileRecord);

			class CCircularChecker
			{
			public:
				class CMember
				{
				public:
					DMibListLinkDS_Link(CMember, m_Link);
					CClusterID m_ClusterID;

				};

				DMibListLinkDS_List(CMember, m_Link) m_List;

				bool f_AlreadyInList(CClusterID _ClusterID)
				{
					auto Iter = m_List.f_GetIterator();

					while (Iter)
					{
						if (Iter->m_ClusterID == _ClusterID)
							return true;

						++Iter;
					}

					return false;
				}
			};

			ECheckFSError f_CheckOrphanClusters();
			ECheckFSError f_CheckFSTree(CClusterID _ClusterID, bool _bMap);
			ECheckFSError fr_CheckFSTree(CClusterID _ClusterID, CCircularChecker &_CircularChecker, bool &_bRemove, bool _bMap, CDirectoryID _ParentDir, NStr::CStr const &_FileName);
		private:

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

		CMibFilePos f_GetTotalSize()
		{
			if (mp_pStorageStream)
				return mp_pStorageStream->f_GetLength();
			return 0;
		}

		void f_SetAttributes(NStr::CStr const &_Path, uint64 _Attribs);
		void f_ChangeFileAttributes(NStr::CStr const &_Path, uint64 _AttribsAdd, uint64 _AttribsRemove);
		void f_SetCreationTime(NStr::CStr const &_Path, const NTime::CTime &_Time);
		void f_SetAccessTime(NStr::CStr const &_Path, const NTime::CTime &_Time);
		void f_SetWriteTime(NStr::CStr const &_Path, const NTime::CTime &_Time);
		uint64 f_GetAttributes(NStr::CStr const &_Path);
		NTime::CTime f_GetCreationTime(NStr::CStr const &_Path);
		NTime::CTime f_GetAccessTime(NStr::CStr const &_Path);
		NTime::CTime f_GetWriteTime(NStr::CStr const &_Path);

		uint64 f_GetFileSize(NStr::CStr const &_Path);

		void f_EnumFiles(NStr::CStr const &_Path, NContainer::TCVector<NStr::CStr> &_Files);
		NContainer::TCVector<NStr::CStr> f_FindFiles(NStr::CStr const &_Path, uint32 _AttribMask = EFileAttrib_Directory | EFileAttrib_File, bool _bRecursive = false);
		NCryptography::CHashDigest_MD5 f_GetFileChecksum(NStr::CStr const &_Path);

		class CFile
		{
			friend class CVirtualFS;
			CVirtualFS::CFileInternal *mp_pInternalFile;
			uint64 mp_Position;
			void fp_CheckValid() const;
			DMibListLinkDS_Link(CFile, m_Link);
		public:

			CFile();
			CFile(CVirtualFS &_FileSystem, NStr::CStr const &_FileName, uint32 _OpenFlags);
			~CFile();

			bool f_IsValid() const;
			void f_Close();
			void f_Open(CVirtualFS &_FileSystem, NStr::CStr const &_FileName, uint32 _OpenFlags);
			void f_Read(void *_pDest, mint _nBytes);
			void f_Write(const void *_pSrc, mint _nBytes);
			void f_Flush(bool _bLocalCacheOnly);
			void f_SetCacheSize(mint _CacheSize);
			bool f_IsAtEndOfFile() const;
			void f_SetLength(const CMibFilePos &_Length);
			CMibFilePos f_GetLength() const;
			void f_SetPosition(NStream::CFilePos _Pos);
			void f_SetPositionFromEnd(NStream::CFilePos _Pos);
			void f_AddPosition(NStream::CFilePos _Pos);
			bool f_IsValidReadPosition(NStream::CFilePos _Pos) const;
			NStream::CFilePos f_GetPosition() const;
		};

	private:
		DMibListLinkDS_List(CFile, m_Link) m_OpenFiles;
		typedef DMibListLinkDS_Iter(CFile, m_Link) COpenFilesIter;
	public:

		void f_CloseOpenFiles();

		void f_CopyFileToFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile);
		void f_CopyFileFromFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile);
		void f_CopyFileToFS(CVirtualFS &_OtherFS, NStr::CStr const &_FromFile, NStr::CStr const &_ToFile);
		void f_CopyFileInFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile);
	};

	typedef CVirtualFS::CFile CVirtualFSFile;

	template <typename t_CStreamType = NStream::CBinaryStreamDefault>
	class TCBinaryStream_VirtualFSFilePtr : public t_CStreamType
	{
		CVirtualFSFile *mp_pFile;

	protected:
		DMibStreamImplementProtected(TCBinaryStream_VirtualFSFilePtr);
	public:
		DMibStreamImplementOperators(TCBinaryStream_VirtualFSFilePtr);

		TCBinaryStream_VirtualFSFilePtr()
		{
			mp_pFile = nullptr;
		}

		TCBinaryStream_VirtualFSFilePtr(CVirtualFSFile *_pFile)
		{
			mp_pFile = _pFile;
		}

		TCBinaryStream_VirtualFSFilePtr(CVirtualFSFile &_File)
		{
			mp_pFile = &_File;
		}

		void f_Open(CVirtualFSFile *_pFile)
		{
			mp_pFile = _pFile;
		}

		void f_Open(CVirtualFSFile &_File)
		{
			mp_pFile = &_File;
		}

		void f_Reset()
		{
			DMibMemLightweightTrackDisableScope;
			mp_pFile->f_SetLength(0);
			mp_pFile->f_Flush(false);
		}

		void f_Flush(bool _bLocalCacheOnly)
		{
			DMibMemLightweightTrackDisableScope;
			mp_pFile->f_Flush(_bLocalCacheOnly);
		}
		void f_SetCacheSize(mint _CacheSize)
		{
			DMibMemLightweightTrackDisableScope;
			mp_pFile->f_SetCacheSize(_CacheSize);
		}

		void f_FeedBytes(const void *_pMem, mint _nBytes)
		{
			DMibMemLightweightTrackDisableScope;
			mp_pFile->f_Write(_pMem, _nBytes);
		}

		void f_ConsumeBytes(void *_pMem, mint _nBytes)
		{
			DMibMemLightweightTrackDisableScope;
			mp_pFile->f_Read(_pMem, _nBytes);
		}

		bool f_IsValid() const
		{
			DMibMemLightweightTrackDisableScope;
			return mp_pFile->f_IsValid();
		}

		bool f_IsAtEndOfStream() const
		{
			DMibMemLightweightTrackDisableScope;
			return mp_pFile->f_IsAtEndOfFile();
		}

		NStream::CFilePos f_GetPosition() const
		{
			DMibMemLightweightTrackDisableScope;
			return mp_pFile->f_GetPosition();
		}

		void f_SetPosition(NStream::CFilePos _Pos)
		{
			DMibMemLightweightTrackDisableScope;
			mp_pFile->f_SetPosition(_Pos);
		}

		void f_SetPositionFromEnd(NStream::CFilePos _Pos)
		{
			DMibMemLightweightTrackDisableScope;
			mp_pFile->f_SetPositionFromEnd(_Pos);
		}

		void f_AddPosition(NStream::CFilePos _Pos)
		{
			DMibMemLightweightTrackDisableScope;
			mp_pFile->f_AddPosition(_Pos);
		}

		bool f_IsValidReadPosition(NStream::CFilePos _Pos) const
		{
			DMibMemLightweightTrackDisableScope;
			return mp_pFile->f_IsValidReadPosition(_Pos);
		}

		NStream::CFilePos f_GetLength() const
		{
			DMibMemLightweightTrackDisableScope;
			return mp_pFile->f_GetLength();
		}

		mint f_ContainerLengthLimit() const
		{
			return NStream::fg_CapLengthLimit(f_GetLength() - f_GetPosition());
		}

		void f_SetLength(NStream::CFilePos _Length)
		{
			DMibMemLightweightTrackDisableScope;
			return mp_pFile->f_SetLength(_Length);
		}
	};

	template <typename t_CStreamType = NStream::CBinaryStreamDefault>
	class TCBinaryStream_VirtualFSFile : public t_CStreamType
	{
		CVirtualFSFile mp_File;

	protected:
		DMibStreamImplementProtected(TCBinaryStream_VirtualFSFile);
	public:
		DMibStreamImplementOperators(TCBinaryStream_VirtualFSFile);

		TCBinaryStream_VirtualFSFile()
		{
		}

		~TCBinaryStream_VirtualFSFile()
		{
			DMibMemLightweightTrackDisableScope;
			mp_File.f_Close();
		}

		CVirtualFSFile &f_GetVFSFile()
		{
			return mp_File;
		}

		void f_Reset()
		{
			DMibMemLightweightTrackDisableScope;
			mp_File.f_SetLength(0);
			mp_File.f_Flush(false);
		}

		void f_Flush(bool _bLocalCacheOnly)
		{
			DMibMemLightweightTrackDisableScope;
			mp_File.f_Flush(_bLocalCacheOnly);
		}
		void f_SetCacheSize(mint _CacheSize)
		{
			DMibMemLightweightTrackDisableScope;
			mp_File.f_SetCacheSize(_CacheSize);
		}

		void f_FeedBytes(const void *_pMem, mint _nBytes)
		{
			DMibMemLightweightTrackDisableScope;
			mp_File.f_Write(_pMem, _nBytes);
		}

		void f_ConsumeBytes(void *_pMem, mint _nBytes)
		{
			DMibMemLightweightTrackDisableScope;
			mp_File.f_Read(_pMem, _nBytes);
		}

		bool f_IsValid() const
		{
			DMibMemLightweightTrackDisableScope;
			return mp_File.f_IsValid();
		}

		bool f_IsAtEndOfStream() const
		{
			DMibMemLightweightTrackDisableScope;
			return mp_File.f_IsAtEndOfFile();
		}

		NStream::CFilePos f_GetPosition() const
		{
			DMibMemLightweightTrackDisableScope;
			return mp_File.f_GetPosition();
		}

		void f_SetPosition(NStream::CFilePos _Pos)
		{
			DMibMemLightweightTrackDisableScope;
			mp_File.f_SetPosition(_Pos);
		}

		void f_SetPositionFromEnd(NStream::CFilePos _Pos)
		{
			DMibMemLightweightTrackDisableScope;
			mp_File.f_SetPositionFromEnd(_Pos);
		}

		void f_AddPosition(NStream::CFilePos _Pos)
		{
			DMibMemLightweightTrackDisableScope;
			mp_File.f_AddPosition(_Pos);
		}

		bool f_IsValidReadPosition(NStream::CFilePos _Pos) const
		{
			DMibMemLightweightTrackDisableScope;
			return mp_File.f_IsValidReadPosition(_Pos);
		}

		NStream::CFilePos f_GetLength() const
		{
			DMibMemLightweightTrackDisableScope;
			return mp_File.f_GetLength();
		}

		mint f_ContainerLengthLimit() const
		{
			return NStream::fg_CapLengthLimit(f_GetLength() - f_GetPosition());
		}

		void f_SetLength(NStream::CFilePos _Length)
		{
			DMibMemLightweightTrackDisableScope;
			return mp_File.f_SetLength(_Length);
		}
	};
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif
