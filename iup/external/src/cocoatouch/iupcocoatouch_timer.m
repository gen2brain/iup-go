/** \file
 * \brief Timer for the iOS UIKit driver.
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_timer.h"


@interface IupCocoaTouchTimerController : NSObject
@property(assign) CFTimeInterval startTime;
@property(retain) NSTimer* theTimer;
- (void)onTimerCallback:(NSTimer*)timer;
@end

@implementation IupCocoaTouchTimerController
- (void)dealloc
{
	[self setTheTimer:nil];
	[super dealloc];
}

- (void)onTimerCallback:(NSTimer*)timer
{
	Ihandle* ih = (Ihandle*)[[[self theTimer] userInfo] pointerValue];
	Icallback cb = IupGetCallback(ih, "ACTION_CB");
	if (!cb)
	{
		return;
	}
	NSUInteger elapsed_ms = (NSUInteger)(((CACurrentMediaTime() - [self startTime]) * 1000.0) + 0.5);
	iupAttribSetInt(ih, "ELAPSEDTIME", (int)elapsed_ms);
	if (cb(ih) == IUP_CLOSE)
	{
		IupExitLoop();
	}
}
@end

IUP_SDK_API void iupdrvTimerRun(Ihandle* ih)
{
	if (ih->handle != nil)
	{
		return;
	}

	unsigned int time_ms = iupAttribGetInt(ih, "TIME");
	if (time_ms == 0)
	{
		return;
	}

	IupCocoaTouchTimerController* controller = [[IupCocoaTouchTimerController alloc] init];
	[controller setStartTime:CACurrentMediaTime()];

	NSTimer* timer = [NSTimer timerWithTimeInterval:(time_ms/1000.0)
		target:controller
		selector:@selector(onTimerCallback:)
		userInfo:[NSValue valueWithPointer:ih]
		repeats:YES];
	[controller setTheTimer:timer];

	/* CommonModes keeps the timer firing through modal sheets and scroll tracking */
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];

	ih->handle = (void*)controller;
}

static void cocoaTouchTimerDestroy(Ihandle* ih)
{
	if (ih->handle == nil)
	{
		return;
	}
	IupCocoaTouchTimerController* controller = (IupCocoaTouchTimerController*)ih->handle;
	[[controller theTimer] invalidate];
	[controller release];
	ih->handle = nil;
}

IUP_SDK_API void iupdrvTimerStop(Ihandle* ih)
{
	cocoaTouchTimerDestroy(ih);
}

static int cocoaTouchTimerSetRunAttrib(Ihandle* ih, const char* value)
{
	if (iupStrBoolean(value))
		iupdrvTimerRun(ih);
	else
		iupdrvTimerStop(ih);

	return 0;
}

/* the base getters read ih->serial, the controller lives in ih->handle */
static char* cocoaTouchTimerGetRunAttrib(Ihandle* ih)
{
	return iupStrReturnBoolean(ih->handle != nil);
}

static char* cocoaTouchTimerGetWidAttrib(Ihandle* ih)
{
	return iupStrReturnInt((int)(intptr_t)ih->handle);
}

IUP_SDK_API void iupdrvTimerInitClass(Iclass* ic)
{
	ic->UnMap = cocoaTouchTimerDestroy;

	iupClassRegisterAttribute(ic, "WID", cocoaTouchTimerGetWidAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
	iupClassRegisterAttribute(ic, "RUN", cocoaTouchTimerGetRunAttrib, cocoaTouchTimerSetRunAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
}
