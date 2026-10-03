/** \file
 * \brief Motif Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <Xm/Xm.h>
#include <Xm/ScrollBar.h>
#include <X11/cursorfont.h>
#if defined(XM_UTF8)
#include <Xm/Cursor.h>
#endif
#include <X11/IntrinsicP.h>
#include <X11/CompositeP.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_image.h"

#include "iupmot_color.h"
#include "iupmot_drv.h"


IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
  XtCallActionProc(ih->handle, "ArmAndActivate", 0, 0, 0 );
}

IUP_DRV_API void iupmotSetGLBackgroundChild(Ihandle* ih)
{
  Ihandle* native_parent;

  if (!ih->handle || !XtWindow(ih->handle) || iupAttribGet(ih, "BGCOLOR"))
    return;

  native_parent = iupChildTreeGetNativeParent(ih);
  if (native_parent && (IupClassMatch(native_parent, "glbackgroundbox") || iupAttribGet(native_parent, "_IUPMOT_GLTRANSPARENT")))
  {
    XSetWindowBackgroundPixmap(iupmot_display, XtWindow(ih->handle), ParentRelative);
    iupAttribSet(ih, "_IUPMOT_GLTRANSPARENT", "1");
  }
}

static void motSaveAttributesRec(Ihandle* ih)
{
  Ihandle* child;

  IupSaveClassAttributes(ih);

  for (child = ih->firstchild; child; child = child->brother)
    motSaveAttributesRec(child);
}

static Widget motGetShell(Widget widget)
{
  while (widget && !XtIsShell(widget))
    widget = XtParent(widget);
  return widget;
}

static int motReparentWidget(Widget widget, Widget new_parent)
{
  Widget old_parent = XtParent(widget);
  Boolean managed;

  if (XtClass(old_parent) != XtClass(new_parent))
    return 0;
  if (motGetShell(old_parent) != motGetShell(new_parent))
    return 0;
  if (!XtIsComposite(old_parent) || !XtIsComposite(new_parent))
    return 0;
  if (XtIsRealized(widget) && !XtIsRealized(new_parent))
    return 0;

  managed = XtIsManaged(widget);
  if (managed)
    XtUnmanageChild(widget);

  (*((CompositeWidgetClass)XtClass(old_parent))->composite_class.delete_child)(widget);
  widget->core.parent = new_parent;
  (*((CompositeWidgetClass)XtClass(new_parent))->composite_class.insert_child)(widget);

  if (XtIsWidget(widget) && XtIsRealized(widget))
    XReparentWindow(iupmot_display, XtWindow(widget), XtWindow(new_parent), widget->core.x, widget->core.y);

  if (managed)
    XtManageChild(widget);

  return 1;
}

IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  Widget new_parent = iupChildTreeGetNativeParentHandle(ih);
  Widget widget = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget) widget = ih->handle;

  if (XtParent(widget) != new_parent)
  {
    if (motReparentWidget(widget, new_parent))
      return;

    {
      int old_visible = IupGetInt(ih, "VISIBLE");
      if (old_visible)
        IupSetAttribute(ih, "VISIBLE", "NO");

      motSaveAttributesRec(ih);
      IupUnmap(ih);
      IupMap(ih);

      if (old_visible)
        IupSetAttribute(ih, "VISIBLE", "Yes");
    }
  }
}

IUP_DRV_API void iupmotSetPosition(Widget widget, int x, int y)
{
  /* avoid setting both at the same time,
     so if one is invalid all will fail */
  XtVaSetValues(widget,
    XmNx, (XtArgVal)x,
    NULL);
  XtVaSetValues(widget,
    XmNy, (XtArgVal)y,
    NULL);

  /* to position outside parent area */
  {
    Position new_x, new_y;
    XtVaGetValues(widget,
      XmNx, &new_x,
      XmNy, &new_y,
      NULL);
    if (x!=new_x || y!=new_y)
      XtMoveWidget(widget, x, y);
  }
}

IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle* ih)
{
  Widget widget = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget) widget = ih->handle;

  if (ih->currentwidth > 0 && ih->currentheight > 0)
  {
    XtConfigureWidget(widget, (Position)ih->x, (Position)ih->y, (Dimension)ih->currentwidth, (Dimension)ih->currentheight, widget->core.border_width);
  }
  else
    iupmotSetPosition(widget, ih->x, ih->y);
}

IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  Widget widget = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget) widget = ih->handle;

  iupmotDestroyDragDrop(ih);

  XtUnrealizeWidget(widget); /* To match the call to XtRealizeWidget */
  XtDestroyWidget(widget);   /* To match the call to XtCreateManagedWidget */

  iupAttribSet(ih, "_IUPMOT_FONTLIST", NULL);
}

IUP_SDK_API void iupdrvPostRedraw(Ihandle* ih)
{
  XExposeEvent evt;
  Dimension w, h;

  XtVaGetValues(ih->handle, XmNwidth, &w, XmNheight, &h, NULL);

  evt.type = Expose;
  evt.display = iupmot_display;
  evt.send_event = True;
  evt.window = XtWindow(ih->handle);

  evt.x = 0;
  evt.y = 0;
  evt.width = w;
  evt.height = h;

  evt.count = 0;

  /* POST a Redraw */
  XSendEvent(iupmot_display, XtWindow(ih->handle), False, ExposureMask, (XEvent*)&evt);
}

IUP_SDK_API void iupdrvRedrawNow(Ihandle* ih)
{
  Widget w;

  /* POST a Redraw */
  iupdrvPostRedraw(ih);

  /* if this element has an inner native parent (like IupTabs),
     then redraw that native parent if different from the element. */
  w = (Widget)iupClassObjectGetInnerNativeContainerHandle(ih, (Ihandle*)IupGetAttribute(ih, "VALUE_HANDLE"));
  if (w && w != ih->handle)
  {
    Widget handle = ih->handle;
    ih->handle = w;
    iupdrvPostRedraw(ih);
    ih->handle = handle;
  }

  /* flush exposure events. */
  XmUpdateDisplay(ih->handle);
}

IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int* x, int* y)
{
  Window child;
  XTranslateCoordinates(iupmot_display, RootWindow(iupmot_display, iupmot_screen),
                                        XtWindow(ih->handle),
                                        *x, *y, x, y, &child);
}

IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int* x, int* y)
{
  Window child;
  XTranslateCoordinates(iupmot_display, XtWindow(ih->handle),
                                        RootWindow(iupmot_display, iupmot_screen),
                                        *x, *y, x, y, &child);
}

IUP_DRV_API void iupmotHelpCallback(Widget w, Ihandle* ih, XtPointer call_data)
{
  Icallback cb = IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
    IupExitLoop();

  (void)call_data;
  (void)w;
}

IUP_DRV_API void iupmotEnterLeaveWindowEvent(Widget w, Ihandle* ih, XEvent* evt, Boolean* cont)
{
  Icallback cb = NULL;
  (void)cont;
  (void)w;

  if (evt->type == EnterNotify)
  {
    iupmotTipEnterNotify(ih);

    cb = IupGetCallback(ih, "ENTERWINDOW_CB");
  }
  else  if (evt->type == LeaveNotify)
  {
    iupmotTipLeaveNotify();

    cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
  }

  if (cb)
    cb(ih);
}

IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  if (iupdrvIsVisible(ih))
  {
    Widget widget = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
    if (!widget) widget = ih->handle;

    if (iupStrEqualNoCase(value, "TOP"))
      XRaiseWindow(iupmot_display, XtWindow(widget));
    else
      XLowerWindow(iupmot_display, XtWindow(widget));
  }

  return 0;
}

IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int enable)
{
  Widget widget = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget) widget = ih->handle;

  if (enable)
  {
    XtMapWidget(widget);
    iupAttribSet(ih, "_IUPMOT_UNMAPPED", NULL);
  }
  else
  {
    XtUnmapWidget(widget);
    iupAttribSet(ih, "_IUPMOT_UNMAPPED", "1");
  }
}

IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  if (XtIsShell(ih->handle))
  {
    XWindowAttributes wa;
    XGetWindowAttributes(iupmot_display, XtWindow(ih->handle), &wa);
    return (wa.map_state == IsViewable);
  }

  return XtIsManaged(ih->handle) && !iupAttribGet(ih, "_IUPMOT_UNMAPPED");
}

IUP_SDK_API int iupdrvIsActive(Ihandle* ih)
{
  return XtIsSensitive(ih->handle);
}

IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  Widget widget = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget) widget = ih->handle;

  XtSetSensitive(widget, enable);
}

IUP_DRV_API char* iupmotGetXWindowAttrib(Ihandle* ih)
{
  return (char*)XtWindow(ih->handle);
}

IUP_DRV_API void iupmotSetBgColor(Widget w, Pixel color)
{
  Pixel fgcolor = (Pixel)-1;
  XtVaGetValues(w, XmNforeground, &fgcolor, NULL);

  XmChangeColor(w, color);

  /* XmChangeColor also sets the XmNforeground color, so we must reset to the previous one. */
  XtVaSetValues(w, XmNforeground, fgcolor, NULL);
  XtVaSetValues(w, XmNbackgroundPixmap, XmUNSPECIFIED_PIXMAP, NULL);
}

IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Pixel color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
    iupmotSetBgColor(ih->handle, color);
  return 1;
}

IUP_DRV_API char* iupmotGetBgColorAttrib(Ihandle* ih)
{
  unsigned char r, g, b;
  Pixel color;
  XtVaGetValues(ih->handle, XmNbackground, &color, NULL);
  iupmotColorGetRGB(color, &r, &g, &b);
  return iupStrReturnStrf("%d %d %d", (int)r, (int)g, (int)b);
}

IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  Pixel color = iupmotColorGetPixelStr(value);
  if (color != (Pixel)-1)
    XtVaSetValues(ih->handle, XmNforeground, color, NULL);
  return 1;
}

IUP_DRV_API void iupmotGetWindowSize(Ihandle* ih, int* width, int* height)
{
  Dimension w, h;
  XtVaGetValues(ih->handle, XmNwidth, &w, XmNheight, &h, NULL);
  *width = w;
  *height = h;
}

static Cursor motEmptyCursor(Ihandle* ih)
{
  /* creates an empty cursor */
  XColor cursor_color = {0L,0,0,0,0,0};
  char bitsnull[1] = {0x00};
  Pixmap pixmapnull;
  Cursor cur;

  pixmapnull = XCreateBitmapFromData(iupmot_display,
    XtWindow(ih->handle),
    bitsnull,
    1,1);

  cur = XCreatePixmapCursor(iupmot_display,
    pixmapnull,
    pixmapnull,
    &cursor_color,
    &cursor_color,
    0,0);

  XFreePixmap(iupmot_display, pixmapnull);

  return cur;
}

static Cursor motGetCursor(Ihandle* ih, const char* name)
{
  static struct {
    const char* iupname;
    const char* xcursorname;
    int         sysname;
  } table[] = {
    { "NONE",           NULL,                  0},
    { "NULL",           NULL,                  0},
    { "ARROW",          "left_ptr",            XC_left_ptr},
    { "BUSY",           "watch",               XC_watch},
    { "CROSS",          "crosshair",           XC_crosshair},
    { "HAND",           "hand2",               XC_hand2},
    { "HELP",           "question_arrow",      XC_question_arrow},
    { "IUP",            "question_arrow",      XC_question_arrow},
    { "MOVE",           "fleur",               XC_fleur},
    { "PEN",            "pencil",              XC_pencil},
    { "RESIZE_N",       "top_side",            XC_top_side},
    { "RESIZE_S",       "bottom_side",         XC_bottom_side},
    { "RESIZE_NS",      "sb_v_double_arrow",   XC_sb_v_double_arrow},
    { "SPLITTER_HORIZ", "sb_v_double_arrow",   XC_sb_v_double_arrow},
    { "RESIZE_W",       "left_side",           XC_left_side},
    { "RESIZE_E",       "right_side",          XC_right_side},
    { "RESIZE_WE",      "sb_h_double_arrow",   XC_sb_h_double_arrow},
    { "SPLITTER_VERT",  "sb_h_double_arrow",   XC_sb_h_double_arrow},
    { "RESIZE_NE",      "top_right_corner",    XC_top_right_corner},
    { "RESIZE_SE",      "bottom_right_corner", XC_bottom_right_corner},
    { "RESIZE_NW",      "top_left_corner",     XC_top_left_corner},
    { "RESIZE_SW",      "bottom_left_corner",  XC_bottom_left_corner},
    { "TEXT",           "xterm",               XC_xterm},
    { "UPARROW",        "center_ptr",          XC_center_ptr}
  };

  Cursor cur;
  char str[200];
  int i, count = sizeof(table)/sizeof(table[0]);

  /* check the cursor cache first (per control)*/
  snprintf(str, sizeof(str), "_IUPMOT_CURSOR_%s", name);
  cur = (Cursor)iupAttribGet(ih, str);
  if (cur)
    return cur;

  /* check the pre-defined IUP names first */
  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, table[i].iupname))
    {
      if (table[i].sysname)
      {
#if defined(XM_UTF8)
        Screen* screen = ScreenOfDisplay(iupmot_display, iupmot_screen);
        cur = XmeLoadCursor(iupmot_display, screen, table[i].xcursorname);
        if (!cur)
#endif
          cur = XCreateFontCursor(iupmot_display, table[i].sysname);
      }
      else
        cur = motEmptyCursor(ih);

      break;
    }
  }

  if (i == count)
  {
    /* check for a name defined cursor */
    cur = (Cursor)iupImageGetCursor(name);
  }

  iupAttribSet(ih, str, (char*)cur);
  return cur;
}

IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  Cursor cur = motGetCursor(ih, value);
  if (cur)
  {
    XDefineCursor(iupmot_display, XtWindow(ih->handle), cur);
    return 1;
  }
  return 0;
}

IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "XMFONTLIST", iupmotGetFontListAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "XFONTSTRUCT", iupmotGetFontStructAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "XFONTID", iupmotGetFontIdAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
}

IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPMARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIPICON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACCESSIBLETITLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACCESSIBLEDESCRIPTION", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
}

IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  return 15;
}

IUP_DRV_API void iupmotSetPixmap(Ihandle* ih, const char* name, const char* prop, int make_inactive)
{
  if (name)
  {
    Pixmap old_pixmap;
    Pixmap pixmap = (Pixmap)iupImageGetImage(name, ih, make_inactive, NULL);
    if (!pixmap)
      pixmap = XmUNSPECIFIED_PIXMAP;
    XtVaGetValues(ih->handle, prop, &old_pixmap, NULL);
    if (pixmap != old_pixmap)
      XtVaSetValues(ih->handle, prop, pixmap, NULL);
    return;
  }

  /* if not defined */
  XtVaSetValues(ih->handle, prop, XmUNSPECIFIED_PIXMAP, NULL);
}

IUP_DRV_API void iupmotButtonPressReleaseEvent(Widget w, Ihandle* ih, XEvent* evt, Boolean* cont)
{
  IFniiiis cb;

  XButtonEvent* but_evt = (XButtonEvent*)evt;
  if (but_evt->button!=Button1 &&
      but_evt->button!=Button2 &&
      but_evt->button!=Button3 &&
      but_evt->button!=Button4 &&
      but_evt->button!=Button5)
    return;

  cb = (IFniiiis) IupGetCallback(ih,"BUTTON_CB");
  if (cb)
  {
    int ret, doubleclick = 0;
    int b = IUP_BUTTON1+(but_evt->button-1);
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

    /* Double/Single Click */
    if (but_evt->type==ButtonPress)
    {
      static Time last = 0;
      unsigned long elapsed = but_evt->time - last;
      last = but_evt->time;
      if ((int)elapsed <= XtGetMultiClickTime(iupmot_display))
        doubleclick = 1;
    }

    iupmotButtonKeySetStatus(but_evt->state, but_evt->button, status, doubleclick);

    ret = cb(ih, b, (but_evt->type==ButtonPress), but_evt->x, but_evt->y, status);
    if (ret==IUP_CLOSE)
      IupExitLoop();
    else if (ret==IUP_IGNORE)
      *cont=False;
  }

  (void)w;
}

IUP_DRV_API void iupmotDummyPointerMotionEvent(Widget w, XtPointer* data, XEvent* evt, Boolean* cont)
{
  /* Used only when global callbacks are enabled */
  (void)w;
  (void)data;
  (void)evt;
  (void)cont;
}

IUP_DRV_API void iupmotPointerMotionEvent(Widget w, Ihandle* ih, XEvent* evt, Boolean* cont)
{
  IFniis cb = (IFniis)IupGetCallback(ih,"MOTION_CB");
  if (cb)
  {
    XMotionEvent* motion_evt = (XMotionEvent*)evt;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupmotButtonKeySetStatus(motion_evt->state, 0, status, 0);
    cb(ih, motion_evt->x, motion_evt->y, status);
  }

  (void)w;
  (void)cont;
}

IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  Window focus;
  int revert_to;
  XKeyEvent evt;
  memset(&evt, 0, sizeof(XKeyEvent));
  evt.display = iupmot_display;
  evt.send_event = True;
  evt.root = DefaultRootWindow(iupmot_display);

  XGetInputFocus(iupmot_display, &focus, &revert_to);
  evt.window = focus;

  iupdrvKeyEncode(key, &evt.keycode, &evt.state);
  if (!evt.keycode)
    return;

  if (press & 0x01)
  {
    evt.type = KeyPress;
    XSendEvent(iupmot_display, (Window)InputFocus, False, KeyPressMask, (XEvent*)&evt);
  }

  if (press & 0x02)
  {
    evt.type = KeyRelease;
    XSendEvent(iupmot_display, (Window)InputFocus, False, KeyReleaseMask, (XEvent*)&evt);
  }
}

IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
  /* VirtualBox does not reproduce the mouse move visually, but it is working. */
  XWarpPointer(iupmot_display,None,RootWindow(iupmot_display, iupmot_screen),0,0,0,0,x,y);
}

static Window mot_send_mouse_window = 0;

static Window motSendMousePointerWindow(void)
{
  Window root, window, child;
  int x_root, y_root, x, y;
  unsigned int state;

  XQueryPointer(iupmot_display, RootWindow(iupmot_display, DefaultScreen(iupmot_display)),
                &root, &child, &x_root, &y_root, &x, &y, &state);

  window = child;
  while (child)
  {
    window = child;
    XQueryPointer(iupmot_display, window, &root, &child, &x_root, &y_root, &x, &y, &state);
  }
  return window;
}

static void motSendMouseEvent(int type, Window window, int x, int y, unsigned int state, unsigned int button)
{
  XEvent evt;
  Window child;
  int wx = 0, wy = 0;

  if (!XtWindowToWidget(iupmot_display, window))
    return;

  XTranslateCoordinates(iupmot_display, DefaultRootWindow(iupmot_display), window, x, y, &wx, &wy, &child);

  memset(&evt, 0, sizeof(XEvent));
  if (type == MotionNotify)
  {
    evt.xmotion.type = MotionNotify;
    evt.xmotion.display = iupmot_display;
    evt.xmotion.send_event = True;
    evt.xmotion.window = window;
    evt.xmotion.root = DefaultRootWindow(iupmot_display);
    evt.xmotion.x = wx;
    evt.xmotion.y = wy;
    evt.xmotion.x_root = x;
    evt.xmotion.y_root = y;
    evt.xmotion.state = state;
    evt.xmotion.same_screen = True;
    XSendEvent(iupmot_display, window, False, PointerMotionMask | ButtonMotionMask | Button1MotionMask | Button2MotionMask |
               Button3MotionMask | Button4MotionMask | Button5MotionMask, &evt);
  }
  else
  {
    evt.xbutton.type = type;
    evt.xbutton.display = iupmot_display;
    evt.xbutton.send_event = True;
    evt.xbutton.window = window;
    evt.xbutton.root = DefaultRootWindow(iupmot_display);
    evt.xbutton.x = wx;
    evt.xbutton.y = wy;
    evt.xbutton.x_root = x;
    evt.xbutton.y_root = y;
    evt.xbutton.state = state;
    evt.xbutton.button = button;
    evt.xbutton.same_screen = True;
    evt.xbutton.time = XtLastTimestampProcessed(iupmot_display);
    XSendEvent(iupmot_display, window, False, (type == ButtonRelease) ? ButtonReleaseMask : ButtonPressMask, &evt);
  }
}

IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  unsigned int button = (bt >= IUP_BUTTON1 && bt <= IUP_BUTTON5) ? (unsigned int)(bt - IUP_BUTTON1 + Button1) : 0;
  unsigned int mask = button ? (unsigned int)(Button1Mask << (button - Button1)) : 0;

  iupdrvWarpPointer(x, y);

  if (status == -1)
  {
    if (button && mot_send_mouse_window)
      motSendMouseEvent(MotionNotify, mot_send_mouse_window, x, y, mask, 0);
    return;
  }

  if (!button)
    return;

  if (status == 0)
  {
    Window window = mot_send_mouse_window ? mot_send_mouse_window : motSendMousePointerWindow();
    if (window)
      motSendMouseEvent(ButtonRelease, window, x, y, mask, button);
    mot_send_mouse_window = 0;
  }
  else
  {
    Window window = motSendMousePointerWindow();
    if (!window)
      return;

    mot_send_mouse_window = window;
    motSendMouseEvent(ButtonPress, window, x, y, 0, button);
  }
}

#ifndef WIN32
#include <unistd.h>
#include <time.h>
IUP_SDK_API void iupdrvSleep(int time)
{
  usleep(time*1000);  /* milli to micro */
}

IUP_SDK_API unsigned int iupdrvGetTickCount(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (unsigned int)((unsigned long long)ts.tv_sec * 1000 + (unsigned long long)ts.tv_nsec / 1000000);
}
#else
IUP_SDK_API void iupdrvSleep(int time)
{
  clock_t goal = (clock_t)(time*CLOCKS_PER_SEC)/1000 + clock();
  while(goal > clock());
}

IUP_SDK_API unsigned int iupdrvGetTickCount(void)
{
  return (unsigned int)(((unsigned long long)clock() * 1000) / CLOCKS_PER_SEC);
}
#endif

IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle* ih, const char* title)
{
  (void)title;
  (void)ih;
}

IUP_SDK_API void iupdrvSetAccessibleDescription(Ihandle* ih, const char* description)
{
  (void)description;
  (void)ih;
}

IUP_SDK_API int iupdrvIsSystemDarkMode(void)
{
  unsigned char bg_r, bg_g, bg_b;
  unsigned char fg_r, fg_g, fg_b;
  double bg_lum, fg_lum;
  XColor xcolor;
  Colormap colormap;
  XrmDatabase db;
  XrmValue value;
  char* type = NULL;
  char* bg_str = NULL;
  char* fg_str = NULL;

  db = XrmGetDatabase(iupmot_display);
  if (!db)
    return 0;

  if (XrmGetResource(db, "*background", "*Background", &type, &value))
    bg_str = value.addr;
  if (XrmGetResource(db, "*foreground", "*Foreground", &type, &value))
    fg_str = value.addr;

  if (!bg_str || !fg_str)
    return 0;

  colormap = DefaultColormap(iupmot_display, iupmot_screen);

  if (!XParseColor(iupmot_display, colormap, bg_str, &xcolor))
    return 0;

  bg_r = xcolor.red >> 8;
  bg_g = xcolor.green >> 8;
  bg_b = xcolor.blue >> 8;

  if (!XParseColor(iupmot_display, colormap, fg_str, &xcolor))
    return 0;

  fg_r = xcolor.red >> 8;
  fg_g = xcolor.green >> 8;
  fg_b = xcolor.blue >> 8;

  /* ITU-R BT.709 relative luminance */
  bg_lum = 0.2126 * bg_r + 0.7152 * bg_g + 0.0722 * bg_b;
  fg_lum = 0.2126 * fg_r + 0.7152 * fg_g + 0.0722 * fg_b;

  return (bg_lum < fg_lum) ? 1 : 0;
}

IUP_DRV_API void iupmotScrolledWindowWheelEvent(Widget w, Ihandle* ih, XEvent* evt, Boolean* cont)
{
  XButtonEvent* but_evt = (XButtonEvent*)evt;
  Widget sb_win, sb;
  int value, slider_size, maximum, increment;

  if (evt->type != ButtonPress)
    return;
  if (but_evt->button != Button4 && but_evt->button != Button5)
    return;

  sb_win = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!sb_win)
    return;

  XtVaGetValues(sb_win, XmNverticalScrollBar, &sb, NULL);
  if (!sb)
    return;

  XtVaGetValues(sb, XmNvalue, &value, XmNsliderSize, &slider_size, XmNmaximum, &maximum, XmNincrement, &increment, NULL);

  if (but_evt->button == Button4)
    value -= increment * 3;
  else
    value += increment * 3;

  if (value < 0)
    value = 0;
  if (value > maximum - slider_size)
    value = maximum - slider_size;

  XmScrollBarSetValues(sb, value, slider_size, increment, increment * 3, True);

  (void)w;
  (void)cont;
}
