/** \file
 * \brief File Descriptor Watch for the CocoaTouch Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_fdwatch.h"

#include "iupcocoatouch_drv.h"


typedef struct _IupCocoaTouchFdWatch
{
  CFFileDescriptorRef fdref;
  CFRunLoopSourceRef source;
  CFOptionFlags callbacks;
} IupCocoaTouchFdWatch;

static void cocoaTouchFdWatchProc(CFFileDescriptorRef fdref, CFOptionFlags callback_types, void* info)
{
  Ihandle* ih = (Ihandle*)info;
  IupCocoaTouchFdWatch* watch;

  (void)callback_types;

  if (!iupObjectCheck(ih))
    return;

  iupFdWatchCallAction(ih);

  /* CFFileDescriptor disables the callback after every fire */
  watch = (IupCocoaTouchFdWatch*)iupAttribGet(ih, "_IUPCOCOATOUCH_FDWATCH");
  if (watch && watch->fdref == fdref)
    CFFileDescriptorEnableCallBacks(fdref, watch->callbacks);
}

IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih)
{
  int mode = iupFdWatchGetMode(ih);
  CFFileDescriptorContext context = { 0, (void*)ih, NULL, NULL, NULL };
  IupCocoaTouchFdWatch* watch;
  CFOptionFlags callbacks = 0;

  if (iupAttribGet(ih, "_IUPCOCOATOUCH_FDWATCH"))
    return 1;

  if (mode & IUP_FDWATCH_READ)
    callbacks |= kCFFileDescriptorReadCallBack;
  if (mode & IUP_FDWATCH_WRITE)
    callbacks |= kCFFileDescriptorWriteCallBack;

  watch = (IupCocoaTouchFdWatch*)calloc(1, sizeof(IupCocoaTouchFdWatch));
  if (!watch)
    return 0;

  watch->callbacks = callbacks;
  watch->fdref = CFFileDescriptorCreate(kCFAllocatorDefault, iupFdWatchGetFd(ih), false, cocoaTouchFdWatchProc, &context);
  if (!watch->fdref)
  {
    free(watch);
    return 0;
  }

  CFFileDescriptorEnableCallBacks(watch->fdref, callbacks);
  watch->source = CFFileDescriptorCreateRunLoopSource(kCFAllocatorDefault, watch->fdref, 0);
  if (!watch->source)
  {
    CFRelease(watch->fdref);
    free(watch);
    return 0;
  }

  CFRunLoopAddSource(CFRunLoopGetMain(), watch->source, kCFRunLoopCommonModes);
  iupAttribSet(ih, "_IUPCOCOATOUCH_FDWATCH", (char*)watch);
  return 1;
}

IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih)
{
  IupCocoaTouchFdWatch* watch = (IupCocoaTouchFdWatch*)iupAttribGet(ih, "_IUPCOCOATOUCH_FDWATCH");
  if (!watch)
    return;

  CFRunLoopRemoveSource(CFRunLoopGetMain(), watch->source, kCFRunLoopCommonModes);
  CFFileDescriptorDisableCallBacks(watch->fdref, watch->callbacks);
  CFFileDescriptorInvalidate(watch->fdref);
  CFRelease(watch->source);
  CFRelease(watch->fdref);
  free(watch);

  iupAttribSet(ih, "_IUPCOCOATOUCH_FDWATCH", NULL);
}

IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic)
{
  (void)ic;
}
