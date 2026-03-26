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
#include "iup_drvfont.h"

#include "iupefl_drv.h"


static Eo* eflTipGetWidget(Ihandle* ih)
{
  Eo* widget = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (!widget)
    widget = iupeflGetWidget(ih);
  return widget;
}


static void eflTipSetLabelText(Evas_Object* label, Ihandle* ih, const char* tip)
{
  const char* tipfont = iupAttribGet(ih, "TIPFONT");
  const char* tipfgcolor = iupAttribGet(ih, "TIPFGCOLOR");
  const char* tipbgcolor = iupAttribGet(ih, "TIPBGCOLOR");

  if (tipfont || tipfgcolor || tipbgcolor)
  {
    char markup[1024];
    char open_tags[256] = "";
    char close_tags[256] = "";
    int open_pos = 0, close_pos = 0;
    unsigned char r, g, b;

    if (tipfont && !iupStrEqualNoCase(tipfont, "SYSTEM"))
    {
      char typeface[50];
      int size, is_bold, is_italic, is_underline, is_strikeout;
      if (iupGetFontInfo(tipfont, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
      {
        if (size < 0) size = -size;
        open_pos += snprintf(open_tags + open_pos, sizeof(open_tags) - open_pos, "<font=%s><font_size=%d>", typeface, size);
        close_pos += snprintf(close_tags + close_pos, sizeof(close_tags) - close_pos, "</font_size></font>");
      }
    }

    if (tipfgcolor && iupStrToRGB(tipfgcolor, &r, &g, &b))
    {
      open_pos += snprintf(open_tags + open_pos, sizeof(open_tags) - open_pos, "<color=#%02X%02X%02X>", r, g, b);
      close_pos += snprintf(close_tags + close_pos, sizeof(close_tags) - close_pos, "</color>");
    }

    (void)tipbgcolor;
    (void)open_pos;
    (void)close_pos;

    snprintf(markup, sizeof(markup), "%s%s%s", open_tags, tip, close_tags);
    elm_object_text_set(label, markup);
  }
  else
    elm_object_text_set(label, tip);
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
        efl_gfx_hint_size_min_set(ic, EINA_SIZE2D(16, 16));
        efl_gfx_hint_size_max_set(ic, EINA_SIZE2D(16, 16));
        elm_box_pack_end(box, ic);
        efl_gfx_entity_visible_set(ic, EINA_TRUE);
      }
      else if (ic)
        efl_del(ic);
    }

    label = elm_label_add(tooltip);
    eflTipSetLabelText(label, ih, tip);
    elm_box_pack_end(box, label);
    efl_gfx_entity_visible_set(label, EINA_TRUE);

    return box;
  }

  content = elm_label_add(tooltip);
  eflTipSetLabelText(content, ih, tip);
  return content;
}

IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  Evas_Object* widget;

  widget = eflTipGetWidget(ih);
  if (!widget)
    return 1;

  if (value && *value)
  {
    if (IupGetCallback(ih, "TIPS_CB") || iupAttribGet(ih, "TIPICON") ||
        iupAttribGet(ih, "TIPFONT") || iupAttribGet(ih, "TIPFGCOLOR") || iupAttribGet(ih, "TIPBGCOLOR"))
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

IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
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

IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  (void)ih;
  return NULL;
}
