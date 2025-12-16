/** \file
 * \brief Cocoa System Tray Driver
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

#include "iupcocoa_drv.h"

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

int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu)
{
  NSStatusItem* status_item = cocoaGetStatusItem(ih, 1);
  if (!status_item) return 0;

  if (menu && iupObjectCheck(menu))
  {
    if (!menu->handle)
    {
      if (IupMap(menu) == IUP_ERROR)
        return 0;
    }

    NSMenu* ns_menu = (NSMenu*)menu->handle;
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
