/** \file
 * \brief IupDialog class
 *
 * See Copyright Notice in "iup.h"
 */

#undef GTK_DISABLE_DEPRECATED  /* Since GTK 3.14 gtk_status_icon is deprecated. */
#include <gtk/gtk.h>

#ifdef HILDON
#include <hildon/hildon-program.h>
#endif
                                         
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_class.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_dlglist.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"
#include "iup_assert.h"

#include "iupgtk_drv.h"


static void gtkDialogSetMinMax(Ihandle* ih, int min_w, int min_h, int max_w, int max_h);



/****************************************************************
                     Utilities
****************************************************************/

static gboolean gtkDialogChildDestroyEvent(GtkWidget *widget, Ihandle *ih)
{
  /* It seems that the documentation for this callback is not correct */
  /* The second parameter must be the user_data or it will fail. */
  (void)widget;

  /* If the IUP dialog was not destroyed, destroy it here. */
  if (iupObjectCheck(ih))
    IupDestroy(ih);

  /* this callback is useful to destroy children dialogs when the parent is destroyed. */
  /* The application is responsible for destroying the children before this happen. */

  return FALSE;
}

void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  gtk_window_set_transient_for((GtkWindow*)ih->handle, (GtkWindow*)parent);

  /* manually remove child windows when parent is destroyed */
  g_signal_connect(G_OBJECT(parent), "destroy", G_CALLBACK(gtkDialogChildDestroyEvent), ih);
}

int iupdrvDialogIsVisible(Ihandle* ih)
{
  return iupdrvIsVisible(ih);
}

void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int *w, int *h)
{
  int width, height;
  int border = 0, caption = 0, menu;
  if (!handle)
    handle = ih->handle;

  gtk_window_get_size((GtkWindow*)handle, &width, &height);  /* client size */

  if (ih)
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  if (w) *w = width + 2*border;
  if (h) *h = height + 2*border + caption;  /* menu is inside the dialog_manager */
}

void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  if (visible)
    gtk_widget_show(ih->handle);
  else
    gtk_widget_hide(ih->handle);
}

void iupdrvDialogGetPosition(Ihandle *ih, InativeHandle* handle, int *x, int *y)
{
  if (!handle)
    handle = ih->handle;

  if (iupgtkIsVisible(handle))
  {
    gint gx, gy;
    gtk_window_get_position((GtkWindow*)handle, &gx, &gy);
    if (x) *x = gx;
    if (y) *y = gy;
  }
  else if (ih)
  {
    /* gtk_window_get_position returns an outdated value if window is not visible */
    if (x) *x = iupAttribGetInt(ih, "_IUPGTK_OLD_X");
    if (y) *y = iupAttribGetInt(ih, "_IUPGTK_OLD_Y");
  }
}

void iupdrvDialogSetPosition(Ihandle *ih, int x, int y)
{
  gtk_window_move((GtkWindow*)ih->handle, x, y);
}

static int gtkDialogGetMenuSize(Ihandle* ih)
{
#ifdef HILDON
  return 0;
#else                    
  if (ih->data->menu && !iupStrBoolean(IupGetGlobal("GLOBALMENU")))
    return iupdrvMenuGetMenuBarSize(ih->data->menu);
  else
    return 0;
#endif
}

#define iupABS(_x) ((_x)<0? -(_x): (_x))

static void gtkDialogGetWindowDecor(Ihandle* ih, int *win_border, int *win_caption)
{
  int x, y, frame_x, frame_y;
  gdk_window_get_origin(iupgtkGetWindow(ih->handle), &x, &y);
  gdk_window_get_root_origin(iupgtkGetWindow(ih->handle), &frame_x, &frame_y);
  *win_border = iupABS(x - frame_x);   /* For unknown reason GTK sometimes give negative results */
  *win_caption = iupABS(y - frame_y) - *win_border;
}

void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu)
{
#ifdef HILDON
  /* In Hildon, borders have fixed dimensions, but are drawn as part
     of the client area! */
  if (border)
    *border = (iupAttribGetBoolean(ih, "HILDONWINDOW") && !iupAttribGetBoolean(ih, "FULLSCREEN")) ? 12 : 0;
  if (caption)
    *caption = 0;
  if (menu)
    *menu = 0;
#else
  static int native_border = 0;
  static int native_caption = 0;

  int has_titlebar = iupAttribGetBoolean(ih, "RESIZE")  || /* GTK and Motif only */
                     iupAttribGetBoolean(ih, "MAXBOX")  ||
                     iupAttribGetBoolean(ih, "MINBOX")  ||
                     iupAttribGetBoolean(ih, "MENUBOX") || 
                     iupAttribGet(ih, "TITLE");

  int has_border = has_titlebar ||
                   iupAttribGetBoolean(ih, "RESIZE") ||
                   iupAttribGetBoolean(ih, "BORDER");

  *menu = gtkDialogGetMenuSize(ih);

  if (ih->handle && iupdrvIsVisible(ih))
  {
    int win_border, win_caption;
    gtkDialogGetWindowDecor(ih, &win_border, &win_caption);

#ifdef WIN32
    if (*menu)
      win_caption -= *menu;
#endif

    *border = 0;
    if (has_border)
      *border = win_border;

    *caption = 0;
    if (has_titlebar)
      *caption = win_caption;

    if (!native_border && *border)
      native_border = win_border;

    if (!native_caption && *caption)
      native_caption = win_caption;
  }

  /* I could not set the size of the window including the decorations when the dialog is hidden */
  /* So we have to estimate the size of borders and caption when the dialog is hidden           */

  *border = 0;
  if (has_border)
  {
    if (native_border)
      *border = native_border;
    else
      *border = 5;
  }

  *caption = 0;
  if (has_titlebar)
  {
    if (native_caption)
      *caption = native_caption;
    else
      *caption = 20;
  }

  if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
    *caption = 0;
#endif
}

int iupdrvDialogSetPlacement(Ihandle* ih)
{
  char* placement;
  int old_state = ih->data->show_state;
  ih->data->show_state = IUP_SHOW;

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
  {
    gtk_window_fullscreen((GtkWindow*)ih->handle);
    return 1;
  }
  
  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;

    gtk_window_unmaximize((GtkWindow*)ih->handle);
    gtk_window_deiconify((GtkWindow*)ih->handle);

    if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE") && iupDialogCustomFrameRestore(ih))
    {
      ih->data->show_state = IUP_RESTORE;
      return 1;
    }

    return 0;
  }

  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE") && iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    iupDialogCustomFrameMaximize(ih);
    iupAttribSet(ih, "PLACEMENT", NULL); /* reset to NORMAL */
    ih->data->show_state = IUP_MAXIMIZE;
    return 1;
  }

  if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    ih->data->show_state = IUP_MINIMIZE;
    gtk_window_iconify((GtkWindow*)ih->handle);
  }
  else if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    ih->data->show_state = IUP_MAXIMIZE;
    gtk_window_maximize((GtkWindow*)ih->handle);
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height, x, y;
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    /* position the decoration outside the screen */
    x = -(border);
    y = -(border+caption+menu);

    /* the dialog client area will cover the task bar */
    iupdrvGetFullSize(&width, &height);

    height += menu; /* menu is inside the client area. */

    /* set the new size and position */
    /* The resize evt will update the layout */
    iupdrvDialogSetPosition(ih, x, y);
    gtk_window_resize((GtkWindow*)ih->handle, width, height); 

    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", NULL); /* reset to NORMAL */
  return 1;
}


/****************************************************************
                     Callbacks and Events
****************************************************************/


gboolean iupgtkDialogDeleteEvent(GtkWidget *widget, GdkEvent *evt, Ihandle *ih)
{
  Icallback cb;
  (void)widget;
  (void)evt;

  /* even when ACTIVE=NO the dialog gets this evt */
  if (!iupdrvIsActive(ih))
    return TRUE;

  cb = IupGetCallback(ih, "CLOSE_CB");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_IGNORE)
      return TRUE;
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  IupHide(ih); /* default: close the window */

  return TRUE; /* do not propagate */
}

static gboolean gtkDialogConfigureEvent(GtkWidget *widget, GdkEventConfigure *evt, Ihandle *ih)
{
  int old_width, old_height, old_x, old_y;
  gint x, y;
  (void)widget;

#ifndef HILDON
  /* In hildon the menu is not a menubar */
  if (ih->data->menu && ih->data->menu->handle)
  {
    if (evt->width > 0)
      gtk_widget_set_size_request(ih->data->menu->handle, evt->width, -1);
  }
#endif

  if (ih->data->ignore_resize) 
    return FALSE; 

  old_width = iupAttribGetInt(ih, "_IUPGTK_OLD_WIDTH");
  old_height = iupAttribGetInt(ih, "_IUPGTK_OLD_HEIGHT");

  /* Check the size change, because configure is called also for position changes */
  if (evt->width != old_width || evt->height != old_height)
  {
    IFnii cb;
    int border, caption, menu;
    iupAttribSetInt(ih, "_IUPGTK_OLD_WIDTH", evt->width);
    iupAttribSetInt(ih, "_IUPGTK_OLD_HEIGHT", evt->height);

    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  /* update dialog size */
#ifdef HILDON
    /* In Hildon, the configure event contains the window size, not the client area size */
    ih->currentwidth = evt->width;
    ih->currentheight = evt->height;
#else
    ih->currentwidth = evt->width + 2*border;
    ih->currentheight = evt->height + 2*border + caption;  /* menu is inside the window client area */
#endif

    cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
    if (!cb || cb(ih, evt->width, evt->height - menu)!=IUP_IGNORE)  /* width and height here are for the client area */
    {
      ih->data->ignore_resize = 1;
      IupRefresh(ih);
      ih->data->ignore_resize = 0;
    }
  }

  old_x = iupAttribGetInt(ih, "_IUPGTK_OLD_X");
  old_y = iupAttribGetInt(ih, "_IUPGTK_OLD_Y");
  iupdrvDialogGetPosition(ih, NULL, &x, &y);  /* ignore evt->x and evt->y because they are the clientpos and not X/Y */

  /* Check the position change, because configure is called also for size changes */
  if (x != old_x || y != old_y)
  {
    IFnii cb;
    iupAttribSetInt(ih, "_IUPGTK_OLD_X", x);
    iupAttribSetInt(ih, "_IUPGTK_OLD_Y", y);

    cb = (IFnii)IupGetCallback(ih, "MOVE_CB");
    if (cb)
      cb(ih, x, y);
  }

  return FALSE;
}

static gboolean gtkDialogWindowStateEvent(GtkWidget *widget, GdkEventWindowState *evt, Ihandle *ih)
{
  int state = -1;
  (void)widget;

  iupAttribSet(ih, "MAXIMIZED", NULL);
  iupAttribSet(ih, "MINIMIZED", NULL);

  if ((evt->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) &&        /* if flag changed and  */
      (evt->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) &&    /* is now set           */
      !(evt->new_window_state & GDK_WINDOW_STATE_WITHDRAWN))     /* is visible           */
  {
    state = IUP_MAXIMIZE;
    iupAttribSet(ih, "MAXIMIZED", "Yes");
  }
  else if ((evt->changed_mask & GDK_WINDOW_STATE_ICONIFIED) &&
           (evt->new_window_state & GDK_WINDOW_STATE_ICONIFIED) &&
           !(evt->new_window_state & GDK_WINDOW_STATE_WITHDRAWN))
  {
    state = IUP_MINIMIZE;
    iupAttribSet(ih, "MINIMIZED", "Yes");
  }
  else if ((evt->changed_mask & GDK_WINDOW_STATE_ICONIFIED) &&
           (evt->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) &&
           !(evt->new_window_state & GDK_WINDOW_STATE_WITHDRAWN))
    state = IUP_MAXIMIZE;
  else if (((evt->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) &&       /* maximized changed */
            !(evt->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) &&  /* not maximized     */
            !(evt->new_window_state & GDK_WINDOW_STATE_WITHDRAWN) &&  /* is visible        */
            !(evt->new_window_state & GDK_WINDOW_STATE_ICONIFIED))    /* not minimized     */
            ||                                                       /* OR                     */
           ((evt->changed_mask & GDK_WINDOW_STATE_ICONIFIED) &&       /* minimized changed   */
            !(evt->new_window_state & GDK_WINDOW_STATE_ICONIFIED) &&  /* not minimized       */
            !(evt->new_window_state & GDK_WINDOW_STATE_WITHDRAWN) &&  /* is visible          */
            !(evt->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)))   /* not maximized       */
    state = IUP_RESTORE;

  if (state < 0)
    return FALSE;

  if (ih->data->show_state != state)
  {
    IFni cb;
    ih->data->show_state = state;

    cb = (IFni)IupGetCallback(ih, "SHOW_CB");
    if (cb && cb(ih, state) == IUP_CLOSE) 
      IupExitLoop();
  }

  return FALSE;
}


/****************************************************************
                     Idialog Methods
****************************************************************/


/* replace the common dialog SetChildrenPosition method because of 
   the menu that it is inside the dialog. */
static void gtkDialogSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    char* offset = iupAttribGet(ih, "CHILDOFFSET");

    /* Native container, position is reset */
    x = 0;
    y = 0;

    if (offset) iupStrToIntInt(offset, &x, &y, 'x');

    y += gtkDialogGetMenuSize(ih);

    /* Child coordinates are relative to client left-top corner. */
    if (ih->firstchild)
      iupBaseSetPosition(ih->firstchild, x, y);
  }
}

static void* gtkDialogGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return (void*)gtk_bin_get_child((GtkBin*)ih->handle);
}

static int gtkDialogMapMethod(Ihandle* ih)
{
  int decorations = 0;
  int functions = 0;
  InativeHandle* parent;
  GtkWidget* inner_parent;
  int has_titlebar = 0;

#ifdef HILDON
  if (iupAttribGetBoolean(ih, "HILDONWINDOW")) 
  {
    HildonProgram *program = HILDON_PROGRAM(hildon_program_get_instance());
    ih->handle = hildon_window_new();
    if (ih->handle)
      hildon_program_add_window(program, HILDON_WINDOW(ih->handle)); 
  } 
  else 
  {
    iupAttribSet(ih, "DIALOGHINT", "YES");  /* otherwise not displayed correctly */ 
    ih->handle = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  }
#else
  ih->handle = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#endif
  if (!ih->handle)
    return IUP_ERROR;

  parent = iupDialogGetNativeParent(ih);
  if (parent)
  {
    gtk_window_set_transient_for((GtkWindow*)ih->handle, (GtkWindow*)parent);

    /* manually remove child windows when parent is destroyed */
    g_signal_connect(G_OBJECT(parent), "destroy", G_CALLBACK(gtkDialogChildDestroyEvent), ih);
  }

  g_signal_connect(G_OBJECT(ih->handle), "focus-in-event",     G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "focus-out-event",    G_CALLBACK(iupgtkFocusInOutEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "enter-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "leave-notify-event", G_CALLBACK(iupgtkEnterLeaveEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "show-help",          G_CALLBACK(iupgtkShowHelp), ih);

  /* The iupgtkKeyPressEvent of the control with the focus will propagate the key up to the dialog. */
  /* Inside iupgtkKeyPressEvent we test this to avoid duplicate calls. */
  g_signal_connect(G_OBJECT(ih->handle), "key-press-event",    G_CALLBACK(iupgtkKeyPressEvent), ih);

  g_signal_connect(G_OBJECT(ih->handle), "configure-event",    G_CALLBACK(gtkDialogConfigureEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "window-state-event", G_CALLBACK(gtkDialogWindowStateEvent), ih);
  g_signal_connect(G_OBJECT(ih->handle), "delete-event",       G_CALLBACK(iupgtkDialogDeleteEvent), ih);
                                    
  gtk_window_set_default_size((GtkWindow*)ih->handle, 100, 100); /* set this to avoid size calculation problems  */

  if (iupAttribGetBoolean(ih, "DIALOGHINT")) 
    gtk_window_set_type_hint(GTK_WINDOW(ih->handle), GDK_WINDOW_TYPE_HINT_DIALOG);

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME"))
  {
#if GTK_CHECK_VERSION(3, 10, 0)
    iupDialogCustomFrameSimulateCheckCallbacks(ih); /* no need for full simulation */
    iupAttribSet(ih, "HIDETITLEBAR", "Yes");
#else
    IupSetAttribute(ih, "CUSTOMFRAMESIMULATE", "Yes");
#endif
  } 

  if (iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
  {
    iupDialogCustomFrameSimulateCheckCallbacks(ih);

    g_signal_connect(G_OBJECT(ih->handle), "button-press-event", G_CALLBACK(iupgtkButtonEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "button-release-event", G_CALLBACK(iupgtkButtonEvent), ih);
    g_signal_connect(G_OBJECT(ih->handle), "motion-notify-event", G_CALLBACK(iupgtkMotionNotifyEvent), ih);

    gtk_widget_add_events(ih->handle, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
                                      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK);
  }

#if GTK_CHECK_VERSION(3, 10, 0)
  if (iupAttribGetBoolean(ih, "HIDETITLEBAR"))
    gtk_window_set_titlebar(GTK_WINDOW(ih->handle), gtk_fixed_new());
#endif

  /* the container that will receive the child element. */
  inner_parent = iupgtkNativeContainerNew(0);
  gtk_container_add((GtkContainer*)ih->handle, inner_parent);
  gtk_widget_show(inner_parent);

  /* initialize the widget */
  gtk_widget_realize(ih->handle);

  if (iupAttribGet(ih, "TITLE"))
    has_titlebar = 1;
  if (iupAttribGetBoolean(ih, "RESIZE")) 
  {
    functions   |= GDK_FUNC_RESIZE;
    decorations |= GDK_DECOR_RESIZEH;

    decorations |= GDK_DECOR_BORDER;  /* has_border */
  }
  else
    iupAttribSet(ih, "MAXBOX", "NO");
  if (iupAttribGetBoolean(ih, "MENUBOX")) 
  {
    functions   |= GDK_FUNC_CLOSE;
    decorations |= GDK_DECOR_MENU;
    has_titlebar = 1;
  }
  if (iupAttribGetBoolean(ih, "MAXBOX")) 
  {
    functions   |= GDK_FUNC_MAXIMIZE;
    decorations |= GDK_DECOR_MAXIMIZE;
    has_titlebar = 1;
  }
  if (iupAttribGetBoolean(ih, "MINBOX")) 
  {
    functions   |= GDK_FUNC_MINIMIZE;
    decorations |= GDK_DECOR_MINIMIZE;
    has_titlebar = 1;
  }
  if (has_titlebar)
  {
    functions   |= GDK_FUNC_MOVE;
    decorations |= GDK_DECOR_TITLE;
    gtk_window_set_title((GtkWindow*)ih->handle, "");
  }
  if (iupAttribGetBoolean(ih, "BORDER") || has_titlebar)
    decorations |= GDK_DECOR_BORDER;  /* has_border */

  if (decorations == 0)
    gtk_window_set_decorated((GtkWindow*)ih->handle, FALSE);
  else if (!iupAttribGetBoolean(ih, "HIDETITLEBAR"))
  {
    GdkWindow* window = iupgtkGetWindow(ih->handle);
    if (window)
    {
      gdk_window_set_decorations(window, (GdkWMDecoration)decorations);
      gdk_window_set_functions(window, (GdkWMFunction)functions);
    }
  }

  /* configure for DRAG&DROP */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  /* configure the size range */
  gtkDialogSetMinMax(ih, 1, 1, 65535, 65535);  /* MINSIZE and MAXSIZE default values */

  /* Ignore VISIBLE before mapping */
  iupAttribSet(ih, "VISIBLE", NULL);

  if (iupStrBoolean(IupGetGlobal("INPUTCALLBACKS")))
    gtk_widget_add_events(ih->handle, GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_BUTTON_MOTION_MASK);

  return IUP_NOERROR;
}

static void gtkDialogUnMapMethod(Ihandle* ih)
{
  GtkWidget* inner_parent, *parent;

#if GTK_CHECK_VERSION(2, 10, 0) && !GTK_CHECK_VERSION(3, 14, 0)
  GtkStatusIcon* status_icon;
#endif

  if (ih->data->menu) 
  {
    ih->data->menu->handle = NULL; /* the dialog will destroy the native menu */
    IupDestroy(ih->data->menu);  
    ih->data->menu = NULL;
  }

#if GTK_CHECK_VERSION(2, 10, 0) && !GTK_CHECK_VERSION(3, 14, 0)
  status_icon = (GtkStatusIcon*)iupAttribGet(ih, "_IUPDLG_STATUSICON");
  if (status_icon)
  {
    g_object_unref(status_icon);
    iupAttribSet(ih, "_IUPDLG_STATUSICON", NULL);
  }
#endif

  /* disconnect signal handlers */
#if GLIB_CHECK_VERSION(2, 32, 0)
  g_signal_handlers_disconnect_by_data(G_OBJECT(ih->handle), ih);
#endif
  parent = iupDialogGetNativeParent(ih);
  if (parent)
    g_signal_handlers_disconnect_by_func(G_OBJECT(parent), gtkDialogChildDestroyEvent, ih);

  inner_parent = gtk_bin_get_child((GtkBin*)ih->handle);
  gtk_widget_unrealize(inner_parent);
  gtk_widget_destroy(inner_parent);  

  gtk_widget_unrealize(ih->handle); /* To match the call to gtk_widget_realize */
  gtk_widget_destroy(ih->handle);   /* To match the call to gtk_window_new     */
}

static void gtkDialogLayoutUpdateMethod(Ihandle *ih)
{
  int border, caption, menu;
  int width, height;

  if (ih->data->ignore_resize ||
      iupAttribGet(ih, "_IUPGTK_FS_STYLE"))
    return;

  /* for dialogs the position is not updated here */
  ih->data->ignore_resize = 1;

  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  /* set size excluding the border */
  width = ih->currentwidth - 2*border;
  height = ih->currentheight - 2*border - caption;   /* menu is inside the client area. */
  if(width <= 0) width = 1;
  if(height <= 0) height = 1;
  gtk_window_resize((GtkWindow*)ih->handle, width, height);

  if (!iupAttribGetBoolean(ih, "RESIZE"))
  {
    GdkGeometry geometry;
    geometry.min_width = width;
    geometry.min_height = height;
    geometry.max_width = width;
    geometry.max_height = height;
    gtk_window_set_geometry_hints((GtkWindow*)ih->handle, ih->handle,
                                  &geometry, (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
  }

  ih->data->ignore_resize = 0;
}


/****************************************************************************
                                   Attributes
****************************************************************************/

static void gtkDialogSetMinMax(Ihandle* ih, int min_w, int min_h, int max_w, int max_h)
{
  /* The minmax size restricts the client area */
  GdkGeometry geometry;
  int decorwidth = 0, decorheight = 0;
  iupDialogGetDecorSize(ih, &decorwidth, &decorheight);

  geometry.min_width = 1;
  if (min_w > decorwidth)
    geometry.min_width = min_w-decorwidth;

  geometry.min_height = 1;
  if (min_h > decorheight)
    geometry.min_height = min_h-decorheight;

  geometry.max_width = 65535;
  if (max_w > decorwidth && max_w > geometry.min_width)
    geometry.max_width = max_w-decorwidth;

  geometry.max_height = 65535;
  if (max_h > decorheight && max_h > geometry.min_height)
    geometry.max_height = max_h-decorheight;

  /* must set both at the same time, or GTK will assume its default */
  gtk_window_set_geometry_hints((GtkWindow*)ih->handle, ih->handle,
                                &geometry, (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}

static int gtkDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  int min_w = 1, min_h = 1;          /* MINSIZE default value */
  int max_w = 65535, max_h = 65535;  /* MAXSIZE default value */
  iupStrToIntInt(value, &min_w, &min_h, 'x');

  /* if MAXSIZE also set, must be also updated here */
  iupStrToIntInt(iupAttribGet(ih, "MAXSIZE"), &max_w, &max_h, 'x');

  gtkDialogSetMinMax(ih, min_w, min_h, max_w, max_h);

  return iupBaseSetMinSizeAttrib(ih, value);
}

static int gtkDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  int min_w = 1, min_h = 1;          /* MINSIZE default value */
  int max_w = 65535, max_h = 65535;  /* MAXSIZE default value */
  iupStrToIntInt(value, &max_w, &max_h, 'x');

  /* if MINSIZE also set, must be also updated here */
  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');

  gtkDialogSetMinMax(ih, min_w, min_h, max_w, max_h);

  return iupBaseSetMaxSizeAttrib(ih, value);
}

static int gtkDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    value = "";
  gtk_window_set_title((GtkWindow*)ih->handle, iupgtkStrConvertToSystem(value));

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") || iupAttribGetBoolean(ih, "CUSTOMFRAMESIMULATE"))
    return 0;

  return 1;
}

static char* gtkDialogGetActiveWindowAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean (gtk_window_is_active((GtkWindow*)ih->handle)); 
}    

static char* gtkDialogGetClientSizeAttrib(Ihandle *ih)
{
  if (ih->handle)
  {
    int width, height;
    gtk_window_get_size((GtkWindow*)ih->handle, &width, &height);

    /* remove the menu because it is placed inside the client area */
    height -= gtkDialogGetMenuSize(ih);

    return iupStrReturnIntInt(width, height, 'x');
  }
  else
    return iupDialogGetClientSizeAttrib(ih);
}

static char* gtkDialogGetClientOffsetAttrib(Ihandle *ih)
{
  /* remove the menu because it is placed inside the client area */
  return iupStrReturnIntInt(0, -gtkDialogGetMenuSize(ih), 'x');
}

static int gtkDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{                       
  if (iupStrBoolean(value))
  {
    if (!iupAttribGet(ih, "_IUPGTK_FS_STYLE"))
    {
      /* save the previous decoration attributes */
      /* during fullscreen these attributes can be consulted by the application */
      iupAttribSetStr(ih, "_IUPGTK_FS_MAXBOX", iupAttribGet(ih, "MAXBOX"));
      iupAttribSetStr(ih, "_IUPGTK_FS_MINBOX", iupAttribGet(ih, "MINBOX"));
      iupAttribSetStr(ih, "_IUPGTK_FS_MENUBOX",iupAttribGet(ih, "MENUBOX"));
      iupAttribSetStr(ih, "_IUPGTK_FS_RESIZE", iupAttribGet(ih, "RESIZE"));
      iupAttribSetStr(ih, "_IUPGTK_FS_BORDER", iupAttribGet(ih, "BORDER"));
      iupAttribSetStr(ih, "_IUPGTK_FS_TITLE",  iupAttribGet(ih, "TITLE"));

      /* remove the decorations attributes */
      iupAttribSet(ih, "MAXBOX", "NO");
      iupAttribSet(ih, "MINBOX", "NO");
      iupAttribSet(ih, "MENUBOX", "NO");
      IupSetAttribute(ih, "TITLE", NULL);  /* must use IupSetAttribute to update the native implementation */
      iupAttribSet(ih, "RESIZE", "NO");
      iupAttribSet(ih, "BORDER", "NO");

      if (iupdrvIsVisible(ih))
        gtk_window_fullscreen((GtkWindow*)ih->handle);

      iupAttribSet(ih, "_IUPGTK_FS_STYLE", "YES");
    }
  }
  else
  {
    char* fs_style = iupAttribGet(ih, "_IUPGTK_FS_STYLE");
    if (fs_style)
    {
      iupAttribSet(ih, "_IUPGTK_FS_STYLE", NULL);

      /* restore the decorations attributes */
      iupAttribSetStr(ih, "MAXBOX", iupAttribGet(ih, "_IUPGTK_FS_MAXBOX"));
      iupAttribSetStr(ih, "MINBOX", iupAttribGet(ih, "_IUPGTK_FS_MINBOX"));
      iupAttribSetStr(ih, "MENUBOX",iupAttribGet(ih, "_IUPGTK_FS_MENUBOX"));
      IupSetAttribute(ih, "TITLE",  iupAttribGet(ih, "_IUPGTK_FS_TITLE"));   /* must use IupSetAttribute to update the native implementation */
      iupAttribSetStr(ih, "RESIZE", iupAttribGet(ih, "_IUPGTK_FS_RESIZE"));
      iupAttribSetStr(ih, "BORDER", iupAttribGet(ih, "_IUPGTK_FS_BORDER"));

      if (iupdrvIsVisible(ih))
        gtk_window_unfullscreen((GtkWindow*)ih->handle);

      /* remove auxiliar attributes */
      iupAttribSet(ih, "_IUPGTK_FS_MAXBOX", NULL);
      iupAttribSet(ih, "_IUPGTK_FS_MINBOX", NULL);
      iupAttribSet(ih, "_IUPGTK_FS_MENUBOX",NULL);
      iupAttribSet(ih, "_IUPGTK_FS_RESIZE", NULL);
      iupAttribSet(ih, "_IUPGTK_FS_BORDER", NULL);
      iupAttribSet(ih, "_IUPGTK_FS_TITLE",  NULL);
    }
  }
  return 1;
}

static int gtkDialogSetTopMostAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    gtk_window_set_keep_above((GtkWindow*)ih->handle, TRUE);
  else
    gtk_window_set_keep_above((GtkWindow*)ih->handle, FALSE);
  return 1;
}

#if GTK_CHECK_VERSION(2, 12, 0)
static int gtkDialogSetOpacityAttrib(Ihandle *ih, const char *value)
{
  int opacity;
  if (!iupStrToInt(value, &opacity))
    return 0;

#if GTK_CHECK_VERSION(3, 8, 0)
  gtk_widget_set_opacity(ih->handle, (double)opacity/255.0);
#else
  gtk_window_set_opacity((GtkWindow*)ih->handle, (double)opacity/255.0);
#endif
  return 1;
}

static int gtkDialogSetShapeImageAttrib(Ihandle *ih, const char *value)
{
  GdkPixbuf* pixbuf = iupImageGetImage(value, ih, 0, NULL);
  if (pixbuf)
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    GdkWindow* window = iupgtkGetWindow(ih->handle);
    if (window)
    {
      cairo_region_t *shape;

#if GTK_CHECK_VERSION(3, 10, 0)
      cairo_surface_t* surface = gdk_cairo_surface_create_from_pixbuf(pixbuf, 0, window);
#else
      int width = gdk_pixbuf_get_width(pixbuf);
      int height = gdk_pixbuf_get_height(pixbuf);
      cairo_surface_t* surface = gdk_window_create_similar_surface(window, CAIRO_CONTENT_COLOR_ALPHA, width, height);
      cairo_t* cr = cairo_create(surface);
      gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
      cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
      cairo_paint(cr);
      cairo_destroy(cr);
#endif

      shape = gdk_cairo_region_create_from_surface(surface);
      cairo_surface_destroy(surface);

      gtk_widget_shape_combine_region(ih->handle, shape);
      cairo_region_destroy(shape);
    }
#else
    GdkBitmap* mask = NULL;
    gdk_pixbuf_render_pixmap_and_mask(pixbuf, NULL, &mask, 255);
    if (mask) 
    {
      gtk_widget_shape_combine_mask(ih->handle, mask, 0, 0);
      g_object_unref(mask);
    }
#endif
    return 1;
  }
  return 0;
}

#endif

static int gtkDialogSetIconAttrib(Ihandle* ih, const char *value)
{
  if (!value)
    gtk_window_set_icon((GtkWindow*)ih->handle, NULL);
  else
  {
    GdkPixbuf* icon = (GdkPixbuf*)iupImageGetIcon(value);
    if (icon)
      gtk_window_set_icon((GtkWindow*)ih->handle, icon);
  }
  return 1;
}

static int gtkDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  if (iupdrvBaseSetBgColorAttrib(ih, value))
  {
#if GTK_CHECK_VERSION(3, 0, 0)
    GdkWindow* window = iupgtkGetWindow(ih->handle);
    if (window)
      gdk_window_set_background_pattern(window, NULL);
#else
    GtkStyle *style = gtk_widget_get_style(ih->handle);
    if (style->bg_pixmap[GTK_STATE_NORMAL])
    {
      style = gtk_style_copy(style);
      style->bg_pixmap[GTK_STATE_NORMAL] = NULL;
      gtk_widget_set_style(ih->handle, style);
    }
#endif
    return 1;
  }
  else
  {
    GdkPixbuf* pixbuf = iupImageGetImage(value, ih, 0, NULL);
    if (pixbuf)
    {
#if GTK_CHECK_VERSION(3, 0, 0)
    GdkWindow* window = iupgtkGetWindow(ih->handle);
    if (window)
    {
      /* TODO: this is NOT working!!!! */
      cairo_pattern_t* pattern;
      int width = gdk_pixbuf_get_width(pixbuf);
      int height = gdk_pixbuf_get_height(pixbuf);

      cairo_surface_t* surface = gdk_window_create_similar_surface(window, CAIRO_CONTENT_COLOR_ALPHA, width, height);
      cairo_t* cr = cairo_create(surface);
      gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
      cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
      cairo_paint(cr);
      cairo_destroy(cr);

      pattern = cairo_pattern_create_for_surface(surface);
      cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

      gdk_window_set_background_pattern(window, pattern);
      cairo_pattern_destroy (pattern);

      cairo_surface_destroy(surface);
    }
#else
      GdkPixmap* pixmap;
      GtkStyle *style = gtk_style_copy(gtk_widget_get_style(ih->handle));

      gdk_pixbuf_render_pixmap_and_mask(pixbuf, &pixmap, NULL, 255);

      style->bg_pixmap[GTK_STATE_NORMAL] = pixmap;
      gtk_widget_set_style(ih->handle, style);
#endif

      return 1;
    }
  }

  return 0;
}

/* gtk_status_icon - deprecated in 3.14, but still available in 3.22 */
#if GTK_CHECK_VERSION(2, 10, 0)
static int gtkDialogTaskDoubleClick(int button)
{
  static int last_button = -1;
  static GTimer* timer = NULL;
  if (last_button == -1 || last_button != button)
  {
    last_button = button;
    if (timer)
      g_timer_destroy(timer);
    timer = g_timer_new();
    return 0;
  }
  else
  {
    double seconds;

    if (!timer)  /* just in case */
      return 0;

    seconds = g_timer_elapsed(timer, NULL);
    if (seconds < 0.4)
    {
      /* reset state */
      g_timer_destroy(timer);
      timer = NULL;
      last_button = -1;  
      return 1;
    }
    else
    {
      g_timer_reset(timer);
      return 0;
    }
  }
}

static void gtkDialogTaskAction(GtkStatusIcon *status_icon, Ihandle *ih)
{
  /* from GTK source code it is called only when button==1 and pressed==1 */
  int button = 1;
  int pressed = 1;
  int dclick = gtkDialogTaskDoubleClick(button);
  IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
  if (cb && cb(ih, button, pressed, dclick) == IUP_CLOSE)
    IupExitLoop();
  (void)status_icon;
}

static void gtkDialogTaskPopupMenu(GtkStatusIcon *status_icon, guint gbutton, guint activate_time, Ihandle *ih)
{
  /* from GTK source code it is called only when button==3 and pressed==1 */
  int button = 3;
  int pressed = 1;
  int dclick = gtkDialogTaskDoubleClick(button);
  IFniii cb = (IFniii)IupGetCallback(ih, "TRAYCLICK_CB");
  if (cb && cb(ih, button, pressed, dclick) == IUP_CLOSE)
    IupExitLoop();
  (void)activate_time;
  (void)gbutton;
  (void)status_icon;
}

static GtkStatusIcon* gtkDialogGetStatusIcon(Ihandle *ih)
{
  GtkStatusIcon* status_icon = (GtkStatusIcon*)iupAttribGet(ih, "_IUPDLG_STATUSICON");
  if (!status_icon)
  {
    status_icon = gtk_status_icon_new();

    g_signal_connect(G_OBJECT(status_icon), "activate", G_CALLBACK(gtkDialogTaskAction), ih);
    g_signal_connect(G_OBJECT(status_icon), "popup-menu", G_CALLBACK(gtkDialogTaskPopupMenu), ih);

    iupAttribSet(ih, "_IUPDLG_STATUSICON", (char*)status_icon);
  }
  return status_icon;
}

static int gtkDialogSetTrayAttrib(Ihandle *ih, const char *value)
{
  GtkStatusIcon* status_icon = gtkDialogGetStatusIcon(ih);
  gtk_status_icon_set_visible(status_icon, iupStrBoolean(value));
  return 1;
}

static int gtkDialogSetTrayTipAttrib(Ihandle *ih, const char *value)
{
  GtkStatusIcon* status_icon = gtkDialogGetStatusIcon(ih);
#if GTK_CHECK_VERSION(2, 16, 0)
  if (value)
  {
    gtk_status_icon_set_has_tooltip(status_icon, TRUE);
    if (iupAttribGetBoolean(ih, "TIPMARKUP"))
      gtk_status_icon_set_tooltip_markup(status_icon, value);
    else
      gtk_status_icon_set_tooltip_text(status_icon, value);
  }
  else
    gtk_status_icon_set_has_tooltip(status_icon, FALSE);
#else
  gtk_status_icon_set_tooltip(status_icon, value);
#endif
  return 1;
}

static int gtkDialogSetTrayImageAttrib(Ihandle *ih, const char *value)
{
  GtkStatusIcon* status_icon = gtkDialogGetStatusIcon(ih);
  GdkPixbuf* icon = (GdkPixbuf*)iupImageGetIcon(value);
  gtk_status_icon_set_from_pixbuf(status_icon, icon);
  return 1;
}
#endif  /* gtk_status_icon */

#if GTK_CHECK_VERSION(3, 4, 0)
static int gtkDialogSetHideTitleBarAttrib(Ihandle *ih, const char *value)
{
  gtk_window_set_hide_titlebar_when_maximized(GTK_WINDOW(ih->handle), iupStrBoolean(value));
  return 1;
}
#endif  /* GTK_CHECK_VERSION(3, 4, 0) */

/****************************************************************************************************************/


void iupdrvDialogInitClass(Iclass* ic)
{
  /* Driver Dependent Class methods */
  ic->Map = gtkDialogMapMethod;
  ic->UnMap = gtkDialogUnMapMethod;
  ic->LayoutUpdate = gtkDialogLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = gtkDialogGetInnerNativeContainerHandleMethod;
  ic->SetChildrenPosition = gtkDialogSetChildrenPositionMethod;

  /* Callback Windows and GTK Only */
  iupClassRegisterCallback(ic, "TRAYCLICK_CB", "iii");

  /* Driver Dependent Attribute functions */
  iupClassRegisterAttribute(ic, iupgtkGetNativeWindowHandleName(), iupgtkGetNativeWindowHandleAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_NO_STRING);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, "DLGBGCOLOR", NULL, IUPAF_DEFAULT);  /* force new default value */

  /* Base Container */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", gtkDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);  /* dialog is the only not read-only */
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", gtkDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* Special */
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtkDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupDialog only */
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, gtkDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, gtkDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, gtkDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", NULL, gtkDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, gtkDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);  /* saveunder not supported in GTK */
  iupClassRegisterAttribute(ic, "MAXIMIZED", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupDialog Windows and GTK Only */
  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", gtkDialogGetActiveWindowAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, gtkDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DIALOGHINT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
#if GTK_CHECK_VERSION(2, 12, 0)
  iupClassRegisterAttribute(ic, "OPACITY", NULL, gtkDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, gtkDialogSetShapeImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHAPEIMAGE", NULL, gtkDialogSetShapeImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
#endif
/* gtk_status_icon - deprecated in 3.14, but still available in 3.22 */
#if GTK_CHECK_VERSION(2, 10, 0)
  iupClassRegisterAttribute(ic, "TRAY", NULL, gtkDialogSetTrayAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYIMAGE", NULL, gtkDialogSetTrayImageAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIP", NULL, gtkDialogSetTrayTipAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TRAYTIPMARKUP", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);
#endif
  iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NOT_MAPPED);

#if GTK_CHECK_VERSION(3, 4, 0)
  iupClassRegisterAttribute(ic, "HIDETITLEBAR", NULL, gtkDialogSetHideTitleBarAttrib, NULL, NULL, IUPAF_NO_INHERIT);
#endif

  /* Not Supported */
  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLIENT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIMENU", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICHILD", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
