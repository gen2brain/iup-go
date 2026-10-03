/** \file
 * \brief Canvas Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>
#include <FL/platform.H>

#include <cstring>
#include <climits>
#include <cstdlib>
#include <vector>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_canvas.h"
#include "iup_key.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Canvas Container - routes events to scrollbars first
 ****************************************************************************/

template <class Base>
class IupFltkCanvasContainer : public Base
{
public:
  Ihandle* iup_handle;

  IupFltkCanvasContainer(int x, int y, int w, int h, Ihandle* ih)
    : Base(x, y, w, h), iup_handle(ih) {}

protected:
  void draw() override
  {
    auto* canvas = reinterpret_cast<Fl_Widget*>(iup_handle->handle);
    if (canvas)
    {
      auto* sb_h = reinterpret_cast<Fl_Scrollbar*>(iupAttribGet(iup_handle, "_IUPFLTK_SBHORIZ"));
      auto* sb_v = reinterpret_cast<Fl_Scrollbar*>(iupAttribGet(iup_handle, "_IUPFLTK_SBVERT"));
      int overdraw = canvas->damage() && !(this->damage() & ~FL_DAMAGE_CHILD);
      std::vector<Fl_Widget*> covered;

      if (overdraw)
      {
        for (int i = 0; i < this->children(); i++)
        {
          Fl_Widget* child = this->child(i);
          if (child != canvas && child != sb_h && child != sb_v && child->visible() && !child->damage())
            covered.push_back(child);
        }
      }

      fl_push_clip(canvas->x(), canvas->y(), canvas->w(), canvas->h());
      Base::draw();
      for (Fl_Widget* child : covered)
        this->draw_child(*child);
      fl_pop_clip();

      if (sb_h && sb_h->visible()) this->draw_child(*sb_h);
      if (sb_v && sb_v->visible()) this->draw_child(*sb_v);
    }
    else
      Base::draw();
  }

public:
  int handle(int event) override
  {
    if (event == FL_PUSH || event == FL_DRAG || event == FL_RELEASE ||
        event == FL_MOUSEWHEEL || event == FL_ENTER || event == FL_LEAVE ||
        event == FL_MOVE)
    {
      auto* sb_h = reinterpret_cast<Fl_Scrollbar*>(iupAttribGet(iup_handle, "_IUPFLTK_SBHORIZ"));
      auto* sb_v = reinterpret_cast<Fl_Scrollbar*>(iupAttribGet(iup_handle, "_IUPFLTK_SBVERT"));

      int ex = Fl::event_x();
      int ey = Fl::event_y();

      if (sb_v && sb_v->visible() &&
          ex >= sb_v->x() && ex < sb_v->x() + sb_v->w() &&
          ey >= sb_v->y() && ey < sb_v->y() + sb_v->h())
      {
        return sb_v->handle(event);
      }

      if (sb_h && sb_h->visible() &&
          ex >= sb_h->x() && ex < sb_h->x() + sb_h->w() &&
          ey >= sb_h->y() && ey < sb_h->y() + sb_h->h())
      {
        return sb_h->handle(event);
      }
    }

    return Base::handle(event);
  }
};

/****************************************************************************
 * Custom Canvas Widget
 ****************************************************************************/

class IupFltkCanvas : public Fl_Widget
{
public:
  Ihandle* ih;
  int in_draw;
  int blit_pending;
  IupFltkCanvas(int x, int y, int w, int h, Ihandle* _ih)
    : Fl_Widget(x, y, w, h), ih(_ih), in_draw(0), blit_pending(0)
  {
  }

  void draw() override
  {
    if (!ih)
      return;

    if (blit_pending)
    {
      auto offscreen = static_cast<Fl_Offscreen>(reinterpret_cast<size_t>(iupAttribGet(ih, "_IUP_FLTK_OFFSCREEN")));
      blit_pending = 0;
      if (offscreen && iupAttribGetInt(ih, "_IUP_FLTK_OFFSCREEN_W") == w() && iupAttribGetInt(ih, "_IUP_FLTK_OFFSCREEN_H") == h())
      {
        fl_copy_offscreen(x(), y(), w(), h(), offscreen, 0, 0);
        return;
      }
    }

    if (iupAttribGet(ih, "_IUPGL_COMPOSITE"))
    {
      IFn glcb = static_cast<IFn>(IupGetCallback(ih, "ACTION"));
      iupAttribSet(ih, "_IUPGL_IN_DRAW", "1");
      if (glcb && !(ih->data->inside_resize))
      {
        in_draw = 1;
        glcb(ih);
        in_draw = 0;
      }
      iupAttribSet(ih, "_IUPGL_IN_DRAW", nullptr);

      auto* px = reinterpret_cast<unsigned char*>(iupAttribGet(ih, "_IUPGL_COMPOSITE_PIXELS"));
      int pw = iupAttribGetInt(ih, "_IUPGL_COMPOSITE_W");
      int ph = iupAttribGetInt(ih, "_IUPGL_COMPOSITE_H");
      if (px && pw > 0 && ph > 0)
      {
        auto* rgb = static_cast<unsigned char*>(malloc(static_cast<size_t>(pw) * ph * 3));
        if (rgb)
        {
          size_t i, n = static_cast<size_t>(pw) * ph;
          for (i = 0; i < n; i++)   /* BGRA (top-left) -> RGB for fl_draw_image */
          {
            rgb[i*3+0] = px[i*4+2];
            rgb[i*3+1] = px[i*4+1];
            rgb[i*3+2] = px[i*4+0];
          }
          fl_draw_image(rgb, x(), y(), pw, ph, 3, 0);
          free(rgb);
        }
      }
      return;
    }

    IFn cb = static_cast<IFn>(IupGetCallback(ih, "ACTION"));
    if (cb && !(ih->data->inside_resize))
    {
#if defined(FLTK_USE_WAYLAND)
      if (iupfltkIsWayland())
        iupAttribSet(ih, "CAIRO_CR", reinterpret_cast<char*>(fl_wl_gc()));
      else
#endif
#if FLTK_USE_CAIRO
        iupAttribSet(ih, "CAIRO_CR", reinterpret_cast<char*>(fl_cairo_gc()));
#endif

      int cx, cy, cw, ch;
      fl_clip_box(x(), y(), w(), h(), cx, cy, cw, ch);
      if (cw <= 0 || ch <= 0)
      {
        cx = x(); cy = y(); cw = w(); ch = h();
      }
      iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", cx - x(), cy - y(), cx - x() + cw - 1, cy - y() + ch - 1);
      in_draw = 1;
      cb(ih);
      in_draw = 0;
      iupAttribSet(ih, "CLIPRECT", nullptr);

      iupAttribSet(ih, "CAIRO_CR", nullptr);
    }
    else
    {
      unsigned char r = 255, g = 255, b = 255;
      const char* bgcolor = iupAttribGetStr(ih, "BGCOLOR");
      if (bgcolor)
        iupStrToRGB(bgcolor, &r, &g, &b);
      fl_color(fl_rgb_color(r, g, b));
      fl_rectf(x(), y(), w(), h());
    }
  }

public:
  int handle(int event) override
  {
    if (!ih)
      return Fl_Widget::handle(event);

    switch (event)
    {
      case FL_DND_ENTER: case FL_DND_DRAG: case FL_DND_LEAVE: case FL_DND_RELEASE:
        if (iupfltkDragDropHandleEvent(this, ih, event))
          return 1;
        break;

      case FL_PASTE:
        if (iupfltkDragDropHandleEvent(this, ih, event))
          return 1;
        break;

      case FL_PUSH:
      case FL_RELEASE:
      {
        if (event == FL_PUSH && iupAttribGetBoolean(ih, "CANFOCUS"))
          Fl::focus(this);

        iupfltkDragDropHandleEvent(this, ih, event);
        iupfltkMouseButtonEvent(this, ih, event);
        return 1;
      }

      case FL_DRAG:
      case FL_MOVE:
      {
        if (event == FL_DRAG && iupfltkDragDropHandleEvent(this, ih, event))
          return 1;
        iupfltkMouseMoveEvent(this, ih);
        return 1;
      }

      case FL_ENTER:
      case FL_LEAVE:
      {
        iupfltkEnterLeaveEvent(this, ih, event);
        return 1;
      }

      case FL_FOCUS:
      case FL_UNFOCUS:
      {
        iupfltkFocusInOutEvent(this, ih, event);
        return 1;
      }

      case FL_MOUSEWHEEL:
      {
        auto wcb = reinterpret_cast<IFnfiis>(IupGetCallback(ih, "WHEEL_CB"));

        if (iupAttribGetBoolean(ih, "WHEELDROPFOCUS"))
        {
          Ihandle* ih_focus = IupGetFocus();
          if (iupObjectCheck(ih_focus))
            iupAttribSetClassObject(ih_focus, "SHOWDROPDOWN", "NO");
        }

        if (wcb)
        {
          int delta = -Fl::event_dy();
          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupfltkButtonKeySetStatus(Fl::event_state(), 0, status, 0);
          wcb(ih, static_cast<float>(delta), Fl::event_x() - x(), Fl::event_y() - y(), status);
        }
        else
        {
          int delta = -Fl::event_dy();
          int deltax = -Fl::event_dx();

          if (delta != 0)
          {
            double posy = ih->data->posy;
            posy -= delta * iupAttribGetDouble(ih, "DY") / 10.0;
            IupSetDouble(ih, "POSY", posy);
          }
          else if (deltax != 0)
          {
            double posx = ih->data->posx;
            posx -= deltax * iupAttribGetDouble(ih, "DX") / 10.0;
            IupSetDouble(ih, "POSX", posx);
          }

          auto scb = reinterpret_cast<IFniff>(IupGetCallback(ih, "SCROLL_CB"));
          if (scb)
          {
            int op = delta > 0 ? IUP_SBUP : IUP_SBDN;
            if (delta == 0)
              op = deltax > 0 ? IUP_SBLEFT : IUP_SBRIGHT;
            scb(ih, op, static_cast<float>(ih->data->posx), static_cast<float>(ih->data->posy));
          }
        }
        return 1;
      }

      case FL_KEYBOARD:
      {
        if (iupfltkKeyPressEvent(this, ih))
          return 1;
        break;
      }

      case FL_KEYUP:
      {
        if (iupfltkKeyReleaseEvent(this, ih))
          return 1;
        break;
      }
    }

    return Fl_Widget::handle(event);
  }
};

/****************************************************************************
 * Scrollbar Callbacks
 ****************************************************************************/

static void fltkCanvasScrollHorizCallback(Fl_Widget* widget, void* data)
{
  auto* ih = static_cast<Ihandle*>(data);
  auto* sb = static_cast<Fl_Scrollbar*>(widget);

  double xmin = iupAttribGetDouble(ih, "XMIN");
  double xmax = iupAttribGetDouble(ih, "XMAX");
  double dx = iupAttribGetDouble(ih, "DX");
  double range = xmax - xmin - dx;

  if (range > 0)
    ih->data->posx = xmin + (sb->value() - xmin);
  else
    ih->data->posx = xmin;

  auto scroll_cb = reinterpret_cast<IFniff>(IupGetCallback(ih, "SCROLL_CB"));
  if (scroll_cb)
  {
    scroll_cb(ih, IUP_SBPOSH, static_cast<float>(ih->data->posx), static_cast<float>(ih->data->posy));
  }
  else
  {
    IFn action_cb = static_cast<IFn>(IupGetCallback(ih, "ACTION"));
    if (action_cb)
      iupdrvRedrawNow(ih);
  }
}

static void fltkCanvasScrollVertCallback(Fl_Widget* widget, void* data)
{
  auto* ih = static_cast<Ihandle*>(data);
  auto* sb = static_cast<Fl_Scrollbar*>(widget);

  double ymin = iupAttribGetDouble(ih, "YMIN");
  double ymax = iupAttribGetDouble(ih, "YMAX");
  double dy = iupAttribGetDouble(ih, "DY");
  double range = ymax - ymin - dy;

  if (range > 0)
    ih->data->posy = ymin + (sb->value() - ymin);
  else
    ih->data->posy = ymin;

  auto scroll_cb = reinterpret_cast<IFniff>(IupGetCallback(ih, "SCROLL_CB"));
  if (scroll_cb)
  {
    scroll_cb(ih, IUP_SBPOSV, static_cast<float>(ih->data->posx), static_cast<float>(ih->data->posy));
  }
  else
  {
    IFn action_cb = static_cast<IFn>(IupGetCallback(ih, "ACTION"));
    if (action_cb)
      iupdrvRedrawNow(ih);
  }
}

/****************************************************************************
 * Child Layout Update
 ****************************************************************************/

static void fltkCanvasUpdateChildLayout(Ihandle* ih)
{
  auto* sb_win = reinterpret_cast<Fl_Group*>(iupAttribGet(ih, "_IUP_EXTRAPARENT"));
  if (!sb_win) return;
  auto* sb_horiz = reinterpret_cast<Fl_Scrollbar*>(iupAttribGet(ih, "_IUPFLTK_SBHORIZ"));
  auto* sb_vert = reinterpret_cast<Fl_Scrollbar*>(iupAttribGet(ih, "_IUPFLTK_SBVERT"));
  int sb_vert_width = 0, sb_horiz_height = 0;
  int width = sb_win->w();
  int height = sb_win->h();
  int border = iupAttribGetInt(ih, "_IUPFLTK_BORDER");
  int sb_size = iupdrvGetScrollbarSize();

  if (sb_vert && sb_vert->visible())
    sb_vert_width = sb_size;
  if (sb_horiz && sb_horiz->visible())
    sb_horiz_height = sb_size;

  int ox = sb_win->as_window() ? 0 : sb_win->x();
  int oy = sb_win->as_window() ? 0 : sb_win->y();

  if (sb_vert && sb_vert->visible())
    sb_vert->resize(ox + width - sb_vert_width - border, oy + border, sb_vert_width, height - sb_horiz_height - 2 * border);

  if (sb_horiz && sb_horiz->visible())
    sb_horiz->resize(ox + border, oy + height - sb_horiz_height - border, width - sb_vert_width - 2 * border, sb_horiz_height);

  auto* canvas = reinterpret_cast<IupFltkCanvas*>(ih->handle);
  if (canvas)
    canvas->resize(ox + border, oy + border, width - sb_vert_width - 2 * border, height - sb_horiz_height - 2 * border);
}

static void fltkCanvasDeferredResize(void* data)
{
  auto* ih = static_cast<Ihandle*>(data);
  IupFltkCanvas* canvas;

  if (!iupObjectCheck(ih) || !ih->handle)
    return;

  canvas = reinterpret_cast<IupFltkCanvas*>(ih->handle);

  auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "RESIZE_CB"));
  if (cb && !ih->data->inside_resize)
  {
    ih->data->inside_resize = 1;
    cb(ih, canvas->w(), canvas->h());
    ih->data->inside_resize = 0;
  }

  canvas->redraw();
}

static void fltkCanvasScrollbarToggled(Ihandle* ih)
{
  iupAttribSet(ih, "SB_RESIZE", "YES");
  fltkCanvasUpdateChildLayout(ih);
  Fl::remove_timeout(fltkCanvasDeferredResize, reinterpret_cast<void*>(ih));
  Fl::add_timeout(0.0, fltkCanvasDeferredResize, reinterpret_cast<void*>(ih));
}

/****************************************************************************
 * Scrollbar DX/DY Attributes
 ****************************************************************************/

static int fltkCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    double dx;
    if (!iupStrToDoubleDef(value, &dx, 0.1))
      return 1;

    iupAttribSet(ih, "SB_RESIZE", nullptr);

    auto* sb_horiz = reinterpret_cast<Fl_Scrollbar*>(iupAttribGet(ih, "_IUPFLTK_SBHORIZ"));
    if (!sb_horiz)
      return 1;

    double xmin = iupAttribGetDouble(ih, "XMIN");
    double xmax = iupAttribGetDouble(ih, "XMAX");
    double linex = iupAttribGetDouble(ih, "LINEX");
    if (linex == 0) linex = dx / 10.0;
    if (linex == 0) linex = 1;

    if (dx >= (xmax - xmin))
    {
      if (iupAttribGetBoolean(ih, "XAUTOHIDE"))
      {
        if (sb_horiz->visible())
        {
          sb_horiz->hide();
          fltkCanvasScrollbarToggled(ih);
        }
        iupAttribSet(ih, "XHIDDEN", "YES");
      }
      else
      {
        sb_horiz->deactivate();
      }
      ih->data->posx = xmin;
    }
    else
    {
      if (!sb_horiz->visible())
      {
        sb_horiz->show();
        fltkCanvasScrollbarToggled(ih);
      }
      sb_horiz->activate();

      sb_horiz->value(ih->data->posx, dx, xmin, xmax);
      sb_horiz->linesize(linex);

      iupAttribSet(ih, "XHIDDEN", "NO");
    }
  }
  return 1;
}

static int fltkCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    double dy;
    if (!iupStrToDoubleDef(value, &dy, 0.1))
      return 1;

    iupAttribSet(ih, "SB_RESIZE", nullptr);

    auto* sb_vert = reinterpret_cast<Fl_Scrollbar*>(iupAttribGet(ih, "_IUPFLTK_SBVERT"));
    if (!sb_vert)
      return 1;

    double ymin = iupAttribGetDouble(ih, "YMIN");
    double ymax = iupAttribGetDouble(ih, "YMAX");
    double liney = iupAttribGetDouble(ih, "LINEY");
    if (liney == 0) liney = dy / 10.0;
    if (liney == 0) liney = 1;

    if (dy >= (ymax - ymin))
    {
      if (iupAttribGetBoolean(ih, "YAUTOHIDE"))
      {
        if (sb_vert->visible())
        {
          sb_vert->hide();
          fltkCanvasScrollbarToggled(ih);
        }
        iupAttribSet(ih, "YHIDDEN", "YES");
      }
      else
      {
        sb_vert->deactivate();
      }
      ih->data->posy = ymin;
    }
    else
    {
      if (!sb_vert->visible())
      {
        sb_vert->show();
        fltkCanvasScrollbarToggled(ih);
      }
      sb_vert->activate();

      sb_vert->value(ih->data->posy, dy, ymin, ymax);
      sb_vert->linesize(liney);

      iupAttribSet(ih, "YHIDDEN", "NO");
    }
  }
  return 1;
}

/****************************************************************************
 * POSX/POSY Attributes
 ****************************************************************************/

static int fltkCanvasSetPosXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    double posx;
    if (!iupStrToDouble(value, &posx))
      return 1;

    double xmin = iupAttribGetDouble(ih, "XMIN");
    double xmax = iupAttribGetDouble(ih, "XMAX");
    double dx = iupAttribGetDouble(ih, "DX");

    if (dx >= xmax - xmin)
      return 0;

    if (posx < xmin) posx = xmin;
    if (posx > (xmax - dx)) posx = xmax - dx;
    ih->data->posx = posx;

    auto* sb_horiz = reinterpret_cast<Fl_Scrollbar*>(iupAttribGet(ih, "_IUPFLTK_SBHORIZ"));
    if (sb_horiz)
    {
      sb_horiz->value(posx, dx, xmin, xmax);
      sb_horiz->redraw();
    }
  }
  return 1;
}

static int fltkCanvasSetPosYAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    double posy;
    if (!iupStrToDouble(value, &posy))
      return 1;

    double ymin = iupAttribGetDouble(ih, "YMIN");
    double ymax = iupAttribGetDouble(ih, "YMAX");
    double dy = iupAttribGetDouble(ih, "DY");

    if (dy >= ymax - ymin)
      return 0;

    if (posy < ymin) posy = ymin;
    if (posy > (ymax - dy)) posy = ymax - dy;
    ih->data->posy = posy;

    auto* sb_vert = reinterpret_cast<Fl_Scrollbar*>(iupAttribGet(ih, "_IUPFLTK_SBVERT"));
    if (sb_vert)
    {
      sb_vert->value(posy, dy, ymin, ymax);
      sb_vert->redraw();
    }
  }
  return 1;
}

/****************************************************************************
 * Other Attributes
 ****************************************************************************/

static int fltkCanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    auto* canvas = reinterpret_cast<IupFltkCanvas*>(ih->handle);
    if (canvas)
    {
      canvas->color(fl_rgb_color(r, g, b));
      canvas->redraw();
    }
    return 1;
  }
  return 0;
}

static char* fltkCanvasGetDrawSizeAttrib(Ihandle* ih)
{
  auto* canvas = reinterpret_cast<IupFltkCanvas*>(ih->handle);
  if (canvas)
    return iupStrReturnIntInt(canvas->w(), canvas->h(), 'x');
  return nullptr;
}

static char* fltkCanvasGetDrawableAttrib(Ihandle* ih)
{
  return reinterpret_cast<char*>(ih->handle);
}

static char* fltkCanvasGetXDisplayAttrib(Ihandle* ih)
{
  (void)ih;
  return static_cast<char*>(iupdrvGetDisplay());
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

IUP_DRV_API int iupfltkCanvasDeferBlit(Ihandle* ih)
{
  auto* canvas = reinterpret_cast<IupFltkCanvas*>(iupAttribGet(ih, "_IUPFLTK_CANVAS"));
  if (!canvas || canvas != reinterpret_cast<IupFltkCanvas*>(ih->handle) || canvas->in_draw)
    return 0;

  canvas->blit_pending = 1;
  canvas->redraw();
  return 1;
}

static int fltkCanvasMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  ih->data->sb = iupBaseGetScrollbar(ih);

  Fl_Group::current(nullptr);

  Fl_Group* sb_win;
  if (iupfltkIsX11() && IupClassMatch(ih, "glcanvas") && !IupClassMatch(ih, "glbackgroundbox"))
    sb_win = new IupFltkCanvasContainer<Fl_Window>(0, 0, 1, 1, ih);
  else
    sb_win = new IupFltkCanvasContainer<Fl_Group>(0, 0, 1, 1, ih);
  sb_win->end();
  sb_win->resizable(nullptr);
  sb_win->box(FL_NO_BOX);

  sb_win->begin();

  auto* canvas = new IupFltkCanvas(0, 0, 1, 1, ih);

  if (ih->data->sb & IUP_SB_HORIZ)
  {
    auto* sb_horiz = new Fl_Scrollbar(0, 0, 1, 1);
    sb_horiz->type(FL_HORIZONTAL);
    sb_horiz->hide();
    sb_horiz->when(FL_WHEN_CHANGED);
    sb_horiz->callback(fltkCanvasScrollHorizCallback, ih);
    iupAttribSet(ih, "_IUPFLTK_SBHORIZ", reinterpret_cast<char*>(sb_horiz));
    iupAttribSet(ih, "XHIDDEN", "YES");
  }

  if (ih->data->sb & IUP_SB_VERT)
  {
    auto* sb_vert = new Fl_Scrollbar(0, 0, 1, 1);
    sb_vert->type(FL_VERTICAL);
    sb_vert->hide();
    sb_vert->when(FL_WHEN_CHANGED);
    sb_vert->callback(fltkCanvasScrollVertCallback, ih);
    iupAttribSet(ih, "_IUPFLTK_SBVERT", reinterpret_cast<char*>(sb_vert));
    iupAttribSet(ih, "YHIDDEN", "YES");
  }

  sb_win->end();

  ih->handle = reinterpret_cast<InativeHandle*>(canvas);
  iupAttribSet(ih, "_IUP_EXTRAPARENT", reinterpret_cast<char*>(sb_win));
  iupAttribSet(ih, "_IUPFLTK_CANVAS", reinterpret_cast<char*>(canvas));

  if (iupAttribGetBoolean(ih, "BORDER"))
  {
    sb_win->box(FL_DOWN_FRAME);
    iupAttribSetInt(ih, "_IUPFLTK_BORDER", Fl::box_dx(FL_DOWN_FRAME));
  }

  if (ih->iclass->is_interactive)
  {
    if (iupAttribGetBoolean(ih, "CANFOCUS"))
      iupfltkSetCanFocus(canvas, 1);
  }

  iupfltkAddToParent(ih);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  fltkCanvasSetDXAttrib(ih, nullptr);
  fltkCanvasSetDYAttrib(ih, nullptr);

  return IUP_NOERROR;
}

/****************************************************************************
 * Unmap Method
 ****************************************************************************/

static void fltkCanvasUnMapMethod(Ihandle* ih)
{
  auto* canvas = reinterpret_cast<IupFltkCanvas*>(ih->handle);

  Fl::remove_timeout(fltkCanvasDeferredResize, reinterpret_cast<void*>(ih));
  if (canvas)
    canvas->ih = nullptr;
  iupAttribSet(ih, "_IUPFLTK_CANVAS", nullptr);

  {
    auto offscreen = static_cast<Fl_Offscreen>(reinterpret_cast<size_t>(iupAttribGet(ih, "_IUP_FLTK_OFFSCREEN")));
    if (offscreen)
    {
      fl_delete_offscreen(offscreen);
      iupAttribSet(ih, "_IUP_FLTK_OFFSCREEN", nullptr);
    }
  }

  iupdrvBaseUnMapMethod(ih);
}

/****************************************************************************
 * Layout Update
 ****************************************************************************/

static void fltkCanvasLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  fltkCanvasUpdateChildLayout(ih);

  auto* canvas = reinterpret_cast<IupFltkCanvas*>(ih->handle);
  if (canvas)
  {
    auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "RESIZE_CB"));
    if (cb && !ih->data->inside_resize)
    {
      ih->data->inside_resize = 1;
      cb(ih, canvas->w(), canvas->h());
      ih->data->inside_resize = 0;
    }
  }
}

/****************************************************************************
 * Inner Container Handle
 ****************************************************************************/

static void* fltkCanvasGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  auto* extra_parent = reinterpret_cast<Fl_Group*>(iupAttribGet(ih, "_IUP_EXTRAPARENT"));
  if (extra_parent)
    return reinterpret_cast<void*>(extra_parent);
  return ih->handle;
}

static int fltkCanvasSetUpdateRectAttrib(Ihandle* ih, const char* value)
{
  auto* widget = reinterpret_cast<Fl_Widget*>(ih->handle);
  int x1, y1, x2, y2;
  if (widget && value && iupStrToRect(value, &x1, &y1, &x2, &y2))
    widget->damage(FL_DAMAGE_ALL, widget->x() + x1, widget->y() + y1, x2 - x1 + 1, y2 - y1 + 1);
  else
    iupdrvPostRedraw(ih);
  return 0;
}

/****************************************************************************
 * Canvas Driver Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvCanvasInitClass(Iclass* ic)
{
  ic->Map = fltkCanvasMapMethod;
  ic->UnMap = fltkCanvasUnMapMethod;
  ic->LayoutUpdate = fltkCanvasLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = fltkCanvasGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, fltkCanvasSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "DRAWSIZE", fltkCanvasGetDrawSizeAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAWABLE", fltkCanvasGetDrawableAttrib, nullptr, nullptr, nullptr, IUPAF_NO_STRING | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, iupfltkGetNativeWindowHandleName(), iupfltkGetNativeWindowHandleAttrib, nullptr, nullptr, nullptr, IUPAF_NO_STRING | IUPAF_NO_INHERIT);
  if (iupdrvGetDisplay())
    iupClassRegisterAttribute(ic, "XDISPLAY", fltkCanvasGetXDisplayAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "CAIRO_CR", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "UPDATERECT", nullptr, fltkCanvasSetUpdateRectAttrib, nullptr, nullptr, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DX", nullptr, fltkCanvasSetDXAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", nullptr, fltkCanvasSetDYAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, fltkCanvasSetPosXAttrib, "0", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, fltkCanvasSetPosYAttrib, "0", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XMIN", nullptr, nullptr, "0", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XMAX", nullptr, nullptr, "1", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMIN", nullptr, nullptr, "0", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMAX", nullptr, nullptr, "1", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEX", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEY", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XAUTOHIDE", nullptr, nullptr, "YES", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YAUTOHIDE", nullptr, nullptr, "YES", nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XHIDDEN", nullptr, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YHIDDEN", nullptr, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKINGSTORE", nullptr, nullptr, "YES", nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOUCH", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
