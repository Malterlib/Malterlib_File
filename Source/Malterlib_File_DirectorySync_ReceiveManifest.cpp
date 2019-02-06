// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_DirectorySync.h"
#include "Malterlib_File_DirectorySync_ReceiveInternal.h"

namespace NMib::NFile
{
	TCFuture<void> CDirectorySyncReceive::CInternal::f_SyncManifest()
	{
		return f_RSync
			(
				[pConfig = m_pConfig](CRunningSyncState &_RSyncState)
				{
					auto &Config = *pConfig;
					
					CStr PreviousManifestFileName;
					switch (Config.m_PreviousManifest.f_GetTypeID())
					{
					case 0:
						{
							break;
						}
					case 1:
						{
							auto &Manifest = Config.m_PreviousManifest.f_Get<1>();

							PreviousManifestFileName = Config.m_FileOptions.f_TransformFileName
								(
								 	Config.m_TempDirectory
								 	, fg_Format("{}.manifest", fg_RandomID())
								 	, EDirectorySyncStreamType_ManifestSource
								)
							;
							if (!PreviousManifestFileName.f_StartsWith("<Internal>"))
								_RSyncState.m_TempFiles.f_Insert(PreviousManifestFileName);

							CFile::fs_CreateDirectory(Config.m_TempDirectory);

							TCUniquePointer<NStream::CBinaryStream> pStream = Config.m_FileOptions.f_OpenFile
								(
								 	PreviousManifestFileName
								 	, EDirectorySyncStreamType_ManifestSource
								 	, EFileOpen_Write
								)
							;

							*pStream << Manifest;

							break;
						}
					case 2:
						{
							PreviousManifestFileName = Config.m_PreviousManifest.f_Get<2>();
							break;
						}
					default:
						DMibNeverGetHere;
						break;
					}

					CStr ManifestDestination = Config.m_OutputManifestPath;
					if (ManifestDestination.f_IsEmpty())
					{
						ManifestDestination = Config.m_FileOptions.f_TransformFileName
							(
							 	Config.m_TempDirectory
							 	, fg_Format("{}.manifest", fg_RandomID())
							 	, EDirectorySyncStreamType_ManifestDestination
							)
						;
						if (!ManifestDestination.f_StartsWith("<Internal>"))
							_RSyncState.m_TempFiles.f_Insert(ManifestDestination);
					}

					if (!ManifestDestination.f_StartsWith("<Internal>"))
						CFile::fs_CreateDirectory(CFile::fs_GetPath(ManifestDestination));

					if (ManifestDestination != PreviousManifestFileName)
					{
						if (PreviousManifestFileName && (PreviousManifestFileName.f_StartsWith("<Internal>") || CFile::fs_FileExists(PreviousManifestFileName)))
						{
							_RSyncState.m_pSourceStream = Config.m_FileOptions.f_OpenFile
								(
								 	PreviousManifestFileName
								 	, EDirectorySyncStreamType_ManifestSource
								 	, EFileOpen_Read | EFileOpen_ShareAll
								)
							;
						}
						_RSyncState.m_pSourceDestinationStream = Config.m_FileOptions.f_OpenFile
							(
								ManifestDestination
								, EDirectorySyncStreamType_ManifestDestination
								, EFileOpen_Read | EFileOpen_Write | EFileOpen_DontTruncate | EFileOpen_ShareAll
							)
						;
						_RSyncState.m_pClient
							= fg_Construct(*_RSyncState.m_pSourceStream, *_RSyncState.m_pSourceDestinationStream, 256, 4*1024*1024, 8*1024*1024, nullptr, ERSyncClientFlag_TruncateOutput)
						;
					}
					else
					{
						_RSyncState.m_pSourceDestinationStream = Config.m_FileOptions.f_OpenFile
							(
								ManifestDestination
								, EDirectorySyncStreamType_ManifestSourceDestination
								, EFileOpen_Read | EFileOpen_Write | EFileOpen_DontTruncate | EFileOpen_ShareAll
							)
						;
						CStr TempFileName = Config.m_FileOptions.f_TransformFileName
							(
							 	Config.m_TempDirectory
							 	, fg_Format("{}.rsynctemp", fg_RandomID())
							 	, EDirectorySyncStreamType_TempManifestSourceDestination
							)
						;
						if (!TempFileName.f_StartsWith("<Internal>"))
						{
							_RSyncState.m_TempFiles.f_Insert(TempFileName);
							CFile::fs_CreateDirectory(Config.m_TempDirectory);
						}
						_RSyncState.m_pTempStream = Config.m_FileOptions.f_OpenFile
							(
							 	TempFileName
							 	, EDirectorySyncStreamType_TempManifestSourceDestination
							 	, EFileOpen_Read | EFileOpen_Write | EFileOpen_DontTruncate | EFileOpen_ShareAll
							)
						;
						_RSyncState.m_pClient = fg_Construct
							(
							 	*_RSyncState.m_pSourceDestinationStream
							 	, *_RSyncState.m_pSourceDestinationStream
							 	, 256
							 	, 4*1024*1024
							 	, 8*1024*1024
							 	, &*_RSyncState.m_pTempStream
							 	, ERSyncClientFlag_TruncateOutput
							)
						;
					}

					return false;
				}
				, [this](CRunningSyncState &_RSyncState) -> TCFuture<void>
				{
					TCPromise<void> Promise;

					g_Dispatch(m_FileActor) / [pSourceDestinationStream = fg_Move(_RSyncState.m_pSourceDestinationStream)]() mutable
						{
							CDirectoryManifest Manifest;
							pSourceDestinationStream->f_SetPosition(0);
							*pSourceDestinationStream >> Manifest;
							pSourceDestinationStream.f_Clear();
							return Manifest;
						}
						> Promise / [=](CDirectoryManifest &&_Manifest)
						{
							m_pManifest = fg_Construct(fg_Move(_Manifest));
							Promise.f_SetResult();
						}
					;

					return Promise.f_MoveFuture();
				}
				, [this](CActorSubscription &&_Subscription) -> TCFuture<CDirectorySyncClient::FRunRSync>
				{
					return DMibCallActor
						(
							m_Client
							, CDirectorySyncClient::f_StartManifestRSync
							, fg_Move(_Subscription)
						)
					;
				}
			)
		;
	}
}
