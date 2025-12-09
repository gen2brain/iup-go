/** \file
 * \brief macOS Driver keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"

#include "iupcocoa_drv.h"
#include "iupcocoa_keycodes.h"


typedef struct _Imac2iupkey
{
  int maccode; /* The native virtual keycode, from iupcocoa_keycodes.h */
  int iupcode; /* The IUP equivalent base keycode */
} Imac2iupkey;

static Imac2iupkey s_macKeyMap[] = {
  {kVK_Escape, K_ESC},
  {kVK_Home, K_HOME},
  {kVK_UpArrow, K_UP},
  {kVK_PageUp, K_PGUP},
  {kVK_LeftArrow, K_LEFT},
  {kVK_RightArrow, K_RIGHT},
  {kVK_End, K_END},
  {kVK_DownArrow, K_DOWN},
  {kVK_PageDown, K_PGDN},
  {kVK_ForwardDelete, K_DEL},
  {kVK_Space, K_SP},
  {kVK_Tab, K_TAB},
  {kVK_Return, K_CR},
  {kVK_Delete, K_BS},

  {kVK_Shift, K_LSHIFT},
  {kVK_CapsLock, K_CAPS},
  {kVK_Option, K_LALT},
  {kVK_Control, K_LCTRL},
  {kVK_RightShift, K_RSHIFT},
  {kVK_RightOption, K_RALT},
  {kVK_RightControl, K_RCTRL},

  {kVK_Help, K_HELP},

  {kVK_ANSI_1, K_1},
  {kVK_ANSI_2, K_2},
  {kVK_ANSI_3, K_3},
  {kVK_ANSI_4, K_4},
  {kVK_ANSI_5, K_5},
  {kVK_ANSI_6, K_6},
  {kVK_ANSI_7, K_7},
  {kVK_ANSI_8, K_8},
  {kVK_ANSI_9, K_9},
  {kVK_ANSI_0, K_0},

  {kVK_ANSI_A, K_a},
  {kVK_ANSI_B, K_b},
  {kVK_ANSI_C, K_c},
  {kVK_ANSI_D, K_d},
  {kVK_ANSI_E, K_e},
  {kVK_ANSI_F, K_f},
  {kVK_ANSI_G, K_g},
  {kVK_ANSI_H, K_h},
  {kVK_ANSI_I, K_i},
  {kVK_ANSI_J, K_j},
  {kVK_ANSI_K, K_k},
  {kVK_ANSI_L, K_l},
  {kVK_ANSI_M, K_m},
  {kVK_ANSI_N, K_n},
  {kVK_ANSI_O, K_o},
  {kVK_ANSI_P, K_p},
  {kVK_ANSI_Q, K_q},
  {kVK_ANSI_R, K_r},
  {kVK_ANSI_S, K_s},
  {kVK_ANSI_T, K_t},
  {kVK_ANSI_U, K_u},
  {kVK_ANSI_V, K_v},
  {kVK_ANSI_W, K_w},
  {kVK_ANSI_X, K_x},
  {kVK_ANSI_Y, K_y},
  {kVK_ANSI_Z, K_z},

  {kVK_F1, K_F1},
  {kVK_F2, K_F2},
  {kVK_F3, K_F3},
  {kVK_F4, K_F4},
  {kVK_F5, K_F5},
  {kVK_F6, K_F6},
  {kVK_F7, K_F7},
  {kVK_F8, K_F8},
  {kVK_F9, K_F9},
  {kVK_F10, K_F10},
  {kVK_F11, K_F11},
  {kVK_F12, K_F12},
  {kVK_F13, K_F13},
  {kVK_F14, K_F14},
  {kVK_F15, K_F15},
  {kVK_F16, K_F16},
  {kVK_F17, K_F17},
  {kVK_F18, K_F18},
  {kVK_F19, K_F19},
  {kVK_F20, K_F20},

  {kVK_ANSI_Semicolon, K_semicolon},
  {kVK_ANSI_Equal, K_equal},
  {kVK_ANSI_Comma, K_comma},
  {kVK_ANSI_Minus, K_minus},
  {kVK_ANSI_Period, K_period},
  {kVK_ANSI_Slash, K_slash},
  {kVK_ANSI_Grave, K_grave},
  {kVK_ANSI_LeftBracket, K_bracketleft},
  {kVK_ANSI_Backslash, K_backslash},
  {kVK_ANSI_RightBracket, K_bracketright},
  {kVK_ANSI_Quote, K_apostrophe},

  {kVK_ANSI_Keypad0, K_0},
  {kVK_ANSI_Keypad1, K_1},
  {kVK_ANSI_Keypad2, K_2},
  {kVK_ANSI_Keypad3, K_3},
  {kVK_ANSI_Keypad4, K_4},
  {kVK_ANSI_Keypad5, K_5},
  {kVK_ANSI_Keypad6, K_6},
  {kVK_ANSI_Keypad7, K_7},
  {kVK_ANSI_Keypad8, K_8},
  {kVK_ANSI_Keypad9, K_9},
  {kVK_ANSI_KeypadMultiply, K_asterisk},
  {kVK_ANSI_KeypadPlus, K_plus},
  {kVK_ANSI_KeypadMinus, K_minus},
  {kVK_ANSI_KeypadDecimal, K_period},
  {kVK_ANSI_KeypadDivide, K_slash},
  {kVK_ANSI_KeypadClear, K_CLEAR},
  {kVK_ANSI_KeypadEquals, K_equal},
  {kVK_ANSI_KeypadEnter, K_CR},

  {kVK_JIS_KeypadComma, K_comma}
};

static int iupObjectIsNativeContainer(Ihandle* ih)
{
  if (ih->iclass->childtype != IUP_CHILDNONE &&
      ih->iclass->nativetype != IUP_TYPEVOID)
    return 1;
  else
    return 0;
}

/*
 * Decodes an NSEvent to its corresponding IUP key code.
 *
 * For printable ASCII keys without Ctrl/Alt/Sys modifiers:
 * - Uses the actual character from the event
 * - This respects keyboard layout, Shift state, and CapsLock automatically
 *
 * For special keys or when Ctrl/Alt/Sys modifiers are present:
 * - Applies IUP modifier encoding systematically
 * - Converts lowercase to uppercase when modifiers are present
 * - Handles CapsLock inverting Shift state for letters
 */
static int cocoaKeyDecode(NSEvent *ns_event, int mac_key_code)
{
  const size_t array_length = sizeof(s_macKeyMap) / sizeof(s_macKeyMap[0]);
  int iup_base_key = 0;
  size_t i;

  /* Handle modifier keys directly */
  if (mac_key_code == kVK_Shift)
    return K_LSHIFT;
  if (mac_key_code == kVK_RightShift)
    return K_RSHIFT;
  if (mac_key_code == kVK_Control)
    return K_LCTRL;
  if (mac_key_code == kVK_RightControl)
    return K_RCTRL;
  if (mac_key_code == kVK_Option)
    return K_LALT;
  if (mac_key_code == kVK_RightOption)
    return K_RALT;
  if (mac_key_code == kVK_Command || mac_key_code == kVK_RightCommand)
    return 0;
  if (mac_key_code == kVK_CapsLock)
    return K_CAPS;

  /* Look up base key code from virtual key code */
  for (i = 0; i < array_length; i++)
  {
    if (s_macKeyMap[i].maccode == mac_key_code)
    {
      iup_base_key = s_macKeyMap[i].iupcode;
      break;
    }
  }

  if (iup_base_key == 0)
    return 0;

  /* Get modifier states */
  int has_shift = ([ns_event modifierFlags] & NSEventModifierFlagShift) != 0;
  int has_ctrl = ([ns_event modifierFlags] & NSEventModifierFlagControl) != 0;
  int has_alt = ([ns_event modifierFlags] & NSEventModifierFlagOption) != 0;
  int has_sys = ([ns_event modifierFlags] & NSEventModifierFlagCommand) != 0;
  int has_caps = ([ns_event modifierFlags] & NSEventModifierFlagCapsLock) != 0;

  int iup_result_key = iup_base_key;

  /* For printable keys without Ctrl/Alt/Sys, use the actual character.
   * This automatically handles keyboard layout, Shift, and CapsLock.
   * The range K_exclam to K_tilde covers all printable ASCII except space.
   * Space (K_SP) is intentionally excluded because it should get Shift
   * encoding when combined with Shift, allowing applications to distinguish
   * Space from Shift+Space if needed. */
  if ((iup_base_key >= K_exclam && iup_base_key <= K_tilde) &&
      ([ns_event type] == NSEventTypeKeyDown || [ns_event type] == NSEventTypeKeyUp) &&
      !(has_ctrl || has_alt || has_sys))
  {
    NSString *chars = [ns_event characters];
    if ([chars length] > 0)
    {
      unichar ch = [chars characterAtIndex:0];
      if (ch >= K_SP && ch <= K_tilde)
      {
        iup_result_key = (int)ch;
      }
    }
  }
  else
  {
    /* For special keys or when modifiers are present, apply IUP encoding */
    int is_letter = (iup_result_key >= K_a && iup_result_key <= K_z);

    /* CapsLock inverts shift state for letters */
    if (has_caps && is_letter)
    {
      has_shift = !has_shift;
    }

    /* Convert to uppercase when Ctrl/Alt/Sys are present */
    if (has_ctrl || has_alt || has_sys)
    {
      if (iup_result_key >= K_a && iup_result_key <= K_z)
      {
        iup_result_key = iup_toupper(iup_result_key);
      }
      else if (iup_result_key == K_ccedilla)
      {
        iup_result_key = K_Ccedilla;
      }
    }

    /* Apply Shift encoding for:
     * - Non-printable keys (arrows, function keys, etc.)
     * - Printable keys when other modifiers are present
     * This matches the behavior in GTK and Windows implementations. */
    if (has_shift &&
        ((iup_result_key < K_exclam || iup_result_key > K_tilde) ||
         (has_ctrl || has_alt || has_sys)))
    {
      iup_result_key = iup_XkeyShift(iup_result_key);
    }

    /* Apply remaining modifier encodings */
    if (has_ctrl)
      iup_result_key = iup_XkeyCtrl(iup_result_key);
    if (has_alt)
      iup_result_key = iup_XkeyAlt(iup_result_key);
    if (has_sys)
      iup_result_key = iup_XkeySys(iup_result_key);
  }

  return iup_result_key;
}

bool iupCocoaKeyDownEvent(Ihandle *ih, NSEvent *ns_event, int mac_key_code)
{
  int result;
  int iup_key_code;

  if (!ih->iclass->is_interactive)
  {
    return false;
  }

  iup_key_code = cocoaKeyDecode(ns_event, mac_key_code);

  if (iup_key_code == 0)
  {
    return false;
  }

  if (iupObjectIsNativeContainer(ih))
  {
    NSWindow* win = (NSWindow*)IupGetDialog(ih)->handle;
    if (win)
    {
      NSResponder* first_responder = [win firstResponder];
      if (first_responder && [first_responder isKindOfClass:[NSView class]])
      {
        Ihandle* focused_ih = (Ihandle*)objc_getAssociatedObject(first_responder, IHANDLE_ASSOCIATED_OBJ_KEY);
        if (iupObjectCheck(focused_ih) && focused_ih != ih)
        {
          /* This event is for the container (ih), but a child control has focus. */
          /* If the child has its own K_ANY handler, let it process the event first. */
          /* The container should not process it to avoid duplication. */
          if (IupGetCallback(focused_ih, "K_ANY") != NULL)
          {
            return false;
          }
        }
      }
    }
  }

  result = iupKeyCallKeyCb(ih, iup_key_code);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return true;
  }
  else if (result == IUP_IGNORE)
  {
    return true;
  }

  if (iupObjectCheck(ih))
  {
    if (ih->iclass->nativetype == IUP_TYPECANVAS)
    {
      result = iupKeyCallKeyPressCb(ih, iup_key_code, 1);
      if (result == IUP_CLOSE)
      {
        IupExitLoop();
        return true;
      }
      else if (result == IUP_IGNORE)
      {
        return true;
      }
    }

    if ([ns_event modifierFlags] & NSEventModifierFlagOption)
    {
      int base_code = iup_XkeyBase(iup_key_code);
      if (base_code < 128 && iupKeyProcessMnemonic(ih, base_code))
      {
        return true;
      }
    }

    int has_shift = ([ns_event modifierFlags] & NSEventModifierFlagShift) ? 1 : 0;
    BOOL nav_result = iupKeyProcessNavigation(ih, iup_key_code, has_shift);

    if (nav_result)
      return true;

    if (iup_key_code == K_F1)
    {
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb)
      {
        if (cb(ih) == IUP_CLOSE)
          IupExitLoop();
      }
    }
  }

  return false;
}

bool iupCocoaKeyUpEvent(Ihandle *ih, NSEvent *ns_event, int mac_key_code)
{
  int result;
  int iup_key_code;

  if (!ih->iclass->is_interactive)
    return false;

  /* Call KEYPRESS_CB for canvas elements with press=0 */
  if (ih->iclass->nativetype == IUP_TYPECANVAS)
  {
    iup_key_code = cocoaKeyDecode(ns_event, mac_key_code);
    if (iup_key_code == 0)
      return false;

    result = iupKeyCallKeyPressCb(ih, iup_key_code, 0);
    if (result == IUP_CLOSE)
    {
      IupExitLoop();
      return true;
    }
    else if (result == IUP_IGNORE)
    {
      return true;
    }
  }

  return false;
}

bool iupcocoaKeyEvent(Ihandle *ih, NSEvent *ns_event, int mac_key_code, bool is_pressed)
{
  if (is_pressed)
  {
    return iupCocoaKeyDownEvent(ih, ns_event, mac_key_code);
  }
  else
  {
    return iupCocoaKeyUpEvent(ih, ns_event, mac_key_code);
  }
}

bool iupcocoaModifierEvent(Ihandle *ih, NSEvent *ns_event, int mac_key_code)
{
  bool is_pressed = false;
  NSEventModifierFlags flags = [ns_event modifierFlags];

  /* Cocoa sends flagsChanged events for modifier keys. */
  switch (mac_key_code)
  {
    case kVK_Shift:
    case kVK_RightShift:
      is_pressed = (flags & NSEventModifierFlagShift) != 0;
      break;
    case kVK_Control:
    case kVK_RightControl:
      is_pressed = (flags & NSEventModifierFlagControl) != 0;
      break;
    case kVK_Option:
    case kVK_RightOption:
      is_pressed = (flags & NSEventModifierFlagOption) != 0;
      break;
    case kVK_CapsLock:
      is_pressed = (flags & NSEventModifierFlagCapsLock) != 0;
      break;
    case kVK_Function:
      is_pressed = (flags & NSEventModifierFlagFunction) != 0;
      break;
    case kVK_Command:
    case kVK_RightCommand:
      is_pressed = (flags & NSEventModifierFlagCommand) != 0;
      break;
    default:
      return false;
  }

  return iupcocoaKeyEvent(ih, ns_event, mac_key_code, is_pressed);
}

void iupcocoaButtonKeySetStatus(NSEvent *ns_event, char *out_status)
{
  NSEventModifierFlags flags = [ns_event modifierFlags];

  /* Set modifier key states */
  if (flags & NSEventModifierFlagShift)
    iupKEY_SETSHIFT(out_status);
  if (flags & NSEventModifierFlagControl)
    iupKEY_SETCONTROL(out_status);
  if (flags & NSEventModifierFlagOption)
    iupKEY_SETALT(out_status);
  if (flags & NSEventModifierFlagCommand)
    iupKEY_SETSYS(out_status);

  /* Check for double-click on mouse events */
  NSEventType event_type = [ns_event type];
  if (event_type == NSEventTypeLeftMouseDown ||
      event_type == NSEventTypeRightMouseDown ||
      event_type == NSEventTypeOtherMouseDown)
  {
    if ([ns_event clickCount] == 2)
      iupKEY_SETDOUBLE(out_status);
  }

  /* Get current state of pressed mouse buttons */
  NSUInteger pressed_buttons = [NSEvent pressedMouseButtons];

  if (pressed_buttons & (1 << 0))
    iupKEY_SETBUTTON1(out_status);
  if (pressed_buttons & (1 << 1))
    iupKEY_SETBUTTON3(out_status);
  if (pressed_buttons & (1 << 2))
    iupKEY_SETBUTTON2(out_status);
  if (pressed_buttons & (1 << 3))
    iupKEY_SETBUTTON4(out_status);
  if (pressed_buttons & (1 << 4))
    iupKEY_SETBUTTON5(out_status);
}

int iupcocoaKeyDecode(CGEventRef event)
{
  CGKeyCode mac_key_code = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
  CGEventFlags flags = CGEventGetFlags(event);
  int iup_base_key = 0;
  const size_t array_length = sizeof(s_macKeyMap) / sizeof(s_macKeyMap[0]);
  size_t i;

  /* This function is used for simulating key events via iupdrvSendKey.
   * It only has the virtual key code and flags, not the actual character. */

  /* Look up base key code */
  for (i = 0; i < array_length; i++)
  {
    if (s_macKeyMap[i].maccode == mac_key_code)
    {
      iup_base_key = s_macKeyMap[i].iupcode;
      break;
    }
  }

  if (iup_base_key == 0)
    return 0;

  int iup_result_key = iup_base_key;

  /* Get modifier states */
  int has_shift = (flags & kCGEventFlagMaskShift) != 0;
  int has_ctrl = (flags & kCGEventFlagMaskControl) != 0;
  int has_alt = (flags & kCGEventFlagMaskAlternate) != 0;
  int has_sys = (flags & kCGEventFlagMaskCommand) != 0;
  int has_caps = (flags & kCGEventFlagMaskAlphaShift) != 0;

  int is_letter = (iup_result_key >= K_a && iup_result_key <= K_z);

  /* CapsLock inverts shift state for letters */
  if (has_caps && is_letter)
  {
    has_shift = !has_shift;
  }

  /* Convert to uppercase when Ctrl/Alt/Sys are present */
  if (has_ctrl || has_alt || has_sys)
  {
    if (iup_result_key >= K_a && iup_result_key <= K_z)
    {
      iup_result_key = iup_toupper(iup_result_key);
    }
    else if (iup_result_key == K_ccedilla)
    {
      iup_result_key = K_Ccedilla;
    }
  }

  /* Apply Shift encoding for non-printable keys or when other modifiers are present */
  if (has_shift &&
      ((iup_result_key < K_exclam || iup_result_key > K_tilde) ||
       (has_ctrl || has_alt || has_sys)))
  {
    iup_result_key = iup_XkeyShift(iup_result_key);
  }

  /* Apply remaining modifier encodings */
  if (has_ctrl)
    iup_result_key = iup_XkeyCtrl(iup_result_key);
  if (has_alt)
    iup_result_key = iup_XkeyAlt(iup_result_key);
  if (has_sys)
    iup_result_key = iup_XkeySys(iup_result_key);

  return iup_result_key;
}

void iupdrvKeyEncode(int code, unsigned int *maccode, unsigned int *state)
{
  int i;
  int iup_base_key = iup_XkeyBase(code);
  const size_t array_length = sizeof(s_macKeyMap) / sizeof(s_macKeyMap[0]);

  *maccode = 0;
  *state = 0;

  /* Look up macOS virtual key code from IUP key code */
  for (i = 0; i < array_length; i++)
  {
    if (s_macKeyMap[i].iupcode == iup_base_key)
    {
      *maccode = s_macKeyMap[i].maccode;
      break;
    }
  }

  if (*maccode == 0)
    return;

  /* Encode modifier flags */
  if (iup_isShiftXkey(code))
    *state |= NSEventModifierFlagShift;
  if (iup_isCtrlXkey(code))
    *state |= NSEventModifierFlagControl;
  if (iup_isAltXkey(code))
    *state |= NSEventModifierFlagOption;
  if (iup_isSysXkey(code))
    *state |= NSEventModifierFlagCommand;
}