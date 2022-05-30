/** \file
 * \brief Motif Driver implementation of iupdrvSetGlobal
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>

#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_key.h"

#include "iupmot_drv.h"


static int mot_dispatchers_set = 0;
static XtEventDispatchProc motKeyPressEventDispatchProc_OLD = NULL;
static XtEventDispatchProc motKeyReleaseEventDispatchProc_OLD = NULL;
static XtEventDispatchProc motMotionNotifyEventDispatchProc_OLD = NULL;
static XtEventDispatchProc motButtonPressEventDispatchProc_OLD = NULL;
static XtEventDispatchProc motButtonReleaseEventDispatchProc_OLD = NULL;


static Boolean motKeyEventDispatchProc(XEvent* evt)
{
  IFii cb = (IFii)IupGetFunction("GLOBALKEYPRESS_CB");
  if (cb)
  {
    XKeyEvent* evt_key = (XKeyEvent*)evt;
    int pressed = (evt_key->type==KeyPress)? 1: 0;
    int code = iupmotKeyDecode(evt_key);
    if (code != 0)
      cb(code, pressed);
  }

  if (evt->type==KeyPress)
    return motKeyPressEventDispatchProc_OLD(evt);
  else
    return motKeyReleaseEventDispatchProc_OLD(evt);
}

static Boolean motMotionNotifyEventDispatchProc(XEvent* evt)
{
  IFiis cb = (IFiis)IupGetFunction("GLOBALMOTION_CB");
  if (cb)
  {
    XMotionEvent *evt_motion = (XMotionEvent*)evt;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    int x = (int)evt_motion->x;
    int y = (int)evt_motion->y;
    Window child;

    XTranslateCoordinates(iupmot_display, evt_motion->window,
                                          RootWindow(iupmot_display, iupmot_screen),
                                          x, y, &x, &y, &child);

    iupmotButtonKeySetStatus(evt_motion->state, 0, status, 0);
    cb(x, y, status);
  }
  return motMotionNotifyEventDispatchProc_OLD(evt);
}

static Boolean motButtonEventDispatchProc(XEvent* evt)
{
  IFiiiis cb = (IFiiiis)IupGetFunction("GLOBALBUTTON_CB");
  if (cb)
  {
    XButtonEvent *evt_button = (XButtonEvent*)evt;
    static Time last = 0;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    int x = (int)evt_button->x;
    int y = (int)evt_button->y;
    Window child;
    int doubleclick = 0;
    int b = IUP_BUTTON1+(evt_button->button-1);

    if (evt_button->button!=Button1 &&
        evt_button->button!=Button2 &&
        evt_button->button!=Button3 &&
        evt_button->button!=Button4 &&
        evt_button->button!=Button5) 
    {
      if (evt->type==ButtonPress)
        return motButtonPressEventDispatchProc_OLD(evt);
      else
        return motButtonReleaseEventDispatchProc_OLD(evt);
    }

    XTranslateCoordinates(iupmot_display, evt_button->window,
                                          RootWindow(iupmot_display, iupmot_screen),
                                          x, y, &x, &y, &child);

    /* Double/Single Click */
    if (evt_button->type==ButtonPress)
    {
      unsigned long elapsed = evt_button->time - last;
      last = evt_button->time;
      if ((int)elapsed <= XtGetMultiClickTime(iupmot_display))
        doubleclick = 1;
    }

    iupmotButtonKeySetStatus(evt_button->state, evt_button->button, status, doubleclick);

    cb(b, (evt_button->type==ButtonPress), x, y, status);
  }

  if (evt->type==ButtonPress)
    return motButtonPressEventDispatchProc_OLD(evt);
  else
    return motButtonReleaseEventDispatchProc_OLD(evt);
}

IUP_SDK_API int iupdrvSetGlobal(const char *name, const char *value)
{
  if (iupStrEqual(name, "INPUTCALLBACKS"))
  {
    if (iupStrBoolean(value))
    {
      if (!mot_dispatchers_set)
      {
        motKeyPressEventDispatchProc_OLD = XtSetEventDispatcher(iupmot_display, KeyPress, motKeyEventDispatchProc);
        motKeyReleaseEventDispatchProc_OLD = XtSetEventDispatcher(iupmot_display, KeyRelease, motKeyEventDispatchProc);
        motMotionNotifyEventDispatchProc_OLD = XtSetEventDispatcher(iupmot_display, MotionNotify, motMotionNotifyEventDispatchProc);
        motButtonPressEventDispatchProc_OLD = XtSetEventDispatcher(iupmot_display, ButtonPress, motButtonEventDispatchProc);
        motButtonReleaseEventDispatchProc_OLD = XtSetEventDispatcher(iupmot_display, ButtonRelease, motButtonEventDispatchProc);
        mot_dispatchers_set = 1;
      }
    }
    else if (mot_dispatchers_set)
    {
      XtSetEventDispatcher(iupmot_display, KeyPress, motKeyPressEventDispatchProc_OLD);
      XtSetEventDispatcher(iupmot_display, KeyRelease, motKeyReleaseEventDispatchProc_OLD);
      XtSetEventDispatcher(iupmot_display, MotionNotify, motMotionNotifyEventDispatchProc_OLD);
      XtSetEventDispatcher(iupmot_display, ButtonPress, motButtonPressEventDispatchProc_OLD);
      XtSetEventDispatcher(iupmot_display, ButtonRelease, motButtonReleaseEventDispatchProc_OLD);
      mot_dispatchers_set = 0;
    }
    return 1;
  }
  if (iupStrEqual(name, "AUTOREPEAT"))
  {
    XKeyboardControl values;
    if (iupStrBoolean(value))
      values.auto_repeat_mode = 1;
    else
      values.auto_repeat_mode = 0;
    XChangeKeyboardControl(iupmot_display, KBAutoRepeatMode, &values);
    return 0;
  }
  return 1;
}

IUP_SDK_API char* iupdrvGetGlobal(const char *name)
{
  (void)name;
  return NULL;
}
