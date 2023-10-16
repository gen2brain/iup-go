/** \file
 * \brief Clipboard for the Windows Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
 
#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupwin_drv.h"
#include "iupwin_str.h"


/* ATTENTION:
  "If an application calls OpenClipboard with hwnd set to NULL, 
   EmptyClipboard sets the clipboard owner to NULL; 
   this causes SetClipboardData to fail." 
  Because of this we use GetForegroundWindow() */

typedef struct _APMFILEHEADER
{
  WORD  key1,
        key2,
        hmf,
        bleft, btop, bright, bbottom,
        inch,
        reserved1,
        reserved2,
        checksum;
} APMFILEHEADER;

static WORD winAPMChecksum(APMFILEHEADER* papm)
{
  WORD* pw = (WORD*)papm;
  WORD  wSum = 0;
  int   i;
  
  /* The checksum in a Placeable Metafile header is calculated */
  /* by XOR-ing the first 10 words of the header.              */
  
  for (i = 0; i < 10; i++)
    wSum ^= *pw++;
  
  return wSum;
}

static void winWritePlacebleFile(HANDLE hFile, unsigned char* buffer, DWORD dwSize, LONG mm, LONG xExt, LONG yExt)
{
  DWORD nBytesWrite;
  APMFILEHEADER APMHeader;
  int w = xExt, h = yExt;
  
  if (mm == MM_ANISOTROPIC || mm == MM_ISOTROPIC)
  {
    int res = 30;
    w = xExt / res;
    h = yExt / res;
  }
  
  APMHeader.key1 = 0xCDD7;
  APMHeader.key2 = 0x9AC6;
  APMHeader.hmf = 0;
  APMHeader.bleft = 0;
  APMHeader.btop = 0;
  APMHeader.bright = (short)w;
  APMHeader.bbottom = (short)h;
  APMHeader.inch = 100;  /* this number works fine in Word, etc.. */
  APMHeader.reserved1 = 0;
  APMHeader.reserved2 = 0;
  APMHeader.checksum = winAPMChecksum(&APMHeader);
  
  WriteFile(hFile, (LPSTR)&APMHeader, sizeof(APMFILEHEADER), &nBytesWrite, NULL);
  WriteFile(hFile, buffer, dwSize, &nBytesWrite, NULL);
}

static int winClipboardSetSaveEMFAttrib(Ihandle *ih, const char *value)
{
  HENHMETAFILE Handle;
  DWORD dwSize, nBytesWrite;
  unsigned char* buffer;
  HANDLE hFile;
  (void)ih;
  
  OpenClipboard(GetForegroundWindow());
  Handle = (HENHMETAFILE)GetClipboardData(CF_ENHMETAFILE);
  if (Handle == NULL)
  {
    CloseClipboard();
    return 0;
  }
  
  dwSize = GetEnhMetaFileBits(Handle, 0, NULL);
  
  buffer = (unsigned char*)malloc(dwSize);
  
  GetEnhMetaFileBits(Handle, dwSize, buffer);
  
  hFile = CreateFile(iupwinStrToSystemFilename(value), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
  if (hFile)
  {
    WriteFile(hFile, buffer, dwSize, &nBytesWrite, NULL);
    CloseHandle(hFile);
  }
  
  free(buffer);
  
  CloseClipboard();
  return 0;
}

static int winClipboardSetSaveWMFAttrib(Ihandle *ih, const char *value)
{
  DWORD dwSize;
  unsigned char* buffer;
  METAFILEPICT* lpMFP;
  HANDLE Handle;
  HANDLE hFile;
  (void)ih;
  
  OpenClipboard(GetForegroundWindow());
  Handle = (HENHMETAFILE)GetClipboardData(CF_METAFILEPICT);
  if (Handle == NULL)
  {
    CloseClipboard();
    return 0;
  }
  
  lpMFP = (METAFILEPICT*) GlobalLock(Handle);
  
  dwSize = GetMetaFileBitsEx(lpMFP->hMF, 0, NULL);
  
  buffer = (unsigned char*)malloc(dwSize);
  
  GetMetaFileBitsEx(lpMFP->hMF, dwSize, buffer);
  
  hFile = CreateFile(iupwinStrToSystemFilename(value), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
  if (hFile)
  {
    winWritePlacebleFile(hFile, buffer, dwSize, lpMFP->mm, lpMFP->xExt, lpMFP->yExt);
    CloseHandle(hFile);
  }
  
  GlobalUnlock(Handle);
  free(buffer);
  
  CloseClipboard();
  return 0;
}

static int winClipboardSetTextAttrib(Ihandle *ih, const char *value)
{
  HANDLE hHandle;
  TCHAR* wstr;
  char* dos_str;
  void* clip_str;
  int size;
  (void)ih;

  if (!OpenClipboard(GetForegroundWindow()))
    return 0;

  if (!value)
  {
    EmptyClipboard();
    CloseClipboard();
    return 0;
  }

  /* CF_TEXT/CF_UNICODETEXT: Each line ends with a carriage return/linefeed (CR-LF) combination. */
  dos_str = iupStrToDos(value);
#ifdef UNICODE
  wstr = iupwinStrToSystem(dos_str);
#else
  if (dos_str != value) 
    wstr = iupStrReturnStr(dos_str);
  else
    wstr = dos_str;
#endif
  if (dos_str != value) free(dos_str);

  size = (lstrlen(wstr)+1) * sizeof(TCHAR);
  hHandle = GlobalAlloc(GMEM_MOVEABLE, size); 
  if (!hHandle)
    return 0;

  clip_str = GlobalLock(hHandle);
  CopyMemory(clip_str, wstr, size);
  GlobalUnlock(hHandle);

#ifdef UNICODE
  SetClipboardData(CF_UNICODETEXT, hHandle);
#else
  SetClipboardData(CF_TEXT, hHandle);
#endif

  CloseClipboard();

  return 0;
}

static char* winClipboardGetTextAttrib(Ihandle *ih)
{
  HANDLE hHandle;
  char* str;
  (void)ih;

  if (!OpenClipboard(GetForegroundWindow()))
    return NULL;

#ifdef UNICODE
  hHandle = GetClipboardData(CF_UNICODETEXT);
  if (hHandle)
  {
    WCHAR* wstr = (WCHAR*)GlobalLock(hHandle);
    str = iupwinStrFromSystem(wstr);
  }
  else
#endif
  {
    hHandle = GetClipboardData(CF_TEXT);
    if (!hHandle)
    {
      CloseClipboard();
      return NULL;
    }

    str = (char*)GlobalLock(hHandle);
  }

  str = iupStrReturnStr(str);
  if (str)
    iupStrToUnix(str);

  GlobalUnlock(hHandle);
  CloseClipboard();
  return str;
}

static int winClipboardSetImageAttrib(Ihandle *ih, const char *value)
{
  HBITMAP hBitmap;

  if (!OpenClipboard(GetForegroundWindow()))
    return 0;

  if (!value)
  {
    EmptyClipboard();
    CloseClipboard();
    return 0;
  }

  hBitmap = (HBITMAP)iupImageGetImage(value, ih, 0, NULL);
  iupImageRemoveFromCache(ih, hBitmap);  /* to avoid being destroyed later */

  SetClipboardData(CF_BITMAP, (HANDLE)hBitmap);
  CloseClipboard();

  return 0;
}

static int winClipboardSetNativeImageAttrib(Ihandle *ih, const char *value)
{
  if (!OpenClipboard(GetForegroundWindow()))
    return 0;

  if (!value)
  {
    EmptyClipboard();
    CloseClipboard();
    return 0;
  }

  SetClipboardData(CF_DIB, (HANDLE)value);
  CloseClipboard();

  (void)ih;
  return 0;
}

static HANDLE winCopyHandle(HANDLE hHandle)
{
  void *src_data, *dst_data;
  HANDLE hNewHandle;
  SIZE_T size = GlobalSize(hHandle);
  if (size == 0)
    return NULL;
  hNewHandle = GlobalAlloc(GMEM_MOVEABLE, size); 
  if (!hNewHandle)
    return NULL;

  src_data = GlobalLock(hHandle);
  dst_data = GlobalLock(hNewHandle);
  CopyMemory(dst_data, src_data, size);
  GlobalUnlock(hHandle);
  GlobalUnlock(hNewHandle);

  return hNewHandle;
}

static char* winClipboardGetNativeImageAttrib(Ihandle *ih)
{
  HANDLE hHandle;

  if (!OpenClipboard(GetForegroundWindow()))
    return 0;

  hHandle = GetClipboardData(CF_DIB);
  if (!hHandle)
  {
    CloseClipboard();
    return NULL;
  }

  hHandle = winCopyHandle(hHandle);   /* must duplicate because CloseClipboard will invalidate the handle */
  CloseClipboard();
  
  (void)ih;
  return (char*)hHandle;
}

static int winClipboardGetFormatId(Ihandle *ih)
{
  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return 0;
  return RegisterClipboardFormat(iupwinStrToSystem(format));
}

static int winClipboardSetFormatDataAttrib(Ihandle *ih, const char *value)
{
  HANDLE hHandle;
  void* data;
  int size;
  UINT format_id;

  if (!OpenClipboard(GetForegroundWindow()))
    return 0;

  if (!value)
  {
    EmptyClipboard();
    CloseClipboard();
    return 0;
  }

  size = iupAttribGetInt(ih, "FORMATDATASIZE");
  if (!size)
    return 0;

  format_id = winClipboardGetFormatId(ih);
  if (format_id==0)
    return 0;

  hHandle = GlobalAlloc(GMEM_MOVEABLE, size); 
  if (!hHandle)
    return 0;

  data = GlobalLock(hHandle);
  CopyMemory(data, value, size);
  GlobalUnlock(hHandle);

  SetClipboardData(format_id, hHandle);
  CloseClipboard();

  return 0;
}

static char* winClipboardGetFormatDataAttrib(Ihandle *ih)
{
  HANDLE hHandle;
  char* data;
  UINT format_id;
  int size;

  if (!OpenClipboard(GetForegroundWindow()))
    return NULL;

  format_id = winClipboardGetFormatId(ih);
  if (format_id == 0)
    return NULL;

  hHandle = GetClipboardData(format_id);
  if (!hHandle)
  {
    CloseClipboard();
    return NULL;
  }
  
  size = (int)GlobalSize(hHandle);
  if (size == 0)
  {
    CloseClipboard();
    return NULL;
  }
  data = iupStrGetMemory(size+1); /* reserve room for terminator if get FORMATDATASTRING is used */

  CopyMemory(data, (char*)GlobalLock(hHandle), size);
  GlobalUnlock(hHandle);
 
  GlobalUnlock(hHandle);
  CloseClipboard();

  iupAttribSetInt(ih, "FORMATDATASIZE", size);
  return data;
}

static char* winClipboardGetFormatDataStringAttrib(Ihandle *ih)
{
  TCHAR* data = (TCHAR*)winClipboardGetFormatDataAttrib(ih);
  int size = iupAttribGetInt(ih, "FORMATDATASIZE");
  data[size] = 0;  /* add terminator, even if there is one already */
  return iupStrReturnStr(iupwinStrFromSystem(data));
}

static int winClipboardSetFormatDataStringAttrib(Ihandle *ih, const char *value)
{
  if (value)
  {
    int len = (int)strlen(value);
    TCHAR* wstr = iupwinStrToSystemLen(value, &len);
    iupAttribSetInt(ih, "FORMATDATASIZE", sizeof(TCHAR) * (len + 1)); /* include the terminator, because the other side may not be an IUP application */
    return winClipboardSetFormatDataAttrib(ih, (char*)wstr);
  }
  else
    return winClipboardSetFormatDataAttrib(ih, NULL);
}

static int winClipboardIsAvailable(UINT format_id)
{
  int check;
  OpenClipboard(GetForegroundWindow());
  check = IsClipboardFormatAvailable(format_id);
  CloseClipboard();
  return check;
}

static char* winClipboardGetTextAvailableAttrib(Ihandle *ih)
{
  (void)ih;
#ifdef UNICODE
  return iupStrReturnBoolean(winClipboardIsAvailable(CF_TEXT) || winClipboardIsAvailable(CF_UNICODETEXT));
#else
  return iupStrReturnBoolean (winClipboardIsAvailable(CF_TEXT)); 
#endif
}

static char* winClipboardGetImageAvailableAttrib(Ihandle *ih)
{
  (void)ih;
  return iupStrReturnBoolean (winClipboardIsAvailable(CF_DIB)); 
}

static char* winClipboardGetWMFAvailableAttrib(Ihandle *ih)
{
  (void)ih;
  return iupStrReturnBoolean (winClipboardIsAvailable(CF_METAFILEPICT)); 
}

static char* winClipboardGetEMFAvailableAttrib(Ihandle *ih)
{
  (void)ih;
  return iupStrReturnBoolean (winClipboardIsAvailable(CF_ENHMETAFILE)); 
}

static char* winClipboardGetFormatAvailableAttrib(Ihandle *ih)
{
  UINT format_id = winClipboardGetFormatId(ih);
  if (format_id==0)
    return NULL;

  return iupStrReturnBoolean (winClipboardIsAvailable(format_id)); 
}

static int winClipboardSetAddFormatAttrib(Ihandle *ih, const char *value)
{
  if (value)
    RegisterClipboardFormat(iupwinStrToSystem(value));

  (void)ih;
  return 0;
}

/******************************************************************************/

IUP_API Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "clipboard";
  ic->format = NULL;  /* no parameters */
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  /* Attribute functions */
  iupClassRegisterAttribute(ic, "TEXT", winClipboardGetTextAttrib, winClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", winClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEIMAGE", winClipboardGetNativeImageAttrib, winClipboardSetNativeImageAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winClipboardSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", winClipboardGetImageAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "WMFAVAILABLE", winClipboardGetWMFAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EMFAVAILABLE", winClipboardGetEMFAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEEMF", NULL, winClipboardSetSaveEMFAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEWMF", NULL, winClipboardSetSaveWMFAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, winClipboardSetAddFormatAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", winClipboardGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", winClipboardGetFormatDataAttrib, winClipboardSetFormatDataAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", winClipboardGetFormatDataStringAttrib, winClipboardSetFormatDataStringAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}


