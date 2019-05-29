// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#include <Mib/Core/Core>
#include <Mib/File/MalterlibFS>

namespace NMib::NFile
{
	bool fg_ReadExeFSFile(NStr::CStr _FileName, NContainer::CByteVector &_ReadTo);

	class CExeFS
	{
	public:
		NMib::NFile::CVirtualFS m_FileSystem;

		// Either these two...
		NMib::NFile::TCBinaryStreamFile<> m_ExeFile;
		NStream::CBinaryStreamSubStream<> m_SubStream;
		// ...or this one will be in use depending on how the ExeFs files were attached to the exe.
		NStream::CBinaryStreamMemoryPtr<> m_InProcStream;

		~CExeFS()
		{
			// Close in correct order
			m_FileSystem.f_Close();
			m_SubStream.f_Close();
			m_ExeFile.f_Close();
		}
	};
	
	bool fg_OpenExeFS(CExeFS &_FS);
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif
