/** \file
 * \brief IupPopover control for GTK
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_class.h"
#include "iup_popover.h"

#include "iupgtk_drv.h"


#if GTK_CHECK_VERSION(3, 12, 0)

/***********************************************************************************
 * Native GtkPopover implementation (GTK >= 3.12)
 ***********************************************************************************/

typedef struct _iupGtkPopover
{
  GtkPopover popover;
  gboolean has_arrow;
} iupGtkPopover;

typedef struct _iupGtkPopoverClass
{
  GtkPopoverClass parent_class;
} iupGtkPopoverClass;

static GType iup_gtk_popover_get_type(void) G_GNUC_CONST;
static void iup_gtk_popover_class_init(iupGtkPopoverClass* _class);
static void iup_gtk_popover_init(iupGtkPopover* popover);

G_DEFINE_TYPE(iupGtkPopover, iup_gtk_popover, GTK_TYPE_POPOVER)

#define IUP_GTK_POPOVER(obj) ((iupGtkPopover*)(obj))

static void iup_gtk_popover_init(iupGtkPopover* popover)
{
  popover->has_arrow = TRUE;
}

static void iup_gtk_popover_get_frame(GtkWidget* widget, GtkBorder* margin, GtkBorder* frame, int* radius)
{
  GtkStyleContext* context = gtk_widget_get_style_context(widget);
  GtkStateFlags state = gtk_style_context_get_state(context);
  int border_width = gtk_container_get_border_width(GTK_CONTAINER(widget));
  GtkBorder border;

  gtk_style_context_get_margin(context, state, margin);
  gtk_style_context_get_padding(context, state, frame);
  gtk_style_context_get_border(context, state, &border);
  gtk_style_context_get(context, state, GTK_STYLE_PROPERTY_BORDER_RADIUS, radius, NULL);

  frame->left += border.left + border_width;
  frame->right += border.right + border_width;
  frame->top += border.top + border_width;
  frame->bottom += border.bottom + border_width;
}

static void iup_gtk_popover_get_body(GtkWidget* widget, GdkRectangle* body)
{
  GtkAllocation allocation;
  GtkBorder margin, frame;
  int radius;

  gtk_widget_get_allocation(widget, &allocation);
  iup_gtk_popover_get_frame(widget, &margin, &frame, &radius);

  body->x = margin.left;
  body->y = margin.top;
  body->width = allocation.width - margin.left - margin.right;
  body->height = allocation.height - margin.top - margin.bottom;
}

static void iup_gtk_popover_measure(GtkWidget* widget, GtkOrientation orientation, int for_size, int* minimum, int* natural)
{
  GtkWidget* child = gtk_bin_get_child(GTK_BIN(widget));
  GtkBorder margin, frame;
  int radius, min = 0, nat = 0, extra;

  iup_gtk_popover_get_frame(widget, &margin, &frame, &radius);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
  {
    if (child)
    {
      if (for_size < 0)
        gtk_widget_get_preferred_width(child, &min, &nat);
      else
        gtk_widget_get_preferred_width_for_height(child, MAX(0, for_size - margin.top - margin.bottom - frame.top - frame.bottom), &min, &nat);
    }
    extra = frame.left + frame.right + margin.left + margin.right;
  }
  else
  {
    if (child)
    {
      if (for_size < 0)
        gtk_widget_get_preferred_height(child, &min, &nat);
      else
        gtk_widget_get_preferred_height_for_width(child, MAX(0, for_size - margin.left - margin.right - frame.left - frame.right), &min, &nat);
    }
    extra = frame.top + frame.bottom + margin.top + margin.bottom;
  }

  *minimum = MAX(min, 2 * radius) + extra;
  *natural = MAX(nat, 2 * radius) + extra;
}

static void iup_gtk_popover_get_preferred_width(GtkWidget* widget, gint* minimum, gint* natural)
{
  if (IUP_GTK_POPOVER(widget)->has_arrow)
    GTK_WIDGET_CLASS(iup_gtk_popover_parent_class)->get_preferred_width(widget, minimum, natural);
  else
    iup_gtk_popover_measure(widget, GTK_ORIENTATION_HORIZONTAL, -1, minimum, natural);
}

static void iup_gtk_popover_get_preferred_height(GtkWidget* widget, gint* minimum, gint* natural)
{
  if (IUP_GTK_POPOVER(widget)->has_arrow)
    GTK_WIDGET_CLASS(iup_gtk_popover_parent_class)->get_preferred_height(widget, minimum, natural);
  else
    iup_gtk_popover_measure(widget, GTK_ORIENTATION_VERTICAL, -1, minimum, natural);
}

static void iup_gtk_popover_get_preferred_width_for_height(GtkWidget* widget, gint height, gint* minimum, gint* natural)
{
  if (IUP_GTK_POPOVER(widget)->has_arrow)
    GTK_WIDGET_CLASS(iup_gtk_popover_parent_class)->get_preferred_width_for_height(widget, height, minimum, natural);
  else
    iup_gtk_popover_measure(widget, GTK_ORIENTATION_HORIZONTAL, height, minimum, natural);
}

static void iup_gtk_popover_get_preferred_height_for_width(GtkWidget* widget, gint width, gint* minimum, gint* natural)
{
  if (IUP_GTK_POPOVER(widget)->has_arrow)
    GTK_WIDGET_CLASS(iup_gtk_popover_parent_class)->get_preferred_height_for_width(widget, width, minimum, natural);
  else
    iup_gtk_popover_measure(widget, GTK_ORIENTATION_VERTICAL, width, minimum, natural);
}

static void iup_gtk_popover_rounded_path(cairo_t* cr, const GdkRectangle* rect, int radius)
{
  double r = MIN(radius, MIN(rect->width, rect->height) / 2);
  double x = rect->x, y = rect->y, w = rect->width, h = rect->height;

  cairo_new_sub_path(cr);
  cairo_arc(cr, x + w - r, y + r, r, -G_PI_2, 0);
  cairo_arc(cr, x + w - r, y + h - r, r, 0, G_PI_2);
  cairo_arc(cr, x + r, y + h - r, r, G_PI_2, G_PI);
  cairo_arc(cr, x + r, y + r, r, G_PI, 3 * G_PI_2);
  cairo_close_path(cr);
}

static void iup_gtk_popover_update_shape(GtkWidget* widget)
{
  GdkWindow* window = gtk_widget_get_window(widget);
  cairo_surface_t* surface;
  cairo_region_t* region;
  GdkRectangle body;
  GtkBorder margin, frame;
  int radius;
  cairo_t* cr;

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY(gtk_widget_get_display(widget)))
    return;
#endif

  if (!window)
    return;

  iup_gtk_popover_get_frame(widget, &margin, &frame, &radius);
  iup_gtk_popover_get_body(widget, &body);

  surface = gdk_window_create_similar_surface(window, CAIRO_CONTENT_COLOR_ALPHA,
                                              gdk_window_get_width(window), gdk_window_get_height(window));
  cr = cairo_create(surface);
  cairo_set_source_rgba(cr, 0, 0, 0, 1);
  iup_gtk_popover_rounded_path(cr, &body, radius);
  cairo_fill(cr);
  cairo_destroy(cr);

  region = gdk_cairo_region_create_from_surface(surface);
  cairo_surface_destroy(surface);

  gtk_widget_shape_combine_region(widget, region);
  cairo_region_destroy(region);

  gdk_window_set_child_shapes(gtk_widget_get_parent_window(widget));
}

static void iup_gtk_popover_size_allocate(GtkWidget* widget, GtkAllocation* allocation)
{
  GtkWidget* child;

  if (IUP_GTK_POPOVER(widget)->has_arrow)
  {
    GTK_WIDGET_CLASS(iup_gtk_popover_parent_class)->size_allocate(widget, allocation);
    return;
  }

  gtk_widget_set_allocation(widget, allocation);

  child = gtk_bin_get_child(GTK_BIN(widget));
  if (child)
  {
    GtkAllocation child_alloc;
    GdkRectangle body;
    GtkBorder margin, frame;
    int radius;

    iup_gtk_popover_get_frame(widget, &margin, &frame, &radius);
    iup_gtk_popover_get_body(widget, &body);

    child_alloc.x = body.x + frame.left;
    child_alloc.y = body.y + frame.top;
    child_alloc.width = MAX(1, body.width - frame.left - frame.right);
    child_alloc.height = MAX(1, body.height - frame.top - frame.bottom);
    gtk_widget_size_allocate(child, &child_alloc);
  }

  if (gtk_widget_get_realized(widget))
  {
    gdk_window_move_resize(gtk_widget_get_window(widget), 0, 0, allocation->width, allocation->height);
    iup_gtk_popover_update_shape(widget);
  }

  gtk_widget_queue_draw(widget);
}

static void iup_gtk_popover_map(GtkWidget* widget)
{
  GTK_WIDGET_CLASS(iup_gtk_popover_parent_class)->map(widget);

  if (!IUP_GTK_POPOVER(widget)->has_arrow)
    iup_gtk_popover_update_shape(widget);
}

static gboolean iup_gtk_popover_draw(GtkWidget* widget, cairo_t* cr)
{
  GtkStyleContext* context;
  GtkWidget* child;
  GdkRectangle body;

  if (IUP_GTK_POPOVER(widget)->has_arrow)
    return GTK_WIDGET_CLASS(iup_gtk_popover_parent_class)->draw(widget, cr);

  context = gtk_widget_get_style_context(widget);
  iup_gtk_popover_get_body(widget, &body);

  gtk_render_background(context, cr, body.x, body.y, body.width, body.height);
  gtk_render_frame(context, cr, body.x, body.y, body.width, body.height);

  child = gtk_bin_get_child(GTK_BIN(widget));
  if (child)
    gtk_container_propagate_draw(GTK_CONTAINER(widget), child, cr);

  return GDK_EVENT_PROPAGATE;
}

static void iup_gtk_popover_class_init(iupGtkPopoverClass* _class)
{
  GtkWidgetClass* widget_class = (GtkWidgetClass*)_class;
  widget_class->get_preferred_width = iup_gtk_popover_get_preferred_width;
  widget_class->get_preferred_height = iup_gtk_popover_get_preferred_height;
  widget_class->get_preferred_width_for_height = iup_gtk_popover_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = iup_gtk_popover_get_preferred_height_for_width;
  widget_class->size_allocate = iup_gtk_popover_size_allocate;
  widget_class->map = iup_gtk_popover_map;
  widget_class->draw = iup_gtk_popover_draw;
}

static void iup_gtk_popover_set_has_arrow(GtkWidget* widget, gboolean has_arrow)
{
  iupGtkPopover* popover = IUP_GTK_POPOVER(widget);

  if (popover->has_arrow == has_arrow)
    return;

  popover->has_arrow = has_arrow;
  gtk_widget_queue_resize(widget);
}

static void gtkPopoverClosedCb(GtkPopover* popover, Ihandle* ih)
{
  IFni show_cb;

  (void)popover;

  iupAttribSetInt(ih, "_IUPGTK_POPOVER_SHOWN", 0);

  show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
  if (show_cb)
    show_cb(ih, IUP_HIDE);
}

static int gtkPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  GtkPopover* popover;

  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    if (!anchor || !anchor->handle)
      return 0;

    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    popover = (GtkPopover*)ih->handle;

    iup_gtk_popover_set_has_arrow(ih->handle, iupAttribGetBoolean(ih, "ARROW"));

    if (ih->firstchild)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih);
    }

    {
      int position = iupPopoverGetPosition(ih);
      int has_arrow = IUP_GTK_POPOVER(popover)->has_arrow;
      GtkPositionType gtk_pos;
      GdkRectangle pointing_to;
      GtkWidget* anchor_widget = (GtkWidget*)anchor->handle;
      GtkAllocation alloc;
      int min_w, req_w, min_h, req_h;

      gtk_widget_get_allocation(anchor_widget, &alloc);
      iup_gtk_popover_measure(GTK_WIDGET(popover), GTK_ORIENTATION_HORIZONTAL, -1, &min_w, &req_w);
      iup_gtk_popover_measure(GTK_WIDGET(popover), GTK_ORIENTATION_VERTICAL, req_w, &min_h, &req_h);

      switch (position)
      {
      case IUP_POPOVER_TOP:
      case IUP_POPOVER_TOPLEFT:
      case IUP_POPOVER_TOPRIGHT:
        gtk_pos = GTK_POS_TOP;
        break;
      case IUP_POPOVER_LEFT:
      case IUP_POPOVER_LEFTTOP:
      case IUP_POPOVER_LEFTBOTTOM:
        gtk_pos = GTK_POS_LEFT;
        break;
      case IUP_POPOVER_RIGHT:
      case IUP_POPOVER_RIGHTTOP:
      case IUP_POPOVER_RIGHTBOTTOM:
        gtk_pos = GTK_POS_RIGHT;
        break;
      default:
        gtk_pos = GTK_POS_BOTTOM;
        break;
      }

      gtk_popover_set_position(popover, gtk_pos);

      pointing_to.x = 0;
      pointing_to.y = 0;
      pointing_to.width = alloc.width;
      pointing_to.height = alloc.height;

      switch (position)
      {
      case IUP_POPOVER_BOTTOMLEFT:
      case IUP_POPOVER_TOPLEFT:
        pointing_to.width = has_arrow ? 1 : req_w;
        break;
      case IUP_POPOVER_BOTTOMRIGHT:
      case IUP_POPOVER_TOPRIGHT:
        pointing_to.width = has_arrow ? 1 : req_w;
        pointing_to.x = alloc.width - pointing_to.width;
        break;
      case IUP_POPOVER_LEFTTOP:
      case IUP_POPOVER_RIGHTTOP:
        pointing_to.height = has_arrow ? 1 : req_h;
        break;
      case IUP_POPOVER_LEFTBOTTOM:
      case IUP_POPOVER_RIGHTBOTTOM:
        pointing_to.height = has_arrow ? 1 : req_h;
        pointing_to.y = alloc.height - pointing_to.height;
        break;
      }

      pointing_to.x += iupAttribGetInt(ih, "OFFSETX");
      pointing_to.y += iupAttribGetInt(ih, "OFFSETY");

      gtk_popover_set_pointing_to(popover, &pointing_to);
    }

    {
      int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
      gtk_popover_set_modal(popover, autohide);
    }

#if GTK_CHECK_VERSION(3, 20, 0)
    gtk_popover_set_constrain_to(popover, GTK_POPOVER_CONSTRAINT_NONE);
#endif

#if GTK_CHECK_VERSION(3, 22, 0)
    gtk_popover_popup(popover);
#else
    gtk_widget_show(GTK_WIDGET(popover));
#endif

    iupAttribSetInt(ih, "_IUPGTK_POPOVER_SHOWN", 1);

    {
      IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
      if (show_cb)
        show_cb(ih, IUP_SHOW);
    }
  }
  else
  {
    if (ih->handle)
    {
#if GTK_CHECK_VERSION(3, 22, 0)
      gtk_popover_popdown((GtkPopover*)ih->handle);
#else
      gtk_widget_hide(ih->handle);
#endif
    }
  }

  return 0;
}

static char* gtkPopoverGetVisibleAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return "NO";
  /* gtk_widget_get_visible stays TRUE until the popdown animation ends */
  return iupStrReturnBoolean(iupAttribGetInt(ih, "_IUPGTK_POPOVER_SHOWN"));
}

static void gtkPopoverLayoutUpdateMethod(Ihandle* ih)
{
  GtkWidget* inner_parent = (GtkWidget*)iupAttribGet(ih, "_IUP_GTK_INNER_PARENT");

  if (inner_parent && ih->currentwidth > 0 && ih->currentheight > 0)
    gtk_widget_set_size_request(inner_parent, ih->currentwidth, ih->currentheight);

  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static void* gtkPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, "_IUP_GTK_INNER_PARENT");
}

static int gtkPopoverMapMethod(Ihandle* ih)
{
  GtkWidget* popover;
  GtkWidget* inner_parent;
  Ihandle* anchor;

  anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
  if (!anchor || !anchor->handle)
    return IUP_ERROR;

  popover = g_object_new(iup_gtk_popover_get_type(), "relative-to", (GtkWidget*)anchor->handle, NULL);
  if (!popover)
    return IUP_ERROR;

  ih->handle = popover;

  inner_parent = iupgtkNativeContainerNew(0);
  if (!inner_parent)
  {
    g_object_ref_sink(popover);
    g_object_unref(popover);
    return IUP_ERROR;
  }

  gtk_container_add(GTK_CONTAINER(popover), inner_parent);
  gtk_widget_show(inner_parent);
  iupAttribSet(ih, "_IUP_GTK_INNER_PARENT", (char*)inner_parent);

  g_signal_connect(G_OBJECT(popover), "closed", G_CALLBACK(gtkPopoverClosedCb), ih);

  /* Keep a reference to prevent destruction */
  g_object_ref_sink(popover);

  return IUP_NOERROR;
}

static void gtkPopoverUnMapMethod(Ihandle* ih)
{
  GtkWidget* popover = GTK_WIDGET(ih->handle);

  if (popover)
  {
    gtk_widget_hide(popover);
    g_object_unref(popover);
  }

  ih->handle = NULL;
}

#else

/***********************************************************************************
 * GTK2 fallback using popup window (no GtkPopover before 3.12)
 ***********************************************************************************/

static gboolean gtkPopoverButtonPressEvent(GtkWidget* widget, GdkEventButton* event, Ihandle* ih)
{
  int px, py, pw, ph;
  GtkAllocation alloc;

  (void)widget;

  if (!iupAttribGetBoolean(ih, "AUTOHIDE"))
    return FALSE;

  gtk_widget_get_allocation(ih->handle, &alloc);
  gdk_window_get_origin(gtk_widget_get_window(ih->handle), &px, &py);
  pw = alloc.width;
  ph = alloc.height;

  if (event->x_root < px || event->x_root > px + pw ||
      event->y_root < py || event->y_root > py + ph)
  {
    IupSetAttribute(ih, "VISIBLE", "NO");
    return TRUE;
  }

  return FALSE;
}

static void gtkPopoverParentStateEvent(GtkWidget* widget, GdkEventWindowState* event, Ihandle* ih)
{
  (void)widget;

  if (event->new_window_state & (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_WITHDRAWN))
  {
    if (ih->handle && gtk_widget_get_visible(ih->handle))
      gtk_widget_hide(ih->handle);
  }
}

static int gtkPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  GtkWindow* popup;

  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    GtkWidget* anchor_widget;
    GtkWidget* toplevel;
    int ax, ay, aw, ah;
    int x, y;

    if (!anchor || !anchor->handle)
      return 0;

    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    popup = (GtkWindow*)ih->handle;
    anchor_widget = (GtkWidget*)anchor->handle;

    toplevel = gtk_widget_get_toplevel(anchor_widget);
    if (GTK_IS_WINDOW(toplevel))
    {
      gtk_window_set_transient_for(popup, GTK_WINDOW(toplevel));

      if (!iupAttribGet(ih, "_IUP_FALLBACK_PARENT_CONNECTED"))
      {
        g_signal_connect(G_OBJECT(toplevel), "window-state-event", G_CALLBACK(gtkPopoverParentStateEvent), ih);
        iupAttribSet(ih, "_IUP_FALLBACK_PARENT_CONNECTED", "1");
      }
    }

    {
      GtkAllocation alloc;
      gtk_widget_get_allocation(anchor_widget, &alloc);
      aw = alloc.width;
      ah = alloc.height;

      ax = 0;
      ay = 0;
      iupdrvClientToScreen(anchor, &ax, &ay);
    }

    if (ih->firstchild)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih);
    }

    iupPopoverCalcPosition(ih,
      ax, ay, aw, ah,
      ih->currentwidth, ih->currentheight,
      &x, &y);

    if (ih->currentwidth > 0 && ih->currentheight > 0)
      gtk_window_resize(popup, ih->currentwidth, ih->currentheight);

    gtk_window_move(popup, x, y);
    gtk_widget_show(GTK_WIDGET(popup));

    if (iupAttribGetBoolean(ih, "AUTOHIDE"))
      gtk_grab_add(GTK_WIDGET(popup));

    {
      IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
      if (show_cb)
        show_cb(ih, IUP_SHOW);
    }
  }
  else
  {
    if (ih->handle)
    {
      gtk_grab_remove(GTK_WIDGET(ih->handle));
      gtk_widget_hide(ih->handle);

      {
        IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
        if (show_cb)
          show_cb(ih, IUP_HIDE);
      }
    }
  }

  return 0;
}

static char* gtkPopoverGetVisibleAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return "NO";
  return iupStrReturnBoolean(gtk_widget_get_visible(ih->handle));
}

static void gtkPopoverLayoutUpdateMethod(Ihandle* ih)
{
  GtkWidget* inner_parent = (GtkWidget*)iupAttribGet(ih, "_IUP_GTK_INNER_PARENT");

  if (inner_parent && ih->currentwidth > 0 && ih->currentheight > 0)
    gtk_widget_set_size_request(inner_parent, ih->currentwidth, ih->currentheight);

  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static void* gtkPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, "_IUP_GTK_INNER_PARENT");
}

static int gtkPopoverMapMethod(Ihandle* ih)
{
  GtkWidget* popup;
  GtkWidget* frame;
  GtkWidget* inner_parent;

  popup = gtk_window_new(GTK_WINDOW_POPUP);
  if (!popup)
    return IUP_ERROR;

  ih->handle = popup;

  gtk_window_set_decorated(GTK_WINDOW(popup), FALSE);
  gtk_window_set_type_hint(GTK_WINDOW(popup), GDK_WINDOW_TYPE_HINT_POPUP_MENU);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
  gtk_container_add(GTK_CONTAINER(popup), frame);
  gtk_widget_show(frame);

  inner_parent = iupgtkNativeContainerNew(0);
  if (!inner_parent)
  {
    gtk_widget_destroy(popup);
    return IUP_ERROR;
  }

  gtk_container_add(GTK_CONTAINER(frame), inner_parent);
  gtk_widget_show(inner_parent);
  iupAttribSet(ih, "_IUP_GTK_INNER_PARENT", (char*)inner_parent);

  g_signal_connect(G_OBJECT(popup), "button-press-event", G_CALLBACK(gtkPopoverButtonPressEvent), ih);

  return IUP_NOERROR;
}

static void gtkPopoverUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
    gtk_widget_destroy(ih->handle);

  ih->handle = NULL;
}

#endif /* GTK >= 3.12 */

IUP_SDK_API void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = gtkPopoverMapMethod;
  ic->UnMap = gtkPopoverUnMapMethod;
  ic->LayoutUpdate = gtkPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = gtkPopoverGetInnerNativeContainerHandleMethod;

  /* NOT_MAPPED because the setter handles mapping */
  iupClassRegisterAttribute(ic, "VISIBLE", gtkPopoverGetVisibleAttrib, gtkPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
