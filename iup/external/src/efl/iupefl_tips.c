/** \file
 * \brief EFL Driver tooltips
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"

#include "iupefl_drv.h"


static Eo* eflTipGetWidget(Ihandle* ih)
{
  Eo* widget = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget)
    widget = iupeflGetWidget(ih);
  return widget;
}

static Evas_Object* eflTipContentCb(void* data, Evas_Object* obj, Evas_Object* tooltip)
{
  Ihandle* ih = (Ihandle*)data;
  const char* tip;
  const char* icon_name;
  IFnii tips_cb;
  Evas_Object* content;

  (void)obj;

  tips_cb = (IFnii)IupGetCallback(ih, "TIPS_CB");
  if (tips_cb)
  {
    int x, y;
    iupdrvGetCursorPos(&x, &y);
    iupdrvScreenToClient(ih, &x, &y);
    tips_cb(ih, x, y);
  }

  tip = iupAttribGet(ih, "TIP");
  if (!tip || !*tip)
    return NULL;

  icon_name = iupAttribGet(ih, "TIPICON");

  if (icon_name)
  {
    Evas_Object* icon;
    Evas_Object* label;
    Evas_Object* box;

    box = elm_box_add(tooltip);
    elm_box_horizontal_set(box, EINA_TRUE);
    elm_box_padding_set(box, 4, 0);

    icon = (Evas_Object*)iupImageGetIcon(icon_name);
    if (icon)
    {
      Evas_Object* ic = elm_icon_add(tooltip);
      if (ic && elm_icon_standard_set(ic, icon_name))
      {
        evas_object_size_hint_min_set(ic, 16, 16);
        evas_object_size_hint_max_set(ic, 16, 16);
        elm_box_pack_end(box, ic);
        evas_object_show(ic);
      }
      else if (ic)
        evas_object_del(ic);
    }

    label = elm_label_add(tooltip);
    elm_object_text_set(label, tip);
    elm_box_pack_end(box, label);
    evas_object_show(label);

    return box;
  }

  content = elm_label_add(tooltip);
  elm_object_text_set(content, tip);
  return content;
}

int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  Evas_Object* widget;

  widget = eflTipGetWidget(ih);
  if (!widget)
    return 1;

  if (value && *value)
  {
    if (IupGetCallback(ih, "TIPS_CB") || iupAttribGet(ih, "TIPICON"))
      elm_object_tooltip_content_cb_set(widget, eflTipContentCb, ih, NULL);
    else
      elm_object_tooltip_text_set(widget, value);
  }
  else
  {
    const char* old_tip = iupAttribGet(ih, "TIP");
    if (old_tip && *old_tip)
      elm_object_tooltip_unset(widget);
  }

  return 1;
}

int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  Evas_Object* widget;

  widget = eflTipGetWidget(ih);
  if (!widget)
    return 0;

  if (iupStrBoolean(value))
  {
    const char* tip = iupAttribGet(ih, "TIP");
    if (tip && *tip)
      elm_object_tooltip_show(widget);
  }
  else
    elm_object_tooltip_hide(widget);

  return 0;
}

char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  (void)ih;
  return NULL;
}
