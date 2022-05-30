/** \file
 * \brief MAC Driver iupdrvSetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>

#include <Cocoa/Cocoa.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_strmessage.h"

#include "iupcocoa_drv.h"


int iupmac_utf8autoconvert = 1;


static void macGlobalSendKey(int key, int press)
{
#if 0
  Ihandle* focus;
 // original code used int for guint. anging to NSUInteger
 //int keyval, state;
 NSUInteger keyval, state;

  focus = IupGetFocus();
  if (!focus)
    return;

  iupmacKeyEncode(key, &keyval, &state);
  if (!keyval)
    return;
 
  if (press & 0x01)
  {
	CGEventRef event = CGEventCreateKeyboardEvent (NULL, (CGKeyCode)keyval, true);
	if(state & kVK_Control)
		CGEventSetFlags(event,kCGEventFlagMaskControl);
	if(state & kVK_Option)
		CGEventSetFlags(event,kCGEventFlagMaskOption);
	if(state & kVK_Command)
		CGEventSetFlags(event,kCGEventFlagMaskCommand);
	if(state & kVK_Shift)
		CGEventSetFlags(event,kCGEventFlagMaskShift);
	CGEventPost(kCGSessionEventTap, event);
	CFRelease(event2);
  }

  if (press & 0x02)
  {
	CGEventRef event = CGEventCreateKeyboardEvent (NULL, (CGKeyCode)keyval, false);
	if(state & kVK_Control)
		CGEventSetFlags(event,kCGEventFlagMaskControl);
	if(state & kVK_Option)
		CGEventSetFlags(event,kCGEventFlagMaskOption);
	if(state & kVK_Command)
		CGEventSetFlags(event,kCGEventFlagMaskCommand);
	if(state & kVK_Shift)
		CGEventSetFlags(event,kCGEventFlagMaskShift);
	CGEventPost(kCGSessionEventTap, event);
	if(state>0) {
		event2 = CGEventCreateKeyboardEvent (NULL, (CGKeyCode)state, false);
		CGEventPost(kCGSessionEventTap, event2);
		CFRelease(event2);
	}
	CFRelease(event);	
  }
#endif
}

// Note: Everything before MENU is unaudited
int iupdrvSetGlobal(const char *name, const char *value)
{
  if (iupStrEqual(name, "LANGUAGE"))
  {
    iupStrMessageUpdateLanguage(value);
    return 1;
  }
  if (iupStrEqual(name, "CURSORPOS"))
  {
    int x, y;
    if (iupStrToIntInt(value, &x, &y, 'x') == 2) {
	  CGEventRef event=CGEventCreateMouseEvent(NULL,kCGEventMouseMoved,CGPointMake(x,y),0);
	  CGEventPost(kCGSessionEventTap, event);
	  CFRelease(event);
	}	
    return 0;
  }
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    if (!value || iupStrBoolean(value))
      iupmac_utf8autoconvert = 1;
    else
      iupmac_utf8autoconvert = 0;
    return 0;
  }
  if (iupStrEqual(name, "KEYPRESS"))
  {
    int key;
    if (iupStrToInt(value, &key))
      macGlobalSendKey(key, 0x01);
    return 0;
  }
  if (iupStrEqual(name, "KEYRELEASE"))
  {
    int key;
    if (iupStrToInt(value, &key))
      macGlobalSendKey(key, 0x02);
    return 0;
  }
  if (iupStrEqual(name, "KEY"))
  {
    int key;
    if (iupStrToInt(value, &key))
      macGlobalSendKey(key, 0x03);
    return 0;
  }
	

	// Allows the user to set the global application menu
	if (iupStrEqual(name, "MENU"))
	{
		// We expect the value to be an Ihandle for an IupMenu
		Ihandle* ih = (Ihandle*)value;
		iupCocoaMenuSetApplicationMenu(ih);

		return 1;
	}
	
	if (iupStrEqual(name, "ACTIVATIONPOLICY"))
	{
		if(iupStrEqual(value, "REGULAR"))
		{
			[[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
		}
		else if(iupStrEqual(value, "ACCESSORY"))
		{
			[[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyAccessory];
		}
		else if(iupStrEqual(value, "PROHIBITED"))
		{
			[[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyProhibited];
		}
	}
	
  return 1;
}

char *iupdrvGetGlobal(const char *name)
{
  if (iupStrEqual(name, "CURSORPOS"))
  {
    char *str = iupStrGetMemory(50);
    int x, y;
    iupdrvGetCursorPos(&x, &y);
    sprintf(str, "%dx%d", (int)x, (int)y);
    return str;
  }
  if (iupStrEqual(name, "SHIFTKEY"))
  {
    char key[5];
    iupdrvGetKeyState(key);
    if (key[0] == 'S')
      return "ON";
    else
      return "OFF";
  }
  if (iupStrEqual(name, "CONTROLKEY"))
  {
    char key[5];
    iupdrvGetKeyState(key);
    if (key[1] == 'C')
      return "ON";
    else
      return "OFF";
  }
  if (iupStrEqual(name, "MODKEYSTATE"))
  {
    char *str = iupStrGetMemory(5);
    iupdrvGetKeyState(str);
    return str;
  }
  if (iupStrEqual(name, "SCREENSIZE"))
  {
    char *str = iupStrGetMemory(50);
    int w, h;
    iupdrvGetScreenSize(&w, &h);
    sprintf(str, "%dx%d", w, h);
    return str;
  }
  if (iupStrEqual(name, "FULLSIZE"))
  {
    char *str = iupStrGetMemory(50);
    int w, h;
    iupdrvGetFullSize(&w, &h);
    sprintf(str, "%dx%d", w, h);
    return str;
  }
  if (iupStrEqual(name, "SCREENDEPTH"))
  {
    char *str = iupStrGetMemory(50);
    int bpp = iupdrvGetScreenDepth();
    sprintf(str, "%d", bpp);
    return str;
  }
  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    char *str = iupStrGetMemory(50);
	int x=0,y=0,w,h;
    iupdrvGetFullSize(&w, &h);
    sprintf(str, "%d %d %d %d", x, y, w, h);
    return str;
  }
  if (iupStrEqual(name, "MONITORSINFO"))
  {
    int i;
	NSArray* arr = [NSScreen screens];
	int monitors_count = [arr count];
    char *str = iupStrGetMemory(monitors_count*50);
    char* pstr = str;
	NSRect frame;

	  for(NSScreen* screen in arr)
	  {
      frame = [screen frame];
      pstr += sprintf(pstr, "%d %d %d %d\n", frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
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
  if (iupStrEqual(name, "UTF8AUTOCONVERT"))
  {
    if (iupmac_utf8autoconvert)
      return "YES";
    else
      return "NO";
  }
  if (iupStrEqual(name, "MENU"))
  {
	  return (char*)iupCocoaMenuGetApplicationMenu();
  }
	if (iupStrEqual(name, "ACTIVATIONPOLICY"))
	{
		NSApplicationActivationPolicy activation_policy = [[NSApplication sharedApplication] activationPolicy];
		switch(activation_policy)
		{
			case NSApplicationActivationPolicyRegular:
			{
				return iupStrReturnStr("REGULAR");
			}
			case NSApplicationActivationPolicyAccessory:
			{
				return iupStrReturnStr("ACCESSORY");
			}
			case NSApplicationActivationPolicyProhibited:
			{
				return iupStrReturnStr("PROHIBITED");
			}
			default:
			{
				return NULL;
			}
		}
	}

	
  return NULL;
}
