/** \file
 * \brief Haiku Canvas Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <InterfaceDefs.h>
#include <Looper.h>
#include <Message.h>
#include <OS.h>
#include <Point.h>
#include <Rect.h>
#include <ScrollBar.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_key.h"
#include "iup_drv.h"
#include "iup_canvas.h"
}

#include "iuphaiku_drv.h"


void iuphaikuCanvasOnDraw(Ihandle* ih, BView* view, BRect dirty)
{
  if (!ih || !ih->data) return;
  if (ih->data->inside_resize) return;

  IFn cb = IupGetCallback(ih, "ACTION");
  if (!cb)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(iupAttribGetStr(ih, "BGCOLOR"), &r, &g, &b))
    {
      rgb_color c = { r, g, b, 255 };
      view->PushState();
      view->SetHighColor(c);
      view->SetDrawingMode(B_OP_COPY);
      view->FillRect(dirty);
      view->PopState();
    }
    return;
  }

  /* Save/restore so a nested draw triggered by cb sees its outer's value. */
  const char* prev = iupAttribGet(ih, "CLIPRECT");
  char* saved = prev ? iupStrDup(prev) : NULL;
  iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d",
                   (int)dirty.left, (int)dirty.top,
                   (int)dirty.right, (int)dirty.bottom);
  cb(ih);
  if (saved) { iupAttribSetStr(ih, "CLIPRECT", saved); free(saved); }
  else { iupAttribSet(ih, "CLIPRECT", NULL); }
}

void iuphaikuCanvasOnFrameResized(Ihandle* ih, BView* view, float new_w, float new_h)
{
  (void)view;
  if (!ih || !ih->data) return;
  ih->data->inside_resize = 1;
  IFnii cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (cb) cb(ih, (int)(new_w + 1), (int)(new_h + 1));
  ih->data->inside_resize = 0;
}

void iuphaikuCanvasOnAttachedToWindow(Ihandle* ih, BView* view)
{
  /* BView::ResizeTo skips FrameResized when detached: catch up here. */
  if (!ih || !ih->data) return;
  BRect b = view->Bounds();
  int w = (int)(b.Width() + 1), h = (int)(b.Height() + 1);
  if (w <= 1 && h <= 1) return;
  IFnii cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (!cb) return;
  ih->data->inside_resize = 1;
  cb(ih, w, h);
  ih->data->inside_resize = 0;
}

void iuphaikuCanvasOnMakeFocus(Ihandle* ih, BView* view, bool focused)
{
  (void)view;
  iuphaikuFocusInOutEvent(ih, focused ? 1 : 0);
}

void iuphaikuCanvasOnMouseDown(Ihandle* ih, BView* view, BPoint where)
{
  if (!ih) return;
  if (!iupdrvIsActive(ih)) return;
  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    view->MakeFocus(true);

  BMessage* msg = NULL;
  if (view->Looper()) msg = view->Looper()->CurrentMessage();
  int32 buttons = 0, mods = 0, clicks = 1;
  if (msg)
  {
    msg->FindInt32("buttons", &buttons);
    msg->FindInt32("modifiers", &mods);
    msg->FindInt32("clicks", &clicks);
  }

  view->SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

  int btn = 0;
  if      (buttons & B_PRIMARY_MOUSE_BUTTON)   btn = IUP_BUTTON1;
  else if (buttons & B_SECONDARY_MOUSE_BUTTON) btn = IUP_BUTTON3;
  else if (buttons & B_TERTIARY_MOUSE_BUTTON)  btn = IUP_BUTTON2;

  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  iuphaikuButtonKeySetStatus((unsigned)mods, (unsigned)buttons, 0, status, clicks == 2 ? 1 : 0);

  iuphaikuDnDMouseDown(ih, where, (unsigned)buttons);

  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb) cb(ih, btn, 1, (int)where.x, (int)where.y, status);
}

void iuphaikuCanvasOnMouseUp(Ihandle* ih, BView* view, BPoint where)
{
  if (!ih) return;

  BMessage* msg = NULL;
  if (view->Looper()) msg = view->Looper()->CurrentMessage();
  int32 mods = 0;
  if (msg) msg->FindInt32("modifiers", &mods);

  iuphaikuDnDMouseUp(ih);

  int btn = IUP_BUTTON1;
  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  iuphaikuButtonKeySetStatus((unsigned)mods, 0, 0, status, 0);

  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb) cb(ih, btn, 0, (int)where.x, (int)where.y, status);
}

void iuphaikuCanvasOnMouseMoved(Ihandle* ih, BView* view, BPoint where, unsigned int transit, const BMessage* drag)
{
  if (!ih) return;
  if (!iupdrvIsActive(ih)) return;

  if (transit == B_ENTERED_VIEW)
  {
    IFn cb = IupGetCallback(ih, "ENTERWINDOW_CB");
    if (cb) cb(ih);
  }
  else if (transit == B_EXITED_VIEW)
  {
    IFn cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
    if (cb) cb(ih);
    return;
  }

  BMessage* msg = NULL;
  if (view->Looper()) msg = view->Looper()->CurrentMessage();
  int32 buttons = 0, mods = 0;
  if (msg)
  {
    msg->FindInt32("buttons", &buttons);
    msg->FindInt32("modifiers", &mods);
  }

  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  iuphaikuButtonKeySetStatus((unsigned)mods, (unsigned)buttons, 0, status, 0);

  if (iuphaikuDnDMouseMoved(ih, view, where, transit, drag))
    return;

  IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb) cb(ih, (int)where.x, (int)where.y, status);
}

bool iuphaikuCanvasOnKeyDown(Ihandle* ih, BView* view, const char* bytes, int numBytes)
{
  if (!ih || numBytes < 1) return false;

  BMessage* msg = NULL;
  if (view->Looper()) msg = view->Looper()->CurrentMessage();
  int32 raw_char = 0, mods = 0;
  if (msg)
  {
    msg->FindInt32("raw_char", &raw_char);
    msg->FindInt32("modifiers", &mods);
  }

  int code = iuphaikuKeyDecode((unsigned char)bytes[0], raw_char, (unsigned)mods);
  if (code == 0) return false;

  int ret_press = iupKeyCallKeyPressCb(ih, code, 1);
  int ret_any   = iupKeyCallKeyCb(ih, code);
  if (ret_press == IUP_CLOSE || ret_any == IUP_CLOSE)
    IupExitLoop();
  return true;
}

bool iuphaikuCanvasOnKeyUp(Ihandle* ih, BView* view, const char* bytes, int numBytes)
{
  if (!ih || numBytes < 1) return false;

  BMessage* msg = NULL;
  if (view->Looper()) msg = view->Looper()->CurrentMessage();
  int32 raw_char = 0, mods = 0;
  if (msg)
  {
    msg->FindInt32("raw_char", &raw_char);
    msg->FindInt32("modifiers", &mods);
  }

  int code = iuphaikuKeyDecode((int)(unsigned char)bytes[0], (int)raw_char, (unsigned)mods);
  if (code) iupKeyCallKeyPressCb(ih, code, 0);
  return code != 0;
}

bool iuphaikuCanvasOnMessageReceived(Ihandle* ih, BView* view, BMessage* msg)
{
  if (iuphaikuDnDMessageReceived(ih, view, msg)) return true;
  if (msg && msg->what == B_MOUSE_WHEEL_CHANGED && ih)
  {
    float dx = 0.0f, dy = 0.0f;
    msg->FindFloat("be:wheel_delta_x", &dx);
    msg->FindFloat("be:wheel_delta_y", &dy);

    int32 mods = 0;
    msg->FindInt32("modifiers", &mods);

    BPoint pt;
    uint32 buttons = 0;
    view->GetMouse(&pt, &buttons, false);

    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iuphaikuButtonKeySetStatus((unsigned)mods, buttons, 0, status, 0);

    /* Haiku wheel delta is positive-down; IUP convention positive-up. */
    IFnfiis cb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
    if (cb) cb(ih, -dy, (int)pt.x, (int)pt.y, status);

    /* Drive the vertical scrollbar so SCROLL_CB-based scrollers (ScrollBox)
       get wheel scrolling without each app wiring WHEEL_CB manually. */
    if (ih->data && (ih->data->sb & IUP_SB_VERT) && dy != 0.0f)
    {
      double ymin = iupAttribGetDouble(ih, "YMIN");
      double ymax = iupAttribGetDouble(ih, "YMAX");
      double page = iupAttribGetDouble(ih, "DY");
      if (page < (ymax - ymin))
      {
        double step = (page > 0) ? (page / 5.0) : ((ymax - ymin) / 20.0);
        if (step < 1) step = 1;
        double newpos = ih->data->posy + dy * step;
        if (newpos < ymin) newpos = ymin;
        if (newpos > ymax - page) newpos = ymax - page;
        if (BScrollBar* vsb = (BScrollBar*)iupAttribGet(ih, "_IUPHAIKU_CANVAS_VSB"))
          vsb->SetValue((float)newpos);
      }
    }
    return true;
  }
  return false;
}


class IupHaikuCanvasView : public BView
{
public:
  explicit IupHaikuCanvasView(Ihandle* ih)
    : BView(BRect(0, 0, 0, 0), "iup_canvas", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE),
      fIhandle(ih)
  {
    /* Track theme; SetViewColor() override happens in BGCOLOR setter. */
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

  void Draw(BRect dirty) override
  {
    iuphaikuCanvasOnDraw(fIhandle, this, dirty);
  }

  void FrameResized(float new_w, float new_h) override
  {
    BView::FrameResized(new_w, new_h);
    iuphaikuCanvasOnFrameResized(fIhandle, this, new_w, new_h);
  }

  void AttachedToWindow() override
  {
    BView::AttachedToWindow();
    iuphaikuCanvasOnAttachedToWindow(fIhandle, this);
  }

  void MakeFocus(bool focused = true) override
  {
    BView::MakeFocus(focused);
    iuphaikuCanvasOnMakeFocus(fIhandle, this, focused);
  }

  void MouseDown(BPoint where) override
  {
    iuphaikuCanvasOnMouseDown(fIhandle, this, where);
  }

  void MouseUp(BPoint where) override
  {
    iuphaikuCanvasOnMouseUp(fIhandle, this, where);
  }

  void MouseMoved(BPoint where, uint32 transit, const BMessage* drag) override
  {
    iuphaikuCanvasOnMouseMoved(fIhandle, this, where, transit, drag);
  }

  void KeyDown(const char* bytes, int32 numBytes) override
  {
    if (!iuphaikuCanvasOnKeyDown(fIhandle, this, bytes, numBytes))
      BView::KeyDown(bytes, numBytes);
  }

  void KeyUp(const char* bytes, int32 numBytes) override
  {
    if (!iuphaikuCanvasOnKeyUp(fIhandle, this, bytes, numBytes))
      BView::KeyUp(bytes, numBytes);
  }

  void MessageReceived(BMessage* msg) override
  {
    if (!iuphaikuCanvasOnMessageReceived(fIhandle, this, msg))
      BView::MessageReceived(msg);
  }

private:
  Ihandle* fIhandle;
};


class IupHaikuCanvasScrollBar : public BScrollBar
{
public:
  IupHaikuCanvasScrollBar(Ihandle* ih, orientation dir)
    : BScrollBar(BRect(0, 0, 0, 0),
                 dir == B_HORIZONTAL ? "iup_canvas_hsb" : "iup_canvas_vsb",
                 NULL, 0, 0, dir),
      fIhandle(ih), fHoriz(dir == B_HORIZONTAL), fSilent(false) {}

  void SetSilent(bool s) { fSilent = s; }
  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

  void ValueChanged(float v) override
  {
    BScrollBar::ValueChanged(v);
    if (fSilent || !fIhandle || !fIhandle->data) return;

    double pos = v;
    if (fHoriz) { fIhandle->data->posx = pos; iupAttribSetDouble(fIhandle, "POSX", pos); }
    else        { fIhandle->data->posy = pos; iupAttribSetDouble(fIhandle, "POSY", pos); }

    IFniff cb = (IFniff)IupGetCallback(fIhandle, "SCROLL_CB");
    if (cb)
    {
      int op = fHoriz ? IUP_SBPOSH : IUP_SBPOSV;
      cb(fIhandle, op, (float)fIhandle->data->posx, (float)fIhandle->data->posy);
    }
    else if (IupHaikuCanvasView* inner =
        (IupHaikuCanvasView*)iupAttribGet(fIhandle, "_IUPHAIKU_CANVAS_INNER"))
      inner->Invalidate();
  }

private:
  Ihandle* fIhandle;
  bool fHoriz;
  bool fSilent;
};


class IupHaikuCanvasWrap : public BView
{
public:
  explicit IupHaikuCanvasWrap(Ihandle* ih)
    : BView(BRect(0, 0, 0, 0), "iup_canvas_wrap", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FRAME_EVENTS),
      fIhandle(ih), fCanvas(NULL), fHSB(NULL), fVSB(NULL)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  }

  void Attach(IupHaikuCanvasView* canvas, IupHaikuCanvasScrollBar* hsb, IupHaikuCanvasScrollBar* vsb)
  {
    fCanvas = canvas; fHSB = hsb; fVSB = vsb;
    AddChild(canvas);
    if (hsb) AddChild(hsb);
    if (vsb) AddChild(vsb);
  }

  IupHaikuCanvasView* CanvasView() const { return fCanvas; }
  IupHaikuCanvasScrollBar* HSB() const { return fHSB; }
  IupHaikuCanvasScrollBar* VSB() const { return fVSB; }

  void FrameResized(float w, float h) override
  {
    BView::FrameResized(w, h);
    RelayoutChildren();
  }

  void RelayoutChildren();

private:
  Ihandle* fIhandle;
  IupHaikuCanvasView* fCanvas;
  IupHaikuCanvasScrollBar* fHSB;
  IupHaikuCanvasScrollBar* fVSB;
};


static bool haikuCanvasAxisFits(Ihandle* ih, bool horiz)
{
  double lo = iupAttribGetDouble(ih, horiz ? "XMIN" : "YMIN");
  double hi = iupAttribGetDouble(ih, horiz ? "XMAX" : "YMAX");
  double page = iupAttribGetDouble(ih, horiz ? "DX" : "DY");
  return page >= (hi - lo);
}

void IupHaikuCanvasWrap::RelayoutChildren()
{
  if (!fCanvas || !fIhandle) return;

  float total_w = Bounds().Width();
  float total_h = Bounds().Height();
  float sb_w = B_V_SCROLL_BAR_WIDTH;
  float sb_h = B_H_SCROLL_BAR_HEIGHT;

  bool show_h = fHSB && !(iupAttribGetBoolean(fIhandle, "XAUTOHIDE") && haikuCanvasAxisFits(fIhandle, true));
  bool show_v = fVSB && !(iupAttribGetBoolean(fIhandle, "YAUTOHIDE") && haikuCanvasAxisFits(fIhandle, false));

  float c_w = total_w - (show_v ? sb_w : 0);
  float c_h = total_h - (show_h ? sb_h : 0);
  if (c_w < 0) c_w = 0;
  if (c_h < 0) c_h = 0;

  fCanvas->MoveTo(0, 0);
  fCanvas->ResizeTo(c_w, c_h);

  if (fHSB)
  {
    if (show_h)
    {
      fHSB->MoveTo(0, c_h + 1);
      fHSB->ResizeTo(c_w, sb_h - 1);
      if (fHSB->IsHidden(fHSB)) fHSB->Show();
    }
    else if (!fHSB->IsHidden(fHSB)) fHSB->Hide();
  }

  if (fVSB)
  {
    if (show_v)
    {
      fVSB->MoveTo(c_w + 1, 0);
      fVSB->ResizeTo(sb_w - 1, c_h);
      if (fVSB->IsHidden(fVSB)) fVSB->Show();
    }
    else if (!fVSB->IsHidden(fVSB)) fVSB->Hide();
  }

  iupAttribSet(fIhandle, "XHIDDEN", show_h ? "NO" : "YES");
  iupAttribSet(fIhandle, "YHIDDEN", show_v ? "NO" : "YES");
  iupAttribSetInt(fIhandle, "_IUPHAIKU_CANVAS_INNER_W", (int)(c_w + 1));
  iupAttribSetInt(fIhandle, "_IUPHAIKU_CANVAS_INNER_H", (int)(c_h + 1));
}

static IupHaikuCanvasView* haikuCanvasGetInner(Ihandle* ih)
{
  if (IupHaikuCanvasView* v = (IupHaikuCanvasView*)iupAttribGet(ih, "_IUPHAIKU_CANVAS_INNER"))
    return v;
  return dynamic_cast<IupHaikuCanvasView*>((BView*)ih->handle);
}

static IupHaikuCanvasWrap* haikuCanvasGetWrap(Ihandle* ih)
{
  return dynamic_cast<IupHaikuCanvasWrap*>((BView*)ih->handle);
}

static void haikuCanvasSyncScrollBar(Ihandle* ih, bool horiz)
{
  IupHaikuCanvasWrap* wrap = haikuCanvasGetWrap(ih);
  if (!wrap) return;
  IupHaikuCanvasScrollBar* sb = horiz ? wrap->HSB() : wrap->VSB();
  if (!sb) return;

  double lo = iupAttribGetDouble(ih, horiz ? "XMIN" : "YMIN");
  double hi = iupAttribGetDouble(ih, horiz ? "XMAX" : "YMAX");
  double page = iupAttribGetDouble(ih, horiz ? "DX" : "DY");
  double pos = horiz ? ih->data->posx : ih->data->posy;
  double range = hi - lo;

  LooperLockGuard guard(sb->Looper());
  sb->SetSilent(true);
  if (page >= range || range <= 0)
  {
    sb->SetRange(0, 0);
    sb->SetProportion(1.0f);
    sb->SetValue(0);
  }
  else
  {
    if (pos < lo) pos = lo;
    if (pos > hi - page) pos = hi - page;
    sb->SetRange((float)lo, (float)(hi - page));
    sb->SetProportion((float)(page / range));
    sb->SetSteps((float)(range * 0.05), (float)page);
    sb->SetValue((float)pos);
  }
  sb->SetSilent(false);
}

static int haikuCanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
  IupHaikuCanvasView* canvas = haikuCanvasGetInner(ih);
  if (!canvas) return 1;
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 1;
  rgb_color c = { r, g, b, 255 };
  LooperLockGuard guard(canvas->Looper());
  canvas->SetViewColor(c);
  canvas->Invalidate();
  return 1;
}

static int haikuCanvasMapMethod(Ihandle* ih)
{
  ih->data->sb = iupBaseGetScrollbar(ih);

  IupHaikuCanvasView* canvas = new IupHaikuCanvasView(ih);

  if (ih->data->sb == 0)
  {
    ih->handle = (InativeHandle*)canvas;
    iuphaikuAddToParent(ih);
    return IUP_NOERROR;
  }

  IupHaikuCanvasScrollBar* hsb = (ih->data->sb & IUP_SB_HORIZ) ? new IupHaikuCanvasScrollBar(ih, B_HORIZONTAL) : NULL;
  IupHaikuCanvasScrollBar* vsb = (ih->data->sb & IUP_SB_VERT) ? new IupHaikuCanvasScrollBar(ih, B_VERTICAL) : NULL;

  IupHaikuCanvasWrap* wrap = new IupHaikuCanvasWrap(ih);
  wrap->Attach(canvas, hsb, vsb);

  ih->handle = (InativeHandle*)wrap;
  iupAttribSet(ih, "_IUPHAIKU_CANVAS_INNER", (char*)canvas);
  if (vsb) iupAttribSet(ih, "_IUPHAIKU_CANVAS_VSB", (char*)vsb);
  iuphaikuAddToParent(ih);

  haikuCanvasSyncScrollBar(ih, true);
  haikuCanvasSyncScrollBar(ih, false);
  return IUP_NOERROR;
}

static void haikuCanvasUnMapMethod(Ihandle* ih)
{
  if (IupHaikuCanvasWrap* wrap = haikuCanvasGetWrap(ih))
  {
    if (wrap->CanvasView()) wrap->CanvasView()->SetIhandle(NULL);
    if (wrap->HSB())        wrap->HSB()->SetIhandle(NULL);
    if (wrap->VSB())        wrap->VSB()->SetIhandle(NULL);
  }
  else if (IupHaikuCanvasView* v = dynamic_cast<IupHaikuCanvasView*>((BView*)ih->handle))
  {
    v->SetIhandle(NULL);
  }
  iupAttribSet(ih, "_IUPHAIKU_CANVAS_INNER", NULL);
  iupAttribSet(ih, "_IUPHAIKU_CANVAS_VSB", NULL);
  iupdrvBaseUnMapMethod(ih);
}

static char* haikuCanvasGetDrawSizeAttrib(Ihandle* ih)
{
  IupHaikuCanvasView* inner = haikuCanvasGetInner(ih);
  if (!inner) return NULL;
  BRect b = inner->Bounds();
  return iupStrReturnIntInt((int)(b.Width() + 1), (int)(b.Height() + 1), 'x');
}

static int haikuCanvasSetPosXAttrib(Ihandle* ih, const char* value)
{
  if (!(ih->data->sb & IUP_SB_HORIZ)) return 1;
  double posx, xmin, xmax, dx;
  if (!iupStrToDouble(value, &posx)) return 1;
  xmin = iupAttribGetDouble(ih, "XMIN");
  xmax = iupAttribGetDouble(ih, "XMAX");
  dx   = iupAttribGetDouble(ih, "DX");
  if (dx >= xmax - xmin) return 0;
  if (posx < xmin) posx = xmin;
  if (posx > xmax - dx) posx = xmax - dx;
  ih->data->posx = posx;
  haikuCanvasSyncScrollBar(ih, true);
  return 1;
}

static int haikuCanvasSetPosYAttrib(Ihandle* ih, const char* value)
{
  if (!(ih->data->sb & IUP_SB_VERT)) return 1;
  double posy, ymin, ymax, dy;
  if (!iupStrToDouble(value, &posy)) return 1;
  ymin = iupAttribGetDouble(ih, "YMIN");
  ymax = iupAttribGetDouble(ih, "YMAX");
  dy   = iupAttribGetDouble(ih, "DY");
  if (dy >= ymax - ymin) return 0;
  if (posy < ymin) posy = ymin;
  if (posy > ymax - dy) posy = ymax - dy;
  ih->data->posy = posy;
  haikuCanvasSyncScrollBar(ih, false);
  return 1;
}

static int haikuCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  if (!(ih->data->sb & IUP_SB_HORIZ)) return 1;
  double dx;
  if (!iupStrToDoubleDef(value, &dx, 0.1)) return 1;
  iupAttribSetDouble(ih, "DX", dx);
  haikuCanvasSyncScrollBar(ih, true);
  if (IupHaikuCanvasWrap* wrap = haikuCanvasGetWrap(ih)) wrap->RelayoutChildren();
  return 1;
}

static int haikuCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  if (!(ih->data->sb & IUP_SB_VERT)) return 1;
  double dy;
  if (!iupStrToDoubleDef(value, &dy, 0.1)) return 1;
  iupAttribSetDouble(ih, "DY", dy);
  haikuCanvasSyncScrollBar(ih, false);
  if (IupHaikuCanvasWrap* wrap = haikuCanvasGetWrap(ih)) wrap->RelayoutChildren();
  return 1;
}

static void haikuCanvasLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  if (IupHaikuCanvasWrap* wrap = haikuCanvasGetWrap(ih))
  {
    LooperLockGuard guard(wrap->Looper());
    wrap->RelayoutChildren();
  }
}

static void* haikuCanvasGetInnerNativeContainerMethod(Ihandle* ih, Ihandle* /*child*/)
{
  if (IupHaikuCanvasView* inner = haikuCanvasGetInner(ih))
    return inner;
  return ih->handle;
}

extern "C" IUP_SDK_API void iupdrvCanvasInitClass(Iclass* ic)
{
  ic->Map = haikuCanvasMapMethod;
  ic->UnMap = haikuCanvasUnMapMethod;
  ic->LayoutUpdate = haikuCanvasLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = haikuCanvasGetInnerNativeContainerMethod;

  iupClassRegisterAttribute(ic, "DRAWSIZE", haikuCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BORDER", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAWFONT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, haikuCanvasSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "DX",   NULL,                   haikuCanvasSetDXAttrib,   NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY",   NULL,                   haikuCanvasSetDYAttrib,   NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, haikuCanvasSetPosXAttrib, "0",  NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, haikuCanvasSetPosYAttrib, "0",  NULL, IUPAF_NO_INHERIT);
}
