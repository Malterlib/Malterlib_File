// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_MalterlibFS.h"

namespace NMib::NFile
{
	class CVirtualFS_FixFSFileTree
	{
		CVirtualFS::CCheckReporter *mp_pReporter;
		uint32 mp_Version;
	public:

		CVirtualFS_FixFSFileTree(CVirtualFS::CCheckReporter *_pReporter, uint32 _Version)
		{
			mp_pReporter = _pReporter;
			mp_Version = _Version;
		}

		class CDirectoryKey
		{
		public:
			CVirtualFS::CDirectoryID m_DirectoryID;
			CVirtualFS::CClusterID m_ClusterID;
		};


		class CFile
		{
		public:
			CVirtualFS_FixFSFileTree *m_pTree;
			CFile(CVirtualFS_FixFSFileTree *_pTree)
			{
				m_DirectoryID = 0;
				m_ParentID = 0;
				m_FileClusterID = 0;
				m_bIsDir = false;
				m_FileSize = 0;
				m_pParent = nullptr;
				m_pTree = _pTree;
			}
			~CFile()
			{
				f_SetParent(nullptr);
				m_Children.f_DeleteAllDefiniteType();
				if (m_FileClusterIDLink.f_IsInTree())
					m_pTree->m_FileClusters.f_Remove(this);
				if (m_DirectoryIDLink.f_IsInTree())
					m_pTree->m_Directories.f_Remove(this);
			}

			bint m_bIsDir;
			NStr::CMStrPreserve m_FileName;
			CVirtualFS::CDirectoryID m_DirectoryID;
			CVirtualFS::CDirectoryID m_ParentID;
			CVirtualFS::CClusterID m_FileClusterID;
			CMibFilePos m_FileSize;
			CFile *m_pParent;

			CVirtualFS::CDirectory m_Directory;

			uint32 f_GetDepth()
			{
				uint32 Depth = 0;
				CFile *pParent = this;
				while (pParent)
				{
					if (pParent->m_FileClusterID == 1 && pParent->m_DirectoryID == 1)
						return 0xffffffff;
					++Depth;
					pParent = pParent->m_pParent;
				}
				return Depth;
			}

			void f_SetParent(CFile *_pParent)
			{
				if (m_pParent)
				{
					if (m_ChildLink.f_IsInList())
						m_pParent->m_Children.f_Remove(this);
				}
				m_pParent = _pParent;
				if (_pParent)
					_pParent->m_Children.f_Insert(this);
			}

			class CCompareFileClusterID
			{
			public:
				inline_small const CVirtualFS::CClusterID &operator () (CFile const &_Node) const
				{
					return _Node.m_FileClusterID;
				}
			};

			NIntrusive::TCAVLLink<> m_FileClusterIDLink;

			DMibListLinkDS_Link(CFile, m_ChildLink);
			DMibListLinkDS_List(CFile, m_ChildLink) m_Children;

			CVirtualFS::CDirectory::CFile *f_GetDirectoryEntry(CVirtualFS::CDirectoryID _Cluster)
			{
				mint nFiles = m_Directory.m_Files.f_GetLen();
				for (mint i = 0; i < nFiles; ++i)
				{
					if (m_Directory.m_Files[i].m_FileRecordCluster == _Cluster)
						return &m_Directory.m_Files[i];
				}
				return nullptr;
			}
			CFile *f_GetFileByName(const NStr::CStr &_Name)
			{
				DMibListLinkDS_Iter(CFile, m_ChildLink) Iter = m_Children;

				while (Iter)
				{
					if (Iter->m_FileName == _Name)
						return Iter;

					++Iter;
				}
				return nullptr;
			}

			class CCompareDirectoryID
			{
			public:
				inline_small bint operator () (CFile const &_Left, CFile const &_Right) const
				{
					if (_Left.m_DirectoryID < _Right.m_DirectoryID)
						return true;
					if (_Left.m_DirectoryID > _Right.m_DirectoryID)
						return false;
					return _Left.m_FileClusterID < _Right.m_FileClusterID;
				}
				inline_small bint operator () (CFile const &_Left, const CDirectoryKey &_Right) const
				{
					if (_Left.m_DirectoryID < _Right.m_DirectoryID)
						return true;
					if (_Left.m_DirectoryID > _Right.m_DirectoryID)
						return false;
					return _Left.m_FileClusterID < _Right.m_ClusterID;
				}
				inline_small bint operator () (const CDirectoryKey &_Left, CFile const &_Right) const
				{
					if (_Left.m_DirectoryID < _Right.m_DirectoryID)
						return true;
					if (_Left.m_DirectoryID > _Right.m_DirectoryID)
						return false;
					return _Left.m_ClusterID < _Right.m_FileClusterID;
				}
			};

			NIntrusive::TCAVLLink<> m_DirectoryIDLink;
			DMibListLinkDS_Link(CFile, m_RootLink);
		};
		DMibListLinkDS_List(CFile, m_RootLink) m_Roots;

		void f_FindRoots();

		void fr_TraceTree(CFile *_pFile, int _Depth, CVirtualFS::CCheckReporter *_pReporter)
		{
			DMibListLinkDS_Iter(CFile, m_ChildLink) Iter = _pFile->m_Children;

			while (Iter)
			{
				NStr::CStr ToTrace = NStr::CStr::CFormat("{sj*6}{}: Dir: {} Cluster: {}    Parent: {}     FileSize: {}") << "" << NStr::CStr(Iter->m_FileName) << Iter->m_DirectoryID << Iter->m_FileClusterID << Iter->m_ParentID << Iter->m_FileSize << _Depth * 3;
				if (1)
					DMibDTrace("{}" DMibNewLine, ToTrace);
				else
					_pReporter->f_ReportError(ToTrace);
				fr_TraceTree(Iter, _Depth + 1, _pReporter);
				++Iter;
			}
		}
		void f_TraceTree(CVirtualFS::CCheckReporter *_pReporter)
		{
			DMibListLinkDS_Iter(CFile, m_RootLink) Iter = m_Roots;

			while (Iter)
			{
				NStr::CStr ToTrace = NStr::CStr::CFormat("Root Dir: {} Cluster: {}    Parent: {}    FileSize: {}") << Iter->m_DirectoryID << Iter->m_FileClusterID <<  Iter->m_ParentID << Iter->m_FileSize;
				if (1)
					DMibDTrace("{}" DMibNewLine, ToTrace);
				else
					_pReporter->f_ReportError(ToTrace);

				fr_TraceTree(Iter, 1, _pReporter);

				++Iter;
			}
		}

		void f_CopyFile(CVirtualFS::CFileRecord &_FileRecord, CVirtualFS::CFile &_File, uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream);
		void fr_CopyTree(NStr::CStr const &_Path, CFile *_pFile, uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream, CVirtualFS *_pFixDestinationFS);
		void f_CopyTree(uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream, CVirtualFS *_pFixDestinationFS);

		CFile *f_GetFile(CVirtualFS::CClusterID _Cluster)
		{
			CVirtualFS_FixFSFileTree::CFile *pFile = m_FileClusters.f_FindEqual(_Cluster);

			if (!pFile)
			{
				pFile = DMibNew CVirtualFS_FixFSFileTree::CFile(this);
				pFile->m_FileClusterID = _Cluster;
				m_FileClusters.f_Insert(pFile);
			}
			return pFile;
		}


		CFile *f_GetDirectory(CVirtualFS::CDirectoryID _DirID)
		{
			CVirtualFS_FixFSFileTree::CDirectoryKey Key;
			Key.m_DirectoryID = _DirID;
			Key.m_ClusterID = 0;

			CVirtualFS_FixFSFileTree::CFile *pFile = m_Directories.f_FindSmallestGreaterThanEqual(Key);
			if (pFile && pFile->m_DirectoryID != _DirID)
				pFile = nullptr;

			if (!pFile)
			{
				// No directory found, lets create a virtual directory
				pFile = DMibNew CVirtualFS_FixFSFileTree::CFile(this);
				pFile->m_DirectoryID = _DirID;
				m_Directories.f_Insert(pFile);
			}
			return pFile;
		}

		CFile *f_GetDirectory(CVirtualFS::CClusterID _Cluster, CVirtualFS::CDirectoryID _DirID)
		{
			CVirtualFS_FixFSFileTree::CDirectoryKey Key;
			Key.m_DirectoryID = _DirID;
			Key.m_ClusterID = _Cluster;
			CVirtualFS_FixFSFileTree::CFile *pFileDir = m_Directories.f_FindEqual(Key);
			if (!pFileDir)
			{
				Key.m_ClusterID = 0;
				pFileDir = m_Directories.f_FindEqual(Key);
			}
			CVirtualFS_FixFSFileTree::CFile *pFileCluster = m_FileClusters.f_FindEqual(_Cluster);

			if (pFileDir)
			{
				if (pFileCluster)
				{
					if (pFileCluster == pFileDir)
						return pFileDir;

					DMibSafeCheck(pFileDir->m_FileClusterID == 0, "This directory must be virtual");

					// Move over children from virtual directory to real directory
					while (pFileDir->m_Children.f_GetFirst())
					{
						CFile *pFile = pFileDir->m_Children.f_GetFirst();
						pFile->f_SetParent(pFileCluster);
					}

					// We can now discard the virtual directory
					delete pFileDir;

					if (pFileCluster->m_DirectoryIDLink.f_IsInTree())
						m_Directories.f_Remove(pFileCluster);

					// Map the cluster file to be a directory
					pFileCluster->m_DirectoryID = _DirID;
					pFileCluster->m_bIsDir = true;
					m_Directories.f_Insert(pFileCluster);

					return pFileCluster;
				}
				else
				{
					DMibSafeCheck(pFileDir->m_FileClusterID == 0, "This directory must be virtual");

					if (pFileDir->m_FileClusterIDLink.f_IsInTree())
						m_FileClusters.f_Remove(pFileDir);
					pFileDir->m_FileClusterID = _Cluster;
					pFileDir->m_bIsDir = true;
					m_FileClusters.f_Insert(pFileDir);
					return pFileDir;
				}
			}
			else if (pFileCluster)
			{
				pFileCluster->m_DirectoryID = _DirID;
				pFileCluster->m_bIsDir = true;
				m_Directories.f_Insert(pFileCluster);
				return pFileCluster;
			}

			CVirtualFS_FixFSFileTree::CFile *pFile = DMibNew CVirtualFS_FixFSFileTree::CFile(this);
			pFile->m_DirectoryID = _DirID;
			pFile->m_FileClusterID = _Cluster;
			pFile->m_bIsDir = true;
			m_FileClusters.f_Insert(pFile);
			m_Directories.f_Insert(pFile);
			return pFile;
		}

		void f_Destroy()
		{
			m_Roots.f_Clear();

			m_FileClusters.f_DeleteAllDefiniteType();
			m_Directories.f_DeleteAllDefiniteType();
		}

		NIntrusive::TCAVLTree<&CFile::m_FileClusterIDLink, CFile::CCompareFileClusterID> m_FileClusters;
		NIntrusive::TCAVLTree<&CFile::m_DirectoryIDLink, CFile::CCompareDirectoryID> m_Directories;
	};


	void CVirtualFS::f_Open(NStream::CBinaryStream *_pStorageStream)
	{
		fp_Close();

		mp_pStorageStream = _pStorageStream;
		fp_SetStreamCluster(0); // Seek to beginning
		mp_RootData.f_Read(*mp_pStorageStream);
		if (!mp_RootData.f_Validate())
			DMibErrorFile("Root data corrupt");

		mp_pStorageStream->f_SetCacheSize(mp_RootData.m_ClusterSize);

		mp_ClusterIDsPerCluster = (mp_RootData.m_ClusterSize - (sizeof(NCryptography::CHashDigest_MD5) + sizeof(CClusterID)*2 + sizeof(uint64))) / sizeof(CClusterID);
		mp_DirectoryCache.f_SetCacheSize(mp_RootData.m_DirectoryCacheSize);
		mp_ClusterCache.f_SetCacheSize(mp_RootData.m_ClusterCacheSize);
	}

	void CVirtualFS::f_Flush()
	{
		mp_DirectoryCache.f_Flush();
		mp_ClusterCache.f_Flush();
		if (mp_pStorageStream)
			mp_pStorageStream->f_Flush(false);
	}


	CVirtualFS::CDirectoryCacheEntry *CVirtualFS::CDirectoryCache::f_GetNewCacheEntry(CDirectoryCacheEntry *_pParent, NStr::CStr const &_Path)
	{
		CVirtualFS::CFileInternal *pNewFile = m_pVirtualFS->fp_OpenNewFile();
		if (!pNewFile)
			return nullptr;

		pNewFile->m_FileRecord.m_FileAttributes = EFileAttrib_Directory;

		CDirectoryCacheEntry *pEntry = DMibNew CDirectoryCacheEntry;
		pEntry->m_DirectoryFileClusterID = pNewFile->m_FileDescriptorID;
		if (_pParent->m_FullPath.f_GetLen())
			pEntry->m_FullPath = _pParent->m_FullPath + "/" + _Path;
		else
			pEntry->m_FullPath = _Path;
		m_Tree.f_Insert(pEntry);

		pNewFile->m_FileRecord.m_ParentDirectory = _pParent->m_Directory.m_DirectoryID;
		pNewFile->m_bFileRecordDirty = true;

		_pParent->m_Directory.f_AddChild(_Path, pEntry->m_DirectoryFileClusterID);
		_pParent->m_bDirty = true;

		pEntry->m_bDirty = true;
		pEntry->m_Directory.m_DirectoryID = m_pVirtualFS->fp_GetDirectoryID();
		m_pVirtualFS->fp_CloseFile(pNewFile);

		++pEntry->m_RefCount;

		return pEntry;
	}

	CVirtualFS::CDirectoryCacheEntry *CVirtualFS::CDirectoryCache::f_GetExistingCacheEntry(CClusterID _DirectoryCluster, NStr::CStr const &_Path)
	{
		CFileInternal *pFile = m_pVirtualFS->fp_OpenFile(_DirectoryCluster);
		if (!(pFile->m_FileRecord.m_FileAttributes & EFileAttrib_Directory))
		{
			m_pVirtualFS->fp_CloseFile(pFile);
			return nullptr;
		}

		CDirectoryCacheEntry *pEntry = DMibNew CDirectoryCacheEntry;
		pEntry->m_DirectoryFileClusterID = _DirectoryCluster;
		pEntry->m_FullPath = _Path;
		m_Tree.f_Insert(pEntry);

		CBinaryStreamFileInternal Stream;
		Stream.f_Open(pFile);
		pEntry->m_Directory.f_Read(Stream);
		m_pVirtualFS->fp_CloseFile(pFile);

		return pEntry;
	}

	uint64 CVirtualFS::f_GetClusterSize()
	{
		return mp_RootData.m_ClusterSize;
	}

	uint64 CVirtualFS::f_GetClusterIDsPerCluster()
	{
		return mp_ClusterIDsPerCluster;
	}

	uint64 CVirtualFS::f_GetNumberOfFreeClusters()
	{
		return mp_RootData.m_nFreeClusters;
	}

	uint64 CVirtualFS::f_GetFreeSize()
	{
		return mp_RootData.m_nFreeClusters * mp_RootData.m_ClusterCacheSize;
	}

	uint64 CVirtualFS::f_GetFileSystemGrowSize()
	{
		return mp_RootData.m_FileSystemGrowSize * mp_RootData.m_ClusterSize;
	}

	void CVirtualFS::f_SetFileSystemGrowSize(uint64 _Size)
	{
		mp_RootData.m_FileSystemGrowSize = _Size / mp_RootData.m_ClusterSize;
		fp_SaveRootData();
	}

	void CVirtualFS::f_Close()
	{
		fp_Close();

	}

	void CVirtualFS_FixFSFileTree::f_FindRoots()
	{

		// Start by removing invalid entries so they are roots
		auto Iter = m_FileClusters.f_GetIterator();

		while (Iter)
		{
			CFile *pIter = Iter;
			if (pIter->m_FileName.f_IsEmpty())
			{
				pIter->f_SetParent(nullptr);
			}
			++Iter;
		}

		// Find roots
		Iter = m_FileClusters;

		while (Iter)
		{
			CFile *pRoot = Iter;
			while (pRoot->m_pParent)
			{
				pRoot = pRoot->m_pParent;
			}
			m_Roots.f_Insert(pRoot);
			++Iter;
		}

		// Merge equal directory entries

		{
			bint bDoneSomething = true;
			while (bDoneSomething)
			{
				bDoneSomething = false;
				auto Iter = m_Directories.f_GetIterator();

				while (Iter)
				{
					CFile *pFile = Iter;
					auto Iter2 = Iter;

					CFile *pBestFile = pFile;
					uint32 BestDepth = pBestFile->f_GetDepth();
					mint nDirs = 0;
					while (Iter2)
					{
						CFile *pFile2 = Iter2;
						if (pFile2->m_DirectoryID != pFile->m_DirectoryID)
							break;
						++nDirs;
						uint32 ThisDepth = pFile2->f_GetDepth();
						if (ThisDepth > BestDepth)
						{
							pBestFile = pFile2;
							BestDepth = ThisDepth;
						}
						++Iter2;
					}

					if (nDirs > 1)
					{
						auto Iter2 = Iter;

						while (Iter2)
						{
							CFile *pFile2 = Iter2;
							if (pFile2->m_DirectoryID != pFile->m_DirectoryID)
								break;

							if (pFile2 != pBestFile)
							{
								DMibListLinkDS_Iter(CFile, m_ChildLink) ChildIter = pFile2->m_Children;
								while (ChildIter)
								{
									CFile *pChild = ChildIter;
									++ChildIter;

									CVirtualFS::CDirectory::CFile *pDirEntry = pBestFile->f_GetDirectoryEntry(pChild->m_FileClusterID);
									if (pDirEntry)
									{
										pChild->m_FileName = pDirEntry->m_FileName;
										pChild->f_SetParent(pBestFile);
										bDoneSomething = true;
									}
								}
							}
							++Iter2;
						}
					}

					++Iter;
				}
			}
		}
	}

	void CVirtualFS_FixFSFileTree::f_CopyFile(CVirtualFS::CFileRecord &_FileRecord, CVirtualFS::CFile &_File, uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream)
	{
		mp_pReporter->f_StepDown(_FileRecord.m_FileSize);
		try
		{
			CVirtualFS::CClusterChain Chain;
			Chain.m_ClusterIDs.f_SetLen(_ClusterIDsPerCluster);

			CVirtualFS::CClusterID Cluster = _FileRecord.m_FirstFileChainCluster;
			NContainer::CByteVector Temp;
			Temp.f_SetLen(_ClusterSize);

			mint iCluster = 0;

			while (Cluster)
			{
				_pStorageStream->f_SetPosition(Cluster * _ClusterSize);
				Chain.f_Read(*_pStorageStream);

				if (!Chain.f_Validate(Cluster, mp_Version))
					return;

				for (uint64 i = 0; i < Chain.m_nClusterIDs; ++i)
				{
					_pStorageStream->f_SetPosition(Chain.m_ClusterIDs[i] * _ClusterSize);

					if (iCluster * _ClusterSize < _FileRecord.m_FileSize)
					{
						mint Size = fg_Min(_FileRecord.m_FileSize - iCluster * _ClusterSize, _ClusterSize);
						_pStorageStream->f_ConsumeBytes(Temp.f_GetArray(), Size);
						_File.f_Write(Temp.f_GetArray(), Size);
						mp_pReporter->f_IncreaseProgress(Size);
					}

					++iCluster;
				}

				Cluster = Chain.m_NextChainCluster;
			}

	//		_File.f_SetLength(_FileRecord.m_FileSize);
			mp_pReporter->f_StepUp();
		}
		catch (NException::CException)
		{
			mp_pReporter->f_StepUp();
		}
	}

	void CVirtualFS_FixFSFileTree::fr_CopyTree(NStr::CStr const &_Path, CFile *_pFile, uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream, CVirtualFS *_pFixDestinationFS)
	{
		CVirtualFS::CClusterID Cluster = _pFile->m_FileClusterID;
		_pStorageStream->f_SetPosition(Cluster * _ClusterSize);

		CVirtualFS::CFileRecord FileRecord;
		FileRecord.f_Read(*_pStorageStream);

		bool bIsDir = _pFile->m_bIsDir || !_pFile->m_Children.f_IsEmpty();
		if (bIsDir && _Path != "")
			_pFixDestinationFS->f_CreateDirectory(_Path);

		if (FileRecord.f_Validate(Cluster, mp_Version))
		{
			if (!(FileRecord.m_FileAttributes & NFile::EFileAttrib_Directory))
			{
				CVirtualFS::CFile File;
				if (bIsDir)
				{
					File.f_Open(*_pFixDestinationFS, _Path + "/Recovered." + FileRecord.m_MD5Checksum.f_GetString(), EFileOpen_Write);
				}
				else
					File.f_Open(*_pFixDestinationFS, _Path, EFileOpen_Write);

				f_CopyFile(FileRecord, File, _ClusterSize, _ClusterIDsPerCluster, _pStorageStream);
			}

			if (_Path != "" && _pFixDestinationFS->f_FileExists(_Path))
			{
				_pFixDestinationFS->f_SetAttributes(_Path, FileRecord.m_FileAttributes);
				_pFixDestinationFS->f_SetAccessTime(_Path, FileRecord.m_AccessTime);
				_pFixDestinationFS->f_SetCreationTime(_Path, FileRecord.m_CreationTime);
				_pFixDestinationFS->f_SetWriteTime(_Path, FileRecord.m_WriteTime);
			}
		}

		DMibListLinkDS_Iter(CFile, m_ChildLink) Iter = _pFile->m_Children;

		mp_pReporter->f_StepDown(_pFile->m_Children.f_GetLen());

		while (Iter)
		{
			NStr::CStr FileName = Iter->m_FileName;
			if (FileName == "")
				FileName = FileRecord.m_MD5Checksum.f_GetString();

			NStr::CStr Path;
			if (_Path != "")
				Path = _Path + "/" + FileName;
			else
				Path = FileName;

			fr_CopyTree(Path, Iter, _ClusterSize, _ClusterIDsPerCluster, _pStorageStream, _pFixDestinationFS);
			++Iter;
			mp_pReporter->f_IncreaseProgress(1);
		}
		mp_pReporter->f_StepUp();
	}

	void CVirtualFS_FixFSFileTree::f_CopyTree(uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream, CVirtualFS *_pFixDestinationFS)
	{

		uint64 iCurrentRecover = 0;

		mp_pReporter->f_StepDown(m_Roots.f_GetLen());

		for (mint i = 0; i < 2; ++i)
		{
			DMibListLinkDS_Iter(CFile, m_RootLink) Iter = m_Roots;

			while (Iter)
			{

				CFile *pFile = Iter;

				if (pFile->m_FileClusterID == 1 && pFile->m_bIsDir && pFile->m_DirectoryID == 1)
				{
					if (i == 0)
					{
						DMibDTrace("Root Dir: {} Cluster: {}" DMibNewLine, Iter->m_DirectoryID << Iter->m_FileClusterID);
						// This is the root
						fr_CopyTree("", pFile, _ClusterSize, _ClusterIDsPerCluster, _pStorageStream, _pFixDestinationFS);
					}
				}
				else if (i == 1)
				{
					DMibDTrace("Root Dir: {} Cluster: {}" DMibNewLine, Iter->m_DirectoryID << Iter->m_FileClusterID);
					if (pFile->m_bIsDir)
					{
						NStr::CStr PathName = (NStr::CStr::CFormat("Lost&Found/RecoveredDir.{sj8,sf0}") << iCurrentRecover).f_GetStr();
						_pFixDestinationFS->f_CreateDirectory(PathName);
						++iCurrentRecover;

						fr_CopyTree(PathName, pFile, _ClusterSize, _ClusterIDsPerCluster, _pStorageStream, _pFixDestinationFS);
					}
					else
					{
						_pFixDestinationFS->f_CreateDirectory("Lost&Found");
						NStr::CStr PathName = (NStr::CStr::CFormat("Lost&Found/Recovered.{sj8,sf0}") << iCurrentRecover).f_GetStr();
						++iCurrentRecover;

						fr_CopyTree(PathName, pFile, _ClusterSize, _ClusterIDsPerCluster, _pStorageStream, _pFixDestinationFS);
					}
				}

				if (i == 1)
					mp_pReporter->f_IncreaseProgress(1);

				++Iter;
			}
		}
		mp_pReporter->f_StepUp();
	}

	CVirtualFS::ECheckFSError CVirtualFS::fs_CheckFS(NStream::CBinaryStream *_pStorageStream, CCheckReporter *_pReporter, bint _bFix, CVirtualFS *_pFixDestinationFS)
	{
		try
		{
			if (_pFixDestinationFS)
			{
				if (_pReporter)
					_pReporter->f_ReportError("Rebuilding file system.");

				// First detemine the cluster size

				_pStorageStream->f_SetPosition(0);

				CRootData RootData;
				RootData.f_Read(*_pStorageStream);

				uint64 ClusterSize = 128;
				uint64 nMaxFiles = 0;
				uint64 BestClusterSize = 128;
				uint64 FileLenth = _pStorageStream->f_GetLength();
				uint32 Version = 0x100;
				_pReporter->f_StepDown(3);

				if (RootData.f_Validate())
				{
					BestClusterSize = ClusterSize = RootData.m_ClusterSize;
					Version = RootData.m_Version;
					nMaxFiles = 1;
				}
				else
				{
					// Loop until the number of files found decrease
					_pReporter->f_StepDown(13);
					aint iLoop = 0;
					uint32 Versions[2] = {0x102, 0x103};
					uint32 nVersions = 2;
					while (ClusterSize < FileLenth)
					{
						uint64 nClusters = FileLenth / ClusterSize;
						CFileRecord FileRecord;
						uint64 nFiles[2] = {0, 0};
						if (iLoop < 13)
							_pReporter->f_StepDown(nClusters);
						for (uint64 i = 0; i < nClusters; ++i)
						{
							_pStorageStream->f_SetPosition(ClusterSize * i);
							FileRecord.f_Read(*_pStorageStream);
							for (mint j = 0; j < nVersions; ++j)
							{
								if (FileRecord.f_Validate(i, Versions[j]))
									++nFiles[j];
							}
							if (iLoop < 13)
								_pReporter->f_IncreaseProgress(1);

						}

						if (iLoop < 13)
							_pReporter->f_StepUp();

						++iLoop;

						bint bWasBetter = false;
						for (mint j = 0; j < nVersions; ++j)
						{
							if (nFiles[j] >= nMaxFiles)
							{
								bWasBetter = true;
								Version = Versions[j];
								BestClusterSize = ClusterSize;
								nMaxFiles = nFiles[j];
							}
						}

						if (!bWasBetter)
							break;

						ClusterSize <<= 1;
					}
					_pReporter->f_StepUp();
				}

				_pReporter->f_IncreaseProgress(1);

				if (nMaxFiles == 0)
				{
					// If we have found no files we don't need to do anything more
					_pReporter->f_StepUp();
					return ECheckFSError_None;
				}

				ClusterSize = BestClusterSize;
				uint64 ClusterIDsPerCluster = (ClusterSize - (sizeof(NCryptography::CHashDigest_MD5) + sizeof(CClusterID)*2 + sizeof(uint64))) / sizeof(CClusterID);
				uint64 nClusters = FileLenth / ClusterSize;

				// First lets build up the directory tree
				CVirtualFS_FixFSFileTree Tree(_pReporter, Version);

				_pReporter->f_StepDown(nClusters);

				for (uint64 i = 0; i < nClusters; ++i)
				{
					_pStorageStream->f_SetPosition(ClusterSize * i);
					CFileRecord FileRecord;
					FileRecord.f_Read(*_pStorageStream);

					if (FileRecord.f_Validate(i, Version) && CCheckFSContext::fs_CheckClusterChain(Version, ClusterSize, ClusterIDsPerCluster, _pStorageStream, FileRecord))
					{
						// Only a directory can have parent dir 0
						if (FileRecord.m_ParentDirectory == 0 && !(FileRecord.m_FileAttributes & EFileAttrib_Directory))
							continue;

						CDirectory Dir;
						CVirtualFS_FixFSFileTree::CFile *pFile;
						if (FileRecord.m_FileAttributes & EFileAttrib_Directory)
						{
							try
							{
								NContainer::CByteVector Data;
								if (!CCheckFSContext::fs_ReadFileData(Version, ClusterSize, ClusterIDsPerCluster, _pStorageStream, FileRecord, Data))
									continue;

								NStream::CBinaryStreamMemoryPtr<> Stream;
								Stream.f_OpenRead(Data.f_GetArray(), Data.f_GetLen());

								Dir.f_Read(Stream);

								if (!Dir.f_Validate(i, Version))
								{
									if (Version < 0x103)
									{
										if (!Dir.f_Validate(i, 0x101))
											continue;
									}
									else
										continue;
								}
								if (FileRecord.m_ParentDirectory == 0 && Dir.m_DirectoryID != 1) // Only root dir can have parent dir 0
									continue;
							}
							catch(NException::CException)
							{
								continue;
							}

							pFile = Tree.f_GetDirectory(i, Dir.m_DirectoryID);
							pFile->m_Directory = Dir;
						}
						else
							pFile = Tree.f_GetFile(i);

						pFile->m_ParentID = FileRecord.m_ParentDirectory;
						pFile->m_FileSize = FileRecord.m_FileSize;

						if (!pFile->m_pParent)
						{
							CVirtualFS_FixFSFileTree::CFile *pParent = Tree.f_GetDirectory(pFile->m_ParentID);
							pFile->f_SetParent(pParent);
						}

						if (FileRecord.m_FileAttributes & EFileAttrib_Directory)
						{
							for (mint i = 0; i < Dir.m_Files.f_GetLen(); ++i)
							{
								CVirtualFS_FixFSFileTree::CFile *pSubFile = Tree.f_GetFile(Dir.m_Files[i].m_FileRecordCluster);
								NStr::CStr FileName = Dir.m_Files[i].m_FileName;
								pSubFile->m_FileName = FileName;
								// Set file as parent
								pSubFile->f_SetParent(pFile);
							}
						}
					}

					_pReporter->f_IncreaseProgress(1);
				}

				_pReporter->f_StepUp();

				_pReporter->f_IncreaseProgress(1);

				Tree.f_FindRoots();
#ifdef DMibDebug
				Tree.f_TraceTree(_pReporter);
#endif
				Tree.f_CopyTree(ClusterSize, ClusterIDsPerCluster, _pStorageStream, _pFixDestinationFS);

				_pReporter->f_IncreaseProgress(1);

				Tree.f_Destroy();

				if (_pReporter)
					_pReporter->f_ReportError("File system successfully rebuilt.");

				_pReporter->f_StepUp();

				return ECheckFSError_None;
			}
			else
			{
				if (_pReporter)
				{
					if (_bFix)
						_pReporter->f_ReportError("Fixing file system.");
					else
						_pReporter->f_ReportError("Checking file system.");
				}

				ECheckFSError Ret = ECheckFSError_None;
				for (int i = 0; i < 2; ++i)
				{

					_pStorageStream->f_SetPosition(0);

					CCheckFSContext Context(_pStorageStream, _pReporter, _bFix, _pFixDestinationFS);

					Ret = ECheckFSError_None;

					Context.mp_RootData.f_Read(*_pStorageStream);
					if (!Context.mp_RootData.f_Validate())
					{
						if (_pReporter)
						{
							_pReporter->f_ReportError("Error: Root data checksum failure");
							_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_ClusterSize {}") << Context.mp_RootData.m_ClusterSize);
							_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_RootFileCluster {}") << Context.mp_RootData.m_RootFileCluster);
							_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_FirstFreeCluster {}") << Context.mp_RootData.m_FirstFreeCluster);
							_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_FileSystemGrowSize {}") << Context.mp_RootData.m_FileSystemGrowSize);
							_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_nFreeClusters {}") << Context.mp_RootData.m_nFreeClusters);
							_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_DirectoryCacheSize {}") << Context.mp_RootData.m_DirectoryCacheSize);
							_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_ClusterCacheSize {}") << Context.mp_RootData.m_ClusterCacheSize);
						}
						Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
						break;
					}

					if (Context.mp_RootData.m_Version < 0x102)
					{
						if (_bFix)
						{
							Context.mp_pReporter->f_ReportError("Upgrading file system version");
						}
						else
						{
							Context.mp_pReporter->f_ReportError("File system version upgrade needed");
							Ret = fg_Max(Ret, ECheckFSError_VersionUpgradeFixable);
						}
					}

					uint64 ClusterSize = Context.mp_RootData.m_ClusterSize;

					Context.mp_ClusterIDsPerCluster = (ClusterSize - (sizeof(NCryptography::CHashDigest_MD5) + sizeof(CClusterID)*2 + sizeof(uint64))) / sizeof(CClusterID);

					CClusterID nClusters = ((_pStorageStream->f_GetLength() + ClusterSize -1) / ClusterSize );
					CMibFilePos Size = CMibFilePos(nClusters) * CMibFilePos(ClusterSize);
					if (Size != _pStorageStream->f_GetLength())
					{
						// Unaligned stream length
						if (_pReporter)
							_pReporter->f_ReportError("Error: The stream isn't a multiple of the cluster size");

						if (_bFix)
						{
							_pStorageStream->f_SetPositionFromEnd(0);
							while (_pStorageStream->f_GetLength() < Size)
								_pStorageStream->f_FeedBytes("\0", 1);
						}
						else
							--nClusters;
					}

					try
					{
						Context.mp_ClusterMap.f_SetLen(nClusters);
					}
					catch (NException::CException)
					{
						return fg_Max(Ret, ECheckFSError_NotCheckableMemoryInsufficient);
					}

					Context.f_AddClusterID(0, 1, CCheckFSContext::EClusterCheckFlag_RootData, true);

					// Add free clusters to cluster map
					Context.f_AddFreeClusters(Context.mp_RootData.m_FirstFreeCluster);

					// Check file tree
					_pReporter->f_StepDown(2);

					Ret = fg_Max(Ret, Context.f_CheckFSTree(Context.mp_RootData.m_RootFileCluster, true));
					_pReporter->f_IncreaseProgress(1);

					// Check for orphaned clusters/files
					Ret = fg_Max(Ret, Context.f_CheckOrphanClusters());
					_pReporter->f_IncreaseProgress(1);
					_pReporter->f_StepUp();

					if (_bFix)
					{
						if (Context.mp_RootData.m_Version < 0x102)
							Context.mp_RootData.m_Version = 0x102;
						// Recount number of free clusters
						Ret = fg_Max(Ret, Context.f_UpdateFreeClusters());
					}

					if (Ret != ECheckFSError_InplaceFixableSecondPass)
						break;
				}
				if (Ret == ECheckFSError_None && _pReporter)
				{
					if (_bFix)
						_pReporter->f_ReportError("Successfully fixed file system errors.");
					else
						_pReporter->f_ReportError("The file system contains no errors.");
				}
				return Ret;
			}

		}
		catch (NException::CException &_Exception)
		{
			if (_pReporter)
			{
				if (_bFix)
					_pReporter->f_ReportError(NStr::CStr("Exception during file system fix: ") + _Exception.f_GetErrorStr());
				else
					_pReporter->f_ReportError(NStr::CStr("Exception during file system check: ") + _Exception.f_GetErrorStr());
			}

			if (_bFix && _pFixDestinationFS)
				return ECheckFSError_NotFixable;
			else
				return ECheckFSError_NewFSFixable;

		}
	}



	CVirtualFS::ECheckFSError CVirtualFS::CCheckFSContext::f_UpdateFreeClusters()
	{
		// Rebuild whole free cluster chain

		mp_RootData.m_FirstFreeCluster = 0;
		mp_RootData.m_nFreeClusters = 0;

		mint nClusters = mp_ClusterMap.f_GetLen();

		CClusterCheck *pClusters = mp_ClusterMap.f_GetArray();

		for (mint i = 0; i < nClusters; ++i)
		{
			if (pClusters[i].m_Flags & EClusterCheckFlag_Free)
			{
				// Search for the type of cluster
				f_Seek(i);

				CFreeCluster NewFree;
				NewFree.m_NextFreeCluster = mp_RootData.m_FirstFreeCluster;
				mp_RootData.m_FirstFreeCluster = i;
				++mp_RootData.m_nFreeClusters;
				NewFree.f_GenerateChecksum(i, mp_RootData.m_Version);
				NewFree.f_Write(*mp_pStorageStream);
			}
		}

		fp_SaveRootData();

		return ECheckFSError_None;
	}

	void CVirtualFS::CCheckFSContext::fp_SaveRootData()
	{
		mp_RootData.f_GenerateChecksum();
		f_Seek(0);
		mp_RootData.f_Write(*mp_pStorageStream);
		mp_pStorageStream->f_Flush(true);
	}

	void CVirtualFS::CCheckFSContext::f_FreeCluster(CClusterID _Cluster)
	{
		//
		mp_ClusterMap[_Cluster].m_ClusterID = 1;
		mp_ClusterMap[_Cluster].m_Flags = EClusterCheckFlag_Free;
	}

	void CVirtualFS::CCheckFSContext::f_Seek(CClusterID _Cluster)
	{
		mp_pStorageStream->f_SetPosition(_Cluster * mp_RootData.m_ClusterSize);
	}

	uint64 CVirtualFS::CCheckFSContext::f_AddClusterID(CClusterID _Cluster, CClusterID _File, uint64 _Flags, bint _bMap)
	{
		if (_Cluster > mp_ClusterMap.f_GetLen())
		{
			if (mp_pReporter)
			{
				mp_pReporter->f_ReportError("Cluster Map: Cluster is invalid");
			}
			return EClusterCheckFlag_Invalid; // Unrecoverable
		}

		if (mp_ClusterMap[_Cluster].m_ClusterID)
		{
			if (mp_pReporter)
			{
				if (mp_ClusterMap[_Cluster].m_Flags & EClusterCheckFlag_Free)
					mp_pReporter->f_ReportError("Cluster Map: Cluster is in free list");
				if (mp_ClusterMap[_Cluster].m_Flags & EClusterCheckFlag_FileRecord)
					mp_pReporter->f_ReportError("Cluster Map: Cluster already belongs to a file record");
				if (mp_ClusterMap[_Cluster].m_Flags & EClusterCheckFlag_FileClusterChain)
					mp_pReporter->f_ReportError("Cluster Map: Cluster already belongs to a file data range");
			}
			return mp_ClusterMap[_Cluster].m_Flags;
		}

		if (_bMap)
		{
			mp_ClusterMap[_Cluster].m_ClusterID = _File;
			mp_ClusterMap[_Cluster].m_Flags = _Flags;
		}
		return 0;
	}

	CVirtualFS::ECheckFSError CVirtualFS::CCheckFSContext::f_AddFreeClusters(CClusterID _ClusterID)
	{
		CClusterID Cluster = _ClusterID;

		uint64 nFreeClusters = 0;

		while (Cluster)
		{
			f_Seek(Cluster);

			CFreeCluster FreeCluster;
			FreeCluster.f_Read(*mp_pStorageStream);

			if (!FreeCluster.f_Validate(Cluster, mp_RootData.m_Version))
			{
				if (mp_pReporter)
				{
					mp_pReporter->f_ReportError("Free Cluster Failed Validation");
				}

				// We need to exit here because we cannot trust the data
				if (mp_bFix)
					return ECheckFSError_None;
				else
					return ECheckFSError_InplaceFixable;
			}

			++nFreeClusters;
			if (f_AddClusterID(Cluster, 1, EClusterCheckFlag_Free, true))
			{
				if (mp_pReporter)
				{
					mp_pReporter->f_ReportError("Failed to add free cluster to cluster map, aborting");
				}

				// We need to exit here in case we are in an circular loop
				if (mp_bFix)
					return ECheckFSError_None;
				else
					return ECheckFSError_InplaceFixable;
			}

			Cluster = FreeCluster.m_NextFreeCluster;
		}
		if (nFreeClusters != mp_RootData.m_nFreeClusters)
		{
			if (mp_pReporter)
			{
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("Number of free  clusters does not match {} != {}") << nFreeClusters << mp_RootData.m_nFreeClusters);
			}
			return ECheckFSError_InplaceFixable;
		}
		else
			return ECheckFSError_None;
	}

	bint CVirtualFS::CCheckFSContext::f_WriteFileData(CVirtualFS::CFileRecord &_FileRecord, NContainer::CByteVector &_Data, mint _Length)
	{
		DMibSafeCheck(_Data.f_GetLen() <= _FileRecord.m_FileSize, "Write can only shrink file");

		_FileRecord.m_FileSize = _Length;

		CClusterChain Chain;
		Chain.m_ClusterIDs.f_SetLen(mp_ClusterIDsPerCluster);

		CClusterID Cluster = _FileRecord.m_FirstFileChainCluster;

		mint iCluster = 0;
		mint iEndCluster = (_FileRecord.m_FileSize + mp_RootData.m_ClusterSize - 1) / mp_RootData.m_ClusterSize;

		while (Cluster)
		{
			f_Seek(Cluster);
			Chain.f_Read(*mp_pStorageStream);

			int Changed = 0;

			if (!Chain.f_Validate(Cluster, mp_RootData.m_Version))
				return false;

			if (iCluster >= iEndCluster)
			{
				f_FreeCluster(Cluster);
				Changed = 2;
			}

			uint64 nClusterIDs = Chain.m_nClusterIDs;
			for (uint64 i = 0; i < nClusterIDs; ++i)
			{
				if (iCluster >= iEndCluster)
				{
					f_FreeCluster(Chain.m_ClusterIDs[i]);
					Changed = fg_Max(Changed, 1);
					--Chain.m_nClusterIDs;
				}
				else
				{
					f_Seek(Chain.m_ClusterIDs[i]);

					mint Size = fg_Min(_FileRecord.m_FileSize - iCluster * mp_RootData.m_ClusterSize, mp_RootData.m_ClusterSize);

					mp_pStorageStream->f_FeedBytes(_Data.f_GetArray() + iCluster * mp_RootData.m_ClusterSize, Size);

					++iCluster;
				}
			}

			if (iCluster >= iEndCluster)
			{
				Chain.m_NextChainCluster = 0;
				Changed = fg_Max(Changed, 1);
			}

			if (Changed == 1)
			{
				f_Seek(Cluster);
				Chain.f_GenerateChecksum(Cluster, mp_RootData.m_Version);
				Chain.f_Write(*mp_pStorageStream);
			}

			Cluster = Chain.m_NextChainCluster;
		}

		return true;
	}

	bint CVirtualFS::CCheckFSContext::fs_ReadFileData(uint32 _Version, uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream, CVirtualFS::CFileRecord &_FileRecord, NContainer::CByteVector &_Data)
	{
		_Data.f_SetLen(_FileRecord.m_FileSize);

		CClusterChain Chain;
		Chain.m_ClusterIDs.f_SetLen(_ClusterIDsPerCluster);

		CClusterID Cluster = _FileRecord.m_FirstFileChainCluster;

		mint iCluster = 0;

		while (Cluster)
		{
			_pStorageStream->f_SetPosition(Cluster * _ClusterSize);
			Chain.f_Read(*_pStorageStream);

			if (!Chain.f_Validate(Cluster, _Version))
				return false;

			for (uint64 i = 0; i < Chain.m_nClusterIDs; ++i)
			{
				_pStorageStream->f_SetPosition(Chain.m_ClusterIDs[i] * _ClusterSize);

				if (iCluster * _ClusterSize < _FileRecord.m_FileSize)
				{
					mint Size = fg_Min(_FileRecord.m_FileSize - iCluster * _ClusterSize, _ClusterSize);

					_pStorageStream->f_ConsumeBytes(_Data.f_GetArray() + iCluster * _ClusterSize, Size);
				}

				++iCluster;
			}

			Cluster = Chain.m_NextChainCluster;
		}

		return true;
	}

	bint CVirtualFS::CCheckFSContext::fs_CheckClusterChain(uint32 _Version, uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream, CVirtualFS::CFileRecord &_FileRecord)
	{
		CClusterChain Chain;
		Chain.m_ClusterIDs.f_SetLen(_ClusterIDsPerCluster);

		CClusterID Cluster = _FileRecord.m_FirstFileChainCluster;

		while (Cluster)
		{
			_pStorageStream->f_SetPosition(Cluster * _ClusterSize);
			Chain.f_Read(*_pStorageStream);

			if (!Chain.f_Validate(Cluster, _Version))
				return false;

			for (uint64 i = 0; i < Chain.m_nClusterIDs; ++i)
			{
				uint32 Position = Chain.m_ClusterIDs[i] * _ClusterSize;
				if (!_pStorageStream->f_IsValidReadPosition(Position))
					return false;
			}

			Cluster = Chain.m_NextChainCluster;
		}

		return true;
	}

	bint CVirtualFS::CCheckFSContext::f_ReadFileData(CVirtualFS::CFileRecord &_FileRecord, NContainer::CByteVector &_Data)
	{
		_Data.f_SetLen(_FileRecord.m_FileSize);

		CClusterChain Chain;
		Chain.m_ClusterIDs.f_SetLen(mp_ClusterIDsPerCluster);

		CClusterID Cluster = _FileRecord.m_FirstFileChainCluster;

		mint iCluster = 0;

		while (Cluster)
		{
			f_Seek(Cluster);
			Chain.f_Read(*mp_pStorageStream);

			if (!Chain.f_Validate(Cluster, mp_RootData.m_Version))
				return false;

			for (uint64 i = 0; i < Chain.m_nClusterIDs; ++i)
			{
				f_Seek(Chain.m_ClusterIDs[i]);

				if (iCluster * mp_RootData.m_ClusterSize < _FileRecord.m_FileSize)
				{
					mint Size = fg_Min(_FileRecord.m_FileSize - iCluster * mp_RootData.m_ClusterSize, mp_RootData.m_ClusterSize);

					mp_pStorageStream->f_ConsumeBytes(_Data.f_GetArray() + iCluster * mp_RootData.m_ClusterSize, Size);
				}

				++iCluster;
			}

			Cluster = Chain.m_NextChainCluster;
		}

		return true;
	}

	CVirtualFS::ECheckFSError CVirtualFS::CCheckFSContext::f_CheckOrphanClusters()
	{
		ECheckFSError Ret = ECheckFSError_None;


		mint nClusters = mp_ClusterMap.f_GetLen();

		mp_pReporter->f_StepDown(nClusters * 2);

		mint nOrphan = 0;
		mint nFiles = 0;

		CClusterCheck *pClusters = mp_ClusterMap.f_GetArray();
		for (mint i = 0; i < nClusters; ++i)
		{
			if (!pClusters[i].m_ClusterID)
			{
				// Search for the type of cluster
				f_Seek(i);
				CVirtualFS::CFileRecord FileRecord;
				FileRecord.f_Read(*mp_pStorageStream);

				if (FileRecord.f_Validate(i, mp_RootData.m_Version)) // This is a file record
				{
					++nFiles;
					if (mp_bFix)
					{
						f_FreeCluster(i);
					}
					else
					{
						ECheckFSError TreeCheck = f_CheckFSTree(i, false);

						if (TreeCheck != ECheckFSError_None)
						{
							// If this file is invalid lets just throw it away
							Ret = fg_Max(Ret, ECheckFSError_InplaceFixable);
						}
						else
						{
							// If we have only one invalid file, lets just kill it
							if (nFiles <= 1)
								Ret = fg_Max(Ret, ECheckFSError_InplaceFixable);
							else
								Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
						}
					}
				}
			}
			mp_pReporter->f_IncreaseProgress(1);
		}

		for (mint i = 0; i < nClusters; ++i)
		{
			if (!pClusters[i].m_ClusterID)
			{
				// Search for the type of cluster
				f_Seek(i);
				CVirtualFS::CFileRecord FileRecord;
				FileRecord.f_Read(*mp_pStorageStream);

				if (FileRecord.f_Validate(i, mp_RootData.m_Version)) // This is a file record
				{
					if (mp_bFix)
					{
//							DMibSafeCheck(0, "No files should be here");
					}
				}
				else
				{
					++nOrphan;
					if (mp_bFix)
					{
						f_FreeCluster(i);
					}
					else
					{
						Ret = fg_Max(Ret, ECheckFSError_InplaceFixable); // We can just return the data
					}
				}
			}
			mp_pReporter->f_IncreaseProgress(1);
		}

		if (mp_pReporter)
		{
			if (nOrphan)
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("Found {} orphan clusters.") << nOrphan);
			if (nFiles)
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("Found {} orphan files.") << nFiles);
		}

		mp_pReporter->f_StepUp();

		return Ret;
	}

	CVirtualFS::ECheckFSError CVirtualFS::CCheckFSContext::f_CheckFSTree(CClusterID _ClusterID, bint _bMap)
	{
		CCircularChecker CircularChecker;
		bint bRemove = false;
		ECheckFSError Ret = fr_CheckFSTree(_ClusterID, CircularChecker, bRemove, _bMap, 0, "Root");
		if (bRemove)
			Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
		return Ret;
	}

	CVirtualFS::ECheckFSError CVirtualFS::CCheckFSContext::fr_CheckFSTree(CClusterID _ClusterID, CCircularChecker &_CircularChecker, bint &_bRemove, bint _bMap, CDirectoryID _ParentDir, NStr::CStr const &_FileName)
	{
		DMibSafeCheck(!_CircularChecker.f_AlreadyInList(_ClusterID), "We should make sure this does not happen in the directory iterator");

		ECheckFSError Ret = ECheckFSError_None;

		CCircularChecker::CMember Member;
		Member.m_ClusterID = _ClusterID;
		_CircularChecker.m_List.f_Insert(Member);

		//

		f_Seek(_ClusterID);

		uint64 AddResult = f_AddClusterID(_ClusterID, _ClusterID, EClusterCheckFlag_FileRecord, _bMap);

		if (AddResult)
		{
			if (AddResult & EClusterCheckFlag_FileRecord)
			{
				if (mp_pReporter)
				{
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: This file record is already referenced somewhere else ") << _FileName);
				}
				return fg_Max(Ret, ECheckFSError_NewFSFixable);
			}

			if (AddResult & EClusterCheckFlag_Invalid)
			{
				if (mp_pReporter)
				{
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: The file record cluster is invalid (outside of the file stream)") << _FileName);
				}
				return fg_Max(Ret, ECheckFSError_NewFSFixable);
			}

			if (AddResult & EClusterCheckFlag_Free)

			Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
		}

		CVirtualFS::CFileRecord FileRecord;
		FileRecord.f_Read(*mp_pStorageStream);
		if (!FileRecord.f_Validate(_ClusterID, mp_RootData.m_Version))
		{
			if (mp_pReporter)
			{
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: File record checksum failure") << _FileName);
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_CreationTime {}") << FileRecord.m_CreationTime.f_GetSeconds());
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_AccessTime {}") << FileRecord.m_AccessTime.f_GetSeconds());
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_WriteTime {}") << FileRecord.m_WriteTime.f_GetSeconds());
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_FileSize {}") << FileRecord.m_FileSize);
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_FileAttributes 0x{nfh,sj16,sf0}") << FileRecord.m_FileAttributes);
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_FirstFileChainCluster {}") << FileRecord.m_FirstFileChainCluster);
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_ParentDirectory {}") << FileRecord.m_ParentDirectory);
			}

			if (mp_bFix)
			{
				f_FreeCluster(_ClusterID);
				_bRemove = true;
				return Ret;
			}
			else
			{
//					return fg_Max(Ret, ECheckFSError_InplaceFixable);
				return fg_Max(Ret, ECheckFSError_NewFSFixable);
			}
		}

		bint bChangedFileRecord = false;

		if (FileRecord.m_ParentDirectory != _ParentDir)
		{
			if (mp_pReporter)
			{
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: File record parent directory ID mismatch") << _FileName);
			}

			if (mp_bFix)
			{
				FileRecord.m_ParentDirectory = _ParentDir;
				bChangedFileRecord = true;
			}
			else
			{
//					return fg_Max(Ret, ECheckFSError_InplaceFixable);
				return fg_Max(Ret, ECheckFSError_NewFSFixable);
			}
		}

		// Check file size and cluster allocation

		CClusterChain Chain;
		Chain.m_ClusterIDs.f_SetLen(mp_ClusterIDsPerCluster);

		CClusterID Current = FileRecord.m_FirstFileChainCluster;
		uint64 FileSize = 0;
		uint64 iEndCluster = (FileRecord.m_FileSize + mp_RootData.m_ClusterSize - 1) / mp_RootData.m_ClusterSize;
		uint64 iCluster = 0;

		while (Current)
		{
			f_Seek(Current);
			Chain.f_Read(*mp_pStorageStream);
			int Changed = 0;

			if (!Chain.f_Validate(Current, mp_RootData.m_Version))
			{
				if (mp_pReporter)
				{
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: File id chain checksum error") << _FileName);
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_ClusterFirstIDIndex {}") << Chain.m_ClusterFirstIDIndex);
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_nClusterIDs {}") << Chain.m_nClusterIDs);
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_NextChainCluster {}") << Chain.m_NextChainCluster);
				}

				// Delete this entry
				if (mp_bFix)
				{
					// Free file
					f_FreeCluster(_ClusterID);
					_bRemove = true;
					Ret = fg_Max(Ret, ECheckFSError_InplaceFixableSecondPass);

					return Ret;
				}
				else
				{
					return fg_Max(Ret, ECheckFSError_NewFSFixable);
//					return fg_Max(Ret, ECheckFSError_InplaceFixable);
				}
			}

//				mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Checking chain {} {}") << _FileName << Current << Chain.m_NextChainCluster);

			uint64 AddResult = f_AddClusterID(Current, _ClusterID, EClusterCheckFlag_FileClusterChain, _bMap);

			if (AddResult)
			{
				if (mp_pReporter)
				{
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: File cluster chain cluster used somewhere else") << _FileName);
				}
				Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
			}

			if (iCluster >= iEndCluster)
			{
				if (mp_pReporter)
				{
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: File id chain contains too many ids") << _FileName);
				}
				if (mp_bFix)
				{
					f_FreeCluster(Current);
					Changed = 2;
				}
				else
					Ret = fg_Max(Ret, ECheckFSError_InplaceFixable);
			}

			// Add all custers to the cluster map


			uint64 nClusterIDs = Chain.m_nClusterIDs;
			for (uint64 i = 0; i < nClusterIDs; ++i)
			{
				if (iCluster >= iEndCluster && mp_bFix)
				{
					f_FreeCluster(Chain.m_ClusterIDs[i]);
					Changed = fg_Max(Changed, 1);
					--Chain.m_nClusterIDs;

				}
				else
				{
					FileSize += mp_RootData.m_ClusterSize;
					uint64 AddResult = f_AddClusterID(Chain.m_ClusterIDs[i], _ClusterID, EClusterCheckFlag_FileData, _bMap);

					if (AddResult)
					{
						if (mp_pReporter)
						{
							mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: File data cluster used somewhere else") << _FileName);
						}
						Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
					}

					++iCluster;
				}
			}

			CClusterID NextCluster = Chain.m_NextChainCluster;

			if (iCluster >= iEndCluster)
			{
				if (Chain.m_NextChainCluster != 0)
				{
					if (mp_pReporter)
					{
						mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: Cluster chain error") << _FileName);
					}
					if (mp_bFix)
					{
						Chain.m_NextChainCluster = 0;
						Changed = fg_Max(Changed, 1);
					}
					else
						Ret = fg_Max(Ret, ECheckFSError_InplaceFixable);

				}
			}

			if (Changed == 1)
			{
				bChangedFileRecord = true;
				f_Seek(Current);
				Chain.f_GenerateChecksum(Current, mp_RootData.m_Version);
				Chain.f_Write(*mp_pStorageStream);
			}

			Current = NextCluster;
		}

		if (FileRecord.m_FileSize > FileSize || iCluster > iEndCluster || bChangedFileRecord)
		{
			if (mp_pReporter)
			{
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: File cluster data size mismatch") << _FileName);
			}
			if (mp_bFix)
			{
				FileRecord.m_FileSize = fg_Min(FileRecord.m_FileSize, FileSize);
				f_Seek(_ClusterID);
				FileRecord.f_GenerateChecksum(_ClusterID, mp_RootData.m_Version);
				FileRecord.f_Write(*mp_pStorageStream);
			}
			else
			{
				Ret = fg_Max(Ret, ECheckFSError_InplaceFixable);
			}
		}

		if (FileRecord.m_FileAttributes & NFile::EFileAttrib_Directory)
		{
			// Read directory data
			NContainer::CByteVector DirectoryData;
			f_ReadFileData(FileRecord, DirectoryData);

			NStream::CBinaryStreamMemoryPtr<> Stream;
			Stream.f_OpenRead(DirectoryData.f_GetArray(), DirectoryData.f_GetLen());

			CVirtualFS::CDirectory Directory;
			Directory.f_Read(Stream);
			bint bChanged = false;

			if (!Directory.f_Validate(_ClusterID, mp_RootData.m_Version))
			{
				if (mp_pReporter)
				{
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: Directory failed validation") << _FileName);
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_Files.f_GetLen() {}") << Directory.m_Files.f_GetLen());
				}

				// We cannot trust the data in the diretory
				Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
			}
			else
			{
				mp_MaxDirectoryID = fg_Max(mp_MaxDirectoryID, Directory.m_DirectoryID);

				mint nFiles = Directory.m_Files.f_GetLen();

				mp_pReporter->f_StepDown(nFiles);

				for (mint i = 0; i < nFiles; ++i)
				{
					CClusterID ClusterID = Directory.m_Files[i].m_FileRecordCluster;
					NStr::CStr FileName = Directory.m_Files[i].m_FileName;

					if (_CircularChecker.f_AlreadyInList(ClusterID))
					{
						if (mp_pReporter)
						{
							mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: Circular directory dependency found") << _FileName);
						}
						Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
					}
					else
					{
						bint bRemove = false;
						Ret = fg_Max(Ret, fr_CheckFSTree(ClusterID, _CircularChecker, bRemove, _bMap, Directory.m_DirectoryID, _FileName + "/" + FileName));

						if (bRemove && mp_bFix)
						{
							bChanged = true;
							Directory.m_Files.f_Remove(i);
							--i;
							--nFiles;
						}
					}
					mp_pReporter->f_IncreaseProgress(1);
				}

				mp_pReporter->f_StepUp();
			}

			// Test directory size
			if (!bChanged)
			{
				NStream::CBinaryStreamMemory<> Stream;
				Directory.f_GenerateChecksum(_ClusterID, mp_RootData.m_Version);
				Directory.f_Write(Stream);
				if ((uint64)Stream.f_GetLength() != FileRecord.m_FileSize)
				{
					//DMibSafeCheck((uint64)Stream.f_GetLength() < FileRecord.m_FileSize, "Should not happen");
					if (mp_pReporter)
					{
						mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: Directory file length not correct") << _FileName);
					}
					if (mp_bFix)
					{
						bChanged = true;
					}
					else
					{
						Ret = fg_Max(Ret, ECheckFSError_InplaceFixable);
					}
				}
			}

			if (mp_RootData.m_Version == 0x101 && mp_bFix)
			{
				// Convert to new version;
				bChanged = true;
			}

			if (bChanged)
			{
				NStream::CBinaryStreamMemory<> Stream;
				Directory.f_GenerateChecksum(_ClusterID, mp_RootData.m_Version);
				Directory.f_Write(Stream);

				f_WriteFileData(FileRecord, Stream.f_GetVector(), Stream.f_GetLength());

				f_Seek(_ClusterID);
				FileRecord.f_GenerateChecksum(_ClusterID, mp_RootData.m_Version);
				FileRecord.f_Write(*mp_pStorageStream);
			}
		}
		return Ret;
	}

	void CVirtualFS::f_Create(NStream::CBinaryStream *_pStorageStream, uint64 _ClusterSize, uint64 _FileSystemGrowSize, uint64 _InitialSize)
	{
		fp_Close();
		if (_ClusterSize < 128)
			DMibErrorFile("Cluster size have to be equal to or larger than 128 bytes");
		if (((_FileSystemGrowSize / _ClusterSize) * _ClusterSize != _FileSystemGrowSize))
			DMibErrorFile("File system grow size has to be a multiple of the cluster size");

		mp_pStorageStream = _pStorageStream;
		mp_pStorageStream->f_SetCacheSize(_ClusterSize);
		mp_RootData.m_ClusterSize = _ClusterSize;
		mp_RootData.m_RootFileCluster = 1;
		mp_RootData.m_FirstFreeCluster = 0;
		mp_RootData.m_FileSystemGrowSize = _FileSystemGrowSize / _ClusterSize;
		mp_RootData.m_nFreeClusters = 0;
		mp_RootData.m_DirectoryCacheSize = 256;
		mp_RootData.m_CurrentDirectoryID = 0;
		mp_RootData.m_ClusterCacheSize = (8*1024*1024 + _ClusterSize - 1) / _ClusterSize; // Cache 8 MiB of data as default
		mp_DirectoryCache.f_SetCacheSize(mp_RootData.m_DirectoryCacheSize);
		mp_ClusterCache.f_SetCacheSize(mp_RootData.m_ClusterCacheSize);

		mp_ClusterIDsPerCluster = (mp_RootData.m_ClusterSize - (sizeof(NCryptography::CHashDigest_MD5) + sizeof(CClusterID)*2 + sizeof(uint64))) / sizeof(CClusterID);
		CClusterID InitialClusters = _InitialSize / _ClusterSize;

		if (InitialClusters < 4)
			InitialClusters = 4; // Make room for root cluster, a root directory and its file record

		// Make file the required size
		{
			mp_pStorageStream->f_SetPosition(fp_GetFilePos(InitialClusters) - sizeof(uint32));
			uint32 Temp = 0;
			*mp_pStorageStream << Temp;
		}

		// Write root directory
		{
			// Cluster 0 = Root data
			// Cluster 1 = Root file record
			// Cluster 2 = Root file record file chain data
			// Cluster 3 = Directory data
			CFileRecord RootCluster;
			CDirectory Directory;
			Directory.m_DirectoryID = ++mp_RootData.m_CurrentDirectoryID;
			Directory.f_GenerateChecksum(1, mp_RootData.m_Version);
			fp_SetStreamCluster(3);
			Directory.f_Write(*mp_pStorageStream);
			uint64 Size = mp_pStorageStream->f_GetPosition() - fp_GetFilePos(3);

			CClusterID NumFreeClusters = InitialClusters - 4;
			mp_RootData.m_nFreeClusters = NumFreeClusters;
			if (NumFreeClusters)
				mp_RootData.m_FirstFreeCluster = 4;
			else
				mp_RootData.m_FirstFreeCluster = 0;

			for (CClusterID i = 0; i < NumFreeClusters; ++i)
			{
				CFreeCluster NewFreeCluster;
				if (i < (NumFreeClusters-1))
					NewFreeCluster.m_NextFreeCluster = mp_RootData.m_FirstFreeCluster + i+1;
				else
					NewFreeCluster.m_NextFreeCluster = 0; // No more free
				NewFreeCluster.f_GenerateChecksum(mp_RootData.m_FirstFreeCluster + i, mp_RootData.m_Version);
				fp_SetStreamCluster(mp_RootData.m_FirstFreeCluster + i);
				NewFreeCluster.f_Write(*mp_pStorageStream);
			}

			NTime::CTime Time = NTime::CTime::fs_NowUTC();
			RootCluster.m_CreationTime = Time;
			RootCluster.m_AccessTime = Time;
			RootCluster.m_WriteTime = Time;
			RootCluster.m_FileSize = Size;
			RootCluster.m_FileAttributes = EFileAttrib_Directory | EFileAttrib_System;
			RootCluster.m_FirstFileChainCluster = 2;
			RootCluster.m_ParentDirectory = 0;
			RootCluster.f_GenerateChecksum(1, mp_RootData.m_Version);
			fp_SetStreamCluster(1);
			RootCluster.f_Write(*mp_pStorageStream);

			fp_SetStreamCluster(2);
			CClusterChain Chain;
			Chain.m_ClusterIDs.f_SetLen(1);
			Chain.m_ClusterIDs[0] = 3;
			Chain.m_nClusterIDs = 1;
			Chain.m_ClusterFirstIDIndex = 0;
			Chain.m_NextChainCluster = 0;
			Chain.f_GenerateChecksum(2, mp_RootData.m_Version);
			Chain.f_Write(*mp_pStorageStream);
		}

		//CDirectory

		fp_SaveRootData();
	}
	void CVirtualFS::f_SetCacheSizes(mint _nDirectories, mint _nClusterBytes)
	{
		mint nClusters = fg_Max((_nClusterBytes + mp_RootData.m_ClusterSize - 1) / mp_RootData.m_ClusterSize, mint(1));
		mp_DirectoryCache.f_SetCacheSize(fg_Max(mint(1), _nDirectories));
		mp_ClusterCache.f_SetCacheSize(nClusters);
	}

	void CVirtualFS::f_CloseOpenFiles()
	{
		CFile *pFile = m_OpenFiles.f_Pop();
		while (pFile)
		{
			pFile->f_Close();
			pFile = m_OpenFiles.f_Pop();
		}
	}

	void CVirtualFS::fp_Close()
	{
		// Close open user files
		f_CloseOpenFiles();
		if (!mp_bOutOfSpace)
			f_Flush();
		mp_DirectoryCache.f_Destroy();
		mp_ClusterCache.f_Destroy();

		if (mp_pStorageStream)
		{
			mp_pStorageStream = nullptr;
		}
	}

	void CVirtualFS::fp_SaveRootData()
	{
		mp_RootData.f_GenerateChecksum();
		fp_SetStreamCluster(0);
		mp_RootData.f_Write(*mp_pStorageStream);
	}

	CVirtualFS::CClusterID CVirtualFS::fp_GetNewCluster()
	{
		if (!mp_RootData.m_nFreeClusters && mp_RootData.m_FileSystemGrowSize)
		{
			try
			{
				CFilePos CurrentLength = mp_pStorageStream->f_GetLength() / mp_RootData.m_ClusterSize;
				CFilePos NewLength = CurrentLength + mp_RootData.m_FileSystemGrowSize;
				mp_pStorageStream->f_SetPosition(NewLength * mp_RootData.m_ClusterSize - sizeof(uint32));
				uint32 Dummy = 0;
				mp_pStorageStream->f_FeedBytes(&Dummy, sizeof(uint32));

				CClusterID NumFreeClusters = NewLength - CurrentLength;
				mp_RootData.m_nFreeClusters = NumFreeClusters;
				if (NumFreeClusters)
					mp_RootData.m_FirstFreeCluster = CurrentLength;
				else
					mp_RootData.m_FirstFreeCluster = 0;

				for (CClusterID i = 0; i < NumFreeClusters; ++i)
				{
					CFreeCluster NewFreeCluster;
					if (i < (NumFreeClusters-1))
						NewFreeCluster.m_NextFreeCluster = mp_RootData.m_FirstFreeCluster + i+1;
					else
						NewFreeCluster.m_NextFreeCluster = 0; // No more free
					NewFreeCluster.f_GenerateChecksum(mp_RootData.m_FirstFreeCluster + i, mp_RootData.m_Version);
					CClusterCacheEntry *pCluster = mp_ClusterCache.f_GetCluster(mp_RootData.m_FirstFreeCluster + i);
					NStream::CBinaryStreamMemoryPtr<> Stream;
					pCluster->f_OpenStream(Stream);
					NewFreeCluster.f_Write(Stream);
					pCluster->f_CloseStream(true);
					mp_ClusterCache.f_ReturnCluster(pCluster);
				}

				fp_SaveRootData();

			}
			catch (CExceptionFile)
			{
				mp_bOutOfSpace = true;
				DMibErrorFile("Out of space");
			}
		}

		if (mp_RootData.m_FirstFreeCluster)
		{
			CClusterID FreeClusterID = mp_RootData.m_FirstFreeCluster;
			CClusterCacheEntry *pCluster = mp_ClusterCache.f_GetCluster(mp_RootData.m_FirstFreeCluster);
			NStream::CBinaryStreamMemoryPtr<> Stream;
			CFreeCluster FreeCluster;
			pCluster->f_OpenStream(Stream);
			FreeCluster.f_Read(Stream);
			pCluster->f_CloseStream(false);
			mp_ClusterCache.f_ReturnCluster(pCluster);

			if (!FreeCluster.f_Validate(mp_RootData.m_FirstFreeCluster, mp_RootData.m_Version))
			{
				DMibErrorFile("Free cluster record checksum failure");
			}

			mp_RootData.m_FirstFreeCluster = FreeCluster.m_NextFreeCluster;
			--mp_RootData.m_nFreeClusters;

			fp_SaveRootData();

			return FreeClusterID;
		}

		DMibErrorFile("Out of space");
		mp_bOutOfSpace = true;

//			return 0;
	}

	void CVirtualFS::fp_ReturnCluster(CVirtualFS::CClusterID _Cluster)
	{
		CFreeCluster NewFree;
		NewFree.m_NextFreeCluster = mp_RootData.m_FirstFreeCluster;
		mp_RootData.m_FirstFreeCluster = _Cluster;
		++mp_RootData.m_nFreeClusters;
		NewFree.f_GenerateChecksum(_Cluster, mp_RootData.m_Version);

		CClusterCacheEntry *pCluster = mp_ClusterCache.f_GetCluster(_Cluster);
		NStream::CBinaryStreamMemoryPtr<> Stream;
		pCluster->f_OpenStream(Stream);
		NewFree.f_Write(Stream);
		pCluster->f_CloseStream(true);
		mp_ClusterCache.f_ReturnCluster(pCluster);
		fp_SaveRootData();
	}

	void CVirtualFS::CFileInternal::f_SetLength(uint64 _Length)
	{
		if (m_FileRecord.m_FileSize != _Length)
		{
			f_Flush();
			if (_Length < m_FileRecord.m_FileSize)
			{
				if (m_FileRecord.m_FirstFileChainCluster)
				{
					CClusterID CurrentID = m_FileRecord.m_FirstFileChainCluster;
					f_ReadClusterChain(CurrentID);
					m_bFileRecordDirty = true;
					m_FileRecord.m_FileSize = _Length;
					uint64 ClusterSize = m_pVirtualFS->mp_RootData.m_ClusterSize;

					while (1)
					{
						CClusterID Next = m_ClusterChain.m_NextChainCluster;
						if (m_ClusterChain.m_ClusterFirstIDIndex * ClusterSize >= _Length)
						{
							for (uint64 i = 0; i < m_ClusterChain.m_nClusterIDs; ++i)
							{
								m_pVirtualFS->fp_ReturnCluster(m_ClusterChain.m_ClusterIDs[i]);
							}
							m_pVirtualFS->fp_ReturnCluster(CurrentID);
							m_bClusterChainDirty = false;
							m_bValidClusterChain = false;
							m_ClusterChain.m_NextChainCluster = 0;
							m_ClusterChain.m_ClusterFirstIDIndex = 0;
							m_ClusterChain.m_nClusterIDs = 0;
							if (CurrentID == m_FileRecord.m_FirstFileChainCluster)
								m_FileRecord.m_FirstFileChainCluster = 0;
						}
						else if ((m_ClusterChain.m_ClusterFirstIDIndex + m_ClusterChain.m_nClusterIDs) * ClusterSize >= _Length)
						{
							int64 nNewClusterIDs = ((_Length + ClusterSize - 1) / ClusterSize) - m_ClusterChain.m_ClusterFirstIDIndex;
							for (uint64 i = nNewClusterIDs; i < m_ClusterChain.m_nClusterIDs; ++i)
							{
								m_pVirtualFS->fp_ReturnCluster(m_ClusterChain.m_ClusterIDs[i]);
							}
							m_ClusterChain.m_nClusterIDs = nNewClusterIDs;
							m_ClusterChain.m_NextChainCluster = 0;
							m_bClusterChainDirty = true;
						}
						if (Next)
						{
							CurrentID = Next;
							f_ReadClusterChain(Next);
						}
						else
							break;
					}
				}
			}
			else
			{
				CFilePos Pos = _Length;
				f_GetCluster(Pos);
				m_FileRecord.m_FileSize = _Length;
				m_bFileRecordDirty = true;
			}
			f_Flush();
		}
	}


	CVirtualFS::CFileInternal *CVirtualFS::fp_OpenNewFile()
	{

		CVirtualFS::CClusterID FileCluster = 0;
		CVirtualFS::CClusterID FileChainCluster = 0;
		CFileInternal *pFile = nullptr;
		try
		{
			FileCluster = fp_GetNewCluster();
//				FileChainCluster = fp_GetNewCluster();
			pFile = DMibNew CFileInternal;
			pFile->f_Create(this, FileCluster, false);

			pFile->m_FileRecord.f_Reset();
			pFile->m_FileRecord.m_FileAttributes = 0;
			pFile->m_FileRecord.m_FirstFileChainCluster = FileChainCluster;
			pFile->m_bFileRecordDirty = true;

			pFile->m_ClusterChain.m_NextChainCluster = 0;
			pFile->m_ClusterChain.m_ClusterFirstIDIndex = 0;
			pFile->m_ClusterChain.m_nClusterIDs = 0;
			pFile->m_bClusterChainDirty = false;
			pFile->m_ClusterChainCluster = FileChainCluster;

			++pFile->m_RefCount;
			mp_InternalFiles.f_Insert(pFile);
		}
		catch (NException::CException)
		{
			if (FileCluster)
				fp_ReturnCluster(FileCluster);

			if (FileChainCluster)
				fp_ReturnCluster(FileChainCluster);

			if (pFile)
				delete pFile;

			throw;
		}

		return pFile;
	}

	CVirtualFS::CFileInternal *CVirtualFS::fp_OpenFile(CClusterID _ClusterID)
	{
		CFileInternal *pFile = mp_InternalFiles.f_FindEqual(_ClusterID);
		if (pFile)
		{
			++pFile->m_RefCount;
			return pFile;
		}

		try
		{
			pFile = DMibNew CFileInternal;
			pFile->f_Create(this, _ClusterID, true);
			++pFile->m_RefCount;
			mp_InternalFiles.f_Insert(pFile);
		}
		catch (NException::CException)
		{
			if (pFile)
				delete pFile;

			throw;
		}

		return pFile;
	}

	void CVirtualFS::fp_CloseFile(CFileInternal *_pFile)
	{
		--_pFile->m_RefCount;

		if (_pFile->m_RefCount == 0)
		{
			mp_InternalFiles.f_Remove(_pFile);
			_pFile->f_Destroy();
			delete _pFile;
		}

	}

	void CVirtualFS::f_EnumFiles(NStr::CStr const &_Path, NContainer::TCVector<NStr::CStr> &_Files)
	{
		fp_CheckPath(_Path, true);
		CDirectoryCacheEntry *pEntry = mp_DirectoryCache.f_GetDirectory(_Path);

		if (!pEntry)
		{
			DMibErrorFile("No such directory");
		}

		_Files.f_SetLen(pEntry->m_Directory.m_Files.f_GetLen());

		for (mint i = 0; i < _Files.f_GetLen(); ++i)
			_Files[i] = pEntry->m_Directory.m_Files[i].m_FileName;

		mp_DirectoryCache.f_ReturnDirectory(pEntry);
	}

	static bint fsg_MatchPattern(const ch8 *_pStr, const ch8 *_pPattern)
	{
		NStr::CStr Temp0 = NStr::CStr(_pStr).f_UpperCase();
		NStr::CStr Temp1 = NStr::CStr(_pPattern).f_UpperCase();
		const char *pParse = Temp0;
		const char *pPattern = Temp1;

		while (*pParse && *pPattern)
		{
			if (*pPattern == '*')
			{
				++pPattern;
				while (*pParse && *pParse != *pPattern)
					++pParse;
			}
			else if (*pPattern == '?')
			{
				++pPattern;
				++pParse;
			}
			else
			{
				if (*pPattern != *pParse)
					break;
				++pPattern;
				++pParse;
			}
		}

		if (*pParse == *pPattern)
			return true;

		if (*pParse == 0 && *pPattern == '*' && pPattern[1] == 0)
			return true;

		return false;
	}

	static void fsg_FindFilesRecursive(CVirtualFS *_pFS, const NStr::CStr &_Path, const NStr::CStr &_Pattern, uint32 _AttribMask, bint _bRecursive, NContainer::TCVector<NStr::CStr> &_Files)
	{
		NContainer::TCVector<NStr::CStr> Files;
		if (!_Path.f_IsEmpty() && !_pFS->f_FileExists(_Path, EFileAttrib_Directory))
			return;
		_pFS->f_EnumFiles(_Path, Files);

		mint nFiles = Files.f_GetLen();

		for (mint i = 0; i < nFiles; ++i)
		{
			NStr::CStr FileName = Files[i];
			if (fsg_MatchPattern(FileName, _Pattern))
			{
				NStr::CStr FullFileName = NFile::CFile::fs_AppendPath(_Path, FileName);

				uint32 Attribs = (uint32)_pFS->f_GetAttributes(FullFileName);
				if ((!(Attribs & NFile::EFileAttrib_Directory) && (_AttribMask & NFile::EFileAttrib_File)) || Attribs & _AttribMask)
					_Files.f_Insert(FullFileName);
				if (Attribs & NFile::EFileAttrib_Directory)
				{
					if (_bRecursive)
					{
						fsg_FindFilesRecursive(_pFS, FullFileName, _Pattern, _AttribMask, _bRecursive, _Files);
					}
				}
			}
		}
	}
	NContainer::TCVector<NStr::CStr> CVirtualFS::f_FindFiles(NStr::CStr const &_Path, uint32 _AttribMask, bint _bRecursive)
	{
		NStr::CStr Pattern = NFile::CFile::fs_GetFile(_Path);
		NStr::CStr Path = NFile::CFile::fs_GetPath(_Path);

		NContainer::TCVector<NStr::CStr> Ret;
		fsg_FindFilesRecursive(this, Path, Pattern, _AttribMask, _bRecursive, Ret);
		return Ret;
	}

	NCryptography::CHashDigest_MD5 CVirtualFS::f_GetFileChecksum(NStr::CStr const &_Path)
	{
		CFile File;
		File.f_Open(*this, _Path, EFileOpen_Read);

		NCryptography::CHash_MD5 Checksum;
		CMibFilePos Length = File.f_GetLength();

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

	void CVirtualFS::f_OpenFile(CFile &_File, NStr::CStr const &_FileName, uint32 _OpenFlags)
	{
		CVirtualFS::CFileInternal *pFile = nullptr;
		if (_OpenFlags & EFileOpen_Write)
		{
			if (_OpenFlags & EFileOpen_DontCreate)
				pFile = fp_OpenFileFromPath(_FileName, false);
			else if (_OpenFlags & EFileOpen_DontOpenExisting)
			{
				pFile = fp_OpenFileFromPath(_FileName, false);
				if (pFile)
				{
					fp_CloseFile(pFile);
					DMibErrorFile("File already exists");
				}
				pFile = fp_OpenFileFromPath(_FileName, true);
			}
			else if (_OpenFlags & EFileOpen_DontTruncate)
			{
				pFile = fp_OpenFileFromPath(_FileName, false);
				if (!pFile)
					pFile = fp_OpenFileFromPath(_FileName, true);
			}
			else
			{
				pFile = fp_OpenFileFromPath(_FileName, false);
				if (pFile)
				{
					if (pFile->m_FileRecord.m_FileAttributes & EFileAttrib_Directory)
					{
						fp_CloseFile(pFile);
						DMibErrorFile("File is a directory and cannot be opened");
					}

					fp_CloseFile(pFile);
					f_DeleteFile(_FileName);
				}
				pFile = fp_OpenFileFromPath(_FileName, true);
			}
		}
		else
			pFile = fp_OpenFileFromPath(_FileName, false);

		if (!pFile)
			DMibErrorFile("File not found or could not be created");

		if (pFile->m_FileRecord.m_FileAttributes & EFileAttrib_Directory)
		{
			fp_CloseFile(pFile);
			DMibErrorFile("File is a directory and cannot be opened");
		}

		if (_OpenFlags & EFileOpen_Write)
		{
			if (pFile->m_FileRecord.m_FileAttributes & EFileAttrib_BackedUp)
			{
				pFile->m_FileRecord.m_FileAttributes &= ~uint64(EFileAttrib_BackedUp);
				pFile->m_bFileRecordDirty = true;
			}
		}

		_File.f_Close();
		_File.mp_pInternalFile = pFile;

	}

	void CVirtualFS::f_DeleteFile(NStr::CStr const &_File)
	{
		fp_CheckPath(_File);
		CVirtualFS::CFileInternal *pFile = fp_OpenFileFromPath(_File, false, true);

		try
		{
			if (!pFile)
				DMibErrorFile("File not found");
			if (pFile->m_RefCount > 1)
				DMibErrorFile("Cannot delete an open file");

			if (pFile->m_FileRecord.m_FileAttributes & EFileAttrib_Directory)
			{
				CDirectoryCacheEntry *pDir = mp_DirectoryCache.f_GetDirectory(_File);
				if (pDir->m_Directory.m_Files.f_GetLen() != 0)
				{
					fp_CloseFile(pFile);
					mp_DirectoryCache.f_ReturnCacheEntry(pDir);
					DMibErrorFile("Cannot delete non-empty directory");
				}
				mp_DirectoryCache.f_ReturnCacheEntry(pDir);
				mp_DirectoryCache.f_RemoveCacheEntry(pDir);
			}

			pFile->f_Delete();
		}
		catch (CExceptionFile)
		{
			if (pFile)
				fp_CloseFile(pFile);
			throw;
		}
		if (pFile)
			fp_CloseFile(pFile);
	}

	void CVirtualFS::fpr_DeleteDirectoryRecursive(NStr::CStr const &_File)
	{
		uint64 Attribs = f_GetAttributes(_File);
		if (Attribs & NFile::EFileAttrib_Directory)
		{
			NContainer::TCVector<NStr::CStr> Files;
			f_EnumFiles(_File, Files);
			for (mint i = 0; i < Files.f_GetLen(); ++i)
			{
				NStr::CStr File = _File + "/" + Files[i];
				fpr_DeleteDirectoryRecursive(File);
			}
		}
		f_DeleteFile(_File);
	}

	void CVirtualFS::f_DeleteDirectoryRecursive(NStr::CStr const &_File)
	{
		fp_CheckPath(_File);
		fpr_DeleteDirectoryRecursive(_File);
	}

	void CVirtualFS::f_RenameFile(NStr::CStr const &_FileFrom, NStr::CStr const &_FileTo)
	{
		fp_CheckPath(_FileFrom);
		fp_CheckPath(_FileTo);

		if (_FileTo.f_Find(_FileFrom) == 0)
			DMibErrorFile("Cannot rename, destination location inside source directory");

		NStr::CStr ParentPath;
		NStr::CStr File;
		aint iPath = _FileTo.f_FindCharReverse('/');
		if (iPath >= 0)
		{
			ParentPath = _FileTo.f_Left(iPath);
			File = _FileTo.f_Extract(iPath + 1);
		}
		else
		{
			File = _FileTo;
		}

		CDirectoryCacheEntry *pDir = mp_DirectoryCache.f_GetDirectory(ParentPath);
		if (pDir)
		{
			CClusterID Cluster = pDir->m_Directory.f_GetChild(File);
			if (!Cluster)
			{
				CVirtualFS::CFileInternal *pFile = fp_OpenFileFromPath(_FileFrom, false, true);

				if (!pFile)
				{
					mp_DirectoryCache.f_ReturnCacheEntry(pDir);
					DMibErrorFile("Cannot rename, source file not found");
				}

				CDirectory::CFile FileToInsert;
				FileToInsert.m_FileName = File;
				FileToInsert.m_FileRecordCluster = pFile->m_FileDescriptorID;

				pFile->m_FileRecord.m_ParentDirectory = pDir->m_Directory.m_DirectoryID;
				pFile->m_bFileRecordDirty = true;

				pDir->m_Directory.m_Files.f_Insert(FileToInsert);
				pDir->m_bDirty = true;

				CDirectoryCacheEntry *pCache = mp_DirectoryCache.m_Tree.f_FindEqual(_FileFrom);
				if (pCache)
				{
					mp_DirectoryCache.m_Tree.f_Remove(pCache);
					pCache->m_FullPath = _FileTo;
					mp_DirectoryCache.m_Tree.f_Insert(pCache);
				}


				fp_CloseFile(pFile);
			}
			else
			{
				mp_DirectoryCache.f_ReturnCacheEntry(pDir);
				DMibErrorFile("Cannot rename, destination name already exists");
			}

			mp_DirectoryCache.f_ReturnCacheEntry(pDir);
		}


	}

	void CVirtualFS::fp_CheckPath(const NStr::CStr &_Path, bint _bCanBeEmpty)
	{
		if (_Path.f_Find("//") >= 0)
			DMibErrorFile("Invalid path: An empty component found ('//').");
		if (_Path.f_IsEmpty())
		{
			if (!_bCanBeEmpty)
				DMibErrorFile("Invalid path: Empty path.");
		}
		else
		{
			if (_Path[0] == '/')
				DMibErrorFile("Invalid path: Path cannot start with '/'.");
			uint32 Pathlen = _Path.f_GetLen();
			if (_Path[Pathlen-1] == '/')
				DMibErrorFile("Invalid path: Path cannot end with '/'.");
		}
	}

	void CVirtualFS::f_CreateDirectory(NStr::CStr const &_Path)
	{
		fp_CheckPath(_Path);

		CDirectoryCacheEntry *pCacheEntry = mp_DirectoryCache.f_GetNewDirectory(_Path);

		if (!pCacheEntry)
		{
			DMibErrorFile("Failed to create directory");
		}
		mp_DirectoryCache.f_ReturnDirectory(pCacheEntry);
	}

	CVirtualFS::CFileInternal *CVirtualFS::fp_OpenFileFromPath(NStr::CStr const &_Path, bint _bCreate, bint _bRemove, bool _bAllowRoot)
	{
		fp_CheckPath(_Path, _bAllowRoot);

		NStr::CStr ParentPath;
		NStr::CStr File;
		aint iPath = _Path.f_FindCharReverse('/');
		if (iPath >= 0)
		{
			ParentPath = _Path.f_Left(iPath);
			File = _Path.f_Extract(iPath + 1);
		}
		else
		{
			File = _Path;
		}

		CDirectoryCacheEntry *pDir = mp_DirectoryCache.f_GetDirectory(ParentPath);
		if (pDir)
		{
			CClusterID Cluster;
			if (_bAllowRoot && ParentPath.f_IsEmpty() && File.f_IsEmpty())
				Cluster = pDir->m_DirectoryFileClusterID;
			else
				Cluster = pDir->m_Directory.f_GetChild(File);

			if (!Cluster)
			{
				if (_bCreate)
				{
					CFileInternal *pRet = fp_OpenNewFile();
					if (!pRet)
					{
						mp_DirectoryCache.f_ReturnCacheEntry(pDir);
						return nullptr;
					}

					CDirectory::CFile FileToInsert;
					FileToInsert.m_FileName = File;
					FileToInsert.m_FileRecordCluster = pRet->m_FileDescriptorID;

					pRet->m_FileRecord.m_ParentDirectory = pDir->m_Directory.m_DirectoryID;
					pRet->m_bFileRecordDirty = true;

					pDir->m_Directory.m_Files.f_Insert(FileToInsert);
					pDir->m_bDirty = true;
					mp_DirectoryCache.f_ReturnCacheEntry(pDir);
					return pRet;
				}
				else
				{
					mp_DirectoryCache.f_ReturnCacheEntry(pDir);
					return nullptr;
				}
			}

			if (_bRemove)
			{
				pDir->m_Directory.f_RemoveChild(Cluster);
				pDir->m_bDirty = true;
			}

			CFileInternal *pRet = fp_OpenFile(Cluster);
			if (!pRet)
			{
				mp_DirectoryCache.f_ReturnCacheEntry(pDir);
				return nullptr;
			}

			mp_DirectoryCache.f_ReturnCacheEntry(pDir);
			return pRet;
		}

		return nullptr;
	}


	void CVirtualFS::f_SetAttributes(NStr::CStr const &_Path, uint64 _Attribs)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		pFile->m_FileRecord.m_FileAttributes = (pFile->m_FileRecord.m_FileAttributes & EFileAttrib_Directory) | (_Attribs & (~EFileAttrib_Directory));
		pFile->m_bFileRecordDirty = true;
		fp_CloseFile(pFile);
	}

	void CVirtualFS::f_ChangeFileAttributes(NStr::CStr const &_Path, uint64 _AttribsAdd, uint64 _AttribsRemove)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		uint64 Attribs = (pFile->m_FileRecord.m_FileAttributes & ~_AttribsRemove) | _AttribsAdd;

		// Set attribs, but don't let user set directory attribute
		pFile->m_FileRecord.m_FileAttributes = (pFile->m_FileRecord.m_FileAttributes & EFileAttrib_Directory) | (Attribs & (~EFileAttrib_Directory));
		pFile->m_bFileRecordDirty = true;
		fp_CloseFile(pFile);
	}

	void CVirtualFS::f_SetCreationTime(NStr::CStr const &_Path, const NTime::CTime &_Time)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		pFile->m_FileRecord.m_CreationTime = _Time;
		pFile->m_bFileRecordDirty = true;
		fp_CloseFile(pFile);
	}

	void CVirtualFS::f_SetAccessTime(NStr::CStr const &_Path, const NTime::CTime &_Time)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		pFile->m_FileRecord.m_AccessTime = _Time;
		pFile->m_bFileRecordDirty = true;
		fp_CloseFile(pFile);
	}

	void CVirtualFS::f_SetWriteTime(NStr::CStr const &_Path, const NTime::CTime &_Time)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		pFile->m_FileRecord.m_WriteTime = _Time;
		pFile->m_bFileRecordDirty = true;
		fp_CloseFile(pFile);
	}

	uint64 CVirtualFS::f_GetAttributes(NStr::CStr const &_Path)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		uint64 Ret = pFile->m_FileRecord.m_FileAttributes;
		fp_CloseFile(pFile);
		return Ret;
	}

	NTime::CTime CVirtualFS::f_GetCreationTime(NStr::CStr const &_Path)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		NTime::CTime Ret = pFile->m_FileRecord.m_CreationTime;
		fp_CloseFile(pFile);
		return Ret;
	}

	NTime::CTime CVirtualFS::f_GetAccessTime(NStr::CStr const &_Path)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		NTime::CTime Ret = pFile->m_FileRecord.m_AccessTime;
		fp_CloseFile(pFile);
		return Ret;
	}

	NTime::CTime CVirtualFS::f_GetWriteTime(NStr::CStr const &_Path)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		NTime::CTime Ret = pFile->m_FileRecord.m_WriteTime;
		fp_CloseFile(pFile);
		return Ret;
	}

	uint64 CVirtualFS::f_GetFileSize(NStr::CStr const &_Path)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		uint64 Ret = pFile->m_FileRecord.m_FileSize;
		fp_CloseFile(pFile);
		return Ret;
	}


	bint CVirtualFS::f_ReadFileToMemory(NStr::CStr const &_File, NContainer::CByteVector &_Data)
	{
		try
		{
			CVirtualFSFile File(*this, _File, EFileOpen_Read);
			mint Len = File.f_GetLength();
			_Data.f_SetLen(Len);
			File.f_Read(_Data.f_GetArray(), Len);
			return true;
		}
		catch (NFile::CExceptionFile)
		{
		}
		return false;
	}

	bint CVirtualFS::f_FileExists(NStr::CStr const &_Path)
	{
		if (_Path.f_IsEmpty())
			return false;
		fp_CheckPath(_Path);

		NStr::CStr ParentPath;
		NStr::CStr File;
		aint iPath = _Path.f_FindCharReverse('/');
		if (iPath >= 0)
		{
			ParentPath = _Path.f_Left(iPath);
			File = _Path.f_Extract(iPath + 1);
		}
		else
		{
			File = _Path;
		}

		CDirectoryCacheEntry *pDir = mp_DirectoryCache.f_GetDirectory(ParentPath);
		if (pDir)
		{
			CClusterID Cluster = pDir->m_Directory.f_GetChild(File);
			if (!Cluster)
			{
				mp_DirectoryCache.f_ReturnDirectory(pDir);
				return false;
			}
			mp_DirectoryCache.f_ReturnDirectory(pDir);
			return true;
		}

		return false;
	}

	bint CVirtualFS::f_FileExists(NStr::CStr const &_Path, NFile::EFileAttrib _AttribMask)
	{
		if (_Path.f_IsEmpty())
			return false;
		fp_CheckPath(_Path);

		NStr::CStr ParentPath;
		NStr::CStr File;
		aint iPath = _Path.f_FindCharReverse('/');
		if (iPath >= 0)
		{
			ParentPath = _Path.f_Left(iPath);
			File = _Path.f_Extract(iPath + 1);
		}
		else
		{
			File = _Path;
		}

		CDirectoryCacheEntry *pDir = mp_DirectoryCache.f_GetDirectory(ParentPath);
		if (pDir)
		{
			CClusterID Cluster = pDir->m_Directory.f_GetChild(File);
			if (!Cluster)
			{
				mp_DirectoryCache.f_ReturnDirectory(pDir);
				return false;
			}
			mp_DirectoryCache.f_ReturnDirectory(pDir);

			uint32 Attribs = f_GetAttributes(_Path);
			if (!(Attribs & NFile::EFileAttrib_Directory))
			{
				if (_AttribMask & NFile::EFileAttrib_File)
					return true;
			}
			if (Attribs & NFile::EFileAttrib_Directory)
			{
				if (_AttribMask & NFile::EFileAttrib_Directory)
					return true;
			}

			return true;
		}

		return false;
	}

	void CVirtualFS::f_CopyFileToFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile)
	{
		NFile::EFileAttrib FileAttribs = NFile::CFile::fs_GetAttributes(_FromFile);

		if (FileAttribs & NFile::EFileAttrib_Link)
		{
			auto LinkDestination = NFile::CFile::fs_ResolveSymbolicLink(_FromFile);
			CFile ToFile(*this, _ToFile, EFileOpen_Write|EFileOpen_ShareAll);
			NContainer::CByteVector Data;
			NFile::CFile::fs_WriteStringToVector(Data, LinkDestination);
			ToFile.f_Write(Data.f_GetArray(), Data.f_GetLen());
		}
		else
		{
			NFile::CFile FromFile;
			CFile ToFile(*this, _ToFile, EFileOpen_Write|EFileOpen_ShareAll);
			FromFile.f_Open(_FromFile, EFileOpen_Read|EFileOpen_ShareAll);

			CMibFilePos ToCopy = FromFile.f_GetLength();
			while (ToCopy)
			{
				mint ThisTime = fg_Min(ToCopy, 16384);
				uint8 Temp[16384];

				FromFile.f_Read(Temp, ThisTime);
				ToFile.f_Write(Temp, ThisTime);
				ToCopy -= ThisTime;
			}
		}

		f_SetAttributes(_ToFile, FileAttribs);
	}

	void CVirtualFS::f_CopyFileFromFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile)
	{
		NFile::CFile ToFile;
		CFile FromFile(*this, _FromFile, EFileOpen_Read|EFileOpen_ShareAll);
		ToFile.f_Open(_ToFile, EFileOpen_Write|EFileOpen_ShareAll);

		CMibFilePos ToCopy = FromFile.f_GetLength();
		while (ToCopy)
		{
			mint ThisTime = fg_Min(ToCopy, 16384);
			uint8 Temp[16384];

			FromFile.f_Read(Temp, ThisTime);
			ToFile.f_Write(Temp, ThisTime);
			ToCopy -= ThisTime;
		}
	}

	void CVirtualFS::f_CopyFileToFS(CVirtualFS &_OtherFS, NStr::CStr const &_FromFile, NStr::CStr const &_ToFile)
	{
		CFile FromFile(_OtherFS, _FromFile, EFileOpen_Read|EFileOpen_ShareAll);
		CFile ToFile(*this, _ToFile, EFileOpen_Write|EFileOpen_ShareAll);

		CMibFilePos ToCopy = FromFile.f_GetLength();
		while (ToCopy)
		{
			mint ThisTime = fg_Min(ToCopy, 16384);
			uint8 Temp[16384];

			FromFile.f_Read(Temp, ThisTime);
			ToFile.f_Write(Temp, ThisTime);
			ToCopy -= ThisTime;
		}
	}

	void CVirtualFS::f_CopyFileInFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile)
	{
		CFile FromFile(*this, _FromFile, EFileOpen_Read|EFileOpen_ShareAll);
		CFile ToFile(*this, _ToFile, EFileOpen_Write|EFileOpen_ShareAll);

		CMibFilePos ToCopy = FromFile.f_GetLength();
		while (ToCopy)
		{
			mint ThisTime = fg_Min(ToCopy, 16384);
			uint8 Temp[16384];

			FromFile.f_Read(Temp, ThisTime);
			ToFile.f_Write(Temp, ThisTime);
			ToCopy -= ThisTime;
		}
	}

	CVirtualFS::CFile::CFile()
	{
		mp_pInternalFile = nullptr;
		mp_Position = 0;
	}

	CVirtualFS::CFile::CFile(CVirtualFS &_FileSystem, NStr::CStr const &_FileName, uint32 _OpenFlags)
	{
		mp_pInternalFile = nullptr;
		mp_Position = 0;
		_FileSystem.f_OpenFile(*this, _FileName, _OpenFlags);
		_FileSystem.m_OpenFiles.f_Insert(this);
	}

	CVirtualFS::CFile::~CFile()
	{
		f_Close();
	}

	void CVirtualFS::CFile::fp_CheckValid() const
	{
		if (!f_IsValid())
		{
			DMibErrorFile("File is not open");
		}
	}

	bint CVirtualFS::CFile::f_IsValid() const
	{
		return mp_pInternalFile != nullptr;
	}

	void CVirtualFS::CFile::f_Close()
	{
		if (mp_pInternalFile)
		{
			mp_pInternalFile->m_pVirtualFS->fp_CloseFile(mp_pInternalFile);
			mp_pInternalFile = nullptr;
		}
		m_Link.f_Unlink();
		mp_Position = 0;
	}

	void CVirtualFS::CFile::f_Open(CVirtualFS &_FileSystem, NStr::CStr const &_FileName, uint32 _OpenFlags)
	{
		f_Close();
		_FileSystem.f_OpenFile(*this, _FileName, _OpenFlags);
		_FileSystem.m_OpenFiles.f_Insert(this);
	}

	void CVirtualFS::CFile::f_Read(void *_pDest, mint _nBytes)
	{
		fp_CheckValid();
		mp_pInternalFile->f_Read(_pDest, mp_Position, _nBytes);
		mp_Position += _nBytes;
	}

	void CVirtualFS::CFile::f_Write(const void *_pSrc, mint _nBytes)
	{
		fp_CheckValid();
		mp_pInternalFile->f_Write(_pSrc, mp_Position, _nBytes);
		mp_Position += _nBytes;
	}

	void CVirtualFS::CFile::f_Flush(bint _bLocalCacheOnly)
	{
		fp_CheckValid();
		mp_pInternalFile->f_Destroy();
	}

	void CVirtualFS::CFile::f_SetCacheSize(mint _CacheSize)
	{

	}

	bint CVirtualFS::CFile::f_IsAtEndOfFile() const
	{
		fp_CheckValid();
		return mp_Position == mp_pInternalFile->m_FileRecord.m_FileSize;
	}

	void CVirtualFS::CFile::f_SetLength(const CMibFilePos &_Length)
	{
		fp_CheckValid();
		mp_pInternalFile->f_SetLength(_Length);
	}

	CMibFilePos CVirtualFS::CFile::f_GetLength() const
	{
		fp_CheckValid();
		return mp_pInternalFile->m_FileRecord.m_FileSize;
	}

	void CVirtualFS::CFile::f_SetPosition(NStream::CFilePos _Pos)
	{
		fp_CheckValid();
//			if (_Pos < 0 || _Pos > mp_pInternalFile->m_FileRecord.m_FileSize)
//				DMibErrorFile("Position is outside of file");

		mp_Position = _Pos;
	}

	void CVirtualFS::CFile::f_SetPositionFromEnd(NStream::CFilePos _Pos)
	{
		fp_CheckValid();
		mp_Position = mp_pInternalFile->m_FileRecord.m_FileSize + _Pos;
	}

	void CVirtualFS::CFile::f_AddPosition(NStream::CFilePos _Pos)
	{
		fp_CheckValid();
		mp_Position += _Pos;
	}

	bint CVirtualFS::CFile::f_IsValidReadPosition(NStream::CFilePos _Pos) const
	{
		return _Pos >= 0 && _Pos < NStream::CFilePos(mp_pInternalFile->m_FileRecord.m_FileSize);
	}

	NStream::CFilePos CVirtualFS::CFile::f_GetPosition() const
	{
		fp_CheckValid();
		return mp_Position;
	}
}
