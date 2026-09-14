// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once

#include <Mib/Core/Core>
#include <Mib/Container/Vector>
#include <Mib/String/String>

namespace NMib::NFile
{
	// How a pattern relates to the files under a directory.
	enum class EPathGlobCover
	{
		mc_None			// No file below can match.
		, mc_Some		// Some file below may match.
		, mc_All		// Every file below matches.
	};

	// Which syntax a pattern is written in.
	enum class EPathGlobDialect
	{
		mc_EditorConfig	// '{a,b}' alternates and '**' crosses components wherever it stands.
		, mc_Git		// Braces are literal, '**' crosses components only as a whole component, and '[^...]' negates too.
	};

	// A glob over slash-separated paths, as EditorConfig sections and gitignore rules spell
	// them: '*' and '?' stay within one path component, '**' crosses components, '[...]' is a
	// set, '{a,b}' alternates, and '\' escapes. A pattern without a separator matches a file's
	// name at any depth; one with a separator matches the whole path relative to where the
	// pattern was written, and a leading '/' only anchors it. Ignoring case folds ASCII letters
	// only, as git's core.ignoreCase does.
	struct CPathGlob
	{
		// The working sets of a match. One path meets many patterns, so the caller owns them
		// and every match reuses their capacity. A state is in a set when its stamp is the
		// set's generation, which makes emptying a set one increment.
		struct CScratch
		{
			NContainer::TCVector<umint> m_Active;
			NContainer::TCVector<umint> m_Next;
			NContainer::TCVector<umint> m_Stamps;
			umint m_Generation = 0;
		};

		explicit CPathGlob(NStr::CStr const &_Pattern, EPathGlobDialect _Dialect = EPathGlobDialect::mc_EditorConfig, bool _bIgnoreCase = false);

		bool f_HasSeparator() const;

		// Matches a path relative to where the pattern was written; the file's name is passed
		// converted as well, since a pattern without a separator matches it alone.
		bool f_Match(NStr::CUStr const &_Path, NStr::CUStr const &_FileName, CScratch &_Scratch) const;

		// Judges the files under a directory, given relative to where the pattern was written
		// and empty for that directory itself.
		EPathGlobCover f_MatchBelow(NStr::CUStr const &_Directory, CScratch &_Scratch) const;

		// The code points of a path or pattern; NUL is rejected.
		static NStr::CUStr fs_ToUnicode(NStr::CStr const &_Text);

		// A path or pattern as the dialect matches it: git's by its bytes, one to a character, EditorConfig's by its code
		// points. NUL is rejected.
		static NStr::CUStr fs_ToText(NStr::CStr const &_Text, EPathGlobDialect _Dialect);

	private:
		enum struct EKind
		{
			mc_Accept
			, mc_Empty
			, mc_Branch
			, mc_Literal
			, mc_Any
			, mc_Star
			, mc_RecursiveStar
			, mc_Set
		};

		struct CRange
		{
			ch32 m_First = 0;
			ch32 m_Last = 0;
		};

		struct CNode
		{
			EKind m_Kind = EKind::mc_Literal;
			umint m_iNext = 0;
			ch32 m_Character = 0;
			bool m_bNegated = false;
			bool m_bVerbatim = false; // A character git's pattern escapes, compared as written, also where case is ignored.
			NContainer::TCVector<CRange> m_Ranges;
			NContainer::TCVector<umint> m_Alternatives;
			NContainer::TCVector<umint> m_Closure[2];		// The consuming states reached without consuming; [1] at a component start.
		};

		ch32 const *fp_SetEnd(ch32 const *_pStart, ch32 const *_pEnd) const;
		static ch32 const *fsp_ClassEnd(ch32 const *_pStart, ch32 const *_pEnd);
		bool fp_AddClassRanges(NStr::CStr const &_Name, NContainer::TCVector<CRange> &o_Ranges) const;
		umint fp_Compile(ch32 const *_pStart, ch32 const *_pEnd, umint _iNext);
		void fp_CollectClosure(umint _iState, bool _bComponentStart, NContainer::TCVector<umint> &o_States) const;
		void fp_Run(NStr::CUStr const &_Text, CScratch &_Scratch) const;
		bool fp_ReachesAccept(umint _iState, bool _bComponentStart) const;

		NContainer::TCVector<CNode> mp_Nodes;
		umint mp_iStart = 0;
		EPathGlobDialect mp_Dialect = EPathGlobDialect::mc_EditorConfig;
		bool mp_bHasSeparator = false;
		bool mp_bIgnoreCase = false;
		bool mp_bInvalid = false;       // A git pattern git rejects, an unclosed set or an unknown class, matches nothing.
		bool mp_bRecursiveName = false; // An EditorConfig pattern without a separator holds a '**', crossing directories.
	};
}
