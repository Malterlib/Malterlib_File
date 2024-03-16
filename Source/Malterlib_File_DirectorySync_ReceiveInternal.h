// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_DirectorySync.h"
#include <Mib/Concurrency/ActorSubscription>
#include <Mib/Cryptography/RandomID>
#include <Mib/File/RSync>

namespace NMib::NFile
{
	using namespace NAtomic;
	using namespace NStr;
	using namespace NConcurrency;
	using namespace NContainer;
	using namespace NCryptography;
	using namespace NFunction;
	using namespace NStorage;
	using namespace NFile;
	using namespace NTime;
	
	struct CDirectorySyncReceive::CInternal : public CActorInternal
	{
		struct CByteStats
		{
			uint64 m_nIncoming = 0;
			uint64 m_nOutgoing = 0;
		};
		
		struct CRunningSyncState
		{
			CRunningSyncState();
			~CRunningSyncState();
			TCFuture<void> f_Destroy();

			CIntrusiveRefCountWithWeak m_RefCount;

			TCUniquePointer<NStream::CBinaryStream> m_pSourceDestinationStream = fg_Construct<TCBinaryStreamFile<>>();
			TCUniquePointer<NStream::CBinaryStream> m_pSourceStream = fg_Construct<TCBinaryStreamFile<>>();
			TCUniquePointer<NStream::CBinaryStream> m_pTempStream = fg_Construct<TCBinaryStreamFile<>>();

			TCUniquePointer<CRSyncClient> m_pClient;
			
			CDirectorySyncClient::FRunRSync m_fRunProtocol;
			
			TCVector<CStr> m_TempFiles;

			CStr m_DestinationFilename;
			
			CByteStats m_ByteStats;
			bool m_bFinished = false;
		};
		
		CInternal(CDirectorySyncReceive *_pThis, CConfig &&_Config, TCDistributedActorInterface<CDirectorySyncClient> &&_Client);
		~CInternal();

		TCFuture<void> f_RunRSyncProtocol(TCSharedPointerSupportWeak<CRunningSyncState> _pState);
		
		TCFuture<void> f_SyncManifest();
		TCFuture<void> f_HandleExcessFiles();

		TCFuture<void> f_SyncFile(CStr const &_FileName);

		TCFuture<void> f_RSync
			(
				TCFunctionMutable<bool (CRunningSyncState *_pState)> _fInitRSync
				, TCFunctionMutable<TCFuture<void> (CRunningSyncState *_pState)> _fOnDone
				, TCFunctionMutable<TCFuture<CDirectorySyncClient::FRunRSync> (CActorSubscription &&_Subscription)> _fStartRSync
			)
		;
		
		void f_RunFileSyncs(TCPromise<void> const &_Promise);
		static void fs_CheckDestroy(TCSharedPointer<NAtomic::TCAtomic<bool>> const &_pDestroyed);

		CDirectorySyncReceive *m_pThis;
		TCSharedPointer<CConfig> m_pConfig;
		TCDistributedActorInterface<CDirectorySyncClient> m_Client;
		TCMap<CStr, TCSharedPointerSupportWeak<CRunningSyncState>> m_RSyncStates;
		TCSet<CStr> m_PendingFileSyncs;
		CClock m_Clock{true};
		TCSharedPointer<CDirectoryManifest> m_pManifest;
		CDirectorySyncStats m_Stats;
		CStr m_SyncErrors;
		TCSharedPointer<TCAtomic<bool>> m_pDestroyed = fg_Construct(false);
		bool m_bStartedSync = false;
		mint m_nRunningSyncs = 0;
		
	private:
		static void fsp_RunRSyncProtocol
			(
				TCSharedPointerSupportWeak<CRunningSyncState> const &_pState
				, CSecureByteVector &&_ServerPacket
				, TCPromise<CByteStats> const &_Promise
			)
		;
	};
}
