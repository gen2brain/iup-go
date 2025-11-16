/** \file
 * \brief Label Control for GTK4
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
#include "iup_image.h"
#include "iup_label.h"
#include "iup_drv.h"
#include "iup_image.h"
#include "iup_focus.h"

#include "iupgtk4_drv.h"

void iupdrvLabelAddExtraPadding(Ihandle* ih, int* x, int* y)
{
  /* GTK4 GtkLabel has internal padding beyond what Pango reports.
   * Testing shows GtkLabel requests 5px more width than Pango measures.
   * Add this difference to prevent text clipping. */
  if (ih->data->type == IUP_LABEL_TEXT)
    *x += 5;

  (void)y;
}

static int gtk4LabelSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  /* Set background on the label widget */
  if (ih->handle)
    iupgtk4SetBgColor(ih->handle, r, g, b);

  return 1;
}

static int gtk4LabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    GtkLabel* label = (GtkLabel*)ih->handle;
    if (iupgtk4SetMnemonicTitle(ih, label, value))
    {
      Ihandle* next = iupFocusNextInteractive(ih);
      if (next)
      {
        if (next->handle)
          gtk_label_set_mnemonic_widget(label, next->handle);
        else
          iupAttribSet(next, "_IUPGTK4_LABELMNEMONIC", (char*)label);
      }
    }
    return 1;
  }

  return 0;
}

static int gtk4LabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    GtkLabel* label = (GtkLabel*)ih->handle;
    if (iupStrBoolean(value))
      gtk_label_set_wrap(label, TRUE);
    else
      gtk_label_set_wrap(label, FALSE);
    return 1;
  }
  return 0;
}

static int gtk4LabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    GtkLabel* label = (GtkLabel*)ih->handle;
    if (iupStrBoolean(value))
      gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_END);
    else
      gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_NONE);
    return 1;
  }
  return 0;
}

static int gtk4LabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    PangoAlignment alignment;
    float xalign, yalign;
    char value1[30], value2[30];

    iupStrToStrStr(value, value1, value2, ':');

    if (iupStrEqualNoCase(value1, "ARIGHT"))
    {
      xalign = 1.0f;
      alignment = PANGO_ALIGN_RIGHT;
    }
    else if (iupStrEqualNoCase(value1, "ACENTER"))
    {
      xalign = 0.5f;
      alignment = PANGO_ALIGN_CENTER;
    }
    else
    {
      xalign = 0;
      alignment = PANGO_ALIGN_LEFT;
    }

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      yalign = 1.0f;
    else if (iupStrEqualNoCase(value2, "ATOP"))
      yalign = 0;
    else
      yalign = 0.5f;

    if (ih->data->type == IUP_LABEL_TEXT)
    {
      GtkLabel* label = (GtkLabel*)ih->handle;
      gtk_label_set_xalign(label, xalign);
      gtk_label_set_yalign(label, yalign);
      pango_layout_set_alignment(gtk_label_get_layout(label), alignment);
    }
    else if (ih->data->type == IUP_LABEL_IMAGE)
    {
      gtk_widget_set_halign(ih->handle, xalign == 0 ? GTK_ALIGN_START : (xalign == 1.0f ? GTK_ALIGN_END : GTK_ALIGN_CENTER));
      gtk_widget_set_valign(ih->handle, yalign == 0 ? GTK_ALIGN_START : (yalign == 1.0f ? GTK_ALIGN_END : GTK_ALIGN_CENTER));
    }

    return 1;
  }
  else
    return 0;
}

static int gtk4LabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    iupgtk4SetMargin(ih->handle, ih->data->horiz_padding, ih->data->vert_padding);
    return 0;
  }
  else
    return 1;
}

static void gtk4LabelSetPaintable(Ihandle* ih, const char* name, int make_inactive)
{
  GtkPicture* picture = (GtkPicture*)ih->handle;

  if (name)
  {
    GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(name, ih, make_inactive, NULL);
    if (paintable)
    {
      int pw = gdk_paintable_get_intrinsic_width(paintable);
      int ph = gdk_paintable_get_intrinsic_height(paintable);

      /* Use GtkPicture which renders paintables at exact pixel size */
      gtk_picture_set_paintable(picture, paintable);

      /* Set size request to ensure widget requests the image's size */
      gtk_widget_set_size_request(GTK_WIDGET(picture), pw, ph);

      return;
    }
  }

  gtk_picture_set_paintable(picture, NULL);
}

static int gtk4LabelSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (iupdrvIsActive(ih))
      gtk4LabelSetPaintable(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
      {
        gtk4LabelSetPaintable(ih, value, 1);
      }
    }

    return 1;
  }
  else
    return 0;
}

static int gtk4LabelSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        gtk4LabelSetPaintable(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        gtk4LabelSetPaintable(ih, name, 1);
      }
    }
    return 1;
  }
  else
    return 0;
}

static int gtk4LabelSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        gtk4LabelSetPaintable(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        gtk4LabelSetPaintable(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      gtk4LabelSetPaintable(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int gtk4LabelMapMethod(Ihandle* ih)
{
  char* value;
  GtkWidget* label;

  value = iupAttribGet(ih, "SEPARATOR");
  if (value)
  {
    if (iupStrEqualNoCase(value, "HORIZONTAL"))
    {
      ih->data->type = IUP_LABEL_SEP_HORIZ;
      label = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    }
    else
    {
      ih->data->type = IUP_LABEL_SEP_VERT;
      label = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    }
  }
  else
  {
    value = iupAttribGet(ih, "IMAGE");
    if (value)
    {
      ih->data->type = IUP_LABEL_IMAGE;
      /* Use GtkPicture instead of GtkImage for exact pixel-perfect rendering */
      label = gtk_picture_new();
      /* Set picture to not be resizable - keep original size */
      gtk_picture_set_can_shrink(GTK_PICTURE(label), FALSE);
    }
    else
    {
      ih->data->type = IUP_LABEL_TEXT;
      label = gtk_label_new(NULL);
    }
  }

  if (!label)
    return IUP_ERROR;

  ih->handle = label;

  /* Unlike GTK3 which used GtkEventBox wrapper for event handling,
   * GTK4 allows event controllers directly on any widget (all widgets receive events).
   * Attach controllers directly to label for proper sizing like GTK3's GtkEventBox. */
  iupgtk4SetupButtonEvents(label, ih);
  iupgtk4SetupMotionEvents(label, ih);
  iupgtk4SetupEnterLeaveEvents(label, ih);

  iupgtk4AddToParent(ih);

  gtk_widget_realize(label);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

void iupdrvLabelInitClass(Iclass* ic)
{
  ic->Map = gtk4LabelMapMethod;

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, gtk4LabelSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "BGCOLOR", iupBaseNativeParentGetBgColorAttrib, gtk4LabelSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtk4LabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtk4LabelSetAlignmentAttrib, "ALEFT:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtk4LabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, gtk4LabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, gtk4LabelSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, gtk4LabelSetWordWrapAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, gtk4LabelSetEllipsisAttrib, NULL, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}
