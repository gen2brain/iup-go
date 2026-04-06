/** \file
 * \brief Toggle Control for GTK4
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_toggle.h"

#include "iupgtk4_drv.h"


#define IUP_TOGGLE_BOX 18

static int gtk4_toggle_border_x = -1, gtk4_toggle_border_y = -1;
static int gtk4_toggle_struct_x = 0, gtk4_toggle_struct_y = 0;
static int gtk4_toggle_min_w = 0, gtk4_toggle_min_h = 0;

static void gtk4ToggleMeasureBorders(void)
{
  GtkWidget* temp_window;
  GtkWidget* temp_toggle;
  GtkWidget* temp_image;
  GdkPaintable* temp_paintable;
  GtkRequisition button_size, child_size;

  temp_window = gtk_window_new();
  temp_toggle = gtk_toggle_button_new();
  gtk_widget_add_css_class(temp_toggle, "iup-measure-no-min");
  temp_image = gtk_picture_new();
  temp_paintable = gdk_paintable_new_empty(64, 64);
  gtk_picture_set_paintable(GTK_PICTURE(temp_image), temp_paintable);
  gtk_picture_set_can_shrink(GTK_PICTURE(temp_image), FALSE);
  gtk_button_set_child(GTK_BUTTON(temp_toggle), temp_image);
  gtk_window_set_child(GTK_WINDOW(temp_window), temp_toggle);

  gtk_widget_get_preferred_size(temp_toggle, NULL, &button_size);
  gtk_widget_get_preferred_size(temp_image, NULL, &child_size);

  gtk4_toggle_border_x = button_size.width - child_size.width;
  gtk4_toggle_border_y = button_size.height - child_size.height;
  if (gtk4_toggle_border_x < 0) gtk4_toggle_border_x = 0;
  if (gtk4_toggle_border_y < 0) gtk4_toggle_border_y = 0;

  if (gtk4_toggle_border_x > gtk4_toggle_border_y)
    gtk4_toggle_border_x = gtk4_toggle_border_y;

  gtk_widget_add_css_class(temp_toggle, "iup-measure-zero-pad");
  gtk_widget_queue_resize(temp_toggle);
  gtk_widget_get_preferred_size(temp_toggle, NULL, &button_size);
  gtk4_toggle_struct_x = button_size.width - child_size.width;
  gtk4_toggle_struct_y = button_size.height - child_size.height;
  if (gtk4_toggle_struct_x < 0) gtk4_toggle_struct_x = 0;
  if (gtk4_toggle_struct_y < 0) gtk4_toggle_struct_y = 0;

  {
    GtkRequisition min_size;
    gtk_widget_remove_css_class(temp_toggle, "iup-measure-no-min");
    gtk_widget_remove_css_class(temp_toggle, "iup-measure-zero-pad");

    GdkPaintable* empty_paintable = gdk_paintable_new_empty(1, 1);
    gtk_picture_set_paintable(GTK_PICTURE(temp_image), empty_paintable);
    gtk_widget_queue_resize(temp_toggle);
    gtk_widget_get_preferred_size(temp_toggle, &min_size, NULL);
    g_object_unref(empty_paintable);

    gtk4_toggle_min_w = min_size.width;
    gtk4_toggle_min_h = min_size.height;
  }

  g_object_unref(temp_paintable);
  gtk_window_destroy(GTK_WINDOW(temp_window));
}

IUP_SDK_API void iupdrvToggleAddBorders(Ihandle* ih, int* x, int* y)
{
  int has_user_padding = 0;
  int has_user_size = 0;

  if (gtk4_toggle_border_x < 0)
    gtk4ToggleMeasureBorders();

  if (ih)
  {
    has_user_padding = (ih->data->horiz_padding > 0 || ih->data->vert_padding > 0);
    has_user_size = (ih->userwidth > 0 || ih->userheight > 0);
  }

  if (has_user_padding || has_user_size)
  {
    (*x) += gtk4_toggle_struct_x;
    (*y) += gtk4_toggle_struct_y;
  }
  else
  {
    (*x) += gtk4_toggle_border_x;
    (*y) += gtk4_toggle_border_y;
  }

  if (*x < gtk4_toggle_min_w) *x = gtk4_toggle_min_w;
  if (*y < gtk4_toggle_min_h) *y = gtk4_toggle_min_h;
}

IUP_SDK_API void iupdrvToggleAddSwitch(Ihandle* ih, int* x, int* y, const char* str)
{
  static int switch_w = -1;
  static int switch_h = -1;
  (void)ih;

  if (switch_w < 0)
  {
    GtkWidget* temp_switch = gtk_switch_new();
    int min_w, nat_w, min_h, nat_h;

    g_object_ref_sink(temp_switch);

    /* Get natural and minimum sizes using gtk_widget_measure */
    gtk_widget_measure(temp_switch, GTK_ORIENTATION_HORIZONTAL, -1, &min_w, &nat_w, NULL, NULL);
    gtk_widget_measure(temp_switch, GTK_ORIENTATION_VERTICAL, -1, &min_h, &nat_h, NULL, NULL);

    /* Use natural size with fallback */
    switch_w = (nat_w > 0) ? nat_w : 48;
    switch_h = (nat_h > 0) ? nat_h : 24;

    g_object_unref(temp_switch);
  }

  (*x) += 2 + switch_w + 2;
  if ((*y) < 2 + switch_h + 2) (*y) = 2 + switch_h + 2;
  else (*y) += 2 + 2;

  if (str && str[0])
    (*x) += 8;
}

IUP_SDK_API void iupdrvToggleAddCheckBox(Ihandle* ih, int* x, int* y, const char* str)
{
  static int check_w = -1;
  static int check_h = -1;
  (void)ih;

  if (check_w < 0)
  {
    GtkWidget* temp_check = gtk_check_button_new();
    int min_w, nat_w, min_h, nat_h;

    g_object_ref_sink(temp_check);

    gtk_widget_measure(temp_check, GTK_ORIENTATION_HORIZONTAL, -1, &min_w, &nat_w, NULL, NULL);
    gtk_widget_measure(temp_check, GTK_ORIENTATION_VERTICAL, -1, &min_h, &nat_h, NULL, NULL);

    check_w = (nat_w > 0) ? nat_w : IUP_TOGGLE_BOX;
    check_h = (nat_h > 0) ? nat_h : IUP_TOGGLE_BOX;

    g_object_unref(temp_check);
  }

  (*x) += 2 + check_w + 2;
  if ((*y) < 2 + check_h + 2) (*y) = 2 + check_h + 2;
  else (*y) += 2 + 2;

  if (str && str[0])
    (*x) += 8;
}

static int gtk4ToggleGetCheck(Ihandle* ih)
{
  if (!ih->handle)
    return 0;

  if (GTK_IS_SWITCH(ih->handle))
    return gtk_switch_get_active(GTK_SWITCH(ih->handle)) ? 1 : 0;

  if (GTK_IS_TOGGLE_BUTTON(ih->handle))
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ih->handle)) ? 1 : 0;

  if (GTK_IS_CHECK_BUTTON(ih->handle))
  {
    if (gtk_check_button_get_inconsistent(GTK_CHECK_BUTTON(ih->handle)))
      return -1;
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(ih->handle)))
      return 1;
  }

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

static void gtk4ToggleRadioDeactivatePrevious(Ihandle* radio, Ihandle* ih)
{
  Ihandle* last_ih = (Ihandle*)iupAttribGet(radio, "_IUPGTK4_RADIO_ACTIVE");
  if (!last_ih || last_ih == ih || !iupObjectCheck(last_ih))
    return;

  iupAttribSet(last_ih, "_IUPGTK4_IGNORE_TOGGLE", "1");

  if (GTK_IS_TOGGLE_BUTTON(last_ih->handle))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(last_ih->handle), FALSE);
  else if (GTK_IS_CHECK_BUTTON(last_ih->handle))
    gtk_check_button_set_active(GTK_CHECK_BUTTON(last_ih->handle), FALSE);

  iupAttribSet(last_ih, "_IUPGTK4_IGNORE_TOGGLE", NULL);

  if (last_ih->data->type == IUP_TOGGLE_IMAGE)
    gtk4ToggleUpdateImage(last_ih, iupdrvIsActive(last_ih), 0);
}

static int gtk4ToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  if (GTK_IS_SWITCH(ih->handle))
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

  if (GTK_IS_TOGGLE_BUTTON(ih->handle))
  {
    int check;
    Ihandle* radio = iupRadioFindToggleParent(ih);

    iupAttribSet(ih, "_IUPGTK4_IGNORE_TOGGLE", "1");

    if (iupStrEqualNoCase(value, "TOGGLE"))
      check = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ih->handle));
    else
      check = iupStrBoolean(value);

    if (check && radio)
      gtk4ToggleRadioDeactivatePrevious(radio, ih);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ih->handle), check);

    if (ih->data->type == IUP_TOGGLE_IMAGE)
      gtk4ToggleUpdateImage(ih, iupdrvIsActive(ih), check);

    if (radio && check)
      iupAttribSet(radio, "_IUPGTK4_RADIO_ACTIVE", (char*)ih);

    iupAttribSet(ih, "_IUPGTK4_IGNORE_TOGGLE", NULL);
    return 0;
  }

  if (!GTK_IS_CHECK_BUTTON(ih->handle))
    return 0;

  if (iupStrEqualNoCase(value, "NOTDEF"))
    gtk_check_button_set_inconsistent(GTK_CHECK_BUTTON(ih->handle), TRUE);
  else
  {
    int check;
    Ihandle* radio = iupRadioFindToggleParent(ih);
    gtk_check_button_set_inconsistent(GTK_CHECK_BUTTON(ih->handle), FALSE);

    iupAttribSet(ih, "_IUPGTK4_IGNORE_TOGGLE", "1");

    if (iupStrEqualNoCase(value, "TOGGLE"))
    {
      if (gtk_check_button_get_active(GTK_CHECK_BUTTON(ih->handle)))
        check = 0;
      else
        check = 1;
    }
    else
      check = iupStrBoolean(value);

    if (check && radio)
      gtk4ToggleRadioDeactivatePrevious(radio, ih);

    gtk_check_button_set_active(GTK_CHECK_BUTTON(ih->handle), check);

    if (ih->data->type == IUP_TOGGLE_IMAGE)
      gtk4ToggleUpdateImage(ih, iupdrvIsActive(ih), gtk4ToggleGetCheck(ih));

    if (radio && check)
      iupAttribSet(radio, "_IUPGTK4_RADIO_ACTIVE", (char*)ih);

    iupAttribSet(ih, "_IUPGTK4_IGNORE_TOGGLE", NULL);
  }

  return 0;
}

static char* gtk4ToggleGetValueAttrib(Ihandle* ih)
{
  return iupStrReturnChecked(gtk4ToggleGetCheck(ih));
}

static GtkLabel* gtk4ToggleGetLabel(GtkWidget* widget)
{
  GtkWidget* child;
  for (child = gtk_widget_get_first_child(widget); child; child = gtk_widget_get_next_sibling(child))
  {
    if (GTK_IS_LABEL(child))
      return GTK_LABEL(child);
  }
  return NULL;
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
    gtk_check_button_set_label(GTK_CHECK_BUTTON(ih->handle), value ? value : "");

    GtkLabel* label = gtk4ToggleGetLabel(ih->handle);
    if (label)
      iupgtk4SetMnemonicTitle(ih, label, value);

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
    if (ih->data->horiz_padding > 0 || ih->data->vert_padding > 0)
    {
      iupgtk4CssSetWidgetPadding(ih->handle, ih->data->horiz_padding, ih->data->vert_padding);
      iupgtk4CssSetWidgetCustom(ih->handle, "min-width", "0; min-height: 0");
    }
    else
    {
      iupgtk4CssResetWidgetPadding(ih->handle);
      iupgtk4CssResetWidgetCustom(ih->handle);
    }
    return 0;
  }
  else
    return 1;
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
  iupgtk4CssSetWidgetFgColor(ih->handle, r, g, b);

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
        const char* safe_family = family ? family : "sans";
        char escaped_family[256];
        int fi = 0, fo = 0;
        int size = pango_font_description_get_size(fontdesc) / PANGO_SCALE;
        PangoWeight weight = pango_font_description_get_weight(fontdesc);
        PangoStyle style = pango_font_description_get_style(fontdesc);

        while (safe_family[fi] && fo < (int)sizeof(escaped_family) - 2)
        {
          char c = safe_family[fi++];
          if (c == '"' || c == '\\' || c == ';' || c == '}' || c == '{')
            escaped_family[fo++] = '\\';
          escaped_family[fo++] = c;
        }
        escaped_family[fo] = 0;

        snprintf(css, sizeof(css), "font-family: \"%s\"; font-size: %dpt; font-weight: %d; font-style: %s;",
                escaped_family, size, weight,
                (style == PANGO_STYLE_ITALIC) ? "italic" : "normal");

        iupgtk4CssSetWidgetFont(ih->handle, css);
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

static int gtk4ToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    gtk4ToggleUpdateImage(ih, iupdrvIsActive(ih), gtk4ToggleGetCheck(ih));
    return 1;
  }
  return 0;
}

static int gtk4ToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    float xalign, yalign;
    char value1[30], value2[30];

    iupStrToStrStr(value, value1, sizeof(value1), value2, sizeof(value2), ':');

    if (iupStrEqualNoCase(value1, "ARIGHT"))
      xalign = 1.0f;
    else if (iupStrEqualNoCase(value1, "ALEFT"))
      xalign = 0;
    else
      xalign = 0.5f;

    if (iupStrEqualNoCase(value2, "ABOTTOM"))
      yalign = 1.0f;
    else if (iupStrEqualNoCase(value2, "ATOP"))
      yalign = 0;
    else
      yalign = 0.5f;

    GtkWidget* child = gtk_button_get_child(GTK_BUTTON(ih->handle));
    if (child)
    {
      gtk_widget_set_halign(child, xalign == 0 ? GTK_ALIGN_START : (xalign == 1.0f ? GTK_ALIGN_END : GTK_ALIGN_CENTER));
      gtk_widget_set_valign(child, yalign == 0 ? GTK_ALIGN_START : (yalign == 1.0f ? GTK_ALIGN_END : GTK_ALIGN_CENTER));
    }

    return 1;
  }

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

  if (ih->data->is_radio && check)
  {
    Ihandle* radio = iupRadioFindToggleParent(ih);
    if (radio)
    {
      gtk4ToggleRadioDeactivatePrevious(radio, ih);
      iupAttribSet(radio, "_IUPGTK4_RADIO_ACTIVE", (char*)ih);
    }
  }

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
    int is_first = !iupAttribGet(radio, "_IUPGTK4_RADIO_ACTIVE");

    if (ih->data->type == IUP_TOGGLE_IMAGE)
    {
      ih->handle = gtk_toggle_button_new();
    }
    else
    {
      GtkCheckButton* last_tg = (GtkCheckButton*)iupAttribGet(radio, "_IUPGTK4_LASTRADIOBUTTON");
      ih->handle = gtk_check_button_new();

      if (last_tg)
        gtk_check_button_set_group(GTK_CHECK_BUTTON(ih->handle), last_tg);

      iupAttribSet(radio, "_IUPGTK4_LASTRADIOBUTTON", (char*)ih->handle);
    }

    if (is_first)
    {
      if (GTK_IS_TOGGLE_BUTTON(ih->handle))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ih->handle), TRUE);
      else
        gtk_check_button_set_active(GTK_CHECK_BUTTON(ih->handle), TRUE);

      iupAttribSet(radio, "_IUPGTK4_RADIO_ACTIVE", (char*)ih);
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
    iupgtk4ButtonApplyImagePadding(ih->handle);
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

IUP_SDK_API void iupdrvToggleInitClass(Iclass* ic)
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
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, gtk4ToggleSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtk4ToggleSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, gtk4ToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
