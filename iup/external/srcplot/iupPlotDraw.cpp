#include <stdio.h>
#include <string.h>

#include "iupPlot.h"
#include "iup_drvfont.h"


void iupPlotBuildFont(Ihandle* ih, int fontStyle, int fontSize, char* fontStr, int fontStrSize)
{
  char* fontFace = IupGetAttribute(ih, "FONTFACE");
  if (!fontFace)
    fontFace = (char*)"";

  const char* styleStr = "";
  switch (fontStyle)
  {
  case IUP_PLOT_FONT_BOLD:        styleStr = "Bold "; break;
  case IUP_PLOT_FONT_ITALIC:      styleStr = "Italic "; break;
  case IUP_PLOT_FONT_BOLD_ITALIC: styleStr = "Bold Italic "; break;
  default:                        styleStr = ""; break;
  }

  snprintf(fontStr, fontStrSize, "%s, %s%d", fontFace, styleStr, fontSize);
}

void iupPlotDrawMark(iupPlotDrawContext* ctx, int x, int y, int markStyle, int markSize, long color)
{
  int s = markSize;

  switch (markStyle)
  {
  case IUP_PLOT_MARK_PLUS:
    iupPlotDrawLine(ctx->ih, x - s, y, x + s, y, color, IUP_DRAW_STROKE, 1);
    iupPlotDrawLine(ctx->ih, x, y - s, x, y + s, color, IUP_DRAW_STROKE, 1);
    break;
  case IUP_PLOT_MARK_X:
    iupPlotDrawLine(ctx->ih, x - s, y - s, x + s, y + s, color, IUP_DRAW_STROKE, 1);
    iupPlotDrawLine(ctx->ih, x - s, y + s, x + s, y - s, color, IUP_DRAW_STROKE, 1);
    break;
  case IUP_PLOT_MARK_STAR:
    iupPlotDrawLine(ctx->ih, x - s, y, x + s, y, color, IUP_DRAW_STROKE, 1);
    iupPlotDrawLine(ctx->ih, x, y - s, x, y + s, color, IUP_DRAW_STROKE, 1);
    iupPlotDrawLine(ctx->ih, x - s, y - s, x + s, y + s, color, IUP_DRAW_STROKE, 1);
    iupPlotDrawLine(ctx->ih, x - s, y + s, x + s, y - s, color, IUP_DRAW_STROKE, 1);
    break;
  case IUP_PLOT_MARK_CIRCLE:
    iupPlotDrawArc(ctx->ih, x - s, y - s, x + s, y + s, 0, 360, color, IUP_DRAW_FILL, 1);
    break;
  case IUP_PLOT_MARK_HOLLOW_CIRCLE:
    iupPlotDrawArc(ctx->ih, x - s, y - s, x + s, y + s, 0, 360, color, IUP_DRAW_STROKE, 1);
    break;
  case IUP_PLOT_MARK_BOX:
    iupPlotDrawRectangle(ctx->ih, x - s, y - s, x + s, y + s, color, IUP_DRAW_FILL, 1);
    break;
  case IUP_PLOT_MARK_HOLLOW_BOX:
    iupPlotDrawRectangle(ctx->ih, x - s, y - s, x + s, y + s, color, IUP_DRAW_STROKE, 1);
    break;
  case IUP_PLOT_MARK_DIAMOND:
    {
      int points[8];
      points[0] = x;     points[1] = y - s;
      points[2] = x + s; points[3] = y;
      points[4] = x;     points[5] = y + s;
      points[6] = x - s; points[7] = y;
      iupPlotDrawPolygon(ctx->ih, points, 4, color, IUP_DRAW_FILL, 1);
    }
    break;
  case IUP_PLOT_MARK_HOLLOW_DIAMOND:
    {
      int points[8];
      points[0] = x;     points[1] = y - s;
      points[2] = x + s; points[3] = y;
      points[4] = x;     points[5] = y + s;
      points[6] = x - s; points[7] = y;
      iupPlotDrawPolygon(ctx->ih, points, 4, color, IUP_DRAW_STROKE, 1);
    }
    break;
  }
}

void iupPlotDrawAlignedText(iupPlotDrawContext* ctx, double refX, double refY,
                            int plotAlignment, const char* text, long color,
                            const char* font, double orientation)
{
  int absX = iupPlotDrawCalcX(ctx, refX);
  int absY = iupPlotDrawCalcY(ctx, refY);

  int len = (int)strlen(text);
  int w = 0, h = 0;

  IupSetAttribute(ctx->ih, "DRAWFONT", font);
  iupDrawGetTextSize(ctx->ih, text, len, &w, &h, orientation);

  int drawX = absX;
  int drawY = absY;

  switch (plotAlignment)
  {
  case IUP_PLOT_NORTH:
    drawX = absX - w / 2;
    drawY = absY;
    break;
  case IUP_PLOT_SOUTH:
    drawX = absX - w / 2;
    drawY = absY - h;
    break;
  case IUP_PLOT_WEST:
    drawX = absX;
    drawY = absY - h / 2;
    break;
  case IUP_PLOT_EAST:
    drawX = absX - w;
    drawY = absY - h / 2;
    break;
  case IUP_PLOT_NORTH_EAST:
    drawX = absX - w;
    drawY = absY;
    break;
  case IUP_PLOT_NORTH_WEST:
    drawX = absX;
    drawY = absY;
    break;
  case IUP_PLOT_SOUTH_EAST:
    drawX = absX - w;
    drawY = absY - h;
    break;
  case IUP_PLOT_SOUTH_WEST:
    drawX = absX;
    drawY = absY - h;
    break;
  case IUP_PLOT_CENTER:
    drawX = absX - w / 2;
    drawY = absY - h / 2;
    break;
  }

  iupPlotDrawText(ctx->ih, text, len, drawX, drawY, w, h, color, font, IUP_DRAW_LEFT, orientation);
}

/************************************************************************************/

void iupPlotBox::Draw(const iupPlotRect &inRect, iupPlotDrawContext* ctx) const
{
  int x1 = iupPlotDrawCalcX(ctx, inRect.mX);
  int y1 = iupPlotDrawCalcY(ctx, inRect.mY + inRect.mHeight - 1);
  int x2 = iupPlotDrawCalcX(ctx, inRect.mX + inRect.mWidth - 1);
  int y2 = iupPlotDrawCalcY(ctx, inRect.mY);

  int drawStyle = iupPlotLineStyleToDrawStyle(mLineStyle);
  iupPlotDrawRectangle(ctx->ih, x1, y1, x2, y2, mColor, drawStyle, mLineWidth);
}

bool iupPlotGrid::DrawX(iupPlotTickIter* inTickIter, iupPlotTrafo* inTrafo, const iupPlotRect &inRect, iupPlotDrawContext* ctx) const
{
  if (mShowX)
  {
    if (!inTickIter->Init())
      return false;

    double theX;
    bool theIsMajorTick;

    int drawStyle = iupPlotLineStyleToDrawStyle(mLineStyle);

    int absY1 = iupPlotDrawCalcY(ctx, inRect.mY);
    int absY2 = iupPlotDrawCalcY(ctx, inRect.mY + inRect.mHeight - 1);

    while (inTickIter->GetNextTick(theX, theIsMajorTick, NULL))
    {
      if ((theIsMajorTick && mMajor) || (!theIsMajorTick && !mMajor))
      {
        double theScreenX = inTrafo->Transform(theX);
        int absX = iupPlotDrawCalcX(ctx, theScreenX);
        iupPlotDrawLine(ctx->ih, absX, absY1, absX, absY2, mColor, drawStyle, mLineWidth);
      }
    }
  }

  return true;
}

bool iupPlotGrid::DrawY(iupPlotTickIter* inTickIter, iupPlotTrafo* inTrafo, const iupPlotRect &inRect, iupPlotDrawContext* ctx) const
{
  if (mShowY)
  {
    if (!inTickIter->Init())
      return false;

    double theY;
    bool theIsMajorTick;

    int drawStyle = iupPlotLineStyleToDrawStyle(mLineStyle);

    int absX1 = iupPlotDrawCalcX(ctx, inRect.mX);
    int absX2 = iupPlotDrawCalcX(ctx, inRect.mX + inRect.mWidth - 1);

    while (inTickIter->GetNextTick(theY, theIsMajorTick, NULL))
    {
      if ((theIsMajorTick && mMajor) || (!theIsMajorTick && !mMajor))
      {
        double theScreenY = inTrafo->Transform(theY);
        int absY = iupPlotDrawCalcY(ctx, theScreenY);
        iupPlotDrawLine(ctx->ih, absX1, absY, absX2, absY, mColor, drawStyle, mLineWidth);
      }
    }
  }

  return true;
}

/************************************************************************************************/

void iupPlot::DrawCrossSamplesH(const iupPlotRect &inRect, const iupPlotData *inXData, const iupPlotData *inYData, iupPlotDrawContext* ctx) const
{
  int theCount = inXData->GetCount();
  if (theCount == 0)
    return;

  double theXTarget = mAxisX.mTrafo->TransformBack((double)mCrossHairX);
  bool theFirstIsLess = inXData->GetSample(0) < theXTarget;

  int absX1 = iupPlotDrawCalcX(ctx, inRect.mX);
  int absX2 = iupPlotDrawCalcX(ctx, inRect.mX + inRect.mWidth - 1);

  long color = iupDrawColor(0, 0, 0, 255);
  IupGetAttribute(ctx->ih, "FGCOLOR");

  for (int i = 0; i < theCount; i++)
  {
    double theX = inXData->GetSample(i);
    bool theCurrentIsLess = theX < theXTarget;

    if (theCurrentIsLess != theFirstIsLess)
    {
      double theY = inYData->GetSample(i);
      double theScreenY = mAxisY.mTrafo->Transform(theY);
      int absY = iupPlotDrawCalcY(ctx, theScreenY);

      iupPlotDrawLine(ctx->ih, absX1, absY, absX2, absY, color, IUP_DRAW_STROKE, 1);

      theFirstIsLess = theCurrentIsLess;
    }
  }
}

void iupPlot::DrawCrossHairH(const iupPlotRect &inRect, iupPlotDrawContext* ctx) const
{
  int absX = iupPlotDrawCalcX(ctx, mCrossHairX);
  int absY1 = iupPlotDrawCalcY(ctx, inRect.mY);
  int absY2 = iupPlotDrawCalcY(ctx, inRect.mY + inRect.mHeight - 1);

  iupPlotDrawLine(ctx->ih, absX, absY1, absX, absY2, mAxisY.mColor, IUP_DRAW_STROKE, 1);

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];

    const iupPlotData *theXData = dataset->GetDataX();
    const iupPlotData *theYData = dataset->GetDataY();

    DrawCrossSamplesH(inRect, theXData, theYData, ctx);
  }
}

void iupPlot::DrawCrossSamplesV(const iupPlotRect &inRect, const iupPlotData *inXData, const iupPlotData *inYData, iupPlotDrawContext* ctx) const
{
  int theCount = inXData->GetCount();
  if (theCount == 0)
    return;

  double theYTarget = mAxisY.mTrafo->TransformBack((double)mCrossHairY);
  bool theFirstIsLess = inYData->GetSample(0) < theYTarget;

  int absY1 = iupPlotDrawCalcY(ctx, inRect.mY);
  int absY2 = iupPlotDrawCalcY(ctx, inRect.mY + inRect.mHeight - 1);

  long color = iupDrawColor(0, 0, 0, 255);

  for (int i = 0; i < theCount; i++)
  {
    double theY = inYData->GetSample(i);
    bool theCurrentIsLess = theY < theYTarget;

    if (theCurrentIsLess != theFirstIsLess)
    {
      double theX = inXData->GetSample(i);
      double theScreenX = mAxisX.mTrafo->Transform(theX);
      int absX = iupPlotDrawCalcX(ctx, theScreenX);

      iupPlotDrawLine(ctx->ih, absX, absY1, absX, absY2, color, IUP_DRAW_STROKE, 1);

      theFirstIsLess = theCurrentIsLess;
    }
  }
}

void iupPlot::DrawCrossHairV(const iupPlotRect &inRect, iupPlotDrawContext* ctx) const
{
  int absX1 = iupPlotDrawCalcX(ctx, inRect.mX);
  int absX2 = iupPlotDrawCalcX(ctx, inRect.mX + inRect.mWidth - 1);
  int absY = iupPlotDrawCalcY(ctx, mCrossHairY);

  iupPlotDrawLine(ctx->ih, absX1, absY, absX2, absY, mAxisX.mColor, IUP_DRAW_STROKE, 1);

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];

    const iupPlotData *theXData = dataset->GetDataX();
    const iupPlotData *theYData = dataset->GetDataY();

    DrawCrossSamplesV(inRect, theXData, theYData, ctx);
  }
}

void iupPlot::SetTitleFont(Ihandle* ih) const
{
  int theFontSize = mTitle.mFontSize;
  if (theFontSize == 0)
  {
    int size = IupGetInt(ih, "FONTSIZE");
    if (size > 0) size += 6;
    else size -= 8;

    theFontSize = size;
  }

  SetFont(ih, mTitle.mFontStyle, theFontSize);
}

void iupPlot::DrawTitle(iupPlotDrawContext* ctx) const
{
  if (mTitle.GetText())
  {
    SetTitleFont(ctx->ih);

    char fontStr[256];
    iupPlotBuildFont(ctx->ih, mTitle.mFontStyle == -1 ? mDefaultFontStyle : mTitle.mFontStyle,
                     mTitle.mFontSize == 0 ? (IupGetInt(ih, "FONTSIZE") > 0 ? IupGetInt(ih, "FONTSIZE") + 6 : IupGetInt(ih, "FONTSIZE") - 8) : mTitle.mFontSize,
                     fontStr, sizeof(fontStr));

    iupPlotDrawAlignedText(ctx, mTitle.mPosX, mTitle.mPosY, IUP_PLOT_NORTH,
                           mTitle.GetText(), mTitle.mColor, fontStr, 0);
  }
}

void iupPlot::DrawBackground(iupPlotDrawContext* ctx) const
{
  if (!mBack.mTransparent)
  {
    IupDrawResetClip(ctx->ih);
    iupPlotDrawRectangle(ctx->ih, mViewportBack.mX, mViewportBack.mY,
                        mViewportBack.mX + mViewportBack.mWidth - 1,
                        mViewportBack.mY + mViewportBack.mHeight - 1,
                        mBack.mColor, IUP_DRAW_FILL, 1);
  }
}

void iupPlot::DrawInactive(iupPlotDrawContext* ctx) const
{
  IupDrawResetClip(ctx->ih);
  long inactive_color = iupDrawColor(128, 128, 128, 96);
  iupPlotDrawRectangle(ctx->ih, mViewportBack.mX, mViewportBack.mY,
                      mViewportBack.mX + mViewportBack.mWidth - 1,
                      mViewportBack.mY + mViewportBack.mHeight - 1,
                      inactive_color, IUP_DRAW_FILL, 1);
}

void iupPlot::DrawBackgroundImage(iupPlotDrawContext* ctx) const
{
  Ihandle* image = IupImageGetHandle(mBack.GetImage());
  if (image)
  {
    double theScreenMinX = mAxisX.mTrafo->Transform(mBack.mImageMinX);
    double theScreenMinY = mAxisY.mTrafo->Transform(mBack.mImageMinY);
    double theScreenMaxX = mAxisX.mTrafo->Transform(mBack.mImageMaxX);
    double theScreenMaxY = mAxisY.mTrafo->Transform(mBack.mImageMaxY);

    double theScreenW = theScreenMaxX - theScreenMinX + 1;
    double theScreenH = theScreenMaxY - theScreenMinY + 1;

    int theX = iupPlotDrawCalcX(ctx, theScreenMinX);
    int theY = iupPlotDrawCalcY(ctx, theScreenMinY + theScreenH - 1);
    int theW = iupPlotRound(theScreenW);
    int theH = iupPlotRound(theScreenH);

    iupPlotDrawImage(ctx->ih, mBack.GetImage(), 0, NULL, theX, theY, theW, theH);
  }
}

bool iupPlot::DrawLegend(const iupPlotRect &inRect, iupPlotDrawContext* ctx, iupPlotRect &ioPos) const
{
  if (mLegend.mShow)
  {
    int ds;
    int theFontHeight;

    char fontStr[256];
    int fontStyle = mLegend.mFontStyle == -1 ? mDefaultFontStyle : mLegend.mFontStyle;
    int fontSize = mLegend.mFontSize == 0 ? mDefaultFontSize : mLegend.mFontSize;
    iupPlotBuildFont(ctx->ih, fontStyle, fontSize, fontStr, sizeof(fontStr));
    SetFont(ctx->ih, mLegend.mFontStyle, mLegend.mFontSize);

    iupdrvFontGetFontDim(IupGetAttribute(ctx->ih, "DRAWFONT"), NULL, &theFontHeight, NULL, NULL);

    int theMargin = theFontHeight / 2;
    if (mLegend.mPosition == IUP_PLOT_BOTTOMCENTER)
      theMargin = 0;
    int theTotalHeight = mDataSetListCount*theFontHeight + 2 * theMargin;
    int theLineSpace = 20;

    int theWidth, theMaxWidth = 0;
    for (ds = 0; ds < mDataSetListCount; ds++)
    {
      iupPlotDataSet* dataset = mDataSetList[ds];

      IupSetAttribute(ctx->ih, "DRAWFONT", fontStr);
      iupDrawGetTextSize(ctx->ih, dataset->GetName(), (int)strlen(dataset->GetName()), &theWidth, NULL, 0);

      if (dataset->mMode == IUP_PLOT_MARK || dataset->mMode == IUP_PLOT_MARKLINE)
      {
        if (dataset->mMarkSize + 6 > theLineSpace)
          theLineSpace = dataset->mMarkSize + 6;
      }

      theWidth += theLineSpace;

      if (theWidth > theMaxWidth)
        theMaxWidth = theWidth;
    }

    if (theMaxWidth == 0)
      return false;

    theMaxWidth += 2 * theMargin;

    if (mLegend.mPosition != IUP_PLOT_XY)
    {
      int theScreenX = inRect.mX;
      int theScreenY = inRect.mY;

      switch (mLegend.mPosition)
      {
      case IUP_PLOT_TOPLEFT:
        theScreenX += 2;
        theScreenY += inRect.mHeight - theTotalHeight - 2;
        break;
      case IUP_PLOT_BOTTOMLEFT:
        theScreenX += 2;
        theScreenY += 2;
        break;
      case IUP_PLOT_BOTTOMRIGHT:
        theScreenX += inRect.mWidth - theMaxWidth - 2;
        theScreenY += 2;
        break;
      case IUP_PLOT_BOTTOMCENTER:
        theScreenX += (inRect.mWidth - theMaxWidth) / 2;
        theScreenY = theFontHeight / 4;
        break;
      default: /* IUP_PLOT_TOPRIGHT */
        theScreenX += inRect.mWidth - theMaxWidth - 2;
        theScreenY += inRect.mHeight - theTotalHeight - 2;
        break;
      }

      ioPos.mX = theScreenX;
      ioPos.mY = theScreenY;
    }

    ioPos.mWidth = theMaxWidth;
    ioPos.mHeight = theTotalHeight;

    int clipX1 = iupPlotDrawCalcX(ctx, ioPos.mX);
    int clipY1 = iupPlotDrawCalcY(ctx, ioPos.mY + ioPos.mHeight - 1);
    int clipX2 = iupPlotDrawCalcX(ctx, ioPos.mX + ioPos.mWidth - 1);
    int clipY2 = iupPlotDrawCalcY(ctx, ioPos.mY);
    IupDrawSetClipRect(ctx->ih, clipX1, clipY1, clipX2, clipY2);

    if (mLegend.mBoxShow)
    {
      int bx1 = iupPlotDrawCalcX(ctx, ioPos.mX + 1);
      int by1 = iupPlotDrawCalcY(ctx, ioPos.mY + ioPos.mHeight - 2);
      int bx2 = iupPlotDrawCalcX(ctx, ioPos.mX + ioPos.mWidth - 2);
      int by2 = iupPlotDrawCalcY(ctx, ioPos.mY + 1);
      iupPlotDrawRectangle(ctx->ih, bx1, by1, bx2, by2, mLegend.mBoxBackColor, IUP_DRAW_FILL, 1);

      int rx1 = iupPlotDrawCalcX(ctx, ioPos.mX);
      int ry1 = iupPlotDrawCalcY(ctx, ioPos.mY + ioPos.mHeight - 1);
      int rx2 = iupPlotDrawCalcX(ctx, ioPos.mX + ioPos.mWidth - 1);
      int ry2 = iupPlotDrawCalcY(ctx, ioPos.mY);
      int drawStyle = iupPlotLineStyleToDrawStyle(mLegend.mBoxLineStyle);
      iupPlotDrawRectangle(ctx->ih, rx1, ry1, rx2, ry2, mLegend.mBoxColor, drawStyle, mLegend.mBoxLineWidth);
    }

    for (ds = 0; ds < mDataSetListCount; ds++)
    {
      iupPlotDataSet* dataset = mDataSetList[ds];

      int theLegendX = ioPos.mX + theMargin;
      int theLegendY = ioPos.mY + (mDataSetListCount - 1 - ds)*theFontHeight + theMargin;

      int theLegendCenterY = theLegendY + theFontHeight / 2;
      int theMarkY = theLegendCenterY - theFontHeight / 8;

      if (dataset->mMode == IUP_PLOT_MARK || dataset->mMode == IUP_PLOT_MARKLINE)
      {
        int absMarkX = iupPlotDrawCalcX(ctx, theLegendX + (theLineSpace - 3) / 2);
        int absMarkY = iupPlotDrawCalcY(ctx, theMarkY);
        iupPlotDrawMark(ctx, absMarkX, absMarkY, dataset->mMarkStyle, dataset->mMarkSize, dataset->mColor);
      }
      if (dataset->mMode != IUP_PLOT_MARK)
      {
        int absLX1 = iupPlotDrawCalcX(ctx, theLegendX);
        int absLX2 = iupPlotDrawCalcX(ctx, theLegendX + theLineSpace - 3);
        int absLY = iupPlotDrawCalcY(ctx, theMarkY);
        int drawStyle = iupPlotLineStyleToDrawStyle(dataset->mLineStyle);
        iupPlotDrawLine(ctx->ih, absLX1, absLY, absLX2, absLY, dataset->mColor, drawStyle, dataset->mLineWidth);
      }

      double textRefX = theLegendX + theLineSpace;
      double textRefY = theLegendCenterY;
      iupPlotDrawAlignedText(ctx, textRefX, textRefY, IUP_PLOT_WEST, dataset->GetName(), dataset->mColor, fontStr, 0);
    }
  }

  return true;
}

int iupStrToColor(const char* str, long *color);

long iupPlotDrawGetSampleColorTable(Ihandle* ih, int index)
{
  char* value = IupGetAttributeId(ih, "SAMPLECOLOR", index);
  long color;
  if (iupStrToColor(value, &color))
    return color;

  switch (index % 12)
  {
  case  0: return iupDrawColor(220, 60, 20, 255);
  case  1: return iupDrawColor(0, 128, 0, 255);
  case  2: return iupDrawColor(20, 100, 220, 255);

  case  3: return iupDrawColor(220, 128, 0, 255);
  case  4: return iupDrawColor(128, 0, 128, 255);
  case  5: return iupDrawColor(0, 128, 220, 255);

  case  6: return iupDrawColor(220, 60, 128, 255);
  case  7: return iupDrawColor(128, 220, 0, 255);
  case  8: return iupDrawColor(192, 60, 60, 255);

  case  9: return iupDrawColor(60, 60, 128, 255);
  case 10: return iupDrawColor(220, 60, 220, 255);
  case 11: return iupDrawColor(60, 128, 128, 255);
  }

  return 0;
}

bool iupPlot::DrawSampleColorLegend(iupPlotDataSet *dataset, const iupPlotRect &inRect, iupPlotDrawContext* ctx, iupPlotRect &ioPos) const
{
  if (mLegend.mShow)
  {
    int theFontHeight;

    char fontStr[256];
    int fontStyle = mLegend.mFontStyle == -1 ? mDefaultFontStyle : mLegend.mFontStyle;
    int fontSize = mLegend.mFontSize == 0 ? mDefaultFontSize : mLegend.mFontSize;
    iupPlotBuildFont(ctx->ih, fontStyle, fontSize, fontStr, sizeof(fontStr));
    SetFont(ctx->ih, mLegend.mFontStyle, mLegend.mFontSize);

    iupdrvFontGetFontDim(IupGetAttribute(ctx->ih, "DRAWFONT"), NULL, &theFontHeight, NULL, NULL);

    int theMargin = theFontHeight / 2;
    if (mLegend.mPosition == IUP_PLOT_BOTTOMCENTER)
      theMargin = 0;
    int theCount = dataset->GetCount();
    int theTotalHeight = theCount*theFontHeight + 2 * theMargin;
    int theLineSpace = theFontHeight / 2 + 3;

    int theWidth, theMaxWidth = 0;
    for (int i = 0; i < theCount; i++)
    {
      IupSetAttribute(ctx->ih, "DRAWFONT", fontStr);
      const char* sampleStr = ((iupPlotDataString *)dataset->GetDataX())->GetSampleString(i);
      iupDrawGetTextSize(ctx->ih, sampleStr, (int)strlen(sampleStr), &theWidth, NULL, 0);

      theWidth += theLineSpace;

      if (theWidth > theMaxWidth)
        theMaxWidth = theWidth;
    }

    if (theMaxWidth == 0)
      return false;

    theMaxWidth += 2 * theMargin;

    if (mLegend.mPosition != IUP_PLOT_XY)
    {
      int theScreenX = inRect.mX;
      int theScreenY = inRect.mY;

      switch (mLegend.mPosition)
      {
      case IUP_PLOT_TOPLEFT:
        theScreenX += 2;
        theScreenY += inRect.mHeight - theTotalHeight - 2;
        break;
      case IUP_PLOT_BOTTOMLEFT:
        theScreenX += 2;
        theScreenY += 2;
        break;
      case IUP_PLOT_BOTTOMRIGHT:
        theScreenX += inRect.mWidth - theMaxWidth - 2;
        theScreenY += 2;
        break;
      case IUP_PLOT_BOTTOMCENTER:
        theScreenX += (inRect.mWidth - theMaxWidth) / 2;
        theScreenY = theFontHeight / 4;
        break;
      default: /* IUP_PLOT_TOPRIGHT */
        theScreenX += inRect.mWidth - theMaxWidth - 2;
        theScreenY += inRect.mHeight - theTotalHeight - 2;
        break;
      }

      ioPos.mX = theScreenX;
      ioPos.mY = theScreenY;
    }

    ioPos.mWidth = theMaxWidth;
    ioPos.mHeight = theTotalHeight;

    int clipX1 = iupPlotDrawCalcX(ctx, ioPos.mX);
    int clipY1 = iupPlotDrawCalcY(ctx, ioPos.mY + ioPos.mHeight - 1);
    int clipX2 = iupPlotDrawCalcX(ctx, ioPos.mX + ioPos.mWidth - 1);
    int clipY2 = iupPlotDrawCalcY(ctx, ioPos.mY);
    IupDrawSetClipRect(ctx->ih, clipX1, clipY1, clipX2, clipY2);

    if (mLegend.mBoxShow)
    {
      int bx1 = iupPlotDrawCalcX(ctx, ioPos.mX + 1);
      int by1 = iupPlotDrawCalcY(ctx, ioPos.mY + ioPos.mHeight - 2);
      int bx2 = iupPlotDrawCalcX(ctx, ioPos.mX + ioPos.mWidth - 2);
      int by2 = iupPlotDrawCalcY(ctx, ioPos.mY + 1);
      iupPlotDrawRectangle(ctx->ih, bx1, by1, bx2, by2, mLegend.mBoxBackColor, IUP_DRAW_FILL, 1);

      int rx1 = iupPlotDrawCalcX(ctx, ioPos.mX);
      int ry1 = iupPlotDrawCalcY(ctx, ioPos.mY + ioPos.mHeight - 1);
      int rx2 = iupPlotDrawCalcX(ctx, ioPos.mX + ioPos.mWidth - 1);
      int ry2 = iupPlotDrawCalcY(ctx, ioPos.mY);
      int drawStyle = iupPlotLineStyleToDrawStyle(mLegend.mBoxLineStyle);
      iupPlotDrawRectangle(ctx->ih, rx1, ry1, rx2, ry2, mLegend.mBoxColor, drawStyle, mLegend.mBoxLineWidth);
    }

    for (int i = 0; i < theCount; i++)
    {
      long sampleColor = iupPlotDrawGetSampleColorTable(ih, i);

      int theLegendX = ioPos.mX + theMargin;
      int theLegendY = ioPos.mY + (theCount - 1 - i)*theFontHeight + theMargin;

      int boxSize = theLineSpace - 3;

      int bx1 = iupPlotDrawCalcX(ctx, theLegendX);
      int by1 = iupPlotDrawCalcY(ctx, theLegendY + boxSize);
      int bx2 = iupPlotDrawCalcX(ctx, theLegendX + boxSize);
      int by2 = iupPlotDrawCalcY(ctx, theLegendY);
      iupPlotDrawRectangle(ctx->ih, bx1, by1, bx2, by2, sampleColor, IUP_DRAW_FILL, 1);

      double textRefX = theLegendX + theLineSpace;
      double textRefY = theLegendY + boxSize / 2;
      iupPlotDrawAlignedText(ctx, textRefX, textRefY, IUP_PLOT_WEST,
                             ((iupPlotDataString *)dataset->GetDataX())->GetSampleString(i),
                             sampleColor, fontStr, 0);
    }
  }

  return true;
}
