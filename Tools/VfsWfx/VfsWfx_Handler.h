// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#include "windows.h"
#include "fsplugin.h"
#include <Mib/File/MalterlibFS>

#define DMibAllowCodeStandardViolations 1


enum EFileError
{
	EFileError_OK = 0,
	EFileError_Exists = 1,
	EFileError_NotFound = 2,
	EFileError_ReadError = 3,
	EFileError_WriteError = 4,
	EFileError_UserAbort = 5,
	EFileError_NotSupported = 6,
	EFileError_ExistsResumeAllowed = 7
};

enum EExecuteError
{
	EExecuteError_OK = 0,
	EExecuteError_Error = 1,
	EExecuteError_YourSelf = -1,
	EExecuteError_SymLink = -2
};

enum ECopyFlags
{
	ECopyFlags_OverWrite = 1,
	ECopyFlags_Resume = 2,
	ECopyFlags_Move = 4,
	ECopyFlags_Exists_SameCase = 8,
	ECopyFlags_Exists_DifferentCase = 16
};
 
// flags for tRequestProc
enum ERequest
{
	ERequest_Other = 0,
	ERequest_UserName = 1,
	ERequest_Password = 2,
	ERequest_Account = 3,
	ERequest_UserNameFirewall = 4,
	ERequest_PasswordFirewall = 5,
	ERequest_TargetDir = 6,
	ERequest_URL = 7,
	ERequest_MsgOK = 8,
	ERequest_MsgYesNo = 9,
	ERequest_MsgOKCancel = 10
};

// flags for tLogProc
enum ELog
{
	ELog_Connect = 1,
	ELog_Disconnect = 2,
	ELog_Details = 3,
	ELog_TransferComplete = 4,
	ELog_ConnectComplete = 5,
	ELog_ImportantError = 6,
	ELog_OperationComplete = 7
};

// flags for FsStatusInfo
enum EStatus
{
	EStatus_Start = 0,
	EStatus_End = 1
};

enum EStatusOp
{
	EStatusOp_List = 1,
	EStatusOp_GetSingle = 2,
	EStatusOp_GetMulti = 3,
	EStatusOp_PutSingle = 4,
	EStatusOp_PutMulti = 5,
	EStatusOp_RenMoveSingle = 6,
	EStatusOp_RenMoveMulti = 7,
	EStatusOp_Delete = 8,
	EStatusOp_Attrib = 9,
	EStatusOp_CreateDirectory = 10,
	EStatusOp_Exec = 11,
	EStatusOp_CalcSize = 12,
	EStatusOp_SEARCH = 13,
	EStatusOp_SEARCH_TEXT = 14,
	EStatusOp_SYNC_SEARCH = 15,
	EStatusOp_SYNC_GET = 16,
	EStatusOp_SYNC_PUT = 17,
	EStatusOp_SYNC_DELETE = 18
};

enum EIconFlag
{
	EIconFlag_Small = 1,
	EIconFlag_BackGround = 2
};

enum EIconType
{
	EIconType_UseDefault,
	EIconType_Exracted,
	EIconType_ExractedDestroy,
	EIconType_Delayed
};


// Total Commander Callbacks

typedef int (__stdcall *FProgressProc)(int PluginNr,ch16* SourceName,          ch16* TargetName,int PercentDone);
typedef void (__stdcall *FLogProc)(int PluginNr,int MsgType,ch16* LogString);
typedef bint (__stdcall *FRequestProc)(int PluginNr,int RequestType,ch16* CustomTitle, ch16* CustomText,ch16* ReturnedText,int maxlen);

class CFindData
{
public:

    uint32 m_FileAttributes;
    CTime m_CreationTime;
    CTime m_LastAccessTime;
    CTime m_LastWriteTime;
    uint64 m_FileSize;
    CStr m_FileName;
};

class CFindInstance
{
public:
	virtual ~CFindInstance()
	{
	}

	virtual int f_GetNext(CFindData &_Data) = 0;
};

class CFileSystem
{
public:

	CStr m_Name;
	CStr m_FileName;
	CVirtualFS *m_pVirtualFS;
	NFile::TCBinaryStreamFile<> *m_pFile;
	
	class CCompare
	{
	public:
		inline_small CStr const &operator () (CFileSystem const &_Node) const
		{
			return _Node.m_Name;
		}
		inline_small bint operator () (CStr const &_Left, CStr const &_Right) const
		{
			return fg_StrCmp(_Left, _Right) < 0;
		}
	};

	DMibIntrusiveLink(CFileSystem, NMib::NIntrusive::TCAVLLink<>, m_Link);

	CFileSystem()
	{
		m_pVirtualFS = nullptr;
		m_pFile = nullptr;
	}

	~CFileSystem()
	{
		CloseFs();
	}

	void f_Feed(CBinaryStream &_Stream) const
	{
		_Stream << m_Name;
		_Stream << m_FileName;
	}

	void f_Consume(CBinaryStream &_Stream)
	{
		_Stream >> m_Name;
		_Stream >> m_FileName;
	}

	void CloseFs()
	{
		if (m_pVirtualFS)
			delete m_pVirtualFS;
		m_pVirtualFS = nullptr;

		if (m_pFile)
			delete m_pFile;
		m_pFile = nullptr;
	}

	class CFSReporter : public CVirtualFS::CCheckReporter
	{
	public:
		virtual void f_ReportError(NStr::CStr _Error)
		{
			DDTrace("{}\n", _Error);
		}

	};

	CVirtualFS::ECheckFSError f_CheckFSFixNewFS()
	{
		CloseFs();

		if (!NFile::CFile::fs_FileExists(m_FileName))
			return CVirtualFS::ECheckFSError_NotFixable;

		m_pFile = DNew TCBinaryStreamFile<>;
		m_pFile->f_Open(m_FileName, EFileOpen_DontTruncate | EFileOpen_Write | EFileOpen_Read);
		CVirtualFS::ECheckFSError Ret;

		{
			NFile::TCBinaryStreamFile<> NewFile;
			NewFile.f_Open(m_FileName + ".fixfstmp", EFileOpen_Write | EFileOpen_Read);
			CVirtualFS NewFS;
			NewFS.f_Create(&NewFile, 16384, 1024*1024, 1024*1024);

			CFSReporter Reporter;

			Ret = CVirtualFS::fs_CheckFS(m_pFile, &Reporter, false, &NewFS);
		}

		CloseFs();

		if (Ret == CVirtualFS::ECheckFSError_None)
		{
			NFile::CFile::fs_DeleteFile(m_FileName);
			NFile::CFile::fs_RenameFile(m_FileName + ".fixfstmp", m_FileName);
		}

		return Ret;
	}

	CVirtualFS::ECheckFSError f_CheckFS()
	{
		CloseFs();

		if (!NFile::CFile::fs_FileExists(m_FileName))
			return CVirtualFS::ECheckFSError_NotFixable;

		m_pFile = DNew TCBinaryStreamFile<>;
		m_pFile->f_Open(m_FileName, EFileOpen_DontTruncate | EFileOpen_Write | EFileOpen_Read);

		CFSReporter Reporter;

		CVirtualFS::ECheckFSError Ret = CVirtualFS::fs_CheckFS(m_pFile, &Reporter, false, nullptr);

		CloseFs();
		return Ret;
	}


	CVirtualFS::ECheckFSError f_CheckFSFix()
	{
		CloseFs();

		if (!NFile::CFile::fs_FileExists(m_FileName))
			return CVirtualFS::ECheckFSError_NotFixable;

		m_pFile = DNew TCBinaryStreamFile<>;
		m_pFile->f_Open(m_FileName, EFileOpen_DontTruncate | EFileOpen_Write | EFileOpen_Read);

		CFSReporter Reporter;

		CVirtualFS::ECheckFSError Ret = CVirtualFS::fs_CheckFS(m_pFile, &Reporter, true, nullptr);

		CloseFs();
		return Ret;
	}


	bool OpenFs()
	{
		if (m_pVirtualFS)
			return true;
		if (!NFile::CFile::fs_FileExists(m_FileName))
			return false;

		try
		{
			m_pFile = DNew TCBinaryStreamFile<>;
			m_pFile->f_Open(m_FileName, EFileOpen_DontTruncate | EFileOpen_Write | EFileOpen_Read);
			m_pVirtualFS = DNew CVirtualFS();
			m_pVirtualFS->f_Open(m_pFile);
		}
		catch (CException)
		{
			CloseFs();
			return false;
		}
		return true;
	}

};

class CVfsWfx
{
public:

	NIntrusive::TCAVLTree<CFileSystem::CLinkTraits_m_Link, CFileSystem::CCompare> m_FileSystems;

	void fp_UnloadAll()
	{
		CFileSysIter Iter = m_FileSystems;

		while (Iter)
		{
			Iter->CloseFs();
			++Iter;
		}
	}

public:
	typedef NIntrusive::TCAVLTree<CFileSystem::CLinkTraits_m_Link, CFileSystem::CCompare>::CIterator CFileSysIter;

	FProgressProc m_ProgressProc;
	FLogProc m_LogProc;
	FRequestProc m_RequestProc;
	aint m_PluginNumber;

	CStr m_IniPath;

	CVfsWfx()
	{
		
	}

	~CVfsWfx()
	{
		m_FileSystems.f_DeleteAll();
	}

	CFileSystem *fp_GetFS(CStr _Path, CStr &_FsPath);

	aint f_Init(FProgressProc _ProgressProc, FLogProc _LogProc, FRequestProc _RequestProc, aint _PluginNumber);
	void f_SetIniPath(CStr _Path);
	void f_SaveSettings();
	void f_LoadSettings();

	CFindInstance *f_FindFirstFile(const ch16 *_pPath);
	bint f_FindNextFile(CFindInstance * _pInstance, CFindData &_FindData);
	void f_FindClose(CFindInstance * _pInstance);
	int f_CreateDirectory(const ch16 *_pDirectory);
	int f_Execute(void *_pWinMain, CStr &_RemoteName, CStr const &_Verb);
	int f_RenMoveFile(const ch16 *_pOldName, const ch16 *_pNewName, bint _bMove, bint _bOverWrite);
	int f_GetFile(const ch16 *_pRemoteName, const ch16 *_pLocalName, int _CopyFlags, RemoteInfoStruct* _RemoteInfo);
	int f_PutFile(const ch16 *_pLocalName, const ch16 *_pRemoteName, int _CopyFlags);
	int f_DeleteFile(const ch16 *_pRemoteName);
	int f_SetAttribs(const ch16 *_pRemoteName, uint32 _Attrib);
	int f_SetTimeCreation(const ch16 *_pRemoteName, CTime _Time);
	int f_SetTimeAccess(const ch16 *_pRemoteName, CTime _Time);
	int f_SetTimeWrite(const ch16 *_pRemoteName, CTime _Time);

};

extern CVfsWfx g_VfsWfx;

