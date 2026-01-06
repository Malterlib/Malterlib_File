// Copyright © 2015 Hansoft AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

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
				const ch8 *pStartStr = " nEHoanTHeus ntHUESanotheuSN thuSNOAtheuSNAOtheuSN athueSNOAtehuSN thuSNAOteuhNST hanSEUTAOheu";
				const ch8 *pEndStr = "khxlheu nEHoanTHeus ntHUESanotheuSN thuSNOAtheuSNAOtheuSN onethuu athueSNOAtehuSN thuSNAOteuhNST hanSEUTAOheu";
				OrigVector.f_Insert((const uint8 *)pStartStr, NMib::NStr::fg_StrLen(pStartStr));

				NMib::NContainer::CByteVector ChangedVector;
				ChangedVector.f_Insert((const uint8 *)pEndStr, NMib::NStr::fg_StrLen(pEndStr));

				NMib::NContainer::CByteVector PatchData = NMib::NFile::fg_MalterlibPatchEncode(OrigVector, ChangedVector);
				NMib::NContainer::CByteVector Decoded = NMib::NFile::fg_MalterlibPatchDecode(OrigVector, PatchData);

				mint Overhead = 40;
				mint FoundChunkOverhead = 4 * 3;
				mint ExpectedFoundChunks = 2;
				mint ExpectedRaw0 = 4 + 7;
				mint ExpectedRaw1 = 4 + 7;
				mint ExpectedSize = Overhead + FoundChunkOverhead * ExpectedFoundChunks + ExpectedRaw0 + ExpectedRaw1;

				DMibExpect(PatchData.f_GetLen(), ==, ExpectedSize);
				DMibExpect(Decoded, ==, ChangedVector);
			};
		}
	};

	DMibTestRegister(CPatch_Tests, Malterlib::File);
}
