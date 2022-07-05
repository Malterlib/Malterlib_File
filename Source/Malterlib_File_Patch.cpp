// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_Patch.h"

namespace NMib::NFile
{
	using namespace NCryptography;
	
	class CMalterlibPatchEncodeHelper
	{
	public:

		static constexpr mint mc_MapLength = 64;

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
				CMalterlibPatchEncodeHelper &m_Context;
				CCompare(CMalterlibPatchEncodeHelper *_pContext)
					: m_Context(*_pContext)
				{
				}

				inline_small bool operator () (const CMappedChunk &_Left, const CMappedChunk &_Right) const
				{
					mint Pos0 = _Left.m_MappedPositions.f_GetFirst() - m_Context.m_MappedPos.f_GetArray();
					mint Pos1 = _Right.m_MappedPositions.f_GetFirst() - m_Context.m_MappedPos.f_GetArray();
					mint Len0 = m_Context.m_nMapped - Pos0;
					mint Len1 = m_Context.m_nMapped - Pos1;
					mint CompareLen = fg_Min(Len0, Len1, mc_MapLength);

					DMibFastCheck((m_Context.m_pOrigData + Pos0) >= m_Context.m_pOrigData);
					DMibFastCheck((m_Context.m_pOrigData + Pos0 + CompareLen) <= m_Context.m_pOrigDataEnd);
					DMibFastCheck((m_Context.m_pOrigData + Pos1) >= m_Context.m_pOrigData);
					DMibFastCheck((m_Context.m_pOrigData + Pos1 + CompareLen) <= m_Context.m_pOrigDataEnd);

					auto Diff = NMemory::fg_MemCmp(m_Context.m_pOrigData + Pos0, m_Context.m_pOrigData + Pos1, CompareLen);
					if (Diff)
						return Diff < 0;

					return Len0 < Len1;
				}

				inline_small bool operator () (const CMappedChunk &_Left, const uint8 *_pSecond) const
				{
					mint Pos0 = _Left.m_MappedPositions.f_GetFirst() - m_Context.m_MappedPos.f_GetArray();
					mint Pos1 = _pSecond - m_Context.m_pFindData;
					mint Len0 = m_Context.m_nMapped - Pos0;
					mint Len1 = m_Context.m_FindLen - Pos1;
					mint CompareLen = fg_Min(Len0, Len1, mc_MapLength);

					DMibFastCheck((m_Context.m_pOrigData + Pos0) >= m_Context.m_pOrigData);
					DMibFastCheck((m_Context.m_pOrigData + Pos0 + CompareLen) <= m_Context.m_pOrigDataEnd);
					DMibFastCheck(_pSecond >= m_Context.m_pFindData);
					DMibFastCheck((_pSecond + CompareLen) <= m_Context.m_pFindDataEnd);

					auto Diff = NMemory::fg_MemCmp(m_Context.m_pOrigData + Pos0, _pSecond, CompareLen);
					if (Diff)
						return Diff < 0;

					return Len0 < Len1;
				}

				inline_small bool operator () (const uint8 *_pFirst, const CMappedChunk &_Right) const
				{
					mint Pos0 = _pFirst - m_Context.m_pFindData;
					mint Pos1 = _Right.m_MappedPositions.f_GetFirst() - m_Context.m_MappedPos.f_GetArray();
					mint Len0 = m_Context.m_FindLen - Pos0;
					mint Len1 = m_Context.m_nMapped - Pos1;
					mint CompareLen = fg_Min(Len0, Len1, mc_MapLength);

					DMibFastCheck(_pFirst >= m_Context.m_pFindData);
					DMibFastCheck((_pFirst + CompareLen) <= m_Context.m_pFindDataEnd);
					DMibFastCheck((m_Context.m_pOrigData + Pos1) >= m_Context.m_pOrigData);
					DMibFastCheck((m_Context.m_pOrigData + Pos1 + CompareLen) <= m_Context.m_pOrigDataEnd);

					auto Diff = NMemory::fg_MemCmp(_pFirst, m_Context.m_pOrigData + Pos1, CompareLen);
					if (Diff)
						return Diff < 0;

					return Len0 < Len1;
				}
			};

			NIntrusive::TCAVLLinkAggregate<> m_TreeLink;
			DMibListLinkSA_List(CMappedPos, m_Link) m_MappedPositions;
		};
		typedef DMibListLinkS_Iter(CMappedPos, m_Link) CMappedPosIter;

		NIntrusive::TCAVLTree<&CMappedChunk::m_TreeLink, CMappedChunk::CCompare> m_MappedTree;

		NContainer::TCVector<CMappedChunk> m_MappedChunk;
		NContainer::TCVector<CMappedPos> m_MappedPos;
		mint m_nMapped;
		const uint8 *m_pOrigData;

		const uint8 *m_pFindData;
		mint m_FindLen;

#if DMibEnableSafeCheck > 0
		const uint8 *m_pOrigDataEnd;
		const uint8 *m_pFindDataEnd;
#endif

		void f_MapOrig(const NContainer::CByteVector &_Orig)
		{
			const uint8 *pOrig = _Orig.f_GetArray();
			if (!pOrig)
				return;
			m_pOrigData = pOrig;
			const uint8 *pOrigSave = pOrig;
			const uint8 *pOrigEnd = pOrig + _Orig.f_GetLen();
#if DMibEnableSafeCheck > 0
			m_pOrigDataEnd = pOrigEnd;
#endif

			aint iMapped = 0;
			mint nMapped = pOrigEnd - pOrig;
			m_nMapped = nMapped;
			m_MappedPos.f_SetLen(nMapped);
			m_MappedChunk.f_SetLen(nMapped);
			CMappedPos *pMappedArray = m_MappedPos.f_GetArray();
			CMappedChunk *pMappedChunkArray = m_MappedChunk.f_GetArray();

			m_pFindData = m_pOrigData;
			m_FindLen = _Orig.f_GetLen();
#if DMibEnableSafeCheck > 0
			m_pFindDataEnd = m_pFindData + m_FindLen;
#endif

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

		void f_CreatePatchData(const NContainer::CByteVector &_Orig, const NContainer::CByteVector &_Changed, NStream::CBinaryStreamMemory<> &_Stream)
		{
			m_pFindData = _Changed.f_GetArray();
			m_FindLen = _Changed.f_GetLen();
#if DMibEnableSafeCheck > 0
			m_pFindDataEnd = m_pFindData + m_FindLen;
#endif

			const uint8 *pChanged = _Changed.f_GetArray();
			const uint8 *pChangedEnd = pChanged + _Changed.f_GetLen();
			const uint8 *pStartUnsaved = pChanged;

			const uint8 *pOrig = _Orig.f_GetArray();
			const uint8 *pOrigEnd = pOrig + _Orig.f_GetLen();
			const CMappedPos *pMappedArray = m_MappedPos.f_GetArray();

			while (pChanged < pChangedEnd)
			{
				CMappedChunk *pChunk = m_MappedTree.f_FindLargestLessThanEqual(pChanged, CMappedChunk::CCompare(this));
				if (pChunk)
				{
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

					if (BestLen > 8) // Only use if resulting patch file is smaller
					{
						if (pStartUnsaved != pChanged)
						{
							// Encode data
							uint32 nBytes = pChanged - pStartUnsaved;
							_Stream << nBytes;
							_Stream.f_FeedBytes(pStartUnsaved, nBytes);
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

	NContainer::CByteVector fg_MalterlibPatchEncode(const NContainer::CByteVector &_Orig, const NContainer::CByteVector &_Changed)
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

		return Stream.f_MoveVector();
	}

	NContainer::CByteVector fg_MalterlibPatchDecode(const NContainer::CByteVector &_Orig, const NContainer::CByteVector &_PatchData)
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

		CHash_MD5 HashOrig;
		HashOrig.f_AddData(_Orig.f_GetArray(), _Orig.f_GetLen());

		if (_Orig.f_GetLen() != OrigSize || CHashDigest_MD5(HashOrig) != OrigHash)
			DMibError("Original file does not match patch");

		NContainer::CByteVector Changed;
		const uint8 *pOrigData = _Orig.f_GetArray();
		Changed.f_SetLen(ChangedSize);
		uint8 *pChangedData = Changed.f_GetArray();

		uint32 Pos = 0;

		while (!Stream.f_IsAtEndOfStream())
		{
			uint32 Oper;
			Stream >> Oper;

			if (Oper)
			{
				if (Oper + Pos > ChangedSize)
					DMibError("Patch could not be applied: Oper + Pos > ChangedSize");

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
					DMibError("Patch could not be applied: Pos + Size > ChangedSize");
				if (OrigPos + Size > OrigSize)
					DMibError("Patch could not be applied: OrigPos + Size > OrigSize");

				NMemory::fg_MemCopy(pChangedData + Pos, pOrigData + OrigPos, Size);
				Pos += Size;
			}
		}

		if (Pos != ChangedSize)
			DMibError("Patch could not be applied: Pos != ChangedSize");

		CHash_MD5 HashChanged;
		HashChanged.f_AddData(Changed.f_GetArray(), Changed.f_GetLen());
		if (CHashDigest_MD5(HashChanged) != ChangedHash)
			DMibError("Patched data did not match patch digest");

		return Changed;
	}
}
