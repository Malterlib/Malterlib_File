// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <Mib/File/MalterlibFS>

#if 0
class CTestVirtualFS : public CMalterlibTest
{
public:


	NMib::NStr::CStr Certify(CTestInterface &_Interface)
	{
//		DMibTrace("\n\nVirtualFS\n\n", 0);

		NMib::NStr::CStr FileName = "VirtualFS.bin";
		NMib::NFile::TCBinaryStreamFile<> File;
		NMib::NFile::CVirtualFS VirtualFS;


		{
			NMib::NFile::TCBinaryStreamFile<> File;
			NMib::NFile::CVirtualFS VirtualFS;
			File.f_Open(FileName, NMib::NFile::EFileOpen_Write | NMib::NFile::EFileOpen_Read);
			VirtualFS.f_Create(&File, 128, 512*1024, 1024*1024);
		}

		{
			File.f_Open(FileName, NMib::NFile::EFileOpen_Write | NMib::NFile::EFileOpen_Read | NMib::NFile::EFileOpen_DontTruncate);
			VirtualFS.f_Open(&File);
		}

		{
			VirtualFS.f_CreateDirectory("Test/Test/Test1/Test3");
		}

		{
			NMib::NFile::CVirtualFSFile VirtualFile;
			VirtualFile.f_Open(VirtualFS, "Test.bin", NMib::NFile::EFileOpen_Write);

			VirtualFile.f_SetPositionFromEnd(10000);
			uint32 Test = 0;
			VirtualFile.f_Write(&Test, 4);
		}

		return ("");
	}


};


DMibRuntimeClass(CMalterlibTest, CTestVirtualFS);
#endif
