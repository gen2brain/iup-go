/** \file
 * \brief IupDialog - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_draw.H>
#include <FL/platform.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <climits>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_class.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_dlglist.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_focus.h"
#include "iup_str.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"
#include "iup_image.h"
}

#include "iupfltk_drv.h"

#ifdef IUPX11_USE_DLOPEN
#include "iupunix_x11.h"
#endif


class IupFltkDialogInner : public Fl_Group
{
public:
  Fl_Image* bg_image;
  Ihandle* iup_handle;

  IupFltkDialogInner(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Group(x, y, w, h), bg_image(NULL), iup_handle(ih) {}

  void draw() override
  {
    if (bg_image)
    {
      fl_push_clip(x(), y(), w(), h());

      if (iup_handle && iupAttribGetBoolean(iup_handle, "BACKIMAGEZOOM"))
      {
        Fl_Image* scaled = bg_image->copy(w(), h());
        if (scaled)
        {
          scaled->draw(x(), y());
          delete scaled;
        }
      }
      else
      {
        for (int iy = y(); iy < y() + h(); iy += bg_image->h())
          for (int ix = x(); ix < x() + w(); ix += bg_image->w())
            bg_image->draw(ix, iy);
      }

      fl_pop_clip();
      draw_children();
    }
    else
      Fl_Group::draw();
  }
};

class IupFltkDialog : public Fl_Double_Window
{
public:
  Ihandle* iup_handle;
  IupFltkDialogInner* inner_group;

  IupFltkDialog(int w, int h, Ihandle* ih)
    : Fl_Double_Window(w, h), iup_handle(ih), inner_group(nullptr)
  {
  }

  IupFltkDialog(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Double_Window(x, y, w, h), iup_handle(ih), inner_group(nullptr)
  {
  }

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS:
        iupCallGetFocusCb(iup_handle);
        break;

      case FL_UNFOCUS:
        iupCallKillFocusCb(iup_handle);
        break;

      case FL_SHORTCUT:
      case FL_KEYBOARD:
      {
        if (iupfltkKeyPressEvent(this, iup_handle))
          return 1;

        if (Fl::event_key() == FL_Escape)
          return 1;

        break;
      }

      case FL_DND_ENTER:
      case FL_DND_DRAG:
      case FL_DND_LEAVE:
      case FL_DND_RELEASE:
      case FL_PASTE:
        if (iupfltkDragDropHandleEvent(this, iup_handle, event))
          return 1;
        break;
    }

    return Fl_Double_Window::handle(event);
  }

  void resize(int x, int y, int w, int h) override
  {
    Fl_Double_Window::resize(x, y, w, h);

    if (!iup_handle)
      return;

    int menu_h = 0;
    if (iup_handle->data->menu && iup_handle->data->menu->handle)
    {
      menu_h = iupdrvMenuGetMenuBarSize(iup_handle->data->menu);
      Fl_Menu_Bar* menubar = (Fl_Menu_Bar*)iup_handle->data->menu->handle;
      menubar->resize(0, 0, w, menu_h);
    }
    if (inner_group)
      inner_group->resize(0, 0, w, h);

    if (iup_handle->data->ignore_resize)
      return;

    if (!visible())
      return;

    int border = 0, caption = 0, menu = 0;
    iupdrvDialogGetDecoration(iup_handle, &border, &caption, &menu);

    int new_width = w + 2 * border;
    int new_height = h + 2 * border + caption;

    iup_handle->currentwidth = new_width;
    iup_handle->currentheight = new_height;

    int client_w = w;
    int client_h = h - menu;

    IFnii cb = (IFnii)IupGetCallback(iup_handle, "RESIZE_CB");
    if (!cb || cb(iup_handle, client_w, client_h) != IUP_IGNORE)
    {
      iup_handle->data->ignore_resize = 1;
      IupRefresh(iup_handle);
      iup_handle->data->ignore_resize = 0;
    }
  }
};

IUP_DRV_API void iupfltkX11SetSkipTaskbar(Fl_Window* window)
{
#if defined(FLTK_USE_X11)
  if (!window || !iupfltkIsX11() || !fl_xid(window))
    return;
#ifdef IUPX11_USE_DLOPEN
  if (!iupX11Open())
    return;
#endif

  Atom net_wm_state = XInternAtom(fl_display, "_NET_WM_STATE", 0);
  Atom skip_taskbar = XInternAtom(fl_display, "_NET_WM_STATE_SKIP_TASKBAR", 0);
  Window root = XRootWindow(fl_display, fl_screen);

  XEvent xev;
  memset(&xev, 0, sizeof(xev));
  xev.xclient.type = ClientMessage;
  xev.xclient.window = fl_xid(window);
  xev.xclient.message_type = net_wm_state;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 1;
  xev.xclient.data.l[1] = (long)skip_taskbar;

  XSendEvent(fl_display, root, 0, SubstructureNotifyMask | SubstructureRedirectMask, &xev);
#else
  (void)window;
#endif
}

static int fltkDialogGetMenuSize(Ihandle* ih)
{
  if (ih->data->menu)
    return iupdrvMenuGetMenuBarSize(ih->data->menu);
  return 0;
}

extern "C" IUP_SDK_API void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu)
{
  *menu = fltkDialogGetMenuSize(ih);

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") || iupAttribGetBoolean(ih, "HIDETITLEBAR"))
  {
    *border = 0;
    *caption = 0;
    return;
  }

  int has_titlebar = iupAttribGetBoolean(ih, "RESIZE") ||
                     iupAttribGetBoolean(ih, "MAXBOX") ||
                     iupAttribGetBoolean(ih, "MINBOX") ||
                     iupAttribGetBoolean(ih, "MENUBOX") ||
                     iupAttribGet(ih, "TITLE");

  int has_border = has_titlebar ||
                   iupAttribGetBoolean(ih, "RESIZE") ||
                   iupAttribGetBoolean(ih, "BORDER");

  if (ih->handle)
  {
    IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
    int dw = dialog->decorated_w() - dialog->w();
    int dh = dialog->decorated_h() - dialog->h();

    if (dw > 0 || dh > 0)
    {
      *border = has_border ? dw / 2 : 0;
      *caption = has_titlebar ? (dh - dw) : 0;
      if (*caption < 0) *caption = 0;
      return;
    }
  }

  *border = has_border ? 1 : 0;
  *caption = has_titlebar ? 25 : 0;
}

extern "C" IUP_SDK_API void iupdrvDialogGetPosition(Ihandle *ih, InativeHandle* handle, int *x, int *y)
{
  IupFltkDialog* dialog;

  if (!handle)
    handle = ih->handle;

  dialog = (IupFltkDialog*)handle;

  if (dialog)
  {
    if (x) *x = dialog->x_root();
    if (y) *y = dialog->y_root();
  }
}

extern "C" IUP_SDK_API void iupdrvDialogSetPosition(Ihandle *ih, int x, int y)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (dialog)
    dialog->position(x, y);
}

extern "C" IUP_SDK_API void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int *w, int *h)
{
  int border = 0, caption = 0, menu = 0;

  if (!handle)
    handle = ih->handle;

  IupFltkDialog* dialog = (IupFltkDialog*)handle;

  if (dialog)
  {
    if (ih)
      iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    if (w) *w = dialog->w() + 2 * border;
    if (h) *h = dialog->h() + 2 * border + caption;
  }
}

extern "C" IUP_SDK_API void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (!dialog)
    return;

  if (visible)
  {
    dialog->show();

#if defined(FLTK_USE_X11)
    if (iupfltkIsX11() && fl_xid(dialog)
#ifdef IUPX11_USE_DLOPEN
        && iupX11Open()
#endif
        )
    {
      if (iupAttribGetBoolean(ih, "CUSTOMFRAME") && !iupAttribGet(ih, "_IUPFLTK_CUSTOMFRAME_FIXED"))
      {
        Atom net_wm_state = XInternAtom(fl_display, "_NET_WM_STATE", 0);
        XDeleteProperty(fl_display, fl_xid(dialog), net_wm_state);
        iupAttribSet(ih, "_IUPFLTK_CUSTOMFRAME_FIXED", "1");
      }
    }
#endif

    if (iupDialogGetNativeParent(ih))
      iupfltkX11SetSkipTaskbar(dialog);

    const char* cursor = iupAttribGetStr(ih, "CURSOR");
    if (cursor)
      iupdrvBaseSetCursorAttrib(ih, cursor);
  }
  else
    dialog->hide();
}

extern "C" IUP_SDK_API int iupdrvDialogIsVisible(Ihandle* ih)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (!dialog)
    return 0;
  return dialog->visible();
}

extern "C" IUP_SDK_API int iupdrvDialogSetPlacement(Ihandle* ih)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (!dialog)
    return 0;

  char* placement;
  int old_state = ih->data->show_state;

  ih->data->show_state = IUP_SHOW;
  iupAttribSet(ih, "MAXIMIZED", NULL);
  iupAttribSet(ih, "MINIMIZED", NULL);

  if (iupAttribGetBoolean(ih, "FULLSCREEN"))
  {
    dialog->fullscreen();
    return 1;
  }

  placement = iupAttribGet(ih, "PLACEMENT");
  if (!placement)
  {
    if (old_state == IUP_MAXIMIZE && dialog->maximize_active())
    {
      dialog->un_maximize();
      ih->data->show_state = IUP_RESTORE;
    }
    else if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;
    return 0;
  }

  if (iupStrEqualNoCase(placement, "MINIMIZED"))
  {
    ih->data->show_state = IUP_MINIMIZE;
    iupAttribSet(ih, "MINIMIZED", "Yes");
    dialog->iconize();
  }
  else if (iupStrEqualNoCase(placement, "MAXIMIZED"))
  {
    ih->data->show_state = IUP_MAXIMIZE;
    iupAttribSet(ih, "MAXIMIZED", "Yes");
    dialog->maximize();
  }
  else if (iupStrEqualNoCase(placement, "NORMAL") || placement[0] == 0)
  {
    if (dialog->maximize_active())
      dialog->un_maximize();
    else
      dialog->show();
    ih->data->show_state = IUP_RESTORE;
  }
  else if (iupStrEqualNoCase(placement, "FULL"))
  {
    int width, height;
    int border, caption, menu;
    iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

    int fx = -(border);
    int fy = -(border + caption + menu);

    iupdrvGetFullSize(&width, &height);
    height += menu;

    iupdrvDialogSetPosition(ih, fx, fy);
    dialog->size(width, height);

    if (old_state == IUP_MAXIMIZE || old_state == IUP_MINIMIZE)
      ih->data->show_state = IUP_RESTORE;
  }

  iupAttribSet(ih, "PLACEMENT", NULL);
  return 1;
}

extern "C" IUP_SDK_API void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* parent)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (!dialog)
    return;

  if (parent)
    dialog->set_non_modal();
  else
    dialog->clear_modal_states();
}

static int fltkDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (dialog)
    dialog->label(value ? value : "");
  return 1;
}

static int fltkDialogSetResizeAttrib(Ihandle* ih, const char* value)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (dialog)
  {
    if (iupStrBoolean(value))
    {
      dialog->resizable(dialog->inner_group);
      dialog->size_range(1, 1);
    }
    else
    {
      dialog->resizable(NULL);
      dialog->size_range(dialog->w(), dialog->h(), dialog->w(), dialog->h());
    }
  }
  return 1;
}

static int fltkDialogSetMinSizeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return iupBaseSetMinSizeAttrib(ih, value);

  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  int min_w = 1, min_h = 1;
  iupStrToIntInt(value, &min_w, &min_h, 'x');

  int border = 0, caption = 0, menu = 0;
  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  int cw = min_w - 2 * border;
  int ch = min_h - 2 * border - caption;
  if (cw < 1) cw = 1;
  if (ch < 1) ch = 1;

  int max_w = 0, max_h = 0;
  iupStrToIntInt(iupAttribGet(ih, "MAXSIZE"), &max_w, &max_h, 'x');
  int mw = max_w > 0 ? max_w - 2 * border : 0;
  int mh = max_h > 0 ? max_h - 2 * border - caption : 0;

  dialog->size_range(cw, ch, mw, mh);

  return iupBaseSetMinSizeAttrib(ih, value);
}

static int fltkDialogSetMaxSizeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return iupBaseSetMaxSizeAttrib(ih, value);

  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  int max_w = 0, max_h = 0;
  iupStrToIntInt(value, &max_w, &max_h, 'x');

  int border = 0, caption = 0, menu = 0;
  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  int mw = max_w > 0 ? max_w - 2 * border : 0;
  int mh = max_h > 0 ? max_h - 2 * border - caption : 0;

  int min_w = 1, min_h = 1;
  iupStrToIntInt(iupAttribGet(ih, "MINSIZE"), &min_w, &min_h, 'x');
  int cmin_w = min_w - 2 * border;
  int cmin_h = min_h - 2 * border - caption;
  if (cmin_w < 1) cmin_w = 1;
  if (cmin_h < 1) cmin_h = 1;

  dialog->size_range(cmin_w, cmin_h, mw, mh);

  return iupBaseSetMaxSizeAttrib(ih, value);
}

static int fltkDialogSetBringFrontAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
    if (dialog)
      dialog->show();
  }
  return 0;
}

static int fltkDialogSetTopMostAttrib(Ihandle* ih, const char* value)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (!dialog)
    return 0;

  if (iupStrBoolean(value))
    dialog->set_non_modal();

  return 1;
}


static int fltkDialogSetIconAttrib(Ihandle* ih, const char* value)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (!dialog)
    return 1;

  if (!value)
  {
    dialog->icon((const Fl_RGB_Image*)NULL);
    return 1;
  }

  Fl_RGB_Image* icon = (Fl_RGB_Image*)iupImageGetIcon(value);
  if (icon)
    dialog->icon(icon);

  return 1;
}

static int fltkDialogSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (dialog)
  {
    Fl_Color c = fl_rgb_color(r, g, b);
    dialog->color(c);
    if (dialog->inner_group)
      dialog->inner_group->color(c);
    dialog->redraw();
  }

  return 1;
}

static int fltkDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (!dialog)
    return 0;

  if (iupStrToRGB(value, &r, &g, &b))
  {
    Fl_Color c = fl_rgb_color(r, g, b);
    dialog->color(c);
    if (dialog->inner_group)
    {
      dialog->inner_group->color(c);
      dialog->inner_group->bg_image = NULL;
    }
    dialog->redraw();
    return 1;
  }
  else
  {
    Fl_Image* image = (Fl_Image*)iupImageGetImage(value, ih, 0, NULL);
    if (image)
    {
      if (dialog->inner_group)
        dialog->inner_group->bg_image = image;
      dialog->redraw();
      return 1;
    }
  }

  return 0;
}

static int fltkDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (!dialog)
    return 0;

  if (iupStrBoolean(value))
    dialog->fullscreen();
  else
    dialog->fullscreen_off();

  return 1;
}

static char* fltkDialogGetClientSizeAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
    int menu = fltkDialogGetMenuSize(ih);
    return iupStrReturnIntInt(dialog->w(), dialog->h() - menu, 'x');
  }
  return iupDialogGetClientSizeAttrib(ih);
}

static char* fltkDialogGetClientOffsetAttrib(Ihandle* ih)
{
  (void)ih;
  return const_cast<char*>("0x0");
}

static void fltkDialogSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (ih->firstchild)
  {
    char* offset = iupAttribGet(ih, "CHILDOFFSET");
    x = 0;
    y = 0;
    if (offset) iupStrToIntInt(offset, &x, &y, 'x');

    y += fltkDialogGetMenuSize(ih);

    iupBaseSetPosition(ih->firstchild, x, y);
  }
}

static void* fltkDialogGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (dialog)
    return (void*)dialog->inner_group;
  return NULL;
}

static void fltkDialogCloseCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;

  if (!iupdrvIsActive(ih))
    return;

  Icallback cb = IupGetCallback(ih, "CLOSE_CB");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_IGNORE)
      return;
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  IupHide(ih);
}

static int fltkDialogMapMethod(Ihandle* ih)
{
  IupFltkDialog* dialog = new IupFltkDialog(100, 100, ih);
  dialog->end();

  ih->handle = (InativeHandle*)dialog;

  dialog->begin();
  IupFltkDialogInner* inner = new IupFltkDialogInner(0, 0, 100, 100, ih);
  inner->end();
  inner->box(FL_FLAT_BOX);
  inner->resizable(NULL);
  dialog->end();
  dialog->inner_group = inner;

  if (iupAttribGetBoolean(ih, "RESIZE"))
    dialog->resizable(inner);
  else
    dialog->resizable(NULL);

  const char* title = iupAttribGetStr(ih, "TITLE");
  if (title)
    dialog->label(title);

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME") || iupAttribGetBoolean(ih, "HIDETITLEBAR"))
  {
    dialog->border(0);
  }
  else
  {
    int has_border = iupAttribGetBoolean(ih, "BORDER") ||
                     iupAttribGetBoolean(ih, "RESIZE") ||
                     iupAttribGet(ih, "TITLE");
    if (!has_border)
      dialog->border(0);
  }

  if (iupDialogGetNativeParent(ih))
    dialog->set_non_modal();

  if (ih->data->menu && !ih->data->menu->handle)
  {
    ih->data->menu->parent = ih;
    IupMap(ih->data->menu);
  }

  dialog->callback(fltkDialogCloseCallback, (void*)ih);

  if (iupAttribGetBoolean(ih, "CUSTOMFRAME"))
    iupDialogCustomFrameSimulateCheckCallbacks(ih);

  iupAttribSet(ih, "VISIBLE", NULL);

  return IUP_NOERROR;
}

static void fltkDialogUnMapMethod(Ihandle* ih)
{
  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;
  if (dialog)
  {
    if (ih->data->menu)
    {
      ih->data->menu->handle = NULL;
      IupDestroy(ih->data->menu);
      ih->data->menu = NULL;
    }

    delete dialog;
    ih->handle = NULL;
  }
}

static void fltkDialogLayoutUpdateMethod(Ihandle* ih)
{
  int border, caption, menu;
  int width, height;

  IupFltkDialog* dialog = (IupFltkDialog*)ih->handle;

  if (ih->data->ignore_resize)
    return;

  ih->data->ignore_resize = 1;

  iupdrvDialogGetDecoration(ih, &border, &caption, &menu);

  width = ih->currentwidth - 2 * border;
  height = ih->currentheight - 2 * border - caption;

  if (width <= 0) width = 1;
  if (height <= 0) height = 1;

  if (dialog)
  {
    dialog->size(width, height);

    if (ih->data->menu && ih->data->menu->handle)
    {
      Fl_Menu_Bar* menubar = (Fl_Menu_Bar*)ih->data->menu->handle;
      menubar->resize(0, 0, width, menu);
    }

    if (dialog->inner_group)
    {
      dialog->inner_group->resize(0, 0, width, height);
    }
  }

  if (!iupAttribGetBoolean(ih, "RESIZE"))
  {
    dialog->resizable(NULL);
    dialog->size_range(width, height, width, height);
  }
  else
  {
    if (!dialog->resizable())
      dialog->resizable(dialog->inner_group);
  }

  ih->data->ignore_resize = 0;
}

extern "C" IUP_SDK_API void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = fltkDialogMapMethod;
  ic->UnMap = fltkDialogUnMapMethod;
  ic->LayoutUpdate = fltkDialogLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = fltkDialogGetInnerNativeContainerHandleMethod;
  ic->SetChildrenPosition = fltkDialogSetChildrenPositionMethod;

  iupClassRegisterCallback(ic, "MOVE_CB", "ii");

  iupClassRegisterAttribute(ic, "CLIENTSIZE", fltkDialogGetClientSizeAttrib, iupDialogSetClientSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", fltkDialogGetClientOffsetAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_DEFAULTVALUE | IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkDialogSetBgColorAttrib, "DLGBGCOLOR", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, fltkDialogSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, fltkDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, fltkDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINSIZE", NULL, fltkDialogSetMinSizeAttrib, IUPAF_SAMEASSYSTEM, "1x1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXSIZE", NULL, fltkDialogSetMaxSizeAttrib, IUPAF_SAMEASSYSTEM, "65535x65535", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, fltkDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZE", NULL, fltkDialogSetResizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BORDER", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINBOX", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXBOX", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUBOX", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, iupfltkGetNativeWindowHandleName(), iupfltkGetNativeWindowHandleAttrib, NULL, NULL, NULL, IUPAF_NO_INHERIT | IUPAF_NO_STRING);

  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, fltkDialogSetBringFrontAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, fltkDialogSetTopMostAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETITLEBAR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MAXIMIZED", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINIMIZED", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICLIENT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDIMENU", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MDICHILD", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
