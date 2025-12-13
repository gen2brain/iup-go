/** \file
 * \brief macOS Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#import <Cocoa/Cocoa.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_str.h"
#include "iup_loop.h"
#include "iupcocoa_drv.h"

static IFidle mac_idle_cb = NULL;
static int mac_main_loop_should_quit = 0;
static int mac_main_loop_level = 0;

void iupdrvSetIdleFunction(Icallback f)
{
  mac_idle_cb = (IFidle)f;
}

static int macLoopCallIdle(void)
{
  int ret = mac_idle_cb();
  if (ret == IUP_CLOSE)
  {
    mac_idle_cb = NULL;
    IupExitLoop();
    return IUP_CLOSE;
  }
  if (ret == IUP_IGNORE)
    mac_idle_cb = NULL;
  return ret;
}

void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");
  if (!exit_loop || iupStrBoolean(exit_loop))
  {
    mac_main_loop_should_quit = 1;

    NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                        location:NSMakePoint(0, 0)
                                   modifierFlags:0
                                       timestamp:0
                                    windowNumber:0
                                         context:nil
                                         subtype:0
                                           data1:0
                                           data2:0];
    [NSApp postEvent:event atStart:NO];
  }
}

static int macLoopProcessMessage(NSEvent* event)
{
  [NSApp sendEvent:event];
  return IUP_DEFAULT;
}

int IupMainLoopLevel(void)
{
  return mac_main_loop_level;
}

int IupMainLoop(void)
{
  static int has_done_entry = 0;

  mac_main_loop_level++;

  if (0 == has_done_entry)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  mac_main_loop_should_quit = 0;

  while (!mac_main_loop_should_quit)
  {
    @autoreleasepool {
      if (mac_idle_cb)
      {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate dateWithTimeIntervalSinceNow:0.0]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
        if (event != nil)
        {
          macLoopProcessMessage(event);
        }
        else
        {
          macLoopCallIdle();
        }
      }
      else
      {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate distantFuture]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
        if (event != nil)
        {
          macLoopProcessMessage(event);
        }
      }
    }
  }

  /* If we are exiting a nested loop, reset the quit flag so the parent loop can continue execution. */
  if (mac_main_loop_level > 1 && mac_main_loop_should_quit)
  {
    mac_main_loop_should_quit = 0;
  }

  mac_main_loop_level--;

  if (mac_main_loop_level == 0)
  {
    iupLoopCallExitCb();
  }

  return IUP_NOERROR;
}

int IupLoopStepWait(void)
{
  NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                      untilDate:[NSDate distantFuture]
                                         inMode:NSDefaultRunLoopMode
                                        dequeue:YES];
  if (event != nil)
    macLoopProcessMessage(event);

  if (mac_main_loop_should_quit)
    return IUP_CLOSE;

  return IUP_DEFAULT;
}

int IupLoopStep(void)
{
  NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                      untilDate:[NSDate dateWithTimeIntervalSinceNow:0.0]
                                         inMode:NSDefaultRunLoopMode
                                        dequeue:YES];
  if (event != nil)
    macLoopProcessMessage(event);
  else if (mac_idle_cb)
    macLoopCallIdle();

  if (mac_main_loop_should_quit)
    return IUP_CLOSE;

  return IUP_DEFAULT;
}

void IupFlush(void)
{
  while (YES)
  {
    NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate dateWithTimeIntervalSinceNow:0.0]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    if (event == nil)
      break;

    macLoopProcessMessage(event);

    /* If a callback requested an exit, stop flushing */
    if (mac_main_loop_should_quit)
      break;
  }
}

void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  char* s_copy = iupStrDup(s);
  dispatch_async(dispatch_get_main_queue(),
      ^{
      IFnsidv cb = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
      if (cb)
      {
        cb(ih, s_copy, i, d, p);
      }
        free(s_copy);
      }
      );
}
