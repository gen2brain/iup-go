/** \file
 * \brief EFL Global Attributes
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"

#include "iupefl_drv.h"


static char* efl_app_name = NULL;
static char* efl_theme_path = NULL;
static const void* efl_theme_data = NULL;
static size_t efl_theme_data_len = 0;

int iupeflIsSystemDarkMode(void)
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

int iupdrvSetGlobal(const char* name, const char* value)
{
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

  if (iupStrEqual(name, "DEFAULTFONT"))
  {
    eflApplyGlobalFontOverlays(value);
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
  if (iupStrEqual(name, "GLOBALMENU"))
  {
    return 1;
  }
  if (iupStrEqual(name, "IABORTWITHWARNING"))
  {
    return 1;
  }

  return 1;
}

char* iupdrvGetGlobal(const char* name)
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
  if (iupStrEqual(name, "DARKMODE"))
  {
    return iupStrReturnBoolean(iupeflIsSystemDarkMode());
  }

  return NULL;
}

int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  (void)value;
  return 1;
}

int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  if (efl_app_name)
    free(efl_app_name);

  if (value)
    efl_app_name = strdup(value);
  else
    efl_app_name = NULL;

  return 0;
}
