/** \file
 * \brief Cocoa Driver - Thread Support
 *
 * See Copyright Notice in "iup.h"
 */

#import <Foundation/Foundation.h>

#include "iup.h"

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

IUP_SDK_API void* iupdrvThreadStart(Ihandle* ih)
{
  IupCocoaThread* thread = [[IupCocoaThread alloc] initWithIhandle:ih];

  const char* name = iupAttribGet(ih, "THREADNAME");
  if (name)
    [thread setName:[NSString stringWithUTF8String:name]];

  [thread start];
  return (void*)thread;
}

IUP_SDK_API void iupdrvThreadJoin(void* handle)
{
  IupCocoaThread* thread = (IupCocoaThread*)handle;
  while (![thread isFinished])
    [NSThread sleepForTimeInterval:0.001];
}

IUP_SDK_API void iupdrvThreadYield(void)
{
  [NSThread sleepForTimeInterval:0.0];
}

IUP_SDK_API int iupdrvThreadIsCurrent(void* handle)
{
  return (IupCocoaThread*)handle == [NSThread currentThread];
}

IUP_SDK_API void iupdrvThreadExit(int code)
{
  [NSThread exit];
}

IUP_SDK_API void iupdrvThreadDestroy(void* handle)
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

IUP_SDK_API void* iupdrvMutexCreate(void)
{
  return (void*)[[NSLock alloc] init];
}

IUP_SDK_API void iupdrvMutexLock(void* handle)
{
  [(NSLock*)handle lock];
}

IUP_SDK_API void iupdrvMutexUnlock(void* handle)
{
  [(NSLock*)handle unlock];
}

IUP_SDK_API void iupdrvMutexDestroy(void* handle)
{
  if (handle)
    [(NSLock*)handle release];
}
