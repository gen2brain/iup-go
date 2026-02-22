/** \file
 * \brief Text Control
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
#include "iup_array.h"
#include "iup_mask.h"
#include "iup_text.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"

#include "iupefl_drv.h"


/****************************************************************
                     Callbacks
****************************************************************/

static void eflTextChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  Efl_Text_Change_Info* info = (Efl_Text_Change_Info*)ev->info;
  IFnis action_cb;
  IFn value_cb;
  int ret;

  if (ih->data->disable_callbacks)
    return;

  action_cb = (IFnis)IupGetCallback(ih, "ACTION");

  if (action_cb || ih->data->mask || ih->data->nc)
  {
    const char* insert_value = NULL;
    int start, end;
    int remove_dir = 0;

    if (info->type == EFL_TEXT_CHANGE_TYPE_INSERT)
    {
      insert_value = info->content;
      start = (int)info->position;
      end = start;
    }
    else
    {
      start = (int)info->position;
      end = start + (int)info->length;
      remove_dir = 1;
    }

    ret = iupEditCallActionCb(ih, action_cb, insert_value, start, end, ih->data->mask, ih->data->nc, remove_dir, 1);

    if (ret == 0)
    {
      ih->data->disable_callbacks = 1;

      if (info->type == EFL_TEXT_CHANGE_TYPE_INSERT)
      {
        Eo* entry = ev->object;
        Efl_Text_Cursor_Object* cur_start = efl_ui_textbox_cursor_create(entry);
        Efl_Text_Cursor_Object* cur_end = efl_ui_textbox_cursor_create(entry);

        if (cur_start && cur_end)
        {
          efl_text_cursor_object_position_set(cur_start, (int)info->position);
          efl_text_cursor_object_position_set(cur_end, (int)(info->position + info->length));
          efl_text_cursor_object_range_delete(cur_start, cur_end);
        }

        if (cur_start) efl_del(cur_start);
        if (cur_end) efl_del(cur_end);
      }
      else
      {
        Eo* entry = ev->object;
        Efl_Text_Cursor_Object* cur = efl_text_interactive_main_cursor_get(entry);
        if (cur)
        {
          efl_text_cursor_object_position_set(cur, (int)info->position);
          efl_text_cursor_object_text_insert(cur, info->content);
        }
      }

      ih->data->disable_callbacks = 0;
      return;
    }
    else if (ret != -1 && info->type == EFL_TEXT_CHANGE_TYPE_INSERT && info->length == 1)
    {
      Eo* entry = ev->object;
      Efl_Text_Cursor_Object* cur_start;
      Efl_Text_Cursor_Object* cur_end;
      char replacement[2];

      ih->data->disable_callbacks = 1;

      cur_start = efl_ui_textbox_cursor_create(entry);
      cur_end = efl_ui_textbox_cursor_create(entry);

      if (cur_start && cur_end)
      {
        Efl_Text_Cursor_Object* main_cur;

        efl_text_cursor_object_position_set(cur_start, (int)info->position);
        efl_text_cursor_object_position_set(cur_end, (int)(info->position + info->length));
        efl_text_cursor_object_range_delete(cur_start, cur_end);

        replacement[0] = (char)ret;
        replacement[1] = 0;
        efl_text_cursor_object_position_set(cur_start, (int)info->position);
        efl_text_cursor_object_text_insert(cur_start, replacement);

        main_cur = efl_text_interactive_main_cursor_get(entry);
        if (main_cur)
          efl_text_cursor_object_position_set(main_cur, (int)info->position + 1);
      }

      if (cur_start) efl_del(cur_start);
      if (cur_end) efl_del(cur_end);

      ih->data->disable_callbacks = 0;
    }
  }

  value_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (value_cb)
    value_cb(ih);
}

static void eflSpinChangedCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IFni cb;
  int pos;

  if (ih->data->disable_callbacks)
    return;

  pos = (int)efl_ui_range_value_get(ev->object);

  cb = (IFni)IupGetCallback(ih, "SPIN_CB");
  if (cb)
    cb(ih, pos);

  IFn vcb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (vcb)
    vcb(ih);
}

/****************************************************************
                     Attributes
****************************************************************/

static int eflTextSetValueAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return 0;

  if (iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
  {
    if (value && value[0])
    {
      int val = atoi(value);
      ih->data->disable_callbacks = 1;
      efl_ui_range_value_set(widget, (double)val);
      ih->data->disable_callbacks = 0;
    }
    return 0;
  }

  ih->data->disable_callbacks = 1;

  if (value && value[0])
    iupeflSetText(widget, iupeflStrConvertToSystem(value));
  else
    iupeflSetText(widget, "");

  if (ih->data->is_multiline)
  {
    Efl_Text_Cursor_Object* cur = efl_text_interactive_main_cursor_get(widget);
    if (cur)
      efl_text_cursor_object_move(cur, EFL_TEXT_CURSOR_MOVE_TYPE_FIRST);
  }

  ih->data->disable_callbacks = 0;

  return 0;
}

static char* eflTextGetValueAttrib(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget)
    return NULL;

  if (iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
  {
    int val = (int)efl_ui_range_value_get(widget);
    return iupStrReturnInt(val);
  }

  const char* text = iupeflGetText(widget);
  if (!text || !text[0])
    return NULL;

  return iupStrReturnStr(text);
}

static int eflTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget || !iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (value)
  {
    int val = atoi(value);
    ih->data->disable_callbacks = 1;
    efl_ui_range_value_set(widget, (double)val);
    ih->data->disable_callbacks = 0;
  }

  return 0;
}

static char* eflTextGetSpinValueAttrib(Ihandle* ih)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget || !iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return "0";

  int val = (int)efl_ui_range_value_get(widget);
  return iupStrReturnInt(val);
}

static int eflTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);
  double min, max;

  if (!widget || !iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  efl_ui_range_limits_get(widget, &min, &max);
  if (value)
    min = (double)atoi(value);
  efl_ui_range_limits_set(widget, min, max);

  return 0;
}

static int eflTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);
  double min, max;

  if (!widget || !iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  efl_ui_range_limits_get(widget, &min, &max);
  if (value)
    max = (double)atoi(value);
  efl_ui_range_limits_set(widget, min, max);

  return 0;
}

static int eflTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);

  if (!widget || !iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (value)
  {
    int inc = atoi(value);
    efl_ui_range_step_set(widget, (double)inc);
  }

  return 0;
}

static int eflTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  efl_text_interactive_editable_set(entry, iupStrBoolean(value) ? EINA_FALSE : EINA_TRUE);

  return 0;
}

static char* eflTextGetReadOnlyAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return "NO";

  return efl_text_interactive_editable_get(entry) ? "NO" : "YES";
}

static int eflTextSetPasswordAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  efl_text_password_set(entry, iupStrBoolean(value) ? EINA_TRUE : EINA_FALSE);

  return 0;
}

static int eflTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  int pos = 0;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  iupStrToInt(value, &pos);

  Efl_Text_Cursor_Object* cur = efl_text_interactive_main_cursor_get(entry);
  if (cur)
    efl_text_cursor_object_position_set(cur, pos);

  return 0;
}

static char* eflTextGetCaretPosAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  int pos = 0;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return "0";

  Efl_Text_Cursor_Object* cur = efl_text_interactive_main_cursor_get(entry);
  if (cur)
    pos = efl_text_cursor_object_position_get(cur);

  return iupStrReturnInt(pos);
}

static int eflTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  int start = 0, end = 0;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (!value || !value[0])
  {
    efl_text_interactive_all_unselect(entry);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    efl_text_interactive_all_select(entry);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':'))
  {
    Efl_Text_Cursor_Object* cur = efl_ui_textbox_cursor_create(entry);
    Efl_Text_Cursor_Object* cur2 = efl_ui_textbox_cursor_create(entry);

    if (cur && cur2)
    {
      efl_text_cursor_object_position_set(cur, start);
      efl_text_cursor_object_position_set(cur2, end);
      efl_text_interactive_selection_cursors_set(entry, cur, cur2);
    }
  }

  return 0;
}

static char* eflTextGetSelectionAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  Efl_Text_Cursor_Object *sel_start, *sel_end;
  int start, end;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return NULL;

  if (!efl_text_interactive_have_selection_get(entry))
    return NULL;

  efl_text_interactive_selection_cursors_get(entry, &sel_start, &sel_end);
  if (!sel_start || !sel_end)
    return NULL;

  start = efl_text_cursor_object_position_get(sel_start);
  end = efl_text_cursor_object_position_get(sel_end);

  return iupStrReturnIntInt(start, end, ':');
}

static char* eflTextGetSelectedTextAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  Efl_Text_Cursor_Object *sel_start, *sel_end;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return NULL;

  if (!efl_text_interactive_have_selection_get(entry))
    return NULL;

  efl_text_interactive_selection_cursors_get(entry, &sel_start, &sel_end);
  if (!sel_start || !sel_end)
    return NULL;

  char* text = efl_text_cursor_object_range_text_get(sel_start, sel_end);
  if (text && text[0])
  {
    char* ret = iupStrReturnStr(text);
    free(text);
    return ret;
  }

  return NULL;
}

static int eflTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  Efl_Text_Cursor_Object *sel_start, *sel_end;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (!efl_text_interactive_have_selection_get(entry))
    return 0;

  efl_text_interactive_selection_cursors_get(entry, &sel_start, &sel_end);
  if (!sel_start || !sel_end)
    return 0;

  ih->data->disable_callbacks = 1;

  efl_text_cursor_object_range_delete(sel_start, sel_end);

  if (value && value[0])
    efl_text_cursor_object_text_insert(sel_start, iupeflStrConvertToSystem(value));

  ih->data->disable_callbacks = 0;

  return 0;
}

static int eflTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  Efl_Text_Cursor_Object* cur;
  int pos;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  ih->data->disable_callbacks = 1;

  cur = efl_text_interactive_main_cursor_get(entry);
  if (cur)
  {
    efl_text_cursor_object_move(cur, EFL_TEXT_CURSOR_MOVE_TYPE_LAST);
    pos = efl_text_cursor_object_position_get(cur);

    if (ih->data->is_multiline && ih->data->append_newline && pos != 0)
      efl_text_cursor_object_text_insert(cur, "\n");

    if (value)
      efl_text_cursor_object_text_insert(cur, value);

    efl_text_cursor_object_move(cur, EFL_TEXT_CURSOR_MOVE_TYPE_LAST);

    if (ih->data->is_multiline)
    {
      Eina_Rect rect = efl_text_cursor_object_cursor_geometry_get(cur, EFL_TEXT_CURSOR_TYPE_BEFORE);
      efl_ui_scrollable_scroll(entry, rect, EINA_FALSE);
    }
  }

  ih->data->disable_callbacks = 0;

  return 0;
}

static int eflTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (!value)
    value = "";

  ih->data->disable_callbacks = 1;

  Efl_Text_Cursor_Object* cur = efl_text_interactive_main_cursor_get(entry);
  if (cur)
    efl_text_cursor_object_text_insert(cur, value);

  ih->data->disable_callbacks = 0;

  return 0;
}

static int eflTextSetActiveAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);

  if (!entry)
    return 0;

  iupeflSetDisabled(entry, iupStrBoolean(value) ? EINA_FALSE : EINA_TRUE);

  return 0;
}

static int eflTextSetBgColorAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  unsigned char r, g, b;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  efl_text_background_type_set(entry, EFL_TEXT_STYLE_BACKGROUND_TYPE_SOLID_COLOR);
  efl_text_background_color_set(entry, r, g, b, 255);
  return 1;
}

static int eflTextSetFgColorAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  unsigned char r, g, b;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  efl_text_color_set(entry, r, g, b, 255);
  return 1;
}

static int eflTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  int pos = 0;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  iupStrToInt(value, &pos);

  Efl_Text_Cursor_Object* cur = efl_text_interactive_main_cursor_get(entry);
  if (cur)
    efl_text_cursor_object_position_set(cur, pos);

  return 0;
}

static int eflTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  return eflTextSetScrollToAttrib(ih, value);
}

static int eflTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  double align = 0.0;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (iupStrEqualNoCase(value, "ACENTER"))
    align = 0.5;
  else if (iupStrEqualNoCase(value, "ARIGHT"))
    align = 1.0;

  efl_text_horizontal_align_set(entry, align);
  return 1;
}

static char* eflTextGetAlignmentAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  double align;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return "ALEFT";

  align = efl_text_horizontal_align_get(entry);
  if (align > 0.75)
    return "ARIGHT";
  else if (align > 0.25)
    return "ACENTER";
  return "ALEFT";
}

static int eflTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  Efl_Text_Cursor_Object* cursor;
  Efl_Text_Cursor_Object* line_cursor;
  int lin = 1, col = 1;
  int line_start_pos;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (!value)
    return 0;

  iupStrToIntInt(value, &lin, &col, ',');
  if (lin < 1) lin = 1;
  if (col < 1) col = 1;

  cursor = efl_text_interactive_main_cursor_get(entry);
  if (!cursor)
    return 0;

  line_cursor = efl_ui_textbox_cursor_create(entry);
  if (!line_cursor)
    return 0;

  efl_text_cursor_object_line_number_set(line_cursor, lin - 1);
  efl_text_cursor_object_move(line_cursor, EFL_TEXT_CURSOR_MOVE_TYPE_LINE_START);
  line_start_pos = efl_text_cursor_object_position_get(line_cursor);

  efl_text_cursor_object_position_set(cursor, line_start_pos + col - 1);

  efl_del(line_cursor);

  return 0;
}

static char* eflTextGetCaretAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  Efl_Text_Cursor_Object* cursor;
  Efl_Text_Cursor_Object* line_cursor;
  int lin, col;
  int line_start_pos, cursor_pos;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return NULL;

  cursor = efl_text_interactive_main_cursor_get(entry);
  if (!cursor)
    return NULL;

  lin = efl_text_cursor_object_line_number_get(cursor) + 1;

  line_cursor = efl_ui_textbox_cursor_create(entry);
  if (!line_cursor)
    return iupStrReturnIntInt(lin, 1, ',');

  efl_text_cursor_object_line_number_set(line_cursor, lin - 1);
  efl_text_cursor_object_move(line_cursor, EFL_TEXT_CURSOR_MOVE_TYPE_LINE_START);
  line_start_pos = efl_text_cursor_object_position_get(line_cursor);
  cursor_pos = efl_text_cursor_object_position_get(cursor);
  col = cursor_pos - line_start_pos + 1;

  efl_del(line_cursor);

  return iupStrReturnIntInt(lin, col, ',');
}

static int eflTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  Efl_Text_Cursor_Object *cursor_start, *cursor_end;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (!value)
    return 0;

  if (iupStrEqualNoCase(value, "COPY"))
    efl_ui_textbox_selection_copy(entry);
  else if (iupStrEqualNoCase(value, "CUT"))
    efl_ui_textbox_selection_cut(entry);
  else if (iupStrEqualNoCase(value, "PASTE"))
    efl_ui_textbox_selection_paste(entry);
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    if (efl_text_interactive_have_selection_get(entry))
    {
      efl_text_interactive_selection_cursors_get(entry, &cursor_start, &cursor_end);
      if (cursor_start && cursor_end)
        efl_text_cursor_object_range_delete(cursor_start, cursor_end);
    }
  }

  return 0;
}

static char* eflTextGetCountAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  const char* text;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return "0";

  text = efl_text_get(entry);
  if (!text)
    return "0";

  return iupStrReturnInt((int)strlen(text));
}

static char* eflTextGetLineCountAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  const char* text;
  const char* p;
  int count = 1;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return "1";

  text = efl_text_get(entry);
  if (!text || !*text)
    return "1";

  p = text;
  while (*p)
  {
    if (*p == '\n')
      count++;
    p++;
  }

  return iupStrReturnInt(count);
}

static char* eflTextGetLineValueAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  Efl_Text_Cursor_Object* cursor;
  Efl_Text_Cursor_Object* line_start;
  Efl_Text_Cursor_Object* line_end;
  char* line_text;
  char* ret;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return NULL;

  cursor = efl_text_interactive_main_cursor_get(entry);
  if (!cursor)
    return NULL;

  line_start = efl_ui_textbox_cursor_create(entry);
  line_end = efl_ui_textbox_cursor_create(entry);
  if (!line_start || !line_end)
  {
    if (line_start) efl_del(line_start);
    if (line_end) efl_del(line_end);
    return NULL;
  }

  efl_text_cursor_object_position_set(line_start, efl_text_cursor_object_position_get(cursor));
  efl_text_cursor_object_position_set(line_end, efl_text_cursor_object_position_get(cursor));

  efl_text_cursor_object_move(line_start, EFL_TEXT_CURSOR_MOVE_TYPE_LINE_START);
  efl_text_cursor_object_move(line_end, EFL_TEXT_CURSOR_MOVE_TYPE_LINE_END);

  line_text = efl_text_cursor_object_range_text_get(line_start, line_end);

  efl_del(line_start);
  efl_del(line_end);

  if (line_text)
  {
    ret = iupStrReturnStr(line_text);
    free(line_text);
    return ret;
  }

  return NULL;
}

static int eflTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  int start = 0, end = 0;
  Efl_Text_Cursor_Object* cursor_start;
  Efl_Text_Cursor_Object* cursor_end;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (!value || !value[0])
  {
    efl_text_interactive_all_unselect(entry);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;

  if (start < 0) start = 0;
  if (end < 0) end = 0;

  cursor_start = efl_ui_textbox_cursor_create(entry);
  cursor_end = efl_ui_textbox_cursor_create(entry);

  if (cursor_start && cursor_end)
  {
    efl_text_cursor_object_position_set(cursor_start, start);
    efl_text_cursor_object_position_set(cursor_end, end);
    efl_text_interactive_selection_cursors_set(entry, cursor_start, cursor_end);
  }

  return 0;
}

static char* eflTextGetSelectionPosAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  Efl_Text_Cursor_Object *sel_start, *sel_end;
  int start_pos, end_pos;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return NULL;

  if (!efl_text_interactive_have_selection_get(entry))
    return NULL;

  efl_text_interactive_selection_cursors_get(entry, &sel_start, &sel_end);
  if (!sel_start || !sel_end)
    return NULL;

  start_pos = efl_text_cursor_object_position_get(sel_start);
  end_pos = efl_text_cursor_object_position_get(sel_end);

  if (start_pos == end_pos)
    return NULL;

  return iupStrReturnIntInt(start_pos, end_pos, ':');
}

static int eflTextSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    ih->data->nc = 0;
  else
    ih->data->nc = atoi(value);
  return 0;
}

static int eflTextSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  efl_text_set(efl_part(entry, "efl.text_guide"), value ? value : "");
  return 1;
}

static char* eflTextGetCueBannerAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  const char* text;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return NULL;

  text = efl_text_get(efl_part(entry, "efl.text_guide"));
  if (!text || !*text)
    return NULL;
  return iupStrReturnStr(text);
}

static int eflTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  Eo* entry = iupeflGetWidget(ih);
  int tabsize = 8;
  int charwidth, charheight;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return 0;

  if (value)
    iupStrToInt(value, &tabsize);

  iupdrvFontGetCharSize(ih, &charwidth, &charheight);
  if (charwidth <= 0) charwidth = 8;

  efl_text_tab_stops_set(entry, tabsize * charwidth);
  return 0;
}

static char* eflTextGetTabSizeAttrib(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);
  int tabpixels;
  int charwidth, charheight;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return "8";

  tabpixels = efl_text_tab_stops_get(entry);

  iupdrvFontGetCharSize(ih, &charwidth, &charheight);
  if (charwidth > 0)
    return iupStrReturnInt(tabpixels / charwidth);
  return "8";
}

static int eflTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  int horiz = 0, vert = 0;
  Eo* entry;

  iupStrToIntInt(value, &horiz, &vert, 'x');

  ih->data->horiz_padding = horiz;
  ih->data->vert_padding = vert;

  entry = iupeflGetWidget(ih);
  if (entry && !iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
  {
    efl_gfx_hint_margin_set(entry, horiz, horiz, vert, vert);
  }

  return 0;
}

static char* eflTextGetPaddingAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->horiz_padding, ih->data->vert_padding, 'x');
}

/****************************************************************
                     Methods
****************************************************************/

static int eflTextMapMethod(Ihandle* ih)
{
  Eo* parent;
  Eo* parent_win;
  Eo* widget;
  char* value;
  int is_spin;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  /* EFL_UI_TEXTBOX_CLASS and EFL_UI_SPIN_BUTTON_CLASS require a proper window parent for their internal canvas objects. */
  parent_win = efl_provider_find(parent, EFL_UI_WIN_CLASS);
  if (!parent_win)
    parent_win = iupeflGetMainWindow();
  if (!parent_win)
    return IUP_ERROR;

  is_spin = iupAttribGetBoolean(ih, "SPIN");

  if (is_spin && !ih->data->is_multiline)
  {
    double min = (double)iupAttribGetInt(ih, "SPINMIN");
    double max = (double)iupAttribGetInt(ih, "SPINMAX");
    double inc = (double)iupAttribGetInt(ih, "SPININC");
    double val = (double)iupAttribGetInt(ih, "SPINVALUE");

    if (max <= min) max = 100;
    if (inc <= 0) inc = 1;

    widget = efl_add(EFL_UI_SPIN_BUTTON_CLASS, parent_win);
    if (!widget)
      return IUP_ERROR;

    efl_ui_range_limits_set(widget, min, max);
    efl_ui_range_step_set(widget, inc);
    efl_ui_range_value_set(widget, val);
    efl_ui_spin_button_direct_text_input_set(widget, EINA_TRUE);

    if (iupAttribGetBoolean(ih, "SPINWRAP"))
      efl_ui_spin_button_wraparound_set(widget, EINA_TRUE);

    efl_event_callback_add(widget, EFL_UI_RANGE_EVENT_CHANGED, eflSpinChangedCallback, ih);

    iupAttribSet(ih, "_IUP_EFL_IS_SPINNER", "1");
  }
  else
  {
    widget = efl_add(EFL_UI_TEXTBOX_CLASS, parent_win);
    if (!widget)
      return IUP_ERROR;

    if (ih->data->is_multiline)
    {
      efl_text_multiline_set(widget, EINA_TRUE);
      efl_ui_textbox_scrollable_set(widget, EINA_TRUE);

      if (iupAttribGetBoolean(ih, "WORDWRAP"))
        efl_text_wrap_set(widget, EFL_TEXT_FORMAT_WRAP_WORD);
      else
        efl_text_wrap_set(widget, EFL_TEXT_FORMAT_WRAP_NONE);
    }
    else
    {
      efl_text_multiline_set(widget, EINA_FALSE);
      efl_ui_textbox_scrollable_set(widget, EINA_TRUE);
    }

    if (iupAttribGetBoolean(ih, "PASSWORD"))
      efl_text_password_set(widget, EINA_TRUE);

    value = iupAttribGet(ih, "VALUE");
    if (value && value[0])
      iupeflSetText(widget, iupeflStrConvertToSystem(value));

    /* EFL_TEXT_INTERACTIVE_EVENT_CHANGED_USER only fires for user-initiated changes, not programmatic style changes. */
    efl_event_callback_add(widget, EFL_TEXT_INTERACTIVE_EVENT_CHANGED_USER, eflTextChangedCallback, ih);
  }

  ih->handle = (InativeHandle*)widget;

  iupeflBaseAddCallbacks(ih, widget);

  iupeflAddToParent(ih);

  iupeflApplyTextStyle(ih, widget);

  return IUP_NOERROR;
}

static void eflTextUnMapMethod(Ihandle* ih)
{
  Eo* entry = iupeflGetWidget(ih);

  if (entry)
  {
    if (iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
      efl_event_callback_del(entry, EFL_UI_RANGE_EVENT_CHANGED, eflSpinChangedCallback, ih);
    else
      efl_event_callback_del(entry, EFL_TEXT_INTERACTIVE_EVENT_CHANGED_USER, eflTextChangedCallback, ih);

    iupeflDelete(entry);
  }
}

static int eflTextSetFontAttrib(Ihandle* ih, const char* value)
{
  Eo* widget = iupeflGetWidget(ih);

  iupdrvSetFontAttrib(ih, value);

  if (widget)
    iupeflApplyTextStyle(ih, widget);

  return 1;
}

/****************************************************************
                     Driver Functions
****************************************************************/

static int efl_text_border_x = -1;
static int efl_text_border_y = -1;
static int efl_text_multiline_border_x = -1;
static int efl_text_multiline_border_y = -1;
static int efl_spinner_width = -1;
static int efl_spinner_height = -1;

static void eflTextMeasureBorders(void)
{
  Eo* temp_win;
  Eo* textbox;
  Eina_Size2D single_min;
  int charwidth, charheight;

  if (efl_text_border_x >= 0)
    return;

  temp_win = efl_add(EFL_UI_WIN_CLASS, iupeflGetLoop(),
                     efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_BASIC),
                     efl_text_set(efl_added, "iup_text_measure"));
  if (!temp_win)
  {
    efl_text_border_x = 10;
    efl_text_border_y = 6;
    efl_text_multiline_border_x = 10;
    efl_text_multiline_border_y = 4;
    efl_spinner_width = 80;
    efl_spinner_height = 34;
    return;
  }

  textbox = efl_add(EFL_UI_TEXTBOX_CLASS, temp_win);
  if (textbox)
  {
    Efl_Text_Cursor_Object* cursor;
    Eina_Rect cursor_rect;

    efl_text_multiline_set(textbox, EINA_FALSE);
    efl_text_set(textbox, "W");
    efl_gfx_entity_visible_set(textbox, EINA_TRUE);
    efl_canvas_group_calculate(textbox);
    single_min = efl_gfx_hint_size_combined_min_get(textbox);

    cursor = efl_text_interactive_main_cursor_get(textbox);
    if (cursor)
    {
      cursor_rect = efl_text_cursor_object_cursor_geometry_get(cursor, EFL_TEXT_CURSOR_TYPE_BEFORE);
      charwidth = cursor_rect.w > 0 ? cursor_rect.w : 8;
      charheight = cursor_rect.h > 0 ? cursor_rect.h : 16;
    }
    else
    {
      iupdrvFontGetCharSize(NULL, &charwidth, &charheight);
      if (charwidth <= 0) charwidth = 8;
      if (charheight <= 0) charheight = 16;
    }

    efl_text_border_x = single_min.w - charwidth;
    efl_text_border_y = single_min.h - charheight;

    if (efl_text_border_x < 0) efl_text_border_x = 0;
    if (efl_text_border_y < 0) efl_text_border_y = 0;

    efl_del(textbox);
  }
  else
  {
    efl_text_border_x = 10;
    efl_text_border_y = 6;
  }

  efl_text_multiline_border_x = efl_text_border_x;
  efl_text_multiline_border_y = efl_text_border_y;

  {
    Eo* spinner = efl_add(EFL_UI_SPIN_BUTTON_CLASS, temp_win);
    if (spinner)
    {
      Eina_Size2D spin_min;
      efl_ui_range_limits_set(spinner, 0, 100);
      efl_canvas_group_calculate(spinner);
      spin_min = efl_gfx_hint_size_combined_min_get(spinner);
      efl_spinner_width = spin_min.w;
      efl_spinner_height = spin_min.h;
      if (efl_spinner_width < 80) efl_spinner_width = 80;
      if (efl_spinner_height < 20) efl_spinner_height = 34;
      efl_del(spinner);
    }
    else
    {
      efl_spinner_width = 80;
      efl_spinner_height = 34;
    }
  }

  efl_del(temp_win);
}

void iupeflTextGetBorder(int* border_x, int* border_y)
{
  eflTextMeasureBorders();

  if (border_x)
    *border_x = efl_text_border_x;
  if (border_y)
    *border_y = efl_text_border_y;
}

void iupdrvTextAddBorders(Ihandle* ih, int* w, int* h)
{
  eflTextMeasureBorders();

  if (ih->data && ih->data->is_multiline)
  {
    *w += efl_text_multiline_border_x;
    *h += efl_text_multiline_border_y;
  }
  else
  {
    if (iupAttribGetBoolean(ih, "SPIN"))
    {
      int add = efl_spinner_height - *h;
      if (add < 0) add = 0;
      *h += add;
    }
    else
    {
      *w += efl_text_border_x;
      *h += efl_text_border_y;
    }
  }
}

void iupdrvTextAddSpin(Ihandle* ih, int* w, int h)
{
  (void)ih;
  (void)h;
  eflTextMeasureBorders();

  if (*w < efl_spinner_width)
    *w = efl_spinner_width;
}

void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int* pos)
{
  Eo* entry = iupeflGetWidget(ih);
  const char* text;
  int current_line = 1;
  int current_pos = 0;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
  {
    *pos = (lin - 1) * 80 + (col - 1);
    return;
  }

  text = iupeflGetText(entry);
  if (!text)
  {
    *pos = 0;
    return;
  }

  const char* p = text;
  while (*p && current_line < lin)
  {
    if (*p == '\n')
      current_line++;
    current_pos++;
    p++;
  }

  current_pos += (col - 1);

  int text_len = strlen(text);
  if (current_pos > text_len)
    current_pos = text_len;

  *pos = current_pos;
}

void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int* lin, int* col)
{
  Eo* entry = iupeflGetWidget(ih);
  const char* text;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
  {
    *lin = (pos / 80) + 1;
    *col = (pos % 80) + 1;
    return;
  }

  text = iupeflGetText(entry);
  if (!text)
  {
    *lin = 1;
    *col = 1;
    return;
  }

  int current_line = 1;
  int current_col = 1;
  int current_pos = 0;
  const char* p = text;

  while (*p && current_pos < pos)
  {
    if (*p == '\n')
    {
      current_line++;
      current_col = 1;
    }
    else
    {
      current_col++;
    }
    current_pos++;
    p++;
  }

  *lin = current_line;
  *col = current_col;
}

void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  (void)ih;
  return NULL;
}

void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  (void)ih;
  (void)state;
}

void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
  Eo* entry = iupeflGetWidget(ih);
  char* selectionpos;
  int start_pos, end_pos;
  char format[256] = "";
  Efl_Text_Cursor_Object* start_cursor;
  Efl_Text_Cursor_Object* end_cursor;

  (void)bulk;

  if (!ih->data->is_multiline)
    return;

  if (!entry || iupAttribGet(ih, "_IUP_EFL_IS_SPINNER"))
    return;

  selectionpos = iupAttribGet(formattag, "SELECTIONPOS");
  if (!selectionpos)
    return;

  if (iupStrToIntInt(selectionpos, &start_pos, &end_pos, ':') != 2)
    return;

  start_cursor = efl_ui_textbox_cursor_create(entry);
  end_cursor = efl_ui_textbox_cursor_create(entry);

  if (!start_cursor || !end_cursor)
  {
    if (start_cursor) efl_del(start_cursor);
    if (end_cursor) efl_del(end_cursor);
    return;
  }

  efl_text_cursor_object_position_set(start_cursor, start_pos);
  efl_text_cursor_object_position_set(end_cursor, end_pos);

  {
    char* bgcolor = iupAttribGet(formattag, "BGCOLOR");
    if (bgcolor)
    {
      unsigned char r, g, b;
      if (iupStrToRGB(bgcolor, &r, &g, &b))
      {
        char color_str[64];
        sprintf(color_str, "background_type=solid background_color=#%02X%02X%02X ", r, g, b);
        strcat(format, color_str);
      }
    }
  }

  {
    char* fgcolor = iupAttribGet(formattag, "FGCOLOR");
    if (fgcolor)
    {
      unsigned char r, g, b;
      if (iupStrToRGB(fgcolor, &r, &g, &b))
      {
        char color_str[32];
        sprintf(color_str, "color=#%02X%02X%02X ", r, g, b);
        strcat(format, color_str);
      }
    }
  }

  {
    char* underline = iupAttribGet(formattag, "UNDERLINE");
    if (underline && iupStrBoolean(underline))
      strcat(format, "underline=single ");
  }

  {
    char* bold = iupAttribGet(formattag, "BOLD");
    if (bold && iupStrBoolean(bold))
      strcat(format, "font_weight=bold ");
  }

  {
    char* italic = iupAttribGet(formattag, "ITALIC");
    if (italic && iupStrBoolean(italic))
      strcat(format, "font_style=italic ");
  }

  if (format[0])
    efl_text_formatter_attribute_insert(start_cursor, end_cursor, format);

  efl_del(start_cursor);
  efl_del(end_cursor);
}

void iupdrvTextAddExtraPadding(Ihandle* ih, int* w, int* h)
{
  (void)ih;
  eflTextMeasureBorders();

  if (w) *w += 4;
  if (h) *h += efl_text_border_y > 4 ? efl_text_border_y / 2 : 4;
}

void iupdrvTextInitClass(Iclass* ic)
{
  ic->Map = eflTextMapMethod;
  ic->UnMap = eflTextUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", eflTextGetValueAttrib, eflTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", eflTextGetReadOnlyAttrib, eflTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, eflTextSetPasswordAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "CARETPOS", eflTextGetCaretPosAttrib, eflTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", eflTextGetSelectionAttrib, eflTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", eflTextGetSelectedTextAttrib, eflTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, eflTextSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, eflTextSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, eflTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, eflTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupeflBaseGetActiveAttrib, eflTextSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, eflTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, eflTextSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FONT", NULL, eflTextSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ALIGNMENT", eflTextGetAlignmentAttrib, eflTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", eflTextGetCaretAttrib, eflTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, eflTextSetClipboardAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", eflTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", eflTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", eflTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", eflTextGetSelectionPosAttrib, eflTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, eflTextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CUEBANNER", eflTextGetCueBannerAttrib, eflTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", eflTextGetTabSizeAttrib, eflTextSetTabSizeAttrib, "8", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PADDING", eflTextGetPaddingAttrib, eflTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SPINVALUE", eflTextGetSpinValueAttrib, eflTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, eflTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, eflTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, eflTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_IHANDLE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FILTER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
