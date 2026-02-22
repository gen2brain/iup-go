/** \file
 * \brief Canvas Control for GTK4
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_canvas.h"
#include "iup_key.h"

#include "iupgtk4_drv.h"


static void gtk4CanvasUpdateChildLayout(Ihandle *ih, int flush)
{
  iupGtk4Fixed* sb_win = (iupGtk4Fixed*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  GtkWidget* sb_horiz = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_SBHORIZ");
  GtkWidget* sb_vert = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_SBVERT");
  int sb_vert_width=0, sb_horiz_height=0;
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int border = iupAttribGetInt(ih, "_IUPGTK4_BORDER");

  if (sb_vert && iupgtk4IsVisible(sb_vert))
    sb_vert_width = iupdrvGetScrollbarSize();
  if (sb_horiz && iupgtk4IsVisible(sb_horiz))
    sb_horiz_height = iupdrvGetScrollbarSize();

  /* Scrollbars need explicit sizing, but canvas should use preferred size for expansion */
  if (sb_vert_width)
    iupgtk4NativeContainerSetBounds(GTK_WIDGET(sb_win), sb_vert,
                                     width-sb_vert_width-border, border,
                                     sb_vert_width, height-sb_horiz_height-2*border);
  if (sb_horiz_height)
    iupgtk4NativeContainerSetBounds(GTK_WIDGET(sb_win), sb_horiz,
                                     border, height-sb_horiz_height-border,
                                     width-sb_vert_width-2*border, sb_horiz_height);

  /* Canvas at (0,0) - border is drawn by container snapshot, not by offsetting canvas */
  iupgtk4FixedMove(sb_win, ih->handle, 0, 0);

  if (flush)
    IupFlush();
}

static int gtk4CanvasScroll2Iup(GtkScrollType scroll, int vert)
{
  switch(scroll)
  {
    case GTK_SCROLL_STEP_UP:
      return IUP_SBUP;
    case GTK_SCROLL_STEP_DOWN:
      return IUP_SBDN;
    case GTK_SCROLL_PAGE_UP:
      return IUP_SBPGUP;
    case GTK_SCROLL_PAGE_DOWN:
      return IUP_SBPGDN;
    case GTK_SCROLL_STEP_LEFT:
      return IUP_SBLEFT;
    case GTK_SCROLL_STEP_RIGHT:
      return IUP_SBRIGHT;
    case GTK_SCROLL_PAGE_LEFT:
      return IUP_SBPGLEFT;
    case GTK_SCROLL_PAGE_RIGHT:
      return IUP_SBPGRIGHT;
    case GTK_SCROLL_STEP_BACKWARD:
      return vert? IUP_SBUP: IUP_SBLEFT;
    case GTK_SCROLL_STEP_FORWARD:
      return vert? IUP_SBDN: IUP_SBRIGHT;
    case GTK_SCROLL_PAGE_BACKWARD:
      return vert? IUP_SBPGUP: IUP_SBPGLEFT;
    case GTK_SCROLL_PAGE_FORWARD:
      return vert? IUP_SBPGDN: IUP_SBPGRIGHT;
    case GTK_SCROLL_JUMP:
    case GTK_SCROLL_START:
    case GTK_SCROLL_END:
      return vert? IUP_SBPOSV: IUP_SBPOSH;
    case GTK_SCROLL_NONE:
      return -1;
  }

  return -1;
}

static gboolean gtk4CanvasScrollHorizChangeValue(GtkRange *range, GtkScrollType scroll, double value, Ihandle *ih)
{
  IFniff cb = (IFniff)IupGetCallback(ih,"SCROLL_CB");
  int op = gtk4CanvasScroll2Iup(scroll, 0);
  double posx, posy;

  if (op == -1)
    return FALSE;

  if (iupAttribGet(ih, "_IUPGTK4_SETSBPOS"))
    return FALSE;

  posx = value;
  ih->data->posx = (float)posx;
  posy = ih->data->posy;

  if (cb)
    cb(ih, op, (float)posx, (float)posy);
  else
  {
    IFn cb2 = (IFn)IupGetCallback(ih,"ACTION");
    if (cb2)
      iupdrvRedrawNow(ih);
  }

  (void)range;
  return FALSE;
}

static gboolean gtk4CanvasScrollVertChangeValue(GtkRange *range, GtkScrollType scroll, double value, Ihandle *ih)
{
  IFniff cb = (IFniff)IupGetCallback(ih,"SCROLL_CB");
  int op = gtk4CanvasScroll2Iup(scroll, 1);
  double posx, posy;

  if (op == -1)
    return FALSE;

  if (iupAttribGet(ih, "_IUPGTK4_SETSBPOS"))
    return FALSE;

  posy = value;
  ih->data->posy = (float)posy;
  posx = ih->data->posx;

  if (cb)
    cb(ih, op, (float)posx, (float)posy);
  else
  {
    IFn cb2 = (IFn)IupGetCallback(ih,"ACTION");
    if (cb2)
      iupdrvRedrawNow(ih);
  }

  (void)range;
  return FALSE;
}

static void gtk4CanvasAdjustmentSetValue(Ihandle* ih, GtkAdjustment* adjustment, double value)
{
  iupAttribSet(ih, "_IUPGTK4_SETSBPOS", "1");
  gtk_adjustment_set_value(adjustment, value);
  iupAttribSet(ih, "_IUPGTK4_SETSBPOS", NULL);
}

static void gtk4CanvasAdjustHorizValueChanged(GtkAdjustment* adjustment, Ihandle* ih)
{
  double posx, posy;
  IFniff cb;

  if (iupAttribGet(ih, "_IUPGTK4_SETSBPOS"))
    return;

  cb = (IFniff)IupGetCallback(ih,"SCROLL_CB");
  posx = gtk_adjustment_get_value(adjustment);
  ih->data->posx = (float)posx;
  posy = ih->data->posy;

  if (cb)
  {
    cb(ih, IUP_SBPOSH, (float)posx, (float)posy);
    /* SCROLL_CB uses IupDraw* which calls iupdrvDrawFlush,
     * which already calls gtk_widget_queue_draw to trigger ACTION callback */
  }
  else
  {
    IFn cb2 = (IFn)IupGetCallback(ih,"ACTION");
    if (cb2)
      iupdrvRedrawNow(ih);
  }
}

static void gtk4CanvasAdjustVertValueChanged(GtkAdjustment* adjustment, Ihandle* ih)
{
  double posx, posy;
  IFniff cb;

  if (iupAttribGet(ih, "_IUPGTK4_SETSBPOS"))
    return;

  cb = (IFniff)IupGetCallback(ih,"SCROLL_CB");
  posy = gtk_adjustment_get_value(adjustment);
  ih->data->posy = (float)posy;
  posx = ih->data->posx;

  if (cb)
  {
    cb(ih, IUP_SBPOSV, (float)posx, (float)posy);
    /* SCROLL_CB uses IupDraw* which calls iupdrvDrawFlush,
     * which already calls gtk_widget_queue_draw to trigger ACTION callback */
  }
  else
  {
    IFn cb2 = (IFn)IupGetCallback(ih,"ACTION");
    if (cb2)
      iupdrvRedrawNow(ih);
  }
}

static gboolean gtk4CanvasScrollEvent(GtkEventControllerScroll *controller, double dx, double dy, Ihandle *ih)
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
    int delta = dy < 0 ? 1 : -1;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    int button = dy < 0 ? 4 : 5;
    GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(controller));
    iupgtk4ButtonKeySetStatus(state, button, status, 0);

    GdkEvent *event = gtk_event_controller_get_current_event(GTK_EVENT_CONTROLLER(controller));
    double x, y;
    gdk_event_get_position(event, &x, &y);

    wcb(ih, (float)delta, (int)x, (int)y, status);
  }
  else
  {
    IFniff scb = (IFniff)IupGetCallback(ih,"SCROLL_CB");

    if (dy != 0)
    {
      int delta = dy < 0 ? 1 : -1;
      double posy = ih->data->posy;
      posy -= delta*iupAttribGetDouble(ih, "DY")/10.0;
      IupSetDouble(ih, "POSY", posy);

      if (scb)
      {
        int op = dy < 0 ? IUP_SBUP : IUP_SBDN;
        scb(ih, op, (float)ih->data->posx, (float)ih->data->posy);
      }
    }
    else if (dx != 0)
    {
      int delta = dx < 0 ? 1 : -1;
      double posx = ih->data->posx;
      posx -= delta*iupAttribGetDouble(ih, "DX")/10.0;
      IupSetDouble(ih, "POSX", posx);

      if (scb)
      {
        int op = dx < 0 ? IUP_SBLEFT : IUP_SBRIGHT;
        scb(ih, op, (float)ih->data->posx, (float)ih->data->posy);
      }
    }
  }

  (void)controller;
  return TRUE;
}

static void gtk4CanvasButtonPressed(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle *ih)
{
  GtkWidget* widget = ih->handle;

  if (n_press == 1)
  {
    gtk_widget_grab_focus(widget);
  }

  (void)gesture;
  (void)x;
  (void)y;
}

static void gtk4CanvasButtonReleased(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle *ih)
{
  (void)gesture;
  (void)n_press;
  (void)x;
  (void)y;
  (void)ih;
}

gboolean gtk4CanvasFocusLeave(GtkEventControllerFocus *controller, Ihandle *ih)
{
  (void)controller;
  (void)ih;
  return FALSE;
}

static int gtk4CanvasSetBgColorAttrib(Ihandle* ih, const char* value);

static void gtk4CanvasDraw(GtkDrawingArea *area, cairo_t* cr, int width, int height, Ihandle *ih)
{
  IFn cb = (IFn)IupGetCallback(ih,"ACTION");
  cairo_surface_t* buffer;

  /* Check if there's a persistent buffer from SCROLL_CB or other drawing outside ACTION */
  buffer = (cairo_surface_t*)iupAttribGet(ih, "_IUPGTK4_CANVAS_BUFFER");

  /* If buffer exists and is valid, use it instead of calling ACTION
   * This happens when SCROLL_CB drew to the buffer */
  if (buffer && cairo_image_surface_get_width(buffer) == width &&
      cairo_image_surface_get_height(buffer) == height)
  {
    cairo_set_source_surface(cr, buffer, 0, 0);
    cairo_paint(cr);
    return;
  }

  if (cb && !(ih->data->inside_resize))
  {

    double x1, y1, x2, y2;
    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
    iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", (int)x1, (int)y1, (int)x2-1, (int)y2-1);
    iupAttribSet(ih, "CAIRO_CR", (char*)cr);

    /* Store width/height so IupDraw can use them without triggering CSS recalculation */
    iupAttribSetInt(ih, "_IUPGTK4_DRAW_WIDTH", width);
    iupAttribSetInt(ih, "_IUPGTK4_DRAW_HEIGHT", height);

    cb(ih);

    iupAttribSet(ih, "CLIPRECT", NULL);
    iupAttribSet(ih, "CAIRO_CR", NULL);

    /* Clear buffer reference so next draw calls ACTION again
     * The buffer is only meant to persist for SCROLL_CB drawings done outside ACTION
     * Note: We only clear the reference, not destroy the surface - IupDraw manages lifecycle */
    buffer = (cairo_surface_t*)iupAttribGet(ih, "_IUPGTK4_CANVAS_BUFFER");
    if (buffer)
    {
      cairo_surface_destroy(buffer);
      iupAttribSet(ih, "_IUPGTK4_CANVAS_BUFFER", NULL);
    }
  }

  (void)area;
  (void)width;
  (void)height;
}

static void gtk4CanvasSizeAllocate(GtkDrawingArea *drawing_area, int width, int height, gpointer user_data)
{
  Ihandle *ih = (Ihandle*)user_data;
  IFnii cb;

  if (!ih->data->inside_resize)
  {
    ih->data->inside_resize = 1;

    cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
    if (cb)
    {
      cb(ih, width, height);
    }

    ih->data->inside_resize = 0;
  }

  gtk4CanvasUpdateChildLayout(ih, 0);

  (void)drawing_area;
}

static char* gtk4CanvasGetDrawSizeAttrib(Ihandle *ih)
{
  if (ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    int w = gtk_widget_get_width(ih->handle);
    int h = gtk_widget_get_height(ih->handle);
    if (w > 0 && h > 0)
      return iupStrReturnIntInt(w, h, 'x');
  }
  return NULL;
}

static int gtk4CanvasCheckScroll(double min, double max, double *page, double *pos)
{
  double old_pos = *pos;
  double range = max-min;
  if (*page > range) *page = range;
  if (*page <= 0) *page = range/10.;

  if (*pos < min) *pos = min;
  if (*pos > (max - *page)) *pos = max - *page;

  if (old_pos == *pos)
    return 0;
  else
    return 1;
}

static int gtk4CanvasSetDXAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    double xmin, xmax, linex;
    double dx;
    int value_changed;
    GtkAdjustment* sb_horiz_adjust;
    GtkWidget* sb_horiz = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_SBHORIZ");
    if (!sb_horiz) return 1;

    if (!iupStrToDoubleDef(value, &dx, 0.1))
      return 1;

    iupAttribSet(ih, "SB_RESIZE", NULL);

    xmin = iupAttribGetDouble(ih, "XMIN");
    xmax = iupAttribGetDouble(ih, "XMAX");

    if (!iupAttribGet(ih,"LINEX"))
    {
      linex = dx/10;
      if (linex==0)
        linex = 1;
    }
    else
      linex = iupAttribGetDouble(ih,"LINEX");

    /* GtkScrollbar no longer inherits from GtkRange, use gtk_scrollbar_get_adjustment */
    sb_horiz_adjust = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(sb_horiz));

    if (dx >= (xmax-xmin))
    {
      if (iupAttribGetBoolean(ih, "XAUTOHIDE"))
      {
        if (iupgtk4IsVisible(sb_horiz))
        {
          if (iupdrvIsVisible(ih))
            iupAttribSet(ih, "SB_RESIZE", "YES");
          gtk_widget_set_visible(sb_horiz, FALSE);
          gtk4CanvasUpdateChildLayout(ih, 1);
        }

        iupAttribSet(ih, "XHIDDEN", "YES");
      }
      else
        gtk_widget_set_sensitive(sb_horiz, FALSE);

      ih->data->posx = xmin;
      gtk4CanvasAdjustmentSetValue(ih, sb_horiz_adjust, xmin);
    }
    else
    {
      if (!iupgtk4IsVisible(sb_horiz))
      {
        if (iupdrvIsVisible(ih))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        gtk_widget_set_visible(sb_horiz, TRUE);
        gtk4CanvasUpdateChildLayout(ih, 1);
      }
      gtk_widget_set_sensitive(sb_horiz, TRUE);

      double page_size = dx;
      double dvalue = gtk_adjustment_get_value(sb_horiz_adjust);
      value_changed = gtk4CanvasCheckScroll(xmin, xmax, &page_size, &dvalue);
      gtk_adjustment_configure(sb_horiz_adjust, dvalue, xmin, xmax, linex, page_size, dx);

      iupAttribSet(ih, "XHIDDEN", "NO");

      /* gtk_adjustment_value_changed removed, changes propagate automatically */
      (void)value_changed;
    }
  }
  return 1;
}

static int gtk4CanvasSetDYAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    double ymin, ymax, liney;
    double dy;
    int value_changed;
    GtkAdjustment* sb_vert_adjust;
    GtkWidget* sb_vert = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_SBVERT");
    if (!sb_vert) return 1;

    if (!iupStrToDoubleDef(value, &dy, 0.1))
      return 1;

    iupAttribSet(ih, "SB_RESIZE", NULL);

    ymin = iupAttribGetDouble(ih, "YMIN");
    ymax = iupAttribGetDouble(ih, "YMAX");

    if (!iupAttribGet(ih,"LINEY"))
    {
      liney = dy/10;
      if (liney==0)
        liney = 1;
    }
    else
      liney = iupAttribGetDouble(ih,"LINEY");

    /* GtkScrollbar no longer inherits from GtkRange, use gtk_scrollbar_get_adjustment */
    sb_vert_adjust = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(sb_vert));

    if (dy >= (ymax-ymin))
    {
      if (iupAttribGetBoolean(ih, "YAUTOHIDE"))
      {
        if (iupgtk4IsVisible(sb_vert))
        {
          if (iupdrvIsVisible(ih))
            iupAttribSet(ih, "SB_RESIZE", "YES");
          gtk_widget_set_visible(sb_vert, FALSE);
          gtk4CanvasUpdateChildLayout(ih, 1);
        }

        iupAttribSet(ih, "YHIDDEN", "YES");
      }
      else
        gtk_widget_set_sensitive(sb_vert, FALSE);

      ih->data->posy = ymin;
      gtk4CanvasAdjustmentSetValue(ih, sb_vert_adjust, ymin);
    }
    else
    {
      if (!iupgtk4IsVisible(sb_vert))
      {
        if (iupdrvIsVisible(ih))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        gtk_widget_set_visible(sb_vert, TRUE);
        gtk4CanvasUpdateChildLayout(ih, 1);
      }
      gtk_widget_set_sensitive(sb_vert, TRUE);

      double page_size = dy;
      double dvalue = gtk_adjustment_get_value(sb_vert_adjust);
      value_changed = gtk4CanvasCheckScroll(ymin, ymax, &page_size, &dvalue);
      gtk_adjustment_configure(sb_vert_adjust, dvalue, ymin, ymax, liney, page_size, dy);

      iupAttribSet(ih, "YHIDDEN", "NO");

      /* gtk_adjustment_value_changed removed, changes propagate automatically */
      (void)value_changed;
    }
  }
  return 1;
}

static int gtk4CanvasSetPosXAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    GtkWidget* sb_horiz = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_SBHORIZ");
    if (sb_horiz)
    {
      double xmin, xmax, dx, posx;
      GtkAdjustment* sb_horiz_adjust;

      if (!iupStrToDoubleDef(value, &posx, 0.0))
        return 1;

      xmin = iupAttribGetDouble(ih, "XMIN");
      xmax = iupAttribGetDouble(ih, "XMAX");
      dx = iupAttribGetDouble(ih, "DX");

      if (posx < xmin) posx = xmin;
      if (posx > (xmax - dx)) posx = xmax - dx;

      ih->data->posx = (float)posx;

      /* GtkScrollbar no longer inherits from GtkRange, use gtk_scrollbar_get_adjustment */
    sb_horiz_adjust = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(sb_horiz));
      gtk4CanvasAdjustmentSetValue(ih, sb_horiz_adjust, posx);
    }
  }

  return 1;
}

static int gtk4CanvasSetPosYAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    GtkWidget* sb_vert = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_SBVERT");
    if (sb_vert)
    {
      double ymin, ymax, dy, posy;
      GtkAdjustment* sb_vert_adjust;

      if (!iupStrToDoubleDef(value, &posy, 0.0))
        return 1;

      ymin = iupAttribGetDouble(ih, "YMIN");
      ymax = iupAttribGetDouble(ih, "YMAX");
      dy = iupAttribGetDouble(ih, "DY");

      if (posy < ymin) posy = ymin;
      if (posy > (ymax - dy)) posy = ymax - dy;

      ih->data->posy = (float)posy;

      /* GtkScrollbar no longer inherits from GtkRange, use gtk_scrollbar_get_adjustment */
      sb_vert_adjust = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(sb_vert));
      gtk4CanvasAdjustmentSetValue(ih, sb_vert_adjust, posy);
    }
  }

  return 1;
}

static int gtk4CanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!value)
    value = iupAttribGetStr(ih, "BGCOLOR");

  if (!value)
    value = IupGetGlobal("DLGBGCOLOR");

  if (iupStrToRGB(value, &r, &g, &b))
  {
    iupgtk4SetBgColor(ih->handle, r, g, b);
    return 1;
  }

  return 0;
}


static void gtk4CanvasLayoutUpdateMethod(Ihandle* ih)
{
  int border;
  int sb_vert_width = 0, sb_horiz_height = 0;
  int width, height;

  /* Validate handle before attempting to set size */
  if (!ih->handle || !GTK_IS_DRAWING_AREA(ih->handle))
    return;

  /* Get border and scrollbar sizes */
  border = iupAttribGetInt(ih, "_IUPGTK4_BORDER");

  if (ih->data->sb)
  {
    GtkWidget* sb_vert = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_SBVERT");
    GtkWidget* sb_horiz = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_SBHORIZ");

    if (sb_vert && iupgtk4IsVisible(sb_vert))
      sb_vert_width = iupdrvGetScrollbarSize();
    if (sb_horiz && iupgtk4IsVisible(sb_horiz))
      sb_horiz_height = iupdrvGetScrollbarSize();
  }

  /* Calculate content size accounting for scrollbars only (NOT border).
     The border is drawn by the container's snapshot method and naturally creates spacing. */
  width = ih->currentwidth - sb_vert_width;
  height = ih->currentheight - sb_horiz_height;

  /* GTK4 DrawingArea REQUIRES content_width/height to have any size. */
  gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(ih->handle), width);
  gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(ih->handle), height);

  iupdrvBaseLayoutUpdateMethod(ih);

  if (ih->data->sb)
    gtk4CanvasUpdateChildLayout(ih, 0);
}

static int gtk4CanvasMapMethod(Ihandle* ih)
{
  GtkWidget* sb_win;

  if (!ih->parent)
    return IUP_ERROR;

  ih->data->sb = iupBaseGetScrollbar(ih);

  ih->handle = gtk_drawing_area_new();

  if (!ih->handle)
      return IUP_ERROR;

  sb_win = iupgtk4NativeContainerNew();
  if (!sb_win)
    return IUP_ERROR;

  iupgtk4NativeContainerAdd(sb_win, ih->handle);

  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)sb_win);

  iupgtk4AddToParent(ih);

  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(ih->handle), (GtkDrawingAreaDrawFunc)gtk4CanvasDraw, ih, NULL);

  iupgtk4SetupFocusEvents(ih->handle, ih);
  iupgtk4SetupKeyEvents(ih->handle, ih);
  iupgtk4SetupEnterLeaveEvents(ih->handle, ih);

  GtkEventController* scroll_controller = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
  gtk_widget_add_controller(ih->handle, scroll_controller);
  g_signal_connect(scroll_controller, "scroll", G_CALLBACK(gtk4CanvasScrollEvent), ih);

  GtkGesture* click_gesture = gtk_gesture_click_new();
  gtk_widget_add_controller(ih->handle, GTK_EVENT_CONTROLLER(click_gesture));
  g_signal_connect(click_gesture, "pressed", G_CALLBACK(gtk4CanvasButtonPressed), ih);
  g_signal_connect(click_gesture, "released", G_CALLBACK(gtk4CanvasButtonReleased), ih);

  iupgtk4SetupButtonEvents(ih->handle, ih);
  iupgtk4SetupMotionEvents(ih->handle, ih);

  g_signal_connect(G_OBJECT(ih->handle), "resize", G_CALLBACK(gtk4CanvasSizeAllocate), ih);

  if (ih->iclass->is_interactive)
  {
    if (iupAttribGetBoolean(ih, "CANFOCUS"))
      iupgtk4SetCanFocus(ih->handle, 1);
  }

  if (iupAttribGetBoolean(ih, "BORDER"))
  {
    /* Border is drawn using snapshot method on iupGtk4Fixed container,
     * and canvas size is reduced in LayoutUpdateMethod (same approach as GTK3). */
    iupAttribSetInt(ih, "_IUPGTK4_BORDER", 1);
    iupgtk4NativeContainerSetBorder(sb_win, 1);
  }

  gtk_widget_realize(sb_win);

  if (ih->data->sb & IUP_SB_HORIZ)
  {
    /* Create adjustment first */
    GtkAdjustment* sb_horiz_adjust = gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 10.0, 10.0);

    /* Create scrollbar with the adjustment */
    GtkWidget* sb_horiz = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, sb_horiz_adjust);
    iupgtk4NativeContainerAdd(sb_win, sb_horiz);
    gtk_widget_set_visible(sb_horiz, TRUE);

    /* Connect to adjustment's value-changed signal BEFORE realize */
    g_signal_connect(G_OBJECT(sb_horiz_adjust), "value-changed", G_CALLBACK(gtk4CanvasAdjustHorizValueChanged), ih);

    gtk_widget_realize(sb_horiz);

    iupAttribSet(ih, "_IUPGTK4_SBHORIZ", (char*)sb_horiz);
  }

  if (ih->data->sb & IUP_SB_VERT)
  {
    /* Create adjustment first */
    GtkAdjustment* sb_vert_adjust = gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 10.0, 10.0);

    /* Create scrollbar with the adjustment */
    GtkWidget* sb_vert = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, sb_vert_adjust);
    iupgtk4NativeContainerAdd(sb_win, sb_vert);
    gtk_widget_set_visible(sb_vert, TRUE);

    /* Connect to adjustment's value-changed signal BEFORE realize */
    g_signal_connect(G_OBJECT(sb_vert_adjust), "value-changed", G_CALLBACK(gtk4CanvasAdjustVertValueChanged), ih);

    gtk_widget_realize(sb_vert);

    iupAttribSet(ih, "_IUPGTK4_SBVERT", (char*)sb_vert);
  }

  gtk_widget_realize(ih->handle);

  /* Set initial content size BEFORE any subsystem (like EGL) tries to access the GdkSurface.
     On Wayland, the wl_surface won't be properly accessible until content size is set.
     We use RASTERSIZE if available, otherwise a minimal default.
     The LayoutUpdate method will update this to the final calculated size later. */
  {
    int init_width = ih->userwidth > 0 ? ih->userwidth : (ih->naturalwidth > 0 ? ih->naturalwidth : 1);
    int init_height = ih->userheight > 0 ? ih->userheight : (ih->naturalheight > 0 ? ih->naturalheight : 1);

    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(ih->handle), init_width);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(ih->handle), init_height);
  }

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  iupgtk4UpdateMnemonic(ih);

  gtk4CanvasSetBgColorAttrib(ih, iupAttribGetStr(ih, "BGCOLOR"));

  gtk4CanvasSetDXAttrib(ih, NULL);
  gtk4CanvasSetDYAttrib(ih, NULL);

  return IUP_NOERROR;
}

void iupdrvCanvasInitClass(Iclass* ic)
{
  ic->Map = gtk4CanvasMapMethod;
  ic->LayoutUpdate = gtk4CanvasLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", iupBaseNativeParentGetBgColorAttrib, gtk4CanvasSetBgColorAttrib, "255 255 255", NULL, IUPAF_NO_SAVE|IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "DX", NULL, gtk4CanvasSetDXAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", NULL, gtk4CanvasSetDYAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, gtk4CanvasSetPosXAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, gtk4CanvasSetPosYAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAWSIZE", gtk4CanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CURSOR", NULL, iupdrvBaseSetCursorAttrib, IUPAF_SAMEASSYSTEM, "ARROW", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAWDRIVER", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAWCOLOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAWLINEWIDTH", NULL, NULL, "1", NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "XMIN", NULL, NULL, "0.0", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XMAX", NULL, NULL, "1.0", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMIN", NULL, NULL, "0.0", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMAX", NULL, NULL, "1.0", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LINEX", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEY", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "XAUTOHIDE", NULL, NULL, "YES", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YAUTOHIDE", NULL, NULL, "YES", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CAIRO_CR", NULL, NULL, NULL, NULL, IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "CLIPRECT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, "YES", NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
