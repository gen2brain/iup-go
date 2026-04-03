/** \file
 * \brief IupPopover control for GTK4
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

#include "iupgtk4_drv.h"

#include "iup_popover.h"


static void gtk4PopoverClosedCb(GtkPopover *popover, Ihandle* ih)
{
  IFni show_cb;

  (void)popover;

  show_cb = (IFni)IupGetCallback(ih, "SHOW_CB");
  if (show_cb)
    show_cb(ih, IUP_HIDE);
}

static void gtk4PopoverAnchorDestroyCb(GtkWidget* anchor, Ihandle* ih)
{
  (void)anchor;

  /* Anchor is being destroyed, unparent the popover first */
  if (ih->handle && GTK_IS_WIDGET(ih->handle))
  {
    gtk_widget_unparent((GtkWidget*)ih->handle);
    ih->handle = NULL;
  }
}

static int gtk4PopoverSetVisibleAttrib(Ihandle* ih, const char* value)
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

    /* Set arrow visibility */
    {
      int show_arrow = iupAttribGetBoolean(ih, "ARROW");
      gtk_popover_set_has_arrow(popover, show_arrow);
    }

    /* Layout the child before computing offsets (need popover size) */
    if (ih->firstchild && ih->firstchild->handle)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih->firstchild);
    }

    /* Set position based on POSITION attribute */
    {
      int position = iupPopoverGetPosition(ih);
      GtkPositionType gtk_pos;
      GdkRectangle pointing_to;
      int use_pointing_to = 0;
      GtkWidget* anchor_widget = (GtkWidget*)anchor->handle;
      int aw = gtk_widget_get_width(anchor_widget);
      int ah = gtk_widget_get_height(anchor_widget);

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

      switch (position)
      {
      case IUP_POPOVER_BOTTOMLEFT:
      case IUP_POPOVER_TOPLEFT:
        pointing_to.x = 0;
        pointing_to.y = 0;
        pointing_to.width = 1;
        pointing_to.height = ah;
        use_pointing_to = 1;
        break;
      case IUP_POPOVER_BOTTOMRIGHT:
      case IUP_POPOVER_TOPRIGHT:
        pointing_to.x = aw - 1;
        pointing_to.y = 0;
        pointing_to.width = 1;
        pointing_to.height = ah;
        use_pointing_to = 1;
        break;
      case IUP_POPOVER_LEFTTOP:
      case IUP_POPOVER_RIGHTTOP:
        pointing_to.x = 0;
        pointing_to.y = 0;
        pointing_to.width = aw;
        pointing_to.height = 1;
        use_pointing_to = 1;
        break;
      case IUP_POPOVER_LEFTBOTTOM:
      case IUP_POPOVER_RIGHTBOTTOM:
        pointing_to.x = 0;
        pointing_to.y = ah - 1;
        pointing_to.width = aw;
        pointing_to.height = 1;
        use_pointing_to = 1;
        break;
      }

      {
        int offsetx = iupAttribGetInt(ih, "OFFSETX");
        int offsety = iupAttribGetInt(ih, "OFFSETY");
        if (offsetx != 0 || offsety != 0)
        {
          if (!use_pointing_to)
          {
            pointing_to.x = 0;
            pointing_to.y = 0;
            pointing_to.width = aw;
            pointing_to.height = ah;
            use_pointing_to = 1;
          }
          pointing_to.x += offsetx;
          pointing_to.y += offsety;
        }
      }

      gtk_popover_set_position(popover, gtk_pos);
      gtk_popover_set_offset(popover, 0, 0);

      if (use_pointing_to)
        gtk_popover_set_pointing_to(popover, &pointing_to);
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
    {
      popover = (GtkPopover*)ih->handle;
      gtk_popover_popdown(popover);
    }
  }

  return 0;
}

static char* gtk4PopoverGetVisibleAttrib(Ihandle* ih)
{
  GtkPopover* popover;

  if (!ih->handle)
    return "NO";

  popover = (GtkPopover*)ih->handle;
  if (!GTK_IS_WIDGET(popover))
    return "NO";

  return iupStrReturnBoolean(gtk_widget_get_visible(GTK_WIDGET(popover)));
}

static void gtk4PopoverLayoutUpdateMethod(Ihandle* ih)
{
  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static void* gtk4PopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, "_IUP_GTK4_INNER_PARENT");
}

static int gtk4PopoverMapMethod(Ihandle* ih)
{
  GtkWidget* popover;
  GtkWidget* inner_parent;
  Ihandle* anchor;

  /* GTK4 popover must be parented before children can be mapped/realized */
  anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
  if (!anchor || !anchor->handle || !GTK_IS_WIDGET(anchor->handle))
    return IUP_ERROR;

  popover = gtk_popover_new();
  if (!popover)
    return IUP_ERROR;

  ih->handle = popover;

  /* Set autohide before parenting */
  {
    int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
    gtk_popover_set_autohide(GTK_POPOVER(popover), autohide);
  }

  /* Parent to anchor, required before children can be mapped */
  gtk_widget_set_parent(popover, (GtkWidget*)anchor->handle);

  /* Create inner container for IUP children */
  inner_parent = iupgtk4NativeContainerNew();
  if (!inner_parent)
  {
    gtk_widget_unparent(popover);
    return IUP_ERROR;
  }

  iupgtk4NativeContainerSetIhandle(inner_parent, ih);
  gtk_popover_set_child(GTK_POPOVER(popover), inner_parent);
  iupAttribSet(ih, "_IUP_GTK4_INNER_PARENT", (char*)inner_parent);

  /* Connect closed signal */
  g_signal_connect(G_OBJECT(popover), "closed", G_CALLBACK(gtk4PopoverClosedCb), ih);

  /* Connect to anchor's destroy signal to unparent popover before anchor is finalized */
  g_signal_connect(G_OBJECT(anchor->handle), "destroy", G_CALLBACK(gtk4PopoverAnchorDestroyCb), ih);

  return IUP_NOERROR;
}

static void gtk4PopoverUnMapMethod(Ihandle* ih)
{
  GtkWidget* popover = (GtkWidget*)ih->handle;
  Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");

  /* Disconnect from anchor's destroy signal */
  if (anchor && anchor->handle && GTK_IS_WIDGET(anchor->handle))
    g_signal_handlers_disconnect_by_func(anchor->handle, gtk4PopoverAnchorDestroyCb, ih);

  if (popover && GTK_IS_WIDGET(popover))
    gtk_widget_unparent(popover);

  ih->handle = NULL;
}

IUP_SDK_API void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = gtk4PopoverMapMethod;
  ic->UnMap = gtk4PopoverUnMapMethod;
  ic->LayoutUpdate = gtk4PopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = gtk4PopoverGetInnerNativeContainerHandleMethod;

  /* Override VISIBLE attribute, NOT_MAPPED because setter handles mapping */
  iupClassRegisterAttribute(ic, "VISIBLE", gtk4PopoverGetVisibleAttrib, gtk4PopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
