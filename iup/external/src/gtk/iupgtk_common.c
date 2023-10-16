/** \file
 * \brief GTK Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>             
#include <limits.h>             

#include <gtk/gtk.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_assert.h"

#include "iupgtk_drv.h"

#if GTK_CHECK_VERSION(3, 0, 0)
typedef struct _iupGtkFixed
{
  GtkFixed fixed;
} iupGtkFixed;

typedef struct _iupGtkFixedClass
{
  GtkFixedClass parent_class;
} iupGtkFixedClass;

static GType iup_gtk_fixed_get_type (void) G_GNUC_CONST;
static void iup_gtk_fixed_class_init (iupGtkFixedClass *_class);
static void iup_gtk_fixed_init (iupGtkFixed *fixed);
static void iup_gtk_fixed_get_preferred_size (GtkWidget *widget, gint *minimum, gint *natural);

G_DEFINE_TYPE (iupGtkFixed, iup_gtk_fixed, GTK_TYPE_FIXED)

static void iup_gtk_fixed_class_init (iupGtkFixedClass *_class)
{
  GtkWidgetClass *widget_class = (GtkWidgetClass*) _class;
  widget_class->get_preferred_width = iup_gtk_fixed_get_preferred_size;
  widget_class->get_preferred_height = iup_gtk_fixed_get_preferred_size;
}

static int iupGtkFixedWindow = 0;

static void iup_gtk_fixed_init (iupGtkFixed *fixed)
{
  if (iupGtkFixedWindow)
    gtk_widget_set_has_window(GTK_WIDGET(fixed), TRUE);
}

static void iup_gtk_fixed_get_preferred_size (GtkWidget *widget, gint *minimum, gint *natural)
{
  (void)widget;
  /* all this is just to replace this method, so it will behave like GtkLayout. */
  *minimum = 0;
  *natural = 0;
}

static GtkWidget* iup_gtk_fixed_new(void)
{
  return g_object_new (iup_gtk_fixed_get_type(), NULL);
}
#endif

/* WARNING: in GTK there are many controls that are not native windows,
so theirs GdkWindow will NOT return a native window exclusive of that control,
in fact it can return a base native window shared by many controls.
IupCanvas is a special case that uses an exclusive native window. */
GtkWidget* iupgtkNativeContainerNew(int has_window)
{
  GtkWidget* widget;

#if GTK_CHECK_VERSION(3, 0, 0)
  iupGtkFixedWindow = has_window;
  widget = iup_gtk_fixed_new();
  iupGtkFixedWindow = 0;
#else
  widget = gtk_fixed_new();
#endif

#if GTK_CHECK_VERSION(2, 18, 0)
  gtk_widget_set_has_window(widget, has_window);
#else
  gtk_fixed_set_has_window(GTK_FIXED(widget), has_window);
#endif

  return widget;
}

void iupgtkNativeContainerAdd(GtkWidget* container, GtkWidget* widget)
{
  gtk_fixed_put(GTK_FIXED(container), widget, 0, 0);
}

void iupgtkNativeContainerMove(GtkWidget* container, GtkWidget* widget, int x, int y)
{
  gtk_fixed_move(GTK_FIXED(container), widget, x, y);
}

/* GTK only has absolute positioning using a native container,
so all elements returned by iupChildTreeGetNativeParentHandle should be a native container.
If not looks in the native parent. */
static GtkWidget* gtkGetNativeParent(Ihandle* ih)
{
  GtkWidget* widget = iupChildTreeGetNativeParentHandle(ih);
  while (widget && !GTK_IS_FIXED(widget))
    widget = gtk_widget_get_parent(widget);
  return widget;
}

const char* iupgtkGetWidgetClassName(GtkWidget* widget)
{
  /* Used for debugging */
  return g_type_name(G_TYPE_FROM_CLASS(GTK_WIDGET_GET_CLASS(widget)));
}

void iupgtkUpdateMnemonic(Ihandle* ih)
{
  GtkLabel* label = (GtkLabel*)iupAttribGet(ih, "_IUPGTK_LABELMNEMONIC");
  if (label) gtk_label_set_mnemonic_widget(label, ih->handle);
}

IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
  gtk_widget_activate(ih->handle);
}

IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  GtkWidget* old_parent;
  GtkWidget* new_parent = gtkGetNativeParent(ih);
  GtkWidget* widget = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");  /* here is used as the native child because is the outermost component of the element */
  if (!widget) widget = ih->handle;
  old_parent = gtk_widget_get_parent(widget);
  if (old_parent != new_parent)
  {
    gtk_widget_reparent(widget, new_parent);
    gtk_widget_realize(widget);
  }
}

IUP_DRV_API void iupgtkAddToParent(Ihandle* ih)
{
  GtkWidget* parent = gtkGetNativeParent(ih);
  GtkWidget* widget = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT"); /* here is used as the native child because is the outermost component of the element */
  if (!widget) widget = ih->handle;

  iupgtkNativeContainerAdd(parent, widget);
}

void iupgtkSetPosSize(GtkContainer* parent, GtkWidget* widget, int x, int y, int width, int height)
{
  iupgtkNativeContainerMove((GtkWidget*)parent, widget, x, y);

  if (width > 0 && height > 0)
    gtk_widget_set_size_request(widget, width, height);
}

IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{
  GtkWidget* parent = gtkGetNativeParent(ih);
  GtkWidget* widget = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget) widget = ih->handle;

  iupgtkSetPosSize(GTK_CONTAINER(parent), widget, ih->x, ih->y, ih->currentwidth, ih->currentheight);
}

IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  GtkWidget* widget = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget) widget = ih->handle;
  gtk_widget_hide(widget);
  gtk_widget_unrealize(widget);
  gtk_widget_destroy(widget);   /* To match the call to gtk_*****_new     */
}

IUP_SDK_API void iupdrvPostRedraw(Ihandle *ih)
{
  GdkWindow* window = iupgtkGetWindow(ih->handle);
  if (window)
    gdk_window_invalidate_rect(window, NULL, TRUE);

  /* Post a REDRAW */
  gtk_widget_queue_draw(ih->handle);
}

IUP_SDK_API void iupdrvRedrawNow(Ihandle *ih)
{
  GdkWindow* window = iupgtkGetWindow(ih->handle);
  if (window)
    gdk_window_invalidate_rect(window, NULL, TRUE);

  /* Post a REDRAW */
  gtk_widget_queue_draw(ih->handle);

  /* Force a REDRAW */
  if (window)
    gdk_window_process_updates(window, TRUE);
}

static GtkWidget* gtkGetWindowedParent(GtkWidget* widget)
{
#if GTK_CHECK_VERSION(2, 18, 0)
  while (widget && !gtk_widget_get_has_window(widget))
#else
	while (widget && GTK_WIDGET_NO_WINDOW(widget))
#endif
    widget = gtk_widget_get_parent(widget);
  return widget;
}

IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
  gint win_x = 0, win_y = 0;
  gint dx = 0, dy = 0;
  GtkWidget* wparent = gtkGetWindowedParent(ih->handle);
  if (ih->handle != wparent)
    gtk_widget_translate_coordinates(ih->handle, wparent, 0, 0, &dx, &dy); /* widget origin relative to GDK window */
  gdk_window_get_origin(iupgtkGetWindow(wparent), &win_x, &win_y);  /* GDK window origin relative to screen */
  *x -= win_x + dx;
  *y -= win_y + dy;
}

IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
  gint win_x = 0, win_y = 0;
  gint dx = 0, dy = 0;
  GtkWidget* wparent = gtkGetWindowedParent(ih->handle);
  if (ih->handle != wparent)
    gtk_widget_translate_coordinates(ih->handle, wparent, 0, 0, &dx, &dy); /* widget relative to GDK window */
  gdk_window_get_origin(iupgtkGetWindow(wparent), &win_x, &win_y);  /* GDK window relative to screen */
  *x += win_x + dx;
  *y += win_y + dy;
}

IUP_DRV_API gboolean iupgtkShowHelp(GtkWidget *widget, GtkWidgetHelpType *arg1, Ihandle *ih)
{
  Icallback cb;
  (void)widget;
  (void)arg1;

  cb = IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE) 
    IupExitLoop();

  return FALSE;
}

IUP_DRV_API gboolean iupgtkEnterLeaveEvent(GtkWidget *widget, GdkEventCrossing *evt, Ihandle *ih)
{
  Icallback cb = NULL;
  (void)widget;

  if (evt->type == GDK_ENTER_NOTIFY)
    cb = IupGetCallback(ih, "ENTERWINDOW_CB");
  else  if (evt->type == GDK_LEAVE_NOTIFY)
    cb = IupGetCallback(ih, "LEAVEWINDOW_CB");

  if (cb) 
    cb(ih);

  return FALSE;
}

int iupgtkSetMnemonicTitle(Ihandle* ih, GtkLabel* label, const char* value)
{
  char c = '_';
  char* str;

  if (!value) 
    value = "";

  str = iupStrProcessMnemonic(value, &c, 1);  /* replace & by c, the returned value of c is ignored in GTK */
  if (str != value)
  {
    gtk_label_set_text_with_mnemonic(label, iupgtkStrConvertToSystem(str));
    free(str);
    return 1;
  }
  else
  {
    if (iupAttribGetBoolean(ih, "MARKUP"))
      gtk_label_set_markup(label, iupgtkStrConvertToSystem(str));
    else
      gtk_label_set_text(label, iupgtkStrConvertToSystem(str));
  }
  return 0;
}

IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  if (iupdrvIsVisible(ih))
  {
    GdkWindow* window = iupgtkGetWindow(ih->handle);
    if (iupStrEqualNoCase(value, "TOP"))
      gdk_window_raise(window);
    else
      gdk_window_lower(window);
  }

  return 0;
}

IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int visible)
{
  GtkWidget* container = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (visible)
  {
    if (container) gtk_widget_show(container);
    gtk_widget_show(ih->handle);
  }
  else
  {
    if (container) gtk_widget_hide(container);
    gtk_widget_hide(ih->handle);
  }
}

int iupgtkIsVisible(GtkWidget* widget)
{
#if GTK_CHECK_VERSION(2, 18, 0)
  return gtk_widget_get_visible(widget);
#else
  return GTK_WIDGET_VISIBLE(widget);
#endif
}

IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  if (iupgtkIsVisible(ih->handle))
  {
    /* if marked as visible, since we use gtk_widget_hide and NOT gtk_widget_hide_all
       must check its parents. */
    Ihandle* parent = ih->parent;
    while (parent)
    {
      if (parent->iclass->nativetype != IUP_TYPEVOID)
      {
        if (!iupgtkIsVisible(parent->handle))
          return 0;
      }

      parent = parent->parent;
    }
    return 1;
  }
  else
    return 0;
}

IUP_SDK_API int iupdrvIsActive(Ihandle *ih)
{
#if GTK_CHECK_VERSION(2, 18, 0)
  return gtk_widget_is_sensitive(ih->handle);
#else
  return GTK_WIDGET_IS_SENSITIVE(ih->handle);
#endif
}

IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  GtkWidget* container = (GtkWidget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (container) gtk_widget_set_sensitive(container, enable);
  gtk_widget_set_sensitive(ih->handle, enable);
}

#if GTK_CHECK_VERSION(3, 0, 0)
static void iupgdkRGBASet(GdkRGBA* rgba, unsigned char r, unsigned char g, unsigned char b)
{
  rgba->red = iupgtkColorToDouble(r);
  rgba->green = iupgtkColorToDouble(g);
  rgba->blue = iupgtkColorToDouble(b);
  rgba->alpha = 1.0;
}

static GdkRGBA gtkDarkerRGBA(GdkRGBA *rgba)
{
  GdkRGBA dark_rgba = {0,0,0,1.0};

  dark_rgba.red = (rgba->red*9)/10;
  dark_rgba.green = (rgba->green*9)/10;
  dark_rgba.blue = (rgba->blue*9)/10;

  return dark_rgba;
}

static gdouble gtkCROPDouble(gdouble x)
{
  if (x > 1.0) return 1.0;
  return x;
}

static GdkRGBA gtkLighterRGBA(GdkRGBA *rgba)
{
  GdkRGBA light_rgba = {0,0,0,1.0};

  light_rgba.red = gtkCROPDouble((rgba->red*11)/10);
  light_rgba.green = gtkCROPDouble((rgba->green*11)/10);
  light_rgba.blue = gtkCROPDouble((rgba->blue*11)/10);

  return light_rgba;
}
#else
static GdkColor gtkDarkerColor(GdkColor *color)
{
  GdkColor dark_color = {0L,0,0,0};

  dark_color.red = (color->red*9)/10;
  dark_color.green = (color->green*9)/10;
  dark_color.blue = (color->blue*9)/10;

  return dark_color;
}

static guint16 gtkCROP16(int x)
{
  if (x > 65535) return 65535;
  return (guint16)x;
}

static GdkColor gtkLighterColor(GdkColor *color)
{
  GdkColor light_color = {0L,0,0,0};

  light_color.red = gtkCROP16(((int)color->red*11)/10);
  light_color.green = gtkCROP16(((int)color->green*11)/10);
  light_color.blue = gtkCROP16(((int)color->blue*11)/10);

  return light_color;
}
#endif

#if GTK_CHECK_VERSION(3, 16, 0)
static char* gtkGetSelectedColorStr(void)
{
  GdkRGBA rgba;
  unsigned char r, g, b;
  iupStrToRGB(IupGetGlobal("TXTHLCOLOR"), &r, &g, &b);
  iupgdkRGBASet(&rgba, r, g, b);
  return gdk_rgba_to_string(&rgba);
}
#else
#if GTK_CHECK_VERSION(3, 0, 0)
static GdkRGBA gtkGetSelectedColorRGBA(void)
{
  GdkRGBA rgba;
  unsigned char r, g, b;
  iupStrToRGB(IupGetGlobal("TXTHLCOLOR"), &r, &g, &b);
  iupgdkRGBASet(&rgba, r, g, b);
  return rgba;
}
#endif
#endif

void iupgtkSetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkRGBA rgba, light_rgba, dark_rgba;
  int is_txt = 0;

  if (GTK_IS_TREE_VIEW(handle) ||
      GTK_IS_TEXT_VIEW(handle) ||
      GTK_IS_ENTRY(handle) ||
      GTK_IS_COMBO_BOX(handle))
      is_txt = 1;

  iupgdkRGBASet(&rgba, r, g, b);
  dark_rgba = gtkDarkerRGBA(&rgba);
  light_rgba = gtkLighterRGBA(&rgba);

#if GTK_CHECK_VERSION(3, 16, 0)
  {
    char *bg, *bg_light, *bg_dark;
    char *css;
    GtkCssProvider *provider;

    bg = gdk_rgba_to_string(&rgba);
    bg_light = gdk_rgba_to_string(&light_rgba);
    bg_dark = gdk_rgba_to_string(&dark_rgba);
    
    /* style background color using CSS */
    provider = gtk_css_provider_new();
    if (is_txt)
    {
      char* selected = gtkGetSelectedColorStr();

      css = g_strdup_printf("*          { background-color: %s; }"
                            "*:hover    { background-color: %s; }"
                            "*:active   { background-color: %s; }"
                            "*:selected { background-color: %s; }"
                            "*:disabled { background-color: %s; }",
                            bg, bg_light, bg_dark, selected, bg_light);

      g_free(selected); 
    }
    else
    {
      css = g_strdup_printf("*          { background-color: %s; }"
                            "*:hover    { background-color: %s; }"
                            "*:active   { background-color: %s; }"
                            "*:disabled { background-color: %s; }",
                            bg, bg_light, bg_dark, bg);
    }
    gtk_style_context_add_provider(gtk_widget_get_style_context(handle), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    
    g_free(bg); g_free(bg_light); g_free(bg_dark);
    g_free(css);
    g_object_unref(provider);
  }
#else /* GTK < 3.16 */
  gtk_widget_override_background_color(handle, GTK_STATE_FLAG_NORMAL, &rgba);
  gtk_widget_override_background_color(handle, GTK_STATE_FLAG_ACTIVE, &dark_rgba);  /* active */
  gtk_widget_override_background_color(handle, GTK_STATE_FLAG_PRELIGHT, &light_rgba);  /* hover */

  if (is_txt)
  {
    GdkRGBA selected_rgba = gtkGetSelectedColorRGBA();
    gtk_widget_override_background_color(handle, GTK_STATE_FLAG_INSENSITIVE, &light_rgba);  /* disabled */
    gtk_widget_override_background_color(handle, GTK_STATE_FLAG_SELECTED, &selected_rgba);
  }
  else
    gtk_widget_override_background_color(handle, GTK_STATE_FLAG_INSENSITIVE, &rgba);  /* disabled */
#endif
#else  /* GTK 2.x */
  GtkRcStyle *rc_style;  
  GdkColor color;

  iupgdkColorSetRGB(&color, r, g, b);

  rc_style = gtk_widget_get_modifier_style(handle);

  rc_style->base[GTK_STATE_NORMAL]   = rc_style->bg[GTK_STATE_NORMAL] = rc_style->bg[GTK_STATE_INSENSITIVE] = color;
  rc_style->base[GTK_STATE_ACTIVE]   = rc_style->bg[GTK_STATE_ACTIVE] = gtkDarkerColor(&color);
  rc_style->base[GTK_STATE_PRELIGHT] = rc_style->bg[GTK_STATE_PRELIGHT] = rc_style->base[GTK_STATE_INSENSITIVE] = gtkLighterColor(&color);

  rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_BASE | GTK_RC_BG;
  rc_style->color_flags[GTK_STATE_ACTIVE] |= GTK_RC_BASE | GTK_RC_BG;  /* active */
  rc_style->color_flags[GTK_STATE_PRELIGHT] |= GTK_RC_BASE | GTK_RC_BG;  /* hover */
  rc_style->color_flags[GTK_STATE_INSENSITIVE] |= GTK_RC_BASE | GTK_RC_BG;   /* disabled */

  gtk_widget_modify_style(handle, rc_style);
#endif
}

void iupgtkSetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkRGBA rgba;

  iupgdkRGBASet(&rgba, r, g, b);

  gtk_widget_override_color(handle, GTK_STATE_FLAG_NORMAL, &rgba);
  gtk_widget_override_color(handle, GTK_STATE_FLAG_ACTIVE, &rgba);
  gtk_widget_override_color(handle, GTK_STATE_FLAG_PRELIGHT, &rgba);
#else
  GtkRcStyle *rc_style;  
  GdkColor color;

  iupgdkColorSetRGB(&color, r, g, b);

  rc_style = gtk_widget_get_modifier_style(handle);  

  rc_style->fg[GTK_STATE_ACTIVE] = rc_style->fg[GTK_STATE_NORMAL] = rc_style->fg[GTK_STATE_PRELIGHT] = color;
  rc_style->text[GTK_STATE_ACTIVE] = rc_style->text[GTK_STATE_NORMAL] = rc_style->text[GTK_STATE_PRELIGHT] = color;

  rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_TEXT | GTK_RC_FG;
  rc_style->color_flags[GTK_STATE_ACTIVE] |= GTK_RC_TEXT | GTK_RC_FG;
  rc_style->color_flags[GTK_STATE_PRELIGHT] |= GTK_RC_TEXT | GTK_RC_FG;

  /* do not set at CHILD_CONTAINER */
  gtk_widget_modify_style(handle, rc_style);
#endif
}

IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkSetBgColor(ih->handle, r, g, b);

  /* DO NOT NEED TO UPDATE GTK IMAGES SINCE THEY DO NOT DEPEND ON BGCOLOR */

  return 1;
}

IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupgtkSetFgColor(ih->handle, r, g, b);

  return 1;
}

static GdkCursor* gtkEmptyCursor(Ihandle* ih)
{
#if GTK_CHECK_VERSION(2, 16, 0)
  (void)ih;
  return gdk_cursor_new(GDK_BLANK_CURSOR);
#else  
  /* creates an empty cursor */
  GdkColor cursor_color = {0L,0,0,0};
  char bitsnull[1] = {0x00};

  GdkWindow* window = iupgtkGetWindow(ih->handle);
  GdkPixmap* pixmapnull = gdk_bitmap_create_from_data(
    (GdkDrawable*)window,
    bitsnull,
    1,1);
  GdkCursor* cur = gdk_cursor_new_from_pixmap(
    pixmapnull,
    pixmapnull,
    &cursor_color,
    &cursor_color,
    0,0);

  g_object_unref(pixmapnull);

  return cur;
#endif
}

static GdkCursor* gtkGetCursor(Ihandle* ih, const char* name)
{
  static struct {
    const char* iupname;
    int         sysname;
  } table[] = {
    { "NONE",      0}, 
    { "NULL",      0}, 
    { "ARROW",     GDK_LEFT_PTR},
    { "BUSY",      GDK_WATCH},
    { "CROSS",     GDK_CROSSHAIR},
    { "HAND",      GDK_HAND2},
    { "HELP",      GDK_QUESTION_ARROW},
    { "IUP",       GDK_QUESTION_ARROW},
    { "MOVE",      GDK_FLEUR},
    { "PEN",       GDK_PENCIL},
    { "RESIZE_N",  GDK_TOP_SIDE},
    { "RESIZE_S",  GDK_BOTTOM_SIDE},
    { "RESIZE_NS", GDK_SB_V_DOUBLE_ARROW},
    { "SPLITTER_HORIZ", GDK_SB_V_DOUBLE_ARROW},
    { "RESIZE_W",  GDK_LEFT_SIDE},
    { "RESIZE_E",  GDK_RIGHT_SIDE},
    { "RESIZE_WE", GDK_SB_H_DOUBLE_ARROW},
    { "SPLITTER_VERT", GDK_SB_H_DOUBLE_ARROW},
    { "RESIZE_NE", GDK_TOP_RIGHT_CORNER},
    { "RESIZE_SE", GDK_BOTTOM_RIGHT_CORNER},
    { "RESIZE_NW", GDK_TOP_LEFT_CORNER},
    { "RESIZE_SW", GDK_BOTTOM_LEFT_CORNER},
    { "TEXT",      GDK_XTERM}, 
    { "UPARROW",   GDK_CENTER_PTR} 
  };

  GdkCursor* cur;
  char str[200];
  int i, count = sizeof(table)/sizeof(table[0]);

  /* check the cursor cache first (per control)*/
  sprintf(str, "_IUPGTK_CURSOR_%s", name);
  cur = (GdkCursor*)iupAttribGet(ih, str);
  if (cur)
    return cur;

  /* check the pre-defined IUP names first */
  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, table[i].iupname)) 
    {
      if (table[i].sysname)
        cur = gdk_cursor_new(table[i].sysname);
      else
        cur = gtkEmptyCursor(ih);

      break;
    }
  }

  if (i == count)
  {
    /* check for a name defined cursor */
    cur = iupImageGetCursor(name);
  }

  /* save the cursor in cache */
  iupAttribSet(ih, str, (char*)cur);

  return cur;
}

IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  GdkCursor* cur = gtkGetCursor(ih, value);
  if (cur)
  {
    GdkWindow* window = iupgtkGetWindow(ih->handle);
    if (window)
    {
      gdk_window_set_cursor(window, cur);
      gdk_display_flush(gdk_display_get_default());
    }
    return 1;
  }
  return 0;
}

void iupgdkColorSetRGB(GdkColor* color, unsigned char r, unsigned char g, unsigned char b)
{
  color->red = iupCOLOR8TO16(r);
  color->green = iupCOLOR8TO16(g);
  color->blue = iupCOLOR8TO16(b);
  color->pixel = 0;
}

IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  static int size = 0;

  if (iupStrBoolean(IupGetGlobal("OVERLAYSCROLLBAR")))
    return 1;

  if (size == 0)
  {
    gint slider_width, trough_border;

#if GTK_CHECK_VERSION(3, 0, 0)
    GtkWidget* sb = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);
#else
    GtkWidget* sb = gtk_vscrollbar_new(NULL);
#endif

    gtk_widget_style_get(sb, "slider-width", &slider_width,
                             "trough-border", &trough_border,
                             NULL);

    size = trough_border * 2 + slider_width;

    gtk_widget_destroy(sb);
  }

  return size;
}

IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, iupgtkGetNativeFontIdName(), iupgtkGetFontIdAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "PANGOFONTDESC", iupgtkGetPangoFontDescAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "PANGOLAYOUT", iupgtkGetPangoLayoutAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_STRING);
}

IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPMARKUP", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIPICON", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_DEFAULT);
}

gboolean iupgtkMotionNotifyEvent(GtkWidget *widget, GdkEventMotion *evt, Ihandle *ih)
{
  IFniis cb;

  if (evt->is_hint)
  {
    int x, y;
    iupgtkWindowGetPointer(iupgtkGetWindow(widget), &x, &y, NULL);
    evt->x = x;
    evt->y = y;
  }

  cb = (IFniis)IupGetCallback(ih,"MOTION_CB");
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupgtkButtonKeySetStatus(evt->state, 0, status, 0);
    cb(ih, (int)evt->x, (int)evt->y, status);
  }

  return FALSE;
}

IUP_DRV_API gboolean iupgtkButtonEvent(GtkWidget *widget, GdkEventButton *evt, Ihandle *ih)
{
  IFniiiis cb = (IFniiiis)IupGetCallback(ih,"BUTTON_CB");
  if (cb)
  {
    int doubleclick = 0, ret, press = 1;
    int b = IUP_BUTTON1+(evt->button-1);
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

    if (evt->type == GDK_BUTTON_RELEASE)
      press = 0;

    if (evt->type == GDK_2BUTTON_PRESS)
      doubleclick = 1;

    iupgtkButtonKeySetStatus(evt->state, evt->button, status, doubleclick);

    if (doubleclick)
    {
      /* Must compensate the fact that in GTK there is an extra button press event 
         when occurs a double click, we compensate that completing the event 
         with a button release before the double click. */

      status[5] = ' '; /* clear double click */

      ret = cb(ih, b, 0, (int)evt->x, (int)evt->y, status);  /* release */
      if (ret==IUP_CLOSE)
        IupExitLoop();
      else if (ret==IUP_IGNORE)
        return TRUE;

      status[5] = 'D'; /* restore double click */
    }

    ret = cb(ih, b, press, (int)evt->x, (int)evt->y, status);
    if (ret==IUP_CLOSE)
      IupExitLoop();
    else if (ret==IUP_IGNORE)
      return TRUE;
  }

  (void)widget;
  return FALSE;
}

static void igtkSendKey(GdkWindow* window, GdkEventType type, guint keyval, guint state, gint keycode, gint group)
{
  GdkEvent* evt = gdk_event_new(type);

  evt->key.window = g_object_ref(window);
  evt->key.keyval = keyval;
  evt->key.state = state;
  evt->key.hardware_keycode = (guint16)keycode;
  evt->key.group = (guint8)group;

  gtk_main_do_event(evt);
  gdk_event_free(evt);
}

IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  Ihandle* focus;
  guint keyval, state;
  gint nkeys = 0; 
  GdkKeymapKey *keys; 
  GdkWindow* window;

  focus = IupGetFocus();
  if (!focus)
    return;

  iupdrvKeyEncode(key, &keyval, &state);
  if (!keyval)
    return;

  if (!gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), keyval, &keys, &nkeys))
    return;

  window = iupgtkGetWindow(focus->handle);

  if (press & 0x01)
    igtkSendKey(window, GDK_KEY_PRESS, keyval, state, keys[0].keycode, keys[0].group);

  if (press & 0x02)
    igtkSendKey(window, GDK_KEY_RELEASE, keyval, state, keys[0].keycode, keys[0].group);

  g_free(keys);
}

IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
  /* VirtualBox does not reproduce the mouse move visually, but it is working. */
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkDeviceManager* device_manager = gdk_display_get_device_manager(gdk_display_get_default());
  GdkDevice* device = gdk_device_manager_get_client_pointer(device_manager);
  gdk_device_warp(device, gdk_screen_get_default(), x, y);
#elif GTK_CHECK_VERSION(2, 8, 0)
  gdk_display_warp_pointer(gdk_display_get_default(), gdk_screen_get_default(), x, y);
#endif
}

IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  /* always update cursor */
  /* must be before sending the message because the cursor position will be used */
  /* this will also send an extra motion event */
  iupdrvWarpPointer(x, y);

  if (status != -1)
  {
    GtkWidget* grab_widget;
    gint origin_x, origin_y;

    /* TODO check gdk_event_set_pointer_emulated */

#if GTK_CHECK_VERSION(3, 0, 0)
    GdkDeviceManager* device_manager = gdk_display_get_device_manager(gdk_display_get_default());
    GdkDevice* device = gdk_device_manager_get_client_pointer(device_manager);
#endif

    GdkEventButton evt;
    memset(&evt, 0, sizeof(GdkEventButton));
    evt.send_event = TRUE;

    evt.x_root = x;
    evt.y_root = y;
    evt.type = (status==0)? GDK_BUTTON_RELEASE: ((status==2)? GDK_2BUTTON_PRESS: GDK_BUTTON_PRESS);
#if GTK_CHECK_VERSION(3, 0, 0)
    evt.device = device;
#else
    evt.device = gdk_device_get_core_pointer();
#endif

    grab_widget = gtk_grab_get_current();
    if (grab_widget) 
      evt.window = iupgtkGetWindow(grab_widget);
    else
    {
#if GTK_CHECK_VERSION(3, 0, 0)
      evt.window = gdk_device_get_window_at_position(device, NULL, NULL);
#else
      evt.window = gdk_window_at_pointer(NULL, NULL);
#endif
    }

    switch(bt)
    {
    case IUP_BUTTON1:
      evt.state = GDK_BUTTON1_MASK;
      evt.button = 1;
      break;
    case IUP_BUTTON2:
      evt.state = GDK_BUTTON2_MASK;
      evt.button = 2;
      break;
    case IUP_BUTTON3:
      evt.state = GDK_BUTTON3_MASK;
      evt.button = 3;
      break;
    case IUP_BUTTON4:
      evt.state = GDK_BUTTON4_MASK;
      evt.button = 4;
      break;
    case IUP_BUTTON5:
      evt.state = GDK_BUTTON5_MASK;
      evt.button = 5;
      break;
    default:
      return;
    }

    gdk_window_get_origin(evt.window, &origin_x, &origin_y);
    evt.x = x - origin_x;
    evt.y = y - origin_y;

    gdk_event_put((GdkEvent*)&evt);
  }
#if 0 /* kept for future reference */
  else
  {
    GtkWidget* grab_widget;
    gint origin_x, origin_y;

    GdkEventMotion evt;
    memset(&evt, 0, sizeof(GdkEventMotion));
    evt.send_event = TRUE;

    evt.x_root = x;
    evt.y_root = y;
    evt.type = GDK_MOTION_NOTIFY;
    evt.device = gdk_device_get_core_pointer();

    grab_widget = gtk_grab_get_current();
    if (grab_widget) 
      evt.window = iupgtkGetWindow(grab_widget);
    else
      evt.window = gdk_window_at_pointer(NULL, NULL);

    switch(bt)
    {
    case IUP_BUTTON1:
      evt.state = GDK_BUTTON1_MASK;
      break;
    case IUP_BUTTON2:
      evt.state = GDK_BUTTON2_MASK;
      break;
    case IUP_BUTTON3:
      evt.state = GDK_BUTTON3_MASK;
      break;
    case IUP_BUTTON4:
      evt.state = GDK_BUTTON4_MASK;
      break;
    case IUP_BUTTON5:
      evt.state = GDK_BUTTON5_MASK;
      break;
    default:
      return;
    }

    gdk_window_get_origin(evt.window, &origin_x, &origin_y);
    evt.x = x - origin_x;
    evt.y = y - origin_y;

    gdk_event_put((GdkEvent*)&evt);
  }
#endif
}

IUP_SDK_API void iupdrvSleep(int time)
{
  g_usleep(time*1000);  /* mili to micro */
}

GdkWindow* iupgtkGetWindow(GtkWidget *widget)
{
#if GTK_CHECK_VERSION(2, 14, 0)
  return gtk_widget_get_window(widget);
#else
  return widget->window;
#endif
}

void iupgtkWindowGetPointer(GdkWindow *window, int *x, int *y, GdkModifierType *mask)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkDisplay *display = gdk_window_get_display(window);
  GdkDeviceManager* device_manager = gdk_display_get_device_manager(display);
  GdkDevice* device = gdk_device_manager_get_client_pointer(device_manager);
  gdk_window_get_device_position(window, device, x, y, mask);
#else
  gdk_window_get_pointer(window, x, y, mask);
#endif
}

void iupgtkSetMargin(GtkWidget* widget, int horiz_padding, int vert_padding, int mandatory_gtk3)
{
#if GTK_CHECK_VERSION(3, 12, 0)
  if (mandatory_gtk3)
  {
    gtk_widget_set_margin_top(widget, vert_padding);
    gtk_widget_set_margin_bottom(widget, vert_padding);
    gtk_widget_set_margin_start(widget, horiz_padding);
    gtk_widget_set_margin_end(widget, horiz_padding);
  }
#elif GTK_CHECK_VERSION(3, 4, 0)
  gtk_widget_set_margin_top(widget, vert_padding);
  gtk_widget_set_margin_bottom(widget, vert_padding);
  gtk_widget_set_margin_left(widget, horiz_padding);
  gtk_widget_set_margin_right(widget, horiz_padding);
#endif
  (void)widget;
  (void)horiz_padding;
  (void)vert_padding;
  (void)mandatory_gtk3;
}

void iupgtkClearSizeStyleCSS(GtkWidget* widget)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GtkStyleContext *context = gtk_widget_get_style_context(widget);
  const char* str = "*{ padding-bottom: 0px ; padding-top: 0px; padding-left: 2px;  padding-right: 2px; "
                        "margin-bottom: 0px;  margin-top: 0px;  margin-left: 0px;   margin-right: 0px; }";
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider), str, -1, NULL);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(provider);
#else
  (void)widget;
#endif
}

IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{
  /* AtkText Interface? */
  (void)title;
  (void)ih;
}

/* Deprecated but still used for GTK2:
  gdk_bitmap_create_from_data  (since 2.22)
  gdk_font_id
  gdk_font_from_description
*/
