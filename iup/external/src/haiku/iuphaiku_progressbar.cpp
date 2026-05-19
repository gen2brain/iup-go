/** \file
 * \brief Haiku ProgressBar Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <ControlLook.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <Rect.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_progressbar.h"
#include "iup_drvfont.h"
}

#include "iuphaiku_drv.h"


#define IUPHAIKU_PB_TICK 'IpbT'

class IupHaikuProgressBar : public BView
{
public:
  explicit IupHaikuProgressBar(orientation o)
    : BView(BRect(0, 0, 0, 0), "iup_progressbar", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
      fOrientation(o), fValue(0.0f), fMarquee(false), fPhase(0.0f),
      fRunner(NULL),
      fHasBarColor(false), fHasBgColor(false)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  }

  ~IupHaikuProgressBar() override { delete fRunner; }

  void SetValue(float v01)
  {
    if (v01 < 0) v01 = 0;
    if (v01 > 1) v01 = 1;
    if (fValue == v01) return;
    fValue = v01;
    if (!fMarquee) Invalidate();
  }

  void SetMarquee(bool on)
  {
    if (fMarquee == on) return;
    fMarquee = on;
    if (on)
    {
      if (!fRunner)
        fRunner = new BMessageRunner(BMessenger(this), new BMessage(IUPHAIKU_PB_TICK),50000);  /* 50 ms */
    }
    else
    {
      delete fRunner;
      fRunner = NULL;
    }
    Invalidate();
  }

  void SetBarColor(rgb_color c) { fBarColor = c; fHasBarColor = true; Invalidate(); }
  void SetBgColor(rgb_color c)  { fBgColor  = c; fHasBgColor  = true; SetViewColor(c); Invalidate(); }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_PB_TICK)
    {
      fPhase += 0.02f;
      if (fPhase >= 1.4f) fPhase -= 1.4f;
      Invalidate();
      return;
    }
    BView::MessageReceived(msg);
  }

  void Draw(BRect dirty) override
  {
    BRect b = Bounds();
    rgb_color bg  = fHasBgColor  ? fBgColor  : ui_color(B_PANEL_BACKGROUND_COLOR);
    rgb_color bar = fHasBarColor ? fBarColor : ui_color(B_STATUS_BAR_COLOR);

    if (fMarquee)
    {
      DrawMarquee(b, bg, bar);
      return;
    }

    if (fOrientation == B_HORIZONTAL)
    {
      float pos = b.left + (b.Width() * fValue);
      be_control_look->DrawStatusBar(this, b, dirty, bg, bar, pos);
      return;
    }

    /* Vertical: no native variant; framed fill from bottom up. */
    SetLowColor(bg);
    FillRect(b, B_SOLID_LOW);
    BRect fill = b;
    fill.InsetBy(1, 1);
    fill.top = fill.bottom - (fill.Height() * fValue);
    if (fill.Height() >= 0)
    {
      SetHighColor(bar);
      FillRect(fill);
    }
    SetHighColor(tint_color(bg, B_DARKEN_2_TINT));
    StrokeRect(b);
  }

private:
  /* Phase wraps [0..1.4) so the 30% strip exits before re-entering. */
  void DrawMarquee(BRect b, rgb_color bg, rgb_color bar)
  {
    SetLowColor(bg);
    FillRect(b, B_SOLID_LOW);
    SetHighColor(tint_color(bg, B_DARKEN_2_TINT));
    StrokeRect(b);

    BRect strip = b;
    strip.InsetBy(1, 1);
    const float frac = 0.3f;
    if (fOrientation == B_HORIZONTAL)
    {
      float total = strip.Width();
      float start = (fPhase - frac) * total;
      strip.left  = strip.left + start;
      strip.right = strip.left + frac * total;
      if (strip.left  < b.left + 1)  strip.left  = b.left + 1;
      if (strip.right > b.right - 1) strip.right = b.right - 1;
    }
    else
    {
      float total = strip.Height();
      float start = (fPhase - frac) * total;
      strip.top    = strip.top + start;
      strip.bottom = strip.top + frac * total;
      if (strip.top    < b.top + 1)    strip.top    = b.top + 1;
      if (strip.bottom > b.bottom - 1) strip.bottom = b.bottom - 1;
    }
    if (strip.Width() >= 0 && strip.Height() >= 0)
    {
      SetHighColor(bar);
      FillRect(strip);
    }
  }

  orientation fOrientation;
  float fValue;
  bool fMarquee;
  float fPhase;
  BMessageRunner* fRunner;
  rgb_color fBarColor, fBgColor;
  bool fHasBarColor, fHasBgColor;
};

static int haikuPbSetOrientationAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) return 0;  /* creation-only */
  if (iupStrEqualNoCase(value, "VERTICAL"))
    IupSetAttribute(ih, "RASTERSIZE", "16x200");
  else
    IupSetAttribute(ih, "RASTERSIZE", "200x16");
  return 1;
}

static int haikuPbSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrToDouble(value, &(ih->data->value))) return 0;
  iProgressBarCropValue(ih);
  IupHaikuProgressBar* bar = (IupHaikuProgressBar*)ih->handle;
  if (bar)
  {
    double range = ih->data->vmax - ih->data->vmin;
    float t = (range > 0) ? (float)((ih->data->value - ih->data->vmin) / range) : 0.0f;
    LooperLockGuard guard(bar->Looper());
    bar->SetValue(t);
  }
  return 0;
}

static int haikuPbSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  IupHaikuProgressBar* bar = (IupHaikuProgressBar*)ih->handle;
  ih->data->marquee = iupStrBoolean(value) ? 1 : 0;
  if (bar)
  {
    LooperLockGuard guard(bar->Looper());
    bar->SetMarquee(ih->data->marquee != 0);
  }
  return 1;
}

static int haikuPbSetBgColorAttrib(Ihandle* ih, const char* value)
{
  IupHaikuProgressBar* bar = (IupHaikuProgressBar*)ih->handle;
  if (!bar || !value) return 1;
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 1;
  rgb_color c = { r, g, b, 255 };
  LooperLockGuard guard(bar->Looper());
  bar->SetBgColor(c);
  return 1;
}

static int haikuPbSetFgColorAttrib(Ihandle* ih, const char* value)
{
  IupHaikuProgressBar* bar = (IupHaikuProgressBar*)ih->handle;
  if (!bar || !value) return 1;
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 1;
  rgb_color c = { r, g, b, 255 };
  LooperLockGuard guard(bar->Looper());
  bar->SetBarColor(c);
  return 1;
}

static int haikuPbMapMethod(Ihandle* ih)
{
  bool vertical = iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL");
  orientation o = vertical ? B_VERTICAL : B_HORIZONTAL;

  IupHaikuProgressBar* bar = new IupHaikuProgressBar(o);
  ih->handle = (InativeHandle*)bar;

  iuphaikuAddToParent(ih);
  iuphaikuUpdateWidgetFont(ih, bar);

  double range = ih->data->vmax - ih->data->vmin;
  float t = (range > 0) ? (float)((ih->data->value - ih->data->vmin) / range) : 0.0f;
  {
    LooperLockGuard guard(bar->Looper());
    bar->SetValue(t);
    if (ih->data->marquee) bar->SetMarquee(true);
  }
  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
{
  bool vertical = iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL");
  if (w) *w = vertical ? 16 : 100;
  if (h) *h = vertical ? 100 : 16;
}

extern "C" IUP_SDK_API void iupdrvProgressBarInitClass(Iclass* ic)
{
  ic->Map = haikuPbMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, haikuPbSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, haikuPbSetFgColorAttrib, IUPAF_SAMEASSYSTEM, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, haikuPbSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, haikuPbSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MARQUEE", NULL, haikuPbSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
