// Copyright © 2015 Hansoft AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "PCH.h"
#include "windows.h"
// contents of fsplugin.h  version 1.3 (10.Dec.2002)
#include "fsplugin.h"

#define DMibAllowCodeStandardViolations 1


uint32 MalterlibFileAttribToWin32FileAttrib(uint32 _Attrib);
uint32 Win32FileAttribToMalterlibFileAttrib(uint32 _Attrib);
FILETIME MalterlibTimeToFileTime(CTime _Time);
CTime FileTimeToMalterlibTime(FILETIME _Time);


static CStr fg_StrFromWindows(const CWStr &_Str)
{
	CStr Temp;
	Temp.fp_CopyFrom(_Str.f_GetStr(), _Str.f_GetLen());
	return Temp.f_EncodeUTF8(false, false);
}


aint CVfsWfx::f_Init(FProgressProc _ProgressProc, FLogProc _LogProc, FRequestProc _RequestProc, aint _PluginNumber)
{
	m_ProgressProc = _ProgressProc;
	m_LogProc = _LogProc;
	m_RequestProc = _RequestProc;
	m_PluginNumber = _PluginNumber;

	return 0;
}

void CVfsWfx::f_SetIniPath(CStr _Path)
{
	m_IniPath = _Path + "/IdsVfs.Settings";

	f_LoadSettings();
}

void CVfsWfx::f_SaveSettings()
{
	TCBinaryStreamFile<> File;
	File.f_Open(m_IniPath, EFileOpen_Write);

	File << m_FileSystems;

}

void CVfsWfx::f_LoadSettings()
{
	if (CFile::fs_FileExists(m_IniPath))
	{
		TCBinaryStreamFile<> File;
		File.f_Open(m_IniPath, EFileOpen_Read);

		File >> m_FileSystems;
	}

}

class CFindRoot : public CFindInstance
{
	mint m_iCurrent;
	TCVector<CStr> m_lStatic;

	CVfsWfx::CFileSysIter m_FileSysIter;
public:

	CFindRoot()
	{
		m_iCurrent = 0;
		m_lStatic.f_SetLen(3);
		m_lStatic[0] = "Add file system";
		m_lStatic[1] = "Create file system";
		m_lStatic[2] = "Unload all file systems";
	}

	int f_GetNext(CFindData &_Data)
	{
		_Data.m_LastWriteTime = _Data.m_LastAccessTime = _Data.m_CreationTime = CTime::fs_NowUTC();
		_Data.m_FileSize = 0;
		_Data.m_FileAttributes = 0;

		if (m_iCurrent < m_lStatic.f_GetLen())
		{
			_Data.m_FileName = m_lStatic[m_iCurrent];
			++m_iCurrent;
			return 1;
		}
		else
		{
			if (m_iCurrent == m_lStatic.f_GetLen())
				m_FileSysIter = g_VfsWfx.m_FileSystems;

			++m_iCurrent;

			if (m_FileSysIter)
			{
				_Data.m_FileAttributes = EFileAttrib_Directory;
				_Data.m_FileName = m_FileSysIter->m_Name;
				++m_FileSysIter;
				return 1;
			}

			return 0;
		}
	}
};


class CFindFileSystem : public CFindInstance
{
	mint m_iCurrent;

	CFileSystem *m_pFileSystem;
	TCVector<CStr> m_Files;
	CStr m_Path;

public:

	CFindFileSystem(CFileSystem *_pFileSystem, CStr _Path)
	{
		m_Path = _Path;

		if (m_Path != "")
			m_Path = m_Path + "/";

		m_pFileSystem = _pFileSystem;
		m_iCurrent = 0;
		if (m_pFileSystem->OpenFs())
		{
			try
			{
				m_pFileSystem->m_pVirtualFS->f_EnumFiles(_Path, m_Files);
			}
			catch (CException)
			{
			}
		}
	}

	int f_GetNext(CFindData &_Data)
	{


		if (m_iCurrent < m_Files.f_GetLen())
		{
			_Data.m_FileAttributes = m_pFileSystem->m_pVirtualFS->f_GetAttributes(m_Path + m_Files[m_iCurrent]);
			_Data.m_LastWriteTime = m_pFileSystem->m_pVirtualFS->f_GetWriteTime(m_Path + m_Files[m_iCurrent]);
			_Data.m_LastAccessTime = m_pFileSystem->m_pVirtualFS->f_GetAccessTime(m_Path + m_Files[m_iCurrent]);
			_Data.m_CreationTime = m_pFileSystem->m_pVirtualFS->f_GetCreationTime(m_Path + m_Files[m_iCurrent]);
			_Data.m_FileSize = m_pFileSystem->m_pVirtualFS->f_GetFileSize(m_Path + m_Files[m_iCurrent]);
			_Data.m_FileName = m_Files[m_iCurrent];
			++m_iCurrent;

			return 1;
		}

		return 0;
	}
};

CFileSystem *CVfsWfx::fp_GetFS(CStr _Path, CStr &_FsPath)
{
	if (fg_StrLen(_Path) > 1)
	{
		CStr Path;

		Path = _Path.f_Extract(1);

		aint iFind = Path.f_Find("/");

		CStr ToFind;
		_FsPath.f_Clear();
		if (iFind >= 0)
		{
			ToFind = Path.f_Left(iFind);
			_FsPath = Path.f_Extract(iFind + 1);
		}
		else
		{
			ToFind = Path;
		}

		CFileSystem *pFileSys = g_VfsWfx.m_FileSystems.f_FindEqual(ToFind);

		return pFileSys;
	}
	return nullptr;
}


CFindInstance *CVfsWfx::f_FindFirstFile(const ch16 *_pPath)
{
	if (_pPath == CStr("/"))
	{
		return DNew CFindRoot;
	}
	else if (fg_StrLen(_pPath) > 1)
	{

		CStr FsPath;

		CFileSystem *pFileSys = fp_GetFS(fg_StrFromWindows(_pPath), FsPath);

		if (!pFileSys)
			return nullptr;

		return DNew CFindFileSystem(pFileSys, FsPath);
	}
	return nullptr;
}

bool CVfsWfx::f_FindNextFile(CFindInstance * _pInstance, CFindData &_FindData)
{
	return _pInstance->f_GetNext(_FindData);
}

void CVfsWfx::f_FindClose(CFindInstance * _pInstance)
{
	delete _pInstance;
}

int CVfsWfx::f_CreateDirectory(const ch16 *_pDirectory)
{
	CStr FsPath;
	CFileSystem *pFileSys = fp_GetFS(fg_StrFromWindows(_pDirectory), FsPath);

	if (pFileSys)
	{
		try
		{
			pFileSys->OpenFs();
			pFileSys->m_pVirtualFS->f_CreateDirectory(FsPath);
			return 1;
		}
		catch (CException)
		{
		}
	}

	return 0;
}

int CVfsWfx::f_Execute(void *_pWinMain, CStr &_RemoteName, CStr const &_Verb)
{
	bool bChanged = false;
	if (_Verb == "open")
	{
		if (_RemoteName == "/Add file system")
		{
			DDTrace("Add file system\n", 0);
			CEditFileSystemClass Params;
			Params.m_Heading = "Add Malterlib File System";
			Params.m_ClusterSize = -1;
			Params.m_GrowSize = -1;
			Params.m_InitialSize = -1;
			Params.m_VolumeName = "Default";
			Params.m_pParentWindow = _pWinMain;
			Params.m_bCreateFile = false;

			while (1)
			{
				if (fg_EditFileSystem(Params))
				{
					CFileSystem *pFileSys = g_VfsWfx.m_FileSystems.f_FindEqual(Params.m_VolumeName);
					if (pFileSys)
					{
						fg_ErrorBox(_pWinMain, "The volume name already exists.");
						continue;
					}
					if (!CFile::fs_FileExists(Params.m_FilePath))
					{
						fg_ErrorBox(_pWinMain, "Cannot find the specified file.");
						continue;
					}

					pFileSys = DNew CFileSystem;
					pFileSys->m_Name = Params.m_VolumeName;
					pFileSys->m_FileName = Params.m_FilePath;
					g_VfsWfx.m_FileSystems.f_Insert(pFileSys);
					bChanged = true;

					g_VfsWfx.f_SaveSettings();

					break;
				}
				else
					break;
			}
		}
		else if (_RemoteName == "/Create file system")
		{
			DDTrace("Create file system\n", 0);
			CEditFileSystemClass Params;
			Params.m_Heading = "Create New Malterlib File System";
			Params.m_ClusterSize = 16384;
			Params.m_GrowSize = 1024*1024;
			Params.m_InitialSize = 1024*1024;
			Params.m_VolumeName = "Default";
			Params.m_pParentWindow = _pWinMain;
			Params.m_bCreateFile = true;

			while (1)
			{
				if (fg_EditFileSystem(Params))
				{
					CFileSystem *pFileSys = g_VfsWfx.m_FileSystems.f_FindEqual(Params.m_VolumeName);
					if (pFileSys)
					{
						fg_ErrorBox(_pWinMain, "The volume name already exists.");
						continue;
					}

					try
					{
						CVirtualFS NewFs;
						TCBinaryStreamFile<> File;
						File.f_Open(Params.m_FilePath, NMib::NFile::EFileOpen_Write | NMib::NFile::EFileOpen_Read);
						NewFs.f_Create(&File, Params.m_ClusterSize, Params.m_GrowSize, Params.m_InitialSize);
					}
					catch (NException::CException &_Exception)
					{
						fg_ErrorBox(_pWinMain, "Failed to create the file system. The error returned was:\n" + CStr(_Exception.f_GetErrorStr()));
						continue;
					}

					pFileSys = DNew CFileSystem;
					pFileSys->m_Name = Params.m_VolumeName;
					pFileSys->m_FileName = Params.m_FilePath;
					g_VfsWfx.m_FileSystems.f_Insert(pFileSys);
					bChanged = true;

					g_VfsWfx.f_SaveSettings();

					break;
				}
				else
					break;
			}

		}
		else if (_RemoteName == "/Unload all file systems")
		{
			DDTrace("Unload all file systems\n", 0);
			g_VfsWfx.fp_UnloadAll();
		}

	}

	if (_Verb == "properties")
	{
		if (_RemoteName.f_FindCharReverse('/') == 0)
		{
            CStr Name = _RemoteName.f_Extract(1);

			CFileSystem *pFileSystem = g_VfsWfx.m_FileSystems.f_FindEqual(Name);
			if (pFileSystem)
			{
				g_VfsWfx.m_FileSystems.f_Remove(pFileSystem);

				CEditFileSystemClass Params;
				Params.m_Heading = "Edit Malterlib File System";
				if (pFileSystem->m_pVirtualFS)
				{
					Params.m_ClusterSize = -((int)pFileSystem->m_pVirtualFS->f_GetClusterSize());
                	Params.m_GrowSize = pFileSystem->m_pVirtualFS->f_GetFileSystemGrowSize();
				}
				else
				{
					Params.m_ClusterSize = -1;
                	Params.m_GrowSize = -1;
				}
				Params.m_InitialSize = -1;
				Params.m_VolumeName = pFileSystem->m_Name;
				Params.m_pParentWindow = _pWinMain;
				Params.m_bCreateFile = false;
				Params.m_FilePath = pFileSystem->m_FileName;

				while (1)
				{
					aint Ret = fg_EditFileSystem(Params);
					if (Ret == 1)
					{
						CFileSystem *pFileSys = g_VfsWfx.m_FileSystems.f_FindEqual(Params.m_VolumeName);
						if (pFileSys)
						{
							fg_ErrorBox(_pWinMain, "The volume name already exists.");
							continue;
						}
						if (!CFile::fs_FileExists(Params.m_FilePath))
						{
							fg_ErrorBox(_pWinMain, "Cannot find the specified file.");
							continue;
						}

						pFileSystem->m_FileName = Params.m_FilePath;
						pFileSystem->m_Name = Params.m_VolumeName;

						if (pFileSystem->m_pVirtualFS)
							pFileSystem->m_pVirtualFS->f_SetFileSystemGrowSize(Params.m_GrowSize);

						bChanged = true;

						break;
					}
					else if (Ret == 2)
					{
						CVirtualFS::ECheckFSError Error = pFileSystem->f_CheckFS();
						if (Error == CVirtualFS::ECheckFSError_InplaceFixable)
						{
							if (MessageBoxA((HWND)_pWinMain, "The file system contains fixable errors.\nWould you like to fix them?", "Check Virtual FS", MB_YESNO) == IDYES)
							{
								if (pFileSystem->f_CheckFSFix())
									fg_ErrorBox(_pWinMain, "Failed to fix the file system.");
								else
									MessageBoxA((HWND)_pWinMain, "The file system was successfully fixed.", "Check Virtual FS", MB_OK);
							}
						}
						else if (Error == CVirtualFS::ECheckFSError_NewFSFixable)
						{
							if (MessageBoxA((HWND)_pWinMain, "The file system contains fixable errors.\nA new file system must be created.\nWould you like to fix them?", "Check Virtual FS", MB_YESNO) == IDYES)
							{
								if (pFileSystem->f_CheckFSFixNewFS())
									fg_ErrorBox(_pWinMain, "Failed to fix the file system.");
								else
									MessageBoxA((HWND)_pWinMain, "The file system was successfully fixed.", "Check Virtual FS", MB_OK);
							}
						}
						else if (Error == CVirtualFS::ECheckFSError_NotFixable)
						{
							MessageBoxA((HWND)_pWinMain, "The file system contained errors that is not fixable.", "Check Virtual FS", MB_OK);
						}
						else if (Error == CVirtualFS::ECheckFSError_None)
						{
							MessageBoxA((HWND)_pWinMain, "The file system contains no errors.", "Check Virtual FS", MB_OK);
						}
					}
					else
						break;
				}

				g_VfsWfx.m_FileSystems.f_Insert(pFileSystem);

				if (bChanged)
					g_VfsWfx.f_SaveSettings();
			}
		}
	}

	if (bChanged)
	{
		_RemoteName = _RemoteName.f_Left(_RemoteName.f_FindCharReverse('/') + 1);
		return EExecuteError_SymLink;
	}
	return EExecuteError_OK;
}

int CVfsWfx::f_RenMoveFile(const ch16 *_pOldName, const ch16 *_pNewName, bool _bMove, bool _bOverWrite)
{
	return 0;
}

int CVfsWfx::f_GetFile(const ch16 *_pRemoteName, const ch16 *_pLocalName, int _CopyFlags, RemoteInfoStruct *_pRemoteInfo)
{
	CStr FsPath;
	CFileSystem *pFileSys = fp_GetFS(fg_StrFromWindows(_pRemoteName), FsPath);

	if (!(_CopyFlags & FS_COPYFLAGS_OVERWRITE))
	{
		if (NFile::CFile::fs_FileExists(fg_StrFromWindows(_pLocalName)))
			return FS_FILE_EXISTS;
	}

	if (pFileSys)
	{
		try
		{
			pFileSys->OpenFs();
			{
				NFile::CFile LocalFile;
				LocalFile.f_Open(fg_StrFromWindows(_pLocalName), EFileOpen_Write | EFileOpen_NoLocalCache | EFileOpen_ShareAll);

				CVirtualFSFile RemoteFile;
				pFileSys->m_pVirtualFS->f_OpenFile(RemoteFile, FsPath, EFileOpen_Read);

				enum
				{
					EBufferSize = 65536
				};
				uint8 Buffer[EBufferSize];
				uint64 Size = RemoteFile.f_GetLength();
				uint64 Progress = 0;
				uint64 ToCopy = Size;

				while (ToCopy)
				{
					uint64 ThisTime = fg_Min(ToCopy, (uint64)EBufferSize);
					RemoteFile.f_Read(Buffer, ThisTime);
					LocalFile.f_Write(Buffer, ThisTime);
					ToCopy -= ThisTime;
					Progress += ThisTime;
					m_ProgressProc(m_PluginNumber, (ch16 *)_pRemoteName, (ch16 *)_pLocalName, (Progress*100)/Size);
				}

				_pRemoteInfo->Attr = MalterlibFileAttribToWin32FileAttrib(pFileSys->m_pVirtualFS->f_GetAttributes(FsPath));
				_pRemoteInfo->LastWriteTime = MalterlibTimeToFileTime(pFileSys->m_pVirtualFS->f_GetWriteTime(FsPath));
				_pRemoteInfo->SizeHigh = Size >> 32;
				_pRemoteInfo->SizeLow = Size & 0xffffffffui64;
				LocalFile.f_SetWriteTime(pFileSys->m_pVirtualFS->f_GetWriteTime(FsPath));
				LocalFile.f_SetCreationTime(pFileSys->m_pVirtualFS->f_GetCreationTime(FsPath));
				LocalFile.f_SetAttributes((NFile::EFileAttrib)pFileSys->m_pVirtualFS->f_GetAttributes(FsPath));
			}

			if (_CopyFlags & FS_COPYFLAGS_MOVE)
				pFileSys->m_pVirtualFS->f_DeleteFile(FsPath);

			return FS_FILE_OK;
		}
		catch (CException)
		{
		}
	}

	return FS_FILE_NOTFOUND;
}

int CVfsWfx::f_PutFile(const ch16 *_pLocalName, const ch16 *_pRemoteName, int _CopyFlags)
{
	CStr FsPath;
	CFileSystem *pFileSys = fp_GetFS(fg_StrFromWindows(_pRemoteName), FsPath);

	if (pFileSys)
	{
		try
		{
			pFileSys->OpenFs();
			{
				if (!(_CopyFlags & FS_COPYFLAGS_OVERWRITE))
				{
					if (pFileSys->m_pVirtualFS->f_FileExists(FsPath))
						return FS_FILE_EXISTS;
				}

				NFile::CFile LocalFile;
				LocalFile.f_Open(fg_StrFromWindows(_pLocalName), EFileOpen_Read | EFileOpen_ShareAll);

				{
					CVirtualFSFile RemoteFile;
					pFileSys->m_pVirtualFS->f_OpenFile(RemoteFile, FsPath, EFileOpen_Write);

					enum
					{
						EBufferSize = 65536
					};
					uint8 Buffer[EBufferSize];
					uint64 Size = LocalFile.f_GetLength();
					uint64 Progress = 0;
					uint64 ToCopy = Size;

					while (ToCopy)
					{
						uint64 ThisTime = fg_Min(ToCopy, (uint64)EBufferSize);
						LocalFile.f_Read(Buffer, ThisTime);
						RemoteFile.f_Write(Buffer, ThisTime);
						ToCopy -= ThisTime;
						Progress += ThisTime;
						m_ProgressProc(m_PluginNumber, (ch16 *)_pRemoteName, (ch16 *)_pLocalName, (Progress*100)/Size);
					}
				}
				pFileSys->m_pVirtualFS->f_SetWriteTime(FsPath, LocalFile.f_GetWriteTime());
				pFileSys->m_pVirtualFS->f_SetCreationTime(FsPath, LocalFile.f_GetCreationTime());
				pFileSys->m_pVirtualFS->f_SetAttributes(FsPath, LocalFile.f_GetAttributes());
			}

			if (_CopyFlags & FS_COPYFLAGS_MOVE)
				NFile::CFile::fs_DeleteFile(fg_StrFromWindows(_pLocalName));

			return FS_FILE_OK;
		}
		catch (CException)
		{
		}
	}

	return FS_FILE_NOTFOUND;
}

int CVfsWfx::f_DeleteFile(const ch16 *_pRemoteName)
{
	CStr FsPath;
	CFileSystem *pFileSys = fp_GetFS(fg_StrFromWindows(_pRemoteName), FsPath);

	if (pFileSys)
	{
		try
		{
			pFileSys->OpenFs();
			{
				pFileSys->m_pVirtualFS->f_DeleteFile(FsPath);
			}
			return true;
		}
		catch (CException)
		{
		}
	}

	return false;
}

int CVfsWfx::f_SetAttribs(const ch16 *_pRemoteName, uint32 _Attrib)
{
	return 0;
}

int CVfsWfx::f_SetTimeCreation(const ch16 *_pRemoteName, CTime _Time)
{
	return 0;
}

int CVfsWfx::f_SetTimeAccess(const ch16 *_pRemoteName, CTime _Time)
{
	return 0;
}

int CVfsWfx::f_SetTimeWrite(const ch16 *_pRemoteName, CTime _Time)
{
	return 0;
}


