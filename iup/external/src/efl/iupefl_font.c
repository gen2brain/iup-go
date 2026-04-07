/** \file
 * \brief EFL Font Management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_markup.h"

#include "iupefl_drv.h"


typedef struct _IeflFont
{
  char* font_name;
  int size;
  int is_bold;
  int is_italic;
  int is_underline;
  int is_strikeout;
  int charwidth;
  int charheight;
  int ascent;
  int descent;
} IeflFont;

static Eo* efl_font_measure_tb = NULL;
static Ecore_Evas* efl_font_buffer_ee = NULL;

static Evas* eflFontGetMeasureEvas(void)
{
  if (efl_font_buffer_ee)
    return ecore_evas_get(efl_font_buffer_ee);

  {
    Eo* win = iupeflGetMainWindow();
    if (win)
      return evas_object_evas_get(win);
  }

  efl_font_buffer_ee = ecore_evas_buffer_new(1, 1);
  if (efl_font_buffer_ee)
    return ecore_evas_get(efl_font_buffer_ee);

  return NULL;
}

static void eflFontApplyStyle(Eo* tb, const char* family, int size, int is_bold, int is_italic)
{
  char style[256];
  snprintf(style, sizeof(style), "font=%s font_size=%d font_weight=%s font_style=%s color=#000000",
           family, size,
           is_bold ? "bold" : "normal",
           is_italic ? "italic" : "normal");
  efl_canvas_textblock_style_apply(tb, style);
}

static void eflFontParse(const char* value, char* family, int* size, int* is_bold, int* is_italic, int* is_underline, int* is_strikeout)
{
  const char* p;

  iupStrCopyN(family, 100, "Sans");
  *size = 12;
  *is_bold = 0;
  *is_italic = 0;
  *is_underline = 0;
  *is_strikeout = 0;

  if (!value)
    return;

  p = strchr(value, ',');
  if (p)
  {
    int len = (int)(p - value);
    if (len > 0 && len < 100)
    {
      strncpy(family, value, len);
      family[len] = 0;
    }
    p++;

    while (*p)
    {
      while (*p == ' ') p++;

      if (strncasecmp(p, "Bold", 4) == 0)
      {
        *is_bold = 1;
        p += 4;
      }
      else if (strncasecmp(p, "Italic", 6) == 0)
      {
        *is_italic = 1;
        p += 6;
      }
      else if (strncasecmp(p, "Underline", 9) == 0)
      {
        *is_underline = 1;
        p += 9;
      }
      else if (strncasecmp(p, "Strikeout", 9) == 0)
      {
        *is_strikeout = 1;
        p += 9;
      }
      else if (*p >= '0' && *p <= '9')
      {
        *size = 0; iupStrToInt(p, size);
        while (*p >= '0' && *p <= '9') p++;
      }
      else
        p++;
    }
  }
  else
  {
    iupStrCopyN(family, 100, value);
  }
}

static Eo* eflFontGetMeasureTextblock(void)
{
  Evas* evas;

  if (efl_font_measure_tb)
    return efl_font_measure_tb;

  evas = eflFontGetMeasureEvas();
  if (!evas)
    return NULL;

  efl_font_measure_tb = efl_add(EFL_CANVAS_TEXTBLOCK_CLASS, evas);
  return efl_font_measure_tb;
}

static void eflFontMeasure(const char* family, int size, int is_bold, int is_italic, int* charwidth, int* charheight, int* ascent, int* descent)
{
  Eo* tb;
  Eina_Size2D sz;
  static const char* measure_str = "abcdefghijklmnopqrstuvwxyz";
  static const int measure_len = 26;

  tb = eflFontGetMeasureTextblock();
  if (!tb)
  {
    if (charwidth) *charwidth = size * 2 / 3;
    if (charheight) *charheight = size;
    if (ascent) *ascent = size * 3 / 4;
    if (descent) *descent = size / 4;
    return;
  }

  eflFontApplyStyle(tb, family, size, is_bold, is_italic);

  efl_text_set(tb, measure_str);
  sz = efl_canvas_textblock_size_native_get(tb);

  if (charwidth)
  {
    if (sz.w > 0)
      *charwidth = (sz.w + measure_len / 2) / measure_len;
    else
      *charwidth = size * 2 / 3;
  }

  if (charheight)
  {
    efl_text_set(tb, "X\nX");
    sz = efl_canvas_textblock_size_native_get(tb);
    *charheight = sz.h / 2;
    if (*charheight <= 0) *charheight = size;
  }

  if (ascent)
    *ascent = size * 3 / 4;
  if (descent)
    *descent = size / 4;
}

static void eflFontMeasureString(const char* family, int size, int is_bold, int is_italic, const char* str, int len, int* w, int* h)
{
  Eo* tb;
  Eina_Size2D sz;

  if (!str || !str[0] || len == 0)
  {
    if (w) *w = 0;
    if (h) *h = size;
    return;
  }

  if (len < 0)
    len = (int)strlen(str);

  tb = eflFontGetMeasureTextblock();
  if (!tb)
  {
    if (w) *w = len * (size * 2 / 3);
    if (h) *h = size;
    return;
  }

  eflFontApplyStyle(tb, family, size, is_bold, is_italic);

  if (str[len] == '\0')
  {
    efl_text_set(tb, str);
  }
  else
  {
    char* tmp = (char*)malloc(len + 1);
    memcpy(tmp, str, len);
    tmp[len] = '\0';
    efl_text_set(tb, tmp);
    free(tmp);
  }

  sz = efl_canvas_textblock_size_native_get(tb);

  if (w) *w = sz.w > 0 ? sz.w : len * (size * 2 / 3);
  if (h) *h = sz.h > 0 ? sz.h : size;
}

static IeflFont* eflFontGet(Ihandle* ih)
{
  IeflFont* font = (IeflFont*)iupAttribGet(ih, "_IUP_EFL_FONT");
  if (!font && ih)
  {
    char* font_str = iupGetFontValue(ih);
    if (!font_str)
      font_str = IupGetGlobal("DEFAULTFONT");
    if (font_str)
    {
      char family[100];
      int size, is_bold, is_italic, is_underline, is_strikeout;

      font = (IeflFont*)calloc(1, sizeof(IeflFont));
      iupAttribSet(ih, "_IUP_EFL_FONT", (char*)font);

      eflFontParse(font_str, family, &size, &is_bold, &is_italic, &is_underline, &is_strikeout);

      font->font_name = strdup(family);
      font->size = size;
      font->is_bold = is_bold;
      font->is_italic = is_italic;
      font->is_underline = is_underline;
      font->is_strikeout = is_strikeout;

      eflFontMeasure(family, size, is_bold, is_italic, &font->charwidth, &font->charheight, &font->ascent, &font->descent);
    }
  }
  return font;
}

IUP_DRV_API void iupeflFontFree(Ihandle* ih)
{
  IeflFont* font = (IeflFont*)iupAttribGet(ih, "_IUP_EFL_FONT");
  if (font)
  {
    if (font->font_name)
      free(font->font_name);
    free(font);
    iupAttribSet(ih, "_IUP_EFL_FONT", NULL);
  }
}

IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  IeflFont* font = eflFontGet(ih);
  char family[100];
  int size, is_bold, is_italic, is_underline, is_strikeout;

  if (!font)
  {
    font = (IeflFont*)calloc(1, sizeof(IeflFont));
    iupAttribSet(ih, "_IUP_EFL_FONT", (char*)font);
  }

  eflFontParse(value, family, &size, &is_bold, &is_italic, &is_underline, &is_strikeout);

  if (font->font_name)
    free(font->font_name);

  font->font_name = strdup(family);
  font->size = size;
  font->is_bold = is_bold;
  font->is_italic = is_italic;
  font->is_underline = is_underline;
  font->is_strikeout = is_strikeout;

  eflFontMeasure(family, size, is_bold, is_italic, &font->charwidth, &font->charheight, &font->ascent, &font->descent);

  /* If FONT is changed after mapping, must update the widget */
  if (ih->handle && (ih->iclass->nativetype != IUP_TYPEVOID))
  {
    Evas_Object* widget = iupeflGetWidget(ih);
    if (widget)
      iupeflApplyTextStyle(ih, widget);
  }

  return 1;
}

IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int* w, int* h)
{
  IeflFont* font = eflFontGet(ih);
  int max_w = 0;
  int num_lines = 1;
  const char* p;
  const char* line_start;
  int line_w, line_h;
  int charheight;
  int size = 12;
  int is_bold = 0;
  int is_italic = 0;
  char family[100] = "Sans";

  p = str;
  line_start = str;

  if (font)
  {
    size = font->size;
    is_bold = font->is_bold;
    is_italic = font->is_italic;
    iupStrCopyN(family, sizeof(family), font->font_name ? font->font_name : "Sans");
    charheight = font->charheight;
  }
  else
  {
    charheight = 16;
  }

  if (!str || !str[0])
  {
    if (w) *w = 0;
    if (h) *h = charheight;
    return;
  }

  if (ih && iupAttribGetBoolean(ih, "MARKUP"))
  {
    char* efl_markup = iupMarkupToEfl(str);
    Eo* tb = eflFontGetMeasureTextblock();
    if (tb)
    {
      Eina_Size2D sz;
      eflFontApplyStyle(tb, family, size, is_bold, is_italic);
      efl_text_markup_set(tb, efl_markup);
      sz = efl_canvas_textblock_size_native_get(tb);
      if (w) *w = sz.w > 0 ? sz.w : 0;
      if (h) *h = sz.h > 0 ? sz.h : charheight;
    }
    free(efl_markup);
    return;
  }

  while (*p)
  {
    if (*p == '\n')
    {
      if (p > line_start)
      {
        eflFontMeasureString(family, size, is_bold, is_italic, line_start, (int)(p - line_start), &line_w, &line_h);
        if (line_w > max_w)
          max_w = line_w;
      }
      num_lines++;
      line_start = p + 1;
    }
    p++;
  }

  if (p > line_start)
  {
    eflFontMeasureString(family, size, is_bold, is_italic, line_start, -1, &line_w, &line_h);
    if (line_w > max_w)
      max_w = line_w;
  }

  if (w) *w = max_w;
  if (h) *h = num_lines * charheight;
}

IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  IeflFont* font = eflFontGet(ih);
  int w;
  int size = 12;
  int is_bold = 0;
  int is_italic = 0;
  const char* family = "Sans";

  if (!str || !str[0])
    return 0;

  if (font)
  {
    family = font->font_name ? font->font_name : "Sans";
    size = font->size;
    is_bold = font->is_bold;
    is_italic = font->is_italic;
  }

  eflFontMeasureString(family, size, is_bold, is_italic, str, -1, &w, NULL);
  return w;
}

IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int* charwidth, int* charheight)
{
  IeflFont* font = eflFontGet(ih);

  if (font)
  {
    if (charwidth) *charwidth = font->charwidth;
    if (charheight) *charheight = font->charheight;
  }
  else
  {
    int cw, ch;

    eflFontMeasure("Sans", 12, 0, 0, &cw, &ch, NULL, NULL);

    if (charwidth) *charwidth = cw;
    if (charheight) *charheight = ch;
  }
}

IUP_SDK_API void iupdrvFontGetTextSize(const char* font_str, const char* str, int len, int* w, int* h)
{
  char family[100];
  int size, is_bold, is_italic, is_underline, is_strikeout;

  eflFontParse(font_str, family, &size, &is_bold, &is_italic, &is_underline, &is_strikeout);

  eflFontMeasureString(family, size, is_bold, is_italic, str, len, w, h);
}

IUP_SDK_API void iupdrvFontGetFontDim(const char* font_str, int* max_width, int* line_height, int* ascent, int* descent)
{
  char family[100];
  int size, is_bold, is_italic, is_underline, is_strikeout;
  int cw, ch, asc, desc;

  eflFontParse(font_str, family, &size, &is_bold, &is_italic, &is_underline, &is_strikeout);

  eflFontMeasure(family, size, is_bold, is_italic, &cw, &ch, &asc, &desc);

  if (max_width) *max_width = cw;
  if (line_height) *line_height = ch;
  if (ascent) *ascent = asc;
  if (descent) *descent = desc;
}

IUP_SDK_API char* iupdrvGetSystemFont(void)
{
  static char system_font[200] = "";
  const Eina_List* overlays;
  const Eina_List* l;
  Elm_Font_Overlay* fo;

  if (system_font[0] != '\0')
    return system_font;

  overlays = elm_config_font_overlay_list_get();
  if (overlays)
  {
    EINA_LIST_FOREACH(overlays, l, fo)
    {
      if (fo->text_class && strcmp(fo->text_class, "button") == 0)
      {
        if (fo->font && fo->font[0] && fo->size > 0)
        {
          snprintf(system_font, sizeof(system_font), "%s, %d", fo->font, (int)fo->size);
          return system_font;
        }
        break;
      }
    }
  }

  iupStrCopyN(system_font, sizeof(system_font), "Sans, 11");
  return system_font;
}

static int eflFontFamilyCompare(const void* a, const void* b)
{
  return iupStrCompare(*(const char**)a, *(const char**)b, 0, 1);
}

IUP_SDK_API int iupdrvFontGetFamilyList(char*** list)
{
  Evas* evas;
  Eina_List* fonts_list;
  Eina_Hash* fonts_hash;
  Eina_Iterator* it;
  const char* key;
  int capacity = 256, count = 0;
  char** temp;

  evas = eflFontGetMeasureEvas();
  if (!evas)
  {
    *list = NULL;
    return 0;
  }

  fonts_list = evas_font_available_list(evas);
  if (!fonts_list)
  {
    *list = NULL;
    return 0;
  }

  fonts_hash = elm_font_available_hash_add(fonts_list);
  if (!fonts_hash)
  {
    evas_font_available_list_free(evas, fonts_list);
    *list = NULL;
    return 0;
  }

  temp = (char**)malloc(capacity * sizeof(char*));

  it = eina_hash_iterator_key_new(fonts_hash);
  EINA_ITERATOR_FOREACH(it, key)
  {
    if (count >= capacity)
    {
      char** new_temp;
      capacity *= 2;
      new_temp = (char**)realloc(temp, capacity * sizeof(char*));
      if (!new_temp)
        break;
      temp = new_temp;
    }
    temp[count] = iupStrDup(key);
    count++;
  }
  eina_iterator_free(it);

  elm_font_available_hash_del(fonts_hash);
  evas_font_available_list_free(evas, fonts_list);

  if (count == 0)
  {
    free(temp);
    *list = NULL;
    return 0;
  }

  *list = (char**)realloc(temp, count * sizeof(char*));
  qsort(*list, count, sizeof(char*), eflFontFamilyCompare);

  return count;
}

IUP_DRV_API void iupeflBuildTextStyle(Ihandle* ih, char* style, int style_size)
{
  IeflFont* font = eflFontGet(ih);
  char* fgcolor;
  unsigned char r = 0, g = 0, b = 0;
  char font_family[100] = "Sans";
  char font_style[50] = "";
  int font_size = 12;

  if (font)
  {
    if (font->font_name)
      strncpy(font_family, font->font_name, sizeof(font_family) - 1);
    font_size = font->size > 0 ? font->size : 12;

    if (font->is_bold && font->is_italic)
      iupStrCopyN(font_style, sizeof(font_style), ":style=Bold Italic");
    else if (font->is_bold)
      iupStrCopyN(font_style, sizeof(font_style), ":style=Bold");
    else if (font->is_italic)
      iupStrCopyN(font_style, sizeof(font_style), ":style=Italic");
  }

  fgcolor = iupAttribGetStr(ih, "FGCOLOR");
  if (!fgcolor)
    fgcolor = IupGetGlobal("DLGFGCOLOR");
  if (fgcolor)
    iupStrToRGB(fgcolor, &r, &g, &b);

  snprintf(style, style_size, "DEFAULT='font=%s%s font_size=%d color=#%02X%02X%02X'",
           font_family, font_style, font_size, r, g, b);
}

IUP_DRV_API void iupeflApplyTextStyle(Ihandle* ih, Eo* widget)
{
  Evas_Object* edje;
  Evas_Object* part_obj;
  char style[256];
  char* fgcolor;
  unsigned char r = 0, g = 0, b = 0;

  if (!widget)
    return;

  if (efl_isa(widget, EFL_UI_TEXTBOX_CLASS))
  {
    IeflFont* font = eflFontGet(ih);

    if (font && font->font_name)
    {
      efl_text_font_family_set(widget, font->font_name);
      efl_text_font_size_set(widget, font->size > 0 ? font->size : 11);
      efl_text_font_weight_set(widget, font->is_bold ? EFL_TEXT_FONT_WEIGHT_BOLD : EFL_TEXT_FONT_WEIGHT_NORMAL);
      efl_text_font_slant_set(widget, font->is_italic ? EFL_TEXT_FONT_SLANT_ITALIC : EFL_TEXT_FONT_SLANT_NORMAL);
    }

    fgcolor = iupAttribGetStr(ih, "FGCOLOR");
    if (!fgcolor)
      fgcolor = IupGetGlobal("DLGFGCOLOR");
    if (fgcolor)
      iupStrToRGB(fgcolor, &r, &g, &b);

    if (!iupAttribGetBoolean(ih, "ACTIVE"))
    {
      unsigned char bg_r = 192, bg_g = 192, bg_b = 192;
      char* bgcolor = iupAttribGetStr(ih, "BGCOLOR");
      if (!bgcolor)
        bgcolor = IupGetGlobal("DLGBGCOLOR");
      if (bgcolor)
        iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);
      iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);
    }

    efl_text_color_set(widget, r, g, b, 255);
    return;
  }

  if (!efl_isa(widget, EFL_UI_LAYOUT_BASE_CLASS))
    return;

  edje = elm_layout_edje_get(widget);
  if (!edje)
    return;

  iupeflBuildTextStyle(ih, style, sizeof(style));

  /* EFL_UI_CHECK and EFL_UI_RADIO use TEXT parts which don't support font customization.
     Edje recalc resets any font changes, and the part API doesn't implement Efl.Text_Font_Properties.
     Only FGCOLOR is supported via color classes. */
  if (efl_isa(widget, EFL_UI_CHECK_CLASS) || efl_isa(widget, EFL_UI_RADIO_CLASS))
  {
    fgcolor = iupAttribGetStr(ih, "FGCOLOR");
    if (!fgcolor)
      fgcolor = IupGetGlobal("DLGFGCOLOR");
    if (fgcolor && iupStrToRGB(fgcolor, &r, &g, &b))
    {
      edje_object_color_class_set(edje, "/fg/normal/check/text", r, g, b, 255, 0, 0, 0, 0, 0, 0, 0, 0);
      edje_object_color_class_set(edje, "/fg/normal/radio/text", r, g, b, 255, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    return;
  }

  /* Legacy widgets use TEXTBLOCK with elm.text part */
  edje_object_part_text_style_user_pop(edje, "elm.text");
  edje_object_part_text_style_user_push(edje, "elm.text", style);

  part_obj = (Evas_Object*)edje_object_part_object_get(edje, "elm.text");
  if (part_obj)
  {
    const char* obj_type = evas_object_type_get(part_obj);

    if (obj_type && strcmp(obj_type, "textblock") == 0)
    {
      Evas_Textblock_Style* ts = evas_textblock_style_new();
      evas_textblock_style_set(ts, style);
      evas_object_textblock_style_user_push(part_obj, ts);
      evas_textblock_style_free(ts);

      edje_object_calc_force(edje);
    }
  }
}

IUP_DRV_API void iupeflUpdateWidgetFont(Ihandle* ih, Evas_Object* widget)
{
  iupeflApplyTextStyle(ih, widget);
}

IUP_SDK_API void iupdrvFontInit(void)
{
  efl_font_measure_tb = NULL;
  efl_font_buffer_ee = ecore_evas_buffer_new(1, 1);
}

IUP_SDK_API void iupdrvFontFinish(void)
{
  if (efl_font_measure_tb)
  {
    efl_del(efl_font_measure_tb);
    efl_font_measure_tb = NULL;
  }

  if (efl_font_buffer_ee)
  {
    ecore_evas_free(efl_font_buffer_ee);
    efl_font_buffer_ee = NULL;
  }
}
