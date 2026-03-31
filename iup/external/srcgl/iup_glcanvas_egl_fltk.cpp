/** \file
 * \brief FLTK helper functions for EGL GLCanvas
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/platform.H>

#if defined(FLTK_USE_WAYLAND)
  #include <FL/wayland.H>
#endif

extern "C" {

#include "iup.h"
#include "iup_object.h"

void* iupFltkGLGetCanvasWindow(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return NULL;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  Fl_Window* window = widget->window();
  if (!window)
    return NULL;

  if (!window->shown())
    window->show();

  return (void*)window;
}

class IupFltkGLWindow : public Fl_Window
{
public:
  IupFltkGLWindow(int x, int y, int w, int h)
    : Fl_Window(x, y, w, h) {}

  void draw() override {}
};

void* iupFltkGLCreateChildWindow(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return NULL;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  Fl_Window* parent = widget->window();
  if (!parent || !parent->shown())
    return NULL;

  int x = widget->x();
  int y = widget->y();
  int w = widget->w();
  int h = widget->h();

  if (w <= 0) w = 1;
  if (h <= 0) h = 1;

  Fl_Group::current(NULL);
  IupFltkGLWindow* child = new IupFltkGLWindow(x, y, w, h);
  child->end();
  child->border(0);

  parent->add(child);
  child->show();

  return (void*)child;
}

int iupFltkGLResizeChildWindow(void* child_window, Ihandle* ih)
{
  Fl_Window* child = (Fl_Window*)child_window;
  if (!child || !ih || !ih->handle)
    return 0;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  int x = widget->x();
  int y = widget->y();
  int w = widget->w();
  int h = widget->h();
  if (w <= 0) w = 1;
  if (h <= 0) h = 1;

  if (x == child->x() && y == child->y() && w == child->w() && h == child->h())
    return 0;

  child->resize(x, y, w, h);
  return 1;
}

void iupFltkGLDestroyChildWindow(void* child_window)
{
  Fl_Window* child = (Fl_Window*)child_window;
  if (child)
  {
    Fl_Group* parent = child->parent();
    if (parent)
      parent->remove(child);
    delete child;
  }
}

int iupFltkGLIsWayland(void)
{
#if defined(FLTK_USE_WAYLAND)
  return fl_wl_display() != NULL;
#else
  return 0;
#endif
}

void* iupFltkGLGetDisplay(void)
{
#if defined(FLTK_USE_WAYLAND)
  if (fl_wl_display())
    return NULL;
#endif
  return (void*)fl_x11_display();
}

unsigned long iupFltkGLGetXid(void* fl_window)
{
  Fl_Window* window = (Fl_Window*)fl_window;
  if (!window)
    return 0;

#if defined(FLTK_USE_WAYLAND)
  if (fl_wl_display())
    return 0;
#endif

  return (unsigned long)fl_xid(window);
}

void* iupFltkGLGetWlDisplay(void)
{
#if defined(FLTK_USE_WAYLAND)
  return (void*)fl_wl_display();
#else
  return NULL;
#endif
}

void* iupFltkGLGetWlSurface(void* fl_window)
{
#if defined(FLTK_USE_WAYLAND)
  Fl_Window* window = (Fl_Window*)fl_window;
  if (!window || !fl_wl_display())
    return NULL;

  struct wld_window* wld = fl_wl_xid(window);
  if (!wld)
    return NULL;

  return (void*)fl_wl_surface(wld);
#else
  (void)fl_window;
  return NULL;
#endif
}

void* iupFltkGLGetWlCompositor(void)
{
#if defined(FLTK_USE_WAYLAND)
  if (!fl_wl_display())
    return NULL;
  return (void*)fl_wl_compositor();
#else
  return NULL;
#endif
}

int iupFltkGLGetBufferScale(void* fl_window)
{
#if defined(FLTK_USE_WAYLAND)
  Fl_Window* window = (Fl_Window*)fl_window;
  if (window && fl_wl_display())
    return fl_wl_buffer_scale(window);
#else
  (void)fl_window;
#endif
  return 1;
}

void iupFltkGLGetWidgetPosition(Ihandle* ih, int* x, int* y)
{
  *x = ih->x;
  *y = ih->y;
}

} /* extern "C" */
