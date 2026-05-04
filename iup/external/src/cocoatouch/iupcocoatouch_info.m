/** \file
 * \brief iOS UIKit System Information
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <langinfo.h>

#import <UIKit/UIKit.h>
#import <os/log.h>

#include "iup.h"
#include "iup_varg.h"

#include "iup_str.h"
#include "iup_drvinfo.h"

static NSString* cocoaTouchAppSupportPath(NSString* app_name)
{
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	if ([paths count] == 0)
	{
		return nil;
	}
	NSString* sub = app_name;
	if (sub.length == 0)
	{
		sub = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleIdentifier"];
	}
	if (sub.length == 0)
	{
		sub = @"IUP";
	}
	return [[paths objectAtIndex:0] stringByAppendingPathComponent:sub];
}

IUP_SDK_API void iupdrvAddScreenOffset(int* x, int* y, int add)
{
	(void)x; (void)y; (void)add;
}

IUP_SDK_API void iupdrvGetScreenSize(int* width, int* height)
{
	CGRect rect = [[UIScreen mainScreen] bounds];
	if (width)  *width  = (int)rect.size.width;
	if (height) *height = (int)rect.size.height;
}

IUP_SDK_API void iupdrvGetFullSize(int* width, int* height)
{
	iupdrvGetScreenSize(width, height);
}

IUP_SDK_API int iupdrvGetScreenDepth(void)
{
	return 32;
}

IUP_SDK_API double iupdrvGetScreenDpi(void)
{
	return [[UIScreen mainScreen] scale] * 72.0;
}

IUP_SDK_API int iupdrvScaleNaturalPx(int px)
{
	/* UIKit layout is in points; Retina scale is rendering-only */
	return px;
}

extern void iupCocoaTouchGetLastTouchScreen(CGPoint* out);

IUP_SDK_API void iupdrvGetCursorPos(int* x, int* y)
{
	CGPoint p;
	iupCocoaTouchGetLastTouchScreen(&p);
	if (x) *x = (int)p.x;
	if (y) *y = (int)p.y;
}

IUP_SDK_API void iupdrvGetKeyState(char* key)
{
	if (!key) return;
	key[0] = ' ';
	key[1] = ' ';
	key[2] = ' ';
	key[3] = ' ';
	key[4] = 0;
}

IUP_SDK_API char* iupdrvGetSystemName(void)
{
	return "iOS";
}

IUP_SDK_API char* iupdrvGetSystemVersion(void)
{
	NSString* version = [[NSProcessInfo processInfo] operatingSystemVersionString];
	const char* c_str = [version UTF8String];
	size_t len = strlen(c_str);
	char* out = iupStrGetMemory((int)len);
	strlcpy(out, c_str, len + 1);
	return out;
}

IUP_SDK_API char* iupdrvGetComputerName(void)
{
	char host[_POSIX_HOST_NAME_MAX + 1];
	if (gethostname(host, _POSIX_HOST_NAME_MAX) != 0)
	{
		host[0] = 0;
	}
	size_t len = strlen(host);
	char* out = iupStrGetMemory((int)len);
	strlcpy(out, host, len + 1);
	return out;
}

IUP_SDK_API char* iupdrvGetUserName(void)
{
	NSString* user = NSFullUserName();
	const char* c_str = [user UTF8String];
	size_t len = strlen(c_str);
	char* out = iupStrGetMemory((int)len);
	strlcpy(out, c_str, len + 1);
	return out;
}

IUP_SDK_API int iupdrvGetPreferencePath(char* filename, const char* app_name, int use_system)
{
	(void)use_system;
	filename[0] = '\0';

	NSString* ns_name = app_name ? [NSString stringWithUTF8String:app_name] : @"";
	NSString* dir = cocoaTouchAppSupportPath(ns_name);
	if (!dir)
	{
		return 0;
	}

	NSError* err = nil;
	if (![[NSFileManager defaultManager] createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:&err])
	{
		return 0;
	}

	NSString* file = [dir stringByAppendingPathComponent:@"config"];
	const char* c_path = [file fileSystemRepresentation];
	if (!c_path)
	{
		return 0;
	}
	iupStrCopyN(filename, 10240, c_path);
	return 1;
}

IUP_SDK_API char* iupdrvGetCurrentDirectory(void)
{
	char buf[PATH_MAX];
	if (!getcwd(buf, sizeof(buf)))
	{
		return NULL;
	}
	size_t len = strlen(buf);
	char* out = iupStrGetMemory((int)len);
	strlcpy(out, buf, len + 1);
	return out;
}

IUP_SDK_API int iupdrvSetCurrentDirectory(const char* dir)
{
	return chdir(dir) == 0 ? 1 : 0;
}

IUP_API void IupLogV(const char* type, const char* format, va_list arglist)
{
	os_log_type_t level = OS_LOG_TYPE_DEFAULT;
	if (iupStrEqualNoCase(type, "ERROR") ||
	    iupStrEqualNoCase(type, "WARNING")) level = OS_LOG_TYPE_ERROR;
	else if (iupStrEqualNoCase(type, "CRITICAL") ||
	         iupStrEqualNoCase(type, "ALERT") ||
	         iupStrEqualNoCase(type, "EMERGENCY")) level = OS_LOG_TYPE_FAULT;

	NSString* fmt = [NSString stringWithUTF8String:format];
	NSString* msg = [[[NSString alloc] initWithFormat:fmt arguments:arglist] autorelease];
	const char* tag = (type && *type) ? type : "INFO";
	os_log_with_type(OS_LOG_DEFAULT, level, "[Iup %{public}s] %{public}s", tag, [msg UTF8String]);
}

IUP_API void IupLog(const char* type, const char* format, ...)
{
	va_list arglist;
	va_start(arglist, format);
	IupLogV(type, format, arglist);
	va_end(arglist);
}

IUP_SDK_API char* iupdrvLocaleInfo(void)
{
	return iupStrReturnStr(nl_langinfo(CODESET));
}
