/** \file
 * \brief Button Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_button.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"

#include "iupefl_drv.h"


static void eflButtonClickedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Icallback cb;

  (void)ev;

  cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }
}

static void eflButtonPressedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFniiiis cb;

  (void)ev;

  iupAttribSet(ih, "PRESSED", "YES");

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* name = iupAttribGet(ih, "IMPRESS");
    if (name)
    {
      Eo* btn = iupeflGetWidget(ih);
      Eo* image = iupeflImageGetImage(name, ih, 0);
      if (image)
        efl_content_set(btn, image);
    }
  }

  cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    iupKEY_SETBUTTON1(status);
    cb(ih, IUP_BUTTON1, 1, 0, 0, status);
  }
}

static void eflButtonUnpressedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFniiiis cb;

  (void)ev;

  iupAttribSet(ih, "PRESSED", "NO");

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* name = iupAttribGet(ih, "IMPRESS");
    if (name)
    {
      name = iupAttribGet(ih, "IMAGE");
      if (name)
      {
        Eo* btn = iupeflGetWidget(ih);
        Eo* image = iupeflImageGetImage(name, ih, 0);
        if (image)
          efl_content_set(btn, image);
      }
    }
  }

  cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (cb)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
    cb(ih, IUP_BUTTON1, 0, 0, 0, status);
  }
}

static void eflButtonRepeatedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Icallback cb;

  (void)ev;

  cb = IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }
}

static int eflButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  Eo* btn = iupeflGetWidget(ih);

  if (!btn)
    return 0;

  efl_text_set(btn, iupeflStrConvertToSystem(value ? value : ""));

  if (value && value[0])
    ih->data->type |= IUP_BUTTON_TEXT;
  else
    ih->data->type &= ~IUP_BUTTON_TEXT;

  return 1;
}

static int eflButtonSetFontAttrib(Ihandle* ih, const char* value)
{
  iupdrvSetFontAttrib(ih, value);
  return 1;
}

static int eflButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  return 0;
}

static int eflButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  Eo* btn = iupeflGetWidget(ih);
  Eo* image;

  if (!btn)
    return 1;

  if (value && value[0])
  {
    ih->data->type |= IUP_BUTTON_IMAGE;

    if (iupdrvIsActive(ih))
    {
      image = iupeflImageGetImage(value, ih, 0);
      if (image)
        efl_content_set(btn, image);
    }
    else
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
      {
        image = iupeflImageGetImage(value, ih, 1);
        if (image)
          efl_content_set(btn, image);
      }
    }
  }
  else
  {
    ih->data->type &= ~IUP_BUTTON_IMAGE;
    efl_content_unset(btn);
  }

  return 1;
}

static int eflButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  Eo* btn = iupeflGetWidget(ih);

  if (!btn)
    return 0;

  efl_ui_widget_disabled_set(btn, iupStrBoolean(value) ? EINA_FALSE : EINA_TRUE);

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupStrBoolean(value))
    {
      char* name = iupAttribGet(ih, "IMINACTIVE");
      if (name)
      {
        Eo* image = iupeflImageGetImage(name, ih, 0);
        if (image)
          efl_content_set(btn, image);
      }
      else
      {
        name = iupAttribGet(ih, "IMAGE");
        if (name)
        {
          Eo* image = iupeflImageGetImage(name, ih, 1);
          if (image)
            efl_content_set(btn, image);
        }
      }
    }
    else
    {
      char* name = iupAttribGet(ih, "IMAGE");
      if (name)
      {
        Eo* image = iupeflImageGetImage(name, ih, 0);
        if (image)
          efl_content_set(btn, image);
      }
    }
  }

  return 0;
}

static int eflButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      Eo* btn = iupeflGetWidget(ih);
      if (btn)
      {
        if (value)
        {
          Eo* image = iupeflImageGetImage(value, ih, 0);
          if (image)
            efl_content_set(btn, image);
        }
        else
        {
          char* name = iupAttribGet(ih, "IMAGE");
          if (name)
          {
            Eo* image = iupeflImageGetImage(name, ih, 1);
            if (image)
              efl_content_set(btn, image);
          }
        }
      }
    }
    return 1;
  }
  return 0;
}

static int eflButtonSetImPressAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    iupdrvPostRedraw(ih);
    return 1;
  }
  return 0;
}

static int eflButtonSetFlatAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Eo* btn = iupeflGetWidget(ih);
  Eo* color_rect;
  unsigned char r, g, b;

  if (!btn)
    return 1;

  if (!value || !value[0])
    return 1;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 1;

  if (ih->data->type & IUP_BUTTON_IMAGE)
    return 1;

  if (ih->data->type & IUP_BUTTON_TEXT)
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title && title[0])
      return 1;
  }

  color_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_COLORRECT");
  if (!color_rect)
  {
    color_rect = efl_add(EFL_CANVAS_RECTANGLE_CLASS, btn);
    if (!color_rect)
      return 1;

    efl_gfx_hint_size_min_set(color_rect, EINA_SIZE2D(16, 16));
    iupAttribSet(ih, "_IUP_EFL_COLORRECT", (char*)color_rect);
    efl_content_set(btn, color_rect);
    efl_gfx_entity_visible_set(color_rect, EINA_TRUE);
  }

  efl_gfx_color_set(color_rect, r, g, b, 255);

  return 1;
}

static int eflButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflButtonMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* btn;
  char* value;
  int has_border = 1;

  parent = iupeflGetParentWidget(ih);

  value = iupAttribGet(ih, "IMAGE");
  if (value && value[0])
  {
    ih->data->type = IUP_BUTTON_IMAGE;

    value = iupAttribGet(ih, "TITLE");
    if (value && value[0])
      ih->data->type |= IUP_BUTTON_TEXT;
  }
  else
    ih->data->type = IUP_BUTTON_TEXT;

  if (ih->data->type & IUP_BUTTON_IMAGE &&
      iupAttribGet(ih, "IMPRESS") &&
      !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
    has_border = 0;

  if (iupAttribGetBoolean(ih, "FLAT"))
    has_border = 0;

  if (!has_border)
    btn = efl_add(EFL_UI_BUTTON_CLASS, parent, efl_ui_widget_style_set(efl_added, "anchor"));
  else
    btn = efl_add(EFL_UI_BUTTON_CLASS, parent);

  if (!btn)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)btn;

  value = iupAttribGet(ih, "TITLE");
  if (value && value[0])
    efl_text_set(btn, iupeflStrConvertToSystem(value));

  value = iupAttribGet(ih, "IMAGE");
  if (value && value[0])
  {
    Eo* image = iupeflImageGetImage(value, ih, 0);
    if (image)
      efl_content_set(btn, image);
  }

  efl_event_callback_add(btn, EFL_INPUT_EVENT_CLICKED, eflButtonClickedCallback, ih);
  efl_event_callback_add(btn, EFL_INPUT_EVENT_PRESSED, eflButtonPressedCallback, ih);
  efl_event_callback_add(btn, EFL_INPUT_EVENT_UNPRESSED, eflButtonUnpressedCallback, ih);
  efl_event_callback_add(btn, EFL_UI_AUTOREPEAT_EVENT_REPEATED, eflButtonRepeatedCallback, ih);

  efl_event_callback_add(btn, EFL_EVENT_POINTER_IN, iupeflPointerInEvent, ih);
  efl_event_callback_add(btn, EFL_EVENT_POINTER_OUT, iupeflPointerOutEvent, ih);
  efl_event_callback_add(btn, EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
  efl_event_callback_add(btn, EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);
  efl_event_callback_add(btn, EFL_EVENT_FOCUS_IN, iupeflManagerFocusChangedEvent, ih);
  efl_event_callback_add(btn, EFL_EVENT_FOCUS_OUT, iupeflManagerFocusChangedEvent, ih);

  iupeflAddToParent(ih);

  return IUP_NOERROR;
}

static void eflButtonUnMapMethod(Ihandle* ih)
{
  Eo* btn = iupeflGetWidget(ih);

  if (btn)
  {
    efl_event_callback_del(btn, EFL_INPUT_EVENT_CLICKED, eflButtonClickedCallback, ih);
    efl_event_callback_del(btn, EFL_INPUT_EVENT_PRESSED, eflButtonPressedCallback, ih);
    efl_event_callback_del(btn, EFL_INPUT_EVENT_UNPRESSED, eflButtonUnpressedCallback, ih);
    efl_event_callback_del(btn, EFL_UI_AUTOREPEAT_EVENT_REPEATED, eflButtonRepeatedCallback, ih);

    efl_event_callback_del(btn, EFL_EVENT_POINTER_IN, iupeflPointerInEvent, ih);
    efl_event_callback_del(btn, EFL_EVENT_POINTER_OUT, iupeflPointerOutEvent, ih);
    efl_event_callback_del(btn, EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
    efl_event_callback_del(btn, EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);
    efl_event_callback_del(btn, EFL_EVENT_FOCUS_IN, iupeflManagerFocusChangedEvent, ih);
    efl_event_callback_del(btn, EFL_EVENT_FOCUS_OUT, iupeflManagerFocusChangedEvent, ih);

    efl_del(btn);
  }

  ih->handle = NULL;
}

static void eflButtonComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  int natural_w = 0, natural_h = 0;
  int type = ih->data->type;

  (void)children_expand;

  if (type & IUP_BUTTON_IMAGE)
  {
    iupImageGetInfo(iupAttribGet(ih, "IMAGE"), &natural_w, &natural_h, NULL);

    if (type & IUP_BUTTON_TEXT)
    {
      int text_w, text_h;
      char* title = iupAttribGet(ih, "TITLE");
      if (title && title[0])
      {
        iupdrvFontGetMultiLineStringSize(ih, title, &text_w, &text_h);

        if (ih->data->img_position == IUP_IMGPOS_RIGHT ||
            ih->data->img_position == IUP_IMGPOS_LEFT)
        {
          natural_w += text_w + ih->data->spacing;
          natural_h = iupMAX(natural_h, text_h);
        }
        else
        {
          natural_w = iupMAX(natural_w, text_w);
          natural_h += text_h + ih->data->spacing;
        }
      }
    }
  }
  else if (type & IUP_BUTTON_TEXT)
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title && title[0])
      iupdrvFontGetMultiLineStringSize(ih, title, &natural_w, &natural_h);
    else if (iupAttribGet(ih, "BGCOLOR"))
    {
      natural_w = 16;
      natural_h = 16;
    }
  }

  iupdrvButtonAddBorders(ih, &natural_w, &natural_h);

  natural_w += 2 * ih->data->horiz_padding;
  natural_h += 2 * ih->data->vert_padding;

  *w = natural_w;
  *h = natural_h;
}

static int efl_button_padding_x = -1;
static int efl_button_padding_y = -1;

static void eflButtonMeasurePadding(void)
{
  Eo* temp_win;
  Eo* temp_button;
  Eina_Size2D btn_min;
  int text_w = 0, text_h = 0;

  if (efl_button_padding_x >= 0)
    return;

  temp_win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                     efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC));
  if (!temp_win)
  {
    efl_button_padding_x = 24;
    efl_button_padding_y = 16;
    return;
  }

  temp_button = efl_add(EFL_UI_BUTTON_CLASS, temp_win);
  if (!temp_button)
  {
    efl_del(temp_win);
    efl_button_padding_x = 24;
    efl_button_padding_y = 16;
    return;
  }

  efl_text_set(temp_button, "Wg");

  efl_canvas_group_calculate(temp_button);

  btn_min = efl_gfx_hint_size_combined_min_get(temp_button);

  iupdrvFontGetMultiLineStringSize(NULL, "Wg", &text_w, &text_h);

  efl_button_padding_x = btn_min.w - text_w;
  efl_button_padding_y = btn_min.h - text_h;

  if (efl_button_padding_x < 0)
    efl_button_padding_x = 0;
  if (efl_button_padding_y < 0)
    efl_button_padding_y = 0;

  efl_del(temp_win);
}

void iupdrvButtonAddBorders(Ihandle* ih, int* x, int* y)
{
  int has_border = 1;

  if (ih)
  {
    if (ih->data->type & IUP_BUTTON_IMAGE &&
        iupAttribGet(ih, "IMPRESS") &&
        !iupAttribGetBoolean(ih, "IMPRESSBORDER"))
      has_border = 0;

    if (iupAttribGetBoolean(ih, "FLAT"))
      has_border = 0;
  }

  if (has_border)
  {
    eflButtonMeasurePadding();
    *x += efl_button_padding_x;
    *y += efl_button_padding_y;
  }
}

void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = eflButtonMapMethod;
  ic->UnMap = eflButtonUnMapMethod;
  ic->ComputeNaturalSize = eflButtonComputeNaturalSizeMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, eflButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, eflButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, eflButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, eflButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, eflButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, eflButtonSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupeflBaseGetActiveAttrib, eflButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FLAT", NULL, eflButtonSetFlatAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflButtonSetBgColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, eflButtonSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FONT", NULL, eflButtonSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
}
