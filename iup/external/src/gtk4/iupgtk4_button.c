/** \file
 * \brief Button Control for GTK4
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
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"

#include "iupgtk4_drv.h"

/* Store measured button padding for different button types */
static int gtk4_button_padding_text_x = 0;
static int gtk4_button_padding_text_y = 0;
static int gtk4_button_padding_image_x = 0;
static int gtk4_button_padding_image_y = 0;
static int gtk4_button_padding_both_x = 0;
static int gtk4_button_padding_both_y = 0;
static int gtk4_button_padding_measured = 0;

static void gtk4ButtonMeasurePadding(void)
{
  GtkWidget* temp_button;
  GtkWidget* temp_image;
  GtkWidget* temp_label;
  GtkWidget* temp_box;
  GtkRequisition button_size, child_size;
  GdkPaintable* temp_paintable;
  GtkCssProvider* provider;
  GtkStyleContext* context;

  /* Apply the same CSS that we use for real buttons BEFORE measuring! */
  provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(provider,
    "button { padding: 4px; min-width: 0; min-height: 0; }");

  /* Measure TEXT-only button */
  temp_button = gtk_button_new_with_label("Test");
  context = gtk_widget_get_style_context(temp_button);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_widget_get_preferred_size(temp_button, NULL, &button_size);
  temp_label = gtk_button_get_child(GTK_BUTTON(temp_button));
  gtk_widget_get_preferred_size(temp_label, NULL, &child_size);
  gtk4_button_padding_text_x = button_size.width - child_size.width;
  gtk4_button_padding_text_y = button_size.height - child_size.height;
  g_object_ref_sink(temp_button);
  g_object_unref(temp_button);

  /* Measure IMAGE-only button */
  temp_button = gtk_button_new();
  context = gtk_widget_get_style_context(temp_button);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  temp_image = gtk_picture_new();
  temp_paintable = gdk_paintable_new_empty(16, 16);
  gtk_picture_set_paintable(GTK_PICTURE(temp_image), temp_paintable);
  gtk_button_set_child(GTK_BUTTON(temp_button), temp_image);
  gtk_widget_get_preferred_size(temp_button, NULL, &button_size);
  gtk_widget_get_preferred_size(temp_image, NULL, &child_size);
  gtk4_button_padding_image_x = button_size.width - child_size.width;
  gtk4_button_padding_image_y = button_size.height - child_size.height;
  g_object_unref(temp_paintable);
  g_object_ref_sink(temp_button);
  g_object_unref(temp_button);

  /* Measure IMAGE+TEXT button (use spacing=2) */
  temp_button = gtk_button_new();
  context = gtk_widget_get_style_context(temp_button);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  temp_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  temp_image = gtk_picture_new();
  temp_paintable = gdk_paintable_new_empty(16, 16);
  gtk_picture_set_paintable(GTK_PICTURE(temp_image), temp_paintable);
  gtk_box_append(GTK_BOX(temp_box), temp_image);
  temp_label = gtk_label_new("Test");
  gtk_box_append(GTK_BOX(temp_box), temp_label);
  gtk_button_set_child(GTK_BUTTON(temp_button), temp_box);
  gtk_widget_get_preferred_size(temp_button, NULL, &button_size);
  gtk_widget_get_preferred_size(temp_box, NULL, &child_size);
  gtk4_button_padding_both_x = button_size.width - child_size.width;
  gtk4_button_padding_both_y = button_size.height - child_size.height;
  g_object_unref(temp_paintable);
  g_object_ref_sink(temp_button);
  g_object_unref(temp_button);

  g_object_unref(provider);

  gtk4_button_padding_measured = 1;
}

void iupdrvButtonAddBorders(Ihandle* ih, int* x, int* y)
{
  int button_type;
  char* image;
  char* title;

  /* Measure padding on first call */
  if (!gtk4_button_padding_measured)
    gtk4ButtonMeasurePadding();

  image = iupAttribGet(ih, "IMAGE");
  title = iupAttribGet(ih, "TITLE");

  if (image && title && *title != 0)
    button_type = IUP_BUTTON_BOTH;
  else if (image)
    button_type = IUP_BUTTON_IMAGE;
  else
    button_type = IUP_BUTTON_TEXT;

  /* Use measured GTK4 default padding for each button type */
  if (button_type == IUP_BUTTON_TEXT)
  {
    (*x) += gtk4_button_padding_text_x;
    (*y) += gtk4_button_padding_text_y;
  }
  else if (button_type == IUP_BUTTON_IMAGE)
  {
    (*x) += gtk4_button_padding_image_x;
    (*y) += gtk4_button_padding_image_y;
  }
  else if (button_type == IUP_BUTTON_BOTH)
  {
    (*x) += gtk4_button_padding_both_x;
    (*y) += gtk4_button_padding_both_y;
  }
}

static GtkLabel* gtk4ButtonGetLabel(Ihandle* ih)
{
  if (!GTK_IS_BUTTON(ih->handle))
    return NULL;

  if (ih->data->type == IUP_BUTTON_TEXT)
  {
    GtkWidget* child = gtk_button_get_child(GTK_BUTTON(ih->handle));
    if (GTK_IS_LABEL(child))
      return (GtkLabel*)child;
  }
  else if (ih->data->type == IUP_BUTTON_BOTH)
  {
    GtkWidget* box = gtk_button_get_child(GTK_BUTTON(ih->handle));
    if (GTK_IS_BOX(box))
    {
      GtkWidget* label = gtk_widget_get_first_child(box);
      while (label)
      {
        if (GTK_IS_LABEL(label))
          return (GtkLabel*)label;
        label = gtk_widget_get_next_sibling(label);
      }
    }
  }
  return NULL;
}

static int gtk4ButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_TEXT)
  {
    GtkLabel* label = gtk4ButtonGetLabel(ih);
    if (label)
    {
      iupgtk4SetMnemonicTitle(ih, label, value);
      return 1;
    }
  }

  return 0;
}

static int gtk4ButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  PangoAlignment alignment;
  float xalign, yalign;
  char value1[30], value2[30];

  if (iupAttribGet(ih, "_IUPGTK4_EVENTBOX"))
    return 0;

  iupStrToStrStr(value, value1, value2, ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
  {
    xalign = 1.0f;
    alignment = PANGO_ALIGN_RIGHT;
  }
  else if (iupStrEqualNoCase(value1, "ALEFT"))
  {
    xalign = 0;
    alignment = PANGO_ALIGN_LEFT;
  }
  else
  {
    xalign = 0.5f;
    alignment = PANGO_ALIGN_CENTER;
  }

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    yalign = 1.0f;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    yalign = 0;
  else
    yalign = 0.5f;

  if (ih->data->type == IUP_BUTTON_TEXT)
  {
    GtkLabel* label = gtk4ButtonGetLabel(ih);
    if (label)
    {
      gtk_label_set_xalign(label, xalign);
      gtk_label_set_yalign(label, yalign);
      PangoLayout* layout = gtk_label_get_layout(label);
      if (layout)
        pango_layout_set_alignment(layout, alignment);
    }
  }

  return 1;
}

static int gtk4ButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
    iupgtk4SetMargin(ih->handle, ih->data->horiz_padding, ih->data->vert_padding);
    return 0;
  }
  else
    return 1;
}

static int gtk4ButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_BUTTON_TEXT && GTK_IS_BUTTON(ih->handle))
  {
    GtkWidget* child = gtk_button_get_child(GTK_BUTTON(ih->handle));
    if (child && GTK_IS_WIDGET(child) && !GTK_IS_LABEL(child))
    {
      unsigned char r, g, b;
      if (!iupStrToRGB(value, &r, &g, &b))
        return 0;

      iupgtk4SetBgColor(child, r, g, b);
      return 1;
    }
  }

  return iupdrvBaseSetBgColorAttrib(ih, value);
}

static int gtk4ButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  GtkLabel* label = gtk4ButtonGetLabel(ih);
  if (!label) return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtk4SetFgColor((GtkWidget*)label, r, g, b);

  return 1;
}

static int gtk4ButtonSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    GtkLabel* label = gtk4ButtonGetLabel(ih);
    if (label)
      iupgtk4UpdateWidgetFont(ih, (GtkWidget*)label);
  }
  return 1;
}

static void gtk4ButtonSetPaintable(Ihandle* ih, const char* name, int make_inactive)
{
  GtkWidget* child = NULL;
  GtkPicture* picture = NULL;

  /* Get child - either from GtkButton or GtkBox (for IMAGE+IMPRESS) */
  if (GTK_IS_BUTTON(ih->handle))
    child = gtk_button_get_child(GTK_BUTTON(ih->handle));
  else if (GTK_IS_BOX(ih->handle))
    child = gtk_widget_get_first_child(GTK_WIDGET(ih->handle));

  if (!child)
    return;

  if (GTK_IS_PICTURE(child))
    picture = GTK_PICTURE(child);
  else if (GTK_IS_BOX(child))
  {
    GtkWidget* widget = gtk_widget_get_first_child(child);
    while (widget)
    {
      if (GTK_IS_PICTURE(widget))
      {
        picture = GTK_PICTURE(widget);
        break;
      }
      widget = gtk_widget_get_next_sibling(widget);
    }
  }

  if (name && picture)
  {
    GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(name, ih, make_inactive, NULL);
    if (paintable)
    {
      int pw = gdk_paintable_get_intrinsic_width(paintable);
      int ph = gdk_paintable_get_intrinsic_height(paintable);

      /* Set content_fit BEFORE setting the paintable */
      gtk_picture_set_content_fit(picture, GTK_CONTENT_FIT_SCALE_DOWN);

      gtk_picture_set_paintable(picture, paintable);

      /* Don't set size_request - let GtkPicture use paintable's intrinsic size.
         The paintable already has the correct size, and GtkPicture will honor it
         when using GTK_CONTENT_FIT_SCALE_DOWN with can_shrink=FALSE. */
      (void)pw;
      (void)ph;

      return;
    }
  }

  if (picture)
    gtk_picture_set_paintable(picture, NULL);
}

static int gtk4ButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (iupdrvIsActive(ih))
      gtk4ButtonSetPaintable(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
        gtk4ButtonSetPaintable(ih, value, 1);
    }
    return 1;
  }
  else
    return 0;
}

static int gtk4ButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        gtk4ButtonSetPaintable(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        gtk4ButtonSetPaintable(ih, name, 1);
      }
    }
    return 1;
  }
  else
    return 0;
}

static int gtk4ButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        gtk4ButtonSetPaintable(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        gtk4ButtonSetPaintable(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      gtk4ButtonSetPaintable(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static void gtk4ButtonPressed(GtkGestureClick* gesture, int n_press, double x, double y, Ihandle* ih)
{
  /* Handle IMPRESS image swap on press */
  if (ih->data->type == IUP_BUTTON_IMAGE)
  {
    char* impress = iupAttribGet(ih, "IMPRESS");
    if (impress)
      gtk4ButtonSetPaintable(ih, impress, 0);
  }

  /* Call BUTTON_CB with pressed=1 */
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    int button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    int b = IUP_BUTTON1 + (button - 1);
    int doubleclick = (n_press == 2) ? 1 : 0;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));

    iupgtk4ButtonKeySetStatus(state, button, status, doubleclick);

    int ret = cb(ih, b, 1, (int)x, (int)y, status);  /* pressed = 1 */
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_IGNORE)
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
  }
}

static void gtk4ButtonReleased(GtkGestureClick* gesture, int n_press, double x, double y, Ihandle* ih)
{
  /* Handle IMPRESS image restore on release */
  if (ih->data->type == IUP_BUTTON_IMAGE)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image)
      gtk4ButtonSetPaintable(ih, image, 0);
  }

  /* Call BUTTON_CB with pressed=0 */
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    int button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    int b = IUP_BUTTON1 + (button - 1);
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));

    iupgtk4ButtonKeySetStatus(state, button, status, 0);  /* no doubleclick on release */

    int ret = cb(ih, b, 0, (int)x, (int)y, status);  /* pressed = 0 */
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_IGNORE)
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
  }
}

static void gtk4ButtonClicked(GtkButton* widget, Ihandle* ih)
{
  /* "clicked" signal is for ACTION callback only */
  Icallback cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
  (void)widget;
}

static void gtk4ButtonGesturePressed(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle* ih)
{

  /* Handle IMPRESS image swap on press */
  if (ih->data->type == IUP_BUTTON_IMAGE)
  {
    char* impress = iupAttribGet(ih, "IMPRESS");
    if (impress)
    {
      gtk4ButtonSetPaintable(ih, impress, 0);
    }
  }

  /* Call BUTTON_CB with pressed=1 */
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    int button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    int b = IUP_BUTTON1 + (button - 1);
    int doubleclick = (n_press == 2) ? 1 : 0;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));

    iupgtk4ButtonKeySetStatus(state, button, status, doubleclick);

    int ret = cb(ih, b, 1, (int)x, (int)y, status);  /* press = 1 */
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_IGNORE)
    {
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
    }
  }
}

static void gtk4ButtonGestureReleased(GtkGestureClick *gesture, int n_press, double x, double y, Ihandle* ih)
{

  /* Handle IMPRESS image swap on release - restore original IMAGE */
  if (ih->data->type == IUP_BUTTON_IMAGE)
  {
    char* impress = iupAttribGet(ih, "IMPRESS");
    if (impress)
    {
      char* image = iupAttribGet(ih, "IMAGE");
      gtk4ButtonSetPaintable(ih, image, 0);
    }
  }

  /* Call BUTTON_CB with pressed=0 */
  IFniiiis button_cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (button_cb)
  {
    int button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    int b = IUP_BUTTON1 + (button - 1);
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    GdkModifierType state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));

    iupgtk4ButtonKeySetStatus(state, button, status, 0);

    int ret = button_cb(ih, b, 0, (int)x, (int)y, status);  /* press = 0 */
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_IGNORE)
    {
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
      return;
    }
  }

  Icallback action_cb = IupGetCallback(ih, "ACTION");
  if (action_cb)
  {
    int ret = action_cb(ih);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }
}

static void gtk4ButtonGestureBegin(GtkGesture *gesture, GdkEventSequence *sequence, Ihandle* ih)
{
  (void)gesture;
  (void)sequence;
}

static void gtk4ButtonMotionEnter(GtkEventControllerMotion* controller, double x, double y, Ihandle* ih)
{
  /* Add visual feedback on hover - reduce opacity slightly */
  if (ih->data->type == IUP_BUTTON_IMAGE && iupAttribGet(ih, "IMPRESS"))
  {
    gtk_widget_set_opacity(ih->handle, 0.7);
  }

  (void)controller;
  (void)x;
  (void)y;
}

static void gtk4ButtonMotionLeave(GtkEventControllerMotion* controller, Ihandle* ih)
{
  /* Restore full opacity when mouse leaves */
  if (ih->data->type == IUP_BUTTON_IMAGE && iupAttribGet(ih, "IMPRESS"))
  {
    gtk_widget_set_opacity(ih->handle, 1.0);
  }

  (void)controller;
}

static int gtk4ButtonMapMethod(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "IMAGE");
  int has_border = 1;

  if (value)
  {
    ih->data->type = IUP_BUTTON_IMAGE;

    value = iupAttribGet(ih, "TITLE");
    if (value && *value != 0)
      ih->data->type |= IUP_BUTTON_TEXT;
  }
  else
    ih->data->type = IUP_BUTTON_TEXT;

  /* Check if button should have no border (IMPRESS without IMPRESSBORDER) */
  if (ih->data->type == IUP_BUTTON_IMAGE &&
      iupAttribGet(ih, "IMPRESS") &&
      !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
  {
    has_border = 0;
  }

  /* For borderless image buttons (IMPRESS without border), use a box instead of GtkButton
     This avoids GtkButton's minimum size constraints */
  if (!has_border)
  {
    ih->handle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    iupAttribSet(ih, "_IUPGTK_EVENTBOX", "1");
  }
  else
  {
    ih->handle = gtk_button_new();

    /* Set CSS padding to match what we measured and what AddBorders expects */
    GtkCssProvider* provider = gtk_css_provider_new();
    GtkStyleContext* context = gtk_widget_get_style_context(ih->handle);

    if (iupAttribGet(ih, "IMPRESS") && !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
    {
      /* IMPRESS without border: no padding since AddBorders is not called for these */
      gtk_css_provider_load_from_string(provider,
        "button { padding: 0; margin: 0; border-width: 0; outline-width: 0; min-width: 0; min-height: 0; }");
    }
    else
    {
      /* Normal buttons: 4px padding (same as measurement) */
      gtk_css_provider_load_from_string(provider,
        "button { padding: 4px; min-width: 0; min-height: 0; }");
    }

    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    /* Handle FLAT attribute for buttons with borders */
    if (iupAttribGetBoolean(ih, "FLAT"))
    {
      GtkStyleContext* context = gtk_widget_get_style_context(ih->handle);
      gtk_style_context_add_class(context, "flat");
    }
  }

  if (!ih->handle)
    return IUP_ERROR;

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    /* Use GtkPicture for exact pixel-size rendering */
    GtkWidget* image = gtk_picture_new();
    gtk_picture_set_can_shrink(GTK_PICTURE(image), FALSE);
    /* Use SCALE_DOWN to show at natural size, never scale up */
    gtk_picture_set_content_fit(GTK_PICTURE(image), GTK_CONTENT_FIT_SCALE_DOWN);

    if (ih->data->type & IUP_BUTTON_TEXT)
    {
      /* Use IUP's spacing value (default 2) to match size calculation */
      GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, ih->data->spacing);
      gtk_box_append(GTK_BOX(box), image);

      GtkWidget* label = gtk_label_new("");
      gtk_box_append(GTK_BOX(box), label);

      if (GTK_IS_BUTTON(ih->handle))
        gtk_button_set_child(GTK_BUTTON(ih->handle), box);
      else
        gtk_box_append(GTK_BOX(ih->handle), box);
    }
    else
    {
      /* IMAGE-only button (no text) */
      if (GTK_IS_BUTTON(ih->handle))
        gtk_button_set_child(GTK_BUTTON(ih->handle), image);
      else
        gtk_box_append(GTK_BOX(ih->handle), image);
    }

    /* Load and size the picture immediately after creating the widget */
    value = iupAttribGet(ih, "IMAGE");
    if (value)
    {
      GdkPaintable* paintable = (GdkPaintable*)iupImageGetImage(value, ih, 0, NULL);
      if (paintable)
      {
        int pw = gdk_paintable_get_intrinsic_width(paintable);
        int ph = gdk_paintable_get_intrinsic_height(paintable);

        /* Use GtkPicture which renders paintables at exact pixel size */
        gtk_picture_set_paintable(GTK_PICTURE(image), paintable);

        /* Set content_fit to prevent scaling up */
        gtk_picture_set_content_fit(GTK_PICTURE(image), GTK_CONTENT_FIT_SCALE_DOWN);

        /* Don't set size_request - let GtkPicture use paintable's intrinsic size */
        (void)pw;
        (void)ph;
      }
    }
  }
  else
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (!title)
    {
      if (iupAttribGet(ih, "BGCOLOR"))
      {
        GtkWidget* drawarea = gtk_drawing_area_new();
        gtk_widget_set_size_request(drawarea, 20, 20);
        gtk_button_set_child(GTK_BUTTON(ih->handle), drawarea);
      }
      else
        gtk_button_set_label(GTK_BUTTON(ih->handle), "");
    }
    else
      gtk_button_set_label(GTK_BUTTON(ih->handle), "");
  }

  iupgtk4AddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtk4SetCanFocus(ih->handle, 0);

  iupgtk4SetupFocusEvents(ih->handle, ih);
  iupgtk4SetupKeyEvents(ih->handle, ih);
  iupgtk4SetupEnterLeaveEvents(ih->handle, ih);

  /* Setup click handler - attach gesture directly to button with CAPTURE phase */
  if (GTK_IS_BUTTON(ih->handle))
  {
    /* Create GtkGestureClick for press/release events */
    GtkGesture* gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);  /* 0 = all buttons */

    /* Set propagation phase to CAPTURE to intercept events BEFORE GtkButton's internal gesture claims them */
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(gesture), GTK_PHASE_CAPTURE);

    /* Attach gesture to the button widget itself */
    gtk_widget_add_controller(ih->handle, GTK_EVENT_CONTROLLER(gesture));

    /* Connect both pressed and released signals */
    g_signal_connect(gesture, "pressed", G_CALLBACK(gtk4ButtonPressed), ih);
    g_signal_connect(gesture, "released", G_CALLBACK(gtk4ButtonReleased), ih);

    /* Also connect "clicked" signal for ACTION callback */
    g_signal_connect(G_OBJECT(ih->handle), "clicked", G_CALLBACK(gtk4ButtonClicked), ih);
  }
  else
  {
    /* For GtkBox (borderless image button), add a gesture click controller */
    GtkGesture* gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_PRIMARY);

    /* Connect gesture signals */
    g_signal_connect(gesture, "begin", G_CALLBACK(gtk4ButtonGestureBegin), ih);
    g_signal_connect(gesture, "pressed", G_CALLBACK(gtk4ButtonGesturePressed), ih);
    g_signal_connect(gesture, "released", G_CALLBACK(gtk4ButtonGestureReleased), ih);

    gtk_widget_add_controller(ih->handle, GTK_EVENT_CONTROLLER(gesture));

    /* Add motion controller for hover indication on IMAGE+IMPRESS buttons */
    if (ih->data->type == IUP_BUTTON_IMAGE && iupAttribGet(ih, "IMPRESS"))
    {
      GtkEventController* motion = gtk_event_controller_motion_new();
      g_signal_connect(motion, "enter", G_CALLBACK(gtk4ButtonMotionEnter), ih);
      g_signal_connect(motion, "leave", G_CALLBACK(gtk4ButtonMotionLeave), ih);
      gtk_widget_add_controller(ih->handle, motion);
    }
  }

  gtk_widget_realize(ih->handle);

  iupgtk4UpdateMnemonic(ih);

  return IUP_NOERROR;
}

void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = gtk4ButtonMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, gtk4ButtonSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, gtk4ButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtk4ButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtk4ButtonSetFgColorAttrib, "DLGFGCOLOR", NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TITLE", NULL, gtk4ButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtk4ButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, gtk4ButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, gtk4ButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtk4ButtonSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ACENTER:ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEPOSITION", NULL, NULL, IUPAF_SAMEASSYSTEM, "LEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FLAT", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "CANFOCUS", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
}
