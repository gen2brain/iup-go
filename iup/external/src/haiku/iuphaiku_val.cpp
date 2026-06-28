/** \file
 * \brief Haiku Val (Slider) Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <Message.h>
#include <Rect.h>
#include <Slider.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_val.h"
#include "iup_drvfont.h"
}

#include "iuphaiku_drv.h"


/* BSlider is integer-based; IUP Val is double in [vmin..vmax]. */
static const int32 kValPrecision = 10000;

#define IUPHAIKU_VAL_MOD_MSG 'IupV'
#define IUPHAIKU_VAL_INV_MSG 'IupR'

class IupHaikuSlider : public BSlider
{
public:
  IupHaikuSlider(Ihandle* ih, orientation o)
    : BSlider(BRect(0, 0, 0, 0), "iup_val", NULL,
              NULL, 0, kValPrecision,
              o == B_VERTICAL ? B_VERTICAL : B_HORIZONTAL,
              B_BLOCK_THUMB),
      fIhandle(ih), fSuppress(false) {}

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }
  void SetSuppress(bool s) { fSuppress = s; }

  void AttachedToWindow() override
  {
    BSlider::AttachedToWindow();
    SetTarget(this);
    SetModificationMessage(new BMessage(IUPHAIKU_VAL_MOD_MSG));
    SetMessage(new BMessage(IUPHAIKU_VAL_INV_MSG));
  }

  void MessageReceived(BMessage* msg) override
  {
    if (!fSuppress && fIhandle && msg
        && (msg->what == IUPHAIKU_VAL_MOD_MSG || msg->what == IUPHAIKU_VAL_INV_MSG))
    {
      Dispatch();
      return;
    }
    BSlider::MessageReceived(msg);
  }

  void MakeFocus(bool focus = true) override
  {
    BSlider::MakeFocus(focus);
    if (fIhandle) iuphaikuFocusInOutEvent(fIhandle, focus ? 1 : 0);
  }

private:
  void Dispatch();

  Ihandle* fIhandle;
  bool fSuppress;
};

/* helpers */

/* BSlider vertical (bottom-min/top-max) already matches IUP default INVERTED=YES. */
static bool haikuValShouldFlip(Ihandle* ih)
{
  bool vertical = (ih->data->orientation == IVAL_VERTICAL);
  bool inverted = (ih->data->inverted != 0);
  return (vertical && !inverted) || (!vertical && inverted);
}

static int32 haikuValEncode(Ihandle* ih, double v)
{
  double vmin = ih->data->vmin;
  double vmax = ih->data->vmax;
  if (vmax <= vmin) return 0;
  double t = (v - vmin) / (vmax - vmin);
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  if (haikuValShouldFlip(ih)) t = 1.0 - t;
  return (int32)(t * kValPrecision + 0.5);
}

static double haikuValDecode(Ihandle* ih, int32 raw)
{
  double vmin = ih->data->vmin;
  double vmax = ih->data->vmax;
  double t = (double)raw / kValPrecision;
  if (haikuValShouldFlip(ih)) t = 1.0 - t;
  return vmin + t * (vmax - vmin);
}

void IupHaikuSlider::Dispatch()
{
  double old = fIhandle->data->val;
  fIhandle->data->val = haikuValDecode(fIhandle, Value());
  iupValCropValue(fIhandle);

  IFn cb = IupGetCallback(fIhandle, "VALUECHANGED_CB");
  if (cb)
  {
    if (fIhandle->data->val == old) return;
    int ret = cb(fIhandle);
    if (ret == IUP_CLOSE) IupExitLoop();
  }
}

/* Attribute setters */

static int haikuValSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrToDouble(value, &(ih->data->val))) return 0;
  iupValCropValue(ih);
  IupHaikuSlider* s = (IupHaikuSlider*)ih->handle;
  if (s)
  {
    LooperLockGuard guard(s->Looper());
    s->SetSuppress(true);
    s->SetValue(haikuValEncode(ih, ih->data->val));
    s->SetSuppress(false);
  }
  return 0;
}

static int haikuValSetActiveAttrib(Ihandle* ih, const char* value)
{
  IupHaikuSlider* s = (IupHaikuSlider*)ih->handle;
  if (s)
  {
    LooperLockGuard guard(s->Looper());
    s->SetEnabled(iupStrBoolean(value));
  }
  return iupBaseSetActiveAttrib(ih, value);
}

static int haikuValSetStepAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrToDoubleDef(value, &(ih->data->step), 0.01)) return 1;
  IupHaikuSlider* s = (IupHaikuSlider*)ih->handle;
  if (!s) return 1;
  double range = ih->data->vmax - ih->data->vmin;
  if (range <= 0) return 1;
  int32 inc = (int32)((ih->data->step / range) * kValPrecision + 0.5);
  if (inc < 1) inc = 1;
  LooperLockGuard guard(s->Looper());
  s->SetKeyIncrementValue(inc);
  return 1;
}

static hash_mark_location haikuValHashLocation(Ihandle* ih)
{
  const char* pos = iupAttribGet(ih, "TICKSPOS");
  if (!pos) return B_HASH_MARKS_BOTTOM;
  if (iupStrEqualNoCase(pos, "BOTH"))    return B_HASH_MARKS_BOTH;
  if (iupStrEqualNoCase(pos, "REVERSE")) return B_HASH_MARKS_TOP;
  return B_HASH_MARKS_BOTTOM;
}

static int haikuValSetShowTicksAttrib(Ihandle* ih, const char* value)
{
  IupHaikuSlider* s = (IupHaikuSlider*)ih->handle;
  if (!s) return 1;
  int n = 0;
  iupStrToInt(value, &n);
  LooperLockGuard guard(s->Looper());
  if (n > 0)
  {
    s->SetHashMarkCount(n);
    s->SetHashMarks(haikuValHashLocation(ih));
  }
  else
  {
    s->SetHashMarks(B_HASH_MARKS_NONE);
  }
  return 1;
}

static int haikuValSetTicksPosAttrib(Ihandle* ih, const char* /*value*/)
{
  IupHaikuSlider* s = (IupHaikuSlider*)ih->handle;
  if (!s) return 1;
  LooperLockGuard guard(s->Looper());
  if (s->HashMarks() != B_HASH_MARKS_NONE)
    s->SetHashMarks(haikuValHashLocation(ih));
  return 1;
}

/* Map */

static int haikuValMapMethod(Ihandle* ih)
{
  orientation o = (ih->data->orientation == IVAL_VERTICAL) ? B_VERTICAL : B_HORIZONTAL;

  IupHaikuSlider* slider = new IupHaikuSlider(ih, o);
  ih->handle = (InativeHandle*)slider;

  iuphaikuAddToParent(ih);
  iuphaikuUpdateWidgetFont(ih, slider);

  /* Push core state to the native slider. */
  {
    LooperLockGuard guard(slider->Looper());
    slider->SetSuppress(true);
    slider->SetValue(haikuValEncode(ih, ih->data->val));
    slider->SetSuppress(false);
  }

  char* showticks = iupAttribGet(ih, "SHOWTICKS");
  if (showticks) haikuValSetShowTicksAttrib(ih, showticks);

  char* step = iupAttribGet(ih, "STEP");
  if (step) haikuValSetStepAttrib(ih, step);

  return IUP_NOERROR;
}

static void haikuValUnMapMethod(Ihandle* ih)
{
  IupHaikuSlider* s = (IupHaikuSlider*)ih->handle;
  if (s) s->SetIhandle(NULL);
  iupdrvBaseUnMapMethod(ih);
}

/* Driver hooks */

extern "C" IUP_SDK_API void iupdrvValGetMinSize(Ihandle* ih, int *w, int *h)
{
  int vertical = (ih->data->orientation == IVAL_VERTICAL);
  if (w) *w = vertical ? 22 : 80;
  if (h) *h = vertical ? 80 : 22;
}

extern "C" IUP_SDK_API void iupdrvValInitClass(Iclass* ic)
{
  ic->Map = haikuValMapMethod;
  ic->UnMap = haikuValUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuValSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, haikuValSetValueAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STEP", NULL, haikuValSetStepAttrib, "0.01", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", NULL, NULL, "0.1", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWTICKS", NULL, haikuValSetShowTicksAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, haikuValSetTicksPosAttrib, IUPAF_SAMEASSYSTEM, "NORMAL", IUPAF_NO_INHERIT);
}
