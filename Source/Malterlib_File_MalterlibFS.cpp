// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Malterlib_File_MalterlibFS.h"

namespace NMib::NFile
{
	struct CVirtualFS_FixFSFileTree
	{
		struct CDirectoryKey
		{
			CVirtualFS::CDirectoryID m_DirectoryID;
			CVirtualFS::CClusterID m_ClusterID;
		};

		struct CFile
		{
			struct CCompareFileClusterID
			{
				inline_small CVirtualFS::CClusterID const &operator () (CFile const &_Node) const;
			};

			struct CCompareDirectoryID
			{
				inline_small COrdering_Partial operator () (CFile const &_Left, CFile const &_Right) const;

				inline_small COrdering_Partial operator () (CFile const &_Left, CDirectoryKey const &_Right) const;

				inline_small COrdering_Partial operator () (CDirectoryKey const &_Left, CFile const &_Right) const;
			};

			CFile(CVirtualFS_FixFSFileTree *_pTree);

			~CFile();

			uint32 f_GetDepth();

			void f_SetParent(CFile *_pParent);

			CVirtualFS::CDirectory::CFile *f_GetDirectoryEntry(CVirtualFS::CDirectoryID _Cluster);

			CFile *f_GetFileByName(NStr::CStr const &_Name);

			CVirtualFS_FixFSFileTree *m_pTree;
			bool m_bIsDir;
			NStr::CMStrPreserve m_FileName;
			CVirtualFS::CDirectoryID m_DirectoryID;
			CVirtualFS::CDirectoryID m_ParentID;
			CVirtualFS::CClusterID m_FileClusterID;
			CMibFilePos m_FileSize;
			CFile *m_pParent;

			CVirtualFS::CDirectory m_Directory;

			NIntrusive::TCAVLLink<> m_FileClusterIDLink;

			DMibListLinkDS_Link(CFile, m_ChildLink);
			DMibListLinkDS_List(CFile, m_ChildLink) m_Children;

			NIntrusive::TCAVLLink<> m_DirectoryIDLink;
			DMibListLinkDS_Link(CFile, m_RootLink);
		};

		CVirtualFS_FixFSFileTree(CVirtualFS::CCheckReporter *_pReporter, uint32 _Version);

		void f_FindRoots();

		void f_TraceTreeRecursive(CFile *_pFile, int _Depth, CVirtualFS::CCheckReporter *_pReporter);

		void f_TraceTree(CVirtualFS::CCheckReporter *_pReporter);

		void f_CopyFile(CVirtualFS::CFileRecord &_FileRecord, CVirtualFS::CFile &_File, uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream);
		void f_CopyTreeRecursive
			(
				NStr::CStr const &_Path
				, CFile *_pFile
				, uint64 _ClusterSize
				, uint64 _ClusterIDsPerCluster
				, NStream::CBinaryStream *_pStorageStream
				, CVirtualFS *_pFixDestinationFS
			)
		;
		void f_CopyTree(uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream, CVirtualFS *_pFixDestinationFS);

		CFile *f_GetFile(CVirtualFS::CClusterID _Cluster);

		CFile *f_GetDirectory(CVirtualFS::CDirectoryID _DirID);

		CFile *f_GetDirectory(CVirtualFS::CClusterID _Cluster, CVirtualFS::CDirectoryID _DirID);

		void f_Destroy();

		DMibListLinkDS_List(CFile, m_RootLink) m_Roots;

		NIntrusive::TCAVLTree<&CFile::m_FileClusterIDLink, CFile::CCompareFileClusterID> m_FileClusters;
		NIntrusive::TCAVLTree<&CFile::m_DirectoryIDLink, CFile::CCompareDirectoryID> m_Directories;

	private:
		CVirtualFS::CCheckReporter *mp_pReporter;
		uint32 mp_Version;
	};

	CVirtualFS::CClusterID const &CVirtualFS_FixFSFileTree::CFile::CCompareFileClusterID::operator () (CFile const &_Node) const
	{
		return _Node.m_FileClusterID;
	}

	COrdering_Partial CVirtualFS_FixFSFileTree::CFile::CCompareDirectoryID::operator () (CFile const &_Left, CFile const &_Right) const
	{
		if (auto Result = _Left.m_DirectoryID <=> _Right.m_DirectoryID; Result != 0)
			return Result;
		return _Left.m_FileClusterID <=> _Right.m_FileClusterID;
	}

	COrdering_Partial CVirtualFS_FixFSFileTree::CFile::CCompareDirectoryID::operator () (CFile const &_Left, CDirectoryKey const &_Right) const
	{
		if (auto Result = _Left.m_DirectoryID <=> _Right.m_DirectoryID; Result != 0)
			return Result;
		return _Left.m_FileClusterID <=> _Right.m_ClusterID;
	}

	COrdering_Partial CVirtualFS_FixFSFileTree::CFile::CCompareDirectoryID::operator () (CDirectoryKey const &_Left, CFile const &_Right) const
	{
		if (auto Result = _Left.m_DirectoryID <=> _Right.m_DirectoryID; Result != 0)
			return Result;
		return _Left.m_ClusterID <=> _Right.m_FileClusterID;
	}

	CVirtualFS_FixFSFileTree::CFile::CFile(CVirtualFS_FixFSFileTree *_pTree)
	{
		m_DirectoryID = 0;
		m_ParentID = 0;
		m_FileClusterID = 0;
		m_bIsDir = false;
		m_FileSize = 0;
		m_pParent = nullptr;
		m_pTree = _pTree;
	}

	CVirtualFS_FixFSFileTree::CFile::~CFile()
	{
		f_SetParent(nullptr);
		m_Children.f_DeleteAllDefiniteType();
		if (m_FileClusterIDLink.f_IsInTree())
			m_pTree->m_FileClusters.f_Remove(this);
		if (m_DirectoryIDLink.f_IsInTree())
			m_pTree->m_Directories.f_Remove(this);
	}

	uint32 CVirtualFS_FixFSFileTree::CFile::f_GetDepth()
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

	void CVirtualFS_FixFSFileTree::CFile::f_SetParent(CFile *_pParent)
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

	CVirtualFS::CDirectory::CFile *CVirtualFS_FixFSFileTree::CFile::f_GetDirectoryEntry(CVirtualFS::CDirectoryID _Cluster)
	{
		umint nFiles = m_Directory.m_Files.f_GetLen();
		for (umint iFile = 0; iFile < nFiles; ++iFile)
		{
			if (m_Directory.m_Files[iFile].m_FileRecordCluster == _Cluster)
				return &m_Directory.m_Files[iFile];
		}
		return nullptr;
	}

	CVirtualFS_FixFSFileTree::CFile *CVirtualFS_FixFSFileTree::CFile::f_GetFileByName(NStr::CStr const &_Name)
	{
		DMibListLinkDS_Iter(CFile, m_ChildLink) iChild = m_Children;

		while (iChild)
		{
			if (iChild->m_FileName == _Name)
				return iChild;

			++iChild;
		}
		return nullptr;
	}

	CVirtualFS_FixFSFileTree::CVirtualFS_FixFSFileTree(CVirtualFS::CCheckReporter *_pReporter, uint32 _Version)
	{
		mp_pReporter = _pReporter;
		mp_Version = _Version;
	}

	void CVirtualFS_FixFSFileTree::f_TraceTreeRecursive(CFile *_pFile, int _Depth, CVirtualFS::CCheckReporter *_pReporter)
	{
		DMibListLinkDS_Iter(CFile, m_ChildLink) iChild = _pFile->m_Children;

		while (iChild)
		{
			NStr::CStr ToTrace
				= NStr::CStr::CFormat("{sj*6}{}: Dir: {} Cluster: {}    Parent: {}     FileSize: {}")
				<< ""
				<< NStr::CStr(iChild->m_FileName)
				<< iChild->m_DirectoryID
				<< iChild->m_FileClusterID
				<< iChild->m_ParentID
				<< iChild->m_FileSize
				<< _Depth * 3
			;
			if (1)
				DMibDTrace("{}" DMibNewLine, ToTrace);
			else
				_pReporter->f_ReportError(ToTrace);
			f_TraceTreeRecursive(iChild, _Depth + 1, _pReporter);
			++iChild;
		}
	}

	void CVirtualFS_FixFSFileTree::f_TraceTree(CVirtualFS::CCheckReporter *_pReporter)
	{
		DMibListLinkDS_Iter(CFile, m_RootLink) iRoot = m_Roots;

		while (iRoot)
		{
			NStr::CStr ToTrace
				= NStr::CStr::CFormat("Root Dir: {} Cluster: {}    Parent: {}    FileSize: {}") << iRoot->m_DirectoryID << iRoot->m_FileClusterID << iRoot->m_ParentID << iRoot->m_FileSize
			;
			if (1)
				DMibDTrace("{}" DMibNewLine, ToTrace);
			else
				_pReporter->f_ReportError(ToTrace);

			f_TraceTreeRecursive(iRoot, 1, _pReporter);

			++iRoot;
		}
	}

	CVirtualFS_FixFSFileTree::CFile *CVirtualFS_FixFSFileTree::f_GetFile(CVirtualFS::CClusterID _Cluster)
	{
		CVirtualFS_FixFSFileTree::CFile *pFile = m_FileClusters.f_FindEqual(_Cluster);

		if (!pFile)
		{
			pFile = fg_ConstructObject<CVirtualFS_FixFSFileTree::CFile>(NMemory::CDefaultAllocator(), this);
			pFile->m_FileClusterID = _Cluster;
			m_FileClusters.f_Insert(pFile);
		}
		return pFile;
	}

	CVirtualFS_FixFSFileTree::CFile *CVirtualFS_FixFSFileTree::f_GetDirectory(CVirtualFS::CDirectoryID _DirID)
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
			pFile = fg_ConstructObject<CVirtualFS_FixFSFileTree::CFile>(NMemory::CDefaultAllocator(), this);
			pFile->m_DirectoryID = _DirID;
			m_Directories.f_Insert(pFile);
		}
		return pFile;
	}

	CVirtualFS_FixFSFileTree::CFile *CVirtualFS_FixFSFileTree::f_GetDirectory(CVirtualFS::CClusterID _Cluster, CVirtualFS::CDirectoryID _DirID)
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
				fg_DeleteObject(NMemory::CDefaultAllocator(), pFileDir);

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

		CVirtualFS_FixFSFileTree::CFile *pFile = fg_ConstructObject<CVirtualFS_FixFSFileTree::CFile>(NMemory::CDefaultAllocator(), this);
		pFile->m_DirectoryID = _DirID;
		pFile->m_FileClusterID = _Cluster;
		pFile->m_bIsDir = true;
		m_FileClusters.f_Insert(pFile);
		m_Directories.f_Insert(pFile);
		return pFile;
	}

	void CVirtualFS_FixFSFileTree::f_Destroy()
	{
		m_Roots.f_Clear();

		m_FileClusters.f_DeleteAllDefiniteType();
		m_Directories.f_DeleteAllDefiniteType();
	}

	CVirtualFS::CRootData::CRootData()
	{
		m_ClusterSize = 2048; // Cluster size minimum cluster size is 128
		m_RootFileCluster = 0; // Cluster at which the root directory resides
		m_FirstFreeCluster = 0; // First free cluster
		m_FileSystemGrowSize = 512 * 1024; // We grow the master file with this much
		m_nFreeClusters = 0; // Max Cluster Size
		m_DirectoryCacheSize = 128; // How many directories to cache
		m_ClusterCacheSize = 128; // How many clusters to cache
		m_CurrentDirectoryID = 0;
		m_Version = 0x103;
	}

	void CVirtualFS::CRootData::f_WriteInternal(NStream::CBinaryStream &_Stream)
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

	void CVirtualFS::CRootData::f_Write(NStream::CBinaryStream &_Stream)
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

	void CVirtualFS::CRootData::f_Read(NStream::CBinaryStream &_Stream)
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

	void CVirtualFS::CRootData::f_GenerateChecksum()
	{
		NCryptography::CHash_MD5 Checksum;
		NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
		Stream.f_Open(Checksum);

		Stream << (uint32)0xa042132f;
		f_WriteInternal(Stream);

		m_MD5Checksum = Checksum;
	}

	bool CVirtualFS::CRootData::f_Validate()
	{
		NCryptography::CHash_MD5 Checksum;
		NCryptography::TCBinaryStreamHashRef<NCryptography::CHash_MD5> Stream;
		Stream.f_Open(Checksum);

		Stream << (uint32)0xa042132f;
		f_WriteInternal(Stream);

		return m_MD5Checksum == Checksum;
	}

	void CVirtualFS::fp_SetStreamCluster(CClusterID _Cluster)
	{
		mp_pStorageStream->f_SetPosition(fp_GetFilePos(_Cluster));
	}

	void CVirtualFS::CFileRecord::f_Reset()
	{
		m_WriteTime = m_AccessTime = m_CreationTime = NTime::CTime::fs_NowUTC();
		m_FileSize = 0;
		m_FileAttributes = 0;
		m_FirstFileChainCluster = 0;
		m_ParentDirectory = 0;
	}

	void CVirtualFS::CFileRecord::f_GenerateChecksum(uint64 _ClusterID, uint32 _Version)
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

	bool CVirtualFS::CFileRecord::f_Validate(uint64 _ClusterID, uint32 _Version)
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

	void CVirtualFS::CFileRecord::f_WriteInternal(NStream::CBinaryStream &_Stream)
	{
		_Stream << m_CreationTime;
		_Stream << m_AccessTime;
		_Stream << m_WriteTime;
		_Stream << m_FileSize;
		_Stream << m_FileAttributes;
		_Stream << m_FirstFileChainCluster;
		_Stream << m_ParentDirectory;
	}

	void CVirtualFS::CFileRecord::f_Write(NStream::CBinaryStream &_Stream)
	{
		_Stream << m_MD5Checksum;
		f_WriteInternal(_Stream);
	}

	void CVirtualFS::CFileRecord::f_Read(NStream::CBinaryStream &_Stream)
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

	void CVirtualFS::CClusterChain::f_GenerateChecksum(uint64 _ClusterID, uint32 _Version)
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

	bool CVirtualFS::CClusterChain::f_Validate(uint64 _ClusterID, uint32 _Version)
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

	void CVirtualFS::CClusterChain::f_WriteInternal(NStream::CBinaryStream &_Stream)
	{
		_Stream << m_NextChainCluster;
		_Stream << m_ClusterFirstIDIndex;
		_Stream << m_nClusterIDs;
		if (m_nClusterIDs > m_ClusterIDs.f_GetLen())
			return;
		for (umint iClusterID = 0; iClusterID < m_nClusterIDs; ++iClusterID)
			_Stream << m_ClusterIDs[iClusterID];
	}

	void CVirtualFS::CClusterChain::f_Write(NStream::CBinaryStream &_Stream)
	{
		_Stream << m_MD5Checksum;
		f_WriteInternal(_Stream);
	}

	void CVirtualFS::CClusterChain::f_Read(NStream::CBinaryStream &_Stream)
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

			for (umint iClusterID = 0; iClusterID < m_nClusterIDs; ++iClusterID)
				_Stream >> m_ClusterIDs[iClusterID];
		}
		catch (NException::CException)
		{
			m_MD5Checksum = NCryptography::CHashDigest_MD5();
		}
	}

	void CVirtualFS::CDirectory::CFile::f_FeedPreserve(NStream::CBinaryStream &_Stream) const
	{
		_Stream << m_FileRecordCluster;
		_Stream << m_FileName;
	}

	void CVirtualFS::CDirectory::CFile::f_FeedNormal(NStream::CBinaryStream &_Stream) const
	{
		_Stream << m_FileRecordCluster;
		NStr::CMStrPreserve ToSave(m_FileName);
		ToSave.m_MixedType = NStr::EMStrType_CStr;
		ToSave.m_OriginalType = NStr::EStrType_UTF;
		_Stream << ToSave;
	}

	void CVirtualFS::CDirectory::CFile::f_Consume(NStream::CBinaryStream &_Stream)
	{
		_Stream >> m_FileRecordCluster;
		_Stream >> m_FileName;
	}

	void CVirtualFS::CDirectory::f_GenerateChecksum(uint64 _ClusterID, uint32 _Version)
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

	void CVirtualFS::CDirectory::f_AddChild(NStr::CStr const &_Path, CClusterID _ClusterID)
	{
		CFile File;
		File.m_FileRecordCluster = _ClusterID;
		File.m_FileName = _Path;
		m_Files.f_Insert(File);
	}

	bool CVirtualFS::CDirectory::f_Validate(uint64 _ClusterID, uint32 _Version)
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
			for (umint iFile = 0; iFile < nFiles; ++iFile)
				m_Files[iFile].m_FileName.m_OriginalType = NStr::EStrType_Ansi;
			f_WriteInternalValidate(Stream);
			for (umint iFile = 0; iFile < nFiles; ++iFile)
				m_Files[iFile].m_FileName.m_OriginalType = NStr::EStrType_Unicode;
		}
		else
			f_WriteInternalValidate(Stream);

		return m_MD5Checksum == Checksum;
	}

	void CVirtualFS::CDirectory::f_WriteInternalValidate(NStream::CBinaryStream &_Stream)
	{
		_Stream << m_DirectoryID;
		uint32 nFiles = m_Files.f_GetLen();
		_Stream << nFiles;
		for (auto &File : m_Files)
			File.f_FeedPreserve(_Stream);
	}

	void CVirtualFS::CDirectory::f_WriteInternal(NStream::CBinaryStream &_Stream)
	{
		_Stream << m_DirectoryID;
		uint32 nFiles = m_Files.f_GetLen();
		_Stream << nFiles;
		for (auto &File : m_Files)
			File.f_FeedNormal(_Stream);
	}

	void CVirtualFS::CDirectory::f_Write(NStream::CBinaryStream &_Stream)
	{
		_Stream << m_MD5Checksum;
		f_WriteInternal(_Stream);
	}

	void CVirtualFS::CDirectory::f_Read(NStream::CBinaryStream &_Stream)
	{
		try
		{
			_Stream >> m_MD5Checksum;
			_Stream >> m_DirectoryID;

			uint32 nFiles;
			_Stream >> nFiles;
			umint MaxFiles = ((_Stream.f_GetLength() - _Stream.f_GetPosition()) / (sizeof(CClusterID) + sizeof(uint32)));
			if (nFiles > MaxFiles)
			{
				m_Files.f_Clear();
				m_MD5Checksum = NCryptography::CHashDigest_MD5();
				return;
			}
			m_Files.f_SetLen(nFiles);
			for (umint iFile = 0; iFile < nFiles; ++iFile)
				_Stream >> m_Files[iFile];
		}
		catch (NException::CException)
		{
			m_Files.f_Clear();
			m_MD5Checksum = NCryptography::CHashDigest_MD5();
		}
	}

	CVirtualFS::CClusterID CVirtualFS::CDirectory::f_GetChild(NStr::CStr const &_Child)
	{
		for (umint iFile = 0; iFile < m_Files.f_GetLen(); ++iFile)
		{
			if (m_Files[iFile].m_FileName == _Child)
				return m_Files[iFile].m_FileRecordCluster;
		}

		return 0;
	}

	void CVirtualFS::CDirectory::f_RemoveChild(CClusterID _Child)
	{
		for (umint iFile = 0; iFile < m_Files.f_GetLen(); ++iFile)
		{
			if (m_Files[iFile].m_FileRecordCluster == _Child)
			{
				m_Files.f_Remove(iFile);
				return;
			}
		}
	}

	void CVirtualFS::CFreeCluster::f_WriteInternal(NStream::CBinaryStream &_Stream)
	{
		_Stream << m_NextFreeCluster;
	}

	void CVirtualFS::CFreeCluster::f_Write(NStream::CBinaryStream &_Stream)
	{
		_Stream << m_MD5Checksum;
		f_WriteInternal(_Stream);
	}

	void CVirtualFS::CFreeCluster::f_Read(NStream::CBinaryStream &_Stream)
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

	void CVirtualFS::CFreeCluster::f_GenerateChecksum(uint64 _ClusterID, uint32 _Version)
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

	bool CVirtualFS::CFreeCluster::f_Validate(uint64 _ClusterID, uint32 _Version)
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

	CVirtualFS::CClusterCacheEntry::CClusterCacheEntry()
	{
		m_bDirty = false;
		m_RefCount = 0;
	}

	void CVirtualFS::CClusterCacheEntry::f_MakeDirty()
	{
		m_bDirty = true;
	}

	void CVirtualFS::CClusterCacheEntry::f_OpenStream(NStream::TCBinaryStreamMemoryPtr<> &_Stream)
	{
		umint Len = m_ClusterMemory.f_GetLen();
		_Stream.f_OpenReadWrite(m_ClusterMemory.f_GetArray(), Len, Len);
	}

	void CVirtualFS::CClusterCacheEntry::f_CloseStream(bool _bWritten)
	{
		if (_bWritten)
			f_MakeDirty();
	}

	CVirtualFS::CClusterCache::CClusterCache()
	{
		m_nCached = 0;
		m_nCachedMax = 0;
	}

	void CVirtualFS::CClusterCache::f_Create(CVirtualFS *_pVirtualFS)
	{
		m_pVirtualFS = _pVirtualFS;
	}

	CVirtualFS::CClusterCache::~CClusterCache()
	{
		f_Destroy();
	}

	void CVirtualFS::CClusterCache::f_Destroy()
	{
		DMibSafeCheck(m_InUse.f_IsEmpty(), "This should not happed. Did you close all files?");

		f_Flush();
		m_Tree.f_DeleteAllDefiniteType();
	}

	void CVirtualFS::CClusterCache::f_Flush(CClusterCacheEntry *_pCluster)
	{
		if (_pCluster->m_bDirty)
		{
			m_pVirtualFS->fp_SetStreamCluster(_pCluster->m_ClusterID);
			m_pVirtualFS->mp_pStorageStream->f_FeedBytes(_pCluster->m_ClusterMemory.f_GetArray(), _pCluster->m_ClusterMemory.f_GetLen());
			m_pVirtualFS->mp_pStorageStream->f_Flush(true);
			_pCluster->m_bDirty = false;
		}
	}

	void CVirtualFS::CClusterCache::f_Flush()
	{
		auto iCluster = m_InUse.f_GetIterator();

		while (iCluster)
		{
			if (iCluster->m_bDirty)
				f_Flush(iCluster);

			++iCluster;
		}
	}

	void CVirtualFS::CClusterCache::f_SetCacheSize(umint _nCachedMax)
	{
		m_nCachedMax = _nCachedMax;
		f_MakeSpace(0);
	}

	void CVirtualFS::CClusterCache::f_RemoveCacheEntry(CClusterCacheEntry *_pEntry)
	{
		m_Tree.f_Remove(_pEntry);
		--m_nCached;
		fg_DeleteObject(NMemory::CDefaultAllocator(), _pEntry);
	}

	void CVirtualFS::CClusterCache::f_MakeSpace(umint _Space)
	{
		while ((m_nCached + _Space) > m_nCachedMax)
		{
			CClusterCacheEntry *pEntry = m_Cached.f_GetLast();

			if (!pEntry)
				return; // nothing we can do no more unused cache entries to delete

			f_RemoveCacheEntry(pEntry);
		}
	}

	CVirtualFS::CClusterCacheEntry *CVirtualFS::CClusterCache::f_GetCluster(CClusterID _Cluster)
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

	void CVirtualFS::CClusterCache::f_ReturnCluster(CClusterCacheEntry *_pCluster)
	{
		if (_pCluster->m_RefCount == 1)
		{
			if (_pCluster->m_bDirty)
				f_Flush(_pCluster);

			--_pCluster->m_RefCount;
			m_Cached.f_InsertFirst(_pCluster);
		}
	}

	CVirtualFS::CDirectoryCacheEntry::CDirectoryCacheEntry()
	{
		m_bDirty = false;
		m_RefCount = 0;
	}

	CVirtualFS::CDirectoryCache::CDirectoryCache()
	{
		m_nCached = 0;
		m_nCachedMax = 0;
	}

	CVirtualFS::CDirectoryCache::~CDirectoryCache()
	{
		f_Destroy();
	}

	void CVirtualFS::CDirectoryCache::f_Destroy()
	{
		DMibSafeCheck(m_InUse.f_IsEmpty(), "This should not happed. Did you close all files?");
		f_Flush();
		m_Tree.f_DeleteAllDefiniteType();
	}

	void CVirtualFS::CDirectoryCache::f_Flush()
	{
		auto iCluster = m_InUse.f_GetIterator();

		while (iCluster)
		{
			if (iCluster->m_bDirty)
				f_Flush(iCluster);
			++iCluster;
		}
	}

	void CVirtualFS::CDirectoryCache::f_Flush(CDirectoryCacheEntry *_pEntry)
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

	void CVirtualFS::CDirectoryCache::f_Create(CVirtualFS *_pVirtualFS)
	{
		m_pVirtualFS = _pVirtualFS;
	}

	CVirtualFS::CDirectoryCacheEntry *CVirtualFS::CDirectoryCache::f_GetCacheEntry(CDirectoryCacheEntry *_pParentEntry, NStr::CStr const &_CurrentPath)
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

	void CVirtualFS::CDirectoryCache::f_ReturnCacheEntry(CDirectoryCacheEntry *_pEntry)
	{
		if ((--_pEntry->m_RefCount) == 0)
		{
			if (_pEntry->m_bDirty)
				f_Flush(_pEntry);

			m_Cached.f_InsertFirst(_pEntry);
		}
	}

	CVirtualFS::CDirectoryCacheEntry *CVirtualFS::CDirectoryCache::f_GetDirectoryCommon(NStr::CStr const &_Path)
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
					CurrentPath = _Path.f_Extract(Find.f_GetLen() + 1);
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

	CVirtualFS::CDirectoryCacheEntry *CVirtualFS::CDirectoryCache::f_GetDirectory(NStr::CStr const &_Path)
	{
		CDirectoryCacheEntry *pCache = f_GetDirectoryCommon(_Path);

		if (pCache->m_FullPath != _Path)
		{
			f_ReturnCacheEntry(pCache);
			return nullptr;
		}
		return pCache;
	}

	CVirtualFS::CDirectoryCacheEntry *CVirtualFS::CDirectoryCache::f_GetNewDirectory(NStr::CStr const &_Path)
	{
		CDirectoryCacheEntry *pCache = f_GetDirectoryCommon(_Path);

		if (pCache->m_FullPath.f_GetLen() < _Path.f_GetLen())
		{
			NStr::CStr CurrentPath;
			if (pCache->m_FullPath != "")
				CurrentPath = _Path.f_Extract(pCache->m_FullPath.f_GetLen() + 1);
			else
				CurrentPath = _Path;

			while (pCache && !CurrentPath.f_IsEmpty())
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

	void CVirtualFS::CDirectoryCache::f_ReturnDirectory(CDirectoryCacheEntry *_pEntry)
	{
		f_ReturnCacheEntry(_pEntry);
	}

	void CVirtualFS::CDirectoryCache::f_RemoveCacheEntry(CDirectoryCacheEntry *_pEntry)
	{
		DMibSafeCheck(!_pEntry->m_bDirty, "Should not be dirty");
		DMibSafeCheck(_pEntry->m_RefCount == 0, "Should not be dirty");

		m_Tree.f_Remove(_pEntry);
		--m_nCached;
		fg_DeleteObject(NMemory::CDefaultAllocator(), _pEntry);
	}

	void CVirtualFS::CDirectoryCache::f_MakeSpace(umint _Space)
	{
		while ((m_nCached + _Space) > m_nCachedMax)
		{
			CDirectoryCacheEntry *pEntry = m_Cached.f_GetLast();

			if (!pEntry)
				return; // nothing we can do no more unused cache entries to delete

			f_RemoveCacheEntry(pEntry);
		}
	}

	void CVirtualFS::CDirectoryCache::f_SetCacheSize(umint _Size)
	{
		m_nCachedMax = _Size;
		f_MakeSpace(0);
	}

	CVirtualFS::CFileInternal::CFileInternal()
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

	CVirtualFS::CFileInternal::~CFileInternal() noexcept(false)
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

	CVirtualFS::CFileInternal::CClusterCache::CClusterCache(CClusterCacheEntry *_pClusterCacheEntry, CFilePos _FilePosStart)
	{
		m_pClusterCacheEntry = _pClusterCacheEntry;
		m_FilePosStart = _FilePosStart;
	}

	CVirtualFS::CClusterCacheEntry *CVirtualFS::CFileInternal::f_GetCluster(CFilePos &_Position)
	{
		uint64 ClusterSize = m_pVirtualFS->mp_RootData.m_ClusterSize;
		DMibListLinkDS_Iter(CClusterCache, m_Link) iCachedCluster = m_CachedClusters;

		while (iCachedCluster)
		{
			CClusterCache *pEntry = iCachedCluster;
			uint64 End = pEntry->m_FilePosStart + ClusterSize;
			if (_Position >= pEntry->m_FilePosStart && _Position < End)
			{
				m_CachedClusters.f_InsertFirst(pEntry);
				_Position -= pEntry->m_FilePosStart;
				return pEntry->m_pClusterCacheEntry;
			}
			++iCachedCluster;
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

	void CVirtualFS::CFileInternal::f_Read(void *_pDest, CFilePos _Position, umint _nBytes)
	{
		CFilePos CurrentPosition = _Position;
		uint8 *pDest = (uint8 *)_pDest;

		uint64 ClusterSize = m_pVirtualFS->mp_RootData.m_ClusterSize;

		if (_Position + _nBytes > m_FileRecord.m_FileSize)
			DMibErrorFile("Read would read past end of file");

		while (_nBytes)
		{
			CFilePos Pos = CurrentPosition;
			CClusterCacheEntry *pEntry = f_GetCluster(Pos);
			umint MaxBytes = fg_Min(ClusterSize - Pos, _nBytes);
			NMemory::fg_MemCopy(pDest, pEntry->m_ClusterMemory.f_GetArray() + Pos, MaxBytes);
			pDest += MaxBytes;
			_nBytes -= MaxBytes;
			CurrentPosition += MaxBytes;
		}
	}

	void CVirtualFS::CFileInternal::f_Write(void const *_pSource, CFilePos _Position, umint _nBytes)
	{
		CFilePos CurrentPosition = _Position;
		uint8 *pSrc = (uint8 *)_pSource;

		uint64 ClusterSize = m_pVirtualFS->mp_RootData.m_ClusterSize;

		while (_nBytes)
		{
			CFilePos Pos = CurrentPosition;
			CClusterCacheEntry *pEntry = f_GetCluster(Pos);
			umint MaxBytes = fg_Min(ClusterSize - Pos, _nBytes);
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

	void CVirtualFS::CFileInternal::f_WriteClusterChain()
	{
		if (!m_bClusterChainDirty)
			return;
		m_bClusterChainDirty = false;

		m_ClusterChain.f_GenerateChecksum(m_ClusterChainCluster, m_pVirtualFS->mp_RootData.m_Version);
		CClusterCacheEntry *pCluster = m_pVirtualFS->mp_ClusterCache.f_GetCluster(m_ClusterChainCluster);
		NStream::TCBinaryStreamMemoryPtr<> Stream;
		pCluster->f_OpenStream(Stream);
		m_ClusterChain.f_Write(Stream);
		pCluster->f_CloseStream(true);
		m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster);
	}

	void CVirtualFS::CFileInternal::f_Delete()
	{
		f_Destroy();

		if (m_FileRecord.m_FirstFileChainCluster)
		{
			f_ReadClusterChain(m_FileRecord.m_FirstFileChainCluster);
			m_pVirtualFS->fp_ReturnCluster(m_FileRecord.m_FirstFileChainCluster);
			while (1)
			{
				for (uint64 iClusterID = 0; iClusterID < m_ClusterChain.m_nClusterIDs; ++iClusterID)
					m_pVirtualFS->fp_ReturnCluster(m_ClusterChain.m_ClusterIDs[iClusterID]);
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

	void CVirtualFS::CFileInternal::f_ReadClusterChain(CClusterID _ClusterID)
	{
		if (m_bClusterChainDirty)
			f_Flush();

		m_ClusterChainCluster = _ClusterID;
		CClusterCacheEntry *pCluster = m_pVirtualFS->mp_ClusterCache.f_GetCluster(_ClusterID);
		NStream::TCBinaryStreamMemoryPtr<> Stream;
		pCluster->f_OpenStream(Stream);
		m_ClusterChain.f_Read(Stream);
		pCluster->f_CloseStream(false);
		m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster);
		m_bValidClusterChain = true;

		if (!m_ClusterChain.f_Validate(_ClusterID, m_pVirtualFS->mp_RootData.m_Version))
			DMibErrorFile("Cluster chain record checksum failure");
	}

	CVirtualFS::CClusterID CVirtualFS::CFileInternal::f_GetDataCluster(CFilePos &_FilePosition)
	{
		CClusterID ClusterIndex = m_pVirtualFS->fp_GetClusterID(_FilePosition, _FilePosition);

		uint64 ClusterIDsPerCluster = m_pVirtualFS->mp_ClusterIDsPerCluster;
		// CClusterID ClusterDataLocation = ClusterIndex / ClusterIDsPerCluster;
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
				for (uint64 iClusterID = m_ClusterChain.m_nClusterIDs; iClusterID < ClusterIDsPerCluster; ++iClusterID)
					m_ClusterChain.m_ClusterIDs[iClusterID] = m_pVirtualFS->fp_GetNewCluster();
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
			for (uint64 iClusterID = m_ClusterChain.m_nClusterIDs; iClusterID <= IndexInChunk; ++iClusterID)
				m_ClusterChain.m_ClusterIDs[iClusterID] = m_pVirtualFS->fp_GetNewCluster();
			m_ClusterChain.m_nClusterIDs = IndexInChunk + 1;
			m_bClusterChainDirty = true;
		}

		LastIndex = m_ClusterChain.m_ClusterFirstIDIndex + m_ClusterChain.m_nClusterIDs;
		if (!(ClusterIndex >= m_ClusterChain.m_ClusterFirstIDIndex && ClusterIndex < LastIndex))
			DMibErrorFile("Cluster chain error");

		f_Flush();

		return m_ClusterChain.m_ClusterIDs[IndexInChunk];
	}

	void CVirtualFS::CFileInternal::f_Create(CVirtualFS *_pVirtualFS, CClusterID _FileDescriptorID, bool _bLoad)
	{
		m_FileDescriptorID = _FileDescriptorID;
		m_pVirtualFS = _pVirtualFS;

		m_ClusterChain.m_ClusterIDs.f_SetLen(m_pVirtualFS->mp_ClusterIDsPerCluster);

		if (_bLoad)
		{
			CClusterCacheEntry *pCluster = m_pVirtualFS->mp_ClusterCache.f_GetCluster(m_FileDescriptorID);
			NStream::TCBinaryStreamMemoryPtr<> Stream;
			pCluster->f_OpenStream(Stream);
			m_FileRecord.f_Read(Stream);
			pCluster->f_CloseStream(false);
			m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster);

			if (!m_FileRecord.f_Validate(_FileDescriptorID, m_pVirtualFS->mp_RootData.m_Version))
				DMibErrorFile("File record checksum failure");
		}
	}

	void CVirtualFS::CFileInternal::f_WriteFileRecord()
	{
		if (m_bFileRecordDirty)
		{
			m_bFileRecordDirty = false;
			m_FileRecord.f_GenerateChecksum(m_FileDescriptorID, m_pVirtualFS->mp_RootData.m_Version);
			CClusterCacheEntry *pCluster = m_pVirtualFS->mp_ClusterCache.f_GetCluster(m_FileDescriptorID);
			NStream::TCBinaryStreamMemoryPtr<> Stream;
			pCluster->f_OpenStream(Stream);
			m_FileRecord.f_Write(Stream);
			pCluster->f_CloseStream(true);
			m_pVirtualFS->mp_ClusterCache.f_ReturnCluster(pCluster);
		}
	}

	void CVirtualFS::CFileInternal::f_Destroy()
	{
		f_Flush();
		m_bValidClusterChain = false;
	}

	void CVirtualFS::CFileInternal::f_Flush()
	{
		f_WriteClusterChain();
		f_WriteFileRecord();
	}

	CVirtualFS::CBinaryStreamFileInternal::CBinaryStreamFileInternal()
	{
		mp_Position = 0;
		mp_pFile = nullptr;
	}

	void CVirtualFS::CBinaryStreamFileInternal::f_Open(CFileInternal *_pFile)
	{
		mp_pFile = _pFile;
	}

	void CVirtualFS::CBinaryStreamFileInternal::f_Reset()
	{
		mp_Position = 0;
		mp_pFile = nullptr;
	}

	void CVirtualFS::CBinaryStreamFileInternal::f_FeedBytes(void const *_pMem, umint _nBytes)
	{
		mp_pFile->f_Write(_pMem, mp_Position, _nBytes);
		mp_Position += _nBytes;
	}

	void CVirtualFS::CBinaryStreamFileInternal::f_ConsumeBytes(void *_pMem, umint _nBytes)
	{
		mp_pFile->f_Read(_pMem, mp_Position, _nBytes);
		mp_Position += _nBytes;
	}

	bool CVirtualFS::CBinaryStreamFileInternal::f_IsValid() const
	{
		return mp_pFile != nullptr;
	}

	bool CVirtualFS::CBinaryStreamFileInternal::f_IsAtEndOfStream() const
	{
		return mp_Position == mp_pFile->m_FileRecord.m_FileSize;
	}

	NStream::CFilePos CVirtualFS::CBinaryStreamFileInternal::f_GetPosition() const
	{
		return mp_Position;
	}

	void CVirtualFS::CBinaryStreamFileInternal::f_SetPosition(NStream::CFilePos _Pos)
	{
		mp_Position = _Pos;
	}

	void CVirtualFS::CBinaryStreamFileInternal::f_SetPositionFromEnd(NStream::CFilePos _Pos)
	{
		mp_Position = mp_pFile->m_FileRecord.m_FileSize - _Pos;
	}

	void CVirtualFS::CBinaryStreamFileInternal::f_AddPosition(NStream::CFilePos _Pos)
	{
		mp_Position += _Pos;
	}

	bool CVirtualFS::CBinaryStreamFileInternal::f_IsValidReadPosition(NStream::CFilePos _Pos) const
	{
		return _Pos >= 0 && _Pos < NStream::CFilePos(mp_pFile->m_FileRecord.m_FileSize);
	}

	void CVirtualFS::CBinaryStreamFileInternal::f_Flush(bool _bLocalCacheOnly)
	{
		return mp_pFile->f_Flush();
	}

	void CVirtualFS::CBinaryStreamFileInternal::f_SetCacheSize(umint _CacheSize)
	{
	}

	NStream::CFilePos CVirtualFS::CBinaryStreamFileInternal::f_GetLength() const
	{
		return mp_pFile->m_FileRecord.m_FileSize;
	}

	umint CVirtualFS::CBinaryStreamFileInternal::f_ContainerLengthLimit() const
	{
		return NStream::fg_CapLengthLimit(f_GetLength() - f_GetPosition());
	}

	void CVirtualFS::CBinaryStreamFileInternal::f_SetLength(NStream::CFilePos _Length)
	{
		return mp_pFile->f_SetLength(_Length);
	}

	CVirtualFS::CVirtualFS()
	{
		mp_bOutOfSpace = false;
		mp_pStorageStream = nullptr;
		mp_ClusterCache.f_Create(this);
		mp_DirectoryCache.f_Create(this);
	}

	CVirtualFS::~CVirtualFS()
	{
		fp_Close();
	}

	bool CVirtualFS::f_IsValid()
	{
		return mp_pStorageStream != nullptr;
	}

	void CVirtualFS::CCheckReporter::f_IncreaseProgress(uint64 _nInc)
	{
	}

	void CVirtualFS::CCheckReporter::f_StepDown(uint64 _nMax)
	{
	}

	void CVirtualFS::CCheckReporter::f_StepUp()
	{
	}

	CVirtualFS::CCheckFSContext::CClusterCheck::CClusterCheck()
	{
		m_Flags = 0;
		m_ClusterID = 0;
	}

	CVirtualFS::CCheckFSContext::CCheckFSContext(NStream::CBinaryStream *_pStorageStream, CCheckReporter *_pReporter, bool _bFix, CVirtualFS *_pFixDestinationFS)
	{
		mp_pStorageStream = _pStorageStream;
		mp_pReporter = _pReporter;
		mp_bFix = _bFix;
		mp_pFixDestinationFS = _pFixDestinationFS;
		mp_MaxDirectoryID = 0;
	}

	bool CVirtualFS::CCheckFSContext::CCircularChecker::f_AlreadyInList(CClusterID _ClusterID)
	{
		auto iMember = m_List.f_GetIterator();

		while (iMember)
		{
			if (iMember->m_ClusterID == _ClusterID)
				return true;

			++iMember;
		}

		return false;
	}

	CMibFilePos CVirtualFS::f_GetTotalSize()
	{
		if (mp_pStorageStream)
			return mp_pStorageStream->f_GetLength();
		return 0;
	}

	void CVirtualFS::f_Open(NStream::CBinaryStream *_pStorageStream)
	{
		fp_Close();

		mp_pStorageStream = _pStorageStream;
		fp_SetStreamCluster(0); // Seek to beginning
		mp_RootData.f_Read(*mp_pStorageStream);
		if (!mp_RootData.f_Validate())
			DMibErrorFile("Root data corrupt");

		mp_pStorageStream->f_SetCacheSize(mp_RootData.m_ClusterSize);

		mp_ClusterIDsPerCluster = (mp_RootData.m_ClusterSize - (sizeof(NCryptography::CHashDigest_MD5) + sizeof(CClusterID) * 2 + sizeof(uint64))) / sizeof(CClusterID);
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

		CDirectoryCacheEntry *pEntry = fg_ConstructObject<CDirectoryCacheEntry>(NMemory::CDefaultAllocator());
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

		CDirectoryCacheEntry *pEntry = fg_ConstructObject<CDirectoryCacheEntry>(NMemory::CDefaultAllocator());
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
		auto iFile = m_FileClusters.f_GetIterator();

		while (iFile)
		{
			CFile *pIter = iFile;
			if (pIter->m_FileName.f_IsEmpty())
				pIter->f_SetParent(nullptr);
			++iFile;
		}

		// Find roots
		iFile = m_FileClusters;

		while (iFile)
		{
			CFile *pRoot = iFile;
			while (pRoot->m_pParent)
				pRoot = pRoot->m_pParent;
			m_Roots.f_Insert(pRoot);
			++iFile;
		}

		// Merge equal directory entries

		{
			bool bDoneSomething = true;
			while (bDoneSomething)
			{
				bDoneSomething = false;
				auto iDirectory = m_Directories.f_GetIterator();

				while (iDirectory)
				{
					CFile *pFile = iDirectory;
					auto iDuplicate = iDirectory;

					CFile *pBestFile = pFile;
					uint32 BestDepth = pBestFile->f_GetDepth();
					umint nDirs = 0;
					while (iDuplicate)
					{
						CFile *pFile2 = iDuplicate;
						if (pFile2->m_DirectoryID != pFile->m_DirectoryID)
							break;
						++nDirs;
						uint32 ThisDepth = pFile2->f_GetDepth();
						if (ThisDepth > BestDepth)
						{
							pBestFile = pFile2;
							BestDepth = ThisDepth;
						}
						++iDuplicate;
					}

					if (nDirs > 1)
					{
						auto iDuplicate = iDirectory;

						while (iDuplicate)
						{
							CFile *pFile2 = iDuplicate;
							if (pFile2->m_DirectoryID != pFile->m_DirectoryID)
								break;

							if (pFile2 != pBestFile)
							{
								DMibListLinkDS_Iter(CFile, m_ChildLink) iChild = pFile2->m_Children;
								while (iChild)
								{
									CFile *pChild = iChild;
									++iChild;

									CVirtualFS::CDirectory::CFile *pDirEntry = pBestFile->f_GetDirectoryEntry(pChild->m_FileClusterID);
									if (pDirEntry)
									{
										pChild->m_FileName = pDirEntry->m_FileName;
										pChild->f_SetParent(pBestFile);
										bDoneSomething = true;
									}
								}
							}
							++iDuplicate;
						}
					}

					++iDirectory;
				}
			}
		}
	}

	void CVirtualFS_FixFSFileTree::f_CopyFile
		(
			CVirtualFS::CFileRecord &_FileRecord
			, CVirtualFS::CFile &_File
			, uint64 _ClusterSize
			, uint64 _ClusterIDsPerCluster
			, NStream::CBinaryStream *_pStorageStream
		)
	{
		mp_pReporter->f_StepDown(_FileRecord.m_FileSize);
		try
		{
			CVirtualFS::CClusterChain Chain;
			Chain.m_ClusterIDs.f_SetLen(_ClusterIDsPerCluster);

			CVirtualFS::CClusterID Cluster = _FileRecord.m_FirstFileChainCluster;
			NContainer::CByteVector Temp;
			Temp.f_SetLen(_ClusterSize);

			umint iCluster = 0;

			while (Cluster)
			{
				_pStorageStream->f_SetPosition(Cluster * _ClusterSize);
				Chain.f_Read(*_pStorageStream);

				if (!Chain.f_Validate(Cluster, mp_Version))
					return;

				for (uint64 iClusterID = 0; iClusterID < Chain.m_nClusterIDs; ++iClusterID)
				{
					_pStorageStream->f_SetPosition(Chain.m_ClusterIDs[iClusterID] * _ClusterSize);

					if (iCluster * _ClusterSize < _FileRecord.m_FileSize)
					{
						umint Size = fg_Min(_FileRecord.m_FileSize - iCluster * _ClusterSize, _ClusterSize);
						_pStorageStream->f_ConsumeBytes(Temp.f_GetArray(), Size);
						_File.f_Write(Temp.f_GetArray(), Size);
						mp_pReporter->f_IncreaseProgress(Size);
					}

					++iCluster;
				}

				Cluster = Chain.m_NextChainCluster;
			}

			// _File.f_SetLength(_FileRecord.m_FileSize);
			mp_pReporter->f_StepUp();
		}
		catch (NException::CException)
		{
			mp_pReporter->f_StepUp();
		}
	}

	void CVirtualFS_FixFSFileTree::f_CopyTreeRecursive
		(
			NStr::CStr const &_Path
			, CFile *_pFile
			, uint64 _ClusterSize
			, uint64 _ClusterIDsPerCluster
			, NStream::CBinaryStream *_pStorageStream
			, CVirtualFS *_pFixDestinationFS
		)
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
					File.f_Open(*_pFixDestinationFS, _Path + "/Recovered." + FileRecord.m_MD5Checksum.f_GetString(), EFileOpen_Write);
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

		DMibListLinkDS_Iter(CFile, m_ChildLink) iChild = _pFile->m_Children;

		mp_pReporter->f_StepDown(_pFile->m_Children.f_GetLen());

		while (iChild)
		{
			NStr::CStr FileName = iChild->m_FileName;
			if (FileName == "")
				FileName = FileRecord.m_MD5Checksum.f_GetString();

			NStr::CStr Path;
			if (_Path != "")
				Path = _Path + "/" + FileName;
			else
				Path = FileName;

			f_CopyTreeRecursive(Path, iChild, _ClusterSize, _ClusterIDsPerCluster, _pStorageStream, _pFixDestinationFS);
			++iChild;
			mp_pReporter->f_IncreaseProgress(1);
		}
		mp_pReporter->f_StepUp();
	}

	void CVirtualFS_FixFSFileTree::f_CopyTree(uint64 _ClusterSize, uint64 _ClusterIDsPerCluster, NStream::CBinaryStream *_pStorageStream, CVirtualFS *_pFixDestinationFS)
	{
		uint64 iCurrentRecover = 0;

		mp_pReporter->f_StepDown(m_Roots.f_GetLen());

		for (umint iPass = 0; iPass < 2; ++iPass)
		{
			DMibListLinkDS_Iter(CFile, m_RootLink) iRoot = m_Roots;

			while (iRoot)
			{
				CFile *pFile = iRoot;

				if (pFile->m_FileClusterID == 1 && pFile->m_bIsDir && pFile->m_DirectoryID == 1)
				{
					if (iPass == 0)
					{
						DMibDTrace("Root Dir: {} Cluster: {}" DMibNewLine, iRoot->m_DirectoryID, iRoot->m_FileClusterID);
						// This is the root
						f_CopyTreeRecursive("", pFile, _ClusterSize, _ClusterIDsPerCluster, _pStorageStream, _pFixDestinationFS);
					}
				}
				else if (iPass == 1)
				{
					DMibDTrace("Root Dir: {} Cluster: {}" DMibNewLine, iRoot->m_DirectoryID, iRoot->m_FileClusterID);
					if (pFile->m_bIsDir)
					{
						NStr::CStr PathName = (NStr::CStr::CFormat("Lost&Found/RecoveredDir.{sj8,sf0}") << iCurrentRecover).f_GetStr();
						_pFixDestinationFS->f_CreateDirectory(PathName);
						++iCurrentRecover;

						f_CopyTreeRecursive(PathName, pFile, _ClusterSize, _ClusterIDsPerCluster, _pStorageStream, _pFixDestinationFS);
					}
					else
					{
						_pFixDestinationFS->f_CreateDirectory("Lost&Found");
						NStr::CStr PathName = (NStr::CStr::CFormat("Lost&Found/Recovered.{sj8,sf0}") << iCurrentRecover).f_GetStr();
						++iCurrentRecover;

						f_CopyTreeRecursive(PathName, pFile, _ClusterSize, _ClusterIDsPerCluster, _pStorageStream, _pFixDestinationFS);
					}
				}

				if (iPass == 1)
					mp_pReporter->f_IncreaseProgress(1);

				++iRoot;
			}
		}
		mp_pReporter->f_StepUp();
	}

	CVirtualFS::ECheckFSError CVirtualFS::fs_CheckFS(NStream::CBinaryStream *_pStorageStream, CCheckReporter *_pReporter, bool _bFix, CVirtualFS *_pFixDestinationFS)
	{
		DMibRequire(_pStorageStream);
		DMibRequire(_pReporter);
		try
		{
			if (_pFixDestinationFS)
			{
				_pReporter->f_ReportError("Rebuilding file system.");

				// First detemine the cluster size

				_pStorageStream->f_SetPosition(0);

				CRootData RootData;
				RootData.f_Read(*_pStorageStream);

				uint64 nMaxFiles = 0;
				uint64 BestClusterSize = 128;
				uint64 FileLenth = _pStorageStream->f_GetLength();
				uint32 Version = 0x100;
				_pReporter->f_StepDown(3);

				if (RootData.f_Validate())
				{
					BestClusterSize = RootData.m_ClusterSize;
					Version = RootData.m_Version;
					nMaxFiles = 1;
				}
				else
				{
					uint64 ClusterSize = 128;
					// Loop until the number of files found decrease
					_pReporter->f_StepDown(13);
					aint iLoop = 0;
					uint32 Versions[2] = {0x102, 0x103};
					uint32 nVersions = 2;
					while (ClusterSize < FileLenth)
					{
						uint64 nClusters = FileLenth / ClusterSize;
						CFileRecord FileRecord;
						uint64 FileCounts[2] = {0, 0};
						if (iLoop < 13)
							_pReporter->f_StepDown(nClusters);
						for (uint64 iCluster = 0; iCluster < nClusters; ++iCluster)
						{
							_pStorageStream->f_SetPosition(ClusterSize * iCluster);
							FileRecord.f_Read(*_pStorageStream);
							for (umint iVersion = 0; iVersion < nVersions; ++iVersion)
							{
								if (FileRecord.f_Validate(iCluster, Versions[iVersion]))
									++FileCounts[iVersion];
							}
							if (iLoop < 13)
								_pReporter->f_IncreaseProgress(1);
						}

						if (iLoop < 13)
							_pReporter->f_StepUp();

						++iLoop;

						bool bWasBetter = false;
						for (umint iVersion = 0; iVersion < nVersions; ++iVersion)
						{
							if (FileCounts[iVersion] >= nMaxFiles)
							{
								bWasBetter = true;
								Version = Versions[iVersion];
								BestClusterSize = ClusterSize;
								nMaxFiles = FileCounts[iVersion];
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

				umint ClusterSize = BestClusterSize;
				uint64 ClusterIDsPerCluster = (ClusterSize - (sizeof(NCryptography::CHashDigest_MD5) + sizeof(CClusterID) * 2 + sizeof(uint64))) / sizeof(CClusterID);
				uint64 nClusters = FileLenth / ClusterSize;

				// First lets build up the directory tree
				CVirtualFS_FixFSFileTree Tree(_pReporter, Version);

				_pReporter->f_StepDown(nClusters);

				for (uint64 iCluster = 0; iCluster < nClusters; ++iCluster)
				{
					_pStorageStream->f_SetPosition(ClusterSize * iCluster);
					CFileRecord FileRecord;
					FileRecord.f_Read(*_pStorageStream);

					if (FileRecord.f_Validate(iCluster, Version) && CCheckFSContext::fs_CheckClusterChain(Version, ClusterSize, ClusterIDsPerCluster, _pStorageStream, FileRecord))
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

								NStream::TCBinaryStreamMemoryPtr<> Stream;
								Stream.f_OpenRead(Data.f_GetArray(), Data.f_GetLen());

								Dir.f_Read(Stream);

								if (!Dir.f_Validate(iCluster, Version))
								{
									if (Version < 0x103)
									{
										if (!Dir.f_Validate(iCluster, 0x101))
											continue;
									}
									else
										continue;
								}
								if (FileRecord.m_ParentDirectory == 0 && Dir.m_DirectoryID != 1) // Only root dir can have parent dir 0
									continue;
							}
							catch (NException::CException)
							{
								continue;
							}

							pFile = Tree.f_GetDirectory(iCluster, Dir.m_DirectoryID);
							pFile->m_Directory = Dir;
						}
						else
							pFile = Tree.f_GetFile(iCluster);

						pFile->m_ParentID = FileRecord.m_ParentDirectory;
						pFile->m_FileSize = FileRecord.m_FileSize;

						if (!pFile->m_pParent)
						{
							CVirtualFS_FixFSFileTree::CFile *pParent = Tree.f_GetDirectory(pFile->m_ParentID);
							pFile->f_SetParent(pParent);
						}

						if (FileRecord.m_FileAttributes & EFileAttrib_Directory)
						{
							for (umint iFile = 0; iFile < Dir.m_Files.f_GetLen(); ++iFile)
							{
								CVirtualFS_FixFSFileTree::CFile *pSubFile = Tree.f_GetFile(Dir.m_Files[iFile].m_FileRecordCluster);
								NStr::CStr FileName = Dir.m_Files[iFile].m_FileName;
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

				_pReporter->f_ReportError("File system successfully rebuilt.");

				_pReporter->f_StepUp();

				return ECheckFSError_None;
			}
			else
			{
				if (_bFix)
					_pReporter->f_ReportError("Fixing file system.");
				else
					_pReporter->f_ReportError("Checking file system.");

				ECheckFSError Ret = ECheckFSError_None;
				for (int iPass = 0; iPass < 2; ++iPass)
				{
					_pStorageStream->f_SetPosition(0);

					CCheckFSContext Context(_pStorageStream, _pReporter, _bFix, _pFixDestinationFS);

					Ret = ECheckFSError_None;

					Context.mp_RootData.f_Read(*_pStorageStream);
					if (!Context.mp_RootData.f_Validate())
					{
						_pReporter->f_ReportError("Error: Root data checksum failure");
						_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_ClusterSize {}") << Context.mp_RootData.m_ClusterSize);
						_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_RootFileCluster {}") << Context.mp_RootData.m_RootFileCluster);
						_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_FirstFreeCluster {}") << Context.mp_RootData.m_FirstFreeCluster);
						_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_FileSystemGrowSize {}") << Context.mp_RootData.m_FileSystemGrowSize);
						_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_nFreeClusters {}") << Context.mp_RootData.m_nFreeClusters);
						_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_DirectoryCacheSize {}") << Context.mp_RootData.m_DirectoryCacheSize);
						_pReporter->f_ReportError(NStr::CStr::CFormat("\tm_ClusterCacheSize {}") << Context.mp_RootData.m_ClusterCacheSize);
						Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
						break;
					}

					if (Context.mp_RootData.m_Version < 0x102)
					{
						if (_bFix)
							Context.mp_pReporter->f_ReportError("Upgrading file system version");
						else
						{
							Context.mp_pReporter->f_ReportError("File system version upgrade needed");
							Ret = fg_Max(Ret, ECheckFSError_VersionUpgradeFixable);
						}
					}

					uint64 ClusterSize = Context.mp_RootData.m_ClusterSize;

					Context.mp_ClusterIDsPerCluster = (ClusterSize - (sizeof(NCryptography::CHashDigest_MD5) + sizeof(CClusterID) * 2 + sizeof(uint64))) / sizeof(CClusterID);

					CClusterID nClusters = ((_pStorageStream->f_GetLength() + ClusterSize - 1) / ClusterSize);
					CMibFilePos Size = CMibFilePos(nClusters) * CMibFilePos(ClusterSize);
					if (Size != _pStorageStream->f_GetLength())
					{
						// Unaligned stream length
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
			if (_bFix)
				_pReporter->f_ReportError(NStr::CStr("Exception during file system fix: ") + _Exception.f_GetErrorStr());
			else
				_pReporter->f_ReportError(NStr::CStr("Exception during file system check: ") + _Exception.f_GetErrorStr());

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

		umint nClusters = mp_ClusterMap.f_GetLen();

		CClusterCheck *pClusters = mp_ClusterMap.f_GetArray();

		for (umint iCluster = 0; iCluster < nClusters; ++iCluster)
		{
			if (pClusters[iCluster].m_Flags & EClusterCheckFlag_Free)
			{
				// Search for the type of cluster
				f_Seek(iCluster);

				CFreeCluster NewFree;
				NewFree.m_NextFreeCluster = mp_RootData.m_FirstFreeCluster;
				mp_RootData.m_FirstFreeCluster = iCluster;
				++mp_RootData.m_nFreeClusters;
				NewFree.f_GenerateChecksum(iCluster, mp_RootData.m_Version);
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

	uint64 CVirtualFS::CCheckFSContext::f_AddClusterID(CClusterID _Cluster, CClusterID _File, uint64 _Flags, bool _bMap)
	{
		if (_Cluster > mp_ClusterMap.f_GetLen())
		{
			if (mp_pReporter)
				mp_pReporter->f_ReportError("Cluster Map: Cluster is invalid");
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
					mp_pReporter->f_ReportError("Free Cluster Failed Validation");

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
					mp_pReporter->f_ReportError("Failed to add free cluster to cluster map, aborting");

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
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("Number of free  clusters does not match {} != {}") << nFreeClusters << mp_RootData.m_nFreeClusters);
			return ECheckFSError_InplaceFixable;
		}
		else
			return ECheckFSError_None;
	}

	bool CVirtualFS::CCheckFSContext::f_WriteFileData(CVirtualFS::CFileRecord &_FileRecord, NContainer::CByteVector &_Data, umint _Length)
	{
		DMibSafeCheck(_Data.f_GetLen() <= _FileRecord.m_FileSize, "Write can only shrink file");

		_FileRecord.m_FileSize = _Length;

		CClusterChain Chain;
		Chain.m_ClusterIDs.f_SetLen(mp_ClusterIDsPerCluster);

		CClusterID Cluster = _FileRecord.m_FirstFileChainCluster;

		umint iCluster = 0;
		umint iEndCluster = (_FileRecord.m_FileSize + mp_RootData.m_ClusterSize - 1) / mp_RootData.m_ClusterSize;

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
			for (uint64 iClusterID = 0; iClusterID < nClusterIDs; ++iClusterID)
			{
				if (iCluster >= iEndCluster)
				{
					f_FreeCluster(Chain.m_ClusterIDs[iClusterID]);
					Changed = fg_Max(Changed, 1);
					--Chain.m_nClusterIDs;
				}
				else
				{
					f_Seek(Chain.m_ClusterIDs[iClusterID]);

					umint Size = fg_Min(_FileRecord.m_FileSize - iCluster * mp_RootData.m_ClusterSize, mp_RootData.m_ClusterSize);

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

	auto CVirtualFS::CCheckFSContext::fs_ReadFileData
		(
			uint32 _Version
			, uint64 _ClusterSize
			, uint64 _ClusterIDsPerCluster
			, NStream::CBinaryStream *_pStorageStream
			, CVirtualFS::CFileRecord &_FileRecord
			, NContainer::CByteVector &_Data
		)
		-> bool
	{
		_Data.f_SetLen(_FileRecord.m_FileSize);

		CClusterChain Chain;
		Chain.m_ClusterIDs.f_SetLen(_ClusterIDsPerCluster);

		CClusterID Cluster = _FileRecord.m_FirstFileChainCluster;

		umint iCluster = 0;

		while (Cluster)
		{
			_pStorageStream->f_SetPosition(Cluster * _ClusterSize);
			Chain.f_Read(*_pStorageStream);

			if (!Chain.f_Validate(Cluster, _Version))
				return false;

			for (uint64 iClusterID = 0; iClusterID < Chain.m_nClusterIDs; ++iClusterID)
			{
				_pStorageStream->f_SetPosition(Chain.m_ClusterIDs[iClusterID] * _ClusterSize);

				if (iCluster * _ClusterSize < _FileRecord.m_FileSize)
				{
					umint Size = fg_Min(_FileRecord.m_FileSize - iCluster * _ClusterSize, _ClusterSize);

					_pStorageStream->f_ConsumeBytes(_Data.f_GetArray() + iCluster * _ClusterSize, Size);
				}

				++iCluster;
			}

			Cluster = Chain.m_NextChainCluster;
		}

		return true;
	}

	auto CVirtualFS::CCheckFSContext::fs_CheckClusterChain
		(
			uint32 _Version
			, uint64 _ClusterSize
			, uint64 _ClusterIDsPerCluster
			, NStream::CBinaryStream *_pStorageStream
			, CVirtualFS::CFileRecord &_FileRecord
		)
		-> bool
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

			for (uint64 iClusterID = 0; iClusterID < Chain.m_nClusterIDs; ++iClusterID)
			{
				uint32 Position = Chain.m_ClusterIDs[iClusterID] * _ClusterSize;
				if (!_pStorageStream->f_IsValidReadPosition(Position))
					return false;
			}

			Cluster = Chain.m_NextChainCluster;
		}

		return true;
	}

	bool CVirtualFS::CCheckFSContext::f_ReadFileData(CVirtualFS::CFileRecord &_FileRecord, NContainer::CByteVector &_Data)
	{
		_Data.f_SetLen(_FileRecord.m_FileSize);

		CClusterChain Chain;
		Chain.m_ClusterIDs.f_SetLen(mp_ClusterIDsPerCluster);

		CClusterID Cluster = _FileRecord.m_FirstFileChainCluster;

		umint iCluster = 0;

		while (Cluster)
		{
			f_Seek(Cluster);
			Chain.f_Read(*mp_pStorageStream);

			if (!Chain.f_Validate(Cluster, mp_RootData.m_Version))
				return false;

			for (uint64 iClusterID = 0; iClusterID < Chain.m_nClusterIDs; ++iClusterID)
			{
				f_Seek(Chain.m_ClusterIDs[iClusterID]);

				if (iCluster * mp_RootData.m_ClusterSize < _FileRecord.m_FileSize)
				{
					umint Size = fg_Min(_FileRecord.m_FileSize - iCluster * mp_RootData.m_ClusterSize, mp_RootData.m_ClusterSize);

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

		umint nClusters = mp_ClusterMap.f_GetLen();

		mp_pReporter->f_StepDown(nClusters * 2);

		umint nOrphan = 0;
		umint nFiles = 0;

		CClusterCheck *pClusters = mp_ClusterMap.f_GetArray();
		for (umint iCluster = 0; iCluster < nClusters; ++iCluster)
		{
			if (!pClusters[iCluster].m_ClusterID)
			{
				// Search for the type of cluster
				f_Seek(iCluster);
				CVirtualFS::CFileRecord FileRecord;
				FileRecord.f_Read(*mp_pStorageStream);

				if (FileRecord.f_Validate(iCluster, mp_RootData.m_Version)) // This is a file record
				{
					++nFiles;
					if (mp_bFix)
						f_FreeCluster(iCluster);
					else
					{
						ECheckFSError TreeCheck = f_CheckFSTree(iCluster, false);

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

		for (umint iCluster = 0; iCluster < nClusters; ++iCluster)
		{
			if (!pClusters[iCluster].m_ClusterID)
			{
				// Search for the type of cluster
				f_Seek(iCluster);
				CVirtualFS::CFileRecord FileRecord;
				FileRecord.f_Read(*mp_pStorageStream);

				if (FileRecord.f_Validate(iCluster, mp_RootData.m_Version)) // This is a file record
				{
					if (mp_bFix)
					{
						// DMibSafeCheck(0, "No files should be here");
					}
				}
				else
				{
					++nOrphan;
					if (mp_bFix)
						f_FreeCluster(iCluster);
					else
						Ret = fg_Max(Ret, ECheckFSError_InplaceFixable); // We can just return the data
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

	CVirtualFS::ECheckFSError CVirtualFS::CCheckFSContext::f_CheckFSTree(CClusterID _ClusterID, bool _bMap)
	{
		CCircularChecker CircularChecker;
		bool bRemove = false;
		ECheckFSError Ret = f_CheckFSTreeRecursive(_ClusterID, CircularChecker, bRemove, _bMap, 0, "Root");
		if (bRemove)
			Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
		return Ret;
	}

	auto CVirtualFS::CCheckFSContext::f_CheckFSTreeRecursive
		(
			CClusterID _ClusterID
			, CCircularChecker &_CircularChecker
			, bool &_bRemove
			, bool _bMap
			, CDirectoryID _ParentDir
			, NStr::CStr const &_FileName
		)
		-> CVirtualFS::ECheckFSError
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
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: This file record is already referenced somewhere else ") << _FileName);
				return fg_Max(Ret, ECheckFSError_NewFSFixable);
			}

			if (AddResult & EClusterCheckFlag_Invalid)
			{
				if (mp_pReporter)
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: The file record cluster is invalid (outside of the file stream)") << _FileName);
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
				// return fg_Max(Ret, ECheckFSError_InplaceFixable);
				return fg_Max(Ret, ECheckFSError_NewFSFixable);
			}
		}

		bool bChangedFileRecord = false;

		if (FileRecord.m_ParentDirectory != _ParentDir)
		{
			if (mp_pReporter)
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: File record parent directory ID mismatch") << _FileName);

			if (mp_bFix)
			{
				FileRecord.m_ParentDirectory = _ParentDir;
				bChangedFileRecord = true;
			}
			else
			{
				// return fg_Max(Ret, ECheckFSError_InplaceFixable);
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
					// return fg_Max(Ret, ECheckFSError_InplaceFixable);
				}
			}

			// mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Checking chain {} {}") << _FileName << Current << Chain.m_NextChainCluster);

			uint64 AddResult = f_AddClusterID(Current, _ClusterID, EClusterCheckFlag_FileClusterChain, _bMap);

			if (AddResult)
			{
				if (mp_pReporter)
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: File cluster chain cluster used somewhere else") << _FileName);
				Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
			}

			if (iCluster >= iEndCluster)
			{
				if (mp_pReporter)
					mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: File id chain contains too many ids") << _FileName);
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
			for (uint64 iClusterID = 0; iClusterID < nClusterIDs; ++iClusterID)
			{
				if (iCluster >= iEndCluster && mp_bFix)
				{
					f_FreeCluster(Chain.m_ClusterIDs[iClusterID]);
					Changed = fg_Max(Changed, 1);
					--Chain.m_nClusterIDs;
				}
				else
				{
					FileSize += mp_RootData.m_ClusterSize;
					uint64 AddResult = f_AddClusterID(Chain.m_ClusterIDs[iClusterID], _ClusterID, EClusterCheckFlag_FileData, _bMap);

					if (AddResult)
					{
						if (mp_pReporter)
							mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: File data cluster used somewhere else") << _FileName);
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
						mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: Cluster chain error") << _FileName);
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
				mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: File cluster data size mismatch") << _FileName);
			if (mp_bFix)
			{
				FileRecord.m_FileSize = fg_Min(FileRecord.m_FileSize, FileSize);
				f_Seek(_ClusterID);
				FileRecord.f_GenerateChecksum(_ClusterID, mp_RootData.m_Version);
				FileRecord.f_Write(*mp_pStorageStream);
			}
			else
				Ret = fg_Max(Ret, ECheckFSError_InplaceFixable);
		}

		if (FileRecord.m_FileAttributes & NFile::EFileAttrib_Directory)
		{
			// Read directory data
			NContainer::CByteVector DirectoryData;
			f_ReadFileData(FileRecord, DirectoryData);

			NStream::TCBinaryStreamMemoryPtr<> Stream;
			Stream.f_OpenRead(DirectoryData.f_GetArray(), DirectoryData.f_GetLen());

			CVirtualFS::CDirectory Directory;
			Directory.f_Read(Stream);
			bool bChanged = false;

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

				umint nFiles = Directory.m_Files.f_GetLen();

				mp_pReporter->f_StepDown(nFiles);

				for (umint iFile = 0; iFile < nFiles; ++iFile)
				{
					CClusterID ClusterID = Directory.m_Files[iFile].m_FileRecordCluster;
					NStr::CStr FileName = Directory.m_Files[iFile].m_FileName;

					if (_CircularChecker.f_AlreadyInList(ClusterID))
					{
						if (mp_pReporter)
							mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: Circular directory dependency found") << _FileName);
						Ret = fg_Max(Ret, ECheckFSError_NewFSFixable);
					}
					else
					{
						bool bRemove = false;
						Ret = fg_Max(Ret, f_CheckFSTreeRecursive(ClusterID, _CircularChecker, bRemove, _bMap, Directory.m_DirectoryID, _FileName + "/" + FileName));

						if (bRemove && mp_bFix)
						{
							bChanged = true;
							Directory.m_Files.f_Remove(iFile);
							--iFile;
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
				NStream::TCBinaryStreamMemory<> Stream;
				Directory.f_GenerateChecksum(_ClusterID, mp_RootData.m_Version);
				Directory.f_Write(Stream);
				if ((uint64)Stream.f_GetLength() != FileRecord.m_FileSize)
				{
					//DMibSafeCheck((uint64)Stream.f_GetLength() < FileRecord.m_FileSize, "Should not happen");
					if (mp_pReporter)
						mp_pReporter->f_ReportError(NStr::CStr::CFormat("{}: Error: Directory file length not correct") << _FileName);
					if (mp_bFix)
						bChanged = true;
					else
						Ret = fg_Max(Ret, ECheckFSError_InplaceFixable);
				}
			}

			if (mp_RootData.m_Version == 0x101 && mp_bFix)
			{
				// Convert to new version;
				bChanged = true;
			}

			if (bChanged)
			{
				NStream::TCBinaryStreamMemory<> Stream;
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
		mp_RootData.m_ClusterCacheSize = (8 * 1024 * 1024 + _ClusterSize - 1) / _ClusterSize; // Cache 8 MiB of data as default
		mp_DirectoryCache.f_SetCacheSize(mp_RootData.m_DirectoryCacheSize);
		mp_ClusterCache.f_SetCacheSize(mp_RootData.m_ClusterCacheSize);

		mp_ClusterIDsPerCluster = (mp_RootData.m_ClusterSize - (sizeof(NCryptography::CHashDigest_MD5) + sizeof(CClusterID) * 2 + sizeof(uint64))) / sizeof(CClusterID);
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

			for (CClusterID iFreeCluster = 0; iFreeCluster < NumFreeClusters; ++iFreeCluster)
			{
				CFreeCluster NewFreeCluster;
				if (iFreeCluster < (NumFreeClusters - 1))
					NewFreeCluster.m_NextFreeCluster = mp_RootData.m_FirstFreeCluster + iFreeCluster + 1;
				else
					NewFreeCluster.m_NextFreeCluster = 0; // No more free
				NewFreeCluster.f_GenerateChecksum(mp_RootData.m_FirstFreeCluster + iFreeCluster, mp_RootData.m_Version);
				fp_SetStreamCluster(mp_RootData.m_FirstFreeCluster + iFreeCluster);
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

	void CVirtualFS::f_SetCacheSizes(umint _nDirectories, umint _nClusterBytes)
	{
		umint nClusters = fg_Max((_nClusterBytes + mp_RootData.m_ClusterSize - 1) / mp_RootData.m_ClusterSize, umint(1));
		mp_DirectoryCache.f_SetCacheSize(fg_Max(umint(1), _nDirectories));
		mp_ClusterCache.f_SetCacheSize(nClusters);
	}

	void CVirtualFS::f_CloseOpenFiles()
	{
		CFile *pFile = mp_OpenFiles.f_Pop();
		while (pFile)
		{
			pFile->f_Close();
			pFile = mp_OpenFiles.f_Pop();
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
			mp_pStorageStream = nullptr;
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

				for (CClusterID iFreeCluster = 0; iFreeCluster < NumFreeClusters; ++iFreeCluster)
				{
					CFreeCluster NewFreeCluster;
					if (iFreeCluster < (NumFreeClusters - 1))
						NewFreeCluster.m_NextFreeCluster = mp_RootData.m_FirstFreeCluster + iFreeCluster + 1;
					else
						NewFreeCluster.m_NextFreeCluster = 0; // No more free
					NewFreeCluster.f_GenerateChecksum(mp_RootData.m_FirstFreeCluster + iFreeCluster, mp_RootData.m_Version);
					CClusterCacheEntry *pCluster = mp_ClusterCache.f_GetCluster(mp_RootData.m_FirstFreeCluster + iFreeCluster);
					NStream::TCBinaryStreamMemoryPtr<> Stream;
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
			NStream::TCBinaryStreamMemoryPtr<> Stream;
			CFreeCluster FreeCluster;
			pCluster->f_OpenStream(Stream);
			FreeCluster.f_Read(Stream);
			pCluster->f_CloseStream(false);
			mp_ClusterCache.f_ReturnCluster(pCluster);

			if (!FreeCluster.f_Validate(mp_RootData.m_FirstFreeCluster, mp_RootData.m_Version))
				DMibErrorFile("Free cluster record checksum failure");

			mp_RootData.m_FirstFreeCluster = FreeCluster.m_NextFreeCluster;
			--mp_RootData.m_nFreeClusters;

			fp_SaveRootData();

			return FreeClusterID;
		}

		DMibErrorFile("Out of space");
		mp_bOutOfSpace = true;

		// return 0;
	}

	void CVirtualFS::fp_ReturnCluster(CVirtualFS::CClusterID _Cluster)
	{
		CFreeCluster NewFree;
		NewFree.m_NextFreeCluster = mp_RootData.m_FirstFreeCluster;
		mp_RootData.m_FirstFreeCluster = _Cluster;
		++mp_RootData.m_nFreeClusters;
		NewFree.f_GenerateChecksum(_Cluster, mp_RootData.m_Version);

		CClusterCacheEntry *pCluster = mp_ClusterCache.f_GetCluster(_Cluster);
		NStream::TCBinaryStreamMemoryPtr<> Stream;
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
							for (uint64 iClusterID = 0; iClusterID < m_ClusterChain.m_nClusterIDs; ++iClusterID)
								m_pVirtualFS->fp_ReturnCluster(m_ClusterChain.m_ClusterIDs[iClusterID]);
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
							for (uint64 iClusterID = nNewClusterIDs; iClusterID < m_ClusterChain.m_nClusterIDs; ++iClusterID)
								m_pVirtualFS->fp_ReturnCluster(m_ClusterChain.m_ClusterIDs[iClusterID]);
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
			// FileChainCluster = fp_GetNewCluster();
			pFile = fg_ConstructObject<CFileInternal>(NMemory::CDefaultAllocator());
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
				fg_DeleteObject(NMemory::CDefaultAllocator(), pFile);

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
			pFile = fg_ConstructObject<CFileInternal>(NMemory::CDefaultAllocator());
			pFile->f_Create(this, _ClusterID, true);
			++pFile->m_RefCount;
			mp_InternalFiles.f_Insert(pFile);
		}
		catch (NException::CException)
		{
			if (pFile)
				fg_DeleteObject(NMemory::CDefaultAllocator(), pFile);

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
			fg_DeleteObject(NMemory::CDefaultAllocator(), _pFile);
		}
	}

	void CVirtualFS::f_EnumFiles(NStr::CStr const &_Path, NContainer::TCVector<NStr::CStr> &_Files)
	{
		fp_CheckPath(_Path, true);
		CDirectoryCacheEntry *pEntry = mp_DirectoryCache.f_GetDirectory(_Path);

		if (!pEntry)
			DMibErrorFile("No such directory");

		_Files.f_SetLen(pEntry->m_Directory.m_Files.f_GetLen());

		for (umint iFile = 0; iFile < _Files.f_GetLen(); ++iFile)
			_Files[iFile] = pEntry->m_Directory.m_Files[iFile].m_FileName;

		mp_DirectoryCache.f_ReturnDirectory(pEntry);
	}

	static bool fsg_MatchPattern(ch8 const *_pStr, ch8 const *_pPattern)
	{
		NStr::CStr Temp0 = NStr::CStr(_pStr).f_UpperCase();
		NStr::CStr Temp1 = NStr::CStr(_pPattern).f_UpperCase();
		char const *pParse = Temp0;
		char const *pPattern = Temp1;

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

	static void fsg_FindFilesRecursive(CVirtualFS *_pFS, NStr::CStr const &_Path, NStr::CStr const &_Pattern, uint32 _AttribMask, bool _bRecursive, NContainer::TCVector<NStr::CStr> &_Files)
	{
		NContainer::TCVector<NStr::CStr> Files;
		if (!_Path.f_IsEmpty() && !_pFS->f_FileExists(_Path, EFileAttrib_Directory))
			return;
		_pFS->f_EnumFiles(_Path, Files);

		umint nFiles = Files.f_GetLen();

		for (umint iFile = 0; iFile < nFiles; ++iFile)
		{
			NStr::CStr FileName = Files[iFile];
			if (fsg_MatchPattern(FileName, _Pattern))
			{
				NStr::CStr FullFileName = NFile::CFile::fs_AppendPath(_Path, FileName);

				uint32 Attribs = (uint32)_pFS->f_GetAttributes(FullFileName);
				if ((!(Attribs & NFile::EFileAttrib_Directory) && (_AttribMask & NFile::EFileAttrib_File)) || Attribs & _AttribMask)
					_Files.f_Insert(FullFileName);
				if (Attribs & NFile::EFileAttrib_Directory)
				{
					if (_bRecursive)
						fsg_FindFilesRecursive(_pFS, FullFileName, _Pattern, _AttribMask, _bRecursive, _Files);
				}
			}
		}
	}

	NContainer::TCVector<NStr::CStr> CVirtualFS::f_FindFiles(NStr::CStr const &_Path, uint32 _AttribMask, bool _bRecursive)
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

		NFile::CFileIoTempBuffer Buffer;

		while (Length)
		{
			auto BufferResult = Buffer.f_UseBuffer(Length);
			File.f_Read(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			Checksum.f_AddData(BufferResult.m_pBuffer, BufferResult.m_nBytes);

			Length -= BufferResult.m_nBytes;
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

	void CVirtualFS::fp_DeleteDirectoryRecursive(NStr::CStr const &_File)
	{
		uint64 Attribs = f_GetAttributes(_File);
		if (Attribs & NFile::EFileAttrib_Directory)
		{
			NContainer::TCVector<NStr::CStr> Files;
			f_EnumFiles(_File, Files);
			for (umint iFile = 0; iFile < Files.f_GetLen(); ++iFile)
			{
				NStr::CStr File = _File + "/" + Files[iFile];
				fp_DeleteDirectoryRecursive(File);
			}
		}
		f_DeleteFile(_File);
	}

	void CVirtualFS::f_DeleteDirectoryRecursive(NStr::CStr const &_File)
	{
		fp_CheckPath(_File);
		fp_DeleteDirectoryRecursive(_File);
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
			File = _FileTo;

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

	void CVirtualFS::fp_CheckPath(NStr::CStr const &_Path, bool _bCanBeEmpty)
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
			if (_Path[Pathlen - 1] == '/')
				DMibErrorFile("Invalid path: Path cannot end with '/'.");
		}
	}

	void CVirtualFS::f_CreateDirectory(NStr::CStr const &_Path)
	{
		fp_CheckPath(_Path);

		CDirectoryCacheEntry *pCacheEntry = mp_DirectoryCache.f_GetNewDirectory(_Path);

		if (!pCacheEntry)
			DMibErrorFile("Failed to create directory");
		mp_DirectoryCache.f_ReturnDirectory(pCacheEntry);
	}

	CVirtualFS::CFileInternal *CVirtualFS::fp_OpenFileFromPath(NStr::CStr const &_Path, bool _bCreate, bool _bRemove, bool _bAllowRoot)
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
			File = _Path;

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

	void CVirtualFS::f_SetCreationTime(NStr::CStr const &_Path, NTime::CTime const &_Time)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		pFile->m_FileRecord.m_CreationTime = _Time;
		pFile->m_bFileRecordDirty = true;
		fp_CloseFile(pFile);
	}

	void CVirtualFS::f_SetAccessTime(NStr::CStr const &_Path, NTime::CTime const &_Time)
	{
		CFileInternal *pFile = fp_OpenFileFromPath(_Path, false, false, true);
		if (!pFile)
			DMibErrorFile("No such file or directory");

		// Set attribs, but don't let user set directory attribute
		pFile->m_FileRecord.m_AccessTime = _Time;
		pFile->m_bFileRecordDirty = true;
		fp_CloseFile(pFile);
	}

	void CVirtualFS::f_SetWriteTime(NStr::CStr const &_Path, NTime::CTime const &_Time)
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

	bool CVirtualFS::f_ReadFileToMemory(NStr::CStr const &_File, NContainer::CByteVector &_Data)
	{
		try
		{
			CVirtualFSFile File(*this, _File, EFileOpen_Read);
			umint Len = File.f_GetLength();
			_Data.f_SetLen(Len);
			File.f_Read(_Data.f_GetArray(), Len);
			return true;
		}
		catch (NFile::CExceptionFile)
		{
		}
		return false;
	}

	bool CVirtualFS::f_FileExists(NStr::CStr const &_Path)
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
			File = _Path;

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

	bool CVirtualFS::f_FileExists(NStr::CStr const &_Path, NFile::EFileAttrib _AttribMask)
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
			File = _Path;

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
			CFile ToFile(*this, _ToFile, EFileOpen_Write | EFileOpen_ShareAll);
			NContainer::CByteVector Data;
			NFile::CFile::fs_WriteStringToVector(Data, LinkDestination);
			ToFile.f_Write(Data.f_GetArray(), Data.f_GetLen());
		}
		else
		{
			NFile::CFile FromFile;
			CFile ToFile(*this, _ToFile, EFileOpen_Write | EFileOpen_ShareAll);
			FromFile.f_Open(_FromFile, EFileOpen_Read | EFileOpen_ShareAll);

			CFileIoTempBuffer Buffer;

			CMibFilePos ToCopy = FromFile.f_GetLength();
			while (ToCopy)
			{
				auto BufferResult = Buffer.f_UseBuffer(ToCopy);

				FromFile.f_Read(BufferResult.m_pBuffer, BufferResult.m_nBytes);
				ToFile.f_Write(BufferResult.m_pBuffer, BufferResult.m_nBytes);
				ToCopy -= BufferResult.m_nBytes;
			}
		}

		f_SetAttributes(_ToFile, FileAttribs);
	}

	void CVirtualFS::f_CopyFileFromFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile)
	{
		NFile::CFile ToFile;
		CFile FromFile(*this, _FromFile, EFileOpen_Read | EFileOpen_ShareAll);
		ToFile.f_Open(_ToFile, EFileOpen_Write | EFileOpen_ShareAll);

		CFileIoTempBuffer Buffer;

		CMibFilePos ToCopy = FromFile.f_GetLength();
		while (ToCopy)
		{
			auto BufferResult = Buffer.f_UseBuffer(ToCopy);

			FromFile.f_Read(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			ToFile.f_Write(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			ToCopy -= BufferResult.m_nBytes;
		}
	}

	void CVirtualFS::f_CopyFileToFS(CVirtualFS &_OtherFS, NStr::CStr const &_FromFile, NStr::CStr const &_ToFile)
	{
		CFile FromFile(_OtherFS, _FromFile, EFileOpen_Read | EFileOpen_ShareAll);
		CFile ToFile(*this, _ToFile, EFileOpen_Write | EFileOpen_ShareAll);

		CFileIoTempBuffer Buffer;

		CMibFilePos ToCopy = FromFile.f_GetLength();
		while (ToCopy)
		{
			auto BufferResult = Buffer.f_UseBuffer(ToCopy);

			FromFile.f_Read(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			ToFile.f_Write(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			ToCopy -= BufferResult.m_nBytes;
		}
	}

	void CVirtualFS::f_CopyFileInFS(NStr::CStr const &_FromFile, NStr::CStr const &_ToFile)
	{
		CFile FromFile(*this, _FromFile, EFileOpen_Read | EFileOpen_ShareAll);
		CFile ToFile(*this, _ToFile, EFileOpen_Write | EFileOpen_ShareAll);

		CFileIoTempBuffer Buffer;

		CMibFilePos ToCopy = FromFile.f_GetLength();
		while (ToCopy)
		{
			auto BufferResult = Buffer.f_UseBuffer(ToCopy);

			FromFile.f_Read(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			ToFile.f_Write(BufferResult.m_pBuffer, BufferResult.m_nBytes);
			ToCopy -= BufferResult.m_nBytes;
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
		_FileSystem.mp_OpenFiles.f_Insert(this);
	}

	CVirtualFS::CFile::~CFile()
	{
		f_Close();
	}

	void CVirtualFS::CFile::fp_CheckValid() const
	{
		if (!f_IsValid())
			DMibErrorFile("File is not open");
	}

	bool CVirtualFS::CFile::f_IsValid() const
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
		mp_Link.f_Unlink();
		mp_Position = 0;
	}

	void CVirtualFS::CFile::f_Open(CVirtualFS &_FileSystem, NStr::CStr const &_FileName, uint32 _OpenFlags)
	{
		f_Close();
		_FileSystem.f_OpenFile(*this, _FileName, _OpenFlags);
		_FileSystem.mp_OpenFiles.f_Insert(this);
	}

	void CVirtualFS::CFile::f_Read(void *_pDest, umint _nBytes)
	{
		fp_CheckValid();
		mp_pInternalFile->f_Read(_pDest, mp_Position, _nBytes);
		mp_Position += _nBytes;
	}

	void CVirtualFS::CFile::f_Write(void const *_pSrc, umint _nBytes)
	{
		fp_CheckValid();
		mp_pInternalFile->f_Write(_pSrc, mp_Position, _nBytes);
		mp_Position += _nBytes;
	}

	void CVirtualFS::CFile::f_Flush(bool _bLocalCacheOnly)
	{
		fp_CheckValid();
		mp_pInternalFile->f_Destroy();
	}

	void CVirtualFS::CFile::f_SetCacheSize(umint _CacheSize)
	{
	}

	bool CVirtualFS::CFile::f_IsAtEndOfFile() const
	{
		fp_CheckValid();
		return mp_Position == mp_pInternalFile->m_FileRecord.m_FileSize;
	}

	void CVirtualFS::CFile::f_SetLength(CMibFilePos const &_Length)
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
		// if (_Pos < 0 || _Pos > mp_pInternalFile->m_FileRecord.m_FileSize)
		// 	DMibErrorFile("Position is outside of file");

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

	bool CVirtualFS::CFile::f_IsValidReadPosition(NStream::CFilePos _Pos) const
	{
		return _Pos >= 0 && _Pos < NStream::CFilePos(mp_pInternalFile->m_FileRecord.m_FileSize);
	}

	NStream::CFilePos CVirtualFS::CFile::f_GetPosition() const
	{
		fp_CheckValid();
		return mp_Position;
	}
}
