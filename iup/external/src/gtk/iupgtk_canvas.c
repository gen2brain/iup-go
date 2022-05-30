/** \file
 * \brief Canvas Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

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

#include "iupgtk_drv.h"


static void gtkCanvasUpdateChildLayout(Ihandle *ih, int flush)
{
  GtkContainer* sb_win = (GtkContainer*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  GtkWidget* sb_horiz = (GtkWidget*)iupAttribGet(ih, "_IUPGTK_SBHORIZ");
  GtkWidget* sb_vert = (GtkWidget*)iupAttribGet(ih, "_IUPGTK_SBVERT");
  int sb_vert_width=0, sb_horiz_height=0;
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int border = iupAttribGetInt(ih, "_IUPGTK_BORDER");

  if (sb_vert && iupgtkIsVisible(sb_vert))
    sb_vert_width = iupdrvGetScrollbarSize();
  if (sb_horiz && iupgtkIsVisible(sb_horiz))
    sb_horiz_height = iupdrvGetScrollbarSize();

  if (sb_vert_width)
    iupgtkSetPosSize(sb_win, sb_vert, width-sb_vert_width-border, border, sb_vert_width, height-sb_horiz_height-2*border);
  if (sb_horiz_height)
    iupgtkSetPosSize(sb_win, sb_horiz, border, height-sb_horiz_height-border, width-sb_vert_width-2*border, sb_horiz_height);

  iupgtkSetPosSize(sb_win, ih->handle, border, border, width-sb_vert_width-2*border, height-sb_horiz_height-2*border);

  if (flush)
    IupFlush();
}

static int gtkCanvasScroll2Iup(GtkScrollType scroll, int vert)
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

  /* No IUP_SBDRAGV or IUP_SBDRAGH support in GTK */

  return -1;
}

static gboolean gtkCanvasScrollHorizChangeValue(GtkRange *range, GtkScrollType scroll, double value, Ihandle *ih)
{
  int op = gtkCanvasScroll2Iup(scroll, 0);
  iupAttribSetInt(ih, "_IUPGTK_SBOP", op);
  (void)range;
  (void)value;
  return FALSE;
}

static void gtkCanvasAdjustHorizValueChanged(GtkAdjustment *adjustment, Ihandle *ih)
{
  double posx, posy, xmin, xmax, dx;
  IFniff scroll_cb;

  posx = gtk_adjustment_get_value(adjustment);
  if (ih->data->posx==posx)
    return;

  xmin = iupAttribGetDouble(ih, "XMIN");
  xmax = iupAttribGetDouble(ih, "XMAX");
  dx = iupAttribGetDouble(ih, "DX");
  if (posx < xmin) posx = xmin;
  if (posx > xmax-dx) posx = xmax-dx;
  ih->data->posx = posx;

  posy = ih->data->posy;

  if (iupAttribGet(ih, "_IUPGTK_SETSBPOS"))
    return;

  scroll_cb = (IFniff)IupGetCallback(ih,"SCROLL_CB");
  if (scroll_cb)
  {
    int op = IUP_SBPOSH;
    char* sbop = iupAttribGet(ih, "_IUPGTK_SBOP");
    if (sbop) iupStrToInt(sbop, &op);
    if (op == -1)
      return;

    scroll_cb(ih, op, (float)posx, (float)posy);

    iupAttribSet(ih, "_IUPGTK_SBOP", NULL);
  }
  else
  {
    IFnff cb = (IFnff)IupGetCallback(ih,"ACTION");
    if (cb)
    {
      /* REDRAW Now (since 3.24) - to allow a full native redraw process */
      iupdrvRedrawNow(ih);
      /* cb(ih, (float)posx, (float)posy); - OLD method */
    }
  }
}

static gboolean gtkCanvasScrollVertChangeValue(GtkRange *range, GtkScrollType scroll, double value, Ihandle *ih)
{
  int op = gtkCanvasScroll2Iup(scroll, 1);
  iupAttribSetInt(ih, "_IUPGTK_SBOP", op);
  (void)range;
  (void)value;
  return FALSE;
}

static void gtkCanvasAdjustVertValueChanged(GtkAdjustment *adjustment, Ihandle *ih)
{
  double posx, posy, ymin, ymax, dy;
  IFniff scroll_cb;

  posy = gtk_adjustment_get_value(adjustment);
  if (ih->data->posy==posy)
    return;

  ymin = iupAttribGetDouble(ih, "YMIN");
  ymax = iupAttribGetDouble(ih, "YMAX");
  dy = iupAttribGetDouble(ih, "DY");

  if (posy < ymin) posy = ymin;
  if (posy > ymax-dy) posy = ymax-dy;
  ih->data->posy = posy;

  posx = ih->data->posx;

  if (iupAttribGet(ih, "_IUPGTK_SETSBPOS"))
    return;

  scroll_cb = (IFniff)IupGetCallback(ih,"SCROLL_CB");
  if (scroll_cb)
  {
    int op = IUP_SBPOSV;
    char* sbop = iupAttribGet(ih, "_IUPGTK_SBOP");
    if (sbop) iupStrToInt(sbop, &op);
    if (op == -1)
      return;

    scroll_cb(ih, op, (float)posx, (float)posy);

    iupAttribSet(ih, "_IUPGTK_SBOP", NULL);
  }
  else
  {
    IFnff cb = (IFnff)IupGetCallback(ih,"ACTION");
    if (cb)
    {
      /* REDRAW Now (since 3.24) - to allow a full native redraw process */
      iupdrvRedrawNow(ih);
      /* cb(ih, (float)posx, (float)posy); - OLD method */
    }
  }
}

static gboolean gtkCanvasScrollEvent(GtkWidget *widget, GdkEventScroll *evt, Ihandle *ih)
{    
  /* occurs only for the mouse wheel. Not related to the scrollbars */
  IFnfiis wcb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");

  if (evt->direction > GDK_SCROLL_RIGHT)
    return TRUE;

  if (iupAttribGetBoolean(ih, "WHEELDROPFOCUS"))
  {
    Ihandle* ih_focus = IupGetFocus();
    if (iupObjectCheck(ih_focus))
      iupAttribSetClassObject(ih_focus, "SHOWDROPDOWN", "NO");
  }

  if (wcb)
  {
    int delta = evt->direction==GDK_SCROLL_UP||evt->direction==GDK_SCROLL_LEFT? 1: -1;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    int button = evt->direction==GDK_SCROLL_UP||evt->direction==GDK_SCROLL_LEFT? 4: 5;
    iupgtkButtonKeySetStatus(evt->state, button, status, 0);

    wcb(ih, (float)delta, (int)evt->x, (int)evt->y, status);
  }
  else
  {
    IFniff scb = (IFniff)IupGetCallback(ih,"SCROLL_CB");
    int delta = evt->direction==GDK_SCROLL_UP||evt->direction==GDK_SCROLL_LEFT? 1: -1;

    if (evt->direction==GDK_SCROLL_UP || evt->direction==GDK_SCROLL_DOWN)
    {
      double posy = ih->data->posy;
      posy -= delta*iupAttribGetDouble(ih, "DY")/10.0;
      IupSetDouble(ih, "POSY", posy);
    }
    else
    {
      double posx = ih->data->posx;
      posx -= delta*iupAttribGetDouble(ih, "DX")/10.0;
      IupSetDouble(ih, "POSX", posx);
    }

    if (scb)
    {
      int scroll_gtk2iup[4] = {IUP_SBUP, IUP_SBDN, IUP_SBLEFT, IUP_SBRIGHT};
      int op = scroll_gtk2iup[evt->direction];
      scb(ih, op, (float)ih->data->posx, (float)ih->data->posy);
    }
  }
  (void)widget;
  return TRUE;
}

static gboolean gtkCanvasButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih)
{
  if (evt->type == GDK_BUTTON_PRESS)
  {
    gtk_grab_add(widget);

    /* Force focus on canvas click */
    if (iupAttribGetBoolean(ih, "CANFOCUS"))
      gtk_widget_grab_focus(ih->handle);
  }
  else  /* GDK_BUTTON_RELEASE */
  {
    gtk_grab_remove(widget);
  }

  iupgtkButtonEvent(widget, evt, ih);

  return TRUE; /* stop other handlers from being invoked */
}

static gboolean gtkCanvasMotionNotifyEvent(GtkWidget *widget, GdkEventMotion *evt, Ihandle *ih)
{
  iupgtkMotionNotifyEvent(widget, evt, ih);
  return TRUE; /* stop other handlers from being invoked */
}

gboolean gtkCanvasFocusOutEvent(GtkWidget *widget, GdkEventFocus *evt, Ihandle *ih)
{
  if (widget == gtk_grab_get_current())
    gtk_grab_remove(widget);

  return iupgtkFocusInOutEvent(widget, evt, ih);
}

static int gtkCanvasSetBgColorAttrib(Ihandle* ih, const char* value);

#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean gtkCanvasDraw(GtkWidget *widget, cairo_t* cr, Ihandle *ih)
#else
static gboolean gtkCanvasExposeEvent(GtkWidget *widget, GdkEventExpose *evt, Ihandle *ih)
#endif
{
  IFnff cb = (IFnff)IupGetCallback(ih,"ACTION");
  if (cb && !(ih->data->inside_resize))
  {
    /* IMPORTANT: this will not fully work at the first time because the GTK internal double buffer already started. 
                  On the first time, the canvas will be configured correctly but after calling the application callback,
                  GTK will overwrite its contents with the BGCOLOR. */
    if (!iupAttribGet(ih, "_IUPGTK_NO_BGCOLOR"))
      gtkCanvasSetBgColorAttrib(ih, iupAttribGetStr(ih, "BGCOLOR"));  /* reset to update window attributes */

#if GTK_CHECK_VERSION(3, 0, 0)
    {
      GdkRectangle rect;
      gdk_cairo_get_clip_rectangle(cr, &rect);
      iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", rect.x, rect.y, rect.x+rect.width-1, rect.y+rect.height-1);
      iupAttribSet(ih, "CAIRO_CR", (char*)cr);
    }
#else
    iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", evt->area.x, evt->area.y, evt->area.x+evt->area.width-1, evt->area.y+evt->area.height-1);
#endif

    cb(ih, (float)ih->data->posx, (float)ih->data->posy);

    iupAttribSet(ih, "CLIPRECT", NULL);
    iupAttribSet(ih, "CAIRO_CR", NULL);
  }

  (void)widget;
  return FALSE;
}

#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean gtkCanvasBorderDraw(GtkWidget *widget, cairo_t* cr, void* user)
#else
static gboolean gtkCanvasBorderExposeEvent(GtkWidget *widget, GdkEventExpose *evt, void* user)
#endif
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GtkStyleContext* context = gtk_widget_get_style_context (widget);
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);  
  gtk_style_context_save (context);
  gtk_style_context_add_class(context, GTK_STYLE_CLASS_FRAME);
  gtk_render_frame (context, cr, 0, 0, allocation.width, allocation.height);
  gtk_style_context_restore (context);
#else
  GdkWindow* window = iupgtkGetWindow(widget);
  GtkStyle *style = gtk_widget_get_style(widget);
  GtkAllocation allocation;
#if GTK_CHECK_VERSION(2, 18, 0)
  gtk_widget_get_allocation(widget, &allocation);
#else
  allocation = widget->allocation;
#endif
  gtk_paint_shadow(style, window, GTK_STATE_NORMAL, GTK_SHADOW_IN,
	                  &evt->area, widget, "scrolled_window",
	                  allocation.x, allocation.y, allocation.width, allocation.height);
#endif
  (void)user;
  return FALSE;
}

static void gtkCanvasLayoutUpdateMethod(Ihandle *ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  /* Force GdkWindow size update when not visible,
     so when mapped before show GDK returns the correct value. */
  if (!iupdrvIsVisible(ih))
  {
    GdkWindow* window = iupgtkGetWindow(ih->handle);
    if (window)
      gdk_window_resize(window, ih->currentwidth, ih->currentheight);
  }

  gtkCanvasUpdateChildLayout(ih, 0);
}

static void gtkCanvasSizeAllocate(GtkWidget* widget, GdkRectangle *allocation, Ihandle *ih)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (cb && !(ih->data->inside_resize))
  {
    ih->data->inside_resize = 1;  /* avoid recursion */
    cb(ih, allocation->width, allocation->height);
    ih->data->inside_resize = 0;
  }

  (void)widget;
}

static void gtkCanvasAdjustmentSetValue(Ihandle *ih, GtkAdjustment *adjustment, double value)
{
  iupAttribSet(ih, "_IUPGTK_SETSBPOS", "1");
  gtk_adjustment_set_value(adjustment, value);
  iupAttribSet(ih, "_IUPGTK_SETSBPOS", NULL);
}

static int gtkCanvasCheckScroll(double min, double max, double *page, double *pos)
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

static int gtkCanvasSetDXAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    double xmin, xmax, linex;
    double dx;
    int value_changed;
    GtkAdjustment* sb_horiz_adjust;
    GtkWidget* sb_horiz = (GtkWidget*)iupAttribGet(ih, "_IUPGTK_SBHORIZ");
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

    sb_horiz_adjust = gtk_range_get_adjustment(GTK_RANGE(sb_horiz));

    if (dx >= (xmax-xmin))
    {
      if (iupAttribGetBoolean(ih, "XAUTOHIDE"))
      {
        if (iupgtkIsVisible(sb_horiz))
        {
          if (iupdrvIsVisible(ih))
            iupAttribSet(ih, "SB_RESIZE", "YES");
          gtk_widget_hide(sb_horiz);
          gtkCanvasUpdateChildLayout(ih, 1);
        }

        iupAttribSet(ih, "XHIDDEN", "YES");
      }
      else
        gtk_widget_set_sensitive(sb_horiz, FALSE);

      ih->data->posx = xmin;
      gtkCanvasAdjustmentSetValue(ih, sb_horiz_adjust, xmin);
    }
    else
    {
      if (!iupgtkIsVisible(sb_horiz))
      {
        if (iupdrvIsVisible(ih))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        gtk_widget_show(sb_horiz);
        gtkCanvasUpdateChildLayout(ih, 1);
      }
      gtk_widget_set_sensitive(sb_horiz, TRUE);

#if GTK_CHECK_VERSION(2, 14, 0)
    {
      double page_size = dx;
      double dvalue = gtk_adjustment_get_value(sb_horiz_adjust);
      value_changed = gtkCanvasCheckScroll(xmin, xmax, &page_size, &dvalue);
      gtk_adjustment_configure(sb_horiz_adjust, dvalue, xmin, xmax, linex, page_size, dx);
    }
#else
      sb_horiz_adjust->lower = xmin;
      sb_horiz_adjust->upper = xmax;
      sb_horiz_adjust->step_increment = linex;
      sb_horiz_adjust->page_size = dx;

      value_changed = gtkCanvasCheckScroll(xmin, xmax, &sb_horiz_adjust->page_size, &sb_horiz_adjust->value);
      sb_horiz_adjust->page_increment = sb_horiz_adjust->page_size;

      gtk_adjustment_changed(sb_horiz_adjust);
#endif
      iupAttribSet(ih, "XHIDDEN", "NO");

      if (value_changed)
        gtk_adjustment_value_changed(sb_horiz_adjust);
    }
  }
  return 1;
}

static int gtkCanvasSetDYAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    double ymin, ymax, liney;
    double dy;
    int value_changed;
    GtkAdjustment* sb_vert_adjust;
    GtkWidget* sb_vert = (GtkWidget*)iupAttribGet(ih, "_IUPGTK_SBVERT");
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

    sb_vert_adjust = gtk_range_get_adjustment(GTK_RANGE(sb_vert));

    if (dy >= (ymax-ymin))
    {
      if (iupAttribGetBoolean(ih, "YAUTOHIDE"))
      {
        if (iupgtkIsVisible(sb_vert))
        {
          if (iupdrvIsVisible(ih))
            iupAttribSet(ih, "SB_RESIZE", "YES");
          gtk_widget_hide(sb_vert);
          gtkCanvasUpdateChildLayout(ih, 1);
        }
        
        iupAttribSet(ih, "YHIDDEN", "YES");
      }
      else
        gtk_widget_set_sensitive(sb_vert, FALSE);

      ih->data->posy = ymin;
      gtkCanvasAdjustmentSetValue(ih, sb_vert_adjust, ymin);
    }
    else
    {
      if (!iupgtkIsVisible(sb_vert))
      {
        if (iupdrvIsVisible(ih))
          iupAttribSet(ih, "SB_RESIZE", "YES");
        gtk_widget_show(sb_vert);
        gtkCanvasUpdateChildLayout(ih, 1);
      }
      gtk_widget_set_sensitive(sb_vert, TRUE);

#if GTK_CHECK_VERSION(2, 14, 0)
    {
      double page_size = dy;
      double dvalue = gtk_adjustment_get_value(sb_vert_adjust);
      value_changed = gtkCanvasCheckScroll(ymin, ymax, &page_size, &dvalue);
      gtk_adjustment_configure(sb_vert_adjust, dvalue, ymin, ymax, liney, page_size, dy);
    }
#else
      sb_vert_adjust->lower = ymin;
      sb_vert_adjust->upper = ymax;
      sb_vert_adjust->step_increment = liney;
      sb_vert_adjust->page_size = dy;

      value_changed = gtkCanvasCheckScroll(ymin, ymax, &sb_vert_adjust->page_size, &sb_vert_adjust->value);
      sb_vert_adjust->page_increment = sb_vert_adjust->page_size;

      gtk_adjustment_changed(sb_vert_adjust);
#endif
      iupAttribSet(ih, "YHIDDEN", "NO");

      if (value_changed)
        gtk_adjustment_value_changed(sb_vert_adjust);
    }
  }
  return 1;
}

static int gtkCanvasSetPosXAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    double posx, xmin, xmax, dx;
    GtkAdjustment* sb_horiz_adjust;
    GtkWidget* sb_horiz = (GtkWidget*)iupAttribGet(ih, "_IUPGTK_SBHORIZ");
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

    sb_horiz_adjust = gtk_range_get_adjustment(GTK_RANGE(sb_horiz));
    gtkCanvasAdjustmentSetValue(ih, sb_horiz_adjust, posx);
  }
  return 1;
}

static int gtkCanvasSetPosYAttrib(Ihandle* ih, const char *value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    double posy, ymin, ymax, dy;
    GtkAdjustment* sb_vert_adjust;
    GtkWidget* sb_vert = (GtkWidget*)iupAttribGet(ih, "_IUPGTK_SBVERT");
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

    sb_vert_adjust = gtk_range_get_adjustment(GTK_RANGE(sb_vert));
    gtkCanvasAdjustmentSetValue(ih, sb_vert_adjust, posy);
  }
  return 1;
}

static int gtkCanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
  GtkWidget* sb_win = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!IupGetCallback(ih, "ACTION"))
  {
    unsigned char r, g, b;

    /* ignore given value, must use only from parent for the scrollbars */
    char* parent_value = iupBaseNativeParentGetBgColor(ih);

    if (iupStrToRGB(parent_value, &r, &g, &b) && iupStrBoolean(IupGetGlobal("SB_BGCOLOR")))
    {
      GtkWidget* sb;

      iupgtkSetBgColor(sb_win, r, g, b);

      sb = (GtkWidget*)iupAttribGet(ih, "_IUPGTK_SBHORIZ");
      if (sb) iupgtkSetBgColor(sb, r, g, b);
      sb = (GtkWidget*)iupAttribGet(ih, "_IUPGTK_SBVERT");
      if (sb) iupgtkSetBgColor(sb, r, g, b);
    }

    /* enable automatic double buffering */
    gtk_widget_set_double_buffered(ih->handle, TRUE);
    gtk_widget_set_double_buffered(sb_win, TRUE);

    return iupdrvBaseSetBgColorAttrib(ih, value);
  }
  else
  {
    /* disable automatic double buffering if not a container or an OpenGL canvas */
    if (ih->iclass->childtype != IUP_CHILDNONE && !iupAttribGet(ih, "_IUP_GLCONTROLDATA"))
    {
      gtk_widget_set_double_buffered(ih->handle, TRUE);
      gtk_widget_set_double_buffered(sb_win, TRUE);
    }
    else
    {
      gtk_widget_set_double_buffered(ih->handle, FALSE);
      gtk_widget_set_double_buffered(sb_win, FALSE);

#if !GTK_CHECK_VERSION(3, 0, 0)
      gdk_window_set_back_pixmap(iupgtkGetWindow(ih->handle), NULL, FALSE);
#endif
    }

    iupAttribSet(ih, "_IUPGTK_NO_BGCOLOR", "1");
    return 1;
  }
}

static char* gtkCanvasGetDrawSizeAttrib(Ihandle *ih)
{
  GdkWindow* window = iupgtkGetWindow(ih->handle);
  if (window)
  {
    int w, h;
#if GTK_CHECK_VERSION(2, 24, 0)
    w = gdk_window_get_width(window);
    h = gdk_window_get_height(window);
#else
    gdk_drawable_get_size(window, &w, &h);
#endif
    return iupStrReturnIntInt(w, h, 'x');
  }
  else
    return NULL;
}

static char* gtkCanvasGetDrawableAttrib(Ihandle* ih)
{
  return (char*)iupgtkGetWindow(ih->handle);
}

static int gtkCanvasMapMethod(Ihandle* ih)
{
  GtkWidget* sb_win;
#if !GTK_CHECK_VERSION(3, 0, 0)
  void* visual;
#endif

  if (!ih->parent)
    return IUP_ERROR;

  ih->data->sb = iupBaseGetScrollbar(ih);

#if !GTK_CHECK_VERSION(3, 0, 0)
  visual = (void*)IupGetAttribute(ih, "VISUAL");   /* defined by the OpenGL Canvas in X11 or NULL */
  if (visual)
    iupgtkPushVisualAndColormap(visual, (void*)iupAttribGet(ih, "COLORMAP"));
#endif

  /* canvas is also a container */
  /* use a window to be a full native container */
  ih->handle = iupgtkNativeContainerNew(1);  

#if !GTK_CHECK_VERSION(3, 0, 0)
  if (visual)
    gtk_widget_pop_colormap();
#endif

  if (!ih->handle)
      return IUP_ERROR;

  sb_win = iupgtkNativeContainerNew(0);
  if (!sb_win)
    return IUP_ERROR;

  iupgtkNativeContainerAdd(sb_win, ih->handle);

  gtk_widget_show(sb_win);

  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)sb_win);

  /* the application intends to draw on the widget */
  gtk_widget_set_app_paintable(ih->handle, TRUE);

  /* add to the parent, all GTK controls must call this. */
  iupgtkAddToParent(ih);

  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(gtkCanvasFocusOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(iupgtkKeyPressEvent),   ih);
  g_signal_connect(G_OBJECT(ih->handle), "key-release-event",  G_CALLBACK(iupgtkKeyReleaseEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp),        ih);

#if GTK_CHECK_VERSION(3, 0, 0)
  g_signal_connect(G_OBJECT(ih->handle), "draw",               G_CALLBACK(gtkCanvasDraw), ih);
#else
  g_signal_connect(G_OBJECT(ih->handle), "expose-event",       G_CALLBACK(gtkCanvasExposeEvent), ih);
#endif
  g_signal_connect(G_OBJECT(ih->handle), "button-press-event", G_CALLBACK(gtkCanvasButtonEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "button-release-event",G_CALLBACK(gtkCanvasButtonEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "motion-notify-event", G_CALLBACK(gtkCanvasMotionNotifyEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "scroll-event",G_CALLBACK(gtkCanvasScrollEvent), ih);

  g_signal_connect(G_OBJECT(ih->handle), "size-allocate", G_CALLBACK(gtkCanvasSizeAllocate), ih);

  /* To receive mouse events on a drawing area, you will need to enable them. */
  gtk_widget_add_events(ih->handle, GDK_EXPOSURE_MASK|
    GDK_POINTER_MOTION_MASK|GDK_POINTER_MOTION_HINT_MASK|
    GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_BUTTON_MOTION_MASK|
    GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | 
    GDK_SCROLL_MASK |  /* Added for GTK3, but it seems to work ok for GTK2 */
    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
    GDK_FOCUS_CHANGE_MASK|GDK_STRUCTURE_MASK);

  /* To receive keyboard events, you will need to set the GTK_CAN_FOCUS flag on the drawing area. */
  if (ih->iclass->is_interactive)
  {
    if (iupAttribGetBoolean(ih, "CANFOCUS"))
      iupgtkSetCanFocus(ih->handle, 1);
  }

  if (iupAttribGetBoolean(ih, "BORDER"))
  {
    iupAttribSetInt(ih, "_IUPGTK_BORDER", 1);
#if GTK_CHECK_VERSION(3, 0, 0)
    g_signal_connect(G_OBJECT(sb_win), "draw", G_CALLBACK(gtkCanvasBorderDraw), NULL);
#else
    g_signal_connect(G_OBJECT(sb_win), "expose-event", G_CALLBACK(gtkCanvasBorderExposeEvent), NULL);
#endif
  }

  gtk_widget_realize(sb_win);

  if (ih->data->sb & IUP_SB_HORIZ)
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkWidget* sb_horiz = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, NULL);
#else
    GtkWidget* sb_horiz = gtk_hscrollbar_new(NULL);
#endif
    iupgtkNativeContainerAdd(sb_win, sb_horiz);
    gtk_widget_show(sb_horiz);
    gtk_widget_realize(sb_horiz);

    g_signal_connect(G_OBJECT(sb_horiz), "change-value",G_CALLBACK(gtkCanvasScrollHorizChangeValue), ih);
    g_signal_connect(G_OBJECT(gtk_range_get_adjustment(GTK_RANGE(sb_horiz))), "value-changed",G_CALLBACK(gtkCanvasAdjustHorizValueChanged), ih);

    iupAttribSet(ih, "_IUPGTK_SBHORIZ", (char*)sb_horiz);
  }

  if (ih->data->sb & IUP_SB_VERT)
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkWidget* sb_vert = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);
#else
    GtkWidget* sb_vert = gtk_vscrollbar_new(NULL);
#endif
    iupgtkNativeContainerAdd(sb_win, sb_vert);
    gtk_widget_show(sb_vert);
    gtk_widget_realize(sb_vert);

    g_signal_connect(G_OBJECT(sb_vert), "change-value",G_CALLBACK(gtkCanvasScrollVertChangeValue), ih);
    g_signal_connect(G_OBJECT(gtk_range_get_adjustment(GTK_RANGE(sb_vert))), "value-changed",G_CALLBACK(gtkCanvasAdjustVertValueChanged), ih);
    iupAttribSet(ih, "_IUPGTK_SBVERT", (char*)sb_vert);
  }

  gtk_widget_realize(ih->handle);

  /* configure for DRAG&DROP */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  /* update a mnemonic in a label if necessary */
  iupgtkUpdateMnemonic(ih);

  /* force the update of BGCOLOR here, to let derived classes ignore it if ACTION is defined */
  gtkCanvasSetBgColorAttrib(ih, iupAttribGetStr(ih, "BGCOLOR"));

  gtkCanvasSetDXAttrib(ih, NULL);
  gtkCanvasSetDYAttrib(ih, NULL);

  return IUP_NOERROR;
}

void iupdrvCanvasInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkCanvasMapMethod;
  ic->LayoutUpdate = gtkCanvasLayoutUpdateMethod;

  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkCanvasSetBgColorAttrib, "255 255 255", NULL, IUPAF_DEFAULT);  /* force new default value */

  /* IupCanvas only */
  iupClassRegisterAttribute(ic, "DRAWSIZE", gtkCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DX", NULL, gtkCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "DY", NULL, gtkCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, gtkCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, gtkCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);  /* force new default value */

  iupClassRegisterAttribute(ic, "DRAWABLE", gtkCanvasGetDrawableAttrib, NULL, NULL, NULL, IUPAF_NO_STRING);

  /* IupCanvas Windows or X only */
  iupClassRegisterAttribute(ic, iupgtkGetNativeWindowHandleName(), iupgtkGetNativeWindowHandleAttrib, NULL, NULL, NULL, IUPAF_NO_STRING | IUPAF_NO_INHERIT);
  if (iupdrvGetDisplay())
    iupClassRegisterAttribute(ic, "XDISPLAY", (IattribGetFunc)iupdrvGetDisplay, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, "YES", NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
