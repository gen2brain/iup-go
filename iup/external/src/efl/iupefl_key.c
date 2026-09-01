/** \file
 * \brief EFL Driver keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_drv.h"

#include <Ecore_IMF.h>

#include "iupefl_drv.h"


IUP_SDK_API void iupdrvKeyEncode(int code, unsigned int *keyval, unsigned int *state)
{
  *keyval = (unsigned int)iup_XkeyBase(code);
  *state = 0;
}

IUP_DRV_API int iupeflKeyDecodeFromName(const char* keyname, const char* keysym, const char* keystr)
{
  if (!keyname)
    return 0;

  if (keysym && strncmp(keysym, "KP_", 3) == 0)
    keyname = keysym;

  if (strcmp(keyname, "Return") == 0) return K_CR;
  if (strcmp(keyname, "KP_Enter") == 0) return K_KP_CR;
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

  /* Keypad mappings */
  if (strcmp(keyname, "KP_0") == 0) return K_KP_0;
  if (strcmp(keyname, "KP_1") == 0) return K_KP_1;
  if (strcmp(keyname, "KP_2") == 0) return K_KP_2;
  if (strcmp(keyname, "KP_3") == 0) return K_KP_3;
  if (strcmp(keyname, "KP_4") == 0) return K_KP_4;
  if (strcmp(keyname, "KP_5") == 0) return K_KP_5;
  if (strcmp(keyname, "KP_6") == 0) return K_KP_6;
  if (strcmp(keyname, "KP_7") == 0) return K_KP_7;
  if (strcmp(keyname, "KP_8") == 0) return K_KP_8;
  if (strcmp(keyname, "KP_9") == 0) return K_KP_9;
  if (strcmp(keyname, "KP_Multiply") == 0) return K_KP_MULT;
  if (strcmp(keyname, "KP_Add") == 0) return K_KP_PLUS;
  if (strcmp(keyname, "KP_Subtract") == 0) return K_KP_MINUS;
  if (strcmp(keyname, "KP_Decimal") == 0) return K_KP_DECIMAL;
  if (strcmp(keyname, "KP_Divide") == 0) return K_KP_DIV;
  if (strcmp(keyname, "KP_Separator") == 0) return K_KP_SEP;
  if (strcmp(keyname, "KP_Equal") == 0) return K_KP_EQUAL;
  if (strcmp(keyname, "KP_Space") == 0) return K_SP;
  if (strcmp(keyname, "KP_Tab") == 0) return K_TAB;
  if (strcmp(keyname, "KP_Home") == 0) return K_KP_HOME;
  if (strcmp(keyname, "KP_Up") == 0) return K_KP_UP;
  if (strcmp(keyname, "KP_Page_Up") == 0) return K_KP_PGUP;
  if (strcmp(keyname, "KP_Prior") == 0) return K_KP_PGUP;
  if (strcmp(keyname, "KP_Left") == 0) return K_KP_LEFT;
  if (strcmp(keyname, "KP_Begin") == 0) return K_KP_MIDDLE;
  if (strcmp(keyname, "KP_Right") == 0) return K_KP_RIGHT;
  if (strcmp(keyname, "KP_End") == 0) return K_KP_END;
  if (strcmp(keyname, "KP_Down") == 0) return K_KP_DOWN;
  if (strcmp(keyname, "KP_Page_Down") == 0) return K_KP_PGDN;
  if (strcmp(keyname, "KP_Next") == 0) return K_KP_PGDN;
  if (strcmp(keyname, "KP_Insert") == 0) return K_KP_INS;
  if (strcmp(keyname, "KP_Delete") == 0) return K_KP_DEL;

  /* Dead keys */
  if (strcmp(keyname, "dead_tilde") == 0) return K_tilde;
  if (strcmp(keyname, "dead_acute") == 0) return K_acute;
  if (strcmp(keyname, "dead_grave") == 0) return K_grave;
  if (strcmp(keyname, "dead_circumflex") == 0) return K_circum;
  if (strcmp(keyname, "dead_diaeresis") == 0) return K_diaeresis;

  /* Ctrl+letter leaves the string empty or a control byte, the symbol still names the letter */
  if (keysym && keysym[0] && !keysym[1])
    return (int)(unsigned char)keysym[0];

  if (keystr && keystr[0] && !keystr[1])
    return (int)(unsigned char)keystr[0];

  return 0;
}

static int eflKeyApplyModifiers(int code, Efl_Input_Key* key_event)
{
  int has_ctrl = efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_CONTROL, NULL);
  int has_alt = efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_ALT, NULL);
  int has_sys = efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_META, NULL) ||
                efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_SUPER, NULL);
  int has_shift = efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_SHIFT, NULL);

  if (has_ctrl || has_alt || has_sys)
  {
    if (code >= K_a && code <= K_z)
      code = iup_toupper(code);
    else if (code == K_ccedilla)
      code = K_Ccedilla;
  }

  if (has_shift)
  {
    if ((code < K_exclam || code > K_tilde) || has_ctrl || has_alt || has_sys)
      code = iup_XkeyShift(code);
  }

  if (has_ctrl)
    code = iup_XkeyCtrl(code);

  if (has_alt)
    code = iup_XkeyAlt(code);

  if (has_sys)
    code = iup_XkeySys(code);

  return code;
}

IUP_DRV_API void iupeflKeyEncode(int key, const char** keyname, const char** keystr)
{
  (void)key;
  *keyname = NULL;
  *keystr = NULL;
}

IUP_DRV_API void iupeflButtonKeySetStatus(Evas_Modifier* modifiers, unsigned int button, char* status, int doubleclick)
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

static int efl_imf_committed = 0;
static int efl_imf_ignored = 0;

/* a commit consumed by TEXTINPUT_CB suppresses the K_ANY for that key */
static int eflKeyTextInputStr(Ihandle* ih, const char* keystr)
{
  if (!IupGetCallback(ih, "TEXTINPUT_CB"))
    return 0;
  if (!keystr || !keystr[0])
    return 0;
  if (!keystr[1] && ((unsigned char)keystr[0] < 0x20 || (unsigned char)keystr[0] == 0x7F))
    return 0;
  return iupKeyCallTextInputCb(ih, keystr) == IUP_IGNORE;
}

static int eflKeyTextInput(Ihandle* ih, Efl_Input_Key* key_event, const char* keystr)
{
  if (efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_CONTROL, NULL) ||
      efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_ALT, NULL))
    return 0;
  return eflKeyTextInputStr(ih, keystr);
}

static void eflKeyImfCommitEvent(void* data, Ecore_IMF_Context* ctx, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  (void)ctx;
  if (!iupObjectCheck(ih))
    return;
  efl_imf_committed = 1;
  if (eflKeyTextInputStr(ih, (const char*)event_info))
    efl_imf_ignored = 1;
}

static Ecore_IMF_Context* eflKeyImfContext(Ihandle* ih, Eo* widget)
{
  Ecore_IMF_Context* imf = (Ecore_IMF_Context*)iupAttribGet(ih, "_IUPEFL_IMF");
  const Ecore_IMF_Context_Info* info;
  const char* id;
  Evas* evas;
  Ecore_Evas* ee;

  if (imf)
    return imf;
  if (iupAttribGet(ih, "_IUPEFL_NOIMF"))
    return NULL;

  id = ecore_imf_context_default_id_get();
  if (id)
  {
    info = ecore_imf_context_info_by_id_get(id);
    if (info && info->canvas_type && !iupStrEqual(info->canvas_type, "evas"))
      id = ecore_imf_context_default_id_by_canvas_type_get("evas");
  }

  imf = id ? ecore_imf_context_add(id) : NULL;
  if (!imf)
  {
    iupAttribSet(ih, "_IUPEFL_NOIMF", "1");
    return NULL;
  }

  evas = widget ? evas_object_evas_get(widget) : NULL;
  ee = evas ? ecore_evas_ecore_evas_get(evas) : NULL;
  if (ee)
    ecore_imf_context_client_window_set(imf, (void*)(uintptr_t)ecore_evas_window_get(ee));
  if (evas)
    ecore_imf_context_client_canvas_set(imf, evas);

  ecore_imf_context_event_callback_add(imf, ECORE_IMF_CALLBACK_COMMIT, eflKeyImfCommitEvent, ih);
  ecore_imf_context_focus_in(imf);

  iupAttribSet(ih, "_IUPEFL_IMF", (char*)imf);
  return imf;
}

IUP_DRV_API void iupeflKeyImfDestroy(Ihandle* ih)
{
  Ecore_IMF_Context* imf = (Ecore_IMF_Context*)iupAttribGet(ih, "_IUPEFL_IMF");
  if (!imf)
    return;
  ecore_imf_context_focus_out(imf);
  ecore_imf_context_event_callback_del(imf, ECORE_IMF_CALLBACK_COMMIT, eflKeyImfCommitEvent);
  ecore_imf_context_del(imf);
  iupAttribSet(ih, "_IUPEFL_IMF", NULL);
}

static Ecore_IMF_Keyboard_Modifiers eflKeyImfModifiers(Efl_Input_Key* key_event)
{
  Ecore_IMF_Keyboard_Modifiers mod = ECORE_IMF_KEYBOARD_MODIFIER_NONE;
  if (efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_CONTROL, NULL)) mod |= ECORE_IMF_KEYBOARD_MODIFIER_CTRL;
  if (efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_ALT, NULL))     mod |= ECORE_IMF_KEYBOARD_MODIFIER_ALT;
  if (efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_SHIFT, NULL))   mod |= ECORE_IMF_KEYBOARD_MODIFIER_SHIFT;
  if (efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_SUPER, NULL))   mod |= ECORE_IMF_KEYBOARD_MODIFIER_WIN;
  if (efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_ALTGR, NULL))   mod |= ECORE_IMF_KEYBOARD_MODIFIER_ALTGR;
  return mod;
}

static Ecore_IMF_Keyboard_Locks eflKeyImfLocks(Efl_Input_Key* key_event)
{
  Ecore_IMF_Keyboard_Locks locks = ECORE_IMF_KEYBOARD_LOCK_NONE;
  if (efl_input_lock_enabled_get(key_event, EFL_INPUT_LOCK_NUM, NULL))    locks |= ECORE_IMF_KEYBOARD_LOCK_NUM;
  if (efl_input_lock_enabled_get(key_event, EFL_INPUT_LOCK_CAPS, NULL))   locks |= ECORE_IMF_KEYBOARD_LOCK_CAPS;
  if (efl_input_lock_enabled_get(key_event, EFL_INPUT_LOCK_SCROLL, NULL)) locks |= ECORE_IMF_KEYBOARD_LOCK_SCROLL;
  return locks;
}

/* returns 1 when the key must not reach K_ANY */
static int eflKeyImfTextInput(Ihandle* ih, Eo* widget, Efl_Input_Key* key_event,
                              const char* keyname, const char* keystr)
{
  Ecore_IMF_Context* imf;
  Ecore_IMF_Event_Key_Down imf_ev;
  Eina_Bool filtered;

  if (!IupGetCallback(ih, "TEXTINPUT_CB"))
    return 0;
  if (iup_isKeyPadXkey(iupeflKeyDecodeFromName(keyname, efl_input_key_sym_get(key_event), keystr)))
    return 0;

  imf = eflKeyImfContext(ih, widget);
  if (!imf)
    return eflKeyTextInput(ih, key_event, keystr);

  memset(&imf_ev, 0, sizeof(imf_ev));
  imf_ev.keyname = keyname;
  imf_ev.key = efl_input_key_sym_get(key_event);
  imf_ev.string = keystr;
  imf_ev.compose = efl_input_key_compose_string_get(key_event);
  imf_ev.timestamp = (unsigned int)efl_input_timestamp_get(key_event);
  imf_ev.keycode = (unsigned int)efl_input_key_code_get(key_event);
  imf_ev.modifiers = eflKeyImfModifiers(key_event);
  imf_ev.locks = eflKeyImfLocks(key_event);

  efl_imf_committed = 0;
  efl_imf_ignored = 0;
  filtered = ecore_imf_context_filter_event(imf, ECORE_IMF_EVENT_KEY_DOWN, (Ecore_IMF_Event*)&imf_ev);

  if (efl_imf_ignored)
    return 1;
  if (efl_imf_committed)
    return 0;
  if (filtered)
    return 1;

  return eflKeyTextInput(ih, key_event, keystr);
}

IUP_DRV_API void iupeflKeyDownEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Key* key_event = ev->info;
  const char* keyname = efl_input_key_name_get(key_event);
  const char* keystr = efl_input_key_string_get(key_event);
  int code;
  int result;
  int has_shift;

  if (eflKeyImfTextInput(ih, ev->object, key_event, keyname, keystr))
    return;

  code = iupeflKeyDecodeFromName(keyname, efl_input_key_sym_get(key_event), keystr);
  if (code == 0)
    return;

  code = eflKeyApplyModifiers(code, key_event);

  has_shift = efl_input_modifier_enabled_get(key_event, EFL_INPUT_MODIFIER_SHIFT, NULL);

  result = iupKeyCallKeyCb(ih, code);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return;
  }
  if (result == IUP_IGNORE)
    return;

  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype == IUP_TYPECANVAS)
  {
    result = iupKeyCallKeyPressCb(ih, code, 1);
    if (result == IUP_CLOSE)
    {
      IupExitLoop();
      return;
    }
    if (result == IUP_IGNORE)
      return;
  }

  if (iupKeyProcessNavigation(ih, code, has_shift))
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

IUP_DRV_API void iupeflKeyUpEvent(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Key* key_event = ev->info;

  if (ih->iclass->nativetype == IUP_TYPECANVAS)
  {
    const char* keyname = efl_input_key_name_get(key_event);
    const char* keystr = efl_input_key_string_get(key_event);
    int code = iupeflKeyDecodeFromName(keyname, efl_input_key_sym_get(key_event), keystr);
    if (code != 0)
    {
      code = eflKeyApplyModifiers(code, key_event);
      iupKeyCallKeyPressCb(ih, code, 0);
    }
  }
}
