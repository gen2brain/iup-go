/** \file
 * \brief Menu Resources
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_menu.h"

#include "iupgtk4_drv.h"
#include "iupgtk4_x11.h"

#ifdef GDK_WINDOWING_WIN32
#include <gdk/win32/gdkwin32.h>
#endif

#ifdef GDK_WINDOWING_MACOS
#include <gdk/macos/gdkmacos.h>
#endif

typedef struct _ImenuPos
{
  int x, y;
  Ihandle* ih;
} ImenuPos;

static int gtk4IsWin32Backend(void)
{
#ifdef GDK_WINDOWING_WIN32
  GdkDisplay *display = gdk_display_get_default();
  return (display && GDK_IS_WIN32_DISPLAY(display));
#else
  return 0;
#endif
}

static int gtk4IsMacosBackend(void)
{
#ifdef GDK_WINDOWING_MACOS
  GdkDisplay *display = gdk_display_get_default();
  return (display && GDK_IS_MACOS_DISPLAY(display));
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

static void gtk4MenuRadioActivated(GSimpleAction* action, GVariant* parameter, gpointer user_data)
{
  Ihandle* parent_menu = (Ihandle*)user_data;
  const char* target;
  Ihandle* picked = NULL;
  Ihandle* child;
  Icallback cb;

  if (!parameter || !parent_menu)
    return;

  g_simple_action_set_state(action, parameter);

  target = g_variant_get_string(parameter, NULL);
  for (child = parent_menu->firstchild; child; child = child->brother)
  {
    if (!iupStrEqual(child->iclass->name, "menuitem"))
      continue;
    const char* item_target = iupAttribGet(child, "_IUPGTK4_RADIO_TARGET");
    if (item_target && strcmp(item_target, target) == 0)
    {
      picked = child;
      iupAttribSet(child, "VALUE", "ON");
    }
    else
      iupAttribSet(child, "VALUE", "OFF");
  }

  if (!picked)
    return;
  cb = IupGetCallback(picked, "ACTION");
  if (cb && cb(picked) == IUP_CLOSE)
    IupExitLoop();
}

/* Recursively build GMenu model from IUP menu hierarchy
 * is_root: TRUE if this is the root menu bar level (requires submenus only), FALSE if this is inside a submenu */
/* Convert IUP accelerator text ("Ctrl+N") to a GTK accel string ("<Control>n") for the menu "accel" attribute. */
static gboolean gtk4MenuBuildAccel(const char* text, char* buffer, size_t bufsize)
{
  guint key = 0;
  GdkModifierType mods = 0;
  const char* p = text;

  buffer[0] = 0;
  while (*p)
  {
    char token[48];
    const char* plus = strchr(p, '+');
    int len = plus ? (int)(plus - p) : (int)strlen(p);
    if (len <= 0 || len >= (int)sizeof(token) || strlen(buffer) + sizeof(token) >= (size_t)bufsize)
      return FALSE;
    memcpy(token, p, len);
    token[len] = 0;

    if (plus)
    {
      if (iupStrEqualNoCase(token, "Ctrl") || iupStrEqualNoCase(token, "Control"))
        strcat(buffer, "<Control>");
      else if (iupStrEqualNoCase(token, "Shift"))
        strcat(buffer, "<Shift>");
      else if (iupStrEqualNoCase(token, "Alt"))
        strcat(buffer, "<Alt>");
      else if (iupStrEqualNoCase(token, "Meta") || iupStrEqualNoCase(token, "Super") || iupStrEqualNoCase(token, "Cmd"))
        strcat(buffer, "<Super>");
      else
        return FALSE;
      p = plus + 1;
    }
    else
    {
      if (len == 1 && token[0] >= 'A' && token[0] <= 'Z')
        token[0] = (char)(token[0] + 32);
      strcat(buffer, token);
      break;
    }
  }

  gtk_accelerator_parse(buffer, &key, &mods);
  return key != 0;
}

static Ihandle* gtk4MenuGetRootMenu(Ihandle* ih)
{
  while (ih->parent && ih->parent->iclass->nativetype == IUP_TYPEMENU)
    ih = ih->parent;
  return ih;
}

static GSimpleActionGroup* gtk4MenuGetActionGroup(Ihandle* menu)
{
  Ihandle* root = gtk4MenuGetRootMenu(menu);
  if (iupMenuIsMenuBar(root))
    return (GSimpleActionGroup*)iupAttribGet(root->parent, "_IUPGTK4_MENU_ACTION_GROUP");
  return (GSimpleActionGroup*)iupAttribGet(root, "_IUPGTK4_ACTION_GROUP");
}

static GMenu* gtk4MenuFindEntryPos(Ihandle* menu, Ihandle* child, int *pos)
{
  GMenu* section = (GMenu*)iupAttribGet(menu, "_IUPGTK4_SECTION0");
  Ihandle* c;

  *pos = 0;
  for (c = menu->firstchild; c && c != child; c = c->brother)
  {
    GMenu* sep_section = (GMenu*)iupAttribGet(c, "_IUPGTK4_SECTION");
    if (sep_section)
    {
      section = sep_section;
      *pos = 0;
    }
    else if (iupAttribGet(c, "_IUPGTK4_ENTRY"))
      (*pos)++;
  }
  return section;
}

static int gtk4MenuSectionIndex(GMenuModel* outer, GMenu* section)
{
  int i, n = g_menu_model_get_n_items(outer);
  for (i = 0; i < n; i++)
  {
    GMenuModel* link = g_menu_model_get_item_link(outer, i, G_MENU_LINK_SECTION);
    if (link) g_object_unref(link);
    if (link == (GMenuModel*)section)
      return i;
  }
  return -1;
}

static void gtk4MenuChildInsert(Ihandle* ih, GMenuItem* mitem)
{
  int pos;
  GMenu* section = gtk4MenuFindEntryPos(ih->parent, ih, &pos);
  g_menu_insert_item(section, pos, mitem);
  iupAttribSet(ih, "_IUPGTK4_ENTRY", "1");
}

static void gtk4MenuChildRemove(Ihandle* ih)
{
  int pos;
  GMenu* section;

  if (!iupAttribGet(ih, "_IUPGTK4_ENTRY"))
    return;
  section = gtk4MenuFindEntryPos(ih->parent, ih, &pos);
  g_menu_remove(section, pos);
  iupAttribSet(ih, "_IUPGTK4_ENTRY", NULL);
}

static GMenuItem* gtk4MenuItemBuildEntry(Ihandle* menu, Ihandle* ih, GSimpleActionGroup* action_group, const char* new_title)
{
  GMenuItem* mitem;
  char* title;
  char* processed_title;
  char c = '_';

  title = (char*)new_title;
  if (!title) title = iupAttribGet(ih, "TITLE");
  if (!title) title = "";

  /* Process mnemonic: convert & to _ for GTK */
  processed_title = iupStrProcessMnemonic(title, &c, 1);

  if (iupAttribGetBoolean(menu, "RADIO"))
  {
    char target[32];
    char full_action_name[96];
    char* radio_action_name = iupAttribGet(menu, "_IUPGTK4_RADIO_ACTION_NAME");

    snprintf(target, sizeof(target), "%p", (void*)ih);

    if (!radio_action_name)
    {
      char name[64];
      GSimpleAction* radio_action;

      snprintf(name, sizeof(name), "radio-%p", (void*)menu);
      radio_action = g_simple_action_new_stateful(name, G_VARIANT_TYPE_STRING, g_variant_new_string(target));
      g_signal_connect(radio_action, "activate", G_CALLBACK(gtk4MenuRadioActivated), menu);
      g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(radio_action));
      g_object_unref(radio_action);

      iupAttribSetStr(menu, "_IUPGTK4_RADIO_ACTION_NAME", name);
      radio_action_name = iupAttribGet(menu, "_IUPGTK4_RADIO_ACTION_NAME");
      iupAttribSet(ih, "VALUE", "ON");
    }
    else if (iupAttribGetBoolean(ih, "VALUE"))
    {
      GAction* action = g_action_map_lookup_action(G_ACTION_MAP(action_group), radio_action_name);
      if (action)
        g_simple_action_set_state(G_SIMPLE_ACTION(action), g_variant_new_string(target));
    }
    else
      iupAttribSet(ih, "VALUE", "OFF");

    iupAttribSetStr(ih, "_IUPGTK4_RADIO_TARGET", target);
    iupAttribSetStr(ih, "_IUPGTK4_RADIO_ACTION_NAME", radio_action_name);

    snprintf(full_action_name, sizeof(full_action_name), "menu.%s", radio_action_name);

    mitem = g_menu_item_new(processed_title, NULL);
    g_menu_item_set_action_and_target(mitem, full_action_name, "s", iupAttribGet(ih, "_IUPGTK4_RADIO_TARGET"));

    if (processed_title != title)
      free(processed_title);
    return mitem;
  }

  {
    GAction* existing;
    char action_name[64];
    char full_action_name[96];
    char accel_buf[128];
    gboolean has_accel = FALSE;
    char* tab;
    char* label_str = processed_title;
    char* label_copy = NULL;
    int is_checkable = 0;

    snprintf(action_name, sizeof(action_name), "item-%p", (void*)ih);

    /* Check if this is a checkable item (VALUE attribute present or HIDEMARK not set) */
    {
      char* value_str = iupAttribGetStr(ih, "VALUE");
      if (value_str || !iupAttribGetBoolean(ih, "HIDEMARK"))
        is_checkable = 1;

      existing = g_action_map_lookup_action(G_ACTION_MAP(action_group), action_name);
      if (!existing)
      {
        GSimpleAction* action;
        int is_active = 1;

        if (is_checkable)
        {
          action = g_simple_action_new_stateful(action_name, NULL, g_variant_new_boolean(iupStrBoolean(value_str)));
          iupAttribSetStr(ih, "_IUPGTK4_CHECKABLE", "1");
        }
        else
          action = g_simple_action_new(action_name, NULL);

        iupAttribSetStr(ih, "_IUPGTK4_ACTION_NAME", action_name);

        if (iupAttribGet(ih, "ACTIVE"))
          is_active = iupAttribGetBoolean(ih, "ACTIVE");

        g_signal_connect(action, "activate", G_CALLBACK(gtk4MenuActionActivated), ih);
        g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
        g_simple_action_set_enabled(action, is_active);
        g_object_unref(action);
      }
    }

    snprintf(full_action_name, sizeof(full_action_name), "menu.%s", action_name);

    tab = strchr(processed_title, '\t');
    if (tab)
    {
      has_accel = gtk4MenuBuildAccel(tab + 1, accel_buf, sizeof(accel_buf));
      if (has_accel)
      {
        label_copy = iupStrDup(processed_title);
        label_copy[tab - processed_title] = 0;
        label_str = label_copy;
      }
    }

    if (iupMenuIsMenuBar(menu))
    {
      /* GtkPopoverMenuBar requires submenus at root level; wrap the action in a single-item submenu. */
      GMenu* item_submenu = g_menu_new();
      g_menu_append(item_submenu, NULL, full_action_name);
      mitem = g_menu_item_new(label_str, NULL);
      g_menu_item_set_submenu(mitem, G_MENU_MODEL(item_submenu));
      g_object_unref(item_submenu);
    }
    else
    {
      mitem = g_menu_item_new(label_str, full_action_name);
      if (has_accel)
        g_menu_item_set_attribute(mitem, "accel", "s", accel_buf);
    }

    if (label_copy) free(label_copy);
    if (processed_title != title)
      free(processed_title);
    return mitem;
  }
}

IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih_menu)
{
  GtkWidget* menubar_widget;

  if (!ih_menu || !ih_menu->handle)
    return 0;

  menubar_widget = (GtkWidget*)ih_menu->handle;

  int height = gtk_widget_get_height(menubar_widget);

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

static void gtk4MenuParentDestroyCb(GtkWidget* parent, gpointer user_data)
{
  Ihandle* ih = (Ihandle*)user_data;
  GtkWidget* popover = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_POPOVER");

  if (popover && GTK_IS_WIDGET(popover))
  {
    gtk_widget_unparent(popover);
    iupAttribSet(ih, "_IUPGTK4_POPOVER", NULL);
  }

  iupAttribSet(ih, "_IUPGTK4_POPOVER_PARENT", NULL);
}

static void gtk4AnchorPopoverClosedCb(GtkPopover *popover, gpointer user_data)
{
  Ihandle *ih = (Ihandle*)user_data;
  GtkWidget *anchor_window;

  /* Hide anchor window when popover closes */
  anchor_window = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_ANCHOR_WINDOW");
  if (anchor_window)
    gtk_widget_set_visible(anchor_window, FALSE);

  (void)popover;
}

static void gtk4PopoverMenuSetVHomogeneous(GtkWidget* popover)
{
  GtkWidget* sw = gtk_popover_get_child(GTK_POPOVER(popover));
  if (sw && GTK_IS_SCROLLED_WINDOW(sw))
  {
    GtkWidget* vp = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(sw));
    if (vp && GTK_IS_VIEWPORT(vp))
    {
      GtkWidget* stack = gtk_viewport_get_child(GTK_VIEWPORT(vp));
      if (stack && GTK_IS_STACK(stack))
        gtk_stack_set_vhomogeneous(GTK_STACK(stack), TRUE);
    }
  }
}

IUP_SDK_API int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
  GtkWidget* popover;
  GtkWidget* parent_widget = NULL;
  GMenu* menu_model;
  GSimpleActionGroup* action_group;
  GMainLoop* loop;
  int local_x = 0, local_y = 0;
  int use_anchor_window = 0;

  /* Get stored menu model and action group from MapMethod */
  menu_model = (GMenu*)iupAttribGet(ih, "_IUPGTK4_MENU_MODEL");
  action_group = (GSimpleActionGroup*)iupAttribGet(ih, "_IUPGTK4_ACTION_GROUP");

  if (!menu_model || !action_group)
    return IUP_ERROR;

  /* Try to find an active visible window */
  {
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

  /* If no parent_widget (no visible window), use anchor window */
  if (!parent_widget)
  {
    use_anchor_window = 1;
  }

  if (use_anchor_window)
  {
    /* Use positioned anchor window for free positioning at (x,y) */
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

      /* Hide anchor window from taskbar on all platforms */
      {
        GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(anchor_window));
        if (surface)
        {
#ifdef GDK_WINDOWING_X11
          if (iupgtk4X11IsBackend())
            iupgtk4X11HideFromTaskbar(surface);
#endif
#ifdef GDK_WINDOWING_WIN32
          if (gtk4IsWin32Backend() && GDK_IS_WIN32_SURFACE(surface))
          {
            HWND hwnd = gdk_win32_surface_get_handle(surface);
            if (hwnd)
              iupgtk4Win32HideFromTaskbar(hwnd);
          }
#endif
#ifdef GDK_WINDOWING_MACOS
          if (gtk4IsMacosBackend() && GDK_IS_MACOS_SURFACE(surface))
          {
            gpointer nswindow = gdk_macos_surface_get_native_window(GDK_MACOS_SURFACE(surface));
            if (nswindow)
              iupgtk4MacosHideFromTaskbar(nswindow);
          }
#endif
        }
      }
    }
    else
    {
      gtk_widget_set_visible(anchor_window, TRUE);
      gtk_window_present(GTK_WINDOW(anchor_window));
    }

    /* Position the anchor window at (x,y) on platforms that support it */
    {
      GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(anchor_window));

      if (surface)
      {
#ifdef GDK_WINDOWING_X11
        if (iupgtk4X11IsBackend())
          iupgtk4X11MoveWindow(surface, x, y);
#endif
#ifdef GDK_WINDOWING_WIN32
        if (gtk4IsWin32Backend() && GDK_IS_WIN32_SURFACE(surface))
        {
          HWND hwnd = gdk_win32_surface_get_handle(surface);
          if (hwnd)
            iupgtk4Win32MoveWindow(hwnd, x, y);
        }
#endif
#ifdef GDK_WINDOWING_MACOS
        if (gtk4IsMacosBackend() && GDK_IS_MACOS_SURFACE(surface))
        {
          gpointer nswindow = gdk_macos_surface_get_native_window(GDK_MACOS_SURFACE(surface));
          if (nswindow)
            iupgtk4MacosMoveWindow(nswindow, x, y);
        }
#endif
      }
    }

    /* Create popover for anchor window approach */
    popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu_model));
    if (!popover)
      return IUP_ERROR;

    gtk4PopoverMenuSetVHomogeneous(popover);

    /* Insert action group into anchor window */
    gtk_widget_insert_action_group(anchor_window, "menu", G_ACTION_GROUP(action_group));

    gtk_widget_set_parent(popover, anchor_window);
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);
    gtk_widget_set_halign(popover, GTK_ALIGN_START);

    GdkRectangle pointing_rect = {0, 0, 1, 1};
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &pointing_rect);
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);

    /* Store popover for cleanup */
    iupAttribSet(ih, "_IUPGTK4_POPOVER", (char*)popover);
    ih->handle = popover;

    /* Connect closed signal to hide anchor window */
    g_signal_connect(popover, "closed", G_CALLBACK(gtk4AnchorPopoverClosedCb), (gpointer)ih);

    /* Flush pending idles so GTK's separator sync runs before sizing */
    while (g_main_context_pending(NULL))
      g_main_context_iteration(NULL, FALSE);

    gtk_popover_popup(GTK_POPOVER(popover));

    return IUP_NOERROR;
  }

  /* Dialog popup path (tree, etc.): Convert screen coordinates to parent_widget-local coordinates */
  {
    GtkNative *native = gtk_widget_get_native(parent_widget);
    if (native)
    {
      double native_x, native_y;
      int win_x = 0, win_y = 0;
      gtk_native_get_surface_transform(native, &native_x, &native_y);

#ifdef GDK_WINDOWING_X11
      if (iupgtk4X11IsBackend())
      {
        GdkSurface* surface = gtk_native_get_surface(native);
        if (surface)
          iupgtk4X11GetWindowPosition(surface, &win_x, &win_y);
      }
#endif

      graphene_point_t point_in = {x - win_x - native_x, y - win_y - native_y};
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
      GtkWidget* old_parent = (GtkWidget*)iupAttribGet(ih, "_IUPGTK4_POPOVER_PARENT");
      if (old_parent && GTK_IS_WIDGET(old_parent))
        g_signal_handlers_disconnect_by_func(old_parent, gtk4MenuParentDestroyCb, ih);

      GtkWidget* parent = gtk_widget_get_parent(old_popover);
      if (parent)
        gtk_widget_unparent(old_popover);
      iupAttribSet(ih, "_IUPGTK4_POPOVER", NULL);
    }
  }

  /* Create a nested main loop, this will block until the popover is closed.
     This is needed because IUP expects IupPopup() to be synchronous. */
  loop = g_main_loop_new(NULL, FALSE);

  /* Create a fresh popover each time */
  popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu_model));
  if (!popover)
  {
    g_main_loop_unref(loop);
    return IUP_ERROR;
  }

  gtk4PopoverMenuSetVHomogeneous(popover);

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

  /* Connect to parent's destroy signal to unparent popover before window is finalized */
  g_signal_connect(parent_widget, "destroy", G_CALLBACK(gtk4MenuParentDestroyCb), ih);

  gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);

  gtk_widget_set_halign(popover, GTK_ALIGN_START);

  {
    GdkRectangle pointing_rect = {local_x, local_y, 1, 1};
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &pointing_rect);
  }
  gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);

  /* Flush pending idles so GTK's separator sync runs before sizing */
  while (g_main_context_pending(NULL))
    g_main_context_iteration(NULL, FALSE);

  /* Show the popover */
  gtk_popover_popup(GTK_POPOVER(popover));

  /* Block here until popover is closed, the closed signal handler will quit the loop */
  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  return IUP_NOERROR;
}

static int gtk4MenuMapMethod(Ihandle* ih)
{
  GMenu* sec0;

  if (iupMenuIsMenuBar(ih))
  {
    Ihandle* dialog = ih->parent;
    GSimpleActionGroup* action_group;
    GMenu* outer;
    GtkWidget* menubar_widget;
    GtkWidget* inner_parent;
    GtkWidget* menu_box = (GtkWidget*)iupAttribGet(dialog, "_IUPGTK4_MENU_BOX");

    if (!dialog->handle || !menu_box)
      return IUP_ERROR;

    action_group = g_simple_action_group_new();
    iupAttribSet(dialog, "_IUPGTK4_MENU_ACTION_GROUP", (char*)action_group);
    gtk_widget_insert_action_group(dialog->handle, "menu", G_ACTION_GROUP(action_group));
    g_object_unref(action_group);  /* window takes ownership */

    outer = g_menu_new();
    sec0 = g_menu_new();
    g_menu_append_section(outer, NULL, G_MENU_MODEL(sec0));
    g_object_unref(sec0);

    /* Create GtkPopoverMenuBar widget from model */
    menubar_widget = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(outer));
    g_object_unref(outer);  /* menubar_widget takes ownership */

    /* Menu bar should not expand - it wants its natural height only */
    gtk_widget_set_vexpand(menubar_widget, FALSE);
    gtk_widget_set_hexpand(menubar_widget, TRUE);

    /* inner_parent must expand to fill remaining vertical space */
    inner_parent = (GtkWidget*)iupAttribGet(dialog, "_IUPGTK4_INNER_PARENT");
    gtk_widget_set_vexpand(inner_parent, TRUE);
    gtk_widget_set_hexpand(inner_parent, TRUE);

    /* Add menu bar to the top of the menu box (before inner_parent) */
    gtk_box_prepend(GTK_BOX(menu_box), menubar_widget);

    iupAttribSet(ih, "_IUPGTK4_GMENU", (char*)outer);
    iupAttribSet(ih, "_IUPGTK4_SECTION0", (char*)sec0);
    ih->handle = menubar_widget;
  }
  else if (ih->parent)
  {
    GMenu* outer = (GMenu*)iupAttribGet(ih->parent, "_IUPGTK4_SUBMENU_GMENU");
    if (!outer)
      return IUP_ERROR;

    sec0 = g_menu_new();
    g_menu_append_section(outer, NULL, G_MENU_MODEL(sec0));
    g_object_unref(sec0);

    iupAttribSet(ih, "_IUPGTK4_GMENU", (char*)outer);
    iupAttribSet(ih, "_IUPGTK4_SECTION0", (char*)sec0);
    iupAttribSet(ih, "_IUP_RECENT_GMENU", (char*)outer);
    ih->handle = (GtkWidget*)ih;
  }
  else
  {
    GSimpleActionGroup* action_group = g_simple_action_group_new();
    GMenu* outer = g_menu_new();

    sec0 = g_menu_new();
    g_menu_append_section(outer, NULL, G_MENU_MODEL(sec0));
    g_object_unref(sec0);

    iupAttribSet(ih, "_IUPGTK4_MENU_MODEL", (char*)outer);
    iupAttribSet(ih, "_IUPGTK4_ACTION_GROUP", (char*)action_group);
    iupAttribSet(ih, "_IUPGTK4_GMENU", (char*)outer);
    iupAttribSet(ih, "_IUPGTK4_SECTION0", (char*)sec0);
    ih->handle = (GtkWidget*)ih;  /* Placeholder, real handle set in iupdrvMenuPopup */
  }

  ih->serial = iupMenuGetChildId(ih);
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
    /* Disconnect destroy signal before removing action group */
    g_signal_handlers_disconnect_by_func(popover_parent, gtk4MenuParentDestroyCb, ih);
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

  /* Clean up anchor window if it was created */
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
  {
    Ihandle* dialog = ih->parent;
    if (dialog && dialog->handle && GTK_IS_WIDGET(ih->handle))
    {
      GtkWidget* menu_box = (GtkWidget*)iupAttribGet(dialog, "_IUPGTK4_MENU_BOX");
      if (menu_box)
        gtk_box_remove(GTK_BOX(menu_box), (GtkWidget*)ih->handle);
      gtk_widget_insert_action_group(dialog->handle, "menu", NULL);
      iupAttribSet(dialog, "_IUPGTK4_MENU_ACTION_GROUP", NULL);
    }
    ih->parent = NULL;
  }

  iupAttribSet(ih, "_IUPGTK4_GMENU", NULL);
  iupAttribSet(ih, "_IUPGTK4_SECTION0", NULL);

  /* For popup menus, handle is the popover which we just cleaned up. */
  ih->handle = NULL;
}

IUP_SDK_API void iupdrvMenuInitClass(Iclass* ic)
{
  ic->Map = gtk4MenuMapMethod;
  ic->UnMap = gtk4MenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
}

static int gtk4MenuItemSetValueAttrib(Ihandle* ih, const char* value)
{
  char* radio_action_name = (char*)iupAttribGet(ih, "_IUPGTK4_RADIO_ACTION_NAME");
  if (radio_action_name)
  {
    if (iupStrBoolean(value))
    {
      const char* target = iupAttribGet(ih, "_IUPGTK4_RADIO_TARGET");
      Ihandle* dialog = IupGetDialog(ih);
      GSimpleActionGroup* action_group = (GSimpleActionGroup*)iupAttribGet(dialog, "_IUPGTK4_MENU_ACTION_GROUP");
      if (target && action_group)
      {
        GAction* action = g_action_map_lookup_action(G_ACTION_MAP(action_group), radio_action_name);
        if (action)
          g_simple_action_set_state(G_SIMPLE_ACTION(action), g_variant_new_string(target));
      }
    }
    return 1;
  }

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

  /* Always return 1 to store VALUE in hash table - needed for menu building to read initial state */
  return 1;
}

static char* gtk4MenuItemGetValueAttrib(Ihandle* ih)
{
  char* radio_action_name = (char*)iupAttribGet(ih, "_IUPGTK4_RADIO_ACTION_NAME");
  if (radio_action_name)
  {
    const char* target = iupAttribGet(ih, "_IUPGTK4_RADIO_TARGET");
    Ihandle* dialog = IupGetDialog(ih);
    GSimpleActionGroup* action_group = (GSimpleActionGroup*)iupAttribGet(dialog, "_IUPGTK4_MENU_ACTION_GROUP");
    if (target && action_group)
    {
      GAction* action = g_action_map_lookup_action(G_ACTION_MAP(action_group), radio_action_name);
      if (action)
      {
        GVariant* state = g_action_get_state(action);
        if (state)
        {
          const char* current = g_variant_get_string(state, NULL);
          int match = (strcmp(current, target) == 0);
          g_variant_unref(state);
          return iupStrReturnChecked(match);
        }
      }
    }
    return NULL;
  }

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

static int gtk4MenuItemMapMethod(Ihandle* ih)
{
  Ihandle* menu = ih->parent;
  GSimpleActionGroup* action_group;
  GMenuItem* mitem;

  if (!menu || !iupAttribGet(menu, "_IUPGTK4_SECTION0"))
    return IUP_ERROR;

  action_group = gtk4MenuGetActionGroup(menu);
  if (!action_group)
    return IUP_ERROR;

  mitem = gtk4MenuItemBuildEntry(menu, ih, action_group, NULL);
  if (!mitem)
    return IUP_ERROR;

  gtk4MenuChildInsert(ih, mitem);
  g_object_unref(mitem);

  ih->serial = iupMenuGetChildId(ih);
  ih->handle = (GtkWidget*)ih;
  return IUP_NOERROR;
}

static void gtk4MenuItemUnMapMethod(Ihandle* ih)
{
  char* action_name = iupAttribGet(ih, "_IUPGTK4_ACTION_NAME");

  gtk4MenuChildRemove(ih);

  if (action_name)
  {
    GSimpleActionGroup* action_group = gtk4MenuGetActionGroup(ih->parent);
    if (action_group)
      g_action_map_remove_action(G_ACTION_MAP(action_group), action_name);
    iupAttribSet(ih, "_IUPGTK4_ACTION_NAME", NULL);
    iupAttribSet(ih, "_IUPGTK4_CHECKABLE", NULL);
  }

  ih->handle = NULL;
}

static int gtk4SubmenuMapMethod(Ihandle* ih)
{
  Ihandle* menu = ih->parent;
  GMenu* sub;
  GMenuItem* mitem;
  char* title;
  char* processed_title;
  char c = '_';

  if (!menu || !iupAttribGet(menu, "_IUPGTK4_SECTION0"))
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");
  if (!title) title = "";
  processed_title = iupStrProcessMnemonic(title, &c, 1);

  sub = g_menu_new();
  mitem = g_menu_item_new(processed_title, NULL);
  g_menu_item_set_submenu(mitem, G_MENU_MODEL(sub));

  gtk4MenuChildInsert(ih, mitem);
  g_object_unref(mitem);

  iupAttribSet(ih, "_IUPGTK4_SUBMENU_GMENU", (char*)sub);
  g_object_unref(sub);

  if (processed_title != title)
    free(processed_title);

  ih->serial = iupMenuGetChildId(ih);
  ih->handle = (GtkWidget*)ih;
  return IUP_NOERROR;
}

static void gtk4SubmenuUnMapMethod(Ihandle* ih)
{
  gtk4MenuChildRemove(ih);
  iupAttribSet(ih, "_IUPGTK4_SUBMENU_GMENU", NULL);
  ih->handle = NULL;
}

static int gtk4MenuSeparatorMapMethod(Ihandle* ih)
{
  Ihandle* menu = ih->parent;

  if (!menu || !iupAttribGet(menu, "_IUPGTK4_SECTION0"))
    return IUP_ERROR;

  if (!iupMenuIsMenuBar(menu))
  {
    GMenuModel* outer = (GMenuModel*)iupAttribGet(menu, "_IUPGTK4_GMENU");
    GMenu* section;
    GMenu* new_section;
    int pos, n, sec_index;

    section = gtk4MenuFindEntryPos(menu, ih, &pos);
    new_section = g_menu_new();

    n = g_menu_model_get_n_items(G_MENU_MODEL(section));
    while (n > pos)
    {
      GMenuItem* moved = g_menu_item_new_from_model(G_MENU_MODEL(section), pos);
      g_menu_append_item(new_section, moved);
      g_object_unref(moved);
      g_menu_remove(section, pos);
      n--;
    }

    sec_index = gtk4MenuSectionIndex(outer, section);
    g_menu_insert_section((GMenu*)outer, sec_index + 1, NULL, G_MENU_MODEL(new_section));
    g_object_unref(new_section);

    iupAttribSet(ih, "_IUPGTK4_SECTION", (char*)new_section);
  }

  ih->serial = iupMenuGetChildId(ih);
  ih->handle = (GtkWidget*)ih;
  return IUP_NOERROR;
}

static void gtk4MenuSeparatorUnMapMethod(Ihandle* ih)
{
  GMenu* section = (GMenu*)iupAttribGet(ih, "_IUPGTK4_SECTION");

  if (section)
  {
    Ihandle* menu = ih->parent;
    GMenuModel* outer = (GMenuModel*)iupAttribGet(menu, "_IUPGTK4_GMENU");
    GMenu* prev_section;
    int pos, n, sec_index;

    prev_section = gtk4MenuFindEntryPos(menu, ih, &pos);

    n = g_menu_model_get_n_items(G_MENU_MODEL(section));
    while (n > 0)
    {
      GMenuItem* moved = g_menu_item_new_from_model(G_MENU_MODEL(section), 0);
      g_menu_append_item(prev_section, moved);
      g_object_unref(moved);
      g_menu_remove(section, 0);
      n--;
    }

    sec_index = gtk4MenuSectionIndex(outer, section);
    if (sec_index >= 0)
      g_menu_remove((GMenu*)outer, sec_index);

    iupAttribSet(ih, "_IUPGTK4_SECTION", NULL);
  }

  ih->handle = NULL;
}

static int gtk4MenuItemSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGet(ih, "_IUPGTK4_ENTRY"))
  {
    Ihandle* menu = ih->parent;
    GSimpleActionGroup* action_group = gtk4MenuGetActionGroup(menu);
    GMenu* section;
    GMenuItem* mitem;
    int pos;

    if (!action_group)
      return 1;

    section = gtk4MenuFindEntryPos(menu, ih, &pos);
    mitem = gtk4MenuItemBuildEntry(menu, ih, action_group, value);
    if (mitem)
    {
      g_menu_remove(section, pos);
      g_menu_insert_item(section, pos, mitem);
      g_object_unref(mitem);
    }
  }
  return 1;
}

static int gtk4SubmenuSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGet(ih, "_IUPGTK4_ENTRY"))
  {
    Ihandle* menu = ih->parent;
    GMenu* sub = (GMenu*)iupAttribGet(ih, "_IUPGTK4_SUBMENU_GMENU");
    GMenu* section;
    GMenuItem* mitem;
    char* title;
    char* processed_title;
    char c = '_';
    int pos;

    if (!sub)
      return 1;

    title = (char*)value;
    if (!title) title = "";
    processed_title = iupStrProcessMnemonic(title, &c, 1);

    section = gtk4MenuFindEntryPos(menu, ih, &pos);

    g_object_ref(sub);
    g_menu_remove(section, pos);

    mitem = g_menu_item_new(processed_title, NULL);
    g_menu_item_set_submenu(mitem, G_MENU_MODEL(sub));
    g_menu_insert_item(section, pos, mitem);
    g_object_unref(mitem);
    g_object_unref(sub);

    if (processed_title != title)
      free(processed_title);
  }
  return 1;
}

static int gtk4MenuItemSetActiveAttrib(Ihandle* ih, const char* value)
{
  char* action_name = iupAttribGet(ih, "_IUPGTK4_ACTION_NAME");
  if (action_name)
  {
    GSimpleActionGroup* action_group = gtk4MenuGetActionGroup(ih->parent);
    if (action_group)
    {
      GAction* action = g_action_map_lookup_action(G_ACTION_MAP(action_group), action_name);
      if (action)
        g_simple_action_set_enabled(G_SIMPLE_ACTION(action), iupStrBoolean(value));
    }
  }
  return 1;
}

/*******************************************************************************************/

static void gtk4RecentActionActivated(GSimpleAction* action, GVariant* parameter, gpointer user_data)
{
  Ihandle* menu = (Ihandle*)user_data;
  Icallback recent_cb;
  Ihandle* config;
  const char* action_name;
  int index;

  if (!menu)
    return;

  recent_cb = (Icallback)iupAttribGet(menu, "_IUP_RECENT_CB");
  config = (Ihandle*)iupAttribGet(menu, "_IUP_CONFIG");

  if (!recent_cb || !config)
    return;

  action_name = g_action_get_name(G_ACTION(action));
  if (sscanf(action_name, "recent-%d", &index) == 1)
  {
    char attr_name[32];
    const char* filename;

    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", index);
    filename = iupAttribGet(menu, attr_name);

    if (filename)
    {
      IupSetStrAttribute(config, "RECENTFILENAME", filename);
      IupSetStrAttribute(config, "TITLE", filename);
      config->parent = menu;

      recent_cb(config);

      config->parent = NULL;
      IupSetAttribute(config, "RECENTFILENAME", NULL);
      IupSetAttribute(config, "TITLE", NULL);
    }
  }

  (void)parameter;
}

IUP_SDK_API int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
  iupAttribSetInt(menu, "_IUP_RECENT_MAX", max_recent);
  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);
  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
  return 0;
}

IUP_SDK_API int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
  GSimpleActionGroup* action_group;
  GMenu* recent_menu;
  Ihandle* dialog;
  int max_recent, i, existing;

  if (!menu)
    return -1;

  max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
  existing = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");

  if (count > max_recent)
    count = max_recent;

  iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

  for (i = 0; i < count; i++)
  {
    char attr_name[32];
    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", i);
    iupAttribSetStr(menu, attr_name, filenames[i]);
  }

  for (; i < existing; i++)
  {
    char attr_name[32];
    snprintf(attr_name, sizeof(attr_name), "_IUP_RECENT_FILE%d", i);
    iupAttribSet(menu, attr_name, NULL);
  }

  iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);

  recent_menu = (GMenu*)iupAttribGet(menu, "_IUP_RECENT_GMENU");
  if (!recent_menu)
    return 0;

  /* Find the root menu by traversing up the parent chain */
  {
    Ihandle* root_menu = menu;
    while (root_menu->parent)
      root_menu = root_menu->parent;
    dialog = IupGetDialog(root_menu);
  }

  if (!dialog)
    return -1;

  action_group = (GSimpleActionGroup*)iupAttribGet(dialog, "_IUPGTK4_MENU_ACTION_GROUP");
  if (!action_group)
    return -1;

  for (i = 0; i < count; i++)
  {
    char action_name[32];
    GAction* existing_action;

    snprintf(action_name, sizeof(action_name), "recent-%d", i);

    existing_action = g_action_map_lookup_action(G_ACTION_MAP(action_group), action_name);
    if (!existing_action)
    {
      GSimpleAction* action = g_simple_action_new(action_name, NULL);
      g_signal_connect(action, "activate", G_CALLBACK(gtk4RecentActionActivated), menu);
      g_action_map_add_action(G_ACTION_MAP(action_group), G_ACTION(action));
      g_object_unref(action);
    }
  }

  g_menu_remove_all(recent_menu);

  for (i = 0; i < count; i++)
  {
    char action_name[64];
    snprintf(action_name, sizeof(action_name), "menu.recent-%d", i);
    g_menu_append(recent_menu, filenames[i], action_name);
  }

  return 0;
}

/*******************************************************************************************/

IUP_SDK_API void iupdrvMenuItemInitClass(Iclass* ic)
{
  ic->Map = gtk4MenuItemMapMethod;
  ic->UnMap = gtk4MenuItemUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", NULL, gtk4MenuItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "VALUE", gtk4MenuItemGetValueAttrib, gtk4MenuItemSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TITLE", NULL, gtk4MenuItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "HIDEMARK", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}

IUP_SDK_API void iupdrvSubmenuInitClass(Iclass* ic)
{
  ic->Map = gtk4SubmenuMapMethod;
  ic->UnMap = gtk4SubmenuUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TITLE", NULL, gtk4SubmenuSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
}

IUP_SDK_API void iupdrvMenuSeparatorInitClass(Iclass* ic)
{
  ic->Map = gtk4MenuSeparatorMapMethod;
  ic->UnMap = gtk4MenuSeparatorUnMapMethod;
}
