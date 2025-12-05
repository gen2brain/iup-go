/** \file
 * \brief Toggle Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/ToggleB.h>
#include <Xm/DrawingA.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_toggle.h"
#include "iup_drv.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"

#define SWITCH_TRACK_WIDTH  40
#define SWITCH_TRACK_HEIGHT 20
#define SWITCH_THUMB_SIZE   14
#define SWITCH_THUMB_MARGIN 3

typedef struct _IupMotSwitchData
{
  Ihandle* ih;
  int checked_state;
  Widget drawing_area;
} IupMotSwitchData;


void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y)
{
  iupdrvButtonAddBorders(ih, x, y);
}

void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* str)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    (*x) += 2 + SWITCH_TRACK_WIDTH + 2;
    if ((*y) < 2 + SWITCH_TRACK_HEIGHT + 2)
      (*y) = 2 + SWITCH_TRACK_HEIGHT + 2;
    else
      (*y) += 2 + 2;

    if (str && str[0])
      (*x) += 8;

    return;
  }

  {
    int check_box = 15;
    (void)ih;

    (*x) += 3 + check_box + 3;
    if ((*y) < 3 + check_box + 5) (*y) = 3 + check_box + 5;
    else (*y) += 3+5;

    if (str && str[0])
      (*x) += 4;
  }
}

/*********************************************************************************/
/* SWITCH Control                                                               */
/*********************************************************************************/

static void motSwitchDraw(Ihandle* ih, IupMotSwitchData* switch_data, IdrawCanvas* dc)
{
  int width, height;
  long track_color, thumb_color, shadow_dark, shadow_light;
  int is_checked = switch_data->checked_state;
  int is_active = iupdrvIsActive(ih);
  int thumb_x, thumb_y;

  iupdrvDrawGetSize(dc, &width, &height);

  if (!is_active)
  {
    track_color = iupDrawColor(200, 200, 200, 255);
    thumb_color = iupDrawColor(220, 220, 220, 255);
  }
  else if (is_checked)
  {
    track_color = iupDrawColor(180, 180, 180, 255);
    thumb_color = iupDrawColor(240, 240, 240, 255);
  }
  else
  {
    track_color = iupDrawColor(220, 220, 220, 255);
    thumb_color = iupDrawColor(240, 240, 240, 255);
  }

  iupdrvDrawRectangle(dc, 0, 0, SWITCH_TRACK_WIDTH - 1, SWITCH_TRACK_HEIGHT - 1, track_color, IUP_DRAW_FILL, 1);

  shadow_dark = iupDrawColor(100, 100, 100, 255);
  shadow_light = iupDrawColor(250, 250, 250, 255);

  iupdrvDrawLine(dc, 0, 0, SWITCH_TRACK_WIDTH - 1, 0, shadow_dark, IUP_DRAW_STROKE, 1);
  iupdrvDrawLine(dc, 0, 0, 0, SWITCH_TRACK_HEIGHT - 1, shadow_dark, IUP_DRAW_STROKE, 1);

  iupdrvDrawLine(dc, SWITCH_TRACK_WIDTH - 1, 1, SWITCH_TRACK_WIDTH - 1, SWITCH_TRACK_HEIGHT - 1, shadow_light, IUP_DRAW_STROKE, 1);
  iupdrvDrawLine(dc, 1, SWITCH_TRACK_HEIGHT - 1, SWITCH_TRACK_WIDTH - 1, SWITCH_TRACK_HEIGHT - 1, shadow_light, IUP_DRAW_STROKE, 1);

  if (is_checked)
    thumb_x = SWITCH_TRACK_WIDTH - SWITCH_THUMB_SIZE - SWITCH_THUMB_MARGIN;
  else
    thumb_x = SWITCH_THUMB_MARGIN;

  thumb_y = (SWITCH_TRACK_HEIGHT - SWITCH_THUMB_SIZE) / 2;

  iupdrvDrawRectangle(dc, thumb_x, thumb_y, thumb_x + SWITCH_THUMB_SIZE - 1, thumb_y + SWITCH_THUMB_SIZE - 1, thumb_color, IUP_DRAW_FILL, 1);

  iupdrvDrawLine(dc, thumb_x, thumb_y, thumb_x + SWITCH_THUMB_SIZE - 1, thumb_y, shadow_light, IUP_DRAW_STROKE, 1);
  iupdrvDrawLine(dc, thumb_x, thumb_y, thumb_x, thumb_y + SWITCH_THUMB_SIZE - 1, shadow_light, IUP_DRAW_STROKE, 1);

  iupdrvDrawLine(dc, thumb_x + SWITCH_THUMB_SIZE - 1, thumb_y + 1, thumb_x + SWITCH_THUMB_SIZE - 1, thumb_y + SWITCH_THUMB_SIZE - 1, shadow_dark, IUP_DRAW_STROKE, 1);
  iupdrvDrawLine(dc, thumb_x + 1, thumb_y + SWITCH_THUMB_SIZE - 1, thumb_x + SWITCH_THUMB_SIZE - 1, thumb_y + SWITCH_THUMB_SIZE - 1, shadow_dark, IUP_DRAW_STROKE, 1);
}

static void motSwitchExposeCallback(Widget w, IupMotSwitchData* switch_data, XEvent* evt, Boolean* cont)
{
  Ihandle* ih = switch_data->ih;
  IdrawCanvas* dc;
  Window win;

  (void)w;
  (void)evt;
  (void)cont;

  win = XtWindow(switch_data->drawing_area);
  if (!win)
    return;

  if (!iupAttribGet(ih, "DRAWABLE"))
    iupAttribSet(ih, "DRAWABLE", (char*)win);

  dc = iupdrvDrawCreateCanvas(ih);
  if (!dc)
    return;

  iupDrawParentBackground(dc, ih);

  motSwitchDraw(ih, switch_data, dc);

  iupdrvDrawFlush(dc);

  iupdrvDrawKillCanvas(dc);
}

static void motSwitchButtonPressCallback(Widget w, Ihandle* ih, XButtonPressedEvent* evt, Boolean* cont)
{
  IupMotSwitchData* switch_data = (IupMotSwitchData*)iupAttribGet(ih, "_IUPMOT_SWITCHDATA");
  IFni cb;
  int new_check;

  (void)w;
  (void)evt;
  (void)cont;

  if (!switch_data)
    return;

  iupmotTipLeaveNotify();

  new_check = switch_data->checked_state ? 0 : 1;
  switch_data->checked_state = new_check;

  XClearArea(iupmot_display, XtWindow(switch_data->drawing_area), 0, 0, 0, 0, True);

  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb && cb(ih, new_check) == IUP_CLOSE)
    IupExitLoop();

  if (iupObjectCheck(ih))
    iupBaseCallValueChangedCb(ih);
}

/*********************************************************************************/

static int motToggleSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    char* parent_value = iupAttribGetInheritNativeParent(ih, "BGCOLOR");
    if (!parent_value)
    {
      /* if not defined at a native parent, 
         then change the toggle button color to the given color instead using the default */
      if (iupdrvBaseSetBgColorAttrib(ih, value))  /* let XmChangeColor do its job */
      {
        parent_value = IupGetGlobal("DLGBGCOLOR");
        XtVaSetValues(ih->handle, XmNbackground, iupmotColorGetPixelStr(parent_value), NULL);  /* reset just the background */

        if (ih->data->is_radio)
          XtVaSetValues(ih->handle, XmNselectColor, iupmotColorGetPixel(0, 0, 0), NULL);
        XtVaSetValues(ih->handle, XmNunselectColor, iupmotColorGetPixelStr(value), NULL);
        return 1;
      }
    }
    else
    {
      /* ignore given value, must use only from parent */
      if (iupdrvBaseSetBgColorAttrib(ih, parent_value))
      {
        if (ih->data->is_radio)
          XtVaSetValues(ih->handle, XmNselectColor, iupmotColorGetPixel(0, 0, 0), NULL);
        XtVaSetValues(ih->handle, XmNunselectColor, iupmotColorGetPixelStr(parent_value), NULL);
        return 1;
      }
    }
    return 0; 
  }
  else
  {
    if (ih->data->flat)
    {
      /* ignore given value, must use only from parent */
      value = iupBaseNativeParentGetBgColor(ih);

      if (iupdrvBaseSetBgColorAttrib(ih, value))
        return 1;
    }

    return iupdrvBaseSetBgColorAttrib(ih, value); 
  }
}

static int motToggleSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    Pixel color;

    /* ignore given value, must use only from parent */
    value = iupAttribGetInheritNativeParent(ih, "BACKGROUND");

    color = iupmotColorGetPixelStr(value);
    if (color != (Pixel)-1)
    {
      XtVaSetValues(ih->handle, XmNbackground, color, NULL);
      return 1;
    }
    else
    {
      Pixmap pixmap = (Pixmap)iupImageGetImage(value, ih, 0, NULL);
      if (pixmap)
      {
        XtVaSetValues(ih->handle, XmNbackgroundPixmap, pixmap, NULL);
        return 1;
      }
    }
  }
  else
  {
    if (ih->data->flat)
    {
      /* ignore given value, must use only from parent */
      value = iupAttribGetInheritNativeParent(ih, "BACKGROUND");

      if (iupdrvBaseSetBgColorAttrib(ih, value))
        return 1;
      else
      {
        Pixmap pixmap = (Pixmap)iupImageGetImage(value, ih, 0, NULL);
        if (pixmap)
        {
          XtVaSetValues(ih->handle, XmNbackgroundPixmap, pixmap, NULL);
          return 1;
        }
      }
    }
  }
  return 0;
}

static int motToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
    return 0;

  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    iupmotSetMnemonicTitle(ih, NULL, 0, value);
    return 1;
  }

  return 0;
}

static int motToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  unsigned char align;
  char value1[30], value2[30];

  if (ih->data->type == IUP_TOGGLE_TEXT)
    return 0;

  iupStrToStrStr(value, value1, value2, ':');   /* value2 is ignored, NOT supported in Motif */

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    align = XmALIGNMENT_END;
  else if (iupStrEqualNoCase(value1, "ALEFT"))
    align = XmALIGNMENT_BEGINNING;
  else /* "ACENTER" (default) */
    align = XmALIGNMENT_CENTER;

  XtVaSetValues (ih->handle, XmNalignment, align, NULL);
  return 1;
}

static int motToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupmotSetPixmap(ih, value, XmNlabelPixmap, 0);

    if (!iupAttribGet(ih, "IMINACTIVE"))
    {
      /* if not active and IMINACTIVE is not defined 
         then automatically create one based on IMAGE */
      iupmotSetPixmap(ih, value, XmNlabelInsensitivePixmap, 1); /* make_inactive */
    }
    return 1;
  }
  else
    return 0;
}

static int motToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupmotSetPixmap(ih, value, XmNlabelInsensitivePixmap, 0);
    return 1;
  }
  else
    return 0;
}

static int motToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupmotSetPixmap(ih, value, XmNselectPixmap, 0);
    return 1;
  }
  else
    return 0;
}

static int motToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  Ihandle *radio;
  unsigned char check;

  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupMotSwitchData* switch_data = (IupMotSwitchData*)iupAttribGet(ih, "_IUPMOT_SWITCHDATA");
    int new_check;

    if (!switch_data)
      return 0;

    if (iupStrEqualNoCase(value, "TOGGLE"))
      new_check = !switch_data->checked_state;
    else
      new_check = iupStrBoolean(value);

    switch_data->checked_state = new_check;

    if (ih->handle)
      XClearArea(iupmot_display, XtWindow(switch_data->drawing_area), 0, 0, 0, 0, True);

    return 0;
  }

  if (iupStrEqualNoCase(value,"TOGGLE"))
    check = (unsigned char)-1;
  else if (iupStrEqualNoCase(value,"NOTDEF"))
    check = XmINDETERMINATE;
  else if (iupStrBoolean(value))
    check = XmSET;
  else
    check = XmUNSET;

  /* This is necessary because Motif toggle does not have support for radio. 
     It is implemented using an external RadioBox that we do not use. */
  radio = iupRadioFindToggleParent(ih);
  if (radio)
  {
    Ihandle* last_tg;
    unsigned char oldcheck;

    if (check == (unsigned char)-1)
      check = XmSET;

    XtVaGetValues(ih->handle, XmNset, &oldcheck, NULL);

    /* After map the first toggle in radio will set value=ON, but last_tg will be NULL */
    last_tg = (Ihandle*)iupAttribGet(radio, "_IUPMOT_LASTTOGGLE");
    if (check)
    {
      if (iupObjectCheck(last_tg) && last_tg != ih)
        XtVaSetValues(last_tg->handle, XmNset, XmUNSET, NULL);
      iupAttribSet(radio, "_IUPMOT_LASTTOGGLE", (char*)ih);
    }

    if (last_tg != ih && oldcheck != check)
      XtVaSetValues(ih->handle, XmNset, check, NULL);
  }
  else
  {
    if (check == (unsigned char)-1)
    {
      unsigned char oldcheck;
      XtVaGetValues(ih->handle, XmNset, &oldcheck, NULL);

      if (oldcheck == XmSET)
        check = XmUNSET;
      else
        check = XmSET;
    }

    XtVaSetValues(ih->handle, XmNset, check, NULL);
  }

  return 0;
}

static char* motToggleGetValueAttrib(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupMotSwitchData* switch_data = (IupMotSwitchData*)iupAttribGet(ih, "_IUPMOT_SWITCHDATA");
    if (switch_data)
      return iupStrReturnChecked(switch_data->checked_state);
    return iupStrReturnChecked(0);
  }

  {
    int check;
    unsigned char set = 0;
    XtVaGetValues (ih->handle, XmNset, &set, NULL);
    check = set;
    if (check == XmINDETERMINATE) check = -1;
    return iupStrReturnChecked(check);
  }
}

static int motToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle && ih->data->type == IUP_TOGGLE_IMAGE)
  {
    XtVaSetValues(ih->handle, XmNmarginHeight, ih->data->vert_padding,
                              XmNmarginWidth, ih->data->horiz_padding, NULL);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static char* motToggleGetSelectColorAttrib(Ihandle* ih)
{
  unsigned char r, g, b;
  Pixel color;
  XtVaGetValues(ih->handle, XmNselectColor, &color, NULL); 
  iupmotColorGetRGB(color, &r, &g, &b);
  return iupStrReturnStrf("%d %d %d", (int)r, (int)g, (int)b);
}

static int motToggleSetSelectColorAttrib(Ihandle* ih, const char *value)
{
  Pixel color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
    XtVaSetValues(ih->handle, XmNselectColor, color, NULL);
  return 1;
}

static void motToggleValueChangedCallback(Widget w, Ihandle* ih, XmToggleButtonCallbackStruct* call_data)
{
  Ihandle *radio;
  IFni cb;
  int check = call_data->set;

  /* Must manually hide the tip if the toggle is pressed. */
  iupmotTipLeaveNotify();

  /* This is necessary because Motif toggle does not have support for radio. 
     It is implemented using an external RadioBox that we do not use. */
  radio = iupRadioFindToggleParent(ih);
  if (radio)
  {
    if (check)
    {
      Ihandle* last_tg = (Ihandle*)iupAttribGet(radio, "_IUPMOT_LASTTOGGLE");
      if (iupObjectCheck(last_tg) && last_tg != ih)
      {
        /* uncheck last toggle */
        XtVaSetValues(last_tg->handle, XmNset, XmUNSET, NULL);

        cb = (IFni) IupGetCallback(last_tg, "ACTION");
        if (cb && cb(last_tg, 0) == IUP_CLOSE)
            IupExitLoop();

        if (iupObjectCheck(ih))
          iupBaseCallValueChangedCb(last_tg);
      }
      iupAttribSet(radio, "_IUPMOT_LASTTOGGLE", (char*)ih);

      if (last_tg != ih)
      {
        cb = (IFni)IupGetCallback(ih, "ACTION");
        if (cb && cb (ih, 1) == IUP_CLOSE)
            IupExitLoop();

        if (iupObjectCheck(ih))
          iupBaseCallValueChangedCb(ih);
      }
    }
    else
    {
      /* Force stay checked */
      XtVaSetValues(ih->handle, XmNset, XmSET, NULL);
    }
  }
  else
  {
    if (check == XmINDETERMINATE)
       check = -1;

    cb = (IFni)IupGetCallback(ih, "ACTION");
    if (cb)
    {
      if (cb(ih, check) == IUP_CLOSE) 
        IupExitLoop();
    }

    if (iupObjectCheck(ih))
      iupBaseCallValueChangedCb(ih);
  }

  (void)w;
}

static void motToggleEnterLeaveWindowEvent(Widget w, Ihandle* ih, XEvent *evt, Boolean *cont)
{
  /* Used only when FLAT=Yes */
  unsigned char check = 0;

  iupmotEnterLeaveWindowEvent(w, ih, evt, cont);

  XtVaGetValues (ih->handle, XmNset, &check, NULL);
  if (check == XmSET)
    XtVaSetValues(ih->handle, XmNshadowThickness, 2, NULL);
  else
  {
    if (evt->type == EnterNotify)
      XtVaSetValues(ih->handle, XmNshadowThickness, 2, NULL);
    else  if (evt->type == LeaveNotify)
      XtVaSetValues(ih->handle, XmNshadowThickness, 0, NULL);
  }
}

static int motToggleMapMethod(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  char* value;
  int num_args = 0;
  Arg args[40];

  if (radio)
    ih->data->is_radio = 1;

  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupMotSwitchData* switch_data;

    if (ih->data->is_radio)
    {
      iupAttribSet(ih, "SWITCH", "NO");
      goto regular_toggle;
    }

    ih->data->type = IUP_TOGGLE_TEXT;

    switch_data = (IupMotSwitchData*)calloc(1, sizeof(IupMotSwitchData));
    switch_data->ih = ih;
    iupAttribSet(ih, "_IUPMOT_SWITCHDATA", (char*)switch_data);

    iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);
    iupMOT_SETARG(args, num_args, XmNx, 0);
    iupMOT_SETARG(args, num_args, XmNy, 0);
    iupMOT_SETARG(args, num_args, XmNwidth, SWITCH_TRACK_WIDTH);
    iupMOT_SETARG(args, num_args, XmNheight, SWITCH_TRACK_HEIGHT);
    iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);
    iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
    iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);

    if (iupAttribGetBoolean(ih, "CANFOCUS"))
      iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
    else
      iupMOT_SETARG(args, num_args, XmNtraversalOn, False);

    switch_data->drawing_area = XtCreateManagedWidget(
      iupDialogGetChildIdStr(ih),
      xmDrawingAreaWidgetClass,
      iupChildTreeGetNativeParentHandle(ih),
      args, num_args);

    if (!switch_data->drawing_area)
    {
      free(switch_data);
      return IUP_ERROR;
    }

    ih->handle = switch_data->drawing_area;
    ih->serial = iupDialogGetChildId(ih);

    XtAddCallback(ih->handle, XmNexposeCallback, (XtCallbackProc)motSwitchExposeCallback, (XtPointer)switch_data);
    XtAddEventHandler(ih->handle, ButtonPressMask, False, (XtEventHandler)motSwitchButtonPressCallback, (XtPointer)ih);
    XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, KeyPressMask, False, (XtEventHandler)iupmotKeyPressEvent, (XtPointer)ih);
    XtAddCallback(ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);

    value = iupAttribGet(ih, "VALUE");
    if (value && iupStrBoolean(value))
      switch_data->checked_state = 1;
    else
      switch_data->checked_state = 0;

    XtVaSetValues(ih->handle, XmNbackground, iupmotColorGetPixelStr(iupBaseNativeParentGetBgColor(ih)), NULL);

    XtRealizeWidget(ih->handle);

    iupAttribSet(ih, "DRAWABLE", (char*)XtWindow(ih->handle));

    return IUP_NOERROR;
  }

regular_toggle:

  value = iupAttribGet(ih, "IMAGE");
  if (value)
  {
    ih->data->type = IUP_TOGGLE_IMAGE;
    iupMOT_SETARG(args, num_args, XmNlabelType, XmPIXMAP); 
  }
  else
  {
    ih->data->type = IUP_TOGGLE_TEXT;
    iupMOT_SETARG(args, num_args, XmNlabelType, XmSTRING); 
  }

  /* Core */
  iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* not visible when managed */
  iupMOT_SETARG(args, num_args, XmNx, 0);  /* x-position */
  iupMOT_SETARG(args, num_args, XmNy, 0);  /* y-position */
  iupMOT_SETARG(args, num_args, XmNwidth, 10);  /* default width to avoid 0 */
  iupMOT_SETARG(args, num_args, XmNheight, 10); /* default height to avoid 0 */
  /* Primitive */
  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
  else
    iupMOT_SETARG(args, num_args, XmNtraversalOn, False);
  iupMOT_SETARG(args, num_args, XmNhighlightThickness, 2);
  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
  /* Label */
  iupMOT_SETARG(args, num_args, XmNrecomputeSize, False);  /* no automatic resize from text */
  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);  /* default padding */
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNmarginTop, 0);     /* no extra margins */
  iupMOT_SETARG(args, num_args, XmNmarginLeft, 0);
  iupMOT_SETARG(args, num_args, XmNmarginBottom, 0);
  iupMOT_SETARG(args, num_args, XmNmarginRight, 0);

  if (ih->data->is_radio)
  {
    iupMOT_SETARG(args, num_args, XmNtoggleMode, XmTOGGLE_BOOLEAN);
    iupMOT_SETARG(args, num_args, XmNindicatorType, XmONE_OF_MANY_ROUND);

    if (!iupAttribGet(radio, "_IUPMOT_LASTTOGGLE"))
    {
      /* this is the first toggle in the radio, and the last toggle with VALUE=ON */
      /* After map the attribute value will be processed and _IUPMOT_LASTTOGGLE will be set */
      iupAttribSet(ih, "VALUE","ON");
    }

    /* make sure it has at least one name */
    if (!iupAttribGetHandleName(ih))
      iupAttribSetHandleName(ih);
  }
  else
  {
    if (ih->data->type == IUP_TOGGLE_TEXT && iupAttribGetBoolean(ih, "3STATE"))
      iupMOT_SETARG(args, num_args, XmNtoggleMode, XmTOGGLE_INDETERMINATE);
    else
      iupMOT_SETARG(args, num_args, XmNtoggleMode, XmTOGGLE_BOOLEAN);
    iupMOT_SETARG(args, num_args, XmNindicatorType, XmN_OF_MANY);
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupMOT_SETARG(args, num_args, XmNindicatorOn, XmINDICATOR_NONE);
    iupMOT_SETARG(args, num_args, XmNalignment, XmALIGNMENT_CENTER);
    iupMOT_SETARG(args, num_args, XmNshadowThickness, 2);
  }
  else
  {
    iupMOT_SETARG(args, num_args, XmNspacing, 3);
    iupMOT_SETARG(args, num_args, XmNindicatorOn, XmINDICATOR_CHECK_BOX);
    iupMOT_SETARG(args, num_args, XmNalignment, XmALIGNMENT_BEGINNING);

    if (ih->data->is_radio)
    {
      iupMOT_SETARG(args, num_args, XmNindicatorSize, 13);
      iupMOT_SETARG(args, num_args, XmNselectColor, iupmotColorGetPixel(0, 0, 0));
    }
    else
      iupMOT_SETARG(args, num_args, XmNindicatorSize, 15);

    iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);
    iupMOT_SETARG(args, num_args, XmNdetailShadowThickness, 2);
  }

  ih->handle = XtCreateManagedWidget(
    iupDialogGetChildIdStr(ih),  /* child identifier */
    xmToggleButtonWidgetClass,     /* widget class */
    iupChildTreeGetNativeParentHandle(ih), /* widget parent */
    args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  XtAddCallback(ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);

  if (ih->data->type == IUP_TOGGLE_IMAGE && ih->data->flat)
  {
    XtVaSetValues(ih->handle, XmNshadowThickness, 0, NULL);
    XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)motToggleEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)motToggleEnterLeaveWindowEvent, (XtPointer)ih);
  }
  else
  {
    XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
    XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  }

  XtAddEventHandler(ih->handle, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyPressMask,    False, (XtEventHandler)iupmotKeyPressEvent,    (XtPointer)ih);

  XtAddCallback(ih->handle, XmNvalueChangedCallback, (XtCallbackProc)motToggleValueChangedCallback, (XtPointer)ih);

  if (iupStrBoolean(IupGetGlobal("INPUTCALLBACKS")))
    XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotDummyPointerMotionEvent, NULL);

  /* Disable Drag Source */
  iupmotDisableDragSource(ih->handle);

  /* initialize the widget */
  XtRealizeWidget(ih->handle);

  if (ih->data->type == IUP_TOGGLE_TEXT)
    iupmotSetXmString(ih->handle, XmNlabelString, "");

  return IUP_NOERROR;
}

static void motToggleUnMapMethod(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupMotSwitchData* switch_data = (IupMotSwitchData*)iupAttribGet(ih, "_IUPMOT_SWITCHDATA");
    if (switch_data)
    {
      free(switch_data);
      iupAttribSet(ih, "_IUPMOT_SWITCHDATA", NULL);
    }
  }

  iupdrvBaseUnMapMethod(ih);
}

void iupdrvToggleInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motToggleMapMethod;
  ic->UnMap = motToggleUnMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", iupmotGetBgColorAttrib, motToggleSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_SAVE);
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, motToggleSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, motToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupToggle only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, motToggleSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ACENTER:ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, motToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, motToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, motToggleSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", motToggleGetValueAttrib, motToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTCOLOR", motToggleGetSelectColorAttrib, motToggleSetSelectColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, motToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  /* NOT supported */
  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
