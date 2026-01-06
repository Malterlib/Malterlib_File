// Copyright © 2015 Hansoft AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

namespace NMib::NFile
{
	enum ERSyncFlag
	{
		ERSyncFlag_None = 0
		, ERSyncFlag_ClientTruncateOutput = DMibBit(0)
		, ERSyncFlag_UseSHA256 = DMibBit(1)
	};

	class CRSyncServer
	{
		class CImplementation;
		NStorage::TCUniquePointer<CImplementation> mp_pImpl;

	public:
		CRSyncServer(NStream::CBinaryStream &_FileToSend, uint32 _MaxPacketSize, ERSyncFlag _Flags = ERSyncFlag_None);
		~CRSyncServer();

		void f_SetSyncStream(NStream::CBinaryStream *_pFileToSend);

		bool f_ProcessPacket(NContainer::CIOByteVector const &_ClientData, NContainer::CIOByteVector &_ToSendToClient, NFunction::TCFunction<void ()> const &_fCheckAbort);
	};

	class CRSyncClient
	{
		class CImplementation;
		NStorage::TCUniquePointer<CImplementation> mp_pImpl;

	public:
		CRSyncClient
			(
				NStream::CBinaryStream &_OldFile
				, NStream::CBinaryStream &_NewFile
				, uint32 _MinChunkSize
				, uint32 _MaxChunkSize
				, uint32 _MaxPacketSize
				, uint32 _MaxQueue
				, NStream::CBinaryStream *_pTempStream = nullptr
				, ERSyncFlag _Flags = ERSyncFlag_ClientTruncateOutput
			)
		;
		~CRSyncClient();

		bool f_ProcessPacket
			(
				NContainer::CIOByteVector const &_ServerData
				, NContainer::CIOByteVector &_ToSendToServer
				, bool &_bWantOneMoreProcess
				, NFunction::TCFunction<void ()> const &_fCheckAbort
			)
		;
		uint64 f_GetRawBytes();
		void f_GetProgress(uint32 &_Stage, uint32 &_Stages, uint64 &_BytesTransfered, uint64 &_TotalBytes);
	};
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif
