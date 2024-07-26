// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

namespace NMib::NFile
{
	template <typename tf_CStream>
	void CDirectoryManifestConfig::f_Stream(tf_CStream &_Stream, uint32 _Version)
	{
		uint32 Version = _Version;
		_Stream % Version;
		if (Version < EManifestConfigStreamVersion_Min || Version > EManifestConfigStreamVersion_Current)
			DMibError("Invalid directory manifest version");
		DMibBinaryStreamVersion(_Stream, Version);
		
		_Stream % m_Root;
		_Stream % m_IncludeWildcards;
		_Stream % m_ExcludeWildcards;
		_Stream % m_AddSyncFlagsWildcards;
		_Stream % m_RemoveSyncFlagsWildcards;
		if (Version >= EManifestConfigStreamVersion_SupportFlagsAndMaxDigestSize)
		{
			_Stream % m_MaxDigestSize;
			_Stream % m_Flags;
		}
	}
	
	template <typename tf_CStream>
	void CDirectoryManifestFile::f_Stream(tf_CStream &_Stream, uint32 _Version)
	{
		if (_Version < CDirectoryManifest::EManifestStreamVersion_Min || _Version > CDirectoryManifest::EManifestStreamVersion_Current)
			DMibError("Invalid directory manifest version");

		if (_Version >= CDirectoryManifest::EManifestStreamVersion_OptionalDigest)
			_Stream % m_Digest;
		else
		{
			if constexpr (tf_CStream::mc_bFeed)
			{
				NCryptography::CHashDigest_SHA256 Digest;
				if (m_Digest)
					Digest = *m_Digest;
				_Stream << Digest;
			}
			else
			{
				NCryptography::CHashDigest_SHA256 Digest;
				_Stream >> Digest;
				m_Digest = Digest;
			}
		}

		_Stream % m_Length;
		_Stream % m_WriteTime;
		_Stream % m_OriginalPath;
		_Stream % m_SymlinkData;
		_Stream % m_Owner;
		_Stream % m_Group;
		_Stream % m_Attributes;
		_Stream % m_Flags;
	}
	
	template <typename tf_CStream>
	void CDirectoryManifest::f_Stream(tf_CStream &_Stream, uint32 _Version)
	{
		uint32 Version = _Version;
		_Stream % Version;
		if (Version < EManifestStreamVersion_Min || Version > EManifestStreamVersion_Current)
			DMibError("Invalid directory manifest version");
		DMibBinaryStreamVersion(_Stream, Version);

		if constexpr (tf_CStream::mc_bFeed)
		{
			uint32 nFiles = m_Files.f_GetLen();
			_Stream << nFiles;
			for (auto &File : m_Files)
			{
				_Stream << m_Files.fs_GetKey(File);
				File.f_Stream(_Stream, Version);
			}
		}
		else
		{
			uint32 nFiles;
			_Stream >> nFiles;
			while (nFiles--)
			{
				NStr::CStr Key;
				_Stream >> Key;
				auto &File = m_Files[Key];
				File.f_Stream(_Stream, Version);
			}
		}
	}

	template <typename tf_CStream>
	void CDirectoryManifestLatestVersion::f_Stream(tf_CStream &_Stream)
	{
		CDirectoryManifest::f_Stream(_Stream, EManifestStreamVersion_Current);
	}
}
