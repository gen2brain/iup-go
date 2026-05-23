/** \file
 * \brief Global attributes (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>

#import <UIKit/UIKit.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_strmessage.h"

#include "iupcocoatouch_drv.h"


static int s_cocoatouch_utf8_autoconvert = 1;

static char* cocoaTouchInfoPlistString(NSString* key)
{
	NSString* s = [[[NSBundle mainBundle] infoDictionary] objectForKey:key];
	if (!s.length) return NULL;
	return iupStrReturnStr([s UTF8String]);
}

IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
	if (iupStrEqual(name, "LANGUAGE"))
	{
		iupStrMessageUpdateLanguage(value);
		return 1;
	}
	if (iupStrEqual(name, "UTF8AUTOCONVERT"))
	{
		s_cocoatouch_utf8_autoconvert = (!value || iupStrBoolean(value)) ? 1 : 0;
		return 0;
	}
	return 1;
}

IUP_SDK_API char* iupdrvGetGlobal(const char* name)
{
	if (iupStrEqual(name, "CURSORPOS"))
	{
		char* str = iupStrGetMemory(50);
		int x = 0, y = 0;
		iupdrvGetCursorPos(&x, &y);
		snprintf(str, 50, "%dx%d", x, y);
		return str;
	}
	if (iupStrEqual(name, "APPID"))
		return cocoaTouchInfoPlistString(@"CFBundleIdentifier");
	if (iupStrEqual(name, "APPNAME"))
	{
		char* s = cocoaTouchInfoPlistString(@"CFBundleDisplayName");
		return s ? s : cocoaTouchInfoPlistString(@"CFBundleName");
	}
	if (iupStrEqual(name, "SHIFTKEY") || iupStrEqual(name, "CONTROLKEY"))
	{
		return "OFF";
	}
	if (iupStrEqual(name, "MODKEYSTATE"))
	{
		char* str = iupStrGetMemory(5);
		iupdrvGetKeyState(str);
		return str;
	}
	if (iupStrEqual(name, "SCREENSIZE"))
	{
		char* str = iupStrGetMemory(50);
		int w = 0, h = 0;
		iupdrvGetScreenSize(&w, &h);
		snprintf(str, 50, "%dx%d", w, h);
		return str;
	}
	if (iupStrEqual(name, "FULLSIZE"))
	{
		char* str = iupStrGetMemory(50);
		int w = 0, h = 0;
		iupdrvGetFullSize(&w, &h);
		snprintf(str, 50, "%dx%d", w, h);
		return str;
	}
	if (iupStrEqual(name, "SCREENDEPTH"))
	{
		char* str = iupStrGetMemory(50);
		snprintf(str, 50, "%d", iupdrvGetScreenDepth());
		return str;
	}
	if (iupStrEqual(name, "VIRTUALSCREEN"))
	{
		char* str = iupStrGetMemory(50);
		int w = 0, h = 0;
		iupdrvGetFullSize(&w, &h);
		snprintf(str, 50, "0 0 %d %d", w, h);
		return str;
	}
	if (iupStrEqual(name, "MONITORSCOUNT"))
	{
		return iupStrReturnInt((int)[[UIScreen screens] count]);
	}
	if (iupStrEqual(name, "MONITORSINFO"))
	{
		NSArray* arr = [UIScreen screens];
		NSUInteger n = [arr count];
		size_t cap = n * 50;
		char* str = iupStrGetMemory((int)cap);
		char* p = str;
		for (UIScreen* s in arr)
		{
			size_t used = (size_t)(p - str);
			size_t avail = (used < cap) ? (cap - used) : 0;
			if (avail == 0) break;
			CGRect f = [s bounds];
			p += snprintf(p, avail, "%d %d %d %d\n",
				(int)f.origin.x, (int)f.origin.y, (int)f.size.width, (int)f.size.height);
		}
		return str;
	}
	if (iupStrEqual(name, "TRUECOLORCANVAS"))
	{
		return iupdrvGetScreenDepth() > 8 ? "YES" : "NO";
	}
	if (iupStrEqual(name, "UTF8AUTOCONVERT"))
	{
		return s_cocoatouch_utf8_autoconvert ? "YES" : "NO";
	}
	if (iupStrEqual(name, "DARKMODE"))
	{
		UIWindow* window = iupCocoaTouchFindCurrentWindow();
		UITraitCollection* trait = window ? window.traitCollection : [UITraitCollection currentTraitCollection];
		return iupStrReturnBoolean(trait.userInterfaceStyle == UIUserInterfaceStyleDark);
	}
	if (iupStrEqual(name, "TOUCHREADY"))
		return iupStrReturnBoolean(1);
	return NULL;
}
