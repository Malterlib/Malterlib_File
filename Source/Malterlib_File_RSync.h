// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

namespace NMib
{
	namespace NDataProcessing
	{
		class CRSyncServer
		{
			class CImplementation;
			NPtr::TCUniquePointer<CImplementation> mp_pImpl;
			
		public:
			CRSyncServer(NStream::CBinaryStream &_FileToSend, uint32 _MaxPacketSize);
			~CRSyncServer();

			void f_SetSyncStream(NStream::CBinaryStream *_pFileToSend);
			
			bint f_ProcessPacket(NContainer::TCVector<uint8> const &_ClientData, NContainer::TCVector<uint8> &_ToSendToClient, bint &_bWantOneMoreProcess);
		};

		class CRSyncClient
		{
			class CImplementation;
			NPtr::TCUniquePointer<CImplementation> mp_pImpl;
			
		public:
			CRSyncClient(NStream::CBinaryStream &_OldFile, NStream::CBinaryStream &_NewFile, uint32 _MinChunkSize, uint32 _MaxChunkSize, uint32 _MaxPacketSize);
			~CRSyncClient();
			
			bint f_ProcessPacket(NContainer::TCVector<uint8> const &_ServerData, NContainer::TCVector<uint8> &_ToSendToServer, bint &_bWantOneMoreProcess);
			uint64 f_GetRawBytes();
			void f_GetProgress(uint32 &_Stage, uint32 &_Stages, uint64 &_BytesTransfered, uint64 &_TotalBytes);
		};
	}
}

