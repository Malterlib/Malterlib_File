// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_DirectorySync.h"
#include "Malterlib_File_DirectorySync_ReceiveInternal.h"

namespace NMib::NFile
{
	TCFuture<void> CDirectorySyncReceive::CInternal::f_SyncManifest()
	{
		auto InterfaceVersion = m_Client->f_InterfaceVersion();

		auto ClientFlags = ERSyncFlag_ClientTruncateOutput;
		if (InterfaceVersion >= CDirectorySyncClient::EProtocolVersion_UseSHA256)
			ClientFlags |= ERSyncFlag_UseSHA256;

		CDirectoryManifest::EManifestStreamVersion ManifestVersion = CDirectoryManifest::EManifestStreamVersion_Min;

		if (InterfaceVersion >= CDirectorySyncClient::EProtocolVersion_OptionalDigest)
			ManifestVersion = CDirectoryManifest::EManifestStreamVersion_OptionalDigest;

		return f_RSync
			(
				[pConfig = m_pConfig, ClientFlags, ManifestVersion](CRunningSyncState *_pRSyncState)
				{
					auto &Config = *pConfig;
					auto &RSyncState = *_pRSyncState;
					
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
								RSyncState.m_TempFiles.f_Insert(PreviousManifestFileName);

							CFile::fs_CreateDirectory(Config.m_TempDirectory);

							TCUniquePointer<NStream::CBinaryStream> pStream = Config.m_FileOptions.f_OpenFile
								(
									PreviousManifestFileName
									, EDirectorySyncStreamType_ManifestSource
									, EFileOpen_Write
								)
							;

							Manifest.f_Stream(fg_FeedStream(*pStream), ManifestVersion);

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
							RSyncState.m_TempFiles.f_Insert(ManifestDestination);
					}

					if (!ManifestDestination.f_StartsWith("<Internal>"))
						CFile::fs_CreateDirectory(CFile::fs_GetPath(ManifestDestination));

					if (ManifestDestination != PreviousManifestFileName)
					{
						if (PreviousManifestFileName && (PreviousManifestFileName.f_StartsWith("<Internal>") || CFile::fs_FileExists(PreviousManifestFileName)))
						{
							RSyncState.m_pSourceStream = Config.m_FileOptions.f_OpenFile
								(
									PreviousManifestFileName
									, EDirectorySyncStreamType_ManifestSource
									, EFileOpen_Read | EFileOpen_ShareAll
								)
							;
						}
						RSyncState.m_pSourceDestinationStream = Config.m_FileOptions.f_OpenFile
							(
								ManifestDestination
								, EDirectorySyncStreamType_ManifestDestination
								, EFileOpen_Read | EFileOpen_Write | EFileOpen_DontTruncate | EFileOpen_ShareAll
							)
						;
						RSyncState.m_pClient
							= fg_Construct(*RSyncState.m_pSourceStream, *RSyncState.m_pSourceDestinationStream, 256, 4*1024*1024, 8*1024*1024, nullptr, ClientFlags)
						;
					}
					else
					{
						RSyncState.m_pSourceDestinationStream = Config.m_FileOptions.f_OpenFile
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
							RSyncState.m_TempFiles.f_Insert(TempFileName);
							CFile::fs_CreateDirectory(Config.m_TempDirectory);
						}
						RSyncState.m_pTempStream = Config.m_FileOptions.f_OpenFile
							(
								TempFileName
								, EDirectorySyncStreamType_TempManifestSourceDestination
								, EFileOpen_Read | EFileOpen_Write | EFileOpen_DontTruncate | EFileOpen_ShareAll
							)
						;
						RSyncState.m_pClient = fg_Construct
							(
								*RSyncState.m_pSourceDestinationStream
								, *RSyncState.m_pSourceDestinationStream
								, 256
								, 4*1024*1024
								, 8*1024*1024
								, &*RSyncState.m_pTempStream
								, ClientFlags
							)
						;
					}

					return false;
				}
				, [pThisUnsafe = this, ManifestVersion](CRunningSyncState *_pRSyncState) -> TCFuture<void>
				{
					co_await NConcurrency::ECoroutineFlag_AllowReferences;
					auto pThis = pThisUnsafe;

					auto BlockingActorCheckout = fg_BlockingActor();
					CDirectoryManifest Manifest = co_await
						(
							g_Dispatch(BlockingActorCheckout) / [pSourceDestinationStream = fg_Move(_pRSyncState->m_pSourceDestinationStream), ManifestVersion]() mutable
							{
								CDirectoryManifest Manifest;
								pSourceDestinationStream->f_SetPosition(0);
								Manifest.f_Stream(fg_ConsumeStream(*pSourceDestinationStream), ManifestVersion);
								pSourceDestinationStream.f_Clear();
								return Manifest;
							}
						)
					;
					pThis->m_pManifest = fg_Construct(fg_Move(Manifest));

					co_return {};
				}
				, [this](CActorSubscription &&_Subscription) -> TCFuture<CDirectorySyncClient::FRunRSync>
				{
					return g_Future <<= m_Client.f_CallActor(&CDirectorySyncClient::f_StartManifestRSync)(fg_Move(_Subscription));
				}
			)
		;
	}
}
