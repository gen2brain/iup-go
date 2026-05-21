/** \file
 * \brief Motif Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <Xm/Xm.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_thread.h"

#include "iupmot_drv.h"


/* local variables */
static int mot_mainloop = 0;
static int mot_exitmainloop = 0;
static IFidle mot_idle_cb = NULL;
static XtWorkProcId mot_idle_id;


static Boolean motIdlecbWorkProc(XtPointer client_data)
{
  (void)client_data;
  if (mot_idle_cb)
  {
    int ret = mot_idle_cb();
    if (ret == IUP_CLOSE)
    {
      mot_idle_cb = NULL;
      IupExitLoop();
      return True; /* removes the working procedure */
    }
    if (ret == IUP_IGNORE)
    {
      mot_idle_cb = NULL;
      return True; /* removes the working procedure */
    }

    return False; /* keeps the working procedure */
  }

  return True; /* removes the working procedure */
}

IUP_SDK_API void iupdrvSetIdleFunction(Icallback f)
{
  if (mot_idle_cb)
    XtRemoveWorkProc(mot_idle_id);

  mot_idle_cb = (IFidle)f;

  if (mot_idle_cb)
    mot_idle_id = XtAppAddWorkProc(iupmot_appcontext, motIdlecbWorkProc, NULL);
}

static int motLoopProcessEvent(void)
{
  XtAppProcessEvent(iupmot_appcontext, XtIMAll);
  return (mot_exitmainloop)? IUP_CLOSE : IUP_DEFAULT;
}

IUP_API void IupExitLoop(void)
{
  char* exit_loop = IupGetGlobal("EXITLOOP");
  if (mot_mainloop > 1 || !exit_loop || iupStrBoolean(exit_loop))
    mot_exitmainloop = 1;
}

IUP_API int IupMainLoopLevel(void)
{
  return mot_mainloop;
}

IUP_API int IupMainLoop(void)
{
  static int has_done_entry = 0;
  if (0 == has_done_entry)
  {
    has_done_entry = 1;
    iupLoopCallEntryCb();
  }

  mot_mainloop++;
  mot_exitmainloop = 0;

  while (!mot_exitmainloop)
  {
    if (motLoopProcessEvent() == IUP_CLOSE)
      break;
  }

  mot_exitmainloop = 0;
  mot_mainloop--;

  if(0 == mot_mainloop)
  {
    iupLoopCallExitCb();
  }
  return IUP_NOERROR;
}

IUP_API int IupLoopStepWait(void)
{
  while(!XtAppPending(iupmot_appcontext));

  return motLoopProcessEvent();
}

IUP_API int IupLoopStep(void)
{
  if (!XtAppPending(iupmot_appcontext))
    return IUP_DEFAULT;

  return motLoopProcessEvent();
}

IUP_API void IupFlush(void)
{
  int count = 0;

  while (count<100 && XtAppPending(iupmot_appcontext) != 0)
  {
    if (motLoopProcessEvent() == IUP_CLOSE)
      break;

    count++;
  }
}


typedef struct _motPostMessageNode {
  Ihandle* ih;
  char* s;
  int i;
  double d;
  char* p;
  struct _motPostMessageNode* next;
} motPostMessageNode;

static motPostMessageNode* mot_postmsg_head = NULL;
static motPostMessageNode* mot_postmsg_tail = NULL;
static void* mot_postmsg_mutex = NULL;
static int mot_postmsg_pipe[2] = { -1, -1 };

/* a cross-thread work proc can not interrupt Xt's idle select(), so posts wake us via the pipe */
static void motPostMessageInputProc(XtPointer client_data, int* source, XtInputId* id)
{
  char buf[256];
  (void)client_data; (void)source; (void)id;

  while (read(mot_postmsg_pipe[0], buf, sizeof(buf)) > 0)
    ;

  for (;;)
  {
    motPostMessageNode* node;

    iupdrvMutexLock(mot_postmsg_mutex);
    node = mot_postmsg_head;
    if (node)
    {
      mot_postmsg_head = node->next;
      if (!mot_postmsg_head)
        mot_postmsg_tail = NULL;
    }
    iupdrvMutexUnlock(mot_postmsg_mutex);

    if (!node)
      break;

    if (iupObjectCheck(node->ih))
    {
      IFnsidv cb = (IFnsidv)IupGetCallback(node->ih, "POSTMESSAGE_CB");
      if (cb)
        cb(node->ih, node->s, node->i, node->d, node->p);
    }

    if (node->s) free(node->s);
    free(node);
  }
}

void iupmotLoopPostMessageInit(void)
{
  if (mot_postmsg_pipe[0] != -1)
    return;

  mot_postmsg_mutex = iupdrvMutexCreate();

  if (pipe(mot_postmsg_pipe) != 0)
  {
    mot_postmsg_pipe[0] = mot_postmsg_pipe[1] = -1;
    return;
  }

  fcntl(mot_postmsg_pipe[0], F_SETFL, fcntl(mot_postmsg_pipe[0], F_GETFL) | O_NONBLOCK);
  fcntl(mot_postmsg_pipe[1], F_SETFL, fcntl(mot_postmsg_pipe[1], F_GETFL) | O_NONBLOCK);
  fcntl(mot_postmsg_pipe[0], F_SETFD, FD_CLOEXEC);
  fcntl(mot_postmsg_pipe[1], F_SETFD, FD_CLOEXEC);

  XtAppAddInput(iupmot_appcontext, mot_postmsg_pipe[0], (XtPointer)XtInputReadMask, motPostMessageInputProc, NULL);
}

IUP_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  motPostMessageNode* node = (motPostMessageNode*)malloc(sizeof(motPostMessageNode));
  node->ih = ih;
  node->s = iupStrDup(s);
  node->i = i;
  node->d = d;
  node->p = p;
  node->next = NULL;

  iupdrvMutexLock(mot_postmsg_mutex);
  if (mot_postmsg_tail)
    mot_postmsg_tail->next = node;
  else
    mot_postmsg_head = node;
  mot_postmsg_tail = node;
  iupdrvMutexUnlock(mot_postmsg_mutex);

  if (mot_postmsg_pipe[1] != -1)
  {
    char b = 1;
    ssize_t r = write(mot_postmsg_pipe[1], &b, 1);
    (void)r;
  }
}
