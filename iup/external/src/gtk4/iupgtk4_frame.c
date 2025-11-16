/** \file
 * \brief Frame Control for GTK4
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

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_frame.h"

#include "iupgtk4_drv.h"

void iupdrvFrameGetDecorOffset(Ihandle* ih, int* x, int* y)
{
  (void)ih;
  *x = 2;
  *y = 2;
}

int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvFrameGetTitleHeight(Ihandle* ih, int* h)
{
  (void)ih;
  (void)h;
  return 0;
}

int iupdrvFrameGetDecorSize(Ihandle* ih, int* w, int* h)
{
  (void)ih;
  (void)w;
  (void)h;
  return 0;
}

static int gtk4FrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    GtkFrame* frame = (GtkFrame*)ih->handle;
    gtk_frame_set_label(frame, iupgtk4StrConvertToSystem(value));
    return 1;
  }
  return 0;
}

static int gtk4FrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  GtkWidget* widget = gtk_frame_get_label_widget((GtkFrame*)ih->handle);

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
  {
    value = iupBaseNativeParentGetBgColor(ih);
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (widget)
    iupgtk4SetBgColor(widget, r, g, b);

  widget = gtk_frame_get_child((GtkFrame*)ih->handle);
  if (widget)
    iupgtk4SetBgColor(widget, r, g, b);

  if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    return 1;
  else
    return 0;
}

static int gtk4FrameSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  GtkWidget* label = gtk_frame_get_label_widget((GtkFrame*)ih->handle);
  if (!label) return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetFgColor(label, r, g, b);

  return 1;
}

static int gtk4FrameSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    GtkWidget* label = gtk_frame_get_label_widget((GtkFrame*)ih->handle);
    if (label)
      iupgtk4UpdateWidgetFont(ih, (GtkWidget*)label);
  }
  return 1;
}

static void* gtk4FrameGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return (void*)gtk_frame_get_child((GtkFrame*)ih->handle);
}

static int gtk4FrameMapMethod(Ihandle* ih)
{
  char* title;
  GtkWidget* inner_parent;
  GtkCssProvider* provider;
  GtkStyleContext* context;

  if (!ih->parent)
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");

  ih->handle = gtk_frame_new(NULL);
  if (!ih->handle)
    return IUP_ERROR;

  if (title)
  {
    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
    gtk_frame_set_label((GtkFrame*)ih->handle, iupgtk4StrConvertToSystem(title));
  }
  else
  {
    if (iupAttribGet(ih, "BGCOLOR") || iupAttribGet(ih, "BACKCOLOR"))
      iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");
  }

  inner_parent = iupgtk4NativeContainerNew();

  gtk_frame_set_child((GtkFrame*)ih->handle, inner_parent);

  iupgtk4AddToParent(ih);

  gtk_widget_realize(ih->handle);

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    gtk4FrameSetBgColorAttrib(ih, NULL);

  return IUP_NOERROR;
}

void iupdrvFrameInitClass(Iclass* ic)
{
  ic->Map = gtk4FrameMapMethod;
  ic->GetInnerNativeContainerHandle = gtk4FrameGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, gtk4FrameSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, gtk4FrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", iupFrameGetBgColorAttrib, gtk4FrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SUNKEN", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtk4FrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtk4FrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
}
