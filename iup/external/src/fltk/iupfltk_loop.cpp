/** \file
 * \brief FLTK Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <FL/Fl.H>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"
#include "iup_str.h"
#include "iup_object.h"
#include "iup_dlglist.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Idle Callback Management
 ****************************************************************************/

static IFidle fltk_idle_cb = NULL;

static void fltkIdleFunc(void* data)
{
  (void)data;

  if (fltk_idle_cb)
  {
    int ret = fltk_idle_cb();

    if (ret == IUP_CLOSE)
    {
      fltk_idle_cb = NULL;
      IupExitLoop();
      Fl::remove_idle(fltkIdleFunc, NULL);
      return;
    }

    if (ret == IUP_IGNORE)
    {
      fltk_idle_cb = NULL;
      Fl::remove_idle(fltkIdleFunc, NULL);
      return;
    }
  }
  else
  {
    Fl::remove_idle(fltkIdleFunc, NULL);
  }
}

extern "C" IUP_SDK_API void iupdrvSetIdleFunction(Icallback f)
{
  Fl::remove_idle(fltkIdleFunc, NULL);

  fltk_idle_cb = (IFidle)f;

  if (fltk_idle_cb)
    Fl::add_idle(fltkIdleFunc, NULL);
}

/****************************************************************************
 * Main Loop Management
 ****************************************************************************/

static int fltk_main_loop_level = 0;
static bool fltk_loop_exit_flag[10] = {false};


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

  while (!fltk_loop_exit_flag[current_level])
  {
    if (Fl::wait() == 0)
    {
      if (iupDlgListVisibleCount() <= 0)
        break;
      Fl::wait(0.1);
    }
  }

  fltk_loop_exit_flag[current_level] = false;
  fltk_main_loop_level--;

  if (fltk_main_loop_level == 0)
    iupLoopCallExitCb();

  return IUP_NOERROR;
}

extern "C" IUP_API int IupLoopStepWait(void)
{
  Fl::wait();
  return IUP_DEFAULT;
}

extern "C" IUP_API int IupLoopStep(void)
{
  Fl::check();

  if (fltk_idle_cb)
  {
    int ret = fltk_idle_cb();
    if (ret == IUP_CLOSE)
    {
      fltk_idle_cb = NULL;
      return IUP_CLOSE;
    }
    if (ret == IUP_IGNORE)
      fltk_idle_cb = NULL;
  }

  return IUP_DEFAULT;
}

extern "C" IUP_API void IupFlush(void)
{
  IFidle old_fltk_idle_cb = NULL;
  if (fltk_idle_cb)
  {
    old_fltk_idle_cb = fltk_idle_cb;
    iupdrvSetIdleFunction(NULL);
  }

  while (Fl::ready())
    Fl::check();

  if (old_fltk_idle_cb)
    iupdrvSetIdleFunction((Icallback)old_fltk_idle_cb);
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

static fltkPostMessageNode* fltk_post_queue_head = NULL;
static fltkPostMessageNode* fltk_post_queue_tail = NULL;
static void* fltk_post_queue_mutex = NULL;

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
        fltk_post_queue_tail = NULL;
    }
    iupdrvMutexUnlock(fltk_post_queue_mutex);

    if (!node)
      break;

    if (iupObjectCheck(node->ih))
    {
      IFnsidv cb = (IFnsidv)IupGetCallback(node->ih, "POSTMESSAGE_CB");
      if (cb)
        cb(node->ih, node->s, node->i, node->d, node->p);
    }

    if (node->s)
      free(node->s);
    free(node);
    count++;
  }

  iupdrvMutexLock(fltk_post_queue_mutex);
  int has_more = (fltk_post_queue_head != NULL);
  iupdrvMutexUnlock(fltk_post_queue_mutex);

  if (has_more)
    Fl::awake(fltkPostMessageDrain, NULL);
}

extern "C" IUP_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  if (!fltk_post_queue_mutex)
    fltk_post_queue_mutex = iupdrvMutexCreate();

  fltkPostMessageNode* node = (fltkPostMessageNode*)malloc(sizeof(fltkPostMessageNode));
  node->ih = ih;
  node->s = iupStrDup(s);
  node->i = i;
  node->d = d;
  node->p = p;
  node->next = NULL;

  iupdrvMutexLock(fltk_post_queue_mutex);
  if (fltk_post_queue_tail)
    fltk_post_queue_tail->next = node;
  else
    fltk_post_queue_head = node;
  fltk_post_queue_tail = node;
  iupdrvMutexUnlock(fltk_post_queue_mutex);

  Fl::awake(fltkPostMessageDrain, NULL);
}

/****************************************************************************
 * Loop Cleanup
 ****************************************************************************/

IUP_DRV_API void iupfltkLoopCleanup(void)
{
  Fl::remove_idle(fltkIdleFunc, NULL);
  fltk_idle_cb = NULL;
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
    fltk_post_queue_head = NULL;
    fltk_post_queue_tail = NULL;

    iupdrvMutexDestroy(fltk_post_queue_mutex);
    fltk_post_queue_mutex = NULL;
  }
}
