// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#include <Mib/Core/Core>
#include <Mib/Concurrency/ConcurrencyManager>
#include <Mib/Concurrency/DistributedActor>
#include <Mib/Storage/Variant>
#include <Mib/Storage/Optional>

#include "Malterlib_File_DirectoryManifest.h"

namespace NMib::NFile
{
	enum EDirectorySyncStreamType : uint32
	{
		EDirectorySyncStreamType_None                            = DMibBit(0)
		, EDirectorySyncStreamType_Source                        = DMibBit(1)
		, EDirectorySyncStreamType_Destination                   = DMibBit(2)
		, EDirectorySyncStreamType_Temp                          = DMibBit(3)
		, EDirectorySyncStreamType_Manifest                      = DMibBit(4)
		, EDirectorySyncStreamType_SourceDestination             = EDirectorySyncStreamType_Source   | EDirectorySyncStreamType_Destination
		, EDirectorySyncStreamType_ManifestSource                = EDirectorySyncStreamType_Manifest | EDirectorySyncStreamType_Source
		, EDirectorySyncStreamType_ManifestDestination           = EDirectorySyncStreamType_Manifest | EDirectorySyncStreamType_Destination
		, EDirectorySyncStreamType_ManifestSourceDestination     = EDirectorySyncStreamType_Manifest | EDirectorySyncStreamType_SourceDestination
		, EDirectorySyncStreamType_TempManifestSource            = EDirectorySyncStreamType_Temp     | EDirectorySyncStreamType_ManifestSource
		, EDirectorySyncStreamType_TempManifestDestination       = EDirectorySyncStreamType_Temp     | EDirectorySyncStreamType_ManifestDestination
		, EDirectorySyncStreamType_TempManifestSourceDestination = EDirectorySyncStreamType_Temp     | EDirectorySyncStreamType_ManifestSourceDestination
		, EDirectorySyncStreamType_TempSourceDestination         = EDirectorySyncStreamType_Temp     | EDirectorySyncStreamType_SourceDestination
	};

	struct CDirectorySyncClient : public NConcurrency::CActor
	{
		enum : uint32 
		{
			EProtocolVersion_Min = 0x101
			, EProtocolVersion_UseSHA256 = 0x102
			, EProtocolVersion_OptionalDigest = 0x103
			, EProtocolVersion_Current = 0x103
		};

		using FRunRSync = NConcurrency::TCActorFunctorWithID<NConcurrency::TCFuture<NContainer::CSecureByteVector> (NContainer::CSecureByteVector _Packet)>;
		
		CDirectorySyncClient();
		
		virtual NConcurrency::TCFuture<FRunRSync> f_StartManifestRSync(NConcurrency::TCActorSubscriptionWithID<> _Subscription) = 0;
		virtual NConcurrency::TCFuture<FRunRSync> f_StartRSync(NStr::CStr _FileName, NConcurrency::TCActorSubscriptionWithID<> _Subscription) = 0;
		virtual NConcurrency::TCFuture<void> f_Finished() = 0;
	};
	
	struct CDirectorySyncStats
	{
		template <typename tf_CStream>
		void f_Stream(tf_CStream &_Stream);

		fp64 f_IncomingBytesPerSecond() const;
		fp64 f_OutgoingBytesPerSecond() const;

		template <typename tf_CStr>
		void f_Format(tf_CStr &o_Str) const;

		uint64 m_nSyncedFiles = 0;
		uint64 m_OutgoingBytes = 0;
		uint64 m_IncomingBytes = 0;
		fp64 m_nSeconds = 0.0;
	};

	struct CDirectorySyncFileOptions
	{
		NStorage::TCUniquePointer<NStream::CBinaryStream> f_OpenFile
			(
				NStr::CStr const &_FileName
				, EDirectorySyncStreamType _FileType
				, EFileOpen _OpenFlags
				, EFileAttrib _Attributes = EFileAttrib_None
			)
		; ///< Opens _FileName. If m_fOpenStream is set, use it

		/// Utility function that produces a file path. If m_fFilePathRenamer is set, it will be called, otherwise _BasePath and _FileName is concatenated
		NStr::CStr f_TransformFileName(NStr::CStr const &_BasePath, NStr::CStr const &_FileName, EDirectorySyncStreamType _FileType);

		NFunction::TCFunctionMutable
			<
				NStorage::TCUniquePointer<NStream::CBinaryStream>
				(
					NStr::CStr const &_FileName
					, EDirectorySyncStreamType _FileType
					, NFile::EFileOpen _OpenFlags
					, NFile::EFileAttrib _Attributes
				)
			>
			m_fOpenStream
		; ///< If set, this functor is called to produce a stream that is used instead of the default file stream

		NFunction::TCFunctionMutable<NStr::CStr (NStr::CStr const &_BasePath, NStr::CStr const &_FileName, EDirectorySyncStreamType _FileType)>
			m_fTransformFilePath
		;	///< If set, this functor is called to transform the file name used for file operations. Return a string starting <Internal> to specify that file
			///	operations should not be performed for manifest files
	};
	
	struct CDirectorySyncSend : public CDirectorySyncClient
	{
		struct CConfig
		{
			CConfig(); ///< Default constructor

			CConfig(NStr::CStr const &_FileName); ///< Constructor for the case when only a single file is transferred

			NStr::CStr m_BasePath;	///< Care has been taken to make sure that no files outside this directory can be downloaded. Also, only files that are specified in
									///		the manifst can be downloaded.
			NStorage::TCVariant<CDirectoryManifest, CDirectoryManifestConfig, NStr::CStr> m_Manifest;	///< Specify manifest, a config to generate a manifest, or a path to a file with
																										///		an existing manifest that corresponds to the files available in m_BasePath.
			NStorage::TCOptional<bool> m_bUseOriginalLocation;	///< Use the original location of files in manifest. Defaults to true when m_Manifest is a CDirectoryManifestConfig,
																///		otherwise false
			CDirectorySyncFileOptions m_FileOptions; ///< Functionality for transformation of file names and content
		};

		struct CSyncResult
		{
			template <typename tf_CStream>
			void f_Stream(tf_CStream &_Stream);

			CDirectorySyncStats m_Stats;
			bool m_bFinished = false;
		};
		
		CDirectorySyncSend(CConfig &&_Config);
		~CDirectorySyncSend();
		
		NConcurrency::TCFuture<CSyncResult> f_GetResult();

	private:
		struct CInternal;

		NConcurrency::TCFuture<FRunRSync> f_StartManifestRSync(NConcurrency::TCActorSubscriptionWithID<> _Subscription) override;
		NConcurrency::TCFuture<FRunRSync> f_StartRSync(NStr::CStr _FileName, NConcurrency::TCActorSubscriptionWithID<> _Subscription) override;
		NConcurrency::TCFuture<void> f_Finished() override;

		NConcurrency::TCFuture<void> fp_Destroy() override;
		
		NStorage::TCUniquePointer<CInternal> mp_pInternal;
	};

	struct CDirectorySyncReceive : public NConcurrency::CActor
	{
		enum EExcessFilesAction : uint32
		{
			EExcessFilesAction_Ignore = 0
			, EExcessFilesAction_Fail
			, EExcessFilesAction_Delete
		};

		enum ESyncFlag : uint32
		{
			ESyncFlag_None = 0
			, ESyncFlag_WriteTime = DMibBit(0)
			, ESyncFlag_Owner = DMibBit(1)
			, ESyncFlag_Group = DMibBit(2)
			, ESyncFlag_Attributes = DMibBit(3)
		};

		enum EEasyConfigFlag : uint32
		{
			EEasyConfigFlag_None = 0
			, EEasyConfigFlag_AllowOverwrite = DMibBit(0)
			, EEasyConfigFlag_DestinationIsDirectory = DMibBit(1)
		};
		
		struct CConfig
		{
			CConfig(); ///< Default constructor

			CConfig(NStr::CStr const &_Destination, EEasyConfigFlag _Flags = EEasyConfigFlag_None); ///< Constructor for the case when only a single file is transferred, or when only a destination directory is specified.

			NStorage::TCVariant<void, CDirectoryManifest, NStr::CStr> m_PreviousManifest;
			NStr::CStr m_OutputManifestPath;

			NStr::CStr m_TempDirectory = NFile::CFile::fs_GetTemporaryDirectory(); ///< Used to create temporary files used during RSync
			
			NStr::CStr m_BasePath;				///< The base path for destination files
			NStr::CStr m_PreviousBasePath;		///< The location to use for fallback source for rsync comparison
			
			NContainer::TCSet<NStr::CStr> m_IncludeWildcards; ///< Relative to m_BasePath. Leave empty to include all files in manifest
			NContainer::TCSet<NStr::CStr> m_ExcludeWildcards; ///< Relative to m_BasePath. Evaluated after include wild cards as a filtering step.

			CDirectorySyncFileOptions m_FileOptions; ///< Functionality for transformation of file names and content

			EExcessFilesAction m_ExcessFilesAction = EExcessFilesAction_Ignore;
			ESyncFlag m_SyncFlags = ESyncFlag_WriteTime | ESyncFlag_Owner | ESyncFlag_Group | ESyncFlag_Attributes;
			
			uint32 m_QueueSize = NFile::gc_IdealNetworkQueueSize;
			uint32 m_RSyncConcurrency = 8; ///< Number of simultaneous rsyncs to run.
		};
		
		struct CSyncResult
		{
			CDirectoryManifest m_Manifest;
			CDirectorySyncStats m_Stats;
		};
		
		CDirectorySyncReceive(CConfig &&_Config, NConcurrency::TCDistributedActorInterface<CDirectorySyncClient> &&_Client);
		~CDirectorySyncReceive();
		
		NConcurrency::TCFuture<CSyncResult> f_PerformSync();
		
	private:
		struct CInternal;

		NConcurrency::TCFuture<void> fp_Destroy() override;
		
		NStorage::TCUniquePointer<CInternal> mp_pInternal;
	};
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif

#include "Malterlib_File_DirectorySync.hpp"

