/** \file
 * \brief WinUI Driver - File Dialog
 *
 * Uses IFileDialog COM API (IFileOpenDialog/IFileSaveDialog)
 * with IFileDialogEvents for FILE_CB callback support.
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
#include "iupcbs.h"
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


#define WINUI_FILEDLG_HELP_BTN    150

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

static int winuiFileDlgFileExists(const char* filename)
{
  std::wstring wname = iupwinuiStringToWString(filename);
  DWORD attrib = GetFileAttributesW(wname.c_str());
  if (attrib == INVALID_FILE_ATTRIBUTES)
    return 0;
  if (attrib & FILE_ATTRIBUTE_DIRECTORY)
    return 0;
  return 1;
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


/******************************************************************************
 * IFileDialogEvents + IFileDialogControlEvents
 ******************************************************************************/

class winuiFileDlgEventHandler : public IFileDialogEvents, public IFileDialogControlEvents
{
public:
  IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
  {
    if (riid == IID_IUnknown || riid == IID_IFileDialogEvents)
    {
      *ppv = static_cast<IFileDialogEvents*>(this);
      AddRef();
      return S_OK;
    }
    else if (riid == IID_IFileDialogControlEvents)
    {
      *ppv = static_cast<IFileDialogControlEvents*>(this);
      AddRef();
      return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
  }

  IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_cRef); }
  IFACEMETHODIMP_(ULONG) Release()
  {
    long cRef = InterlockedDecrement(&_cRef);
    if (!cRef) delete this;
    return cRef;
  }

  IFACEMETHODIMP OnFileOk(IFileDialog* pfd);
  IFACEMETHODIMP OnSelectionChange(IFileDialog* pfd);
  IFACEMETHODIMP OnTypeChange(IFileDialog* pfd);
  IFACEMETHODIMP OnFolderChange(IFileDialog*) { return S_OK; }
  IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; }
  IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; }
  IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; }

  IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD dwIDCtl);
  IFACEMETHODIMP OnItemSelected(IFileDialogCustomize*, DWORD, DWORD) { return S_OK; }
  IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL) { return S_OK; }
  IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) { return S_OK; }

  winuiFileDlgEventHandler(Ihandle* _ih) : _cRef(1), ih(_ih) {}

private:
  ~winuiFileDlgEventHandler() {}
  Ihandle* ih;
  long _cRef;
};

IFACEMETHODIMP winuiFileDlgEventHandler::OnFileOk(IFileDialog* pfd)
{
  IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (!cb)
    return S_OK;

  char* filename = NULL;

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
  {
    IFileOpenDialog* pfod = NULL;
    HRESULT hr = pfd->QueryInterface(IID_PPV_ARGS(&pfod));
    if (SUCCEEDED(hr))
    {
      IShellItemArray* pItems = NULL;
      hr = pfod->GetResults(&pItems);
      if (SUCCEEDED(hr) && pItems)
      {
        IShellItem* psi = NULL;
        pItems->GetItemAt(0, &psi);
        if (psi)
        {
          filename = winuiFileDlgGetPathFromItem(psi);
          psi->Release();
        }
        pItems->Release();
      }
      pfod->Release();
    }
  }
  else
  {
    IShellItem* psi = NULL;
    HRESULT hr = pfd->GetResult(&psi);
    if (SUCCEEDED(hr) && psi)
    {
      filename = winuiFileDlgGetPathFromItem(psi);
      psi->Release();
    }
  }

  if (!filename)
    return S_OK;

  int ret = cb(ih, filename, (char*)"OK");
  if (ret == IUP_IGNORE || ret == IUP_CONTINUE)
  {
    if (ret == IUP_CONTINUE)
    {
      char* value = iupAttribGet(ih, "FILE");
      if (value)
        pfd->SetFileName(iupwinuiStringToWString(value).c_str());
    }
    return S_FALSE;
  }

  return S_OK;
}

IFACEMETHODIMP winuiFileDlgEventHandler::OnSelectionChange(IFileDialog* pfd)
{
  IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (!cb)
    return S_OK;

  IShellItem* psi = NULL;
  HRESULT hr = pfd->GetCurrentSelection(&psi);
  if (FAILED(hr) || !psi)
    return S_OK;

  char* filename = NULL;
  char* status = (char*)"SELECT";

  SFGAOF attr;
  hr = psi->GetAttributes(SFGAO_FILESYSTEM | SFGAO_FOLDER, &attr);
  if (SUCCEEDED(hr) && (attr & SFGAO_FILESYSTEM))
  {
    filename = winuiFileDlgGetPathFromItem(psi);
    if (attr & SFGAO_FOLDER)
      status = (char*)"OTHER";
  }
  psi->Release();

  if (!filename)
    return S_OK;

  int ret = cb(ih, filename, status);
  if (ret == IUP_IGNORE || ret == IUP_CONTINUE)
    return S_FALSE;

  return S_OK;
}

IFACEMETHODIMP winuiFileDlgEventHandler::OnTypeChange(IFileDialog* pfd)
{
  IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (!cb)
    return S_OK;

  IShellItem* folder = NULL;
  HRESULT hr = pfd->GetFolder(&folder);
  if (FAILED(hr) || !folder)
    return S_OK;

  char* pathname = winuiFileDlgGetPathFromItem(folder);
  folder->Release();
  if (!pathname || pathname[0] == 0)
    return S_OK;

  LPWSTR pszFileName = NULL;
  hr = pfd->GetFileName(&pszFileName);
  if (FAILED(hr) || !pszFileName)
    return S_OK;

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
  {
    /* the returned filename may contain more than one name in quotes */
    LPWSTR p = pszFileName;
    int found = 0;
    while (*p)
    {
      if (*p == L'"')
      {
        if (!found)
          found = 1;
        else
        {
          *(p - 1) = 0;
          break;
        }
      }
      if (found)
        *p = *(p + 1);
      p++;
    }
  }

  char* name = iupwinuiHStringToString(winrt::hstring(pszFileName));
  CoTaskMemFree(pszFileName);
  if (!name || name[0] == 0)
    return S_OK;

  UINT index;
  pfd->GetFileTypeIndex(&index);
  iupAttribSetInt(ih, "FILTERUSED", index);

  int pathlen = (int)strlen(pathname);
  int namelen = (int)strlen(name);
  char* buffer = (char*)malloc(pathlen + namelen + 2);
  strcpy(buffer, pathname);
  strcat(buffer, "\\");
  strcat(buffer, name);

  int ret = cb(ih, buffer, (char*)"FILTER");
  free(buffer);

  if (ret == IUP_CONTINUE)
  {
    char* value = iupAttribGet(ih, "FILE");
    if (value)
      pfd->SetFileName(iupwinuiStringToWString(value).c_str());
  }

  return S_OK;
}

IFACEMETHODIMP winuiFileDlgEventHandler::OnButtonClicked(IFileDialogCustomize*, DWORD dwIDCtl)
{
  if (dwIDCtl == WINUI_FILEDLG_HELP_BTN)
  {
    IFn cb = (IFn)IupGetCallback(ih, "HELP_CB");
    if (cb)
      cb(ih);
  }

  return S_OK;
}


/******************************************************************************
 * Dialog setup helpers
 ******************************************************************************/

static void winuiFileDlgSetFileAndDir(IFileDialog* pfd, Ihandle* ih)
{
  char* value = iupAttribGet(ih, "FILE");
  char* directory = iupStrDup(iupAttribGet(ih, "DIRECTORY"));

  if (value)
  {
    char name[4096] = "";
    char dir[4096] = "";

    iupStrFileNameSplit(value, dir, name);

    if (name[0] != 0)
      pfd->SetFileName(iupwinuiStringToWString(name).c_str());
    else
      pfd->SetFileName(iupwinuiStringToWString(value).c_str());

    if (dir[0] != 0)
      winuiFileDlgSetInitialDir(pfd, dir);
    else if (directory)
      winuiFileDlgSetInitialDir(pfd, directory);
  }
  else if (directory)
  {
    winuiFileDlgSetInitialDir(pfd, directory);
  }

  if (directory)
    free(directory);
}


/******************************************************************************
 * Popup
 ******************************************************************************/

static int winuiFileDlgPopup(Ihandle* ih, int x, int y)
{
  char* value;
  HWND parent = (HWND)iupDialogGetNativeParent(ih);

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  if (!parent)
    parent = GetActiveWindow();

  iupAttribSet(ih, "NATIVEPARENT", (char*)parent);

  char* dialogtype = iupAttribGet(ih, "DIALOGTYPE");

  if (iupStrEqualNoCase(dialogtype, "DIR"))
  {
    IFileOpenDialog* pDialog = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pDialog);
    if (FAILED(hr) || !pDialog)
      return IUP_ERROR;

    DWORD options = 0;
    pDialog->GetOptions(&options);
    options |= FOS_PICKFOLDERS;

    if (iupAttribGetBoolean(ih, "NOCHANGEDIR"))
      options |= FOS_NOCHANGEDIR;

    if (iupAttribGetBoolean(ih, "SHOWHIDDEN"))
      options |= FOS_FORCESHOWHIDDEN;

    pDialog->SetOptions(options);

    value = iupAttribGet(ih, "TITLE");
    if (value)
      pDialog->SetTitle(iupwinuiStringToWString(value).c_str());

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
  else
  {
    int isSave = iupStrEqualNoCase(dialogtype, "SAVE");

    IFileOpenDialog* opfd = NULL;
    IFileSaveDialog* spfd = NULL;
    IFileDialog* pfd = NULL;
    HRESULT hr;

    if (isSave)
    {
      hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&spfd));
      if (SUCCEEDED(hr))
        hr = spfd->QueryInterface(IID_IFileDialog, reinterpret_cast<void**>(&pfd));
    }
    else
    {
      hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&opfd));
      if (SUCCEEDED(hr))
        hr = opfd->QueryInterface(IID_IFileDialog, reinterpret_cast<void**>(&pfd));
    }

    if (FAILED(hr) || !pfd)
    {
      if (pfd) pfd->Release();
      if (opfd) opfd->Release();
      if (spfd) spfd->Release();
      return IUP_ERROR;
    }

    /* event handler */
    winuiFileDlgEventHandler* pfde = new (std::nothrow) winuiFileDlgEventHandler(ih);
    if (!pfde)
    {
      pfd->Release();
      if (opfd) opfd->Release();
      if (spfd) spfd->Release();
      return IUP_ERROR;
    }

    DWORD dwCookie = 0;
    hr = pfd->Advise(pfde, &dwCookie);
    if (FAILED(hr))
    {
      pfde->Release();
      pfd->Release();
      if (opfd) opfd->Release();
      if (spfd) spfd->Release();
      return IUP_ERROR;
    }

    /* flags */
    DWORD dwFlags;
    pfd->GetOptions(&dwFlags);

    value = iupAttribGet(ih, "ALLOWNEW");
    if (!value)
      value = isSave ? (char*)"YES" : (char*)"NO";
    if (iupStrBoolean(value))
      dwFlags |= FOS_CREATEPROMPT;
    else
      dwFlags |= FOS_FILEMUSTEXIST;

    if (iupAttribGetBoolean(ih, "NOCHANGEDIR"))
      dwFlags |= FOS_NOCHANGEDIR;

    if (iupAttribGetBoolean(ih, "MULTIPLEFILES") && !isSave)
      dwFlags |= FOS_ALLOWMULTISELECT;

    if (!iupAttribGetBoolean(ih, "NOOVERWRITEPROMPT"))
      dwFlags |= FOS_OVERWRITEPROMPT;

    if (iupAttribGetBoolean(ih, "SHOWHIDDEN"))
      dwFlags |= FOS_FORCESHOWHIDDEN;

    if (iupAttribGetBoolean(ih, "SHOWPREVIEW"))
      dwFlags |= FOS_FORCEPREVIEWPANEON;

    pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);

    /* default extension */
    value = iupAttribGet(ih, "EXTDEFAULT");
    if (value)
      pfd->SetDefaultExtension(iupwinuiStringToWString(value).c_str());

    /* filters */
    char* extfilter = iupAttribGet(ih, "EXTFILTER");
    char* filter = iupAttribGet(ih, "FILTER");
    char* filterinfo = iupAttribGet(ih, "FILTERINFO");
    winuiFileDlgSetFilters(pfd, extfilter, filter, filterinfo);

    /* initial filter index */
    int filterIndex;
    value = iupAttribGet(ih, "FILTERUSED");
    if (iupStrToInt(value, &filterIndex))
      pfd->SetFileTypeIndex(filterIndex);

    /* initial directory and file */
    winuiFileDlgSetFileAndDir(pfd, ih);

    /* title */
    value = iupAttribGet(ih, "TITLE");
    if (value)
      pfd->SetTitle(iupwinuiStringToWString(value).c_str());

    /* help button */
    if (IupGetCallback(ih, "HELP_CB"))
    {
      IFileDialogCustomize* pfdc = NULL;
      hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdc));
      if (SUCCEEDED(hr))
      {
        pfdc->AddPushButton(WINUI_FILEDLG_HELP_BTN,
                            iupwinuiStringToWString(IupGetLanguageString("IUP_HELP")).c_str());
        pfdc->Release();
      }
    }

    /* INIT callback */
    IFnss file_cb = (IFnss)IupGetCallback(ih, "FILE_CB");
    if (file_cb)
      file_cb(ih, NULL, (char*)"INIT");

    /* show */
    hr = winuiFileDlgShow(pfd, parent, ih);

    /* FINISH callback */
    if (file_cb)
      file_cb(ih, NULL, (char*)"FINISH");

    if (SUCCEEDED(hr))
    {
      if (iupAttribGetBoolean(ih, "MULTIPLEFILES") && opfd)
      {
        IShellItemArray* pItems = NULL;
        opfd->GetResults(&pItems);

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
        IShellItem* pItem = NULL;
        pfd->GetResult(&pItem);
        if (pItem)
        {
          char* filename = winuiFileDlgGetPathFromItem(pItem);
          if (filename)
          {
            char* dir = iupStrFileGetPath(filename);
            int dir_len = (int)strlen(dir);
            iupAttribSetStr(ih, "DIRECTORY", dir);

            iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);
            free(dir);

            if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
              dir_len = 0;

            iupAttribSetStrId(ih, "MULTIVALUE", 1, filename + dir_len);
            iupAttribSetStr(ih, "VALUE", filename);
            iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);

            if (winuiFileDlgFileExists(filename))
            {
              iupAttribSet(ih, "FILEEXIST", "YES");
              iupAttribSet(ih, "STATUS", "0");
            }
            else
            {
              iupAttribSet(ih, "FILEEXIST", "NO");
              iupAttribSet(ih, "STATUS", "1");
            }
          }
          pItem->Release();
        }
      }

      UINT index;
      pfd->GetFileTypeIndex(&index);
      iupAttribSetInt(ih, "FILTERUSED", index);
    }
    else
    {
      iupAttribSet(ih, "FILTERUSED", NULL);
      iupAttribSet(ih, "VALUE", NULL);
      iupAttribSet(ih, "DIRECTORY", NULL);
      iupAttribSet(ih, "FILEEXIST", NULL);
      iupAttribSet(ih, "STATUS", "-1");
    }

    pfd->Unadvise(dwCookie);
    pfde->Release();

    pfd->Release();
    if (opfd) opfd->Release();
    if (spfd) spfd->Release();
  }

  iupAttribSet(ih, "NATIVEPARENT", NULL);

  return IUP_NOERROR;
}

extern "C" void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winuiFileDlgPopup;

  iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PREVIEWDC", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PREVIEWWIDTH", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PREVIEWHEIGHT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
