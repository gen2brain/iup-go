/** \file
 * \brief Button Control
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"

#include "iupcocoa_drv.h"
#include "iupcocoa_keycodes.h"

static const CGFloat kIupCocoaDefaultWidthNSButton = 64.0;
static const CGFloat kIupCocoaDefaultHeightNSButton = 32.0;
static const void* IUP_COCOA_BUTTON_RECEIVER_OBJ_KEY = @"IUP_COCOA_BUTTON_RECEIVER_OBJ_KEY";

@interface IupCocoaFlatButton : NSButton
@property (nonatomic, assign) BOOL isFlat;
@property (nonatomic, assign) BOOL isHovering;
@end

@implementation IupCocoaFlatButton

- (void)updateTrackingAreas
{
  [super updateTrackingAreas];

  for (NSTrackingArea *area in [self trackingAreas])
  {
    [self removeTrackingArea:area];
  }

  NSTrackingArea *trackingArea = [[NSTrackingArea alloc]
    initWithRect:[self bounds]
         options:(NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                  NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect)
           owner:self
        userInfo:nil];

  [self addTrackingArea:trackingArea];
  [trackingArea release];
}

- (void)mouseEntered:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  if (self.isFlat && !self.isHovering && [self isEnabled])
  {
    self.isHovering = YES;
    [self setBordered:YES];
    [self setNeedsDisplay:YES];
  }

  IFn cb = (IFn)IupGetCallback(ih, "ENTERWINDOW_CB");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

- (void)mouseExited:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  if (self.isFlat && self.isHovering)
  {
    self.isHovering = NO;
    [self setBordered:NO];
    [self setNeedsDisplay:YES];
  }

  IFn cb = (IFn)IupGetCallback(ih, "LEAVEWINDOW_CB");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

- (void)mouseMoved:(NSEvent *)event
{
  [super mouseMoved:event];

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
    iupCocoaCommonBaseHandleMouseMotionCallback(ih, event, self);
}

- (void)mouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
    iupCocoaCommonBaseHandleMouseButtonCallback(ih, event, self, true);

  [self highlight:YES];

  BOOL keepTracking = YES;
  BOOL mouseIsInside = YES;
  NSPoint currentLocation;

  while (keepTracking)
  {
    NSEvent* nextEvent = [[self window] nextEventMatchingMask:(NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged)];

    switch ([nextEvent type])
    {
      case NSEventTypeLeftMouseDragged:
        currentLocation = [self convertPoint:[nextEvent locationInWindow] fromView:nil];
        mouseIsInside = [self mouse:currentLocation inRect:[self bounds]];
        [self highlight:mouseIsInside];
        break;

      case NSEventTypeLeftMouseUp:
        currentLocation = [self convertPoint:[nextEvent locationInWindow] fromView:nil];
        mouseIsInside = [self mouse:currentLocation inRect:[self bounds]];

        [self highlight:NO];

        if (ih)
          iupCocoaCommonBaseHandleMouseButtonCallback(ih, nextEvent, self, false);

        if (mouseIsInside)
          [self performClick:self];

        keepTracking = NO;
        break;

      default:
        break;
    }
  }
}

- (void)mouseUp:(NSEvent *)event
{
  /* This method is kept for right and other mouse buttons which don't use custom tracking */
  [super mouseUp:event];

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
    iupCocoaCommonBaseHandleMouseButtonCallback(ih, event, self, false);
}

- (void)rightMouseDown:(NSEvent *)event
{
  [super rightMouseDown:event];

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
    iupCocoaCommonBaseHandleMouseButtonCallback(ih, event, self, true);
}

- (void)rightMouseUp:(NSEvent *)event
{
  [super rightMouseUp:event];

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
    iupCocoaCommonBaseHandleMouseButtonCallback(ih, event, self, false);
}

- (void)otherMouseDown:(NSEvent *)event
{
  [super otherMouseDown:event];

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
    iupCocoaCommonBaseHandleMouseButtonCallback(ih, event, self, true);
}

- (void)otherMouseUp:(NSEvent *)event
{
  [super otherMouseUp:event];

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
    iupCocoaCommonBaseHandleMouseButtonCallback(ih, event, self, false);
}

- (void)scrollWheel:(NSEvent *)event
{
  [super scrollWheel:event];

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    if (!iupCocoaCommonBaseScrollWheelCallback(ih, event, self))
      [super scrollWheel:event];
  }
}

- (void)keyDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    int mac_key_code = [event keyCode];

    /* Enter activates the button when it has focus */
    if (mac_key_code == kVK_Return)
    {
      [self performClick:nil];
      return;
    }

    if (!iupCocoaKeyEvent(ih, event, mac_key_code, true))
      [super keyDown:event];
  }
  else
    [super keyDown:event];
}

- (void)keyUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    int mac_key_code = [event keyCode];
    if (!iupCocoaKeyEvent(ih, event, mac_key_code, false))
      [super keyUp:event];
  }
  else
    [super keyUp:event];
}

- (void)flagsChanged:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    int mac_key_code = [event keyCode];
    if (!iupCocoaModifierEvent(ih, event, mac_key_code))
      [super flagsChanged:event];
  }
  else
    [super flagsChanged:event];
}

- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if (result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
    if (ih)
      iupCocoaFocusIn(ih);
  }
  return result;
}

- (BOOL)resignFirstResponder
{
  BOOL result = [super resignFirstResponder];
  if (result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
    if (ih)
      iupCocoaFocusOut(ih);
  }
  return result;
}

- (BOOL)acceptsFirstResponder
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    if (iupAttribGet(ih, "_IUPCOCOA_CANFOCUS"))
      return iupAttribGetBoolean(ih, "_IUPCOCOA_CANFOCUS");
    return iupAttribGetBoolean(ih, "CANFOCUS");
  }
  return [super acceptsFirstResponder];
}

- (BOOL) needsPanelToBecomeKey
{
  return YES;
}

@end


@interface IupCocoaButtonReceiver : NSObject
   - (IBAction)myButtonClickAction:(id)the_sender;
   @end

   @implementation IupCocoaButtonReceiver

   - (IBAction)myButtonClickAction:(id)the_sender
{
  Icallback callback_function;
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);

  callback_function = IupGetCallback(ih, "ACTION");
  if(callback_function)
  {
    if(callback_function(ih) == IUP_CLOSE)
    {
      IupExitLoop();
    }
  }
}
@end

static void cocoaButtonCalculateBorders(Ihandle* ih, int *border_x, int *border_y)
{
  NSButton* temp_button = [[NSButton alloc] initWithFrame:NSZeroRect];
  BOOL is_flat = iupAttribGetBoolean(ih, "FLAT");
  BOOL has_impress_no_border = (ih->data->type & IUP_BUTTON_IMAGE) && iupAttribGet(ih, "IMPRESS") && !iupAttribGetBoolean(ih, "IMPRESSBORDER");

  [temp_button setBezelStyle:NSRegularSquareBezelStyle];
  [temp_button setFont:[NSFont systemFontOfSize:0]];

  if (has_impress_no_border || is_flat)
  {
    [temp_button setBordered:NO];
  }
  else
  {
    [temp_button setBordered:YES];
  }

  NSImage *temp_image;
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    temp_image = [[NSImage alloc] initWithSize:NSMakeSize(16, 16)];

    if (ih->data->type & IUP_BUTTON_TEXT)
    {
      [temp_button setTitle:@"Test"];
      [temp_button setImage:temp_image];

      NSCellImagePosition position;
      switch(ih->data->img_position)
      {
        case IUP_IMGPOS_TOP:
          position = NSImageAbove;
          break;
        case IUP_IMGPOS_BOTTOM:
          position = NSImageBelow;
          break;
        case IUP_IMGPOS_RIGHT:
          position = NSImageRight;
          break;
        case IUP_IMGPOS_LEFT:
        default:
          position = NSImageLeft;
          break;
      }
      [temp_button setImagePosition:position];
    }
    else
    {
      [temp_button setImage:temp_image];
      [temp_button setImagePosition:NSImageOnly];
    }
  }
  else
  {
    [temp_button setTitle:@"Test"];
    [temp_button setImagePosition:NSNoImage];
  }

  NSSize fitting_size = [temp_button fittingSize];
  NSSize intrinsic_size = [temp_button intrinsicContentSize];

  if (intrinsic_size.width > 0 && intrinsic_size.height > 0)
  {
    *border_x = (int)(fitting_size.width - intrinsic_size.width);
    *border_y = (int)(fitting_size.height - intrinsic_size.height);

    if (*border_x < 0) *border_x = 0;
    if (*border_y < 0) *border_y = 0;
  }
  else
  {
    *border_x = 8;
    *border_y = 8;
  }

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    [temp_image release];
  }
  [temp_button release];
}

void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
  int border_x = 0, border_y = 0;
  BOOL is_text = (ih->data->type & IUP_BUTTON_TEXT) && !(ih->data->type & IUP_BUTTON_IMAGE);

  cocoaButtonCalculateBorders(ih, &border_x, &border_y);

  *x += border_x;
  *y += border_y;

  /* Add user-defined padding */
  *x += 2 * ih->data->horiz_padding;
  *y += 2 * ih->data->vert_padding;

  if (ih->data->horiz_padding == 0)
  {
    *x += 10;
  }
  if (ih->data->vert_padding == 0)
  {
    *y += 5;
  }

  /* Ensure minimum button height */
  if (*y < (int)kIupCocoaDefaultHeightNSButton)
  {
    *y = (int)kIupCocoaDefaultHeightNSButton;
  }
}

static int cocoaButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  NSButton* the_button = ih->handle;

  if (ih->data->type & IUP_BUTTON_TEXT)
  {
    if(value && *value!=0)
    {
      char* stripped_str = iupStrProcessMnemonic(value, NULL, 0);
      NSString* ns_string = [NSString stringWithUTF8String:stripped_str];

      if(stripped_str && stripped_str != value)
      {
        free(stripped_str);
      }

      char* fgcolor = iupAttribGet(ih, "FGCOLOR");
      unsigned char r, g, b;
      if (fgcolor && iupStrToRGB(fgcolor, &r, &g, &b))
      {
        NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
        NSMutableAttributedString* attributed_title = [[NSMutableAttributedString alloc] initWithString:ns_string];
        NSRange range = NSMakeRange(0, [attributed_title length]);
        [attributed_title addAttribute:NSForegroundColorAttributeName value:color range:range];
        [the_button setAttributedTitle:attributed_title];
        [attributed_title release];
      }
      else
      {
        [the_button setTitle:ns_string];
      }

      /* Prevent text wrapping */
      [[the_button cell] setLineBreakMode:NSLineBreakByClipping];

      if(ih->data->type & IUP_BUTTON_IMAGE)
      {
        NSCellImagePosition position;
        switch(ih->data->img_position)
        {
          case IUP_IMGPOS_TOP:
            position = NSImageAbove;
            break;
          case IUP_IMGPOS_BOTTOM:
            position = NSImageBelow;
            break;
          case IUP_IMGPOS_RIGHT:
            position = NSImageRight;
            break;
          case IUP_IMGPOS_LEFT:
          default:
            position = NSImageLeft;
            break;
        }
        [the_button setImagePosition:position];
      }

      if(ih->handle)
      {
        IupRefresh(ih);
      }
      return 1;
    }
  }

  return 0;
}

static int cocoaButtonSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  NSButton* the_button = ih->handle;
  IupCocoaFont* font = iupCocoaFindFont(value);

  if (font && font.nativeFont)
  {
    [the_button setFont:font.nativeFont];

    if (ih->handle)
      IupRefresh(ih);

    return 1;
  }

  return 0;
}

static int cocoaButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  NSButton* the_button = ih->handle;
  char value1[30], value2[30];
  NSTextAlignment alignment = NSCenterTextAlignment;

  iupStrToStrStr(value, value1, value2, ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
  {
    alignment = NSRightTextAlignment;
    ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
  }
  else if (iupStrEqualNoCase(value1, "ALEFT"))
  {
    alignment = NSLeftTextAlignment;
    ih->data->horiz_alignment = IUP_ALIGN_ALEFT;
  }
  else /* ACENTER (default) */
  {
    alignment = NSCenterTextAlignment;
    ih->data->horiz_alignment = IUP_ALIGN_ACENTER;
  }

  [[the_button cell] setAlignment:alignment];

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    ih->data->vert_alignment = IUP_ALIGN_ATOP;
  else /* ACENTER (default) */
    ih->data->vert_alignment = IUP_ALIGN_ACENTER;

  return 1;
}

static char* cocoaButtonGetAlignmentAttrib(Ihandle* ih)
{
  char* horiz_align2str[3] = {"ALEFT", "ACENTER", "ARIGHT"};
  char* vert_align2str[3] = {"ATOP", "ACENTER", "ABOTTOM"};

  int horiz = ih->data->horiz_alignment;
  int vert = ih->data->vert_alignment;

  if (horiz < IUP_ALIGN_ALEFT || horiz > IUP_ALIGN_ARIGHT)
    horiz = IUP_ALIGN_ACENTER;
  if (vert < IUP_ALIGN_ATOP || vert > IUP_ALIGN_ABOTTOM)
    vert = IUP_ALIGN_ACENTER;

  return iupStrReturnStrf("%s:%s", horiz_align2str[horiz], vert_align2str[vert]);
}

static int cocoaButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  NSButton* the_button = ih->handle;

  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    /* Apply content insets if available (macOS 11.0+) */
    if ([the_button respondsToSelector:@selector(setContentInsets:)])
    {
      NSEdgeInsets insets = NSEdgeInsetsMake(
        ih->data->vert_padding,
        ih->data->horiz_padding,
        ih->data->vert_padding,
        ih->data->horiz_padding
      );

      NSMethodSignature *signature = [NSButton instanceMethodSignatureForSelector:@selector(setContentInsets:)];
      NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
      [invocation setSelector:@selector(setContentInsets:)];
      [invocation setTarget:the_button];
      [invocation setArgument:&insets atIndex:2];
      [invocation invoke];
    }

    IupRefresh(ih);
    return 0;
  }

  return 1;
}

static int cocoaButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  NSButton* the_button = ih->handle;
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  /* NSButton background color requires layer-backed view */
  if (@available(macOS 10.12, *))
  {
    [the_button setWantsLayer:YES];
    [[the_button layer] setBackgroundColor:[[NSColor colorWithCalibratedRed:r/255.0
                                     green:g/255.0
                                      blue:b/255.0
                                     alpha:1.0] CGColor]];
  }

  return 1;
}

static int cocoaButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  NSButton* the_button = ih->handle;
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
  NSString* title = [the_button title];

  if (title && [title length] > 0)
  {
    NSMutableAttributedString* attributed_title = [[NSMutableAttributedString alloc] initWithString:title];
    NSRange range = NSMakeRange(0, [attributed_title length]);
    [attributed_title addAttribute:NSForegroundColorAttributeName value:color range:range];
    [the_button setAttributedTitle:attributed_title];
    [attributed_title release];
  }

  return 1;
}

static int cocoaButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  NSButton* the_button = ih->handle;

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    NSImage* the_bitmap;
    int make_inactive = 0;

    if (!iupAttribGetBoolean(ih, "ACTIVE"))
      make_inactive = 1;

    the_bitmap = iupImageGetImage(value, ih, make_inactive, NULL);
    [the_button setImage:the_bitmap];

    if (ih->handle)
    {
      IupRefresh(ih);
    }
    return 1;
  }

  return 0;
}

static int cocoaButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupAttribGetBoolean(ih, "ACTIVE"))
    {
      NSButton* the_button = ih->handle;
      NSImage* the_bitmap = iupImageGetImage(value, ih, 0, NULL);
      [the_button setImage:the_bitmap];
    }
    return 1;
  }

  return 0;
}

static int cocoaButtonSetImpressAttrib(Ihandle* ih, const char* value)
{
  NSButton* the_button = ih->handle;

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (value && *value != 0)
    {
      int make_inactive = 0;
      if (!iupAttribGetBoolean(ih, "ACTIVE"))
        make_inactive = 1;

      NSImage* the_bitmap = iupImageGetImage(value, ih, make_inactive, NULL);
      [the_button setAlternateImage:the_bitmap];
    }
    else
    {
      [the_button setAlternateImage:nil];
    }

    if (ih->handle)
    {
      IupRefresh(ih);
    }
    return 1;
  }

  return 0;
}

static int cocoaButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  NSButton* the_button = ih->handle;
  int active = iupStrBoolean(value);

  [the_button setEnabled:active];

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* image_name = iupAttribGet(ih, active ? "IMAGE" : "IMINACTIVE");
    if (!image_name && !active)
      image_name = iupAttribGet(ih, "IMAGE");

    if (image_name)
    {
      int make_inactive = !active && !iupAttribGet(ih, "IMINACTIVE");
      NSImage* the_bitmap = iupImageGetImage(image_name, ih, make_inactive, NULL);
      [the_button setImage:the_bitmap];
    }

    char* impress_name = iupAttribGet(ih, "IMPRESS");
    if (impress_name)
    {
      int make_inactive = !active;
      NSImage* the_bitmap = iupImageGetImage(impress_name, ih, make_inactive, NULL);
      [the_button setAlternateImage:the_bitmap];
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

void cocoaButtonLayoutUpdateMethod(Ihandle *ih)
{
  NSView* parent_view = iupCocoaCommonBaseLayoutGetParentView(ih);
  NSView* child_view = iupCocoaCommonBaseLayoutGetChildView(ih);

  if (!parent_view || !child_view)
  {
    return;
  }

  NSRect parent_rect = [parent_view frame];

  NSRect child_rect = NSMakeRect(
      ih->x,
      parent_rect.size.height - ih->y - ih->currentheight,
      ih->currentwidth,
      ih->currentheight
      );

  [child_view setFrame:NSIntegralRect(child_rect)];
}

static int cocoaButtonMapMethod(Ihandle* ih)
{
  char* value;
  NSButton* the_button;
  int has_border = 1;
  BOOL is_flat = iupAttribGetBoolean(ih, "FLAT");

  the_button = [[IupCocoaFlatButton alloc] initWithFrame:NSZeroRect];
  [(IupCocoaFlatButton*)the_button setIsFlat:is_flat];
  [(IupCocoaFlatButton*)the_button setIsHovering:NO];

  [the_button setTitle:@""];
  [the_button setBezelStyle:NSRegularSquareBezelStyle];
  [the_button setButtonType:NSButtonTypeMomentaryPushIn];

  value = iupAttribGet(ih, "IMAGE");
  if(value)
  {
    ih->data->type = IUP_BUTTON_IMAGE;

    const char* title = iupAttribGet(ih, "TITLE");
    if (title && *title!=0)
    {
      ih->data->type |= IUP_BUTTON_TEXT;

      char* stripped_str = iupStrProcessMnemonic(title, NULL, 0);
      NSString* ns_string = [NSString stringWithUTF8String:stripped_str];

      if(stripped_str && stripped_str != title)
        free(stripped_str);

      [the_button setTitle:ns_string];

      NSCellImagePosition position;
      switch(ih->data->img_position)
      {
        case IUP_IMGPOS_TOP:
          position = NSImageAbove;
          break;
        case IUP_IMGPOS_BOTTOM:
          position = NSImageBelow;
          break;
        case IUP_IMGPOS_RIGHT:
          position = NSImageRight;
          break;
        case IUP_IMGPOS_LEFT:
        default:
          position = NSImageLeft;
          break;
      }
      [the_button setImagePosition:position];
    }
    else
    {
      [the_button setImagePosition:NSImageOnly];
    }

    if (iupAttribGet(ih, "IMPRESS") && !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
    {
      has_border = 0;
      [the_button setButtonType:NSButtonTypeMomentaryChange];
      [the_button setBordered:NO];
    }

    NSImage* the_bitmap;
    int make_inactive = 0;

    if(iupAttribGet(ih, "IMINACTIVE") && !iupAttribGetBoolean(ih, "ACTIVE"))
    {
      value = iupAttribGet(ih, "IMINACTIVE");
    }
    else if(!iupAttribGetBoolean(ih, "ACTIVE"))
    {
      make_inactive = 1;
    }

    the_bitmap = iupImageGetImage(value, ih, make_inactive, NULL);
    [the_button setImage:the_bitmap];

    value = iupAttribGet(ih, "IMPRESS");
    if(value && *value!=0)
    {
      the_bitmap = iupImageGetImage(value, ih, make_inactive, NULL);
      [the_button setAlternateImage:the_bitmap];
    }
  }
  else
  {
    ih->data->type = IUP_BUTTON_TEXT;
  }

  [the_button setFont:[NSFont systemFontOfSize:0]];
  [[the_button cell] setLineBreakMode:NSLineBreakByClipping];

  /* Apply initial padding if set and API is available */
  if ((ih->data->horiz_padding != 0 || ih->data->vert_padding != 0) &&
      [the_button respondsToSelector:@selector(setContentInsets:)])
  {
    NSEdgeInsets insets = NSEdgeInsetsMake(
      ih->data->vert_padding,
      ih->data->horiz_padding,
      ih->data->vert_padding,
      ih->data->horiz_padding
    );

    NSMethodSignature *signature = [NSButton instanceMethodSignatureForSelector:@selector(setContentInsets:)];
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
    [invocation setSelector:@selector(setContentInsets:)];
    [invocation setTarget:the_button];
    [invocation setArgument:&insets atIndex:2];
    [invocation invoke];
  }

  ih->handle = the_button;
  iupCocoaSetAssociatedViews(ih, the_button, the_button);

  objc_setAssociatedObject(the_button, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

  IupCocoaButtonReceiver* button_receiver = [[IupCocoaButtonReceiver alloc] init];
  [the_button setTarget:button_receiver];
  [the_button setAction:@selector(myButtonClickAction:)];
  objc_setAssociatedObject(the_button, IUP_COCOA_BUTTON_RECEIVER_OBJ_KEY, (id)button_receiver, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

  iupCocoaAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    [the_button setRefusesFirstResponder:YES];
    iupCocoaSetCanFocus(ih, 0);
  }
  else
  {
    iupCocoaSetCanFocus(ih, 1);
  }

  if (is_flat && has_border)
  {
    [the_button setBordered:NO];
    [the_button updateTrackingAreas];
  }

  if (!iupAttribGetBoolean(ih, "ACTIVE"))
    [the_button setEnabled:NO];

  return IUP_NOERROR;
}

static void cocoaButtonUnMapMethod(Ihandle* ih)
{
  id the_button = ih->handle;

  Ihandle* context_menu_ih = (Ihandle*)iupCocoaCommonBaseGetContextMenuAttrib(ih);
  if(context_menu_ih)
  {
    IupDestroy(context_menu_ih);
    iupCocoaCommonBaseSetContextMenuAttrib(ih, NULL);
  }

  id button_receiver = objc_getAssociatedObject(the_button, IUP_COCOA_BUTTON_RECEIVER_OBJ_KEY);
  objc_setAssociatedObject(the_button, IUP_COCOA_BUTTON_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
  [button_receiver release];

  iupCocoaRemoveFromParent(ih);
  iupCocoaSetAssociatedViews(ih, nil, nil);

  [the_button release];
  ih->handle = NULL;
}

void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = cocoaButtonMapMethod;
  ic->UnMap = cocoaButtonUnMapMethod;
  ic->LayoutUpdate = cocoaButtonLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaButtonSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, cocoaButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FONT", NULL, cocoaButtonSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", cocoaButtonGetAlignmentAttrib, cocoaButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, cocoaButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, cocoaButtonSetImpressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESSBORDER", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, cocoaButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);

  iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupCocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);
}
