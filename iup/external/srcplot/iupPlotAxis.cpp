#include <stdio.h>
#include <string.h>

#include "iupPlot.h"
#include "iup_drvfont.h"


static int iPlotCountDigit(int inNum)
{
  int theCount = 0;
  while (inNum != 0)
  {
    inNum = inNum / 10;
    theCount++;
  }
  if (theCount == 0) theCount = 1;
  return theCount;
}

static int iPlotEstimateNumberCharCount(bool inFormatAuto, const char* inFormatString, double inMin, double inMax)
{
  int thePrecision = 0;

  while (*inFormatString)
  {
    if (*inFormatString == '.')
      break;
    inFormatString++;
  }

  if (*inFormatString == '.')
  {
    inFormatString++;
    iupStrToInt(inFormatString, &thePrecision);
  }

  if (inFormatAuto)
  {
    int theMinPrecision = iupPlotCalcPrecision(inMin);
    int theMaxPrecision = iupPlotCalcPrecision(inMax);
    if (theMinPrecision > theMaxPrecision)
      thePrecision = iupPlotMax(thePrecision, theMinPrecision);
    else
      thePrecision = iupPlotMax(thePrecision, theMaxPrecision);
  }

  int theMin = iupPlotRound(inMin);
  int theMax = iupPlotRound(inMax);
  int theNumDigitMin = iPlotCountDigit(theMin);
  int theNumDigitMax = iPlotCountDigit(theMax);
  if (theNumDigitMin > theNumDigitMax)
    thePrecision += theNumDigitMin;
  else
    thePrecision += theNumDigitMax;

  thePrecision += 3;  // sign, decimal symbol, exp

  return thePrecision;
}

static bool iPlotGetTickFormat(Ihandle* ih, IFnssds formatticknumber_cb, char* inBuf, const char *inFormatString, double inValue)
{
  char* decimal_symbol = IupGetGlobal("DEFAULTDECIMALSYMBOL");

  if (formatticknumber_cb)
  {
    int ret = formatticknumber_cb(ih, inBuf, (char*)inFormatString, inValue, decimal_symbol);
    if (ret == IUP_IGNORE)
      return false;
    else if (ret == IUP_CONTINUE)
      iupStrPrintfDoubleLocale(inBuf, inFormatString, inValue, decimal_symbol);
  }
  else
    iupStrPrintfDoubleLocale(inBuf, inFormatString, inValue, decimal_symbol);

  return true;
}

/************************************************************************************************/

double iupPlotTrafoLinear::Transform(double inValue) const
{
  return inValue * mSlope + mOffset;
}

double iupPlotTrafoLinear::TransformBack(double inValue) const
{
  if (mSlope != 0)
    return (inValue - mOffset) / mSlope;
  else
    return 0;
}

bool iupPlotTrafoLinear::Calculate(int inBegin, int inEnd, const iupPlotAxis& inAxis)
{
  double theDataRange = inAxis.mMax - inAxis.mMin;
  if (theDataRange < kFloatSmall)
    return false;

  double theMin = inAxis.mMin;
  if (inAxis.mDiscrete)
  {
    theDataRange++;
    theMin -= 0.5;
  }

  double theTargetRange = inEnd - inBegin;
  double theScale = theTargetRange / theDataRange;

  if (!inAxis.mReverse)
    mOffset = inBegin - theMin * theScale;
  else
    mOffset = inEnd + theMin * theScale;

  mSlope = theScale;
  if (inAxis.mReverse)
    mSlope *= -1;

  return true;
}

/************************************************************************************************/

double iupPlotTrafoLog::Transform(double inValue) const
{
  if (inValue < kLogMinClipValue) inValue = kLogMinClipValue;
  return iupPlotLog(inValue, mBase)*mSlope + mOffset;
}

double iupPlotTrafoLog::TransformBack(double inValue) const
{
  if (mSlope != 0)
    return iupPlotExp((inValue - mOffset) / mSlope, mBase);
  else
    return 0;
}

bool iupPlotTrafoLog::Calculate(int inBegin, int inEnd, const iupPlotAxis& inAxis)
{
  double theBase = inAxis.mLogBase;
  double theDataRange = iupPlotLog(inAxis.mMax, theBase) - iupPlotLog(inAxis.mMin, theBase);
  if (theDataRange < kFloatSmall)
    return false;

  double theTargetRange = inEnd - inBegin;
  double theScale = theTargetRange / theDataRange;

  if (!inAxis.mReverse)
    mOffset = inBegin - iupPlotLog(inAxis.mMin, theBase) * theScale;
  else
    mOffset = inEnd + iupPlotLog(inAxis.mMin, theBase) * theScale;

  mBase = theBase;

  mSlope = theScale;
  if (inAxis.mReverse)
    mSlope *= -1;

  return true;
}

/************************************************************************************************/

void iupPlotAxis::Init()
{
  if (mLogScale)
  {
    mTickIter = &mLogTickIter;
    mTrafo = &mLogTrafo;
  }
  else
  {
    mTickIter = &mLinTickIter;
    mTrafo = &mLinTrafo;
  }

  mTickIter->SetAxis(this);

  if (mPosition == IUP_PLOT_START)
    mReverseTicksLabel = false;
  if (mPosition == IUP_PLOT_END)
    mReverseTicksLabel = true;
}

void iupPlotAxis::GetTickNumberSize(Ihandle* ih, int *outWitdh, int *outHeight) const
{
  int theTickFontWidth, theTickFontHeight;
  SetFont(ih, mTick.mFontStyle, mTick.mFontSize);
  iupDrawGetTextSize(ih, "1234567890.", 11, &theTickFontWidth, &theTickFontHeight, 0);
  theTickFontWidth /= 11;
  if (outHeight) *outHeight = theTickFontHeight;
  if (outWitdh)  *outWitdh  = theTickFontWidth * iPlotEstimateNumberCharCount(mTick.mFormatAuto, mTick.mFormatString, mHasZoom? mNoZoomMin: mMin, mHasZoom? mNoZoomMax: mMax);
}

void iupPlotAxis::SetNamedTickIter(const iupPlotDataString *inStringData)
{
  mTickIter = &mNamedTickIter;
  mTickIter->SetAxis(this);
  mNamedTickIter.SetStringList(inStringData);
}

void iupPlotAxis::CheckZoomOutLimit(double inRange)
{
  if (mMin < mNoZoomMin)
  {
    mMin = mNoZoomMin;
    mMax = mMin + inRange;
    if (mMax > mNoZoomMax)
      mMax = mNoZoomMax;
  }

  if (mMax > mNoZoomMax)
  {
    mMax = mNoZoomMax;
    mMin = mMax - inRange;
    if (mMin < mNoZoomMin)
      mMin = mNoZoomMin;
  }
}

void iupPlotAxis::InitZoom()
{
  if (!mHasZoom)
  {
    mNoZoomMin = mMin;
    mNoZoomMax = mMax;
    mNoZoomAutoScaleMin = mAutoScaleMin;
    mNoZoomAutoScaleMax = mAutoScaleMax;

    mHasZoom = true;

    mAutoScaleMin = false;
    mAutoScaleMax = false;
  }
}

bool iupPlotAxis::ResetZoom()
{
  if (mHasZoom)
  {
    mHasZoom = false;

    mMin = mNoZoomMin;
    mMax = mNoZoomMax;
    mAutoScaleMin = mNoZoomAutoScaleMin;
    mAutoScaleMax = mNoZoomAutoScaleMax;
    return true;
  }

  return false;
}

bool iupPlotAxis::ZoomOut(double inCenter)
{
  if (inCenter < mMin || inCenter > mMax)
    return false;

  if (!mHasZoom)
    return false;

  double theRange = mMax - mMin;
  double theNewRange = theRange * 1.1; // 10%
  double theFactor = (inCenter - mMin) / theRange;
  double theOffset = (theNewRange - theRange);

  mMin -= theOffset*theFactor;
  mMax += theOffset*(1.0 - theFactor);

  CheckZoomOutLimit(theNewRange);

  if (mMin == mNoZoomMin && mMax == mNoZoomMax)
    ResetZoom();

  return true;
}

bool iupPlotAxis::ZoomIn(double inCenter)
{
  if (inCenter < mMin || inCenter > mMax)
    return false;

  InitZoom();

  double theRange = mMax - mMin;
  double theNewRange = theRange * 0.9; // 10%
  double theFactor = (inCenter - mMin) / theRange;
  double theOffset = (theRange - theNewRange);

  mMin += theOffset*theFactor;
  mMax -= theOffset*(1.0 - theFactor);

  return true;
}

bool iupPlotAxis::ZoomTo(double inMin, double inMax)
{
  InitZoom();

  iPlotCheckMinMax(inMin, inMax);

  if (inMin > mNoZoomMax || inMax < mNoZoomMin)
    return false;

  if (inMin < mNoZoomMin) inMin = mNoZoomMin;
  if (inMax > mNoZoomMax) inMax = mNoZoomMax;

  mMin = inMin;
  mMax = inMax;

  if (mMin == mNoZoomMin && mMax == mNoZoomMax)
    ResetZoom();

  return true;
}

bool iupPlotAxis::Pan(double inOffset)
{
  if (!mHasZoom)
    return false;

  double theRange = mMax - mMin;

  mMin = mPanMin - inOffset;
  mMax = mMin + theRange;

  CheckZoomOutLimit(theRange);
  return true;
}

bool iupPlotAxis::Scroll(double inDelta, bool inFullPage)
{
  if (!mHasZoom)
    return false;

  double theRange = mMax - mMin;

  double thePage;
  if (inFullPage)
    thePage = theRange;
  else
    thePage = theRange / 10.0;

  double theOffset = thePage * inDelta;

  mMin += theOffset;
  mMax += theOffset;

  CheckZoomOutLimit(theRange);
  return true;
}

bool iupPlotAxis::ScrollTo(double inMin)
{
  if (inMin < mNoZoomMin || inMin > mNoZoomMax)
    return false;

  if (!mHasZoom)
    return false;

  double theRange = mMax - mMin;

  mMin = inMin;
  mMax = mMin + theRange;

  CheckZoomOutLimit(theRange);
  return true;
}

/************************************************************************************************/

static void iPlotFillArrowI(iupPlotDrawContext* ctx, int inX1, int inY1, int inX2, int inY2, int inX3, int inY3, long color)
{
  int points[6];
  points[0] = inX1;
  points[1] = inY1;
  points[2] = inX2;
  points[3] = inY2;
  points[4] = inX3;
  points[5] = inY3;
  iupPlotDrawPolygon(ctx->ih, points, 3, color, IUP_DRAW_FILL, 1);
}

static void iPlotDrawArrow(iupPlotDrawContext* ctx, double inX, double inY, int inVertical, int inDirection, int inSize, long color, int lineWidth)
{
  int theSizeDir = iupPlotRound(inSize * 0.7);

  if (inVertical)
  {
    int theAbsX = iupPlotDrawCalcX(ctx, inX);
    int theAbsY1 = iupPlotDrawCalcY(ctx, inY);
    int theAbsY2 = iupPlotDrawCalcY(ctx, inY + inDirection*inSize);
    iupPlotDrawLine(ctx->ih, theAbsX, theAbsY1, theAbsX, theAbsY2, color, IUP_DRAW_STROKE, lineWidth);

    int theArrowTipY = theAbsY2;
    int theArrowBaseY = iupPlotDrawCalcY(ctx, inY + inDirection*inSize - inDirection*theSizeDir);
    iPlotFillArrowI(ctx, theAbsX, theArrowTipY,
                         theAbsX - theSizeDir, theArrowBaseY,
                         theAbsX + theSizeDir, theArrowBaseY, color);
  }
  else
  {
    int theAbsY = iupPlotDrawCalcY(ctx, inY);
    int theAbsX1 = iupPlotDrawCalcX(ctx, inX);
    int theAbsX2 = iupPlotDrawCalcX(ctx, inX + inDirection*inSize);
    iupPlotDrawLine(ctx->ih, theAbsX1, theAbsY, theAbsX2, theAbsY, color, IUP_DRAW_STROKE, lineWidth);

    int theArrowTipX = theAbsX2;
    int theArrowBaseX = iupPlotDrawCalcX(ctx, inX + inDirection*inSize - inDirection*theSizeDir);
    iPlotFillArrowI(ctx, theArrowTipX, theAbsY,
                         theArrowBaseX, theAbsY - theSizeDir,
                         theArrowBaseX, theAbsY + theSizeDir, color);
  }
}

void iupPlotAxis::SetFont(Ihandle* ih, int inFontStyle, int inFontSize) const
{
  if (inFontStyle == -1) inFontStyle = mDefaultFontStyle;
  if (inFontSize == 0) inFontSize = mDefaultFontSize;
  char fontStr[256];
  iupPlotBuildFont(ih, inFontStyle, inFontSize, fontStr, sizeof(fontStr));
  IupSetStrAttribute(ih, "DRAWFONT", fontStr);
}

int iupPlotAxis::GetTickNumberHeight(Ihandle* ih) const
{
  int height;
  if (mTick.mRotateNumber)
  {
    int theXTickNumberWidth;
    GetTickNumberSize(ih, &theXTickNumberWidth, NULL);
    height = theXTickNumberWidth;
  }
  else
  {
    int theXTickNumberHeight;
    GetTickNumberSize(ih, NULL, &theXTickNumberHeight);
    height = theXTickNumberHeight;
  }
  return height;
}

int iupPlotAxis::GetTickNumberWidth(Ihandle* ih) const
{
  int width;
  if (mTick.mRotateNumber)
  {
    int theYTickNumberHeight;
    GetTickNumberSize(ih, NULL, &theYTickNumberHeight);
    width = theYTickNumberHeight;
  }
  else
  {
    int theYTickNumberWidth;
    GetTickNumberSize(ih, &theYTickNumberWidth, NULL);
    width = theYTickNumberWidth;
  }
  return width;
}

int iupPlotAxis::GetArrowSize() const
{
  return mTick.mMinorSize + 2;  // to avoid too small sizes
}

/*****************************************************************************/

double iupPlotAxisX::GetScreenYOriginX(const iupPlotAxis& inAxisY) const
{
  double theTargetY = 0;
  if (mPosition != IUP_PLOT_CROSSORIGIN)
  {
    if (mPosition == IUP_PLOT_START)
    {
      if (inAxisY.mReverse)
        theTargetY = inAxisY.mMax;
      else
        theTargetY = inAxisY.mMin;
    }
    else
    {
      if (inAxisY.mReverse)
        theTargetY = inAxisY.mMin;
      else
        theTargetY = inAxisY.mMax;
    }
  }
  if (inAxisY.mDiscrete)
    theTargetY -= 0.5;

  return inAxisY.mTrafo->Transform(theTargetY);
}

bool iupPlotAxisX::DrawX(const iupPlotRect &inRect, iupPlotDrawContext* ctx, const iupPlotAxis& inAxisY) const
{
  if (!mShow)
    return true;

  double theScreenY = GetScreenYOriginX(inAxisY);
  double theScreenX1 = inRect.mX;
  double theScreenX2 = theScreenX1 + inRect.mWidth;

  int absX1 = iupPlotDrawCalcX(ctx, theScreenX1);
  int absX2 = iupPlotDrawCalcX(ctx, theScreenX2);
  int absY = iupPlotDrawCalcY(ctx, theScreenY);
  iupPlotDrawLine(ctx->ih, absX1, absY, absX2, absY, mColor, IUP_DRAW_STROKE, mLineWidth);

  if (mShowArrow)
  {
    if (!mReverse)
      iPlotDrawArrow(ctx, theScreenX2, theScreenY, 0, 1, GetArrowSize(), mColor, mLineWidth);
    else
      iPlotDrawArrow(ctx, theScreenX1, theScreenY, 0, -1, GetArrowSize(), mColor, mLineWidth);
  }

  if (mTick.mShow)
  {
    if (!mTickIter->Init())
      return false;

    double theX;
    bool theIsMajorTick;
    char theFormatString[30];
    iupStrCopyN(theFormatString, sizeof(theFormatString), mTick.mFormatString);

    IFnssds formatticknumber_cb = (IFnssds)IupGetCallback(ctx->ih, "XTICKFORMATNUMBER_CB");

    if (mTick.mShowNumber)
      SetFont(ctx->ih, mTick.mFontStyle, mTick.mFontSize);

    while (mTickIter->GetNextTick(theX, theIsMajorTick, theFormatString))
      DrawXTick(theX, theScreenY, theIsMajorTick, theFormatString, ctx, formatticknumber_cb);

    int theTickSpace = mTick.mMajorSize;
    if (mTick.mShowNumber)
      theTickSpace += GetTickNumberHeight(ctx->ih) + mTick.mMinorSize;

    if (mReverseTicksLabel)
      theScreenY += theTickSpace;
    else
      theScreenY -= theTickSpace;
  }

  if (GetLabel())
  {
    SetFont(ctx->ih, mFontStyle, mFontSize);

    int theLabelSpacing = mLabelSpacing;
    if (mLabelSpacing == -1)
    {
      int theXFontHeight;
      iupdrvFontGetFontDim(IupGetAttribute(ctx->ih, "DRAWFONT"), NULL, &theXFontHeight, NULL, NULL);
      theLabelSpacing = theXFontHeight / 10;
    }

    if (mReverseTicksLabel)
      theScreenY += theLabelSpacing;
    else
      theScreenY -= theLabelSpacing;

    char fontStr[256];
    int fontStyle = mFontStyle == -1 ? mDefaultFontStyle : mFontStyle;
    int fontSize = mFontSize == 0 ? mDefaultFontSize : mFontSize;
    iupPlotBuildFont(ctx->ih, fontStyle, fontSize, fontStr, sizeof(fontStr));

    if (mLabelCentered)
    {
      double theScreenX = theScreenX1 + inRect.mWidth / 2;
      iupPlotDrawAlignedText(ctx, theScreenX, theScreenY, mReverseTicksLabel ? IUP_PLOT_SOUTH : IUP_PLOT_NORTH, GetLabel(), mColor, fontStr, 0);
    }
    else
    {
      double theScreenX = theScreenX2;
      iupPlotDrawAlignedText(ctx, theScreenX, theScreenY, mReverseTicksLabel ? IUP_PLOT_SOUTH_EAST : IUP_PLOT_NORTH_EAST, GetLabel(), mColor, fontStr, 0);
    }
  }

  return true;
}

void iupPlotAxisX::DrawXTick(double inX, double inScreenY, bool inMajor, const char*inFormatString, iupPlotDrawContext* ctx, IFnssds formatticknumber_cb) const
{
  int theTickSize;
  double theScreenX = mTrafo->Transform(inX);
  if (inMajor)
  {
    theTickSize = mTick.mMajorSize;

    if (mTick.mShowNumber)
    {
      char theBuf[128];
      if (iPlotGetTickFormat(ctx->ih, formatticknumber_cb, theBuf, inFormatString, inX))
      {
        double theScreenY;
        if (mReverseTicksLabel)
          theScreenY = inScreenY + theTickSize + mTick.mMinorSize;  // Use minor size as spacing
        else
          theScreenY = inScreenY - theTickSize - mTick.mMinorSize;  // Use minor size as spacing

        char fontStr[256];
        int fontStyle = mTick.mFontStyle == -1 ? mDefaultFontStyle : mTick.mFontStyle;
        int fontSize = mTick.mFontSize == 0 ? mDefaultFontSize : mTick.mFontSize;
        iupPlotBuildFont(ctx->ih, fontStyle, fontSize, fontStr, sizeof(fontStr));

        if (mTick.mRotateNumber)
          iupPlotDrawAlignedText(ctx, theScreenX, theScreenY, mReverseTicksLabel ? IUP_PLOT_WEST : IUP_PLOT_EAST, theBuf, mColor, fontStr, mTick.mRotateNumberAngle);
        else
          iupPlotDrawAlignedText(ctx, theScreenX, theScreenY, mReverseTicksLabel ? IUP_PLOT_SOUTH : IUP_PLOT_NORTH, theBuf, mColor, fontStr, 0);
      }
    }
  }
  else
    theTickSize = mTick.mMinorSize;

  int absX = iupPlotDrawCalcX(ctx, theScreenX);
  int absY1 = iupPlotDrawCalcY(ctx, inScreenY);
  int absY2;
  if (mReverseTicksLabel)
    absY2 = iupPlotDrawCalcY(ctx, inScreenY + theTickSize);
  else
    absY2 = iupPlotDrawCalcY(ctx, inScreenY - theTickSize);

  iupPlotDrawLine(ctx->ih, absX, absY1, absX, absY2, mColor, IUP_DRAW_STROKE, mLineWidth);
}

/*****************************************************************************/

double iupPlotAxisY::GetScreenXOriginY(const iupPlotAxis& inAxisX) const
{
  double theTargetX = 0;
  if (mPosition != IUP_PLOT_CROSSORIGIN)
  {
    if (mPosition == IUP_PLOT_START)
    {
      if (inAxisX.mReverse)
        theTargetX = inAxisX.mMax;
      else
        theTargetX = inAxisX.mMin;
    }
    else
    {
      if (inAxisX.mReverse)
        theTargetX = inAxisX.mMin;
      else
        theTargetX = inAxisX.mMax;
    }
  }
  if (inAxisX.mDiscrete)
    theTargetX -= 0.5;

  return inAxisX.mTrafo->Transform(theTargetX);
}

bool iupPlotAxisY::DrawY(const iupPlotRect &inRect, iupPlotDrawContext* ctx, const iupPlotAxis& inAxisX) const
{
  if (!mShow)
    return true;

  int drawStyle = IUP_DRAW_STROKE;

  double theScreenX = GetScreenXOriginY(inAxisX);
  double theScreenY1 = inRect.mY;
  double theScreenY2 = theScreenY1 + inRect.mHeight;

  int absX = iupPlotDrawCalcX(ctx, theScreenX);
  int absY1 = iupPlotDrawCalcY(ctx, theScreenY1);
  int absY2 = iupPlotDrawCalcY(ctx, theScreenY2);
  iupPlotDrawLine(ctx->ih, absX, absY1, absX, absY2, mColor, drawStyle, mLineWidth);

  if (mShowArrow)
  {
    if (!mReverse)
      iPlotDrawArrow(ctx, theScreenX, theScreenY2, 1, 1, GetArrowSize(), mColor, mLineWidth);
    else
      iPlotDrawArrow(ctx, theScreenX, theScreenY1, 1, -1, GetArrowSize(), mColor, mLineWidth);
  }

  if (mTick.mShow)
  {
    if (!mTickIter->Init())
      return false;

    double theY;
    bool theIsMajorTick;
    char theFormatString[30];
    iupStrCopyN(theFormatString, sizeof(theFormatString), mTick.mFormatString);

    IFnssds formatticknumber_cb = (IFnssds)IupGetCallback(ctx->ih, "YTICKFORMATNUMBER_CB");

    if (mTick.mShowNumber)
      SetFont(ctx->ih, mTick.mFontStyle, mTick.mFontSize);

    while (mTickIter->GetNextTick(theY, theIsMajorTick, theFormatString))
      DrawYTick(theY, theScreenX, theIsMajorTick, theFormatString, ctx, formatticknumber_cb);

    int theTickSpace = mTick.mMajorSize;  // skip major tick
    if (mTick.mShowNumber)
      theTickSpace += GetTickNumberWidth(ctx->ih) + mTick.mMinorSize;  // Use minor size as spacing

    if (mReverseTicksLabel)
      theScreenX += theTickSpace;
    else
      theScreenX -= theTickSpace;
  }

  if (GetLabel())
  {
    SetFont(ctx->ih, mFontStyle, mFontSize);

    int theLabelSpacing = mLabelSpacing;
    if (mLabelSpacing == -1)
    {
      int theYFontHeight;
      iupdrvFontGetFontDim(IupGetAttribute(ctx->ih, "DRAWFONT"), NULL, &theYFontHeight, NULL, NULL);
      theLabelSpacing = theYFontHeight / 10;  // default spacing
    }

    if (mReverseTicksLabel)
      theScreenX += theLabelSpacing;
    else
      theScreenX -= theLabelSpacing;

    char fontStr[256];
    int fontStyle = mFontStyle == -1 ? mDefaultFontStyle : mFontStyle;
    int fontSize = mFontSize == 0 ? mDefaultFontSize : mFontSize;
    iupPlotBuildFont(ctx->ih, fontStyle, fontSize, fontStr, sizeof(fontStr));

    if (mLabelCentered)
    {
      double theScreenY = theScreenY1 + inRect.mHeight / 2;
      iupPlotDrawAlignedText(ctx, theScreenX, theScreenY, mReverseTicksLabel ? IUP_PLOT_NORTH : IUP_PLOT_SOUTH, GetLabel(), mColor, fontStr, 90);
    }
    else
    {
      double theScreenY = theScreenY2;
      iupPlotDrawAlignedText(ctx, theScreenX, theScreenY, mReverseTicksLabel ? IUP_PLOT_NORTH_EAST : IUP_PLOT_SOUTH_EAST, GetLabel(), mColor, fontStr, 90);
    }
  }

  return true;
}

void iupPlotAxisY::DrawYTick(double inY, double inScreenX, bool inMajor, const char* inFormatString, iupPlotDrawContext* ctx, IFnssds formatticknumber_cb) const
{
  int theTickSize;
  double theScreenY = mTrafo->Transform(inY);
  if (inMajor)
  {
    theTickSize = mTick.mMajorSize;

    if (mTick.mShowNumber)
    {
      char theBuf[128];
      if (iPlotGetTickFormat(ctx->ih, formatticknumber_cb, theBuf, inFormatString, inY))
      {
        double theScreenX;
        if (mReverseTicksLabel)
          theScreenX = inScreenX + theTickSize + mTick.mMinorSize;  // Use minor size as spacing
        else
          theScreenX = inScreenX - theTickSize - mTick.mMinorSize;  // Use minor size as spacing

        char fontStr[256];
        int fontStyle = mTick.mFontStyle == -1 ? mDefaultFontStyle : mTick.mFontStyle;
        int fontSize = mTick.mFontSize == 0 ? mDefaultFontSize : mTick.mFontSize;
        iupPlotBuildFont(ctx->ih, fontStyle, fontSize, fontStr, sizeof(fontStr));

        if (mTick.mRotateNumber)
          iupPlotDrawAlignedText(ctx, theScreenX, theScreenY, mReverseTicksLabel ? IUP_PLOT_NORTH : IUP_PLOT_SOUTH, theBuf, mColor, fontStr, mTick.mRotateNumberAngle);
        else
          iupPlotDrawAlignedText(ctx, theScreenX, theScreenY, mReverseTicksLabel ? IUP_PLOT_WEST : IUP_PLOT_EAST, theBuf, mColor, fontStr, 0);
      }
    }
  }
  else
    theTickSize = mTick.mMinorSize;

  int absY = iupPlotDrawCalcY(ctx, theScreenY);
  int absX1 = iupPlotDrawCalcX(ctx, inScreenX);
  int absX2;
  if (mReverseTicksLabel)
    absX2 = iupPlotDrawCalcX(ctx, inScreenX + theTickSize);
  else
    absX2 = iupPlotDrawCalcX(ctx, inScreenX - theTickSize);

  iupPlotDrawLine(ctx->ih, absX1, absY, absX2, absY, mColor, IUP_DRAW_STROKE, mLineWidth);
}
