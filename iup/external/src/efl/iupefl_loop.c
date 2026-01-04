/** \file
 * \brief EFL main loop integration
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_loop.h"
#include "iup_object.h"

#include "iupefl_drv.h"


static int efl_mainloop = 0;
static int efl_exitmainloop = 0;

static IFidle efl_idle_cb = NULL;
static Ecore_Idle_Enterer* efl_idler = NULL;

#define EFL_MODAL_LOOP_MAX_DEPTH 10
static int efl_modal_loop_level = 0;
static int efl_modal_loop_exit_flag[EFL_MODAL_LOOP_MAX_DEPTH] = {0};

/* Dynamic loading of efl_loop_message_pending_flush (requires patched EFL) */
typedef void (*efl_loop_message_pending_flush_fn)(Eo *obj);
static efl_loop_message_pending_flush_fn efl_pending_flush_func = NULL;
static int efl_pending_flush_checked = 0;

void iupeflMessagePendingFlush(Eo *loop)
{
  if (!efl_pending_flush_checked)
  {
    efl_pending_flush_checked = 1;
    efl_pending_flush_func = (efl_loop_message_pending_flush_fn)dlsym(RTLD_DEFAULT, "efl_loop_message_pending_flush");
  }
  if (efl_pending_flush_func)
    efl_pending_flush_func(loop);
}

/****************************************************************************
 * Idle Function
 ****************************************************************************/

static Eina_Bool eflIdlerCallback(void *data)
{
  (void)data;

  if (efl_idle_cb)
  {
    int ret = efl_idle_cb();
    if (ret == IUP_CLOSE)
    {
      efl_idle_cb = NULL;
      efl_idler = NULL;
      IupExitLoop();
      return ECORE_CALLBACK_CANCEL;
    }
    if (ret == IUP_IGNORE)
    {
      efl_idle_cb = NULL;
      efl_idler = NULL;
      return ECORE_CALLBACK_CANCEL;
    }
    return ECORE_CALLBACK_RENEW;
  }

  efl_idler = NULL;
  return ECORE_CALLBACK_CANCEL;
}

IUP_SDK_API void iupdrvSetIdleFunction(Icallback func)
{
  if (efl_idler)
  {
    ecore_idle_enterer_del(efl_idler);
    efl_idler = NULL;
  }

  efl_idle_cb = (IFidle)func;

  if (efl_idle_cb)
    efl_idler = ecore_idle_enterer_add(eflIdlerCallback, NULL);
}

/****************************************************************************
 * Main Loop
 ****************************************************************************/

IUP_SDK_API void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");
  if (efl_mainloop > 1 || !exit_loop || iupStrBoolean(exit_loop))
  {
    int i;
    efl_exitmainloop = 1;
    for (i = 0; i < efl_modal_loop_level; i++)
      efl_modal_loop_exit_flag[i] = 1;
    if (efl_mainloop == 1)
    {
      iupeflMessagePendingFlush(efl_main_loop_get());
      ecore_main_loop_quit();
    }
  }
}

IUP_SDK_API int IupMainLoopLevel(void)
{
  return efl_mainloop;
}

IUP_SDK_API int IupMainLoop(void)
{
  static int has_done_entry = 0;

  if (has_done_entry == 0)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  efl_mainloop++;
  efl_exitmainloop = 0;

  if (efl_mainloop == 1)
  {
    ecore_main_loop_begin();
  }
  else
  {
    Eo *loop = efl_main_loop_get();
    while (!efl_exitmainloop)
    {
      iupeflMessagePendingFlush(loop);
      ecore_main_loop_iterate_may_block(EINA_TRUE);
    }
  }

  efl_exitmainloop = 0;
  efl_mainloop--;

  if (efl_mainloop == 0)
    iupLoopCallExitCb();

  return IUP_NOERROR;
}

IUP_SDK_API int IupLoopStepWait(void)
{
  ecore_main_loop_iterate_may_block(EINA_TRUE);
  return efl_exitmainloop ? IUP_CLOSE : IUP_DEFAULT;
}

IUP_SDK_API int IupLoopStep(void)
{
  ecore_main_loop_iterate();
  return efl_exitmainloop ? IUP_CLOSE : IUP_DEFAULT;
}

IUP_SDK_API void IupFlush(void)
{
  int count = 0;
  Eo* loop = efl_main_loop_get();

  IFidle old_idle_cb = NULL;
  if (efl_idle_cb)
  {
    old_idle_cb = efl_idle_cb;
    iupdrvSetIdleFunction(NULL);
  }

  iupeflMessagePendingFlush(loop);

  while (count < 100 && ecore_main_loop_iterate_may_block(EINA_FALSE))
    count++;

  if (old_idle_cb)
    iupdrvSetIdleFunction((Icallback)old_idle_cb);
}

/****************************************************************************
 * Modal Loop Support
 ****************************************************************************/

void iupeflModalLoopRun(void)
{
  int level, i;
  Eo *loop = efl_main_loop_get();

  if (efl_modal_loop_level >= EFL_MODAL_LOOP_MAX_DEPTH)
    return;

  level = efl_modal_loop_level;
  efl_modal_loop_level++;
  efl_modal_loop_exit_flag[level] = 0;

  while (!efl_modal_loop_exit_flag[level])
  {
    iupeflMessagePendingFlush(loop);
    ecore_main_loop_iterate_may_block(EINA_TRUE);
  }

  /* Process remaining messages after modal loop exits. */
  iupeflMessagePendingFlush(loop);
  for (i = 0; i < 10; i++)
    ecore_main_loop_iterate_may_block(EINA_FALSE);

  efl_modal_loop_exit_flag[level] = 0;
  efl_modal_loop_level--;
}

void iupeflModalLoopQuit(void)
{
  if (efl_modal_loop_level > 0)
    efl_modal_loop_exit_flag[efl_modal_loop_level - 1] = 1;
}

void iupeflLoopCleanup(void)
{
  if (efl_idler)
  {
    ecore_idle_enterer_del(efl_idler);
    efl_idler = NULL;
  }
  efl_idle_cb = NULL;

  while (efl_mainloop > 0)
  {
    efl_exitmainloop = 1;
    ecore_main_loop_quit();
    efl_mainloop--;
  }
}

/****************************************************************************
 * Post Message
 ****************************************************************************/

typedef struct _IeflPostMessage
{
  Ihandle* ih;
  char* s;
  int i;
  double d;
  void* p;
} IeflPostMessage;

static void eflPostMessageCallback(void* data)
{
  IeflPostMessage* msg = (IeflPostMessage*)data;

  IFnsidv cb = (IFnsidv)IupGetCallback(msg->ih, "POSTMESSAGE_CB");
  if (cb)
    cb(msg->ih, msg->s, msg->i, msg->d, msg->p);

  if (msg->s)
    free(msg->s);
  free(msg);
}

IUP_SDK_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  IeflPostMessage* msg = (IeflPostMessage*)malloc(sizeof(IeflPostMessage));

  msg->ih = ih;
  msg->s = s ? iupStrDup(s) : NULL;
  msg->i = i;
  msg->d = d;
  msg->p = p;

  ecore_main_loop_thread_safe_call_async(eflPostMessageCallback, msg);
}
