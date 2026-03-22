/** \file
 * \brief Windows Single Instance Support
 *
 * This implementation is standalone and can be used with any IUP backend
 * on Windows (native Win32, GTK, Qt, EFL).
 *
 * Uses a named mutex for instance detection and WM_COPYDATA via a hidden
 * message-only window for inter-instance communication.
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dlglist.h"

#include "iup_singleinstance.h"

#ifndef LOCALE_INVARIANT
#define LOCALE_INVARIANT 0x007f
#endif

static HANDLE si_mutex = NULL;
static HWND si_hwnd = NULL;
static WCHAR* si_nameW = NULL;

static WCHAR* iupWinSIStrToWide(const char* str)
{
  int len;
  WCHAR* wstr;

  if (!str)
    return NULL;

  len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
  if (len == 0)
    return NULL;

  wstr = (WCHAR*)malloc(len * sizeof(WCHAR));
  if (!wstr)
    return NULL;

  MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len);
  return wstr;
}

static char* iupWinSIStrFromWide(const WCHAR* wstr)
{
  int len;
  char* str;

  if (!wstr)
    return NULL;

  len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  if (len == 0)
    return NULL;

  str = (char*)malloc(len);
  if (!str)
    return NULL;

  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
  return str;
}

static void iupWinSIDeliverData(const char* data, int len)
{
  Ihandle* dlg;

  for (dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
  {
    IFnsi cb = (IFnsi)IupGetCallback(dlg, "COPYDATA_CB");
    if (cb)
    {
      cb(dlg, (char*)data, len);
      return;
    }
  }
}

static LRESULT CALLBACK iupWinSIWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  if (msg == WM_COPYDATA)
  {
    COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lp;
    char* iup_id = (char*)cds->dwData;

    if (iup_id && iup_id[0] == 'I' && iup_id[1] == 'U' && iup_id[2] == 'P')
    {
      char* data = iupWinSIStrFromWide((WCHAR*)cds->lpData);
      if (data)
      {
        iupWinSIDeliverData(data, (int)strlen(data) + 1);
        free(data);
      }
    }
    return 0;
  }

  return DefWindowProc(hwnd, msg, wp, lp);
}

static HWND iupWinSIFindWindow(const WCHAR* nameW)
{
  return FindWindowW(L"IupSingleInstanceWindow", nameW);
}

static void iupWinSISendToFirst(const WCHAR* nameW)
{
  HWND hWnd = iupWinSIFindWindow(nameW);
  if (hWnd)
  {
    LPWSTR cmdLine = GetCommandLineW();
    int len;

    SetForegroundWindow(hWnd);

    len = lstrlenW(cmdLine);
    if (len != 0)
    {
      COPYDATASTRUCT cds;
      cds.dwData = (ULONG_PTR)"IUP_DATA";
      cds.cbData = (len + 1) * sizeof(WCHAR);
      cds.lpData = cmdLine;
      SendMessage(hWnd, WM_COPYDATA, 0, (LPARAM)&cds);
    }
  }
}

static int iupWinSICreateWindow(const WCHAR* nameW)
{
  WNDCLASSW wc;
  HINSTANCE hInstance = GetModuleHandle(NULL);

  ZeroMemory(&wc, sizeof(wc));
  wc.lpfnWndProc = iupWinSIWndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = L"IupSingleInstanceWindow";
  RegisterClassW(&wc);

  si_hwnd = CreateWindowW(L"IupSingleInstanceWindow", nameW, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

  return (si_hwnd != NULL);
}

int iupdrvSingleInstanceSet(const char* name)
{
  WCHAR* nameW;

  if (!name || !name[0])
    return 0;

  nameW = iupWinSIStrToWide(name);
  if (!nameW)
    return 0;

  if (si_mutex)
    ReleaseMutex(si_mutex);

  si_mutex = CreateMutexW(NULL, FALSE, nameW);

  if (si_mutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
  {
    CloseHandle(si_mutex);
    si_mutex = NULL;

    iupWinSISendToFirst(nameW);
    free(nameW);
    return 1;
  }

  if (si_mutex == NULL)
  {
    free(nameW);
    return 0;
  }

  si_nameW = nameW;
  iupWinSICreateWindow(nameW);

  return 0;
}

void iupdrvSingleInstanceClose(void)
{
  if (si_hwnd)
  {
    DestroyWindow(si_hwnd);
    si_hwnd = NULL;
  }

  if (si_mutex)
  {
    ReleaseMutex(si_mutex);
    CloseHandle(si_mutex);
    si_mutex = NULL;
  }

  if (si_nameW)
  {
    free(si_nameW);
    si_nameW = NULL;
  }
}
