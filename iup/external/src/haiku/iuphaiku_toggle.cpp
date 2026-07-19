/** \file
 * \brief Haiku Toggle Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <type_traits>

#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <Font.h>
#include <Message.h>
#include <RadioButton.h>
#include <Rect.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_drv.h"
#include "iup_key.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_toggle.h"
#include "iup_drvfont.h"
}

#include "iuphaiku_drv.h"


#define IUPHAIKU_TOGGLE_MSG 'IupG'

class IupHaikuSwitchToggle;

static int32 haikuToggleNativeValue(BView* v);
static void  haikuToggleNativeSetValue(BView* v, int32 val);
static void  haikuToggleNativeSetEnabled(BView* v, bool enabled);
static void  haikuToggleNativeSetLabel(BView* v, const char* s);

/* Radio mutex: BRadioButton's native auto-uncheck ignores BButton siblings. */
static void haikuToggleApplyCheck(Ihandle* ih, int check)
{
  if (!ih || !iupObjectCheck(ih)) return;
  BView* self;

  Ihandle* radio = iupRadioFindToggleParent(ih);
  if (radio)
  {
    Ihandle* last = (Ihandle*)iupAttribGet(radio, "_IUPHAIKU_LASTTOGGLE");
    if (check)
    {
      if (iupObjectCheck(last) && last != ih)
      {
        BView* lastV = (BView*)last->handle;
        if (lastV)
        {
          LooperLockGuard g(lastV->Looper());
          haikuToggleNativeSetValue(lastV, B_CONTROL_OFF);
        }
        IFni acb = (IFni)IupGetCallback(last, "ACTION");
        if (acb && acb(last, 0) == IUP_CLOSE) IupExitLoop();
        Icallback vcb = IupGetCallback(last, "VALUECHANGED_CB");
        if (vcb && vcb(last) == IUP_CLOSE) IupExitLoop();
        /* Peer callbacks may have destroyed ih/radio. */
        if (!iupObjectCheck(ih) || !iupObjectCheck(radio)) return;
      }
      iupAttribSet(radio, "_IUPHAIKU_LASTTOGGLE", (char*)ih);
    }
  }

  self = (BView*)ih->handle;
  if (self)
  {
    LooperLockGuard g(self->Looper());
    haikuToggleNativeSetValue(self, check ? B_CONTROL_ON : B_CONTROL_OFF);
  }
}

static void haikuToggleDispatchClick(Ihandle* ih)
{
  if (!ih || !iupObjectCheck(ih)) return;
  BView* self = (BView*)ih->handle;
  if (!self) return;

  if (!iupRadioFindToggleParent(ih) && ih->data->type == IUP_TOGGLE_TEXT &&
      iupAttribGetBoolean(ih, "3STATE") && dynamic_cast<BCheckBox*>(self))
  {
    /* BCheckBox click is 2-state, so drive OFF -> ON -> NOTDEF -> OFF here */
    int prev = iupAttribGetInt(ih, "_IUPHAIKU_TRISTATE");
    int next = (prev == 0) ? 1 : (prev == 1) ? -1 : 0;
    int32 nat = (next < 0) ? B_CONTROL_PARTIALLY_ON : (next > 0) ? B_CONTROL_ON : B_CONTROL_OFF;
    {
      LooperLockGuard g(self->Looper());
      if (BControl* c = dynamic_cast<BControl*>(self)) c->SetValue(nat);
    }
    iupAttribSetInt(ih, "_IUPHAIKU_TRISTATE", next);

    IFni cb = (IFni)IupGetCallback(ih, "ACTION");
    int ret = IUP_DEFAULT;
    if (cb) ret = cb(ih, next);
    Icallback vc = IupGetCallback(ih, "VALUECHANGED_CB");
    if (vc) { int r = vc(ih); if (r == IUP_CLOSE) ret = IUP_CLOSE; }
    if (ret == IUP_CLOSE) IupExitLoop();
    return;
  }

  int now = (haikuToggleNativeValue(self) == B_CONTROL_ON) ? 1 : 0;

  /* Radio: re-click cannot deactivate. */
  Ihandle* radio = iupRadioFindToggleParent(ih);
  if (radio && !now)
  {
    LooperLockGuard g(self->Looper());
    haikuToggleNativeSetValue(self, B_CONTROL_ON);
    now = 1;
  }

  if (radio)
    haikuToggleApplyCheck(ih, 1);

  IFni cb = (IFni)IupGetCallback(ih, "ACTION");
  int ret = IUP_DEFAULT;
  if (cb) ret = cb(ih, now);
  Icallback vc = IupGetCallback(ih, "VALUECHANGED_CB");
  if (vc) { int r = vc(ih); if (r == IUP_CLOSE) ret = IUP_CLOSE; }
  if (ret == IUP_CLOSE) IupExitLoop();
}

/* Wiring for BControl-derived toggles (BCheckBox / BRadioButton). */

template <typename Base>
class IupHaikuToggleT : public Base
{
public:
  IupHaikuToggleT(Ihandle* ih, const char* label)
    : Base(BRect(0, 0, 0, 0), "iup_toggle", label, new BMessage(IUPHAIKU_TOGGLE_MSG)),
      fIhandle(ih), fForeColor{0, 0, 0, 0}, fHasFgColor(false) {}

  void AttachedToWindow() override
  {
    Base::AttachedToWindow();
    Base::SetTarget(this);
  }

  void MakeFocus(bool focus = true) override
  {
    Base::MakeFocus(focus);
    if (fIhandle) iuphaikuFocusInOutEvent(fIhandle, focus ? 1 : 0);
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_TOGGLE_MSG && fIhandle)
    {
      haikuToggleDispatchClick(fIhandle);
      return;
    }
    Base::MessageReceived(msg);
  }

  /* Custom draw needed for explicit textColor (DrawLabel ignores our HighColor) and for box-on-right */
  void Draw(BRect updateRect) override
  {
    bool over_gl = iuphaikuPaintGLBackgroundSlice(this, fIhandle);

    bool rightButton = false;
    if constexpr (std::is_same_v<Base, BCheckBox>)
      rightButton = fIhandle && iupAttribGetBoolean(fIhandle, "RIGHTBUTTON");

    if (!over_gl && !fHasFgColor && !rightButton) { Base::Draw(updateRect); return; }

    rgb_color base = over_gl ? ui_color(B_PANEL_BACKGROUND_COLOR) : this->ViewColor();
    font_height fh;
    this->GetFontHeight(&fh);
    uint32 flags = be_control_look->Flags(this);

    BRect indicator(0.0f, 2.0f, ceilf(3.0f + fh.ascent), ceilf(5.0f + fh.ascent));
    BRect labelRect(this->Bounds());
    if (rightButton)
    {
      float w = indicator.Width();
      indicator.right = this->Bounds().right;
      indicator.left = indicator.right - w;
      labelRect.right = indicator.left - 1 - be_control_look->DefaultLabelSpacing();
    }
    else
      labelRect.left = indicator.right + 1 + be_control_look->DefaultLabelSpacing();

    BRect rect(indicator);
    if constexpr (std::is_same_v<Base, BCheckBox>)
      be_control_look->DrawCheckBox(this, rect, updateRect, base, flags);
    else
      be_control_look->DrawRadioButton(this, rect, updateRect, base, flags);

    BAlignment alignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER);
    be_control_look->DrawLabel(this, this->Label(), (const BBitmap*)NULL,
                               labelRect, updateRect, base, flags, alignment,
                               fHasFgColor ? &fForeColor : (const rgb_color*)NULL);
  }

  void SetForeColor(rgb_color c) { fForeColor = c; fHasFgColor = true; this->Invalidate(); }

  Ihandle* GetIhandle() const { return fIhandle; }
  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

private:
  Ihandle* fIhandle;
  rgb_color fForeColor;
  bool fHasFgColor;
};

typedef IupHaikuToggleT<BCheckBox>    IupHaikuCheckBox;
typedef IupHaikuToggleT<BRadioButton> IupHaikuRadioButton;


/* Image toggle: BButton in B_TOGGLE_BEHAVIOR mode. */
class IupHaikuImageToggle : public BButton
{
public:
  IupHaikuImageToggle(Ihandle* ih, const char* label)
    : BButton(BRect(0, 0, 0, 0), "iup_imgtoggle", label,
              new BMessage(IUPHAIKU_TOGGLE_MSG), B_FOLLOW_NONE),
      fIhandle(ih)
  {
    SetBehavior(B_TOGGLE_BEHAVIOR);
  }

  void AttachedToWindow() override
  {
    BButton::AttachedToWindow();
    SetTarget(this);
  }

  void MakeFocus(bool focus = true) override
  {
    BButton::MakeFocus(focus);
    if (fIhandle) iuphaikuFocusInOutEvent(fIhandle, focus ? 1 : 0);
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_TOGGLE_MSG && fIhandle)
    {
      haikuToggleDispatchClick(fIhandle);
      return;
    }
    BButton::MessageReceived(msg);
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

private:
  Ihandle* fIhandle;
};


/* SWITCH toggle: squared inset track with be_control_look->DrawSliderThumb on top. */
class IupHaikuSwitchToggle : public BView
{
public:
  static constexpr int kTrackW = 40;
  static constexpr int kTrackH = 18;
  static constexpr int kThumbW = 17;
  static constexpr int kThumbH = 14;
  static constexpr int kMargin = 1;
  static constexpr int kGap    = 4;

  IupHaikuSwitchToggle(Ihandle* ih, const char* label)
    : BView(BRect(0, 0, 0, 0), "iup_switch", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
      fIhandle(ih), fLabel(NULL), fValue(0),
      fEnabled(true), fHover(false), fPressed(false), fHasFgColor(false)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    SetLabel(label);
  }

  void SetForeColor(rgb_color c) { fForeColor = c; fHasFgColor = true; Invalidate(); }

  ~IupHaikuSwitchToggle() override { free(fLabel); }

  void Draw(BRect /*update*/) override
  {
    BRect b = Bounds();
    rgb_color base = ViewColor();

    BRect track = TrackRect();
    BRect thumb = ThumbRect();

    uint32 flags = 0;
    if (!fEnabled)             flags |= BControlLook::B_DISABLED;
    if (fHover && fEnabled)    flags |= BControlLook::B_HOVER;
    if (fPressed && fEnabled)  flags |= BControlLook::B_ACTIVATED;
    if (IsFocus())             flags |= BControlLook::B_FOCUSED;

    rgb_color fill = fValue ? ui_color(B_CONTROL_HIGHLIGHT_COLOR)
                            : be_control_look ? be_control_look->SliderBarColor(base)
                                              : tint_color(base, B_DARKEN_1_TINT);
    if (!fEnabled) fill = tint_color(fill, B_LIGHTEN_1_TINT);

    rgb_color shadow = tint_color(base, B_DARKEN_2_TINT);
    rgb_color light  = tint_color(base, B_LIGHTEN_1_TINT);

    SetHighColor(shadow);
    StrokeLine(BPoint(track.left, track.top),    BPoint(track.right, track.top));
    StrokeLine(BPoint(track.left, track.top),    BPoint(track.left,  track.bottom));
    SetHighColor(light);
    StrokeLine(BPoint(track.left + 1, track.bottom), BPoint(track.right, track.bottom));
    StrokeLine(BPoint(track.right, track.top + 1),   BPoint(track.right, track.bottom));

    BRect inner(track.left + 1, track.top + 1, track.right - 1, track.bottom - 1);
    SetHighColor(fill);
    FillRect(inner);

    rgb_color thumb_base = base;
    if (fHover && fEnabled)
    {
      rgb_color hi = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
      thumb_base.red   = (uint8)((base.red   * 7 + hi.red   * 3) / 10);
      thumb_base.green = (uint8)((base.green * 7 + hi.green * 3) / 10);
      thumb_base.blue  = (uint8)((base.blue  * 7 + hi.blue  * 3) / 10);
    }
    if (be_control_look)
      be_control_look->DrawSliderThumb(this, thumb, b, thumb_base, flags, B_HORIZONTAL);
    else
    {
      SetHighColor(tint_color(base, 1.05f));
      FillRect(thumb);
      SetHighColor(tint_color(base, B_DARKEN_3_TINT));
      StrokeRect(thumb);
    }

    if (fLabel && fLabel[0])
    {
      font_height fh; GetFontHeight(&fh);
      rgb_color text = fHasFgColor ? fForeColor
                       : fEnabled ? ui_color(B_PANEL_TEXT_COLOR)
                                  : tint_color(ui_color(B_PANEL_TEXT_COLOR), B_DISABLED_LABEL_TINT);
      float gap = be_control_look ? be_control_look->DefaultLabelSpacing() : (float)kGap;
      float text_x = track.right + gap + 1;
      float text_y = (b.Height() - (fh.ascent + fh.descent)) / 2.0f + fh.ascent;
      SetHighColor(text);
      SetLowColor(base);
      SetDrawingMode(B_OP_OVER);
      DrawString(fLabel, BPoint(text_x, text_y));
    }
  }

  void MouseDown(BPoint /*where*/) override
  {
    if (!fEnabled || !fIhandle) return;
    SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
    fPressed = true;
    Invalidate();
  }

  void MouseUp(BPoint where) override
  {
    if (!fIhandle) return;
    bool was = fPressed;
    fPressed = false;
    Invalidate();
    if (was && fEnabled && Bounds().Contains(where))
    {
      fValue = !fValue;
      Invalidate();
      BMessenger(this).SendMessage(IUPHAIKU_TOGGLE_MSG);
    }
  }

  void MouseMoved(BPoint /*where*/, uint32 transit, const BMessage* /*drag*/) override
  {
    bool was = fHover;
    fHover = fEnabled && (transit == B_INSIDE_VIEW || transit == B_ENTERED_VIEW);
    if (was != fHover) Invalidate();
  }

  void KeyDown(const char* bytes, int32 numBytes) override
  {
    if (fEnabled && fIhandle && numBytes == 1 && (bytes[0] == ' ' || bytes[0] == B_ENTER))
    {
      fValue = !fValue;
      Invalidate();
      BMessenger(this).SendMessage(IUPHAIKU_TOGGLE_MSG);
      return;
    }
    BView::KeyDown(bytes, numBytes);
  }

  void MakeFocus(bool focus = true) override
  {
    BView::MakeFocus(focus);
    Invalidate();
    if (fIhandle) iuphaikuFocusInOutEvent(fIhandle, focus ? 1 : 0);
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_TOGGLE_MSG && fIhandle)
    {
      if (Window()) Window()->UpdateIfNeeded();
      haikuToggleDispatchClick(fIhandle);
      return;
    }
    BView::MessageReceived(msg);
  }

  int32 Value() const { return fValue ? B_CONTROL_ON : B_CONTROL_OFF; }
  void SetValue(int32 v)
  {
    int n = (v == B_CONTROL_ON) ? 1 : 0;
    if (n == fValue) return;
    fValue = n;
    Invalidate();
  }
  void SetEnabled(bool e)
  {
    if (e == fEnabled) return;
    fEnabled = e;
    Invalidate();
  }
  void SetLabel(const char* s)
  {
    if (fLabel) free(fLabel);
    fLabel = iupStrDup(s ? s : "");
    Invalidate();
  }
  const char* Label() const { return fLabel ? fLabel : ""; }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

private:
  BRect TrackRect() const
  {
    BRect b = Bounds();
    int y_off = (int)((b.Height() + 1 - kTrackH) / 2.0f);
    if (y_off < 0) y_off = 0;
    return BRect(0, y_off, kTrackW - 1, y_off + kTrackH - 1);
  }

  BRect ThumbRect() const
  {
    BRect t = TrackRect();
    float margin_y = (t.Height() + 1 - kThumbH) / 2.0f;
    float x_min = t.left + kMargin;
    float x_max = t.right - kMargin - kThumbW + 1;
    float x = fValue ? x_max : x_min;
    return BRect(x, t.top + margin_y, x + kThumbW - 1, t.top + margin_y + kThumbH - 1);
  }

  Ihandle* fIhandle;
  char* fLabel;
  int fValue;
  bool fEnabled;
  bool fHover;
  bool fPressed;
  rgb_color fForeColor;
  bool fHasFgColor;
};


static int32 haikuToggleNativeValue(BView* v)
{
  if (IupHaikuSwitchToggle* sw = dynamic_cast<IupHaikuSwitchToggle*>(v)) return sw->Value();
  if (BControl* c = dynamic_cast<BControl*>(v)) return c->Value();
  return B_CONTROL_OFF;
}

static void haikuToggleNativeSetValue(BView* v, int32 val)
{
  if (IupHaikuSwitchToggle* sw = dynamic_cast<IupHaikuSwitchToggle*>(v)) { sw->SetValue(val); return; }
  if (BControl* c = dynamic_cast<BControl*>(v)) c->SetValue(val);
}

static void haikuToggleNativeSetEnabled(BView* v, bool enabled)
{
  if (IupHaikuSwitchToggle* sw = dynamic_cast<IupHaikuSwitchToggle*>(v)) { sw->SetEnabled(enabled); return; }
  if (BControl* c = dynamic_cast<BControl*>(v)) c->SetEnabled(enabled);
}

static void haikuToggleNativeSetLabel(BView* v, const char* s)
{
  if (IupHaikuSwitchToggle* sw = dynamic_cast<IupHaikuSwitchToggle*>(v)) { sw->SetLabel(s); return; }
  if (BControl* c = dynamic_cast<BControl*>(v)) c->SetLabel(s);
}

static bool haikuToggleNativeSetForeColor(BView* v, rgb_color c)
{
  if (IupHaikuSwitchToggle* sw = dynamic_cast<IupHaikuSwitchToggle*>(v)) { sw->SetForeColor(c); return true; }
  if (auto* cb = dynamic_cast<IupHaikuCheckBox*>(v))    { cb->SetForeColor(c); return true; }
  if (auto* rb = dynamic_cast<IupHaikuRadioButton*>(v)) { rb->SetForeColor(c); return true; }
  return false;
}

/* Helpers */

static char* haikuStrippedMnemonic(const char* title)
{
  if (!title) return NULL;
  if (!strchr(title, '&')) return iupStrDup(title);
  return iupStrProcessMnemonic(title, NULL, 0);
}

/* Attribute setters */

static int haikuToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  BView* v = (BView*)ih->handle;
  if (!v) return 0;

  int three = (ih->data->type == IUP_TOGGLE_TEXT && !ih->data->is_radio &&
               iupAttribGetBoolean(ih, "3STATE") && dynamic_cast<BCheckBox*>(v));

  int32 want = B_CONTROL_OFF;
  if (iupStrEqualNoCase(value, "ON"))           want = B_CONTROL_ON;
  else if (iupStrEqualNoCase(value, "NOTDEF"))  want = B_CONTROL_PARTIALLY_ON;
  else if (iupStrEqualNoCase(value, "TOGGLE"))
  {
    if (three)
    {
      int prev = iupAttribGetInt(ih, "_IUPHAIKU_TRISTATE");
      int next = (prev == 0) ? 1 : (prev == 1) ? -1 : 0;
      want = (next < 0) ? B_CONTROL_PARTIALLY_ON : (next > 0) ? B_CONTROL_ON : B_CONTROL_OFF;
    }
    else
      want = (haikuToggleNativeValue(v) == B_CONTROL_ON) ? B_CONTROL_OFF : B_CONTROL_ON;
  }

  if (want == B_CONTROL_PARTIALLY_ON)
  {
    /* 3STATE is BCheckBox-only. */
    LooperLockGuard guard(v->Looper());
    if (BControl* c = dynamic_cast<BControl*>(v)) c->SetValue(want);
  }
  else
    haikuToggleApplyCheck(ih, want == B_CONTROL_ON);

  if (three)
    iupAttribSetInt(ih, "_IUPHAIKU_TRISTATE", (want == B_CONTROL_PARTIALLY_ON) ? -1 : (want == B_CONTROL_ON) ? 1 : 0);

  return 0;
}

static char* haikuToggleGetValueAttrib(Ihandle* ih)
{
  BView* v = (BView*)ih->handle;
  if (!v) return NULL;
  switch (haikuToggleNativeValue(v))
  {
    case B_CONTROL_ON:           return iupStrReturnStr("ON");
    case B_CONTROL_PARTIALLY_ON: return iupStrReturnStr("NOTDEF");
    default:                     return iupStrReturnStr("OFF");
  }
}

static int haikuToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_TOGGLE_TEXT) return 0;
  BView* v = (BView*)ih->handle;
  if (!v) return 1;
  int mn = iupStrFindMnemonic(value);
  if (mn) iupKeySetMnemonic(ih, mn, 0);
  char* stripped = haikuStrippedMnemonic(value);
  LooperLockGuard guard(v->Looper());
  haikuToggleNativeSetLabel(v, stripped ? stripped : "");
  if (stripped) free(stripped);
  return 1;
}

static int haikuToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || !value) return 1;
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 1;
  rgb_color c = { r, g, b, 255 };
  BView* v = (BView*)ih->handle;
  LooperLockGuard guard(v->Looper());
  if (!haikuToggleNativeSetForeColor(v, c)) v->SetHighColor(c);
  return 1;
}

static int haikuToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  BView* v = (BView*)ih->handle;
  if (v)
  {
    LooperLockGuard guard(v->Looper());
    haikuToggleNativeSetEnabled(v, iupStrBoolean(value));
  }
  return iupBaseSetActiveAttrib(ih, value);
}

static void haikuToggleApplyImage(Ihandle* ih, const char* name, int make_inactive)
{
  if (ih->data->type != IUP_TOGGLE_IMAGE || !name) return;
  IupHaikuImageToggle* btn = (IupHaikuImageToggle*)ih->handle;
  if (!btn) return;
  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  BBitmap* bm = (BBitmap*)iupImageGetImage(name, ih, make_inactive, bgcolor);
  if (!bm) return;
  LooperLockGuard guard(btn->Looper());
  btn->SetIcon(bm, 0);
}

static int haikuToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_TOGGLE_IMAGE || !value) return 0;
  if (iupdrvIsActive(ih))
    haikuToggleApplyImage(ih, value, 0);
  else
  {
    char* iminactive = iupAttribGet(ih, "IMINACTIVE");
    haikuToggleApplyImage(ih, iminactive ? iminactive : value, iminactive ? 0 : 1);
  }
  return 1;
}

static int haikuToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_TOGGLE_IMAGE) return 0;
  if (!iupdrvIsActive(ih))
  {
    if (value)
      haikuToggleApplyImage(ih, value, 0);
    else
    {
      char* image = iupAttribGet(ih, "IMAGE");
      if (image) haikuToggleApplyImage(ih, image, 1);
    }
  }
  return 1;
}

/* Map */

static int haikuToggleMapMethod(Ihandle* ih)
{
  if (iupAttribGet(ih, "IMAGE"))
    ih->data->type = IUP_TOGGLE_IMAGE;
  else
    ih->data->type = IUP_TOGGLE_TEXT;

  Ihandle* radio = iupRadioFindToggleParent(ih);
  int is_switch = (ih->data->type == IUP_TOGGLE_TEXT) && !radio
                  && iupAttribGetBoolean(ih, "SWITCH");

  BView* native = NULL;
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    native = new IupHaikuImageToggle(ih, "");
  }
  else if (is_switch)
  {
    char* title = iupAttribGet(ih, "TITLE");
    char* stripped = haikuStrippedMnemonic(title);
    native = new IupHaikuSwitchToggle(ih, stripped ? stripped : "");
    if (stripped) free(stripped);
  }
  else
  {
    char* title = iupAttribGet(ih, "TITLE");
    char* stripped = haikuStrippedMnemonic(title);
    if (radio)
      native = new IupHaikuRadioButton(ih, stripped ? stripped : "");
    else
      native = new IupHaikuCheckBox(ih, stripped ? stripped : "");
    if (stripped) free(stripped);
  }

  ih->handle = (InativeHandle*)native;
  iuphaikuAddToParent(ih);

  iuphaikuSetGLBackgroundChild(ih, native);

  iuphaikuUpdateWidgetFont(ih, native);

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image) haikuToggleApplyImage(ih, image, 0);
  }

  /* First-mapped radio toggle auto-becomes ON via attribute replay. */
  if (radio && !iupAttribGet(radio, "_IUPHAIKU_LASTTOGGLE") && !iupAttribGet(ih, "VALUE"))
    iupAttribSet(ih, "VALUE", "ON");

  return IUP_NOERROR;
}

static void haikuToggleUnMapMethod(Ihandle* ih)
{
  /* Null the back-pointer; in-flight messages become no-ops. */
  BView* v = (BView*)ih->handle;
  if (v)
  {
    if (IupHaikuImageToggle* it = dynamic_cast<IupHaikuImageToggle*>(v))      it->SetIhandle(NULL);
    else if (IupHaikuSwitchToggle* sw = dynamic_cast<IupHaikuSwitchToggle*>(v)) sw->SetIhandle(NULL);
    else if (IupHaikuCheckBox* cb = dynamic_cast<IupHaikuCheckBox*>(v))        cb->SetIhandle(NULL);
    else if (IupHaikuRadioButton* rb = dynamic_cast<IupHaikuRadioButton*>(v))  rb->SetIhandle(NULL);
  }
  iupdrvBaseUnMapMethod(ih);
}

/* Driver hooks */

extern "C" IUP_SDK_API void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y)
{
  if (ih && ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (!be_control_look) { if (x) *x += 24; if (y) *y += 12; return; }
    float left, top, right, bottom;
    be_control_look->GetInsets(BControlLook::B_BUTTON_FRAME,
                               BControlLook::B_BUTTON_BACKGROUND, 0,
                               left, top, right, bottom);
    float spacing = be_control_look->DefaultLabelSpacing();
    if (x) *x += (int)(left + right + spacing - 1 + 0.5f);
    if (y) *y += (int)(top + bottom + spacing + 0.5f);
    return;
  }
  if (x) *x += 4;
  if (y) *y += 4;
}

static void haikuToggleProbeChrome(Ihandle* ih, int* chrome_w, int* min_h)
{
  static const char* kProbe = "Mg";  /* short label for ascender + descender sample */

  BCheckBox probe(BRect(0, 0, 0, 0), "iup_probe", kProbe, NULL);
  BFont* bf = ih ? iuphaikuGetBFont(iupGetFontValue(ih)) : NULL;
  if (bf) probe.SetFont(bf);

  float pw = 0, ph = 0;
  probe.GetPreferredSize(&pw, &ph);

  float lw = probe.StringWidth(kProbe);

  *chrome_w = (int)ceilf(pw - lw) + 2;
  *min_h    = (int)ceilf(ph);
}

extern "C" IUP_SDK_API void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* /*str*/)
{
  int chrome_w, min_h;
  haikuToggleProbeChrome(ih, &chrome_w, &min_h);
  if (x) *x += chrome_w;
  if (y && *y < min_h) *y = min_h;
}

extern "C" IUP_SDK_API void iupdrvToggleAddSwitch(Ihandle* ih, int *x, int *y, const char* /*str*/)
{
  float gap = be_control_look ? be_control_look->DefaultLabelSpacing() : (float)IupHaikuSwitchToggle::kGap;
  int chrome = IupHaikuSwitchToggle::kTrackW + (int)gap + 1;
  if (x) *x += chrome;

  int min_h = IupHaikuSwitchToggle::kTrackH;
  if (ih)
  {
    BFont* bf = iuphaikuGetBFont(iupGetFontValue(ih));
    if (bf)
    {
      font_height fh; bf->GetHeight(&fh);
      int label_h = (int)ceilf(fh.ascent + fh.descent);
      if (label_h > min_h) min_h = label_h;
    }
  }
  if (y && *y < min_h) *y = min_h;
}

static int haikuToggleSetRightButtonAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  BView* v = (BView*)ih->handle;
  if (v && v->Window())
  {
    LooperLockGuard g(v->Looper());
    v->Invalidate();
  }
  return 1;
}

extern "C" IUP_SDK_API void iupdrvToggleInitClass(Iclass* ic)
{
  ic->Map = haikuToggleMapMethod;
  ic->UnMap = haikuToggleUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, haikuToggleSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, haikuToggleSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", haikuToggleGetValueAttrib, haikuToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, haikuToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, haikuToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, haikuToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, NULL, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  /* SWITCH is pre-map only; haikuToggleMapMethod consumes it. */
  iupClassRegisterAttribute(ic, "SWITCH", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  /* 3STATE: VALUE=NOTDEF maps to B_CONTROL_PARTIALLY_ON. */
  iupClassRegisterAttribute(ic, "3STATE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, NULL, "ACENTER:ACENTER", NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FLAT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, haikuToggleSetRightButtonAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
