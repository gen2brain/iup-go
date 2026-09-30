/** \file
 * \brief FLTK Driver TIPS (tooltips) management
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Tooltip.H>

#include <cstdio>
#include <cstdlib>
#include <map>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_attrib.h"
}

#include "iupfltk_drv.h"


static Fl_Widget* fltkTipGetWidget(Ihandle* ih)
{
  auto* widget = reinterpret_cast<Fl_Widget*>(iupAttribGet(ih, "_IUP_EXTRAPARENT"));
  if (!widget)
    widget = reinterpret_cast<Fl_Widget*>(ih->handle);
  return widget;
}

static void fltkTipUpdateStyle(Ihandle* ih)
{
  const char* value;
  unsigned char r, g, b;

  value = iupAttribGet(ih, "TIPFONT");
  if (value)
  {
    int fl_font_id, fl_size;
    if (iupfltkGetFont(ih, &fl_font_id, &fl_size))
    {
      Fl_Tooltip::font(fl_font_id);
      Fl_Tooltip::size(fl_size);
    }
  }

  value = iupAttribGet(ih, "TIPBGCOLOR");
  if (value && iupStrToRGB(value, &r, &g, &b))
    Fl_Tooltip::color(fl_rgb_color(r, g, b));

  value = iupAttribGet(ih, "TIPFGCOLOR");
  if (value && iupStrToRGB(value, &r, &g, &b))
    Fl_Tooltip::textcolor(fl_rgb_color(r, g, b));
}

static std::map<Fl_Widget*, Ihandle*> fltk_tip_owners;

static void fltkTipUpdateRect()
{
  Fl_Widget* widget = Fl::belowmouse();
  while (widget && !widget->tooltip())
    widget = widget->parent();
  if (!widget)
    return;

  auto found = fltk_tip_owners.find(widget);
  if (found == fltk_tip_owners.end())
    return;

  Ihandle* ih = found->second;
  if (!iupObjectCheck(ih) || fltkTipGetWidget(ih) != widget)
    return;

  int x1, y1, x2, y2;
  char* rect = iupAttribGet(ih, "TIPRECT");
  if (!rect || !iupStrToRect(rect, &x1, &y1, &x2, &y2))
    return;

  int x = Fl::event_x() - widget->x();
  int y = Fl::event_y() - widget->y();
  if (x >= x1 && x <= x2 && y >= y1 && y <= y2)
    Fl_Tooltip::enter_area(widget, x1, y1, x2 - x1 + 1, y2 - y1 + 1, widget->tooltip());
  else
    Fl_Tooltip::enter_area(widget, 0, 0, 0, 0, nullptr);
}

static int fltkTipDispatch(int event, Fl_Window* window)
{
  int ret = Fl::handle_(event, window);
  if (event == FL_MOVE || event == FL_ENTER)
    fltkTipUpdateRect();
  return ret;
}

IUP_DRV_API void iupfltkTipsRemove(Ihandle* ih)
{
  auto found = fltk_tip_owners.find(fltkTipGetWidget(ih));
  if (found != fltk_tip_owners.end() && found->second == ih)
    fltk_tip_owners.erase(found);
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  Fl_Widget* widget = fltkTipGetWidget(ih);
  if (!widget)
    return 0;

  if (!value || value[0] == 0)
  {
    widget->copy_tooltip(nullptr);
    fltk_tip_owners.erase(widget);
  }
  else
  {
    widget->copy_tooltip(value);
    fltk_tip_owners[widget] = ih;
    if (!Fl::event_dispatch())
      Fl::event_dispatch(fltkTipDispatch);
  }

  const char* delay = iupAttribGet(ih, "TIPDELAY");
  if (delay)
  {
    int ms = 0;
    if (iupStrToInt(delay, &ms))
      Fl_Tooltip::delay(static_cast<float>(ms) / 1000.0f);
  }

  fltkTipUpdateStyle(ih);

  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  Fl_Widget* widget = fltkTipGetWidget(ih);
  if (!widget)
    return 0;

  if (iupStrBoolean(value))
  {
    const char* tip = iupAttribGet(ih, "TIP");
    if (tip)
    {
      fltkTipUpdateStyle(ih);
      Fl_Tooltip::current(widget);
    }
  }
  else
  {
    Fl_Tooltip::current(nullptr);
  }

  return 0;
}

extern "C" IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  Fl_Widget* widget = fltkTipGetWidget(ih);
  if (!widget)
    return iupStrReturnBoolean(0);

  return iupStrReturnBoolean(Fl_Tooltip::current() == widget ? 1 : 0);
}
