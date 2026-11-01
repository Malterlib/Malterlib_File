// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Mib/Core/Core>

namespace NMib::NFile
{
	NContainer::CByteVector fg_MalterlibPatchEncode(NContainer::CByteVector const &_Orig, NContainer::CByteVector const &_Changed);
	NContainer::CByteVector fg_MalterlibPatchDecode(NContainer::CByteVector const &_Orig, NContainer::CByteVector const &_PatchData);
}

#ifndef DMibPNoShortCuts
using namespace NMib::NFile;
#endif
