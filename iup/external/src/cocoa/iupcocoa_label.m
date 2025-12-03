/** \file
 * \brief Label Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Cocoa/Cocoa.h>
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
#include "iup_label.h"
#include "iup_drv.h"
#include "iup_image.h"
#include "iup_focus.h"

#include "iup_childtree.h"

#include "iupcocoa_drv.h"

#import "IupCocoaVerticalAlignmentTextFieldCell.h"


@interface IUPCocoaLabelEventView : NSView
{
	NSTrackingArea* _mouseTrackingArea;
}
@end

@implementation IUPCocoaLabelEventView

- (void)dealloc
{
	if (_mouseTrackingArea)
	{
		[self removeTrackingArea:_mouseTrackingArea];
		[_mouseTrackingArea release];
	}
	[super dealloc];
}

- (void)mouseDown:(NSEvent *)theEvent
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupcocoaCommonBaseHandleMouseButtonCallback(ih, theEvent, self, true))
    [super mouseDown:theEvent];
}

- (void)mouseUp:(NSEvent *)theEvent
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupcocoaCommonBaseHandleMouseButtonCallback(ih, theEvent, self, false))
    [super mouseUp:theEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupcocoaCommonBaseHandleMouseButtonCallback(ih, theEvent, self, true))
    [super rightMouseDown:theEvent];
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupcocoaCommonBaseHandleMouseButtonCallback(ih, theEvent, self, false))
    [super rightMouseUp:theEvent];
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupcocoaCommonBaseHandleMouseButtonCallback(ih, theEvent, self, true))
    [super otherMouseDown:theEvent];
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupcocoaCommonBaseHandleMouseButtonCallback(ih, theEvent, self, false))
    [super otherMouseUp:theEvent];
}

- (void)mouseMoved:(NSEvent *)theEvent
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupcocoaCommonBaseHandleMouseMotionCallback(ih, theEvent, self))
    [super mouseMoved:theEvent];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
  [self mouseMoved:theEvent];
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
  [self mouseMoved:theEvent];
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
  [self mouseMoved:theEvent];
}

- (void)updateTrackingAreas
{
	[super updateTrackingAreas];

	if (_mouseTrackingArea)
	{
		[self removeTrackingArea:_mouseTrackingArea];
		[_mouseTrackingArea release];
		_mouseTrackingArea = nil;
	}

	NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
                                   NSTrackingMouseMoved |
                                   NSTrackingActiveInKeyWindow |
                                   NSTrackingInVisibleRect;
	_mouseTrackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
	                                                  options:options
	                                                    owner:self
	                                                 userInfo:nil];
	[self addTrackingArea:_mouseTrackingArea];
}

- (void)mouseEntered:(NSEvent *)theEvent
{
  [super mouseEntered:theEvent];

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  IFn cb = (IFn)IupGetCallback(ih, "ENTERWINDOW_CB");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

- (void)mouseExited:(NSEvent *)theEvent
{
  [super mouseExited:theEvent];

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  IFn cb = (IFn)IupGetCallback(ih, "LEAVEWINDOW_CB");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

@end


static NSTextField* cocoaLabelGetTextField(Ihandle* ih)
{
  return (NSTextField*)iupcocoaGetMainView(ih);
}

static NSImageView* cocoaLabelGetImageView(Ihandle* ih)
{
  return (NSImageView*)iupcocoaGetMainView(ih);
}

void iupdrvLabelAddExtraPadding(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  (void)x;
  (void)y;
}

static int cocoaLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    IupRefresh(ih);
    return 0;
  }

  return 1;
}

static int cocoaLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  NSTextField* the_label = cocoaLabelGetTextField(ih);
  if (!the_label)
    return 0;

  if ([the_label respondsToSelector:@selector(cell)])
  {
    id cell = [the_label cell];
    if ((nil != cell) && [cell isKindOfClass:[NSImageCell class]])
      return 0;
  }

  NSString* ns_string = nil;
  BOOL has_newlines = NO;
  if (value)
  {
    char* stripped_str = iupStrProcessMnemonic(value, NULL, 0);
    ns_string = [NSString stringWithUTF8String:stripped_str];

    // Check if value contains newlines for multi-line support
    has_newlines = (strchr(value, '\n') != NULL);

    if (stripped_str && stripped_str != value)
      free(stripped_str);
  }
  else
  {
    ns_string = @"";
  }

  // Configure multi-line mode if needed
  if (has_newlines)
  {
    [the_label setUsesSingleLineMode:NO];
    [[the_label cell] setScrollable:NO];
    [[the_label cell] setWraps:YES];
    [[the_label cell] setLineBreakMode:NSLineBreakByWordWrapping];
  }
  else
  {
    [the_label setUsesSingleLineMode:YES];
    [[the_label cell] setScrollable:YES];
    [[the_label cell] setWraps:NO];
    [[the_label cell] setLineBreakMode:NSLineBreakByClipping];
  }

  IupCocoaFont* iup_font = iupcocoaGetFont(ih);
  char* fgcolor = iupAttribGet(ih, "FGCOLOR");
  unsigned char r, g, b;
  BOOL need_attributed = [iup_font usesAttributes] || (fgcolor && iupStrToRGB(fgcolor, &r, &g, &b));

  // Get alignment setting to apply to paragraph style
  NSTextAlignment text_alignment = [the_label alignment];

  if (need_attributed)
  {
    NSMutableAttributedString* attr_str;

    if ([iup_font usesAttributes])
    {
      attr_str = [[NSMutableAttributedString alloc] initWithString:ns_string
                                                        attributes:[iup_font attributeDictionary]];
    }
    else
    {
      NSMutableDictionary* attrs = [NSMutableDictionary dictionary];
      NSFont* native_font = [iup_font nativeFont];
      if (native_font)
        [attrs setObject:native_font forKey:NSFontAttributeName];

      attr_str = [[NSMutableAttributedString alloc] initWithString:ns_string attributes:attrs];
    }

    if (fgcolor && iupStrToRGB(fgcolor, &r, &g, &b))
    {
      NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
      NSRange range = NSMakeRange(0, [attr_str length]);
      [attr_str addAttribute:NSForegroundColorAttributeName value:color range:range];
    }

    // Apply paragraph style for text alignment (required for attributed strings)
    NSMutableParagraphStyle* paragraph_style = [[NSMutableParagraphStyle alloc] init];
    [paragraph_style setAlignment:text_alignment];
    [attr_str addAttribute:NSParagraphStyleAttributeName value:paragraph_style range:NSMakeRange(0, [ns_string length])];
    [paragraph_style release];

    [the_label setAttributedStringValue:attr_str];
    [attr_str release];
  }
  else
  {
    [the_label setStringValue:ns_string];
  }

  if (ih->handle)
    IupRefresh(ih);

  return 1;
}

static int cocoaLabelSetActiveAttrib(Ihandle* ih, const char* value)
{
  NSView* the_view = iupcocoaGetMainView(ih);
  if (!the_view)
    return iupBaseSetActiveAttrib(ih, value);

  BOOL is_active = (BOOL)iupStrBoolean(value);

  if ([the_view isKindOfClass:[NSTextField class]])
  {
    NSTextField* the_label = (NSTextField*)the_view;
    [the_label setEnabled:is_active];

    IupCocoaFont* iup_font = iupcocoaGetFont(ih);
    BOOL uses_attributed_string = ([iup_font usesAttributes] || [[the_label attributedStringValue] length] > 0);

    NSColor* color;
    if (is_active)
    {
      char* user_color = iupAttribGet(ih, "_IUPCOCOA_USER_FGCOLOR");
      if (user_color)
      {
        unsigned char r, g, b;
        if (iupStrToRGB(user_color, &r, &g, &b))
          color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
        else
          color = [NSColor controlTextColor];
      }
      else
      {
        color = [NSColor controlTextColor];
      }
    }
    else
    {
      color = [NSColor disabledControlTextColor];
    }

    if (uses_attributed_string)
    {
      NSMutableAttributedString* attr_str = [[the_label attributedStringValue] mutableCopy];
      if (attr_str && [attr_str length] > 0)
      {
        NSRange range = NSMakeRange(0, [attr_str length]);
        [attr_str addAttribute:NSForegroundColorAttributeName value:color range:range];
        [the_label setAttributedStringValue:attr_str];
        [attr_str release];
      }
    }
    else
    {
      [the_label setTextColor:color];
    }
  }
  else if ([the_view isKindOfClass:[NSImageView class]])
  {
    NSImageView* image_view = (NSImageView*)the_view;

    char* image_name;
    int make_inactive = 0;

    if (is_active)
    {
      image_name = iupAttribGet(ih, "IMAGE");
    }
    else
    {
      image_name = iupAttribGet(ih, "IMINACTIVE");
      if (!image_name)
      {
        image_name = iupAttribGet(ih, "IMAGE");
        make_inactive = 1;
      }
    }

    if (image_name)
    {
      id the_bitmap = iupImageGetImage(image_name, ih, make_inactive, NULL);
      [image_view setImage:the_bitmap];
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static char* cocoaLabelGetTitleAttrib(Ihandle* ih)
{
  NSTextField* the_label = cocoaLabelGetTextField(ih);
  if (the_label)
  {
    if ([the_label respondsToSelector:@selector(stringValue)])
    {
      NSString* ns_string = [the_label stringValue];
      if (ns_string)
      {
        return iupStrReturnStr([ns_string UTF8String]);
      }
    }
  }
  return NULL;
}

static char* cocoaLabelGetAlignmentAttrib(Ihandle* ih)
{
  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
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

  return NULL;
}

static int cocoaLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    char value1[30], value2[30];
    iupStrToStrStr(value, value1, value2, ':');

    if (ih->data->type == IUP_LABEL_TEXT)
    {
      NSTextField* the_label = cocoaLabelGetTextField(ih);
      NSCAssert([the_label isKindOfClass:[NSTextField class]], @"Expected NSTextField");

      if (iupStrEqualNoCase(value1, "ARIGHT"))
      {
        [the_label setAlignment:NSTextAlignmentRight];
        ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
      }
      else if (iupStrEqualNoCase(value1, "ACENTER"))
      {
        [the_label setAlignment:NSTextAlignmentCenter];
        ih->data->horiz_alignment = IUP_ALIGN_ACENTER;
      }
      else
      {
        [the_label setAlignment:NSTextAlignmentLeft];
        ih->data->horiz_alignment = IUP_ALIGN_ALEFT;
      }

      if (iupStrEqualNoCase(value2, "ABOTTOM"))
      {
        NSCAssert([[the_label cell] isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
        IUPCocoaVerticalAlignmentTextFieldCell* vertical_alignment_cell = (IUPCocoaVerticalAlignmentTextFieldCell*)[the_label cell];
        [vertical_alignment_cell setAlignmentMode:IUPTextVerticalAlignmentBottom];
        ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
      }
      else if (iupStrEqualNoCase(value2, "ATOP"))
      {
        NSCAssert([[the_label cell] isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
        IUPCocoaVerticalAlignmentTextFieldCell* vertical_alignment_cell = (IUPCocoaVerticalAlignmentTextFieldCell*)[the_label cell];
        [vertical_alignment_cell setAlignmentMode:IUPTextVerticalAlignmentTop];
        ih->data->vert_alignment = IUP_ALIGN_ATOP;
      }
      else
      {
        NSCAssert([[the_label cell] isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
        IUPCocoaVerticalAlignmentTextFieldCell* vertical_alignment_cell = (IUPCocoaVerticalAlignmentTextFieldCell*)[the_label cell];
        [vertical_alignment_cell setAlignmentMode:IUPTextVerticalAlignmentCenter];
        ih->data->vert_alignment = IUP_ALIGN_ACENTER;
      }

      return 1;
    }
    else if (ih->data->type == IUP_LABEL_IMAGE)
    {
      NSImageView* the_label = cocoaLabelGetImageView(ih);
      NSCAssert([the_label isKindOfClass:[NSImageView class]], @"Expected NSImageView");

      if (iupStrEqualNoCase(value1, "ARIGHT"))
      {
        ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;

        if (iupStrEqualNoCase(value2, "ABOTTOM"))
        {
          [the_label setImageAlignment:NSImageAlignBottomRight];
          ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
        }
        else if (iupStrEqualNoCase(value2, "ATOP"))
        {
          [the_label setImageAlignment:NSImageAlignTopRight];
          ih->data->vert_alignment = IUP_ALIGN_ATOP;
        }
        else
        {
          [the_label setImageAlignment:NSImageAlignRight];
          ih->data->vert_alignment = IUP_ALIGN_ACENTER;
        }
      }
      else if (iupStrEqualNoCase(value1, "ACENTER"))
      {
        ih->data->horiz_alignment = IUP_ALIGN_ACENTER;

        if (iupStrEqualNoCase(value2, "ABOTTOM"))
        {
          [the_label setImageAlignment:NSImageAlignBottom];
          ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
        }
        else if (iupStrEqualNoCase(value2, "ATOP"))
        {
          [the_label setImageAlignment:NSImageAlignTop];
          ih->data->vert_alignment = IUP_ALIGN_ATOP;
        }
        else
        {
          [the_label setImageAlignment:NSImageAlignCenter];
          ih->data->vert_alignment = IUP_ALIGN_ACENTER;
        }
      }
      else
      {
        ih->data->horiz_alignment = IUP_ALIGN_ALEFT;

        if (iupStrEqualNoCase(value2, "ABOTTOM"))
        {
          [the_label setImageAlignment:NSImageAlignBottomLeft];
          ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
        }
        else if (iupStrEqualNoCase(value2, "ATOP"))
        {
          [the_label setImageAlignment:NSImageAlignTopLeft];
          ih->data->vert_alignment = IUP_ALIGN_ATOP;
        }
        else
        {
          [the_label setImageAlignment:NSImageAlignLeft];
          ih->data->vert_alignment = IUP_ALIGN_ACENTER;
        }
      }

      return 1;
    }
  }

  return 0;
}

static int cocoaLabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    NSTextField* the_label = cocoaLabelGetTextField(ih);
    NSCAssert([the_label isKindOfClass:[NSTextField class]], @"Expected NSTextField");

    if (iupStrBoolean(value))
    {
      if ([the_label respondsToSelector:@selector(setLineBreakMode:)])
      {
        [the_label setLineBreakMode:NSLineBreakByWordWrapping];
        IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
        NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
        [vertical_cell setUseWordWrap:YES];
        [vertical_cell setUseEllipsis:NO];
      }
      else
      {
        char* ellipsis_state = iupAttribGet(ih, "ELLIPSIS");
        if (iupStrBoolean(ellipsis_state))
        {
          [the_label setUsesSingleLineMode:NO];
          [[the_label cell] setScrollable:NO];
          [[the_label cell] setWraps:YES];
          [[the_label cell] setLineBreakMode:NSLineBreakByTruncatingTail];
          [[the_label cell] setTruncatesLastVisibleLine:YES];

          IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
          NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
          [vertical_cell setUseWordWrap:YES];
          [vertical_cell setUseEllipsis:YES];
        }
        else
        {
          [the_label setUsesSingleLineMode:NO];
          [[the_label cell] setScrollable:NO];
          [[the_label cell] setWraps:YES];
          [[the_label cell] setLineBreakMode:NSLineBreakByWordWrapping];
          [[the_label cell] setTruncatesLastVisibleLine:NO];

          IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
          NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
          [vertical_cell setUseWordWrap:YES];
          [vertical_cell setUseEllipsis:NO];
        }
      }
    }
    else
    {
      if ([the_label respondsToSelector:@selector(setLineBreakMode:)])
      {
        char* ellipsis_state = iupAttribGet(ih, "ELLIPSIS");
        if (iupStrBoolean(ellipsis_state))
        {
          [the_label setLineBreakMode:NSLineBreakByTruncatingTail];

          IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
          NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
          [vertical_cell setUseWordWrap:NO];
          [vertical_cell setUseEllipsis:YES];
        }
        else
        {
          [the_label setLineBreakMode:NSLineBreakByClipping];

          IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
          NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
          [vertical_cell setUseWordWrap:NO];
          [vertical_cell setUseEllipsis:NO];
        }
      }
      else
      {
        char* ellipsis_state = iupAttribGet(ih, "ELLIPSIS");
        if (iupStrBoolean(ellipsis_state))
        {
          [the_label setUsesSingleLineMode:NO];
          [[the_label cell] setScrollable:NO];
          [[the_label cell] setWraps:YES];
          [[the_label cell] setLineBreakMode:NSLineBreakByWordWrapping];
          [[the_label cell] setTruncatesLastVisibleLine:YES];

          IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
          NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
          [vertical_cell setUseWordWrap:NO];
          [vertical_cell setUseEllipsis:YES];
        }
        else
        {
          [the_label setUsesSingleLineMode:YES];
          [[the_label cell] setScrollable:YES];
          [[the_label cell] setWraps:NO];
          [[the_label cell] setLineBreakMode:NSLineBreakByClipping];
          [[the_label cell] setTruncatesLastVisibleLine:NO];

          IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
          NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
          [vertical_cell setUseWordWrap:NO];
          [vertical_cell setUseEllipsis:NO];
        }
      }
    }

    if (ih->handle)
      IupRefresh(ih);

    return 1;
  }
  return 0;
}

static int cocoaLabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    NSTextField* the_label = cocoaLabelGetTextField(ih);
    NSCAssert([the_label isKindOfClass:[NSTextField class]], @"Expected NSTextField");

    if (iupStrBoolean(value))
    {
      if ([the_label respondsToSelector:@selector(setLineBreakMode:)])
      {
        [the_label setUsesSingleLineMode:YES];
        [the_label setLineBreakMode:NSLineBreakByTruncatingTail];

        IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
        NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
        [vertical_cell setUseWordWrap:NO];
        [vertical_cell setUseEllipsis:YES];
      }
      else
      {
        [[the_label cell] setScrollable:NO];
        [[the_label cell] setWraps:YES];
        [[the_label cell] setLineBreakMode:NSLineBreakByWordWrapping];
        [[the_label cell] setTruncatesLastVisibleLine:YES];

        IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
        NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
        [vertical_cell setUseWordWrap:YES];
        [vertical_cell setUseEllipsis:YES];
      }
    }
    else
    {
      if ([the_label respondsToSelector:@selector(setLineBreakMode:)])
      {
        char* wordwrap_state = iupAttribGet(ih, "WORDWRAP");
        if (iupStrBoolean(wordwrap_state))
        {
          [the_label setUsesSingleLineMode:NO];
          [the_label setLineBreakMode:NSLineBreakByWordWrapping];

          IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
          NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
          [vertical_cell setUseWordWrap:YES];
          [vertical_cell setUseEllipsis:NO];
        }
        else
        {
          [the_label setLineBreakMode:NSLineBreakByClipping];

          IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
          NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
          [vertical_cell setUseWordWrap:NO];
          [vertical_cell setUseEllipsis:NO];
        }
      }
      else
      {
        char* wordwrap_state = iupAttribGet(ih, "WORDWRAP");
        if (iupStrBoolean(wordwrap_state))
        {
          [[the_label cell] setScrollable:NO];
          [[the_label cell] setWraps:YES];
          [[the_label cell] setLineBreakMode:NSLineBreakByWordWrapping];
          [[the_label cell] setTruncatesLastVisibleLine:YES];

          IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
          NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
          [vertical_cell setUseWordWrap:YES];
          [vertical_cell setUseEllipsis:NO];
        }
        else
        {
          [[the_label cell] setScrollable:YES];
          [[the_label cell] setWraps:NO];
          [[the_label cell] setLineBreakMode:NSLineBreakByClipping];
          [[the_label cell] setTruncatesLastVisibleLine:NO];

          IUPCocoaVerticalAlignmentTextFieldCell* vertical_cell = [the_label cell];
          NSCAssert([vertical_cell isKindOfClass:[IUPCocoaVerticalAlignmentTextFieldCell class]], @"Expected IUPCocoaVerticalAlignmentTextFieldCell");
          [vertical_cell setUseWordWrap:NO];
          [vertical_cell setUseEllipsis:NO];
        }
      }
    }

    if (ih->handle)
      IupRefresh(ih);

    return 1;
  }
  return 0;
}

static int cocoaLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_IMAGE)
    return 0;

  NSImageView* image_view = cocoaLabelGetImageView(ih);
  if (!image_view)
    return 0;

  char* name;
  int make_inactive = 0;

  if (iupdrvIsActive(ih))
  {
    name = (char*)value;
  }
  else
  {
    name = iupAttribGet(ih, "IMINACTIVE");
    if (!name)
    {
      name = (char*)value;
      make_inactive = 1;
    }
  }

  if (name)
  {
    id the_bitmap = iupImageGetImage(name, ih, make_inactive, NULL);
    [image_view setImage:the_bitmap];

    if (ih->handle)
      IupRefresh(ih);
  }

  return 1;
}

static int cocoaLabelSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_IMAGE)
    return 0;

  if (iupdrvIsActive(ih))
    return 1;

  NSImageView* image_view = cocoaLabelGetImageView(ih);
  if (!image_view)
    return 0;

  if (value)
  {
    id the_bitmap = iupImageGetImage(value, ih, 0, NULL);
    [image_view setImage:the_bitmap];
  }
  else
  {
    char* name = iupAttribGet(ih, "IMAGE");
    if (name)
    {
      id the_bitmap = iupImageGetImage(name, ih, 1, NULL);
      [image_view setImage:the_bitmap];
    }
  }

  return 1;
}

static int cocoaLabelSetBgColorAttrib(Ihandle* ih, const char* value)
{
  NSView* the_view = iupcocoaGetMainView(ih);
  NSView* root_view = iupcocoaGetRootView(ih);
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];

  // Set background on the inner widget (text field or image view)
  if ([the_view isKindOfClass:[NSTextField class]])
  {
    NSTextField* text_field = (NSTextField*)the_view;
    [text_field setBackgroundColor:color];
    [text_field setDrawsBackground:YES];
  }
  else if ([the_view isKindOfClass:[NSImageView class]])
  {
    // For image labels, set background on the wrapper
    [root_view setWantsLayer:YES];
    [[root_view layer] setBackgroundColor:[color CGColor]];
  }

  // Set background on the wrapper view (event view)
  [root_view setWantsLayer:YES];
  [[root_view layer] setBackgroundColor:[color CGColor]];

  return 1;
}

static int cocoaLabelSetFgColorAttrib(Ihandle* ih, const char* value)
{
  int type = iupLabelGetTypeBeforeMap(ih);
  if (type != IUP_LABEL_SEP_HORIZ && type != IUP_LABEL_SEP_VERT)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(value, &r, &g, &b))
    {
      iupAttribSetStr(ih, "_IUPCOCOA_USER_FGCOLOR", value);

      if (ih->handle)
      {
        NSView* the_view = iupcocoaGetMainView(ih);
        if ([the_view isKindOfClass:[NSTextField class]])
        {
          NSTextField* text_field = (NSTextField*)the_view;
          if (iupdrvIsActive(ih))
          {
            NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];

            NSAttributedString* current_attr_string = [text_field attributedStringValue];
            if (current_attr_string && [current_attr_string length] > 0)
            {
              NSMutableAttributedString* attr_str = [current_attr_string mutableCopy];
              NSRange range = NSMakeRange(0, [attr_str length]);
              [attr_str addAttribute:NSForegroundColorAttributeName value:color range:range];
              [text_field setAttributedStringValue:attr_str];
              [attr_str release];
            }
            else
            {
              [text_field setTextColor:color];
            }
          }
        }
      }
    }
  }
  return 1;
}

static int cocoaLabelSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  if (!ih->handle || ih->data->type != IUP_LABEL_TEXT)
    return 1;

  IupCocoaFont* font = iupcocoaFindFont(value);
  if (!font || !font.nativeFont)
    return 0;

  NSTextField* the_label = cocoaLabelGetTextField(ih);
  if (the_label)
  {
    NSAttributedString* current_attr_string = [the_label attributedStringValue];

    if (current_attr_string && [current_attr_string length] > 0)
    {
      NSMutableAttributedString* new_attr_string = [current_attr_string mutableCopy];
      NSRange full_range = NSMakeRange(0, [new_attr_string length]);

      [new_attr_string addAttribute:NSFontAttributeName value:font.nativeFont range:full_range];

      if ([font usesAttributes])
      {
        NSDictionary* font_attrs = [font attributeDictionary];
        for (NSString* attr_key in font_attrs)
        {
          if (![attr_key isEqualToString:NSForegroundColorAttributeName])
          {
            [new_attr_string addAttribute:attr_key value:[font_attrs objectForKey:attr_key] range:full_range];
          }
        }
      }

      [the_label setAttributedStringValue:new_attr_string];
      [new_attr_string release];
    }
    else
    {
      [the_label setFont:font.nativeFont];
    }

    IupRefresh(ih);
  }

  return 1;
}

static int cocoaLabelSetSelectable(Ihandle* ih, const char* value)
{
  NSView* the_view = iupcocoaGetMainView(ih);
  BOOL is_active = (BOOL)iupStrBoolean(value);

  if ([the_view isKindOfClass:[NSTextField class]])
  {
    NSTextField* the_label = (NSTextField*)the_view;

    /* Note: In older macOS versions, setSelectable:YES may cause visual glitches with vertical alignment. */
    [the_label setSelectable:is_active];
  }
  else if ([the_view isKindOfClass:[NSImageView class]])
  {
    /* Not supported for image views */
  }

  return 1;
}

static char* cocoaLabelGetSelectable(Ihandle* ih)
{
  NSView* the_view = iupcocoaGetMainView(ih);
  BOOL is_active = NO;

  if ([the_view isKindOfClass:[NSTextField class]])
  {
    NSTextField* the_label = (NSTextField*)the_view;
    is_active = [the_label isSelectable];
  }
  else if ([the_view isKindOfClass:[NSImageView class]])
  {
    /* Not supported for image views */
  }

  return iupStrReturnBoolean(is_active);
}

static int cocoaLabelMapMethod(Ihandle* ih)
{
  char* value;
  id the_actual_label = nil;

  value = iupAttribGet(ih, "SEPARATOR");
  if (value)
  {
    if (iupStrEqualNoCase(value, "HORIZONTAL"))
    {
      ih->data->type = IUP_LABEL_SEP_HORIZ;
      NSBox* horizontal_separator = [[NSBox alloc] initWithFrame:NSMakeRect(0.0, 0.0, 250.0, 1.0)];
      [horizontal_separator setBoxType:NSBoxSeparator];
      the_actual_label = horizontal_separator;
    }
    else
    {
      ih->data->type = IUP_LABEL_SEP_VERT;
      NSBox* vertical_separator = [[NSBox alloc] initWithFrame:NSMakeRect(0.0, 0.0, 1.0, 250.0)];
      [vertical_separator setBoxType:NSBoxSeparator];
      the_actual_label = vertical_separator;
    }
  }
  else
  {
    value = iupAttribGet(ih, "IMAGE");
    if (value)
    {
      ih->data->type = IUP_LABEL_IMAGE;

      iupAttribSet(ih, "_IUPCOCOA_ACTIVE", "YES");

      char *name;
      int make_inactive = 0;

      if (iupdrvIsActive(ih))
      {
        name = iupAttribGet(ih, "IMAGE");
      }
      else
      {
        name = iupAttribGet(ih, "IMINACTIVE");
        if (!name)
        {
          name = iupAttribGet(ih, "IMAGE");
          make_inactive = 1;
        }
      }

      id the_bitmap = iupImageGetImage(name, ih, make_inactive, NULL);
      int width;
      int height;
      int bpp;

      iupdrvImageGetInfo(the_bitmap, &width, &height, &bpp);

      NSImageView* image_view = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
      [image_view setImage:the_bitmap];

      the_actual_label = image_view;
    }
    else
    {
      ih->data->type = IUP_LABEL_TEXT;

      iupAttribSet(ih, "_IUPCOCOA_ACTIVE", "YES");

      the_actual_label = [[NSTextField alloc] initWithFrame:NSZeroRect];

      IUPCocoaVerticalAlignmentTextFieldCell* textfield_cell = [[IUPCocoaVerticalAlignmentTextFieldCell alloc] initTextCell:@""];
      [the_actual_label setCell:textfield_cell];
      [textfield_cell release];

      [the_actual_label setBezeled:NO];
      [the_actual_label setDrawsBackground:NO];
      [the_actual_label setEditable:NO];
      [the_actual_label setSelectable:NO];
      [the_actual_label setFont:[NSFont systemFontOfSize:[NSFont systemFontSize]]];

      // Set initial alignment based on ALIGNMENT attribute if already set
      char* alignment = iupAttribGet(ih, "ALIGNMENT");
      if (alignment)
      {
        char value1[30], value2[30];
        iupStrToStrStr(alignment, value1, value2, ':');

        if (iupStrEqualNoCase(value1, "ARIGHT"))
          [the_actual_label setAlignment:NSTextAlignmentRight];
        else if (iupStrEqualNoCase(value1, "ACENTER"))
          [the_actual_label setAlignment:NSTextAlignmentCenter];
        else
          [the_actual_label setAlignment:NSTextAlignmentLeft];
      }

      if ([the_actual_label respondsToSelector:@selector(setLineBreakMode:)])
      {
        [the_actual_label setLineBreakMode:NSLineBreakByClipping];
      }
      else
      {
        [[the_actual_label cell] setTruncatesLastVisibleLine:NO];
        [the_actual_label setUsesSingleLineMode:YES];
        [[the_actual_label cell] setScrollable:YES];
        [[the_actual_label cell] setWraps:NO];
        [[the_actual_label cell] setLineBreakMode:NSLineBreakByClipping];
        [[the_actual_label cell] setTruncatesLastVisibleLine:NO];
      }

      char* title = iupAttribGet(ih, "TITLE");
      if (title)
      {
        NSString* ns_string = [NSString stringWithUTF8String:title];

        // Check if title contains newlines for multi-line support
        BOOL has_newlines = (strchr(title, '\n') != NULL);

        if (has_newlines)
        {
          // Configure for multi-line display
          [(NSTextField*)the_actual_label setUsesSingleLineMode:NO];
          [[(NSTextField*)the_actual_label cell] setScrollable:NO];
          [[(NSTextField*)the_actual_label cell] setWraps:YES];
          [[(NSTextField*)the_actual_label cell] setLineBreakMode:NSLineBreakByWordWrapping];
        }

        [(NSTextField*)the_actual_label setStringValue:ns_string];
      }
    }
  }

  if (!the_actual_label)
  {
    return IUP_ERROR;
  }

  IUPCocoaLabelEventView* event_view_wrapper = [[IUPCocoaLabelEventView alloc] initWithFrame:NSZeroRect];
  [event_view_wrapper addSubview:the_actual_label];

  [the_actual_label setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
  [the_actual_label setFrame:[event_view_wrapper bounds]];
  [the_actual_label release];

  ih->handle = event_view_wrapper;
  objc_setAssociatedObject(event_view_wrapper, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

  iupcocoaSetAssociatedViews(ih, the_actual_label, event_view_wrapper);

  iupcocoaAddToParent(ih);

  if (IupGetCallback(ih, "DROPFILES_CB"))
  {
    iupAttribSet(ih, "DROPFILESTARGET", "YES");
  }

  return IUP_NOERROR;
}

static void cocoaLabelUnMapMethod(Ihandle* ih)
{
  iupdrvBaseUnMapMethod(ih);
}

void iupdrvLabelInitClass(Iclass* ic)
{
  ic->Map = cocoaLabelMapMethod;
  ic->UnMap = cocoaLabelUnMapMethod;

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, cocoaLabelSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", iupBaseNativeParentGetBgColorAttrib, cocoaLabelSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaLabelSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FONT", NULL, cocoaLabelSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "TITLE", cocoaLabelGetTitleAttrib, cocoaLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupLabel only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", cocoaLabelGetAlignmentAttrib, cocoaLabelSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT:ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaLabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, cocoaLabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  /* IupLabel GTK and Motif only */
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, cocoaLabelSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupLabel Windows and GTK only */
  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, cocoaLabelSetWordWrapAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, cocoaLabelSetEllipsisAttrib, NULL, NULL, IUPAF_DEFAULT);

  /* Mac only */
  iupClassRegisterAttribute(ic, "SELECTABLE", cocoaLabelGetSelectable, cocoaLabelSetSelectable, IUPAF_SAMEASSYSTEM, "NO", IUPAF_DEFAULT|IUPAF_NO_INHERIT);

  /* Not supported */
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
