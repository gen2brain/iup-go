/** \file
 * \brief Timer for the Mac Driver.
 *
 * See Copyright Notice in "iup.h"
 */


// TODO: FEATURE: Support Apple 'tolerance' property which controls battery vs. accuracy.


#import <Cocoa/Cocoa.h>
 
#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_timer.h"

@interface IupCocoaTimerController : NSObject
// CFTimeInterval is a double
@property (assign) CFTimeInterval startTime;
@property (retain) NSTimer* theTimer;
- (void) onTimerCallback:(NSTimer*)theTimer;
@end

@implementation IupCocoaTimerController
@synthesize theTimer;

- (void) dealloc
{
	[self setTheTimer:nil];
	[super dealloc];
}

- (void) onTimerCallback:(NSTimer*)theTimer
{
  Icallback callback_function;
  Ihandle* ih = (Ihandle*)[[[self theTimer] userInfo] pointerValue];
  callback_function = IupGetCallback(ih, "ACTION_CB");
	
  if(callback_function)
  {
	  CFTimeInterval start_time = [self startTime];
	  double current_time = CACurrentMediaTime();
	  NSUInteger elapsed_time = (NSUInteger)(((current_time - start_time) * 1000.0) + 0.5);
	  iupAttribSetInt(ih, "ELAPSEDTIME", (int)elapsed_time);
	  
    if(callback_function(ih) == IUP_CLOSE)
	{
		IupExitLoop();
	}
  }
}

@end


void iupdrvTimerRun(Ihandle* ih)
{
  unsigned int time_ms;

  if (ih->handle != nil) /* timer already started */
  {
	  return;
  }
  time_ms = iupAttribGetInt(ih, "TIME");
  if(time_ms > 0)
  {
	  IupCocoaTimerController* timer_controller = [[IupCocoaTimerController alloc] init];
	  // CACurrentMediaTime is tied to a real time clock. It uses mach_absolute_time() under the hood.
	  // GNUStep: Neither of these is likely directly portable (CACurrentMediaTime more likely), so we may need an #ifdef here.
	  // [[NSDate date] timeIntervalSince1970]; isn't so great because it is affected by network clock changes and so forth.
	  double start_time = CACurrentMediaTime();

	  NSTimer* the_timer = [NSTimer timerWithTimeInterval:(time_ms/1000.0)
		target:timer_controller
        selector:@selector(onTimerCallback:)
        userInfo:(id)[NSValue valueWithPointer:ih]
		repeats:YES
	];
	  


	  // Cocoa seems to block timers or events sometimes. This can be seen
	  // when I'm animating (via a timer) and you open an popup box or move a slider.
	  // Apparently, sheets and dialogs can also block (try printing).
	  // To work around this, Cocoa provides different run-loop modes. I need to
	  // specify the modes to avoid the blockage.
	  // NSDefaultRunLoopMode seems to be the default. I don't think I need to explicitly
	  // set this one, but just in case, I will set it anyway.
	  [[NSRunLoop currentRunLoop] addTimer:the_timer forMode:NSRunLoopCommonModes];


	[timer_controller setTheTimer:the_timer];
	  [timer_controller setStartTime:start_time];

	  ih->handle = timer_controller;
	  ih->serial = (int)(intptr_t)timer_controller; // for WID
  }
	
}

static void cocoaTimerDestroy(Ihandle* ih)
{
	if(nil != ih->handle)
	{
		IupCocoaTimerController* timer_controller = (IupCocoaTimerController*)ih->handle;
		NSTimer* the_timer = [timer_controller theTimer];
		
		[the_timer invalidate];
		
		// This will also release the timer instance via the dealloc
		[timer_controller release];
		
		ih->handle = nil;
	}
}

void iupdrvTimerStop(Ihandle* ih)
{

	cocoaTimerDestroy(ih);

}

// copied from iTimerSetRunAttrib because it is static, and we needed to override TimerGetRunAttrib, but must also override this to do so.
static int cocoaSetRunAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
    iupdrvTimerRun(ih);
  else
    iupdrvTimerStop(ih);

  return 0;
}

static char* cocoaTimerGetRunAttrib(Ihandle *ih)
{
  return iupStrReturnBoolean(ih->handle != nil);
}

static char* cocoaTimerGetWidAttrib(Ihandle *ih)
{
  // WARNING: This is going to truncate the pointer, which means it will be useless by anybody who actually gets the WID.
  // The WID API is mostly useless, except for maybe checking if the timer is running.
  return iupStrReturnInt((int)(intptr_t)ih->handle);
}

void iupdrvTimerInitClass(Iclass* ic)
{
	(void)ic;
	// This must be UnMap and not Destroy because we're using the ih->handle and UnMap will clear the pointer to NULL before we reach Destroy.
	ic->UnMap = cocoaTimerDestroy;
	
	// We need to override because we don't use ih->serial to store the timer.
	// We need a full pointer and serial would truncate and be wrong.
	// The base code assumes serial.
	iupClassRegisterAttribute(ic, "WID", cocoaTimerGetWidAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
	iupClassRegisterAttribute(ic, "RUN", cocoaTimerGetRunAttrib, cocoaSetRunAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);


	
}


