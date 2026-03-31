/** \file
 * \brief FLTK Driver TIPS (tooltips) management
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Tooltip.H>

#include <cstdlib>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
}

#include "iupfltk_drv.h"


static Fl_Widget* fltkTipGetWidget(Ihandle* ih)
{
  Fl_Widget* widget = (Fl_Widget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget)
    widget = (Fl_Widget*)ih->handle;
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

extern "C" IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  Fl_Widget* widget = fltkTipGetWidget(ih);
  if (!widget)
    return 0;

  if (!value || value[0] == 0)
    widget->copy_tooltip(NULL);
  else
    widget->copy_tooltip(value);

  const char* delay = iupAttribGet(ih, "TIPDELAY");
  if (delay)
  {
    int ms = 0;
    if (iupStrToInt(delay, &ms))
      Fl_Tooltip::delay((float)ms / 1000.0f);
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
    Fl_Tooltip::current(NULL);
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
