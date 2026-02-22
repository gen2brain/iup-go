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

#include "iup_drvfont.h"
#include "iupcocoa_drv.h"


@interface IupCocoaFontDialogDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) Ihandle* ih;
- (void) onOkClicked:(id)sender;
- (void) onCancelClicked:(id)sender;
- (void) onHelpClicked:(id)sender;
- (void) changeFont:(id)sender;
- (BOOL) windowShouldClose:(id)sender;
@end

@implementation IupCocoaFontDialogDelegate
@synthesize ih;

- (void) onOkClicked:(id)sender
{
  NSFontPanel* panel = [[NSFontManager sharedFontManager] fontPanel:YES];
  [panel orderOut:nil];
  [NSApp stopModalWithCode:NSModalResponseOK];
}

- (void) onCancelClicked:(id)sender
{
  NSFontPanel* panel = [[NSFontManager sharedFontManager] fontPanel:YES];
  [panel orderOut:nil];
  [NSApp stopModalWithCode:NSModalResponseCancel];
}

- (void) onHelpClicked:(id)sender
{
  if (self.ih)
  {
    Icallback cb = IupGetCallback(self.ih, "HELP_CB");
    if (cb && cb(self.ih) == IUP_CLOSE)
    {
      [self onCancelClicked:sender];
    }
  }
}

/* An action must be set for the panel to work. */
- (void) changeFont:(id)sender
{
  (void)sender;
}

- (BOOL)windowShouldClose:(id)sender
{
  [self onCancelClicked:sender];
  return NO;
}
@end


static int cocoaFontDlgPopup(Ihandle* ih, int x, int y)
{
  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  char* value = iupAttribGet(ih, "VALUE");
  if (!value)
  {
    value = IupGetGlobal("DEFAULTFONT");
  }

  char typeface[100] = "";
  int size = 12;
  int is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  if (!iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
  {
    iupGetFontInfo(IupGetGlobal("DEFAULTFONT"), typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout);
  }

  NSFont* font = nil;

  /* System font names starting with '.' are internal and cannot be used with fontWithName:.
     Use the proper system font API instead. */
  if (typeface[0] == '.' || strcmp(typeface, "System") == 0 || strlen(typeface) == 0)
  {
    font = [NSFont systemFontOfSize:(CGFloat)size];
  }
  else
  {
    NSString* ns_typeface = [NSString stringWithUTF8String:typeface];
    font = [NSFont fontWithName:ns_typeface size:(CGFloat)size];
  }

  if (!font)
  {
    font = [NSFont systemFontOfSize:(CGFloat)size];
  }

  NSFontManager* fontManager = [NSFontManager sharedFontManager];
  NSFontTraitMask trait_mask = 0;
  if (is_bold) trait_mask |= NSBoldFontMask;
  if (is_italic) trait_mask |= NSItalicFontMask;
  if (trait_mask)
  {
    font = [fontManager convertFont:font toHaveTrait:trait_mask];
  }

  IupCocoaFontDialogDelegate* delegate = [[IupCocoaFontDialogDelegate alloc] init];
  delegate.ih = ih;

  [fontManager setTarget:delegate];
  [fontManager setAction:@selector(changeFont:)];
  [fontManager setSelectedFont:font isMultiple:NO];

  NSFontPanel* panel = [fontManager fontPanel:YES];
  [panel setDelegate:delegate];

  char* title_str = iupAttribGet(ih, "TITLE");
  if (title_str)
  {
    [panel setTitle:[NSString stringWithUTF8String:title_str]];
  }

  CGFloat view_width = 260;
  CGFloat button_height = 32;
  CGFloat ok_button_width = 82;
  CGFloat cancel_button_width = 82;
  CGFloat ok_cancel_spacing = 6;

  NSView* accessoryView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, view_width, button_height)];

  NSButton* okButton = [[NSButton alloc] initWithFrame:NSMakeRect(view_width - ok_button_width, 0, ok_button_width, button_height)];
  [okButton setBezelStyle:NSBezelStyleRounded];
  [okButton setTitle:@"OK"];
  [okButton setKeyEquivalent:@"\r"];
  [okButton setAction:@selector(onOkClicked:)];
  [okButton setTarget:delegate];

  NSButton* cancelButton = [[NSButton alloc] initWithFrame:NSMakeRect(view_width - ok_button_width - ok_cancel_spacing - cancel_button_width, 0, cancel_button_width, button_height)];
  [cancelButton setBezelStyle:NSBezelStyleRounded];
  [cancelButton setTitle:@"Cancel"];
  [cancelButton setKeyEquivalent:@"\e"];
  [cancelButton setAction:@selector(onCancelClicked:)];
  [cancelButton setTarget:delegate];

  [accessoryView addSubview:okButton];
  [accessoryView addSubview:cancelButton];

  if (IupGetCallback(ih, "HELP_CB"))
  {
    NSButton* helpButton = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 32, 32)];
    [helpButton setBezelStyle:NSBezelStyleHelpButton];
    [helpButton setTitle:@""];
    [helpButton setAction:@selector(onHelpClicked:)];
    [helpButton setTarget:delegate];
    [accessoryView addSubview:helpButton];
    [helpButton release];
  }

  [panel setAccessoryView:accessoryView];

  [okButton release];
  [cancelButton release];
  [accessoryView release];

  NSInteger result = [NSApp runModalForWindow:panel];

  if (result == NSModalResponseOK)
  {
    NSFont* finalFont = [fontManager convertFont:[fontManager selectedFont]];
    NSDictionary* finalAttributes = [fontManager convertAttributes:@{}];

    NSFontTraitMask traits = [fontManager traitsOfFont:finalFont];
    is_bold = (traits & NSBoldFontMask) != 0;
    is_italic = (traits & NSItalicFontMask) != 0;

    is_underline = [finalAttributes objectForKey:NSUnderlineStyleAttributeName] != nil;
    is_strikeout = [finalAttributes objectForKey:NSStrikethroughStyleAttributeName] != nil;

    size = (int)[finalFont pointSize];
    const char* faceName = [[finalFont familyName] UTF8String];

    iupAttribSetStrf(ih, "VALUE", "%s, %s%s%s%s %d", faceName,
                      is_bold ? "Bold " : "",
                      is_italic ? "Italic " : "",
                      is_underline ? "Underline " : "",
                      is_strikeout ? "Strikeout " : "",
                      size);

    if (iupAttribGetBoolean(ih, "SHOWCOLOR"))
    {
      NSColor* color = [finalAttributes objectForKey:NSForegroundColorAttributeName];
      if (color)
      {
        color = [color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
        CGFloat red = 0, green = 0, blue = 0;
        [color getRed:&red green:&green blue:&blue alpha:NULL];
        iupAttribSetStrf(ih, "COLOR", "%d %d %d", (int)(red * 255), (int)(green * 255), (int)(blue * 255));
      }
      else
      {
        iupAttribSet(ih, "COLOR", NULL);
      }
    }
    else
    {
      iupAttribSet(ih, "COLOR", NULL);
    }
    iupAttribSet(ih, "STATUS", "1");
  }
  else
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "COLOR", NULL);
    iupAttribSet(ih, "STATUS", NULL);
  }

  [panel setAccessoryView:nil];
  [panel setDelegate:nil];
  [fontManager setTarget:nil];
  [delegate release];

  return IUP_NOERROR;
}

void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = cocoaFontDlgPopup;

  iupClassRegisterAttribute(ic, "COLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PREVIEWTEXT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}