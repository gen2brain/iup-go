/** \file
 * \brief iOS UIKit Driver Core
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>
#include <unistd.h>

#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import <os/log.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_globalattrib.h"
#include "iup_object.h"
#include "iup_str.h"

#include "iupcocoatouch_drv.h"

#import "IupViewController.h"


static dispatch_source_t s_stdout_src = NULL;
static dispatch_source_t s_stderr_src = NULL;

/* sideloaded apps drop stdout/stderr; pipe them through os_log so printf surfaces */
static dispatch_source_t cocoaTouchMakeStdioSource(int fd, const char* tag, os_log_type_t level)
{
	dispatch_source_t src = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, fd, 0,
	    dispatch_get_global_queue(QOS_CLASS_UTILITY, 0));
	dispatch_source_set_event_handler(src, ^{
		char buf[1024];
		ssize_t n = read(fd, buf, sizeof(buf) - 1);
		if (n <= 0) return;
		if (buf[n - 1] == '\n') n--;
		buf[n] = '\0';
		os_log_with_type(OS_LOG_DEFAULT, level, "[Iup %{public}s] %{public}s", tag, buf);
	});
	dispatch_resume(src);
	return src;
}

static void cocoaTouchRedirectStdio(void)
{
	static int done = 0;
	if (done) return;
	done = 1;

	setvbuf(stdout, NULL, _IOLBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	int out_pipe[2], err_pipe[2];
	if (pipe(out_pipe) == 0)
	{
		dup2(out_pipe[1], STDOUT_FILENO);
		close(out_pipe[1]);
		s_stdout_src = cocoaTouchMakeStdioSource(out_pipe[0], "Stdout", OS_LOG_TYPE_DEFAULT);
	}
	if (pipe(err_pipe) == 0)
	{
		dup2(err_pipe[1], STDERR_FILENO);
		close(err_pipe[1]);
		s_stderr_src = cocoaTouchMakeStdioSource(err_pipe[0], "Stderr", OS_LOG_TYPE_ERROR);
	}
}

IUP_SDK_API void* iupdrvGetDisplay(void)
{
	return NULL;  /* no X11-style display handle on iOS */
}

static void cocoaTouchGlobalSetFromUIColor(const char* attrib, UIColor* color)
{
	if (!color) return;
	CGFloat r = 0, g = 0, b = 0, a = 0;
	if (![color getRed:&r green:&g blue:&b alpha:&a]) return;
	iupGlobalSetDefaultColorAttrib(attrib,
		(int)(r * 255), (int)(g * 255), (int)(b * 255));
}

static void cocoaTouchUpdateGlobalColors(void)
{
	UIWindow* window = iupCocoaTouchFindCurrentWindow();
	UITraitCollection* trait = window ? window.traitCollection : [UITraitCollection currentTraitCollection];
	UIColor* fg_color = [[UIColor labelColor]                     resolvedColorWithTraitCollection:trait];
	UIColor* bg_color = [[UIColor systemBackgroundColor]          resolvedColorWithTraitCollection:trait];
	UIColor* txt_bg   = [[UIColor secondarySystemBackgroundColor] resolvedColorWithTraitCollection:trait];
	UIColor* menu_bg  = [[UIColor tertiarySystemBackgroundColor]  resolvedColorWithTraitCollection:trait];
	UIColor* link_fg  = [[UIColor linkColor]                      resolvedColorWithTraitCollection:trait];

	if (fg_color)  cocoaTouchGlobalSetFromUIColor("DLGFGCOLOR",  fg_color);
	else           iupGlobalSetDefaultColorAttrib("DLGFGCOLOR",    0,   0,   0);

	if (bg_color)  cocoaTouchGlobalSetFromUIColor("DLGBGCOLOR",  bg_color);
	else           iupGlobalSetDefaultColorAttrib("DLGBGCOLOR",  237, 237, 237);

	if (txt_bg)    cocoaTouchGlobalSetFromUIColor("TXTBGCOLOR",  txt_bg);
	else           iupGlobalSetDefaultColorAttrib("TXTBGCOLOR",  255, 255, 255);

	if (fg_color)  cocoaTouchGlobalSetFromUIColor("TXTFGCOLOR",  fg_color);
	else           iupGlobalSetDefaultColorAttrib("TXTFGCOLOR",    0,   0,   0);

	if (menu_bg)   cocoaTouchGlobalSetFromUIColor("MENUBGCOLOR", menu_bg);
	else           iupGlobalSetDefaultColorAttrib("MENUBGCOLOR", 183, 183, 183);

	if (fg_color)  cocoaTouchGlobalSetFromUIColor("MENUFGCOLOR", fg_color);
	else           iupGlobalSetDefaultColorAttrib("MENUFGCOLOR",   0,   0,   0);

	if (link_fg)   cocoaTouchGlobalSetFromUIColor("LINKFGCOLOR", link_fg);
	else           iupGlobalSetDefaultColorAttrib("LINKFGCOLOR",   0,  88, 208);
}

static const void* IUP_COCOATOUCH_THEME_REFRESH_KEY = @"IupCocoaTouchThemeRefresh";

void iupCocoaTouchRegisterThemeRefresh(UIView* view, IupCocoaTouchThemeRefreshBlock block)
{
	if (!view) return;
	objc_setAssociatedObject(view, IUP_COCOATOUCH_THEME_REFRESH_KEY, block, OBJC_ASSOCIATION_COPY_NONATOMIC);
}

static void cocoaTouchRefreshTreeRecursive(UIView* view)
{
	if (!view) return;
	IupCocoaTouchThemeRefreshBlock block = objc_getAssociatedObject(view, IUP_COCOATOUCH_THEME_REFRESH_KEY);
	if (block) block(view);
	for (UIView* child in view.subviews) cocoaTouchRefreshTreeRecursive(child);
}

void iupCocoaTouchRefreshAllThemes(void)
{
	for (UIScene* scene in [UIApplication sharedApplication].connectedScenes)
	{
		if (![scene isKindOfClass:[UIWindowScene class]]) continue;
		UIWindowScene* ws = (UIWindowScene*)scene;
		for (UIWindow* window in ws.windows)
		{
			cocoaTouchRefreshTreeRecursive(window.rootViewController.view);
			for (UIViewController* presented = window.rootViewController.presentedViewController;
			     presented; presented = presented.presentedViewController)
			{
				cocoaTouchRefreshTreeRecursive(presented.view);
			}
		}
	}
}

static void cocoaTouchFireThemeChangedRecursive(UIViewController* vc, int dark_mode)
{
	if (!vc) return;
	if ([vc isKindOfClass:[IupViewController class]])
	{
		Ihandle* ih = ((IupViewController*)vc).ihandle;
		if (ih && iupObjectCheck(ih))
		{
			IFni cb = (IFni)IupGetCallback(ih, "THEMECHANGED_CB");
			if (cb) cb(ih, dark_mode);
		}
	}
	if ([vc isKindOfClass:[UINavigationController class]])
		for (UIViewController* child in ((UINavigationController*)vc).viewControllers)
			cocoaTouchFireThemeChangedRecursive(child, dark_mode);
	if (vc.presentedViewController)
		cocoaTouchFireThemeChangedRecursive(vc.presentedViewController, dark_mode);
}

static void cocoaTouchFireThemeChangedAll(int dark_mode)
{
	for (UIScene* scene in [UIApplication sharedApplication].connectedScenes)
	{
		if (![scene isKindOfClass:[UIWindowScene class]]) continue;
		for (UIWindow* window in ((UIWindowScene*)scene).windows)
			cocoaTouchFireThemeChangedRecursive(window.rootViewController, dark_mode);
	}
}

void iupCocoaTouchHandleTraitFlip(void)
{
	/* dedupe across the multiple VCs that fire on each flip */
	static UIUserInterfaceStyle s_last_style = UIUserInterfaceStyleUnspecified;
	UIWindow* window = iupCocoaTouchFindCurrentWindow();
	UIUserInterfaceStyle now = window ? window.traitCollection.userInterfaceStyle : UIUserInterfaceStyleUnspecified;
	if (now == s_last_style) return;
	s_last_style = now;

	cocoaTouchUpdateGlobalColors();
	IupSetGlobal("_IUP_RESET_GLOBALCOLORS", "YES");
	iupCocoaTouchRefreshAllThemes();
	int dark_mode = iupStrBoolean(IupGetGlobal("DARKMODE"));
	cocoaTouchFireThemeChangedAll(dark_mode);
}

IUP_SDK_API int iupdrvOpen(int* argc, char*** argv)
{
	(void)argc;
	(void)argv;

	[UIApplication sharedApplication];

	cocoaTouchRedirectStdio();

	IupSetGlobal("DRIVER", "CocoaTouch");
	IupSetGlobal("WINDOWING", "QUARTZ");

	iupdrvFontInit();
	cocoaTouchUpdateGlobalColors();
	IupSetGlobal("_IUP_RESET_GLOBALCOLORS", "YES");

	return IUP_NOERROR;
}

IUP_SDK_API void iupdrvClose(void)
{
	iupdrvFontFinish();
}
