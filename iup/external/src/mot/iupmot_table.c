/** \file
 * \brief IupTable control - Motif driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/BulletinB.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <Xm/Text.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xrender.h>

#ifdef IUP_USE_XFT
#include <X11/Xft/Xft.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_childtree.h"
#include "iup_dialog.h"
#include "iup_image.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_table.h"


#define MOT_TABLE_DEF_ROW_HEIGHT    20
#define MOT_TABLE_DEF_COL_WIDTH     80
#define MOT_TABLE_HEADER_HEIGHT     24
#define MOT_TABLE_CELL_PADDING      3

/* ========================================================================= */
/* Motif-specific data structure (XmDrawingArea-based)                      */
/* ========================================================================= */

typedef struct
{
  char* value;
  char* image;
} ImotTableCell;

typedef struct _ImotTableData
{
  Widget container;          /* XmBulletinBoard parent */
  Widget drawing_area;       /* XmDrawingArea for table */
  Widget sb_horiz;
  Widget sb_vert;
  Widget edit_text;          /* XmTextField for cell editing (created on demand) */

  GC gc;
  XFontStruct* font_struct;
  int font_struct_owned;     /* 1 if loaded here, 0 if from the cache */

#ifdef IUP_USE_XFT
  XftDraw* xft_draw;
  XftFont* xft_font;
#endif

  int row_height;
  int header_height;
  int* col_widths;           /* Array of column widths [num_col] - displayed width (includes stretch) */
  int* col_natural_widths;   /* Array of natural column widths [num_col] - from auto-sizing, before stretch */
  int* col_width_set;        /* Array of flags: 1 if width explicitly set, 0 if auto [num_col] */

  int scroll_x;              /* Horizontal scroll position (pixels) */
  int scroll_y;              /* Vertical scroll position (pixels) */
  int first_visible_row;     /* First visible row (0-based) */
  int first_visible_col;     /* First visible column (0-based) */

  int current_row;           /* Current focused row (1-based, 0=none) */
  int current_col;           /* Current focused column (1-based, 0=none) */

  char* row_selected;        /* [num_lin] selection flags */
  int row_selected_size;     /* allocated size of row_selected */
  int anchor_row;            /* shift-click range anchor (1-based, 0=none) */
  int pending_scroll_lin, pending_scroll_col;

  /* Row drag-reorder state (SHOWDRAGDROP) */
  int drag_source_row;       /* Row where the drag started (1-based, 0=none) */
  int drag_target_row;       /* Insert-before index while dragging (0-based, -1=none) */
  int row_dragging;          /* 1 once past the drag threshold */
  int drag_start_x;
  int drag_start_y;

  /* Column drag-reorder state (ALLOWREORDER) */
  int drag_source_col;       /* Header column where the drag started (1-based, 0=none) */
  int drag_target_col;       /* Insert-before index while dragging (0-based, -1=none) */
  int col_dragging;          /* 1 once past the drag threshold */

  /* Column resize state (USERRESIZE) */
  int resize_col;            /* Column whose right divider is dragged (1-based, 0=none) */
  int resize_start_width;
  Cursor resize_cursor;
  int resize_cursor_shown;

  ImotTableCell** cells;     /* [num_lin][num_col] */
  char** col_titles;         /* [num_col] -> string */

  int edit_lin;              /* Row being edited (1-based, 0=not editing) */
  int edit_col;              /* Column being edited (1-based, 0=not editing) */

  int show_grid;

  XtIntervalId autosize_timer;

  int sort_column;           /* Currently sorted column (1-based, 0=none) */
  char* sort_signs;

  Pixel bg_pixel;
  Pixel fg_pixel;
  Pixel header_bg_pixel;
  Pixel grid_pixel;
  Pixel select_bg_pixel;

} ImotTableData;

#define IMOT_TABLE_DATA(ih) ((ImotTableData*)(ih->data->native_data))

typedef struct _ImotTableFont
{
  XFontStruct* font_struct;
#ifdef IUP_USE_XFT
  XftFont* xft_font;
#endif
} ImotTableFont;

/* ========================================================================= */
/* Forward Declarations                                                      */
/* ========================================================================= */

static void motTableRedraw(Ihandle* ih);
static void motTableUpdateScrollbars(Ihandle* ih);
static void motTableCellToPixel(Ihandle* ih, int lin, int col, int* px, int* py, int* pw, int* ph);
static void motTableEditKeyPressCallback(Widget w, XtPointer client_data, XEvent* event, Boolean* cont);
static void motTableEndCellEdit(Ihandle* ih, int apply);
static void motTablePixelToCell(Ihandle* ih, int px, int py, int* lin, int* col);
static void motTableStartCellEdit(Ihandle* ih, int lin, int col);
static void motTableEndCellEdit(Ihandle* ih, int apply);
static int motTableFindTargetRow(Ihandle* ih, int py);
static void motTableMoveRow(Ihandle* ih, int from, int to);
static void motTableRowDragMotion(Widget w, XtPointer client_data, XEvent* event, Boolean* cont);

/* ========================================================================= */
/* Helper Functions - Sort Indicators                                       */
/* ========================================================================= */

static void motTableDrawSortArrow(Display* display, Window window, GC gc, int x, int y, int is_up)
{
  XPoint points[3];
  int arrow_size = 6;  /* Triangle side length */

  if (is_up)
  {
    points[0].x = x;                    points[0].y = y + arrow_size;
    points[1].x = x + arrow_size;       points[1].y = y + arrow_size;
    points[2].x = x + arrow_size / 2;   points[2].y = y;
  }
  else
  {
    points[0].x = x;                    points[0].y = y;
    points[1].x = x + arrow_size;       points[1].y = y;
    points[2].x = x + arrow_size / 2;   points[2].y = y + arrow_size;
  }

  XFillPolygon(display, window, gc, points, 3, Convex, CoordModeOrigin);
}

/* ========================================================================= */
/* Helper Functions - Selection                                             */
/* ========================================================================= */

static char* motTableSelection(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (ih->data->num_lin <= 0)
    return NULL;

  if (mot_data->row_selected_size < ih->data->num_lin)
  {
    mot_data->row_selected = (char*)realloc(mot_data->row_selected, ih->data->num_lin);
    memset(mot_data->row_selected + mot_data->row_selected_size, 0, ih->data->num_lin - mot_data->row_selected_size);
    mot_data->row_selected_size = ih->data->num_lin;
  }

  return mot_data->row_selected;
}

static void motTableFreeCell(ImotTableCell* cell)
{
  if (cell->value)
    free(cell->value);
  if (cell->image)
    free(cell->image);
}

static void motTableSortRows(Ihandle* ih, int col, int ascending)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int i, j;
  int num_rows = ih->data->num_lin;
  int num_cols = ih->data->num_col;
  int* order;
  int* new_pos;

  if (!mot_data->cells || num_rows < 2 || col < 1 || col > num_cols)
    return;

  order = (int*)malloc(num_rows * sizeof(int));
  new_pos = (int*)malloc((num_rows + 1) * sizeof(int));
  for (i = 0; i < num_rows; i++)
    order[i] = i + 1;

  for (i = 0; i < num_rows - 1; i++)
  {
    for (j = 0; j < num_rows - i - 1; j++)
    {
      const char* val1 = mot_data->cells[j][col - 1].value;
      const char* val2 = mot_data->cells[j + 1][col - 1].value;
      int should_swap = 0;

      if (!val1) val1 = "";
      if (!val2) val2 = "";

      int cmp = iupStrCompare(val1, val2, 0, 1);

      if (ascending)
        should_swap = (cmp > 0);  /* Ascending: swap if val1 > val2 */
      else
        should_swap = (cmp < 0);  /* Descending: swap if val1 < val2 */

      if (should_swap)
      {
        ImotTableCell* temp_row = mot_data->cells[j];
        int temp_order = order[j];
        mot_data->cells[j] = mot_data->cells[j + 1];
        mot_data->cells[j + 1] = temp_row;
        order[j] = order[j + 1];
        order[j + 1] = temp_order;
      }
    }
  }

  for (i = 0; i < num_rows; i++)
    new_pos[order[i]] = i + 1;

  if (mot_data->row_selected)
  {
    char* row_selected = motTableSelection(ih);
    char* sel = (char*)malloc(num_rows);
    memcpy(sel, row_selected, num_rows);
    for (i = 0; i < num_rows; i++)
      row_selected[i] = sel[order[i] - 1];
    free(sel);
  }

  if (mot_data->current_row > 0 && mot_data->current_row <= num_rows)
    mot_data->current_row = new_pos[mot_data->current_row];
  if (mot_data->anchor_row > 0 && mot_data->anchor_row <= num_rows)
    mot_data->anchor_row = new_pos[mot_data->anchor_row];

  iupTableSortLinAttribs(ih, order);

  free(order);
  free(new_pos);
}

static int motTableRowSelected(Ihandle* ih, int lin)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data->row_selected || lin < 1 || lin > mot_data->row_selected_size)
    return 0;

  return mot_data->row_selected[lin - 1];
}

static void motTableSelectRow(Ihandle* ih, int lin, int select, int exclusive)
{
  char* sel = motTableSelection(ih);

  if (!sel || lin < 1 || lin > ih->data->num_lin)
    return;

  if (exclusive)
    memset(sel, 0, ih->data->num_lin);

  sel[lin - 1] = (char)(select ? 1 : 0);
}

static void motTableSelectRange(Ihandle* ih, int from, int to)
{
  char* sel = motTableSelection(ih);
  int lin;

  if (!sel)
    return;

  if (from > to)
  {
    lin = from;
    from = to;
    to = lin;
  }

  memset(sel, 0, ih->data->num_lin);

  for (lin = from; lin <= to; lin++)
  {
    if (lin >= 1 && lin <= ih->data->num_lin)
      sel[lin - 1] = 1;
  }
}

/* ========================================================================= */
/* Helper Functions - Cell Storage                                          */
/* ========================================================================= */

static char* motTableGetCellValueInternal(Ihandle* ih, int lin, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return NULL;

  /* Virtual mode: Query data via VALUE_CB callback (row and column are 1-based for IUP) */
  sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
  if (value_cb)
  {
    return value_cb(ih, lin, col);
  }

  if (!mot_data || !mot_data->cells)
    return NULL;

  return mot_data->cells[lin-1][col-1].value;
}

static char* motTableGetCellImage(Ihandle* ih, int lin, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (mot_data->cells && lin >= 1 && lin <= ih->data->num_lin && col >= 1 && col <= ih->data->num_col &&
      mot_data->cells[lin-1][col-1].image)
    return mot_data->cells[lin-1][col-1].image;

  return iupTableGetCellImageCb(ih, lin, col);
}

static int motTableGetCellImageSize(Ihandle* ih, const char* name, int* img_w, int* img_h)
{
  int avail_h = IMOT_TABLE_DATA(ih)->row_height - 2;

  *img_w = 0;
  *img_h = 0;
  iupImageGetInfo(name, img_w, img_h, NULL);
  if (*img_w <= 0 || *img_h <= 0)
    return 0;

  if (ih->data->fit_image && *img_h > avail_h && avail_h > 0)
  {
    *img_w = (*img_w * avail_h) / *img_h;
    *img_h = avail_h;
  }
  return 1;
}

static void motTableSetCellValueInternal(Ihandle* ih, int lin, int col, const char* value)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || !mot_data->cells)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  if (mot_data->cells[lin-1][col-1].value)
  {
    free(mot_data->cells[lin-1][col-1].value);
    mot_data->cells[lin-1][col-1].value = NULL;
  }

  if (value)
    mot_data->cells[lin-1][col-1].value = iupStrDup(value);
}

/* ========================================================================= */
/* Helper Functions - Coordinate Conversion                                 */
/* ========================================================================= */

static void motTableCellToPixel(Ihandle* ih, int lin, int col, int* px, int* py, int* pw, int* ph)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int x = 0, y = 0, c;

  for (c = 0; c < col - 1 && c < ih->data->num_col; c++)
    x += mot_data->col_widths[c];

  y = mot_data->header_height + (lin - 1) * mot_data->row_height;

  x -= mot_data->scroll_x;
  y -= mot_data->scroll_y;

  if (px) *px = x;
  if (py) *py = y;
  if (pw) *pw = (col <= ih->data->num_col) ? mot_data->col_widths[col-1] : MOT_TABLE_DEF_COL_WIDTH;
  if (ph) *ph = mot_data->row_height;
}

static void motTablePixelToCell(Ihandle* ih, int px, int py, int* lin, int* col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int x, c, row;

  px += mot_data->scroll_x;

  *col = 0;
  x = 0;
  for (c = 0; c < ih->data->num_col; c++)
  {
    if (px >= x && px < x + mot_data->col_widths[c])
    {
      *col = c + 1;
      break;
    }
    x += mot_data->col_widths[c];
  }

  *lin = 0;
  if (py < mot_data->header_height)
  {
    *lin = 0; /* Header row */
  }
  else
  {
    py += mot_data->scroll_y;
    row = (py - mot_data->header_height) / mot_data->row_height;
    if (row >= 0 && row < ih->data->num_lin)
      *lin = row + 1;
  }
}

static int motTableFindTargetRow(Ihandle* ih, int py)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int num_lin = ih->data->num_lin;
  int y = py + mot_data->scroll_y;
  int lin;

  for (lin = 1; lin <= num_lin; lin++)
  {
    int mid = mot_data->header_height + (lin - 1) * mot_data->row_height + mot_data->row_height / 2;
    if (y < mid)
      return lin - 1;
  }
  return num_lin;
}

static void motTableMoveRow(Ihandle* ih, int from, int to)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int num_lin = ih->data->num_lin;
  ImotTableCell* row;
  int l;

  if (from == to || from < 1 || to < 1 || from > num_lin || to > num_lin)
    return;
  if (!mot_data->cells)
    return;

  row = mot_data->cells[from - 1];
  if (from < to)
    for (l = from - 1; l < to - 1; l++)
      mot_data->cells[l] = mot_data->cells[l + 1];
  else
    for (l = from - 1; l > to - 1; l--)
      mot_data->cells[l] = mot_data->cells[l - 1];
  mot_data->cells[to - 1] = row;

  if (mot_data->row_selected && mot_data->row_selected_size >= num_lin)
  {
    char selected = mot_data->row_selected[from - 1];
    if (from < to)
      for (l = from - 1; l < to - 1; l++)
        mot_data->row_selected[l] = mot_data->row_selected[l + 1];
    else
      for (l = from - 1; l > to - 1; l--)
        mot_data->row_selected[l] = mot_data->row_selected[l - 1];
    mot_data->row_selected[to - 1] = selected;
  }

  iupTableMoveLinAttribs(ih, from, to);

  mot_data->current_row = to;
  motTableRedraw(ih);
}

static int motTableHeaderDivider(Ihandle* ih, int px, int py)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int c, x = -mot_data->scroll_x;

  if (py < 0 || py >= mot_data->header_height)
    return 0;

  for (c = 0; c < ih->data->num_col; c++)
  {
    x += mot_data->col_widths[c];
    if (px >= x - 3 && px <= x + 3)
      return c + 1;
  }
  return 0;
}

static int motTableFindTargetCol(Ihandle* ih, int px)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int c, x = -mot_data->scroll_x;

  for (c = 0; c < ih->data->num_col; c++)
  {
    if (px < x + mot_data->col_widths[c] / 2)
      return c;
    x += mot_data->col_widths[c];
  }
  return ih->data->num_col;
}

static void motTableMoveCol(Ihandle* ih, int from, int to)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int num_col = ih->data->num_col;
  int step = (from < to) ? 1 : -1;
  int c, lin, width, natural, width_set;
  char sign;
  char* title;

  if (from == to || from < 1 || to < 1 || from > num_col || to > num_col)
    return;

  width = mot_data->col_widths[from - 1];
  natural = mot_data->col_natural_widths[from - 1];
  width_set = mot_data->col_width_set[from - 1];
  sign = mot_data->sort_signs[from - 1];
  title = mot_data->col_titles[from - 1];
  for (c = from - 1; c != to - 1; c += step)
  {
    mot_data->col_widths[c] = mot_data->col_widths[c + step];
    mot_data->col_natural_widths[c] = mot_data->col_natural_widths[c + step];
    mot_data->col_width_set[c] = mot_data->col_width_set[c + step];
    mot_data->sort_signs[c] = mot_data->sort_signs[c + step];
    mot_data->col_titles[c] = mot_data->col_titles[c + step];
  }
  mot_data->col_widths[to - 1] = width;
  mot_data->col_natural_widths[to - 1] = natural;
  mot_data->col_width_set[to - 1] = width_set;
  mot_data->sort_signs[to - 1] = sign;
  mot_data->col_titles[to - 1] = title;

  for (lin = 0; mot_data->cells && lin < ih->data->num_lin; lin++)
  {
    ImotTableCell cell = mot_data->cells[lin][from - 1];
    for (c = from - 1; c != to - 1; c += step)
      mot_data->cells[lin][c] = mot_data->cells[lin][c + step];
    mot_data->cells[lin][to - 1] = cell;
  }

  iupTableMoveColAttribs(ih, from, to);

  if (mot_data->sort_column > 0)
    mot_data->sort_column = iupTableMoveColPos(mot_data->sort_column, from, to);
  if (mot_data->current_col > 0)
    mot_data->current_col = iupTableMoveColPos(mot_data->current_col, from, to);

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

static void motTableHeaderSort(Ihandle* ih, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int sign = (mot_data->sort_column == col && mot_data->sort_signs[col-1] == 1) ? -1 : 1;

  IFni sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
  if (sort_cb && sort_cb(ih, col) == IUP_IGNORE)
    return;

  if (mot_data->sort_column > 0 && mot_data->sort_column <= ih->data->num_col)
    mot_data->sort_signs[mot_data->sort_column - 1] = 0;

  mot_data->sort_column = col;
  mot_data->sort_signs[col-1] = sign;

  if (!IupGetCallback(ih, "VALUE_CB"))
    motTableSortRows(ih, col, (sign == 1));

  motTableRedraw(ih);
}

static void motTableHeaderPointerMotion(Widget w, XtPointer client_data, XEvent* event, Boolean* cont)
{
  Ihandle* ih = (Ihandle*)client_data;
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  XMotionEvent* motion = (XMotionEvent*)event;
  int show;

  (void)cont;

  if (!mot_data || mot_data->resize_col || mot_data->drag_source_col)
    return;

  show = ih->data->user_resize && motTableHeaderDivider(ih, motion->x, motion->y);
  if (show == mot_data->resize_cursor_shown)
    return;

  if (show)
  {
    if (!mot_data->resize_cursor)
      mot_data->resize_cursor = XCreateFontCursor(iupmot_display, XC_sb_h_double_arrow);
    XDefineCursor(iupmot_display, XtWindow(w), mot_data->resize_cursor);
  }
  else
    XUndefineCursor(iupmot_display, XtWindow(w));
  mot_data->resize_cursor_shown = show;
}

static void motTableRowDragMotion(Widget w, XtPointer client_data, XEvent* event, Boolean* cont)
{
  Ihandle* ih = (Ihandle*)client_data;
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  XMotionEvent* motion = (XMotionEvent*)event;
  int target;

  (void)w;
  (void)cont;

  if (mot_data && mot_data->resize_col)
  {
    int width = mot_data->resize_start_width + motion->x - mot_data->drag_start_x;
    int col = mot_data->resize_col;
    if (width < 10)
      width = 10;
    mot_data->col_widths[col - 1] = width;
    mot_data->col_natural_widths[col - 1] = width;
    mot_data->col_width_set[col - 1] = 1;
    motTableUpdateScrollbars(ih);
    motTableRedraw(ih);
    return;
  }

  if (mot_data && mot_data->drag_source_col)
  {
    if (!mot_data->col_dragging)
    {
      int dx = motion->x - mot_data->drag_start_x;
      int dy = motion->y - mot_data->drag_start_y;
      if (dx * dx + dy * dy < 25)
        return;
      mot_data->col_dragging = 1;
    }

    target = motTableFindTargetCol(ih, motion->x);
    if (target != mot_data->drag_target_col)
    {
      mot_data->drag_target_col = target;
      motTableRedraw(ih);
    }
    return;
  }

  if (mot_data && iupTableCellsMode(ih))
  {
    int lin, col;
    motTablePixelToCell(ih, motion->x, motion->y, &lin, &col);
    if (lin > 0 && col > 0)
      iupTableCellsExtendTo(ih, lin, col);
    return;
  }

  if (!mot_data || mot_data->drag_source_row < 1 || !ih->data->show_dragdrop)
    return;

  if (!mot_data->row_dragging)
  {
    int dx = motion->x - mot_data->drag_start_x;
    int dy = motion->y - mot_data->drag_start_y;
    if (dx * dx + dy * dy < 25)
      return;
    mot_data->row_dragging = 1;
  }

  target = motTableFindTargetRow(ih, motion->y);
  if (target != mot_data->drag_target_row)
  {
    mot_data->drag_target_row = target;
    motTableRedraw(ih);
  }
}

/* ========================================================================= */
/* Drawing Functions                                                         */
/* ========================================================================= */

static void motTableGetCellFont(Ihandle* ih, int lin, int col, ImotTableFont* font)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  char* value = NULL;

  font->font_struct = mot_data->font_struct;
#ifdef IUP_USE_XFT
  font->xft_font = mot_data->xft_font;
#endif

  if (lin > 0)
  {
    value = iupAttribGetId2(ih, "FONT", lin, col);
    if (!value)
      value = iupAttribGetId2(ih, "FONT", 0, col);
    if (!value)
      value = iupAttribGetId2(ih, "FONT", lin, 0);
  }

  if (!value || !*value)
    return;

#ifdef IUP_USE_XFT
  if (font->xft_font)
  {
    XftFont* xft_font = (XftFont*)iupmotGetXftFont(value);
    if (xft_font)
      font->xft_font = xft_font;
    return;
  }
#endif

  {
    XFontStruct* font_struct = iupmotGetFontStruct(value);
    if (font_struct)
      font->font_struct = font_struct;
  }
}

static int motTableGetTextWidth(ImotTableFont* font, const char* text, int len)
{
#ifdef IUP_USE_XFT
  if (font->xft_font)
  {
    XGlyphInfo extents;
    XftTextExtentsUtf8(iupmot_display, font->xft_font, (FcChar8*)text, len, &extents);
    return extents.width;
  }
#endif
  if (font->font_struct)
    return XTextWidth(font->font_struct, text, len);
  return len * 7;
}

static int motTableDrawCellImage(Ihandle* ih, Window window, int lin, int col, int x, int y, int w, int h, Pixel bg_pixel)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  Display* display = iupmot_display;
  char* name = motTableGetCellImage(ih, lin, col);
  char bgcolor[30];
  unsigned char r, g, b;
  int img_w, img_h, bpp, draw_w, draw_h, dx, dy;
  Pixmap pixmap;

  if (!name)
    return 0;

  iupmotColorGetRGB(bg_pixel, &r, &g, &b);
  snprintf(bgcolor, sizeof(bgcolor), "%d %d %d", (int)r, (int)g, (int)b);

  pixmap = (Pixmap)iupImageGetImage(name, ih, !XtIsSensitive(ih->handle), bgcolor);
  if (!pixmap)
    return 0;

  iupdrvImageGetInfo((void*)pixmap, &img_w, &img_h, &bpp);
  if (img_w <= 0 || img_h <= 0)
    return 0;

  draw_w = img_w;
  draw_h = img_h;
  if (ih->data->fit_image && img_h > h - 2 && h > 2)
  {
    draw_h = h - 2;
    draw_w = (img_w * draw_h) / img_h;
  }
  if (draw_w > w - 4)
    draw_w = w - 4;
  if (draw_w <= 0)
    return 0;

  dx = x + 2;
  dy = y + (h - draw_h) / 2;

  if (draw_h != img_h)
  {
    XRenderPictFormat* fmt = XRenderFindVisualFormat(display, iupmot_visual);
    if (fmt)
    {
      Picture src = XRenderCreatePicture(display, pixmap, fmt, 0, NULL);
      Picture dst = XRenderCreatePicture(display, window, fmt, 0, NULL);
      double scale = (double)img_h / (double)draw_h;
      XTransform xf = {{
        { XDoubleToFixed(scale), XDoubleToFixed(0), XDoubleToFixed(0) },
        { XDoubleToFixed(0), XDoubleToFixed(scale), XDoubleToFixed(0) },
        { XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1) }
      }};
      XRenderSetPictureTransform(display, src, &xf);
      XRenderSetPictureFilter(display, src, FilterBilinear, NULL, 0);
      XRenderComposite(display, PictOpSrc, src, None, dst, 0, 0, 0, 0, dx, dy, (unsigned int)draw_w, (unsigned int)draw_h);
      XRenderFreePicture(display, dst);
      XRenderFreePicture(display, src);
    }
  }
  else
  {
    int src_y = 0;
    if (draw_h > h)
    {
      src_y = (draw_h - h) / 2;
      draw_h = h;
      dy = y;
    }
    XCopyArea(display, pixmap, window, mot_data->gc, 0, src_y, (unsigned int)draw_w, (unsigned int)draw_h, dx, dy);
  }

  return draw_w + 4;
}

static void motTableDrawCell(Ihandle* ih, int lin, int col, int is_header)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  Display* display = iupmot_display;
  Window window = XtWindow(mot_data->drawing_area);
  int x, y, w, h;
  const char* text;
  int text_len, text_width, text_x, text_y;
  int is_focused_cell = (lin == mot_data->current_row && col == mot_data->current_col);
  int is_focused_row = (!is_header && motTableRowSelected(ih, lin));
  int img_offset = 0;
  Pixel cell_bg;

  if (is_header)
  {
    int c, cell_x = 0;
    for (c = 0; c < col - 1 && c < ih->data->num_col; c++)
      cell_x += mot_data->col_widths[c];

    x = cell_x - mot_data->scroll_x;
    y = 0;
    w = mot_data->col_widths[col-1];
    h = mot_data->header_height;

    text = mot_data->col_titles[col-1];
  }
  else
  {
    motTableCellToPixel(ih, lin, col, &x, &y, &w, &h);
    text = motTableGetCellValueInternal(ih, lin, col);
  }

  char* bgcolor = NULL;

  /* Get background color with hierarchy: per-cell > per-column > per-row > alternating > default */
  if (!is_header)
  {
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);
    if (!bgcolor)
      bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);
    if (!bgcolor)
      bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);

    if (!bgcolor)
    {
      char* alternate_color = iupAttribGet(ih, "ALTERNATECOLOR");
      if (iupStrEqualNoCase(alternate_color, "YES"))
      {
        if (lin % 2 == 0)
          bgcolor = iupAttribGetStr(ih, "EVENROWCOLOR");
        else
          bgcolor = iupAttribGetStr(ih, "ODDROWCOLOR");
      }
    }
  }

  int cells_selected = !is_header && iupTableCellsIsSelected(ih, lin, col);
  if (cells_selected)
    bgcolor = iupTableCellsBgColor();

  if (is_focused_row)
    cell_bg = mot_data->select_bg_pixel;
  else if (is_header)
    cell_bg = mot_data->header_bg_pixel;
  else if (bgcolor && *bgcolor)
    cell_bg = iupmotColorGetPixelStr(bgcolor);
  else
    cell_bg = mot_data->bg_pixel;

  XSetForeground(display, mot_data->gc, cell_bg);
  XFillRectangle(display, window, mot_data->gc, x, y, w, h);

  if (mot_data->show_grid)
  {
    XSetForeground(display, mot_data->gc, mot_data->grid_pixel);
    XDrawLine(display, window, mot_data->gc, x, y + h - 1, x + w, y + h - 1); /* Bottom */
    XDrawLine(display, window, mot_data->gc, x + w - 1, y, x + w - 1, y + h); /* Right */
  }

  if (!is_header && ih->data->show_image)
    img_offset = motTableDrawCellImage(ih, window, lin, col, x, y, w, h, cell_bg);

  if (text && text[0])
  {
    char* fgcolor = NULL;
    ImotTableFont font;

    /* Get foreground color with hierarchy: per-cell > per-column > per-row > default */
    if (!is_header)
    {
      fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);
      if (!fgcolor)
        fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);
      if (!fgcolor)
        fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);
      if (cells_selected)
        fgcolor = iupTableCellsFgColor();
    }

    motTableGetCellFont(ih, is_header ? 0 : lin, col, &font);

    unsigned char tr, tg, tb;
    if (fgcolor && *fgcolor)
      iupStrToRGB(fgcolor, &tr, &tg, &tb);
    else
      iupmotColorGetRGB(mot_data->fg_pixel, &tr, &tg, &tb);

    if (!XtIsSensitive(ih->handle))
    {
      unsigned char br, bgc, bb;
      iupmotColorGetRGB(mot_data->bg_pixel, &br, &bgc, &bb);
      tr = (tr + br) / 2; tg = (tg + bgc) / 2; tb = (tb + bb) / 2;
    }

    XSetForeground(display, mot_data->gc, iupmotColorGetPixel(tr, tg, tb));

    text_len = strlen(text);
    text_width = motTableGetTextWidth(&font, text, text_len);

    char align_attr[64];
    char* align_str = NULL;
    snprintf(align_attr, sizeof(align_attr), "ALIGNMENT%d", col);
    align_str = iupAttribGet(ih, align_attr);

    if (align_str && (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT")))
    {
      text_x = x + w - text_width - MOT_TABLE_CELL_PADDING;
    }
    else if (align_str && (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER")))
    {
      text_x = x + img_offset + (w - img_offset - text_width) / 2;
    }
    else
    {
      text_x = x + img_offset + MOT_TABLE_CELL_PADDING;
    }

#ifdef IUP_USE_XFT
    if (font.xft_font)
      text_y = y + h / 2 + font.xft_font->ascent / 2;
    else
#endif
      text_y = y + h / 2 + (font.font_struct ? font.font_struct->ascent / 2 : 6);

#ifdef IUP_USE_XFT
    if (mot_data->xft_draw && font.xft_font)
    {
      XftColor xft_color;
      XRenderColor render_color;

      render_color.red = tr * 257;
      render_color.green = tg * 257;
      render_color.blue = tb * 257;
      render_color.alpha = 0xffff;

      XftColorAllocValue(display, DefaultVisual(display, iupmot_screen), DefaultColormap(display, iupmot_screen), &render_color, &xft_color);
      XftDrawStringUtf8(mot_data->xft_draw, &xft_color, font.xft_font, text_x, text_y, (FcChar8*)text, text_len);
      XftColorFree(display, DefaultVisual(display, iupmot_screen), DefaultColormap(display, iupmot_screen), &xft_color);
    }
    else
#endif
    {
      if (font.font_struct)
        XSetFont(display, mot_data->gc, font.font_struct->fid);
      XDrawString(display, window, mot_data->gc, text_x, text_y, text, text_len);
    }

    if (is_header && mot_data->sort_column == col && mot_data->sort_signs)
    {
      int arrow_x;
      int arrow_y = y + (h - 6) / 2;

      if (align_str && (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT")))
        arrow_x = x + MOT_TABLE_CELL_PADDING;
      else
        arrow_x = x + w - 12;

      if (mot_data->sort_signs[col-1] == 1)  /* Ascending */
      {
        XSetForeground(display, mot_data->gc, mot_data->fg_pixel);
        motTableDrawSortArrow(display, window, mot_data->gc, arrow_x, arrow_y, 1);
      }
      else if (mot_data->sort_signs[col-1] == -1)  /* Descending */
      {
        XSetForeground(display, mot_data->gc, mot_data->fg_pixel);
        motTableDrawSortArrow(display, window, mot_data->gc, arrow_x, arrow_y, 0);
      }
    }
  }

  if (is_focused_cell && iupAttribGetBoolean(ih, "FOCUSRECT"))
  {
    /* Use XOR mode for focus rectangle, always creates contrast by inverting pixels */
    XSetFunction(display, mot_data->gc, GXxor);
    XSetForeground(display, mot_data->gc, mot_data->fg_pixel ^ mot_data->bg_pixel);

    char dash_list[] = {2, 2};  /* 2 pixels on, 2 pixels off */
    XSetLineAttributes(display, mot_data->gc, 1, LineOnOffDash, CapButt, JoinMiter);
    XSetDashes(display, mot_data->gc, 0, dash_list, 2);

    XDrawRectangle(display, window, mot_data->gc, x + 1, y + 1, w - 3, h - 3);

    XSetFunction(display, mot_data->gc, GXcopy);
    XSetLineAttributes(display, mot_data->gc, 1, LineSolid, CapButt, JoinMiter);
  }
}

static void motTableAutoSizeColumns(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int c, lin1;
  int max_rows_to_check = (ih->data->num_lin > 100) ? 100 : ih->data->num_lin;  /* Limit for performance */

  if (ih->data->show_image)
  {
    int row_height = MOT_TABLE_DEF_ROW_HEIGHT;
    for (lin1 = 1; !ih->data->fit_image && lin1 <= max_rows_to_check; lin1++)
    {
      for (c = 1; c <= ih->data->num_col; c++)
      {
        char* name = motTableGetCellImage(ih, lin1, c);
        int img_w, img_h;
        if (name && motTableGetCellImageSize(ih, name, &img_w, &img_h) && img_h + 4 > row_height)
          row_height = img_h + 4;
      }
    }
    mot_data->row_height = row_height;
  }

  for (c = 0; c < ih->data->num_col; c++)
  {
    int max_width = MOT_TABLE_DEF_COL_WIDTH;
    int title_width, cell_width;

    if (mot_data->col_width_set[c])
    {
      continue;
    }

    if (mot_data->col_titles[c])
    {
      ImotTableFont font;
      motTableGetCellFont(ih, 0, c + 1, &font);
      title_width = motTableGetTextWidth(&font, mot_data->col_titles[c], strlen(mot_data->col_titles[c]));
      title_width += 20;  /* Add padding for sort indicator space */
      if (title_width > max_width)
        max_width = title_width;
    }

    for (lin1 = 0; lin1 < max_rows_to_check; lin1++)
    {
      const char* cell_value = IupGetAttributeId2(ih, "", lin1 + 1, c + 1);
      cell_width = 16;  /* Add padding (8px left + 8px right) */
      if (cell_value && cell_value[0])
      {
        ImotTableFont font;
        motTableGetCellFont(ih, lin1 + 1, c + 1, &font);
        cell_width += motTableGetTextWidth(&font, cell_value, strlen(cell_value));
      }
      if (ih->data->show_image)
      {
        char* name = motTableGetCellImage(ih, lin1 + 1, c + 1);
        int img_w, img_h;
        if (name && motTableGetCellImageSize(ih, name, &img_w, &img_h))
          cell_width += img_w + 4;
      }
      if (cell_width > max_width)
        max_width = cell_width;
    }

    mot_data->col_natural_widths[c] = max_width;
  }
}

static void motTableAutoSizeTimeout(XtPointer client_data, XtIntervalId* id)
{
  Ihandle* ih = (Ihandle*)client_data;
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  (void)id;

  if (!mot_data)
    return;

  mot_data->autosize_timer = 0;
  motTableAutoSizeColumns(ih);
  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

static void motTableQueueAutoSize(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (mot_data && !mot_data->autosize_timer)
    mot_data->autosize_timer = XtAppAddTimeOut(iupmot_appcontext, 0, motTableAutoSizeTimeout, (XtPointer)ih);
}

static void motTableDrawTable(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  Display* display = iupmot_display;
  Window window = XtWindow(mot_data->drawing_area);
  Dimension width, height;
  int lin, col;

  if (!window)
    return;


  XtVaGetValues(mot_data->drawing_area, XmNwidth, &width, XmNheight, &height, NULL);

  /* Clear background */
  XSetForeground(display, mot_data->gc, mot_data->bg_pixel);
  XFillRectangle(display, window, mot_data->gc, 0, 0, width, height);

  for (col = 1; col <= ih->data->num_col; col++)
  {
    motTableDrawCell(ih, 0, col, 1);
  }

  {
    int first_visible_row, last_visible_row;
    int visible_height = height - mot_data->header_height;

    first_visible_row = (mot_data->scroll_y / mot_data->row_height) + 1;
    last_visible_row = ((mot_data->scroll_y + visible_height) / mot_data->row_height) + 1;

    if (first_visible_row < 1) first_visible_row = 1;
    if (last_visible_row > ih->data->num_lin) last_visible_row = ih->data->num_lin;

    for (lin = first_visible_row; lin <= last_visible_row; lin++)
    {
      for (col = 1; col <= ih->data->num_col; col++)
      {
        motTableDrawCell(ih, lin, col, 0);
      }
    }

    {
      int content_right = -mot_data->scroll_x;
      for (col = 0; col < ih->data->num_col; col++)
        content_right += mot_data->col_widths[col];

      if (content_right < width)
      {
        int fw = width - content_right;

        XSetForeground(display, mot_data->gc, mot_data->header_bg_pixel);
        XFillRectangle(display, window, mot_data->gc, content_right, 0, fw, mot_data->header_height);
        if (mot_data->show_grid)
        {
          XSetForeground(display, mot_data->gc, mot_data->grid_pixel);
          XDrawLine(display, window, mot_data->gc, content_right, mot_data->header_height - 1, width, mot_data->header_height - 1);
        }

        for (lin = first_visible_row; lin <= last_visible_row; lin++)
        {
          int ry = mot_data->header_height + (lin - 1) * mot_data->row_height - mot_data->scroll_y;
          Pixel bg = mot_data->bg_pixel;

          if (motTableRowSelected(ih, lin))
            bg = mot_data->select_bg_pixel;
          else
          {
            char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);
            if (!bgcolor && iupStrEqualNoCase(iupAttribGet(ih, "ALTERNATECOLOR"), "YES"))
              bgcolor = (lin % 2 == 0) ? iupAttribGetStr(ih, "EVENROWCOLOR") : iupAttribGetStr(ih, "ODDROWCOLOR");
            if (bgcolor && *bgcolor)
              bg = iupmotColorGetPixelStr(bgcolor);
          }

          XSetForeground(display, mot_data->gc, bg);
          XFillRectangle(display, window, mot_data->gc, content_right, ry, fw, mot_data->row_height);
          if (mot_data->show_grid)
          {
            XSetForeground(display, mot_data->gc, mot_data->grid_pixel);
            XDrawLine(display, window, mot_data->gc, content_right, ry + mot_data->row_height - 1, width, ry + mot_data->row_height - 1);
          }
        }
      }
    }
  }

  if (mot_data->row_dragging && mot_data->drag_target_row >= 0 &&
      mot_data->drag_target_row != mot_data->drag_source_row - 1 &&
      mot_data->drag_target_row != mot_data->drag_source_row)
  {
    int ly = mot_data->header_height + mot_data->drag_target_row * mot_data->row_height - mot_data->scroll_y;
    XSetForeground(display, mot_data->gc, iupmotColorGetPixel(0, 120, 215));
    XSetLineAttributes(display, mot_data->gc, 2, LineSolid, CapButt, JoinMiter);
    XDrawLine(display, window, mot_data->gc, 0, ly, width, ly);
    XSetLineAttributes(display, mot_data->gc, 1, LineSolid, CapButt, JoinMiter);
  }

  if (mot_data->col_dragging && mot_data->drag_target_col >= 0 &&
      mot_data->drag_target_col != mot_data->drag_source_col - 1 &&
      mot_data->drag_target_col != mot_data->drag_source_col)
  {
    int lx = -mot_data->scroll_x;
    for (col = 0; col < mot_data->drag_target_col; col++)
      lx += mot_data->col_widths[col];
    XSetForeground(display, mot_data->gc, iupmotColorGetPixel(0, 120, 215));
    XSetLineAttributes(display, mot_data->gc, 2, LineSolid, CapButt, JoinMiter);
    XDrawLine(display, window, mot_data->gc, lx, 0, lx, height);
    XSetLineAttributes(display, mot_data->gc, 1, LineSolid, CapButt, JoinMiter);
  }
}

static void motTableRedraw(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (mot_data && mot_data->drawing_area && XtWindow(mot_data->drawing_area))
  {
    motTableDrawTable(ih);
  }
}

/* ========================================================================= */
/* Cell Editing                                                              */
/* ========================================================================= */

static void motTableStartCellEdit(Ihandle* ih, int lin, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int x, y, w, h;
  const char* value;

  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    int ret = editbegin_cb(ih, lin, col);
    if (ret == IUP_IGNORE)
      return;
  }

  if (mot_data->edit_lin != 0)
    motTableEndCellEdit(ih, 1);

  if (!mot_data->edit_text)
  {
    int num_args = 0;
    Arg args[15];
    XmFontList fontlist_for_edit;

    iupMOT_SETARG(args, num_args, XmNx, 0);
    iupMOT_SETARG(args, num_args, XmNy, 0);
    iupMOT_SETARG(args, num_args, XmNwidth, 100);
    iupMOT_SETARG(args, num_args, XmNheight, 20);
    iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);
    iupMOT_SETARG(args, num_args, XmNmarginWidth, 2);
    iupMOT_SETARG(args, num_args, XmNhighlightThickness, 0);

    fontlist_for_edit = (XmFontList)iupmotGetFontListAttrib(ih);
    if (fontlist_for_edit)
    {
      iupMOT_SETARG(args, num_args, XmNrenderTable, fontlist_for_edit);
      iupMOT_SETARG(args, num_args, XmNfontList, fontlist_for_edit);
    }

    mot_data->edit_text = XmCreateText(mot_data->container, "edit_text", args, num_args);

    XtAddEventHandler(mot_data->edit_text, KeyPressMask, False, (XtEventHandler)motTableEditKeyPressCallback, (XtPointer)ih);
  }

  motTableCellToPixel(ih, lin, col, &x, &y, &w, &h);
  XtVaSetValues(mot_data->edit_text, XmNx, x, XmNy, y, XmNwidth, w, XmNheight, h, NULL);

  value = motTableGetCellValueInternal(ih, lin, col);
  XmTextSetString(mot_data->edit_text, (char*)(value ? value : ""));

  XtManageChild(mot_data->edit_text);
  XmProcessTraversal(mot_data->edit_text, XmTRAVERSE_CURRENT);
  XmTextSetSelection(mot_data->edit_text, 0, XmTextGetLastPosition(mot_data->edit_text), CurrentTime);

  mot_data->edit_lin = lin;
  mot_data->edit_col = col;
}

static void motTableEditKeyPressCallback(Widget w, XtPointer client_data, XEvent* event, Boolean* cont)
{
  Ihandle* ih = (Ihandle*)client_data;
  KeySym keysym;

  (void)w;
  (void)cont;

  if (event->type != KeyPress)
    return;

  keysym = XkbKeycodeToKeysym(iupmot_display, ((XKeyEvent*)event)->keycode, 0, 0);

  if (keysym == XK_Return || keysym == XK_KP_Enter)
  {
    motTableEndCellEdit(ih, 1);
    *cont = False;  /* Stop event propagation */
  }
  else if (keysym == XK_Escape)
  {
    motTableEndCellEdit(ih, 0);
    *cont = False;  /* Stop event propagation */
  }
}

static void motTableEndCellEdit(Ihandle* ih, int apply)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int lin = mot_data->edit_lin;
  int col = mot_data->edit_col;

  if (lin == 0 || !mot_data->edit_text)
    return;

  char* text = XmTextGetString(mot_data->edit_text);

  IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb)
  {
    int ret = editend_cb(ih, lin, col, text, apply ? 1 : 0);
    if (ret == IUP_IGNORE)
      apply = 0;
  }

  if (apply)
  {
    /* Get old value for comparison, must copy before it gets modified */
    char* old_text_ptr = iupdrvTableGetCellValue(ih, lin, col);
    char* old_text = old_text_ptr ? iupStrDup(old_text_ptr) : NULL;

    motTableSetCellValueInternal(ih, lin, col, text);

    IFniis value_cb = (IFniis)IupGetCallback(ih, "VALUE_CB");
    if (value_cb)
      value_cb(ih, lin, col, text);

    int text_changed = 0;
    if (!old_text && text && *text)
      text_changed = 1;
    else if (old_text && !text)
      text_changed = 1;
    else if (old_text && text && strcmp(old_text, text) != 0)
      text_changed = 1;

    if (text_changed)
    {
      IFnii valuechanged_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
      if (valuechanged_cb)
        valuechanged_cb(ih, lin, col);
    }

    if (old_text)
      free(old_text);
  }

  XtFree(text);

  XtUnmanageChild(mot_data->edit_text);
  mot_data->edit_lin = 0;
  mot_data->edit_col = 0;

  XmProcessTraversal(mot_data->drawing_area, XmTRAVERSE_CURRENT);

  motTableRedraw(ih);
}

/* ========================================================================= */
/* Event Callbacks                                                           */
/* ========================================================================= */

static void motTableExposeCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle* ih = (Ihandle*)client_data;
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  (void)w;
  (void)call_data;

  if (mot_data && mot_data->pending_scroll_lin > 0)
  {
    int lin = mot_data->pending_scroll_lin;
    mot_data->pending_scroll_lin = 0;
    iupdrvTableScrollToCell(ih, lin, mot_data->pending_scroll_col);
    return;
  }

  motTableDrawTable(ih);
}

static void motTableResizeCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle* ih = (Ihandle*)client_data;
  (void)w;
  (void)call_data;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

static void motTableInputCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle* ih = (Ihandle*)client_data;
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  XEvent* event = ((XmDrawingAreaCallbackStruct*)call_data)->event;
  int lin, col;

  if (event->type == ButtonPress)
  {
    XButtonEvent* button_event = (XButtonEvent*)event;

    /* Handle mouse wheel scrolling (Button4 = scroll up, Button5 = scroll down) */
    if (button_event->button == Button4 || button_event->button == Button5)
    {
      int scroll_amount = mot_data->row_height * 3;
      int new_scroll_y = mot_data->scroll_y;

      if (button_event->button == Button4)
        new_scroll_y -= scroll_amount;
      else
        new_scroll_y += scroll_amount;

      if (new_scroll_y < 0)
        new_scroll_y = 0;

      mot_data->scroll_y = new_scroll_y;

      motTableUpdateScrollbars(ih);
      motTableRedraw(ih);
      return;
    }

    if (button_event->button == Button1 && ih->data->user_resize)
    {
      int divider = motTableHeaderDivider(ih, button_event->x, button_event->y);
      if (divider)
      {
        mot_data->resize_col = divider;
        mot_data->resize_start_width = mot_data->col_widths[divider - 1];
        mot_data->drag_start_x = button_event->x;
        return;
      }
    }

    motTablePixelToCell(ih, button_event->x, button_event->y, &lin, &col);

    if (lin == 0 && col > 0)
    {
      if (ih->data->allow_reorder && button_event->button == Button1)
      {
        mot_data->drag_source_col = col;
        mot_data->drag_target_col = -1;
        mot_data->col_dragging = 0;
        mot_data->drag_start_x = button_event->x;
        mot_data->drag_start_y = button_event->y;
      }
      else if (ih->data->sortable)
        motTableHeaderSort(ih, col);
    }
    else if (lin > 0 && col > 0)
    {
      IFniis cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
      if (cb)
      {
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        int ret;
        iupmotButtonKeySetStatus(button_event->state, button_event->button, status, 0);
        ret = cb(ih, lin, col, status);
        if (!iupObjectCheck(ih))
          return;
        if (ret == IUP_CLOSE)
          IupExitLoop();
        else if (ret == IUP_IGNORE)
          return;
      }

      if (iupTableCellsMode(ih) && button_event->button == Button1 && (button_event->state & ShiftMask))
      {
        iupTableCellsExtendTo(ih, lin, col);
        return;
      }

      if (mot_data->edit_lin != 0)
        motTableEndCellEdit(ih, 1);

      mot_data->current_row = lin;
      mot_data->current_col = col;
      if (button_event->button == Button3)
        iupAttribSet(ih, "_IUPTABLE_CELLS_KEEP", "1");
      iupTableCellsCollapse(ih);
      iupAttribSet(ih, "_IUPTABLE_CELLS_KEEP", NULL);

      char* selmode = iupAttribGetStr(ih, "SELECTIONMODE");

      if (!iupStrEqualNoCase(selmode, "NONE") && !iupTableCellsMode(ih))
      {
        if (iupStrEqualNoCase(selmode, "MULTIPLE") && (button_event->state & ShiftMask) && mot_data->anchor_row > 0)
          motTableSelectRange(ih, mot_data->anchor_row, lin);
        else if (iupStrEqualNoCase(selmode, "MULTIPLE") && (button_event->state & ControlMask))
        {
          motTableSelectRow(ih, lin, !motTableRowSelected(ih, lin), 0);
          mot_data->anchor_row = lin;
        }
        else
        {
          motTableSelectRow(ih, lin, 1, 1);
          mot_data->anchor_row = lin;
        }
      }

      if (ih->data->show_dragdrop && button_event->button == Button1)
      {
        mot_data->drag_source_row = lin;
        mot_data->drag_target_row = -1;
        mot_data->row_dragging = 0;
        mot_data->drag_start_x = button_event->x;
        mot_data->drag_start_y = button_event->y;
      }

      if (button_event->button == Button3)
      {
        IFnii rcb = (IFnii)IupGetCallback(ih, "RIGHTCLICK_CB");
        if (rcb)
          rcb(ih, lin, col);
      }

      IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
      if (enteritem_cb)
        enteritem_cb(ih, lin, col);

      iupTableCallMultiSelectionCb(ih);

      motTableRedraw(ih);

      if (button_event->type == ButtonPress && button_event->button == Button1)
      {
        static Time last_click_time = 0;
        static int last_click_lin = 0, last_click_col = 0;

        if ((button_event->time - last_click_time) < 300 && lin == last_click_lin && col == last_click_col)
        {
          char name[50];
          char* editable;
          snprintf(name, sizeof(name), "EDITABLE%d", col);
          editable = iupAttribGetStr(ih, name);
          if (!editable)
            editable = iupAttribGetStr(ih, "EDITABLE");

          if (iupStrBoolean(editable))
            motTableStartCellEdit(ih, lin, col);
        }

        last_click_time = button_event->time;
        last_click_lin = lin;
        last_click_col = col;
      }
    }
  }
  else if (event->type == ButtonRelease)
  {
    XButtonEvent* button_event = (XButtonEvent*)event;

    if (button_event->button == Button1 && mot_data->resize_col)
    {
      mot_data->resize_col = 0;
      return;
    }

    if (button_event->button == Button1 && mot_data->drag_source_col)
    {
      int src = mot_data->drag_source_col;
      int tgt = mot_data->drag_target_col;
      int was_dragging = mot_data->col_dragging;

      mot_data->drag_source_col = 0;
      mot_data->drag_target_col = -1;
      mot_data->col_dragging = 0;

      if (was_dragging && tgt >= 0 && tgt != src - 1 && tgt != src)
      {
        int dst = (tgt < src - 1) ? tgt + 1 : tgt;
        IFnii reorder_cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
        int ret = reorder_cb ? reorder_cb(ih, src, dst) : IUP_DEFAULT;
        if (!iupObjectCheck(ih))
          return;
        if (ret != IUP_IGNORE)
          motTableMoveCol(ih, src, dst);
        if (ret == IUP_CLOSE)
          IupExitLoop();
      }
      else if (!was_dragging && ih->data->sortable)
        motTableHeaderSort(ih, src);

      motTableRedraw(ih);
      return;
    }

    if (button_event->button == Button1 && mot_data->drag_source_row >= 1)
    {
      int src = mot_data->drag_source_row;
      int tgt = mot_data->drag_target_row;
      int was_dragging = mot_data->row_dragging;

      mot_data->drag_source_row = 0;
      mot_data->drag_target_row = -1;
      mot_data->row_dragging = 0;

      if (was_dragging && tgt >= 0)
      {
        int is_ctrl = 0;
        if (iupTableCallDragDropCb(ih, src - 1, tgt, &is_ctrl) == IUP_CONTINUE)
        {
          int num_lin = ih->data->num_lin;
          int to = (tgt > src - 1) ? tgt - 1 : tgt;
          if (to >= num_lin) to = num_lin - 1;
          if (to < 0) to = 0;
          motTableMoveRow(ih, src, to + 1);
        }
        else
          motTableRedraw(ih);
      }
    }
  }

  (void)w;
}

static void motTableKeyPressCallback(Widget w, XtPointer client_data, XEvent* event, Boolean* cont)
{
  Ihandle* ih = (Ihandle*)client_data;
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  KeySym keysym = XLookupKeysym((XKeyEvent*)event, 0);
  int redraw = 0;

  *cont = True;

  if (mot_data->edit_lin != 0)
  {
    if (keysym == XK_Escape)
    {
      motTableEndCellEdit(ih, 0);
      return;
    }
    else if (keysym == XK_Return)
    {
      motTableEndCellEdit(ih, 1);
      return;
    }
    return;
  }

  if (iupTableCellsMode(ih))
  {
    unsigned int state = ((XKeyEvent*)event)->state;
    if (state & ShiftMask)
    {
      int dlin = 0, dcol = 0;
      if (keysym == XK_Up) dlin = -1;
      else if (keysym == XK_Down) dlin = 1;
      else if (keysym == XK_Left) dcol = -1;
      else if (keysym == XK_Right) dcol = 1;
      if (dlin || dcol)
      {
        iupTableCellsExtendBy(ih, dlin, dcol);
        return;
      }
    }
    else if ((keysym == XK_a || keysym == XK_A) && (state & ControlMask))
    {
      iupTableCellsSelectAll(ih);
      return;
    }
  }

  if (keysym == XK_Up && mot_data->current_row > 1)
  {
    mot_data->current_row--;
    iupTableCellsCollapse(ih);
    redraw = 1;

    IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enteritem_cb)
      enteritem_cb(ih, mot_data->current_row, mot_data->current_col);
  }
  else if (keysym == XK_Down && mot_data->current_row < ih->data->num_lin)
  {
    mot_data->current_row++;
    iupTableCellsCollapse(ih);
    redraw = 1;

    IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enteritem_cb)
      enteritem_cb(ih, mot_data->current_row, mot_data->current_col);
  }
  else if (keysym == XK_Left && mot_data->current_col > 1)
  {
    mot_data->current_col--;
    iupTableCellsCollapse(ih);
    redraw = 1;

    IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enteritem_cb)
      enteritem_cb(ih, mot_data->current_row, mot_data->current_col);
  }
  else if (keysym == XK_Right && mot_data->current_col < ih->data->num_col)
  {
    mot_data->current_col++;
    iupTableCellsCollapse(ih);
    redraw = 1;

    IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enteritem_cb)
      enteritem_cb(ih, mot_data->current_row, mot_data->current_col);
  }
  else if (keysym == XK_Return)
  {
    if (mot_data->current_row > 0 && mot_data->current_col > 0)
    {
      char name[50];
      char* editable;
      snprintf(name, sizeof(name), "EDITABLE%d", mot_data->current_col);
      editable = iupAttribGetStr(ih, name);
      if (!editable)
        editable = iupAttribGetStr(ih, "EDITABLE");

      if (iupStrBoolean(editable))
        motTableStartCellEdit(ih, mot_data->current_row, mot_data->current_col);
    }
  }
  else if ((keysym == XK_c || keysym == XK_C) && (((XKeyEvent*)event)->state & ControlMask))
  {
    if (mot_data->current_row > 0 && mot_data->current_col > 0)
    {
      char* value = motTableGetCellValueInternal(ih, mot_data->current_row, mot_data->current_col);
      if (value)
      {
        IupSetGlobal("CLIPBOARD", value);
      }
    }
  }
  else if ((keysym == XK_v || keysym == XK_V) && (((XKeyEvent*)event)->state & ControlMask))
  {
    if (mot_data->current_row > 0 && mot_data->current_col > 0)
    {
      char name[50];
      char* editable;
      snprintf(name, sizeof(name), "EDITABLE%d", mot_data->current_col);
      editable = iupAttribGetStr(ih, name);
      if (!editable)
        editable = iupAttribGetStr(ih, "EDITABLE");

      if (iupStrBoolean(editable))
      {
        char* text = IupGetGlobal("CLIPBOARD");
        if (text && *text)
        {
          /* Get old value for comparison - must copy before it gets modified */
          char* old_text_ptr = iupdrvTableGetCellValue(ih, mot_data->current_row, mot_data->current_col);
          char* old_text = old_text_ptr ? iupStrDup(old_text_ptr) : NULL;

          motTableSetCellValueInternal(ih, mot_data->current_row, mot_data->current_col, text);

          IFniis value_cb = (IFniis)IupGetCallback(ih, "VALUE_CB");
          if (value_cb)
            value_cb(ih, mot_data->current_row, mot_data->current_col, text);

          int text_changed = 0;
          if (!old_text && text && *text)
            text_changed = 1;
          else if (old_text && !text)
            text_changed = 1;
          else if (old_text && text && strcmp(old_text, text) != 0)
            text_changed = 1;

          if (text_changed)
          {
            IFnii valuechanged_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
            if (valuechanged_cb)
              valuechanged_cb(ih, mot_data->current_row, mot_data->current_col);
          }

          if (old_text)
            free(old_text);

          redraw = 1;
        }
      }
    }
  }

  if (redraw)
  {
    if (!iupStrEqualNoCase(iupAttribGetStr(ih, "SELECTIONMODE"), "NONE") && !iupTableCellsMode(ih))
    {
      motTableSelectRow(ih, mot_data->current_row, 1, 1);
      mot_data->anchor_row = mot_data->current_row;
    }

    iupTableCallMultiSelectionCb(ih);

    motTableRedraw(ih);
  }

  (void)w;
}

/* ========================================================================= */
/* Scrollbar Callbacks                                                       */
/* ========================================================================= */

static void motTableScrollCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle* ih = (Ihandle*)client_data;
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  XmScrollBarCallbackStruct* cbs = (XmScrollBarCallbackStruct*)call_data;

  if (w == mot_data->sb_horiz)
  {
    mot_data->scroll_x = cbs->value;
  }
  else if (w == mot_data->sb_vert)
  {
    mot_data->scroll_y = cbs->value;
  }

  motTableRedraw(ih);
}

static void motTableUpdateScrollbars(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  Dimension width, height;
  int total_width = 0, total_height, c;
  int page_width, page_height, max_scroll_x, max_scroll_y;
  int natural_total_width = 0;

  if (!mot_data->sb_horiz || !mot_data->sb_vert)
    return;

  XtVaGetValues(mot_data->drawing_area, XmNwidth, &width, XmNheight, &height, NULL);

  for (c = 0; c < ih->data->num_col; c++)
  {
    natural_total_width += mot_data->col_natural_widths[c];
  }

  for (c = 0; c < ih->data->num_col; c++)
    mot_data->col_widths[c] = mot_data->col_natural_widths[c];
  total_width = natural_total_width;

  if (ih->data->num_col > 0 && !mot_data->col_width_set[ih->data->num_col - 1] && ih->data->stretch_last)
  {
    int last_col = ih->data->num_col - 1;
    int other_cols_width = natural_total_width - mot_data->col_natural_widths[last_col];
    int available_for_last = width - other_cols_width;

    if (available_for_last < mot_data->col_natural_widths[last_col])
      available_for_last = mot_data->col_natural_widths[last_col];

    mot_data->col_widths[last_col] = available_for_last;
    total_width = other_cols_width + available_for_last;
  }

  total_height = mot_data->header_height + ih->data->num_lin * mot_data->row_height;

  page_width = width;
  max_scroll_x = total_width - page_width;
  if (max_scroll_x < 0) max_scroll_x = 0;

  /* Ensure sliderSize doesn't exceed maximum - minimum */
  if (page_width > total_width)
    page_width = total_width;

  if (mot_data->scroll_x > max_scroll_x)
    mot_data->scroll_x = max_scroll_x;

  if (max_scroll_x > 0)
  {
    XtVaSetValues(mot_data->sb_horiz,
                  XmNminimum, 0,
                  XmNmaximum, total_width,
                  XmNvalue, mot_data->scroll_x,
                  XmNsliderSize, page_width,
                  XmNincrement, 10,
                  XmNpageIncrement, page_width,
                  NULL);
    XtManageChild(mot_data->sb_horiz);
  }
  else
  {
    mot_data->scroll_x = 0;
    XtUnmanageChild(mot_data->sb_horiz);
  }

  page_height = height;
  max_scroll_y = total_height - page_height;
  if (max_scroll_y < 0) max_scroll_y = 0;

  /* Ensure sliderSize doesn't exceed maximum - minimum */
  if (page_height > total_height)
    page_height = total_height;

  if (mot_data->scroll_y > max_scroll_y)
    mot_data->scroll_y = max_scroll_y;

  if (max_scroll_y > 0)
  {
    XtVaSetValues(mot_data->sb_vert,
                  XmNminimum, 0,
                  XmNmaximum, total_height,
                  XmNvalue, mot_data->scroll_y,
                  XmNsliderSize, page_height,
                  XmNincrement, mot_data->row_height,
                  XmNpageIncrement, page_height,
                  NULL);
    XtManageChild(mot_data->sb_vert);
  }
  else
  {
    mot_data->scroll_y = 0;
    XtUnmanageChild(mot_data->sb_vert);
  }
}

/* ========================================================================= */
/* Widget Creation - MapMethod                                              */
/* ========================================================================= */

static int motTableMapMethod(Ihandle* ih)
{
  ImotTableData* mot_data;
  Widget parent = iupChildTreeGetNativeParentHandle(ih);
  char* child_id = iupDialogGetChildIdStr(ih);
  int num_args = 0;
  Arg args[30];
  int i, c;
  XGCValues gcvalues;

  if (!parent)
    return IUP_ERROR;

  mot_data = (ImotTableData*)calloc(1, sizeof(ImotTableData));
  if (!mot_data)
    return IUP_ERROR;

  ih->data->native_data = mot_data;

  mot_data->row_height = MOT_TABLE_DEF_ROW_HEIGHT;
  mot_data->header_height = MOT_TABLE_HEADER_HEIGHT;
  mot_data->show_grid = iupAttribGetBoolean(ih, "SHOWGRID");

  mot_data->col_widths = (int*)calloc(ih->data->num_col, sizeof(int));
  mot_data->col_natural_widths = (int*)calloc(ih->data->num_col, sizeof(int));
  mot_data->col_width_set = (int*)calloc(ih->data->num_col, sizeof(int));

  for (c = 0; c < ih->data->num_col; c++)
  {
    char name[50];
    snprintf(name, sizeof(name), "RASTERWIDTH%d", c + 1);
    char* width_str = iupAttribGet(ih, name);
    if (!width_str)
    {
      snprintf(name, sizeof(name), "WIDTH%d", c + 1);
      width_str = iupAttribGet(ih, name);
    }

    int col_width = 0;
    if (width_str && iupStrToInt(width_str, &col_width) && col_width > 0)
    {
      mot_data->col_widths[c] = col_width;
      mot_data->col_natural_widths[c] = col_width;
      mot_data->col_width_set[c] = 1;
    }
    else
    {
      mot_data->col_widths[c] = MOT_TABLE_DEF_COL_WIDTH;
      mot_data->col_natural_widths[c] = MOT_TABLE_DEF_COL_WIDTH;
      mot_data->col_width_set[c] = 0;
    }
  }

  mot_data->sort_signs = (char*)calloc(ih->data->num_col, sizeof(char));
  mot_data->sort_column = 0;

  if (!iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    mot_data->cells = (ImotTableCell**)calloc(ih->data->num_lin, sizeof(ImotTableCell*));
    for (i = 0; i < ih->data->num_lin; i++)
    {
      mot_data->cells[i] = (ImotTableCell*)calloc(ih->data->num_col, sizeof(ImotTableCell));
    }
  }
  else
  {
    /* Virtual mode: no cell storage needed, VALUE_CB provides data on demand */
    mot_data->cells = NULL;
  }

  mot_data->col_titles = (char**)calloc(ih->data->num_col, sizeof(char*));
  for (c = 0; c < ih->data->num_col; c++)
  {
    char default_title[32];
    snprintf(default_title, sizeof(default_title), "Col %d", c + 1);
    mot_data->col_titles[c] = iupStrDup(default_title);
  }

  num_args = 0;
  iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);
  iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);
  iupMOT_SETARG(args, num_args, XmNresizePolicy, XmRESIZE_NONE);

  mot_data->container = XtCreateManagedWidget(child_id, xmBulletinBoardWidgetClass, parent, args, num_args);

  if (!mot_data->container)
  {
    free(mot_data);
    return IUP_ERROR;
  }

  ih->serial = iupDialogGetChildId(ih);

  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)mot_data->container);

  num_args = 0;
  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNshadowThickness, 0);
  iupMOT_SETARG(args, num_args, XmNresizePolicy, XmRESIZE_NONE);

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    iupMOT_SETARG(args, num_args, XmNtraversalOn, True);
    iupMOT_SETARG(args, num_args, XmNnavigationType, XmTAB_GROUP);
  }

  mot_data->drawing_area = XtCreateManagedWidget("draw_area", xmDrawingAreaWidgetClass, mot_data->container, args, num_args);

  if (!mot_data->drawing_area)
  {
    XtDestroyWidget(mot_data->container);
    free(mot_data);
    return IUP_ERROR;
  }

  ih->handle = mot_data->drawing_area;

  mot_data->sb_horiz = XtVaCreateManagedWidget("sb_horiz", xmScrollBarWidgetClass, mot_data->container, XmNorientation, XmHORIZONTAL, NULL);

  XtAddCallback(mot_data->sb_horiz, XmNvalueChangedCallback, motTableScrollCallback, (XtPointer)ih);
  XtAddCallback(mot_data->sb_horiz, XmNdragCallback, motTableScrollCallback, (XtPointer)ih);

  mot_data->sb_vert = XtVaCreateManagedWidget("sb_vert", xmScrollBarWidgetClass, mot_data->container, XmNorientation, XmVERTICAL, NULL);

  XtAddCallback(mot_data->sb_vert, XmNvalueChangedCallback, motTableScrollCallback, (XtPointer)ih);
  XtAddCallback(mot_data->sb_vert, XmNdragCallback, motTableScrollCallback, (XtPointer)ih);

  XtAddCallback(mot_data->drawing_area, XmNexposeCallback, motTableExposeCallback, (XtPointer)ih);
  XtAddCallback(mot_data->drawing_area, XmNresizeCallback, motTableResizeCallback, (XtPointer)ih);
  XtAddCallback(mot_data->drawing_area, XmNinputCallback, motTableInputCallback, (XtPointer)ih);

  /* the default translation gives Ctrl+Btn1 to traversal only, so the input callback never sees it */
  XtOverrideTranslations(mot_data->drawing_area,
                         XtParseTranslationTable("#override c<Btn1Down>: DrawingAreaInput() ManagerGadgetTraverseCurrent()"));

  XtAddEventHandler(mot_data->drawing_area, KeyPressMask, False, (XtEventHandler)motTableKeyPressCallback, (XtPointer)ih);
  XtAddEventHandler(mot_data->drawing_area, Button1MotionMask, False, (XtEventHandler)motTableRowDragMotion, (XtPointer)ih);
  XtAddEventHandler(mot_data->drawing_area, PointerMotionMask, False, (XtEventHandler)motTableHeaderPointerMotion, (XtPointer)ih);
  XtAddEventHandler(mot_data->drawing_area, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
  XtAddEventHandler(mot_data->drawing_area, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(mot_data->drawing_area, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);

  XtRealizeWidget(mot_data->container);

  {
    unsigned char bg_r, bg_g, bg_b;

    XtVaGetValues(mot_data->container, XmNbackground, &mot_data->bg_pixel, XmNforeground, &mot_data->fg_pixel, NULL);

    iupmotColorGetRGB(mot_data->bg_pixel, &bg_r, &bg_g, &bg_b);

    {
      unsigned char header_r = (bg_r > 20) ? bg_r - 20 : 0;
      unsigned char header_g = (bg_g > 20) ? bg_g - 20 : 0;
      unsigned char header_b = (bg_b > 20) ? bg_b - 20 : 0;
      mot_data->header_bg_pixel = iupmotColorGetPixel(header_r, header_g, header_b);
    }

    {
      unsigned char grid_r = (bg_r > 40) ? bg_r - 40 : 0;
      unsigned char grid_g = (bg_g > 40) ? bg_g - 40 : 0;
      unsigned char grid_b = (bg_b > 40) ? bg_b - 40 : 0;
      mot_data->grid_pixel = iupmotColorGetPixel(grid_r, grid_g, grid_b);
    }

    {
      unsigned char select_r = (unsigned char)((bg_r * 3 + 200) / 4);
      unsigned char select_g = (unsigned char)((bg_g * 3 + 220) / 4);
      unsigned char select_b = (unsigned char)((bg_b * 1 + 255 * 3) / 4);
      mot_data->select_bg_pixel = iupmotColorGetPixel(select_r, select_g, select_b);
    }
  }

  gcvalues.foreground = BlackPixel(iupmot_display, iupmot_screen);
  gcvalues.background = WhitePixel(iupmot_display, iupmot_screen);
  mot_data->gc = XCreateGC(iupmot_display, XtWindow(mot_data->drawing_area), GCForeground | GCBackground, &gcvalues);

#ifdef IUP_USE_XFT
  mot_data->xft_font = (XftFont*)iupmotGetXftFontAttrib(ih);
  if (mot_data->xft_font)
  {
    mot_data->xft_draw = XftDrawCreate(iupmot_display, XtWindow(mot_data->drawing_area),
                                       DefaultVisual(iupmot_display, iupmot_screen),
                                       DefaultColormap(iupmot_display, iupmot_screen));
    mot_data->font_struct_owned = 0;
  }
  else
#endif
  {
    mot_data->font_struct = (XFontStruct*)iupmotGetFontStructAttrib(ih);
    if (!mot_data->font_struct)
    {
      mot_data->font_struct = XLoadQueryFont(iupmot_display, "fixed");
      mot_data->font_struct_owned = 1;
    }
    else
    {
      mot_data->font_struct_owned = 0;
    }
    if (mot_data->font_struct)
      XSetFont(iupmot_display, mot_data->gc, mot_data->font_struct->fid);
  }

  mot_data->current_row = 0;
  mot_data->current_col = 0;

  motTableUpdateScrollbars(ih);
  motTableQueueAutoSize(ih);

  return IUP_NOERROR;
}

/* ========================================================================= */
/* Layout Update Method                                                     */
/* ========================================================================= */

static void motTableSetSize(Ihandle* ih, Widget container, int setsize, int use_width, int use_height)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int width, height;
  Dimension border = 0;
  int sb_size = iupdrvGetScrollbarSize();

  XtVaGetValues(container, XmNborderWidth, &border, NULL);
  width = use_width - 2*(int)border;
  height = use_height - 2*(int)border;

  if (width <= 0) width = 1;
  if (height <= 0) height = 1;

  if (setsize)
  {
    XtVaSetValues(container, XmNwidth, (XtArgVal)width, XmNheight, (XtArgVal)height, NULL);
  }

  XtVaSetValues(mot_data->drawing_area, XmNx, 0, XmNy, 0, XmNwidth, width - sb_size, XmNheight, height - sb_size, NULL);

  XtVaSetValues(mot_data->sb_horiz, XmNx, 0, XmNy, height - sb_size, XmNwidth, width - sb_size, XmNheight, sb_size, NULL);

  XtVaSetValues(mot_data->sb_vert, XmNx, width - sb_size, XmNy, 0, XmNwidth, sb_size, XmNheight, height - sb_size, NULL);

  motTableUpdateScrollbars(ih);
}

static void motTableLayoutUpdateMethod(Ihandle* ih)
{
  Widget container = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");

  motTableSetSize(ih, container, 1, ih->currentwidth, ih->currentheight);
  iupmotSetPosition(container, ih->x, ih->y);
}

/* ========================================================================= */
/* Widget Destruction - UnMapMethod                                         */
/* ========================================================================= */

static void motTableUnMapMethod(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int i, c;

  if (!mot_data)
    return;

  if (mot_data->autosize_timer)
    XtRemoveTimeOut(mot_data->autosize_timer);

  if (mot_data->gc)
    XFreeGC(iupmot_display, mot_data->gc);

  if (mot_data->resize_cursor)
    XFreeCursor(iupmot_display, mot_data->resize_cursor);

#ifdef IUP_USE_XFT
  if (mot_data->xft_draw)
    XftDrawDestroy(mot_data->xft_draw);
#endif

  if (mot_data->font_struct && mot_data->font_struct_owned)
    XFreeFont(iupmot_display, mot_data->font_struct);

  if (mot_data->cells)
  {
    for (i = 0; i < ih->data->num_lin; i++)
    {
      if (mot_data->cells[i])
      {
        for (c = 0; c < ih->data->num_col; c++)
        {
          motTableFreeCell(&mot_data->cells[i][c]);
        }
        free(mot_data->cells[i]);
      }
    }
    free(mot_data->cells);
  }

  if (mot_data->col_titles)
  {
    for (c = 0; c < ih->data->num_col; c++)
    {
      if (mot_data->col_titles[c])
        free(mot_data->col_titles[c]);
    }
    free(mot_data->col_titles);
  }

  if (mot_data->row_selected)
    free(mot_data->row_selected);

  if (mot_data->col_widths)
    free(mot_data->col_widths);

  if (mot_data->col_natural_widths)
    free(mot_data->col_natural_widths);

  if (mot_data->col_width_set)
    free(mot_data->col_width_set);

  if (mot_data->sort_signs)
    free(mot_data->sort_signs);

  if (mot_data->edit_text)
    XtDestroyWidget(mot_data->edit_text);

  if (mot_data->container)
    XtDestroyWidget(mot_data->container);

  free(mot_data);
  ih->data->native_data = NULL;
}

/* ========================================================================= */
/* Driver Functions - iupdrvTable* API                                      */
/* ========================================================================= */

static int motTableFollowPos(int cur, int pos, int delta, int count)
{
  if (cur <= 0)
    return cur;
  if (cur >= pos && !(delta < 0 && cur == pos))
    cur += delta;
  if (cur > count)
    cur = count;
  return cur;
}

static void motTableFollowLins(ImotTableData* mot_data, int pos, int delta, int num_lin)
{
  mot_data->current_row = motTableFollowPos(mot_data->current_row, pos, delta, num_lin);
  mot_data->anchor_row = motTableFollowPos(mot_data->anchor_row, pos, delta, num_lin);
}

IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int old_num_lin, i, col;

  if (!mot_data || num_lin < 0)
    return;

  old_num_lin = ih->data->num_lin;
  if (num_lin == old_num_lin)
    return;

  if (!iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    for (i = num_lin; i < old_num_lin; i++)
    {
      for (col = 0; col < ih->data->num_col; col++)
      {
        motTableFreeCell(&mot_data->cells[i][col]);
      }
      free(mot_data->cells[i]);
    }

    if (num_lin == 0)
    {
      free(mot_data->cells);
      mot_data->cells = NULL;
    }
    else
    {
      mot_data->cells = (ImotTableCell**)realloc(mot_data->cells, num_lin * sizeof(ImotTableCell*));
      for (i = old_num_lin; i < num_lin; i++)
        mot_data->cells[i] = (ImotTableCell*)calloc(ih->data->num_col, sizeof(ImotTableCell));
    }
  }

  ih->data->num_lin = num_lin;
  motTableFollowLins(mot_data, num_lin + 1, 0, num_lin);

  if (mot_data->row_selected && mot_data->row_selected_size > num_lin)
    memset(mot_data->row_selected + num_lin, 0, mot_data->row_selected_size - num_lin);

  motTableUpdateScrollbars(ih);
  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int old_num_col, i;

  if (!mot_data || num_col <= 0)
    return;

  old_num_col = ih->data->num_col;
  if (num_col == old_num_col)
    return;

  mot_data->col_widths = (int*)realloc(mot_data->col_widths, num_col * sizeof(int));
  mot_data->col_natural_widths = (int*)realloc(mot_data->col_natural_widths, num_col * sizeof(int));
  mot_data->col_width_set = (int*)realloc(mot_data->col_width_set, num_col * sizeof(int));
  mot_data->sort_signs = (char*)realloc(mot_data->sort_signs, num_col * sizeof(char));
  for (i = old_num_col; i < num_col; i++)
  {
    mot_data->col_widths[i] = MOT_TABLE_DEF_COL_WIDTH;
    mot_data->col_natural_widths[i] = MOT_TABLE_DEF_COL_WIDTH;
    mot_data->col_width_set[i] = 0;
    mot_data->sort_signs[i] = 0;
  }

  for (i = num_col; i < old_num_col; i++)
  {
    if (mot_data->col_titles[i])
      free(mot_data->col_titles[i]);
  }
  mot_data->col_titles = (char**)realloc(mot_data->col_titles, num_col * sizeof(char*));
  for (i = old_num_col; i < num_col; i++)
    mot_data->col_titles[i] = NULL;

  for (i = 0; mot_data->cells && i < ih->data->num_lin; i++)
  {
    int j;
    for (j = num_col; j < old_num_col; j++)
      motTableFreeCell(&mot_data->cells[i][j]);
    mot_data->cells[i] = (ImotTableCell*)realloc(mot_data->cells[i], num_col * sizeof(ImotTableCell));
    for (j = old_num_col; j < num_col; j++)
    {
      mot_data->cells[i][j].value = NULL;
      mot_data->cells[i][j].image = NULL;
    }
  }

  ih->data->num_col = num_col;
  mot_data->current_col = motTableFollowPos(mot_data->current_col, num_col + 1, 0, num_col);

  motTableUpdateScrollbars(ih);
  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int lin, new_num_lin;

  if (!mot_data)
    return;

  /* pos is 1-based from core, convert to 0-based */
  pos = pos - 1;

  new_num_lin = ih->data->num_lin + 1;

  if (!iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    mot_data->cells = (ImotTableCell**)realloc(mot_data->cells, new_num_lin * sizeof(ImotTableCell*));

    for (lin = new_num_lin - 1; lin > pos; lin--)
      mot_data->cells[lin] = mot_data->cells[lin - 1];

    mot_data->cells[pos] = (ImotTableCell*)calloc(ih->data->num_col, sizeof(ImotTableCell));
  }

  ih->data->num_lin = new_num_lin;
  motTableFollowLins(mot_data, pos + 1, 1, new_num_lin);

  if (mot_data->row_selected)
  {
    char* sel = motTableSelection(ih);
    for (lin = new_num_lin - 1; lin > pos; lin--)
      sel[lin] = sel[lin - 1];
    sel[pos] = 0;
  }

  motTableUpdateScrollbars(ih);
  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int lin, col, new_num_lin;

  if (!mot_data)
    return;

  /* pos is 1-based from core, convert to 0-based */
  pos = pos - 1;

  if (pos < 0 || pos >= ih->data->num_lin)
    return;

  new_num_lin = ih->data->num_lin - 1;

  if (!iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    for (col = 0; col < ih->data->num_col; col++)
    {
      motTableFreeCell(&mot_data->cells[pos][col]);
    }
    free(mot_data->cells[pos]);

    for (lin = pos; lin < new_num_lin; lin++)
      mot_data->cells[lin] = mot_data->cells[lin + 1];

    if (new_num_lin > 0)
      mot_data->cells = (ImotTableCell**)realloc(mot_data->cells, new_num_lin * sizeof(ImotTableCell*));
    else
    {
      free(mot_data->cells);
      mot_data->cells = NULL;
    }
  }

  if (mot_data->row_selected)
  {
    for (lin = pos; lin < mot_data->row_selected_size - 1; lin++)
      mot_data->row_selected[lin] = mot_data->row_selected[lin + 1];
    mot_data->row_selected[mot_data->row_selected_size - 1] = 0;
  }

  ih->data->num_lin = new_num_lin;
  motTableFollowLins(mot_data, pos + 1, -1, new_num_lin);

  motTableUpdateScrollbars(ih);
  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int lin, col, new_num_col;

  if (!mot_data)
    return;

  /* pos is 1-based from core, convert to 0-based */
  pos = pos - 1;

  new_num_col = ih->data->num_col + 1;

  mot_data->col_widths = (int*)realloc(mot_data->col_widths, new_num_col * sizeof(int));
  mot_data->col_natural_widths = (int*)realloc(mot_data->col_natural_widths, new_num_col * sizeof(int));
  mot_data->col_width_set = (int*)realloc(mot_data->col_width_set, new_num_col * sizeof(int));
  mot_data->sort_signs = (char*)realloc(mot_data->sort_signs, new_num_col * sizeof(char));
  for (col = new_num_col - 1; col > pos; col--)
  {
    mot_data->col_widths[col] = mot_data->col_widths[col - 1];
    mot_data->col_natural_widths[col] = mot_data->col_natural_widths[col - 1];
    mot_data->col_width_set[col] = mot_data->col_width_set[col - 1];
    mot_data->sort_signs[col] = mot_data->sort_signs[col - 1];
  }
  mot_data->col_widths[pos] = MOT_TABLE_DEF_COL_WIDTH;
  mot_data->col_natural_widths[pos] = MOT_TABLE_DEF_COL_WIDTH;
  mot_data->col_width_set[pos] = 0;
  mot_data->sort_signs[pos] = 0;

  mot_data->col_titles = (char**)realloc(mot_data->col_titles, new_num_col * sizeof(char*));
  for (col = new_num_col - 1; col > pos; col--)
  {
    mot_data->col_titles[col] = mot_data->col_titles[col - 1];
  }
  mot_data->col_titles[pos] = NULL;

  for (lin = 0; mot_data->cells && lin < ih->data->num_lin; lin++)
  {
    mot_data->cells[lin] = (ImotTableCell*)realloc(mot_data->cells[lin], new_num_col * sizeof(ImotTableCell));
    for (col = new_num_col - 1; col > pos; col--)
    {
      mot_data->cells[lin][col] = mot_data->cells[lin][col - 1];
    }
    mot_data->cells[lin][pos].value = NULL;
    mot_data->cells[lin][pos].image = NULL;
  }

  ih->data->num_col = new_num_col;
  mot_data->current_col = motTableFollowPos(mot_data->current_col, pos + 1, 1, new_num_col);

  motTableUpdateScrollbars(ih);
  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int lin, col, new_num_col;

  if (!mot_data)
    return;

  /* pos is 1-based from core, convert to 0-based */
  pos = pos - 1;

  if (pos < 0 || pos >= ih->data->num_col)
    return;

  new_num_col = ih->data->num_col - 1;

  if (mot_data->col_titles[pos])
    free(mot_data->col_titles[pos]);

  for (lin = 0; mot_data->cells && lin < ih->data->num_lin; lin++)
  {
    motTableFreeCell(&mot_data->cells[lin][pos]);

    for (col = pos; col < new_num_col; col++)
    {
      mot_data->cells[lin][col] = mot_data->cells[lin][col + 1];
    }

    if (new_num_col > 0)
      mot_data->cells[lin] = (ImotTableCell*)realloc(mot_data->cells[lin], new_num_col * sizeof(ImotTableCell));
    else
    {
      free(mot_data->cells[lin]);
      mot_data->cells[lin] = NULL;
    }
  }

  for (col = pos; col < new_num_col; col++)
  {
    mot_data->col_widths[col] = mot_data->col_widths[col + 1];
    mot_data->col_natural_widths[col] = mot_data->col_natural_widths[col + 1];
    mot_data->col_width_set[col] = mot_data->col_width_set[col + 1];
  }
  if (new_num_col > 0)
  {
    mot_data->col_widths = (int*)realloc(mot_data->col_widths, new_num_col * sizeof(int));
    mot_data->col_natural_widths = (int*)realloc(mot_data->col_natural_widths, new_num_col * sizeof(int));
    mot_data->col_width_set = (int*)realloc(mot_data->col_width_set, new_num_col * sizeof(int));
  }
  else
  {
    free(mot_data->col_widths);
    mot_data->col_widths = NULL;
    free(mot_data->col_natural_widths);
    mot_data->col_natural_widths = NULL;
    free(mot_data->col_width_set);
    mot_data->col_width_set = NULL;
  }

  for (col = pos; col < new_num_col; col++)
  {
    mot_data->col_titles[col] = mot_data->col_titles[col + 1];
  }
  if (new_num_col > 0)
    mot_data->col_titles = (char**)realloc(mot_data->col_titles, new_num_col * sizeof(char*));
  else
  {
    free(mot_data->col_titles);
    mot_data->col_titles = NULL;
  }

  ih->data->num_col = new_num_col;
  mot_data->current_col = motTableFollowPos(mot_data->current_col, pos + 1, -1, new_num_col);

  motTableUpdateScrollbars(ih);
  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  motTableSetCellValueInternal(ih, lin, col, value);
  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  return motTableGetCellValueInternal(ih, lin, col);
}

IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  ImotTableCell* cell;

  if (!mot_data || !mot_data->cells || lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  cell = &mot_data->cells[lin-1][col-1];
  if (cell->image)
    free(cell->image);
  cell->image = image ? iupStrDup(image) : NULL;

  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || col < 1 || col > ih->data->num_col)
    return;

  if (mot_data->col_titles[col-1])
    free(mot_data->col_titles[col-1]);

  mot_data->col_titles[col-1] = title ? iupStrDup(title) : NULL;
  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || col < 1 || col > ih->data->num_col)
    return NULL;

  return mot_data->col_titles[col-1];
}

IUP_SDK_API void iupdrvTableSetSortSign(Ihandle* ih, int col, int sign)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || !mot_data->sort_signs || col < 1 || col > ih->data->num_col)
    return;

  if (mot_data->sort_column > 0 && mot_data->sort_column <= ih->data->num_col)
    mot_data->sort_signs[mot_data->sort_column - 1] = 0;

  mot_data->sort_column = sign ? col : 0;
  mot_data->sort_signs[col - 1] = (char)sign;

  motTableRedraw(ih);
}

IUP_SDK_API int iupdrvTableGetSortSign(Ihandle* ih, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || !mot_data->sort_signs || mot_data->sort_column != col)
    return 0;

  return mot_data->sort_signs[col - 1];
}

IUP_SDK_API void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || col < 1 || col > ih->data->num_col)
    return;

  mot_data->col_widths[col-1] = width;
  mot_data->col_natural_widths[col-1] = width;
  mot_data->col_width_set[col-1] = 1;
  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

IUP_SDK_API int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || col < 1 || col > ih->data->num_col)
    return 0;

  return mot_data->col_widths[col-1];
}

IUP_SDK_API void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data)
    return;

  mot_data->current_row = lin;
  mot_data->current_col = col;

  if (!iupStrEqualNoCase(iupAttribGetStr(ih, "SELECTIONMODE"), "NONE") && !iupTableCellsMode(ih))
  {
    motTableSelectRow(ih, lin, 1, 1);
    mot_data->anchor_row = lin;
  }

  iupdrvTableScrollToCell(ih, lin, col);
}

IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data)
    return;

  if (lin) *lin = mot_data->current_row;
  if (col) *col = mot_data->current_col;
}

IUP_SDK_API int iupdrvTableIsLinSelected(Ihandle* ih, int lin)
{
  if (!IMOT_TABLE_DATA(ih))
    return 0;

  return motTableRowSelected(ih, lin);
}

IUP_SDK_API void iupdrvTableSelectLin(Ihandle* ih, int lin, int select)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data)
    return;

  motTableSelectRow(ih, lin, select, 0);
  motTableRedraw(ih);
}

IUP_SDK_API int* iupdrvTableGetSelectedLins(Ihandle* ih, int* count)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int* lins;
  int lin, i = 0;

  *count = 0;

  if (!mot_data || !mot_data->row_selected || ih->data->num_lin <= 0)
    return NULL;

  lins = (int*)malloc(sizeof(int) * ih->data->num_lin);

  for (lin = 1; lin <= ih->data->num_lin; lin++)
  {
    if (motTableRowSelected(ih, lin))
      lins[i++] = lin;
  }

  if (i == 0)
  {
    free(lins);
    return NULL;
  }

  *count = i;
  return lins;
}

IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  Dimension width, height;
  int visible_height, row_top, row_bottom, c, col_left, col_right;

  if (!mot_data || !mot_data->drawing_area || !mot_data->col_widths || mot_data->row_height <= 0)
    return;

  XtVaGetValues(mot_data->drawing_area, XmNwidth, &width, XmNheight, &height, NULL);

  if (!XtWindow(mot_data->drawing_area) || height <= mot_data->header_height + 1)
  {
    mot_data->pending_scroll_lin = lin;
    mot_data->pending_scroll_col = col;
    return;
  }

  visible_height = height - mot_data->header_height;
  row_top = (lin - 1) * mot_data->row_height;
  row_bottom = row_top + mot_data->row_height;

  if (row_top < mot_data->scroll_y)
    mot_data->scroll_y = row_top;
  else if (row_bottom > mot_data->scroll_y + visible_height)
    mot_data->scroll_y = row_bottom - visible_height;

  if (mot_data->scroll_y < 0)
    mot_data->scroll_y = 0;

  col_left = 0;
  for (c = 0; c < col - 1 && c < ih->data->num_col; c++)
    col_left += mot_data->col_widths[c];
  col_right = col_left + mot_data->col_widths[col - 1];

  if (col_left < mot_data->scroll_x)
    mot_data->scroll_x = col_left;
  else if (col_right > mot_data->scroll_x + width)
    mot_data->scroll_x = col_right - width;

  if (mot_data->scroll_x < 0)
    mot_data->scroll_x = 0;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih)
{
  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableUpdateCellStyle(Ihandle* ih, int lin, int col)
{
  (void)lin;
  (void)col;
  motTableQueueAutoSize(ih);
  motTableRedraw(ih);
}

/* ========================================================================= */
/* Attribute Handlers                                                        */
/* ========================================================================= */

static int motTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
    ih->data->sortable = 0;

  if (ih->handle)
  {
    ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
    if (!ih->data->sortable && mot_data && mot_data->sort_signs && mot_data->sort_column > 0)
    {
      mot_data->sort_signs[mot_data->sort_column - 1] = 0;
      mot_data->sort_column = 0;
    }
    motTableRedraw(ih);
  }

  return 0;  /* Do not store in hash table */
}

static int motTableSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);
  motTableRedraw(ih);
  return 1;
}

IUP_SDK_API void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data)
    return;

  mot_data->show_grid = show;
  motTableRedraw(ih);
}

IUP_SDK_API int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  Dimension border = 0;

  if (mot_data && mot_data->container)
  {
    XtVaGetValues(mot_data->container, XmNborderWidth, &border, NULL);
    return 2 * (int)border;
  }

  return 0;
}

IUP_SDK_API int iupdrvTableGetRowHeight(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  if (mot_data && mot_data->row_height > 0)
    return mot_data->row_height;

  return MOT_TABLE_DEF_ROW_HEIGHT;
}

IUP_SDK_API int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  if (mot_data && mot_data->header_height > 0)
    return mot_data->header_height;

  return MOT_TABLE_HEADER_HEIGHT;
}

IUP_SDK_API void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  int sb_size = iupdrvGetScrollbarSize();
  int border = iupdrvTableGetBorderWidth(ih);

  /* motTableSetSize always reserves horizontal scrollbar space */
  *w += sb_size + border;
  *h += sb_size + border;
}

/* ========================================================================= */
/* Class Initialization                                                      */
/* ========================================================================= */

IUP_SDK_API void iupdrvTableInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motTableMapMethod;
  ic->UnMap = motTableUnMapMethod;
  ic->LayoutUpdate = motTableLayoutUpdateMethod;

  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, motTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ACTIVE", iupBaseGetActiveAttrib, motTableSetActiveAttrib);
}
