/** \file
 * \brief iOS message loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#import <UIKit/UIKit.h>
#import <CoreFoundation/CoreFoundation.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"

#include "iupcocoatouch_drv.h"

#import "IupAppDelegate.h"

static int s_main_loop_level = 0;
static int s_modal_loop_level = 0;
static int s_modal_should_exit = 0;
static int s_entry_finished = 0;

static IFidle s_idle_cb = NULL;
static CFRunLoopObserverRef s_idle_observer = NULL;

static void iupCocoaTouchRemoveIdleObserver(void)
{
	if (!s_idle_observer) return;
	CFRunLoopRemoveObserver(CFRunLoopGetMain(), s_idle_observer, kCFRunLoopCommonModes);
	CFRelease(s_idle_observer);
	s_idle_observer = NULL;
}

static void iupCocoaTouchIdleObserver(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void* info)
{
	(void)observer; (void)activity; (void)info;
	if (!s_idle_cb) return;
	int ret = s_idle_cb();
	if (ret == IUP_CLOSE)
	{
		s_idle_cb = NULL;
		iupCocoaTouchRemoveIdleObserver();
		IupExitLoop();
		return;
	}
	if (ret == IUP_IGNORE)
	{
		s_idle_cb = NULL;
		iupCocoaTouchRemoveIdleObserver();
		return;
	}
	/* re-poke the loop so BeforeWaiting fires again */
	dispatch_async(dispatch_get_main_queue(), ^{});
}

IUP_SDK_API void iupdrvSetIdleFunction(Icallback f)
{
	s_idle_cb = (IFidle)f;
	if (s_idle_cb)
	{
		if (!s_idle_observer)
		{
			s_idle_observer = CFRunLoopObserverCreate(NULL, kCFRunLoopBeforeWaiting, true, 0, iupCocoaTouchIdleObserver, NULL);
			CFRunLoopAddObserver(CFRunLoopGetMain(), s_idle_observer, kCFRunLoopCommonModes);
		}
	}
	else
	{
		iupCocoaTouchRemoveIdleObserver();
	}
}

IUP_DRV_API void iupCocoaTouchMarkEntryFinished(void)
{
	s_entry_finished = 1;
}

/* break the modal pump if active, otherwise dismiss the topmost presented sheet */
void IupExitLoop(void)
{
	if (s_modal_loop_level > 0)
	{
		s_modal_should_exit = 1;
		return;
	}
	UIViewController* top = iupCocoaTouchFindTopPresentedViewController();
	if (top && top.presentingViewController != nil)
		[top dismissViewControllerAnimated:YES completion:nil];
}

int IupMainLoopLevel(void)
{
	return s_main_loop_level + s_modal_loop_level;
}

/* three-state: UIApplicationMain on first call, IUP_OPENED inside ENTRY_POINT, or nested CFRunLoop pump */
int IupMainLoop(void)
{
	@autoreleasepool
	{
		if ([[UIApplication sharedApplication] delegate] == nil)
		{
			s_main_loop_level++;
			char prog[] = "iup";
			char* argv[] = { prog, NULL };
			int argc = 1;
			int ret = UIApplicationMain(argc, argv, nil, NSStringFromClass([IupAppDelegate class]));
			s_main_loop_level--;
			return ret;
		}
	}

	if (!s_entry_finished)
		return IUP_OPENED;

	s_modal_loop_level++;
	s_modal_should_exit = 0;
	while (!s_modal_should_exit)
	{
		@autoreleasepool {
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true);
		}
	}
	s_modal_loop_level--;
	return IUP_NOERROR;
}

int IupLoopStepWait(void)
{
	@autoreleasepool {
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true);
	}
	return IUP_DEFAULT;
}

int IupLoopStep(void)
{
	@autoreleasepool {
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
	}
	return IUP_DEFAULT;
}

void IupFlush(void)
{
	@autoreleasepool {
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
	}
}

void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
	char* s_copy = iupStrDup(s);
	dispatch_async(dispatch_get_main_queue(),
		^{
			if (ih && iupObjectCheck(ih))
			{
				IFnsidv cb = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
				if (cb)
				{
					cb(ih, s_copy, i, d, p);
				}
			}
			free(s_copy);
		}
	);
}
