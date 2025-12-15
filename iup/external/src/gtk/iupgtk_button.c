/** \file
 * \brief Button Control
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

#include "iupgtk_drv.h"


#if !GTK_CHECK_VERSION(2, 6, 0)
static void gtk_button_set_image(GtkButton *button, GtkWidget *image)
{
}
static GtkWidget* gtk_button_get_image(GtkButton *button)
{
  return NULL;
}
#endif

void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  /* Measure actual borders dynamically since they depend on theme */
  static int text_border_x = -1, text_border_y = -1;
  static int image_text_border_x = -1, image_text_border_y = -1;
  static int image_border_x = -1, image_border_y = -1;

  int has_image = 0;
  int has_text = 0;
  int has_bgcolor = 0;

  if (ih)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    char* title = iupAttribGet(ih, "TITLE");
    char* bgcolor = iupAttribGet(ih, "BGCOLOR");
    has_image = (image != NULL);
    has_text = (title != NULL && *title != 0);
    has_bgcolor = (!has_image && !has_text && bgcolor != NULL);
  }

  if (has_bgcolor)
  {
    GtkWidget* temp_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget* temp_button = gtk_button_new();

    gtk_container_add(GTK_CONTAINER(temp_window), temp_button);
    gtk_widget_show_all(temp_window);
    gtk_widget_realize(temp_window);
    gtk_widget_realize(temp_button);

    gint button_nat_w, button_nat_h;
    gtk_widget_get_preferred_width(temp_button, NULL, &button_nat_w);
    gtk_widget_get_preferred_height(temp_button, NULL, &button_nat_h);

    /* Empty button borders */
    (*x) += button_nat_w;
    (*y) += button_nat_h;

    gtk_widget_destroy(temp_window);
  }
  else if (has_image && has_text)
  {
    if (image_text_border_x == -1)
    {
      GtkWidget* temp_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      GtkWidget* temp_button = gtk_button_new();
      GtkWidget* temp_image = gtk_image_new_from_icon_name("document-open", GTK_ICON_SIZE_BUTTON);

      gtk_button_set_image(GTK_BUTTON(temp_button), temp_image);
      gtk_button_set_label(GTK_BUTTON(temp_button), "Test");
      gtk_button_set_always_show_image(GTK_BUTTON(temp_button), TRUE);

      gtk_container_add(GTK_CONTAINER(temp_window), temp_button);
      gtk_widget_show_all(temp_window);
      gtk_widget_realize(temp_window);
      gtk_widget_realize(temp_button);

      /* Get button's preferred size */
      gint button_width, button_height;
      gtk_widget_get_preferred_width(temp_button, NULL, &button_width);
      gtk_widget_get_preferred_height(temp_button, NULL, &button_height);

      /* Get child allocation */
      GtkWidget* child = gtk_bin_get_child(GTK_BIN(temp_button));
      GtkAllocation child_alloc;
      if (child)
      {
        gtk_widget_get_allocation(child, &child_alloc);
        image_text_border_x = button_width - child_alloc.width;
        image_text_border_y = button_height - child_alloc.height;
      }
      else
      {
        image_text_border_x = 11;
        image_text_border_y = 11;
      }

      gtk_widget_destroy(temp_window);
    }
    (*x) += image_text_border_x;
    (*y) += image_text_border_y;
  }
  else if (has_image)
  {
    if (image_border_x == -1)
    {
      GtkWidget* temp_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      GtkWidget* temp_button = gtk_button_new();
      GtkWidget* temp_image = gtk_image_new_from_icon_name("document-open", GTK_ICON_SIZE_BUTTON);

      gtk_button_set_image(GTK_BUTTON(temp_button), temp_image);

      gtk_container_add(GTK_CONTAINER(temp_window), temp_button);
      gtk_widget_show_all(temp_window);
      gtk_widget_realize(temp_window);
      gtk_widget_realize(temp_button);

      /* Get button's preferred size */
      gint button_width, button_height;
      gtk_widget_get_preferred_width(temp_button, NULL, &button_width);
      gtk_widget_get_preferred_height(temp_button, NULL, &button_height);

      /* Get child allocation */
      GtkWidget* child = gtk_bin_get_child(GTK_BIN(temp_button));
      GtkAllocation child_alloc;
      if (child)
      {
        gtk_widget_get_allocation(child, &child_alloc);
        image_border_x = button_width - child_alloc.width;
        image_border_y = button_height - child_alloc.height;
      }
      else
      {
        image_border_x = 11;
        image_border_y = 11;
      }

      gtk_widget_destroy(temp_window);
    }
    (*x) += image_border_x;
    (*y) += image_border_y;
  }
  else
  {
    if (text_border_x == -1)
    {
      GtkWidget* temp_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      GtkWidget* temp_button = gtk_button_new_with_label("Test");

      gtk_container_add(GTK_CONTAINER(temp_window), temp_button);
      gtk_widget_show_all(temp_window);
      gtk_widget_realize(temp_window);
      gtk_widget_realize(temp_button);

      gint button_nat_w, button_nat_h;
      gtk_widget_get_preferred_width(temp_button, NULL, &button_nat_w);
      gtk_widget_get_preferred_height(temp_button, NULL, &button_nat_h);

      GtkWidget* child = gtk_bin_get_child(GTK_BIN(temp_button));
      if (child)
      {
        gint child_nat_w, child_nat_h;
        gtk_widget_get_preferred_width(child, NULL, &child_nat_w);
        gtk_widget_get_preferred_height(child, NULL, &child_nat_h);

        text_border_x = button_nat_w - child_nat_w;
        text_border_y = button_nat_h - child_nat_h;
      }
      else
      {
        text_border_x = 11;
        text_border_y = 11;
      }

      gtk_widget_destroy(temp_window);
    }
    (*x) += text_border_x;
    (*y) += text_border_y;
  }
#else
  /* GTK2: Measure actual borders since they depend on theme AND button type */
  GtkWidget* temp_button;
  GtkWidget* child;
  GtkRequisition button_size, child_size;
  int border_x, border_y;

  /* Check if we need to measure for image+text button, image-only, or text-only button */
  int has_image = 0;
  int has_text = 0;
  int has_bgcolor = 0;

  if (ih)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    char* title = iupAttribGet(ih, "TITLE");
    char* bgcolor = iupAttribGet(ih, "BGCOLOR");
    has_image = (image != NULL);
    has_text = (title != NULL && *title != 0);
    has_bgcolor = (!has_image && !has_text && bgcolor != NULL);
  }

  if (has_bgcolor)
  {
    GtkWidget* temp_window;
    GtkRequisition req;

    temp_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    temp_button = gtk_button_new();
    gtk_widget_set_can_focus(temp_button, TRUE);
    gtk_container_add(GTK_CONTAINER(temp_window), temp_button);
    gtk_widget_show_all(temp_window);
    gtk_widget_realize(temp_window);
    gtk_widget_realize(temp_button);

    gtk_widget_size_request(temp_button, &req);

    (*x) += req.width;
    (*y) += req.height;

    gtk_widget_destroy(temp_window);
    return;
  }
  else if (has_image && has_text)
  {
    /* Create button with both image and text to match actual structure */
    GdkPixbuf* temp_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 32, 32);
    GtkWidget* temp_image = gtk_image_new_from_pixbuf(temp_pixbuf);

    temp_button = gtk_button_new();
    gtk_widget_set_can_focus(temp_button, TRUE);
    gtk_button_set_alignment(GTK_BUTTON(temp_button), 0.5f, 0.5f);
    gtk_button_set_image(GTK_BUTTON(temp_button), temp_image);
    gtk_button_set_label(GTK_BUTTON(temp_button), "Test");

    /* Get the alignment child that contains the box */
    child = gtk_bin_get_child(GTK_BIN(temp_button));

    gtk_widget_size_request(temp_button, &button_size);
    gtk_widget_size_request(child, &child_size);

    g_object_unref(temp_pixbuf);
  }
  else if (has_image)
  {
    /* Image-only button */
    GdkPixbuf* temp_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 32, 32);
    GtkWidget* temp_image = gtk_image_new_from_pixbuf(temp_pixbuf);

    temp_button = gtk_button_new();
    gtk_widget_set_can_focus(temp_button, TRUE);
    gtk_button_set_alignment(GTK_BUTTON(temp_button), 0.5f, 0.5f);
    gtk_button_set_image(GTK_BUTTON(temp_button), temp_image);

    /* For image-only buttons, we need to measure the GtkAlignment, not the image */
    child = gtk_bin_get_child(GTK_BIN(temp_button));

    gtk_widget_size_request(temp_button, &button_size);
    gtk_widget_size_request(child, &child_size);

    g_object_unref(temp_pixbuf);
  }
  else
  {
    /* Text-only button */
    temp_button = gtk_button_new_with_label("Test");
    gtk_widget_set_can_focus(temp_button, TRUE);
    gtk_button_set_alignment(GTK_BUTTON(temp_button), 0.5f, 0.5f);
    child = gtk_bin_get_child(GTK_BIN(temp_button));

    gtk_widget_size_request(temp_button, &button_size);
    gtk_widget_size_request(child, &child_size);
  }

  /* Force allocation to see what GTK2 actually gives to the child */
  GtkAllocation alloc = {0, 0, button_size.width, button_size.height};
  gtk_widget_size_allocate(temp_button, &alloc);

  /* Now get the ACTUAL allocated size of the child */
  GtkWidget* allocated_child = gtk_bin_get_child(GTK_BIN(temp_button));
  GtkAllocation child_alloc;
  if (allocated_child)
  {
    gtk_widget_get_allocation(allocated_child, &child_alloc);

    /* Calculate border from allocated child size */
    border_x = button_size.width - child_alloc.width;
    border_y = button_size.height - child_alloc.height;

    GtkBorder *inner_border = NULL;
    gtk_widget_style_get(temp_button, "inner-border", &inner_border, NULL);

    if (!inner_border)
    {
      border_x += 6;
      border_y += 6;
    }

    if (inner_border)
      gtk_border_free(inner_border);

  }
  else
  {
    /* Fallback to requested size if no child */
    border_x = button_size.width - child_size.width;
    border_y = button_size.height - child_size.height;
  }

  (*x) += border_x;
  (*y) += border_y;

  gtk_widget_destroy(temp_button);
#endif
}

static void gtkButtonChildrenCb(GtkWidget *widget, gpointer client_data)
{
  if (GTK_IS_LABEL(widget))
  {
    GtkLabel **label = (GtkLabel**) client_data;
    *label = (GtkLabel*)widget;
  }
}

static GtkLabel* gtkButtonGetLabel(Ihandle* ih)
{
  if (ih->data->type == IUP_BUTTON_TEXT)  /* text only */
  {
    GtkWidget* child = gtk_bin_get_child((GtkBin*)ih->handle);
    if (GTK_IS_LABEL(child))
      return (GtkLabel*)child;
    else
      return NULL;
  }
  else if (ih->data->type == IUP_BUTTON_BOTH) /* both */
  {
    /* when both is set, button contains an GtkAlignment, that contains a GtkBox, that contains a label and an image */
    GtkContainer *container = (GtkContainer*)gtk_bin_get_child((GtkBin*)gtk_bin_get_child((GtkBin*)ih->handle));
    GtkLabel* label = NULL;
    gtk_container_foreach(container, gtkButtonChildrenCb, &label);
    return label;
  }
  else
    return NULL;
}

static void gtkButtonFixVerticalAlignment(Ihandle* ih)
{
  /* GTK3 uses BASELINE alignment for image+text buttons, which doesn't vertically
     center the image and text relative to each other. Fix this by setting CENTER alignment. */
  if (ih->data->type == IUP_BUTTON_BOTH)
  {
    GtkWidget* align = gtk_bin_get_child((GtkBin*)ih->handle);
    if (align && GTK_IS_BIN(align))
    {
      GtkWidget* box = gtk_bin_get_child((GtkBin*)align);
      if (box && GTK_IS_BOX(box))
      {
        GList* children = gtk_container_get_children(GTK_CONTAINER(box));
        GList* l;
        for (l = children; l != NULL; l = l->next)
        {
          GtkWidget* child = GTK_WIDGET(l->data);
#if GTK_CHECK_VERSION(3, 0, 0)
          gtk_widget_set_valign(child, GTK_ALIGN_CENTER);
#else
          if (GTK_IS_MISC(child))
            gtk_misc_set_alignment(GTK_MISC(child), 0.5, 0.5);
#endif
        }
        g_list_free(children);
      }
    }
  }
}

static int gtkButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_TEXT)  /* text or both */
  {
    GtkLabel* label = gtkButtonGetLabel(ih);
    if (label)
    {
      iupgtkSetMnemonicTitle(ih, label, value);
      return 1;
    }
  }

  return 0;
}

static int gtkButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  GtkButton* button = (GtkButton*)ih->handle;
  PangoAlignment alignment;
  float xalign, yalign;
  char value1[30], value2[30];

  if (iupAttribGet(ih, "_IUPGTK_EVENTBOX"))
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
  else /* ACENTER (default) */
  {
    xalign = 0.5f;
    alignment = PANGO_ALIGN_CENTER;
  }

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    yalign = 1.0f;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    yalign = 0;
  else  /* ACENTER (default) */
    yalign = 0.5f;

#if GTK_CHECK_VERSION(3, 14, 0)
  /* gtk_button_set_alignment() deprecated in 3.14 */
  g_object_set(G_OBJECT(button), "xalign", xalign, "yalign", yalign, NULL);
#else
  gtk_button_set_alignment(button, xalign, yalign);
#endif

  if (ih->data->type == IUP_BUTTON_TEXT)   /* text only */
  {
    GtkLabel* label = gtkButtonGetLabel(ih);
    if (label)
    {
      PangoLayout* layout = gtk_label_get_layout(label);
      if (layout) 
        pango_layout_set_alignment(layout, alignment);
    }
  }

  return 1;
}

static int gtkButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle)
  {
#if GTK_CHECK_VERSION(3, 4, 0)
    iupgtkSetMargin(ih->handle, ih->data->horiz_padding, ih->data->vert_padding, 0);
#else
    (void)ih; /* Padding already handled in size calculation */
#endif
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean gtkButtonColorDrawAreaDraw(GtkWidget* widget, cairo_t* cr, Ihandle* ih)
{
  GtkStyleContext* context = gtk_widget_get_style_context(widget);
  int width = gtk_widget_get_allocated_width(widget);
  int height = gtk_widget_get_allocated_height(widget);

  (void)ih;
  gtk_render_background(context, cr, 0, 0, width, height);

  return FALSE;
}

static void gtkButtonColorDrawAreaRealize(GtkWidget* widget, Ihandle* ih)
{
  char* value = iupAttribGet(ih, "BGCOLOR");
  if (value)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(value, &r, &g, &b))
      iupgtkSetBgColor(widget, r, g, b);
  }
}
#else
static gboolean gtkButtonColorExposeAfter(GtkWidget* widget, GdkEventExpose* evt, Ihandle* ih)
{
  GtkAllocation allocation;
  cairo_t* cr;
  GdkWindow* window;
  char* value;
  unsigned char r, g, b;
  int x, y, w, h;
  gint xthickness, ythickness;

  (void)evt;

  value = iupAttribGet(ih, "BGCOLOR");
  if (!value || !iupStrToRGB(value, &r, &g, &b))
    return FALSE;

  window = gtk_widget_get_window(widget);
  if (!window)
    return FALSE;

  gtk_widget_get_allocation(widget, &allocation);

  /* Draw inside the button frame (just inside xthickness/ythickness) */
  xthickness = GTK_WIDGET(widget)->style->xthickness;
  ythickness = GTK_WIDGET(widget)->style->ythickness;

  /* GTK2 buttons use parent's window, so we need allocation offset */
  x = allocation.x + xthickness;
  y = allocation.y + ythickness;
  w = allocation.width - 2 * xthickness;
  h = allocation.height - 2 * ythickness;

  if (w > 0 && h > 0)
  {
    cr = gdk_cairo_create(window);
    cairo_set_source_rgb(cr, r / 255.0, g / 255.0, b / 255.0);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
    cairo_destroy(cr);
  }

  return FALSE;
}
#endif

static int gtkButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  if (ih->data->type == IUP_BUTTON_TEXT)
  {
    /* Color button with frame+drawarea */
    GtkWidget* frame = gtk_bin_get_child(GTK_BIN(ih->handle));
    if (frame && GTK_IS_FRAME(frame))
    {
      unsigned char r, g, b;
      if (!iupStrToRGB(value, &r, &g, &b))
        return 0;

      GtkWidget* drawarea = gtk_bin_get_child(GTK_BIN(frame));
      if (gtk_widget_get_realized(drawarea))
        iupgtkSetBgColor(drawarea, r, g, b);
      return 1;
    }
  }
#else
  if (ih->data->type == IUP_BUTTON_TEXT)
  {
    char* bgcolor = iupAttribGet(ih, "BGCOLOR");
    if (bgcolor)
    {
      gtk_widget_queue_draw(ih->handle);
      return 1;
    }
  }
#endif

  return iupdrvBaseSetBgColorAttrib(ih, value);
}

static int gtkButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  GtkLabel* label = gtkButtonGetLabel(ih);
  if (!label) return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkSetFgColor((GtkWidget*)label, r, g, b);

  return 1;
}

static int gtkButtonSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    GtkLabel* label = gtkButtonGetLabel(ih);
    if (label)
      iupgtkUpdateWidgetFont(ih, (GtkWidget*)label);
  }
  return 1;
}

static void gtkButtonSetPixbuf(Ihandle* ih, const char* name, int make_inactive)
{
  GtkImage* image;
  if (!iupAttribGet(ih, "_IUPGTK_EVENTBOX"))
    image = (GtkImage*)gtk_button_get_image((GtkButton*)ih->handle);
  else
    image = (GtkImage*)gtk_bin_get_child((GtkBin*)ih->handle);

  if (name && image)
  {
    GdkPixbuf* pixbuf = iupImageGetImage(name, ih, make_inactive, NULL);
    GdkPixbuf* old_pixbuf = gtk_image_get_pixbuf(image);
    if (pixbuf != old_pixbuf)
      gtk_image_set_from_pixbuf(image, pixbuf);
    return;
  }

  /* if not defined */
#if GTK_CHECK_VERSION(2, 8, 0)
  if (image)
    gtk_image_clear(image);
#endif
}

static int gtkButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (iupdrvIsActive(ih))
      gtkButtonSetPixbuf(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
      {
        /* if not active and IMINACTIVE is not defined 
           then automatically create one based on IMAGE */
        gtkButtonSetPixbuf(ih, value, 1); /* make_inactive */
      }
    }
    return 1;
  }
  else
    return 0;
}

static int gtkButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        gtkButtonSetPixbuf(ih, value, 0);
      else
      {
        /* if not defined then automatically create one based on IMAGE */
        char* name = iupAttribGet(ih, "IMAGE");
        gtkButtonSetPixbuf(ih, name, 1); /* make_inactive */
      }
    }
    return 1;
  }
  else
    return 0;
}

static int gtkButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* update the inactive image if necessary */
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        gtkButtonSetPixbuf(ih, name, 0);
      else
      {
        /* if not defined then automatically create one based on IMAGE */
        name = iupAttribGet(ih, "IMAGE");
        gtkButtonSetPixbuf(ih, name, 1); /* make_inactive */
      }
    }
    else
    {
      /* must restore the normal image */
      char* name = iupAttribGet(ih, "IMAGE");
      gtkButtonSetPixbuf(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static gboolean gtkButtonEnterLeaveEvent(GtkWidget *widget, GdkEventCrossing *evt, Ihandle *ih)
{
  /* Used when FLAT=Yes, to manage relief */

  iupgtkEnterLeaveEvent(widget, evt, ih);
  (void)widget;

  if (evt->type == GDK_ENTER_NOTIFY)
    gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NORMAL);
  else  if (evt->type == GDK_LEAVE_NOTIFY)
    gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NONE);

  return FALSE;
}

static void gtkButtonClicked(GtkButton *widget, Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE) 
      IupExitLoop();
  }
  (void)widget;
}

static gboolean gtkButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih)
{
  if (iupgtkButtonEvent(widget, evt, ih)==TRUE)
    return TRUE;

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* name = iupAttribGet(ih, "IMPRESS");
    if (name)
    {
      if (evt->type == GDK_BUTTON_PRESS)
        gtkButtonSetPixbuf(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        gtkButtonSetPixbuf(ih, name, 0);
      }
    }

    if (evt->type == GDK_BUTTON_RELEASE &&
        iupAttribGet(ih, "_IUPGTK_EVENTBOX"))
    {
      Icallback cb = IupGetCallback(ih, "ACTION");
      if (cb)
      {
        if (cb(ih) == IUP_CLOSE) 
          IupExitLoop();
      }
    }
  }

  return FALSE;
}

static void gtkButtonLayoutUpdateMethod(Ihandle *ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
}

static int gtkButtonMapMethod(Ihandle* ih)
{
  int has_border = 1;

  char* value = iupAttribGet(ih, "IMAGE");
  if (value)
  {
    ih->data->type = IUP_BUTTON_IMAGE;

    value = iupAttribGet(ih, "TITLE");
    if (value && *value!=0)
      ih->data->type |= IUP_BUTTON_TEXT;
  }
  else
    ih->data->type = IUP_BUTTON_TEXT;

  if (ih->data->type == IUP_BUTTON_IMAGE &&
      iupAttribGet(ih, "IMPRESS") &&
      !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
    has_border = 0;

  if (!has_border)
  {
    GtkWidget *img = gtk_image_new();
    ih->handle = gtk_event_box_new();
    gtk_container_add((GtkContainer*)ih->handle, img);
    gtk_widget_show(img);
    iupAttribSet(ih, "_IUPGTK_EVENTBOX", "1");
  }
  else
    ih->handle = gtk_button_new();

  if (!ih->handle)
    return IUP_ERROR;

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (has_border)
    {
      gtk_button_set_image((GtkButton*)ih->handle, gtk_image_new());

      if (ih->data->type & IUP_BUTTON_TEXT)
      {
#if GTK_CHECK_VERSION(3, 6, 0)
        gtk_button_set_always_show_image((GtkButton*)ih->handle, TRUE);
#else
        GtkSettings* settings = gtk_widget_get_settings(ih->handle);
        g_object_set(settings, "gtk-button-images", (int)TRUE, NULL);
#endif

#if GTK_CHECK_VERSION(2, 10, 0)
        gtk_button_set_image_position((GtkButton*)ih->handle, ih->data->img_position);  /* IUP and GTK have the same Ids */
#endif

        gtk_button_set_label((GtkButton*)ih->handle, "");
      }
    }
  }
  else
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (!title || *title == 0)
    {
      if (iupAttribGet(ih, "BGCOLOR"))
      {
#if GTK_CHECK_VERSION(3, 0, 0)
        GtkWidget* frame = gtk_frame_new(NULL);
        GtkWidget* drawarea = gtk_drawing_area_new();
        gtk_widget_set_has_window(drawarea, TRUE);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
        gtk_container_add(GTK_CONTAINER(frame), drawarea);
        gtk_widget_show(drawarea);

        g_signal_connect(G_OBJECT(drawarea), "realize", G_CALLBACK(gtkButtonColorDrawAreaRealize), ih);
        g_signal_connect(G_OBJECT(drawarea), "draw", G_CALLBACK(gtkButtonColorDrawAreaDraw), ih);

        /* Add frame directly as button's child so it can expand to fill */
        gtk_container_add(GTK_CONTAINER(ih->handle), frame);
        gtk_widget_show(frame);
#else
        /* GTK2: Draw directly on button via expose-event */
        g_signal_connect_after(G_OBJECT(ih->handle), "expose-event", G_CALLBACK(gtkButtonColorExposeAfter), ih);
#endif
      }
      else
        gtk_button_set_label((GtkButton*)ih->handle, "");
    }
    else
      gtk_button_set_label((GtkButton*)ih->handle, "");
  }

  (void)has_border;

  iupgtkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtkSetCanFocus(ih->handle, 0);

  if (has_border && iupAttribGetBoolean(ih, "FLAT"))
  {
    gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NONE);

    g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(gtkButtonEnterLeaveEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(gtkButtonEnterLeaveEvent), ih);
  }
  else
  {
    if (has_border)
      gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NORMAL);

    g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  }

  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(iupgtkKeyPressEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);

  if (!iupAttribGet(ih, "_IUPGTK_EVENTBOX"))
    g_signal_connect(G_OBJECT(ih->handle), "clicked", G_CALLBACK(gtkButtonClicked), ih);

  g_signal_connect(G_OBJECT(ih->handle), "button-press-event", G_CALLBACK(gtkButtonEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "button-release-event",G_CALLBACK(gtkButtonEvent), ih);

  if (!has_border)
  {
    GtkWidget* img = gtk_bin_get_child((GtkBin*)ih->handle);
    gtk_widget_realize(img);
  }

#if !GTK_CHECK_VERSION(3, 0, 0)
  /* GTK2: Apply button alignment before realize to actually center content. */
  if (has_border && !iupAttribGet(ih, "_IUPGTK_EVENTBOX"))
  {
    gtk_button_set_alignment((GtkButton*)ih->handle, 0.5f, 0.5f);
  }
#endif

  gtk_widget_realize(ih->handle);

  /* Fix vertical alignment for image+text buttons */
  gtkButtonFixVerticalAlignment(ih);

  /* update a mnemonic in a label if necessary */
  iupgtkUpdateMnemonic(ih);

  return IUP_NOERROR;
}

void iupdrvButtonInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkButtonMapMethod;
  ic->LayoutUpdate = gtkButtonLayoutUpdateMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "FONT", NULL, gtkButtonSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, gtkButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkButtonSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtkButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupButton only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtkButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtkButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, gtkButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, gtkButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}
