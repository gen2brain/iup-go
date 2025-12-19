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

static void gtk4FrameMeasureDecor(int has_title, int* decor_w, int* decor_h)
{
  GtkWidget* temp_frame = gtk_frame_new(NULL);
  GtkWidget* temp_child = gtk_fixed_new();

  if (has_title)
    gtk_frame_set_label(GTK_FRAME(temp_frame), "Tj");

  gtk_frame_set_child(GTK_FRAME(temp_frame), temp_child);
  gtk_widget_set_size_request(temp_child, 100, 100);

  int frame_min_w, frame_nat_w, frame_min_h, frame_nat_h;
  gtk_widget_measure(temp_frame, GTK_ORIENTATION_HORIZONTAL, -1, &frame_min_w, &frame_nat_w, NULL, NULL);
  gtk_widget_measure(temp_frame, GTK_ORIENTATION_VERTICAL, -1, &frame_min_h, &frame_nat_h, NULL, NULL);

  int child_min_w, child_nat_w, child_min_h, child_nat_h;
  gtk_widget_measure(temp_child, GTK_ORIENTATION_HORIZONTAL, -1, &child_min_w, &child_nat_w, NULL, NULL);
  gtk_widget_measure(temp_child, GTK_ORIENTATION_VERTICAL, -1, &child_min_h, &child_nat_h, NULL, NULL);

  *decor_w = frame_nat_w - child_nat_w;
  *decor_h = frame_nat_h - child_nat_h;

  if (*decor_w < 0) *decor_w = 0;
  if (*decor_h < 0) *decor_h = 0;

  g_object_ref_sink(temp_frame);
  g_object_unref(temp_frame);
}

void iupdrvFrameGetDecorOffset(Ihandle* ih, int* x, int* y)
{
  static int measured = 0;
  static int offset_x = 0, offset_y = 0;
  (void)ih;

  if (!measured)
  {
    int decor_w, decor_h;
    gtk4FrameMeasureDecor(0, &decor_w, &decor_h);
    offset_x = decor_w / 2;
    offset_y = decor_h / 2;
    measured = 1;
  }

  *x = offset_x;
  *y = offset_y;
}

int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvFrameGetTitleHeight(Ihandle* ih, int* h)
{
  const char* title = iupAttribGet(ih, "TITLE");
  if (!title || !*title)
  {
    *h = 0;
    return 1;
  }

  /* If frame is mapped, measure the actual label widget */
  if (ih->handle)
  {
    GtkFrame* frame = (GtkFrame*)ih->handle;
    GtkWidget* label_widget = gtk_frame_get_label_widget(frame);
    if (label_widget)
    {
      int min_height, nat_height;
      gtk_widget_measure(label_widget, GTK_ORIENTATION_VERTICAL, -1, &min_height, &nat_height, NULL, NULL);
      *h = nat_height;
      return 1;
    }
  }

  /* Fallback: Create a temporary GtkLabel to measure title height */
  GtkWidget* temp_label = gtk_label_new(title);

  int min_height, nat_height;
  gtk_widget_measure(temp_label, GTK_ORIENTATION_VERTICAL, -1, &min_height, &nat_height, NULL, NULL);

  *h = nat_height;

  /* Cleanup */
  g_object_ref_sink(temp_label);
  g_object_unref(temp_label);

  return 1;
}

int iupdrvFrameGetDecorSize(Ihandle* ih, int* w, int* h)
{
  static int titled_measured = 0, untitled_measured = 0;
  static int titled_w = 0, titled_h = 0;
  static int untitled_w = 0, untitled_h = 0;
  const char* title = iupAttribGet(ih, "TITLE");
  int has_title = (title && *title) ? 1 : 0;

  if (has_title)
  {
    if (!titled_measured)
    {
      gtk4FrameMeasureDecor(1, &titled_w, &titled_h);
      titled_measured = 1;
    }
    *w = titled_w;
    *h = titled_h;
  }
  else
  {
    if (!untitled_measured)
    {
      gtk4FrameMeasureDecor(0, &untitled_w, &untitled_h);
      untitled_measured = 1;
    }
    *w = untitled_w;
    *h = untitled_h;
  }

  return 1;
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
