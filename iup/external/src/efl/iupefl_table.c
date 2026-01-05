/** \file
 * \brief IupTable control
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
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_stdcontrols.h"
#include "iup_tablecontrol.h"
#include "iup_key.h"
#include "iup_focus.h"

#include "iupefl_drv.h"


typedef struct _IeflTableData
{
  Evas_Object* scroller;       /* elm_scroller - main scrollable container (normal mode) */
  Evas_Object* table;          /* elm_table - grid layout (normal mode) */
  Evas_Object* top_spacer;     /* Spacer above visible cells (virtual mode) */
  Evas_Object* bottom_spacer;  /* Spacer below visible cells (virtual mode) */
  Evas_Object** header_labels; /* Array of column header labels */
  Evas_Object** cell_labels;   /* 2D array (flattened): [(lin-1) * num_col + (col-1)] */
  Evas_Object** cell_bgs;      /* Background rectangles for cells (for colors) */
  Evas_Object* focus_rect;     /* Focus rectangle overlay */
  int focus_rect_lin;          /* Current focus rect position */
  int focus_rect_col;
  int header_height;           /* Height of header row */
  int row_height;              /* Height of data rows */
  int* col_widths;             /* Array of column widths */
  int alloc_num_col;           /* Allocated size of header_labels and col_widths */
  int alloc_num_lin;           /* Allocated rows for cell_labels/cell_bgs */
  int selected_lin;            /* Currently selected row (1-based, 0 = none) */
  int selected_col;            /* Currently selected column (1-based, 0 = none) */
  int target_height;           /* Target height for VISIBLELINES constraint (0 = no constraint) */
  int is_virtual;              /* 1 if VIRTUALMODE=YES */
  int has_focus;               /* 1 if table control has keyboard focus */
  unsigned char sel_r, sel_g, sel_b;  /* Selection color (queried or default) */
  int editing_lin;             /* Row being edited (1-based, 0 = not editing) */
  int editing_col;             /* Column being edited (1-based, 0 = not editing) */
  char* original_value;        /* Value before editing started (for cancel) */
  /* Virtual mode specific */
  int first_visible_row;       /* First visible row (1-based) in virtual mode */
  int visible_row_count;       /* Number of visible rows in virtual mode */
  Ecore_Job* scroll_job;       /* Pending scroll update job (for debouncing) */
  /* Sorting */
  int sort_column;             /* Currently sorted column (1-based, 0=none) */
  int sort_ascending;          /* 1=ascending, 0=descending */
} IeflTableData;

#define IEFL_TABLE_DATA(ih) ((IeflTableData*)(ih->data->native_data))

#define DEFAULT_COL_WIDTH 80
#define DEFAULT_ROW_HEIGHT 24
#define DEFAULT_HEADER_HEIGHT 28
#define CELL_PADDING 4

/* Forward declarations */
static void eflTableUpdateVisibleRows(Ihandle* ih);
static void eflTableRebuildVirtualCells(Ihandle* ih);
static void eflTableUpdateSortIndicators(Ihandle* ih);
static void eflTableSortRows(Ihandle* ih, int col, int ascending);
static void eflTableRefreshCells(Ihandle* ih);
static void eflTableRefreshHeaders(Ihandle* ih);
static void eflTableDoSort(Ihandle* ih, int col);
static int eflTableFindTargetColumn(Ihandle* ih, int x);
static void eflTableSwapColumns(Ihandle* ih, int col1, int col2);
static void eflTableUpdateDragIndicator(Ihandle* ih, int target);
static void eflTableHideDragIndicator(Ihandle* ih);
static void eflTableDragPointerMove(void* cb_data, const Efl_Event* ev);
static void eflTableDragPointerUp(void* cb_data, const Efl_Event* ev);

/* ========================================================================= */
/* Helper Functions                                                          */
/* ========================================================================= */

static int eflTableColHasExplicitWidth(Ihandle* ih, int col)
{
  char name[50];

  sprintf(name, "RASTERWIDTH%d", col);
  if (iupAttribGet(ih, name))
    return 1;

  sprintf(name, "WIDTH%d", col);
  if (iupAttribGet(ih, name))
    return 1;

  return 0;
}

static int eflTableGetColumnAlignment(Ihandle* ih, int col)
{
  char name[50];
  char* value;

  sprintf(name, "ALIGNMENT%d", col);
  value = iupAttribGet(ih, name);

  if (value)
  {
    if (iupStrEqualNoCase(value, "ARIGHT"))
      return 1;
    else if (iupStrEqualNoCase(value, "ACENTER"))
      return 0;
  }

  return -1;  /* ALEFT (default) */
}

static void eflTableGetCellBgColor(Ihandle* ih, int lin, int col, unsigned char* r, unsigned char* g, unsigned char* b)
{
  char* bgcolor = NULL;
  char* alternate_color;

  /* Priority: per-cell > per-row > per-column > alternating > global */
  if (lin > 0 && col > 0)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);

  if (!bgcolor && lin > 0)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);

  if (!bgcolor && col > 0)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);

  if (!bgcolor && lin > 0)
  {
    alternate_color = iupAttribGet(ih, "ALTERNATECOLOR");
    if (iupStrBoolean(alternate_color))
    {
      if (lin % 2 == 0)
        bgcolor = iupAttribGet(ih, "EVENROWCOLOR");
      else
        bgcolor = iupAttribGet(ih, "ODDROWCOLOR");
    }
  }

  if (!bgcolor)
    bgcolor = iupAttribGetStr(ih, "BGCOLOR");

  if (bgcolor && iupStrToRGB(bgcolor, r, g, b))
    return;

  *r = 255; *g = 255; *b = 255;
}

static char* eflTableGetCellFont(Ihandle* ih, int lin, int col)
{
  char* font = NULL;

  /* Priority: per-cell > per-row > per-column > global */
  if (lin > 0 && col > 0)
    font = iupAttribGetId2(ih, "FONT", lin, col);

  if (!font && lin > 0)
    font = iupAttribGetId2(ih, "FONT", lin, 0);

  if (!font && col > 0)
    font = iupAttribGetId2(ih, "FONT", 0, col);

  if (!font)
    font = iupAttribGetStr(ih, "FONT");

  return font;
}

static void eflTableGetCellFgColor(Ihandle* ih, int lin, int col, unsigned char* r, unsigned char* g, unsigned char* b)
{
  char* fgcolor = NULL;

  /* Priority: per-cell > per-row > per-column > global */
  if (lin > 0 && col > 0)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);

  if (!fgcolor && lin > 0)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);

  if (!fgcolor && col > 0)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);

  if (!fgcolor)
    fgcolor = iupAttribGetStr(ih, "FGCOLOR");

  if (fgcolor && iupStrToRGB(fgcolor, r, g, b))
    return;

  *r = 0; *g = 0; *b = 0;
}

static void eflTableUpdateFocusRect(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int lin, col;
  int num_col = ih->data->num_col;
  int idx;
  Evas_Object* cell_widget;
  int x, y, w, h;

  if (!data || !data->focus_rect || !data->table)
    return;

  /* Hide focus rect if FOCUSRECT=NO */
  if (!iupAttribGetBoolean(ih, "FOCUSRECT"))
  {
    evas_object_hide(data->focus_rect);
    return;
  }

  lin = data->selected_lin;
  col = data->selected_col;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
  {
    evas_object_hide(data->focus_rect);
    return;
  }

  /* Get the cell background to overlay */
  idx = (lin - 1) * num_col + (col - 1);
  cell_widget = data->cell_bgs ? data->cell_bgs[idx] : NULL;
  if (!cell_widget)
    cell_widget = data->cell_labels ? data->cell_labels[idx] : NULL;
  if (!cell_widget)
  {
    evas_object_hide(data->focus_rect);
    return;
  }

  /* Get cell geometry and position focus rect as overlay */
  evas_object_geometry_get(cell_widget, &x, &y, &w, &h);
  evas_object_geometry_set(data->focus_rect, x, y, w, h);

  data->focus_rect_lin = lin;
  data->focus_rect_col = col;

  /* Style: semi-transparent overlay, dimmed when table doesn't have focus */
  if (data->has_focus)
    evas_object_color_set(data->focus_rect, 0, 0, 0, 80);
  else
    evas_object_color_set(data->focus_rect, 0, 0, 0, 30);

  evas_object_raise(data->focus_rect);
  evas_object_show(data->focus_rect);
}

static char* eflTableGetCellText(Ihandle* ih, int lin, int col)
{
  char name[50];
  char* value;
  sIFnii value_cb;

  value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
  if (value_cb)
  {
    value = value_cb(ih, lin, col);
    if (value)
    {
      return value;
    }
  }

  sprintf(name, "CELLVALUE%d:%d", lin, col);
  value = iupAttribGet(ih, name);
  return value;
}

static char* eflTableGetHeaderText(Ihandle* ih, int col)
{
  char name[50];
  char* title;

  sprintf(name, "COLTITLE%d", col);
  title = iupAttribGet(ih, name);
  if (!title)
  {
    sprintf(name, "%d:0", col);
    title = iupAttribGet(ih, name);
  }
  return title;
}

static void eflTableSwapRows(Ihandle* ih, int lin1, int lin2)
{
  int col;
  int num_col = ih->data->num_col;

  for (col = 1; col <= num_col; col++)
  {
    char name1[50];
    char name2[50];
    char* val1;
    char* val2;
    char* temp;

    sprintf(name1, "CELLVALUE%d:%d", lin1, col);
    sprintf(name2, "CELLVALUE%d:%d", lin2, col);
    val1 = iupAttribGet(ih, name1);
    val2 = iupAttribGet(ih, name2);

    temp = val1 ? iupStrDup(val1) : NULL;

    if (val2)
      iupAttribSetStr(ih, name1, val2);
    else
      iupAttribSet(ih, name1, NULL);

    if (temp)
    {
      iupAttribSetStr(ih, name2, temp);
      free(temp);
    }
    else
      iupAttribSet(ih, name2, NULL);
  }
}

static void eflTableUpdateCellLabel(Ihandle* ih, int lin, int col)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int num_col = ih->data->num_col;
  int idx;
  char* text;
  Evas_Object* label;
  Evas_Object* cell_bg;
  int align;
  const char* align_tag;
  char markup[1024];
  unsigned char fg_r, fg_g, fg_b;
  unsigned char bg_r, bg_g, bg_b;

  if (!data || !data->cell_labels)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > num_col)
    return;

  idx = (lin - 1) * num_col + (col - 1);
  label = data->cell_labels[idx];

  if (!label)
    return;

  text = eflTableGetCellText(ih, lin, col);

  /* Get colors */
  eflTableGetCellFgColor(ih, lin, col, &fg_r, &fg_g, &fg_b);
  eflTableGetCellBgColor(ih, lin, col, &bg_r, &bg_g, &bg_b);

  /* Apply alignment via style and color via markup */
  align = eflTableGetColumnAlignment(ih, col);
  switch (align)
  {
    case 1: align_tag = "right"; break;
    case 0: align_tag = "center"; break;
    default: align_tag = "left"; break;
  }

  /* Build and push text style with alignment and font */
  {
    char style[512];
    char* font = eflTableGetCellFont(ih, lin, col);

    if (font && font[0])
    {
      char font_family[100] = "Sans";
      char font_style_str[50] = "";
      int font_size = 10;
      int is_bold = 0, is_italic = 0;
      const char* p;

      p = strchr(font, ',');
      if (p)
      {
        int len = (int)(p - font);
        if (len > 0 && len < 100)
        {
          strncpy(font_family, font, len);
          font_family[len] = 0;
        }
        p++;
        while (*p)
        {
          while (*p == ' ') p++;
          if (strncasecmp(p, "Bold", 4) == 0)
          {
            is_bold = 1;
            p += 4;
          }
          else if (strncasecmp(p, "Italic", 6) == 0)
          {
            is_italic = 1;
            p += 6;
          }
          else if (*p >= '0' && *p <= '9')
          {
            font_size = atoi(p);
            while (*p >= '0' && *p <= '9') p++;
          }
          else
            p++;
        }
      }
      else
      {
        if (sscanf(font, "%99s %d", font_family, &font_size) < 2)
          strcpy(font_family, font);
      }

      if (is_bold && is_italic)
        strcpy(font_style_str, ":style=Bold Italic");
      else if (is_bold)
        strcpy(font_style_str, ":style=Bold");
      else if (is_italic)
        strcpy(font_style_str, ":style=Italic");

      snprintf(style, sizeof(style), "DEFAULT='align=%s font=%s%s font_size=%d'",
               align_tag, font_family, font_style_str, font_size);
    }
    else
    {
      snprintf(style, sizeof(style), "DEFAULT='align=%s'", align_tag);
    }

    elm_entry_text_style_user_pop(label);
    elm_entry_text_style_user_push(label, style);
  }

  snprintf(markup, sizeof(markup), "<color=#%02X%02X%02X>%s</color>",
           fg_r, fg_g, fg_b, text ? text : "");
  elm_object_text_set(label, markup);

  /* Update cell background, preserve selection highlight */
  if (data->cell_bgs)
  {
    cell_bg = data->cell_bgs[idx];
    if (cell_bg)
    {
      if (lin == data->selected_lin)
        evas_object_color_set(cell_bg, data->sel_r, data->sel_g, data->sel_b, 200);
      else
        evas_object_color_set(cell_bg, bg_r, bg_g, bg_b, 255);
    }
  }
}

static void eflTableRefreshCells(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int lin, col;
  int num_lin = ih->data->num_lin;
  int num_col = ih->data->num_col;

  if (!data || !data->cell_labels)
    return;

  for (lin = 1; lin <= num_lin; lin++)
  {
    for (col = 1; col <= num_col; col++)
    {
      eflTableUpdateCellLabel(ih, lin, col);
    }
  }
}

static void eflTableSortRows(Ihandle* ih, int col, int ascending)
{
  int num_lin = ih->data->num_lin;
  int i, j;
  int swapped;

  if (num_lin <= 1)
    return;

  for (i = 0; i < num_lin - 1; i++)
  {
    swapped = 0;
    for (j = 0; j < num_lin - i - 1; j++)
    {
      char* val1 = eflTableGetCellText(ih, j + 1, col);
      char* val2 = eflTableGetCellText(ih, j + 2, col);
      int cmp = strcmp(val1 ? val1 : "", val2 ? val2 : "");
      int should_swap = ascending ? (cmp > 0) : (cmp < 0);

      if (should_swap)
      {
        eflTableSwapRows(ih, j + 1, j + 2);
        swapped = 1;
      }
    }
    if (!swapped)
      break;
  }

  eflTableRefreshCells(ih);
}

static void eflTableUpdateSortIndicators(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int col;
  int num_col = ih->data->num_col;

  if (!data || !data->header_labels)
    return;

  for (col = 0; col < num_col; col++)
  {
    Evas_Object* label = data->header_labels[col];
    char* original_title;
    char markup[512];

    if (!label)
      continue;

    original_title = eflTableGetHeaderText(ih, col + 1);

    if (col + 1 == data->sort_column)
    {
      const char* arrow = data->sort_ascending ? " \xe2\x96\xb2" : " \xe2\x96\xbc";
      snprintf(markup, sizeof(markup), "<b>%s%s</b>", original_title ? original_title : "", arrow);
      elm_object_text_set(label, markup);
    }
    else
    {
      snprintf(markup, sizeof(markup), "<b>%s</b>", original_title ? original_title : "");
      elm_object_text_set(label, markup);
    }
  }
}

static void eflTableRefreshHeaders(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int col;
  int num_col = ih->data->num_col;

  if (!data || !data->header_labels)
    return;

  for (col = 0; col < num_col; col++)
  {
    Evas_Object* label = data->header_labels[col];
    if (label)
    {
      char* title = eflTableGetHeaderText(ih, col + 1);
      char markup[512];
      snprintf(markup, sizeof(markup), "<b>%s</b>", title ? title : "");
      elm_object_text_set(label, markup);
    }
  }

  eflTableUpdateSortIndicators(ih);
}

static void eflTableDoSort(Ihandle* ih, int col)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  IFni cb;

  if (!ih->data->sortable)
    return;

  if (col == data->sort_column)
    data->sort_ascending = !data->sort_ascending;
  else
  {
    data->sort_column = col;
    data->sort_ascending = 1;
  }

  eflTableUpdateSortIndicators(ih);

  cb = (IFni)IupGetCallback(ih, "SORT_CB");
  if (cb)
  {
    int ret = cb(ih, col);
    if (ret == IUP_DEFAULT && !data->is_virtual)
      eflTableSortRows(ih, col, data->sort_ascending);
  }
  else if (!data->is_virtual)
  {
    eflTableSortRows(ih, col, data->sort_ascending);
  }
}

static int eflTableFindTargetColumn(Ihandle* ih, int x)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int num_col = ih->data->num_col;
  int col;

  if (!data || !data->header_labels)
    return -1;

  for (col = 0; col < num_col; col++)
  {
    Evas_Object* label = data->header_labels[col];
    if (label)
    {
      int lx, ly, lw, lh;
      int mid_x;
      evas_object_geometry_get(label, &lx, &ly, &lw, &lh);
      mid_x = lx + lw / 2;
      if (x < mid_x)
        return col + 1;
    }
  }
  return num_col;
}

static void eflTableSwapAttrib(Ihandle* ih, const char* name1, const char* name2)
{
  char* val1 = iupAttribGet(ih, name1);
  char* val2 = iupAttribGet(ih, name2);
  char* temp = val1 ? iupStrDup(val1) : NULL;

  if (val2)
    iupAttribSetStr(ih, name1, val2);
  else
    iupAttribSet(ih, name1, NULL);

  if (temp)
  {
    iupAttribSetStr(ih, name2, temp);
    free(temp);
  }
  else
    iupAttribSet(ih, name2, NULL);
}

static void eflTableSwapColumns(Ihandle* ih, int col1, int col2)
{
  int lin;
  int num_lin = ih->data->num_lin;
  char name1[50], name2[50];

  sprintf(name1, "COLTITLE%d", col1);
  sprintf(name2, "COLTITLE%d", col2);
  eflTableSwapAttrib(ih, name1, name2);

  sprintf(name1, "WIDTH%d", col1);
  sprintf(name2, "WIDTH%d", col2);
  eflTableSwapAttrib(ih, name1, name2);

  sprintf(name1, "RASTERWIDTH%d", col1);
  sprintf(name2, "RASTERWIDTH%d", col2);
  eflTableSwapAttrib(ih, name1, name2);

  sprintf(name1, "ALIGNMENT%d", col1);
  sprintf(name2, "ALIGNMENT%d", col2);
  eflTableSwapAttrib(ih, name1, name2);

  sprintf(name1, "0:%d", col1);
  sprintf(name2, "0:%d", col2);
  eflTableSwapAttrib(ih, name1, name2);

  sprintf(name1, "BGCOLOR0:%d", col1);
  sprintf(name2, "BGCOLOR0:%d", col2);
  eflTableSwapAttrib(ih, name1, name2);

  sprintf(name1, "FGCOLOR0:%d", col1);
  sprintf(name2, "FGCOLOR0:%d", col2);
  eflTableSwapAttrib(ih, name1, name2);

  sprintf(name1, "FONT0:%d", col1);
  sprintf(name2, "FONT0:%d", col2);
  eflTableSwapAttrib(ih, name1, name2);

  for (lin = 1; lin <= num_lin; lin++)
  {
    sprintf(name1, "CELLVALUE%d:%d", lin, col1);
    sprintf(name2, "CELLVALUE%d:%d", lin, col2);
    eflTableSwapAttrib(ih, name1, name2);

    sprintf(name1, "BGCOLOR%d:%d", lin, col1);
    sprintf(name2, "BGCOLOR%d:%d", lin, col2);
    eflTableSwapAttrib(ih, name1, name2);

    sprintf(name1, "FGCOLOR%d:%d", lin, col1);
    sprintf(name2, "FGCOLOR%d:%d", lin, col2);
    eflTableSwapAttrib(ih, name1, name2);

    sprintf(name1, "FONT%d:%d", lin, col1);
    sprintf(name2, "FONT%d:%d", lin, col2);
    eflTableSwapAttrib(ih, name1, name2);
  }
}

/* Visual indicator for column reordering */
static Evas_Object* efl_table_drag_indicator = NULL;

static void eflTableHideDragIndicator(Ihandle* ih)
{
  (void)ih;
  if (efl_table_drag_indicator)
    evas_object_hide(efl_table_drag_indicator);
}

static void eflTableUpdateDragIndicator(Ihandle* ih, int target)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int source = iupAttribGetInt(ih, "_IUPTABLE_DRAG_SOURCE");
  Evas_Object* label;
  int x, y, w, h;
  int indicator_x;

  if (!data || !data->header_labels || target < 1 || target > ih->data->num_col)
    return;

  if (target == source)
  {
    eflTableHideDragIndicator(ih);
    return;
  }

  label = data->header_labels[target - 1];
  if (!label)
    return;

  evas_object_geometry_get(label, &x, &y, &w, &h);

  if (!efl_table_drag_indicator)
  {
    Evas* evas = evas_object_evas_get(data->table);
    efl_table_drag_indicator = evas_object_rectangle_add(evas);
    evas_object_color_set(efl_table_drag_indicator, 0, 120, 215, 255);
  }

  indicator_x = (source < target) ? x + w - 1 : x;
  evas_object_geometry_set(efl_table_drag_indicator, indicator_x, y, 2, h);
  evas_object_show(efl_table_drag_indicator);
  evas_object_raise(efl_table_drag_indicator);
}

static void eflTableDragPointerMove(void* cb_data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)cb_data;
  Efl_Input_Pointer* pointer = ev->info;
  Eina_Position2D pointer_pos;
  int start_x, start_y;
  int target, old_target;

  if (!iupAttribGet(ih, "_IUPTABLE_DRAG_SOURCE"))
    return;

  pointer_pos = efl_input_pointer_position_get(pointer);

  if (!iupAttribGetInt(ih, "_IUPTABLE_DRAGGING"))
  {
    start_x = iupAttribGetInt(ih, "_IUPTABLE_DRAG_START_X");
    start_y = iupAttribGetInt(ih, "_IUPTABLE_DRAG_START_Y");
    if (abs(pointer_pos.x - start_x) > 5 || abs(pointer_pos.y - start_y) > 5)
      iupAttribSetInt(ih, "_IUPTABLE_DRAGGING", 1);
    else
      return;
  }

  target = eflTableFindTargetColumn(ih, pointer_pos.x);
  if (target >= 1)
  {
    old_target = iupAttribGetInt(ih, "_IUPTABLE_DRAG_TARGET");
    if (target != old_target)
    {
      iupAttribSetInt(ih, "_IUPTABLE_DRAG_TARGET", target);
      eflTableUpdateDragIndicator(ih, target);
    }
  }
}

static void eflTableDragPointerUp(void* cb_data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)cb_data;
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int is_dragging;
  int source, target;

  (void)ev;

  is_dragging = iupAttribGetInt(ih, "_IUPTABLE_DRAGGING");
  source = iupAttribGetInt(ih, "_IUPTABLE_DRAG_SOURCE");
  target = iupAttribGetInt(ih, "_IUPTABLE_DRAG_TARGET");

  iupAttribSet(ih, "_IUPTABLE_DRAGGING", NULL);
  iupAttribSet(ih, "_IUPTABLE_DRAG_SOURCE", NULL);
  iupAttribSet(ih, "_IUPTABLE_DRAG_TARGET", NULL);
  iupAttribSet(ih, "_IUPTABLE_DRAG_START_X", NULL);
  iupAttribSet(ih, "_IUPTABLE_DRAG_START_Y", NULL);

  eflTableHideDragIndicator(ih);

  if (is_dragging && source != target && source >= 1 && target >= 1)
  {
    eflTableSwapColumns(ih, source, target);

    if (data)
    {
      if (data->sort_column == source)
        data->sort_column = target;
      else if (data->sort_column == target)
        data->sort_column = source;

      if (data->is_virtual)
        eflTableUpdateVisibleRows(ih);
      else
        eflTableRefreshCells(ih);
    }

    eflTableRefreshHeaders(ih);
  }
  else if (!is_dragging && source >= 1)
  {
    eflTableDoSort(ih, source);
  }
}

static void eflTableResizeCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IeflTableData* table_data = IEFL_TABLE_DATA(ih);

  (void)ev;

  if (!iupObjectCheck(ih))
    return;

  /* Force table to fill viewport when viewport is wider than table's natural minimum */
  if (table_data && table_data->scroller && table_data->table)
  {
    Evas_Coord vp_w, vp_h, table_w, table_h;
    evas_object_geometry_get(table_data->scroller, NULL, NULL, &vp_w, &vp_h);
    evas_object_geometry_get(table_data->table, NULL, NULL, &table_w, &table_h);

    if (vp_w > table_w && vp_w > 0)
      evas_object_resize(table_data->table, vp_w, table_h);

  }

  /* Update focus rect position after layout */
  eflTableUpdateFocusRect(ih);
}

static void eflTableFocusRectJob(void* data)
{
  Ihandle* ih = (Ihandle*)data;

  iupAttribSet(ih, "_IUP_EFL_FOCUSRECT_JOB", NULL);

  if (!iupObjectCheck(ih))
    return;

  eflTableUpdateFocusRect(ih);
}

static void eflTableFocusInCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  IeflTableData* table_data = IEFL_TABLE_DATA(ih);

  (void)event_info;

  if (!iupObjectCheck(ih) || !table_data)
    return;

  table_data->has_focus = 1;
  eflTableUpdateFocusRect(ih);

  evas_object_focus_set(obj, EINA_TRUE);

  if (iupdrvIsActive(ih))
    iupCallGetFocusCb(ih);
}

static void eflTableFocusOutCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  IeflTableData* table_data = IEFL_TABLE_DATA(ih);

  (void)obj;
  (void)event_info;

  if (!iupObjectCheck(ih) || !table_data)
    return;

  if (table_data->editing_lin > 0)
    return;

  table_data->has_focus = 0;
  eflTableUpdateFocusRect(ih);

  iupCallKillFocusCb(ih);
}

/****************************************************************************
 * Editing Support
 ****************************************************************************/

static int eflTableIsCellEditable(Ihandle* ih, int col)
{
  char name[50];
  char* editable_str;

  sprintf(name, "EDITABLE%d", col);
  editable_str = iupAttribGet(ih, name);
  if (!editable_str)
    editable_str = iupAttribGet(ih, "EDITABLE");

  return iupStrBoolean(editable_str);
}

static void eflTableEndCellEdit(Ihandle* ih, int apply);

static void eflTableEditActivatedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  (void)obj;
  (void)event_info;

  if (!iupObjectCheck(ih))
    return;

  eflTableEndCellEdit(ih, 1);
}

static void eflTableEditAbortedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  (void)obj;
  (void)event_info;

  if (!iupObjectCheck(ih))
    return;

  eflTableEndCellEdit(ih, 0);
}

static void eflTableEditChangedCallback(void* data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)data;
  IeflTableData* table_data = IEFL_TABLE_DATA(ih);
  IFniis edition_cb;

  (void)event_info;

  if (!iupObjectCheck(ih) || !table_data)
    return;

  if (table_data->editing_lin == 0)
    return;

  edition_cb = (IFniis)IupGetCallback(ih, "EDITION_CB");
  if (edition_cb)
  {
    const char* markup = elm_entry_entry_get(obj);
    char* text = markup ? elm_entry_markup_to_utf8(markup) : NULL;
    edition_cb(ih, table_data->editing_lin, table_data->editing_col, text);
    if (text)
      free(text);
  }
}

static void eflTableStartCellEdit(Ihandle* ih, int lin, int col)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int num_col = ih->data->num_col;
  int idx;
  Evas_Object* entry;
  IFnii editbegin_cb;
  char* current_value;

  if (!data || data->is_virtual)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > num_col)
    return;

  if (!eflTableIsCellEditable(ih, col))
    return;

  if (data->editing_lin > 0)
    eflTableEndCellEdit(ih, 1);

  editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    int ret = editbegin_cb(ih, lin, col);
    if (ret == IUP_IGNORE)
      return;
  }

  idx = (lin - 1) * num_col + (col - 1);
  entry = data->cell_labels ? data->cell_labels[idx] : NULL;
  if (!entry)
    return;

  current_value = eflTableGetCellText(ih, lin, col);
  if (data->original_value)
    free(data->original_value);
  data->original_value = current_value ? strdup(current_value) : NULL;

  data->editing_lin = lin;
  data->editing_col = col;

  elm_entry_text_style_user_pop(entry);
  elm_object_text_set(entry, current_value ? current_value : "");

  elm_object_focus_allow_set(entry, EINA_TRUE);
  elm_entry_editable_set(entry, EINA_TRUE);
  elm_object_focus_set(entry, EINA_TRUE);
  elm_entry_select_all(entry);

  evas_object_smart_callback_add(entry, "activated", eflTableEditActivatedCallback, ih);
  evas_object_smart_callback_add(entry, "aborted", eflTableEditAbortedCallback, ih);
  evas_object_smart_callback_add(entry, "changed,user", eflTableEditChangedCallback, ih);
}

static void eflTableEndCellEdit(Ihandle* ih, int apply)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int num_col = ih->data->num_col;
  int idx;
  Evas_Object* entry;
  char* new_text = NULL;
  int lin, col;
  IFniisi editend_cb;
  IFnii valuechanged_cb;

  if (!data || data->editing_lin == 0)
    return;

  lin = data->editing_lin;
  col = data->editing_col;

  idx = (lin - 1) * num_col + (col - 1);
  entry = data->cell_labels ? data->cell_labels[idx] : NULL;
  if (!entry)
  {
    data->editing_lin = 0;
    data->editing_col = 0;
    if (data->original_value)
    {
      free(data->original_value);
      data->original_value = NULL;
    }
    return;
  }

  {
    const char* markup = elm_entry_entry_get(entry);
    new_text = markup ? elm_entry_markup_to_utf8(markup) : NULL;
  }

  editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb)
  {
    int ret = editend_cb(ih, lin, col, new_text, apply);
    if (ret == IUP_IGNORE && apply)
    {
      if (new_text)
        free(new_text);
      return;
    }
  }

  evas_object_smart_callback_del(entry, "activated", eflTableEditActivatedCallback);
  evas_object_smart_callback_del(entry, "aborted", eflTableEditAbortedCallback);
  evas_object_smart_callback_del(entry, "changed,user", eflTableEditChangedCallback);

  if (apply)
  {
    if (!data->original_value || strcmp(new_text ? new_text : "", data->original_value ? data->original_value : "") != 0)
    {
      iupdrvTableSetCellValue(ih, lin, col, new_text);

      valuechanged_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
      if (valuechanged_cb)
        valuechanged_cb(ih, lin, col);
    }
    else
    {
      eflTableUpdateCellLabel(ih, lin, col);
    }
  }
  else
  {
    eflTableUpdateCellLabel(ih, lin, col);
  }

  elm_entry_editable_set(entry, EINA_FALSE);
  elm_object_focus_allow_set(entry, EINA_FALSE);
  evas_object_focus_set(entry, EINA_FALSE);

  data->editing_lin = 0;
  data->editing_col = 0;

  if (new_text)
    free(new_text);

  if (data->original_value)
  {
    free(data->original_value);
    data->original_value = NULL;
  }

  if (data->scroller)
  {
    elm_object_focus_set(data->scroller, EINA_TRUE);
    evas_object_focus_set(data->scroller, EINA_TRUE);
  }
}

static void eflTableHeaderClickCallback(void* cb_data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)cb_data;
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  Efl_Input_Pointer* pointer = ev->info;
  int col = 0;
  int i;

  if (!data || !data->header_labels)
    return;

  for (i = 0; i < ih->data->num_col; i++)
  {
    if (data->header_labels[i] == ev->object)
    {
      col = i + 1;
      break;
    }
  }
  if (col == 0)
    return;

  /* If ALLOWREORDER is enabled, start drag tracking */
  if (iupAttribGetBoolean(ih, "ALLOWREORDER") && efl_input_pointer_button_get(pointer) == 1)
  {
    Eina_Position2D pointer_pos = efl_input_pointer_position_get(pointer);
    iupAttribSetInt(ih, "_IUPTABLE_DRAG_SOURCE", col);
    iupAttribSetInt(ih, "_IUPTABLE_DRAG_TARGET", col);
    iupAttribSetInt(ih, "_IUPTABLE_DRAG_START_X", pointer_pos.x);
    iupAttribSetInt(ih, "_IUPTABLE_DRAG_START_Y", pointer_pos.y);
    return;
  }

  /* Normal sorting behavior */
  eflTableDoSort(ih, col);
}

static void eflTableCellClickCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IeflTableData* table_data = IEFL_TABLE_DATA(ih);
  Efl_Input_Pointer* pointer = ev->info;
  IFniis cb;
  int lin = 0, col = 0;
  int num_col = ih->data->num_col;
  int max_lin;
  int i;
  int is_double_click = 0;

  if (!table_data || !table_data->cell_labels)
    return;

  if (efl_input_pointer_double_click_get(pointer))
    is_double_click = 1;

  max_lin = table_data->is_virtual ? table_data->alloc_num_lin : ih->data->num_lin;

  /* Find which cell was clicked */
  for (i = 0; i < max_lin * num_col; i++)
  {
    if (table_data->cell_labels[i] == ev->object)
    {
      int pool_row = (i / num_col) + 1;
      col = (i % num_col) + 1;
      if (table_data->is_virtual)
        lin = table_data->first_visible_row + pool_row - 1;
      else
        lin = pool_row;
      break;
    }
  }

  if (lin == 0)
    return;

  /* Update selection */
  table_data->selected_lin = lin;
  table_data->selected_col = col;

  /* Update row highlight for all cells in each row */
  if (table_data->cell_bgs)
  {
    int row, c;
    for (row = 0; row < max_lin; row++)
    {
      for (c = 0; c < num_col; c++)
      {
        int idx = row * num_col + c;
        Evas_Object* cell_bg = table_data->cell_bgs[idx];
        if (cell_bg)
        {
          int actual_row = table_data->is_virtual ? (table_data->first_visible_row - 1 + row) : row;
          if (actual_row == lin - 1)
          {
            evas_object_color_set(cell_bg, table_data->sel_r, table_data->sel_g, table_data->sel_b, 200);
          }
          else
          {
            unsigned char r, g, b;
            eflTableGetCellBgColor(ih, actual_row + 1, c + 1, &r, &g, &b);
            evas_object_color_set(cell_bg, r, g, b, 255);
          }
        }
      }
    }
  }

  /* Update focus rectangle */
  eflTableUpdateFocusRect(ih);

  /* Ensure scroller has focus for keyboard navigation */
  if (table_data->scroller)
  {
    elm_object_focus_set(table_data->scroller, EINA_TRUE);
    evas_object_focus_set(table_data->scroller, EINA_TRUE);
  }

  /* Handle double-click: start editing if cell is editable */
  if (is_double_click && eflTableIsCellEditable(ih, col))
  {
    eflTableStartCellEdit(ih, lin, col);
    return;
  }

  /* Fire callback */
  cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
  if (cb)
    cb(ih, lin, col, "l");
}

static Evas_Object* eflTableCreateCellWidget(Ihandle* ih, Evas_Object* parent, const char* text, int is_header, int lin, int col)
{
  Evas_Object* entry;
  int align;
  const char* align_tag;
  char markup[1024];
  unsigned char fg_r, fg_g, fg_b;

  entry = elm_entry_add(parent);
  elm_entry_single_line_set(entry, EINA_TRUE);
  elm_entry_editable_set(entry, EINA_FALSE);
  elm_entry_scrollable_set(entry, EINA_FALSE);
  elm_entry_context_menu_disabled_set(entry, EINA_TRUE);
  elm_object_focus_allow_set(entry, EINA_FALSE);

  /* Remove the xterm/I-beam cursor that entry sets by default */
  {
    Evas_Object* edje = elm_layout_edje_get(entry);
    if (edje)
      elm_object_cursor_unset(edje);
  }

  /* Get column alignment */
  align = eflTableGetColumnAlignment(ih, col);
  switch (align)
  {
    case 1: align_tag = "right"; break;
    case 0: align_tag = "center"; break;
    default: align_tag = "left"; break;
  }

  /* Get foreground color */
  eflTableGetCellFgColor(ih, lin, col, &fg_r, &fg_g, &fg_b);

  /* Build and push text style with alignment and font */
  {
    char style[512];
    char* font = eflTableGetCellFont(ih, lin, col);

    if (font && font[0])
    {
      char font_family[100] = "Sans";
      char font_style_str[50] = "";
      int font_size = 10;
      int is_bold = 0, is_italic = 0;
      const char* p;

      /* Parse IUP font format using same logic as eflFontParse */
      p = strchr(font, ',');
      if (p)
      {
        int len = (int)(p - font);
        if (len > 0 && len < 100)
        {
          strncpy(font_family, font, len);
          font_family[len] = 0;
        }
        p++;
        while (*p)
        {
          while (*p == ' ') p++;
          if (strncasecmp(p, "Bold", 4) == 0)
          {
            is_bold = 1;
            p += 4;
          }
          else if (strncasecmp(p, "Italic", 6) == 0)
          {
            is_italic = 1;
            p += 6;
          }
          else if (*p >= '0' && *p <= '9')
          {
            font_size = atoi(p);
            while (*p >= '0' && *p <= '9') p++;
          }
          else
            p++;
        }
      }
      else
      {
        /* Simple format like "Monospace 10" */
        if (sscanf(font, "%99s %d", font_family, &font_size) < 2)
          strcpy(font_family, font);
      }

      if (is_bold && is_italic)
        strcpy(font_style_str, ":style=Bold Italic");
      else if (is_bold)
        strcpy(font_style_str, ":style=Bold");
      else if (is_italic)
        strcpy(font_style_str, ":style=Italic");

      snprintf(style, sizeof(style), "DEFAULT='align=%s font=%s%s font_size=%d'",
               align_tag, font_family, font_style_str, font_size);
    }
    else
    {
      snprintf(style, sizeof(style), "DEFAULT='align=%s'", align_tag);
    }

    elm_entry_text_style_user_push(entry, style);
  }

  if (is_header)
  {
    snprintf(markup, sizeof(markup), "<color=#%02X%02X%02X><b>%s</b></color>",
             fg_r, fg_g, fg_b, text ? text : "");
  }
  else
  {
    snprintf(markup, sizeof(markup), "<color=#%02X%02X%02X>%s</color>",
             fg_r, fg_g, fg_b, text ? text : "");
  }
  elm_object_text_set(entry, markup);

  evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_show(entry);

  return entry;
}

static void eflTableClearCells(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int alloc_col, alloc_lin;
  int i;

  if (!data)
    return;

  /* Use allocated sizes, not current ih->data values which may have changed */
  alloc_col = data->alloc_num_col;
  alloc_lin = data->alloc_num_lin;

  /* Clear header labels */
  if (data->header_labels)
  {
    for (i = 0; i < alloc_col; i++)
    {
      if (data->header_labels[i])
      {
        evas_object_del(data->header_labels[i]);
        data->header_labels[i] = NULL;
      }
    }
    free(data->header_labels);
    data->header_labels = NULL;
  }

  /* Clear cell labels */
  if (data->cell_labels)
  {
    for (i = 0; i < alloc_lin * alloc_col; i++)
    {
      if (data->cell_labels[i])
      {
        evas_object_del(data->cell_labels[i]);
        data->cell_labels[i] = NULL;
      }
    }
    free(data->cell_labels);
    data->cell_labels = NULL;
  }

  /* Clear cell backgrounds */
  if (data->cell_bgs)
  {
    for (i = 0; i < alloc_lin * alloc_col; i++)
    {
      if (data->cell_bgs[i])
      {
        evas_object_del(data->cell_bgs[i]);
        data->cell_bgs[i] = NULL;
      }
    }
    free(data->cell_bgs);
    data->cell_bgs = NULL;
  }

  data->alloc_num_col = 0;
  data->alloc_num_lin = 0;
}

static void eflTableRebuildCells(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int num_col = ih->data->num_col;
  int num_lin = ih->data->num_lin;
  int col, lin;
  Evas_Object* label;
  Evas_Object* bg;
  char* text;
  int col_width;

  if (!data || !data->table)
    return;

  /* Clear existing cells */
  eflTableClearCells(ih);

  if (num_col <= 0)
    return;

  /* Allocate column widths array if needed */
  if (!data->col_widths)
  {
    data->col_widths = (int*)calloc(num_col, sizeof(int));
    for (col = 0; col < num_col; col++)
    {
      char name[50];
      char* width_str;
      int width = DEFAULT_COL_WIDTH;

      /* Check RASTERWIDTH first, then WIDTH */
      sprintf(name, "RASTERWIDTH%d", col + 1);
      width_str = iupAttribGet(ih, name);
      if (!width_str)
      {
        sprintf(name, "WIDTH%d", col + 1);
        width_str = iupAttribGet(ih, name);
      }

      if (width_str && iupStrToInt(width_str, &width) && width > 0)
        data->col_widths[col] = width;
      else
        data->col_widths[col] = DEFAULT_COL_WIDTH;
    }
  }

  /* Allocate header labels */
  data->header_labels = (Evas_Object**)calloc(num_col, sizeof(Evas_Object*));
  data->alloc_num_col = num_col;

  /* Create header row (row 0 in table) */
  for (col = 0; col < num_col; col++)
  {
    char name[50];
    unsigned char bg_r, bg_g, bg_b;
    int is_last_col = (col == num_col - 1);
    int should_stretch = is_last_col && ih->data->stretch_last && !eflTableColHasExplicitWidth(ih, col + 1);

    sprintf(name, "COLTITLE%d", col + 1);
    text = iupAttribGet(ih, name);
    if (!text)
    {
      sprintf(name, "Col %d", col + 1);
      text = name;
    }

    col_width = data->col_widths[col];

    /* Create header background */
    bg = evas_object_rectangle_add(evas_object_evas_get(data->table));
    eflTableGetCellBgColor(ih, 0, col + 1, &bg_r, &bg_g, &bg_b);
    evas_object_color_set(bg, bg_r, bg_g, bg_b, 255);
    evas_object_size_hint_min_set(bg, col_width, data->header_height);
    evas_object_size_hint_weight_set(bg, should_stretch ? EVAS_HINT_EXPAND : 0.0, 0.0);
    evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(bg);
    elm_table_pack(data->table, bg, col, 0, 1, 1);

    /* Create header label on top */
    label = eflTableCreateCellWidget(ih, data->table, text, 1, 0, col + 1);
    evas_object_size_hint_min_set(label, col_width, data->header_height);
    evas_object_size_hint_weight_set(label, should_stretch ? EVAS_HINT_EXPAND : 0.0, 0.0);
    elm_table_pack(data->table, label, col, 0, 1, 1);
    data->header_labels[col] = label;
    efl_event_callback_add(label, EFL_EVENT_POINTER_DOWN, eflTableHeaderClickCallback, ih);
    efl_event_callback_add(label, EFL_EVENT_POINTER_MOVE, eflTableDragPointerMove, ih);
    efl_event_callback_add(label, EFL_EVENT_POINTER_UP, eflTableDragPointerUp, ih);
  }

  if (num_lin <= 0)
    return;

  /* Allocate cell labels and cell backgrounds */
  data->cell_labels = (Evas_Object**)calloc(num_lin * num_col, sizeof(Evas_Object*));
  data->cell_bgs = (Evas_Object**)calloc(num_lin * num_col, sizeof(Evas_Object*));
  data->alloc_num_lin = num_lin;

  /* Determine if last column should stretch (only compute once) */
  {
    int should_stretch_last = ih->data->stretch_last && !eflTableColHasExplicitWidth(ih, num_col);

    /* Create data rows (rows 1+ in table) */
    for (lin = 1; lin <= num_lin; lin++)
    {
      /* Create cell backgrounds and labels for this row */
      for (col = 1; col <= num_col; col++)
      {
        int idx = (lin - 1) * num_col + (col - 1);
        unsigned char bg_r, bg_g, bg_b;
        int is_last_col = (col == num_col);
        double weight_x = (is_last_col && should_stretch_last) ? EVAS_HINT_EXPAND : 0.0;

        col_width = data->col_widths[col - 1];

        /* Create cell background - query with proper (lin, col) for full hierarchy support */
        bg = evas_object_rectangle_add(evas_object_evas_get(data->table));
        eflTableGetCellBgColor(ih, lin, col, &bg_r, &bg_g, &bg_b);
        evas_object_color_set(bg, bg_r, bg_g, bg_b, 255);

        evas_object_size_hint_min_set(bg, col_width, data->row_height);
        evas_object_size_hint_weight_set(bg, weight_x, 0.0);
        evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_show(bg);

        /* Pack background first (lower z-order) */
        elm_table_pack(data->table, bg, col - 1, lin, 1, 1);
        data->cell_bgs[idx] = bg;

        /* Create cell label on top of background */
        text = eflTableGetCellText(ih, lin, col);
        label = eflTableCreateCellWidget(ih, data->table, text, 0, lin, col);
        evas_object_size_hint_min_set(label, col_width, data->row_height);
        evas_object_size_hint_weight_set(label, weight_x, 0.0);

        /* Add click handler */
        efl_event_callback_add(label, EFL_EVENT_POINTER_DOWN, eflTableCellClickCallback, ih);

        elm_table_pack(data->table, label, col - 1, lin, 1, 1);
        data->cell_labels[idx] = label;
      }
    }
  }

  /* Add dummy expanding column to absorb extra space when:
   * - STRETCHLAST=NO (last column shouldn't expand), OR
   * - Last column has explicit width (shouldn't stretch even if STRETCHLAST=YES) */
  {
    int need_dummy = !ih->data->stretch_last || eflTableColHasExplicitWidth(ih, num_col);
    if (need_dummy)
    {
      Evas_Object* dummy;
      unsigned char bg_r, bg_g, bg_b;

      /* Add dummy to header row */
      dummy = evas_object_rectangle_add(evas_object_evas_get(data->table));
      eflTableGetCellBgColor(ih, 0, num_col + 1, &bg_r, &bg_g, &bg_b);
      evas_object_color_set(dummy, bg_r, bg_g, bg_b, 255);
      evas_object_size_hint_min_set(dummy, 1, data->header_height);
      evas_object_size_hint_weight_set(dummy, EVAS_HINT_EXPAND, 0.0);
      evas_object_size_hint_align_set(dummy, EVAS_HINT_FILL, EVAS_HINT_FILL);
      evas_object_show(dummy);
      elm_table_pack(data->table, dummy, num_col, 0, 1, 1);

      /* Add dummy to each data row */
      for (lin = 1; lin <= num_lin; lin++)
      {
        dummy = evas_object_rectangle_add(evas_object_evas_get(data->table));
        eflTableGetCellBgColor(ih, lin, num_col + 1, &bg_r, &bg_g, &bg_b);
        evas_object_color_set(dummy, bg_r, bg_g, bg_b, 255);
        evas_object_size_hint_min_set(dummy, 1, data->row_height);
        evas_object_size_hint_weight_set(dummy, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(dummy, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_show(dummy);
        elm_table_pack(data->table, dummy, num_col, lin, 1, 1);
      }
    }
  }
}

/****************************************************************************
 * Virtual Mode
 ****************************************************************************/

static void eflTableRebuildHeaders(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int num_col = ih->data->num_col;
  int col;
  Evas_Object* label;
  Evas_Object* bg;
  char* text;
  int col_width;

  if (!data || !data->table || num_col <= 0)
    return;

  /* Allocate column widths array if needed */
  if (!data->col_widths)
  {
    data->col_widths = (int*)calloc(num_col, sizeof(int));
    data->alloc_num_col = num_col;
    for (col = 0; col < num_col; col++)
    {
      char name[50];
      char* width_str;
      int width = DEFAULT_COL_WIDTH;

      sprintf(name, "RASTERWIDTH%d", col + 1);
      width_str = iupAttribGet(ih, name);
      if (!width_str)
      {
        sprintf(name, "WIDTH%d", col + 1);
        width_str = iupAttribGet(ih, name);
      }

      if (width_str && iupStrToInt(width_str, &width) && width > 0)
        data->col_widths[col] = width;
      else
        data->col_widths[col] = DEFAULT_COL_WIDTH;
    }
  }

  /* Allocate header labels */
  if (!data->header_labels)
    data->header_labels = (Evas_Object**)calloc(num_col, sizeof(Evas_Object*));

  /* Create header row (row 0 in table) */
  for (col = 0; col < num_col; col++)
  {
    char name[50];
    unsigned char bg_r, bg_g, bg_b;
    int is_last_col = (col == num_col - 1);
    int should_stretch = is_last_col && ih->data->stretch_last && !eflTableColHasExplicitWidth(ih, col + 1);

    sprintf(name, "COLTITLE%d", col + 1);
    text = iupAttribGet(ih, name);
    if (!text)
    {
      sprintf(name, "Col %d", col + 1);
      text = name;
    }

    col_width = data->col_widths[col];

    /* Create header background */
    bg = evas_object_rectangle_add(evas_object_evas_get(data->table));
    eflTableGetCellBgColor(ih, 0, col + 1, &bg_r, &bg_g, &bg_b);
    evas_object_color_set(bg, bg_r, bg_g, bg_b, 255);
    evas_object_size_hint_min_set(bg, col_width, data->header_height);
    evas_object_size_hint_weight_set(bg, should_stretch ? EVAS_HINT_EXPAND : 0.0, 0.0);
    evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(bg);
    elm_table_pack(data->table, bg, col, 0, 1, 1);

    /* Create header label on top */
    label = eflTableCreateCellWidget(ih, data->table, text, 1, 0, col + 1);
    evas_object_size_hint_min_set(label, col_width, data->header_height);
    evas_object_size_hint_weight_set(label, should_stretch ? EVAS_HINT_EXPAND : 0.0, 0.0);
    elm_table_pack(data->table, label, col, 0, 1, 1);
    data->header_labels[col] = label;
    efl_event_callback_add(label, EFL_EVENT_POINTER_DOWN, eflTableHeaderClickCallback, ih);
    efl_event_callback_add(label, EFL_EVENT_POINTER_MOVE, eflTableDragPointerMove, ih);
    efl_event_callback_add(label, EFL_EVENT_POINTER_UP, eflTableDragPointerUp, ih);
  }
}

static void eflTableScrollJobCallback(void* data)
{
  Ihandle* ih = (Ihandle*)data;
  IeflTableData* table_data;

  if (!ih || !iupObjectCheck(ih))
    return;

  table_data = IEFL_TABLE_DATA(ih);
  if (table_data)
    table_data->scroll_job = NULL;

  eflTableUpdateVisibleRows(ih);
}

static void eflTableVirtualScrollCallback(void* cb_data, Evas_Object* obj, void* event_info)
{
  Ihandle* ih = (Ihandle*)cb_data;
  IeflTableData* data;

  (void)obj;
  (void)event_info;

  if (!ih || !iupObjectCheck(ih))
    return;

  data = IEFL_TABLE_DATA(ih);
  if (!data)
    return;

  /* schedule job if not already pending */
  if (!data->scroll_job)
    data->scroll_job = ecore_job_add(eflTableScrollJobCallback, ih);
}


static void eflTableRebuildVirtualCells(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  Evas_Object* table;
  int num_col = ih->data->num_col;
  int num_lin = ih->data->num_lin;
  int visible_rows;
  int row_height;
  int lin, col;
  int col_width;
  char* text;
  Evas_Object* bg;
  Evas_Object* label;
  sIFnii value_cb;

  if (!data || !data->table)
    return;

  table = data->table;

  if (num_col <= 0)
    return;

  row_height = data->row_height > 0 ? data->row_height : DEFAULT_ROW_HEIGHT;

  /* Determine visible rows, use VISIBLELINES, or calculate based on screen height */
  visible_rows = iupAttribGetInt(ih, "VISIBLELINES");
  if (visible_rows <= 0)
  {
    int screen_width = 0, screen_height = 0;
    iupdrvGetScreenSize(&screen_width, &screen_height);
    if (screen_height > 0)
      visible_rows = screen_height / row_height + 5;
    else
      visible_rows = 50;
  }

  if (visible_rows > num_lin)
    visible_rows = num_lin;

  /* Allocate column widths if needed */
  if (!data->col_widths)
  {
    data->col_widths = (int*)calloc(num_col, sizeof(int));
    for (col = 0; col < num_col; col++)
    {
      char name[50];
      char* width_str;
      int width = DEFAULT_COL_WIDTH;

      sprintf(name, "RASTERWIDTH%d", col + 1);
      width_str = iupAttribGet(ih, name);
      if (!width_str)
      {
        sprintf(name, "WIDTH%d", col + 1);
        width_str = iupAttribGet(ih, name);
      }

      if (width_str && iupStrToInt(width_str, &width) && width > 0)
        data->col_widths[col] = width;
      else
        data->col_widths[col] = DEFAULT_COL_WIDTH;
    }
  }

  /* Create headers at row 0 */
  data->header_labels = (Evas_Object**)calloc(num_col, sizeof(Evas_Object*));
  data->alloc_num_col = num_col;

  for (col = 0; col < num_col; col++)
  {
    char name[50];
    unsigned char bg_r, bg_g, bg_b;
    int is_last_col = (col == num_col - 1);
    int should_stretch = is_last_col && ih->data->stretch_last && !eflTableColHasExplicitWidth(ih, col + 1);

    sprintf(name, "COLTITLE%d", col + 1);
    text = iupAttribGet(ih, name);
    if (!text)
    {
      sprintf(name, "Col %d", col + 1);
      text = name;
    }

    col_width = data->col_widths[col];

    bg = evas_object_rectangle_add(evas_object_evas_get(table));
    eflTableGetCellBgColor(ih, 0, col + 1, &bg_r, &bg_g, &bg_b);
    evas_object_color_set(bg, bg_r, bg_g, bg_b, 255);
    evas_object_size_hint_min_set(bg, col_width, data->header_height);
    evas_object_size_hint_weight_set(bg, should_stretch ? EVAS_HINT_EXPAND : 0.0, 0.0);
    evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(bg);
    elm_table_pack(table, bg, col, 0, 1, 1);

    label = eflTableCreateCellWidget(ih, table, text, 1, 0, col + 1);
    evas_object_size_hint_min_set(label, col_width, data->header_height);
    evas_object_size_hint_weight_set(label, should_stretch ? EVAS_HINT_EXPAND : 0.0, 0.0);
    elm_table_pack(table, label, col, 0, 1, 1);
    data->header_labels[col] = label;
    efl_event_callback_add(label, EFL_EVENT_POINTER_DOWN, eflTableHeaderClickCallback, ih);
    efl_event_callback_add(label, EFL_EVENT_POINTER_MOVE, eflTableDragPointerMove, ih);
    efl_event_callback_add(label, EFL_EVENT_POINTER_UP, eflTableDragPointerUp, ih);
  }

  if (num_lin <= 0)
    return;

  /* Allocate cell arrays for visible rows only */
  data->cell_labels = (Evas_Object**)calloc(visible_rows * num_col, sizeof(Evas_Object*));
  data->cell_bgs = (Evas_Object**)calloc(visible_rows * num_col, sizeof(Evas_Object*));
  data->alloc_num_lin = visible_rows;

  value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");

  /* Create top spacer at row 1 (between headers and cells) - initially 0 height */
  {
    Evas_Object* spacer = evas_object_rectangle_add(evas_object_evas_get(table));
    evas_object_color_set(spacer, 0, 0, 0, 0);
    evas_object_size_hint_min_set(spacer, 1, 0);
    evas_object_show(spacer);
    elm_table_pack(table, spacer, 0, 1, num_col, 1);
    data->top_spacer = spacer;
  }

  /* Create visible data rows at rows 2 to visible_rows+1 */
  {
    int should_stretch_last = ih->data->stretch_last && !eflTableColHasExplicitWidth(ih, num_col);

    for (lin = 1; lin <= visible_rows; lin++)
    {
      int table_row = lin + 1;  /* Offset by 1 for top_spacer at row 1 */
      for (col = 1; col <= num_col; col++)
      {
        int idx = (lin - 1) * num_col + (col - 1);
        unsigned char bg_r, bg_g, bg_b;
        int is_last_col = (col == num_col);
        double weight_x = (is_last_col && should_stretch_last) ? EVAS_HINT_EXPAND : 0.0;

        col_width = data->col_widths[col - 1];

        bg = evas_object_rectangle_add(evas_object_evas_get(table));
        eflTableGetCellBgColor(ih, lin, col, &bg_r, &bg_g, &bg_b);
        evas_object_color_set(bg, bg_r, bg_g, bg_b, 255);
        evas_object_size_hint_min_set(bg, col_width, row_height);
        evas_object_size_hint_weight_set(bg, weight_x, 0.0);
        evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_show(bg);
        elm_table_pack(table, bg, col - 1, table_row, 1, 1);
        data->cell_bgs[idx] = bg;

        /* Get text from VALUE_CB */
        text = NULL;
        if (value_cb)
          text = value_cb(ih, lin, col);

        label = eflTableCreateCellWidget(ih, table, text, 0, lin, col);
        evas_object_size_hint_min_set(label, col_width, row_height);
        evas_object_size_hint_weight_set(label, weight_x, 0.0);
        efl_event_callback_add(label, EFL_EVENT_POINTER_DOWN, eflTableCellClickCallback, ih);
        elm_table_pack(table, label, col - 1, table_row, 1, 1);
        data->cell_labels[idx] = label;
      }
    }
  }

  /* Create bottom spacer for remaining virtual rows */
  if (num_lin > visible_rows)
  {
    int spacer_height = (num_lin - visible_rows) * row_height;
    Evas_Object* spacer = evas_object_rectangle_add(evas_object_evas_get(table));
    evas_object_color_set(spacer, 0, 0, 0, 0);
    evas_object_size_hint_min_set(spacer, 1, spacer_height);
    evas_object_show(spacer);
    elm_table_pack(table, spacer, 0, visible_rows + 2, num_col, 1);
    data->bottom_spacer = spacer;
  }

  /* Add dummy expanding column to absorb extra space when:
   * - STRETCHLAST=NO (last column shouldn't expand), OR
   * - Last column has explicit width (shouldn't stretch even if STRETCHLAST=YES) */
  {
    int need_dummy = !ih->data->stretch_last || eflTableColHasExplicitWidth(ih, num_col);
    if (need_dummy)
    {
      Evas_Object* dummy;
      unsigned char bg_r, bg_g, bg_b;

      /* Add dummy to header row */
      dummy = evas_object_rectangle_add(evas_object_evas_get(table));
      eflTableGetCellBgColor(ih, 0, num_col + 1, &bg_r, &bg_g, &bg_b);
      evas_object_color_set(dummy, bg_r, bg_g, bg_b, 255);
      evas_object_size_hint_min_set(dummy, 1, data->header_height);
      evas_object_size_hint_weight_set(dummy, EVAS_HINT_EXPAND, 0.0);
      evas_object_size_hint_align_set(dummy, EVAS_HINT_FILL, EVAS_HINT_FILL);
      evas_object_show(dummy);
      elm_table_pack(table, dummy, num_col, 0, 1, 1);

      /* Add dummy to top spacer row */
      dummy = evas_object_rectangle_add(evas_object_evas_get(table));
      evas_object_color_set(dummy, bg_r, bg_g, bg_b, 255);
      evas_object_size_hint_min_set(dummy, 1, 1);
      evas_object_size_hint_weight_set(dummy, EVAS_HINT_EXPAND, 0.0);
      evas_object_size_hint_align_set(dummy, EVAS_HINT_FILL, EVAS_HINT_FILL);
      evas_object_show(dummy);
      elm_table_pack(table, dummy, num_col, 1, 1, 1);

      /* Add dummy to each visible data row */
      for (lin = 1; lin <= visible_rows; lin++)
      {
        int table_row = lin + 1;
        dummy = evas_object_rectangle_add(evas_object_evas_get(table));
        eflTableGetCellBgColor(ih, lin, num_col + 1, &bg_r, &bg_g, &bg_b);
        evas_object_color_set(dummy, bg_r, bg_g, bg_b, 255);
        evas_object_size_hint_min_set(dummy, 1, row_height);
        evas_object_size_hint_weight_set(dummy, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(dummy, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_show(dummy);
        elm_table_pack(table, dummy, num_col, table_row, 1, 1);
      }

      /* Add dummy to bottom spacer row if exists */
      if (num_lin > visible_rows)
      {
        dummy = evas_object_rectangle_add(evas_object_evas_get(table));
        evas_object_color_set(dummy, bg_r, bg_g, bg_b, 255);
        evas_object_size_hint_min_set(dummy, 1, 1);
        evas_object_size_hint_weight_set(dummy, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(dummy, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_show(dummy);
        elm_table_pack(table, dummy, num_col, visible_rows + 2, 1, 1);
      }
    }
  }

  data->first_visible_row = 1;
  data->visible_row_count = visible_rows;
}

static void eflTableUpdateVisibleRows(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int num_col = ih->data->num_col;
  int num_lin = ih->data->num_lin;
  int first_data_row;
  int pool_row, col;
  sIFnii value_cb;
  int align;
  const char* align_tag;
  char markup[1024];
  unsigned char fg_r, fg_g, fg_b;
  int row_height;
  int scroll_y = 0;

  if (!data || !data->is_virtual)
    return;

  if (num_col <= 0 || num_lin <= 0)
    return;

  if (!data->cell_labels || !data->cell_bgs)
    return;

  value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
  row_height = data->row_height > 0 ? data->row_height : DEFAULT_ROW_HEIGHT;

  /* Get first data row from scroll position */
  if (data->scroller)
  {
    int scroll_x;
    elm_scroller_region_get(data->scroller, &scroll_x, &scroll_y, NULL, NULL);
    first_data_row = (scroll_y / row_height) + 1;
  }
  else
    first_data_row = 1;

  if (first_data_row < 1)
    first_data_row = 1;
  if (first_data_row > num_lin - data->alloc_num_lin + 1)
    first_data_row = num_lin - data->alloc_num_lin + 1;
  if (first_data_row < 1)
    first_data_row = 1;

  /* Skip expensive cell updates if first visible row hasn't changed */
  if (first_data_row == data->first_visible_row)
    return;

  /* Update spacer heights to keep cells in visible viewport */
  if (data->top_spacer)
  {
    int top_height = (first_data_row - 1) * row_height;
    evas_object_size_hint_min_set(data->top_spacer, 1, top_height);
  }
  if (data->bottom_spacer)
  {
    int bottom_height = (num_lin - first_data_row - data->alloc_num_lin + 1) * row_height;
    if (bottom_height < 0)
      bottom_height = 0;
    evas_object_size_hint_min_set(data->bottom_spacer, 1, bottom_height);
  }

  /* Update each visible cell content based on which data row it represents */
  for (pool_row = 1; pool_row <= data->alloc_num_lin; pool_row++)
  {
    int data_row = first_data_row + pool_row - 1;
    if (data_row > num_lin)
      break;

    for (col = 1; col <= num_col; col++)
    {
      int idx = (pool_row - 1) * num_col + (col - 1);
      Evas_Object* entry = data->cell_labels[idx];
      Evas_Object* bg = data->cell_bgs[idx];
      char* text = NULL;
      unsigned char bg_r, bg_g, bg_b;

      if (!entry)
        continue;

      /* Get cell value from VALUE_CB callback with correct data row */
      if (value_cb)
        text = value_cb(ih, data_row, col);

      /* Update cell text with proper formatting */
      align = eflTableGetColumnAlignment(ih, col);
      switch (align)
      {
        case 1: align_tag = "right"; break;
        case 0: align_tag = "center"; break;
        default: align_tag = "left"; break;
      }
      eflTableGetCellFgColor(ih, data_row, col, &fg_r, &fg_g, &fg_b);
      snprintf(markup, sizeof(markup),
               "<align=%s><color=#%02X%02X%02X>%s</color></align>",
               align_tag, fg_r, fg_g, fg_b, text ? text : "");
      elm_entry_entry_set(entry, markup);

      /* Update background color */
      if (bg)
      {
        eflTableGetCellBgColor(ih, data_row, col, &bg_r, &bg_g, &bg_b);
        if (data_row == data->selected_lin)
          evas_object_color_set(bg, data->sel_r, data->sel_g, data->sel_b, 200);
        else
          evas_object_color_set(bg, bg_r, bg_g, bg_b, 255);
      }
    }
  }

  data->first_visible_row = first_data_row;
}

/****************************************************************************
 * Driver Interface
 ****************************************************************************/

void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int old_num_col = ih->data->num_col;

  if (num_col < 0)
    num_col = 0;

  ih->data->num_col = num_col;

  /* Reallocate column widths if needed */
  if (data && num_col != old_num_col)
  {
    if (data->col_widths)
    {
      free(data->col_widths);
      data->col_widths = NULL;
    }

    if (num_col > 0)
    {
      int col;
      data->col_widths = (int*)calloc(num_col, sizeof(int));
      for (col = 0; col < num_col; col++)
        data->col_widths[col] = DEFAULT_COL_WIDTH;
    }

    if (ih->handle)
      eflTableRebuildCells(ih);
  }
}

void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int old_num_lin = ih->data->num_lin;
  int i;

  if (num_lin < 0)
    num_lin = 0;

  ih->data->num_lin = num_lin;

  (void)i;

  if (!ih->handle || num_lin == old_num_lin)
    return;

  if (data && data->is_virtual)
  {
    /* Virtual mode: update visible rows */
    eflTableUpdateVisibleRows(ih);
  }
  else
  {
    eflTableRebuildCells(ih);
  }
}

void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  (void)pos;
  iupdrvTableSetNumCol(ih, ih->data->num_col + 1);
}

void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  (void)pos;
  if (ih->data->num_col > 0)
    iupdrvTableSetNumCol(ih, ih->data->num_col - 1);
}

void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  (void)pos;
  iupdrvTableSetNumLin(ih, ih->data->num_lin + 1);
}

void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  (void)pos;
  if (ih->data->num_lin > 0)
    iupdrvTableSetNumLin(ih, ih->data->num_lin - 1);
}

void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  char name[50];
  sprintf(name, "CELLVALUE%d:%d", lin, col);
  iupAttribSetStr(ih, name, value);

  if (ih->handle)
    eflTableUpdateCellLabel(ih, lin, col);
}

char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  return eflTableGetCellText(ih, lin, col);
}

void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  char name[50];

  sprintf(name, "COLTITLE%d", col);
  iupAttribSetStr(ih, name, title);

  if (ih->handle && data && data->header_labels)
  {
    if (col >= 1 && col <= ih->data->num_col)
    {
      Evas_Object* label = data->header_labels[col - 1];
      if (label)
      {
        char markup[512];
        snprintf(markup, sizeof(markup), "<b>%s</b>", title ? title : "");
        elm_object_text_set(label, markup);
      }
    }
  }
}

char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  char name[50];
  sprintf(name, "COLTITLE%d", col);
  return iupAttribGet(ih, name);
}

void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int lin;

  if (!data || !data->col_widths)
    return;

  if (col < 1 || col > ih->data->num_col)
    return;

  data->col_widths[col - 1] = width;

  if (ih->handle)
  {
    /* Update header label width */
    if (data->header_labels && data->header_labels[col - 1])
      evas_object_size_hint_min_set(data->header_labels[col - 1], width, data->header_height);

    /* Update all cell labels in this column */
    if (data->cell_labels)
    {
      int max_lin = data->is_virtual ? data->alloc_num_lin : ih->data->num_lin;
      for (lin = 1; lin <= max_lin; lin++)
      {
        int idx = (lin - 1) * ih->data->num_col + (col - 1);
        if (data->cell_labels[idx])
          evas_object_size_hint_min_set(data->cell_labels[idx], width, data->row_height);
      }
    }
  }
}

int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);

  if (!data || !data->col_widths)
    return DEFAULT_COL_WIDTH;

  if (col < 1 || col > ih->data->num_col)
    return DEFAULT_COL_WIDTH;

  return data->col_widths[col - 1];
}

void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int num_col = ih->data->num_col;
  int max_lin;

  if (!data)
    return;

  data->selected_lin = lin;
  data->selected_col = col;

  max_lin = data->is_virtual ? data->alloc_num_lin : ih->data->num_lin;

  /* Update row highlight for all cells in each row */
  if (data->cell_bgs)
  {
    int row, c;
    for (row = 0; row < max_lin; row++)
    {
      for (c = 0; c < num_col; c++)
      {
        int idx = row * num_col + c;
        Evas_Object* cell_bg = data->cell_bgs[idx];
        if (cell_bg)
        {
          int actual_row = data->is_virtual ? (data->first_visible_row - 1 + row) : row;
          if (actual_row == lin - 1)
            evas_object_color_set(cell_bg, data->sel_r, data->sel_g, data->sel_b, 200);
          else
          {
            unsigned char r, g, b;
            eflTableGetCellBgColor(ih, actual_row + 1, c + 1, &r, &g, &b);
            evas_object_color_set(cell_bg, r, g, b, 255);
          }
        }
      }
    }
  }

  /* Update focus rectangle */
  eflTableUpdateFocusRect(ih);

  /* Scroll to cell */
  iupdrvTableScrollToCell(ih, lin, col);
}

void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);

  if (!data)
  {
    *lin = 1;
    *col = 1;
    return;
  }

  *lin = data->selected_lin > 0 ? data->selected_lin : 1;
  *col = data->selected_col > 0 ? data->selected_col : 1;
}

void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int x, y, w, h;
  int total_x = 0;
  int i;

  if (!data || !data->scroller)
    return;

  /* Calculate x position based on column widths */
  if (data->col_widths)
  {
    for (i = 0; i < col - 1 && i < ih->data->num_col; i++)
      total_x += data->col_widths[i] + CELL_PADDING;
  }

  /* Calculate y position */
  y = data->header_height + (lin - 1) * (data->row_height + CELL_PADDING);
  x = total_x;
  w = (col <= ih->data->num_col && data->col_widths) ? data->col_widths[col - 1] : DEFAULT_COL_WIDTH;
  h = data->row_height;

  elm_scroller_region_bring_in(data->scroller, x, y, w, h);
}

void iupdrvTableRedraw(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int lin, col;
  int num_col = ih->data->num_col;
  int num_lin = ih->data->num_lin;

  if (!data)
    return;

  if (data->is_virtual)
  {
    /* Virtual mode: just update visible rows (cells are reused) */
    eflTableUpdateVisibleRows(ih);
    return;
  }

  if (!data->cell_labels)
    return;

  /* Normal mode: update all visible cells */
  for (lin = 1; lin <= num_lin; lin++)
  {
    for (col = 1; col <= num_col; col++)
      eflTableUpdateCellLabel(ih, lin, col);
  }
}

void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);

  if (!data || !data->table)
    return;

  /* Use table padding to simulate grid lines */
  if (show)
    elm_table_padding_set(data->table, 1, 1);
  else
    elm_table_padding_set(data->table, 0, 0);
}

int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  return 1;
}

int iupdrvTableGetRowHeight(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  return data ? data->row_height : DEFAULT_ROW_HEIGHT;
}

int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  return data ? data->header_height : DEFAULT_HEADER_HEIGHT;
}

void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  int sb = iupdrvGetScrollbarSize();
  int visiblelines;
  int grid_padding = 0;

  /* Add vertical scrollbar width + horizontal border */
  *w += sb + 2;

  /* Add frame border */
  *h += 2;

  /* Add grid line spacing if SHOWGRID is enabled */
  if (iupAttribGetBoolean(ih, "SHOWGRID"))
  {
    visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
    if (visiblelines > 0)
      grid_padding = visiblelines;  /* 1px between header and each visible row */
    else
    {
      grid_padding = ih->data->num_lin;
      if (grid_padding > 15)
        grid_padding = 15;  /* Cap at 15 like natural size calculation */
    }

    *h += grid_padding;
  }
}

static void eflTableKeyDownCallback(void* data, const Efl_Event* ev)
{
  Ihandle* ih = (Ihandle*)data;
  IeflTableData* table_data = IEFL_TABLE_DATA(ih);
  Efl_Input_Key* key_ev = ev->info;
  const char* keyname;
  int lin, col;
  int new_lin, new_col;
  IFnii enteritem_cb;
  IFni kany_cb;
  int key = 0;
  int is_editing;

  keyname = efl_input_key_name_get(key_ev);

  if (!table_data || !keyname)
    return;

  is_editing = (table_data->editing_lin > 0);

  lin = table_data->selected_lin > 0 ? table_data->selected_lin : 1;
  col = table_data->selected_col > 0 ? table_data->selected_col : 1;
  new_lin = lin;
  new_col = col;

  /* Convert key to IUP key code and handle navigation */
  if (!strcmp(keyname, "Up"))
  {
    key = K_UP;
    if (!is_editing && lin > 1)
      new_lin = lin - 1;
  }
  else if (!strcmp(keyname, "Down"))
  {
    key = K_DOWN;
    if (!is_editing && lin < ih->data->num_lin)
      new_lin = lin + 1;
  }
  else if (!strcmp(keyname, "Left"))
  {
    key = K_LEFT;
    if (!is_editing && col > 1)
      new_col = col - 1;
  }
  else if (!strcmp(keyname, "Right"))
  {
    key = K_RIGHT;
    if (!is_editing && col < ih->data->num_col)
      new_col = col + 1;
  }
  else if (!strcmp(keyname, "Tab"))
  {
    key = K_TAB;
    if (is_editing)
      eflTableEndCellEdit(ih, 1);
    if (col < ih->data->num_col)
      new_col = col + 1;
    else if (lin < ih->data->num_lin)
    {
      new_lin = lin + 1;
      new_col = 1;
    }
  }
  else if (!strcmp(keyname, "Return") || !strcmp(keyname, "KP_Enter"))
  {
    key = K_CR;
    if (!is_editing && eflTableIsCellEditable(ih, col))
    {
      efl_input_processed_set(key_ev, EINA_TRUE);
      eflTableStartCellEdit(ih, lin, col);
      return;
    }
  }
  else if (!strcmp(keyname, "Escape"))
  {
    key = K_ESC;
    if (is_editing)
    {
      efl_input_processed_set(key_ev, EINA_TRUE);
      eflTableEndCellEdit(ih, 0);
      return;
    }
  }
  else if (!strcmp(keyname, "F2"))
  {
    key = K_F2;
    if (!is_editing && eflTableIsCellEditable(ih, col))
    {
      efl_input_processed_set(key_ev, EINA_TRUE);
      eflTableStartCellEdit(ih, lin, col);
      return;
    }
  }
  else if (!strcmp(keyname, "Home"))
  {
    key = K_HOME;
    if (!is_editing)
      new_col = 1;
  }
  else if (!strcmp(keyname, "End"))
  {
    key = K_END;
    if (!is_editing)
      new_col = ih->data->num_col;
  }
  else if (!strcmp(keyname, "Prior"))
  {
    key = K_PGUP;
    if (!is_editing)
      new_lin = 1;
  }
  else if (!strcmp(keyname, "Next"))
  {
    key = K_PGDN;
    if (!is_editing)
      new_lin = ih->data->num_lin;
  }
  else if ((!strcmp(keyname, "c") || !strcmp(keyname, "C")) &&
           efl_input_modifier_enabled_get(key_ev, EFL_INPUT_MODIFIER_CONTROL, NULL))
  {
    key = K_cC;
    if (!is_editing)
    {
      char* value = eflTableGetCellText(ih, lin, col);
      if (value && *value)
        IupStoreGlobal("CLIPBOARD", value);
      efl_input_processed_set(key_ev, EINA_TRUE);
    }
  }
  else if ((!strcmp(keyname, "v") || !strcmp(keyname, "V")) &&
           efl_input_modifier_enabled_get(key_ev, EFL_INPUT_MODIFIER_CONTROL, NULL))
  {
    key = K_cV;
    if (!is_editing && !table_data->is_virtual && eflTableIsCellEditable(ih, col))
    {
      const char* text = IupGetGlobal("CLIPBOARD");
      if (text && *text)
      {
        IFnii valuechanged_cb;

        iupdrvTableSetCellValue(ih, lin, col, text);

        valuechanged_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
        if (valuechanged_cb)
          valuechanged_cb(ih, lin, col);
      }
      efl_input_processed_set(key_ev, EINA_TRUE);
    }
  }

  /* Mark navigation keys as handled to prevent EFL focus navigation */
  if (key == K_UP || key == K_DOWN || key == K_LEFT || key == K_RIGHT ||
      key == K_TAB || key == K_HOME || key == K_END || key == K_PGUP || key == K_PGDN)
  {
    efl_input_processed_set(key_ev, EINA_TRUE);
  }

  /* Fire K_ANY callback */
  if (key != 0)
  {
    kany_cb = (IFni)IupGetCallback(ih, "K_ANY");
    if (kany_cb)
    {
      int ret = kany_cb(ih, key);
      if (ret == IUP_IGNORE)
        return;
      if (ret == IUP_CLOSE)
      {
        IupExitLoop();
        return;
      }
    }
  }

  /* Update focus if position changed */
  if (new_lin != lin || new_col != col)
  {
    iupdrvTableSetFocusCell(ih, new_lin, new_col);

    /* Fire ENTERITEM_CB */
    enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enteritem_cb)
      enteritem_cb(ih, new_lin, new_col);
  }
}

static int eflTableMapMethod(Ihandle* ih)
{
  Evas_Object* parent;
  Evas_Object* scroller;
  Evas_Object* table;
  IeflTableData* data;
  int is_virtual;

  parent = iupeflGetParentWidget(ih);
  if (!parent)
    return IUP_ERROR;

  /* Allocate EFL-specific data */
  data = (IeflTableData*)calloc(1, sizeof(IeflTableData));
  if (!data)
    return IUP_ERROR;

  ih->data->native_data = data;

  data->header_height = DEFAULT_HEADER_HEIGHT;
  data->row_height = DEFAULT_ROW_HEIGHT;
  data->selected_lin = 0;
  data->selected_col = 0;
  data->target_height = 0;
  data->sort_column = 0;
  data->sort_ascending = 1;

  /* Check for virtual mode */
  is_virtual = iupAttribGetBoolean(ih, "VIRTUALMODE");
  data->is_virtual = is_virtual;

  /* Calculate target height if VISIBLELINES is set */
  {
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
    if (visiblelines > 0)
    {
      int grid_padding = 0;
      if (iupAttribGetBoolean(ih, "SHOWGRID"))
        grid_padding = visiblelines;
      data->target_height = data->header_height + (data->row_height * visiblelines) + grid_padding + 2;
    }
  }

  /* Get selection color from global TXTHLCOLOR */
  {
    char* hlcolor = IupGetGlobal("TXTHLCOLOR");
    if (hlcolor)
      iupStrToRGB(hlcolor, &data->sel_r, &data->sel_g, &data->sel_b);
    else
    {
      data->sel_r = 51;
      data->sel_g = 102;
      data->sel_b = 153;
    }
  }

  if (is_virtual)
  {
    /*
     * Virtual mode: Same scroller+table as normal mode, but with spacer sandwich.
     * Layout: scroller -> table -> [top_spacer, header, visible_cells, bottom_spacer]
     * On scroll: resize spacers to keep cells in view, update cell content.
     */
    scroller = elm_scroller_add(parent);
    if (!scroller)
    {
      free(data);
      ih->data->native_data = NULL;
      return IUP_ERROR;
    }

    elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_AUTO);
    evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);

    elm_object_focus_allow_set(scroller, EINA_TRUE);
    efl_event_callback_add(scroller, EFL_EVENT_KEY_DOWN, eflTableKeyDownCallback, ih);
    efl_event_callback_add(scroller, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflTableResizeCallback, ih);
    evas_object_smart_callback_add(scroller, "focused", eflTableFocusInCallback, ih);
    evas_object_smart_callback_add(scroller, "unfocused", eflTableFocusOutCallback, ih);
    evas_object_smart_callback_add(scroller, "scroll", eflTableVirtualScrollCallback, ih);

    data->scroller = scroller;

    table = elm_table_add(scroller);
    if (!table)
    {
      evas_object_del(scroller);
      free(data);
      ih->data->native_data = NULL;
      return IUP_ERROR;
    }

    elm_table_homogeneous_set(table, EINA_FALSE);

    if (iupAttribGetBoolean(ih, "SHOWGRID"))
      elm_table_padding_set(table, 1, 1);
    else
      elm_table_padding_set(table, 0, 0);

    evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(table, EVAS_HINT_FILL, 0.0);
    evas_object_show(table);

    data->table = table;

    elm_object_content_set(scroller, table);

    data->focus_rect = evas_object_rectangle_add(evas_object_evas_get(table));
    evas_object_pass_events_set(data->focus_rect, EINA_TRUE);
    data->focus_rect_lin = 0;
    data->focus_rect_col = 0;

    ih->handle = (void*)scroller;

    iupeflAddToParent(ih);
    evas_object_show(scroller);

    /* Initialize visible row tracking */
    data->first_visible_row = 1;
    data->visible_row_count = 0;
    data->top_spacer = NULL;
    data->bottom_spacer = NULL;

    /* Build cells with spacer sandwich */
    eflTableRebuildVirtualCells(ih);
  }
  else
  {
    /* Normal mode: scroller + elm_table */
    scroller = elm_scroller_add(parent);
    if (!scroller)
    {
      free(data);
      ih->data->native_data = NULL;
      return IUP_ERROR;
    }

    elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_AUTO);
    evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);

    elm_object_focus_allow_set(scroller, EINA_TRUE);
    efl_event_callback_add(scroller, EFL_EVENT_KEY_DOWN, eflTableKeyDownCallback, ih);
    efl_event_callback_add(scroller, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflTableResizeCallback, ih);
    evas_object_smart_callback_add(scroller, "focused", eflTableFocusInCallback, ih);
    evas_object_smart_callback_add(scroller, "unfocused", eflTableFocusOutCallback, ih);

    data->scroller = scroller;

    table = elm_table_add(scroller);
    if (!table)
    {
      evas_object_del(scroller);
      free(data);
      ih->data->native_data = NULL;
      return IUP_ERROR;
    }

    elm_table_homogeneous_set(table, EINA_FALSE);

    if (iupAttribGetBoolean(ih, "SHOWGRID"))
      elm_table_padding_set(table, 1, 1);
    else
      elm_table_padding_set(table, 0, 0);

    evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(table, EVAS_HINT_FILL, 0.0);
    evas_object_show(table);

    data->table = table;

    elm_object_content_set(scroller, table);

    data->focus_rect = evas_object_rectangle_add(evas_object_evas_get(table));
    evas_object_pass_events_set(data->focus_rect, EINA_TRUE);
    data->focus_rect_lin = 0;
    data->focus_rect_col = 0;

    ih->handle = (void*)scroller;

    iupeflAddToParent(ih);
    evas_object_show(scroller);

    /* Normal mode: create all cells */
    eflTableRebuildCells(ih);
  }

  /* Apply pre-set FOCUSCELL attribute */
  {
    char* focuscell = iupAttribGet(ih, "FOCUSCELL");
    if (focuscell)
    {
      int lin = 0, col = 0;
      if (iupStrToIntInt(focuscell, &lin, &col, ':') == 2)
      {
        if (lin >= 1 && lin <= ih->data->num_lin &&
            col >= 1 && col <= ih->data->num_col)
        {
          iupdrvTableSetFocusCell(ih, lin, col);

          /* Schedule deferred focus rect update after layout is computed */
          if (!iupAttribGet(ih, "_IUP_EFL_FOCUSRECT_JOB"))
          {
            Ecore_Job* job = ecore_job_add(eflTableFocusRectJob, ih);
            iupAttribSet(ih, "_IUP_EFL_FOCUSRECT_JOB", (char*)job);
          }
        }
      }
    }
  }

  return IUP_NOERROR;
}

static void eflTableUnMapMethod(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);

  if (data)
  {
    /* Cancel any active editing */
    if (data->editing_lin > 0)
    {
      data->editing_lin = 0;
      data->editing_col = 0;
    }

    /* Free original value if allocated */
    if (data->original_value)
    {
      free(data->original_value);
      data->original_value = NULL;
    }

    /* Cancel pending scroll job */
    if (data->scroll_job)
    {
      ecore_job_del(data->scroll_job);
      data->scroll_job = NULL;
    }

    /* Clear all cells */
    eflTableClearCells(ih);

    /* Delete focus rectangle */
    if (data->focus_rect)
    {
      evas_object_del(data->focus_rect);
      data->focus_rect = NULL;
    }

    /* Free column widths */
    if (data->col_widths)
    {
      free(data->col_widths);
      data->col_widths = NULL;
    }

    if (data->scroller)
    {
      efl_event_callback_del(data->scroller, EFL_EVENT_KEY_DOWN, eflTableKeyDownCallback, ih);
      efl_event_callback_del(data->scroller, EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, eflTableResizeCallback, ih);
      evas_object_smart_callback_del(data->scroller, "focused", eflTableFocusInCallback);
      evas_object_smart_callback_del(data->scroller, "unfocused", eflTableFocusOutCallback);
      if (data->is_virtual)
        evas_object_smart_callback_del(data->scroller, "scroll", eflTableVirtualScrollCallback);
      evas_object_del(data->scroller);
      data->scroller = NULL;
      data->table = NULL;
      data->top_spacer = NULL;
      data->bottom_spacer = NULL;
    }

    free(data);
    ih->data->native_data = NULL;
  }

  ih->handle = NULL;
}

static void eflTableLayoutUpdateMethod(Ihandle* ih)
{
  IeflTableData* data = IEFL_TABLE_DATA(ih);
  int height = ih->currentheight;

  /* If VISIBLELINES is set, clamp height to target */
  if (data && data->target_height > 0 && height > data->target_height)
    height = data->target_height;

  iupeflSetPosSize(ih, ih->x, ih->y, ih->currentwidth, height);

  /* Also set max size hint on scroller to prevent expansion */
  if (data && data->scroller && data->target_height > 0)
    evas_object_size_hint_max_set(data->scroller, -1, data->target_height);
}

static int eflTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
  {
    IeflTableData* data = IEFL_TABLE_DATA(ih);

    ih->data->sortable = 0;

    if (data)
    {
      data->sort_column = 0;
      eflTableUpdateSortIndicators(ih);
    }
  }

  return 0;
}

static int eflTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 1;
}

void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = eflTableMapMethod;
  ic->UnMap = eflTableUnMapMethod;
  ic->LayoutUpdate = eflTableLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SIZE", NULL, NULL, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, eflTableSetSortableAttrib);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, eflTableSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EDITABLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "EDITABLE", NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "USERRESIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
