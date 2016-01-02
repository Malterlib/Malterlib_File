// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/Core/Core>

namespace NMib
{
	namespace NDataProcessing
	{
		void fg_MalterlibPatchEncode(const NContainer::TCVector<uint8> &_Orig, const NContainer::TCVector<uint8> &_Changed, NContainer::TCVector<uint8> &_PatchData);
		bint fg_MalterlibPatchDecode(const NContainer::TCVector<uint8> &_Orig, NContainer::TCVector<uint8> &_Changed, const NContainer::TCVector<uint8> &_PatchData);
	}
}

