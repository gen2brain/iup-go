/** \file
 * \brief Menu Resources
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_label.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_menu.h"

#include "iupgtk4_drv.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

/* Menus are completely redesigned using GMenu and GAction. GtkMenuBar, GtkMenu, GtkMenuItem are removed.
   We use GMenu model with GSimpleActionGroup for proper native menus. */

typedef struct _ImenuPos
{
  int x, y;
  Ihandle* ih;
} ImenuPos;

static int gtk4IsX11Backend(void)
{
#ifdef GDK_WINDOWING_X11
  GdkDisplay *display = gdk_display_get_default();
  return (display && GDK_IS_X11_DISPLAY(display));
#else
  return 0;
#endif
}

static void gtk4MenuActionActivated(GSimpleAction* action, GVariant* parameter, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  Icallback cb;
  GVariant* state;

  if (!ih)
    return;

  /* Handle AUTOTOGGLE for checkable items */
  state = g_action_get_state(G_ACTION(action));
  if (state != NULL)
  {
    /* This is a stateful (checkable) action */
    gboolean checked = g_variant_get_boolean(state);
    g_variant_unref(state);

    if (iupAttribGetBoolean(ih, "AUTOTOGGLE"))
    {
      /* Toggle the state */
      checked = !checked;
      g_simple_action_set_state(action, g_variant_new_boolean(checked));

      /* Update IUP's VALUE attribute */
      iupAttribSet(ih, "VALUE", checked ? "ON" : "OFF");

    }
  }

  cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_CLOSE)
    {
      IupExitLoop();
    }
  }

  (void)parameter;
}

/* Recursively build GMenu model from IUP menu hierarchy
 * is_root: TRUE if this is the root menu bar level (requires submenus only), FALSE if this is inside a submenu */
static GMenu* gtk4BuildMenuModel(Ihandle* ih_menu, GSimpleActionGroup* action_group, int is_root)
{
  GMenu* menu;
  Ihandle* child;


  if (!ih_menu)
    return NULL;

  menu = g_menu_new();

  for (child = ih_menu->firstchild; child; child = child->brother)
  {
    char* title;
    char* processed_title;
    char c = '_';

    if (iupStrEqual(child->iclass->name, "submenu"))
    {
      /* Submenu: get title and recursively build submenu */
      Ihandle* submenu_ih = child->firstchild;  /* The Menu child of Submenu */
      GMenu* submenu_model;

      title = iupAttribGet(child, "TITLE");
      if (!title) title = "";

      /* Process mnemonic: convert & to _ for GTK */
      processed_title = iupStrProcessMnemonic(title, &c, 1);


      if (submenu_ih)
      {
        /* Submenus are never at root, so pass FALSE */
        submenu_model = gtk4BuildMenuModel(submenu_ih, action_group, FALSE);
        if (submenu_model)
        {
          g_menu_append_submenu(menu, processed_title, G_MENU_MODEL(submenu_model));
          g_object_unref(submenu_model);  /* menu takes ownership */
        }
      }

      /* Free processed title if it was allocated */
      if (processed_title != title)
        free(processed_title);
    }
    else if (iupStrEqual(child->iclass->name, "item"))
    {
      /* Menu item: create action and add to menu */
      GSimpleAction* action;
      char action_name[64];
      int is_checkable = 0;
      int is_checked = 0;
      int is_active = 1;

      title = iupAttribGet(child, "TITLE");
      if (!title) title = "";

      /* Process mnemonic: convert & to _ for GTK */
      processed_title = iupStrProcessMnemonic(title, &c, 1);

      /* Create unique action name using ih pointer */
      snprintf(action_name, sizeof(action_name), "item-%p", (void*)child);


      /* Check if this is a checkable item (VALUE attribute present or HIDEMARK not set) */
      char* value_str = iupAttribGetStr(child, "VALUE");

      if (value_str || !iupAttribGetBoolean(child, "HIDEMARK"))
      {
        is_checkable = 1;
        is_checked = iupStrBoolean(value_str);
      }

      if (iupAttribGet(child, "ACTIVE"))
        is_active = iupAttribGetBoolean(child, "ACTIVE");

      /* Create action with state for checkable items */
      if (is_checkable)
      {
        GVariant* state = g_variant_new_boolean(is_checked);
        action = g_simple_action_new_stateful(action_name, NULL, state);

        /* Store attributes needed for VALUE get/set - use SetStr to duplicate strings */
        iupAttribSetStr(child, "_IUPGTK4_CHECKABLE", "1");
        iupAttribSetStr(child, "_IUPGTK4_ACTION_NAME", action_name);
      }
      else
      {
        action = g_simple_action_new(action_name, NULL);
      }

      g_signal_connect(action, "activate", G_CALLBACK(gtk4MenuActionActivated), child);
      g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
      g_simple_action_set_enabled(action, is_active);
      g_object_unref(action);  /* action_group takes ownership */

      /* Add to menu with full action name */
      char full_action_name[96];
      snprintf(full_action_name, sizeof(full_action_name), "menu.%s", action_name);

      if (is_root)
      {
        /* GtkPopoverMenuBar requires submenus at root level.
         * For root-level items, create a submenu with single unlabeled item.
         * Menu bar shows the title, clicking opens dropdown and immediately triggers the action. */
        GMenu* item_submenu = g_menu_new();
        /* Add item without label - will just trigger the action */
        g_menu_append(item_submenu, NULL, full_action_name);
        g_menu_append_submenu(menu, processed_title, G_MENU_MODEL(item_submenu));
        g_object_unref(item_submenu);
      }
      else
      {
        /* Regular submenu item - add directly */
        g_menu_append(menu, processed_title, full_action_name);
      }

      /* Free processed title if it was allocated */
      if (processed_title != title)
        free(processed_title);
    }
    else if (iupStrEqual(child->iclass->name, "separator"))
    {
      if (is_root)
      {
        /* Separators cannot be at root level in GtkPopoverMenuBar - skip */
      }
      else
      {
        /* Separator in submenu */
        g_menu_append(menu, NULL, NULL);
      }
    }
  }

  return menu;
}

void iupgtk4DialogSetMenuBar(Ihandle* ih_dialog, Ihandle* ih_menu)
{
  GMenu* menubar_model;
  GSimpleActionGroup* action_group;
  GtkWidget* menubar_widget;
  GtkWidget* inner_parent;


  if (!ih_dialog || !ih_menu)
    return;

  /* Create action group for this dialog's menu */
  action_group = g_simple_action_group_new();

  /* Store action group on dialog for later access by VALUE attribute setter */
  iupAttribSet(ih_dialog, "_IUPGTK4_MENU_ACTION_GROUP", (char*)action_group);

  /* Build menu model from IUP hierarchy (is_root=TRUE for menu bar) */
  menubar_model = gtk4BuildMenuModel(ih_menu, action_group, TRUE);
  if (!menubar_model)
  {
    g_object_unref(action_group);
    return;
  }

  /* Insert action group into window with "menu" prefix */
  gtk_widget_insert_action_group(ih_dialog->handle, "menu", G_ACTION_GROUP(action_group));
  g_object_unref(action_group);  /* window takes ownership */

  /* Create GtkPopoverMenuBar widget from model */
  menubar_widget = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menubar_model));
  g_object_unref(menubar_model);  /* menubar_widget takes ownership */

  /* Get menu VBox container */
  GtkWidget* menu_box = (GtkWidget*)iupAttribGet(ih_dialog, "_IUPGTK4_MENU_BOX");
  if (!menu_box)
  {
    gtk_widget_unparent(menubar_widget);
    return;
  }

  /* Configure expansion properties BEFORE adding to box. */
  inner_parent = (GtkWidget*)iupAttribGet(ih_dialog, "_IUPGTK4_INNER_PARENT");

  /* Menu bar should NOT expand - it wants its natural height only */
  gtk_widget_set_vexpand(menubar_widget, FALSE);
  gtk_widget_set_hexpand(menubar_widget, TRUE);  /* But expand horizontally */

  /* inner_parent MUST expand to fill remaining vertical space */
  gtk_widget_set_vexpand(inner_parent, TRUE);
  gtk_widget_set_hexpand(inner_parent, TRUE);

  /* Add menu bar to the top of the menu box (before inner_parent) */
  gtk_box_prepend(GTK_BOX(menu_box), menubar_widget);

  /* Store the menubar widget handle in the menu Ihandle */
  ih_menu->handle = menubar_widget;
}

int iupdrvMenuGetMenuBarSize(Ihandle* ih_menu)
{
  GtkWidget* menubar_widget;

  if (!ih_menu || !ih_menu->handle)
    return 0;

  menubar_widget = (GtkWidget*)ih_menu->handle;

  int height = gtk_widget_get_allocated_height(menubar_widget);

  /* If not yet allocated (during initial layout), measure natural size */
  if (height == 0)
  {
    int min_height, nat_height;
    gtk_widget_measure(menubar_widget, GTK_ORIENTATION_VERTICAL, -1, &min_height, &nat_height, NULL, NULL);
    height = nat_height;
  }

  return height;
}

static void gtk4PopoverClosedCb(GtkPopover *popover, gpointer user_data)
{
  GMainLoop* loop = (GMainLoop*)user_data;
  if (loop && g_main_loop_is_running(loop))
    g_main_loop_quit(loop);
}

static void gtk4AnchorPopoverClosedCb(GtkPopover *popover, gpointer user_data)
{
  Ihandle *ih = (Ihandle*)user_data;
  GtkWidget *anchor_window;

  /* Hide anchor window when popover closes (for tray popups) */
  anchor_window = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ANCHOR_WINDOW");
  if (anchor_window)
    gtk_widget_set_visible(anchor_window, FALSE);

  (void)popover;
}

int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  GtkWidget* popover;
  GtkWidget* parent_widget = NULL;
  GMenu* menu_model;
  GSimpleActionGroup* action_group;
  GMainLoop* loop;
  int local_x = 0, local_y = 0;
  Ihandle* popup_dialog;
  int use_anchor_window = 0;

  /* Get stored menu model and action group from MapMethod */
  menu_model = (GMenu*)iupAttribGet(ih, "_IUPGTK4_MENU_MODEL");
  action_group = (GSimpleActionGroup*)iupAttribGet(ih, "_IUPGTK4_ACTION_GROUP");

  if (!menu_model || !action_group)
    return IUP_ERROR;

  /* Get the dialog that triggered the popup (stored by controls like tree) */
  popup_dialog = (Ihandle*)IupGetGlobal("_IUPGTK4_POPUP_DIALOG");
  IupSetGlobal("_IUPGTK4_POPUP_DIALOG", NULL);

  if (popup_dialog)
  {
    parent_widget = (GtkWidget*)iupAttribGet(popup_dialog, "_IUPGTK4_INNER_PARENT");
    if (!parent_widget)
      parent_widget = (GtkWidget*)popup_dialog->handle;
  }

  /* If no popup_dialog was set, try to find a visible dialog */
  if (!parent_widget)
  {
    /* Try to get the active/focused window from GTK */
    GList* toplevels = gtk_window_list_toplevels();
    GList* l;
    for (l = toplevels; l != NULL; l = l->next)
    {
      GtkWindow* win = GTK_WINDOW(l->data);
      if (gtk_window_is_active(win) && gtk_widget_get_visible(GTK_WIDGET(win)))
      {
        /* Found an active visible window, get its inner_parent if it's an IUP dialog */
        Ihandle* dlg = (Ihandle*)g_object_get_data(G_OBJECT(win), "IUP");
        if (dlg)
        {
          parent_widget = (GtkWidget*)iupAttribGet(dlg, "_IUPGTK4_INNER_PARENT");
          if (!parent_widget)
            parent_widget = GTK_WIDGET(win);
        }
        else
          parent_widget = GTK_WIDGET(win);
        break;
      }
    }
    g_list_free(toplevels);
  }

  /* If still no parent_widget (e.g., tray popup with no visible window), use anchor window */
  if (!parent_widget)
  {
    use_anchor_window = 1;
  }

  if (use_anchor_window)
  {
    /* Tray popup: Use positioned anchor window for free positioning at (x,y) */
    GtkWidget* anchor_window = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ANCHOR_WINDOW");
    GtkWidget* old_popover = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_POPOVER");

    /* Clean up previous popover if exists */
    if (old_popover && GTK_IS_WIDGET(old_popover))
    {
      GtkWidget* parent = gtk_widget_get_parent(old_popover);
      if (parent)
        gtk_widget_unparent(old_popover);
      iupAttribSet(ih, "_IUPGTK4_POPOVER", NULL);
    }

    if (!anchor_window)
    {
      /* Create a new invisible anchor window for the popover */
      anchor_window = gtk_window_new();
      gtk_window_set_decorated(GTK_WINDOW(anchor_window), FALSE);
      gtk_window_set_default_size(GTK_WINDOW(anchor_window), 1, 1);
      gtk_widget_set_opacity(anchor_window, 0.0);
      gtk_window_set_deletable(GTK_WINDOW(anchor_window), FALSE);

      iupAttribSet(ih, "_IUPGTK4_ANCHOR_WINDOW", (char*)anchor_window);

      gtk_window_present(GTK_WINDOW(anchor_window));

      /* Wait for window to be fully mapped */
      while (!gtk_widget_get_mapped(anchor_window))
        g_main_context_iteration(NULL, FALSE);

#ifdef GDK_WINDOWING_X11
      if (gtk4IsX11Backend())
      {
        GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(anchor_window));
        if (surface && GDK_IS_X11_SURFACE(surface))
        {
          GdkDisplay *gdk_display = gdk_display_get_default();
          Display *xdisplay = gdk_x11_display_get_xdisplay(gdk_display);
          Window xwindow = gdk_x11_surface_get_xid(surface);

          if (xdisplay && xwindow)
            iupgtk4X11HideFromTaskbar(xdisplay, xwindow);
        }
      }
#endif
    }
    else
    {
      gtk_widget_set_visible(anchor_window, TRUE);
      gtk_window_present(GTK_WINDOW(anchor_window));
    }

#ifdef GDK_WINDOWING_X11
    /* On X11: Position the anchor window at (x,y) */
    if (gtk4IsX11Backend())
    {
      GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(anchor_window));
      if (surface && GDK_IS_X11_SURFACE(surface))
      {
        GdkDisplay *gdk_display = gdk_display_get_default();
        Display *xdisplay = gdk_x11_display_get_xdisplay(gdk_display);
        Window xwindow = gdk_x11_surface_get_xid(surface);

        if (xdisplay && xwindow)
          iupgtk4X11MoveWindow(xdisplay, xwindow, x, y);
      }
    }
#endif

    /* Create popover for anchor window approach */
    popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu_model));
    if (!popover)
      return IUP_ERROR;

    /* Insert action group into anchor window */
    gtk_widget_insert_action_group(anchor_window, "menu", G_ACTION_GROUP(action_group));

    gtk_widget_set_parent(popover, anchor_window);
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);

    GdkRectangle pointing_rect = {0, 0, 1, 1};
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &pointing_rect);
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);

    /* On Wayland: Use offset to position popover at screen coordinates (x,y)
     * since we can't position the anchor window itself */
#ifndef GDK_WINDOWING_X11
    gtk_popover_set_offset(GTK_POPOVER(popover), x, y);
#else
    if (!gtk4IsX11Backend())
    {
      gtk_popover_set_offset(GTK_POPOVER(popover), x, y);
    }
#endif

    /* Store popover for cleanup */
    iupAttribSet(ih, "_IUPGTK4_POPOVER", (char*)popover);
    ih->handle = popover;

    /* Connect closed signal to hide anchor window */
    g_signal_connect(popover, "closed", G_CALLBACK(gtk4AnchorPopoverClosedCb), (gpointer)ih);

    gtk_popover_popup(GTK_POPOVER(popover));

    return IUP_NOERROR;
  }

  /* Dialog popup path (tree, etc.): Convert screen coordinates to parent_widget-local coordinates */
  {
    GtkNative *native = gtk_widget_get_native(parent_widget);
    if (native)
    {
      double native_x, native_y;
      gtk_native_get_surface_transform(native, &native_x, &native_y);

      graphene_point_t point_in = {x - native_x, y - native_y};
      graphene_point_t point_out;

      if (gtk_widget_compute_point(GTK_WIDGET(native), parent_widget, &point_in, &point_out))
      {
        local_x = (int)point_out.x;
        local_y = (int)point_out.y;
      }
      else
      {
        local_x = x;
        local_y = y;
      }
    }
  }

  /* Clean up previous popover if exists */
  {
    GtkWidget* old_popover = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_POPOVER");
    if (old_popover && GTK_IS_WIDGET(old_popover))
    {
      GtkWidget* parent = gtk_widget_get_parent(old_popover);
      if (parent)
        gtk_widget_unparent(old_popover);
      iupAttribSet(ih, "_IUPGTK4_POPOVER", NULL);
    }
  }

  /* Create a nested main loop, this will block until the popover is closed.
     This is needed because IUP expects IupPopup() to be synchronous (like GTK3's gtk_menu_popup + gtk_main). */
  loop = g_main_loop_new(NULL, FALSE);

  /* Create a fresh popover each time */
  popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu_model));
  if (!popover)
  {
    g_main_loop_unref(loop);
    return IUP_ERROR;
  }

  /* Connect closed signal to quit the nested main loop */
  g_signal_connect(popover, "closed", G_CALLBACK(gtk4PopoverClosedCb), (gpointer)loop);

  /* Store popover for cleanup in UnMapMethod */
  iupAttribSet(ih, "_IUPGTK4_POPOVER", (char*)popover);
  iupAttribSet(ih, "_IUPGTK4_POPOVER_PARENT", (char*)parent_widget);
  ih->handle = popover;

  /* Insert action group into the parent widget */
  gtk_widget_insert_action_group(parent_widget, "menu", G_ACTION_GROUP(action_group));

  /* Parent popover to the dialog's inner container */
  gtk_widget_set_parent(popover, parent_widget);

  gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);

  /* Position relative to parent widget at click location */
  {
    GdkRectangle pointing_rect = {local_x, local_y, 1, 1};
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &pointing_rect);
  }
  gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);

  /* Show the popover */
  gtk_popover_popup(GTK_POPOVER(popover));

  /* Block here until popover is closed, the closed signal handler will quit the loop */
  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  return IUP_NOERROR;
}

static void gtk4ItemUpdateImage(Ihandle* ih, const char* value, const char* image, const char* impress)
{
  GdkTexture* texture;
  GtkWidget* box;
  GtkWidget* img_widget;

  if (!impress || !iupStrBoolean(value))
    texture = iupImageGetImage(image, ih, 0, NULL);
  else
    texture = iupImageGetImage(impress, ih, 0, NULL);

  /* Menu items use custom box with image and label */
  box = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_MENU_BOX");
  if (!box)
    return;

  img_widget = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_MENU_IMAGE");

  if (texture)
  {
    if (!img_widget)
    {
      img_widget = gtk_image_new();
      iupAttribSet(ih, "_IUPGTK4_MENU_IMAGE", (char*)img_widget);
      gtk_box_prepend(GTK_BOX(box), img_widget);
    }

    gtk_image_set_from_paintable(GTK_IMAGE(img_widget), GDK_PAINTABLE(texture));
  }
  else if (img_widget)
  {
    gtk_box_remove(GTK_BOX(box), img_widget);
    iupAttribSet(ih, "_IUPGTK4_MENU_IMAGE", NULL);
  }
}

static void gtk4MenuShow(GtkWidget *widget, Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "OPEN_CB");
  if (!cb && ih->parent) cb = (Icallback)IupGetCallback(ih->parent, "OPEN_CB");
  if (cb) cb(ih);

  (void)widget;
}

static void gtk4MenuHide(GtkWidget *widget, Ihandle* ih)
{
  Icallback cb = IupGetCallback(ih, "MENUCLOSE_CB");
  if (!cb && ih->parent) cb = (Icallback)IupGetCallback(ih->parent, "MENUCLOSE_CB");
  if (cb) cb(ih);

  (void)widget;
}

static int gtk4MenuMapMethod(Ihandle* ih)
{
  /* Menubars are created in iupgtk4DialogSetMenuBar(), but popup menus need GtkPopoverMenu */

  if (iupMenuIsMenuBar(ih))
  {
    /* Handled in iupgtk4DialogSetMenuBar() when dialog is mapped */
    return IUP_NOERROR;
  }

  /* This is a popup menu - store data for deferred creation in iupdrvMenuPopup.
   * We can't create the popover here because it needs a parent widget to resolve actions.
   * Store the action group and menu model, create popover when popup is called. */
  {
    GMenu* menu_model;
    GSimpleActionGroup* action_group;

    /* Create action group for this popup menu */
    action_group = g_simple_action_group_new();

    /* Build menu model from IUP hierarchy (is_root=FALSE for popup menus) */
    menu_model = gtk4BuildMenuModel(ih, action_group, FALSE);
    if (!menu_model)
    {
      g_object_unref(action_group);
      return IUP_ERROR;
    }

    /* Store for deferred popover creation */
    iupAttribSet(ih, "_IUPGTK4_MENU_MODEL", (char*)menu_model);
    iupAttribSet(ih, "_IUPGTK4_ACTION_GROUP", (char*)action_group);

    /* Set a placeholder handle so IupMap considers this mapped */
    ih->handle = (GtkWidget*)ih;  /* Placeholder, real handle set in iupdrvMenuPopup */
  }

  return IUP_NOERROR;
}

static void gtk4MenuUnMapMethod(Ihandle* ih)
{
  GMenu* menu_model = (GMenu*)iupAttribGet(ih, "_IUPGTK4_MENU_MODEL");
  GSimpleActionGroup* action_group = (GSimpleActionGroup*)iupAttribGet(ih, "_IUPGTK4_ACTION_GROUP");
  GtkWidget* popover = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_POPOVER");
  GtkWidget* popover_parent = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_POPOVER_PARENT");
  GtkWidget* anchor_window = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ANCHOR_WINDOW");

  /* Remove action group from parent widget first */
  if (popover_parent && GTK_IS_WIDGET(popover_parent))
  {
    gtk_widget_insert_action_group(popover_parent, "menu", NULL);
    iupAttribSet(ih, "_IUPGTK4_POPOVER_PARENT", NULL);
  }

  /* Remove action group from anchor window if used */
  if (anchor_window && GTK_IS_WIDGET(anchor_window))
  {
    gtk_widget_insert_action_group(anchor_window, "menu", NULL);
  }

  if (popover && GTK_IS_WIDGET(popover))
  {
    GtkWidget* parent = gtk_widget_get_parent(popover);
    if (parent)
      gtk_widget_unparent(popover);

    iupAttribSet(ih, "_IUPGTK4_POPOVER", NULL);
  }

  /* Clean up anchor window if it was created (for tray popups) */
  if (anchor_window && GTK_IS_WIDGET(anchor_window))
  {
    gtk_window_destroy(GTK_WINDOW(anchor_window));
    iupAttribSet(ih, "_IUPGTK4_ANCHOR_WINDOW", NULL);
  }

  if (menu_model)
  {
    g_object_unref(menu_model);
    iupAttribSet(ih, "_IUPGTK4_MENU_MODEL", NULL);
  }

  if (action_group)
  {
    g_object_unref(action_group);
    iupAttribSet(ih, "_IUPGTK4_ACTION_GROUP", NULL);
  }

  if (iupMenuIsMenuBar(ih))
    ih->parent = NULL;

  /* For popup menus, handle is the popover which we just cleaned up. */
  ih->handle = NULL;
}

void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = gtk4MenuMapMethod;
  ic->UnMap = gtk4MenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
}

static int gtk4ItemSetTitleImageAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGet(ih, "_IUPGTK4_IMAGE_ITEM"))
  {
    gtk4ItemUpdateImage(ih, NULL, value, NULL);
    return 1;
  }
  return 0;
}

static int gtk4ItemSetImageAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGet(ih, "_IUPGTK4_IMAGE_ITEM"))
  {
    gtk4ItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), value, iupAttribGet(ih, "IMPRESS"));
    return 1;
  }
  return 0;
}

static int gtk4ItemSetImpressAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGet(ih, "_IUPGTK4_IMAGE_ITEM"))
  {
    gtk4ItemUpdateImage(ih, iupAttribGet(ih, "VALUE"), iupAttribGet(ih, "IMAGE"), value);
    return 1;
  }
  return 0;
}

static int gtk4ItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  char *str;
  GtkWidget* label;

  if (!value)
  {
    str = "     ";
    value = str;
  }
  else
    str = iupMenuProcessTitle(ih, value);

  label = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_MENU_LABEL");
  if (label)
    iupgtk4SetMnemonicTitle(ih, (GtkLabel*)label, str);

  if (str != value) free(str);
  return 1;
}

static int gtk4ItemSetValueAttrib(Ihandle* ih, const char* value)
{
  /* GMenu-based system: check state is stored in GAction, not widget */
  char* action_name = (char*)iupAttribGet(ih, "_IUPGTK4_ACTION_NAME");

  if (action_name && iupAttribGet(ih, "_IUPGTK4_CHECKABLE"))
  {
    Ihandle* dialog = IupGetDialog(ih);
    GSimpleActionGroup* action_group = (GSimpleActionGroup*)iupAttribGet(dialog, "_IUPGTK4_MENU_ACTION_GROUP");


    if (action_group)
    {
      GAction* action = g_action_map_lookup_action(G_ACTION_MAP(action_group), action_name);
      if (action)
      {
        gboolean active = iupStrBoolean(value);
        g_simple_action_set_state(G_SIMPLE_ACTION(action), g_variant_new_boolean(active));
        /* Return 1 to store in hash table - needed for GetValue to work */
        return 1;
      }
    }
    /* Menu not built yet - return 1 to store in hash table for later use during menu building */
    return 1;
  }
  else if (iupAttribGet(ih, "_IUPGTK4_IMAGE_ITEM"))
  {
    gtk4ItemUpdateImage(ih, value, iupAttribGet(ih, "IMAGE"), iupAttribGet(ih, "IMPRESS"));
  }

  /* Always return 1 to store VALUE in hash table - needed for menu building to read initial state */
  return 1;
}

static char* gtk4ItemGetValueAttrib(Ihandle* ih)
{
  /* GMenu-based system: check state is stored in GAction, not widget */
  char* action_name = (char*)iupAttribGet(ih, "_IUPGTK4_ACTION_NAME");

  if (action_name && iupAttribGet(ih, "_IUPGTK4_CHECKABLE"))
  {
    Ihandle* dialog = IupGetDialog(ih);
    GSimpleActionGroup* action_group = (GSimpleActionGroup*)iupAttribGet(dialog, "_IUPGTK4_MENU_ACTION_GROUP");

    if (action_group)
    {
      GAction* action = g_action_map_lookup_action(G_ACTION_MAP(action_group), action_name);
      if (action)
      {
        GVariant* state = g_action_get_state(action);
        if (state)
        {
          gboolean active = g_variant_get_boolean(state);
          g_variant_unref(state);
          return iupStrReturnChecked(active);
        }
      }
    }
  }

  return NULL;
}

static int gtk4ItemMapMethod(Ihandle* ih)
{
  /* Native menu system handles items in gtk4BuildMenuModel() */
  /* Items don't need native widgets - they're represented in GMenuModel */
  (void)ih;
  return IUP_NOERROR;
}

void iupdrvItemInitClass(Iclass* ic)
{
  ic->Map = gtk4ItemMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", gtk4ItemGetValueAttrib, gtk4ItemSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtk4ItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, gtk4ItemSetTitleImageAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtk4ItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, gtk4ItemSetImpressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HIDEMARK", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}

static int gtk4SubmenuSetImageAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGet(ih, "_IUPGTK4_IMAGE_ITEM"))
  {
    gtk4ItemUpdateImage(ih, NULL, value, NULL);
    return 1;
  }
  return 0;
}

static int gtk4SubmenuMapMethod(Ihandle* ih)
{
  /* Native menu system handles submenus in gtk4BuildMenuModel() */
  (void)ih;
  return IUP_NOERROR;
}

void iupdrvSubmenuInitClass(Iclass* ic)
{
  ic->Map = gtk4SubmenuMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TITLE", NULL, gtk4ItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, gtk4SubmenuSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}

static int gtk4SeparatorMapMethod(Ihandle* ih)
{
  /* Native menu system handles separators in gtk4BuildMenuModel() */
  (void)ih;
  return IUP_NOERROR;
}

void iupdrvSeparatorInitClass(Iclass* ic)
{
  ic->Map = gtk4SeparatorMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
}
