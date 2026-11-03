// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once

#include "windows.h"
#include "fsplugin.h"
#include <Mib/File/MalterlibFS>

#define DMibAllowCodeStandardViolations 1

enum EFileError
{
	EFileError_OK                    = 0
	, EFileError_Exists              = 1
	, EFileError_NotFound            = 2
	, EFileError_ReadError           = 3
	, EFileError_WriteError          = 4
	, EFileError_UserAbort           = 5
	, EFileError_NotSupported        = 6
	, EFileError_ExistsResumeAllowed = 7
};

enum EExecuteError
{
	EExecuteError_OK         = 0
	, EExecuteError_Error    = 1
	, EExecuteError_YourSelf = -1
	, EExecuteError_SymLink  = -2
};

enum ECopyFlags
{
	ECopyFlags_OverWrite              = 1
	, ECopyFlags_Resume               = 2
	, ECopyFlags_Move                 = 4
	, ECopyFlags_Exists_SameCase      = 8
	, ECopyFlags_Exists_DifferentCase = 16
};

// flags for tRequestProc
enum ERequest
{
	ERequest_Other              = 0
	, ERequest_UserName         = 1
	, ERequest_Password         = 2
	, ERequest_Account          = 3
	, ERequest_UserNameFirewall = 4
	, ERequest_PasswordFirewall = 5
	, ERequest_TargetDir        = 6
	, ERequest_URL              = 7
	, ERequest_MsgOK            = 8
	, ERequest_MsgYesNo         = 9
	, ERequest_MsgOKCancel      = 10
};

// flags for tLogProc
enum ELog
{
	ELog_Connect             = 1
	, ELog_Disconnect        = 2
	, ELog_Details           = 3
	, ELog_TransferComplete  = 4
	, ELog_ConnectComplete   = 5
	, ELog_ImportantError    = 6
	, ELog_OperationComplete = 7
};

// flags for FsStatusInfo
enum EStatus
{
	EStatus_Start = 0
	, EStatus_End = 1
};

enum EStatusOp
{
	EStatusOp_List              = 1
	, EStatusOp_GetSingle       = 2
	, EStatusOp_GetMulti        = 3
	, EStatusOp_PutSingle       = 4
	, EStatusOp_PutMulti        = 5
	, EStatusOp_RenMoveSingle   = 6
	, EStatusOp_RenMoveMulti    = 7
	, EStatusOp_Delete          = 8
	, EStatusOp_Attrib          = 9
	, EStatusOp_CreateDirectory = 10
	, EStatusOp_Exec            = 11
	, EStatusOp_CalcSize        = 12
	, EStatusOp_Search          = 13
	, EStatusOp_SearchText      = 14
	, EStatusOp_SyncSearch      = 15
	, EStatusOp_SyncGet         = 16
	, EStatusOp_SyncPut         = 17
	, EStatusOp_SyncDelete      = 18
};

enum EIconFlag
{
	EIconFlag_Small        = 1
	, EIconFlag_BackGround = 2
};

enum EIconType
{
	EIconType_UseDefault
	, EIconType_Exracted
	, EIconType_ExractedDestroy
	, EIconType_Delayed
};

// Total Commander Callbacks

typedef int (__stdcall *FProgressProc)(int _PluginNumber, ch16 *_pSourceName, ch16 *_pTargetName, int _PercentDone);
typedef void (__stdcall *FLogProc)(int _PluginNumber, int _MessageType, ch16 *_pLogString);
typedef BOOL (__stdcall *FRequestProc)(int _PluginNumber, int _RequestType, ch16 *_pCustomTitle, ch16 *_pCustomText, ch16 *_pReturnedText, int _MaxLength);

struct CFindData
{
	uint32 m_FileAttributes;
	CTime m_CreationTime;
	CTime m_LastAccessTime;
	CTime m_LastWriteTime;
	uint64 m_FileSize;
	CStr m_FileName;
};

struct CFindInstance
{
	virtual ~CFindInstance();

	virtual int f_GetNext(CFindData &_Data) = 0;
};

struct CFileSystem
{
	CStr m_Name;
	CStr m_FileName;
	CVirtualFS *m_pVirtualFS;
	NFile::TCBinaryStreamFile<> *m_pFile;

	struct CCompare
	{
		inline_small CStr const &operator () (CFileSystem const &_Node) const;
		inline_small bool operator () (CStr const &_Left, CStr const &_Right) const;
	};

	NMib::NIntrusive::TCAVLLink<> m_Link;

	CFileSystem();
	~CFileSystem();

	void f_Feed(CBinaryStream &_Stream) const;
	void f_Consume(CBinaryStream &_Stream);

	void f_CloseFs();

	struct CFSReporter : public CVirtualFS::CCheckReporter
	{
		virtual void f_ReportError(NStr::CStr const &_Error);
	};

	CVirtualFS::ECheckFSError f_CheckFSFixNewFS();
	CVirtualFS::ECheckFSError f_CheckFS();
	CVirtualFS::ECheckFSError f_CheckFSFix();

	bool f_OpenFs();
};

struct CVfsWfx
{
	typedef NIntrusive::TCAVLTree<&CFileSystem::m_Link, CFileSystem::CCompare>::CIterator CFileSysIter;

	NIntrusive::TCAVLTree<&CFileSystem::m_Link, CFileSystem::CCompare> m_FileSystems;

	FProgressProc m_fProgressProc;
	FLogProc m_fLogProc;
	FRequestProc m_fRequestProc;
	aint m_PluginNumber;

	CStr m_IniPath;

	CVfsWfx();
	~CVfsWfx();

	aint f_Init(FProgressProc _fProgressProc, FLogProc _fLogProc, FRequestProc _fRequestProc, aint _PluginNumber);
	void f_SetIniPath(CStr _Path);
	void f_SaveSettings();
	void f_LoadSettings();

	CFindInstance *f_FindFirstFile(ch16 const *_pPath);
	bool f_FindNextFile(CFindInstance *_pInstance, CFindData &_FindData);
	void f_FindClose(CFindInstance *_pInstance);
	int f_CreateDirectory(ch16 const *_pDirectory);
	int f_Execute(void *_pWinMain, CStr &_RemoteName, CStr const &_Verb);
	int f_RenMoveFile(ch16 const *_pOldName, ch16 const *_pNewName, bool _bMove, bool _bOverWrite);
	int f_GetFile(ch16 const *_pRemoteName, ch16 const *_pLocalName, int _CopyFlags, RemoteInfoStruct *_pRemoteInfo);
	int f_PutFile(ch16 const *_pLocalName, ch16 const *_pRemoteName, int _CopyFlags);
	int f_DeleteFile(ch16 const *_pRemoteName);
	int f_SetAttribs(ch16 const *_pRemoteName, uint32 _Attrib);
	int f_SetTimeCreation(ch16 const *_pRemoteName, CTime _Time);
	int f_SetTimeAccess(ch16 const *_pRemoteName, CTime _Time);
	int f_SetTimeWrite(ch16 const *_pRemoteName, CTime _Time);

private:
	void fp_UnloadAll();
	CFileSystem *fp_GetFS(CStr _Path, CStr &_FsPath);
};

extern CVfsWfx g_VfsWfx;

#include "VfsWfx_Handler.hpp"

