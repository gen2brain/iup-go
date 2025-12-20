/** \file
 * \brief Unix SNI (StatusNotifierItem) System Tray - Shared Implementation
 *
 * This file provides a unified SNI implementation with dbusmenu protocol for Unix drivers (GTK2, GTK3, GTK4, Motif).
 *
 * This module should depend only on IUP core headers and UNIX system headers.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef IUPDBUS_USE_DLOPEN
#include "iupunix_dbus_dlopen.h"
#else
#include <dbus/dbus.h>
#endif

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_childtree.h"
#include "iup_tray.h"

#define SNI_WATCHER_BUS_NAME "org.kde.StatusNotifierWatcher"
#define SNI_WATCHER_OBJECT_PATH "/StatusNotifierWatcher"
#define SNI_WATCHER_INTERFACE "org.kde.StatusNotifierWatcher"

#define SNI_ITEM_INTERFACE "org.kde.StatusNotifierItem"
#define SNI_ITEM_OBJECT_PATH "/StatusNotifierItem"

#define DBUSMENU_INTERFACE "com.canonical.dbusmenu"
#define DBUSMENU_OBJECT_PATH "/MenuBar"

static const char sni_introspection_xml[] =
  "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
  "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
  "<node>\n"
  "  <interface name=\"org.kde.StatusNotifierItem\">\n"
  "    <property name=\"Category\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"Id\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"Title\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"Status\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"WindowId\" type=\"i\" access=\"read\"/>\n"
  "    <property name=\"IconName\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"IconPixmap\" type=\"a(iiay)\" access=\"read\"/>\n"
  "    <property name=\"OverlayIconName\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"OverlayIconPixmap\" type=\"a(iiay)\" access=\"read\"/>\n"
  "    <property name=\"AttentionIconName\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"AttentionIconPixmap\" type=\"a(iiay)\" access=\"read\"/>\n"
  "    <property name=\"AttentionMovieName\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"ToolTip\" type=\"(sa(iiay)ss)\" access=\"read\"/>\n"
  "    <property name=\"ItemIsMenu\" type=\"b\" access=\"read\"/>\n"
  "    <property name=\"Menu\" type=\"o\" access=\"read\"/>\n"
  "    <method name=\"Activate\">\n"
  "      <arg name=\"x\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"y\" type=\"i\" direction=\"in\"/>\n"
  "    </method>\n"
  "    <method name=\"SecondaryActivate\">\n"
  "      <arg name=\"x\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"y\" type=\"i\" direction=\"in\"/>\n"
  "    </method>\n"
  "    <method name=\"ContextMenu\">\n"
  "      <arg name=\"x\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"y\" type=\"i\" direction=\"in\"/>\n"
  "    </method>\n"
  "    <method name=\"Scroll\">\n"
  "      <arg name=\"delta\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"orientation\" type=\"s\" direction=\"in\"/>\n"
  "    </method>\n"
  "    <signal name=\"NewTitle\"/>\n"
  "    <signal name=\"NewIcon\"/>\n"
  "    <signal name=\"NewAttentionIcon\"/>\n"
  "    <signal name=\"NewOverlayIcon\"/>\n"
  "    <signal name=\"NewToolTip\"/>\n"
  "    <signal name=\"NewStatus\">\n"
  "      <arg name=\"status\" type=\"s\"/>\n"
  "    </signal>\n"
  "  </interface>\n"
  "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
  "    <method name=\"Introspect\">\n"
  "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
  "    </method>\n"
  "  </interface>\n"
  "  <interface name=\"org.freedesktop.DBus.Properties\">\n"
  "    <method name=\"Get\">\n"
  "      <arg name=\"interface\" direction=\"in\" type=\"s\"/>\n"
  "      <arg name=\"property\" direction=\"out\" type=\"s\"/>\n"
  "      <arg name=\"value\" direction=\"out\" type=\"v\"/>\n"
  "    </method>\n"
  "    <method name=\"GetAll\">\n"
  "      <arg name=\"interface\" direction=\"in\" type=\"s\"/>\n"
  "      <arg name=\"properties\" direction=\"out\" type=\"a{sv}\"/>\n"
  "    </method>\n"
  "  </interface>\n"
  "</node>\n";

static const char dbusmenu_introspection_xml[] =
  "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
  "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
  "<node>\n"
  "  <interface name=\"com.canonical.dbusmenu\">\n"
  "    <property name=\"Version\" type=\"u\" access=\"read\"/>\n"
  "    <property name=\"TextDirection\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"Status\" type=\"s\" access=\"read\"/>\n"
  "    <property name=\"IconThemePath\" type=\"as\" access=\"read\"/>\n"
  "    <method name=\"GetLayout\">\n"
  "      <arg name=\"parentId\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"recursionDepth\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"propertyNames\" type=\"as\" direction=\"in\"/>\n"
  "      <arg name=\"revision\" type=\"u\" direction=\"out\"/>\n"
  "      <arg name=\"layout\" type=\"(ia{sv}av)\" direction=\"out\"/>\n"
  "    </method>\n"
  "    <method name=\"GetGroupProperties\">\n"
  "      <arg name=\"ids\" type=\"ai\" direction=\"in\"/>\n"
  "      <arg name=\"propertyNames\" type=\"as\" direction=\"in\"/>\n"
  "      <arg name=\"properties\" type=\"a(ia{sv})\" direction=\"out\"/>\n"
  "    </method>\n"
  "    <method name=\"GetProperty\">\n"
  "      <arg name=\"id\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"name\" type=\"s\" direction=\"in\"/>\n"
  "      <arg name=\"value\" type=\"v\" direction=\"out\"/>\n"
  "    </method>\n"
  "    <method name=\"Event\">\n"
  "      <arg name=\"id\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"eventId\" type=\"s\" direction=\"in\"/>\n"
  "      <arg name=\"data\" type=\"v\" direction=\"in\"/>\n"
  "      <arg name=\"timestamp\" type=\"u\" direction=\"in\"/>\n"
  "    </method>\n"
  "    <method name=\"EventGroup\">\n"
  "      <arg name=\"events\" type=\"a(isvu)\" direction=\"in\"/>\n"
  "      <arg name=\"idErrors\" type=\"ai\" direction=\"out\"/>\n"
  "    </method>\n"
  "    <method name=\"AboutToShow\">\n"
  "      <arg name=\"id\" type=\"i\" direction=\"in\"/>\n"
  "      <arg name=\"needUpdate\" type=\"b\" direction=\"out\"/>\n"
  "    </method>\n"
  "    <method name=\"AboutToShowGroup\">\n"
  "      <arg name=\"ids\" type=\"ai\" direction=\"in\"/>\n"
  "      <arg name=\"updatesNeeded\" type=\"ai\" direction=\"out\"/>\n"
  "      <arg name=\"idErrors\" type=\"ai\" direction=\"out\"/>\n"
  "    </method>\n"
  "    <signal name=\"ItemsPropertiesUpdated\">\n"
  "      <arg name=\"updatedProps\" type=\"a(ia{sv})\"/>\n"
  "      <arg name=\"removedProps\" type=\"a(ias)\"/>\n"
  "    </signal>\n"
  "    <signal name=\"LayoutUpdated\">\n"
  "      <arg name=\"revision\" type=\"u\"/>\n"
  "      <arg name=\"parent\" type=\"i\"/>\n"
  "    </signal>\n"
  "    <signal name=\"ItemActivationRequested\">\n"
  "      <arg name=\"id\" type=\"i\"/>\n"
  "      <arg name=\"timestamp\" type=\"u\"/>\n"
  "    </signal>\n"
  "  </interface>\n"
  "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
  "    <method name=\"Introspect\">\n"
  "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
  "    </method>\n"
  "  </interface>\n"
  "  <interface name=\"org.freedesktop.DBus.Properties\">\n"
  "    <method name=\"Get\">\n"
  "      <arg name=\"interface\" direction=\"in\" type=\"s\"/>\n"
  "      <arg name=\"property\" direction=\"out\" type=\"s\"/>\n"
  "      <arg name=\"value\" direction=\"out\" type=\"v\"/>\n"
  "    </method>\n"
  "    <method name=\"GetAll\">\n"
  "      <arg name=\"interface\" direction=\"in\" type=\"s\"/>\n"
  "      <arg name=\"properties\" direction=\"out\" type=\"a{sv}\"/>\n"
  "    </method>\n"
  "  </interface>\n"
  "</node>\n";

/* Menu item tracking for dbusmenu */
#define MAX_MENU_ITEMS 256

typedef struct _IupUnixSNIMenuItem {
  int id;
  Ihandle* ih;
} IupUnixSNIMenuItem;

typedef struct _IupUnixSNI {
  DBusConnection* connection;
  char* bus_name;
  Ihandle* ih;

  /* Icon data */
  int icon_width;
  int icon_height;
  unsigned char* icon_pixels;

  /* State */
  char* tooltip;
  int visible;
  int registered;
  struct timeval last_click_time;
  int last_button;
  int cleanup_in_progress;
  int ref_count;

  /* Menu (dbusmenu) */
  Ihandle* menu_ih;
  int menu_revision;
  int next_menu_id;
  IupUnixSNIMenuItem menu_items[MAX_MENU_ITEMS];
  int menu_item_count;

  /* IUP Timer for dbus dispatch */
  Ihandle* timer;
} IupUnixSNI;

static int g_sni_initialized = 0;

/****************************************************************************
 * Double-click detection
 ****************************************************************************/

static int sniDoubleClick(IupUnixSNI* sni, int button)
{
  struct timeval now;
  long diff_ms;

  gettimeofday(&now, NULL);

  if (sni->last_button == -1 || sni->last_button != button)
  {
    sni->last_button = button;
    sni->last_click_time = now;
    return 0;
  }

  diff_ms = (now.tv_sec - sni->last_click_time.tv_sec) * 1000 +
            (now.tv_usec - sni->last_click_time.tv_usec) / 1000;

  if (diff_ms < 400)
  {
    sni->last_button = -1;
    return 1;
  }

  sni->last_click_time = now;
  return 0;
}

/****************************************************************************
 * Callback invocation
 ****************************************************************************/

static void sniInvokeCallback(IupUnixSNI* sni, int button, int pressed, int x, int y)
{
  int dclick = sniDoubleClick(sni, button);
  IFniii cb = (IFniii)IupGetCallback(sni->ih, "TRAYCLICK_CB");

  (void)x;
  (void)y;

  if (cb)
  {
    if (cb(sni->ih, button, pressed, dclick) == IUP_CLOSE)
      IupExitLoop();
  }
}

/****************************************************************************
 * Icon pixmap serialization
 ****************************************************************************/

static void sniAppendIconPixmap(DBusMessageIter* iter, IupUnixSNI* sni)
{
  DBusMessageIter array_iter, struct_iter, byte_array_iter;

  dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);

  if (sni->icon_pixels && sni->icon_width > 0 && sni->icon_height > 0)
  {
    int x, y;
    dbus_int32_t w = (dbus_int32_t)sni->icon_width;
    dbus_int32_t h = (dbus_int32_t)sni->icon_height;

    dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);
    dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &w);
    dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &h);

    dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "y", &byte_array_iter);

    /* Pixels are already in ARGB format from the driver */
    for (y = 0; y < sni->icon_height; y++)
    {
      for (x = 0; x < sni->icon_width; x++)
      {
        int offset = (y * sni->icon_width + x) * 4;
        unsigned char a = sni->icon_pixels[offset + 0];
        unsigned char r = sni->icon_pixels[offset + 1];
        unsigned char g = sni->icon_pixels[offset + 2];
        unsigned char b = sni->icon_pixels[offset + 3];

        dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &a);
        dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &r);
        dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &g);
        dbus_message_iter_append_basic(&byte_array_iter, DBUS_TYPE_BYTE, &b);
      }
    }

    dbus_message_iter_close_container(&struct_iter, &byte_array_iter);
    dbus_message_iter_close_container(&array_iter, &struct_iter);
  }

  dbus_message_iter_close_container(iter, &array_iter);
}

/****************************************************************************
 * Menu ID management
 ****************************************************************************/

static void sniClearMenuItems(IupUnixSNI* sni)
{
  sni->menu_item_count = 0;
  sni->next_menu_id = 1;
}

static int sniAddMenuItem(IupUnixSNI* sni, Ihandle* item)
{
  if (sni->menu_item_count >= MAX_MENU_ITEMS)
    return -1;

  int id = sni->next_menu_id++;
  sni->menu_items[sni->menu_item_count].id = id;
  sni->menu_items[sni->menu_item_count].ih = item;
  sni->menu_item_count++;
  return id;
}

static Ihandle* sniFindMenuItemById(IupUnixSNI* sni, int id)
{
  int i;
  for (i = 0; i < sni->menu_item_count; i++)
  {
    if (sni->menu_items[i].id == id)
      return sni->menu_items[i].ih;
  }
  return NULL;
}

/****************************************************************************
 * dbusmenu: Build menu layout
 ****************************************************************************/

static void sniAppendMenuItemProperties(DBusMessageIter* dict_iter, Ihandle* item, int is_submenu)
{
  DBusMessageIter dict_entry_iter, variant_iter;
  const char* title;
  char* class_name;

  if (!item)
    return;

  class_name = IupGetClassName(item);

  /* Check if separator */
  if (iupStrEqual(class_name, "separator"))
  {
    const char* prop_name = "type";
    const char* value = "separator";
    dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
    dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
    dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
    dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
    return;
  }

  /* Label - convert & to _ for accelerator */
  title = IupGetAttribute(item, "TITLE");
  if (title && title[0])
  {
    const char* prop_name = "label";
    char* label = iupStrDup(title);
    char* p = label;
    while (*p)
    {
      if (*p == '&')
        *p = '_';
      p++;
    }

    dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
    dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
    dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &label);
    dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
    dbus_message_iter_close_container(dict_iter, &dict_entry_iter);

    free(label);
  }

  /* Submenu indicator */
  if (is_submenu)
  {
    const char* prop_name = "children-display";
    const char* value = "submenu";
    dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
    dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
    dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
    dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
  }

  /* Enabled state */
  {
    const char* active = IupGetAttribute(item, "ACTIVE");
    if (active && iupStrEqualNoCase(active, "NO"))
    {
      const char* prop_name = "enabled";
      dbus_bool_t value = FALSE;
      dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "b", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
    }
  }

  /* Toggle state - only for checkable items */
  {
    const char* hidemark = iupAttribGetStr(item, "HIDEMARK");
    const char* value_attr = iupAttribGet(item, "VALUE");
    int is_checkable = 0;

    /* Item is checkable if:
       - VALUE was explicitly set, OR
       - HIDEMARK is explicitly set to NO */
    if (value_attr)
      is_checkable = 1;
    else if (hidemark && !iupStrBoolean(hidemark))
      is_checkable = 1;

    if (is_checkable)
    {
      /* Toggle type */
      {
        const char* prop_name = "toggle-type";
        const char* toggle_type = "checkmark";
        dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
        dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
        dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
        dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &toggle_type);
        dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
        dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
      }
      /* Toggle state */
      {
        const char* prop_name = "toggle-state";
        dbus_int32_t state = (value_attr && iupStrEqualNoCase(value_attr, "ON")) ? 1 : 0;
        dbus_message_iter_open_container(dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
        dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
        dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "i", &variant_iter);
        dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_INT32, &state);
        dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
        dbus_message_iter_close_container(dict_iter, &dict_entry_iter);
      }
    }
  }
}

static void sniAppendMenuLayout(DBusMessageIter* parent_iter, IupUnixSNI* sni, Ihandle* menu, int parent_id, int depth);

static void sniAppendMenuChildren(DBusMessageIter* children_iter, IupUnixSNI* sni, Ihandle* menu, int depth)
{
  Ihandle* child;

  if (!menu || depth <= 0)
    return;

  for (child = menu->firstchild; child; child = child->brother)
  {
    char* class_name = IupGetClassName(child);
    DBusMessageIter variant_iter, child_struct_iter;

    if (iupStrEqual(class_name, "submenu"))
    {
      /* Submenu: get the child menu */
      Ihandle* submenu = child->firstchild;
      int item_id = sniAddMenuItem(sni, child);
      if (item_id < 0) continue;

      dbus_message_iter_open_container(children_iter, DBUS_TYPE_VARIANT, "(ia{sv}av)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_STRUCT, NULL, &child_struct_iter);

      /* Item ID */
      dbus_message_iter_append_basic(&child_struct_iter, DBUS_TYPE_INT32, &item_id);

      /* Properties */
      {
        DBusMessageIter props_iter;
        dbus_message_iter_open_container(&child_struct_iter, DBUS_TYPE_ARRAY, "{sv}", &props_iter);
        sniAppendMenuItemProperties(&props_iter, child, 1);
        dbus_message_iter_close_container(&child_struct_iter, &props_iter);
      }

      /* Children (recursive) */
      {
        DBusMessageIter sub_children_iter;
        dbus_message_iter_open_container(&child_struct_iter, DBUS_TYPE_ARRAY, "v", &sub_children_iter);
        if (submenu && depth > 1)
          sniAppendMenuChildren(&sub_children_iter, sni, submenu, depth - 1);
        dbus_message_iter_close_container(&child_struct_iter, &sub_children_iter);
      }

      dbus_message_iter_close_container(&variant_iter, &child_struct_iter);
      dbus_message_iter_close_container(children_iter, &variant_iter);
    }
    else if (iupStrEqual(class_name, "item") || iupStrEqual(class_name, "separator"))
    {
      /* Regular item or separator */
      int item_id = sniAddMenuItem(sni, child);
      if (item_id < 0) continue;

      dbus_message_iter_open_container(children_iter, DBUS_TYPE_VARIANT, "(ia{sv}av)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_STRUCT, NULL, &child_struct_iter);

      /* Item ID */
      dbus_message_iter_append_basic(&child_struct_iter, DBUS_TYPE_INT32, &item_id);

      /* Properties */
      {
        DBusMessageIter props_iter;
        dbus_message_iter_open_container(&child_struct_iter, DBUS_TYPE_ARRAY, "{sv}", &props_iter);
        sniAppendMenuItemProperties(&props_iter, child, 0);
        dbus_message_iter_close_container(&child_struct_iter, &props_iter);
      }

      /* Empty children array */
      {
        DBusMessageIter sub_children_iter;
        dbus_message_iter_open_container(&child_struct_iter, DBUS_TYPE_ARRAY, "v", &sub_children_iter);
        dbus_message_iter_close_container(&child_struct_iter, &sub_children_iter);
      }

      dbus_message_iter_close_container(&variant_iter, &child_struct_iter);
      dbus_message_iter_close_container(children_iter, &variant_iter);
    }
  }
}

static void sniAppendMenuLayout(DBusMessageIter* parent_iter, IupUnixSNI* sni, Ihandle* menu, int parent_id, int depth)
{
  DBusMessageIter struct_iter, props_iter, children_iter;

  dbus_message_iter_open_container(parent_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

  /* Parent ID (0 for root) */
  dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_INT32, &parent_id);

  /* Properties (empty for root) */
  dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &props_iter);
  if (parent_id == 0 && menu)
  {
    /* Root menu might have children-display property */
    const char* prop_name = "children-display";
    const char* value = "submenu";
    DBusMessageIter dict_entry_iter, variant_iter;
    dbus_message_iter_open_container(&props_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
    dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
    dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
    dbus_message_iter_close_container(&props_iter, &dict_entry_iter);
  }
  dbus_message_iter_close_container(&struct_iter, &props_iter);

  /* Children */
  dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "v", &children_iter);
  if (menu && depth > 0)
    sniAppendMenuChildren(&children_iter, sni, menu, depth);
  dbus_message_iter_close_container(&struct_iter, &children_iter);

  dbus_message_iter_close_container(parent_iter, &struct_iter);
}

/****************************************************************************
 * dbusmenu message handler
 ****************************************************************************/

static DBusHandlerResult sniDBusMenuHandler(DBusConnection* connection, DBusMessage* message, void* user_data)
{
  IupUnixSNI* sni = (IupUnixSNI*)user_data;
  const char* interface = dbus_message_get_interface(message);
  const char* member = dbus_message_get_member(message);

  if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Introspectable", "Introspect"))
  {
    DBusMessage* reply = dbus_message_new_method_return(message);
    const char* xml_ptr = dbusmenu_introspection_xml;
    dbus_message_append_args(reply, DBUS_TYPE_STRING, &xml_ptr, DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (strcmp(interface, DBUSMENU_INTERFACE) != 0 &&
      strcmp(interface, "org.freedesktop.DBus.Properties") != 0)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (dbus_message_is_method_call(message, DBUSMENU_INTERFACE, "GetLayout"))
  {
    dbus_int32_t parent_id, recursion_depth;
    DBusMessage* reply;
    DBusMessageIter iter;
    dbus_uint32_t revision;

    dbus_message_get_args(message, NULL, DBUS_TYPE_INT32, &parent_id, DBUS_TYPE_INT32, &recursion_depth, DBUS_TYPE_INVALID);

    /* Rebuild menu item ID mapping */
    sniClearMenuItems(sni);

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &iter);

    revision = (dbus_uint32_t)sni->menu_revision;
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &revision);

    /* Layout structure */
    if (recursion_depth < 0)
      recursion_depth = 10;

    sniAppendMenuLayout(&iter, sni, sni->menu_ih, parent_id, recursion_depth);

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, DBUSMENU_INTERFACE, "Event"))
  {
    dbus_int32_t id;
    const char* event_id;
    dbus_uint32_t timestamp;
    DBusMessage* reply;

    dbus_message_get_args(message, NULL, DBUS_TYPE_INT32, &id, DBUS_TYPE_STRING, &event_id, DBUS_TYPE_INVALID);

    reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);

    if (strcmp(event_id, "clicked") == 0)
    {
      Ihandle* item = sniFindMenuItemById(sni, id);
      if (item)
      {
        /* First call TRAYCLICK_CB with button 3 */
        sniInvokeCallback(sni, 3, 1, 0, 0);

        /* Then trigger the item's action callback */
        IFn cb = (IFn)IupGetCallback(item, "ACTION");
        if (cb)
        {
          int ret = cb(item);
          if (ret == IUP_CLOSE)
            IupExitLoop();
        }
      }
    }

    (void)timestamp;
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, DBUSMENU_INTERFACE, "AboutToShow"))
  {
    dbus_int32_t id;
    DBusMessage* reply;
    dbus_bool_t need_update = FALSE;

    dbus_message_get_args(message, NULL, DBUS_TYPE_INT32, &id, DBUS_TYPE_INVALID);

    /* Call TRAYCLICK_CB before showing menu */
    if (id == 0)
      sniInvokeCallback(sni, 3, 1, 0, 0);

    reply = dbus_message_new_method_return(message);
    dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &need_update, DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, DBUSMENU_INTERFACE, "AboutToShowGroup"))
  {
    DBusMessage* reply;
    DBusMessageIter iter, array_iter;

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &iter);

    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "i", &array_iter);
    dbus_message_iter_close_container(&iter, &array_iter);

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, DBUSMENU_INTERFACE, "GetGroupProperties"))
  {
    DBusMessage* reply;
    DBusMessageIter iter, array_iter;

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &iter);

    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(ia{sv})", &array_iter);
    dbus_message_iter_close_container(&iter, &array_iter);

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, DBUSMENU_INTERFACE, "GetProperty"))
  {
    dbus_int32_t id;
    const char* prop_name;
    DBusMessage* reply;
    DBusMessageIter iter, variant_iter;

    dbus_message_get_args(message, NULL, DBUS_TYPE_INT32, &id, DBUS_TYPE_STRING, &prop_name, DBUS_TYPE_INVALID);

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &iter);

    /* Return empty string as default */
    {
      const char* value = "";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Properties", "Get"))
  {
    const char* prop_interface;
    const char* prop_name;
    DBusMessage* reply;
    DBusMessageIter iter, variant_iter;

    dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &prop_interface, DBUS_TYPE_STRING, &prop_name, DBUS_TYPE_INVALID);

    if (strcmp(prop_interface, DBUSMENU_INTERFACE) != 0)
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &iter);

    if (strcmp(prop_name, "Version") == 0)
    {
      dbus_uint32_t value = 4;
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "u", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_UINT32, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "TextDirection") == 0)
    {
      const char* value = "ltr";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "Status") == 0)
    {
      const char* value = "normal";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "IconThemePath") == 0)
    {
      DBusMessageIter array_iter;
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "as", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "s", &array_iter);
      dbus_message_iter_close_container(&variant_iter, &array_iter);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Properties", "GetAll"))
  {
    const char* prop_interface;
    DBusMessage* reply;
    DBusMessageIter iter, dict_iter, dict_entry_iter, variant_iter;

    dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &prop_interface, DBUS_TYPE_INVALID);

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);

    if (strcmp(prop_interface, DBUSMENU_INTERFACE) == 0)
    {
      {
        const char* prop_name = "Version";
        dbus_uint32_t value = 4;
        dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
        dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
        dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "u", &variant_iter);
        dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_UINT32, &value);
        dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
        dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
      }

      {
        const char* prop_name = "TextDirection";
        const char* value = "ltr";
        dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
        dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
        dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
        dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
        dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
        dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
      }

      {
        const char* prop_name = "Status";
        const char* value = "normal";
        dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
        dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
        dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
        dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
        dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
        dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
      }

      {
        const char* prop_name = "IconThemePath";
        DBusMessageIter array_iter;
        dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
        dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
        dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "as", &variant_iter);
        dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "s", &array_iter);
        dbus_message_iter_close_container(&variant_iter, &array_iter);
        dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
        dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
      }
    }

    dbus_message_iter_close_container(&iter, &dict_iter);

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/****************************************************************************
 * SNI message handler
 ****************************************************************************/

static DBusHandlerResult sniMessageHandler(DBusConnection* connection, DBusMessage* message, void* user_data)
{
  IupUnixSNI* sni = (IupUnixSNI*)user_data;
  const char* interface = dbus_message_get_interface(message);

  if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Introspectable", "Introspect"))
  {
    DBusMessage* reply = dbus_message_new_method_return(message);
    const char* xml_ptr = sni_introspection_xml;
    dbus_message_append_args(reply, DBUS_TYPE_STRING, &xml_ptr, DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (strcmp(interface, SNI_ITEM_INTERFACE) != 0 &&
      strcmp(interface, "org.freedesktop.DBus.Properties") != 0)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (dbus_message_is_method_call(message, SNI_ITEM_INTERFACE, "Activate"))
  {
    dbus_int32_t x, y;
    dbus_message_get_args(message, NULL, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y, DBUS_TYPE_INVALID);

    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);

    sniInvokeCallback(sni, 1, 1, x, y);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, SNI_ITEM_INTERFACE, "SecondaryActivate"))
  {
    dbus_int32_t x, y;
    dbus_message_get_args(message, NULL, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y, DBUS_TYPE_INVALID);

    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);

    sniInvokeCallback(sni, 2, 1, x, y);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, SNI_ITEM_INTERFACE, "ContextMenu"))
  {
    dbus_int32_t x, y;
    dbus_message_get_args(message, NULL, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y, DBUS_TYPE_INVALID);

    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);

    /* If menu is set, tray host will use dbusmenu protocol instead */
    /* This method is called only when dbusmenu is not available or menu not set */
    if (!sni->menu_ih)
      sniInvokeCallback(sni, 3, 1, x, y);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, SNI_ITEM_INTERFACE, "Scroll"))
  {
    DBusMessage* reply = dbus_message_new_method_return(message);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Properties", "Get"))
  {
    const char* prop_interface;
    const char* prop_name;
    DBusMessage* reply;
    DBusMessageIter iter, variant_iter;

    dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &prop_interface, DBUS_TYPE_STRING, &prop_name, DBUS_TYPE_INVALID);

    if (strcmp(prop_interface, SNI_ITEM_INTERFACE) != 0)
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &iter);

    if (strcmp(prop_name, "Category") == 0)
    {
      const char* value = "ApplicationStatus";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "Id") == 0)
    {
      const char* value = sni->bus_name;
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "Title") == 0)
    {
      const char* value = "IUP Application";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "Status") == 0)
    {
      const char* value = sni->visible ? "Active" : "Passive";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "WindowId") == 0)
    {
      dbus_int32_t value = 0;
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "i", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_INT32, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "IconName") == 0)
    {
      const char* value = "";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "IconPixmap") == 0)
    {
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "a(iiay)", &variant_iter);
      sniAppendIconPixmap(&variant_iter, sni);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "OverlayIconName") == 0 || strcmp(prop_name, "AttentionIconName") == 0 || strcmp(prop_name, "AttentionMovieName") == 0)
    {
      const char* value = "";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "OverlayIconPixmap") == 0 || strcmp(prop_name, "AttentionIconPixmap") == 0)
    {
      DBusMessageIter array_iter;
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "a(iiay)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);
      dbus_message_iter_close_container(&variant_iter, &array_iter);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "ToolTip") == 0)
    {
      DBusMessageIter struct_iter, array_iter;
      const char* tooltip_text = sni->tooltip ? sni->tooltip : "";
      const char* empty = "";

      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "(sa(iiay)ss)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &empty);
      dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);
      dbus_message_iter_close_container(&struct_iter, &array_iter);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &tooltip_text);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &empty);
      dbus_message_iter_close_container(&variant_iter, &struct_iter);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "ItemIsMenu") == 0)
    {
      dbus_bool_t value = FALSE;
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "b", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }
    else if (strcmp(prop_name, "Menu") == 0)
    {
      const char* value = sni->menu_ih ? DBUSMENU_OBJECT_PATH : "/NO_DBUSMENU";
      dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "o", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_OBJECT_PATH, &value);
      dbus_message_iter_close_container(&iter, &variant_iter);
    }

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_method_call(message, "org.freedesktop.DBus.Properties", "GetAll"))
  {
    const char* prop_interface;
    DBusMessage* reply;
    DBusMessageIter iter, dict_iter, dict_entry_iter, variant_iter;

    dbus_message_get_args(message, NULL, DBUS_TYPE_STRING, &prop_interface, DBUS_TYPE_INVALID);

    if (strcmp(prop_interface, SNI_ITEM_INTERFACE) != 0)
    {
      reply = dbus_message_new_method_return(message);
      dbus_message_iter_init_append(reply, &iter);
      dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
      dbus_message_iter_close_container(&iter, &dict_iter);
      dbus_connection_send(connection, reply, NULL);
      dbus_message_unref(reply);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    reply = dbus_message_new_method_return(message);
    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);

    {
      const char* prop_name = "Category";
      const char* value = "ApplicationStatus";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "Id";
      const char* value = sni->bus_name;
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "Title";
      const char* value = "IUP Application";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "Status";
      const char* value = sni->visible ? "Active" : "Passive";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "WindowId";
      dbus_int32_t value = 0;
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "i", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_INT32, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "IconName";
      const char* value = "";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "IconPixmap";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "a(iiay)", &variant_iter);
      sniAppendIconPixmap(&variant_iter, sni);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "OverlayIconName";
      const char* value = "";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "OverlayIconPixmap";
      DBusMessageIter array_iter;
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "a(iiay)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);
      dbus_message_iter_close_container(&variant_iter, &array_iter);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "AttentionIconName";
      const char* value = "";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "AttentionIconPixmap";
      DBusMessageIter array_iter;
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "a(iiay)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);
      dbus_message_iter_close_container(&variant_iter, &array_iter);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "AttentionMovieName";
      const char* value = "";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "ToolTip";
      DBusMessageIter struct_iter, array_iter;
      const char* tooltip_text = sni->tooltip ? sni->tooltip : "";
      const char* empty = "";

      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "(sa(iiay)ss)", &variant_iter);
      dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &empty);
      dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "(iiay)", &array_iter);
      dbus_message_iter_close_container(&struct_iter, &array_iter);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &tooltip_text);
      dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &empty);
      dbus_message_iter_close_container(&variant_iter, &struct_iter);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "ItemIsMenu";
      dbus_bool_t value = FALSE;
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "b", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_BOOLEAN, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    {
      const char* prop_name = "Menu";
      const char* value = sni->menu_ih ? DBUSMENU_OBJECT_PATH : "/NO_DBUSMENU";
      dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter);
      dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, &prop_name);
      dbus_message_iter_open_container(&dict_entry_iter, DBUS_TYPE_VARIANT, "o", &variant_iter);
      dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_OBJECT_PATH, &value);
      dbus_message_iter_close_container(&dict_entry_iter, &variant_iter);
      dbus_message_iter_close_container(&dict_iter, &dict_entry_iter);
    }

    dbus_message_iter_close_container(&iter, &dict_iter);

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/****************************************************************************
 * Signal emission
 ****************************************************************************/

static void sniEmitSignal(IupUnixSNI* sni, const char* signal_name)
{
  DBusMessage* signal = dbus_message_new_signal(SNI_ITEM_OBJECT_PATH, SNI_ITEM_INTERFACE, signal_name);

  if (signal)
  {
    dbus_connection_send(sni->connection, signal, NULL);
    dbus_connection_flush(sni->connection);
    dbus_message_unref(signal);
  }
}

static void sniEmitNewStatus(IupUnixSNI* sni)
{
  DBusMessage* signal = dbus_message_new_signal(SNI_ITEM_OBJECT_PATH, SNI_ITEM_INTERFACE, "NewStatus");

  if (signal)
  {
    const char* status = sni->visible ? "Active" : "Passive";
    dbus_message_append_args(signal, DBUS_TYPE_STRING, &status, DBUS_TYPE_INVALID);
    dbus_connection_send(sni->connection, signal, NULL);
    dbus_connection_flush(sni->connection);
    dbus_message_unref(signal);
  }
}

/****************************************************************************
 * Watcher registration
 ****************************************************************************/

static void sniRegisterToWatcher(IupUnixSNI* sni)
{
  DBusMessage* msg;
  DBusMessage* reply;
  DBusError error;

  dbus_error_init(&error);

  msg = dbus_message_new_method_call(SNI_WATCHER_BUS_NAME, SNI_WATCHER_OBJECT_PATH, SNI_WATCHER_INTERFACE, "RegisterStatusNotifierItem");

  if (!msg)
    return;

  dbus_message_append_args(msg, DBUS_TYPE_STRING, &sni->bus_name, DBUS_TYPE_INVALID);

  reply = dbus_connection_send_with_reply_and_block(sni->connection, msg, 1000, &error);

  if (dbus_error_is_set(&error))
    dbus_error_free(&error);

  if (reply)
    dbus_message_unref(reply);

  dbus_message_unref(msg);

  dbus_connection_flush(sni->connection);

  sni->registered = 1;
}

static void sniUnregisterFromWatcher(IupUnixSNI* sni)
{
  if (!sni->registered)
    return;

  /* Release our bus name, this effectively unregisters us from the watcher */
  if (sni->connection && sni->bus_name)
  {
    dbus_bus_release_name(sni->connection, sni->bus_name, NULL);
    dbus_connection_flush(sni->connection);
  }

  sni->registered = 0;
}

static void sniReacquireBusName(IupUnixSNI* sni)
{
  if (sni->connection && sni->bus_name)
  {
    dbus_bus_request_name(sni->connection, sni->bus_name, DBUS_NAME_FLAG_DO_NOT_QUEUE, NULL);
    dbus_connection_flush(sni->connection);
  }
}

/****************************************************************************
 * Timer callback for DBus dispatch
 ****************************************************************************/

static int sniTimerCallback(Ihandle* timer)
{
  IupUnixSNI* sni = (IupUnixSNI*)IupGetAttribute(timer, "_IUPUNIX_SNI");
  DBusConnection* conn;

  if (!sni || sni->cleanup_in_progress)
    return IUP_DEFAULT;

  sni->ref_count++;

  conn = sni->connection;

  if (!conn)
  {
    sni->ref_count--;
    return IUP_DEFAULT;
  }

  dbus_connection_read_write(conn, 0);

  if (!sni->cleanup_in_progress && sni->connection == conn)
  {
    while (dbus_connection_get_dispatch_status(conn) == DBUS_DISPATCH_DATA_REMAINS)
    {
      if (sni->cleanup_in_progress || sni->connection != conn)
        break;
      dbus_connection_dispatch(conn);
    }
  }

  sni->ref_count--;

  return IUP_DEFAULT;
}

/****************************************************************************
 * Public API
 ****************************************************************************/

static int sniInit(void)
{
  if (g_sni_initialized)
    return 1;

#ifdef IUPDBUS_USE_DLOPEN
  if (!iupDBusOpen())
    return 0;
#endif

  g_sni_initialized = 1;
  return 1;
}

static void sniFinish(void)
{
  if (g_sni_initialized)
  {
#ifdef IUPDBUS_USE_DLOPEN
    iupDBusClose();
#endif
    g_sni_initialized = 0;
  }
}

static int sniIsAvailable(void)
{
#ifdef IUPDBUS_USE_DLOPEN
  return iupDBusOpen() ? 1 : 0;
#else
  return 1;
#endif
}

static int sniCreate(Ihandle* ih)
{
  IupUnixSNI* sni;
  DBusError error;
  static unsigned int counter = 0;
  int result;
  DBusObjectPathVTable sni_vtable = { NULL, sniMessageHandler, NULL, NULL, NULL, NULL };
  DBusObjectPathVTable menu_vtable = { NULL, sniDBusMenuHandler, NULL, NULL, NULL, NULL };

  if (!sniInit())
    return 0;

  dbus_error_init(&error);

  sni = (IupUnixSNI*)calloc(1, sizeof(IupUnixSNI));
  sni->ih = ih;
  sni->visible = 0;
  sni->last_button = -1;
  sni->menu_ih = NULL;
  sni->menu_revision = 1;
  sni->next_menu_id = 1;
  sni->menu_item_count = 0;

  sni->connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
  if (dbus_error_is_set(&error))
  {
    dbus_error_free(&error);
    free(sni);
    return 0;
  }

  sni->bus_name = (char*)malloc(64);
  snprintf(sni->bus_name, 64, "org.kde.StatusNotifierItem-%u-%u", getpid(), counter++);

  result = dbus_bus_request_name(sni->connection, sni->bus_name, DBUS_NAME_FLAG_REPLACE_EXISTING, &error);

  if (dbus_error_is_set(&error) || result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
  {
    if (dbus_error_is_set(&error))
      dbus_error_free(&error);
    dbus_connection_unref(sni->connection);
    free(sni->bus_name);
    free(sni);
    return 0;
  }

  if (!dbus_connection_register_object_path(sni->connection, SNI_ITEM_OBJECT_PATH, &sni_vtable, sni))
  {
    dbus_bus_release_name(sni->connection, sni->bus_name, NULL);
    dbus_connection_unref(sni->connection);
    free(sni->bus_name);
    free(sni);
    return 0;
  }

  /* Register dbusmenu object path */
  dbus_connection_register_object_path(sni->connection, DBUSMENU_OBJECT_PATH, &menu_vtable, sni);

  sniRegisterToWatcher(sni);

  /* Create IUP timer for DBus dispatch */
  sni->timer = IupTimer();
  IupSetAttribute(sni->timer, "TIME", "50");
  IupSetAttribute(sni->timer, "_IUPUNIX_SNI", (char*)sni);
  IupSetCallback(sni->timer, "ACTION_CB", (Icallback)sniTimerCallback);
  IupSetAttribute(sni->timer, "RUN", "YES");

  iupAttribSet(ih, "_IUPUNIX_SNI", (char*)sni);

  return 1;
}

static void sniDestroy(Ihandle* ih)
{
  IupUnixSNI* sni = (IupUnixSNI*)iupAttribGet(ih, "_IUPUNIX_SNI");

  if (sni)
  {
    iupAttribSet(ih, "_IUPUNIX_SNI", NULL);

    sni->cleanup_in_progress = 1;
    sni->ih = NULL;

    /* Stop timer */
    if (sni->timer)
    {
      IupSetAttribute(sni->timer, "RUN", "NO");
      IupDestroy(sni->timer);
      sni->timer = NULL;
    }

    if (sni->connection)
    {
      dbus_connection_flush(sni->connection);
      dbus_connection_unregister_object_path(sni->connection, DBUSMENU_OBJECT_PATH);
      dbus_connection_unregister_object_path(sni->connection, SNI_ITEM_OBJECT_PATH);

      if (sni->bus_name)
        dbus_bus_release_name(sni->connection, sni->bus_name, NULL);
    }

    if (sni->ref_count == 0)
    {
      if (sni->connection)
      {
        dbus_connection_close(sni->connection);
        dbus_connection_unref(sni->connection);
      }

      if (sni->icon_pixels)
        free(sni->icon_pixels);

      if (sni->bus_name)
        free(sni->bus_name);

      if (sni->tooltip)
        free(sni->tooltip);

      free(sni);
    }
  }
}

/******************************************************************************/
/* Driver Interface Implementation                                            */
/******************************************************************************/

void iupdrvTrayInitClass(Iclass* ic)
{
  (void)ic;
}

int iupdrvTraySetVisible(Ihandle* ih, int visible)
{
  IupUnixSNI* sni = (IupUnixSNI*)iupAttribGet(ih, "_IUPUNIX_SNI");

  if (!sni)
  {
    if (!visible)
      return 1;

    if (!sniCreate(ih))
      return 0;
    sni = (IupUnixSNI*)iupAttribGet(ih, "_IUPUNIX_SNI");
  }

  if (!sni)
    return 0;

  if (visible && !sni->visible)
  {
    sni->visible = 1;
    if (!sni->registered)
    {
      sniReacquireBusName(sni);
      sniRegisterToWatcher(sni);
    }
  }
  else if (!visible && sni->visible)
  {
    sni->visible = 0;
    sniUnregisterFromWatcher(sni);
  }

  return 1;
}

int iupdrvTraySetImage(Ihandle* ih, const char* value)
{
  IupUnixSNI* sni = (IupUnixSNI*)iupAttribGet(ih, "_IUPUNIX_SNI");
  int width, height;
  unsigned char* pixels = NULL;

  if (!sni)
  {
    if (!sniCreate(ih))
      return 0;
    sni = (IupUnixSNI*)iupAttribGet(ih, "_IUPUNIX_SNI");
  }

  if (!sni)
    return 0;

  /* Free previous icon */
  if (sni->icon_pixels)
  {
    free(sni->icon_pixels);
    sni->icon_pixels = NULL;
  }
  sni->icon_width = 0;
  sni->icon_height = 0;

  /* Get new icon pixels from driver */
  if (value && iupdrvGetIconPixels(ih, value, &width, &height, &pixels))
  {
    sni->icon_width = width;
    sni->icon_height = height;
    sni->icon_pixels = pixels;
  }

  sniEmitSignal(sni, "NewIcon");

  return 1;
}

int iupdrvTraySetTip(Ihandle* ih, const char* value)
{
  IupUnixSNI* sni = (IupUnixSNI*)iupAttribGet(ih, "_IUPUNIX_SNI");

  if (!sni)
  {
    if (!sniCreate(ih))
      return 0;
    sni = (IupUnixSNI*)iupAttribGet(ih, "_IUPUNIX_SNI");
  }

  if (!sni)
    return 0;

  if (sni->tooltip)
    free(sni->tooltip);

  sni->tooltip = value ? strdup(value) : NULL;
  sniEmitSignal(sni, "NewToolTip");

  return 1;
}

int iupdrvTraySetMenu(Ihandle* ih, Ihandle* menu)
{
  IupUnixSNI* sni = (IupUnixSNI*)iupAttribGet(ih, "_IUPUNIX_SNI");

  if (!sni)
  {
    if (!sniCreate(ih))
      return 0;
    sni = (IupUnixSNI*)iupAttribGet(ih, "_IUPUNIX_SNI");
  }

  if (!sni)
    return 0;

  if (menu && iupObjectCheck(menu))
  {
    sni->menu_ih = menu;
    sni->menu_revision++;
  }
  else
  {
    sni->menu_ih = NULL;
  }

  return 1;
}

void iupdrvTrayDestroy(Ihandle* ih)
{
  sniDestroy(ih);
}

int iupdrvTrayIsAvailable(void)
{
#ifdef IUPDBUS_USE_DLOPEN
  return iupDBusOpen() ? 1 : 0;
#else
  return 1;
#endif
}
