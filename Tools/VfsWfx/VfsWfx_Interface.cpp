// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "PCH.h"

#include <windows.h>
#include "fsplugin.h"
#include "Proj/Msvc/resource.h"

#define DMibAllowCodeStandardViolations 1

static CStr fg_StrFromWindows(CWStr const &_Str)
{
	return _Str;
}

static CWStr fg_StrToWindows(CStr const &_Str)
{
	return _Str;
}

int __stdcall FsInit(int _PluginNumber, tProgressProc _fProgressProc, tLogProc _fLogProc, tRequestProc _fRequestProc)
{
	return 0;
}

HANDLE __stdcall FsFindFirst(char *_pPath, WIN32_FIND_DATA *o_pFindData)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsFindNext(HANDLE _pHandle, WIN32_FIND_DATA *o_pFindData)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsMkDir(char *_pPath)
{
	DMibPDebugBreak;
	return 0;
}

int __stdcall FsExecuteFile(HWND _pMainWindow, char *_pRemoteName, char *_pVerb)
{
	DMibPDebugBreak;
	return 0;
}

int __stdcall FsRenMovFile(char *_pOldName, char *_pNewName, BOOL _bMove, BOOL _bOverWrite, RemoteInfoStruct *_pRemoteInfo)
{
	DMibPDebugBreak;
	return 0;
}

int __stdcall FsGetFile(char *_pRemoteName, char *_pLocalName, int _CopyFlags, RemoteInfoStruct *_pRemoteInfo)
{
	DMibPDebugBreak;
	return 0;
}

int __stdcall FsPutFile(char *_pLocalName, char *_pRemoteName, int _CopyFlags)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsDeleteFile(char *_pRemoteName)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsRemoveDir(char *_pRemoteName)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsSetAttr(char *_pRemoteName, int _NewAttributes)
{
	DMibPDebugBreak;
	return 0;
}

BOOL __stdcall FsSetTime(char *_pRemoteName, FILETIME *_pCreationTime, FILETIME *_pLastAccessTime, FILETIME *_pLastWriteTime)
{
	DMibPDebugBreak;
	return 0;
}

void __stdcall FsStatusInfo(char *_pRemoteDir, int _InfoStartEnd, int _InfoOperation)
{
	DMibPDebugBreak;
	return;
}

int __stdcall FsExtractCustomIcon(char *_pRemoteName, int _ExtractFlags, HICON *o_pIcon)
{
	DMibPDebugBreak;
	return 0;
}

int __stdcall FsInitW(int _PluginNumber, tProgressProcW _fProgressProc, tLogProcW _fLogProc, tRequestProcW _fRequestProc)
{
	return int(g_VfsWfx.f_Init(_fProgressProc, _fLogProc, _fRequestProc, _PluginNumber));
}

HANDLE __stdcall FsFindFirstW(WCHAR *_pPath, WIN32_FIND_DATAW *o_pFindData)
{
	CWStr Path = _pPath;
	fg_StrReplaceChar(Path, '\\', '/');
	if (Path.f_GetLen() > 1)
	{
		if (Path[Path.f_GetLen() - 1] == '/')
			Path = Path.f_Left(Path.f_GetLen() - 1);
	}
	void *pInstance = g_VfsWfx.f_FindFirstFile(Path);
	if (!pInstance)
		return INVALID_HANDLE_VALUE;

	if (!FsFindNextW(pInstance, o_pFindData))
	{
		FsFindClose(pInstance);
		return INVALID_HANDLE_VALUE;
	}

	return pInstance;
}

uint32 fg_MalterlibFileAttribToWin32FileAttrib(uint32 _Attrib)
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

uint32 fg_Win32FileAttribToMalterlibFileAttrib(uint32 _Attrib)
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

FILETIME fg_MalterlibTimeToFileTime(CTime _Time)
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

CTime fg_FileTimeToMalterlibTime(FILETIME _Time)
{
	SYSTEMTIME Time;
	FileTimeToSystemTime(&_Time, &Time);

	return CTimeConvert::fs_CreateTime(Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds);
}

BOOL __stdcall FsFindNextW(HANDLE _pHandle, WIN32_FIND_DATAW *_pFindData)
{
	CFindData FindData;
	memset(_pFindData, 0, sizeof(*_pFindData));
	int bRet = g_VfsWfx.f_FindNextFile((CFindInstance *)_pHandle, FindData);
	fg_StrCopy(_pFindData->cFileName, fg_StrToWindows(FindData.m_FileName), MAX_PATH);
	fg_StrReplaceChar(_pFindData->cFileName, '/', '\\');
	fg_StrCopy(_pFindData->cAlternateFileName, _pFindData->cFileName, 13);
	_pFindData->dwFileAttributes = fg_MalterlibFileAttribToWin32FileAttrib(FindData.m_FileAttributes);
	_pFindData->ftCreationTime = fg_MalterlibTimeToFileTime(FindData.m_CreationTime);
	_pFindData->ftLastAccessTime = fg_MalterlibTimeToFileTime(FindData.m_LastAccessTime);
	_pFindData->ftLastWriteTime = fg_MalterlibTimeToFileTime(FindData.m_LastWriteTime);
	_pFindData->nFileSizeLow = FindData.m_FileSize & 0xffffffffi64;
	_pFindData->nFileSizeHigh = FindData.m_FileSize >> 32;

	if (!bRet)
		SetLastError(ERROR_NO_MORE_FILES);

	return bRet != 0;
}

int __stdcall FsFindClose(HANDLE _pHandle)
{
	g_VfsWfx.f_FindClose((CFindInstance *)_pHandle);
	return 0;
}

BOOL __stdcall FsMkDirW(WCHAR *_pPath)
{
	CWStr Path = _pPath;
	fg_StrReplaceChar(Path, '\\', '/');
	return g_VfsWfx.f_CreateDirectory(Path);
}

int __stdcall FsExecuteFileW(HWND _pMainWindow, WCHAR *_pRemoteName, WCHAR *_pVerb)
{
	CStr RemoteName = fg_StrFromWindows(_pRemoteName);
	fg_StrReplaceChar(RemoteName, '\\', '/');
	int Ret = g_VfsWfx.f_Execute(_pMainWindow, RemoteName, fg_StrFromWindows(_pVerb));

	if (*_pRemoteName)
	{
		fg_StrCopy(_pRemoteName, fg_StrToWindows(RemoteName), MAX_PATH);
		fg_StrReplaceChar(_pRemoteName, '/', '\\');
	}
	return Ret;
}

int __stdcall FsRenMovFileW(WCHAR *_pOldName, WCHAR *_pNewName, BOOL _bMove, BOOL _bOverWrite, RemoteInfoStruct *_pRemoteInfo)
{
	CWStr OldName = _pOldName;
	fg_StrReplaceChar(OldName, '\\', '/');
	CWStr NewName = _pNewName;
	fg_StrReplaceChar(NewName, '\\', '/');
	return g_VfsWfx.f_RenMoveFile(OldName, NewName, _bMove, _bOverWrite);
}

int __stdcall FsGetFileW(WCHAR *_pRemoteName, WCHAR *_pLocalName, int _CopyFlags, RemoteInfoStruct *_pRemoteInfo)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');
	CWStr LocalName = _pLocalName;
	fg_StrReplaceChar(LocalName, '\\', '/');
	return g_VfsWfx.f_GetFile(RemoteName, LocalName, _CopyFlags, _pRemoteInfo);
}

int __stdcall FsPutFileW(WCHAR *_pLocalName, WCHAR *_pRemoteName, int _CopyFlags)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');
	CWStr LocalName = _pLocalName;
	fg_StrReplaceChar(LocalName, '\\', '/');
	return g_VfsWfx.f_PutFile(LocalName, RemoteName, _CopyFlags);
}

BOOL __stdcall FsDeleteFileW(WCHAR *_pRemoteName)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');
	return g_VfsWfx.f_DeleteFile(RemoteName);
}

BOOL __stdcall FsRemoveDirW(WCHAR *_pRemoteName)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');
	return g_VfsWfx.f_DeleteFile(RemoteName);
}

BOOL __stdcall FsSetAttrW(WCHAR *_pRemoteName, int _NewAttributes)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');
	return g_VfsWfx.f_SetAttribs(RemoteName, fg_Win32FileAttribToMalterlibFileAttrib(_NewAttributes));
}

BOOL __stdcall FsSetTimeW(WCHAR *_pRemoteName, FILETIME *_pCreationTime, FILETIME *_pLastAccessTime, FILETIME *_pLastWriteTime)
{
	CWStr RemoteName = _pRemoteName;
	fg_StrReplaceChar(RemoteName, '\\', '/');

	if (_pCreationTime)
		return g_VfsWfx.f_SetTimeCreation(RemoteName, fg_FileTimeToMalterlibTime(*_pCreationTime));
	if (_pLastAccessTime)
		return g_VfsWfx.f_SetTimeAccess(RemoteName, fg_FileTimeToMalterlibTime(*_pLastAccessTime));
	if (_pLastWriteTime)
		return g_VfsWfx.f_SetTimeWrite(RemoteName, fg_FileTimeToMalterlibTime(*_pLastWriteTime));

	return true;
}

void __stdcall FsStatusInfoW(WCHAR *_pRemoteDir, int _InfoStartEnd, int _InfoOperation)
{
	// This function may be used to initialize variables and to flush buffers

	/*
	char text[MAX_PATH];

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

void __stdcall FsGetDefRootName(char *o_pDefRootName, int _MaxLength)
{
	fg_StrCopy(o_pDefRootName, "Malterlib File System", _MaxLength);
}

void __stdcall FsSetDefaultParams(FsDefaultParamStruct *_pDps)
{
	CStr Path = _pDps->DefaultIniName;
	Path = Path.f_ReplaceChar('\\', '/');
	Path = Path.f_Left(Path.f_FindCharReverse('/'));
	g_VfsWfx.f_SetIniPath(Path);
}

extern HINSTANCE g_pInstance;

int __stdcall FsExtractCustomIconW(WCHAR *_pRemoteName, int _ExtractFlags, HICON *o_pIcon)
{
	CWStr RemoteName = _pRemoteName;
	if (RemoteName == "\\Add file system")
	{
		*o_pIcon = LoadIconA(g_pInstance, MAKEINTRESOURCEA(IDI_DRIVE));
		return EIconType_Exracted;
	}
	else if (RemoteName == "\\Create file system")
	{
		*o_pIcon = LoadIconA(g_pInstance, MAKEINTRESOURCEA(IDI_CREATEDRIVE));
		return EIconType_Exracted;
	}
	else if (RemoteName == "\\Unload all file systems")
	{
		*o_pIcon = LoadIconA(g_pInstance, MAKEINTRESOURCEA(IDI_CLOSEDRIVES));
		return EIconType_Exracted;
	}

	return EIconType_UseDefault;
}
