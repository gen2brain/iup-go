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
  /* LAYOUT_DECORATION_ESTIMATE */
#ifdef WIN32
  int border_size = 2*5;
#else
#ifdef HILDON
  int border_size = 2*7+1; /* borders are not symmetric */
#else
  int border_size = 2*5+1; /* borders are not symmetric */
#endif
#endif
  (void)ih;  
  (*x) += border_size;
  (*y) += border_size;
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
    /* when both is set, button contains an GtkAlignment, 
       that contains a GtkBox, that contains a label and an image */
    GtkContainer *container = (GtkContainer*)gtk_bin_get_child((GtkBin*)gtk_bin_get_child((GtkBin*)ih->handle));
    GtkLabel* label = NULL;
    gtk_container_foreach(container, gtkButtonChildrenCb, &label);
    return label;
  }
  else
    return NULL;
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

  gtk_button_set_alignment(button, xalign, yalign);
  /* TODO:   g_object_set(widget, "xalign", xalign, "yalign", yalign, NULL); */

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
    if (ih->data->type == IUP_BUTTON_TEXT)   /* text only */
    {
      GtkMisc* misc = (GtkMisc*)gtk_bin_get_child((GtkBin*)ih->handle);
      gtk_misc_set_padding(misc, ih->data->horiz_padding, ih->data->vert_padding);
    }
    else
    {
      GtkAlignment* alignment = (GtkAlignment*)gtk_bin_get_child((GtkBin*)ih->handle);
      if (GTK_IS_ALIGNMENT(alignment))
        gtk_alignment_set_padding(alignment, ih->data->vert_padding, ih->data->vert_padding, 
                                             ih->data->horiz_padding, ih->data->horiz_padding);
    }
#endif
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int gtkButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_BUTTON_TEXT)
  {
    /* Color button */
    GtkWidget* frame = gtk_button_get_image(GTK_BUTTON(ih->handle));
    if (frame && GTK_IS_FRAME(frame))
    {
      unsigned char r, g, b;
      if (!iupStrToRGB(value, &r, &g, &b))
        return 0;

      iupgtkSetBgColor(gtk_bin_get_child(GTK_BIN(frame)), r, g, b);
      return 1;
    }
  }

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

  if (ih->data->type == IUP_BUTTON_TEXT)
  {
    /* Color button */
    GtkWidget* frame = gtk_button_get_image(GTK_BUTTON(ih->handle));
    if (frame && GTK_IS_FRAME(frame))
    {
      int x = 0, y = 0;
      iupdrvButtonAddBorders(ih, &x, &y);
      if (ih->currentwidth - x > 0 && ih->currentheight - y > 0)
        gtk_widget_set_size_request(frame, ih->currentwidth-x, ih->currentheight-y);
    }
  }
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
    if (!title) 
    {
      if (iupAttribGet(ih, "BGCOLOR"))
      {
        GtkWidget* frame = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(2, 18, 0)
        GtkWidget* drawarea = gtk_drawing_area_new();
        gtk_widget_set_has_window(drawarea, TRUE);
#else
        GtkWidget* drawarea = gtk_fixed_new();
        gtk_fixed_set_has_window(GTK_FIXED(drawarea), TRUE);
#endif
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
        gtk_container_add(GTK_CONTAINER(frame), drawarea);
        gtk_widget_show(drawarea);

        gtk_button_set_image((GtkButton*)ih->handle, frame);
      }
      else
        gtk_button_set_label((GtkButton*)ih->handle, "");
    }
    else
      gtk_button_set_label((GtkButton*)ih->handle, "");
  }

  if (has_border)
    iupgtkClearSizeStyleCSS(ih->handle);

  /* add to the parent, all GTK controls must call this. */
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

  gtk_widget_realize(ih->handle);

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
