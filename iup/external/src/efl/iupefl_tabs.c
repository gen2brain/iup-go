/** \file
 * \brief Tabs Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_tabs.h"
#include "iup_drvfont.h"

#include "iupefl_drv.h"


/****************************************************************
                     Tab Reorder Support
****************************************************************/

static Eo* efl_drag_indicator = NULL;

static void eflTabsAddReorderCallbacks(Ihandle* ih, Eo* item);
static void eflTabsRemoveReorderCallbacks(Ihandle* ih, Eo* item);

static Ihandle* eflTabsChildAtPos(Ihandle* ih, int pos)
{
  Ihandle* removed = (Ihandle*)iupAttribGet(ih, "_IUPEFL_REMOVED_CHILD");
  if (removed)
  {
    int removed_pos = iupAttribGetInt(ih, "_IUPEFL_REMOVED_POS");
    if (pos == removed_pos)
      return removed;
    if (pos > removed_pos)
      pos--;
  }
  return IupGetChild(ih, pos);
}

static int eflTabsPageToPos(Ihandle* ih, Eo* page)
{
  Ihandle* removed = (Ihandle*)iupAttribGet(ih, "_IUPEFL_REMOVED_CHILD");
  int removed_pos = removed ? iupAttribGetInt(ih, "_IUPEFL_REMOVED_POS") : -1;
  Ihandle* child;
  int pos = 0;

  if (!page)
    return -1;

  for (child = ih->firstchild; child; child = child->brother, pos++)
  {
    if (pos == removed_pos)
      pos++;
    if ((Eo*)iupAttribGet(child, "_IUPTAB_PAGE") == page)
      return pos;
  }

  if (removed && (Eo*)iupAttribGet(removed, "_IUPTAB_PAGE") == page)
    return removed_pos;
  return -1;
}

static int eflTabsIndexToPos(Ihandle* ih, int index)
{
  Eo* pager = iupeflGetWidget(ih);
  return pager ? eflTabsPageToPos(ih, efl_pack_content_get(pager, index)) : -1;
}

static int eflTabsInsertIndex(Ihandle* ih, Ihandle* child)
{
  Ihandle* c;
  int index = 0;
  for (c = ih->firstchild; c && c != child; c = c->brother)
  {
    if (iupAttribGet(c, "_IUPTAB_PAGE") && !iupAttribGetInt(c, "_IUPEFL_TAB_HIDDEN"))
      index++;
  }
  return index;
}

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
    efl_gfx_entity_visible_set(efl_drag_indicator, EINA_FALSE);
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
    efl_drag_indicator = efl_add(EFL_CANVAS_RECTANGLE_CLASS, evas_object_evas_get(tab_bar));
    efl_gfx_color_set(efl_drag_indicator, 0, 120, 215, 255);
  }

  page = efl_pack_content_get(pager, target);
  if (!page)
    return;

  item = efl_ui_tab_page_tab_bar_item_get(page);
  if (!item)
    return;

  geom = efl_gfx_entity_geometry_get(item);

  x = (source < target) ? geom.x + geom.w - 1 : geom.x;
  efl_gfx_entity_position_set(efl_drag_indicator, EINA_POSITION2D(x, geom.y));
  efl_gfx_entity_size_set(efl_drag_indicator, EINA_SIZE2D(2, geom.h));
  efl_gfx_entity_visible_set(efl_drag_indicator, EINA_TRUE);
  efl_gfx_stack_raise_to_top(efl_drag_indicator);
}

static void eflTabsReorderTab(Ihandle* ih, int source, int target)
{
  Eo* pager = iupeflGetWidget(ih);
  Eo* page;
  Eo* ref_page;
  Eo* item;
  Ihandle* child;
  Ihandle* ref_child;
  Ihandle* current;
  int old_pos, new_pos;

  page = efl_pack_content_get(pager, source);
  if (!page)
    return;

  old_pos = eflTabsPageToPos(ih, page);
  child = IupGetChild(ih, old_pos);
  if (!child)
    return;

  ref_page = efl_pack_content_get(pager, source < target ? target + 1 : target);
  ref_child = ref_page ? IupGetChild(ih, eflTabsPageToPos(ih, ref_page)) : NULL;
  if (ref_child)
  {
    int ref_pos = IupGetChildPos(ih, ref_child);
    new_pos = old_pos < ref_pos ? ref_pos - 1 : ref_pos;
  }
  else
    new_pos = IupGetChildCount(ih) - 1;

  {
    IFnii cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
    if (cb && cb(ih, old_pos, new_pos) == IUP_IGNORE)
      return;
  }

  current = IupGetChild(ih, iupdrvTabsGetCurrentTab(ih));

  item = efl_ui_tab_page_tab_bar_item_get(page);
  if (item)
    eflTabsRemoveReorderCallbacks(ih, item);

  efl_pack_unpack(pager, page);

  efl_pack_at(pager, page, target);

  item = efl_ui_tab_page_tab_bar_item_get(page);
  if (item)
    eflTabsAddReorderCallbacks(ih, item);

  iupAttribSet(ih, "_IUPTABS_REORDERING", "1");
  IupReparent(child, ih, ref_child);
  iupAttribSet(ih, "_IUPTABS_REORDERING", NULL);

  if (current)
  {
    iupdrvTabsSetCurrentTab(ih, IupGetChildPos(ih, current));
    iupAttribSet(ih, "_IUP_EFL_PREV_CHILD", (char*)current);
  }

  IupRefresh(ih);
}

static void eflTabsDragPointerDown(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  Eo* clicked_item;
  int pos;
  int button;
  Eina_Position2D pointer_pos;

  button = efl_input_pointer_button_get(pointer);

  if (button == 3)
  {
    IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
    if (cb)
    {
      pos = eflTabsIndexToPos(ih, eflTabsGetItemPosition(ih, ev->object));
      if (pos >= 0)
        cb(ih, pos);
    }
    return;
  }

  if (!iupAttribGetBoolean(ih, "ALLOWREORDER"))
    return;

  if (button != 1)
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

static void eflTabsDragPointerMove(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
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

static void eflTabsDragPointerUp(void* data, const Efl_Event* ev)
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
  int raw_w = 0, raw_h = 0, dst_w, dst_h;

  if (!item || !tabimage)
    return;

  iupImageGetInfo(tabimage, &raw_w, &raw_h, NULL);
  dst_w = raw_w;
  dst_h = raw_h;
  if (raw_w > 0 && raw_h > 0)
    iupTabsScaleImageSize(ih, raw_w, raw_h, &dst_w, &dst_h);

  efl_img = iupeflImageGetImage(tabimage, ih, 0);
  if (efl_img)
  {
    if (dst_w > 0 && dst_h > 0 && (dst_w != raw_w || dst_h != raw_h))
    {
      evas_object_image_smooth_scale_set(efl_img, EINA_TRUE);
      efl_gfx_hint_size_min_set(efl_img, EINA_SIZE2D(dst_w, dst_h));
      efl_gfx_hint_size_max_set(efl_img, EINA_SIZE2D(dst_w, dst_h));
      efl_gfx_entity_size_set(efl_img, EINA_SIZE2D(dst_w, dst_h));
    }
    efl_gfx_entity_visible_set(efl_img, EINA_TRUE);
    efl_content_set(efl_part(item, "icon"), efl_img);
  }
}

IUP_SDK_API int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

IUP_SDK_API int iupdrvTabsExtraMargin(void)
{
  return 4;
}

IUP_SDK_API int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  (void)ih;
  return 1;
}

static void eflTabsShowSelectedTab(Ihandle* ih)
{
  Eo* scroller = (Eo*)iupAttribGet(ih, "_IUP_EFL_TAB_SCROLLER");
  Eo* pager = iupeflGetWidget(ih);
  Eo* tab_bar = pager ? efl_ui_tab_pager_tab_bar_get(pager) : NULL;
  Eo* selected = tab_bar ? efl_ui_selectable_last_selected_get(tab_bar) : NULL;
  Eo* box = scroller ? efl_content_get(scroller) : NULL;
  Eina_Rect item, content;

  if (!selected || !box)
    return;

  efl_canvas_group_calculate(tab_bar);
  item = efl_gfx_entity_geometry_get(selected);
  content = efl_gfx_entity_geometry_get(box);
  item.x -= content.x;
  item.y -= content.y;
  efl_ui_scrollable_scroll(scroller, item, EINA_FALSE);
}

static void eflTabsLayoutJob(void* data)
{
  Ihandle* ih = (Ihandle*)data;
  iupAttribSet(ih, "_IUP_EFL_LAYOUT_JOB", NULL);
  iupLayoutUpdate(ih);
  eflTabsShowSelectedTab(ih);
}

static void eflTabsScheduleLayout(Ihandle* ih)
{
  if (iupAttribGet(ih, "_IUP_EFL_LAYOUT_JOB"))
    return;

  iupAttribSet(ih, "_IUP_EFL_LAYOUT_JOB", (char*)ecore_job_add(eflTabsLayoutJob, ih));
}

IUP_SDK_API void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  Eo* pager = iupeflGetWidget(ih);

  if (pager)
  {
    Ihandle* child = eflTabsChildAtPos(ih, pos);
    Eo* page = child ? (Eo*)iupAttribGet(child, "_IUPTAB_PAGE") : NULL;
    if (page && efl_pack_index_get(pager, page) >= 0)
    {
      {
      Eo* tab_bar;
      iupAttribSet(ih, "_IUP_EFL_IGNORE_CHANGE", "1");
      tab_bar = efl_ui_tab_pager_tab_bar_get(pager);
      if (tab_bar)
      {
        Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
        if (item)
          efl_ui_selectable_selected_set(item, EINA_TRUE);
      }
      iupAttribSet(ih, "_IUP_EFL_IGNORE_CHANGE", NULL);
      }

      iupAttribSet(ih, "_IUP_EFL_PREV_CHILD", (char*)child);
      eflTabsScheduleLayout(ih);
    }
  }
}

IUP_SDK_API int iupdrvTabsGetCurrentTab(Ihandle* ih)
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
          return eflTabsPageToPos(ih, page);
      }
    }
  }

  return 0;
}

IUP_SDK_API int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  (void)pos;
  return iupAttribGetInt(child, "_IUPEFL_TAB_HIDDEN") ? 0 : 1;
}

/****************************************************************
                     Callbacks
****************************************************************/

static Ihandle* eflTabsPrevChild(Ihandle* ih)
{
  Ihandle* child = (Ihandle*)iupAttribGet(ih, "_IUP_EFL_PREV_CHILD");
  if (child && iupObjectCheck(child) && child->parent == ih)
    return child;
  return NULL;
}

static void eflTabsItemSelectedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* selected;
  Eo* pager;
  Ihandle* child;
  Ihandle* prev_child;
  IFnnn cb;

  if (iupAttribGet(ih, "_IUP_EFL_IGNORE_CHANGE"))
    return;

  pager = iupeflGetWidget(ih);
  if (!pager)
    return;

  selected = efl_ui_selectable_last_selected_get(ev->object);
  if (!selected)
    return;

  {
  Eo* page = efl_parent_get(selected);
  if (!page)
    return;

  child = IupGetChild(ih, eflTabsPageToPos(ih, page));
  prev_child = eflTabsPrevChild(ih);

  if (!child || child == prev_child)
    return;

  iupAttribSet(ih, "_IUP_EFL_PREV_CHILD", (char*)child);

  eflTabsScheduleLayout(ih);

  cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
  if (cb)
    cb(ih, child, prev_child);
  else
  {
    IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
    if (cb2 && prev_child)
      cb2(ih, IupGetChildPos(ih, child), IupGetChildPos(ih, prev_child));
  }
  }
}

static void eflTabsSetPageHidden(Ihandle* ih, Ihandle* child, int hide)
{
  Eo* pager = iupeflGetWidget(ih);
  Eo* page = (Eo*)iupAttribGet(child, "_IUPTAB_PAGE");
  int hidden = iupAttribGetInt(child, "_IUPEFL_TAB_HIDDEN");

  if (!pager || !page)
    return;

  if (hide && !hidden)
  {
    Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
    int count = efl_content_count(pager);
    int my_pos = efl_pack_index_get(pager, page);

    if (item && efl_ui_selectable_selected_get(item))
    {
      Eo* other = NULL;
      int i;
      for (i = my_pos + 1; i < count && !other; i++)
        other = efl_pack_content_get(pager, i);
      for (i = my_pos - 1; i >= 0 && !other; i--)
        other = efl_pack_content_get(pager, i);
      if (other)
      {
        Eo* other_item = efl_ui_tab_page_tab_bar_item_get(other);
        if (other_item)
        {
          iupAttribSet(ih, "_IUP_EFL_IGNORE_CHANGE", "1");
          efl_ui_selectable_selected_set(other_item, EINA_TRUE);
          iupAttribSet(ih, "_IUP_EFL_IGNORE_CHANGE", NULL);
          iupAttribSet(ih, "_IUP_EFL_PREV_CHILD", (char*)IupGetChild(ih, eflTabsPageToPos(ih, other)));
        }
      }
    }

    efl_pack_unpack(pager, page);
    if (item)
      efl_gfx_entity_visible_set(item, EINA_FALSE);
    iupAttribSetInt(child, "_IUPEFL_TAB_HIDDEN", 1);
  }
  else if (!hide && hidden)
  {
    Eo* item;
    efl_pack_at(pager, page, eflTabsInsertIndex(ih, child));
    item = efl_ui_tab_page_tab_bar_item_get(page);
    if (item)
      efl_gfx_entity_visible_set(item, EINA_TRUE);
    iupAttribSetInt(child, "_IUPEFL_TAB_HIDDEN", 0);
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

  pos = eflTabsPageToPos(ih, page);

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
    eflTabsSetPageHidden(ih, child, 1);
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
  if (icon && efl_ui_image_icon_set(icon, "window-close"))
  {
    efl_content_set(btn, icon);
    efl_gfx_entity_visible_set(icon, EINA_TRUE);
  }
  else
  {
    if (icon)
      efl_del(icon);
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
        {
          char c = 0;
          char* str = iupStrProcessMnemonic(value, &c, -1);
          efl_text_set(item, str ? str : "");
          if (c)
            iupKeySetMnemonic(ih, c, pos);
          if (str && str != value)
            free(str);
        }
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
    eflTabsSetPageHidden(ih, child, iupStrBoolean(value) ? 0 : 1);

  return 0;
}


/****************************************************************
                     Methods
****************************************************************/

static void eflTabsSetSubtreeVisible(Ihandle* ih, Eina_Bool visible)
{
  Ihandle* child;

  if (ih->iclass->nativetype != IUP_TYPEVOID && ih->handle)
  {
    Eina_Bool show = visible && !iupAttribGet(ih, "_IUPEFL_HIDDEN");
    Eo* widget = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
    Eo* bg_rect = (Eo*)iupAttribGet(ih, "_IUP_EFL_BGRECT");

    if (!widget)
      widget = iupeflGetWidget(ih);

    if (widget)
      efl_gfx_entity_visible_set(widget, show);

    if (bg_rect)
      efl_gfx_entity_visible_set(bg_rect, show);

    if (ih->iclass->nativetype == IUP_TYPECANVAS)
      iupeflCanvasSetScrollBarsVisible(ih, show);
  }

  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING))
      eflTabsSetSubtreeVisible(child, visible);
  }
}

static void eflTabsLayoutUpdateMethod(Ihandle* ih)
{
  Eo* pager = iupeflGetWidget(ih);
  Ihandle* child;
  Eina_Bool visible;
  int current_tab;
  int pos;

  iupdrvBaseLayoutUpdateMethod(ih);

  current_tab = iupdrvTabsGetCurrentTab(ih);

  /* a nested IupTabs lays out after the outer one hid it, and must not show its page again */
  visible = pager ? efl_gfx_entity_visible_get(pager) : EINA_TRUE;

  pos = 0;
  for (child = ih->firstchild; child; child = child->brother, pos++)
    eflTabsSetSubtreeVisible(child, visible && pos == current_tab ? EINA_TRUE : EINA_FALSE);
}

IUP_SDK_API void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
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
      iupTabsScaleImageSize(ih, img_w, img_h, &img_w, &img_h);

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
    Eo* pager;
    Eo* page;
    Eo* content_box;
    char* tabtitle, *tabimage;
    int pos;

    if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
      return;
    pager = iupeflGetWidget(ih);

    pos = IupGetChildPos(ih, child);

    page = efl_add(EFL_UI_TAB_PAGE_CLASS, pager);
    if (!page)
      return;

    content_box = efl_add(iupefl_fixed_class_get(), page);
    if (content_box)
    {
      efl_gfx_hint_weight_set(content_box, 1.0, 1.0);
      efl_gfx_hint_align_set(content_box, -1.0, -1.0);
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

    {
    Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
    if (item)
    {
      if (tabtitle)
      {
        char c = 0;
        char* str = iupStrProcessMnemonic(tabtitle, &c, -1);
        efl_text_set(item, str ? str : "");
        if (c)
          iupKeySetMnemonic(ih, c, pos);
        if (str && str != tabtitle)
          free(str);
      }
      if (tabimage)
        eflTabsSetItemIcon(item, tabimage, ih);

      if (ih->data->show_close)
      {
        Evas_Object* close_btn = eflTabsCreateCloseButton(ih, child, item);
        iupAttribSet(child, "_IUPTAB_CLOSE", (char*)close_btn);
      }

      eflTabsAddReorderCallbacks(ih, item);
    }
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
    Eo* pager = iupeflGetWidget(ih);
    Eo* page = (Eo*)iupAttribGet(child, "_IUPTAB_PAGE");
    Eo* close_btn = (Eo*)iupAttribGet(child, "_IUPTAB_CLOSE");

    if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
      return;

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

    iupAttribSet(ih, "_IUPEFL_REMOVED_CHILD", (char*)child);
    iupAttribSetInt(ih, "_IUPEFL_REMOVED_POS", pos);
    iupTabsCheckCurrentTab(ih, pos, 1);
    iupAttribSet(ih, "_IUPEFL_REMOVED_CHILD", NULL);

    if (page && pager)
    {
      if (!iupAttribGetInt(child, "_IUPEFL_TAB_HIDDEN"))
        efl_pack_unpack(pager, page);
      iupeflDelete(page);
    }
  }

  child->handle = NULL;

  iupAttribSet(child, "_IUPTAB_PAGE", NULL);
  iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
  iupAttribSet(child, "_IUPTAB_CLOSE", NULL);

  if (ih->handle)
    eflTabsScheduleLayout(ih);
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
    Eo* box = efl_content_get(efl_part(tab_bar, "efl.content"));
    if (box)
    {
      Eo* scroller;
      efl_content_unset(efl_part(tab_bar, "efl.content"));
      scroller = efl_add(EFL_UI_SCROLLER_CLASS, tab_bar);
      efl_ui_scrollbar_bar_mode_set(scroller, EFL_UI_SCROLLBAR_MODE_OFF, EFL_UI_SCROLLBAR_MODE_OFF);
      efl_ui_scrollable_match_content_set(scroller, EINA_FALSE, EINA_TRUE);
      efl_content_set(scroller, box);
      efl_content_set(efl_part(tab_bar, "efl.content"), scroller);
      iupAttribSet(ih, "_IUP_EFL_TAB_SCROLLER", (char*)scroller);
    }
    efl_event_callback_add(tab_bar, EFL_UI_EVENT_ITEM_SELECTED, eflTabsItemSelectedCallback, ih);
  }

  iupAttribSet(ih, "_IUP_EFL_PREV_CHILD", (char*)ih->firstchild);

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
      efl_event_callback_del(tab_bar, EFL_UI_EVENT_ITEM_SELECTED, eflTabsItemSelectedCallback, ih);
    }
  }

  iupdrvBaseUnMapMethod(ih);
}

static int eflTabsSetTabTipAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    Eo* page = (Eo*)iupAttribGet(child, "_IUPTAB_PAGE");
    if (page)
    {
      Eo* item = efl_ui_tab_page_tab_bar_item_get(page);
      if (item)
      {
        if (value && *value)
          elm_object_tooltip_text_set(item, value);
        else
          elm_object_tooltip_unset(item);
      }
    }
  }

  return 0;
}

static int eflTabsSetShowCloseAttrib(Ihandle* ih, int pos, const char* value)
{
  if (pos == IUP_INVALID_ID)
  {
    ih->data->show_close = iupStrBoolean(value);

    if (ih->handle)
    {
      int i, count = IupGetChildCount(ih);
      for (i = 0; i < count; i++)
      {
        Ihandle* child = IupGetChild(ih, i);
        if (!child) continue;

        char* child_show_close = iupAttribGet(child, "SHOWCLOSE");
        if (!child_show_close)
          eflTabsSetShowCloseAttrib(ih, i, value);
      }
    }

    return 1;
  }
  else
  {
    Ihandle* child = IupGetChild(ih, pos);
    if (child)
      iupAttribSetStr(child, "SHOWCLOSE", value);

    if (ih->handle)
    {
      Eo* page = child ? (Eo*)iupAttribGet(child, "_IUPTAB_PAGE") : NULL;
      Eo* item = page ? efl_ui_tab_page_tab_bar_item_get(page) : NULL;
      if (!item) return 0;

      Eo* close_btn = child ? (Eo*)iupAttribGet(child, "_IUPTAB_CLOSE") : NULL;

      if (iupStrBoolean(value))
      {
        if (!close_btn)
        {
          close_btn = eflTabsCreateCloseButton(ih, child, item);
          iupAttribSet(child, "_IUPTAB_CLOSE", (char*)close_btn);
        }
      }
      else
      {
        if (close_btn)
        {
          efl_event_callback_del(close_btn, EFL_INPUT_EVENT_CLICKED, eflTabsCloseButtonClicked, child);
          efl_del(close_btn);
          iupAttribSet(child, "_IUPTAB_CLOSE", NULL);
        }
      }
    }

    return 0;
  }
}

IUP_SDK_API void iupdrvTabsInitClass(Iclass* ic)
{
  ic->Map = eflTabsMapMethod;
  ic->UnMap = eflTabsUnMapMethod;
  ic->LayoutUpdate = eflTabsLayoutUpdateMethod;
  ic->ChildAdded = eflTabsChildAddedMethod;
  ic->ChildRemoved = eflTabsChildRemovedMethod;

  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");

  /* TABTYPE is read-only. EFL Tab_Pager only supports TOP position (theme-controlled) */
  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, NULL, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, eflTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, eflTabsSetTabImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, eflTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABPADDING", iupTabsGetTabPaddingAttrib, eflTabsSetTabPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "TABTIP", NULL, eflTabsSetTabTipAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SHOWCLOSE", NULL, eflTabsSetShowCloseAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, eflTabsSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MULTILINE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
