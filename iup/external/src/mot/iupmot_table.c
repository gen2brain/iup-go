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
#include <X11/XKBlib.h>

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

#include "iupmot_drv.h"
#include "iupmot_color.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_table.h"


#define MOT_TABLE_DEF_ROW_HEIGHT    20
#define MOT_TABLE_DEF_COL_WIDTH     80
#define MOT_TABLE_HEADER_HEIGHT     24
#define MOT_TABLE_CELL_PADDING      3

/* ========================================================================= */
/* Motif-specific data structure (XmDrawingArea-based)                      */
/* ========================================================================= */

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

  /* Row drag-reorder state (SHOWDRAGDROP) */
  int drag_source_row;       /* Row where the drag started (1-based, 0=none) */
  int drag_target_row;       /* Insert-before index while dragging (0-based, -1=none) */
  int row_dragging;          /* 1 once past the drag threshold */
  int drag_start_x;
  int drag_start_y;

  char*** cell_values;       /* [num_lin][num_col] -> string */
  char** col_titles;         /* [num_col] -> string */

  int edit_lin;              /* Row being edited (1-based, 0=not editing) */
  int edit_col;              /* Column being edited (1-based, 0=not editing) */

  int show_grid;

  int columns_autosized;

  int sort_column;           /* Currently sorted column (1-based, 0=none) */
  char* sort_signs;

  Pixel bg_pixel;
  Pixel fg_pixel;
  Pixel header_bg_pixel;
  Pixel grid_pixel;
  Pixel select_bg_pixel;

  /* VISIBLELINES/VISIBLECOLUMNS constraints (0 = no constraint) */
  int target_height;
  int visible_columns;

} ImotTableData;

#define IMOT_TABLE_DATA(ih) ((ImotTableData*)(ih->data->native_data))

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

static void motTableSortRows(Ihandle* ih, int col, int ascending)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int i, j;
  int num_rows = ih->data->num_lin;
  int num_cols = ih->data->num_col;

  if (!mot_data->cell_values || num_rows < 2 || col < 1 || col > num_cols)
    return;

  for (i = 0; i < num_rows - 1; i++)
  {
    for (j = 0; j < num_rows - i - 1; j++)
    {
      const char* val1 = mot_data->cell_values[j][col - 1];
      const char* val2 = mot_data->cell_values[j + 1][col - 1];
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
        char** temp_row = mot_data->cell_values[j];
        mot_data->cell_values[j] = mot_data->cell_values[j + 1];
        mot_data->cell_values[j + 1] = temp_row;
      }
    }
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

  if (!mot_data || !mot_data->cell_values)
    return NULL;

  return mot_data->cell_values[lin-1][col-1];
}

static void motTableSetCellValueInternal(Ihandle* ih, int lin, int col, const char* value)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || !mot_data->cell_values)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  if (mot_data->cell_values[lin-1][col-1])
  {
    free(mot_data->cell_values[lin-1][col-1]);
    mot_data->cell_values[lin-1][col-1] = NULL;
  }

  if (value)
    mot_data->cell_values[lin-1][col-1] = iupStrDup(value);
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
  py += mot_data->scroll_y;

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
  char** row;
  int l;

  if (from == to || from < 1 || to < 1 || from > num_lin || to > num_lin)
    return;
  if (!mot_data->cell_values)
    return;

  row = mot_data->cell_values[from - 1];
  if (from < to)
    for (l = from - 1; l < to - 1; l++)
      mot_data->cell_values[l] = mot_data->cell_values[l + 1];
  else
    for (l = from - 1; l > to - 1; l--)
      mot_data->cell_values[l] = mot_data->cell_values[l - 1];
  mot_data->cell_values[to - 1] = row;

  iupTableMoveLinAttribs(ih, from, to);

  mot_data->current_row = to;
  motTableRedraw(ih);
}

static void motTableRowDragMotion(Widget w, XtPointer client_data, XEvent* event, Boolean* cont)
{
  Ihandle* ih = (Ihandle*)client_data;
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  XMotionEvent* motion = (XMotionEvent*)event;
  int target;

  (void)w;
  (void)cont;

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

static void motTableDrawCell(Ihandle* ih, int lin, int col, int is_header)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  Display* display = iupmot_display;
  Window window = XtWindow(mot_data->drawing_area);
  int x, y, w, h;
  const char* text;
  int text_len, text_width, text_x, text_y;
  int is_focused_cell = (lin == mot_data->current_row && col == mot_data->current_col);
  int is_focused_row = (lin == mot_data->current_row && !is_header);

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

  if (is_focused_row)
  {
    XSetForeground(display, mot_data->gc, mot_data->select_bg_pixel);
  }
  else if (is_header)
  {
    XSetForeground(display, mot_data->gc, mot_data->header_bg_pixel);
  }
  else if (bgcolor && *bgcolor)
  {
    XSetForeground(display, mot_data->gc, iupmotColorGetPixelStr(bgcolor));
  }
  else
  {
    XSetForeground(display, mot_data->gc, mot_data->bg_pixel);
  }

  XFillRectangle(display, window, mot_data->gc, x, y, w, h);

  if (mot_data->show_grid)
  {
    XSetForeground(display, mot_data->gc, mot_data->grid_pixel);
    XDrawLine(display, window, mot_data->gc, x, y + h - 1, x + w, y + h - 1); /* Bottom */
    XDrawLine(display, window, mot_data->gc, x + w - 1, y, x + w - 1, y + h); /* Right */
  }

  if (text && text[0])
  {
    char* fgcolor = NULL;

    /* Get foreground color with hierarchy: per-cell > per-column > per-row > default */
    if (!is_header)
    {
      fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);
      if (!fgcolor)
        fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);
      if (!fgcolor)
        fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);
    }

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

#ifdef IUP_USE_XFT
    if (mot_data->xft_font)
    {
      XGlyphInfo extents;
      XftTextExtents8(display, mot_data->xft_font, (XftChar8*)text, text_len, &extents);
      text_width = extents.width;
    }
    else
#endif
    if (mot_data->font_struct)
      text_width = XTextWidth(mot_data->font_struct, text, text_len);
    else
      text_width = text_len * 7;

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
      text_x = x + (w - text_width) / 2;
    }
    else
    {
      text_x = x + MOT_TABLE_CELL_PADDING;
    }

#ifdef IUP_USE_XFT
    if (mot_data->xft_font)
      text_y = y + h / 2 + mot_data->xft_font->ascent / 2;
    else
#endif
      text_y = y + h / 2 + (mot_data->font_struct ? mot_data->font_struct->ascent / 2 : 6);

#ifdef IUP_USE_XFT
    if (mot_data->xft_draw && mot_data->xft_font)
    {
      XftColor xft_color;
      XRenderColor render_color;

      render_color.red = tr * 257;
      render_color.green = tg * 257;
      render_color.blue = tb * 257;
      render_color.alpha = 0xffff;

      XftColorAllocValue(display, DefaultVisual(display, iupmot_screen), DefaultColormap(display, iupmot_screen), &render_color, &xft_color);
      XftDrawString8(mot_data->xft_draw, &xft_color, mot_data->xft_font, text_x, text_y, (XftChar8*)text, text_len);
      XftColorFree(display, DefaultVisual(display, iupmot_screen), DefaultColormap(display, iupmot_screen), &xft_color);
    }
    else
#endif
      XDrawString(display, window, mot_data->gc, text_x, text_y, text, text_len);

    if (is_header && ih->data->sortable && mot_data->sort_column == col && mot_data->sort_signs)
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

static void motTableDrawTable(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  Display* display = iupmot_display;
  Window window = XtWindow(mot_data->drawing_area);
  Dimension width, height;
  int lin, col;

  if (!window)
    return;

  if (!mot_data->columns_autosized)
  {
    int c, lin1;
    int max_rows_to_check = (ih->data->num_lin > 100) ? 100 : ih->data->num_lin;  /* Limit for performance */

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
#ifdef IUP_USE_XFT
        if (mot_data->xft_font)
        {
          XGlyphInfo extents;
          XftTextExtents8(iupmot_display, mot_data->xft_font, (XftChar8*)mot_data->col_titles[c], strlen(mot_data->col_titles[c]), &extents);
          title_width = extents.width;
        }
        else
#endif
          title_width = XTextWidth(mot_data->font_struct, mot_data->col_titles[c], strlen(mot_data->col_titles[c]));
        title_width += 20;  /* Add padding for sort indicator space */
        if (title_width > max_width)
          max_width = title_width;
      }

      for (lin1 = 0; lin1 < max_rows_to_check; lin1++)
      {
        const char* cell_value = IupGetAttributeId2(ih, "", lin1 + 1, c + 1);
        if (cell_value && cell_value[0])
        {
#ifdef IUP_USE_XFT
          if (mot_data->xft_font)
          {
            XGlyphInfo extents;
            XftTextExtents8(iupmot_display, mot_data->xft_font, (XftChar8*)cell_value, strlen(cell_value), &extents);
            cell_width = extents.width;
          }
          else
#endif
            cell_width = XTextWidth(mot_data->font_struct, cell_value, strlen(cell_value));
          cell_width += 16;  /* Add padding (8px left + 8px right) */
          if (cell_width > max_width)
            max_width = cell_width;
        }
      }

      if (max_width > mot_data->col_widths[c])
      {
        mot_data->col_widths[c] = max_width;
        mot_data->col_natural_widths[c] = max_width;
      }
    }

    mot_data->columns_autosized = 1;

    motTableUpdateScrollbars(ih);
  }

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

          if (lin == mot_data->current_row)
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
    if (ret == IUP_IGNORE && apply)
    {
      XtFree(text);
      XmProcessTraversal(mot_data->edit_text, XmTRAVERSE_CURRENT);
      XmTextSetSelection(mot_data->edit_text, 0, XmTextGetLastPosition(mot_data->edit_text), CurrentTime);
      return;
    }
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
  (void)w;
  (void)call_data;

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

    motTablePixelToCell(ih, button_event->x, button_event->y, &lin, &col);

    if (lin == 0 && col > 0)
    {
      if (ih->data->sortable)
      {
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
    }
    else if (lin > 0 && col > 0)
    {
      if (mot_data->edit_lin != 0)
        motTableEndCellEdit(ih, 1);

      mot_data->current_row = lin;
      mot_data->current_col = col;

      if (ih->data->show_dragdrop && button_event->button == Button1)
      {
        mot_data->drag_source_row = lin;
        mot_data->drag_target_row = -1;
        mot_data->row_dragging = 0;
        mot_data->drag_start_x = button_event->x;
        mot_data->drag_start_y = button_event->y;
      }

      IFniis cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
      if (cb)
        cb(ih, lin, col, "1");

      IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
      if (enteritem_cb)
        enteritem_cb(ih, lin, col);

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

  if (keysym == XK_Up && mot_data->current_row > 1)
  {
    mot_data->current_row--;
    redraw = 1;

    IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enteritem_cb)
      enteritem_cb(ih, mot_data->current_row, mot_data->current_col);
  }
  else if (keysym == XK_Down && mot_data->current_row < ih->data->num_lin)
  {
    mot_data->current_row++;
    redraw = 1;

    IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enteritem_cb)
      enteritem_cb(ih, mot_data->current_row, mot_data->current_col);
  }
  else if (keysym == XK_Left && mot_data->current_col > 1)
  {
    mot_data->current_col--;
    redraw = 1;

    IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enteritem_cb)
      enteritem_cb(ih, mot_data->current_row, mot_data->current_col);
  }
  else if (keysym == XK_Right && mot_data->current_col < ih->data->num_col)
  {
    mot_data->current_col++;
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
    motTableRedraw(ih);

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
  else
  {
    total_width = natural_total_width;
    for (c = 0; c < ih->data->num_col; c++)
      mot_data->col_widths[c] = mot_data->col_natural_widths[c];
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
    mot_data->cell_values = (char***)calloc(ih->data->num_lin, sizeof(char**));
    for (i = 0; i < ih->data->num_lin; i++)
    {
      mot_data->cell_values[i] = (char**)calloc(ih->data->num_col, sizeof(char*));
    }
  }
  else
  {
    /* Virtual mode: no cell storage needed, VALUE_CB provides data on demand */
    mot_data->cell_values = NULL;
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

  XtAddEventHandler(mot_data->drawing_area, KeyPressMask, False, (XtEventHandler)motTableKeyPressCallback, (XtPointer)ih);
  XtAddEventHandler(mot_data->drawing_area, Button1MotionMask, False, (XtEventHandler)motTableRowDragMotion, (XtPointer)ih);
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

  mot_data->columns_autosized = 0;

  /* Store constraints for VISIBLELINES/VISIBLECOLUMNS clamping in LayoutUpdate */
  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    int row_height = iupdrvTableGetRowHeight(ih);
    int header_height = iupdrvTableGetHeaderHeight(ih);
    int border = iupdrvTableGetBorderWidth(ih);
    int sb_size = iupdrvGetScrollbarSize();

    /* motTableSetSize reserves horizontal scrollbar space, and VISIBLELINES can trigger it */
    mot_data->target_height = header_height + (row_height * visiblelines) + border + sb_size;
  }
  else
  {
    mot_data->target_height = 0;
  }

  mot_data->visible_columns = iupAttribGetInt(ih, "VISIBLECOLUMNS");

  motTableUpdateScrollbars(ih);

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
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  Widget container = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  int width = ih->currentwidth;
  int height = ih->currentheight;

  if (mot_data && mot_data->target_height > 0)
  {
    if (height > mot_data->target_height)
      height = mot_data->target_height;
  }

  if (mot_data && mot_data->visible_columns > 0 && mot_data->col_widths)
  {
    int c, cols_width = 0;
    int num_cols = mot_data->visible_columns;
    if (num_cols > ih->data->num_col)
      num_cols = ih->data->num_col;

    for (c = 0; c < num_cols; c++)
      cols_width += mot_data->col_widths[c];

    int sb_size = iupdrvGetScrollbarSize();
    int border = iupdrvTableGetBorderWidth(ih);

    int visiblelines = mot_data->target_height > 0 ? 1 : 0;  /* target_height > 0 means VISIBLELINES was set */
    int need_vert_sb = (visiblelines && ih->data->num_lin > iupAttribGetInt(ih, "VISIBLELINES"));
    int vert_sb_width = need_vert_sb ? sb_size : 0;

    int target_width = cols_width + vert_sb_width + border + sb_size;

    if (width > target_width)
      width = target_width;
  }

  motTableSetSize(ih, container, 1, width, height);
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

  if (mot_data->gc)
    XFreeGC(iupmot_display, mot_data->gc);

#ifdef IUP_USE_XFT
  if (mot_data->xft_draw)
    XftDrawDestroy(mot_data->xft_draw);
#endif

  if (mot_data->font_struct && mot_data->font_struct_owned)
    XFreeFont(iupmot_display, mot_data->font_struct);

  if (mot_data->cell_values)
  {
    for (i = 0; i < ih->data->num_lin; i++)
    {
      if (mot_data->cell_values[i])
      {
        for (c = 0; c < ih->data->num_col; c++)
        {
          if (mot_data->cell_values[i][c])
            free(mot_data->cell_values[i][c]);
        }
        free(mot_data->cell_values[i]);
      }
    }
    free(mot_data->cell_values);
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

IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int old_num_lin, i, col;

  if (!mot_data || num_lin < 0)
    return;

  old_num_lin = ih->data->num_lin;
  if (num_lin == old_num_lin)
    return;

  for (i = num_lin; i < old_num_lin; i++)
  {
    for (col = 0; col < ih->data->num_col; col++)
    {
      if (mot_data->cell_values[i][col])
        free(mot_data->cell_values[i][col]);
    }
    free(mot_data->cell_values[i]);
  }

  if (num_lin == 0)
  {
    free(mot_data->cell_values);
    mot_data->cell_values = NULL;
  }
  else
  {
    mot_data->cell_values = (char***)realloc(mot_data->cell_values, num_lin * sizeof(char**));
    for (i = old_num_lin; i < num_lin; i++)
      mot_data->cell_values[i] = (char**)calloc(ih->data->num_col, sizeof(char*));
  }

  ih->data->num_lin = num_lin;

  motTableUpdateScrollbars(ih);
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

  mot_data->col_titles = (char**)realloc(mot_data->col_titles, num_col * sizeof(char*));
  for (i = old_num_col; i < num_col; i++)
    mot_data->col_titles[i] = NULL;

  for (i = 0; i < ih->data->num_lin; i++)
  {
    mot_data->cell_values[i] = (char**)realloc(mot_data->cell_values[i], num_col * sizeof(char*));
    int j;
    for (j = old_num_col; j < num_col; j++)
      mot_data->cell_values[i][j] = NULL;
  }

  ih->data->num_col = num_col;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int lin, new_num_lin;

  if (!mot_data)
    return;

  if (!mot_data->cell_values)
    return;

  /* pos is 1-based from core, convert to 0-based */
  pos = pos - 1;

  new_num_lin = ih->data->num_lin + 1;

  mot_data->cell_values = (char***)realloc(mot_data->cell_values, new_num_lin * sizeof(char**));

  for (lin = new_num_lin - 1; lin > pos; lin--)
  {
    mot_data->cell_values[lin] = mot_data->cell_values[lin - 1];
  }

  mot_data->cell_values[pos] = (char**)calloc(ih->data->num_col, sizeof(char*));

  ih->data->num_lin = new_num_lin;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int lin, col, new_num_lin;

  if (!mot_data)
    return;

  if (!mot_data->cell_values)
    return;

  /* pos is 1-based from core, convert to 0-based */
  pos = pos - 1;

  if (pos < 0 || pos >= ih->data->num_lin)
    return;

  for (col = 0; col < ih->data->num_col; col++)
  {
    if (mot_data->cell_values[pos][col])
      free(mot_data->cell_values[pos][col]);
  }
  free(mot_data->cell_values[pos]);

  new_num_lin = ih->data->num_lin - 1;
  for (lin = pos; lin < new_num_lin; lin++)
  {
    mot_data->cell_values[lin] = mot_data->cell_values[lin + 1];
  }

  if (new_num_lin > 0)
    mot_data->cell_values = (char***)realloc(mot_data->cell_values, new_num_lin * sizeof(char**));
  else
  {
    free(mot_data->cell_values);
    mot_data->cell_values = NULL;
  }

  ih->data->num_lin = new_num_lin;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int lin, col, new_num_col;

  if (!mot_data)
    return;

  if (!mot_data->cell_values)
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

  for (lin = 0; lin < ih->data->num_lin; lin++)
  {
    mot_data->cell_values[lin] = (char**)realloc(mot_data->cell_values[lin], new_num_col * sizeof(char*));
    for (col = new_num_col - 1; col > pos; col--)
    {
      mot_data->cell_values[lin][col] = mot_data->cell_values[lin][col - 1];
    }
    mot_data->cell_values[lin][pos] = NULL;
  }

  ih->data->num_col = new_num_col;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int lin, col, new_num_col;

  if (!mot_data)
    return;

  if (!mot_data->cell_values)
    return;

  /* pos is 1-based from core, convert to 0-based */
  pos = pos - 1;

  if (pos < 0 || pos >= ih->data->num_col)
    return;

  new_num_col = ih->data->num_col - 1;

  if (mot_data->col_titles[pos])
    free(mot_data->col_titles[pos]);

  for (lin = 0; lin < ih->data->num_lin; lin++)
  {
    if (mot_data->cell_values[lin][pos])
      free(mot_data->cell_values[lin][pos]);

    for (col = pos; col < new_num_col; col++)
    {
      mot_data->cell_values[lin][col] = mot_data->cell_values[lin][col + 1];
    }

    if (new_num_col > 0)
      mot_data->cell_values[lin] = (char**)realloc(mot_data->cell_values[lin], new_num_col * sizeof(char*));
    else
    {
      free(mot_data->cell_values[lin]);
      mot_data->cell_values[lin] = NULL;
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

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  motTableSetCellValueInternal(ih, lin, col, value);
  motTableRedraw(ih);
}

IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  return motTableGetCellValueInternal(ih, lin, col);
}

IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  (void)ih;
  (void)lin;
  (void)col;
  (void)image;
}

IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || col < 1 || col > ih->data->num_col)
    return;

  if (mot_data->col_titles[col-1])
    free(mot_data->col_titles[col-1]);

  mot_data->col_titles[col-1] = title ? iupStrDup(title) : NULL;
  motTableRedraw(ih);
}

IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || col < 1 || col > ih->data->num_col)
    return NULL;

  return mot_data->col_titles[col-1];
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
  motTableRedraw(ih);

  IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (enteritem_cb)
  {
    enteritem_cb(ih, lin, col);
  }
}

IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data)
    return;

  if (lin) *lin = mot_data->current_row;
  if (col) *col = mot_data->current_col;
}

IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  /* Not implemented */
  (void)ih;
  (void)lin;
  (void)col;
}

IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih)
{
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
  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

  *w += sb_size + border;

  *h += border;

  /* motTableSetSize always reserves horizontal scrollbar space */
  if (visiblecolumns > 0 && ih->data->num_col > visiblecolumns)
  {
    *h += sb_size;
  }
  else if (visiblelines == 0)
  {
    *h += sb_size;
  }
  /* with VISIBLELINES the target_height from MapMethod already includes sb_size */
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

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "USERRESIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FITIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "IMAGE", NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, motTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ACTIVE", iupBaseGetActiveAttrib, motTableSetActiveAttrib);
}
