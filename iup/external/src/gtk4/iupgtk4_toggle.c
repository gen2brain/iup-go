/** \file
 * \brief Toggle Control for GTK4
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
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_toggle.h"

#include "iupgtk4_drv.h"


#define IUP_TOGGLE_BOX 18

void iupdrvToggleAddBorders(Ihandle* ih, int* x, int* y)
{
  iupdrvButtonAddBorders(ih, x, y);
}

void iupdrvToggleAddCheckBox(Ihandle* ih, int* x, int* y, const char* str)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    GtkWidget* temp_switch = gtk_switch_new();
    int switch_w, switch_h;

    /* Get natural size (3rd parameter) */
    gtk_widget_measure(temp_switch, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &switch_w, NULL, NULL);
    gtk_widget_measure(temp_switch, GTK_ORIENTATION_VERTICAL, -1, NULL, &switch_h, NULL, NULL);

    g_object_ref_sink(temp_switch);
    g_object_unref(temp_switch);

    /* GtkSwitch needs extra vertical padding to prevent bottom clipping (same as GTK3) */
    (*x) += 2 + switch_w + 2;
    if ((*y) < 2 + switch_h + 8) (*y) = 2 + switch_h + 8;
    else (*y) += 2 + 8;

    if (str && str[0])
      (*x) += 8;
  }
  else
  {
    int check_box = IUP_TOGGLE_BOX;
    (void)ih;

    (*x) += 2 + check_box + 2;
    if ((*y) < 2 + check_box + 2) (*y) = 2 + check_box + 2;
    else (*y) += 2 + 2;

    if (str && str[0])
      (*x) += 8;
  }
}

static int gtk4ToggleGetCheck(Ihandle* ih)
{
  if (ih->handle && GTK_IS_SWITCH(ih->handle))
    return gtk_switch_get_active(GTK_SWITCH(ih->handle)) ? 1 : 0;

  if (!ih->handle || !GTK_IS_CHECK_BUTTON(ih->handle))
    return 0; /* Widget not created yet or wrong type */

  if (gtk_check_button_get_inconsistent(GTK_CHECK_BUTTON(ih->handle)))
    return -1;
  if (gtk_check_button_get_active(GTK_CHECK_BUTTON(ih->handle)))
    return 1;
  else
    return 0;
}

static void gtk4ToggleSetPaintable(Ihandle* ih, const char* name, int make_inactive)
{
  GtkWidget* child = NULL;

  if (GTK_IS_BUTTON(ih->handle))
    child = gtk_button_get_child(GTK_BUTTON(ih->handle));

  if (child && GTK_IS_PICTURE(child))
  {
    if (name)
    {
      GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(name, ih, make_inactive, NULL);
      if (paintable)
      {
        int pw = gdk_paintable_get_intrinsic_width(paintable);
        int ph = gdk_paintable_get_intrinsic_height(paintable);

        gtk_picture_set_paintable(GTK_PICTURE(child), paintable);
        return;
      }
    }

    gtk_picture_set_paintable(GTK_PICTURE(child), NULL);
  }
}

static void gtk4ToggleUpdateImage(Ihandle* ih, int active, int check)
{
  char* name;

  if (!active)
  {
    name = iupAttribGet(ih, "IMINACTIVE");
    if (name)
      gtk4ToggleSetPaintable(ih, name, 0);
    else
    {
      name = iupAttribGet(ih, "IMAGE");
      gtk4ToggleSetPaintable(ih, name, 1);
    }
  }
  else
  {
    if (check)
    {
      name = iupAttribGet(ih, "IMPRESS");
      if (name)
        gtk4ToggleSetPaintable(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        gtk4ToggleSetPaintable(ih, name, 0);
      }
    }
    else
    {
      name = iupAttribGet(ih, "IMAGE");
      if (name)
        gtk4ToggleSetPaintable(ih, name, 0);
    }
  }
}

static int gtk4ToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle && GTK_IS_SWITCH(ih->handle))
  {
    int check;
    iupAttribSet(ih, "_IUPGTK4_IGNORE_TOGGLE", "1");

    if (iupStrEqualNoCase(value, "TOGGLE"))
      check = !gtk_switch_get_active(GTK_SWITCH(ih->handle));
    else
      check = iupStrBoolean(value);

    gtk_switch_set_active(GTK_SWITCH(ih->handle), check);

    iupAttribSet(ih, "_IUPGTK4_IGNORE_TOGGLE", NULL);
    return 0;
  }

  if (!ih->handle || !GTK_IS_CHECK_BUTTON(ih->handle))
    return 0; /* Widget not created yet or wrong type */

  if (iupStrEqualNoCase(value, "NOTDEF"))
    gtk_check_button_set_inconsistent(GTK_CHECK_BUTTON(ih->handle), TRUE);
  else
  {
    int check;
    Ihandle* last_ih = NULL;
    Ihandle* radio = iupRadioFindToggleParent(ih);
    gtk_check_button_set_inconsistent(GTK_CHECK_BUTTON(ih->handle), FALSE);

    iupAttribSet(ih, "_IUPGTK4_IGNORE_TOGGLE", "1");
    if (radio)
    {
      last_ih = (Ihandle*)IupGetAttribute(radio, "VALUE_HANDLE");
      if (last_ih)
        iupAttribSet(last_ih, "_IUPGTK4_IGNORE_TOGGLE", "1");
    }

    if (iupStrEqualNoCase(value, "TOGGLE"))
    {
      if (gtk_check_button_get_active(GTK_CHECK_BUTTON(ih->handle)))
        check = 0;
      else
        check = 1;
    }
    else
      check = iupStrBoolean(value);

    gtk_check_button_set_active(GTK_CHECK_BUTTON(ih->handle), check);

    if (ih->data->type == IUP_TOGGLE_IMAGE)
      gtk4ToggleUpdateImage(ih, iupdrvIsActive(ih), gtk4ToggleGetCheck(ih));

    iupAttribSet(ih, "_IUPGTK4_IGNORE_TOGGLE", NULL);
    if (last_ih)
      iupAttribSet(last_ih, "_IUPGTK4_IGNORE_TOGGLE", NULL);
  }

  return 0;
}

static char* gtk4ToggleGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnChecked(gtk4ToggleGetCheck(ih));
}

static int gtk4ToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    if (GTK_IS_SWITCH(ih->handle))
      return 0;
  }

  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    gtk_check_button_set_label(GTK_CHECK_BUTTON(ih->handle), iupgtk4StrConvertToSystem(value ? value : ""));
    gtk_check_button_set_use_underline(GTK_CHECK_BUTTON(ih->handle), TRUE);
    return 1;
  }

  return 0;
}

static int gtk4ToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle && ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupgtk4SetMargin(ih->handle, ih->data->horiz_padding, ih->data->vert_padding);
    return 0;
  }
  else
    return 1;
}

static GtkWidget* gtk4ToggleGetLabel(Ihandle* ih)
{
  GtkWidget* child;

  if (!ih->handle)
    return NULL;

  if (GTK_IS_CHECK_BUTTON(ih->handle))
  {
    /* GtkCheckButton has label as child */
    child = gtk_check_button_get_child(GTK_CHECK_BUTTON(ih->handle));
    if (child && GTK_IS_LABEL(child))
      return child;
  }
  else if (GTK_IS_TOGGLE_BUTTON(ih->handle))
  {
    /* GtkToggleButton has label or box as child */
    child = gtk_button_get_child(GTK_BUTTON(ih->handle));
    if (child && GTK_IS_LABEL(child))
      return child;
  }

  return NULL;
}

static int gtk4ToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    if (ih->handle && GTK_IS_SWITCH(ih->handle))
      return 0; /* GtkSwitch does not have a label */
  }

  if (ih->data->type != IUP_TOGGLE_TEXT)
    return 0;

  if (!ih->handle)
    return 1; /* Store for later */

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  /* GtkCheckButton doesn't expose label widget, use CSS to style text color */
  {
    char css[256];
    GtkCssProvider* provider = gtk_css_provider_new();
    GtkStyleContext* context = gtk_widget_get_style_context(ih->handle);

    sprintf(css, "checkbutton { color: rgb(%d,%d,%d); }", r, g, b);
    gtk_css_provider_load_from_string(provider, css);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
  }

  return 1;
}

static int gtk4ToggleSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle && ih->data->type == IUP_TOGGLE_TEXT)
  {
    /* GtkCheckButton doesn't expose label widget, use CSS to style font */
    if (GTK_IS_CHECK_BUTTON(ih->handle))
    {
      PangoFontDescription* fontdesc = (PangoFontDescription*)iupgtk4GetPangoFontDescAttrib(ih);
      if (fontdesc)
      {
        char css[512];
        const char* family = pango_font_description_get_family(fontdesc);
        int size = pango_font_description_get_size(fontdesc) / PANGO_SCALE;
        PangoWeight weight = pango_font_description_get_weight(fontdesc);
        PangoStyle style = pango_font_description_get_style(fontdesc);

        GtkCssProvider* provider = gtk_css_provider_new();
        GtkStyleContext* context = gtk_widget_get_style_context(ih->handle);

        sprintf(css, "checkbutton { font-family: \"%s\"; font-size: %dpt; font-weight: %d; font-style: %s; }",
                family, size, weight,
                (style == PANGO_STYLE_ITALIC) ? "italic" : "normal");

        gtk_css_provider_load_from_string(provider, css);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(provider);
      }
    }
  }
  return 1;
}

static int gtk4ToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  /* When IMAGE is set, change type to IMAGE toggle */
  if (value)
  {
    if (ih->data->type != IUP_TOGGLE_IMAGE)
      ih->data->type = IUP_TOGGLE_IMAGE;
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    int active = iupdrvIsActive(ih);
    if (active)
      gtk4ToggleSetPaintable(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
        gtk4ToggleSetPaintable(ih, value, 1);
    }
    return 1;
  }
  else
    return 0;
}

static int gtk4ToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        gtk4ToggleSetPaintable(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        gtk4ToggleSetPaintable(ih, name, 1);
      }
    }
    return 1;
  }
  else
    return 0;
}

static int gtk4ToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        gtk4ToggleSetPaintable(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        gtk4ToggleSetPaintable(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      gtk4ToggleSetPaintable(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static void gtk4ToggleToggled(GtkCheckButton* widget, Ihandle* ih)
{
  IFni cb;
  int check = gtk4ToggleGetCheck(ih);
  (void)widget;

  if (iupAttribGet(ih, "_IUPGTK4_IGNORE_TOGGLE"))
    return;

  if (ih->data->type == IUP_TOGGLE_IMAGE)
    gtk4ToggleUpdateImage(ih, iupdrvIsActive(ih), check);

  iupBaseCallValueChangedCb(ih);

  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih, check) == IUP_CLOSE)
      IupExitLoop();
  }
}

static void gtk4SwitchToggled(GtkSwitch* widget, GParamSpec* pspec, Ihandle* ih)
{
  IFni cb;
  (void)widget;
  (void)pspec;

  if (iupAttribGet(ih, "_IUPGTK4_IGNORE_TOGGLE"))
    return;

  iupBaseCallValueChangedCb(ih);

  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int check = gtk_switch_get_active(GTK_SWITCH(ih->handle)) ? 1 : 0;
    if (cb(ih, check) == IUP_CLOSE)
      IupExitLoop();
  }
}

static int gtk4ToggleMapMethod(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  char* value;

  if (!ih->parent)
    return IUP_ERROR;

  value = iupAttribGet(ih, "IMAGE");
  if (value)
    ih->data->type = IUP_TOGGLE_IMAGE;
  else
    ih->data->type = IUP_TOGGLE_TEXT;

  if (radio)
  {
    /* Radio toggles with IMAGE should use GtkToggleButton, not GtkCheckButton */
    if (ih->data->type == IUP_TOGGLE_IMAGE)
    {
      /* IMAGE toggles in radio group use GtkToggleButton */
      ih->handle = gtk_toggle_button_new();
    }
    else
    {
      GtkCheckButton* last_tg = (GtkCheckButton*)iupAttribGet(radio, "_IUPGTK4_LASTRADIOBUTTON");
      ih->handle = gtk_check_button_new();

      if (last_tg)
        gtk_check_button_set_group(GTK_CHECK_BUTTON(ih->handle), last_tg);
      else
      {
        /* First radio button in group should be active by default */
        gtk_check_button_set_active(GTK_CHECK_BUTTON(ih->handle), TRUE);
      }

      iupAttribSet(radio, "_IUPGTK4_LASTRADIOBUTTON", (char*)ih->handle);
    }

    if (!iupAttribGetHandleName(ih))
      iupAttribSetHandleName(ih);

    ih->data->is_radio = 1;
  }
  else
  {
    if (ih->data->type == IUP_TOGGLE_TEXT)
    {
      if (iupAttribGetBoolean(ih, "SWITCH"))
      {
        ih->handle = gtk_switch_new();

        /* Prevent SWITCH from expanding beyond its natural size */
        gtk_widget_set_hexpand(ih->handle, FALSE);
        gtk_widget_set_vexpand(ih->handle, FALSE);
        gtk_widget_set_halign(ih->handle, GTK_ALIGN_START);
        gtk_widget_set_valign(ih->handle, GTK_ALIGN_START);
      }
      else
      {
        ih->handle = gtk_check_button_new();
      }
    }
    else
    {
      /* Use GtkToggleButton for IMAGE toggles (same as GTK3) */
      ih->handle = gtk_toggle_button_new();
    }
  }

  if (!ih->handle)
    return IUP_ERROR;

  /* Set picture child for IMAGE toggles (GtkToggleButton inherits from GtkButton) */
  if (ih->data->type == IUP_TOGGLE_IMAGE && !iupAttribGetBoolean(ih, "SWITCH"))
  {
    /* Use GtkPicture for image rendering */
    GtkWidget* image = gtk_picture_new();

    /* Keep image at natural size */
    gtk_picture_set_can_shrink(GTK_PICTURE(image), TRUE);
    gtk_picture_set_content_fit(GTK_PICTURE(image), GTK_CONTENT_FIT_SCALE_DOWN);

    /* Center the image in the button */
    gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(image, GTK_ALIGN_CENTER);

    gtk_button_set_child(GTK_BUTTON(ih->handle), image);

    /* Reset CSS padding to zero so image has full space */
    {
      GtkCssProvider* provider = gtk_css_provider_new();
      gtk_css_provider_load_from_string(provider, "button { padding: 0; min-width: 0; min-height: 0; }");
      gtk_widget_add_css_class(ih->handle, "iup-image-toggle");
      GtkStyleContext* context = gtk_widget_get_style_context(ih->handle);
      gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      g_object_unref(provider);
    }
  }

  iupgtk4AddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtk4SetCanFocus(ih->handle, 0);

  iupgtk4SetupFocusEvents(ih->handle, ih);
  iupgtk4SetupKeyEvents(ih->handle, ih);
  iupgtk4SetupEnterLeaveEvents(ih->handle, ih);

  if (iupAttribGetBoolean(ih, "SWITCH") && ih->data->type == IUP_TOGGLE_TEXT)
  {
    g_signal_connect(G_OBJECT(ih->handle), "notify::active", G_CALLBACK(gtk4SwitchToggled), ih);
  }
  else
  {
    g_signal_connect(G_OBJECT(ih->handle), "toggled", G_CALLBACK(gtk4ToggleToggled), ih);
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE || iupAttribGetBoolean(ih, "3STATE"))
  {
    iupgtk4SetupButtonEvents(ih->handle, ih);
  }

  gtk_widget_realize(ih->handle);

  iupgtk4UpdateMnemonic(ih);

  return IUP_NOERROR;
}

void iupdrvToggleInitClass(Iclass* ic)
{
  ic->Map = gtk4ToggleMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, gtk4ToggleSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, gtk4ToggleSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtk4ToggleSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", gtk4ToggleGetValueAttrib, gtk4ToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtk4ToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtk4ToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, gtk4ToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, gtk4ToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CANFOCUS", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}
