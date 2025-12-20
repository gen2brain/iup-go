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
#include "iup_table.h"
#include "iup_childtree.h"
#include "iup_dialog.h"
#include "iup_layout.h"

#include "iupmot_drv.h"
#include "iupmot_color.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_key.h"
#include "iup_tablecontrol.h"


/* Default sizes */
#define MOT_TABLE_DEF_ROW_HEIGHT    20
#define MOT_TABLE_DEF_COL_WIDTH     80
#define MOT_TABLE_HEADER_HEIGHT     24
#define MOT_TABLE_CELL_PADDING      3

/* ========================================================================= */
/* Motif-specific data structure (XmDrawingArea-based)                      */
/* ========================================================================= */

typedef struct _ImotTableData
{
  /* Widgets */
  Widget container;          /* XmBulletinBoard parent */
  Widget drawing_area;       /* XmDrawingArea for table */
  Widget sb_horiz;           /* Horizontal scrollbar */
  Widget sb_vert;            /* Vertical scrollbar */
  Widget edit_text;          /* XmTextField for cell editing (created on demand) */

  /* X11 graphics */
  GC gc;                     /* Graphics context */
  XFontStruct* font_struct;  /* Current font (old X11) */
  int font_struct_owned;     /* 1 if we loaded font_struct ourselves, 0 if from cache */

#ifdef IUP_USE_XFT
  XftDraw* xft_draw;         /* XFT drawing context */
  XftFont* xft_font;         /* XFT font */
#endif

  /* Table geometry */
  int row_height;            /* Height of each row */
  int header_height;         /* Height of header row */
  int* col_widths;           /* Array of column widths [num_col] - displayed width (includes stretch) */
  int* col_natural_widths;   /* Array of natural column widths [num_col] - from auto-sizing, before stretch */
  int* col_width_set;        /* Array of flags: 1 if width explicitly set, 0 if auto [num_col] */

  /* Scroll state */
  int scroll_x;              /* Horizontal scroll position (pixels) */
  int scroll_y;              /* Vertical scroll position (pixels) */
  int first_visible_row;     /* First visible row (0-based) */
  int first_visible_col;     /* First visible column (0-based) */

  /* Selection/focus */
  int current_row;           /* Current focused row (1-based, 0=none) */
  int current_col;           /* Current focused column (1-based, 0=none) */

  /* Cell data storage (2D array) */
  char*** cell_values;       /* [num_lin][num_col] -> string */
  char** col_titles;         /* [num_col] -> string */

  /* Cell editing state */
  int edit_lin;              /* Row being edited (1-based, 0=not editing) */
  int edit_col;              /* Column being edited (1-based, 0=not editing) */

  /* Show grid flag */
  int show_grid;             /* 1 = show grid lines, 0 = no grid */

  /* Auto-sizing flag */
  int columns_autosized;     /* 1 = columns have been auto-sized, 0 = not yet */

  /* Sort state */
  int sort_column;           /* Currently sorted column (1-based, 0=none) */
  char* sort_signs;          /* Array of sort signs for each column [num_col] ("UP", "DOWN", or NULL) */

  /* Motif theme colors */
  Pixel bg_pixel;            /* Widget background color */
  Pixel fg_pixel;            /* Widget foreground color */
  Pixel header_bg_pixel;     /* Header background (derived from bg) */
  Pixel grid_pixel;          /* Grid line color (derived from bg) */
  Pixel select_bg_pixel;     /* Selection background color */

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

/* ========================================================================= */
/* Helper Functions - Sort Indicators                                       */
/* ========================================================================= */

/* Draw a simple triangle arrow for sort indicator */
static void motTableDrawSortArrow(Display* display, Window window, GC gc, int x, int y, int is_up)
{
  XPoint points[3];
  int arrow_size = 6;  /* Triangle side length */

  if (is_up)
  {
    /* Up arrow: triangle pointing up */
    points[0].x = x;                    points[0].y = y + arrow_size;
    points[1].x = x + arrow_size;       points[1].y = y + arrow_size;
    points[2].x = x + arrow_size / 2;   points[2].y = y;
  }
  else
  {
    /* Down arrow: triangle pointing down */
    points[0].x = x;                    points[0].y = y;
    points[1].x = x + arrow_size;       points[1].y = y;
    points[2].x = x + arrow_size / 2;   points[2].y = y + arrow_size;
  }

  /* Fill the triangle */
  XFillPolygon(display, window, gc, points, 3, Convex, CoordModeOrigin);
}

/* Sort rows based on the specified column */
static void motTableSortRows(Ihandle* ih, int col, int ascending)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int i, j;
  int num_rows = ih->data->num_lin;
  int num_cols = ih->data->num_col;

  if (!mot_data->cell_values || num_rows < 2 || col < 1 || col > num_cols)
    return;

  /* Simple bubble sort */
  for (i = 0; i < num_rows - 1; i++)
  {
    for (j = 0; j < num_rows - i - 1; j++)
    {
      const char* val1 = mot_data->cell_values[j][col - 1];
      const char* val2 = mot_data->cell_values[j + 1][col - 1];
      int should_swap = 0;

      /* Handle NULL values (treat as empty string) */
      if (!val1) val1 = "";
      if (!val2) val2 = "";

      /* Compare strings */
      int cmp = strcmp(val1, val2);

      /* Determine if swap is needed based on sort direction */
      if (ascending)
        should_swap = (cmp > 0);  /* Ascending: swap if val1 > val2 */
      else
        should_swap = (cmp < 0);  /* Descending: swap if val1 < val2 */

      if (should_swap)
      {
        /* Swap entire rows (all columns) */
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

  /* Non-virtual mode: Use internal storage */
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

  /* Free old value */
  if (mot_data->cell_values[lin-1][col-1])
  {
    free(mot_data->cell_values[lin-1][col-1]);
    mot_data->cell_values[lin-1][col-1] = NULL;
  }

  /* Store new value */
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

  /* Calculate X position (sum of column widths before this column) */
  for (c = 0; c < col - 1 && c < ih->data->num_col; c++)
    x += mot_data->col_widths[c];

  /* Calculate Y position */
  y = mot_data->header_height + (lin - 1) * mot_data->row_height;

  /* Apply scroll offset */
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

  /* Apply scroll offset */
  px += mot_data->scroll_x;
  py += mot_data->scroll_y;

  /* Find column */
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

  /* Find row */
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

  /* Get cell bounds */
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

  /* Draw cell background */
  char* bgcolor = NULL;

  /* Get background color with hierarchy: per-cell > per-column > per-row > alternating > default */
  if (!is_header)
  {
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);  /* Try L:C */
    if (!bgcolor)
      bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);  /* Try :C (per-column) */
    if (!bgcolor)
      bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);  /* Try L:* (per-row) */

    /* Check for alternating row colors if no explicit color is set */
    if (!bgcolor)
    {
      char* alternate_color = iupAttribGet(ih, "ALTERNATECOLOR");
      if (iupStrEqualNoCase(alternate_color, "YES"))
      {
        /* Use EVENROWCOLOR for even rows, ODDROWCOLOR for odd rows */
        if (lin % 2 == 0)
          bgcolor = iupAttribGetStr(ih, "EVENROWCOLOR");
        else
          bgcolor = iupAttribGetStr(ih, "ODDROWCOLOR");
      }
    }
  }

  if (is_focused_row)
  {
    /* Focused row, highlight entire row with selection color */
    XSetForeground(display, mot_data->gc, mot_data->select_bg_pixel);
  }
  else if (is_header)
  {
    /* Header background */
    XSetForeground(display, mot_data->gc, mot_data->header_bg_pixel);
  }
  else if (bgcolor && *bgcolor)
  {
    /* Custom cell background color */
    XSetForeground(display, mot_data->gc, iupmotColorGetPixelStr(bgcolor));
  }
  else
  {
    /* Default cell background */
    XSetForeground(display, mot_data->gc, mot_data->bg_pixel);
  }

  XFillRectangle(display, window, mot_data->gc, x, y, w, h);

  /* Draw grid lines */
  if (mot_data->show_grid)
  {
    XSetForeground(display, mot_data->gc, mot_data->grid_pixel);
    XDrawLine(display, window, mot_data->gc, x, y + h - 1, x + w, y + h - 1); /* Bottom */
    XDrawLine(display, window, mot_data->gc, x + w - 1, y, x + w - 1, y + h); /* Right */
  }

  /* Draw cell text */
  if (text && text[0])
  {
    char* fgcolor = NULL;

    /* Get foreground color with hierarchy: per-cell > per-column > per-row > default */
    if (!is_header)
    {
      fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);  /* Try L:C */
      if (!fgcolor)
        fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);  /* Try :C (per-column) */
      if (!fgcolor)
        fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);  /* Try L:* (per-row) */
    }

    /* Set text color */
    if (fgcolor && *fgcolor)
      XSetForeground(display, mot_data->gc, iupmotColorGetPixelStr(fgcolor));
    else
      XSetForeground(display, mot_data->gc, mot_data->fg_pixel);

    text_len = strlen(text);

    /* Calculate text width */
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
      text_width = text_len * 7; /* Fallback */

    /* Get column alignment (for both headers and cells) */
    char align_attr[64];
    char* align_str = NULL;
    sprintf(align_attr, "ALIGNMENT%d", col);
    align_str = iupAttribGet(ih, align_attr);

    /* Calculate text X position based on alignment */
    if (align_str && (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT")))
    {
      /* Right align */
      text_x = x + w - text_width - MOT_TABLE_CELL_PADDING;
    }
    else if (align_str && (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER")))
    {
      /* Center align */
      text_x = x + (w - text_width) / 2;
    }
    else
    {
      /* Left align (default) */
      text_x = x + MOT_TABLE_CELL_PADDING;
    }

    /* Calculate text Y position */
#ifdef IUP_USE_XFT
    if (mot_data->xft_font)
      text_y = y + h / 2 + mot_data->xft_font->ascent / 2;
    else
#endif
      text_y = y + h / 2 + (mot_data->font_struct ? mot_data->font_struct->ascent / 2 : 6);

    /* Draw text */
#ifdef IUP_USE_XFT
    if (mot_data->xft_draw && mot_data->xft_font)
    {
      XftColor xft_color;
      XRenderColor render_color;
      unsigned long pixel;
      XColor xcolor;

      /* Get pixel value from fgcolor */
      if (fgcolor && *fgcolor)
        pixel = iupmotColorGetPixelStr(fgcolor);
      else
        pixel = mot_data->fg_pixel;

      /* Convert pixel to XRenderColor */
      xcolor.pixel = pixel;
      XQueryColor(display, DefaultColormap(display, iupmot_screen), &xcolor);
      render_color.red = xcolor.red;
      render_color.green = xcolor.green;
      render_color.blue = xcolor.blue;
      render_color.alpha = 0xffff;

      XftColorAllocValue(display, DefaultVisual(display, iupmot_screen), DefaultColormap(display, iupmot_screen), &render_color, &xft_color);
      XftDrawString8(mot_data->xft_draw, &xft_color, mot_data->xft_font, text_x, text_y, (XftChar8*)text, text_len);
      XftColorFree(display, DefaultVisual(display, iupmot_screen), DefaultColormap(display, iupmot_screen), &xft_color);
    }
    else
#endif
      XDrawString(display, window, mot_data->gc, text_x, text_y, text, text_len);

    /* Draw sort arrow for sorted column headers (only if sorting is enabled) */
    if (is_header && ih->data->sortable && mot_data->sort_column == col && mot_data->sort_signs)
    {
      int arrow_x;
      int arrow_y = y + (h - 6) / 2;  /* Center vertically */

      /* Position arrow based on alignment */
      if (align_str && (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT")))
        arrow_x = x + MOT_TABLE_CELL_PADDING;  /* Left side for right-aligned text */
      else
        arrow_x = x + w - 12;  /* Right side for left/center-aligned text */

      /* Draw arrow based on sort direction */
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

  /* Draw focus rectangle around the specific focused cell if FOCUSRECT=YES */
  if (is_focused_cell && iupAttribGetBoolean(ih, "FOCUSRECT"))
  {
    /* Use XOR mode for focus rectangle, always creates contrast by inverting pixels */
    XSetFunction(display, mot_data->gc, GXxor);
    XSetForeground(display, mot_data->gc, mot_data->fg_pixel ^ mot_data->bg_pixel);

    /* Set dashed line style */
    char dash_list[] = {2, 2};  /* 2 pixels on, 2 pixels off */
    XSetLineAttributes(display, mot_data->gc, 1, LineOnOffDash, CapButt, JoinMiter);
    XSetDashes(display, mot_data->gc, 0, dash_list, 2);

    XDrawRectangle(display, window, mot_data->gc, x + 1, y + 1, w - 3, h - 3);

    /* Reset to normal drawing mode and solid line */
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

  /* Auto-size columns on first draw (when data is accessible) */
  if (!mot_data->columns_autosized)
  {
    int c, lin;
    int max_rows_to_check = (ih->data->num_lin > 100) ? 100 : ih->data->num_lin;  /* Limit for performance */

    for (c = 0; c < ih->data->num_col; c++)
    {
      int max_width = MOT_TABLE_DEF_COL_WIDTH;  /* Start with default */
      int title_width, cell_width;

      /* Check if user set explicit width for this column (via RASTERWIDTH or WIDTH) */
      if (mot_data->col_width_set[c])
      {
        /* User set explicit width, skip auto-sizing for this column */
        continue;
      }

      /* Measure column title */
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

      /* Measure cell content (check first N rows for performance) */
      for (lin = 0; lin < max_rows_to_check; lin++)
      {
        /* Use IupGetAttributeId2 to access cell values through the attribute system */
        const char* cell_value = IupGetAttributeId2(ih, "", lin + 1, c + 1);
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

      /* Update column width if it needs to be wider */
      if (max_width > mot_data->col_widths[c])
      {
        mot_data->col_widths[c] = max_width;
        mot_data->col_natural_widths[c] = max_width;
      }
    }

    /* Mark as done */
    mot_data->columns_autosized = 1;

    /* Update scrollbars since column widths may have changed */
    motTableUpdateScrollbars(ih);
  }

  /* Get widget size */
  XtVaGetValues(mot_data->drawing_area, XmNwidth, &width, XmNheight, &height, NULL);

  /* Clear background */
  XSetForeground(display, mot_data->gc, mot_data->bg_pixel);
  XFillRectangle(display, window, mot_data->gc, 0, 0, width, height);

  /* Draw header row */
  for (col = 1; col <= ih->data->num_col; col++)
  {
    motTableDrawCell(ih, 0, col, 1);
  }

  /* Draw data cells - ONLY VISIBLE ROWS for virtual rendering */
  {
    int first_visible_row, last_visible_row;
    int visible_height = height - mot_data->header_height;

    /* Calculate which rows are actually visible on screen */
    first_visible_row = (mot_data->scroll_y / mot_data->row_height) + 1;
    last_visible_row = ((mot_data->scroll_y + visible_height) / mot_data->row_height) + 1;

    /* Clamp to valid range */
    if (first_visible_row < 1) first_visible_row = 1;
    if (last_visible_row > ih->data->num_lin) last_visible_row = ih->data->num_lin;

    /* Only draw visible rows */
    for (lin = first_visible_row; lin <= last_visible_row; lin++)
    {
      for (col = 1; col <= ih->data->num_col; col++)
      {
        motTableDrawCell(ih, lin, col, 0);
      }
    }
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

  /* Call EDITBEGIN_CB - allow application to block editing */
  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    int ret = editbegin_cb(ih, lin, col);
    if (ret == IUP_IGNORE)
      return;  /* Block editing */
  }

  if (mot_data->edit_lin != 0)
    motTableEndCellEdit(ih, 1); /* End previous edit */

  /* Create edit widget if needed */
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

    /* Set font on edit widget to match the table's drawing font */
    fontlist_for_edit = (XmFontList)iupmotGetFontListAttrib(ih);
    if (fontlist_for_edit)
    {
      iupMOT_SETARG(args, num_args, XmNrenderTable, fontlist_for_edit);
      iupMOT_SETARG(args, num_args, XmNfontList, fontlist_for_edit);
    }

    mot_data->edit_text = XmCreateText(mot_data->container, "edit_text", args, num_args);

    /* Add key event handler for Enter/Escape */
    XtAddEventHandler(mot_data->edit_text, KeyPressMask, False, (XtEventHandler)motTableEditKeyPressCallback, (XtPointer)ih);
  }

  /* Position edit widget over cell */
  motTableCellToPixel(ih, lin, col, &x, &y, &w, &h);
  XtVaSetValues(mot_data->edit_text, XmNx, x, XmNy, y, XmNwidth, w, XmNheight, h, NULL);

  /* Set initial value */
  value = motTableGetCellValueInternal(ih, lin, col);
  XmTextSetString(mot_data->edit_text, (char*)(value ? value : ""));

  /* Show and focus edit widget */
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
    /* Apply changes and end editing */
    motTableEndCellEdit(ih, 1);
    *cont = False;  /* Stop event propagation */
  }
  else if (keysym == XK_Escape)
  {
    /* Cancel editing without applying changes */
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

  /* Call EDITEND_CB - allow application to validate/reject edit */
  IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb)
  {
    int ret = editend_cb(ih, lin, col, text, apply ? 1 : 0);
    if (ret == IUP_IGNORE && apply)
    {
      /* Application rejected the edit, keep editor open */
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

    /* Call VALUE_CB callback to update data source */
    IFniis value_cb = (IFniis)IupGetCallback(ih, "VALUE_CB");
    if (value_cb)
      value_cb(ih, lin, col, text);

    /* Call VALUECHANGED_CB only if text actually changed */
    int text_changed = 0;
    if (!old_text && text && *text)
      text_changed = 1;  /* NULL -> non-empty */
    else if (old_text && !text)
      text_changed = 1;  /* non-empty -> NULL */
    else if (old_text && text && strcmp(old_text, text) != 0)
      text_changed = 1;  /* different text */

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

  /* Restore focus to the table's drawing area */
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
      int scroll_amount = mot_data->row_height * 3;  /* Scroll 3 rows at a time */
      int new_scroll_y = mot_data->scroll_y;

      if (button_event->button == Button4)  /* Scroll up */
        new_scroll_y -= scroll_amount;
      else  /* Button5: Scroll down */
        new_scroll_y += scroll_amount;

      /* Clamp to valid range */
      if (new_scroll_y < 0)
        new_scroll_y = 0;

      mot_data->scroll_y = new_scroll_y;

      /* Update scrollbars (which will clamp scroll_y properly) and redraw */
      motTableUpdateScrollbars(ih);
      motTableRedraw(ih);
      return;
    }

    motTablePixelToCell(ih, button_event->x, button_event->y, &lin, &col);

    /* Check for column header click (sorting) */
    if (lin == 0 && col > 0)
    {
      /* Check if SORTABLE is enabled */
      if (ih->data->sortable)
      {
        /* Update sort state - toggle direction for same column, ascending for new column */
        if (mot_data->sort_column == col)
        {
          /* Same column - toggle direction */
          if (mot_data->sort_signs[col-1] == 1)
            mot_data->sort_signs[col-1] = -1;  /* UP -> DOWN */
          else
            mot_data->sort_signs[col-1] = 1;   /* DOWN -> UP */
        }
        else
        {
          /* New column - clear previous and set ascending */
          if (mot_data->sort_column > 0 && mot_data->sort_column <= ih->data->num_col)
            mot_data->sort_signs[mot_data->sort_column - 1] = 0;

          mot_data->sort_column = col;
          mot_data->sort_signs[col-1] = 1;  /* Start with ascending */
        }

        /* Check if user wants to handle sorting */
        IFni sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
        int sort_result = IUP_DEFAULT;

        if (sort_cb)
          sort_result = sort_cb(ih, col);

        /* If callback returned DEFAULT, perform automatic sorting */
        /* If callback returned IGNORE, user handled sorting themselves */
        if (sort_result == IUP_DEFAULT)
        {
          /* Perform automatic internal sorting */
          /* Check if in virtual mode (has VALUE_CB callback) */
          sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
          if (!value_cb)
          {
            /* In non-virtual mode, perform internal sorting */
            motTableSortRows(ih, col, (mot_data->sort_signs[col-1] == 1));
          }
        }

        /* Redraw to show updated sort indicators and sorted data */
        motTableRedraw(ih);
      }
    }
    else if (lin > 0 && col > 0)
    {
      /* End any active edit */
      if (mot_data->edit_lin != 0)
        motTableEndCellEdit(ih, 1);

      /* Update focus */
      mot_data->current_row = lin;
      mot_data->current_col = col;

      /* Call CLICK_CB */
      IFniis cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
      if (cb)
        cb(ih, lin, col, "1");

      /* Call ENTERITEM_CB for cell selection change */
      IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
      if (enteritem_cb)
        enteritem_cb(ih, lin, col);

      motTableRedraw(ih);

      /* Double-click handling */
      if (button_event->type == ButtonPress && button_event->button == Button1)
      {
        static Time last_click_time = 0;
        static int last_click_lin = 0, last_click_col = 0;

        if ((button_event->time - last_click_time) < 300 && lin == last_click_lin && col == last_click_col)
        {
          char name[50];
          char* editable;
          sprintf(name, "EDITABLE%d", col);
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

  (void)w;
}

static void motTableKeyPressCallback(Widget w, XtPointer client_data, XEvent* event, Boolean* cont)
{
  Ihandle* ih = (Ihandle*)client_data;
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  KeySym keysym = XLookupKeysym((XKeyEvent*)event, 0);
  int redraw = 0;

  *cont = True;

  /* If editing, let edit widget handle keys except Escape/Enter */
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
    return; /* Let edit widget handle other keys */
  }

  /* Arrow key navigation */
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
      sprintf(name, "EDITABLE%d", mot_data->current_col);
      editable = iupAttribGetStr(ih, name);
      if (!editable)
        editable = iupAttribGetStr(ih, "EDITABLE");

      if (iupStrBoolean(editable))
        motTableStartCellEdit(ih, mot_data->current_row, mot_data->current_col);
    }
  }
  else if ((keysym == XK_c || keysym == XK_C) && (((XKeyEvent*)event)->state & ControlMask))
  {
    /* Ctrl+C: Copy current cell to clipboard */
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
    /* Ctrl+V: Paste from clipboard to current cell */
    if (mot_data->current_row > 0 && mot_data->current_col > 0)
    {
      /* Check if cell is editable */
      char name[50];
      char* editable;
      sprintf(name, "EDITABLE%d", mot_data->current_col);
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

          /* Set the cell value internally */
          motTableSetCellValueInternal(ih, mot_data->current_row, mot_data->current_col, text);

          /* Call VALUE_CB to update data source (if it exists) */
          IFniis value_cb = (IFniis)IupGetCallback(ih, "VALUE_CB");
          if (value_cb)
            value_cb(ih, mot_data->current_row, mot_data->current_col, text);

          /* Trigger VALUECHANGED_CB callback only if text actually changed */
          int text_changed = 0;
          if (!old_text && text && *text)
            text_changed = 1;  /* NULL -> non-empty */
          else if (old_text && !text)
            text_changed = 1;  /* non-empty -> NULL */
          else if (old_text && text && strcmp(old_text, text) != 0)
            text_changed = 1;  /* different text */

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

  /* Get drawable area size */
  XtVaGetValues(mot_data->drawing_area, XmNwidth, &width, XmNheight, &height, NULL);

  /* Calculate natural total content size (before stretch) */
  for (c = 0; c < ih->data->num_col; c++)
  {
    natural_total_width += mot_data->col_natural_widths[c];
  }

  /* Handle STRETCHLAST: adjust last column to fill available space */
  if (ih->data->num_col > 0 && !mot_data->col_width_set[ih->data->num_col - 1] && ih->data->stretch_last)
  {
    int last_col = ih->data->num_col - 1;
    int other_cols_width = natural_total_width - mot_data->col_natural_widths[last_col];
    int available_for_last = width - other_cols_width;

    /* Use natural width as minimum */
    if (available_for_last < mot_data->col_natural_widths[last_col])
      available_for_last = mot_data->col_natural_widths[last_col];

    mot_data->col_widths[last_col] = available_for_last;
    total_width = other_cols_width + available_for_last;
  }
  else
  {
    /* No stretch, use natural widths */
    total_width = natural_total_width;
    for (c = 0; c < ih->data->num_col; c++)
      mot_data->col_widths[c] = mot_data->col_natural_widths[c];
  }

  total_height = mot_data->header_height + ih->data->num_lin * mot_data->row_height;

  /* Horizontal scrollbar */
  page_width = width;
  max_scroll_x = total_width - page_width;
  if (max_scroll_x < 0) max_scroll_x = 0;

  /* Ensure sliderSize doesn't exceed maximum - minimum */
  if (page_width > total_width)
    page_width = total_width;

  /* Clamp scroll position BEFORE setting it in the scrollbar */
  if (mot_data->scroll_x > max_scroll_x)
    mot_data->scroll_x = max_scroll_x;

  /* Show/hide horizontal scrollbar based on whether scrolling is needed */
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

  /* Vertical scrollbar */
  page_height = height;
  max_scroll_y = total_height - page_height;
  if (max_scroll_y < 0) max_scroll_y = 0;

  /* Ensure sliderSize doesn't exceed maximum - minimum */
  if (page_height > total_height)
    page_height = total_height;

  /* Clamp scroll position BEFORE setting it in the scrollbar */
  if (mot_data->scroll_y > max_scroll_y)
    mot_data->scroll_y = max_scroll_y;

  /* Show/hide vertical scrollbar based on whether scrolling is needed */
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

  /* Allocate Motif-specific data */
  mot_data = (ImotTableData*)calloc(1, sizeof(ImotTableData));
  if (!mot_data)
    return IUP_ERROR;

  ih->data->native_data = mot_data;

  /* Initialize geometry */
  mot_data->row_height = MOT_TABLE_DEF_ROW_HEIGHT;
  mot_data->header_height = MOT_TABLE_HEADER_HEIGHT;
  mot_data->show_grid = iupAttribGetBoolean(ih, "SHOWGRID");

  /* Allocate column widths */
  mot_data->col_widths = (int*)calloc(ih->data->num_col, sizeof(int));
  mot_data->col_natural_widths = (int*)calloc(ih->data->num_col, sizeof(int));
  mot_data->col_width_set = (int*)calloc(ih->data->num_col, sizeof(int));

  /* Initialize column widths, checking for explicit RASTERWIDTH/WIDTH */
  for (c = 0; c < ih->data->num_col; c++)
  {
    char name[50];
    sprintf(name, "RASTERWIDTH%d", c + 1);
    char* width_str = iupAttribGet(ih, name);
    if (!width_str)
    {
      sprintf(name, "WIDTH%d", c + 1);
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

  /* Initialize sort state */
  mot_data->sort_signs = (char*)calloc(ih->data->num_col, sizeof(char));
  mot_data->sort_column = 0;

  /* Allocate cell storage (skip in virtual mode) */
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

  /* Allocate column titles */
  mot_data->col_titles = (char**)calloc(ih->data->num_col, sizeof(char*));
  for (c = 0; c < ih->data->num_col; c++)
  {
    char default_title[32];
    sprintf(default_title, "Col %d", c + 1);
    mot_data->col_titles[c] = iupStrDup(default_title);
  }

  /* Create container (XmBulletinBoard) */
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

  /* Store container for layout updates */
  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)mot_data->container);

  /* Create drawing area */
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

  /* Create horizontal scrollbar */
  mot_data->sb_horiz = XtVaCreateManagedWidget("sb_horiz", xmScrollBarWidgetClass, mot_data->container, XmNorientation, XmHORIZONTAL, NULL);

  XtAddCallback(mot_data->sb_horiz, XmNvalueChangedCallback, motTableScrollCallback, (XtPointer)ih);
  XtAddCallback(mot_data->sb_horiz, XmNdragCallback, motTableScrollCallback, (XtPointer)ih);

  /* Create vertical scrollbar */
  mot_data->sb_vert = XtVaCreateManagedWidget("sb_vert", xmScrollBarWidgetClass, mot_data->container, XmNorientation, XmVERTICAL, NULL);

  XtAddCallback(mot_data->sb_vert, XmNvalueChangedCallback, motTableScrollCallback, (XtPointer)ih);
  XtAddCallback(mot_data->sb_vert, XmNdragCallback, motTableScrollCallback, (XtPointer)ih);

  /* Add callbacks to drawing area */
  XtAddCallback(mot_data->drawing_area, XmNexposeCallback, motTableExposeCallback, (XtPointer)ih);
  XtAddCallback(mot_data->drawing_area, XmNresizeCallback, motTableResizeCallback, (XtPointer)ih);
  XtAddCallback(mot_data->drawing_area, XmNinputCallback, motTableInputCallback, (XtPointer)ih);

  XtAddEventHandler(mot_data->drawing_area, KeyPressMask, False, (XtEventHandler)motTableKeyPressCallback, (XtPointer)ih);
  XtAddEventHandler(mot_data->drawing_area, FocusChangeMask, False, (XtEventHandler)iupmotFocusChangeEvent, (XtPointer)ih);
  XtAddEventHandler(mot_data->drawing_area, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(mot_data->drawing_area, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);

  /* Realize widgets */
  XtRealizeWidget(mot_data->container);

  /* Query Motif widget colors */
  {
    unsigned char bg_r, bg_g, bg_b;

    XtVaGetValues(mot_data->container, XmNbackground, &mot_data->bg_pixel, XmNforeground, &mot_data->fg_pixel, NULL);

    /* Get RGB components of background color */
    iupmotColorGetRGB(mot_data->bg_pixel, &bg_r, &bg_g, &bg_b);

    /* Derive header color (slightly darker than background) */
    {
      unsigned char header_r = (bg_r > 20) ? bg_r - 20 : 0;
      unsigned char header_g = (bg_g > 20) ? bg_g - 20 : 0;
      unsigned char header_b = (bg_b > 20) ? bg_b - 20 : 0;
      mot_data->header_bg_pixel = iupmotColorGetPixel(header_r, header_g, header_b);
    }

    /* Derive grid color (darker than background) */
    {
      unsigned char grid_r = (bg_r > 40) ? bg_r - 40 : 0;
      unsigned char grid_g = (bg_g > 40) ? bg_g - 40 : 0;
      unsigned char grid_b = (bg_b > 40) ? bg_b - 40 : 0;
      mot_data->grid_pixel = iupmotColorGetPixel(grid_r, grid_g, grid_b);
    }

    /* Derive selection color (tinted blue version of background) */
    {
      unsigned char select_r = (unsigned char)((bg_r * 3 + 200) / 4);
      unsigned char select_g = (unsigned char)((bg_g * 3 + 220) / 4);
      unsigned char select_b = (unsigned char)((bg_b * 1 + 255 * 3) / 4);
      mot_data->select_bg_pixel = iupmotColorGetPixel(select_r, select_g, select_b);
    }
  }

  /* Create graphics context */
  gcvalues.foreground = BlackPixel(iupmot_display, iupmot_screen);
  gcvalues.background = WhitePixel(iupmot_display, iupmot_screen);
  mot_data->gc = XCreateGC(iupmot_display, XtWindow(mot_data->drawing_area), GCForeground | GCBackground, &gcvalues);

  /* Load font */
#ifdef IUP_USE_XFT
  mot_data->xft_font = (XftFont*)iupmotGetXftFontAttrib(ih);
  if (mot_data->xft_font)
  {
    /* Create XFT drawing context */
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
      mot_data->font_struct_owned = 1;  /* We loaded it, we own it */
    }
    else
    {
      mot_data->font_struct_owned = 0;  /* From cache, don't free */
    }
    if (mot_data->font_struct)
      XSetFont(iupmot_display, mot_data->gc, mot_data->font_struct->fid);
  }

  /* Set initial focus - no row/col selected on start (0 = none) */
  mot_data->current_row = 0;
  mot_data->current_col = 0;

  /* Initialize auto-sizing flag (will run on first expose) */
  mot_data->columns_autosized = 0;

  /* Store constraints for VISIBLELINES/VISIBLECOLUMNS clamping in LayoutUpdate */
  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    int row_height = iupdrvTableGetRowHeight(ih);
    int header_height = iupdrvTableGetHeaderHeight(ih);
    int border = iupdrvTableGetBorderWidth(ih);
    int sb_size = iupdrvGetScrollbarSize();

    /* Include horizontal scrollbar height, motTableSetSize reserves space for it,
       and vertical scrollbar (from VISIBLELINES) can trigger horizontal scrollbar */
    mot_data->target_height = header_height + (row_height * visiblelines) + border + sb_size;
  }
  else
  {
    mot_data->target_height = 0;
  }

  /* Store VISIBLECOLUMNS constraint */
  mot_data->visible_columns = iupAttribGetInt(ih, "VISIBLECOLUMNS");

  /* Update scrollbars */
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

  /* Get border width */
  XtVaGetValues(container, XmNborderWidth, &border, NULL);
  width = use_width - 2*(int)border;
  height = use_height - 2*(int)border;

  /* Ensure minimum size */
  if (width <= 0) width = 1;
  if (height <= 0) height = 1;

  /* Set container size */
  if (setsize)
  {
    XtVaSetValues(container, XmNwidth, (XtArgVal)width, XmNheight, (XtArgVal)height, NULL);
  }

  /* Position and size the drawing area */
  XtVaSetValues(mot_data->drawing_area, XmNx, 0, XmNy, 0, XmNwidth, width - sb_size, XmNheight, height - sb_size, NULL);

  /* Position and size the horizontal scrollbar */
  XtVaSetValues(mot_data->sb_horiz, XmNx, 0, XmNy, height - sb_size, XmNwidth, width - sb_size, XmNheight, sb_size, NULL);

  /* Position and size the vertical scrollbar */
  XtVaSetValues(mot_data->sb_vert, XmNx, width - sb_size, XmNy, 0, XmNwidth, sb_size, XmNheight, height - sb_size, NULL);

  /* Update scrollbars */
  motTableUpdateScrollbars(ih);
}

static void motTableLayoutUpdateMethod(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  Widget container = (Widget)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  int width = ih->currentwidth;
  int height = ih->currentheight;

  /* If VISIBLELINES is set, clamp height to target */
  if (mot_data && mot_data->target_height > 0)
  {
    if (height > mot_data->target_height)
      height = mot_data->target_height;
  }

  /* If VISIBLECOLUMNS is set, clamp width to show exactly N columns */
  if (mot_data && mot_data->visible_columns > 0 && mot_data->col_widths)
  {
    int c, cols_width = 0;
    int num_cols = mot_data->visible_columns;
    if (num_cols > ih->data->num_col)
      num_cols = ih->data->num_col;

    /* Sum up the widths of the visible columns */
    for (c = 0; c < num_cols; c++)
      cols_width += mot_data->col_widths[c];

    int sb_size = iupdrvGetScrollbarSize();
    int border = iupdrvTableGetBorderWidth(ih);

    /* Only add vertical scrollbar width if it will actually be visible */
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

  /* Free graphics context */
  if (mot_data->gc)
    XFreeGC(iupmot_display, mot_data->gc);

  /* Free XFT resources */
#ifdef IUP_USE_XFT
  if (mot_data->xft_draw)
    XftDrawDestroy(mot_data->xft_draw);
#endif

  /* Free font only if we loaded it ourselves (not from cache) */
  if (mot_data->font_struct && mot_data->font_struct_owned)
    XFreeFont(iupmot_display, mot_data->font_struct);

  /* Free cell storage */
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

  /* Free column titles */
  if (mot_data->col_titles)
  {
    for (c = 0; c < ih->data->num_col; c++)
    {
      if (mot_data->col_titles[c])
        free(mot_data->col_titles[c]);
    }
    free(mot_data->col_titles);
  }

  /* Free column widths */
  if (mot_data->col_widths)
    free(mot_data->col_widths);

  if (mot_data->col_natural_widths)
    free(mot_data->col_natural_widths);

  if (mot_data->col_width_set)
    free(mot_data->col_width_set);

  /* Free sort signs array */
  if (mot_data->sort_signs)
    free(mot_data->sort_signs);

  /* Destroy widgets */
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

void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  /* Not implemented - would require reallocation */
  (void)ih;
  (void)num_lin;
}

void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  int old_num_col, i;

  if (!mot_data || num_col <= 0)
    return;

  old_num_col = ih->data->num_col;
  if (num_col == old_num_col)
    return;

  /* Reallocate column widths */
  mot_data->col_widths = (int*)realloc(mot_data->col_widths, num_col * sizeof(int));
  mot_data->col_natural_widths = (int*)realloc(mot_data->col_natural_widths, num_col * sizeof(int));
  mot_data->col_width_set = (int*)realloc(mot_data->col_width_set, num_col * sizeof(int));
  for (i = old_num_col; i < num_col; i++)
  {
    mot_data->col_widths[i] = MOT_TABLE_DEF_COL_WIDTH;
    mot_data->col_natural_widths[i] = MOT_TABLE_DEF_COL_WIDTH;
    mot_data->col_width_set[i] = 0;
  }

  /* Reallocate column titles */
  mot_data->col_titles = (char**)realloc(mot_data->col_titles, num_col * sizeof(char*));
  for (i = old_num_col; i < num_col; i++)
    mot_data->col_titles[i] = NULL;

  /* Reallocate cell_values for each row */
  for (i = 0; i < ih->data->num_lin; i++)
  {
    mot_data->cell_values[i] = (char**)realloc(mot_data->cell_values[i], num_col * sizeof(char*));
    /* Initialize new cells to NULL */
    int j;
    for (j = old_num_col; j < num_col; j++)
      mot_data->cell_values[i][j] = NULL;
  }

  /* Update num_col */
  ih->data->num_col = num_col;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

void iupdrvTableAddLin(Ihandle* ih, int pos)
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

  /* Reallocate cell_values array */
  mot_data->cell_values = (char***)realloc(mot_data->cell_values, new_num_lin * sizeof(char**));

  /* Shift rows down to make room for new row */
  for (lin = new_num_lin - 1; lin > pos; lin--)
  {
    mot_data->cell_values[lin] = mot_data->cell_values[lin - 1];
  }

  /* Allocate new row */
  mot_data->cell_values[pos] = (char**)calloc(ih->data->num_col, sizeof(char*));

  /* Update num_lin */
  ih->data->num_lin = new_num_lin;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

void iupdrvTableDelLin(Ihandle* ih, int pos)
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

  /* Free cell values in the row being deleted */
  for (col = 0; col < ih->data->num_col; col++)
  {
    if (mot_data->cell_values[pos][col])
      free(mot_data->cell_values[pos][col]);
  }
  free(mot_data->cell_values[pos]);

  /* Shift rows up */
  new_num_lin = ih->data->num_lin - 1;
  for (lin = pos; lin < new_num_lin; lin++)
  {
    mot_data->cell_values[lin] = mot_data->cell_values[lin + 1];
  }

  /* Reallocate to smaller size */
  if (new_num_lin > 0)
    mot_data->cell_values = (char***)realloc(mot_data->cell_values, new_num_lin * sizeof(char**));
  else
  {
    free(mot_data->cell_values);
    mot_data->cell_values = NULL;
  }

  /* Update num_lin */
  ih->data->num_lin = new_num_lin;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

void iupdrvTableAddCol(Ihandle* ih, int pos)
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

  /* Reallocate column widths */
  mot_data->col_widths = (int*)realloc(mot_data->col_widths, new_num_col * sizeof(int));
  mot_data->col_natural_widths = (int*)realloc(mot_data->col_natural_widths, new_num_col * sizeof(int));
  mot_data->col_width_set = (int*)realloc(mot_data->col_width_set, new_num_col * sizeof(int));
  /* Shift widths to make room */
  for (col = new_num_col - 1; col > pos; col--)
  {
    mot_data->col_widths[col] = mot_data->col_widths[col - 1];
    mot_data->col_natural_widths[col] = mot_data->col_natural_widths[col - 1];
    mot_data->col_width_set[col] = mot_data->col_width_set[col - 1];
  }
  mot_data->col_widths[pos] = MOT_TABLE_DEF_COL_WIDTH;
  mot_data->col_natural_widths[pos] = MOT_TABLE_DEF_COL_WIDTH;
  mot_data->col_width_set[pos] = 0;

  /* Reallocate column titles */
  mot_data->col_titles = (char**)realloc(mot_data->col_titles, new_num_col * sizeof(char*));
  /* Shift titles to make room */
  for (col = new_num_col - 1; col > pos; col--)
  {
    mot_data->col_titles[col] = mot_data->col_titles[col - 1];
  }
  mot_data->col_titles[pos] = NULL;

  /* Add column to each row */
  for (lin = 0; lin < ih->data->num_lin; lin++)
  {
    mot_data->cell_values[lin] = (char**)realloc(mot_data->cell_values[lin], new_num_col * sizeof(char*));
    /* Shift cells to make room */
    for (col = new_num_col - 1; col > pos; col--)
    {
      mot_data->cell_values[lin][col] = mot_data->cell_values[lin][col - 1];
    }
    mot_data->cell_values[lin][pos] = NULL;
  }

  /* Update num_col */
  ih->data->num_col = new_num_col;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

void iupdrvTableDelCol(Ihandle* ih, int pos)
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

  /* Free column title */
  if (mot_data->col_titles[pos])
    free(mot_data->col_titles[pos]);

  /* Free cell values in column and shift */
  for (lin = 0; lin < ih->data->num_lin; lin++)
  {
    if (mot_data->cell_values[lin][pos])
      free(mot_data->cell_values[lin][pos]);

    /* Shift cells left */
    for (col = pos; col < new_num_col; col++)
    {
      mot_data->cell_values[lin][col] = mot_data->cell_values[lin][col + 1];
    }

    /* Reallocate row to smaller size */
    if (new_num_col > 0)
      mot_data->cell_values[lin] = (char**)realloc(mot_data->cell_values[lin], new_num_col * sizeof(char*));
    else
    {
      free(mot_data->cell_values[lin]);
      mot_data->cell_values[lin] = NULL;
    }
  }

  /* Shift column widths left */
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

  /* Shift column titles left */
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

  /* Update num_col */
  ih->data->num_col = new_num_col;

  motTableUpdateScrollbars(ih);
  motTableRedraw(ih);
}

void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  motTableSetCellValueInternal(ih, lin, col, value);
  motTableRedraw(ih);
}

char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  return motTableGetCellValueInternal(ih, lin, col);
}

void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || col < 1 || col > ih->data->num_col)
    return;

  if (mot_data->col_titles[col-1])
    free(mot_data->col_titles[col-1]);

  mot_data->col_titles[col-1] = title ? iupStrDup(title) : NULL;
  motTableRedraw(ih);
}

char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || col < 1 || col > ih->data->num_col)
    return NULL;

  return mot_data->col_titles[col-1];
}

void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
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

int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data || col < 1 || col > ih->data->num_col)
    return 0;

  return mot_data->col_widths[col-1];
}

void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
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

void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data)
    return;

  if (lin) *lin = mot_data->current_row;
  if (col) *col = mot_data->current_col;
}

void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  /* Not implemented */
  (void)ih;
  (void)lin;
  (void)col;
}

void iupdrvTableRedraw(Ihandle* ih)
{
  motTableRedraw(ih);
}

/* ========================================================================= */
/* Attribute Handlers                                                        */
/* ========================================================================= */

static int motTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  /* Store value in ih->data first */
  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
    ih->data->sortable = 0;

  /* Redraw to show/hide sort arrows */
  if (ih->handle)
  {
    motTableRedraw(ih);
  }

  return 0;  /* Do not store in hash table */
}

void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);

  if (!mot_data)
    return;

  mot_data->show_grid = show;
  motTableRedraw(ih);
}

int iupdrvTableGetBorderWidth(Ihandle* ih)
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

int iupdrvTableGetRowHeight(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  if (mot_data && mot_data->row_height > 0)
    return mot_data->row_height;

  /* Fallback before mapping */
  return MOT_TABLE_DEF_ROW_HEIGHT;
}

int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  ImotTableData* mot_data = IMOT_TABLE_DATA(ih);
  if (mot_data && mot_data->header_height > 0)
    return mot_data->header_height;

  /* Fallback before mapping */
  return MOT_TABLE_HEADER_HEIGHT;
}

void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  int sb_size = iupdrvGetScrollbarSize();
  int border = iupdrvTableGetBorderWidth(ih);
  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

  /* Motif: add scrollbar width + border */
  *w += sb_size + border;

  /* Vertical border */
  *h += border;

  /* Add horizontal scrollbar height when it will be needed.
     motTableSetSize always reserves space for horizontal scrollbar,
     so we must account for it in natural size calculation. */
  if (visiblecolumns > 0 && ih->data->num_col > visiblecolumns)
  {
    /* VISIBLECOLUMNS will cause horizontal scrollbar */
    *h += sb_size;
  }
  else if (visiblelines == 0)
  {
    /* No VISIBLELINES constraint - natural size must account for
       the horizontal scrollbar space that motTableSetSize reserves */
    *h += sb_size;
  }
  /* When VISIBLELINES is set (without VISIBLECOLUMNS causing scrollbar),
     target_height in MapMethod already includes sb_size and will clamp the height */
}

/* ========================================================================= */
/* Class Initialization                                                      */
/* ========================================================================= */

void iupdrvTableInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motTableMapMethod;
  ic->UnMap = motTableUnMapMethod;
  ic->LayoutUpdate = motTableLayoutUpdateMethod;

  /* Register FONT attribute */
  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE | IUPAF_NOT_MAPPED);

  /* Mark unsupported features */
  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "USERRESIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  /* Replace core SET handlers to update native widget */
  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, motTableSetSortableAttrib);

}
