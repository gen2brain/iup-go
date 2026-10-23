/** \file
 * \brief Haiku Timer
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <climits>
#include <map>
#include <vector>

#include <Autolock.h>
#include <Locker.h>

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


/* A timer's "handle" is a BHandler that owns a BMessageRunner. */

#define IUPHAIKU_TIMER_TICK 'IupT'

static BLocker& haikuTimerRegistryLock()
{
  static BLocker lock("iup_timer_registry");
  return lock;
}

static std::map<int, Ihandle*>& haikuTimerRegistry()
{
  static std::map<int, Ihandle*> registry;
  return registry;
}

static void haikuTimerRegister(int serial, Ihandle* ih)
{
  BAutolock guard(haikuTimerRegistryLock());
  haikuTimerRegistry()[serial] = ih;
}

static void haikuTimerUnregister(int serial)
{
  BAutolock guard(haikuTimerRegistryLock());
  haikuTimerRegistry().erase(serial);
}

Ihandle* iuphaikuTimerFromSerial(int serial)
{
  BAutolock guard(haikuTimerRegistryLock());
  auto found = haikuTimerRegistry().find(serial);
  return found == haikuTimerRegistry().end() ? nullptr : found->second;
}

class IupHaikuTimer : public BHandler
{
public:
  explicit IupHaikuTimer(Ihandle* ih)
    : BHandler("iup_timer"), fIhandle(ih), fRunner(nullptr), fInterval(0), fDeadline(0), fRunning(false) {}
  ~IupHaikuTimer() override { delete fRunner; }

  void MessageReceived(BMessage* msg) override
  {
    if (msg && msg->what == IUPHAIKU_TIMER_TICK && fIhandle)
    {
      {
        BAutolock guard(haikuTimerRegistryLock());
        if (!fRunning) return;
        fDeadline = system_time() + fInterval;
      }
      Fire(true);
      return;
    }
    BHandler::MessageReceived(msg);
  }

  void Fire(bool allow_hop)
  {
    /* the play timer stays on be_app because its sleep would freeze the dialog */
    BWindow* target = nullptr;
    if (allow_hop && !iupAttribGet(fIhandle, "_IUP_PLAYFILE"))
    {
      for (Ihandle* dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
      {
        if (!dlg->handle || dlg->handle == reinterpret_cast<InativeHandle*>(-1)) continue;
        if (!iupdrvDialogIsVisible(dlg)) continue;
        target = reinterpret_cast<BWindow*>(dlg->handle);
        break;
      }
    }
    if (target)
    {
      BMessage hop(IUPHAIKU_TIMER_HOP_MSG);
      hop.AddInt32("serial", static_cast<int32>(fIhandle->serial));
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
  }

  bool ClaimDue(bigtime_t now)
  {
    BAutolock guard(haikuTimerRegistryLock());
    if (!fRunning || now < fDeadline) return false;
    fDeadline = now + fInterval;
    return true;
  }

  bigtime_t Deadline()
  {
    BAutolock guard(haikuTimerRegistryLock());
    return fRunning ? fDeadline : -1;
  }

  void Start(bigtime_t interval_us)
  {
    Stop();
    {
      BAutolock guard(haikuTimerRegistryLock());
      fInterval = interval_us;
      fDeadline = system_time() + interval_us;
      fRunning = true;
    }
    BLooper* app_looper = be_app;
    if (!app_looper) return;
    if (!Looper())
    {
      if (!iuphaikuCanLockAppLooper()) return;
      LooperLockGuard guard(app_looper);
      app_looper->AddHandler(this);
    }
    if (!Looper()) return;
    BMessage tick(IUPHAIKU_TIMER_TICK);
    fRunner = new BMessageRunner(BMessenger(this), &tick, interval_us);
  }

  void Stop()
  {
    {
      BAutolock guard(haikuTimerRegistryLock());
      fRunning = false;
    }
    delete fRunner;
    fRunner = nullptr;
  }

private:
  Ihandle* fIhandle;
  BMessageRunner* fRunner;
  bigtime_t fInterval;
  bigtime_t fDeadline;
  bool fRunning;
};

static IupHaikuTimer* haikuTimerOf(Ihandle* ih)
{
  return reinterpret_cast<IupHaikuTimer*>(iupAttribGet(ih, "_IUPHAIKU_TIMER"));
}

static std::vector<IupHaikuTimer*> haikuTimerSnapshot()
{
  std::vector<IupHaikuTimer*> timers;

  BAutolock guard(haikuTimerRegistryLock());
  for (auto& entry : haikuTimerRegistry())
  {
    IupHaikuTimer* t = entry.second ? haikuTimerOf(entry.second) : nullptr;
    if (t) timers.push_back(t);
  }
  return timers;
}

IUP_DRV_API void iuphaikuTimerRunDue(void)
{
  bigtime_t now = system_time();

  for (IupHaikuTimer* t : haikuTimerSnapshot())
  {
    if (t->ClaimDue(now))
      t->Fire(false);
  }
}

IUP_DRV_API bigtime_t iuphaikuTimerNextWait(void)
{
  bigtime_t now = system_time();
  bigtime_t best = -1;

  for (IupHaikuTimer* t : haikuTimerSnapshot())
  {
    bigtime_t deadline = t->Deadline();
    if (deadline < 0) continue;
    bigtime_t wait = deadline - now;
    if (wait < 0) wait = 0;
    if (best < 0 || wait < best) best = wait;
  }
  return best;
}

static int haikuTimerNextSerial()
{
  static int next_serial = 0;
  if (next_serial == INT_MAX) next_serial = 0;
  return ++next_serial;
}

extern "C" IUP_SDK_API void iupdrvTimerRun(Ihandle* ih)
{
  auto* t = reinterpret_cast<IupHaikuTimer*>(iupAttribGet(ih, "_IUPHAIKU_TIMER"));
  if (!t)
  {
    t = new IupHaikuTimer(ih);
    iupAttribSet(ih, "_IUPHAIKU_TIMER", reinterpret_cast<char*>(t));
  }
  int time_ms = iupAttribGetInt(ih, "TIME");
  if (time_ms <= 0) time_ms = 100;
  haikuTimerUnregister(ih->serial);
  t->Start(static_cast<bigtime_t>(time_ms) * 1000);
  ih->serial = haikuTimerNextSerial();
  haikuTimerRegister(ih->serial, ih);
}

extern "C" IUP_SDK_API void iupdrvTimerStop(Ihandle* ih)
{
  auto* t = haikuTimerOf(ih);
  if (t) t->Stop();
  haikuTimerUnregister(ih->serial);
  ih->serial = -1;
}

static void haikuTimerDestroy(Ihandle* ih)
{
  auto* t = reinterpret_cast<IupHaikuTimer*>(iupAttribGet(ih, "_IUPHAIKU_TIMER"));
  haikuTimerUnregister(ih->serial);
  if (t)
  {
    BLooper* l = t->Looper();
    if (l) { LooperLockGuard guard(l); l->RemoveHandler(t); }
    delete t;
    iupAttribSet(ih, "_IUPHAIKU_TIMER", nullptr);
  }
}

extern "C" IUP_SDK_API void iupdrvTimerInitClass(Iclass* ic)
{
  ic->Destroy = haikuTimerDestroy;
}
