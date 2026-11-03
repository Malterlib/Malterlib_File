// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Malterlib_File_ChangeNotificationActor.h"
#include <Mib/Concurrency/ActorSubscription>

namespace NMib::NFile
{
	using namespace NContainer;
	using namespace NConcurrency;
	using namespace NStorage;
	using namespace NStr;

	struct CFileChangeNotificationActor::CInternal : public CActorInternal
	{
		struct CNotification
		{
			struct CSavedChange
			{
				CFileChangeNotification::CNotification m_Notification;
				uint64 m_Sequence = 0;
			};

			CNotification(NConcurrency::CActor *_pActor, CCoalesceSettings &&_CoalesceSettings)
				: m_pDestroyed(fg_Construct(false))
				, m_CoalesceSettings(fg_Move(_CoalesceSettings))
			{
			}

			~CNotification()
			{
				if (m_pDestroyed)
					*m_pDestroyed = true;
			}

			TCSharedPointer<bool> m_pDestroyed;
			TCUniquePointer<CFileChangeNotifier> m_pNotifier;

			TCVector<CSavedChange> m_Changes;
			TCMap<CStr, EFileChangeNotification> m_LastChange;
			CCoalesceSettings m_CoalesceSettings;
			TCActorFunctor<TCFuture<void> (NContainer::TCVector<CFileChangeNotification::CNotification> _Changes)> m_fOnChange;
			uint64 m_Sequence = 0;
			umint m_nOutstanding = 0;
			bool m_bScheduledTimeout = false;
		};

		void f_HandleNotification(CNotification *_pNotification, CFileChangeNotification::CNotification const &_Change);
		void f_SendNotifications(CNotification *_pNotification, bool _bForce);

		NContainer::TCLinkedList<CNotification> m_Notifications;
	};

	CFileChangeNotificationActor::CFileChangeNotificationActor()
		: mp_pInternal(fg_Construct())
	{
	}

	CFileChangeNotificationActor::~CFileChangeNotificationActor()
	{
	}

	auto CFileChangeNotificationActor::f_RegisterForChanges
		(
			NMib::NStr::CStr _Path
			, NMib::NFile::EFileChange _OpenFlags
			, NConcurrency::TCActorFunctor<NConcurrency::TCFuture<void> (NContainer::TCVector<CFileChangeNotification::CNotification> _Changes)> _fOnChange
			, CCoalesceSettings _CoalesceSettings
		)
		-> NConcurrency::TCFuture<NConcurrency::CActorSubscription>
	{
		if (_CoalesceSettings.m_nMaxOutstanding == 0)
			co_return DMibErrorInstance("CCoalesceSettings::m_nMaxOutstanding has to be 1 or higher");

		auto &Internal = *mp_pInternal;

		auto CaptureScope = co_await g_CaptureExceptions;

		auto *pNotification = &(Internal.m_Notifications.f_Insert(fg_Construct(this, fg_Move(_CoalesceSettings))));
		pNotification->m_fOnChange = fg_Move(_fOnChange);

		auto Callback = g_ActorSubscription / [this, pNotification, pDestroyed = pNotification->m_pDestroyed]() -> TCFuture<void>
			{
				if (*pDestroyed)
					co_return {};

				auto &Internal = *mp_pInternal;
				pNotification->m_pNotifier.f_Clear();
				Internal.f_SendNotifications(pNotification, true);
				Internal.m_Notifications.f_Remove(*pNotification);
				co_return {};
			}
		;

		NFunction::TCFunction<void (CFileChangeNotification::CNotification const &_Change)> fNotify =
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

				NConcurrency::g_Dispatch(ThisActor) / [=, this]
					{
						if (*pDestroyed)
							return;

						auto &Internal = *mp_pInternal;

						Internal.f_HandleNotification(pNotification, _Change);
					}
					> NConcurrency::g_DiscardResult
				;
			}
		;

		// The subscription removes the notification when it is destroyed, so re-validate it on every resume
		auto OnResume = co_await fg_OnResume
			(
				[&, pDestroyed = pNotification->m_pDestroyed]() -> NException::CExceptionPointer
				{
					if (*pDestroyed)
					{
						pNotification = nullptr;
						return DMibErrorInstance("Change notification was removed");
					}
					return {};
				}
			)
		;

		// Opening a notification may scan the watched tree and block.
		auto BlockingActorCheckout = fg_BlockingActor();

		pNotification->m_pNotifier = co_await
			(
				g_Dispatch(BlockingActorCheckout) / [_Path, _OpenFlags, fNotify = fg_Move(fNotify)]() -> TCUniquePointer<CFileChangeNotifier>
				{
					return fg_Construct(_Path, _OpenFlags, fNotify);
				}
			)
		;

		co_return fg_Move(Callback);
	}

	void CFileChangeNotificationActor::CInternal::f_HandleNotification(CNotification *_pNotification, CFileChangeNotification::CNotification const &_Change)
	{
		auto &Notification = *_pNotification;
		auto &LastChange = *Notification.m_LastChange(_Change.m_Path, EFileChangeNotification_Undefined);

		if (LastChange == EFileChangeNotification_Modified && _Change.m_Notification == EFileChangeNotification_Modified)
			return;

		LastChange = _Change.m_Notification;

		auto Sequence = ++Notification.m_Sequence;

#ifdef DMibFileChangeNotificationsDebug
		switch (_Change.m_Notification)
		{
		case EFileChangeNotification_Unknown: DMibFileChangeNotificationsDebugOut("@@@ {} Unknown {}", Sequence, _Change.m_Path); break;
		case EFileChangeNotification_Added: DMibFileChangeNotificationsDebugOut("@@@ {} Added {}", Sequence, _Change.m_Path); break;
		case EFileChangeNotification_Removed: DMibFileChangeNotificationsDebugOut("@@@ {} Removed {}", Sequence, _Change.m_Path); break;
		case EFileChangeNotification_Modified: DMibFileChangeNotificationsDebugOut("@@@ {} Modified {}", Sequence, _Change.m_Path); break;
		case EFileChangeNotification_Renamed: DMibFileChangeNotificationsDebugOut("@@@ {} Renamed {} -> {}", Sequence, _Change.m_PathFrom, _Change.m_Path); break;
		case EFileChangeNotification_Undefined: break;
		}
#endif

		Notification.m_Changes.f_Insert({_Change, Sequence});

		if (Notification.m_CoalesceSettings.m_Delay > 0.0)
		{
			if (Notification.m_bScheduledTimeout)
				return;
			Notification.m_bScheduledTimeout = true;
			fg_Timeout(Notification.m_CoalesceSettings.m_Delay) > [this, _pNotification, pDestroyed = Notification.m_pDestroyed]() -> TCFuture<void>
				{
					if (*pDestroyed)
						co_return {};

					_pNotification->m_bScheduledTimeout = false;

					f_SendNotifications(_pNotification, false);

					co_return {};
				}
			;
		}
		else
			f_SendNotifications(_pNotification, false);
	}

	void CFileChangeNotificationActor::CInternal::f_SendNotifications(CNotification *_pNotification, bool _bForce)
	{
		auto &Notification = *_pNotification;
		if (Notification.m_nOutstanding >= Notification.m_CoalesceSettings.m_nMaxOutstanding && !_bForce)
			return;

		auto Changes = fg_Move(Notification.m_Changes);
		Notification.m_LastChange.f_Clear();

		if (Changes.f_IsEmpty())
			return;

		++Notification.m_nOutstanding;
		auto pCleanup = g_OnScopeExitActor / [this, _pNotification, pDestroyed = Notification.m_pDestroyed]
			{
				if (*pDestroyed)
					return;

				--_pNotification->m_nOutstanding;
				f_SendNotifications(_pNotification, false);
			}
		;

		TCVector<CFileChangeNotification::CNotification> Notifications;
		for (auto &Change : Changes)
		{
			Notifications.f_Insert(Change.m_Notification);

#ifdef DMibFileChangeNotificationsDebug
			ch8 const *pForced = _bForce ? " FORCED" : "";

			switch (Change.m_Notification.m_Notification)
			{
			case EFileChangeNotification_Unknown: DMibFileChangeNotificationsDebugOut("=== {}{} Unknown {}", Change.m_Sequence, pForced, Change.m_Notification.m_Path); break;
			case EFileChangeNotification_Added: DMibFileChangeNotificationsDebugOut("=== {}{} Added {}", Change.m_Sequence, pForced, Change.m_Notification.m_Path); break;
			case EFileChangeNotification_Removed: DMibFileChangeNotificationsDebugOut("=== {}{} Removed {}", Change.m_Sequence, pForced, Change.m_Notification.m_Path); break;
			case EFileChangeNotification_Modified: DMibFileChangeNotificationsDebugOut("=== {}{} Modified {}", Change.m_Sequence, pForced, Change.m_Notification.m_Path); break;
			case EFileChangeNotification_Renamed:
				DMibFileChangeNotificationsDebugOut("=== {}{} Renamed {} -> {}", Change.m_Sequence, pForced, Change.m_Notification.m_PathFrom, Change.m_Notification.m_Path);
				break;
			case EFileChangeNotification_Undefined: break;
			}
#endif
		}

		Notification.m_fOnChange(Notifications) > [pCleanup](auto &&_Result)
			{
			}
		;

		return;
	}
}
