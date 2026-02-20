/** \file
 * \brief WinUI Driver - Clipboard
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_class.h"
#include "iup_image.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

using namespace winrt;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::Graphics::Imaging;
using namespace Microsoft::UI::Xaml::Media::Imaging;


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

static WORD winuiAPMChecksum(APMFILEHEADER* papm)
{
  WORD* pw = (WORD*)papm;
  WORD  wSum = 0;
  int   i;

  for (i = 0; i < 10; i++)
    wSum ^= *pw++;

  return wSum;
}

static void winuiWritePlaceableFile(HANDLE hFile, unsigned char* buffer, DWORD dwSize, LONG mm, LONG xExt, LONG yExt)
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
  APMHeader.inch = 100;
  APMHeader.reserved1 = 0;
  APMHeader.reserved2 = 0;
  APMHeader.checksum = winuiAPMChecksum(&APMHeader);

  WriteFile(hFile, (LPSTR)&APMHeader, sizeof(APMFILEHEADER), &nBytesWrite, NULL);
  WriteFile(hFile, buffer, dwSize, &nBytesWrite, NULL);
}

static UINT winuiClipboardGetFormatId(Ihandle* ih)
{
  char* format = iupAttribGetStr(ih, "FORMAT");
  if (!format)
    return 0;
  std::wstring wformat = iupwinuiStringToWString(format);
  return RegisterClipboardFormatW(wformat.c_str());
}

static int winuiClipboardIsAvailable(UINT format_id)
{
  int check;
  OpenClipboard(GetForegroundWindow());
  check = IsClipboardFormatAvailable(format_id);
  CloseClipboard();
  return check;
}

/************************** Text **************************/

static int winuiClipboardSetTextAttrib(Ihandle* ih, const char* value)
{
  (void)ih;

  DataPackage dataPackage;

  if (!value)
  {
    dataPackage.SetText(L"");
  }
  else
  {
    dataPackage.SetText(iupwinuiStringToHString(value));
  }

  Clipboard::SetContent(dataPackage);
  return 0;
}

static char* winuiClipboardGetTextAttrib(Ihandle* ih)
{
  (void)ih;

  DataPackageView content = Clipboard::GetContent();
  if (content.Contains(StandardDataFormats::Text()))
  {
    hstring text = content.GetTextAsync().get();
    return iupwinuiHStringToString(text);
  }

  return NULL;
}

static char* winuiClipboardGetTextAvailableAttrib(Ihandle* ih)
{
  (void)ih;

  DataPackageView content = Clipboard::GetContent();
  int available = content.Contains(StandardDataFormats::Text()) ? 1 : 0;

  return iupStrReturnBoolean(available);
}

/************************** Image **************************/

static char* winuiClipboardGetImageAvailableAttrib(Ihandle* ih)
{
  (void)ih;

  DataPackageView content = Clipboard::GetContent();
  int available = content.Contains(StandardDataFormats::Bitmap()) ? 1 : 0;

  return iupStrReturnBoolean(available);
}

static int winuiClipboardSetImageAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    DataPackage dataPackage;
    Clipboard::SetContent(dataPackage);
    return 0;
  }

  void* handle = iupImageGetImage(value, ih, 0, NULL);
  WriteableBitmap bitmap = winuiGetBitmapFromHandle(handle);
  if (!bitmap)
    return 0;

  int width = bitmap.PixelWidth();
  int height = bitmap.PixelHeight();
  IBuffer pixelBuffer = bitmap.PixelBuffer();

  InMemoryRandomAccessStream stream;

  BitmapEncoder encoder = BitmapEncoder::CreateAsync(BitmapEncoder::PngEncoderId(), stream).get();

  encoder.SetPixelData(
    BitmapPixelFormat::Bgra8,
    BitmapAlphaMode::Premultiplied,
    width,
    height,
    96.0,
    96.0,
    array_view<uint8_t const>(pixelBuffer.data(), pixelBuffer.Length())
  );

  encoder.FlushAsync().get();

  stream.Seek(0);

  RandomAccessStreamReference streamRef = RandomAccessStreamReference::CreateFromStream(stream);

  DataPackage dataPackage;
  dataPackage.SetBitmap(streamRef);
  Clipboard::SetContent(dataPackage);

  return 0;
}

/************************** Native Image **************************/

static HANDLE winuiCopyHandle(HANDLE hHandle)
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

static int winuiClipboardSetNativeImageAttrib(Ihandle* ih, const char* value)
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

static char* winuiClipboardGetNativeImageAttrib(Ihandle* ih)
{
  HANDLE hHandle;

  if (!OpenClipboard(GetForegroundWindow()))
    return NULL;

  hHandle = GetClipboardData(CF_DIB);
  if (!hHandle)
  {
    CloseClipboard();
    return NULL;
  }

  hHandle = winuiCopyHandle(hHandle);
  CloseClipboard();

  (void)ih;
  return (char*)hHandle;
}

/************************** Metafiles **************************/

static char* winuiClipboardGetWMFAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnBoolean(winuiClipboardIsAvailable(CF_METAFILEPICT));
}

static char* winuiClipboardGetEMFAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnBoolean(winuiClipboardIsAvailable(CF_ENHMETAFILE));
}

static int winuiClipboardSetSaveEMFAttrib(Ihandle* ih, const char* value)
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

  std::wstring wpath = iupwinuiStringToWString(value);
  hFile = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    WriteFile(hFile, buffer, dwSize, &nBytesWrite, NULL);
    CloseHandle(hFile);
  }

  free(buffer);

  CloseClipboard();
  return 0;
}

static int winuiClipboardSetSaveWMFAttrib(Ihandle* ih, const char* value)
{
  DWORD dwSize;
  unsigned char* buffer;
  METAFILEPICT* lpMFP;
  HANDLE Handle;
  HANDLE hFile;
  (void)ih;

  OpenClipboard(GetForegroundWindow());
  Handle = GetClipboardData(CF_METAFILEPICT);
  if (Handle == NULL)
  {
    CloseClipboard();
    return 0;
  }

  lpMFP = (METAFILEPICT*)GlobalLock(Handle);

  dwSize = GetMetaFileBitsEx(lpMFP->hMF, 0, NULL);

  buffer = (unsigned char*)malloc(dwSize);

  GetMetaFileBitsEx(lpMFP->hMF, dwSize, buffer);

  std::wstring wpath = iupwinuiStringToWString(value);
  hFile = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    winuiWritePlaceableFile(hFile, buffer, dwSize, lpMFP->mm, lpMFP->xExt, lpMFP->yExt);
    CloseHandle(hFile);
  }

  GlobalUnlock(Handle);
  free(buffer);

  CloseClipboard();
  return 0;
}

/************************** Custom Format **************************/

static int winuiClipboardSetAddFormatAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    std::wstring wformat = iupwinuiStringToWString(value);
    RegisterClipboardFormatW(wformat.c_str());
  }

  (void)ih;
  return 0;
}

static char* winuiClipboardGetFormatAvailableAttrib(Ihandle* ih)
{
  UINT format_id = winuiClipboardGetFormatId(ih);
  if (format_id == 0)
    return NULL;

  return iupStrReturnBoolean(winuiClipboardIsAvailable(format_id));
}

static int winuiClipboardSetFormatDataAttrib(Ihandle* ih, const char* value)
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
  {
    CloseClipboard();
    return 0;
  }

  format_id = winuiClipboardGetFormatId(ih);
  if (format_id == 0)
  {
    CloseClipboard();
    return 0;
  }

  hHandle = GlobalAlloc(GMEM_MOVEABLE, size);
  if (!hHandle)
  {
    CloseClipboard();
    return 0;
  }

  data = GlobalLock(hHandle);
  CopyMemory(data, value, size);
  GlobalUnlock(hHandle);

  SetClipboardData(format_id, hHandle);
  CloseClipboard();

  return 0;
}

static char* winuiClipboardGetFormatDataAttrib(Ihandle* ih)
{
  HANDLE hHandle;
  char* data;
  UINT format_id;
  int size;

  if (!OpenClipboard(GetForegroundWindow()))
    return NULL;

  format_id = winuiClipboardGetFormatId(ih);
  if (format_id == 0)
  {
    CloseClipboard();
    return NULL;
  }

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
  data = iupStrGetMemory(size + 1);

  CopyMemory(data, (char*)GlobalLock(hHandle), size);
  GlobalUnlock(hHandle);

  CloseClipboard();

  iupAttribSetInt(ih, "FORMATDATASIZE", size);
  return data;
}

static int winuiClipboardSetFormatDataStringAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    std::wstring wstr = iupwinuiStringToWString(value);
    int wlen = (int)wstr.length();
    iupAttribSetInt(ih, "FORMATDATASIZE", (int)(sizeof(wchar_t) * (wlen + 1)));
    return winuiClipboardSetFormatDataAttrib(ih, (const char*)wstr.c_str());
  }
  else
    return winuiClipboardSetFormatDataAttrib(ih, NULL);
}

static char* winuiClipboardGetFormatDataStringAttrib(Ihandle* ih)
{
  char* rawData = winuiClipboardGetFormatDataAttrib(ih);
  if (!rawData)
    return NULL;

  int size = iupAttribGetInt(ih, "FORMATDATASIZE");
  wchar_t* wdata = (wchar_t*)rawData;
  int wcharCount = size / (int)sizeof(wchar_t);

  if (wcharCount > 0 && wdata[wcharCount - 1] == 0)
    wcharCount--;

  if (wcharCount == 0)
    return NULL;

  int len = WideCharToMultiByte(CP_UTF8, 0, wdata, wcharCount, NULL, 0, NULL, NULL);
  if (len <= 0)
    return NULL;

  char* str = iupStrGetMemory(len + 1);
  WideCharToMultiByte(CP_UTF8, 0, wdata, wcharCount, str, len, NULL, NULL);
  str[len] = 0;
  return str;
}

/******************************************************************************/

extern "C" Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

extern "C" Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = (char*)"clipboard";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  iupClassRegisterAttribute(ic, "TEXT", winuiClipboardGetTextAttrib, winuiClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", winuiClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEIMAGE", winuiClipboardGetNativeImageAttrib, winuiClipboardSetNativeImageAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winuiClipboardSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", winuiClipboardGetImageAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "WMFAVAILABLE", winuiClipboardGetWMFAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EMFAVAILABLE", winuiClipboardGetEMFAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEEMF", NULL, winuiClipboardSetSaveEMFAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEWMF", NULL, winuiClipboardSetSaveWMFAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, winuiClipboardSetAddFormatAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", winuiClipboardGetFormatAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", winuiClipboardGetFormatDataAttrib, winuiClipboardSetFormatDataAttrib, NULL, NULL, IUPAF_NO_STRING|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", winuiClipboardGetFormatDataStringAttrib, winuiClipboardSetFormatDataStringAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}
