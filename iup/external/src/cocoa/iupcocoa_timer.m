/** \file
 * \brief Timer for the macOS Driver.
 *
 * See Copyright Notice in "iup.h"
 */

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
@property (assign) CFTimeInterval startTime;
@property (retain) NSTimer* nsTimer;
- (void) onTimerCallback:(NSTimer*)timer;
@end


@implementation IupCocoaTimerController

- (void) dealloc
{
  [_nsTimer release];
  _nsTimer = nil;
  [super dealloc];
}

- (void) onTimerCallback:(NSTimer*)timer
{
  Ihandle* ih = (Ihandle*)[timer.userInfo pointerValue];

  if (!iupObjectCheck(ih))
  {
    [timer invalidate];
    return;
  }

  Icallback action_cb = IupGetCallback(ih, "ACTION_CB");
  if (action_cb)
  {
    CFTimeInterval start_time = [self startTime];
    double current_time = CACurrentMediaTime();
    int elapsed_time_ms = (int)(((current_time - start_time) * 1000.0) + 0.5);
    iupAttribSetInt(ih, "ELAPSEDTIME", elapsed_time_ms);

    if (action_cb(ih) == IUP_CLOSE)
    {
      IupExitLoop();
    }
  }
}

@end


void iupdrvTimerRun(Ihandle* ih)
{
  if (ih->handle != nil)
    return;

  unsigned int time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms > 0)
  {
    IupCocoaTimerController* timer_controller = [[IupCocoaTimerController alloc] init];

    CFTimeInterval start_time = CACurrentMediaTime();

    NSTimer* ns_timer = [NSTimer timerWithTimeInterval:(time_ms / 1000.0)
                                                target:timer_controller
                                              selector:@selector(onTimerCallback:)
                                              userInfo:[NSValue valueWithPointer:ih]
                                               repeats:YES];

    float tolerance = iupAttribGetFloat(ih, "TOLERANCE");
    if (tolerance > 0.0f)
    {
      [ns_timer setTolerance:(NSTimeInterval)tolerance];
    }

    [[NSRunLoop currentRunLoop] addTimer:ns_timer forMode:NSRunLoopCommonModes];

    [timer_controller setNsTimer:ns_timer];
    [timer_controller setStartTime:start_time];

    ih->handle = timer_controller;
  }
}

void iupdrvTimerStop(Ihandle* ih)
{
  if (ih->handle != nil)
  {
    IupCocoaTimerController* timer_controller = (IupCocoaTimerController*)ih->handle;
    NSTimer* ns_timer = [timer_controller nsTimer];

    [ns_timer invalidate];
    [timer_controller release];

    ih->handle = nil;
  }
}

static int cocoaSetRunAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
    iupdrvTimerRun(ih);
  else
    iupdrvTimerStop(ih);

  return 0;
}

/* The base implementation uses ih->serial, but we use ih->handle. */
static char* cocoaTimerGetRunAttrib(Ihandle *ih)
{
  return iupStrReturnBoolean(ih->handle != nil);
}

static char* cocoaTimerGetWidAttrib(Ihandle *ih)
{
  /* WARNING: This truncates the controller pointer on 64-bit architectures.
     It should only be used to check for a non-NULL value. */
  return iupStrReturnInt((int)(intptr_t)ih->handle);
}

void iupdrvTimerInitClass(Iclass* ic)
{
  ic->UnMap = iupdrvTimerStop;

  iupClassRegisterAttribute(ic, "WID", cocoaTimerGetWidAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "RUN", cocoaTimerGetRunAttrib, cocoaSetRunAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  /*
   * TOLERANCE is a macOS-specific attribute to improve power consumption by allowing the system to fire the timer later than scheduled.
   * The value is a float in seconds.
   */
  iupClassRegisterAttribute(ic, "TOLERANCE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
}