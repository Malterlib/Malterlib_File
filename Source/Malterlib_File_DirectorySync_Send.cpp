// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_DirectorySync.h"
#include <Mib/Cryptography/RandomID>
#include <Mib/File/RSync>
#include <Mib/Concurrency/ActorSubscription>

namespace NMib::NFile
{
	using namespace NStr;
	using namespace NConcurrency;
	using namespace NContainer;
	using namespace NAtomic;
	using namespace NCryptography;
	using namespace NStorage;
	using namespace NFile;
	using namespace NStream;
	using namespace NFunction;
	using namespace NTime;
	
	struct CDirectorySyncSend::CInternal : public CActorInternal
	{
		struct CByteStats
		{
			uint64 m_nIncoming = 0;
			uint64 m_nOutgoing = 0;
		};
		
		struct CRunningSyncState : public TCSharedPointerIntrusiveBase<ESharedPointerOption_SupportWeakPointer>
		{
			TCFuture<CByteStats> f_Destroy();
		
			TCActor<CSeparateThreadActor> m_FileActor;
			TCUniquePointer<NStream::CBinaryStream> m_pFile = fg_Construct<TCBinaryStreamFile<>>();
			CBinaryStreamMemory<> m_FileMemory;
			TCUniquePointer<CRSyncServer> m_pRSyncServer;
			CByteStats m_ByteStats;
			CActorSubscription m_Subscription;
		};
		
		CInternal(CDirectorySyncSend *_pThis, CConfig &&_Config);
		static void fs_CheckDestroy(TCSharedPointer<NAtomic::TCAtomic<bool>> const &_pDestroyed);
		NException::CExceptionPointer f_CheckFileName(CStr const &_FileName);
		auto f_StartRSync(TCActorSubscriptionWithID<> &&_Subscription, TCFunctionMutable<void (CRunningSyncState &_State)> &&_fOpenRSync) -> TCFuture<FRunRSync>;

		CDirectorySyncSend *m_pThis = nullptr;
		TCSharedPointer<CConfig> m_pConfig;
		TCSharedPointer<CDirectoryManifest> m_pManifest = fg_Construct();
		TCActor<CSeparateThreadActor> m_FileActor;
		TCSharedPointer<TCAtomic<bool>> m_pDestroyed = fg_Construct(false);
		TCMap<CStr, TCSharedPointerSupportWeak<CRunningSyncState>> m_RSyncStates;
		CClock m_Clock{true};
		CDirectorySyncStats m_Stats;
		bool m_bFinished = false;
		bool m_bUseOriginalLocation = false;
	};

	CDirectorySyncSend::CInternal::CInternal(CDirectorySyncSend *_pThis, CConfig &&_Config)
		: m_pThis(_pThis)
		, m_pConfig(fg_Construct(fg_Move(_Config)))
	{
		m_FileActor = fg_Construct(fg_Construct(), "Directory sync file access");
		if (m_pConfig->m_bUseOriginalLocation)
			m_bUseOriginalLocation = *m_pConfig->m_bUseOriginalLocation;
		else if (m_pConfig->m_Manifest.f_IsOfType<CDirectoryManifestConfig>())
			m_bUseOriginalLocation = true;
	}
	
	CDirectorySyncSend::CDirectorySyncSend(CConfig &&_Config)
		: mp_pInternal(fg_Construct(this, fg_Move(_Config)))
	{
	}
	
	CDirectorySyncSend::~CDirectorySyncSend() = default;
	
	auto CDirectorySyncSend::f_GetResult() -> TCFuture<CSyncResult>
	{
		auto &Internal = *mp_pInternal;
		
		CSyncResult Result;
		Result.m_bFinished = Internal.m_bFinished;
		Result.m_Stats = Internal.m_Stats;
		Result.m_Stats.m_nSeconds = Internal.m_Clock.f_GetTime();
		
		return fg_Explicit(Result);
	}
	
	void CDirectorySyncSend::CInternal::fs_CheckDestroy(TCSharedPointer<NAtomic::TCAtomic<bool>> const &_pDestroyed)
	{
		if (*_pDestroyed)
			DMibError("Directory sync send destroyed");
	}
	
	NException::CExceptionPointer CDirectorySyncSend::CInternal::f_CheckFileName(CStr const &_FileName)
	{
		CStr Error;
		if (!CFile::fs_IsSafeRelativePath(_FileName, Error))
			return NException::fg_ExceptionPointer(DMibErrorInstance("The path cannot {}"_f << Error));

		return nullptr;
	}
	
	TCFuture<void> CDirectorySyncSend::fp_Destroy()
	{
		auto &Internal = *mp_pInternal;
		
		*Internal.m_pDestroyed = true;
		
		TCActorResultVector<CInternal::CByteStats> StateDestroys;
		
		for (auto &pState : Internal.m_RSyncStates)
			pState->f_Destroy() > StateDestroys.f_AddResult();
		
		TCPromise<void> Promise;
		
		StateDestroys.f_GetResults() > Promise / [this, Promise]
			{
				auto &Internal = *mp_pInternal;
				Internal.m_FileActor->f_Destroy() > Promise;
			}
		;
		
		return Promise.f_MoveFuture();
	}

	auto CDirectorySyncSend::CInternal::CRunningSyncState::f_Destroy() -> TCFuture<CByteStats>
	{
		TCFuture<void> SubscriptionDestroyFuture;
		
		if (m_Subscription)
			SubscriptionDestroyFuture = m_Subscription->f_Destroy();
		else
			SubscriptionDestroyFuture = fg_Explicit();
	
		TCPromise<CByteStats> Promise;

		fg_Move(SubscriptionDestroyFuture) > Promise / [pThis = TCSharedPointerSupportWeak<CRunningSyncState>(this), Promise]
			{
				if (!pThis->m_pRSyncServer)
					return Promise.f_SetResult(CByteStats{});
				
				g_Dispatch(pThis->m_FileActor) / [pThis]
					{
						pThis->m_pRSyncServer.f_Clear();
						pThis->m_FileMemory.f_Clear();
						pThis->m_pFile.f_Clear();
						return pThis->m_ByteStats;
					}
					> Promise
				;
			}
		;

		return Promise.f_MoveFuture();
	}
	
	auto CDirectorySyncSend::CInternal::f_StartRSync(TCActorSubscriptionWithID<> &&_Subscription, TCFunctionMutable<void (CRunningSyncState &_State)> &&_fOpenRSync)
		-> TCFuture<FRunRSync>
	{
		CStr RSyncID = fg_RandomID();
		auto &pRSyncState = m_RSyncStates[RSyncID] = fg_Construct();
		auto &RSyncState = *pRSyncState;
		
		auto pCleanup = g_OnScopeExitActor > [this, RSyncID]
			{
				m_RSyncStates.f_Remove(RSyncID);
			}
		;
		
		RSyncState.m_FileActor = m_FileActor;
		RSyncState.m_Subscription = fg_Move(_Subscription);
		
		TCPromise<FRunRSync> Promise;
		
		g_Dispatch(RSyncState.m_FileActor) / [pRSyncState, fOpenRSync = fg_Move(_fOpenRSync)]() mutable
			{
				fOpenRSync(*pRSyncState);
			}
			> Promise / [=]()
			{
				Promise.f_SetResult
					(
						g_ActorFunctor(pRSyncState->m_FileActor)
						(
							g_ActorSubscription / [=]() -> TCFuture<void>
							{
								m_RSyncStates.f_Remove(RSyncID);
								
								TCPromise<void> Promise;
								pRSyncState->f_Destroy() > Promise / [this, Promise](CByteStats &&_Stats)
									{
										m_Stats.m_OutgoingBytes += _Stats.m_nOutgoing;
										m_Stats.m_IncomingBytes += _Stats.m_nIncoming;
										++m_Stats.m_nSyncedFiles;
										
										Promise.f_SetResult();
									}
								;
								return Promise.f_MoveFuture();
							}
						)
						/ [=](CSecureByteVector &&_Packet) -> TCFuture<CSecureByteVector>
						{
							return TCFuture<CSecureByteVector>::fs_RunProtected() / [=, Packet = fg_Move(_Packet)]() -> CSecureByteVector
								{
									if (!pRSyncState->m_pRSyncServer)
										DMibError("RSync server destroyed");

									CSecureByteVector ToSendToClient;
									pRSyncState->m_pRSyncServer->f_ProcessPacket(Packet, ToSendToClient);

									pRSyncState->m_ByteStats.m_nIncoming += Packet.f_GetLen();
									pRSyncState->m_ByteStats.m_nOutgoing += ToSendToClient.f_GetLen();
									
									return ToSendToClient;
								}
							;
						}
					)
				;
				pCleanup->f_Clear();
			}
		;
		
		return Promise.f_MoveFuture();
	}

	auto CDirectorySyncSend::f_StartManifestRSync(TCActorSubscriptionWithID<> &&_Subscription) -> TCFuture<FRunRSync>
	{
		auto &Internal = *mp_pInternal;
		return Internal.f_StartRSync
			(
				fg_Move(_Subscription)
				, [pManifest = Internal.m_pManifest, pConfig = Internal.m_pConfig, pDestroyed = Internal.m_pDestroyed](CInternal::CRunningSyncState &_RSyncState)
				{
					auto &Config = *pConfig;
					
					CBinaryStream *pBinaryStream = nullptr;
					switch (Config.m_Manifest.f_GetTypeID())
					{
					case 0:
						{
							auto &Manifest = Config.m_Manifest.f_Get<0>();
							_RSyncState.m_FileMemory << Manifest;
							pBinaryStream = &_RSyncState.m_FileMemory;
							*pManifest = Manifest;
							break;
						}
					case 1:
						{
							auto &ManifestConfig = Config.m_Manifest.f_Get<1>();
							auto Manifest = CDirectoryManifest::fs_GetManifest(ManifestConfig, [&]{ CInternal::fs_CheckDestroy(pDestroyed); });
							_RSyncState.m_FileMemory << Manifest;
							pBinaryStream = &_RSyncState.m_FileMemory;
							*pManifest = fg_Move(Manifest);
							break;
						}
					case 2:
						{
							auto &ManifestFileName = Config.m_Manifest.f_Get<2>();
							_RSyncState.m_pFile = Config.m_FileOptions.f_OpenFile
								(
								 	ManifestFileName
								 	, EDirectorySyncStreamType_ManifestSource
								 	, EFileOpen_Read | EFileOpen_ShareAll
								)
							;
							*_RSyncState.m_pFile >> *pManifest;
							_RSyncState.m_pFile->f_SetPosition(0);
							pBinaryStream = &*_RSyncState.m_pFile;
							break;
						}
					default:
						DMibError("Invalid manifest type");
					}
							
					_RSyncState.m_pRSyncServer = fg_Construct(*pBinaryStream, 8*1024*1024);
				}
			)
		;
	}
	
	auto CDirectorySyncSend::f_StartRSync(NStr::CStr const &_FileName, TCActorSubscriptionWithID<> &&_Subscription) -> TCFuture<FRunRSync>
	{
		auto &Internal = *mp_pInternal;
		
		if (auto pException = Internal.f_CheckFileName(_FileName))
			return pException;
		
		auto *pFile = Internal.m_pManifest->m_Files.f_FindEqual(_FileName);
		
		if (!pFile)
			return DMibErrorInstance("File does not exist in manifest");

		if (!pFile->f_IsFile())
			return DMibErrorInstance("Is not a file in manifest");
			
		CStr FilePath;
		
		if (Internal.m_bUseOriginalLocation)
			FilePath = pFile->m_OriginalPath;
		else
			FilePath = _FileName;

		FilePath = Internal.m_pConfig->m_FileOptions.f_TransformFileName(Internal.m_pConfig->m_BasePath, FilePath, EDirectorySyncStreamType_Source);

		return Internal.f_StartRSync
			(
				fg_Move(_Subscription)
				, [=, pConfig = Internal.m_pConfig](CInternal::CRunningSyncState &_RSyncState)
				{
					_RSyncState.m_pFile = pConfig->m_FileOptions.f_OpenFile(FilePath, EDirectorySyncStreamType_Source, EFileOpen_Read | EFileOpen_ShareAll);

					_RSyncState.m_pRSyncServer = fg_Construct(*_RSyncState.m_pFile, 8*1024*1024);
				}
			)
		;
	}

	TCFuture<void> CDirectorySyncSend::f_Finished()
	{
		auto &Internal = *mp_pInternal;
		Internal.m_bFinished = true;
		
		return fg_Explicit();
	}
}
