/** \file
 * \brief Global attributes (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>
#include <stdatomic.h>

#import <UIKit/UIKit.h>
#import <Network/Network.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_strmessage.h"

#include "iupcocoatouch_drv.h"


static int s_cocoatouch_utf8_autoconvert = 1;
static nw_path_monitor_t s_cocoatouch_network_monitor = NULL;
static _Atomic unsigned int s_cocoatouch_network_index = 0;

IUP_SDK_API unsigned int iupCocoaTouchGetNetworkIndex(void)
{
	return atomic_load(&s_cocoatouch_network_index);
}

static void cocoaTouchSetNetwork(const char* value)
{
	nw_interface_type_t type;

	if (s_cocoatouch_network_monitor)
	{
		nw_path_monitor_cancel(s_cocoatouch_network_monitor);
		nw_release(s_cocoatouch_network_monitor);
		s_cocoatouch_network_monitor = NULL;
	}
	atomic_store(&s_cocoatouch_network_index, 0);

	if (iupStrEqualNoCase(value, "CELLULAR"))
		type = nw_interface_type_cellular;
	else if (iupStrEqualNoCase(value, "WIFI"))
		type = nw_interface_type_wifi;
	else
		return;

	nw_path_monitor_t monitor = nw_path_monitor_create_with_type(type);
	nw_path_monitor_set_queue(monitor, dispatch_get_main_queue());
	nw_path_monitor_set_update_handler(monitor, ^(nw_path_t path) {
		__block unsigned int index = 0;
		if (monitor == s_cocoatouch_network_monitor && nw_path_get_status(path) == nw_path_status_satisfied)
		{
			nw_path_enumerate_interfaces(path, ^bool(nw_interface_t iface) {
				index = nw_interface_get_index(iface);
				return false;
			});
		}
		if (monitor == s_cocoatouch_network_monitor)
			atomic_store(&s_cocoatouch_network_index, index);
	});
	s_cocoatouch_network_monitor = monitor;
	nw_path_monitor_start(monitor);
}

static char* cocoaTouchInfoPlistString(NSString* key)
{
	NSString* s = [[[NSBundle mainBundle] infoDictionary] objectForKey:key];
	if (!s.length) return NULL;
	return iupStrReturnStr([s UTF8String]);
}

static void cocoaTouchHaptic(const char* value)
{
	if (iupStrEqualNoCase(value, "SELECTION"))
	{
		UISelectionFeedbackGenerator* generator = [[UISelectionFeedbackGenerator alloc] init];
		[generator selectionChanged];
		[generator release];
	}
	else
	{
		UIImpactFeedbackStyle style;
		UIImpactFeedbackGenerator* generator;

		if (iupStrEqualNoCase(value, "LIGHT"))
			style = UIImpactFeedbackStyleLight;
		else if (iupStrEqualNoCase(value, "MEDIUM"))
			style = UIImpactFeedbackStyleMedium;
		else if (iupStrEqualNoCase(value, "HEAVY"))
			style = UIImpactFeedbackStyleHeavy;
		else
			return;

		generator = [[UIImpactFeedbackGenerator alloc] initWithStyle:style];
		[generator impactOccurred];
		[generator release];
	}
}

static int cocoaTouchSetGlobal(const char* name, const char* value)
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
	if (iupStrEqual(name, "NETWORK"))
	{
		cocoaTouchSetNetwork(value);
		return 1;
	}
	if (iupStrEqual(name, "HAPTIC"))
	{
		cocoaTouchHaptic(value);
		return 0;
	}
	return 1;
}

IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
	@autoreleasepool
	{
		return cocoaTouchSetGlobal(name, value);
	}
}

static char* cocoaTouchGetGlobal(const char* name)
{
	if (iupStrEqual(name, "NETWORKHANDLE"))
	{
		unsigned int index = iupCocoaTouchGetNetworkIndex();
		return index ? iupStrReturnInt((int)index) : NULL;
	}
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
	if (iupStrEqual(name, "TOUCHREADY"))
		return iupStrReturnBoolean(1);
	return NULL;
}

IUP_SDK_API char* iupdrvGetGlobal(const char* name)
{
	@autoreleasepool
	{
		return cocoaTouchGetGlobal(name);
	}
}
