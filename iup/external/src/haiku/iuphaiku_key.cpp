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

static int haikuApplyModifiers(int code, unsigned int modifiers)
{
  if (modifiers & B_SHIFT_KEY)   code = iup_XkeyShift(code);
  if (modifiers & B_CONTROL_KEY) code = iup_XkeyCtrl(code);
  if (modifiers & B_OPTION_KEY)  code = iup_XkeyAlt(code);
  if (modifiers & B_COMMAND_KEY) code = iup_XkeySys(code);
  return code;
}


IUP_DRV_API int iuphaikuKeyDecode(int byte, int raw_char, unsigned int modifiers)
{
  int code = 0;
  uint8 b = (uint8)(byte & 0xFF);
  uint8 raw = (uint8)(raw_char & 0xFF);

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
