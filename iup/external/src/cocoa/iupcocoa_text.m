/** \file
 * \brief Text Control
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
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_array.h"
#include "iup_text.h"
#include "iup_childtree.h"

#include "iupcocoa_drv.h"
#import "IupCocoaTextSpinnerFilesOwner.h"

static const void* IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY = "IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY";
static const void* IUP_COCOA_TEXT_DELEGATE_OBJ_KEY = "IUP_COCOA_TEXT_DELEGATE_OBJ_KEY";
static const void* IUP_COCOA_TEXT_FORMATTER_KEY = "IUP_COCOA_TEXT_FORMATTER_KEY";
static const void* IUP_COCOA_TEXT_OVERWRITE_KEY = "IUP_COCOA_TEXT_OVERWRITE_KEY";

typedef enum
{
  IupCocoaTextSubType_UNKNOWN = -1,
  IUPCOCOATEXTSUBTYPE_FIELD,
  IUPCOCOATEXTSUBTYPE_VIEW,
  IUPCOCOATEXTSUBTYPE_STEPPER,
} IupCocoaTextSubType;

/* Forward declaration */
static bool cocoaTextComputeLineColumnFromRangeForTextView(NSTextView* text_view, NSRange native_selection_range,
              NSUInteger* out_start_line, NSUInteger* out_start_column, NSUInteger* out_end_line, NSUInteger* out_end_column);

/* Each IUP text subtype requires a completely different Cocoa native widget.
   This function provides a consistent and centralized way to distinguish which subtype we need.
   */
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
  return IupCocoaTextSubType_UNKNOWN;
}

static NSView* cocoaTextGetRootView(Ihandle* ih)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSView* root_container_view = (NSView*)ih->handle;
        NSCAssert([root_container_view isKindOfClass:[NSView class]], @"Expected NSView");
        return root_container_view;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSView* root_container_view = (NSView*)ih->handle;
        NSCAssert([root_container_view isKindOfClass:[NSView class]], @"Expected NSView");
        return root_container_view;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSStackView* root_container_view = (NSView*)ih->handle;
        NSCAssert([root_container_view isKindOfClass:[NSStackView class]], @"Expected NSStackView");
        return root_container_view;
      }
    default:
      {
        break;
      }
  }
  return nil;
}

static NSTextField* cocoaTextGetTextField(Ihandle* ih)
{
#ifdef USE_NSSTACKVIEW_TEXTFIELD_CONTAINER
  NSStackView* root_container_view = (NSStackView*)ih->handle;
  NSTextField* text_field = [[root_container_view views] firstObject];
  NSCAssert([text_field isKindOfClass:[NSTextField class]], @"Expected NSTextField");
  return text_field;
#else
  NSTextField* root_container_view = (NSTextField*)ih->handle;
  NSCAssert([root_container_view isKindOfClass:[NSTextField class]], @"Expected NSTextField");
  return root_container_view;
#endif
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
  IUPTextSpinnerContainer* spinner_container = (IUPTextSpinnerContainer*)objc_getAssociatedObject((id)ih->handle, IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY);
  NSCAssert([spinner_container isKindOfClass:[IUPTextSpinnerContainer class]], @"Expected IUPTextSpinnerContainer");
  return [spinner_container stepperView];
}

static NSTextField* cocoaTextGetStepperTextField(Ihandle* ih)
{
  IUPTextSpinnerContainer* spinner_container = (IUPTextSpinnerContainer*)objc_getAssociatedObject((id)ih->handle, IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY);
  NSCAssert([spinner_container isKindOfClass:[IUPTextSpinnerContainer class]], @"Expected IUPTextSpinnerContainer");
  return [spinner_container textField];
}

static IUPStepperObject* cocoaTextGetStepperObject(Ihandle* ih)
{
  IUPTextSpinnerContainer* spinner_container = (IUPTextSpinnerContainer*)objc_getAssociatedObject((id)ih->handle, IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY);
  NSCAssert([spinner_container isKindOfClass:[IUPTextSpinnerContainer class]], @"Expected IUPTextSpinnerContainer");
  return [spinner_container stepperObject];
}

static IUPStepperObjectController* cocoaTextGetStepperObjectController(Ihandle* ih)
{
  IUPTextSpinnerContainer* spinner_container = (IUPTextSpinnerContainer*)objc_getAssociatedObject((id)ih->handle, IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY);
  NSCAssert([spinner_container isKindOfClass:[IUPTextSpinnerContainer class]], @"Expected IUPTextSpinnerContainer");
  return [spinner_container stepperObjectController];
}

/* Custom text field cell to control text insets */
@interface IupCocoaTextFieldCell : NSTextFieldCell
@end

@implementation IupCocoaTextFieldCell

- (NSRect)drawingRectForBounds:(NSRect)rect
{
  NSRect drawingRect = [super drawingRectForBounds:rect];
  /* Super already provides inset - reset to exactly 2px to match NSTextView textContainerInset */
  CGFloat currentInset = drawingRect.origin.x - rect.origin.x;
  CGFloat targetInset = 2.0;
  CGFloat adjustment = targetInset - currentInset;

  drawingRect.origin.x += adjustment;
  drawingRect.size.width -= (2.0 * adjustment);

  return drawingRect;
}

@end

@interface IupCocoaTextField : NSTextField
@end

@implementation IupCocoaTextField

+ (Class)cellClass
{
  return [IupCocoaTextFieldCell class];
}

- (BOOL) textView:(NSTextView*)text_view shouldChangeTextInRange:(NSRange)change_range replacementString:(NSString*)replacement_string
{
  BOOL ret_flag;
  if([super textView:text_view shouldChangeTextInRange:change_range replacementString:replacement_string])
  {
    id the_delegate = [self delegate];
    if([the_delegate respondsToSelector:@selector(textView:shouldChangeTextInRange:replacementString:)])
    {
      ret_flag = [the_delegate textView:text_view shouldChangeTextInRange:change_range replacementString:replacement_string];
      if(NO == ret_flag)
      {
        return NO;
      }
    }
  }
  else
  {
    return NO;
  }

  NSTextField* text_field = self;
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);

  if(ih->data->disable_callbacks)
  {
    return YES;
  }

  IFnis action_cb = (IFnis)IupGetCallback(ih, "ACTION");
  int ret_val;

  int start_pos = (int)change_range.location;
  int end_pos = (int)(start_pos + change_range.length);

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

    NSString* replacement = [NSString stringWithUTF8String:replace_char];

    ih->data->disable_callbacks = 1;
    [text_view replaceCharactersInRange:change_range withString:replacement];
    ih->data->disable_callbacks = 0;

    [text_view didChangeText];
    return NO;
  }

  [text_view didChangeText];
  return YES;
}

- (NSMenu *)textView:(NSTextView *)text_view menu:(NSMenu *)the_menu forEvent:(NSEvent *)the_event atIndex:(NSUInteger)char_index
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

@end

/* Custom secure text field cell to control text insets */
@interface IupCocoaSecureTextFieldCell : NSSecureTextFieldCell
@end

@implementation IupCocoaSecureTextFieldCell

- (NSRect)drawingRectForBounds:(NSRect)rect
{
  NSRect drawingRect = [super drawingRectForBounds:rect];
  /* Super already provides inset - reset to exactly 2px to match NSTextView textContainerInset */
  CGFloat currentInset = drawingRect.origin.x - rect.origin.x;
  CGFloat targetInset = 2.0;
  CGFloat adjustment = targetInset - currentInset;

  drawingRect.origin.x += adjustment;
  drawingRect.size.width -= (2.0 * adjustment);

  return drawingRect;
}

@end

@interface IupCocoaSecureTextField : NSSecureTextField
@end

@implementation IupCocoaSecureTextField

+ (Class)cellClass
{
  return [IupCocoaSecureTextFieldCell class];
}

- (BOOL) textView:(NSTextView*)text_view shouldChangeTextInRange:(NSRange)change_range replacementString:(NSString*)replacement_string
{
  BOOL ret_flag;
  if([super textView:text_view shouldChangeTextInRange:change_range replacementString:replacement_string])
  {
    id the_delegate = [self delegate];
    if([the_delegate respondsToSelector:@selector(textView:shouldChangeTextInRange:replacementString:)])
    {
      ret_flag = [the_delegate textView:text_view shouldChangeTextInRange:change_range replacementString:replacement_string];
      if(NO == ret_flag)
      {
        return NO;
      }
    }
  }
  else
  {
    return NO;
  }

  NSTextField* text_field = self;
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);

  if(ih->data->disable_callbacks)
  {
    return YES;
  }

  IFnis action_cb = (IFnis)IupGetCallback(ih, "ACTION");
  int ret_val;

  int start_pos = (int)change_range.location;
  int end_pos = (int)(start_pos + change_range.length);

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

    NSString* replacement = [NSString stringWithUTF8String:replace_char];

    ih->data->disable_callbacks = 1;
    [text_view replaceCharactersInRange:change_range withString:replacement];
    ih->data->disable_callbacks = 0;

    [text_view didChangeText];
    return NO;
  }

  [text_view didChangeText];
  return YES;
}

- (NSMenu *)textView:(NSTextView *)text_view menu:(NSMenu *)the_menu forEvent:(NSEvent *)the_event atIndex:(NSUInteger)char_index
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (!iupAttribGet(ih, "_IUPCOCOA_CONTEXTMENU_SET"))
  {
    return the_menu;
  }

  Ihandle* menu_ih = (Ihandle*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");

  if(menu_ih && menu_ih->handle)
  {
    return (NSMenu*)menu_ih->handle;
  }
  else
  {
    return nil;
  }
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

      pos = (int)cursor_range.location;
      lin = (int)lin_start;
      col = (int)col_start;
      break;
    }
    case IUPCOCOATEXTSUBTYPE_FIELD:
    {
      NSTextField* text_field = cocoaTextGetTextField(ih);
      NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
      if (!field_editor)
        return;

      NSRange selected_range = [field_editor selectedRange];
      pos = (int)selected_range.location;
      col = pos + 1;
      lin = 1;
      break;
    }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
    {
      NSTextField* text_field = cocoaTextGetStepperTextField(ih);
      NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
      if (!field_editor)
        return;

      NSRange selected_range = [field_editor selectedRange];
      pos = (int)selected_range.location;
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

@interface IupCocoaTextFieldDelegate : NSObject <NSControlTextEditingDelegate, NSTextFieldDelegate>
@end

@implementation IupCocoaTextFieldDelegate

- (void) controlTextDidChange:(NSNotification*)the_notification
{
  NSTextField* text_field = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);

  if(ih->data->disable_callbacks)
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

  if (ih && !ih->data->disable_callbacks)
    cocoaTextCallCaretCb(ih);
}

@end

/* Inherit from IupCocoaTextFieldDelegate to get VALUECHANGED_CB */
@interface IupCocoaTextSpinnerDelegate : IupCocoaTextFieldDelegate
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTextSpinnerDelegate

- (IBAction) mySpinnerClickAction:(id)the_sender
{
  IFni callback_function;
  Ihandle* ih = [self ihandle];

  callback_function = (IFni)IupGetCallback(ih, "SPIN_CB");
  if(callback_function)
  {
    IUPStepperObject* stepper_object = cocoaTextGetStepperObject(ih);
    NSNumber* ns_number = [stepper_object stepperValue];

    int current_value = [ns_number intValue];

    int ret_val = callback_function(ih, current_value);

    if(IUP_IGNORE == ret_val)
    {
      /* We can't do anything with this */
    }
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

- (NSMenu *)textView:(NSTextView *)textView menu:(NSMenu *)menu forEvent:(NSEvent *)event atIndex:(NSUInteger)charIndex
{
  Ihandle* ih = [self ihandle];
  if (iupAttribGet(ih, "_IUPCOCOA_CONTEXTMENU_SET"))
  {
    Ihandle* menu_ih = (Ihandle*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");
    if (menu_ih && menu_ih->handle)
      return (NSMenu*)menu_ih->handle;
    else
      return nil;  /* Disable context menu */
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
        [text_view replaceCharactersInRange:change_range withString:filtered_string];
        ih->data->disable_callbacks = 0;
        [text_view didChangeText];
        return NO;
      }
    }
  }

  BOOL is_overwrite = NO;
  NSRange effective_range = change_range;
  NSNumber* overwrite_mode = objc_getAssociatedObject(text_view, IUP_COCOA_TEXT_OVERWRITE_KEY);
  if([overwrite_mode boolValue] && [replacement_string length] > 0 && change_range.length == 0)
  {
    NSTextStorage* text_storage = [text_view textStorage];
    NSUInteger text_length = [text_storage length];

    if(change_range.location < text_length)
    {
      NSUInteger chars_to_replace = [replacement_string length];
      NSUInteger available_chars = text_length - change_range.location;

      if(chars_to_replace > available_chars)
        chars_to_replace = available_chars;

      effective_range.length = chars_to_replace;
      is_overwrite = YES;
    }
  }

  IFnis action_cb = (IFnis)IupGetCallback(ih, "ACTION");
  int ret_val;

  int start_pos = (int)effective_range.location;
  int end_pos = (int)(start_pos + effective_range.length);

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

    NSString* replacement = [NSString stringWithUTF8String:replace_char];

    ih->data->disable_callbacks = 1;
    [text_view replaceCharactersInRange:effective_range withString:replacement];
    ih->data->disable_callbacks = 0;

    [text_view didChangeText];
    return NO;
  }

  if(is_overwrite)
  {
    ih->data->disable_callbacks = 1;
    [text_view replaceCharactersInRange:effective_range withString:replacement_string];
    ih->data->disable_callbacks = 0;
    [text_view didChangeText];
    return NO;
  }

  [text_view didChangeText];
  return YES;
}

- (void) textViewDidChangeSelection:(NSNotification*)the_notification
{
  Ihandle* ih = [self ihandle];

  if (ih && !ih->data->disable_callbacks)
    cocoaTextCallCaretCb(ih);
}

@end

void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{
  static int spin_min_width = -1;

  (void)h;
  (void)ih;

  /* Measure the minimum width required by NSStepper */
  if (spin_min_width < 0)
  {
    NSStepper* temp_stepper = [[NSStepper alloc] initWithFrame:NSZeroRect];
    [temp_stepper setMinValue:0];
    [temp_stepper setMaxValue:100];
    [temp_stepper setIncrement:1];

    NSSize fittingSize = [temp_stepper fittingSize];
    spin_min_width = (int)fittingSize.width;

    [temp_stepper release];
  }

  /* Only enforce minimum width, don't force expansion */
  if (*w < spin_min_width)
    *w = spin_min_width;
}

void iupdrvTextAddBorders(Ihandle* ih, int *x, int *y)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);

  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        /* Multiline text view with NSScrollView */
        /* NSScrollView with NSBezelBorder has minimal borders */
        const int SCROLLVIEW_BORDER_X = 2;
        const int SCROLLVIEW_BORDER_Y = 2;

        *x += SCROLLVIEW_BORDER_X;

        /* For VISIBLELINES, we need to account for NSTextView's actual line height */
        int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
        if (visiblelines > 0)
        {
          static CGFloat cocoa_line_height = -1.0;

          if (cocoa_line_height < 0.0)
          {
            /* Measure actual NSTextView line height */
            IupCocoaFont* iup_font = iupcocoaGetFont(ih);
            NSFont* font = [iup_font nativeFont];
            if (!font)
            {
              const char* default_font = IupGetGlobal("DEFAULTFONT");
              iup_font = iupcocoaFindFont(default_font);
              font = iup_font ? [iup_font nativeFont] : [NSFont systemFontOfSize:13];
            }

            /* NSLayoutManager calculates the actual line height used by NSTextView */
            NSLayoutManager* layoutManager = [[NSLayoutManager alloc] init];
            cocoa_line_height = [layoutManager defaultLineHeightForFont:font];
            [layoutManager release];
          }

          int char_height;
          iupdrvFontGetCharSize(ih, NULL, &char_height);

          /* Calculate the difference per line */
          int line_spacing = (int)lroundf(cocoa_line_height) - char_height;
          if (line_spacing < 0)
            line_spacing = 0;

          /* Add the total spacing for all visible lines */
          int total_spacing = line_spacing * visiblelines;
          *y += total_spacing;
        }

        *y += SCROLLVIEW_BORDER_Y;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
  case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        /* Single line text field (NSTextField) and spinbox */
        int border_size = 2 * 3;
        *x += border_size;
        *y += border_size;
        break;
      }
  default:
      {
        break;
      }
  }
}

/* GOTCHA: Modern characters may be multiple bytes (e.g. emoji characters). */
static NSUInteger cocoaTextCountGlyphsInString(NSString* text_string)
{
  NSRange full_range = NSMakeRange(0, [text_string length]);
  __block int glyph_count = 0;
  [text_string enumerateSubstringsInRange:full_range
                                  options:NSStringEnumerationByComposedCharacterSequences
                               usingBlock:^(NSString* substring, NSRange substring_range,
                                   NSRange enclosing_range, BOOL* stop)
                               {
                                 glyph_count++;
                               }
  ];
  return glyph_count;
}

/* Custom formatter to support UPPERCASE/LOWERCASE filters */
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
  /* Transform the string in real-time as the user types */
  if ([self uppercase] && partial_string && *partial_string)
  {
    NSString* upper = [*partial_string uppercaseString];
    if (![*partial_string isEqualToString:upper])
    {
      *partial_string = upper;
      return NO; /* Force the field to update */
    }
  }
  else if ([self lowercase] && partial_string && *partial_string)
  {
    NSString* lower = [*partial_string lowercaseString];
    if (![*partial_string isEqualToString:lower])
    {
      *partial_string = lower;
      return NO; /* Force the field to update */
    }
  }

  /* Check NC limit if applicable */
  Ihandle* ih = [self ihandle];
  if (ih && ih->data->nc > 0)
  {
    NSUInteger glyph_count = cocoaTextCountGlyphsInString(*partial_string);
    if (glyph_count > ih->data->nc)
    {
      return NO;
    }
  }

  return YES;
}

@end

/* This formatter supports the NC feature. */
@interface IupFormatter : NSFormatter
@property(nonatomic, assign) Ihandle* ihandle;
@end

/* I only use isPartialStringValid for "NC" */
@implementation IupFormatter

/* Required. Pass-through behavior */
- (NSString*) stringForObjectValue:(id)obj_val
{
  return obj_val;
}

/* Required. Pass-through behavior */
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

  /* Empty string is okay */
  if([partial_string length] == 0)
  {
    return YES;
  }
  /* Make sure to limit the length if the IUP "NC" feature is in use */
  Ihandle* ih = [self ihandle];
  if(ih->data->nc > 0)
  {
    /* I think this is supposed to be a glyph count instead of a byte count. */
    NSUInteger glyph_count = cocoaTextCountGlyphsInString(partial_string);
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

  /* Empty string is okay */
  if([partial_string length] == 0)
  {
    return YES;
  }
  /* Make sure to limit the length if the IUP "NC" feature is in use */
  Ihandle* ih = [self ihandle];
  if(ih->data->nc > 0)
  {
    NSUInteger glyph_count = cocoaTextCountGlyphsInString(partial_string);
    if(glyph_count > ih->data->nc)
    {
      return NO;
    }
  }

  NSMutableCharacterSet* allowed_character_set = [[NSCharacterSet decimalDigitCharacterSet] mutableCopy];
  [allowed_character_set autorelease];
  /* Allow scientific notation, decimal points, and positive and negative. */
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

          break;
        }
      case IUPCOCOATEXTSUBTYPE_STEPPER:
        {
          /* Leave the existing the Number formatter alone */
          return 0;

          break;
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
        if (char_width == 0) char_width = 8; /* fallback */
        CGFloat tab_width = tab_size * char_width;

        NSMutableParagraphStyle* paragraph_style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
        [paragraph_style autorelease];

        /* Create tab stops */
        NSMutableArray* tab_stops = [NSMutableArray array];
        for (int i = 1; i <= 32; i++)
        {
          NSTextTab* tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentLeft location:i * tab_width options:@{}];
          [tab_stops addObject:tab];
          [tab release];
        }
        [paragraph_style setTabStops:tab_stops];
        [paragraph_style setDefaultTabInterval:tab_width];

        /* Apply to the text view */
        [text_view setDefaultParagraphStyle:paragraph_style];

        /* Update typing attributes */
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

static int cocoaTextSetValueAttrib(Ihandle* ih, const char* value)
{
  NSString* ns_string;

  if(NULL == value)
  {
    ns_string = @"";
  }
  else
  {
    ns_string = [NSString stringWithUTF8String:value];
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

        /* Ensure a foreground color is present. If a user-defined FGCOLOR exists, it will be used. */
        /* Otherwise, default to the system's adaptive text color to support dark/light mode. */
        if (![attributes objectForKey:NSForegroundColorAttributeName])
        {
          char* fgcolor_str = iupAttribGet(ih, "FGCOLOR");
          unsigned char r, g, b;
          if (fgcolor_str && iupStrToRGB(fgcolor_str, &r, &g, &b))
          {
            [attributes setObject:[NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0] forKey:NSForegroundColorAttributeName];
          }
          else
          {
            [attributes setObject:[NSColor textColor] forKey:NSForegroundColorAttributeName];
          }
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

        NSNumberFormatter* conversion_formatter = [[NSNumberFormatter alloc] init];
        [conversion_formatter autorelease];
        NSNumberFormatter* text_field_formatter = (NSNumberFormatter*)[text_field formatter];
        [conversion_formatter setNumberStyle:[text_field_formatter numberStyle]];
        NSNumber* ns_number = [conversion_formatter numberFromString:ns_string];

        double current_number = [ns_number doubleValue];
        NSStepper* stepper_view = cocoaTextGetStepperView(ih);
        double max_value = [stepper_view maxValue];
        if(current_number > max_value)
        {
          current_number = max_value;
        }
        double min_value = [stepper_view minValue];
        if(current_number < min_value)
        {
          current_number = min_value;
        }

        if(iupAttribGetBoolean(ih, "SPINAUTO"))
        {
          NSNumber* number_to_set = [NSNumber numberWithDouble:current_number];
          IUPStepperObject* stepper_object = cocoaTextGetStepperObject(ih);
          [stepper_object setStepperValue:number_to_set];
        }
        else
        {
          ns_string = [NSString stringWithFormat:@"%lf", current_number];
          [text_field setStringValue:ns_string];
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

        unsigned char r, g, b;
        if(iupStrToRGB(value, &r, &g, &b))
        {
          CGFloat red = r/255.0;
          CGFloat green = g/255.0;
          CGFloat blue = b/255.0;

          NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
          [text_view setBackgroundColor:the_color];
        }
        else
        {
          NSColor* the_color = [NSColor textBackgroundColor];
          [text_view setBackgroundColor:the_color];
        }

        [text_view didChangeText];
        [undo_manager endUndoGrouping];

        return 1;

        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);

        unsigned char r, g, b;
        if(iupStrToRGB(value, &r, &g, &b))
        {
          CGFloat red = r/255.0;
          CGFloat green = g/255.0;
          CGFloat blue = b/255.0;

          NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
          [text_field setBackgroundColor:the_color];
        }
        else
        {
          [text_field setBackgroundColor:nil];
        }
        return 1;

        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);

        unsigned char r, g, b;
        if(iupStrToRGB(value, &r, &g, &b))
        {
          CGFloat red = r/255.0;
          CGFloat green = g/255.0;
          CGFloat blue = b/255.0;

          NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
          [text_field setBackgroundColor:the_color];
        }
        else
        {
          [text_field setBackgroundColor:nil];
        }

        return 1;

        break;
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
        NSTextView* text_view = cocoaTextGetTextView(ih);
        NSTextStorage* text_storage = [text_view textStorage];
        NSRange full_range = NSMakeRange(0, [text_storage length]);

        NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
        [undo_manager beginUndoGrouping];
        [text_storage beginEditing];

        unsigned char r, g, b;
        NSColor* the_color;
        if (iupStrToRGB(value, &r, &g, &b))
        {
          CGFloat red = r / 255.0;
          CGFloat green = g / 255.0;
          CGFloat blue = b / 255.0;
          the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
        }
        else
        {
          the_color = [NSColor textColor];
        }

        /* This sets typing attributes for new text. */
        [text_view setTextColor:the_color];

        /* This applies the color to all existing text. */
        if ([text_storage length] > 0)
        {
          [text_storage addAttribute:NSForegroundColorAttributeName value:the_color range:full_range];
        }

        [text_storage endEditing];
        [text_view didChangeText];
        [undo_manager endUndoGrouping];

        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);

        unsigned char r, g, b;
        if(iupStrToRGB(value, &r, &g, &b))
        {
          CGFloat red = r/255.0;
          CGFloat green = g/255.0;
          CGFloat blue = b/255.0;

          NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
          [text_field setTextColor:the_color];
        }
        else
        {
          [text_field setTextColor:nil];
        }
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);

        unsigned char r, g, b;
        if(iupStrToRGB(value, &r, &g, &b))
        {
          CGFloat red = r/255.0;
          CGFloat green = g/255.0;
          CGFloat blue = b/255.0;

          NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
          [text_field setTextColor:the_color];
        }
        else
        {
          [text_field setTextColor:nil];
        }

        break;
      }
    default:
      {
        break;
      }
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);
}

/* For the provided start_line, start_column, end_line, end_column, get the native NSRange for the selection. */
static bool cocoaTextComputeRangeFromLineColumnForTextView(NSTextView* text_view, NSUInteger start_line, NSUInteger start_column, NSUInteger end_line, NSUInteger end_column, NSRange* out_range)
{
  *out_range = NSMakeRange(0, 0);

  if(end_line < start_line)
  {
    return false;
  }
  else if((end_line == start_line) && (end_column < start_column))
  {
    return false;
  }

  NSLayoutManager* layout_manager = [text_view layoutManager];
  NSUInteger number_of_glyphs = [layout_manager numberOfGlyphs];
  if(0 == number_of_glyphs)
  {
    if((start_line <= 1) && (start_column <= 1))
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  NSUInteger number_of_lines;
  NSUInteger index;

  NSRange line_range = NSMakeRange(0, 0);

  bool found_start_line = false;
  bool found_start_column = false;
  NSUInteger selection_start_index = 0;

  for(number_of_lines = 0, index = 0; index < number_of_glyphs;)
  {
    NSUInteger last_index = index;
    NSRange last_range = line_range;

    (void) [layout_manager lineFragmentRectForGlyphAtIndex:index
                                            effectiveRange:&line_range];
    index = NSMaxRange(line_range);
    number_of_lines++;
    if(number_of_lines == start_line)
    {
      found_start_line = true;
      if(line_range.length >= start_column)
      {
        found_start_column = true;
        NSUInteger overshot_count = line_range.length - (start_column-1);
        selection_start_index = index - overshot_count;
      }
      else
      {
        found_start_column = false;
        selection_start_index = index;
      }

      /* Edge case: end_line is the same line as start_line */
      if(start_line == end_line)
      {
        number_of_lines--;
        line_range = last_range;
        index = last_index;
      }

      break;
    }
  }

  if(!found_start_line)
  {
    return false;
  }

  /* Do end */
  bool found_end_line = false;
  bool found_end_column = false;
  NSUInteger selection_end_index = 0;

  for(; index < number_of_glyphs;)
  {
    (void) [layout_manager lineFragmentRectForGlyphAtIndex:index
                                            effectiveRange:&line_range];
    index = NSMaxRange(line_range);
    number_of_lines++;
    if(number_of_lines == end_line)
    {
      found_end_line = true;
      if(line_range.length >= end_column)
      {
        found_start_column = true;
        NSUInteger overshot_count = line_range.length - (end_column-1);
        selection_end_index = index - overshot_count;
      }
      else
      {
        found_end_column = false;
        selection_end_index = index;
      }
      break;
    }
  }

  if(!found_end_line)
  {
    selection_end_index = index;
  }

  NSRange selection_range = NSMakeRange(selection_start_index, selection_end_index-selection_start_index);
  *out_range = selection_range;

  return true;
}

/* For a provided native_selection_range, get the start_line, start_column, end_line, end_column */
static bool cocoaTextComputeLineColumnFromRangeForTextView(NSTextView* text_view, NSRange native_selection_range, NSUInteger* out_start_line, NSUInteger* out_start_column, NSUInteger* out_end_line, NSUInteger* out_end_column)
{
  *out_start_line = 1;
  *out_start_column = 1;
  *out_end_line = 1;
  *out_end_column = 1;

  NSUInteger start_line = 1;
  NSUInteger start_column = 1;
  NSUInteger end_line = 1;
  NSUInteger end_column = 1;

  NSLayoutManager* layout_manager = [text_view layoutManager];
  NSUInteger number_of_glyphs = [layout_manager numberOfGlyphs];
  if(0 == number_of_glyphs)
  {
    if(native_selection_range.location == 0)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  NSUInteger number_of_lines;
  NSUInteger index;
  NSRange line_range = NSMakeRange(0, 0);

  bool found_start_line = false;
  bool found_start_column = false;
  NSUInteger selection_start_index = native_selection_range.location;
  NSUInteger selection_end_index = native_selection_range.location + native_selection_range.length;

  for(number_of_lines = 0, index = 0; index < number_of_glyphs;)
  {
    (void) [layout_manager lineFragmentRectForGlyphAtIndex:index
                                            effectiveRange:&line_range];
    index = NSMaxRange(line_range);
    number_of_lines++;
    if(index >= selection_start_index)
    {
      found_start_line = true;
      start_line = number_of_lines;

      found_start_column = true;
      NSUInteger overshot_count = index - (selection_start_index);
      start_column = line_range.length - overshot_count + 1;

      /* Edge case: end_line is the same line as start_line */
      if(index >= selection_end_index)
      {
        NSUInteger overshot_end_count = index - (selection_end_index);
        end_column = line_range.length - overshot_end_count + 1;
        end_line = start_line;

        *out_start_line = start_line;
        *out_start_column = start_column;
        *out_end_line = end_line;
        *out_end_column = end_column;
        return true;
      }
      break;
    }
  }

  if(!found_start_line)
  {
    return false;
  }

  /* Do end */
  bool found_end_line = false;
  bool found_end_column = false;

  for(; index < number_of_glyphs;)
  {
    (void) [layout_manager lineFragmentRectForGlyphAtIndex:index
                                            effectiveRange:&line_range];
    index = NSMaxRange(line_range);
    number_of_lines++;
    if(index >= selection_end_index)
    {
      found_end_line = true;
      end_line = number_of_lines;

      found_end_column = true;

      NSUInteger overshot_count = index - (selection_end_index);
      end_column = line_range.length - overshot_count + 1;
      break;
    }
  }

  if(!found_end_line)
  {
    end_line = number_of_lines;
    end_column = line_range.length;
  }

  *out_start_line = start_line;
  *out_start_column = start_column;
  *out_end_line = end_line;
  *out_end_column = end_column;
  return true;
}

void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
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
            *pos = (int)native_selection_range.location;
          }
        }
        return;

        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];

        if(![field_editor isKindOfClass:[NSTextView class]])
        {
          if(pos)
          {
            if(col > 0)
            {
              *pos = col - 1;
            }
            else
            {
              *pos = 0;
            }
          }
          return;
        }

        NSTextView* text_view = (NSTextView*)field_editor;
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
            *pos = (int)native_selection_range.location;
          }
        }
        return;

        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];

        if(![field_editor isKindOfClass:[NSTextView class]])
        {
          if(pos)
          {
            if(col > 0)
            {
              *pos = col - 1;
            }
            else
            {
              *pos = 0;
            }
          }
          return;
        }

        NSTextView* text_view = (NSTextView*)field_editor;
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
            *pos = (int)native_selection_range.location;
          }
        }
        return;

        break;
      }
    default:
      {
        break;
      }
  }
}

void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        NSRange range_position = NSMakeRange(pos, 0);

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
        return;

        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];

        if(![field_editor isKindOfClass:[NSTextView class]])
        {
          if(lin)
          {
            *lin = 1;
          }
          if(col)
          {
            *col = pos + 1;
          }

          return;
        }

        NSTextView* text_view = (NSTextView*)field_editor;
        NSUInteger lin_start=1;
        NSUInteger col_start=1;
        NSUInteger lin_end=1;
        NSUInteger col_end=1;
        NSRange range_position = NSMakeRange(pos, 0);
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
        return;

        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];

        if(![field_editor isKindOfClass:[NSTextView class]])
        {
          if(lin)
          {
            *lin = 1;
          }
          if(col)
          {
            *col = pos + 1;
          }

          return;
        }

        NSTextView* text_view = (NSTextView*)field_editor;
        NSUInteger lin_start=1;
        NSUInteger col_start=1;
        NSUInteger lin_end=1;
        NSUInteger col_end=1;
        NSRange range_position = NSMakeRange(pos, 0);
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
        return;

        break;
      }
    default:
      {
        break;
      }
  }
}

void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
  [undo_manager beginUndoGrouping];

  NSTextStorage* text_storage = [text_view textStorage];
  [text_storage beginEditing];
  return NULL;
}

void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  NSTextView* text_view = cocoaTextGetTextView(ih);
  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];

  NSTextStorage* text_storage = [text_view textStorage];
  [text_storage endEditing];
  [undo_manager endUndoGrouping];
}

/* You must pass in a valid paragraph_style. This routine will alter it depending on which Iup attributes were set. */
static bool cocoaTextParseParagraphAttributes(NSMutableParagraphStyle* paragraph_style, Ihandle* formattag)
{
  bool needs_paragraph_style = false;
  char* format;

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
    char* str;
    NSMutableArray* tab_array = [NSMutableArray array];

    while(format)
    {
      str = iupStrDupUntil((char**)&format, ' ');
      if (!str) break;
      pos = atoi(str);
      free(str);

      str = iupStrDupUntil((char**)&format, ' ');
      if (!str) break;

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
    }

    [paragraph_style setTabStops:tab_array];
    needs_paragraph_style = true;
  }

  return needs_paragraph_style;
}

static bool cocoaTextParseParagraphFormat(Ihandle* ih, Ihandle* formattag, NSTextView* text_view, NSRange selection_range)
{
  NSTextStorage* text_storage = [text_view textStorage];
  NSString* all_string = [text_storage string];
  __block bool did_change_attribute = false;

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
        [text_view shouldChangeTextInRange:paragraph_range replacementString:nil];
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
- (NSString*) markerWithTabsForItemNumber:(NSInteger)item_num;

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

- (NSString*) markerWithTabsForItemNumber:(NSInteger)item_num
{
  NSString* base_string = [self markerForItemNumber:item_num];
  NSString* customized_marker = nil;
  customized_marker = [NSString stringWithFormat:@"\t%@\t", base_string];
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
      __block NSUInteger item_count = 1;
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

          NSRange paragraph_range = NSMakeRange(start_paragraph_index, end_paragraph_index-start_paragraph_index);

          IupNumberingTextList* text_list = [[IupNumberingTextList alloc] initWithMarkerFormat:which_list_marker options:0];
          [text_list autorelease];
          [text_list setNumberingStyle:which_number_style];
          [array_of_text_lists addObject:text_list];

          NSMutableAttributedString* current_line = [[text_storage attributedSubstringFromRange:paragraph_range] mutableCopy];
          [current_line autorelease];
          NSDictionary<NSAttributedStringKey, id>* current_line_attributes = [current_line attributesAtIndex:0 effectiveRange:NULL];

          NSString* marker_with_style_prefix = nil;
          marker_with_style_prefix = [text_list markerWithTabsForItemNumber:item_count];

          NSAttributedString* attribued_prefix = [[NSAttributedString alloc] initWithString:marker_with_style_prefix attributes:current_line_attributes];
          [attribued_prefix autorelease];
          [current_line insertAttributedString:attribued_prefix atIndex:0];

          ih->data->disable_callbacks = 1;
          [text_view shouldChangeTextInRange:paragraph_range replacementString:[current_line string]];
          ih->data->disable_callbacks = 0;
          [text_storage replaceCharactersInRange:paragraph_range withAttributedString:current_line];

          item_count++;
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
      [text_storage addAttribute:NSParagraphStyleAttributeName value:paragraph_style range:applied_paragraph_range];

      [text_storage endEditing];
      [text_view didChangeText];

    }
    else /* We attempt to remove list formatting */
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

          NSRange paragraph_range = NSMakeRange(start_paragraph_index, end_paragraph_index-start_paragraph_index);

          NSMutableAttributedString* current_line = [[text_storage attributedSubstringFromRange:paragraph_range] mutableCopy];
          [current_line autorelease];
          NSString* current_line_nsstr = [current_line string];

          NSString* marker_prefix_pattern = @"^\t.*?\t";
          NSError* ns_error = nil;
          NSRegularExpression* reg_ex = [NSRegularExpression
            regularExpressionWithPattern:marker_prefix_pattern
                                 options:NSRegularExpressionAnchorsMatchLines
                                   error:&ns_error
          ];

          NSArray<NSTextCheckingResult*>* regex_matches = [reg_ex matchesInString:current_line_nsstr
                                                                          options:kNilOptions
                                                                            range:NSMakeRange(0, [current_line_nsstr length])
          ];

          for(NSTextCheckingResult* match in regex_matches)
          {
            [current_line deleteCharactersInRange:[match range]];
          }

          ih->data->disable_callbacks = 1;
          [text_view shouldChangeTextInRange:paragraph_range replacementString:[current_line string]];
          ih->data->disable_callbacks = 0;
          [text_storage replaceCharactersInRange:paragraph_range withAttributedString:current_line];
        }
      ];
      NSRange applied_paragraph_range = NSMakeRange(applied_paragraph_start, applied_paragraph_end-applied_paragraph_start);
      NSMutableDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[[text_storage attributedSubstringFromRange:applied_paragraph_range] attributesAtIndex:0 effectiveRange:NULL] mutableCopy];
      [text_storage_attributes autorelease];
      NSMutableParagraphStyle* paragraph_style = [[text_storage_attributes objectForKey:NSParagraphStyleAttributeName] mutableCopy];
      [paragraph_style autorelease];
      if(nil == paragraph_style)
      {
      }
      else
      {
        [paragraph_style setTextLists:[NSArray array]];
        [text_storage removeAttribute:NSParagraphStyleAttributeName range:applied_paragraph_range];
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
  NSFont* target_font = start_font;

  NSInteger current_weight = [font_manager weightOfFont:target_font];
  if(current_weight < font_target_weight)
  {
    while(current_weight < font_target_weight)
    {
      NSFont* result_font;
      result_font = [font_manager convertWeight:YES ofFont:target_font];
      current_weight = [font_manager weightOfFont:target_font];

      if(result_font == target_font)
      {
        break;
      }
      else
      {
        target_font = result_font;
      }
    }
  }
  else if(current_weight > font_target_weight)
  {
    while(current_weight > font_target_weight)
    {
      NSFont* result_font;
      result_font = [font_manager convertWeight:NO ofFont:target_font];
      current_weight = [font_manager weightOfFont:target_font];

      if(result_font == target_font)
      {
        break;
      }
      else
      {
        target_font = result_font;
      }
    }
  }
  return target_font;
}

static NSMutableDictionary* cocoaTextParseCharacterFormat(Ihandle* ih, Ihandle* formattag, NSTextView* text_view, NSRange selection_range)
{
  char* format;
  bool did_change_attribute = false;
  bool did_change_font_size = false;
  bool did_change_font_family = false;
  bool did_change_font_traits = false;
  bool did_change_font_weight = false;

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
    if(font_size_int < 0)
    {
    }
    else
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
      font_family_name = [NSString stringWithUTF8String:mapped_name];
    }
    else
    {
      font_family_name = [NSString stringWithUTF8String:format];
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

  format = iupAttribGet(formattag, "FGCOLOR");
  if(format)
  {
    unsigned char r, g, b;
    if(iupStrToRGB(format, &r, &g, &b))
    {
      CGFloat red = r/255.0;
      CGFloat green = g/255.0;
      CGFloat blue = b/255.0;

      NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
      [attribute_dict setValue:the_color
                        forKey:NSForegroundColorAttributeName];

      needs_add_font_fgcolor = true;
    }
    else
    {
      needs_add_font_fgcolor = false;
    }
    did_change_attribute = true;
    did_change_font_fgcolor = true;
  }

  format = iupAttribGet(formattag, "BGCOLOR");
  if(format)
  {
    unsigned char r, g, b;
    if(iupStrToRGB(format, &r, &g, &b))
    {
      CGFloat red = r/255.0;
      CGFloat green = g/255.0;
      CGFloat blue = b/255.0;

      NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
      [attribute_dict setObject:the_color
                         forKey:NSBackgroundColorAttributeName];
      needs_add_font_bgcolor = true;
    }
    else
    {
      needs_add_font_bgcolor = false;
    }
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
    else
    {
      if(iupAttribGet(formattag, "FGCOLOR"))
      {
      }
      else
      {
        [attribute_dict removeObjectForKey:NSForegroundColorAttributeName];
        did_change_font_fgcolor = true;
        needs_add_font_fgcolor = false;
      }
    }
    did_change_attribute = true;
  }

  if(did_change_attribute)
  {
    NSFontManager* font_manager = [NSFontManager sharedFontManager];
    IupCocoaFont* iup_font = iupcocoaGetFont(ih);

    NSTextStorage* text_storage = [text_view textStorage];

    if(selection_range.location < [text_storage length])
    {
      [text_storage beginEditing];
      ih->data->disable_callbacks = 1;

      NSUInteger loc = selection_range.location;
      NSUInteger end = selection_range.length + loc;
      while(loc < end)
      {
        NSRange attrib_range;

        NSRange sub_range = {loc, end-loc};
        NSAttributedString* current_substring = [text_storage attributedSubstringFromRange:sub_range];
        NSDictionary<NSAttributedStringKey, id>* current_substring_attributes = [current_substring attributesAtIndex:0 effectiveRange:&attrib_range];

        NSFont* target_font = nil;

        if(did_change_font_family || did_change_font_traits || did_change_font_size || did_change_font_weight)
        {
          NSFont* base_font = [current_substring_attributes objectForKey:NSFontAttributeName];
          if(nil == base_font)
          {
            base_font = [iup_font nativeFont];
          }
          target_font = base_font;

          if(did_change_font_family)
          {
            target_font = [font_manager convertFont:target_font toFamily:font_family_name];
          }
          if(did_change_font_size)
          {
            target_font = [font_manager convertFont:target_font toSize:font_size];
          }
          if(did_change_font_traits)
          {
            target_font = [font_manager convertFont:target_font toHaveTrait:trait_mask];
          }

          if(did_change_font_weight)
          {
            target_font = cocoaTextChangeFontWeight(target_font, font_target_weight);
          }

          [attribute_dict setObject:target_font forKey:NSFontAttributeName];
        }
        NSRange adjusted_range = { loc, attrib_range.length };
        [text_view shouldChangeTextInRange:adjusted_range replacementString:nil];
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
    else /* For setTypingAttributes: */
    {
      NSDictionary<NSAttributedStringKey, id>* current_substring_attributes = [text_view typingAttributes];

      NSFont* target_font = nil;

      if(did_change_font_family || did_change_font_traits || did_change_font_size || did_change_font_weight)
      {
        NSFont* base_font = [current_substring_attributes objectForKey:NSFontAttributeName];
        if(nil == base_font)
        {
          base_font = [iup_font nativeFont];
        }
        target_font = base_font;

        if(did_change_font_family)
        {
          target_font = [font_manager convertFont:target_font toFamily:font_family_name];
        }
        if(did_change_font_size)
        {
          target_font = [font_manager convertFont:target_font toSize:font_size];
        }
        if(did_change_font_traits)
        {
          target_font = [font_manager convertFont:target_font toHaveTrait:trait_mask];
        }

        if(did_change_font_weight)
        {
          target_font = cocoaTextChangeFontWeight(target_font, font_target_weight);
        }

        [attribute_dict setObject:target_font forKey:NSFontAttributeName];
      }
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

void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
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
      NSUInteger start = (NSUInteger)start_int;
      NSUInteger end = (NSUInteger)end_int;
      native_selection_range = NSMakeRange(start, end-start);
    }
    else
    {
      NSTextStorage* text_storage = [text_view textStorage];
      native_selection_range = NSMakeRange([text_storage length], 0);

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

  IupCocoaFont* iup_font = iupcocoaGetFont(ih);

  NSMutableDictionary* attribute_dict = [[iup_font attributeDictionary] mutableCopy];
  [attribute_dict autorelease];
  NSTextStorage* text_storage = [text_view textStorage];
  NSDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[text_storage attributedSubstringFromRange:native_selection_range] attributesAtIndex:0 effectiveRange:NULL];
  [attribute_dict addEntriesFromDictionary:text_storage_attributes];

  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
  [undo_manager beginUndoGrouping];

  [text_view shouldChangeTextInRange:native_selection_range replacementString:nil];

  cocoaTextParseBulletNumberListFormat(ih, formattag, text_view, native_selection_range);
  cocoaTextParseParagraphFormat(ih, formattag, text_view, native_selection_range);
  cocoaTextParseCharacterFormat(ih, formattag, text_view, native_selection_range);

  [text_view didChangeText];
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
          [text_storage beginEditing];

          ih->data->disable_callbacks = 1;
          [text_view shouldChangeTextInRange:selection_range replacementString:nil];
          [text_storage setAttributes:nil range:selection_range];
          ih->data->disable_callbacks = 0;

          [text_storage endEditing];

          IupCocoaFont* iup_font = iupcocoaGetFont(ih);

          [text_view setTypingAttributes:[iup_font attributeDictionary]];
          [text_view setRichText:enable_formatting];

          [text_view didChangeText];
          [text_storage endEditing];
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
  [text_storage setAttributes:nil range:selection_range];
  ih->data->disable_callbacks = 0;

  [text_storage endEditing];

  [text_view didChangeText];
  [text_storage endEditing];
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

static char* cocoaTextGetSelectedTextAttrib(Ihandle* ih)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        NSRange selected_range = [text_view selectedRange];
        NSString* selected_string = [[[text_view textStorage] string] substringWithRange:selected_range];
        return iupStrReturnStr([selected_string UTF8String]);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSRange selected_range = [field_editor selectedRange];
        NSString* selected_string = [[field_editor string] substringWithRange:selected_range];
        return iupStrReturnStr([selected_string UTF8String]);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSRange selected_range = [field_editor selectedRange];
        NSString* selected_string = [[field_editor string] substringWithRange:selected_range];
        return iupStrReturnStr([selected_string UTF8String]);
        break;
      }
    default:
      {
        break;
      }
  }

  return NULL;
}

static int cocoaTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if(NULL == value)
  {
    return 0;
  }

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
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

        NSString* ns_insert_string = [NSString stringWithUTF8String:value];

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
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSRange selected_range = [field_editor selectedRange];
        if((NSNotFound == selected_range.location) || (0 == selected_range.length))
        {
          return 0;
        }

        NSString* ns_string = [NSString stringWithUTF8String:value];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        [(NSTextView*)field_editor insertText:ns_string replacementRange:selected_range];
        return 0;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSRange selected_range = [field_editor selectedRange];
        if((NSNotFound == selected_range.location) || (0 == selected_range.length))
        {
          return 0;
        }

        NSString* ns_string = [NSString stringWithUTF8String:value];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        [(NSTextView*)field_editor insertText:ns_string replacementRange:selected_range];
        return 0;

        break;
      }
    default:
      {
        break;
      }
  }
  return 0;
}

static int cocoaTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  NSTextView* text_view = nil;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        text_view = cocoaTextGetTextView(ih);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    default:
      {
        return 0;
        break;
      }
  }

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
    int ret_val;
    NSUInteger lin_start=1;
    NSUInteger col_start=1;
    NSUInteger lin_end=1;
    NSUInteger col_end=1;

    switch(sub_type)
    {
      case IUPCOCOATEXTSUBTYPE_VIEW:
        {
          ret_val = sscanf(value, "%lu,%lu:%lu,%lu", &lin_start, &col_start, &lin_end, &col_end);
          if(ret_val != 4)
          {
            return 0;
          }
          if(lin_start<1 || col_start<1 || lin_end<1 || col_end<1)
          {
            return 0;
          }
          break;
        }
      case IUPCOCOATEXTSUBTYPE_FIELD:
    case IUPCOCOATEXTSUBTYPE_STEPPER:
        {
          int col_start_int = 1;
          int col_end_int = 1;
          if(iupStrToIntInt(value, &col_start_int, &col_end_int, ':')!=2)
          {
            return 0;
          }
          col_start = col_start_int;
          col_end = col_end_int;

          if(col_start<0 || col_end<0)
          {
            return 0;
          }
          break;
        }
    default:
        {
          return 0;
          break;
        }
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
  NSTextView* text_view = nil;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        text_view = cocoaTextGetTextView(ih);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    default:
      {
        return NULL;
        break;
      }
  }

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

  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        return iupStrReturnStrf("%lu,%lu:%lu,%lu", lin_start, col_start, lin_end, col_end);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
  case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        if(lin_end > 1)
        {
          did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, 1, col_start, 1, col_end, &selected_range);
          if(did_find_range)
          {
            col_start = selected_range.location + 1;
            col_end = col_start + selected_range.length;
          }
          else
          {
            return NULL;
          }
        }

        return iupStrReturnIntInt((int)col_start, (int)col_end, ':');
        break;
      }
  default:
      {
        return NULL;
        break;
      }
  }

  return NULL;
}

static int cocoaTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  NSTextView* text_view = nil;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        text_view = cocoaTextGetTextView(ih);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    default:
      {
        return 0;
        break;
      }
  }

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
    start = (NSUInteger)start_int;
    end = (NSUInteger)end_int;
    selection_range = NSMakeRange(start, end-start);
  }

  [text_view setSelectedRange:selection_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];

  return 0;
}

static char* cocoaTextGetSelectionPosAttrib(Ihandle* ih)
{
  NSTextView* text_view = nil;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        text_view = cocoaTextGetTextView(ih);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    default:
      {
        return NULL;
        break;
      }
  }

  NSRange selected_range = [text_view selectedRange];
  if(NSNotFound == selected_range.location)
  {
    return NULL;
  }
  NSUInteger start = selected_range.location;
  NSUInteger end = selected_range.location + selected_range.length;

  return iupStrReturnIntInt((int)start, (int)end, ':');
}

static int cocoaTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  NSTextView* text_view = nil;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        text_view = cocoaTextGetTextView(ih);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    default:
      {
        return 0;
        break;
      }
  }

  NSRange cursor_range = NSMakeRange(0, 0);

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
  NSUInteger lin_start=start_int;
  NSUInteger col_start=end_int;
  NSUInteger lin_end=start_int;
  NSUInteger col_end=end_int;
  bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &cursor_range);
  if(did_find_range)
  {
    [text_view setSelectedRange:cursor_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];
    [text_view scrollRangeToVisible:cursor_range];
  }

  return 0;
}

static char* cocoaTextGetCaretAttrib(Ihandle* ih)
{
  NSTextView* text_view = nil;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        text_view = cocoaTextGetTextView(ih);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    default:
      {
        return NULL;
        break;
      }
  }

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

  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        return iupStrReturnIntInt((int)lin_start, (int)col_start, ':');
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
  case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        if(lin_end > 1)
        {
          did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, 1, col_start, 1, col_end, &cursor_range);
          if(did_find_range)
          {
            col_start = cursor_range.location + 1;
          }
          else
          {
            return NULL;
          }
        }

        return iupStrReturnInt((int)col_start);
        break;
      }
  default:
      {
        return NULL;
        break;
      }
  }

  return NULL;
}

static int cocoaTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  NSTextView* text_view = nil;
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);

  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      text_view = cocoaTextGetTextView(ih);
      break;
    case IUPCOCOATEXTSUBTYPE_FIELD:
    {
      NSTextField* text_field = cocoaTextGetTextField(ih);
      NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
      if ([field_editor isKindOfClass:[NSTextView class]])
        text_view = (NSTextView*)field_editor;
      break;
    }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
    {
      NSTextField* text_field = cocoaTextGetStepperTextField(ih);
      NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
      if ([field_editor isKindOfClass:[NSTextView class]])
        text_view = (NSTextView*)field_editor;
      break;
    }
    default:
      return 0;
  }

  if (!text_view)
    return 0;

  NSRange cursor_range = NSMakeRange(0, 0);

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    iupStrToIntInt(value, &lin, &col, ',');
    if (lin < 1) lin = 1;
    if (col < 1) col = 1;

    NSUInteger lin_start = lin;
    NSUInteger col_start = col;
    NSUInteger lin_end = lin;
    NSUInteger col_end = col;

    bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &cursor_range);
    if (did_find_range)
      [text_view scrollRangeToVisible:cursor_range];
  }
  else
  {
    int pos = 1;
    iupStrToInt(value, &pos);
    if (pos < 1) pos = 1;
    pos--;
    cursor_range = NSMakeRange(pos, 0);
    [text_view scrollRangeToVisible:cursor_range];
  }

  return 0;
}

static int cocoaTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  int pos = 0;
  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  NSTextView* text_view = nil;
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);

  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      text_view = cocoaTextGetTextView(ih);
      break;
    case IUPCOCOATEXTSUBTYPE_FIELD:
    {
      NSTextField* text_field = cocoaTextGetTextField(ih);
      NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
      if ([field_editor isKindOfClass:[NSTextView class]])
        text_view = (NSTextView*)field_editor;
      break;
    }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
    {
      NSTextField* text_field = cocoaTextGetStepperTextField(ih);
      NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
      if ([field_editor isKindOfClass:[NSTextView class]])
        text_view = (NSTextView*)field_editor;
      break;
    }
    default:
      return 0;
  }

  if (!text_view)
    return 0;

  NSRange cursor_range = NSMakeRange(pos, 0);
  [text_view scrollRangeToVisible:cursor_range];

  return 0;
}

static int cocoaTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  NSTextView* text_view = nil;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        text_view = cocoaTextGetTextView(ih);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    default:
      {
        return 0;
        break;
      }
  }

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
  cursor_range = NSMakeRange(pos, 0);

  [text_view setSelectedRange:cursor_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];

  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        [text_view scrollRangeToVisible:cursor_range];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
  case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        break;
      }
  default:
      {
        return 0;
        break;
      }
  }
  return 0;
}

static char* cocoaTextGetCaretPosAttrib(Ihandle* ih)
{
  NSTextView* text_view = nil;

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        text_view = cocoaTextGetTextView(ih);
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        text_view = (NSTextView*)field_editor;
        break;
      }
    default:
      {
        return NULL;
        break;
      }
  }

  NSRange cursor_range = [[[text_view selectedRanges] lastObject] rangeValue];
  if(NSNotFound == cursor_range.location)
  {
    return NULL;
  }
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        [text_view scrollRangeToVisible:cursor_range];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
  case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        break;
      }
  default:
      {
        return NULL;
        break;
      }
  }
  return iupStrReturnInt((int)cursor_range.location);
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

static int cocoaTextSetCueBannerAttrib(Ihandle *ih, const char *value)
{
  NSString* ns_string;

  if(NULL == value)
  {
    ns_string = @"";
  }
  else
  {
    ns_string = [NSString stringWithUTF8String:value];
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

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
        NSTextView* text_view = cocoaTextGetTextView(ih);
        [text_view setSelectable:is_active];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        [text_field setSelectable:is_active];
        [text_field setEnabled:is_active];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        [text_field setSelectable:is_active];
        [text_field setEnabled:is_active];

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
        is_active = [text_field isSelectable];
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSStepper* stepper_view = cocoaTextGetStepperView(ih);
        is_active = [text_field isSelectable] && [stepper_view isEnabled];
        break;
      }
    default:
      {
        break;
      }
  }

  return iupStrReturnBoolean(is_active);
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
      NSString* temp = [NSString stringWithUTF8String:value];
      ns_append_string = [@"\n" stringByAppendingString:temp];
    }
    else
    {
      ns_append_string = [NSString stringWithUTF8String:value];
    }

    NSRange change_range = NSMakeRange([text_storage length], 0);

    NSMutableDictionary* attribute_dict = [[iup_font attributeDictionary] mutableCopy];
    [attribute_dict autorelease];
    NSDictionary<NSAttributedStringKey, id>* text_storage_attributes = nil;
    if(change_range.location == 0)
    {
      text_storage_attributes = [iup_font attributeDictionary];
    }
    else
    {
      text_storage_attributes = [text_storage attributesAtIndex:change_range.location-1 effectiveRange:NULL];
    }

    [attribute_dict addEntriesFromDictionary:text_storage_attributes];

    NSAttributedString* attributed_append_string = [[NSAttributedString alloc] initWithString:ns_append_string attributes:attribute_dict];
    [attributed_append_string autorelease];

    NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
    [undo_manager beginUndoGrouping];

    ih->data->disable_callbacks = 1;
    [text_view shouldChangeTextInRange:change_range replacementString:[attributed_append_string string]];
    [text_storage beginEditing];

    [text_storage appendAttributedString:attributed_append_string];

    ih->data->disable_callbacks = 0;

    [text_storage endEditing];
    [text_view didChangeText];
    [undo_manager endUndoGrouping];

    /* Scroll to end to show the appended text */
    NSRange end_range = NSMakeRange([[text_view string] length], 0);
    [text_view scrollRangeToVisible:end_range];
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

          NSString* ns_append_string = [NSString stringWithUTF8String:value];

          NSAttributedString* attributed_append_string = [[NSAttributedString alloc] initWithString:ns_append_string attributes:[iup_font attributeDictionary]];
          [attributed_append_string autorelease];

          [old_string_value appendAttributedString:attributed_append_string];

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

  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_VIEW:
      {
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

        NSString* ns_insert_string = [NSString stringWithUTF8String:value];

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
        break;
      }
    case IUPCOCOATEXTSUBTYPE_FIELD:
      {
        NSTextField* text_field = cocoaTextGetTextField(ih);
        NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        NSRange selected_range = [field_editor selectedRange];
        if(NSNotFound == selected_range.location)
        {
          return cocoaTextSetAppendAttrib(ih, value);
        }
        else if(selected_range.length > 0)
        {
          return cocoaTextSetSelectedTextAttrib(ih, value);
        }

        NSString* ns_string = [NSString stringWithUTF8String:value];
        NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
        [(NSTextView*)field_editor insertText:ns_string replacementRange:selected_range];
        return 0;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        break;
      }
    default:
      {
        break;
      }
  }
  return 0;
}

static int cocoaTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  ih->data->disable_callbacks = 1;

  NSView* the_view = nil;

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
        NSTextField* text_field = cocoaTextGetTextField(ih);
        the_view = text_field;
        break;
      }
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        the_view = text_field;
        break;
      }
    default:
      {
        break;
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
    /* Pasting is handled by the shouldChangeTextInRange: delegate methods. */
    /* Programmatically triggering paste: will be validated by the delegate. */
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

  NSUInteger glyph_count = cocoaTextCountGlyphsInString(text_string);
  return iupStrReturnInt((int)glyph_count);
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

  NSLayoutManager* layout_manager = [text_view layoutManager];
  NSUInteger number_of_lines;
  NSUInteger index;

  NSUInteger number_of_glyphs = [layout_manager numberOfGlyphs];
  NSRange line_range = NSMakeRange(0, 0);

  for(number_of_lines = 0, index = 0; index < number_of_glyphs;)
  {
    (void) [layout_manager lineFragmentRectForGlyphAtIndex:index effectiveRange:&line_range];
    index = NSMaxRange(line_range);
  }

  return iupStrReturnInt((int)number_of_lines);
}

/************************************** Begin SPIN *****************************************************/

static int cocoaTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOATEXTSUBTYPE_STEPPER:
      {
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);

        int int_value = 0;
        if(iupStrToInt(value, &int_value))
        {
          ih->data->disable_callbacks = 1;

          NSStepper* stepper_view = cocoaTextGetStepperView(ih);
          double max_value = [stepper_view maxValue];
          if((double)int_value > max_value)
          {
            int_value = (int)max_value;
          }
          double min_value = [stepper_view minValue];
          if((double)int_value < min_value)
          {
            int_value = (int)min_value;
          }

          if(iupAttribGetBoolean(ih, "SPINAUTO"))
          {
            NSNumber* number_to_set = [NSNumber numberWithInt:int_value];
            IUPStepperObject* stepper_object = cocoaTextGetStepperObject(ih);
            [stepper_object setStepperValue:number_to_set];
          }
          else
          {
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
        NSTextField* text_field = cocoaTextGetStepperTextField(ih);
        NSString* ns_string = [text_field stringValue];
        return iupStrReturnStr([ns_string UTF8String]);

        break;
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

static int cocoaTextMapMethod(Ihandle* ih)
{
  NSView* root_view = nil;
  NSView* main_view = nil;

  if (ih->data->is_multiline)
  {
    NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSZeroRect];
    NSSize scrollview_content_size = [scroll_view contentSize];
    NSTextView* text_view;

    if (iupAttribGetBoolean(ih, "BORDER"))
      [scroll_view setBorderType:NSBezelBorder];
    else
      [scroll_view setBorderType:NSNoBorder];

    [scroll_view setHasVerticalScroller:YES];
    [scroll_view setHasHorizontalScroller:YES];
    [scroll_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

    text_view = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0,
        scrollview_content_size.width, scrollview_content_size.height)];

    [text_view setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
    [text_view setVerticallyResizable:YES];
    [text_view setHorizontallyResizable:YES];
    [text_view setAutoresizingMask:NSViewWidthSizable];

    [text_view setEditable:!iupAttribGetBoolean(ih, "READONLY")];
    [text_view setSelectable:YES];
    [text_view setRichText:ih->data->has_formatting];
    [text_view setImportsGraphics:NO];
    [text_view setUsesFindPanel:YES];
    [text_view setAllowsUndo:YES];
    [text_view setUsesRuler:NO];

    /* Set text container inset - NSSize(width, height) where width=left+right, height=top+bottom
     * Use 4.0 for width (2px left + 2px right) and 2.0 for height (1px top + 1px bottom) */
    [text_view setTextContainerInset:NSMakeSize(4.0, 2.0)];

    NSTextContainer* text_container = [text_view textContainer];
    [text_container setContainerSize:NSMakeSize(scrollview_content_size.width, FLT_MAX)];
    [text_container setWidthTracksTextView:YES];

    [scroll_view setDocumentView:text_view];

    if (iupAttribGetBoolean(ih, "WORDWRAP"))
    {
      ih->data->sb &= ~IUP_SB_HORIZ;

      [scroll_view setHasHorizontalScroller:NO];
      [text_view setHorizontallyResizable:NO];
      [text_container setContainerSize:NSMakeSize(scrollview_content_size.width, FLT_MAX)];
      [text_container setWidthTracksTextView:YES];
      [text_container setLineFragmentPadding:0.0];
    }
    else
    {
      [scroll_view setHasHorizontalScroller:YES];
      [text_view setHorizontallyResizable:YES];
      [text_container setContainerSize:NSMakeSize(FLT_MAX, FLT_MAX)];
      [text_container setWidthTracksTextView:NO];
      [text_container setLineFragmentPadding:0.0];
    }

    if (ih->data->sb & IUP_SB_HORIZ)
      [scroll_view setHasHorizontalScroller:YES];
    else
      [scroll_view setHasHorizontalScroller:NO];

    if (!(ih->data->sb & IUP_SB_VERT))
      [scroll_view setHasVerticalScroller:NO];

    IupCocoaFont* iup_font = iupcocoaGetFont(ih);
    if (iup_font)
    {
      [text_view setFont:[iup_font nativeFont]];
      [text_view setTypingAttributes:[iup_font attributeDictionary]];
    }

    [text_view setTextColor:[NSColor textColor]];
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
    NSBundle* framework_bundle = [NSBundle bundleWithIdentifier:@"br.puc-rio.tecgraf.iup"];
    NSNib* text_spinner_nib = nil;
    if(!iupAttribGetBoolean(ih, "SPINAUTO"))
    {
      text_spinner_nib = [NSNib IupTextSpinnerNoBindings];
    }
    else
    {
      text_spinner_nib = [NSNib IupTextSpinner];
    }

    NSArray* top_level_objects = nil;
    IUPTextSpinnerFilesOwner* files_owner = [[IUPTextSpinnerFilesOwner alloc] init];
    IUPTextSpinnerContainer* text_spinner_container = [[IUPTextSpinnerContainer alloc] init];

    [text_spinner_nib instantiateWithOwner:files_owner topLevelObjects:&top_level_objects];

    NSStackView* stack_view = [files_owner stackView];
    NSStepper* stepper_view = [files_owner stepperView];
    NSTextField* text_field = [files_owner textField];
    IUPStepperObject* stepper_object = [files_owner stepperObject];
    IUPStepperObjectController* stepper_object_controller = [files_owner stepperObjectController];

    [text_spinner_container setStepperView:stepper_view];
    [text_spinner_container setTextField:text_field];
    [text_spinner_container setStepperObject:stepper_object];
    [text_spinner_container setStepperObjectController:stepper_object_controller];

    {
      /* Formatter will be set later */
    }

    objc_setAssociatedObject(stack_view, IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY, (id)text_spinner_container, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

    root_view = [stack_view retain];
    main_view = text_field;

    [text_spinner_container release];
    [files_owner release];
    [text_spinner_nib release];

    IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
    [number_formatter autorelease];
    [number_formatter setIhandle:ih];
    [number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
    [number_formatter setPartialStringValidationEnabled:YES];
    [number_formatter setNumberStyle:NSNumberFormatterNoStyle];
    [text_field setFormatter:number_formatter];

    [stepper_view setValueWraps:(BOOL)iupAttribGetBoolean(ih, "SPINWRAP")];

    IupCocoaTextSpinnerDelegate* text_spinner_delegate = [[IupCocoaTextSpinnerDelegate alloc] init];
    [text_field setDelegate:text_spinner_delegate];
    [text_spinner_delegate setIhandle:ih];

    [stepper_view setTarget:text_spinner_delegate];
    [stepper_view setAction:@selector(mySpinnerClickAction:)];

    objc_setAssociatedObject(stepper_view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
    objc_setAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
    objc_setAssociatedObject(text_field, IUP_COCOA_TEXT_DELEGATE_OBJ_KEY, (id)text_spinner_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [text_spinner_delegate release];

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

  IupCocoaFont* iup_font = iupcocoaGetFont(ih);
  if (iup_font && [main_view respondsToSelector:@selector(setFont:)])
  {
    [(id)main_view setFont:[iup_font nativeFont]];
  }

  if (iupAttribGet(ih, "BGCOLOR"))
    cocoaTextSetBgColorAttrib(ih, iupAttribGetStr(ih, "BGCOLOR"));
  if (iupAttribGet(ih, "FGCOLOR"))
    cocoaTextSetFgColorAttrib(ih, iupAttribGetStr(ih, "FGCOLOR"));
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

  return IUP_NOERROR;
}

static void cocoaTextUnMapMethod(Ihandle* ih)
{
  id the_view = ih->handle;

  {
    Ihandle* context_menu_ih = (Ihandle*)iupcocoaCommonBaseGetContextMenuAttrib(ih);
    if(NULL != context_menu_ih)
    {
      IupDestroy(context_menu_ih);
    }
    iupcocoaCommonBaseSetContextMenuAttrib(ih, NULL);
  }

  iupcocoaRemoveFromParent(ih);
  iupcocoaSetAssociatedViews(ih, nil, nil);
  [the_view release];
  ih->handle = NULL;
}

static void cocoaTextComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int natural_w = 0,
      natural_h = 0,
      visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS"),
      visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  (void)children_expand;
  int single_char_width;

  iupdrvFontGetCharSize(ih, NULL, &natural_h);
  natural_w = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");

  single_char_width = natural_w / 10;

  natural_w = (visiblecolumns*natural_w)/10;

  natural_w += (single_char_width * 2);

  if (ih->data->is_multiline)
  {
    natural_h = visiblelines*natural_h;
  }

  if (iupAttribGetBoolean(ih, "BORDER"))
    iupdrvTextAddBorders(ih, &natural_w, &natural_h);

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

void iupdrvTextInitClass(Iclass* ic)
{
  ic->Map = cocoaTextMapMethod;
  ic->UnMap = cocoaTextUnMapMethod;
  ic->ComputeNaturalSize = cocoaTextComputeNaturalSizeMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "ACTIVE", cocoaTextGetActiveAttrib, cocoaTextSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", cocoaTextGetValueAttrib, cocoaTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LINEVALUE", cocoaTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SELECTEDTEXT", cocoaTextGetSelectedTextAttrib, cocoaTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", cocoaTextGetSelectionAttrib, cocoaTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", cocoaTextGetSelectionPosAttrib, cocoaTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", cocoaTextGetCaretAttrib, cocoaTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", cocoaTextGetCaretPosAttrib, cocoaTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "INSERT", NULL, cocoaTextSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, cocoaTextSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "READONLY", cocoaTextGetReadOnlyAttrib, cocoaTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, cocoaTextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, cocoaTextSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, cocoaTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, cocoaTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SPINMIN", NULL, cocoaTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, cocoaTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, cocoaTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", cocoaTextGetSpinValueAttrib, cocoaTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COUNT", cocoaTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LINECOUNT", cocoaTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", cocoaTextGetFormattingAttrib, cocoaTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, cocoaTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, cocoaTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, cocoaTextSetTabSizeAttrib, "8", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, cocoaTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FILTER", NULL, cocoaTextSetFilterAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", cocoaTextGetScrollVisibleAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", cocoaTextGetOverwriteAttrib, cocoaTextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTEXTMENU", iupcocoaCommonBaseGetContextMenuAttrib, cocoaTextSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}
