/** \file
 * \brief Toggle Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3, 0, 0)
#include <gdk/gdkkeysyms-compat.h>
#endif

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

#ifdef HILDON
#define IUP_TOGGLE_BOX 30
#else
#if GTK_CHECK_VERSION(3, 0, 0)
#define IUP_TOGGLE_BOX 18
#else
#define IUP_TOGGLE_BOX 16
#endif
#endif

#if !GTK_CHECK_VERSION(3, 0, 0)
/* GTK2 Switch dimensions (GTK3-style flat rectangle) */
#define SWITCH_TRACK_WIDTH  48
#define SWITCH_TRACK_HEIGHT 22
#define SWITCH_THUMB_WIDTH  22
#define SWITCH_THUMB_HEIGHT 18
#define SWITCH_THUMB_MARGIN 2

typedef struct _IupGtkSwitchData
{
  Ihandle* ih;
  int checked_state;
  GtkWidget* drawing_area;
} IupGtkSwitchData;

static void gtkSwitchDraw(Ihandle* ih, IupGtkSwitchData* switch_data, GdkWindow* window, GdkGC* gc)
{
  int is_checked = switch_data->checked_state;
  int is_active = iupdrvIsActive(ih);
  int thumb_x, thumb_y;
  GtkStyle* style;
  GdkColor track_color, thumb_color, border_color;
  GdkColormap* colormap;

  style = gtk_widget_get_style(ih->handle);
  colormap = gtk_widget_get_colormap(ih->handle);

  if (!is_active)
  {
    track_color = style->bg[GTK_STATE_INSENSITIVE];
    thumb_color = style->light[GTK_STATE_INSENSITIVE];
    border_color = style->dark[GTK_STATE_INSENSITIVE];
  }
  else if (is_checked)
  {
    track_color = style->bg[GTK_STATE_SELECTED];
    thumb_color = style->light[GTK_STATE_NORMAL];
    border_color = style->dark[GTK_STATE_SELECTED];
  }
  else
  {
    track_color = style->dark[GTK_STATE_NORMAL];
    thumb_color = style->light[GTK_STATE_NORMAL];
    border_color = style->dark[GTK_STATE_ACTIVE];
  }

  gdk_colormap_alloc_color(colormap, &track_color, FALSE, TRUE);
  gdk_colormap_alloc_color(colormap, &thumb_color, FALSE, TRUE);
  gdk_colormap_alloc_color(colormap, &border_color, FALSE, TRUE);

  /* Draw track (filled rectangle) */
  gdk_gc_set_foreground(gc, &track_color);
  gdk_draw_rectangle(window, gc, TRUE, 0, 0, SWITCH_TRACK_WIDTH, SWITCH_TRACK_HEIGHT);

  /* Draw track border */
  gdk_gc_set_foreground(gc, &border_color);
  gdk_draw_rectangle(window, gc, FALSE, 0, 0, SWITCH_TRACK_WIDTH - 1, SWITCH_TRACK_HEIGHT - 1);

  /* Calculate thumb position */
  if (is_checked)
    thumb_x = SWITCH_TRACK_WIDTH - SWITCH_THUMB_WIDTH - SWITCH_THUMB_MARGIN;
  else
    thumb_x = SWITCH_THUMB_MARGIN;

  thumb_y = (SWITCH_TRACK_HEIGHT - SWITCH_THUMB_HEIGHT) / 2;

  /* Draw thumb (filled rectangle) */
  gdk_gc_set_foreground(gc, &thumb_color);
  gdk_draw_rectangle(window, gc, TRUE, thumb_x, thumb_y, SWITCH_THUMB_WIDTH, SWITCH_THUMB_HEIGHT);

  /* Draw thumb border */
  gdk_gc_set_foreground(gc, &border_color);
  gdk_draw_rectangle(window, gc, FALSE, thumb_x, thumb_y, SWITCH_THUMB_WIDTH - 1, SWITCH_THUMB_HEIGHT - 1);
}

static gboolean gtkSwitchExposeEvent(GtkWidget* widget, GdkEventExpose* event, IupGtkSwitchData* switch_data)
{
  Ihandle* ih = switch_data->ih;
  GdkGC* gc;

  (void)event;

  gc = gdk_gc_new(widget->window);
  if (!gc)
    return FALSE;

  gtkSwitchDraw(ih, switch_data, widget->window, gc);

  g_object_unref(gc);

  return FALSE;
}

static gboolean gtkSwitchButtonPressEvent(GtkWidget* widget, GdkEventButton* event, Ihandle* ih)
{
  IupGtkSwitchData* switch_data;
  IFni cb;
  int new_check;

  (void)widget;
  (void)event;

  switch_data = (IupGtkSwitchData*)iupAttribGet(ih, "_IUPGTK_SWITCHDATA");
  if (!switch_data)
    return FALSE;

  new_check = switch_data->checked_state ? 0 : 1;
  switch_data->checked_state = new_check;

  gtk_widget_queue_draw(switch_data->drawing_area);

  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb && cb(ih, new_check) == IUP_CLOSE)
    IupExitLoop();

  if (iupObjectCheck(ih))
    iupBaseCallValueChangedCb(ih);

  return TRUE;
}

static void gtkSwitchDestroyCallback(GtkWidget* widget, Ihandle* ih)
{
  IupGtkSwitchData* switch_data = (IupGtkSwitchData*)iupAttribGet(ih, "_IUPGTK_SWITCHDATA");
  (void)widget;

  if (switch_data)
  {
    free(switch_data);
    iupAttribSet(ih, "_IUPGTK_SWITCHDATA", NULL);
  }
}
#endif

void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y)
{
  iupdrvButtonAddBorders(ih, x, y);
}

void iupdrvToggleAddSwitch(Ihandle* ih, int *x, int *y, const char* str)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  static int switch_w = -1;
  static int switch_h = -1;
  (void)ih;

  if (switch_w < 0)
  {
    GtkWidget* temp_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget* temp_switch = gtk_switch_new();
    GtkAllocation allocation;
    int min_w, nat_w, min_h, nat_h;

    /* Add to window, show, and realize to get actual allocated size */
    gtk_container_add(GTK_CONTAINER(temp_window), temp_switch);
    gtk_widget_show_all(temp_window);
    gtk_widget_realize(temp_window);
    gtk_widget_realize(temp_switch);

    /* Force size allocation */
    gtk_widget_get_preferred_width(temp_switch, &min_w, &nat_w);
    gtk_widget_get_preferred_height(temp_switch, &min_h, &nat_h);

    /* Get the actual allocated size after realization */
    gtk_widget_get_allocation(temp_switch, &allocation);

    /* Use allocated size with fallback */
    switch_w = (allocation.width > 0) ? allocation.width : 48;
    switch_h = (allocation.height > 0) ? allocation.height : 24;

    gtk_widget_destroy(temp_window);
  }

  (*x) += 2 + switch_w + 2;
  if ((*y) < 2 + switch_h + 2) (*y) = 2 + switch_h + 2;
  else (*y) += 2+2;

  if (str && str[0])
    (*x) += 8;
#else
  /* GTK2: Use fixed dimensions matching our custom drawing */
  (void)ih;

  (*x) += 2 + SWITCH_TRACK_WIDTH + 2;
  if ((*y) < 2 + SWITCH_TRACK_HEIGHT + 2) (*y) = 2 + SWITCH_TRACK_HEIGHT + 2;
  else (*y) += 2+2;

  if (str && str[0])
    (*x) += 8;
#endif
}

void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* str)
{
  static int check_w = -1;
  static int check_h = -1;
  (void)ih;

  if (check_w < 0)
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkWidget* temp_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget* temp_check = gtk_check_button_new();
    GtkAllocation allocation;
    int min_w, nat_w, min_h, nat_h;

    gtk_container_add(GTK_CONTAINER(temp_window), temp_check);
    gtk_widget_show_all(temp_window);
    gtk_widget_realize(temp_window);
    gtk_widget_realize(temp_check);

    gtk_widget_get_preferred_width(temp_check, &min_w, &nat_w);
    gtk_widget_get_preferred_height(temp_check, &min_h, &nat_h);

    gtk_widget_get_allocation(temp_check, &allocation);

    check_w = (allocation.width > 0) ? allocation.width : IUP_TOGGLE_BOX;
    check_h = (allocation.height > 0) ? allocation.height : IUP_TOGGLE_BOX;

    gtk_widget_destroy(temp_window);
#else
    GtkWidget* temp_check = gtk_check_button_new();
    GtkRequisition requisition;

    gtk_widget_size_request(temp_check, &requisition);

    check_w = (requisition.width > 0) ? requisition.width : IUP_TOGGLE_BOX;
    check_h = (requisition.height > 0) ? requisition.height : IUP_TOGGLE_BOX;

    gtk_widget_destroy(temp_check);
#endif
  }

  (*x) += 2 + check_w + 2;
  if ((*y) < 2 + check_h + 2) (*y) = 2 + check_h + 2;
  else (*y) += 2+2;

  if (str && str[0])
    (*x) += 8;
}

static int gtkToggleGetCheck(Ihandle* ih)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    if (GTK_IS_SWITCH(ih->handle))
        return gtk_switch_get_active(GTK_SWITCH(ih->handle))? 1: 0;
  }
#endif
  if (gtk_toggle_button_get_inconsistent((GtkToggleButton*)ih->handle))
    return -1;
  if (gtk_toggle_button_get_active((GtkToggleButton*)ih->handle))
    return 1;
  else
    return 0;
}

static void gtkToggleSetPixbuf(Ihandle* ih, const char* name, int make_inactive)
{
  GtkButton* button = (GtkButton*)ih->handle;
  GtkImage* image = (GtkImage*)gtk_button_get_image(button);

  if (name)
  {
    GdkPixbuf* pixbuf = iupImageGetImage(name, ih, make_inactive, NULL);
    GdkPixbuf* old_pixbuf = gtk_image_get_pixbuf(image);
    if (pixbuf != old_pixbuf)
      gtk_image_set_from_pixbuf(image, pixbuf);
    return;
  }

  /* if not defined */
#if GTK_CHECK_VERSION(2, 8, 0)
  gtk_image_clear(image);
#endif
}

static void gtkToggleUpdateImage(Ihandle* ih, int active, int check)
{
  char* name;

  if (!active)
  {
    name = iupAttribGet(ih, "IMINACTIVE");
    if (name)
      gtkToggleSetPixbuf(ih, name, 0);
    else
    {
      /* if not defined then automatically create one based on IMAGE */
      name = iupAttribGet(ih, "IMAGE");
      gtkToggleSetPixbuf(ih, name, 1); /* make_inactive */
    }
  }
  else
  {
    /* must restore the normal image */
    if (check)
    {
      name = iupAttribGet(ih, "IMPRESS");
      if (name)
        gtkToggleSetPixbuf(ih, name, 0);
      else
      {
        /* if not defined then automatically create one based on IMAGE */
        name = iupAttribGet(ih, "IMAGE");
        gtkToggleSetPixbuf(ih, name, 0);
      }
    }
    else
    {
      name = iupAttribGet(ih, "IMAGE");
      if (name)
        gtkToggleSetPixbuf(ih, name, 0);
    }
  }
}


/*************************************************************************/


static int gtkToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    if (GTK_IS_SWITCH(ih->handle))
    {
      int check;
      iupAttribSet(ih, "_IUPGTK_IGNORE_TOGGLE", "1");

      if (iupStrEqualNoCase(value,"TOGGLE"))
        check = !gtk_switch_get_active(GTK_SWITCH(ih->handle));
      else
        check = iupStrBoolean(value);

      gtk_switch_set_active(GTK_SWITCH(ih->handle), check);

      iupAttribSet(ih, "_IUPGTK_IGNORE_TOGGLE", NULL);
    }
#else
    IupGtkSwitchData* switch_data = (IupGtkSwitchData*)iupAttribGet(ih, "_IUPGTK_SWITCHDATA");
    if (switch_data)
    {
      int new_check;

      if (iupStrEqualNoCase(value, "TOGGLE"))
        new_check = !switch_data->checked_state;
      else
        new_check = iupStrBoolean(value);

      switch_data->checked_state = new_check;

      if (ih->handle && switch_data->drawing_area)
        gtk_widget_queue_draw(switch_data->drawing_area);
    }
#endif
    return 0;
  }

  if (iupStrEqualNoCase(value,"NOTDEF"))
    gtk_toggle_button_set_inconsistent((GtkToggleButton*)ih->handle, TRUE);
  else
  {
    int check;
    Ihandle* last_ih = NULL;
    Ihandle* radio = iupRadioFindToggleParent(ih);
    gtk_toggle_button_set_inconsistent((GtkToggleButton*)ih->handle, FALSE);

    /* This action causes the toggled signal to be emitted. */
    iupAttribSet(ih, "_IUPGTK_IGNORE_TOGGLE", "1");
    if (radio)
    {
      last_ih = (Ihandle*)IupGetAttribute(radio, "VALUE_HANDLE");
      if (last_ih)
        iupAttribSet(last_ih, "_IUPGTK_IGNORE_TOGGLE", "1");
    }

    if (iupStrEqualNoCase(value,"TOGGLE"))
    {
      if (gtk_toggle_button_get_active((GtkToggleButton*)ih->handle))
        check = 0;
      else
        check = 1;
    }
    else
      check = iupStrBoolean(value);

    if (check)
      gtk_toggle_button_set_active((GtkToggleButton*)ih->handle, TRUE);
    else
    {
      gtk_toggle_button_set_active((GtkToggleButton*)ih->handle, FALSE);

      if (ih->data->type == IUP_TOGGLE_IMAGE && ih->data->flat)
        gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NONE);
    }

    if (ih->data->type == IUP_TOGGLE_IMAGE)
      gtkToggleUpdateImage(ih, iupdrvIsActive(ih), gtkToggleGetCheck(ih));

    iupAttribSet(ih, "_IUPGTK_IGNORE_TOGGLE", NULL);
    if (last_ih)
      iupAttribSet(last_ih, "_IUPGTK_IGNORE_TOGGLE", NULL);
  }

  return 0;
}

static char* gtkToggleGetValueAttrib(Ihandle* ih)
{
#if !GTK_CHECK_VERSION(3, 0, 0)
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupGtkSwitchData* switch_data = (IupGtkSwitchData*)iupAttribGet(ih, "_IUPGTK_SWITCHDATA");
    if (switch_data)
      return iupStrReturnChecked(switch_data->checked_state);
    return iupStrReturnChecked(0);
  }
#endif
  return iupStrReturnChecked(gtkToggleGetCheck(ih));
}

static int gtkToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
    return 0; /* Switch does not have a title */

  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    GtkButton* button = (GtkButton*)ih->handle;
    GtkLabel* label = (GtkLabel*)gtk_button_get_image(button);
    iupgtkSetMnemonicTitle(ih, label, value);
    return 1;
  }

  return 0;
}

static int gtkToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  GtkButton* button = (GtkButton*)ih->handle;
  float xalign, yalign;
  char value1[30], value2[30];

  if (ih->data->type == IUP_TOGGLE_TEXT)
    return 0;

  iupStrToStrStr(value, value1, value2, ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    xalign = 1.0f;
  else if (iupStrEqualNoCase(value1, "ALEFT"))
    xalign = 0;
  else /* "ACENTER" (default) */
    xalign = 0.5f;

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

  return 1;
}

static int gtkToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle && ih->data->type == IUP_TOGGLE_IMAGE)
  {
#if GTK_CHECK_VERSION(3, 4, 0)
    iupgtkSetMargin(ih->handle, ih->data->horiz_padding, ih->data->vert_padding, 0);
#else
    GtkButton* button = (GtkButton*)ih->handle;
    GtkMisc* misc = (GtkMisc*)gtk_button_get_image(button);
    gtk_misc_set_padding(misc, ih->data->horiz_padding, ih->data->vert_padding);
#endif
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int gtkToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  GtkWidget* label;

  if (iupAttribGetBoolean(ih, "SWITCH"))
    return 0; /* Switch does not have an internal label */

  label = (GtkWidget*)gtk_button_get_image((GtkButton*)ih->handle);
  if (!label) return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkSetFgColor(label, r, g, b);

  return 1;
}

static int gtkToggleSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    GtkWidget* label = NULL;

    if (iupAttribGetBoolean(ih, "SWITCH"))
      return 1; /* Switch does not have an internal label, but font attribute must be stored */

    label = gtk_button_get_image((GtkButton*)ih->handle);
    if (label)
      iupgtkUpdateWidgetFont(ih, label);
  }
  return 1;
}

static int gtkToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMAGE"))
      iupAttribSet(ih, "IMAGE", (char*)value);
    gtkToggleUpdateImage(ih, iupdrvIsActive(ih), gtkToggleGetCheck(ih));
    return 1;
  }
  else
    return 0;
}

static int gtkToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMINACTIVE"))
      iupAttribSet(ih, "IMINACTIVE", (char*)value);
    gtkToggleUpdateImage(ih, iupdrvIsActive(ih), gtkToggleGetCheck(ih));
    return 1;
  }
  else
    return 0;
}

static int gtkToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (value != iupAttribGet(ih, "IMPRESS"))
      iupAttribSet(ih, "IMPRESS", (char*)value);
    gtkToggleUpdateImage(ih, iupdrvIsActive(ih), gtkToggleGetCheck(ih));
    return 1;
  }
  else
    return 0;
}

static int gtkToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* update the inactive image if necessary */
  if (ih->data->type == IUP_TOGGLE_IMAGE)
    gtkToggleUpdateImage(ih, iupStrBoolean(value), gtkToggleGetCheck(ih));

  return iupBaseSetActiveAttrib(ih, value);
}

/****************************************************************************************************/

#if GTK_CHECK_VERSION(3, 0, 0)
static void gtkSwitchToggled(GtkSwitch *widget, GParamSpec *pspec, Ihandle* ih)
{
  IFni cb;
  int check;
  (void)widget;
  (void)pspec;

  if (iupAttribGet(ih, "_IUPGTK_IGNORE_TOGGLE"))
    return;

  check = gtk_switch_get_active(GTK_SWITCH(ih->handle))? 1: 0;

  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb && cb(ih, check) == IUP_CLOSE)
    IupExitLoop();

  if (iupObjectCheck(ih))
    iupBaseCallValueChangedCb(ih);
}
#endif

static void gtkToggleToggled(GtkToggleButton *widget, Ihandle* ih)
{
  IFni cb;
  int check;

  if (iupAttribGet(ih, "_IUPGTK_IGNORE_TOGGLE"))
    return;

  check = gtkToggleGetCheck(ih);

  if (ih->data->type == IUP_TOGGLE_IMAGE)
    gtkToggleUpdateImage(ih, iupdrvIsActive(ih), check);

  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb && cb(ih, check) == IUP_CLOSE)
    IupExitLoop();

  if (iupObjectCheck(ih))
    iupBaseCallValueChangedCb(ih);

  (void)widget;
}

static int gtkToggleUpdate3StateCheck(Ihandle *ih, int keyb)
{
  int check = gtkToggleGetCheck(ih);
  if (check == 1)  /* GOTO check == -1 */
  {
    gtk_toggle_button_set_inconsistent((GtkToggleButton*)ih->handle, TRUE);
    gtkToggleToggled((GtkToggleButton*)ih->handle, ih);
    return TRUE; /* ignore message to avoid change toggle state */
  }
  else if (check == -1)  /* GOTO check == 0 */
  {
    gtk_toggle_button_set_inconsistent((GtkToggleButton*)ih->handle, FALSE);
    if (keyb)
    {
      gtk_toggle_button_set_active((GtkToggleButton*)ih->handle, FALSE);
      return TRUE; /* ignore message to avoid change toggle state */
    }
  }
  else  /* (check == 0)  GOTO check == 1 */
  {
    gtk_toggle_button_set_inconsistent((GtkToggleButton*)ih->handle, FALSE);
    if (keyb)
    {
      gtk_toggle_button_set_active((GtkToggleButton*)ih->handle, TRUE);
      return TRUE; /* ignore message to avoid change toggle state */
    }
  }

  return FALSE;
}

static gboolean gtkToggleButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih)
{
  if (iupAttribGet(ih, "_IUPGTK_IGNORE_TOGGLE"))
    return FALSE;

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    char* name = iupAttribGet(ih, "IMPRESS");
    if (name)
    {
      if (evt->type == GDK_BUTTON_PRESS)
        gtkToggleUpdateImage(ih, iupdrvIsActive(ih), 1);
      else
        gtkToggleUpdateImage(ih, iupdrvIsActive(ih), 0);
    }
  }
  else
  {
    if (evt->type == GDK_BUTTON_RELEASE)
    {
      if (gtkToggleUpdate3StateCheck(ih, 0))
        return TRUE; /* ignore message to avoid change toggle state */
    }
  }

  (void)widget;
  return FALSE;
}

static gboolean gtkToggleKeyEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  if (evt->type == GDK_KEY_PRESS)
  {
    if (evt->keyval == GDK_space || evt->keyval == GDK_Return)
      return TRUE; /* ignore message to avoid change toggle state */
  }
  else
  {
    if (evt->keyval == GDK_space || evt->keyval == GDK_Return)
    {
      if (gtkToggleUpdate3StateCheck(ih, 1))
        return TRUE; /* ignore message to avoid change toggle state */
    }
  }

  (void)widget;
  return FALSE;
}

static gboolean gtkToggleEnterLeaveEvent(GtkWidget *widget, GdkEventCrossing *evt, Ihandle *ih)
{
  /* Used only when FLAT=Yes */

  iupgtkEnterLeaveEvent(widget, evt, ih);
  (void)widget;

  if (gtkToggleGetCheck(ih) == 1)
    gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NORMAL);
  else
  {
    if (evt->type == GDK_ENTER_NOTIFY)
      gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NORMAL);
    else  if (evt->type == GDK_LEAVE_NOTIFY)
      gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NONE);
  }

  return FALSE;
}

static int gtkToggleMapMethod(Ihandle* ih)
{
  Ihandle* radio = iupRadioFindToggleParent(ih);
  char *value;
  int is3state = 0;

  if (!ih->parent)
    return IUP_ERROR;

  value = iupAttribGet(ih, "IMAGE");
  if (value)
    ih->data->type = IUP_TOGGLE_IMAGE;
  else
    ih->data->type = IUP_TOGGLE_TEXT;

  if (radio)
  {
    GtkRadioButton* last_tg = (GtkRadioButton*)iupAttribGet(radio, "_IUPGTK_LASTRADIOBUTTON");

    /* Disable SWITCH for radio toggles */
    if (iupAttribGetBoolean(ih, "SWITCH"))
      iupAttribSet(ih, "SWITCH", "NO");

    if (last_tg)
      ih->handle = gtk_radio_button_new_from_widget(last_tg);
    else
      ih->handle = gtk_radio_button_new(NULL);
    iupAttribSet(radio, "_IUPGTK_LASTRADIOBUTTON", (char*)ih->handle);

    /* make sure it has at least one name */
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
#if GTK_CHECK_VERSION(3, 0, 0)
        ih->handle = gtk_switch_new();
#else
        /* GTK2: Create custom switch using GtkDrawingArea */
        IupGtkSwitchData* switch_data;

        switch_data = (IupGtkSwitchData*)calloc(1, sizeof(IupGtkSwitchData));
        switch_data->ih = ih;
        iupAttribSet(ih, "_IUPGTK_SWITCHDATA", (char*)switch_data);

        switch_data->drawing_area = gtk_drawing_area_new();
        gtk_widget_set_size_request(switch_data->drawing_area, SWITCH_TRACK_WIDTH, SWITCH_TRACK_HEIGHT);

        ih->handle = switch_data->drawing_area;

        if (!ih->handle)
        {
          free(switch_data);
          return IUP_ERROR;
        }

        /* Set up events */
        gtk_widget_add_events(ih->handle, GDK_BUTTON_PRESS_MASK | GDK_EXPOSURE_MASK);

        /* Add to parent */
        iupgtkAddToParent(ih);

        if (!iupAttribGetBoolean(ih, "CANFOCUS"))
          iupgtkSetCanFocus(ih->handle, 0);

        /* Connect signals */
        g_signal_connect(G_OBJECT(ih->handle), "expose-event", G_CALLBACK(gtkSwitchExposeEvent), switch_data);
        g_signal_connect(G_OBJECT(ih->handle), "button-press-event", G_CALLBACK(gtkSwitchButtonPressEvent), ih);
        g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
        g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
        g_signal_connect(G_OBJECT(ih->handle), "focus-in-event", G_CALLBACK(iupgtkFocusInOutEvent), ih);
        g_signal_connect(G_OBJECT(ih->handle), "focus-out-event", G_CALLBACK(iupgtkFocusInOutEvent), ih);
        g_signal_connect(G_OBJECT(ih->handle), "key-press-event", G_CALLBACK(iupgtkKeyPressEvent), ih);
        g_signal_connect(G_OBJECT(ih->handle), "show-help", G_CALLBACK(iupgtkShowHelp), ih);
        g_signal_connect(G_OBJECT(ih->handle), "destroy", G_CALLBACK(gtkSwitchDestroyCallback), ih);

        /* Set initial value */
        value = iupAttribGet(ih, "VALUE");
        if (value && iupStrBoolean(value))
          switch_data->checked_state = 1;
        else
          switch_data->checked_state = 0;

        gtk_widget_realize(ih->handle);

        return IUP_NOERROR;
#endif
      }
      else
      {
        ih->handle = gtk_check_button_new();
        if (iupAttribGetBoolean(ih, "3STATE"))
          is3state = 1;
      }
    }
    else
      ih->handle = gtk_toggle_button_new();
  }

  if (!ih->handle)
    return IUP_ERROR;

  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    if (iupAttribGetBoolean(ih, "SWITCH"))
    {
      /* GtkSwitch does not have an internal label */
    }
    else
#endif
    {
      gtk_button_set_image((GtkButton*)ih->handle, gtk_label_new(NULL));
      gtk_toggle_button_set_mode((GtkToggleButton*)ih->handle, TRUE);
    }
  }
  else
  {
    gtk_button_set_image((GtkButton*)ih->handle, gtk_image_new());
    gtk_toggle_button_set_mode((GtkToggleButton*)ih->handle, FALSE);
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  if (!iupAttribGetBoolean(ih, "SWITCH"))
#endif
    iupgtkClearSizeStyleCSS(ih->handle);

  /* add to the parent, all GTK controls must call this. */
  iupgtkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupgtkSetCanFocus(ih->handle, 0);

  if (ih->data->type == IUP_TOGGLE_IMAGE && ih->data->flat)
  {
    gtk_button_set_relief((GtkButton*)ih->handle, GTK_RELIEF_NONE);

    g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(gtkToggleEnterLeaveEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(gtkToggleEnterLeaveEvent), ih);
  }
  else
  {
    g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  }

  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(iupgtkKeyPressEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);

#if GTK_CHECK_VERSION(3, 0, 0)
  if (iupAttribGetBoolean(ih, "SWITCH") && ih->data->type == IUP_TOGGLE_TEXT)
  {
    g_signal_connect(G_OBJECT(ih->handle), "notify::active", G_CALLBACK(gtkSwitchToggled), ih);
  }
  else
#endif
  {
    g_signal_connect(G_OBJECT(ih->handle), "toggled",            G_CALLBACK(gtkToggleToggled), ih);
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE || is3state)
  {
    g_signal_connect(G_OBJECT(ih->handle), "button-press-event",  G_CALLBACK(gtkToggleButtonEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "button-release-event",G_CALLBACK(gtkToggleButtonEvent), ih);
  }

  if (is3state)
  {
    g_signal_connect(G_OBJECT(ih->handle), "key-press-event",  G_CALLBACK(gtkToggleKeyEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "key-release-event",  G_CALLBACK(gtkToggleKeyEvent), ih);
  }

  gtk_widget_realize(ih->handle);

  /* update a mnemonic in a label if necessary */
  iupgtkUpdateMnemonic(ih);

  return IUP_NOERROR;
}

void iupdrvToggleInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = gtkToggleMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Common */
  iupClassRegisterAttribute(ic, "FONT", NULL, gtkToggleSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, gtkToggleSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkToggleSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);  /* black */
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtkToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupToggle only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, gtkToggleSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT); /* force new default value */
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtkToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, gtkToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, gtkToggleSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", gtkToggleGetValueAttrib, gtkToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, gtkToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

  /* NOT supported */
  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
