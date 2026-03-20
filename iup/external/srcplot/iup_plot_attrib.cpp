/*
 * IupPlot element attributes
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iupPlot.h"

#include "iup_plot.h"

#include "iup_class.h"
#include "iup_register.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"

#include "iup_plot_ctrl.h"



#ifndef M_E
#define M_E 2.71828182846
#endif


static void iPlotCheckCurrentDataSet(Ihandle* ih)
{
  int count = ih->data->current_plot->mDataSetListCount;
  if (ih->data->current_plot->mCurrentDataSet == count)
    ih->data->current_plot->mCurrentDataSet--;
}

static void iPlotPlotInsert(Ihandle* ih, int p)
{
  for (int i = ih->data->plot_list_count; i > p; i--)
    ih->data->plot_list[i] = ih->data->plot_list[i - 1];

  ih->data->plot_list[p] = new iupPlot(ih, ih->data->default_font_style, ih->data->default_font_size);

  ih->data->plot_list_count++;

  if (ih->handle)
    iupPlotUpdateViewports(ih);
}

static void iPlotPlotRemove(Ihandle* ih, int p)
{
  if (ih->data->current_plot_index == ih->data->plot_list_count - 1)
    ih->data->current_plot_index--;

  delete ih->data->plot_list[p];

  for (int i = p; i < ih->data->plot_list_count; i++)
    ih->data->plot_list[i] = ih->data->plot_list[i + 1];

  ih->data->plot_list[ih->data->plot_list_count - 1] = NULL;

  ih->data->plot_list_count--;

  iupPlotSetPlotCurrent(ih, ih->data->current_plot_index);

  if (ih->handle)
    iupPlotUpdateViewports(ih);
}

static char* iupStrReturnColor(long color)
{
  unsigned char r = iupDrawRed(color);
  unsigned char g = iupDrawGreen(color);
  unsigned char b = iupDrawBlue(color);
  unsigned char a = iupDrawAlpha(color);
  return iupStrReturnRGBA(r, g, b, a);
}

int iupStrToColor(const char* str, long *color)
{
  unsigned char r, g, b, a;
  if (iupStrToRGBA(str, &r, &g, &b, &a))
  {
    *color = iupDrawColor(r, g, b, a);
    return 1;
  }
  return 0;
}

static long iPlotGetColor(Ihandle* ih, const char* value, const char* default_attrib)
{
  long color = 0;
  if (iupStrToColor(value, &color))
    return color;
  else
  {
    value = IupGetAttribute(ih, default_attrib);
    iupStrToColor(value, &color);
    return color;
  }
}

static int iPlotGetCDFontStyle(const char* value)
{
  if (!value)
    return -1;
  if (iupStrEqualNoCase(value, "PLAIN") || value[0] == 0)
    return IUP_PLOT_FONT_PLAIN;
  if (iupStrEqualNoCase(value, "BOLD"))
    return IUP_PLOT_FONT_BOLD;
  if (iupStrEqualNoCase(value, "ITALIC"))
    return IUP_PLOT_FONT_ITALIC;
  if (iupStrEqualNoCase(value, "BOLDITALIC") ||
      iupStrEqualNoCase(value, "BOLD ITALIC") ||
      iupStrEqualNoCase(value, "ITALIC BOLD"))
      return IUP_PLOT_FONT_BOLD_ITALIC;
  return -1;
}

static int iPlotGetCDFontSize(const char* value)
{
  if (!value)
    return 0;

  int ii;
  if (iupStrToInt(value, &ii))
    return ii;

  return 0;
}

static char* iPlotGetPlotFontSize(Ihandle* ih, int size)
{
  if (size)
    return iupStrReturnInt(size);
  else
    return IupGetAttribute(ih, "FONTSIZE");
}

static char* iPlotGetPlotFontStyle(Ihandle* ih, int style)
{
  if (style >= IUP_PLOT_FONT_PLAIN && style <= IUP_PLOT_FONT_BOLD_ITALIC)
  {
    const char* style_str[4] = { "Plain", "Bold", "Italic", "Bold Italic" };
    return (char*)style_str[style];
  }
  else
    return IupGetAttribute(ih, "FONTSTYLE");
}

static char* iPlotGetPlotPenStyle(int style)
{
  if (style >= IUP_PLOT_LINE_CONTINUOUS && style <= IUP_PLOT_LINE_DASH_DOT_DOT)
  {
    const char* style_str[5] = { "CONTINUOUS", "DASHED", "DOTTED", "DASH_DOT", "DASH_DOT_DOT" };
    return (char*)style_str[style];
  }
  else
    return (char*)"CONTINUOUS";
}

static int iPlotGetCDPenStyle(const char* value)
{
  if (!value || iupStrEqualNoCase(value, "CONTINUOUS"))
    return IUP_PLOT_LINE_CONTINUOUS;
  else if (iupStrEqualNoCase(value, "DASHED"))
    return IUP_PLOT_LINE_DASHED;
  else if (iupStrEqualNoCase(value, "DOTTED"))
    return IUP_PLOT_LINE_DOTTED;
  else if (iupStrEqualNoCase(value, "DASH_DOT"))
    return IUP_PLOT_LINE_DASH_DOT;
  else if (iupStrEqualNoCase(value, "DASH_DOT_DOT"))
    return IUP_PLOT_LINE_DASH_DOT_DOT;
  else
    return IUP_PLOT_LINE_CONTINUOUS;
}

static char* iPlotGetPlotMarkStyle(int style)
{
  if (style >= IUP_PLOT_MARK_PLUS && style <= IUP_PLOT_MARK_HOLLOW_DIAMOND)
  {
    const char* style_str[9] = { "PLUS", "STAR", "CIRCLE", "X", "BOX", "DIAMOND", "HOLLOW_CIRCLE", "HOLLOW_BOX", "HOLLOW_DIAMOND" };
    return (char*)style_str[style];
  }
  else
    return (char*)"X";
}

static int iPlotGetCDMarkStyle(const char* value)
{
  if (!value || iupStrEqualNoCase(value, "PLUS"))
    return IUP_PLOT_MARK_PLUS;
  else if (iupStrEqualNoCase(value, "STAR"))
    return IUP_PLOT_MARK_STAR;
  else if (iupStrEqualNoCase(value, "CIRCLE"))
    return IUP_PLOT_MARK_CIRCLE;
  else if (iupStrEqualNoCase(value, "X"))
    return IUP_PLOT_MARK_X;
  else if (iupStrEqualNoCase(value, "BOX"))
    return IUP_PLOT_MARK_BOX;
  else if (iupStrEqualNoCase(value, "DIAMOND"))
    return IUP_PLOT_MARK_DIAMOND;
  else if (iupStrEqualNoCase(value, "HOLLOW_CIRCLE"))
    return IUP_PLOT_MARK_HOLLOW_CIRCLE;
  else if (iupStrEqualNoCase(value, "HOLLOW_BOX"))
    return IUP_PLOT_MARK_HOLLOW_BOX;
  else if (iupStrEqualNoCase(value, "HOLLOW_DIAMOND"))
    return IUP_PLOT_MARK_HOLLOW_DIAMOND;
  else
    return IUP_PLOT_MARK_PLUS;
}


/**************************************************************************************/


static int iPlotSetShowCrossHairAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "HORIZONTAL"))
    ih->data->show_cross_hair = IUP_PLOT_CROSSHAIR_HORIZ;
  else if (iupStrEqualNoCase(value, "VERTICAL"))
    ih->data->show_cross_hair = IUP_PLOT_CROSSHAIR_VERT;
  else
    ih->data->show_cross_hair = IUP_PLOT_CROSSHAIR_NONE;
  return 0;
}

static char* iPlotGetShowCrossHairAttrib(Ihandle* ih)
{
  const char* cross_hair_str[3] = { "NONE", "VERTICAL", "HORIZONTAL" };
  return (char*)cross_hair_str[ih->data->show_cross_hair];
}


static int iPlotSetAntialiasAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

static char* iPlotGetAntialiasAttrib(Ihandle* ih)
{
  (void)ih;
  return NULL;
}

static int iPlotSetRedrawAttrib(Ihandle* ih, const char* value)
{
  // when REDRAW is set, the default is
  int flush = 1, // always flush
    only_current = 0, // redraw all plots
    reset_redraw = 1;  // always render

  if (iupStrEqualNoCase(value, "NOFLUSH"))
    flush = 0;
  else if (iupStrEqualNoCase(value, "CURRENT"))
  {
    flush = 0;  // same as NOFLUSH
    only_current = 1;
  }

  iupPlotRedraw(ih, flush, only_current, reset_redraw);

  return 0;
}

static char* iPlotGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mDataSetListCount);
}

static int iPlotSetLegendAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->current_plot->mLegend.mShow = true;
  else
    ih->data->current_plot->mLegend.mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mLegend.mShow);
}

static int iPlotSetLegendPosAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "TOPLEFT"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_TOPLEFT;
  else if (iupStrEqualNoCase(value, "BOTTOMLEFT"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_BOTTOMLEFT;
  else if (iupStrEqualNoCase(value, "BOTTOMRIGHT"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_BOTTOMRIGHT;
  else if (iupStrEqualNoCase(value, "BOTTOMCENTER"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_BOTTOMCENTER;
  else if (iupStrEqualNoCase(value, "TOPRIGHT"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_TOPRIGHT;
  else if (iupStrEqualNoCase(value, "XY"))
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_XY;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendPosAttrib(Ihandle* ih)
{
  const char* legendpos_str[] = { "TOPRIGHT", "TOPLEFT", "BOTTOMRIGHT", "BOTTOMLEFT", "BOTTOMCENTER", "XY" };
  return (char*)legendpos_str[ih->data->current_plot->mLegend.mPosition];
}

static int iPlotSetBackImageAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mBack.SetImage(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetBackImageAttrib(Ihandle* ih)
{
  return (char*)(ih->data->current_plot->mBack.GetImage());
}

static int iPlotSetLegendPosXYAttrib(Ihandle* ih, const char* value)
{
  int x, y;
  if (iupStrToIntInt(value, &x, &y, ',') == 2)
  {
    ih->data->current_plot->mLegend.mPosition = IUP_PLOT_XY;
    ih->data->current_plot->mLegend.mPos.mX = x;
    ih->data->current_plot->mLegend.mPos.mY = y;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendPosXYAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->current_plot->mLegend.mPos.mX, ih->data->current_plot->mLegend.mPos.mY, ',');
}

static int iPlotSetBackImageXMinAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    ih->data->current_plot->mBack.mImageMinX = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetBackImageYMinAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    ih->data->current_plot->mBack.mImageMinY = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetBackImageXMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->current_plot->mBack.mImageMinX);
}

static char* iPlotGetBackImageYMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->current_plot->mBack.mImageMinY);
}

static int iPlotSetBackImageXMaxAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    ih->data->current_plot->mBack.mImageMaxX = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetBackImageYMaxAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    ih->data->current_plot->mBack.mImageMaxY = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetBackImageXMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->current_plot->mBack.mImageMaxX);
}

static char* iPlotGetBackImageYMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->current_plot->mBack.mImageMaxY);
}

static int iPlotSetBackColorAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mBack.mColor = iPlotGetColor(ih, value, "BGCOLOR");
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetBackColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mBack.mColor);
}

static int iPlotSetBGColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    /* must manually reset all colors */
    for (int p = 0; p < ih->data->plot_list_count; p++)
    {
      ih->data->plot_list[p]->mLegend.mBoxBackColor = color;
      ih->data->plot_list[p]->mBack.mColor = color;

      ih->data->plot_list[p]->mRedraw = true;
    }
  }
  return 1;
}

static int iPlotSetFGColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    /* must manually reset all colors */
    for (int p = 0; p < ih->data->plot_list_count; p++)
    {
      ih->data->plot_list[p]->mBox.mColor = color;
      ih->data->plot_list[p]->mAxisX.mColor = color;
      ih->data->plot_list[p]->mAxisY.mColor = color;
      ih->data->plot_list[p]->mLegend.mBoxColor = color;
      ih->data->plot_list[p]->mTitle.mColor = color;

      ih->data->plot_list[p]->mRedraw = true;
    }
  }
  return 1;
}

static int iPlotSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];

  if (!iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  int style = IUP_PLOT_FONT_PLAIN;
  if (is_bold) style |= IUP_PLOT_FONT_BOLD;
  if (is_italic) style |= IUP_PLOT_FONT_ITALIC;

  ih->data->default_font_size = size;
  ih->data->default_font_style = style;

  for (int p = 0; p < ih->data->plot_list_count; p++)
  {
    /* store the default attributes in local members to speed up */
    ih->data->plot_list[p]->mDefaultFontSize = size;
    ih->data->plot_list[p]->mDefaultFontStyle = style;
    ih->data->plot_list[p]->mAxisX.mDefaultFontSize = size;
    ih->data->plot_list[p]->mAxisX.mDefaultFontStyle = style;
    ih->data->plot_list[p]->mAxisY.mDefaultFontSize = size;
    ih->data->plot_list[p]->mAxisY.mDefaultFontStyle = style;

    ih->data->plot_list[p]->mRedraw = true;
  }

  return 1;
}

static int iPlotSetTitleColorAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mTitle.mColor = iPlotGetColor(ih, value, "FGCOLOR");
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetTitleColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mTitle.mColor);
}

static int iPlotSetTitleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mTitle.SetText(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetTitleAttrib(Ihandle* ih)
{
  return iupStrReturnStr(ih->data->current_plot->mTitle.GetText());
}

static int iPlotSetTitleFontSizeAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mTitle.mFontSize = iPlotGetCDFontSize(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetTitleFontSizeAttrib(Ihandle* ih)
{
  int theFontSize = ih->data->current_plot->mTitle.mFontSize;
  if (theFontSize == 0)
  {
    int size = IupGetInt(ih, "FONTSIZE");
    if (size > 0) size += 6;
    else size -= 8;

    theFontSize = size;
  }

  return iPlotGetPlotFontSize(ih, theFontSize);
}

static int iPlotSetTitleFontStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mTitle.mFontStyle = iPlotGetCDFontStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetTitleFontStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotFontStyle(ih, ih->data->current_plot->mTitle.mFontStyle);
}

static int iPlotSetTitlePosAutoAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->current_plot->mTitle.mAutoPos = true;
  else
    ih->data->current_plot->mTitle.mAutoPos = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetTitlePosAutoAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mTitle.mAutoPos);
}

static int iPlotSetTitlePosXYAttrib(Ihandle* ih, const char* value)
{
  int x, y;
  if (iupStrToIntInt(value, &x, &y, ',') == 2)
  {
    ih->data->current_plot->mTitle.mAutoPos = false;
    ih->data->current_plot->mTitle.mPosX = x;
    ih->data->current_plot->mTitle.mPosY = y;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetTitlePosXYAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->current_plot->mTitle.mPosX, ih->data->current_plot->mTitle.mPosY, ',');
}

static int iPlotSetLegendFontSizeAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mLegend.mFontSize = iPlotGetCDFontSize(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetLegendFontStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mLegend.mFontStyle = iPlotGetCDFontStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendFontStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotFontStyle(ih, ih->data->current_plot->mLegend.mFontStyle);
}

static char* iPlotGetLegendFontSizeAttrib(Ihandle* ih)
{
  return iPlotGetPlotFontSize(ih, ih->data->current_plot->mLegend.mFontSize);
}

static int iPlotSetMarginLeftAutoAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mBack.mMarginAuto.mLeft = iupStrBoolean(value);
  return 0;
}

static int iPlotSetMarginRightAutoAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mBack.mMarginAuto.mRight = iupStrBoolean(value);
  return 0;
}

static int iPlotSetMarginTopAutoAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mBack.mMarginAuto.mTop = iupStrBoolean(value);
  return 0;
}

static int iPlotSetMarginBottomAutoAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mBack.mMarginAuto.mBottom = iupStrBoolean(value);
  return 0;
}

static char* iPlotGetMarginLeftAutoAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mBack.mMarginAuto.mLeft);
}

static char* iPlotGetMarginRightAutoAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mBack.mMarginAuto.mRight);
}

static char* iPlotGetMarginTopAutoAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mBack.mMarginAuto.mTop);
}

static char* iPlotGetMarginBottomAutoAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mBack.mMarginAuto.mBottom);
}

static int iPlotSetMarginLeftAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
  {
    ih->data->current_plot->mBack.mMarginAuto.mLeft = 1;
    ih->data->current_plot->mRedraw = true;
  }
  else
  {
    int ii;
    if (iupStrToInt(value, &ii))
    {
      ih->data->current_plot->mBack.mMarginAuto.mLeft = 0;
      ih->data->current_plot->mBack.mMargin.mLeft = ii;
      ih->data->current_plot->mRedraw = true;
    }
  }
  return 0;
}

static int iPlotSetMarginRightAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
  {
    ih->data->current_plot->mBack.mMarginAuto.mRight = 1;
    ih->data->current_plot->mRedraw = true;
  }
  else
  {
    int ii;
    if (iupStrToInt(value, &ii))
    {
      ih->data->current_plot->mBack.mMarginAuto.mRight = 0;
      ih->data->current_plot->mBack.mMargin.mRight = ii;
      ih->data->current_plot->mRedraw = true;
    }
  }
  return 0;
}

static int iPlotSetMarginTopAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
  {
    ih->data->current_plot->mBack.mMarginAuto.mTop = 1;
    ih->data->current_plot->mRedraw = true;
  }
  else
  {
    int ii;
    if (iupStrToInt(value, &ii))
    {
      ih->data->current_plot->mBack.mMarginAuto.mTop = 0;
      ih->data->current_plot->mBack.mMargin.mTop = ii;
      ih->data->current_plot->mRedraw = true;
    }
  }
  return 0;
}

static int iPlotSetMarginBottomAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
  {
    ih->data->current_plot->mBack.mMarginAuto.mBottom = 1;
    ih->data->current_plot->mRedraw = true;
  }
  else
  {
    int ii;
    if (iupStrToInt(value, &ii))
    {
      ih->data->current_plot->mBack.mMarginAuto.mBottom = 0;
      ih->data->current_plot->mBack.mMargin.mBottom = ii;
      ih->data->current_plot->mRedraw = true;
    }
  }
  return 0;
}

static char* iPlotGetMarginLeftAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mBack.mMargin.mLeft);
}

static char* iPlotGetMarginRightAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mBack.mMargin.mRight);
}

static char* iPlotGetMarginTopAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mBack.mMargin.mTop);
}

static char* iPlotGetMarginBottomAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mBack.mMargin.mBottom);
}

static int iPlotSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->current_plot->mBack.mHorizPadding, &ih->data->current_plot->mBack.mVertPadding, 'x');
  if (ih->handle)
    iupdrvRedrawNow(ih);
  return 0;
}

static char* iPlotGetPaddingAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->current_plot->mBack.mHorizPadding, ih->data->current_plot->mBack.mVertPadding, 'x');
}

static int iPlotSetGridAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "VERTICAL"))  /* vertical grid - X axis  */
  {
    ih->data->current_plot->mGrid.mShowX = true;
    ih->data->current_plot->mGrid.mShowY = false;
  }
  else if (iupStrEqualNoCase(value, "HORIZONTAL")) /* horizontal grid - Y axis */
  {
    ih->data->current_plot->mGrid.mShowY = true;
    ih->data->current_plot->mGrid.mShowX = false;
  }
  else if (iupStrEqualNoCase(value, "YES"))
  {
    ih->data->current_plot->mGrid.mShowX = true;
    ih->data->current_plot->mGrid.mShowY = true;
  }
  else
  {
    ih->data->current_plot->mGrid.mShowY = false;
    ih->data->current_plot->mGrid.mShowX = false;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetGridAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mGrid.mShowX && ih->data->current_plot->mGrid.mShowY)
    return (char*)"YES";
  else if (ih->data->current_plot->mGrid.mShowY)
    return (char*)"HORIZONTAL";
  else if (ih->data->current_plot->mGrid.mShowX)
    return (char*)"VERTICAL";
  else
    return (char*)"NO";
}

static int iPlotSetGridColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    ih->data->current_plot->mGrid.mColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetGridColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mGrid.mColor);
}

static int iPlotSetGridLineStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mGrid.mLineStyle = iPlotGetCDPenStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetGridLineStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotPenStyle(ih->data->current_plot->mGrid.mLineStyle);
}

static int iPlotSetGridLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mGrid.mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetGridLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  return iupStrReturnInt(ih->data->current_plot->mGrid.mLineWidth);
}

static int iPlotSetGridMinorAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "VERTICAL"))  /* vertical grid - X axis  */
  {
    ih->data->current_plot->mGridMinor.mShowX = true;
    ih->data->current_plot->mGridMinor.mShowY = false;
  }
  else if (iupStrEqualNoCase(value, "HORIZONTAL")) /* horizontal grid - Y axis */
  {
    ih->data->current_plot->mGridMinor.mShowY = true;
    ih->data->current_plot->mGridMinor.mShowX = false;
  }
  else if (iupStrEqualNoCase(value, "YES"))
  {
    ih->data->current_plot->mGridMinor.mShowX = true;
    ih->data->current_plot->mGridMinor.mShowY = true;
  }
  else
  {
    ih->data->current_plot->mGridMinor.mShowY = false;
    ih->data->current_plot->mGridMinor.mShowX = false;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetGridMinorAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mGridMinor.mShowX && ih->data->current_plot->mGridMinor.mShowY)
    return (char*)"YES";
  else if (ih->data->current_plot->mGridMinor.mShowY)
    return (char*)"HORIZONTAL";
  else if (ih->data->current_plot->mGridMinor.mShowX)
    return (char*)"VERTICAL";
  else
    return (char*)"NO";
}

static int iPlotSetGridMinorColorAttrib(Ihandle* ih, const char* value)
{
  long color;
  if (iupStrToColor(value, &color))
  {
    ih->data->current_plot->mGridMinor.mColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetGridMinorColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mGridMinor.mColor);
}

static int iPlotSetGridMinorLineStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mGridMinor.mLineStyle = iPlotGetCDPenStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetGridMinorLineStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotPenStyle(ih->data->current_plot->mGridMinor.mLineStyle);
}

static int iPlotSetGridMinorLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mGridMinor.mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetGridMinorLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  return iupStrReturnInt(ih->data->current_plot->mGridMinor.mLineWidth);
}

static int iPlotSetLegendBoxLineStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mLegend.mBoxLineStyle = iPlotGetCDPenStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendBoxLineStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotPenStyle(ih->data->current_plot->mLegend.mBoxLineStyle);
}

static int iPlotSetBoxLineStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mBox.mLineStyle = iPlotGetCDPenStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetBoxColorAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mBox.mColor = iPlotGetColor(ih, value, "FGCOLOR");
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetBoxColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mBox.mColor);
}

static int iPlotSetBoxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->current_plot->mBox.mShow = true;
  else
    ih->data->current_plot->mBox.mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetBoxAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mBox.mShow);
}

static int iPlotSetLegendBoxAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->current_plot->mLegend.mBoxShow = true;
  else
    ih->data->current_plot->mLegend.mBoxShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendBoxAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mLegend.mBoxShow);
}

static int iPlotSetLegendBoxColorAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mLegend.mBoxColor = iPlotGetColor(ih, value, "FGCOLOR");
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendBoxBackColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mLegend.mBoxBackColor);
}

static int iPlotSetLegendBoxBackColorAttrib(Ihandle* ih, const char* value)
{
  ih->data->current_plot->mLegend.mBoxBackColor = iPlotGetColor(ih, value, "BGCOLOR");
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetLegendBoxColorAttrib(Ihandle* ih)
{
  return iupStrReturnColor(ih->data->current_plot->mLegend.mBoxColor);
}

static int iPlotSetLegendBoxLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mLegend.mBoxLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetLegendBoxLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  return iupStrReturnInt(ih->data->current_plot->mLegend.mBoxLineWidth);
}

static char* iPlotGetBoxLineStyleAttrib(Ihandle* ih)
{
  return iPlotGetPlotPenStyle(ih->data->current_plot->mBox.mLineStyle);
}

static int iPlotSetBoxLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mBox.mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetBoxLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  return iupStrReturnInt(ih->data->current_plot->mBox.mLineWidth);
}

static int iPlotSetCurrentAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    int imax = ih->data->current_plot->mDataSetListCount;
    ih->data->current_plot->mCurrentDataSet = ((ii >= 0) && (ii < imax) ? ii : -1);
    ih->data->current_plot->mRedraw = true;
  }
  else
  {
    ii = ih->data->current_plot->FindDataSet(value);
    if (ii != -1)
    {
      int imax = ih->data->current_plot->mDataSetListCount;
      ih->data->current_plot->mCurrentDataSet = ((ii >= 0) && (ii < imax) ? ii : -1);
      ih->data->current_plot->mRedraw = true;
    }
  }
  return 0;
}

static char* iPlotGetCurrentAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot->mCurrentDataSet);
}

static int iPlotSetPlotCurrentAttrib(Ihandle* ih, const char* value)
{
  int i;
  if (iupStrToInt(value, &i))
  {
    if (i >= 0 && i < ih->data->plot_list_count)
      iupPlotSetPlotCurrent(ih, i);
  }
  else
  {
    for (i = 0; i < ih->data->plot_list_count; i++)
    {
      const char* title = ih->data->plot_list[i]->mTitle.GetText();
      if (iupStrEqual(title, value))
      {
        iupPlotSetPlotCurrent(ih, i);
        return 0;
      }
    }
  }
  return 0;
}

static char* iPlotGetPlotCurrentAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->current_plot_index);
}

static char* iPlotGetPlotCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->plot_list_count);
}

static int iPlotSetPlotCountAttrib(Ihandle* ih, const char* value)
{
  int count;
  if (iupStrToInt(value, &count))
  {
    if (count > 0 && count < IUP_PLOT_MAX_PLOTS)
    {
      if (count != ih->data->plot_list_count)
      {
        if (count < ih->data->plot_list_count)
        {
          // Remove at the end
          if (ih->data->current_plot_index >= count)
            iupPlotSetPlotCurrent(ih, count - 1);

          for (int i = count; i < ih->data->plot_list_count; i++)
          {
            delete ih->data->plot_list[i];
            ih->data->plot_list[i] = NULL;
          }
        }
        else
        {
          // Add at the end
          for (int i = ih->data->plot_list_count; i < count; i++)
            ih->data->plot_list[i] = new iupPlot(ih, ih->data->default_font_style, ih->data->default_font_size);
        }
      }

      ih->data->plot_list_count = count;

      if (ih->handle)
        iupPlotUpdateViewports(ih);
    }
  }
  return 0;
}

static int iPlotSetPlotInsertAttrib(Ihandle* ih, const char* value)
{
  if (!value)
  {
    if (ih->data->plot_list_count < IUP_PLOT_MAX_PLOTS)
    {
      // Insert at the end (append)
      iPlotPlotInsert(ih, ih->data->plot_list_count);
      iupPlotSetPlotCurrent(ih, ih->data->plot_list_count - 1);
    }
  }
  else
  {
    // Insert before reference
    int i;
    if (iupStrToInt(value, &i))
    {
      if (i >= 0 && i < ih->data->plot_list_count)
      {
        iPlotPlotInsert(ih, i);
        iupPlotSetPlotCurrent(ih, ih->data->plot_list_count - 1);
      }
    }
    else
    {
      for (i = 0; i < ih->data->plot_list_count; i++)
      {
        const char* title = ih->data->plot_list[i]->mTitle.GetText();
        if (iupStrEqual(title, value))
        {
          iPlotPlotInsert(ih, i);
          iupPlotSetPlotCurrent(ih, ih->data->plot_list_count - 1);
          return 0;
        }
      }
    }
  }
  return 0;
}

static char* iPlotGetPlotNumColAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->numcol);
}

static int iPlotSetPlotNumColAttrib(Ihandle* ih, const char* value)
{
  int numcol;
  if (iupStrToInt(value, &numcol))
  {
    if (numcol > 0)
    {
      ih->data->numcol = numcol;

      if (ih->handle)
        iupPlotUpdateViewports(ih);
    }
  }
  return 0;
}

static int iPlotSetPlotRemoveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->plot_list_count == 1)
    return 0;

  if (!value || iupStrEqualNoCase(value, "CURRENT"))
  {
    iPlotPlotRemove(ih, ih->data->current_plot_index);
    return 0;
  }

  int i;
  if (iupStrToInt(value, &i))
  {
    if (i >= 0 && i < ih->data->plot_list_count)
      iPlotPlotRemove(ih, i);
  }
  else
  {
    for (i = 0; i < ih->data->plot_list_count; i++)
    {
      const char* title = ih->data->plot_list[i]->mTitle.GetText();
      if (iupStrEqual(title, value))
      {
        iPlotPlotRemove(ih, i);
        return 0;
      }
    }
  }
  return 0;
}

static int iPlotSetSyncViewAttrib(Ihandle* ih, const char* value)
{
  ih->data->sync_view = iupStrBoolean(value);
  return 0;
}

static char* iPlotGetSyncViewAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->sync_view);
}

static int iPlotSetMergeViewAttrib(Ihandle* ih, const char* value)
{
  ih->data->merge_view = iupStrBoolean(value);

  if (ih->handle)
    iupPlotUpdateViewports(ih);

  return 0;
}

static char* iPlotGetMergeViewAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->merge_view);
}

static int iPlotSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  ih->data->read_only = iupStrBoolean(value);
  return 0;
}

static char* iPlotGetReadOnlyAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->read_only);
}

static int iPlotSetGraphicsModeAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

static char* iPlotGetGraphicsModeAttrib(Ihandle* ih)
{
  (void)ih;
  return (char*)"NATIVE";
}

static int iPlotSetDataSetClippingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AREAOFFSET"))
    ih->data->current_plot->mDataSetClipping = IUP_PLOT_CLIPAREAOFFSET;
  else if (iupStrEqualNoCase(value, "NONE"))
    ih->data->current_plot->mDataSetClipping = IUP_PLOT_CLIPNONE;
  else
    ih->data->current_plot->mDataSetClipping = IUP_PLOT_CLIPAREA;

  return 0;
}

static char* iPlotGetDataSetClippingAttrib(Ihandle* ih)
{
  const char* dataset_clipping_str[] = { "NONE", "AREA", "AREAOFFSET" };
  return (char*)dataset_clipping_str[ih->data->current_plot->mDataSetClipping];
}

static int iPlotSetUseImageRGBAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    return iPlotSetGraphicsModeAttrib(ih, "IMAGERGB");
  return 0;
}

static int iPlotSetUseContextPlusAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    return iPlotSetGraphicsModeAttrib(ih, "NATIVEPLUS");
  return 0;
}

static int iPlotSetShowMenuContextAttrib(Ihandle* ih, const char* value)
{
  int screen_x = 0, screen_y = 0;
  if (iupStrToIntInt(value, &screen_x, &screen_y, ',') == 2)
  {
    int sx, sy;
    IupGetIntInt(ih, "SCREENPOSITION", &sx, &sy);

    int x = screen_x - sx;
    int y = screen_y - sy;

    x -= ih->data->current_plot->mViewport.mX;
    y = ih->data->current_plot->mViewport.mHeight - 1 - (y - ih->data->current_plot->mViewport.mY);

    iupPlotShowMenuContext(ih, screen_x, screen_y, x, y);
  }
  return 0;
}

static int iPlotSetZoomAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "+"))
    iupPlotSetZoom(ih, 1);
  else if (iupStrEqualNoCase(value, "-"))
    iupPlotSetZoom(ih, -1);
  else
    iupPlotSetZoom(ih, 0);
  return 0;
}

static char* iPlotGetCanvasAttrib(Ihandle* ih)
{
  (void)ih;
  return NULL;
}

static int iPlotSetRemoveAttrib(Ihandle* ih, const char* value)
{
  if (!value || iupStrEqualNoCase(value, "CURRENT"))
  {
    ih->data->current_plot->RemoveDataSet(ih->data->current_plot->mCurrentDataSet);
    ih->data->current_plot->mRedraw = true;
    iPlotCheckCurrentDataSet(ih);
    return 0;
  }

  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->RemoveDataSet(ii);
    ih->data->current_plot->mRedraw = true;
    iPlotCheckCurrentDataSet(ih);
  }
  else
  {
    ii = ih->data->current_plot->FindDataSet(value);
    if (ii != -1)
    {
      ih->data->current_plot->RemoveDataSet(ii);
      ih->data->current_plot->mRedraw = true;
      iPlotCheckCurrentDataSet(ih);
    }
  }
  return 0;
}

static int iPlotSetClearAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  ih->data->current_plot->RemoveAllDataSets();
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetDSLineStyleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mLineStyle = iPlotGetCDPenStyle(value);

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSLineStyleAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];

  return iPlotGetPlotPenStyle(dataset->mLineStyle);
}

static int iPlotSetDSLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSAreaTransparencyAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnInt(dataset->mAreaTransparency);
}

static int iPlotSetDSAreaTransparencyAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mAreaTransparency = (unsigned char)ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnInt(dataset->mLineWidth);
}

static int iPlotSetDSMarkStyleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mMarkStyle = iPlotGetCDMarkStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSMarkStyleAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];

  return iPlotGetPlotMarkStyle(dataset->mMarkStyle);
}

static int iPlotSetDSMarkSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mMarkSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSMarkSizeAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnInt(dataset->mMarkSize);
}

static int iPlotSetDSNameAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (!value)  // can not be NULL
    return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->SetName(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSNameAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnStr(dataset->GetName());
}

static int iPlotSetDSUserDataAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mUserData = (void*)value;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSUserDataAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return (char*)dataset->mUserData;
}

static int iPlotSetDSColorAttrib(Ihandle* ih, const char* value)
{
  long color;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToColor(value, &color))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSColorAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnColor(dataset->mColor);
}

static void iPlotSetPieDefaults(Ihandle* ih)
{
  IupSetAttribute(ih, "AXS_XAUTOMIN", "NO");
  IupSetAttribute(ih, "AXS_XAUTOMAX", "NO");
  IupSetAttribute(ih, "AXS_YAUTOMIN", "NO");
  IupSetAttribute(ih, "AXS_YAUTOMAX", "NO");
  IupSetAttribute(ih, "AXS_XMIN", "-1");
  IupSetAttribute(ih, "AXS_XMAX", "1");
  IupSetAttribute(ih, "AXS_YMIN", "-1");
  IupSetAttribute(ih, "AXS_YMAX", "1");
  IupSetAttribute(ih, "AXS_X", "NO");
  IupSetAttribute(ih, "AXS_Y", "NO");
  iupAttribSet(ih, "_IUP_PIE_DEFAULTS", "1");
}

static void iPlotResetPieDefaults(Ihandle* ih)
{
  IupSetAttribute(ih, "AXS_XAUTOMIN", NULL);
  IupSetAttribute(ih, "AXS_XAUTOMAX", NULL);
  IupSetAttribute(ih, "AXS_YAUTOMIN", NULL);
  IupSetAttribute(ih, "AXS_YAUTOMAX", NULL);
  IupSetAttribute(ih, "AXS_XMIN", NULL);
  IupSetAttribute(ih, "AXS_XMAX", NULL);
  IupSetAttribute(ih, "AXS_YMIN", NULL);
  IupSetAttribute(ih, "AXS_YMAX", NULL);
  IupSetAttribute(ih, "AXS_X", NULL);
  IupSetAttribute(ih, "AXS_Y", NULL);
  iupAttribSet(ih, "_IUP_PIE_DEFAULTS", NULL);
}

static int iPlotSetDSModeAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];

  if (iupStrEqualNoCase(value, "BAR"))
  {
    dataset->mMode = IUP_PLOT_BAR;
    ih->data->current_plot->mAxisX.mDiscrete = true;
  }
  else if (iupStrEqualNoCase(value, "HORIZONTALBAR"))
  {
    dataset->mMode = IUP_PLOT_HORIZONTALBAR;
    ih->data->current_plot->mAxisY.mDiscrete = true;
  }
  else if (iupStrEqualNoCase(value, "MULTIBAR"))
  {
    dataset->mMode = IUP_PLOT_MULTIBAR;
    ih->data->current_plot->mAxisX.mDiscrete = true;
    ih->data->current_plot->UpdateMultibarCount();
  }
  else if (iupStrEqualNoCase(value, "AREA"))
    dataset->mMode = IUP_PLOT_AREA;
  else if (iupStrEqualNoCase(value, "MARK"))
    dataset->mMode = IUP_PLOT_MARK;
  else if (iupStrEqualNoCase(value, "STEM"))
    dataset->mMode = IUP_PLOT_STEM;
  else if (iupStrEqualNoCase(value, "MARKSTEM"))
    dataset->mMode = IUP_PLOT_MARKSTEM;
  else if (iupStrEqualNoCase(value, "MARKLINE"))
    dataset->mMode = IUP_PLOT_MARKLINE;
  else if (iupStrEqualNoCase(value, "STEP"))
    dataset->mMode = IUP_PLOT_STEP;
  else if (iupStrEqualNoCase(value, "ERRORBAR"))
    dataset->mMode = IUP_PLOT_ERRORBAR;
  else if (iupStrEqualNoCase(value, "PIE"))
    dataset->mMode = IUP_PLOT_PIE;
  else  /* LINE */
    dataset->mMode = IUP_PLOT_LINE;

  if (dataset->mMode == IUP_PLOT_PIE && !iupAttribGet(ih, "_IUP_PIE_DEFAULTS"))
    iPlotSetPieDefaults(ih);
  if (dataset->mMode != IUP_PLOT_PIE && iupAttribGet(ih, "_IUP_PIE_DEFAULTS"))
    iPlotResetPieDefaults(ih);

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSModeAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  const char* mode_str[] = { "LINE", "MARK", "MARKLINE", "AREA", "BAR", "STEM", "MARKSTEM", "HORIZONTALBAR", "MULTIBAR", "STEP", "ERRORBAR", "PIE" };

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return (char*)mode_str[dataset->mMode];
}

static int iPlotSetDSRemoveAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->RemoveSample(ii);

    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSCountAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnInt(dataset->GetCount());
}

static int iPlotSetDSBarOutlineColorAttrib(Ihandle* ih, const char* value)
{
  long color;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToColor(value, &color))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mBarOutlineColor = color;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSBarOutlineColorAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnColor(dataset->mBarOutlineColor);
}

static int iPlotSetDSBarOutlineAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mBarShowOutline = iupStrBoolean(value) ? true : false;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSBarOutlineAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnBoolean(dataset->mBarShowOutline ? 1 : 0);
}

static int iPlotSetDSBarMulticolorAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mBarMulticolor = iupStrBoolean(value) ? true : false;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSBarMulticolorAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnBoolean(dataset->mBarMulticolor ? 1 : 0);
}

static int iPlotSetDSBarSpacingAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToInt(value, &ii))
  {
    if (ii >= 0 && ii <= 100)
    {
      iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
      dataset->mBarSpacingPercent = ii;
      ih->data->current_plot->mRedraw = true;
    }
  }
  return 0;
}

static char* iPlotGetDSBarSpacingAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnInt(dataset->mBarSpacingPercent);
}

static int iPlotSetDSPieRadiusAttrib(Ihandle* ih, const char* value)
{
  double ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToDouble(value, &ii))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mPieRadius = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSPieRadiusAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnDouble(dataset->mPieRadius);
}

static int iPlotSetDSPieStartAngleAttrib(Ihandle* ih, const char* value)
{
  double ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToDouble(value, &ii))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mPieStartAngle = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSPieStartAngleAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnDouble(dataset->mPieStartAngle);
}

static int iPlotSetDSPieHoleAttrib(Ihandle* ih, const char* value)
{
  double ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToDouble(value, &ii))
  {
    if (ii < 0) ii = 0;
    if (ii > 1) ii = 1;
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mPieHole = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetDSPieHoleAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnDouble(dataset->mPieHole);
}

static int iPlotSetDSPieContourAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mPieContour = iupStrBoolean(value) ? true : false;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSPieContourAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnBoolean(dataset->mPieContour ? 1 : 0);
}

static int iPlotSetDSPieSliceLabelAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];

  if (iupStrEqualNoCase(value, "NONE"))
    dataset->mPieSliceLabel = IUP_PLOT_NONE;
  else if (iupStrEqualNoCase(value, "X"))
    dataset->mPieSliceLabel = IUP_PLOT_X;
  else if (iupStrEqualNoCase(value, "Y"))
    dataset->mPieSliceLabel = IUP_PLOT_Y;
  else if (iupStrEqualNoCase(value, "PERCENT"))
    dataset->mPieSliceLabel = IUP_PLOT_PERCENT;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSPieSliceLabelAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  const char* mode_str[] = { "NONE", "X", "Y", "PERCENT" };

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return (char*)mode_str[dataset->mPieSliceLabel];
}

static char* iPlotGetDSPieSliceLabelPosAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnDouble(dataset->mPieSliceLabelPos);
}

static char* iPlotGetDSStrXDataAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnBoolean(dataset->GetDataX()->IsString() ? 1 : 0);
}

static char* iPlotGetDSExtraAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnBoolean(dataset->GetExtra() ? 1 : 0);
}

static int iPlotSetDSOrderedXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mOrderedX = iupStrBoolean(value) ? true : false;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSOrderedXAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnBoolean(dataset->mOrderedX ? 1 : 0);
}

static int iPlotSetDSSelectedAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return 0;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  dataset->mSelectedCurve = iupStrBoolean(value) ? true : false;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetDSSelectedAttrib(Ihandle* ih)
{
  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
    return NULL;

  iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
  return iupStrReturnBoolean(dataset->mSelectedCurve ? 1 : 0);
}

static int iPlotSetDSPieSliceLabelPosAttrib(Ihandle* ih, const char* value)
{
  double ii;

  if (ih->data->current_plot->mCurrentDataSet < 0 ||
      ih->data->current_plot->mCurrentDataSet >= ih->data->current_plot->mDataSetListCount)
      return 0;

  if (iupStrToDouble(value, &ii))
  {
    iupPlotDataSet* dataset = ih->data->current_plot->mDataSetList[ih->data->current_plot->mCurrentDataSet];
    dataset->mPieSliceLabelPos = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetHighlightModeAttrib(Ihandle* ih)
{
  const char* mode_str[] = { "NONE", "SAMPLE", "CURVE", "BOTH" };
  return (char*)mode_str[ih->data->current_plot->mHighlightMode];
}

static int iPlotSetHighlightModeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "SAMPLE"))
    ih->data->current_plot->mHighlightMode = IUP_PLOT_HIGHLIGHT_SAMPLE;
  else if (iupStrEqualNoCase(value, "CURVE"))
    ih->data->current_plot->mHighlightMode = IUP_PLOT_HIGHLIGHT_CURVE;
  else if (iupStrEqualNoCase(value, "BOTH"))
    ih->data->current_plot->mHighlightMode = IUP_PLOT_HIGHLIGHT_BOTH;
  else  /* NONE */
    ih->data->current_plot->mHighlightMode = IUP_PLOT_HIGHLIGHT_NONE;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetScreenToleranceAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->current_plot->mScreenTolerance);
}

static int iPlotSetScreenToleranceAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    ih->data->current_plot->mScreenTolerance = xx;
  }

  return 0;
}

/* ========== */
/* axis props */
/* ========== */

static int iPlotSetAxisXDiscreteAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  axis->mDiscrete = iupStrBoolean(value) ? true : false;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXDiscreteAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnBoolean(axis->mDiscrete ? 1 : 0);
}

static int iPlotSetAxisYDiscreteAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  axis->mDiscrete = iupStrBoolean(value) ? true : false;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisYDiscreteAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnBoolean(axis->mDiscrete ? 1 : 0);
}

static int iPlotSetAxisXLabelAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  axis->SetLabel(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYLabelAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  axis->SetLabel(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXLabelAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnStr(axis->GetLabel());
}

static char* iPlotGetAxisYLabelAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnStr(axis->GetLabel());
}

static int iPlotSetAxisXAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mShow = true;
  else
    axis->mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mShow = true;
  else
    axis->mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mShow);
}

static char* iPlotGetAxisYAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mShow);
}

static int iPlotSetAxisXLabelCenteredAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mLabelCentered = true;
  else
    axis->mLabelCentered = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYLabelCenteredAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mLabelCentered = true;
  else
    axis->mLabelCentered = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXLabelCenteredAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mLabelCentered);
}

static char* iPlotGetAxisYLabelCenteredAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mLabelCentered);
}

static int iPlotSetAxisXColorAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  axis->mColor = iPlotGetColor(ih, value, "FGCOLOR");
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYColorAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  axis->mColor = iPlotGetColor(ih, value, "FGCOLOR");
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXColorAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnColor(axis->mColor);
}

static char* iPlotGetAxisYColorAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnColor(axis->mColor);
}

static int iPlotSetAxisXLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mAxisX.mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXLineWidthAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnInt(axis->mLineWidth);
}

static int iPlotSetAxisYLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->current_plot->mAxisY.mLineWidth = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisYLineWidthAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnInt(axis->mLineWidth);
}

static int iPlotSetAxisXLabelSpacingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
  {
    ih->data->current_plot->mAxisX.mLabelSpacing = -1;
    ih->data->current_plot->mRedraw = true;
  }
  else
  {
    int ii;
    if (iupStrToInt(value, &ii))
    {
      ih->data->current_plot->mAxisX.mLabelSpacing = ii;
      ih->data->current_plot->mRedraw = true;
    }
  }
  return 0;
}

static char* iPlotGetAxisXLabelSpacingAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  if (axis->mLabelSpacing == -1)
    return (char*)"AUTO";
  else
    return iupStrReturnInt(axis->mLabelSpacing);
}

static int iPlotSetAxisYLabelSpacingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AUTO"))
  {
    ih->data->current_plot->mAxisY.mLabelSpacing = -1;
    ih->data->current_plot->mRedraw = true;
  }
  else
  {
    int ii;
    if (iupStrToInt(value, &ii))
    {
      ih->data->current_plot->mAxisY.mLabelSpacing = ii;
      ih->data->current_plot->mRedraw = true;
    }
  }
  return 0;
}

static char* iPlotGetAxisYLabelSpacingAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  if (axis->mLabelSpacing == -1)
    return (char*)"AUTO";
  else
    return iupStrReturnInt(axis->mLabelSpacing);
}

static int iPlotSetViewportSquareAttrib(Ihandle* ih, const char* value)
{
  iupPlotResetZoom(ih, 0);

  ih->data->current_plot->mViewportSquare = iupStrBoolean(value) ? true : false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetViewportSquareAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mViewportSquare);
}

static int iPlotSetAxisScaleEqualAttrib(Ihandle* ih, const char* value)
{
  iupPlotResetZoom(ih, 0);

  ih->data->current_plot->mScaleEqual = iupStrBoolean(value) ? true : false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisScaleEqualAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->current_plot->mScaleEqual);
}

static int iPlotSetAxisXAutoMinAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  iupPlotResetZoom(ih, 0);

  if (iupStrBoolean(value))
    axis->mAutoScaleMin = true;
  else
    axis->mAutoScaleMin = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYAutoMinAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  iupPlotResetZoom(ih, 0);

  if (iupStrBoolean(value))
    axis->mAutoScaleMin = true;
  else
    axis->mAutoScaleMin = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXAutoMinAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mAutoScaleMin);
}

static char* iPlotGetAxisYAutoMinAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mAutoScaleMin);
}

static int iPlotSetAxisXAutoMaxAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  iupPlotResetZoom(ih, 0);

  if (iupStrBoolean(value))
    axis->mAutoScaleMax = true;
  else
    axis->mAutoScaleMax = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYAutoMaxAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  iupPlotResetZoom(ih, 0);

  if (iupStrBoolean(value))
    axis->mAutoScaleMax = true;
  else
    axis->mAutoScaleMax = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXAutoMaxAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mAutoScaleMax);
}

static char* iPlotGetAxisYAutoMaxAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mAutoScaleMax);
}

static int iPlotSetAxisXMinAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotResetZoom(ih, 0);

    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mMin = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYMinAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotResetZoom(ih, 0);

    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mMin = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXMinAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnDouble(axis->mMin);
}

static char* iPlotGetAxisYMinAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnDouble(axis->mMin);
}

static int iPlotSetAxisXMaxAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotResetZoom(ih, 0);

    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mMax = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYMaxAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotResetZoom(ih, 0);

    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mMax = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXMaxAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnDouble(axis->mMax);
}

static char* iPlotGetAxisYMaxAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnDouble(axis->mMax);
}

static int iPlotSetAxisXReverseAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mReverse = true;
  else
    axis->mReverse = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYReverseAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mReverse = true;
  else
    axis->mReverse = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXReverseAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnBoolean(axis->mReverse);
}

static char* iPlotGetAxisYReverseAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnBoolean(axis->mReverse);
}

static int iPlotSetAxisXReverseTicksLabelAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mReverseTicksLabel = true;
  else
    axis->mReverseTicksLabel = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYReverseTicksLabelAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mReverseTicksLabel = true;
  else
    axis->mReverseTicksLabel = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXReverseTicksLabelAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnBoolean(axis->mReverseTicksLabel);
}

static char* iPlotGetAxisYReverseTicksLabelAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnBoolean(axis->mReverseTicksLabel);
}

static int iPlotSetAxisXCrossOriginAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mPosition = IUP_PLOT_CROSSORIGIN;
  else
    axis->mPosition = IUP_PLOT_START;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYCrossOriginAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mPosition = IUP_PLOT_CROSSORIGIN;
  else
    axis->mPosition = IUP_PLOT_START;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXCrossOriginAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mPosition == IUP_PLOT_CROSSORIGIN);
}

static char* iPlotGetAxisYCrossOriginAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mPosition == IUP_PLOT_CROSSORIGIN);
}

static int iPlotSetAxisXPositionAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrEqualNoCase(value, "CROSSORIGIN"))
    axis->mPosition = IUP_PLOT_CROSSORIGIN;
  else if(iupStrEqualNoCase(value, "END"))
    axis->mPosition = IUP_PLOT_END;
  else
    axis->mPosition = IUP_PLOT_START;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYPositionAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrEqualNoCase(value, "CROSSORIGIN"))
    axis->mPosition = IUP_PLOT_CROSSORIGIN;
  else if (iupStrEqualNoCase(value, "END"))
    axis->mPosition = IUP_PLOT_END;
  else
    axis->mPosition = IUP_PLOT_START;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXPositionAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  const char* originpos_str[] = { "START", "CROSSORIGIN", "END" };
  return (char*)originpos_str[axis->mPosition];
}

static char* iPlotGetAxisYPositionAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  const char* originpos_str[] = { "START", "CROSSORIGIN", "END" };
  return (char*)originpos_str[axis->mPosition];
}

static int iPlotSetAxisXScaleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrEqualNoCase(value, "LIN"))
  {
    axis->mLogScale = false;
  }
  else if (iupStrEqualNoCase(value, "LOG10"))
  {
    axis->mLogScale = true;
    axis->mLogBase = 10.0;
  }
  else if (iupStrEqualNoCase(value, "LOG2"))
  {
    axis->mLogScale = true;
    axis->mLogBase = 2.0;
  }
  else
  {
    axis->mLogScale = true;
    axis->mLogBase = (double)M_E;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYScaleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrEqualNoCase(value, "LIN"))
  {
    axis->mLogScale = false;
  }
  else if (iupStrEqualNoCase(value, "LOG10"))
  {
    axis->mLogScale = true;
    axis->mLogBase = 10.0;
  }
  else if (iupStrEqualNoCase(value, "LOG2"))
  {
    axis->mLogScale = true;
    axis->mLogBase = 2.0;
  }
  else
  {
    axis->mLogScale = true;
    axis->mLogBase = (double)M_E;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXScaleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (axis->mLogScale)
  {
    if (axis->mLogBase == 10.0)
      return (char*)"LOG10";
    else if (axis->mLogBase == 2.0)
      return (char*)"LOG2";
    else
      return (char*)"LOGN";
  }
  else
    return (char*)"LIN";
}

static char* iPlotGetAxisYScaleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (axis->mLogScale)
  {
    if (axis->mLogBase == 10.0)
      return (char*)"LOG10";
    else if (axis->mLogBase == 2.0)
      return (char*)"LOG2";
    else
      return (char*)"LOGN";
  }
  else
    return (char*)"LIN";
}

static int iPlotSetAxisXFontSizeAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  axis->mFontSize = iPlotGetCDFontSize(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYFontSizeAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  axis->mFontSize = iPlotGetCDFontSize(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXFontSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iPlotGetPlotFontSize(ih, axis->mFontSize);
}

static char* iPlotGetAxisYFontSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iPlotGetPlotFontSize(ih, axis->mFontSize);
}

static int iPlotSetAxisXFontStyleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  axis->mFontStyle = iPlotGetCDFontStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYFontStyleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  axis->mFontStyle = iPlotGetCDFontStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXFontStyleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iPlotGetPlotFontStyle(ih, axis->mFontStyle);
}

static char* iPlotGetAxisYFontStyleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iPlotGetPlotFontStyle(ih, axis->mFontStyle);
}

static int iPlotSetAxisXArrowAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mShowArrow = true;
  else
    axis->mShowArrow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXArrowAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mShowArrow);
}

static int iPlotSetAxisYArrowAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mShowArrow = true;
  else
    axis->mShowArrow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisYArrowAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mShowArrow);
}

static int iPlotSetAxisXAutoTickSizeAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mTick.mAutoSize = true;
  else
    axis->mTick.mAutoSize = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYAutoTickSizeAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mTick.mAutoSize = true;
  else
    axis->mTick.mAutoSize = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXAutoTickSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mTick.mAutoSize);
}

static char* iPlotGetAxisYAutoTickSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mTick.mAutoSize);
}

static int iPlotSetAxisXTickSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mTick.mMinorSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYTickSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mTick.mMinorSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnInt(axis->mTick.mMinorSize);
}

static char* iPlotGetAxisYTickSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnInt(axis->mTick.mMinorSize);
}

static int iPlotSetAxisXTickMajorSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mTick.mMajorSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYTickMajorSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mTick.mMajorSize = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickMajorSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnInt(axis->mTick.mMajorSize);
}

static char* iPlotGetAxisYTickMajorSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnInt(axis->mTick.mMajorSize);
}

static int iPlotSetAxisXTickFontSizeAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  axis->mTick.mFontSize = iPlotGetCDFontSize(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYTickFontSizeAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  axis->mTick.mFontSize = iPlotGetCDFontSize(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXTickFontSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iPlotGetPlotFontSize(ih, axis->mTick.mFontSize);
}

static char* iPlotGetAxisYTickFontSizeAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iPlotGetPlotFontSize(ih, axis->mTick.mFontSize);
}

static int iPlotSetAxisXTickFontStyleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  axis->mTick.mFontStyle = iPlotGetCDFontStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYTickFontStyleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  axis->mTick.mFontStyle = iPlotGetCDFontStyle(value);
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXTickFontStyleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iPlotGetPlotFontStyle(ih, axis->mTick.mFontStyle);
}

static char* iPlotGetAxisYTickFontStyleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iPlotGetPlotFontStyle(ih, axis->mTick.mFontStyle);
}

static int iPlotSetAxisXTickFormatAutoAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  axis->mTick.mFormatAuto = iupStrBoolean(value) ? true : false;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYTickFormatAutoAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  axis->mTick.mFormatAuto = iupStrBoolean(value) ? true : false;
  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXTickFormatAutoAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnBoolean(axis->mTick.mFormatAuto);
}

static char* iPlotGetAxisYTickFormatAutoAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnBoolean(axis->mTick.mFormatAuto);
}

static int iPlotSetAxisXTickFormatAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (value && value[0] != 0)
  {
    iupStrCopyN(axis->mTick.mFormatString, sizeof(axis->mTick.mFormatString), value);
    axis->mTick.mFormatAuto = false;
  }
  else
  {
    iupStrCopyN(axis->mTick.mFormatString, sizeof(axis->mTick.mFormatString), IUP_PLOT_DEF_NUMBERFORMAT);
    axis->mTick.mFormatAuto = true;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYTickFormatAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (value && value[0] != 0)
  {
    iupStrCopyN(axis->mTick.mFormatString, sizeof(axis->mTick.mFormatString), value);
    axis->mTick.mFormatAuto = false;
  }
  else
  {
    iupStrCopyN(axis->mTick.mFormatString, sizeof(axis->mTick.mFormatString), IUP_PLOT_DEF_NUMBERFORMAT);
    axis->mTick.mFormatAuto = true;
  }

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisXTickFormatPrecisionAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  int precision;
  if (iupStrToInt(value, &precision))
  {
    snprintf(axis->mTick.mFormatString, sizeof(axis->mTick.mFormatString), "%%.%df", precision);
    axis->mTick.mFormatAuto = false;
  }
  else
  {
    iupStrCopyN(axis->mTick.mFormatString, sizeof(axis->mTick.mFormatString), IUP_PLOT_DEF_NUMBERFORMAT);
    axis->mTick.mFormatAuto = true;
  }
  return 0;
}

static int iPlotSetAxisYTickFormatPrecisionAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  int precision;
  if (iupStrToInt(value, &precision))
  {
    snprintf(axis->mTick.mFormatString, sizeof(axis->mTick.mFormatString), "%%.%df", precision);
    axis->mTick.mFormatAuto = false;
  }
  else
  {
    iupStrCopyN(axis->mTick.mFormatString, sizeof(axis->mTick.mFormatString), IUP_PLOT_DEF_NUMBERFORMAT);
    axis->mTick.mFormatAuto = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickFormatAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnStr(axis->mTick.mFormatString);
}

static char* iPlotGetAxisYTickFormatAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnStr(axis->mTick.mFormatString);
}

static char* iPlotGetAxisXTickFormatPrecisionAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  int precision = iupStrGetFormatPrecision(axis->mTick.mFormatString);
  return iupStrReturnInt(precision);
}

static char* iPlotGetAxisYTickFormatPrecisionAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  int precision = iupStrGetFormatPrecision(axis->mTick.mFormatString);
  return iupStrReturnInt(precision);
}

static int iPlotSetAxisXTipFormatAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (value && value[0] != 0)
    iupStrCopyN(axis->mTipFormatString, sizeof(axis->mTipFormatString), value);
  else
    iupStrCopyN(axis->mTipFormatString, sizeof(axis->mTipFormatString), IUP_PLOT_DEF_TIPFORMAT);

  return 0;
}

static int iPlotSetAxisYTipFormatAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (value && value[0] != 0)
    iupStrCopyN(axis->mTipFormatString, sizeof(axis->mTipFormatString), value);
  else
    iupStrCopyN(axis->mTipFormatString, sizeof(axis->mTipFormatString), IUP_PLOT_DEF_TIPFORMAT);

  return 0;
}

static int iPlotSetAxisXTipFormatPrecisionAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  int precision;
  if (iupStrToInt(value, &precision))
    snprintf(axis->mTipFormatString, sizeof(axis->mTipFormatString), "%%.%df", precision);
  else
    iupStrCopyN(axis->mTipFormatString, sizeof(axis->mTipFormatString), IUP_PLOT_DEF_TIPFORMAT);
  return 0;
}

static int iPlotSetAxisYTipFormatPrecisionAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  int precision;
  if (iupStrToInt(value, &precision))
    snprintf(axis->mTipFormatString, sizeof(axis->mTipFormatString), "%%.%df", precision);
  else
    iupStrCopyN(axis->mTipFormatString, sizeof(axis->mTipFormatString), IUP_PLOT_DEF_TIPFORMAT);
  return 0;
}

static char* iPlotGetAxisXTipFormatAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnStr(axis->mTipFormatString);
}

static char* iPlotGetAxisYTipFormatAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnStr(axis->mTipFormatString);
}

static char* iPlotGetAxisXTipFormatPrecisionAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  int precision = iupStrGetFormatPrecision(axis->mTipFormatString);
  return iupStrReturnInt(precision);
}

static char* iPlotGetAxisYTipFormatPrecisionAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  int precision = iupStrGetFormatPrecision(axis->mTipFormatString);
  return iupStrReturnInt(precision);
}

static int iPlotSetAxisXTickAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mTick.mShow = true;
  else
    axis->mTick.mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXTickAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mTick.mShow);
}

static int iPlotSetAxisYTickAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mTick.mShow = true;
  else
    axis->mTick.mShow = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisYTickAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mTick.mShow);
}

static int iPlotSetAxisXTickNumberAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mTick.mShowNumber = true;
  else
    axis->mTick.mShowNumber = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYTickNumberAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mTick.mShowNumber = true;
  else
    axis->mTick.mShowNumber = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXTickNumberAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mTick.mShowNumber);
}

static char* iPlotGetAxisYTickNumberAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mTick.mShowNumber);
}

static int iPlotSetAxisXTickRotateNumberAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mTick.mRotateNumber = true;
  else
    axis->mTick.mRotateNumber = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYTickRotateNumberAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mTick.mRotateNumber = true;
  else
    axis->mTick.mRotateNumber = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXTickRotateNumberAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnBoolean(axis->mTick.mRotateNumber);
}

static char* iPlotGetAxisYTickRotateNumberAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mTick.mRotateNumber);
}

static int iPlotSetAxisXTickRotateNumberAngleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrToDouble(value, &(axis->mTick.mRotateNumberAngle)))
    ih->data->current_plot->mRedraw = true;

  return 0;
}

static int iPlotSetAxisYTickRotateNumberAngleAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrToDouble(value, &(axis->mTick.mRotateNumberAngle)))
    ih->data->current_plot->mRedraw = true;

  return 0;
}

static char* iPlotGetAxisXTickRotateNumberAngleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  return iupStrReturnDouble(axis->mTick.mRotateNumberAngle);
}

static char* iPlotGetAxisYTickRotateNumberAngleAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnDouble(axis->mTick.mRotateNumberAngle);
}

static int iPlotSetAxisXTickMajorSpanAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mTick.mMajorSpan = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYTickMajorSpanAttrib(Ihandle* ih, const char* value)
{
  double xx;
  if (iupStrToDouble(value, &xx))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mTick.mMajorSpan = xx;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickMajorSpanAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnDouble(axis->mTick.mMajorSpan);
}

static char* iPlotGetAxisYTickMajorSpanAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnDouble(axis->mTick.mMajorSpan);
}

static int iPlotSetAxisXTickDivisionAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
    axis->mTick.mMinorDivision = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static int iPlotSetAxisYTickDivisionAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
    axis->mTick.mMinorDivision = ii;
    ih->data->current_plot->mRedraw = true;
  }
  return 0;
}

static char* iPlotGetAxisXTickDivisionAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnInt(axis->mTick.mMinorDivision);
}

static char* iPlotGetAxisYTickDivisionAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;
  return iupStrReturnInt(axis->mTick.mMinorDivision);
}

static int iPlotSetAxisXAutoTickAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;

  if (iupStrBoolean(value))
    axis->mTick.mAutoSpacing = true;
  else
    axis->mTick.mAutoSpacing = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static int iPlotSetAxisYAutoTickAttrib(Ihandle* ih, const char* value)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  if (iupStrBoolean(value))
    axis->mTick.mAutoSpacing = true;
  else
    axis->mTick.mAutoSpacing = false;

  ih->data->current_plot->mRedraw = true;
  return 0;
}

static char* iPlotGetAxisXAutoTickAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisX;
  return iupStrReturnBoolean(axis->mTick.mAutoSpacing);
}

static char* iPlotGetAxisYAutoTickAttrib(Ihandle* ih)
{
  iupPlotAxis* axis = &ih->data->current_plot->mAxisY;

  return iupStrReturnBoolean(axis->mTick.mAutoSpacing);
}

void iupPlotRegisterAttributes(Iclass* ic)
{
  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iPlotSetBGColorAttrib, IUPAF_SAMEASSYSTEM, "255 255 255", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iPlotSetFGColorAttrib, IUPAF_SAMEASSYSTEM, "0 0 0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FONT", NULL, iPlotSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* IupPlot only */

  iupClassRegisterAttribute(ic, "ANTIALIAS", iPlotGetAntialiasAttrib, iPlotSetAntialiasAttrib, IUPAF_SAMEASSYSTEM, "No", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDRAW", NULL, iPlotSetRedrawAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SYNCVIEW", iPlotGetSyncViewAttrib, iPlotSetSyncViewAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MERGEVIEW", iPlotGetMergeViewAttrib, iPlotSetMergeViewAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", iPlotGetReadOnlyAttrib, iPlotSetReadOnlyAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CANVAS", iPlotGetCanvasAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRAPHICSMODE", iPlotGetGraphicsModeAttrib, iPlotSetGraphicsModeAttrib, IUPAF_SAMEASSYSTEM, "NATIVEPLUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "USE_IMAGERGB", NULL, iPlotSetUseImageRGBAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "USE_CONTEXTPLUS", NULL, iPlotSetUseContextPlusAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUCONTEXT", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUITEMPROPERTIES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUITEMVALUES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWMENUCONTEXT", NULL, iPlotSetShowMenuContextAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPFORMAT", NULL, NULL, IUPAF_SAMEASSYSTEM, "%s (%s, %s)", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ZOOM", NULL, iPlotSetZoomAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EDITABLEVALUES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DATASETCLIPPING", iPlotGetDataSetClippingAttrib, iPlotSetDataSetClippingAttrib, IUPAF_SAMEASSYSTEM, "AREA", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCROSSHAIR", iPlotGetShowCrossHairAttrib, iPlotSetShowCrossHairAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MARGINLEFTAUTO", iPlotGetMarginLeftAutoAttrib, iPlotSetMarginLeftAutoAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINRIGHTAUTO", iPlotGetMarginRightAutoAttrib, iPlotSetMarginRightAutoAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINTOPAUTO", iPlotGetMarginTopAutoAttrib, iPlotSetMarginTopAutoAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINBOTTOMAUTO", iPlotGetMarginBottomAutoAttrib, iPlotSetMarginBottomAutoAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINLEFT", iPlotGetMarginLeftAttrib, iPlotSetMarginLeftAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINRIGHT", iPlotGetMarginRightAttrib, iPlotSetMarginRightAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINTOP", iPlotGetMarginTopAttrib, iPlotSetMarginTopAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINBOTTOM", iPlotGetMarginBottomAttrib, iPlotSetMarginBottomAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iPlotGetPaddingAttrib, iPlotSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "5x5", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BACKCOLOR", iPlotGetBackColorAttrib, iPlotSetBackColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGE", iPlotGetBackImageAttrib, iPlotSetBackImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGE_XMIN", iPlotGetBackImageXMinAttrib, iPlotSetBackImageXMinAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGE_YMIN", iPlotGetBackImageYMinAttrib, iPlotSetBackImageYMinAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGE_XMAX", iPlotGetBackImageXMaxAttrib, iPlotSetBackImageXMaxAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGE_YMAX", iPlotGetBackImageYMaxAttrib, iPlotSetBackImageYMaxAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HIGHLIGHTMODE", iPlotGetHighlightModeAttrib, iPlotSetHighlightModeAttrib, IUPAF_SAMEASSYSTEM, "NONE", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCREENTOLERANCE", iPlotGetScreenToleranceAttrib, iPlotSetScreenToleranceAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TITLE", iPlotGetTitleAttrib, iPlotSetTitleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLECOLOR", iPlotGetTitleColorAttrib, iPlotSetTitleColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEFONTSIZE", iPlotGetTitleFontSizeAttrib, iPlotSetTitleFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEFONTSTYLE", iPlotGetTitleFontStyleAttrib, iPlotSetTitleFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEPOSAUTO", iPlotGetTitlePosAutoAttrib, iPlotSetTitlePosAutoAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEPOSXY", iPlotGetTitlePosXYAttrib, iPlotSetTitlePosXYAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LEGEND", iPlotGetLegendAttrib, iPlotSetLegendAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "LEGENDSHOW", iPlotGetLegendAttrib, iPlotSetLegendAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDPOS", iPlotGetLegendPosAttrib, iPlotSetLegendPosAttrib, IUPAF_SAMEASSYSTEM, "TOPRIGHT", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDPOSXY", iPlotGetLegendPosXYAttrib, iPlotSetLegendPosXYAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDFONTSIZE", iPlotGetLegendFontSizeAttrib, iPlotSetLegendFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDFONTSTYLE", iPlotGetLegendFontStyleAttrib, iPlotSetLegendFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDBOX", iPlotGetLegendBoxAttrib, iPlotSetLegendBoxAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDBOXCOLOR", iPlotGetLegendBoxColorAttrib, iPlotSetLegendBoxColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDBOXBACKCOLOR", iPlotGetLegendBoxBackColorAttrib, iPlotSetLegendBoxBackColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDBOXLINESTYLE", iPlotGetLegendBoxLineStyleAttrib, iPlotSetLegendBoxLineStyleAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDBOXLINEWIDTH", iPlotGetLegendBoxLineWidthAttrib, iPlotSetLegendBoxLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "GRID", iPlotGetGridAttrib, iPlotSetGridAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDCOLOR", iPlotGetGridColorAttrib, iPlotSetGridColorAttrib, IUPAF_SAMEASSYSTEM, "200 200 200", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDLINESTYLE", iPlotGetGridLineStyleAttrib, iPlotSetGridLineStyleAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDLINEWIDTH", iPlotGetGridLineWidthAttrib, iPlotSetGridLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDMINOR", iPlotGetGridMinorAttrib, iPlotSetGridMinorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDMINORCOLOR", iPlotGetGridMinorColorAttrib, iPlotSetGridMinorColorAttrib, IUPAF_SAMEASSYSTEM, "200 200 200", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDMINORLINESTYLE", iPlotGetGridMinorLineStyleAttrib, iPlotSetGridMinorLineStyleAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDMINORLINEWIDTH", iPlotGetGridMinorLineWidthAttrib, iPlotSetGridMinorLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BOX", iPlotGetBoxAttrib, iPlotSetBoxAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BOXCOLOR", iPlotGetBoxColorAttrib, iPlotSetBoxColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BOXLINESTYLE", iPlotGetBoxLineStyleAttrib, iPlotSetBoxLineStyleAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BOXLINEWIDTH", iPlotGetBoxLineWidthAttrib, iPlotSetBoxLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DS_LINESTYLE", iPlotGetDSLineStyleAttrib, iPlotSetDSLineStyleAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_LINEWIDTH", iPlotGetDSLineWidthAttrib, iPlotSetDSLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_AREATRANSPARENCY", iPlotGetDSAreaTransparencyAttrib, iPlotSetDSAreaTransparencyAttrib, IUPAF_SAMEASSYSTEM, "255", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_MARKSTYLE", iPlotGetDSMarkStyleAttrib, iPlotSetDSMarkStyleAttrib, IUPAF_SAMEASSYSTEM, "X", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_MARKSIZE", iPlotGetDSMarkSizeAttrib, iPlotSetDSMarkSizeAttrib, IUPAF_SAMEASSYSTEM, "7", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_NAME", iPlotGetDSNameAttrib, iPlotSetDSNameAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "DS_LEGEND", iPlotGetDSNameAttrib, iPlotSetDSNameAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_COLOR", iPlotGetDSColorAttrib, iPlotSetDSColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_MODE", iPlotGetDSModeAttrib, iPlotSetDSModeAttrib, IUPAF_SAMEASSYSTEM, "LINE", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_REMOVE", NULL, iPlotSetDSRemoveAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_COUNT", iPlotGetDSCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_USERDATA", iPlotGetDSUserDataAttrib, iPlotSetDSUserDataAttrib, NULL, NULL, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_BAROUTLINE", iPlotGetDSBarOutlineAttrib, iPlotSetDSBarOutlineAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_BAROUTLINECOLOR", iPlotGetDSBarOutlineColorAttrib, iPlotSetDSBarOutlineColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_BARMULTICOLOR", iPlotGetDSBarMulticolorAttrib, iPlotSetDSBarMulticolorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_BARSPACING", iPlotGetDSBarSpacingAttrib, iPlotSetDSBarSpacingAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_PIERADIUS", iPlotGetDSPieRadiusAttrib, iPlotSetDSPieRadiusAttrib, IUPAF_SAMEASSYSTEM, "0.95", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_PIESTARTANGLE", iPlotGetDSPieStartAngleAttrib, iPlotSetDSPieStartAngleAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_PIECONTOUR", iPlotGetDSPieContourAttrib, iPlotSetDSPieContourAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_PIEHOLE", iPlotGetDSPieHoleAttrib, iPlotSetDSPieHoleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_PIESLICELABEL", iPlotGetDSPieSliceLabelAttrib, iPlotSetDSPieSliceLabelAttrib, IUPAF_SAMEASSYSTEM, "NONE", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_PIESLICELABELPOS", iPlotGetDSPieSliceLabelPosAttrib, iPlotSetDSPieSliceLabelPosAttrib, IUPAF_SAMEASSYSTEM, "0.95", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_STRXDATA", iPlotGetDSStrXDataAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_EXTRA", iPlotGetDSExtraAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_ORDEREDX", iPlotGetDSOrderedXAttrib, iPlotSetDSOrderedXAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_SELECTED", iPlotGetDSSelectedAttrib, iPlotSetDSSelectedAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "VIEWPORTSQUARE", iPlotGetViewportSquareAttrib, iPlotSetViewportSquareAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_SCALEEQUAL", iPlotGetAxisScaleEqualAttrib, iPlotSetAxisScaleEqualAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_AUTOSCALEEQUAL", iPlotGetAxisScaleEqualAttrib, iPlotSetAxisScaleEqualAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_X", iPlotGetAxisXAttrib, iPlotSetAxisXAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_Y", iPlotGetAxisYAttrib, iPlotSetAxisYAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XLABEL", iPlotGetAxisXLabelAttrib, iPlotSetAxisXLabelAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YLABEL", iPlotGetAxisYLabelAttrib, iPlotSetAxisYLabelAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XLABELCENTERED", iPlotGetAxisXLabelCenteredAttrib, iPlotSetAxisXLabelCenteredAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YLABELCENTERED", iPlotGetAxisYLabelCenteredAttrib, iPlotSetAxisYLabelCenteredAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XLABELSPACING", iPlotGetAxisXLabelSpacingAttrib, iPlotSetAxisXLabelSpacingAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YLABELSPACING", iPlotGetAxisYLabelSpacingAttrib, iPlotSetAxisYLabelSpacingAttrib, IUPAF_SAMEASSYSTEM, "AUTO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XFONTSIZE", iPlotGetAxisXFontSizeAttrib, iPlotSetAxisXFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YFONTSIZE", iPlotGetAxisYFontSizeAttrib, iPlotSetAxisYFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XFONTSTYLE", iPlotGetAxisXFontStyleAttrib, iPlotSetAxisXFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YFONTSTYLE", iPlotGetAxisYFontStyleAttrib, iPlotSetAxisYFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XDISCRETE", iPlotGetAxisXDiscreteAttrib, iPlotSetAxisXDiscreteAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YDISCRETE", iPlotGetAxisYDiscreteAttrib, iPlotSetAxisYDiscreteAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XLINEWIDTH", iPlotGetAxisXLineWidthAttrib, iPlotSetAxisXLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YLINEWIDTH", iPlotGetAxisYLineWidthAttrib, iPlotSetAxisYLineWidthAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XCOLOR", iPlotGetAxisXColorAttrib, iPlotSetAxisXColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YCOLOR", iPlotGetAxisYColorAttrib, iPlotSetAxisYColorAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XAUTOMIN", iPlotGetAxisXAutoMinAttrib, iPlotSetAxisXAutoMinAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YAUTOMIN", iPlotGetAxisYAutoMinAttrib, iPlotSetAxisYAutoMinAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XAUTOMAX", iPlotGetAxisXAutoMaxAttrib, iPlotSetAxisXAutoMaxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YAUTOMAX", iPlotGetAxisYAutoMaxAttrib, iPlotSetAxisYAutoMaxAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XMIN", iPlotGetAxisXMinAttrib, iPlotSetAxisXMinAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YMIN", iPlotGetAxisYMinAttrib, iPlotSetAxisYMinAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XMAX", iPlotGetAxisXMaxAttrib, iPlotSetAxisXMaxAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YMAX", iPlotGetAxisYMaxAttrib, iPlotSetAxisYMaxAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XREVERSE", iPlotGetAxisXReverseAttrib, iPlotSetAxisXReverseAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YREVERSE", iPlotGetAxisYReverseAttrib, iPlotSetAxisYReverseAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XREVERSETICKSLABEL", iPlotGetAxisXReverseTicksLabelAttrib, iPlotSetAxisXReverseTicksLabelAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YREVERSETICKSLABEL", iPlotGetAxisYReverseTicksLabelAttrib, iPlotSetAxisYReverseTicksLabelAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XCROSSORIGIN", iPlotGetAxisXCrossOriginAttrib, iPlotSetAxisXCrossOriginAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YCROSSORIGIN", iPlotGetAxisYCrossOriginAttrib, iPlotSetAxisYCrossOriginAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XPOSITION", iPlotGetAxisXPositionAttrib, iPlotSetAxisXPositionAttrib, IUPAF_SAMEASSYSTEM, "START", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YPOSITION", iPlotGetAxisYPositionAttrib, iPlotSetAxisYPositionAttrib, IUPAF_SAMEASSYSTEM, "START", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XSCALE", iPlotGetAxisXScaleAttrib, iPlotSetAxisXScaleAttrib, IUPAF_SAMEASSYSTEM, "LIN", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YSCALE", iPlotGetAxisYScaleAttrib, iPlotSetAxisYScaleAttrib, IUPAF_SAMEASSYSTEM, "LIN", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XARROW", iPlotGetAxisXArrowAttrib, iPlotSetAxisXArrowAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YARROW", iPlotGetAxisYArrowAttrib, iPlotSetAxisYArrowAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XTICK", iPlotGetAxisXTickAttrib, iPlotSetAxisXTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICK", iPlotGetAxisYTickAttrib, iPlotSetAxisYTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XTICKSIZEAUTO", iPlotGetAxisXAutoTickSizeAttrib, iPlotSetAxisXAutoTickSizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKSIZEAUTO", iPlotGetAxisYAutoTickSizeAttrib, iPlotSetAxisYAutoTickSizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_XAUTOTICKSIZE", iPlotGetAxisXAutoTickSizeAttrib, iPlotSetAxisXAutoTickSizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_YAUTOTICKSIZE", iPlotGetAxisYAutoTickSizeAttrib, iPlotSetAxisYAutoTickSizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKMINORSIZE", iPlotGetAxisXTickSizeAttrib, iPlotSetAxisXTickSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKMINORSIZE", iPlotGetAxisYTickSizeAttrib, iPlotSetAxisYTickSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_XTICKSIZE", iPlotGetAxisXTickSizeAttrib, iPlotSetAxisXTickSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_YTICKSIZE", iPlotGetAxisYTickSizeAttrib, iPlotSetAxisYTickSizeAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKMAJORSIZE", iPlotGetAxisXTickMajorSizeAttrib, iPlotSetAxisXTickMajorSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKMAJORSIZE", iPlotGetAxisYTickMajorSizeAttrib, iPlotSetAxisYTickMajorSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XTICKAUTO", iPlotGetAxisXAutoTickAttrib, iPlotSetAxisXAutoTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKAUTO", iPlotGetAxisYAutoTickAttrib, iPlotSetAxisYAutoTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_XAUTOTICK", iPlotGetAxisXAutoTickAttrib, iPlotSetAxisXAutoTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_YAUTOTICK", iPlotGetAxisYAutoTickAttrib, iPlotSetAxisYAutoTickAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKMAJORSPAN", iPlotGetAxisXTickMajorSpanAttrib, iPlotSetAxisXTickMajorSpanAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKMAJORSPAN", iPlotGetAxisYTickMajorSpanAttrib, iPlotSetAxisYTickMajorSpanAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKMINORDIVISION", iPlotGetAxisXTickDivisionAttrib, iPlotSetAxisXTickDivisionAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKMINORDIVISION", iPlotGetAxisYTickDivisionAttrib, iPlotSetAxisYTickDivisionAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_XTICKDIVISION", iPlotGetAxisXTickDivisionAttrib, iPlotSetAxisXTickDivisionAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "AXS_YTICKDIVISION", iPlotGetAxisYTickDivisionAttrib, iPlotSetAxisYTickDivisionAttrib, IUPAF_SAMEASSYSTEM, "5", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XTICKNUMBER", iPlotGetAxisXTickNumberAttrib, iPlotSetAxisXTickNumberAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKNUMBER", iPlotGetAxisYTickNumberAttrib, iPlotSetAxisYTickNumberAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKROTATENUMBER", iPlotGetAxisXTickRotateNumberAttrib, iPlotSetAxisXTickRotateNumberAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKROTATENUMBER", iPlotGetAxisYTickRotateNumberAttrib, iPlotSetAxisYTickRotateNumberAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKROTATENUMBERANGLE", iPlotGetAxisXTickRotateNumberAngleAttrib, iPlotSetAxisXTickRotateNumberAngleAttrib, IUPAF_SAMEASSYSTEM, "90", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKROTATENUMBERANGLE", iPlotGetAxisYTickRotateNumberAngleAttrib, iPlotSetAxisYTickRotateNumberAngleAttrib, IUPAF_SAMEASSYSTEM, "90", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFORMATAUTO", iPlotGetAxisXTickFormatAutoAttrib, iPlotSetAxisXTickFormatAutoAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFORMATAUTO", iPlotGetAxisYTickFormatAutoAttrib, iPlotSetAxisYTickFormatAutoAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFORMAT", iPlotGetAxisXTickFormatAttrib, iPlotSetAxisXTickFormatAttrib, IUPAF_SAMEASSYSTEM, IUP_PLOT_DEF_NUMBERFORMAT, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFORMAT", iPlotGetAxisYTickFormatAttrib, iPlotSetAxisYTickFormatAttrib, IUPAF_SAMEASSYSTEM, IUP_PLOT_DEF_NUMBERFORMAT, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFORMATPRECISION", iPlotGetAxisXTickFormatPrecisionAttrib, iPlotSetAxisXTickFormatPrecisionAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFORMATPRECISION", iPlotGetAxisYTickFormatPrecisionAttrib, iPlotSetAxisYTickFormatPrecisionAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFONTSIZE", iPlotGetAxisXTickFontSizeAttrib, iPlotSetAxisXTickFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFONTSIZE", iPlotGetAxisYTickFontSizeAttrib, iPlotSetAxisYTickFontSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFONTSTYLE", iPlotGetAxisXTickFontStyleAttrib, iPlotSetAxisXTickFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFONTSTYLE", iPlotGetAxisYTickFontStyleAttrib, iPlotSetAxisYTickFontStyleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XTIPFORMAT", iPlotGetAxisXTipFormatAttrib, iPlotSetAxisXTipFormatAttrib, IUPAF_SAMEASSYSTEM, IUP_PLOT_DEF_TIPFORMAT, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTIPFORMAT", iPlotGetAxisYTipFormatAttrib, iPlotSetAxisYTipFormatAttrib, IUPAF_SAMEASSYSTEM, IUP_PLOT_DEF_TIPFORMAT, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTIPFORMATPRECISION", iPlotGetAxisXTipFormatPrecisionAttrib, iPlotSetAxisXTipFormatPrecisionAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTIPFORMATPRECISION", iPlotGetAxisYTipFormatPrecisionAttrib, iPlotSetAxisYTipFormatPrecisionAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "REMOVE", NULL, iPlotSetRemoveAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLEAR", NULL, iPlotSetClearAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", iPlotGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CURRENT", iPlotGetCurrentAttrib, iPlotSetCurrentAttrib, IUPAF_SAMEASSYSTEM, "-1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PLOT_NUMCOL", iPlotGetPlotNumColAttrib, iPlotSetPlotNumColAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PLOT_CURRENT", iPlotGetPlotCurrentAttrib, iPlotSetPlotCurrentAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PLOT_COUNT", iPlotGetPlotCountAttrib, iPlotSetPlotCountAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PLOT_REMOVE", NULL, iPlotSetPlotRemoveAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PLOT_INSERT", NULL, iPlotSetPlotInsertAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FORMULA_MIN", NULL, NULL, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMULA_MAX", NULL, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMULA_PARAMETRIC", NULL, NULL, IUPAF_SAMEASSYSTEM, "No", IUPAF_NO_INHERIT);
}

