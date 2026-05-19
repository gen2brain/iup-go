/** \file
 * \brief Haiku Popover (BWindow with B_FLOATING_APP_WINDOW_FEEL)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdio>

#include <InterfaceDefs.h>
#include <Message.h>
#include <Messenger.h>
#include <Point.h>
#include <Rect.h>
#include <View.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_layout.h"
#include "iup_popover.h"
}

#include "iuphaiku_drv.h"


class IupHaikuPopoverRoot : public BView
{
public:
  explicit IupHaikuPopoverRoot(BRect frame)
    : BView(frame, "iup_popover_root", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
  {
    SetViewUIColor(B_MENU_BACKGROUND_COLOR);
  }

  void Draw(BRect /*update*/) override
  {
    BRect b = Bounds();
    SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
    StrokeRect(b);
  }
};


class IupHaikuPopover : public BWindow
{
public:
  explicit IupHaikuPopover(Ihandle* ih)
    : BWindow(BRect(0, 0, 99, 99), "iup_popover",
              B_NO_BORDER_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
              B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE),
      fIhandle(ih), fRootView(NULL)
  {
    fRootView = new IupHaikuPopoverRoot(Bounds());
    AddChild(fRootView);
  }

  void WindowActivated(bool active) override
  {
    BWindow::WindowActivated(active);
    if (active || !fIhandle) return;
    if (!iupAttribGetBoolean(fIhandle, "AUTOHIDE")) return;
    BMessenger(this).SendMessage('IuPH');  /* defer hide so the click finishes */
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == 'IuPH' && fIhandle && iupObjectCheck(fIhandle))
    {
      /* Use cached rect; cross-window Lock on the anchor's looper here can AB-BA. */
      const char* rect = iupAttribGet(fIhandle, "_IUPHAIKU_POPOVER_ANCHOR_RECT");
      if (rect)
      {
        int x = 0, y = 0, w = 0, h = 0;
        if (sscanf(rect, "%d %d %d %d", &x, &y, &w, &h) == 4)
        {
          BPoint cur;
          uint32 buttons = 0;
          get_mouse(&cur, &buttons);
          BRect r(x, y, x + w - 1, y + h - 1);
          if (r.Contains(cur)) return;
        }
      }
      IupSetAttribute(fIhandle, "VISIBLE", "NO");
      return;
    }
    BWindow::MessageReceived(msg);
  }

  BView* RootView() const { return fRootView; }
  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

private:
  Ihandle* fIhandle;
  BView* fRootView;
};


static void* haikuPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* /*child*/)
{
  IupHaikuPopover* p = (IupHaikuPopover*)ih->handle;
  return p ? (void*)p->RootView() : NULL;
}

static int haikuPopoverMapMethod(Ihandle* ih)
{
  IupHaikuPopover* popover = new IupHaikuPopover(ih);
  ih->handle = (InativeHandle*)popover;
  return IUP_NOERROR;
}

static void haikuPopoverUnMapMethod(Ihandle* ih)
{
  IupHaikuPopover* popover = (IupHaikuPopover*)ih->handle;
  /* Quit() UnlockFully()s + deletes the looper; LooperLockGuard would unlock freed memory. */
  if (popover && popover->Lock())
  {
    popover->SetIhandle(NULL);
    popover->Quit();  /* deletes the window + its child views */
  }
  ih->handle = NULL;
}

static void haikuPopoverLayoutUpdateMethod(Ihandle* ih)
{
  IupHaikuPopover* popover = (IupHaikuPopover*)ih->handle;
  if (!popover) return;

  int w = ih->currentwidth  > 0 ? ih->currentwidth  : 1;
  int h = ih->currentheight > 0 ? ih->currentheight : 1;

  LooperLockGuard guard(popover);
  popover->ResizeTo((float)(w - 1), (float)(h - 1));
}

/* SetVisible drives placement: SHOW reads ANCHOR via core helper, otherwise Hide(). */

static void haikuPopoverSetVisible(Ihandle* ih, int visible)
{
  if (!visible)
  {
    IupHaikuPopover* popover = (IupHaikuPopover*)ih->handle;
    if (!popover) return;
    LooperLockGuard guard(popover);
    if (!popover->IsHidden()) popover->Hide();
    return;
  }

  Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
  if (!anchor) anchor = IupGetAttributeHandle(ih, "ANCHOR");
  if (!anchor || !anchor->handle) return;

  if (!ih->handle && IupMap(ih) == IUP_ERROR) return;

  if (ih->firstchild && ih->firstchild->handle)
  {
    iupLayoutCompute(ih);
    iupLayoutUpdate(ih->firstchild);
  }

  IupHaikuPopover* popover = (IupHaikuPopover*)ih->handle;
  if (!popover) return;

  int ax = 0, ay = 0, aw = 0, ah = 0;
  if (anchor->iclass && anchor->iclass->nativetype == IUP_TYPECONTROL)
  {
    BView* av = (BView*)anchor->handle;
    LooperLockGuard guard(av->Looper());
    BPoint origin = av->ConvertToScreen(BPoint(0, 0));
    BRect bounds = av->Bounds();
    ax = (int)origin.x;
    ay = (int)origin.y;
    aw = (int)(bounds.Width() + 1);
    ah = (int)(bounds.Height() + 1);
  }
  iupAttribSetStrf(ih, "_IUPHAIKU_POPOVER_ANCHOR_RECT", "%d %d %d %d", ax, ay, aw, ah);

  int pw = ih->currentwidth  > 0 ? ih->currentwidth  : ih->naturalwidth;
  int ph = ih->currentheight > 0 ? ih->currentheight : ih->naturalheight;
  if (pw < 1) pw = 1;
  if (ph < 1) ph = 1;

  /* Core helper handles all 12 POSITION variants + AUTOFLIP + OFFSETX/Y. */
  int px = 0, py = 0;
  iupPopoverCalcPosition(ih, ax, ay, aw, ah, pw, ph, &px, &py);

  /* Show() unlocks the looper internally via Run(); keep it outside the guard. */
  {
    LooperLockGuard guard(popover);
    popover->MoveTo((float)px, (float)py);
    popover->ResizeTo((float)(pw - 1), (float)(ph - 1));
  }
  if (popover->IsHidden()) popover->Show();
}

static int haikuPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  haikuPopoverSetVisible(ih, iupStrBoolean(value));
  return 1;
}

static char* haikuPopoverGetVisibleAttrib(Ihandle* ih)
{
  IupHaikuPopover* popover = (IupHaikuPopover*)ih->handle;
  if (!popover) return iupStrReturnBoolean(0);
  return iupStrReturnBoolean(popover->IsHidden() ? 0 : 1);
}

extern "C" IUP_SDK_API void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = haikuPopoverMapMethod;
  ic->UnMap = haikuPopoverUnMapMethod;
  ic->LayoutUpdate = haikuPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = haikuPopoverGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "VISIBLE", haikuPopoverGetVisibleAttrib, haikuPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_SAVE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ARROW", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
