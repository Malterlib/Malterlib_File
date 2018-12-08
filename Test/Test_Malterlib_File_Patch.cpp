// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/File/Patch>

#if 0
class CTestDataProcessing : public CMalterlibTest
{
public:

	NMib::NStr::CStr Certify(CTestInterface &_Interface)
	{
		NMib::NContainer::CByteVector OrigVector;
		const ch8 *pStartStr = " nEHoanTHeus ntHUESanotheuSN thuSNOAtheuSNAOtheuSN athueSNOAtehuSN thuSNAOteuhNST hanSEUTAOheu";
		const ch8 *pEndStr = "khxlheu nEHoanTHeus ntHUESanotheuSN thuSNOAtheuSNAOtheuSN onethuu athueSNOAtehuSN thuSNAOteuhNST hanSEUTAOheu";
		OrigVector.f_Insert((const uint8 *)pStartStr, NMib::NStr::fg_StrLen(pStartStr));

		NMib::NContainer::CByteVector ChangedVector;
		ChangedVector.f_Insert((const uint8 *)pEndStr, NMib::NStr::fg_StrLen(pEndStr));

		NMib::NContainer::CByteVector PatchData;
		NMib::NFile::fg_MalterlibPatchEncode(OrigVector, ChangedVector, PatchData);

		NMib::NContainer::CByteVector Decoded;
		if (!NMib::NFile::fg_MalterlibPatchDecode(OrigVector, Decoded, PatchData))
			return "Patch decode failed";

		if (Decoded.f_Compare(ChangedVector) != 0)
			return "Patch was incorrectly decoded";

		return "";
	}
		
};


DMibRuntimeClass(CMalterlibTest, CTestDataProcessing);
#endif
