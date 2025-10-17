/** \file
 * \brief macOS System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <os/log.h>

#include <sys/utsname.h>
#include <unistd.h>
#include <limits.h>
#include <langinfo.h>

#include "iup.h"
#include "iup_varg.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_class.h"
#include "iup_classbase.h"


IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
  NSString *curDir = [[NSFileManager defaultManager] currentDirectoryPath];
  const char *dir = [curDir UTF8String];
  size_t size = strlen(dir) + 1;
  char *buffer = (char *)iupStrGetMemory(size);
  strcpy(buffer, dir);
  return buffer;
}

IUP_SDK_API int iupdrvSetCurrentDirectory(const char* dir)
{
  NSString *path = [NSString stringWithUTF8String:dir];
  BOOL r = [[NSFileManager defaultManager] changeCurrentDirectoryPath:path];
  return (r ? 1 : 0);
}

int iupdrvMakeDirectory(const char* name)
{
  NSString *path = [NSString stringWithUTF8String:name];
  NSDictionary *dic = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithUnsignedInt:0775], NSFilePosixPermissions, nil];
  BOOL r = [[NSFileManager defaultManager] createDirectoryAtPath:path withIntermediateDirectories:YES attributes:dic error:NULL];
  return (r ? 1 : 0);
}

int iupdrvIsFile(const char* name)
{
  NSString *path = [NSString stringWithUTF8String:name];
  BOOL isDir;
  BOOL r = [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDir];
  return (r && !isDir ? 1 : 0);
}

int iupdrvIsDirectory(const char* name)
{
  NSString *path = [NSString stringWithUTF8String:name];
  BOOL isDir;
  BOOL r = [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDir];
  return (r && isDir ? 1 : 0);
}

int iupdrvGetWindowDecor(void* wnd, int *border, int *caption)
{
  (void)wnd;

  NSRect contentRect = NSMakeRect(0, 0, 200, 200);
  NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;

  NSRect frameRect = [NSWindow frameRectForContentRect:contentRect styleMask:style];

  if (border)
  {
    *border = iupROUND((frameRect.size.width - contentRect.size.width) / 2.0);
  }
  if (caption)
  {
    *caption = iupROUND(frameRect.size.height - contentRect.size.height);
  }

  return 1;
}

void iupdrvAddScreenOffset(int *x, int *y, int add)
{
  NSScreen *screen = [NSScreen mainScreen];
  if (screen)
  {
    NSRect frame = [screen frame];
    NSRect visibleFrame = [screen visibleFrame];

    /* In IUP's top-left coordinate system, the offset is the top-left corner of the visible area (work area). */
    int offset_x = visibleFrame.origin.x;
    /* The y-offset is the height of the main menu bar. */
    int offset_y = frame.size.height - (visibleFrame.origin.y + visibleFrame.size.height);

    if (add)
    {
      if (x) *x += offset_x;
      if (y) *y += offset_y;
    }
    else
    {
      if (x) *x -= offset_x;
      if (y) *y -= offset_y;
    }
  }
}

void iupdrvGetScreenSize(int *width, int *height)
{
  /* Returns the usable screen area, excluding the menu bar and dock. */
  NSRect screen_rect = [[NSScreen mainScreen] visibleFrame];
  if (width) *width = (int)screen_rect.size.width;
  if (height) *height = (int)screen_rect.size.height;
}

void iupdrvGetFullSize(int *width, int *height)
{
  NSRect screen_rect = [[NSScreen mainScreen] frame];
  if (width) *width = (int)screen_rect.size.width;
  if (height) *height = (int)screen_rect.size.height;
}

int iupdrvGetScreenDepth(void)
{
  NSScreen* screen = [NSScreen mainScreen];
  if (screen)
  {
    NSDictionary* deviceDescription = [screen deviceDescription];
    NSNumber* bitsPerSample = [deviceDescription objectForKey:@"NSDeviceBitsPerSample"];
    if (bitsPerSample)
    {
      int bits = [bitsPerSample intValue];
      return bits * 4;
    }
  }
  return 32;
}

double iupdrvGetScreenDpi(void)
{
  NSScreen* screen = [NSScreen mainScreen];
  if (screen != nil)
  {
    NSDictionary* deviceDescription = [screen deviceDescription];
    NSSize resolution = [[deviceDescription objectForKey:NSDeviceResolution] sizeValue];
    return resolution.height;
  }
  return 72.0;
}

void iupdrvGetCursorPos(int *x, int *y)
{
  /* [NSEvent mouseLocation] origin is bottom-left of the primary screen. */
  NSPoint mouse_point = [NSEvent mouseLocation];

  /* Invert the y-axis for IUP's top-left origin system. */
  NSRect screen_rect = [[NSScreen mainScreen] frame];
  CGFloat inverted_y = screen_rect.size.height - mouse_point.y;

  if (x) *x = iupROUND(mouse_point.x);
  if (y) *y = iupROUND(inverted_y);
}

void iupdrvGetKeyState(char* key)
{
  NSEventModifierFlags flags = [NSEvent modifierFlags];

  key[0] = (flags & NSEventModifierFlagShift)   ? 'S' : ' ';
  key[1] = (flags & NSEventModifierFlagControl) ? 'C' : ' ';
  key[2] = (flags & NSEventModifierFlagOption)  ? 'A' : ' '; /* Alt Key */
  key[3] = (flags & NSEventModifierFlagCommand) ? 'Y' : ' '; /* System Key */
  key[4] = 0;
}

char *iupdrvGetSystemName(void)
{
  NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
  const char* codename = NULL;

  /* Starting with macOS 11 (Big Sur), the major version number is the primary identifier. */
  if (version.majorVersion >= 11)
  {
    switch (version.majorVersion)
    {
      case 26: codename = "Tahoe"; break;
      case 15: codename = "Sequoia"; break;
      case 14: codename = "Sonoma"; break;
      case 13: codename = "Ventura"; break;
      case 12: codename = "Monterey"; break;
      case 11: codename = "Big Sur"; break;
    }
  }
  /* For older macOS versions (10.x), check the minor version. */
  else if (version.majorVersion == 10)
  {
    switch (version.minorVersion)
    {
      case 15: codename = "Catalina"; break;
      case 14: codename = "Mojave"; break;
      case 13: codename = "High Sierra"; break;
      case 12: codename = "Sierra"; break;
      case 11: codename = "El Capitan"; break;
      case 10: codename = "Yosemite"; break;
    }
  }

  char buffer[100];
  if (codename)
  {
    sprintf(buffer, "macOS %s", codename);
  }
  else
  {
    strcpy(buffer, "macOS");
  }

  char* iup_str = iupStrGetMemory((int)strlen(buffer) + 1);
  strcpy(iup_str, buffer);
  return iup_str;
}

char *iupdrvGetSystemVersion(void)
{
  char* str = iupStrGetMemory(100);

  NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
  sprintf(str, "%ld.%ld.%ld", (long)version.majorVersion, (long)version.minorVersion, (long)version.patchVersion);

  /* Append architecture info. */
  struct utsname systemInfo;
  if (uname(&systemInfo) == 0)
  {
    strcat(str, " (");
    strcat(str, systemInfo.machine);
    strcat(str, ")");
  }

  return str;
}

char *iupdrvGetComputerName(void)
{
  NSString* computer_name = [(NSString *)SCDynamicStoreCopyComputerName(NULL, NULL) autorelease];
  if (!computer_name)
  {
    return NULL;
  }

  const char* c_str = [computer_name UTF8String];
  if (!c_str)
  {
    return NULL;
  }

  char* iup_str = iupStrGetMemory((int)strlen(c_str) + 1);
  strcpy(iup_str, c_str);

  return iup_str;
}

char *iupdrvGetUserName(void)
{
  NSString* user_name = NSUserName();
  if (!user_name)
  {
    return NULL;
  }

  const char* c_str = [user_name UTF8String];
  if (!c_str)
  {
    return NULL;
  }

  char* iup_str = iupStrGetMemory((int)strlen(c_str) + 1);
  strcpy(iup_str, c_str);

  return iup_str;
}

int iupdrvGetPreferencePath(char *filename, int use_system)
{
  (void)use_system;

  NSArray* support_paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  if([support_paths count] > 0)
  {
    NSString* bundle_name = [[NSBundle mainBundle] bundleIdentifier];
    if (!bundle_name)
    {
      bundle_name = [[NSProcessInfo processInfo] processName];
    }

    NSString* ns_path = [[support_paths objectAtIndex:0] stringByAppendingPathComponent:bundle_name];

    BOOL is_dir = NO;
    if (![[NSFileManager defaultManager] fileExistsAtPath:ns_path isDirectory:&is_dir])
    {
      NSError* the_error = nil;
      if (![[NSFileManager defaultManager] createDirectoryAtPath:ns_path withIntermediateDirectories:YES attributes:nil error:&the_error])
      {
        os_log_error(OS_LOG_DEFAULT, "Failed to create preference directory: %{public}@", the_error);
        filename[0] = '\0';
        return 0;
      }
    }

    const char* c_path = [ns_path fileSystemRepresentation];
    if(NULL != c_path)
    {
      strcpy(filename, c_path);
      strcat(filename, "/");
      return 1;
    }
  }

  filename[0] = '\0';
  return 0;
}


char* iupdrvLocaleInfo(void)
{
  return iupStrReturnStr(nl_langinfo(CODESET));
}

void IupLogV(const char* type, const char* format, va_list arglist)
{
  char buffer[2048];
  vsnprintf(buffer, sizeof(buffer), format, arglist);

  os_log_type_t log_type = OS_LOG_TYPE_DEFAULT;
  if (iupStrEqualNoCase(type, "DEBUG"))
    log_type = OS_LOG_TYPE_DEBUG;
  else if (iupStrEqualNoCase(type, "INFO"))
    log_type = OS_LOG_TYPE_INFO;
  else if (iupStrEqualNoCase(type, "WARNING"))
    log_type = OS_LOG_TYPE_ERROR;
  else if (iupStrEqualNoCase(type, "ERROR"))
    log_type = OS_LOG_TYPE_ERROR;
  else if (iupStrEqualNoCase(type, "CRITICAL") || iupStrEqualNoCase(type, "ALERT") || iupStrEqualNoCase(type, "EMERGENCY"))
    log_type = OS_LOG_TYPE_FAULT;

  /* os_log requires a static format string for privacy, so we must mark dynamic content as public. */
  os_log_with_type(OS_LOG_DEFAULT, log_type, "%{public}s", buffer);
}

void IupLog(const char* type, const char* format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  IupLogV(type, format, arglist);
  va_end(arglist);
}
