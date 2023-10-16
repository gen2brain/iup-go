/** \file
 * \brief Windows Driver keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include <windows.h>

#include "iup.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"

#include "iup_drv.h"
#include "iup_focus.h"
#include "iup_attrib.h"
#include "iupwin_drv.h"

                   
/* some older mingw-w64.sf.net headers are missing the following define */
#ifndef MAPVK_VK_TO_CHAR
#define MAPVK_VK_TO_CHAR    2
#endif
                  
typedef struct _Iwin2iupkey
{
  int iupcode;
  int shift_iupcode;   /* base code when shift or caps are pressed */
  int altgr_iupcode;
} Iwin2iupkey;

static int winMapVirtualKeyToChar(int wincode)
{
  return LOWORD(MapVirtualKeyA(wincode, MAPVK_VK_TO_CHAR));
}

static void winKeyInitXKey(Iwin2iupkey* map)
{
  map[VK_CANCEL].iupcode =    K_cPAUSE;
  map[VK_ESCAPE].iupcode =    K_ESC;
  map[VK_PAUSE].iupcode =     K_PAUSE;
  map[VK_SNAPSHOT].iupcode =  K_Print;
  map[VK_APPS].iupcode =      K_Menu;

  map[VK_CAPITAL].iupcode =   K_CAPS;
  map[VK_NUMLOCK].iupcode =   K_NUM;
  map[VK_SCROLL].iupcode =    K_SCROLL;

  map[VK_SHIFT].iupcode =   K_LSHIFT;
  map[VK_CONTROL].iupcode = K_LCTRL;
  map[VK_MENU].iupcode =    K_LALT;
                         
  map[VK_HOME].iupcode =      K_HOME;
  map[VK_UP].iupcode =        K_UP;
  map[VK_PRIOR].iupcode =     K_PGUP;
  map[VK_LEFT].iupcode =      K_LEFT;
  map[VK_CLEAR].iupcode =     K_MIDDLE;
  map[VK_RIGHT].iupcode =     K_RIGHT;
  map[VK_END].iupcode =       K_END;
  map[VK_DOWN].iupcode =      K_DOWN;
  map[VK_NEXT].iupcode =      K_PGDN;
  map[VK_INSERT].iupcode =    K_INS;
  map[VK_DELETE].iupcode =    K_DEL;
  map[VK_SPACE].iupcode =     K_SP;
  map[VK_TAB].iupcode =       K_TAB;
  map[VK_RETURN].iupcode =    K_CR;
  map[VK_BACK].iupcode =      K_BS;

  map[VK_F1].iupcode =  K_F1;
  map[VK_F2].iupcode =  K_F2;
  map[VK_F3].iupcode =  K_F3;
  map[VK_F4].iupcode =  K_F4;
  map[VK_F5].iupcode =  K_F5;
  map[VK_F6].iupcode =  K_F6;
  map[VK_F7].iupcode =  K_F7;
  map[VK_F8].iupcode =  K_F8;
  map[VK_F9].iupcode =  K_F9;
  map[VK_F10].iupcode = K_F10;
  map[VK_F11].iupcode = K_F11;
  map[VK_F12].iupcode = K_F12;
  map[VK_F13].iupcode = K_F13;
  map[VK_F14].iupcode = K_F14;
  map[VK_F15].iupcode = K_F15;
  map[VK_F16].iupcode = K_F16;
  map[VK_F17].iupcode = K_F17;
  map[VK_F18].iupcode = K_F18;
  map[VK_F19].iupcode = K_F19;
  map[VK_F20].iupcode = K_F20;

  map[VK_OEM_PLUS].iupcode = winMapVirtualKeyToChar(VK_OEM_PLUS);  /* Usually is K_plus, but it can be K_Equal when + needs a Shift */
  map[VK_OEM_COMMA].iupcode = winMapVirtualKeyToChar(VK_OEM_COMMA);  /* Usually is K_comma */
  map[VK_OEM_MINUS].iupcode = winMapVirtualKeyToChar(VK_OEM_MINUS);  /* Usually is K_minus */
  map[VK_OEM_PERIOD].iupcode = winMapVirtualKeyToChar(VK_OEM_PERIOD); /* Usually is K_period */

  map[VK_NUMPAD0].iupcode =   K_0;
  map[VK_NUMPAD1].iupcode =   K_1;
  map[VK_NUMPAD2].iupcode =   K_2;
  map[VK_NUMPAD3].iupcode =   K_3;
  map[VK_NUMPAD4].iupcode =   K_4;
  map[VK_NUMPAD5].iupcode =   K_5;
  map[VK_NUMPAD6].iupcode =   K_6;
  map[VK_NUMPAD7].iupcode =   K_7;
  map[VK_NUMPAD8].iupcode =   K_8;
  map[VK_NUMPAD9].iupcode =   K_9;
  map[VK_MULTIPLY].iupcode =  K_asterisk;
  map[VK_ADD].iupcode =       K_plus;
  map[VK_SUBTRACT].iupcode =  K_minus;
  map[VK_DIVIDE].iupcode =    K_slash;

  map[VK_DECIMAL].iupcode =   winMapVirtualKeyToChar(VK_DECIMAL);
  map[VK_SEPARATOR].iupcode = winMapVirtualKeyToChar(VK_SEPARATOR);

  /* 
  if (!map[VK_OEM_1].iupcode) map[VK_OEM_1].iupcode = winMapVirtualKeyToChar(VK_OEM_1);
  if (!map[VK_OEM_2].iupcode) map[VK_OEM_2].iupcode = winMapVirtualKeyToChar(VK_OEM_2);
  if (!map[VK_OEM_3].iupcode) map[VK_OEM_3].iupcode = winMapVirtualKeyToChar(VK_OEM_3);
  if (!map[VK_OEM_4].iupcode) map[VK_OEM_4].iupcode = winMapVirtualKeyToChar(VK_OEM_4);
  if (!map[VK_OEM_5].iupcode) map[VK_OEM_5].iupcode = winMapVirtualKeyToChar(VK_OEM_5);
  if (!map[VK_OEM_6].iupcode) map[VK_OEM_6].iupcode = winMapVirtualKeyToChar(VK_OEM_6);
  if (!map[VK_OEM_7].iupcode) map[VK_OEM_7].iupcode = winMapVirtualKeyToChar(VK_OEM_7);
  if (!map[VK_OEM_8].iupcode) map[VK_OEM_8].iupcode = winMapVirtualKeyToChar(VK_OEM_8);
  */

  map[VK_OEM_102].iupcode = winMapVirtualKeyToChar(VK_OEM_102);  /*  "<>" or "\|" on RT 102-key kbd. */

  {
    HKL k = GetKeyboardLayout(0);    
    if ((int)HIWORD(k) == 0x0416)
    {
      /* ABNT extra definitions */
      map[0xC2].iupcode = winMapVirtualKeyToChar(0xC2);
      map[0xC1].iupcode = winMapVirtualKeyToChar(0xC1);
      map[0xC1].shift_iupcode = '?';
    }
  }
}

static void winKeySetCharMap(Iwin2iupkey* winkey_map, char c)
{
  SHORT ret = VkKeyScanA(c);
  int wincode = LOBYTE(ret);
  if (wincode != -1)
  {
    int state = HIBYTE(ret) & 0x07;  /* Only Shift, Ctrl and Alt states */
    if (state & 1) /* Shift */
      winkey_map[wincode].shift_iupcode = (BYTE)c;
    else if ((state & 2) && (state & 4)) /* Ctrl & Alt = Alt Gr */
      winkey_map[wincode].altgr_iupcode = (BYTE)c;
    else if (state == 0)
      winkey_map[wincode].iupcode = (BYTE)c;
  }
}

static Iwin2iupkey winkey_map[256];

void iupwinKeyInit(void)
{
  int i;

  memset(winkey_map, 0, sizeof(Iwin2iupkey)*256);

  for (i=32; i<127; i++)
    winKeySetCharMap(winkey_map, (char)i);

  winKeySetCharMap(winkey_map, K_ccedilla);
  winKeySetCharMap(winkey_map, K_Ccedilla);
  winKeySetCharMap(winkey_map, K_acute);
  winKeySetCharMap(winkey_map, K_diaeresis);

  winKeyInitXKey(winkey_map);

  for (i=0; i<256; i++)
  {
    if (!(winkey_map[i].iupcode))    
    {  
      winkey_map[i].iupcode = winMapVirtualKeyToChar(i);

      /* TODO: how to get the shift code? 
      if (winkey_map[i].iupcode && !(winkey_map[i].shift_iupcode))
      {
        winkey_map[i].shift_iupcode = LOWORD((DWORD)CharUpperA((LPSTR)MAKELONG(winkey_map[i].iupcode, 0)));
        if (winkey_map[i].shift_iupcode == winkey_map[i].iupcode)
          winkey_map[i].shift_iupcode = 0;
      }  */
    }
  }
}

void iupdrvKeyEncode(int code, unsigned int *wincode, unsigned int *state)
{
  int i, iupcode = iup_XkeyBase(code);

  /* Must un-remap always */
  for (i = 0; i < 256; i++)
  {
    if (winkey_map[i].iupcode == iupcode)
    {
      *wincode = i;
      break;
    }
    if (winkey_map[i].shift_iupcode == iupcode)
    {
      *wincode = i;
      code = iup_XkeyShift(code);  /* add Shift */
      break;
    }
    if (winkey_map[i].altgr_iupcode == iupcode)
    {
      *wincode = i;
      code = iup_XkeyCtrl(code);  /* add Ctrl */
      code = iup_XkeyAlt(code);   /* add Alt */
      break;
    }
  }

  *state = 0;
  if (iup_isShiftXkey(code))
    *state = VK_SHIFT;
  else if (iup_isCtrlXkey(code))
    *state = VK_CONTROL;
  else if (iup_isAltXkey(code))
    *state = VK_MENU;
  else if (iup_isSysXkey(code))
    *state = VK_LWIN;
}

static int winKeyMap2Iup(int wincode)
{
  int code = wincode;

  int has_shift = GetKeyState(VK_SHIFT) & 0x8000;
  int has_ctrl = GetKeyState(VK_CONTROL) & 0x8000;
  int has_alt = GetKeyState(VK_MENU) & 0x8000;
  int has_sys = (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000);

  if (has_ctrl || has_alt || has_sys)
  {
    /* If it has some of the other modifiers then use upper case version */
    if (wincode >= K_a && wincode <= K_z)
      code = iup_toupper(wincode);
    else if (wincode==K_ccedilla)
      code = K_Ccedilla;
  }

  if (has_shift)  /* Shift */
  {
    /* only add Shift modifiers for non-ASCii codes, except for K_SP and bellow, 
       and except when other modifiers are used */
    if ((wincode < K_exclam || wincode > K_tilde) ||
        (has_ctrl || has_alt || has_sys))
      code = iup_XkeyShift(code);  
  }

  if (has_ctrl)   /* Ctrl */
    code = iup_XkeyCtrl(code);

  if (has_alt)    /* Alt */
    code = iup_XkeyAlt(code);

  if (has_sys)    /* Apple/Win */
    code = iup_XkeySys(code);

  return code;
}

#define win_ischar(_c)  (_c >= 0x41 && _c <= 0x5A)

static int winKeyAdjust(int wincode)
{
  if (wincode == VK_SHIFT && (GetKeyState(VK_RSHIFT) & 0x8000))
    return K_RSHIFT;
  else if (wincode == VK_CONTROL && (GetKeyState(VK_RCONTROL) & 0x8000))
    return K_RCTRL;
  else if (wincode == VK_MENU && (GetKeyState(VK_RMENU) & 0x8000))
    return K_RALT;
  else
  {
    int has_caps = GetKeyState(VK_CAPITAL) & 0x01;
    int has_shift = GetKeyState(VK_SHIFT) & 0x8000;
    int has_ctrl = GetKeyState(VK_CONTROL) & 0x8000;
    int has_alt = GetKeyState(VK_MENU) & 0x8000;
    int is_char = win_ischar(wincode);

    /* Caps Locks changes Shift modifier if is a character */
    if (has_caps && is_char)
    {
      if (has_shift)
        has_shift = 0;
      else
        has_shift = 1;
    }

    if (has_shift && winkey_map[wincode].shift_iupcode)
      return winkey_map[wincode].shift_iupcode;

    if (has_ctrl && has_alt && winkey_map[wincode].altgr_iupcode)
      return winkey_map[wincode].altgr_iupcode;

    return winkey_map[wincode].iupcode;
  }
}

int iupwinKeyDecode(int wincode)
{
  int iupcode = winKeyAdjust(wincode);
  if (!iupcode)
    iupcode=wincode;

  return winKeyMap2Iup(iupcode);
}

int iupwinKeyEvent(Ihandle* ih, int wincode, int press)
{
  int result, code;

  if (!ih->iclass->is_interactive)
    return 1;

  code = iupwinKeyDecode(wincode);
  if (code == 0)
    return 1;

  if (press)
  {
    result = iupKeyCallKeyCb(ih, code);
    if (result == IUP_CLOSE)
    {
      IupExitLoop();
      return 1;
    }
    else if (result == IUP_IGNORE)
      return 0;

    /* in the previous callback the dialog could be destroyed */
    if (iupObjectCheck(ih))
    {
      /* this is called only for canvas */
      if (ih->iclass->nativetype == IUP_TYPECANVAS) 
      {
        result = iupKeyCallKeyPressCb(ih, code, 1);
        if (result == IUP_CLOSE)
        {
          IupExitLoop();
          return 1;
        }
        else if (result == IUP_IGNORE)
          return 0;
      }

      if ((GetKeyState(VK_MENU) & 0x8000) && wincode < 128) /* Alt + mnemonic */
      {
        if (iupKeyProcessMnemonic(ih, wincode))
          return 0;
      }

      if (iupKeyProcessNavigation(ih, code, (GetKeyState(VK_SHIFT) & 0x8000)))
        return 0;
    }
  }
  else
  {
    /* this is called only for canvas */
    if (ih->iclass->nativetype == IUP_TYPECANVAS)
    {
      result = iupKeyCallKeyPressCb(ih, code, 0);
      if (result == IUP_CLOSE)
      {
        IupExitLoop();
        return 1;
      }
      else if (result == IUP_IGNORE)
        return 0;
    }
  }

  return 1;
}

void iupwinButtonKeySetStatus(WORD keys, char* status, int doubleclick)
{
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
}
