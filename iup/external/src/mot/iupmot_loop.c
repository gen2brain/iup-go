/** \file
 * \brief Motif Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>    
#include <stdio.h>    

#include <Xm/Xm.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"

#include "iup_str.h"

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


typedef struct {
  Ihandle* ih;
  char* s;
  int i;
  double d;
  char* p;
} motPostMessageUserData;

static Boolean motPostMessagebWorkProc(XtPointer client_data)
{
  motPostMessageUserData* user_data = (motPostMessageUserData*)client_data;
  Ihandle* ih = user_data->ih;
  IFnsidv cb = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
  if (cb)
    cb(ih, user_data->s, user_data->i, user_data->d, user_data->p);
  if (user_data->s) free(user_data->s);
  free(user_data);
  return True; /* removes the working procedure */
}

IUP_API void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  motPostMessageUserData* user_data = (motPostMessageUserData*)malloc(sizeof(motPostMessageUserData));
  user_data->ih = ih;
  user_data->s = iupStrDup(s);
  user_data->i = i;
  user_data->d = d;
  user_data->p = p;
  //XtAppAddWorkProc(iupmot_appcontext, motPostMessagebWorkProc, NULL);
  XtAppAddWorkProc(iupmot_appcontext, motPostMessagebWorkProc, user_data);
}
