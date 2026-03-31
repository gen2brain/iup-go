/** \file
 * \brief IupPopover control - FLTK implementation
 *
 * Uses Fl_Window with border(0). AUTOHIDE mode uses set_override() for
 * popup-like behavior (auto-dismiss on click outside).
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_childtree.h"
#include "iup_class.h"
#include "iup_register.h"
#include "iup_popover.h"
}

#include "iupfltk_drv.h"


class IupFltkPopover : public Fl_Window
{
public:
  Ihandle* iup_handle;
  Fl_Group* content_group;
  int autohide_enabled;

  IupFltkPopover(int w, int h, Ihandle* ih, int autohide)
    : Fl_Window(w, h), iup_handle(ih), content_group(NULL), autohide_enabled(autohide)
  {
    border(0);
    set_non_modal();

    content_group = new Fl_Group(1, 1, w - 2, h - 2);
    content_group->end();
    content_group->resizable(NULL);

    end();
  }

  void draw() override
  {
    Fl_Window::draw();

    fl_color(FL_DARK2);
    fl_rect(0, 0, w(), h());
  }

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_HIDE:
      {
        IFni cb = (IFni)IupGetCallback(iup_handle, "SHOW_CB");
        if (cb)
          cb(iup_handle, IUP_HIDE);
        break;
      }
      case FL_SHOW:
      {
        IFni cb = (IFni)IupGetCallback(iup_handle, "SHOW_CB");
        if (cb)
          cb(iup_handle, IUP_SHOW);
        break;
      }
      case FL_UNFOCUS:
      {
        if (autohide_enabled)
        {
          Ihandle* anchor = (Ihandle*)iupAttribGet(iup_handle, "_IUP_POPOVER_ANCHOR");
          Fl_Widget* below = Fl::belowmouse();

          if (anchor && anchor->handle && below == (Fl_Widget*)anchor->handle)
            break;

          hide();
        }
        break;
      }
    }

    return Fl_Window::handle(event);
  }
};

static int fltkPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    if (!anchor || !anchor->handle)
      return 0;

    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    IupFltkPopover* popover = (IupFltkPopover*)ih->handle;
    Fl_Widget* anchor_widget = (Fl_Widget*)anchor->handle;

    if (ih->firstchild && ih->firstchild->handle)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih->firstchild);
    }

    int ax = 0, ay = 0;
    Fl_Widget* w = anchor_widget;
    while (w)
    {
      ax += w->x();
      ay += w->y();
      Fl_Window* win = w->as_window();
      if (win)
      {
        ax = win->x_root();
        ay = win->y_root();
        break;
      }
      w = w->parent();
    }

    if (!anchor_widget->as_window())
    {
      Fl_Window* win = anchor_widget->window();
      if (win)
      {
        ax = win->x_root() + anchor_widget->x();
        ay = win->y_root() + anchor_widget->y();
      }
    }

    int x, y;
    const char* pos = iupAttribGetStr(ih, "POSITION");

    if (iupStrEqualNoCase(pos, "TOP"))
    {
      x = ax;
      y = ay - ih->currentheight;
    }
    else if (iupStrEqualNoCase(pos, "LEFT"))
    {
      x = ax - ih->currentwidth;
      y = ay;
    }
    else if (iupStrEqualNoCase(pos, "RIGHT"))
    {
      x = ax + anchor_widget->w();
      y = ay;
    }
    else
    {
      x = ax;
      y = ay + anchor_widget->h();
    }

    popover->resize(x, y, ih->currentwidth, ih->currentheight);

    if (popover->content_group)
      popover->content_group->resize(1, 1, ih->currentwidth - 2, ih->currentheight - 2);

    popover->show();
  }
  else
  {
    if (ih->handle)
    {
      IupFltkPopover* popover = (IupFltkPopover*)ih->handle;
      popover->hide();
    }
  }

  return 0;
}

static char* fltkPopoverGetVisibleAttrib(Ihandle* ih)
{
  if (!ih->handle)
    return (char*)"NO";

  IupFltkPopover* popover = (IupFltkPopover*)ih->handle;
  return iupStrReturnBoolean(popover->visible());
}

static void fltkPopoverLayoutUpdateMethod(Ihandle* ih)
{
  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 1, 1);
    iupLayoutUpdate(ih->firstchild);
  }
}

static void* fltkPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  IupFltkPopover* popover = (IupFltkPopover*)ih->handle;
  if (popover)
    return (void*)popover->content_group;
  return NULL;
}

static int fltkPopoverMapMethod(Ihandle* ih)
{
  int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");

  IupFltkPopover* popover = new IupFltkPopover(100, 100, ih, autohide);

  ih->handle = (InativeHandle*)popover;

  return IUP_NOERROR;
}

static void fltkPopoverUnMapMethod(Ihandle* ih)
{
  IupFltkPopover* popover = (IupFltkPopover*)ih->handle;

  if (popover)
  {
    popover->hide();
    delete popover;
  }

  ih->handle = NULL;
}

extern "C" IUP_SDK_API void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = fltkPopoverMapMethod;
  ic->UnMap = fltkPopoverUnMapMethod;
  ic->LayoutUpdate = fltkPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = fltkPopoverGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "VISIBLE", fltkPopoverGetVisibleAttrib, fltkPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
