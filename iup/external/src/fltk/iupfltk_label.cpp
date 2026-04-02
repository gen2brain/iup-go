/** \file
 * \brief Label Control - FLTK Implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
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
#include "iup_key.h"
#include "iup_drvfont.h"
#include "iup_label.h"
}

#include "iupfltk_drv.h"


class IupFltkLabel : public Fl_Box
{
public:
  Ihandle* iup_handle;

  IupFltkLabel(int x, int y, int w, int h, Ihandle* ih)
    : Fl_Box(x, y, w, h), iup_handle(ih)
  {
  }

protected:
  int handle(int event) override
  {
    switch (event)
    {
      case FL_ENTER:
      case FL_LEAVE:
        iupfltkEnterLeaveEvent(this, iup_handle, event);
        return 1;
      case FL_PUSH:
      case FL_RELEASE:
      {
        IFniiiis cb = (IFniiiis)IupGetCallback(iup_handle, "BUTTON_CB");
        if (cb)
        {
          int button = IUP_BUTTON1;
          if (Fl::event_button() == FL_MIDDLE_MOUSE) button = IUP_BUTTON2;
          else if (Fl::event_button() == FL_RIGHT_MOUSE) button = IUP_BUTTON3;
          int pressed = (event == FL_PUSH) ? 1 : 0;
          char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
          iupfltkButtonKeySetStatus(Fl::event_state(), button, status, 0);
          cb(iup_handle, button, pressed, Fl::event_x() - x(), Fl::event_y() - y(), status);
          return 1;
        }
        break;
      }
    }
    return Fl_Box::handle(event);
  }
};

static void fltkLabelSetPixmap(Ihandle* ih, const char* name, int make_inactive)
{
  if (!name)
    return;

  IupFltkLabel* label = (IupFltkLabel*)ih->handle;
  if (!label)
    return;

  const char* bgcolor = iupBaseNativeParentGetBgColorAttrib(ih);
  Fl_Image* image = (Fl_Image*)iupImageGetImage(name, ih, make_inactive, bgcolor);
  if (image)
  {
    label->image(image);
    label->redraw_label();
  }
}

extern "C" IUP_SDK_API void iupdrvLabelAddExtraPadding(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  (void)x;
  (void)y;
}

static int fltkLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    IupFltkLabel* label = (IupFltkLabel*)ih->handle;
    if (label)
    {
      iupfltkSetMnemonicTitle(ih, label, value);
      return 1;
    }
  }
  return 0;
}

static char* fltkLabelGetTitleAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    IupFltkLabel* label = (IupFltkLabel*)ih->handle;
    if (label)
    {
      const char* text = label->label();
      if (text)
        return iupStrReturnStr(text);
    }
  }
  return NULL;
}

static int fltkLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  char value1[30], value2[30];
  iupStrToStrStr(value, value1, sizeof(value1), value2, sizeof(value2), ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
  else if (iupStrEqualNoCase(value1, "ACENTER"))
    ih->data->horiz_alignment = IUP_ALIGN_ACENTER;
  else
    ih->data->horiz_alignment = IUP_ALIGN_ALEFT;

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    ih->data->vert_alignment = IUP_ALIGN_ATOP;
  else
    ih->data->vert_alignment = IUP_ALIGN_ACENTER;

  if (ih->handle)
  {
    IupFltkLabel* label = (IupFltkLabel*)ih->handle;
    Fl_Align align = 0;

    if (ih->data->horiz_alignment == IUP_ALIGN_ALEFT) align |= FL_ALIGN_LEFT;
    else if (ih->data->horiz_alignment == IUP_ALIGN_ARIGHT) align |= FL_ALIGN_RIGHT;
    else align |= FL_ALIGN_CENTER;

    if (ih->data->vert_alignment == IUP_ALIGN_ATOP) align |= FL_ALIGN_TOP;
    else if (ih->data->vert_alignment == IUP_ALIGN_ABOTTOM) align |= FL_ALIGN_BOTTOM;

    label->align(align | FL_ALIGN_INSIDE);
  }

  return 1;
}

static char* fltkLabelGetAlignmentAttrib(Ihandle* ih)
{
  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    char* horiz_align2str[3] = {(char*)"ALEFT", (char*)"ACENTER", (char*)"ARIGHT"};
    char* vert_align2str[3] = {(char*)"ATOP", (char*)"ACENTER", (char*)"ABOTTOM"};

    int horiz = ih->data->horiz_alignment;
    int vert = ih->data->vert_alignment;

    if (horiz < IUP_ALIGN_ALEFT || horiz > IUP_ALIGN_ARIGHT)
      horiz = IUP_ALIGN_ALEFT;
    if (vert < IUP_ALIGN_ATOP || vert > IUP_ALIGN_ABOTTOM)
      vert = IUP_ALIGN_ACENTER;

    return iupStrReturnStrf("%s:%s", horiz_align2str[horiz], vert_align2str[vert]);
  }
  return NULL;
}

static int fltkLabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    IupFltkLabel* label = (IupFltkLabel*)ih->handle;
    if (label)
    {
      Fl_Align align = label->align();
      if (iupStrBoolean(value))
        align |= FL_ALIGN_WRAP;
      else
        align &= ~FL_ALIGN_WRAP;
      label->align(align);
      return 1;
    }
  }
  return 0;
}

static int fltkLabelSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkLabel* label = (IupFltkLabel*)ih->handle;
  if (label)
  {
    label->color(fl_rgb_color(r, g, b));
    label->box(FL_FLAT_BOX);
    return 1;
  }
  return 0;
}

static int fltkLabelSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupFltkLabel* label = (IupFltkLabel*)ih->handle;
  if (label)
  {
    label->labelcolor(fl_rgb_color(r, g, b));
    return 1;
  }
  return 0;
}

static int fltkLabelSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  return 1;
}

static int fltkLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (iupdrvIsActive(ih))
      fltkLabelSetPixmap(ih, value, 0);
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
        fltkLabelSetPixmap(ih, value, 1);
    }
    return 1;
  }
  return 0;
}

static int fltkLabelSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        fltkLabelSetPixmap(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        fltkLabelSetPixmap(ih, name, 1);
      }
    }
    return 1;
  }
  return 0;
}

static int fltkLabelSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
        fltkLabelSetPixmap(ih, name, 0);
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        fltkLabelSetPixmap(ih, name, 1);
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      fltkLabelSetPixmap(ih, name, 0);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int fltkLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
    return 0;
  else
    return 1;
}

static int fltkLabelMapMethod(Ihandle* ih)
{
  char* value;

  value = iupAttribGet(ih, "SEPARATOR");
  if (value)
  {
    if (iupStrEqualNoCase(value, "HORIZONTAL"))
      ih->data->type = IUP_LABEL_SEP_HORIZ;
    else
      ih->data->type = IUP_LABEL_SEP_VERT;
  }
  else
  {
    value = iupAttribGet(ih, "IMAGE");
    if (value)
      ih->data->type = IUP_LABEL_IMAGE;
    else
      ih->data->type = IUP_LABEL_TEXT;
  }

  IupFltkLabel* label = new IupFltkLabel(0, 0, 10, 10, ih);

  if (ih->data->type == IUP_LABEL_SEP_HORIZ)
  {
    label->box(FL_THIN_DOWN_FRAME);
  }
  else if (ih->data->type == IUP_LABEL_SEP_VERT)
  {
    label->box(FL_THIN_DOWN_FRAME);
  }
  else if (ih->data->type == IUP_LABEL_TEXT)
  {
    label->box(FL_NO_BOX);
    label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    char* title = iupAttribGet(ih, "TITLE");
    if (title)
      iupfltkSetMnemonicTitle(ih, label, title);
  }
  else
  {
    label->box(FL_NO_BOX);
  }

  ih->handle = (InativeHandle*)label;

  iupfltkAddToParent(ih);

  value = iupAttribGet(ih, "PADDING");
  if (value)
    fltkLabelSetPaddingAttrib(ih, value);

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvLabelInitClass(Iclass* ic)
{
  ic->Map = fltkLabelMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, fltkLabelSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, fltkLabelSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, fltkLabelSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, fltkLabelSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", fltkLabelGetTitleAttrib, fltkLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", fltkLabelGetAlignmentAttrib, fltkLabelSetAlignmentAttrib, "ALEFT:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, fltkLabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, fltkLabelSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, fltkLabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, fltkLabelSetWordWrapAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
