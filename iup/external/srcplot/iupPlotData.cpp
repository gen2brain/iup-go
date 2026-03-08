
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iupPlot.h"


static inline bool iPlotCheckInsideBox(double x, double y, double boxMinX, double boxMaxX, double boxMinY, double boxMaxY)
{
  if (x > boxMaxX || x < boxMinX || y > boxMaxY || y < boxMinY)
    return false;

  return true;
}


/************************************************************************************************/


bool iupPlotDataReal::CalculateRange(double &outMin, double &outMax) const
{
  int theCount = iupArrayCount(mArray);
  if (theCount > 0)
  {
    double* theData = (double*)iupArrayGetData(mArray);
    outMax = outMin = theData[0];
    for (int i = 1; i < theCount; i++)
    {
      if (theData[i] > outMax)
        outMax = theData[i];
      if (theData[i] < outMin)
        outMin = theData[i];
    }
    return true;
  }

  return false;
}

iupPlotDataString::~iupPlotDataString()
{
  for (int i = 0; i < mCount; i++)
    free(mData[i]);
}

bool iupPlotDataString::CalculateRange(double &outMin, double &outMax) const
{
  if (mCount > 0)
  {
    outMin = 0;
    outMax = mCount - 1;
    return true;
  }

  return false;
}

bool iupPlotDataBool::CalculateRange(double &outMin, double &outMax) const
{
  if (mCount > 0)
  {
    outMin = 0;
    outMax = mCount - 1;
    return true;
  }

  return false;
}


/************************************************************************************************/


iupPlotDataSet::iupPlotDataSet(bool strXdata)
: mColor(iupDrawColor(0,0,0,255)), mLineStyle(IUP_PLOT_LINE_CONTINUOUS), mLineWidth(1), mAreaTransparency(255), mMarkStyle(IUP_PLOT_MARK_X), mMarkSize(7),
  mMultibarIndex(-1), mMultibarCount(0), mBarOutlineColor(0), mBarShowOutline(false), mBarSpacingPercent(10),
  mPieStartAngle(0), mPieRadius(0.95), mPieContour(false), mPieHole(0), mPieSliceLabelPos(0.95),
  mHighlightedSample(-1), mHighlightedCurve(false), mBarMulticolor(false), mOrderedX(false), mSelectedCurve(false),
  mPieSliceLabel(IUP_PLOT_NONE), mMode(IUP_PLOT_LINE), mName(NULL), mHasSelected(false), mUserData(0)
{
  if (strXdata)
    mDataX = (iupPlotData*)(new iupPlotDataString());
  else
    mDataX = (iupPlotData*)(new iupPlotDataReal());

  mDataY = (iupPlotData*)new iupPlotDataReal();

  mSelection = new iupPlotDataBool();
  mSegment = NULL;
  mExtra = NULL;
}

iupPlotDataSet::~iupPlotDataSet()
{
  SetName(NULL);

  delete mDataX;
  delete mDataY;
  delete mSelection;
  if (mSegment)
    delete mSegment;
  if (mExtra)
    delete mExtra;
}

bool iupPlotDataSet::FindSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, double inScreenTolerance,
                                int &outSampleIndex, double &outX, double &outY) const
{
  switch (mMode)
  {
  case IUP_PLOT_MULTIBAR:
    return this->FindMultipleBarSample(inTrafoX, inTrafoY, inScreenX, inScreenY, outSampleIndex, outX, outY);
  case IUP_PLOT_BAR:
    return this->FindBarSample(inTrafoX, inTrafoY, inScreenX, inScreenY, outSampleIndex, outX, outY);
  case IUP_PLOT_HORIZONTALBAR:
    return this->FindHorizontalBarSample(inTrafoX, inTrafoY, inScreenX, inScreenY, outSampleIndex, outX, outY);
  case IUP_PLOT_PIE:
    return this->FindPieSample(inTrafoX, inTrafoY, inScreenX, inScreenY, outSampleIndex, outX, outY);
  default:
    return this->FindPointSample(inTrafoX, inTrafoY, inScreenX, inScreenY, inScreenTolerance, outSampleIndex, outX, outY);
  }
}

bool iupPlotDataSet::FindPointSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, double inScreenTolerance,
                                     int &outSampleIndex, double &outX, double &outY) const
{
  double theX, theY, theScreenX, theScreenY;
  double thePrevScreenX = 0;

  int theCount = mDataX->GetCount();

  for (int i = 0; i < theCount; i++)
  {
    theX = mDataX->GetSample(i);
    theY = mDataY->GetSample(i);
    theScreenX = inTrafoX->Transform(theX);
    theScreenY = inTrafoY->Transform(theY);

    // optimization when X values are ordered
    if (mOrderedX && i > 0 && (inScreenX < thePrevScreenX - inScreenTolerance || inScreenX > theScreenX + inScreenTolerance))
    {
      if (inScreenX < thePrevScreenX - inScreenTolerance)
        break;

      thePrevScreenX = theScreenX;
      continue;
    }

    if (fabs(theScreenX - inScreenX) < inScreenTolerance &&
        fabs(theScreenY - inScreenY) < inScreenTolerance)
    {
      outX = theX;
      outY = theY;
      outSampleIndex = i;
      return true;
    }

    thePrevScreenX = theScreenX;
  }

  return false;
}

bool iupPlotDataSet::FindMultipleBarSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY,
                                           int &outSampleIndex, double &outX, double &outY) const
{
  double theX, theY, theScreenX, theScreenY;

  int theCount = mDataX->GetCount();

  double theScreenY0 = inTrafoY->Transform(0);

  double theMinX = mDataX->GetSample(0);
  double theScreenMinX = inTrafoX->Transform(theMinX);
  double theMaxX = mDataX->GetSample(theCount - 1);
  double theScreenMaxX = inTrafoX->Transform(theMaxX);

  double theTotalBarWidth = (theScreenMaxX - theScreenMinX) / (theCount - 1);
  theTotalBarWidth *= 1 - (double)mBarSpacingPercent / 100.0;
  double theBarWidth = theTotalBarWidth / mMultibarCount;

  for (int i = 0; i < theCount; i++)
  {
    theX = mDataX->GetSample(i);
    theY = mDataY->GetSample(i);
    theScreenX = inTrafoX->Transform(theX);
    theScreenY = inTrafoY->Transform(theY);

    double theBarX = (theScreenX - theTotalBarWidth / 2) + (mMultibarIndex*theBarWidth);
    double theBarHeight = theScreenY - theScreenY0;

    if (iPlotCheckInsideBox(inScreenX, inScreenY, theBarX, theBarX + theBarWidth, theScreenY0, theScreenY0 + theBarHeight))
    {
      outX = theX;
      outY = theY;
      outSampleIndex = i;
      return true;
    }
  }

  return false;
}

bool iupPlotDataSet::FindBarSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY,
                                   int &outSampleIndex, double &outX, double &outY) const
{
  double theX, theY, theScreenX, theScreenY;

  int theCount = mDataX->GetCount();

  double theScreenY0 = inTrafoY->Transform(0);

  double theMinX = mDataX->GetSample(0);
  double theScreenMinX = inTrafoX->Transform(theMinX);
  double theMaxX = mDataX->GetSample(theCount - 1);
  double theScreenMaxX = inTrafoX->Transform(theMaxX);

  double theBarWidth = (theScreenMaxX - theScreenMinX) / (theCount - 1);
  theBarWidth *= 1 - (double)mBarSpacingPercent / 100.0;

  for (int i = 0; i < theCount; i++)
  {
    theX = mDataX->GetSample(i);
    theY = mDataY->GetSample(i);
    theScreenX = inTrafoX->Transform(theX);
    theScreenY = inTrafoY->Transform(theY);

    double theBarX = theScreenX - theBarWidth / 2;
    double theBarHeight = theScreenY - theScreenY0;

    if (iPlotCheckInsideBox(inScreenX, inScreenY, theBarX, theBarX + theBarWidth, theScreenY0, theScreenY0 + theBarHeight))
    {
      outX = theX;
      outY = theY;
      outSampleIndex = i;
      return true;
    }
  }

  return false;
}

bool iupPlotDataSet::FindHorizontalBarSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY,
                                             int &outSampleIndex, double &outX, double &outY) const
{
  double theX, theY, theScreenX, theScreenY;

  int theCount = mDataX->GetCount();

  double theScreenX0 = inTrafoX->Transform(0);

  double theMinY = mDataY->GetSample(0);
  double theScreenMinY = inTrafoY->Transform(theMinY);
  double theMaxY = mDataY->GetSample(theCount - 1);
  double theScreenMaxY = inTrafoY->Transform(theMaxY);

  double theBarHeight = (theScreenMaxY - theScreenMinY) / (theCount - 1);
  theBarHeight *= 1 - (double)mBarSpacingPercent / 100.0;

  for (int i = 0; i < theCount; i++)
  {
    theX = mDataX->GetSample(i);
    theY = mDataY->GetSample(i);
    theScreenX = inTrafoX->Transform(theX);
    theScreenY = inTrafoY->Transform(theY);

    double theBarY = theScreenY - theBarHeight / 2;
    double theBarWidth = theScreenX - theScreenX0;

    if (iPlotCheckInsideBox(inScreenX, inScreenY, theScreenX0, theScreenX0 + theBarWidth, theBarY, theBarY + theBarHeight))
    {
      outX = theX;
      outY = theY;
      outSampleIndex = i;
      return true;
    }
  }

  return false;
}

bool iupPlotDataSet::FindPieSample(iupPlotTrafo *inTrafoX, iupPlotTrafo *inTrafoY, double inScreenX, double inScreenY, int &outSampleIndex, double &outX, double &outY) const
{
  double theX, theY;

  int theCount = mDataX->GetCount();
  double sum = 0;

  for (int i = 0; i < theCount; i++)
  {
    theY = mDataY->GetSample(i);

    if (theY <= 0)
      continue;

    sum += theY;
  }

  double inX = inTrafoX->TransformBack(inScreenX);
  double inY = inTrafoY->TransformBack(inScreenY);

  double inRadius = sqrt(inX*inX + inY*inY);

  double holeRadius = mPieHole * mPieRadius;

  double inAngle = atan2(inY, inX);

  inAngle = (180.0 / M_PI) * inAngle;

  if (inAngle < 0)
    inAngle += 360.;

  if (inRadius < holeRadius || inRadius > mPieRadius)
    return false;

  double startAngle = mPieStartAngle;

  for (int i = 0; i < theCount; i++)
  {
    theX = mDataX->GetSample(i);
    theY = mDataY->GetSample(i);

    if (theY <= 0)
      continue;

    double angle = (theY*360.) / sum;

    if (inAngle > startAngle  &&
        inAngle < startAngle + angle)
    {
      outX = theX;
      outY = theY;
      outSampleIndex = i;
      return true;
    }

    startAngle += angle;
  }

  return false;
}

static bool iPlotCheckInsideBoxTolerance(double x1, double y1, double x2, double y2, double inX, double inY, double inTolerance)
{
  if (x1 > x2)
  {
    double tmp = x1;
    x1 = x2;
    x2 = tmp;
  }

  if (y1 > y2)
  {
    double tmp = y1;
    y1 = y2;
    y2 = tmp;
  }

  x1 -= inTolerance;
  x2 += inTolerance;

  y1 -= inTolerance;
  y2 += inTolerance;

  if (inX < x1 || inX > x2)
    return false;

  if (inY < y1 || inY > y2)
    return false;

  return true;
}

bool iupPlotDataSet::FindSegment(iupPlotTrafo *mTrafoX, iupPlotTrafo *mTrafoY, double inScreenX, double inScreenY, double inScreenTolerance,
                                 int &outSampleIndex1, int &outSampleIndex2, double &outX1, double &outY1, double &outX2, double &outY2) const
{
  if (!mTrafoX || !mTrafoY)
    return false;

  double lowestDist = 0;
  int found_Id = -1;
  double found_x1 = 0, found_y1 = 0, found_x2 = 0, found_y2 = 0;
  bool found = false;

  double theX1 = mDataX->GetSample(0);
  double theY1 = mDataY->GetSample(0);
  double theScreenX1 = mTrafoX->Transform(theX1);
  double theScreenY1 = mTrafoY->Transform(theY1);

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount - 1; i++)
  {
    double theX2 = mDataX->GetSample(i + 1);
    double theY2 = mDataY->GetSample(i + 1);
    double theScreenX2 = mTrafoX->Transform(theX2);
    double theScreenY2 = mTrafoY->Transform(theY2);

    // optimization when X values are ordered
    if (mOrderedX && (inScreenX < theScreenX1 || inScreenX > theScreenX2))
    {
      if (inScreenX < theScreenX1)
        break;

      theX1 = theX2;
      theY1 = theY2;
      theScreenX1 = theScreenX2;
      theScreenY1 = theScreenY2;
      continue;
    }

    // inX,inY must be inside box theScreenX1,theScreenY1 - theScreenX2,theScreenY2
    if (!iPlotCheckInsideBoxTolerance(theScreenX1, theScreenY1, theScreenX2, theScreenY2, inScreenX, inScreenY, inScreenTolerance))
    {
      theX1 = theX2;
      theY1 = theY2;
      theScreenX1 = theScreenX2;
      theScreenY1 = theScreenY2;
      continue;
    }

    double v1x = theScreenX2 - theScreenX1;
    double v1y = theScreenY2 - theScreenY1;

    double v1 = v1x*v1x + v1y*v1y;

    double v2x = inScreenX - theScreenX1;
    double v2y = inScreenY - theScreenY1;

    double prod = v1x*v2x + v1y*v2y;

    if (v1 == 0.)
    {
      theX1 = theX2;
      theY1 = theY2;
      theScreenX1 = theScreenX2;
      theScreenY1 = theScreenY2;
      continue;
    }

    double p1 = prod / v1;

    if (p1<0. || p1>1.)
    {
      theX1 = theX2;
      theY1 = theY2;
      theScreenX1 = theScreenX2;
      theScreenY1 = theScreenY2;
      continue;
    }

    double px = theScreenX1 + (theScreenX2 - theScreenX1)*p1;
    double py = theScreenY1 + (theScreenY2 - theScreenY1)*p1;

    double d = sqrt((inScreenX - px)*(inScreenX - px) + (inScreenY - py)*(inScreenY - py));

    if (!found || fabs(d) < lowestDist)
    {
      lowestDist = fabs(d);
      found_Id = i;
      found_x1 = theX1;
      found_x2 = theX2;
      found_y1 = theY1;
      found_y2 = theY2;
      found = true;
    }

    theX1 = theX2;
    theY1 = theY2;
    theScreenX1 = theScreenX2;
    theScreenY1 = theScreenY2;
  }

  if (found && lowestDist < inScreenTolerance)
  {
    outSampleIndex1 = found_Id;
    outSampleIndex2 = found_Id + 1;
    outX1 = found_x1;
    outY1 = found_y1;
    outX2 = found_x2;
    outY2 = found_y2;
    return true;
  }

  return false;
}

bool iupPlotDataSet::SelectSamples(double inMinX, double inMaxX, double inMinY, double inMaxY, const iupPlotSampleNotify* inNotify)
{
  bool theChanged = false;
  mHasSelected = false;

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    bool theSelected = mSelection->GetSampleBool(i);

    if (theX >= inMinX && theX <= inMaxX &&
        theY >= inMinY && theY <= inMaxY)
    {
      mHasSelected = true;

      if (!theSelected)
      {
        if (inNotify->cb)
        {
          int ret = inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)theSelected);
          if (ret == IUP_IGNORE)
            continue;
        }

        theChanged = true;
        mSelection->SetSampleBool(i, true);
      }
    }
    else
    {
      if (theSelected)
      {
        if (inNotify->cb)
        {
          int ret = inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)theSelected);
          if (ret == IUP_IGNORE)
            continue;
        }

        theChanged = true;
        mSelection->SetSampleBool(i, false);
      }
    }
  }

  return theChanged;
}

bool iupPlotDataSet::ClearSelection(const iupPlotSampleNotify* inNotify)
{
  bool theChanged = false;

  if (!mHasSelected)
    return theChanged;

  mHasSelected = false;

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
  {
    bool theSelected = mSelection->GetSampleBool(i);
    if (theSelected)
    {
      if (inNotify->cb)
      {
        double theX = mDataX->GetSample(i);
        double theY = mDataY->GetSample(i);
        int ret = inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)theSelected);
        if (ret == IUP_IGNORE)
          continue;
      }

      theChanged = true;
      mSelection->SetSampleBool(i, false);
    }
  }

  return theChanged;
}

bool iupPlotDataSet::DeleteSelectedSamples(const iupPlotSampleNotify* inNotify)
{
  bool theChanged = false;

  if (!mHasSelected)
    return theChanged;

  mHasSelected = false;

  int theCount = mDataX->GetCount();
  for (int i = theCount - 1; i >= 0; i--)
  {
    bool theSelected = mSelection->GetSampleBool(i);
    if (theSelected)
    {
      if (inNotify->cb)
      {
        double theX = mDataX->GetSample(i);
        double theY = mDataY->GetSample(i);
        int ret = inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)theSelected);
        if (ret == IUP_IGNORE)
          continue;
      }

      theChanged = true;
      RemoveSample(i);
    }
  }

  return theChanged;
}

int iupPlotDataSet::GetCount()
{
  return mDataX->GetCount();
}

void iupPlotDataSet::AddSample(double inX, double inY)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  theXData->AddSample(inX);
  theYData->AddSample(inY);
  mSelection->AddSample(false);
  if (mSegment)
    mSegment->AddSample(false);
  if (mExtra)
    mExtra->AddSample(0);
}

void iupPlotDataSet::InsertSample(int inSampleIndex, double inX, double inY)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  theXData->InsertSample(inSampleIndex, inX);
  theYData->InsertSample(inSampleIndex, inY);
  mSelection->InsertSample(inSampleIndex, false);
  if (mSegment)
    mSegment->InsertSample(inSampleIndex, false);
  if (mExtra)
    mExtra->InsertSample(inSampleIndex, 0);
}

void iupPlotDataSet::InitSegment()
{
  mSegment = new iupPlotDataBool();

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
    mSegment->AddSample(false);
}

void iupPlotDataSet::InitExtra()
{
  mExtra = new iupPlotDataReal();

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
    mExtra->AddSample(0);
}

void iupPlotDataSet::AddSampleSegment(double inX, double inY, bool inSegment)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  if (!mSegment)
    InitSegment();

  theXData->AddSample(inX);
  theYData->AddSample(inY);
  mSelection->AddSample(false);
  mSegment->AddSample(inSegment);
  if (mExtra)
    mExtra->AddSample(0);
}

void iupPlotDataSet::InsertSampleSegment(int inSampleIndex, double inX, double inY, bool inSegment)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  if (!mSegment)
    InitSegment();

  theXData->InsertSample(inSampleIndex, inX);
  theYData->InsertSample(inSampleIndex, inY);
  mSelection->InsertSample(inSampleIndex, false);
  mSegment->InsertSample(inSampleIndex, inSegment);
  if (mExtra)
    mExtra->InsertSample(inSampleIndex, 0);
}

void iupPlotDataSet::AddSample(const char* inX, double inY)
{
  iupPlotDataString *theXData = (iupPlotDataString*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (!theXData->IsString())
    return;

  theXData->AddSample(inX);
  theYData->AddSample(inY);
  mSelection->AddSample(false);
  if (mSegment)
    mSegment->AddSample(false);
  if (mExtra)
    mExtra->AddSample(0);
}

void iupPlotDataSet::InsertSample(int inSampleIndex, const char* inX, double inY)
{
  iupPlotDataString *theXData = (iupPlotDataString*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (!theXData->IsString())
    return;

  theXData->InsertSample(inSampleIndex, inX);
  theYData->InsertSample(inSampleIndex, inY);
  mSelection->InsertSample(inSampleIndex, false);
  if (mSegment)
    mSegment->InsertSample(inSampleIndex, false);
  if (mExtra)
    mExtra->InsertSample(inSampleIndex, 0);
}

void iupPlotDataSet::RemoveSample(int inSampleIndex)
{
  mDataX->RemoveSample(inSampleIndex);
  mDataY->RemoveSample(inSampleIndex);
  mSelection->RemoveSample(inSampleIndex);
  if (mSegment)
    mSegment->RemoveSample(inSampleIndex);
  if (mExtra)
    mExtra->RemoveSample(inSampleIndex);
}

void iupPlotDataSet::GetSample(int inSampleIndex, double *inX, double *inY)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  int theCount = theXData->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  if (inX) *inX = theXData->GetSample(inSampleIndex);
  if (inY) *inY = theYData->GetSample(inSampleIndex);
}

void iupPlotDataSet::GetSample(int inSampleIndex, const char* *inX, double *inY)
{
  iupPlotDataString *theXData = (iupPlotDataString*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (!theXData->IsString())
    return;

  int theCount = theXData->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  if (inX) *inX = theXData->GetSampleString(inSampleIndex);
  if (inY) *inY = theYData->GetSample(inSampleIndex);
}

bool iupPlotDataSet::GetSampleSelection(int inSampleIndex)
{
  int theCount = mDataX->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return false;

  return mSelection->GetSampleBool(inSampleIndex);
}

double iupPlotDataSet::GetSampleExtra(int inSampleIndex)
{
  int theCount = mDataX->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount || !mExtra)
    return 0;

  return mExtra->GetSample(inSampleIndex);
}

void iupPlotDataSet::SetSample(int inSampleIndex, double inX, double inY)
{
  iupPlotDataReal *theXData = (iupPlotDataReal*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (theXData->IsString())
    return;

  int theCount = theXData->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  theXData->SetSample(inSampleIndex, inX);
  theYData->SetSample(inSampleIndex, inY);
}

void iupPlotDataSet::SetSample(int inSampleIndex, const char* inX, double inY)
{
  iupPlotDataString *theXData = (iupPlotDataString*)mDataX;
  iupPlotDataReal *theYData = (iupPlotDataReal*)mDataY;

  if (!theXData->IsString())
    return;

  int theCount = theXData->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  theXData->SetSampleString(inSampleIndex, inX);
  theYData->SetSample(inSampleIndex, inY);
}

void iupPlotDataSet::SetSampleSelection(int inSampleIndex, bool inSelected)
{
  int theCount = mDataX->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  mSelection->SetSampleBool(inSampleIndex, inSelected);
}

void iupPlotDataSet::SetSampleExtra(int inSampleIndex, double inExtra)
{
  int theCount = mDataX->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  if (!mExtra)
    InitExtra();

  mExtra->SetSample(inSampleIndex, inExtra);
}


/************************************************************************************/


#define HIGHLIGHT_ALPHA 64
#define HIGHLIGHT_OFFSET 12
#define SELECT_ALPHA 128
#define SELECT_OFFSET 8


static void iPlotDrawHighlightedBar(iupPlotDrawContext* ctx, int x1, int y1, int x2, int y2, long color, int lineWidth)
{
  long hlColor = iupDrawColor(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), HIGHLIGHT_ALPHA);
  iupdrvDrawRectangle(ctx->dc, x1, y1, x2, y2, hlColor, IUP_DRAW_STROKE, lineWidth + HIGHLIGHT_OFFSET);
}

static void iPlotDrawHighlightedStem(iupPlotDrawContext* ctx, int x1, int y1, int x2, int y2, long color, int markSize, int lineWidth)
{
  long hlColor = iupDrawColor(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), HIGHLIGHT_ALPHA);
  iupdrvDrawLine(ctx->dc, x1, y1, x2, y2, hlColor, IUP_DRAW_STROKE, lineWidth + HIGHLIGHT_OFFSET);
  iupPlotDrawMark(ctx, x2, y2, IUP_PLOT_MARK_CIRCLE, markSize + HIGHLIGHT_OFFSET, hlColor);
}

static void iPlotDrawHighlightedMark(iupPlotDrawContext* ctx, int x, int y, long color, int markSize)
{
  long hlColor = iupDrawColor(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), HIGHLIGHT_ALPHA);
  iupPlotDrawMark(ctx, x, y, IUP_PLOT_MARK_CIRCLE, markSize + HIGHLIGHT_OFFSET, hlColor);
}

static void iPlotDrawHighlightedArc(iupPlotDrawContext* ctx, int x1, int y1, int x2, int y2, double startAngle, double endAngle, long color, int lineWidth)
{
  long hlColor = iupDrawColor(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), HIGHLIGHT_ALPHA);
  iupdrvDrawArc(ctx->dc, x1, y1, x2, y2, startAngle, endAngle, hlColor, IUP_DRAW_STROKE, lineWidth + HIGHLIGHT_OFFSET);
}

static void iPlotDrawHighlightedCurve(iupPlotDrawContext* ctx, int inCount, const iupPlotData* inDataX, const iupPlotData* inDataY, const iupPlotDataBool* inSegment,
                                      const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, long color, int lineWidth, bool inConnectPreviousX, bool inSelected = false)
{
  long hlColor = iupDrawColor(iupDrawRed(color), iupDrawGreen(color), iupDrawBlue(color), inSelected ? SELECT_ALPHA : HIGHLIGHT_ALPHA);
  int hlWidth = lineWidth + (inSelected ? SELECT_OFFSET : HIGHLIGHT_OFFSET);

  iupPlotPointBuffer buf;
  buf.Begin(inCount * 2);

  for (int i = 0; i < inCount; i++)
  {
    double theX = inDataX->GetSample(i);
    double theY = inDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);
    int absX = iupPlotDrawCalcX(ctx, theScreenX);
    int absY = iupPlotDrawCalcY(ctx, theScreenY);

    if (i > 0 && inSegment && inSegment->GetSampleBool(i))
    {
      buf.DrawStroke(ctx, hlColor, IUP_PLOT_LINE_CONTINUOUS, hlWidth);
      buf.Clear();
      buf.Begin(inCount - i);
    }

    if (inConnectPreviousX && i > 0)
    {
      int prevAbsX = iupPlotDrawCalcX(ctx, inTrafoX->Transform(inDataX->GetSample(i - 1)));
      buf.AddVertex(prevAbsX, absY);
    }

    buf.AddVertex(absX, absY);
  }

  buf.DrawStroke(ctx, hlColor, IUP_PLOT_LINE_CONTINUOUS, hlWidth);
}

void iupPlotDataSet::DrawDataLine(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify, bool inShowMark, bool inErrorBar) const
{
  int theCount = mDataX->GetCount();
  int drawStyle = iupPlotLineStyleToDrawStyle(mLineStyle);

  iupPlotPointBuffer buf;
  buf.Begin(theCount);

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);
    int absX = iupPlotDrawCalcX(ctx, theScreenX);
    int absY = iupPlotDrawCalcY(ctx, theScreenY);

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (inShowMark)
    {
      int theMarkSize = mMarkSize;
      if (mExtra)
      {
        if (inErrorBar)
          DrawErrorBar(inTrafoY, ctx, i, theY, theScreenX);
        else
          SetSampleExtraMarkSize(inTrafoY, i, &theMarkSize);
      }

      iupPlotDrawMark(ctx, absX, absY, mMarkStyle, theMarkSize, mColor);
    }

    if (i == mHighlightedSample)
      iPlotDrawHighlightedMark(ctx, absX, absY, mColor, mMarkSize);

    if (i > 0 && mSegment && mSegment->GetSampleBool(i))
    {
      buf.DrawStroke(ctx, mColor, mLineStyle, mLineWidth);
      buf.Clear();
      buf.Begin(theCount - i);
    }

    buf.AddVertex(absX, absY);
  }

  buf.DrawStroke(ctx, mColor, mLineStyle, mLineWidth);

  if (mHighlightedCurve)
    iPlotDrawHighlightedCurve(ctx, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, mColor, mLineWidth, false);
  else if (mSelectedCurve)
    iPlotDrawHighlightedCurve(ctx, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, mColor, mLineWidth, false, true);
}

void iupPlotDataSet::DrawErrorBar(const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, int index, double theY, double theScreenX) const
{
  double theError = mExtra->GetSample(index);
  double theScreenErrorY1 = inTrafoY->Transform(theY - theError);
  double theScreenErrorY2 = inTrafoY->Transform(theY + theError);
  double theBarWidth = (double)mMarkSize;

  int absX = iupPlotDrawCalcX(ctx, theScreenX);
  int absY1 = iupPlotDrawCalcY(ctx, theScreenErrorY1);
  int absY2 = iupPlotDrawCalcY(ctx, theScreenErrorY2);
  int absXL = iupPlotDrawCalcX(ctx, theScreenX - theBarWidth);
  int absXR = iupPlotDrawCalcX(ctx, theScreenX + theBarWidth);

  iupdrvDrawLine(ctx->dc, absX, absY1, absX, absY2, mColor, IUP_DRAW_STROKE, mLineWidth);
  iupdrvDrawLine(ctx->dc, absXL, absY1, absXR, absY1, mColor, IUP_DRAW_STROKE, mLineWidth);
  iupdrvDrawLine(ctx->dc, absXL, absY2, absXR, absY2, mColor, IUP_DRAW_STROKE, mLineWidth);
}

void iupPlotDataSet::SetSampleExtraMarkSize(const iupPlotTrafo *inTrafoY, int inSampleIndex, int *outMarkSize) const
{
  double theMarkSize = mExtra->GetSample(inSampleIndex);
  int theScreenSize = 1;
  if (theMarkSize != 0)
    theScreenSize = iupPlotRound(inTrafoY->Transform(theMarkSize));
  if (theScreenSize < 1) theScreenSize = 1;

  *outMarkSize = theScreenSize;
}

void iupPlotDataSet::DrawDataMark(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);
    int absX = iupPlotDrawCalcX(ctx, theScreenX);
    int absY = iupPlotDrawCalcY(ctx, theScreenY);

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    int theMarkSize = mMarkSize;
    if (mExtra)
      SetSampleExtraMarkSize(inTrafoY, i, &theMarkSize);

    iupPlotDrawMark(ctx, absX, absY, mMarkStyle, theMarkSize, mColor);

    if (i == mHighlightedSample)
      iPlotDrawHighlightedMark(ctx, absX, absY, mColor, mMarkSize);
  }
}

void iupPlotDataSet::DrawDataStem(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify, bool inShowMark) const
{
  double theScreenY0 = inTrafoY->Transform(0);
  int absY0 = iupPlotDrawCalcY(ctx, theScreenY0);

  int theCount = mDataX->GetCount();
  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);
    int absX = iupPlotDrawCalcX(ctx, theScreenX);
    int absY = iupPlotDrawCalcY(ctx, theScreenY);

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (inShowMark)
    {
      int theMarkSize = mMarkSize;
      if (mExtra)
        SetSampleExtraMarkSize(inTrafoY, i, &theMarkSize);

      iupPlotDrawMark(ctx, absX, absY, mMarkStyle, theMarkSize, mColor);
    }

    iupdrvDrawLine(ctx->dc, absX, absY0, absX, absY, mColor, IUP_DRAW_STROKE, mLineWidth);

    if (i == mHighlightedSample)
      iPlotDrawHighlightedStem(ctx, absX, absY0, absX, absY, mColor, mMarkSize, mLineWidth);
  }
}

void iupPlotDataSet::DrawDataArea(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  double theScreenY0 = inTrafoY->Transform(0);

  long fillColor = mColor;
  if (mAreaTransparency != 255)
    fillColor = iupDrawColor(iupDrawRed(mColor), iupDrawGreen(mColor), iupDrawBlue(mColor), mAreaTransparency);

  iupPlotPointBuffer fillBuf;
  fillBuf.Begin(theCount + 2);

  double theFirstScreenX = 0, theLastScreenX = 0;

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);
    int absX = iupPlotDrawCalcX(ctx, theScreenX);
    int absY = iupPlotDrawCalcY(ctx, theScreenY);

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (i == 0)
    {
      theFirstScreenX = theScreenX;
      fillBuf.AddVertex(iupPlotDrawCalcX(ctx, theScreenX), iupPlotDrawCalcY(ctx, theScreenY0));
    }

    if (i > 0 && mSegment && mSegment->GetSampleBool(i))
    {
      fillBuf.AddVertex(iupPlotDrawCalcX(ctx, theLastScreenX), iupPlotDrawCalcY(ctx, theScreenY0));
      fillBuf.DrawFill(ctx, fillColor);
      fillBuf.Clear();
      fillBuf.Begin(theCount - i + 2);
      fillBuf.AddVertex(iupPlotDrawCalcX(ctx, theScreenX), iupPlotDrawCalcY(ctx, theScreenY0));
    }

    fillBuf.AddVertex(absX, absY);

    if (i == mHighlightedSample)
      iPlotDrawHighlightedMark(ctx, absX, absY, mColor, mMarkSize);

    theLastScreenX = theScreenX;
  }

  fillBuf.AddVertex(iupPlotDrawCalcX(ctx, theLastScreenX), iupPlotDrawCalcY(ctx, theScreenY0));
  fillBuf.DrawFill(ctx, fillColor);

  if (mAreaTransparency != 255)
  {
    iupPlotPointBuffer lineBuf;
    lineBuf.Begin(theCount);

    for (int i = 0; i < theCount; i++)
    {
      double theX = mDataX->GetSample(i);
      double theY = mDataY->GetSample(i);
      double theScreenX = inTrafoX->Transform(theX);
      double theScreenY = inTrafoY->Transform(theY);

      if (i > 0 && mSegment && mSegment->GetSampleBool(i))
      {
        lineBuf.DrawStroke(ctx, mColor, IUP_PLOT_LINE_CONTINUOUS, mLineWidth);
        lineBuf.Clear();
        lineBuf.Begin(theCount - i);
      }

      lineBuf.AddVertex(iupPlotDrawCalcX(ctx, theScreenX), iupPlotDrawCalcY(ctx, theScreenY));
    }

    lineBuf.DrawStroke(ctx, mColor, IUP_PLOT_LINE_CONTINUOUS, mLineWidth);
  }

  if (mHighlightedCurve)
    iPlotDrawHighlightedCurve(ctx, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, mColor, mLineWidth, false);
  else if (mSelectedCurve)
    iPlotDrawHighlightedCurve(ctx, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, mColor, mLineWidth, false, true);
}

static void iPlotDrawBarRect(iupPlotDrawContext* ctx, double x, double y, double barWidth, double barHeight, long color, int style, int lineWidth)
{
  int x1, y1, x2, y2;
  if (barHeight > 0)
  {
    x1 = iupPlotDrawCalcX(ctx, x);
    y1 = iupPlotDrawCalcY(ctx, y + barHeight);
    x2 = iupPlotDrawCalcX(ctx, x + barWidth);
    y2 = iupPlotDrawCalcY(ctx, y);
  }
  else
  {
    x1 = iupPlotDrawCalcX(ctx, x);
    y1 = iupPlotDrawCalcY(ctx, y);
    x2 = iupPlotDrawCalcX(ctx, x + barWidth);
    y2 = iupPlotDrawCalcY(ctx, y + barHeight);
  }
  iupdrvDrawRectangle(ctx->dc, x1, y1, x2, y2, color, style, lineWidth);
}

void iupPlotDataSet::DrawDataBar(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  double theScreenY0 = inTrafoY->Transform(0);

  double theMinX = mDataX->GetSample(0);
  double theScreenMinX = inTrafoX->Transform(theMinX);
  double theMaxX = mDataX->GetSample(theCount - 1);
  double theScreenMaxX = inTrafoX->Transform(theMaxX);

  double theBarWidth = (theScreenMaxX - theScreenMinX) / (theCount - 1);
  theBarWidth *= 1 - (double)mBarSpacingPercent / 100.0;

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    double theBarX = theScreenX - theBarWidth / 2;
    double theBarHeight = theScreenY - theScreenY0;

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (theBarHeight == 0)
      continue;

    long drawColor = mColor;
    if (mBarMulticolor)
      drawColor = iupPlotDrawGetSampleColorTable(inNotify->ih, i);

    iPlotDrawBarRect(ctx, theBarX, theScreenY0, theBarWidth, theBarHeight, drawColor, IUP_DRAW_FILL, 1);

    if (mBarShowOutline)
      iPlotDrawBarRect(ctx, theBarX, theScreenY0, theBarWidth, theBarHeight, mBarOutlineColor, IUP_DRAW_STROKE, mLineWidth);

    if (i == mHighlightedSample)
    {
      int bx1, by1, bx2, by2;
      if (theBarHeight > 0) { bx1 = iupPlotDrawCalcX(ctx, theBarX); by1 = iupPlotDrawCalcY(ctx, theScreenY0 + theBarHeight); bx2 = iupPlotDrawCalcX(ctx, theBarX + theBarWidth); by2 = iupPlotDrawCalcY(ctx, theScreenY0); }
      else { bx1 = iupPlotDrawCalcX(ctx, theBarX); by1 = iupPlotDrawCalcY(ctx, theScreenY0); bx2 = iupPlotDrawCalcX(ctx, theBarX + theBarWidth); by2 = iupPlotDrawCalcY(ctx, theScreenY0 + theBarHeight); }
      iPlotDrawHighlightedBar(ctx, bx1, by1, bx2, by2, drawColor, mLineWidth);
    }
  }
}

void iupPlotDataSet::DrawDataHorizontalBar(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  double theScreenX0 = inTrafoX->Transform(0);

  double theMinY = mDataY->GetSample(0);
  double theScreenMinY = inTrafoY->Transform(theMinY);
  double theMaxY = mDataY->GetSample(theCount - 1);
  double theScreenMaxY = inTrafoY->Transform(theMaxY);

  double theBarHeight = (theScreenMaxY - theScreenMinY) / (theCount - 1);
  theBarHeight *= 1 - (double)mBarSpacingPercent / 100.0;

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    double theBarY = theScreenY - theBarHeight / 2;
    double theBarWidth = theScreenX - theScreenX0;

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (theBarWidth == 0)
      continue;

    long drawColor = mColor;
    if (mBarMulticolor)
      drawColor = iupPlotDrawGetSampleColorTable(inNotify->ih, i);

    iPlotDrawBarRect(ctx, theScreenX0, theBarY, theBarWidth, theBarHeight, drawColor, IUP_DRAW_FILL, 1);

    if (mBarShowOutline)
      iPlotDrawBarRect(ctx, theScreenX0, theBarY, theBarWidth, theBarHeight, mBarOutlineColor, IUP_DRAW_STROKE, mLineWidth);

    if (i == mHighlightedSample)
    {
      int bx1, by1, bx2, by2;
      if (theBarWidth > 0) { bx1 = iupPlotDrawCalcX(ctx, theScreenX0); by1 = iupPlotDrawCalcY(ctx, theBarY + theBarHeight); bx2 = iupPlotDrawCalcX(ctx, theScreenX0 + theBarWidth); by2 = iupPlotDrawCalcY(ctx, theBarY); }
      else { bx1 = iupPlotDrawCalcX(ctx, theScreenX0 + theBarWidth); by1 = iupPlotDrawCalcY(ctx, theBarY + theBarHeight); bx2 = iupPlotDrawCalcX(ctx, theScreenX0); by2 = iupPlotDrawCalcY(ctx, theBarY); }
      iPlotDrawHighlightedBar(ctx, bx1, by1, bx2, by2, drawColor, mLineWidth);
    }
  }
}

void iupPlotDataSet::DrawDataMultiBar(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();
  double theScreenY0 = inTrafoY->Transform(0);

  double theMinX = mDataX->GetSample(0);
  double theScreenMinX = inTrafoX->Transform(theMinX);
  double theMaxX = mDataX->GetSample(theCount - 1);
  double theScreenMaxX = inTrafoX->Transform(theMaxX);

  double theTotalBarWidth = (theScreenMaxX - theScreenMinX) / (theCount - 1);
  theTotalBarWidth *= 1 - (double)mBarSpacingPercent / 100.0;
  double theBarWidth = theTotalBarWidth / mMultibarCount;

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);

    double theBarX = (theScreenX - theTotalBarWidth / 2) + (mMultibarIndex*theBarWidth);
    double theBarHeight = theScreenY - theScreenY0;

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (theBarHeight == 0)
      continue;

    iPlotDrawBarRect(ctx, theBarX, theScreenY0, theBarWidth, theBarHeight, mColor, IUP_DRAW_FILL, 1);

    if (mBarShowOutline)
      iPlotDrawBarRect(ctx, theBarX, theScreenY0, theBarWidth, theBarHeight, mBarOutlineColor, IUP_DRAW_STROKE, mLineWidth);

    if (i == mHighlightedSample)
    {
      int bx1, by1, bx2, by2;
      if (theBarHeight > 0) { bx1 = iupPlotDrawCalcX(ctx, theBarX); by1 = iupPlotDrawCalcY(ctx, theScreenY0 + theBarHeight); bx2 = iupPlotDrawCalcX(ctx, theBarX + theBarWidth); by2 = iupPlotDrawCalcY(ctx, theScreenY0); }
      else { bx1 = iupPlotDrawCalcX(ctx, theBarX); by1 = iupPlotDrawCalcY(ctx, theScreenY0); bx2 = iupPlotDrawCalcX(ctx, theBarX + theBarWidth); by2 = iupPlotDrawCalcY(ctx, theScreenY0 + theBarHeight); }
      iPlotDrawHighlightedBar(ctx, bx1, by1, bx2, by2, mColor, mLineWidth);
    }
  }
}

void iupPlotDataSet::DrawDataStep(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();

  iupPlotPointBuffer buf;
  buf.Begin(theCount * 2);

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);
    double theScreenX = inTrafoX->Transform(theX);
    double theScreenY = inTrafoY->Transform(theY);
    int absX = iupPlotDrawCalcX(ctx, theScreenX);
    int absY = iupPlotDrawCalcY(ctx, theScreenY);

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    if (i > 0 && mSegment && mSegment->GetSampleBool(i))
    {
      buf.DrawStroke(ctx, mColor, mLineStyle, mLineWidth);
      buf.Clear();
      buf.Begin(theCount - i);
    }

    if (i > 0)
    {
      int prevAbsX = iupPlotDrawCalcX(ctx, inTrafoX->Transform(mDataX->GetSample(i - 1)));
      buf.AddVertex(prevAbsX, absY);
    }

    buf.AddVertex(absX, absY);

    if (i == mHighlightedSample)
      iPlotDrawHighlightedMark(ctx, absX, absY, mColor, mMarkSize);
  }

  buf.DrawStroke(ctx, mColor, mLineStyle, mLineWidth);

  if (mHighlightedCurve)
    iPlotDrawHighlightedCurve(ctx, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, mColor, mLineWidth, true);
  else if (mSelectedCurve)
    iPlotDrawHighlightedCurve(ctx, theCount, mDataX, mDataY, mSegment, inTrafoX, inTrafoY, mColor, mLineWidth, true, true);
}

static int iPlotGetPieTextAlignment(double bisectrix, double inPieSliceLabelPos)
{
  if (inPieSliceLabelPos < 0)
    bisectrix += 180;

  bisectrix = fmod(bisectrix, 360);

  if (bisectrix < 22.5)       return IUP_PLOT_EAST;
  else if (bisectrix < 67.5)  return IUP_PLOT_NORTH_EAST;
  else if (bisectrix < 112.5) return IUP_PLOT_NORTH;
  else if (bisectrix < 157.5) return IUP_PLOT_NORTH_WEST;
  else if (bisectrix < 202.5) return IUP_PLOT_WEST;
  else if (bisectrix < 247.5) return IUP_PLOT_SOUTH_WEST;
  else if (bisectrix < 292.5) return IUP_PLOT_SOUTH;
  else if (bisectrix < 337.5) return IUP_PLOT_SOUTH_EAST;
  else                        return IUP_PLOT_EAST;
}

void iupPlotDataSet::DrawDataPie(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify, const iupPlotAxis& inAxisY, long inBackColor) const
{
  int theXCount = mDataX->GetCount();
  int theYCount = mDataY->GetCount();

  if ((theXCount == 0) || (theYCount == 0))
    return;

  if (theXCount != theYCount)
    return;

  double xc, yc, w, h;

  int theCount = mDataX->GetCount();
  double sum = 0;

  for (int i = 0; i < theCount; i++)
  {
    double theY = mDataY->GetSample(i);
    if (theY <= 0)
      continue;
    sum += theY;
  }

  xc = inTrafoX->Transform(0);
  yc = inTrafoY->Transform(0);

  w = 2.0 * mPieRadius;
  h = 2.0 * mPieRadius;
  w *= ((iupPlotTrafoLinear *)inTrafoX)->mSlope;
  h *= ((iupPlotTrafoLinear *)inTrafoY)->mSlope;

  double w1 = 2.0 * (mPieRadius * 1.01);
  double h1 = 2.0 * (mPieRadius * 1.01);
  w1 *= ((iupPlotTrafoLinear *)inTrafoX)->mSlope;
  h1 *= ((iupPlotTrafoLinear *)inTrafoY)->mSlope;

  double startAngle = mPieStartAngle;

  char fontStr[256];
  if (mPieSliceLabel != IUP_PLOT_NONE)
  {
    inAxisY.SetFont(ctx->ih, inAxisY.mFontStyle, inAxisY.mFontSize);
    int fontStyle = inAxisY.mFontStyle == -1 ? inAxisY.mDefaultFontStyle : inAxisY.mFontStyle;
    int fontSize = inAxisY.mFontSize == 0 ? inAxisY.mDefaultFontSize : inAxisY.mFontSize;
    iupPlotBuildFont(ctx->ih, fontStyle, fontSize, fontStr, sizeof(fontStr));
  }

  for (int i = 0; i < theCount; i++)
  {
    double theX = mDataX->GetSample(i);
    double theY = mDataY->GetSample(i);

    if (theY <= 0)
      continue;

    double angle = (theY * 360.) / sum;

    if (inNotify->cb)
      inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));

    long sampleColor = iupPlotDrawGetSampleColorTable(inNotify->ih, i);

    int ax1 = iupPlotDrawCalcX(ctx, xc - w / 2);
    int ay1 = iupPlotDrawCalcY(ctx, yc + h / 2);
    int ax2 = iupPlotDrawCalcX(ctx, xc + w / 2);
    int ay2 = iupPlotDrawCalcY(ctx, yc - h / 2);
    iupdrvDrawArc(ctx->dc, ax1, ay1, ax2, ay2, startAngle, startAngle + angle, sampleColor, IUP_DRAW_FILL, 1);

    if (mPieContour)
      iupdrvDrawArc(ctx->dc, ax1, ay1, ax2, ay2, startAngle, startAngle + angle, mColor, IUP_DRAW_STROKE, mLineWidth);

    if (i == mHighlightedSample)
    {
      int hx1 = iupPlotDrawCalcX(ctx, xc - w1 / 2);
      int hy1 = iupPlotDrawCalcY(ctx, yc + h1 / 2);
      int hx2 = iupPlotDrawCalcX(ctx, xc + w1 / 2);
      int hy2 = iupPlotDrawCalcY(ctx, yc - h1 / 2);
      iPlotDrawHighlightedArc(ctx, hx1, hy1, hx2, hy2, startAngle, startAngle + angle, sampleColor, mLineWidth);
    }

    if (mPieSliceLabel != IUP_PLOT_NONE)
    {
      double bisectrix = (startAngle + startAngle + angle) / 2;
      int text_alignment = iPlotGetPieTextAlignment(bisectrix, mPieSliceLabelPos);

      double px = xc + (((w / 2.) * fabs(mPieSliceLabelPos)) * cos(bisectrix * IUP_PLOT_DEG2RAD));
      double py = yc + (((h / 2.) * fabs(mPieSliceLabelPos)) * sin(bisectrix * IUP_PLOT_DEG2RAD));

      char theBuf[128];
      switch (mPieSliceLabel)
      {
      case IUP_PLOT_X:
        if (mDataX->IsString())
          iupPlotDrawAlignedText(ctx, px, py, text_alignment, ((iupPlotDataString *)mDataX)->GetSampleString(i), inAxisY.mColor, fontStr, 0);
        else
        {
          snprintf(theBuf, sizeof(theBuf), "%d", i);
          iupPlotDrawAlignedText(ctx, px, py, text_alignment, theBuf, inAxisY.mColor, fontStr, 0);
        }
        break;
      case IUP_PLOT_Y:
        iupStrPrintfDoubleLocale(theBuf, inAxisY.mTick.mFormatString, theY, IupGetGlobal("DEFAULTDECIMALSYMBOL"));
        iupPlotDrawAlignedText(ctx, px, py, text_alignment, theBuf, inAxisY.mColor, fontStr, 0);
        break;
      case IUP_PLOT_PERCENT:
      {
        double percent = (theY * 100.) / sum;
        iupStrPrintfDoubleLocale(theBuf, inAxisY.mTick.mFormatString, percent, IupGetGlobal("DEFAULTDECIMALSYMBOL"));
        strcat(theBuf, " %");
        iupPlotDrawAlignedText(ctx, px, py, text_alignment, theBuf, inAxisY.mColor, fontStr, 0);
        break;
      }
      default:
        break;
      }
    }

    startAngle += angle;
  }

  if (mPieHole > 0)
  {
    double hw = mPieHole * 2.0 * mPieRadius;
    double hh = mPieHole * 2.0 * mPieRadius;
    hw *= ((iupPlotTrafoLinear *)inTrafoX)->mSlope;
    hh *= ((iupPlotTrafoLinear *)inTrafoY)->mSlope;

    int hx1 = iupPlotDrawCalcX(ctx, xc - hw / 2);
    int hy1 = iupPlotDrawCalcY(ctx, yc + hh / 2);
    int hx2 = iupPlotDrawCalcX(ctx, xc + hw / 2);
    int hy2 = iupPlotDrawCalcY(ctx, yc - hh / 2);
    iupdrvDrawArc(ctx->dc, hx1, hy1, hx2, hy2, 0, 360, inBackColor, IUP_DRAW_FILL, 1);

    if (mPieContour)
      iupdrvDrawArc(ctx->dc, hx1, hy1, hx2, hy2, 0, 360, mColor, IUP_DRAW_STROKE, mLineWidth);
  }
}

void iupPlotDataSet::DrawSelection(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const
{
  int theCount = mDataX->GetCount();

  for (int i = 0; i < theCount; i++)
  {
    if (mSelection->GetSampleBool(i))
    {
      double theX = mDataX->GetSample(i);
      double theY = mDataY->GetSample(i);
      double theScreenX = inTrafoX->Transform(theX);
      double theScreenY = inTrafoY->Transform(theY);
      int absX = iupPlotDrawCalcX(ctx, theScreenX);
      int absY = iupPlotDrawCalcY(ctx, theScreenY);

      if (inNotify->cb)
      {
        int ret = inNotify->cb(inNotify->ih, inNotify->ds, i, theX, theY, (int)mSelection->GetSampleBool(i));
        if (ret == IUP_IGNORE)
          continue;
      }

      iupPlotDrawMark(ctx, absX, absY, IUP_PLOT_MARK_BOX, 7, iupDrawColor(128, 128, 128, 128));
      iupPlotDrawMark(ctx, absX, absY, IUP_PLOT_MARK_HOLLOW_BOX, 7, iupDrawColor(0, 0, 0, 128));
    }
  }
}

void iupPlotDataSet::DrawData(const iupPlotTrafo *inTrafoX, const iupPlotTrafo *inTrafoY, iupPlotDrawContext* ctx, const iupPlotSampleNotify* inNotify) const
{
  int theXCount = mDataX->GetCount();
  int theYCount = mDataY->GetCount();

  if ((theXCount == 0) || (theYCount == 0))
    return;

  if (theXCount != theYCount)
    return;

  switch (mMode)
  {
  case IUP_PLOT_LINE:
    DrawDataLine(inTrafoX, inTrafoY, ctx, inNotify, false, false);
    break;
  case IUP_PLOT_MARK:
    DrawDataMark(inTrafoX, inTrafoY, ctx, inNotify);
    break;
  case IUP_PLOT_STEM:
    DrawDataStem(inTrafoX, inTrafoY, ctx, inNotify, false);
    break;
  case IUP_PLOT_MARKSTEM:
    DrawDataStem(inTrafoX, inTrafoY, ctx, inNotify, true);
    break;
  case IUP_PLOT_MARKLINE:
    DrawDataLine(inTrafoX, inTrafoY, ctx, inNotify, true, false);
    break;
  case IUP_PLOT_AREA:
    DrawDataArea(inTrafoX, inTrafoY, ctx, inNotify);
    break;
  case IUP_PLOT_BAR:
    DrawDataBar(inTrafoX, inTrafoY, ctx, inNotify);
    break;
  case IUP_PLOT_PIE:
    break;
  case IUP_PLOT_HORIZONTALBAR:
    DrawDataHorizontalBar(inTrafoX, inTrafoY, ctx, inNotify);
    break;
  case IUP_PLOT_MULTIBAR:
    DrawDataMultiBar(inTrafoX, inTrafoY, ctx, inNotify);
    break;
  case IUP_PLOT_ERRORBAR:
    DrawDataLine(inTrafoX, inTrafoY, ctx, inNotify, true, true);
    break;
  case IUP_PLOT_STEP:
    DrawDataStep(inTrafoX, inTrafoY, ctx, inNotify);
    break;
  }

  if (mHasSelected)
    DrawSelection(inTrafoX, inTrafoY, ctx, inNotify);
}

