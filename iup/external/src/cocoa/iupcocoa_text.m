/** \file
 * \brief Text Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_array.h"
#include "iup_text.h"
#include "iup_globalattrib.h"

#include "iupcocoa_drv.h"


static const void* IUP_COCOA_TEXT_STEPPER_OBJ_KEY = "IUP_COCOA_TEXT_STEPPER_OBJ_KEY";
static const void* IUP_COCOA_TEXT_DELEGATE_OBJ_KEY = "IUP_COCOA_TEXT_DELEGATE_OBJ_KEY";
static const void* IUP_COCOA_TEXT_FORMATTER_KEY = "IUP_COCOA_TEXT_FORMATTER_KEY";
static const void* IUP_COCOA_TEXT_OVERWRITE_KEY = "IUP_COCOA_TEXT_OVERWRITE_KEY";

typedef enum
{
  IUPCOCOATEXTSUBTYPE_FIELD,
  IUPCOCOATEXTSUBTYPE_VIEW,
  IUPCOCOATEXTSUBTYPE_STEPPER,
} IupCocoaTextSubType;

static bool cocoaTextComputeLineColumnFromRangeForTextView(NSTextView* text_view, NSRange native_selection_range,
              NSUInteger* out_start_line, NSUInteger* out_start_column, NSUInteger* out_end_line, NSUInteger* out_end_column);

static IupCocoaTextSubType cocoaTextGetSubType(Ihandle* ih)
{
  if(ih->data->is_multiline)
  {
    return IUPCOCOATEXTSUBTYPE_VIEW;
  }
  else if(iupAttribGetBoolean(ih, "SPIN"))
  {
    return IUPCOCOATEXTSUBTYPE_STEPPER;
  }
  else
  {
    return IUPCOCOATEXTSUBTYPE_FIELD;
  }
}

static NSView* cocoaTextGetRootView(Ihandle* ih)
{
  NSView* root_container_view = (NSView*)ih->handle;
  NSCAssert([root_container_view isKindOfClass:[NSView class]], @"Expected NSView");
  return root_container_view;
}

static NSTextField* cocoaTextGetTextField(Ihandle* ih)
{
  NSTextField* root_container_view = (NSTextField*)ih->handle;
  NSCAssert([root_container_view isKindOfClass:[NSTextField class]], @"Expected NSTextField");
  return root_container_view;
}

static NSTextView* cocoaTextGetTextView(Ihandle* ih)
{
  NSScrollView* root_container_view = (NSScrollView*)cocoaTextGetRootView(ih);
  NSCAssert([root_container_view isKindOfClass:[NSScrollView class]], @"Expected NSScrollView");
  NSTextView* text_view = [root_container_view documentView];
  NSCAssert([text_view isKindOfClass:[NSTextView class]], @"Expected NSTextView");
  return text_view;
}

static NSStepper* cocoaTextGetStepperView(Ihandle* ih)
{
  NSStepper* stepper = (NSStepper*)objc_getAssociatedObject((id)ih->handle, IUP_COCOA_TEXT_STEPPER_OBJ_KEY);
  NSCAssert([stepper isKindOfClass:[NSStepper class]], @"Expected NSStepper");
  return stepper;
}

static NSTextField* cocoaTextGetStepperTextField(Ihandle* ih)
{
  return (NSTextField*)iupcocoaGetMainView(ih);
}

static void cocoaTextSetCorrections(NSTextView* text_view, BOOL enable)
{
#ifndef GNUSTEP
  [text_view setAutomaticSpellingCorrectionEnabled:enable];
  [text_view setContinuousSpellCheckingEnabled:enable];
  [text_view setAutomaticQuoteSubstitutionEnabled:enable];
  [text_view setAutomaticDashSubstitutionEnabled:enable];
  [text_view setAutomaticTextReplacementEnabled:enable];
#else
  (void)text_view;
  (void)enable;
#endif
}

static NSRect cocoaTextCellCenterVertically(NSCell* cell, NSRect bounds, NSRect drawing_rect, BOOL* measuring)
{
  CGFloat natural, extra;

  if (*measuring)
    return drawing_rect;

  *measuring = YES;
  natural = [cell cellSizeForBounds:NSMakeRect(0, 0, bounds.size.width, 10000.0)].height;
  *measuring = NO;

  extra = floor((bounds.size.height - natural) / 2.0);
  if (extra > 0)
  {
    drawing_rect.origin.y += extra;
    drawing_rect.size.height -= extra;
  }
  return drawing_rect;
}

@interface IupCocoaTextFieldCell : NSTextFieldCell
{
  CGFloat _horizPadding;
  CGFloat _vertPadding;
  BOOL _measuring;
  BOOL _autoCorrect;
}
- (void)setIupPaddingHoriz:(CGFloat)horiz vert:(CGFloat)vert;
- (void)setIupAutoCorrect:(BOOL)enable;
@end

@implementation IupCocoaTextFieldCell

- (instancetype)initTextCell:(NSString*)string
{
  self = [super initTextCell:string];
  if (self)
  {
    _horizPadding = 2.0;
    _vertPadding = 2.0;
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder*)coder
{
  self = [super initWithCoder:coder];
  if (self)
  {
    _horizPadding = 2.0;
    _vertPadding = 2.0;
  }
  return self;
}

- (void)setIupPaddingHoriz:(CGFloat)horiz vert:(CGFloat)vert
{
  _horizPadding = horiz;
  _vertPadding = vert;
}

- (void)setIupAutoCorrect:(BOOL)enable
{
  _autoCorrect = enable;
}

/* the window's field editor is shared by every text field, so each field applies its own setting */
- (NSText*)setUpFieldEditorAttributes:(NSText*)text_object
{
  NSText* editor = [super setUpFieldEditorAttributes:text_object];
  if ([editor isKindOfClass:[NSTextView class]])
    cocoaTextSetCorrections((NSTextView*)editor, _autoCorrect);
  return editor;
}

- (NSRect)drawingRectForBounds:(NSRect)rect
{
  NSRect drawingRect = [super drawingRectForBounds:rect];

  CGFloat currentInsetX = drawingRect.origin.x - rect.origin.x;
  CGFloat adjustmentX = _horizPadding - currentInsetX;
  drawingRect.origin.x += adjustmentX;
  drawingRect.size.width -= (2.0 * adjustmentX);

  CGFloat currentInsetY = drawingRect.origin.y - rect.origin.y;
  CGFloat adjustmentY = _vertPadding - currentInsetY;
  drawingRect.origin.y += adjustmentY;
  drawingRect.size.height -= (2.0 * adjustmentY);

  return cocoaTextCellCenterVertically(self, rect, drawingRect, &_measuring);
}

@end

static BOOL cocoaTextHandleShouldChangeText(NSTextField* text_field, NSTextView* text_view, NSRange change_range, NSString* replacement_string)
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);

  if(ih->data->disable_callbacks || iupAttribGet(ih, "_IUPCOCOA_DISABLE_TEXT_CB"))
  {
    return YES;
  }

  IFnis action_cb = (IFnis)IupGetCallback(ih, "ACTION");

  int start_pos = iupcocoaStrUnitToChar([text_view string], change_range.location);
  int end_pos = iupcocoaStrUnitToChar([text_view string], change_range.location + change_range.length);

  if(end_pos < start_pos)
  {
    end_pos = start_pos;
  }

  const char* c_str;
  int remove_dir;

  if([replacement_string length] == 0)
  {
    c_str = NULL;
    remove_dir = -1;
  }
  else
  {
    c_str = [replacement_string UTF8String];
    remove_dir = 0;
  }

  int ret_val = iupEditCallActionCb(ih, action_cb, c_str, start_pos, end_pos, ih->data->mask, ih->data->nc, remove_dir, YES);

  if(ret_val == 0)
  {
    return NO;
  }
  else if(ret_val != -1 && c_str != NULL)
  {
    char replace_char[2];
    replace_char[0] = (char)ret_val;
    replace_char[1] = 0;

    NSString* replacement = iupcocoaStrToNSString(replace_char);

    ih->data->disable_callbacks = 1;
    [text_view replaceCharactersInRange:change_range withString:replacement];
    ih->data->disable_callbacks = 0;

    [text_view didChangeText];
    return NO;
  }

  return YES;
}

static BOOL cocoaTextWantsMouse(Ihandle* ih)
{
  return ih && (IupGetCallback(ih, "BUTTON_CB") || IupGetCallback(ih, "MOTION_CB"));
}

static void cocoaTextSendRelease(Ihandle* ih, NSView* view, NSEventType up_type)
{
  NSEvent* up_event = [NSApp currentEvent];
  if (ih && up_event && [up_event type] == up_type)
    iupcocoaCommonBaseHandleControlMouseButton(ih, up_event, view, false);
}

/* while editing, clicks go to the field editor; the field takes them so BUTTON_CB sees them */
static NSView* cocoaTextFieldHitTest(NSTextField* field, NSView* hit)
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(field, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (hit && hit != field && cocoaTextWantsMouse(ih) && [hit isDescendantOf:field])
    return field;
  return hit;
}

#define IUPCOCOA_TEXTFIELD_MOUSE_METHODS \
- (NSView*)hitTest:(NSPoint)point \
{ \
  return cocoaTextFieldHitTest(self, [super hitTest:point]); \
} \
- (void)mouseDown:(NSEvent*)event \
{ \
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY); \
  if (ih && !iupcocoaCommonBaseHandleControlMouseButton(ih, event, self, true)) \
    return; \
  NSText* editor = [self currentEditor]; \
  if (editor && cocoaTextWantsMouse(ih)) \
    [editor mouseDown:event]; \
  else \
    [super mouseDown:event]; \
  cocoaTextSendRelease(ih, self, NSEventTypeLeftMouseUp); \
} \
- (void)mouseUp:(NSEvent*)event \
{ \
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY); \
  if (!ih || iupcocoaCommonBaseHandleControlMouseButton(ih, event, self, false)) \
    [super mouseUp:event]; \
} \
- (void)rightMouseDown:(NSEvent*)event \
{ \
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY); \
  if (ih && !iupcocoaCommonBaseHandleControlMouseButton(ih, event, self, true)) \
    return; \
  NSText* editor = [self currentEditor]; \
  if (editor && cocoaTextWantsMouse(ih)) \
    [editor rightMouseDown:event]; \
  else \
    [super rightMouseDown:event]; \
  cocoaTextSendRelease(ih, self, NSEventTypeRightMouseUp); \
} \
- (void)rightMouseUp:(NSEvent*)event \
{ \
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY); \
  if (!ih || iupcocoaCommonBaseHandleControlMouseButton(ih, event, self, false)) \
    [super rightMouseUp:event]; \
} \
- (void)updateTrackingAreas \
{ \
  [super updateTrackingAreas]; \
  iupcocoaCommonBaseUpdateMotionArea(self); \
} \
- (void)mouseMoved:(NSEvent*)event \
{ \
  [super mouseMoved:event]; \
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY); \
  if (ih) \
    iupcocoaCommonBaseHandleMouseMotionCallback(ih, event, self); \
}

static void cocoaTextFieldSelectionChanged(NSTextField* field)
{
  id the_delegate = [field delegate];
  if ([the_delegate respondsToSelector:@selector(checkFieldCaret:)])
    [the_delegate performSelector:@selector(checkFieldCaret:) withObject:field afterDelay:0];
}

@interface NSTextField (IupCocoaTextFieldEditorDelegate) <NSTextViewDelegate>
@end

@interface IupCocoaTextField : NSTextField
@end

@implementation IupCocoaTextField

IUPCOCOA_TEXTFIELD_MOUSE_METHODS

- (void)scrollWheel:(NSEvent*)the_event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && IupGetCallback(ih, "WHEEL_CB"))
  {
    if ([the_event deltaY] != 0)
      iupcocoaCommonBaseScrollWheelCallback(ih, the_event, self);
    return;
  }
  [super scrollWheel:the_event];
}

+ (Class)cellClass
{
  return [IupCocoaTextFieldCell class];
}

- (BOOL) textView:(NSTextView*)text_view shouldChangeTextInRange:(NSRange)change_range replacementString:(NSString*)replacement_string
{
  id the_delegate = [self delegate];
  if([the_delegate respondsToSelector:@selector(textView:shouldChangeTextInRange:replacementString:)])
  {
    if(![the_delegate textView:text_view shouldChangeTextInRange:change_range replacementString:replacement_string])
      return NO;
  }
  return cocoaTextHandleShouldChangeText(self, text_view, change_range, replacement_string);
}

- (void) textViewDidChangeSelection:(NSNotification*)notification
{
  if ([NSTextField instancesRespondToSelector:_cmd])
    [super textViewDidChangeSelection:notification];
  cocoaTextFieldSelectionChanged(self);
}

- (NSMenu*)textView:(NSTextView*)text_view menu:(NSMenu*)the_menu forEvent:(NSEvent*)the_event atIndex:(NSUInteger)char_index
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (iupAttribGet(ih, "_IUPCOCOA_CONTEXTMENU_SET"))
  {
    Ihandle* menu_ih = (Ihandle*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");
    if (menu_ih && menu_ih->handle)
      return (NSMenu*)menu_ih->handle;
    else
      return nil;
  }
  return the_menu;
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

/* the field editor takes the responder next, so the focus out comes from end editing */
- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if (result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
    if (ih && iupObjectCheck(ih))
      iupcocoaFocusIn(ih);
  }
  return result;
}

@end

@interface IupCocoaSecureTextFieldCell : NSSecureTextFieldCell
{
  CGFloat _horizPadding;
  CGFloat _vertPadding;
  BOOL _measuring;
}
- (void)setIupPaddingHoriz:(CGFloat)horiz vert:(CGFloat)vert;
@end

@implementation IupCocoaSecureTextFieldCell

- (instancetype)initTextCell:(NSString*)string
{
  self = [super initTextCell:string];
  if (self)
  {
    _horizPadding = 2.0;
    _vertPadding = 2.0;
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder*)coder
{
  self = [super initWithCoder:coder];
  if (self)
  {
    _horizPadding = 2.0;
    _vertPadding = 2.0;
  }
  return self;
}

- (void)setIupPaddingHoriz:(CGFloat)horiz vert:(CGFloat)vert
{
  _horizPadding = horiz;
  _vertPadding = vert;
}

- (NSRect)drawingRectForBounds:(NSRect)rect
{
  NSRect drawingRect = [super drawingRectForBounds:rect];

  CGFloat currentInsetX = drawingRect.origin.x - rect.origin.x;
  CGFloat adjustmentX = _horizPadding - currentInsetX;
  drawingRect.origin.x += adjustmentX;
  drawingRect.size.width -= (2.0 * adjustmentX);

  CGFloat currentInsetY = drawingRect.origin.y - rect.origin.y;
  CGFloat adjustmentY = _vertPadding - currentInsetY;
  drawingRect.origin.y += adjustmentY;
  drawingRect.size.height -= (2.0 * adjustmentY);

  return cocoaTextCellCenterVertically(self, rect, drawingRect, &_measuring);
}

@end

@interface IupCocoaSecureTextField : NSSecureTextField
@end

@implementation IupCocoaSecureTextField

IUPCOCOA_TEXTFIELD_MOUSE_METHODS

- (void)scrollWheel:(NSEvent*)the_event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && IupGetCallback(ih, "WHEEL_CB"))
  {
    if ([the_event deltaY] != 0)
      iupcocoaCommonBaseScrollWheelCallback(ih, the_event, self);
    return;
  }
  [super scrollWheel:the_event];
}

+ (Class)cellClass
{
  return [IupCocoaSecureTextFieldCell class];
}

- (BOOL) textView:(NSTextView*)text_view shouldChangeTextInRange:(NSRange)change_range replacementString:(NSString*)replacement_string
{
  id the_delegate = [self delegate];
  if([the_delegate respondsToSelector:@selector(textView:shouldChangeTextInRange:replacementString:)])
  {
    if(![the_delegate textView:text_view shouldChangeTextInRange:change_range replacementString:replacement_string])
      return NO;
  }
  return cocoaTextHandleShouldChangeText(self, text_view, change_range, replacement_string);
}

- (void) textViewDidChangeSelection:(NSNotification*)notification
{
  if ([NSSecureTextField instancesRespondToSelector:_cmd])
    [super textViewDidChangeSelection:notification];
  cocoaTextFieldSelectionChanged(self);
}

- (NSMenu*)textView:(NSTextView*)text_view menu:(NSMenu*)the_menu forEvent:(NSEvent*)the_event atIndex:(NSUInteger)char_index
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (iupAttribGet(ih, "_IUPCOCOA_CONTEXTMENU_SET"))
  {
    Ihandle* menu_ih = (Ihandle*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");
    if (menu_ih && menu_ih->handle)
      return (NSMenu*)menu_ih->handle;
    else
      return nil;
  }
  return the_menu;
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

/* the field editor takes the responder next, so the focus out comes from end editing */
- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if (result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
    if (ih && iupObjectCheck(ih))
      iupcocoaFocusIn(ih);
  }
  return result;
}

@end


@interface IupCocoaTextView : NSTextView
@property(nonatomic, retain) NSColor* defaultColor;
@end

@implementation IupCocoaTextView

- (void)scrollToEnd
{
  [self scrollRangeToVisible:NSMakeRange([[self string] length], 0)];
}

- (void)mouseDown:(NSEvent*)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && !iupcocoaCommonBaseHandleControlMouseButton(ih, event, self, true))
    return;
  [super mouseDown:event];
  cocoaTextSendRelease(ih, self, NSEventTypeLeftMouseUp);
}

- (void)mouseUp:(NSEvent*)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih || iupcocoaCommonBaseHandleControlMouseButton(ih, event, self, false))
    [super mouseUp:event];
}

- (void)rightMouseDown:(NSEvent*)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && !iupcocoaCommonBaseHandleControlMouseButton(ih, event, self, true))
    return;
  [super rightMouseDown:event];
  cocoaTextSendRelease(ih, self, NSEventTypeRightMouseUp);
}

- (void)rightMouseUp:(NSEvent*)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih || iupcocoaCommonBaseHandleControlMouseButton(ih, event, self, false))
    [super rightMouseUp:event];
}

- (void)updateTrackingAreas
{
  [super updateTrackingAreas];
  iupcocoaCommonBaseUpdateMotionArea(self);
}

- (void)mouseMoved:(NSEvent*)event
{
  [super mouseMoved:event];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
    iupcocoaCommonBaseHandleMouseMotionCallback(ih, event, self);
}

- (void)scrollWheel:(NSEvent*)the_event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && IupGetCallback(ih, "WHEEL_CB"))
  {
    if ([the_event deltaY] != 0)
      iupcocoaCommonBaseScrollWheelCallback(ih, the_event, self);
    return;
  }
  [super scrollWheel:the_event];
}

- (void)dealloc
{
  [_defaultColor release];
  [super dealloc];
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

- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if(result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
    if(ih && iupObjectCheck(ih))
      iupcocoaFocusIn(ih);
  }
  return result;
}

- (BOOL)resignFirstResponder
{
  BOOL result = [super resignFirstResponder];
  if(result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
    if(ih && iupObjectCheck(ih))
      iupcocoaFocusOut(ih);
  }
  return result;
}

@end

static void cocoaTextCallCaretCb(Ihandle* ih)
{
  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb)
    return;

  int col, lin, pos;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
    {
      NSTextView* text_view = cocoaTextGetTextView(ih);
      NSRange cursor_range = [[[text_view selectedRanges] lastObject] rangeValue];
      if(NSNotFound == cursor_range.location)
      {
        NSTextStorage* text_storage = [text_view textStorage];
        cursor_range.location = [text_storage length];
        cursor_range.length = 0;
      }

      NSUInteger lin_start=1;
      NSUInteger col_start=1;
      NSUInteger lin_end=1;
      NSUInteger col_end=1;
      bool did_find_range = cocoaTextComputeLineColumnFromRangeForTextView(text_view, cursor_range, &lin_start, &col_start, &lin_end, &col_end);
      if(!did_find_range)
        return;

      pos = iupcocoaStrUnitToChar([[text_view textStorage] string], cursor_range.location);
      lin = (int)lin_start;
      col = (int)col_start;
      break;
    }
    case IUPCOCOATEXTSUBTYPE_FIELD:
    {
      NSTextField* text_field = cocoaTextGetTextField(ih);
      NSText* field_editor = [text_field currentEditor];
      if (!field_editor)
        return;

      NSRange selected_range = [field_editor selectedRange];
      pos = iupcocoaStrUnitToChar([field_editor string], selected_range.location);
      col = pos + 1;
      lin = 1;
      break;
    }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
    {
      NSTextField* text_field = cocoaTextGetStepperTextField(ih);
      NSText* field_editor = [text_field currentEditor];
      if (!field_editor)
        return;

      NSRange selected_range = [field_editor selectedRange];
      pos = iupcocoaStrUnitToChar([field_editor string], selected_range.location);
      col = pos + 1;
      lin = 1;
      break;
    }
    default:
      return;
  }

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;
    cb(ih, lin, col, pos);
  }
}

static BOOL cocoaTextTabCommand(Ihandle* ih, SEL command)
{
  NSEvent* current_event;

  if (!iupObjectCheck(ih) ||
      (command != @selector(insertTab:) && command != @selector(insertBacktab:) && command != @selector(insertTabIgnoringFieldEditor:)))
    return NO;

  current_event = [NSApp currentEvent];
  if (!current_event || [current_event type] != NSEventTypeKeyDown)
    return NO;

  if (iupcocoaKeyEvent(ih, current_event, [current_event keyCode], true))
    return YES;
  return (command == @selector(insertBacktab:) && ih->data->is_multiline) ? YES : NO;
}

@interface IupCocoaTextFieldDelegate : NSObject <NSControlTextEditingDelegate, NSTextFieldDelegate>
@end

@implementation IupCocoaTextFieldDelegate

- (BOOL) control:(NSControl*)control textView:(NSTextView*)text_view doCommandBySelector:(SEL)command
{
  (void)text_view;
  return cocoaTextTabCommand((Ihandle*)objc_getAssociatedObject(control, IHANDLE_ASSOCIATED_OBJ_KEY), command);
}

- (void) controlTextDidChange:(NSNotification*)the_notification
{
  NSTextField* text_field = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);

  if(ih->data->disable_callbacks || iupAttribGet(ih, "_IUPCOCOA_DISABLE_TEXT_CB"))
  {
    return;
  }

  IFn value_changed_callback = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if(NULL != value_changed_callback)
  {
    int ret_val = value_changed_callback(ih);
    (void)ret_val;
  }

  [self performSelector:@selector(checkCaretPosition:) withObject:the_notification afterDelay:0];
}

- (void) checkCaretPosition:(NSNotification*)the_notification
{
  NSTextField* text_field = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (ih && iupObjectCheck(ih) && !ih->data->disable_callbacks && !iupAttribGet(ih, "_IUPCOCOA_DISABLE_TEXT_CB"))
    cocoaTextCallCaretCb(ih);
}

- (void) checkFieldCaret:(NSTextField*)text_field
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (ih && iupObjectCheck(ih) && !ih->data->disable_callbacks && !iupAttribGet(ih, "_IUPCOCOA_DISABLE_TEXT_CB"))
    cocoaTextCallCaretCb(ih);
}

- (void) controlTextDidBeginEditing:(NSNotification*)the_notification
{
  NSTextField* text_field = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (ih && iupObjectCheck(ih))
    iupcocoaFocusIn(ih);
}

- (void) controlTextDidEndEditing:(NSNotification*)the_notification
{
  iupcocoaTextFieldSaveRange(the_notification);
  /* the window parks the responder while it swaps the field editor, so check on the next pass */
  [self performSelector:@selector(checkFocusOut:) withObject:[the_notification object] afterDelay:0];
}

- (void) checkFocusOut:(NSTextField*)text_field
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);
  id first_responder = [[text_field window] firstResponder];

  if (first_responder == text_field || [text_field currentEditor])
    return;

  if ([first_responder isKindOfClass:[NSView class]] && [(NSView*)first_responder isDescendantOf:text_field])
    return;

  if (ih && iupObjectCheck(ih))
    iupcocoaFocusOut(ih);
}

@end

/* Inherit from IupCocoaTextFieldDelegate to get VALUECHANGED_CB */
@interface IupCocoaTextSpinnerDelegate : IupCocoaTextFieldDelegate
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTextSpinnerDelegate

- (IBAction) mySpinnerClickAction:(id)the_sender
{
  Ihandle* ih = [self ihandle];
  NSStepper* stepper_view = (NSStepper*)the_sender;
  int current_value = [stepper_view intValue];

  if(iupAttribGetBoolean(ih, "SPINAUTO"))
  {
    NSTextField* text_field = cocoaTextGetStepperTextField(ih);
    [text_field setIntValue:current_value];
  }

  IFni callback_function = (IFni)IupGetCallback(ih, "SPIN_CB");
  if(callback_function)
  {
    callback_function(ih, current_value);
  }
}

@end

/* Inherit from IupCocoaTextFieldDelegate to get VALUECHANGED_CB */
@interface IupCocoaTextViewDelegate : IupCocoaTextFieldDelegate <NSTextViewDelegate>
{
  NSUndoManager* undoManager;
}
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTextViewDelegate

- (BOOL) textView:(NSTextView*)text_view doCommandBySelector:(SEL)command
{
  return cocoaTextTabCommand((Ihandle*)objc_getAssociatedObject(text_view, IHANDLE_ASSOCIATED_OBJ_KEY), command);
}

- (instancetype) init
{
  self = [super init];
  if(nil != self)
  {
    undoManager = [[NSUndoManager alloc] init];
  }
  return self;
}

- (void) dealloc
{
  [undoManager release];
  [super dealloc];
}

- (NSUndoManager*) undoManagerForTextView:(NSTextView*)text_view
{
  return undoManager;
}

- (NSMenu*)textView:(NSTextView*)textView menu:(NSMenu*)menu forEvent:(NSEvent*)event atIndex:(NSUInteger)charIndex
{
  Ihandle* ih = [self ihandle];
  if (iupAttribGet(ih, "_IUPCOCOA_CONTEXTMENU_SET"))
  {
    Ihandle* menu_ih = (Ihandle*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");
    if (menu_ih && menu_ih->handle)
      return (NSMenu*)menu_ih->handle;
    else
      return nil;
  }
  return menu;
}

- (BOOL) textView:(NSTextView*)text_view shouldChangeTextInRange:(NSRange)change_range replacementString:(NSString*)replacement_string
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_view, IHANDLE_ASSOCIATED_OBJ_KEY);

  if(ih->data->disable_callbacks)
  {
    return YES;
  }

  BOOL is_overwrite = NO;
  NSRange effective_range = change_range;
  NSNumber* overwrite_mode = objc_getAssociatedObject(text_view, IUP_COCOA_TEXT_OVERWRITE_KEY);
  if([overwrite_mode boolValue] && change_range.length == 0 && [replacement_string length] > 0 &&
     [replacement_string rangeOfComposedCharacterSequenceAtIndex:0].length == [replacement_string length])
  {
    NSString* text = [[text_view textStorage] string];

    if(change_range.location < [text length] &&
       ![[NSCharacterSet newlineCharacterSet] characterIsMember:[text characterAtIndex:change_range.location]])
    {
      effective_range = [text rangeOfComposedCharacterSequenceAtIndex:change_range.location];
      is_overwrite = YES;
    }
  }

  char* filter_case = iupAttribGet(ih, "_IUPCOCOA_FILTER_CASE");
  char* filter_number = iupAttribGet(ih, "_IUPCOCOA_FILTER_NUMBER");

  if(filter_case || filter_number)
  {
    if([replacement_string length] > 0)
    {
      NSString* filtered_string = replacement_string;

      if(filter_number)
      {
        NSCharacterSet* non_digit_set = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
        NSString* digits_only = [[replacement_string componentsSeparatedByCharactersInSet:non_digit_set] componentsJoinedByString:@""];

        if([digits_only length] == 0 && [replacement_string length] > 0)
        {
          return NO;
        }
        filtered_string = digits_only;
      }

      if(filter_case)
      {
        if(iupStrEqualNoCase(filter_case, "UPPER"))
        {
          filtered_string = [filtered_string uppercaseString];
        }
        else if(iupStrEqualNoCase(filter_case, "LOWER"))
        {
          filtered_string = [filtered_string lowercaseString];
        }
      }

      if(![filtered_string isEqualToString:replacement_string])
      {
        ih->data->disable_callbacks = 1;
        [text_view replaceCharactersInRange:effective_range withString:filtered_string];
        ih->data->disable_callbacks = 0;
        iupAttribSet(ih, "_IUPCOCOA_USER_EDIT", "1");
        [text_view didChangeText];
        return NO;
      }
    }
  }

  IFnis action_cb = (IFnis)IupGetCallback(ih, "ACTION");
  int ret_val;

  int start_pos = iupcocoaStrUnitToChar([text_view string], effective_range.location);
  int end_pos = iupcocoaStrUnitToChar([text_view string], effective_range.location + effective_range.length);

  if(end_pos < start_pos)
  {
    end_pos = start_pos;
  }

  const char* c_str;
  int remove_dir;

  if([replacement_string length] == 0)
  {
    c_str = NULL;
    remove_dir = -1;
  }
  else
  {
    c_str = [replacement_string UTF8String];
    remove_dir = 0;
  }

  ret_val = iupEditCallActionCb(ih, action_cb, c_str, start_pos, end_pos, ih->data->mask, ih->data->nc, remove_dir, YES);

  if(ret_val == 0)
  {
    return NO;
  }
  else if(ret_val != -1 && c_str != NULL)
  {
    char replace_char[2];
    replace_char[0] = (char)ret_val;
    replace_char[1] = 0;

    NSString* replacement = iupcocoaStrToNSString(replace_char);

    ih->data->disable_callbacks = 1;
    [text_view replaceCharactersInRange:effective_range withString:replacement];
    ih->data->disable_callbacks = 0;

    iupAttribSet(ih, "_IUPCOCOA_USER_EDIT", "1");
    [text_view didChangeText];
    return NO;
  }

  if(is_overwrite)
  {
    ih->data->disable_callbacks = 1;
    [text_view replaceCharactersInRange:effective_range withString:replacement_string];
    ih->data->disable_callbacks = 0;
    iupAttribSet(ih, "_IUPCOCOA_USER_EDIT", "1");
    [text_view didChangeText];
    return NO;
  }

  iupAttribSet(ih, "_IUPCOCOA_USER_EDIT", "1");
  return YES;
}

- (BOOL) textView:(NSTextView*)textView clickedOnLink:(id)link atIndex:(NSUInteger)charIndex
{
  Ihandle* ih = [self ihandle];
  const char* url = NULL;

  if([link isKindOfClass:[NSURL class]])
    url = [[link absoluteString] UTF8String];
  else if([link isKindOfClass:[NSString class]])
    url = [link UTF8String];

  if(url)
  {
    IFns cb = (IFns)IupGetCallback(ih, "LINK_CB");
    if(cb)
    {
      int ret = cb(ih, (char*)url);
      if(ret == IUP_CLOSE)
        IupExitLoop();
      else if(ret == IUP_DEFAULT)
        IupHelp(url);
    }
    else
      IupHelp(url);
  }

  (void)textView;
  (void)charIndex;
  return YES;
}

- (void) textDidChange:(NSNotification*)the_notification
{
  Ihandle* ih = [self ihandle];
  (void)the_notification;

  if (!ih || !iupAttribGet(ih, "_IUPCOCOA_USER_EDIT"))
    return;
  iupAttribSet(ih, "_IUPCOCOA_USER_EDIT", NULL);
  if (ih->data->disable_callbacks || iupAttribGet(ih, "_IUPCOCOA_DISABLE_TEXT_CB"))
    return;

  iupBaseCallValueChangedCb(ih);
}

- (void) checkViewCaret
{
  Ihandle* ih = [self ihandle];
  if (ih && iupObjectCheck(ih))
    cocoaTextCallCaretCb(ih);
}

- (void) textViewDidChangeSelection:(NSNotification*)the_notification
{
  Ihandle* ih = [self ihandle];
  (void)the_notification;

  if (!ih)
    return;

  if (ih->data->disable_callbacks || iupAttribGet(ih, "_IUPCOCOA_DISABLE_TEXT_CB"))
    return;

  [self performSelector:@selector(checkViewCaret) withObject:nil afterDelay:0];
}

@end

IUP_SDK_API void iupdrvTextAddSpin(Ihandle* ih, int* w, int h)
{
  static int spin_arrow_widths[4] = {-1, -1, -1, -1};
  NSControlSize size = ih ? iupcocoaGetControlSize(ih) : NSControlSizeRegular;
  int spin_arrow_width = spin_arrow_widths[size];

  (void)h;

  if (spin_arrow_width < 0)
  {
    NSStepper* temp_stepper = [[NSStepper alloc] initWithFrame:NSZeroRect];
    iupcocoaSetViewControlSize(temp_stepper, size);
    [temp_stepper setMinValue:0];
    [temp_stepper setMaxValue:100];
    [temp_stepper setIncrement:1];

    NSSize fittingSize = [temp_stepper fittingSize];
    int stepper_width = (int)ceilf(fittingSize.width);
    int spacing = 4;
    spin_arrow_width = stepper_width + spacing;
    spin_arrow_widths[size] = spin_arrow_width;

    [temp_stepper release];
  }

  *w += spin_arrow_width;
}

#define IUPCOCOA_TEXTVIEW_INSET_W 4
#define IUPCOCOA_TEXTVIEW_INSET_H 2

IUP_SDK_API void iupdrvTextAddBorders(Ihandle* ih, int* x, int* y)
{
  static int cocoa_textfield_border_ws[4] = {-1, -1, -1, -1};
  static int cocoa_textfield_border_hs[4];
  static int cocoa_scrollview_border_w = -1;
  static int cocoa_scrollview_border_h = -1;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  NSControlSize size = iupcocoaGetControlSize(ih);

  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        if (cocoa_scrollview_border_w < 0)
        {
          NSScrollView* temp_scroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
          NSTextView* temp_text = [[NSTextView alloc] initWithFrame:NSZeroRect];
          [temp_scroll setDocumentView:temp_text];
          [temp_scroll setBorderType:NSBezelBorder];
          [temp_scroll setHasVerticalScroller:YES];

          NSSize content_size = [temp_scroll contentSize];
#ifdef GNUSTEP
          NSSize frame_size = [NSScrollView frameSizeForContentSize:content_size
                                              hasHorizontalScroller:NO
                                                hasVerticalScroller:YES
                                                         borderType:NSBezelBorder];
#else
          NSSize frame_size = [NSScrollView frameSizeForContentSize:content_size
                                            horizontalScrollerClass:nil
                                              verticalScrollerClass:[NSScroller class]
                                                         borderType:NSBezelBorder
                                                        controlSize:NSControlSizeRegular
                                                      scrollerStyle:NSScrollerStyleOverlay];
#endif

          cocoa_scrollview_border_w = (int)lroundf(frame_size.width - content_size.width);
          cocoa_scrollview_border_h = (int)lroundf(frame_size.height - content_size.height);

          if (cocoa_scrollview_border_w < 0) cocoa_scrollview_border_w = 2;
          if (cocoa_scrollview_border_h < 0) cocoa_scrollview_border_h = 2;

          [temp_text release];
          [temp_scroll release];
        }

        *x += cocoa_scrollview_border_w;

        int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
        if (visiblelines > 0)
        {
          CGFloat cocoa_line_height;
          {
            IupCocoaFont* iup_font = iupcocoaGetFont(ih);
            NSFont* font = [iup_font nativeFont];
            if (!font)
            {
              const char* default_font = IupGetGlobal("DEFAULTFONT");
              iup_font = iupcocoaFindFont(default_font);
              font = iup_font ? [iup_font nativeFont] : [NSFont systemFontOfSize:13];
            }

            NSLayoutManager* layoutManager = [[NSLayoutManager alloc] init];
            cocoa_line_height = [layoutManager defaultLineHeightForFont:font];
            [layoutManager release];
          }

          int char_height;
          iupdrvFontGetCharSize(ih, NULL, &char_height);

          int line_spacing = (int)lroundf(cocoa_line_height) - char_height;
          if (line_spacing < 0)
            line_spacing = 0;

          int total_spacing = line_spacing * visiblelines;
          *y += total_spacing;
        }

        if (!ih->data->horiz_padding && !ih->data->vert_padding)
        {
          *x += 2 * IUPCOCOA_TEXTVIEW_INSET_W;
          *y += 2 * IUPCOCOA_TEXTVIEW_INSET_H;
        }

        *y += cocoa_scrollview_border_h;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        if (cocoa_textfield_border_ws[size] < 0)
        {
          int cocoa_textfield_border_w, cocoa_textfield_border_h;
          NSTextField* temp_field = [[NSTextField alloc] initWithFrame:NSZeroRect];
          [temp_field setBordered:YES];
          [temp_field setBezeled:YES];
          if (size != NSControlSizeRegular)
          {
            iupcocoaSetViewControlSize(temp_field, size);
            [temp_field setFont:iupcocoaGetControlSizeFont(size)];
          }
          [temp_field setStringValue:@"W"];

          NSFont* font = [temp_field font];
          if (!font)
            font = [NSFont systemFontOfSize:13];

          NSDictionary* attrs = @{NSFontAttributeName: font};
          NSSize text_size = [@"W" sizeWithAttributes:attrs];

          NSSize intrinsic_size = [temp_field intrinsicContentSize];

          cocoa_textfield_border_w = (int)lroundf(intrinsic_size.width - text_size.width);
          cocoa_textfield_border_h = (int)lroundf(intrinsic_size.height - text_size.height);

          if (cocoa_textfield_border_w < 0) cocoa_textfield_border_w = 6;
          if (cocoa_textfield_border_h < 0) cocoa_textfield_border_h = 6;

          cocoa_textfield_border_ws[size] = cocoa_textfield_border_w;
          cocoa_textfield_border_hs[size] = cocoa_textfield_border_h;

          [temp_field release];
        }

        *x += cocoa_textfield_border_ws[size];
        *y += cocoa_textfield_border_hs[size];
        break;
      }
    default:
      {
        break;
      }
  }
}

IUP_SDK_API void iupdrvTextAddExtraPadding(Ihandle* ih, int* w, int* h)
{
  static int cocoa_textfield_extra_ws[4] = {-1, -1, -1, -1};
  static int cocoa_textfield_extra_hs[4];
  NSControlSize size = ih ? iupcocoaGetControlSize(ih) : NSControlSizeRegular;
  int cocoa_textfield_extra_w = cocoa_textfield_extra_ws[size];
  int cocoa_textfield_extra_h = cocoa_textfield_extra_hs[size];

  if (cocoa_textfield_extra_w < 0)
  {
    NSTextField* temp_bordered = [[NSTextField alloc] initWithFrame:NSZeroRect];
    NSTextField* temp_noframe = [[NSTextField alloc] initWithFrame:NSZeroRect];

    if (size != NSControlSizeRegular)
    {
      iupcocoaSetViewControlSize(temp_bordered, size);
      iupcocoaSetViewControlSize(temp_noframe, size);
      [temp_bordered setFont:iupcocoaGetControlSizeFont(size)];
      [temp_noframe setFont:iupcocoaGetControlSizeFont(size)];
    }

    [temp_bordered setBordered:YES];
    [temp_bordered setBezeled:YES];
    [temp_bordered setStringValue:@"W"];

    [temp_noframe setBordered:NO];
    [temp_noframe setBezeled:NO];
    [temp_noframe setStringValue:@"W"];

    NSSize bordered_size = [temp_bordered intrinsicContentSize];
    NSSize noframe_size = [temp_noframe intrinsicContentSize];

    NSFont* font = [temp_noframe font];
    if (!font)
      font = [NSFont systemFontOfSize:13];

    NSDictionary* attrs = @{NSFontAttributeName: font};
    NSSize text_size = [@"W" sizeWithAttributes:attrs];

    int extra_from_noframe_w = (int)lroundf(noframe_size.width - text_size.width);
    int extra_from_noframe_h = (int)lroundf(noframe_size.height - text_size.height);

    if (extra_from_noframe_w <= 0 || extra_from_noframe_h <= 0)
    {
      int bezel_w = (int)lroundf(bordered_size.width - noframe_size.width);
      int bezel_h = (int)lroundf(bordered_size.height - noframe_size.height);

      cocoa_textfield_extra_w = (extra_from_noframe_w > 0) ? extra_from_noframe_w : (bezel_w > 0 ? bezel_w : 0);
      cocoa_textfield_extra_h = (extra_from_noframe_h > 0) ? extra_from_noframe_h : (bezel_h > 0 ? bezel_h : 0);
    }
    else
    {
      cocoa_textfield_extra_w = extra_from_noframe_w;
      cocoa_textfield_extra_h = extra_from_noframe_h;
    }

    if (cocoa_textfield_extra_w < 0) cocoa_textfield_extra_w = 0;
    if (cocoa_textfield_extra_h < 0) cocoa_textfield_extra_h = 0;

    cocoa_textfield_extra_ws[size] = cocoa_textfield_extra_w;
    cocoa_textfield_extra_hs[size] = cocoa_textfield_extra_h;

    [temp_bordered release];
    [temp_noframe release];
  }

  if (w) *w += cocoa_textfield_extra_w;
  if (h) *h += cocoa_textfield_extra_h;
}

@interface IupCaseFormatter : NSFormatter
@property(nonatomic, assign) BOOL uppercase;
@property(nonatomic, assign) BOOL lowercase;
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCaseFormatter

- (NSString*) stringForObjectValue:(id)obj_val
{
  NSString* str = obj_val;
  if ([self uppercase])
    return [str uppercaseString];
  else if ([self lowercase])
    return [str lowercaseString];
  return str;
}

- (BOOL)getObjectValue:(id*)out_obj_result forString:(NSString*)the_string errorDescription:(NSString**)the_error
{
  if(the_error)
    *the_error = nil;

  if(out_obj_result)
  {
    if ([self uppercase])
      *out_obj_result = [[the_string uppercaseString] retain];
    else if ([self lowercase])
      *out_obj_result = [[the_string lowercaseString] retain];
    else
      *out_obj_result = [the_string retain];
  }
  return YES;
}

- (BOOL)isPartialStringValid:(NSString**)partial_string proposedSelectedRange:(NSRangePointer)proposed_sel_range originalString:(NSString*)orig_string originalSelectedRange:(NSRange)orig_sel_range errorDescription:(NSString**)the_error
{
  if ([self uppercase] && partial_string && *partial_string)
  {
    NSString* upper = [*partial_string uppercaseString];
    if (![*partial_string isEqualToString:upper])
    {
      *partial_string = upper;
      return NO;
    }
  }
  else if ([self lowercase] && partial_string && *partial_string)
  {
    NSString* lower = [*partial_string lowercaseString];
    if (![*partial_string isEqualToString:lower])
    {
      *partial_string = lower;
      return NO;
    }
  }

  Ihandle* ih = [self ihandle];
  if (ih && ih->data->nc > 0)
  {
    NSUInteger glyph_count = (NSUInteger)iupcocoaStrUnitToChar(*partial_string, [*partial_string length]);
    if (glyph_count > ih->data->nc)
    {
      return NO;
    }
  }

  return YES;
}

@end

@interface IupFormatter : NSFormatter
@property(nonatomic, assign) Ihandle* ihandle;
@end

/* I only use isPartialStringValid for "NC" */
@implementation IupFormatter

- (NSString*) stringForObjectValue:(id)obj_val
{
  return obj_val;
}

- (BOOL)getObjectValue:(id*)out_obj_result forString:(NSString*)the_string errorDescription:(NSString**)the_error
{
  if(the_error)
  {
    *the_error = nil;
  }
  if(out_obj_result)
  {
    *out_obj_result = [the_string retain];
  }
  return YES;
}

- (BOOL)isPartialStringValid:(NSString*)partial_string newEditingString:(NSString**)new_string errorDescription:(NSString**)the_error
{
  if(new_string)
  {
    *new_string = nil;
  }
  if(the_error)
  {
    *the_error = nil;
  }

  if([partial_string length] == 0)
  {
    return YES;
  }
  /* Make sure to limit the length if the IUP "NC" feature is in use */
  Ihandle* ih = [self ihandle];
  if(ih->data->nc > 0)
  {
    /* I think this is supposed to be a glyph count instead of a byte count. */
    NSUInteger glyph_count = (NSUInteger)iupcocoaStrUnitToChar(partial_string, [partial_string length]);
    if(glyph_count > ih->data->nc)
    {
      return NO;
    }
  }
  return YES;
}

@end

/* This formatter exists to support the FILTER feature and the "NC" feature. */
@interface IupNumberFormatter : NSNumberFormatter
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupNumberFormatter

- (BOOL)isPartialStringValid:(NSString*)partial_string newEditingString:(NSString**)new_string errorDescription:(NSString**)the_error
{
  if(new_string)
  {
    *new_string = nil;
  }
  if(the_error)
  {
    *the_error = nil;
  }

  if([partial_string length] == 0)
  {
    return YES;
  }
  /* Make sure to limit the length if the IUP "NC" feature is in use */
  Ihandle* ih = [self ihandle];
  if(ih->data->nc > 0)
  {
    NSUInteger glyph_count = (NSUInteger)iupcocoaStrUnitToChar(partial_string, [partial_string length]);
    if(glyph_count > ih->data->nc)
    {
      return NO;
    }
  }

  NSMutableCharacterSet* allowed_character_set = [[NSCharacterSet decimalDigitCharacterSet] mutableCopy];
  [allowed_character_set autorelease];
  if(NSNumberFormatterNoStyle == [self numberStyle])
  {
    [allowed_character_set addCharactersInString:@"-"];
  }
  else
  {
    [allowed_character_set addCharactersInString:@".eE-, "];
  }
  NSCharacterSet* disallowed_character_set = [allowed_character_set invertedSet];

  if([partial_string rangeOfCharacterFromSet:disallowed_character_set].location != NSNotFound)
  {
    return NO;
  }

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSNumberFormatter* conversion_formatter = [[NSNumberFormatter alloc] init];
        [conversion_formatter autorelease];
        [conversion_formatter setNumberStyle:[self numberStyle]];
        NSNumber* ns_number = [conversion_formatter numberFromString:partial_string];

        double current_number = [ns_number doubleValue];
        NSStepper* stepper_view = cocoaTextGetStepperView(ih);
        double max_value = [stepper_view maxValue];
        if(current_number > max_value)
        {
          return NO;
        }
        double min_value = [stepper_view minValue];
        if(current_number < min_value)
        {
          return NO;
        }

        break;
      }
    default:
      {
        break;
      }
  }

  return YES;
}

@end

/* Only for NSTextField. */
static int cocoaTextSetNCAttrib(Ihandle* ih, const char* value)
{
  if(!iupStrToInt(value, &ih->data->nc))
  {
    ih->data->nc = 0;
  }

  if(ih->handle)
  {
    IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
    switch(sub_type)
    {
      case IUPCOCOATEXTSUBTYPE_VIEW:
        {
          break;
        }
      case IUPCOCOATEXTSUBTYPE_FIELD:
        {
          NSTextField* text_field = cocoaTextGetTextField(ih);
          /* If we already attached a formatter, it should already have the code to check for NC. */
          if([text_field formatter])
          {
            return 0;
          }

          IupFormatter* nc_formatter = [[IupFormatter alloc] init];
          [nc_formatter autorelease];
          [nc_formatter setIhandle:ih];
          [text_field setFormatter:nc_formatter];
          return 0;
        }
      case IUPCOCOATEXTSUBTYPE_STEPPER:
        {
          /* Leave the existing the Number formatter alone */
          return 0;
        }
      default:
        {
          break;
        }
    }

    return 0;
  }
  else
  {
    return 1; /* store until not mapped, when mapped will be set again */
  }
}

static int cocoaTextSetFilterAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        if(value)
        {
          if(iupStrEqualNoCase(value, "LOWERCASE"))
          {
            iupAttribSet(ih, "_IUPCOCOA_FILTER_CASE", "LOWER");
            return 1;
          }
          else if(iupStrEqualNoCase(value, "UPPERCASE"))
          {
            iupAttribSet(ih, "_IUPCOCOA_FILTER_CASE", "UPPER");
            return 1;
          }
          else if(iupStrEqualNoCase(value, "NUMBER"))
          {
            iupAttribSet(ih, "_IUPCOCOA_FILTER_NUMBER", "1");
            return 1;
          }
        }

        iupAttribSet(ih, "_IUPCOCOA_FILTER_CASE", NULL);
        iupAttribSet(ih, "_IUPCOCOA_FILTER_NUMBER", NULL);
        return 1;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        objc_setAssociatedObject(text_field, IUP_COCOA_TEXT_FORMATTER_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

        if(NULL == value)
        {
          if(ih->data->nc > 0)
          {
            IupFormatter* nc_formatter = [[IupFormatter alloc] init];
            [nc_formatter autorelease];
            [nc_formatter setIhandle:ih];
            [text_field setFormatter:nc_formatter];
          }
          else
          {
            [text_field setFormatter:nil];
          }
          return 1;
        }

        if(iupStrEqualNoCase(value, "NUMBER"))
        {
          IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
          [number_formatter autorelease];
          [number_formatter setIhandle:ih];
          [number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
          [number_formatter setPartialStringValidationEnabled:YES];
          [number_formatter setNumberStyle:NSNumberFormatterDecimalStyle];
          [text_field setFormatter:number_formatter];
          return 1;
        }
        if(iupStrEqualNoCase(value, "INTEGER"))
        {
          IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
          [number_formatter autorelease];
          [number_formatter setIhandle:ih];
          [number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
          [number_formatter setPartialStringValidationEnabled:YES];
          [number_formatter setNumberStyle:NSNumberFormatterNoStyle];
          [text_field setFormatter:number_formatter];
          return 1;
        }
        if(iupStrEqualNoCase(value, "SCIENTIFIC"))
        {
          IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
          [number_formatter autorelease];
          [number_formatter setIhandle:ih];
          [number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
          [number_formatter setPartialStringValidationEnabled:YES];
          [number_formatter setNumberStyle:NSNumberFormatterScientificStyle];
          [text_field setFormatter:number_formatter];
          return 1;
        }
        if(iupStrEqualNoCase(value, "LOWERCASE"))
        {
          IupCaseFormatter* formatter = [[IupCaseFormatter alloc] init];
          [formatter autorelease];
          [formatter setIhandle:ih];
          [formatter setLowercase:YES];
          [formatter setUppercase:NO];
          [text_field setFormatter:formatter];
          objc_setAssociatedObject(text_field, IUP_COCOA_TEXT_FORMATTER_KEY, formatter, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
          return 1;
        }
        if(iupStrEqualNoCase(value, "UPPERCASE"))
        {
          IupCaseFormatter* formatter = [[IupCaseFormatter alloc] init];
          [formatter autorelease];
          [formatter setIhandle:ih];
          [formatter setUppercase:YES];
          [formatter setLowercase:NO];
          [text_field setFormatter:formatter];
          objc_setAssociatedObject(text_field, IUP_COCOA_TEXT_FORMATTER_KEY, formatter, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
          return 1;
        }
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);

        if(NULL == value)
        {
          IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
          [number_formatter autorelease];
          [number_formatter setIhandle:ih];
          [number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
          [number_formatter setPartialStringValidationEnabled:YES];
          [number_formatter setNumberStyle:NSNumberFormatterDecimalStyle];
          [text_field setFormatter:number_formatter];
          return 1;
        }

        if(iupStrEqualNoCase(value, "NUMBER"))
        {
          IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
          [number_formatter autorelease];
          [number_formatter setIhandle:ih];
          [number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
          [number_formatter setPartialStringValidationEnabled:YES];
          [number_formatter setNumberStyle:NSNumberFormatterDecimalStyle];
          [text_field setFormatter:number_formatter];
          return 1;
        }
        if(iupStrEqualNoCase(value, "INTEGER"))
        {
          IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
          [number_formatter autorelease];
          [number_formatter setIhandle:ih];
          [number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
          [number_formatter setPartialStringValidationEnabled:YES];
          [number_formatter setNumberStyle:NSNumberFormatterNoStyle];
          [text_field setFormatter:number_formatter];
          return 1;
        }
        if(iupStrEqualNoCase(value, "SCIENTIFIC"))
        {
          IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
          [number_formatter autorelease];
          [number_formatter setIhandle:ih];
          [number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
          [number_formatter setPartialStringValidationEnabled:YES];
          [number_formatter setNumberStyle:NSNumberFormatterScientificStyle];
          [text_field setFormatter:number_formatter];
          return 1;
        }
        break;
      }
    default:
      {
        break;
      }
  }

  return 0;
}

static int cocoaTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  if(!ih->data->is_multiline)
  {
    return 0;
  }

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        IupCocoaFont* iup_font = iupcocoaGetFont(ih);

        int tab_size = 8;
        if(value) iupStrToInt(value, &tab_size);

        CGFloat char_width = [[iup_font nativeFont] maximumAdvancement].width;
        if (char_width == 0) char_width = 8;
        CGFloat tab_width = tab_size * char_width;

        NSMutableParagraphStyle* paragraph_style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
        [paragraph_style autorelease];

        NSMutableArray* tab_stops = [NSMutableArray array];
        for (int i = 1; i <= 32; i++)
        {
          NSTextTab* tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentLeft location:i * tab_width options:@{}];
          [tab_stops addObject:tab];
          [tab release];
        }
        [paragraph_style setTabStops:tab_stops];
        [paragraph_style setDefaultTabInterval:tab_width];

        [text_view setDefaultParagraphStyle:paragraph_style];

        NSMutableDictionary* typing_attributes = [[text_view typingAttributes] mutableCopy];
        [typing_attributes autorelease];
        [typing_attributes setObject:paragraph_style forKey:NSParagraphStyleAttributeName];
        [text_view setTypingAttributes:typing_attributes];

        return 1;
      }
    default:
      {
        break;
      }
  }
  return 0;
}

static NSColor* cocoaTextColorFromStr(const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
    return [NSColor colorWithSRGBRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
  return nil;
}

static int cocoaTextSetValueAttrib(Ihandle* ih, const char* value)
{
  NSString* ns_string;

  if(NULL == value)
  {
    ns_string = @"";
  }
  else
  {
    ns_string = iupcocoaStrToNSString(value);
  }

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        IupCocoaFont* iup_font = iupcocoaGetFont(ih);
        NSTextStorage* text_storage = [text_view textStorage];

        NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
        [undo_manager beginUndoGrouping];
        [text_storage beginEditing];

        NSMutableDictionary* attributes = [[iup_font attributeDictionary] mutableCopy];

        if (![attributes objectForKey:NSForegroundColorAttributeName])
        {
          NSColor* fg_color = [(IupCocoaTextView*)text_view defaultColor];
          [attributes setObject:(fg_color ? fg_color : [NSColor textColor]) forKey:NSForegroundColorAttributeName];
        }

        NSAttributedString* attributed_string = [[NSAttributedString alloc] initWithString:ns_string attributes:attributes];
        [attributes release];

        ih->data->disable_callbacks = 1;
        [text_view shouldChangeTextInRange:NSMakeRange(0, [text_storage length]) replacementString:ns_string];
        [text_storage setAttributedString:attributed_string];
        ih->data->disable_callbacks = 0;

        [attributed_string release];

        [text_storage endEditing];
        [text_view didChangeText];
        [undo_manager endUndoGrouping];

        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSCAssert([text_field isKindOfClass:[NSTextField class]], @"Expected NSTextField");
        IupCocoaFont* iup_font = iupcocoaGetFont(ih);
        if([iup_font usesAttributes])
        {
          NSAttributedString* attributed_string = [[NSAttributedString alloc] initWithString:ns_string attributes:[iup_font attributeDictionary]];
          [text_field setAttributedStringValue:attributed_string];
          [attributed_string release];
        }
        else
        {
          [text_field setStringValue:ns_string];
        }
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSCAssert([text_field isKindOfClass:[NSTextField class]], @"Expected NSTextField");

        if(!iupAttribGetBoolean(ih, "SPINAUTO"))
        {
          [text_field setStringValue:ns_string];
        }
        else
        {
          NSNumberFormatter* conversion_formatter = [[NSNumberFormatter alloc] init];
          [conversion_formatter autorelease];
          NSNumberFormatter* text_field_formatter = (NSNumberFormatter*)[text_field formatter];
          [conversion_formatter setNumberStyle:[text_field_formatter numberStyle]];
          NSNumber* ns_number = [conversion_formatter numberFromString:ns_string];

          double current_number = [ns_number doubleValue];
          NSStepper* stepper_view = cocoaTextGetStepperView(ih);
          double max_value = [stepper_view maxValue];
          if(current_number > max_value)
            current_number = max_value;
          double min_value = [stepper_view minValue];
          if(current_number < min_value)
            current_number = min_value;

          [stepper_view setDoubleValue:current_number];
          [text_field setIntegerValue:(NSInteger)current_number];
        }

        break;
      }
    default:
      {
        break;
      }
  }

  return 0;
}

static char* cocoaTextGetValueAttrib(Ihandle* ih)
{
  char* value = NULL;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);

        NSString* ns_string = [[text_view textStorage] string];
        value = iupStrReturnStr([ns_string UTF8String]);

        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);

        NSString* ns_string = [text_field stringValue];
        value = iupStrReturnStr([ns_string UTF8String]);

        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);

        NSString* ns_string = [text_field stringValue];
        value = iupStrReturnStr([ns_string UTF8String]);

        break;
      }
    default:
      {
        break;
      }
  }

  if(NULL == value)
  {
    value = "";
  }

  return value;
}

static void cocoaTextFieldSetBackground(Ihandle* ih, NSTextField* text_field, NSColor* color)
{
  if (!iupAttribGetBoolean(ih, "BORDER"))
  {
    [text_field setBezeled:NO];
    [text_field setBordered:NO];
  }
  else
  {
#ifndef GNUSTEP
    [text_field setBezeled:color == nil];
    [text_field setBordered:YES];
#else
    [text_field setBezeled:YES];
#endif
  }
  [text_field setDrawsBackground:YES];
  [text_field setBackgroundColor:color ? color : [NSColor textBackgroundColor]];
}

static int cocoaTextSetBgColorAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);

        NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
        [undo_manager beginUndoGrouping];

        NSColor* the_color = cocoaTextColorFromStr(value);
        if (the_color)
          [text_view setBackgroundColor:the_color];
        else
          [text_view setBackgroundColor:[NSColor textBackgroundColor]];

        [text_view didChangeText];
        [undo_manager endUndoGrouping];

        return 1;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        cocoaTextFieldSetBackground(ih, cocoaTextGetTextField(ih), cocoaTextColorFromStr(value));
        return 1;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        cocoaTextFieldSetBackground(ih, cocoaTextGetStepperTextField(ih), cocoaTextColorFromStr(value));
        return 1;
      }
    default:
      {
        break;
      }
  }

  return 0;
}

static int cocoaTextSetFgColorAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        IupCocoaTextView* text_view = (IupCocoaTextView*)cocoaTextGetTextView(ih);
        NSTextStorage* text_storage = [text_view textStorage];
        NSUInteger length = [text_storage length];
        NSUInteger index = 0;
        NSColor* old_color = [text_view defaultColor];
        NSMutableDictionary* typing;

        NSColor* the_color = cocoaTextColorFromStr(value);
        if (!the_color || (!iupGlobalDefaultColorChanged("TXTFGCOLOR") && iupStrEqualNoCase(value, IupGetGlobal("TXTFGCOLOR"))))
          the_color = [NSColor textColor];

        NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
        [undo_manager beginUndoGrouping];

        /* runs without a color of their own follow the default; format tag colors stay */
        [text_storage beginEditing];
        while (index < length)
        {
          NSRange run;
          id fg = [text_storage attribute:NSForegroundColorAttributeName atIndex:index
                    longestEffectiveRange:&run inRange:NSMakeRange(index, length - index)];
          if (!fg || [fg isEqual:old_color])
            [text_storage addAttribute:NSForegroundColorAttributeName value:the_color range:run];
          index = NSMaxRange(run);
        }
        [text_storage endEditing];

        typing = [[text_view typingAttributes] mutableCopy];
        [typing setObject:the_color forKey:NSForegroundColorAttributeName];
        [text_view setTypingAttributes:typing];
        [typing release];
        [text_view setDefaultColor:the_color];

        [text_view didChangeText];
        [undo_manager endUndoGrouping];

        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSColor* the_color = cocoaTextColorFromStr(value);
        if (the_color)
          [text_field setTextColor:the_color];
        else
          [text_field setTextColor:nil];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSColor* the_color = cocoaTextColorFromStr(value);
        if (the_color)
          [text_field setTextColor:the_color];
        else
          [text_field setTextColor:nil];
        break;
      }
    default:
      {
        break;
      }
  }

  return 1;
}

#define COCOA_TEXT_CHUNK 4096

/* index after the n-th newline from "from"; stops at the last line, *skipped says how many were found */
static NSUInteger cocoaTextSkipLines(NSString* text, NSUInteger from, NSUInteger n, NSUInteger* skipped)
{
  NSUInteger length = [text length], line_start = from;
  unichar buf[COCOA_TEXT_CHUNK];

  *skipped = 0;
  while (*skipped < n && from < length)
  {
    NSUInteger k = length - from < COCOA_TEXT_CHUNK ? length - from : COCOA_TEXT_CHUNK, i;
    [text getCharacters:buf range:NSMakeRange(from, k)];
    for (i = 0; i < k && *skipped < n; i++)
    {
      if (buf[i] == '\n')
      {
        (*skipped)++;
        line_start = from + i + 1;
      }
    }
    from += k;
  }
  return line_start;
}

/* newlines in [from, to); *line_start is set after the last one */
static NSUInteger cocoaTextCountLines(NSString* text, NSUInteger from, NSUInteger to, NSUInteger* line_start)
{
  NSUInteger count = 0;
  unichar buf[COCOA_TEXT_CHUNK];

  while (from < to)
  {
    NSUInteger k = to - from < COCOA_TEXT_CHUNK ? to - from : COCOA_TEXT_CHUNK, i;
    [text getCharacters:buf range:NSMakeRange(from, k)];
    for (i = 0; i < k; i++)
    {
      if (buf[i] == '\n')
      {
        count++;
        *line_start = from + i + 1;
      }
    }
    from += k;
  }
  return count;
}

static bool cocoaTextComputeRangeFromLineColumnForTextView(NSTextView* text_view, NSUInteger start_line, NSUInteger start_column, NSUInteger end_line, NSUInteger end_column, NSRange* out_range)
{
  *out_range = NSMakeRange(0, 0);

  if(start_line < 1)
    start_line = 1;
  if(end_line < 1)
    end_line = 1;
  if(end_line < start_line)
    return false;
  if((end_line == start_line) && (end_column < start_column))
    return false;

  NSString* text = [[text_view textStorage] string];
  NSUInteger text_length = [text length];

  if(text_length == 0)
  {
    if((start_line <= 1) && (start_column <= 1))
      return true;
    return false;
  }

  /* lines are logical paragraphs delimited by newlines, not wrapped display lines */
  NSUInteger skipped;
  NSUInteger line_start = cocoaTextSkipLines(text, 0, start_line - 1, &skipped);
  if(skipped < start_line - 1)
    return false;

  NSUInteger start_pos = iupcocoaStrAdvanceChars(text, line_start, start_column - 1);

  NSUInteger end_line_start = line_start;
  if(end_line > start_line)
    end_line_start = cocoaTextSkipLines(text, line_start, end_line - start_line, &skipped);

  NSUInteger end_line_end = cocoaTextSkipLines(text, end_line_start, 1, &skipped);
  if(skipped == 0)
    end_line_end = text_length;

  NSUInteger end_pos = iupcocoaStrAdvanceChars(text, end_line_start, end_column - 1);
  if(end_pos > end_line_end)
    end_pos = end_line_end;

  *out_range = NSMakeRange(start_pos, end_pos - start_pos);
  return true;
}

static bool cocoaTextComputeLineColumnFromRangeForTextView(NSTextView* text_view, NSRange native_selection_range, NSUInteger* out_start_line, NSUInteger* out_start_column, NSUInteger* out_end_line, NSUInteger* out_end_column)
{
  *out_start_line = 1;
  *out_start_column = 1;
  *out_end_line = 1;
  *out_end_column = 1;

  NSString* text = [[text_view textStorage] string];
  NSUInteger text_length = [text length];

  if(text_length == 0)
  {
    if(native_selection_range.location == 0)
      return true;
    return false;
  }

  NSUInteger start_pos = native_selection_range.location;
  NSUInteger end_pos = start_pos + native_selection_range.length;
  if(start_pos > text_length) start_pos = text_length;
  if(end_pos > text_length) end_pos = text_length;

  NSUInteger line_start = 0;
  NSUInteger line = 1 + cocoaTextCountLines(text, 0, start_pos, &line_start);

  *out_start_line = line;
  *out_start_column = iupcocoaStrCountChars(text, line_start, start_pos) + 1;

  line += cocoaTextCountLines(text, start_pos, end_pos, &line_start);

  *out_end_line = line;
  *out_end_column = iupcocoaStrCountChars(text, line_start, end_pos) + 1;

  return true;
}

IUP_SDK_API void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int* pos)
{
  if (cocoaTextGetSubType(ih) != IUPCOCOATEXTSUBTYPE_VIEW)
  {
    if (pos)
      *pos = col > 0 ? col - 1 : 0;
    return;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSUInteger lin_start = lin;
  NSUInteger col_start = col;
  NSUInteger lin_end = lin;
  NSUInteger col_end = col;
  NSRange native_selection_range = NSMakeRange(0, 0);

  bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &native_selection_range);
  if(did_find_range)
  {
    if(pos)
    {
      *pos = iupcocoaStrUnitToChar([[text_view textStorage] string], native_selection_range.location);
    }
  }
}

IUP_SDK_API void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int* lin, int* col)
{
  if (cocoaTextGetSubType(ih) != IUPCOCOATEXTSUBTYPE_VIEW)
  {
    if (lin)
      *lin = 1;
    if (col)
      *col = pos + 1;
    return;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSRange range_position = NSMakeRange(iupcocoaStrCharToUnit([[text_view textStorage] string], pos), 0);

  NSUInteger lin_start=1;
  NSUInteger col_start=1;
  NSUInteger lin_end=1;
  NSUInteger col_end=1;
  bool did_find_range = cocoaTextComputeLineColumnFromRangeForTextView(text_view, range_position, &lin_start, &col_start, &lin_end, &col_end);
  if(did_find_range)
  {
    if(lin)
    {
      *lin = (int)lin_start;
    }
    if(col)
    {
      *col = (int)col_start;
    }
  }
}

IUP_SDK_API void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
  [undo_manager beginUndoGrouping];
  /* Each child parser does its own text-storage begin/endEditing. */
  return NULL;
}

IUP_SDK_API void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
  [undo_manager endUndoGrouping];
}

/* You must pass in a valid paragraph_style. This routine will alter it depending on which Iup attributes were set. */
static bool cocoaTextParseParagraphAttributes(NSMutableParagraphStyle* paragraph_style, Ihandle* formattag)
{
  bool needs_paragraph_style = false;
  const char* format;

  format = iupAttribGet(formattag, "ALIGNMENT");
  if(format)
  {
    if(iupStrEqualNoCase(format, "JUSTIFY"))
    {
      [paragraph_style setAlignment:NSTextAlignmentJustified];
    }
    else if(iupStrEqualNoCase(format, "RIGHT"))
    {
      [paragraph_style setAlignment:NSTextAlignmentRight];
    }
    else if(iupStrEqualNoCase(format, "CENTER"))
    {
      [paragraph_style setAlignment:NSTextAlignmentCenter];
    }
    else /* "LEFT" */
    {
      [paragraph_style setAlignment:NSTextAlignmentLeft];
    }
    needs_paragraph_style = true;
  }

  format = iupAttribGet(formattag, "INDENT");
  if(format)
  {
    int val = 0;
    if(iupStrToInt(format, &val))
    {
      [paragraph_style setFirstLineHeadIndent:(CGFloat)val];
      needs_paragraph_style = true;
    }

    format = iupAttribGet(formattag, "INDENTRIGHT");
    if(format && iupStrToInt(format, &val))
    {
      [paragraph_style setTailIndent:(CGFloat)(-val)];
      needs_paragraph_style = true;
    }

    format = iupAttribGet(formattag, "INDENTOFFSET");
    if(format && iupStrToInt(format, &val))
    {
      [paragraph_style setHeadIndent:(CGFloat)val];
      needs_paragraph_style = true;
    }
  }

  format = iupAttribGet(formattag, "LINESPACING");
  if(format)
  {
    double val = 0;

    if(iupStrEqualNoCase(format, "SINGLE"))
    {
      [paragraph_style setLineSpacing:1.0];
    }
    else if(iupStrEqualNoCase(format, "ONEHALF"))
    {
      [paragraph_style setLineSpacing:1.5];
    }
    else if(iupStrEqualNoCase(format, "DOUBLE"))
    {
      [paragraph_style setLineSpacing:2.0];
    }
    else if(iupStrToDouble(format, &val))
    {
      [paragraph_style setLineSpacing:val];
    }
    needs_paragraph_style = true;
  }

  format = iupAttribGet(formattag, "SPACEBEFORE");
  if(format)
  {
    double val = 0;
    if(iupStrToDouble(format, &val))
    {
      [paragraph_style setParagraphSpacingBefore:val];
      needs_paragraph_style = true;
    }
  }

  format = iupAttribGet(formattag, "SPACEAFTER");
  if(format)
  {
    double val = 0;
    if(iupStrToDouble(format, &val))
    {
      [paragraph_style setParagraphSpacing:val];
      needs_paragraph_style = true;
    }
  }

  format = iupAttribGet(formattag, "TABSARRAY");
  if(format)
  {
    int pos = 0;
    int prev_pos = 0;
    int last_pos = 0;
    char* str;
    NSMutableArray* tab_array = [NSMutableArray array];

    while(format)
    {
      str = iupStrDupUntil(&format, ' ');
      if (!str) break;
      iupStrToInt(str, &pos);
      free(str);

      str = iupStrDupUntil(&format, ' ');
      if (!str)
      {
        if (!format || !*format) break;
        str = iupStrDup(format);
        format = NULL;
      }

      NSTextTab* text_tab = nil;
      if(iupStrEqualNoCase(str, "LEFT"))
      {
        text_tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentLeft location:(CGFloat)pos options:[NSDictionary dictionary]];
      }
      else if(iupStrEqualNoCase(str, "RIGHT"))
      {
        text_tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentRight location:(CGFloat)pos options:[NSDictionary dictionary]];
      }
      else if(iupStrEqualNoCase(str, "CENTER"))
      {
        text_tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentCenter location:(CGFloat)pos options:[NSDictionary dictionary]];
      }
      else
      {
        text_tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentLeft location:(CGFloat)pos options:[NSDictionary dictionary]];
      }
      free(str);

      [tab_array addObject:text_tab];
      [text_tab release];

      prev_pos = last_pos;
      last_pos = pos;
    }

    [paragraph_style setTabStops:tab_array];

    CGFloat default_interval = (CGFloat)(last_pos - prev_pos);
    if (default_interval <= 0.0)
      default_interval = (CGFloat)last_pos;
    if (default_interval > 0.0)
      [paragraph_style setDefaultTabInterval:default_interval];

    needs_paragraph_style = true;
  }

  return needs_paragraph_style;
}

static bool cocoaTextParseParagraphFormat(Ihandle* ih, Ihandle* formattag, NSTextView* text_view, NSRange selection_range)
{
  NSTextStorage* text_storage = [text_view textStorage];
  NSString* all_string = [text_storage string];
  __block bool did_change_attribute = false;

  /* shouldChangeTextInRange: triggers layout; must be outside begin/endEditing. */
  ih->data->disable_callbacks = 1;
  [text_view shouldChangeTextInRange:selection_range replacementString:nil];
  ih->data->disable_callbacks = 0;

  [text_storage beginEditing];

  [all_string enumerateSubstringsInRange:selection_range options:NSStringEnumerationByParagraphs usingBlock:
    ^(NSString * _Nullable substring, NSRange substring_range, NSRange enclosing_range, BOOL * _Nonnull stop)
    {
      *stop = NO;

      NSUInteger start_paragraph_index;
      NSUInteger end_paragraph_index;
      NSUInteger contents_end_paragraph_index;
      [all_string getParagraphStart:&start_paragraph_index end:&end_paragraph_index contentsEnd:&contents_end_paragraph_index forRange:enclosing_range];

      NSRange paragraph_range = NSMakeRange(start_paragraph_index, end_paragraph_index-start_paragraph_index);

      NSAttributedString* current_paragraph = [text_storage attributedSubstringFromRange:paragraph_range];
      NSDictionary<NSAttributedStringKey, id>* current_paragraph_attributes = [current_paragraph attributesAtIndex:0 effectiveRange:NULL];

      NSMutableParagraphStyle* paragraph_style = nil;
      paragraph_style = [[current_paragraph_attributes objectForKey:NSParagraphStyleAttributeName] mutableCopy];
      if(nil == paragraph_style)
      {
        paragraph_style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
      }
      [paragraph_style autorelease];

      bool needs_paragraph_style = cocoaTextParseParagraphAttributes(paragraph_style, formattag);
      if(needs_paragraph_style)
      {
        [text_storage addAttribute:NSParagraphStyleAttributeName value:paragraph_style range:paragraph_range];
        did_change_attribute = true;
      }
    }
  ];

  [text_storage endEditing];

  if(did_change_attribute)
  {
    [text_view didChangeText];
  }
  return did_change_attribute;
}

const NSUInteger kIupNumberingStyleNone = 0;
const NSUInteger kIupNumberingStyleRightParenthesis = 1;
const NSUInteger kIupNumberingStyleParenthesis = 2;
const NSUInteger kIupNumberingStylePeriod = 3;
const NSUInteger kIupNumberingStyleNonNumber = 4;

@interface IupNumberingTextList : NSTextList

@property(nonatomic, assign) NSUInteger numberingStyle;

- (NSString*) markerForItemNumber:(NSInteger)item_num;

@end

@implementation IupNumberingTextList

- (NSString*) markerForItemNumber:(NSInteger)item_num
{
  NSString* base_string = [super markerForItemNumber:item_num];
  NSString* customized_marker = nil;
  switch([self numberingStyle])
  {
    case kIupNumberingStyleRightParenthesis:
      {
        customized_marker = [NSString stringWithFormat:@"%@)", base_string];
        break;
      }
    case kIupNumberingStyleParenthesis:
      {
        customized_marker = [NSString stringWithFormat:@"(%@)", base_string];
        break;
      }
    case kIupNumberingStylePeriod:
      {
        customized_marker = [NSString stringWithFormat:@"%@.", base_string];
        break;
      }
    case kIupNumberingStyleNonNumber:
      {
        customized_marker = @"";
        break;
      }

    case kIupNumberingStyleNone:
                        default:
      {
        customized_marker = base_string;
        break;
      }
  }

  return customized_marker;
}

@end

static bool cocoaTextParseBulletNumberListFormat(Ihandle* ih, Ihandle* formattag, NSTextView* text_view, NSRange selection_range)
{
  static NSString* IupNSTextListMarkerBox = @"{box}";
  static NSString* IupNSTextListMarkerCheck = @"{check}";
  static NSString* IupNSTextListMarkerCircle = @"{circle}";
  static NSString* IupNSTextListMarkerDiamond = @"{diamond}";
  static NSString* IupNSTextListMarkerDisc = @"{disc}";
  static NSString* IupNSTextListMarkerHyphen = @"{hyphen}";
  static NSString* IupNSTextListMarkerSquare = @"{square}";
  static NSString* IupNSTextListMarkerLowercaseHexadecimal = @"{lower-hexadecimal}";
  static NSString* IupNSTextListMarkerUppercaseHexadecimal = @"{upper-hexadecimal}";
  static NSString* IupNSTextListMarkerOctal = @"{octal}";
  static NSString* IupNSTextListMarkerLowercaseAlpha = @"{lower-alpha}";
  static NSString* IupNSTextListMarkerUppercaseAlpha = @"{upper-alpha}";
  static NSString* IupNSTextListMarkerLowercaseLatin = @"{lower-latin}";
  static NSString* IupNSTextListMarkerUppercaseLatin = @"{upper-latin}";
  static NSString* IupNSTextListMarkerLowercaseRoman = @"{lower-roman}";
  static NSString* IupNSTextListMarkerUppercaseRoman = @"{upper-roman}";
  static NSString* IupNSTextListMarkerDecimal = @"{decimal}";

  char* format;
  bool use_list = false;
  NSString* which_list_marker = nil;

  format = iupAttribGet(formattag, "NUMBERING");
  if(format)
  {
    use_list = true;
    if(iupStrEqualNoCase(format, "BULLET"))
    {
      which_list_marker = IupNSTextListMarkerDisc;
    }
    else if(iupStrEqualNoCase(format, "ARABIC"))
    {
      which_list_marker = IupNSTextListMarkerDecimal;
    }
    else if(iupStrEqualNoCase(format, "LCLETTER"))
    {
      which_list_marker = IupNSTextListMarkerLowercaseAlpha;
    }
    else if(iupStrEqualNoCase(format, "UCLETTER"))
    {
      which_list_marker = IupNSTextListMarkerUppercaseAlpha;
    }
    else if(iupStrEqualNoCase(format, "LCROMAN"))
    {
      which_list_marker = IupNSTextListMarkerLowercaseRoman;
    }
    else if(iupStrEqualNoCase(format, "UCROMAN"))
    {
      which_list_marker = IupNSTextListMarkerUppercaseRoman;
    }
    else if(iupStrEqualNoCase(format, "NONE"))
    {
      use_list = false;
    }
    else if(iupStrEqualNoCase(format, "BOX"))
    {
      which_list_marker = IupNSTextListMarkerBox;
    }
    else if(iupStrEqualNoCase(format, "CHECK"))
    {
      which_list_marker = IupNSTextListMarkerCheck;
    }
    else if(iupStrEqualNoCase(format, "CIRCLE"))
    {
      which_list_marker = IupNSTextListMarkerCircle;
    }
    else if(iupStrEqualNoCase(format, "DIAMOND"))
    {
      which_list_marker = IupNSTextListMarkerDiamond;
    }
    else if(iupStrEqualNoCase(format, "HYPHEN"))
    {
      which_list_marker = IupNSTextListMarkerHyphen;
    }
    else if(iupStrEqualNoCase(format, "SQUARE"))
    {
      which_list_marker = IupNSTextListMarkerSquare;
    }
    else if(iupStrEqualNoCase(format, "LCHEX"))
    {
      which_list_marker = IupNSTextListMarkerLowercaseHexadecimal;
    }
    else if(iupStrEqualNoCase(format, "UCHEX"))
    {
      which_list_marker = IupNSTextListMarkerUppercaseHexadecimal;
    }
    else if(iupStrEqualNoCase(format, "OCTAL"))
    {
      which_list_marker = IupNSTextListMarkerOctal;
    }
    else if(iupStrEqualNoCase(format, "LCLATIN"))
    {
      which_list_marker = IupNSTextListMarkerLowercaseLatin;
    }
    else if(iupStrEqualNoCase(format, "UCLATIN"))
    {
      which_list_marker = IupNSTextListMarkerUppercaseLatin;
    }
    else
    {
      use_list = false;
    }

    if(use_list)
    {
      NSUInteger which_number_style = kIupNumberingStyleNone;

      format = iupAttribGet(formattag, "NUMBERINGSTYLE");
      if(format)
      {
        if(iupStrEqualNoCase(format, "RIGHTPARENTHESIS"))
        {
          which_number_style = kIupNumberingStyleRightParenthesis;
        }
        else if(iupStrEqualNoCase(format, "PARENTHESES"))
        {
          which_number_style = kIupNumberingStyleParenthesis;
        }
        else if(iupStrEqualNoCase(format, "PERIOD"))
        {
          which_number_style = kIupNumberingStylePeriod;
        }
        else if(iupStrEqualNoCase(format, "NONUMBER"))
        {
          which_number_style = kIupNumberingStyleNonNumber;
        }
        else if(iupStrEqualNoCase(format, "NONE"))
        {
          which_number_style = kIupNumberingStyleNone;
        }
        else
        {
          which_number_style = kIupNumberingStyleNone;
        }
      }

      NSTextStorage* text_storage = [text_view textStorage];
      NSString* all_string = [text_storage string];

      [text_storage beginEditing];

      __block NSUInteger applied_paragraph_start = NSUIntegerMax;
      __block NSUInteger applied_paragraph_end = 0;
      __block NSMutableArray<NSTextList*>* array_of_text_lists = [NSMutableArray array];
      [all_string enumerateSubstringsInRange:selection_range options:NSStringEnumerationByParagraphs usingBlock:
        ^(NSString * _Nullable substring, NSRange substring_range, NSRange enclosing_range, BOOL * _Nonnull stop)
        {
          *stop = NO;

          NSUInteger start_paragraph_index;
          NSUInteger end_paragraph_index;
          NSUInteger contents_end_paragraph_index;
          [all_string getParagraphStart:&start_paragraph_index end:&end_paragraph_index contentsEnd:&contents_end_paragraph_index forRange:enclosing_range];

          if(applied_paragraph_start > start_paragraph_index)
          {
            applied_paragraph_start = start_paragraph_index;
          }
          if(applied_paragraph_end < end_paragraph_index)
          {
            applied_paragraph_end = end_paragraph_index;
          }

          IupNumberingTextList* text_list = [[IupNumberingTextList alloc] initWithMarkerFormat:which_list_marker options:0];
          [text_list autorelease];
          [text_list setNumberingStyle:which_number_style];
          [array_of_text_lists addObject:text_list];
        }
      ];

      if(applied_paragraph_start == NSUIntegerMax)
      {
        applied_paragraph_start = selection_range.location;
      }
      if(applied_paragraph_end == 0)
      {
        applied_paragraph_end = selection_range.location + selection_range.length;
      }

      NSRange applied_paragraph_range = NSMakeRange(applied_paragraph_start, applied_paragraph_end-applied_paragraph_start);
      NSMutableDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[[text_storage attributedSubstringFromRange:applied_paragraph_range] attributesAtIndex:0 effectiveRange:NULL] mutableCopy];
      [text_storage_attributes autorelease];
      NSMutableParagraphStyle* paragraph_style = [[text_storage_attributes objectForKey:NSParagraphStyleAttributeName] mutableCopy];
      [paragraph_style autorelease];
      if(nil == paragraph_style)
      {
        paragraph_style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
        [paragraph_style autorelease];
      }

      [paragraph_style setTextLists:array_of_text_lists];

      int numberingtab = 24;
      char* tab_str = iupAttribGet(formattag, "NUMBERINGTAB");
      if(tab_str)
        iupStrToInt(tab_str, &numberingtab);

      if(!iupAttribGet(formattag, "INDENT"))
      {
        [paragraph_style setFirstLineHeadIndent:0.0];
        [paragraph_style setHeadIndent:(CGFloat)numberingtab];
      }

      if(!iupAttribGet(formattag, "TABSARRAY"))
      {
        NSTextTab* tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentLeft location:(CGFloat)numberingtab options:[NSDictionary dictionary]];
        [paragraph_style setTabStops:[NSArray arrayWithObject:tab]];
        [tab release];
      }

      [text_storage addAttribute:NSParagraphStyleAttributeName value:paragraph_style range:applied_paragraph_range];

      [text_storage endEditing];
      [text_view didChangeText];

    }
    else
    {
      NSTextStorage* text_storage = [text_view textStorage];
      NSString* all_string = [text_storage string];

      [text_storage beginEditing];

      __block NSUInteger applied_paragraph_start = NSUIntegerMax;
      __block NSUInteger applied_paragraph_end = 0;
      [all_string enumerateSubstringsInRange:selection_range options:NSStringEnumerationByParagraphs usingBlock:
        ^(NSString * _Nullable substring, NSRange substring_range, NSRange enclosing_range, BOOL * _Nonnull stop)
        {
          *stop = NO;

          NSUInteger start_paragraph_index;
          NSUInteger end_paragraph_index;
          NSUInteger contents_end_paragraph_index;
          [all_string getParagraphStart:&start_paragraph_index end:&end_paragraph_index contentsEnd:&contents_end_paragraph_index forRange:enclosing_range];
          if(applied_paragraph_start > start_paragraph_index)
          {
            applied_paragraph_start = start_paragraph_index;
          }
          if(applied_paragraph_end < end_paragraph_index)
          {
            applied_paragraph_end = end_paragraph_index;
          }
        }
      ];

      if(applied_paragraph_start != NSUIntegerMax && applied_paragraph_end > 0)
      {
        NSRange applied_paragraph_range = NSMakeRange(applied_paragraph_start, applied_paragraph_end-applied_paragraph_start);
        NSMutableDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[[text_storage attributedSubstringFromRange:applied_paragraph_range] attributesAtIndex:0 effectiveRange:NULL] mutableCopy];
        [text_storage_attributes autorelease];
        NSMutableParagraphStyle* paragraph_style = [[text_storage_attributes objectForKey:NSParagraphStyleAttributeName] mutableCopy];
        [paragraph_style autorelease];
        if(paragraph_style)
        {
          [paragraph_style setTextLists:[NSArray array]];
          [paragraph_style setFirstLineHeadIndent:0.0];
          [paragraph_style setHeadIndent:0.0];
          [text_storage addAttribute:NSParagraphStyleAttributeName value:paragraph_style range:applied_paragraph_range];
        }
      }

      [text_storage endEditing];
      [text_view didChangeText];
    }
    return true;
  }
  return false;
}

static NSFont* cocoaTextChangeFontWeight(NSFont* start_font, int font_target_weight)
{
  NSFontManager* font_manager = [NSFontManager sharedFontManager];

  if (font_target_weight >= 6)
    return [font_manager convertFont:start_font toHaveTrait:NSBoldFontMask];
  else
    return [font_manager convertFont:start_font toNotHaveTrait:NSBoldFontMask];
}

static NSFont* cocoaTextApplySmallCaps(NSFont* font)
{
#ifndef GNUSTEP
  NSDictionary* feature = @{
    NSFontFeatureTypeIdentifierKey: @(37),    /* kLowerCaseType */
    NSFontFeatureSelectorIdentifierKey: @(1)  /* kLowerCaseSmallCapsSelector */
  };
  NSFontDescriptor* desc = [[font fontDescriptor] fontDescriptorByAddingAttributes:@{ NSFontFeatureSettingsAttribute: @[feature] }];
  NSFont* result = [NSFont fontWithDescriptor:desc size:[font pointSize]];
  return result ? result : font;
#else
  return font;
#endif
}

typedef struct {
  bool change_family;
  bool change_size;
  bool change_traits;
  bool change_weight;
  bool change_smallcaps;
  NSString* family_name;
  CGFloat size;
  NSFontTraitMask trait_mask;
  int weight;
} IupCocoaTextFontIntent;

static void cocoaTextApplyFontChanges(NSMutableDictionary* attribute_dict, NSDictionary* base_attributes, IupCocoaFont* iup_font, const IupCocoaTextFontIntent* intent)
{
  if(!(intent->change_family || intent->change_traits || intent->change_size || intent->change_weight || intent->change_smallcaps))
    return;

  NSFontManager* font_manager = [NSFontManager sharedFontManager];
  NSFont* target_font = [base_attributes objectForKey:NSFontAttributeName];
  if(nil == target_font)
    target_font = [iup_font nativeFont];

  if(intent->change_family)
    target_font = [font_manager convertFont:target_font toFamily:intent->family_name];
  if(intent->change_size)
    target_font = [font_manager convertFont:target_font toSize:intent->size];
  if(intent->change_traits)
    target_font = [font_manager convertFont:target_font toHaveTrait:intent->trait_mask];
  if(intent->change_weight)
    target_font = cocoaTextChangeFontWeight(target_font, intent->weight);
  if(intent->change_smallcaps)
    target_font = cocoaTextApplySmallCaps(target_font);

  [attribute_dict setObject:target_font forKey:NSFontAttributeName];
}

static NSMutableDictionary* cocoaTextParseCharacterFormat(Ihandle* ih, Ihandle* formattag, NSTextView* text_view, NSRange selection_range)
{
  char* format;
  bool did_change_attribute = false;
  bool did_change_font_size = false;
  bool did_change_font_family = false;
  bool did_change_font_traits = false;
  bool did_change_font_weight = false;
  bool did_change_smallcaps = false;

  bool did_change_font_fgcolor = false;
  bool did_change_font_bgcolor = false;

  bool needs_add_font_fgcolor = false;
  bool needs_add_font_bgcolor = false;

  NSString* font_family_name = nil;
  CGFloat font_size = 0.0;
  NSFontTraitMask trait_mask = 0;
  int font_target_weight = 5;
  NSMutableDictionary* attribute_dict = [NSMutableDictionary dictionary];

  format = iupAttribGet(formattag, "FONTSIZE");
  if(format)
  {
    int font_size_int = 0;
    iupStrToInt(format, &font_size_int);
    if(font_size_int >= 0)
    {
      font_size = (CGFloat)font_size_int;
      did_change_font_size = true;
      did_change_attribute = true;
    }
  }

  format = iupAttribGet(formattag, "FONTFACE");
  if(format)
  {
    const char* mapped_name = iupFontGetMacName(format);
    if(mapped_name)
    {
      font_family_name = iupcocoaStrToNSString(mapped_name);
    }
    else
    {
      font_family_name = iupcocoaStrToNSString(format);
    }

    did_change_attribute = true;
    did_change_font_family = true;
  }

  format = iupAttribGet(formattag, "RISE");
  if(format)
  {
    if(iupStrEqualNoCase(format, "SUPERSCRIPT"))
    {
      [attribute_dict setValue:[NSNumber numberWithInt:1]
                        forKey:NSSuperscriptAttributeName];
      font_size = font_size * 0.6444444444444;
      did_change_font_size = true;
    }
    else if(iupStrEqualNoCase(format, "SUBSCRIPT"))
    {
      [attribute_dict setValue:[NSNumber numberWithInt:-1]
                        forKey:NSSuperscriptAttributeName];
      font_size = font_size * 0.6444444444444;
      did_change_font_size = true;
    }
    else
    {
      int offset_val = 0;
      iupStrToInt(format, &offset_val);
      [attribute_dict setValue:[NSNumber numberWithDouble:(double)offset_val]
                        forKey:NSBaselineOffsetAttributeName];
    }
    did_change_attribute = true;
  }

  format = iupAttribGet(formattag, "FONTSCALE");
  if(format)
  {
    double fval = 0;
    if (iupStrEqualNoCase(format, "XX-SMALL"))
      fval = 0.5787037037037;
    else if (iupStrEqualNoCase(format, "X-SMALL"))
      fval = 0.6444444444444;
    else if (iupStrEqualNoCase(format, "SMALL"))
      fval = 0.8333333333333;
    else if (iupStrEqualNoCase(format, "MEDIUM"))
      fval = 1.0;
    else if (iupStrEqualNoCase(format, "LARGE"))
      fval = 1.2;
    else if (iupStrEqualNoCase(format, "X-LARGE"))
      fval = 1.4399999999999;
    else if (iupStrEqualNoCase(format, "XX-LARGE"))
      fval = 1.728;
    else
      iupStrToDouble(format, &fval);

    if(fval > 0)
    {
      font_size = font_size * fval;
      did_change_font_size = true;
      did_change_attribute = true;
    }
  }

  format = iupAttribGet(formattag, "ITALIC");
  if(format)
  {
    if(iupStrBoolean(format))
    {
      trait_mask |= NSItalicFontMask;
      did_change_attribute = true;
      did_change_font_traits = true;
    }
  }

  format = iupAttribGet(formattag, "UNDERLINE");
  if(format)
  {
    if(iupStrEqualNoCase(format, "SINGLE"))
    {
      [attribute_dict setValue:[NSNumber numberWithInt:NSUnderlinePatternSolid|NSUnderlineStyleSingle]
                        forKey:NSUnderlineStyleAttributeName];
    }
    else if(iupStrEqualNoCase(format, "DOUBLE"))
    {
      [attribute_dict setValue:[NSNumber numberWithInt:NSUnderlinePatternSolid|NSUnderlineStyleDouble]
                        forKey:NSUnderlineStyleAttributeName];
    }
    else if(iupStrEqualNoCase(format, "DOTTED"))
    {
      [attribute_dict setValue:[NSNumber numberWithInt:NSUnderlineStylePatternDot|NSUnderlineStyleSingle]
                        forKey:NSUnderlineStyleAttributeName];
    }
    else /* "NONE" */
    {
      [attribute_dict setValue:[NSNumber numberWithInt:NSUnderlineStyleNone]
                        forKey:NSUnderlineStyleAttributeName];
    }
    did_change_attribute = true;
  }

  format = iupAttribGet(formattag, "STRIKEOUT");
  if(format)
  {
    if(iupStrBoolean(format))
    {
      [attribute_dict setValue:[NSNumber numberWithInt:YES]
                        forKey:NSStrikethroughStyleAttributeName];
    }
    else
    {
      [attribute_dict setValue:[NSNumber numberWithInt:NO]
                        forKey:NSStrikethroughStyleAttributeName];
    }
    did_change_attribute = true;
  }

  format = iupAttribGet(formattag, "WEIGHT");
  if(format)
  {
    int target_weight = 5;

    if(iupStrEqualNoCase(format, "EXTRALIGHT"))
    {
      target_weight = 0;
    }
    else if(iupStrEqualNoCase(format, "LIGHT"))
    {
      target_weight = 3;
    }
    else if(iupStrEqualNoCase(format, "SEMIBOLD"))
    {
      target_weight = 6;
    }
    else if(iupStrEqualNoCase(format, "BOLD"))
    {
      target_weight = 8;
    }
    else if (iupStrEqualNoCase(format, "EXTRABOLD"))
    {
      target_weight = 10;
    }
    else if(iupStrEqualNoCase(format, "HEAVY"))
    {
      target_weight = 11;
    }
    else /* "NORMAL" */
    {
      target_weight = 5;
    }
    font_target_weight = target_weight;

    did_change_attribute = true;
    did_change_font_weight = true;
  }

  if(iupAttribGetBoolean(formattag, "SMALLCAPS"))
  {
    did_change_attribute = true;
    did_change_smallcaps = true;
  }

  format = iupAttribGet(formattag, "FGCOLOR");
  if(format)
  {
    NSColor* the_color = cocoaTextColorFromStr(format);
    if (the_color)
      [attribute_dict setValue:the_color forKey:NSForegroundColorAttributeName];
    needs_add_font_fgcolor = (the_color != nil);
    did_change_attribute = true;
    did_change_font_fgcolor = true;
  }

  format = iupAttribGet(formattag, "BGCOLOR");
  if(format)
  {
    NSColor* the_color = cocoaTextColorFromStr(format);
    if (the_color)
      [attribute_dict setObject:the_color forKey:NSBackgroundColorAttributeName];
    needs_add_font_bgcolor = (the_color != nil);
    did_change_attribute = true;
    did_change_font_bgcolor = true;
  }

  format = iupAttribGet(formattag, "DISABLED");
  if(format)
  {
    if(iupStrBoolean(format))
    {
      NSColor* the_color = [NSColor disabledControlTextColor];
      [attribute_dict setValue:the_color
                        forKey:NSForegroundColorAttributeName];
      did_change_font_fgcolor = true;
      needs_add_font_fgcolor = true;
    }
    else if(!iupAttribGet(formattag, "FGCOLOR"))
    {
      [attribute_dict removeObjectForKey:NSForegroundColorAttributeName];
      did_change_font_fgcolor = true;
      needs_add_font_fgcolor = false;
    }
    did_change_attribute = true;
  }

  format = iupAttribGet(formattag, "LINK");
  if(format)
  {
    NSURL* link_url = [NSURL URLWithString:iupcocoaStrToNSString(format)];
    if(link_url)
      [attribute_dict setValue:link_url forKey:NSLinkAttributeName];

    if(!iupAttribGet(formattag, "FGCOLOR"))
    {
      NSColor* blue = [NSColor colorWithSRGBRed:0.0 green:0.0 blue:1.0 alpha:1.0];
      [attribute_dict setValue:blue forKey:NSForegroundColorAttributeName];
      did_change_font_fgcolor = true;
      needs_add_font_fgcolor = true;
    }

    if(!iupAttribGet(formattag, "UNDERLINE"))
    {
      [attribute_dict setValue:[NSNumber numberWithInt:NSUnderlinePatternSolid|NSUnderlineStyleSingle]
                        forKey:NSUnderlineStyleAttributeName];
    }

    did_change_attribute = true;
  }

  if(did_change_attribute)
  {
    IupCocoaFont* iup_font = iupcocoaGetFont(ih);
    IupCocoaTextFontIntent font_intent = {
      did_change_font_family, did_change_font_size, did_change_font_traits,
      did_change_font_weight, did_change_smallcaps,
      font_family_name, font_size, trait_mask, font_target_weight
    };

    NSTextStorage* text_storage = [text_view textStorage];

    if(selection_range.location < [text_storage length])
    {
      /* shouldChangeTextInRange: triggers layout; must be outside begin/endEditing. */
      ih->data->disable_callbacks = 1;
      [text_view shouldChangeTextInRange:selection_range replacementString:nil];

      [text_storage beginEditing];

      NSUInteger loc = selection_range.location;
      NSUInteger end = selection_range.length + loc;
      while(loc < end)
      {
        NSRange attrib_range;

        NSRange sub_range = {loc, end-loc};
        NSAttributedString* current_substring = [text_storage attributedSubstringFromRange:sub_range];
        NSDictionary<NSAttributedStringKey, id>* current_substring_attributes = [current_substring attributesAtIndex:0 effectiveRange:&attrib_range];

        cocoaTextApplyFontChanges(attribute_dict, current_substring_attributes, iup_font, &font_intent);

        NSRange adjusted_range = { loc, attrib_range.length };
        [text_storage addAttributes:attribute_dict range:adjusted_range];

        if(did_change_font_fgcolor && !needs_add_font_fgcolor)
        {
          [text_storage removeAttribute:NSForegroundColorAttributeName range:adjusted_range];
        }
        if(did_change_font_bgcolor && !needs_add_font_bgcolor)
        {
          [text_storage removeAttribute:NSBackgroundColorAttributeName range:adjusted_range];
        }

        loc += attrib_range.length;
      }

      ih->data->disable_callbacks = 0;
      [text_storage endEditing];
      [text_view didChangeText];

      attribute_dict = nil;
    }
    else
    {
      NSDictionary<NSAttributedStringKey, id>* current_substring_attributes = [text_view typingAttributes];

      cocoaTextApplyFontChanges(attribute_dict, current_substring_attributes, iup_font, &font_intent);
    }
  }

  if(did_change_attribute)
  {
    return attribute_dict;
  }
  else
  {
    return nil;
  }
}

static bool cocoaTextInsertImage(Ihandle* ih, NSTextView* text_view, Ihandle* formattag, NSRange range)
{
  char* image_name = iupAttribGet(formattag, "IMAGE");
  if(!image_name)
    return false;

  NSImage* ns_image = (NSImage*)iupImageGetImage(image_name, ih, 0, NULL);
  if(!ns_image)
    return false;

  int img_w, img_h;
  int new_w = 0, new_h = 0;
  char* attr;

  iupImageGetInfo(image_name, &img_w, &img_h, NULL);

  attr = iupAttribGet(formattag, "WIDTH");
  if(attr) iupStrToInt(attr, &new_w);
  attr = iupAttribGet(formattag, "HEIGHT");
  if(attr) iupStrToInt(attr, &new_h);

  NSTextAttachment* attachment = [[[NSTextAttachment alloc] init] autorelease];
#ifndef GNUSTEP
  attachment.image = ns_image;

  if((new_w > 0 && new_w != img_w) || (new_h > 0 && new_h != img_h))
  {
    if(new_w <= 0) new_w = img_w;
    if(new_h <= 0) new_h = img_h;
    attachment.bounds = CGRectMake(0, 0, new_w, new_h);
  }
#else
  (void)ns_image;
  (void)new_w;
  (void)new_h;
  (void)img_w;
  (void)img_h;
#endif

  NSMutableAttributedString* attr_str = [[[NSAttributedString attributedStringWithAttachment:attachment] mutableCopy] autorelease];
  NSTextStorage* text_storage = [text_view textStorage];

  if (range.location < [text_storage length])
  {
    NSDictionary* at = [text_storage attributesAtIndex:range.location effectiveRange:NULL];
    NSMutableDictionary* keep = [NSMutableDictionary dictionary];
    if (at[NSFontAttributeName]) keep[NSFontAttributeName] = at[NSFontAttributeName];
    if (at[NSParagraphStyleAttributeName]) keep[NSParagraphStyleAttributeName] = at[NSParagraphStyleAttributeName];
    [attr_str addAttributes:keep range:NSMakeRange(0, [attr_str length])];
  }

  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
  [undo_manager beginUndoGrouping];

  ih->data->disable_callbacks = 1;
  [text_view shouldChangeTextInRange:range replacementString:[attr_str string]];
  [text_storage beginEditing];
  [text_storage replaceCharactersInRange:range withAttributedString:attr_str];
  ih->data->disable_callbacks = 0;
  [text_storage endEditing];
  [text_view didChangeText];

  [undo_manager endUndoGrouping];

  return true;
}

IUP_SDK_API void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
  if(!ih->data->is_multiline)
  {
    return;
  }
  if(!ih->data->has_formatting)
  {
    return;
  }
  NSTextView* text_view = cocoaTextGetTextView(ih);

  if (iupAttribGet(formattag, "FONTSCALE") && !iupAttribGet(formattag, "FONTSIZE"))
    iupAttribSet(formattag, "FONTSIZE", iupGetFontSizeAttrib(ih));

  char* iup_selection = NULL;
  NSRange native_selection_range = {0, 0};
  iup_selection = iupAttribGet(formattag, "SELECTION");
  if(iup_selection)
  {
    int ret_val;
    NSUInteger lin_start=1, col_start=1, lin_end=1, col_end=1;

    ret_val = sscanf(iup_selection, "%lu,%lu:%lu,%lu", &lin_start, &col_start, &lin_end, &col_end);
    if(ret_val != 4)
    {
      return;
    }
    if(lin_start<1 || col_start<1 || lin_end<1 || col_end<1)
    {
      return;
    }

    bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &native_selection_range);
    if(!did_find_range)
    {
      return;
    }
  }
  else
  {
    char* iup_selection_pos = iupAttribGet(formattag, "SELECTIONPOS");
    if(iup_selection_pos)
    {
      int start_int = 0;
      int end_int = 0;

      if(iupStrToIntInt(iup_selection_pos, &start_int, &end_int, ':')!=2)
      {
        return;
      }
      if(start_int<0 || end_int<0)
      {
        return;
      }
      NSString* text = [[text_view textStorage] string];
      NSUInteger start = iupcocoaStrCharToUnit(text, start_int);
      NSUInteger end = iupcocoaStrCharToUnit(text, end_int);
      native_selection_range = NSMakeRange(start, end-start);
    }
    else
    {
      NSTextStorage* text_storage = [text_view textStorage];
      native_selection_range = NSMakeRange([text_storage length], 0);

      if(cocoaTextInsertImage(ih, text_view, formattag, native_selection_range))
        return;

      NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
      [undo_manager beginUndoGrouping];

      NSDictionary* typing_attributes = [text_view typingAttributes];
      NSMutableParagraphStyle* paragraph_style = [[typing_attributes objectForKey:NSParagraphStyleAttributeName] mutableCopy];
      if(nil == paragraph_style)
      {
        paragraph_style = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
      }
      [paragraph_style autorelease];
      bool needs_paragraph_style_change = cocoaTextParseParagraphAttributes(paragraph_style, formattag);

      NSMutableDictionary* new_character_attributes = cocoaTextParseCharacterFormat(ih, formattag, text_view, native_selection_range);

      if(new_character_attributes || needs_paragraph_style_change)
      {
        NSMutableDictionary* merged_attributes = [typing_attributes mutableCopy];
        [merged_attributes autorelease];

        if(new_character_attributes != nil)
        {
          [merged_attributes addEntriesFromDictionary:new_character_attributes];
        }
        if(needs_paragraph_style_change)
        {
          [merged_attributes setObject:paragraph_style forKey:NSParagraphStyleAttributeName];
        }

        [text_view setTypingAttributes:merged_attributes];
        typing_attributes = merged_attributes;
      }

      NSAttributedString* attributed_append_string = [[NSAttributedString alloc] initWithString:@"\n" attributes:typing_attributes];
      [attributed_append_string autorelease];

      NSRange change_range = NSMakeRange([text_storage length], 0);
      ih->data->disable_callbacks = 1;
      [text_view shouldChangeTextInRange:change_range replacementString:[attributed_append_string string]];
      [text_storage beginEditing];
      [text_storage appendAttributedString:attributed_append_string];
      ih->data->disable_callbacks = 0;
      [text_storage endEditing];

      native_selection_range = NSMakeRange([text_storage length]-1, 1);
      cocoaTextParseBulletNumberListFormat(ih, formattag, text_view, native_selection_range);

      change_range = NSMakeRange([text_storage length]-1, 1);
      ih->data->disable_callbacks = 1;
      [text_view shouldChangeTextInRange:change_range replacementString:@""];
      [text_storage beginEditing];
      [text_storage deleteCharactersInRange:change_range];
      ih->data->disable_callbacks = 0;
      [text_storage endEditing];

      [undo_manager endUndoGrouping];

      return;
    }
  }

  if(cocoaTextInsertImage(ih, text_view, formattag, native_selection_range))
    return;

  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
  [undo_manager beginUndoGrouping];

  /* Each parser does its own begin/endEditing, don't nest. */
  cocoaTextParseBulletNumberListFormat(ih, formattag, text_view, native_selection_range);
  cocoaTextParseParagraphFormat(ih, formattag, text_view, native_selection_range);
  cocoaTextParseCharacterFormat(ih, formattag, text_view, native_selection_range);

  [undo_manager endUndoGrouping];
}

static char* cocoaTextGetFormattingAttrib(Ihandle* ih)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
    {
      return iupStrReturnBoolean(ih->data->has_formatting);
    }
    case IUPCOCOATEXTSUBTYPE_FIELD:
    case IUPCOCOATEXTSUBTYPE_STEPPER:
    default:
    {
      break;
    }
  }

  return iupStrReturnBoolean(false);
}

static int cocoaTextSetFormattingAttrib(Ihandle* ih, const char* value)
{
  if(!ih->handle)
  {
    ih->data->has_formatting = iupStrBoolean(value);
    return 0;
  }

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        bool enable_formatting = iupStrBoolean(value);

        if(enable_formatting)
        {
          [text_view setRichText:enable_formatting];
          NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
          [undo_manager removeAllActions];
        }
        else
        {
          NSTextView* text_view = cocoaTextGetTextView(ih);
          NSRange selection_range;
          NSTextStorage* text_storage = [text_view textStorage];
          selection_range = NSMakeRange(0, [text_storage length]);

          NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];

          ih->data->disable_callbacks = 1;
          [text_view shouldChangeTextInRange:selection_range replacementString:nil];
          ih->data->disable_callbacks = 0;

          [text_storage beginEditing];
          [text_storage setAttributes:@{} range:selection_range];

          IupCocoaFont* iup_font = iupcocoaGetFont(ih);

          [text_view setTypingAttributes:[iup_font attributeDictionary]];
          [text_view setRichText:enable_formatting];

          [text_storage endEditing];
          [text_view didChangeText];
          [undo_manager removeAllActions];
        }

        ih->data->has_formatting = enable_formatting;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
  case IUPCOCOATEXTSUBTYPE_STEPPER:
                           default:
      {
        ih->data->has_formatting = 0;
        break;
      }
  }

  return 0;
}

static int cocoaTextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
  if(!ih->data->is_multiline)
  {
    return 0;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSRange selection_range;
  NSTextStorage* text_storage = [text_view textStorage];

  if(iupStrEqualNoCase(value, "ALL"))
  {
    selection_range = NSMakeRange(0, [text_storage length]);
  }
  else
  {
    selection_range = [text_view selectedRange];
    if(NSNotFound == selection_range.location)
    {
      return 0;
    }
  }

  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
  [undo_manager beginUndoGrouping];
  [text_storage beginEditing];

  ih->data->disable_callbacks = 1;
  [text_view shouldChangeTextInRange:selection_range replacementString:nil];
  [text_storage setAttributes:@{} range:selection_range];
  ih->data->disable_callbacks = 0;

  [text_storage endEditing];
  [text_view didChangeText];
  [undo_manager endUndoGrouping];

  return 0;
}

static int cocoaTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);

        if(iupStrEqualNoCase(value, "ARIGHT"))
        {
          [text_view setAlignment:NSTextAlignmentRight];
        }
        else if (iupStrEqualNoCase(value, "ACENTER"))
        {
          [text_view setAlignment:NSTextAlignmentCenter];
        }
        else /* "ALEFT" */
        {
          [text_view setAlignment:NSTextAlignmentLeft];
        }
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        if(iupStrEqualNoCase(value, "ARIGHT"))
        {
          [text_field setAlignment:NSTextAlignmentRight];
        }
        else if (iupStrEqualNoCase(value, "ACENTER"))
        {
          [text_field setAlignment:NSTextAlignmentCenter];
        }
        else /* "ALEFT" */
        {
          [text_field setAlignment:NSTextAlignmentLeft];
        }

        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        if(iupStrEqualNoCase(value, "ARIGHT"))
        {
          [text_field setAlignment:NSTextAlignmentRight];
        }
        else if (iupStrEqualNoCase(value, "ACENTER"))
        {
          [text_field setAlignment:NSTextAlignmentCenter];
        }
        else /* "ALEFT" */
        {
          [text_field setAlignment:NSTextAlignmentLeft];
        }

        break;
      }
    default:
      {
        break;
      }
  }

  return 1;
}

static char cocoa_text_field_range_key;

static NSTextView* cocoaTextFieldCurrentEditor(NSTextField* text_field)
{
  NSText* editor = [text_field currentEditor];
  return [editor isKindOfClass:[NSTextView class]] ? (NSTextView*)editor : nil;
}

static NSString* cocoaTextFieldString(NSTextField* text_field)
{
  NSTextView* editor = cocoaTextFieldCurrentEditor(text_field);
  return editor ? [editor string] : [text_field stringValue];
}

static NSRange cocoaTextFieldGetRange(NSTextField* text_field)
{
  NSTextView* editor = cocoaTextFieldCurrentEditor(text_field);
  if (editor)
    return [editor selectedRange];

  NSValue* stored = objc_getAssociatedObject(text_field, &cocoa_text_field_range_key);
  NSRange range = stored ? [stored rangeValue] : NSMakeRange(0, 0);
  NSUInteger length = [[text_field stringValue] length];
  if (range.location > length)
    range = NSMakeRange(length, 0);
  else if (NSMaxRange(range) > length)
    range.length = length - range.location;
  return range;
}

static void cocoaTextFieldSetRange(NSTextField* text_field, NSRange range)
{
  NSTextView* editor = cocoaTextFieldCurrentEditor(text_field);
  if (editor)
    [editor setSelectedRange:range];
  else
    objc_setAssociatedObject(text_field, &cocoa_text_field_range_key, [NSValue valueWithRange:range], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

IUP_DRV_API void iupcocoaTextFieldSaveRange(NSNotification* notification)
{
  NSTextField* text_field = [notification object];
  NSText* editor = [[notification userInfo] objectForKey:@"NSFieldEditor"];
  if ([text_field isKindOfClass:[NSTextField class]] && editor)
    objc_setAssociatedObject(text_field, &cocoa_text_field_range_key, [NSValue valueWithRange:[editor selectedRange]], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

IUP_DRV_API void iupcocoaTextFieldReplace(Ihandle* ih, NSTextField* text_field, NSRange range, NSString* ns_string)
{
  NSTextView* editor = cocoaTextFieldCurrentEditor(text_field);
  char* saved = iupAttribGet(ih, "_IUPCOCOA_DISABLE_TEXT_CB");

  iupAttribSet(ih, "_IUPCOCOA_DISABLE_TEXT_CB", "1");
  if (editor)
  {
    if ([editor shouldChangeTextInRange:range replacementString:ns_string])
    {
      [editor replaceCharactersInRange:range withString:ns_string];
      [editor setSelectedRange:NSMakeRange(range.location + [ns_string length], 0)];
    }
    [editor didChangeText];
  }
  else
  {
    [text_field setStringValue:[[text_field stringValue] stringByReplacingCharactersInRange:range withString:ns_string]];
    cocoaTextFieldSetRange(text_field, NSMakeRange(range.location + [ns_string length], 0));
  }
  iupAttribSet(ih, "_IUPCOCOA_DISABLE_TEXT_CB", saved);
}

IUP_DRV_API char* iupcocoaTextFieldGetCaret(NSTextField* text_field, int base)
{
  return iupStrReturnInt(iupcocoaStrUnitToChar(cocoaTextFieldString(text_field), cocoaTextFieldGetRange(text_field).location) + base);
}

IUP_DRV_API void iupcocoaTextFieldSetCaret(NSTextField* text_field, const char* value, int base)
{
  int pos = base;
  iupStrToInt(value, &pos);
  pos -= base;
  if (pos < 0)
    pos = 0;
  cocoaTextFieldSetRange(text_field, NSMakeRange(iupcocoaStrCharToUnit(cocoaTextFieldString(text_field), pos), 0));
}

IUP_DRV_API char* iupcocoaTextFieldGetSelection(NSTextField* text_field, int base)
{
  NSRange range = cocoaTextFieldGetRange(text_field);
  NSString* text = cocoaTextFieldString(text_field);
  if (range.length == 0)
    return NULL;
  return iupStrReturnIntInt(iupcocoaStrUnitToChar(text, range.location) + base, iupcocoaStrUnitToChar(text, NSMaxRange(range)) + base, ':');
}

IUP_DRV_API void iupcocoaTextFieldSetSelection(NSTextField* text_field, const char* value, int base)
{
  NSString* text = cocoaTextFieldString(text_field);
  int start, end;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    cocoaTextFieldSetRange(text_field, NSMakeRange(cocoaTextFieldGetRange(text_field).location, 0));
    return;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    cocoaTextFieldSetRange(text_field, NSMakeRange(0, [text length]));
    return;
  }

  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return;
  start -= base;
  end -= base;
  if (start < 0 || end < 0)
    return;

  NSUInteger start_unit = iupcocoaStrCharToUnit(text, start);
  NSUInteger end_unit = iupcocoaStrCharToUnit(text, end);
  if (end_unit < start_unit)
    end_unit = start_unit;
  cocoaTextFieldSetRange(text_field, NSMakeRange(start_unit, end_unit - start_unit));
}

IUP_DRV_API char* iupcocoaTextFieldGetSelectedText(NSTextField* text_field)
{
  NSRange range = cocoaTextFieldGetRange(text_field);
  if (range.length == 0)
    return NULL;
  return iupStrReturnStr([[cocoaTextFieldString(text_field) substringWithRange:range] UTF8String]);
}

IUP_DRV_API void iupcocoaTextFieldSetSelectedText(Ihandle* ih, NSTextField* text_field, const char* value)
{
  NSRange range = cocoaTextFieldGetRange(text_field);
  if (value && range.length > 0)
    iupcocoaTextFieldReplace(ih, text_field, range, iupcocoaStrToNSString(value));
}

IUP_DRV_API void iupcocoaTextFieldInsert(Ihandle* ih, NSTextField* text_field, const char* value)
{
  if (value)
    iupcocoaTextFieldReplace(ih, text_field, cocoaTextFieldGetRange(text_field), iupcocoaStrToNSString(value));
}

IUP_DRV_API void iupcocoaTextFieldScrollTo(NSTextField* text_field, const char* value, int base)
{
  NSTextView* editor = cocoaTextFieldCurrentEditor(text_field);
  int pos = base;
  if (!editor)
    return;
  iupStrToInt(value, &pos);
  pos -= base;
  if (pos < 0)
    pos = 0;
  [editor scrollRangeToVisible:NSMakeRange(iupcocoaStrCharToUnit([editor string], pos), 0)];
}

static NSTextField* cocoaTextGetSingleLineField(Ihandle* ih)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  if (sub_type == IUPCOCOATEXTSUBTYPE_FIELD)
    return cocoaTextGetTextField(ih);
  if (sub_type == IUPCOCOATEXTSUBTYPE_STEPPER)
    return cocoaTextGetStepperTextField(ih);
  return nil;
}

static char* cocoaTextGetSelectedTextAttrib(Ihandle* ih)
{
  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
    return iupcocoaTextFieldGetSelectedText(single_field);

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSRange selected_range = [text_view selectedRange];
  NSString* selected_string = [[[text_view textStorage] string] substringWithRange:selected_range];
  return iupStrReturnStr([selected_string UTF8String]);
}

static int cocoaTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if(NULL == value)
  {
    return 0;
  }

  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
  {
    iupcocoaTextFieldSetSelectedText(ih, single_field, value);
    return 0;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSRange selected_range = [text_view selectedRange];
  if((NSNotFound == selected_range.location) || (0 == selected_range.length))
  {
    return 0;
  }

  NSTextStorage* text_storage = [text_view textStorage];
  IupCocoaFont* iup_font = iupcocoaGetFont(ih);

  NSMutableDictionary* attribute_dict = [[iup_font attributeDictionary] mutableCopy];
  [attribute_dict autorelease];
  NSDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[text_storage attributedSubstringFromRange:NSMakeRange(selected_range.location, 1)] attributesAtIndex:0 effectiveRange:NULL];
  [attribute_dict addEntriesFromDictionary:text_storage_attributes];

  NSString* ns_insert_string = iupcocoaStrToNSString(value);

  NSAttributedString* attributed_insert_string = [[NSAttributedString alloc] initWithString:ns_insert_string attributes:attribute_dict];
  [attributed_insert_string autorelease];

  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
  [undo_manager beginUndoGrouping];

  ih->data->disable_callbacks = 1;
  [text_view shouldChangeTextInRange:selected_range replacementString:[attributed_insert_string string]];
  [text_storage beginEditing];

  [text_storage replaceCharactersInRange:selected_range withAttributedString:attributed_insert_string];

  ih->data->disable_callbacks = 0;

  [text_storage endEditing];

  [text_view didChangeText];
  [undo_manager endUndoGrouping];

  return 0;
}

static int cocoaTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
  {
    iupcocoaTextFieldSetSelection(single_field, value, 1);
    return 0;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);

  NSRange selection_range = NSMakeRange(0, 0);

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
  }
  else if (iupStrEqualNoCase(value, "ALL"))
  {
    NSTextStorage* text_storage = [text_view textStorage];
    NSUInteger start = 0;
    NSUInteger end = [text_storage length];
    selection_range = NSMakeRange(start, end);
  }
  else
  {
    NSUInteger lin_start=1;
    NSUInteger col_start=1;
    NSUInteger lin_end=1;
    NSUInteger col_end=1;

    if(sscanf(value, "%lu,%lu:%lu,%lu", &lin_start, &col_start, &lin_end, &col_end) != 4)
    {
      return 0;
    }
    if(lin_start<1 || col_start<1 || lin_end<1 || col_end<1)
    {
      return 0;
    }

    bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &selection_range);
    if(!did_find_range)
    {
      return 0;
    }
  }

  [text_view setSelectedRange:selection_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];

  return 0;
}

static char* cocoaTextGetSelectionAttrib(Ihandle* ih)
{
  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
    return iupcocoaTextFieldGetSelection(single_field, 1);

  NSTextView* text_view = cocoaTextGetTextView(ih);

  NSRange selected_range = [text_view selectedRange];
  if(NSNotFound == selected_range.location)
  {
    return NULL;
  }

  NSUInteger lin_start=1;
  NSUInteger col_start=1;
  NSUInteger lin_end=1;
  NSUInteger col_end=1;
  bool did_find_range = cocoaTextComputeLineColumnFromRangeForTextView(text_view, selected_range, &lin_start, &col_start, &lin_end, &col_end);
  if(!did_find_range)
  {
    return NULL;
  }

  return iupStrReturnStrf("%lu,%lu:%lu,%lu", lin_start, col_start, lin_end, col_end);
}

static int cocoaTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
  {
    iupcocoaTextFieldSetSelection(single_field, value, 0);
    return 0;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);

  NSUInteger start;
  NSUInteger end;
  NSRange selection_range = NSMakeRange(0, 0);

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
  }
  else if (iupStrEqualNoCase(value, "ALL"))
  {
    NSTextStorage* text_storage = [text_view textStorage];

    start = 0;
    end = [text_storage length];
    selection_range = NSMakeRange(start, end);
  }
  else
  {
    int start_int = 0;
    int end_int = 0;

    if(iupStrToIntInt(value, &start_int, &end_int, ':')!=2)
    {
      return 0;
    }
    if(start_int<0 || end_int<0)
    {
      return 0;
    }
    NSString* text = [[text_view textStorage] string];
    start = iupcocoaStrCharToUnit(text, start_int);
    end = iupcocoaStrCharToUnit(text, end_int);
    selection_range = NSMakeRange(start, end-start);
  }

  [text_view setSelectedRange:selection_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];

  return 0;
}

static char* cocoaTextGetSelectionPosAttrib(Ihandle* ih)
{
  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
    return iupcocoaTextFieldGetSelection(single_field, 0);

  NSTextView* text_view = cocoaTextGetTextView(ih);

  NSRange selected_range = [text_view selectedRange];
  if(NSNotFound == selected_range.location)
  {
    return NULL;
  }
  NSString* text = [[text_view textStorage] string];
  int start = iupcocoaStrUnitToChar(text, selected_range.location);
  int end = iupcocoaStrUnitToChar(text, selected_range.location + selected_range.length);

  return iupStrReturnIntInt(start, end, ':');
}

static int cocoaTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
  {
    iupcocoaTextFieldSetCaret(single_field, value, 1);
    return 0;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSRange cursor_range = NSMakeRange(0, 0);
  int lin_int = 1, col_int = 1;
  iupStrToIntInt(value, &lin_int, &col_int, ',');
  if (lin_int < 1) lin_int = 1;
  if (col_int < 1) col_int = 1;
  NSUInteger lin_start = (NSUInteger)lin_int;
  NSUInteger col_start = (NSUInteger)col_int;
  if (cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_start, col_start, &cursor_range))
  {
    [text_view setSelectedRange:cursor_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];
    [text_view scrollRangeToVisible:cursor_range];
  }

  return 0;
}

static char* cocoaTextGetCaretAttrib(Ihandle* ih)
{
  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
    return iupcocoaTextFieldGetCaret(single_field, 1);

  NSTextView* text_view = cocoaTextGetTextView(ih);

  NSRange cursor_range = [[[text_view selectedRanges] lastObject] rangeValue];
  if(NSNotFound == cursor_range.location)
  {
    return NULL;
  }

  NSUInteger lin_start=1;
  NSUInteger col_start=1;
  NSUInteger lin_end=1;
  NSUInteger col_end=1;
  bool did_find_range = cocoaTextComputeLineColumnFromRangeForTextView(text_view, cursor_range, &lin_start, &col_start, &lin_end, &col_end);
  if(!did_find_range)
  {
    return NULL;
  }

  [text_view scrollRangeToVisible:cursor_range];

  return iupStrReturnIntInt((int)lin_start, (int)col_start, ',');
}

static int cocoaTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
  {
    iupcocoaTextFieldSetCaret(single_field, value, 1);
    iupcocoaTextFieldScrollTo(single_field, value, 1);
    return 0;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSRange cursor_range = NSMakeRange(0, 0);
  int lin = 1, col = 1;
  iupStrToIntInt(value, &lin, &col, ',');
  if (lin < 1) lin = 1;
  if (col < 1) col = 1;

  if (cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin, col, lin, col, &cursor_range))
    [text_view scrollRangeToVisible:cursor_range];

  return 0;
}

static int cocoaTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
  {
    iupcocoaTextFieldSetCaret(single_field, value, 0);
    iupcocoaTextFieldScrollTo(single_field, value, 0);
    return 0;
  }

  int pos = 0;
  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  NSTextView* text_view = cocoaTextGetTextView(ih);

  NSRange cursor_range = NSMakeRange(iupcocoaStrCharToUnit([[text_view textStorage] string], pos), 0);
  [text_view scrollRangeToVisible:cursor_range];

  return 0;
}

static int cocoaTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
  {
    iupcocoaTextFieldSetCaret(single_field, value, 0);
    return 0;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);

  NSRange cursor_range = NSMakeRange(0, 0);

  int pos = 0;
  if(!iupStrToInt(value, &pos))
  {
    return 0;
  }
  if(pos<0)
  {
    return 0;
  }
  cursor_range = NSMakeRange(iupcocoaStrCharToUnit([[text_view textStorage] string], pos), 0);

  [text_view setSelectedRange:cursor_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];
  [text_view scrollRangeToVisible:cursor_range];
  return 0;
}

static char* cocoaTextGetCaretPosAttrib(Ihandle* ih)
{
  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
    return iupcocoaTextFieldGetCaret(single_field, 0);

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSRange cursor_range = [[[text_view selectedRanges] lastObject] rangeValue];
  if(NSNotFound == cursor_range.location)
  {
    return NULL;
  }
  [text_view scrollRangeToVisible:cursor_range];
  return iupStrReturnInt(iupcocoaStrUnitToChar([[text_view textStorage] string], cursor_range.location));
}

static char* cocoaTextGetLineValueAttrib(Ihandle* ih)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);

        NSRange cursor_range = [[[text_view selectedRanges] lastObject] rangeValue];
        if(NSNotFound == cursor_range.location)
        {
          return NULL;
        }

        cursor_range.length = 0;

        NSTextStorage* text_storage = [text_view textStorage];
        NSString* text_string = [text_storage string];
        NSUInteger start_index = 0;
        NSUInteger contents_end_index = 0;

        [text_string getLineStart:&start_index end:NULL contentsEnd:&contents_end_index forRange:cursor_range];

        NSRange line_range = NSMakeRange(start_index, contents_end_index - start_index);
        NSString* selected_string = [text_string substringWithRange:line_range];
        return iupStrReturnStr([selected_string UTF8String]);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
  case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        return cocoaTextGetValueAttrib(ih);
        break;
      }
  default:
      {
        return NULL;
        break;
      }
  }
}

static int cocoaTextSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  NSString* ns_string;

  if(NULL == value)
  {
    ns_string = @"";
  }
  else
  {
    ns_string = iupcocoaStrToNSString(value);
  }

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        [text_field setPlaceholderString:ns_string];
        return 1;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        [text_field setPlaceholderString:ns_string];
        break;
      }
    default:
      {
        break;
      }
  }

  return 0;
}

static int cocoaTextSetActiveAttrib(Ihandle* ih, const char* value)
{
  BOOL is_active = (BOOL)iupStrBoolean(value);
  /* setSelectable:NO also clears editable, so restore it on re-activation */
  BOOL is_editable = is_active && !iupAttribGetBoolean(ih, "READONLY");

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        [text_view setSelectable:is_active];
        if (is_active)
          [text_view setEditable:is_editable];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        [text_field setSelectable:is_active];
        [text_field setEnabled:is_active];
        if (is_active)
          [text_field setEditable:is_editable];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        [text_field setSelectable:is_active];
        [text_field setEnabled:is_active];
        if (is_active)
          [text_field setEditable:is_editable];

        NSStepper* stepper_view = cocoaTextGetStepperView(ih);
        [stepper_view setEnabled:is_active];
        break;
      }
    default:
      {
        break;
      }
  }

  return 0;
}

static char* cocoaTextGetActiveAttrib(Ihandle* ih)
{
  int is_active = true;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        is_active = [text_view isSelectable];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        is_active = [text_field isEnabled];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSStepper* stepper_view = cocoaTextGetStepperView(ih);
        is_active = [text_field isEnabled] && [stepper_view isEnabled];
        break;
      }
    default:
      {
        break;
      }
  }

  return iupStrReturnBoolean(is_active);
}

static int cocoaTextSetAutoCorrectAttrib(Ihandle* ih, const char* value)
{
  BOOL enable = iupStrBoolean(value) ? YES : NO;
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);

  if (sub_type == IUPCOCOATEXTSUBTYPE_VIEW)
  {
    cocoaTextSetCorrections(cocoaTextGetTextView(ih), enable);
    return 1;
  }
  if (sub_type != IUPCOCOATEXTSUBTYPE_FIELD)
    return 1;

  NSTextField* text_field = cocoaTextGetTextField(ih);
  if (![[text_field cell] isKindOfClass:[IupCocoaTextFieldCell class]])
    return 1;

  [(IupCocoaTextFieldCell*)[text_field cell] setIupAutoCorrect:enable];
  NSText* editor = [text_field currentEditor];
  if ([editor isKindOfClass:[NSTextView class]])
    cocoaTextSetCorrections((NSTextView*)editor, enable);
  return 1;
}

static int cocoaTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  BOOL is_editable = !(BOOL)iupStrBoolean(value);

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        [text_view setEditable:is_editable];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        [text_field setEditable:is_editable];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        [text_field setEditable:is_editable];
        break;
      }
    default:
      {
        break;
      }
  }

  return 0;
}

static char* cocoaTextGetReadOnlyAttrib(Ihandle* ih)
{
  int is_editable = true;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        is_editable = [text_view isEditable];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        is_editable = [text_field isEditable];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        is_editable = [text_field isEditable];
        break;
      }
    default:
      {
        break;
      }
  }

  return iupStrReturnBoolean(!is_editable);
}

static int cocoaTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
  {
    return 0;
  }

  if(value == NULL)
  {
    return 0;
  }

  ih->data->disable_callbacks = 1;
  if(ih->data->is_multiline)
  {
    NSTextView* text_view = cocoaTextGetTextView(ih);
    NSTextStorage* text_storage = [text_view textStorage];

    NSString* ns_append_string = nil;
    IupCocoaFont* iup_font = iupcocoaGetFont(ih);

    if(ih->data->append_newline && ([text_storage length] > 0))
    {
      NSString* temp = iupcocoaStrToNSString(value);
      ns_append_string = [@"\n" stringByAppendingString:temp];
    }
    else
    {
      ns_append_string = iupcocoaStrToNSString(value);
    }

    NSRange change_range = NSMakeRange([text_storage length], 0);

    NSMutableDictionary* attribute_dict = [[iup_font attributeDictionary] mutableCopy];
    NSColor* default_color = [(IupCocoaTextView*)text_view defaultColor];
    [attribute_dict autorelease];
    [attribute_dict setObject:(default_color ? default_color : [NSColor textColor]) forKey:NSForegroundColorAttributeName];

    NSAttributedString* attributed_append_string = [[NSAttributedString alloc] initWithString:ns_append_string attributes:attribute_dict];
    [attributed_append_string autorelease];

    NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
    [undo_manager beginUndoGrouping];

    [text_view shouldChangeTextInRange:change_range replacementString:[attributed_append_string string]];
    [text_storage beginEditing];

    [text_storage appendAttributedString:attributed_append_string];

    ih->data->disable_callbacks = 0;

    [text_storage endEditing];
    [text_view didChangeText];
    [undo_manager endUndoGrouping];

    if (ih->data->append_scroll)
    {
      [NSObject cancelPreviousPerformRequestsWithTarget:text_view selector:@selector(scrollToEnd) object:nil];
      [text_view performSelector:@selector(scrollToEnd) withObject:nil afterDelay:0];
    }
  }
  else
  {
    IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
    switch(sub_type)
    {
      case IUPCOCOATEXTSUBTYPE_FIELD:
        {
          NSTextField* text_field = cocoaTextGetTextField(ih);
          IupCocoaFont* iup_font = iupcocoaGetFont(ih);
          NSMutableAttributedString* old_string_value = [[[text_field attributedStringValue] mutableCopy] autorelease];

          NSString* ns_append_string = iupcocoaStrToNSString(value);

          NSAttributedString* attributed_append_string = [[NSAttributedString alloc] initWithString:ns_append_string attributes:[iup_font attributeDictionary]];
          [attributed_append_string autorelease];

          [old_string_value appendAttributedString:attributed_append_string];
          [text_field setAttributedStringValue:old_string_value];

          break;
        }
      default:
        {
          break;
        }
    }
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static int cocoaTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if(NULL == ih->handle)
  {
    return 0;
  }
  if(NULL == value)
  {
    return 0;
  }

  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
  {
    iupcocoaTextFieldInsert(ih, single_field, value);
    return 0;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSTextStorage* text_storage = [text_view textStorage];

  NSRange insertion_point = [[[text_view selectedRanges] lastObject] rangeValue];
  if(NSNotFound == insertion_point.location)
  {
    int saved_newline = ih->data->append_newline;
    ih->data->append_newline = 0;
    int ret_val = cocoaTextSetAppendAttrib(ih, value);
    ih->data->append_newline = saved_newline;
    return ret_val;
  }
  else if([text_storage length] == insertion_point.location)
  {
    int saved_newline = ih->data->append_newline;
    ih->data->append_newline = 0;
    int ret_val = cocoaTextSetAppendAttrib(ih, value);
    ih->data->append_newline = saved_newline;
    return ret_val;
  }
  else if(insertion_point.length > 0)
  {
    return cocoaTextSetSelectedTextAttrib(ih, value);
  }

  IupCocoaFont* iup_font = iupcocoaGetFont(ih);

  NSMutableDictionary* attribute_dict = [[iup_font attributeDictionary] mutableCopy];
  [attribute_dict autorelease];
  NSDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[text_storage attributedSubstringFromRange:NSMakeRange(insertion_point.location, 1)] attributesAtIndex:0 effectiveRange:NULL];
  [attribute_dict addEntriesFromDictionary:text_storage_attributes];

  NSString* ns_insert_string = iupcocoaStrToNSString(value);

  NSAttributedString* attributed_insert_string = [[NSAttributedString alloc] initWithString:ns_insert_string attributes:attribute_dict];
  [attributed_insert_string autorelease];

  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
  [undo_manager beginUndoGrouping];

  ih->data->disable_callbacks = 1;
  [text_view shouldChangeTextInRange:insertion_point replacementString:[attributed_insert_string string]];
  [text_storage beginEditing];

  [text_storage insertAttributedString:attributed_insert_string atIndex:insertion_point.location];

  ih->data->disable_callbacks = 0;

  [text_storage endEditing];

  [text_view didChangeText];
  [undo_manager endUndoGrouping];

  return 0;
}

static void cocoaTextClipboardNoEditor(Ihandle* ih, NSTextField* text_field, const char* value)
{
  NSPasteboard* paste_board = [NSPasteboard generalPasteboard];

  if(iupStrEqualNoCase(value, "COPY") || iupStrEqualNoCase(value, "CUT"))
  {
    [paste_board clearContents];
    [paste_board setString:[text_field stringValue] forType:NSPasteboardTypeString];

    if(iupStrEqualNoCase(value, "CUT"))
      cocoaTextSetValueAttrib(ih, "");
  }
  else if(iupStrEqualNoCase(value, "PASTE"))
  {
    if([paste_board availableTypeFromArray:@[NSPasteboardTypeString]])
    {
      NSString* pasteboard_string = [paste_board stringForType:NSPasteboardTypeString];
      if(pasteboard_string)
        cocoaTextSetValueAttrib(ih, [pasteboard_string UTF8String]);
    }
  }
  else if(iupStrEqualNoCase(value, "CLEAR"))
  {
    [paste_board clearContents];
  }
}

static int cocoaTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  ih->data->disable_callbacks = 1;

  NSView* the_view = nil;
  NSTextField* text_field = nil;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        the_view = text_view;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        text_field = cocoaTextGetTextField(ih);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        text_field = cocoaTextGetStepperTextField(ih);
        break;
      }
    default:
      {
        break;
      }
  }

  /* NSTextField has no paste:, its field editor edits it; with no editor the whole value is used */
  if(text_field)
  {
    the_view = [text_field currentEditor];

    if(!the_view)
    {
      cocoaTextClipboardNoEditor(ih, text_field, value);
      ih->data->disable_callbacks = 0;
      return 0;
    }
  }

  if(iupStrEqualNoCase(value, "COPY"))
  {
    [[NSApplication sharedApplication] sendAction:@selector(copy:) to:the_view from:nil];
  }
  else if(iupStrEqualNoCase(value, "CUT"))
  {
    [[NSApplication sharedApplication] sendAction:@selector(cut:) to:the_view from:nil];
  }
  else if(iupStrEqualNoCase(value, "PASTE"))
  {
    [[NSApplication sharedApplication] sendAction:@selector(paste:) to:the_view from:nil];
  }
  else if(iupStrEqualNoCase(value, "CLEAR"))
  {
    [[NSPasteboard generalPasteboard] clearContents];
  }
  else if(iupStrEqualNoCase(value, "UNDO"))
  {
    switch(sub_type)
    {
      case IUPCOCOATEXTSUBTYPE_VIEW:
        {
          NSTextView* text_view = (NSTextView*)the_view;
          NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
          [undo_manager undo];
          break;
        }
      case IUPCOCOATEXTSUBTYPE_FIELD:
    case IUPCOCOATEXTSUBTYPE_STEPPER:
        {
          /* the field editor keeps the history of the field being edited */
          NSUndoManager* undo_manager = [the_view undoManager];
          [undo_manager undo];
          break;
        }
    default:
        {
          break;
        }
    }
  }
  else if(iupStrEqualNoCase(value, "REDO"))
  {
    switch(sub_type)
    {
      case IUPCOCOATEXTSUBTYPE_VIEW:
        {
          NSTextView* text_view = (NSTextView*)the_view;
          NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
          [undo_manager redo];
          break;
        }
      case IUPCOCOATEXTSUBTYPE_FIELD:
    case IUPCOCOATEXTSUBTYPE_STEPPER:
        {
          NSUndoManager* undo_manager = [the_view undoManager];
          [undo_manager redo];
          break;
        }
    default:
        {
          break;
        }
    }
  }
  else if(iupStrEqualNoCase(value, "CLEARUNDO"))
  {
    switch(sub_type)
    {
      case IUPCOCOATEXTSUBTYPE_VIEW:
        {
          NSTextView* text_view = (NSTextView*)the_view;
          NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
          [undo_manager removeAllActions];
          break;
        }
      case IUPCOCOATEXTSUBTYPE_FIELD:
    case IUPCOCOATEXTSUBTYPE_STEPPER:
        {
          NSUndoManager* undo_manager = [the_view undoManager];
          [undo_manager removeAllActions];
          break;
        }
    default:
        {
          break;
        }
    }
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static char* cocoaTextGetCountAttrib(Ihandle* ih)
{
  NSString* text_string = nil;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        NSTextStorage* text_storage = [text_view textStorage];
        text_string = [text_storage string];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        text_string = [text_field stringValue];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        text_string = [text_field stringValue];
        break;
      }
    default:
      {
        break;
      }
  }

  if((nil == text_string) || ([text_string length] == 0))
  {
    return iupStrReturnInt(0);
  }

  return iupStrReturnInt(iupcocoaStrUnitToChar(text_string, [text_string length]));
}

static char* cocoaTextGetScrollVisibleAttrib(Ihandle* ih)
{
  if(!ih->data->is_multiline)
    return NULL;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  if(sub_type == IUPCOCOATEXTSUBTYPE_VIEW)
  {
    NSScrollView* scroll_view = (NSScrollView*)cocoaTextGetRootView(ih);
    if([scroll_view isKindOfClass:[NSScrollView class]])
    {
      BOOL has_horizontal = [scroll_view hasHorizontalScroller];
      BOOL has_vertical = [scroll_view hasVerticalScroller];

      if(has_horizontal && has_vertical)
        return "YES";
      else if(has_vertical)
        return "VERTICAL";
      else if(has_horizontal)
        return "HORIZONTAL";
      else
        return "NO";
    }
  }

  return "NO";
}

static int cocoaTextSetOverwriteAttrib(Ihandle* ih, const char* value)
{
  if(!ih->data->is_multiline)
    return 0;

  NSTextView* text_view = cocoaTextGetTextView(ih);
  if(nil == text_view)
    return 0;

  if(iupStrBoolean(value))
    objc_setAssociatedObject(text_view, IUP_COCOA_TEXT_OVERWRITE_KEY, @(YES), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  else
    objc_setAssociatedObject(text_view, IUP_COCOA_TEXT_OVERWRITE_KEY, @(NO), OBJC_ASSOCIATION_RETAIN_NONATOMIC);

  return 1;
}

static char* cocoaTextGetOverwriteAttrib(Ihandle* ih)
{
  if(!ih->data->is_multiline)
    return NULL;

  NSTextView* text_view = cocoaTextGetTextView(ih);
  if(nil == text_view)
    return NULL;

  NSNumber* overwrite = objc_getAssociatedObject(text_view, IUP_COCOA_TEXT_OVERWRITE_KEY);
  return iupStrReturnBoolean([overwrite boolValue]);
}

static char* cocoaTextGetLineCountAttrib(Ihandle* ih)
{
  if(!ih->data->is_multiline)
  {
    return "1";
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSString* text = [[text_view textStorage] string];
  NSUInteger line_start = 0;

  return iupStrReturnInt((int)(1 + cocoaTextCountLines(text, 0, [text length], &line_start)));
}

/************************************** Begin SPIN *****************************************************/

static int cocoaTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        int int_value = 0;
        if(iupStrToInt(value, &int_value))
        {
          ih->data->disable_callbacks = 1;

          NSStepper* stepper_view = cocoaTextGetStepperView(ih);
          double max_value = [stepper_view maxValue];
          if((double)int_value > max_value)
            int_value = (int)max_value;
          double min_value = [stepper_view minValue];
          if((double)int_value < min_value)
            int_value = (int)min_value;

          [stepper_view setIntValue:int_value];

          if(iupAttribGetBoolean(ih, "SPINAUTO"))
          {
            NSTextField* text_field = cocoaTextGetStepperTextField(ih);
            NSString* ns_string = [NSString stringWithFormat:@"%d", int_value];
            [text_field setStringValue:ns_string];
          }

          ih->data->disable_callbacks = 0;
        }

        break;
      }
    default:
      {
        break;
      }
  }

  return 1;
}

static char* cocoaTextGetSpinValueAttrib(Ihandle* ih)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSStepper* stepper_view = cocoaTextGetStepperView(ih);
        return iupStrReturnInt([stepper_view intValue]);
      }
    default:
      {
        break;
      }
  }

  return NULL;
}

static int cocoaTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        int new_min;
        if(iupStrToInt(value, &new_min))
        {
          ih->data->disable_callbacks = 1;

          NSStepper* stepper_view = cocoaTextGetStepperView(ih);
          [stepper_view setMinValue:(double)new_min];

          ih->data->disable_callbacks = 0;
        }

        break;
      }
    default:
      {
        break;
      }
  }

  return 1;
}

static int cocoaTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        int new_max;
        if(iupStrToInt(value, &new_max))
        {
          ih->data->disable_callbacks = 1;

          NSStepper* stepper_view = cocoaTextGetStepperView(ih);
          [stepper_view setMaxValue:(double)new_max];

          ih->data->disable_callbacks = 0;
        }

        break;
      }
    default:
      {
        break;
      }
  }

  return 1;
}

static int cocoaTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        int new_inc;
        if(iupStrToInt(value, &new_inc))
        {
          ih->data->disable_callbacks = 1;

          NSStepper* stepper_view = cocoaTextGetStepperView(ih);
          [stepper_view setIncrement:(double)new_inc];

          ih->data->disable_callbacks = 0;
        }

        break;
      }
    default:
      {
        break;
      }
  }

  return 1;
}

/************************************** End SPIN *****************************************************/

static int cocoaTextSetContextMenuAttrib(Ihandle* ih, const char* value)
{
  Ihandle* menu_ih = (Ihandle*)value;
  id widget_to_attach = nil;

  if (!ih->handle)
  {
    iupcocoaCommonBaseSetContextMenuForWidget(ih, nil, menu_ih);
    return 1;
  }

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch (sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      widget_to_attach = cocoaTextGetTextView(ih);
      break;
    case IUPCOCOATEXTSUBTYPE_FIELD:
      widget_to_attach = cocoaTextGetTextField(ih);
      break;
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      widget_to_attach = cocoaTextGetStepperTextField(ih);
      break;
    default:
      return 0;
  }

  iupcocoaCommonBaseSetContextMenuForWidget(ih, widget_to_attach, menu_ih);

  return 1;
}

static void cocoaTextApplyPadding(Ihandle* ih);

static int cocoaTextLayoutIndexAtPoint(NSLayoutManager* layout_manager, NSTextContainer* text_container, NSPoint point, NSString* text)
{
  CGFloat fraction = 0;
  NSUInteger glyph_index = [layout_manager glyphIndexForPoint:point inTextContainer:text_container fractionOfDistanceThroughGlyph:&fraction];
  NSUInteger char_index = [layout_manager characterIndexForGlyphAtIndex:glyph_index];
  if (fraction > 0.5 && char_index < [text length])
  {
    unichar c = [text characterAtIndex:char_index];
    if (c != '\n' && c != '\r')
      char_index++;
  }
  return (int)char_index;
}

static int cocoaTextViewIndexAtPoint(NSTextView* text_view, NSPoint point)
{
  NSPoint origin = [text_view textContainerOrigin];
  point.x -= origin.x;
  point.y -= origin.y;
  return iupcocoaStrUnitToChar([text_view string], cocoaTextLayoutIndexAtPoint([text_view layoutManager], [text_view textContainer], point, [text_view string]));
}

static int cocoaTextConvertXYToPos(Ihandle* ih, int x, int y)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  if (sub_type == IUPCOCOATEXTSUBTYPE_VIEW)
    return cocoaTextViewIndexAtPoint(cocoaTextGetTextView(ih), NSMakePoint(x, y));

  NSTextField* text_field = (sub_type == IUPCOCOATEXTSUBTYPE_STEPPER) ? cocoaTextGetStepperTextField(ih) : cocoaTextGetTextField(ih);
  NSPoint point = NSMakePoint(x, y);
  if (![text_field isFlipped])
    point.y = [text_field bounds].size.height - y;

  NSText* editor = [text_field currentEditor];
  if ([editor isKindOfClass:[NSTextView class]])
    return cocoaTextViewIndexAtPoint((NSTextView*)editor, [editor convertPoint:point fromView:text_field]);

  NSAttributedString* value = [text_field attributedStringValue];
  NSRect title_rect = [[text_field cell] titleRectForBounds:[text_field bounds]];
  NSTextStorage* text_storage = [[NSTextStorage alloc] initWithAttributedString:value];
  NSLayoutManager* layout_manager = [[NSLayoutManager alloc] init];
  NSTextContainer* text_container = [[NSTextContainer alloc] initWithContainerSize:NSMakeSize(1.0e7, 1.0e7)];
  NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
  if ([field_editor isKindOfClass:[NSTextView class]])
    [text_container setLineFragmentPadding:[[(NSTextView*)field_editor textContainer] lineFragmentPadding]];
  [layout_manager addTextContainer:text_container];
  [text_storage addLayoutManager:layout_manager];

  NSPoint local = NSMakePoint(point.x - NSMinX(title_rect), [text_field isFlipped] ? point.y - NSMinY(title_rect) : NSMaxY(title_rect) - point.y);
  int pos = iupcocoaStrUnitToChar([value string], cocoaTextLayoutIndexAtPoint(layout_manager, text_container, local, [value string]));

  [text_container release];
  [layout_manager release];
  [text_storage release];
  return pos;
}

static int cocoaTextMapMethod(Ihandle* ih)
{
  NSView* root_view = nil;
  NSView* main_view = nil;

  if (ih->data->is_multiline)
  {
    NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 200, 100)];
    NSSize scrollview_content_size = [scroll_view contentSize];
    NSTextView* text_view;

    if (iupAttribGetBoolean(ih, "BORDER"))
      [scroll_view setBorderType:NSBezelBorder];
    else
      [scroll_view setBorderType:NSNoBorder];

    [scroll_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

    text_view = [[IupCocoaTextView alloc] initWithFrame:NSMakeRect(0, 0,
        scrollview_content_size.width, scrollview_content_size.height)];

    [text_view setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
    [text_view setVerticallyResizable:YES];
    [text_view setAutoresizingMask:NSViewWidthSizable];

    [text_view setEditable:!iupAttribGetBoolean(ih, "READONLY")];
    [text_view setSelectable:YES];
    cocoaTextSetCorrections(text_view, NO);
    [text_view setRichText:ih->data->has_formatting];
    [text_view setImportsGraphics:NO];
    [text_view setUsesFindPanel:YES];
    [text_view setAllowsUndo:YES];
    [text_view setUsesRuler:NO];

    /* the inset applies to each side */
    [text_view setTextContainerInset:NSMakeSize(IUPCOCOA_TEXTVIEW_INSET_W, IUPCOCOA_TEXTVIEW_INSET_H)];

    NSTextContainer* text_container = [text_view textContainer];

    [scroll_view setDocumentView:text_view];

    if (iupAttribGetBoolean(ih, "WORDWRAP") || ih->data->has_formatting)
    {
      ih->data->sb &= ~IUP_SB_HORIZ;

      [text_view setHorizontallyResizable:NO];
      [text_container setContainerSize:NSMakeSize(scrollview_content_size.width, FLT_MAX)];
      [text_container setWidthTracksTextView:YES];
      [text_container setLineFragmentPadding:0.0];
    }
    else
    {
      [text_view setHorizontallyResizable:YES];
      [text_container setContainerSize:NSMakeSize(FLT_MAX, FLT_MAX)];
      [text_container setWidthTracksTextView:NO];
      [text_container setLineFragmentPadding:0.0];
    }

    [scroll_view setHasHorizontalScroller:(ih->data->sb & IUP_SB_HORIZ) ? YES : NO];
    [scroll_view setHasVerticalScroller:(ih->data->sb & IUP_SB_VERT) ? YES : NO];
    [scroll_view setAutohidesScrollers:iupAttribGetBoolean(ih, "AUTOHIDE") ? YES : NO];

    IupCocoaFont* iup_font = iupcocoaGetFont(ih);
    if (iup_font)
    {
      [text_view setFont:[iup_font nativeFont]];
      [text_view setTypingAttributes:[iup_font attributeDictionary]];
    }

    [text_view setTextColor:[NSColor textColor]];
    [(IupCocoaTextView*)text_view setDefaultColor:[NSColor textColor]];
    [text_view setBackgroundColor:[NSColor textBackgroundColor]];

    root_view = scroll_view;
    main_view = text_view;

    IupCocoaTextViewDelegate* text_view_delegate = [[IupCocoaTextViewDelegate alloc] init];
    [text_view setDelegate:text_view_delegate];
    [text_view_delegate setIhandle:ih];
    objc_setAssociatedObject(text_view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
    objc_setAssociatedObject(text_view, IUP_COCOA_TEXT_DELEGATE_OBJ_KEY, text_view_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [text_view_delegate release];
    [text_view release];
  }
  else if(iupAttribGetBoolean(ih, "SPIN"))
  {
    NSTextField* text_field = [[IupCocoaTextField alloc] initWithFrame:NSZeroRect];
    NSStepper* stepper_view = [[NSStepper alloc] initWithFrame:NSZeroRect];

    iupcocoaSetViewControlSize(stepper_view, iupcocoaGetControlSize(ih));
    [stepper_view sizeToFit];
    CGFloat stepper_width = NSWidth([stepper_view frame]);
    CGFloat stepper_height = NSHeight([stepper_view frame]);
    CGFloat spacing = 4;

    CGFloat text_height = NSHeight([text_field frame]);
    CGFloat container_height = (text_height > stepper_height) ? text_height : stepper_height;

    NSView* container_view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 100, container_height)];

    [text_field setFrame:NSMakeRect(0, 0, 100 - stepper_width - spacing, container_height)];
    [text_field setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

    [stepper_view setFrame:NSMakeRect(100 - stepper_width, (container_height - stepper_height) / 2, stepper_width, stepper_height)];
    [stepper_view setAutoresizingMask:NSViewMinXMargin | NSViewMinYMargin | NSViewMaxYMargin];

    [container_view addSubview:text_field];
    [container_view addSubview:stepper_view];

    [stepper_view setValueWraps:(BOOL)iupAttribGetBoolean(ih, "SPINWRAP")];

    IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
    [number_formatter autorelease];
    [number_formatter setIhandle:ih];
    [number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
    [number_formatter setPartialStringValidationEnabled:YES];
    [number_formatter setNumberStyle:NSNumberFormatterNoStyle];
    [text_field setFormatter:number_formatter];

    IupCocoaTextSpinnerDelegate* text_spinner_delegate = [[IupCocoaTextSpinnerDelegate alloc] init];
    [text_field setDelegate:text_spinner_delegate];
    [text_spinner_delegate setIhandle:ih];

    [stepper_view setTarget:text_spinner_delegate];
    [stepper_view setAction:@selector(mySpinnerClickAction:)];

    objc_setAssociatedObject(stepper_view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
    objc_setAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
    objc_setAssociatedObject(text_field, IUP_COCOA_TEXT_DELEGATE_OBJ_KEY, (id)text_spinner_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [text_spinner_delegate release];

    root_view = container_view;
    main_view = text_field;

    objc_setAssociatedObject(container_view, IUP_COCOA_TEXT_STEPPER_OBJ_KEY, stepper_view, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

    [text_field release];
    [stepper_view release];

    ih->data->has_formatting = 0;

    ih->expand = ih->expand & ~IUP_EXPAND_HEIGHT;
  }
  else
  {
    NSTextField* text_field;

    if(iupAttribGetBoolean(ih, "PASSWORD"))
    {
      text_field = [[IupCocoaSecureTextField alloc] initWithFrame:NSZeroRect];
    }
    else
    {
      text_field = [[IupCocoaTextField alloc] initWithFrame:NSZeroRect];
    }

    IupCocoaTextFieldDelegate* text_field_delegate = [[IupCocoaTextFieldDelegate alloc] init];
    [text_field setDelegate:text_field_delegate];
    objc_setAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
    objc_setAssociatedObject(text_field, IUP_COCOA_TEXT_DELEGATE_OBJ_KEY, (id)text_field_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [text_field_delegate release];

    ih->expand = ih->expand & ~IUP_EXPAND_HEIGHT;

    ih->data->has_formatting = 0;

    root_view = text_field;
    main_view = text_field;
  }

  ih->handle = root_view;
  iupcocoaSetAssociatedViews(ih, main_view, root_view);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)cocoaTextConvertXYToPos);

  if (!ih->data->is_multiline)
  {
    IupCocoaFont* iup_font = iupcocoaGetFont(ih);
    if (iup_font && [main_view respondsToSelector:@selector(setFont:)])
      [(id)main_view setFont:[iup_font nativeFont]];
  }

  if (!ih->data->is_multiline && !iupAttribGetBoolean(ih, "BORDER"))
    cocoaTextSetBgColorAttrib(ih, iupAttribGet(ih, "BGCOLOR"));
  else if (iupAttribGet(ih, "BGCOLOR"))
    cocoaTextSetBgColorAttrib(ih, iupAttribGetStr(ih, "BGCOLOR"));
  if (iupAttribGet(ih, "FGCOLOR"))
    cocoaTextSetFgColorAttrib(ih, iupAttribGetStr(ih, "FGCOLOR"));
  if (ih->data->horiz_padding || ih->data->vert_padding)
    cocoaTextApplyPadding(ih);
  if (iupAttribGet(ih, "ALIGNMENT"))
    cocoaTextSetAlignmentAttrib(ih, iupAttribGetStr(ih, "ALIGNMENT"));

  iupcocoaAddToParent(ih);

  if(ih->data->formattags)
  {
    iupTextUpdateFormatTags(ih);
  }

  if(ih->data->is_multiline)
  {
    NSTextView* text_view = cocoaTextGetTextView(ih);
    NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
    [undo_manager removeAllActions];
  }

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupcocoaSetCanFocus(ih, 0);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

static void cocoaTextUnMapMethod(Ihandle* ih)
{
  id the_view = ih->handle;

  iupcocoaCommonBaseDestroyContextMenu(ih);

  if (ih->data->is_multiline)
  {
    NSScrollView* scroll_view = (NSScrollView*)the_view;
    NSTextView* text_view = [scroll_view documentView];
    if (text_view)
    {
      [NSObject cancelPreviousPerformRequestsWithTarget:text_view];
      id view_delegate = [text_view delegate];
      if (view_delegate)
      {
        [NSObject cancelPreviousPerformRequestsWithTarget:view_delegate];
        if ([view_delegate respondsToSelector:@selector(setIhandle:)])
          [view_delegate setIhandle:NULL];
      }
      [text_view setDelegate:nil];
      objc_setAssociatedObject(text_view, IUP_COCOA_TEXT_DELEGATE_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
      objc_setAssociatedObject(text_view, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
    }
  }
  else if (iupAttribGetBoolean(ih, "SPIN"))
  {
    NSTextField* text_field = (NSTextField*)iupcocoaGetMainView(ih);
    NSStepper* stepper_view = (NSStepper*)objc_getAssociatedObject(the_view, IUP_COCOA_TEXT_STEPPER_OBJ_KEY);
    if (text_field)
    {
      if ([text_field delegate])
        [NSObject cancelPreviousPerformRequestsWithTarget:[text_field delegate]];
      [text_field setDelegate:nil];
      objc_setAssociatedObject(text_field, IUP_COCOA_TEXT_DELEGATE_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
      objc_setAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
    }
    if (stepper_view)
    {
      [stepper_view setTarget:nil];
      objc_setAssociatedObject(stepper_view, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
    }
    objc_setAssociatedObject(the_view, IUP_COCOA_TEXT_STEPPER_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  else
  {
    NSTextField* text_field = (NSTextField*)the_view;
    if ([text_field delegate])
      [NSObject cancelPreviousPerformRequestsWithTarget:[text_field delegate]];
    [text_field setDelegate:nil];
    objc_setAssociatedObject(text_field, IUP_COCOA_TEXT_DELEGATE_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
  }

  iupcocoaRemoveFromParent(ih);
  iupcocoaSetAssociatedViews(ih, nil, nil);
  [the_view release];
  ih->handle = NULL;
}

static void cocoaTextComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  int natural_w = 0,
      natural_h = 0,
      visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS"),
      visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  (void)children_expand;
  int single_char_width;

  iupdrvFontGetCharSize(ih, NULL, &natural_h);
  natural_w = iupFontGetZerosWidth(ih, 10);

  single_char_width = natural_w / 10;

  natural_w = (visiblecolumns*natural_w)/10;

  natural_w += (single_char_width * 2);

  if (ih->data->is_multiline)
  {
    natural_h = visiblelines*natural_h;
  }

  if (iupAttribGetBoolean(ih, "BORDER"))
    iupdrvTextAddBorders(ih, &natural_w, &natural_h);
  else
    iupdrvTextAddExtraPadding(ih, &natural_w, &natural_h);

  if (iupAttribGetBoolean(ih, "SPIN"))
    iupdrvTextAddSpin(ih, &natural_w, natural_h);

  natural_w += 2*ih->data->horiz_padding;
  natural_h += 2*ih->data->vert_padding;

  if (ih->data->is_multiline && ih->data->sb)
  {
    int sb_size = iupdrvGetScrollbarSize();
    if (ih->data->sb & IUP_SB_HORIZ)
      natural_h += sb_size;
    if (ih->data->sb & IUP_SB_VERT)
      natural_w += sb_size;
  }

  *w = natural_w;
  *h = natural_h;

  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING_IGNORE))
      iupBaseComputeNaturalSize(child);
  }
}

static void cocoaTextApplyPaddingToField(NSTextField* text_field, int horiz_padding, int vert_padding)
{
  NSCell* cell = [text_field cell];
  if ([cell respondsToSelector:@selector(setIupPaddingHoriz:vert:)])
    [(id)cell setIupPaddingHoriz:(CGFloat)horiz_padding vert:(CGFloat)vert_padding];
  [text_field setNeedsDisplay:YES];
}

static void cocoaTextApplyPadding(Ihandle* ih)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch (sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
    {
      NSTextView* text_view = cocoaTextGetTextView(ih);
      [text_view setTextContainerInset:NSMakeSize(ih->data->horiz_padding, ih->data->vert_padding)];
      break;
    }
    case IUPCOCOATEXTSUBTYPE_FIELD:
    {
      NSTextField* text_field = cocoaTextGetTextField(ih);
      cocoaTextApplyPaddingToField(text_field, ih->data->horiz_padding, ih->data->vert_padding);
      break;
    }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
    {
      NSTextField* text_field = cocoaTextGetStepperTextField(ih);
      cocoaTextApplyPaddingToField(text_field, ih->data->horiz_padding, ih->data->vert_padding);
      break;
    }
  }
}

static int cocoaTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    cocoaTextApplyPadding(ih);
    return 0;
  }
  else
    return 1;
}

IUP_SDK_API int iupdrvTextGetFormatTags(Ihandle* ih, Ihandle* bulk_tag)
{
  if(!ih->data->is_multiline || !ih->data->has_formatting)
  {
    return 0;
  }

  NSTextView* text_view = cocoaTextGetTextView(ih);
  if(nil == text_view)
  {
    return 0;
  }

  NSTextStorage* text_storage = [text_view textStorage];
  NSUInteger length = [text_storage length];
  NSFontManager* font_manager = [NSFontManager sharedFontManager];

  [text_storage enumerateAttributesInRange:NSMakeRange(0, length)
                                   options:0
                                usingBlock:^(NSDictionary<NSAttributedStringKey, id>* attributes, NSRange range, BOOL* stop)
  {
    Ihandle* formattag = IupUser();
    IupSetStrf(formattag, "SELECTIONPOS", "%d:%d", iupcocoaStrUnitToChar([text_storage string], range.location), iupcocoaStrUnitToChar([text_storage string], range.location + range.length));
    IupAppend(bulk_tag, formattag);

    NSFont* font = [attributes objectForKey:NSFontAttributeName];
    if(nil != font)
    {
      NSFontTraitMask traits = [font_manager traitsOfFont:font];
      IupSetAttribute(formattag, "WEIGHT", (traits & NSBoldFontMask) ? "BOLD" : "NORMAL");
      IupSetAttribute(formattag, "ITALIC", (traits & NSItalicFontMask) ? "YES" : "NO");
      IupSetStrAttribute(formattag, "FONTFACE", [[font familyName] UTF8String]);
      IupSetInt(formattag, "FONTSIZE", (int)([font pointSize] + 0.5));
    }

    NSNumber* strikethrough = [attributes objectForKey:NSStrikethroughStyleAttributeName];
    IupSetAttribute(formattag, "STRIKEOUT", (nil != strikethrough && [strikethrough intValue] != 0) ? "YES" : "NO");

    id link = [attributes objectForKey:NSLinkAttributeName];
    if(nil != link)
    {
      NSString* url = [link isKindOfClass:[NSURL class]] ? [(NSURL*)link absoluteString] : (NSString*)link;
      if(nil != url)
      {
        IupSetStrAttribute(formattag, "LINK", [url UTF8String]);
      }
    }

    NSParagraphStyle* paragraph_style = [attributes objectForKey:NSParagraphStyleAttributeName];
    if(nil != paragraph_style)
    {
      CGFloat indent = [paragraph_style firstLineHeadIndent];
      if(indent <= 0.0)
      {
        indent = [paragraph_style headIndent];
      }
      if(indent > 0.0)
      {
        IupSetInt(formattag, "INDENT", (int)indent);
      }
    }

    (void)stop;
  }];

  return 1;
}

IUP_DRV_API int iupcocoaTextFieldCaretChar(NSTextField* text_field)
{
  return iupcocoaStrUnitToChar(cocoaTextFieldString(text_field), cocoaTextFieldGetRange(text_field).location);
}

static int cocoaTextCaretChar(Ihandle* ih)
{
  NSTextField* single_field = cocoaTextGetSingleLineField(ih);
  if (single_field)
    return iupcocoaTextFieldCaretChar(single_field);

  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSRange range = [[[text_view selectedRanges] lastObject] rangeValue];
  if (NSNotFound == range.location)
    range.location = [[text_view textStorage] length];
  return iupcocoaStrUnitToChar([[text_view textStorage] string], range.location);
}

static int cocoaTextSetSilent(Ihandle* ih, const char* value, IattribSetFunc set)
{
  char* saved = iupAttribGet(ih, "_IUPCOCOA_DISABLE_TEXT_CB");
  int ret;

  iupAttribSet(ih, "_IUPCOCOA_DISABLE_TEXT_CB", "1");
  ret = set(ih, value);
  iupAttribSet(ih, "_IUPCOCOA_DISABLE_TEXT_CB", saved);

  if (ih->handle && IupGetCallback(ih, "CARET_CB"))
    ih->data->last_caret_pos = cocoaTextCaretChar(ih);
  return ret;
}

static int cocoaTextSetValueAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetValueAttrib);
}

static int cocoaTextSetSelectedTextAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetSelectedTextAttrib);
}

static int cocoaTextSetSelectionAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetSelectionAttrib);
}

static int cocoaTextSetSelectionPosAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetSelectionPosAttrib);
}

static int cocoaTextSetCaretAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetCaretAttrib);
}

static int cocoaTextSetCaretPosAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetCaretPosAttrib);
}

static int cocoaTextSetInsertAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetInsertAttrib);
}

static int cocoaTextSetAppendAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetAppendAttrib);
}

static int cocoaTextSetClipboardAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetClipboardAttrib);
}

static int cocoaTextSetScrollToAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetScrollToAttrib);
}

static int cocoaTextSetScrollToPosAttribSilent(Ihandle* ih, const char* value)
{
  return cocoaTextSetSilent(ih, value, cocoaTextSetScrollToPosAttrib);
}

IUP_SDK_API void iupdrvTextInitClass(Iclass* ic)
{
  ic->Map = cocoaTextMapMethod;
  ic->UnMap = cocoaTextUnMapMethod;
  ic->ComputeNaturalSize = cocoaTextComputeNaturalSizeMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTextSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, cocoaTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", cocoaTextGetActiveAttrib, cocoaTextSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", cocoaTextGetValueAttrib, cocoaTextSetValueAttribSilent, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LINEVALUE", cocoaTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SELECTEDTEXT", cocoaTextGetSelectedTextAttrib, cocoaTextSetSelectedTextAttribSilent, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", cocoaTextGetSelectionAttrib, cocoaTextSetSelectionAttribSilent, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", cocoaTextGetSelectionPosAttrib, cocoaTextSetSelectionPosAttribSilent, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", cocoaTextGetCaretAttrib, cocoaTextSetCaretAttribSilent, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", cocoaTextGetCaretPosAttrib, cocoaTextSetCaretPosAttribSilent, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "INSERT", NULL, cocoaTextSetInsertAttribSilent, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, cocoaTextSetAppendAttribSilent, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "READONLY", cocoaTextGetReadOnlyAttrib, cocoaTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, cocoaTextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, cocoaTextSetClipboardAttribSilent, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, cocoaTextSetScrollToAttribSilent, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, cocoaTextSetScrollToPosAttribSilent, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SPINMIN", NULL, cocoaTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, cocoaTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, cocoaTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", cocoaTextGetSpinValueAttrib, cocoaTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COUNT", cocoaTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LINECOUNT", cocoaTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", cocoaTextGetFormattingAttrib, cocoaTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSARRAY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, cocoaTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, cocoaTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, cocoaTextSetTabSizeAttrib, "8", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOCORRECT", NULL, cocoaTextSetAutoCorrectAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, cocoaTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FILTER", NULL, cocoaTextSetFilterAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", cocoaTextGetScrollVisibleAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", cocoaTextGetOverwriteAttrib, cocoaTextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTEXTMENU", iupcocoaCommonBaseGetContextMenuAttrib, cocoaTextSetContextMenuAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}
