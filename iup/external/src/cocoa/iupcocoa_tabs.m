/*
 * This implementation uses NSTabViewController, a modern high-level API for managing
 * tabs in macOS. While this simplifies integration with the view controller architecture,
 * it imposes limitations on customizability.
 *
 * Supported features:
 * - TABTYPE: TOP, BOTTOM, LEFT, RIGHT (vertical tabs use classic NSTabView bezel style)
 * - TABIMAGE, TABTITLE, TABVISIBLE
 * - RIGHTCLICK_CB callback
 *
 * Attributes NOT SUPPORTED due to NSTabViewController API limitations:
 * - FONT, FGCOLOR, BGCOLOR: No public APIs to style individual tabs
 * - TABPADDING: Cannot customize spacing in segmented control style
 * - SHOWCLOSE: No built-in support for close buttons on tabs
 * - MULTILINE: Horizontal tabs become scrollable instead of wrapping. Vertical tabs are always stacked.
 *
 * Note: LEFT/RIGHT orientations use NSTabView's bezel border style which has a
 * different appearance than the modern segmented controls used for TOP/BOTTOM.
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_tabs.h"
#include "iup_array.h"
#include "iup_image.h"

#include "iupcocoa_drv.h"


static int cocoaTabsPosFixFromNative(Ihandle* ih, int native_pos);
static int cocoaTabsCreateAndInsertItem(Ihandle* ih, Ihandle* child, int iup_pos);

@interface IupTabView : NSTabView
@end

@implementation IupTabView

- (BOOL)isFlipped
{
  return YES;
}

- (Ihandle*)ihandle
{
  return (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
}

- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if (result)
  {
    Ihandle* ih = [self ihandle];
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
    Ihandle* ih = [self ihandle];
    if (ih)
      iupCocoaFocusOut(ih);
  }
  return result;
}

- (BOOL)acceptsFirstResponder
{
  Ihandle* ih = [self ihandle];
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

- (void)keyDown:(NSEvent *)event
{
  Ihandle* ih = [self ihandle];
  if (ih)
  {
    int mac_key_code = [event keyCode];
    if (!iupCocoaKeyEvent(ih, event, mac_key_code, true))
      [super keyDown:event];
  }
  else
    [super keyDown:event];
}

- (void)keyUp:(NSEvent *)event
{
  Ihandle* ih = [self ihandle];
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
  Ihandle* ih = [self ihandle];
  if (ih)
  {
    int mac_key_code = [event keyCode];
    if (!iupCocoaModifierEvent(ih, event, mac_key_code))
      [super flagsChanged:event];
  }
  else
    [super flagsChanged:event];
}

- (void)rightMouseDown:(NSEvent*)event
{
  Ihandle* ih = [self ihandle];
  if (!ih)
  {
    [super rightMouseDown:event];
    return;
  }

  IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
  if (!cb)
  {
    [super rightMouseDown:event];
    return;
  }

  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  NSTabViewItem* item = [self tabViewItemAtPoint:point];
  if (item)
  {
    NSInteger native_pos = [self indexOfTabViewItem:item];
    if (native_pos != NSNotFound)
    {
      int iup_pos = cocoaTabsPosFixFromNative(ih, (int)native_pos);
      if (iup_pos != -1)
      {
        cb(ih, iup_pos);
      }
    }
  }
  else
  {
    [super rightMouseDown:event];
  }
}

@end


@interface IupTabViewController : NSTabViewController
@property(nonatomic, assign) NSUInteger previousSelectedIndex;
@end

@implementation IupTabViewController

- (Ihandle*)ihandle
{
  return (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
}

- (void)tabView:(NSTabView*)tab_view didSelectTabViewItem:(nullable NSTabViewItem*)tab_view_item
{
  [super tabView:tab_view didSelectTabViewItem:tab_view_item];

  Ihandle* ih = [self ihandle];
  if (!ih)
  {
    return;
  }

  NSUInteger prev_pos_native = self.previousSelectedIndex;
  NSInteger current_pos_native = [[self tabViewItems] indexOfObject:tab_view_item];

  if (iupAttribGet(ih, "_IUPCOCOA_IGNORE_CHANGE"))
  {
    self.previousSelectedIndex = current_pos_native;
    return;
  }

  // The delegate provides native indices (visible tabs only).
  // Convert them back to IUP indices (all children) for the callbacks.
  int pos = cocoaTabsPosFixFromNative(ih, (int)current_pos_native);
  int prev_pos = cocoaTabsPosFixFromNative(ih, (int)prev_pos_native);

  if (pos != -1 && prev_pos != -1)
  {
    Ihandle* child = IupGetChild(ih, pos);
    Ihandle* prev_child = IupGetChild(ih, prev_pos);

    IFnnn cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
    if (cb)
    {
      cb(ih, child, prev_child);
    }
    else
    {
      IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
      if (cb2)
      {
        cb2(ih, pos, prev_pos);
      }
    }
  }

  self.previousSelectedIndex = current_pos_native;
}

@end


static IupTabViewController* cocoaGetTabViewController(Ihandle* ih)
{
  if (ih && ih->handle)
  {
    IupTabViewController* tab_view_controller = (IupTabViewController*)ih->handle;
    NSCAssert([tab_view_controller isKindOfClass:[IupTabViewController class]], @"Expected IupTabViewController");
    return tab_view_controller;
  }
  return nil;
}

// Manages an array tracking the visibility of each IUP child tab.
// This is necessary because Cocoa's API removes invisible tabs, not just hides them.
static Iarray* cocoaTabsGetVisibleArray(Ihandle* ih)
{
  Iarray* visible_array = (Iarray*)iupAttribGet(ih, "_IUPCOCOA_VISIBLEARRAY");
  if (!visible_array)
  {
    int i, count = IupGetChildCount(ih);
    visible_array = iupArrayCreate(count > 0 ? count : 1, sizeof(int));
    iupAttribSet(ih, "_IUPCOCOA_VISIBLEARRAY", (char*)visible_array);
    if (count > 0)
    {
      int* visible_data = (int*)iupArrayGetData(visible_array);
      for (i = 0; i < count; i++)
      {
        visible_data[i] = 1; // All visible by default
      }
    }
  }
  return visible_array;
}

static int cocoaTabsPosFixToNative(Ihandle* ih, int iup_pos)
{
  Iarray* visible_array = cocoaTabsGetVisibleArray(ih);
  int* visible_data = (int*)iupArrayGetData(visible_array);
  if (iup_pos < 0 || iup_pos >= iupArrayCount(visible_array) || !visible_data[iup_pos])
  {
    return -1;
  }

  int native_pos = 0;
  for (int i = 0; i < iup_pos; i++)
  {
    if (visible_data[i])
    {
      native_pos++;
    }
  }

  return native_pos;
}

static int cocoaTabsPosFixFromNative(Ihandle* ih, int native_pos)
{
  if (native_pos < 0)
  {
    return -1;
  }

  Iarray* visible_array = cocoaTabsGetVisibleArray(ih);
  int* visible_data = (int*)iupArrayGetData(visible_array);
  int count = iupArrayCount(visible_array);
  int current_native_pos = -1;

  for (int iup_pos = 0; iup_pos < count; iup_pos++)
  {
    if (visible_data[iup_pos])
    {
      current_native_pos++;
    }
    if (current_native_pos == native_pos)
    {
      return iup_pos;
    }
  }
  return -1;
}

int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvTabsExtraMargin(void)
{
  return 0;
}

int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  return 1;
}

void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
  if (!tab_view_controller)
  {
    return;
  }

  int native_pos = cocoaTabsPosFixToNative(ih, pos);
  if (native_pos < 0)
  {
    return;
  }

  iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
  [tab_view_controller setSelectedTabViewItemIndex:native_pos];
  tab_view_controller.previousSelectedIndex = native_pos;
  iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);
}

int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
  if (!tab_view_controller)
  {
    return -1;
  }

  int native_pos = (int)[tab_view_controller selectedTabViewItemIndex];
  return cocoaTabsPosFixFromNative(ih, native_pos);
}

int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  Ihandle* ih = IupGetParent(child);
  if (!ih) return 0;

  Iarray* visible_array = cocoaTabsGetVisibleArray(ih);
  if (pos < 0 || pos >= iupArrayCount(visible_array))
  {
    return 0;
  }

  return ((int*)iupArrayGetData(visible_array))[pos];
}

static int cocoaTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (!child) return 0;

  int is_visible = iupdrvTabsIsTabVisible(child, pos);
  int new_visible = iupStrBoolean(value);

  if (is_visible == new_visible) return 0;

  Iarray* visible_array = cocoaTabsGetVisibleArray(ih);
  int* visible_data = (int*)iupArrayGetData(visible_array);
  IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);

  if (new_visible)
  {
    visible_data[pos] = 1;
    cocoaTabsCreateAndInsertItem(ih, child, pos);
  }
  else
  {
    int native_pos = cocoaTabsPosFixToNative(ih, pos);
    iupTabsCheckCurrentTab(ih, pos, 0);
    visible_data[pos] = 0;

    if (native_pos >= 0 && (NSUInteger)native_pos < [[tab_view_controller tabViewItems] count])
    {
      NSTabViewItem* tab_view_item = [[tab_view_controller tabViewItems] objectAtIndex:native_pos];
      iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
      [tab_view_controller removeTabViewItem:tab_view_item];
      iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);
      tab_view_controller.previousSelectedIndex = [tab_view_controller selectedTabViewItemIndex];
    }
  }
  IupRefresh(ih);
  return 0;
}

static char* cocoaTabsGetMultilineAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->is_multiline);
}

static int cocoaTabsSetMultilineAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  /* The MULTILINE attribute behavior is dictated by TABTYPE/TABORIENTATION on Cocoa.
     Horizontal tabs are strictly single line (they scroll).
     Vertical tabs must report as multiline for the IUP core to calculate the natural size correctly (stacked height).
  */
  if (ih->data->orientation == ITABS_VERTICAL)
  {
    ih->data->is_multiline = 1;
  }
  else
  {
    ih->data->is_multiline = 0;
  }

  return 0;
}

static int cocoaTabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
  NSTabViewControllerTabStyle new_style;
  NSTabViewType new_bezel_type = NSTopTabsBezelBorder;

  if (iupStrEqualNoCase(value, "BOTTOM"))
  {
    ih->data->type = ITABS_BOTTOM;
    ih->data->orientation = ITABS_HORIZONTAL;
    ih->data->is_multiline = 0;
    new_style = NSTabViewControllerTabStyleSegmentedControlOnBottom;
  }
  else if (iupStrEqualNoCase(value, "LEFT"))
  {
    ih->data->type = ITABS_LEFT;
    ih->data->orientation = ITABS_VERTICAL;
    ih->data->is_multiline = 1;
    new_style = NSTabViewControllerTabStyleUnspecified;
    new_bezel_type = NSLeftTabsBezelBorder;
  }
  else if (iupStrEqualNoCase(value, "RIGHT"))
  {
    ih->data->type = ITABS_RIGHT;
    ih->data->orientation = ITABS_VERTICAL;
    ih->data->is_multiline = 1;
    new_style = NSTabViewControllerTabStyleUnspecified;
    new_bezel_type = NSRightTabsBezelBorder;
  }
  else
  {
    ih->data->type = ITABS_TOP;
    ih->data->orientation = ITABS_HORIZONTAL;
    ih->data->is_multiline = 0;
    new_style = NSTabViewControllerTabStyleSegmentedControlOnTop;
  }

  if (ih->handle)
  {
    IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
    NSTabView* tab_view = [tab_view_controller tabView];

    [tab_view_controller setTabStyle:new_style];

    if (new_style == NSTabViewControllerTabStyleUnspecified)
    {
      [tab_view setTabViewType:new_bezel_type];
    }

    [tab_view invalidateIntrinsicContentSize];
    [tab_view setNeedsLayout:YES];
    [tab_view setNeedsDisplay:YES];

    [[tab_view_controller view] invalidateIntrinsicContentSize];
    [[tab_view_controller view] setNeedsLayout:YES];
    [[tab_view_controller view] setNeedsDisplay:YES];

    NSView* parent_view = iupCocoaCommonBaseLayoutGetParentView(ih);
    if (parent_view)
    {
      [parent_view setNeedsLayout:YES];
    }
  }
  return 0;
}

static int cocoaTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child) iupAttribSetStr(child, "TABTITLE", value);

  int native_pos = cocoaTabsPosFixToNative(ih, pos);
  if (native_pos < 0) return 0;

  IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
  if ((NSUInteger)native_pos >= [[tab_view_controller tabViewItems] count]) return 0;

  NSTabViewItem* tab_view_item = [[tab_view_controller tabViewItems] objectAtIndex:native_pos];
  NSString* tab_title = @"";
  if (value)
  {
    char* stripped_str = iupStrProcessMnemonic(value, NULL, 0);
    tab_title = [NSString stringWithUTF8String:stripped_str];
    if (stripped_str && stripped_str != value) free(stripped_str);
  }
  [tab_view_item setLabel:tab_title];

  IupRefresh(ih);
  return 0;
}

static int cocoaTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child) iupAttribSetStr(child, "TABIMAGE", value);

  int native_pos = cocoaTabsPosFixToNative(ih, pos);
  if (native_pos < 0) return 1;

  IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
  if ((NSUInteger)native_pos >= [[tab_view_controller tabViewItems] count]) return 1;

  NSTabViewItem* tab_view_item = [[tab_view_controller tabViewItems] objectAtIndex:native_pos];
  NSImage* bitmap_image = value ? iupImageGetImage(value, ih, 0, NULL) : nil;
  [tab_view_item setImage:bitmap_image];

  IupRefresh(ih);
  return 1;
}

static int cocoaTabsCreateAndInsertItem(Ihandle* ih, Ihandle* child, int iup_pos)
{
  IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
  int native_pos = cocoaTabsPosFixToNative(ih, iup_pos);
  if (native_pos < 0) return -1;

  NSViewController* view_controller = [[[NSViewController alloc] initWithNibName:nil bundle:nil] autorelease];
  NSView* content_view = [[[NSView alloc] initWithFrame:NSZeroRect] autorelease];

  [view_controller setView:content_view];
  NSTabViewItem* tab_item = [NSTabViewItem tabViewItemWithViewController:view_controller];

  iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
  [tab_view_controller insertTabViewItem:tab_item atIndex:native_pos];
  iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);

  iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)content_view);

  char* attr_val = iupAttribGet(child, "TABTITLE");
  if (!attr_val) attr_val = iupAttribGetId(ih, "TABTITLE", iup_pos);
  cocoaTabsSetTabTitleAttrib(ih, iup_pos, attr_val);

  attr_val = iupAttribGet(child, "TABIMAGE");
  if (!attr_val) attr_val = iupAttribGetId(ih, "TABIMAGE", iup_pos);
  cocoaTabsSetTabImageAttrib(ih, iup_pos, attr_val);

  return 0;
}

static void cocoaTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (!iupAttribGetHandleName(child)) iupAttribSetHandleName(child);

  if (ih->handle)
  {
    Iarray* visible_array = cocoaTabsGetVisibleArray(ih);
    int pos = IupGetChildPos(ih, child);
    iupArrayInsert(visible_array, pos, 1);
    ((int*)iupArrayGetData(visible_array))[pos] = 1;

    cocoaTabsCreateAndInsertItem(ih, child, pos);
    IupRefresh(ih);
  }
}

static void cocoaTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (!ih->handle) return;

  NSView* content_view = (NSView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
  if (!content_view) return;

  IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
  Iarray* visible_array = cocoaTabsGetVisibleArray(ih);

  int native_pos = -1;
  int is_visible = ((int*)iupArrayGetData(visible_array))[pos];
  if (is_visible)
  {
    native_pos = cocoaTabsPosFixToNative(ih, pos);
  }

  iupArrayRemove(visible_array, pos, 1);

  if (native_pos >= 0)
  {
    iupTabsCheckCurrentTab(ih, pos, 1);

    NSArray* array_of_items = [tab_view_controller tabViewItems];
    if ((NSUInteger)native_pos < [array_of_items count])
    {
      NSTabViewItem* tab_view_item = [array_of_items objectAtIndex:native_pos];

      iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
      [tab_view_controller removeTabViewItem:tab_view_item];
      iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);

      tab_view_controller.previousSelectedIndex = [tab_view_controller selectedTabViewItemIndex];
    }
  }

  iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
  IupRefresh(ih);
}

static int cocoaTabsMapMethod(Ihandle* ih)
{
  IupTabViewController* tab_view_controller = [[IupTabViewController alloc] init];
  [tab_view_controller setCanPropagateSelectedChildViewControllerTitle:NO];

  objc_setAssociatedObject(tab_view_controller, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
  ih->handle = tab_view_controller;

  [tab_view_controller view];

  object_setClass([tab_view_controller tabView], [IupTabView class]);
  objc_setAssociatedObject([tab_view_controller tabView], IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

  iupCocoaSetAssociatedViews(ih, [tab_view_controller view], [tab_view_controller view]);
  cocoaTabsGetVisibleArray(ih);

  cocoaTabsSetTabTypeAttrib(ih, iupTabsGetTabTypeAttrib(ih));
  iupCocoaAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    [[tab_view_controller tabView] setRefusesFirstResponder:YES];
    iupCocoaSetCanFocus(ih, 0);
  }
  else
  {
    iupCocoaSetCanFocus(ih, 1);
  }

  if (ih->firstchild)
  {
    Ihandle* child;
    Ihandle* current_child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");

    for (child = ih->firstchild; child; child = child->brother)
    {
      cocoaTabsChildAddedMethod(ih, child);
    }

    if (current_child)
    {
      IupSetAttribute(ih, "VALUE_HANDLE", (char*)current_child);
      iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", NULL);
    }

    IupRefresh(ih);
  }

  return IUP_NOERROR;
}

static void cocoaTabsUnMapMethod(Ihandle* ih)
{
  id tab_control = ih->handle;
  if (!tab_control) return;

  Iarray* visible_array = (Iarray*)iupAttribGet(ih, "_IUPCOCOA_VISIBLEARRAY");
  if (visible_array)
  {
    iupArrayDestroy(visible_array);
    iupAttribSet(ih, "_IUPCOCOA_VISIBLEARRAY", NULL);
  }

  iupCocoaRemoveFromParent(ih);
  iupCocoaSetAssociatedViews(ih, nil, nil);
  [tab_control release];
  ih->handle = NULL;
}

static void cocoaTabsLayoutUpdateMethod(Ihandle* ih)
{
  NSView* parent_view = iupCocoaCommonBaseLayoutGetParentView(ih);
  if (!parent_view) return;

  NSView* child_view = iupCocoaCommonBaseLayoutGetChildView(ih);
  if (!child_view) return;

  NSRect parent_bounds = [parent_view bounds];
  NSRect child_rect;

  if ([parent_view isFlipped])
  {
    child_rect = NSMakeRect(
        ih->x,
        ih->y,
        ih->currentwidth,
        ih->currentheight
    );
  }
  else
  {
    child_rect = NSMakeRect(
        ih->x,
        parent_bounds.size.height - ih->y - ih->currentheight,
        ih->currentwidth,
        ih->currentheight
    );
  }

  [child_view setFrame:child_rect];

  iupdrvUpdateTip(ih);
}

void iupdrvTabsInitClass(Iclass* ic)
{
  ic->Map = cocoaTabsMapMethod;
  ic->UnMap = cocoaTabsUnMapMethod;
  ic->LayoutUpdate = cocoaTabsLayoutUpdateMethod;
  ic->ChildAdded = cocoaTabsChildAddedMethod;
  ic->ChildRemoved = cocoaTabsChildRemovedMethod;

  iupClassRegisterCallback(ic, "RIGHTCLICK_CB", "i");
  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");

  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, cocoaTabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, cocoaTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, cocoaTabsSetTabImageAttrib, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, cocoaTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupCocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);

  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABPADDING", NULL, NULL, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTILINE", cocoaTabsGetMultilineAttrib, cocoaTabsSetMultilineAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCLOSE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
