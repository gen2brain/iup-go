/** \file
 * \brief IupShareDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_stdcontrols.h"


@interface IupCocoaShareDelegate : NSObject <NSSharingServicePickerDelegate, NSSharingServiceDelegate>
{
@public
  int status;
}
@end

@implementation IupCocoaShareDelegate

- (void)sharingServicePicker:(NSSharingServicePicker*)picker didChooseSharingService:(NSSharingService*)service
{
  (void)picker;
  if (!service)
    status = 0;
}

- (id<NSSharingServiceDelegate>)sharingServicePicker:(NSSharingServicePicker*)picker delegateForSharingService:(NSSharingService*)service
{
  (void)picker; (void)service;
  return self;
}

- (void)sharingService:(NSSharingService*)service didShareItems:(NSArray*)items
{
  (void)service; (void)items;
  status = 1;
}

- (void)sharingService:(NSSharingService*)service didFailToShareItems:(NSArray*)items error:(NSError*)error
{
  (void)service; (void)items;
  status = ([error code] == NSUserCancelledError) ? 0 : -1;
}

@end

static int cocoaShareDlgPopup(Ihandle* ih, int x, int y)
{
  NSMutableArray* items = [NSMutableArray array];
  const char* text = iupAttribGet(ih, "TEXT");
  const char* url = iupAttribGet(ih, "URL");
  const char* file = iupAttribGet(ih, "FILE");
  NSWindow* window = [NSApp keyWindow];
  IupCocoaShareDelegate* delegate;
  NSSharingServicePicker* picker;
  NSView* view;
  NSRect rect;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  if (!window)
    window = [NSApp mainWindow];

  if (text)
    [items addObject:[NSString stringWithUTF8String:text]];
  if (url)
  {
    NSURL* nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    if (nsurl)
      [items addObject:nsurl];
  }
  if (file)
    [items addObject:[NSURL fileURLWithPath:[NSString stringWithUTF8String:file]]];

  if ([items count] == 0 || !window)
  {
    iupAttribSet(ih, "STATUS", "-1");
    return IUP_ERROR;
  }

  view = [window contentView];
  rect = NSMakeRect(NSMidX([view bounds]), NSMidY([view bounds]), 1, 1);

  delegate = [[IupCocoaShareDelegate alloc] init];
  delegate->status = 2;

  picker = [[NSSharingServicePicker alloc] initWithItems:items];
  [picker setDelegate:delegate];
  [picker showRelativeToRect:rect ofView:view preferredEdge:NSMinYEdge];

  while (delegate->status == 2)
  {
    @autoreleasepool {
      NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate dateWithTimeIntervalSinceNow:0.05] inMode:NSDefaultRunLoopMode dequeue:YES];
      if (event)
        [NSApp sendEvent:event];
    }
  }

  iupAttribSet(ih, "STATUS", delegate->status == 1 ? "1" : (delegate->status == 0 ? "0" : "-1"));

  [picker setDelegate:nil];
  [picker release];
  [delegate release];
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvShareDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = cocoaShareDlgPopup;
}
