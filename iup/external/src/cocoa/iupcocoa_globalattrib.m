/** \file
 * \brief MAC Driver iupdrvSetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_key.h"
#include "iup_strmessage.h"

#include "iupcocoa_drv.h"
#include "iupcocoa_keycodes.h"


int utf8autoconvert = 1;
static CFMachPortRef eventTap = NULL;
static CFRunLoopSourceRef runLoopSource = NULL;

static NSRect iupCocoaGetVirtualScreenRect(void)
{
  NSArray* screens = [NSScreen screens];
  if ([screens count] == 0)
    return NSZeroRect;

  NSRect virtualRect = [[screens objectAtIndex:0] frame];

  for (NSScreen* screen in screens)
  {
    NSRect screenFrame = [screen frame];
    virtualRect = NSUnionRect(virtualRect, screenFrame);
  }

  return virtualRect;
}

static CGEventRef iupCocoaGlobalEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
  NSRect main_screen_frame = [[NSScreen mainScreen] frame];
  CGFloat main_screen_top = main_screen_frame.origin.y + main_screen_frame.size.height;

  /* Handle event tap being disabled by the system due to timeout */
  if (type == kCGEventTapDisabledByTimeout)
  {
    if (eventTap)
      CGEventTapEnable(eventTap, true);
    return event;
  }

  switch (type)
  {
    case kCGEventLeftMouseDown:
    case kCGEventRightMouseDown:
    case kCGEventOtherMouseDown:
    {
      IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
      if (cb)
      {
        CGPoint location = CGEventGetLocation(event);
        CGEventFlags flags = CGEventGetFlags(event);
        int64_t button_number = CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
        int64_t click_count = CGEventGetIntegerValueField(event, kCGMouseEventClickState);
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

        if (flags & kCGEventFlagMaskShift)
          iupKEY_SETSHIFT(status);
        if (flags & kCGEventFlagMaskControl)
          iupKEY_SETCONTROL(status);
        if (flags & kCGEventFlagMaskAlternate)
          iupKEY_SETALT(status);
        if (flags & kCGEventFlagMaskCommand)
          iupKEY_SETSYS(status);

        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonLeft))
          iupKEY_SETBUTTON1(status);
        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonRight))
          iupKEY_SETBUTTON3(status);
        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonCenter))
          iupKEY_SETBUTTON2(status);

        if (click_count == 2)
          iupKEY_SETDOUBLE(status);

        int iup_button = (button_number == 0) ? IUP_BUTTON1 :
                         (button_number == 1) ? IUP_BUTTON3 :
                         (button_number == 2) ? IUP_BUTTON2 :
                         IUP_BUTTON1 + (int)button_number;

        int iup_y = (int)(main_screen_top - location.y);
        cb(iup_button, 1, (int)location.x, iup_y, status);
      }
      break;
    }
    case kCGEventLeftMouseUp:
    case kCGEventRightMouseUp:
    case kCGEventOtherMouseUp:
    {
      IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
      if (cb)
      {
        CGPoint location = CGEventGetLocation(event);
        CGEventFlags flags = CGEventGetFlags(event);
        int64_t button_number = CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

        if (flags & kCGEventFlagMaskShift)
          iupKEY_SETSHIFT(status);
        if (flags & kCGEventFlagMaskControl)
          iupKEY_SETCONTROL(status);
        if (flags & kCGEventFlagMaskAlternate)
          iupKEY_SETALT(status);
        if (flags & kCGEventFlagMaskCommand)
          iupKEY_SETSYS(status);

        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonLeft))
          iupKEY_SETBUTTON1(status);
        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonRight))
          iupKEY_SETBUTTON3(status);
        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonCenter))
          iupKEY_SETBUTTON2(status);

        int iup_button = (button_number == 0) ? IUP_BUTTON1 :
                         (button_number == 1) ? IUP_BUTTON3 :
                         (button_number == 2) ? IUP_BUTTON2 :
                         IUP_BUTTON1 + (int)button_number;

        int iup_y = (int)(main_screen_top - location.y);
        cb(iup_button, 0, (int)location.x, iup_y, status);
      }
      break;
    }
    case kCGEventMouseMoved:
    case kCGEventLeftMouseDragged:
    case kCGEventRightMouseDragged:
    case kCGEventOtherMouseDragged:
    {
      IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
      if (cb)
      {
        CGPoint location = CGEventGetLocation(event);
        CGEventFlags flags = CGEventGetFlags(event);
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

        if (flags & kCGEventFlagMaskShift)
          iupKEY_SETSHIFT(status);
        if (flags & kCGEventFlagMaskControl)
          iupKEY_SETCONTROL(status);
        if (flags & kCGEventFlagMaskAlternate)
          iupKEY_SETALT(status);
        if (flags & kCGEventFlagMaskCommand)
          iupKEY_SETSYS(status);

        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonLeft))
          iupKEY_SETBUTTON1(status);
        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonRight))
          iupKEY_SETBUTTON3(status);
        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonCenter))
          iupKEY_SETBUTTON2(status);

        int iup_y = (int)(main_screen_top - location.y);
        cb((int)location.x, iup_y, status);
      }
      break;
    }
    case kCGEventScrollWheel:
    {
      IFfiis cb = (IFfiis)IupGetFunction("GLOBALWHEEL_CB");
      if (cb)
      {
        CGPoint location = CGEventGetLocation(event);
        int64_t delta = CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1);
        CGEventFlags flags = CGEventGetFlags(event);
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

        if (flags & kCGEventFlagMaskShift)
          iupKEY_SETSHIFT(status);
        if (flags & kCGEventFlagMaskControl)
          iupKEY_SETCONTROL(status);
        if (flags & kCGEventFlagMaskAlternate)
          iupKEY_SETALT(status);
        if (flags & kCGEventFlagMaskCommand)
          iupKEY_SETSYS(status);

        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonLeft))
          iupKEY_SETBUTTON1(status);
        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonRight))
          iupKEY_SETBUTTON3(status);
        if (CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonCenter))
          iupKEY_SETBUTTON2(status);

        int iup_y = (int)(main_screen_top - location.y);
        cb((float)(-delta), (int)location.x, iup_y, status);
      }
      break;
    }
    case kCGEventKeyDown:
    case kCGEventKeyUp:
    {
      IFii cb = (IFii)IupGetFunction("GLOBALKEYPRESS_CB");
      if (cb)
      {
        int pressed = (type == kCGEventKeyDown) ? 1 : 0;
        int code = iupcocoaKeyDecode(event);
        if (code != 0)
          cb(code, pressed);
      }
      break;
    }
    default:
      break;
  }

  (void)proxy;
  (void)refcon;
  return event;
}

int iupdrvSetGlobal(const char *name, const char *value)
{
  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    if (iupStrBoolean(value))
    {
      if (!eventTap)
      {
        CGEventMask mask = (1 << kCGEventMouseMoved)    | (1 << kCGEventLeftMouseDragged) |
                           (1 << kCGEventRightMouseDragged) | (1 << kCGEventOtherMouseDragged) |
                           (1 << kCGEventLeftMouseDown) | (1 << kCGEventLeftMouseUp) |
                           (1 << kCGEventRightMouseDown)  | (1 << kCGEventRightMouseUp) |
                           (1 << kCGEventOtherMouseDown)| (1 << kCGEventOtherMouseUp) |
                           (1 << kCGEventScrollWheel) |
                           (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);

        eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                                             kCGEventTapOptionDefault, mask,
                                             iupCocoaGlobalEventCallback, NULL);
        if (eventTap)
        {
          runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
          if (runLoopSource)
          {
            CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
            CGEventTapEnable(eventTap, true);
          }
          else
          {
            CFRelease(eventTap);
            eventTap = NULL;
            return 0;
          }
        }
        else
        {
          return 0;
        }
      }
    }
    else
    {
      if (eventTap)
      {
        CGEventTapEnable(eventTap, false);

        if (runLoopSource)
        {
          CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
          CFRelease(runLoopSource);
          runLoopSource = NULL;
        }

        CFRelease(eventTap);
        eventTap = NULL;
      }
    }
    return 1;
  }
  if (iupStrEqual(name, "UTF8MODE"))
  {
    if (!value || iupStrBoolean(value))
      utf8autoconvert = 0;
    else
      utf8autoconvert = 1;
    return 1;
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    if (!value || iupStrBoolean(value))
      utf8autoconvert = 1;
    else
      utf8autoconvert = 0;
    return 0;
  }
  if (iupStrEqual(name, "MENU"))
  {
    Ihandle* ih = (Ihandle*)value;
    if (ih && iupObjectCheck(ih))
    {
      if (!ih->handle)
        IupMap(ih);

      iupcocoaMenuSetApplicationMenu(ih);
    }
    else
    {
      iupcocoaMenuSetApplicationMenu(NULL);
    }
    return 1;
  }
  if (iupStrEqual(name, "ACTIVATIONPOLICY"))
  {
    NSApplicationActivationPolicy old_policy = [[NSApplication sharedApplication] activationPolicy];
    NSApplicationActivationPolicy new_policy = old_policy;

    if (iupStrEqual(value, "REGULAR"))
      new_policy = NSApplicationActivationPolicyRegular;
    else if (iupStrEqual(value, "ACCESSORY"))
      new_policy = NSApplicationActivationPolicyAccessory;
    else if (iupStrEqual(value, "PROHIBITED"))
      new_policy = NSApplicationActivationPolicyProhibited;
    else
      return 0;

    if (old_policy != new_policy)
    {
      if ([[NSApplication sharedApplication] setActivationPolicy:new_policy])
      {
        if (new_policy == NSApplicationActivationPolicyRegular ||
            new_policy == NSApplicationActivationPolicyAccessory)
        {
          [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
        }
      }
    }
    return 1;
  }

  return 1;
}

char *iupdrvGetGlobal(const char *name)
{
  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    NSRect primary_screen_rect = [[NSScreen mainScreen] frame];
    CGFloat primary_screen_top = primary_screen_rect.origin.y + primary_screen_rect.size.height;

    NSRect rect = iupCocoaGetVirtualScreenRect();
    CGFloat virtual_top = rect.origin.y + rect.size.height;

    int iup_x = (int)rect.origin.x;
    int iup_y = (int)(primary_screen_top - virtual_top);

    return iupStrReturnStrf("%d %d %d %d", iup_x, iup_y, (int)rect.size.width, (int)rect.size.height);
  }
  if (iupStrEqual(name, "MONITORSCOUNT"))
  {
    return iupStrReturnInt([[NSScreen screens] count]);
  }
  if (iupStrEqual(name, "MONITORSINFO"))
  {
    NSArray* arr = [NSScreen screens];
    NSUInteger monitors_count = [arr count];
    char *str = iupStrGetMemory(monitors_count * 50);
    char* pstr = str;

    NSRect primary_screen_rect = [[NSScreen mainScreen] frame];
    CGFloat primary_screen_top = primary_screen_rect.origin.y + primary_screen_rect.size.height;

    for (NSScreen* screen in arr)
    {
      NSRect frame = [screen frame];
      CGFloat screen_top = frame.origin.y + frame.size.height;
      int iup_x = (int)frame.origin.x;
      int iup_y = (int)(primary_screen_top - screen_top);
      pstr += sprintf(pstr, "%d %d %d %d\n", iup_x, iup_y, (int)frame.size.width, (int)frame.size.height);
    }
    return str;
  }
  if (iupStrEqual(name, "TRUECOLORCANVAS"))
  {
    if (iupdrvGetScreenDepth() > 8)
      return "YES";
    else
      return "NO";
  }
  if (iupStrEqual(name, "UTF8MODE"))
  {
    return iupStrReturnBoolean(!utf8autoconvert);
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    return iupStrReturnBoolean(utf8autoconvert);
  }
  if (iupStrEqual(name, "EXEFILENAME"))
  {
    return iupStrReturnStr([[[NSBundle mainBundle] executablePath] fileSystemRepresentation]);
  }
  if (iupStrEqual(name, "MENU"))
  {
    return (char*)iupcocoaMenuGetApplicationMenu();
  }
  if (iupStrEqual(name, "ACTIVATIONPOLICY"))
  {
    NSApplicationActivationPolicy activation_policy = [[NSApplication sharedApplication] activationPolicy];
    switch (activation_policy)
    {
      case NSApplicationActivationPolicyRegular:
        return "REGULAR";
      case NSApplicationActivationPolicyAccessory:
        return "ACCESSORY";
      case NSApplicationActivationPolicyProhibited:
        return "PROHIBITED";
      default:
        return NULL;
    }
  }
  if (iupStrEqual(name, "DARKMODE"))
  {
    return iupStrReturnBoolean(iupcocoaIsSystemDarkMode());
  }

  return NULL;
}
