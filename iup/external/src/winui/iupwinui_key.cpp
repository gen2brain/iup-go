/** \file
 * \brief WinUI Driver - Keyboard Handling
 *
 * Translates Windows virtual key codes to IUP key codes.
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupkey.h"
#include "iup_object.h"
#include "iup_drv.h"
#include "iup_key.h"
#include "iup_str.h"
}

#include "iupwinui_drv.h"

static int winuiKeyMap[256] = {0};
static int winuiKeyMapShift[256] = {0};
static int winui_key_initialized = 0;

static void winuiKeyInit(void)
{
  if (winui_key_initialized)
    return;

  memset(winuiKeyMap, 0, sizeof(winuiKeyMap));
  memset(winuiKeyMapShift, 0, sizeof(winuiKeyMapShift));

  winuiKeyMap[VK_ESCAPE] = K_ESC;
  winuiKeyMap[VK_PAUSE] = K_PAUSE;
  winuiKeyMap[VK_SNAPSHOT] = K_Print;
  winuiKeyMap[VK_APPS] = K_Menu;

  winuiKeyMap[VK_CAPITAL] = K_CAPS;
  winuiKeyMap[VK_NUMLOCK] = K_NUM;
  winuiKeyMap[VK_SCROLL] = K_SCROLL;

  winuiKeyMap[VK_SHIFT] = K_LSHIFT;
  winuiKeyMap[VK_CONTROL] = K_LCTRL;
  winuiKeyMap[VK_MENU] = K_LALT;

  winuiKeyMap[VK_HOME] = K_HOME;
  winuiKeyMap[VK_UP] = K_UP;
  winuiKeyMap[VK_PRIOR] = K_PGUP;
  winuiKeyMap[VK_LEFT] = K_LEFT;
  winuiKeyMap[VK_CLEAR] = K_MIDDLE;
  winuiKeyMap[VK_RIGHT] = K_RIGHT;
  winuiKeyMap[VK_END] = K_END;
  winuiKeyMap[VK_DOWN] = K_DOWN;
  winuiKeyMap[VK_NEXT] = K_PGDN;
  winuiKeyMap[VK_INSERT] = K_INS;
  winuiKeyMap[VK_DELETE] = K_DEL;
  winuiKeyMap[VK_SPACE] = K_SP;
  winuiKeyMap[VK_TAB] = K_TAB;
  winuiKeyMap[VK_RETURN] = K_CR;
  winuiKeyMap[VK_BACK] = K_BS;

  winuiKeyMap[VK_F1] = K_F1;
  winuiKeyMap[VK_F2] = K_F2;
  winuiKeyMap[VK_F3] = K_F3;
  winuiKeyMap[VK_F4] = K_F4;
  winuiKeyMap[VK_F5] = K_F5;
  winuiKeyMap[VK_F6] = K_F6;
  winuiKeyMap[VK_F7] = K_F7;
  winuiKeyMap[VK_F8] = K_F8;
  winuiKeyMap[VK_F9] = K_F9;
  winuiKeyMap[VK_F10] = K_F10;
  winuiKeyMap[VK_F11] = K_F11;
  winuiKeyMap[VK_F12] = K_F12;

  winuiKeyMap[VK_NUMPAD0] = K_0;
  winuiKeyMap[VK_NUMPAD1] = K_1;
  winuiKeyMap[VK_NUMPAD2] = K_2;
  winuiKeyMap[VK_NUMPAD3] = K_3;
  winuiKeyMap[VK_NUMPAD4] = K_4;
  winuiKeyMap[VK_NUMPAD5] = K_5;
  winuiKeyMap[VK_NUMPAD6] = K_6;
  winuiKeyMap[VK_NUMPAD7] = K_7;
  winuiKeyMap[VK_NUMPAD8] = K_8;
  winuiKeyMap[VK_NUMPAD9] = K_9;
  winuiKeyMap[VK_MULTIPLY] = K_asterisk;
  winuiKeyMap[VK_ADD] = K_plus;
  winuiKeyMap[VK_SUBTRACT] = K_minus;
  winuiKeyMap[VK_DIVIDE] = K_slash;

  winuiKeyMap['0'] = K_0;
  winuiKeyMap['1'] = K_1;
  winuiKeyMap['2'] = K_2;
  winuiKeyMap['3'] = K_3;
  winuiKeyMap['4'] = K_4;
  winuiKeyMap['5'] = K_5;
  winuiKeyMap['6'] = K_6;
  winuiKeyMap['7'] = K_7;
  winuiKeyMap['8'] = K_8;
  winuiKeyMap['9'] = K_9;

  winuiKeyMap['A'] = K_a;
  winuiKeyMap['B'] = K_b;
  winuiKeyMap['C'] = K_c;
  winuiKeyMap['D'] = K_d;
  winuiKeyMap['E'] = K_e;
  winuiKeyMap['F'] = K_f;
  winuiKeyMap['G'] = K_g;
  winuiKeyMap['H'] = K_h;
  winuiKeyMap['I'] = K_i;
  winuiKeyMap['J'] = K_j;
  winuiKeyMap['K'] = K_k;
  winuiKeyMap['L'] = K_l;
  winuiKeyMap['M'] = K_m;
  winuiKeyMap['N'] = K_n;
  winuiKeyMap['O'] = K_o;
  winuiKeyMap['P'] = K_p;
  winuiKeyMap['Q'] = K_q;
  winuiKeyMap['R'] = K_r;
  winuiKeyMap['S'] = K_s;
  winuiKeyMap['T'] = K_t;
  winuiKeyMap['U'] = K_u;
  winuiKeyMap['V'] = K_v;
  winuiKeyMap['W'] = K_w;
  winuiKeyMap['X'] = K_x;
  winuiKeyMap['Y'] = K_y;
  winuiKeyMap['Z'] = K_z;

  winuiKeyMapShift['A'] = K_A;
  winuiKeyMapShift['B'] = K_B;
  winuiKeyMapShift['C'] = K_C;
  winuiKeyMapShift['D'] = K_D;
  winuiKeyMapShift['E'] = K_E;
  winuiKeyMapShift['F'] = K_F;
  winuiKeyMapShift['G'] = K_G;
  winuiKeyMapShift['H'] = K_H;
  winuiKeyMapShift['I'] = K_I;
  winuiKeyMapShift['J'] = K_J;
  winuiKeyMapShift['K'] = K_K;
  winuiKeyMapShift['L'] = K_L;
  winuiKeyMapShift['M'] = K_M;
  winuiKeyMapShift['N'] = K_N;
  winuiKeyMapShift['O'] = K_O;
  winuiKeyMapShift['P'] = K_P;
  winuiKeyMapShift['Q'] = K_Q;
  winuiKeyMapShift['R'] = K_R;
  winuiKeyMapShift['S'] = K_S;
  winuiKeyMapShift['T'] = K_T;
  winuiKeyMapShift['U'] = K_U;
  winuiKeyMapShift['V'] = K_V;
  winuiKeyMapShift['W'] = K_W;
  winuiKeyMapShift['X'] = K_X;
  winuiKeyMapShift['Y'] = K_Y;
  winuiKeyMapShift['Z'] = K_Z;

  winui_key_initialized = 1;
}

extern "C" int iupwinuiKeyEvent(Ihandle* ih, int wincode, int press)
{
  int result, code;

  if (!ih->iclass->is_interactive)
    return 1;

  code = iupwinuiKeyDecode(wincode);
  if (code == 0)
    return 1;

  if (!press)
    return 1;

  result = iupKeyCallKeyCb(ih, code);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return 1;
  }
  if (result == IUP_IGNORE)
    return 0;

  if (!iupObjectCheck(ih))
    return 1;

  if ((GetKeyState(VK_MENU) & 0x8000) && wincode < 128)
  {
    if (iupKeyProcessMnemonic(ih, wincode))
      return 0;
  }

  if (iupKeyProcessNavigation(ih, code, (GetKeyState(VK_SHIFT) & 0x8000)))
    return 0;

  return 1;
}

extern "C" int iupwinuiKeyDecode(int wincode)
{
  int iupcode;
  int has_shift, has_ctrl, has_alt, has_sys;

  winuiKeyInit();

  if (wincode < 0 || wincode > 255)
    return 0;

  has_shift = GetKeyState(VK_SHIFT) & 0x8000;
  has_ctrl = GetKeyState(VK_CONTROL) & 0x8000;
  has_alt = GetKeyState(VK_MENU) & 0x8000;
  has_sys = (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000);

  if (wincode == VK_SHIFT && (GetKeyState(VK_RSHIFT) & 0x8000))
    iupcode = K_RSHIFT;
  else if (wincode == VK_CONTROL && (GetKeyState(VK_RCONTROL) & 0x8000))
    iupcode = K_RCTRL;
  else if (wincode == VK_MENU && (GetKeyState(VK_RMENU) & 0x8000))
    iupcode = K_RALT;
  else if (has_shift && winuiKeyMapShift[wincode])
    iupcode = winuiKeyMapShift[wincode];
  else
    iupcode = winuiKeyMap[wincode];

  if (!iupcode)
    iupcode = wincode;

  if (has_ctrl || has_alt || has_sys)
  {
    if (iupcode >= K_a && iupcode <= K_z)
      iupcode = iup_toupper(iupcode);
  }

  if (has_shift)
  {
    if ((iupcode < K_exclam || iupcode > K_tilde) || (has_ctrl || has_alt || has_sys))
      iupcode = iup_XkeyShift(iupcode);
  }

  if (has_ctrl)
    iupcode = iup_XkeyCtrl(iupcode);

  if (has_alt)
    iupcode = iup_XkeyAlt(iupcode);

  if (has_sys)
    iupcode = iup_XkeySys(iupcode);

  return iupcode;
}

extern "C" void iupwinuiButtonKeySetStatus(int keys, int button, char* status, int doubleclick)
{
  iupKEY_SETBUTTON1(status);
  iupKEY_SETBUTTON2(status);
  iupKEY_SETBUTTON3(status);
  iupKEY_SETBUTTON4(status);
  iupKEY_SETBUTTON5(status);
  iupKEY_SETSHIFT(status);
  iupKEY_SETCONTROL(status);
  iupKEY_SETALT(status);
  iupKEY_SETSYS(status);
  iupKEY_SETDOUBLE(status);

  status[0] = ' ';
  status[1] = ' ';
  status[2] = ' ';
  status[3] = ' ';
  status[4] = ' ';
  status[5] = ' ';

  if (keys & MK_SHIFT)
    iupKEY_SETSHIFT(status);

  if (keys & MK_CONTROL)
    iupKEY_SETCONTROL(status);

  if (keys & MK_LBUTTON)
    iupKEY_SETBUTTON1(status);

  if (keys & MK_MBUTTON)
    iupKEY_SETBUTTON2(status);

  if (keys & MK_RBUTTON)
    iupKEY_SETBUTTON3(status);

  if (doubleclick)
    iupKEY_SETDOUBLE(status);

  if (GetKeyState(VK_MENU) & 0x8000)
    iupKEY_SETALT(status);

  if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000))
    iupKEY_SETSYS(status);

  if (keys & MK_XBUTTON1)
    iupKEY_SETBUTTON4(status);

  if (keys & MK_XBUTTON2)
    iupKEY_SETBUTTON5(status);

  (void)button;
}

extern "C" void iupdrvKeyEncode(int code, unsigned int* wincode, unsigned int* state)
{
  int i, iupcode;

  winuiKeyInit();

  iupcode = iup_XkeyBase(code);
  *wincode = 0;
  *state = 0;

  for (i = 0; i < 256; i++)
  {
    if (winuiKeyMap[i] == iupcode)
    {
      *wincode = i;
      break;
    }
    if (winuiKeyMapShift[i] == iupcode)
    {
      *wincode = i;
      code = iup_XkeyShift(code);
      break;
    }
  }

  if (iup_isShiftXkey(code))
    *state = VK_SHIFT;
  else if (iup_isCtrlXkey(code))
    *state = VK_CONTROL;
  else if (iup_isAltXkey(code))
    *state = VK_MENU;
  else if (iup_isSysXkey(code))
    *state = VK_LWIN;
}
