// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Mib/Concurrency/ConcurrencyDefines>
#include <Mib/Concurrency/ConcurrencyManager>
#include <Mib/File/File>
#include <Mib/Concurrency/ActorFunctor>

namespace NMib::NFile
{
	struct CFileChangeNotificationActor : public NConcurrency::CActor
	{
		struct CCoalesceSettings
		{
			umint m_nMaxOutstanding = 5;
			fp64 m_Delay = 1.0;
		};

		CFileChangeNotificationActor();
		~CFileChangeNotificationActor();

		auto f_RegisterForChanges
			(
				NMib::NStr::CStr _Path
				, NMib::NFile::EFileChange _OpenFlags
				, NConcurrency::TCActorFunctor<NConcurrency::TCFuture<void> (NContainer::TCVector<CFileChangeNotification::CNotification> _Changes)> _fOnChange
				, CCoalesceSettings _CoalesceSettings
			)
			-> NConcurrency::TCFuture<NConcurrency::CActorSubscription>
		;

		static constexpr NConcurrency::EPriority mc_Priority = NConcurrency::EPriority_NormalHighCPU; // Tree scans can consume substantial CPU without requiring low latency.

	private:
		struct CInternal;
		NStorage::TCUniquePointer<CInternal> mp_pInternal;
	};
}

#ifndef DMibPNoShortCuts
using namespace NMib::NFile;
#endif
