/*
 * This implementation uses the custom IupCocoaTabBarView library to provide the tab bar UI,
 * and a custom NSView (IupTabsRootView) to manage the content area.
 * This approach allows for customization of fonts, colors, images and close button.
 *
 * Supported features:
 * - TABTYPE: TOP, BOTTOM, LEFT, RIGHT
 * - TABIMAGE, TABTITLE, TABVISIBLE
 * - FONT, FGCOLOR, BGCOLOR
 * - RIGHTCLICK_CB, TABCHANGE_CB, TABCLOSE_CB callbacks
 * - SHOWCLOSE
 * - TABDRAGGABLE
 * - TABLIST
 *
 * Attributes NOT SUPPORTED:
 * - TABPADDING: IupCocoaTabBarView does not expose padding properties.
 * - MULTILINE: Tabs become scrollable instead of wrapping.
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
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_tabs.h"
#include "iup_array.h"
#include "iup_image.h"

#include "iupcocoa_drv.h"
#include "IupCocoaTabBarView.h"


// Forward declarations
static int cocoaTabsPosFixFromNative(Ihandle* ih, int native_pos);
static int cocoaTabsCreateAndInsertItem(Ihandle* ih, Ihandle* child, int iup_pos);
static void cocoaTabsHideShowPage(Ihandle* ih, int old_pos, int new_pos, int is_native);


/*
   ===============================================================================
 * IupTabsRootView:
 * A custom view that holds the IupCocoaTabBarView and the content area.
 * It manages the layout of these two subviews based on the TABTYPE.
 ===============================================================================
 */
@interface IupTabsRootView : NSView
{
  IupCocoaTabBarView* _tabBarView;
  NSView* _contentAreaView;
}
@property(nonatomic, retain) IupCocoaTabBarView* tabBarView;
@property(nonatomic, retain) NSView* contentAreaView;
- (Ihandle*)ihandle;
- (void)layout;
@end

// Private methods
@interface IupTabsRootView ()
  - (void)_handleRightMouseDownOnTabCell:(IupCocoaTabCell *)tab_cell cb:(IFni)cb ih:(Ihandle *)ih event:(NSEvent *)event;
  @end

  @implementation IupTabsRootView
  @synthesize tabBarView = _tabBarView;
  @synthesize contentAreaView = _contentAreaView;

  - (void)dealloc
{
  self.tabBarView = nil;
  self.contentAreaView = nil;
  [super dealloc];
}

- (Ihandle*)ihandle
{
  return (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
}

- (BOOL)isFlipped
{
  return YES;
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize
{
  [super resizeSubviewsWithOldSize:oldSize];
  [self layout];
}

// The layout method is the core of the composite control.
// It positions the tab bar and content area based on ih->data->type.
- (void)layout
{
  [super layout];

  Ihandle* ih = [self ihandle];
  if (!ih) return;

  NSRect bounds = [self bounds];
  NSRect tab_bar_frame = NSZeroRect;
  NSRect content_frame = NSZeroRect;

  // We use kTabBarViewHeight from IupCocoaTabBarView.h as the thickness for horizontal tab bars.
  // For vertical, we use kMinTabCellWidth as the thickness.
  const CGFloat kVerticalTabBarWidth = kMinTabCellWidth;

  switch (ih->data->type) {
    case ITABS_BOTTOM:
      tab_bar_frame = NSMakeRect(0, bounds.size.height - kTabBarViewHeight, bounds.size.width, kTabBarViewHeight);
      content_frame = NSMakeRect(0, 0, bounds.size.width, bounds.size.height - kTabBarViewHeight);
      [self.tabBarView setOrientation:IupCocoaTabBarHorizontal];
      break;
    case ITABS_LEFT:
      tab_bar_frame = NSMakeRect(0, 0, kVerticalTabBarWidth, bounds.size.height);
      content_frame = NSMakeRect(kVerticalTabBarWidth, 0, bounds.size.width - kVerticalTabBarWidth, bounds.size.height);
      [self.tabBarView setOrientation:IupCocoaTabBarVertical];
      break;
    case ITABS_RIGHT:
      tab_bar_frame = NSMakeRect(bounds.size.width - kVerticalTabBarWidth, 0, kVerticalTabBarWidth, bounds.size.height);
      content_frame = NSMakeRect(0, 0, bounds.size.width - kVerticalTabBarWidth, bounds.size.height);
      [self.tabBarView setOrientation:IupCocoaTabBarVertical];
      break;
    case ITABS_TOP:
           default:
      tab_bar_frame = NSMakeRect(0, 0, bounds.size.width, kTabBarViewHeight);
      content_frame = NSMakeRect(0, kTabBarViewHeight, bounds.size.width, bounds.size.height - kTabBarViewHeight);
      [self.tabBarView setOrientation:IupCocoaTabBarHorizontal];
      break;
  }

  [self.tabBarView setFrame:tab_bar_frame];
  [self.contentAreaView setFrame:content_frame];

  iupcocoaCommonBaseLayoutGetChildView(ih);
  iupdrvBaseLayoutUpdateMethod(ih);
}

- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if (result)
  {
    Ihandle* ih = [self ihandle];
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
    Ihandle* ih = [self ihandle];
    if (ih)
      iupcocoaFocusOut(ih);
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

- (void)keyDown:(NSEvent *)event
{
  Ihandle* ih = [self ihandle];
  if (ih)
  {
    int mac_key_code = [event keyCode];
    if (!iupcocoaKeyEvent(ih, event, mac_key_code, true))
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
    if (!iupcocoaKeyEvent(ih, event, mac_key_code, false))
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
    if (!iupcocoaModifierEvent(ih, event, mac_key_code))
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

  // Check if the click is inside the tab bar
  NSPoint pointInWindow = [event locationInWindow];
  NSPoint pointInTabBar = [self.tabBarView convertPoint:pointInWindow fromView:nil];

  if (NSPointInRect(pointInTabBar, [self.tabBarView bounds]))
  {
    IupCocoaTabCell* tab_cell = [self.tabBarView tabCellInPoint:pointInTabBar];
    if (tab_cell)
    {
      // Call the helper method using Objective-C syntax
      [self _handleRightMouseDownOnTabCell:tab_cell cb:cb ih:ih event:event];
    }
    else
      [super rightMouseDown:event];
  }
  else
  {
    [super rightMouseDown:event];
  }
}

// Handles the logic for a right-click on a specific tab
- (void)_handleRightMouseDownOnTabCell:(IupCocoaTabCell *)tab_cell cb:(IFni)cb ih:(Ihandle *)ih event:(NSEvent *)event
{
  // 'self' here now correctly refers to the IupTabsRootView instance
  NSInteger native_pos = [[self.tabBarView tabs] indexOfObject:tab_cell];
  if (native_pos != NSNotFound)
  {
    int iup_pos = cocoaTabsPosFixFromNative(ih, (int)native_pos);
    if (iup_pos != -1)
    {
      cb(ih, iup_pos);
    }
  }
  else
  {
    // This case should be unlikely if tab_cell was found, but forward just in case.
    [super rightMouseDown:event];
  }
}

@end


/*
   ===============================================================================
 * IupTabsDelegate:
 * Handles callbacks from the IupCocoaTabBarView and translates them to IUP.
 ===============================================================================
 */
@interface IupTabsDelegate : NSObject <IupCocoaTabBarViewDelegate, NSMenuDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) int previousIupPos;
@end

@implementation IupTabsDelegate

// A tab was just selected
- (void)tabDidActived:(IupCocoaTabCell*)tab
{
  Ihandle* ih = self.ihandle;
  if (!ih || !tab) return;

  if (iupAttribGet(ih, "_IUPCOCOA_IGNORE_CHANGE"))
  {
    return;
  }

  IupCocoaTabBarView* tab_bar_view = [(IupTabsRootView*)(ih->handle) tabBarView];
  int native_pos = (int)[[tab_bar_view tabs] indexOfObject:tab];
  int pos = cocoaTabsPosFixFromNative(ih, native_pos);

  int prev_pos = self.previousIupPos;
  if (pos == prev_pos) return;

  // Show/Hide the respective content views
  cocoaTabsHideShowPage(ih, prev_pos, pos, 0);

  // Fire IUP callbacks
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

  self.previousIupPos = pos;
}

// User clicked the close button
- (void)tabWillClose:(IupCocoaTabCell*)tab
{
  Ihandle* ih = self.ihandle;
  if (!ih || !tab) return;

  IupCocoaTabBarView* tab_bar_view = [(IupTabsRootView*)(ih->handle) tabBarView];
  int native_pos = (int)[[tab_bar_view tabs] indexOfObject:tab];
  int iup_pos = cocoaTabsPosFixFromNative(ih, native_pos);
  if (iup_pos == -1) return;

  Ihandle* child = IupGetChild(ih, iup_pos);
  if (!child) return;

  IFni cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
  int ret = IUP_DEFAULT;
  if (cb)
  {
    ret = cb(ih, iup_pos);
  }

  if (ret == IUP_CONTINUE) // destroy tab and children
  {
    IupDestroy(child);
    IupRefreshChildren(ih);
  }
  else if (ret == IUP_DEFAULT) // hide tab and children
  {
    iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
    IupSetAttributeId(ih, "TABVISIBLE", iup_pos, "NO");
    iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);
  }
  else if (ret == IUP_IGNORE)
  {
    // Do nothing, but we must manually tell the tab bar not to close
    // by *not* calling [tabBarView removeTabCell:tab];
  }
}

// IupCocoaTabBarView's default is to close *after* tabWillClose.
// We override tabDidClosed to prevent this default,
// as our tabWillClose already handled all logic.
- (void)tabDidClosed:(IupCocoaTabCell*)tab
{
  // Do nothing. This prevents IupCocoaTabBarView from removing the cell
  // automatically. We handled removal inside tabWillClose based
  // on the IUP_CONTINUE/IUP_DEFAULT/IUP_IGNORE logic.
}

@end


/*
   ===============================================================================
 * Helper Functions
 ===============================================================================
 */

static IupTabsRootView* cocoaGetRootView(Ihandle* ih)
{
  if (ih && ih->handle)
  {
    IupTabsRootView* root_view = (IupTabsRootView*)ih->handle;
    NSCAssert([root_view isKindOfClass:[IupTabsRootView class]], @"Expected IupTabsRootView");
    return root_view;
  }
  return nil;
}

static IupCocoaTabBarView* cocoaGetTabBarView(Ihandle* ih)
{
  IupTabsRootView* root_view = cocoaGetRootView(ih);
  if (root_view)
  {
    return [root_view tabBarView];
  }
  return nil;
}

static NSView* cocoaGetContentAreaView(Ihandle* ih)
{
  IupTabsRootView* root_view = cocoaGetRootView(ih);
  if (root_view)
  {
    return [root_view contentAreaView];
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

// Hides the old page and shows the new one
static void cocoaTabsHideShowPage(Ihandle* ih, int old_pos, int new_pos, int is_native)
{
  int iup_old_pos = old_pos;
  int iup_new_pos = new_pos;

  if (is_native)
  {
    iup_old_pos = cocoaTabsPosFixFromNative(ih, old_pos);
    iup_new_pos = cocoaTabsPosFixFromNative(ih, new_pos);
  }

  if (iup_old_pos >= 0)
  {
    Ihandle* old_child = IupGetChild(ih, iup_old_pos);
    if(old_child)
    {
      NSView* old_container = (NSView*)iupAttribGet(old_child, "_IUPTAB_CONTAINER");
      if (old_container) [old_container setHidden:YES];
    }
  }

  if (iup_new_pos >= 0)
  {
    Ihandle* new_child = IupGetChild(ih, iup_new_pos);
    if(new_child)
    {
      NSView* new_container = (NSView*)iupAttribGet(new_child, "_IUPTAB_CONTAINER");
      if (new_container) [new_container setHidden:NO];
    }
  }
}

/*
   ===============================================================================
 * IUP Driver Functions
 ===============================================================================
 */

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
  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (!tab_bar_view)
  {
    return;
  }

  int native_pos = cocoaTabsPosFixToNative(ih, pos);
  if (native_pos < 0 || (NSUInteger)native_pos >= [[tab_bar_view tabs] count])
  {
    return;
  }

  IupCocoaTabCell* tab_cell = [[tab_bar_view tabs] objectAtIndex:native_pos];

  iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
  [tab_cell setAsActiveTab];

  // setAsActiveTab calls the delegate, but we need to manually
  // update the previous position and show/hide the page
  // because the "IGNORE_CHANGE" flag is set.
  IupTabsDelegate* delegate = (IupTabsDelegate*)[tab_bar_view delegate];
  int old_pos = delegate.previousIupPos;
  delegate.previousIupPos = pos;
  cocoaTabsHideShowPage(ih, old_pos, pos, 0);

  iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);
}

int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (!tab_bar_view)
  {
    return -1;
  }

  IupCocoaTabCell* selected_tab = [tab_bar_view selectedTab];
  if (!selected_tab)
  {
    return -1;
  }

  int native_pos = (int)[[tab_bar_view tabs] indexOfObject:selected_tab];
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

/*
   ===============================================================================
 * IUP Attribute Setters
 ===============================================================================
 */

static int cocoaTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (!child) return 0;

  int is_visible = iupdrvTabsIsTabVisible(child, pos);
  int new_visible = iupStrBoolean(value);

  if (is_visible == new_visible) return 0;

  Iarray* visible_array = cocoaTabsGetVisibleArray(ih);
  int* visible_data = (int*)iupArrayGetData(visible_array);
  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);

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

    if (native_pos >= 0 && (NSUInteger)native_pos < [[tab_bar_view tabs] count])
    {
      IupCocoaTabCell* tab_cell = [[tab_bar_view tabs] objectAtIndex:native_pos];

      // We must use the delegate to properly update the selected tab
      IupTabsDelegate* delegate = (IupTabsDelegate*)[tab_bar_view delegate];
      if (delegate)
      {
        iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
        [delegate tabWillClose:tab_cell]; // This will call IUP_DEFAULT logic
        iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);

        // tabWillClose (with IUP_DEFAULT) calls removeTabCell,
        // which updates the selection and fires tabDidActived.
        // We need to update our delegate's internal state.
        int new_native_pos = (int)[[tab_bar_view tabs] indexOfObject:[tab_bar_view selectedTab]];
        delegate.previousIupPos = cocoaTabsPosFixFromNative(ih, new_native_pos);
      }
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
  // MULTILINE is not supported by IupCocoaTabBarView.
  // We set the internal flag based on orientation, as this is used
  // by the IUP core for layout calculations.
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
  if (iupStrEqualNoCase(value, "BOTTOM"))
  {
    ih->data->type = ITABS_BOTTOM;
    ih->data->orientation = ITABS_HORIZONTAL;
    ih->data->is_multiline = 0;
  }
  else if (iupStrEqualNoCase(value, "LEFT"))
  {
    ih->data->type = ITABS_LEFT;
    ih->data->orientation = ITABS_VERTICAL;
    ih->data->is_multiline = 1;
  }
  else if (iupStrEqualNoCase(value, "RIGHT"))
  {
    ih->data->type = ITABS_RIGHT;
    ih->data->orientation = ITABS_VERTICAL;
    ih->data->is_multiline = 1;
  }
  else
  {
    ih->data->type = ITABS_TOP;
    ih->data->orientation = ITABS_HORIZONTAL;
    ih->data->is_multiline = 0;
  }

  if (ih->handle)
  {
    IupTabsRootView* root_view = (IupTabsRootView*)ih->handle;
    [root_view layout];
  }
  return 0;
}

static int cocoaTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child) iupAttribSetStr(child, "TABTITLE", value);

  int native_pos = cocoaTabsPosFixToNative(ih, pos);
  if (native_pos < 0) return 0;

  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (!tab_bar_view || (NSUInteger)native_pos >= [[tab_bar_view tabs] count]) return 0;

  IupCocoaTabCell* tab_cell = [[tab_bar_view tabs] objectAtIndex:native_pos];
  NSString* tab_title = @"";
  if (value)
  {
    char* stripped_str = iupStrProcessMnemonic(value, NULL, 0);
    tab_title = [NSString stringWithUTF8String:stripped_str];
    if (stripped_str && stripped_str != value) free(stripped_str);
  }
  [tab_cell setTitle:tab_title];

  [tab_bar_view redraw];
  return 0;
}

static int cocoaTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child) iupAttribSetStr(child, "TABIMAGE", value);

  int native_pos = cocoaTabsPosFixToNative(ih, pos);
  if (native_pos < 0) return 1;

  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (!tab_bar_view || (NSUInteger)native_pos >= [[tab_bar_view tabs] count]) return 1;

  IupCocoaTabCell* tab_cell = [[tab_bar_view tabs] objectAtIndex:native_pos];
  NSImage* bitmap_image = value ? iupImageGetImage(value, ih, 0, NULL) : nil;
  [tab_cell setImage:bitmap_image];

  [tab_bar_view redraw];
  return 1;
}

static int cocoaTabsSetShowCloseAttrib(Ihandle* ih, int pos, const char* value)
{
  if (pos == -1) // Global attribute
  {
    int i, count = IupGetChildCount(ih);
    for (i = 0; i < count; i++)
    {
      // Call self recursively for each child
      cocoaTabsSetShowCloseAttrib(ih, i, value);
    }
    // Also set the default for future children
    iupAttribSetStr(ih, "SHOWCLOSE", value);
    return 1; // Mark as processed
  }

  Ihandle* child = IupGetChild(ih, pos);
  if (child) iupAttribSetStr(child, "SHOWCLOSE", value);

  int native_pos = cocoaTabsPosFixToNative(ih, pos);
  if (native_pos < 0) return 0; // Not visible, nothing to do

  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (!tab_bar_view || (NSUInteger)native_pos >= [[tab_bar_view tabs] count]) return 0;

  IupCocoaTabCell* tab_cell = [[tab_bar_view tabs] objectAtIndex:native_pos];
  if (iupStrBoolean(value))
  {
    [tab_cell setHasCloseButton:YES];
  }
  else
  {
    [tab_cell setHasCloseButton:NO];
  }

  [tab_bar_view redraw];
  return 1;
}

static int cocoaTabsSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
  {
    return 0;
  }

  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (tab_bar_view)
  {
    IupCocoaFont* iup_font = iupcocoaGetFont(ih);
    if (iup_font)
    {
      [tab_bar_view setTabFont:iup_font.nativeFont];
      [tab_bar_view redraw];
    }
  }
  return 1;
}

static int cocoaTabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (tab_bar_view)
  {
    CGFloat red = r/255.0;
    CGFloat green = g/255.0;
    CGFloat blue = b/255.0;
    NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];

    [tab_bar_view setTabTitleColor:the_color];
    [tab_bar_view setTabActivedTitleColor:the_color];
    [tab_bar_view redraw];
  }
  return 1;
}

static int cocoaTabsSetTabDraggableAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (tab_bar_view)
  {
    if (iupStrBoolean(value))
    {
      [tab_bar_view setAllowsDragging:YES];
    }
    else
    {
      [tab_bar_view setAllowsDragging:NO];
    }
  }
  return 1;
}

static int cocoaTabsSetTabListAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (tab_bar_view)
  {
    if (iupStrBoolean(value))
    {
      [tab_bar_view setAllowsTabListMenu:YES];
    }
    else
    {
      [tab_bar_view setAllowsTabListMenu:NO];
    }
    [tab_bar_view redraw];
  }
  return 1;
}

static int cocoaTabsSetCloseButtonOnHoverAttrib(Ihandle* ih, const char* value)
{
  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (tab_bar_view)
  {
    if (iupStrBoolean(value))
    {
      [tab_bar_view setShowsCloseButtonOnHover:YES];
    }
    else
    {
      [tab_bar_view setShowsCloseButtonOnHover:NO];
    }
    [tab_bar_view redraw];
  }
  return 1;
}

static int cocoaTabsSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  if (tab_bar_view)
  {
    CGFloat red = r/255.0;
    CGFloat green = g/255.0;
    CGFloat blue = b/255.0;
    NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];

    // Set all three color properties for a consistent look
    [tab_bar_view setBgColor:the_color];
    [tab_bar_view setTabBGColor:the_color];
    [tab_bar_view setTabActivedBGColor:the_color];
    [tab_bar_view redraw];
  }
  return 1;
}


/*
   ===============================================================================
 * IUP Methods
 ===============================================================================
 */

static int cocoaTabsCreateAndInsertItem(Ihandle* ih, Ihandle* child, int iup_pos)
{
  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  int native_pos = cocoaTabsPosFixToNative(ih, iup_pos);
  if (native_pos < 0) return -1;

  char* title = iupAttribGet(child, "TABTITLE");
  if (!title) title = iupAttribGetId(ih, "TABTITLE", iup_pos);

  char* image_name = iupAttribGet(child, "TABIMAGE");
  if (!image_name) image_name = iupAttribGetId(ih, "TABIMAGE", iup_pos);

  char* show_close = iupAttribGet(child, "SHOWCLOSE");
  if (!show_close) show_close = iupAttribGet(ih, "SHOWCLOSE");

  NSString* ns_title = @"";
  if (title)
  {
    char* stripped_str = iupStrProcessMnemonic(title, NULL, 0);
    ns_title = [NSString stringWithUTF8String:stripped_str];
    if (stripped_str && stripped_str != title) free(stripped_str);
  }

  NSImage* ns_image = image_name ? iupImageGetImage(image_name, ih, 0, NULL) : nil;

  IupCocoaTabCell *tab_cell = [IupCocoaTabCell tabCellWithTabBarView:tab_bar_view title:ns_title image:ns_image];

  if (iupStrBoolean(show_close))
  {
    [tab_cell setHasCloseButton:YES];
  }

  id<IupCocoaTabBarViewDelegate> delegate = [tab_bar_view delegate];

  if ([delegate respondsToSelector:@selector(tabWillBeCreated:)])
  {
    [delegate tabWillBeCreated:tab_cell];
  }

  iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
  [[tab_bar_view tabs] insertObject:tab_cell atIndex:native_pos];

  if ([[tab_bar_view tabs] count] == 1)
  {
    [tab_cell setAsActiveTab];
    if ([delegate isKindOfClass:[IupTabsDelegate class]])
    {
      [(IupTabsDelegate*)delegate setPreviousIupPos:iup_pos];
    }
  }
  iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);

  if ([delegate respondsToSelector:@selector(tabDidBeCreated:)])
  {
    [delegate tabDidBeCreated:tab_cell];
  }

  [tab_bar_view redraw];

  return 0;
}

static void cocoaTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (!iupAttribGetHandleName(child)) iupAttribSetHandleName(child);

  if (ih->handle)
  {
    // Add to visibility array
    Iarray* visible_array = cocoaTabsGetVisibleArray(ih);
    int pos = IupGetChildPos(ih, child);
    iupArrayInsert(visible_array, pos, 1);
    ((int*)iupArrayGetData(visible_array))[pos] = 1;

    // Create the native tab cell
    cocoaTabsCreateAndInsertItem(ih, child, pos);

    // Create the content container view
    NSView* content_area = cocoaGetContentAreaView(ih);
    NSView* content_container = [[[NSView alloc] initWithFrame:[content_area bounds]] autorelease];
    [content_container setHidden:YES]; // Hide by default

    [content_area addSubview:content_container];
    iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)content_container);

    IupRefresh(ih);
  }
}

static void cocoaTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (!ih->handle) return;

  NSView* content_container = (NSView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
  IupCocoaTabBarView* tab_bar_view = cocoaGetTabBarView(ih);
  Iarray* visible_array = cocoaTabsGetVisibleArray(ih);

  int is_visible = ((int*)iupArrayGetData(visible_array))[pos];

  if (is_visible)
  {
    int native_pos = cocoaTabsPosFixToNative(ih, pos);
    if (native_pos >= 0 && (NSUInteger)native_pos < [[tab_bar_view tabs] count])
    {
      iupTabsCheckCurrentTab(ih, pos, 1);

      IupCocoaTabCell* tab_cell = [[tab_bar_view tabs] objectAtIndex:native_pos];

      iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
      // removeTabCell handles re-activating another tab
      [tab_bar_view removeTabCell:tab_cell];
      iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);

      // Update the delegate's internal state
      IupTabsDelegate* delegate = (IupTabsDelegate*)[tab_bar_view delegate];
      if (delegate)
      {
        int new_native_pos = (int)[[tab_bar_view tabs] indexOfObject:[tab_bar_view selectedTab]];
        delegate.previousIupPos = cocoaTabsPosFixFromNative(ih, new_native_pos);
      }
    }
  }

  if (content_container)
  {
    [content_container removeFromSuperview];
  }

  iupArrayRemove(visible_array, pos, 1);
  iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);

  [tab_bar_view redraw];
  IupRefresh(ih);
}

static int cocoaTabsMapMethod(Ihandle* ih)
{
  // Create the root view
  IupTabsRootView* root_view = [[IupTabsRootView alloc] initWithFrame:NSZeroRect];
  ih->handle = root_view; // DO NOT release root_view, ih->handle owns it
  objc_setAssociatedObject(root_view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

  // Create the Tab Bar
  IupCocoaTabBarView* tab_bar_view = [[IupCocoaTabBarView alloc] initWithFrame:NSZeroRect];
  [root_view setTabBarView:tab_bar_view];
  [root_view addSubview:tab_bar_view];
  [tab_bar_view release];

  // Create the Content Area
  NSView* content_area_view = [[NSView alloc] initWithFrame:NSZeroRect];
  [root_view setContentAreaView:content_area_view];
  [root_view addSubview:content_area_view];
  [content_area_view release];

  // Create the Delegate
  IupTabsDelegate* delegate = [[IupTabsDelegate alloc] init];
  [delegate setIhandle:ih];
  [tab_bar_view setDelegate:delegate];
  // Store the delegate so we can release it on Unmap
  objc_setAssociatedObject(root_view, @"IUP_TABS_DELEGATE", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [delegate release];

  iupcocoaSetAssociatedViews(ih, root_view, root_view);
  cocoaTabsGetVisibleArray(ih);

  // Set attributes that must be set before children are added
  cocoaTabsSetTabTypeAttrib(ih, iupTabsGetTabTypeAttrib(ih));

  if (iupAttribGet(ih, "FONT"))
    cocoaTabsSetFontAttrib(ih, iupAttribGet(ih, "FONT"));
  if (iupAttribGet(ih, "FGCOLOR"))
    cocoaTabsSetFgColorAttrib(ih, iupAttribGet(ih, "FGCOLOR"));
  if (iupAttribGet(ih, "BGCOLOR"))
    cocoaTabsSetBgColorAttrib(ih, iupAttribGet(ih, "BGCOLOR"));
  if (iupAttribGet(ih, "TABDRAGGABLE"))
    cocoaTabsSetTabDraggableAttrib(ih, iupAttribGet(ih, "TABDRAGGABLE"));
  if (iupAttribGet(ih, "TABLIST"))
    cocoaTabsSetTabListAttrib(ih, iupAttribGet(ih, "TABLIST"));
  if (iupAttribGet(ih, "CLOSEBUTTONONHOVER"))
    cocoaTabsSetCloseButtonOnHoverAttrib(ih, iupAttribGet(ih, "CLOSEBUTTONONHOVER"));

  iupcocoaAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    iupcocoaSetCanFocus(ih, 0);
  }
  else
  {
    iupcocoaSetCanFocus(ih, 1);
  }

  if (ih->firstchild)
  {
    Ihandle* child;
    Ihandle* current_child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");
    int current_pos = -1, i = 0;

    if (current_child)
    {
      Ihandle* c = ih->firstchild;
      while (c)
      {
        if (c == current_child)
        {
          current_pos = i;
          break;
        }
        c = c->brother;
        i++;
      }
    }

    for (child = ih->firstchild; child; child = child->brother)
    {
      cocoaTabsChildAddedMethod(ih, child);
    }

    if (current_pos != -1)
    {
      iupdrvTabsSetCurrentTab(ih, current_pos);
      iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", NULL);
    }
    else
    {
      // Activate the first tab by default
      iupdrvTabsSetCurrentTab(ih, 0);
    }

    IupRefresh(ih);
  }

  return IUP_NOERROR;
}

static void cocoaTabsUnMapMethod(Ihandle* ih)
{
  id root_view = ih->handle;
  if (!root_view) return;

  // Release the delegate
  objc_setAssociatedObject(root_view, @"IUP_TABS_DELEGATE", nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

  Iarray* visible_array = (Iarray*)iupAttribGet(ih, "_IUPCOCOA_VISIBLEARRAY");
  if (visible_array)
  {
    iupArrayDestroy(visible_array);
    iupAttribSet(ih, "_IUPCOCOA_VISIBLEARRAY", NULL);
  }

  iupcocoaRemoveFromParent(ih);
  iupcocoaSetAssociatedViews(ih, nil, nil);
  [root_view release];
  ih->handle = NULL;
}

static void cocoaTabsLayoutUpdateMethod(Ihandle* ih)
{
  if (!ih->handle) return;

  iupdrvBaseLayoutUpdateMethod(ih);

  IupTabsRootView* root_view = (IupTabsRootView*)ih->handle;
  [root_view layout];

  NSView* content_area = cocoaGetContentAreaView(ih);
  if (!content_area) return;

  NSRect content_rect = [content_area bounds];

  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    NSView* child_container = (NSView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
    if (child_container)
    {
      [child_container setFrame:content_rect];
    }
  }
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

  iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupcocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);

  iupClassRegisterAttribute(ic, "FONT", NULL, cocoaTabsSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TABDRAGGABLE", NULL, cocoaTabsSetTabDraggableAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABLIST", NULL, cocoaTabsSetTabListAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SHOWCLOSE", NULL, cocoaTabsSetShowCloseAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLOSEBUTTONONHOVER", NULL, cocoaTabsSetCloseButtonOnHoverAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TABPADDING", NULL, NULL, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTILINE", cocoaTabsGetMultilineAttrib, cocoaTabsSetMultilineAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
