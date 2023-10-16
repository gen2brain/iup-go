/** \file
 * \brief Canvas Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/BulletinB.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_canvas.h"
#include "iup_key.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


static void motCanvasScrollbarCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  int op = (int)client_data, ipage, ipos;
  Ihandle *ih;
  IFniff cb;
  double posx, posy;
  (void)call_data;

  XtVaGetValues(w, XmNuserData, &ih, NULL);
  if (!ih) 
    return;

  XtVaGetValues(w, 
    XmNvalue, &ipos, 
    XmNpageIncrement, &ipage,
    NULL);

  if (op > IUP_SBDRAGV)
  {
    iupCanvasCalcScrollRealPos(iupAttribGetDouble(ih,"XMIN"), iupAttribGetDouble(ih,"XMAX"), &posx, 
                               IUP_SB_MIN, IUP_SB_MAX, ipage, &ipos);
    ih->data->posx = posx;
    posy = ih->data->posy;
  }
  else
  {
    iupCanvasCalcScrollRealPos(iupAttribGetDouble(ih,"YMIN"), iupAttribGetDouble(ih,"YMAX"), &posy, 
                               IUP_SB_MIN, IUP_SB_MAX, ipage, &ipos);
    ih->data->posy = posy;
    posx = ih->data->posx;
  }

  cb = (IFniff)IupGetCallback(ih,"SCROLL_CB");
  if (cb)
    cb(ih, op, (float)posx, (float)posy);
  else
  {
    IFnff action_cb = (IFnff)IupGetCallback(ih,"ACTION");
    if (action_cb)
    {
      /* REDRAW Now (since 3.24) - to allow a full native redraw process */
      iupdrvRedrawNow(ih);
      /* action_cb(ih, (float)posx, (float)posy); - OLD method */
    }
  }
}

static void motCanvasSetSize(Ihandle *ih, Widget sb_win, int setsize)
{
  Widget sb_horiz = (Widget)iupAttribGet(ih, "_IUPMOT_SBHORIZ");
  Widget sb_vert = (Widget)iupAttribGet(ih, "_IUPMOT_SBVERT");
  int sb_vert_width=0, sb_horiz_height=0;
  int width, height;
  Dimension border;

  /* IMPORTANT:
   The border, added by the Core, is NOT included in the Motif size.
   So when setting the size, we must compensate the border, 
   so the actual size will be the size we expect. */
  XtVaGetValues(sb_win, XmNborderWidth, &border, NULL);
  width = ih->currentwidth - 2*border;
  height = ih->currentheight - 2*border;

  /* avoid abort in X */
  if (width <= 0) width = 1;
  if (height <= 0) height = 1;

  if (setsize)
  {
    XtVaSetValues(sb_win,
      XmNwidth, (XtArgVal)width,
      XmNheight, (XtArgVal)height,
      NULL);
  }

  if (sb_vert && XtIsManaged(sb_vert))
    sb_vert_width = iupdrvGetScrollbarSize();
  if (sb_horiz && XtIsManaged(sb_horiz))
    sb_horiz_height = iupdrvGetScrollbarSize();

  if (sb_vert_width)
  {
    XtVaSetValues(sb_vert,
      XmNwidth, (XtArgVal)sb_vert_width,
      XmNheight, (XtArgVal)height-sb_horiz_height,
      NULL);
    iupmotSetPosition(sb_vert, width-sb_vert_width, 0);
  }
  if (sb_horiz_height)
  {
    XtVaSetValues(sb_horiz,
      XmNwidth, (XtArgVal)width-sb_vert_width,
      XmNheight, (XtArgVal)sb_horiz_height,
      NULL);
    iupmotSetPosition(sb_horiz, 0, height-sb_horiz_height);
  }

  XtVaSetValues(ih->handle,
    XmNwidth, (XtArgVal)width-sb_vert_width,
    XmNheight, (XtArgVal)height-sb_horiz_height,
    NULL);
}

static void motCanvasUpdateScrollLayout(Ihandle *ih)
{
  Widget sb_win = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  motCanvasSetSize(ih, sb_win, 0);
}

static void motCanvasResizeCallback(Widget w, Ihandle *ih, XtPointer call_data)
{
  IFnii cb;
  (void)call_data;

  if (!XtWindow(w) || !ih) 
    return;

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (cb && !(ih->data->inside_resize))
  {
    /* client size */
    Dimension width, height;
    XtVaGetValues(w, XmNwidth, &width,
                     XmNheight, &height,
                     NULL);

    ih->data->inside_resize = 1;  /* avoid recursion */
    cb(ih, width, height);
    ih->data->inside_resize = 0;
  }
}

static int motCanvasSetBgColorAttrib(Ihandle* ih, const char* value);

static void motCanvasExposeCallback(Widget w, Ihandle *ih, XtPointer call_data)
{
  IFnff cb;
  (void)call_data;

  if (!XtWindow(w) || !ih) 
    return;

  cb = (IFnff)IupGetCallback(ih,"ACTION");
  if (cb && !(ih->data->inside_resize))
  {
    if (!iupAttribGet(ih, "_IUPMOT_NO_BGCOLOR"))
      motCanvasSetBgColorAttrib(ih, iupAttribGetStr(ih, "BGCOLOR"));  /* reset to update window attributes */

    cb(ih, (float)ih->data->posx, (float)ih->data->posy);
  }
}

static void motCanvasInputCallback(Widget w, Ihandle *ih, XtPointer call_data)
{
  XEvent *evt = ((XmDrawingAreaCallbackStruct*)call_data)->event;

  if (!XtWindow(w) || !ih) return;

  switch (evt->type)
  {
  case ButtonPress:
    /* Force focus on canvas click */
    if (iupAttribGetBoolean(ih, "CANFOCUS"))
      XmProcessTraversal(w, XmTRAVERSE_CURRENT);
    /* break missing on purpose... */
  case ButtonRelease:
    {
      XButtonEvent *but_evt = (XButtonEvent*)evt;
      Boolean cont = True;
      iupmotButtonPressReleaseEvent(w, ih, evt, &cont);
      if (cont == False)
        return;

      if ((evt->type==ButtonPress) && (but_evt->button==Button4 || but_evt->button==Button5))
      {                                             
        IFnfiis wcb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");

        if (iupAttribGetBoolean(ih, "WHEELDROPFOCUS"))
        {
          Ihandle* ih_focus = IupGetFocus();
          if (iupObjectCheck(ih_focus))
            iupAttribSetClassObject(ih_focus, "SHOWDROPDOWN", "NO");
        }

        if (wcb)
        {
          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          int delta = but_evt->button==Button4? 1: -1;
          iupmotButtonKeySetStatus(but_evt->state, but_evt->button, status, 0);

          wcb(ih, (float)delta, but_evt->x, but_evt->y, status);
        }
        else
        {
          IFniff scb = (IFniff)IupGetCallback(ih,"SCROLL_CB");
          double posy = ih->data->posy;
          int delta = but_evt->button==Button4? 1: -1;
          int op = but_evt->button==Button4? IUP_SBUP: IUP_SBDN;
          posy -= delta*iupAttribGetDouble(ih, "DY")/10.0;
          IupSetDouble(ih, "POSY", posy);
          if (scb)
            scb(ih, op, (float)ih->data->posx, (float)ih->data->posy);
        }
      }
    }
    break;
  }
}

static void motCanvasSetScrollInfo(Widget sb, int imin, int imax, int ipos, int ipage, int iline)
{
  XtVaSetValues(sb,
                XmNminimum,       imin,
                XmNmaximum,       imax,
                XmNvalue,         ipos,
                XmNincrement,     iline,
                XmNpageIncrement, ipage,
                XmNsliderSize,    ipage,  /* to make the thumb proportional */
                NULL);
}

static int motCanvasSetDXAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    double posx, xmin, xmax;
    double dx;
    int iposx, ipagex, ilinex;
    Widget sb_horiz = (Widget)iupAttribGet(ih, "_IUPMOT_SBHORIZ");
    if (!sb_horiz) return 1;

    if (!iupStrToDoubleDef(value, &dx, 0.1))
      return 1;

    iupAttribSet(ih, "SB_RESIZE", NULL);

    xmin = iupAttribGetDouble(ih, "XMIN");
    xmax = iupAttribGetDouble(ih, "XMAX");
    posx = ih->data->posx;

    iupCanvasCalcScrollIntPos(xmin, xmax, dx, posx, 
                              IUP_SB_MIN, IUP_SB_MAX, &ipagex, &iposx);

    if (!iupAttribGet(ih,"LINEX"))
    {
      ilinex = ipagex/10;
      if (!ilinex)
        ilinex = 1;
    }
    else
    {
      /* line and page conversions are the same */
      double linex = iupAttribGetDouble(ih,"LINEX");
      iupCanvasCalcScrollIntPos(xmin, xmax, linex, 0, 
                                IUP_SB_MIN, IUP_SB_MAX, &ilinex,  NULL);
    }

    if (dx >= (xmax-xmin))
    {
      if (iupAttribGetBoolean(ih, "XAUTOHIDE"))
      {
        if (XtIsManaged(sb_horiz))
        {
          if (iupdrvIsVisible(ih))
            iupAttribSet(ih, "SB_RESIZE", "YES");
          XtUnmanageChild(sb_horiz);
          motCanvasUpdateScrollLayout(ih);
        }

        iupAttribSet(ih, "XHIDDEN", "YES");
      }
      else
        XtSetSensitive(sb_horiz, False);

      ih->data->posx = xmin;
      XtVaSetValues(sb_horiz, XmNvalue, IUP_SB_MIN, NULL);
    }
    else
    {
      if (!XtIsManaged(sb_horiz))
      {
        if (iupdrvIsVisible(ih))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        XtManageChild(sb_horiz);
        motCanvasUpdateScrollLayout(ih);
      }
      XtSetSensitive(sb_horiz, True);

      motCanvasSetScrollInfo(sb_horiz, IUP_SB_MIN, IUP_SB_MAX, iposx, ipagex, ilinex);

      /* update position because it could have being changed */
      iupCanvasCalcScrollRealPos(xmin, xmax, &posx,
                                 IUP_SB_MIN, IUP_SB_MAX, ipagex, &iposx);

      iupAttribSet(ih, "XHIDDEN", "NO");

      ih->data->posx = posx;
    }
  }
  return 1;
}

static int motCanvasSetPosXAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    double xmin, xmax, dx;
    double posx;
    int iposx, ipagex;
    Widget sb_horiz = (Widget)iupAttribGet(ih, "_IUPMOT_SBHORIZ");
    if (!sb_horiz) return 1;

    if (!iupStrToDouble(value, &posx))
      return 1;

    xmin = iupAttribGetDouble(ih, "XMIN");
    xmax = iupAttribGetDouble(ih, "XMAX");
    dx = iupAttribGetDouble(ih, "DX");

    if (dx >= xmax - xmin)
      return 0;

    if (posx < xmin) posx = xmin;
    if (posx > (xmax - dx)) posx = xmax - dx;
    ih->data->posx = posx;

    iupCanvasCalcScrollIntPos(xmin, xmax, dx, posx, 
                              IUP_SB_MIN, IUP_SB_MAX, &ipagex, &iposx);

    XtVaSetValues(sb_horiz, XmNvalue, iposx, NULL);
  }
  return 1;
}

static int motCanvasSetDYAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    double posy, ymin, ymax;
    double dy;
    int iposy, ipagey, iliney;
    Widget sb_vert = (Widget)iupAttribGet(ih, "_IUPMOT_SBVERT");
    if (!sb_vert) return 1;

    if (!iupStrToDoubleDef(value, &dy, 0.1))
      return 1;

    iupAttribSet(ih, "SB_RESIZE", NULL);

    ymin = iupAttribGetDouble(ih, "YMIN");
    ymax = iupAttribGetDouble(ih, "YMAX");
    posy = ih->data->posy;

    iupCanvasCalcScrollIntPos(ymin, ymax, dy, posy, 
                              IUP_SB_MIN, IUP_SB_MAX, &ipagey, &iposy);

    if (!iupAttribGet(ih,"LINEY"))
    {
      iliney = ipagey/10;
      if (!iliney)
        iliney = 1;
    }
    else
    {
      /* line and page conversions are the same */
      double liney = iupAttribGetDouble(ih,"LINEY");
      iupCanvasCalcScrollIntPos(ymin, ymax, liney, 0, 
                                IUP_SB_MIN, IUP_SB_MAX, &iliney,  NULL);
    }

    if (dy >= (ymax-ymin))
    {
      if (iupAttribGetBoolean(ih, "YAUTOHIDE"))
      {
        if (XtIsManaged(sb_vert))
        {
          if (iupdrvIsVisible(ih))
            iupAttribSet(ih, "SB_RESIZE", "YES");
          XtUnmanageChild(sb_vert);
          motCanvasUpdateScrollLayout(ih);
        }

        iupAttribSet(ih, "YHIDDEN", "YES");
      }
      else
        XtSetSensitive(sb_vert, False);

      ih->data->posy = ymin;
      XtVaSetValues(sb_vert, XmNvalue, IUP_SB_MIN, NULL);
    }
    else
    {
      if (!XtIsManaged(sb_vert))
      {
        if (iupdrvIsVisible(ih))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        XtManageChild(sb_vert);
        motCanvasUpdateScrollLayout(ih);
      }
      XtSetSensitive(sb_vert, True);

      motCanvasSetScrollInfo(sb_vert, IUP_SB_MIN, IUP_SB_MAX, iposy, ipagey, iliney);

      /* update position because it could have being changed */
      iupCanvasCalcScrollRealPos(ymin, ymax, &posy,
                                 IUP_SB_MIN, IUP_SB_MAX, ipagey, &iposy);

      iupAttribSet(ih, "YHIDDEN", "NO");

      ih->data->posy = posy;
    }
  }
  return 1;
}

static int motCanvasSetPosYAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    double ymin, ymax, dy;
    double posy;
    int iposy, ipagey;
    Widget sb_vert = (Widget)iupAttribGet(ih, "_IUPMOT_SBVERT");
    if (!sb_vert) return 1;

    if (!iupStrToDouble(value, &posy))
      return 1;

    ymin = iupAttribGetDouble(ih, "YMIN");
    ymax = iupAttribGetDouble(ih, "YMAX");
    dy = iupAttribGetDouble(ih, "DY");

    if (dy >= ymax - ymin)
      return 0;

    if (posy < ymin) posy = ymin;
    if (posy > (ymax - dy)) posy = ymax - dy;
    ih->data->posy = posy;

    iupCanvasCalcScrollIntPos(ymin, ymax, dy, posy, 
                              IUP_SB_MIN, IUP_SB_MAX, &ipagey, &iposy);

    XtVaSetValues(sb_vert, XmNvalue, iposy, NULL);
  }
  return 1;
}

static char* motCanvasGetXDisplayAttrib(Ihandle *ih)
{
  (void)ih;
  return (char*)iupmot_display;
}

static char* motCanvasGetXScreenAttrib(Ihandle *ih)
{
  (void)ih;
  return (char*)iupmot_screen;
}

static char* motCanvasGetXWindowAttrib(Ihandle *ih)
{
  return (char*)XtWindow(ih->handle);
}

static char* motCanvasGetDrawSizeAttrib(Ihandle *ih)
{
  Dimension width, height;
  XtVaGetValues(ih->handle, XmNwidth,  &width,
                            XmNheight, &height, 
                            NULL);

  return iupStrReturnIntInt((int)width, (int)height, 'x');
}

static int motCanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Pixel color;

  /* ignore given value, must use only from parent for the scrollbars */
  char* parent_value = iupBaseNativeParentGetBgColor(ih);

  color = iupmotColorGetPixelStr(parent_value);
  if (color != (Pixel)-1 && iupStrBoolean(IupGetGlobal("SB_BGCOLOR")))
  {
    Widget sb;
    Widget sb_win = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");

    iupmotSetBgColor(sb_win, color);

    sb = (Widget)iupAttribGet(ih, "_IUPMOT_SBVERT");
    if (sb) iupmotSetBgColor(sb, color);

    sb = (Widget)iupAttribGet(ih, "_IUPMOT_SBHORIZ");
    if (sb) iupmotSetBgColor(sb, color);
  }

  if (!IupGetCallback(ih, "ACTION")) 
    iupdrvBaseSetBgColorAttrib(ih, value);  /* Use the given value only here */
  else
  {
    XSetWindowAttributes attrs;
    attrs.background_pixmap = None;
    XChangeWindowAttributes(iupmot_display, XtWindow(ih->handle), CWBackPixmap, &attrs);
    iupAttribSet(ih, "_IUPMOT_NO_BGCOLOR", "1");
  }

  return 1;
}

static void motCanvasLayoutUpdateMethod(Ihandle *ih)
{
  Widget sb_win = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  motCanvasSetSize(ih, sb_win, 1);
  iupmotSetPosition(sb_win, ih->x, ih->y);
}

static int motCanvasMapMethod(Ihandle* ih)
{
  Widget sb_win;
  char *visual;
  int num_args = 0;
  Arg args[20];

  if (!ih->parent)
    return IUP_ERROR;

  ih->data->sb = iupBaseGetScrollbar(ih);

  /*********************/
  /* Create the parent */
  /*********************/

  iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* not visible when managed */
  iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);
  iupMOT_SETARG(args, num_args, XmNwidth, 100);  /* set this to avoid size calculation problems */
  iupMOT_SETARG(args, num_args, XmNheight, 100);
  iupMOT_SETARG(args, num_args, XmNresizePolicy, XmRESIZE_NONE);
  iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);

  if (iupAttribGetBoolean(ih, "BORDER"))
  {
    iupMOT_SETARG(args, num_args, XmNborderWidth, 1);
    iupMOT_SETARG(args, num_args, XmNborderColor, iupmotColorGetPixelStr("0 0 0"));
  }
  else
    iupMOT_SETARG(args, num_args, XmNborderWidth, 0);
  
  sb_win = XtCreateManagedWidget(
    iupDialogGetChildIdStr(ih),  /* child identifier */
    xmBulletinBoardWidgetClass,     /* widget class */
    iupChildTreeGetNativeParentHandle(ih), /* widget parent */
    args, num_args);

  if (!sb_win)
    return IUP_ERROR;

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  /****************************/
  /* Create the drawing area  */
  /****************************/

  num_args = 0;
  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);  /* no shadow margins */
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);  /* no shadow margins */
  iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);
  iupMOT_SETARG(args, num_args, XmNresizePolicy, XmRESIZE_NONE); /* no automatic resize of children */
  if (ih->iclass->is_interactive)
  {
    iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP); /* include in navigation */

    if (iupAttribGetBoolean(ih, "CANFOCUS"))
      iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
    else
      iupMOT_SETARG(args, num_args, XmNtraversalOn, False);
  }
  else
  {
    iupMOT_SETARG(args, num_args, XmNnavigationType, XmNONE);
    iupMOT_SETARG(args, num_args, XmNtraversalOn, False);
  }

  iupMOT_SETARG(args, num_args, XmNx, 0);  /* x-position */
  iupMOT_SETARG(args, num_args, XmNy, 0);  /* y-position */
  iupMOT_SETARG(args, num_args, XmNwidth, 10);  /* default width to avoid 0 */
  iupMOT_SETARG(args, num_args, XmNheight, 10); /* default height to avoid 0 */

  visual = IupGetAttribute(ih, "VISUAL");   /* defined by the OpenGL Canvas or NULL */
  if (visual)
  {
    Colormap colormap = (Colormap)iupAttribGet(ih, "COLORMAP");
    if (colormap)
      iupMOT_SETARG(args, num_args, XmNcolormap,colormap);

    iupmotDialogSetVisual(ih, visual);
  }

  ih->handle = XtCreateManagedWidget(
    "draw_area",                 /* child identifier */
    xmDrawingAreaWidgetClass,    /* widget class */
    sb_win,                  /* widget parent */
    args, num_args);

  if (!ih->handle)
  {
    XtDestroyWidget(sb_win);
    return IUP_ERROR;
  }

  if (visual)
    iupmotDialogResetVisual(ih);

  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)sb_win);

  {
    XSetWindowAttributes attrs;
    attrs.bit_gravity = ForgetGravity; /* For the DrawingArea widget gets Expose events when you resize it to be smaller. */

    if (iupAttribGetBoolean(ih, "BACKINGSTORE"))
      attrs.backing_store = WhenMapped;
    else
      attrs.backing_store = NotUseful;

    XChangeWindowAttributes(iupmot_display, XtWindow(ih->handle), CWBitGravity|CWBackingStore, &attrs);
  }

  if (ih->data->sb & IUP_SB_HORIZ)
  {
    Widget sb_horiz = XtVaCreateManagedWidget("sb_horiz",
      xmScrollBarWidgetClass, sb_win,
      XmNorientation, XmHORIZONTAL,
      XmNsliderMark, XmTHUMB_MARK,
      XmNuserData, ih,
      NULL);

    XtAddCallback(sb_horiz, XmNvalueChangedCallback, motCanvasScrollbarCallback, (void*)IUP_SBPOSH);
    XtAddCallback(sb_horiz, XmNdragCallback, motCanvasScrollbarCallback, (void*)IUP_SBDRAGH);
    XtAddCallback(sb_horiz, XmNdecrementCallback, motCanvasScrollbarCallback, (void*)IUP_SBLEFT);
    XtAddCallback(sb_horiz, XmNincrementCallback, motCanvasScrollbarCallback, (void*)IUP_SBRIGHT);
    XtAddCallback(sb_horiz, XmNpageDecrementCallback, motCanvasScrollbarCallback, (void*)IUP_SBPGLEFT);
    XtAddCallback(sb_horiz, XmNpageIncrementCallback, motCanvasScrollbarCallback, (void*)IUP_SBPGRIGHT);

    iupAttribSet(ih, "_IUPMOT_SBHORIZ", (char*)sb_horiz);
  }

  if (ih->data->sb & IUP_SB_VERT)
  {
    Widget sb_vert = XtVaCreateManagedWidget("sb_vert",
      xmScrollBarWidgetClass, sb_win,
      XmNorientation, XmVERTICAL,
      XmNsliderMark, XmTHUMB_MARK,
      XmNuserData, ih,
      NULL);

    XtAddCallback(sb_vert, XmNvalueChangedCallback, motCanvasScrollbarCallback, (void*)IUP_SBPOSV);
    XtAddCallback(sb_vert, XmNdragCallback, motCanvasScrollbarCallback, (void*)IUP_SBDRAGV);
    XtAddCallback(sb_vert, XmNdecrementCallback, motCanvasScrollbarCallback, (void*)IUP_SBUP);
    XtAddCallback(sb_vert, XmNincrementCallback, motCanvasScrollbarCallback, (void*)IUP_SBDN);
    XtAddCallback(sb_vert, XmNpageDecrementCallback, motCanvasScrollbarCallback, (void*)IUP_SBPGUP);
    XtAddCallback(sb_vert, XmNpageIncrementCallback, motCanvasScrollbarCallback, (void*)IUP_SBPGDN);

    iupAttribSet(ih, "_IUPMOT_SBVERT", (char*)sb_vert);
  }

  XtAddCallback(ih->handle, XmNhelpCallback, (XtCallbackProc)iupmotHelpCallback, (XtPointer)ih);
  XtAddCallback(ih->handle, XmNexposeCallback, (XtCallbackProc)motCanvasExposeCallback, (XtPointer)ih);
  XtAddCallback(ih->handle, XmNresizeCallback, (XtCallbackProc)motCanvasResizeCallback,  (XtPointer)ih);
  XtAddCallback(ih->handle, XmNinputCallback,  (XtCallbackProc)motCanvasInputCallback,   (XtPointer)ih);

  XtAddEventHandler(ih->handle, EnterWindowMask, False,(XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, LeaveWindowMask, False,(XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);

  XtAddEventHandler(ih->handle, FocusChangeMask, False,(XtEventHandler)iupmotFocusChangeEvent,        (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyPressMask,    False, (XtEventHandler)iupmotKeyPressEvent,    (XtPointer)ih);
  XtAddEventHandler(ih->handle, KeyReleaseMask,  False, (XtEventHandler)iupmotCanvasKeyReleaseEvent,  (XtPointer)ih);
  XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotPointerMotionEvent, (XtPointer)ih);

  /* initialize the widget */
  XtRealizeWidget(sb_win);

  motCanvasSetDXAttrib(ih, NULL);
  motCanvasSetDYAttrib(ih, NULL);

  return IUP_NOERROR;
}

void iupdrvCanvasInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motCanvasMapMethod;
  ic->LayoutUpdate = motCanvasLayoutUpdateMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, motCanvasSetBgColorAttrib, "255 255 255", NULL, IUPAF_DEFAULT);  /* force new default value */
  
  /* IupCanvas only */
  iupClassRegisterAttribute(ic, "DRAWSIZE", motCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DX", NULL, motCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "DY", NULL, motCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, motCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, motCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);  /* force new default value */

  /* IupCanvas X only */
  iupClassRegisterAttribute(ic, "XWINDOW", motCanvasGetXWindowAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "XDISPLAY", motCanvasGetXDisplayAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "XSCREEN", motCanvasGetXScreenAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, "YES", NULL, IUPAF_DEFAULT);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
