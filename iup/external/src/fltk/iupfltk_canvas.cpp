/** \file
 * \brief Canvas Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/platform.H>

#if defined(FLTK_USE_WAYLAND)
#include <FL/wayland.H>
#endif

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <climits>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_canvas.h"
#include "iup_key.h"
#include "iup_childtree.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Canvas Container - routes events to scrollbars first
 ****************************************************************************/

class IupFltkCanvasContainer : public Fl_Group
{
public:
  Ihandle* iup_handle;

  IupFltkCanvasContainer(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Group(x, y, w, h), iup_handle(ih) {}

protected:
  int handle(int event) override
  {
    if (event == FL_PUSH || event == FL_DRAG || event == FL_RELEASE ||
        event == FL_MOUSEWHEEL || event == FL_ENTER || event == FL_LEAVE ||
        event == FL_MOVE)
    {
      Fl_Scrollbar* sb_h = (Fl_Scrollbar*)iupAttribGet(iup_handle, "_IUPFLTK_SBHORIZ");
      Fl_Scrollbar* sb_v = (Fl_Scrollbar*)iupAttribGet(iup_handle, "_IUPFLTK_SBVERT");

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

    return Fl_Group::handle(event);
  }
};

/****************************************************************************
 * Custom Canvas Widget
 ****************************************************************************/

class IupFltkCanvas : public Fl_Widget
{
public:
  Ihandle* ih;
  IupFltkCanvas(int x, int y, int w, int h, Ihandle* _ih)
    : Fl_Widget(x, y, w, h), ih(_ih)
  {
  }

protected:
  void draw() override
  {
    if (!ih)
      return;

    IFn cb = (IFn)IupGetCallback(ih, "ACTION");
    if (cb && !(ih->data->inside_resize))
    {
#if defined(FLTK_USE_WAYLAND)
      if (iupfltkIsWayland())
        iupAttribSet(ih, "CAIRO_CR", (char*)fl_wl_gc());
      else
#endif
#if FLTK_USE_CAIRO
        iupAttribSet(ih, "CAIRO_CR", (char*)fl_cairo_gc());
#endif

      iupAttribSetStrf(ih, "CLIPRECT", "%d %d %d %d", 0, 0, w() - 1, h() - 1);
      cb(ih);
      iupAttribSet(ih, "CLIPRECT", NULL);

      iupAttribSet(ih, "CAIRO_CR", NULL);
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
        IFnfiis wcb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");

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
          wcb(ih, (float)delta, Fl::event_x() - x(), Fl::event_y() - y(), status);
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

          IFniff scb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
          if (scb)
          {
            int op = delta > 0 ? IUP_SBUP : IUP_SBDN;
            if (delta == 0)
              op = deltax > 0 ? IUP_SBLEFT : IUP_SBRIGHT;
            scb(ih, op, (float)ih->data->posx, (float)ih->data->posy);
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
  Ihandle* ih = (Ihandle*)data;
  Fl_Scrollbar* sb = (Fl_Scrollbar*)widget;

  double xmin = iupAttribGetDouble(ih, "XMIN");
  double xmax = iupAttribGetDouble(ih, "XMAX");
  double dx = iupAttribGetDouble(ih, "DX");
  double range = xmax - xmin - dx;

  if (range > 0)
    ih->data->posx = xmin + (sb->value() - xmin);
  else
    ih->data->posx = xmin;

  IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
  {
    scroll_cb(ih, IUP_SBPOSH, (float)ih->data->posx, (float)ih->data->posy);
  }
  else
  {
    IFn action_cb = (IFn)IupGetCallback(ih, "ACTION");
    if (action_cb)
      iupdrvRedrawNow(ih);
  }
}

static void fltkCanvasScrollVertCallback(Fl_Widget* widget, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  Fl_Scrollbar* sb = (Fl_Scrollbar*)widget;

  double ymin = iupAttribGetDouble(ih, "YMIN");
  double ymax = iupAttribGetDouble(ih, "YMAX");
  double dy = iupAttribGetDouble(ih, "DY");
  double range = ymax - ymin - dy;

  if (range > 0)
    ih->data->posy = ymin + (sb->value() - ymin);
  else
    ih->data->posy = ymin;

  IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
  {
    scroll_cb(ih, IUP_SBPOSV, (float)ih->data->posx, (float)ih->data->posy);
  }
  else
  {
    IFn action_cb = (IFn)IupGetCallback(ih, "ACTION");
    if (action_cb)
      iupdrvRedrawNow(ih);
  }
}

/****************************************************************************
 * Child Layout Update
 ****************************************************************************/

static void fltkCanvasUpdateChildLayout(Ihandle* ih)
{
  Fl_Group* sb_win = (Fl_Group*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!sb_win) return;
  Fl_Scrollbar* sb_horiz = (Fl_Scrollbar*)iupAttribGet(ih, "_IUPFLTK_SBHORIZ");
  Fl_Scrollbar* sb_vert = (Fl_Scrollbar*)iupAttribGet(ih, "_IUPFLTK_SBVERT");
  int sb_vert_width = 0, sb_horiz_height = 0;
  int width = sb_win->w();
  int height = sb_win->h();
  int border = iupAttribGetInt(ih, "_IUPFLTK_BORDER");
  int sb_size = iupdrvGetScrollbarSize();

  if (sb_vert && sb_vert->visible())
    sb_vert_width = sb_size;
  if (sb_horiz && sb_horiz->visible())
    sb_horiz_height = sb_size;

  int ox = sb_win->x();
  int oy = sb_win->y();

  if (sb_vert && sb_vert->visible())
    sb_vert->resize(ox + width - sb_vert_width - border, oy + border, sb_vert_width, height - sb_horiz_height - 2 * border);

  if (sb_horiz && sb_horiz->visible())
    sb_horiz->resize(ox + border, oy + height - sb_horiz_height - border, width - sb_vert_width - 2 * border, sb_horiz_height);

  IupFltkCanvas* canvas = (IupFltkCanvas*)ih->handle;
  if (canvas)
    canvas->resize(ox + border, oy + border, width - sb_vert_width - 2 * border, height - sb_horiz_height - 2 * border);
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

    Fl_Scrollbar* sb_horiz = (Fl_Scrollbar*)iupAttribGet(ih, "_IUPFLTK_SBHORIZ");
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
          iupAttribSet(ih, "SB_RESIZE", "YES");
          fltkCanvasUpdateChildLayout(ih);
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
        iupAttribSet(ih, "SB_RESIZE", "YES");
        fltkCanvasUpdateChildLayout(ih);
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

    Fl_Scrollbar* sb_vert = (Fl_Scrollbar*)iupAttribGet(ih, "_IUPFLTK_SBVERT");
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
          iupAttribSet(ih, "SB_RESIZE", "YES");
          fltkCanvasUpdateChildLayout(ih);
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
        iupAttribSet(ih, "SB_RESIZE", "YES");
        fltkCanvasUpdateChildLayout(ih);
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

    Fl_Scrollbar* sb_horiz = (Fl_Scrollbar*)iupAttribGet(ih, "_IUPFLTK_SBHORIZ");
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

    Fl_Scrollbar* sb_vert = (Fl_Scrollbar*)iupAttribGet(ih, "_IUPFLTK_SBVERT");
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
    IupFltkCanvas* canvas = (IupFltkCanvas*)ih->handle;
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
  IupFltkCanvas* canvas = (IupFltkCanvas*)ih->handle;
  if (canvas)
    return iupStrReturnIntInt(canvas->w(), canvas->h(), 'x');
  return NULL;
}

static char* fltkCanvasGetDrawableAttrib(Ihandle* ih)
{
  return (char*)ih->handle;
}

static char* fltkCanvasGetXDisplayAttrib(Ihandle* ih)
{
  (void)ih;
  return (char*)iupdrvGetDisplay();
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int fltkCanvasMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  ih->data->sb = iupBaseGetScrollbar(ih);

  Fl_Group::current(NULL);

  IupFltkCanvasContainer* sb_win = new IupFltkCanvasContainer(0, 0, 1, 1, ih);
  sb_win->end();
  sb_win->resizable(NULL);
  sb_win->box(FL_NO_BOX);

  sb_win->begin();

  IupFltkCanvas* canvas = new IupFltkCanvas(0, 0, 1, 1, ih);

  if (ih->data->sb & IUP_SB_HORIZ)
  {
    Fl_Scrollbar* sb_horiz = new Fl_Scrollbar(0, 0, 1, 1);
    sb_horiz->type(FL_HORIZONTAL);
    sb_horiz->hide();
    sb_horiz->when(FL_WHEN_CHANGED);
    sb_horiz->callback(fltkCanvasScrollHorizCallback, ih);
    iupAttribSet(ih, "_IUPFLTK_SBHORIZ", (char*)sb_horiz);
    iupAttribSet(ih, "XHIDDEN", "YES");
  }

  if (ih->data->sb & IUP_SB_VERT)
  {
    Fl_Scrollbar* sb_vert = new Fl_Scrollbar(0, 0, 1, 1);
    sb_vert->type(FL_VERTICAL);
    sb_vert->hide();
    sb_vert->when(FL_WHEN_CHANGED);
    sb_vert->callback(fltkCanvasScrollVertCallback, ih);
    iupAttribSet(ih, "_IUPFLTK_SBVERT", (char*)sb_vert);
    iupAttribSet(ih, "YHIDDEN", "YES");
  }

  sb_win->end();

  ih->handle = (InativeHandle*)canvas;
  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)sb_win);

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

  fltkCanvasSetDXAttrib(ih, NULL);
  fltkCanvasSetDYAttrib(ih, NULL);

  return IUP_NOERROR;
}

/****************************************************************************
 * Unmap Method
 ****************************************************************************/

static void fltkCanvasUnMapMethod(Ihandle* ih)
{
  IupFltkCanvas* canvas = (IupFltkCanvas*)ih->handle;
  if (canvas)
    canvas->ih = NULL;

  iupdrvBaseUnMapMethod(ih);
}

/****************************************************************************
 * Layout Update
 ****************************************************************************/

static void fltkCanvasLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  fltkCanvasUpdateChildLayout(ih);

  IupFltkCanvas* canvas = (IupFltkCanvas*)ih->handle;
  if (canvas)
  {
    IFnii cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
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
  Fl_Group* extra_parent = (Fl_Group*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (extra_parent)
    return (void*)extra_parent;
  return ih->handle;
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

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkCanvasSetBgColorAttrib, "255 255 255", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "DRAWSIZE", fltkCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAWABLE", fltkCanvasGetDrawableAttrib, NULL, NULL, NULL, IUPAF_NO_STRING | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, iupfltkGetNativeWindowHandleName(), iupfltkGetNativeWindowHandleAttrib, NULL, NULL, NULL, IUPAF_NO_STRING | IUPAF_NO_INHERIT);
  if (iupdrvGetDisplay())
    iupClassRegisterAttribute(ic, "XDISPLAY", fltkCanvasGetXDisplayAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "CAIRO_CR", NULL, NULL, NULL, NULL, IUPAF_NO_STRING);

  iupClassRegisterAttribute(ic, "DX", NULL, fltkCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", NULL, fltkCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, fltkCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, fltkCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XMIN", NULL, NULL, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XMAX", NULL, NULL, "1", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMIN", NULL, NULL, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YMAX", NULL, NULL, "1", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEX", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XAUTOHIDE", NULL, NULL, "YES", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YAUTOHIDE", NULL, NULL, "YES", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XHIDDEN", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YHIDDEN", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, "YES", NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
