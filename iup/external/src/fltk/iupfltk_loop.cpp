/** \file
 * \brief FLTK Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>

#include <FL/Fl.H>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"
#include "iup_str.h"
#include "iup_object.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Idle Callback Management
 ****************************************************************************/

static IFidle fltk_idle_cb = nullptr;

static void fltkIdleFunc(void* data)
{
  (void)data;

  if (fltk_idle_cb)
  {
    int ret = fltk_idle_cb();

    if (ret == IUP_CLOSE)
    {
      fltk_idle_cb = nullptr;
      IupExitLoop();
      Fl::remove_idle(fltkIdleFunc, nullptr);
      return;
    }

    if (ret == IUP_IGNORE)
    {
      fltk_idle_cb = nullptr;
      Fl::remove_idle(fltkIdleFunc, nullptr);
      return;
    }
  }
  else
  {
    Fl::remove_idle(fltkIdleFunc, nullptr);
  }
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
  Fl::remove_idle(fltkIdleFunc, nullptr);

  fltk_idle_cb = reinterpret_cast<IFidle>(f);

  if (fltk_idle_cb)
    Fl::add_idle(fltkIdleFunc, nullptr);
}

/****************************************************************************
 * Main Loop Management
 ****************************************************************************/

static int fltk_main_loop_level = 0;
static bool fltk_loop_exit_flag[10] = {false};


static bool fltk_loop_exit_requested = false;

static int fltkLoopStepResult()
{
  if (fltk_main_loop_level > 0 && fltk_main_loop_level <= 10)
  {
    if (!fltk_loop_exit_flag[fltk_main_loop_level - 1])
      return IUP_DEFAULT;
    Fl::awake();
    return IUP_CLOSE;
  }

  if (!fltk_loop_exit_requested)
    return IUP_DEFAULT;
  fltk_loop_exit_requested = false;
  return IUP_CLOSE;
}

extern "C" IUP_API void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");

  if (fltk_main_loop_level > 1 || !exit_loop || iupStrBoolean(exit_loop))
  {
    if (fltk_main_loop_level > 0 && fltk_main_loop_level <= 10)
    {
      fltk_loop_exit_flag[fltk_main_loop_level - 1] = true;
      Fl::awake();
    }
    else if (fltk_main_loop_level == 0)
      fltk_loop_exit_requested = true;
  }
}

extern "C" IUP_API int IupMainLoopLevel(void)
{
  return fltk_main_loop_level;
}

extern "C" IUP_API int IupMainLoop(void)
{
  static int has_done_entry = 0;

  if (has_done_entry == 0)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  fltk_main_loop_level++;
  int current_level = fltk_main_loop_level - 1;

  if (current_level >= 10)
  {
    fltk_main_loop_level--;
    return IUP_ERROR;
  }

  fltk_loop_exit_flag[current_level] = false;
  fltk_loop_exit_requested = false;

  while (!fltk_loop_exit_flag[current_level])
    Fl::wait(1e20);

  fltk_loop_exit_flag[current_level] = false;
  fltk_main_loop_level--;

  if (fltk_main_loop_level == 0)
    iupLoopCallExitCb();

  return IUP_NOERROR;
}

extern "C" IUP_API int IupLoopStepWait(void)
{
  Fl::wait(1e20);
  return fltkLoopStepResult();
}

extern "C" IUP_API int IupLoopStep(void)
{
  Fl::check();
  return fltkLoopStepResult();
}

extern "C" IUP_API void IupFlush(void)
{
  IFidle old_fltk_idle_cb = nullptr;
  if (fltk_idle_cb)
  {
    old_fltk_idle_cb = fltk_idle_cb;
    iupdrvSetIdleFunction(nullptr);
  }

  while (Fl::ready())
    Fl::check();

  if (old_fltk_idle_cb)
    iupdrvSetIdleFunction(reinterpret_cast<Icallback>(old_fltk_idle_cb));
}

/****************************************************************************
 * PostMessage Support
 ****************************************************************************/

#include "iup_thread.h"

typedef struct _fltkPostMessageNode {
  Ihandle* ih;
  char* s;
  int i;
  double d;
  void* p;
  struct _fltkPostMessageNode* next;
} fltkPostMessageNode;

static fltkPostMessageNode* fltk_post_queue_head = nullptr;
static fltkPostMessageNode* fltk_post_queue_tail = nullptr;
static void* fltk_post_queue_mutex = nullptr;

static void fltkPostMessageDrain(void*)
{
  if (!fltk_post_queue_mutex)
    return;

  int count = 0;

  while (count < 100)
  {
    iupdrvMutexLock(fltk_post_queue_mutex);
    fltkPostMessageNode* node = fltk_post_queue_head;
    if (node)
    {
      fltk_post_queue_head = node->next;
      if (!fltk_post_queue_head)
        fltk_post_queue_tail = nullptr;
    }
    iupdrvMutexUnlock(fltk_post_queue_mutex);

    if (!node)
      break;

    if (iupObjectCheck(node->ih))
    {
      auto cb = reinterpret_cast<IFnsidv>(IupGetCallback(node->ih, "POSTMESSAGE_CB"));
      if (cb)
      {
        if (cb(node->ih, node->s, node->i, node->d, node->p) == IUP_CLOSE)
          IupExitLoop();
      }
    }

    if (node->s)
      free(node->s);
    free(node);
    count++;
  }

  iupdrvMutexLock(fltk_post_queue_mutex);
  int has_more = (fltk_post_queue_head != nullptr);
  iupdrvMutexUnlock(fltk_post_queue_mutex);

  if (has_more)
    Fl::awake(fltkPostMessageDrain, nullptr);
}

extern "C" IUP_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  if (!fltk_post_queue_mutex)
    fltk_post_queue_mutex = iupdrvMutexCreate();

  auto* node = static_cast<fltkPostMessageNode*>(malloc(sizeof(fltkPostMessageNode)));
  node->ih = ih;
  node->s = iupStrDup(s);
  node->i = i;
  node->d = d;
  node->p = p;
  node->next = nullptr;

  iupdrvMutexLock(fltk_post_queue_mutex);
  if (fltk_post_queue_tail)
    fltk_post_queue_tail->next = node;
  else
    fltk_post_queue_head = node;
  fltk_post_queue_tail = node;
  iupdrvMutexUnlock(fltk_post_queue_mutex);

  Fl::awake(fltkPostMessageDrain, nullptr);
}

/****************************************************************************
 * Loop Cleanup
 ****************************************************************************/

IUP_DRV_API void iupfltkLoopCleanup()
{
  Fl::remove_idle(fltkIdleFunc, nullptr);
  fltk_idle_cb = nullptr;
  fltk_main_loop_level = 0;

  if (fltk_post_queue_mutex)
  {
    fltkPostMessageNode* node = fltk_post_queue_head;
    while (node)
    {
      fltkPostMessageNode* next = node->next;
      if (node->s)
        free(node->s);
      free(node);
      node = next;
    }
    fltk_post_queue_head = nullptr;
    fltk_post_queue_tail = nullptr;

    iupdrvMutexDestroy(fltk_post_queue_mutex);
    fltk_post_queue_mutex = nullptr;
  }
}
