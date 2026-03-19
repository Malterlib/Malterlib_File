// Copyright © 2015 Hansoft AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "PCH.h"
#define DMibAllowCodeStandardViolations 1

#ifndef NTDDI_VERSION
#if defined(_M_X64)
#define _WIN32_WINNT _WIN32_WINNT_WS03
#define NTDDI_VERSION NTDDI_WS03
#else
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#define NTDDI_VERSION NTDDI_WINXPSP2
#endif
#include <sdkddkver.h>
#endif

#include "windows.h"
#include "Proj/Msvc/resource.h"

HINSTANCE g_Instance = nullptr;
BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		g_Instance = (HINSTANCE)hModule;
	}
    return TRUE;
}

INT_PTR CALLBACK CreateFileSystemProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CEditFileSystemClass *pThis = (CEditFileSystemClass *)(umint)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
			case SC_CLOSE:

                EndDialog(hwndDlg, 0);
				return 1;
			};
		}
		break;
	case WM_INITDIALOG:
		{
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (umint)lParam);
			pThis = (CEditFileSystemClass *)(umint)lParam;
			POINT Pnt;
			GetCursorPos(&Pnt);
			HMONITOR hMon = MonitorFromPoint(Pnt, MONITOR_DEFAULTTONEAREST);
			MONITORINFO MonInfo;
			fg_MemClear(MonInfo);
			MonInfo.cbSize = sizeof(MonInfo);
			GetMonitorInfo(hMon, &MonInfo);

			RECT Rect;
			GetWindowRect(hwndDlg, &Rect);

			int Width = Rect.right - Rect.left;
			int Height = Rect.bottom - Rect.top;
			int MonWidth = MonInfo.rcWork.right - MonInfo.rcWork.left;
			int MonHeight = MonInfo.rcWork.bottom - MonInfo.rcWork.top;

			SetWindowTextA(hwndDlg, pThis->m_Heading);

			HWND Temp = GetDlgItem(hwndDlg, IDC_EditFileName);
			SetWindowTextA(Temp, pThis->m_FilePath);
			Temp = GetDlgItem(hwndDlg, IDC_EditVolumeName);
			SetWindowTextA(Temp, pThis->m_VolumeName);
			Temp = GetDlgItem(hwndDlg, IDC_EditClusterSize);
			SetWindowTextA(Temp, CStr::fs_ToStr(fg_Abs(pThis->m_ClusterSize)));
			if (pThis->m_ClusterSize < 0)
				EnableWindow(Temp, false);
			Temp = GetDlgItem(hwndDlg, IDC_EditInitialSize);
			SetWindowTextA(Temp, CStr::fs_ToStr(fg_Abs(pThis->m_InitialSize)));
			if (pThis->m_InitialSize < 0)
				EnableWindow(Temp, false);
			Temp = GetDlgItem(hwndDlg, IDC_EditGrowSize);
			SetWindowTextA(Temp, CStr::fs_ToStr(fg_Abs(pThis->m_GrowSize)));
			if (pThis->m_GrowSize < 0)
				EnableWindow(Temp, false);

			SetWindowPos(hwndDlg, nullptr, MonInfo.rcWork.left + MonWidth / 2 - Width / 2, MonInfo.rcWork.top + MonHeight / 2 - Height / 2, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
			return 1;
		}
		break;
	case WM_COMMAND:
		{
			int Control = LOWORD(wParam);
			int Message = HIWORD(wParam);
//			HWND Window = (HWND)lParam;
			if (Message == BN_CLICKED)
			{
				if (Control == IDC_CHECKFS)
				{
					EndDialog(hwndDlg, 2);
					return 1;
				}
				else if (Control == IDC_ButtonCancel)
				{
					EndDialog(hwndDlg, 0);
					return 1;
				}
				else if (Control == IDC_ButtonOk)
				{
					CStr TempStr;
					HWND Temp = GetDlgItem(hwndDlg, IDC_EditFileName);
					GetWindowTextA(Temp, TempStr.f_GetStr(1024), 1024); pThis->m_FilePath = TempStr;
					Temp = GetDlgItem(hwndDlg, IDC_EditVolumeName);
					GetWindowTextA(Temp, TempStr.f_GetStr(1024), 1024); pThis->m_VolumeName = TempStr;
					Temp = GetDlgItem(hwndDlg, IDC_EditClusterSize);
					GetWindowTextA(Temp, TempStr.f_GetStr(1024), 1024); pThis->m_ClusterSize = fg_StrToInt(TempStr.f_GetStr(), 16384);
					Temp = GetDlgItem(hwndDlg, IDC_EditInitialSize);
					GetWindowTextA(Temp, TempStr.f_GetStr(1024), 1024); pThis->m_InitialSize = fg_StrToInt(TempStr.f_GetStr(), 1024*1024);
					Temp = GetDlgItem(hwndDlg, IDC_EditGrowSize);
					GetWindowTextA(Temp, TempStr.f_GetStr(1024), 1024); pThis->m_GrowSize = fg_StrToInt(TempStr.f_GetStr(), 1024*1024);
					if (!pThis->m_FilePath)
					{
						MessageBoxA(hwndDlg, "You have to choose the file where you\nwant the file system to be stored.", "Error", MB_OK | MB_ICONERROR);
						return 1;
					}
					if (!pThis->m_VolumeName)
					{
						MessageBoxA(hwndDlg, "You have to name the volumed.", "Error", MB_OK | MB_ICONERROR);
						return 1;
					}
					EndDialog(hwndDlg, 1);
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
					OpenFileName.hwndOwner = hwndDlg;

					bool bRet = GetSaveFileNameA(&OpenFileName) != 0;
					if (bRet)
					{
						HWND Temp = GetDlgItem(hwndDlg, IDC_EditFileName);
						SetWindowTextA(Temp, FileName);
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
	int bRet = DialogBoxParamA(g_Instance, MAKEINTRESOURCEA(IDD_EDITFS), (HWND)_Params.m_pParentWindow, CreateFileSystemProc, (LPARAM)&_Params);

	return bRet;
}

void fg_ErrorBox(void *_pParent, const ch8 *_pMessage)
{
	MessageBoxA((HWND)_pParent, _pMessage, "Error", MB_OK | MB_ICONERROR);
}
