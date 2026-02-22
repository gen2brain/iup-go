/** \file
 * \brief List Control
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
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_list.h"
#include "iup_class.h"

#include "iupcocoa_drv.h"


/* Pasteboard type for internal list reordering (SHOWDRAGDROP=YES). */
/* It must be a valid UTI string, preferably in reverse-DNS format. */
static NSPasteboardType const IupListPasteboardType = @"br.puc-rio.tecgraf.iup.list";

/* Pasteboard type for cross-list DND (DRAGDROPLIST=YES), carrying Ihandle pointer. */
static NSPasteboardType const IupInternalDndType = @"br.puc-rio.tecgraf.iup.dnd.internal.handle";

static const CGFloat kIupCocoaDefaultWidthNSPopUpButton = 100.0;
static const CGFloat kIupCocoaDefaultHeightNSPopUpButton = 26.0;
static const CGFloat kIupCocoaDefaultWidthNSComboBox = 100.0;
static const CGFloat kIupCocoaDefaultHeightNSComboBox = 26.0;
static const CGFloat kIupCocoaMinColumnWidth = 50.0;
static const CGFloat kIupCocoaListDefaultPadding = 0.0;

static const void* IUP_COCOA_LIST_POPUPBUTTON_RECEIVER_OBJ_KEY = @"IUP_COCOA_LIST_POPUPBUTTON_RECEIVER_OBJ_KEY";
static const void* IUP_COCOA_LIST_DELEGATE_OBJ_KEY = @"IUP_COCOA_LIST_DELEGATE_OBJ_KEY";
static const void* IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY = @"IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY";

/* Shared row height for lists */
static CGFloat s_cocoa_measured_row_height = -1.0;

/* Forward declarations */
static char* cocoaListGetValueAttrib(Ihandle* ih);
static int cocoaListSetValueAttrib(Ihandle* ih, const char* value);
static void cocoaListUpdateDragDrop(Ihandle* ih);

@interface IupCocoaListTableViewReceiver : NSObject <NSTableViewDelegate, NSTableViewDataSource>
{
  NSMutableArray* dataArray;
}
@property (nonatomic, retain) NSMutableArray* dataArray;
- (NSInteger) numberOfRowsInTableView:(NSTableView*)table_view;
- (NSView*) tableView:(NSTableView*)table_view viewForTableColumn:(NSTableColumn*)table_column row:(NSInteger)the_row;
- (void) tableViewSelectionDidChange:(NSNotification*)the_notification;
- (CGFloat) tableView:(NSTableView*)table_view heightOfRow:(NSInteger)row;
- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row;
- (NSDragOperation)tableView:(NSTableView *)tableView validateDrop:(id <NSDraggingInfo>)info proposedRow:(NSInteger)row proposedDropOperation:(NSTableViewDropOperation)dropOperation;
- (BOOL)tableView:(NSTableView *)tableView acceptDrop:(id <NSDraggingInfo>)info row:(NSInteger)row dropOperation:(NSTableViewDropOperation)dropOperation;
- (void)tableView:(NSTableView *)tableView draggingSession:(NSDraggingSession *)session willBeginAtPoint:(NSPoint)screenPoint forRowIndexes:(NSIndexSet *)rowIndexes;
- (void)tableView:(NSTableView *)tableView draggingSession:(NSDraggingSession *)session endedAtPoint:(NSPoint)screenPoint operation:(NSDragOperation)operation;
- (void)listDoubleClickAction:(id)sender;
@end

typedef enum
{
  IUPCOCOALISTSUBTYPE_UNKNOWN = -1,
  IUPCOCOALISTSUBTYPE_DROPDOWN,
  IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN,
  IUPCOCOALISTSUBTYPE_EDITBOX,
  IUPCOCOALISTSUBTYPE_MULTIPLELIST,
  IUPCOCOALISTSUBTYPE_SINGLELIST
} IupCocoaListSubType;

static IupCocoaListSubType cocoaListGetSubType(Ihandle* ih)
{
  if (ih->data->is_dropdown)
  {
    if (ih->data->has_editbox)
      return IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN;
    else
      return IUPCOCOALISTSUBTYPE_DROPDOWN;
  }
  else
  {
    if (ih->data->has_editbox)
      return IUPCOCOALISTSUBTYPE_EDITBOX;
    else if (ih->data->is_multiple)
      return IUPCOCOALISTSUBTYPE_MULTIPLELIST;
    else
      return IUPCOCOALISTSUBTYPE_SINGLELIST;
  }
  return IUPCOCOALISTSUBTYPE_UNKNOWN;
}

static NSView* cocoaListGetBaseWidget(Ihandle* ih)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSView* root_widget = (NSView*)ih->handle;

  if (nil == root_widget)
    return nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        NSCAssert([root_widget isKindOfClass:[NSPopUpButton class]], @"Expecting a NSPopUpButton");
        return root_widget;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSCAssert([root_widget isKindOfClass:[NSComboBox class]], @"Expecting a NSComboBox");
        return root_widget;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        NSTableView* table_view = (NSTableView*)iupAttribGet(ih, "_IUPCOCOA_TABLEVIEW");
        return table_view;
      }
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
    case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        NSTableView* table_view = (NSTableView*)iupAttribGet(ih, "_IUPCOCOA_TABLEVIEW");
        if (table_view)
          return table_view;

        if ([root_widget isKindOfClass:[NSTableView class]])
          return root_widget;
        else if ([root_widget isKindOfClass:[NSScrollView class]])
        {
          NSView* base_widget = [(NSScrollView*)root_widget documentView];
          return base_widget;
        }
        else
        {
          NSCAssert(0, @"Unexpected subtype");
          return root_widget;
        }
      }
    default:
      {
        NSCAssert(0, @"Unexpected subtype");
        return root_widget;
      }
  }
}

static int cocoaListGetMaxWidth(Ihandle* ih)
{
  int max_width = 0;

  if (!ih->handle)
  {
    int count = 0;
    char* value;
    while ((value = iupAttribGetId(ih, "", count+1)) != NULL)
    {
      int width = iupdrvFontGetStringWidth(ih, value);
      if (width > max_width)
        max_width = width;
      count++;
    }
    return max_width;
  }

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
        if (popup_button)
        {
          NSMenu* menu = [popup_button menu];

          for (NSMenuItem* item in [menu itemArray])
          {
            NSString* title = [item title];
            if (title)
            {
              const char* c_str = [title UTF8String];
              int width = iupdrvFontGetStringWidth(ih, c_str);
              if (width > max_width)
                max_width = width;
            }
          }
        }
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
        if (combo_box)
        {
          NSInteger count = [combo_box numberOfItems];

          for (NSInteger i = 0; i < count; i++)
          {
            id obj = [combo_box itemObjectValueAtIndex:i];
            NSString* str = nil;
            if ([obj isKindOfClass:[NSString class]])
              str = (NSString*)obj;
            else if ([obj respondsToSelector:@selector(stringValue)])
              str = [obj stringValue];

            if (str)
            {
              const char* c_str = [str UTF8String];
              int width = iupdrvFontGetStringWidth(ih, c_str);
              if (width > max_width)
                max_width = width;
            }
          }
        }
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
    case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
        if (table_view)
        {
          IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
          if(list_receiver)
          {
            NSMutableArray* data_array = [list_receiver dataArray];

            for (NSDictionary* item_data in data_array)
            {
              NSString* str = [item_data objectForKey:@"text"];
              if (str)
              {
                const char* c_str = [str UTF8String];
                int width = iupdrvFontGetStringWidth(ih, c_str);
                if (width > max_width)
                  max_width = width;
              }
            }
          }
        }
        break;
      }
    default:
      break;
  }

  return max_width;
}

static NSFont* cocoaGetNativeFont(Ihandle* ih)
{
  IupCocoaFont *iup_font = NULL;

  if (ih)
  {
    iup_font = iupcocoaGetFont(ih);
  }

  if (!iup_font)
  {
    /* Fallback to DEFAULTFONT */
    const char* default_font = IupGetGlobal("DEFAULTFONT");
    iup_font = iupcocoaFindFont(default_font);
  }

  return iup_font ? [iup_font nativeFont] : nil;
}

static void cocoaListCaretNotification(NSNotification* notification, Ihandle* ih)
{
  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  NSTextField* text_field = [notification object];
  NSText* field_editor = [[text_field window] fieldEditor:NO forObject:text_field];

  if (field_editor)
  {
    NSRange range = [field_editor selectedRange];
    int pos = (int)range.location;
    int lin = 1;

    if (pos != ih->data->last_caret_pos)
    {
      ih->data->last_caret_pos = pos;
      cb(ih, lin, pos + 1, pos);
    }
  }
}

static CGFloat cocoaListGetScrollbarSize(void)
{
  /* Get the width of a standard vertical scrollbar */
  NSScroller* scroller = [[NSScroller alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
  [scroller setScrollerStyle:NSScrollerStyleLegacy];
  CGFloat width = [NSScroller scrollerWidthForControlSize:NSControlSizeRegular
                                             scrollerStyle:[scroller scrollerStyle]];
  [scroller release];

  if (width < 15.0)
    width = 15.0;

  return width;
}

static void cocoaListUpdateDropExpand(Ihandle* ih)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  if (!ih->data->is_dropdown)
    return;

  if (!iupAttribGetBoolean(ih, "DROPEXPAND"))
    return;

  if (sub_type == IUPCOCOALISTSUBTYPE_DROPDOWN)
  {
    NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
    if (!popup_button) return;

    int max_text_width = cocoaListGetMaxWidth(ih);
    CGFloat scrollbar_width = cocoaListGetScrollbarSize();
    CGFloat button_width = 22.0;
    CGFloat padding = 6.0;
    CGFloat required_width = padding + (CGFloat)max_text_width + button_width + scrollbar_width + padding;

    [[popup_button menu] setMinimumWidth:required_width];
  }
  else if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN)
  {
    /* NSComboBox dropdown width is controlled by the combo box frame width.
       This is a limitation of the Cocoa NSComboBox API. */
  }
}

static void cocoaListUpdateColumnWidth(Ihandle* ih)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  if (sub_type == IUPCOCOALISTSUBTYPE_MULTIPLELIST ||
      sub_type == IUPCOCOALISTSUBTYPE_SINGLELIST ||
      sub_type == IUPCOCOALISTSUBTYPE_EDITBOX)
  {
    NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
    if (!table_view) return;

    NSArray* columns = [table_view tableColumns];
    if ([columns count] == 0) return;

    NSTableColumn* column = [columns firstObject];
    int max_text_width = cocoaListGetMaxWidth(ih);

    CGFloat padding = kIupCocoaListDefaultPadding * 2;
    if (ih->data->spacing > 0)
      padding = (CGFloat)(ih->data->spacing * 2);

    CGFloat required_width = (CGFloat)max_text_width + padding;

    if (ih->data->show_image && ih->data->maximg_w > 0)
      required_width += ih->data->maximg_w + 4;

    if (ih->data->is_multiple)
      required_width += 20;

    if (required_width < kIupCocoaMinColumnWidth)
      required_width = kIupCocoaMinColumnWidth;

    [column setWidth:required_width];
    [table_view sizeToFit];
    [table_view setNeedsDisplay:YES];
  }
}

static void cocoaListCallCaretCbForTextView(Ihandle* ih, NSTextView* textView)
{
  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  NSRange range = [textView selectedRange];
  int pos = (int)range.location;
  int lin = 1;

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;
    cb(ih, lin, pos + 1, pos);
  }
}

@interface IupCocoaListTextField : NSTextField
@end

@implementation IupCocoaListTextField
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
  if (result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
    if (ih)
      iupcocoaFocusIn(ih);
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
      iupcocoaFocusOut(ih);
  }
  return result;
}

- (void)keyDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    int mac_key_code = [event keyCode];
    /* iupcocoaKeyEvent will internally bubble the event up to the dialog if needed. */
    BOOL handled = iupcocoaKeyEvent(ih, event, mac_key_code, true);

    if (!handled)
    {
      [super keyDown:event];
    }
  }
  else
  {
    [super keyDown:event];
  }
}

- (void)keyUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    int mac_key_code = [event keyCode];
    BOOL handled = iupcocoaKeyEvent(ih, event, mac_key_code, false);

    if (!handled)
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
    BOOL handled = iupcocoaModifierEvent(ih, event, mac_key_code);

    if (!handled)
      [super flagsChanged:event];
  }
  else
    [super flagsChanged:event];
}

@end

/* Mouse button helper for list controls.
   IUP_DEFAULT propagates the event so the widget handles selection normally, IUP_IGNORE consumes it. */
static BOOL cocoaListHandleMouseButton(Ihandle* ih, NSEvent* the_event, NSView* represented_view, bool is_pressed)
{
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (!cb)
    return YES;

  NSPoint the_point = [the_event locationInWindow];
  NSPoint converted_point = [represented_view convertPoint:the_point fromView:nil];
  CGFloat final_y = converted_point.y;

  if (![represented_view isFlipped])
  {
    NSRect view_bounds = [represented_view bounds];
    final_y = view_bounds.size.height - converted_point.y;
  }

  NSInteger which_cocoa_button = [the_event buttonNumber];
  char mod_status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  iupcocoaButtonKeySetStatus(the_event, mod_status);

  if ([the_event modifierFlags] & NSEventModifierFlagControl && 0 == which_cocoa_button)
    which_cocoa_button = 1;

  int which_iup_button = iupcocoaCommonBaseIupButtonForCocoaButton(which_cocoa_button);

  int ret = cb(ih, which_iup_button, is_pressed, iupROUND(converted_point.x), iupROUND(final_y), mod_status);
  if (ret == IUP_CLOSE)
  {
    IupExitLoop();
    return NO;
  }
  else if (ret == IUP_IGNORE)
    return NO;

  return YES;
}

@interface IupCocoaListTableView : NSTableView
@end

@implementation IupCocoaListTableView

- (NSMenu *)menuForEvent:(NSEvent *)event
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
  return [super menuForEvent:event];
}

- (BOOL)acceptsFirstResponder
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (ih)
  {
    BOOL canFocus;
    if (iupAttribGet(ih, "_IUPCOCOA_CANFOCUS"))
    {
      canFocus = iupAttribGetBoolean(ih, "_IUPCOCOA_CANFOCUS");
    }
    else
    {
      canFocus = iupAttribGetBoolean(ih, "CANFOCUS");
    }
    return canFocus;
  }

  BOOL result = [super acceptsFirstResponder];
  return result;
}

- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if (result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);

    if (ih)
    {
      iupcocoaFocusIn(ih);
    }
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
    {
      iupcocoaFocusOut(ih);
    }
  }
  return result;
}

- (void)keyDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    int mac_key_code = [event keyCode];
    BOOL handled = iupcocoaKeyEvent(ih, event, mac_key_code, true);
    if (!handled)
    {
      [super keyDown:event];
    }
  }
  else
  {
    [super keyDown:event];
  }
}

- (void)keyUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    int mac_key_code = [event keyCode];
    BOOL handled = iupcocoaKeyEvent(ih, event, mac_key_code, false);
    if (!handled)
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
    BOOL handled = iupcocoaModifierEvent(ih, event, mac_key_code);
    if (!handled)
      [super flagsChanged:event];
  }
  else
    [super flagsChanged:event];
}

- (void)mouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, true))
    [super mouseDown:event];
}

- (void)mouseUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, false))
    [super mouseUp:event];
}

- (void)rightMouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, true))
    [super rightMouseDown:event];
}

- (void)rightMouseUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, false))
    [super rightMouseUp:event];
}

- (void)otherMouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, true))
    [super otherMouseDown:event];
}

- (void)otherMouseUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, false))
    [super otherMouseUp:event];
}

@end

@interface IupCocoaComboBox : NSComboBox
- (NSMenu *)textView:(NSTextView *)text_view menu:(NSMenu *)the_menu forEvent:(NSEvent *)the_event atIndex:(NSUInteger)char_index;
@end

@implementation IupCocoaComboBox
- (NSMenu *)textView:(NSTextView *)text_view menu:(NSMenu *)the_menu forEvent:(NSEvent *)the_event atIndex:(NSUInteger)char_index
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);

  /* Check if the user has ever configured the CONTEXTMENU attribute. */
  if (!iupAttribGet(ih, "_IUPCOCOA_CONTEXTMENU_SET"))
  {
    /* If not, allow the default system menu to appear. */
    return the_menu;
  }

  /* The attribute has been configured. Check its value. */
  Ihandle* menu_ih = (Ihandle*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");

  if(menu_ih && menu_ih->handle)
  {
    /* A valid custom menu is set; return it. */
    return (NSMenu*)menu_ih->handle;
  }
  else
  {
    /* The attribute was set to NULL; disable the context menu. */
    return nil;
  }
}

- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if (result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
    if (ih)
      iupcocoaFocusIn(ih);
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
      iupcocoaFocusOut(ih);
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

- (void)keyDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    int mac_key_code = [event keyCode];
    /* iupcocoaKeyEvent will internally bubble the event up to the dialog if needed. */
    BOOL handled = iupcocoaKeyEvent(ih, event, mac_key_code, true);

    if (!handled)
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
    if (!iupcocoaKeyEvent(ih, event, mac_key_code, false))
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
    if (!iupcocoaModifierEvent(ih, event, mac_key_code))
      [super flagsChanged:event];
  }
  else
    [super flagsChanged:event];
}

- (void)mouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, true))
    [super mouseDown:event];
}

- (void)mouseUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, false))
    [super mouseUp:event];
}

- (void)rightMouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, true))
    [super rightMouseDown:event];
}

- (void)rightMouseUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, false))
    [super rightMouseUp:event];
}

- (void)otherMouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, true))
    [super otherMouseDown:event];
}

- (void)otherMouseUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, false))
    [super otherMouseUp:event];
}

@end

@interface IupCocoaListPopUpButton : NSPopUpButton
@end

@implementation IupCocoaListPopUpButton

- (void)mouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, true))
    [super mouseDown:event];
}

- (void)mouseUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, false))
    [super mouseUp:event];
}

- (void)rightMouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, true))
    [super rightMouseDown:event];
}

- (void)rightMouseUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, false))
    [super rightMouseUp:event];
}

- (void)otherMouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, true))
    [super otherMouseDown:event];
}

- (void)otherMouseUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih && cocoaListHandleMouseButton(ih, event, self, false))
    [super otherMouseUp:event];
}

@end

@interface IupCocoaListPopupButtonReceiver : NSObject
            - (IBAction) onSelectionChanged:(id)the_sender;
            @end

            @implementation IupCocoaListPopupButtonReceiver

            - (IBAction) onSelectionChanged:(id)the_sender;
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
        if (cb)
        {
          NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
          NSInteger index_of_selected_item = [popup_button indexOfSelectedItem];
          int adjusted_index = (int)(index_of_selected_item+1);
          iupListSingleCallActionCb(ih, cb, adjusted_index);
        }
        iupBaseCallValueChangedCb(ih);
        break;
      }
    default:
      break;
  }
}

@end

@interface IupCocoaListDelegate : NSObject <NSComboBoxDelegate, NSTextFieldDelegate, NSTextViewDelegate>
- (void) comboBoxSelectionDidChange:(NSNotification*)the_notification;
- (void) controlTextDidChange:(NSNotification*)the_notification;
- (void) controlTextDidEndEditing:(NSNotification*)the_notification;
- (void) controlTextDidBeginEditing:(NSNotification*)the_notification;
- (BOOL) control:(NSControl*)control textView:(NSTextView*)textView doCommandBySelector:(SEL)commandSelector;
- (BOOL) textView:(NSTextView *)textView shouldChangeTextInRange:(NSRange)affectedCharRange replacementString:(NSString *)replacementString;
@end

@implementation IupCocoaListDelegate

- (void) comboBoxSelectionDidChange:(NSNotification*)the_notification
{
  NSComboBox* combo_box = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(combo_box, IHANDLE_ASSOCIATED_OBJ_KEY);

  IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    NSInteger index_of_selected_item = [combo_box indexOfSelectedItem];
    int adjusted_index = (int)(index_of_selected_item+1);
    iupListSingleCallActionCb(ih, cb, adjusted_index);
  }
  iupBaseCallValueChangedCb(ih);
}

- (void) controlTextDidChange:(NSNotification*)the_notification
{
  id text_obj = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_obj, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  const char* filter = iupAttribGet(ih, "FILTER");
  if(filter)
  {
    NSTextField* text_field = (NSTextField*)text_obj;
    NSString* current_text = [text_field stringValue];
    NSString* modified_text = nil;

    if (iupStrEqualNoCase(filter, "LOWERCASE"))
    {
      modified_text = [current_text lowercaseString];
    }
    else if (iupStrEqualNoCase(filter, "UPPERCASE"))
    {
      modified_text = [current_text uppercaseString];
    }
    else if (iupStrEqualNoCase(filter, "NUMBER"))
    {
      NSCharacterSet* non_digits = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
      modified_text = [[current_text componentsSeparatedByCharactersInSet:non_digits] componentsJoinedByString:@""];
    }

    if (modified_text && ![modified_text isEqualToString:current_text])
    {
      NSText* field_editor = [[text_field window] fieldEditor:NO forObject:text_field];
      NSRange selected_range = [field_editor selectedRange];

      [text_field setStringValue:modified_text];

      if(selected_range.location > [modified_text length])
      {
        selected_range.location = [modified_text length];
      }
      [field_editor setSelectedRange:selected_range];
    }
  }

  if (ih->data->nc > 0)
  {
    NSTextField* text_field = (NSTextField*)text_obj;
    NSString* current_text = [text_field stringValue];

    if ([current_text length] > ih->data->nc)
    {
      NSString* truncated_text = [current_text substringToIndex:ih->data->nc];
      NSText* field_editor = [[text_field window] fieldEditor:NO forObject:text_field];
      NSRange selected_range = [field_editor selectedRange];

      [text_field setStringValue:truncated_text];

      if(selected_range.location > [truncated_text length])
      {
        selected_range.location = [truncated_text length];
      }
      [field_editor setSelectedRange:selected_range];
    }
  }

  cocoaListCaretNotification(the_notification, ih);
  iupBaseCallValueChangedCb(ih);
}

- (void) controlTextDidEndEditing:(NSNotification*)the_notification
{
  id text_obj = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_obj, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  cocoaListCaretNotification(the_notification, ih);
}

- (void) controlTextDidBeginEditing:(NSNotification*)the_notification
{
  id text_obj = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_obj, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  IFn cb = (IFn)IupGetCallback(ih, "EDIT_CB");
  if (!cb) return;

  NSText *fieldEditor = [[text_obj window] fieldEditor:YES forObject:text_obj];
  if ([fieldEditor isKindOfClass:[NSTextView class]])
  {
    [(NSTextView *)fieldEditor setDelegate:self];
  }
}

- (void)textViewDidChangeSelection:(NSNotification *)notification
{
  NSTextView *textView = [notification object];

  NSControl *control = nil;
  for (NSView* view = [textView superview]; view != nil; view = [view superview])
  {
    if ([view isKindOfClass:[NSControl class]])
    {
      control = (NSControl*)view;
      break;
    }
  }

  if (!control) return;

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(control, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  cocoaListCallCaretCbForTextView(ih, textView);
}

- (BOOL) control:(NSControl*)control textView:(NSTextView*)textView doCommandBySelector:(SEL)commandSelector
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(control, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return NO;

  /* Give IUP key handler first chance at any command key (arrows, page up/down, delete, etc.) */
  /* This allows K_ANY callbacks to intercept any key, not just text input keys */
  NSEvent* current_event = [NSApp currentEvent];
  if (current_event && [current_event type] == NSEventTypeKeyDown)
  {
    int mac_key_code = [current_event keyCode];

    /* First, let the control itself try to handle the key event. */
    BOOL handled = iupcocoaKeyEvent(ih, current_event, mac_key_code, true);
    if (handled)
      return YES; /* Returning YES prevents the default command. */

    /* If not handled, allow the parent dialog's K_ANY to intercept it. */
    /* This is needed because some controls (like NSTextView) consume navigation */
    /* keys and do not propagate them up the responder chain, preventing the */
    /* dialog's window-level handler from ever seeing the event. */
    Ihandle* dialog_ih = IupGetDialog(ih);
    if (dialog_ih && dialog_ih != ih)
    {
      if (iupcocoaKeyEvent(dialog_ih, current_event, mac_key_code, true))
        return YES; /* Handled by dialog's K_ANY. Prevents default command. */
    }
  }

  /* If not handled by IUP, apply NC (max length) constraint for non-deletion commands */
  if (ih->data->nc > 0)
  {
    NSString* current_text = [textView string];
    if ([current_text length] >= ih->data->nc)
    {
      if (commandSelector != @selector(deleteBackward:) &&
          commandSelector != @selector(deleteForward:) &&
          commandSelector != @selector(deleteWordBackward:) &&
          commandSelector != @selector(deleteWordForward:) &&
          commandSelector != @selector(deleteToBeginningOfLine:) &&
          commandSelector != @selector(deleteToEndOfLine:))
      {
        NSRange selected_range = [textView selectedRange];
        if (selected_range.length == 0)
        {
          return YES;
        }
      }
    }
  }

  return NO;
}

- (BOOL) textView:(NSTextView *)textView shouldChangeTextInRange:(NSRange)affectedCharRange replacementString:(NSString *)replacementString
{
  NSControl* control = nil;

  for (NSView* view = [textView superview]; view != nil; view = [view superview])
  {
    if ([view isKindOfClass:[NSControl class]])
    {
      control = (NSControl*)view;
      break;
    }
  }

  if (!control) return YES;

  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(control, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return YES;

  IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
  if (!cb) return YES;

  if (affectedCharRange.length > 0 && [replacementString length] == 0)
  {
    int ret = iupEditCallActionCb(ih, cb, NULL, (int)affectedCharRange.location,
                                   (int)(affectedCharRange.location + affectedCharRange.length),
                                   ih->data->mask, ih->data->nc, 1, 1);
    if (ret == 0)
      return NO;
  }
  else if ([replacementString length] > 0)
  {
    const char* insert_value = [replacementString UTF8String];

    for (NSUInteger i = 0; i < [replacementString length]; i++)
    {
      unichar c = [replacementString characterAtIndex:i];
      char single_char[5];
      int len = 0;

      if (c < 0x80)
      {
        single_char[0] = (char)c;
        single_char[1] = 0;
        len = 1;
      }
      else
      {
        NSString* singleCharStr = [NSString stringWithCharacters:&c length:1];
        const char* utf8 = [singleCharStr UTF8String];
        strncpy(single_char, utf8, 4);
        single_char[4] = 0;
        len = (int)strlen(single_char);
      }

      int pos = (int)(affectedCharRange.location + i);
      int ret = iupEditCallActionCb(ih, cb, single_char, pos, pos,
                                     ih->data->mask, ih->data->nc, 0, 1);

      if (ret == 0)
        return NO;
      else if (ret > 0 && ret != (int)c)
      {
        NSString* replacement = [NSString stringWithFormat:@"%c", ret];
        [textView insertText:replacement replacementRange:NSMakeRange(pos, 0)];
        return NO;
      }
    }
  }

  return YES;
}

@end

@interface IupCocoaListTableCellView : NSTableCellView
@property (nonatomic, retain) NSColor* customBackgroundColor;
@property (nonatomic, retain) NSColor* customTextColor;
@end

@implementation IupCocoaListTableCellView
@synthesize customBackgroundColor;
@synthesize customTextColor;

- (instancetype) initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame:frameRect];
  if (self)
  {
    [self setWantsLayer:YES];

    /* Use standard NSImageView via superclass property.
       It must be set as non-editable so it does not interfere with dragging. */
    self.imageView = [[[NSImageView alloc] initWithFrame:NSZeroRect] autorelease];
    [self.imageView setImageFrameStyle:NSImageFrameNone];
    [self.imageView setImageAlignment:NSImageAlignCenter];
    [self.imageView setImageScaling:NSImageScaleProportionallyDown];
    [self.imageView setEditable:NO];
    [self.imageView setTranslatesAutoresizingMaskIntoConstraints:NO];
    [self addSubview:self.imageView];

    /* Use custom NSTextField with controlled text insets.
       It must be non-editable and non-selectable so it does not interfere with dragging. */
    NSTextField* textField = [[[IupCocoaListTextField alloc] initWithFrame:NSZeroRect] autorelease];
    [textField setBezeled:NO];
    [textField setDrawsBackground:NO];
    [textField setEditable:NO];
    [textField setSelectable:NO];
    [textField setLineBreakMode:NSLineBreakByClipping];
    [textField setTranslatesAutoresizingMaskIntoConstraints:NO];
    [self addSubview:textField];
    [self setTextField:textField];
  }
  return self;
}

- (void) dealloc
{
  [customBackgroundColor release];
  [customTextColor release];
  [super dealloc];
}

- (void)setBackgroundStyle:(NSBackgroundStyle)backgroundStyle
{
  [super setBackgroundStyle:backgroundStyle];

  if (backgroundStyle == NSBackgroundStyleNormal)
  {
    if (self.customBackgroundColor)
    {
      self.layer.backgroundColor = [self.customBackgroundColor CGColor];
    }
    else
    {
      self.layer.backgroundColor = [[NSColor clearColor] CGColor];
    }

    if (self.customTextColor)
    {
      [self.textField setTextColor:self.customTextColor];
    }
    else
    {
      [self.textField setTextColor:[NSColor controlTextColor]];
    }
  }
  else
  {
    self.layer.backgroundColor = [[NSColor clearColor] CGColor]; /* Use system selection color */
    [self.textField setTextColor:[NSColor selectedControlTextColor]];
  }
}

- (void) updateLayoutWithPadding:(CGFloat)padding showImage:(BOOL)showImage
{
  NSMutableArray* constraintsToRemove = [NSMutableArray array];
  for (NSLayoutConstraint* constraint in [self constraints])
  {
    if (constraint.firstItem == self.imageView || constraint.secondItem == self.imageView ||
        constraint.firstItem == [self textField] || constraint.secondItem == [self textField])
    {
      [constraintsToRemove addObject:constraint];
    }
  }
  [self removeConstraints:constraintsToRemove];

  NSDictionary* views = @{@"imageView": self.imageView, @"textField": [self textField]};
  NSDictionary* metrics = @{@"padding": @(padding)};

  if (showImage && ![self.imageView isHidden])
  {
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-padding-[imageView(16)]-4-[textField]-padding-|"
                 options:NSLayoutFormatAlignAllCenterY
                 metrics:metrics
                   views:views]];

    [self addConstraint:[NSLayoutConstraint constraintWithItem:self.imageView
              attribute:NSLayoutAttributeCenterY
              relatedBy:NSLayoutRelationEqual
                 toItem:self
              attribute:NSLayoutAttributeCenterY
             multiplier:1.0
               constant:0.0]];

    [self addConstraint:[NSLayoutConstraint constraintWithItem:self.imageView
              attribute:NSLayoutAttributeHeight
              relatedBy:NSLayoutRelationEqual
                 toItem:nil
              attribute:NSLayoutAttributeNotAnAttribute
             multiplier:1.0
               constant:16.0]];
  }
  else
  {
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-padding-[textField]-padding-|"
                 options:0
                 metrics:metrics
                   views:views]];
  }

  [self addConstraint:[NSLayoutConstraint constraintWithItem:[self textField]
            attribute:NSLayoutAttributeCenterY
            relatedBy:NSLayoutRelationEqual
               toItem:self
            attribute:NSLayoutAttributeCenterY
           multiplier:1.0
             constant:0.0]];
}
@end

@implementation IupCocoaListTableViewReceiver

@synthesize dataArray;

- (instancetype) init
{
  self = [super init];
  if (self)
  {
    dataArray = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void) dealloc
{
  [dataArray release];
  [super dealloc];
}

- (NSInteger) numberOfRowsInTableView:(NSTableView*)table_view
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(table_view, IHANDLE_ASSOCIATED_OBJ_KEY);

  /* Virtual mode: return item_count from ih->data */
  if (ih && ih->data && ih->data->is_virtual)
    return ih->data->item_count;

  return [dataArray count];
}

- (NSView*) tableView:(NSTableView*)table_view viewForTableColumn:(NSTableColumn*)table_column row:(NSInteger)the_row
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(table_view, IHANDLE_ASSOCIATED_OBJ_KEY);

  NSString* string_item = nil;
  NSImage* image_item = nil;

  /* Virtual mode: fetch from VALUE_CB and IMAGE_CB */
  if (ih && ih->data && ih->data->is_virtual)
  {
    if (the_row < 0 || the_row >= ih->data->item_count)
      return nil;

    char* text = iupListGetItemValueCb(ih, (int)the_row + 1);  /* 1-based */
    string_item = [NSString stringWithUTF8String:(text ? text : "")];

    /* Query IMAGE_CB for the image if SHOWIMAGE is enabled */
    if (ih->data->show_image)
    {
      char* image_name = iupListGetItemImageCb(ih, (int)the_row + 1);  /* 1-based */
      if (image_name)
      {
        void* handle = iupImageGetImage(image_name, ih, 0, NULL);
        if (handle)
          image_item = (NSImage*)handle;
      }
    }
  }
  else
  {
    if (the_row < 0 || the_row >= [dataArray count])
      return nil;

    NSDictionary* item_data = [dataArray objectAtIndex:the_row];
    string_item = [item_data objectForKey:@"text"];
    image_item = [item_data objectForKey:@"image"];
    if (image_item == (id)[NSNull null])
      image_item = nil;
  }

  IupCocoaListTableCellView* cell_view = [table_view makeViewWithIdentifier:@"IupCocoaListTableCellView" owner:self];

  if (nil == cell_view)
  {
    cell_view = [[[IupCocoaListTableCellView alloc] initWithFrame:NSZeroRect] autorelease];
    [cell_view setIdentifier:@"IupCocoaListTableCellView"];
    [cell_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  }

  [cell_view setCustomBackgroundColor:nil];
  [cell_view setCustomTextColor:nil];

  NSFont* font = nil;
  if (ih)
  {
    font = (NSFont*)cocoaGetNativeFont(ih);

    unsigned char r, g, b;
    char* color_str = iupAttribGet(ih, "BGCOLOR");
    if(iupStrToRGB(color_str, &r, &g, &b))
    {
      [cell_view setCustomBackgroundColor:[NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0]];
    }

    color_str = iupAttribGet(ih, "FGCOLOR");
    if(iupStrToRGB(color_str, &r, &g, &b))
    {
      [cell_view setCustomTextColor:[NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0]];
    }
  }
  if (!font)
  {
    const char* default_font = IupGetGlobal("DEFAULTFONT");
    IupCocoaFont* iup_font = iupcocoaFindFont(default_font);
    font = iup_font ? [iup_font nativeFont] : nil;
  }
  if (font)
  {
    [[cell_view textField] setFont:font];
  }

  [[cell_view textField] setStringValue:string_item];

  if (ih && ih->data->show_image && image_item)
  {
    /* Set scaling mode based on FITIMAGE attribute */
    if (ih->data->fit_image)
      [[cell_view imageView] setImageScaling:NSImageScaleProportionallyDown];
    else
      [[cell_view imageView] setImageScaling:NSImageScaleNone];

    [[cell_view imageView] setImage:image_item];
    [[cell_view imageView] setHidden:NO];
  }
  else
  {
    [[cell_view imageView] setImage:nil];
    [[cell_view imageView] setHidden:YES];
  }

  CGFloat padding = kIupCocoaListDefaultPadding;
  if (ih && ih->data && ih->data->spacing > 0)
  {
    padding = (CGFloat)ih->data->spacing;
  }

  [cell_view updateLayoutWithPadding:padding showImage:(ih && ih->data->show_image && image_item != nil)];

  return cell_view;
}

- (CGFloat) tableView:(NSTableView*)table_view heightOfRow:(NSInteger)row
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(table_view, IHANDLE_ASSOCIATED_OBJ_KEY);

  CGFloat row_height;
  NSFont* font = nil;
  if (ih)
    font = cocoaGetNativeFont(ih);
  if (!font)
  {
    const char* default_font = IupGetGlobal("DEFAULTFONT");
    IupCocoaFont* iup_font = iupcocoaFindFont(default_font);
    font = iup_font ? [iup_font nativeFont] : [NSFont systemFontOfSize:13];
  }

  int char_width, char_height;
  if (ih)
    iupdrvFontGetCharSize(ih, &char_width, &char_height);
  else
    char_height = 16;

  row_height = (CGFloat)char_height;

  int item_space = 0;
  iupdrvListAddItemSpace(ih, &item_space);
  row_height += (CGFloat)item_space;

  if (ih && ih->data && ih->data->spacing > 0)
    row_height += (CGFloat)(ih->data->spacing * 2);

  if (ih && ih->data && ih->data->show_image)
  {
    if (row >= 0 && row < [dataArray count])
    {
      NSDictionary* item_data = [dataArray objectAtIndex:row];
      NSImage* image = [item_data objectForKey:@"image"];
      if (image && image != (id)[NSNull null])
      {
        NSSize imageSize = [image size];
        CGFloat padding = (ih->data->spacing > 0) ? (CGFloat)(ih->data->spacing * 2) : 0;
        if (imageSize.height + padding > row_height)
          row_height = imageSize.height + padding;
      }
    }
  }

  return row_height;
}

- (void) tableViewSelectionDidChange:(NSNotification*)the_notification
{
  NSTableView* table_view = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(table_view, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOX)
  {
    NSInteger selected_row = [table_view selectedRow];
    if (selected_row >= 0)
    {
      if (selected_row < [dataArray count])
      {
        NSDictionary* item_data = [dataArray objectAtIndex:selected_row];
        NSString* selected_text = [item_data objectForKey:@"text"];

        NSTextField* text_field = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        if (text_field)
        {
          [text_field setStringValue:selected_text];
        }
      }
    }

    IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
    if (cb)
    {
      NSInteger index_of_selected_item = [table_view selectedRow];
      int adjusted_index = (int)(index_of_selected_item+1);
      iupListSingleCallActionCb(ih, cb, adjusted_index);
    }
    iupBaseCallValueChangedCb(ih);
  }
  else if (sub_type == IUPCOCOALISTSUBTYPE_SINGLELIST)
  {
    IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
    if (cb)
    {
      NSInteger index_of_selected_item = [table_view selectedRow];
      int adjusted_index = (int)(index_of_selected_item+1);
      iupListSingleCallActionCb(ih, cb, adjusted_index);
    }
    iupBaseCallValueChangedCb(ih);
  }
  else if (sub_type == IUPCOCOALISTSUBTYPE_MULTIPLELIST)
  {
    IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
    IFnsii cb = (IFnsii) IupGetCallback(ih, "ACTION");
    if (multi_cb || cb)
    {
      NSIndexSet* selected_indexes = [table_view selectedRowIndexes];
      NSUInteger count = [selected_indexes count];

      if (count == 0)
      {
        iupListMultipleCallActionCb(ih, cb, multi_cb, NULL, 0);
      }
      else
      {
        int* pos = malloc(sizeof(int)*count);
        if (pos)
        {
          NSUInteger idx = [selected_indexes firstIndex];
          int i = 0;
          while (idx != NSNotFound)
          {
            pos[i] = (int)idx;
            idx = [selected_indexes indexGreaterThanIndex:idx];
            i++;
          }
          iupListMultipleCallActionCb(ih, cb, multi_cb, pos, (int)count);
          free(pos);
        }
      }
    }
    iupBaseCallValueChangedCb(ih);
  }
}

- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(tableView, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    NSWindow* window = [tableView window];
    if (window && [window firstResponder] != tableView)
    {
      [window makeFirstResponder:tableView];
    }
  }
  return YES;
}

/* Drag Source */
- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(tableView, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return nil;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  BOOL enable_internal_dnd = ih->data->show_dragdrop && (sub_type == IUPCOCOALISTSUBTYPE_SINGLELIST);
  BOOL enable_crosslist_dnd = iupAttribGetBoolean(ih, "DRAGDROPLIST");

  if (!enable_internal_dnd && !enable_crosslist_dnd)
    return nil;

  NSPasteboardItem *pboardItem = [[[NSPasteboardItem alloc] init] autorelease];

  if (enable_internal_dnd)
  {
    NSString *rowIndexStr = [NSString stringWithFormat:@"%ld", (long)row];
    [pboardItem setString:rowIndexStr forType:IupListPasteboardType];
  }

  if (enable_crosslist_dnd)
  {
    int pos = (int)row + 1;

    if (ih->data->is_multiple)
    {
      char *buffer = cocoaListGetValueAttrib(ih);
      int count = iupdrvListGetCount(ih);

      if (buffer && row >= 0 && row < count)
      {
        size_t buffer_len = strlen(buffer);
        if (row < (NSInteger)buffer_len && buffer[row] == '-')
        {
          iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
          [tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];

          char* new_value = malloc(count + 1);
          if (new_value)
          {
            memset(new_value, '-', count);
            new_value[row] = '+';
            new_value[count] = '\0';
            iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", new_value);
            free(new_value);
          }
          iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
        }
      }
    }
    else
    {
      if ([tableView selectedRow] != row)
      {
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        [tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
        iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", pos);
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
      }
    }

    NSData *handleData = [NSData dataWithBytes:&ih length:sizeof(Ihandle*)];
    [pboardItem setData:handleData forType:IupInternalDndType];
  }

  if ([[pboardItem types] count] == 0)
    return nil;

  return pboardItem;
}

/* Drop Target - VALIDATION */
- (NSDragOperation)tableView:(NSTableView *)tableView validateDrop:(id <NSDraggingInfo>)info proposedRow:(NSInteger)row proposedDropOperation:(NSTableViewDropOperation)dropOperation
{
  if (dropOperation != NSTableViewDropAbove)
    return NSDragOperationNone;

  NSPasteboard *pboard = [info draggingPasteboard];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(tableView, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih)
    return NSDragOperationNone;

  NSEventModifierFlags flags = [NSEvent modifierFlags];
  int is_shift = (flags & NSEventModifierFlagShift) != 0;
  int is_copy = (flags & NSEventModifierFlagOption) != 0;

  if ([[pboard types] containsObject:IupListPasteboardType])
  {
    if ([info draggingSource] != tableView)
      return NSDragOperationNone;

    NSString *rowIndexStr = [pboard stringForType:IupListPasteboardType];
    if (!rowIndexStr) return NSDragOperationNone;

    NSInteger sourceRow = -1;
    if (![[NSScanner scannerWithString:rowIndexStr] scanInteger:&sourceRow])
      return NSDragOperationNone;

    if (sourceRow == row || sourceRow + 1 == row)
      return NSDragOperationNone;

    IFniiii cb = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");
    if (cb)
    {
      if (cb(ih, (int)sourceRow + 1, (int)row + 1, is_shift, is_copy) == IUP_IGNORE)
        return NSDragOperationNone;
    }

    [tableView setDropRow:row dropOperation:NSTableViewDropAbove];
    return is_copy ? NSDragOperationCopy : NSDragOperationMove;
  }

  if ([[pboard types] containsObject:IupInternalDndType])
  {
    NSData *handleData = [pboard dataForType:IupInternalDndType];
    if (!handleData || [handleData length] != sizeof(Ihandle*))
      return NSDragOperationNone;

    Ihandle* ih_source;
    memcpy(&ih_source, [handleData bytes], sizeof(Ihandle*));

    if (!iupObjectCheck(ih_source) || (!IupClassMatch(ih_source, "list") && !IupClassMatch(ih_source, "flatlist")))
      return NSDragOperationNone;

    [tableView setDropRow:row dropOperation:NSTableViewDropAbove];

    BOOL source_wants_move = iupAttribGetBoolean(ih_source, "DRAGSOURCEMOVE");

    if (is_copy || !source_wants_move)
      return NSDragOperationCopy;
    else
      return NSDragOperationMove;
  }

  return NSDragOperationNone;
}

/* Drop Target - ACCEPTANCE */
- (BOOL)tableView:(NSTableView *)tableView acceptDrop:(id <NSDraggingInfo>)info row:(NSInteger)row dropOperation:(NSTableViewDropOperation)dropOperation
{
  NSPasteboard *pboard = [info draggingPasteboard];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(tableView, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return NO;

  NSEventModifierFlags flags = [NSEvent modifierFlags];
  int is_shift = (flags & NSEventModifierFlagShift) != 0;
  int is_copy = (flags & NSEventModifierFlagOption) != 0;

  if ([[pboard types] containsObject:IupListPasteboardType])
  {
    NSString *rowIndexStr = [pboard stringForType:IupListPasteboardType];
    if (!rowIndexStr) return NO;

    NSInteger sourceRow = -1;
    if (![[NSScanner scannerWithString:rowIndexStr] scanInteger:&sourceRow])
      return NO;

    IFniiii cb = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");
    if (cb && cb(ih, (int)sourceRow + 1, (int)row + 1, is_shift, is_copy) == IUP_IGNORE)
      return NO;

    if (sourceRow < 0 || sourceRow >= [self.dataArray count])
      return NO;

    NSDictionary* sourceItem = [self.dataArray objectAtIndex:sourceRow];
    NSString* text = [sourceItem objectForKey:@"text"];
    NSImage* image = [sourceItem objectForKey:@"image"];
    if (image == (id)[NSNull null]) image = nil;

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

    if (is_copy)
    {
      iupdrvListInsertItem(ih, (int)row, [text UTF8String]);
      if (image)
        iupdrvListSetImageHandle(ih, (int)row + 1, image);
    }
    else
    {
      NSInteger targetRow = row;
      if (sourceRow < targetRow)
        targetRow--;

      iupdrvListRemoveItem(ih, (int)sourceRow);
      iupdrvListInsertItem(ih, (int)targetRow, [text UTF8String]);
      if (image)
        iupdrvListSetImageHandle(ih, (int)targetRow + 1, image);
    }

    NSInteger finalRow = row;
    if (!is_copy && sourceRow < row)
      finalRow--;

    if (finalRow >= 0 && finalRow < [tableView numberOfRows])
    {
      [tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:finalRow] byExtendingSelection:NO];
      [tableView scrollRowToVisible:finalRow];
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", (int)finalRow + 1);
    }

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
    return YES;
  }

  if ([[pboard types] containsObject:IupInternalDndType])
  {
    NSData *handleData = [pboard dataForType:IupInternalDndType];
    if (!handleData || [handleData length] != sizeof(Ihandle*))
      return NO;

    Ihandle* ih_source;
    memcpy(&ih_source, [handleData bytes], sizeof(Ihandle*));

    if (!iupObjectCheck(ih_source) || (!IupClassMatch(ih_source, "list") && !IupClassMatch(ih_source, "flatlist")))
      return NO;

    BOOL is_move = IupGetInt(ih_source, "DRAGSOURCEMOVE") && !is_copy;

    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
    iupAttribSet(ih_source, "_IUPLIST_IGNORE_ACTION", "1");

    int drop_pos = (int)row;

    if (IupGetInt(ih_source, "MULTIPLE"))
    {
      char *value_buffer = IupGetAttribute(ih_source, "VALUE");
      int count = iupdrvListGetCount(ih_source);

      if (value_buffer && count > 0)
      {
        int* items_to_remove = is_move ? malloc(count * sizeof(int)) : NULL;
        int remove_count = 0;

        for (int i = 0; i < count; i++)
        {
          if (i < (int)strlen(value_buffer) && value_buffer[i] == '+')
          {
            int src_pos_1based = i + 1;
            const char* item_text = IupGetAttributeId(ih_source, "", src_pos_1based);
            void* hImage = iupdrvListGetImageHandle(ih_source, src_pos_1based);

            if (item_text)
            {
              iupdrvListInsertItem(ih, drop_pos, item_text);
              if (hImage && hImage != (void*)[NSNull null])
                iupdrvListSetImageHandle(ih, drop_pos + 1, hImage);

              drop_pos++;

              if (is_move && items_to_remove)
                items_to_remove[remove_count++] = src_pos_1based;
            }
          }
        }

        if (is_move && items_to_remove)
        {
          for (int i = remove_count - 1; i >= 0; i--)
            IupSetInt(ih_source, "REMOVEITEM", items_to_remove[i]);

          free(items_to_remove);
        }
      }
    }
    else
    {
      int src_pos_1based = IupGetInt(ih_source, "VALUE");
      if (src_pos_1based > 0)
      {
        const char* item_text = IupGetAttributeId(ih_source, "", src_pos_1based);
        void* hImage = iupdrvListGetImageHandle(ih_source, src_pos_1based);

        if (item_text)
        {
          iupdrvListInsertItem(ih, drop_pos, item_text);
          if (hImage && hImage != (void*)[NSNull null])
            iupdrvListSetImageHandle(ih, drop_pos + 1, hImage);

          if (is_move)
            IupSetInt(ih_source, "REMOVEITEM", src_pos_1based);
        }
      }
    }

    iupAttribSet(ih_source, "_IUPLIST_IGNORE_ACTION", NULL);
    iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

    return YES;
  }

  return NO;
}

- (void)tableView:(NSTableView *)tableView draggingSession:(NSDraggingSession *)session willBeginAtPoint:(NSPoint)screenPoint forRowIndexes:(NSIndexSet *)rowIndexes
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(tableView, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  NSUInteger sourceRow = [rowIndexes firstIndex];
  if (sourceRow != NSNotFound)
  {
    iupAttribSetInt(ih, "_IUPLIST_DRAGITEM", (int)sourceRow + 1);
  }
}

- (void)tableView:(NSTableView *)tableView draggingSession:(NSDraggingSession *)session endedAtPoint:(NSPoint)screenPoint operation:(NSDragOperation)operation
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(tableView, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih) return;

  iupAttribSet(ih, "_IUPLIST_DRAGITEM", NULL);
}

- (void)listDoubleClickAction:(id)sender
{
  NSTableView* table_view = (NSTableView*)sender;
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(table_view, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih)
    return;

  NSInteger clicked_row = [table_view clickedRow];
  if (clicked_row < 0)
    return;

  IFnis cb = (IFnis)IupGetCallback(ih, "DBLCLICK_CB");
  if (cb)
    iupListSingleCallDblClickCb(ih, cb, (int)clicked_row + 1);
}

@end

static void cocoaListSortItems(Ihandle* ih)
{
  if (!iupAttribGet(ih, "_IUPLIST_SORT_ENABLED"))
    return;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  if (sub_type == IUPCOCOALISTSUBTYPE_DROPDOWN)
  {
    NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
    NSMenu* menu = [popup_button menu];
    NSArray* items = [[menu itemArray] sortedArrayUsingComparator:^NSComparisonResult(NSMenuItem* item1, NSMenuItem* item2) {
      return [[item1 title] localizedCaseInsensitiveCompare:[item2 title]];
    }];

    for (NSInteger i = [menu numberOfItems] - 1; i >= 0; i--)
    {
      [menu removeItemAtIndex:i];
    }

    for (NSMenuItem* item in items)
    {
      [menu addItem:item];
    }
  }
  else if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN)
  {
    NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
    NSInteger count = [combo_box numberOfItems];
    NSMutableArray* items = [NSMutableArray arrayWithCapacity:count];

    for (NSInteger i = 0; i < count; i++)
    {
      id obj = [combo_box itemObjectValueAtIndex:i];
      [items addObject:obj];
    }

    [items sortUsingComparator:^NSComparisonResult(id obj1, id obj2) {
      NSString* str1 = [obj1 isKindOfClass:[NSString class]] ? (NSString*)obj1 : [obj1 stringValue];
      NSString* str2 = [obj2 isKindOfClass:[NSString class]] ? (NSString*)obj2 : [obj2 stringValue];
      return [str1 localizedCaseInsensitiveCompare:str2];
    }];

    [combo_box removeAllItems];
    for (id obj in items)
    {
      [combo_box addItemWithObjectValue:obj];
    }
  }
  else if (sub_type >= IUPCOCOALISTSUBTYPE_EDITBOX)
  {
    NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
    IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
    if (list_receiver)
    {
      NSMutableArray* data_array = [list_receiver dataArray];
      [data_array sortUsingComparator:^NSComparisonResult(NSDictionary* dict1, NSDictionary* dict2) {
        NSString* str1 = [dict1 objectForKey:@"text"];
        NSString* str2 = [dict2 objectForKey:@"text"];
        return [str1 localizedCaseInsensitiveCompare:str2];
      }];
      [table_view reloadData];
    }
  }
}

void iupdrvListAddItemSpace(Ihandle* ih, int* h)
{
  (void)ih;
  *h += 2;
}

void iupdrvListAddBorders(Ihandle* ih, int *x, int *y)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        static int popup_decor_w = -1;
        static int popup_decor_h = -1;

        if (popup_decor_w == -1)
        {
          /* Measure NSPopUpButton decorations dynamically */
          NSPopUpButton* tempButton = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
          NSFont* font = cocoaGetNativeFont(ih);
          if (font)
            [tempButton setFont:font];

          /* Add a test item to measure with content */
          [tempButton addItemWithTitle:@"WWWWWWWWWW"];

          NSSize intrinsic_size = [tempButton intrinsicContentSize];
          popup_decor_h = (int)lroundf(intrinsic_size.height);

          /* Measure the width decoration: intrinsic width for "WWWWWWWWWW" minus text width. */
          int text_width = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");
          int sb_size = iupdrvGetScrollbarSize();
          popup_decor_w = (int)lroundf(intrinsic_size.width) - text_width - sb_size;
          if (popup_decor_w < 0) popup_decor_w = 0;

          [tempButton release];
        }

        if (*y < popup_decor_h)
          *y = popup_decor_h;

        *x += popup_decor_w;

        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        static int combo_decor_w = -1;
        static int combo_decor_h = -1;

        if (combo_decor_w == -1)
        {
          /* Measure NSComboBox decorations dynamically */
          NSComboBox* tempComboBox = [[NSComboBox alloc] initWithFrame:NSZeroRect];
          NSFont* font = cocoaGetNativeFont(ih);
          if (font)
            [tempComboBox setFont:font];

          /* Set test string value to measure with content */
          [tempComboBox setStringValue:@"WWWWWWWWWW"];

          NSSize intrinsic_size = [tempComboBox intrinsicContentSize];
          combo_decor_h = (int)lroundf(intrinsic_size.height);

          /* Measure the width decoration: intrinsic width minus text width. */
          int text_width = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");
          int sb_size = iupdrvGetScrollbarSize();
          combo_decor_w = (int)lroundf(intrinsic_size.width) - text_width - sb_size;
          if (combo_decor_w < 0) combo_decor_w = 0;

          [tempComboBox release];
        }

        if (*y < combo_decor_h)
          *y = combo_decor_h;

        *x += combo_decor_w;

        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
    case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        /* Measure NSScrollView borders dynamically.
         * We measure the actual border by comparing widget size with content size. */
        static int cocoa_scroll_border_x = -1;
        static int cocoa_scroll_border_y = -1;

        if (cocoa_scroll_border_x == -1)
        {
          /* Create temporary scroll view to measure borders */
          NSScrollView* temp_scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 200, 100)];
          [temp_scroll setBorderType:NSBezelBorder];
          [temp_scroll setHasVerticalScroller:NO];
          [temp_scroll setHasHorizontalScroller:NO];

          NSRect frame_rect = [temp_scroll frame];
          NSSize content_size = [temp_scroll contentSize];

          cocoa_scroll_border_x = (int)lroundf(frame_rect.size.width - content_size.width);
          cocoa_scroll_border_y = (int)lroundf(frame_rect.size.height - content_size.height);

          [temp_scroll release];
        }

        /* Add measured borders */
        *x += cocoa_scroll_border_x;
        *y += cocoa_scroll_border_y;

        /* In Cocoa, scrollbars overlay the content (unlike GTK/Windows where they add to width). */
        if (ih->data->sb)
        {
          int sb_size = iupdrvGetScrollbarSize();
          *x += sb_size;
        }

        if (ih->data->has_editbox)
        {
          int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

          if (visiblelines > 0)
          {
            /* For EDITBOX with VISIBLELINES: VISIBLELINES includes the entry line.
             * We need to subtract the content of (visiblelines-1) items and add back the text entry height instead. */
            int char_width, char_height;
            iupdrvFontGetCharSize(ih, &char_width, &char_height);
            int item_height = char_height;
            iupdrvListAddItemSpace(ih, &item_height);

            /* Subtract one item height (since visiblelines includes the entry) */
            *y -= item_height;

            /* Add text entry natural height (measured from NSTextField) */
            NSTextField* temp_text = [[NSTextField alloc] initWithFrame:NSZeroRect];
            NSSize text_intrinsic = [temp_text intrinsicContentSize];
            int text_height = (int)lroundf(text_intrinsic.height);
            [temp_text release];

            *y += text_height;
          }

          /* Add NSStackView spacing (3px between text and list) */
          *y += 2*3;
        }

        break;
      }
    default:
      break;
  }
}

int iupdrvListGetCount(Ihandle* ih)
{
  if (ih->handle)
  {
    IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
    switch(sub_type)
    {
      case IUPCOCOALISTSUBTYPE_DROPDOWN:
        {
          NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
          NSInteger number_of_items = [[popup_button menu] numberOfItems];
          return (int)number_of_items;
        }
      case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
        {
          NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
          NSInteger number_of_items = [combo_box numberOfItems];
          return (int)number_of_items;
        }
      case IUPCOCOALISTSUBTYPE_EDITBOX:
      case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
      case IUPCOCOALISTSUBTYPE_SINGLELIST:
        {
          if (ih->data->is_virtual)
            return ih->data->item_count;

          NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
          if (!table_view)
          {
            return 0;
          }

          /* Query the data model directly to ensure the count is always accurate, */
          /* avoiding potential timing issues with NSTableView's numberOfRows property after a reload. */
          IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
          if (list_receiver)
          {
            return (int)[[list_receiver dataArray] count];
          }

          return 0;
        }
       default:
        break;
    }
    return 0;
  }
  else
  {
    int count = 0;
    while(iupAttribGetId(ih, "", count+1))
    {
      count++;
    }
    return count;
  }
}

void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  if (!value)
    value = "";

  NSString* ns_string = [NSString stringWithUTF8String:value];

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
        if (!popup_button) return;
        NSMenu* menu = [popup_button menu];

        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        [menu addItemWithTitle:ns_string action:nil keyEquivalent:@""];
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

        if (iupAttribGet(ih, "_IUPLIST_SORT_ENABLED"))
          cocoaListSortItems(ih);

        cocoaListUpdateDropExpand(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
        if (!combo_box) return;
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        [combo_box addItemWithObjectValue:ns_string];
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

        if (iupAttribGet(ih, "_IUPLIST_SORT_ENABLED"))
          cocoaListSortItems(ih);

        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
    case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
        if (!table_view) return;

        IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
        if (!list_receiver) return;
        NSMutableArray* data_array = [list_receiver dataArray];

        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        NSMutableDictionary* item_data = [NSMutableDictionary dictionaryWithObjectsAndKeys:ns_string, @"text", [NSNull null], @"image", nil];

        [data_array addObject:item_data];

        if (iupAttribGet(ih, "_IUPLIST_SORT_ENABLED"))
          cocoaListSortItems(ih);

        [table_view reloadData];
        [table_view setNeedsDisplay:YES];

        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

        cocoaListUpdateColumnWidth(ih);
        break;
      }
    default:
      break;
  }
}

void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  if (!value)
    value = "";

  NSString* ns_string = [NSString stringWithUTF8String:value];

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
        if (!popup_button) return;
        [[popup_button menu] insertItemWithTitle:ns_string action:nil keyEquivalent:@"" atIndex:pos];

        if (iupAttribGet(ih, "_IUPLIST_SORT_ENABLED"))
          cocoaListSortItems(ih);

        cocoaListUpdateDropExpand(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
        if (!combo_box) return;
        [combo_box insertItemWithObjectValue:ns_string atIndex:pos];

        if (iupAttribGet(ih, "_IUPLIST_SORT_ENABLED"))
          cocoaListSortItems(ih);

        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
    case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
        if (!table_view) return;

        IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
        if (!list_receiver) return;
        NSMutableArray* data_array = [list_receiver dataArray];

        if (pos < 0) pos = 0;
        if (pos > [data_array count]) pos = (int)[data_array count];

        NSMutableDictionary* item_data = [NSMutableDictionary dictionaryWithObjectsAndKeys:ns_string, @"text", [NSNull null], @"image", nil];

        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        [data_array insertObject:item_data atIndex:pos];

        if (iupAttribGet(ih, "_IUPLIST_SORT_ENABLED"))
          cocoaListSortItems(ih);

        [table_view reloadData];
        [table_view setNeedsDisplay:YES];
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

        cocoaListUpdateColumnWidth(ih);
        break;
      }
    default:
      break;
  }
  iupListUpdateOldValue(ih, pos, 0);
}

void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
        if (!popup_button) return;
        [[popup_button menu] removeItemAtIndex:pos];
        cocoaListUpdateDropExpand(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
        if (!combo_box) return;
        [combo_box removeItemAtIndex:pos];
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
    case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
        if (!table_view) return;

        IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
        if (!list_receiver) return;
        NSMutableArray* data_array = [list_receiver dataArray];

        if (pos < 0 || pos >= [data_array count]) return;

        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        [data_array removeObjectAtIndex:pos];
        [table_view reloadData];
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

        cocoaListUpdateColumnWidth(ih);
        break;
      }
    default:
      break;
  }
  iupListUpdateOldValue(ih, pos, 1);
}

void iupdrvListRemoveAllItems(Ihandle* ih)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
        if (!popup_button) return;
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        [[popup_button menu] removeAllItems];
        [popup_button selectItemAtIndex:-1];
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
        cocoaListUpdateDropExpand(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
        if (!combo_box) return;
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
        [combo_box removeAllItems];
        iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
      case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
        if (!table_view) return;

        IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
        if (!list_receiver) return;
        NSMutableArray* data_array = [list_receiver dataArray];

        if ([data_array count] > 0)
        {
            iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
            [data_array removeAllObjects];
            [table_view reloadData];
            iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
        }

        cocoaListUpdateColumnWidth(ih);
        break;
      }
      default:
      break;
  }
}

void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  int pos = id - 1;

  if (sub_type == IUPCOCOALISTSUBTYPE_DROPDOWN)
  {
    NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
    NSMenuItem *menu_item = [popup_button itemAtIndex:pos];
    return [menu_item image];
  }
  else if(sub_type >= IUPCOCOALISTSUBTYPE_EDITBOX)
  {
    NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
    IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
    NSDictionary* item_data = [[list_receiver dataArray] objectAtIndex:pos];
    NSImage* image = [item_data objectForKey:@"image"];
    if (image != nil && ![image isKindOfClass:[NSNull class]])
    {
      return image;
    }
  }
  return NULL;
}

int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  int pos = id - 1;
  NSImage* image = (NSImage*)hImage;

  if (sub_type == IUPCOCOALISTSUBTYPE_DROPDOWN)
  {
    NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
    NSMenuItem *menu_item = [popup_button itemAtIndex:pos];

    if (image)
    {
      NSSize img_size = [image size];
      CGFloat target_size = 16.0;

      if (img_size.width > target_size || img_size.height > target_size)
      {
        CGFloat scale = target_size / MAX(img_size.width, img_size.height);
        [image setSize:NSMakeSize(img_size.width * scale, img_size.height * scale)];
      }
    }

    [menu_item setImage:image];
    return 1;
  }
  else if(sub_type >= IUPCOCOALISTSUBTYPE_EDITBOX)
  {
    NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
    IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
    NSMutableDictionary* item_data = [[list_receiver dataArray] objectAtIndex:pos];
    if (image)
    {
      [item_data setObject:image forKey:@"image"];
    }
    else
    {
      [item_data setObject:[NSNull null] forKey:@"image"];
    }
    [table_view reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:pos] columnIndexes:[NSIndexSet indexSetWithIndex:0]];
    return 1;
  }
  return 0;
}

void iupdrvListSetItemCount(Ihandle* ih, int count)
{
  if (!ih->data->is_virtual)
    return;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  if (sub_type == IUPCOCOALISTSUBTYPE_MULTIPLELIST ||
      sub_type == IUPCOCOALISTSUBTYPE_SINGLELIST)
  {
    NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
    if (table_view)
    {
      [table_view reloadData];
    }
  }
}

static char* cocoaListGetIdValueAttrib(Ihandle* ih, int id_value)
{
  int pos = iupListGetPosAttrib(ih, id_value);
  if (pos >= 0)
  {
    IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
    switch(sub_type)
    {
      case IUPCOCOALISTSUBTYPE_DROPDOWN:
        {
          NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
          NSString* ns_string = [popup_button itemTitleAtIndex:pos];
          const char* c_str = [ns_string UTF8String];
          char* iup_str = iupStrReturnStr(c_str);
          return iup_str;
        }
      case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
        {
          NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
          id object_value = [combo_box itemObjectValueAtIndex:pos];
          NSString* ns_string = nil;

          if ([object_value isKindOfClass:[NSString class]])
            ns_string = (NSString*)object_value;
          else
            ns_string = [object_value stringValue];

          const char* c_str = [ns_string UTF8String];
          char* iup_str = iupStrReturnStr(c_str);
          return iup_str;
        }
      case IUPCOCOALISTSUBTYPE_EDITBOX:
      case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
      case IUPCOCOALISTSUBTYPE_SINGLELIST:
        {
          if (ih->data->is_virtual)
          {
            char* text = iupListGetItemValueCb(ih, pos + 1);
            return text;
          }
          else
          {
            NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
            IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
            NSMutableArray* data_array = [list_receiver dataArray];
            NSDictionary* item_data = [data_array objectAtIndex:pos];
            NSString* ns_string = [item_data objectForKey:@"text"];

            const char* c_str = [ns_string UTF8String];
            char* iup_str = iupStrReturnStr(c_str);
            return iup_str;
          }
        }
   default:
        break;
    }
  }
  return NULL;
}

static int cocoaListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (pos < 0 || !ih->data->show_image)
    return 0;

  NSImage* image = (NSImage*)iupImageGetImage(value, ih, 0, NULL);
  int result = iupdrvListSetImageHandle(ih, id, image);

  if (result)
    cocoaListUpdateColumnWidth(ih);

  return result;
}

static char* cocoaListGetValueAttrib(Ihandle* ih)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
        NSInteger index_of_selected_item = [popup_button indexOfSelectedItem];
        int adjusted_index = (int)(index_of_selected_item+1);
        return iupStrReturnInt(adjusted_index);
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
        NSString* ns_string = [combo_box stringValue];
        const char* c_str = [ns_string UTF8String];
        char* iup_str = iupStrReturnStr(c_str);
        return iup_str;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        NSTextField* text_field = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        if (text_field)
        {
          NSString* ns_string = [text_field stringValue];
          const char* c_str = [ns_string UTF8String];
          char* iup_str = iupStrReturnStr(c_str);
          return iup_str;
        }
        break;
      }
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
      {
        NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
        size_t count = (size_t)[table_view numberOfRows];

        char* ret_str = iupStrGetMemory((int)(count+1));
        memset(ret_str, '-', count);
        ret_str[count] = '\0';

        NSIndexSet* selected_index = [table_view selectedRowIndexes];
        NSUInteger selected_i = [selected_index firstIndex];
        while (selected_i != NSNotFound)
        {
          ret_str[selected_i] = '+';
          selected_i = [selected_index indexGreaterThanIndex:selected_i];
        }

        return ret_str;
      }
    case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
        NSInteger index_of_selected_item = [table_view selectedRow];
        int adjusted_index = (int)(index_of_selected_item+1);
        return iupStrReturnInt(adjusted_index);
      }
    default:
      break;
  }

  return NULL;
}

static int cocoaListSetValueAttrib(Ihandle* ih, const char* value)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
        int iup_pos;
        if (iupStrToInt(value, &iup_pos) == 1)
        {
          NSInteger adjusted_index = (NSInteger)(iup_pos-1);
          [popup_button selectItemAtIndex:adjusted_index];
          iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", iup_pos);
        }
        else
        {
          [popup_button selectItemAtIndex:-1];
          iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
        }
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
        NSString* ns_string = nil;
        if (NULL == value)
          ns_string = @"";
        else
          ns_string = [NSString stringWithUTF8String:value];

        [combo_box setStringValue:ns_string];
        [combo_box selectItemWithObjectValue:ns_string];
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        NSTextField* text_field = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        if (text_field)
        {
          NSString* ns_string = nil;
          if (NULL == value)
            ns_string = @"";
          else
            ns_string = [NSString stringWithUTF8String:value];
          [text_field setStringValue:ns_string];
        }
        break;
      }
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
      {
        NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
        [table_view deselectAll:nil];

        if (NULL != value)
        {
          size_t len;
          size_t count;

          count = (size_t)[table_view numberOfRows];
          len = strlen(value);
          if (len < count)
            count = len;

          for (size_t i = 0; i < count; i++)
          {
            if (value[i] == '+')
            {
              [table_view selectRowIndexes:[NSIndexSet indexSetWithIndex:i] byExtendingSelection:YES];
            }
          }
          iupAttribSetStr(ih, "_IUPLIST_OLDVALUE", value);
        }
        else
        {
          iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
        }
        break;
      }
    case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
        [table_view deselectAll:nil];

        int iup_pos;
        if (iupStrToInt(value, &iup_pos) == 1 && iup_pos > 0)
        {
          NSInteger adjusted_index = (NSInteger)(iup_pos-1);
          [table_view selectRowIndexes:[NSIndexSet indexSetWithIndex:adjusted_index] byExtendingSelection:NO];
          [table_view scrollRowToVisible:adjusted_index];
          iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", iup_pos);
        }
        else
        {
          [table_view deselectAll:nil];
          iupAttribSet(ih, "_IUPLIST_OLDVALUE", NULL);
        }
        break;
      }
    default:
      break;
  }

  return 0;
}

static void cocoaListUpdateDragDrop(Ihandle* ih)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  if (sub_type == IUPCOCOALISTSUBTYPE_SINGLELIST ||
      sub_type == IUPCOCOALISTSUBTYPE_MULTIPLELIST ||
      sub_type == IUPCOCOALISTSUBTYPE_EDITBOX)
  {
    NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
    if (!table_view) return;

    BOOL enable_internal_dnd = ih->data->show_dragdrop && (sub_type == IUPCOCOALISTSUBTYPE_SINGLELIST);
    BOOL enable_crosslist_dnd = iupAttribGetBoolean(ih, "DRAGDROPLIST");

    BOOL enable_drag_source = enable_internal_dnd || (enable_crosslist_dnd && iupAttribGetBoolean(ih, "DRAGSOURCE"));
    BOOL enable_drop_target = enable_internal_dnd || (enable_crosslist_dnd && iupAttribGetBoolean(ih, "DROPTARGET"));

    if (enable_drag_source)
    {
      NSDragOperation source_mask = NSDragOperationMove | NSDragOperationCopy;
      [table_view setDraggingSourceOperationMask:source_mask forLocal:YES];
      [table_view setDraggingSourceOperationMask:NSDragOperationNone forLocal:NO];
      [table_view setVerticalMotionCanBeginDrag:YES];
    }
    else
    {
      [table_view setDraggingSourceOperationMask:NSDragOperationNone forLocal:YES];
      [table_view setDraggingSourceOperationMask:NSDragOperationNone forLocal:NO];
      [table_view setVerticalMotionCanBeginDrag:NO];
    }

    NSMutableArray* registeredTypes = [NSMutableArray array];
    if (enable_internal_dnd)
      [registeredTypes addObject:IupListPasteboardType];

    if (enable_crosslist_dnd && enable_drop_target)
      [registeredTypes addObject:IupInternalDndType];

    if ([registeredTypes count] > 0 && enable_drop_target)
    {
      [table_view registerForDraggedTypes:registeredTypes];
      [table_view setDraggingDestinationFeedbackStyle:enable_internal_dnd ?
        NSTableViewDraggingDestinationFeedbackStyleGap :
        NSTableViewDraggingDestinationFeedbackStyleRegular];
    }
    else
    {
      [table_view unregisterDraggedTypes];
    }
  }
}

static int cocoaListSetShowDragDropAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->show_dragdrop = 1;
  else
    ih->data->show_dragdrop = 0;

  if (ih->handle)
  {
    cocoaListUpdateDragDrop(ih);
    return 0; /* Applied */
  }
  return 1; /* Will be updated in Map */
}

static int cocoaListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  if (sub_type == IUPCOCOALISTSUBTYPE_DROPDOWN)
  {
    NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
    if (iupStrBoolean(value))
    {
      [popup_button performClick:nil];
    }
  }
  else if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN)
  {
    /* NSComboBox does not provide a public API to programmatically show or hide
       the dropdown menu. This is a limitation of NSComboBox on macOS.
       The SHOWDROPDOWN attribute is only supported for DROPDOWN without EDITBOX. */
  }

  return 0;
}

static int cocoaListSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSText* field_editor = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
        field_editor = [[combo_box window] fieldEditor:YES forObject:combo_box];
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        NSTextField* text_field = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        if (text_field)
        {
          field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
        }
        break;
      }
    default:
      break;
  }

  if (field_editor)
  {
    NSString* ns_string = [NSString stringWithUTF8String:value];
    [field_editor insertText:ns_string];
  }

  return 0;
}

static int cocoaListSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    value = "";

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
        NSString* current = [combo_box stringValue];
        NSString* append = [NSString stringWithUTF8String:value];
        NSString* new_value = [current stringByAppendingString:append];
        [combo_box setStringValue:new_value];
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        NSTextField* text_field = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        if (text_field)
        {
          NSString* current = [text_field stringValue];
          NSString* append = [NSString stringWithUTF8String:value];
          NSString* new_value = [current stringByAppendingString:append];
          [text_field setStringValue:new_value];
        }
        break;
      }
    default:
      break;
  }

  return 0;
}

static int cocoaListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return 0;
  }

  if (!text_control)
    return 0;

  NSText* field_editor = [[text_control window] fieldEditor:YES forObject:text_control];

  if (!field_editor)
  {
    [[text_control window] makeFirstResponder:text_control];
    field_editor = [[text_control window] fieldEditor:YES forObject:text_control];
    if (!field_editor)
      return 0;
  }

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    [field_editor setSelectedRange:NSMakeRange(0, 0)];
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    [field_editor selectAll:nil];
    return 0;
  }

  int start = 1, end = 1;
  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;

  if (start < 1 || end < 1)
    return 0;

  start--;
  end--;

  NSRange range = NSMakeRange(start, end - start + 1);
  [field_editor setSelectedRange:range];

  return 0;
}

static char* cocoaListGetSelectionAttrib(Ihandle* ih)
{
  if (!ih->data->has_editbox)
    return NULL;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return NULL;
  }

  if (!text_control)
    return NULL;

  NSText* field_editor = [[text_control window] fieldEditor:NO forObject:text_control];

  if (!field_editor)
    return NULL;

  NSRange range = [field_editor selectedRange];
  if (range.length == 0)
    return NULL;

  int start = (int)range.location + 1;
  int end = (int)(range.location + range.length);
  return iupStrReturnIntInt(start, end, ':');
}

static int cocoaListSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return 0;
  }

  if (!text_control)
    return 0;

  NSText* field_editor = [[text_control window] fieldEditor:YES forObject:text_control];

  if (!field_editor)
  {
    [[text_control window] makeFirstResponder:text_control];
    field_editor = [[text_control window] fieldEditor:YES forObject:text_control];

    if (!field_editor)
      return 0;
  }

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    [field_editor setSelectedRange:NSMakeRange(0, 0)];
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    [field_editor selectAll:nil];
    return 0;
  }

  int start = 0, end = 0;
  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;

  if (start < 0 || end < 0)
    return 0;

  NSRange range = NSMakeRange(start, end - start);
  [field_editor setSelectedRange:range];

  return 0;
}

static char* cocoaListGetSelectionPosAttrib(Ihandle* ih)
{
  if (!ih->data->has_editbox)
    return NULL;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return NULL;
  }

  if (!text_control)
    return NULL;

  NSText* field_editor = [[text_control window] fieldEditor:NO forObject:text_control];

  if (!field_editor)
    return NULL;

  NSRange range = [field_editor selectedRange];
  if (range.length == 0)
    return NULL;

  int start = (int)range.location;
  int end = (int)(range.location + range.length);
  return iupStrReturnIntInt(start, end, ':');
}

static int cocoaListSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox || !value)
    return 0;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return 0;
  }

  if (!text_control)
    return 0;

  NSText* field_editor = [[text_control window] fieldEditor:YES forObject:text_control];

  if (!field_editor)
    return 0;

  NSRange range = [field_editor selectedRange];
  if (range.length > 0)
  {
    NSString* ns_string = [NSString stringWithUTF8String:value];
    [field_editor replaceCharactersInRange:range withString:ns_string];
  }

  return 0;
}

static char* cocoaListGetSelectedTextAttrib(Ihandle* ih)
{
  if (!ih->data->has_editbox)
    return NULL;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;
  NSString* full_string = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        full_string = [(NSComboBox*)text_control stringValue];
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        if (text_control)
        {
          full_string = [(NSTextField*)text_control stringValue];
        }
        break;
      }
    default:
      return NULL;
  }

  if (!text_control || !full_string)
    return NULL;

  NSText* field_editor = [[text_control window] fieldEditor:NO forObject:text_control];

  if (!field_editor)
    return NULL;

  NSRange range = [field_editor selectedRange];
  if (range.length == 0)
    return NULL;

  NSString* selected = [full_string substringWithRange:range];
  const char* c_str = [selected UTF8String];
  return iupStrReturnStr(c_str);
}

static int cocoaListSetCaretAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox || !value)
    return 0;

  int pos = 1;
  iupStrToInt(value, &pos);
  if (pos < 1) pos = 1;
  pos--;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return 0;
  }

  if (text_control)
  {
    NSText* field_editor = [[text_control window] fieldEditor:YES forObject:text_control];
    if(field_editor)
    {
      [field_editor setSelectedRange:NSMakeRange(pos, 0)];
    }
  }

  return 0;
}

static char* cocoaListGetCaretAttrib(Ihandle* ih)
{
  if (!ih->data->has_editbox)
    return NULL;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return NULL;
  }

  if (!text_control)
    return NULL;

  NSText* field_editor = [[text_control window] fieldEditor:NO forObject:text_control];

  if (!field_editor)
    return NULL;

  NSRange range = [field_editor selectedRange];
  int pos = (int)range.location + 1;
  return iupStrReturnInt(pos);
}

static int cocoaListSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox || !value)
    return 0;

  int pos = 0;
  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return 0;
  }

  if (text_control)
  {
    NSText* field_editor = [[text_control window] fieldEditor:YES forObject:text_control];
    if(field_editor)
    {
      [field_editor setSelectedRange:NSMakeRange(pos, 0)];
    }
  }

  return 0;
}

static char* cocoaListGetCaretPosAttrib(Ihandle* ih)
{
  if (!ih->data->has_editbox)
    return NULL;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return NULL;
  }

  if (!text_control)
    return NULL;

  NSText* field_editor = [[text_control window] fieldEditor:NO forObject:text_control];

  if (!field_editor)
    return NULL;

  NSRange range = [field_editor selectedRange];
  int pos = (int)range.location;
  return iupStrReturnInt(pos);
}

static int cocoaListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox || !value)
    return 0;

  int pos = 1;
  iupStrToInt(value, &pos);
  if (pos < 1) pos = 1;
  pos--;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return 0;
  }

  if (text_control)
  {
    NSText* field_editor = [[text_control window] fieldEditor:YES forObject:text_control];
    if(field_editor)
    {
      [field_editor scrollRangeToVisible:NSMakeRange(pos, 0)];
    }
  }

  return 0;
}

static int cocoaListSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox || !value)
    return 0;

  int pos = 0;
  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return 0;
  }

  if (text_control)
  {
    NSText* field_editor = [[text_control window] fieldEditor:YES forObject:text_control];
    if(field_editor)
    {
      [field_editor scrollRangeToVisible:NSMakeRange(pos, 0)];
    }
  }

  return 0;
}

static int cocoaListSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
    NSFont* font = cocoaGetNativeFont(ih);

    if (sub_type == IUPCOCOALISTSUBTYPE_MULTIPLELIST ||
        sub_type == IUPCOCOALISTSUBTYPE_SINGLELIST ||
        sub_type == IUPCOCOALISTSUBTYPE_EDITBOX)
    {
      NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);

      [table_view reloadData];
      cocoaListUpdateColumnWidth(ih);
      [table_view setNeedsDisplay:YES];
    }
    else if (sub_type == IUPCOCOALISTSUBTYPE_DROPDOWN)
    {
      NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
      if (popup_button && font)
        [popup_button setFont:font];

      cocoaListUpdateDropExpand(ih);
    }
    else if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN)
    {
      NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
      if (combo_box)
      {
        if (font)
          [combo_box setFont:font];

        NSInteger selected = [combo_box indexOfSelectedItem];
        [combo_box reloadData];
        if (selected != NSNotFound)
          [combo_box selectItemAtIndex:selected];
      }
    }
  }
  return 1;
}

static int cocoaListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  BOOL is_editable = !iupStrBoolean(value);
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
    {
      NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
      if (combo_box)
        [combo_box setEditable:is_editable];
      break;
    }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
    {
      NSTextField* text_field = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
      if (text_field)
        [text_field setEditable:is_editable];
      break;
    }
    default:
      break;
  }

  return 0;
}

static char* cocoaListGetReadOnlyAttrib(Ihandle* ih)
{
  if (!ih->data->has_editbox)
    return NULL;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
    {
      NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
      if (combo_box)
        return iupStrReturnBoolean(![combo_box isEditable]);
      break;
    }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
    {
      NSTextField* text_field = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
      if (text_field)
        return iupStrReturnBoolean(![text_field isEditable]);
      break;
    }
    default:
      break;
  }

  return NULL;
}

static int cocoaListSetClipboardAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox || !value)
    return 0;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  NSControl* text_control = nil;

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        text_control = (NSControl*)cocoaListGetBaseWidget(ih);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        text_control = (NSControl*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        break;
      }
    default:
      return 0;
  }

  if (!text_control)
    return 0;

  NSText* field_editor = [[text_control window] fieldEditor:YES forObject:text_control];

  if (!field_editor)
    return 0;

  if (iupStrEqualNoCase(value, "COPY"))
  {
    [field_editor copy:nil];
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    [field_editor cut:nil];
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    [field_editor paste:nil];
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    [field_editor delete:nil];
  }

  return 0;
}

static int cocoaListSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = 0;

  return 1;
}

static int cocoaListSetContextMenuAttrib(Ihandle* ih, const char* value)
{
  Ihandle* menu_ih = (Ihandle*)value;
  id widget_to_attach = nil;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      return 0;

    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      widget_to_attach = cocoaListGetBaseWidget(ih);
      break;

    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
      case IUPCOCOALISTSUBTYPE_SINGLELIST:
      widget_to_attach = cocoaListGetBaseWidget(ih);
      break;

      case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        widget_to_attach = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
        if (table_view)
        {
          iupcocoaCommonBaseSetContextMenuForWidget(ih, table_view, menu_ih);
        }
        break;
      }
      default:
      return 0;
  }

  if (widget_to_attach)
  {
    iupcocoaCommonBaseSetContextMenuForWidget(ih, widget_to_attach, menu_ih);
  }

  return 1;
}

static int cocoaListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_dropdown)
  {
    int pos = 1;
    if (iupStrToInt(value, &pos))
    {
      NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
      NSInteger adjusted_pos = pos - 1;
      if (adjusted_pos >= 0 && adjusted_pos < [table_view numberOfRows])
      {
        [table_view scrollRowToVisible:adjusted_pos];
      }
    }
  }
  return 0;
}

static int cocoaListSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->is_dropdown)
    return 0;

  if (!iupStrToInt(value, &ih->data->spacing))
    ih->data->spacing = 0;

  if (ih->handle)
  {
    NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
    [table_view reloadData];
    cocoaListUpdateColumnWidth(ih);
    return 0;
  }
  else
    return 1;
}

static int cocoaListSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
    NSTextField* text_field = nil;
    if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN)
    {
      text_field = (NSTextField*)cocoaListGetBaseWidget(ih);
    }
    else if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOX)
    {
      text_field = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
    }

    if (text_field)
    {
      NSRect currentFrame = [text_field frame];
      NSRect superBounds = [[text_field superview] bounds];

      if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOX)
      {
        CGFloat newWidth = superBounds.size.width - (ih->data->horiz_padding * 2);
        if (newWidth > 0)
        {
          currentFrame.origin.x = ih->data->horiz_padding;
          currentFrame.size.width = newWidth;
          [text_field setFrame:currentFrame];
        }
      }
    }
    return 0;
  }
  else
    return 1;
}

static int cocoaListSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN)
  {
    NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
    if (value)
    {
      NSString* placeholder = [NSString stringWithUTF8String:value];
      [[combo_box cell] setPlaceholderString:placeholder];
    }
    else
    {
      [[combo_box cell] setPlaceholderString:nil];
    }
  }
  else if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOX)
  {
    NSTextField* text_field = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
    if (text_field)
    {
      if (value)
      {
        NSString* placeholder = [NSString stringWithUTF8String:value];
        [[text_field cell] setPlaceholderString:placeholder];
      }
      else
      {
        [[text_field cell] setPlaceholderString:nil];
      }
    }
  }

  return 1;
}

static int cocoaListSetFilterAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  /* Store filter for use in controlTextDidChange: delegate method.
     The actual filtering is applied in the text change handler. */
  iupAttribSet(ih, "FILTER", value);
  return 1;
}

static char* cocoaListGetScrollVisibleAttrib(Ihandle* ih)
{
  NSScrollView* scroll_view = nil;
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  if (sub_type == IUPCOCOALISTSUBTYPE_DROPDOWN ||
      sub_type == IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN)
    return NULL;

  if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOX)
    scroll_view = (NSScrollView*)iupAttribGet(ih, "_IUPCOCOA_SCROLLVIEW");
  else if (sub_type == IUPCOCOALISTSUBTYPE_MULTIPLELIST ||
           sub_type == IUPCOCOALISTSUBTYPE_SINGLELIST)
    scroll_view = (NSScrollView*)ih->handle;

  if (!scroll_view)
    return "NO";

  NSView* documentView = [scroll_view documentView];
  if (!documentView)
    return "NO";

  NSSize contentSize = [scroll_view contentSize];
  NSRect documentFrame = [documentView frame];

  BOOL has_vert = NO;
  BOOL has_horiz = NO;

  if ([scroll_view hasVerticalScroller])
    has_vert = (documentFrame.size.height > contentSize.height);

  if ([scroll_view hasHorizontalScroller])
    has_horiz = (documentFrame.size.width > contentSize.width);

  if (has_vert && has_horiz)
    return "BOTH";
  else if (has_vert)
    return "VERTICAL";
  else if (has_horiz)
    return "HORIZONTAL";
  else
    return "NO";
}

static int cocoaListSetDragDropListAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
  {
    cocoaListUpdateDragDrop(ih);
  }
  return 1;
}

static char* cocoaListGetImageNativeHandleAttribId(Ihandle* ih, int id)
{
  if (!ih->data->show_image)
    return NULL;

  void* handle = iupdrvListGetImageHandle(ih, id);
  return (char*)handle;
}

static int cocoaListSetAutoRedrawAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  /* NSView handles automatic redrawing, no manual control needed on macOS */
  return 1;
}

static int cocoaListMapMethod(Ihandle* ih)
{
  NSView* root_view = nil;
  NSView* main_view = nil;

  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        NSPopUpButton* popup_button = [[IupCocoaListPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, kIupCocoaDefaultWidthNSPopUpButton, kIupCocoaDefaultHeightNSPopUpButton) pullsDown:NO];
        root_view = popup_button;
        main_view = root_view;

        objc_setAssociatedObject(popup_button, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

        IupCocoaListPopupButtonReceiver* list_receiver = [[IupCocoaListPopupButtonReceiver alloc] init];
        [popup_button setTarget:list_receiver];
        [popup_button setAction:@selector(onSelectionChanged:)];
        objc_setAssociatedObject(popup_button, IUP_COCOA_LIST_POPUPBUTTON_RECEIVER_OBJ_KEY, (id)list_receiver, OBJC_ASSOCIATION_RETAIN);
        [list_receiver release];

        if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        {
          iupcocoaSetCanFocus(ih, 0);
        }
        else
        {
          iupcocoaSetCanFocus(ih, 1);
        }

        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = [[IupCocoaComboBox alloc] initWithFrame:NSMakeRect(0, 0, kIupCocoaDefaultWidthNSComboBox, kIupCocoaDefaultHeightNSComboBox)];

        root_view = combo_box;
        main_view = root_view;

        objc_setAssociatedObject(combo_box, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

        IupCocoaListDelegate* list_delegate = [[IupCocoaListDelegate alloc] init];
        [combo_box setDelegate:list_delegate];
        objc_setAssociatedObject(combo_box, IUP_COCOA_LIST_DELEGATE_OBJ_KEY, (id)list_delegate, OBJC_ASSOCIATION_RETAIN);
        [list_delegate release];

        int voptions = iupAttribGetInt(ih, "VISIBLEITEMS");
        if (voptions == 0)
          voptions = iupAttribGetInt(ih, "VISIBLE_ITEMS");
        if (voptions <= 0)
          voptions = 5;
        [combo_box setNumberOfVisibleItems:voptions];

        if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        {
          iupcocoaSetCanFocus(ih, 0);
        }
        else
        {
          iupcocoaSetCanFocus(ih, 1);
        }

        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        NSStackView* stack_view = [[NSStackView alloc] initWithFrame:NSZeroRect];
        [stack_view setOrientation:NSUserInterfaceLayoutOrientationVertical];
        [stack_view setAlignment:NSLayoutAttributeLeading];
        [stack_view setSpacing:3];
        [stack_view setDistribution:NSStackViewDistributionFill];

        NSTextField* text_field = [[IupCocoaListTextField alloc] initWithFrame:NSZeroRect];
        objc_setAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

        IupCocoaListDelegate* text_delegate = [[IupCocoaListDelegate alloc] init];
        [text_field setDelegate:text_delegate];
        objc_setAssociatedObject(text_field, IUP_COCOA_LIST_DELEGATE_OBJ_KEY, (id)text_delegate, OBJC_ASSOCIATION_RETAIN);
        [text_delegate release];

        NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSZeroRect];
        NSTableView* table_view = [[IupCocoaListTableView alloc] initWithFrame:NSZeroRect];
        objc_setAssociatedObject(table_view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

        NSTableColumn* first_column = [[NSTableColumn alloc] initWithIdentifier:@"IupList"];
        [first_column setResizingMask:NSTableColumnAutoresizingMask | NSTableColumnUserResizingMask];
        [first_column setMinWidth:kIupCocoaMinColumnWidth];
        [first_column setMaxWidth:10000];
        [table_view addTableColumn:first_column];
        [first_column release];

        [table_view setHeaderView:nil];
        [table_view setAllowsMultipleSelection:NO];
        [table_view setColumnAutoresizingStyle:NSTableViewUniformColumnAutoresizingStyle];

        IupCocoaListTableViewReceiver* list_receiver = [[IupCocoaListTableViewReceiver alloc] init];
        objc_setAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY, (id)list_receiver, OBJC_ASSOCIATION_RETAIN);
        [list_receiver release];

        [table_view setDataSource:list_receiver];
        [table_view setDelegate:list_receiver];
        [table_view setDoubleAction:@selector(listDoubleClickAction:)];
        [table_view setTarget:list_receiver];

        [scroll_view setDocumentView:table_view];
        [table_view release];

        BOOL autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
        if (ih->data->sb)
        {
          [scroll_view setHasVerticalScroller:YES];
          [scroll_view setHasHorizontalScroller:YES];
          [scroll_view setAutohidesScrollers:autohide];
        }
        else
        {
          [scroll_view setHasVerticalScroller:NO];
          [scroll_view setHasHorizontalScroller:NO];
        }

        [scroll_view setBorderType:NSBezelBorder];

        [stack_view addArrangedSubview:text_field];
        [stack_view addArrangedSubview:scroll_view];
        [text_field release];
        [scroll_view release];

        [[text_field.widthAnchor constraintEqualToAnchor:stack_view.widthAnchor] setActive:YES];
        [[scroll_view.widthAnchor constraintEqualToAnchor:stack_view.widthAnchor] setActive:YES];

        /* Set height constraint for scroll_view based on VISIBLELINES */
        int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
        if (visiblelines > 0)
        {
          /* Calculate scroll view height: (visiblelines-1) items worth of space.
           * VISIBLELINES includes the text entry, so subtract 1 for list items. */
          int char_width, char_height;
          iupdrvFontGetCharSize(ih, &char_width, &char_height);
          int item_height = char_height;
          iupdrvListAddItemSpace(ih, &item_height);

          int num_items_in_list = visiblelines - 1;
          int list_content_height = num_items_in_list * item_height;

          /* Add borders (NSBezelBorder is 2px total: 1px top + 1px bottom) */
          int list_total_height = list_content_height + 2;

          [[scroll_view.heightAnchor constraintEqualToConstant:list_total_height] setActive:YES];
        }
        else
        {
          [scroll_view setContentHuggingPriority:NSLayoutPriorityDefaultLow forOrientation:NSLayoutConstraintOrientationVertical];
          [scroll_view setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow forOrientation:NSLayoutConstraintOrientationVertical];
        }

        [text_field setContentHuggingPriority:NSLayoutPriorityRequired forOrientation:NSLayoutConstraintOrientationVertical];

        root_view = stack_view;
        main_view = text_field;

        iupAttribSet(ih, "_IUPCOCOA_EDITFIELD", (char*)text_field);
        iupAttribSet(ih, "_IUPCOCOA_TABLEVIEW", (char*)table_view);
        iupAttribSet(ih, "_IUPCOCOA_SCROLLVIEW", (char*)scroll_view);

        if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        {
          iupcocoaSetCanFocus(ih, 0);
        }
        else
        {
          iupcocoaSetCanFocus(ih, 1);
        }

        break;
      }
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
    case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        NSTableView* table_view = [[IupCocoaListTableView alloc] initWithFrame:NSZeroRect];

        objc_setAssociatedObject(table_view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

        NSTableColumn* first_column = [[NSTableColumn alloc] initWithIdentifier:@"IupList"];
        [first_column setResizingMask:NSTableColumnAutoresizingMask | NSTableColumnUserResizingMask];
        [first_column setMinWidth:kIupCocoaMinColumnWidth];
        [first_column setMaxWidth:10000];
        [table_view addTableColumn:first_column];
        [first_column release];

        [table_view setHeaderView:nil];
        [table_view setIntercellSpacing:NSMakeSize(0, 0)];
        [table_view setColumnAutoresizingStyle:NSTableViewUniformColumnAutoresizingStyle];
        [table_view setAutoresizesSubviews:YES];
        [table_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

        IupCocoaListTableViewReceiver* list_receiver = [[IupCocoaListTableViewReceiver alloc] init];
        objc_setAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY, (id)list_receiver, OBJC_ASSOCIATION_RETAIN);
        [list_receiver release];

        [table_view setDataSource:list_receiver];
        [table_view setDelegate:list_receiver];
        [table_view setDoubleAction:@selector(listDoubleClickAction:)];
        [table_view setTarget:list_receiver];

        if (sub_type == IUPCOCOALISTSUBTYPE_MULTIPLELIST)
          [table_view setAllowsMultipleSelection:YES];
        else
          [table_view setAllowsMultipleSelection:NO];

        /* Virtual mode: set fixed row height for better performance */
        if (ih->data->is_virtual)
        {
          int char_height;
          iupdrvFontGetCharSize(ih, NULL, &char_height);
          int row_height = char_height + 4;
          [table_view setRowHeight:row_height];
        }

        NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSZeroRect];
        [scroll_view setDocumentView:table_view];
        [table_view release];

        root_view = scroll_view;
        main_view = table_view;

        iupAttribSet(ih, "_IUPCOCOA_TABLEVIEW", (char*)table_view);

        BOOL autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
        if (ih->data->sb)
        {
          [scroll_view setHasVerticalScroller:YES];
          [scroll_view setHasHorizontalScroller:YES];
          [scroll_view setAutohidesScrollers:autohide];
        }
        else
        {
          [scroll_view setHasVerticalScroller:NO];
          [scroll_view setHasHorizontalScroller:NO];
        }

        [scroll_view setBorderType:NSBezelBorder];
        [scroll_view setAutoresizesSubviews:YES];
        [scroll_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [[scroll_view contentView] setAutoresizesSubviews:YES];

        if (!iupAttribGetBoolean(ih, "CANFOCUS"))
        {
          iupcocoaSetCanFocus(ih, 0);
        }
        else
        {
          iupcocoaSetCanFocus(ih, 1);
        }

        break;
      }
    default:
      break;
  }

  ih->handle = root_view;
  iupcocoaSetAssociatedViews(ih, main_view, root_view);

  IupCocoaFont* iup_font = iupcocoaGetFont(ih);
  if (iup_font && [main_view respondsToSelector:@selector(setFont:)])
  {
    [(id)main_view setFont:[iup_font nativeFont]];
  }

  iupcocoaAddToParent(ih);

  if (iupAttribGetBoolean(ih, "SORT"))
  {
    IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
    if (sub_type == IUPCOCOALISTSUBTYPE_DROPDOWN)
    {
      iupAttribSet(ih, "_IUPLIST_SORT_ENABLED", "1");
    }
    else if (sub_type == IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN)
    {
      iupAttribSet(ih, "_IUPLIST_SORT_ENABLED", "1");
    }
    else if (sub_type >= IUPCOCOALISTSUBTYPE_EDITBOX)
    {
      NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
      IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
      if (list_receiver)
      {
        iupAttribSet(ih, "_IUPLIST_SORT_ENABLED", "1");
      }
    }
  }

  /* Don't populate items in virtual mode */
  if (!ih->data->is_virtual)
    iupListSetInitialItems(ih);

  if (sub_type == IUPCOCOALISTSUBTYPE_DROPDOWN)
  {
    NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
    [popup_button selectItemAtIndex:-1];
  }

  cocoaListUpdateDropExpand(ih);

  if (sub_type == IUPCOCOALISTSUBTYPE_MULTIPLELIST ||
      sub_type == IUPCOCOALISTSUBTYPE_SINGLELIST ||
      sub_type == IUPCOCOALISTSUBTYPE_EDITBOX)
  {
    cocoaListUpdateColumnWidth(ih);

    /* Disable drag-drop in virtual mode */
    if (!ih->data->is_virtual)
      cocoaListUpdateDragDrop(ih);

    NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
    if (table_view)
    {
      [table_view reloadData];
    }
  }

  return IUP_NOERROR;
}

static void cocoaListUnMapMethod(Ihandle* ih)
{
  NSView* root_view = ih->handle;
  if (!root_view) return;

  NSView* base_view = cocoaListGetBaseWidget(ih);
  IupCocoaListSubType sub_type = cocoaListGetSubType(ih);

  switch(sub_type)
  {
    case IUPCOCOALISTSUBTYPE_DROPDOWN:
      {
        NSPopUpButton* popup_button = (NSPopUpButton*)base_view;
        [popup_button setTarget:nil];
        objc_setAssociatedObject(base_view, IUP_COCOA_LIST_POPUPBUTTON_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
      {
        NSComboBox* combo_box = (NSComboBox*)base_view;
        [combo_box setDelegate:nil];
        objc_setAssociatedObject(base_view, IUP_COCOA_LIST_DELEGATE_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN);
        break;
      }
    case IUPCOCOALISTSUBTYPE_EDITBOX:
      {
        NSTextField* text_field = (NSTextField*)iupAttribGet(ih, "_IUPCOCOA_EDITFIELD");
        NSTableView* table_view = (NSTableView*)iupAttribGet(ih, "_IUPCOCOA_TABLEVIEW");
        if(text_field)
        {
          [text_field setDelegate:nil];
          objc_setAssociatedObject(text_field, IUP_COCOA_LIST_DELEGATE_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN);
        }
        if(table_view)
        {
          [table_view setDataSource:nil];
          [table_view setDelegate:nil];
        }
        objc_setAssociatedObject(base_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN);
        break;
      }
    case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
    case IUPCOCOALISTSUBTYPE_SINGLELIST:
      {
        NSTableView* table_view = (NSTableView*)base_view;
        [table_view setDataSource:nil];
        [table_view setDelegate:nil];
        objc_setAssociatedObject(base_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN);
        break;
      }
    default:
      break;
  }

  Ihandle* menu_ih = (Ihandle*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");
  if (menu_ih)
  {
    IupDestroy(menu_ih);
    iupAttribSet(ih, "_COCOA_CONTEXT_MENU_IH", NULL);
  }

  objc_setAssociatedObject(base_view, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

  iupcocoaRemoveFromParent(ih);
  iupcocoaSetAssociatedViews(ih, nil, nil);
  [root_view release];
  ih->handle = NULL;
}

void iupdrvListInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = cocoaListMapMethod;
  ic->UnMap = cocoaListUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, cocoaListSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttributeId(ic, "IDVALUE", cocoaListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", cocoaListGetValueAttrib, cocoaListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDRAGDROP", NULL, cocoaListSetShowDragDropAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGDROPLIST", NULL, cocoaListSetDragDropListAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, cocoaListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, cocoaListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, cocoaListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, cocoaListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", cocoaListGetSelectedTextAttrib, cocoaListSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", cocoaListGetSelectionAttrib, cocoaListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", cocoaListGetSelectionPosAttrib, cocoaListSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", cocoaListGetCaretAttrib, cocoaListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", cocoaListGetCaretPosAttrib, cocoaListSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, cocoaListSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, cocoaListSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", cocoaListGetReadOnlyAttrib, cocoaListSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, cocoaListSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, cocoaListSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, cocoaListSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, cocoaListSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTEXTMENU", iupcocoaCommonBaseGetContextMenuAttrib, cocoaListSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, cocoaListSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", NULL, cocoaListSetFilterAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", cocoaListGetScrollVisibleAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, cocoaListSetAutoRedrawAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", NULL, cocoaListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", cocoaListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING|IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLE_ITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_NO_INHERIT);
}
