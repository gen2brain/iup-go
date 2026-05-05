/** \file
 * \brief IupDialog class (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_class.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"

#include "iupcocoatouch_drv.h"

#import "IupViewController.h"


static const void* IUPCOCOATOUCH_DIALOG_DELEGATE_KEY = @"IUPCOCOATOUCH_DIALOG_DELEGATE";

@interface IupCocoaTouchDialogDelegate : NSObject <UIAdaptivePresentationControllerDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTouchDialogDelegate

- (BOOL)presentationControllerShouldDismiss:(UIPresentationController*)pc
{
	(void)pc;
	Ihandle* ih = _ihandle;
	if (!ih || !iupObjectCheck(ih)) return YES;
	Icallback cb = (Icallback)IupGetCallback(ih, "CLOSE_CB");
	if (cb && cb(ih) == IUP_IGNORE) return NO;
	return YES;
}

/* swipe-dismissed: skip the redundant dismiss in iupdrvDialogSetVisible(0) */
- (void)presentationControllerDidDismiss:(UIPresentationController*)pc
{
	(void)pc;
	Ihandle* ih = _ihandle;
	if (!ih || !iupObjectCheck(ih)) return;
	iupAttribSet(ih, "_IUPCOCOA_SHEET_GONE", "1");
	iupAttribSet(ih, "_IUP_DIALOG_DEFER_DESTROY", NULL);
	IupExitLoop();
}

- (void)closeButtonTapped:(id)sender
{
	(void)sender;
	Ihandle* ih = _ihandle;
	if (!ih || !iupObjectCheck(ih)) return;
	Icallback cb = (Icallback)IupGetCallback(ih, "CLOSE_CB");
	if (cb && cb(ih) == IUP_IGNORE) return;
	IupExitLoop();
}

@end


static IupViewController* cocoaTouchDialogVC(Ihandle* ih)
{
	if (!ih || !ih->handle) return nil;
	id h = ih->handle;
	return [h isKindOfClass:[IupViewController class]] ? (IupViewController*)h : nil;
}

/* show_state gate so a double iupDialogHide early-returns on the second pass */
IUP_SDK_API int iupdrvDialogIsVisible(Ihandle* ih)
{
	if (!ih || !ih->handle) return 0;
	if (ih->data && ih->data->show_state == IUP_HIDE) return 0;
	return 1;
}

IUP_SDK_API void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int* w, int* h)
{
	(void)handle;
	IupCocoaTouchFixed* client = iupCocoaTouchDialogGetClientArea(ih);
	if (client)
	{
		CGRect bounds = [client bounds];
		if (w) *w = iupROUND(bounds.size.width);
		if (h) *h = iupROUND(bounds.size.height);
		return;
	}
	CGRect screen = [[UIScreen mainScreen] bounds];
	if (w) *w = iupROUND(screen.size.width);
	if (h) *h = iupROUND(screen.size.height);
}

/* visible=0 dismisses + pumps until the animation completes, so a deferred Destroy doesn't race UIKit */
IUP_SDK_API void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
	if (visible || !ih || !ih->handle) return;
	if (iupAttribGet(ih, "_IUPCOCOA_SHEET_GONE")) return;

	IupViewController* vc = cocoaTouchDialogVC(ih);
	if (!vc) return;
	UIViewController* presenter = vc.navigationController ?: vc;

	if (presenter.presentingViewController == nil) return;

	if (presenter.isBeingDismissed)
	{
		while (presenter.presentingViewController != nil)
		{
			@autoreleasepool { CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true); }
		}
		return;
	}

	__block BOOL done = NO;
	[presenter dismissViewControllerAnimated:YES completion:^{ done = YES; }];
	while (!done)
	{
		@autoreleasepool { CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true); }
	}
}

IUP_SDK_API void iupdrvDialogGetPosition(Ihandle* ih, InativeHandle* handle, int* x, int* y)
{
	(void)ih; (void)handle;
	if (x) *x = 0;
	if (y) *y = 0;
}

IUP_SDK_API void iupdrvDialogSetPosition(Ihandle* ih, int x, int y)
{
	(void)ih; (void)x; (void)y;
}

IUP_SDK_API void iupdrvDialogGetDecoration(Ihandle* ih, int* border, int* caption, int* menu)
{
	(void)ih;
	if (border)  *border  = 0;
	if (caption) *caption = 0;
	if (menu)    *menu    = 0;
}

IUP_SDK_API int iupdrvDialogSetPlacement(Ihandle* ih)
{
	if (iupAttribGetBoolean(ih, "FULLSCREEN"))
	{
		ih->data->show_state = IUP_MAXIMIZE;
		return 1;
	}

	const char* placement = iupAttribGet(ih, "PLACEMENT");
	if (!placement)
	{
		ih->data->show_state = IUP_SHOW;
		return 1;
	}

	IupViewController* vc = cocoaTouchDialogVC(ih);

	if (iupStrEqualNoCase(placement, "MAXIMIZED") || iupStrEqualNoCase(placement, "FULL"))
	{
		if (vc)
		{
			vc.hideStatusBar = YES;
			vc.hideHomeIndicator = YES;
			[vc setNeedsStatusBarAppearanceUpdate];
		}
		ih->data->show_state = IUP_MAXIMIZE;
	}
	else if (iupStrEqualNoCase(placement, "MINIMIZED"))
	{
		/* no public API to self-minimize on iOS */
		ih->data->show_state = IUP_MINIMIZE;
	}
	else
	{
		ih->data->show_state = IUP_SHOW;
	}

	iupAttribSet(ih, "PLACEMENT", NULL);
	return 1;
}

IUP_SDK_API void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
	(void)ih; (void)parent;
}

static char* cocoaTouchDialogGetClientSizeAttrib(Ihandle* ih)
{
	return iupStrReturnIntInt(ih->currentwidth, ih->currentheight, 'x');
}

static char* cocoaTouchDialogGetClientOffsetAttrib(Ihandle* ih)
{
	(void)ih;
	return iupStrReturnIntInt(0, 0, 'x');
}

/* swap titleView to [icon][title] stack when ICON is set, else fall back to vc.title */
static void cocoaTouchDialogRefreshTitleView(Ihandle* ih, const char* icon_name)
{
	IupViewController* vc = cocoaTouchDialogVC(ih);
	if (!vc) return;
	UIImage* image = (icon_name && *icon_name) ? (UIImage*)iupImageGetImage(icon_name, ih, 0, NULL) : nil;
	if (!image)
	{
		vc.navigationItem.titleView = nil;
		return;
	}

	UIImageView* iv = [[[UIImageView alloc] initWithImage:image] autorelease];
	iv.contentMode = UIViewContentModeScaleAspectFit;
	iv.translatesAutoresizingMaskIntoConstraints = NO;
	[iv.widthAnchor constraintEqualToConstant:24].active = YES;
	[iv.heightAnchor constraintEqualToConstant:24].active = YES;

	UILabel* label = [[[UILabel alloc] init] autorelease];
	label.text = vc.title ?: @"";
	label.font = [UIFont boldSystemFontOfSize:17];
	label.textColor = [UIColor labelColor];

	UIStackView* stack = [[[UIStackView alloc] initWithArrangedSubviews:@[iv, label]] autorelease];
	stack.axis = UILayoutConstraintAxisHorizontal;
	stack.alignment = UIStackViewAlignmentCenter;
	stack.spacing = 8;

	vc.navigationItem.titleView = stack;
}

static int cocoaTouchDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
	IupViewController* vc = cocoaTouchDialogVC(ih);
	if (!vc) return 1;
	NSString* title = (value && *value) ? [NSString stringWithUTF8String:value] : nil;
	[vc setTitle:title];
	cocoaTouchDialogRefreshTitleView(ih, iupAttribGet(ih, "ICON"));
	return 1;
}

extern void iupCocoaTouchDialogAttachDrawerMenu(Ihandle* dialog_ih, Ihandle* drawer_ih);

static int cocoaTouchDialogSetDrawerAttrib(Ihandle* ih, const char* value)
{
	Ihandle* drawer = (value && *value) ? IupGetHandle(value) : NULL;
	if (drawer && !drawer->handle) IupMap(drawer);
	if (!ih->handle) return 1;
	iupCocoaTouchDialogAttachDrawerMenu(ih, drawer && drawer->handle ? drawer : NULL);
	return 1;
}

static int cocoaTouchDialogSetBgColorAttrib(Ihandle* ih, const char* value)
{
	IupViewController* vc = cocoaTouchDialogVC(ih);
	if (!vc) return 0;

	/* swap DLGBGCOLOR -> systemBackgroundColor so the bg auto-flips on dark-mode change */
	if (value && iupStrEqual(value, IupGetGlobal("DLGBGCOLOR")))
	{
		[[vc view] setBackgroundColor:[UIColor systemBackgroundColor]];
		return 1;
	}

	UIColor* color = iupCocoaTouchToNativeColor(value);
	if (!color) return 0;
	[[vc view] setBackgroundColor:color];
	return 1;
}

static const void* IUPCOCOATOUCH_BG_IMAGEVIEW_KEY = @"IUPCOCOATOUCH_BG_IMAGEVIEW";

static void cocoaTouchDialogClearBackgroundImage(UIView* root)
{
	UIImageView* existing = (UIImageView*)objc_getAssociatedObject(root, IUPCOCOATOUCH_BG_IMAGEVIEW_KEY);
	if (existing)
	{
		[existing removeFromSuperview];
		objc_setAssociatedObject(root, IUPCOCOATOUCH_BG_IMAGEVIEW_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	}
}

static int cocoaTouchDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
	IupViewController* vc = cocoaTouchDialogVC(ih);
	if (!vc) return 0;
	UIView* root = [vc view];

	UIColor* color = iupCocoaTouchToNativeColor(value);
	if (color)
	{
		cocoaTouchDialogClearBackgroundImage(root);
		[root setBackgroundColor:color];
		return 1;
	}

	UIImage* image = (UIImage*)iupImageGetImage(value, ih, 0, NULL);
	if (!image) return 0;

	cocoaTouchDialogClearBackgroundImage(root);

	if (iupAttribGetBoolean(ih, "BACKIMAGEZOOM"))
	{
		/* autoresizing keeps the bg in sync with root through rotation/resize */
		UIImageView* iv = [[UIImageView alloc] initWithFrame:root.bounds];
		iv.image = image;
		iv.contentMode = UIViewContentModeScaleToFill;
		iv.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
		[root insertSubview:iv atIndex:0];
		objc_setAssociatedObject(root, IUPCOCOATOUCH_BG_IMAGEVIEW_KEY, iv, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
		[iv release];
	}
	else
	{
		[root setBackgroundColor:[UIColor colorWithPatternImage:image]];
	}
	return 1;
}

static int cocoaTouchDialogSetBackImageZoomAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	const char* background = iupAttribGet(ih, "BACKGROUND");
	if (background) cocoaTouchDialogSetBackgroundAttrib(ih, background);
	return 1;
}

static void cocoaTouchDialogApplyFullscreen(IupViewController* vc, BOOL on)
{
	vc.hideStatusBar = on;
	vc.hideHomeIndicator = on;
	[vc setNeedsStatusBarAppearanceUpdate];
	[vc setNeedsUpdateOfHomeIndicatorAutoHidden];
	[vc.navigationController setNavigationBarHidden:on animated:YES];
}

static int cocoaTouchDialogSetHideTitleBarAttrib(Ihandle* ih, const char* value)
{
	IupViewController* vc = cocoaTouchDialogVC(ih);
	if (!vc) return 1;
	[vc.navigationController setNavigationBarHidden:(iupStrBoolean(value) ? YES : NO) animated:YES];
	return 1;
}

static int cocoaTouchDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
	IupViewController* vc = cocoaTouchDialogVC(ih);
	if (!vc) return 1;
	cocoaTouchDialogApplyFullscreen(vc, iupStrBoolean(value) ? YES : NO);
	return 1;
}

static int cocoaTouchDialogSetIconAttrib(Ihandle* ih, const char* value)
{
	cocoaTouchDialogRefreshTitleView(ih, value);
	return 1;
}

static int cocoaTouchDialogSetTitleBarStyleAttrib(Ihandle* ih, const char* value)
{
	IupViewController* vc = cocoaTouchDialogVC(ih);
	UINavigationBar* bar = vc.navigationController.navigationBar;
	if (!bar) return 1;

	UINavigationBarAppearance* app = [[[UINavigationBarAppearance alloc] init] autorelease];
	UIColor* tint = nil;
	if (iupStrEqualNoCase(value, "LIFTED"))
	{
		[app configureWithOpaqueBackground];
		app.backgroundColor = [UIColor secondarySystemGroupedBackgroundColor];
		tint = [UIColor labelColor];
	}
	else if (iupStrEqualNoCase(value, "PRIMARY"))
	{
		[app configureWithOpaqueBackground];
		app.backgroundColor = bar.tintColor ?: [UIColor systemBlueColor];
		app.titleTextAttributes  = @{ NSForegroundColorAttributeName: [UIColor whiteColor] };
		app.largeTitleTextAttributes = @{ NSForegroundColorAttributeName: [UIColor whiteColor] };
		tint = [UIColor whiteColor];
	}
	else
	{
		[app configureWithDefaultBackground];
		tint = [UIColor labelColor];
	}
	bar.standardAppearance   = app;
	bar.scrollEdgeAppearance = app;
	bar.compactAppearance    = app;
	bar.tintColor = tint;
	return 1;
}

static UIInterfaceOrientationMask cocoaTouchDialogParseOrientationMask(const char* value)
{
	if (!value || !*value) return UIInterfaceOrientationMaskAll;
	if (iupStrEqualNoCase(value, "PORTRAIT"))  return UIInterfaceOrientationMaskPortrait;
	if (iupStrEqualNoCase(value, "LANDSCAPE")) return UIInterfaceOrientationMaskLandscape;
	if (iupStrEqualNoCase(value, "SENSOR"))    return UIInterfaceOrientationMaskAll;
	if (iupStrEqualNoCase(value, "LOCKED"))
	{
		UIInterfaceOrientation cur = UIInterfaceOrientationPortrait;
		UIWindow* w = iupCocoaTouchFindCurrentWindow();
		if (w && w.windowScene) cur = w.windowScene.interfaceOrientation;
		switch (cur)
		{
			case UIInterfaceOrientationPortrait:           return UIInterfaceOrientationMaskPortrait;
			case UIInterfaceOrientationPortraitUpsideDown: return UIInterfaceOrientationMaskPortraitUpsideDown;
			case UIInterfaceOrientationLandscapeLeft:      return UIInterfaceOrientationMaskLandscapeLeft;
			case UIInterfaceOrientationLandscapeRight:     return UIInterfaceOrientationMaskLandscapeRight;
			default:                                       return UIInterfaceOrientationMaskAll;
		}
	}
	return UIInterfaceOrientationMaskAll;
}

static int cocoaTouchDialogSetOrientationAttrib(Ihandle* ih, const char* value)
{
	IupViewController* vc = cocoaTouchDialogVC(ih);
	if (!vc) return 1;
	vc.allowedOrientations = cocoaTouchDialogParseOrientationMask(value);

	NSOperatingSystemVersion ver = [[NSProcessInfo processInfo] operatingSystemVersion];
	if (ver.majorVersion >= 16)
	{
		UIWindowScene* scene = iupCocoaTouchFindCurrentWindow().windowScene;
		if (scene)
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
			UIWindowSceneGeometryPreferencesIOS* prefs =
				[[[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:vc.allowedOrientations] autorelease];
			[scene requestGeometryUpdateWithPreferences:prefs errorHandler:nil];
			[vc setNeedsUpdateOfSupportedInterfaceOrientations];
#pragma clang diagnostic pop
		}
	}
	else
	{
		[UIViewController attemptRotationToDeviceOrientation];
	}
	return 1;
}

static int cocoaTouchDialogSetBringFrontAttrib(Ihandle* ih, const char* value)
{
	(void)ih; (void)value;
	return 0;
}

static int cocoaTouchDialogSetOpacityAttrib(Ihandle* ih, const char* value)
{
	int op = 255;
	if (value && !iupStrToInt(value, &op)) op = 255;
	if (op < 0) op = 0;
	if (op > 255) op = 255;
	IupViewController* vc = cocoaTouchDialogVC(ih);
	if (!vc) return 1;
	[vc view].alpha = (CGFloat)op / 255.0;
	return 1;
}

static int cocoaTouchDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
	return iupBaseSetMinSizeAttrib(ih, value);
}

static int cocoaTouchDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
	return iupBaseSetMaxSizeAttrib(ih, value);
}

static int cocoaTouchDialogMapMethod(Ihandle* ih)
{
	UIWindow* window = iupCocoaTouchFindCurrentWindow();
	if (!window) return IUP_ERROR;

	IupViewController* vc = [[IupViewController alloc] init];
	[vc setIhandle:ih];
	ih->handle = vc;

	BOOL fullscreen = iupAttribGetBoolean(ih, "FULLSCREEN");

	IupCocoaTouchDialogDelegate* delegate = [[IupCocoaTouchDialogDelegate alloc] init];
	delegate.ihandle = ih;
	objc_setAssociatedObject(vc, IUPCOCOATOUCH_DIALOG_DELEGATE_KEY, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[delegate release];

	const char* title_str = iupAttribGet(ih, "TITLE");
	if (title_str) [vc setTitle:[NSString stringWithUTF8String:title_str]];

	UIViewController* root = [window rootViewController];
	BOOL presenting = !(root == nil || iupCocoaTouchIsLaunchPlaceholder(root));

	/* always wrap in a UINavigationController so MENU/DRAWER setters have a nav bar to target */
	UINavigationController* nav = [[UINavigationController alloc] initWithRootViewController:vc];
	[nav setNavigationBarHidden:fullscreen animated:NO];

	/* default close buttons fill the slots MENU/DRAWER don't claim; setters overwrite when applied */
	if (presenting && !fullscreen)
	{
		if (!iupAttribGet(ih, "DRAWER"))
		{
			vc.navigationItem.leftBarButtonItem = [[[UIBarButtonItem alloc]
			    initWithImage:[UIImage systemImageNamed:@"chevron.backward"]
			            style:UIBarButtonItemStylePlain
			           target:delegate
			           action:@selector(closeButtonTapped:)] autorelease];
		}
		if (!iupAttribGet(ih, "MENU"))
		{
			vc.navigationItem.rightBarButtonItem = [[[UIBarButtonItem alloc]
			    initWithImage:[UIImage systemImageNamed:@"xmark"]
			            style:UIBarButtonItemStylePlain
			           target:delegate
			           action:@selector(closeButtonTapped:)] autorelease];
		}
	}

	UIModalPresentationStyle style = (fullscreen || !presenting) ? UIModalPresentationFullScreen : UIModalPresentationPageSheet;
	[nav setModalPresentationStyle:style];
	nav.presentationController.delegate = delegate;
	if (style == UIModalPresentationPageSheet &&
	    [nav.sheetPresentationController respondsToSelector:@selector(setPrefersScrollingExpandsWhenScrolledToEdge:)])
	{
		nav.sheetPresentationController.prefersScrollingExpandsWhenScrolledToEdge = NO;
	}

	if (!presenting)
	{
		[window setRootViewController:nav];
		[nav release];
		IFni cb = (IFni)IupGetCallback(ih, "SHOW_CB");
		if (cb) cb(ih, IUP_SHOW);
	}
	else
	{
		UIViewController* top = iupCocoaTouchFindTopPresentedViewController();
		if (top == nil) top = root;
		Ihandle* ih_ref = ih;
		[top presentViewController:nav animated:YES completion:^{
			IFni cb = (IFni)IupGetCallback(ih_ref, "SHOW_CB");
			if (cb) cb(ih_ref, IUP_SHOW);
		}];
		[nav release];
	}

	/* hold IupDestroy off until the user dismisses the sheet */
	iupAttribSet(ih, "_IUP_DIALOG_DEFER_DESTROY", "1");

	return IUP_NOERROR;
}

static void cocoaTouchDialogUnMapMethod(Ihandle* ih)
{
	IupViewController* vc = cocoaTouchDialogVC(ih);
	if (vc)
	{
		[vc setIhandle:NULL];
		UIViewController* presenter = vc.navigationController ?: vc;
		presenter.presentationController.delegate = nil;

		IupCocoaTouchDialogDelegate* delegate = (IupCocoaTouchDialogDelegate*)objc_getAssociatedObject(vc, IUPCOCOATOUCH_DIALOG_DELEGATE_KEY);
		if (delegate) delegate.ihandle = NULL;
		vc.navigationItem.leftBarButtonItem.target = nil;
		vc.navigationItem.rightBarButtonItem.target = nil;

		objc_setAssociatedObject(vc, IUPCOCOATOUCH_DIALOG_DELEGATE_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

		if (presenter.presentingViewController != nil && !presenter.isBeingDismissed)
		{
			[presenter dismissViewControllerAnimated:NO completion:nil];
		}
	}
	[(id)vc release];
	ih->handle = NULL;
}

static void cocoaTouchDialogLayoutUpdateMethod(Ihandle* ih)
{
	/* UIKit owns dialog geometry; children get repositioned by iupdrvBaseLayoutUpdateMethod */
	(void)ih;
}

IUP_SDK_API void iupdrvDialogInitClass(Iclass* ic)
{
	ic->Map          = cocoaTouchDialogMapMethod;
	ic->UnMap        = cocoaTouchDialogUnMapMethod;
	ic->LayoutUpdate = cocoaTouchDialogLayoutUpdateMethod;

	/* shrink to viewport + suppress auto-focus so the keyboard doesn't pop on map */
	iupClassRegisterReplaceAttribDef(ic, "SHRINK", "YES", NULL);
	iupClassRegisterReplaceAttribDef(ic, "SHOWNOFOCUS", "YES", NULL);

	iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaTouchDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTouchDialogSetBgColorAttrib, "DLGBGCOLOR", NULL, IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "BACKGROUND", NULL, cocoaTouchDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "BACKIMAGEZOOM", NULL, cocoaTouchDialogSetBackImageZoomAttrib, NULL, NULL, IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "CLIENTSIZE", cocoaTouchDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CLIENTOFFSET", cocoaTouchDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_DEFAULTVALUE|IUPAF_READONLY|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, cocoaTouchDialogSetFullScreenAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ORIENTATION", NULL, cocoaTouchDialogSetOrientationAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, cocoaTouchDialogSetBringFrontAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ICON", NULL, cocoaTouchDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TITLEBARSTYLE", NULL, cocoaTouchDialogSetTitleBarStyleAttrib, IUPAF_SAMEASSYSTEM, "FLAT", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DRAWER", NULL, cocoaTouchDialogSetDrawerAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "MINSIZE", NULL, cocoaTouchDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MAXSIZE", NULL, cocoaTouchDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "OPACITY", NULL, cocoaTouchDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);

	iupClassRegisterCallback(ic, "THEMECHANGED_CB", "i");

	iupClassRegisterAttribute(ic, "MAXBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MINBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MENUBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "HIDETITLEBAR", NULL, cocoaTouchDialogSetHideTitleBarAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "RESIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "BORDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DIALOGFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CUSTOMFRAMESIMULATE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "HIDETASKBAR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TOPMOST", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SHAPEIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
