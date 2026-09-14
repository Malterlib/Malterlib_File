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

	ch32 fg_FoldCase(ch32 _Character)
	{
		return _Character >= 'A' && _Character <= 'Z' ? _Character + ('a' - 'A') : _Character;
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

	CUStr CPathGlob::fs_ToText(CStr const &_Text, EPathGlobDialect _Dialect)
	{
		if (_Dialect != EPathGlobDialect::mc_Git)
			return fs_ToUnicode(_Text);

		CUStr Text;
		auto pText = Text.f_GetStr(_Text.f_GetLen() + 1);
		auto pSource = _Text.f_GetStr();
		for (umint i = 0; i < _Text.f_GetLen(); ++i)
		{
			if (pSource[i] == 0)
				DMibError("NUL is not allowed in path patterns or paths");

			pText[i] = ch32(uch8(pSource[i]));
		}

		pText[_Text.f_GetLen()] = 0;
		Text.f_SetStrLen(_Text.f_GetLen());

		return Text;
	}

	CPathGlob::CPathGlob(CStr const &_Pattern, EPathGlobDialect _Dialect, bool _bIgnoreCase)
		: mp_Dialect(_Dialect)
		, mp_bIgnoreCase(_bIgnoreCase)
	{
		mp_bHasSeparator = _Pattern.f_StartsWith("/");
		auto Pattern = fs_ToText(_Pattern.f_RemovePrefix("/"), _Dialect);
		if (Pattern.f_GetLen() > 1024)
			DMibError("Path patterns longer than 1024 characters are not supported");

		mp_Nodes.f_Insert().m_Kind = EKind::mc_Accept;
		mp_iStart = fp_Compile(Pattern.f_GetStr(), Pattern.f_GetStr() + Pattern.f_GetLen(), 0);
		for (auto const &Node : mp_Nodes)
			mp_bRecursiveName |= mp_Dialect == EPathGlobDialect::mc_EditorConfig && !mp_bHasSeparator && Node.m_Kind == EKind::mc_RecursiveStar;

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

		Active.f_ClearNoTrim();
		++_Scratch.m_Generation;
		fAddStates(mp_iStart, true, Active);

		auto pEnd = _Text.f_GetStr() + _Text.f_GetLen();
		for (auto pParse = _Text.f_GetStr(); pParse != pEnd; ++pParse)
		{
			auto Character = mp_bIgnoreCase ? fg_FoldCase(*pParse) : *pParse;

			Next.f_ClearNoTrim();
			++_Scratch.m_Generation;
			for (auto iState : Active)
			{
				auto const &Node = mp_Nodes[iState];
				bool bMatches = false;
				switch (Node.m_Kind)
				{
					case EKind::mc_Literal:
						bMatches = Character == (mp_bIgnoreCase && !Node.m_bVerbatim ? fg_FoldCase(Node.m_Character) : Node.m_Character);

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
							bMatches |= Character >= Range.m_First && Character <= Range.m_Last;
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
		if (mp_bInvalid)
			return false;

		auto fAccepts = [&](CUStr const &_Text)
			{
				fp_Run(_Text, _Scratch);
				for (auto iState : _Scratch.m_Active)
				{
					if (mp_Nodes[iState].m_Kind == EKind::mc_Accept)
						return true;
				}

				return false;
			}
		;

		if (!mp_bRecursiveName)
			return fAccepts(mp_bHasSeparator ? _Path : _FileName);

		// EditorConfig reads a pattern without a separator as one behind '**/', so a '**' in it matches from any
		// directory down: 'a**z.c' meets 'x/a/nested/z.c'.
		if (fAccepts(_Path))
			return true;

		auto pPath = _Path.f_GetStr();
		for (umint i = 0; i < _Path.f_GetLen(); ++i)
		{
			if (pPath[i] == '/' && fAccepts(CUStr(pPath + i + 1, _Path.f_GetLen() - i - 1)))
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
		if (mp_bInvalid)
			return EPathGlobCover::mc_None;

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
		// A recursive star that can end the pattern swallows all of them. It must reach the end after
		// consuming a name, as a path below does, not only by swallowing a separator.
		CUStr Prefix = _Directory;
		if (Prefix)
			Prefix += U"/";

		fp_Run(Prefix, _Scratch);
		if (_Scratch.m_Active.f_IsEmpty())
			return EPathGlobCover::mc_None;

		for (auto iState : _Scratch.m_Active)
		{
			auto const &Node = mp_Nodes[iState];
			if (Node.m_Kind == EKind::mc_RecursiveStar && fp_ReachesAccept(Node.m_iNext, false))
				return EPathGlobCover::mc_All;
		}

		return EPathGlobCover::mc_Some;
	}

	// The ']' ending the git class '[:name:]' that starts at _pStart: the first ']' behind it, when a ':' stands in front of
	// it. Null for a '[' that opens no class, whose characters are then members of the set.
	ch32 const *CPathGlob::fsp_ClassEnd(ch32 const *_pStart, ch32 const *_pEnd)
	{
		if (_pStart + 1 == _pEnd || *_pStart != '[' || *(_pStart + 1) != ':')
			return nullptr;

		auto pClose = _pStart + 2;
		while (pClose != _pEnd && *pClose != ']')
			++pClose;

		if (pClose == _pEnd || pClose == _pStart + 2 || *(pClose - 1) != ':')
			return nullptr;

		return pClose;
	}

	// In git's syntax the set is read member by member as fp_Compile reads it: a ']' leading the set, behind its
	// negation, is a member, a '[:class:]' is one member and starts no range, and a range's end is one character, a '['
	// too.
	ch32 const *CPathGlob::fp_SetEnd(ch32 const *_pStart, ch32 const *_pEnd) const
	{
		auto pParse = _pStart + 1;

		// EditorConfig defines no leading ']' as a member, which its own tests write escaped: '[\]ab]'. Reading one as git
		// does is out of scope by decision.
		if (mp_Dialect != EPathGlobDialect::mc_Git)
		{
			while (pParse != _pEnd && *pParse != ']')
			{
				if (*pParse == '\\' && pParse + 1 != _pEnd)
					++pParse;

				++pParse;
			}

			return pParse;
		}

		if (pParse != _pEnd && (*pParse == '!' || *pParse == '^'))
			++pParse;

		// A ']' leading the set is its first member, which may start a range as any other.
		bool bFirst = true;

		auto fSkipCharacter = [&]
			{
				if (*pParse == '\\' && pParse + 1 != _pEnd)
					++pParse;

				++pParse;
			}
		;

		while (pParse != _pEnd && (*pParse != ']' || bFirst))
		{
			bFirst = false;
			if (auto pClassEnd = fsp_ClassEnd(pParse, _pEnd))
			{
				pParse = pClassEnd + 1;

				continue;
			}

			fSkipCharacter();
			if (pParse != _pEnd && *pParse == '-' && pParse + 1 != _pEnd && *(pParse + 1) != ']')
			{
				++pParse;
				fSkipCharacter();
			}
		}

		return pParse;
	}

	// A POSIX class of git's sets, over ASCII as git's are. False for a class git does not know.
	bool CPathGlob::fp_AddClassRanges(CStr const &_Name, TCVector<CRange> &o_Ranges) const
	{
		struct CClass
		{
			ch8 const *m_pName;
			ch8 const *m_pRanges; // Pairs of first and last.
		};

		static constexpr CClass c_Classes[] =
			{
				{"alnum", "09AZaz"}
				, {"alpha", "AZaz"}
				, {"blank", "  \t\t"}
				, {"cntrl", "\x01\x1F\x7F\x7F"}
				, {"digit", "09"}
				, {"graph", "!~"}
				, {"lower", "az"}
				, {"print", " ~"}
				, {"punct", "!/:@[`{~"}
				, {"space", "\t\r  "}
				, {"upper", "AZ"}
				, {"xdigit", "09AFaf"}
			}
		;

		for (auto const &Class : c_Classes)
		{
			if (_Name != Class.m_pName)
				continue;

			for (auto pRange = Class.m_pRanges; *pRange; pRange += 2)
			{
				ch32 First = uch8(pRange[0]);
				ch32 Last = uch8(pRange[1]);
				o_Ranges.f_Insert({First, Last});
				if (mp_bIgnoreCase && First <= 'Z' && Last >= 'A')
					o_Ranges.f_Insert({fg_FoldCase(fg_Max(First, ch32('A'))), fg_FoldCase(fg_Min(Last, ch32('Z')))});
			}

			return true;
		}

		return false;
	}

	umint CPathGlob::fp_Compile(ch32 const *_pStart, ch32 const *_pEnd, umint _iNext)
	{
		TCVector<umint> Sequence;
		for (auto pParse = _pStart; pParse != _pEnd; ++pParse)
		{
			CNode Node;
			Node.m_Character = *pParse;
			if (*pParse == '\\' && pParse + 1 != _pEnd)
			{
				Node.m_Character = *++pParse;
				Node.m_bVerbatim = mp_Dialect == EPathGlobDialect::mc_Git;
			}
			else if (*pParse == '\\' && mp_Dialect == EPathGlobDialect::mc_Git)
				mp_bInvalid = true; // git rejects a pattern ending in an escape of nothing.
			else if (*pParse == '*')
			{
				Node.m_Kind = EKind::mc_Star;
				if (pParse + 1 != _pEnd && *(pParse + 1) == '*')
				{
					bool bComponentStart = pParse == _pStart || *(pParse - 1) == '/';
					while (pParse + 1 != _pEnd && *(pParse + 1) == '*')
						++pParse;

					bool bComponentEnd = pParse + 1 == _pEnd || *(pParse + 1) == '/' || (pParse + 2 < _pEnd && *(pParse + 1) == '\\' && *(pParse + 2) == '/');
					if (mp_Dialect != EPathGlobDialect::mc_Git || (bComponentStart && bComponentEnd))
						Node.m_Kind = EKind::mc_RecursiveStar;
				}
			}
			else if (*pParse == '?')
				Node.m_Kind = EKind::mc_Any;
			else if (*pParse == '[' && mp_Dialect == EPathGlobDialect::mc_Git && fp_SetEnd(pParse, _pEnd) == _pEnd)
				mp_bInvalid = true;
			// A bracket holding a separator is a set, which matches no separator, where EditorConfig's core reads it as the
			// characters it is written with: a deliberate difference, which the documentation names.
			else if (*pParse == '[' && fp_SetEnd(pParse, _pEnd) != _pEnd)
			{
				Node.m_Kind = EKind::mc_Set;
				auto pEnd = fp_SetEnd(pParse, _pEnd);

				// git anchors a pattern at a separator written anywhere in it, inside a set too, though no set matches one.
				for (auto pSlash = pParse; mp_Dialect == EPathGlobDialect::mc_Git && pSlash != pEnd; ++pSlash)
					mp_bHasSeparator |= *pSlash == '/';

				++pParse;
				Node.m_bNegated = *pParse == '!' || (mp_Dialect == EPathGlobDialect::mc_Git && *pParse == '^');
				if (Node.m_bNegated)
					++pParse;

				while (pParse != pEnd)
				{
					auto First = *pParse++;
					// A '[:' that the first ']' behind it does not close as ':]' is two members of the set.
					auto pClassEnd = mp_Dialect == EPathGlobDialect::mc_Git && First == '[' ? fsp_ClassEnd(pParse - 1, pEnd) : nullptr;
					if (pClassEnd)
					{
						auto pName = pParse + 1;
						if (!fp_AddClassRanges(CStr(CUStr(pName, pClassEnd - 1 - pName)), Node.m_Ranges))
						{
							mp_bInvalid = true;
							pParse = pEnd;

							break;
						}

						pParse = pClassEnd + 1;

						continue;
					}

					if (First == '\\' && pParse != pEnd)
						First = *pParse++;
					auto Last = First;
					bool bRange = pEnd - pParse >= 2 && *pParse == '-';
					if (bRange)
					{
						++pParse;
						Last = *pParse++;
						if (Last == '\\' && pParse != pEnd)
							Last = *pParse++;
					}

					Node.m_Ranges.f_Insert({First, Last});

					// git matches a descending range's first character, as it compares that before the range.
					if (mp_Dialect == EPathGlobDialect::mc_Git && Last < First)
						Node.m_Ranges.f_Insert({First, First});

					// The text is folded to lower case, so a range's upper case letters stand for their lower case. git compares
					// a single member as written.
					bool bFolds = mp_Dialect != EPathGlobDialect::mc_Git || bRange;
					if (mp_bIgnoreCase && bFolds && First <= 'Z' && Last >= 'A')
						Node.m_Ranges.f_Insert({fg_FoldCase(fg_Max(First, ch32('A'))), fg_FoldCase(fg_Min(Last, ch32('Z')))});
				}
			}
			else if (*pParse == '{' && mp_Dialect != EPathGlobDialect::mc_Git)
			{
				umint nDepth = 1;
				TCVector<ch32 const *> Separators;
				auto pEnd = pParse + 1;
				for (; pEnd != _pEnd; ++pEnd)
				{
					if (*pEnd == '\\' && pEnd + 1 != _pEnd)
						++pEnd;
					else if (*pEnd == '[' && fp_SetEnd(pEnd, _pEnd) != _pEnd)
						pEnd = fp_SetEnd(pEnd, _pEnd);
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
				// Only a separator written as one lets a recursive star match no directory: git's '**\/' needs one.
				if (Node.m_Kind == EKind::mc_Literal && Node.m_Character == '/' && !Node.m_bVerbatim)
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
