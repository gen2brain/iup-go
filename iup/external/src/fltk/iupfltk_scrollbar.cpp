/** \file
 * \brief Scrollbar Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_scrollbar.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
}

#include "iupfltk_drv.h"


#define ISCROLLBAR_RANGE 10000

class IupFltkScrollbar : public Fl_Scrollbar
{
public:
  Ihandle* iup_handle;
  int last_op;

  IupFltkScrollbar(int x, int y, int w, int h, Ihandle* ih, int horizontal)
    : Fl_Scrollbar(x, y, w, h), iup_handle(ih), last_op(-1)
  {
    if (horizontal)
      type(FL_HORIZONTAL);
    else
      type(FL_VERTICAL);
  }

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS:
      case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event);
        break;
      case FL_ENTER:
      case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event);
        break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle))
          return 1;
        break;
      case FL_PUSH:
      {
        int is_horiz = (iup_handle->data->orientation == ISCROLLBAR_HORIZONTAL);
        int pos = is_horiz ? (Fl::event_x() - x()) : (Fl::event_y() - y());
        int sz = is_horiz ? w() : h();
        int arrow = is_horiz ? h() : w();

        if (pos < arrow)
          last_op = is_horiz ? IUP_SBLEFT : IUP_SBUP;
        else if (pos >= sz - arrow)
          last_op = is_horiz ? IUP_SBRIGHT : IUP_SBDN;
        else
        {
          double val = (maximum() - minimum()) ? (value() - minimum()) / (maximum() - minimum()) : 0.5;
          int track = sz - 2 * arrow;
          int ss = (int)(slider_size() * track + 0.5);
          int sliderpos = arrow + (int)(val * (track - ss) + 0.5);
          if (pos < sliderpos)
            last_op = is_horiz ? IUP_SBPGLEFT : IUP_SBPGUP;
          else if (pos >= sliderpos + ss)
            last_op = is_horiz ? IUP_SBPGRIGHT : IUP_SBPGDN;
          else
            last_op = is_horiz ? IUP_SBDRAGH : IUP_SBDRAGV;
        }
        break;
      }
      case FL_RELEASE:
        if (last_op == IUP_SBDRAGH || last_op == IUP_SBDRAGV)
          last_op = (iup_handle->data->orientation == ISCROLLBAR_HORIZONTAL) ? IUP_SBPOSH : IUP_SBPOSV;
        break;
    }
    return Fl_Scrollbar::handle(event);
  }
};

static void fltkScrollbarUpdateNative(Ihandle* ih)
{
  IupFltkScrollbar* sb = (IupFltkScrollbar*)ih->handle;
  if (!sb)
    return;

  double range = ih->data->vmax - ih->data->vmin;
  if (range <= 0)
    return;

  double fval = (ih->data->val - ih->data->vmin) / range;
  double fpage = ih->data->pagesize / range;

  sb->value(fval * ISCROLLBAR_RANGE, (int)(fpage * ISCROLLBAR_RANGE),
            0, ISCROLLBAR_RANGE);

  int istep = (int)((ih->data->linestep / range) * ISCROLLBAR_RANGE);
  if (istep < 1) istep = 1;
  sb->linesize(istep);
}

static void fltkScrollbarCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  IupFltkScrollbar* sb = (IupFltkScrollbar*)w;

  double range = ih->data->vmax - ih->data->vmin;
  double old_val = ih->data->val;

  if (ISCROLLBAR_RANGE > 0)
    ih->data->val = ((double)sb->value() / (double)ISCROLLBAR_RANGE) * range + ih->data->vmin;
  else
    ih->data->val = ih->data->vmin;

  iupScrollbarCropValue(ih);

  IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
  if (scroll_cb)
  {
    int op = sb->last_op;
    if (op < 0)
      op = (ih->data->orientation == ISCROLLBAR_HORIZONTAL) ? IUP_SBPOSH : IUP_SBPOSV;

    float posx = 0, posy = 0;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      posx = (float)ih->data->val;
    else
      posy = (float)ih->data->val;

    scroll_cb(ih, op, posx, posy);
  }

  IFn valuechanged_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (valuechanged_cb)
  {
    if (ih->data->val != old_val)
      valuechanged_cb(ih);
  }
}

extern "C" IUP_SDK_API void iupdrvScrollbarGetMinSize(Ihandle* ih, int *w, int *h)
{
  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    *w = 100;
    *h = 15;
  }
  else
  {
    *w = 15;
    *h = 100;
  }
}

static int fltkScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    iupScrollbarCropValue(ih);
    fltkScrollbarUpdateNative(ih);
  }
  return 0;
}

static int fltkScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->linestep), 0.01))
    fltkScrollbarUpdateNative(ih);
  return 0;
}

static int fltkScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
    fltkScrollbarUpdateNative(ih);
  return 0;
}

static int fltkScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    iupScrollbarCropValue(ih);
    fltkScrollbarUpdateNative(ih);
  }
  return 0;
}

static int fltkScrollbarMapMethod(Ihandle* ih)
{
  int horizontal = (ih->data->orientation == ISCROLLBAR_HORIZONTAL);

  IupFltkScrollbar* sb = new IupFltkScrollbar(0, 0, 10, 10, ih, horizontal);
  ih->handle = (InativeHandle*)sb;

  if (ih->data->inverted)
  {
    /* FLTK doesn't have a direct inversion mode for scrollbars */
  }

  fltkScrollbarUpdateNative(ih);

  sb->callback(fltkScrollbarCallback, (void*)ih);

  iupfltkAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    sb->visible_focus(0);

  return IUP_NOERROR;
}

static void fltkScrollbarUnMapMethod(Ihandle* ih)
{
  IupFltkScrollbar* sb = (IupFltkScrollbar*)ih->handle;
  if (sb)
  {
    delete sb;
    ih->handle = NULL;
  }
}

extern "C" IUP_SDK_API void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = fltkScrollbarMapMethod;
  ic->UnMap = fltkScrollbarUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, fltkScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, fltkScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, fltkScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, fltkScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
