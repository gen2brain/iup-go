/** \file
 * \brief Motif Font mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include <Xm/Xm.h>
#include <Xm/XmP.h>

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

static ImotFont* motFindFont(const char* foundry, const char *font)
{
  char xlfd[1024];
  XFontStruct* fontstruct;
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

    fontstruct = motLoadFont(foundry, typeface, size, is_bold, is_italic, xlfd);
    if (!fontstruct) 
      return NULL;
  }

  /* create room in the array */
  fonts = (ImotFont*)iupArrayInc(mot_fonts);

  strcpy(fonts[i].font, font);
  strcpy(fonts[i].xlfd, xlfd);
  fonts[i].fontstruct = fontstruct;
  fonts[i].fontlist = motFontCreateRenderTable(fontstruct, is_underline, is_strikeout);
  fonts[i].charwidth = motFontCalcCharWidth(fontstruct);
  fonts[i].charheight = fontstruct->ascent + fontstruct->descent;

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
    font = "Fixed, 11";
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
  else
    return (char*)motfont->fontstruct->fid;
}

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
    XtVaSetValues(ih->handle, XmNrenderTable, motfont->fontlist, NULL);

  return 1;
}

IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
  XFontStruct* fontstruct;
  int len;
  char* line_end;

  if (!str || str[0]==0)
    return 0;

  fontstruct = (XFontStruct*)iupmotGetFontStructAttrib(ih);
  if (!fontstruct)
    return 0;

  line_end = strchr(str, '\n');
  if (line_end)
    len = (int)(line_end-str);
  else
    len = (int)strlen(str);

  return XTextWidth(fontstruct, str, len);
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
        lw = XTextWidth(motfont->fontstruct, curstr, l_len);
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
    if (max_width) *max_width = motfont->fontstruct->max_bounds.width;
    if (line_height) *line_height = motfont->fontstruct->ascent + motfont->fontstruct->descent;
    if (ascent)    *ascent = motfont->fontstruct->ascent;
    if (descent)   *descent = motfont->fontstruct->descent;
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
    XFreeFont(iupmot_display, fonts[i].fontstruct);
    fonts[i].fontstruct = NULL;
  }
  iupArrayDestroy(mot_fonts);
}
