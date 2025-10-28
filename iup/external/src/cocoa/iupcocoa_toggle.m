/** \file
 * \brief Toggle Control
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
#include "iup_toggle.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"

#include "iupcocoa_drv.h"
#include "iupcocoa_keycodes.h"


static const void* IUP_COCOA_TOGGLE_RECEIVER_OBJ_KEY = "IUP_COCOA_TOGGLE_RECEIVER_OBJ_KEY";

@interface IupCocoaToggleButton : NSButton
@end

@implementation IupCocoaToggleButton

- (BOOL)acceptsFirstResponder
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih)
    return [super acceptsFirstResponder];

  const char* canfocus = iupAttribGet(ih, "_IUPCOCOA_CANFOCUS");
  if (canfocus && iupStrEqualNoCase(canfocus, "NO"))
    return NO;

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
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
    iupcocoaFocusOut(ih);

  return [super resignFirstResponder];
}

- (BOOL) needsPanelToBecomeKey
{
  return YES;
}

- (void)mouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);

  if (ih && !ih->data->is_radio && ih->data->type == IUP_TOGGLE_TEXT && [self allowsMixedState])
  {
    int current = iupAttribGetInt(ih, "_IUPCOCOA_3STATE_CURRENT");

    if (current == 1)
    {
      [self setState:NSMixedState];
      iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", -1);
    }
    else if (current == -1)
    {
      [self setState:NSOffState];
      iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", 0);
    }
    else
    {
      [self setState:NSOnState];
      iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", 1);
    }

    [self sendAction:[self action] to:[self target]];
    return;
  }

  [super mouseDown:event];
}

- (void)keyDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih)
  {
    [super keyDown:event];
    return;
  }

  int keyCode = [event keyCode];
  if (iupcocoaKeyEvent(ih, event, keyCode, true))
    return;

  if (!ih->data->is_radio && ih->data->type == IUP_TOGGLE_TEXT && [self allowsMixedState])
  {
    if (keyCode == kVK_Space || keyCode == kVK_Return || keyCode == kVK_ANSI_KeypadEnter)
    {
      int current = iupAttribGetInt(ih, "_IUPCOCOA_3STATE_CURRENT");

      if (current == 1)
      {
        [self setState:NSMixedState];
        iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", -1);
      }
      else if (current == -1)
      {
        [self setState:NSOffState];
        iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", 0);
      }
      else
      {
        [self setState:NSOnState];
        iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", 1);
      }

      [self sendAction:[self action] to:[self target]];
      return;
    }
  }

  [super keyDown:event];
}


@end


@interface IupCocoaToggleReceiver : NSObject
  - (IBAction) myToggleClickAction:(id)the_sender;
@end

@implementation IupCocoaToggleReceiver

- (IBAction) myToggleClickAction:(id)the_sender;
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);
  Ihandle* radio = iupRadioFindToggleParent(ih);
  NSControlStateValue new_state = [the_sender state];

  if (iupAttribGetBoolean(ih, "IGNOREDOUBLECLICK"))
  {
    NSEvent* current_event = [NSApp currentEvent];
    if (current_event && [current_event clickCount] > 1)
    {
      return;
    }
  }

  if ([the_sender isKindOfClass:[NSSwitch class]])
  {
    /* NSSwitch is simple, no 3-state, no radio, no image */
  }
  else if (!radio && ih->data->type == IUP_TOGGLE_TEXT && [the_sender allowsMixedState])
  {
    int state_val = (new_state == NSOnState) ? 1 : (new_state == NSMixedState) ? -1 : 0;
    iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", state_val);
  }
  else if (radio)
  {
    if (new_state == NSOnState)
    {
      Ihandle* last_tg = (Ihandle*)iupAttribGet(radio, "_IUPCOCOA_LASTTOGGLE");
      if (iupObjectCheck(last_tg) && last_tg != ih)
      {
        NSButton* last_button = (NSButton*)last_tg->handle;
        [last_button setState:NSOffState];

        if (last_tg->data->type == IUP_TOGGLE_IMAGE)
        {
          char* name = iupAttribGet(last_tg, "IMAGE");
          if (name)
          {
            int make_inactive = !iupdrvIsActive(last_tg);
            NSImage* the_bitmap = iupImageGetImage(name, last_tg, make_inactive, NULL);
            [last_button setImage:the_bitmap];
          }
        }

        IFni action_cb = (IFni)IupGetCallback(last_tg, "ACTION");
        if (action_cb)
        {
          if (action_cb(last_tg, 0) == IUP_CLOSE)
          {
            IupExitLoop();
            return;
          }
        }

        if (iupObjectCheck(last_tg))
        {
          iupBaseCallValueChangedCb(last_tg);
        }
      }

      iupAttribSet(radio, "_IUPCOCOA_LASTTOGGLE", (char*)ih);

      if (ih->data->type == IUP_TOGGLE_IMAGE)
      {
        char* name = iupAttribGet(ih, "IMPRESS");
        if (name)
        {
          int make_inactive = !iupdrvIsActive(ih);
          NSImage* the_bitmap = iupImageGetImage(name, ih, make_inactive, NULL);
          [the_sender setImage:the_bitmap];
        }
      }
    }
  }
  else
  {
    if (ih->data->type == IUP_TOGGLE_IMAGE)
    {
      char* name = NULL;
      int is_checked = (new_state == NSOnState) ? 1 : 0;

      if (is_checked)
      {
        name = iupAttribGet(ih, "IMPRESS");
        if (!name)
          name = iupAttribGet(ih, "IMAGE");
      }
      else
      {
        name = iupAttribGet(ih, "IMAGE");
      }

      if (name)
      {
        int make_inactive = !iupdrvIsActive(ih);
        NSImage* the_bitmap = iupImageGetImage(name, ih, make_inactive, NULL);
        [the_sender setImage:the_bitmap];
      }
    }
  }

  IFni action_callback_function = (IFni)IupGetCallback(ih, "ACTION");
  if (action_callback_function)
  {
    int state = (int)new_state;
    if (new_state == NSMixedState)
      state = -1;

    if (action_callback_function(ih, state) == IUP_CLOSE)
    {
      IupExitLoop();
    }
  }

  if (iupObjectCheck(ih))
  {
    iupBaseCallValueChangedCb(ih);
  }
}


@end


void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (!ih->data->flat)
    {
      *x += 8;
      *y += 8;
    }
    else
    {
      *x += 4;
      *y += 4;
    }
  }
}

void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* str)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    /* NSSwitch dimensions estimate (macOS 11+) */
    int switch_w = 38;
    int switch_h = 20;

    *x += 2 + switch_w + 2;
    if ((*y) < 2 + switch_h + 2) *y = 2 + switch_h + 2;
    else *y += 2+2;

    if (str && str[0]) /* add spacing between switch and text */
      *x += 8;
  }
  else
  {
    *x += 20; /* checkbox width estimate */
    *y = (*y < 20) ? 20 : *y; /* checkbox height estimate */

    *x += 4; /* margins */
    *y += 4;

    if (str && *str != 0) /* add spacing between check box and text */
    {
      *x += 8;
    }
  }
}


static void cocoaToggleUpdateImageSize(Ihandle* ih)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    NSButton* the_toggle = ih->handle;
    NSImage* image = [the_toggle image];

    if (image)
    {
      NSSize imageSize = [image size];

      int width = (int)imageSize.width;
      int height = (int)imageSize.height;

      width += 2 * ih->data->horiz_padding;
      height += 2 * ih->data->vert_padding;

      if (!ih->data->flat)
      {
        width += 8;
        height += 8;
      }

      NSRect frame = [the_toggle frame];
      frame.size.width = width;
      frame.size.height = height;
      [the_toggle setFrame:frame];
    }
  }
}

static void cocoaToggleUpdateImage(Ihandle* ih, int active, int check)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    NSButton* the_toggle = ih->handle;
    char* name = NULL;

    if (!active)
    {
      name = iupAttribGet(ih, "IMINACTIVE");
      if (!name)
      {
        if (check)
        {
          name = iupAttribGet(ih, "IMPRESS");
          if (!name)
            name = iupAttribGet(ih, "IMAGE");
        }
        else
          name = iupAttribGet(ih, "IMAGE");
      }
    }
    else
    {
      if (check)
      {
        name = iupAttribGet(ih, "IMPRESS");
        if (!name)
          name = iupAttribGet(ih, "IMAGE");
      }
      else
      {
        name = iupAttribGet(ih, "IMAGE");
      }
    }

    if (name)
    {
      int make_inactive = !active;
      NSImage* the_bitmap = iupImageGetImage(name, ih, make_inactive, NULL);
      [the_toggle setImage:the_bitmap];
      cocoaToggleUpdateImageSize(ih);
    }
    else
    {
      [the_toggle setImage:nil];
    }
  }
}

static int cocoaToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  id the_toggle = ih->handle;

  if ([the_toggle isKindOfClass:[NSSwitch class]])
  {
    /* NSSwitch does not have a title */
    return 0;
  }

  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    char* stripped_str = iupStrProcessMnemonic(value, NULL, 0);

    if (stripped_str && *stripped_str != 0)
    {
      NSString* ns_string = [NSString stringWithUTF8String:stripped_str];
      [the_toggle setTitle:ns_string];
    }
    else
    {
      [the_toggle setTitle:@""];
    }

    if (stripped_str && stripped_str != value)
    {
      free(stripped_str);
    }

    return 1;
  }

  return 0;
}

static int cocoaToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  id the_toggle = ih->handle;
  Ihandle* radio = iupRadioFindToggleParent(ih);

  if ([the_toggle isKindOfClass:[NSSwitch class]])
  {
    if (iupStrEqualNoCase(value, "NOTDEF"))
    {
      /* NSSwitch does not support mixed state */
      return 0;
    }
    else if (iupStrEqualNoCase(value, "TOGGLE"))
    {
      NSControlStateValue current_state = [the_toggle state];
      NSControlStateValue new_state = (current_state == NSOffState) ? NSOnState : NSOffState;
      [the_toggle setState:new_state];
    }
    else
    {
      int new_state = iupStrBoolean(value);
      [the_toggle setState:new_state ? NSOnState : NSOffState];
    }
  }
  else
  {
    /* Existing NSButton logic */
    if (iupStrEqualNoCase(value, "NOTDEF"))
    {
      [the_toggle setAllowsMixedState:YES];
      [the_toggle setState:NSMixedState];
      if (!radio && ih->data->type == IUP_TOGGLE_TEXT)
        iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", -1);
    }
    else if (iupStrEqualNoCase(value, "TOGGLE"))
    {
      if (!radio && ih->data->type == IUP_TOGGLE_TEXT && [the_toggle allowsMixedState])
      {
        int current = iupAttribGetInt(ih, "_IUPCOCOA_3STATE_CURRENT");

        if (current == 1)
        {
          [the_toggle setState:NSMixedState];
          iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", -1);
        }
        else if (current == -1)
        {
          [the_toggle setState:NSOffState];
          iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", 0);
        }
        else
        {
          [the_toggle setState:NSOnState];
          iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", 1);
        }
      }
      else
      {
        NSControlStateValue current_state = [the_toggle state];
        NSControlStateValue new_state = (current_state == NSOffState) ? NSOnState : NSOffState;

        if (radio && new_state == NSOnState)
        {
          Ihandle* last_tg = (Ihandle*)iupAttribGet(radio, "_IUPCOCOA_LASTTOGGLE");
          if (iupObjectCheck(last_tg) && last_tg != ih)
          {
            NSButton* last_button = (NSButton*)last_tg->handle;
            [last_button setState:NSOffState];

            if (last_tg->data->type == IUP_TOGGLE_IMAGE)
            {
              cocoaToggleUpdateImage(last_tg, iupdrvIsActive(last_tg), 0);
            }
          }
          iupAttribSet(radio, "_IUPCOCOA_LASTTOGGLE", (char*)ih);
        }

        [the_toggle setState:new_state];

        if (ih->data->type == IUP_TOGGLE_IMAGE)
        {
          cocoaToggleUpdateImage(ih, iupdrvIsActive(ih), new_state == NSOnState);
        }

        if (!radio && ih->data->type == IUP_TOGGLE_TEXT && [the_toggle allowsMixedState])
        {
          int state_val = (new_state == NSOnState) ? 1 : 0;
          iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", state_val);
        }
      }
    }
    else
    {
      int new_state = iupStrBoolean(value);

      if (radio)
      {
        if (new_state)
        {
          Ihandle* last_tg = (Ihandle*)iupAttribGet(radio, "_IUPCOCOA_LASTTOGGLE");
          if (iupObjectCheck(last_tg) && last_tg != ih)
          {
            NSButton* last_button = (NSButton*)last_tg->handle;
            [last_button setState:NSOffState];

            if (last_tg->data->type == IUP_TOGGLE_IMAGE)
            {
              cocoaToggleUpdateImage(last_tg, iupdrvIsActive(last_tg), 0);
            }
          }
          iupAttribSet(radio, "_IUPCOCOA_LASTTOGGLE", (char*)ih);
          [the_toggle setState:NSOnState];
        }
        else
        {
          if (ih == (Ihandle*)iupAttribGet(radio, "_IUPCOCOA_LASTTOGGLE"))
            return 0;
        }
      }
      else
      {
        [the_toggle setState:new_state ? NSOnState : NSOffState];

        if (ih->data->type == IUP_TOGGLE_TEXT && [the_toggle allowsMixedState])
        {
          iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", new_state ? 1 : 0);
        }
      }

      if (ih->data->type == IUP_TOGGLE_IMAGE)
      {
        cocoaToggleUpdateImage(ih, iupdrvIsActive(ih), new_state);
      }
    }
  }

  return 0;
}

static char* cocoaToggleGetValueAttrib(Ihandle* ih)
{
  id the_toggle = ih->handle;
  NSControlStateValue current_state = [the_toggle state];

  if ([the_toggle isKindOfClass:[NSSwitch class]])
  {
    if (current_state == NSOnState)
      return iupStrReturnChecked(1);
    else
      return iupStrReturnChecked(0);
  }
  else
  {
    if (current_state == NSMixedState)
      return iupStrReturnChecked(-1);
    else if (current_state == NSOnState)
      return iupStrReturnChecked(1);
    else
      return iupStrReturnChecked(0);
  }
}

static int cocoaToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    NSButton* the_toggle = ih->handle;

    if (value != iupAttribGet(ih, "IMAGE"))
      iupAttribSet(ih, "IMAGE", (char*)value);

    cocoaToggleUpdateImage(ih, iupdrvIsActive(ih), [the_toggle state] == NSOnState);
    return 1;
  }

  return 0;
}

static int cocoaToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMINACTIVE"))
      iupAttribSet(ih, "IMINACTIVE", (char*)value);

    if (!iupdrvIsActive(ih))
    {
      NSButton* the_toggle = ih->handle;
      cocoaToggleUpdateImage(ih, 0, [the_toggle state] == NSOnState);
    }

    return 1;
  }

  return 0;
}

static int cocoaToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    NSButton* the_toggle = ih->handle;

    if (value != iupAttribGet(ih, "IMPRESS"))
      iupAttribSet(ih, "IMPRESS", (char*)value);

    if ([the_toggle state] == NSOnState)
    {
      cocoaToggleUpdateImage(ih, iupdrvIsActive(ih), 1);
    }

    return 1;
  }

  return 0;
}

static int cocoaToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    NSButton* the_toggle = ih->handle;
    int active = iupStrBoolean(value);
    int check = ([the_toggle state] == NSOnState) ? 1 : 0;
    cocoaToggleUpdateImage(ih, active, check);
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int cocoaToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupAttribSet(ih, "ALIGNMENT", (char*)value);
    return 1;
  }

  return 0;
}

static int cocoaToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    NSButton* the_toggle = ih->handle;
    [[the_toggle cell] setImageScaling:NSImageScaleProportionallyDown];
    cocoaToggleUpdateImageSize(ih);
  }

  return 1;
}

static int cocoaToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  id the_toggle = ih->handle;
  if ([the_toggle isKindOfClass:[NSSwitch class]])
  {
    /* NSSwitch does not have text */
    return 0;
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];

    NSMutableAttributedString* attrTitle = [[NSMutableAttributedString alloc]
      initWithString:[the_toggle title]];
    NSRange range = NSMakeRange(0, [attrTitle length]);
    [attrTitle addAttribute:NSForegroundColorAttributeName value:color range:range];
    [the_toggle setAttributedTitle:attrTitle];
    [attrTitle release];
  }

  return 1;
}

static int cocoaToggleSetFlatAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    NSButton* the_toggle = ih->handle;
    ih->data->flat = iupStrBoolean(value);

    if (ih->data->flat)
    {
      [the_toggle setBordered:NO];
      [[the_toggle cell] setImageScaling:NSImageScaleProportionallyDown];
    }
    else
    {
      [the_toggle setBordered:YES];
      if (@available(macOS 10.14, *))
      {
        [the_toggle setBezelStyle:NSBezelStyleRegularSquare];
      }
      else
      {
        [the_toggle setBezelStyle:NSShadowlessSquareBezelStyle];
      }
    }

    cocoaToggleUpdateImageSize(ih);
    return 1;
  }

  return 0;
}

static int cocoaToggleMapMethod(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  char* value;
  id the_toggle; /* Use id to hold either NSButton or NSSwitch */
  NSRect initialFrame = NSMakeRect(0, 0, 0, 0);
  int initial_checked = 0;
  int is_switch = 0;

  iupAttribSet(ih, "_IUPCOCOA_ACTIVE", "YES");

  value = iupAttribGet(ih, "IMAGE");
  if (value && *value != 0)
  {
    ih->data->type = IUP_TOGGLE_IMAGE;
  }
  else
  {
    ih->data->type = IUP_TOGGLE_TEXT;
    if (iupAttribGetBoolean(ih, "SWITCH"))
    {
      is_switch = 1;
    }
  }

  if (is_switch)
  {
    the_toggle = [[NSSwitch alloc] initWithFrame:initialFrame];
    /* NSSwitch is only available on 10.10+ but that should be fine */
  }
  else
  {
    the_toggle = [[IupCocoaToggleButton alloc] initWithFrame:initialFrame];

    if (ih->data->type == IUP_TOGGLE_IMAGE)
    {
      [the_toggle setButtonType:NSPushOnPushOffButton];

      if (ih->data->flat)
      {
        [the_toggle setBordered:NO];
        [[the_toggle cell] setImageScaling:NSImageScaleProportionallyDown];
      }
      else
      {
        if (@available(macOS 10.14, *))
        {
          [the_toggle setBezelStyle:NSBezelStyleRegularSquare];
        }
        else
        {
          [the_toggle setBezelStyle:NSShadowlessSquareBezelStyle];
        }
      }

      [[the_toggle cell] setImagePosition:NSImageOnly];
      [the_toggle setTitle:@""];
    }
    else /* IUP_TOGGLE_TEXT and not a switch */
    {
      if (radio)
      {
        [the_toggle setButtonType:NSRadioButton];
        ih->data->is_radio = 1;

        if (!iupAttribGet(radio, "_IUPCOCOA_LASTTOGGLE"))
        {
          iupAttribSet(ih, "VALUE", "ON");
          initial_checked = 1;
        }

        if (!iupAttribGetHandleName(ih))
          iupAttribSetHandleName(ih);
      }
      else
      {
        [the_toggle setButtonType:NSSwitchButton]; /* This is the checkbox style */

        if (iupAttribGetBoolean(ih, "3STATE"))
        {
          [the_toggle setAllowsMixedState:YES];
        }
      }

      value = iupAttribGet(ih, "TITLE");
      if (value && *value != 0)
      {
        char* stripped_str = iupStrProcessMnemonic(value, NULL, 0);
        NSString* ns_string = [NSString stringWithUTF8String:stripped_str];

        if (stripped_str && stripped_str != value)
          free(stripped_str);

        [the_toggle setTitle:ns_string];
      }
    }
  }

  ih->handle = the_toggle;

  objc_setAssociatedObject(the_toggle, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

  IupCocoaToggleReceiver* toggle_receiver = [[IupCocoaToggleReceiver alloc] init];
  [the_toggle setTarget:toggle_receiver];
  [the_toggle setAction:@selector(myToggleClickAction:)];
  objc_setAssociatedObject(the_toggle, IUP_COCOA_TOGGLE_RECEIVER_OBJ_KEY, (id)toggle_receiver, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [toggle_receiver release];

  iupcocoaSetAssociatedViews(ih, the_toggle, the_toggle);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupcocoaSetCanFocus(ih, 0);
  else
    iupcocoaSetCanFocus(ih, 1);

  iupcocoaAddToParent(ih);

  value = iupAttribGet(ih, "VALUE");
  if (!value && initial_checked)
    value = "ON";

  if (value)
  {
    cocoaToggleSetValueAttrib(ih, value);
  }
  else
  {
    if (ih->data->type == IUP_TOGGLE_IMAGE)
    {
      cocoaToggleUpdateImage(ih, iupdrvIsActive(ih), 0);
    }
    else if (ih->data->type == IUP_TOGGLE_TEXT && !is_switch && [the_toggle allowsMixedState])
    {
      iupAttribSetInt(ih, "_IUPCOCOA_3STATE_CURRENT", 0);
    }
  }

  return IUP_NOERROR;
}

static void cocoaToggleUnMapMethod(Ihandle* ih)
{
  id the_toggle = ih->handle;

  Ihandle* context_menu_ih = (Ihandle*)iupcocoaCommonBaseGetContextMenuAttrib(ih);
  if (context_menu_ih != NULL)
  {
    IupDestroy(context_menu_ih);
    iupcocoaCommonBaseSetContextMenuAttrib(ih, NULL);
  }

  id button_receiver = objc_getAssociatedObject(the_toggle, IUP_COCOA_TOGGLE_RECEIVER_OBJ_KEY);
  objc_setAssociatedObject(the_toggle, IUP_COCOA_TOGGLE_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

  Ihandle* radio = iupRadioFindToggleParent(ih);
  if (radio)
  {
    Ihandle* last_tg = (Ihandle*)iupAttribGet(radio, "_IUPCOCOA_LASTTOGGLE");
    if (last_tg == ih)
      iupAttribSet(radio, "_IUPCOCOA_LASTTOGGLE", NULL);
  }

  iupcocoaRemoveFromParent(ih);
  iupcocoaSetAssociatedViews(ih, nil, nil);
  [the_toggle release];
  ih->handle = NULL;
}


void iupdrvToggleInitClass(Iclass* ic)
{
  ic->Map = cocoaToggleMapMethod;
  ic->UnMap = cocoaToggleUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaToggleSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", cocoaToggleGetValueAttrib, cocoaToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, cocoaToggleSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, cocoaToggleSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, cocoaToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, cocoaToggleSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, cocoaToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLAT", NULL, cocoaToggleSetFlatAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "SWITCH", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);

  iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupcocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);
}
