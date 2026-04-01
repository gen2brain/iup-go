/** \file
 * \brief Toggle Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Button.H>
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
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_toggle.h"
}

#include "iupfltk_drv.h"


#define SWITCH_TRACK_WIDTH  40
#define SWITCH_TRACK_HEIGHT 20
#define SWITCH_THUMB_SIZE   14
#define SWITCH_THUMB_MARGIN 3

class IupFltkSwitch : public Fl_Widget
{
public:
  Ihandle* iup_handle;
  int checked;

  IupFltkSwitch(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Widget(x, y, w, h), iup_handle(ih), checked(0) {}

protected:
  void draw() override
  {
    int tx = x();
    int ty = y();

    Fl_Color track_color;
    Fl_Color thumb_color;

    if (!active_r())
    {
      track_color = FL_INACTIVE_COLOR;
      thumb_color = FL_INACTIVE_COLOR;
    }
    else if (checked)
    {
      track_color = FL_SELECTION_COLOR;
      thumb_color = FL_GRAY;
    }
    else
    {
      track_color = FL_BACKGROUND2_COLOR;
      thumb_color = FL_GRAY;
    }

    fl_draw_box(FL_THIN_DOWN_BOX, tx, ty, SWITCH_TRACK_WIDTH, SWITCH_TRACK_HEIGHT, track_color);

    int thumb_x, thumb_y;
    if (checked)
      thumb_x = tx + SWITCH_TRACK_WIDTH - SWITCH_THUMB_SIZE - SWITCH_THUMB_MARGIN;
    else
      thumb_x = tx + SWITCH_THUMB_MARGIN;
    thumb_y = ty + (SWITCH_TRACK_HEIGHT - SWITCH_THUMB_SIZE) / 2;

    fl_draw_box(FL_UP_BOX, thumb_x, thumb_y, SWITCH_THUMB_SIZE, SWITCH_THUMB_SIZE, thumb_color);

    if (Fl::focus() == this)
    {
      fl_color(FL_BLACK);
      fl_focus_rect(tx, ty, SWITCH_TRACK_WIDTH, SWITCH_TRACK_HEIGHT);
    }
  }

  int handle(int event) override
  {
    switch (event)
    {
      case FL_PUSH:
      {
        checked = !checked;
        redraw();

        IFni cb = (IFni)IupGetCallback(iup_handle, "ACTION");
        if (cb && cb(iup_handle, checked) == IUP_CLOSE)
          IupExitLoop();

        if (iupObjectCheck(iup_handle))
          iupBaseCallValueChangedCb(iup_handle);

        return 1;
      }
      case FL_KEYBOARD:
      {
        int key = Fl::event_key();
        if (key == ' ' || key == FL_Enter || key == FL_KP_Enter)
        {
          checked = !checked;
          redraw();

          IFni cb = (IFni)IupGetCallback(iup_handle, "ACTION");
          if (cb && cb(iup_handle, checked) == IUP_CLOSE)
            IupExitLoop();

          if (iupObjectCheck(iup_handle))
            iupBaseCallValueChangedCb(iup_handle);

          return 1;
        }
        if (iupfltkKeyPressEvent(this, iup_handle))
          return 1;
        break;
      }
      case FL_FOCUS:
      case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event);
        redraw();
        return 1;
      case FL_ENTER:
      case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event);
        break;
    }
    return Fl_Widget::handle(event);
  }
};

class IupFltkCheckButton : public Fl_Check_Button
{
public:
  Ihandle* iup_handle;

  IupFltkCheckButton(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Check_Button(x, y, w, h), iup_handle(ih) {}

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS:
      case FL_UNFOCUS:
      {
        IFni cb = (IFni)IupGetCallback(iup_handle, "FOCUS_CB");
        if (cb)
          cb(iup_handle, event == FL_FOCUS ? 1 : 0);
        break;
      }
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
    return Fl_Check_Button::handle(event);
  }
};

class IupFltkRadioButton : public Fl_Round_Button
{
public:
  Ihandle* iup_handle;

  IupFltkRadioButton(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Round_Button(x, y, w, h), iup_handle(ih)
  {
    type(FL_RADIO_BUTTON);
  }

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS: case FL_UNFOCUS:
        iupfltkFocusInOutEvent(this, iup_handle, event); break;
      case FL_ENTER: case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event); break;
      case FL_KEYBOARD:
        if (iupfltkKeyPressEvent(this, iup_handle)) return 1; break;
    }
    return Fl_Round_Button::handle(event);
  }
};

class IupFltkToggleButton : public Fl_Button
{
public:
  Ihandle* iup_handle;

  IupFltkToggleButton(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Button(x, y, w, h), iup_handle(ih)
  {
    type(FL_TOGGLE_BUTTON);
  }

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_FOCUS:
      case FL_UNFOCUS:
      {
        IFni cb = (IFni)IupGetCallback(iup_handle, "FOCUS_CB");
        if (cb)
          cb(iup_handle, event == FL_FOCUS ? 1 : 0);
        break;
      }
      case FL_ENTER:
      {
        IFn cb = (IFn)IupGetCallback(iup_handle, "ENTERWINDOW_CB");
        if (cb) cb(iup_handle);
        if (iup_handle->data->flat)
          box(FL_UP_BOX);
        redraw();
        break;
      }
      case FL_LEAVE:
      {
        IFn cb = (IFn)IupGetCallback(iup_handle, "LEAVEWINDOW_CB");
        if (cb) cb(iup_handle);
        if (iup_handle->data->flat && !value())
          box(FL_FLAT_BOX);
        redraw();
        break;
      }
    }
    return Fl_Button::handle(event);
  }
};

static void fltkToggleCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  Fl_Button* button = (Fl_Button*)w;
  int state = button->value();

  IFni cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih, state) == IUP_CLOSE)
      IupExitLoop();
  }

  IFn vcb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (vcb)
  {
    if (vcb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

static void fltkToggleSetPixmap(Ihandle* ih, const char* name, int make_inactive)
{
  if (!name)
    return;

  Fl_Button* button = (Fl_Button*)ih->handle;
  if (!button)
    return;

  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  Fl_Image* image = (Fl_Image*)iupImageGetImage(name, ih, make_inactive, bgcolor);
  if (image)
    button->image(image);
}

extern "C" IUP_SDK_API void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y)
{
  iupdrvButtonAddBorders(ih, x, y);
}

extern "C" IUP_SDK_API void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* str)
{
  (void)str;
  if (ih->data->type == IUP_TOGGLE_IMAGE)
    return;
  *x += 20;
  (void)y;
}

static int fltkToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupFltkSwitch* sw = (IupFltkSwitch*)ih->handle;
    if (sw)
    {
      sw->checked = iupStrBoolean(value);
      sw->redraw();
    }
    return 0;
  }

  Fl_Button* button = (Fl_Button*)ih->handle;
  if (!button)
    return 0;

  if (iupStrEqualNoCase(value, "NOTDEF"))
    button->value(0);
  else if (iupStrBoolean(value))
    button->value(1);
  else
    button->value(0);

  return 0;
}

static char* fltkToggleGetValueAttrib(Ihandle* ih)
{
  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    IupFltkSwitch* sw = (IupFltkSwitch*)ih->handle;
    if (sw)
      return iupStrReturnChecked(sw->checked);
    return iupStrReturnChecked(0);
  }

  Fl_Button* button = (Fl_Button*)ih->handle;
  if (!button)
    return NULL;

  return iupStrReturnChecked(button->value());
}

static int fltkToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
  {
    Fl_Button* button = (Fl_Button*)ih->handle;
    if (button)
    {
      iupfltkSetMnemonicTitle(ih, button, value);
      return 1;
    }
  }
  return 0;
}

static int fltkToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
    return 0;

  iupAttribSet(ih, "ALIGNMENT", (char*)value);

  if (ih->handle)
    iupdrvPostRedraw(ih);

  return 1;
}

static char* fltkToggleGetAlignmentAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_TOGGLE_TEXT)
    return NULL;

  char* value = iupAttribGet(ih, "ALIGNMENT");
  if (!value)
    return (char*)"ACENTER:ACENTER";

  return value;
}

static int fltkToggleSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (widget)
    widget->color(fl_rgb_color(r, g, b));

  return 1;
}

static int fltkToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Fl_Widget* widget = (Fl_Widget*)ih->handle;
  if (widget)
    widget->labelcolor(fl_rgb_color(r, g, b));

  return 1;
}

static int fltkToggleSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  return 1;
}

static int fltkToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (iupdrvIsActive(ih))
      fltkToggleSetPixmap(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
        fltkToggleSetPixmap(ih, value, 1);
    }
    return 1;
  }
  return 0;
}

static int fltkToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        fltkToggleSetPixmap(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        fltkToggleSetPixmap(ih, name, 1);
      }
    }
    return 1;
  }
  return 0;
}

static int fltkToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        fltkToggleSetPixmap(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        fltkToggleSetPixmap(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      fltkToggleSetPixmap(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int fltkToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
    return 0;
  else
    return 1;
}

static int fltkToggleSetFlatAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    ih->data->flat = iupStrBoolean(value);
    if (ih->handle)
    {
      Fl_Button* button = (Fl_Button*)ih->handle;
      if (ih->data->flat)
        button->box(FL_FLAT_BOX);
      else
        button->box(FL_UP_BOX);
    }
    return 0;
  }
  return 1;
}

static int fltkToggleMapMethod(Ihandle* ih)
{
  char* value;

  value = iupAttribGet(ih, "IMAGE");
  if (value)
    ih->data->type = IUP_TOGGLE_IMAGE;
  else
    ih->data->type = IUP_TOGGLE_TEXT;

  Ihandle* radio = iupRadioFindToggleParent(ih);
  int is_radio = (radio != NULL);

  if (iupAttribGetBoolean(ih, "SWITCH") && !is_radio)
  {
    ih->data->type = IUP_TOGGLE_TEXT;

    IupFltkSwitch* sw = new IupFltkSwitch(0, 0, SWITCH_TRACK_WIDTH, SWITCH_TRACK_HEIGHT, ih);
    ih->handle = (InativeHandle*)sw;

    value = iupAttribGet(ih, "VALUE");
    if (value && iupStrBoolean(value))
      sw->checked = 1;

    iupfltkAddToParent(ih);

    if (!iupAttribGetBoolean(ih, "CANFOCUS"))
      iupfltkSetCanFocus(sw, 0);

    return IUP_NOERROR;
  }

  if (is_radio)
    iupAttribSet(ih, "SWITCH", "NO");

  Fl_Button* button;

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    IupFltkToggleButton* toggle = new IupFltkToggleButton(0, 0, 10, 10, ih);
    button = toggle;

    if (is_radio)
    {
      toggle->type(FL_RADIO_BUTTON);
      ih->data->is_radio = 1;

      if (ih == IupGetHandle(IupGetAttribute(radio, "VALUE_HANDLE")))
        toggle->value(1);
      else if (!IupGetAttribute(radio, "VALUE_HANDLE"))
        toggle->value(1);
    }

    if (ih->data->flat)
      toggle->box(FL_FLAT_BOX);
  }
  else if (is_radio)
  {
    IupFltkRadioButton* radio_btn = new IupFltkRadioButton(0, 0, 10, 10, ih);
    button = radio_btn;

    ih->data->is_radio = 1;

    if (ih == IupGetHandle(IupGetAttribute(radio, "VALUE_HANDLE")))
      radio_btn->value(1);
    else if (!IupGetAttribute(radio, "VALUE_HANDLE"))
      radio_btn->value(1);
  }
  else
  {
    IupFltkCheckButton* check = new IupFltkCheckButton(0, 0, 10, 10, ih);
    button = check;
  }

  ih->handle = (InativeHandle*)button;

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image)
      fltkToggleSetPixmap(ih, image, 0);
  }
  else
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title)
      iupfltkSetMnemonicTitle(ih, button, title);
  }

  iupfltkAddToParent(ih);

  button->callback(fltkToggleCallback, (void*)ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    button->visible_focus(0);

  value = iupAttribGet(ih, "PADDING");
  if (value)
    fltkToggleSetPaddingAttrib(ih, value);

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvToggleAddSwitch(Ihandle* ih, int *x, int *y, const char* str)
{
  (void)ih;

  (*x) += 2 + SWITCH_TRACK_WIDTH + 2;
  if ((*y) < 2 + SWITCH_TRACK_HEIGHT + 2)
    (*y) = 2 + SWITCH_TRACK_HEIGHT + 2;
  else
    (*y) += 2 + 2;

  if (str && str[0])
    (*x) += 8;
}

extern "C" IUP_SDK_API void iupdrvToggleInitClass(Iclass* ic)
{
  ic->Map = fltkToggleMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, fltkToggleSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, fltkToggleSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkToggleSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, fltkToggleSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, fltkToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", fltkToggleGetValueAttrib, fltkToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", fltkToggleGetAlignmentAttrib, fltkToggleSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, fltkToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, fltkToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, fltkToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLAT", NULL, fltkToggleSetFlatAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
