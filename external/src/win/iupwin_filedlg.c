/** \file
 * \brief IupFileDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drvinfo.h"
#include "iup_key.h"

#include "iupwin_drv.h"
#include "iupwin_str.h"


/* Not defined for Cygwin or MingW */
#ifndef OFN_FORCESHOWHIDDEN
#define OFN_FORCESHOWHIDDEN          0x10000000
#endif

#define IUP_MAX_FILENAME_SIZE 65000
#define IUP_PREVIEWCANVAS 3000

#define IUP_EDIT        0x0480


enum {IUP_DIALOGOPEN, IUP_DIALOGSAVE, IUP_DIALOGDIR};


static int winIsFile(const TCHAR* name)
{
  DWORD attrib = GetFileAttributes(name);
  if (attrib == INVALID_FILE_ATTRIBUTES)
    return 0;
  if (attrib & FILE_ATTRIBUTE_DIRECTORY)
    return 0;
  return 1;
}

static void winFileDlgStrReplacePathSlash(TCHAR* name)
{
  /* check for "/" */
  int i, len = lstrlen(name);
  for (i = 0; i < len; i++)
  {
    if (name[i] == TEXT('/'))
      name[i] = TEXT('\\');
  }
}

static INT CALLBACK winFileDlgBrowseCallback(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
  (void)lParam;
  if (uMsg == BFFM_INITIALIZED)
  {
    char* value;
    Ihandle* ih = (Ihandle*)lpData;

    ih->handle = hWnd;
    iupDialogUpdatePosition(ih);
    ih->handle = NULL;  /* reset handle */

    value = iupStrDup(iupAttribGet(ih, "DIRECTORY"));
    if (value)
    {
      TCHAR* wstr = iupwinStrToSystemFilename(value);
      winFileDlgStrReplacePathSlash(wstr);
      SendMessage(hWnd, BFFM_SETEXPANDED, TRUE, (LPARAM)wstr);
      SendMessage(hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)wstr);
      free(value);
    }
  }
  else if (uMsg == BFFM_SELCHANGED)
  {
    TCHAR buffer[IUP_MAX_FILENAME_SIZE];
    ITEMIDLIST* selecteditem = (ITEMIDLIST*)lParam;
    if (SHGetPathFromIDList(selecteditem, buffer) && buffer[0] != 0)
      SendMessage(hWnd, BFFM_ENABLEOK, 0, (LPARAM)TRUE);
    else
      SendMessage(hWnd, BFFM_ENABLEOK, 0, (LPARAM)FALSE);
  }
  return 0;
}

static void winFileDlgGetFolder(Ihandle *ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  BROWSEINFO browseinfo;
  TCHAR filename[MAX_PATH];
  LPITEMIDLIST selecteditem;

  /* if NOT set will NOT be Modal */
  /* anyway it will be modal only relative to its parent */
  if (!parent)
    parent = GetActiveWindow();

  ZeroMemory(&browseinfo, sizeof(BROWSEINFO));
  browseinfo.lpszTitle = iupwinStrToSystem(iupAttribGet(ih, "TITLE"));
  browseinfo.pszDisplayName = filename; 
  browseinfo.lpfn = winFileDlgBrowseCallback;
  browseinfo.lParam = (LPARAM)ih;
  browseinfo.ulFlags = IupGetGlobal("_IUPWIN_COINIT_MULTITHREADED")? 0: BIF_NEWDIALOGSTYLE;
  browseinfo.hwndOwner = parent;

  if (iupAttribGetBoolean(ih, "SHOWEDITBOX"))
    browseinfo.ulFlags |= BIF_EDITBOX;

  selecteditem = SHBrowseForFolder(&browseinfo);
  if (!selecteditem)
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "STATUS", "-1");
  }
  else
  {
    SHGetPathFromIDList(selecteditem, filename);
    iupAttribSetStr(ih, "VALUE", iupwinStrFromSystemFilename(filename));
    iupAttribSet(ih, "STATUS", "0");
  }

  iupAttribSet(ih, "FILEEXIST", NULL);
  iupAttribSet(ih, "FILTERUSED", NULL);
}

/************************************************************************************************/

static int winFileDlgGetSelectedFile(Ihandle* ih, HWND hWnd, TCHAR* filename)
{
  int ret = CommDlg_OpenSave_GetFilePath(GetParent(hWnd), filename, IUP_MAX_FILENAME_SIZE);
  if (ret < 0)
    return 0;

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
  {
    /* check if there are more than 1 files and return only the first one */
    int found = 0;
    while(*filename != 0)
    {            
      if (*filename == TEXT('"'))
      {
        if (!found)
          found = 1;
        else
        {
          *(filename-1) = 0;
          return 1;
        }
      }
      if (found)
        *filename = *(filename+1);
      filename++;
    }
  }

  return 1;
}

static int winFileDlgWmNotify(HWND hWnd, LPOFNOTIFY pofn)
{
  Ihandle* ih = (Ihandle*)pofn->lpOFN->lCustData;
  switch (pofn->hdr.code)
  {
  case CDN_INITDONE:
    {
      HWND hWndPreview;

      IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
      if (cb) cb(ih, NULL, "INIT");

      hWndPreview = GetDlgItem(hWnd, IUP_PREVIEWCANVAS);
      if (hWndPreview) 
        RedrawWindow(hWndPreview, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW);
      break;
    }
  case CDN_FILEOK:
  case CDN_SELCHANGE:
    {
      HWND hWndPreview;

      IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
      if (cb)
      {
        TCHAR filename[IUP_MAX_FILENAME_SIZE];
        if (winFileDlgGetSelectedFile(ih, hWnd, filename))
        {
          int ret;
          char* file_msg;

          if (pofn->hdr.code == CDN_FILEOK)
            file_msg = "OK";
          else
          {
            if (winIsFile(filename))
              file_msg = "SELECT";
            else
              file_msg = "OTHER";
          }

          ret = cb(ih, iupwinStrFromSystemFilename(filename), file_msg);

          if (pofn->hdr.code == CDN_FILEOK && (ret == IUP_IGNORE || ret == IUP_CONTINUE)) 
          {
            if (ret == IUP_CONTINUE) 
            {
              char* value = iupAttribGet(ih, "FILE");
              if (value)
              {
                iupwinStrCopy(filename, value, IUP_MAX_FILENAME_SIZE);
                winFileDlgStrReplacePathSlash(filename);
                SendMessage(GetParent(hWnd), CDM_SETCONTROLTEXT, (WPARAM)IUP_EDIT, (LPARAM)filename);
              }
            }

            SetWindowLongPtr(hWnd, DWLP_MSGRESULT, 1L);
            return 1; /* will refuse the file */
          }
        }
      }

      hWndPreview = GetDlgItem(hWnd, IUP_PREVIEWCANVAS);
      if (pofn->hdr.code == CDN_SELCHANGE && hWndPreview) 
        RedrawWindow(hWndPreview, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW);
      break;
    }
  case CDN_TYPECHANGE:
    {
      IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
      if (cb)
      {
        TCHAR filename[IUP_MAX_FILENAME_SIZE];
        if (winFileDlgGetSelectedFile(ih, hWnd, filename))
        {
          iupAttribSetInt(ih, "FILTERUSED", (int)pofn->lpOFN->nFilterIndex);
          if (cb(ih, iupwinStrFromSystemFilename(filename), "FILTER") == IUP_CONTINUE)
          {
            char* value = iupAttribGet(ih, "FILE");
            if (value)
            {
              iupwinStrCopy(filename, value, IUP_MAX_FILENAME_SIZE);
              winFileDlgStrReplacePathSlash(filename);
              SendMessage(GetParent(hWnd), CDM_SETCONTROLTEXT, (WPARAM)IUP_EDIT, (LPARAM)filename);
            }
          }
        }
      }
      break;
    }
  case CDN_HELP:
    {
      Icallback cb = (Icallback) IupGetCallback(ih, "HELP_CB");
      if (cb && cb(ih) == IUP_CLOSE) 
        EndDialog(GetParent(hWnd), IDCANCEL);
      break;
    }
  }

  return 0;
}

static UINT_PTR CALLBACK winFileDlgSimpleHook(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
  (void)wParam;
  switch(uiMsg)
  {
  case WM_INITDIALOG:
    {
      OPENFILENAME* openfilename = (OPENFILENAME*)lParam;
      Ihandle* ih = (Ihandle*)openfilename->lCustData;

      ih->handle = GetParent(hWnd);
      iupDialogUpdatePosition(ih);
      ih->handle = NULL;  /* reset handle */

      SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)ih);
      break;
    }
  case WM_DESTROY:
    {
      Ihandle* ih = (Ihandle*)GetWindowLongPtr(hWnd, DWLP_USER);
      IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
      if (cb) cb(ih, NULL, "FINISH");
      break;
    }
  case WM_NOTIFY:
    return winFileDlgWmNotify(hWnd, (LPOFNOTIFY)lParam);
  }
  return 0;
}

static void winFileDlgSetPreviewCanvasPos(HWND hWnd, HWND hWndPreview)
{
  int height, width, y, x;
  RECT rect, dlgrect;
  HWND hWndFileList = GetDlgItem(GetParent(hWnd), 0x0471);       /* path combo list with edit box (cmb2) */
  HWND hWndFileCombo = GetDlgItem(GetParent(hWnd), 0x047C); /* file name combo list with edit box (cmb13) */

  /* NOTE: it could be positioned only bellow the default controls */

  /* GetWindowRect return screen coordinates, must convert to parent's client coordinates */
  GetWindowRect(hWnd, &dlgrect);

  GetWindowRect(hWndPreview, &rect);
  y = rect.top - dlgrect.top;   /* at first time this is 0, else use system positioned value (not the same as RC) */
  height = rect.bottom - rect.top; /* keep the same height */

  if (y != 0) /* not first time */
  {
    /* position the child window that contains the template, must have room for the preview canvas */
    SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, (dlgrect.right - dlgrect.left), (dlgrect.bottom - dlgrect.top), SWP_NOMOVE | SWP_NOZORDER);
  }

  GetWindowRect(hWndFileList, &rect);
  x = rect.left - dlgrect.left;   /* horizontally align with file list at left */

  GetWindowRect(hWndFileCombo, &rect);
  width = (rect.right - dlgrect.left) - x;  /* set size to align with file combo at right */

  SetWindowPos(hWndPreview, HWND_TOP, x, y, width, height, SWP_NOZORDER);
}

static void winFileDlgUpdatePreviewGLCanvas(Ihandle* ih)
{
  Ihandle* glcanvas = IupGetAttributeHandle(ih, "PREVIEWGLCANVAS");
  if (glcanvas)
  {
    iupAttribSet(glcanvas, "HWND", iupAttribGet(ih, "HWND"));
    glcanvas->iclass->Map(glcanvas);  /* this will call Map only for the IupGLCanvas, NOT for the IupCanvas */
  }
}

static int winFileCheckPreviewCanvas(HWND hWnd, LPARAM lParam, int *x, int *y)
{
  HWND hWndPreview = GetDlgItem(hWnd, IUP_PREVIEWCANVAS);
  POINT pt;
  RECT rect;

  pt.x = GET_X_LPARAM(lParam);
  pt.y = GET_Y_LPARAM(lParam);
  if (!MapWindowPoints(hWnd, hWndPreview, &pt, 1))
  {
    pt.x = 0;
    pt.y = 0;
    MapWindowPoints(hWndPreview, hWnd, &pt, 1);
    pt.x = -pt.x;
    pt.y = -pt.y;
  }
  *x = pt.x;
  *y = pt.y;

  GetClientRect(hWndPreview, &rect);
  if (pt.x >= rect.left && pt.y >= rect.top &&
      pt.x <= rect.right && pt.y <= rect.bottom)
    return 1;
  return 0;
}

static int iupwinButtonDownXY(Ihandle* ih, UINT msg, WPARAM wp, int x, int y)
{
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  int ret, doubleclick = 0;
  int b = 0;

  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return 0;

  if (msg == WM_XBUTTONDBLCLK ||
      msg == WM_LBUTTONDBLCLK ||
      msg == WM_MBUTTONDBLCLK ||
      msg == WM_RBUTTONDBLCLK)
      doubleclick = 1;

  iupwinButtonKeySetStatus(LOWORD(wp), status, doubleclick);

  if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK)
    b = IUP_BUTTON1;
  else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK)
    b = IUP_BUTTON2;
  else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK)
    b = IUP_BUTTON3;
  else if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK)
  {
    if (HIWORD(wp) == XBUTTON1)
      b = IUP_BUTTON4;
    else
      b = IUP_BUTTON5;
  }

  ret = cb(ih, b, 1, x, y, status);
  if (ret == IUP_CLOSE)
    IupExitLoop();
  else if (ret == IUP_IGNORE)
    return -1;

  return 1;
}

static int iupwinButtonUpXY(Ihandle* ih, UINT msg, WPARAM wp, int x, int y)
{
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  int ret, b = 0;
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return 0;

  iupwinButtonKeySetStatus(LOWORD(wp), status, 0);

  /* also updates the button status, since wp could not have the flag */
  if (msg == WM_LBUTTONUP)
  {
    b = IUP_BUTTON1;
    iupKEY_SETBUTTON1(status);
  }
  else if (msg == WM_MBUTTONUP)
  {
    b = IUP_BUTTON2;
    iupKEY_SETBUTTON2(status);
  }
  else if (msg == WM_RBUTTONUP)
  {
    b = IUP_BUTTON3;
    iupKEY_SETBUTTON3(status);
  }
  else if (msg == WM_XBUTTONUP)
  {
    if (HIWORD(wp) == XBUTTON1)
    {
      b = IUP_BUTTON4;
      iupKEY_SETBUTTON4(status);
    }
    else
    {
      b = IUP_BUTTON5;
      iupKEY_SETBUTTON5(status);
    }
  }

  ret = cb(ih, b, 0, x, y, status);
  if (ret == IUP_CLOSE)
    IupExitLoop();
  else if (ret == IUP_IGNORE)
    return -1;

  return 1;
}

static int iupwinMouseMoveXY(Ihandle* ih, WPARAM wp, int x, int y)
{
  IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupwinButtonKeySetStatus(LOWORD(wp), status, 0);
    cb(ih, x, y, status);
    return 1;
  }
  return 0;
}

static UINT_PTR CALLBACK winFileDlgPreviewHook(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
  /* hWnd here is a handle to the child window that contains the template,
     NOT the file dialog. Only the preview canvas is a child of this window. */

  switch(uiMsg)
  {
  case WM_INITDIALOG:
    {
      OPENFILENAME* openfilename = (OPENFILENAME*)lParam;
      Ihandle* ih = (Ihandle*)openfilename->lCustData;
      HWND hWndPreview = GetDlgItem(hWnd, IUP_PREVIEWCANVAS);

      ih->handle = GetParent(hWnd);
      iupDialogUpdatePosition(ih);
      ih->handle = NULL;  /* reset handle */

      if (hWndPreview)
      {
        RECT rect;

        winFileDlgSetPreviewCanvasPos(hWnd, hWndPreview);

        GetClientRect(hWndPreview, &rect);
        iupAttribSetInt(ih, "PREVIEWWIDTH", rect.right - rect.left);
        iupAttribSetInt(ih, "PREVIEWHEIGHT", rect.bottom - rect.top);
      }
      SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)ih);
      iupAttribSet(ih, "WID", (char*)hWndPreview);
      iupAttribSet(ih, "HWND", (char*)hWndPreview);
      winFileDlgUpdatePreviewGLCanvas(ih);
      break;
    }
  case WM_DRAWITEM:
    {
      if (wParam == IUP_PREVIEWCANVAS)
      {
        LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
        Ihandle* ih = (Ihandle*)GetWindowLongPtr(hWnd, DWLP_USER);
        /* callback here always exists */
        IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
        if (cb)
        {
          TCHAR filename[IUP_MAX_FILENAME_SIZE];
          iupAttribSet(ih, "PREVIEWDC", (char*)lpDrawItem->hDC);
          iupAttribSet(ih, "HDC_WMPAINT", (char*)lpDrawItem->hDC);

          if (winFileDlgGetSelectedFile(ih, hWnd, filename))
          {
            if (winIsFile(filename))
              cb(ih, iupwinStrFromSystemFilename(filename), "PAINT");
            else
              cb(ih, NULL, "PAINT");
          }
          else
            cb(ih, NULL, "PAINT");
          iupAttribSet(ih, "PREVIEWDC", NULL);
          iupAttribSet(ih, "HDC_WMPAINT", NULL);
        }
      }
      break;
    }
  case WM_SIZE:
    {
      HWND hWndPreview = GetDlgItem(hWnd, IUP_PREVIEWCANVAS);
      if (hWndPreview)
      {
        Ihandle* ih = (Ihandle*)GetWindowLongPtr(hWnd, DWLP_USER);
        RECT rect;

        winFileDlgSetPreviewCanvasPos(hWnd, hWndPreview);

        GetClientRect(hWndPreview, &rect);
        iupAttribSetInt(ih, "PREVIEWWIDTH", rect.right-rect.left);
        iupAttribSetInt(ih, "PREVIEWHEIGHT", rect.bottom - rect.top);

        RedrawWindow(hWndPreview, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW);
      }
      break;
    }
  case WM_DESTROY:
    {
      Ihandle* ih = (Ihandle*)GetWindowLongPtr(hWnd, DWLP_USER);
      /* callback here always exists */
      IFnss cb = (IFnss)IupGetCallback(ih, "FILE_CB");
      if (cb) cb(ih, NULL, "FINISH");
      break;
    }
  case WM_XBUTTONDBLCLK:
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_XBUTTONDOWN:
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
  {
    int x, y;
    if (winFileCheckPreviewCanvas(hWnd, lParam, &x, &y))
    {
      Ihandle* ih = (Ihandle*)GetWindowLongPtr(hWnd, DWLP_USER);
      iupwinButtonDownXY(ih, uiMsg, wParam, x, y);
    }
    break;
  }
  case WM_MOUSEMOVE:
  {
    int x, y;
    Ihandle* ih = (Ihandle*)GetWindowLongPtr(hWnd, DWLP_USER);
    winFileCheckPreviewCanvas(hWnd, lParam, &x, &y); /* allow outside window values */
    iupwinMouseMoveXY(ih, wParam, x, y);
    break;
  }
  case WM_MOUSEWHEEL:
  {
    Ihandle* ih = (Ihandle*)GetWindowLongPtr(hWnd, DWLP_USER);
    IFnfiis cb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
    short delta = (short)HIWORD(wParam);
    if (cb)
    {
      HWND hWndPreview = GetDlgItem(hWnd, IUP_PREVIEWCANVAS);
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      POINT p;
      p.x = GET_X_LPARAM(lParam);
      p.y = GET_Y_LPARAM(lParam);

      ScreenToClient(hWndPreview, &p); /* allow outside window values */

      iupwinButtonKeySetStatus(LOWORD(wParam), status, 0);

      cb(ih, (float)delta / 120.0f, p.x, p.y, status);
    }

    return 1;
  }
  case WM_XBUTTONUP:
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
  {
    int x, y;
    if (winFileCheckPreviewCanvas(hWnd, lParam, &x, &y))
    {
      Ihandle* ih = (Ihandle*)GetWindowLongPtr(hWnd, DWLP_USER);
      iupwinButtonUpXY(ih, uiMsg, wParam, x, y);
    }
    break;
  }
  case WM_NOTIFY:
      return winFileDlgWmNotify(hWnd, (LPOFNOTIFY)lParam);
  }
  return 0;
}

static TCHAR* winFileDlgStrReplaceSeparator(const TCHAR* name)
{
  int i=0, len = lstrlen(name);
  TCHAR* buffer = (TCHAR*)malloc((len+2)*sizeof(TCHAR));

  /* replace symbols "|" by terminator "\0" */

  while (*name) 
  {
    buffer[i] = *name;

    if (buffer[i] == L'|')
      buffer[i] = 0;

    i++;
    name++;
  }

  buffer[i] = 0;
  buffer[i+1] = 0;      /* additional 0 at the end */
  return buffer;
}

static int winFileDlgUseHook(Ihandle *ih, int x, int y)
{
  if (IupGetCallback(ih, "FILE_CB") || IupGetCallback(ih, "HELP_CB"))
    return 1;

  if (x != IUP_CENTER && x != IUP_CURRENT && x != IUP_CENTERPARENT)
    return 1;

  if (y != IUP_CENTER && y != IUP_CURRENT && y != IUP_CENTERPARENT)
    return 1;

  return 0;
}

static int winFileDlgPopup(Ihandle *ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  OPENFILENAME openfilename;
  int result, dialogtype;
  char *value, *initial_dir=NULL;
  TCHAR* extfilter = NULL;

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  value = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(value, "SAVE"))
    dialogtype = IUP_DIALOGSAVE;
  else if (iupStrEqualNoCase(value, "DIR"))
    dialogtype = IUP_DIALOGDIR;
  else
    dialogtype = IUP_DIALOGOPEN;

  if (dialogtype == IUP_DIALOGDIR)
  {
    winFileDlgGetFolder(ih);
    return IUP_NOERROR;
  }

  /* if NOT set will NOT be Modal */
  /* anyway it will be modal only relative to its parent */
  if (!parent)
    parent = GetActiveWindow();

  ZeroMemory(&openfilename, sizeof(OPENFILENAME));
  openfilename.lStructSize = sizeof(OPENFILENAME);
  openfilename.hwndOwner = parent;

  value = iupAttribGet(ih, "EXTFILTER");
  if (value)
  {
    int index;
    extfilter = winFileDlgStrReplaceSeparator(iupwinStrToSystem(value));
    openfilename.lpstrFilter = extfilter;

    value = iupAttribGet(ih, "FILTERUSED");
    if (iupStrToInt(value, &index))
      openfilename.nFilterIndex = index;
    else
      openfilename.nFilterIndex = 1;
  }
  else 
  {
    value = iupAttribGet(ih, "FILTER");
    if (value)
    {
      TCHAR *winfo, *wvalue;
      int sz1, sz2;
      char* info = iupAttribGet(ih, "FILTERINFO");
      if (!info)
        info = value;

      winfo = iupwinStrToSystem(info);
      wvalue = iupwinStrToSystem(value);

      /* concat FILTERINFO+FILTER */
      sz1 = lstrlen(winfo)+1; /* each part has a terminator */
      sz2 = lstrlen(wvalue)+1;
      extfilter = (TCHAR*)malloc((sz1+sz2+1)*sizeof(TCHAR));
      memcpy(extfilter, winfo, sz1*sizeof(TCHAR)); /* copy also the terminator */
      memcpy(extfilter+sz1, wvalue, sz2*sizeof(TCHAR));
      extfilter[sz1+sz2] = 0;  /* additional terminator at the end */

      openfilename.lpstrFilter = extfilter;
      openfilename.nFilterIndex = 1;
    }
  }

  openfilename.lpstrFile = (TCHAR*)malloc((IUP_MAX_FILENAME_SIZE+1)*sizeof(TCHAR));
  value = iupAttribGet(ih, "FILE");
  if (value)
  {
    iupwinStrCopy(openfilename.lpstrFile, value, IUP_MAX_FILENAME_SIZE);
    winFileDlgStrReplacePathSlash(openfilename.lpstrFile);
  }
  else
    openfilename.lpstrFile[0] = 0;

  openfilename.nMaxFile = IUP_MAX_FILENAME_SIZE;

  /* only supports extensions with up to three characters, should NOT include the period */
  openfilename.lpstrDefExt = iupwinStrToSystem(iupAttribGet(ih, "EXTDEFAULT"));

  initial_dir = iupStrDup(iupAttribGet(ih, "DIRECTORY"));
  openfilename.lpstrInitialDir = iupwinStrToSystemFilename(initial_dir);
  if (openfilename.lpstrInitialDir)
    winFileDlgStrReplacePathSlash((TCHAR*)openfilename.lpstrInitialDir);

  openfilename.lpstrTitle = iupwinStrToSystem(iupAttribGet(ih, "TITLE"));
  openfilename.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

  if (iupAttribGetBoolean(ih, "NOPLACESBAR"))
    openfilename.FlagsEx = OFN_EX_NOPLACESBAR;

  if (!iupAttribGetBoolean(ih, "NOOVERWRITEPROMPT"))
    openfilename.Flags |= OFN_OVERWRITEPROMPT;

  if (iupAttribGetBoolean(ih, "SHOWHIDDEN"))
    openfilename.Flags |= OFN_FORCESHOWHIDDEN;

  value = iupAttribGet(ih, "ALLOWNEW");
  if (!value)
  {
    if (dialogtype == IUP_DIALOGSAVE)
      value = "YES";
    else
      value = "NO";
  }
  if (iupStrBoolean(value))
    openfilename.Flags |= OFN_CREATEPROMPT;
  else
    openfilename.Flags |= OFN_FILEMUSTEXIST;

  if (iupAttribGetBoolean(ih, "NOCHANGEDIR"))
    openfilename.Flags |= OFN_NOCHANGEDIR;

  if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
     openfilename.Flags |= OFN_ALLOWMULTISELECT;

  if (winFileDlgUseHook(ih, x, y))
  {
    openfilename.lpfnHook = winFileDlgSimpleHook;
    openfilename.Flags |= OFN_ENABLEHOOK;
    openfilename.lCustData = (LPARAM)ih;
  }

  openfilename.Flags |= OFN_EXPLORER | OFN_ENABLESIZING;

  if (iupAttribGetBoolean(ih, "SHOWPREVIEW") && IupGetCallback(ih, "FILE_CB"))
  {
    openfilename.Flags |= OFN_ENABLETEMPLATE;
    openfilename.hInstance = iupwin_dll_hinstance? iupwin_dll_hinstance: iupwin_hinstance;
    openfilename.lpTemplateName = TEXT("iupPreviewDlg");
    openfilename.lpfnHook = winFileDlgPreviewHook;
  }

  if (IupGetCallback(ih, "HELP_CB"))
    openfilename.Flags |= OFN_SHOWHELP;

  if (dialogtype == IUP_DIALOGOPEN)
    result = GetOpenFileName(&openfilename);
  else
    result = GetSaveFileName(&openfilename);

  if (result)
  {
    if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
    {
      /* If there is more than one file, replace terminator by the separator */
      if (openfilename.lpstrFile[openfilename.nFileOffset-1] == 0 && 
          openfilename.nFileOffset>0) 
      {
        int i = 0;
        int count = 0;

        char* dir = iupwinStrFromSystemFilename(openfilename.lpstrFile);  /* already is the directory, but without the last separator */

        if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
          iupAttribSetStrf(ih, "VALUE", "%s|", dir);

        iupAttribSetStrf(ih, "DIRECTORY", "%s\\", dir);  /* add the last separator */
        dir = iupAttribGet(ih, "DIRECTORY");

        iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);  /* same as directory, includes last separator */
        count++;

        while (openfilename.lpstrFile[i] != 0 || openfilename.lpstrFile[i + 1] != 0)
        {
          if (openfilename.lpstrFile[i] == 0)
          {
            char* filename = iupwinStrFromSystemFilename(openfilename.lpstrFile + i + 1);
            if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
            {
              char nameid[100];
              value = iupAttribGet(ih, "VALUE");
              sprintf(nameid, "MULTIVALUE%d", count);
              iupAttribSetStrf(ih, nameid, "%s%s", dir, filename);

              iupAttribSetStrf(ih, "VALUE", "%s%s|", value, iupAttribGetId(ih, "MULTIVALUE", count));
            }
            else
              iupAttribSetStrId(ih, "MULTIVALUE", count, filename);
            count++;

            openfilename.lpstrFile[i] = TEXT('|');
          }

          i++;
        }

        iupAttribSetInt(ih, "MULTIVALUECOUNT", count);
        openfilename.lpstrFile[i] = TEXT('|');  /* one last at the end */

        if (!iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
          iupAttribSetStr(ih, "VALUE", iupwinStrFromSystemFilename(openfilename.lpstrFile));  /* here file was already modified to match the IUP format */
      }
      else
      {
        /* if there is only one file selected the returned value is different 
           and includes just that file */
        char* filename = iupwinStrFromSystemFilename(openfilename.lpstrFile);
        char* dir = iupStrFileGetPath(filename);
        int dir_len = (int)strlen(dir);
        iupAttribSetStr(ih, "DIRECTORY", dir);

        iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);  /* same as directory, includes last separator */

        if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
          dir_len = 0;

        iupAttribSetStrId(ih, "MULTIVALUE", 1, filename + dir_len);

        iupAttribSetStr(ih, "VALUE", filename);  /* here value is not separated by '|' */

        iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);
        free(dir);
      }

      iupAttribSet(ih, "STATUS", "0");
      iupAttribSet(ih, "FILEEXIST", "YES");
    }
    else
    {
      char* filename = iupwinStrFromSystemFilename(openfilename.lpstrFile);
      char* dir = iupStrFileGetPath(filename);
      iupAttribSetStr(ih, "DIRECTORY", dir);
      free(dir);

      if (winIsFile(openfilename.lpstrFile))  /* check if file exists */
      {
        iupAttribSet(ih, "FILEEXIST", "YES");
        iupAttribSet(ih, "STATUS", "0");
      }
      else
      {
        iupAttribSet(ih, "FILEEXIST", "NO");
        iupAttribSet(ih, "STATUS", "1");
      }

      iupAttribSetStr(ih, "VALUE", filename);
    }

    iupAttribSetInt(ih, "FILTERUSED", (int)openfilename.nFilterIndex);
  }
  else
  {
    iupAttribSet(ih, "FILTERUSED", NULL);
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "DIRECTORY", NULL);
    iupAttribSet(ih, "FILEEXIST", NULL);
    iupAttribSet(ih, "STATUS", "-1");
  }

  if (extfilter) free(extfilter);
  if (initial_dir) free(initial_dir);
  if (openfilename.lpstrFile) free(openfilename.lpstrFile);

  return IUP_NOERROR;
}

void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winFileDlgPopup;

  /* IupFileDialog Windows and GTK Only */
  iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Windows Only */
  iupClassRegisterAttribute(ic, "NOPLACESBAR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
