// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "PCH.h"

#include <windows.h>
#include "fsplugin.h"
#include "Proj/Msvc/resource.h"

#define DMibAllowCodeStandardViolations 1

static CStr fg_StrFromWindows(const CWStr &_Str)
{
	return _Str;
}

static CWStr fg_StrToWindows(const CStr &_Str)
{
	return _Str;
}

int __stdcall FsInit(int PluginNr,tProgressProc pProgressProc, tLogProc pLogProc,tRequestProc pRequestProc)
{
	return 0;
}

HANDLE __stdcall FsFindFirst(char* Path,WIN32_FIND_DATA *FindData)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsFindNext(HANDLE Hdl,WIN32_FIND_DATA *FindData)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsMkDir(char* Path)
{
	DMibPDebugBreak;
	return 0;
}

int __stdcall FsExecuteFile(HWND MainWin,char* RemoteName,char* Verb)
{
	DMibPDebugBreak;
	return 0;
}

int __stdcall FsRenMovFile(char* OldName,char* NewName,BOOL Move, BOOL OverWrite,RemoteInfoStruct* ri)
{
	DMibPDebugBreak;
	return 0;
}

int __stdcall FsGetFile(char* RemoteName,char* LocalName,int CopyFlags, RemoteInfoStruct* ri)
{
	DMibPDebugBreak;
	return 0;
}

int __stdcall FsPutFile(char* LocalName,char* RemoteName,int CopyFlags)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsDeleteFile(char* RemoteName)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsRemoveDir(char* RemoteName)
{
	DMibPDebugBreak;
	return 0;
}


BOOL __stdcall FsSetAttr(char* RemoteName,int NewAttr)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsSetTime(char* RemoteName,FILETIME *CreationTime,FILETIME *LastAccessTime,FILETIME *LastWriteTime)
{
	DMibPDebugBreak;
	return 0;
}


void __stdcall FsStatusInfo(char* RemoteDir,int InfoStartEnd,int InfoOperation)
{
	DMibPDebugBreak;
	return;
}


int __stdcall FsExtractCustomIcon(char* RemoteName,int ExtractFlags,HICON* TheIcon)
{
	DMibPDebugBreak;
	return 0;
}



int __stdcall FsInitW(int PluginNr,tProgressProcW pProgressProcW, tLogProcW pLogProcW,tRequestProcW pRequestProcW)
{
	return int(g_VfsWfx.f_Init(pProgressProcW, pLogProcW, pRequestProcW, PluginNr));
}

HANDLE __stdcall FsFindFirstW(WCHAR* _pPath,WIN32_FIND_DATAW *FindData)
{
	CWStr Path = _pPath;
	fg_StrReplaceChar(Path, '\\', '/');
	if (Path.f_GetLen() > 1)
	{
		if (Path[Path.f_GetLen()-1] == '/')
			Path = Path.f_Left(Path.f_GetLen()-1);
	}
	void *pInstance = g_VfsWfx.f_FindFirstFile(Path);
	if (!pInstance)
		return INVALID_HANDLE_VALUE;

	if (!FsFindNextW(pInstance, FindData))
	{
		FsFindClose(pInstance);
		return INVALID_HANDLE_VALUE;
	}

	return pInstance;
}

uint32 MalterlibFileAttribToWin32FileAttrib(uint32 _Attrib)
{
	uint32 Ret = 0;
	if (_Attrib & EFileAttrib_Directory)
		Ret |= FILE_ATTRIBUTE_DIRECTORY;
	if (_Attrib & EFileAttrib_Hidden)
		Ret |= FILE_ATTRIBUTE_HIDDEN;
	if (_Attrib & EFileAttrib_Link)
		Ret |= FILE_ATTRIBUTE_REPARSE_POINT;
	if (_Attrib & EFileAttrib_ReadOnly)
		Ret |= FILE_ATTRIBUTE_READONLY;
	if (_Attrib & EFileAttrib_System)
		Ret |= FILE_ATTRIBUTE_SYSTEM;

	return Ret;
}

uint32 Win32FileAttribToMalterlibFileAttrib(uint32 _Attrib)
{
	uint32 Ret = 0;
	if (_Attrib & FILE_ATTRIBUTE_DIRECTORY)
		Ret |= EFileAttrib_Directory;
	if (_Attrib & FILE_ATTRIBUTE_HIDDEN)
		Ret |= EFileAttrib_Hidden;
	if (_Attrib & FILE_ATTRIBUTE_REPARSE_POINT)
		Ret |= EFileAttrib_Link;
	if (_Attrib & FILE_ATTRIBUTE_READONLY)
		Ret |= EFileAttrib_ReadOnly;
	if (_Attrib & FILE_ATTRIBUTE_SYSTEM)
		Ret |= EFileAttrib_System;

	return Ret;
}

FILETIME MalterlibTimeToFileTime(CTime _Time)
{

	CTimeConvert::CDateTime DateTime;
	CTimeConvert(_Time).f_ExtractDateTime(DateTime);

	SYSTEMTIME Time;
	Time.wYear = DateTime.m_Year;
	Time.wMonth = DateTime.m_Month;
	Time.wDay = DateTime.m_DayOfMonth;
	Time.wDayOfWeek = DateTime.m_DayOfWeek + 1;
	Time.wHour = DateTime.m_Hour;
	Time.wMinute = DateTime.m_Minute;
	Time.wSecond = DateTime.m_Second;
	Time.wMilliseconds = (DateTime.m_Fraction * 1000.0).f_ToInt();

	FILETIME Ret;
	SystemTimeToFileTime(&Time, &Ret);

	return Ret;
}

CTime FileTimeToMalterlibTime(FILETIME _Time)
{
	SYSTEMTIME Time;
	FileTimeToSystemTime(&_Time, &Time);

	return CTimeConvert::fs_CreateTime(Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds);
}

BOOL __stdcall FsFindNextW(HANDLE Hdl,WIN32_FIND_DATAW *_pFindData)
{
	CFindData FindData;
	memset(_pFindData, 0, sizeof(*_pFindData));
	int bRet = g_VfsWfx.f_FindNextFile((CFindInstance *)Hdl, FindData);
	fg_StrCopy(_pFindData->cFileName, fg_StrToWindows(FindData.m_FileName), MAX_PATH) ;
	fg_StrReplaceChar(_pFindData->cFileName, '/', '\\');
	fg_StrCopy(_pFindData->cAlternateFileName, _pFindData->cFileName, 13);
	_pFindData->dwFileAttributes = MalterlibFileAttribToWin32FileAttrib(FindData.m_FileAttributes);
	_pFindData->ftCreationTime = MalterlibTimeToFileTime(FindData.m_CreationTime);
	_pFindData->ftLastAccessTime = MalterlibTimeToFileTime(FindData.m_LastAccessTime);
	_pFindData->ftLastWriteTime = MalterlibTimeToFileTime(FindData.m_LastWriteTime);
	_pFindData->nFileSizeLow = FindData.m_FileSize & 0xffffffffi64;
	_pFindData->nFileSizeHigh = FindData.m_FileSize >> 32;

	if (!bRet)
	{
		SetLastError(ERROR_NO_MORE_FILES);
	}

	return bRet != 0;
}

int __stdcall FsFindClose(HANDLE Hdl)
{
	g_VfsWfx.f_FindClose((CFindInstance *)Hdl);
	return 0;
}

BOOL __stdcall FsMkDirW(WCHAR* _pPath)
{	
	CWStr Path = _pPath;
	fg_StrReplaceChar(Path, '\\', '/');
	return g_VfsWfx.f_CreateDirectory(Path);
}

int __stdcall FsExecuteFileW(HWND MainWin,WCHAR* _pRemoteName,WCHAR* Verb)
{
	CStr RemoteName = fg_StrFromWindows(_pRemoteName);
	fg_StrReplaceChar(RemoteName, '\\', '/');
	int Ret = g_VfsWfx.f_Execute(MainWin, RemoteName, fg_StrFromWindows(Verb));

	if (*_pRemoteName)
	{
		fg_StrCopy(_pRemoteName, fg_StrToWindows(RemoteName), MAX_PATH);
		fg_StrReplaceChar(_pRemoteName, '/', '\\');
	}
	return Ret;
}

int __stdcall FsRenMovFileW(WCHAR* _pOldName,WCHAR* _pNewName,BOOL Move, BOOL OverWrite,RemoteInfoStruct* ri)
{
	CWStr OldName = _pOldName;
	fg_StrReplaceChar(OldName, '\\', '/');
	CWStr NewName = _pNewName;
	fg_StrReplaceChar(NewName, '\\', '/');	
	return g_VfsWfx.f_RenMoveFile(OldName, NewName, Move, OverWrite);
}

int __stdcall FsGetFileW(WCHAR* _pRemoteName,WCHAR* _pLocalName,int CopyFlags,RemoteInfoStruct* ri)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');
	CWStr LocalName = _pLocalName;
	fg_StrReplaceChar(LocalName, '\\', '/');
	return g_VfsWfx.f_GetFile(RemoteName, LocalName, CopyFlags, ri);
}

int __stdcall FsPutFileW(WCHAR* _pLocalName,WCHAR* _pRemoteName,int CopyFlags)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');
	CWStr LocalName = _pLocalName;
	fg_StrReplaceChar(LocalName, '\\', '/');
	return g_VfsWfx.f_PutFile(LocalName, RemoteName, CopyFlags);
}

BOOL __stdcall FsDeleteFileW(WCHAR* _pRemoteName)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');
	return g_VfsWfx.f_DeleteFile(RemoteName);
}

BOOL __stdcall FsRemoveDirW(WCHAR* _pRemoteName)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');
	return g_VfsWfx.f_DeleteFile(RemoteName);
}

BOOL __stdcall FsSetAttrW(WCHAR* _pRemoteName,int NewAttr)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');
	return g_VfsWfx.f_SetAttribs(RemoteName, Win32FileAttribToMalterlibFileAttrib(NewAttr));
}

BOOL __stdcall FsSetTimeW(WCHAR* _pRemoteName,FILETIME *CreationTime, FILETIME *LastAccessTime,FILETIME *LastWriteTime)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');

	if (CreationTime)
		return g_VfsWfx.f_SetTimeCreation(RemoteName, FileTimeToMalterlibTime(*CreationTime));
	if (LastAccessTime)
		return g_VfsWfx.f_SetTimeAccess(RemoteName, FileTimeToMalterlibTime(*LastAccessTime));
	if (LastWriteTime)
		return g_VfsWfx.f_SetTimeWrite(RemoteName, FileTimeToMalterlibTime(*LastWriteTime));

	return true;
}

void __stdcall FsStatusInfoW(WCHAR* RemoteDir,int InfoStartEnd,int InfoOperation)
{
	// This function may be used to initialize variables and to flush buffers
	
/*	char text[MAX_PATH];

	if (InfoStartEnd==FS_STATUS_START)
		strcpy(text,"Start: ");
	else
		strcpy(text,"End: ");
	
	switch (InfoOperation) {
	case FS_STATUS_OP_LIST:
		strcat(text,"Get directory list");
		break;
	case FS_STATUS_OP_GET_SINGLE:
		strcat(text,"Get single file");
		break;
	case FS_STATUS_OP_GET_MULTI:
		strcat(text,"Get multiple files");
		break;
	case FS_STATUS_OP_PUT_SINGLE:
		strcat(text,"Put single file");
		break;
	case FS_STATUS_OP_PUT_MULTI:
		strcat(text,"Put multiple files");
		break;
	case FS_STATUS_OP_RENMOV_SINGLE:
		strcat(text,"Rename/Move/Remote copy single file");
		break;
	case FS_STATUS_OP_RENMOV_MULTI:
		strcat(text,"Rename/Move/Remote copy multiple files");
		break;
	case FS_STATUS_OP_DELETE:
		strcat(text,"Delete multiple files");
		break;
	case FS_STATUS_OP_ATTRIB:
		strcat(text,"Change attributes of multiple files");
		break;
	case FS_STATUS_OP_MKDIR:
		strcat(text,"Create directory");
		break;
	case FS_STATUS_OP_EXEC:
		strcat(text,"Execute file or command line");
		break;
	default:
		strcat(text,"Unknown operation");
	}
	if (InfoOperation != FS_STATUS_OP_LIST)   // avoid recursion due to re-reading!
		MessageBox(0,text,RemoteDir,0);
*/
}

void __stdcall FsGetDefRootName(char* DefRootName,int maxlen)
{
	fg_StrCopy(DefRootName,"Malterlib File System",maxlen);
}

void __stdcall FsSetDefaultParams(FsDefaultParamStruct* _pDps)
{
	CStr Path = _pDps->DefaultIniName;
	Path = Path.f_ReplaceChar('\\', '/');
	Path = Path.f_Left(Path.f_FindCharReverse('/'));
	g_VfsWfx.f_SetIniPath(Path);
}

extern HINSTANCE g_Instance;

int __stdcall FsExtractCustomIconW(WCHAR* _pRemoteName,int ExtractFlags,HICON* TheIcon)
{
	CWStr RemoteName = _pRemoteName;
	if (RemoteName == "\\Add file system")
	{
		*TheIcon = LoadIconA(g_Instance,  MAKEINTRESOURCEA(IDI_DRIVE));
		return EIconType_Exracted;
	}
	else if (RemoteName == "\\Create file system")
	{
		*TheIcon = LoadIconA(g_Instance,  MAKEINTRESOURCEA(IDI_CREATEDRIVE));
		return EIconType_Exracted;
	}
	else if (RemoteName == "\\Unload all file systems")
	{
		*TheIcon = LoadIconA(g_Instance,  MAKEINTRESOURCEA(IDI_CLOSEDRIVES));
		return EIconType_Exracted;
	}

	return EIconType_UseDefault;
}