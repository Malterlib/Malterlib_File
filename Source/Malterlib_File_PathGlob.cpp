// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "Malterlib_File_PathGlob.h"

namespace NMib::NFile
{
	using namespace NStr;
	using namespace NContainer;
}

namespace
{
	using namespace NMib;

	bool fg_IsNumericGlobRange(ch8 const *_pStart, ch8 const *_pEnd)
	{
		auto pParse = _pStart;
		for (umint i = 0; i < 2; ++i)
		{
			if (pParse != _pEnd && (*pParse == '-' || *pParse == '+'))
				++pParse;

			auto pDigits = pParse;
			while (pParse != _pEnd && *pParse >= '0' && *pParse <= '9')
				++pParse;

			if (pParse == pDigits)
				return false;

			if (i == 0)
			{
				if (_pEnd - pParse < 2 || *pParse != '.' || *(pParse + 1) != '.')
					return false;

				pParse += 2;
			}
		}

		return pParse == _pEnd;
	}
}

namespace NMib::NFile
{
	CUStr CPathGlob::fs_ToUnicode(CStr const &_Text)
	{
		for (auto pParse = _Text.f_GetStr(); pParse != _Text.f_GetStr() + _Text.f_GetLen(); ++pParse)
		{
			if (*pParse == 0)
				DMibError("NUL is not allowed in path patterns or paths");
		}

		return CUStr(_Text);
	}

	CPathGlob::CPathGlob(CStr const &_Pattern)
	{
		mp_bHasSeparator = _Pattern.f_StartsWith("/");
		auto Pattern = fs_ToUnicode(_Pattern.f_RemovePrefix("/"));
		if (Pattern.f_GetLen() > 1024)
			DMibError("Path patterns longer than 1024 characters are not supported");

		mp_Nodes.f_Insert().m_Kind = EKind::mc_Accept;
		mp_iStart = fp_Compile(Pattern.f_GetStr(), Pattern.f_GetStr() + Pattern.f_GetLen(), 0);
		for (umint i = 0; i < mp_Nodes.f_GetLen(); ++i)
		{
			for (umint iVariant = 0; iVariant < 2; ++iVariant)
				fp_CollectClosure(i, iVariant != 0, mp_Nodes[i].m_Closure[iVariant]);
		}
	}

	bool CPathGlob::f_HasSeparator() const
	{
		return mp_bHasSeparator;
	}

	// Runs the pattern over the text and leaves the states it can be in afterwards active.
	void CPathGlob::fp_Run(CUStr const &_Text, CScratch &_Scratch) const
	{
		auto &Active = _Scratch.m_Active;
		auto &Next = _Scratch.m_Next;
		auto &Stamps = _Scratch.m_Stamps;
		if (Stamps.f_GetLen() < mp_Nodes.f_GetLen())
		{
			Stamps.f_SetLen(mp_Nodes.f_GetLen());
			for (auto &Stamp : Stamps)
				Stamp = 0;

			_Scratch.m_Generation = 0;
		}

		auto fAddStates = [&](umint _iState, bool _bComponentStart, TCVector<umint> &o_States)
			{
				for (auto iReached : mp_Nodes[_iState].m_Closure[_bComponentStart ? 1 : 0])
				{
					if (Stamps[iReached] == _Scratch.m_Generation)
						continue;

					Stamps[iReached] = _Scratch.m_Generation;
					o_States.f_Insert(iReached);
				}
			}
		;

		Active.f_Clear();
		++_Scratch.m_Generation;
		fAddStates(mp_iStart, true, Active);

		auto pEnd = _Text.f_GetStr() + _Text.f_GetLen();
		for (auto pParse = _Text.f_GetStr(); pParse != pEnd; ++pParse)
		{
			Next.f_Clear();
			++_Scratch.m_Generation;
			for (auto iState : Active)
			{
				auto const &Node = mp_Nodes[iState];
				bool bMatches = false;
				switch (Node.m_Kind)
				{
					case EKind::mc_Literal:
						bMatches = *pParse == Node.m_Character;

						break;
					case EKind::mc_Any:
					case EKind::mc_Star:
						bMatches = *pParse != '/';

						break;
					case EKind::mc_RecursiveStar:
						bMatches = true;

						break;
					case EKind::mc_Set:
					{
						for (auto const &Range : Node.m_Ranges)
							bMatches |= *pParse >= Range.m_First && *pParse <= Range.m_Last;
						bMatches = *pParse != '/' && bMatches != Node.m_bNegated;

						break;
					}
					default: break;
				}

				if (bMatches)
				{
					bool bRepeat = Node.m_Kind == EKind::mc_Star || Node.m_Kind == EKind::mc_RecursiveStar;
					fAddStates(bRepeat ? iState : Node.m_iNext, *pParse == '/', Next);
				}
			}

			// Swapping keeps both buffers' capacity for the next character.
			fg_Swap(Active, Next);
			if (Active.f_IsEmpty())
				return;
		}
	}

	bool CPathGlob::f_Match(CUStr const &_Path, CUStr const &_FileName, CScratch &_Scratch) const
	{
		fp_Run(mp_bHasSeparator ? _Path : _FileName, _Scratch);
		for (auto iState : _Scratch.m_Active)
		{
			if (mp_Nodes[iState].m_Kind == EKind::mc_Accept)
				return true;
		}

		return false;
	}

	bool CPathGlob::fp_ReachesAccept(umint _iState, bool _bComponentStart) const
	{
		for (auto iReached : mp_Nodes[_iState].m_Closure[_bComponentStart ? 1 : 0])
		{
			if (mp_Nodes[iReached].m_Kind == EKind::mc_Accept)
				return true;
		}

		return false;
	}

	EPathGlobCover CPathGlob::f_MatchBelow(CUStr const &_Directory, CScratch &_Scratch) const
	{
		// A pattern without a separator meets every file's name at any depth. Only a lone
		// star meets them all; anything else may or may not.
		if (!mp_bHasSeparator)
		{
			for (auto iState : mp_Nodes[mp_iStart].m_Closure[1])
			{
				auto const &Node = mp_Nodes[iState];
				bool bStar = Node.m_Kind == EKind::mc_Star || Node.m_Kind == EKind::mc_RecursiveStar;
				if (bStar && fp_ReachesAccept(Node.m_iNext, false))
					return EPathGlobCover::mc_All;
			}

			return EPathGlobCover::mc_Some;
		}

		// After the directory's own path, what is left of the pattern faces the paths below.
		// A recursive star that can end the pattern swallows all of them.
		CUStr Prefix = _Directory;
		if (Prefix)
			Prefix += U"/";

		fp_Run(Prefix, _Scratch);
		if (_Scratch.m_Active.f_IsEmpty())
			return EPathGlobCover::mc_None;

		for (auto iState : _Scratch.m_Active)
		{
			auto const &Node = mp_Nodes[iState];
			if (Node.m_Kind == EKind::mc_RecursiveStar && fp_ReachesAccept(Node.m_iNext, true))
				return EPathGlobCover::mc_All;
		}

		return EPathGlobCover::mc_Some;
	}

	ch32 const *CPathGlob::fsp_SetEnd(ch32 const *_pStart, ch32 const *_pEnd)
	{
		auto pParse = _pStart + 1;
		while (pParse != _pEnd && *pParse != ']')
		{
			if (*pParse == '\\' && pParse + 1 != _pEnd)
				++pParse;

			++pParse;
		}

		return pParse;
	}

	umint CPathGlob::fp_Compile(ch32 const *_pStart, ch32 const *_pEnd, umint _iNext)
	{
		TCVector<umint> Sequence;
		for (auto pParse = _pStart; pParse != _pEnd; ++pParse)
		{
			CNode Node;
			Node.m_Character = *pParse;
			if (*pParse == '\\' && pParse + 1 != _pEnd)
				Node.m_Character = *++pParse;
			else if (*pParse == '*')
			{
				Node.m_Kind = EKind::mc_Star;
				if (pParse + 1 != _pEnd && *(pParse + 1) == '*')
				{
					Node.m_Kind = EKind::mc_RecursiveStar;
					++pParse;
				}
			}
			else if (*pParse == '?')
				Node.m_Kind = EKind::mc_Any;
			else if (*pParse == '[' && fsp_SetEnd(pParse, _pEnd) != _pEnd)
			{
				Node.m_Kind = EKind::mc_Set;
				auto pEnd = fsp_SetEnd(pParse, _pEnd);
				++pParse;
				Node.m_bNegated = *pParse == '!';
				if (Node.m_bNegated)
					++pParse;

				while (pParse != pEnd)
				{
					auto First = *pParse++;
					if (First == '\\' && pParse != pEnd)
						First = *pParse++;
					auto Last = First;
					if (pEnd - pParse >= 2 && *pParse == '-')
					{
						++pParse;
						Last = *pParse++;
						if (Last == '\\' && pParse != pEnd)
							Last = *pParse++;
					}

					Node.m_Ranges.f_Insert({First, Last});
				}
			}
			else if (*pParse == '{')
			{
				umint nDepth = 1;
				TCVector<ch32 const *> Separators;
				auto pEnd = pParse + 1;
				for (; pEnd != _pEnd; ++pEnd)
				{
					if (*pEnd == '\\' && pEnd + 1 != _pEnd)
						++pEnd;
					else if (*pEnd == '[' && fsp_SetEnd(pEnd, _pEnd) != _pEnd)
						pEnd = fsp_SetEnd(pEnd, _pEnd);
					else if (*pEnd == '{')
						++nDepth;
					else if (*pEnd == '}' && --nDepth == 0)
						break;
					else if (*pEnd == ',' && nDepth == 1)
						Separators.f_Insert(pEnd);
				}

				if (pEnd != _pEnd && !Separators.f_IsEmpty())
				{
					auto iJoin = mp_Nodes.f_GetLen();
					mp_Nodes.f_Insert().m_Kind = EKind::mc_Empty;
					Node.m_Kind = EKind::mc_Branch;
					Separators.f_Insert(pEnd);
					auto pAlternative = pParse + 1;
					for (auto pSeparator : Separators)
					{
						Node.m_Alternatives.f_Insert(fp_Compile(pAlternative, pSeparator, iJoin));
						pAlternative = pSeparator + 1;
					}

					Sequence.f_Insert(mp_Nodes.f_GetLen());
					mp_Nodes.f_Insert(fg_Move(Node));
					Sequence.f_Insert(iJoin);
					pParse = pEnd;

					continue;
				}

				if (pEnd != _pEnd)
				{
					CStr Range(CUStr(pParse + 1, pEnd - pParse - 1));
					if (fg_IsNumericGlobRange(Range.f_GetStr(), Range.f_GetStr() + Range.f_GetLen()))
						DMibError("Numeric ranges in path patterns are not supported: {{{}}}"_f << Range);
				}
			}

			if (Node.m_Kind == EKind::mc_Literal && Node.m_Character == '/')
				mp_bHasSeparator = true;
			Sequence.f_Insert(mp_Nodes.f_GetLen());
			mp_Nodes.f_Insert(fg_Move(Node));
		}

		for (umint i = 0; i < Sequence.f_GetLen(); ++i)
			mp_Nodes[Sequence[i]].m_iNext = i + 1 == Sequence.f_GetLen() ? _iNext : Sequence[i + 1];

		return Sequence.f_IsEmpty() ? _iNext : Sequence[0];
	}

	// The consuming states reachable from a state without consuming a character: through
	// branches, empty nodes, the repeat of a star, and, at a component start, the separator a
	// recursive star may swallow. Computed once per node when compiling.
	void CPathGlob::fp_CollectClosure(umint _iState, bool _bComponentStart, TCVector<umint> &o_States) const
	{
		struct CPending
		{
			umint m_iState = 0;
			bool m_bSkipSeparator = false;
		};

		TCVector<uint8> Visited;
		Visited.f_SetLen(mp_Nodes.f_GetLen());
		for (auto &Value : Visited)
			Value = 0;

		TCVector<CPending> Pending = {{_iState, false}};
		while (!Pending.f_IsEmpty())
		{
			auto State = Pending.f_PopBack();
			auto iState = State.m_iState;
			uint8 Mask = State.m_bSkipSeparator ? 2 : 1;
			if (Visited[iState] & Mask)
				continue;
			Visited[iState] |= Mask;

			auto const &Node = mp_Nodes[iState];
			if (Node.m_Kind == EKind::mc_Branch)
			{
				for (auto iAlternative : Node.m_Alternatives)
					Pending.f_Insert({iAlternative, State.m_bSkipSeparator});
			}
			else if (Node.m_Kind == EKind::mc_Empty)
				Pending.f_Insert({Node.m_iNext, State.m_bSkipSeparator});
			else if (State.m_bSkipSeparator)
			{
				if (Node.m_Kind == EKind::mc_Literal && Node.m_Character == '/')
					Pending.f_Insert({Node.m_iNext, false});
			}
			else
			{
				o_States.f_Insert(iState);
				if (Node.m_Kind == EKind::mc_Star || Node.m_Kind == EKind::mc_RecursiveStar)
				{
					Pending.f_Insert({Node.m_iNext, false});
					if (Node.m_Kind == EKind::mc_RecursiveStar && _bComponentStart)
						Pending.f_Insert({Node.m_iNext, true});
				}
			}
		}
	}
}
