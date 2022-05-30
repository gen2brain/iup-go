/** \file
 * \brief MAC OS System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

/* This module should depend only on IUP core headers and
   Mac OS Cocoa system headers. */

#import <Cocoa/Cocoa.h>

#include <sys/utsname.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#include "iup.h" 

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"

char* iupdrvGetCurrentDirectory(void)
{
	NSString *curDir = [[NSFileManager defaultManager] currentDirectoryPath];
	const char *dir = [curDir UTF8String];
	size_t size = strlen(dir)+1;
	char *buffer = (char *)iupStrGetMemory(size);
	strcpy(buffer,dir);
	return buffer;
}

int iupdrvSetCurrentDirectory(const char* dir)
{
	NSString *path = [NSString stringWithUTF8String:dir];
	Boolean r = [[NSFileManager defaultManager] changeCurrentDirectoryPath: path];
	return (r ? 1 : 0);
}

int iupdrvMakeDirectory(const char* name) 
{
	NSString *path = [NSString stringWithUTF8String:name];
	NSDictionary *dic = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithUnsignedInt:0775],NSFilePosixPermissions,nil];
	Boolean r = [[NSFileManager defaultManager] createDirectoryAtPath:path withIntermediateDirectories:YES attributes:dic error:NULL];
	return (r ? 1 : 0);
}

int iupdrvIsFile(const char* name)
{
	NSString *path = [NSString stringWithUTF8String:name];
	Boolean isDir;
	Boolean r = [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDir];
	return (r && !isDir ? 1 : 0);
}

int iupdrvIsDirectory(const char* name)
{
	NSString *path = [NSString stringWithUTF8String:name];
	Boolean isDir;
	Boolean r = [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDir];
	return (r && isDir ? 1 : 0);
}

int iupdrvGetWindowDecor(void* wnd, int *border, int *caption)
{
	NSScreen *screen = [NSScreen mainScreen];
	if(screen!=nil) {
	  NSRect frame = [screen frame];
	  NSRect visibleFrame = [screen visibleFrame];
	  *border = visibleFrame.origin.x;
  	  *caption = frame.size.height - visibleFrame.origin.y - visibleFrame.size.height;
 	  return 1;
	}
	*border = 0;
	*caption = 0;
	return 0;
 }

void iupdrvGetScreenSize(int *width, int *height)
{
	NSScreen *screen = [NSScreen mainScreen];
	NSRect frame = [screen visibleFrame];
	*width = frame.size.width;
	*height = frame.size.height;
}

void iupdrvGetFullSize(int *width, int *height)
{
	NSScreen *screen = [NSScreen mainScreen];
	NSRect frame = [screen frame];
	*width = frame.size.width;
	*height = frame.size.height;	
}

int iupdrvGetScreenDepth(void)
{
	return NSBitsPerPixelFromDepth([[NSScreen mainScreen] depth]);
}

void iupdrvGetCursorPos(int *x, int *y)
{
	NSPoint mouseLoc;
	mouseLoc = [NSEvent mouseLocation]; //get current mouse position
	NSLog(@"Mouse location: %f %f", mouseLoc.x, mouseLoc.y);
	*x = (int)mouseLoc.x;
	*y = (int)mouseLoc.y;
}

void iupdrvGetKeyState(char* key)
{
	CGEventFlags flags =CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState);
	if (flags & kCGEventFlagMaskShift)
    key[0] = 'S';
  else
    key[0] = ' ';

  if (flags & kCGEventFlagMaskControl)
    key[1] = 'C';
  else
    key[1] = ' ';

  if (flags & kCGEventFlagMaskAlternate)
    key[2] = 'A';
  else
    key[2] = ' ';

  if (flags & kCGEventFlagMaskCommand)
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

char *iupdrvGetSystemVersion(void)
{
  char* str = (char*)iupStrGetMemory(70); 
  SInt32 systemVersion, versionMajor, versionMinor, versionBugFix, systemArchitecture;

  if (Gestalt(gestaltSystemVersion, &systemVersion) != noErr)
  {
    printf("Unable to obtain system version\n");
    return NULL;
  }

  if (systemVersion < 0x1040)
  {
    /* Major, Minor, Bug fix */
    sprintf(str, "%d.%d.%d", ((systemVersion & 0xF000) >> 12) * 10 + ((systemVersion & 0x0F00) >> 8),
                                ((systemVersion & 0x00F0) >> 4), (systemVersion & 0x000F));
  }
  else  /* MAC_OS_X_VERSION_10_4 or later */
  {
    Gestalt(gestaltSystemVersionMajor,  &versionMajor);
    Gestalt(gestaltSystemVersionMinor,  &versionMinor);
    Gestalt(gestaltSystemVersionBugFix, &versionBugFix);
    
    sprintf(str, "%d.%d.%d", versionMajor, versionMinor, versionBugFix);
  }

  if(Gestalt(gestaltSysArchitecture, &systemArchitecture) == noErr)
  {
    if (systemArchitecture == gestalt68k)
      sprintf(str, "%s %s", str, "(Motorola 68k)");
    else if (systemArchitecture == gestaltPowerPC)
      sprintf(str, "%s %s", str, "(Power PC)");
    else /* gestaltIntel */
      sprintf(str, "%s %s", str, "(Intel)");  
  }

  return str;
}

char *iupdrvGetComputerName(void)
{
  char* str = iupStrGetMemory(50);
  CFStringRef computerName = CSCopyMachineName();
  CFStringGetCString(computerName, str, 50, kCFStringEncodingUTF8);
  CFRelease(computerName);
  return str;
}

char *iupdrvGetUserName(void)
{
  char* str = (char*)iupStrGetMemory(50);
  CFStringRef userName = CSCopyUserName(TRUE);  /* TRUE = login name   FALSE = user name */
  CFStringGetCString(userName, str, 50, kCFStringEncodingUTF8);
  CFRelease(userName);
  return str;
}

