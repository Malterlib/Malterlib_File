// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_ChangeNotificationActor.h"
#include <Mib/Concurrency/ActorCallbackManager>

namespace NMib
{
	namespace NFile
	{
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
				
				NPtr::TCSharedPointer<bool> m_pDestroyed;
				NPtr::TCUniquePointer<CFileChangeNotifier> m_pNotifier;
				NConcurrency::TCActorCallbackManager<void (CFileChangeNotification::CNotification const &_Change), false, CAutoDestroy> m_CallbackManager;
				CNotification(NConcurrency::CActor *_pActor)
					: m_CallbackManager(_pActor, false)
					, m_pDestroyed(fg_Construct(false))
				{
				}
				~CNotification()
				{
					if (m_pDestroyed)
						*m_pDestroyed = true;
				}
			};
			
			NContainer::TCLinkedList<CNotification> m_Notifications;
		};
		
		CFileChangeNotificationActor::CFileChangeNotificationActor()
			: mp_pInternal(fg_Construct())
		{
		}

		CFileChangeNotificationActor::~CFileChangeNotificationActor()
		{
		}
			
		NConcurrency::TCContinuation<NConcurrency::CActorCallback> CFileChangeNotificationActor::f_RegisterForChanges
			(
				NMib::NStr::CStr const &_Path
				, NMib::NFile::EFileChange _OpenFlags
				, NFunction::TCFunction<void (CFileChangeNotification::CNotification const &_Change)> const &_fOnChange
				, NConcurrency::TCActor<NConcurrency::CActor> const &_Actor
			)
		{
			NConcurrency::TCContinuation<NConcurrency::CActorCallback> Continuation;
			auto &Internal = *mp_pInternal;
			
			try
			{				
				auto *pNotification = &(Internal.m_Notifications.f_Insert(fg_Construct(this)));
				auto Callback = pNotification->m_CallbackManager.f_Register(_Actor, _fOnChange, pNotification, &Internal);
				
				pNotification->m_pNotifier = 
					fg_Construct
					(
						_Path
						, _OpenFlags
						, 
						[
							pNotification
							, pDestroyed = pNotification->m_pDestroyed
							, ThisWeak = NConcurrency::fg_ThisActor(this).f_Weak()
						]
						(CFileChangeNotification::CNotification const &_Change)
						{
							auto ThisActor = ThisWeak.f_Lock();
							
							if (!ThisActor)
								return;
							
							ThisActor
								(
									&NConcurrency::CActor::f_Dispatch
									, [=]
									{
										if (*pDestroyed)
											return;
										pNotification->m_CallbackManager(_Change);
									}
								)
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
	}
}
