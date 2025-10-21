/** \file
 * \brief IupDialog class
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
#include "iup_class.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_dlglist.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"

#include "iupcocoa_drv.h"
#include "iupcocoa_keycodes.h"


static const void* TRAYMENUIHANDLE_ASSOCIATED_OBJ_KEY = @"TRAYMENUIHANDLE_ASSOCIATED_OBJ_KEY";
static const void* DOCKPROGRESS_ASSOCIATED_OBJ_KEY = @"DOCKPROGRESS_ASSOCIATED_OBJ_KEY";
static const void* TRAYCLICK_DELEGATE_ASSOCIATED_OBJ_KEY = @"TRAYCLICK_DELEGATE_ASSOCIATED_OBJ_KEY";
static const char IUPCocoaZoomRestoreFrameKey = 0;

static void* IupCocoaAppearanceContext = &IupCocoaAppearanceContext;

@interface ModalInfo : NSObject
{
  Ihandle* _ih;
  NSModalSession _modalSession;
}
@property(assign, nonatomic) Ihandle* ih;
@property(assign, nonatomic) NSModalSession modalSession;
@end

@implementation ModalInfo
@synthesize ih;
@synthesize modalSession;
@end

@interface IupCocoaWindowDelegate : NSObject <NSWindowDelegate>
@end

@interface IupTrayClickDelegate : NSObject
- (void) trayAction:(id)sender;
@end

@interface IupCocoaWindow : NSWindow
@end

@implementation IupCocoaWindow

- (BOOL)performKeyEquivalent:(NSEvent *)event
{
  if ([event type] == NSEventTypeKeyDown)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY); // this is the dialog handle
    if (iupObjectCheck(ih))
    {
      NSResponder* first_responder = [self firstResponder];
      Ihandle* focused_ih = nil;

      if (first_responder && [first_responder isKindOfClass:[NSView class]])
      {
        focused_ih = (Ihandle*)objc_getAssociatedObject(first_responder, IHANDLE_ASSOCIATED_OBJ_KEY);
      }

      if (!iupObjectCheck(focused_ih))
      {
        focused_ih = ih;
      }

      int mac_key_code = [event keyCode];
      if (iupCocoaKeyEvent(focused_ih, event, mac_key_code, true))
      {
        return YES;
      }

      if (mac_key_code == kVK_Return)
      {
        if (focused_ih && IupClassMatch(focused_ih, "button"))
        {
          return [super performKeyEquivalent:event];
        }
        Ihandle* button_ih = IupGetAttributeHandle(ih, "DEFAULTENTER");
        if (iupObjectCheck(button_ih))
        {
          iupdrvActivate(button_ih);
          return YES;
        }
      }
      else if (mac_key_code == kVK_Escape)
      {
        Ihandle* button_ih = IupGetAttributeHandle(ih, "DEFAULTESC");
        if (iupObjectCheck(button_ih))
        {
          iupdrvActivate(button_ih);
          return YES;
        }
      }
    }
  }

  return [super performKeyEquivalent:event];
}

- (void)keyDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    int mac_key_code = [event keyCode];
    BOOL handled = iupCocoaKeyEvent(ih, event, mac_key_code, true);
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

@end

/****************************************************************
 ****************** Utilities & Helpers *************************
 ****************************************************************/

static NSStatusItem* cocoaDialogGetStatusItem(Ihandle* ih)
{
  if (!ih || !ih->handle) return nil;

  NSStatusItem* status_item = (NSStatusItem*)iupAttribGet(ih, "_IUPCOCOA_STATUSITEM");
  return status_item;
}

static NSWindowStyleMask cocoaDialogGetStyleMask(Ihandle* ih)
{
  NSWindowStyleMask style_mask;
  int has_titlebar = 0;

  if (iupAttribGet(ih, "TITLE"))
    has_titlebar = 1;

  if (iupAttribGetBoolean(ih, "MAXBOX") ||
      iupAttribGetBoolean(ih, "MINBOX") ||
      iupAttribGetBoolean(ih, "MENUBOX"))
    has_titlebar = 1;

  if (iupAttribGetBoolean(ih, "BORDER"))
    has_titlebar = 1;

  if (has_titlebar)
    style_mask = NSWindowStyleMaskTitled;
  else
    style_mask = NSWindowStyleMaskBorderless;

  if (iupAttribGetBoolean(ih, "RESIZE"))
    style_mask |= NSWindowStyleMaskResizable;

  if (iupAttribGetBoolean(ih, "MINBOX"))
    style_mask |= NSWindowStyleMaskMiniaturizable;

  if (iupAttribGetBoolean(ih, "MENUBOX"))
    style_mask |= NSWindowStyleMaskClosable;

  if (iupAttribGetBoolean(ih, "TOOLBOX"))
    style_mask |= NSWindowStyleMaskUtilityWindow;

  return style_mask;
}

static void cocoaDialogUpdateStyleMask(Ihandle* ih)
{
  NSWindow* window = cocoaDialogGetWindow(ih);
  if (window)
  {
    NSWindowStyleMask new_mask = cocoaDialogGetStyleMask(ih);
    [window setStyleMask:new_mask];
  }
}

/****************************************************************
 ********************** Modal Loop ******************************
 ****************************************************************/

@interface IupNonRunLoopModalAppDelegate : NSObject <NSApplicationDelegate>
{
  Ihandle* _ih;
}
- (instancetype) initWithIhandle:(Ihandle*)ih;
- (Ihandle*) ih;
@end

@implementation IupNonRunLoopModalAppDelegate

- (instancetype) initWithIhandle:(Ihandle*)ih
{
  self = [super init];
  if(nil != self)
  {
    _ih = ih;
  }
  return self;
}

- (Ihandle*) ih
{
  return _ih;
}

- (void) applicationWillFinishLaunching:(NSNotification*)a_notification
{
  if ([NSApp activationPolicy] == NSApplicationActivationPolicyProhibited)
  {
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  }

  Ihandle* ih = _ih;
  if (ih->data->menu)
  {
    if (!ih->data->menu->handle)
      IupMap(ih->data->menu);

    if (ih->data->menu->handle)
      iupCocoaMenuSetApplicationMenu(ih->data->menu);
  }
  else
  {
    Ihandle* global_menu = (Ihandle*)IupGetGlobal("MENU");
    if (global_menu)
    {
      if (!global_menu->handle)
        IupMap(global_menu);

      if (global_menu->handle)
        iupCocoaMenuSetApplicationMenu(global_menu);
    }
    else
    {
      iupCocoaEnsureDefaultApplicationMenu();
    }
  }
}

- (void) applicationDidFinishLaunching:(NSNotification*)a_notification
{
  Ihandle* ih = _ih;
  NSWindow* the_window = cocoaDialogGetWindow(ih);

  [NSApp activateIgnoringOtherApps:YES];
  [the_window makeKeyAndOrderFront:nil];
  [the_window makeMainWindow];

  [NSApp runModalForWindow:the_window];

  [NSApp performSelectorOnMainThread:@selector(stop:) withObject:NSApp waitUntilDone:NO];
}

- (void) applicationWillTerminate:(NSNotification*)a_notification
{
  IFentry exit_callback = (IFentry)IupGetFunction("EXIT_CB");
  if(NULL != exit_callback)
  {
    exit_callback();
  }
}
@end

bool cocoaDialogExitModal(Ihandle* modal_ih)
{
  if(!iupObjectCheck(modal_ih))
  {
    return false;
  }

  Ihandle* parent_ih = IupGetAttributeHandle(modal_ih, "PARENTDIALOG");
  NSWindow* parent_window = nil;
  if(iupObjectCheck(parent_ih))
  {
    parent_window = cocoaDialogGetWindow(parent_ih);
  }

  if(!parent_window)
  {
    NSWindow* the_window = cocoaDialogGetWindow(modal_ih);
    if(the_window)
    {
      parent_window = [the_window parentWindow];
      if(parent_window)
      {
        parent_ih = (Ihandle*)objc_getAssociatedObject(parent_window, IHANDLE_ASSOCIATED_OBJ_KEY);
      }
    }
  }

  if (parent_window && iupObjectCheck(parent_ih))
  {
    [NSApp activateIgnoringOtherApps:YES];
    [parent_window makeKeyAndOrderFront:nil];
    iupCocoaFocusIn(parent_ih);
    return true;
  }

  return false;
}

static void cocoaDialogChildDestroyNotification(NSNotification* notification)
{
  NSWindow* child_window = [notification object];
  Ihandle* child_ih = (Ihandle*)objc_getAssociatedObject(child_window, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (iupObjectCheck(child_ih))
  {
    NSWindow* parent_window = [child_window parentWindow];
    if (parent_window)
    {
      Ihandle* parent_ih = (Ihandle*)objc_getAssociatedObject(parent_window, IHANDLE_ASSOCIATED_OBJ_KEY);
      if (iupObjectCheck(parent_ih))
      {
        if (iupAttribGetBoolean(child_ih, "MODAL"))
        {
          cocoaDialogExitModal(child_ih);
        }
      }
    }
  }
}

/****************************************************************
 ********************** Delegates *******************************
 ****************************************************************/

@implementation IupCocoaWindowDelegate

- (BOOL) windowShouldClose:(id)the_sender
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (!iupObjectCheck(ih))
  {
    return YES;
  }

  Icallback callback_function = IupGetCallback(ih, "CLOSE_CB");
  if(callback_function)
  {
    int ret = callback_function(ih);
    if (ret == IUP_IGNORE)
    {
      return NO;
    }
  }

  IupHide(ih);

  return NO;
}

- (void) windowWillClose:(NSNotification*)the_notification
{
  NSWindow* the_window = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_window, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (!iupObjectCheck(ih)) return;

  if(iupAttribGetBoolean(ih, "MODAL"))
  {
    cocoaDialogExitModal(ih);
  }

  NSArray<NSWindow*>* child_windows = [[the_window childWindows] copy];

  for (NSWindow* child_window in child_windows)
  {
    Ihandle* child_ih = (Ihandle*)objc_getAssociatedObject(child_window, IHANDLE_ASSOCIATED_OBJ_KEY);

    if (iupObjectCheck(child_ih))
    {
      if(iupAttribGetBoolean(child_ih, "MODAL"))
      {
        cocoaDialogExitModal(child_ih);
      }
      IupDestroy(child_ih);
    }
  }
  [child_windows release];
}

- (void) windowDidBecomeKey:(NSNotification*)notification
{
  NSWindow* the_window = [notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_window, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (!iupObjectCheck(ih)) return;

  iupCocoaFocusIn(ih);

  if (ih->data->menu)
  {
    iupCocoaMenuSetApplicationMenu(ih->data->menu);
  }
  else
  {
    Ihandle* global_menu = (Ihandle*)IupGetGlobal("MENU");
    iupCocoaMenuSetApplicationMenu(global_menu);
  }
}

- (void) windowDidResignKey:(NSNotification*)notification
{
  NSWindow* the_window = [notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_window, IHANDLE_ASSOCIATED_OBJ_KEY);
}

- (NSSize) windowWillResize:(NSWindow*)the_sender toSize:(NSSize)frame_size
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupObjectCheck(ih)) return frame_size;

  NSSize content_size = [the_sender contentRectForFrameRect:NSMakeRect(0, 0, frame_size.width, frame_size.height)].size;

  IFnii cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if(!cb || cb(ih, content_size.width, content_size.height)!=IUP_IGNORE)
  {
    return frame_size;
  }
  else
  {
    return [the_sender frame].size;
  }
}

- (NSRect)windowWillUseStandardFrame:(NSWindow *)window defaultFrame:(NSRect)newFrame
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(window, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (ih && ih->data->ignore_resize)
    return [window frame];

  NSValue* previousFrameValue = objc_getAssociatedObject(window, &IUPCocoaZoomRestoreFrameKey);

  if (previousFrameValue)
  {
    NSRect restoreFrame = [previousFrameValue rectValue];
    objc_setAssociatedObject(window, &IUPCocoaZoomRestoreFrameKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return restoreFrame;
  }
  else
  {
    NSRect currentFrame = [window frame];
    NSValue* frameValue = [NSValue valueWithRect:currentFrame];
    objc_setAssociatedObject(window, &IUPCocoaZoomRestoreFrameKey, frameValue, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return newFrame;
  }
}

- (void) windowDidResize:(NSNotification*)the_notification
{
  NSWindow* the_window = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_window, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupObjectCheck(ih)) return;

  if (ih->data->ignore_resize) return;

  NSRect frame_rect = [the_window frame];
  NSRect content_rect = [[the_window contentView] bounds];

  int new_width = iupROUND(frame_rect.size.width);
  int new_height = iupROUND(frame_rect.size.height);

  if (ih->currentwidth == new_width && ih->currentheight == new_height)
    return;

  ih->currentwidth = new_width;
  ih->currentheight = new_height;

  int new_state = IUP_RESTORE;
  if ([the_window isZoomed])
    new_state = IUP_MAXIMIZE;

  if (ih->data->show_state != new_state)
  {
    IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
    ih->data->show_state = new_state;
    if (show_cb && show_cb(ih, new_state) == IUP_CLOSE)
      IupExitLoop();
  }

  IFnii resize_cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (!resize_cb || resize_cb(ih, (int)content_rect.size.width, (int)content_rect.size.height) != IUP_IGNORE)
  {
    ih->data->ignore_resize = 1;
    IupRefresh(ih);
    ih->data->ignore_resize = 0;
  }
}

- (void) windowDidMove:(NSNotification*)the_notification
{
  NSWindow* the_window = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_window, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupObjectCheck(ih)) return;

  IFnii cb = (IFnii)IupGetCallback(ih, "MOVE_CB");
  if (cb)
  {
    int x, y;
    iupdrvDialogGetPosition(ih, NULL, &x, &y);
    cb(ih, x, y);
  }
}

- (void) windowDidMiniaturize:(NSNotification*)the_notification
{
  NSWindow* the_window = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_window, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupObjectCheck(ih)) return;

  if (ih->data->show_state != IUP_MINIMIZE)
  {
    IFni cb = (IFni)IupGetCallback(ih, "SHOW_CB");
    ih->data->show_state = IUP_MINIMIZE;
    if (cb && cb(ih, IUP_MINIMIZE) == IUP_CLOSE)
      IupExitLoop();
  }
}

- (void) windowDidDeminiaturize:(NSNotification*)the_notification
{
  NSWindow* the_window = [the_notification object];
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_window, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupObjectCheck(ih)) return;

  int new_state = [the_window isZoomed] ? IUP_MAXIMIZE : IUP_RESTORE;
  if (ih->data->show_state != new_state)
  {
    IFni cb = (IFni)IupGetCallback(ih, "SHOW_CB");
    ih->data->show_state = new_state;
    if (cb && cb(ih, new_state) == IUP_CLOSE)
      IupExitLoop();
  }
}

- (void) windowWillEnterFullScreen:(NSNotification*)the_notification
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject([the_notification object], IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupObjectCheck(ih)) return;

  NSWindow* window = [the_notification object];
  if (!objc_getAssociatedObject(window, &IUPCocoaZoomRestoreFrameKey))
  {
    NSRect currentFrame = [window frame];
    NSValue* frameValue = [NSValue valueWithRect:currentFrame];
    objc_setAssociatedObject(window, &IUPCocoaZoomRestoreFrameKey, frameValue, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }

  iupAttribSet(ih, "_IUPCOCOA_FULLSCREEN", "YES");
}

- (void) windowDidEnterFullScreen:(NSNotification*)the_notification
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject([the_notification object], IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupObjectCheck(ih)) return;

  ih->data->ignore_resize = 1;
  IupRefresh(ih);
  ih->data->ignore_resize = 0;
}

- (void) windowDidExitFullScreen:(NSNotification*)the_notification
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject([the_notification object], IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupObjectCheck(ih)) return;

  iupAttribSet(ih, "_IUPCOCOA_FULLSCREEN", "NO");
  ih->data->ignore_resize = 1;
  IupRefresh(ih);
  ih->data->ignore_resize = 0;
}

- (void)cocoaDialogChildDestroyNotification:(NSNotification*)notification
{
  cocoaDialogChildDestroyNotification(notification);
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
  if (context == IupCocoaAppearanceContext)
  {
    if (@available(macOS 10.14, *))
    {
      NSWindow* the_window = (NSWindow*)object;
      Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_window, IHANDLE_ASSOCIATED_OBJ_KEY);

      if (!iupObjectCheck(ih))
        return;

      int dark_mode = iupCocoaIsSystemDarkMode();

      IFni cb = (IFni)IupGetCallback(ih, "THEMECHANGED_CB");
      if (cb)
        cb(ih, dark_mode);
    }
  }
  else
  {
    [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
  }
}

@end

@implementation IupTrayClickDelegate
- (void) trayAction:(id)sender
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(sender, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupObjectCheck(ih)) return;

  IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
  if (cb)
  {
    NSEvent* event = [NSApp currentEvent];
    NSEventType event_type = [event type];
    int pressed = 0;
    int dclick = 0;
    int button = 0;

    switch(event_type)
    {
      case NSEventTypeLeftMouseDown:
        pressed = 1;
        button = 1;
        break;
      case NSEventTypeLeftMouseUp:
        button = 1;
        break;
      case NSEventTypeRightMouseDown:
        button = 3;
        pressed = 1;
        break;
      case NSEventTypeRightMouseUp:
        button = 3;
        break;
      case NSEventTypeOtherMouseDown:
        button = 2;
        pressed = 1;
        break;
      case NSEventTypeOtherMouseUp:
        button = 2;
        break;
      default:
        button = 1;
        pressed = 1;
        break;
    }

    if ([event clickCount] >= 2)
      dclick = 1;

    if (([event modifierFlags] & NSEventModifierFlagControl) && button == 1)
      button = 3;

    if (cb(ih, button, pressed, dclick) == IUP_CLOSE)
    {
      IupExitLoop();
    }
  }
}
@end


/****************************************************************
 ******************* Driver Functions ***************************
 ****************************************************************/

int iupdrvDialogIsVisible(Ihandle* ih)
{
  if (!ih->data->first_show)
  {
    return 0;
  }

  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (the_window)
    return (int)[the_window isVisible];

  return 0;
}

void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int *w, int *h)
{
  NSWindow* the_window = handle ? (NSWindow*)handle : cocoaDialogGetWindow(ih);
  if (!the_window) return;

  NSRect frame_rect = [the_window frame];

  if (w) *w = iupROUND(frame_rect.size.width);
  if (h) *h = iupROUND(frame_rect.size.height);
}

void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window)
  {
    return;
  }

  if (visible)
  {
    if (iupAttribGetBoolean(ih, "SHOWNOACTIVATE"))
    {
      [the_window orderFront:nil];
    }
    else
    {
      [the_window makeKeyAndOrderFront:nil];
    }
  }
  else
  {
    Ihandle* parent_ih = IupGetAttributeHandle(ih, "PARENTDIALOG");
    NSWindow* parent_window = nil;

    if (iupObjectCheck(parent_ih))
    {
      parent_window = cocoaDialogGetWindow(parent_ih);
    }

    if (!parent_window)
    {
      parent_window = [the_window parentWindow];
      if (parent_window)
      {
        parent_ih = (Ihandle*)objc_getAssociatedObject(parent_window, IHANDLE_ASSOCIATED_OBJ_KEY);
      }
    }

    if (parent_window && iupObjectCheck(parent_ih))
    {
      [NSApp activateIgnoringOtherApps:YES];
      [parent_window makeKeyAndOrderFront:nil];
    }

    [the_window orderOut:nil];

    if (parent_window && iupObjectCheck(parent_ih))
    {
      IupSetFocus(parent_ih);
    }
  }
}

void iupdrvDialogGetPosition(Ihandle *ih, InativeHandle* handle, int *x, int *y)
{
  NSWindow* the_window = handle ? (NSWindow*)handle : cocoaDialogGetWindow(ih);
  if (!the_window) return;

  NSRect the_rect = [the_window frame];

  if (x) *x = the_rect.origin.x;
  if (y) *y = iupCocoaComputeIupScreenHeightFromCartesian(the_rect.origin.y + the_rect.size.height);
}

void iupdrvDialogSetPosition(Ihandle *ih, int x, int y)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window) return;

  int inverted_y = iupCocoaComputeCartesianScreenHeightFromIup(y);
  [the_window setFrameTopLeftPoint:NSMakePoint(x, inverted_y)];
}

void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu)
{
  *menu = 0; // In Cocoa, the menu bar is not part of the window's decoration height.

  // Determine the window's style mask, which defines its decorations.
  NSWindowStyleMask style_mask = 0;
  NSWindow* the_window = cocoaDialogGetWindow(ih);

  if (the_window)
  {
    style_mask = [the_window styleMask];
  }
  else
  {
    // If the window is not yet mapped, we must infer the style mask from IUP attributes.
    // This logic is derived from what cocoaDialogMapMethod would do.
    int has_titlebar = iupAttribGet(ih, "TITLE") ||
                       iupAttribGetBoolean(ih, "MAXBOX") ||
                       iupAttribGetBoolean(ih, "MINBOX") ||
                       iupAttribGetBoolean(ih, "MENUBOX");

    if (has_titlebar || iupAttribGetBoolean(ih, "BORDER"))
      style_mask = NSWindowStyleMaskTitled;
    else
      style_mask = NSWindowStyleMaskBorderless;

    if (iupAttribGetBoolean(ih, "RESIZE"))
      style_mask |= NSWindowStyleMaskResizable;
    if (iupAttribGetBoolean(ih, "MINBOX"))
      style_mask |= NSWindowStyleMaskMiniaturizable;
    if (iupAttribGetBoolean(ih, "MENUBOX"))
      style_mask |= NSWindowStyleMaskClosable;
    if (iupAttribGetBoolean(ih, "TOOLBOX"))
      style_mask |= NSWindowStyleMaskUtilityWindow;
  }

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") || iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
  {
    style_mask = NSWindowStyleMaskBorderless;
  }

  // Using a sample content rectangle, we ask Cocoa for the corresponding frame rectangle.
  // The difference between them gives us the total size of the decorations.
  NSRect sample_content_rect = NSMakeRect(0, 0, 100, 100);
  NSRect sample_frame_rect = [NSWindow frameRectForContentRect:sample_content_rect styleMask:style_mask];

  int total_decor_width = (int)round(sample_frame_rect.size.width - sample_content_rect.size.width);
  int total_decor_height = (int)round(sample_frame_rect.size.height - sample_content_rect.size.height);

  // Translate these total decoration sizes into IUP's `border` and `caption` model.
  // The IUP layout engine formulas assume:
  // total_decor_width = 2 * border
  // total_decor_height = caption + 2 * border
  // We derive our values to satisfy these formulas.

  *border = total_decor_width / 2;
  *caption = total_decor_height - (2 * (*border));
  if (*caption < 0)
  {
    *caption = 0;
  }

  if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
  {
    *caption = 0;
  }
}

int iupdrvDialogSetPlacement(Ihandle* ih)
{
  id root_object = (id)ih->handle;
  if([root_object isKindOfClass:[NSStatusItem class]] || !root_object) return 0;

  NSWindow* the_window = cocoaDialogGetWindow(ih);
  char* placement;
  int old_state = ih->data->show_state;

  ih->data->show_state = IUP_SHOW;

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
  {
    if (!([the_window styleMask] & NSWindowStyleMaskFullScreen))
    {
      [the_window toggleFullScreen:nil];
    }
    return 1;
  }

  if ([the_window styleMask] & NSWindowStyleMaskFullScreen)
  {
    [the_window toggleFullScreen:nil];
  }

  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;

    if ([the_window isZoomed])
      [the_window zoom:nil];
    if ([the_window isMiniaturized])
      [the_window deminiaturize:nil];

    if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE") && iupDialogCustomFrameRestore(ih))
    {
      ih->data->show_state = IUP_RESTORE;
      return 1;
    }

    return 0;
  }

  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE") && iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    iupDialogCustomFrameMaximize(ih);
    iupAttribSet(ih, "PLACEMENT", NULL);
    ih->data->show_state = IUP_MAXIMIZE;
    return 1;
  }

  if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    ih->data->show_state = IUP_MINIMIZE;
    if (![the_window isMiniaturized])
      [the_window miniaturize:nil];
  }
  else if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    ih->data->show_state = IUP_MAXIMIZE;
    if (![the_window isZoomed])
      [the_window zoom:nil];
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    NSRect screen_frame = [[NSScreen mainScreen] visibleFrame];
    [the_window setFrame:screen_frame display:YES];

    if ([the_window isMiniaturized])
      [the_window deminiaturize:nil];

    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", NULL);
  return 1;
}

void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  id root_object = (id)ih->handle;
  if([root_object isKindOfClass:[NSStatusItem class]]) return;

  NSWindow* parent_window = (NSWindow*)parent;
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if ([parent_window isKindOfClass:[NSWindow class]])
  {
    [parent_window addChildWindow:the_window ordered:NSWindowAbove];
  }
}


/****************************************************************
 ********************** Attributes ******************************
 ****************************************************************/

static int cocoaDialogSetMenuAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle && [(NSWindow*)ih->handle isKeyWindow])
  {
    Ihandle* menu_ih = IupGetHandle(value);
    iupCocoaMenuSetApplicationMenu(menu_ih);
  }

  return 1;
}

static int cocoaDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window)
    return iupBaseSetMinSizeAttrib(ih, value);

  int min_w = 1, min_h = 1;
  int max_w = 65535, max_h = 65535;
  iupStrToIntInt(value, &min_w, &min_h, 'x');

  iupStrToIntInt(iupAttribGet(ih, "MAXSIZE"), &max_w, &max_h, 'x');

  int decorwidth = 0, decorheight = 0;
  iupDialogGetDecorSize(ih, &decorwidth, &decorheight);

  NSSize minSize = NSMakeSize(1, 1);
  if (min_w > decorwidth)
    minSize.width = min_w - decorwidth;
  if (min_h > decorheight)
    minSize.height = min_h - decorheight;

  NSSize maxSize = NSMakeSize(65535, 65535);
  if (max_w > decorwidth && max_w > minSize.width)
    maxSize.width = max_w - decorwidth;
  if (max_h > decorheight && max_h > minSize.height)
    maxSize.height = max_h - decorheight;

  [the_window setContentMinSize:minSize];
  [the_window setContentMaxSize:maxSize];

  return iupBaseSetMinSizeAttrib(ih, value);
}

static int cocoaDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window)
    return iupBaseSetMaxSizeAttrib(ih, value);

  int min_w = 1, min_h = 1;
  int max_w = 65535, max_h = 65535;
  iupStrToIntInt(value, &max_w, &max_h, 'x');

  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');

  int decorwidth = 0, decorheight = 0;
  iupDialogGetDecorSize(ih, &decorwidth, &decorheight);

  NSSize minSize = NSMakeSize(1, 1);
  if (min_w > decorwidth)
    minSize.width = min_w - decorwidth;
  if (min_h > decorheight)
    minSize.height = min_h - decorheight;

  NSSize maxSize = NSMakeSize(65535, 65535);
  if (max_w > decorwidth && max_w > minSize.width)
    maxSize.width = max_w - decorwidth;
  if (max_h > decorheight && max_h > minSize.height)
    maxSize.height = max_h - decorheight;

  [the_window setContentMinSize:minSize];
  [the_window setContentMaxSize:maxSize];

  return iupBaseSetMaxSizeAttrib(ih, value);
}

static char* cocoaDialogGetClientSizeAttrib(Ihandle *ih)
{
  if (ih->handle)
  {
    NSWindow* the_window = cocoaDialogGetWindow(ih);
    if (the_window)
    {
      NSRect content_rect = [[the_window contentView] bounds];
      int width = (int)content_rect.size.width;
      int height = (int)content_rect.size.height;

      if (iupAttribGetBoolean(ih, "CUSTOMFRAMEDRAW"))
      {
        int border, caption, menu;
        iupdrvDialogGetDecoration(ih, &border, &caption, &menu);
        width -= 2 * border;
        height -= caption + menu + 2 * border;
      }

      if (width < 0) width = 0;
      if (height < 0) height = 0;

      return iupStrReturnIntInt(width, height, 'x');
    }
  }

  return iupDialogGetClientSizeAttrib(ih);
}

static char* cocoaDialogGetClientOffsetAttrib(Ihandle *ih)
{
  if (iupAttribGetBoolean(ih, "CUSTOMFRAMEDRAW"))
  {
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    int x = border;
    int y = border + caption + menu;

    return iupStrReturnIntInt(x, y, 'x');
  }

  return "0x0";
}

static char* cocoaDialogGetResizeAttrib(Ihandle* ih)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window) return iupAttribGet(ih, "RESIZE");

  NSWindowStyleMask style_mask = [the_window styleMask];
  return iupStrReturnBoolean(style_mask & NSWindowStyleMaskResizable);
}

static int cocoaDialogSetResizeAttrib(Ihandle* ih, const char* value)
{
  cocoaDialogUpdateStyleMask(ih);
  return 1;
}

static int cocoaDialogSetMinBoxAttrib(Ihandle* ih, const char* value)
{
  cocoaDialogUpdateStyleMask(ih);
  return 1;
}

static int cocoaDialogSetMaxBoxAttrib(Ihandle* ih, const char* value)
{
  cocoaDialogUpdateStyleMask(ih);
  return 1;
}

static int cocoaDialogSetMenuBoxAttrib(Ihandle* ih, const char* value)
{
  cocoaDialogUpdateStyleMask(ih);
  return 1;
}

static int cocoaDialogSetBorderAttrib(Ihandle* ih, const char* value)
{
  cocoaDialogUpdateStyleMask(ih);
  return 1;
}

static int cocoaDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (the_window)
  {
    NSString* ns_string = value ? [NSString stringWithUTF8String:value] : @"";
    [the_window setTitle:ns_string];
  }

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") || iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
    return 0;

  return 1;
}

static int cocoaDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window) return 0;

  if (iupStrBoolean(value))
  {
    if (!iupAttribGet(ih, "_IUPCOCOA_FS_STYLE"))
    {
      iupAttribSetStr(ih, "_IUPCOCOA_FS_MAXBOX", iupAttribGet(ih, "MAXBOX"));
      iupAttribSetStr(ih, "_IUPCOCOA_FS_MINBOX", iupAttribGet(ih, "MINBOX"));
      iupAttribSetStr(ih, "_IUPCOCOA_FS_MENUBOX", iupAttribGet(ih, "MENUBOX"));
      iupAttribSetStr(ih, "_IUPCOCOA_FS_RESIZE", iupAttribGet(ih, "RESIZE"));
      iupAttribSetStr(ih, "_IUPCOCOA_FS_BORDER", iupAttribGet(ih, "BORDER"));
      iupAttribSetStr(ih, "_IUPCOCOA_FS_TITLE", iupAttribGet(ih, "TITLE"));

      iupAttribSetStr(ih, "_IUPCOCOA_FS_X", IupGetAttribute(ih, "X"));
      iupAttribSetStr(ih, "_IUPCOCOA_FS_Y", IupGetAttribute(ih, "Y"));
      iupAttribSetStr(ih, "_IUPCOCOA_FS_SIZE", IupGetAttribute(ih, "RASTERSIZE"));

      iupAttribSet(ih, "MAXBOX", "NO");
      iupAttribSet(ih, "MINBOX", "NO");
      iupAttribSet(ih, "MENUBOX", "NO");
      IupSetAttribute(ih, "TITLE", NULL);
      iupAttribSet(ih, "RESIZE", "NO");
      iupAttribSet(ih, "BORDER", "NO");

      [the_window toggleFullScreen:nil];

      iupAttribSet(ih, "_IUPCOCOA_FS_STYLE", "YES");
    }
  }
  else
  {
    if (iupAttribGet(ih, "_IUPCOCOA_FS_STYLE"))
    {
      iupAttribSet(ih, "_IUPCOCOA_FS_STYLE", NULL);

      iupAttribSetStr(ih, "MAXBOX", iupAttribGet(ih, "_IUPCOCOA_FS_MAXBOX"));
      iupAttribSetStr(ih, "MINBOX", iupAttribGet(ih, "_IUPCOCOA_FS_MINBOX"));
      iupAttribSetStr(ih, "MENUBOX",iupAttribGet(ih, "_IUPCOCOA_FS_MENUBOX"));
      IupSetAttribute(ih, "TITLE", iupAttribGet(ih, "_IUPCOCOA_FS_TITLE"));
      iupAttribSetStr(ih, "RESIZE", iupAttribGet(ih, "_IUPCOCOA_FS_RESIZE"));
      iupAttribSetStr(ih, "BORDER", iupAttribGet(ih, "_IUPCOCOA_FS_BORDER"));

      [the_window toggleFullScreen:nil];

      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        int x = iupAttribGetInt(ih, "_IUPCOCOA_FS_X");
        int y = iupAttribGetInt(ih, "_IUPCOCOA_FS_Y");
        int w = IupGetInt(ih, "_IUPCOCOA_FS_SIZE");
        int h = IupGetInt2(ih, "_IUPCOCOA_FS_SIZE");

        if (w > 0 && h > 0)
        {
          iupdrvDialogSetPosition(ih, x, y);
          NSRect frame = [the_window frame];
          frame.size.width = w;
          frame.size.height = h;
          [the_window setFrame:frame display:YES];
        }
      });

      iupAttribSet(ih, "_IUPCOCOA_FS_MAXBOX", NULL);
      iupAttribSet(ih, "_IUPCOCOA_FS_MINBOX", NULL);
      iupAttribSet(ih, "_IUPCOCOA_FS_MENUBOX", NULL);
      iupAttribSet(ih, "_IUPCOCOA_FS_TITLE", NULL);
      iupAttribSet(ih, "_IUPCOCOA_FS_RESIZE", NULL);
      iupAttribSet(ih, "_IUPCOCOA_FS_BORDER", NULL);
      iupAttribSet(ih, "_IUPCOCOA_FS_X", NULL);
      iupAttribSet(ih, "_IUPCOCOA_FS_Y", NULL);
      iupAttribSet(ih, "_IUPCOCOA_FS_SIZE", NULL);
    }
  }

  return 1;
}

static char* cocoaDialogGetFullScreenAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "_IUPCOCOA_FULLSCREEN");
}

static int cocoaDialogSetHelpButtonAttrib(Ihandle* ih, const char* value)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window) return 0;

  [the_window setShowsHelpButton:(BOOL)iupStrBoolean(value)];
  return 1;
}

static int cocoaDialogSetDialogHintAttrib(Ihandle* ih, const char* value)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window) return 0;

  if (iupStrBoolean(value))
    [the_window setLevel:NSFloatingWindowLevel];
  else
    [the_window setLevel:NSNormalWindowLevel];

  return 1;
}

static int cocoaDialogSetHideTitleBarAttrib(Ihandle *ih, const char *value)
{
  NSWindow* window = cocoaDialogGetWindow(ih);
  if (!window) return 0;

  if (@available(macOS 10.10, *))
  {
    BOOL hide = (BOOL)iupStrBoolean(value);
    if (hide)
    {
      window.styleMask |= NSWindowStyleMaskFullSizeContentView;
      window.titlebarAppearsTransparent = YES;
      window.titleVisibility = NSWindowTitleHidden;
    }
    else
    {
      window.styleMask &= ~NSWindowStyleMaskFullSizeContentView;
      window.titlebarAppearsTransparent = NO;
      window.titleVisibility = NSWindowTitleVisible;
    }
  }
  return 1;
}

static char* cocoaDialogGetActiveWindowAttrib(Ihandle* ih)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window)
    return NULL;

  return iupStrReturnBoolean([the_window isKeyWindow]);
}

static int cocoaDialogSetTopMostAttrib(Ihandle *ih, const char *value)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window) return 0;

  if (iupStrBoolean(value))
    [the_window setLevel:NSStatusWindowLevel];
  else
    [the_window setLevel:NSNormalWindowLevel];
  return 1;
}

static int cocoaDialogSetBringFrontAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
  {
    NSWindow* the_window = cocoaDialogGetWindow(ih);
    if (the_window)
    {
      [NSApp activateIgnoringOtherApps:YES];
      [the_window makeKeyAndOrderFront:nil];
    }
  }
  return 0;
}

static int cocoaDialogSetOpacityAttrib(Ihandle *ih, const char *value)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window) return 0;

  int opacity;
  if (iupStrToInt(value, &opacity))
  {
    if (opacity < 0) opacity = 0;
    if (opacity > 255) opacity = 255;
    [the_window setAlphaValue:(CGFloat)opacity / 255.0];
    return 1;
  }
  return 0;
}

static int cocoaDialogSetIconAttrib(Ihandle* ih, const char *value)
{
  NSImage* icon = iupImageGetIcon(value);
  [NSApp setApplicationIconImage:icon];
  return 1;
}

static int cocoaDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window) return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
    [the_window setBackgroundColor:color];
    return 1;
  }
  else
  {
    NSImage* image = iupImageGetImage(value, ih, 0, NULL);
    if (image)
    {
      [the_window setBackgroundColor:[NSColor colorWithPatternImage:image]];
      return 1;
    }
  }
  return 0;
}

static int cocoaDialogSetShapeImageAttrib(Ihandle *ih, const char *value)
{
  NSWindow* window = cocoaDialogGetWindow(ih);
  if (!window) return 0;

  if (!value)
  {
    cocoaDialogUpdateStyleMask(ih);
    [window setOpaque:YES];
    [window setBackgroundColor:[NSColor windowBackgroundColor]];
    return 1;
  }

  NSImage* image = iupImageGetImage(value, ih, 0, NULL);
  if (!image)
    return 0;

  [window setStyleMask:NSWindowStyleMaskBorderless];
  [window setOpaque:NO];
  [window setBackgroundColor:[NSColor clearColor]];
  [window setHasShadow:YES];
  [window setMovableByWindowBackground:YES];

  NSImageView* imageView = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 0, image.size.width, image.size.height)];
  [imageView setImage:image];
  [imageView setImageFrameStyle:NSImageFrameNone];
  [imageView setImageScaling:NSImageScaleNone];

  [window setContentView:imageView];
  [window setContentSize:image.size];
  [imageView release];

  return 1;
}

static int cocoaDialogSetOpacityImageAttrib(Ihandle *ih, const char *value)
{
  return cocoaDialogSetShapeImageAttrib(ih, value);
}

static int cocoaDialogSetCustomFrameAttrib(Ihandle *ih, const char *value)
{
  NSWindow* window = cocoaDialogGetWindow(ih);
  if (!window) return 0;

  if (iupStrBoolean(value))
  {
    [window setStyleMask:NSWindowStyleMaskBorderless];
    [window setMovableByWindowBackground:YES];
    [window setHasShadow:YES];
  }
  else
  {
    cocoaDialogUpdateStyleMask(ih);
    [window setMovableByWindowBackground:NO];
  }
  return 1;
}

static char* cocoaDialogGetMaximizedAttrib(Ihandle *ih)
{
  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
    return iupAttribGet(ih, "MAXIMIZED");

  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window)
    return NULL;

  return iupStrReturnBoolean([the_window isZoomed]);
}

static char* cocoaDialogGetMinimizedAttrib(Ihandle *ih)
{
  NSWindow* the_window = cocoaDialogGetWindow(ih);
  if (!the_window)
    return NULL;

  return iupStrReturnBoolean([the_window isMiniaturized]);
}


/****************************************************************
 ********************** Class Methods ***************************
 ****************************************************************/

static void cocoaDialogSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  (void)x;
  (void)y;

  if (ih->firstchild)
  {
    char* offset = iupAttribGet(ih, "CHILDOFFSET");
    int offset_x = 0;
    int offset_y = 0;

    if (offset)
      iupStrToIntInt(offset, &offset_x, &offset_y, 'x');

    iupBaseSetPosition(ih->firstchild, ih->firstchild->x + offset_x, ih->firstchild->y + offset_y);
  }
}

static int cocoaDialogMapMethod(Ihandle* ih)
{
  InativeHandle* parent;
  NSWindowStyleMask style_mask = cocoaDialogGetStyleMask(ih);

  NSWindow* the_window = [[IupCocoaWindow alloc] initWithContentRect:NSMakeRect(0, 0, 100, 100)
                                                   styleMask:style_mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];

  [the_window setReleasedWhenClosed:NO];
  [the_window setAutorecalculatesKeyViewLoop:YES];

  [[the_window contentView] setAutoresizesSubviews:NO];

  ih->handle = (__bridge void*)the_window;
  iupCocoaSetAssociatedViews(ih, [the_window contentView], [the_window contentView]);

  IupCocoaWindowDelegate* window_delegate = [[IupCocoaWindowDelegate alloc] init];
  [the_window setDelegate:window_delegate];
  objc_setAssociatedObject(the_window, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

  parent = iupDialogGetNativeParent(ih);
  if (parent && [parent isKindOfClass:[NSWindow class]])
  {
    NSWindow* parent_window = (NSWindow*)parent;
    [parent_window addChildWindow:the_window ordered:NSWindowAbove];

    [[NSNotificationCenter defaultCenter] addObserver:window_delegate
                                             selector:@selector(cocoaDialogChildDestroyNotification:)
                                                 name:NSWindowWillCloseNotification
                                               object:the_window];
  }

  NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
  [nc addObserver:window_delegate selector:@selector(windowDidResize:) name:NSWindowDidResizeNotification object:the_window];
  [nc addObserver:window_delegate selector:@selector(windowDidMove:) name:NSWindowDidMoveNotification object:the_window];
  [nc addObserver:window_delegate selector:@selector(windowDidMiniaturize:) name:NSWindowDidMiniaturizeNotification object:the_window];
  [nc addObserver:window_delegate selector:@selector(windowDidDeminiaturize:) name:NSWindowDidDeminiaturizeNotification object:the_window];
  [nc addObserver:window_delegate selector:@selector(windowWillEnterFullScreen:) name:NSWindowWillEnterFullScreenNotification object:the_window];
  [nc addObserver:window_delegate selector:@selector(windowDidEnterFullScreen:) name:NSWindowDidEnterFullScreenNotification object:the_window];
  [nc addObserver:window_delegate selector:@selector(windowDidExitFullScreen:) name:NSWindowDidExitFullScreenNotification object:the_window];
  [nc addObserver:window_delegate selector:@selector(windowDidBecomeKey:) name:NSWindowDidBecomeKeyNotification object:the_window];
  [nc addObserver:window_delegate selector:@selector(windowDidResignKey:) name:NSWindowDidResignKeyNotification object:the_window];

  if (@available(macOS 10.14, *))
  {
    [the_window addObserver:window_delegate
                 forKeyPath:@"effectiveAppearance"
                    options:NSKeyValueObservingOptionNew
                    context:IupCocoaAppearanceContext];
  }

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  iupAttribSet(ih, "VISIBLE", NULL);

  return IUP_NOERROR;
}

static void cocoaDialogUnMapMethod(Ihandle* ih)
{
  if (!ih->handle) return;

  if (ih->data->menu)
  {
    ih->data->menu->handle = NULL;
    IupDestroy(ih->data->menu);
    ih->data->menu = NULL;
  }

  NSStatusItem* status_item = (NSStatusItem*)iupAttribGet(ih, "_IUPCOCOA_STATUSITEM");
  if (status_item)
  {
    Ihandle* menu_ih = (Ihandle*)objc_getAssociatedObject(status_item, TRAYMENUIHANDLE_ASSOCIATED_OBJ_KEY);
    if (menu_ih)
      IupDestroy(menu_ih);

    objc_setAssociatedObject([status_item button], IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
    objc_setAssociatedObject(status_item, TRAYCLICK_DELEGATE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(status_item, TRAYMENUIHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

    [[NSStatusBar systemStatusBar] removeStatusItem:status_item];
    [status_item release];
    iupAttribSet(ih, "_IUPCOCOA_STATUSITEM", NULL);
  }

  NSWindow* the_window = (__bridge NSWindow*)ih->handle;

  NSWindow* parent_window = [the_window parentWindow];
  if (parent_window)
  {
    [[NSNotificationCenter defaultCenter] removeObserver:[parent_window delegate]
                                                     name:NSWindowWillCloseNotification
                                                   object:the_window];
    [parent_window removeChildWindow:the_window];
  }

  NSArray<NSWindow*>* child_windows = [[the_window childWindows] copy];
  for (NSWindow* child_window in child_windows)
  {
    Ihandle* child_ih = (Ihandle*)objc_getAssociatedObject(child_window, IHANDLE_ASSOCIATED_OBJ_KEY);
    if (iupObjectCheck(child_ih))
    {
      if(iupAttribGetBoolean(child_ih, "MODAL"))
      {
        cocoaDialogExitModal(child_ih);
      }
      IupDestroy(child_ih);
    }
  }
  [child_windows release];

  if (@available(macOS 10.14, *))
  {
    @try
    {
      [the_window removeObserver:[the_window delegate]
                      forKeyPath:@"effectiveAppearance"
                         context:IupCocoaAppearanceContext];
    }
    @catch (NSException *exception)
    {
      /* Observer might not have been added, ignore */
    }
  }

  [[NSNotificationCenter defaultCenter] removeObserver:[the_window delegate] name:nil object:the_window];

  objc_setAssociatedObject(the_window, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

  [the_window close];

  IupCocoaWindowDelegate* window_delegate = [the_window delegate];
  [the_window setDelegate:nil];
  [window_delegate release];

  iupCocoaSetAssociatedViews(ih, nil, nil);
  [the_window release];
  ih->handle = NULL;
}

static void cocoaDialogLayoutUpdateMethod(Ihandle *ih)
{
  int width, height;

  id root_object = (id)ih->handle;
  if([root_object isKindOfClass:[NSStatusItem class]] || !root_object) return;

  if(ih->data->ignore_resize || iupAttribGet(ih, "_IUPCOCOA_FS_STYLE"))
    return;

  ih->data->ignore_resize = 1;

  NSWindow* the_window = cocoaDialogGetWindow(ih);

  if (!the_window)
  {
    ih->data->ignore_resize = 0;
    return;
  }

  if ([the_window isMiniaturized])
  {
    ih->data->ignore_resize = 0;
    return;
  }

  if ([the_window isZoomed])
  {
    NSRect zoomed_frame = [the_window frame];

    /* Compare the desired total size from IUP with the current zoomed frame size. */
    /* If they differ significantly, we should unzoom before applying the new size. */
    if (abs(ih->currentwidth - (int)zoomed_frame.size.width) > 10 ||
        abs(ih->currentheight - (int)zoomed_frame.size.height) > 10)
    {
      /* Unzoom before resizing */
      [the_window setStyleMask:[the_window styleMask] & ~NSWindowStyleMaskFullScreen];
      [the_window zoom:nil];
      /* Clear the zoom restore frame */
      objc_setAssociatedObject(the_window, &IUPCocoaZoomRestoreFrameKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
      /* Update show state */
      ih->data->show_state = IUP_RESTORE;
    }
    else
    {
      /* Size is similar to zoomed size, don't unzoom */
      ih->data->ignore_resize = 0;
      return;
    }
  }

  int decor_w, decor_h;
  NSRect sample_content_rect = NSMakeRect(0, 0, 100, 100);
  NSRect sample_frame_rect = [NSWindow frameRectForContentRect:sample_content_rect styleMask:[the_window styleMask]];
  decor_w = (int)round(sample_frame_rect.size.width - sample_content_rect.size.width);
  decor_h = (int)round(sample_frame_rect.size.height - sample_content_rect.size.height);

  width = ih->currentwidth - decor_w;
  height = ih->currentheight - decor_h;
  if(width <= 0) width = 1;
  if(height <= 0) height = 1;

  NSSize content_size = NSMakeSize(width, height);

  /* Clear any existing size constraints before resizing */
  [the_window setContentMinSize:NSMakeSize(1, 1)];
  [the_window setContentMaxSize:NSMakeSize(65535, 65535)];

  [the_window setContentSize:content_size];

  if (!iupAttribGetBoolean(ih, "RESIZE"))
  {
    [the_window setContentMinSize:content_size];
    [the_window setContentMaxSize:content_size];
  }

  ih->data->ignore_resize = 0;
}

/****************************************************************
 ********************** TRAY Attributes *************************
 ****************************************************************/

static int cocoaDialogSetTrayAttrib(Ihandle* ih, const char* value)
{
  NSStatusItem* status_item = (NSStatusItem*)iupAttribGet(ih, "_IUPCOCOA_STATUSITEM");

  if (iupStrBoolean(value))
  {
    if (!status_item)
    {
      NSStatusBar* status_bar = [NSStatusBar systemStatusBar];
      status_item = [status_bar statusItemWithLength:NSVariableStatusItemLength];
      [status_item retain];

      iupAttribSet(ih, "_IUPCOCOA_STATUSITEM", (char*)status_item);

      IupTrayClickDelegate* delegate = [[IupTrayClickDelegate alloc] init];
      NSStatusBarButton* button = [status_item button];
      [button setTarget:delegate];
      [button setAction:@selector(trayAction:)];
      [button sendActionOn:NSEventMaskLeftMouseDown | NSEventMaskRightMouseDown |
        NSEventMaskLeftMouseUp | NSEventMaskRightMouseUp |
          NSEventMaskOtherMouseDown | NSEventMaskOtherMouseUp];

      objc_setAssociatedObject(button, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
      objc_setAssociatedObject(status_item, TRAYCLICK_DELEGATE_ASSOCIATED_OBJ_KEY, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
      [delegate release];
    }

    [status_item setVisible:YES];
  }
  else
  {
    if (status_item)
    {
      [status_item setVisible:NO];
    }
  }

  return 1;
}

static int cocoaDialogSetTrayImageAttrib(Ihandle* ih, const char* value)
{
  NSStatusItem* status_item = cocoaDialogGetStatusItem(ih);
  if (!status_item) return 0;

  NSImage* user_image = (NSImage*)iupImageGetIcon(value);
  if (user_image)
  {
    CGFloat height = [[NSStatusBar systemStatusBar] thickness];
    [user_image setSize:NSMakeSize(height-4, height-4)];
    [user_image setTemplate:YES];
    [[status_item button] setImage:user_image];
    return 1;
  }
  return 0;
}

static char* cocoaDialogGetTrayMenuAttrib(Ihandle* ih)
{
  NSStatusItem* status_item = cocoaDialogGetStatusItem(ih);
  if (!status_item) return NULL;

  return (char*)objc_getAssociatedObject(status_item, TRAYMENUIHANDLE_ASSOCIATED_OBJ_KEY);
}

static int cocoaDialogSetTrayMenuAttrib(Ihandle* ih, const char* value)
{
  NSStatusItem* status_item = cocoaDialogGetStatusItem(ih);
  if (!status_item) return 0;

  Ihandle* menu_ih = (Ihandle*)value;
  Ihandle* old_menu_ih = (Ihandle*)objc_getAssociatedObject(status_item, TRAYMENUIHANDLE_ASSOCIATED_OBJ_KEY);

  if (old_menu_ih != menu_ih)
  {
    IupDestroy(old_menu_ih);
  }

  if(NULL == menu_ih)
  {
    [status_item setMenu:nil];
    objc_setAssociatedObject(status_item, TRAYMENUIHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
    return 1;
  }

  if(NULL == menu_ih->handle) IupMap(menu_ih);

  if(menu_ih->iclass->nativetype == IUP_TYPEMENU && [(id)menu_ih->handle isKindOfClass:[NSMenu class]])
  {
    NSMenu* the_menu = (NSMenu*)menu_ih->handle;
    [status_item setMenu:the_menu];
    objc_setAssociatedObject(status_item, TRAYMENUIHANDLE_ASSOCIATED_OBJ_KEY, (id)menu_ih, OBJC_ASSOCIATION_ASSIGN);
  }
  return 1;
}

static int cocoaDialogSetTrayTipAttrib(Ihandle* ih, const char* value)
{
  NSStatusItem* status_item = cocoaDialogGetStatusItem(ih);
  if (status_item && [status_item.button respondsToSelector:@selector(setToolTip:)])
  {
    status_item.button.toolTip = value ? [NSString stringWithUTF8String:value] : nil;
    return 1;
  }
  return 0;
}


/****************************************************************
 ******************** Other Attributes **************************
 ****************************************************************/

static int cocoaDialogSetTaskBarProgressAttrib(Ihandle *ih, const char *value)
{
  if (iupStrBoolean(value))
  {
    NSProgress* progress = [NSProgress progressWithTotalUnitCount:100];
    progress.completedUnitCount = 0;
    [[NSApp dockTile] setBadgeLabel:nil];
    objc_setAssociatedObject((id)ih->handle, DOCKPROGRESS_ASSOCIATED_OBJ_KEY, progress, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  else
  {
    [[NSApp dockTile] setBadgeLabel:nil];
    objc_setAssociatedObject((id)ih->handle, DOCKPROGRESS_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  return 1;
}

static int cocoaDialogSetTaskBarProgressStateAttrib(Ihandle *ih, const char *value)
{
  NSProgress* progress = objc_getAssociatedObject((id)ih->handle, DOCKPROGRESS_ASSOCIATED_OBJ_KEY);
  if (progress)
  {
    if (iupStrEqualNoCase(value, "NOPROGRESS"))
    {
      cocoaDialogSetTaskBarProgressAttrib(ih, "NO");
    }
    else if (iupStrEqualNoCase(value, "INDETERMINATE"))
    {
      [progress setIndeterminate:YES];
      [[NSApp dockTile] setBadgeLabel:@"◉"];
    }
    else if (iupStrEqualNoCase(value, "PAUSED"))
    {
      [progress setIndeterminate:NO];
      [progress setPaused:YES];
    }
    else
    {
      [progress setIndeterminate:NO];
      [progress setPaused:NO];
      [[NSApp dockTile] setBadgeLabel:nil];
    }
  }
  return 1;
}

static int cocoaDialogSetTaskBarProgressValueAttrib(Ihandle *ih, const char *value)
{
  NSProgress* progress = objc_getAssociatedObject((id)ih->handle, DOCKPROGRESS_ASSOCIATED_OBJ_KEY);
  if (progress)
  {
    int int_value;
    if (iupStrToInt(value, &int_value))
    {
      if (int_value < 0) int_value = 0;
      if (int_value > 100) int_value = 100;

      progress.completedUnitCount = int_value;

      if (int_value > 0 && int_value < 100)
      {
        NSString* badge_label = [NSString stringWithFormat:@"%d%%", int_value];
        [[NSApp dockTile] setBadgeLabel:badge_label];
      }
      else if (int_value >= 100)
      {
        [[NSApp dockTile] setBadgeLabel:nil];
        cocoaDialogSetTaskBarProgressAttrib(ih, "NO");
      }
    }
  }
  return 1;
}


/****************************************************************
 ******************** Class Registration ************************
 ****************************************************************/

void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = cocoaDialogMapMethod;
  ic->UnMap = cocoaDialogUnMapMethod;
  ic->LayoutUpdate = cocoaDialogLayoutUpdateMethod;
  ic->SetChildrenPosition = cocoaDialogSetChildrenPositionMethod;

  iupClassRegisterCallback(ic, "TRAYCLICK_CB", "iii");
  iupClassRegisterCallback(ic, "MOVE_CB", "ii");
  iupClassRegisterCallback(ic, "THEMECHANGED_CB", "i");

  iupClassRegisterAttribute(ic, "CLIENTSIZE", cocoaDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", cocoaDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MENU", NULL, cocoaDialogSetMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZE", cocoaDialogGetResizeAttrib, cocoaDialogSetResizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BORDER", NULL, cocoaDialogSetBorderAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINBOX", NULL, cocoaDialogSetMinBoxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXBOX", NULL, cocoaDialogSetMaxBoxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUBOX", NULL, cocoaDialogSetMenuBoxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", NULL, cocoaDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, cocoaDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", cocoaDialogGetFullScreenAttrib, cocoaDialogSetFullScreenAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, cocoaDialogSetCustomFrameAttrib, NULL, "NO", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWNOACTIVATE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, cocoaDialogSetDialogHintAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, cocoaDialogSetHelpButtonAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETITLEBAR", NULL, cocoaDialogSetHideTitleBarAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, cocoaDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, cocoaDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITY", NULL, cocoaDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHAPEIMAGE", NULL, cocoaDialogSetShapeImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, cocoaDialogSetOpacityImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TOPMOST", NULL, cocoaDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", cocoaDialogGetActiveWindowAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXIMIZED", cocoaDialogGetMaximizedAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINIMIZED", cocoaDialogGetMinimizedAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, cocoaDialogSetBringFrontAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TRAY", NULL, cocoaDialogSetTrayAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYIMAGE", NULL, cocoaDialogSetTrayImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYMENU", cocoaDialogGetTrayMenuAttrib, cocoaDialogSetTrayMenuAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIP", NULL, cocoaDialogSetTrayTipAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TASKBARPROGRESS", NULL, cocoaDialogSetTaskBarProgressAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TASKBARPROGRESSSTATE", NULL, cocoaDialogSetTaskBarProgressStateAttrib, IUPAF_SAMEASSYSTEM, "NORMAL", IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TASKBARPROGRESSVALUE", NULL, cocoaDialogSetTaskBarProgressValueAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupCocoaCommonBaseSetLayerBackedAttrib, NULL, "NO", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIPBALLOON", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIPBALLOONTITLE", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIPBALLOONTITLEICON", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIPDELAY", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MDIFRAME", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLIENT", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIMENU", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICHILD", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
