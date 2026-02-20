/** \file
 * \brief WinUI Driver - File Dialog
 *
 * Uses IFileDialog COM API (IFileOpenDialog/IFileSaveDialog).
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <shobjidl.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_dialog.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Dispatching;


static Ihandle* winui_filedlg_ih = NULL;
static HWND winui_filedlg_parent = NULL;
static HHOOK winui_filedlg_hook = NULL;

#define WINUI_FILEDLG_CENTER_TIMER_ID 0x4955

static void CALLBACK winuiFileDlgCenterTimerProc(HWND hwnd, UINT msg, UINT_PTR idTimer, DWORD dwTime)
{
  (void)msg;
  (void)dwTime;
  KillTimer(hwnd, idTimer);

  Ihandle* ih = winui_filedlg_ih;
  winui_filedlg_ih = NULL;
  if (ih)
  {
    ih->handle = (InativeHandle*)hwnd;
    iupDialogUpdatePosition(ih);
    ih->handle = NULL;
  }
}

static LRESULT CALLBACK winuiFileDlgCBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode == HCBT_ACTIVATE && winui_filedlg_ih)
  {
    HWND dlgHwnd = (HWND)wParam;
    if (dlgHwnd != winui_filedlg_parent)
    {
      void* dq_ptr = iupwinuiGetDispatcherQueue();
      if (dq_ptr)
      {
        Windows::Foundation::IInspectable dq_obj{ nullptr };
        copy_from_abi(dq_obj, dq_ptr);
        DispatcherQueue dq = dq_obj.as<DispatcherQueue>();

        Ihandle* ih = winui_filedlg_ih;
        dq.TryEnqueue([dlgHwnd, ih]()
        {
          if (ih)
          {
            ih->handle = (InativeHandle*)dlgHwnd;
            iupDialogUpdatePosition(ih);
            ih->handle = NULL;
          }
        });
      }
      else
      {
        SetTimer(dlgHwnd, WINUI_FILEDLG_CENTER_TIMER_ID, 0, winuiFileDlgCenterTimerProc);
      }

      UnhookWindowsHookEx(winui_filedlg_hook);
      winui_filedlg_hook = NULL;
    }
  }
  return CallNextHookEx(winui_filedlg_hook, nCode, wParam, lParam);
}

static void winuiFileDlgSetInitialDir(IFileDialog* fileDialog, const char* dir)
{
  if (!dir || !dir[0])
    return;

  std::wstring wdir = iupwinuiStringToWString(dir);
  for (auto& ch : wdir)
  {
    if (ch == L'/')
      ch = L'\\';
  }

  IShellItem* folderItem = NULL;
  HRESULT hr = SHCreateItemFromParsingName(wdir.c_str(), NULL, IID_PPV_ARGS(&folderItem));
  if (SUCCEEDED(hr) && folderItem)
  {
    fileDialog->SetFolder(folderItem);
    folderItem->Release();
  }
}

static void winuiFileDlgParseExtFilter(const char* extfilter, std::vector<std::wstring>& names, std::vector<std::vector<std::wstring>>& extensions)
{
  if (!extfilter)
    return;

  char* filters = _strdup(extfilter);
  char* p = filters;
  char* name = p;

  while (*p)
  {
    if (*p == '|')
    {
      *p = 0;
      p++;

      char* pattern = p;
      while (*p && *p != '|')
        p++;
      if (*p == '|')
      {
        *p = 0;
        p++;
      }

      std::wstring wname = iupwinuiStringToHString(name).c_str();
      names.push_back(wname);

      std::vector<std::wstring> exts;
      char* ext = pattern;
      while (*ext)
      {
        char* next = ext;
        while (*next && *next != ';')
          next++;
        bool last = (*next == 0);
        *next = 0;

        if (ext[0] == '*' && ext[1] == '.')
          ext += 1;
        else if (ext[0] == '*' && ext[1] == 0)
          ext = (char*)".*";

        std::wstring wext = iupwinuiStringToHString(ext).c_str();
        exts.push_back(wext);

        if (last)
          break;
        ext = next + 1;
      }
      extensions.push_back(exts);

      name = p;
    }
    else
      p++;
  }

  free(filters);
}

static void winuiFileDlgSetFilters(IFileDialog* fileDialog, const char* extfilter, const char* filter, const char* filterinfo)
{
  std::vector<std::wstring> filterNames;
  std::vector<std::vector<std::wstring>> filterExtensions;
  winuiFileDlgParseExtFilter(extfilter, filterNames, filterExtensions);

  std::vector<COMDLG_FILTERSPEC> filterSpecs;
  std::vector<std::wstring> patterns;

  if (filterNames.size() > 0)
  {
    for (size_t i = 0; i < filterNames.size(); i++)
    {
      std::wstring pattern;
      for (size_t j = 0; j < filterExtensions[i].size(); j++)
      {
        if (j > 0) pattern += L";";
        pattern += L"*";
        pattern += filterExtensions[i][j];
      }
      patterns.push_back(pattern);
    }

    for (size_t i = 0; i < filterNames.size(); i++)
    {
      COMDLG_FILTERSPEC spec;
      spec.pszName = filterNames[i].c_str();
      spec.pszSpec = patterns[i].c_str();
      filterSpecs.push_back(spec);
    }
  }
  else if (filter)
  {
    std::wstring wfilter = iupwinuiStringToWString(filter);
    std::wstring winfo;
    if (filterinfo)
      winfo = iupwinuiStringToWString(filterinfo);
    else
      winfo = wfilter;

    patterns.push_back(wfilter);
    filterNames.push_back(winfo);

    COMDLG_FILTERSPEC spec;
    spec.pszName = filterNames[0].c_str();
    spec.pszSpec = patterns[0].c_str();
    filterSpecs.push_back(spec);
  }

  if (filterSpecs.size() > 0)
    fileDialog->SetFileTypes((UINT)filterSpecs.size(), filterSpecs.data());
}

static char* winuiFileDlgGetPathFromItem(IShellItem* pItem)
{
  LPWSTR filePath = NULL;
  HRESULT hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
  if (SUCCEEDED(hr) && filePath)
  {
    char* result = iupwinuiHStringToString(winrt::hstring(filePath));
    CoTaskMemFree(filePath);
    return result;
  }
  return NULL;
}

static HRESULT winuiFileDlgShow(IFileDialog* pDialog, HWND parent, Ihandle* ih)
{
  winui_filedlg_ih = ih;
  winui_filedlg_parent = parent;
  winui_filedlg_hook = SetWindowsHookEx(WH_CBT, winuiFileDlgCBTProc, NULL, GetCurrentThreadId());

  HRESULT hr = pDialog->Show(parent);

  if (winui_filedlg_hook)
  {
    UnhookWindowsHookEx(winui_filedlg_hook);
    winui_filedlg_hook = NULL;
  }
  winui_filedlg_ih = NULL;

  iupwinuiProcessPendingMessages();

  return hr;
}

static int winuiFileDlgPopup(Ihandle* ih, int x, int y)
{
  char* dialogtype;
  HWND parent = (HWND)iupDialogGetNativeParent(ih);

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  if (!parent)
    parent = GetActiveWindow();

  iupAttribSet(ih, "NATIVEPARENT", (char*)parent);

  dialogtype = iupAttribGet(ih, "DIALOGTYPE");

  if (iupStrEqualNoCase(dialogtype, "DIR"))
  {
    IFileOpenDialog* pDialog = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pDialog);
    if (FAILED(hr) || !pDialog)
      return IUP_ERROR;

    DWORD options = 0;
    pDialog->GetOptions(&options);
    pDialog->SetOptions(options | FOS_PICKFOLDERS);

    char* initial_dir = iupAttribGet(ih, "DIRECTORY");
    if (initial_dir)
      winuiFileDlgSetInitialDir(pDialog, initial_dir);

    hr = winuiFileDlgShow(pDialog, parent, ih);
    if (SUCCEEDED(hr))
    {
      IShellItem* pItem = NULL;
      pDialog->GetResult(&pItem);
      if (pItem)
      {
        char* path = winuiFileDlgGetPathFromItem(pItem);
        if (path)
          iupAttribSetStr(ih, "VALUE", path);
        pItem->Release();
      }
      iupAttribSet(ih, "STATUS", "0");
    }
    else
    {
      iupAttribSet(ih, "VALUE", NULL);
      iupAttribSet(ih, "STATUS", "-1");
    }
    iupAttribSet(ih, "FILEEXIST", NULL);
    iupAttribSet(ih, "FILTERUSED", NULL);

    pDialog->Release();
  }
  else if (iupStrEqualNoCase(dialogtype, "SAVE"))
  {
    IFileSaveDialog* pDialog = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, (void**)&pDialog);
    if (FAILED(hr) || !pDialog)
      return IUP_ERROR;

    char* initial_dir = iupAttribGet(ih, "DIRECTORY");
    if (initial_dir)
      winuiFileDlgSetInitialDir(pDialog, initial_dir);

    char* value = iupAttribGet(ih, "FILE");
    if (value)
      pDialog->SetFileName(iupwinuiStringToWString(value).c_str());

    char* extfilter = iupAttribGet(ih, "EXTFILTER");
    char* filter = iupAttribGet(ih, "FILTER");
    char* filterinfo = iupAttribGet(ih, "FILTERINFO");
    char* extdefault = iupAttribGet(ih, "EXTDEFAULT");

    winuiFileDlgSetFilters(pDialog, extfilter, filter, filterinfo);

    if (extdefault)
      pDialog->SetDefaultExtension(iupwinuiStringToWString(extdefault).c_str());

    hr = winuiFileDlgShow(pDialog, parent, ih);
    if (SUCCEEDED(hr))
    {
      IShellItem* pItem = NULL;
      pDialog->GetResult(&pItem);
      if (pItem)
      {
        char* filename = winuiFileDlgGetPathFromItem(pItem);
        if (filename)
        {
          iupAttribSetStr(ih, "VALUE", filename);

          char* dir = iupStrFileGetPath(filename);
          iupAttribSetStr(ih, "DIRECTORY", dir);
          free(dir);
        }
        pItem->Release();
      }

      iupAttribSet(ih, "FILEEXIST", "NO");
      iupAttribSet(ih, "FILTERUSED", "1");
      iupAttribSet(ih, "STATUS", "0");
    }
    else
    {
      iupAttribSet(ih, "VALUE", NULL);
      iupAttribSet(ih, "DIRECTORY", NULL);
      iupAttribSet(ih, "FILTERUSED", NULL);
      iupAttribSet(ih, "STATUS", "-1");
      iupAttribSet(ih, "FILEEXIST", NULL);
    }

    pDialog->Release();
  }
  else
  {
    IFileOpenDialog* pDialog = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pDialog);
    if (FAILED(hr) || !pDialog)
      return IUP_ERROR;

    char* initial_dir = iupAttribGet(ih, "DIRECTORY");
    if (initial_dir)
      winuiFileDlgSetInitialDir(pDialog, initial_dir);

    char* extfilter = iupAttribGet(ih, "EXTFILTER");
    char* filter = iupAttribGet(ih, "FILTER");

    winuiFileDlgSetFilters(pDialog, extfilter, filter, NULL);

    if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
    {
      DWORD options = 0;
      pDialog->GetOptions(&options);
      pDialog->SetOptions(options | FOS_ALLOWMULTISELECT);

      hr = winuiFileDlgShow(pDialog, parent, ih);
      if (SUCCEEDED(hr))
      {
        IShellItemArray* pItems = NULL;
        pDialog->GetResults(&pItems);

        DWORD count = 0;
        if (pItems)
          pItems->GetCount(&count);

        if (count > 0)
        {
          IShellItem* pFirst = NULL;
          pItems->GetItemAt(0, &pFirst);
          char* firstPath = winuiFileDlgGetPathFromItem(pFirst);
          char* dir = iupStrFileGetPath(firstPath);
          int dir_len = (int)strlen(dir);
          iupAttribSetStr(ih, "DIRECTORY", dir);

          iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);

          int multivaluepath = iupAttribGetBoolean(ih, "MULTIVALUEPATH");
          int offset = multivaluepath ? 0 : dir_len;

          if (count == 1)
          {
            iupAttribSetStrId(ih, "MULTIVALUE", 1, firstPath + offset);
            iupAttribSetStr(ih, "VALUE", firstPath);
            iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);
          }
          else
          {
            int len = dir_len;
            if (dir[dir_len - 1] == '/' || dir[dir_len - 1] == '\\') len--;

            std::string allNames(dir, len);
            allNames += '|';

            int idx = 1;
            for (DWORD i = 0; i < count; i++)
            {
              IShellItem* pItem = NULL;
              pItems->GetItemAt(i, &pItem);
              char* filePath = winuiFileDlgGetPathFromItem(pItem);
              if (filePath)
              {
                iupAttribSetStrId(ih, "MULTIVALUE", idx, filePath + offset);
                allNames += (filePath + offset);
                allNames += '|';
                idx++;
              }
              if (pItem) pItem->Release();
            }

            iupAttribSetInt(ih, "MULTIVALUECOUNT", idx);
            iupAttribSetStr(ih, "VALUE", allNames.c_str());
          }

          free(dir);
          if (pFirst) pFirst->Release();

          iupAttribSet(ih, "FILEEXIST", "YES");
          iupAttribSet(ih, "FILTERUSED", "1");
          iupAttribSet(ih, "STATUS", "0");
        }
        else
        {
          iupAttribSet(ih, "VALUE", NULL);
          iupAttribSet(ih, "STATUS", "-1");
          iupAttribSet(ih, "FILEEXIST", NULL);
          iupAttribSet(ih, "FILTERUSED", NULL);
          iupAttribSet(ih, "DIRECTORY", NULL);
        }

        if (pItems) pItems->Release();
      }
      else
      {
        iupAttribSet(ih, "VALUE", NULL);
        iupAttribSet(ih, "STATUS", "-1");
        iupAttribSet(ih, "FILEEXIST", NULL);
        iupAttribSet(ih, "FILTERUSED", NULL);
        iupAttribSet(ih, "DIRECTORY", NULL);
      }
    }
    else
    {
      hr = winuiFileDlgShow(pDialog, parent, ih);
      if (SUCCEEDED(hr))
      {
        IShellItem* pItem = NULL;
        pDialog->GetResult(&pItem);
        if (pItem)
        {
          char* filename = winuiFileDlgGetPathFromItem(pItem);
          if (filename)
          {
            iupAttribSetStr(ih, "VALUE", filename);

            char* dir = iupStrFileGetPath(filename);
            iupAttribSetStr(ih, "DIRECTORY", dir);
            free(dir);
          }
          pItem->Release();
        }

        iupAttribSet(ih, "FILEEXIST", "YES");
        iupAttribSet(ih, "FILTERUSED", "1");
        iupAttribSet(ih, "STATUS", "0");
      }
      else
      {
        iupAttribSet(ih, "VALUE", NULL);
        iupAttribSet(ih, "DIRECTORY", NULL);
        iupAttribSet(ih, "FILTERUSED", NULL);
        iupAttribSet(ih, "STATUS", "-1");
        iupAttribSet(ih, "FILEEXIST", NULL);
      }
    }

    pDialog->Release();
  }

  iupAttribSet(ih, "NATIVEPARENT", NULL);

  return IUP_NOERROR;
}

extern "C" void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winuiFileDlgPopup;
}
