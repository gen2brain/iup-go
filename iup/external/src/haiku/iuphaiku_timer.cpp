/** \file
 * \brief Haiku Timer
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <Application.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <OS.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_dlglist.h"
#include "iup_dialog.h"
}

#include "iuphaiku_drv.h"


/* A timer's "handle" is a BLooper-derived BHandler that owns a BMessageRunner.
 * MessageReceived dispatches IUP's ACTION_CB. */

#define IUPHAIKU_TIMER_TICK 'IupT'

class IupHaikuTimer : public BHandler
{
public:
  explicit IupHaikuTimer(Ihandle* ih) : BHandler("iup_timer"), fIhandle(ih), fRunner(NULL) {}
  ~IupHaikuTimer() override { delete fRunner; }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_TIMER_TICK && fIhandle)
    {
      /* Hop ACTION_CB to a visible dialog so its setters recurse locally; the
         play timer stays on be_app because its sleep would freeze the dialog. */
      BWindow* target = NULL;
      if (!iupAttribGet(fIhandle, "_IUP_PLAYFILE"))
      {
        for (Ihandle* dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
        {
          if (!dlg->handle || dlg->handle == (InativeHandle*)-1) continue;
          if (!iupdrvDialogIsVisible(dlg)) continue;
          target = (BWindow*)dlg->handle;
          break;
        }
      }
      if (target)
      {
        BMessage hop(IUPHAIKU_TIMER_HOP_MSG);
        hop.AddPointer("ih", fIhandle);
        BMessenger msgr(target);
        if (msgr.IsValid() && msgr.SendMessage(&hop) == B_OK)
          return;
      }
      Icallback cb = IupGetCallback(fIhandle, "ACTION_CB");
      if (cb)
      {
        int ret = cb(fIhandle);
        if (ret == IUP_CLOSE) IupExitLoop();
      }
      return;
    }
    BHandler::MessageReceived(msg);
  }

  void Start(bigtime_t interval_us)
  {
    Stop();
    BLooper* app_looper = be_app;
    if (!app_looper) return;
    if (!Looper())
    {
      LooperLockGuard guard(app_looper);
      app_looper->AddHandler(this);
    }
    BMessage tick(IUPHAIKU_TIMER_TICK);
    fRunner = new BMessageRunner(BMessenger(this), &tick, interval_us);
  }

  void Stop()
  {
    delete fRunner;
    fRunner = NULL;
  }

private:
  Ihandle* fIhandle;
  BMessageRunner* fRunner;
};


extern "C" IUP_SDK_API void iupdrvTimerRun(Ihandle* ih)
{
  IupHaikuTimer* t = (IupHaikuTimer*)iupAttribGet(ih, "_IUPHAIKU_TIMER");
  if (!t)
  {
    t = new IupHaikuTimer(ih);
    iupAttribSet(ih, "_IUPHAIKU_TIMER", (char*)t);
  }
  int time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms <= 0) time_ms = 100;
  t->Start((bigtime_t)time_ms * 1000);
}

extern "C" IUP_SDK_API void iupdrvTimerStop(Ihandle* ih)
{
  IupHaikuTimer* t = (IupHaikuTimer*)iupAttribGet(ih, "_IUPHAIKU_TIMER");
  if (t) t->Stop();
}

static void haikuTimerDestroy(Ihandle* ih)
{
  IupHaikuTimer* t = (IupHaikuTimer*)iupAttribGet(ih, "_IUPHAIKU_TIMER");
  if (t)
  {
    BLooper* l = t->Looper();
    if (l) { LooperLockGuard guard(l); l->RemoveHandler(t); }
    delete t;
    iupAttribSet(ih, "_IUPHAIKU_TIMER", NULL);
  }
}

extern "C" IUP_SDK_API void iupdrvTimerInitClass(Iclass* ic)
{
  ic->Destroy = haikuTimerDestroy;
}
