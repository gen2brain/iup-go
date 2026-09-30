/** \file
 * \brief Haiku Message Loop. Top-level IupMainLoop blocks on be_app->Run;
 * IUP-side hooks (post queue, idle) ride IupHaikuApp::DispatchMessage via
 * 'IuPM' / 'IuIT' codes. Nested levels use a snooze+wake-sem pump.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>

#include <Application.h>
#include <Looper.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_dlglist.h"
#include "iup_thread.h"
}

#include "iuphaiku_drv.h"


static IFidle haiku_idle_cb = nullptr;
static int haiku_main_loop_level = 0;
static volatile bool haiku_loop_exit_flag[10] = { false };

/* Nested-level wake sem; top level uses be_app's port. */
static sem_id haiku_wake_sem = -1;

static sem_id haikuWakeSem()
{
  if (haiku_wake_sem < B_OK)
    haiku_wake_sem = create_sem(0, "iup_main_wake");
  return haiku_wake_sem;
}

/* Release once per nested pump so every level rechecks its exit flag. */
static void haikuWake()
{
  if (haiku_wake_sem < B_OK) return;
  int n = haiku_main_loop_level - 1;
  if (n < 1) n = 1;
  release_sem_etc(haiku_wake_sem, n, B_DO_NOT_RESCHEDULE);
}

typedef struct _haikuPostNode {
  Ihandle* ih;
  char* s;
  int i;
  double d;
  void* p;
  struct _haikuPostNode* next;
} haikuPostNode;

static haikuPostNode* haiku_post_head = nullptr;
static haikuPostNode* haiku_post_tail = nullptr;
static void* haiku_post_mutex = nullptr;
static bool haiku_post_drain_pending = false;

static void haikuPostKick()
{
  /* Single drain msg outstanding; the looper re-arms when work remains. */
  if (be_app)
  {
    BMessage msg(IUPHAIKU_APP_DRAIN_MSG);
    BMessenger(be_app).SendMessage(&msg);
  }
  haikuWakeSem();
  haikuWake();
}

static void haikuDrainPostQueue()
{
  if (!haiku_post_mutex) return;

  int count = 0;
  while (count < 64)
  {
    iupdrvMutexLock(haiku_post_mutex);
    haikuPostNode* node = haiku_post_head;
    if (node)
    {
      haiku_post_head = node->next;
      if (!haiku_post_head) haiku_post_tail = nullptr;
    }
    iupdrvMutexUnlock(haiku_post_mutex);

    if (!node) break;

    if (iupObjectCheck(node->ih))
    {
      auto cb = reinterpret_cast<IFnsidv>(IupGetCallback(node->ih, "POSTMESSAGE_CB"));
      if (cb && cb(node->ih, node->s, node->i, node->d, node->p) == IUP_CLOSE)
        IupExitLoop();
    }
    if (node->s) free(node->s);
    free(node);
    count++;
  }

  iupdrvMutexLock(haiku_post_mutex);
  bool more = (haiku_post_head != nullptr);
  haiku_post_drain_pending = more;
  iupdrvMutexUnlock(haiku_post_mutex);
  if (more) haikuPostKick();
}

extern "C" IUP_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  if (!iupObjectCheck(ih)) return;

  /* Non-blocking fast path: cb runs on the dialog looper (recursive locks). */
  Ihandle* dlg = IupGetDialog(ih);
  if (dlg && dlg->handle && dlg->handle != reinterpret_cast<InativeHandle*>(-1))
  {
    auto* win = dynamic_cast<BWindow*>(reinterpret_cast<BLooper*>(dlg->handle));
    if (win)
    {
      BMessage msg(IUPHAIKU_POST_MSG);
      msg.AddPointer("ih", ih);
      if (s) msg.AddString("s", s);
      msg.AddInt32("i", i);
      msg.AddDouble("d", d);
      msg.AddPointer("p", p);
      BMessenger msgr(win);
      if (msgr.IsValid() && msgr.SendMessage(&msg, BMessenger(), 0) == B_OK)
        return;
    }
  }

  if (!haiku_post_mutex)
    haiku_post_mutex = iupdrvMutexCreate();

  auto* node = static_cast<haikuPostNode*>(malloc(sizeof(haikuPostNode)));
  if (!node) return;
  node->ih = ih;
  node->s = iupStrDup(s);
  node->i = i;
  node->d = d;
  node->p = p;
  node->next = nullptr;

  iupdrvMutexLock(haiku_post_mutex);
  if (haiku_post_tail) haiku_post_tail->next = node;
  else                 haiku_post_head = node;
  haiku_post_tail = node;
  /* One outstanding drain msg saturates be_app's port under burst producers. */
  bool need_kick = !haiku_post_drain_pending;
  if (need_kick) haiku_post_drain_pending = true;
  iupdrvMutexUnlock(haiku_post_mutex);

  if (need_kick) haikuPostKick();
}

extern "C" IUP_SDK_API void iupdrvSetEntryFunction(Icallback func)
{
  (void)func;
}

extern "C" IUP_SDK_API void* iupdrvNativeScopeBegin(void)
{
  return nullptr;
}

extern "C" IUP_SDK_API void iupdrvNativeScopeEnd(void* scope)
{
  (void)scope;
}

extern "C" IUP_SDK_API void iupdrvSetIdleFunction(Icallback f)
{
  haiku_idle_cb = reinterpret_cast<IFidle>(f);
  if (f) iuphaikuAppStartIdleRunner();
  else   iuphaikuAppStopIdleRunner();
}

IUP_DRV_API void iuphaikuPostAppWake()
{
  if (be_app)
  {
    BMessage msg(IUPHAIKU_APP_DRAIN_MSG);
    BMessenger(be_app).SendMessage(&msg);
  }
  haikuWake();
}

IUP_DRV_API void iuphaikuAppDrainPosts()
{
  haikuDrainPostQueue();
}

IUP_DRV_API void iuphaikuAppIdleTick()
{
  if (!haiku_idle_cb) return;
  int ret = haiku_idle_cb();
  if (ret == IUP_CLOSE)
  {
    haiku_idle_cb = nullptr;
    iuphaikuAppStopIdleRunner();
    IupExitLoop();
  }
  else if (ret == IUP_IGNORE)
  {
    haiku_idle_cb = nullptr;
    iuphaikuAppStopIdleRunner();
  }
}

extern "C" IUP_API void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");
  if (haiku_main_loop_level > 1 || !exit_loop || iupStrBoolean(exit_loop))
  {
    if (haiku_main_loop_level > 1 && haiku_main_loop_level <= 10)
    {
      haiku_loop_exit_flag[haiku_main_loop_level - 1] = true;
      haikuWake();
    }
    else if (haiku_main_loop_level == 1)
    {
      if (be_app)
      {
        BMessenger msgr(be_app);
        msgr.SendMessage(B_QUIT_REQUESTED);
      }
      haikuWake();
    }
  }
}

extern "C" IUP_API int IupMainLoopLevel(void)
{
  return haiku_main_loop_level;
}

extern "C" IUP_API int IupMainLoop(void)
{
  static int has_done_entry = 0;
  if (has_done_entry == 0)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  haiku_main_loop_level++;
  int current_level = haiku_main_loop_level - 1;
  if (current_level >= 10)
  {
    haiku_main_loop_level--;
    return IUP_ERROR;
  }
  haiku_loop_exit_flag[current_level] = false;

  if (current_level == 0)
  {
    /* Top level: lock already held from BApplication ctor. */
    if (be_app && iupDlgListVisibleCount() > 0)
      be_app->Run();
  }
  else
  {
    /* Nested: Run() is single-shot, so spin a private pump. */
    iuphaikuModalBegin();
    BLooper* self_looper = BLooper::LooperForThread(find_thread(nullptr));
    auto* self_window = dynamic_cast<BWindow*>(self_looper);
    sem_id sem = haikuWakeSem();
    while (!haiku_loop_exit_flag[current_level])
    {
      haikuDrainPostQueue();

      if (haiku_idle_cb)
      {
        int ret = haiku_idle_cb();
        if (ret == IUP_CLOSE) { haiku_idle_cb = nullptr; break; }
        if (ret == IUP_IGNORE) haiku_idle_cb = nullptr;
      }

      /* Blocked here, so the looper can't repaint itself; drain its pending updates. */
      if (self_window) self_window->UpdateIfNeeded();

      int relock = 0;
      if (self_looper) while (self_looper->IsLocked()) { self_looper->Unlock(); relock++; }
      bigtime_t timeout = haiku_idle_cb ? 0 : 100000;
      acquire_sem_etc(sem, 1, B_RELATIVE_TIMEOUT, timeout);
      while (relock--) self_looper->Lock();
    }
    iuphaikuModalEnd();
  }

  haiku_loop_exit_flag[current_level] = false;
  haiku_main_loop_level--;

  if (haiku_main_loop_level == 0)
    iupLoopCallExitCb();

  return IUP_NOERROR;
}

extern "C" IUP_API int IupLoopStepWait(void)
{
  sem_id sem = haikuWakeSem();
  haikuDrainPostQueue();
  acquire_sem_etc(sem, 1, B_RELATIVE_TIMEOUT, 100000);
  haikuDrainPostQueue();
  return IUP_DEFAULT;
}

extern "C" IUP_API int IupLoopStep(void)
{
  haikuDrainPostQueue();

  if (haiku_idle_cb)
  {
    int ret = haiku_idle_cb();
    if (ret == IUP_CLOSE) { haiku_idle_cb = nullptr; return IUP_CLOSE; }
    if (ret == IUP_IGNORE) haiku_idle_cb = nullptr;
  }
  return IUP_DEFAULT;
}

extern "C" IUP_API void IupFlush(void)
{
  haikuDrainPostQueue();
  snooze(0);
}

IUP_DRV_API void iuphaikuLoopCleanup()
{
  haiku_idle_cb = nullptr;
  haiku_main_loop_level = 0;
  for (auto& flag : haiku_loop_exit_flag) flag = false;

  if (haiku_post_mutex)
  {
    iupdrvMutexLock(haiku_post_mutex);
    haikuPostNode* n = haiku_post_head;
    haiku_post_head = haiku_post_tail = nullptr;
    iupdrvMutexUnlock(haiku_post_mutex);
    while (n) { haikuPostNode* nx = n->next; if (n->s) free(n->s); free(n); n = nx; }
    iupdrvMutexDestroy(haiku_post_mutex);
    haiku_post_mutex = nullptr;
  }

  if (haiku_wake_sem >= B_OK) { delete_sem(haiku_wake_sem); haiku_wake_sem = -1; }
}

static int haiku_modal_depth = 0;
static bigtime_t haiku_modal_start = 0;
static bigtime_t haiku_modal_end = 0;

IUP_DRV_API void iuphaikuModalBegin()
{
  if (haiku_modal_depth++ == 0)
    haiku_modal_start = system_time();
}

IUP_DRV_API void iuphaikuModalEnd()
{
  if (haiku_modal_depth > 0 && --haiku_modal_depth == 0)
    haiku_modal_end = system_time();
}

IUP_DRV_API bool iuphaikuIsModalStaleInput(BMessage* msg)
{
  if (haiku_modal_depth > 0 || !msg) return false;

  switch (msg->what)
  {
    case B_MOUSE_DOWN:
    case B_MOUSE_UP:
    case B_MOUSE_WHEEL_CHANGED:
    case B_KEY_DOWN:
    case B_KEY_UP:
    case B_UNMAPPED_KEY_DOWN:
    case B_UNMAPPED_KEY_UP:
      break;
    default:
      return false;
  }

  bigtime_t when = 0;
  if (msg->FindInt64("when", &when) != B_OK) return false;
  return when >= haiku_modal_start && when <= haiku_modal_end;
}

IUP_DRV_API int iuphaikuLockLooper(BLooper* looper)
{
  if (!looper) return 0;
  return looper->Lock() ? 1 : 0;
}

IUP_DRV_API void iuphaikuUnlockLooper(BLooper* looper)
{
  if (looper) looper->Unlock();
}
