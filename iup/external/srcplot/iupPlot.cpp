
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iupPlot.h"


/************************************************************************************************/


iupPlot::iupPlot(Ihandle* _ih, int inDefaultFontStyle, int inDefaultFontSize)
  :ih(_ih), mCurrentDataSet(-1), mRedraw(true), mDataSetListCount(0), mCrossHairH(false), mCrossHairV(false),
   mGrid(true), mGridMinor(false), mViewportSquare(false), mScaleEqual(false), mHighlightMode(IUP_PLOT_HIGHLIGHT_NONE),
   mDefaultFontSize(inDefaultFontSize), mDefaultFontStyle(inDefaultFontStyle), mScreenTolerance(5),
   mAxisX(inDefaultFontStyle, inDefaultFontSize), mAxisY(inDefaultFontStyle, inDefaultFontSize),
   mCrossHairX(0), mCrossHairY(0), mShowSelectionBand(false), mDataSetListMax(20), mDataSetClipping(IUP_PLOT_CLIPAREA)
{
  mDataSetList = (iupPlotDataSet**)malloc(sizeof(iupPlotDataSet*)* mDataSetListMax); /* use malloc because we will use realloc */
  memset(mDataSetList, 0, sizeof(iupPlotDataSet*)* mDataSetListMax);
}

iupPlot::~iupPlot()
{
  RemoveAllDataSets();
  free(mDataSetList);  /* use free because we used malloc */
}

void iupPlot::SetViewport(int x, int y, int w, int h)
{
  mViewportBack.mX = x;
  mViewportBack.mY = y;
  mViewportBack.mWidth = w;
  mViewportBack.mHeight = h;

  if (mViewportSquare && w != h)
  {
    /* take the smallest length */
    if (w > h)
    {
      mViewport.mX = x + (w - h) / 2;
      mViewport.mY = y;
      mViewport.mWidth = h;
      mViewport.mHeight = h;
    }
    else
    {
      mViewport.mX = x;
      mViewport.mY = y + (h - w) / 2;
      mViewport.mWidth = w;
      mViewport.mHeight = w;
    }
  }
  else
  {
    mViewport.mX = x;
    mViewport.mY = y;
    mViewport.mWidth = w;
    mViewport.mHeight = h;
  }

  mRedraw = true;
}

void iupPlot::SetFont(Ihandle* ih, int inFontStyle, int inFontSize) const
{
  if (inFontStyle == -1) inFontStyle = mDefaultFontStyle;
  if (inFontSize == 0) inFontSize = mDefaultFontSize;
  char fontStr[256];
  iupPlotBuildFont(ih, inFontStyle, inFontSize, fontStr);
  IupSetStrAttribute(ih, "DRAWFONT", fontStr);
}

void iupPlot::UpdateMultibarCount()
{
  int i, count = 0, index = 0;

  for (i = 0; i < mDataSetListCount; i++)
  {
    if (mDataSetList[i]->mMode == IUP_PLOT_MULTIBAR)
      count++;
  }

  for (i = 0; i < mDataSetListCount; i++)
  {
    if (mDataSetList[i]->mMode == IUP_PLOT_MULTIBAR)
    {
      mDataSetList[i]->mMultibarCount = count;
      mDataSetList[i]->mMultibarIndex = index;
      index++;
    }
    else
    {
      mDataSetList[i]->mMultibarCount = 0;
      mDataSetList[i]->mMultibarIndex = 0;
    }
  }
}

static long iPlotGetDefaultColor(int index)
{
  switch (index)
  {
  case 0: return iupDrawColor(255, 0, 0, 255);
  case 1: return iupDrawColor(0, 255, 0, 255);
  case 2: return iupDrawColor(0, 0, 255, 255);
  case 3: return iupDrawColor(0, 255, 255, 255);
  case 4: return iupDrawColor(255, 0, 255, 255);
  case 5: return iupDrawColor(255, 255, 0, 255);
  case 6: return iupDrawColor(128, 0, 0, 255);
  case 7: return iupDrawColor(0, 128, 0, 255);
  case 8: return iupDrawColor(0, 0, 128, 255);
  case 9: return iupDrawColor(0, 128, 128, 255);
  case 10: return iupDrawColor(128, 0, 128, 255);
  case 11: return iupDrawColor(128, 128, 0, 255);
  default: return iupDrawColor(0, 0, 0, 255);
  }
}

long iupPlot::GetNextDataSetColor() const
{
  int def_color = 0, i = 0;
  long theColor;

  do
  {
    theColor = iPlotGetDefaultColor(def_color);

    for (i = 0; i < mDataSetListCount; i++)
    {
      // already used, get another
      long theDataSetColor = iupDrawColor(iupDrawRed(mDataSetList[i]->mColor), iupDrawGreen(mDataSetList[i]->mColor), iupDrawBlue(mDataSetList[i]->mColor), 255);
      if (theDataSetColor == theColor)
        break;
    }

    // not found, use it
    if (i == mDataSetListCount)
      break;

    def_color++;
  } while (def_color < 12);

  return theColor;
}

void iupPlot::AddDataSet(iupPlotDataSet* inDataSet)
{
  if (mDataSetListCount >= mDataSetListMax)
  {
    int old_max = mDataSetListMax;
    mDataSetListMax += 20;
    mDataSetList = (iupPlotDataSet**)realloc(mDataSetList, sizeof(iupPlotDataSet*)* mDataSetListMax);
    memset(mDataSetList + old_max, 0, sizeof(iupPlotDataSet*)* (mDataSetListMax - old_max));
  }

  if (mDataSetListCount < mDataSetListMax)
  {
    long theColor = GetNextDataSetColor();

    mCurrentDataSet = mDataSetListCount;
    mDataSetListCount++;

    char theLegend[30];
    sprintf(theLegend, "plot %d", mCurrentDataSet);

    mDataSetList[mCurrentDataSet] = inDataSet;

    inDataSet->SetName(theLegend);
    inDataSet->mColor = theColor;
  }
}

void iupPlot::RemoveDataSet(int inIndex)
{
  if (mCurrentDataSet == mDataSetListCount - 1)
    mCurrentDataSet--;

  delete mDataSetList[inIndex];

  for (int i = inIndex; i < mDataSetListCount; i++)
    mDataSetList[i] = mDataSetList[i + 1];

  mDataSetList[mDataSetListCount - 1] = NULL;

  mDataSetListCount--;
}

int iupPlot::FindDataSet(const char* inName) const
{
  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    if (iupStrEqualNoCase(mDataSetList[ds]->GetName(), inName))
      return ds;
  }
  return -1;
}

void iupPlot::RemoveAllDataSets()
{
  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    delete mDataSetList[ds];
  }
  mDataSetListCount = 0;
}

void iupPlot::ClearHighlight()
{
  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    dataset->mHighlightedCurve = false;
    dataset->mHighlightedSample = -1;
  }
}

bool iupPlot::FindDataSetSample(double inScreenX, double inScreenY, int &outIndex, const char* &outName, int &outSampleIndex, double &outX, double &outY, const char* &outStrX) const
{
  if (!mAxisX.mTrafo || !mAxisY.mTrafo)
    return false;

  /* search for datasets in the inverse order they are drawn */
  for (int ds = mDataSetListCount - 1; ds >= 0; ds--)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];

    if (dataset->FindSample(mAxisX.mTrafo, mAxisY.mTrafo, inScreenX, inScreenY, mScreenTolerance, outSampleIndex, outX, outY))
    {
      const iupPlotData *theXData = dataset->GetDataX();
      if (theXData->IsString())
      {
        const iupPlotDataString *theStringXData = (const iupPlotDataString *)(theXData);
        outStrX = theStringXData->GetSampleString(outSampleIndex);
      }
      else
        outStrX = NULL;

      outIndex = ds;
      outName = dataset->GetName();

      return true;
    }
  }
  return false;
}

bool iupPlot::FindDataSetSegment(double inScreenX, double inScreenY, int &outIndex, const char* &outName, int &outSampleIndex1, double &outX1, double &outY1, int &outSampleIndex2, double &outX2, double &outY2) const
{
  if (!mAxisX.mTrafo || !mAxisY.mTrafo)
    return false;

  /* search for datasets in the inverse order they are drawn */
  for (int ds = mDataSetListCount - 1; ds >= 0; ds--)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];

    // only for modes that have lines connecting the samples.
    if (dataset->mMode != IUP_PLOT_LINE &&
        dataset->mMode != IUP_PLOT_MARKLINE &&
        dataset->mMode != IUP_PLOT_AREA &&
        dataset->mMode != IUP_PLOT_ERRORBAR)
        continue;

    if (dataset->FindSegment(mAxisX.mTrafo, mAxisY.mTrafo, inScreenX, inScreenY, mScreenTolerance, outSampleIndex1, outSampleIndex2, outX1, outY1, outX2, outY2))
    {
      outIndex = ds;
      outName = dataset->GetName();
      return true;
    }
  }
  return false;
}

void iupPlot::SelectDataSetSamples(double inMinX, double inMaxX, double inMinY, double inMaxY)
{
  bool theChanged = false;

  iPlotCheckMinMax(inMinX, inMaxX);
  iPlotCheckMinMax(inMinY, inMaxY);

  IFniiddi select_cb = (IFniiddi)IupGetCallback(ih, "SELECT_CB");
  if (select_cb)
  {
    Icallback cb = IupGetCallback(ih, "SELECTBEGIN_CB");
    if (cb && cb(ih) == IUP_IGNORE)
      return;
  }

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    iupPlotSampleNotify theNotify = { ih, ds, select_cb };
    if (dataset->SelectSamples(inMinX, inMaxX, inMinY, inMaxY, &theNotify))
      theChanged = true;
  }

  if (select_cb)
  {
    Icallback cb = IupGetCallback(ih, "SELECTEND_CB");
    if (cb)
      return;
  }

  if (theChanged)
    mRedraw = true;
}

void iupPlot::ClearDataSetSelection()
{
  bool theChanged = false;

  IFniiddi select_cb = (IFniiddi)IupGetCallback(ih, "SELECT_CB");
  if (select_cb)
  {
    Icallback cb = IupGetCallback(ih, "SELECTBEGIN_CB");
    if (cb && cb(ih) == IUP_IGNORE)
      return;
  }

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    iupPlotSampleNotify theNotify = { ih, ds, select_cb };
    if (dataset->ClearSelection(&theNotify))
      theChanged = true;
  }

  if (select_cb)
  {
    Icallback cb = IupGetCallback(ih, "SELECTEND_CB");
    if (cb)
      return;
  }

  if (theChanged)
    mRedraw = true;
}

void iupPlot::DeleteSelectedDataSetSamples()
{
  bool theChanged = false;

  IFniiddi delete_cb = (IFniiddi)IupGetCallback(ih, "DELETE_CB");
  if (delete_cb)
  {
    Icallback cb = IupGetCallback(ih, "DELETEBEGIN_CB");
    if (cb && cb(ih) == IUP_IGNORE)
      return;
  }

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    iupPlotSampleNotify theNotify = { ih, ds, delete_cb };
    if (dataset->DeleteSelectedSamples(&theNotify))
      theChanged = true;
  }

  if (delete_cb)
  {
    Icallback cb = IupGetCallback(ih, "DELETEEND_CB");
    if (cb)
      return;
  }

  if (theChanged)
    mRedraw = true;
}

void iupPlot::ConfigureAxis()
{
  mAxisX.Init();
  mAxisY.Init();

  if (mAxisX.mLogScale)
    mAxisY.mPosition = IUP_PLOT_START;  // change at the other axis
  else
  {
    if (mDataSetListCount > 0)
    {
      const iupPlotData *theXData = mDataSetList[0]->GetDataX();   // The first dataset will define the named tick usage
      if (theXData->IsString())
      {
        const iupPlotDataString *theStringXData = (const iupPlotDataString *)(theXData);
        mAxisX.SetNamedTickIter(theStringXData);
      }
    }
  }

  if (mAxisY.mLogScale)
    mAxisX.mPosition = IUP_PLOT_START;   // change at the other axis
}

iupPlotDataSet* iupPlot::HasPie() const
{
  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    if (dataset->mMode == IUP_PLOT_PIE)
      return dataset;
  }
  return NULL;
}

void iupPlot::DataSetClipArea(iupPlotDrawContext* ctx, int xmin, int xmax, int ymin, int ymax) const
{
  if (mDataSetClipping == IUP_PLOT_CLIPAREAOFFSET)
  {
    if (!mAxisY.HasZoom())
    {
      int yoff = (ymax - ymin) / 50; // 2%
      if (yoff < 10) yoff = 10;

      ymin -= yoff;
      ymax += yoff;
    }

    if (!mAxisX.HasZoom())
    {
      int xoff = (xmax - xmin) / 50; // 2%
      if (xoff < 10) xoff = 10;

      xmin -= xoff;
      xmax += xoff;
    }
  }

  if (mDataSetClipping != IUP_PLOT_CLIPNONE)
  {
    int absX1 = iupPlotDrawCalcX(ctx, xmin);
    int absY1 = iupPlotDrawCalcY(ctx, ymax);
    int absX2 = iupPlotDrawCalcX(ctx, xmax);
    int absY2 = iupPlotDrawCalcY(ctx, ymin);
    iupdrvDrawSetClipRect(ctx->dc, absX1, absY1, absX2, absY2);
  }
}

bool iupPlot::PrepareRender(iupPlotDrawContext* ctx)
{
  ConfigureAxis();

  if (!CalculateAxisRange())
    return false;

  if (!CheckRange(mAxisX))
    return false;

  if (!CheckRange(mAxisY))
    return false;

  CalculateTitlePos();

  CalculateTickSize(ih, mAxisX.mTick);
  CalculateTickSize(ih, mAxisY.mTick);

  CalculateMargins(ih);

  return true;
}

bool iupPlot::Render(iupPlotDrawContext* ctx)
{
  ctx->viewportX = mViewport.mX;
  ctx->viewportY = mViewport.mY;
  ctx->viewportW = mViewport.mWidth;
  ctx->viewportH = mViewport.mHeight;

  DrawBackground(ctx);

  {
    int clipX1 = iupPlotDrawCalcX(ctx, 0);
    int clipY1 = iupPlotDrawCalcY(ctx, mViewport.mHeight - 1);
    int clipX2 = iupPlotDrawCalcX(ctx, mViewport.mWidth - 1);
    int clipY2 = iupPlotDrawCalcY(ctx, 0);
    iupdrvDrawSetClipRect(ctx->dc, clipX1, clipY1, clipX2, clipY2);
  }

  if (!mDataSetListCount)
    return true;

  iupPlotRect theDataSetArea;
  theDataSetArea.mX = mBack.mMargin.mLeft + mBack.mHorizPadding;
  theDataSetArea.mY = mBack.mMargin.mBottom + mBack.mVertPadding;
  theDataSetArea.mWidth = mViewport.mWidth - mBack.mMargin.mLeft - mBack.mMargin.mRight - 2 * mBack.mHorizPadding;
  theDataSetArea.mHeight = mViewport.mHeight - mBack.mMargin.mTop - mBack.mMargin.mBottom - 2 * mBack.mVertPadding;

  if (!CalculateTickSpacing(theDataSetArea, ih))
  {
    return false;
  }

  if (!CalculateXTransformation(theDataSetArea))
  {
    return false;
  }

  if (!CalculateYTransformation(theDataSetArea))
  {
    return false;
  }

  IFn pre_cb = (IFn)IupGetCallback(ih, "PREDRAW_CB");
  if (pre_cb)
    pre_cb(ih);

  if (mBack.GetImage())
    DrawBackgroundImage(ctx);

  if (!mGrid.DrawX(mAxisX.mTickIter, mAxisX.mTrafo, theDataSetArea, ctx))
    return false;

  if (mGrid.mShowX)
    mGridMinor.DrawX(mAxisX.mTickIter, mAxisX.mTrafo, theDataSetArea, ctx);

  if (!mGrid.DrawY(mAxisY.mTickIter, mAxisY.mTrafo, theDataSetArea, ctx))
    return false;

  if (mGrid.mShowY)
    mGridMinor.DrawY(mAxisY.mTickIter, mAxisY.mTrafo, theDataSetArea, ctx);

  if (!mAxisX.DrawX(theDataSetArea, ctx, mAxisY))
    return false;

  if (!mAxisY.DrawY(theDataSetArea, ctx, mAxisX))
    return false;

  if (mBox.mShow)
    mBox.Draw(theDataSetArea, ctx);

  DataSetClipArea(ctx, theDataSetArea.mX, theDataSetArea.mX + theDataSetArea.mWidth - 1, theDataSetArea.mY, theDataSetArea.mY + theDataSetArea.mHeight - 1);

  IFniiddi drawsample_cb = (IFniiddi)IupGetCallback(ih, "DRAWSAMPLE_CB");

  iupPlotDataSet* pie_dataset = HasPie();

  for (int ds = 0; ds < mDataSetListCount; ds++)
  {
    iupPlotDataSet* dataset = mDataSetList[ds];
    iupPlotSampleNotify theNotify = { ih, ds, drawsample_cb };

    if (pie_dataset)
    {
      if (dataset != pie_dataset)
        continue;
      else
        dataset->DrawDataPie(mAxisX.mTrafo, mAxisY.mTrafo, ctx, &theNotify, mAxisY, mBack.mColor);
    }

    dataset->DrawData(mAxisX.mTrafo, mAxisY.mTrafo, ctx, &theNotify);
  }

  {
    int clipX1 = iupPlotDrawCalcX(ctx, theDataSetArea.mX);
    int clipY1 = iupPlotDrawCalcY(ctx, theDataSetArea.mY + theDataSetArea.mHeight - 1);
    int clipX2 = iupPlotDrawCalcX(ctx, theDataSetArea.mX + theDataSetArea.mWidth - 1);
    int clipY2 = iupPlotDrawCalcY(ctx, theDataSetArea.mY);
    iupdrvDrawSetClipRect(ctx->dc, clipX1, clipY1, clipX2, clipY2);
  }

  if (mCrossHairH)
    DrawCrossHairH(theDataSetArea, ctx);
  else if (mCrossHairV)
    DrawCrossHairV(theDataSetArea, ctx);

  if (mShowSelectionBand)
  {
    if (mSelectionBand.mX < theDataSetArea.mX)
    {
      mSelectionBand.mWidth = mSelectionBand.mX + mSelectionBand.mWidth - theDataSetArea.mX;
      mSelectionBand.mX = theDataSetArea.mX;
    }
    if (mSelectionBand.mY < theDataSetArea.mY)
    {
      mSelectionBand.mHeight = mSelectionBand.mY + mSelectionBand.mHeight - theDataSetArea.mY;
      mSelectionBand.mY = theDataSetArea.mY;
    }
    if (mSelectionBand.mX + mSelectionBand.mWidth > theDataSetArea.mX + theDataSetArea.mWidth)
      mSelectionBand.mWidth = theDataSetArea.mX + theDataSetArea.mWidth - mSelectionBand.mX;
    if (mSelectionBand.mY + mSelectionBand.mHeight > theDataSetArea.mY + theDataSetArea.mHeight)
      mSelectionBand.mHeight = theDataSetArea.mY + theDataSetArea.mHeight - mSelectionBand.mY;

    mBox.Draw(mSelectionBand, ctx);
  }

  IFn post_cb = (IFn)IupGetCallback(ih, "POSTDRAW_CB");
  if (post_cb)
    post_cb(ih);

  if (pie_dataset)
    DrawSampleColorLegend(pie_dataset, theDataSetArea, ctx, mLegend.mPos);
  else if (!DrawLegend(theDataSetArea, ctx, mLegend.mPos))
    return false;

  {
    int clipX1 = iupPlotDrawCalcX(ctx, 0);
    int clipY1 = iupPlotDrawCalcY(ctx, mViewport.mHeight - 1);
    int clipX2 = iupPlotDrawCalcX(ctx, mViewport.mWidth - 1);
    int clipY2 = iupPlotDrawCalcY(ctx, 0);
    iupdrvDrawSetClipRect(ctx->dc, clipX1, clipY1, clipX2, clipY2);
  }

  DrawTitle(ctx);

  if (!IupGetInt(ih, "ACTIVE"))
    DrawInactive(ctx);

  mRedraw = false;
  return true;
}

