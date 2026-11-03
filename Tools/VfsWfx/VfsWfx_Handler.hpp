// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once

inline CStr const &CFileSystem::CCompare::operator () (CFileSystem const &_Node) const
{
	return _Node.m_Name;
}

inline bool CFileSystem::CCompare::operator () (CStr const &_Left, CStr const &_Right) const
{
	return fg_StrCmp(_Left, _Right) < 0;
}
