/** \file
 * \brief Web Browser Control - FLTK host of the native web engine
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstring>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/platform.H>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
}

#include "iupfltk_drv.h"
#include "iupweb_host.h"


#ifdef __APPLE__
class IupFltkWebHost : public Fl_Widget
{
public:
  Ihandle* ih;
  int rect[4] = {0, 0, 0, 0};
  int clip[4] = {0, 0, -1, 0};

  explicit IupFltkWebHost(Ihandle* handle) : Fl_Widget(0, 0, 1, 1), ih(handle)
  {
  }

  int handle(int event) override
  {
    int ret = Fl_Widget::handle(event);
    if (event == FL_SHOW || event == FL_HIDE)
      place();
    return ret;
  }

  void resize(int x, int y, int w, int h) override
  {
    Fl_Widget::resize(x, y, w, h);
    place();
  }

  void draw() override
  {
  }

  void place()
  {
    int r[4] = {0, 0, 0, 0};
    int c[4] = {0, 0, 0, 0};
    int ox, oy;
    Fl_Window* top;

    if (!ih)
      return;

    top = top_window_offset(ox, oy);
    if (!top || !top->shown())
    {
      iupwebHostSetParent(ih, nullptr);
      return;
    }

    iupwebHostSetParent(ih, fl_mac_xid(top));

    if (visible_r())
    {
      float scale = Fl::screen_scale(top->screen_num());
      int x1 = ox > 0 ? ox : 0, y1 = oy > 0 ? oy : 0;
      int x2 = ox + w() < top->w() ? ox + w() : top->w();
      int y2 = oy + h() < top->h() ? oy + h() : top->h();

      for (Ihandle* parent = ih->parent; parent && parent->iclass->nativetype != IUP_TYPEDIALOG; parent = parent->parent)
      {
        auto* widget = reinterpret_cast<Fl_Widget*>(parent->handle);
        int px, py;

        if (parent->iclass->nativetype == IUP_TYPEVOID || !widget)
          continue;

        widget->top_window_offset(px, py);
        if (px > x1) x1 = px;
        if (py > y1) y1 = py;
        if (px + widget->w() < x2) x2 = px + widget->w();
        if (py + widget->h() < y2) y2 = py + widget->h();
      }

      r[0] = (int)(ox * scale); r[1] = (int)(oy * scale);
      r[2] = (int)(w() * scale); r[3] = (int)(h() * scale);
      if (x2 > x1 && y2 > y1)
      {
        c[0] = (int)(x1 * scale); c[1] = (int)(y1 * scale);
        c[2] = (int)((x2 - x1) * scale); c[3] = (int)((y2 - y1) * scale);
      }
    }

    if (memcmp(r, rect, sizeof(r)) == 0 && memcmp(c, clip, sizeof(c)) == 0)
      return;

    memcpy(rect, r, sizeof(r));
    memcpy(clip, c, sizeof(c));
    iupwebHostSetBounds(ih, r[0], r[1], r[2], r[3], c[0], c[1], c[2], c[3]);
  }
};

#else

class IupFltkWebHost : public Fl_Window
{
public:
  Ihandle* ih;

  explicit IupFltkWebHost(Ihandle* handle) : Fl_Window(0, 0, 1, 1), ih(handle)
  {
    end();
  }

  void show() override
  {
    Fl_Window::show();
    attach();
  }

  void hide() override
  {
    if (ih && shown())
      iupwebHostSetParent(ih, nullptr);
    Fl_Window::hide();
  }

  void resize(int x, int y, int w, int h) override
  {
    Fl_Window::resize(x, y, w, h);
    place();
  }

  void draw() override
  {
  }

  void attach()
  {
    HWND hwnd = (ih && shown()) ? fl_xid(this) : nullptr;
    if (!hwnd)
      return;

    SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) | WS_CLIPCHILDREN);
    iupwebHostSetParent(ih, hwnd);
    place();
  }

  void place()
  {
    HWND hwnd = (ih && shown()) ? fl_xid(this) : nullptr;
    RECT rect;

    if (!hwnd)
      return;

    GetClientRect(hwnd, &rect);
    iupwebHostSetBounds(ih, 0, 0, rect.right, rect.bottom, 0, 0, rect.right, rect.bottom);
    clip(hwnd);
  }

  void clip(HWND hwnd)
  {
    int x1 = x(), y1 = y(), x2 = x() + w(), y2 = y() + h();
    float scale = Fl::screen_scale(screen_num());

    for (Ihandle* parent = ih->parent; parent && parent->iclass->nativetype != IUP_TYPEDIALOG; parent = parent->parent)
    {
      auto* widget = reinterpret_cast<Fl_Widget*>(parent->handle);
      if (parent->iclass->nativetype == IUP_TYPEVOID || !widget)
        continue;

      x1 = x1 > widget->x() ? x1 : widget->x();
      y1 = y1 > widget->y() ? y1 : widget->y();
      x2 = x2 < widget->x() + widget->w() ? x2 : widget->x() + widget->w();
      y2 = y2 < widget->y() + widget->h() ? y2 : widget->y() + widget->h();
    }

    if (x2 < x1) x2 = x1;
    if (y2 < y1) y2 = y1;

    SetWindowRgn(hwnd, CreateRectRgn((int)((x1 - x()) * scale), (int)((y1 - y()) * scale),
                                     (int)((x2 - x()) * scale), (int)((y2 - y()) * scale)), TRUE);
  }
};

#endif

extern "C" void* iupwebHostMap(Ihandle* ih)
{
  Fl_Group::current(nullptr);
  auto* host = new IupFltkWebHost(ih);
  ih->handle = reinterpret_cast<InativeHandle*>(host);

  iupfltkAddToParent(ih);

#ifdef __APPLE__
  Fl_Window* top = host->top_window();
  if (!top || !top->shown())
    return nullptr;

  return reinterpret_cast<void*>(fl_mac_xid(top));
#else
  if (!host->shown())
    return nullptr;

  return reinterpret_cast<void*>(fl_xid(host));
#endif
}

extern "C" void iupwebHostLayoutUpdate(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  auto* host = reinterpret_cast<IupFltkWebHost*>(ih->handle);
  if (host)
    host->place();
}

extern "C" void iupwebHostUnMap(Ihandle* ih)
{
  auto* host = reinterpret_cast<IupFltkWebHost*>(ih->handle);
  if (!host)
    return;

  host->ih = nullptr;
  iupdrvBaseUnMapMethod(ih);
  ih->handle = nullptr;
}
