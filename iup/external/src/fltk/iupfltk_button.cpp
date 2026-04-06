/** \file
 * \brief Button Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>

#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
}

#include "iupfltk_drv.h"


static void fltkButtonSetPixmap(Ihandle* ih, const char* name, int make_inactive)
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

class IupFltkButton : public Fl_Button
{
public:
  Ihandle* iup_handle;

  IupFltkButton(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Button(x, y, w, h), iup_handle(ih)
  {
  }

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
        if (cb)
          cb(iup_handle);
        if (iupAttribGetBoolean(iup_handle, "FLAT"))
          box(FL_UP_BOX);
        redraw();
        break;
      }
      case FL_LEAVE:
      {
        IFn cb = (IFn)IupGetCallback(iup_handle, "LEAVEWINDOW_CB");
        if (cb)
          cb(iup_handle);
        if (iupAttribGetBoolean(iup_handle, "FLAT"))
          box(FL_FLAT_BOX);
        redraw();
        break;
      }
      case FL_PUSH:
      {
        int ret = Fl_Button::handle(event);
        if (iup_handle->data->type & IUP_BUTTON_IMAGE)
        {
          char* impress = iupAttribGet(iup_handle, "IMPRESS");
          if (impress)
            fltkButtonSetPixmap(iup_handle, impress, 0);
        }
        IFniiiis cb = (IFniiiis)IupGetCallback(iup_handle, "BUTTON_CB");
        if (cb)
        {
          int button = IUP_BUTTON1;
          if (Fl::event_button() == FL_MIDDLE_MOUSE) button = IUP_BUTTON2;
          else if (Fl::event_button() == FL_RIGHT_MOUSE) button = IUP_BUTTON3;
          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupfltkButtonKeySetStatus(Fl::event_state(), button, status, 0);
          cb(iup_handle, button, 1, Fl::event_x() - x(), Fl::event_y() - y(), status);
        }
        return ret;
      }
      case FL_RELEASE:
      {
        int ret = Fl_Button::handle(event);
        if (iup_handle->data->type & IUP_BUTTON_IMAGE)
        {
          char* impress = iupAttribGet(iup_handle, "IMPRESS");
          if (impress)
          {
            char* image = iupAttribGet(iup_handle, "IMAGE");
            if (image)
              fltkButtonSetPixmap(iup_handle, image, 0);
          }
        }
        IFniiiis cb = (IFniiiis)IupGetCallback(iup_handle, "BUTTON_CB");
        if (cb)
        {
          int button = IUP_BUTTON1;
          if (Fl::event_button() == FL_MIDDLE_MOUSE) button = IUP_BUTTON2;
          else if (Fl::event_button() == FL_RIGHT_MOUSE) button = IUP_BUTTON3;
          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupfltkButtonKeySetStatus(Fl::event_state(), button, status, 0);
          cb(iup_handle, button, 0, Fl::event_x() - x(), Fl::event_y() - y(), status);
        }
        return ret;
      }
      case FL_KEYBOARD:
      {
        int key = Fl::event_key();
        if (key == FL_Enter || key == FL_KP_Enter)
        {
          do_callback();
          return 1;
        }
        break;
      }
    }

    return Fl_Button::handle(event);
  }
};

static void fltkButtonCallback(Fl_Widget* w, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  Icallback cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

extern "C" IUP_SDK_API void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
  int border_size = 2 * 5;

  if (ih)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    char* title = iupAttribGet(ih, "TITLE");
    if (!image && (!title || !*title) && iupAttribGet(ih, "BGCOLOR"))
    {
      int charwidth, charheight;
      iupdrvFontGetCharSize(ih, &charwidth, &charheight);
      (*x) += charheight;
    }
  }

  (*x) += border_size;
  (*y) += border_size;
}

static int fltkButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_TEXT)
  {
    IupFltkButton* button = (IupFltkButton*)ih->handle;
    if (button)
    {
      iupfltkSetMnemonicTitle(ih, button, value);
      return 1;
    }
  }
  return 0;
}

static int fltkButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  char value1[30], value2[30];
  iupStrToStrStr(value, value1, sizeof(value1), value2, sizeof(value2), ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
  else if (iupStrEqualNoCase(value1, "ALEFT"))
    ih->data->horiz_alignment = IUP_ALIGN_ALEFT;
  else
    ih->data->horiz_alignment = IUP_ALIGN_ACENTER;

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    ih->data->vert_alignment = IUP_ALIGN_ATOP;
  else
    ih->data->vert_alignment = IUP_ALIGN_ACENTER;

  if (ih->handle)
  {
    IupFltkButton* button = (IupFltkButton*)ih->handle;
    Fl_Align align = FL_ALIGN_INSIDE;

    if (ih->data->horiz_alignment == IUP_ALIGN_ALEFT) align |= FL_ALIGN_LEFT;
    else if (ih->data->horiz_alignment == IUP_ALIGN_ARIGHT) align |= FL_ALIGN_RIGHT;
    else align |= FL_ALIGN_CENTER;

    if (ih->data->vert_alignment == IUP_ALIGN_ATOP) align |= FL_ALIGN_TOP;
    else if (ih->data->vert_alignment == IUP_ALIGN_ABOTTOM) align |= FL_ALIGN_BOTTOM;

    if ((ih->data->type & IUP_BUTTON_IMAGE) && (ih->data->type & IUP_BUTTON_TEXT))
    {
      if (ih->data->img_position == IUP_IMGPOS_LEFT || ih->data->img_position == IUP_IMGPOS_RIGHT)
        align |= FL_ALIGN_IMAGE_NEXT_TO_TEXT;
      if (ih->data->img_position == IUP_IMGPOS_RIGHT || ih->data->img_position == IUP_IMGPOS_BOTTOM)
        align |= FL_ALIGN_TEXT_OVER_IMAGE;
    }

    button->align(align | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
  }

  return 1;
}

static char* fltkButtonGetAlignmentAttrib(Ihandle* ih)
{
  char* horiz_align2str[3] = {(char*)"ALEFT", (char*)"ACENTER", (char*)"ARIGHT"};
  char* vert_align2str[3] = {(char*)"ATOP", (char*)"ACENTER", (char*)"ABOTTOM"};
  return iupStrReturnStrf("%s:%s", horiz_align2str[ih->data->horiz_alignment], vert_align2str[ih->data->vert_alignment]);
}

static int fltkButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
    return 0;
  else
    return 1;
}

static int fltkButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkButton* button = (IupFltkButton*)ih->handle;
  if (button)
    button->color(fl_rgb_color(r, g, b));

  return 1;
}

static int fltkButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkButton* button = (IupFltkButton*)ih->handle;
  if (button)
    button->labelcolor(fl_rgb_color(r, g, b));

  return 1;
}

static int fltkButtonSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  return 1;
}

static int fltkButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (iupdrvIsActive(ih))
      fltkButtonSetPixmap(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
        fltkButtonSetPixmap(ih, value, 1);
    }
    return 1;
  }
  return 0;
}

static int fltkButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        fltkButtonSetPixmap(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        fltkButtonSetPixmap(ih, name, 1);
      }
    }
    return 1;
  }
  return 0;
}

static int fltkButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        fltkButtonSetPixmap(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        fltkButtonSetPixmap(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      fltkButtonSetPixmap(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int fltkButtonSetImPressAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (value)
    {
      if (!iupAttribGetBoolean(ih, "IMPRESSBORDER"))
      {
        IupFltkButton* button = (IupFltkButton*)ih->handle;
        if (button)
          button->box(FL_FLAT_BOX);
      }
    }
    return 1;
  }
  return 0;
}

static int fltkButtonSetFlatAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    IupFltkButton* button = (IupFltkButton*)ih->handle;
    if (iupStrBoolean(value))
      button->box(FL_FLAT_BOX);
    else
      button->box(FL_UP_BOX);
  }
  return 1;
}

static int fltkButtonMapMethod(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "IMAGE");
  if (value)
  {
    ih->data->type = IUP_BUTTON_IMAGE;
    value = iupAttribGet(ih, "TITLE");
    if (value && *value != 0)
      ih->data->type |= IUP_BUTTON_TEXT;
  }
  else
    ih->data->type = IUP_BUTTON_TEXT;

  IupFltkButton* button = new IupFltkButton(0, 0, 10, 10, ih);
  ih->handle = (InativeHandle*)button;

  char* title = iupAttribGet(ih, "TITLE");
  if (title)
    iupfltkSetMnemonicTitle(ih, button, title);

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    if (image)
      fltkButtonSetPixmap(ih, image, 0);

    if (ih->data->type & IUP_BUTTON_TEXT)
    {
      Fl_Align align = FL_ALIGN_IMAGE_NEXT_TO_TEXT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP;

      switch (ih->data->img_position)
      {
        case IUP_IMGPOS_RIGHT:
          align |= FL_ALIGN_TEXT_OVER_IMAGE | FL_ALIGN_CENTER;
          break;
        case IUP_IMGPOS_TOP:
          align = FL_ALIGN_IMAGE_OVER_TEXT | FL_ALIGN_INSIDE | FL_ALIGN_CENTER | FL_ALIGN_CLIP;
          break;
        case IUP_IMGPOS_BOTTOM:
          align = FL_ALIGN_TEXT_OVER_IMAGE | FL_ALIGN_INSIDE | FL_ALIGN_CENTER | FL_ALIGN_CLIP;
          break;
        default:
          align |= FL_ALIGN_CENTER;
          break;
      }

      button->align(align);
    }
  }

  iupfltkAddToParent(ih);

  button->callback(fltkButtonCallback, (void*)ih);

  if (iupAttribGetBoolean(ih, "FLAT") ||
      (iupAttribGet(ih, "IMPRESS") && !iupAttribGetBoolean(ih, "IMPRESSBORDER")))
    button->box(FL_FLAT_BOX);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    button->visible_focus(0);

  value = iupAttribGet(ih, "PADDING");
  if (value)
    fltkButtonSetPaddingAttrib(ih, value);

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = fltkButtonMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, fltkButtonSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, fltkButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, fltkButtonSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, fltkButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", fltkButtonGetAlignmentAttrib, fltkButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, fltkButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, fltkButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, fltkButtonSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, fltkButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "FLAT", NULL, fltkButtonSetFlatAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "IMPRESSBORDER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
