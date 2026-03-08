/* Single plot class.
   Multiple plots can be used in the same IupPlot element.

   This is a re-write of the PPlot library,
   directly using IupDraw for drawing and IUP callback for interaction.
   */

#include "iup.h"
#include "iupcbs.h"

#include "iup_array.h"
#include "iup_str.h"

#include "iup_drvdraw.h"
#include "iup_draw.h"

#ifndef __IUPPLOT_H__
#define __IUPPLOT_H__


inline void iPlotCheckMinMax(double &inoutMin, double &inoutMax)
{
  if (inoutMin > inoutMax)
  {
    double theTmp = inoutMin;
    inoutMin = inoutMax;
    inoutMax = theTmp;
  }
}

enum iupPlotMode { IUP_PLOT_LINE, IUP_PLOT_MARK, IUP_PLOT_MARKLINE, IUP_PLOT_AREA, IUP_PLOT_BAR, IUP_PLOT_STEM, IUP_PLOT_MARKSTEM, IUP_PLOT_HORIZONTALBAR, IUP_PLOT_MULTIBAR, IUP_PLOT_STEP, IUP_PLOT_ERRORBAR, IUP_PLOT_PIE };
enum iupPlotLegendPosition { IUP_PLOT_TOPRIGHT, IUP_PLOT_TOPLEFT, IUP_PLOT_BOTTOMRIGHT, IUP_PLOT_BOTTOMLEFT, IUP_PLOT_BOTTOMCENTER, IUP_PLOT_XY };
enum iupPlotSliceLabel { IUP_PLOT_NONE, IUP_PLOT_X, IUP_PLOT_Y, IUP_PLOT_PERCENT };
enum iupPlotHighlight { IUP_PLOT_HIGHLIGHT_NONE, IUP_PLOT_HIGHLIGHT_SAMPLE, IUP_PLOT_HIGHLIGHT_CURVE, IUP_PLOT_HIGHLIGHT_BOTH };
enum iupPlotClipping { IUP_PLOT_CLIPNONE, IUP_PLOT_CLIPAREA, IUP_PLOT_CLIPAREAOFFSET };
enum iupPlotAxisPosition { IUP_PLOT_START, IUP_PLOT_CROSSORIGIN, IUP_PLOT_END };

enum iupPlotLineStyle {
  IUP_PLOT_LINE_CONTINUOUS,
  IUP_PLOT_LINE_DASHED,
  IUP_PLOT_LINE_DOTTED,
  IUP_PLOT_LINE_DASH_DOT,
  IUP_PLOT_LINE_DASH_DOT_DOT
};

enum iupPlotMarkStyle {
  IUP_PLOT_MARK_PLUS,
  IUP_PLOT_MARK_STAR,
  IUP_PLOT_MARK_CIRCLE,
  IUP_PLOT_MARK_X,
  IUP_PLOT_MARK_BOX,
  IUP_PLOT_MARK_DIAMOND,
  IUP_PLOT_MARK_HOLLOW_CIRCLE,
  IUP_PLOT_MARK_HOLLOW_BOX,
  IUP_PLOT_MARK_HOLLOW_DIAMOND
};

enum iupPlotFontStyle {
  IUP_PLOT_FONT_PLAIN = 0,
  IUP_PLOT_FONT_BOLD = 1,
  IUP_PLOT_FONT_ITALIC = 2,
  IUP_PLOT_FONT_BOLD_ITALIC = 3
};

enum iupPlotTextAlignment {
  IUP_PLOT_NORTH,
  IUP_PLOT_SOUTH,
  IUP_PLOT_EAST,
  IUP_PLOT_WEST,
  IUP_PLOT_NORTH_EAST,
  IUP_PLOT_NORTH_WEST,
  IUP_PLOT_SOUTH_EAST,
  IUP_PLOT_SOUTH_WEST,
  IUP_PLOT_CENTER
};

#define IUP_PLOT_DEF_NUMBERFORMAT "%.0f"
#define IUP_PLOT_DEF_NUMBERFORMATSIGNED "% .0f"
#define IUP_PLOT_DEF_TIPFORMAT "%.2f"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define IUP_PLOT_DEG2RAD  0.01745329252

const double kFloatSmall = 1e-20;
const double kLogMinClipValue = 1e-10;

long iupPlotDrawGetSampleColorTable(Ihandle* ih, int index);

int iupPlotCalcPrecision(double inValue);

inline int iupPlotRound(double inFloat)
{
  return (int)(inFloat > 0 ? inFloat + 0.5 : inFloat - 0.5);
}

inline double iupPlotLog(double inFloat, double inBase)
{
  const double kLogMin = 1e-10;
  if (inFloat<kLogMin) inFloat = kLogMin;
  return log10(inFloat) / log10(inBase);
}

inline int iupPlotMax(int a, int b)
{
  return a>b ? a: b;
}

inline double iupPlotExp(double inFloat, double inBase)
{
  const double kExpMax = 1e10;
  if (inFloat>kExpMax) inFloat = kExpMax;
  return pow(inBase, inFloat);
}


/************************************************************************************/
/* Drawing Context                                                                   */
/************************************************************************************/

struct iupPlotDrawContext {
  IdrawCanvas* dc;
  Ihandle* ih;
  int viewportX, viewportY, viewportW, viewportH;
  char font[256];
  int defaultFontStyle;
  int defaultFontSize;
};

inline int iupPlotLineStyleToDrawStyle(int plotStyle)
{
  switch (plotStyle) {
    case IUP_PLOT_LINE_CONTINUOUS:    return IUP_DRAW_STROKE;
    case IUP_PLOT_LINE_DASHED:       return IUP_DRAW_STROKE_DASH;
    case IUP_PLOT_LINE_DOTTED:       return IUP_DRAW_STROKE_DOT;
    case IUP_PLOT_LINE_DASH_DOT:     return IUP_DRAW_STROKE_DASH_DOT;
    case IUP_PLOT_LINE_DASH_DOT_DOT: return IUP_DRAW_STROKE_DASH_DOT_DOT;
    default:                         return IUP_DRAW_STROKE;
  }
}

void iupPlotBuildFont(Ihandle* ih, int fontStyle, int fontSize, char* fontStr, int fontStrSize);
void iupPlotDrawMark(iupPlotDrawContext* ctx, int x, int y, int markStyle, int markSize, long color);
void iupPlotDrawAlignedText(iupPlotDrawContext* ctx, double refX, double refY,
                            int plotAlignment, const char* text, long color,
                            const char* font, double orientation);

inline int iupPlotDrawCalcX(const iupPlotDrawContext* ctx, double x)
{
  return iupPlotRound(x) + ctx->viewportX;
}

inline int iupPlotDrawCalcY(const iupPlotDrawContext* ctx, double y)
{
  return (ctx->viewportH - 1 - iupPlotRound(y)) + ctx->viewportY;
}

inline int iupPlotDrawCalcAbsX(double x, int viewportX)
{
  return iupPlotRound(x) + viewportX;
}

inline int iupPlotDrawCalcAbsY(double y, int viewportY, int viewportH)
{
  return (viewportH - 1 - iupPlotRound(y)) + viewportY;
}


/************************************************************************************/
/* Point Buffer for polylines/polygons                                               */
/************************************************************************************/

class iupPlotPointBuffer {
public:
  iupPlotPointBuffer() : mPoints(NULL), mCount(0), mCapacity(0) {}
  ~iupPlotPointBuffer() { if (mPoints) free(mPoints); }

  void Begin(int estimatedCount) {
    mCount = 0;
    if (estimatedCount > mCapacity) {
      mCapacity = estimatedCount + 64;
      mPoints = (int*)realloc(mPoints, mCapacity * 2 * sizeof(int));
    }
  }

  void AddVertex(int x, int y) {
    if (mCount * 2 + 1 >= mCapacity * 2) {
      mCapacity = mCapacity * 2 + 64;
      mPoints = (int*)realloc(mPoints, mCapacity * 2 * sizeof(int));
    }
    mPoints[mCount * 2] = x;
    mPoints[mCount * 2 + 1] = y;
    mCount++;
  }

  void DrawStroke(iupPlotDrawContext* ctx, long color, int lineStyle, int lineWidth) {
    if (mCount < 2) return;
    int drawStyle = iupPlotLineStyleToDrawStyle(lineStyle);
    for (int i = 0; i < mCount - 1; i++)
      iupdrvDrawLine(ctx->dc, mPoints[i*2], mPoints[i*2+1], mPoints[(i+1)*2], mPoints[(i+1)*2+1], color, drawStyle, lineWidth);
  }

  void DrawFill(iupPlotDrawContext* ctx, long color) {
    if (mCount < 3) return;
    iupdrvDrawPolygon(ctx->dc, mPoints, mCount, color, IUP_DRAW_FILL, 1);
  }

  void Clear() { mCount = 0; }

  int GetCount() const { return mCount; }
  int* GetPoints() const { return mPoints; }

private:
  int* mPoints;
  int mCount;
  int mCapacity;
};


/************************************************************************************/


class iupPlotRect
{
public:
  iupPlotRect() :mX(0), mY(0), mWidth(0), mHeight(0) {}
  iupPlotRect(int inX, int inY, int inWidth, int inHeight)
    : mX(inX), mY(inY), mWidth(inWidth), mHeight(inHeight) {}

  int mX;
  int mY;
  int mWidth;
  int mHeight;
};

class iupPlotMargin
{
public:
  iupPlotMargin() :mLeft(0), mRight(0), mTop(0), mBottom(0) {}
  iupPlotMargin(int inLeft, int inRight, int inTop, int inBottom)
    : mLeft(inLeft), mRight(inRight), mTop(inTop), mBottom(inBottom) {}

  int mLeft;
  int mRight;
  int mTop;
  int mBottom;
};

class iupPlotAxis;

class iupPlotTrafo
{
public:
  virtual ~iupPlotTrafo() {}
  virtual double Transform(double inValue) const = 0;
  virtual double TransformBack(double inValue) const = 0;
  virtual bool Calculate(int inBegin, int inEnd, const iupPlotAxis& inAxis) = 0;
};

class iupPlotTrafoLinear : public iupPlotTrafo
{
public:
  iupPlotTrafoLinear() :mOffset(0), mSlope(0) {}
  double Transform(double inValue) const;
  double TransformBack(double inValue) const;

  bool Calculate(int inBegin, int inEnd, const iupPlotAxis& inAxis);

  double mOffset;
  double mSlope;
};

class iupPlotTrafoLog : public iupPlotTrafo
{
public:
  iupPlotTrafoLog() :mOffset(0), mSlope(0), mBase(10) {}
  double Transform(double inValue) const;
  double TransformBack(double inValue) const;

  bool Calculate(int inBegin, int inEnd, const iupPlotAxis& inAxis);

  double mOffset;
  double mSlope;
  double mBase;
};

class iupPlotData
{
public:
  iupPlotData(int inSize) : mCount(0), mIsString(false) { mArray = iupArrayCreate(20, inSize); }
  virtual ~iupPlotData() { iupArrayDestroy(mArray); }

  bool IsString() const { return mIsString; }
  int GetCount() const { return mCount; }

  virtual bool CalculateRange(double &outMin, double &outMax) const = 0;
  virtual double GetSample(int inSampleIndex) const = 0;

  void RemoveSample(int inSampleIndex) {
    if (inSampleIndex < 0) inSampleIndex = 0; if (inSampleIndex > mCount) inSampleIndex = mCount;
    iupArrayRemove(mArray, inSampleIndex, 1); mCount--;
  }

protected:
  int mCount;
  Iarray* mArray;
  bool mIsString;
};

class iupPlotDataReal : public iupPlotData
{
public:
  iupPlotDataReal() :iupPlotData(sizeof(double)) { mData = (double*)iupArrayGetData(mArray); }

  double GetSample(int inSampleIndex) const { return mData[inSampleIndex]; }
  void SetSample(int inSampleIndex, double inReal) const { mData[inSampleIndex] = inReal; }

  void AddSample(double inReal) { mData = (double*)iupArrayInc(mArray); mData[mCount] = inReal; mCount++; }
  void InsertSample(int inSampleIndex, double inReal) {
    if (inSampleIndex < 0) inSampleIndex = 0; if (inSampleIndex > mCount) inSampleIndex = mCount;
    mData = (double*)iupArrayInsert(mArray, inSampleIndex, 1); mData[inSampleIndex] = inReal; mCount++;
  }

  bool CalculateRange(double &outMin, double &outMax) const;

protected:
  double* mData;
};

class iupPlotDataString : public iupPlotData
{
public:
  iupPlotDataString() :iupPlotData(sizeof(char*)) { mIsString = true; mData = (char**)iupArrayGetData(mArray); }
  ~iupPlotDataString();

  double GetSample(int inSampleIndex) const { return inSampleIndex; }

  const char* GetSampleString(int inSampleIndex) const { return mData[inSampleIndex]; }
  void SetSampleString(int inSampleIndex, const char *inString) const {
    if (inString == mData[inSampleIndex]) return;
    free(mData[inSampleIndex]);
    mData[inSampleIndex] = iupStrDup(inString);
  }

  void AddSample(const char *inString) { mData = (char**)iupArrayInc(mArray); mData[mCount] = iupStrDup(inString); mCount++; }
  void InsertSample(int inSampleIndex, const char *inString) {
    if (inSampleIndex < 0) inSampleIndex = 0; if (inSampleIndex > mCount) inSampleIndex = mCount;
    mData = (char**)iupArrayInsert(mArray, inSampleIndex, 1); mData[inSampleIndex] = iupStrDup(inString); mCount++;
  }

  bool CalculateRange(double &outMin, double &outMax) const;

protected:
  char** mData;
};

class iupPlotDataBool : public iupPlotData
{
public:
  iupPlotDataBool() :iupPlotData(sizeof(bool)) { mData = (bool*)iupArrayGetData(mArray); }

  double GetSample(int inSampleIndex) const { return (int)mData[inSampleIndex]; }

  bool GetSampleBool(int inSampleIndex) const { return mData[inSampleIndex]; }
  void SetSampleBool(int inSampleIndex, bool inBool) { mData[inSampleIndex] = inBool; }

  void AddSample(bool inBool) { mData = (bool*)iupArrayInc(mArray); mData[mCount] = inBool; mCount++; }
  void InsertSample(int inSampleIndex, bool inBool) {
    if (inSampleIndex < 0) inSampleIndex = 0; if (inSampleIndex > mCount) inSampleIndex = mCount;
    mData = (bool*)iupArrayInsert(mArray, inSampleIndex, 1); mData[inSampleIndex] = inBool; mCount++;
  }

  bool CalculateRange(double &outMin, double &outMax) const;

protected:
  bool* mData;
};

struct iupPlotSampleNotify
{
  Ihandle* ih;
  int ds;
  IFniiddi cb;
};

class iupPlotDataSet
{
public:
  iupPlotDataSet(bool strXdata);
  ~iupPlotDataSet();

  void SetName(const char* inName) { if (inName == mName) return; if (mName) free(mName); mName = iupStrDup(inName); }
  const char* GetName() { return mName; }

  bool FindSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, double inScreenTolerance, int &outSampleIndex, double &outX, double &outY) const;
  bool FindPointSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, double inScreenTolerance, int &outSampleIndex, double &outX, double &outY) const;
  bool FindMultipleBarSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, int &outSampleIndex, double &outX, double &outY) const;
  bool FindBarSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, int &outSampleIndex, double &outX, double &outY) const;
  bool FindHorizontalBarSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, int &outSampleIndex, double &outX, double &outY) const;
  bool FindPieSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, int &outSampleIndex, double &outX, double &outY) const;
  bool FindSegment(iupPlotTrafo *mTrafoX, iupPlotTrafo *mTrafoY, double inScreenX, double inScreenY, double inScreenTolerance, int &outSampleIndex1, int &outSampleIndex2, double &outX1, double &outY1, double &outX2, double &outY2) const;

  void DrawData(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const;
  void DrawDataPie(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify, const iupPlotAxis& inAxisY, long inBackColor) const;

  int GetCount();
  void AddSample(double inX, double inY);
  void InsertSample(int inSampleIndex, double inX, double inY);
  void AddSampleSegment(double inX, double inY, bool inSegment);
  void InsertSampleSegment(int inSampleIndex, double inX, double inY, bool inSegment);
  void AddSample(const char* inX, double inY);
  void InsertSample(int inSampleIndex, const char* inX, double inY);
  void RemoveSample(int inSampleIndex);
  void GetSample(int inSampleIndex, double *inX, double *inY);
  void GetSample(int inSampleIndex, const char* *inX, double *inY);
  bool GetSampleSelection(int inSampleIndex);
  double GetSampleExtra(int inSampleIndex);
  void SetSample(int inSampleIndex, double inX, double inY);
  void SetSample(int inSampleIndex, const char* inX, double inY);
  void SetSampleSelection(int inSampleIndex, bool inSelected);
  void SetSampleExtra(int inSampleIndex, double inExtra);

  const iupPlotData* GetDataX() const { return mDataX; }
  const iupPlotData* GetDataY() const { return mDataY; }
  const iupPlotDataBool* GetSelection() const { return mSelection; }
  const iupPlotDataBool* GetSegment() const { return mSegment; }
  const iupPlotDataReal* GetExtra() const { return mExtra; }

  bool SelectSamples(double inMinX, double inMaxX, double inMinY, double inMaxY, const iupPlotSampleNotify* inNotify);
  bool ClearSelection(const iupPlotSampleNotify* inNotify);
  bool DeleteSelectedSamples(const iupPlotSampleNotify* inNotify);

  long mColor;
  iupPlotMode mMode;
  int mLineStyle;
  int mLineWidth;
  unsigned char mAreaTransparency;
  int mMarkStyle;
  int mMarkSize;
  int mMultibarIndex;
  int mMultibarCount;
  long mBarOutlineColor;
  bool mBarShowOutline;
  bool mBarMulticolor;
  int mBarSpacingPercent;
  double mPieRadius;
  double mPieStartAngle;
  bool mPieContour;
  double mPieHole;
  iupPlotSliceLabel mPieSliceLabel;
  double mPieSliceLabelPos;
  void* mUserData;
  bool mOrderedX;
  bool mSelectedCurve;

  // Aux
  int mHighlightedSample;
  bool mHighlightedCurve;

protected:
  char* mName;

  iupPlotData* mDataX;
  iupPlotData* mDataY;
  iupPlotDataBool* mSelection;
  iupPlotDataReal* mExtra;
  iupPlotDataBool* mSegment;
  bool mHasSelected;

  void InitSegment();
  void InitExtra();

  void DrawDataLine(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify, bool inShowMark, bool inErrorBar) const;
  void DrawDataMark(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const;
  void DrawDataStem(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify, bool inShowMark) const;
  void DrawDataArea(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const;
  void DrawDataBar(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const;
  void DrawDataHorizontalBar(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const;
  void DrawDataMultiBar(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const;
  void DrawSelection(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const;
  void DrawDataStep(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const;

  void DrawErrorBar(const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, int index, double theY, double theScreenX) const;
  void SetSampleExtraMarkSize(const iupPlotTrafo *inTrafoY, int inSampleIndex, int *outMarkSize) const;
};

class iupPlotTick;

class iupPlotTickIter
{
public:
  iupPlotTickIter() :mAxis(NULL){}
  virtual ~iupPlotTickIter() {}

  virtual bool Init() = 0;
  virtual bool GetNextTick(double &outTick, bool &outIsMajorTick, char* outFormatString) = 0;

  virtual bool CalculateSpacing(double inParRange, double inDivGuess, iupPlotTick &outTickInfo) const = 0;
  virtual bool AdjustRange(double &, double &) const { return true; };

  void SetAxis(const iupPlotAxis *inAxis) { mAxis = inAxis; };

protected:
  const iupPlotAxis *mAxis;
};

class iupPlotTickIterLinear : public iupPlotTickIter
{
public:
  iupPlotTickIterLinear() :mCurrentTick(0), mCount(0), mDelta(0){}

  virtual bool Init();
  virtual bool GetNextTick(double &outTick, bool &outIsMajorTick, char* outFormatString);

  bool CalculateSpacing(double inParRange, double inDivGuess, iupPlotTick &outTickInfo) const;

protected:
  double mCurrentTick;
  long mCount;
  double mDelta;
};

class iupPlotTickIterLog : public iupPlotTickIter
{
public:
  iupPlotTickIterLog() :mCurrentTick(0), mCount(0), mDelta(0){}

  virtual bool Init();
  virtual bool GetNextTick(double &outTick, bool &outIsMajorTick, char* outFormatString);

  bool CalculateSpacing(double inParRange, double inDivGuess, iupPlotTick &outTickInfo) const;
  virtual bool AdjustRange(double &ioMin, double &ioMax) const;

  double RoundUp(double inFloat) const;
  double RoundDown(double inFloat) const;

protected:
  double mCurrentTick;
  long mCount;
  double mDelta;
};

class iupPlotTickIterNamed : public iupPlotTickIterLinear
{
public:
  iupPlotTickIterNamed(){}

  void SetStringList(const iupPlotDataString* inStringData) { mStringData = inStringData; };

  virtual bool GetNextTick(double &outTick, bool &outIsMajorTick, char* outFormatString);
  bool CalculateSpacing(double inParRange, double inDivGuess, iupPlotTick &outTickInfo) const;

protected:
  const iupPlotDataString* mStringData;
};

class iupPlotTick
{
public:
  iupPlotTick()
    :mAutoSpacing(true), mAutoSize(true), mMinorDivision(1), mShowNumber(true),
    mMajorSpan(1), mMajorSize(1), mMinorSize(1), mShow(true), mRotateNumberAngle(90),
    mFontSize(0), mFontStyle(-1), mRotateNumber(false), mFormatAuto(true)
  {
    strcpy(mFormatString, IUP_PLOT_DEF_NUMBERFORMAT);
  }

  bool mShow;

  bool mShowNumber;
  bool mRotateNumber;
  double mRotateNumberAngle;
  char mFormatString[30];
  bool mFormatAuto;
  int mFontSize;
  int mFontStyle;

  bool mAutoSpacing;
  int mMinorDivision;
  double mMajorSpan;

  bool mAutoSize;
  int mMajorSize;
  int mMinorSize;
};

class iupPlotAxis
{
public:
  iupPlotAxis(int inDefaultFontStyle, int inDefaultFontSize, bool inVertical)
    : mShow(true), mMin(0), mMax(0), mAutoScaleMin(true), mAutoScaleMax(true),
    mReverse(false), mLogScale(false), mPosition(IUP_PLOT_START), mColor(iupDrawColor(0,0,0,255)),
    mMaxDecades(-1), mLogBase(10), mLabelCentered(true), mHasZoom(false),
    mDiscrete(false), mLabel(NULL), mShowArrow(true), mLineWidth(1), mLabelSpacing(-1),
    mFontSize(0), mFontStyle(-1), mDefaultFontSize(inDefaultFontSize),
    mTrafo(NULL), mTickIter(NULL), mDefaultFontStyle(inDefaultFontStyle),
    mNoZoomMin(0), mNoZoomMax(0), mNoZoomAutoScaleMin(false), mNoZoomAutoScaleMax(false),
    mPanMin(0), mReverseTicksLabel(false), mVertical(inVertical)
  {
    strcpy(mTipFormatString, IUP_PLOT_DEF_TIPFORMAT);
  }
  ~iupPlotAxis() { SetLabel(NULL); }

  void SetLabel(const char* inLabel) { if (inLabel == mLabel) return; if (mLabel) free(mLabel); mLabel = iupStrDup(inLabel); }
  const char* GetLabel() const { return mLabel; }

  void Init();
  void SetNamedTickIter(const iupPlotDataString *inStringXData);
  void GetTickNumberSize(Ihandle* ih, int *outWitdh, int *outHeight) const;
  int GetTickNumberHeight(Ihandle* ih) const;
  int GetTickNumberWidth(Ihandle* ih) const;
  int GetArrowSize() const;

  bool HasZoom() const { return mHasZoom; }
  bool ResetZoom();
  bool ZoomIn(double inCenter);
  bool ZoomOut(double inCenter);
  bool ZoomTo(double inMin, double inMax);
  void PanStart() { mPanMin = mMin; }
  bool Pan(double inOffset);
  bool Scroll(double inDelta, bool inFullPage);
  bool ScrollTo(double inMin);

  void SetFont(Ihandle* ih, int inFontStyle, int inFontSize) const;

  bool mShow;
  bool mVertical;
  long mColor;
  double mMin;
  double mMax;
  bool mAutoScaleMin;
  bool mAutoScaleMax;
  bool mReverse;
  iupPlotAxisPosition mPosition;
  bool mShowArrow;
  char mTipFormatString[30];
  bool mReverseTicksLabel;

  int mFontSize;
  int mFontStyle;
  bool mLabelCentered;
  int mDefaultFontSize;
  int mDefaultFontStyle;
  int mLabelSpacing;

  bool mLogScale;
  int mMaxDecades;
  double mLogBase;
  bool mDiscrete;

  int mLineWidth;

  iupPlotTick mTick;

  iupPlotTrafo *mTrafo;
  iupPlotTickIter *mTickIter;

protected:
  char* mLabel;

  iupPlotTrafoLinear mLinTrafo;
  iupPlotTrafoLog mLogTrafo;

  iupPlotTickIterLinear mLinTickIter;
  iupPlotTickIterLog mLogTickIter;
  iupPlotTickIterNamed mNamedTickIter;

  void InitZoom();
  void CheckZoomOutLimit(double inRange);

  bool mHasZoom;
  double mNoZoomMin;
  double mNoZoomMax;
  bool mNoZoomAutoScaleMin;
  bool mNoZoomAutoScaleMax;
  double mPanMin;
};

class iupPlotAxisX : public iupPlotAxis
{
public:
  iupPlotAxisX(int inDefaultFontStyle, int inDefaultFontSize) :
    iupPlotAxis(inDefaultFontStyle, inDefaultFontSize, false) {}

  bool DrawX(const iupPlotRect &inRect, iupPlotDrawContext* ctx, const iupPlotAxis& inAxisY) const;
protected:
  void DrawXTick(double inX, double inScreenY, bool inMajor, const char* inFormatString, iupPlotDrawContext* ctx, IFnssds formatticknumber_cb) const;
  double GetScreenYOriginX(const iupPlotAxis& inAxisY) const;
};

class iupPlotAxisY : public iupPlotAxis
{
public:
  iupPlotAxisY(int inDefaultFontStyle, int inDefaultFontSize) :
    iupPlotAxis(inDefaultFontStyle, inDefaultFontSize, true) {}

  bool DrawY(const iupPlotRect &inRect, iupPlotDrawContext* ctx, const iupPlotAxis& inAxisX) const;

protected:
  void DrawYTick(double inY, double inScreenX, bool inMajor, const char* inFormatString, iupPlotDrawContext* ctx, IFnssds formatticknumber_cb) const;
  double GetScreenXOriginY(const iupPlotAxis& inAxisX) const;
};

class iupPlotGrid
{
public:
  iupPlotGrid(bool inMajor)
    : mShowX(false), mShowY(false), mColor(iupDrawColor(200, 200, 200, 255)),
    mLineStyle(IUP_PLOT_LINE_CONTINUOUS), mLineWidth(1), mMajor(inMajor) {}

  bool DrawX(iupPlotTickIter* inTickIter, iupPlotTrafo* inTrafo, const iupPlotRect &inRect, iupPlotDrawContext* ctx) const;
  bool DrawY(iupPlotTickIter* inTickIter, iupPlotTrafo* inTrafo, const iupPlotRect &inRect, iupPlotDrawContext* ctx) const;

  bool mMajor;
  bool mShowX;
  bool mShowY;
  long mColor;
  int  mLineStyle;
  int  mLineWidth;
};

class iupPlotBox
{
public:
  iupPlotBox()
    : mShow(false), mColor(iupDrawColor(0,0,0,255)),
    mLineStyle(IUP_PLOT_LINE_CONTINUOUS), mLineWidth(1) {}

  void Draw(const iupPlotRect &inRect, iupPlotDrawContext* ctx) const;

  bool mShow;
  long mColor;
  int  mLineStyle;
  int  mLineWidth;
};

class iupPlotLegend
{
public:
  iupPlotLegend()
    : mShow(false), mBoxShow(true), mFontSize(0), mFontStyle(-1), mPosition(IUP_PLOT_TOPRIGHT),
    mBoxLineStyle(IUP_PLOT_LINE_CONTINUOUS), mBoxLineWidth(1), mBoxColor(iupDrawColor(0,0,0,255)), mBoxBackColor(iupDrawColor(255,255,255,255))
  {}

  bool mShow;
  iupPlotLegendPosition mPosition;
  iupPlotRect mPos;
  int mFontSize;
  int mFontStyle;

  bool mBoxShow;
  long mBoxColor;
  long mBoxBackColor;
  int  mBoxLineStyle;
  int  mBoxLineWidth;
};

class iupPlotTitle
{
public:
  iupPlotTitle()
    : mColor(iupDrawColor(0,0,0,255)), mText(NULL), mFontSize(0), mFontStyle(-1),
    mAutoPos(true), mPosX(0), mPosY(0) {}
  ~iupPlotTitle() { if (mText) free(mText); }

  void SetText(const char* inText) { if (inText == mText) return; if (mText) free(mText); mText = iupStrDup(inText); }
  const char* GetText() const { return mText; }

  long mColor;
  int mFontSize;
  int mFontStyle;
  bool mAutoPos;
  int mPosX, mPosY;

protected:
  char* mText;
};

class iupPlotBackground
{
public:
  iupPlotBackground()
    : mColor(iupDrawColor(255,255,255,255)), mMarginAuto(1, 1, 1, 1), mImage(NULL), mHorizPadding(5), mVertPadding(5), mTransparent(false){}
  ~iupPlotBackground() { if (mImage) free(mImage); }

  void SetImage(const char* inImage) { if (inImage == mImage) return; if (mImage) free(mImage); mImage = iupStrDup(inImage); }
  const char* GetImage() const { return mImage; }

  iupPlotMargin mMargin,
                mMarginAuto;
  long mColor;
  double mImageMinX,
         mImageMaxX,
         mImageMinY,
         mImageMaxY;

  int mHorizPadding, mVertPadding;

  bool mTransparent;

protected:
  char* mImage;
};


class iupPlot
{
public:
  iupPlot(Ihandle* ih, int inDefaultFontStyle, int inDefaultFontSize);
  ~iupPlot();

  /*********************************/

  bool mRedraw;
  iupPlotRect mViewport;
  bool mViewportSquare;
  int mDefaultFontSize;
  int mDefaultFontStyle;
  bool mScaleEqual;
  iupPlotClipping mDataSetClipping;

  bool mCrossHairH, mCrossHairV;
  int mCrossHairX, mCrossHairY;
  bool mShowSelectionBand;
  iupPlotRect mSelectionBand;
  iupPlotHighlight mHighlightMode;
  double mScreenTolerance;

  iupPlotBackground mBack;
  iupPlotGrid mGrid;
  iupPlotGrid mGridMinor;
  iupPlotAxisX mAxisX;
  iupPlotAxisY mAxisY;
  iupPlotBox mBox;
  iupPlotLegend mLegend;
  iupPlotTitle mTitle;

  iupPlotDataSet* *mDataSetList;
  int mDataSetListCount;
  int mDataSetListMax;
  int mCurrentDataSet;

  bool PrepareRender(iupPlotDrawContext* ctx);
  bool Render(iupPlotDrawContext* ctx);
  void SetViewport(int x, int y, int w, int h);

  void ResetZoom() { if (mAxisX.ResetZoom()) mRedraw = true; if (mAxisY.ResetZoom()) mRedraw = true; }
  void ZoomIn(double inCenterX, double inCenterY) { if (mAxisX.ZoomIn(inCenterX)) mRedraw = true; if (mAxisY.ZoomIn(inCenterY)) mRedraw = true; }
  void ZoomOut(double inCenterX, double inCenterY) { if (mAxisX.ZoomOut(inCenterX)) mRedraw = true; if (mAxisY.ZoomOut(inCenterY)) mRedraw = true; }
  void ZoomTo(double inMinX, double inMaxX, double inMinY, double inMaxY) { if (mAxisX.ZoomTo(inMinX, inMaxX)) mRedraw = true; if (mAxisY.ZoomTo(inMinY, inMaxY)) mRedraw = true; }
  void PanStart() { mAxisX.PanStart(); mAxisY.PanStart(); }
  void Pan(double inOffsetX, double inOffsetY) { if (mAxisX.Pan(inOffsetX)) mRedraw = true; if (mAxisY.Pan(inOffsetY)) mRedraw = true; }
  void Scroll(double inDelta, bool inFullPage, bool inVertical) { if (inVertical) { if (mAxisY.Scroll(inDelta, inFullPage)) mRedraw = true; } else { if (mAxisX.Scroll(inDelta, inFullPage)) mRedraw = true; } }
  void ScrollTo(double inMinX, double inMinY) { if (mAxisX.ScrollTo(inMinX)) mRedraw = true; if (mAxisY.ScrollTo(inMinY)) mRedraw = true; }

  void TransformBack(int inX, int inY, double &outX, double &outY) const {
    outX = mAxisX.mTrafo->TransformBack((double)inX);
    outY = mAxisY.mTrafo->TransformBack((double)inY);
  }

  bool CheckInsideTitle(Ihandle* ih, int x, int y) const;
  bool CheckInsideLegend(int x, int y) const;

  void AddDataSet(iupPlotDataSet* inDataSet);
  void RemoveDataSet(int inIndex);
  int FindDataSet(const char* inName) const;
  void RemoveAllDataSets();

  bool FindDataSetSample(double inScreenX, double inScreenY, int &outIndex, const char* &outName, int &outSampleIndex, double &outX, double &outY, const char* &outStrX) const;
  bool FindDataSetSegment(double inScreenX, double inScreenY, int &outIndex, const char* &outName, int &outSampleIndex1, double &outX1, double &outY1, int &outSampleIndex2, double &outX2, double &outY2) const;
  void SelectDataSetSamples(double inMinX, double inMaxX, double inMinY, double inMaxY);
  void DeleteSelectedDataSetSamples();
  void ClearDataSetSelection();
  void ClearHighlight();

  void UpdateMultibarCount();

protected:
  Ihandle* ih;
  iupPlotRect mViewportBack;

  void DataSetClipArea(iupPlotDrawContext* ctx, int xmin, int xmax, int ymin, int ymax) const;
  void ConfigureAxis();
  void SetFont(Ihandle* ih, int inFontStyle, int inFontSize) const;
  void SetTitleFont(Ihandle* ih) const;
  bool CheckRange(const iupPlotAxis &inAxis) const;
  bool HasZoom() const { return mAxisX.HasZoom() || mAxisY.HasZoom(); }
  long GetNextDataSetColor() const;
  iupPlotDataSet* HasPie() const;

  /*********************************/

  void DrawTitle(iupPlotDrawContext* ctx) const;
  void DrawBackground(iupPlotDrawContext* ctx) const;
  void DrawBackgroundImage(iupPlotDrawContext* ctx) const;
  bool DrawLegend(const iupPlotRect &inRect, iupPlotDrawContext* ctx, iupPlotRect &ioPos) const;
  bool DrawSampleColorLegend(iupPlotDataSet *inData, const iupPlotRect &inRect, iupPlotDrawContext* ctx, iupPlotRect &ioPos) const;
  void DrawCrossHairH(const iupPlotRect &inRect, iupPlotDrawContext* ctx) const;
  void DrawCrossSamplesH(const iupPlotRect &inRect, const iupPlotData *inXData, const iupPlotData *inYData, iupPlotDrawContext* ctx) const;
  void DrawCrossHairV(const iupPlotRect &inRect, iupPlotDrawContext* ctx) const;
  void DrawCrossSamplesV(const iupPlotRect &inRect, const iupPlotData *inXData, const iupPlotData *inYData, iupPlotDrawContext* ctx) const;
  void DrawInactive(iupPlotDrawContext* ctx) const;

  /*********************************/

  void CalculateTitlePos();
  void CalculateMargins(Ihandle* ih);
  bool CalculateAxisRange();
  void CalculateXRange(double &outXMin, double &outXMax) const;
  void CalculateYRange(double &outYMin, double &outYMax) const;
  bool CalculateYTransformation(const iupPlotRect &inRect);
  bool CalculateXTransformation(const iupPlotRect &inRect);
  bool CalculateLinTransformation(int inBegin, int inEnd, const iupPlotAxis& inAxis, iupPlotTrafoLinear* outTrafo);
  bool CalculateLogTransformation(int inBegin, int inEnd, const iupPlotAxis& inAxis, iupPlotTrafoLog* outTrafo);
  bool CalculateTickSpacing(const iupPlotRect &inRect, Ihandle* ih);
  void CalculateTickSize(Ihandle* ih, iupPlotTick &ioTick);

  int CalcYTickVerticalMargin(Ihandle* ih, bool start) const;
  int CalcXTickHorizontalMargin(Ihandle* ih, bool start) const;
  int CalcXTickVerticalMargin(Ihandle* ih) const;
  int CalcYTickHorizontalMargin(Ihandle* ih) const;
  int CalcTitleVerticalMargin(Ihandle* ih) const;
};

#endif
