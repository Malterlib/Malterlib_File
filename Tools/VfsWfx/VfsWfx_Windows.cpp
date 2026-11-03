// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "PCH.h"
#define DMibAllowCodeStandardViolations 1

#ifndef NTDDI_VERSION
#	if defined(_M_X64)
#		define _WIN32_WINNT _WIN32_WINNT_WS03
#		define NTDDI_VERSION NTDDI_WS03
#	else
#		define _WIN32_WINNT _WIN32_WINNT_WINXP
#		define NTDDI_VERSION NTDDI_WINXPSP2
#	endif
#	include <sdkddkver.h>
#endif

#include "windows.h"
#include "Proj/Msvc/resource.h"

HINSTANCE g_pInstance = nullptr;

// NOLINTNEXTLINE(malterlib-function-naming): the entry point that Windows calls in a DLL
BOOL APIENTRY DllMain(HANDLE _pModule, DWORD _Reason, LPVOID _pReserved)
{
	if (_Reason == DLL_PROCESS_ATTACH)
		g_pInstance = (HINSTANCE)_pModule;
	return TRUE;
}

INT_PTR CALLBACK fg_CreateFileSystemProc(HWND _pDialog, UINT _Message, WPARAM _WParam, LPARAM _LParam)
{
	CEditFileSystemClass *pThis = (CEditFileSystemClass *)(umint)GetWindowLongPtr(_pDialog, GWLP_USERDATA);
	switch (_Message)
	{
	case WM_SYSCOMMAND:
		{
			switch (_WParam)
			{
			case SC_CLOSE:
				EndDialog(_pDialog, 0);
				return 1;
			};
		}
		break;
	case WM_INITDIALOG:
		{
			SetWindowLongPtr(_pDialog, GWLP_USERDATA, (umint)_LParam);
			pThis = (CEditFileSystemClass *)(umint)_LParam;
			POINT Pnt;
			GetCursorPos(&Pnt);
			HMONITOR pMonitor = MonitorFromPoint(Pnt, MONITOR_DEFAULTTONEAREST);
			MONITORINFO MonInfo;
			fg_MemClear(MonInfo);
			MonInfo.cbSize = sizeof(MonInfo);
			GetMonitorInfo(pMonitor, &MonInfo);

			RECT Rect;
			GetWindowRect(_pDialog, &Rect);

			int Width = Rect.right - Rect.left;
			int Height = Rect.bottom - Rect.top;
			int MonWidth = MonInfo.rcWork.right - MonInfo.rcWork.left;
			int MonHeight = MonInfo.rcWork.bottom - MonInfo.rcWork.top;

			SetWindowTextA(_pDialog, pThis->m_Heading);

			HWND pTemp = GetDlgItem(_pDialog, IDC_EditFileName);
			SetWindowTextA(pTemp, pThis->m_FilePath);
			pTemp = GetDlgItem(_pDialog, IDC_EditVolumeName);
			SetWindowTextA(pTemp, pThis->m_VolumeName);
			pTemp = GetDlgItem(_pDialog, IDC_EditClusterSize);
			SetWindowTextA(pTemp, CStr::fs_ToStr(fg_Abs(pThis->m_ClusterSize)));
			if (pThis->m_ClusterSize < 0)
				EnableWindow(pTemp, false);
			pTemp = GetDlgItem(_pDialog, IDC_EditInitialSize);
			SetWindowTextA(pTemp, CStr::fs_ToStr(fg_Abs(pThis->m_InitialSize)));
			if (pThis->m_InitialSize < 0)
				EnableWindow(pTemp, false);
			pTemp = GetDlgItem(_pDialog, IDC_EditGrowSize);
			SetWindowTextA(pTemp, CStr::fs_ToStr(fg_Abs(pThis->m_GrowSize)));
			if (pThis->m_GrowSize < 0)
				EnableWindow(pTemp, false);

			SetWindowPos(_pDialog, nullptr, MonInfo.rcWork.left + MonWidth / 2 - Width / 2, MonInfo.rcWork.top + MonHeight / 2 - Height / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			return 1;
		}
		break;
	case WM_COMMAND:
		{
			int Control = LOWORD(_WParam);
			int Message = HIWORD(_WParam);
			// HWND Window = (HWND)lParam;
			if (Message == BN_CLICKED)
			{
				if (Control == IDC_CHECKFS)
				{
					EndDialog(_pDialog, 2);
					return 1;
				}
				else if (Control == IDC_ButtonCancel)
				{
					EndDialog(_pDialog, 0);
					return 1;
				}
				else if (Control == IDC_ButtonOk)
				{
					CStr TempStr;
					HWND pTemp = GetDlgItem(_pDialog, IDC_EditFileName);
					GetWindowTextA(pTemp, TempStr.f_GetStr(1024), 1024);
					pThis->m_FilePath = TempStr;
					pTemp = GetDlgItem(_pDialog, IDC_EditVolumeName);
					GetWindowTextA(pTemp, TempStr.f_GetStr(1024), 1024);
					pThis->m_VolumeName = TempStr;
					pTemp = GetDlgItem(_pDialog, IDC_EditClusterSize);
					GetWindowTextA(pTemp, TempStr.f_GetStr(1024), 1024);
					pThis->m_ClusterSize = fg_StrToInt(TempStr.f_GetStr(), 16384);
					pTemp = GetDlgItem(_pDialog, IDC_EditInitialSize);
					GetWindowTextA(pTemp, TempStr.f_GetStr(1024), 1024);
					pThis->m_InitialSize = fg_StrToInt(TempStr.f_GetStr(), 1024 * 1024);
					pTemp = GetDlgItem(_pDialog, IDC_EditGrowSize);
					GetWindowTextA(pTemp, TempStr.f_GetStr(1024), 1024);
					pThis->m_GrowSize = fg_StrToInt(TempStr.f_GetStr(), 1024 * 1024);
					if (!pThis->m_FilePath)
					{
						MessageBoxA(_pDialog, "You have to choose the file where you\nwant the file system to be stored.", "Error", MB_OK | MB_ICONERROR);
						return 1;
					}
					if (!pThis->m_VolumeName)
					{
						MessageBoxA(_pDialog, "You have to name the volumed.", "Error", MB_OK | MB_ICONERROR);
						return 1;
					}
					EndDialog(_pDialog, 1);
					return 1;
				}
				else if (Control == IDC_ButtonBrowse)
				{
					OPENFILENAMEA OpenFileName;
					fg_MemClear(&OpenFileName, sizeof(OpenFileName));
					OpenFileName.lStructSize = sizeof(OpenFileName);
					CStr FileName;
					OpenFileName.lpstrFile = FileName.f_GetStr(1024);
					OpenFileName.nMaxFile = 1024;
					OpenFileName.Flags = OFN_DONTADDTORECENT | (pThis->m_bCreateFile ? OFN_OVERWRITEPROMPT : 0);
					OpenFileName.hwndOwner = _pDialog;

					bool bRet = GetSaveFileNameA(&OpenFileName) != 0;
					if (bRet)
					{
						HWND pTemp = GetDlgItem(_pDialog, IDC_EditFileName);
						SetWindowTextA(pTemp, FileName);
					}
				}
			}
		}
		break;
	}

	return false;
}

aint fg_EditFileSystem(CEditFileSystemClass &_Params)
{
	int bRet = DialogBoxParamA(g_pInstance, MAKEINTRESOURCEA(IDD_EDITFS), (HWND)_Params.m_pParentWindow, fg_CreateFileSystemProc, (LPARAM)&_Params);

	return bRet;
}

void fg_ErrorBox(void *_pParent, ch8 const *_pMessage)
{
	MessageBoxA((HWND)_pParent, _pMessage, "Error", MB_OK | MB_ICONERROR);
}
