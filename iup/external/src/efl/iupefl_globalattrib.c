/** \file
 * \brief EFL Global Attributes
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Ecore_Input.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_singleinstance.h"

#include "iupefl_drv.h"


static Ecore_Event_Handler* efl_h_key_down = NULL;
static Ecore_Event_Handler* efl_h_key_up = NULL;
static Ecore_Event_Handler* efl_h_btn_down = NULL;
static Ecore_Event_Handler* efl_h_btn_up = NULL;
static Ecore_Event_Handler* efl_h_move = NULL;
static Ecore_Event_Handler* efl_h_wheel = NULL;

static void eflGlobalFillStatus(unsigned int mods, unsigned int button, char* status, int doubleclick)
{
  if (mods & ECORE_EVENT_MODIFIER_SHIFT) iupKEY_SETSHIFT(status);
  if (mods & ECORE_EVENT_MODIFIER_CTRL)  iupKEY_SETCONTROL(status);
  if (mods & ECORE_EVENT_MODIFIER_ALT)   iupKEY_SETALT(status);
  if (mods & ECORE_EVENT_MODIFIER_WIN)   iupKEY_SETSYS(status);
  if (button == 1) iupKEY_SETBUTTON1(status);
  if (button == 2) iupKEY_SETBUTTON2(status);
  if (button == 3) iupKEY_SETBUTTON3(status);
  if (button == 4) iupKEY_SETBUTTON4(status);
  if (button == 5) iupKEY_SETBUTTON5(status);
  if (doubleclick) iupKEY_SETDOUBLE(status);
}

static int eflApplyMods(int code, unsigned int mods)
{
  if (mods & (ECORE_EVENT_MODIFIER_CTRL | ECORE_EVENT_MODIFIER_ALT | ECORE_EVENT_MODIFIER_WIN))
  {
    if (code >= K_a && code <= K_z) code = iup_toupper(code);
    else if (code == K_ccedilla)    code = K_Ccedilla;
  }
  if (mods & ECORE_EVENT_MODIFIER_SHIFT)
  {
    if ((code < K_exclam || code > K_tilde) ||
        (mods & (ECORE_EVENT_MODIFIER_CTRL | ECORE_EVENT_MODIFIER_ALT | ECORE_EVENT_MODIFIER_WIN)))
      code = iup_XkeyShift(code);
  }
  if (mods & ECORE_EVENT_MODIFIER_CTRL) code = iup_XkeyCtrl(code);
  if (mods & ECORE_EVENT_MODIFIER_ALT)  code = iup_XkeyAlt(code);
  if (mods & ECORE_EVENT_MODIFIER_WIN)  code = iup_XkeySys(code);
  return code;
}

static Eina_Bool eflGlobalKey(void* data, int type, void* event)
{
  (void)data;
  IFii cb = (IFii)IupGetFunction("GLOBALKEYPRESS_CB");
  if (cb)
  {
    Ecore_Event_Key* k = (Ecore_Event_Key*)event;
    int code = iupeflKeyDecodeFromName(k->keyname, k->string);
    if (code != 0)
    {
      code = eflApplyMods(code, k->modifiers);
      cb(code, type == ECORE_EVENT_KEY_DOWN ? 1 : 0);
    }
  }
  return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool eflGlobalButton(void* data, int type, void* event)
{
  (void)data;
  IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
  if (cb)
  {
    Ecore_Event_Mouse_Button* b = (Ecore_Event_Mouse_Button*)event;
    int button = 0;
    switch (b->buttons)
    {
      case 1: button = IUP_BUTTON1; break;
      case 2: button = IUP_BUTTON2; break;
      case 3: button = IUP_BUTTON3; break;
      case 4: button = IUP_BUTTON4; break;
      case 5: button = IUP_BUTTON5; break;
      default: return ECORE_CALLBACK_PASS_ON;
    }
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    eflGlobalFillStatus(b->modifiers, button, status, b->double_click);
    cb(button, type == ECORE_EVENT_MOUSE_BUTTON_DOWN ? 1 : 0, b->root.x, b->root.y, status);
  }
  return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool eflGlobalMove(void* data, int type, void* event)
{
  (void)data; (void)type;
  IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
  if (cb)
  {
    Ecore_Event_Mouse_Move* m = (Ecore_Event_Mouse_Move*)event;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    eflGlobalFillStatus(m->modifiers, 0, status, 0);
    cb(m->root.x, m->root.y, status);
  }
  return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool eflGlobalWheel(void* data, int type, void* event)
{
  (void)data; (void)type;
  IFfiis cb = (IFfiis)IupGetFunction("GLOBALWHEEL_CB");
  if (cb)
  {
    Ecore_Event_Mouse_Wheel* w = (Ecore_Event_Mouse_Wheel*)event;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    eflGlobalFillStatus(w->modifiers, 0, status, 0);
    cb((float)-w->z, w->root.x, w->root.y, status);
  }
  return ECORE_CALLBACK_PASS_ON;
}

static void eflToggleGlobalInput(int enabled)
{
  if (enabled)
  {
    if (!efl_h_key_down) efl_h_key_down = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,         eflGlobalKey,    NULL);
    if (!efl_h_key_up)   efl_h_key_up   = ecore_event_handler_add(ECORE_EVENT_KEY_UP,           eflGlobalKey,    NULL);
    if (!efl_h_btn_down) efl_h_btn_down = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, eflGlobalButton, NULL);
    if (!efl_h_btn_up)   efl_h_btn_up   = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,   eflGlobalButton, NULL);
    if (!efl_h_move)     efl_h_move     = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,        eflGlobalMove,   NULL);
    if (!efl_h_wheel)    efl_h_wheel    = ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL,       eflGlobalWheel,  NULL);
  }
  else
  {
    if (efl_h_key_down) { ecore_event_handler_del(efl_h_key_down); efl_h_key_down = NULL; }
    if (efl_h_key_up)   { ecore_event_handler_del(efl_h_key_up);   efl_h_key_up   = NULL; }
    if (efl_h_btn_down) { ecore_event_handler_del(efl_h_btn_down); efl_h_btn_down = NULL; }
    if (efl_h_btn_up)   { ecore_event_handler_del(efl_h_btn_up);   efl_h_btn_up   = NULL; }
    if (efl_h_move)     { ecore_event_handler_del(efl_h_move);     efl_h_move     = NULL; }
    if (efl_h_wheel)    { ecore_event_handler_del(efl_h_wheel);    efl_h_wheel    = NULL; }
  }
}


static char* efl_theme_path = NULL;
static const void* efl_theme_data = NULL;
static size_t efl_theme_data_len = 0;

IUP_DRV_API int iupeflIsSystemDarkMode(void)
{
  Evas_Object* temp_win;
  Evas_Object* bg;
  Evas_Object* edje;
  Evas_Object* base_part;
  int r = 240, g = 240, b = 240, a = 255;
  double lum;
  int is_dark;

  temp_win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                     efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC));
  if (!temp_win)
    return 0;

  bg = elm_bg_add(temp_win);
  efl_gfx_entity_size_set(bg, EINA_SIZE2D(100, 100));
  efl_gfx_entity_visible_set(bg, EINA_TRUE);

  if (efl_isa(bg, EFL_UI_LAYOUT_BASE_CLASS))
    edje = elm_layout_edje_get(bg);
  else
    edje = NULL;
  if (edje)
  {
    base_part = (Evas_Object*)edje_object_part_object_get(edje, "base");
    if (base_part)
      efl_gfx_color_get(base_part, &r, &g, &b, &a);
  }

  efl_del(temp_win);

  lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
  is_dark = (lum < 128.0) ? 1 : 0;

  return is_dark;
}

static void eflApplyGlobalFontOverlays(const char* font)
{
  char typeface[256];
  int size = 0;
  int is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  char efl_font[280];

  if (!font || !font[0])
    return;

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return;

  if (is_bold && is_italic)
    snprintf(efl_font, sizeof(efl_font), "%s:style=Bold Italic", typeface);
  else if (is_bold)
    snprintf(efl_font, sizeof(efl_font), "%s:style=Bold", typeface);
  else if (is_italic)
    snprintf(efl_font, sizeof(efl_font), "%s:style=Italic", typeface);
  else
    strncpy(efl_font, typeface, sizeof(efl_font) - 1);

  /* Apply to ALL text classes used by Elementary default theme.
     Note: Default button/check/frame have NO text_class - hardcoded fonts. */

  /* Entry/Text widgets */
  elm_config_font_overlay_set("entry_text", efl_font, size);
  elm_config_font_overlay_set("entry_text_disabled", efl_font, size);
  elm_config_font_overlay_set("entry_guide_text", efl_font, size);

  /* List/Genlist */
  elm_config_font_overlay_set("list_item", efl_font, size);
  elm_config_font_overlay_set("list_item_sub", efl_font, size);
  elm_config_font_overlay_set("list_item_selected", efl_font, size);
  elm_config_font_overlay_set("list_group_item", efl_font, size);
  elm_config_font_overlay_set("grid_item", efl_font, size);

  /* Toolbar/Tabs */
  elm_config_font_overlay_set("toolbar_item", efl_font, size);

  /* Menu */
  elm_config_font_overlay_set("menu_item", efl_font, size);

  /* Title bar */
  elm_config_font_overlay_set("title_bar", efl_font, size);
  elm_config_font_overlay_set("title_bar_sub", efl_font, size);

  /* Toggle switch style (not default checkbox) */
  elm_config_font_overlay_set("check_off_text", efl_font, size);
  elm_config_font_overlay_set("check_on_text", efl_font, size);
  elm_config_font_overlay_set("radio", efl_font, size);

  /* Slider */
  elm_config_font_overlay_set("slider", efl_font, size);
  elm_config_font_overlay_set("slider_text", efl_font, size);
  elm_config_font_overlay_set("slider_indicator", efl_font, size);

  /* Spinner */
  elm_config_font_overlay_set("spinner", efl_font, size);

  /* Progress */
  elm_config_font_overlay_set("progressbar", efl_font, size);
  elm_config_font_overlay_set("progress_status", efl_font, size);

  /* Bubble */
  elm_config_font_overlay_set("bubble", efl_font, size);
  elm_config_font_overlay_set("bubble_info", efl_font, size);

  /* Index */
  elm_config_font_overlay_set("index_item_text", efl_font, size);
  elm_config_font_overlay_set("index_highlight_text", efl_font, size);

  /* Hoversel/Button variants */
  elm_config_font_overlay_set("butt", efl_font, size);
  elm_config_font_overlay_set("button", efl_font, size);
  elm_config_font_overlay_set("label", efl_font, size);

  /* Calendar */
  elm_config_font_overlay_set("calendar_weekday_text", efl_font, size);
  elm_config_font_overlay_set("calendar_day_text", efl_font, size);

  elm_config_font_overlay_apply();
}

IUP_SDK_API int iupdrvSetGlobal(const char* name, const char* value)
{
  if (iupStrEqual(name, "SINGLEINSTANCE"))
  {
    if (iupdrvSingleInstanceSet(value))
      return 0;
    else
      return 1;
  }
  if (iupStrEqual(name, "EFLTHEME"))
  {
    if (!efl_theme_path && value && value[0])
    {
      efl_theme_path = strdup(value);
      elm_theme_set(NULL, efl_theme_path);
      iupeflSetGlobalColors();
      return 1;
    }
    return 0;
  }

  if (iupStrEqual(name, "EFLTHEMEDATA"))
  {
    if (!efl_theme_data && value)
    {
      efl_theme_data = (const void*)value;
      if (efl_theme_data_len > 0)
      {
        Eina_File* vf = eina_file_virtualize("iup_embedded_theme.edj", efl_theme_data, efl_theme_data_len, EINA_FALSE);
        if (vf)
          elm_theme_overlay_mmap_add(NULL, vf);
      }
      return 1;
    }
    return 0;
  }

  if (iupStrEqual(name, "EFLTHEMEDATALEN"))
  {
    if (efl_theme_data_len == 0 && value)
    {
      efl_theme_data_len = (size_t)strtoull(value, NULL, 10);
      if (efl_theme_data && efl_theme_data_len > 0)
      {
        Eina_File* vf = eina_file_virtualize("iup_embedded_theme.edj", efl_theme_data, efl_theme_data_len, EINA_FALSE);
        if (vf)
          elm_theme_overlay_mmap_add(NULL, vf);
      }
      return 1;
    }
    return 0;
  }

  if (iupStrEqual(name, "EFLACCEL"))
  {
    if (value && value[0])
    {
      elm_config_accel_preference_set(value);
      return 1;
    }
    return 0;
  }

  if (iupStrEqual(name, "DEFAULTFONT"))
  {
    eflApplyGlobalFontOverlays(value);
    return 1;
  }

  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    eflToggleGlobalInput(iupStrBoolean(value));
    return 1;
  }
  if (iupStrEqual(name, "LANGUAGE"))
  {
    return 1;
  }
  if (iupStrEqual(name, "CURSORPOS"))
  {
    return 0;
  }
  if (iupStrEqual(name, "AUTOREPEAT"))
  {
    return 1;
  }
  if (iupStrEqual(name, "UTF8MODE"))
  {
    return 1;
  }
  if (iupStrEqual(name, "SHOWMENUIMAGES"))
  {
    return 1;
  }

  return 1;
}

IUP_SDK_API char* iupdrvGetGlobal(const char* name)
{
  if (iupStrEqual(name, "VIRTUALSCREEN"))
  {
    int w, h;
    iupdrvGetScreenSize(&w, &h);
    return iupStrReturnIntInt(w, h, 'x');
  }
  if (iupStrEqual(name, "MONITORSINFO"))
  {
    int w, h;
    iupdrvGetScreenSize(&w, &h);
    return iupStrReturnStrf("0 0 %d %d\n", w, h);
  }
  if (iupStrEqual(name, "MONITORSCOUNT"))
  {
    return "1";
  }
  if (iupStrEqual(name, "TRUECOLORCANVAS"))
  {
    return "YES";
  }
  if (iupStrEqual(name, "UTF8MODE"))
  {
    return "YES";
  }
  if (iupStrEqual(name, "DRIVER"))
  {
    return "EFL";
  }
  if (iupStrEqual(name, "SYSTEMLANGUAGE"))
  {
    return iupdrvLocaleInfo();
  }
  if (iupStrEqual(name, "DWM_COMPOSITION"))
  {
    return "YES";
  }
#ifndef _WIN32
  if (iupStrEqual(name, "EXEFILENAME"))
  {
    char* argv0 = IupGetGlobal("ARGV0");
    if (argv0)
    {
      char* exefilename = realpath(argv0, NULL);
      if (exefilename)
      {
        char* str = iupStrReturnStr(exefilename);
        free(exefilename);
        return str;
      }
    }
    return NULL;
  }
#endif
  if (iupStrEqual(name, "EFLACCEL"))
  {
    const char* accel = elm_config_accel_preference_get();
    if (accel)
      return iupStrReturnStr(accel);
    return NULL;
  }
  if (iupStrEqual(name, "EFLENGINE"))
  {
    Eo* win = iupeflGetMainWindow();
    if (win)
    {
      Evas* evas = evas_object_evas_get(win);
      if (evas)
      {
        Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas);
        if (ee)
          return iupStrReturnStr(ecore_evas_engine_name_get(ee));
      }
    }
    return NULL;
  }
  if (iupStrEqual(name, "DARKMODE"))
  {
    return iupStrReturnBoolean(iupeflIsSystemDarkMode());
  }
  if (iupStrEqual(name, "SANDBOX"))
  {
    if (getenv("FLATPAK_ID"))
      return "FLATPAK";
    if (getenv("SNAP"))
      return "SNAP";
    if (getenv("APPIMAGE"))
      return "APPIMAGE";
    return NULL;
  }

  return NULL;
}

