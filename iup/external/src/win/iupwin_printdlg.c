/** \file
 * \brief IupPrintDlg pre-defined dialog, shared by the Win32 and WinUI drivers
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef CINTERFACE
#define CINTERFACE
#endif

#include <windows.h>
#include <commdlg.h>
#include <documenttarget.h>
#include <prntvpt.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_stdcontrols.h"

#include "iupwin_printdlg.h"

#ifdef __cplusplus
#define IWIN_GUID_REF(_guid) _guid
#else
#define IWIN_GUID_REF(_guid) &_guid
#endif

#define IUPWIN_PRINT_MAXRANGES 16

typedef HRESULT (WINAPI *IwinPTOpenProvider)(PCWSTR, DWORD, HPTPROVIDER*);
typedef HRESULT (WINAPI *IwinPTConvertDevModeToPrintTicket)(HPTPROVIDER, ULONG, PDEVMODEW, EPrintTicketScope, IStream*);
typedef HRESULT (WINAPI *IwinPTCloseProvider)(HPTPROVIDER);
typedef HRESULT (WINAPI *IwinPrintDlgExW)(LPPRINTDLGEXW);

static const GUID iupwin_CLSID_PrintDocumentPackageTargetFactory =
  { 0x348ef17d, 0x6c81, 0x4982, { 0x92, 0xb4, 0xee, 0x18, 0x8a, 0x43, 0x86, 0x7a } };
static const GUID iupwin_IID_IPrintDocumentPackageTargetFactory =
  { 0xd2959bf7, 0xb31b, 0x4a3d, { 0x96, 0x00, 0x71, 0x2e, 0xb1, 0x33, 0x5b, 0xa4 } };

static WCHAR* winPrintDlgToWide(const char* str)
{
  int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
  WCHAR* wstr;
  if (len <= 0)
    return NULL;
  wstr = (WCHAR*)malloc(len * sizeof(WCHAR));
  if (wstr)
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len);
  return wstr;
}

static HGLOBAL winPrintDlgCreateDevMode(Ihandle* ih)
{
  HGLOBAL hDevMode = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(DEVMODEW));
  DEVMODEW* dm;
  if (!hDevMode)
    return NULL;

  dm = (DEVMODEW*)GlobalLock(hDevMode);
  dm->dmSize = sizeof(DEVMODEW);
  dm->dmFields = DM_ORIENTATION;
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "LANDSCAPE"))
    dm->dmOrientation = DMORIENT_LANDSCAPE;
  else
    dm->dmOrientation = DMORIENT_PORTRAIT;
  GlobalUnlock(hDevMode);
  return hDevMode;
}

static IStream* winPrintDlgCreateTicket(const WCHAR* printer_name, DEVMODEW* devmode)
{
  HMODULE prntvpt = LoadLibraryW(L"prntvpt.dll");
  IwinPTOpenProvider open_provider;
  IwinPTConvertDevModeToPrintTicket convert;
  IwinPTCloseProvider close_provider;
  HPTPROVIDER provider = NULL;
  IStream* ticket = NULL;
  LARGE_INTEGER zero;

  if (!prntvpt)
    return NULL;

  open_provider = (IwinPTOpenProvider)(void*)GetProcAddress(prntvpt, "PTOpenProvider");
  convert = (IwinPTConvertDevModeToPrintTicket)(void*)GetProcAddress(prntvpt, "PTConvertDevModeToPrintTicket");
  close_provider = (IwinPTCloseProvider)(void*)GetProcAddress(prntvpt, "PTCloseProvider");

  if (open_provider && convert && close_provider && SUCCEEDED(open_provider(printer_name, 1, &provider)))
  {
    if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &ticket)))
    {
      if (FAILED(convert(provider, devmode->dmSize + devmode->dmDriverExtra, devmode, kPTJobScope, ticket)))
      {
        ticket->lpVtbl->Release(ticket);
        ticket = NULL;
      }
      else
      {
        zero.QuadPart = 0;
        ticket->lpVtbl->Seek(ticket, zero, STREAM_SEEK_SET, NULL);
      }
    }
    close_provider(provider);
  }

  FreeLibrary(prntvpt);
  return ticket;
}

static int winPrintDlgPrintPages(Ihandle* ih, PRINTDLGEXW* pd, const WCHAR* printer_name, DEVMODEW* devmode)
{
  HDC hdc = pd->hDC;
  float dpi_x = (float)GetDeviceCaps(hdc, LOGPIXELSX);
  float dpi_y = (float)GetDeviceCaps(hdc, LOGPIXELSY);
  IwinPrintPage page_info;
  IPrintDocumentPackageTargetFactory* factory = NULL;
  IPrintDocumentPackageTarget* target = NULL;
  IStream* ticket;
  WCHAR* job_name;
  void* print_control = NULL;
  int count, copies, copy, ret = 1;

  page_info.paper_width = GetDeviceCaps(hdc, PHYSICALWIDTH) * 96.0f / dpi_x;
  page_info.paper_height = GetDeviceCaps(hdc, PHYSICALHEIGHT) * 96.0f / dpi_y;
  page_info.offset_x = GetDeviceCaps(hdc, PHYSICALOFFSETX) * 96.0f / dpi_x;
  page_info.offset_y = GetDeviceCaps(hdc, PHYSICALOFFSETY) * 96.0f / dpi_y;
  page_info.width = iupRound(GetDeviceCaps(hdc, HORZRES) * 96.0 / dpi_x);
  page_info.height = iupRound(GetDeviceCaps(hdc, VERTRES) * 96.0 / dpi_y);

  count = iupPrintDlgPaginate(ih, page_info.width, page_info.height);
  copies = pd->nCopies > 0 ? (int)pd->nCopies : 1;

  ticket = winPrintDlgCreateTicket(printer_name, devmode);
  job_name = winPrintDlgToWide(iupPrintDlgGetJobName(ih));

  if (FAILED(CoCreateInstance(IWIN_GUID_REF(iupwin_CLSID_PrintDocumentPackageTargetFactory), NULL, CLSCTX_INPROC_SERVER,
                              IWIN_GUID_REF(iupwin_IID_IPrintDocumentPackageTargetFactory), (void**)&factory)) ||
      FAILED(factory->lpVtbl->CreateDocumentPackageTargetForPrintJob(factory, printer_name,
                              job_name ? job_name : L"IUP", NULL, ticket, &target)))
    ret = -1;
  else
  {
    print_control = iupwinPrintDlgBeginDoc(target);
    if (!print_control)
      ret = -1;
  }

  for (copy = 0; copy < copies && ret == 1; copy++)
  {
    DWORD r, num_ranges = (pd->Flags & PD_PAGENUMS) ? pd->nPageRanges : 1;

    for (r = 0; r < num_ranges && ret == 1; r++)
    {
      int first = 1, last = count, page;

      if (pd->Flags & PD_PAGENUMS)
      {
        first = (int)pd->lpPageRanges[r].nFromPage;
        last = (int)pd->lpPageRanges[r].nToPage;
        if (last > count)
          last = count;
      }

      for (page = first; page <= last && ret == 1; page++)
      {
        int page_ret = iupwinPrintDlgDrawPage(ih, print_control, page, &page_info);
        if (page_ret == IUP_CLOSE)
          ret = 0;
        else if (page_ret == IUP_ERROR)
          ret = -1;
      }
    }
  }

  if (ret != 1 && target)
    target->lpVtbl->Cancel(target);
  if (print_control && !iupwinPrintDlgEndDoc(print_control) && ret == 1)
    ret = -1;

  if (target)
    target->lpVtbl->Release(target);
  if (factory)
    factory->lpVtbl->Release(factory);
  if (ticket)
    ticket->lpVtbl->Release(ticket);
  free(job_name);

  return ret;
}

static int winPrintDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  PRINTPAGERANGE ranges[IUPWIN_PRINT_MAXRANGES];
  PRINTDLGEXW pd;
  HMODULE comdlg32 = LoadLibraryW(L"comdlg32.dll");
  IwinPrintDlgExW print_dlg_ex = comdlg32 ? (IwinPrintDlgExW)(void*)GetProcAddress(comdlg32, "PrintDlgExW") : NULL;
  HRESULT hr;
  int page_count;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  if (!print_dlg_ex)
  {
    if (comdlg32)
      FreeLibrary(comdlg32);
    iupAttribSet(ih, "STATUS", "-1");
    return IUP_NOERROR;
  }

  if (!parent)
    parent = (InativeHandle*)GetActiveWindow();

  page_count = iupAttribGetInt(ih, "PAGECOUNT");

  ZeroMemory(&pd, sizeof(pd));
  pd.lStructSize = sizeof(pd);
  pd.hwndOwner = parent ? (HWND)parent : GetDesktopWindow();
  pd.hDevMode = winPrintDlgCreateDevMode(ih);
  pd.Flags = PD_RETURNDC | PD_NOSELECTION | PD_NOCURRENTPAGE | PD_USEDEVMODECOPIESANDCOLLATE;
  pd.nMaxPageRanges = IUPWIN_PRINT_MAXRANGES;
  pd.lpPageRanges = ranges;
  pd.nMinPage = 1;
  pd.nMaxPage = page_count > 0 ? (DWORD)page_count : 0xFFFF;
  pd.nCopies = 1;
  pd.nStartPage = START_PAGE_GENERAL;

  hr = print_dlg_ex(&pd);

  if (FAILED(hr))
    iupAttribSet(ih, "STATUS", "-1");
  else if (pd.dwResultAction != PD_RESULT_PRINT)
    iupAttribSet(ih, "STATUS", "0");
  else
  {
    int ret = -1;

    if (pd.hDC && pd.hDevNames && pd.hDevMode)
    {
      DEVNAMES* devnames = (DEVNAMES*)GlobalLock(pd.hDevNames);
      DEVMODEW* devmode = (DEVMODEW*)GlobalLock(pd.hDevMode);
      if (devnames && devmode)
      {
        iupAttribSet(ih, "ORIENTATION", devmode->dmOrientation == DMORIENT_LANDSCAPE ? "LANDSCAPE" : "PORTRAIT");
        ret = winPrintDlgPrintPages(ih, &pd, (const WCHAR*)devnames + devnames->wDeviceOffset, devmode);
      }
      if (devmode)
        GlobalUnlock(pd.hDevMode);
      if (devnames)
        GlobalUnlock(pd.hDevNames);
    }

    iupAttribSet(ih, "STATUS", ret == 1 ? "1" : (ret == 0 ? "0" : "-1"));
  }

  if (pd.hDC)
    DeleteDC(pd.hDC);
  if (pd.hDevMode)
    GlobalFree(pd.hDevMode);
  if (pd.hDevNames)
    GlobalFree(pd.hDevNames);

  FreeLibrary(comdlg32);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvPrintDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winPrintDlgPopup;
}
