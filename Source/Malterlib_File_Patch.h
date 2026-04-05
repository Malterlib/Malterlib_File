// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Mib/Core/Core>

namespace NMib::NFile
{
	NContainer::CByteVector fg_MalterlibPatchEncode(const NContainer::CByteVector &_Orig, const NContainer::CByteVector &_Changed);
	NContainer::CByteVector fg_MalterlibPatchDecode(const NContainer::CByteVector &_Orig, const NContainer::CByteVector &_PatchData);
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif
