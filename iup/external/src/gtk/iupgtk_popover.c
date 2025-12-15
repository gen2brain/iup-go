/** \file
 * \brief IupPopover control for GTK
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>

#include <stdlib.h>
#include <stdio.h>
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
#include "iup_drvfont.h"
#include "iup_childtree.h"
#include "iup_class.h"
#include "iup_register.h"

#include "iupgtk_drv.h"

#include "iup_popover.h"


/***********************************************************************************
 * Fallback implementation using popup window (no arrow, works with AUTOHIDE=NO)
 ***********************************************************************************/

static gboolean gtkPopoverFallbackFocusOutEvent(GtkWidget* widget, GdkEventFocus* event, Ihandle* ih)
{
  (void)widget;
  (void)event;

  if (iupAttribGetBoolean(ih, "AUTOHIDE"))
  {
    IupSetAttribute(ih, "VISIBLE", "NO");
  }

  return FALSE;
}

static void gtkPopoverFallbackParentStateEvent(GtkWidget* widget, GdkEventWindowState* event, Ihandle* ih)
{
  (void)widget;

  if (event->new_window_state & (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_WITHDRAWN))
  {
    if (ih->handle && gtk_widget_get_visible(ih->handle))
      gtk_widget_hide(ih->handle);
  }
}

static int gtkPopoverFallbackSetVisibleAttrib(Ihandle* ih, const char* value)
{
  GtkWindow* popup;

  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    GtkWidget* anchor_widget;
    GtkWidget* toplevel;
    int ax, ay, aw, ah;
    int x, y;
    gint wx, wy;

    if (!anchor || !anchor->handle)
      return 0;

    /* Map if not yet mapped */
    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    popup = (GtkWindow*)ih->handle;
    anchor_widget = (GtkWidget*)anchor->handle;

    /* Get toplevel window for transient relationship */
    toplevel = gtk_widget_get_toplevel(anchor_widget);
    if (GTK_IS_WINDOW(toplevel))
    {
      gtk_window_set_transient_for(popup, GTK_WINDOW(toplevel));

      /* Connect to parent window state changes to hide when minimized */
      if (!iupAttribGet(ih, "_IUP_FALLBACK_PARENT_CONNECTED"))
      {
        g_signal_connect(G_OBJECT(toplevel), "window-state-event", G_CALLBACK(gtkPopoverFallbackParentStateEvent), ih);
        iupAttribSet(ih, "_IUP_FALLBACK_PARENT_CONNECTED", "1");
      }
    }

    /* Get anchor widget screen position */
    {
      GtkAllocation alloc;
      Ihandle* ih_anchor = anchor;

      gtk_widget_get_allocation(anchor_widget, &alloc);
      aw = alloc.width;
      ah = alloc.height;

      ax = 0;
      ay = 0;
      iupdrvClientToScreen(ih_anchor, &ax, &ay);
    }

    /* Layout the child before positioning */
    if (ih->firstchild)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih);
    }

    /* Position based on POSITION attribute */
    {
      const char* pos = iupAttribGetStr(ih, "POSITION");

      if (iupStrEqualNoCase(pos, "TOP"))
      {
        x = ax;
        y = ay - ih->currentheight;
      }
      else if (iupStrEqualNoCase(pos, "LEFT"))
      {
        x = ax - ih->currentwidth;
        y = ay;
      }
      else if (iupStrEqualNoCase(pos, "RIGHT"))
      {
        x = ax + aw;
        y = ay;
      }
      else /* BOTTOM */
      {
        x = ax;
        y = ay + ah;
      }
    }

    if (ih->currentwidth > 0 && ih->currentheight > 0)
      gtk_window_resize(popup, ih->currentwidth, ih->currentheight);

    gtk_window_move(popup, x, y);
    gtk_widget_show(GTK_WIDGET(popup));

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

static char* gtkPopoverFallbackGetVisibleAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return "NO";
  return iupStrReturnBoolean(gtk_widget_get_visible(ih->handle));
}

static void gtkPopoverFallbackLayoutUpdateMethod(Ihandle* ih)
{
  GtkWidget* inner_parent = (GtkWidget*)iupAttribGet(ih, "_IUP_GTK_INNER_PARENT");

  if (inner_parent && ih->currentwidth > 0 && ih->currentheight > 0)
  {
    gtk_widget_set_size_request(inner_parent, ih->currentwidth, ih->currentheight);
  }

  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static void* gtkPopoverFallbackGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, "_IUP_GTK_INNER_PARENT");
}

static int gtkPopoverFallbackMapMethod(Ihandle* ih)
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

  /* Create a frame to provide border */
  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
  gtk_container_add(GTK_CONTAINER(popup), frame);
  gtk_widget_show(frame);

  /* Create inner container for IUP children */
  inner_parent = iupgtkNativeContainerNew(0);
  if (!inner_parent)
  {
    gtk_widget_destroy(popup);
    return IUP_ERROR;
  }

  gtk_container_add(GTK_CONTAINER(frame), inner_parent);
  gtk_widget_show(inner_parent);
  iupAttribSet(ih, "_IUP_GTK_INNER_PARENT", (char*)inner_parent);

  /* Connect focus-out for autohide */
  g_signal_connect(G_OBJECT(popup), "focus-out-event", G_CALLBACK(gtkPopoverFallbackFocusOutEvent), ih);

  /* Mark as using fallback */
  iupAttribSet(ih, "_IUP_GTK_POPOVER_FALLBACK", "1");

  return IUP_NOERROR;
}

static void gtkPopoverFallbackUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
  {
    gtk_widget_destroy(ih->handle);
  }

  ih->handle = NULL;
}

/***********************************************************************************
 * Native GtkPopover implementation (GTK >= 3.12, with arrow, AUTOHIDE=YES only)
 ***********************************************************************************/

#if GTK_CHECK_VERSION(3, 12, 0)

static void gtkPopoverClosedCb(GtkPopover *popover, Ihandle* ih)
{
  IFni show_cb;

  (void)popover;

  show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
  if (show_cb)
    show_cb(ih, IUP_HIDE);
}

static int gtkPopoverNativeSetVisibleAttrib(Ihandle* ih, const char* value)
{
  GtkPopover* popover;

  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    if (!anchor || !anchor->handle)
      return 0;

    /* Map if not yet mapped */
    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    popover = (GtkPopover*)ih->handle;

    /* Set position based on POSITION attribute */
    {
      const char* pos = iupAttribGetStr(ih, "POSITION");
      GtkPositionType gtk_pos = GTK_POS_BOTTOM;

      if (iupStrEqualNoCase(pos, "TOP"))
        gtk_pos = GTK_POS_TOP;
      else if (iupStrEqualNoCase(pos, "LEFT"))
        gtk_pos = GTK_POS_LEFT;
      else if (iupStrEqualNoCase(pos, "RIGHT"))
        gtk_pos = GTK_POS_RIGHT;
      else
        gtk_pos = GTK_POS_BOTTOM;

      gtk_popover_set_position(popover, gtk_pos);
    }

    /* Native GtkPopover always uses modal (AUTOHIDE=YES) */
    gtk_popover_set_modal(popover, TRUE);

#if GTK_CHECK_VERSION(3, 20, 0)
    /* Allow popover to extend outside window boundaries */
    gtk_popover_set_constrain_to(popover, GTK_POPOVER_CONSTRAINT_NONE);
#endif

    /* Layout the child before showing */
    if (ih->firstchild)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih);
    }

    gtk_popover_popup(popover);

    {
      IFni show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
      if (show_cb)
        show_cb(ih, IUP_SHOW);
    }
  }
  else
  {
    if (ih->handle)
      gtk_widget_hide(ih->handle);
  }

  return 0;
}

static char* gtkPopoverNativeGetVisibleAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return "NO";
  return iupStrReturnBoolean(gtk_widget_get_visible(ih->handle));
}

static void gtkPopoverNativeLayoutUpdateMethod(Ihandle* ih)
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

static int gtkPopoverNativeMapMethod(Ihandle* ih)
{
  GtkWidget* popover;
  GtkWidget* inner_parent;
  Ihandle* anchor;

  /* GTK3 popover should be anchored before children can be mapped properly */
  anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
  if (!anchor || !anchor->handle)
    return IUP_ERROR;

  popover = gtk_popover_new((GtkWidget*)anchor->handle);
  if (!popover)
    return IUP_ERROR;

  ih->handle = popover;

  /* Create inner container for IUP children */
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

  /* Connect closed signal */
  g_signal_connect(G_OBJECT(popover), "closed", G_CALLBACK(gtkPopoverClosedCb), ih);

  /* Keep a reference to prevent destruction */
  g_object_ref_sink(popover);

  return IUP_NOERROR;
}

static void gtkPopoverNativeUnMapMethod(Ihandle* ih)
{
  GtkWidget* popover = GTK_WIDGET(ih->handle);

  if (popover)
  {
    gtk_widget_hide(popover);
    g_object_unref(popover);
  }

  ih->handle = NULL;
}

#endif /* GTK >= 3.12 */

/***********************************************************************************
 * Dispatch functions - choose between native and fallback based on attributes
 ***********************************************************************************/

static int gtkPopoverUseFallback(Ihandle* ih)
{
  /* Check if already mapped as fallback */
  if (iupAttribGet(ih, "_IUP_GTK_POPOVER_FALLBACK"))
    return 1;

  /* If not mapped yet, check attributes to determine which to use */
  if (!ih->handle)
  {
    int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
    int arrow = iupAttribGetBoolean(ih, "ARROW");
#if GTK_CHECK_VERSION(3, 12, 0)
    /* Use fallback when AUTOHIDE=NO or ARROW=NO */
    if (!autohide || !arrow)
      return 1;
    return 0;
#else
    (void)autohide;
    (void)arrow;
    return 1;
#endif
  }

#if GTK_CHECK_VERSION(3, 12, 0)
  /* Already mapped as native */
  return 0;
#else
  return 1;
#endif
}

static int gtkPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  if (gtkPopoverUseFallback(ih))
    return gtkPopoverFallbackSetVisibleAttrib(ih, value);
#if GTK_CHECK_VERSION(3, 12, 0)
  else
    return gtkPopoverNativeSetVisibleAttrib(ih, value);
#else
  return gtkPopoverFallbackSetVisibleAttrib(ih, value);
#endif
}

static char* gtkPopoverGetVisibleAttrib(Ihandle* ih)
{
  if (gtkPopoverUseFallback(ih))
    return gtkPopoverFallbackGetVisibleAttrib(ih);
#if GTK_CHECK_VERSION(3, 12, 0)
  else
    return gtkPopoverNativeGetVisibleAttrib(ih);
#else
  return gtkPopoverFallbackGetVisibleAttrib(ih);
#endif
}

static void gtkPopoverLayoutUpdateMethod(Ihandle* ih)
{
  if (gtkPopoverUseFallback(ih))
    gtkPopoverFallbackLayoutUpdateMethod(ih);
#if GTK_CHECK_VERSION(3, 12, 0)
  else
    gtkPopoverNativeLayoutUpdateMethod(ih);
#else
  gtkPopoverFallbackLayoutUpdateMethod(ih);
#endif
}

static void* gtkPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, "_IUP_GTK_INNER_PARENT");
}

static int gtkPopoverMapMethod(Ihandle* ih)
{
  int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
  int arrow = iupAttribGetBoolean(ih, "ARROW");

#if GTK_CHECK_VERSION(3, 12, 0)
  /* Use native GtkPopover only when AUTOHIDE=YES and ARROW=YES (defaults) */
  if (autohide && arrow)
    return gtkPopoverNativeMapMethod(ih);
  else
    return gtkPopoverFallbackMapMethod(ih);
#else
  (void)autohide;
  (void)arrow;
  return gtkPopoverFallbackMapMethod(ih);
#endif
}

static void gtkPopoverUnMapMethod(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUP_GTK_POPOVER_FALLBACK"))
    gtkPopoverFallbackUnMapMethod(ih);
#if GTK_CHECK_VERSION(3, 12, 0)
  else
    gtkPopoverNativeUnMapMethod(ih);
#else
  gtkPopoverFallbackUnMapMethod(ih);
#endif
}

void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = gtkPopoverMapMethod;
  ic->UnMap = gtkPopoverUnMapMethod;
  ic->LayoutUpdate = gtkPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = gtkPopoverGetInnerNativeContainerHandleMethod;

  /* Override VISIBLE attribute, NOT_MAPPED because setter handles mapping */
  iupClassRegisterAttribute(ic, "VISIBLE", gtkPopoverGetVisibleAttrib, gtkPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
