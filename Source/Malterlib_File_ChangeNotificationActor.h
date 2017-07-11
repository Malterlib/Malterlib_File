// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/Concurrency/ConcurrencyDefines>
#include <Mib/Concurrency/ConcurrencyManager>
#include <Mib/File/File>

namespace NMib
{
	namespace NFile
	{
		class CFileChangeNotificationActor : public NConcurrency::CActor
		{
		public:
			CFileChangeNotificationActor();
			~CFileChangeNotificationActor();
			
			struct CCoalesceSettings
			{
				mint m_nMaxOutstanding = 5;
				fp64 m_Delay = 1.0;
			};
			
			NConcurrency::TCContinuation<NConcurrency::CActorSubscription> f_RegisterForChanges
				(
					NMib::NStr::CStr const &_Path
					, NMib::NFile::EFileChange _OpenFlags
					, NFunction::TCFunction<void (NContainer::TCVector<CFileChangeNotification::CNotification> const &_Changes)> const &_fOnChange
					, NConcurrency::TCActor<NConcurrency::CActor> const &_Actor
					, CCoalesceSettings const &_CoalesceSettings
				)
			;
		private:
			class CInternal;
			NPtr::TCUniquePointer<CInternal> mp_pInternal;
		};
	}
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif
