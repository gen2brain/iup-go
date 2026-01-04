/** \file
 * \brief Label Control
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
#include "iup_label.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupefl_drv.h"


static void eflLabelApplyFont(Ihandle* ih)
{
  Eo* label = iupeflGetWidget(ih);
  if (label && ih->data->type == IUP_LABEL_TEXT)
    iupeflApplyTextStyle(ih, label);
}

static int eflLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  Eo* label = iupeflGetWidget(ih);

  if (!label)
    return 0;

  if (ih->data->type == IUP_LABEL_IMAGE)
    return 0;

  if (ih->data->type == IUP_LABEL_SEP_HORIZ ||
      ih->data->type == IUP_LABEL_SEP_VERT)
    return 0;

  iupeflSetMnemonicTitle(ih, label, value);
  eflLabelApplyFont(ih);

  return 1;
}

static int eflLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  Eo* label = iupeflGetWidget(ih);
  char value1[30], value2[30];

  if (!label)
    return 1;

  if (ih->data->type != IUP_LABEL_TEXT)
    return 1;

  iupStrToStrStr(value, value1, value2, ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    efl_text_horizontal_align_set(label, 1.0);
  else if (iupStrEqualNoCase(value1, "ACENTER"))
    efl_text_horizontal_align_set(label, 0.5);
  else
    efl_text_horizontal_align_set(label, 0.0);

  return 1;
}

static int eflLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  return 0;
}

static int eflLabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  Eo* label = iupeflGetWidget(ih);

  if (!label)
    return 0;

  if (ih->data->type != IUP_LABEL_TEXT)
    return 0;

  if (iupStrBoolean(value))
  {
    efl_text_multiline_set(label, EINA_TRUE);
    efl_text_wrap_set(label, EFL_TEXT_FORMAT_WRAP_WORD);
  }
  else
  {
    efl_text_wrap_set(label, EFL_TEXT_FORMAT_WRAP_NONE);
  }

  return 1;
}

static int eflLabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
  Eo* label = iupeflGetWidget(ih);

  if (!label)
    return 0;

  if (ih->data->type != IUP_LABEL_TEXT)
    return 0;

  if (iupStrBoolean(value))
    efl_text_ellipsis_set(label, 1.0);
  else
    efl_text_ellipsis_set(label, -1.0);

  return 1;
}

static int eflLabelReplaceWithImage(Ihandle* ih, const char* name, int make_inactive)
{
  Eo* old_widget = iupeflGetWidget(ih);
  Eo* image;

  if (!old_widget)
    return 0;

  image = iupeflImageGetImage(name, ih, make_inactive);
  if (!image)
    return 0;

  ih->data->type = IUP_LABEL_IMAGE;

  iupeflBaseRemoveCallbacks(ih, old_widget);
  efl_del(old_widget);
  ih->handle = (InativeHandle*)image;

  iupeflBaseAddCallbacks(ih, image);
  iupeflAddToParent(ih);
  efl_gfx_entity_visible_set(image, EINA_TRUE);

  return 1;
}

static int eflLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  Eo* widget;

  if (!value || !value[0])
    return 0;

  widget = iupeflGetWidget(ih);
  if (!widget)
    return 1;

  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    if (iupeflImageUpdateImage(widget, value, ih, 0))
      return 1;
  }

  return eflLabelReplaceWithImage(ih, value, 0);
}

static int eflLabelSetFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)value;

  if (ih->data->type != IUP_LABEL_TEXT)
    return 1;

  eflLabelApplyFont(ih);
  return 1;
}

static int eflLabelSetFontAttrib(Ihandle* ih, const char* value)
{
  iupdrvSetFontAttrib(ih, value);

  if (ih->data->type != IUP_LABEL_TEXT)
    return 1;

  eflLabelApplyFont(ih);
  return 1;
}

static int eflLabelSetActiveAttrib(Ihandle* ih, const char* value)
{
  Eo* label = iupeflGetWidget(ih);

  if (!label)
    return 0;

  if (efl_isa(label, EFL_UI_WIDGET_CLASS))
  {
    if (iupStrBoolean(value))
      efl_ui_widget_disabled_set(label, EINA_FALSE);
    else
      efl_ui_widget_disabled_set(label, EINA_TRUE);
  }

  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    char* name;
    int make_inactive = !iupStrBoolean(value);

    if (make_inactive)
      name = iupAttribGet(ih, "IMINACTIVE");
    else
      name = iupAttribGet(ih, "IMAGE");

    if (name)
    {
      if (!iupeflImageUpdateImage(label, name, ih, make_inactive))
        eflLabelReplaceWithImage(ih, name, make_inactive);
    }
  }

  return 0;
}

static int eflLabelMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* label;
  char* value;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  value = iupAttribGet(ih, "SEPARATOR");
  if (value)
  {
    if (iupStrEqualNoCase(value, "HORIZONTAL"))
    {
      ih->data->type = IUP_LABEL_SEP_HORIZ;

      label = efl_add(EFL_UI_SEPARATOR_CLASS, parent, efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL));
    }
    else
    {
      ih->data->type = IUP_LABEL_SEP_VERT;

      label = efl_add(EFL_UI_SEPARATOR_CLASS, parent, efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
    }
  }
  else
  {
    value = iupAttribGet(ih, "IMAGE");
    if (value)
    {
      ih->data->type = IUP_LABEL_IMAGE;

      label = iupeflImageGetImage(value, ih, 0);
      if (!label)
      {
        label = efl_add(EFL_UI_IMAGE_CLASS, parent);
        if (!label)
          return IUP_ERROR;
      }
    }
    else
    {
      ih->data->type = IUP_LABEL_TEXT;

      label = efl_add(EFL_UI_TEXTBOX_CLASS, parent,
        efl_text_interactive_editable_set(efl_added, EINA_FALSE),
        efl_text_interactive_selection_allowed_set(efl_added, EINA_FALSE));
      if (!label)
        return IUP_ERROR;

      value = iupAttribGet(ih, "TITLE");
      if (value && value[0])
        iupeflSetMnemonicTitle(ih, label, value);
    }
  }

  ih->handle = (InativeHandle*)label;

  iupeflBaseAddCallbacks(ih, label);

  iupeflAddToParent(ih);

  eflLabelApplyFont(ih);

  if (ih->data->type == IUP_LABEL_TEXT || ih->data->type == IUP_LABEL_IMAGE)
    efl_gfx_entity_visible_set(label, EINA_TRUE);

  return IUP_NOERROR;
}

static void eflLabelUnMapMethod(Ihandle* ih)
{
  Eo* label = iupeflGetWidget(ih);

  if (label)
  {
    iupeflBaseRemoveCallbacks(ih, label);
    efl_del(label);
  }
}

void iupdrvLabelAddExtraPadding(Ihandle* ih, int* x, int* y)
{
  int border_x, border_y;

  if (ih->data->type != IUP_LABEL_TEXT)
    return;

  iupeflTextGetBorder(&border_x, &border_y);

  *x += border_x;
  *y += border_y;
}

void iupdrvLabelInitClass(Iclass* ic)
{
  ic->Map = eflLabelMapMethod;
  ic->UnMap = eflLabelUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, eflLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, eflLabelSetAlignmentAttrib, "ALEFT:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, eflLabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, eflLabelSetWordWrapAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, eflLabelSetEllipsisAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, eflLabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupeflBaseGetActiveAttrib, eflLabelSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupeflSetBgColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, eflLabelSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FONT", NULL, eflLabelSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
}
