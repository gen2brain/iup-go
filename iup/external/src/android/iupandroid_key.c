/** \file
 * \brief Android Driver keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"
#include "iupkey.h"

#include "iup_drv.h"


/* Subset of android.view.KeyEvent KEYCODE_* values needed for IUP mapping. */
#define AKEYCODE_HOME          3
#define AKEYCODE_0             7
#define AKEYCODE_DPAD_UP       19
#define AKEYCODE_DPAD_DOWN     20
#define AKEYCODE_DPAD_LEFT     21
#define AKEYCODE_DPAD_RIGHT    22
#define AKEYCODE_A             29
#define AKEYCODE_TAB           61
#define AKEYCODE_SPACE         62
#define AKEYCODE_ENTER         66
#define AKEYCODE_DEL           67   /* backspace */
#define AKEYCODE_MENU          82
#define AKEYCODE_PAGE_UP       92
#define AKEYCODE_PAGE_DOWN     93
#define AKEYCODE_ESCAPE        111
#define AKEYCODE_FORWARD_DEL   112  /* delete forward */
#define AKEYCODE_CAPS_LOCK     115
#define AKEYCODE_SCROLL_LOCK   116
#define AKEYCODE_SYSRQ         120  /* print screen */
#define AKEYCODE_BREAK         121  /* pause */
#define AKEYCODE_MOVE_HOME     122
#define AKEYCODE_MOVE_END      123
#define AKEYCODE_INSERT        124
#define AKEYCODE_F1            131
#define AKEYCODE_NUM_LOCK      143

/* android.view.KeyEvent META_* flags. */
#define AMETA_SHIFT_ON  0x0001
#define AMETA_ALT_ON    0x0002
#define AMETA_CTRL_ON   0x1000
#define AMETA_META_ON   0x10000

void iupdrvKeyEncode(int code, unsigned int* keyval, unsigned int* state)
{
  int base = iup_XkeyBase(code);
  unsigned int kv = 0;
  unsigned int s = 0;

  if (base >= K_a && base <= K_z)
    kv = AKEYCODE_A + (base - K_a);
  else if (base >= K_A && base <= K_Z)
    kv = AKEYCODE_A + (base - K_A);
  else if (base >= K_0 && base <= K_9)
    kv = AKEYCODE_0 + (base - K_0);
  else if (base >= K_F1 && base <= K_F12)
    kv = AKEYCODE_F1 + (base - K_F1);
  else
  {
    switch (base)
    {
      case K_SP:     kv = AKEYCODE_SPACE;        break;
      case K_BS:     kv = AKEYCODE_DEL;          break;
      case K_TAB:    kv = AKEYCODE_TAB;          break;
      case K_CR:     kv = AKEYCODE_ENTER;        break;
      case K_ESC:    kv = AKEYCODE_ESCAPE;       break;
      case K_HOME:   kv = AKEYCODE_MOVE_HOME;    break;
      case K_END:    kv = AKEYCODE_MOVE_END;     break;
      case K_LEFT:   kv = AKEYCODE_DPAD_LEFT;    break;
      case K_UP:     kv = AKEYCODE_DPAD_UP;      break;
      case K_RIGHT:  kv = AKEYCODE_DPAD_RIGHT;   break;
      case K_DOWN:   kv = AKEYCODE_DPAD_DOWN;    break;
      case K_PGUP:   kv = AKEYCODE_PAGE_UP;      break;
      case K_PGDN:   kv = AKEYCODE_PAGE_DOWN;    break;
      case K_INS:    kv = AKEYCODE_INSERT;       break;
      case K_DEL:    kv = AKEYCODE_FORWARD_DEL;  break;
      case K_PAUSE:  kv = AKEYCODE_BREAK;        break;
      case K_Print:  kv = AKEYCODE_SYSRQ;        break;
      case K_Menu:   kv = AKEYCODE_MENU;         break;
      case K_NUM:    kv = AKEYCODE_NUM_LOCK;     break;
      case K_CAPS:   kv = AKEYCODE_CAPS_LOCK;    break;
      case K_SCROLL: kv = AKEYCODE_SCROLL_LOCK;  break;
      default:       kv = 0;                     break;
    }
  }

  if (iup_isShiftXkey(code)) s |= AMETA_SHIFT_ON;
  if (iup_isCtrlXkey(code))  s |= AMETA_CTRL_ON;
  if (iup_isAltXkey(code))   s |= AMETA_ALT_ON;
  if (iup_isSysXkey(code))   s |= AMETA_META_ON;

  if (keyval) *keyval = kv;
  if (state)  *state = s;
}
