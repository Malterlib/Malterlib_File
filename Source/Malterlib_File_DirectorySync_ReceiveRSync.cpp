// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Malterlib_File_DirectorySync.h"
#include "Malterlib_File_DirectorySync_ReceiveInternal.h"

namespace NMib::NFile
{
	void CDirectorySyncReceive::CInternal::fsp_RunRSyncProtocol
		(
			TCSharedPointerSupportWeak<CRunningSyncState> const &_pState
			, CIOByteVector &&_ServerPacket
			, TCPromise<CByteStats> const &_Promise
		)
	{
		if (*_pState->m_pDestroyed)
		{
			if (!_Promise.f_IsSet())
				_Promise.f_SetException(DMibErrorInstance("RSync aborted"));

			return;
		}

		auto &State = *_pState;
		if (!State.m_pClient)
		{
			if (!_Promise.f_IsSet())
				_Promise.f_SetException(DMibErrorInstance("RSync aborted"));
			return;
		}

		State.m_ByteStats.m_nIncoming += _ServerPacket.f_GetLen();

		bool bWantOneMoreProcess = true;
		bool bDone = false;
		while (bWantOneMoreProcess)
		{
			CIOByteVector ToSendToServer;
			try
			{
				if
					(
						State.m_pClient->f_ProcessPacket
						(
							_ServerPacket
							, ToSendToServer
							, bWantOneMoreProcess
							, [pDestroyed = _pState->m_pDestroyed]
							{
								fs_CheckDestroy(pDestroyed);
							}
						)
					)
				{
					bDone = true;
				}
			}
			catch (NException::CException const &_Exception)
			{
				if (!_Promise.f_IsSet())
					_Promise.f_SetException(DMibErrorInstance(fg_Format("Exception running RSync protocol: {}", _Exception.f_GetErrorStr())));
				return;
			}

			_ServerPacket.f_Clear();
			if (!ToSendToServer.f_IsEmpty())
			{
				State.m_ByteStats.m_nOutgoing += ToSendToServer.f_GetLen();

				State.m_fRunProtocol(fg_Move(ToSendToServer)) > [_pState, _Promise](TCAsyncResult<CIOByteVector> &&_ServerPacket)
					{
						if (!_ServerPacket)
						{
							if (!_Promise.f_IsSet())
								_Promise.f_SetException(DMibErrorInstance(fg_Format("Failed run RSync protocol: {}", _ServerPacket.f_GetExceptionStr())));
							return;
						}

						fsp_RunRSyncProtocol(_pState, fg_Move(*_ServerPacket), _Promise);
					}
				;
			}
		}

		if (bDone)
		{
			State.m_bFinished = true;
			if (!_Promise.f_IsSet())
				_Promise.f_SetResult(State.m_ByteStats);
		}
	}

	TCFuture<void> CDirectorySyncReceive::CInternal::f_RunRSyncProtocol(TCSharedPointerSupportWeak<CRunningSyncState> _pState)
	{
		auto BlockingActorCheckout = fg_BlockingActor();

		CByteStats Stats = co_await
			(
				g_Dispatch(BlockingActorCheckout) / [_pState]() -> TCFuture<CByteStats>
				{
					TCPromise<CByteStats> RunPromise;
					fsp_RunRSyncProtocol(_pState, {}, RunPromise);

					co_return co_await RunPromise.f_MoveFuture();
				}
			)
		;

		++m_Stats.m_nSyncedFiles;
		m_Stats.m_OutgoingBytes += Stats.m_nOutgoing;
		m_Stats.m_IncomingBytes += Stats.m_nIncoming;

		co_return {};
	}

	TCFuture<void> CDirectorySyncReceive::CInternal::f_RSync
		(
			TCFunctionMutable<bool (CRunningSyncState *_pState)> _fInitRSync
			, TCFunctionMutable<TCFuture<void> (CRunningSyncState *_pState)> _fOnDone
			, TCFunctionMutable<TCFuture<CDirectorySyncClient::FRunRSync> (CActorSubscription &&_Subscription)> _fStartRSync
		)
	{
		CStr RSyncID = fg_RandomID(m_RSyncStates);
		auto &pRSyncState = m_RSyncStates[RSyncID] = fg_Construct();

		pRSyncState->m_pDestroyed = m_pDestroyed;

		auto pCanDestroy = m_pCanDestroyTracker;
		if (!pCanDestroy)
			co_return DMibErrorInstance("Aborted");

		auto pCleanup = g_OnScopeExitActor / [this, RSyncID, pRSyncState, pCanDestroy]
			{
				m_RSyncStates.f_Remove(RSyncID);
				pRSyncState->f_Destroy().f_DiscardResult();
			}
		;

		auto BlockingActorCheckout = fg_BlockingActor();
		auto BlockingActor = BlockingActorCheckout.f_Actor();

		TCPromiseFuturePair<void> Promise;
		g_Dispatch(BlockingActor) / [=, fInitRSync = fg_Move(_fInitRSync)]() mutable
			{
				return fInitRSync(&*pRSyncState);
			}
			>
			[
				=
				, this
				, Promise = fg_Move(Promise.m_Promise)
				, pCleanup = pCleanup
				, fOnDone = fg_Move(_fOnDone)
				, fStartRSync = fg_Move(_fStartRSync)
				, BlockingActorCheckout = fg_Move(BlockingActorCheckout)
			]
			(TCAsyncResult<bool> _bAlreadySynced) mutable
			{
				if (!_bAlreadySynced)
				{
					if (!Promise.f_IsSet())
						Promise.f_SetException(_bAlreadySynced.f_GetException());
					return;
				}

				if (*_bAlreadySynced)
				{
					fOnDone(&*pRSyncState) > [=, BlockingActorCheckout = fg_Move(BlockingActorCheckout)](TCAsyncResult<void> &&_Result)
						{
							if (!_Result)
							{
								if (!Promise.f_IsSet())
									Promise.f_SetException(_Result.f_GetException());
								return;
							}

							pRSyncState->f_Destroy() > [=, pCleanup = pCleanup](TCAsyncResult<void> &&_Result)
								{
									if (!_Result)
									{
										if (!Promise.f_IsSet())
											Promise.f_SetException(_Result.f_GetException());
										return;
									}

									if (!Promise.f_IsSet())
										Promise.f_SetResult();
								}
							;
						}
					;
					return;
				}
				fStartRSync
					(
						g_ActorSubscription / [pRSyncState, Promise]
						{
							if (!Promise.f_IsSet() && !pRSyncState->m_bFinished)
								Promise.f_SetException(DMibErrorInstance("Manifest rsync aborted prematurely"));
						}
					)
					> [=, this, pCleanup = pCleanup, fOnDone = fg_Move(fOnDone), BlockingActorCheckout = fg_Move(BlockingActorCheckout)]
					(TCAsyncResult<CDirectorySyncClient::FRunRSync> &&_fRunRSync) mutable
					{
						if (!_fRunRSync)
						{
							if (!Promise.f_IsSet())
								Promise.f_SetException(_fRunRSync.f_GetException());
							return;
						}

						auto &RSyncState = *pRSyncState;
						RSyncState.m_fRunProtocol = fg_Move(*_fRunRSync);
						f_RunRSyncProtocol(pRSyncState) > [=, pCleanup = pCleanup, fOnDone = fg_Move(fOnDone), BlockingActorCheckout = fg_Move(BlockingActorCheckout)]
							(TCAsyncResult<void> &&_Result) mutable
							{
								if (!_Result)
								{
									if (!Promise.f_IsSet())
										Promise.f_SetException(_Result.f_GetException());
									return;
								}

								fOnDone(&*pRSyncState) > [=, pCleanup = pCleanup, BlockingActorCheckout = fg_Move(BlockingActorCheckout)](TCAsyncResult<void> &&_Result)
									{
										if (!_Result)
										{
											if (!Promise.f_IsSet())
												Promise.f_SetException(_Result.f_GetException());
											return;
										}

										pRSyncState->f_Destroy() > [=, pCleanup = pCleanup](TCAsyncResult<void> &&_Result)
											{
												if (!_Result)
												{
													if (!Promise.f_IsSet())
														Promise.f_SetException(_Result.f_GetException());
													return;
												}

												if (!Promise.f_IsSet())
													Promise.f_SetResult();
											}
										;
									}
								;
							}
						;
					}
				;
			}
		;

		co_return co_await fg_Move(Promise.m_Future);
	}
}
