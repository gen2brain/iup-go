/** \file
 * \brief MAC OS System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

/* This module should depend only on IUP core headers and
   Mac OS Carbon system headers. */

#include <Carbon/Carbon.h>

#include <sys/utsname.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <langinfo.h>

#include <gtk/gtk.h>

#include "iup.h" 

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"

#define IUP_MAC_ERROR -1

#if 0
static void iupMacStrToUniChar(const char* buffer, UniChar* outBuf, long length, long* outLen)
{
  CFStringRef stringRef = CFStringCreateWithCString(NULL, buffer, kCFStringEncodingUTF8);
  
  CFStringGetCharacters (stringRef, CFRangeMake(0, CFStringGetLength(stringRef)), outBuf);
  *outLen = (long) CFStringGetLength (stringRef);
    
  CFRelease(stringRef);
}
#endif

static int iMacIsFolder(const char* name)
{
  FSRef refName;
  Boolean isFolder;

  if(FSPathMakeRef((const UInt8*)name, &refName, &isFolder) != noErr)
    return IUP_MAC_ERROR;
    
  return isFolder;
}

IUP_SDK_API int iupdrvGetWindowDecor(void* wnd, int *border, int *caption)
{
  Rect rect;
  CGRect cg;
  int minX, minY;

  CGDirectDisplayID mainDisplayID = CGMainDisplayID();
#ifdef OLD_MAC_INFO
  GDHandle hGDev;
  DMGetGDeviceByDisplayID((DisplayIDType)mainDisplayID, &hGDev, false);
  GetAvailableWindowPositioningBounds(hGDev, &rect);
#else
  HIWindowGetAvailablePositioningBounds(mainDisplayID, kHICoordSpaceScreenPixel, &rect);
#endif

  cg = CGRectMake(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
  
  minX = (int)CGRectGetMinX(cg);
  minY = (int)CGRectGetMinY(cg);
  
  if (minX >= 0 && minY >= 0 && (minY >= minX))
  {
    *border  = minX;
    *caption = minY - *border;
  
    return 1;
  }

  *border  = 0;
  *caption = 0;

  return 0;
}

IUP_SDK_API void iupdrvAddScreenOffset(int *x, int *y, int add)
{
  /* ?????? */
}

IUP_SDK_API void iupdrvGetScreenSize(int *width, int *height)
{
  int w_size = CGDisplayPixelsWide(kCGDirectMainDisplay);
  int h_size = CGDisplayPixelsHigh(kCGDirectMainDisplay);

  *width  = w_size;
  *height = h_size;
}

IUP_SDK_API void iupdrvGetFullSize(int *width, int *height)
{
  CGRect rect;

  rect = CGDisplayBounds(kCGDirectMainDisplay);

  *width  = (int)CGRectGetWidth(rect);
  *height = (int)CGRectGetHeight(rect);
}

IUP_SDK_API int iupdrvGetScreenDepth(void)
{
  return CGDisplayBitsPerPixel(kCGDirectMainDisplay);  /* Deprecated in Mac OS X v10.6 */
}

IUP_SDK_API double iupdrvGetScreenDpi(void)
{
  CGRect rect = CGDisplayBounds(kCGDirectMainDisplay);
  int height = (int)CGRectGetHeight(rect);   /* pixels */
  CGSize size = CGDisplayScreenSize(kCGDirectMainDisplay);  /* millimeters */
  return ((double)height / size.height) * 25.4;  /* mm to inch */
}

IUP_SDK_API void iupdrvGetCursorPos(int *x, int *y)
{
  CGPoint point;
#ifdef OLD_MAC_INFO
  Point pnt;
  GetMouse(&pnt);
  point = CGPointMake(pnt.h, pnt.v);
#else
  HIGetMousePosition(kHICoordSpaceScreenPixel, NULL, &point);
#endif
  
  *x = (int)point.x;
  *y = (int)point.y;
}

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
  if (GetCurrentEventKeyModifiers() & shiftKey)
    key[0] = 'S';
  else
    key[0] = ' ';
  if (GetCurrentEventKeyModifiers() & controlKey)
    key[1] = 'C';
  else
    key[1] = ' ';
  if (GetCurrentEventKeyModifiers() & optionKey)
    key[2] = 'A';
  else
    key[2] = ' ';
  if (GetCurrentEventKeyModifiers() & cmdKey)
    key[3] = 'Y';
  else
    key[3] = ' ';
  
  key[4] = 0;
}

char *iupdrvGetSystemName(void)
{
  SInt32 systemVersion;

  if (Gestalt(gestaltSystemVersion, &systemVersion) == noErr)
  {
    if (systemVersion >= 0x1060)
      return "Snow Leopard";
    else if (systemVersion >= 0x1050)
      return "Leopard";
    else if (systemVersion >= 0x1040)
      return "Tiger";
    else if (systemVersion >= 0x1030)
      return "Panther";
    else if (systemVersion >= 0x1020)
      return "Jaguar";
    else if (systemVersion >= 0x1010)
      return "Puma";
    else if (systemVersion >= 0x1000)
      return "Cheetah";
  }

  return "MacOS";
}

IUP_SDK_API char *iupdrvGetSystemVersion(void)
{
  char* str = iupStrGetMemory(100);
  SInt32 systemVersion, versionMajor, versionMinor, versionBugFix, systemArchitecture;

  if (Gestalt(gestaltSystemVersion, &systemVersion) != noErr)
    return NULL;

  if (systemVersion < 0x1040)
  {
    /* Major, Minor, Bug fix */
    sprintf(str, "%ld.%ld.%ld", (((long)systemVersion & 0xF000) >> 12) * 10 + (((long)systemVersion & 0x0F00) >> 8),
                                (((long)systemVersion & 0x00F0) >> 4), ((long)systemVersion & 0x000F));
  }
  else  /* MAC_OS_X_VERSION_10_4 or later */
  {
    Gestalt(gestaltSystemVersionMajor,  &versionMajor);
    Gestalt(gestaltSystemVersionMinor,  &versionMinor);
    Gestalt(gestaltSystemVersionBugFix, &versionBugFix);
    
    sprintf(str, "%ld.%ld.%ld", (long)versionMajor, (long)versionMinor, (long)versionBugFix);
  }

  if (Gestalt(gestaltSysArchitecture, &systemArchitecture) == noErr)
  {
    if (systemArchitecture == gestalt68k)
      strcat(str, " (Motorola 68k)");
    else if (systemArchitecture == gestaltPowerPC)
      strcat(str, " (Power PC)");
    else /* gestaltIntel */
      strcat(str, " (Intel)");  
  }

  return str;
}

IUP_SDK_API char *iupdrvGetComputerName(void)
{
  char* str = iupStrGetMemory(50);
  CFStringRef computerName = CSCopyMachineName();
  CFStringGetCString(computerName, str, 50, kCFStringEncodingUTF8);
  return str;
}

IUP_SDK_API char *iupdrvGetUserName(void)
{
  char* str = iupStrGetMemory(50);
  CFStringRef userName = CSCopyUserName(TRUE);  /* TRUE = login name   FALSE = user name */
  CFStringGetCString(userName, str, 50, kCFStringEncodingUTF8);
  return str;
}

IUP_SDK_API char* iupdrvLocaleInfo(void)
{
  return iupStrReturnStr(nl_langinfo(CODESET));
}

IUP_SDK_API int iupdrvGetPreferencePath(char *filename, int use_system)
{
  char* home = getenv("HOME");
  if (home)
  {
    (void)use_system; /* unused */
    strcpy(filename, home);
    strcat(filename, "/");
    return 1;
  }
  else
  {
    filename[0] = '\0';
    return 0;
  }
}
