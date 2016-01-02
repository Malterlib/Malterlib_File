// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/File/Patch>

#if 0
class CTestDataProcessing : public CMalterlibTest
{
public:

	NMib::NStr::CStr Certify(CTestInterface &_Interface)
	{
		NMib::NContainer::TCVector<uint8> OrigVector;
		const ch8 *pStartStr = " nEHoanTHeus ntHUESanotheuSN thuSNOAtheuSNAOtheuSN athueSNOAtehuSN thuSNAOteuhNST hanSEUTAOheu";
		const ch8 *pEndStr = "khxlheu nEHoanTHeus ntHUESanotheuSN thuSNOAtheuSNAOtheuSN onethuu athueSNOAtehuSN thuSNAOteuhNST hanSEUTAOheu";
		OrigVector.f_Insert((const uint8 *)pStartStr, NMib::NStr::fg_StrLen(pStartStr));

		NMib::NContainer::TCVector<uint8> ChangedVector;
		ChangedVector.f_Insert((const uint8 *)pEndStr, NMib::NStr::fg_StrLen(pEndStr));

		NMib::NContainer::TCVector<uint8> PatchData;
		NMib::NDataProcessing::fg_MalterlibPatchEncode(OrigVector, ChangedVector, PatchData);

		NMib::NContainer::TCVector<uint8> Decoded;
		if (!NMib::NDataProcessing::fg_MalterlibPatchDecode(OrigVector, Decoded, PatchData))
			return "Patch decode failed";

		if (Decoded.f_Compare(ChangedVector) != 0)
			return "Patch was incorrectly decoded";

		return "";
	}
		
};


DMibRuntimeClass(CMalterlibTest, CTestDataProcessing);
#endif
