/** \file
 * \brief IupColorDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"

@interface IupCocoaMyDelegateForColorDialog : NSResponder <NSWindowDelegate>
- (void) changeFont:(id)sender;
- (void) changeAttributes:(id)sender;
- (void) onOkClicked:(id)sender;
- (void) onCancelClicked:(id)sender;
- (BOOL)windowShouldClose:(id)sender;
@end

@implementation IupCocoaMyDelegateForColorDialog
- (BOOL)windowShouldClose:(id)sender
{
NSColorPanel *panel = [NSColorPanel sharedColorPanel];
[panel close];
[NSApp stopModalWithCode:-1];
	return YES; //???
}
- (void) onOkClicked:(id)sender
{
NSColorPanel *panel = [NSColorPanel sharedColorPanel];
[panel close];
[NSApp stopModalWithCode:0];
}
- (void) onCancelClicked:(id)sender
{
NSColorPanel *panel = [NSColorPanel sharedColorPanel];
[panel close];
[NSApp stopModalWithCode:-1];
}
@end

static int macColorDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  unsigned char r, g, b;
  char* value;

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  iupStrToRGB(iupAttribGet(ih, "VALUE"), &r, &g, &b);
  NSColor *color = [NSColor colorWithCalibratedRed:(r/255.0) green:(g/255.0) blue:(b/255.0) alpha:1];

  NSView *accView = [[NSView alloc] initWithFrame:NSMakeRect(0,0,180,80)];
  NSButton *button1 = [[NSButton alloc] initWithFrame: NSMakeRect( 0.0, 20.0, 80.0, 50.0 ) ];
  NSButton *button2 = [[NSButton alloc] initWithFrame: NSMakeRect( 90.0, 20.0, 80.0, 50.0 ) ];
  [accView addSubview:button1];
  [accView addSubview:button2];

  IupCocoaMyDelegateForColorDialog *delegate = [IupCocoaMyDelegateForColorDialog new];

  [button1 setBezelStyle:NSRoundedBezelStyle];
  [button1 setTitle: @"OK" ];
  [button1 setAction: @selector(onOkClicked:)];
  [button1 setTarget: delegate];
  [button2 setBezelStyle:NSRoundedBezelStyle];
  [button2 setTitle: @"Cancel" ];
  [button2 setAction: @selector(onCancelClicked:)];
  [button2 setTarget: delegate];

  NSColorPanel *panel = [NSColorPanel sharedColorPanel];
  [panel setDefaultButtonCell:[button1 cell]];
  [panel setAccessoryView: accView];
  [panel makeFirstResponder:delegate];
  [panel setDelegate:delegate];
  [panel orderFrontRegardless];
  NSModalSession session = [NSApp beginModalSessionForWindow:panel];
  NSInteger result = 0;
  for (;;) {
    result = [NSApp runModalSession:session];
    if(result != NSRunContinuesResponse)
      break;
    NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask
                untilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]
                inMode:NSDefaultRunLoopMode
                dequeue:YES];
    [NSApp sendEvent:event];
    //[NSApp updateWindows];
  }
  [NSApp endModalSession:session];
  if(result != 0)
  {
    iupAttribSetStr(ih, "VALUE", NULL);
    iupAttribSetStr(ih, "COLORTABLE", NULL);
    iupAttribSetStr(ih, "STATUS", NULL);
    return IUP_NOERROR;
  }
  color = [[panel color] colorUsingColorSpaceName:NSCalibratedRGBColorSpace];  
  CGFloat red=0,green=0,blue=0,alpha=0;
  [color getRed:&red green:&green blue:&blue alpha:&alpha];
 
  iupAttribSetStrf(ih, "VALUE", "%d %d %d", (int)(red*255),(int)(green*255),(int)(blue*255));
  iupAttribSetStr(ih, "COLORTABLE", NULL);
  iupAttribSetStr(ih, "STATUS", "1");

  return IUP_NOERROR;
}

void iupdrvColorDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = macColorDlgPopup;
}
