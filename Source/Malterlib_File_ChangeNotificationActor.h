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
			
			NConcurrency::TCContinuation<NConcurrency::CActorCallback> f_RegisterForChanges
				(
					NMib::NStr::CStr const &_Path
					, NMib::NFile::EFileChange _OpenFlags
					, NFunction::TCFunction<void (CFileChangeNotification::CNotification const &_Change)> const &_fOnChange
					, NConcurrency::TCActor<NConcurrency::CActor> const &_Actor
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
