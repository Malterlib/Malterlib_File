// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Malterlib_File_RSync.h"
#include <Mib/Container/Regions>

#define DFileMapDebug 0
#ifdef DMibDebug
	// Not compatible between versions, so take care
//#define DPacketOrderDebug
#endif

namespace NMib::NFile
{
	using namespace NContainer;
	using namespace NCryptography;
	using namespace NMisc;
	using namespace NMemory;
	enum EPacketType
	{
		EPacketType_ClientInit
		, EPacketType_ServerInit
		, EPacketType_ClientChecksums
		, EPacketType_ServerChecksums
		, EPacketType_ClientOutstandingRanges
		, EPacketType_ServerOutstandingRanges
	};

	struct CPacket
	{
		CPacket(EPacketType _PacketType);
		virtual ~CPacket();

		virtual void f_Feed(NStream::CBinaryStreamDefault &_Stream) const = 0;

		void f_Consume(NStream::CBinaryStreamDefault &_Stream);

		CIOByteVector f_GetVector
			(
#ifdef DPacketOrderDebug
				uint32 _PacketID
#endif
			)
		;

		static NStorage::TCUniquePointer<CPacket> fs_DecodePacket(CIOByteVector const &_Data);

		EPacketType m_PacketType;
#ifdef DPacketOrderDebug
		uint32 m_PacketID;
#endif
	};

	CPacket::CPacket(EPacketType _PacketType)
		: m_PacketType(_PacketType)
	{
	}

	CPacket::~CPacket()
	{
	}

	void CPacket::f_Consume(NStream::CBinaryStreamDefault &_Stream)
	{
#ifdef DPacketOrderDebug
		_Stream >> m_PacketID;
#endif
		// _Stream >> m_PacketType;
	}

	CIOByteVector CPacket::f_GetVector
		(
#ifdef DPacketOrderDebug
			uint32 _PacketID
#endif
		)
	{
#ifdef DPacketOrderDebug
		m_PacketID = _PacketID;
#endif
		NStream::TCBinaryStreamMemory<NStream::CBinaryStreamDefault, CIOByteVector> Stream;
		Stream << *this;
		return Stream.f_MoveVector();
	}

	struct CPacket_ClientInit : public CPacket
	{
		CPacket_ClientInit();

		void f_Feed(NStream::CBinaryStreamDefault &_Stream) const;

		void f_Consume(NStream::CBinaryStreamDefault &_Stream);

		uint32 m_InitialChecksumSize;
	};

	CPacket_ClientInit::CPacket_ClientInit()
		: CPacket(EPacketType_ClientInit)
	{
	}

	void CPacket_ClientInit::f_Feed(NStream::CBinaryStreamDefault &_Stream) const
	{
		CPacket::f_Feed(_Stream);
		_Stream << m_InitialChecksumSize;
	}

	void CPacket_ClientInit::f_Consume(NStream::CBinaryStreamDefault &_Stream)
	{
		CPacket::f_Consume(_Stream);
		_Stream >> m_InitialChecksumSize;
	}

	struct CChecksum
	{
		constexpr CChecksum() = default;

		void f_Feed(NStream::CBinaryStreamDefault &_Stream) const;

		void f_Consume(NStream::CBinaryStreamDefault &_Stream);

		CHashDigest_SHA256_16 m_Digest;
		uint32 m_Running = 0;
	};

	void CChecksum::f_Feed(NStream::CBinaryStreamDefault &_Stream) const
	{
		_Stream << m_Digest;
		_Stream << m_Running;
	}

	void CChecksum::f_Consume(NStream::CBinaryStreamDefault &_Stream)
	{
		_Stream >> m_Digest;
		_Stream >> m_Running;
	}

	static constexpr umint gc_ChecksumNetworkSize = []()
		{
			constexpr CChecksum c_Checksum;

			NStream::TCBinaryStreamNull<> Stream;
			Stream << c_Checksum.m_Digest;
			Stream << c_Checksum.m_Running;

			return Stream.f_GetLength();
		}
		()
	;

	struct CPacket_ServerChecksums : public CPacket
	{
		CPacket_ServerChecksums();

		void f_Feed(NStream::CBinaryStreamDefault &_Stream) const;

		void f_Consume(NStream::CBinaryStreamDefault &_Stream);

		TCVector<CChecksum> m_Checksums;
	};

	CPacket_ServerChecksums::CPacket_ServerChecksums()
		: CPacket(EPacketType_ServerChecksums)
	{
	}

	void CPacket_ServerChecksums::f_Feed(NStream::CBinaryStreamDefault &_Stream) const
	{
		CPacket::f_Feed(_Stream);
		_Stream << m_Checksums;
	}

	void CPacket_ServerChecksums::f_Consume(NStream::CBinaryStreamDefault &_Stream)
	{
		CPacket::f_Consume(_Stream);
		_Stream >> m_Checksums;
	}

	struct COutstandingRange
	{
		void f_Feed(NStream::CBinaryStreamDefault &_Stream) const;

		void f_Consume(NStream::CBinaryStreamDefault &_Stream);

		uint64 m_Start;
		uint64 m_Length;
	};

	void COutstandingRange::f_Feed(NStream::CBinaryStreamDefault &_Stream) const
	{
		_Stream << m_Start;
		_Stream << m_Length;
	}

	void COutstandingRange::f_Consume(NStream::CBinaryStreamDefault &_Stream)
	{
		_Stream >> m_Start;
		_Stream >> m_Length;
	}

	struct CPacket_ClientChecksums : public CPacket
	{
		CPacket_ClientChecksums();

		void f_Feed(NStream::CBinaryStreamDefault &_Stream) const;

		void f_Consume(NStream::CBinaryStreamDefault &_Stream);

		uint32 m_ChunkSize;
		TCVector<COutstandingRange> m_Outstanding;
	};

	CPacket_ClientChecksums::CPacket_ClientChecksums()
		: CPacket(EPacketType_ClientChecksums)
	{
	}

	void CPacket_ClientChecksums::f_Feed(NStream::CBinaryStreamDefault &_Stream) const
	{
		CPacket::f_Feed(_Stream);
		_Stream << m_ChunkSize;
		_Stream << m_Outstanding;
	}

	void CPacket_ClientChecksums::f_Consume(NStream::CBinaryStreamDefault &_Stream)
	{
		CPacket::f_Consume(_Stream);
		_Stream >> m_ChunkSize;
		_Stream >> m_Outstanding;
	}

	struct CPacket_ClientOutstandingRanges : public CPacket
	{
		CPacket_ClientOutstandingRanges();

		void f_Feed(NStream::CBinaryStreamDefault &_Stream) const;

		void f_Consume(NStream::CBinaryStreamDefault &_Stream);

		TCVector<COutstandingRange> m_Outstanding;
		uint32 m_bDone;
	};

	CPacket_ClientOutstandingRanges::CPacket_ClientOutstandingRanges()
		: CPacket(EPacketType_ClientOutstandingRanges)
		, m_bDone(false)
	{
	}

	void CPacket_ClientOutstandingRanges::f_Feed(NStream::CBinaryStreamDefault &_Stream) const
	{
		CPacket::f_Feed(_Stream);
		_Stream << m_Outstanding;
		_Stream << m_bDone;
	}

	void CPacket_ClientOutstandingRanges::f_Consume(NStream::CBinaryStreamDefault &_Stream)
	{
		CPacket::f_Consume(_Stream);
		_Stream >> m_Outstanding;
		_Stream >> m_bDone;
	}

	struct CPacket_ServerOutstandingRanges : public CPacket
	{
		CPacket_ServerOutstandingRanges();

		void f_Feed(NStream::CBinaryStreamDefault &_Stream) const;

		void f_Consume(NStream::CBinaryStreamDefault &_Stream);

		CIOByteVector m_Data;
	};

	CPacket_ServerOutstandingRanges::CPacket_ServerOutstandingRanges()
		: CPacket(EPacketType_ServerOutstandingRanges)
	{
	}

	void CPacket_ServerOutstandingRanges::f_Feed(NStream::CBinaryStreamDefault &_Stream) const
	{
		CPacket::f_Feed(_Stream);
		_Stream << m_Data;
	}

	void CPacket_ServerOutstandingRanges::f_Consume(NStream::CBinaryStreamDefault &_Stream)
	{
		CPacket::f_Consume(_Stream);
		_Stream >> m_Data;
	}

	struct CPacket_ServerInit : public CPacket
	{
		CPacket_ServerInit();

		void f_Feed(NStream::CBinaryStreamDefault &_Stream) const;

		void f_Consume(NStream::CBinaryStreamDefault &_Stream);

		uint64 m_FileSize;
		CPacket_ServerChecksums m_InitialChecksums;
	};

	CPacket_ServerInit::CPacket_ServerInit()
		: CPacket(EPacketType_ServerInit)
	{
	}

	void CPacket_ServerInit::f_Feed(NStream::CBinaryStreamDefault &_Stream) const
	{
		CPacket::f_Feed(_Stream);
		_Stream << m_FileSize;
		_Stream << m_InitialChecksums;
	}

	void CPacket_ServerInit::f_Consume(NStream::CBinaryStreamDefault &_Stream)
	{
		CPacket::f_Consume(_Stream);
		_Stream >> m_FileSize;
		EPacketType Protocol;
		_Stream >> Protocol;
		_Stream >> m_InitialChecksums;
	}

	void CPacket::f_Feed(NStream::CBinaryStreamDefault &_Stream) const
	{
		_Stream << m_PacketType;
#ifdef DPacketOrderDebug
		_Stream << m_PacketID;
#endif
	}

	NStorage::TCUniquePointer<CPacket> CPacket::fs_DecodePacket(CIOByteVector const &_Data)
	{
		NStream::TCBinaryStreamMemoryPtr<> Stream;
		Stream.f_OpenRead(_Data.f_GetArray(), _Data.f_GetLen());
		EPacketType Protocol;
		Stream >> Protocol;

		switch (Protocol)
		{
		case EPacketType_ClientInit:
			{
				NStorage::TCUniquePointer<CPacket_ClientInit> pRet;
				pRet = fg_Construct<CPacket_ClientInit>();
				Stream >> *pRet;
				return fg_Move(pRet);
			}
			break;
		case EPacketType_ServerInit:
			{
				NStorage::TCUniquePointer<CPacket_ServerInit> pRet;
				pRet = fg_Construct<CPacket_ServerInit>();
				Stream >> *pRet;
				return fg_Move(pRet);
			}
			break;
		case EPacketType_ClientChecksums:
			{
				NStorage::TCUniquePointer<CPacket_ClientChecksums> pRet;
				pRet = fg_Construct<CPacket_ClientChecksums>();
				Stream >> *pRet;
				return fg_Move(pRet);
			}
			break;
		case EPacketType_ServerChecksums:
			{
				NStorage::TCUniquePointer<CPacket_ServerChecksums> pRet;
				pRet = fg_Construct<CPacket_ServerChecksums>();
				Stream >> *pRet;
				return fg_Move(pRet);
			}
			break;
		case EPacketType_ClientOutstandingRanges:
			{
				NStorage::TCUniquePointer<CPacket_ClientOutstandingRanges> pRet;
				pRet = fg_Construct<CPacket_ClientOutstandingRanges>();
				Stream >> *pRet;
				return fg_Move(pRet);
			}
			break;
		case EPacketType_ServerOutstandingRanges:
			{
				NStorage::TCUniquePointer<CPacket_ServerOutstandingRanges> pRet;
				pRet = fg_Construct<CPacket_ServerOutstandingRanges>();
				Stream >> *pRet;
				return fg_Move(pRet);
			}
			break;
		}

		return NStorage::TCUniquePointer<CPacket>();
	}

	static constexpr uint32 gc_Offset = 31;

	using CRunningType = uint32;

	uint32 fg_GetRunningSum(void const *_pData, aint _nBytes)
	{
		uint8 const *pIn = (uint8 const *)_pData;

		CRunningType Sum0 = 0;
		CRunningType Sum1 = 0;

		aint iByte = 0;
		for (; iByte < (_nBytes - 4); iByte += 4)
		{
			Sum1 += 4u * (Sum0 + pIn[iByte]) + 3 * pIn[iByte + 1] + 2 * pIn[iByte + 2] + pIn[iByte + 3] + 10u * gc_Offset;
			Sum0 += (pIn[iByte + 0] + pIn[iByte + 1] + pIn[iByte + 2] + pIn[iByte + 3] + 4u * gc_Offset);
		}
		for (; iByte < _nBytes; iByte++)
		{
			Sum0 += (pIn[iByte] + gc_Offset);
			Sum1 += Sum0;
		}
		return ((uint32(Sum0) & uint32(0xFFFF)) | (uint32(Sum1) << 16));
		/*TCBinaryStreamHash<CHash_SHA256_16> Hash;
		Hash << Sum0;
		Hash << Sum1;
		auto Digest = Hash.f_GetDigest();
		NStream::TCBinaryStreamMemoryPtr<> Stream;
		Stream.f_OpenRead(Digest.f_GetData(), 16);
		uint32 Hash32;
		Stream >> Hash32;
		return Hash32;*/
	}

	static_assert(sizeof(CHashDigest_MD5) == sizeof(CHashDigest_SHA256_16));

	CHashDigest_SHA256_16 fg_GetMD5Checksum(CHash_MD5 &_Hash, void const *_pData, umint _nBytes)
	{
		_Hash.f_Reset();
		_Hash.f_AddData(_pData, _nBytes);
		auto TempDigest = fg_Move(_Hash).f_GetDigest();
		CHashDigest_SHA256_16 Return;
		fg_MemCopy(Return.f_GetData(), TempDigest.f_GetData(), TempDigest.mc_Size);
		return Return;
	}

	CHashDigest_SHA256_16 fg_GetMD5Checksum(CHash_MD5 &_Hash, void const *_pData0, umint _nBytes0, void const *_pData1, umint _nBytes1)
	{
		_Hash.f_Reset();
		_Hash.f_AddData(_pData0, _nBytes0);
		_Hash.f_AddData(_pData1, _nBytes1);
		auto TempDigest = fg_Move(_Hash).f_GetDigest();
		CHashDigest_SHA256_16 Return;
		fg_MemCopy(Return.f_GetData(), TempDigest.f_GetData(), TempDigest.mc_Size);
		return Return;
	}

	CHashDigest_SHA256_16 fg_GetSHA256Checksum(CHash_SHA256_16 &_Hash, void const *_pData, umint _nBytes)
	{
		_Hash.f_Reset();
		_Hash.f_AddData(_pData, _nBytes);
		return fg_Move(_Hash);
	}

	CHashDigest_SHA256_16 fg_GetSHA256Checksum(CHash_SHA256_16 &_Hash, void const *_pData0, umint _nBytes0, void const *_pData1, umint _nBytes1)
	{
		_Hash.f_Reset();
		_Hash.f_AddData(_pData0, _nBytes0);
		_Hash.f_AddData(_pData1, _nBytes1);
		return fg_Move(_Hash);
	}

	struct CRollsum
	{
		CRollsum();

		void f_Advance(uint8 _Out, uint8 _In);

		void f_RollIn(uint8 _Byte);

		void f_RollIn(uint8 const *_pData, aint _nBytes);

		void f_RollOut(uint8 _Byte);

		uint32 f_Digest();

		uint32 m_Count;
		CRunningType m_Sum0;
		CRunningType m_Sum1;
	};

	CRollsum::CRollsum()
		: m_Count(0)
		, m_Sum0(0)
		, m_Sum1(0)
	{
	}

	void CRollsum::f_Advance(uint8 _Out, uint8 _In)
	{
		m_Sum0 += _In - _Out;
		m_Sum1 += m_Sum0 - m_Count * (_Out + gc_Offset);
	}

	void CRollsum::f_RollIn(uint8 _Byte)
	{
		m_Sum0 += ((_Byte) + gc_Offset);
		m_Sum1 += m_Sum0;
		++m_Count;
	}

	void CRollsum::f_RollIn(uint8 const *_pData, aint _nBytes)
	{
		uint8 const *pIn = (uint8 const *)_pData;

		aint iByte = 0;
		aint nBytes = (_nBytes - 4);
		for (; iByte < nBytes; iByte += 4)
		{
			m_Sum1 += 4u * (m_Sum0 + pIn[iByte]) + 3 * pIn[iByte + 1] + 2 * pIn[iByte + 2] + pIn[iByte + 3] + 10u * gc_Offset;
			m_Sum0 += (pIn[iByte + 0] + pIn[iByte + 1] + pIn[iByte + 2] + pIn[iByte + 3] + 4u * gc_Offset);
		}
		for (; iByte < _nBytes; iByte++)
		{
			m_Sum0 += (pIn[iByte] + gc_Offset);
			m_Sum1 += m_Sum0;
		}
		m_Count += _nBytes;
	}

	void CRollsum::f_RollOut(uint8 _Byte)
	{
		m_Sum0 -= (_Byte + gc_Offset);
		m_Sum1 -= m_Count * (_Byte + gc_Offset);
		--m_Count;
	}

	uint32 CRollsum::f_Digest()
	{
		/*TCBinaryStreamHash<CHash_SHA256_16> Hash;
		Hash << m_Sum0;
		Hash << m_Sum1;
		auto Digest = Hash.f_GetDigest();
		NStream::TCBinaryStreamMemoryPtr<> Stream;
		Stream.f_OpenRead(Digest.f_GetData(), 16);
		uint32 Hash32;
		Stream >> Hash32;
		return Hash32;*/
		return ((uint32(m_Sum0) & uint32(0xFFFF)) | uint32(m_Sum1) << 16);
	}

	/////////////////////////////////////
	// Server
	/////////////////////////////////////

	struct CRSyncServer::CImplementation
	{
		CImplementation(NStream::CBinaryStream &_FileToSend, uint32 _MaxPacketSize, ERSyncFlag _Flags);

		void f_SetFileStream(NStream::CBinaryStream *_pFileToSend);

		void f_GetChecksums(TCVector<CChecksum> &_Destination, TCVector<COutstandingRange> const &_Outstanding, uint32 _ChunkSize, NFunction::TCFunction<void ()> const &_fCheckAbort);

		bool f_ProcessPacket(CIOByteVector const &_ClientData, CIOByteVector &_ToSendToClient, NFunction::TCFunction<void ()> const &_fCheckAbort);

	private:
		enum EServerMode
		{
			EServerMode_Init
			, EServerMode_SyncChecksum
		};

#ifdef DPacketOrderDebug
		void fp_CheckPacket(CPacket const &_Packet);
#endif

		NStream::CBinaryStream *mp_pFileToSend = nullptr;
		uint32 mp_MaxPacketSize = 0;

		ERSyncFlag mp_Flags = ERSyncFlag_None;

		EServerMode mp_ServerMode = EServerMode_Init;
		bool mp_bDone = false;

#ifdef DPacketOrderDebug
		uint32 mp_LastSentPacketID = 0;
		uint32 mp_LastReceivedPacketID = 0;
#endif
	};

	CRSyncServer::CImplementation::CImplementation(NStream::CBinaryStream &_FileToSend, uint32 _MaxPacketSize, ERSyncFlag _Flags)
		: mp_pFileToSend(&_FileToSend)
		, mp_MaxPacketSize(_MaxPacketSize)
		, mp_Flags(_Flags)
	{
	}

	void CRSyncServer::CImplementation::f_SetFileStream(NStream::CBinaryStream *_pFileToSend)
	{
		mp_pFileToSend = _pFileToSend;
	}

	void CRSyncServer::CImplementation::f_GetChecksums
		(
			TCVector<CChecksum> &_Destination
			, TCVector<COutstandingRange> const &_Outstanding
			, uint32 _ChunkSize
			, NFunction::TCFunction<void ()> const &_fCheckAbort
		)
	{
		//DMibTrace("RSync: Checksums" DMibNewLine, 0);
		CHash_MD5 HashMD5;
		CHash_SHA256_16 HashSHA1;
		for (COutstandingRange const &Range : _Outstanding)
		{
			//DMibTrace("   {} -> {}" DMibNewLine, (Range.m_Start/_ChunkSize), ((Range.m_Start + Range.m_Length)/_ChunkSize));
			mp_pFileToSend->f_SetPosition(Range.m_Start);
			umint nCheckSums = (Range.m_Length + (_ChunkSize - 1)) / _ChunkSize;
			CIOByteVector TempBuffer;
			TempBuffer.f_SetLen(_ChunkSize);
			umint nBytes = Range.m_Length;
			for (umint iChecksum = 0; iChecksum < nCheckSums; ++iChecksum)
			{
				CChecksum &Checksum = _Destination.f_Insert();
				umint nBytesToRead = fg_Min(nBytes, _ChunkSize);
				nBytes -= nBytesToRead;
				mp_pFileToSend->f_ConsumeBytes(TempBuffer.f_GetArray(), nBytesToRead);
				if (mp_Flags & ERSyncFlag_UseSHA256)
					Checksum.m_Digest = fg_GetSHA256Checksum(HashSHA1, TempBuffer.f_GetArray(), nBytesToRead);
				else
					Checksum.m_Digest = fg_GetMD5Checksum(HashMD5, TempBuffer.f_GetArray(), nBytesToRead);
				Checksum.m_Running = fg_GetRunningSum(TempBuffer.f_GetArray(), nBytesToRead);

				if (_fCheckAbort)
					_fCheckAbort();
			}
		}
	}

	bool CRSyncServer::CImplementation::f_ProcessPacket(CIOByteVector const &_ClientData, CIOByteVector &_ToSendToClient, NFunction::TCFunction<void ()> const &_fCheckAbort)
	{
		switch (mp_ServerMode)
		{
		case EServerMode_Init:
			{
				NStorage::TCUniquePointer<CPacket> pPacket = CPacket::fs_DecodePacket(_ClientData);
#ifdef DPacketOrderDebug
				fp_CheckPacket(*pPacket);
#endif
				if (pPacket->m_PacketType == EPacketType_ClientInit)
				{
					CPacket_ClientInit *pTypedPacket = (CPacket_ClientInit *)pPacket.f_Get();
					CPacket_ServerInit ServerInit;

					TCVector<COutstandingRange> Outstanding;
					COutstandingRange &Range = Outstanding.f_Insert();
					Range.m_Start = 0;
					Range.m_Length = mp_pFileToSend->f_GetLength();
					ServerInit.m_FileSize = Range.m_Length;
					if (pTypedPacket->m_InitialChecksumSize)
						f_GetChecksums(ServerInit.m_InitialChecksums.m_Checksums, Outstanding, pTypedPacket->m_InitialChecksumSize, _fCheckAbort);
					_ToSendToClient = ServerInit.f_GetVector
						(
#ifdef DPacketOrderDebug
							++mp_LastSentPacketID
#endif
						)
					;
					mp_ServerMode = EServerMode_SyncChecksum;
					return false;
				}
				// Invalid packet, lets give up
				DMibError("Invalid RSync packet");
				_ToSendToClient.f_Clear();
				return true;
			}
			break;
		case EServerMode_SyncChecksum:
			{
				if (_ClientData.f_IsEmpty())
					return mp_bDone;
				NStorage::TCUniquePointer<CPacket> pPacket = CPacket::fs_DecodePacket(_ClientData);
#ifdef DPacketOrderDebug
				fp_CheckPacket(*pPacket);
#endif
				if (pPacket->m_PacketType == EPacketType_ClientChecksums)
				{
					CPacket_ClientChecksums *pTypedPacket = (CPacket_ClientChecksums *)pPacket.f_Get();
					CPacket_ServerChecksums Checksums;
					f_GetChecksums(Checksums.m_Checksums, pTypedPacket->m_Outstanding, pTypedPacket->m_ChunkSize, _fCheckAbort);
					_ToSendToClient = Checksums.f_GetVector
						(
#ifdef DPacketOrderDebug
							++mp_LastSentPacketID
#endif
						)
					;
					return false;
				}
				else if (pPacket->m_PacketType == EPacketType_ClientOutstandingRanges)
				{
					CPacket_ClientOutstandingRanges *pTypedPacket = (CPacket_ClientOutstandingRanges *)pPacket.f_Get();
					CPacket_ServerOutstandingRanges Outstanding;

					for (COutstandingRange const &Range : pTypedPacket->m_Outstanding)
					{
						if (Outstanding.m_Data.f_GetLen() + Range.m_Length > mp_MaxPacketSize)
							DMibError("Client asking for outstanding range would exceed max packet size");

						mp_pFileToSend->f_SetPosition(Range.m_Start);
						uint8 *pData = Outstanding.m_Data.f_AddArrayAtEnd(Range.m_Length);
						mp_pFileToSend->f_ConsumeBytes(pData, Range.m_Length);

						if (_fCheckAbort)
							_fCheckAbort();
					}
					_ToSendToClient = Outstanding.f_GetVector
						(
#ifdef DPacketOrderDebug
							++mp_LastSentPacketID
#endif
						)
					;
					if (pTypedPacket->m_bDone)
						mp_bDone = true;
					return pTypedPacket->m_bDone;
				}
				// Invalid packet, lets give up
				DMibError("Invalid RSync packet");
				_ToSendToClient.f_Clear();
				return true;
			}
			break;
		}
		DMibError("Invalid RSync packet");
		_ToSendToClient.f_Clear();
		return true;
	}

#ifdef DPacketOrderDebug
	void CRSyncServer::CImplementation::fp_CheckPacket(CPacket const &_Packet)
	{
		DMibCheck(_Packet.m_PacketID > mp_LastReceivedPacketID);
		mp_LastReceivedPacketID = _Packet.m_PacketID;
	}
#endif

	CRSyncServer::CRSyncServer(NStream::CBinaryStream &_FileToSend, uint32 _MaxPacketSize, ERSyncFlag _Flags)
	{
		mp_pImpl = fg_Construct<CImplementation>(_FileToSend, _MaxPacketSize, _Flags);
	}

	void CRSyncServer::f_SetSyncStream(NStream::CBinaryStream *_pFileToSend)
	{
		mp_pImpl->f_SetFileStream(_pFileToSend);
	}

	CRSyncServer::~CRSyncServer()
	{
	}

	auto // Return true when the process is finished and it's safe to delete this object.
	CRSyncServer::f_ProcessPacket
		(
			CIOByteVector const &_ClientData // Data returned from rsync client.
			, CIOByteVector &_ToSendToClient // New packet to send to client. If the function returns true this array should still be sent to the client.
			, NFunction::TCFunction<void ()> const &_fCheckAbort
		)
		-> bool
	{
		return mp_pImpl->f_ProcessPacket(_ClientData, _ToSendToClient, _fCheckAbort);
	}

	///////////////////////////////////
	// Client
	///////////////////////////////////

	struct CChunk
	{
		uint64 f_Length() const;

		uint64 f_Start() const;

		uint64 f_End() const;

		uint64 f_OldFileEnd() const;

		int64 f_OldFileStart() const;

		uint64 m_End;
		int64 m_OldFilePos;
	};

	uint64 CChunk::f_Length() const
	{
		return m_End - TCMap<uint64, CChunk>::fs_GetKey(this);
	}

	uint64 CChunk::f_Start() const
	{
		return TCMap<uint64, CChunk>::fs_GetKey(this);
	}

	uint64 CChunk::f_End() const
	{
		return m_End;
	}

	uint64 CChunk::f_OldFileEnd() const
	{
		if (m_OldFilePos < 0)
			return 0;
		return m_OldFilePos + f_Length();
	}

	int64 CChunk::f_OldFileStart() const
	{
		return m_OldFilePos;
	}

	struct CFileMap
	{
		CFileMap();

		void f_WriteOut(NStream::CBinaryStream &_OutStream, NStream::CBinaryStream &_InStream, NFunction::TCFunction<void ()> const &_fCheckAbort);

		void f_WriteOutFromTemporary(NStream::CBinaryStream &_OutStream, NStream::CBinaryStream &_TemporaryStream, NFunction::TCFunction<void ()> const &_fCheckAbort);

		void f_WriteOutToTemporary(NStream::CBinaryStream &_InStream, NStream::CBinaryStream &_TemporaryStream, NFunction::TCFunction<void ()> const &_fCheckAbort);

		void f_GetOutstanding(TCVector<COutstandingRange> &_Outstanding, uint64 &_TotalBytes, uint64 &_ChecksumSizes, umint _ChunkSize);

#if DFileMapDebug > 0
		void f_Trace(NStr::CStr const &_Name);
#endif
		void f_AddMainChunk(uint64 _Size);

		bool f_AddFoundChunk(uint64 _ServerPos, uint64 _OldPos, uint64 _Length);

		TCMap<uint64, CChunk> m_Chunks;
		uint64 m_nNotFound;

	private:
		void fp_FeedFromStreamWithAbort(NStream::CBinaryStream &_OutStream, NStream::CBinaryStream &_InStream, uint64 _Length, NFunction::TCFunction<void ()> const &_fCheckAbort);
	};

	CFileMap::CFileMap()
		: m_nNotFound(0)
	{
	}

	void CFileMap::f_WriteOut(NStream::CBinaryStream &_OutStream, NStream::CBinaryStream &_InStream, NFunction::TCFunction<void ()> const &_fCheckAbort)
	{
		while (!m_Chunks.f_IsEmpty())
		{
			auto const &Chunk = *m_Chunks.f_FindSmallest();

			if (Chunk.f_OldFileStart() < 0)
				return;

			if (Chunk.f_Start() != _OutStream.f_GetPosition())
				DMibError("Invalid rsync stream");

			_InStream.f_SetPosition(Chunk.f_OldFileStart());
			fp_FeedFromStreamWithAbort(_OutStream, _InStream, Chunk.f_Length(), _fCheckAbort);

			m_Chunks.f_Remove(m_Chunks.fs_GetKey(Chunk));
		}
	}

	void CFileMap::f_WriteOutFromTemporary(NStream::CBinaryStream &_OutStream, NStream::CBinaryStream &_TemporaryStream, NFunction::TCFunction<void ()> const &_fCheckAbort)
	{
		while (!m_Chunks.f_IsEmpty())
		{
			auto &Chunk = *m_Chunks.f_FindSmallest();

			if (Chunk.f_OldFileStart() < 0)
				return;

			if (Chunk.f_Start() != _OutStream.f_GetPosition())
				DMibError("Invalid rsync stream");

			fp_FeedFromStreamWithAbort(_OutStream, _TemporaryStream, Chunk.f_Length(), _fCheckAbort);

			m_Chunks.f_Remove(m_Chunks.fs_GetKey(Chunk));
		}
	}

	void CFileMap::f_WriteOutToTemporary(NStream::CBinaryStream &_InStream, NStream::CBinaryStream &_TemporaryStream, NFunction::TCFunction<void ()> const &_fCheckAbort)
	{
		_TemporaryStream.f_SetPosition(0);
		for (auto &Chunk : m_Chunks)
		{
			if (Chunk.f_OldFileStart() >= 0)
			{
				_InStream.f_SetPosition(Chunk.f_OldFileStart());

				fp_FeedFromStreamWithAbort(_TemporaryStream, _InStream, Chunk.f_Length(), _fCheckAbort);
			}
		}
		_TemporaryStream.f_SetPosition(0);
		_InStream.f_SetPosition(0);
	}

	void CFileMap::f_GetOutstanding(TCVector<COutstandingRange> &_Outstanding, uint64 &_TotalBytes, uint64 &_ChecksumSizes, umint _ChunkSize)
	{
		umint ChecksumSize = gc_ChecksumNetworkSize;
		for (auto &Chunk : m_Chunks)
		{
			if (Chunk.f_OldFileStart() < 0)
			{
				COutstandingRange &Outstanding = _Outstanding.f_Insert();
				Outstanding.m_Start = Chunk.f_Start();
				Outstanding.m_Length = Chunk.f_Length();
				_TotalBytes += Outstanding.m_Length;
				_ChecksumSizes += Outstanding.m_Length * ChecksumSize;
			}
		}

		if (_ChunkSize)
			_ChecksumSizes /= _ChunkSize;
	}

#if DFileMapDebug > 0
	void CFileMap::f_Trace(NStr::CStr const &_Name)
	{
#	if DFileMapDebug > 1
		DMibDTrace(DMibNewLine "{}: {} Chunks {} not found" DMibNewLine, _Name, m_Chunks.f_GetLen(), m_nNotFound);
#	endif
		CChunk const *pLastChunk = nullptr;
		for (auto &Chunk : m_Chunks)
		{
#	if DFileMapDebug > 1
			DMibDTrace("{} -> {} = {} -> {}" DMibNewLine, Chunk.f_Start(), Chunk.f_End(), Chunk.f_OldFileStart(), Chunk.f_OldFileEnd());
#	endif
			if (pLastChunk)
				DMibFastCheck(Chunk.f_Start() == pLastChunk->f_End());
			else
				DMibFastCheck(Chunk.f_Start() == 0);
			pLastChunk = &Chunk;
		}
	}
#endif

	void CFileMap::f_AddMainChunk(uint64 _Size)
	{
		CChunk &Chunk = m_Chunks[0u];
		Chunk.m_End = _Size;
		Chunk.m_OldFilePos = -1;
		++m_nNotFound;
#if DFileMapDebug > 0
		f_Trace("After AddMainChunk");
#endif
	}

	bool CFileMap::f_AddFoundChunk(uint64 _ServerPos, uint64 _OldPos, uint64 _Length)
	{
		auto iChunk = m_Chunks.f_GetIterator_LargestLessThanEqual(_ServerPos);
		DMibCheck(iChunk)("The chunks must already have been created");
#if DFileMapDebug > 0
		// f_Trace("Before AddFoundChunk");
#endif
		if (iChunk)
		{
			CChunk *pChunk = iChunk;
			uint64 ServerPos = pChunk->f_Start();
			if (pChunk->f_OldFileStart() >= 0)
			{
				if (ServerPos == _ServerPos && pChunk->f_Length() == _Length)
				{
					// Check if we can merge chunks
					CChunk *pBefore = nullptr;
					if (_ServerPos)
						pBefore = m_Chunks.f_FindLargestLessThanEqual(_ServerPos - 1);
					++iChunk;
					CChunk *pAfter = iChunk;
					CChunk *pFirst = pChunk;
					if (pBefore && pBefore->f_OldFileStart() >= 0 && pBefore->f_OldFileEnd() == _OldPos)
					{
						pBefore->m_End = pChunk->m_End;
						m_Chunks.f_Remove(pChunk);
						pFirst = pBefore;
					}
					else if (pAfter && pAfter->f_OldFileStart() >= 0 && uint64(pAfter->f_OldFileStart()) == _OldPos + _Length)
						pChunk->m_OldFilePos = _OldPos;
					if (pAfter && pAfter->f_OldFileStart() >= 0 && uint64(pAfter->f_OldFileStart()) == pFirst->f_OldFileEnd())
					{
						pFirst->m_End = pAfter->m_End;
						m_Chunks.f_Remove(pAfter);
					}
				}
				else
					DMibCheck(_ServerPos >= pChunk->f_Start() && (_ServerPos + _Length) <= pChunk->m_End)("Protocol requires no overlapping chunks");
			}
			else
			{
				if (pChunk->f_Start() == _ServerPos)
				{
					CChunk *pBefore = nullptr;
					if (_ServerPos)
						pBefore = m_Chunks.f_FindLargestLessThanEqual(_ServerPos - 1);

					bool bHandle = true;

					if (pBefore)
					{
						DMibCheck(pBefore->f_OldFileStart() >= 0)("Protocol requires no overlapping chunks");

						if (pBefore->f_OldFileEnd() == _OldPos)
						{
							bHandle = false;
							// Just extend at end
							pBefore->m_End += _Length;
							if (pChunk->f_Length() == _Length)
							{
								// Next chunk not needed
								++iChunk;
								CChunk *pMergeChunk = iChunk;
								m_Chunks.f_Remove(pChunk);
								--m_nNotFound;
								if (pMergeChunk && pBefore->f_OldFileEnd() == uint64(pMergeChunk->f_OldFileStart()))
								{
									DMibCheck(pMergeChunk->f_OldFileStart() >= 0)("Next chunk must be found");
									pBefore->m_End = pMergeChunk->m_End;
									m_Chunks.f_Remove(pMergeChunk);
								}
							}
							else
							{
								DMibCheck(_Length < pChunk->f_Length())("Protocol requires no overlapping chunks");

								// Next chunk needs to shrink
								CChunk Copy = *pChunk;
								uint64 Pos = pChunk->f_Start() + _Length;
								m_Chunks.f_Remove(pChunk);
								m_Chunks[Pos] = Copy;
							}
						}
					}

					if (bHandle)
					{
						if (pChunk->f_Length() == _Length)
						{
							// Next chunk not needed
							++iChunk;
							pChunk->m_OldFilePos = _OldPos;
							--m_nNotFound;
							CChunk *pMergeChunk = iChunk;
							if (pMergeChunk && pChunk->f_OldFileEnd() == uint64(pMergeChunk->f_OldFileStart()))
							{
								DMibCheck(pMergeChunk->f_OldFileStart() >= 0)("Next chunk must be found");
								pChunk->m_End = pMergeChunk->m_End;
								m_Chunks.f_Remove(pMergeChunk);
							}
						}
						else
						{
							DMibCheck(_Length < pChunk->f_Length())("Protocol requires no overlapping chunks");

							// Next chunk needs to shrink
							CChunk Copy = *pChunk;
							uint64 Pos = pChunk->f_Start() + _Length;
							m_Chunks[Pos] = Copy;
							pChunk->m_OldFilePos = _OldPos;
							pChunk->m_End = _ServerPos + _Length;
						}
					}
				}
				else
				{
					// Split in the middle
					++iChunk;
					CChunk *pNext = iChunk;
					if (pNext && pNext->f_Start() == _ServerPos + _Length && uint64(pNext->f_OldFileStart()) == _OldPos + _Length)
					{
						// Merge with next
						CChunk &NewChunk = m_Chunks[_ServerPos];
						NewChunk.m_End = pNext->m_End;
						NewChunk.m_OldFilePos = _OldPos;
						m_Chunks.f_Remove(pNext);
						pChunk->m_End = _ServerPos;
					}
					else
					{
						CChunk Copy = *pChunk;
						pChunk->m_End = _ServerPos;
						CChunk &New = m_Chunks[_ServerPos];
						New.m_End = _ServerPos + _Length;
						New.m_OldFilePos = _OldPos;
						if (_ServerPos + _Length != Copy.m_End)
						{
							CChunk &New2 = m_Chunks[_ServerPos + _Length];
							New2 = Copy;
							++m_nNotFound;
						}
					}
				}
			}
		}
#if DFileMapDebug > 0
		// f_Trace("After AddFoundChunk");
#endif

		return m_nNotFound == 0;
	}

	void CFileMap::fp_FeedFromStreamWithAbort(NStream::CBinaryStream &_OutStream, NStream::CBinaryStream &_InStream, uint64 _Length, NFunction::TCFunction<void ()> const &_fCheckAbort)
	{
		auto Length = _Length;
		while (Length)
		{
			auto ThisTime = fg_Min(Length, gc_IdealIoSize);
			_OutStream.f_FeedFromStream(_InStream, ThisTime);
			Length -= ThisTime;
			if (_fCheckAbort)
				_fCheckAbort();
		}
	}

	struct CRSyncClient::CImplementation
	{
		CImplementation
			(
				NStream::CBinaryStream &_OldFile
				, NStream::CBinaryStream &_NewFile
				, uint32 _MinChunkSize
				, uint32 _MaxChunkSize
				, uint32 _MaxPacketSize
				, uint32 _MaxQueue
				, NStream::CBinaryStream *_pTempStream
				, ERSyncFlag _Flags
			)
		;

		uint64 f_GetRawBytes();

		void f_GetProgress(uint32 &_Stage, uint32 &_Stages, uint64 &_BytesTransfered, uint64 &_TotalBytes);

		bool f_FindMatched(CPacket_ServerChecksums const &_CheckSums, uint32 _ChunkSize, NFunction::TCFunction<void ()> const &_fCheckAbort);

		bool f_HandleOutstanding(CIOByteVector &_ToSendToServer, CIOByteVector const &_Data, bool &_bWantOneMoreProcess, NFunction::TCFunction<void ()> const &_fCheckAbort);

		void f_HandleChecksums(CIOByteVector &_ToSendToServer, CPacket_ServerChecksums const &_Checksums, bool &_bWantOneMoreProcess, NFunction::TCFunction<void ()> const &_fCheckAbort);

		bool f_ProcessPacket(CIOByteVector const &_ServerData, CIOByteVector &_ToSendToServer, bool &_bWantOneMoreProcess, NFunction::TCFunction<void ()> const &_fCheckAbort);

	private:
		enum EClientMode
		{
			EClientMode_Init
			, EClientMode_SyncChecksum
			, EClientMode_SyncOutstanding
		};

		struct CInnerHash
		{
			struct CCompare
			{
				inline_small CHashDigest_SHA256_16 const &operator () (CInnerHash const &_Node) const;
			};

			CInnerHash(CHashDigest_SHA256_16 const &_Hash);

			CHashDigest_SHA256_16 m_Hash;
			//TCVector<uint64, NMemory::TCAllocator_Static<sizeof(uint64)*2, sizeof(uint64)>, TCVectorOptions<1>> m_Positions;
			TCVector<uint64, CAllocator_Heap, TCVectorOptions<1, true, false>> m_Positions;
			//TCVector<uint64> m_Positions;

			NIntrusive::TCAVLLink<> m_Link;
		};

		struct COuterHash
		{
			struct CCompare
			{
				inline_small uint32 const &operator () (COuterHash const &_Node) const;
			};

			COuterHash(uint32 _RunningHash);

			uint32 m_RunningHash;
			uint32 m_nFalseAlarms;

			NIntrusive::TCAVLLink<> m_Link;

			NIntrusive::TCAVLTree<&CInnerHash::m_Link, CInnerHash::CCompare> m_InnerHash;
		};

#ifdef DPacketOrderDebug
		void fp_CheckPacket(CPacket const &_Packet);
#endif

		static constexpr umint mcp_HashSize = 65536;
		static constexpr uint32 mcp_HashMask = uint32(mcp_HashSize) - 1;
		static constexpr uint32 mcp_OutstandingPerPacket = gc_IdealIoSize;

		NStream::CBinaryStream &mp_OldFile;
		NStream::CBinaryStream &mp_NewFile;
		NStream::CBinaryStream *mp_pTempStream = nullptr;
		uint32 mp_MinChunkSize = 0;
		uint32 mp_MaxChunkSize = 0;
		uint32 mp_CurrentChunkSize = 0;
		uint32 mp_MaxPacketSize = 0;
		uint32 mp_MaxInFlightPackets = 0;

		uint64 mp_FileSize = 0;
		uint64 mp_RawBytes = 0;
		uint64 mp_RawBytesTotal = 0;

		ERSyncFlag mp_Flags = ERSyncFlag_None;

		bool mp_bSentDoneMessage = false;
		bool mp_bReceivedDoneMessage = false;

#ifdef DPacketOrderDebug
		uint32 mp_LastSentPacketID = 0;
		uint32 mp_LastReceivedPacketID = 0;
#endif

		CFileMap mp_ServerFile;

		EClientMode mp_ClientMode = EClientMode_Init;
		TCLinkedList<TCVector<COutstandingRange>> mp_LastAskedFor;
		TCVector<COutstandingRange> mp_Outstanding;
		TCRegions<CMibFilePos, zbool> mp_UnfoundRegions;

		TCPool<CInnerHash, 128, NThread::CNoLock, NMemory::CPoolType_Growing, NMemory::CAllocator_Heap> mp_InnerPool;
		TCPool<COuterHash, 128, NThread::CNoLock, NMemory::CPoolType_Growing, NMemory::CAllocator_Heap> mp_OuterPool;

		NIntrusive::TCAVLTree<&COuterHash::m_Link, COuterHash::CCompare> mp_Hash[mcp_HashSize];
	};

	CRSyncClient::CImplementation::CImplementation
		(
			NStream::CBinaryStream &_OldFile
			, NStream::CBinaryStream &_NewFile
			, uint32 _MinChunkSize
			, uint32 _MaxChunkSize
			, uint32 _MaxPacketSize
			, uint32 _MaxQueue
			, NStream::CBinaryStream *_pTempStream
			, ERSyncFlag _Flags
		)
		: mp_OldFile(_OldFile)
		, mp_NewFile(_NewFile)
		, mp_pTempStream(_pTempStream)
		, mp_MinChunkSize(_MinChunkSize)
		, mp_MaxChunkSize(_MaxChunkSize)
		, mp_CurrentChunkSize(_MaxChunkSize)
		, mp_MaxInFlightPackets(fg_Max(_MaxQueue / mcp_OutstandingPerPacket, 1u))
		, mp_MaxPacketSize(_MaxPacketSize)
		, mp_Flags(_Flags)
	{
		DMibCheck(&_OldFile != &_NewFile || _pTempStream);
	}

	uint64 CRSyncClient::CImplementation::f_GetRawBytes()
	{
		return mp_RawBytes;
	}

	void CRSyncClient::CImplementation::f_GetProgress(uint32 &_Stage, uint32 &_Stages, uint64 &_BytesTransfered, uint64 &_TotalBytes)
	{
		if (mp_MaxChunkSize < mp_MinChunkSize)
		{
			_Stages = 1;
			_Stage = 1;
		}
		else
		{
			_Stages = 0;
			_Stage = 0;
			uint32 Temp = mp_MaxChunkSize;
			while (Temp >= mp_MinChunkSize)
			{
				++_Stages;
				if (Temp > mp_CurrentChunkSize)
					++_Stage;
				Temp >>= 2;
			}
		}
		_TotalBytes = mp_RawBytesTotal;
		_BytesTransfered = mp_RawBytes;
	}

	bool CRSyncClient::CImplementation::f_FindMatched(CPacket_ServerChecksums const &_CheckSums, uint32 _ChunkSize, NFunction::TCFunction<void ()> const &_fCheckAbort)
	{
		auto ClearHash = fg_OnScopeExit
			(
				[&]()
				{
					for (umint iHash = 0; iHash < mcp_HashSize; ++iHash)
					{
						auto &Tree = mp_Hash[iHash];
						while (auto *pRoot = Tree.f_GetRoot())
						{
							while (auto *pInner = pRoot->m_InnerHash.f_GetRoot())
							{
								pRoot->m_InnerHash.f_Remove(pInner);
								mp_InnerPool.f_Delete(pInner);
							}
							Tree.f_Remove(pRoot);
							mp_OuterPool.f_Delete(pRoot);
						}
					}
				}
			)
		;

		umint ChunkSize = _ChunkSize;
		umint iCurrentChecksum = 0;
		TCVector<COutstandingRange> LastAskedFor = mp_LastAskedFor.f_Pop();

		for (COutstandingRange const &Sequence : LastAskedFor)
		{
			uint64 CurrentPos = Sequence.m_Start;
			uint64 End = CurrentPos + Sequence.m_Length;
			while (CurrentPos < End)
			{
				CChecksum const &Checksum = _CheckSums.m_Checksums[iCurrentChecksum];
				uint32 HashDigest = Checksum.m_Running;
				HashDigest = ((HashDigest & uint32(0xFFFF)) ^ (HashDigest >> 16)) & mcp_HashMask;
				auto &Tree = mp_Hash[HashDigest];
				auto *pOuter = Tree.f_FindEqual(Checksum.m_Running);
				if (!pOuter)
				{
					pOuter = mp_OuterPool.f_New(Checksum.m_Running);
					Tree.f_Insert(pOuter);
				}
				auto *pInner = pOuter->m_InnerHash.f_FindEqual(Checksum.m_Digest);
				if (!pInner)
				{
					pInner = mp_InnerPool.f_New(Checksum.m_Digest);
					pOuter->m_InnerHash.f_Insert(pInner);
				}
				pInner->m_Positions.f_Insert(CurrentPos);
				CurrentPos += ChunkSize;
				++iCurrentChecksum;
			}
		}

		if (mp_OldFile.f_IsValid())
			mp_OldFile.f_SetPosition(0);

		CIOByteVector History;
		History.f_SetLen(ChunkSize);
		uint8 *pHistory = History.f_GetArray();
		bool bFound = false;
		CByteVector ByteBuffer;
		auto BufferSize = fg_Max(ChunkSize, gc_IdealIoSize);
		ByteBuffer.f_SetLen(BufferSize);
		auto pByteBuffer = ByteBuffer.f_GetArray();
		auto TheseRegions = mp_UnfoundRegions;

		CHash_SHA256_16 HashSHA1;
		CHash_MD5 HashMD5;

		for (auto iRegion = TheseRegions.f_GetIterator(); iRegion && !bFound; ++iRegion)
		{
			if (iRegion->f_Data())
				continue; // Region already found
			umint nSkipBytes = 0;
			umint iCurrentHistory = 0;
			CRollsum Rollsum;
			CMibFilePos End = iRegion->f_End();
			CMibFilePos Start = iRegion->f_Start();
			mp_OldFile.f_SetPosition(Start);

			auto fCheckBytes = [&](umint _LocalPos)
				{
					uint32 Digest = Rollsum.f_Digest();
					uint32 HashDigest = Digest;
					HashDigest = ((HashDigest & uint32(0xFFFF)) ^ (HashDigest >> 16)) & mcp_HashMask;

					auto &Map = mp_Hash[HashDigest];
					auto *pFind = Map.f_FindEqual(Digest);
					if (pFind)
					{
						umint First = fg_Min(ChunkSize - iCurrentHistory, Rollsum.m_Count);
						umint Second = fg_Min(iCurrentHistory, Rollsum.m_Count - First);
						CHashDigest_SHA256_16 Digest;
						if (mp_Flags & ERSyncFlag_UseSHA256)
							Digest = fg_GetSHA256Checksum(HashSHA1, pHistory + iCurrentHistory, First, pHistory, Second);
						else
							Digest = fg_GetMD5Checksum(HashMD5, pHistory + iCurrentHistory, First, pHistory, Second);
						auto *pPosition = pFind->m_InnerHash.f_FindEqual(Digest);
						if (pPosition)
						{
							umint nPosition = pPosition->m_Positions.f_GetLen();
							uint64 const *pPositionArray = pPosition->m_Positions.f_GetArray();
							for (umint iPosition = 0; iPosition < nPosition; ++iPosition)
							{
								uint64 const &Position = pPositionArray[iPosition];
								umint Size = Rollsum.m_Count;
								if (Position + Rollsum.m_Count > mp_FileSize)
									Size = mp_FileSize - Position;
								if (mp_ServerFile.f_AddFoundChunk(Position, _LocalPos, Size))
								{
									bFound = true;
									return;
								}
							}
							mp_UnfoundRegions.f_MakeRegion
								(
									_LocalPos
									, _LocalPos + Rollsum.m_Count
									, [&](zbool &_bFound)
									{
										_bFound = true;
									}
								)
							;
							pPosition->m_Positions.f_Clear();
							nSkipBytes = Rollsum.m_Count - 1;
							if (bFound)
								return;
							if (ChunkSize <= 4 * 1024)
							{
								pFind->m_InnerHash.f_Remove(pPosition);
								mp_InnerPool.f_Delete(pPosition);
								if (pFind->m_InnerHash.f_IsEmpty())
								{
									Map.f_Remove(pFind);
									mp_OuterPool.f_Delete(pFind);
								}
							}
						}
						else if (ChunkSize >= 4 * 1024)
						{
							if (++pFind->m_nFalseAlarms > 16)
							{
								// This is to counteract worst case scenarios where performance would become untenable
								while (auto *pInner = pFind->m_InnerHash.f_GetRoot())
								{
									pFind->m_InnerHash.f_Remove(pInner);
									mp_InnerPool.f_Delete(pInner);
								}
								Map.f_Remove(pFind);
								mp_OuterPool.f_Delete(pFind);
							}
						}
					}
				}
			;

			for (CMibFilePos iFilePos = Start; iFilePos < End && !bFound;)
			{
				umint nBuffer = fg_Min(uint64(End - iFilePos), BufferSize);
				mp_OldFile.f_ConsumeBytes(pByteBuffer, nBuffer);
				umint iBuffer = 0;
				if (_fCheckAbort)
					_fCheckAbort();
				auto *pBuffer = pByteBuffer;

				bool bAllSkipped = true;
				while (bAllSkipped)
				{
					bAllSkipped = false;
					if (_fCheckAbort)
						_fCheckAbort();

					if (Rollsum.m_Count < ChunkSize)
					{
						umint nBytes = fg_Min(nBuffer, ChunkSize - Rollsum.m_Count);
						Rollsum.f_RollIn(pBuffer, nBytes);
						{
							umint nHistory0 = fg_Min(nBytes, ChunkSize - iCurrentHistory);
							fg_MemCopy(pHistory + iCurrentHistory, pBuffer, nHistory0);
							umint nHistory1 = fg_Min((ChunkSize - iCurrentHistory) - nHistory0, nBytes - nHistory0);
							fg_MemCopy(pHistory, pBuffer + nHistory0, nHistory1);
							iCurrentHistory += nBytes;
							if (iCurrentHistory >= ChunkSize)
								iCurrentHistory -= ChunkSize;
						}
						iBuffer += nBytes;
						iFilePos += nBytes;

						fCheckBytes(iFilePos - Rollsum.m_Count);
						if (nSkipBytes == ChunkSize - 1 && nBuffer >= ChunkSize)
						{
							Rollsum = {};
							nSkipBytes = 0;
							bAllSkipped = true;
							nBuffer -= nBytes;
							pBuffer += nBytes;
							iBuffer -= nBytes;
						}
					}
				}

				for (; iBuffer < nBuffer && !bFound; ++iBuffer)
				{
					uint8 Byte = pBuffer[iBuffer];
					Rollsum.f_Advance(pHistory[iCurrentHistory], Byte);

					pHistory[iCurrentHistory] = Byte;
					++iCurrentHistory;
					if (iCurrentHistory == ChunkSize)
						iCurrentHistory = 0;
					++iFilePos;

					if (nSkipBytes)
					{
						--nSkipBytes;
						continue;
					}
					fCheckBytes(iFilePos - Rollsum.m_Count);
				}
			}

			while (Rollsum.m_Count > 1 && !bFound)
			{
				Rollsum.f_RollOut(pHistory[iCurrentHistory]);

				++iCurrentHistory;
				if (iCurrentHistory == ChunkSize)
					iCurrentHistory = 0;

				fCheckBytes(End - Rollsum.m_Count);
			}
		}

		return bFound;
	}

	auto CRSyncClient::CImplementation::f_HandleOutstanding
		(
			CIOByteVector &_ToSendToServer
			, CIOByteVector const &_Data
			, bool &_bWantOneMoreProcess
			, NFunction::TCFunction<void ()> const &_fCheckAbort
		)
		-> bool
	{
		mp_ClientMode = EClientMode_SyncOutstanding;

		if (!_Data.f_IsEmpty())
		{
			uint8 const *pData = _Data.f_GetArray();
			umint nData = _Data.f_GetLen();
			TCVector<COutstandingRange> LastAskedFor = mp_LastAskedFor.f_Pop();
			for (COutstandingRange const &Chunk : LastAskedFor)
			{
				if (_fCheckAbort)
					_fCheckAbort();

				if (nData < Chunk.m_Length)
					DMibError("Invalid rsync stream");
				if (mp_NewFile.f_GetPosition() != Chunk.m_Start)
				{
					if (mp_pTempStream)
						mp_ServerFile.f_WriteOutFromTemporary(mp_NewFile, *mp_pTempStream, _fCheckAbort);
					else
						mp_ServerFile.f_WriteOut(mp_NewFile, mp_OldFile, _fCheckAbort);
				}
				if (mp_NewFile.f_GetPosition() != Chunk.m_Start)
					DMibError("Invalid rsync stream");
				mp_NewFile.f_FeedBytes(pData, Chunk.m_Length);
				nData -= Chunk.m_Length;
				pData += Chunk.m_Length;

				auto *pNextChunk = mp_ServerFile.m_Chunks.f_FindSmallest();
				DMibCheck(pNextChunk);

				if (pNextChunk && pNextChunk->f_End() == (Chunk.m_Start + Chunk.m_Length))
					mp_ServerFile.m_Chunks.f_Remove(pNextChunk);
			}

			if (mp_pTempStream)
				mp_ServerFile.f_WriteOutFromTemporary(mp_NewFile, *mp_pTempStream, _fCheckAbort);
			else
				mp_ServerFile.f_WriteOut(mp_NewFile, mp_OldFile, _fCheckAbort);
		}

		bool bIsDone = mp_Outstanding.f_IsEmpty() && mp_LastAskedFor.f_IsEmpty() && mp_bSentDoneMessage && mp_bReceivedDoneMessage;
		if (bIsDone)
		{
			if (mp_Flags & ERSyncFlag_ClientTruncateOutput)
				mp_NewFile.f_SetLength(mp_FileSize);
			return true;
		}

		if (mp_Outstanding.f_IsEmpty() && mp_bSentDoneMessage)
			return false;

		umint MaxPerPacket = mcp_OutstandingPerPacket;
		CPacket_ClientOutstandingRanges ServerPacket;

		umint nOutstanding = mp_Outstanding.f_GetLen();
		umint iRemove = 0;
		for (umint iOutstanding = 0; iOutstanding < nOutstanding && MaxPerPacket; ++iOutstanding)
		{
			COutstandingRange &Outstanding = mp_Outstanding[iOutstanding];
			if (Outstanding.m_Length)
			{
				COutstandingRange &ToSend = ServerPacket.m_Outstanding.f_Insert();
				ToSend.m_Length = fg_Min(MaxPerPacket, Outstanding.m_Length);
				ToSend.m_Start = Outstanding.m_Start;
				Outstanding.m_Length -= ToSend.m_Length;
				Outstanding.m_Start += ToSend.m_Length;
				MaxPerPacket -= ToSend.m_Length;
			}
			if (!Outstanding.m_Length)
				++iRemove;
		}
		mp_RawBytes += mcp_OutstandingPerPacket - MaxPerPacket;
		mp_Outstanding.f_Remove(0, iRemove);

		if (ServerPacket.m_Outstanding.f_IsEmpty())
		{
			ServerPacket.m_bDone = true;
			mp_bSentDoneMessage = true;
		}
		else
			mp_LastAskedFor.f_Insert(ServerPacket.m_Outstanding);

		if (mp_LastAskedFor.f_GetLen() < mp_MaxInFlightPackets)
			_bWantOneMoreProcess = true;

		_ToSendToServer = ServerPacket.f_GetVector
			(
#ifdef DPacketOrderDebug
				++mp_LastSentPacketID
#endif
			)
		;

		return false;
	}

	void CRSyncClient::CImplementation::f_HandleChecksums
		(
			CIOByteVector &_ToSendToServer
			, CPacket_ServerChecksums const &_Checksums
			, bool &_bWantOneMoreProcess
			, NFunction::TCFunction<void ()> const &_fCheckAbort
		)
	{
		bool bAllFound = false;
		if (mp_CurrentChunkSize >= mp_MinChunkSize)
			bAllFound = f_FindMatched(_Checksums, mp_CurrentChunkSize, _fCheckAbort);
		else
			mp_LastAskedFor.f_Pop();
#if DFileMapDebug > 0
		mp_ServerFile.f_Trace("Finished find matches");
#endif
		if (bAllFound)
		{
			// Finished
			f_HandleOutstanding(_ToSendToServer, CIOByteVector(), _bWantOneMoreProcess, _fCheckAbort);
			if (mp_pTempStream)
			{
				mp_ServerFile.f_WriteOutToTemporary(mp_NewFile, *mp_pTempStream, _fCheckAbort);
				mp_ServerFile.f_WriteOutFromTemporary(mp_NewFile, *mp_pTempStream, _fCheckAbort);
			}
			else
				mp_ServerFile.f_WriteOut(mp_NewFile, mp_OldFile, _fCheckAbort);
			mp_ClientMode = EClientMode_SyncOutstanding;
		}
		else
		{
		RedoCheck:
			if (mp_CurrentChunkSize <= mp_MinChunkSize)
			{
				if (mp_pTempStream)
					mp_ServerFile.f_WriteOutToTemporary(mp_NewFile, *mp_pTempStream, _fCheckAbort);

				mp_RawBytesTotal = 0;
				uint64 ChecksumSizes = 0;
				mp_ServerFile.f_GetOutstanding(mp_Outstanding, mp_RawBytesTotal, ChecksumSizes, mp_CurrentChunkSize);
				f_HandleOutstanding(_ToSendToServer, CIOByteVector(), _bWantOneMoreProcess, _fCheckAbort);
			}
			else
			{
				do
				{
					mp_CurrentChunkSize >>= 2;
					if (mp_CurrentChunkSize < mp_MinChunkSize)
						mp_CurrentChunkSize = mp_MinChunkSize;
				}
				while (mp_CurrentChunkSize != mp_MinChunkSize && mp_CurrentChunkSize > mp_FileSize / 2);

				if (mp_CurrentChunkSize < mp_MinChunkSize)
					goto RedoCheck;

				CPacket_ClientChecksums ServerCommand;
				ServerCommand.m_ChunkSize = mp_CurrentChunkSize;
				uint64 Temp = 0;
				uint64 ChecksumSizes = 0;
				mp_ServerFile.f_GetOutstanding(ServerCommand.m_Outstanding, Temp, ChecksumSizes, mp_CurrentChunkSize);

				if (ChecksumSizes > mp_MaxPacketSize)
				{
					mp_CurrentChunkSize = mp_MinChunkSize;
					goto RedoCheck;
				}
				mp_LastAskedFor.f_Insert(ServerCommand.m_Outstanding);
				_ToSendToServer = ServerCommand.f_GetVector
					(
#ifdef DPacketOrderDebug
						++mp_LastSentPacketID
#endif
					)
				;
			}
		}
	}

	auto CRSyncClient::CImplementation::f_ProcessPacket
		(
			CIOByteVector const &_ServerData
			, CIOByteVector &_ToSendToServer
			, bool &_bWantOneMoreProcess
			, NFunction::TCFunction<void ()> const &_fCheckAbort
		)
		-> bool
	{
		_bWantOneMoreProcess = false;
		switch (mp_ClientMode)
		{
		case EClientMode_Init:
			{
				umint OldFileSize;
				if (mp_OldFile.f_IsValid())
					OldFileSize = mp_OldFile.f_GetLength();
				else
					OldFileSize = 0;
				while (mp_MaxChunkSize && mp_MaxChunkSize > OldFileSize / 2)
					mp_MaxChunkSize >>= 1;
				mp_CurrentChunkSize = mp_MaxChunkSize;

				CPacket_ClientInit Init;
				if (mp_MaxChunkSize < mp_MinChunkSize)
					Init.m_InitialChecksumSize = 0;
				else
					Init.m_InitialChecksumSize = mp_MaxChunkSize;
				mp_CurrentChunkSize = mp_MaxChunkSize;
				_ToSendToServer = Init.f_GetVector
					(
#ifdef DPacketOrderDebug
						++mp_LastSentPacketID
#endif
					)
				;
				mp_ClientMode = EClientMode_SyncChecksum;
				return false;
			}
			break;
		case EClientMode_SyncChecksum:
			{
				NStorage::TCUniquePointer<CPacket> pPacket = CPacket::fs_DecodePacket(_ServerData);
#ifdef DPacketOrderDebug
				fp_CheckPacket(*pPacket);
#endif

				if (pPacket->m_PacketType == EPacketType_ServerInit)
				{
					CPacket_ServerInit *pTypedPacket = (CPacket_ServerInit *)pPacket.f_Get();
					mp_FileSize = pTypedPacket->m_FileSize;
					COutstandingRange &Chunk = mp_LastAskedFor.f_Insert().f_Insert();
					Chunk.m_Start = 0;
					Chunk.m_Length = mp_FileSize;
					mp_ServerFile.f_AddMainChunk(mp_FileSize);

					if (mp_OldFile.f_IsValid())
					{
						mp_OldFile.f_SetPosition(0);
						if (mp_OldFile.f_GetLength())
							mp_UnfoundRegions.f_MakeRegion(0, mp_OldFile.f_GetLength());
					}

					f_HandleChecksums(_ToSendToServer, pTypedPacket->m_InitialChecksums, _bWantOneMoreProcess, _fCheckAbort);
					return false;
				}
				else if (pPacket->m_PacketType == EPacketType_ServerChecksums)
				{
					CPacket_ServerChecksums *pTypedPacket = (CPacket_ServerChecksums *)pPacket.f_Get();
					f_HandleChecksums(_ToSendToServer, *pTypedPacket, _bWantOneMoreProcess, _fCheckAbort);
					return false;
				}
				// Invalid packet, lets give up
				DMibError("Invalid RSync packet");
				_ToSendToServer.f_Clear();
				return true;
			}
			break;
		case EClientMode_SyncOutstanding:
			{
				if (!_ServerData.f_IsEmpty())
				{
					NStorage::TCUniquePointer<CPacket> pPacket = CPacket::fs_DecodePacket(_ServerData);
#ifdef DPacketOrderDebug
					fp_CheckPacket(*pPacket);
#endif

					if (pPacket->m_PacketType == EPacketType_ServerOutstandingRanges)
					{
						CPacket_ServerOutstandingRanges *pTypedPacket = (CPacket_ServerOutstandingRanges *)pPacket.f_Get();
						if (pTypedPacket->m_Data.f_IsEmpty())
							mp_bReceivedDoneMessage = true;
						if (f_HandleOutstanding(_ToSendToServer, pTypedPacket->m_Data, _bWantOneMoreProcess, _fCheckAbort))
							return true;
						return false;
					}
					// Invalid packet, lets give up
					DMibError("Invalid RSync packet");
					_ToSendToServer.f_Clear();
				}
				else
				{
					if (f_HandleOutstanding(_ToSendToServer, CIOByteVector(), _bWantOneMoreProcess, _fCheckAbort))
						return true;
					return false;
				}

				return true;
			}
			break;
		}
		DMibError("Invalid RSync packet");
		_ToSendToServer.f_Clear();
		return true;
	}

	CHashDigest_SHA256_16 const &CRSyncClient::CImplementation::CInnerHash::CCompare::operator () (CInnerHash const &_Node) const
	{
		return _Node.m_Hash;
	}

	CRSyncClient::CImplementation::CInnerHash::CInnerHash(CHashDigest_SHA256_16 const &_Hash)
		: m_Hash(_Hash)
	{
	}

	uint32 const &CRSyncClient::CImplementation::COuterHash::CCompare::operator () (COuterHash const &_Node) const
	{
		return _Node.m_RunningHash;
	}

	CRSyncClient::CImplementation::COuterHash::COuterHash(uint32 _RunningHash)
		: m_RunningHash(_RunningHash)
		, m_nFalseAlarms(0)
	{
	}

#ifdef DPacketOrderDebug
	void CRSyncClient::CImplementation::fp_CheckPacket(CPacket const &_Packet)
	{
		DMibCheck(_Packet.m_PacketID > mp_LastReceivedPacketID);
		mp_LastReceivedPacketID = _Packet.m_PacketID;
	}
#endif

	CRSyncClient::CRSyncClient
		(
			NStream::CBinaryStream &_OldFile
			, NStream::CBinaryStream &_NewFile
			, uint32 _MinChunkSize
			, uint32 _MaxChunkSize
			, uint32 _MaxPacketSize
			, uint32 _MaxQueue
			, NStream::CBinaryStream *_pTempStream
			, ERSyncFlag _Flags
		)
	{
		mp_pImpl = fg_Construct<CImplementation>(_OldFile, _NewFile, _MinChunkSize, _MaxChunkSize, _MaxPacketSize, _MaxQueue, _pTempStream, _Flags);
	}

	CRSyncClient::~CRSyncClient()
	{
	}

	auto // Return true when the new file is fully written
	CRSyncClient::f_ProcessPacket
		(
			CIOByteVector const &_ServerData // Data returned from rsync server, send in empty for the first packet (client initiates process)
			, CIOByteVector &_ToSendToServer // New packet to send to server. If the function returns true this array will never be filled and no data should be sent to server.
			, bool &_bWantOneMoreProcess // Set to true when returning if the rsync server needs to send one more packet
			, NFunction::TCFunction<void ()> const &_fCheckAbort
		)
		-> bool
	{
		return mp_pImpl->f_ProcessPacket(_ServerData, _ToSendToServer, _bWantOneMoreProcess, _fCheckAbort);
	}

	uint64 CRSyncClient::f_GetRawBytes()
	{
		return mp_pImpl->f_GetRawBytes();
	}

	void CRSyncClient::f_GetProgress(uint32 &_Stage, uint32 &_Stages, uint64 &_BytesTransfered, uint64 &_TotalBytes)
	{
		return mp_pImpl->f_GetProgress(_Stage, _Stages, _BytesTransfered, _TotalBytes);
	}
}
