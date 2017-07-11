// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_ChangeNotificationActor.h"
#include <Mib/Concurrency/ActorCallbackManager>
#include <Mib/Concurrency/Actor/Timer>

namespace NMib::NFile
{
	using namespace NContainer;
	using namespace NConcurrency;
	using namespace NPtr;
	using namespace NStr;
	
	class CFileChangeNotificationActor::CInternal
	{
	public:
		struct CNotification
		{
			struct CAutoDestroy
			{
				CNotification *m_pNotification = nullptr;
				CInternal *m_pInternal = nullptr;
				CAutoDestroy(CNotification *_pNotification, CInternal *_pInternal)
					: m_pNotification(_pNotification)
					, m_pInternal(_pInternal)
				{
				}
				~CAutoDestroy()
				{
					if (m_pNotification)
						m_pInternal->m_Notifications.f_Remove(*m_pNotification);
				}
			};
			
			CNotification(NConcurrency::CActor *_pActor, CCoalesceSettings const &_CoalesceSettings)
				: m_CallbackManager(_pActor, false)
				, m_pDestroyed(fg_Construct(false))
				, m_CoalesceSettings(_CoalesceSettings)
			{
			}
			
			~CNotification()
			{
				if (m_pDestroyed)
					*m_pDestroyed = true;
			}

			TCSharedPointer<bool> m_pDestroyed;
			TCUniquePointer<CFileChangeNotifier> m_pNotifier;
			TCActorSubscriptionManager<void (TCVector<CFileChangeNotification::CNotification> const &_Changes), false, CAutoDestroy> m_CallbackManager;
			
			TCVector<CFileChangeNotification::CNotification> m_Changes;
			TCMap<CStr, EFileChangeNotification> m_LastChange;
			CCoalesceSettings m_CoalesceSettings;
			mint m_nOutstanding = 0;
			bool m_bScheduledTimeout = false;
		};
		
		void fp_HandleNotification(CNotification *_pNotification, CFileChangeNotification::CNotification const &_Change);
		void fp_SendNotifications(CNotification *_pNotification);
		
		NContainer::TCLinkedList<CNotification> m_Notifications;
	};
	
	CFileChangeNotificationActor::CFileChangeNotificationActor()
		: mp_pInternal(fg_Construct())
	{
	}

	CFileChangeNotificationActor::~CFileChangeNotificationActor()
	{
	}
		
	NConcurrency::TCContinuation<NConcurrency::CActorSubscription> CFileChangeNotificationActor::f_RegisterForChanges
		(
			NMib::NStr::CStr const &_Path
			, NMib::NFile::EFileChange _OpenFlags
			, NFunction::TCFunction<void (NContainer::TCVector<CFileChangeNotification::CNotification> const &_Changes)> const &_fOnChange
			, NConcurrency::TCActor<NConcurrency::CActor> const &_Actor
			, CCoalesceSettings const &_CoalesceSettings
		)
	{
		if (_CoalesceSettings.m_nMaxOutstanding == 0)
			return DMibErrorInstance("CCoalesceSettings::m_nMaxOutstanding has to be 1 or higher");
		
		NConcurrency::TCContinuation<NConcurrency::CActorSubscription> Continuation;
		auto &Internal = *mp_pInternal;
		
		try
		{				
			auto *pNotification = &(Internal.m_Notifications.f_Insert(fg_Construct(this, _CoalesceSettings)));
			auto Callback = pNotification->m_CallbackManager.f_Register(_Actor, _fOnChange, pNotification, &Internal);
			
			pNotification->m_pNotifier = 
				fg_Construct
				(
					_Path
					, _OpenFlags
					, 
					[
						this
						, pNotification
						, pDestroyed = pNotification->m_pDestroyed
						, ThisWeak = NConcurrency::fg_ThisActor(this).f_Weak()
					]
					(CFileChangeNotification::CNotification const &_Change)
					{
						auto ThisActor = ThisWeak.f_Lock();
						
						if (!ThisActor)
							return;
						
						NConcurrency::g_Dispatch(ThisActor) > [=]
							{
								if (*pDestroyed)
									return;
								
								auto &Internal = *mp_pInternal;
								
								Internal.fp_HandleNotification(pNotification, _Change);
							}
							> NConcurrency::fg_DiscardResult()
						;					
					}
				)
			;
			
			Continuation.f_SetResult(fg_Move(Callback));
			return Continuation;
		}
		catch (NException::CException const &)
		{
			Continuation.f_SetCurrentException();
			return Continuation;
		}
		
		return Continuation;
	}
	
	void CFileChangeNotificationActor::CInternal::fp_HandleNotification(CNotification *_pNotification, CFileChangeNotification::CNotification const &_Change)
	{
		auto &Notification = *_pNotification;
		auto &LastChange = *Notification.m_LastChange(_Change.m_Path, EFileChangeNotification_Undefined);
		
		if (LastChange == EFileChangeNotification_Modified && _Change.m_Notification == EFileChangeNotification_Modified)
			return;
		
		LastChange = _Change.m_Notification;
		
		Notification.m_Changes.f_Insert(_Change);
		
		if (Notification.m_CoalesceSettings.m_Delay > 0.0)
		{
			if (Notification.m_bScheduledTimeout)
				return;
			Notification.m_bScheduledTimeout = true;
			fg_Timeout(Notification.m_CoalesceSettings.m_Delay) > [this, _pNotification, pDestroyed = Notification.m_pDestroyed]
				{
					if (*pDestroyed)
						return;
			
					_pNotification->m_bScheduledTimeout = false;
					
					fp_SendNotifications(_pNotification);
				}
			;
		}
		else
			fp_SendNotifications(_pNotification);
	}
	
	void CFileChangeNotificationActor::CInternal::fp_SendNotifications(CNotification *_pNotification)
	{
		auto &Notification = *_pNotification;
		if (Notification.m_nOutstanding >= Notification.m_CoalesceSettings.m_nMaxOutstanding)
			return;

		TCVector<CFileChangeNotification::CNotification> Changes = fg_Move(Notification.m_Changes);
		Notification.m_LastChange.f_Clear();
		
		if (Changes.f_IsEmpty())
			return;
		
		++Notification.m_nOutstanding;
		auto pCleanup = g_OnScopeExitActor > [this, _pNotification, pDestroyed = Notification.m_pDestroyed]
			{
				if (*pDestroyed)
					return;
				
				--_pNotification->m_nOutstanding;
				fp_SendNotifications(_pNotification);
			}
		;
		
		Notification.m_CallbackManager.f_Call(Changes) > [pCleanup](auto &&)
			{
			}
		;

		return;
	}
}
