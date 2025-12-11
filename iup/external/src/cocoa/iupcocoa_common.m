/** \file
 * \brief Cocoa Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#import <CoreGraphics/CoreGraphics.h>
#import <unistd.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_loop.h"
#include "iup_drv.h"
#include "iup_menu.h"

#include "iupcocoa_drv.h"


const void* IHANDLE_ASSOCIATED_OBJ_KEY = @"IHANDLE_ASSOCIATED_OBJ_KEY";
const void* MAINVIEW_ASSOCIATED_OBJ_KEY = @"MAINVIEW_ASSOCIATED_OBJ_KEY";
const void* ROOTVIEW_ASSOCIATED_OBJ_KEY = @"ROOTVIEW_ASSOCIATED_OBJ_KEY";

NSObject* iupcocoaGetRootObject(Ihandle* ih)
{
  if(NULL == ih)
  {
    return nil;
  }
  return (NSObject*)ih->handle;
}

NSView* iupcocoaGetRootView(Ihandle* ih)
{
  if(NULL == ih)
  {
    return nil;
  }
  id root_object = (NSObject*)ih->handle;

  if(nil == root_object)
  {
    return nil;
  }

  NSView* root_view = (NSView*)objc_getAssociatedObject(root_object, ROOTVIEW_ASSOCIATED_OBJ_KEY);

  if (ih->iclass->nativetype == IUP_TYPEMENU || [root_object isKindOfClass:[NSStatusItem class]])
  {
    return nil;
  }

  NSCAssert([root_view isKindOfClass:[NSView class]], @"Expected NSView");
  return root_view;
}

NSView* iupcocoaGetMainView(Ihandle* ih)
{
  if(NULL == ih)
  {
    return nil;
  }
  id root_object = (NSObject*)ih->handle;

  if(nil == root_object)
  {
    return nil;
  }

  NSView* main_view = (NSView*)objc_getAssociatedObject(root_object, MAINVIEW_ASSOCIATED_OBJ_KEY);

  if (ih->iclass->nativetype == IUP_TYPEMENU || [root_object isKindOfClass:[NSStatusItem class]])
  {
    return nil;
  }

  NSCAssert([main_view isKindOfClass:[NSView class]], @"Expected NSView");
  return main_view;
}

void iupcocoaSetAssociatedViews(Ihandle* ih, NSView* main_view, NSView* root_view)
{
  NSCAssert(ih->handle, @"Expected ih->handle to be set");

  objc_setAssociatedObject((id)ih->handle, MAINVIEW_ASSOCIATED_OBJ_KEY, main_view, OBJC_ASSOCIATION_ASSIGN);
  objc_setAssociatedObject((id)ih->handle, ROOTVIEW_ASSOCIATED_OBJ_KEY, root_view, OBJC_ASSOCIATION_ASSIGN);
}

void iupcocoaAddToParent(Ihandle* ih)
{
  NSView* parent_view = nil;

  /* Check if this element is intended for a specific container within a composite control (e.g., IupTabs).
     This allows the native hierarchy (child inside container) to differ from the IUP hierarchy (child inside Tabs). */
  NSView* specific_container = (NSView*)iupAttribGet(ih, "_IUPTAB_CONTAINER");

  if (specific_container != NULL)
  {
    NSCAssert([specific_container isKindOfClass:[NSView class]], @"Expected specific container (e.g., _IUPTAB_CONTAINER) to be an NSView");
    parent_view = specific_container;
  }
  else
  {
    id parent_native_handle = iupChildTreeGetNativeParentHandle(ih);

    if (parent_native_handle == nil)
    {
      /* If parent_native_handle is nil, parent_view remains nil.
         This is expected for top-level elements like dialogs before mapping, or unparented elements. */
    }
    else if([parent_native_handle isKindOfClass:[NSWindow class]])
    {
      NSWindow* parent_window = (NSWindow*)parent_native_handle;
      parent_view = [parent_window contentView];
    }
    else if([parent_native_handle isKindOfClass:[NSBox class]])
    {
      parent_view = [(NSBox*)parent_native_handle contentView];
    }
    else if([parent_native_handle isKindOfClass:[NSView class]])
    {
      parent_view = (NSView*)parent_native_handle;
    }
    else if([parent_native_handle isKindOfClass:[NSViewController class]])
    {
      NSViewController* view_controller = (NSViewController*)parent_native_handle;
      parent_view = [view_controller view];
    }
    else if([parent_native_handle isKindOfClass:[NSStatusItem class]])
    {
      /* An NSStatusItem (tray icon) cannot contain a view. This is an invalid hierarchy.
         We simply ignore the addition.
         Menus are handled via the TRAYMENU attribute and not via child tree addition. */
      return;
    }
    else
    {
      if (ih->iclass->nativetype != IUP_TYPEMENU)
      {
        NSCAssert(0, @"Unexpected type for parent widget: %@", [parent_native_handle class]);
        @throw @"Unexpected type for parent widget";
      }
    }
  }

  id child_handle = ih->handle;
  if([child_handle isKindOfClass:[NSViewController class]])
  {
    child_handle = [child_handle view];
  }

  if([child_handle isKindOfClass:[NSView class]])
  {
    if (parent_view == nil)
    {
      return;
    }

    NSView* the_view = (NSView*)child_handle;

    [the_view setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];

    [parent_view addSubview:the_view];
  }
  else
  {
    if (ih->iclass->nativetype != IUP_TYPEMENU)
    {
      if (child_handle != nil) {
          NSCAssert(0, @"Unexpected type for child widget: %@", [child_handle class]);
          @throw @"Unexpected type for child widget";
      }
    }
  }
}

void iupcocoaRemoveFromParent(Ihandle* ih)
{
  id child_handle = ih->handle;
  if([child_handle isKindOfClass:[NSView class]])
  {
    NSView* the_view = (NSView*)child_handle;
    [the_view removeFromSuperview];
  }
  else if([child_handle isKindOfClass:[NSViewController class]])
  {
    NSViewController* view_controller = (NSViewController*)child_handle;
    NSView* the_view = [view_controller view];
    [the_view removeFromSuperview];
  }
  else
  {
    if (ih->iclass->nativetype != IUP_TYPEMENU)
    {
      NSCAssert(0, @"Unexpected type for widget");
      @throw @"Unexpected type for widget";
    }
  }
}

int iupcocoaComputeCartesianScreenHeightFromIup(int iup_height)
{
  /* This function converts from IUP's top-left based system to Cocoa's global bottom-left based system. */
  NSRect main_screen_frame = [[NSScreen mainScreen] frame];
  CGFloat main_screen_top = main_screen_frame.origin.y + main_screen_frame.size.height;
  CGFloat cartesian_y = main_screen_top - iup_height;
  return iupROUND(cartesian_y);
}

int iupcocoaComputeIupScreenHeightFromCartesian(int cartesian_height)
{
  /* This function converts from Cocoa's global bottom-left coordinate system
     to IUP's top-left based system, where (0,0) is the top-left of the main screen. */
  NSRect main_screen_frame = [[NSScreen mainScreen] frame];
  CGFloat main_screen_top = main_screen_frame.origin.y + main_screen_frame.size.height;
  CGFloat iup_y = main_screen_top - cartesian_height;
  return iupROUND(iup_y);
}

void iupdrvActivate(Ihandle* ih)
{
  id control = ih->handle;
  if ([control respondsToSelector:@selector(performClick:)])
  {
    [control performClick:nil];
  }
}

void iupdrvReparent(Ihandle* ih)
{
  NSView* child_view = iupcocoaCommonBaseLayoutGetChildView(ih);
  if (!child_view) return;

  NSView* old_parent_view = [child_view superview];
  NSView* new_parent_view = iupcocoaCommonBaseLayoutGetParentView(ih);

  if (old_parent_view != new_parent_view && new_parent_view)
  {
    /* Retain the view to prevent it from being deallocated when removed */
    [child_view retain];
    [child_view removeFromSuperview];
    [new_parent_view addSubview:child_view];
    [child_view release];
  }
}

NSView* iupcocoaCommonBaseLayoutGetParentView(Ihandle* ih)
{
  NSView* specific_container = (NSView*)iupAttribGet(ih, "_IUPTAB_CONTAINER");
  if (specific_container != NULL)
  {
    NSCAssert([specific_container isKindOfClass:[NSView class]], @"Expected specific container (e.g., _IUPTAB_CONTAINER) to be an NSView");
    return specific_container;
  }

  id parent_native_handle = iupChildTreeGetNativeParentHandle(ih);
  NSView* parent_view = nil;

  if([parent_native_handle isKindOfClass:[NSWindow class]])
  {
    NSWindow* parent_window = (NSWindow*)parent_native_handle;
    parent_view = [parent_window contentView];
  }
  else if([parent_native_handle isKindOfClass:[NSBox class]])
  {
    parent_view = [(NSBox*)parent_native_handle contentView];
  }
  else if([parent_native_handle isKindOfClass:[NSView class]])
  {
    parent_view = (NSView*)parent_native_handle;
  }
  else if([parent_native_handle isKindOfClass:[NSViewController class]])
  {
    parent_view = [(NSViewController*)parent_native_handle view];
  }
  else if([parent_native_handle isKindOfClass:[NSStatusItem class]])
  {
    return nil;
  }
  else
  {
    if (ih->iclass->nativetype != IUP_TYPEMENU && parent_native_handle != nil)
    {
      NSCAssert(0, @"Unexpected type for parent widget: %@", [parent_native_handle class]);
      @throw @"Unexpected type for parent widget";
    }
  }

  return parent_view;
}

NSView* iupcocoaCommonBaseLayoutGetChildView(Ihandle* ih)
{
  id child_handle = ih->handle;
  NSView* the_view = nil;

  if([child_handle isKindOfClass:[NSViewController class]])
  {
    child_handle = [(NSViewController*)child_handle view];
  }

  if([child_handle isKindOfClass:[NSView class]])
  {
    the_view = (NSView*)child_handle;
  }
  else
  {
    if (ih->iclass->nativetype != IUP_TYPEMENU)
    {
      NSCAssert(0, @"Unexpected type for child widget");
      @throw @"Unexpected type for child widget";
    }
  }
  return the_view;
}

void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{
  NSView* parent_view = iupcocoaCommonBaseLayoutGetParentView(ih);
  if (!parent_view) return;

  NSView* child_view = iupcocoaCommonBaseLayoutGetChildView(ih);
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

  NSRect actual_bounds = [child_view bounds];

  cocoaUpdateTip(ih);
}

void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  if (!ih->handle) return;

  Ihandle* context_menu_ih = (Ihandle*)iupcocoaCommonBaseGetContextMenuAttrib(ih);
  if(NULL != context_menu_ih)
  {
    IupDestroy(context_menu_ih);
  }
  iupcocoaCommonBaseSetContextMenuAttrib(ih, NULL);

  if (iupAttribGet(ih, "_IUPCOCOA_CURSOR_DELEGATE"))
  {
    iupdrvBaseSetCursorAttrib(ih, "NONE");
  }

  iupcocoaTipsDestroy(ih);

  objc_setAssociatedObject(ih->handle, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
  objc_setAssociatedObject(ih->handle, MAINVIEW_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
  objc_setAssociatedObject(ih->handle, ROOTVIEW_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

  id the_handle = ih->handle;
  iupcocoaRemoveFromParent(ih);

  [the_handle release];
  ih->handle = NULL;
}

static void iupCocoaDisplayUpdate(Ihandle *ih)
{
  id the_handle = ih->handle;

  if([the_handle isKindOfClass:[NSView class]])
  {
    NSView* the_view = (NSView*)the_handle;
    [the_view setNeedsDisplay:YES];
  }
  else if([the_handle isKindOfClass:[NSViewController class]])
  {
    NSView* the_view = [(NSViewController*)the_handle view];
    [the_view setNeedsDisplay:YES];
  }

  NSView* main_view = iupcocoaGetMainView(ih);
  if (main_view && main_view != (NSView*)the_handle)
  {
    [main_view setNeedsDisplay:YES];
  }
}

void iupdrvRedrawNow(Ihandle *ih)
{
  iupCocoaDisplayUpdate(ih);
  NSView* view = iupcocoaGetMainView(ih);
  if(view)
  {
    [view displayIfNeeded];
  }
}

void iupdrvPostRedraw(Ihandle *ih)
{
  iupCocoaDisplayUpdate(ih);
}

void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
  NSRect main_screen = [[NSScreen mainScreen] frame];
  CGFloat main_screen_top = main_screen.origin.y + main_screen.size.height;
  NSPoint screen_point = { *x, main_screen_top - *y }; /* IUP screen to Cocoa screen (bottom-left) */

  NSView* main_view = iupcocoaGetMainView(ih);
  if (!main_view) return;

  NSWindow* the_window = [main_view window];
  if (!the_window) return;

  NSRect screen_rect = NSMakeRect(screen_point.x, screen_point.y, 0, 0);
  NSRect window_rect = [the_window convertRectFromScreen:screen_rect];

  NSPoint window_point = window_rect.origin;
  NSPoint view_point = [main_view convertPoint:window_point fromView:nil];

  /* If the view is not flipped, its coordinate system has the origin at the bottom-left.
     We must convert the y-coordinate to IUP's top-left based system. For flipped views, the coordinate is already correct. */
  if (![main_view isFlipped])
  {
    NSRect view_bounds = [main_view bounds];
    view_point.y = view_bounds.size.height - view_point.y;
  }

  *x = iupROUND(view_point.x);
  *y = iupROUND(view_point.y);
}

void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
  NSView* main_view = iupcocoaGetMainView(ih);
  if (!main_view) return;

  NSWindow* the_window = [main_view window];
  if (!the_window) return;

  NSPoint start_point = { *x, *y };

  /* If the view is not flipped, we must convert from IUP's top-left
     coordinate system to Cocoa's bottom-left system before converting to screen coordinates. */
  if (![main_view isFlipped])
  {
    NSRect view_bounds = [main_view bounds];
    start_point.y = view_bounds.size.height - *y;
  }

  NSPoint window_point = [main_view convertPoint:start_point toView:nil];

  NSRect window_rect = NSMakeRect(window_point.x, window_point.y, 0, 0);
  NSRect screen_rect = [the_window convertRectToScreen:window_rect];

  NSRect main_screen = [[NSScreen mainScreen] frame];
  CGFloat main_screen_top = main_screen.origin.y + main_screen.size.height;

  *x = screen_rect.origin.x;
  *y = main_screen_top - screen_rect.origin.y; /* convert from cocoa screen coords to IUP screen coords */
}

int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  NSView* child_view = iupcocoaGetMainView(ih);
  if (!child_view) return 0;

  NSView* parent_view = [child_view superview];
  if (!parent_view) return 0;

  if (iupStrEqualNoCase(value, "TOP"))
  {
    [parent_view addSubview:child_view positioned:NSWindowAbove relativeTo:nil];
  }
  else
  {
    [parent_view addSubview:child_view positioned:NSWindowBelow relativeTo:nil];
  }
  return 1;
}

void iupdrvSetVisible(Ihandle* ih, int visible)
{
  id the_object = ih->handle;
  bool is_hidden = !(bool)visible;

  if([the_object isKindOfClass:[NSView class]])
  {
    [(NSView*)the_object setHidden:is_hidden];
  }
  else if([the_object isKindOfClass:[NSViewController class]])
  {
    [[(NSViewController*)the_object view] setHidden:is_hidden];
  }
}

int iupdrvIsVisible(Ihandle* ih)
{
  id the_object = ih->handle;
  if([the_object isKindOfClass:[NSWindow class]])
  {
    return [(NSWindow*)ih->handle isVisible];
  }
  else if([the_object isKindOfClass:[NSView class]])
  {
    return [(NSView*)the_object isHidden] ? NO : YES;
  }
  else if([the_object isKindOfClass:[NSViewController class]])
  {
    return [[(NSViewController*)the_object view] isHidden] ? NO : YES;
  }
  else
  {
    return 1;
  }
}


int iupdrvIsActive(Ihandle *ih)
{
  char* value = iupAttribGet(ih, "_IUPCOCOA_ACTIVE");
  int result;

  if (value == NULL)
    result = 1;
  else
    result = iupStrBoolean(value);

  return result;
}

void iupdrvSetActive(Ihandle* ih, int enable)
{
  iupAttribSet(ih, "_IUPCOCOA_ACTIVE", iupStrReturnBoolean(enable));

  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
  {
    NSWindow* window = cocoaDialogGetWindow(ih);
    if (!window)
    {
        return;
    }

    if (enable)
    {
      [window setIgnoresMouseEvents:NO];
      [NSApp activateIgnoringOtherApps:YES];
      [window makeKeyAndOrderFront:nil];
    }
    else
    {
      [window setIgnoresMouseEvents:YES];
    }
    return;
  }

  id main_view = iupcocoaGetMainView(ih);

  if ([main_view isKindOfClass:[NSImageView class]])
  {
    return;
  }

  if ([main_view respondsToSelector:@selector(setEnabled:)])
  {
    [(NSControl*)main_view setEnabled:(BOOL)enable];
  }
  else
  {
    if([main_view isKindOfClass:[NSView class]])
    {
      [(NSView*)main_view setNeedsDisplay:YES];
    }
  }
}

int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  id the_object = ih->handle;

  if([the_object respondsToSelector:@selector(setBackgroundColor:)])
  {
    unsigned char r, g, b;
    if(iupStrToRGB(value, &r, &g, &b))
    {
      CGFloat red = r/255.0;
      CGFloat green = g/255.0;
      CGFloat blue = b/255.0;

      NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
      [the_object setBackgroundColor:the_color];
    }
    else
    {
      [the_object setBackgroundColor:nil];
    }
    return 1;
  }

  return 0;
}

int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  id main_view = iupcocoaGetMainView(ih);
  if (!main_view)
    return 0;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  CGFloat red = r/255.0;
  CGFloat green = g/255.0;
  CGFloat blue = b/255.0;
  NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];

  /* For NSTextField, NSTextView, etc. */
  if ([main_view respondsToSelector:@selector(setTextColor:)])
  {
    [main_view setTextColor:the_color];
    return 1;
  }

  /* For NSButton */
  if ([main_view isKindOfClass:[NSButton class]])
  {
    NSButton* button = (NSButton*)main_view;
    NSMutableAttributedString *coloredTitle = [[button attributedTitle] mutableCopy];
    NSRange titleRange = NSMakeRange(0, [coloredTitle length]);
    [coloredTitle addAttribute:NSForegroundColorAttributeName value:the_color range:titleRange];
    [button setAttributedTitle:coloredTitle];
    [coloredTitle release];
    return 1;
  }

  /* For NSBox (used by IupFrame) */
  if ([main_view respondsToSelector:@selector(setTitleColor:)])
  {
    [main_view setTitleColor:the_color];
    return 1;
  }

  return 0; /* Control does not support changing foreground color */
}

@interface IUPCursorTrackingDelegate : NSObject
{
  NSCursor* _cursor;
}
@property(nonatomic, retain) NSCursor* cursor;
- (void)mouseEntered:(NSEvent *)event;
- (void)mouseExited:(NSEvent *)event;
- (void)dealloc;
@end

@implementation IUPCursorTrackingDelegate
@synthesize cursor = _cursor;
- (void)mouseEntered:(NSEvent *)event { [self.cursor set]; }
- (void)mouseExited:(NSEvent *)event { [[NSCursor arrowCursor] set]; }
- (void)dealloc { self.cursor = nil; [super dealloc]; }
@end


static NSCursor* iupCocoaGetCursor(Ihandle* ih, const char* name)
{
  static struct {
    const char* iupname;
    const char* sel_name;
  } table[] = {
    {"ARROW",     "arrowCursor"},
    {"BUSY",      "busyButClickableCursor"}, /* Not part of the API for NSCursor but apps are using it (i.e. Blender) */
    {"CROSS",     "crosshairCursor"},
    {"HAND",      "pointingHandCursor"},
    {"MOVE",      "closedHandCursor"},
    {"RESIZE_N",  "_windowResizeNorthCursor"},
    {"RESIZE_S",  "_windowResizeSouthCursor"},
    {"RESIZE_NS", "resizeUpDownCursor"},
    {"RESIZE_W",  "_windowResizeWestCursor"},
    {"RESIZE_E",  "_windowResizeEastCursor"},
    {"RESIZE_WE", "resizeLeftRightCursor"},
    {"RESIZE_NE", "_windowResizeNorthEastCursor"},
    {"RESIZE_SE", "_windowResizeSouthEastCursor"},
    {"RESIZE_NW", "_windowResizeNorthWestCursor"},
    {"RESIZE_SW", "_windowResizeSouthWestCursor"},
    {"TEXT",      "IBeamCursor"},
    {"HELP",      "contextualMenuCursor"},
    {"IUP",       "contextualMenuCursor"},
    {"NO",        "operationNotAllowedCursor"},
    {"UPARROW",   "arrowCursor"} /* No direct up-arrow cursor */
  };

  if (iupStrEqualNoCase(name, "NONE") || iupStrEqualNoCase(name, "NULL"))
    return nil;

  for (int i = 0; i < sizeof(table)/sizeof(table[0]); i++)
  {
    if (iupStrEqualNoCase(name, table[i].iupname))
    {
      SEL syssel = NSSelectorFromString([NSString stringWithUTF8String:table[i].sel_name]);
      if ([NSCursor respondsToSelector:syssel])
      {
        return [NSCursor performSelector:syssel];
      }
    }
  }

  return iupImageGetCursor(name);
}

int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  NSView* main_view = iupcocoaGetMainView(ih);
  if (!main_view) return 0;

  IUPCursorTrackingDelegate* old_delegate = (IUPCursorTrackingDelegate*)iupAttribGet(ih, "_IUPCOCOA_CURSOR_DELEGATE");
  if (old_delegate)
  {
    NSTrackingArea* old_area = (NSTrackingArea*)iupAttribGet(ih, "_IUPCOCOA_TRACKINGAREA");
    if (old_area)
      [main_view removeTrackingArea:old_area];

    [old_delegate release];
    iupAttribSet(ih, "_IUPCOCOA_CURSOR_DELEGATE", NULL);
    iupAttribSet(ih, "_IUPCOCOA_TRACKINGAREA", NULL);
  }

  NSCursor* cursor = iupCocoaGetCursor(ih, value);
  if (cursor)
  {
    IUPCursorTrackingDelegate* delegate = [[IUPCursorTrackingDelegate alloc] init];
    delegate.cursor = cursor;

    NSTrackingArea* area = [[NSTrackingArea alloc] initWithRect:[main_view bounds]
                                                        options:(NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect)
                                                          owner:delegate
                                                       userInfo:nil];
    [main_view addTrackingArea:area];

    /* Store delegate and area to be cleaned up later. The view retains the area. */
    iupAttribSet(ih, "_IUPCOCOA_CURSOR_DELEGATE", (char*)delegate);
    iupAttribSet(ih, "_IUPCOCOA_TRACKINGAREA", (char*)area);
    [area release];
  }

  return 1;
}

int iupdrvGetScrollbarSize(void)
{
  return [NSScroller scrollerWidthForControlSize:NSControlSizeRegular scrollerStyle:NSScrollerStyleLegacy];
}

IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{
  id the_object = iupcocoaGetMainView(ih);
  if([the_object respondsToSelector:@selector(setAccessibilityLabel:)])
  {
    if(!title)
    {
      [the_object setAccessibilityLabel:nil];
    }
    else
    {
      NSString* ns_title = [NSString stringWithUTF8String:title];
      [the_object setAccessibilityLabel:ns_title];
    }
  }
}

void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
}

void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
}

void iupdrvSendKey(int key, int press)
{
  unsigned int maccode, state;
  iupdrvKeyEncode(key, &maccode, &state);
  if (!maccode) return;

  CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

  if (press & 0x01) /* Press */
  {
    CGEventRef event = CGEventCreateKeyboardEvent(source, (CGKeyCode)maccode, true);
    if (state != 0) CGEventSetFlags(event, (CGEventFlags)state);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
  }

  if (press & 0x02) /* Release */
  {
    CGEventRef event = CGEventCreateKeyboardEvent(source, (CGKeyCode)maccode, false);
    if (state != 0) CGEventSetFlags(event, (CGEventFlags)state);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
  }

  CFRelease(source);
}

void iupdrvSendMouse(int x, int y, int bt, int status)
{
  iupdrvWarpPointer(x, y);

  if (status != -1)
  {
    CGPoint point = CGPointMake(x, y);
    CGEventType press_type, release_type;
    CGMouseButton button;

    switch(bt)
    {
      case IUP_BUTTON1: press_type=kCGEventLeftMouseDown; release_type=kCGEventLeftMouseUp; button=kCGMouseButtonLeft; break;
      case IUP_BUTTON2: press_type=kCGEventOtherMouseDown; release_type=kCGEventOtherMouseUp; button=kCGMouseButtonCenter; break;
      case IUP_BUTTON3: press_type=kCGEventRightMouseDown; release_type=kCGEventRightMouseUp; button=kCGMouseButtonRight; break;
      default: return;
    }

    /* IUP Spec: status: 1 (press), 0 (release), -1 (move), 2 (double-click). */

    if (status == 1) /* Press */
    {
      CGEventRef event = CGEventCreateMouseEvent(NULL, press_type, point, button);
      CGEventSetIntegerValueField(event, kCGMouseEventClickState, 1);
      CGEventPost(kCGHIDEventTap, event);
      CFRelease(event);
    }
    else if (status == 0) /* Release */
    {
      CGEventRef event = CGEventCreateMouseEvent(NULL, release_type, point, button);
      CGEventSetIntegerValueField(event, kCGMouseEventClickState, 1);
      CGEventPost(kCGHIDEventTap, event);
      CFRelease(event);
    }
    else if (status == 2) /* Double-click (Generate full sequence) */
    {
      /* Click 1 (Count 1) */
      CGEventRef event_down1 = CGEventCreateMouseEvent(NULL, press_type, point, button);
      CGEventSetIntegerValueField(event_down1, kCGMouseEventClickState, 1);
      CGEventPost(kCGHIDEventTap, event_down1);

      CGEventRef event_up1 = CGEventCreateMouseEvent(NULL, release_type, point, button);
      CGEventSetIntegerValueField(event_up1, kCGMouseEventClickState, 1);
      CGEventPost(kCGHIDEventTap, event_up1);

      /* Click 2 (Count 2) */
      CGEventRef event_down2 = CGEventCreateMouseEvent(NULL, press_type, point, button);
      CGEventSetIntegerValueField(event_down2, kCGMouseEventClickState, 2);
      CGEventPost(kCGHIDEventTap, event_down2);

      CGEventRef event_up2 = CGEventCreateMouseEvent(NULL, release_type, point, button);
      CGEventSetIntegerValueField(event_up2, kCGMouseEventClickState, 2);
      CGEventPost(kCGHIDEventTap, event_up2);

      CFRelease(event_down1);
      CFRelease(event_up1);
      CFRelease(event_down2);
      CFRelease(event_up2);
    }
  }
}

void iupdrvSleep(int time)
{
  usleep(time * 1000);
}

void iupdrvWarpPointer(int x, int y)
{
  CGPoint point = CGPointMake(x, y);
  CGWarpMouseCursorPosition(point);
  CGAssociateMouseAndMouseCursorPosition(true);
}

void iupcocoaCommonBaseAppendMenuItems(NSMenu* dst_menu, NSMenu* src_menu)
{
  if((src_menu != nil) && ([src_menu numberOfItems] > 0))
  {
    [dst_menu addItem:[NSMenuItem separatorItem]];

    NSArray<NSMenuItem*>* item_array = [src_menu itemArray];
    for(NSMenuItem* a_default_item in item_array)
    {
      NSMenuItem* item_copy = [a_default_item copy];
      [dst_menu addItem:item_copy];
      [item_copy release];
    }
  }
}

void iupcocoaCommonBaseAppendDefaultMenuItemsForClassType(NSMenu* dst_menu, Class class_of_widget)
{
  if([class_of_widget respondsToSelector:@selector(defaultMenu)])
  {
    NSMenu* default_menu = [class_of_widget defaultMenu];
    iupcocoaCommonBaseAppendMenuItems(dst_menu, default_menu);
  }
}

void iupcocoaCommonBaseSetContextMenuForWidget(Ihandle* ih, id widget_to_attach_menu_to, Ihandle* menu_ih)
{
  /* Mark that the user has configured this attribute. This allows delegate methods
     to distinguish between "never set" (use default behavior) and "set to nil" (disable menu). */
  iupAttribSet(ih, "_IUPCOCOA_CONTEXTMENU_SET", "1");
  iupAttribSet(ih, "_COCOA_CONTEXT_MENU_IH", (const char*)menu_ih);

  if (NULL == menu_ih)
  {
    if ([widget_to_attach_menu_to respondsToSelector:@selector(setMenu:)])
    {
      [widget_to_attach_menu_to setMenu:nil];
    }
    return;
  }

  if (NULL == menu_ih->handle)
  {
    IupMap(menu_ih);
  }

  if (menu_ih->iclass->nativetype != IUP_TYPEMENU || ![(id)menu_ih->handle isKindOfClass:[NSMenu class]])
  {
    return;
  }

  NSMenu* the_menu = (NSMenu*)menu_ih->handle;
  if ([widget_to_attach_menu_to respondsToSelector:@selector(setMenu:)])
  {
    iupcocoaCommonBaseAppendDefaultMenuItemsForClassType(the_menu, [widget_to_attach_menu_to class]);
    [widget_to_attach_menu_to setMenu:the_menu];
  }
}

int iupcocoaCommonBaseIupButtonForCocoaButton(NSInteger which_cocoa_button)
{
  if(0 == which_cocoa_button) return IUP_BUTTON1;
  if(1 == which_cocoa_button) return IUP_BUTTON3;
  if(2 == which_cocoa_button) return IUP_BUTTON2;
  if(3 == which_cocoa_button) return IUP_BUTTON4;
  if(4 == which_cocoa_button) return IUP_BUTTON5;
  return (int)(which_cocoa_button + '0'); /* Other buttons */
}

bool iupcocoaCommonBaseHandleMouseButtonCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view, bool is_pressed)
{
  IFniiiis callback_function;
  bool caller_should_propagate = true;

  callback_function = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if(callback_function)
  {
    NSPoint the_point = [the_event locationInWindow];
    NSPoint converted_point = [represented_view convertPoint:the_point fromView:nil];
    CGFloat final_y = converted_point.y;

    /* Convert from Cocoa's coordinate system (origin bottom-left) to IUP's (origin top-left),
       but only if the view is not already flipped (which would mean it is already top-left). */
    if(![represented_view isFlipped])
    {
      NSRect view_bounds = [represented_view bounds];
      final_y = view_bounds.size.height - converted_point.y;
    }

    NSInteger which_cocoa_button = [the_event buttonNumber];
    char mod_status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupcocoaButtonKeySetStatus(the_event, mod_status);

    if([the_event modifierFlags] & NSEventModifierFlagControl && 0 == which_cocoa_button)
    {
      which_cocoa_button = 1; /* Ctrl-Left-click becomes a right-click */
    }

    int which_iup_button = iupcocoaCommonBaseIupButtonForCocoaButton(which_cocoa_button);

    int callback_result = callback_function(ih, which_iup_button, is_pressed, iupROUND(converted_point.x), iupROUND(final_y), mod_status);
    if(IUP_CLOSE == callback_result)
    {
      IupExitLoop();
      caller_should_propagate = false;
    }
    else if(IUP_IGNORE == callback_result)
    {
      caller_should_propagate = false;
    }
    else if(IUP_CONTINUE == callback_result)
    {
      caller_should_propagate = true;
    }
    else
    {
      caller_should_propagate = false;
    }
  }
  else
  {
    caller_should_propagate = true;
  }
  return !caller_should_propagate;
}

bool iupcocoaCommonBaseHandleMouseMotionCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view)
{
  bool caller_should_propagate = true;
  IFniis callback_function;
  callback_function = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if(callback_function)
  {
    NSPoint the_point = [the_event locationInWindow];
    NSPoint converted_point = [represented_view convertPoint:the_point fromView:nil];
    CGFloat final_y = converted_point.y;

    /* Convert from Cocoa's coordinate system (origin bottom-left) to IUP's (origin top-left),
       but only if the view is not already flipped (which would mean it is already top-left). */
    if(![represented_view isFlipped])
    {
      NSRect view_bounds = [represented_view bounds];
      final_y = view_bounds.size.height - converted_point.y;
    }

    char mod_status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupcocoaButtonKeySetStatus(the_event, mod_status);

    callback_function(ih,  iupROUND(converted_point.x), iupROUND(final_y), mod_status);
  }
  return !caller_should_propagate;
}

bool iupcocoaCommonBaseScrollWheelCallback(Ihandle* ih, NSEvent* the_event, NSView* represented_view)
{
  IFnfiis callback_function;
  bool caller_should_propagate = true;

  callback_function = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
  if(callback_function)
  {
    NSPoint the_point = [the_event locationInWindow];
    NSPoint converted_point = [represented_view convertPoint:the_point fromView:nil];
    CGFloat final_y = converted_point.y;

    /* IUP's WHEEL_CB 'delta' corresponds to vertical scrolling.
       On macOS, deltaY > 0 means scroll up (content should move up, showing content above).
       This matches the IUP convention where positive delta means scroll up. */
    CGFloat delta = [the_event deltaY];

    /* IUP does not have a separate parameter for horizontal scrolling (deltaX). */

    /* Convert from Cocoa's coordinate system (origin bottom-left) to IUP's (origin top-left),
       but only if the view is not already flipped (which would mean it is already top-left). */
    if(![represented_view isFlipped])
    {
      NSRect view_bounds = [represented_view bounds];
      final_y = view_bounds.size.height - converted_point.y;
    }

    char mod_status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupcocoaButtonKeySetStatus(the_event, mod_status);

    int callback_result = callback_function(ih, delta, iupROUND(converted_point.x), iupROUND(final_y), mod_status);
    if(IUP_CLOSE == callback_result)
    {
      IupExitLoop();
      caller_should_propagate = false;
    }
    else if(IUP_IGNORE == callback_result)
    {
      caller_should_propagate = false;
    }
  }
  return !caller_should_propagate;
}

int iupcocoaCommonBaseSetLayerBackedAttrib(Ihandle* ih, const char* value)
{
  NSView* main_view = iupcocoaGetMainView(ih);
  if(nil != main_view)
  {
    BOOL should_enable = (BOOL)iupStrBoolean(value);
    [main_view setWantsLayer:should_enable];
    NSView* root_view = iupcocoaGetRootView(ih);
    if(root_view != main_view)
    {
      [root_view setWantsLayer:should_enable];
    }
  }
  return 0;
}

char* iupCocoaCommonBaseGetLayerBackedAttrib(Ihandle* ih)
{
  NSView* main_view = iupcocoaGetRootView(ih);
  if(nil != main_view)
  {
    BOOL is_enabled = [main_view wantsLayer];
    return iupStrReturnBoolean(is_enabled);
  }
  return iupStrReturnBoolean(false);
}

int iupcocoaCommonBaseSetContextMenuAttrib(Ihandle* ih, const char* value)
{
  Ihandle* menu_ih = (Ihandle*)value;
  id widget_to_attach_menu_to = iupcocoaGetMainView(ih);

  iupcocoaCommonBaseSetContextMenuForWidget(ih, widget_to_attach_menu_to, menu_ih);

  return 1;
}

char* iupcocoaCommonBaseGetContextMenuAttrib(Ihandle* ih)
{
  return (char*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");
}

int iupcocoaCommonBaseSetSendActionAttrib(Ihandle* ih, const char* value)
{
  if(NULL == value)
  {
    return 0;
  }

  NSView* target_view = nil;
  id sender_object = nil;
  if(NULL == ih)
  {
    target_view = nil;
    sender_object = nil;
  }
  else
  {
    target_view = iupcocoaGetMainView(ih);
    sender_object = target_view;
  }

  SEL the_selector = sel_getUid(value);

  [[NSApplication sharedApplication] sendAction:the_selector to:target_view from:sender_object];

  return 0;
}

NSWindow* cocoaDialogGetWindow(Ihandle* ih)
{
  if (!ih || !ih->handle) return nil;
  id root_object = (id)ih->handle;
  if ([root_object isKindOfClass:[NSWindow class]])
  {
    return (NSWindow*)root_object;
  }
  return nil;
}

int iupcocoaIsSystemDarkMode(void)
{
  NSAppearance *appearance = [NSApp effectiveAppearance];
  NSString *appearanceName = [appearance bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua, NSAppearanceNameDarkAqua]];
  return [appearanceName isEqualToString:NSAppearanceNameDarkAqua] ? 1 : 0;
}
