/** \file
 * \brief Cocoa System Tray Driver
 *
 * This implementation is standalone and can be used with any IUP backend on macOS (native Cocoa, Qt, GTK).
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#include <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_class.h"
#include "iup_tray.h"

#if defined(IUP_USE_GTK3) || defined(IUP_USE_GTK4) || defined(IUP_USE_GTK2) || defined(IUP_USE_QT)
#define IUPCOCOA_TRAY_USE_EXTERNAL_IMAGE

static NSImage* cocoaTrayCreateImageFromPixels(int width, int height, unsigned char* pixels)
{
  NSBitmapImageRep* rep;
  NSImage* image;
  int x, y;
  unsigned char* destPixels;

  if (!pixels || width <= 0 || height <= 0)
    return nil;

  rep = [[NSBitmapImageRep alloc]
      initWithBitmapDataPlanes:NULL
                    pixelsWide:width
                    pixelsHigh:height
                 bitsPerSample:8
               samplesPerPixel:4
                      hasAlpha:YES
                      isPlanar:NO
                colorSpaceName:NSDeviceRGBColorSpace
                   bytesPerRow:width * 4
                  bitsPerPixel:32];

  if (!rep)
    return nil;

  destPixels = [rep bitmapData];

  for (y = 0; y < height; y++)
  {
    for (x = 0; x < width; x++)
    {
      int srcOffset = (y * width + x) * 4;
      int dstOffset = (y * width + x) * 4;
      unsigned char a = pixels[srcOffset + 0];
      unsigned char r = pixels[srcOffset + 1];
      unsigned char g = pixels[srcOffset + 2];
      unsigned char b = pixels[srcOffset + 3];

      destPixels[dstOffset + 0] = r;
      destPixels[dstOffset + 1] = g;
      destPixels[dstOffset + 2] = b;
      destPixels[dstOffset + 3] = a;
    }
  }

  image = [[NSImage alloc] initWithSize:NSMakeSize(width, height)];
  [image addRepresentation:rep];
  [rep release];

  return image;
}

static const void* TRAYMENU_ITEM_IHANDLE_KEY = @"TRAYMENU_ITEM_IHANDLE_KEY";

@interface IupTrayMenuItemTarget : NSObject
{
  Ihandle* _ih;
  Ihandle* _tray_ih;
}
@property (nonatomic, assign) Ihandle* ih;
@property (nonatomic, assign) Ihandle* tray_ih;
- (instancetype) initWithIhandle:(Ihandle*)ih trayHandle:(Ihandle*)tray_ih;
- (void) onMenuItemAction:(id)sender;
@end

@implementation IupTrayMenuItemTarget
@synthesize ih = _ih;
@synthesize tray_ih = _tray_ih;

- (instancetype) initWithIhandle:(Ihandle*)ih trayHandle:(Ihandle*)tray_ih
{
  self = [super init];
  if (self)
  {
    _ih = ih;
    _tray_ih = tray_ih;
  }
  return self;
}

- (void) dealloc
{
  _ih = NULL;
  _tray_ih = NULL;
  [super dealloc];
}

- (void) onMenuItemAction:(id)sender
{
  if (!_ih) return;

  NSMenuItem* menu_item = (NSMenuItem*)sender;

  if (iupStrEqual(_ih->iclass->name, "submenu"))
    return;

  if (_ih->parent && iupAttribGetBoolean(_ih->parent, "RADIO"))
  {
    Ihandle* child;
    for (child = _ih->parent->firstchild; child; child = child->brother)
    {
      if (iupStrEqual(child->iclass->name, "item"))
      {
        iupAttribSet(child, "VALUE", (child == _ih) ? "ON" : "OFF");
      }
    }
    [menu_item setState:NSControlStateValueOn];
  }
  else if (iupAttribGetBoolean(_ih, "AUTOTOGGLE"))
  {
    NSInteger current_state = [menu_item state];
    BOOL new_state = (current_state != NSControlStateValueOn);
    iupAttribSet(_ih, "VALUE", new_state ? "ON" : "OFF");
    [menu_item setState:new_state ? NSControlStateValueOn : NSControlStateValueOff];
  }

  Icallback cb = IupGetCallback(_ih, "ACTION");
  if (cb && cb(_ih) == IUP_CLOSE)
    IupExitLoop();
}

@end

static NSMenu* cocoaTrayBuildMenuFromIhandle(Ihandle* menu_ih, Ihandle* tray_ih);

static void cocoaTrayBuildMenuItems(NSMenu* ns_menu, Ihandle* parent_ih, Ihandle* tray_ih)
{
  Ihandle* child;

  for (child = parent_ih->firstchild; child; child = child->brother)
  {
    if (iupStrEqual(child->iclass->name, "separator"))
    {
      [ns_menu addItem:[NSMenuItem separatorItem]];
    }
    else if (iupStrEqual(child->iclass->name, "item"))
    {
      char* title = iupAttribGet(child, "TITLE");
      if (!title) title = "";

      char* title_str = iupStrProcessMnemonic(title, NULL, 0);
      NSString* ns_title = title_str ? [NSString stringWithUTF8String:title_str] : @"";
      if (title_str && title_str != title) free(title_str);

      NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:ns_title
                                                    action:@selector(onMenuItemAction:)
                                             keyEquivalent:@""];

      IupTrayMenuItemTarget* target = [[IupTrayMenuItemTarget alloc] initWithIhandle:child trayHandle:tray_ih];
      [item setTarget:target];
      objc_setAssociatedObject(item, TRAYMENU_ITEM_IHANDLE_KEY, target, OBJC_ASSOCIATION_RETAIN);
      [target release];

      char* value = iupAttribGet(child, "VALUE");
      if (value && iupStrBoolean(value))
        [item setState:NSControlStateValueOn];

      char* active = iupAttribGet(child, "ACTIVE");
      if (active && !iupStrBoolean(active))
        [item setEnabled:NO];

      [ns_menu addItem:item];
      [item release];
    }
    else if (iupStrEqual(child->iclass->name, "submenu"))
    {
      char* title = iupAttribGet(child, "TITLE");
      if (!title) title = "";

      char* title_str = iupStrProcessMnemonic(title, NULL, 0);
      NSString* ns_title = title_str ? [NSString stringWithUTF8String:title_str] : @"";
      if (title_str && title_str != title) free(title_str);

      NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:ns_title action:nil keyEquivalent:@""];

      Ihandle* submenu_ih = child->firstchild;
      if (submenu_ih && iupStrEqual(submenu_ih->iclass->name, "menu"))
      {
        NSMenu* submenu = cocoaTrayBuildMenuFromIhandle(submenu_ih, tray_ih);
        [submenu setTitle:ns_title];
        [item setSubmenu:submenu];
        [submenu release];
      }

      char* active = iupAttribGet(child, "ACTIVE");
      if (active && !iupStrBoolean(active))
        [item setEnabled:NO];

      [ns_menu addItem:item];
      [item release];
    }
    else if (iupStrEqual(child->iclass->name, "menu"))
    {
      cocoaTrayBuildMenuItems(ns_menu, child, tray_ih);
    }
  }
}

static NSMenu* cocoaTrayBuildMenuFromIhandle(Ihandle* menu_ih, Ihandle* tray_ih)
{
  NSMenu* ns_menu = [[NSMenu alloc] initWithTitle:@""];
  [ns_menu setAutoenablesItems:NO];

  cocoaTrayBuildMenuItems(ns_menu, menu_ih, tray_ih);

  return ns_menu;
}
#endif

static const void* IHANDLE_ASSOCIATED_OBJ_KEY = @"IHANDLE_ASSOCIATED_OBJ_KEY";
static const void* TRAYCLICK_DELEGATE_ASSOCIATED_OBJ_KEY = @"TRAYCLICK_DELEGATE_ASSOCIATED_OBJ_KEY";
static const void* TRAYMENU_TRAY_IHANDLE_KEY = @"TRAYMENU_TRAY_IHANDLE_KEY";
static const void* TRAY_EVENT_MONITOR_KEY = @"TRAY_EVENT_MONITOR_KEY";

static int g_tray_last_button = 1;
static int g_tray_last_dclick = 0;

@interface IupTrayClickDelegate : NSObject
- (void) trayAction:(id)sender;
@end

@implementation IupTrayClickDelegate
- (void) trayAction:(id)sender
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(sender, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!iupObjectCheck(ih)) return;

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

  IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
  if (cb)
  {
    int ret = cb(ih, button, pressed, dclick);
    if (ret == IUP_CLOSE)
    {
      IupExitLoop();
    }
  }
}
@end

int iupcocoaTrayGetLastButton(void)
{
  return g_tray_last_button;
}

int iupcocoaTrayGetLastDclick(void)
{
  return g_tray_last_dclick;
}

static NSStatusItem* cocoaGetStatusItem(Ihandle* ih, int create)
{
  NSStatusItem* status_item = (NSStatusItem*)iupAttribGet(ih, "_IUPCOCOA_STATUSITEM");

  if (!status_item && create)
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

    id eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:
      NSEventMaskLeftMouseDown | NSEventMaskRightMouseDown | NSEventMaskOtherMouseDown
      handler:^NSEvent*(NSEvent* event) {
        NSWindow* eventWindow = [event window];
        NSWindow* buttonWindow = [button window];
        if (eventWindow && buttonWindow && eventWindow == buttonWindow)
        {
          NSEventType event_type = [event type];
          switch(event_type)
          {
            case NSEventTypeLeftMouseDown:
              g_tray_last_button = 1;
              break;
            case NSEventTypeRightMouseDown:
              g_tray_last_button = 3;
              break;
            case NSEventTypeOtherMouseDown:
              g_tray_last_button = 2;
              break;
            default:
              g_tray_last_button = 1;
              break;
          }
          g_tray_last_dclick = ([event clickCount] >= 2) ? 1 : 0;
          if (([event modifierFlags] & NSEventModifierFlagControl) && g_tray_last_button == 1)
            g_tray_last_button = 3;
        }
        return event;
      }];
    objc_setAssociatedObject(status_item, TRAY_EVENT_MONITOR_KEY, eventMonitor, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }

  return status_item;
}

/******************************************************************************/
/* Driver Interface Implementation                                            */
/******************************************************************************/

int iupdrvTraySetVisible(Ihandle* ih, int visible)
{
  NSStatusItem* status_item = cocoaGetStatusItem(ih, visible);

  if (visible)
  {
    if (status_item)
      [status_item setVisible:YES];
  }
  else
  {
    if (status_item)
      [status_item setVisible:NO];
  }

  return 1;
}

int iupdrvTraySetTip(Ihandle* ih, const char* value)
{
  NSStatusItem* status_item = cocoaGetStatusItem(ih, 1);
  if (!status_item) return 0;

  if ([status_item.button respondsToSelector:@selector(setToolTip:)])
  {
    status_item.button.toolTip = value ? [NSString stringWithUTF8String:value] : nil;
    return 1;
  }
  return 0;
}

int iupdrvTraySetImage(Ihandle* ih, const char* value)
{
  NSStatusItem* status_item = cocoaGetStatusItem(ih, 1);
  NSImage* user_image = nil;

  if (!status_item) return 0;

#ifdef IUPCOCOA_TRAY_USE_EXTERNAL_IMAGE
  {
    int img_width, img_height;
    unsigned char* pixels;
    if (value && iupdrvGetIconPixels(ih, value, &img_width, &img_height, &pixels))
    {
      user_image = cocoaTrayCreateImageFromPixels(img_width, img_height, pixels);
      free(pixels);
    }
  }
#else
  user_image = (NSImage*)iupImageGetIcon(value);
#endif

  if (user_image)
  {
    CGFloat barHeight = [[NSStatusBar systemStatusBar] thickness];
    [user_image setSize:NSMakeSize(barHeight-4, barHeight-4)];
    [user_image setTemplate:YES];
    [[status_item button] setImage:user_image];
    return 1;
  }
  return 0;
}

int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu)
{
  NSStatusItem* status_item = cocoaGetStatusItem(ih, 1);
  if (!status_item) return 0;

  NSMenu* old_menu = [status_item menu];
  if (old_menu)
  {
    objc_setAssociatedObject(old_menu, TRAYMENU_TRAY_IHANDLE_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
#ifdef IUPCOCOA_TRAY_USE_EXTERNAL_IMAGE
    [old_menu release];
#endif
  }

  if (menu && iupObjectCheck(menu))
  {
#ifdef IUPCOCOA_TRAY_USE_EXTERNAL_IMAGE
    NSMenu* ns_menu = cocoaTrayBuildMenuFromIhandle(menu, ih);
#else
    if (!menu->handle)
    {
      if (IupMap(menu) == IUP_ERROR)
        return 0;
    }
    NSMenu* ns_menu = (NSMenu*)menu->handle;
#endif
    [status_item setMenu:ns_menu];
    objc_setAssociatedObject(ns_menu, TRAYMENU_TRAY_IHANDLE_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
  }
  else
  {
    [status_item setMenu:nil];
  }

  return 1;
}

void iupdrvTrayDestroy(Ihandle* ih)
{
  NSStatusItem* status_item = (NSStatusItem*)iupAttribGet(ih, "_IUPCOCOA_STATUSITEM");

  if (status_item)
  {
#ifdef IUPCOCOA_TRAY_USE_EXTERNAL_IMAGE
    NSMenu* menu = [status_item menu];
    if (menu)
    {
      objc_setAssociatedObject(menu, TRAYMENU_TRAY_IHANDLE_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
      [status_item setMenu:nil];
      [menu release];
    }
#endif

    id eventMonitor = objc_getAssociatedObject(status_item, TRAY_EVENT_MONITOR_KEY);
    if (eventMonitor)
    {
      [NSEvent removeMonitor:eventMonitor];
      objc_setAssociatedObject(status_item, TRAY_EVENT_MONITOR_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }

    objc_setAssociatedObject(status_item, TRAYCLICK_DELEGATE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

    [[NSStatusBar systemStatusBar] removeStatusItem:status_item];
    [status_item release];
    iupAttribSet(ih, "_IUPCOCOA_STATUSITEM", NULL);
  }
}

int iupdrvTrayIsAvailable(void)
{
  return 1;
}

void iupdrvTrayInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPBALLOON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLEICON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPDELAY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}

#ifndef IUPCOCOA_TRAY_USE_EXTERNAL_IMAGE
int iupdrvGetIconPixels(Ihandle* ih, const char* value, int* width, int* height, unsigned char** pixels)
{
  NSImage* image;
  NSBitmapImageRep* rep;
  NSSize size;
  int w, h, x, y;
  unsigned char* srcData;
  unsigned char* dstData;

  (void)ih;

  if (!value)
    return 0;

  image = (NSImage*)iupImageGetIcon(value);
  if (!image)
    return 0;

  size = [image size];
  w = (int)size.width;
  h = (int)size.height;

  if (w <= 0 || h <= 0)
    return 0;

  rep = [[NSBitmapImageRep alloc]
      initWithBitmapDataPlanes:NULL
                    pixelsWide:w
                    pixelsHigh:h
                 bitsPerSample:8
               samplesPerPixel:4
                      hasAlpha:YES
                      isPlanar:NO
                colorSpaceName:NSDeviceRGBColorSpace
                   bytesPerRow:w * 4
                  bitsPerPixel:32];

  if (!rep)
    return 0;

  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext setCurrentContext:
      [NSGraphicsContext graphicsContextWithBitmapImageRep:rep]];
  [image drawInRect:NSMakeRect(0, 0, w, h)
           fromRect:NSZeroRect
          operation:NSCompositingOperationCopy
           fraction:1.0];
  [NSGraphicsContext restoreGraphicsState];

  srcData = [rep bitmapData];
  dstData = (unsigned char*)malloc(w * h * 4);
  if (!dstData)
  {
    [rep release];
    return 0;
  }

  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
    {
      int srcOffset = (y * w + x) * 4;
      int dstOffset = (y * w + x) * 4;
      unsigned char r = srcData[srcOffset + 0];
      unsigned char g = srcData[srcOffset + 1];
      unsigned char b = srcData[srcOffset + 2];
      unsigned char a = srcData[srcOffset + 3];

      dstData[dstOffset + 0] = a;
      dstData[dstOffset + 1] = r;
      dstData[dstOffset + 2] = g;
      dstData[dstOffset + 3] = b;
    }
  }

  [rep release];

  *width = w;
  *height = h;
  *pixels = dstData;

  return 1;
}
#endif
