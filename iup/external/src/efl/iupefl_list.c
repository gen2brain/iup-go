/** \file
 * \brief List Control
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
#include "iup_mask.h"
#include "iup_list.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_classbase.h"

#include "iupefl_drv.h"


/****************************************************************
                     Virtual Mode
****************************************************************/

static Elm_Genlist_Item_Class* efl_virtual_item_class = NULL;

static char* eflGenlistTextGet(void* data, Evas_Object* obj, const char* part)
{
  Ihandle* ih = (Ihandle*)evas_object_data_get(obj, "_IUP_IHANDLE");
  int pos = (int)(uintptr_t)data;
  char* text;

  (void)part;

  if (!ih)
    return NULL;

  text = iupListGetItemValueCb(ih, pos);
  if (text)
    return strdup(text);

  return NULL;
}

static Evas_Object* eflGenlistContentGet(void* data, Evas_Object* obj, const char* part)
{
  Ihandle* ih = (Ihandle*)evas_object_data_get(obj, "_IUP_IHANDLE");
  int pos = (int)(uintptr_t)data;
  char* image_name;

  if (!ih || !ih->data->show_image)
    return NULL;

  if (strcmp(part, "elm.swallow.icon") != 0)
    return NULL;

  image_name = iupListGetItemImageCb(ih, pos);
  if (!image_name)
    return NULL;

  return iupeflImageGetImage(image_name, ih, 0);
}

static void eflGenlistItemSelectedCb(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  Elm_Object_Item* item = (Elm_Object_Item*)event_info;
  int pos;
  char* text;
  IFnsii cb;

  (void)obj;

  if (!item)
    return;

  pos = (int)(uintptr_t)elm_object_item_data_get(item);
  text = iupListGetItemValueCb(ih, pos);

  cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih, text ? text : "", pos, 1);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  iupBaseCallValueChangedCb(ih);
}

/****************************************************************
                     Dropdown Callbacks
****************************************************************/

static void eflDropdownCloseList(Ihandle* ih)
{
  Eo* dropdown_list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  IFni cb;

  if (dropdown_list && efl_gfx_entity_visible_get(dropdown_list))
  {
    efl_gfx_entity_visible_set(dropdown_list, EINA_FALSE);

    cb = (IFni)IupGetCallback(ih, "DROPDOWN_CB");
    if (cb)
      cb(ih, 0);
  }
}

static void eflDropdownSetText(Ihandle* ih, const char* text)
{
  Eo* text_label = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LABEL");
  if (text_label)
    efl_text_set(text_label, text ? text : "");
}

static void eflDropdownButtonClickedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* button = (Eo*)ih->handle;
  Eo* dropdown_list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  Eina_Rect button_geom;
  int list_count, visible_items, item_height, list_height;
  IFni cb;
  (void)ev;

  if (!dropdown_list || !button)
    return;

  if (efl_gfx_entity_visible_get(dropdown_list))
  {
    eflDropdownCloseList(ih);
    return;
  }

  button_geom = efl_gfx_entity_geometry_get(button);

  visible_items = iupAttribGetInt(ih, "VISIBLEITEMS");
  if (visible_items <= 0)
    visible_items = 5;

  list_count = efl_content_count(dropdown_list);

  item_height = 0;
  if (list_count > 0)
  {
    Eo* first_item = efl_pack_content_get(dropdown_list, 0);
    if (first_item)
    {
      Eina_Size2D item_size = efl_gfx_hint_size_combined_min_get(first_item);
      item_height = item_size.h;
    }
  }

  if (item_height <= 0)
  {
    iupdrvFontGetCharSize(ih, NULL, &item_height);
    item_height += 8;
  }

  if (list_count > visible_items)
    list_count = visible_items;
  if (list_count <= 0)
    list_count = 1;

  list_height = list_count * item_height;

  efl_gfx_entity_position_set(dropdown_list, EINA_POSITION2D(button_geom.x, button_geom.y + button_geom.h));
  efl_gfx_entity_size_set(dropdown_list, EINA_SIZE2D(button_geom.w, list_height));

  efl_gfx_stack_raise_to_top(dropdown_list);
  efl_gfx_entity_visible_set(dropdown_list, EINA_TRUE);

  cb = (IFni)IupGetCallback(ih, "DROPDOWN_CB");
  if (cb)
    cb(ih, 1);
}

static void eflDropdownWindowPointerDownCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* dropdown_list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  Eo* button = (Eo*)ih->handle;
  Efl_Input_Pointer* pointer = ev->info;
  Eina_Position2D pos;
  Eina_Rect button_rect, list_rect;

  if (!dropdown_list || !efl_gfx_entity_visible_get(dropdown_list))
    return;

  pos = efl_input_pointer_position_get(pointer);
  button_rect = efl_gfx_entity_geometry_get(button);
  list_rect = efl_gfx_entity_geometry_get(dropdown_list);

  if (!eina_rectangle_coords_inside(&button_rect.rect, pos.x, pos.y) &&
      !eina_rectangle_coords_inside(&list_rect.rect, pos.x, pos.y))
  {
    eflDropdownCloseList(ih);
  }
}

static void eflDropdownListSelectionChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* list = ev->object;
  Eo* item;
  IFnsii cb;
  int pos;
  const char* text;

  item = efl_ui_selectable_last_selected_get(list);
  if (!item)
    return;

  pos = efl_ui_item_index_get(item) + 1;
  text = efl_text_get(item);

  iupAttribSetInt(ih, "_IUPEFL_LIST_VALUE", pos);

  eflDropdownSetText(ih, text);

  eflDropdownCloseList(ih);

  cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih, text ? (char*)text : "", pos, 1);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }

  iupBaseCallValueChangedCb(ih);
}

/****************************************************************
                     Callbacks
****************************************************************/

static void eflListSelectionChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Eo* list = ev->object;

  if (ih->data->is_multiple)
  {
    IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");
    IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");

    if (multi_cb || cb)
    {
      int count = efl_content_count(list);
      int* pos = malloc(sizeof(int) * count);
      int sel_count = 0;
      int i;

      for (i = 0; i < count; i++)
      {
        Eo* item = efl_pack_content_get(list, i);
        if (item && efl_ui_selectable_selected_get(item))
        {
          pos[sel_count] = i;
          sel_count++;
        }
      }

      iupListMultipleCallActionCb(ih, cb, multi_cb, pos, sel_count);
      free(pos);
    }
  }
  else
  {
    Eo* item = efl_ui_selectable_last_selected_get(list);
    if (item)
    {
      int pos = efl_ui_item_index_get(item) + 1;
      const char* text = efl_text_get(item);

      iupAttribSetInt(ih, "_IUPEFL_LIST_VALUE", pos);

      if (ih->data->has_editbox && text)
      {
        Eo* entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
        if (entry)
          iupeflSetText(entry, text);
      }

      IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
      if (cb)
        iupListSingleCallActionCb(ih, cb, pos);
    }
  }

  if (!ih->data->has_editbox)
    iupBaseCallValueChangedCb(ih);
}

/****************************************************************
                     Attributes
****************************************************************/

static int eflListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  Eo* dropdown_list;

  if (!ih->data->is_dropdown)
    return 0;

  dropdown_list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  if (!dropdown_list)
    return 0;

  if (iupStrBoolean(value))
  {
    Eo* button = (Eo*)ih->handle;
    Eina_Rect button_geom = efl_gfx_entity_geometry_get(button);
    int visible_items = iupAttribGetInt(ih, "VISIBLEITEMS");
    int item_height, list_count, list_height;

    if (visible_items <= 0)
      visible_items = 5;

    iupdrvFontGetCharSize(ih, NULL, &item_height);
    item_height += 4;

    list_count = efl_content_count(dropdown_list);
    if (list_count > visible_items)
      list_count = visible_items;
    if (list_count <= 0)
      list_count = 1;

    list_height = list_count * item_height;

    efl_gfx_entity_position_set(dropdown_list, EINA_POSITION2D(button_geom.x, button_geom.y + button_geom.h));
    efl_gfx_entity_size_set(dropdown_list, EINA_SIZE2D(button_geom.w, list_height));
    efl_gfx_stack_raise_to_top(dropdown_list);
  }

  efl_gfx_entity_visible_set(dropdown_list, iupStrBoolean(value));

  return 0;
}

static char* eflListGetShowDropdownAttrib(Ihandle* ih)
{
  Eo* dropdown_list;

  if (!ih->data->is_dropdown)
    return NULL;

  dropdown_list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  if (!dropdown_list)
    return NULL;

  return iupStrReturnBoolean(efl_gfx_entity_visible_get(dropdown_list));
}

static int eflListSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;
  if (!ih->data->has_editbox)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  efl_text_interactive_editable_set(entry, !iupStrBoolean(value));
  return 0;
}

static char* eflListGetReadOnlyAttrib(Ihandle* ih)
{
  Eo* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return NULL;

  return iupStrReturnBoolean(!efl_text_interactive_editable_get(entry));
}

static int eflListSetCaretAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;
  Efl_Text_Cursor_Object* cursor;
  int pos = 1;

  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  sscanf(value, "%d", &pos);
  if (pos < 1) pos = 1;
  pos--;

  cursor = efl_text_interactive_main_cursor_get(entry);
  if (cursor)
    efl_text_cursor_object_position_set(cursor, pos);

  return 0;
}

static char* eflListGetCaretAttrib(Ihandle* ih)
{
  Eo* entry;
  Efl_Text_Cursor_Object* cursor;
  int pos;

  if (!ih->data->has_editbox)
    return NULL;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return NULL;

  cursor = efl_text_interactive_main_cursor_get(entry);
  if (!cursor)
    return NULL;

  pos = efl_text_cursor_object_position_get(cursor) + 1;
  return iupStrReturnInt(pos);
}

static int eflListSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;
  Efl_Text_Cursor_Object* cursor;
  int pos = 0;

  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  sscanf(value, "%d", &pos);
  if (pos < 0) pos = 0;

  cursor = efl_text_interactive_main_cursor_get(entry);
  if (cursor)
    efl_text_cursor_object_position_set(cursor, pos);

  return 0;
}

static char* eflListGetCaretPosAttrib(Ihandle* ih)
{
  Eo* entry;
  Efl_Text_Cursor_Object* cursor;

  if (!ih->data->has_editbox)
    return NULL;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return NULL;

  cursor = efl_text_interactive_main_cursor_get(entry);
  if (!cursor)
    return NULL;

  return iupStrReturnInt(efl_text_cursor_object_position_get(cursor));
}

static int eflListSetSelectionAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;
  Efl_Text_Cursor_Object *cursor_start, *cursor_end;
  int start = 1, end = 1;

  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;

  start--;
  end--;

  efl_text_interactive_selection_cursors_get(entry, &cursor_start, &cursor_end);
  if (cursor_start && cursor_end)
  {
    efl_text_cursor_object_position_set(cursor_start, start);
    efl_text_cursor_object_position_set(cursor_end, end);
  }

  return 0;
}

static char* eflListGetSelectionAttrib(Ihandle* ih)
{
  Eo* entry;
  Efl_Text_Cursor_Object *cursor_start, *cursor_end;
  int start_pos, end_pos;

  if (!ih->data->has_editbox)
    return NULL;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return NULL;

  efl_text_interactive_selection_cursors_get(entry, &cursor_start, &cursor_end);
  if (!cursor_start || !cursor_end)
    return NULL;

  start_pos = efl_text_cursor_object_position_get(cursor_start) + 1;
  end_pos = efl_text_cursor_object_position_get(cursor_end) + 1;

  if (start_pos == end_pos)
    return NULL;

  return iupStrReturnIntInt(start_pos, end_pos, ':');
}

static int eflListSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;
  Efl_Text_Cursor_Object *cursor_start, *cursor_end;
  int start = 0, end = 0;

  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;

  efl_text_interactive_selection_cursors_get(entry, &cursor_start, &cursor_end);
  if (cursor_start && cursor_end)
  {
    efl_text_cursor_object_position_set(cursor_start, start);
    efl_text_cursor_object_position_set(cursor_end, end);
  }

  return 0;
}

static char* eflListGetSelectionPosAttrib(Ihandle* ih)
{
  Eo* entry;
  Efl_Text_Cursor_Object *cursor_start, *cursor_end;
  int start_pos, end_pos;

  if (!ih->data->has_editbox)
    return NULL;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return NULL;

  efl_text_interactive_selection_cursors_get(entry, &cursor_start, &cursor_end);
  if (!cursor_start || !cursor_end)
    return NULL;

  start_pos = efl_text_cursor_object_position_get(cursor_start);
  end_pos = efl_text_cursor_object_position_get(cursor_end);

  if (start_pos == end_pos)
    return NULL;

  return iupStrReturnIntInt(start_pos, end_pos, ':');
}

static int eflListSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;
  Efl_Text_Cursor_Object *cursor_start, *cursor_end;
  int start_pos, end_pos;

  if (!ih->data->has_editbox)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  efl_text_interactive_selection_cursors_get(entry, &cursor_start, &cursor_end);
  if (!cursor_start || !cursor_end)
    return 0;

  start_pos = efl_text_cursor_object_position_get(cursor_start);
  end_pos = efl_text_cursor_object_position_get(cursor_end);

  if (start_pos != end_pos)
  {
    efl_text_cursor_object_range_delete(cursor_start, cursor_end);
  }

  if (value && value[0])
  {
    Efl_Text_Cursor_Object* main_cursor = efl_text_interactive_main_cursor_get(entry);
    if (main_cursor)
      efl_text_cursor_object_text_insert(main_cursor, value);
  }

  return 0;
}

static char* eflListGetSelectedTextAttrib(Ihandle* ih)
{
  Eo* entry;
  Efl_Text_Cursor_Object *cursor_start, *cursor_end;
  int start_pos, end_pos;
  const char* full_text;
  int len;
  char* selected;
  char* ret;

  if (!ih->data->has_editbox)
    return NULL;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return NULL;

  efl_text_interactive_selection_cursors_get(entry, &cursor_start, &cursor_end);
  if (!cursor_start || !cursor_end)
    return NULL;

  start_pos = efl_text_cursor_object_position_get(cursor_start);
  end_pos = efl_text_cursor_object_position_get(cursor_end);

  if (start_pos == end_pos)
    return NULL;

  full_text = efl_text_get(entry);
  if (!full_text)
    return NULL;

  len = end_pos - start_pos;
  selected = malloc(len + 1);
  strncpy(selected, full_text + start_pos, len);
  selected[len] = '\0';

  ret = iupStrReturnStr(selected);
  free(selected);
  return ret;
}

static int eflListSetInsertAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;
  Efl_Text_Cursor_Object* cursor;

  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  cursor = efl_text_interactive_main_cursor_get(entry);
  if (cursor)
    efl_text_cursor_object_text_insert(cursor, value);

  return 0;
}

static int eflListSetAppendAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;
  const char* current;
  char* newtext;

  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  current = efl_text_get(entry);
  if (current && current[0])
  {
    newtext = malloc(strlen(current) + strlen(value) + 2);
    sprintf(newtext, "%s\n%s", current, value);
    efl_text_set(entry, newtext);
    free(newtext);
  }
  else
    efl_text_set(entry, value);

  return 0;
}

static int eflListSetClipboardAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;

  if (!ih->data->has_editbox)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  if (iupStrEqualNoCase(value, "COPY"))
    efl_ui_textbox_selection_copy(entry);
  else if (iupStrEqualNoCase(value, "CUT"))
    efl_ui_textbox_selection_cut(entry);
  else if (iupStrEqualNoCase(value, "PASTE"))
    efl_ui_textbox_selection_paste(entry);
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    Efl_Text_Cursor_Object *cursor_start, *cursor_end;
    efl_text_interactive_selection_cursors_get(entry, &cursor_start, &cursor_end);
    if (cursor_start && cursor_end)
      efl_text_cursor_object_range_delete(cursor_start, cursor_end);
  }

  return 0;
}

static int eflListSetScrollToAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;
  Efl_Text_Cursor_Object* cursor;
  int pos = 1;

  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  sscanf(value, "%d", &pos);
  if (pos < 1) pos = 1;
  pos--;

  cursor = efl_text_interactive_main_cursor_get(entry);
  if (cursor)
  {
    efl_text_cursor_object_position_set(cursor, pos);
  }

  return 0;
}

static int eflListSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;
  Efl_Text_Cursor_Object* cursor;
  int pos = 0;

  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  sscanf(value, "%d", &pos);
  if (pos < 0) pos = 0;

  cursor = efl_text_interactive_main_cursor_get(entry);
  if (cursor)
  {
    efl_text_cursor_object_position_set(cursor, pos);
  }

  return 0;
}

static int eflListSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_editbox)
    return 0;

  if (!ih->handle)
    return 1;

  return 1;
}

static int eflListSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  return 1;
}

static int eflListSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->data->has_editbox)
  {
    Eo* entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
    if (entry)
      efl_text_color_set(entry, r, g, b, 255);
  }
  else if (ih->data->is_dropdown)
  {
    Eo* text_label = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LABEL");
    Eo* arrow_label = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_ARROW");
    if (text_label)
      efl_text_color_set(text_label, r, g, b, 255);
    if (arrow_label)
      efl_text_color_set(arrow_label, r, g, b, 255);
  }

  return 1;
}

static int eflListSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  Eo* entry;

  if (!ih->data->has_editbox)
    return 0;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return 0;

  efl_text_set(efl_part(entry, "efl.text_guide"), value ? value : "");
  return 1;
}

static char* eflListGetCueBannerAttrib(Ihandle* ih)
{
  Eo* entry;
  const char* text;

  if (!ih->data->has_editbox)
    return NULL;

  entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
  if (!entry)
    return NULL;

  text = efl_text_get(efl_part(entry, "efl.text_guide"));
  return iupStrReturnStr(text);
}

static int eflListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  Eo* list;
  Eo* item;
  Evas_Object* img;
  int pos;

  if (ih->data->is_dropdown && !ih->data->has_editbox)
    list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  else
    list = iupeflGetWidget(ih);

  if (!list)
    return 0;

  pos = iupListGetPosAttrib(ih, id);
  if (pos < 0)
    return 0;

  item = efl_pack_content_get(list, pos);
  if (!item)
    return 0;

  {
    Eo* old_icon = efl_content_get(efl_part(item, "icon"));
    if (old_icon)
    {
      const char* old_name = efl_key_data_get(old_icon, "_IUP_IMAGE_NAME");
      if (old_name)
        eina_stringshare_del(old_name);
    }
  }

  if (!value || !value[0])
  {
    efl_content_unset(efl_part(item, "icon"));
    return 0;
  }

  img = iupeflImageGetImage(value, ih, 0);
  if (img)
  {
    efl_key_data_set(img, "_IUP_IMAGE_NAME", eina_stringshare_add(value));

    if (ih->data->fit_image)
    {
      int img_w, img_h, charheight;
      int available_height;

      iupImageGetInfo(value, &img_w, &img_h, NULL);
      iupdrvFontGetCharSize(ih, NULL, &charheight);
      available_height = charheight + 2 * ih->data->spacing;

      if (img_h > available_height)
      {
        int scaled_w = (img_w * available_height) / img_h;
        int scaled_h = available_height;

        elm_image_no_scale_set(img, EINA_FALSE);
        elm_image_resizable_set(img, EINA_TRUE, EINA_TRUE);
        elm_image_aspect_fixed_set(img, EINA_TRUE);
        evas_object_size_hint_min_set(img, scaled_w, scaled_h);
        evas_object_size_hint_max_set(img, scaled_w, scaled_h);
        evas_object_resize(img, scaled_w, scaled_h);
      }
    }

    efl_content_set(efl_part(item, "icon"), img);
  }

  return 0;
}

static int eflListSetValueAttrib(Ihandle* ih, const char* value)
{
  Eo* list;
  int is_dropdown = ih->data->is_dropdown;
  int pos;

  if (!value || !value[0])
    return 0;

  if (ih->data->has_editbox)
  {
    Eo* entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
    if (entry)
      iupeflSetText(entry, value);
    return 0;
  }

  if (ih->data->is_virtual)
  {
    Evas_Object* vlist = iupeflGetWidget(ih);
    if (!vlist)
      return 0;

    pos = atoi(value);
    if (pos > 0)
    {
      Elm_Object_Item* item = elm_genlist_nth_item_get(vlist, pos - 1);
      if (item)
        elm_genlist_item_selected_set(item, EINA_TRUE);
    }
    return 0;
  }

  if (is_dropdown)
    list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  else
    list = iupeflGetWidget(ih);

  if (!list)
    return 0;

  pos = atoi(value);
  if (pos > 0)
  {
    Eo* item = efl_pack_content_get(list, pos - 1);
    if (item)
    {
      efl_ui_selectable_selected_set(item, EINA_TRUE);
      iupAttribSetInt(ih, "_IUPEFL_LIST_VALUE", pos);

      if (is_dropdown)
        eflDropdownSetText(ih, efl_text_get(item));
    }
  }

  return 0;
}

static char* eflListGetValueAttrib(Ihandle* ih)
{
  Eo* list = iupeflGetWidget(ih);
  int is_dropdown = ih->data->is_dropdown;

  if (!list)
    return NULL;

  if (ih->data->has_editbox)
  {
    Eo* entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
    if (entry)
    {
      const char* text = iupeflGetText(entry);
      return iupStrReturnStr(text);
    }
    return NULL;
  }

  if (ih->data->is_virtual)
  {
    if (ih->data->is_multiple)
    {
      int count = iupdrvListGetCount(ih);
      char* str = iupStrGetMemory(count + 1);
      int i;

      memset(str, '-', count);
      str[count] = 0;

      for (i = 0; i < count; i++)
      {
        Elm_Object_Item* item = elm_genlist_nth_item_get(list, i);
        if (item && elm_genlist_item_selected_get(item))
          str[i] = '+';
      }
      return str;
    }
    else
    {
      Elm_Object_Item* item = elm_genlist_selected_item_get(list);
      if (item)
      {
        int pos = elm_genlist_item_index_get(item);
        return iupStrReturnInt(pos);
      }
    }
    return "0";
  }

  if (is_dropdown)
  {
    int pos = iupAttribGetInt(ih, "_IUPEFL_LIST_VALUE");
    return iupStrReturnInt(pos);
  }
  else if (ih->data->is_multiple)
  {
    int count = efl_content_count(list);
    char* str = iupStrGetMemory(count + 1);
    int i;

    memset(str, '-', count);
    str[count] = 0;

    for (i = 0; i < count; i++)
    {
      Eo* item = efl_pack_content_get(list, i);
      if (item && efl_ui_selectable_selected_get(item))
        str[i] = '+';
    }
    return str;
  }
  else
  {
    Eo* item = efl_ui_selectable_last_selected_get(list);
    if (item)
    {
      int pos = efl_ui_item_index_get(item) + 1;
      iupAttribSetInt(ih, "_IUPEFL_LIST_VALUE", pos);
      return iupStrReturnInt(pos);
    }
    else
    {
      int pos = iupAttribGetInt(ih, "_IUPEFL_LIST_VALUE");
      if (pos > 0)
        return iupStrReturnInt(pos);
    }
  }

  return "0";
}

static int eflListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  Evas_Object* list = iupeflGetWidget(ih);
  int pos;

  if (!list || !value || ih->data->is_dropdown)
    return 0;

  pos = atoi(value);
  if (pos > 0)
  {
    if (ih->data->is_virtual)
    {
      Elm_Object_Item* item = elm_genlist_nth_item_get(list, pos - 1);
      if (item)
        elm_genlist_item_show(item, ELM_GENLIST_ITEM_SCROLLTO_TOP);
    }
    else
    {
      Eo* item = efl_pack_content_get(list, pos - 1);
      if (item)
        efl_ui_collection_item_scroll(list, item, EINA_FALSE);
    }
  }

  return 0;
}

static char* eflListGetCountAttrib(Ihandle* ih)
{
  Eo* list;

  if (ih->data->is_virtual)
    return iupStrReturnInt(iupdrvListGetCount(ih));

  if (ih->data->is_dropdown && !ih->data->has_editbox)
    list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  else
    list = iupeflGetWidget(ih);

  if (!list)
    return "0";

  return iupStrReturnInt(efl_content_count(list));
}

/****************************************************************
                     Drag and Drop Support
****************************************************************/

static int eflListConvertXYToPos(Ihandle* ih, int x, int y);

static int efl_list_drag_start_x = 0;
static int efl_list_drag_start_y = 0;
static Ihandle* efl_list_drag_source = NULL;
static int efl_list_drag_source_pos = 0;
static Eina_Bool efl_list_drag_active = EINA_FALSE;

static void eflListDragFinishedCb(void *data, const Efl_Event *ev)
{
  (void)data;
  (void)ev;
  efl_list_drag_active = EINA_FALSE;
  efl_list_drag_source = NULL;
  efl_list_drag_source_pos = 0;
}

static void eflListDragPointerDownCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  Eina_Position2D pos;
  int button;
  int idDrag;
  char* item_text;

  if (efl_list_drag_active)
    return;

  button = efl_input_pointer_button_get(pointer);
  if (button != 1)
    return;

  pos = efl_input_pointer_position_get(pointer);
  efl_list_drag_start_x = pos.x;
  efl_list_drag_start_y = pos.y;

  idDrag = eflListConvertXYToPos(ih, pos.x, pos.y);
  if (idDrag > 0)
  {
    item_text = IupGetAttributeId(ih, "", idDrag);
    iupAttribSetStr(ih, "DRAGTEXT", item_text);

    efl_list_drag_source = ih;
    efl_list_drag_source_pos = idDrag;
  }

  iupAttribSet(ih, "_IUPEFL_LIST_DRAG_PENDING", "1");
}

static void eflListDragPointerMoveCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Input_Pointer* pointer = ev->info;
  Eina_Position2D pos;
  int dx, dy;
  Eo* list;
  int idDrag;

  if (efl_list_drag_active)
    return;

  if (!iupAttribGet(ih, "_IUPEFL_LIST_DRAG_PENDING"))
    return;

  pos = efl_input_pointer_position_get(pointer);
  dx = pos.x - efl_list_drag_start_x;
  dy = pos.y - efl_list_drag_start_y;

  if (dx*dx + dy*dy > 25)
  {
    iupAttribSet(ih, "_IUPEFL_LIST_DRAG_PENDING", NULL);

    list = iupeflGetWidget(ih);
    idDrag = eflListConvertXYToPos(ih, efl_list_drag_start_x, efl_list_drag_start_y);

    if (idDrag > 0 && list)
    {
      Eina_Content* content;
      Eina_Slice slice;
      char* item_text;

      if (iupAttribGetBoolean(ih, "DRAGDROPLIST"))
        return;

      item_text = IupGetAttributeId(ih, "", idDrag);
      efl_list_drag_source = ih;

      slice.mem = &efl_list_drag_source;
      slice.len = sizeof(Ihandle*);
      content = eina_content_new(slice, "application/x-iup-list-item");

      if (content)
      {
        Eo* win = iupeflGetMainWindow();
        if (!win)
          win = efl_provider_find(list, EFL_UI_WIN_CLASS);
        if (!win)
        {
          eina_content_free(content);
          return;
        }

        unsigned int seat_id = iupeflGetDefaultSeat(list);
        Eo* drag_win = efl_ui_dnd_drag_start(win, content, "copy", seat_id);
        if (drag_win)
        {
          Eo* drag_label;
          int w = 100, h = 30;

          drag_label = efl_add(EFL_UI_TEXTBOX_CLASS, drag_win,
                               efl_text_interactive_editable_set(efl_added, EINA_FALSE));
          if (drag_label)
          {
            efl_text_set(drag_label, item_text ? item_text : "");
            efl_gfx_entity_size_set(drag_label, EINA_SIZE2D(w, h));
            efl_gfx_entity_visible_set(drag_label, EINA_TRUE);
          }

          efl_ui_dnd_drag_offset_set(win, seat_id, EINA_SIZE2D(-w/2, -h/2));

          if (drag_label)
            efl_content_set(drag_win, drag_label);
          efl_gfx_entity_size_set(drag_win, EINA_SIZE2D(w, h));

          efl_event_callback_add(win, EFL_UI_DND_EVENT_DRAG_FINISHED, eflListDragFinishedCb, ih);
          efl_list_drag_active = EINA_TRUE;
        }
      }
    }
  }
}

static void eflListDragPointerUpCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  (void)ev;
  iupAttribSet(ih, "_IUPEFL_LIST_DRAG_PENDING", NULL);

  if (!efl_list_drag_active)
  {
    efl_list_drag_source = NULL;
    efl_list_drag_source_pos = 0;
  }
}

static void eflListDropCb(void *data, const Efl_Event *ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Ui_Drop_Dropped_Event* drop_ev = ev->info;
  int idDrop;
  Ihandle* ih_source;
  int idDrag;
  int is_shift = 0;
  int is_ctrl = 0;
  IFniiii cb;

  if (!ih || !efl_list_drag_source)
    return;

  idDrop = eflListConvertXYToPos(ih, drop_ev->dnd.position.x, drop_ev->dnd.position.y);
  if (idDrop <= 0)
    idDrop = iupdrvListGetCount(ih) + 1;

  ih_source = efl_list_drag_source;
  idDrag = efl_list_drag_source_pos;

  cb = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");
  if (cb && cb(ih_source, idDrag, idDrop, is_shift, is_ctrl) == IUP_CONTINUE)
  {
    char* sourceText = IupGetAttributeId(ih_source, "", idDrag);

    if (idDrag < idDrop && ih_source == ih)
      idDrop--;

    if (sourceText)
      IupSetAttributeId(ih, "INSERTITEM", idDrop, sourceText);

    IupSetAttributeId(ih_source, "REMOVEITEM", idDrag, NULL);
  }
}

static void eflListEnableDragDrop(Ihandle* ih)
{
  Eo* list = iupeflGetWidget(ih);
  int is_dragdroplist = iupAttribGetBoolean(ih, "DRAGDROPLIST");

  if (!list)
    return;

  efl_event_callback_add(list, EFL_EVENT_POINTER_DOWN, eflListDragPointerDownCb, ih);
  efl_event_callback_add(list, EFL_EVENT_POINTER_MOVE, eflListDragPointerMoveCb, ih);
  efl_event_callback_add(list, EFL_EVENT_POINTER_UP, eflListDragPointerUpCb, ih);

  if (!is_dragdroplist)
  {
    efl_event_callback_add(list, EFL_UI_DND_EVENT_DROP_DROPPED, eflListDropCb, ih);
    efl_event_callback_add(list, EFL_UI_DND_EVENT_DRAG_FINISHED, eflListDragFinishedCb, ih);
  }
}

/****************************************************************
                     XY to Position Conversion
****************************************************************/

static int eflListConvertXYToPos(Ihandle* ih, int x, int y)
{
  Eo* list;
  int count, i;

  (void)x;

  if (ih->data->is_dropdown)
    return -1;

  if (ih->data->is_virtual)
  {
    Evas_Object* vlist = iupeflGetWidget(ih);
    Elm_Object_Item* item = elm_genlist_at_xy_item_get(vlist, x, y, NULL);
    if (item)
      return elm_genlist_item_index_get(item);
    return -1;
  }

  list = iupeflGetWidget(ih);
  if (!list)
    return -1;

  count = efl_content_count(list);
  for (i = 0; i < count; i++)
  {
    Eo* item = efl_pack_content_get(list, i);
    if (item)
    {
      Eina_Rect geom = efl_gfx_entity_geometry_get(item);
      if (y >= geom.y && y < geom.y + geom.h)
        return i + 1;
    }
  }

  return -1;
}

/****************************************************************
                     Methods
****************************************************************/

static int eflListMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* list;
  int is_dropdown;

  parent = iupeflGetParentWidget(ih);
  is_dropdown = ih->data->is_dropdown;

  if (!parent)
    return IUP_ERROR;

  if (ih->data->has_editbox)
  {
    Eo* box;
    Eo* entry;

    box = efl_add(EFL_UI_BOX_CLASS, parent,
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
    if (!box)
      return IUP_ERROR;

    entry = efl_add(EFL_UI_TEXTBOX_CLASS, box);
    efl_text_multiline_set(entry, EINA_FALSE);
    efl_gfx_hint_weight_set(entry, 1.0, 0.0);
    efl_gfx_hint_fill_set(entry, EINA_TRUE, EINA_FALSE);
    efl_pack_end(box, entry);

    {
      char* fgcolor = iupAttribGetStr(ih, "FGCOLOR");
      unsigned char r = 0, g = 0, b = 0;
      if (!fgcolor)
        fgcolor = IupGetGlobal("DLGFGCOLOR");
      if (fgcolor)
        iupStrToRGB(fgcolor, &r, &g, &b);
      efl_text_color_set(entry, r, g, b, 255);
    }

    list = efl_add(EFL_UI_LIST_CLASS, box);
    if (!list)
    {
      iupeflDelete(box);
      return IUP_ERROR;
    }

    efl_gfx_hint_weight_set(list, 1.0, 1.0);
    efl_gfx_hint_fill_set(list, EINA_TRUE, EINA_TRUE);
    efl_pack_end(box, list);

    efl_event_callback_add(list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, eflListSelectionChangedCallback, ih);
    efl_event_callback_add(list, EFL_EVENT_POINTER_DOWN, iupeflPointerDownEvent, ih);
    efl_event_callback_add(list, EFL_EVENT_POINTER_UP, iupeflPointerUpEvent, ih);
    efl_event_callback_add(list, EFL_UI_FOCUS_MANAGER_EVENT_MANAGER_FOCUS_CHANGED, iupeflManagerFocusChangedEvent, ih);
    efl_event_callback_add(list, EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
    efl_event_callback_add(list, EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);

    iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)box);
    iupAttribSet(ih, "_IUPEFL_ENTRY", (char*)entry);

    ih->handle = (InativeHandle*)list;

    iupeflBaseAddCallbacks(ih, entry);
  }
  else if (is_dropdown)
  {
    Eo* button;
    Eo* dropdown_list;
    Eo* hbox;
    Eo* text_label;
    Eo* arrow_label;
    Eo* win = iupeflGetMainWindow();

    if (!win)
      return IUP_ERROR;

    button = efl_add(EFL_UI_BUTTON_CLASS, parent);
    if (!button)
      return IUP_ERROR;

    hbox = efl_add(EFL_UI_BOX_CLASS, button,
                   efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL));
    if (!hbox)
    {
      efl_del(button);
      return IUP_ERROR;
    }

    text_label = efl_add(EFL_UI_TEXTBOX_CLASS, hbox);
    if (text_label)
    {
      efl_text_interactive_editable_set(text_label, EINA_FALSE);
      efl_text_multiline_set(text_label, EINA_FALSE);
      efl_gfx_hint_weight_set(text_label, 1.0, 1.0);
      efl_gfx_hint_fill_set(text_label, EINA_TRUE, EINA_TRUE);
      efl_gfx_hint_align_set(text_label, 0.0, 0.5);
      efl_pack_end(hbox, text_label);

      iupAttribSet(ih, "_IUPEFL_DROPDOWN_LABEL", (char*)text_label);
    }

    arrow_label = efl_add(EFL_UI_TEXTBOX_CLASS, hbox);
    if (arrow_label)
    {
      efl_text_interactive_editable_set(arrow_label, EINA_FALSE);
      efl_text_multiline_set(arrow_label, EINA_FALSE);
      efl_text_set(arrow_label, "\u25BC");
      efl_gfx_hint_weight_set(arrow_label, 0.0, 1.0);
      efl_gfx_hint_fill_set(arrow_label, EINA_FALSE, EINA_TRUE);
      efl_gfx_hint_align_set(arrow_label, 1.0, 0.5);
      efl_pack_end(hbox, arrow_label);

      iupAttribSet(ih, "_IUPEFL_DROPDOWN_ARROW", (char*)arrow_label);
    }

    {
      char* fgcolor = iupAttribGetStr(ih, "FGCOLOR");
      unsigned char r = 0, g = 0, b = 0;
      if (!fgcolor)
        fgcolor = IupGetGlobal("DLGFGCOLOR");
      if (fgcolor)
        iupStrToRGB(fgcolor, &r, &g, &b);
      if (text_label)
        efl_text_color_set(text_label, r, g, b, 255);
      if (arrow_label)
        efl_text_color_set(arrow_label, r, g, b, 255);
    }

    efl_content_set(button, hbox);

    dropdown_list = efl_add(EFL_UI_LIST_CLASS, win);
    if (!dropdown_list)
    {
      efl_del(button);
      return IUP_ERROR;
    }

    efl_gfx_entity_visible_set(dropdown_list, EINA_FALSE);

    efl_event_callback_add(button, EFL_INPUT_EVENT_CLICKED, eflDropdownButtonClickedCallback, ih);
    efl_event_callback_add(dropdown_list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, eflDropdownListSelectionChangedCallback, ih);
    efl_event_callback_add(win, EFL_EVENT_POINTER_DOWN, eflDropdownWindowPointerDownCallback, ih);

    iupAttribSet(ih, "_IUPEFL_DROPDOWN_LIST", (char*)dropdown_list);

    ih->handle = (InativeHandle*)button;
    list = dropdown_list;
  }
  else if (ih->data->is_virtual)
  {
    if (!efl_virtual_item_class)
    {
      efl_virtual_item_class = elm_genlist_item_class_new();
      efl_virtual_item_class->item_style = "default";
      efl_virtual_item_class->func.text_get = eflGenlistTextGet;
      efl_virtual_item_class->func.content_get = eflGenlistContentGet;
      efl_virtual_item_class->func.state_get = NULL;
      efl_virtual_item_class->func.del = NULL;
    }

    list = elm_genlist_add(parent);
    if (!list)
      return IUP_ERROR;

    elm_genlist_homogeneous_set(list, EINA_TRUE);
    elm_genlist_mode_set(list, ELM_LIST_COMPRESS);

    if (ih->data->is_multiple)
      elm_genlist_multi_select_set(list, EINA_TRUE);

    evas_object_data_set(list, "_IUP_IHANDLE", ih);
    evas_object_smart_callback_add(list, "selected", eflGenlistItemSelectedCb, ih);

    ih->handle = (InativeHandle*)list;
  }
  else
  {
    list = efl_add(EFL_UI_LIST_CLASS, parent);
    if (!list)
      return IUP_ERROR;

    if (ih->data->is_multiple)
      efl_ui_multi_selectable_select_mode_set(list, EFL_UI_SELECT_MODE_MULTI);

    efl_event_callback_add(list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, eflListSelectionChangedCallback, ih);
    efl_event_callback_add(list, EFL_EVENT_POINTER_DOWN, iupeflPointerDownEvent, ih);
    efl_event_callback_add(list, EFL_EVENT_POINTER_UP, iupeflPointerUpEvent, ih);
    efl_event_callback_add(list, EFL_UI_FOCUS_MANAGER_EVENT_MANAGER_FOCUS_CHANGED, iupeflManagerFocusChangedEvent, ih);
    efl_event_callback_add(list, EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
    efl_event_callback_add(list, EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);

    ih->handle = (InativeHandle*)list;
  }

  iupeflAddToParent(ih);

  if (ih->data->has_editbox)
  {
    Eo* box = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
    if (box)
      efl_gfx_entity_visible_set(box, EINA_TRUE);
  }

  if (ih->data->is_virtual)
  {
    if (ih->data->item_count > 0)
      iupdrvListSetItemCount(ih, ih->data->item_count);
  }
  else
    iupListSetInitialItems(ih);

  if (is_dropdown || ih->data->has_editbox)
  {
    char* value = iupAttribGet(ih, "VALUE");
    if (value)
      eflListSetValueAttrib(ih, value);
  }
  else if (!ih->data->is_multiple)
  {
    int count = efl_content_count(list);
    if (count > 0)
    {
      Eo* item = efl_pack_content_get(list, 0);
      if (item)
        efl_ui_selectable_selected_set(item, EINA_TRUE);
    }
  }

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)eflListConvertXYToPos);

  if (ih->data->show_dragdrop && !ih->data->is_dropdown && !ih->data->is_multiple)
    eflListEnableDragDrop(ih);
  else if (iupAttribGetBoolean(ih, "DRAGDROPLIST"))
    eflListEnableDragDrop(ih);

  return IUP_NOERROR;
}

static void eflListUnMapMethod(Ihandle* ih)
{
  Eo* list = iupeflGetWidget(ih);
  int is_dropdown = ih->data->is_dropdown;
  Eina_Bool* prev_focused = (Eina_Bool*)iupAttribGet(ih, "_IUP_EFL_MANAGER_FOCUSED");

  if (prev_focused)
  {
    free(prev_focused);
    iupAttribSet(ih, "_IUP_EFL_MANAGER_FOCUSED", NULL);
  }

  if (list)
  {
    if (ih->data->has_editbox)
    {
      Eo* box = (Eo*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
      Eo* entry = (Eo*)iupAttribGet(ih, "_IUPEFL_ENTRY");
      efl_event_callback_del(list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, eflListSelectionChangedCallback, ih);
      efl_event_callback_del(list, EFL_EVENT_POINTER_DOWN, iupeflPointerDownEvent, ih);
      efl_event_callback_del(list, EFL_EVENT_POINTER_UP, iupeflPointerUpEvent, ih);
      efl_event_callback_del(list, EFL_UI_FOCUS_MANAGER_EVENT_MANAGER_FOCUS_CHANGED, iupeflManagerFocusChangedEvent, ih);
      efl_event_callback_del(list, EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
      efl_event_callback_del(list, EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);
      iupeflBaseRemoveCallbacks(ih, entry);
      efl_pack_clear(list);
      iupeflDelete(list);
      if (box)
        iupeflDelete(box);
    }
    else if (is_dropdown)
    {
      Eo* button = (Eo*)ih->handle;
      Eo* dropdown_list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
      Eo* win = iupeflGetMainWindow();

      if (win)
        efl_event_callback_del(win, EFL_EVENT_POINTER_DOWN, eflDropdownWindowPointerDownCallback, ih);

      if (dropdown_list)
      {
        efl_event_callback_del(dropdown_list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, eflDropdownListSelectionChangedCallback, ih);
        efl_pack_clear(dropdown_list);
        efl_del(dropdown_list);
      }

      if (button)
      {
        efl_event_callback_del(button, EFL_INPUT_EVENT_CLICKED, eflDropdownButtonClickedCallback, ih);
        efl_del(button);
      }
    }
    else if (ih->data->is_virtual)
    {
      evas_object_smart_callback_del(list, "selected", eflGenlistItemSelectedCb);
      elm_genlist_clear(list);
      evas_object_del(list);
    }
    else
    {
      if ((ih->data->show_dragdrop && !ih->data->is_dropdown && !ih->data->is_multiple) ||
          iupAttribGetBoolean(ih, "DRAGDROPLIST"))
      {
        efl_event_callback_del(list, EFL_EVENT_POINTER_DOWN, eflListDragPointerDownCb, ih);
        efl_event_callback_del(list, EFL_EVENT_POINTER_MOVE, eflListDragPointerMoveCb, ih);
        efl_event_callback_del(list, EFL_EVENT_POINTER_UP, eflListDragPointerUpCb, ih);
        efl_event_callback_del(list, EFL_UI_DND_EVENT_DROP_DROPPED, eflListDropCb, ih);
        efl_event_callback_del(list, EFL_UI_DND_EVENT_DRAG_FINISHED, eflListDragFinishedCb, ih);
      }
      efl_event_callback_del(list, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, eflListSelectionChangedCallback, ih);
      efl_event_callback_del(list, EFL_EVENT_POINTER_DOWN, iupeflPointerDownEvent, ih);
      efl_event_callback_del(list, EFL_EVENT_POINTER_UP, iupeflPointerUpEvent, ih);
      efl_event_callback_del(list, EFL_UI_FOCUS_MANAGER_EVENT_MANAGER_FOCUS_CHANGED, iupeflManagerFocusChangedEvent, ih);
      efl_event_callback_del(list, EFL_EVENT_KEY_DOWN, iupeflKeyDownEvent, ih);
      efl_event_callback_del(list, EFL_EVENT_KEY_UP, iupeflKeyUpEvent, ih);
      efl_pack_clear(list);
      iupeflDelete(list);
    }
  }

  ih->handle = NULL;
}

int iupdrvListGetCount(Ihandle* ih)
{
  Evas_Object* list;
  int is_dropdown = ih->data->is_dropdown;

  if (ih->data->is_virtual)
  {
    list = iupeflGetWidget(ih);
    if (list)
      return elm_genlist_items_count(list);
    return 0;
  }

  if (is_dropdown && !ih->data->has_editbox)
    list = (Evas_Object*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  else
    list = iupeflGetWidget(ih);

  if (!list)
    return 0;

  return efl_content_count(list);
}

void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  Eo* list;
  Eo* item;
  int is_dropdown = ih->data->is_dropdown;

  if (is_dropdown && !ih->data->has_editbox)
    list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  else
    list = iupeflGetWidget(ih);

  if (!list)
    return;

  item = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, list);
  if (item)
  {
    if (value)
      efl_text_set(item, value);
    efl_pack_end(list, item);
  }
}

void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  Eo* list;
  Eo* item;
  Eo* before;
  int is_dropdown = ih->data->is_dropdown;

  if (is_dropdown && !ih->data->has_editbox)
    list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  else
    list = iupeflGetWidget(ih);

  if (!list)
    return;

  item = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, list);
  if (item)
  {
    if (value)
      efl_text_set(item, value);

    before = efl_pack_content_get(list, pos);
    if (before)
      efl_pack_before(list, item, before);
    else
      efl_pack_end(list, item);
  }
}

void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  Eo* list;
  Eo* item;
  int is_dropdown = ih->data->is_dropdown;

  if (is_dropdown && !ih->data->has_editbox)
    list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  else
    list = iupeflGetWidget(ih);

  if (!list)
    return;

  item = efl_pack_content_get(list, pos);
  if (item)
  {
    Eo* icon = efl_content_get(efl_part(item, "icon"));
    if (icon)
    {
      const char* img_name = efl_key_data_get(icon, "_IUP_IMAGE_NAME");
      if (img_name)
        eina_stringshare_del(img_name);
    }
    efl_pack_unpack(list, item);
    efl_del(item);
  }
}

void iupdrvListRemoveAllItems(Ihandle* ih)
{
  Eo* list;
  int is_dropdown = ih->data->is_dropdown;
  int i, count;

  if (is_dropdown && !ih->data->has_editbox)
    list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  else
    list = iupeflGetWidget(ih);

  if (!list)
    return;

  count = efl_content_count(list);
  for (i = 0; i < count; i++)
  {
    Eo* item = efl_pack_content_get(list, i);
    if (item)
    {
      Eo* icon = efl_content_get(efl_part(item, "icon"));
      if (icon)
      {
        const char* img_name = efl_key_data_get(icon, "_IUP_IMAGE_NAME");
        if (img_name)
          eina_stringshare_del(img_name);
      }
    }
  }

  efl_pack_clear(list);
}

static int efl_list_item_space = -1;
static int efl_list_item_padding_w = -1;
static int efl_list_border = -1;
static int efl_dropdown_button_w = -1;
static int efl_dropdown_border_h = -1;

static void eflListMeasureMetrics(void)
{
  Eo* temp_win;
  Eo* temp_list;
  Eo* item;
  Eina_Size2D item_min;
  int char_height = 16;
  double scale;

  if (efl_list_item_space >= 0)
    return;

  temp_win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                     efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC));
  if (!temp_win)
  {
    efl_list_item_space = 4;
    efl_list_item_padding_w = 10;
    efl_list_border = 4;
    efl_dropdown_button_w = 30;
    efl_dropdown_border_h = 10;
    return;
  }

  temp_list = efl_add(EFL_UI_LIST_CLASS, temp_win);
  if (!temp_list)
  {
    efl_del(temp_win);
    efl_list_item_space = 4;
    efl_list_item_padding_w = 10;
    efl_list_border = 4;
    efl_dropdown_button_w = 30;
    efl_dropdown_border_h = 10;
    return;
  }

  item = efl_add(EFL_UI_LIST_DEFAULT_ITEM_CLASS, temp_list);
  if (item)
  {
    efl_text_set(item, "Wg");
    efl_pack_end(temp_list, item);

    efl_canvas_group_calculate(item);
    efl_canvas_group_calculate(temp_list);

    item_min = efl_gfx_hint_size_combined_min_get(item);

    iupdrvFontGetCharSize(NULL, NULL, &char_height);

    efl_list_item_space = item_min.h - char_height;
    if (efl_list_item_space < 0)
      efl_list_item_space = 0;

    efl_list_item_padding_w = 10;
    efl_list_border = 4;
  }
  else
  {
    efl_list_item_space = 4;
    efl_list_item_padding_w = 10;
    efl_list_border = 4;
  }

  efl_del(temp_win);

  scale = elm_config_scale_get();
  efl_dropdown_button_w = (int)(24 * scale);
  if (efl_dropdown_button_w < 20) efl_dropdown_button_w = 20;

  efl_dropdown_border_h = (int)(10 * scale);
  if (efl_dropdown_border_h < 6) efl_dropdown_border_h = 6;
}

void iupdrvListAddBorders(Ihandle* ih, int *w, int *h)
{
  int is_dropdown = ih->data->is_dropdown;

  eflListMeasureMetrics();

  if (is_dropdown)
  {
    *w += efl_dropdown_button_w;
    *h += efl_dropdown_border_h;
  }
  else
  {
    int sb = iupdrvGetScrollbarSize();
    *w += sb + efl_list_item_padding_w;
    *h += efl_list_border;
  }
}

void iupdrvListAddItemSpace(Ihandle* ih, int *h)
{
  (void)ih;
  eflListMeasureMetrics();
  *h += efl_list_item_space;
}

void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  Eo* list;
  Eo* item;
  int pos;

  if (ih->data->is_dropdown && !ih->data->has_editbox)
    return NULL;

  list = iupeflGetWidget(ih);
  if (!list)
    return NULL;

  pos = iupListGetPosAttrib(ih, id);
  if (pos < 0)
    return NULL;

  item = efl_pack_content_get(list, pos);
  if (!item)
    return NULL;

  return efl_content_get(efl_part(item, "icon"));
}

int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  Eo* list;
  Eo* item;
  const char* img_name;
  Evas_Object* new_img;

  if (ih->data->is_dropdown && !ih->data->has_editbox)
    return 0;

  if (!hImage)
    return 0;

  list = iupeflGetWidget(ih);
  if (!list)
    return 0;

  if (id < 0)
    return 0;

  item = efl_pack_content_get(list, id);
  if (!item)
    return 0;

  img_name = efl_key_data_get((Eo*)hImage, "_IUP_IMAGE_NAME");
  if (!img_name)
    return 0;

  new_img = iupeflImageGetImage(img_name, ih, 0);
  if (!new_img)
    return 0;

  efl_key_data_set(new_img, "_IUP_IMAGE_NAME", eina_stringshare_add(img_name));
  efl_content_set(efl_part(item, "icon"), new_img);
  return 1;
}

void iupdrvListSetItemCount(Ihandle* ih, int count)
{
  Evas_Object* list;
  int i;

  if (!ih->data->is_virtual)
    return;

  list = iupeflGetWidget(ih);
  if (!list)
    return;

  elm_genlist_clear(list);

  for (i = 0; i < count; i++)
  {
    elm_genlist_item_append(list, efl_virtual_item_class, (void*)(uintptr_t)(i + 1), NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
  }
}

static char* eflListGetIdValueAttrib(Ihandle* ih, int id)
{
  Eo* list;
  Eo* item;
  int pos;
  const char* text;

  if (ih->data->is_dropdown && !ih->data->has_editbox)
    list = (Eo*)iupAttribGet(ih, "_IUPEFL_DROPDOWN_LIST");
  else
    list = iupeflGetWidget(ih);

  if (!list)
    return NULL;

  pos = iupListGetPosAttrib(ih, id);
  if (pos < 0)
    return NULL;

  item = efl_pack_content_get(list, pos);
  if (!item)
    return NULL;

  text = efl_text_get(item);
  if (text)
    return iupStrReturnStr(text);

  return NULL;
}

static char* eflListGetImageNativeHandleAttrib(Ihandle* ih, int id)
{
  return (char*)iupdrvListGetImageHandle(ih, id);
}

void iupdrvListInitClass(Iclass* ic)
{
  ic->Map = eflListMapMethod;
  ic->UnMap = eflListUnMapMethod;

  /* Core List Attributes */
  iupClassRegisterAttributeId(ic, "IDVALUE", eflListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", eflListGetValueAttrib, eflListSetValueAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, eflListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", eflListGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* Dropdown */
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", eflListGetShowDropdownAttrib, eflListSetShowDropdownAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_DEFAULT);

  /* Editbox Attributes */
  iupClassRegisterAttribute(ic, "READONLY", eflListGetReadOnlyAttrib, eflListSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "CARET", eflListGetCaretAttrib, eflListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", eflListGetCaretPosAttrib, eflListSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", eflListGetSelectionAttrib, eflListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", eflListGetSelectionPosAttrib, eflListSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", eflListGetSelectedTextAttrib, eflListSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, eflListSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, eflListSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, eflListSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, eflListSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, eflListSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, eflListSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", eflListGetCueBannerAttrib, eflListSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* Colors */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, eflListSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* Images */
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, eflListSetImageAttrib, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", eflListGetImageNativeHandleAttrib, NULL, IUPAF_NO_STRING | IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
