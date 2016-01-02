// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_ExeFS.h"

namespace NAOCC
{
	mint g_ExeFS_nBytes = 0;
	uint8 const* g_ExeFS_pBytes = nullptr;

	void fg_SetExeFSData(int _nBytes, void const *_pData)
	{
		g_ExeFS_nBytes = _nBytes;
		g_ExeFS_pBytes = (uint8 const *)_pData;
	}
}

namespace NMib
{
	namespace NFile
	{

		bint fg_OpenExeFS(CExeFS &_FS)
		{
			// Try new style first.
			if (NAOCC::g_ExeFS_nBytes && NAOCC::g_ExeFS_pBytes)
			{
				try
				{
					_FS.m_InProcStream.f_OpenRead(NAOCC::g_ExeFS_pBytes, NAOCC::g_ExeFS_nBytes);
					_FS.m_FileSystem.f_Open(&_FS.m_InProcStream);
					return true;
				}
				catch(NException::CException)
				{		
					return false;
				}
			}
			
	#if defined(DCompiler_GCC)
			unsigned long long nDataBytes;
			void* pData = NMib::NSys::fg_GetExeData("HS","ExeFs", nDataBytes);
			
			if (!pData)
				return false;
			
			try
			{
				_FS.m_InProcStream.f_OpenRead(pData, nDataBytes);
				_FS.m_FileSystem.f_Open(&_FS.m_InProcStream);
				return true;
			}
			catch(NException::CException)
			{		
			}
			return false;
			
	#elif defined(DPlatformFamily_Windows)
			try
			{
				NStr::CStr FileName = NFile::CFile::fs_GetModulePath(NMisc::fg_FunctionPtrToVoidPtr(&fg_ReadExeFSFile));

				_FS.m_ExeFile.f_Open(FileName, EFileOpen_Read|EFileOpen_ShareAll);
				int32 ClusterSize = 1024;

				int32 nClustersToSearch = 1024;
				CMibFilePos FileSize = fg_AlignDown(_FS.m_ExeFile.f_GetLength() -(sizeof(int64) + 8), ClusterSize);

				const ch8 *pSigOrig = "SFEXESDI"; 
				ch8 Temp[9];
				for (mint i = 0; i < 8; ++i)
					Temp[i] = pSigOrig[7-i];
				Temp[8] = 0;
				const ch8 *pSig = Temp;

				const mint SigSize = 8;
				ch8 Signature[SigSize+1];
				Signature[SigSize] = 0;

				while (nClustersToSearch && FileSize > 0)
				{
					--nClustersToSearch;
					_FS.m_ExeFile.f_SetPosition(FileSize);

					int64 MalterlibFsFilePos = 0;

					_FS.m_ExeFile.f_ConsumeBytes(Signature, SigSize);

					if (NStr::fg_StrCmp(Signature, pSig) == 0)
					{
						// We have an existing file system
						_FS.m_ExeFile >> MalterlibFsFilePos;
						_FS.m_SubStream.f_Open(&_FS.m_ExeFile, MalterlibFsFilePos, FileSize - MalterlibFsFilePos);
						_FS.m_FileSystem.f_Open(&_FS.m_SubStream);

						return true;
					}
					else
					{
						FileSize -= ClusterSize;
					}
				}
			}
			catch (NException::CException)
			{
			}
			return false;
	#endif 
			return false;
		}

		bint fg_ReadExeFSFile(NStr::CStr _FileName, NContainer::TCVector<uint8> &_ReadTo)
		{
			CExeFS ExeFs;

			if (!fg_OpenExeFS(ExeFs))
				return false;

			if (!ExeFs.m_FileSystem.f_FileExists(_FileName))
				return false;
			return ExeFs.m_FileSystem.f_ReadFileToMemory(_FileName, _ReadTo);
		}
	}
}

