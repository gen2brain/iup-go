/** \file
 * \brief Haiku Dialog (BWindow)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <Application.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Path.h>
#include <Rect.h>
#include <Roster.h>
#include <Screen.h>
#include <View.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_drv.h"
#include "iup_globalattrib.h"
#include "iup_key.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_str.h"
#include "iup_dlglist.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
}

#include "iuphaiku_drv.h"


static Ihandle* haikuDialogFindChildAt(Ihandle* ih, int x, int y)
{
  if (!ih) return nullptr;
  for (Ihandle* c = ih->firstchild; c; c = c->brother)
  {
    if (x >= c->x && x < c->x + c->currentwidth &&
        y >= c->y && y < c->y + c->currentheight)
    {
      Ihandle* deeper = haikuDialogFindChildAt(c, x, y);
      return deeper ? deeper : c;
    }
  }
  return nullptr;
}

static Ihandle* haikuDialogFindDropTarget(Ihandle* dlg, int x, int y)
{
  Ihandle* target = haikuDialogFindChildAt(dlg, x, y);
  while (target && !IupGetCallback(target, "DROPFILES_CB"))
    target = target->parent;
  return target ? target : dlg;
}


class IupHaikuRootView : public BView
{
public:
  IupHaikuRootView(BRect frame, Ihandle* ih)
    : BView(frame, "iup_root", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
      fBgImage(nullptr), fZoom(false), fIhandle(ih), fMouseInside(false)
  {
    SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
  }

  void SetBackgroundImage(BBitmap* bm, bool zoom) { fBgImage = bm; fZoom = zoom; Invalidate(); }

  void AttachedToWindow() override
  {
    BView::AttachedToWindow();
    /* all moves, even over children, so it tracks the window edge, not per-view transit */
    SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
  }

  void MouseMoved(BPoint where, uint32 transit, const BMessage* drag) override
  {
    bool inside = Bounds().Contains(where);
    if (inside && !fMouseInside)
    {
      fMouseInside = true;
      IFn cb = IupGetCallback(fIhandle, "ENTERWINDOW_CB");
      if (cb) cb(fIhandle);
    }
    else if (!inside && fMouseInside)
    {
      fMouseInside = false;
      IFn cb = IupGetCallback(fIhandle, "LEAVEWINDOW_CB");
      if (cb) cb(fIhandle);
    }
    BView::MouseMoved(where, transit, drag);
  }

  void Draw(BRect update) override
  {
    if (!fBgImage) { BView::Draw(update); return; }
    SetDrawingMode(B_OP_COPY);
    if (fZoom)
    {
      DrawBitmap(fBgImage, fBgImage->Bounds(), Bounds());
      return;
    }
    BRect r = Bounds();
    float bw = fBgImage->Bounds().Width() + 1;
    float bh = fBgImage->Bounds().Height() + 1;
    for (float y = 0; y < r.Height() + 1; y += bh)
      for (float x = 0; x < r.Width() + 1; x += bw)
        DrawBitmap(fBgImage, BPoint(x, y));
  }

private:
  BBitmap* fBgImage;  /* borrowed; lives in IUP's image cache */
  bool fZoom;
  Ihandle* fIhandle;
  bool fMouseInside;
};


class IupHaikuCommandKeyFilter : public BMessageFilter
{
public:
  IupHaikuCommandKeyFilter() : BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN) {}

  filter_result Filter(BMessage* msg, BHandler** target) override
  {
    int32 mods = 0;
    const char* bytes = nullptr;
    if (msg->FindInt32("modifiers", &mods) != B_OK || !(mods & B_COMMAND_KEY))
      return B_DISPATCH_MESSAGE;
    if (msg->FindString("bytes", &bytes) != B_OK || !bytes)
      return B_DISPATCH_MESSAGE;

    Ihandle* ih = IupGetFocus();
    if (!ih) return B_DISPATCH_MESSAGE;

    int32 raw_char = 0, raw_key = 0;
    msg->FindInt32("raw_char", &raw_char);
    msg->FindInt32("key", &raw_key);

    int code = iuphaikuKeyDecode(static_cast<unsigned char>(bytes[0]), static_cast<int>(raw_char), static_cast<int>(raw_key), static_cast<unsigned>(mods));
    if (!code) return B_DISPATCH_MESSAGE;

    int press = iupKeyCallKeyPressCb(ih, code, 1);
    int any = iupKeyCallKeyCb(ih, code);
    if (press == IUP_CLOSE || any == IUP_CLOSE) { IupExitLoop(); return B_SKIP_MESSAGE; }
    if (press == IUP_IGNORE || any == IUP_IGNORE) return B_SKIP_MESSAGE;

    int nav_code = code;
    if (iup_isShiftXkey(code) && iup_XkeyBase(code) == K_TAB)
      nav_code = iup_XkeyCtrl(K_TAB);

    if (iupKeyProcessNavigation(ih, nav_code, mods & B_SHIFT_KEY))
      return B_SKIP_MESSAGE;

    (void)target;
    return B_DISPATCH_MESSAGE;
  }
};

class IupHaikuWindow : public BWindow
{
public:
  IupHaikuWindow(BRect frame, const char* title, window_look look,
                 window_feel feel, uint32 flags, Ihandle* ih)
    : BWindow(frame, title ? title : "", look, feel,
              flags | B_ASYNCHRONOUS_CONTROLS),
      fIhandle(ih), fRootView(nullptr), fSavedLook(look), fIsFullScreen(false),
      fMoveRunner(nullptr)
  {
    fRootView = new IupHaikuRootView(Bounds(), ih);
    AddChild(fRootView);
    AddCommonFilter(new IupHaikuCommandKeyFilter());
  }

  /* BMenuField::~BMenuField deletes its own filter without unlinking it; ~BLooper would double-free */
  ~IupHaikuWindow() override
  {
    delete fMoveRunner;
    if (BList* list = CommonFilterList())
      for (int32 i = list->CountItems() - 1; i >= 0; --i)
        BLooper::RemoveCommonFilter(static_cast<BMessageFilter*>(list->ItemAt(i)));
  }

  void Minimize(bool minimize) override
  {
    BWindow* children[64];
    int count = 0;

    BWindow::Minimize(minimize);

    for (Ihandle* dlg = iupDlgListFirst(); dlg && count < 64; dlg = iupDlgListNext())
    {
      if (dlg->handle && dlg->handle != reinterpret_cast<InativeHandle*>(this) && iupDialogGetNativeParent(dlg) == reinterpret_cast<InativeHandle*>(this))
        children[count++] = reinterpret_cast<BWindow*>(dlg->handle);
    }

    for (int i = 0; i < count; i++)
    {
      LooperLockGuard guard(children[i]);
      children[i]->Minimize(minimize);
    }
  }

  bool QuitRequested() override
  {
    if (!fIhandle || !iupObjectCheck(fIhandle) || !iupdrvIsActive(fIhandle))
      return false;

    Icallback cb = IupGetCallback(fIhandle, "CLOSE_CB");
    int ret = IUP_DEFAULT;
    if (cb) ret = cb(fIhandle);

    if (ret == IUP_IGNORE)     return false;
    if (ret == IUP_CLOSE)      IupExitLoop();
    else                       IupHide(fIhandle);
    return false;
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_WINDOW_QUIT)
    {
      Quit();
      return;
    }

    if (msg && msg->what == IUPHAIKU_MOVE_SETTLED)
    {
      delete fMoveRunner;
      fMoveRunner = nullptr;
      if (fIhandle && iupObjectCheck(fIhandle))
      {
        auto cb = reinterpret_cast<IFnii>(IupGetCallback(fIhandle, "MOVE_CB"));
        if (cb) cb(fIhandle, static_cast<int>(fLastMovePos.x), static_cast<int>(fLastMovePos.y));
      }
      return;
    }
    if (msg && msg->what == IUPHAIKU_THEME_CHANGED_MSG && fIhandle && iupObjectCheck(fIhandle))
    {
      iupGlobalNotifyThemeChanged();
      return;
    }
    if (msg && (msg->what == B_SIMPLE_DATA || msg->what == B_REFS_RECEIVED)
        && msg->HasRef("refs") && fIhandle && iupObjectCheck(fIhandle))
    {
      BPoint pt = ConvertFromScreen(msg->DropPoint());

      Ihandle* target = haikuDialogFindDropTarget(fIhandle, static_cast<int>(pt.x), static_cast<int>(pt.y));
      auto cb = reinterpret_cast<IFnsiii>(IupGetCallback(target, "DROPFILES_CB"));
      if (cb)
      {
        int lx = static_cast<int>(pt.x) - target->x;
        int ly = static_cast<int>(pt.y) - target->y;

        entry_ref ref;
        int32 count = 0;
        for (; msg->FindRef("refs", count, &ref) == B_OK; ++count) {}
        for (int32 i = 0; i < count; ++i)
        {
          if (msg->FindRef("refs", i, &ref) != B_OK) continue;
          BPath path;
          if (BEntry(&ref).GetPath(&path) != B_OK) continue;
          int ret = cb(target, const_cast<char*>(path.Path()), count - i - 1, lx, ly);
          if (ret == IUP_IGNORE) break;
          if (ret == IUP_CLOSE) { IupExitLoop(); break; }
        }
        return;
      }
    }
    if (msg && msg->what == IUPHAIKU_MENU_ITEM_MSG)
    {
      Ihandle* item_ih = nullptr;
      msg->FindPointer("ih", reinterpret_cast<void**>(&item_ih));
      if (item_ih && iupObjectCheck(item_ih))
      {
        /* AUTOTOGGLE: clicking the item flips VALUE before ACTION fires. */
        if (iupAttribGetBoolean(item_ih, "AUTOTOGGLE"))
        {
          int v = iupStrBoolean(iupAttribGet(item_ih, "VALUE"));
          IupSetAttribute(item_ih, "VALUE", v ? "OFF" : "ON");
        }

        Icallback cb = IupGetCallback(item_ih, "ACTION");
        if (cb)
        {
          int ret = cb(item_ih);
          if (ret == IUP_CLOSE) IupExitLoop();
        }
      }
      return;
    }
    if (msg && msg->what == IUPHAIKU_MENU_RECENT_MSG)
    {
      Ihandle* menu_ih = nullptr;
      int32 index = -1;
      msg->FindPointer("menu", reinterpret_cast<void**>(&menu_ih));
      msg->FindInt32("index", &index);
      iuphaikuRecentDispatch(menu_ih, index);
      return;
    }
    if (msg && msg->what == IUPHAIKU_MENU_CB_MSG)
    {
      Ihandle* ih = nullptr;
      const char* cb_name = nullptr;
      msg->FindPointer("ih", reinterpret_cast<void**>(&ih));
      msg->FindString("cb", &cb_name);
      if (ih && cb_name && iupObjectCheck(ih))
      {
        Icallback cb = IupGetCallback(ih, cb_name);
        if (!cb && ih->parent)
          cb = IupGetCallback(ih->parent, cb_name);
        if (cb) cb(ih);
      }
      return;
    }
    if (msg && msg->what == IUPHAIKU_TIMER_HOP_MSG)
    {
      int32 serial = -1;
      msg->FindInt32("serial", &serial);
      Ihandle* ih = iuphaikuTimerFromSerial(static_cast<int>(serial));
      if (ih && iupObjectCheck(ih))
      {
        Icallback cb = IupGetCallback(ih, "ACTION_CB");
        if (cb && cb(ih) == IUP_CLOSE) IupExitLoop();
      }
      return;
    }
    if (msg && msg->what == IUPHAIKU_POST_MSG)
    {
      Ihandle* ih = nullptr;
      const char* s = nullptr;
      int32 i32 = 0;
      double d = 0;
      void* p = nullptr;
      msg->FindPointer("ih", reinterpret_cast<void**>(&ih));
      msg->FindString("s", &s);
      msg->FindInt32("i", &i32);
      msg->FindDouble("d", &d);
      msg->FindPointer("p", &p);
      if (ih && iupObjectCheck(ih))
      {
        auto cb = reinterpret_cast<IFnsidv>(IupGetCallback(ih, "POSTMESSAGE_CB"));
        if (cb) cb(ih, const_cast<char*>(s), static_cast<int>(i32), d, p);
      }
      return;
    }
    BWindow::MessageReceived(msg);
  }

  void DispatchMessage(BMessage* msg, BHandler* target) override
  {
    if (iuphaikuIsModalStaleInput(msg))
      return;

    if (msg && iupStrBoolean(IupGetGlobal("INPUTCALLBACKS")))
      iuphaikuFireGlobalInputCB(msg);

    /* Command+letter activates a mnemonic; Command is the Haiku shortcut modifier, IUP reports it as Ctrl. */
    if (msg && msg->what == B_KEY_DOWN && fIhandle && iupObjectCheck(fIhandle))
    {
      int32 mods = 0, raw = 0;
      msg->FindInt32("modifiers", &mods);
      msg->FindInt32("raw_char", &raw);
      if ((mods & B_COMMAND_KEY) && !(mods & (B_CONTROL_KEY | B_OPTION_KEY)))
      {
        if (raw >= 'a' && raw <= 'z') raw = raw - 'a' + 'A';
        if (raw >= 'A' && raw <= 'Z' && iupKeyProcessMnemonic(fIhandle, raw))
          return;
      }
    }

    BWindow::DispatchMessage(msg, target);
  }

  void FrameMoved(BPoint where) override
  {
    BWindow::FrameMoved(where);
    if (!fIhandle) return;
    /* opaque drags stream B_WINDOW_MOVED; debounce to one MOVE_CB once it settles */
    fLastMovePos = where;
    delete fMoveRunner;
    BMessage settle(IUPHAIKU_MOVE_SETTLED);
    fMoveRunner = new BMessageRunner(BMessenger(this), &settle, 150000, 1);
  }

  void Zoom(BPoint origin, float width, float height) override
  {
    BWindow::Zoom(origin, width, height);
    if (!fIhandle || !iupObjectCheck(fIhandle)) return;
    iupAttribSet(fIhandle, "MAXIMIZED", iupAttribGetBoolean(fIhandle, "MAXIMIZED") ? nullptr : const_cast<char*>("Yes"));
  }

  void FrameResized(float w, float h) override
  {
    BWindow::FrameResized(w, h);
    if (!fIhandle || fIhandle->data->ignore_resize) return;

    iupdrvDialogGetSize(fIhandle, nullptr, &fIhandle->currentwidth, &fIhandle->currentheight);

    auto cb = reinterpret_cast<IFnii>(IupGetCallback(fIhandle, "RESIZE_CB"));
    if (!cb || cb(fIhandle, static_cast<int>(w + 1), static_cast<int>(h + 1)) != IUP_IGNORE)
    {
      fIhandle->data->ignore_resize = 1;
      IupRefresh(fIhandle);
      fIhandle->data->ignore_resize = 0;
    }
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }
  BView* RootView() const { return fRootView; }

  /* Fullscreen save/restore (no native SetFullScreen on this Haiku build). */
  bool IsFullScreen() const { return fIsFullScreen; }
  void SaveForFullScreen()
  {
    fSavedLook = Look();
    fSavedFrame = Frame();
    fIsFullScreen = true;
  }
  window_look SavedLook() const { return fSavedLook; }
  BRect SavedFrame() const { return fSavedFrame; }
  void ClearFullScreen() { fIsFullScreen = false; }

private:
  Ihandle* fIhandle;
  BView* fRootView;
  window_look fSavedLook;
  BRect fSavedFrame;
  bool fIsFullScreen;
  BMessageRunner* fMoveRunner;
  BPoint fLastMovePos;
};


static window_look haikuDialogResolveLook(Ihandle* ih)
{
  if (!iupAttribGetBoolean(ih, "BORDER")) return B_NO_BORDER_WINDOW_LOOK;
  if (iupAttribGetBoolean(ih, "CUSTOMFRAME")) return B_NO_BORDER_WINDOW_LOOK;
  if (iupAttribGetBoolean(ih, "HIDETITLEBAR")) return B_BORDERED_WINDOW_LOOK;
  if (iupAttribGetBoolean(ih, "TOOLBOX")) return B_FLOATING_WINDOW_LOOK;
  return B_TITLED_WINDOW_LOOK;
}

static window_feel haikuDialogResolveFeel(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "TOPMOST")) return B_FLOATING_ALL_WINDOW_FEEL;
  if (iupDialogGetNativeParent(ih)) return B_FLOATING_SUBSET_WINDOW_FEEL;
  return B_NORMAL_WINDOW_FEEL;
}

static void haikuDialogUpdateFeel(Ihandle* ih, IupHaikuWindow* win)
{
  window_feel feel = haikuDialogResolveFeel(ih);
  if (iupAttribGet(ih, "_IUPHAIKU_SAVED_FEEL"))
    iupAttribSetInt(ih, "_IUPHAIKU_SAVED_FEEL", static_cast<int>(feel));
  else
    win->SetFeel(feel);
}

static uint32 haikuDialogResolveFlags(Ihandle* ih)
{
  uint32 flags = 0;
  if (!iupAttribGetBoolean(ih, "RESIZE"))  flags |= B_NOT_RESIZABLE;
  if (!iupAttribGetBoolean(ih, "MINBOX"))  flags |= B_NOT_MINIMIZABLE;
  if (!iupAttribGetBoolean(ih, "MAXBOX"))  flags |= B_NOT_ZOOMABLE;
  if (!iupAttribGetBoolean(ih, "MENUBOX")) flags |= B_NOT_CLOSABLE;
  return flags;
}

static int haikuDialogMapMethod(Ihandle* ih)
{
  const char* title = iupAttribGet(ih, "TITLE");

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME"))
    iupDialogCustomFrameSimulateCheckCallbacks(ih);

  window_look look = haikuDialogResolveLook(ih);
  window_feel feel = haikuDialogResolveFeel(ih);
  uint32 flags = haikuDialogResolveFlags(ih);

  auto* win = new IupHaikuWindow(BRect(0, 0, 99, 99),
                                           title, look, feel, flags, ih);
  ih->handle = reinterpret_cast<InativeHandle*>(win);

  {
    InativeHandle* parent = iupDialogGetNativeParent(ih);
    if (parent)
      iupdrvDialogSetParent(ih, parent);
  }

  return IUP_NOERROR;
}

static void haikuDialogUnMapMethod(Ihandle* ih)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);

  if (ih->data->menu)
  {
    IupDestroy(ih->data->menu);
    ih->data->menu = nullptr;
  }

  if (win && be_app)
  {
    for (Ihandle* dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
    {
      if (dlg != ih && dlg->handle && iupDialogGetNativeParent(dlg) == reinterpret_cast<InativeHandle*>(win))
      {
        BMessage destroy(IUPHAIKU_APP_DESTROY_DLG);
        destroy.AddPointer("ih", dlg);
        BMessenger(be_app).SendMessage(&destroy);
      }
    }
  }

  if (win && win->Thread() == find_thread(nullptr))
  {
    /* Quit() from the window's own thread never returns, so the callback running here would be cut off */
    win->SetIhandle(nullptr);
    win->Hide();
    win->PostMessage(IUPHAIKU_WINDOW_QUIT);
  }
  else if (win && win->Lock())
  {
    win->SetIhandle(nullptr);
    win->Quit();
  }
  ih->handle = nullptr;
  iuphaikuPostAppWake();
}

static void* haikuDialogGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* /*child*/)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  return win ? reinterpret_cast<void*>(win->RootView()) : nullptr;
}

/* ih->currentwidth/height is full-window size; ResizeTo sizes the content. */
static void haikuDialogContentSize(IupHaikuWindow* win, int total_w, int total_h, int* cw, int* ch)
{
  BRect outer = win->DecoratorFrame();
  BRect inner = win->Frame();
  int decor_w = static_cast<int>(outer.Width() - inner.Width() + 0.5f);
  int decor_h = static_cast<int>(outer.Height() - inner.Height() + 0.5f);
  int w = total_w - decor_w;
  int h = total_h - decor_h;
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  *cw = w;
  *ch = h;
}

static void haikuDialogLayoutUpdateMethod(Ihandle* ih)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win || ih->data->ignore_resize) return;

  ih->data->ignore_resize = 1;
  {
    LooperLockGuard guard(win);
    int total_w = ih->currentwidth  > 0 ? ih->currentwidth  : 1;
    int total_h = ih->currentheight > 0 ? ih->currentheight : 1;
    int cw, ch;
    haikuDialogContentSize(win, total_w, total_h, &cw, &ch);
    win->ResizeTo(static_cast<float>(cw - 1), static_cast<float>(ch - 1));
  }
  ih->data->ignore_resize = 0;
}

extern "C" IUP_SDK_API void iupdrvDialogGetDecoration(Ihandle* ih, int* border, int* caption, int* menu)
{
  if (border) *border = 0;
  if (caption) *caption = 0;
  if (menu) *menu = 0;

  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return;

  if (menu && ih->data && ih->data->menu)
    *menu = iupdrvMenuGetMenuBarSize(ih->data->menu);

  /* IUP formula: 2*border + caption + menu, so caption excludes the top border. */
  BRect outer = win->DecoratorFrame();
  BRect inner = win->Frame();
  int top_decor = static_cast<int>(inner.top - outer.top + 0.5f);
  int bottom_border = static_cast<int>(outer.bottom - inner.bottom + 0.5f);
  if (border)  *border  = static_cast<int>((outer.Width() - inner.Width()) / 2.0f + 0.5f);
  if (caption) *caption = top_decor - bottom_border;
}

IUP_DRV_API BView* iuphaikuDialogRootView(BWindow* win)
{
  auto* iwin = dynamic_cast<IupHaikuWindow*>(win);
  return iwin ? iwin->RootView() : nullptr;
}

extern "C" IUP_SDK_API void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* native_parent)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win || !native_parent) return;

  LooperLockGuard guard(win);
  win->AddToSubset(reinterpret_cast<BWindow*>(native_parent));
  haikuDialogUpdateFeel(ih, win);
}

extern "C" IUP_SDK_API void iupdrvDialogGetPosition(Ihandle* ih, InativeHandle* handle, int* x, int* y)
{
  if (x) *x = 0;
  if (y) *y = 0;

  auto* win = reinterpret_cast<IupHaikuWindow*>(handle ? handle : ih->handle);
  if (!win) return;

  /* IUP position is full-window top-left, BWindow::Frame is content. */
  BRect outer = win->DecoratorFrame();
  if (x) *x = static_cast<int>(outer.left);
  if (y) *y = static_cast<int>(outer.top);
}

extern "C" IUP_SDK_API void iupdrvDialogSetPosition(Ihandle* ih, int x, int y)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return;

  /* MoveTo targets content top-left; offset by decor so outer lands at (x, y). */
  LooperLockGuard guard(win);
  BRect outer = win->DecoratorFrame();
  BRect inner = win->Frame();
  int dx = static_cast<int>(inner.left - outer.left + 0.5f);
  int dy = static_cast<int>(inner.top  - outer.top  + 0.5f);
  win->MoveTo(static_cast<float>(x + dx), static_cast<float>(y + dy));
}

extern "C" IUP_SDK_API void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int* w, int* h)
{
  if (w) *w = 0;
  if (h) *h = 0;

  auto* win = reinterpret_cast<IupHaikuWindow*>(handle ? handle : ih->handle);
  if (!win) return;

  BRect outer = win->DecoratorFrame();
  if (w) *w = static_cast<int>(outer.Width() + 1);
  if (h) *h = static_cast<int>(outer.Height() + 1);
}

extern "C" IUP_SDK_API int iupdrvDialogIsVisible(Ihandle* ih)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 0;
  return win->IsHidden() ? 0 : 1;
}

extern "C" IUP_SDK_API void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return;

  if (visible)
  {
    int total_w = ih->currentwidth  > 0 ? ih->currentwidth  : 100;
    int total_h = ih->currentheight > 0 ? ih->currentheight : 100;

    /* Show() unlocks the looper internally via Run(); keep it outside this guard. */
    {
      LooperLockGuard guard(win);
      int cw, ch;
      haikuDialogContentSize(win, total_w, total_h, &cw, &ch);
      win->ResizeTo(static_cast<float>(cw - 1), static_cast<float>(ch - 1));

      if (iupAttribGetBoolean(ih, "MODAL") &&
          !iupAttribGet(ih, "_IUPHAIKU_SAVED_FEEL"))
      {
        iupAttribSetInt(ih, "_IUPHAIKU_SAVED_FEEL", static_cast<int>(win->Feel()));
        window_feel modal_feel = (win->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL)
                                   ? B_MODAL_SUBSET_WINDOW_FEEL
                                   : B_MODAL_APP_WINDOW_FEEL;
        win->SetFeel(modal_feel);
      }
    }

    if (win->IsHidden())
    {
      /* Defer only when SHOW_CB would race the BWindow looper for the win lock. */
      bool defer = be_app && be_app->IsLaunching() && IupGetCallback(ih, "SHOW_CB");
      if (defer)
      {
        BMessage post(IUPHAIKU_APP_SHOW_WIN);
        post.AddPointer("win", win);
        BMessenger(be_app).SendMessage(&post);
      }
      else
        win->Show();
    }
  }
  else
  {
    {
      LooperLockGuard guard(win);
      if (!win->IsHidden()) win->Hide();

      char* saved = iupAttribGet(ih, "_IUPHAIKU_SAVED_FEEL");
      if (saved)
      {
        auto f = static_cast<window_feel>(iupAttribGetInt(ih, "_IUPHAIKU_SAVED_FEEL"));
        win->SetFeel(f);
        iupAttribSet(ih, "_IUPHAIKU_SAVED_FEEL", nullptr);
      }
    }
    iuphaikuPostAppWake();
  }
}

extern "C" IUP_SDK_API int iupdrvDialogSetPlacement(Ihandle* ih)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 0;

  ih->data->show_state = IUP_SHOW;

  char* placement = iupAttribGet(ih, "PLACEMENT");

  if (iupStrEqualNoCase(placement, "MAXIMIZED") ||
      iupStrEqualNoCase(placement, "FULL"))
  {
    /* No looper lock: the helper re-enters via IupRefresh/SetPosition. */
    iupDialogCustomFrameMaximize(ih);
    ih->data->show_state = IUP_MAXIMIZE;
  }
  else if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    LooperLockGuard guard(win);
    win->Minimize(true);
    ih->data->show_state = IUP_MINIMIZE;
  }
  else  /* NULL / empty / NORMAL: restore from a simulated maximize */
  {
    {
      LooperLockGuard guard(win);
      if (win->IsMinimized()) win->Minimize(false);
    }
    if (!iupDialogCustomFrameRestore(ih))
      return 0;
    ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", "NORMAL");
  return 1;
}

static int haikuDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;
  LooperLockGuard guard(win);
  win->SetTitle(value ? value : "");
  return 1;
}

static char* haikuDialogGetTitleAttrib(Ihandle* ih)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return nullptr;
  return iupStrReturnStr(win->Title());
}

static int haikuDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;
  int w = 1, h = 1;
  iupStrToIntInt(value, &w, &h, 'x');

  LooperLockGuard guard(win);
  float minW, maxW, minH, maxH;
  win->GetSizeLimits(&minW, &maxW, &minH, &maxH);
  win->SetSizeLimits(static_cast<float>(w - 1), maxW, static_cast<float>(h - 1), maxH);
  return 1;
}

static int haikuDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;
  int w = 65535, h = 65535;
  iupStrToIntInt(value, &w, &h, 'x');

  LooperLockGuard guard(win);
  float minW, maxW, minH, maxH;
  win->GetSizeLimits(&minW, &maxW, &minH, &maxH);
  win->SetSizeLimits(minW, static_cast<float>(w - 1), minH, static_cast<float>(h - 1));
  return 1;
}

static int haikuDialogSetResizeAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;
  iupAttribSet(ih, "RESIZE", iupStrBoolean(value) ? "YES" : "NO");

  LooperLockGuard guard(win);
  uint32 flags = win->Flags();
  if (iupStrBoolean(value)) flags &= ~B_NOT_RESIZABLE;
  else                       flags |= B_NOT_RESIZABLE;
  win->SetFlags(flags);
  return 1;
}

static int haikuDialogSetHideTitleBarAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;
  iupAttribSet(ih, "HIDETITLEBAR", iupStrBoolean(value) ? "YES" : "NO");
  LooperLockGuard guard(win);
  win->SetLook(haikuDialogResolveLook(ih));
  return 1;
}

static int haikuDialogSetCustomFrameAttrib(Ihandle* ih, const char* value)
{
  iupAttribSet(ih, "CUSTOMFRAME", iupStrBoolean(value) ? "YES" : "NO");
  if (iupStrBoolean(value))
    iupDialogCustomFrameSimulateCheckCallbacks(ih);
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;
  LooperLockGuard guard(win);
  win->SetLook(haikuDialogResolveLook(ih));
  return 1;
}

static int haikuDialogSetTopMostAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  iupAttribSetStr(ih, "TOPMOST", value);
  if (!win) return 1;
  LooperLockGuard guard(win);
  haikuDialogUpdateFeel(ih, win);
  return 1;
}

static int haikuDialogSetBringFrontAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win || !iupStrBoolean(value)) return 0;
  LooperLockGuard guard(win);
  win->Activate(true);
  return 0;
}

static int haikuDialogSetBgColorAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  rgb_color c = { r, g, b, 255 };
  LooperLockGuard guard(win);
  win->RootView()->SetViewColor(c);
  win->RootView()->SetHighColor(iuphaikuColor(B_PANEL_TEXT_COLOR));
  win->RootView()->Invalidate();
  return 1;
}

static int haikuDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;
  auto* root = dynamic_cast<IupHaikuRootView*>(win->RootView());
  if (!root) return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    rgb_color c = { r, g, b, 255 };
    LooperLockGuard guard(win);
    root->SetBackgroundImage(nullptr, false);
    root->SetViewColor(c);
    root->Invalidate();
    return 1;
  }

  auto* bm = static_cast<BBitmap*>(iupImageGetImage(value, ih, 0, nullptr));
  if (!bm) return 0;
  LooperLockGuard guard(win);
  root->SetBackgroundImage(bm, iupAttribGetBoolean(ih, "BACKIMAGEZOOM") ? true : false);
  return 1;
}

static int haikuDialogSetBackImageZoomAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;
  char* bg = iupAttribGet(ih, "BACKGROUND");
  if (bg) haikuDialogSetBackgroundAttrib(ih, bg);
  return 1;
}

static int haikuDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;

  LooperLockGuard guard(win);
  bool wantFs = iupStrBoolean(value) ? true : false;
  if (wantFs == win->IsFullScreen()) return 1;

  if (wantFs)
  {
    win->SaveForFullScreen();
    BScreen screen(win);
    BRect sf = screen.Frame();
    win->SetLook(B_NO_BORDER_WINDOW_LOOK);
    win->MoveTo(sf.LeftTop());
    win->ResizeTo(sf.Width(), sf.Height());
  }
  else
  {
    win->SetLook(win->SavedLook());
    BRect saved = win->SavedFrame();
    win->MoveTo(saved.LeftTop());
    win->ResizeTo(saved.Width(), saved.Height());
    win->ClearFullScreen();
  }
  return 1;
}

static char* haikuDialogGetClientSizeAttrib(Ihandle* ih)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return nullptr;
  LooperLockGuard guard(win);
  BRect b = win->RootView()->Bounds();
  return iupStrReturnIntInt(static_cast<int>(b.Width() + 1), static_cast<int>(b.Height() + 1), 'x');
}

static char* haikuDialogGetMinimizedAttrib(Ihandle* ih)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return nullptr;
  return iupStrReturnBoolean(win->IsMinimized() ? 1 : 0);
}

static char* haikuDialogGetActiveWindowAttrib(Ihandle* ih)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return nullptr;
  return iupStrReturnBoolean(win->IsActive() ? 1 : 0);
}


static int haikuDialogSetWorkspacesAttrib(Ihandle* ih, const char* value)
{
  auto* win = reinterpret_cast<IupHaikuWindow*>(ih->handle);
  if (!win) return 1;

  uint32 mask;
  if (!value || iupStrEqualNoCase(value, "ALL")) mask = B_ALL_WORKSPACES;
  else if (iupStrEqualNoCase(value, "CURRENT")) mask = B_CURRENT_WORKSPACE;
  else
  {
    int n = 0;
    if (!iupStrToInt(value, &n) || n < 1 || n > 32) return 0;
    mask = 1U << (n - 1);
  }

  LooperLockGuard guard(win);
  win->SetWorkspaces(mask);
  return 1;
}

/* B_BACKGROUND_APP is locked at launch; fake roster events to Deskbar to toggle the team icon. */
static void haikuDialogSetDeskbarTeamVisible(bool visible)
{
  BMessenger deskbar("application/x-vnd.Be-TSKB");
  if (!deskbar.IsValid() || !be_app) return;

  app_info info;
  if (be_app->GetAppInfo(&info) != B_OK) return;

  if (!visible)
  {
    BMessage msg(B_SOME_APP_QUIT);
    msg.AddInt32("be:team", info.team);
    deskbar.SendMessage(&msg);
  }
  else
  {
    BMessage msg(B_SOME_APP_LAUNCHED);
    msg.AddInt32("be:team", info.team);
    msg.AddInt32("be:flags", static_cast<int32>(info.flags));
    msg.AddString("be:signature", info.signature);
    msg.AddRef("be:ref", &info.ref);
    deskbar.SendMessage(&msg);
  }
}

static int haikuDialogSetIconAttrib(Ihandle* ih, const char* value)
{
  if (!value || !be_app) return 0;
  auto* src = static_cast<BBitmap*>(iupImageGetImage(value, ih, 0, nullptr));
  if (!src || src->InitCheck() != B_OK) return 0;

  app_info info;
  if (be_app->GetAppInfo(&info) != B_OK) return 0;
  BFile file(&info.ref, B_READ_WRITE);
  if (file.InitCheck() != B_OK) return 0;
  BAppFileInfo finfo(&file);

  const struct { icon_size which; float side; } sizes[] = {
    { B_LARGE_ICON, 31.0f },
    { B_MINI_ICON,  15.0f },
  };
  for (const auto& size : sizes)
  {
    BBitmap scaled(BRect(0, 0, size.side, size.side),
                   B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);
    if (scaled.InitCheck() != B_OK) continue;
    BView v(scaled.Bounds(), "iup_icon", B_FOLLOW_NONE, B_WILL_DRAW);
    if (scaled.Lock())
    {
      scaled.AddChild(&v);
      v.SetDrawingMode(B_OP_COPY);
      v.DrawBitmap(src, src->Bounds(), scaled.Bounds());
      v.Sync();
      scaled.RemoveChild(&v);
      scaled.Unlock();
    }
    finfo.SetIcon(&scaled, size.which);
  }

  /* Deskbar caches the team icon at launch; QUIT+LAUNCHED forces a re-query. */
  if (!iupAttribGetBoolean(ih, "HIDETASKBAR"))
  {
    haikuDialogSetDeskbarTeamVisible(false);
    haikuDialogSetDeskbarTeamVisible(true);
  }
  return 0;
}

static int haikuDialogSetHideTaskbarAttrib(Ihandle* ih, const char* value)
{
  int hide = iupStrBoolean(value);
  haikuDialogSetDeskbarTeamVisible(!hide);
  iupdrvDialogSetVisible(ih, !hide);
  return 0;
}

extern "C" IUP_SDK_API void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = haikuDialogMapMethod;
  ic->UnMap = haikuDialogUnMapMethod;
  ic->LayoutUpdate = haikuDialogLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = haikuDialogGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "TITLE", haikuDialogGetTitleAttrib, haikuDialogSetTitleAttrib, nullptr, nullptr, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, haikuDialogSetBgColorAttrib, "DLGBGCOLOR", nullptr, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKGROUND", nullptr, haikuDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKIMAGEZOOM", nullptr, haikuDialogSetBackImageZoomAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "RESIZE", nullptr, haikuDialogSetResizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BORDER", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINBOX", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXBOX", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUBOX", nullptr, nullptr, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETITLEBAR", nullptr, haikuDialogSetHideTitleBarAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAME", nullptr, haikuDialogSetCustomFrameAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MINSIZE", nullptr, haikuDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", nullptr, haikuDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TOPMOST", nullptr, haikuDialogSetTopMostAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", nullptr, haikuDialogSetBringFrontAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", nullptr, haikuDialogSetFullScreenAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CLIENTSIZE", haikuDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", iupBaseGetClientOffsetAttrib, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MINIMIZED", haikuDialogGetMinimizedAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", haikuDialogGetActiveWindowAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "WORKSPACES", nullptr, haikuDialogSetWorkspacesAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HIDETASKBAR", nullptr, haikuDialogSetHideTaskbarAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, iuphaikuGetNativeWindowHandleName(), iuphaikuGetNativeWindowHandleAttrib, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT | IUPAF_NO_STRING);

  iupClassRegisterAttribute(ic, "ICON", nullptr, haikuDialogSetIconAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);

  /* Not supported on Haiku - no public APIs for these. */
  iupClassRegisterAttribute(ic, "OPACITY", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TASKBARBUTTON", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZEINC", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITYIMAGE", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHAPEIMAGE", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMPOSITED", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGHINT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
