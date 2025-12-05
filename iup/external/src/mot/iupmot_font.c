/** \file
 * \brief Motif Font mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include <Xm/Xm.h>
#include <Xm/XmP.h>

#ifdef IUP_USE_XFT
#include <X11/Xft/Xft.h>
#endif

#include "iup.h"

#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_array.h"
#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_assert.h"

#include "iupmot_drv.h"


typedef struct _ImotFont
{
  char font[1024];
  char xlfd[1024];  /* X-Windows Font Description */
  XmFontList fontlist;  /* same as XmRenderTable */
  XFontStruct *fontstruct;
#ifdef IUP_USE_XFT
  XftFont *xftfont;
  char xft_pattern[1024];
  int is_xft;
#endif
  int charwidth, charheight;
} ImotFont;

static Iarray* mot_fonts = NULL;

static int motGetFontSize(const char* font_name)
{
  int i = 0;
  while (i < 8)
  {
    font_name = strchr(font_name, '-')+1;
    i++;
  }

  *(strchr(font_name, '-')) = 0;
  return atoi(font_name);
}

static XFontStruct* motLoadFont(const char* foundry, const char *typeface, int size, int bold, int italic, char *xlfd)
{
  XFontStruct* fontstruct;
  char font_name[1024];
  char **font_names_list;
  char *weight, *slant;
  int i, num_fonts, font_size, near_size;
              
  /* no underline or strikeout parsing here */

  if (iupStrEqualNoCase(typeface, "System"))
    typeface = "fixed";

  if (!foundry) foundry = "*";

  if (iupStrEqualNoCase(typeface, "fixed"))
    foundry = "misc";

  if (bold)
    weight = "bold";
  else
    weight = "medium";

  if (italic)
    slant = "i";
  else
    slant = "r";

  sprintf(font_name,"-%s-%s-%s-%s-*-*-*-*-*-*-*-*-*-*", foundry, typeface, weight, slant);

  font_names_list = XListFonts(iupmot_display, font_name, 32767, &num_fonts);
  if (!num_fonts && italic)
  {
    /* try changing 'i' to 'o', for italic */
    if (font_names_list)
      XFreeFontNames(font_names_list);
    slant = "o";
    strstr(font_name, "-i-")[1] = 'o';
    font_names_list = XListFonts(iupmot_display, font_name, 32767, &num_fonts);
  }
  if (!num_fonts && bold)
  {
    /* try removing bold */
    if (font_names_list)
      XFreeFontNames(font_names_list);
    weight = "medium";
    sprintf(font_name,"-%s-%s-%s-%s-*-*-*-*-*-*-*-*-*-*", foundry, typeface, weight, slant);
    font_names_list = XListFonts(iupmot_display, font_name, 32767, &num_fonts);
  }
  if (!num_fonts)
    return NULL;

  if (size < 0) /* if in pixels convert to points */
  {
    double res = ((double)DisplayWidth(iupmot_display, iupmot_screen) / (double)DisplayWidthMM(iupmot_display, iupmot_screen)); /* pixels/mm */
    /* 1 point = 1/72 inch     1 inch = 25.4 mm */
    /* pixel = ((point/72)*25.4)*pixel/mm */
    size = iupRound((-size / res)*2.83464567); /* from pixels to points */
  }

  size *= 10; /* convert to deci-points */

  near_size = -1000;
  for (i=0; i<num_fonts; i++)
  {
    font_size = motGetFontSize(font_names_list[i]);

    if (font_size == size)
    {
      near_size = font_size;
      break;
    }

    if (abs(font_size-size) < abs(near_size-size))
      near_size = font_size;
  }

  XFreeFontNames(font_names_list);

  sprintf(font_name,"-%s-%s-%s-%s-*-*-*-%d-*-*-*-*-*-*", foundry, typeface, weight, slant, near_size);
  fontstruct = XLoadQueryFont(iupmot_display, font_name);

  if (fontstruct)
    strcpy(xlfd, font_name); 

  return fontstruct;
}

#ifdef IUP_USE_XFT
static XftFont* motLoadXftFont(const char* typeface, int size, int bold, int italic, char* pattern_out)
{
  XftFont* xftfont;
  char font_name[1024];
  int xft_size;

  if (iupStrEqualNoCase(typeface, "System"))
    typeface = "Sans";

  if (iupStrEqualNoCase(typeface, "fixed"))
    typeface = "Monospace";

  if (size < 0)
  {
    double res = ((double)DisplayWidth(iupmot_display, iupmot_screen) / (double)DisplayWidthMM(iupmot_display, iupmot_screen));
    xft_size = iupRound((-size / res)*2.83464567);
  }
  else
    xft_size = size;

  if (bold && italic)
    sprintf(font_name, "%s:size=%d:weight=bold:slant=italic", typeface, xft_size);
  else if (bold)
    sprintf(font_name, "%s:size=%d:weight=bold", typeface, xft_size);
  else if (italic)
    sprintf(font_name, "%s:size=%d:slant=italic", typeface, xft_size);
  else
    sprintf(font_name, "%s:size=%d", typeface, xft_size);

  xftfont = XftFontOpenName(iupmot_display, iupmot_screen, font_name);

  if (xftfont && pattern_out)
    strcpy(pattern_out, font_name);

  return xftfont;
}
#endif

static XmFontList motFontCreateRenderTable(XFontStruct* fontstruct, int is_underline, int is_strikeout)
{
  XmFontList fontlist;
  XmRendition rendition;
  Arg args[10];
  int num_args = 0;

  iupMOT_SETARG(args, num_args, XmNfontType, XmFONT_IS_FONT);
  iupMOT_SETARG(args, num_args, XmNfont, (XtPointer)fontstruct);
  iupMOT_SETARG(args, num_args, XmNloadModel, XmLOAD_IMMEDIATE);

  if (is_underline)
    iupMOT_SETARG(args, num_args, XmNunderlineType, XmSINGLE_LINE);
  else
    iupMOT_SETARG(args, num_args, XmNunderlineType, XmNO_LINE);

  if (is_strikeout)
    iupMOT_SETARG(args, num_args, XmNstrikethruType, XmSINGLE_LINE);
  else
    iupMOT_SETARG(args, num_args, XmNstrikethruType, XmNO_LINE);

  rendition = XmRenditionCreate(NULL, "", args, num_args);

  fontlist = XmRenderTableAddRenditions(NULL, &rendition, 1, XmDUPLICATE);

  XmRenditionFree(rendition);

  return fontlist;
}

#ifdef IUP_USE_XFT
static XmFontList motFontCreateXftRenderTable(const char* xft_pattern, int is_underline, int is_strikeout, XftFont* xftfont)
{
  XmFontList fontlist;
  XmRendition rendition;
  Arg args[20];
  int num_args = 0;
  char family[256];
  int size = 0;
  int weight = 0;  /* 0=normal, 1=bold */
  int slant = 0;   /* 0=normal, 1=italic */

  /* Parse the XFT pattern to extract family, size, weight, slant */
  {
    const char* p = xft_pattern;
    char* f = family;

    /* Extract family name (everything before first ':') */
    while (*p && *p != ':' && (f - family) < 255)
      *f++ = *p++;
    *f = '\0';

    /* Parse properties after ':' */
    while (*p)
    {
      if (*p == ':')
      {
        p++;
        if (strncmp(p, "size=", 5) == 0)
        {
          size = atoi(p + 5);
          while (*p && *p != ':') p++;
        }
        else if (strncmp(p, "weight=bold", 11) == 0)
        {
          weight = 1;
          while (*p && *p != ':') p++;
        }
        else if (strncmp(p, "slant=italic", 12) == 0)
        {
          slant = 1;
          while (*p && *p != ':') p++;
        }
        else
        {
          /* Skip unknown property */
          while (*p && *p != ':') p++;
        }
      }
      else
        p++;
    }
  }

  /* Set Motif XFT rendition resources separately */
  iupMOT_SETARG(args, num_args, XmNfontName, family);
  iupMOT_SETARG(args, num_args, XmNfontType, XmFONT_IS_XFT);
  if (size > 0)
    iupMOT_SETARG(args, num_args, XmNfontSize, size);

  /* Build style string for XmNfontStyle (e.g., "Bold Italic") */
  if (weight || slant)
  {
    static char style[64];
    style[0] = '\0';
    if (weight)
      strcat(style, "Bold");
    if (slant)
    {
      if (weight) strcat(style, " ");
      strcat(style, "Italic");
    }
    iupMOT_SETARG(args, num_args, XmNfontStyle, style);
  }

  iupMOT_SETARG(args, num_args, XmNloadModel, XmLOAD_IMMEDIATE);

  if (is_underline)
    iupMOT_SETARG(args, num_args, XmNunderlineType, XmSINGLE_LINE);
  else
    iupMOT_SETARG(args, num_args, XmNunderlineType, XmNO_LINE);

  if (is_strikeout)
    iupMOT_SETARG(args, num_args, XmNstrikethruType, XmSINGLE_LINE);
  else
    iupMOT_SETARG(args, num_args, XmNstrikethruType, XmNO_LINE);

  rendition = XmRenditionCreate(iupmot_appshell, "", args, num_args);

  fontlist = XmRenderTableAddRenditions(NULL, &rendition, 1, XmDUPLICATE);

  XmRenditionFree(rendition);

  return fontlist;
}
#endif

static int motFontCalcCharWidth(XFontStruct *fontstruct)
{
  if (fontstruct->per_char)
  {
    int i, all=0;
    int first = fontstruct->min_char_or_byte2;
    int last  = fontstruct->max_char_or_byte2;
    if (first < 32)  first = 32;  /* space */
    if (last  > 126) last  = 126; /* tilde */
    for (i=first; i<=last; i++)
      all += fontstruct->per_char[i].width;
    return all/(last-first + 1);   /* average character width */
  }
  else
    return fontstruct->max_bounds.width;
}

#ifdef IUP_USE_XFT
static int motFontCalcXftCharWidth(XftFont *xftfont)
{
  int i, all = 0;
  XGlyphInfo extents;

  for (i = 32; i <= 126; i++)
  {
    XftTextExtents8(iupmot_display, xftfont, (FcChar8*)&i, 1, &extents);
    all += extents.xOff;
  }
  return all / (126 - 32 + 1);
}
#endif

static ImotFont* motFindFont(const char* foundry, const char *font)
{
  char xlfd[1024];
  XFontStruct* fontstruct = NULL;
  int try_xft = 0;
#ifdef IUP_USE_XFT
  XftFont* xftfont = NULL;
  char xft_pattern[1024];
#endif
  int i, count = iupArrayCount(mot_fonts);
  int is_underline = 0, is_strikeout = 0;

  ImotFont* fonts = (ImotFont*)iupArrayGetData(mot_fonts);

  /* Check if the font already exists in cache */
  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(font, fonts[i].font))
      return &fonts[i];
  }

  /* not found, create a new one */
  if (font[0] == '-')
  {
    fontstruct = XLoadQueryFont(iupmot_display, font);
    if (!fontstruct)
      return NULL;
    strcpy(xlfd, font);
  }
  else
  {
    int size = 0,
        is_bold = 0,
        is_italic = 0;
    char typeface[1024];
    const char* mapped_name;

    if (!iupFontParsePango(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
      return NULL;

    /* Map standard names to native names */
    mapped_name = iupFontGetXName(typeface);
    if (mapped_name)
      strcpy(typeface, mapped_name);

#ifdef IUP_USE_XFT
    xftfont = motLoadXftFont(typeface, size, is_bold, is_italic, xft_pattern);
    if (xftfont)
      try_xft = 1;
#endif

    fontstruct = motLoadFont(foundry, typeface, size, is_bold, is_italic, xlfd);
    if (!fontstruct && !try_xft)
      return NULL;
  }

  /* create room in the array */
  fonts = (ImotFont*)iupArrayInc(mot_fonts);

  strcpy(fonts[i].font, font);
#ifdef IUP_USE_XFT
  if (try_xft && xftfont)
  {
    strcpy(fonts[i].xft_pattern, xft_pattern);
    fonts[i].xftfont = xftfont;
    fonts[i].is_xft = 1;
    fonts[i].fontlist = motFontCreateXftRenderTable(xft_pattern, is_underline, is_strikeout, xftfont);

    if (fontstruct)
    {
      strcpy(fonts[i].xlfd, xlfd);
      fonts[i].fontstruct = fontstruct;
      fonts[i].charwidth = motFontCalcCharWidth(fontstruct);
      fonts[i].charheight = fontstruct->ascent + fontstruct->descent;
    }
    else
    {
      strcpy(fonts[i].xlfd, "");
      fonts[i].fontstruct = NULL;
      fonts[i].charwidth = motFontCalcXftCharWidth(xftfont);
      fonts[i].charheight = xftfont->ascent + xftfont->descent;
    }
  }
  else
#endif
  {
    strcpy(fonts[i].xlfd, xlfd);
    fonts[i].fontstruct = fontstruct;
#ifdef IUP_USE_XFT
    fonts[i].xftfont = NULL;
    strcpy(fonts[i].xft_pattern, "");
    fonts[i].is_xft = 0;
#endif
    fonts[i].fontlist = motFontCreateRenderTable(fontstruct, is_underline, is_strikeout);
    fonts[i].charwidth = motFontCalcCharWidth(fontstruct);
    fonts[i].charheight = fontstruct->ascent + fontstruct->descent;
  }

  return &fonts[i];
}

IUP_SDK_API char* iupdrvGetSystemFont(void)
{
  static char str[200]; /* must return a static string, because it will be used as the default value for the FONT attribute */
  ImotFont* motfont = NULL;
  char* font = XGetDefault(iupmot_display, "Iup", "fontList");
  if (font)
    motfont = motFindFont(NULL, font);

  if (!motfont)
  {
#ifdef IUP_USE_XFT
    font = "Sans, 12";
#else
    font = "Fixed, 11";
#endif
    motFindFont("misc", font);
  }

  strcpy(str, font);
  return str;
}

char* iupmotFindFontList(XmFontList fontlist)
{
  int i, count = iupArrayCount(mot_fonts);
  ImotFont* fonts = (ImotFont*)iupArrayGetData(mot_fonts);

  /* Check if the font already exists in cache */
  for (i = 0; i < count; i++)
  {
    if (fontlist == fonts[i].fontlist)
      return fonts[i].font;
  }

  return NULL;
}

XmFontList iupmotGetFontList(const char* foundry, const char* value)
{
  ImotFont *motfont = motFindFont(foundry, value);
  if (!motfont) 
    return NULL;
  else
    return motfont->fontlist;
}

XFontStruct* iupmotGetFontStruct(const char* value)
{
  ImotFont *motfont = motFindFont(NULL, value);
  if (!motfont) 
    return NULL;
  else
    return motfont->fontstruct;
}

static ImotFont* motFontCreateNativeFont(Ihandle* ih, const char* value)
{
  ImotFont *motfont = motFindFont(iupAttribGet(ih, "FOUNDRY"), value);
  if (!motfont) 
  {
    iupERROR1("Failed to create Font: %s", value); 
    return NULL;
  }

  iupAttribSet(ih, "_IUPMOT_FONT", (char*)motfont);
  iupAttribSet(ih, "XLFD", motfont->xlfd);
  return motfont;
}

static ImotFont* motGetFont(Ihandle *ih)
{
  ImotFont* motfont = (ImotFont*)iupAttribGet(ih, "_IUPMOT_FONT");
  if (!motfont)
  {
    motfont = motFontCreateNativeFont(ih, iupGetFontValue(ih));
    if (!motfont)
      motfont = motFontCreateNativeFont(ih, IupGetGlobal("DEFAULTFONT"));
  }
  return motfont;
}

char* iupmotGetFontListAttrib(Ihandle *ih)
{
  ImotFont* motfont = motGetFont(ih);
  if (!motfont)
    return NULL;
  else
    return (char*)motfont->fontlist;
}

char* iupmotGetFontStructAttrib(Ihandle *ih)
{
  ImotFont* motfont = motGetFont(ih);
  if (!motfont)
    return NULL;
  else
    return (char*)motfont->fontstruct;
}

char* iupmotGetFontIdAttrib(Ihandle *ih)
{
  ImotFont* motfont = motGetFont(ih);
  if (!motfont)
    return NULL;
#ifdef IUP_USE_XFT
  if (motfont->is_xft)
    return NULL;
#endif
  if (!motfont->fontstruct)
    return NULL;
  return (char*)motfont->fontstruct->fid;
}

#ifdef IUP_USE_XFT
void* iupmotGetXftFontAttrib(Ihandle *ih)
{
  ImotFont* motfont = motGetFont(ih);
  if (!motfont)
    return NULL;
  if (motfont->is_xft)
    return (void*)motfont->xftfont;
  return NULL;
}
#endif

IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
  ImotFont *motfont = motFontCreateNativeFont(ih, value);
  if (!motfont)
    return 0;

  /* If FONT is changed, must update the SIZE attribute */
  iupBaseUpdateAttribFromFont(ih);

  /* FONT attribute must be able to be set before mapping,
    so the font is enable for size calculation. */
  if (ih->handle && (ih->iclass->nativetype != IUP_TYPEVOID))
  {
    XtVaSetValues(ih->handle, XmNrenderTable, motfont->fontlist, NULL);
    XtVaSetValues(ih->handle, XmNfontList, motfont->fontlist, NULL);
  }

  return 1;
}

IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  ImotFont* motfont;
  int len;
  char* line_end;

  if (!str || str[0]==0)
    return 0;

  motfont = motGetFont(ih);
  if (!motfont)
    return 0;

  line_end = strchr(str, '\n');
  if (line_end)
    len = (int)(line_end-str);
  else
    len = (int)strlen(str);

#ifdef IUP_USE_XFT
  if (motfont->is_xft && motfont->xftfont)
  {
    XGlyphInfo extents;
    XftTextExtentsUtf8(iupmot_display, motfont->xftfont, (FcChar8*)str, len, &extents);
    return extents.xOff;
  }
  else
#endif
  {
    if (!motfont->fontstruct)
      return 0;
    return XTextWidth(motfont->fontstruct, str, len);
  }
}

static void motFontGetTextSize(ImotFont* motfont, const char* str, int len, int *w, int *h)
{
  int max_w = 0, line_count = 1;

  if (!motfont)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  if (!str)
  {
    if (w) *w = 0;
    if (h) *h = motfont->charheight * 1;
    return;
  }

  if (str[0])
  {
    int l_len, lw, sum_len = 0;
    const char *nextstr;
    const char *curstr = str;

    do
    {
      nextstr = iupStrNextLine(curstr, &l_len);
      if (sum_len + l_len > len)
        l_len = len - sum_len;

      if (l_len)
      {
#ifdef IUP_USE_XFT
        if (motfont->is_xft && motfont->xftfont)
        {
          XGlyphInfo extents;
          XftTextExtentsUtf8(iupmot_display, motfont->xftfont, (FcChar8*)curstr, l_len, &extents);
          lw = extents.xOff;
        }
        else
#endif
        {
          if (motfont->fontstruct)
            lw = XTextWidth(motfont->fontstruct, curstr, l_len);
          else
            lw = 0;
        }
        max_w = iupMAX(max_w, lw);
      }

      sum_len += l_len;
      if (sum_len == len)
        break;

      if (*nextstr)
        line_count++;

      curstr = nextstr;
    } while(*nextstr);
  }

  if (w) *w = max_w;
  if (h) *h = motfont->charheight * line_count;
}

IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int *w, int *h)
{
  ImotFont* motfont = motGetFont(ih);
  if (motfont)
    motFontGetTextSize(motfont, str, str? (int)strlen(str): 0, w, h);
}

IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int *w, int *h)
{
  ImotFont *motfont = motFindFont(NULL, font);
  if (motfont)
    motFontGetTextSize(motfont, str, len, w, h);
}

IUP_SDK_API void iupdrvFontGetFontDim(const char* font, int *max_width, int *line_height, int *ascent, int *descent)
{
  ImotFont *motfont = motFindFont(NULL, font);
  if (motfont)
  {
#ifdef IUP_USE_XFT
    if (motfont->is_xft && motfont->xftfont)
    {
      if (max_width) *max_width = motfont->xftfont->max_advance_width;
      if (line_height) *line_height = motfont->xftfont->ascent + motfont->xftfont->descent;
      if (ascent)    *ascent = motfont->xftfont->ascent;
      if (descent)   *descent = motfont->xftfont->descent;
    }
    else
#endif
    {
      if (motfont->fontstruct)
      {
        if (max_width) *max_width = motfont->fontstruct->max_bounds.width;
        if (line_height) *line_height = motfont->fontstruct->ascent + motfont->fontstruct->descent;
        if (ascent)    *ascent = motfont->fontstruct->ascent;
        if (descent)   *descent = motfont->fontstruct->descent;
      }
    }
  }
}

IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int *charwidth, int *charheight)
{
  ImotFont* motfont = motGetFont(ih);
  if (!motfont)
  {
    if (charwidth)  *charwidth = 0;
    if (charheight) *charheight = 0;
    return;
  }

  if (charheight) 
    *charheight = motfont->charheight;

  if (charwidth)
    *charwidth = motfont->charwidth;
}

void iupdrvFontInit(void)
{
  mot_fonts = iupArrayCreate(50, sizeof(ImotFont));
}

void iupdrvFontFinish(void)
{
  int i, count = iupArrayCount(mot_fonts);
  ImotFont* fonts = (ImotFont*)iupArrayGetData(mot_fonts);
  for (i = 0; i < count; i++)
  {
    XmFontListFree(fonts[i].fontlist);
    fonts[i].fontlist = NULL;
#ifdef IUP_USE_XFT
    if (fonts[i].is_xft && fonts[i].xftfont)
    {
      XftFontClose(iupmot_display, fonts[i].xftfont);
      fonts[i].xftfont = NULL;
    }
    else
#endif
    {
      if (fonts[i].fontstruct)
      {
        XFreeFont(iupmot_display, fonts[i].fontstruct);
        fonts[i].fontstruct = NULL;
      }
    }
  }
  iupArrayDestroy(mot_fonts);
}
