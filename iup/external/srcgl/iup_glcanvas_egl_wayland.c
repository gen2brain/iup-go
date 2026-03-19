/** \file
 * \brief Shared Wayland helpers for EGL GLCanvas backends.
 *        Included from backend files, not compiled standalone.
 *
 * See Copyright Notice in "iup.h"
 */

/* Retrieve wl_compositor and wl_subcompositor from the Wayland registry.
 * Uses a private event queue to avoid interfering with GTK/Qt/EFL's main queue.
 * Caller must keep the returned queue alive while compositor/subcompositor are used. */

struct wayland_registry_data {
  struct wl_compositor* compositor;
  struct wl_subcompositor* subcompositor;
};

static void wayland_registry_handle_global(void* data, struct wl_registry* registry,
                                           uint32_t name, const char* interface, uint32_t version)
{
  struct wayland_registry_data* reg_data = (struct wayland_registry_data*)data;
  (void)version;

  if (strcmp(interface, "wl_compositor") == 0) {
    reg_data->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
  }
  else if (strcmp(interface, "wl_subcompositor") == 0) {
    reg_data->subcompositor = wl_registry_bind(registry, name, &wl_subcompositor_interface, 1);
  }
}

static void wayland_registry_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{
  (void)data;
  (void)registry;
  (void)name;
}

static const struct wl_registry_listener wayland_registry_listener = {
  wayland_registry_handle_global,
  wayland_registry_handle_global_remove
};

static struct wl_subcompositor* eGLCanvasGetWaylandSubcompositor(struct wl_display* wl_display,
                                                                   struct wl_event_queue** out_queue,
                                                                   struct wl_compositor** out_compositor,
                                                                   struct wl_registry** out_registry)
{
  struct wayland_registry_data reg_data = { NULL, NULL };
  struct wl_registry* registry;
  struct wl_event_queue* queue = NULL;

  if (!wl_display) {
    return NULL;
  }

  queue = wl_display_create_queue(wl_display);
  if (!queue) {
    return NULL;
  }

  registry = wl_display_get_registry(wl_display);
  if (!registry) {
    wl_event_queue_destroy(queue);
    return NULL;
  }

  wl_proxy_set_queue((struct wl_proxy*)registry, queue);

  wl_registry_add_listener(registry, &wayland_registry_listener, &reg_data);

  if (wl_display_roundtrip_queue(wl_display, queue) < 0) {
    wl_registry_destroy(registry);
    wl_event_queue_destroy(queue);
    return NULL;
  }

  if (out_queue) {
    *out_queue = queue;
  }

  if (out_compositor) {
    *out_compositor = reg_data.compositor;
  }

  if (out_registry) {
    *out_registry = registry;
  }

  return reg_data.subcompositor;
}
