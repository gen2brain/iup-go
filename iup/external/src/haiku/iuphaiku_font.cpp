/** \file
 * \brief Haiku Font Management
 *
 * See Copyright Notice in "iup.h"
 */

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Font.h>
#include <TextView.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_array.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_drvfont.h"
}

#include "iuphaiku_drv.h"


/* Cache keyed on IUP font string. BFont* heap-allocated so the record is POD for iupArray. */

typedef struct _IhaikuFont {
  char font_str[200];
  BFont* bfont;
  int charwidth;
  int charheight;
  int max_width;
  int ascent;
  int descent;
  int is_underline;
  int is_strikeout;
} IhaikuFont;

static Iarray* haiku_fonts = NULL;

/* Font Resolution */

static bool haikuFamilyContains(const char* haystack, const char* needle)
{
  size_t hlen = strlen(haystack), nlen = strlen(needle);
  if (nlen == 0 || nlen > hlen) return false;
  for (size_t i = 0; i + nlen <= hlen; ++i)
    if (strncasecmp(haystack + i, needle, nlen) == 0) return true;
  return false;
}

/* Match Pango-style family aliases against Haiku's actual families. Standard
 * names normalize via iupFontGetPangoName before reaching here. */
static const char* haikuResolveAlias(const char* typeface)
{
  if (strcasecmp(typeface, "sans") == 0 ||
      strcasecmp(typeface, "sans-serif") == 0 ||
      strcasecmp(typeface, "helvetica") == 0 ||
      strcasecmp(typeface, "arial") == 0 ||
      strcasecmp(typeface, "system") == 0)
    return "Noto Sans";
  if (strcasecmp(typeface, "monospace") == 0 ||
      strcasecmp(typeface, "mono") == 0 ||
      strcasecmp(typeface, "courier") == 0 ||
      strcasecmp(typeface, "courier new") == 0 ||
      strcasecmp(typeface, "fixed") == 0 ||
      strcasecmp(typeface, "console") == 0 ||
      strcasecmp(typeface, "lucida console") == 0)
    return "Noto Sans Mono";
  if (strcasecmp(typeface, "serif") == 0 ||
      strcasecmp(typeface, "times") == 0 ||
      strcasecmp(typeface, "times new roman") == 0)
    return "Noto Serif";
  return NULL;
}

static bool haikuFindFamilyExact(const char* typeface, font_family out_family)
{
  if (!typeface || !*typeface) return false;
  int32 family_count = count_font_families();
  for (int32 i = 0; i < family_count; ++i)
  {
    font_family fam;
    if (get_font_family(i, &fam) == B_OK && strcasecmp(fam, typeface) == 0)
    {
      strcpy(out_family, fam);
      return true;
    }
  }
  return false;
}

static bool haikuFindFamily(const char* typeface, font_family out_family)
{
  if (!typeface || !*typeface)
    return false;

  if (haikuFindFamilyExact(typeface, out_family))
    return true;

  const char* alias = haikuResolveAlias(typeface);
  if (alias && haikuFindFamilyExact(alias, out_family))
    return true;

  /* Substring fallback for unknown typefaces (covers "Noto Sans Display" given "Display" etc.). */
  int32 family_count = count_font_families();
  for (int32 i = 0; i < family_count; ++i)
  {
    font_family fam;
    if (get_font_family(i, &fam) == B_OK && haikuFamilyContains(fam, typeface))
    {
      strcpy(out_family, fam);
      return true;
    }
  }

  /* Last resort: use the system plain/fixed family for sans/mono families that the alias map didn't catch. */
  font_family sysfam;
  font_style sysst;
  if (haikuFamilyContains(typeface, "mono") || haikuFamilyContains(typeface, "fixed") ||
      haikuFamilyContains(typeface, "console") || haikuFamilyContains(typeface, "courier"))
  {
    be_fixed_font->GetFamilyAndStyle(&sysfam, &sysst);
    strcpy(out_family, sysfam);
    return true;
  }
  be_plain_font->GetFamilyAndStyle(&sysfam, &sysst);
  strcpy(out_family, sysfam);
  return true;
}

static void haikuFindStyle(const font_family family, uint16 want_face, font_style out_style)
{
  strcpy(out_style, "Regular");

  if (want_face == 0)
    want_face = B_REGULAR_FACE;

  int32 style_count = count_font_styles((char*)family);

  /* exact face match */
  for (int32 j = 0; j < style_count; ++j)
  {
    font_style st;
    uint16 face = 0;
    if (get_font_style((char*)family, j, &st, &face) == B_OK)
    {
      if (face == want_face)
      {
        strcpy(out_style, st);
        return;
      }
    }
  }

  /* face bits subset match */
  for (int32 j = 0; j < style_count; ++j)
  {
    font_style st;
    uint16 face = 0;
    if (get_font_style((char*)family, j, &st, &face) == B_OK)
    {
      if ((face & want_face) == want_face)
      {
        strcpy(out_style, st);
        return;
      }
    }
  }

  /* fallback: first style */
  if (style_count > 0)
  {
    font_style st;
    uint32 flags = 0;
    if (get_font_style((char*)family, 0, &st, &flags) == B_OK)
      strcpy(out_style, st);
  }
}

static BFont* haikuBuildBFont(const char* typeface, int size, int is_bold, int is_italic, int is_underline, int is_strikeout)
{
  BFont* bf = new BFont(be_plain_font);

  uint16 want_face = 0;
  if (is_bold) want_face |= B_BOLD_FACE;
  if (is_italic) want_face |= B_ITALIC_FACE;

  font_family family;
  bool family_found = haikuFindFamily(typeface, family);
  if (family_found)
  {
    font_style style;
    haikuFindStyle(family, want_face, style);
    bf->SetFamilyAndStyle(family, style);
  }

  /* IUP size: + points, - pixels; treat |size| as points (close at 72-96 DPI). */
  if (size != 0)
    bf->SetSize((float)(size < 0 ? -size : size));

  uint16 face = bf->Face();
  if (is_bold)      face |= B_BOLD_FACE;
  if (is_italic)    face |= B_ITALIC_FACE;
  if (is_underline) face |= B_UNDERSCORE_FACE;
  else              face &= ~B_UNDERSCORE_FACE;
  if (is_strikeout) face |= B_STRIKEOUT_FACE;
  else              face &= ~B_STRIKEOUT_FACE;
  if (face == 0)    face = B_REGULAR_FACE;
  bf->SetFace(face);

  return bf;
}

static void haikuComputeMetrics(IhaikuFont* hf)
{
  font_height fh;
  hf->bfont->GetHeight(&fh);

  hf->ascent  = (int)ceilf(fh.ascent);
  hf->descent = (int)ceilf(fh.descent);
  hf->charheight = (int)ceilf(fh.ascent + fh.descent + fh.leading);
  hf->charwidth  = (int)ceilf(hf->bfont->StringWidth("x"));
  hf->max_width  = (int)ceilf(hf->bfont->StringWidth("W"));

  if (hf->charheight < 1) hf->charheight = 1;
  if (hf->charwidth  < 1) hf->charwidth  = 1;
  if (hf->max_width  < 1) hf->max_width  = hf->charwidth;
}

/* Cache Lookup */

static IhaikuFont* haikuFindFont(const char* font)
{
  if (!haiku_fonts || !font || !*font)
    return NULL;

  IhaikuFont* fonts = (IhaikuFont*)iupArrayGetData(haiku_fonts);
  int count = iupArrayCount(haiku_fonts);
  for (int i = 0; i < count; ++i)
  {
    if (iupStrEqualNoCase(font, fonts[i].font_str))
      return &fonts[i];
  }

  /* not cached - parse and add */
  char typeface[1024] = {0};
  int size = 0;
  int is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  const char* mapped = iupFontGetPangoName(typeface);
  if (mapped)
    iupStrCopyN(typeface, sizeof(typeface), mapped);

  BFont* bfont = haikuBuildBFont(typeface, size, is_bold, is_italic, is_underline, is_strikeout);
  if (!bfont)
    return NULL;

  IhaikuFont* slot = (IhaikuFont*)iupArrayInc(haiku_fonts);
  iupStrCopyN(slot->font_str, sizeof(slot->font_str), font);
  slot->bfont = bfont;
  slot->is_underline = is_underline;
  slot->is_strikeout = is_strikeout;
  haikuComputeMetrics(slot);
  return slot;
}

static IhaikuFont* haikuFontGet(Ihandle* ih)
{
  IhaikuFont* hf = NULL;
  if (ih)
    hf = haikuFindFont(iupGetFontValue(ih));
  if (!hf)
    hf = haikuFindFont(IupGetGlobal("DEFAULTFONT"));
  if (!hf)
    hf = haikuFindFont("Sans, 10");
  return hf;
}

/* Driver Hooks - lifecycle */

extern "C" IUP_SDK_API void iupdrvFontInit(void)
{
  haiku_fonts = iupArrayCreate(8, sizeof(IhaikuFont));
}

extern "C" IUP_SDK_API void iupdrvFontFinish(void)
{
  if (!haiku_fonts)
    return;

  IhaikuFont* fonts = (IhaikuFont*)iupArrayGetData(haiku_fonts);
  int count = iupArrayCount(haiku_fonts);
  for (int i = 0; i < count; ++i)
  {
    delete fonts[i].bfont;
    fonts[i].bfont = NULL;
  }
  iupArrayDestroy(haiku_fonts);
  haiku_fonts = NULL;
}

/* System default font */

extern "C" IUP_SDK_API char* iupdrvGetSystemFont(void)
{
  static char def[128];
  font_family family;
  font_style style;
  be_plain_font->GetFamilyAndStyle(&family, &style);
  int size = (int)(be_plain_font->Size() + 0.5f);
  if (size <= 0) size = 10;
  snprintf(def, sizeof(def), "%s, %s %d", family, style, size);
  return def;
}

/* Per-widget FONT setter */

extern "C" IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  IhaikuFont* hf = haikuFindFont(value);
  if (!hf)
    return 0;

  iupAttribSetStr(ih, "FONT", value);
  iupBaseUpdateAttribFromFont(ih);

  /* TYPEVOID handle is (void*)-1, TYPEDIALOG is BWindow*; neither takes SetFont */
  if (!ih->iclass || ih->iclass->nativetype != IUP_TYPECONTROL || !ih->handle)
    return 1;

  iuphaikuUpdateWidgetFont(ih, (BView*)ih->handle);
  return 1;
}

/* Helpers exposed via drv.h */

IUP_DRV_API BFont* iuphaikuGetBFont(const char* value)
{
  IhaikuFont* hf = haikuFindFont(value);
  return hf ? hf->bfont : NULL;
}

IUP_DRV_API void iuphaikuFontApplyFace(BFont* bf, int is_bold, int is_italic, int is_underline, int is_strikeout)
{
  if (!bf) return;

  font_family family;
  font_style style;
  bf->GetFamilyAndStyle(&family, &style);

  uint16 want_face = 0;
  if (is_bold) want_face |= B_BOLD_FACE;
  if (is_italic) want_face |= B_ITALIC_FACE;
  haikuFindStyle(family, want_face, style);
  bf->SetFamilyAndStyle(family, style);

  uint16 face = bf->Face();
  if (is_bold)      face |= B_BOLD_FACE;
  if (is_italic)    face |= B_ITALIC_FACE;
  if (is_underline) face |= B_UNDERSCORE_FACE; else face &= ~B_UNDERSCORE_FACE;
  if (is_strikeout) face |= B_STRIKEOUT_FACE;  else face &= ~B_STRIKEOUT_FACE;
  if (face == 0) face = B_REGULAR_FACE;
  bf->SetFace(face);
}

IUP_DRV_API void iuphaikuUpdateWidgetFont(Ihandle* ih, BView* widget)
{
  if (!widget) return;

  IhaikuFont* hf = haikuFontGet(ih);
  if (!hf) return;

  LooperLockGuard guard(widget->Looper());
  widget->SetFont(hf->bfont);

  /* BTextView/BTextControl don't re-style rendered text from BView::SetFont;
     push the font through SetFontAndColor over the existing range. */
  if (BTextView* tv = (BTextView*)iupAttribGet(ih, "_IUPHAIKU_TEXT_INNER"))
    tv->SetFontAndColor(0, INT32_MAX, hf->bfont, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE | B_FONT_FACE, NULL);
}

/* Measurement */

extern "C" IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int* charwidth, int* charheight)
{
  IhaikuFont* hf = haikuFontGet(ih);
  if (!hf)
  {
    if (charwidth)  *charwidth = 0;
    if (charheight) *charheight = 0;
    return;
  }
  if (charwidth)  *charwidth  = hf->charwidth;
  if (charheight) *charheight = hf->charheight;
}

extern "C" IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  if (!str || !*str) return 0;
  IhaikuFont* hf = haikuFontGet(ih);
  if (!hf) return 0;

  /* first line only */
  const char* nl = strchr(str, '\n');
  int len = nl ? (int)(nl - str) : (int)strlen(str);
  if (len <= 0) return 0;
  return (int)(hf->bfont->StringWidth(str, len) + 0.5f);
}

static void haikuFontMeasureMultiLine(IhaikuFont* hf, const char* str, int len, int* w, int* h)
{
  if (!hf || !str || len <= 0)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  int max_w = 0;
  int line_count = 1;
  const char* line = str;
  const char* end = str + len;

  for (const char* p = str; p < end; ++p)
  {
    if (*p == '\n')
    {
      int line_len = (int)(p - line);
      if (line_len > 0)
      {
        int lw = (int)(hf->bfont->StringWidth(line, line_len) + 0.5f);
        if (lw > max_w) max_w = lw;
      }
      line = p + 1;
      line_count++;
    }
  }

  /* last line */
  int line_len = (int)(end - line);
  if (line_len > 0)
  {
    int lw = (int)(hf->bfont->StringWidth(line, line_len) + 0.5f);
    if (lw > max_w) max_w = lw;
  }

  if (w) *w = max_w;
  if (h) *h = line_count * hf->charheight;
}

extern "C" IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int* w, int* h)
{
  IhaikuFont* hf = haikuFontGet(ih);
  if (!str)
  {
    if (w) *w = 0;
    if (h) *h = hf ? hf->charheight : 0;
    return;
  }
  haikuFontMeasureMultiLine(hf, str, (int)strlen(str), w, h);
}

extern "C" IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int* w, int* h)
{
  IhaikuFont* hf = haikuFindFont(font);
  if (!hf) hf = haikuFindFont(IupGetGlobal("DEFAULTFONT"));
  if (!str)
  {
    if (w) *w = 0;
    if (h) *h = hf ? hf->charheight : 0;
    return;
  }
  if (len < 0) len = (int)strlen(str);
  haikuFontMeasureMultiLine(hf, str, len, w, h);
}

extern "C" IUP_SDK_API void iupdrvFontGetFontDim(const char* font, int* max_width, int* line_height, int* ascent, int* descent)
{
  IhaikuFont* hf = haikuFindFont(font);
  if (!hf) hf = haikuFindFont(IupGetGlobal("DEFAULTFONT"));
  if (!hf)
  {
    if (max_width)   *max_width = 0;
    if (line_height) *line_height = 0;
    if (ascent)      *ascent = 0;
    if (descent)     *descent = 0;
    return;
  }
  if (max_width)   *max_width = hf->max_width;
  if (line_height) *line_height = hf->charheight;
  if (ascent)      *ascent = hf->ascent;
  if (descent)     *descent = hf->descent;
}

/* Family enumeration */

static int haikuFamilyCompare(const void* a, const void* b)
{
  const char* sa = *(const char* const*)a;
  const char* sb = *(const char* const*)b;
  return strcasecmp(sa, sb);
}

extern "C" IUP_SDK_API int iupdrvFontGetFamilyList(char*** list)
{
  if (list) *list = NULL;
  int32 family_count = count_font_families();
  if (family_count <= 0)
    return 0;

  char** result = (char**)malloc(sizeof(char*) * family_count);
  int n = 0;
  for (int32 i = 0; i < family_count; ++i)
  {
    font_family fam;
    if (get_font_family(i, &fam) == B_OK)
    {
      result[n++] = iupStrDup(fam);
    }
  }

  qsort(result, n, sizeof(char*), haikuFamilyCompare);
  if (list) *list = result;
  return n;
}
