/** \file
 * \brief Haiku Key Encoding / Decoding
 *
 * See Copyright Notice in "iup.h"
 */

#include <InterfaceDefs.h>
#include <SupportDefs.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iupkey.h"
#include "iup_drv.h"
#include "iup_key.h"
}

#include "iuphaiku_drv.h"


typedef struct {
  uint8 haiku_byte;
  int   iup_code;
} HaikuKeyEntry;

static const HaikuKeyEntry kSpecialKeys[] = {
  { B_HOME,        K_HOME },
  { B_END,         K_END },
  { B_PAGE_UP,     K_PGUP },
  { B_PAGE_DOWN,   K_PGDN },
  { B_INSERT,      K_INS },
  { B_DELETE,      K_DEL },
  { B_BACKSPACE,   K_BS },
  { B_TAB,         K_TAB },
  { B_RETURN,      K_CR },
  { B_ESCAPE,      K_ESC },
  { B_LEFT_ARROW,  K_LEFT },
  { B_RIGHT_ARROW, K_RIGHT },
  { B_UP_ARROW,    K_UP },
  { B_DOWN_ARROW,  K_DOWN },
  { B_F1_KEY,      K_F1 },
  { B_F2_KEY,      K_F2 },
  { B_F3_KEY,      K_F3 },
  { B_F4_KEY,      K_F4 },
  { B_F5_KEY,      K_F5 },
  { B_F6_KEY,      K_F6 },
  { B_F7_KEY,      K_F7 },
  { B_F8_KEY,      K_F8 },
  { B_F9_KEY,      K_F9 },
  { B_F10_KEY,     K_F10 },
  { B_F11_KEY,     K_F11 },
  { B_F12_KEY,     K_F12 },
};

static int haikuLookupSpecial(uint8 b)
{
  for (size_t i = 0; i < sizeof(kSpecialKeys)/sizeof(kSpecialKeys[0]); ++i)
    if (kSpecialKeys[i].haiku_byte == b)
      return kSpecialKeys[i].iup_code;
  return 0;
}

typedef struct {
  uint8 scancode;
  int   iup_code;
  int   iup_numlock_code;
} HaikuKeyPadEntry;

static const HaikuKeyPadEntry kKeyPadKeys[] = {
  { 0x23, K_KP_DIV,    0 },
  { 0x24, K_KP_MULT,   0 },
  { 0x25, K_KP_MINUS,  0 },
  { 0x3a, K_KP_PLUS,   0 },
  { 0x5b, K_KP_CR,     0 },
  { 0x6a, K_KP_EQUAL,  0 },
  { 0x37, K_KP_HOME,   K_KP_7 },
  { 0x38, K_KP_UP,     K_KP_8 },
  { 0x39, K_KP_PGUP,   K_KP_9 },
  { 0x48, K_KP_LEFT,   K_KP_4 },
  { 0x49, K_KP_MIDDLE, K_KP_5 },
  { 0x4a, K_KP_RIGHT,  K_KP_6 },
  { 0x58, K_KP_END,    K_KP_1 },
  { 0x59, K_KP_DOWN,   K_KP_2 },
  { 0x5a, K_KP_PGDN,   K_KP_3 },
  { 0x64, K_KP_INS,    K_KP_0 },
  { 0x65, K_KP_DEL,    K_KP_DECIMAL },
};

static int haikuLookupKeyPad(int key, uint8 b)
{
  for (size_t i = 0; i < sizeof(kKeyPadKeys)/sizeof(kKeyPadKeys[0]); ++i)
  {
    if (kKeyPadKeys[i].scancode != (uint8)key)
      continue;

    if (!kKeyPadKeys[i].iup_numlock_code)
      return kKeyPadKeys[i].iup_code;

    if (key == 0x65)
      return (b == ',')? K_KP_SEP: (b == '.')? K_KP_DECIMAL: K_KP_DEL;

    return (b >= '0' && b <= '9')? kKeyPadKeys[i].iup_numlock_code: kKeyPadKeys[i].iup_code;
  }

  return 0;
}

IUP_DRV_API int iuphaikuKeyPadScanCode(int code, int* byte_val)
{
  int base = iup_XkeyBase(code);

  for (size_t i = 0; i < sizeof(kKeyPadKeys)/sizeof(kKeyPadKeys[0]); ++i)
  {
    if (kKeyPadKeys[i].iup_code != base && kKeyPadKeys[i].iup_numlock_code != base)
      continue;

    if (byte_val)
    {
      switch (base)
      {
        case K_KP_DIV:     *byte_val = '/'; break;
        case K_KP_MULT:    *byte_val = '*'; break;
        case K_KP_MINUS:   *byte_val = '-'; break;
        case K_KP_PLUS:    *byte_val = '+'; break;
        case K_KP_EQUAL:   *byte_val = '='; break;
        case K_KP_SEP:     *byte_val = ','; break;
        case K_KP_DECIMAL: *byte_val = '.'; break;
        case K_KP_CR:      *byte_val = B_RETURN; break;
        case K_KP_HOME:    *byte_val = B_HOME; break;
        case K_KP_END:     *byte_val = B_END; break;
        case K_KP_PGUP:    *byte_val = B_PAGE_UP; break;
        case K_KP_PGDN:    *byte_val = B_PAGE_DOWN; break;
        case K_KP_INS:     *byte_val = B_INSERT; break;
        case K_KP_DEL:     *byte_val = B_DELETE; break;
        case K_KP_LEFT:    *byte_val = B_LEFT_ARROW; break;
        case K_KP_RIGHT:   *byte_val = B_RIGHT_ARROW; break;
        case K_KP_UP:      *byte_val = B_UP_ARROW; break;
        case K_KP_DOWN:    *byte_val = B_DOWN_ARROW; break;
        case K_KP_MIDDLE:  *byte_val = '5'; break;
        default:           *byte_val = '0' + (base - K_KP_0); break;
      }
    }

    return kKeyPadKeys[i].scancode;
  }

  return 0;
}

static int haikuApplyModifiers(int code, unsigned int modifiers)
{
  if (modifiers & B_SHIFT_KEY)   code = iup_XkeyShift(code);
  if (modifiers & B_CONTROL_KEY) code = iup_XkeyCtrl(code);
  if (modifiers & B_OPTION_KEY)  code = iup_XkeyAlt(code);
  if (modifiers & B_COMMAND_KEY) code = iup_XkeySys(code);
  return code;
}


IUP_DRV_API int iuphaikuKeyDecode(int byte, int raw_char, int key, unsigned int modifiers)
{
  int code = haikuLookupKeyPad(key, (uint8)(byte & 0xFF));
  uint8 b = (uint8)(byte & 0xFF);
  uint8 raw = (uint8)(raw_char & 0xFF);

  if (code)
    return haikuApplyModifiers(code, modifiers);

  /* Special key disambiguation: B_*_KEY bytes also appear as Ctrl-letters.
   * If raw_char matches a special-key value too, it's the special key. */
  if (raw == b)
    code = haikuLookupSpecial(b);

  if (!code)
  {
    /* Printable ASCII. */
    if (b >= 0x20 && b < 0x7f)
      code = b;
    else if (b == B_BACKSPACE || b == B_TAB || b == B_RETURN ||
             b == B_ESCAPE   || b == B_DELETE)
      code = haikuLookupSpecial(b);
    else if (b < 0x20 && raw >= 0x20 && raw < 0x7f &&
             (modifiers & (B_CONTROL_KEY | B_OPTION_KEY | B_COMMAND_KEY)))
      /* Modifier held: byte arrives as a control char, raw still holds the letter. */
      code = raw;
  }

  if (!code)
    return 0;

  /* IUP convention: when Ctrl/Alt/Sys is held, lowercase letter becomes upper. */
  if (code >= K_a && code <= K_z &&
      (modifiers & (B_CONTROL_KEY | B_OPTION_KEY | B_COMMAND_KEY)))
    code = (code - K_a) + K_A;

  return haikuApplyModifiers(code, modifiers);
}


IUP_DRV_API void iuphaikuButtonKeySetStatus(unsigned int modifiers, unsigned int buttons, int button, char* status, int doubleclick)
{
  if (modifiers & B_SHIFT_KEY)   iupKEY_SETSHIFT(status);
  if (modifiers & B_CONTROL_KEY) iupKEY_SETCONTROL(status);
  if (modifiers & B_OPTION_KEY)  iupKEY_SETALT(status);
  if (modifiers & B_COMMAND_KEY) iupKEY_SETSYS(status);

  if ((buttons & B_PRIMARY_MOUSE_BUTTON)   || button == 1) iupKEY_SETBUTTON1(status);
  if ((buttons & B_TERTIARY_MOUSE_BUTTON)  || button == 2) iupKEY_SETBUTTON2(status);
  if ((buttons & B_SECONDARY_MOUSE_BUTTON) || button == 3) iupKEY_SETBUTTON3(status);

  if (doubleclick) iupKEY_SETDOUBLE(status);
}

/* IupSendKey: IUP code -> Haiku byte + modifier mask. */

extern "C" IUP_SDK_API void iupdrvKeyEncode(int code, unsigned int *keyval, unsigned int *state)
{
  if (keyval) *keyval = 0;
  if (state)  *state = 0;
  if (!keyval || !state) return;

  int base = iup_XkeyBase(code);

  /* Reverse map specials by scanning the same table. */
  uint8 native = 0;
  for (size_t i = 0; i < sizeof(kSpecialKeys)/sizeof(kSpecialKeys[0]); ++i)
  {
    if (kSpecialKeys[i].iup_code == base)
    {
      native = kSpecialKeys[i].haiku_byte;
      break;
    }
  }

  if (!native)
  {
    if (base >= 0x20 && base < 0x7f)
      native = (uint8)base;
  }
  *keyval = native;

  if (iup_isShiftXkey(code)) *state |= B_SHIFT_KEY;
  if (iup_isCtrlXkey(code))  *state |= B_CONTROL_KEY;
  if (iup_isAltXkey(code))   *state |= B_OPTION_KEY;
  if (iup_isSysXkey(code))   *state |= B_COMMAND_KEY;
}
