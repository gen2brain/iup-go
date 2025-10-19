/** \file
 * \brief GTK System Tray (SNI/StatusNotifierItem)
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <gio/gio.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"

#include "iupgtk_drv.h"

#if GTK_CHECK_VERSION(3, 0, 0)

#define SNI_WATCHER_BUS_NAME "org.kde.StatusNotifierWatcher"
#define SNI_WATCHER_OBJECT_PATH "/StatusNotifierWatcher"
#define SNI_WATCHER_INTERFACE "org.kde.StatusNotifierWatcher"

#define SNI_ITEM_INTERFACE "org.kde.StatusNotifierItem"
#define SNI_ITEM_OBJECT_PATH "/StatusNotifierItem"

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.kde.StatusNotifierItem'>"
  "    <property name='Category' type='s' access='read'/>"
  "    <property name='Id' type='s' access='read'/>"
  "    <property name='Title' type='s' access='read'/>"
  "    <property name='Status' type='s' access='read'/>"
  "    <property name='WindowId' type='i' access='read'/>"
  "    <property name='IconName' type='s' access='read'/>"
  "    <property name='IconPixmap' type='a(iiay)' access='read'/>"
  "    <property name='OverlayIconName' type='s' access='read'/>"
  "    <property name='OverlayIconPixmap' type='a(iiay)' access='read'/>"
  "    <property name='AttentionIconName' type='s' access='read'/>"
  "    <property name='AttentionIconPixmap' type='a(iiay)' access='read'/>"
  "    <property name='AttentionMovieName' type='s' access='read'/>"
  "    <property name='ToolTip' type='(sa(iiay)ss)' access='read'/>"
  "    <property name='ItemIsMenu' type='b' access='read'/>"
  "    <method name='Activate'>"
  "      <arg name='x' type='i' direction='in'/>"
  "      <arg name='y' type='i' direction='in'/>"
  "    </method>"
  "    <method name='SecondaryActivate'>"
  "      <arg name='x' type='i' direction='in'/>"
  "      <arg name='y' type='i' direction='in'/>"
  "    </method>"
  "    <method name='ContextMenu'>"
  "      <arg name='x' type='i' direction='in'/>"
  "      <arg name='y' type='i' direction='in'/>"
  "    </method>"
  "    <method name='Scroll'>"
  "      <arg name='delta' type='i' direction='in'/>"
  "      <arg name='orientation' type='s' direction='in'/>"
  "    </method>"
  "    <signal name='NewTitle'/>"
  "    <signal name='NewIcon'/>"
  "    <signal name='NewAttentionIcon'/>"
  "    <signal name='NewOverlayIcon'/>"
  "    <signal name='NewToolTip'/>"
  "    <signal name='NewStatus'>"
  "      <arg name='status' type='s'/>"
  "    </signal>"
  "  </interface>"
  "</node>";

typedef struct _IupGtkSNI {
  GDBusConnection* connection;
  GDBusNodeInfo* introspection_data;
  guint registration_id;
  guint watcher_id;
  gchar* bus_name;
  Ihandle* ih;
  GdkPixbuf* icon;
  gchar* tooltip;
  gboolean visible;
  gint last_x;
  gint last_y;
} IupGtkSNI;

static int gtkSNIDoubleClick(int button)
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

    if (!timer)
      return 0;

    seconds = g_timer_elapsed(timer, NULL);
    if (seconds < 0.4)
    {
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

static GVariant* gtkSNIPixbufToVariant(GdkPixbuf* pixbuf)
{
  GVariantBuilder builder;
  GVariantBuilder icon_builder;
  GVariantBuilder data_builder;
  gint width, height, rowstride, n_channels;
  guchar* pixels;
  gboolean has_alpha;
  gint x, y;

  if (!pixbuf)
    return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);

  width = gdk_pixbuf_get_width(pixbuf);
  height = gdk_pixbuf_get_height(pixbuf);
  rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  n_channels = gdk_pixbuf_get_n_channels(pixbuf);
  pixels = gdk_pixbuf_get_pixels(pixbuf);
  has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);

  g_variant_builder_init(&data_builder, G_VARIANT_TYPE("ay"));

  for (y = 0; y < height; y++)
  {
    for (x = 0; x < width; x++)
    {
      guchar* pixel = pixels + y * rowstride + x * n_channels;
      guchar r = pixel[0];
      guchar g = pixel[1];
      guchar b = pixel[2];
      guchar a = has_alpha ? pixel[3] : 0xff;

      g_variant_builder_add(&data_builder, "y", a);
      g_variant_builder_add(&data_builder, "y", r);
      g_variant_builder_add(&data_builder, "y", g);
      g_variant_builder_add(&data_builder, "y", b);
    }
  }

  g_variant_builder_init(&icon_builder, G_VARIANT_TYPE("(iiay)"));
  g_variant_builder_add(&icon_builder, "i", width);
  g_variant_builder_add(&icon_builder, "i", height);
  g_variant_builder_add_value(&icon_builder, g_variant_builder_end(&data_builder));

  g_variant_builder_init(&builder, G_VARIANT_TYPE("a(iiay)"));
  g_variant_builder_add_value(&builder, g_variant_builder_end(&icon_builder));

  return g_variant_builder_end(&builder);
}

typedef struct _IupGtkSNICallbackData {
  Ihandle* ih;
  int button;
  int pressed;
  int dclick;
  int x;
  int y;
} IupGtkSNICallbackData;

static gboolean gtkSNIInvokeCallback(gpointer user_data)
{
  IupGtkSNICallbackData* data = (IupGtkSNICallbackData*)user_data;
  IFniii cb = (IFniii)IupGetCallback(data->ih, "TRAYCLICK_CB");

  if (cb)
  {
    if (cb(data->ih, data->button, data->pressed, data->dclick) == IUP_CLOSE)
      IupExitLoop();
  }

  g_free(data);
  return G_SOURCE_REMOVE;
}

static void gtkSNIHandleMethodCall(GDBusConnection* connection,
                                   const gchar* sender,
                                   const gchar* object_path,
                                   const gchar* interface_name,
                                   const gchar* method_name,
                                   GVariant* parameters,
                                   GDBusMethodInvocation* invocation,
                                   gpointer user_data)
{
  IupGtkSNI* sni = (IupGtkSNI*)user_data;
  (void)connection;
  (void)sender;
  (void)object_path;
  (void)interface_name;

  if (g_strcmp0(method_name, "Activate") == 0)
  {
    gint x, y;
    int button = 1;
    int pressed = 1;
    int dclick = gtkSNIDoubleClick(button);
    IFniii cb = (IFniii)IupGetCallback(sni->ih, "TRAYCLICK_CB");

    g_variant_get(parameters, "(ii)", &x, &y);

    g_dbus_method_invocation_return_value(invocation, NULL);

    if (cb)
    {
      IupGtkSNICallbackData* data = g_new(IupGtkSNICallbackData, 1);
      data->ih = sni->ih;
      data->button = button;
      data->pressed = pressed;
      data->dclick = dclick;
      data->x = x;
      data->y = y;
      g_idle_add(gtkSNIInvokeCallback, data);
    }
  }
  else if (g_strcmp0(method_name, "SecondaryActivate") == 0)
  {
    gint x, y;
    int button = 2;
    int pressed = 1;
    int dclick = gtkSNIDoubleClick(button);
    IFniii cb = (IFniii)IupGetCallback(sni->ih, "TRAYCLICK_CB");

    g_variant_get(parameters, "(ii)", &x, &y);

    g_dbus_method_invocation_return_value(invocation, NULL);

    if (cb)
    {
      IupGtkSNICallbackData* data = g_new(IupGtkSNICallbackData, 1);
      data->ih = sni->ih;
      data->button = button;
      data->pressed = pressed;
      data->dclick = dclick;
      data->x = x;
      data->y = y;
      g_idle_add(gtkSNIInvokeCallback, data);
    }
  }
  else if (g_strcmp0(method_name, "ContextMenu") == 0)
  {
    gint x, y;
    int button = 3;
    int pressed = 1;
    int dclick = gtkSNIDoubleClick(button);
    IFniii cb = (IFniii)IupGetCallback(sni->ih, "TRAYCLICK_CB");

    g_variant_get(parameters, "(ii)", &x, &y);

    g_dbus_method_invocation_return_value(invocation, NULL);

    if (cb)
    {
      IupGtkSNICallbackData* data = g_new(IupGtkSNICallbackData, 1);
      data->ih = sni->ih;
      data->button = button;
      data->pressed = pressed;
      data->dclick = dclick;
      data->x = x;
      data->y = y;
      g_idle_add(gtkSNIInvokeCallback, data);
    }
  }
  else if (g_strcmp0(method_name, "Scroll") == 0)
  {
    gint delta;
    gchar* orientation;

    g_variant_get(parameters, "(is)", &delta, &orientation);

    g_free(orientation);
    g_dbus_method_invocation_return_value(invocation, NULL);
  }
  else
  {
    g_dbus_method_invocation_return_dbus_error(invocation,
                                                "org.freedesktop.DBus.Error.UnknownMethod",
                                                "Method is not implemented");
  }
}

static GVariant* gtkSNIHandleGetProperty(GDBusConnection* connection,
                                         const gchar* sender,
                                         const gchar* object_path,
                                         const gchar* interface_name,
                                         const gchar* property_name,
                                         GError** error,
                                         gpointer user_data)
{
  IupGtkSNI* sni = (IupGtkSNI*)user_data;
  (void)connection;
  (void)sender;
  (void)object_path;
  (void)interface_name;
  (void)error;

  if (g_strcmp0(property_name, "Category") == 0)
    return g_variant_new_string("ApplicationStatus");
  else if (g_strcmp0(property_name, "Id") == 0)
    return g_variant_new_string(sni->bus_name);
  else if (g_strcmp0(property_name, "Title") == 0)
    return g_variant_new_string("IUP Application");
  else if (g_strcmp0(property_name, "Status") == 0)
    return g_variant_new_string(sni->visible ? "Active" : "Passive");
  else if (g_strcmp0(property_name, "WindowId") == 0)
    return g_variant_new_int32(0);
  else if (g_strcmp0(property_name, "IconName") == 0)
    return g_variant_new_string("");
  else if (g_strcmp0(property_name, "IconPixmap") == 0)
    return gtkSNIPixbufToVariant(sni->icon);
  else if (g_strcmp0(property_name, "OverlayIconName") == 0)
    return g_variant_new_string("");
  else if (g_strcmp0(property_name, "OverlayIconPixmap") == 0)
    return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);
  else if (g_strcmp0(property_name, "AttentionIconName") == 0)
    return g_variant_new_string("");
  else if (g_strcmp0(property_name, "AttentionIconPixmap") == 0)
    return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);
  else if (g_strcmp0(property_name, "AttentionMovieName") == 0)
    return g_variant_new_string("");
  else if (g_strcmp0(property_name, "ToolTip") == 0)
  {
    GVariantBuilder builder;
    const gchar* tooltip_text = sni->tooltip ? sni->tooltip : "";
    GVariant* empty_icon_array = g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);

    g_variant_builder_init(&builder, G_VARIANT_TYPE("(sa(iiay)ss)"));
    g_variant_builder_add(&builder, "s", "");
    g_variant_builder_add_value(&builder, empty_icon_array);
    g_variant_builder_add(&builder, "s", tooltip_text);
    g_variant_builder_add(&builder, "s", "");

    return g_variant_builder_end(&builder);
  }
  else if (g_strcmp0(property_name, "ItemIsMenu") == 0)
    return g_variant_new_boolean(FALSE);

  return NULL;
}

static const GDBusInterfaceVTable interface_vtable = {
  gtkSNIHandleMethodCall,
  gtkSNIHandleGetProperty,
  NULL
};

static void gtkSNIRegisterToWatcher(IupGtkSNI* sni)
{
  GError* error = NULL;
  GDBusProxy* proxy;
  GVariant* result;

  proxy = g_dbus_proxy_new_sync(sni->connection,
                                G_DBUS_PROXY_FLAGS_NONE,
                                NULL,
                                SNI_WATCHER_BUS_NAME,
                                SNI_WATCHER_OBJECT_PATH,
                                SNI_WATCHER_INTERFACE,
                                NULL,
                                &error);

  if (!proxy)
  {
    if (error)
      g_error_free(error);
    return;
  }

  result = g_dbus_proxy_call_sync(proxy,
                                  "RegisterStatusNotifierItem",
                                  g_variant_new("(s)", sni->bus_name),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  &error);

  if (result)
    g_variant_unref(result);
  if (error)
    g_error_free(error);

  g_object_unref(proxy);
}

static void gtkSNIEmitSignal(IupGtkSNI* sni, const gchar* signal_name)
{
  GError* error = NULL;

  g_dbus_connection_emit_signal(sni->connection,
                                NULL,
                                SNI_ITEM_OBJECT_PATH,
                                SNI_ITEM_INTERFACE,
                                signal_name,
                                NULL,
                                &error);

  if (error)
  {
    g_error_free(error);
  }
}

static void gtkSNIOnNameAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
  IupGtkSNI* sni = (IupGtkSNI*)user_data;
  (void)connection;
  (void)name;

  gtkSNIRegisterToWatcher(sni);
}

static void gtkSNIOnNameLost(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
  (void)connection;
  (void)name;
  (void)user_data;
}

static IupGtkSNI* gtkGetSNI(Ihandle* ih)
{
  IupGtkSNI* sni = (IupGtkSNI*)iupAttribGet(ih, "_IUPGTK_SNI");

  if (!sni)
  {
    GError* error = NULL;
    static guint counter = 0;

    sni = g_new0(IupGtkSNI, 1);
    sni->ih = ih;
    sni->visible = FALSE;
    sni->last_x = -1;
    sni->last_y = -1;

    sni->connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!sni->connection)
    {
      if (error)
        g_error_free(error);
      g_free(sni);
      return NULL;
    }

    sni->bus_name = g_strdup_printf("org.kde.StatusNotifierItem-%u-%u", getpid(), counter++);

    sni->introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);

    sni->registration_id = g_dbus_connection_register_object(
      sni->connection,
      SNI_ITEM_OBJECT_PATH,
      sni->introspection_data->interfaces[0],
      &interface_vtable,
      sni,
      NULL,
      &error);

    if (sni->registration_id == 0)
    {
      if (error)
        g_error_free(error);
      g_dbus_node_info_unref(sni->introspection_data);
      g_object_unref(sni->connection);
      g_free(sni->bus_name);
      g_free(sni);
      return NULL;
    }

    sni->watcher_id = g_bus_own_name_on_connection(
      sni->connection,
      sni->bus_name,
      G_BUS_NAME_OWNER_FLAGS_NONE,
      gtkSNIOnNameAcquired,
      gtkSNIOnNameLost,
      sni,
      NULL);

    if (sni->watcher_id == 0)
    {
      g_dbus_connection_unregister_object(sni->connection, sni->registration_id);
      g_dbus_node_info_unref(sni->introspection_data);
      g_object_unref(sni->connection);
      g_free(sni->bus_name);
      g_free(sni);
      return NULL;
    }

    iupAttribSet(ih, "_IUPGTK_SNI", (char*)sni);
  }

  return sni;
}

int iupgtkSetTrayAttrib(Ihandle* ih, const char* value)
{
  IupGtkSNI* sni = gtkGetSNI(ih);
  if (!sni)
    return 0;

  sni->visible = iupStrBoolean(value);
  gtkSNIEmitSignal(sni, "NewStatus");

  return 1;
}

int iupgtkSetTrayTipAttrib(Ihandle* ih, const char* value)
{
  IupGtkSNI* sni = gtkGetSNI(ih);
  if (!sni)
    return 0;

  g_free(sni->tooltip);
  sni->tooltip = value ? g_strdup(value) : NULL;
  gtkSNIEmitSignal(sni, "NewToolTip");

  return 1;
}

int iupgtkSetTrayImageAttrib(Ihandle* ih, const char* value)
{
  IupGtkSNI* sni = gtkGetSNI(ih);
  if (!sni)
    return 0;

  if (sni->icon)
  {
    g_object_unref(sni->icon);
    sni->icon = NULL;
  }

  GdkPixbuf* icon = (GdkPixbuf*)iupImageGetIcon(value);
  if (icon)
    sni->icon = g_object_ref(icon);

  gtkSNIEmitSignal(sni, "NewIcon");

  return 1;
}

void iupgtkTrayCleanup(Ihandle* ih)
{
  IupGtkSNI* sni = (IupGtkSNI*)iupAttribGet(ih, "_IUPGTK_SNI");

  if (sni)
  {
    if (sni->watcher_id > 0)
      g_bus_unown_name(sni->watcher_id);

    if (sni->registration_id > 0)
      g_dbus_connection_unregister_object(sni->connection, sni->registration_id);

    if (sni->introspection_data)
      g_dbus_node_info_unref(sni->introspection_data);

    if (sni->connection)
      g_object_unref(sni->connection);

    if (sni->icon)
      g_object_unref(sni->icon);

    g_free(sni->bus_name);
    g_free(sni->tooltip);
    g_free(sni);

    iupAttribSet(ih, "_IUPGTK_SNI", NULL);
  }
}

#else /* GTK 2 */
#include "external/src/gtk/iupgtk_tray_xembed.c"
#endif
