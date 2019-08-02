// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_DirectorySync.h"
#include "Malterlib_File_DirectorySync_ReceiveInternal.h"

namespace NMib::NFile
{
	void CDirectorySyncReceive::CInternal::fsp_RunRSyncProtocol
		(
			TCSharedPointerSupportWeak<CRunningSyncState> const &_pState
			, CSecureByteVector &&_ServerPacket
			, TCPromise<CByteStats> const &_Promise
		)
	{
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
			CSecureByteVector ToSendToServer;
			try
			{
				if (State.m_pClient->f_ProcessPacket(_ServerPacket, ToSendToServer, bWantOneMoreProcess))
					bDone = true;
			}
			catch (NException::CException const &_Exception)
			{
				if (!_Promise.f_IsSet())
					_Promise.f_SetException(DMibErrorInstance(fg_Format("Exceptio running RSync protocol: {}", _Exception.f_GetErrorStr())));
				return;
			}
			
			_ServerPacket.f_Clear();
			if (!ToSendToServer.f_IsEmpty())
			{
				State.m_ByteStats.m_nOutgoing += ToSendToServer.f_GetLen();

				State.m_fRunProtocol(fg_Move(ToSendToServer)) > [_pState, _Promise](TCAsyncResult<CSecureByteVector> &&_ServerPacket)
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
		CByteStats Stats = co_await
			(
				g_Dispatch(m_FileActor) / [_pState]
				{
					TCPromise<CByteStats> RunPromise;
					fsp_RunRSyncProtocol(_pState, {}, RunPromise);
					return RunPromise.f_MoveFuture();
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
		TCPromise<void> Promise;

		CStr RSyncID = fg_RandomID();
		auto &pRSyncState = m_RSyncStates[RSyncID] = fg_Construct();
		pRSyncState->m_FileActor = m_FileActor;
		
		auto pCleanup = g_OnScopeExitActor > [this, RSyncID, pRSyncState]
			{
				m_RSyncStates.f_Remove(RSyncID);
				pRSyncState->f_Destroy() > fg_DiscardResult();
			}
		;

		g_Dispatch(m_FileActor) / [=, fInitRSync = fg_Move(_fInitRSync)]() mutable
			{
				return fInitRSync(&*pRSyncState);
			}
			> Promise / [=, fOnDone = fg_Move(_fOnDone), fStartRSync = fg_Move(_fStartRSync)](bool _bAlreadySynced) mutable
			{
				if (_bAlreadySynced)
				{
					fOnDone(&*pRSyncState) > Promise / [=]()
						{
							pRSyncState->f_Destroy() > Promise / [=, pCleanup = pCleanup]()
								{
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
					> Promise / [=, fOnDone = fg_Move(fOnDone)](CDirectorySyncClient::FRunRSync &&_fRunRSync) mutable
					{
						auto &RSyncState = *pRSyncState;
						RSyncState.m_fRunProtocol = fg_Move(_fRunRSync);
						f_RunRSyncProtocol(pRSyncState) > Promise / [=, fOnDone = fg_Move(fOnDone)]() mutable
							{
								fOnDone(&*pRSyncState) > Promise / [=]()
									{
										pRSyncState->f_Destroy() > Promise / [=, pCleanup = pCleanup]()
											{
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
		return Promise.f_MoveFuture();
	}
}
