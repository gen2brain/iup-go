/** \file
 * \brief IupFontDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>

#include <string.h>
#include <memory.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"

#include "iupcocoa_drv.h"

static NSMutableDictionary *attr = NULL;
static NSFont *font = NULL;

@interface MyFontPanel : NSFontPanel
- (void)setPreviewHeight:(CGFloat)height;
@end
@implementation MyFontPanel
- (void)setPreviewHeight:(CGFloat)height
{
//_fontPanelPreviewHeight = height;
}
@end

@interface IupCocoaMyDelegateForFontDialog : NSResponder <NSWindowDelegate>
- (void) changeFont:(id)sender;
- (void) changeAttributes:(id)sender;
- (void) onOkClicked:(id)sender;
- (void) onCancelClicked:(id)sender;
- (BOOL)windowShouldClose:(id)sender;
@end

@implementation IupCocoaMyDelegateForFontDialog
- (BOOL)windowShouldClose:(id)sender
{
  [NSApp stopModalWithCode:-1];
	return YES; //???
}
- (void) onOkClicked:(id)sender
{
  NSFontManager *fontManager = [NSFontManager sharedFontManager];
  NSFontPanel *panel = [fontManager fontPanel:YES];
  font = [fontManager convertFont:[fontManager selectedFont]];
  [panel close];
  [NSApp stopModalWithCode:0];
}
- (void) onCancelClicked:(id)sender
{
  NSFontManager *fontManager = [NSFontManager sharedFontManager];
  NSFontPanel *panel = [fontManager fontPanel:YES];
  [panel close];
  [NSApp stopModalWithCode:0];
}
- (void) changeAttributes:(id)sender
{
  NSFontManager *fontManager = [NSFontManager sharedFontManager];
  NSDictionary *attr2 = [sender convertAttributes:attr];
  [attr addEntriesFromDictionary:attr2];
}
- (void) changeFont:(id)sender
{
}
@end


static int macFontDlgPopup(Ihandle* ih, int x, int y)
{
#if 0
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  unsigned char r, g, b;

  char* standardfont;
  int height_pixels;
  char typeface[50] = "";
  int height = 8;
  int is_bold = 0,
    is_italic = 0, 
    is_underline = 0,
    is_strikeout = 0;
  int res = iupmacGetScreenRes();

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  standardfont = iupAttribGet(ih, "VALUE");
  if (!standardfont)
    standardfont = IupGetGlobal("DEFAULTFONT");

  /* parse the old format first */
  if (!iupFontParseWin(standardfont, typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
  {
    if (!iupFontParsePango(standardfont, typeface, &height, &is_bold, &is_italic, &is_underline, &is_strikeout))
      return IUP_ERROR;
  }

  /* get size in pixels */
  float factor = [[NSScreen mainScreen] userSpaceScaleFactor];
  height_pixels = (int)(height * factor);
  
  NSFont *hFont = [NSFont fontWithName:[NSString stringWithUTF8String:standardfont] size:height];
  attr = [[NSMutableDictionary alloc] init];

  if(is_italic)
    hFont = [[NSFontManager sharedFontManager] convertFont:hFont 
          toHaveTrait:NSItalicFontMask];
  if(is_bold)
    hFont = [[NSFontManager sharedFontManager] convertFont:hFont 
        toHaveTrait:NSBoldFontMask];
  if(is_underline)                           
  	[attr setValue:[NSNumber numberWithInt:NSUnderlinePatternSolid|NSUnderlineStyleSingle] 
	forKey:NSUnderlineStyleAttributeName];
  if(is_strikeout)                           
	[attr setValue:[NSNumber numberWithInt:NSUnderlinePatternSolid|NSUnderlineStyleSingle] 
	forKey:NSStrikethroughStyleAttributeName]; 

  NSView *accView = [[NSView alloc] initWithFrame:NSMakeRect(0,0,180,80)];
  NSButton *button1 = [[NSButton alloc] initWithFrame: NSMakeRect( 0.0, 20.0, 80.0, 50.0 ) ];
  NSButton *button2 = [[NSButton alloc] initWithFrame: NSMakeRect( 90.0, 20.0, 80.0, 50.0 ) ];
  [accView addSubview:button1];
  [accView addSubview:button2];

  MyDelegate *delegate = [MyDelegate new];

  [button1 setBezelStyle:NSRoundedBezelStyle];
  [button1 setTitle: @"OK" ];
  [button1 setAction: @selector(onOkClicked:)];
  [button1 setTarget: delegate];
  [button2 setBezelStyle:NSRoundedBezelStyle];
  [button2 setTitle: @"Cancel" ];
  [button2 setAction: @selector(onCancelClicked:)];
  [button2 setTarget: delegate];

  [NSFontManager setFontPanelFactory:[MyFontPanel class]];
  NSFontManager *fontManager = [NSFontManager sharedFontManager];
  [fontManager setTarget: delegate];
  [fontManager setDelegate: delegate];
  [fontManager setSelectedFont:hFont isMultiple: NO];

  NSFontPanel *panel = [fontManager fontPanel:YES];
  [panel setReleasedWhenClosed:YES]; 
  [panel setAccessoryView: accView];
  [panel setEnabled:YES];
  [panel setDefaultButtonCell:[button1 cell]];
  [panel makeFirstResponder:delegate];
  [panel setDelegate:delegate];
  [panel orderFrontRegardless];
  [fontManager orderFrontFontPanel:delegate];
  NSInteger result = 0;

	
	NSModalSession session = [NSApp beginModalSessionForWindow:[self window]];
	
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
    iupAttribSetStr(ih, "COLOR", NULL);
    iupAttribSetStr(ih, "STATUS", NULL);
    return IUP_NOERROR;
  }
  NSFontTraitMask mask = [fontManager traitsOfFont:font];
  is_bold = (mask & NSBoldFontMask) ? 1 : 0;
  is_italic = (mask & NSItalicFontMask) ? 1 : 0;
  is_underline = [attr valueForKey:NSUnderlineStyleAttributeName] ? 1 : 0;
  is_strikeout = [attr valueForKey:NSStrikethroughStyleAttributeName] ? 1 : 0;
  height = [font pointSize];
  char *faceName = [[font familyName] UTF8String];
  iupAttribSetStrf(ih, "VALUE", "%s, %s%s%s%s %d", faceName,
                                                    is_bold?"Bold ":"",
                                                    is_italic?"Italic ":"",
                                                    is_underline?"Underline ":"",
                                                    is_strikeout?"Strikeout ":"",
                                                    height);
  NSColor *color = [attr valueForKey:NSForegroundColorAttributeName];
  CGFloat red=0,green=0,blue=0,alpha=0;
  if(color)
	[color getRed:&red green:&green blue:&blue alpha:&alpha];
 
  iupAttribSetStrf(ih, "COLOR", "%d %d %d", (int)(red*255),(int)(green*255),(int)(blue*255));
  iupAttribSetStr(ih, "STATUS", "1");
#endif
	
  return IUP_NOERROR;
}

void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = macFontDlgPopup;
}
