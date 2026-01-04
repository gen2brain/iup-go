/** \file
 * \brief Toggle Control
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
#include "iup_toggle.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"

#include "iupefl_drv.h"


/****************************************************************
                     Helper Functions
****************************************************************/

static void eflToggleLabelClickCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* check_widget;
  Eina_Bool current;

  (void)ev;

  if (!iupAttribGetBoolean(ih, "ACTIVE"))
    return;

  check_widget = (Eo*)iupAttribGet(ih, "_IUP_EFL_CHECK");
  if (check_widget)
  {
    current = efl_ui_selectable_selected_get(check_widget);
    efl_ui_selectable_selected_set(check_widget, !current);
  }
}

static void eflToggleUpdateImage(Ihandle* ih, int check)
{
  Eo* toggle = iupeflGetWidget(ih);
  char* name;
  Evas_Object* image;

  if (!toggle || ih->data->type != IUP_TOGGLE_IMAGE)
    return;

  if (check)
  {
    name = iupAttribGet(ih, "IMPRESS");
    if (!name)
      name = iupAttribGet(ih, "IMAGE");
  }
  else
  {
    name = iupAttribGet(ih, "IMAGE");
  }

  if (name)
  {
    image = iupeflImageGetImage(name, ih, 0);
    if (image)
    {
      if (ih->data->type == IUP_TOGGLE_IMAGE)
        elm_object_content_set(toggle, image);
      else
        efl_content_set(toggle, image);
    }
  }
}

/****************************************************************
                     Callbacks
****************************************************************/

static void eflToggleSelectedChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFni cb;
  int check;

  if (iupAttribGet(ih, "_IUP_EFL_IGNORE_TOGGLE"))
    return;

  check = efl_ui_selectable_selected_get(ev->object);

  if (ih->data->type == IUP_TOGGLE_IMAGE)
    eflToggleUpdateImage(ih, check);

  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih, check);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  iupBaseCallValueChangedCb(ih);
}

static void eflRadioGroupValueChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* radio_ih = (Ihandle*)data;
  int new_value = efl_ui_radio_group_selected_value_get(ev->object);
  Ihandle* child;

  if (new_value < 0)
    return;

  for (child = radio_ih->firstchild; child; child = child->brother)
  {
    if (child->iclass->nativetype == IUP_TYPEVOID)
      continue;

    int my_value = (intptr_t)iupAttribGet(child, "_IUP_EFL_RADIO_VALUE");
    if (my_value == new_value)
    {
      IFni cb;

      if (iupAttribGet(child, "_IUP_EFL_IGNORE_TOGGLE"))
        return;

      if (child->data->type == IUP_TOGGLE_IMAGE)
        eflToggleUpdateImage(child, 1);

      cb = (IFni)IupGetCallback(child, "ACTION");
      if (cb)
      {
        int ret = cb(child, 1);
        if (ret == IUP_CLOSE)
          IupExitLoop();
      }

      iupBaseCallValueChangedCb(child);
      break;
    }
  }
}

/****************************************************************
                     Attributes
****************************************************************/

static int eflToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  Eo* toggle = iupeflGetWidget(ih);
  Eo* check_widget;
  int check;
  int is_radio_image = ih->data->is_radio && ih->data->type == IUP_TOGGLE_IMAGE;

  if (!toggle)
    return 0;

  check_widget = (Eo*)iupAttribGet(ih, "_IUP_EFL_CHECK");
  if (!check_widget)
    check_widget = toggle;

  iupAttribSet(ih, "_IUP_EFL_IGNORE_TOGGLE", "1");

  if (iupStrEqualNoCase(value, "TOGGLE"))
  {
    if (ih->data->is_radio && !is_radio_image)
    {
      Eo* radio_group = (Eo*)iupAttribGet(iupRadioFindToggleParent(ih), "_IUP_EFL_RADIO_GROUP");
      int radio_value = (intptr_t)iupAttribGet(ih, "_IUP_EFL_RADIO_VALUE");
      int current = efl_ui_radio_group_selected_value_get(radio_group);
      check = !(current == radio_value);
    }
    else if (ih->data->type == IUP_TOGGLE_IMAGE || iupAttribGetBoolean(ih, "SWITCH"))
    {
      check = !elm_check_state_get(check_widget);
    }
    else
      check = !efl_ui_selectable_selected_get(check_widget);
  }
  else
    check = iupStrBoolean(value);

  if (is_radio_image)
  {
    elm_check_state_set(check_widget, check ? EINA_TRUE : EINA_FALSE);
    eflToggleUpdateImage(ih, check);
  }
  else if (ih->data->is_radio)
  {
    if (check)
    {
      Eo* radio_group = (Eo*)iupAttribGet(iupRadioFindToggleParent(ih), "_IUP_EFL_RADIO_GROUP");
      int radio_value = (intptr_t)iupAttribGet(ih, "_IUP_EFL_RADIO_VALUE");
      efl_ui_radio_group_selected_value_set(radio_group, radio_value);
    }
  }
  else if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    elm_check_state_set(check_widget, check ? EINA_TRUE : EINA_FALSE);
    eflToggleUpdateImage(ih, check);
  }
  else if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    elm_check_state_set(check_widget, check ? EINA_TRUE : EINA_FALSE);
  }
  else
  {
    efl_ui_selectable_selected_set(check_widget, check ? EINA_TRUE : EINA_FALSE);
  }

  iupAttribSet(ih, "_IUP_EFL_IGNORE_TOGGLE", NULL);

  return 0;
}

static char* eflToggleGetValueAttrib(Ihandle* ih)
{
  Eo* toggle = iupeflGetWidget(ih);
  Eo* check_widget;
  int is_radio_image = ih->data->is_radio && ih->data->type == IUP_TOGGLE_IMAGE;

  if (!toggle)
    return "OFF";

  check_widget = (Eo*)iupAttribGet(ih, "_IUP_EFL_CHECK");
  if (!check_widget)
    check_widget = toggle;

  if (is_radio_image || ih->data->type == IUP_TOGGLE_IMAGE || iupAttribGetBoolean(ih, "SWITCH"))
  {
    if (elm_check_state_get(check_widget))
      return "ON";
    else
      return "OFF";
  }
  else if (ih->data->is_radio)
  {
    Eo* radio_group = (Eo*)iupAttribGet(iupRadioFindToggleParent(ih), "_IUP_EFL_RADIO_GROUP");
    int radio_value = (intptr_t)iupAttribGet(ih, "_IUP_EFL_RADIO_VALUE");
    if (efl_ui_radio_group_selected_value_get(radio_group) == radio_value)
      return "ON";
    else
      return "OFF";
  }
  else
  {
    if (efl_ui_selectable_selected_get(check_widget))
      return "ON";
    else
      return "OFF";
  }
}

static int eflToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  Eo* label;

  if (ih->data->type == IUP_TOGGLE_IMAGE)
    return 0;

  if (iupAttribGetBoolean(ih, "SWITCH"))
    return 0;

  label = (Eo*)iupAttribGet(ih, "_IUP_EFL_LABEL");
  if (label)
  {
    efl_text_set(label, iupeflStrConvertToSystem(value));
    iupeflApplyTextStyle(ih, label);
    return 1;
  }

  return 0;
}

static int eflToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  return 0;
}

static int eflToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  Eo* toggle = iupeflGetWidget(ih);

  if (ih->data->type != IUP_TOGGLE_IMAGE)
    return 0;

  if (!toggle)
    return 1;

  if (value && value[0])
  {
    Evas_Object* image = iupeflImageGetImage(value, ih, 0);
    if (image)
      elm_object_content_set(toggle, image);
  }
  else
  {
    elm_object_content_set(toggle, NULL);
  }

  return 1;
}

static int eflToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);
  Eo* check_widget;
  Eo* label;
  Eina_Bool disabled = iupStrBoolean(value) ? EINA_FALSE : EINA_TRUE;

  if (!widget)
    return 0;

  if (ih->data->type == IUP_TOGGLE_IMAGE || iupAttribGetBoolean(ih, "SWITCH"))
  {
    elm_object_disabled_set(widget, disabled);
  }
  else
  {
    check_widget = (Eo*)iupAttribGet(ih, "_IUP_EFL_CHECK");
    label = (Eo*)iupAttribGet(ih, "_IUP_EFL_LABEL");

    if (check_widget)
      efl_ui_widget_disabled_set(check_widget, disabled);
    if (label)
      iupeflApplyTextStyle(ih, label);
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    char* name;
    int check = elm_check_state_get(widget);

    if (disabled)
      name = iupAttribGet(ih, "IMINACTIVE");
    else if (check)
    {
      name = iupAttribGet(ih, "IMPRESS");
      if (!name)
        name = iupAttribGet(ih, "IMAGE");
    }
    else
      name = iupAttribGet(ih, "IMAGE");

    if (name)
    {
      Evas_Object* image = iupeflImageGetImage(name, ih, disabled);
      if (image)
        elm_object_content_set(widget, image);
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int eflToggleSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  Eo* label;
  (void)value;

  if (ih->data->type == IUP_TOGGLE_TEXT && !iupAttribGetBoolean(ih, "SWITCH"))
  {
    label = (Eo*)iupAttribGet(ih, "_IUP_EFL_LABEL");
    if (label)
      iupeflApplyTextStyle(ih, label);
  }

  return 1;
}

static int eflToggleSetFontAttrib(Ihandle* ih, const char* value)
{
  Eo* label;

  iupdrvSetFontAttrib(ih, value);

  if (ih->data->type == IUP_TOGGLE_TEXT && !iupAttribGetBoolean(ih, "SWITCH"))
  {
    label = (Eo*)iupAttribGet(ih, "_IUP_EFL_LABEL");
    if (label)
      iupeflApplyTextStyle(ih, label);
  }

  return 1;
}

/****************************************************************
                     Methods
****************************************************************/

static void eflImageToggleChangedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  IFni cb;
  int check;

  (void)event_info;

  if (iupAttribGet(ih, "_IUP_EFL_IGNORE_TOGGLE"))
    return;

  check = elm_check_state_get(obj);

  eflToggleUpdateImage(ih, check);

  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih, check);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  iupBaseCallValueChangedCb(ih);
}

static void eflSwitchToggleChangedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  IFni cb;
  int check;

  (void)event_info;

  if (iupAttribGet(ih, "_IUP_EFL_IGNORE_TOGGLE"))
    return;

  check = elm_check_state_get(obj);

  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih, check);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  iupBaseCallValueChangedCb(ih);
}

static int eflToggleMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* toggle;
  char* value;
  char* title;
  Ihandle* radio;
  static int radio_value_counter = 1;
  int is_first_radio = 0;
  int is_switch = 0;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  value = iupAttribGet(ih, "IMAGE");
  title = iupAttribGet(ih, "TITLE");

  if (value)
    ih->data->type = IUP_TOGGLE_IMAGE;
  else
    ih->data->type = IUP_TOGGLE_TEXT;

  radio = iupRadioFindToggleParent(ih);

  if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    if (radio)
      iupAttribSet(ih, "SWITCH", "NO");
    else if (ih->data->type == IUP_TOGGLE_TEXT)
      is_switch = 1;
  }

  if (radio)
  {
    ih->data->is_radio = 1;

    if (ih->data->type == IUP_TOGGLE_IMAGE)
    {
      /* Image toggles in radio group use elm_check with icon style + manual radio behavior */
      toggle = elm_check_add(parent);
      if (!toggle)
        return IUP_ERROR;

      elm_object_style_set(toggle, "icon");

      if (!iupAttribGetHandleName(ih))
        iupAttribSetHandleName(ih);

      /* Mark this as a radio image toggle for manual exclusive handling */
      iupAttribSet(ih, "_IUP_EFL_RADIO_IMAGE", "1");

      evas_object_smart_callback_add(toggle, "changed", eflImageToggleChangedCallback, ih);
    }
    else
    {
      Eo* radio_group = (Eo*)iupAttribGet(radio, "_IUP_EFL_RADIO_GROUP");
      Eo* radio_btn;
      Eo* label;

      toggle = efl_add(EFL_UI_BOX_CLASS, parent,
                       efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                       efl_gfx_arrangement_content_padding_set(efl_added, 0, 0));
      if (!toggle)
        return IUP_ERROR;

      radio_btn = efl_add(EFL_UI_RADIO_CLASS, toggle,
                          efl_gfx_hint_align_set(efl_added, 0.0, 0.5));
      if (!radio_btn)
      {
        efl_del(toggle);
        return IUP_ERROR;
      }

      efl_pack_end(toggle, radio_btn);
      iupAttribSet(ih, "_IUP_EFL_CHECK", (char*)radio_btn);

      int my_value = radio_value_counter++;
      iupAttribSet(ih, "_IUP_EFL_RADIO_VALUE", (char*)(intptr_t)my_value);
      efl_ui_radio_state_value_set(radio_btn, my_value);

      if (radio_group)
      {
        efl_ui_radio_group_register(radio_group, radio_btn);
      }
      else
      {
        radio_group = efl_new(EFL_UI_RADIO_GROUP_IMPL_CLASS, NULL);
        iupAttribSet(radio, "_IUP_EFL_RADIO_GROUP", (char*)radio_group);
        efl_ui_radio_group_register(radio_group, radio_btn);
        efl_event_callback_add(radio_group, EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED, eflRadioGroupValueChangedCallback, radio);
        is_first_radio = 1;
      }

      if (!iupAttribGetHandleName(ih))
        iupAttribSetHandleName(ih);

      efl_event_callback_add(radio_btn, EFL_UI_EVENT_SELECTED_CHANGED, eflToggleSelectedChangedCallback, ih);

      if (title && title[0])
      {
        Evas_Object* event_rect;

        label = efl_add(EFL_UI_TEXTBOX_CLASS, toggle,
                        efl_text_interactive_editable_set(efl_added, EINA_FALSE),
                        efl_text_interactive_selection_allowed_set(efl_added, EINA_FALSE),
                        efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                        efl_gfx_hint_align_set(efl_added, 0.0, 0.5));
        if (label)
        {
          efl_text_set(label, iupeflStrConvertToSystem(title));
          efl_pack_end(toggle, label);
          iupAttribSet(ih, "_IUP_EFL_LABEL", (char*)label);

        efl_event_callback_add(label, EFL_EVENT_POINTER_UP, eflToggleLabelClickCallback, ih);
        }
      }
    }
  }
  else if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    toggle = elm_check_add(parent);
    if (!toggle)
      return IUP_ERROR;

    elm_object_style_set(toggle, "icon");

    evas_object_smart_callback_add(toggle, "changed", eflImageToggleChangedCallback, ih);
  }
  else if (is_switch)
  {
    toggle = elm_check_add(parent);
    if (!toggle)
      return IUP_ERROR;

    elm_object_style_set(toggle, "toggle");

    evas_object_smart_callback_add(toggle, "changed", eflSwitchToggleChangedCallback, ih);
  }
  else
  {
    Eo* check_box;
    Eo* label;

    toggle = efl_add(EFL_UI_BOX_CLASS, parent,
                     efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL),
                     efl_gfx_arrangement_content_padding_set(efl_added, 0, 0));
    if (!toggle)
      return IUP_ERROR;

    check_box = efl_add(EFL_UI_CHECK_CLASS, toggle,
                        efl_gfx_hint_align_set(efl_added, 0.0, 0.5));
    if (!check_box)
    {
      efl_del(toggle);
      return IUP_ERROR;
    }

    efl_pack_end(toggle, check_box);
    iupAttribSet(ih, "_IUP_EFL_CHECK", (char*)check_box);

    efl_event_callback_add(check_box, EFL_UI_EVENT_SELECTED_CHANGED, eflToggleSelectedChangedCallback, ih);

    if (title && title[0])
    {
      Evas_Object* event_rect;

      label = efl_add(EFL_UI_TEXTBOX_CLASS, toggle,
                      efl_text_interactive_editable_set(efl_added, EINA_FALSE),
                      efl_text_interactive_selection_allowed_set(efl_added, EINA_FALSE),
                      efl_gfx_hint_weight_set(efl_added, 1.0, 0.0),
                      efl_gfx_hint_align_set(efl_added, 0.0, 0.5));
      if (label)
      {
        efl_text_set(label, iupeflStrConvertToSystem(title));
        efl_pack_end(toggle, label);
        iupAttribSet(ih, "_IUP_EFL_LABEL", (char*)label);

        efl_event_callback_add(label, EFL_EVENT_POINTER_UP, eflToggleLabelClickCallback, ih);
      }
    }
  }

  ih->handle = (InativeHandle*)toggle;

  value = iupAttribGet(ih, "IMAGE");
  if (value && value[0])
  {
    Evas_Object* image = iupeflImageGetImage(value, ih, 0);
    if (image)
    {
      if (ih->data->type == IUP_TOGGLE_IMAGE)
        elm_object_content_set(toggle, image);
      else
        efl_content_set(toggle, image);
    }
  }

  value = iupAttribGet(ih, "VALUE");
  if (value && iupStrBoolean(value))
  {
    if (ih->data->is_radio && ih->data->type == IUP_TOGGLE_IMAGE)
    {
      elm_check_state_set(toggle, EINA_TRUE);
      eflToggleUpdateImage(ih, 1);
    }
    else if (ih->data->is_radio)
    {
      Eo* radio_group = (Eo*)iupAttribGet(radio, "_IUP_EFL_RADIO_GROUP");
      int my_value = (intptr_t)iupAttribGet(ih, "_IUP_EFL_RADIO_VALUE");
      efl_ui_radio_group_selected_value_set(radio_group, my_value);
    }
    else if (ih->data->type == IUP_TOGGLE_IMAGE)
    {
      elm_check_state_set(toggle, EINA_TRUE);
      eflToggleUpdateImage(ih, 1);
    }
    else if (iupAttribGetBoolean(ih, "SWITCH"))
    {
      elm_check_state_set(toggle, EINA_TRUE);
    }
    else
    {
      Eo* check_widget = (Eo*)iupAttribGet(ih, "_IUP_EFL_CHECK");
      if (check_widget)
        efl_ui_selectable_selected_set(check_widget, EINA_TRUE);
    }
  }
  else if (is_first_radio && ih->data->type != IUP_TOGGLE_IMAGE)
  {
    Eo* radio_group = (Eo*)iupAttribGet(radio, "_IUP_EFL_RADIO_GROUP");
    int my_value = (intptr_t)iupAttribGet(ih, "_IUP_EFL_RADIO_VALUE");
    efl_ui_radio_group_selected_value_set(radio_group, my_value);
  }

  {
    Eo* callback_widget = (Eo*)iupAttribGet(ih, "_IUP_EFL_CHECK");
    if (!callback_widget)
      callback_widget = toggle;
    iupeflBaseAddCallbacks(ih, callback_widget);
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE || iupAttribGetBoolean(ih, "SWITCH"))
    efl_gfx_entity_visible_set(toggle, EINA_TRUE);

  iupeflAddToParent(ih);

  if (ih->data->type == IUP_TOGGLE_TEXT && !iupAttribGetBoolean(ih, "SWITCH"))
  {
    Eo* label = (Eo*)iupAttribGet(ih, "_IUP_EFL_LABEL");
    if (label)
      iupeflApplyTextStyle(ih, label);
  }

  return IUP_NOERROR;
}

static void eflToggleUnMapMethod(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);
  Eo* check_widget;

  if (!widget)
  {
    ih->handle = NULL;
    return;
  }

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupeflBaseRemoveCallbacks(ih, widget);
    evas_object_smart_callback_del(widget, "changed", eflImageToggleChangedCallback);
    efl_del(widget);
  }
  else if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    iupeflBaseRemoveCallbacks(ih, widget);
    evas_object_smart_callback_del(widget, "changed", eflSwitchToggleChangedCallback);
    efl_del(widget);
  }
  else
  {
    check_widget = (Eo*)iupAttribGet(ih, "_IUP_EFL_CHECK");
    if (check_widget)
    {
      iupeflBaseRemoveCallbacks(ih, check_widget);
      efl_event_callback_del(check_widget, EFL_UI_EVENT_SELECTED_CHANGED, eflToggleSelectedChangedCallback, ih);

      if (ih->data->is_radio)
      {
        Ihandle* radio = iupRadioFindToggleParent(ih);
        if (radio)
        {
          Eo* radio_group = (Eo*)iupAttribGet(radio, "_IUP_EFL_RADIO_GROUP");
          if (radio_group)
            efl_ui_radio_group_unregister(radio_group, check_widget);
        }
      }
    }

    efl_del(widget);
  }

  {
    Eo* label = (Eo*)iupAttribGet(ih, "_IUP_EFL_LABEL");
    if (label)
      efl_event_callback_del(label, EFL_EVENT_POINTER_UP, eflToggleLabelClickCallback, ih);
  }

  iupAttribSet(ih, "_IUP_EFL_CHECK", NULL);
  iupAttribSet(ih, "_IUP_EFL_LABEL", NULL);
  ih->handle = NULL;
}

static void eflToggleComputeNaturalSizeMethod(Ihandle* ih, int* w, int* h, int* children_expand)
{
  int natural_w = 0, natural_h = 0;
  char* title = iupAttribGet(ih, "TITLE");

  (void)children_expand;

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupImageGetInfo(iupAttribGet(ih, "IMAGE"), &natural_w, &natural_h, NULL);
    iupdrvToggleAddBorders(ih, &natural_w, &natural_h);
  }
  else if (iupAttribGetBoolean(ih, "SWITCH"))
  {
    if (title && title[0])
      iupdrvFontGetMultiLineStringSize(ih, title, &natural_w, &natural_h);
    iupdrvToggleAddSwitch(ih, &natural_w, &natural_h, title);
  }
  else
  {
    if (title && title[0])
      iupdrvFontGetMultiLineStringSize(ih, title, &natural_w, &natural_h);
    iupdrvToggleAddCheckBox(ih, &natural_w, &natural_h, title);
  }

  natural_w += 2 * ih->data->horiz_padding;
  natural_h += 2 * ih->data->vert_padding;

  *w = natural_w;
  *h = natural_h;
}

/****************************************************************
                     Driver Functions
****************************************************************/

void iupdrvToggleAddCheckBox(Ihandle* ih, int* x, int* y, const char* str)
{
  static int check_w = -1;
  static int check_h = -1;
  (void)ih;

  if (check_w < 0)
  {
    Eo* temp_win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                           efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC),
                           efl_gfx_entity_visible_set(efl_added, EINA_FALSE));
    if (temp_win)
    {
      Eo* temp_check = efl_add(EFL_UI_CHECK_CLASS, temp_win);
      if (temp_check)
      {
        Eina_Size2D min_size = efl_gfx_hint_size_combined_min_get(temp_check);
        check_w = (min_size.w > 0) ? min_size.w : 18;
        check_h = (min_size.h > 0) ? min_size.h : 18;
        efl_del(temp_check);
      }
      else
      {
        check_w = 18;
        check_h = 18;
      }
      efl_del(temp_win);
    }
    else
    {
      check_w = 18;
      check_h = 18;
    }
  }

  (*x) += check_w;
  if ((*y) < check_h)
    (*y) = check_h;

  if (str && str[0])
    (*x) += 2;
}

void iupdrvToggleAddSwitch(Ihandle* ih, int* x, int* y, const char* str)
{
  static int switch_w = -1;
  static int switch_h = -1;
  (void)ih;

  if (switch_w < 0)
  {
    Eo* temp_win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                           efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC),
                           efl_gfx_entity_visible_set(efl_added, EINA_FALSE));
    if (temp_win)
    {
      Evas_Object* temp_switch = elm_check_add(temp_win);
      if (temp_switch)
      {
        Eina_Size2D min_size;
        elm_object_style_set(temp_switch, "toggle");
        min_size = efl_gfx_hint_size_combined_min_get(temp_switch);

        if (min_size.w > 0 && min_size.h > 0)
        {
          switch_w = min_size.w;
          switch_h = min_size.h;
        }
        else
        {
          switch_w = 80;
          switch_h = 40;
        }
        efl_del(temp_switch);
      }
      else
      {
        switch_w = 80;
        switch_h = 40;
      }
      efl_del(temp_win);
    }
    else
    {
      switch_w = 80;
      switch_h = 40;
    }
  }

  (*x) += 2 + switch_w + 2;
  if ((*y) < 2 + switch_h + 2)
    (*y) = 2 + switch_h + 2;
  else
    (*y) += 2 + 2;

  if (str && str[0])
    (*x) += 8;
}

void iupdrvToggleAddBorders(Ihandle* ih, int* x, int* y)
{
  (void)ih;
  *x += 4;
  *y += 4;
}

void iupdrvToggleInitClass(Iclass* ic)
{
  ic->Map = eflToggleMapMethod;
  ic->UnMap = eflToggleUnMapMethod;
  ic->ComputeNaturalSize = eflToggleComputeNaturalSizeMethod;

  iupClassRegisterAttribute(ic, "VALUE", eflToggleGetValueAttrib, eflToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, eflToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, eflToggleSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, eflToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, eflToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupeflBaseGetActiveAttrib, eflToggleSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflToggleSetBgColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, eflToggleSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FONT", NULL, eflToggleSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
}
