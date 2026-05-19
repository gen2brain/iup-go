/** \file
 * \brief Haiku Tray (Deskbar replicant)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <Application.h>
#include <Bitmap.h>
#include <Deskbar.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Point.h>
#include <Rect.h>
#include <Roster.h>
#include <String.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_tray.h"
}

class LooperLockGuard
{
public:
  explicit LooperLockGuard(BLooper* looper) : fLooper(looper), fLocked(false)
  {
    if (fLooper) fLocked = fLooper->Lock();
  }
  ~LooperLockGuard()
  {
    if (fLocked && fLooper) fLooper->Unlock();
  }
  LooperLockGuard(const LooperLockGuard&) = delete;
  LooperLockGuard& operator=(const LooperLockGuard&) = delete;
private:
  BLooper* fLooper;
  bool fLocked;
};

static int haikuTrayStubMap(Ihandle* ih)
{
  BView* v = new BView(BRect(0, 0, 0, 0), "iup_tray_stub", B_FOLLOW_NONE, B_WILL_DRAW);
  ih->handle = (InativeHandle*)v;
  return IUP_NOERROR;
}

/* what code dispatched from the replicant view back to the originating app. */
#define IUPHAIKU_TRAY_CLICK_MSG 'IupZ'
#define IUPHAIKU_TRAY_PULSE_MSG 'IuTp'


/* Per-tray handler installed in our app's looper. The replicant view in
 * Deskbar's process posts messages to this via BMessenger. */
class IupHaikuTrayHandler : public BHandler
{
public:
  explicit IupHaikuTrayHandler(Ihandle* ih) : BHandler("iup_tray_handler"), fIhandle(ih) {}

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_TRAY_CLICK_MSG && fIhandle)
    {
      int32 buttons = 1, clicks = 1, sx = 0, sy = 0;
      msg->FindInt32("buttons", &buttons);
      msg->FindInt32("clicks", &clicks);
      msg->FindInt32("screen_x", &sx);
      msg->FindInt32("screen_y", &sy);

      int btn = (buttons & 0x01) ? 1 : (buttons & 0x02) ? 3 : 2;

      /* Right-click pops up the bound menu (if any) at the cursor before
       * the user callback fires - matches Win32 / Cocoa convention. */
      Ihandle* menu = (Ihandle*)iupAttribGet(fIhandle, "_IUPHAIKU_TRAY_MENU");
      if (btn == 3 && menu)
        IupPopup(menu, sx, sy);

      IFniii cb = (IFniii)IupGetCallback(fIhandle, "TRAYCLICK_CB");
      if (cb)
      {
        int ret = cb(fIhandle, btn, (int)clicks, 0);
        if (ret == IUP_CLOSE) IupExitLoop();
      }
      return;
    }
    BHandler::MessageReceived(msg);
  }

private:
  Ihandle* fIhandle;
};


/* Runs in Deskbar's process after archiving; talks back via fMessenger. */
class IupHaikuTrayView : public BView
{
public:
  IupHaikuTrayView(BBitmap* icon, BMessenger app_handler, const char* tip)
    : BView(BRect(0, 0, 15, 15), "iup_tray", B_FOLLOW_NONE,
            B_WILL_DRAW | B_FRAME_EVENTS),
      fIcon(icon ? new BBitmap(icon) : NULL),
      fMessenger(app_handler),
      fPulse(NULL),
      fTip(tip ? tip : "")
  {
    BView::SetViewColor(B_TRANSPARENT_COLOR);
  }

  explicit IupHaikuTrayView(BMessage* archive)
    : BView(archive), fIcon(NULL), fPulse(NULL)
  {
    archive->FindMessenger("iup:messenger", &fMessenger);
    BMessage iconMsg;
    if (archive->FindMessage("iup:icon", &iconMsg) == B_OK)
      fIcon = new BBitmap(&iconMsg);
    const char* tip = NULL;
    if (archive->FindString("iup:tip", &tip) == B_OK && tip) fTip = tip;
  }

  ~IupHaikuTrayView() override { delete fIcon; delete fPulse; }

  void AttachedToWindow() override
  {
    BView::AttachedToWindow();
    /* SetToolTip on the source side isn't archived, so reapply here in Deskbar's process. */
    if (fTip.Length() > 0) SetToolTip(fTip.String());
    if (!fPulse)
    {
      BMessage tick(IUPHAIKU_TRAY_PULSE_MSG);
      fPulse = new BMessageRunner(BMessenger(this), &tick, 2000000);
    }
  }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_TRAY_PULSE_MSG)
    {
      /* Self-remove when the source team dies; Deskbar has no signal for it. */
      team_info ti;
      if (get_team_info(fMessenger.Team(), &ti) != B_OK)
      {
        BDeskbar deskbar;
        deskbar.RemoveItem(Name());
      }
      return;
    }
    BView::MessageReceived(msg);
  }

  __attribute__((used, visibility("default")))
  static BArchivable* Instantiate(BMessage* archive)
  {
    if (validate_instantiation(archive, "IupHaikuTrayView"))
      return new IupHaikuTrayView(archive);
    return NULL;
  }

  status_t Archive(BMessage* archive, bool deep = true) const override
  {
    status_t st = BView::Archive(archive, deep);
    if (st != B_OK) return st;
    archive->AddString("class", "IupHaikuTrayView");
    archive->AddString("add_on", "application/x-vnd.iup-Application");
    archive->AddMessenger("iup:messenger", fMessenger);
    if (fIcon)
    {
      BMessage iconMsg;
      fIcon->Archive(&iconMsg, deep);
      archive->AddMessage("iup:icon", &iconMsg);
    }
    if (fTip.Length() > 0) archive->AddString("iup:tip", fTip.String());
    return B_OK;
  }

  void Draw(BRect /*update*/) override
  {
    BRect b = Bounds();
    if (fIcon)
    {
      SetDrawingMode(B_OP_ALPHA);
      DrawBitmap(fIcon, b);
    }
  }

  void MouseDown(BPoint where) override
  {
    BMessage* current = NULL;
    if (Looper()) current = Looper()->CurrentMessage();
    int32 buttons = 1, clicks = 1;
    if (current)
    {
      current->FindInt32("buttons", &buttons);
      current->FindInt32("clicks", &clicks);
    }
    /* Screen coords cross the process boundary; window coords don't. */
    BPoint screen = ConvertToScreen(where);
    BMessage out(IUPHAIKU_TRAY_CLICK_MSG);
    out.AddInt32("buttons", buttons);
    out.AddInt32("clicks", clicks);
    out.AddInt32("screen_x", (int32)screen.x);
    out.AddInt32("screen_y", (int32)screen.y);
    fMessenger.SendMessage(&out);
  }

private:
  BBitmap* fIcon;
  BMessenger fMessenger;
  BMessageRunner* fPulse;
  BString fTip;
};

static const char* haikuTrayUniqueName(Ihandle* ih)
{
  char* name = iupAttribGet(ih, "_IUPHAIKU_TRAY_NAME");
  if (name) return name;
  app_info info;
  const char* sig = (be_app && be_app->GetAppInfo(&info) == B_OK) ? info.signature : "iup_tray";
  iupAttribSetStr(ih, "_IUPHAIKU_TRAY_NAME", sig);
  return iupAttribGet(ih, "_IUPHAIKU_TRAY_NAME");
}

static void haikuTrayEnsureHandler(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPHAIKU_TRAY_HANDLER")) return;
  if (!be_app) return;
  IupHaikuTrayHandler* h = new IupHaikuTrayHandler(ih);
  {
    LooperLockGuard guard(be_app);
    be_app->AddHandler(h);
  }
  iupAttribSet(ih, "_IUPHAIKU_TRAY_HANDLER", (char*)h);
}

static void haikuTrayRemoveFromDeskbar(Ihandle* ih)
{
  if (!iupAttribGet(ih, "_IUPHAIKU_TRAY_VISIBLE")) return;
  BDeskbar deskbar;
  deskbar.RemoveItem(haikuTrayUniqueName(ih));
  iupAttribSet(ih, "_IUPHAIKU_TRAY_VISIBLE", NULL);
}

static int haikuTrayInstall(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUPHAIKU_TRAY_VISIBLE")) return 1;

  haikuTrayEnsureHandler(ih);
  IupHaikuTrayHandler* handler = (IupHaikuTrayHandler*)iupAttribGet(ih, "_IUPHAIKU_TRAY_HANDLER");
  if (!handler) return 0;

  const char* image = iupAttribGet(ih, "IMAGE");
  BBitmap* icon = image ? (BBitmap*)iupImageGetIcon(image) : NULL;

  const char* tip = iupAttribGet(ih, "TIP");
  BMessenger msgr(handler);
  IupHaikuTrayView* view = new IupHaikuTrayView(icon, msgr, tip);
  view->SetName(haikuTrayUniqueName(ih));

  BDeskbar deskbar;
  if (!deskbar.IsRunning()) { delete view; return 0; }

  /* Sweep any leftover from a crashed previous run before we add ourselves. */
  deskbar.RemoveItem(haikuTrayUniqueName(ih));

  int32 id = 0;
  status_t st = deskbar.AddItem(view, &id);
  delete view;

  if (st == B_OK)
  {
    iupAttribSet(ih, "_IUPHAIKU_TRAY_VISIBLE", "1");
    return 1;
  }
  return 0;
}

/* Driver hooks */

extern "C" IUP_SDK_API int iupdrvTraySetVisible(Ihandle* ih, int visible)
{
  if (visible) return haikuTrayInstall(ih);
  haikuTrayRemoveFromDeskbar(ih);
  return 1;
}

extern "C" IUP_SDK_API int iupdrvTraySetImage(Ihandle* ih, const char* /*value*/)
{
  /* Deskbar replicants are static once added; re-install to refresh icon. */
  if (iupAttribGet(ih, "_IUPHAIKU_TRAY_VISIBLE"))
  {
    haikuTrayRemoveFromDeskbar(ih);
    haikuTrayInstall(ih);
  }
  return 1;
}

extern "C" IUP_SDK_API int iupdrvTraySetTip(Ihandle* ih, const char* value)
{
  if (iupAttribGet(ih, "_IUPHAIKU_TRAY_VISIBLE"))
  {
    /* IUP stores the value AFTER this setter returns; pre-store so reinstall sees it. */
    iupAttribSetStr(ih, "TIP", value);
    haikuTrayRemoveFromDeskbar(ih);
    haikuTrayInstall(ih);
  }
  return 1;
}

extern "C" IUP_SDK_API int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu)
{
  /* Menu lives in our process; right-click message comes from the replicant
   * and the handler IupPopups it at the cursor. */
  iupAttribSet(ih, "_IUPHAIKU_TRAY_MENU", (char*)menu);
  return 1;
}

extern "C" IUP_SDK_API void iupdrvTrayDestroy(Ihandle* ih)
{
  haikuTrayRemoveFromDeskbar(ih);

  IupHaikuTrayHandler* h = (IupHaikuTrayHandler*)iupAttribGet(ih, "_IUPHAIKU_TRAY_HANDLER");
  if (h && be_app)
  {
    {
      LooperLockGuard guard(be_app);
      be_app->RemoveHandler(h);
    }
    delete h;
    iupAttribSet(ih, "_IUPHAIKU_TRAY_HANDLER", NULL);
  }
  iupAttribSet(ih, "_IUPHAIKU_TRAY_MENU", NULL);
}

extern "C" IUP_SDK_API int iupdrvTrayIsAvailable(void)
{
  BDeskbar d;
  return d.IsRunning() ? 1 : 0;
}

extern "C" IUP_SDK_API void iupdrvTrayInitClass(Iclass* ic)
{
  ic->Map = haikuTrayStubMap;
}
