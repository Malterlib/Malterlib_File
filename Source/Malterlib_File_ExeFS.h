// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once

#include <Mib/Core/Core>
#include <Mib/File/MalterlibFS>

namespace NMib::NFile
{
	bool fg_ReadExeFSFile(NStr::CStr const &_FileName, NContainer::CByteVector &_ReadTo);

	struct CExeFS
	{
		NMib::NFile::CVirtualFS m_FileSystem;

		// Either these two...
		NMib::NFile::TCBinaryStreamFile<> m_ExeFile;
		NStream::TCBinaryStreamSubStream<> m_SubStream;
		// ...or this one will be in use depending on how the ExeFs files were attached to the exe.
		NStream::TCBinaryStreamMemoryPtr<> m_InProcStream;

		~CExeFS();
	};

	bool fg_OpenExeFS(CExeFS &_FS);
}

#ifndef DMibPNoShortCuts
using namespace NMib::NFile;
#endif
