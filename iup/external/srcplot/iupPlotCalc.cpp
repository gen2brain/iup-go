
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iupPlot.h"
#include "iup_drvfont.h"



const double kMajorTickXInitialFac = 2.0;
const double kMajorTickYInitialFac = 3.0;
const double kRangeVerySmall = (double)1.0e-3;


void iupPlot::CalculateTitlePos()
{
  // it does not depend on theMargin
  if (mTitle.mAutoPos)
  {
    mTitle.mPosX = mViewport.mWidth / 2;
    mTitle.mPosY = mViewport.mHeight - 1 - mBack.mVertPadding;
  }
}

bool iupPlot::CheckInsideTitle(Ihandle* ih, int x, int y) const
{
  if (mTitle.GetText())
  {
    SetTitleFont(ih);

    int w, h;
    iupDrawGetTextSize(ih, mTitle.GetText(), (int)strlen(mTitle.GetText()), &w, &h, 0);

    int xmin = mTitle.mPosX - w / 2;
    int xmax = mTitle.mPosX + w / 2;
    int ymin = mTitle.mPosY - h;
    int ymax = mTitle.mPosY;

    if (x >= xmin && x <= xmax &&
        y >= ymin && y <= ymax)
      return true;
  }

  return false;
}

bool iupPlot::CheckInsideLegend(int x, int y) const
{
  if (mLegend.mShow)
  {
    if (x >= mLegend.mPos.mX && x < mLegend.mPos.mX + mLegend.mPos.mWidth &&
        y >= mLegend.mPos.mY && y < mLegend.mPos.mY + mLegend.mPos.mHeight)
      return true;
  }

  return false;
}

int iupPlot::CalcTitleVerticalMargin(Ihandle* ih) const
{
  SetTitleFont(ih);

  int theTextHeight;
  iupDrawGetTextSize(ih, mTitle.GetText(), (int)strlen(mTitle.GetText()), NULL, &theTextHeight, 0);
  return theTextHeight + 5 + theTextHeight / 2;
}

int iupPlot::CalcXTickHorizontalMargin(Ihandle* ih, bool start) const
{
  int theXTickHorizontalMargin = 0;

  if (mAxisX.mShowArrow)
  {
    if ((!start && !mAxisX.mReverse) ||
        ( start &&  mAxisX.mReverse))
      theXTickHorizontalMargin += mAxisX.GetArrowSize();
  }

  if (mAxisX.mTick.mShow && mAxisX.mTick.mShowNumber)  // Leave space for half number
    theXTickHorizontalMargin = iupPlotMax(theXTickHorizontalMargin, mAxisX.GetTickNumberWidth(ih) / 2);

  return theXTickHorizontalMargin;
}

int iupPlot::CalcYTickVerticalMargin(Ihandle* ih, bool start) const
{
  int theYTickVerticalMargin = 0;

  if (mAxisY.mShowArrow)
  {
    if ((!start && !mAxisY.mReverse) ||
        ( start &&  mAxisY.mReverse))
        theYTickVerticalMargin += mAxisY.GetArrowSize();
  }

  if (mAxisY.mTick.mShow && mAxisY.mTick.mShowNumber) // Leave space for half number
    theYTickVerticalMargin = iupPlotMax(theYTickVerticalMargin, mAxisY.GetTickNumberHeight(ih) / 2);

  return theYTickVerticalMargin;
}

int iupPlot::CalcXTickVerticalMargin(Ihandle* ih) const
{
  int theXTickVerticalMargin = 0;

  if (mAxisX.mTick.mShow)
  {
    theXTickVerticalMargin += mAxisX.mTick.mMajorSize;

    if (mAxisX.mTick.mShowNumber)
      theXTickVerticalMargin += mAxisX.GetTickNumberHeight(ih) + mAxisX.mTick.mMinorSize;  // Use minor size as spacing
  }

  if (mAxisX.GetLabel())
  {
    int theXFontHeight;
    SetFont(ih, mAxisX.mFontStyle, mAxisX.mFontSize);
    iupdrvFontGetFontDim(IupGetAttribute(ih, "DRAWFONT"), NULL, &theXFontHeight, NULL, NULL);

    theXTickVerticalMargin += theXFontHeight + theXFontHeight / 10;
  }

  return theXTickVerticalMargin;
}

int iupPlot::CalcYTickHorizontalMargin(Ihandle* ih) const
{
  int theYTickHorizontalMargin = 0;

  if (mAxisY.mTick.mShow)
  {
    theYTickHorizontalMargin += mAxisY.mTick.mMajorSize;

    if (mAxisY.mTick.mShowNumber)
      theYTickHorizontalMargin += mAxisY.GetTickNumberWidth(ih) + mAxisY.mTick.mMinorSize;  // Use minor size as spacing
  }

  if (mAxisY.GetLabel())
  {
    int theYFontHeight;
    SetFont(ih, mAxisY.mFontStyle, mAxisY.mFontSize);
    iupdrvFontGetFontDim(IupGetAttribute(ih, "DRAWFONT"), NULL, &theYFontHeight, NULL, NULL);

    theYTickHorizontalMargin += theYFontHeight + theYFontHeight / 10;
  }

  return theYTickHorizontalMargin;
}

void iupPlot::CalculateMargins(Ihandle* ih)
{
  if (mBack.mMarginAuto.mTop)
  {
    mBack.mMargin.mTop = 0;

    if (mTitle.GetText() && mTitle.mAutoPos)  // Affects only top margin
      mBack.mMargin.mTop += CalcTitleVerticalMargin(ih);

    if (mAxisX.mShow && (mAxisX.mPosition == IUP_PLOT_END ||
        (mAxisX.mPosition == IUP_PLOT_CROSSORIGIN &&
         ((!mAxisY.mReverse && 0 >= mAxisY.mMax) || (mAxisY.mReverse && 0 <= mAxisY.mMin)))))
      mBack.mMargin.mTop += CalcXTickVerticalMargin(ih);

    if (mAxisY.mShow)
      mBack.mMargin.mTop = iupPlotMax(mBack.mMargin.mTop, CalcYTickVerticalMargin(ih, false));
  }

  if (mBack.mMarginAuto.mBottom)
  {
    mBack.mMargin.mBottom = 0;

    if (mAxisX.mShow && (mAxisX.mPosition == IUP_PLOT_START ||
        (mAxisX.mPosition == IUP_PLOT_CROSSORIGIN &&
         ((!mAxisY.mReverse && 0 <= mAxisY.mMin) || (mAxisY.mReverse && 0 >= mAxisY.mMax)))))
      mBack.mMargin.mBottom += CalcXTickVerticalMargin(ih);

    if (mAxisY.mShow)
      mBack.mMargin.mBottom = iupPlotMax(mBack.mMargin.mBottom, CalcYTickVerticalMargin(ih, true));
  }

  if (mBack.mMarginAuto.mLeft)
  {
    mBack.mMargin.mLeft = 0;

    if (mAxisY.mShow && (mAxisY.mPosition == IUP_PLOT_START ||
        (mAxisY.mPosition == IUP_PLOT_CROSSORIGIN &&
         ((!mAxisX.mReverse && 0 <= mAxisX.mMin) || (mAxisX.mReverse && 0 >= mAxisX.mMax)))))
      mBack.mMargin.mLeft += CalcYTickHorizontalMargin(ih);

    if (mAxisX.mShow)
      mBack.mMargin.mLeft = iupPlotMax(mBack.mMargin.mLeft, CalcXTickHorizontalMargin(ih, true));
  }

  if (mBack.mMarginAuto.mRight)
  {
    mBack.mMargin.mRight = 0;

    if (mAxisY.mShow && (mAxisY.mPosition == IUP_PLOT_END ||
        (mAxisY.mPosition == IUP_PLOT_CROSSORIGIN &&
         ((!mAxisX.mReverse && 0 >= mAxisX.mMax) || (mAxisX.mReverse && 0 <= mAxisX.mMin)))))
      mBack.mMargin.mRight += CalcYTickHorizontalMargin(ih);

    if (mAxisX.mShow)
      mBack.mMargin.mRight = iupPlotMax(mBack.mMargin.mRight, CalcXTickHorizontalMargin(ih, false));
  }
}

void iupPlot::CalculateXRange(double &outXMin, double &outXMax) const
{
  bool theFirst = true;
  outXMin = 0;
  outXMax = 0;

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    const iupPlotData *theXData = mDataSetList[ds]->GetDataX();

    if (theXData->GetCount() == 0)
      continue;

    double theXMin;
    double theXMax;
    if (mDataSetList[ds]->mMode == IUP_PLOT_PIE)
    {
      theXMin = -1;
      theXMax = 1;
    }
    else if (!theXData->CalculateRange(theXMin, theXMax))
      return;

    if (theFirst)
    {
      outXMin = theXMin;
      outXMax = theXMax;
      theFirst = false;
    }
    if (theXMax>outXMax)
      outXMax = theXMax;
    if (theXMin<outXMin)
      outXMin = theXMin;
  }
}

void iupPlot::CalculateYRange(double &outYMin, double &outYMax) const
{
  bool theFirst = true;
  outYMin = 0;
  outYMax = 0;

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    const iupPlotData *theYData = mDataSetList[ds]->GetDataY();

    double theYMin;
    double theYMax;
    if (mDataSetList[ds]->mMode == IUP_PLOT_PIE)
    {
      theYMin = -1;
      theYMax = 1;
    }
    else if (!theYData->CalculateRange(theYMin, theYMax))
      return;

    if (theFirst)
    {
      outYMin = theYMin;
      outYMax = theYMax;
      theFirst = false;
    }
    if (theYMin<outYMin)
      outYMin = theYMin;
    if (theYMax>outYMax)
      outYMax = theYMax;
  }
}

bool iupPlot::CalculateAxisRange()
{
  if (mAxisX.mAutoScaleMin || mAxisX.mAutoScaleMax)
  {
    double theXMin = 0.0;
    double theXMax = 1.0;

    CalculateXRange(theXMin, theXMax);

    if (mAxisX.mAutoScaleMin)
    {
      mAxisX.mMin = theXMin;
      if (mAxisX.mLogScale && (theXMin < kLogMinClipValue))
        mAxisX.mMin = kLogMinClipValue;
    }

    if (mAxisX.mAutoScaleMax)
      mAxisX.mMax = theXMax;

    if (!mAxisX.mTickIter->AdjustRange(mAxisX.mMin, mAxisX.mMax))
      return false;
  }

  if (mAxisY.mAutoScaleMin || mAxisY.mAutoScaleMax)
  {
    double theYMin = 0.0;
    double theYMax = 1.0;

    CalculateYRange(theYMin, theYMax);

    if (mAxisY.mAutoScaleMin)
    {
      mAxisY.mMin = theYMin;
      if (mAxisY.mLogScale && (theYMin < kLogMinClipValue))
        mAxisY.mMin = kLogMinClipValue;
    }
    if (mAxisY.mAutoScaleMax)
      mAxisY.mMax = theYMax;

    if (!mAxisY.mTickIter->AdjustRange(mAxisY.mMin, mAxisY.mMax))
      return false;
  }

  if (mScaleEqual)
  {
    if (mAxisY.HasZoom() || mAxisX.HasZoom())
    {
      if (mAxisY.mMax - mAxisY.mMin != mAxisX.mMax - mAxisX.mMin)
      {
        double theLength;

        if (mAxisY.mMax - mAxisY.mMin > mAxisX.mMax - mAxisX.mMin)
        {
          theLength = mAxisY.mMax - mAxisY.mMin;
          mAxisX.mMax = mAxisX.mMin + theLength;
        }
        else
        {
          theLength = mAxisX.mMax - mAxisX.mMin;
          mAxisY.mMax = mAxisY.mMin + theLength;
        }
      }
    }
    else
    {
      double theMin = mAxisY.mMin;
      if (mAxisX.mMin < theMin)
        theMin = mAxisX.mMin;

      double theMax = mAxisY.mMax;
      if (mAxisX.mMax > theMax)
        theMax = mAxisX.mMax;

      mAxisX.mMin = theMin;
      mAxisY.mMin = theMin;
      mAxisX.mMax = theMax;
      mAxisY.mMax = theMax;
    }
  }

  return true;
}

bool iupPlot::CheckRange(const iupPlotAxis &inAxis) const
{
  if (inAxis.mLogScale)
  {
    if (inAxis.mMin < kLogMinClipValue)
      return false;
  }
  return true;
}


bool iupPlot::CalculateXTransformation(const iupPlotRect &inRect)
{
  return mAxisX.mTrafo->Calculate(inRect.mX, inRect.mX + inRect.mWidth, mAxisX);
}

bool iupPlot::CalculateYTransformation(const iupPlotRect &inRect)
{
  return mAxisY.mTrafo->Calculate(inRect.mY, inRect.mY + inRect.mHeight, mAxisY);
}

void iupPlot::CalculateTickSize(Ihandle* ih, iupPlotTick &ioTick)
{
  if (ioTick.mAutoSize)
  {
    int theFontHeight;
    SetFont(ih, ioTick.mFontStyle, ioTick.mFontSize);
    iupdrvFontGetFontDim(IupGetAttribute(ih, "DRAWFONT"), NULL, &theFontHeight, NULL, NULL);

    ioTick.mMajorSize = theFontHeight / 2;
    ioTick.mMinorSize = theFontHeight / 4;
  }
}

bool iupPlot::CalculateTickSpacing(const iupPlotRect &inRect, Ihandle* ih)
{
  double theXRange = mAxisX.mMax - mAxisX.mMin;
  double theYRange = mAxisY.mMax - mAxisY.mMin;

  if (theXRange <= 0 || theYRange < 0)
    return false;

  // YRange can be 0, must adjust
  if (mAxisY.mAutoScaleMax && ((mAxisY.mMax != 0 && fabs(theYRange / mAxisY.mMax) < kRangeVerySmall) || theYRange == 0))
  {
    double delta = 0.1;
    if (mAxisY.mMax != 0)
      delta *= fabs(mAxisY.mMax);

    mAxisY.mMax += delta;
    theYRange = mAxisY.mMax - mAxisY.mMin;
  }

  if (mAxisX.mTick.mAutoSpacing)
  {
    int theXFontHeight;
    SetFont(ih, mAxisX.mTick.mFontStyle, mAxisX.mTick.mFontSize);
    iupdrvFontGetFontDim(IupGetAttribute(ih, "DRAWFONT"), NULL, &theXFontHeight, NULL, NULL);

    int theTextWidth;
    iupDrawGetTextSize(ih, "12345", 5, &theTextWidth, NULL, 0);

    double theDivGuess = inRect.mWidth / (kMajorTickXInitialFac*theTextWidth);
    if (!mAxisX.mTickIter->CalculateSpacing(theXRange, theDivGuess, mAxisX.mTick))
      return false;
  }

  if (mAxisY.mTick.mAutoSpacing)
  {
    int theYFontHeight;
    SetFont(ih, mAxisY.mTick.mFontStyle, mAxisY.mTick.mFontSize);
    iupdrvFontGetFontDim(IupGetAttribute(ih, "DRAWFONT"), NULL, &theYFontHeight, NULL, NULL);

    double theDivGuess = inRect.mHeight / (kMajorTickYInitialFac*theYFontHeight);
    if (!mAxisY.mTickIter->CalculateSpacing(theYRange, theDivGuess, mAxisY.mTick))
      return false;
  }

  return true;
}
