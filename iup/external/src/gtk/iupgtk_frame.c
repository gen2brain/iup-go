/** \file
 * \brief Frame Control
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

#include "iupgtk_drv.h"


#if GTK_CHECK_VERSION(3, 0, 0)
static void gtkFrameMeasureDecor(int has_title, int* decor_w, int* decor_h, int* title_h)
{
  GtkWidget* temp_window = gtk_offscreen_window_new();
  GtkWidget* temp_frame = gtk_frame_new(NULL);
  GtkWidget* temp_child = gtk_fixed_new();
  GtkRequisition frame_req, child_req;

  if (has_title)
    gtk_frame_set_label(GTK_FRAME(temp_frame), "Tj");

  gtk_container_add(GTK_CONTAINER(temp_frame), temp_child);
  gtk_container_set_border_width(GTK_CONTAINER(temp_frame), 2);
  gtk_widget_set_size_request(temp_child, 100, 100);

  gtk_container_add(GTK_CONTAINER(temp_window), temp_frame);
  gtk_widget_show_all(temp_window);

  gtk_widget_get_preferred_size(temp_frame, NULL, &frame_req);
  gtk_widget_get_preferred_size(temp_child, NULL, &child_req);

  *decor_w = frame_req.width - child_req.width;
  *decor_h = frame_req.height - child_req.height;
  *title_h = 0;

  if (has_title)
  {
    GtkWidget* label = gtk_frame_get_label_widget(GTK_FRAME(temp_frame));
    if (label)
    {
      GtkRequisition label_req;
      gtk_widget_get_preferred_size(label, NULL, &label_req);
      *title_h = label_req.height;
    }
  }

  if (*decor_w < 0) *decor_w = 4;
  if (*decor_h < 0) *decor_h = 4;

  gtk_widget_destroy(temp_window);
}
#endif

void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  static int measured = 0;
  static int offset_x = 0, offset_y = 0;
  (void)ih;

  if (!measured)
  {
    int decor_w, decor_h, title_h;
    gtkFrameMeasureDecor(0, &decor_w, &decor_h, &title_h);
    offset_x = decor_w / 2;
    offset_y = decor_h / 2;
    measured = 1;
  }

  *x = offset_x;
  *y = offset_y;
#else
  (void)ih;
  *x = 2;
  *y = 2;
#endif
}

int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  static int measured = 0;
  static int cached_title_h = 0;

  if (ih->handle)
  {
    GtkWidget* label = gtk_frame_get_label_widget((GtkFrame*)ih->handle);
    if (label)
    {
      GtkRequisition req;
      gtk_widget_get_preferred_size(label, NULL, &req);
      *h = req.height;
      return 1;
    }
  }

  if (!measured)
  {
    int decor_w, decor_h;
    gtkFrameMeasureDecor(1, &decor_w, &decor_h, &cached_title_h);
    measured = 1;
  }

  if (cached_title_h > 0)
  {
    *h = cached_title_h;
    return 1;
  }

  return 0;
#else
  (void)ih;
  (void)h;
  return 0;
#endif
}

int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  static int titled_measured = 0, untitled_measured = 0;
  static int titled_w = 0, titled_h = 0;
  static int untitled_w = 0, untitled_h = 0;
  const char* title = iupAttribGet(ih, "TITLE");
  int has_title = (title && *title) ? 1 : 0;

  if (has_title)
  {
    if (!titled_measured)
    {
      int title_h;
      gtkFrameMeasureDecor(1, &titled_w, &titled_h, &title_h);
      titled_measured = 1;
    }
    *w = titled_w;
    *h = titled_h;
  }
  else
  {
    if (!untitled_measured)
    {
      int title_h;
      gtkFrameMeasureDecor(0, &untitled_w, &untitled_h, &title_h);
      untitled_measured = 1;
    }
    *w = untitled_w;
    *h = untitled_h;
  }

  return 1;
#else
  (void)ih;
  (void)w;
  (void)h;
  return 0;
#endif
}

static int gtkFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    GtkFrame* frame = (GtkFrame*)ih->handle;
    gtk_frame_set_label(frame, iupgtkStrConvertToSystem(value));
    return 1;
  }
  return 0;
}

static int gtkFrameSetSunkenAttrib(Ihandle* ih, const char* value)
{
  if (!iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    if (iupStrBoolean(value))
      gtk_frame_set_shadow_type((GtkFrame*)ih->handle, GTK_SHADOW_IN);
    else
      gtk_frame_set_shadow_type((GtkFrame*)ih->handle, GTK_SHADOW_ETCHED_IN);

    return 1;
  }
  return 0;
}

static int gtkFrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  GtkWidget* widget = gtk_frame_get_label_widget((GtkFrame*)ih->handle);

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
  {
    /* ignore given value, must use only from parent */
    value = iupBaseNativeParentGetBgColor(ih);
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (widget)
    iupgtkSetBgColor(widget, r, g, b);

  widget = gtk_bin_get_child((GtkBin*)ih->handle);
  iupgtkSetBgColor(widget, r, g, b);

  if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    return 1;  /* save on the hash table */
  else
    return 0;
}

static int gtkFrameSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  GtkWidget* label = gtk_frame_get_label_widget((GtkFrame*)ih->handle);
  if (!label) return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkSetFgColor(label, r, g, b);

  return 1;
}

static int gtkFrameSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    GtkWidget* label = gtk_frame_get_label_widget((GtkFrame*)ih->handle);
    if (label)
      iupgtkUpdateWidgetFont(ih, (GtkWidget*)label);
  }
  return 1;
}

static void* gtkFrameGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return (void*)gtk_bin_get_child((GtkBin*)ih->handle);
}

static int gtkFrameMapMethod(Ihandle* ih)
{
  char *title;
  GtkWidget *inner_parent;

  if (!ih->parent)
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");

  ih->handle = gtk_frame_new(NULL);
  if (!ih->handle)
    return IUP_ERROR;

  if (title)
    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
  else
  {
    if (iupAttribGet(ih, "BGCOLOR") || iupAttribGet(ih, "BACKCOLOR"))
      iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");
  }

  /* the container that will receive the child element. */
  /* use a window to be a full native container */
  inner_parent = iupgtkNativeContainerNew(1);

  gtk_container_add((GtkContainer*)ih->handle, inner_parent);
  gtk_widget_show(inner_parent);

  /* Set uniform border width for symmetric spacing */
  gtk_container_set_border_width(GTK_CONTAINER(ih->handle), 2);

  /* Add to the parent, all GTK controls must call this. */
  iupgtkAddToParent(ih);

  gtk_widget_realize(ih->handle);

#if GTK_CHECK_VERSION(3, 0, 0)
  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    gtkFrameSetBgColorAttrib(ih, NULL);
#endif

  return IUP_NOERROR;
}

void iupdrvFrameInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkFrameMapMethod;
  ic->GetInnerNativeContainerHandle = gtkFrameGetInnerNativeContainerHandleMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "FONT", NULL, gtkFrameSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, gtkFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", iupFrameGetBgColorAttrib, gtkFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SUNKEN", NULL, gtkFrameSetSunkenAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtkFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}
