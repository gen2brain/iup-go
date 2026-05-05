/** \file
 * \brief iOS UIApplication delegate wiring the IUP entry point.
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#import "IupAppDelegate.h"
#import "IupViewController.h"

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"
#include "iup_key.h"
#include "iup_loop.h"
#include "iupcocoatouch_drv.h"


static CGPoint s_last_touch_screen = {0.0, 0.0};
static BOOL s_has_last_touch = NO;

static void iosGlobalTouchPhase(UITouchPhase phase, CGPoint screen_pt)
{
	switch (phase)
	{
		case UITouchPhaseBegan:
		{
			IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
			if (cb)
			{
				char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
				iupKEY_SETBUTTON1(status);
				cb(IUP_BUTTON1, 1, (int)screen_pt.x, (int)screen_pt.y, status);
			}
			break;
		}
		case UITouchPhaseEnded:
		case UITouchPhaseCancelled:
		{
			IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
			if (cb)
			{
				char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
				cb(IUP_BUTTON1, 0, (int)screen_pt.x, (int)screen_pt.y, status);
			}
			break;
		}
		case UITouchPhaseMoved:
		{
			IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
			if (cb)
			{
				char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
				iupKEY_SETBUTTON1(status);
				cb((int)screen_pt.x, (int)screen_pt.y, status);
			}
			break;
		}
		default:
			break;
	}
}

@interface IupWindow : UIWindow
@end

@implementation IupWindow
- (void)sendEvent:(UIEvent*)event
{
	if (event.type == UIEventTypeTouches)
	{
		for (UITouch* t in [event allTouches])
		{
			if (t.phase == UITouchPhaseBegan || t.phase == UITouchPhaseMoved)
			{
				CGPoint window_pt = [t locationInView:nil];
				s_last_touch_screen = [self convertPoint:window_pt toWindow:nil];
				s_has_last_touch = YES;
				break;
			}
		}

		for (UITouch* t in [event allTouches])
		{
			CGPoint window_pt = [t locationInView:nil];
			CGPoint screen_pt = [self convertPoint:window_pt toWindow:nil];
			iosGlobalTouchPhase(t.phase, screen_pt);
		}
	}
	[super sendEvent:event];
}
@end

void iupCocoaTouchGetLastTouchScreen(CGPoint* out)
{
	if (!out) return;
	*out = s_has_last_touch ? s_last_touch_screen : CGPointZero;
}


@interface IupLaunchViewController : IupViewController
@end

@implementation IupLaunchViewController
@end

bool iupCocoaTouchIsLaunchPlaceholder(UIViewController* vc)
{
	return vc != nil && [vc isKindOfClass:[IupLaunchViewController class]];
}


@implementation IupAppDelegate

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
	CGRect window_bounds = [[UIScreen mainScreen] bounds];
	UIWindow* the_window = [[IupWindow alloc] initWithFrame:window_bounds];

	IupLaunchViewController* view_controller = [[[IupLaunchViewController alloc] init] autorelease];
	[the_window setRootViewController:view_controller];

	[self setWindow:the_window];

	iupLoopCallEntryCb();
	iupCocoaTouchMarkEntryFinished();

	[the_window makeKeyAndVisible];

	return YES;
}

- (void)applicationWillTerminate:(UIApplication*)application
{
	/* Apple rarely delivers this; exit handlers must not rely on it. */
	iupLoopCallExitCb();
}

- (UIWindow*)currentWindow
{
	return [self window];
}

@end
