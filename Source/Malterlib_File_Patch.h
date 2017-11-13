// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/Core/Core>

namespace NMib
{
	namespace NFile
	{
		NContainer::TCVector<uint8> fg_MalterlibPatchEncode(const NContainer::TCVector<uint8> &_Orig, const NContainer::TCVector<uint8> &_Changed);
		NContainer::TCVector<uint8> fg_MalterlibPatchDecode(const NContainer::TCVector<uint8> &_Orig, const NContainer::TCVector<uint8> &_PatchData);
	}
}
