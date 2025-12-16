/** \file
 * \brief Motif System Tray (XEmbed)
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_class.h"
#include "iup_tray.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"


#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define MOT_TRAY_EVENT_MASK (ExposureMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask)

typedef struct _IupmotTray {
  Window window;
  Pixmap icon_pixmap;
  Pixmap icon_mask;
  GC gc;
  int width;
  int height;
  int window_width;
  int window_height;
  Atom selection_atom;
  Window tray_window;
  int visible;
  struct timeval last_click_time;
  int last_button;
  Pixmap original_icon;
  Pixmap original_mask;
  int original_width;
  int original_height;
} IupmotTray;

typedef struct _IupmotTrayCallbackData {
  Ihandle* ih;
  int button;
  int pressed;
  int dclick;
  int x;
  int y;
} IupmotTrayCallbackData;

static int motTrayDoubleClick(IupmotTray* tray, int button)
{
  struct timeval now;
  long diff_ms;

  gettimeofday(&now, NULL);

  if (tray->last_button == -1 || tray->last_button != button)
  {
    tray->last_button = button;
    tray->last_click_time = now;
    return 0;
  }

  diff_ms = (now.tv_sec - tray->last_click_time.tv_sec) * 1000 +
            (now.tv_usec - tray->last_click_time.tv_usec) / 1000;

  if (diff_ms < 400)
  {
    tray->last_button = -1;
    return 1;
  }

  tray->last_click_time = now;
  return 0;
}

static Window motTrayGetManagerWindow(Atom selection_atom)
{
  return XGetSelectionOwner(iupmot_display, selection_atom);
}

static void motTraySendDockRequest(IupmotTray* tray)
{
  XClientMessageEvent ev;

  memset(&ev, 0, sizeof(ev));
  ev.type = ClientMessage;
  ev.window = tray->tray_window;
  ev.message_type = XInternAtom(iupmot_display, "_NET_SYSTEM_TRAY_OPCODE", False);
  ev.format = 32;
  ev.data.l[0] = CurrentTime;
  ev.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
  ev.data.l[2] = tray->window;
  ev.data.l[3] = 0;
  ev.data.l[4] = 0;

  XSendEvent(iupmot_display, tray->tray_window, False, NoEventMask, (XEvent *)&ev);
  XSync(iupmot_display, False);
}

static void motTrayScaleIcon(IupmotTray* tray)
{
  if (!tray->original_icon || tray->window_width <= 0 || tray->window_height <= 0)
    return;

  if (tray->icon_pixmap && tray->icon_pixmap != None)
  {
    XFreePixmap(iupmot_display, tray->icon_pixmap);
    tray->icon_pixmap = None;
  }
  if (tray->icon_mask && tray->icon_mask != None)
  {
    XFreePixmap(iupmot_display, tray->icon_mask);
    tray->icon_mask = None;
  }

  tray->width = tray->window_width;
  tray->height = tray->window_height;

  if (tray->original_width == tray->width && tray->original_height == tray->height)
  {
    tray->icon_pixmap = XCreatePixmap(iupmot_display, RootWindow(iupmot_display, iupmot_screen),
                                      tray->width, tray->height,
                                      DefaultDepth(iupmot_display, iupmot_screen));
    XCopyArea(iupmot_display, tray->original_icon, tray->icon_pixmap, tray->gc,
              0, 0, tray->width, tray->height, 0, 0);

    if (tray->original_mask && tray->original_mask != None)
    {
      tray->icon_mask = XCreatePixmap(iupmot_display, RootWindow(iupmot_display, iupmot_screen),
                                      tray->width, tray->height, 1);
      GC mask_gc = XCreateGC(iupmot_display, tray->icon_mask, 0, NULL);
      XCopyArea(iupmot_display, tray->original_mask, tray->icon_mask, mask_gc,
                0, 0, tray->width, tray->height, 0, 0);
      XFreeGC(iupmot_display, mask_gc);
    }
    return;
  }

  tray->icon_pixmap = XCreatePixmap(iupmot_display, RootWindow(iupmot_display, iupmot_screen),
                                    tray->width, tray->height, DefaultDepth(iupmot_display, iupmot_screen));

  {
    XGCValues gc_values;
    GC temp_gc;
    XImage* src_image;
    XImage* dst_image;
    int x, y;
    double x_ratio = (double)tray->original_width / tray->width;
    double y_ratio = (double)tray->original_height / tray->height;

    gc_values.graphics_exposures = False;
    temp_gc = XCreateGC(iupmot_display, tray->icon_pixmap, GCGraphicsExposures, &gc_values);

    src_image = XGetImage(iupmot_display, tray->original_icon, 0, 0,
                          tray->original_width, tray->original_height, AllPlanes, ZPixmap);
    dst_image = XGetImage(iupmot_display, tray->icon_pixmap, 0, 0,
                          tray->width, tray->height, AllPlanes, ZPixmap);

    if (src_image && dst_image)
    {
      for (y = 0; y < tray->height; y++)
      {
        for (x = 0; x < tray->width; x++)
        {
          int src_x = (int)(x * x_ratio);
          int src_y = (int)(y * y_ratio);
          unsigned long pixel = XGetPixel(src_image, src_x, src_y);
          XPutPixel(dst_image, x, y, pixel);
        }
      }
      XPutImage(iupmot_display, tray->icon_pixmap, temp_gc, dst_image, 0, 0, 0, 0, tray->width, tray->height);
    }

    if (src_image) XDestroyImage(src_image);
    if (dst_image) XDestroyImage(dst_image);
    XFreeGC(iupmot_display, temp_gc);
  }

  if (tray->original_mask && tray->original_mask != None)
  {
    XImage* src_mask_image;
    XImage* dst_mask_image;
    int x, y;
    double x_ratio = (double)tray->original_width / tray->width;
    double y_ratio = (double)tray->original_height / tray->height;
    GC mask_gc;

    tray->icon_mask = XCreatePixmap(iupmot_display, RootWindow(iupmot_display, iupmot_screen),
                                    tray->width, tray->height, 1);
    mask_gc = XCreateGC(iupmot_display, tray->icon_mask, 0, NULL);

    src_mask_image = XGetImage(iupmot_display, tray->original_mask, 0, 0,
                               tray->original_width, tray->original_height, AllPlanes, ZPixmap);
    dst_mask_image = XGetImage(iupmot_display, tray->icon_mask, 0, 0,
                               tray->width, tray->height, AllPlanes, ZPixmap);

    if (src_mask_image && dst_mask_image)
    {
      for (y = 0; y < tray->height; y++)
      {
        for (x = 0; x < tray->width; x++)
        {
          int src_x = (int)(x * x_ratio);
          int src_y = (int)(y * y_ratio);
          unsigned long pixel = XGetPixel(src_mask_image, src_x, src_y);
          XPutPixel(dst_mask_image, x, y, pixel);
        }
      }
      XPutImage(iupmot_display, tray->icon_mask, mask_gc, dst_mask_image, 0, 0, 0, 0, tray->width, tray->height);
    }

    if (src_mask_image) XDestroyImage(src_mask_image);
    if (dst_mask_image) XDestroyImage(dst_mask_image);
    XFreeGC(iupmot_display, mask_gc);
  }
}

static void motTrayHandleExpose(Ihandle* ih, IupmotTray* tray, XExposeEvent* event)
{
  if (tray->icon_pixmap)
  {
    XCopyArea(iupmot_display, tray->icon_pixmap, tray->window, tray->gc, 0, 0, tray->width, tray->height, 0, 0);
  }

  (void)ih;
  (void)event;
}

static void motTrayHandleButtonPress(Ihandle* ih, IupmotTray* tray, XButtonEvent* event)
{
  int button = event->button;
  int pressed = 1;
  int dclick = motTrayDoubleClick(tray, button);
  IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");

  if (cb)
  {
    if (cb(ih, button, pressed, dclick) == IUP_CLOSE)
      IupExitLoop();
  }
}

static void motTrayEventHandler(Widget w, XtPointer client_data, XEvent* event, Boolean* continue_dispatch)
{
  Ihandle* ih = (Ihandle*)client_data;
  IupmotTray* tray;
  static Atom xembed_atom = 0;

  *continue_dispatch = False;

  if (!iupObjectCheck(ih))
    return;

  tray = (IupmotTray*)iupAttribGet(ih, "_IUPMOT_TRAY");
  if (!tray)
    return;

  if (xembed_atom == 0)
  {
    xembed_atom = XInternAtom(iupmot_display, "_XEMBED", False);
  }

  switch (event->type)
  {
    case Expose:
      if (event->xexpose.count == 0)
        motTrayHandleExpose(ih, tray, &event->xexpose);
      break;

    case ButtonPress:
      motTrayHandleButtonPress(ih, tray, &event->xbutton);
      break;

    case ConfigureNotify:
    {
      XWindowAttributes wa;
      XGetWindowAttributes(iupmot_display, tray->window, &wa);
      if (wa.width > 0 && wa.height > 0 &&
          (tray->window_width != wa.width || tray->window_height != wa.height))
      {
        tray->window_width = wa.width;
        tray->window_height = wa.height;
        motTrayScaleIcon(tray);
        if (tray->icon_mask)
          XShapeCombineMask(iupmot_display, tray->window, ShapeBounding, 0, 0, tray->icon_mask, ShapeSet);
      }
      motTrayHandleExpose(ih, tray, NULL);
      break;
    }

    case ClientMessage:
      if (event->xclient.message_type == xembed_atom)
      {
        if (event->xclient.data.l[1] == 0)
        {
          XMapWindow(iupmot_display, tray->window);
          XFlush(iupmot_display);
        }
      }
      break;

    case MapNotify:
    {
      Atom xembed_info_atom = XInternAtom(iupmot_display, "_XEMBED_INFO", False);
      long xembed_info[2] = {0, 1};

      XChangeProperty(iupmot_display, tray->window, xembed_info_atom, xembed_info_atom, 32,
                      PropModeReplace, (unsigned char*)xembed_info, 2);
      break;
    }

    case UnmapNotify:
    {
      Atom xembed_info_atom = XInternAtom(iupmot_display, "_XEMBED_INFO", False);
      long xembed_info[2] = {0, 0};

      XChangeProperty(iupmot_display, tray->window, xembed_info_atom, xembed_info_atom, 32,
                      PropModeReplace, (unsigned char*)xembed_info, 2);
      break;
    }
  }

  (void)w;
}

static IupmotTray* motGetTray(Ihandle* ih)
{
  IupmotTray* tray = (IupmotTray*)iupAttribGet(ih, "_IUPMOT_TRAY");

  if (!tray)
  {
    char selection_name[64];
    Widget tray_shell;

    tray = (IupmotTray*)calloc(1, sizeof(IupmotTray));
    tray->last_button = -1;
    tray->width = 0;
    tray->height = 0;
    tray->window_width = 0;
    tray->window_height = 0;
    tray->original_width = 0;
    tray->original_height = 0;

    snprintf(selection_name, sizeof(selection_name), "_NET_SYSTEM_TRAY_S%d", iupmot_screen);
    tray->selection_atom = XInternAtom(iupmot_display, selection_name, False);

    tray->tray_window = motTrayGetManagerWindow(tray->selection_atom);
    if (tray->tray_window == None)
    {
      free(tray);
      return NULL;
    }

    tray_shell = XtVaAppCreateShell("tray", "Tray",
                                    applicationShellWidgetClass,
                                    iupmot_display,
                                    XmNmappedWhenManaged, False,
                                    XmNcolormap, DefaultColormap(iupmot_display, iupmot_screen),
                                    XmNbackgroundPixmap, ParentRelative,
                                    XmNborderWidth, 0,
                                    NULL);

    if (!tray_shell)
    {
      free(tray);
      return NULL;
    }

    XtAddEventHandler(tray_shell, MOT_TRAY_EVENT_MASK, False, motTrayEventHandler, (XtPointer)ih);

    XtRealizeWidget(tray_shell);

    tray->window = XtWindow(tray_shell);
    if (!tray->window)
    {
      XtDestroyWidget(tray_shell);
      free(tray);
      return NULL;
    }

    {
      XGCValues gc_values;
      gc_values.graphics_exposures = False;
      gc_values.function = GXcopy;
      gc_values.foreground = BlackPixel(iupmot_display, iupmot_screen);
      gc_values.background = WhitePixel(iupmot_display, iupmot_screen);
      tray->gc = XCreateGC(iupmot_display, tray->window,
                          GCGraphicsExposures | GCFunction | GCForeground | GCBackground,
                          &gc_values);
    }

    {
      Atom xembed_info_atom = XInternAtom(iupmot_display, "_XEMBED_INFO", False);
      long xembed_info[2] = {0, 0};

      XChangeProperty(iupmot_display, tray->window, xembed_info_atom, xembed_info_atom, 32,
                      PropModeReplace, (unsigned char*)xembed_info, 2);
    }

    iupAttribSet(ih, "_IUPMOT_TRAY_WIDGET", (char*)tray_shell);
    iupAttribSet(ih, "_IUPMOT_TRAY", (char*)tray);
  }

  return tray;
}

/******************************************************************************/
/* Driver Interface Implementation                                            */
/******************************************************************************/

int iupdrvTraySetVisible(Ihandle* ih, int visible)
{
  if (visible)
  {
    char* image;
    char* tip;
    IupmotTray* tray = motGetTray(ih);
    if (!tray)
      return 0;

    tray->visible = 1;

    image = iupAttribGet(ih, "IMAGE");
    if (image)
      iupdrvTraySetImage(ih, image);

    tip = iupAttribGet(ih, "TIP");
    if (tip)
      iupdrvTraySetTip(ih, tip);

    motTraySendDockRequest(tray);
    XFlush(iupmot_display);
  }
  else
  {
    iupdrvTrayDestroy(ih);
  }

  return 1;
}

int iupdrvTraySetTip(Ihandle* ih, const char* value)
{
  IupmotTray* tray = motGetTray(ih);
  if (!tray)
    return 0;

  if (value)
  {
    Atom net_wm_name = XInternAtom(iupmot_display, "_NET_WM_NAME", False);
    Atom utf8_string = XInternAtom(iupmot_display, "UTF8_STRING", False);
    XChangeProperty(iupmot_display, tray->window, net_wm_name, utf8_string, 8, PropModeReplace, (unsigned char*)value, strlen(value));
  }
  else
  {
    Atom net_wm_name = XInternAtom(iupmot_display, "_NET_WM_NAME", False);
    XDeleteProperty(iupmot_display, tray->window, net_wm_name);
  }

  return 1;
}

int iupdrvTraySetImage(Ihandle* ih, const char* value)
{
  IupmotTray* tray = motGetTray(ih);
  Pixmap icon, mask;
  Window root;
  int x, y;
  unsigned int width, height, border, depth;

  if (!tray)
    return 0;

  if (tray->original_icon && tray->original_icon != None)
  {
    XFreePixmap(iupmot_display, tray->original_icon);
    tray->original_icon = None;
  }
  if (tray->original_mask && tray->original_mask != None)
  {
    XFreePixmap(iupmot_display, tray->original_mask);
    tray->original_mask = None;
  }
  if (tray->icon_pixmap && tray->icon_pixmap != None)
  {
    XFreePixmap(iupmot_display, tray->icon_pixmap);
    tray->icon_pixmap = None;
  }
  if (tray->icon_mask && tray->icon_mask != None)
  {
    XFreePixmap(iupmot_display, tray->icon_mask);
    tray->icon_mask = None;
  }

  icon = (Pixmap)iupImageGetIcon(value);
  mask = iupmotImageGetMask(value);

  if (icon && icon != None)
  {
    if (XGetGeometry(iupmot_display, icon, &root, &x, &y, &width, &height, &border, &depth))
    {
      tray->original_icon = XCreatePixmap(iupmot_display, RootWindow(iupmot_display, iupmot_screen), width, height, depth);

      if (tray->original_icon)
      {
        XGCValues gc_values;
        GC temp_gc;

        gc_values.graphics_exposures = False;
        gc_values.function = GXcopy;
        temp_gc = XCreateGC(iupmot_display, tray->original_icon, GCGraphicsExposures | GCFunction, &gc_values);

        XCopyArea(iupmot_display, icon, tray->original_icon, temp_gc, 0, 0, width, height, 0, 0);

        XFreeGC(iupmot_display, temp_gc);
      }

      if (mask && mask != None)
      {
        tray->original_mask = XCreatePixmap(iupmot_display, RootWindow(iupmot_display, iupmot_screen), width, height, 1);
        if (tray->original_mask)
        {
          GC mask_gc = XCreateGC(iupmot_display, tray->original_mask, 0, NULL);
          XCopyArea(iupmot_display, mask, tray->original_mask, mask_gc, 0, 0, width, height, 0, 0);
          XFreeGC(iupmot_display, mask_gc);
        }
      }

      tray->original_width = width;
      tray->original_height = height;

      motTrayScaleIcon(tray);

      if (tray->icon_mask)
        XShapeCombineMask(iupmot_display, tray->window, ShapeBounding, 0, 0, tray->icon_mask, ShapeSet);
      else
        XShapeCombineMask(iupmot_display, tray->window, ShapeBounding, 0, 0, None, ShapeSet);
    }
  }
  else
  {
    XShapeCombineMask(iupmot_display, tray->window, ShapeBounding, 0, 0, None, ShapeSet);
  }

  if (tray->visible)
  {
    XClearWindow(iupmot_display, tray->window);
    motTrayHandleExpose(ih, tray, NULL);
    XFlush(iupmot_display);
  }

  return 1;
}

int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu)
{
  /* XEmbed tray protocol doesn't support automatic menu popup via MENU attribute.
   * The XEmbed protocol has no mechanism for the tray host to request context menu.
   * Applications should use TRAYCLICK_CB callback to show menu manually via IupPopup(). */
  (void)ih;
  (void)menu;
  return 0;
}

void iupdrvTrayDestroy(Ihandle* ih)
{
  IupmotTray* tray = (IupmotTray*)iupAttribGet(ih, "_IUPMOT_TRAY");

  if (tray)
  {
    Widget tray_shell = (Widget)iupAttribGet(ih, "_IUPMOT_TRAY_WIDGET");

    iupAttribSet(ih, "_IUPMOT_TRAY", NULL);
    iupAttribSet(ih, "_IUPMOT_TRAY_WIDGET", NULL);

    if (tray_shell)
    {
      XtRemoveEventHandler(tray_shell, MOT_TRAY_EVENT_MASK, False, motTrayEventHandler, (XtPointer)ih);

      XSync(iupmot_display, False);

      XtDestroyWidget(tray_shell);
    }

    if (tray->original_icon)
      XFreePixmap(iupmot_display, tray->original_icon);

    if (tray->original_mask)
      XFreePixmap(iupmot_display, tray->original_mask);

    if (tray->icon_pixmap)
      XFreePixmap(iupmot_display, tray->icon_pixmap);

    if (tray->icon_mask)
      XFreePixmap(iupmot_display, tray->icon_mask);

    if (tray->gc)
      XFreeGC(iupmot_display, tray->gc);

    free(tray);
  }
}

int iupdrvTrayIsAvailable(void)
{
  return 1;
}

void iupdrvTrayInitClass(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "MENU", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPBALLOONTITLEICON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TIPDELAY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
