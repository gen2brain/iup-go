/** \file
 * \brief Haiku Scrollbar Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <Message.h>
#include <Rect.h>
#include <ScrollBar.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_drv.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_str.h"
#include "iup_scrollbar.h"
}

#include "iuphaiku_drv.h"


/* BScrollBar internally roundf()s range/value/steps, so 0..1 collapses to 2 stops. */
#define ISCROLLBAR_RANGE 10000

class IupHaikuScrollBar : public BScrollBar
{
public:
  IupHaikuScrollBar(Ihandle* ih, orientation o)
    : BScrollBar(BRect(0, 0, 0, 0), "iup_scrollbar", NULL, 0.0f, 1.0f, o),
      fIhandle(ih), fSuppress(false), fMouseDown(false), fIsDragging(false) {}

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }
  void SetSuppress(bool s) { fSuppress = s; }

  void MouseDown(BPoint where) override
  {
    fMouseDown = true;
    fIsDragging = false;
    fLastWhere = where;
    BScrollBar::MouseDown(where);
  }

  void MouseMoved(BPoint where, uint32 code, const BMessage* drag) override
  {
    if (fMouseDown && where != fLastWhere) fIsDragging = true;
    BScrollBar::MouseMoved(where, code, drag);
  }

  void MouseUp(BPoint where) override
  {
    BScrollBar::MouseUp(where);
    /* Final SBPOSx after thumb drag so the user sees a commit op. */
    if (fIhandle && fIsDragging)
      firePosCallbacks();
    fMouseDown = false;
    fIsDragging = false;
  }

  void ValueChanged(float new_value) override
  {
    BScrollBar::ValueChanged(new_value);
    if (fSuppress || !fIhandle) return;
    if (!iupdrvIsActive(fIhandle)) return;

    Ihandle* ih = fIhandle;
    double range = ih->data->vmax - ih->data->vmin;
    if (range <= 0) return;

    int ipage = (int)((ih->data->pagesize / range) * ISCROLLBAR_RANGE);
    if (ipage < 1) ipage = 1;
    int imax = ISCROLLBAR_RANGE - ipage;
    if (imax < 0) imax = 0;

    int ipos = (int)new_value;
    if (ih->data->inverted) ipos = imax - ipos;
    if (ipos < 0) ipos = 0;
    if (ipos > imax) ipos = imax;

    double old_val = ih->data->val;
    ih->data->val = ((double)ipos / (double)ISCROLLBAR_RANGE) * range + ih->data->vmin;
    iupScrollbarCropValue(ih);

    double delta = ih->data->val - old_val;
    bool horiz = ih->data->orientation == ISCROLLBAR_HORIZONTAL;
    int op = horiz ? IUP_SBPOSH : IUP_SBPOSV;

    if (fIsDragging)
    {
      op = horiz ? IUP_SBDRAGH : IUP_SBDRAGV;
    }
    else if (fMouseDown)
    {
      double aline = delta < 0 ? -delta : delta;
      double sline = ih->data->linestep;
      double spage = ih->data->pagestep;
      double tol = (spage - sline) * 0.5;
      if (sline > 0 && aline < sline + tol)
        op = horiz ? (delta < 0 ? IUP_SBLEFT : IUP_SBRIGHT)
                   : (delta < 0 ? IUP_SBUP   : IUP_SBDN);
      else if (spage > 0 && aline < spage + tol)
        op = horiz ? (delta < 0 ? IUP_SBPGLEFT : IUP_SBPGRIGHT)
                   : (delta < 0 ? IUP_SBPGUP   : IUP_SBPGDN);
    }

    Icallback sc = IupGetCallback(ih, "SCROLL_CB");
    if (sc)
    {
      IFniff scroll_cb = (IFniff)sc;
      float posx = horiz ? (float)ih->data->val : 0;
      float posy = horiz ? 0 : (float)ih->data->val;
      scroll_cb(ih, op, posx, posy);
    }

    if (delta != 0.0)
    {
      Icallback vc = IupGetCallback(ih, "VALUECHANGED_CB");
      if (vc)
      {
        int ret = vc(ih);
        if (ret == IUP_CLOSE) IupExitLoop();
      }
    }
  }

private:
  Ihandle* fIhandle;
  bool fSuppress;
  bool fMouseDown;
  bool fIsDragging;
  BPoint fLastWhere;

  void firePosCallbacks()
  {
    Ihandle* ih = fIhandle;
    bool horiz = ih->data->orientation == ISCROLLBAR_HORIZONTAL;
    IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
    if (!scroll_cb) return;
    int op = horiz ? IUP_SBPOSH : IUP_SBPOSV;
    float posx = horiz ? (float)ih->data->val : 0;
    float posy = horiz ? 0 : (float)ih->data->val;
    scroll_cb(ih, op, posx, posy);
  }
};

/* Push ih->data state down to the native BScrollBar in scaled-int space. */
static void haikuSbUpdateNative(Ihandle* ih)
{
  IupHaikuScrollBar* sb = (IupHaikuScrollBar*)ih->handle;
  if (!sb) return;
  double range = ih->data->vmax - ih->data->vmin;
  if (range <= 0) return;

  int ipage = (int)((ih->data->pagesize / range) * ISCROLLBAR_RANGE);
  if (ipage < 1) ipage = 1;
  if (ipage > ISCROLLBAR_RANGE) ipage = ISCROLLBAR_RANGE;

  int imax = ISCROLLBAR_RANGE - ipage;
  if (imax < 0) imax = 0;

  int istep = (int)((ih->data->linestep / range) * ISCROLLBAR_RANGE);
  if (istep < 1) istep = 1;

  int ipagestep = (int)((ih->data->pagestep / range) * ISCROLLBAR_RANGE);
  if (ipagestep < 1) ipagestep = 1;

  int ipos = (int)(((ih->data->val - ih->data->vmin) / range) * ISCROLLBAR_RANGE);
  if (ipos < 0) ipos = 0;
  if (ipos > imax) ipos = imax;
  if (ih->data->inverted) ipos = imax - ipos;

  sb->SetSuppress(true);
  sb->SetRange(0.0f, (float)imax);
  sb->SetSteps((float)istep, (float)ipagestep);
  sb->SetProportion((float)ipage / (float)ISCROLLBAR_RANGE);
  sb->SetValue((float)ipos);
  sb->SetSuppress(false);
}

/* Attribute setters */

static int haikuSbSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    iupScrollbarCropValue(ih);
    haikuSbUpdateNative(ih);
  }
  return 0;
}

static int haikuSbSetLineStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->linestep), 0.01))
    haikuSbUpdateNative(ih);
  return 0;
}

static int haikuSbSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
    haikuSbUpdateNative(ih);
  return 0;
}

static int haikuSbSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    iupScrollbarCropValue(ih);
    haikuSbUpdateNative(ih);
  }
  return 0;
}

/* Map */

static int haikuSbMapMethod(Ihandle* ih)
{
  orientation o = (ih->data->orientation == ISCROLLBAR_HORIZONTAL) ? B_HORIZONTAL : B_VERTICAL;

  IupHaikuScrollBar* sb = new IupHaikuScrollBar(ih, o);
  ih->handle = (InativeHandle*)sb;
  iuphaikuAddToParent(ih);

  haikuSbUpdateNative(ih);
  return IUP_NOERROR;
}

static void haikuSbUnMapMethod(Ihandle* ih)
{
  IupHaikuScrollBar* sb = (IupHaikuScrollBar*)ih->handle;
  if (sb) sb->SetIhandle(NULL);
  iupdrvBaseUnMapMethod(ih);
}

/* Driver hooks */

extern "C" IUP_SDK_API void iupdrvScrollbarGetMinSize(Ihandle* ih, int* w, int* h)
{
  int sb = iupdrvGetScrollbarSize();
  int along = 3 * sb;

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    if (w) *w = along;
    if (h) *h = sb;
  }
  else
  {
    if (w) *w = sb;
    if (h) *h = along;
  }
}

extern "C" IUP_SDK_API void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = haikuSbMapMethod;
  ic->UnMap = haikuSbUnMapMethod;

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, haikuSbSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, haikuSbSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, haikuSbSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, haikuSbSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
