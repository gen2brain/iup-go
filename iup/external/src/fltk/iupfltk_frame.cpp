/** \file
 * \brief Frame Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>

#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_frame.h"
}

#include "iupfltk_drv.h"


class IupFltkFrame : public Fl_Group
{
public:
  Ihandle* iup_handle;

  IupFltkFrame(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Group(x, y, w, h), iup_handle(ih)
  {
    box(FL_ENGRAVED_BOX);
    end();
  }

  int handle(int event) override
  {
    switch (event)
    {
      case FL_ENTER:
      {
        IFn cb = (IFn)IupGetCallback(iup_handle, "ENTERWINDOW_CB");
        if (cb) cb(iup_handle);
        break;
      }
      case FL_LEAVE:
      {
        IFn cb = (IFn)IupGetCallback(iup_handle, "LEAVEWINDOW_CB");
        if (cb) cb(iup_handle);
        break;
      }
    }
    return Fl_Group::handle(event);
  }
};

static int fltkFrameTitleHeight(Ihandle* ih)
{
  int ch;
  iupdrvFontGetCharSize(ih, NULL, &ch);
  return ch;
}

extern "C" IUP_SDK_API void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
  *x = 3;
  *y = 3;

  if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE"))
    *y += fltkFrameTitleHeight(ih);
}

extern "C" IUP_SDK_API int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 0;
}

extern "C" IUP_SDK_API int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
  if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE"))
  {
    *h = fltkFrameTitleHeight(ih);
    return 1;
  }
  return 0;
}

extern "C" IUP_SDK_API int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
  *w = 6;
  *h = 6;

  if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE"))
    *h += fltkFrameTitleHeight(ih);

  return 1;
}

static int fltkFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    IupFltkFrame* frame = (IupFltkFrame*)ih->handle;
    if (frame)
    {
      frame->label(value ? value : "");
      return 1;
    }
  }
  return 0;
}

static char* fltkFrameGetTitleAttrib(Ihandle* ih)
{
  if (iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    IupFltkFrame* frame = (IupFltkFrame*)ih->handle;
    if (frame)
    {
      const char* title = frame->label();
      if (title && *title)
        return iupStrReturnStr(title);
    }
  }
  return NULL;
}

static int fltkFrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    value = iupBaseNativeParentGetBgColor(ih);

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkFrame* frame = (IupFltkFrame*)ih->handle;
  if (frame)
    frame->color(fl_rgb_color(r, g, b));

  if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    return 1;
  else
    return 0;
}

static int fltkFrameSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkFrame* frame = (IupFltkFrame*)ih->handle;
  if (frame)
    frame->labelcolor(fl_rgb_color(r, g, b));

  return 1;
}

static int fltkFrameSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  return 1;
}

static int fltkFrameSetSunkenAttrib(Ihandle* ih, const char* value)
{
  if (!iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
  {
    IupFltkFrame* frame = (IupFltkFrame*)ih->handle;
    if (frame)
    {
      if (iupStrBoolean(value))
        frame->box(FL_DOWN_BOX);
      else
        frame->box(FL_ENGRAVED_BOX);
      return 1;
    }
  }
  return 0;
}

static void* fltkFrameGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  Fl_Group* inner = (Fl_Group*)iupAttribGet(ih, "_IUPFLTK_FRAME_INNER");
  if (inner)
    return (void*)inner;
  return ih->handle;
}

static int fltkFrameMapMethod(Ihandle* ih)
{
  char* title;

  if (!ih->parent)
    return IUP_ERROR;

  title = iupAttribGet(ih, "TITLE");

  Fl_Group::current(NULL);

  IupFltkFrame* frame = new IupFltkFrame(0, 0, 10, 10, ih);
  ih->handle = (InativeHandle*)frame;

  if (title)
  {
    frame->label(title);
    frame->align(FL_ALIGN_TOP_LEFT);
    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
  }

  frame->begin();
  Fl_Group* inner = new Fl_Group(0, 0, 10, 10);
  inner->end();
  inner->resizable(NULL);
  inner->box(FL_NO_BOX);
  frame->end();

  iupAttribSet(ih, "_IUPFLTK_FRAME_INNER", (char*)inner);

  iupfltkAddToParent(ih);

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
  {
    if (iupAttribGet(ih, "BGCOLOR") || iupAttribGet(ih, "BACKCOLOR"))
      iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");
  }

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    fltkFrameSetBgColorAttrib(ih, NULL);

  return IUP_NOERROR;
}

static void fltkFrameLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  IupFltkFrame* frame = (IupFltkFrame*)ih->handle;
  if (!frame) return;

  int title_h = 0;
  if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE"))
    title_h = fltkFrameTitleHeight(ih);

  if (title_h > 0)
  {
    frame->resize(frame->x(), frame->y() + title_h,
                  frame->w(), frame->h() - title_h);
  }

  Fl_Group* inner = (Fl_Group*)iupAttribGet(ih, "_IUPFLTK_FRAME_INNER");
  if (inner)
  {
    int dx = 3, dy = 3;
    int dw = 6, dh = 6;

    int iw = frame->w() - dw;
    int ih_h = frame->h() - dh;
    if (iw < 1) iw = 1;
    if (ih_h < 1) ih_h = 1;

    inner->resize(frame->x() + dx, frame->y() + dy, iw, ih_h);
  }
}

extern "C" IUP_SDK_API void iupdrvFrameInitClass(Iclass* ic)
{
  ic->Map = fltkFrameMapMethod;
  ic->LayoutUpdate = fltkFrameLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = fltkFrameGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, fltkFrameSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, fltkFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", iupFrameGetBgColorAttrib, fltkFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, fltkFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", fltkFrameGetTitleAttrib, fltkFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SUNKEN", NULL, fltkFrameSetSunkenAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
