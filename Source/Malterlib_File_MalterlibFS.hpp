// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once

namespace NMib::NFile
{
	inline CVirtualFS::CDirectoryID CVirtualFS::fp_GetDirectoryID()
	{
		CDirectoryID DirectoryID = ++mp_RootData.m_CurrentDirectoryID;
		fp_SaveRootData();
		return DirectoryID;
	}

	inline CVirtualFS::CFilePos CVirtualFS::fp_GetFilePos(CClusterID _Cluster)
	{
		return mp_RootData.m_ClusterSize * _Cluster;
	}

	inline CVirtualFS::CClusterID CVirtualFS::fp_GetClusterID(CFilePos _Position, CFilePos &_Remaining)
	{
		CClusterID Cluster = _Position / mp_RootData.m_ClusterSize;
		_Remaining = _Position - Cluster * mp_RootData.m_ClusterSize;
		return Cluster;
	}

	inline CVirtualFS::CClusterID const &CVirtualFS::CClusterCacheEntry::CCompare::operator () (CClusterCacheEntry const &_Node) const
	{
		return _Node.m_ClusterID;
	}

	inline NStr::CStr const &CVirtualFS::CDirectoryCacheEntry::CCompare::operator () (CDirectoryCacheEntry const &_Node) const
	{
		return _Node.m_FullPath;
	}

	inline CVirtualFS::CClusterID const &CVirtualFS::CFileInternal::CCompare::operator () (CFileInternal const &_Node) const
	{
		return _Node.m_FileDescriptorID;
	}

	template <typename t_CStreamType>
	TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::TCBinaryStream_VirtualFSFilePtr()
	{
		mp_pFile = nullptr;
	}

	template <typename t_CStreamType>
	TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::TCBinaryStream_VirtualFSFilePtr(CVirtualFSFile *_pFile)
	{
		mp_pFile = _pFile;
	}

	template <typename t_CStreamType>
	TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::TCBinaryStream_VirtualFSFilePtr(CVirtualFSFile &_File)
	{
		mp_pFile = &_File;
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_Open(CVirtualFSFile *_pFile)
	{
		mp_pFile = _pFile;
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_Open(CVirtualFSFile &_File)
	{
		mp_pFile = &_File;
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_Reset()
	{
		DMibMemLightweightTrackDisableScope;
		mp_pFile->f_SetLength(0);
		mp_pFile->f_Flush(false);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_Flush(bool _bLocalCacheOnly)
	{
		DMibMemLightweightTrackDisableScope;
		mp_pFile->f_Flush(_bLocalCacheOnly);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_SetCacheSize(umint _CacheSize)
	{
		DMibMemLightweightTrackDisableScope;
		mp_pFile->f_SetCacheSize(_CacheSize);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_FeedBytes(void const *_pMem, umint _nBytes)
	{
		DMibMemLightweightTrackDisableScope;
		mp_pFile->f_Write(_pMem, _nBytes);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_ConsumeBytes(void *_pMem, umint _nBytes)
	{
		DMibMemLightweightTrackDisableScope;
		mp_pFile->f_Read(_pMem, _nBytes);
	}

	template <typename t_CStreamType>
	bool TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_IsValid() const
	{
		DMibMemLightweightTrackDisableScope;
		return mp_pFile->f_IsValid();
	}

	template <typename t_CStreamType>
	bool TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_IsAtEndOfStream() const
	{
		DMibMemLightweightTrackDisableScope;
		return mp_pFile->f_IsAtEndOfFile();
	}

	template <typename t_CStreamType>
	NStream::CFilePos TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_GetPosition() const
	{
		DMibMemLightweightTrackDisableScope;
		return mp_pFile->f_GetPosition();
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_SetPosition(NStream::CFilePos _Pos)
	{
		DMibMemLightweightTrackDisableScope;
		mp_pFile->f_SetPosition(_Pos);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_SetPositionFromEnd(NStream::CFilePos _Pos)
	{
		DMibMemLightweightTrackDisableScope;
		mp_pFile->f_SetPositionFromEnd(_Pos);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_AddPosition(NStream::CFilePos _Pos)
	{
		DMibMemLightweightTrackDisableScope;
		mp_pFile->f_AddPosition(_Pos);
	}

	template <typename t_CStreamType>
	bool TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_IsValidReadPosition(NStream::CFilePos _Pos) const
	{
		DMibMemLightweightTrackDisableScope;
		return mp_pFile->f_IsValidReadPosition(_Pos);
	}

	template <typename t_CStreamType>
	NStream::CFilePos TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_GetLength() const
	{
		DMibMemLightweightTrackDisableScope;
		return mp_pFile->f_GetLength();
	}

	template <typename t_CStreamType>
	umint TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_ContainerLengthLimit() const
	{
		return NStream::fg_CapLengthLimit(f_GetLength() - f_GetPosition());
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFilePtr<t_CStreamType>::f_SetLength(NStream::CFilePos _Length)
	{
		DMibMemLightweightTrackDisableScope;
		return mp_pFile->f_SetLength(_Length);
	}

	template <typename t_CStreamType>
	TCBinaryStream_VirtualFSFile<t_CStreamType>::TCBinaryStream_VirtualFSFile()
	{
	}

	template <typename t_CStreamType>
	TCBinaryStream_VirtualFSFile<t_CStreamType>::~TCBinaryStream_VirtualFSFile()
	{
		DMibMemLightweightTrackDisableScope;
		mp_File.f_Close();
	}

	template <typename t_CStreamType>
	CVirtualFSFile &TCBinaryStream_VirtualFSFile<t_CStreamType>::f_GetVFSFile()
	{
		return mp_File;
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFile<t_CStreamType>::f_Reset()
	{
		DMibMemLightweightTrackDisableScope;
		mp_File.f_SetLength(0);
		mp_File.f_Flush(false);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFile<t_CStreamType>::f_Flush(bool _bLocalCacheOnly)
	{
		DMibMemLightweightTrackDisableScope;
		mp_File.f_Flush(_bLocalCacheOnly);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFile<t_CStreamType>::f_SetCacheSize(umint _CacheSize)
	{
		DMibMemLightweightTrackDisableScope;
		mp_File.f_SetCacheSize(_CacheSize);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFile<t_CStreamType>::f_FeedBytes(void const *_pMem, umint _nBytes)
	{
		DMibMemLightweightTrackDisableScope;
		mp_File.f_Write(_pMem, _nBytes);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFile<t_CStreamType>::f_ConsumeBytes(void *_pMem, umint _nBytes)
	{
		DMibMemLightweightTrackDisableScope;
		mp_File.f_Read(_pMem, _nBytes);
	}

	template <typename t_CStreamType>
	bool TCBinaryStream_VirtualFSFile<t_CStreamType>::f_IsValid() const
	{
		DMibMemLightweightTrackDisableScope;
		return mp_File.f_IsValid();
	}

	template <typename t_CStreamType>
	bool TCBinaryStream_VirtualFSFile<t_CStreamType>::f_IsAtEndOfStream() const
	{
		DMibMemLightweightTrackDisableScope;
		return mp_File.f_IsAtEndOfFile();
	}

	template <typename t_CStreamType>
	NStream::CFilePos TCBinaryStream_VirtualFSFile<t_CStreamType>::f_GetPosition() const
	{
		DMibMemLightweightTrackDisableScope;
		return mp_File.f_GetPosition();
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFile<t_CStreamType>::f_SetPosition(NStream::CFilePos _Pos)
	{
		DMibMemLightweightTrackDisableScope;
		mp_File.f_SetPosition(_Pos);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFile<t_CStreamType>::f_SetPositionFromEnd(NStream::CFilePos _Pos)
	{
		DMibMemLightweightTrackDisableScope;
		mp_File.f_SetPositionFromEnd(_Pos);
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFile<t_CStreamType>::f_AddPosition(NStream::CFilePos _Pos)
	{
		DMibMemLightweightTrackDisableScope;
		mp_File.f_AddPosition(_Pos);
	}

	template <typename t_CStreamType>
	bool TCBinaryStream_VirtualFSFile<t_CStreamType>::f_IsValidReadPosition(NStream::CFilePos _Pos) const
	{
		DMibMemLightweightTrackDisableScope;
		return mp_File.f_IsValidReadPosition(_Pos);
	}

	template <typename t_CStreamType>
	NStream::CFilePos TCBinaryStream_VirtualFSFile<t_CStreamType>::f_GetLength() const
	{
		DMibMemLightweightTrackDisableScope;
		return mp_File.f_GetLength();
	}

	template <typename t_CStreamType>
	umint TCBinaryStream_VirtualFSFile<t_CStreamType>::f_ContainerLengthLimit() const
	{
		return NStream::fg_CapLengthLimit(f_GetLength() - f_GetPosition());
	}

	template <typename t_CStreamType>
	void TCBinaryStream_VirtualFSFile<t_CStreamType>::f_SetLength(NStream::CFilePos _Length)
	{
		DMibMemLightweightTrackDisableScope;
		return mp_File.f_SetLength(_Length);
	}
}
