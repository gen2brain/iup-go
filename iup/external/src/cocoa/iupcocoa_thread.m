/** \file
 * \brief Cocoa Driver - Thread Support
 *
 * See Copyright Notice in "iup.h"
 */

#import <Foundation/Foundation.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_thread.h"

@interface IupCocoaThread : NSThread
{
  Ihandle* _ih;
}
- (instancetype)initWithIhandle:(Ihandle*)ih;
@end

@implementation IupCocoaThread

- (instancetype)initWithIhandle:(Ihandle*)ih
{
  self = [super init];
  if (self)
    _ih = ih;
  return self;
}

- (void)main
{
  Icallback cb = IupGetCallback(_ih, "THREAD_CB");
  if (cb)
    cb(_ih);
}

@end

void* iupdrvThreadStart(Ihandle* ih)
{
  IupCocoaThread* thread = [[IupCocoaThread alloc] initWithIhandle:ih];

  const char* name = iupAttribGet(ih, "THREADNAME");
  if (name)
    [thread setName:[NSString stringWithUTF8String:name]];

  [thread start];
  return (void*)thread;
}

void iupdrvThreadJoin(void* handle)
{
  IupCocoaThread* thread = (IupCocoaThread*)handle;
  while (![thread isFinished])
    [NSThread sleepForTimeInterval:0.001];
}

void iupdrvThreadYield(void)
{
  [NSThread sleepForTimeInterval:0.0];
}

int iupdrvThreadIsCurrent(void* handle)
{
  return (IupCocoaThread*)handle == [NSThread currentThread];
}

void iupdrvThreadExit(int code)
{
  [NSThread exit];
}

void iupdrvThreadDestroy(void* handle)
{
  if (handle)
  {
    IupCocoaThread* thread = (IupCocoaThread*)handle;
    if ([thread isExecuting])
    {
      while (![thread isFinished])
        [NSThread sleepForTimeInterval:0.001];
    }
    [thread release];
  }
}

void* iupdrvMutexCreate(void)
{
  return (void*)[[NSLock alloc] init];
}

void iupdrvMutexLock(void* handle)
{
  [(NSLock*)handle lock];
}

void iupdrvMutexUnlock(void* handle)
{
  [(NSLock*)handle unlock];
}

void iupdrvMutexDestroy(void* handle)
{
  if (handle)
    [(NSLock*)handle release];
}
