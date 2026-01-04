/** \file
 * \brief EFL Driver keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_drv.h"

#include "iupefl_drv.h"


void iupdrvKeyEncode(int code, unsigned int *keyval, unsigned int *state)
{
  *keyval = (unsigned int)iup_XkeyBase(code);
  *state = 0;
}

static int eflKeyDecodeFromName(const char* keyname, const char* keystr)
{
  if (!keyname)
    return 0;

  if (strcmp(keyname, "Return") == 0) return K_CR;
  if (strcmp(keyname, "KP_Enter") == 0) return K_CR;
  if (strcmp(keyname, "Escape") == 0) return K_ESC;
  if (strcmp(keyname, "BackSpace") == 0) return K_BS;
  if (strcmp(keyname, "Tab") == 0) return K_TAB;
  if (strcmp(keyname, "space") == 0) return K_SP;
  if (strcmp(keyname, "Delete") == 0) return K_DEL;
  if (strcmp(keyname, "Insert") == 0) return K_INS;
  if (strcmp(keyname, "Home") == 0) return K_HOME;
  if (strcmp(keyname, "End") == 0) return K_END;
  if (strcmp(keyname, "Prior") == 0) return K_PGUP;
  if (strcmp(keyname, "Page_Up") == 0) return K_PGUP;
  if (strcmp(keyname, "Next") == 0) return K_PGDN;
  if (strcmp(keyname, "Page_Down") == 0) return K_PGDN;
  if (strcmp(keyname, "Up") == 0) return K_UP;
  if (strcmp(keyname, "Down") == 0) return K_DOWN;
  if (strcmp(keyname, "Left") == 0) return K_LEFT;
  if (strcmp(keyname, "Right") == 0) return K_RIGHT;
  if (strcmp(keyname, "F1") == 0) return K_F1;
  if (strcmp(keyname, "F2") == 0) return K_F2;
  if (strcmp(keyname, "F3") == 0) return K_F3;
  if (strcmp(keyname, "F4") == 0) return K_F4;
  if (strcmp(keyname, "F5") == 0) return K_F5;
  if (strcmp(keyname, "F6") == 0) return K_F6;
  if (strcmp(keyname, "F7") == 0) return K_F7;
  if (strcmp(keyname, "F8") == 0) return K_F8;
  if (strcmp(keyname, "F9") == 0) return K_F9;
  if (strcmp(keyname, "F10") == 0) return K_F10;
  if (strcmp(keyname, "F11") == 0) return K_F11;
  if (strcmp(keyname, "F12") == 0) return K_F12;
  if (strcmp(keyname, "Print") == 0) return K_Print;
  if (strcmp(keyname, "Pause") == 0) return K_PAUSE;

  if (keystr && keystr[0] && !keystr[1])
    return (int)(unsigned char)keystr[0];

  return 0;
}

int iupeflKeyDecode(Evas_Event_Key_Down* ev)
{
  if (!ev)
    return 0;
  return eflKeyDecodeFromName(ev->keyname, ev->string);
}

void iupeflKeyEncode(int key, const char** keyname, const char** keystr)
{
  (void)key;
  *keyname = NULL;
  *keystr = NULL;
}

void iupeflButtonKeySetStatus(Evas_Modifier* modifiers, unsigned int button, char* status, int doubleclick)
{
  if (modifiers)
  {
    if (evas_key_modifier_is_set(modifiers, "Shift"))
      iupKEY_SETSHIFT(status);
    if (evas_key_modifier_is_set(modifiers, "Control"))
      iupKEY_SETCONTROL(status);
    if (evas_key_modifier_is_set(modifiers, "Alt"))
      iupKEY_SETALT(status);
    if (evas_key_modifier_is_set(modifiers, "Super") || evas_key_modifier_is_set(modifiers, "Meta"))
      iupKEY_SETSYS(status);
  }

  if (button == 1)
    iupKEY_SETBUTTON1(status);
  if (button == 2)
    iupKEY_SETBUTTON2(status);
  if (button == 3)
    iupKEY_SETBUTTON3(status);
  if (button == 4)
    iupKEY_SETBUTTON4(status);
  if (button == 5)
    iupKEY_SETBUTTON5(status);

  if (doubleclick)
    iupKEY_SETDOUBLE(status);
}

void iupeflKeyDownEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Key* key_event = ev->info;
  const char* keyname = efl_input_key_name_get(key_event);
  const char* keystr = efl_input_key_string_get(key_event);
  int code;
  int result;

  code = eflKeyDecodeFromName(keyname, keystr);
  if (code == 0)
    return;

  result = iupKeyCallKeyCb(ih, code);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return;
  }

  if (ih->iclass->nativetype == IUP_TYPECANVAS)
  {
    result = iupKeyCallKeyPressCb(ih, code, 1);
    if (result == IUP_CLOSE)
    {
      IupExitLoop();
      return;
    }
  }

  if (iupKeyProcessNavigation(ih, code, 0))
    return;

  if (code == K_F1)
  {
    Icallback cb = IupGetCallback(ih, "HELP_CB");
    if (cb)
    {
      if (cb(ih) == IUP_CLOSE)
        IupExitLoop();
    }
  }
}

void iupeflKeyUpEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Key* key_event = ev->info;

  if (ih->iclass->nativetype == IUP_TYPECANVAS)
  {
    const char* keyname = efl_input_key_name_get(key_event);
    const char* keystr = efl_input_key_string_get(key_event);
    int code = eflKeyDecodeFromName(keyname, keystr);
    if (code != 0)
      iupKeyCallKeyPressCb(ih, code, 0);
  }
}
