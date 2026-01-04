/** \file
 * \brief Tabs Control
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
#include "iup_tabs.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupefl_drv.h"


/****************************************************************
                     Tab Reorder Support
****************************************************************/

static Eo* efl_drag_indicator = NULL;

static void eflTabsAddReorderCallbacks(Ihandle* ih, Eo* item);
static void eflTabsRemoveReorderCallbacks(Ihandle* ih, Eo* item);

static int eflTabsGetItemPosition(Ihandle* ih, Eo* tab_item)
{
  Eo* pager = iupeflGetWidget(ih);
  int count;
  int i;

  if (!pager)
    return -1;

  count = efl_content_count(pager);

  for (i = 0; i < count; i++)
  {
    Eo* page = efl_pack_content_get(pager, i);
    if (page)
    {
      Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
      if (item == tab_item)
        return i;
    }
  }
  return -1;
}

static int eflTabsFindTargetPosition(Ihandle* ih, int x, int y)
{
  Eo* pager = iupeflGetWidget(ih);
  int count;
  int i;

  (void)y;

  if (!pager)
    return -1;

  count = efl_content_count(pager);

  for (i = 0; i < count; i++)
  {
    Eo* page = efl_pack_content_get(pager, i);
    if (page)
    {
      Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
      if (item)
      {
        Eina_Rect geom = efl_gfx_entity_geometry_get(item);
        int mid_x = geom.x + geom.w / 2;
        if (x < mid_x)
          return i;
      }
    }
  }
  return count - 1;
}

static void eflTabsHideDragIndicator(Ihandle* ih)
{
  (void)ih;
  if (efl_drag_indicator)
    evas_object_hide(efl_drag_indicator);
}

static void eflTabsUpdateDragIndicator(Ihandle* ih, int target)
{
  Eo* pager = iupeflGetWidget(ih);
  Eo* tab_bar;
  Eo* page;
  Eo* item;
  Eina_Rect geom;
  int source;
  int x;

  source = iupAttribGetInt(ih, "_IUPTABS_DRAG_SOURCE");

  if (target == source)
  {
    eflTabsHideDragIndicator(ih);
    return;
  }

  tab_bar = efl_ui_tab_pager_tab_bar_get(pager);
  if (!tab_bar)
    return;

  if (!efl_drag_indicator)
  {
    Evas* evas = evas_object_evas_get(tab_bar);
    efl_drag_indicator = evas_object_rectangle_add(evas);
    evas_object_color_set(efl_drag_indicator, 0, 120, 215, 255);
  }

  page = efl_pack_content_get(pager, target);
  if (!page)
    return;

  item = efl_ui_tab_page_tab_bar_item_get(page);
  if (!item)
    return;

  geom = efl_gfx_entity_geometry_get(item);

  x = (source < target) ? geom.x + geom.w - 1 : geom.x;
  evas_object_geometry_set(efl_drag_indicator, x, geom.y, 2, geom.h);
  evas_object_show(efl_drag_indicator);
  evas_object_raise(efl_drag_indicator);
}

static void eflTabsReorderTab(Ihandle* ih, int source, int target)
{
  Eo* pager = iupeflGetWidget(ih);
  Eo* page;
  Eo* item;
  Ihandle* child;
  Ihandle* ref_child;
  int insert_pos;
  int current_tab;

  current_tab = iupdrvTabsGetCurrentTab(ih);

  page = efl_pack_content_get(pager, source);
  if (!page)
    return;

  item = efl_ui_tab_page_tab_bar_item_get(page);
  if (item)
    eflTabsRemoveReorderCallbacks(ih, item);

  efl_pack_unpack(pager, page);

  if (source < target)
    insert_pos = target;
  else
    insert_pos = target;

  efl_pack_at(pager, page, insert_pos);

  item = efl_ui_tab_page_tab_bar_item_get(page);
  if (item)
    eflTabsAddReorderCallbacks(ih, item);

  child = IupGetChild(ih, source);
  if (child)
  {
    if (source < target)
      ref_child = IupGetChild(ih, target + 1);
    else
      ref_child = IupGetChild(ih, target);

    iupAttribSet(ih, "_IUPTABS_REORDERING", "1");
    IupReparent(child, ih, ref_child);
    iupAttribSet(ih, "_IUPTABS_REORDERING", NULL);
  }

  if (current_tab == source)
    current_tab = target;
  else if (source < target && current_tab > source && current_tab <= target)
    current_tab--;
  else if (source > target && current_tab >= target && current_tab < source)
    current_tab++;

  iupdrvTabsSetCurrentTab(ih, current_tab);
  iupAttribSetInt(ih, "_IUP_EFL_PREV_POS", current_tab);

  IupRefresh(ih);
}

static void eflTabsDragPointerDown(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer *pointer = ev->info;
  Eo* clicked_item;
  int pos;
  Eina_Position2D pointer_pos;

  if (!iupAttribGetBoolean(ih, "ALLOWREORDER"))
    return;

  if (efl_input_pointer_button_get(pointer) != 1)
    return;

  clicked_item = ev->object;
  pos = eflTabsGetItemPosition(ih, clicked_item);
  if (pos < 0)
    return;

  pointer_pos = efl_input_pointer_position_get(pointer);

  iupAttribSetInt(ih, "_IUPTABS_DRAG_SOURCE", pos);
  iupAttribSetInt(ih, "_IUPTABS_DRAG_TARGET", pos);
  iupAttribSetInt(ih, "_IUPTABS_DRAG_START_X", pointer_pos.x);
  iupAttribSetInt(ih, "_IUPTABS_DRAG_START_Y", pointer_pos.y);
}

static void eflTabsDragPointerMove(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer *pointer = ev->info;
  Eina_Position2D pointer_pos;
  int start_x, start_y;
  int target, old_target;

  if (!iupAttribGet(ih, "_IUPTABS_DRAG_SOURCE"))
    return;

  pointer_pos = efl_input_pointer_position_get(pointer);

  if (!iupAttribGetInt(ih, "_IUPTABS_DRAGGING"))
  {
    start_x = iupAttribGetInt(ih, "_IUPTABS_DRAG_START_X");
    start_y = iupAttribGetInt(ih, "_IUPTABS_DRAG_START_Y");
    if (abs(pointer_pos.x - start_x) > 5 || abs(pointer_pos.y - start_y) > 5)
      iupAttribSetInt(ih, "_IUPTABS_DRAGGING", 1);
    else
      return;
  }

  target = eflTabsFindTargetPosition(ih, pointer_pos.x, pointer_pos.y);
  if (target >= 0)
  {
    old_target = iupAttribGetInt(ih, "_IUPTABS_DRAG_TARGET");
    if (target != old_target)
    {
      iupAttribSetInt(ih, "_IUPTABS_DRAG_TARGET", target);
      eflTabsUpdateDragIndicator(ih, target);
    }
  }
}

static void eflTabsDragPointerUp(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  int is_dragging;
  int source;
  int target;

  (void)ev;

  is_dragging = iupAttribGetInt(ih, "_IUPTABS_DRAGGING");
  source = iupAttribGetInt(ih, "_IUPTABS_DRAG_SOURCE");
  target = iupAttribGetInt(ih, "_IUPTABS_DRAG_TARGET");

  iupAttribSet(ih, "_IUPTABS_DRAGGING", NULL);
  iupAttribSet(ih, "_IUPTABS_DRAG_SOURCE", NULL);
  iupAttribSet(ih, "_IUPTABS_DRAG_TARGET", NULL);
  iupAttribSet(ih, "_IUPTABS_DRAG_START_X", NULL);
  iupAttribSet(ih, "_IUPTABS_DRAG_START_Y", NULL);

  eflTabsHideDragIndicator(ih);

  if (is_dragging && source != target)
    eflTabsReorderTab(ih, source, target);
}

static void eflTabsAddReorderCallbacks(Ihandle* ih, Eo* item)
{
  efl_event_callback_add(item, EFL_EVENT_POINTER_DOWN, eflTabsDragPointerDown, ih);
  efl_event_callback_add(item, EFL_EVENT_POINTER_MOVE, eflTabsDragPointerMove, ih);
  efl_event_callback_add(item, EFL_EVENT_POINTER_UP, eflTabsDragPointerUp, ih);
}

static void eflTabsRemoveReorderCallbacks(Ihandle* ih, Eo* item)
{
  efl_event_callback_del(item, EFL_EVENT_POINTER_DOWN, eflTabsDragPointerDown, ih);
  efl_event_callback_del(item, EFL_EVENT_POINTER_MOVE, eflTabsDragPointerMove, ih);
  efl_event_callback_del(item, EFL_EVENT_POINTER_UP, eflTabsDragPointerUp, ih);
}

static int eflTabsSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

/****************************************************************
                     Tab Item Helper
****************************************************************/

static void eflTabsSetItemIcon(Eo* item, const char* tabimage, Ihandle* ih)
{
  Eo* efl_img;

  if (!item || !tabimage)
    return;

  efl_img = iupeflImageGetImage(tabimage, ih, 0);
  if (efl_img)
  {
    efl_gfx_entity_visible_set(efl_img, EINA_TRUE);
    efl_content_set(efl_part(item, "icon"), efl_img);
  }
}

int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvTabsExtraMargin(void)
{
  return 0;
}

int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  (void)ih;
  return 1;
}

void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  Eo* pager = iupeflGetWidget(ih);

  if (pager)
  {
    Eo* page = efl_pack_content_get(pager, pos);
    if (page)
    {
      iupAttribSet(ih, "_IUP_EFL_IGNORE_CHANGE", "1");
      Eo* tab_bar = efl_ui_tab_pager_tab_bar_get(pager);
      if (tab_bar)
      {
        Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
        if (item)
          efl_ui_selectable_selected_set(item, EINA_TRUE);
      }
      iupAttribSet(ih, "_IUP_EFL_IGNORE_CHANGE", NULL);
    }
  }
}

int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  Eo* pager = iupeflGetWidget(ih);

  if (pager)
  {
    Eo* tab_bar = efl_ui_tab_pager_tab_bar_get(pager);
    if (tab_bar)
    {
      Eo* selected = efl_ui_selectable_last_selected_get(tab_bar);
      if (selected)
      {
        Eo* page = efl_parent_get(selected);
        if (page)
          return efl_pack_index_get(pager, page);
      }
    }
  }

  return 0;
}

int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  Eo* page = (Eo*)iupAttribGet(child, "_IUPTAB_PAGE");
  (void)pos;
  if (page)
    return efl_gfx_entity_visible_get(page);
  return 0;
}

/****************************************************************
                     Callbacks
****************************************************************/

static void eflTabsLayoutJob(void* data)
{
  Ihandle* ih = (Ihandle*)data;
  iupAttribSet(ih, "_IUP_EFL_LAYOUT_JOB", NULL);
  iupLayoutUpdate(ih);
}

static void eflTabsItemSelectedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* selected;
  Eo* pager;
  int pos, prev_pos;
  IFnnn cb;

  if (iupAttribGet(ih, "_IUP_EFL_IGNORE_CHANGE"))
    return;

  pager = iupeflGetWidget(ih);
  if (!pager)
    return;

  selected = efl_ui_selectable_last_selected_get(ev->object);
  if (!selected)
    return;

  Eo* page = efl_parent_get(selected);
  if (!page)
    return;

  pos = efl_pack_index_get(pager, page);
  prev_pos = iupAttribGetInt(ih, "_IUP_EFL_PREV_POS");

  if (pos == prev_pos)
    return;

  iupAttribSetInt(ih, "_IUP_EFL_PREV_POS", pos);

  if (!iupAttribGet(ih, "_IUP_EFL_LAYOUT_JOB"))
  {
    Ecore_Job* job = ecore_job_add(eflTabsLayoutJob, ih);
    iupAttribSet(ih, "_IUP_EFL_LAYOUT_JOB", (char*)job);
  }

  cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
  if (cb)
  {
    Ihandle* child = IupGetChild(ih, pos);
    Ihandle* prev_child = IupGetChild(ih, prev_pos);
    cb(ih, child, prev_child);
  }
  else
  {
    IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
    if (cb2)
      cb2(ih, pos, prev_pos);
  }
}

static void eflTabsCloseButtonClicked(void* data, const Efl_Event* ev)
{
  Ihandle* child = (Ihandle*)data;
  Ihandle* ih;
  Eo* page;
  Eo* pager;
  int pos;
  int ret = IUP_DEFAULT;
  IFni cb;

  (void)ev;

  if (!child)
    return;

  ih = IupGetParent(child);
  if (!ih)
    return;

  page = (Eo*)iupAttribGet(child, "_IUPTAB_PAGE");
  pager = iupeflGetWidget(ih);
  if (!page || !pager)
    return;

  pos = efl_pack_index_get(pager, page);

  cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
  if (cb)
    ret = cb(ih, pos);

  if (ret == IUP_CONTINUE)
  {
    IupDestroy(child);
    IupRefreshChildren(ih);
  }
  else if (ret == IUP_DEFAULT)
  {
    efl_gfx_entity_visible_set(page, EINA_FALSE);
  }
}

static Eo* eflTabsCreateCloseButton(Ihandle* ih, Ihandle* child, Eo* item)
{
  Eo* btn;
  Eo* icon;
  Eo* parent;

  parent = efl_parent_get(item);
  if (!parent)
    parent = iupeflGetWidget(ih);

  btn = efl_add(EFL_UI_BUTTON_CLASS, parent,
    efl_ui_widget_style_set(efl_added, "anchor"));
  if (!btn)
    return NULL;

  icon = efl_add(EFL_UI_IMAGE_CLASS, btn);
  if (icon)
  {
    efl_file_simple_load(icon, "window-close", NULL);
    efl_content_set(btn, icon);
    efl_gfx_entity_visible_set(icon, EINA_TRUE);
  }
  else
  {
    efl_text_set(btn, "X");
  }

  efl_event_callback_add(btn, EFL_INPUT_EVENT_CLICKED, eflTabsCloseButtonClicked, child);
  efl_gfx_entity_visible_set(btn, EINA_TRUE);

  efl_content_set(efl_part(item, "efl.extra"), btn);

  return btn;
}

/****************************************************************
                     Attributes
****************************************************************/

static int eflTabsSetTabPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  return 1;
}

static int eflTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    iupAttribSetStr(child, "TABTITLE", value);

    if (ih->handle && value)
    {
      Eo* page = (Eo*)iupAttribGet(child, "_IUPTAB_PAGE");
      if (page)
      {
        Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
        if (item)
          efl_text_set(item, iupeflStrConvertToSystem(value));
      }
    }
  }

  return 0;
}

static int eflTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    iupAttribSetStr(child, "TABIMAGE", value);

    if (ih->handle && value)
    {
      Eo* page = (Eo*)iupAttribGet(child, "_IUPTAB_PAGE");
      if (page)
      {
        Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
        if (item)
          eflTabsSetItemIcon(item, value, ih);
      }
    }
  }

  return 1;
}

static int eflTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    Eo* page = (Eo*)iupAttribGet(child, "_IUPTAB_PAGE");
    if (page)
    {
      if (iupStrBoolean(value))
        efl_gfx_entity_visible_set(page, EINA_TRUE);
      else
        efl_gfx_entity_visible_set(page, EINA_FALSE);
    }
  }

  return 0;
}

static int eflTabsSetBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

static int eflTabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

/****************************************************************
                     Methods
****************************************************************/

static void eflTabsPositionWidget(Ihandle* ih, int content_x, int content_y, Eina_Bool visible)
{
  Ihandle* child;

  if (ih->iclass->nativetype != IUP_TYPEVOID && ih->handle)
  {
    Eo* widget = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
    if (!widget)
      widget = iupeflGetWidget(ih);

    if (widget)
    {
      int x = content_x + ih->x;
      int y = content_y + ih->y;

      iupeflSetPosition(widget, x, y);
      iupeflSetSize(widget, ih->currentwidth, ih->currentheight);
      efl_gfx_entity_visible_set(widget, visible);
    }
  }

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
      eflTabsPositionWidget(child, content_x, content_y, visible);
  }
}

static void eflTabsLayoutUpdateMethod(Ihandle* ih)
{
  Ihandle* child;
  int current_tab;
  int pos;

  iupdrvBaseLayoutUpdateMethod(ih);

  current_tab = iupdrvTabsGetCurrentTab(ih);

  pos = 0;
  for (child = ih->firstchild; child; child = child->brother, pos++)
  {
    Eina_Bool is_active = (pos == current_tab);
    Eo* content_box = (Eo*)iupAttribGet(child, "_IUPTAB_CONTAINER");
    int content_x = 0, content_y = 0;

    if (content_box)
    {
      Eina_Rect content_geom = efl_gfx_entity_geometry_get(content_box);
      content_x = content_geom.x;
      content_y = content_geom.y;
    }

    eflTabsPositionWidget(child, content_x, content_y, is_active);
  }
}

void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
  int width = 0;
  int height = 0;
  int charwidth, charheight;

  iupdrvFontGetCharSize(ih, &charwidth, &charheight);

  if (tab_title)
  {
    width = iupdrvFontGetStringWidth(ih, tab_title);
    height = charheight;
  }

  if (tab_image)
  {
    void* img = iupImageGetImage(tab_image, ih, 0, NULL);
    if (img)
    {
      int img_w, img_h;
      iupdrvImageGetInfo(img, &img_w, &img_h, NULL);

      width += img_w;
      if (tab_title)
        width += 8;

      if (img_h > height)
        height = img_h;
    }
  }

  width += 2 * ih->data->horiz_padding + 32;
  height += 2 * ih->data->vert_padding + 16;

  if (ih->data->show_close)
    width += 20;

  if (tab_width) *tab_width = width;
  if (tab_height) *tab_height = height;
}

static void eflTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (!iupAttribGetHandleName(child))
    iupAttribSetHandleName(child);

  if (ih->handle)
  {
    if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
      return;
    Eo* pager = iupeflGetWidget(ih);
    Eo* page;
    Eo* content_box;
    char *tabtitle, *tabimage;
    int pos;

    pos = IupGetChildPos(ih, child);

    page = efl_add(EFL_UI_TAB_PAGE_CLASS, pager);
    if (!page)
      return;

    content_box = efl_add(EFL_UI_BOX_CLASS, page);
    if (content_box)
    {
      efl_gfx_hint_weight_set(content_box, 1.0, 1.0);
      efl_gfx_hint_fill_set(content_box, EINA_TRUE, EINA_TRUE);
      efl_content_set(page, content_box);
    }

    tabtitle = iupAttribGet(child, "TABTITLE");
    if (!tabtitle)
    {
      tabtitle = iupAttribGetId(ih, "TABTITLE", pos);
      if (tabtitle)
        iupAttribSetStr(child, "TABTITLE", tabtitle);
    }

    tabimage = iupAttribGet(child, "TABIMAGE");
    if (!tabimage)
    {
      tabimage = iupAttribGetId(ih, "TABIMAGE", pos);
      if (tabimage)
        iupAttribSetStr(child, "TABIMAGE", tabimage);
    }

    if (!tabtitle && !tabimage)
      tabtitle = "     ";

    Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
    if (item)
    {
      if (tabtitle)
        efl_text_set(item, iupeflStrConvertToSystem(tabtitle));
      if (tabimage)
        eflTabsSetItemIcon(item, tabimage, ih);

      if (ih->data->show_close)
      {
        Evas_Object* close_btn = eflTabsCreateCloseButton(ih, child, item);
        iupAttribSet(child, "_IUPTAB_CLOSE", (char*)close_btn);
      }

      eflTabsAddReorderCallbacks(ih, item);
    }

    efl_pack_at(pager, page, pos);

    iupAttribSet(child, "_IUPTAB_PAGE", (char*)page);
    iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)content_box);
  }
}

static void eflTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (ih->handle)
  {
    if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
      return;

    Eo* pager = iupeflGetWidget(ih);
    Eo* page = (Eo*)iupAttribGet(child, "_IUPTAB_PAGE");
    Eo* close_btn = (Eo*)iupAttribGet(child, "_IUPTAB_CLOSE");

    if (close_btn)
    {
      efl_event_callback_del(close_btn, EFL_INPUT_EVENT_CLICKED, eflTabsCloseButtonClicked, child);
      efl_del(close_btn);
    }

    if (page)
    {
      Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
      if (item)
        eflTabsRemoveReorderCallbacks(ih, item);
    }

    if (page && pager)
    {
      efl_pack_unpack(pager, page);
      iupeflDelete(page);
    }

    iupTabsCheckCurrentTab(ih, pos, 1);
  }

  child->handle = NULL;

  iupAttribSet(child, "_IUPTAB_PAGE", NULL);
  iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
  iupAttribSet(child, "_IUPTAB_CLOSE", NULL);
}

static int eflTabsMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* pager;
  Eo* tab_bar;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  pager = efl_add(EFL_UI_TAB_PAGER_CLASS, parent);
  if (!pager)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)pager;

  tab_bar = efl_ui_tab_pager_tab_bar_get(pager);
  if (tab_bar)
  {
    efl_event_callback_add(tab_bar, EFL_UI_EVENT_ITEM_SELECTED,
                            eflTabsItemSelectedCallback, ih);
  }

  iupAttribSetInt(ih, "_IUP_EFL_PREV_POS", 0);

  if (ih->firstchild)
  {
    Ihandle* child_iter;
    Ihandle* current_child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");

    for (child_iter = ih->firstchild; child_iter; child_iter = child_iter->brother)
      eflTabsChildAddedMethod(ih, child_iter);

    if (current_child)
    {
      IupSetAttribute(ih, "VALUE_HANDLE", (char*)current_child);
      iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", NULL);
    }

    if (efl_content_count(pager) > 0)
    {
      Eo* first_page = efl_pack_content_get(pager, 0);
      if (first_page)
      {
        Eo* first_item = efl_ui_tab_page_tab_bar_item_get(first_page);
        if (first_item)
          efl_ui_selectable_selected_set(first_item, EINA_TRUE);
      }
    }
  }

  iupeflBaseAddCallbacks(ih, pager);

  iupeflAddToParent(ih);

  return IUP_NOERROR;
}

static void eflTabsUnMapMethod(Ihandle* ih)
{
  Eo* pager = iupeflGetWidget(ih);

  if (pager)
  {
    Eo* tab_bar = efl_ui_tab_pager_tab_bar_get(pager);
    if (tab_bar)
    {
      efl_event_callback_del(tab_bar, EFL_UI_EVENT_ITEM_SELECTED,
                              eflTabsItemSelectedCallback, ih);
    }

    iupeflDelete(pager);
  }

  ih->handle = NULL;
}

void iupdrvTabsInitClass(Iclass* ic)
{
  ic->Map = eflTabsMapMethod;
  ic->UnMap = eflTabsUnMapMethod;
  ic->LayoutUpdate = eflTabsLayoutUpdateMethod;
  ic->ChildAdded = eflTabsChildAddedMethod;
  ic->ChildRemoved = eflTabsChildRemovedMethod;

  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");

  /* TABTYPE is read-only - EFL Tab_Pager only supports TOP position (theme-controlled) */
  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, NULL, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, eflTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, eflTabsSetTabImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, eflTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABPADDING", iupTabsGetTabPaddingAttrib, eflTabsSetTabPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, eflTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, eflTabsSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MULTILINE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
