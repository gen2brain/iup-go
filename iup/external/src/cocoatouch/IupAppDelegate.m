/** \file
 * \brief iOS UIApplication delegate wiring the IUP entry point.
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#import "IupAppDelegate.h"
#import "IupViewController.h"

#include "iup_loop.h"
#include "iupcocoatouch_drv.h"


static CGPoint s_last_touch_screen = {0.0, 0.0};
static BOOL s_has_last_touch = NO;

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
