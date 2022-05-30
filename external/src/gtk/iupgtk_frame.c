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


void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  /* LAYOUT_DECORATION_ESTIMATE */
  *x = 2;
  *y = 2;
}

int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
  (void)ih;
  (void)h;
  return 0;
}

int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  (void)w;
  (void)h;
  return 0;
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
