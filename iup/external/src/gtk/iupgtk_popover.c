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


static void gtkPopoverClosedCb(GtkPopover *popover, Ihandle* ih)
{
  IFni show_cb;

  (void)popover;

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

    /* Set autohide (modal in GTK3 terms) */
    {
      int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
      gtk_popover_set_modal(popover, autohide);
    }

#if GTK_CHECK_VERSION(3, 20, 0)
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
  GtkWidget* popover;
  GtkWidget* inner_parent;
  Ihandle* anchor;

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

void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = gtkPopoverMapMethod;
  ic->UnMap = gtkPopoverUnMapMethod;
  ic->LayoutUpdate = gtkPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = gtkPopoverGetInnerNativeContainerHandleMethod;

  /* Override VISIBLE attribute, NOT_MAPPED because setter handles mapping */
  iupClassRegisterAttribute(ic, "VISIBLE", gtkPopoverGetVisibleAttrib, gtkPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
