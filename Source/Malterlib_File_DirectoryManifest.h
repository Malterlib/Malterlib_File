// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#include <Mib/Core/Core>
#include <Mib/Concurrency/ConcurrencyManager>
#include <Mib/Concurrency/DistributedActor>
#include <Mib/Cryptography/Hashes/SHA>
#include <Mib/Storage/Optional>

namespace NMib::NFile
{
	enum EDirectoryManifestSyncFlag /// \brief Flag for how to sync specific files
	{
		EDirectoryManifestSyncFlag_None = 0							///< Normal syncing. In this case the rsync is used for syncing changes
		, EDirectoryManifestSyncFlag_Append = DMibBit(0)			///< Append syncing. Any changes are assumed to be append only
		, EDirectoryManifestSyncFlag_TransactionLog = DMibBit(1)	///< Should be used together with ESyncFlag_Append. This tells the backup manager to sync
																	///		writes to disk as quickly as possible.
	};
	
	struct CDirectoryManifestFile
	{
		bool operator == (CDirectoryManifestFile const &_Right) const;
		
		NStr::CStr const &f_GetFileName() const;
		bool f_IsDirectory() const;
		bool f_IsFile() const;
		
		template <typename tf_CStream>
		void f_Stream(tf_CStream &_Stream, uint32 _Version);

		static EDirectoryManifestSyncFlag fs_ParseSyncFlags(NEncoding::CEJSON const &_JSON);
		static NEncoding::CEJSON fs_GenerateSyncFlags(EDirectoryManifestSyncFlag _Flags);

		NCryptography::CHashDigest_SHA256 m_Digest;
		uint64 m_Length = 0;
		NTime::CTime m_WriteTime;
		NStr::CStr m_OriginalPath;
		NStr::CStr m_SymlinkData;
		NStr::CStr m_Owner;
		NStr::CStr m_Group;
		NFile::EFileAttrib m_Attributes = NFile::EFileAttrib_None;
		EDirectoryManifestSyncFlag m_Flags = EDirectoryManifestSyncFlag_None;
	};

	struct CDirectoryManifestConfig
	{
		using CDestination = NStorage::TCOptional<NStr::CStr>;

		enum
		{
			EManifestConfigStreamVersion = 0x101
		};
		
		static NStr::CStr fs_ParseWildcard(NStr::CStr const &_Wildcard, bool &o_bRecursive);
		CDirectoryManifestConfig const &f_Validate() const;
		void f_AppendConfig(CDirectoryManifestConfig const &_Config);

		template <typename tf_CStream>
		void f_Stream(tf_CStream &_Stream);
		
		NStr::CStr m_Root;																		///< The root directory of the backup.
		NContainer::TCMap<NStr::CStr, CDestination> m_IncludeWildcards = {{"^*", {}}};				///< \brief Relative to m_Root. This is a file search. Only file name can have wildcards.
																								///		Use ^ in the beginning of the file path to create a recursive search. Value is used
																								///		for specifying the destination of the files in the manifest.
																								///		e.g. {"Folder1/^*", "Folder2"} will place all files from Folder1 in Folder2.
		NContainer::TCSet<NStr::CStr> m_ExcludeWildcards;										///< Relative to m_Root. Evaluated after include wild cards as a filtering step on the
																								///		destination path. In remapped space.
		NContainer::TCMap<NStr::CStr, EDirectoryManifestSyncFlag> m_AddSyncFlagsWildcards;		///< Relative to m_Root. In remapped space.
		NContainer::TCMap<NStr::CStr, EDirectoryManifestSyncFlag> m_RemoveSyncFlagsWildcards;	///< Relative to m_Root. Evaluated after m_AddSyncFlagsWildcards. In remapped space.
	};
	
	struct CDirectoryManifest
	{
		template <typename tf_CStream>
		void f_Stream(tf_CStream &_Stream);
		
		NEncoding::CEJSON f_ToJson() const;
		static CDirectoryManifest fs_FromJson(NEncoding::CEJSON const &_JSON);
		
		enum
		{
			EManifestStreamVersion = 0x102
		};
		
		NContainer::TCMap<NStr::CStr, CDirectoryManifestFile> m_Files;
		
		static EDirectoryManifestSyncFlag fs_GetSyncFlags(CDirectoryManifestConfig const &_Config, NStr::CStr const &_FileName);
		
		static void fs_UpdateManifestFile
			(
				CDirectoryManifestConfig const &_Config
				, NStr::CStr const &_FileName
				, CDirectoryManifestFile &o_ManifestFile
				, NStr::CStr const &_OriginalPath
				, NFile::CFile::CFileChecksumState_SHA256 *o_pState = nullptr
				, NFile::EFileOpen _FileOpenFlags = NFile::EFileOpen_None
				, CDirectoryManifestFile const *_pPreviousManifestFile = nullptr
			)
		;
		static CDirectoryManifest fs_GetManifest
			(
				CDirectoryManifestConfig const &_Config
				, NFunction::TCFunctionNoAlloc<void ()> const &_fCheckAbort
				, NContainer::TCMap<NStr::CStr, NFile::CFile::CFileChecksumState_SHA256> *o_pAppendStates = nullptr
				, NFile::EFileOpen _FileOpenFlags = NFile::EFileOpen_None
				, CDirectoryManifest const *_pPreviousManifest = nullptr
			)
		;
	};
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif

#include "Malterlib_File_DirectoryManifest.hpp"
