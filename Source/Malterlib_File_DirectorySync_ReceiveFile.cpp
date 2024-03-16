// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_DirectorySync.h"
#include "Malterlib_File_DirectorySync_ReceiveInternal.h"

namespace NMib::NFile
{
#if DMibConfig_Tests_Enable
	constinit NAtomic::TCAtomic<pfp32> g_TestDirectorySyncSourceOpenDelay = 0.0f;
#endif

	TCFuture<void> CDirectorySyncReceive::CInternal::f_HandleExcessFiles()
	{
		if (m_pConfig->m_ExcessFilesAction == EExcessFilesAction_Ignore)
			co_return {};

		auto BlockingActorCheckout = fg_BlockingActor();
		co_await
			(
				g_Dispatch(BlockingActorCheckout) / [pConfig = m_pConfig, pManifest = m_pManifest, pDestroyed = m_pDestroyed]
				{
					auto &Config = *pConfig;
					auto &Manifest = *pManifest;

					CFile::CFindFilesOptions Options{Config.m_BasePath + "/*", true};
					Options.m_bFollowLinks = false;
					Options.m_fCheckAbort = [&]{ CInternal::fs_CheckDestroy(pDestroyed); };
					Options.m_AttribMask = EFileAttrib_Directory | EFileAttrib_File | EFileAttrib_Link;

					for (auto &File : CFile::fs_FindFiles(Options))
					{
						CStr RelativePath = File.m_Path.f_Extract(Config.m_BasePath.f_GetLen() + 1);
						if (Manifest.m_Files.f_FindEqual(RelativePath))
							continue;

						switch (Config.m_ExcessFilesAction)
						{
						case EExcessFilesAction_Fail:
							{
								DMibError(fg_Format("Found execess file in directory sync destination: {}", RelativePath));
								break;
							}
						case EExcessFilesAction_Delete:
							{
								if (File.m_Attribs & (EFileAttrib_Link | EFileAttrib_File))
									CFile::fs_DeleteFile(File.m_Path);
								else
									CFile::fs_DeleteDirectoryRecursive(File.m_Path);
								break;
							}
						case EExcessFilesAction_Ignore:
							break;
						}
					}
				}
			)
		;
		
		co_return {};
	}
	
	TCFuture<void> CDirectorySyncReceive::CInternal::f_SyncFile(CStr const &_FileName)
	{
		auto ClientFlags = ERSyncFlag_ClientTruncateOutput;
		if (m_Client->f_InterfaceVersion() >= CDirectorySyncClient::EProtocolVersion_UseSHA256)
			ClientFlags |= ERSyncFlag_UseSHA256;

		return f_RSync
			(
				[_FileName, pConfig = m_pConfig, pManifest = m_pManifest, ClientFlags](CRunningSyncState *_pRSyncState) -> bool
				{
					auto &Config = *pConfig;
					auto &RSyncState = *_pRSyncState;

					auto *pManifestFile = pManifest->m_Files.f_FindEqual(_FileName);
					if (!pManifestFile)
						return true;

					CStr Destination = Config.m_FileOptions.f_TransformFileName(Config.m_BasePath, _FileName, EDirectorySyncStreamType_Destination);
					RSyncState.m_DestinationFilename = Destination;

					auto &ManifestFile = *pManifestFile;
					
					if (ManifestFile.m_Attributes & EFileAttrib_Link)
					{
						if (CFile::fs_FileExists(Destination, EFileAttrib_Link))
						{
							if (CFile::fs_ResolveSymbolicLink(Destination) == ManifestFile.m_SymlinkData)
								return true;
							CFile::fs_DeleteFile(Destination);
						}
						else if (CFile::fs_FileExists(Destination, EFileAttrib_Directory))
							CFile::fs_DeleteDirectoryRecursive(Destination);
						else if (CFile::fs_FileExists(Destination))
							CFile::fs_DeleteFile(Destination);
						
						ESymbolicLinkFlag Flags = ESymbolicLinkFlag_AllowEmulation;
						
						if (!CFile::fs_IsPathAbsolute(ManifestFile.m_SymlinkData))
							Flags |= ESymbolicLinkFlag_Relative;
						
						CFile::fs_CreateSymbolicLink(ManifestFile.m_SymlinkData, Destination, ManifestFile.m_Attributes, Flags);
						
						return true;
					}
					else if (ManifestFile.m_Attributes & EFileAttrib_Directory)
					{
						if (CFile::fs_FileExists(Destination, EFileAttrib_Link | EFileAttrib_File))
							CFile::fs_DeleteFile(Destination);

						CFile::fs_CreateDirectory(Destination);

						return true;
					}
					
					if (CFile::fs_FileExists(Destination, EFileAttrib_Link))
						CFile::fs_DeleteFile(Destination);
					else if (CFile::fs_FileExists(Destination, EFileAttrib_Directory))
						CFile::fs_DeleteDirectoryRecursive(Destination);
					
					CStr Source;
					if (CFile::fs_FileExists(Destination))
					{
						auto Stream = Config.m_FileOptions.f_OpenFile(Destination, EDirectorySyncStreamType_Destination, EFileOpen_Read | EFileOpen_ShareAll);
						if (NCryptography::CHash_SHA256::fs_DigestFromStream(*Stream) == ManifestFile.m_Digest)
							return true;
						Source = Config.m_FileOptions.f_TransformFileName(Config.m_BasePath, _FileName, EDirectorySyncStreamType_Source);
					}
					else if (Config.m_PreviousBasePath)
						Source = Config.m_FileOptions.f_TransformFileName(Config.m_PreviousBasePath, _FileName, EDirectorySyncStreamType_Source);

					CFile::fs_CreateDirectory(CFile::fs_GetPath(Destination));
					
					auto Attributes = EFileAttrib_UnixAttributesValid | EFileAttrib_UserRead | EFileAttrib_UserWrite;

					if (Source != Destination)
					{
						RSyncState.m_pSourceDestinationStream = Config.m_FileOptions.f_OpenFile
							(
								Destination
								, EDirectorySyncStreamType_Destination
								, EFileOpen_Read | EFileOpen_Write | EFileOpen_DontTruncate | EFileOpen_ShareAll
								, Attributes
							)
						;

						if (Source && CFile::fs_FileExists(Source))
						{
#if DMibConfig_Tests_Enable
							if (auto Delay = g_TestDirectorySyncSourceOpenDelay.f_Load(); Delay)
								NSys::fg_Thread_Sleep(Delay);
#endif
							try
							{
								RSyncState.m_pSourceStream = Config.m_FileOptions.f_OpenFile(Source, EDirectorySyncStreamType_Source, EFileOpen_Read | EFileOpen_ShareAll);
							}
							catch (CExceptionFile const &)
							{
								// Protect against file dissapearing after existence check
								if (CFile::fs_FileExists(Source))
									throw;
							}
						}

						RSyncState.m_pClient
							= fg_Construct(*RSyncState.m_pSourceStream, *RSyncState.m_pSourceDestinationStream, 256, 4*1024*1024, 8*1024*1024, nullptr, ClientFlags)
						;
					}
					else
					{
						RSyncState.m_pSourceDestinationStream = Config.m_FileOptions.f_OpenFile
							(
								Destination
								, EDirectorySyncStreamType_SourceDestination
								, EFileOpen_Read | EFileOpen_Write | EFileOpen_DontTruncate | EFileOpen_ShareAll
								, Attributes
							)
						;

						CStr TempFileName = CFile::fs_AppendPath(Config.m_TempDirectory, fg_Format("{}.rsynctemp", fg_RandomID()));
						RSyncState.m_TempFiles.f_Insert(TempFileName);
						CFile::fs_CreateDirectory(Config.m_TempDirectory);

						RSyncState.m_pTempStream = Config.m_FileOptions.f_OpenFile
							(
								TempFileName
								, EDirectorySyncStreamType_TempSourceDestination
								, EFileOpen_Read | EFileOpen_Write | EFileOpen_DontTruncate | EFileOpen_ShareAll
								, Attributes
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
				, [=, pConfigUnsafe = m_pConfig, pManifestUnsafe = m_pManifest](CRunningSyncState *_pRSyncState) -> TCFuture<void>
				{
					co_await NConcurrency::ECoroutineFlag_AllowReferences;

					auto pConfig = pConfigUnsafe;
					auto pManifest = pManifestUnsafe;
					auto &Config = *pConfig;
					if (!(Config.m_SyncFlags & (ESyncFlag_WriteTime | ESyncFlag_Owner | ESyncFlag_Group | ESyncFlag_Attributes)))
						co_return {};
					
					auto BlockingActorCheckout = fg_BlockingActor();
					co_await
						(
							g_Dispatch(BlockingActorCheckout) / [=, Destination = _pRSyncState->m_DestinationFilename]
							{
								auto &Config = *pConfig;

								auto *pManifestFile = pManifest->m_Files.f_FindEqual(_FileName);

								if (!pManifestFile)
									return;

								auto &ManifestFile = *pManifestFile;

								if (Config.m_SyncFlags & ESyncFlag_WriteTime)
								{
									if (ManifestFile.m_Attributes & EFileAttrib_Link)
										CFile::fs_SetWriteTimeOnLink(Destination, ManifestFile.m_WriteTime);
									else
										CFile::fs_SetWriteTime(Destination, ManifestFile.m_WriteTime);
								}

								if (Config.m_SyncFlags & ESyncFlag_Attributes)
								{
									if (ManifestFile.m_Attributes & EFileAttrib_Link)
										CFile::fs_SetAttributesOnLink(Destination, ManifestFile.m_Attributes | NMib::NFile::EFileAttrib_UnixAttributesValid);
									else
										CFile::fs_SetAttributes(Destination, ManifestFile.m_Attributes | NMib::NFile::EFileAttrib_UnixAttributesValid);
								}

								if (Config.m_SyncFlags & ESyncFlag_Group)
								{
									if (ManifestFile.m_Attributes & EFileAttrib_Link)
										CFile::fs_SetGroupOnLink(Destination, ManifestFile.m_Group);
									else
										CFile::fs_SetGroup(Destination, ManifestFile.m_Group);
								}

								if (Config.m_SyncFlags & ESyncFlag_Owner)
								{
									if (ManifestFile.m_Attributes & EFileAttrib_Link)
										CFile::fs_SetOwnerOnLink(Destination, ManifestFile.m_Owner);
									else
										CFile::fs_SetOwner(Destination, ManifestFile.m_Owner);
								}
							}
						)
					;

					co_return {};
				}
				, [this, _FileName](CActorSubscription &&_Subscription) -> TCFuture<CDirectorySyncClient::FRunRSync>
				{
					return g_Future <<= m_Client.f_CallActor(&CDirectorySyncClient::f_StartRSync)(_FileName, fg_Move(_Subscription));
				}
			)
		;
	}

	void CDirectorySyncReceive::CInternal::f_RunFileSyncs(TCPromise<void> const &_Promise)
	{
		auto &Config = *m_pConfig;
	
		while (m_nRunningSyncs < Config.m_RSyncConcurrency && !m_PendingFileSyncs.f_IsEmpty() && !m_pThis->f_IsDestroyed())
		{
			auto FileName = *m_PendingFileSyncs.f_FindSmallest();
			m_PendingFileSyncs.f_Remove(FileName);
			
			++m_nRunningSyncs;
			
			f_SyncFile(FileName) > [this, _Promise](TCAsyncResult<void> &&_Result)
				{
					if (!_Result)
						fg_AddStrSep(m_SyncErrors, _Result.f_GetExceptionStr(), "\n");
					--m_nRunningSyncs;
					f_RunFileSyncs(_Promise);
				}
			;
		}
		
		if (m_PendingFileSyncs.f_IsEmpty() && m_nRunningSyncs == 0 && !_Promise.f_IsSet())
		{
			if (m_SyncErrors.f_IsEmpty())
				_Promise.f_SetResult();
			else
				_Promise.f_SetException(DMibErrorInstance(m_SyncErrors));
		}
	}
}
