// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_Patch.h"

namespace NMib
{
	namespace NDataProcessing
	{

		class CMalterlibPatchEncodeHelper
		{
		public:

			enum
			{
				EMapLength = 64
			};

			class CMappedPos
			{
			public:
				DMibListLinkSA_Link(CMappedPos, m_Link);
			};

			class CMappedChunk
			{
			public:

				class CCompare
				{
				public:
					CMalterlibPatchEncodeHelper *m_pContext;
					CCompare(CMalterlibPatchEncodeHelper *_pContext)
						: m_pContext(_pContext)
					{
					}
					inline_small bint operator () (const CMappedChunk &_Left, const CMappedChunk &_Right) const
					{
						mint Pos0 = _Left.m_MappedPositions.f_GetFirst() - m_pContext->m_MappedPos.f_GetArray();
						mint Pos1 = _Right.m_MappedPositions.f_GetFirst() - m_pContext->m_MappedPos.f_GetArray();
						return NMem::fg_MemCmp(m_pContext->m_pOrigData + Pos0, m_pContext->m_pOrigData + Pos1 , EMapLength) < 0;
					}
					inline_small bint operator () (const CMappedChunk &_Left, const uint8 *_pSecond) const
					{
						mint Pos0 = _Left.m_MappedPositions.f_GetFirst() - m_pContext->m_MappedPos.f_GetArray();
						return NMem::fg_MemCmp(m_pContext->m_pOrigData + Pos0, _pSecond, EMapLength) < 0;
					}
					inline_small bint operator () (const uint8 *_pFirst, const CMappedChunk &_Right) const
					{
						mint Pos0 = _Right.m_MappedPositions.f_GetFirst() - m_pContext->m_MappedPos.f_GetArray();
						return NMem::fg_MemCmp(_pFirst, m_pContext->m_pOrigData + Pos0, EMapLength) < 0;
					}
				};

				DMibIntrusiveLink(CMappedChunk, NIntrusive::TCAVLLinkAggregate<>, m_TreeLink);
				DMibListLinkSA_List(CMappedPos, m_Link) m_MappedPositions;
			};
			typedef DMibListLinkS_Iter(CMappedPos, m_Link) CMappedPosIter;

			NIntrusive::TCAVLTree<CMappedChunk::CLinkTraits_m_TreeLink, CMappedChunk::CCompare> m_MappedTree;

//			NMem::TCPool<CMappedChunk, 128*1024, NMib::NThread::CNoLock, NMem::CPoolType_Freeable, NMem::CAllocator_Heap> m_ChunkPool;
			NContainer::TCVector<CMappedChunk> m_MappedChunk;
			NContainer::TCVector<CMappedPos> m_MappedPos;
			const uint8 *m_pOrigData;

			void f_MapOrig(const NContainer::TCVector<uint8> &_Orig)
			{
				const uint8 *pOrig = _Orig.f_GetArray();
				if (!pOrig || _Orig.f_GetLen() <= EMapLength)
					return;
				m_pOrigData = pOrig;
				const uint8 *pOrigSave = pOrig;
				const uint8 *pOrigEnd = pOrig + _Orig.f_GetLen() - EMapLength;

				aint iMapped = 0;
				mint nMapped = pOrigEnd - pOrig;
				m_MappedPos.f_SetLen(nMapped);
				m_MappedChunk.f_SetLen(nMapped);
				CMappedPos *pMappedArray = m_MappedPos.f_GetArray();
				CMappedChunk *pMappedChunkArray = m_MappedChunk.f_GetArray();

				while (pOrig < pOrigEnd)
				{
					CMappedChunk *pChunk = m_MappedTree.f_FindEqual(pOrig, CMappedChunk::CCompare(this));
					CMappedPos *pPos = pMappedArray + (pOrig - pOrigSave);
					if (!pChunk)
					{
						pChunk = pMappedChunkArray + iMapped++;
						pChunk->m_MappedPositions.f_Construct();
						pChunk->m_TreeLink.f_Construct();
						pChunk->m_MappedPositions.f_UnsafeInsert(pPos);
						m_MappedTree.f_Insert(pChunk, CMappedChunk::CCompare(this));
					}
					else
						pChunk->m_MappedPositions.f_UnsafeInsert(pPos);

					++pOrig;
				}
			}

			void f_CreatePatchData(const NContainer::TCVector<uint8> &_Orig, const NContainer::TCVector<uint8> &_Changed, NStream::CBinaryStreamMemory<> &_Stream)
			{
				const uint8 *pChanged = _Changed.f_GetArray();
				const uint8 *pChangedEnd = pChanged + _Changed.f_GetLen();
				const uint8 *pStartUnsaved = pChanged;

				const uint8 *pOrig = _Orig.f_GetArray();
				const uint8 *pOrigEnd = pOrig + _Orig.f_GetLen();
				const CMappedPos *pMappedArray = m_MappedPos.f_GetArray();

				while (pChanged < pChangedEnd - EMapLength)
				{
					CMappedChunk *pChunk = m_MappedTree.f_FindEqual(pChanged, CMappedChunk::CCompare(this));
					if (pChunk)
					{
						if (pStartUnsaved != pChanged)
						{
							// Encode data
							uint32 nBytes = pChanged - pStartUnsaved;
							_Stream << nBytes;
							_Stream.f_FeedBytes(pStartUnsaved, nBytes);
						}
						uint32 BestStart = 0;
						uint32 BestLen = 0;

						CMappedPosIter Iter = pChunk->m_MappedPositions;
						while (Iter)
						{
							const uint8 *pFind0 = pChanged;
							mint MappedPos = (Iter.f_GetCurrent() - pMappedArray);
							const uint8 *pFind1 = pOrig + MappedPos;

							while (pFind0 < pChangedEnd && pFind1 < pOrigEnd)
							{
								if (*pFind0 != *pFind1)
									break;

								++pFind0;
								++pFind1;
							}

							uint32 LenFound = pFind0 - pChanged;
							if (LenFound > BestLen)
							{
								BestStart = MappedPos;
								BestLen = LenFound;
							}

							++Iter;
						}

						_Stream << uint32(0);
						_Stream << BestStart;
						_Stream << BestLen;

						pChanged += BestLen;
						pStartUnsaved = pChanged;
					}
					else
						++pChanged;
				}

				if (pStartUnsaved != pChangedEnd)
				{
					// Encode data
					uint32 nBytes = pChangedEnd - pStartUnsaved;
					_Stream << nBytes;
					_Stream.f_FeedBytes(pStartUnsaved, nBytes);
				}
			}

			~CMalterlibPatchEncodeHelper()
			{
				CMappedChunk *pChunk = m_MappedTree.f_GetRoot();
				while (pChunk)
				{
					m_MappedTree.f_Remove(pChunk, CMappedChunk::CCompare(this));

					pChunk->m_MappedPositions.f_Clear();

					pChunk = m_MappedTree.f_GetRoot();
				}
			}

		};

		void fg_MalterlibPatchEncode(const NContainer::TCVector<uint8> &_Orig, const NContainer::TCVector<uint8> &_Changed, NContainer::TCVector<uint8> &_PatchData)
		{
			CMalterlibPatchEncodeHelper Helper;
			Helper.f_MapOrig(_Orig);

			NStream::CBinaryStreamMemory<> Stream;

			CHash_MD5 HashOrig;
			HashOrig.f_AddData(_Orig.f_GetArray(), _Orig.f_GetLen());
			CHash_MD5 HashChanged;
			HashChanged.f_AddData(_Changed.f_GetArray(), _Changed.f_GetLen());

			Stream << CHashDigest_MD5(HashOrig);
			Stream << CHashDigest_MD5(HashChanged);
			Stream << uint32(_Orig.f_GetLen());
			Stream << uint32(_Changed.f_GetLen());

			Helper.f_CreatePatchData(_Orig, _Changed, Stream);

			_PatchData = Stream.f_GetVector();
		}

		bint fg_MalterlibPatchDecode(const NContainer::TCVector<uint8> &_Orig, NContainer::TCVector<uint8> &_Changed, const NContainer::TCVector<uint8> &_PatchData)
		{
			try
			{
				NStream::CBinaryStreamMemoryPtr<> Stream;
				Stream.f_OpenRead(_PatchData.f_GetArray(), _PatchData.f_GetLen());

				CHashDigest_MD5 OrigHash;
				CHashDigest_MD5 ChangedHash;
				uint32 OrigSize;
				uint32 ChangedSize;
				Stream >> OrigHash;
				Stream >> ChangedHash;
				Stream >> OrigSize;
				Stream >> ChangedSize;

				if (_Orig.f_GetLen() != OrigSize)
					return false;

				CHash_MD5 HashOrig;
				HashOrig.f_AddData(_Orig.f_GetArray(), _Orig.f_GetLen());

				if (CHashDigest_MD5(HashOrig) != OrigHash)
					return false;

				const uint8 *pOrigData = _Orig.f_GetArray();
				_Changed.f_SetLen(ChangedSize);
				uint8 *pChangedData = _Changed.f_GetArray();

				uint32 Pos = 0;

				while (!Stream.f_IsAtEndOfStream())
				{
					uint32 Oper;
					Stream >> Oper;

					if (Oper)
					{
						if (Oper + Pos > ChangedSize)
							return false;
						Stream.f_ConsumeBytes(pChangedData + Pos, Oper);
						Pos += Oper;
					}
					else
					{
						uint32 OrigPos;
						uint32 Size;
						Stream >> OrigPos;
						Stream >> Size;

						if (Pos + Size > ChangedSize)
							return false;
						if (OrigPos + Size > OrigSize)
							return false;

						NMem::fg_MemCopy(pChangedData + Pos, pOrigData + OrigPos, Size);
						Pos += Size;
					}
				}

				if (Pos != ChangedSize)
					return false;

				CHash_MD5 HashChanged;
				HashChanged.f_AddData(_Changed.f_GetArray(), _Changed.f_GetLen());
				if (CHashDigest_MD5(HashChanged) != ChangedHash)
					return false;

			}
			catch(NException::CException)
			{
			}

			return true;
		}

	}
}

