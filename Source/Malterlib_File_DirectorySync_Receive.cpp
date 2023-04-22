// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_DirectorySync.h"
#include "Malterlib_File_DirectorySync_ReceiveInternal.h"

#include <Mib/Concurrency/LogError>

namespace NMib::NFile
{
	CDirectorySyncReceive::CDirectorySyncReceive(CConfig &&_Config, TCDistributedActorInterface<CDirectorySyncClient> &&_Client)
		: mp_pInternal(fg_Construct(this, fg_Move(_Config), fg_Move(_Client)))
	{
	}

	CDirectorySyncReceive::CInternal::CInternal(CDirectorySyncReceive *_pThis, CConfig &&_Config, TCDistributedActorInterface<CDirectorySyncClient> &&_Client)
		: m_pThis(_pThis)
		, m_pConfig(fg_Construct(fg_Move(_Config)))
		, m_Client(fg_Move(_Client))
		, m_FileActor(fg_Construct(), "Directory sync receive file access")
	{
		if (m_pConfig->m_TempDirectory.f_IsEmpty())
			DMibError("You need to specify a valid temp directory");
	}

	CDirectorySyncReceive::CInternal::~CInternal() = default;
	
	CDirectorySyncReceive::~CDirectorySyncReceive() = default;
	CDirectorySyncReceive::CInternal::CRunningSyncState::CRunningSyncState()
	{
	}

	CDirectorySyncReceive::CInternal::CRunningSyncState::~CRunningSyncState() = default;

	
	auto CDirectorySyncReceive::CInternal::CRunningSyncState::f_Destroy() -> TCFuture<void>
	{
		co_await NConcurrency::ECoroutineFlag_AllowReferences;

		CLogError LogError("DirectorySyncReceive");

		auto pThis = TCSharedPointerSupportWeak<CRunningSyncState>(this);
		co_await fg_Move(m_fRunProtocol).f_Destroy().f_Wrap() > LogError.f_Warning("Failed to destroy run protocol");

		if (!pThis->m_pClient)
			co_return {};

		co_await
			(
				g_Dispatch(pThis->m_FileActor) / [pThis]
				{
					pThis->m_pClient.f_Clear();
					pThis->m_pSourceDestinationStream.f_Clear();
					pThis->m_pSourceStream.f_Clear();
					pThis->m_pTempStream.f_Clear();
					for (auto &File : pThis->m_TempFiles)
					{
						try
						{
							CFile::fs_DeleteFile(File);
						}
						catch (CExceptionFile const &)
						{
						}
					}
					pThis->m_TempFiles.f_Clear();
				}
			)
		;


		co_return {};
	}

	void CDirectorySyncReceive::CInternal::fs_CheckDestroy(TCSharedPointer<NAtomic::TCAtomic<bool>> const &_pDestroyed)
	{
		if (_pDestroyed->f_Load(NAtomic::EMemoryOrder_Relaxed))
			DMibError("Directory sync receive destroyed");
	}
	
	TCFuture<void> CDirectorySyncReceive::fp_Destroy()
	{
		auto &Internal = *mp_pInternal;
		*Internal.m_pDestroyed = true;

		CLogError LogError("DirectorySyncReceive");

		if (Internal.m_Client)
			co_await Internal.m_Client.f_Destroy().f_Wrap() > LogError.f_Warning("Failed to destroy client");

		{
			TCActorResultVector<void> RSyncDestroys;

			for (auto &pRSync : Internal.m_RSyncStates)
				pRSync->f_Destroy() > RSyncDestroys.f_AddResult();

			co_await RSyncDestroys.f_GetUnwrappedResults().f_Wrap() > LogError.f_Warning("Failed to destroy rsync states");
		}

		if (Internal.m_FileActor)
			fg_Move(Internal.m_FileActor).f_Destroy() > LogError.f_Warning("Failed to destroy file actor");

		co_return {};
	}
	
	auto CDirectorySyncReceive::f_PerformSync() -> TCFuture<CSyncResult>
	{
		auto &Internal = *mp_pInternal;
		
		if (Internal.m_bStartedSync)
			co_return DMibErrorInstance("Sync already perfromed");
		
		Internal.m_bStartedSync = true;
		
		co_await Internal.f_SyncManifest();
		co_await Internal.f_HandleExcessFiles();

		auto &Config = *Internal.m_pConfig;

		for (auto &File : Internal.m_pManifest->m_Files)
		{
			auto &FileName = Internal.m_pManifest->m_Files.fs_GetKey(File);
			if (!Config.m_IncludeWildcards.f_IsEmpty())
			{
				if (!fg_StrMatchesAnyWildcardInMap(FileName, Config.m_IncludeWildcards))
					continue;
			}
			if (fg_StrMatchesAnyWildcardInMap(FileName, Config.m_ExcludeWildcards))
				continue;
			Internal.m_PendingFileSyncs[FileName];
			CStr Directory = CFile::fs_GetPath(FileName);
			while (!Directory.f_IsEmpty())
			{
				Internal.m_PendingFileSyncs[Directory];
				Directory = CFile::fs_GetPath(Directory);
			}
		}

		TCPromise<void> SyncsPromise;
		Internal.f_RunFileSyncs(SyncsPromise);

		co_await SyncsPromise.f_MoveFuture();

		co_await Internal.m_Client.f_CallActor(&CDirectorySyncClient::f_Finished)();

		CSyncResult SyncResult;
		SyncResult.m_Manifest = fg_Move(*Internal.m_pManifest);
		SyncResult.m_Stats = Internal.m_Stats;
		SyncResult.m_Stats.m_nSeconds = Internal.m_Clock.f_GetTime();

		co_return fg_Move(SyncResult);
	}
}
