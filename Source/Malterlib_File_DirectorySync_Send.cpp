// Copyright © 2015 Hansoft AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_DirectorySync.h"
#include <Mib/Cryptography/RandomID>
#include <Mib/File/RSync>
#include <Mib/Concurrency/ActorSubscription>
#include <Mib/Concurrency/LogError>

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

		struct CRunningSyncState
		{
			TCUnsafeFuture<CByteStats> f_Destroy();

			CIntrusiveRefCountWithWeak m_RefCount;
			TCUniquePointer<NStream::CBinaryStream> m_pFile = fg_Construct<TCBinaryStreamFile<>>();
			CBinaryStreamMemory<> m_FileMemory;
			TCUniquePointer<CRSyncServer> m_pRSyncServer;
			CByteStats m_ByteStats;
			CActorSubscription m_Subscription;
		};

		CInternal(CDirectorySyncSend *_pThis, CConfig &&_Config);
		static void fs_CheckDestroy(TCSharedPointer<NAtomic::TCAtomic<bool>> const &_pDestroyed);
		NException::CExceptionPointer f_CheckFileName(CStr const &_FileName);
		auto f_StartRSync(TCActorSubscriptionWithID<> _Subscription, TCFunctionMutable<void (CRunningSyncState &_State)> _fOpenRSync) -> TCFuture<FRunRSync>;

		CDirectorySyncSend *m_pThis = nullptr;
		TCSharedPointer<CConfig> m_pConfig;
		TCSharedPointer<CDirectoryManifest> m_pManifest = fg_Construct();
		TCSharedPointer<TCAtomic<bool>> m_pDestroyed = fg_Construct(false);
		TCMap<CStr, TCSharedPointerSupportWeak<CRunningSyncState>> m_RSyncStates;
		CClock m_Clock{true};
		CDirectorySyncStats m_Stats;
		TCSharedPointer<CCanDestroyTracker> m_pCanDestroyTracker = fg_Construct();
		bool m_bFinished = false;
		bool m_bUseOriginalLocation = false;
	};

	CDirectorySyncSend::CInternal::CInternal(CDirectorySyncSend *_pThis, CConfig &&_Config)
		: m_pThis(_pThis)
		, m_pConfig(fg_Construct(fg_Move(_Config)))
	{
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

		co_return Result;
	}

	void CDirectorySyncSend::CInternal::fs_CheckDestroy(TCSharedPointer<NAtomic::TCAtomic<bool>> const &_pDestroyed)
	{
		if (*_pDestroyed)
			DMibError("Directory sync aborted");
	}

	NException::CExceptionPointer CDirectorySyncSend::CInternal::f_CheckFileName(CStr const &_FileName)
	{
		CStr Error;
		if (!CFile::fs_IsSafeRelativePath(_FileName, Error))
			return NException::fg_MakeException(DMibErrorInstance("The path cannot {}"_f << Error));

		return nullptr;
	}

	TCFuture<void> CDirectorySyncSend::fp_Destroy()
	{
		auto &Internal = *mp_pInternal;

		*Internal.m_pDestroyed = true;

		CLogError LogError("DirectorySyncSend");

		{
			auto pCanDestroy = fg_Move(Internal.m_pCanDestroyTracker);
			auto CanDestroyFuture = pCanDestroy->f_Future();
			pCanDestroy.f_Clear();
			co_await fg_Move(CanDestroyFuture).f_Wrap() > LogError.f_Warning("Failed to destroy tracker");
		}

		TCFutureVector<CInternal::CByteStats> StateDestroys;

		for (auto &pState : Internal.m_RSyncStates)
			pState->f_Destroy() > StateDestroys;

		co_await fg_AllDone(StateDestroys).f_Wrap() > LogError.f_Warning("Failed to destroy rsync states");

		co_return {};
	}

	auto CDirectorySyncSend::CInternal::CRunningSyncState::f_Destroy() -> TCUnsafeFuture<CByteStats>
	{
		auto pThis = TCSharedPointerSupportWeak<CRunningSyncState>(this); // Keep this alive

		CLogError LogError("DirectorySyncSend");

		if (m_Subscription)
			co_await m_Subscription->f_Destroy().f_Wrap() > LogError.f_Warning("Failed to destroy subscription");

		if (!m_pRSyncServer)
			co_return CByteStats{};

		auto BlockingActorCheckout = fg_BlockingActor();

		auto ByteStats = co_await
			(
				g_Dispatch(BlockingActorCheckout) / [pThis]
				{
					pThis->m_pRSyncServer.f_Clear();
					pThis->m_FileMemory.f_Clear();
					pThis->m_pFile.f_Clear();
					return pThis->m_ByteStats;
				}
			)
		;

		co_return fg_Move(ByteStats);
	}

	auto CDirectorySyncSend::CInternal::f_StartRSync(TCActorSubscriptionWithID<> _Subscription, TCFunctionMutable<void (CRunningSyncState &_State)> _fOpenRSync)
		-> TCFuture<FRunRSync>
	{
		auto pCanDestroy = m_pCanDestroyTracker;

		if (!pCanDestroy)
			co_return DMibErrorInstance("Aborted");

		CStr RSyncID = fg_FastRandomID(m_RSyncStates);
		auto &pRSyncState = m_RSyncStates[RSyncID] = fg_Construct();
		auto &RSyncState = *pRSyncState;

		auto pCleanup = g_OnScopeExitActor / [this, RSyncID]
			{
				m_RSyncStates.f_Remove(RSyncID);
			}
		;

		RSyncState.m_Subscription = fg_Move(_Subscription);

		auto BlockingActorCheckout = fg_BlockingActor();
		auto BlockingActor = BlockingActorCheckout.f_Actor();

		TCPromiseFuturePair<FRunRSync> Promise;

		g_Dispatch(BlockingActor) / [pRSyncState, fOpenRSync = fg_Move(_fOpenRSync), pCanDestroy]() mutable
			{
				fOpenRSync(*pRSyncState);
			}
			> fg_Move(Promise.m_Promise)
			/ [this, pDestroyed = m_pDestroyed, pCleanup, pRSyncState, Promise = Promise.m_Promise, RSyncID, BlockingActorCheckout = fg_Move(BlockingActorCheckout), pCanDestroy]() mutable
			{
				if (m_pThis->f_IsDestroyed())
				{
					Promise.f_SetException(DMibErrorInstance("Aborted"));
					pCleanup.f_Clear();

					return;
				}

				auto BlockingActor = BlockingActorCheckout.f_Actor();
				Promise.f_SetResult
					(
						g_ActorFunctor(BlockingActor)
						(
							g_ActorSubscription / [this, pRSyncState, RSyncID, BlockingActorCheckout = fg_Move(BlockingActorCheckout)]() -> TCFuture<void>
							{
								m_RSyncStates.f_Remove(RSyncID);

								CByteStats Stats = co_await pRSyncState->f_Destroy();

								m_Stats.m_OutgoingBytes += Stats.m_nOutgoing;
								m_Stats.m_IncomingBytes += Stats.m_nIncoming;
								++m_Stats.m_nSyncedFiles;

								co_return {};
							}
						)
						/ [this, pCanDestroy, pDestroyed, pRSyncState](CIOByteVector _Packet) -> TCFuture<CIOByteVector>
						{
							auto CaptureScope = co_await g_CaptureExceptions;

							if (m_pThis->f_IsDestroyed())
								DMibError("Aborted");

							if (!pRSyncState->m_pRSyncServer)
								DMibError("RSync server destroyed");

							CIOByteVector ToSendToClient;
							pRSyncState->m_pRSyncServer->f_ProcessPacket
								(
									_Packet
									, ToSendToClient
									, [pDestroyed]
									{
										fs_CheckDestroy(pDestroyed);
									}
								)
							;

							pRSyncState->m_ByteStats.m_nIncoming += _Packet.f_GetLen();
							pRSyncState->m_ByteStats.m_nOutgoing += ToSendToClient.f_GetLen();

							co_return fg_Move(ToSendToClient);
						}
					)
				;
				pCleanup->f_Clear();
			}
		;

		co_return co_await fg_Move(Promise.m_Future);
	}

	auto CDirectorySyncSend::f_StartManifestRSync(TCActorSubscriptionWithID<> _Subscription) -> TCFuture<FRunRSync>
	{
		uint32 ProtocolVersion = fg_GetCallingHostInfo().f_GetProtocolVersion();
		auto &Internal = *mp_pInternal;

		return Internal.f_StartRSync
			(
				fg_Move(_Subscription)
				, [ProtocolVersion, pManifest = Internal.m_pManifest, pConfig = Internal.m_pConfig, pDestroyed = Internal.m_pDestroyed](CInternal::CRunningSyncState &_RSyncState)
				{
					auto &Config = *pConfig;

					CDirectoryManifest::EManifestStreamVersion ManifestVersion = CDirectoryManifest::EManifestStreamVersion_Min;

					if (ProtocolVersion >= CDirectorySyncClient::EProtocolVersion_OptionalDigest)
						ManifestVersion = CDirectoryManifest::EManifestStreamVersion_OptionalDigest;

					CBinaryStream *pBinaryStream = nullptr;
					switch (Config.m_Manifest.f_GetTypeID())
					{
					case 0:
						{
							auto &Manifest = Config.m_Manifest.f_Get<0>();
							Manifest.f_Stream(fg_FeedStream(_RSyncState.m_FileMemory), ManifestVersion);

							pBinaryStream = &_RSyncState.m_FileMemory;
							*pManifest = Manifest;
							break;
						}
					case 1:
						{
							auto &ManifestConfig = Config.m_Manifest.f_Get<1>();
							auto Manifest = CDirectoryManifest::fs_GetManifest(ManifestConfig, [&]{ CInternal::fs_CheckDestroy(pDestroyed); });
							Manifest.f_Stream(fg_FeedStream(_RSyncState.m_FileMemory), ManifestVersion);
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
									, EFileOpen_Read | EFileOpen_ShareAll | EFileOpen_NoLocalCache | EFileOpen_NoLocalCache
								)
							;
							pManifest->f_Stream(fg_ConsumeStream(*_RSyncState.m_pFile), ManifestVersion);
							_RSyncState.m_pFile->f_SetPosition(0);
							pBinaryStream = &*_RSyncState.m_pFile;
							break;
						}
					default:
						DMibError("Invalid manifest type");
					}

					auto ServerFlags = ERSyncFlag_None;
					if (ProtocolVersion >= CDirectorySyncClient::EProtocolVersion_UseSHA256)
						ServerFlags |= ERSyncFlag_UseSHA256;

					_RSyncState.m_pRSyncServer = fg_Construct(*pBinaryStream, 8 * 1024 * 1024, ServerFlags);
				}
			)
		;
	}

	auto CDirectorySyncSend::f_StartRSync(NStr::CStr _FileName, TCActorSubscriptionWithID<> _Subscription) -> TCFuture<FRunRSync>
	{
		uint32 ProtocolVersion = fg_GetCallingHostInfo().f_GetProtocolVersion();

		auto &Internal = *mp_pInternal;

		if (auto pException = Internal.f_CheckFileName(_FileName))
			co_return pException;

		auto *pFile = Internal.m_pManifest->m_Files.f_FindEqual(_FileName);

		if (!pFile)
			co_return DMibErrorInstance("File does not exist in manifest");

		if (!pFile->f_IsFile())
			co_return DMibErrorInstance("Is not a file in manifest");

		CStr FilePath;

		if (Internal.m_bUseOriginalLocation)
			FilePath = pFile->m_OriginalPath;
		else
			FilePath = _FileName;

		FilePath = Internal.m_pConfig->m_FileOptions.f_TransformFileName(Internal.m_pConfig->m_BasePath, FilePath, EDirectorySyncStreamType_Source);

		co_return co_await Internal.f_StartRSync
			(
				fg_Move(_Subscription)
				, [=, pConfig = Internal.m_pConfig](CInternal::CRunningSyncState &_RSyncState)
				{
					_RSyncState.m_pFile = pConfig->m_FileOptions.f_OpenFile(FilePath, EDirectorySyncStreamType_Source, EFileOpen_Read | EFileOpen_ShareAll | EFileOpen_NoLocalCache);

					auto ServerFlags = ERSyncFlag_None;
					if (ProtocolVersion >= CDirectorySyncClient::EProtocolVersion_UseSHA256)
						ServerFlags |= ERSyncFlag_UseSHA256;

					_RSyncState.m_pRSyncServer = fg_Construct(*_RSyncState.m_pFile, 8 * 1024 * 1024, ServerFlags);
				}
			)
		;
	}

	TCFuture<void> CDirectorySyncSend::f_Finished()
	{
		auto &Internal = *mp_pInternal;
		Internal.m_bFinished = true;

		co_return {};
	}
}
