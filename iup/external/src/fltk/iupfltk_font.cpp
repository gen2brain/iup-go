/** \file
 * \brief FLTK Font mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_array.h"
#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_assert.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Font Cache Structure
 ****************************************************************************/

typedef struct _IfltkFont
{
  char font[200];
  int fl_font;
  int fl_size;
  int charwidth, charheight;
  int max_width, ascent, descent;
  int is_underline;
  int is_strikeout;
} IfltkFont;

static Iarray* fltk_fonts = NULL;

/****************************************************************************
 * Font Name Mapping
 ****************************************************************************/

static Fl_Font fltk_font_count = 0;

IUP_DRV_API int iupfltkMapFontFace(const char* typeface, int is_bold, int is_italic)
{
  if (iupStrEqualNoCase(typeface, "Courier") ||
      iupStrEqualNoCase(typeface, "Monospace") ||
      iupStrEqualNoCase(typeface, "Courier New"))
  {
    int base = FL_COURIER;
    if (is_bold && is_italic) return base + FL_BOLD + FL_ITALIC;
    else if (is_bold) return base + FL_BOLD;
    else if (is_italic) return base + FL_ITALIC;
    return base;
  }
  else if (iupStrEqualNoCase(typeface, "Times") ||
           iupStrEqualNoCase(typeface, "Serif") ||
           iupStrEqualNoCase(typeface, "Times New Roman"))
  {
    int base = FL_TIMES;
    if (is_bold && is_italic) return base + FL_BOLD + FL_ITALIC;
    else if (is_bold) return base + FL_BOLD;
    else if (is_italic) return base + FL_ITALIC;
    return base;
  }
  else if (iupStrEqualNoCase(typeface, "Sans") ||
           iupStrEqualNoCase(typeface, "Helvetica") ||
           iupStrEqualNoCase(typeface, "Arial"))
  {
    int base = FL_HELVETICA;
    if (is_bold && is_italic) return base + FL_BOLD + FL_ITALIC;
    else if (is_bold) return base + FL_BOLD;
    else if (is_italic) return base + FL_ITALIC;
    return base;
  }

  if (fltk_font_count == 0)
    fltk_font_count = Fl::set_fonts(NULL);

  int want_attr = (is_bold ? FL_BOLD : 0) | (is_italic ? FL_ITALIC : 0);
  Fl_Font base_match = -1;

  for (Fl_Font i = 0; i < fltk_font_count; i++)
  {
    int attr = 0;
    const char* name = Fl::get_font_name(i, &attr);
    if (!name) continue;

    int namelen = (int)strlen(typeface);
    int fontnamelen = (int)strlen(name);
    if (fontnamelen < namelen) continue;

    if (strncasecmp(name, typeface, namelen) == 0 &&
        (fontnamelen == namelen || name[namelen] == ' '))
    {
      if (attr == want_attr)
        return i;

      if (attr == 0 && base_match < 0)
        base_match = i;
    }
  }

  if (base_match >= 0)
    return base_match;

  int base = FL_HELVETICA;
  if (is_bold && is_italic) return base + FL_BOLD + FL_ITALIC;
  else if (is_bold) return base + FL_BOLD;
  else if (is_italic) return base + FL_ITALIC;
  return base;
}

/****************************************************************************
 * Font Cache Management
 ****************************************************************************/

static IfltkFont* fltkFindFont(const char* font)
{
  int i, count;
  int size = 10;
  int is_bold = 0,
      is_italic = 0,
      is_underline = 0,
      is_strikeout = 0;
  char typeface[1024];
  const char* mapped_name;
  IfltkFont* fonts;

  if (!fltk_fonts)
    return NULL;

  count = iupArrayCount(fltk_fonts);
  fonts = (IfltkFont*)iupArrayGetData(fltk_fonts);

  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(font, fonts[i].font))
      return &fonts[i];
  }

  if (!iupFontParseWin(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
  {
    if (!iupFontParseX(font, typeface, sizeof(typeface), &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    {
      if (!iupFontParsePango(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
        return NULL;
    }
  }

  mapped_name = iupFontGetPangoName(typeface);
  if (mapped_name)
    iupStrCopyN(typeface, sizeof(typeface), mapped_name);

  int pixel_size;
  if (size < 0)
    pixel_size = -size;
  else
    pixel_size = (int)(size * FL_NORMAL_SIZE / 10.0 + 0.5);

  if (pixel_size <= 0)
    return NULL;

  int fl_font_id = iupfltkMapFontFace(typeface, is_bold, is_italic);

  fl_font(fl_font_id, pixel_size);

  fonts = (IfltkFont*)iupArrayInc(fltk_fonts);

  iupStrCopyN(fonts[i].font, sizeof(fonts[i].font), font);
  fonts[i].fl_font = fl_font_id;
  fonts[i].fl_size = pixel_size;
  fonts[i].is_underline = is_underline;
  fonts[i].is_strikeout = is_strikeout;

  fonts[i].charheight = (int)(fl_height() + 0.5);
  fonts[i].charwidth = (int)(fl_width("x", 1) + 0.5);
  fonts[i].max_width = (int)(fl_width("W", 1) + 0.5);
  fonts[i].ascent = (int)(fl_height() - fl_descent() + 0.5);
  fonts[i].descent = (int)(fl_descent() + 0.5);

  return &fonts[i];
}

static IfltkFont* fltkFontCreateNativeFont(Ihandle* ih, const char* value)
{
  IfltkFont* fltkfont = fltkFindFont(value);
  if (!fltkfont)
  {
    iupERROR1("Failed to create Font: %s", value);
    return NULL;
  }

  iupAttribSet(ih, "_IUP_FLTKFONT", (char*)fltkfont);
  return fltkfont;
}

static IfltkFont* fltkFontGet(Ihandle* ih)
{
  IfltkFont* fltkfont = fltkFindFont(iupGetFontValue(ih));
  if (!fltkfont)
    fltkfont = fltkFindFont(IupGetGlobal("DEFAULTFONT"));
  if (!fltkfont)
    fltkfont = fltkFindFont("Sans, 10");
  return fltkfont;
}

/****************************************************************************
 * FLTK-specific Font Functions
 ****************************************************************************/

IUP_DRV_API int iupfltkGetFont(Ihandle* ih, int* fl_font_id, int* fl_size)
{
  IfltkFont* fltkfont = fltkFontGet(ih);
  if (!fltkfont)
    return 0;

  if (fl_font_id) *fl_font_id = fltkfont->fl_font;
  if (fl_size) *fl_size = fltkfont->fl_size;
  return 1;
}

IUP_DRV_API int iupfltkGetFontFromString(const char* font, int* fl_font_id, int* fl_size)
{
  IfltkFont* fltkfont = fltkFindFont(font);
  if (!fltkfont)
    return 0;

  if (fl_font_id) *fl_font_id = fltkfont->fl_font;
  if (fl_size) *fl_size = fltkfont->fl_size;
  return 1;
}

IUP_DRV_API void iupfltkUpdateWidgetFont(Ihandle* ih, Fl_Widget* widget)
{
  IfltkFont* fltkfont = fltkFontGet(ih);
  if (!fltkfont)
    return;

  widget->labelfont(fltkfont->fl_font);
  widget->labelsize(fltkfont->fl_size);
}

static void fltkFontGetTextSize(Ihandle* ih, IfltkFont* fltkfont, const char* str, int len, int* w, int* h)
{
  int max_w = 0;
  int line_count = 1;

  if (!fltkfont)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  if (!str)
  {
    if (w) *w = 0;
    if (h) *h = fltkfont->charheight;
    return;
  }

  if (h)
    line_count = iupStrLineCount(str, len);

  fl_font(fltkfont->fl_font, fltkfont->fl_size);

  if (str[0])
  {
    const char* curstr = str;
    const char* nextstr;
    int l_len;
    int sum_len = 0;

    do
    {
      nextstr = iupStrNextLine(curstr, &l_len);
      if (sum_len + l_len > len)
        l_len = len - sum_len;

      if (l_len)
      {
        int line_w = (int)(fl_width(curstr, l_len) + 0.5);
        max_w = iupMAX(max_w, line_w);
      }

      sum_len += l_len;
      if (sum_len >= len)
        break;

      curstr = nextstr;
    } while (*nextstr);
  }

  if (w) *w = max_w;
  if (h) *h = fltkfont->charheight * line_count;

  (void)ih;
}

/****************************************************************************
 * Driver Font Functions
 ****************************************************************************/

extern "C" IUP_SDK_API char* iupdrvGetSystemFont(void)
{
  static char str[200];

  int fattr = 0;
  const char* family = Fl::get_font_name(FL_HELVETICA, &fattr);
  if (!family || !family[0])
    family = "Sans";

  snprintf(str, sizeof(str), "%s, 10", family);
  return str;
}

extern "C" IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  IfltkFont* fltkfont = fltkFontCreateNativeFont(ih, value);
  if (!fltkfont)
    return 0;

  iupBaseUpdateAttribFromFont(ih);

  if (ih->handle &&
      (ih->iclass->nativetype != IUP_TYPEVOID) &&
      (ih->iclass->nativetype != IUP_TYPEMENU))
  {
    Fl_Widget* widget = (Fl_Widget*)ih->handle;
    widget->labelfont(fltkfont->fl_font);
    widget->labelsize(fltkfont->fl_size);
    widget->redraw_label();
  }

  return 1;
}

extern "C" IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int* w, int* h)
{
  IfltkFont* fltkfont = fltkFontGet(ih);
  if (fltkfont)
    fltkFontGetTextSize(ih, fltkfont, str, str ? (int)strlen(str) : 0, w, h);
}

extern "C" IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int* w, int* h)
{
  IfltkFont* fltkfont = fltkFindFont(font);
  if (fltkfont)
    fltkFontGetTextSize(NULL, fltkfont, str, len, w, h);
}

extern "C" IUP_SDK_API void iupdrvFontGetFontDim(const char* font, int* max_width, int* line_height, int* ascent, int* descent)
{
  IfltkFont* fltkfont = fltkFindFont(font);
  if (fltkfont)
  {
    if (max_width)   *max_width = fltkfont->max_width;
    if (line_height) *line_height = fltkfont->charheight;
    if (ascent)      *ascent = fltkfont->ascent;
    if (descent)     *descent = fltkfont->descent;
  }
}

extern "C" IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  IfltkFont* fltkfont;
  const char* line_end;
  int len;

  if (!str || str[0] == 0)
    return 0;

  fltkfont = fltkFontGet(ih);
  if (!fltkfont)
    return 0;

  line_end = strchr(str, '\n');
  if (line_end)
    len = (int)(line_end - str);
  else
    len = (int)strlen(str);

  fl_font(fltkfont->fl_font, fltkfont->fl_size);
  return (int)(fl_width(str, len) + 0.5);
}

extern "C" IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int* charwidth, int* charheight)
{
  IfltkFont* fltkfont = fltkFontGet(ih);
  if (!fltkfont)
  {
    if (charwidth)  *charwidth = 0;
    if (charheight) *charheight = 0;
    return;
  }

  if (charwidth)  *charwidth = fltkfont->charwidth;
  if (charheight) *charheight = fltkfont->charheight;
}

static int fltkFontFamilyCompare(const void* a, const void* b)
{
  return iupStrCompare(*(const char**)a, *(const char**)b, 0, 1);
}

extern "C" IUP_SDK_API int iupdrvFontGetFamilyList(char*** list)
{
  Fl_Font total = Fl::set_fonts(NULL);
  int i, count = 0;
  char prev[256] = "";

  char** temp = (char**)malloc(total * sizeof(char*));

  for (i = 0; i < total; i++)
  {
    int attr = 0;
    const char* name = Fl::get_font_name((Fl_Font)i, &attr);
    if (!name || !name[0]) continue;
    if (attr != 0) continue;
    if (iupStrEqual(name, prev)) continue;

    temp[count] = iupStrDup(name);
    iupStrCopyN(prev, sizeof(prev), name);
    count++;
  }

  if (count == 0)
  {
    free(temp);
    *list = NULL;
    return 0;
  }

  *list = (char**)realloc(temp, count * sizeof(char*));
  qsort(*list, count, sizeof(char*), fltkFontFamilyCompare);

  return count;
}

extern "C" IUP_SDK_API void iupdrvFontInit(void)
{
  fltk_fonts = iupArrayCreate(50, sizeof(IfltkFont));
}

extern "C" IUP_SDK_API void iupdrvFontFinish(void)
{
  if (!fltk_fonts)
    return;

  iupArrayDestroy(fltk_fonts);
  fltk_fonts = NULL;
}
