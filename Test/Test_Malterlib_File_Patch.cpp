// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Mib/File/Patch>

namespace
{
	struct CPatch_Tests : public NMib::NTest::CTest
	{
		void f_DoTests()
		{
			DMibTestSuite("Basic")
			{
				NMib::NContainer::CByteVector OrigVector;
				ch8 const *pStartStr = " nEHoanTHeus ntHUESanotheuSN thuSNOAtheuSNAOtheuSN athueSNOAtehuSN thuSNAOteuhNST hanSEUTAOheu";
				ch8 const *pEndStr = "khxlheu nEHoanTHeus ntHUESanotheuSN thuSNOAtheuSNAOtheuSN onethuu athueSNOAtehuSN thuSNAOteuhNST hanSEUTAOheu";
				OrigVector.f_Insert((uint8 const *)pStartStr, NMib::NStr::fg_StrLen(pStartStr));

				NMib::NContainer::CByteVector ChangedVector;
				ChangedVector.f_Insert((uint8 const *)pEndStr, NMib::NStr::fg_StrLen(pEndStr));

				NMib::NContainer::CByteVector PatchData = NMib::NFile::fg_MalterlibPatchEncode(OrigVector, ChangedVector);
				NMib::NContainer::CByteVector Decoded = NMib::NFile::fg_MalterlibPatchDecode(OrigVector, PatchData);

				umint Overhead = 40;
				umint FoundChunkOverhead = 4 * 3;
				umint ExpectedFoundChunks = 2;
				umint ExpectedRaw0 = 4 + 7;
				umint ExpectedRaw1 = 4 + 7;
				umint ExpectedSize = Overhead + FoundChunkOverhead * ExpectedFoundChunks + ExpectedRaw0 + ExpectedRaw1;

				DMibExpect(PatchData.f_GetLen(), ==, ExpectedSize);
				DMibExpect(Decoded, ==, ChangedVector);
			};
		}
	};

	DMibTestRegister(CPatch_Tests, Malterlib::File);
}
