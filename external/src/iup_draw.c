/** \file
 * \brief Canvas Control.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupdraw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_assert.h"
#include "iup_image.h"



IUP_API void IupDrawBegin(Ihandle* ih)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = iupdrvDrawCreateCanvas(ih);
  iupAttribSet(ih, "_IUP_DRAW_DC", (char*)dc);
}

IUP_API void IupDrawEnd(Ihandle* ih)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawFlush(dc);
  iupdrvDrawKillCanvas(dc);
  iupAttribSet(ih, "_IUP_DRAW_DC", NULL);
}

IUP_API void IupDrawGetSize(Ihandle* ih, int *w, int *h)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawGetSize(dc, w, h);
}

IUP_API void IupDrawParentBackground(Ihandle* ih)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupDrawParentBackground(dc, ih);
}

static int iDrawGetStyle(Ihandle* ih)
{
  char* style = iupAttribGetStr(ih, "DRAWSTYLE");
  if (iupStrEqualNoCase(style, "FILL"))
    return IUP_DRAW_FILL;
  else if (iupStrEqualNoCase(style, "STROKE_DASH"))
    return IUP_DRAW_STROKE_DASH;
  else if (iupStrEqualNoCase(style, "STROKE_DOT"))
    return IUP_DRAW_STROKE_DOT;
  else if (iupStrEqualNoCase(style, "STROKE_DASH_DOT"))
    return IUP_DRAW_STROKE_DASH_DOT;
  else if (iupStrEqualNoCase(style, "STROKE_DASH_DOT_DOT"))
    return IUP_DRAW_STROKE_DASH_DOT_DOT;
  else
    return IUP_DRAW_STROKE;
}

static int iDrawGetLineWidth(Ihandle* ih)
{
  int line_width = iupAttribGetInt(ih, "DRAWLINEWIDTH");
  if (line_width == 0)
    return 1;
  else
    return line_width;
}

IUP_API void IupDrawLine(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;
  long color = 0;
  int style, line_width;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);

  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  iupdrvDrawLine(dc, x1, y1, x2, y2, color, style, line_width);
}

IUP_API void IupDrawRectangle(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;
  long color;
  int style, line_width;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);

  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  iupdrvDrawRectangle(dc, x1, y1, x2, y2, color, style, line_width);
}

IUP_API void IupDrawArc(Ihandle* ih, int x1, int y1, int x2, int y2, double a1, double a2)
{
  IdrawCanvas* dc;
  long color = 0;
  int style, line_width;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);

  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  iupdrvDrawArc(dc, x1, y1, x2, y2, a1, a2, color, style, line_width);
}

IUP_API void IupDrawPolygon(Ihandle* ih, int* points, int count)
{
  IdrawCanvas* dc;
  long color = 0;
  int style, line_width;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);

  line_width = iDrawGetLineWidth(ih);
  style = iDrawGetStyle(ih);

  iupdrvDrawPolygon(dc, points, count, color, style, line_width);
}

static void iDrawRotatePoint(int x, int y, int *rx, int *ry, double sin_theta, double cos_theta)
{
  double t;
  t = (x * cos_theta) - (y * sin_theta); *rx = iupROUND(t);
  t = (x * sin_theta) + (y * cos_theta); *ry = iupROUND(t);
}

static void iDrawGetTextBounds(int w, int h, double text_orientation, int *o_w, int *o_h)
{
  int xmin, xmax, ymin, ymax, x_r, y_r;

  double cos_theta = cos(text_orientation * IUP_DEG2RAD);
  double sin_theta = sin(text_orientation * IUP_DEG2RAD);

  iDrawRotatePoint(0, 0, &x_r, &y_r, sin_theta, cos_theta);
  xmax = xmin = x_r;
  ymax = ymin = y_r;
  iDrawRotatePoint(w - 1, 0, &x_r, &y_r, sin_theta, cos_theta);
  xmin = iupMIN(xmin, x_r);
  ymin = iupMIN(ymin, y_r);
  xmax = iupMAX(xmax, x_r);
  ymax = iupMAX(ymax, y_r);
  iDrawRotatePoint(w - 1, h - 1, &x_r, &y_r, sin_theta, cos_theta);
  xmin = iupMIN(xmin, x_r);
  ymin = iupMIN(ymin, y_r);
  xmax = iupMAX(xmax, x_r);
  ymax = iupMAX(ymax, y_r);
  iDrawRotatePoint(0, h - 1, &x_r, &y_r, sin_theta, cos_theta);
  xmin = iupMIN(xmin, x_r);
  ymin = iupMIN(ymin, y_r);
  xmax = iupMAX(xmax, x_r);
  ymax = iupMAX(ymax, y_r);

  if (o_w) *o_w = xmax - xmin + 1;
  if (o_h) *o_h = ymax - ymin + 1;
}

IUP_SDK_API char* iupDrawGetTextSize(Ihandle* ih, const char* text, int len, int *w, int *h, double text_orientation)
{
  char*font = iupAttribGetStr(ih, "DRAWFONT");
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  if (len == 0 || len == -1)
    len = (int)strlen(text);

  if (len == 0)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return font;
  }

  if (text_orientation)
  {
    if (text_orientation == 90)
      iupdrvFontGetTextSize(font, text, len, h, w);
    else
    {
      int txt_w, txt_h;
      iupdrvFontGetTextSize(font, text, len, &txt_w, &txt_h);
      iDrawGetTextBounds(txt_w, txt_h, text_orientation, w, h);
    }
  }
  else
    iupdrvFontGetTextSize(font, text, len, w, h);

  return font;
}

IUP_SDK_API int iupDrawGetTextFlags(Ihandle* ih, const char* align_name, const char* wrap_name, const char* ellipsis_name)
{
  int flags = iupFlatGetHorizontalAlignment(iupAttribGetStr(ih, align_name));
  int wrap = iupAttribGetBoolean(ih, wrap_name);
  int ellipsis = iupAttribGetBoolean(ih, ellipsis_name);
  if (wrap)
    flags |= IUP_DRAW_WRAP;
  if (ellipsis)
    flags |= IUP_DRAW_ELLIPSIS;
  return flags;
}

IUP_API void IupDrawText(Ihandle* ih, const char* text, int len, int x, int y, int w, int h)
{
  IdrawCanvas* dc;
  long color = 0;
  int text_flags;
  double text_orientation;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  iupASSERT(text);
  if (!text || text[0] == 0)
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  color = iupDrawStrToColor(iupAttribGetStr(ih, "DRAWCOLOR"), 0);
  text_orientation = iupAttribGetDouble(ih, "DRAWTEXTORIENTATION");
  text_flags = iupDrawGetTextFlags(ih, "DRAWTEXTALIGNMENT", "DRAWTEXTWRAP", "DRAWTEXTELLIPSIS");
  if (iupAttribGetBoolean(ih, "DRAWTEXTCLIP"))
    text_flags |= IUP_DRAW_CLIP;
  if (iupAttribGetBoolean(ih, "DRAWTEXTLAYOUTCENTER"))
    text_flags |= IUP_DRAW_LAYOUTCENTER;

  if (len == 0 || len == -1)
    len = (int)strlen(text);

  if (len != 0)
  {
    int txt_w, txt_h;
    char* font = iupDrawGetTextSize(ih, text, len, &txt_w, &txt_h, text_orientation);
    if (w == -1 || w == 0) w = txt_w;
    if (h == -1 || h == 0) h = txt_h;
    iupdrvDrawText(dc, text, len, x, y, w, h, color, font, text_flags, text_orientation);
  }
}

IUP_API void IupDrawGetTextSize(Ihandle* ih, const char* text, int len, int *w, int *h)
{
  double text_orientation;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  iupASSERT(text);
  if (!text)
    return;

  text_orientation = iupAttribGetDouble(ih, "DRAWTEXTORIENTATION");

  iupDrawGetTextSize(ih, text, len, w, h, text_orientation);
}

IUP_API void IupDrawGetImageInfo(const char* name, int *w, int *h, int *bpp)
{
  iupImageGetInfo(name, w, h, bpp);
}

IUP_API void IupDrawImage(Ihandle* ih, const char* name, int x, int y, int w, int h)
{
  IdrawCanvas* dc;
  char* bgcolor;
  int make_inactive;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  bgcolor = iupAttribGetStr(ih, "DRAWBGCOLOR");
  make_inactive = iupAttribGetInt(ih, "DRAWMAKEINACTIVE");

  iupdrvDrawImage(dc, name, make_inactive, bgcolor, x, y, w, h);
}

IUP_API void IupDrawSetClipRect(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawSetClipRect(dc, x1, y1, x2, y2);
}

IUP_API void IupDrawGetClipRect(Ihandle* ih, int *x1, int *y1, int *x2, int *y2)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawGetClipRect(dc, x1, y1, x2, y2);
}

IUP_API void IupDrawResetClip(Ihandle* ih)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawResetClip(dc);
}

IUP_API void IupDrawSelectRect(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawSelectRect(dc, x1, y1, x2, y2);
}

IUP_API void IupDrawFocusRect(Ihandle* ih, int x1, int y1, int x2, int y2)
{
  IdrawCanvas* dc;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  dc = (IdrawCanvas*)iupAttribGet(ih, "_IUP_DRAW_DC");
  if (!dc)
    return;

  iupdrvDrawFocusRect(dc, x1, y1, x2, y2);
}


/************************************************************************************************/


IUP_SDK_API long iupDrawColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  a = ~a;
  return (((unsigned long)a) << 24) |
         (((unsigned long)r) << 16) |
         (((unsigned long)g) << 8)  |
         (((unsigned long)b) << 0);
}

IUP_SDK_API long iupDrawColorMakeInactive(long color, long bgcolor)
{
  unsigned char r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color), a = iupDrawAlpha(color);
  unsigned char bg_r = iupDrawRed(bgcolor), bg_g = iupDrawGreen(bgcolor), bg_b = iupDrawBlue(bgcolor);
  iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);
  return iupDrawColor(r, g, b, a);
}

IUP_SDK_API long iupDrawStrToColor(const char* str, long c_def)
{
  unsigned char r, g, b, a;
  if (iupStrToRGBA(str, &r, &g, &b, &a))
    return iupDrawColor(r, g, b, a);
  else
    return c_def;
}

IUP_SDK_API void iupDrawSetColor(Ihandle *ih, const char* name, long color)
{
  char value[60];
  sprintf(value, "%d %d %d", (int)iupDrawRed(color), (int)iupDrawGreen(color), (int)iupDrawBlue(color));
  iupAttribSetStr(ih, name, value);
}

IUP_SDK_API void iupDrawRaiseRect(Ihandle *ih, int x1, int y1, int x2, int y2, long light_shadow, long mid_shadow, long dark_shadow)
{
  iupDrawSetColor(ih, "DRAWCOLOR", light_shadow);
  IupDrawLine(ih, x1, y1, x1, y2);
  IupDrawLine(ih, x1, y1, x2, y1);

  iupDrawSetColor(ih, "DRAWCOLOR", mid_shadow);
  IupDrawLine(ih, x1 + 1, y2 - 1, x2 - 1, y2 - 1);
  IupDrawLine(ih, x2 - 1, y1 + 1, x2 - 1, y2 - 1);

  iupDrawSetColor(ih, "DRAWCOLOR", dark_shadow);
  IupDrawLine(ih, x1, y2, x2, y2);
  IupDrawLine(ih, x2, y1, x2, y2);
}

IUP_SDK_API void iupDrawVertSunkenMark(Ihandle *ih, int x, int y1, int y2, long light_shadow, long dark_shadow)
{
  iupDrawSetColor(ih, "DRAWCOLOR", dark_shadow);
  IupDrawLine(ih, x - 1, y1, x - 1, y2);
  iupDrawSetColor(ih, "DRAWCOLOR", light_shadow);
  IupDrawLine(ih, x, y1, x, y2);
}

IUP_SDK_API void iupDrawHorizSunkenMark(Ihandle *ih, int x1, int x2, int y, long light_shadow, long dark_shadow)
{
  iupDrawSetColor(ih, "DRAWCOLOR", dark_shadow);
  IupDrawLine(ih, x1, y - 1, x2, y - 1);
  iupDrawSetColor(ih, "DRAWCOLOR", light_shadow);
  IupDrawLine(ih, x1, y, x2, y);
}

IUP_SDK_API void iupDrawSunkenRect(Ihandle *ih, int x1, int y1, int x2, int y2, long light_shadow, long mid_shadow, long dark_shadow)
{
  iupDrawSetColor(ih, "DRAWCOLOR", mid_shadow);
  IupDrawLine(ih, x1, y1, x1, y2);
  IupDrawLine(ih, x1, y1, x2, y1);

  iupDrawSetColor(ih, "DRAWCOLOR", dark_shadow);
  IupDrawLine(ih, x1 + 1, y1 + 1, x1 + 1, y2 - 1);
  IupDrawLine(ih, x1 + 1, y1 + 1, x2 - 1, y1 + 1);

  iupDrawSetColor(ih, "DRAWCOLOR", light_shadow);
  IupDrawLine(ih, x1, y2, x2, y2);
  IupDrawLine(ih, x2, y1, x2, y2);
}

IUP_SDK_API void iupDrawCalcShadows(long bgcolor, long *light_shadow, long *mid_shadow, long *dark_shadow)
{
  int r, bg_r = iupDrawRed(bgcolor);
  int g, bg_g = iupDrawGreen(bgcolor);
  int b, bg_b = iupDrawBlue(bgcolor);

  /* light_shadow */

  int max = bg_r;
  if (bg_g > max) max = bg_g;
  if (bg_b > max) max = bg_b;

  if (255 - max < 64)
  {
    r = 255;
    g = 255;
    b = 255;
  }
  else
  {
    /* preserve some color information */
    if (bg_r == max) r = 255;
    else             r = bg_r + (255 - max);
    if (bg_g == max) g = 255;
    else             g = bg_g + (255 - max);
    if (bg_b == max) b = 255;
    else             b = bg_b + (255 - max);
  }

  if (light_shadow) *light_shadow = iupDrawColor((unsigned char)r, (unsigned char)g, (unsigned char)b, 255);

  /* dark_shadow */
  r = bg_r - 128;
  g = bg_g - 128;
  b = bg_b - 128;
  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;

  if (dark_shadow) *dark_shadow = iupDrawColor((unsigned char)r, (unsigned char)g, (unsigned char)b, 255);

  /*   mid_shadow = (dark_shadow+bgcolor)/2    */
  if (mid_shadow) *mid_shadow = iupDrawColor((unsigned char)((bg_r + r) / 2), (unsigned char)((bg_g + g) / 2), (unsigned char)((bg_b + b) / 2), 255);
}

IUP_SDK_API void iupDrawParentBackground(IdrawCanvas* dc, Ihandle* ih)
{
  long color;
  int w, h;
  char* color_str = iupBaseNativeParentGetBgColorAttrib(ih);
  color = iupDrawStrToColor(color_str, 0);
  iupdrvDrawGetSize(dc, &w, &h);
  iupdrvDrawRectangle(dc, 0, 0, w - 1, h - 1, color, IUP_DRAW_FILL, 1);
}


/***********************************************************************************************/

static long iFlatDrawColorMakeInactive(long color, const char* bgcolor)
{
  unsigned char bg_r = 0, bg_g = 0, bg_b = 0;
  unsigned char r = iupDrawRed(color), g = iupDrawGreen(color), b = iupDrawBlue(color), a = iupDrawAlpha(color);
  iupStrToRGB(bgcolor, &bg_r, &bg_g, &bg_b);
  iupImageColorMakeInactive(&r, &g, &b, bg_r, bg_g, bg_b);
  return iupDrawColor(r, g, b, a);
}

IUP_SDK_API void iupFlatDrawBorder(IdrawCanvas* dc, int xmin, int xmax, int ymin, int ymax, int border_width, const char* fgcolor, const char* bgcolor, int active)
{
  long color = 0;

  if (!fgcolor || border_width == 0 || xmin == xmax || ymin == ymax)
    return;

  iupDrawCheckSwapCoord(xmin, xmax);
  iupDrawCheckSwapCoord(ymin, ymax);

  color = iupDrawStrToColor(fgcolor, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  iupdrvDrawRectangle(dc, xmin, ymin, xmax, ymax, color, IUP_DRAW_STROKE, 1);
  while (border_width > 1)
  {
    border_width--;
    iupdrvDrawRectangle(dc, xmin + border_width,
                        ymin + border_width,
                        xmax - border_width,
                        ymax - border_width, color, IUP_DRAW_STROKE, 1);
  }
}

IUP_SDK_API void iupFlatDrawBox(IdrawCanvas* dc, int xmin, int xmax, int ymin, int ymax, const char* fgcolor, const char* bgcolor, int active)
{
  long color;

  if (!fgcolor || xmin == xmax || ymin == ymax)
    return;

  iupDrawCheckSwapCoord(xmin, xmax);
  iupDrawCheckSwapCoord(ymin, ymax);

  color = iupDrawStrToColor(fgcolor, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  iupdrvDrawRectangle(dc, xmin, ymin, xmax, ymax, color, IUP_DRAW_FILL, 1);
}

static void iFlatDrawText(IdrawCanvas* dc, int x, int y, int w, int h, const char* str, const char* font, int text_flags, double text_orientation, const char* fgcolor, const char* bgcolor, int active)
{
  long color;

  if (!fgcolor || !str || str[0] == 0)
    return;

  color = iupDrawStrToColor(fgcolor, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  iupdrvDrawText(dc, str, (int)strlen(str), x, y, w, h, color, font, text_flags | IUP_DRAW_LAYOUTCENTER, text_orientation);  /* layout is always center here */
}

static void iFlatGetIconPosition(int icon_width, int icon_height, int *x, int *y, int width, int height, int horiz_alignment, int vert_alignment)
{
  if (horiz_alignment == IUP_ALIGN_ARIGHT)
    *x = icon_width - width;
  else if (horiz_alignment == IUP_ALIGN_ACENTER)
    *x = (icon_width - width) / 2;
  else  /* ALEFT */
    *x = 0;

  if (vert_alignment == IUP_ALIGN_ABOTTOM)
    *y = icon_height - height;
  else if (vert_alignment == IUP_ALIGN_ACENTER)
    *y = (icon_height - height) / 2;
  else  /* ATOP */
    *y = 0;
}

static void iFlatGetImageTextPosition(int x, int y, int img_position, int spacing,
                                        int img_width, int img_height, int txt_width, int txt_height,
                                        int *img_x, int *img_y, int *txt_x, int *txt_y)
{
  switch (img_position)
  {
  case IUP_IMGPOS_TOP:
    *img_y = y;
    *txt_y = y + img_height + spacing;
    if (img_width > txt_width)
    {
      *img_x = x;
      *txt_x = x + (img_width - txt_width) / 2;
    }
    else
    {
      *img_x = x + (txt_width - img_width) / 2;
      *txt_x = x;
    }
    break;
  case IUP_IMGPOS_BOTTOM:
    *img_y = y + txt_height + spacing;
    *txt_y = y;
    if (img_width > txt_width)
    {
      *img_x = x;
      *txt_x = x + (img_width - txt_width) / 2;
    }
    else
    {
      *img_x = x + (txt_width - img_width) / 2;
      *txt_x = x;
    }
    break;
  case IUP_IMGPOS_RIGHT:
    *img_x = x + txt_width + spacing;
    *txt_x = x;
    if (img_height > txt_height)
    {
      *img_y = y;
      *txt_y = y + (img_height - txt_height) / 2;
    }
    else
    {
      *img_y = y + (txt_height - img_height) / 2;
      *txt_y = y;
    }
    break;
  default: /* IUP_IMGPOS_LEFT (image at left of text) */
    *img_x = x;
    *txt_x = x + img_width + spacing;
    if (img_height > txt_height)
    {
      *img_y = y;
      *txt_y = y + (img_height - txt_height) / 2;
    }
    else
    {
      *img_y = y + (txt_height - img_height) / 2;
      *txt_y = y;
    }
    break;
  }
}

IUP_SDK_API void iupFlatDrawGetIconSize(Ihandle* ih, int img_position, int spacing, int horiz_padding, int vert_padding,
                            const char* imagename, const char* title, int *w, int *h, double text_orientation)
{
  if (imagename)
  {
    int img_width = 0, img_height = 0;
    iupImageGetInfo(imagename, &img_width, &img_height, NULL);

    if (title)
    {
      int txt_width, txt_height;
      iupDrawGetTextSize(ih, title, 0, &txt_width, &txt_height, text_orientation);

      if (img_position == IUP_IMGPOS_RIGHT || img_position == IUP_IMGPOS_LEFT)
      {
        *w = img_width + txt_width + spacing;
        *h = iupMAX(img_height, txt_height);
      }
      else
      {
        *w = iupMAX(img_width, txt_width);
        *h = img_height + txt_height + spacing;
      }
    }
    else
    {
      *w = img_width;
      *h = img_height;
    }
  }
  else if (title)
  {
    int txt_width, txt_height;
    iupDrawGetTextSize(ih, title, 0, &txt_width, &txt_height, text_orientation);

    *w = txt_width;
    *h = txt_height;
  }
  else
  {
    *w = 0;
    *h = 0;
  }

  *w += 2 * horiz_padding;
  *h += 2 * vert_padding;

  /* leave room for focus feedback */
  if (ih->iclass->is_interactive && iupAttribGetBoolean(ih, "CANFOCUS") && iupAttribGetBoolean(ih, "FOCUSFEEDBACK"))
  {
    *w += 2 * 2;  /* space between focus rect and contents */
    *h += 2 * 2;
  }
}

IUP_SDK_API void iupFlatDrawIcon(Ihandle* ih, IdrawCanvas* dc, int icon_x, int icon_y, int icon_width, int icon_height,
                     int img_position, int spacing, int horiz_alignment, int vert_alignment, int horiz_padding, int vert_padding,
                     const char* imagename, int make_inactive, const char* title, int text_flags, double text_orientation, const char* fgcolor, const char* bgcolor, int active)
{
  int x, y, width, height;
  int txt_width, txt_height;
  char* font;
  int clip_x1, clip_y1, clip_x2, clip_y2;

  iupdrvDrawGetClipRect(dc, &clip_x1, &clip_y1, &clip_x2, &clip_y2);
  if (clip_x1 != 0 || clip_y1 != 0 || clip_x2 != 0 || clip_y2)
    iupdrvDrawSetClipRect(dc, iupMAX(icon_x, clip_x1), iupMAX(icon_y, clip_y1), iupMIN(icon_x + icon_width, clip_x2), iupMIN(icon_y + icon_height, clip_y2));  /* intersect */
  else
    iupdrvDrawSetClipRect(dc, icon_x, icon_y, icon_x + icon_width, icon_y + icon_height);

  /* clipping allows the padding area to be drawn */
  icon_width -= 2 * horiz_padding;
  icon_height -= 2 * vert_padding;
  icon_x += horiz_padding;
  icon_y += vert_padding;

  if (imagename)
  {
    int img_width = 0, img_height = 0;
    iupImageGetInfo(imagename, &img_width, &img_height, NULL);

    if (title)
    {
      int img_x, img_y, txt_x, txt_y;

      font = iupDrawGetTextSize(ih, title, 0, &txt_width, &txt_height, text_orientation);

      /* first combine image and text */
      if (img_position == IUP_IMGPOS_RIGHT || img_position == IUP_IMGPOS_LEFT)
      {
        int max_txt_width = icon_width - img_width - spacing;
        if (max_txt_width < 0) max_txt_width = 0;

        /* the text can be larger than the icon space, so wrap and ellipsis can work */
        txt_width = iupMIN(max_txt_width, txt_width);
        if (text_flags & IUP_DRAW_WRAP)
          txt_height = icon_height;
        else
          txt_height = iupMIN(icon_height, txt_height);

        width = img_width + txt_width + spacing;
        height = iupMAX(img_height, txt_height);
      }
      else
      {
        int max_txt_height = icon_height - img_height - spacing;
        if (max_txt_height < 0) max_txt_height = 0;

        /* the text can be larger than the icon space, so wrap and ellipsis can work */
        txt_width = iupMIN(icon_width, txt_width);
        if (text_flags & IUP_DRAW_WRAP)
          txt_height = max_txt_height;
        else
          txt_height = iupMIN(max_txt_height, txt_height);

        width = iupMAX(img_width, txt_width);
        height = img_height + txt_height + spacing;
      }

      iFlatGetIconPosition(icon_width, icon_height, &x, &y, width, height, horiz_alignment, vert_alignment);

      iFlatGetImageTextPosition(x, y, img_position, spacing,
                                img_width, img_height, txt_width, txt_height,
                                &img_x, &img_y, &txt_x, &txt_y);

      iupdrvDrawImage(dc, imagename, make_inactive, bgcolor, icon_x + img_x, icon_y + img_y, img_width, img_height);  /* no zoom */
      iFlatDrawText(dc, icon_x + txt_x, icon_y + txt_y, txt_width, txt_height, title, font, text_flags, text_orientation, fgcolor, bgcolor, active);
    }
    else
    {
      /* if image is larger than the icon space, then the position can be negative, clipping will crop the result */
      width = img_width;
      height = img_height;

      iFlatGetIconPosition(icon_width, icon_height, &x, &y, width, height, horiz_alignment, vert_alignment);

      iupdrvDrawImage(dc, imagename, make_inactive, bgcolor, icon_x + x, icon_y + y, img_width, img_height);  /* no zoom */
    }
  }
  else if (title)
  {
    font = iupDrawGetTextSize(ih, title, 0, &txt_width, &txt_height, text_orientation);

    /* the text can be larger than the icon space, so wrap and ellipsis can work */
    width = iupMIN(icon_width, txt_width);
    if (text_flags & IUP_DRAW_WRAP)
      height = icon_height;
    else
      height = iupMIN(icon_height, txt_height);

    iFlatGetIconPosition(icon_width, icon_height, &x, &y, width, height, horiz_alignment, vert_alignment);

    iFlatDrawText(dc, icon_x + x, icon_y + y, width, height, title, font, text_flags, text_orientation, fgcolor, bgcolor, active);
  }

  iupdrvDrawSetClipRect(dc, clip_x1, clip_y1, clip_x2, clip_y2);

  if (title && !iupAttribGet(ih, "SECONDARYTITLE"))
    iupdrvSetAccessibleTitle(ih, title);  /* for accessibility */
}

IUP_SDK_API int iupFlatGetHorizontalAlignment(const char* value)
{
  int horiz_alignment = IUP_ALIGN_ACENTER;  /* default always "ACENTER" */
  if (iupStrEqualNoCase(value, "ARIGHT"))
    horiz_alignment = IUP_ALIGN_ARIGHT;
  else if (iupStrEqualNoCase(value, "ALEFT"))
    horiz_alignment = IUP_ALIGN_ALEFT;
  return horiz_alignment;
}

IUP_SDK_API int iupFlatGetVerticalAlignment(const char* value)
{
  int vert_alignment = IUP_ALIGN_ACENTER;  /* default always "ACENTER" */
  if (iupStrEqualNoCase(value, "ABOTTOM"))
    vert_alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value, "ATOP"))
    vert_alignment = IUP_ALIGN_ATOP;
  return vert_alignment;
}

IUP_SDK_API int iupFlatGetImagePosition(const char* value)
{
  int img_position = IUP_IMGPOS_LEFT; /* default always "LEFT" */
  if (iupStrEqualNoCase(value, "RIGHT"))
    img_position = IUP_IMGPOS_RIGHT;
  else if (iupStrEqualNoCase(value, "BOTTOM"))
    img_position = IUP_IMGPOS_BOTTOM;
  else if (iupStrEqualNoCase(value, "TOP"))
    img_position = IUP_IMGPOS_TOP;
  return img_position;
}

IUP_SDK_API void iupFlatDrawArrow(IdrawCanvas* dc, int x, int y, int size, const char* color_str, const char* bgcolor, int active, int dir)
{
  int points[6];

  int off1 = iupRound((double)size * 0.13);
  int off2 = iupRound((double)size * 0.87);
  int half = size / 2;

  long color = iupDrawStrToColor(color_str, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  switch (dir)
  {
  case IUPDRAW_ARROW_LEFT:  /* arrow points left */
    points[0] = x + off2;
    points[1] = y;
    points[2] = x + off2;
    points[3] = y + size;
    points[4] = x + off1;
    points[5] = y + half;
    break;
  case IUPDRAW_ARROW_TOP:    /* arrow points top */
    points[0] = x;
    points[1] = y + off2;
    points[2] = x + size;
    points[3] = y + off2;
    points[4] = x + half;
    points[5] = y + off1;
    break;
  case IUPDRAW_ARROW_RIGHT:  /* arrow points right */
    points[0] = x + off1;
    points[1] = y;
    points[2] = x + off1;
    points[3] = y + size;
    points[4] = x + size - off1;
    points[5] = y + half;
    break;
  case IUPDRAW_ARROW_BOTTOM:  /* arrow points bottom */
    points[0] = x;
    points[1] = y + off1;
    points[2] = x + size;
    points[3] = y + off1;
    points[4] = x + half;
    points[5] = y + size - off1;
    break;
  }

  iupdrvDrawPolygon(dc, points, 3, color, IUP_DRAW_FILL, 1);
  iupdrvDrawPolygon(dc, points, 3, color, IUP_DRAW_STROKE, 1);
}

IUP_SDK_API void iupFlatDrawCheckMark(IdrawCanvas* dc, int xmin, int xmax, int ymin, int ymax, const char* color_str, const char* bgcolor, int active)
{
  int points[6];

  long color = iupDrawStrToColor(color_str, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  points[0] = xmin;
  points[1] = (ymax + ymin) / 2;
  points[2] = (xmax + xmin) / 2;
  points[3] = ymax;
  points[4] = xmax;
  points[5] = ymin;

  iupdrvDrawPolygon(dc, points, 3, color, IUP_DRAW_STROKE, 2);
}

IUP_SDK_API void iupFlatDrawDrawCircle(IdrawCanvas* dc, int xc, int yc, int radius, int fill, int line_width, char *fgcolor, char *bgcolor, int active)
{
  int x1, y1, x2, y2;
  int style = (fill) ? IUP_DRAW_FILL : IUP_DRAW_STROKE;

  long color = iupDrawStrToColor(fgcolor, 0);
  if (!active)
    color = iFlatDrawColorMakeInactive(color, bgcolor);

  x1 = xc - radius;
  y1 = yc - radius;
  x2 = xc + radius;
  y2 = yc + radius;

  iupdrvDrawArc(dc, x1, y1, x2, y2, 0.0, 360, color, style, line_width);
}

static char* iFlatDrawGetImageName(Ihandle* ih, const char* baseattrib, const char* state)
{
  char attrib[1024];
  strcpy(attrib, baseattrib);
  strcat(attrib, state);
  return iupAttribGetStr(ih, attrib);
}

IUP_SDK_API const char* iupFlatGetImageName(Ihandle* ih, const char* baseattrib, const char* basevalue, int press, int highlight, int active, int *make_inactive)
{
  const char* imagename = NULL;

  *make_inactive = 0;

  if (active)
  {
    if (press)
      imagename = iFlatDrawGetImageName(ih, baseattrib, "PRESS");
    else
    {
      if (highlight)
        imagename = iFlatDrawGetImageName(ih, baseattrib, "HIGHLIGHT");
    }
  }
  else
  {
    imagename = iFlatDrawGetImageName(ih, baseattrib, "INACTIVE");
    if (!imagename)
      *make_inactive = 1;
  }

  if (!imagename)
  {
    if (!basevalue)
      basevalue = iupAttribGetStr(ih, baseattrib);

    imagename = basevalue;
  }

  return imagename;
}

static char* iFlatDrawGetImageNameId(Ihandle* ih, const char* baseattrib, const char* state, int id)
{
  char attrib[1024];
  strcpy(attrib, baseattrib);
  strcat(attrib, state);
  return iupAttribGetId(ih, attrib, id);
}

IUP_SDK_API const char* iupFlatGetImageNameId(Ihandle* ih, const char* baseattrib, int id, const char* basevalue, int press, int highlight, int active, int *make_inactive)
{
  const char* imagename = NULL;

  *make_inactive = 0;

  if (active)
  {
    if (press == id)
      imagename = iFlatDrawGetImageNameId(ih, baseattrib, "PRESS", id);
    else
    {
      if (highlight == id)
        imagename = iFlatDrawGetImageNameId(ih, baseattrib, "HIGHLIGHT", id);
    }
  }
  else
  {
    imagename = iFlatDrawGetImageNameId(ih, baseattrib, "INACTIVE", id);
    if (!imagename)
      *make_inactive = 1;
  }

  if (!imagename)
  {
    if (!basevalue)
      basevalue = iupAttribGetId(ih, baseattrib, id);

    imagename = basevalue;
  }

  return imagename;
}

IUP_SDK_API char* iupFlatGetDarkerBgColor(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "DARK_DLGBGCOLOR");
  if (!value)
  {
    unsigned char r, g, b;
    iupStrToRGB(IupGetGlobal("DLGBGCOLOR"), &r, &g, &b);
    r = (r * 90) / 100;
    g = (g * 90) / 100;
    b = (b * 90) / 100;
    iupAttribSetStrf(ih, "DARK_DLGBGCOLOR", "%d %d %d", r, g, b);
    return iupAttribGet(ih, "DARK_DLGBGCOLOR");
  }
  else
    return value;
}

IUP_SDK_API int iupFlatSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);
  iupdrvRedrawNow(ih);
  return 0;
}

static void iFlatItemSetTipVisible(Ihandle* ih, const char* tip)
{
  int visible = IupGetInt(ih, "TIPVISIBLE");

  /* do not call IupSetAttribute */
  iupAttribSetStr(ih, "TIP", tip);
  iupdrvBaseSetTipAttrib(ih, tip);

  if (visible)
  {
    IupSetAttribute(ih, "TIPVISIBLE", "No");
    if (tip)
      IupSetAttribute(ih, "TIPVISIBLE", "Yes");
  }
}

static int iFlatItemCheckTip(Ihandle* ih, const char* new_tip)
{
  char* tip = iupAttribGet(ih, "TIP");
  if (!tip && !new_tip)
    return 1;
  if (iupStrEqual(tip, new_tip))
    return 1;
  return 0;
}

IUP_SDK_API void iupFlatItemResetTip(Ihandle* ih)
{
  char* tip = iupAttribGet(ih, "_IUP_FLATITEM_TIP");
  if (!iFlatItemCheckTip(ih, tip))
    iFlatItemSetTipVisible(ih, tip);
}

IUP_SDK_API void iupFlatItemSetTip(Ihandle *ih, const char* tip)
{
  if (!iFlatItemCheckTip(ih, tip))
    iFlatItemSetTipVisible(ih, tip);
}

IUP_SDK_API int iupFlatItemSetTipAttrib(Ihandle* ih, const char* value)
{
  iupAttribSetStr(ih, "_IUP_FLATITEM_TIP", value);
  return iupdrvBaseSetTipAttrib(ih, value);
}

