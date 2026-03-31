/** \file
 * \brief FLTK Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <FL/Fl.H>
#include <FL/platform.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_draw.H>
#include <FL/Enumerations.H>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"
#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_assert.h"
#include "iup_dialog.h"
#include "iup_dlglist.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Native Container (for absolute positioning)
 ****************************************************************************/

IUP_DRV_API Fl_Group* iupfltkNativeContainerNew(void)
{
  Fl_Group* group = new Fl_Group(0, 0, 1, 1);
  group->end();
  group->resizable(NULL);
  return group;
}

IUP_DRV_API void iupfltkNativeContainerAdd(Fl_Group* container, Fl_Widget* widget)
{
  if (container && widget)
  {
    container->add(widget);
    widget->position(0, 0);
  }
}

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static Fl_Group* fltkGetNativeParent(Ihandle* ih)
{
  return (Fl_Group*)iupChildTreeGetNativeParentHandle(ih);
}

IUP_DRV_API Fl_Window* iupfltkGetParentWidget(Ihandle* ih)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  if (parent)
    return (Fl_Window*)parent;

  Ihandle* ih_focus = IupGetFocus();
  if (ih_focus)
  {
    Ihandle* dlg = IupGetDialog(ih_focus);
    if (dlg && dlg->handle)
      return (Fl_Window*)dlg->handle;
  }

  Ihandle* dlg_iter = iupDlgListFirst();
  while (dlg_iter)
  {
    if (dlg_iter->handle && dlg_iter != ih && iupdrvIsVisible(dlg_iter))
      return (Fl_Window*)dlg_iter->handle;
    dlg_iter = iupDlgListNext();
  }

  return NULL;
}

IUP_DRV_API void iupfltkAddToParent(Ihandle* ih)
{
  Fl_Group* parent = fltkGetNativeParent(ih);
  Fl_Widget* widget = (Fl_Widget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!widget)
    widget = (Fl_Widget*)ih->handle;

  if (!parent || !widget)
    return;

  Fl_Menu_Bar* menubar = dynamic_cast<Fl_Menu_Bar*>(widget);
  if (menubar)
  {
    Fl_Group* dialog_parent = parent;
    while (dialog_parent)
    {
      Fl_Window* win = dialog_parent->as_window();
      if (win)
      {
        win->add(menubar);
        return;
      }
      dialog_parent = dialog_parent->parent();
    }
  }

  iupfltkNativeContainerAdd(parent, widget);
}

IUP_DRV_API void iupfltkSetPosSize(Fl_Widget* widget, int x, int y, int width, int height)
{
  if (widget)
  {
    if (x != widget->x() || y != widget->y() ||
        width != widget->w() || height != widget->h())
    {
      Fl_Window* win = widget->window();
      if (win)
        win->damage(FL_DAMAGE_ALL);

      widget->resize(x, y, width, height);
    }
  }
}

/****************************************************************************
 * Driver Base Functions
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle *ih)
{
  Fl_Widget* widget = (Fl_Widget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!widget)
    widget = (Fl_Widget*)ih->handle;

  if (!widget)
    return;

  /* IUP ih->x, ih->y are parent-relative coordinates.
     FLTK widget->resize() expects window-relative coordinates.
     Get the parent group's window position and add it. */
  int x = ih->x;
  int y = ih->y;

  Fl_Group* parent = widget->parent();
  if (parent && !parent->as_window())
  {
    x += parent->x();
    y += parent->y();
  }

  iupfltkSetPosSize(widget, x, y, ih->currentwidth, ih->currentheight);
}

extern "C" IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID ||
      ih->iclass->nativetype == IUP_TYPEMENU)
    return;

  Fl_Widget* extra_parent = (Fl_Widget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (extra_parent)
  {
    extra_parent->hide();

    Fl_Group* parent = extra_parent->parent();
    if (parent)
      parent->remove(extra_parent);

    delete extra_parent;
    iupAttribSet(ih, "_IUP_EXTRAPARENT", NULL);
  }
  else if (ih->handle)
  {
    Fl_Widget* widget = (Fl_Widget*)ih->handle;
    widget->hide();

    Fl_Group* parent = widget->parent();
    if (parent)
      parent->remove(widget);

    delete widget;
  }

  ih->handle = NULL;
}

extern "C" IUP_SDK_API void iupdrvPostRedraw(Ihandle *ih)
{
  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (widget)
    widget->redraw();
}

extern "C" IUP_SDK_API void iupdrvRedrawNow(Ihandle *ih)
{
  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (widget)
  {
    widget->redraw();

    Fl_Window* win = widget->as_window();
    if (!win) win = widget->window();
    if (win) win->flush();
  }
}

extern "C" IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
#if defined(FLTK_USE_X11)
  if (iupfltkIsX11() && iupfltkX11Load() && iupfltk_XWarpPointer && iupfltk_XRootWindow)
    iupfltk_XWarpPointer(fl_display, 0, iupfltk_XRootWindow(fl_display, fl_screen), 0, 0, 0, 0, x, y);
#else
  (void)x;
  (void)y;
#endif
}

/****************************************************************************
 * Screen/Client Coordinate Conversion
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int *x, int *y)
{
  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (!widget)
    return;

  Fl_Window* win = widget->as_window();
  if (win)
  {
    *x -= win->x_root();
    *y -= win->y_root();
  }
  else
  {
    win = widget->window();
    if (win)
    {
      *x -= win->x_root() + widget->x();
      *y -= win->y_root() + widget->y();
    }
  }
}

extern "C" IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int *x, int *y)
{
  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (!widget)
    return;

  Fl_Window* win = widget->as_window();
  if (win)
  {
    *x += win->x_root();
    *y += win->y_root();
  }
  else
  {
    win = widget->window();
    if (win)
    {
      *x += win->x_root() + widget->x();
      *y += win->y_root() + widget->y();
    }
  }
}

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

IUP_DRV_API int iupfltkEnterLeaveEvent(Fl_Widget *widget, Ihandle* ih, int event)
{
  if (event == FL_ENTER)
  {
    int cursor = iupAttribGetInt(ih, "_IUPFLTK_CURSOR");
    if (cursor)
    {
      Fl_Window* win = widget->window();
      if (win)
        win->cursor((Fl_Cursor)cursor);
    }

    Icallback cb = IupGetCallback(ih, "ENTERWINDOW_CB");
    if (cb) cb(ih);
  }
  else if (event == FL_LEAVE)
  {
    int cursor = iupAttribGetInt(ih, "_IUPFLTK_CURSOR");
    if (cursor)
    {
      Fl_Window* win = widget->window();
      if (win)
        win->cursor(FL_CURSOR_DEFAULT);
    }

    Icallback cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
    if (cb) cb(ih);
  }

  return 0;
}

IUP_DRV_API int iupfltkMouseMoveEvent(Fl_Widget *widget, Ihandle *ih)
{
  IFniis cb;

  cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
  if (cb)
  {
    int x = Fl::event_x() - widget->x();
    int y = Fl::event_y() - widget->y();
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupfltkButtonKeySetStatus(Fl::event_state(), 0, status, 0);
    cb(ih, x, y, status);
  }

  return 0;
}

IUP_DRV_API int iupfltkMouseButtonEvent(Fl_Widget *widget, Ihandle *ih, int event)
{
  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    int doubleclick = 0, ret, press = 1;
    int button = IUP_BUTTON1;
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

    if (event == FL_RELEASE)
      press = 0;

    if (Fl::event_clicks() > 0)
      doubleclick = 1;

    int fl_button = Fl::event_button();
    if (fl_button == FL_LEFT_MOUSE)
      button = IUP_BUTTON1;
    else if (fl_button == FL_MIDDLE_MOUSE)
      button = IUP_BUTTON2;
    else if (fl_button == FL_RIGHT_MOUSE)
      button = IUP_BUTTON3;

    iupfltkButtonKeySetStatus(Fl::event_state(), button, status, doubleclick);

    if (doubleclick)
    {
      status[5] = ' ';
      ret = cb(ih, button, 0, Fl::event_x() - widget->x(), Fl::event_y() - widget->y(), status);
      if (ret == IUP_CLOSE)
      {
        IupExitLoop();
        return 1;
      }
      if (ret == IUP_IGNORE)
        return 1;
      status[5] = 'D';
    }

    ret = cb(ih, button, press, Fl::event_x() - widget->x(), Fl::event_y() - widget->y(), status);
    if (ret == IUP_CLOSE)
      IupExitLoop();
    else if (ret == IUP_IGNORE)
      return 1;
  }

  (void)widget;
  return 0;
}

/****************************************************************************
 * Button/Key Status String
 ****************************************************************************/

IUP_DRV_API void iupfltkButtonKeySetStatus(int state, int button, char* status, int doubleclick)
{
  if (state & FL_SHIFT)
    iupKEY_SETSHIFT(status);

  if (state & FL_CTRL)
    iupKEY_SETCONTROL(status);

  if ((state & FL_BUTTON1) || button == IUP_BUTTON1)
    iupKEY_SETBUTTON1(status);

  if ((state & FL_BUTTON2) || button == IUP_BUTTON2)
    iupKEY_SETBUTTON2(status);

  if ((state & FL_BUTTON3) || button == IUP_BUTTON3)
    iupKEY_SETBUTTON3(status);

  if (state & FL_ALT)
    iupKEY_SETALT(status);

  if (state & FL_META)
    iupKEY_SETSYS(status);

  if (doubleclick)
    iupKEY_SETDOUBLE(status);
}

/****************************************************************************
 * Cursor Management
 ****************************************************************************/

static Fl_Cursor fltkGetCursorShape(const char* name)
{
  struct {
    const char* iupname;
    Fl_Cursor flcursor;
  } table[] = {
    { "NONE",           FL_CURSOR_NONE },
    { "NULL",           FL_CURSOR_NONE },
    { "ARROW",          FL_CURSOR_ARROW },
    { "BUSY",           FL_CURSOR_WAIT },
    { "CROSS",          FL_CURSOR_CROSS },
    { "HAND",           FL_CURSOR_HAND },
    { "HELP",           FL_CURSOR_HELP },
    { "IUP",            FL_CURSOR_HELP },
    { "MOVE",           FL_CURSOR_MOVE },
    { "PEN",            FL_CURSOR_CROSS },
    { "RESIZE_N",       FL_CURSOR_N },
    { "RESIZE_S",       FL_CURSOR_S },
    { "RESIZE_NS",      FL_CURSOR_NS },
    { "SPLITTER_HORIZ", FL_CURSOR_NS },
    { "RESIZE_W",       FL_CURSOR_W },
    { "RESIZE_E",       FL_CURSOR_E },
    { "RESIZE_WE",      FL_CURSOR_WE },
    { "SPLITTER_VERT",  FL_CURSOR_WE },
    { "RESIZE_NE",      FL_CURSOR_NE },
    { "RESIZE_SE",      FL_CURSOR_SE },
    { "RESIZE_NW",      FL_CURSOR_NW },
    { "RESIZE_SW",      FL_CURSOR_SW },
    { "TEXT",           FL_CURSOR_INSERT },
    { "UPARROW",        FL_CURSOR_ARROW },
  };

  int i, count = sizeof(table) / sizeof(table[0]);

  for (i = 0; i < count; i++)
  {
    if (iupStrEqualNoCase(name, table[i].iupname))
      return table[i].flcursor;
  }

  return FL_CURSOR_ARROW;
}

extern "C" IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  Fl_Window* win = widget->as_window();

  if (win)
  {
    win->default_cursor(fltkGetCursorShape(value));
  }
  else
  {
    iupAttribSetInt(ih, "_IUPFLTK_CURSOR", (int)fltkGetCursorShape(value));
  }

  return 1;
}

/****************************************************************************
 * Color Attributes
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  widget->color(fl_rgb_color(r, g, b));
  widget->redraw();

  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetFgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  widget->labelcolor(fl_rgb_color(r, g, b));
  widget->redraw();

  return 1;
}

/****************************************************************************
 * Visibility and Active State
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int visible)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return;

  Fl_Widget* container = (Fl_Widget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  Fl_Widget* widget = (Fl_Widget*)ih->handle;

  if (visible)
  {
    if (container) container->show();
    if (widget) widget->show();
  }
  else
  {
    if (container) container->hide();
    if (widget) widget->hide();
  }
}

extern "C" IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return 1;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;

  if (!widget)
    return 0;

  if (!widget->visible())
    return 0;

  Fl_Group* parent = widget->parent();
  while (parent)
  {
    if (!parent->visible())
      return 0;
    parent = parent->parent();
  }

  return 1;
}

extern "C" IUP_SDK_API int iupdrvIsActive(Ihandle *ih)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return 1;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  return widget ? widget->active() : 0;
}

extern "C" IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
  if (ih->iclass->nativetype == IUP_TYPEVOID || ih->iclass->nativetype == IUP_TYPEMENU)
    return;

  Fl_Widget* container = (Fl_Widget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  Fl_Widget* widget = (Fl_Widget*)ih->handle;

  if (enable)
  {
    if (container) container->activate();
    if (widget) widget->activate();
  }
  else
  {
    if (container) container->deactivate();
    if (widget) widget->deactivate();
  }
}

/****************************************************************************
 * Text and Mnemonic Handling
 ****************************************************************************/

IUP_DRV_API void iupfltkSetMnemonicTitle(Ihandle* ih, Fl_Widget* widget, const char* value)
{
  (void)ih;

  if (!value)
    value = "";

  char c;
  char* str = iupStrProcessMnemonic(value, &c, -1);

  if (str != value)
  {
    widget->copy_label(str);
    free(str);
  }
  else
    widget->copy_label(value);
}

/****************************************************************************
 * Z-Order
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

/****************************************************************************
 * Activate and Reparent
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
  Fl_Widget* widget = (Fl_Widget*)ih->handle;

  if (!widget)
    return;

  widget->do_callback();
}

extern "C" IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
  Fl_Group* new_parent = fltkGetNativeParent(ih);
  Fl_Widget* widget = (Fl_Widget*)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  if (!widget)
    widget = (Fl_Widget*)ih->handle;

  if (widget && new_parent)
  {
    Fl_Group* old_parent = widget->parent();

    if (old_parent != new_parent)
    {
      if (old_parent)
        old_parent->remove(widget);
      new_parent->add(widget);
      widget->show();
    }
  }
}

/****************************************************************************
 * Registration
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
  const char* font_id_name = iupfltkGetNativeFontIdName();
  if (font_id_name)
    iupClassRegisterAttribute(ic, font_id_name, NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_NO_STRING);
}

extern "C" IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "TIPMARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TIPICON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
}

/****************************************************************************
 * Scrollbar and Accessibility
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
  return Fl::scrollbar_size();
}

extern "C" IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle *ih, const char* title)
{
  (void)ih;
  (void)title;
}

IUP_DRV_API int iupfltkHandleDropFiles(Ihandle* ih)
{
  const char* text = Fl::event_text();
  if (!text || !strstr(text, "file://"))
    return 0;

  Ihandle* dlg = IupGetDialog(ih);
  if (!dlg)
    return 0;

  IFnsiii cb = (IFnsiii)IupGetCallback(dlg, "DROPFILES_CB");
  if (!cb)
    return 0;

  int x = Fl::event_x();
  int y = Fl::event_y();
  char* buf = strdup(text);
  char* line = strtok(buf, "\r\n");
  int count = 0;

  while (line)
  {
    if (line[0])
    {
      char* filename = line;
      if (strncmp(filename, "file://", 7) == 0)
        filename += 7;
      cb(dlg, filename, count, x, y);
      count++;
    }
    line = strtok(NULL, "\r\n");
  }

  free(buf);
  return 1;
}
