/** \file
 * \brief Button Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_image.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"


static int iAnimatedLabelTimer_CB(Ihandle* timer)
{
  Ihandle* ih = (Ihandle*)iupAttribGet(timer, "_IUP_ANIMATEDLABEL");
  Ihandle* animation = (Ihandle*)iupAttribGet(ih, "_IUP_ANIMATEDLABEL_ANIMATION");

  if (ih->handle && !iupdrvIsVisible(ih))
  {
    if (iupAttribGetBoolean(ih, "STOPWHENHIDDEN"))
      IupSetAttribute(timer, "RUN", "NO");
    return IUP_DEFAULT;
  }

  if (animation)
  {
    Ihandle* child;
    int num_frames = IupGetChildCount(animation);

    int current_frame = iupAttribGetInt(ih, "_IUP_ANIMATEDLABEL_FRAME");
    if (current_frame == num_frames - 1)
      current_frame = 0;
    else
      current_frame++;
    iupAttribSetInt(ih, "_IUP_ANIMATEDLABEL_FRAME", current_frame);

    child = IupGetChild(animation, current_frame);
    IupSetAttributeHandle(ih, "IMAGE", child);
  }

  return IUP_DEFAULT;
}


/***********************************************************************************************/


static int iAnimatedLabelSetStartAttrib(Ihandle* ih, const char* value)
{
  Ihandle* timer = (Ihandle*)iupAttribGet(ih, "_IUP_ANIMATEDLABEL_TIMER");
  IupSetAttribute(timer, "RUN", "YES");
  (void)value;
  return 0;
}

static int iAnimatedLabelSetStopAttrib(Ihandle* ih, const char* value)
{
  Ihandle* timer = (Ihandle*)iupAttribGet(ih, "_IUP_ANIMATEDLABEL_TIMER");
  IupSetAttribute(timer, "RUN", "NO");
  (void)value;
  return 0;
}

static int iAnimatedLabelSetFrameTimeAttrib(Ihandle* ih, const char* value)
{
  Ihandle* timer = (Ihandle*)iupAttribGet(ih, "_IUP_ANIMATEDLABEL_TIMER");
  IupSetStrAttribute(timer, "TIME", value);
  return 0; 
}

static char* iAnimatedLabelGetFrameTimeAttrib(Ihandle* ih)
{
  Ihandle* timer = (Ihandle*)iupAttribGet(ih, "_IUP_ANIMATEDLABEL_TIMER");
  return IupGetAttribute(timer, "TIME");
}

static char* iAnimatedLabelGetRunningAttrib(Ihandle* ih)
{
  Ihandle* timer = (Ihandle*)iupAttribGet(ih, "_IUP_ANIMATEDLABEL_TIMER");
  return IupGetAttribute(timer, "RUN");
}

static int iAnimatedLabelSetAnimationHandleAttrib(Ihandle* ih, const char* value)
{
  Ihandle* animation = (Ihandle*)value;
  Ihandle* child;

  if (!iupObjectCheck(animation))
    return 0;

  if (IupGetChildCount(animation) == 0)
    return 0;

  iupAttribSet(ih, "_IUP_ANIMATEDLABEL_ANIMATION", (char*)animation);
  iupAttribSet(ih, "_IUP_ANIMATEDLABEL_FRAME", "0");

  /* make sure it has at least one name */
  if (!iupAttribGetHandleName(animation))
    iupAttribSetHandleName(animation);

  child = IupGetChild(animation, 0);
  IupSetAttributeHandle(ih, "IMAGE", child);

  value = IupGetAttribute(animation, "FRAMETIME");
  if (value)
  {
    Ihandle* timer = (Ihandle*)iupAttribGet(ih, "_IUP_ANIMATEDLABEL_TIMER");
    IupSetStrAttribute(timer, "TIME", value);
  }

  return 0;
}

static char* iAnimatedLabelGetAnimationHandleAttrib(Ihandle* ih)
{
  return iupAttribGet(ih, "_IUP_ANIMATEDLABEL_ANIMATION");
}

static int iAnimatedLabelSetAnimationAttrib(Ihandle* ih, const char* value)
{
  Ihandle *animation;

  if (!value)
    return 0;

  animation = IupGetHandle(value);
  if (!animation)
    return 0;

  return iAnimatedLabelSetAnimationHandleAttrib(ih, (char*)animation);
}

static char* iAnimatedLabelGetAnimationAttrib(Ihandle* ih)
{
  Ihandle *animation = (Ihandle*)iAnimatedLabelGetAnimationHandleAttrib(ih);
  return IupGetName(animation);
}

static char* iAnimatedLabelGetFrameCountAttrib(Ihandle* ih)
{
  Ihandle *animation = (Ihandle*)iAnimatedLabelGetAnimationHandleAttrib(ih);
  return iupStrReturnInt(IupGetChildCount(animation));
}


/*****************************************************************************************/


static int iAnimatedLabelCreateMethod(Ihandle* ih, void** params)
{
  Ihandle* timer;

  iupAttribSet(ih, "IMAGE", "ANIMATEDLABEL");  /* to make sure the label has an image, even if not defined */

  timer = IupTimer();
  IupSetCallback(timer, "ACTION_CB", (Icallback)iAnimatedLabelTimer_CB);
  IupSetAttribute(timer, "TIME", "30");
  iupAttribSet(timer, "_IUP_ANIMATEDLABEL", (char*)ih);

  iupAttribSet(ih, "_IUP_ANIMATEDLABEL_TIMER", (char*)timer);

  if (params && params[0])
    iAnimatedLabelSetAnimationHandleAttrib(ih, (char*)(params[0]));

  return IUP_NOERROR;
}

static void iAnimatedLabelDestroyMethod(Ihandle* ih)
{
  Ihandle* timer = (Ihandle*)iupAttribGet(ih, "_IUP_ANIMATEDLABEL_TIMER");
  IupDestroy(timer);
}


/******************************************************************************/


Iclass* iupAnimatedLabelNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("label"));

  ic->name = "animatedlabel";
  ic->cons = "AnimatedLabel";
  ic->format = "h"; /* one Ihandle* */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupAnimatedLabelNewClass;
  ic->Create = iAnimatedLabelCreateMethod;
  ic->Destroy = iAnimatedLabelDestroyMethod;


  /* IupAnimatedLabel only */
  iupClassRegisterAttribute(ic, "START", NULL, iAnimatedLabelSetStartAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STOP", NULL, iAnimatedLabelSetStopAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RUNNING", iAnimatedLabelGetRunningAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMETIME", iAnimatedLabelGetFrameTimeAttrib, iAnimatedLabelSetFrameTimeAttrib, IUPAF_SAMEASSYSTEM, "30", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMECOUNT", iAnimatedLabelGetFrameCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ANIMATION", iAnimatedLabelGetAnimationAttrib, iAnimatedLabelSetAnimationAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ANIMATION_HANDLE", iAnimatedLabelGetAnimationHandleAttrib, iAnimatedLabelSetAnimationHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_IHANDLE | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "STOPWHENHIDDEN", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIRST_CONTROL_HANDLE", iAnimatedLabelGetAnimationHandleAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_IHANDLE | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "NEXT_CONTROL_HANDLE", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT | IUPAF_IHANDLE | IUPAF_NO_STRING);

  return ic;
}

IUP_API Ihandle* IupAnimatedLabel(Ihandle* animation)
{
  void *params[2];
  params[0] = (void*)animation;
  params[1] = NULL;
  return IupCreatev("animatedlabel", params);
}
