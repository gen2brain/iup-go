/** \file
 * \brief Table Control - Windows driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <commctrl.h>
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
#include "iup_key.h"
#include "iup_tablecontrol.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_draw.h"
#include "iupwin_str.h"


/****************************************************************************
 * Data Structure
 ****************************************************************************/

typedef struct _IwinTableData {
  HWND list_view;              /* Main ListView control */

  /* Column metadata */
  int* col_widths;             /* Width of each column (pixels) */
  BOOL* col_width_set;         /* TRUE if column has explicit RASTERWIDTH set */
  char** col_titles;           /* Column header texts */

  /* Cell storage (normal mode) */
  char*** cell_values;         /* [num_lin][num_col] -> string */

  /* Current cell tracking */
  int current_row;             /* Currently focused row (1-based, 0=none) */
  int current_col;             /* Currently focused column (1-based, 0=none) */

  /* Sorting state */
  int sort_column;             /* Currently sorted column (1-based, 0=none) */
  char sort_ascending;         /* Sort direction: 1=ascending, -1=descending, 0=not sorted */

  /* UI state */
  BOOL show_grid;              /* Grid lines visible */
  HFONT hfont;                 /* Current font */

  /* Editing state */
  HWND edit_control;           /* Edit control for cell editing */
  int edit_row;                /* Row being edited (1-based) */
  int edit_col;                /* Column being edited (1-based) */
  BOOL edit_ending;            /* Flag to prevent WM_KILLFOCUS interference */

  /* Suppress callbacks during programmatic changes */
  int suppress_callbacks;
} IwinTableData;

#define IWIN_TABLE_DATA(ih) ((IwinTableData*)(ih->data->native_data))

/****************************************************************************
 * Utilities
 ****************************************************************************/

static HWND winTableGetListView(Ihandle* ih)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  return data ? data->list_view : NULL;
}

static void winTableFreeCell(char** cell)
{
  if (cell && *cell)
  {
    free(*cell);
    *cell = NULL;
  }
}

static void winTableSetCell(char** cell, const char* value)
{
  if (*cell)
    free(*cell);

  if (value && value[0])
    *cell = iupStrDup(value);
  else
    *cell = NULL;
}

static void winTableAdjustColumnWidths(Ihandle* ih)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return;

  int num_col = ih->data->num_col;
  int last_col_idx = num_col - 1;

  HDC hdc = GetDC(list_view);
  HFONT hFont = (HFONT)SendMessage(list_view, WM_GETFONT, 0, 0);
  HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

  /* Auto-size columns that don't have explicit width (skip last column) */
  for (int i = 0; i < num_col - 1; i++)
  {
    if (!data->col_width_set[i])
    {
      /* Try auto-size based on header and content */
      ListView_SetColumnWidth(list_view, i + 1, LVSCW_AUTOSIZE_USEHEADER);
      int header_width = ListView_GetColumnWidth(list_view, i + 1);

      /* Also calculate based on title text if available */
      int title_width = 50;  /* Minimum width */
      if (data->col_titles[i])
      {
        SIZE size;
        if (GetTextExtentPoint32A(hdc, data->col_titles[i], strlen(data->col_titles[i]), &size))
        {
          title_width = size.cx + 20;  /* Add padding for sort arrow, margins */
        }
      }

      /* Use the larger of the two, with a reasonable minimum */
      int auto_width = (header_width > title_width) ? header_width : title_width;

      /* Only enforce minimum if column is reasonably wide already */
      if (auto_width < 50)
      {
        /* Very short column - use natural width with small padding */
        if (auto_width < 30)
          auto_width = 30;
      }
      else if (auto_width < 80)
      {
        /* Medium column - enforce reasonable minimum */
        auto_width = 80;
      }

      ListView_SetColumnWidth(list_view, i + 1, auto_width);
      data->col_widths[i] = auto_width;
    }
    else
    {
      /* Use explicit width */
      ListView_SetColumnWidth(list_view, i + 1, data->col_widths[i]);
    }
  }

  SelectObject(hdc, hOldFont);
  ReleaseDC(list_view, hdc);

  /* Handle last column, stretch to fill remaining space */
  RECT rect;
  GetClientRect(list_view, &rect);
  int list_width = rect.right - rect.left;

  /* Calculate total width of all columns except last */
  int used_width = 0;
  for (int i = 0; i < last_col_idx; i++)
    used_width += data->col_widths[i];

  /* Handle last column */
  int last_col_width;

  if (!data->col_width_set[last_col_idx])
  {
    /* Auto-size to get content width */
    ListView_SetColumnWidth(list_view, num_col, LVSCW_AUTOSIZE_USEHEADER);
    int auto_width = ListView_GetColumnWidth(list_view, num_col);

    /* Also try LVSCW_AUTOSIZE (content only, no header) */
    ListView_SetColumnWidth(list_view, num_col, LVSCW_AUTOSIZE);
    int content_width = ListView_GetColumnWidth(list_view, num_col);

    /* Use the smaller of the two */
    auto_width = (content_width < auto_width) ? content_width : auto_width;

    /* Calculate based on title text too */
    if (data->col_titles[last_col_idx])
    {
      HDC hdc2 = GetDC(list_view);
      HFONT hFont2 = (HFONT)SendMessage(list_view, WM_GETFONT, 0, 0);
      HFONT hOldFont2 = (HFONT)SelectObject(hdc2, hFont2);
      SIZE size;
      if (GetTextExtentPoint32A(hdc2, data->col_titles[last_col_idx], strlen(data->col_titles[last_col_idx]), &size))
      {
        int title_width = size.cx + 20;
        if (title_width > auto_width)
          auto_width = title_width;
      }
      SelectObject(hdc2, hOldFont2);
      ReleaseDC(list_view, hdc2);
    }

    if (ih->data->stretch_last)
    {
      /* Stretching enabled, apply minimum width and stretch to fill */
      if (auto_width < 50)
      {
        if (auto_width < 30)
          auto_width = 30;
      }
      else if (auto_width < 80)
      {
        auto_width = 80;
      }

      int remaining = list_width - used_width;
      last_col_width = (remaining > auto_width) ? remaining : auto_width;
    }
    else
    {
      /* No stretching, use auto width as-is (like other columns) */
      last_col_width = auto_width;
    }
  }
  else
  {
    /* Explicit width set, use it */
    last_col_width = data->col_widths[last_col_idx];
  }

  ListView_SetColumnWidth(list_view, num_col, last_col_width);
  data->col_widths[last_col_idx] = last_col_width;
}

/****************************************************************************
 * Sorting
 ****************************************************************************/

/* Update header column with sort arrow indicator */
static void winTableUpdateSortArrow(Ihandle* ih, int col)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view || col < 1 || col > ih->data->num_col)
    return;

  HWND header = ListView_GetHeader(list_view);
  if (!header)
    return;

  /* Update all column headers (skip dummy column at index 0) */
  for (int i = 1; i <= ih->data->num_col; i++)
  {
    HDITEM hdi;
    ZeroMemory(&hdi, sizeof(HDITEM));
    hdi.mask = HDI_FORMAT;

    /* Get current format */
    if (Header_GetItem(header, i, &hdi))
    {
      /* Clear existing sort arrows */
      hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);

      /* Add sort arrow if this is the sorted column */
      if (i == col)
      {
        if (data->sort_ascending == 1)
          hdi.fmt |= HDF_SORTUP;
        else if (data->sort_ascending == -1)
          hdi.fmt |= HDF_SORTDOWN;
      }

      /* Apply updated format */
      Header_SetItem(header, i, &hdi);
    }
  }
}

/* Sort rows in cell_values array based on specified column */
static void winTableSortRows(Ihandle* ih, int col, int ascending)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  int num_rows = ih->data->num_lin;
  int num_cols = ih->data->num_col;

  if (!data->cell_values || num_rows < 2 || col < 1 || col > num_cols)
    return;

  /* Simple bubble sort */
  for (int i = 0; i < num_rows - 1; i++)
  {
    for (int j = 0; j < num_rows - i - 1; j++)
    {
      const char* val1 = data->cell_values[j][col - 1];
      const char* val2 = data->cell_values[j + 1][col - 1];
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
        char** temp_row = data->cell_values[j];
        data->cell_values[j] = data->cell_values[j + 1];
        data->cell_values[j + 1] = temp_row;
      }
    }
  }
}

/* Perform sorting on the table */
static void winTableSort(Ihandle* ih, int col)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view || col < 1 || col > ih->data->num_col)
    return;

  /* Toggle sort direction for same column, ascending for new column */
  if (data->sort_column == col)
  {
    /* Same column, toggle direction */
    if (data->sort_ascending == 1)
      data->sort_ascending = -1;  /* UP -> DOWN */
    else
      data->sort_ascending = 1;   /* DOWN -> UP */
  }
  else
  {
    /* New column, start with ascending */
    data->sort_column = col;
    data->sort_ascending = 1;
  }

  /* Update header with sort arrow */
  winTableUpdateSortArrow(ih, col);

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
    char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
    if (!iupStrBoolean(virtualmode))
    {
      /* In non-virtual mode, perform internal sorting */
      /* Sort the cell_values array */
      winTableSortRows(ih, col, (data->sort_ascending == 1));

      /* Update ListView display with sorted data */
      for (int i = 0; i < ih->data->num_lin; i++)
      {
        for (int j = 0; j < ih->data->num_col; j++)
        {
          const char* value = data->cell_values[i][j];
          LVITEM item;
          ZeroMemory(&item, sizeof(LVITEM));
          item.mask = LVIF_TEXT;
          item.iItem = i;
          item.iSubItem = j + 1;  /* Skip dummy column at index 0 */
          item.pszText = iupwinStrToSystem(value ? value : "");
          ListView_SetItem(list_view, &item);
        }
      }

      /* Redraw to reflect changes */
      InvalidateRect(list_view, NULL, TRUE);
    }
  }
}

/****************************************************************************
 * Cell Operations
 ****************************************************************************/

void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  /* Check if in virtual mode */
  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (!iupStrBoolean(virtualmode))
  {
    /* Normal mode: store in cell_values */
    winTableSetCell(&data->cell_values[lin-1][col-1], value);
  }

  /* Update ListView display */
  LVITEM item;
  ZeroMemory(&item, sizeof(LVITEM));
  item.mask = LVIF_TEXT;
  item.iItem = lin - 1;  /* Convert to 0-based */
  item.iSubItem = col;  /* Dummy column at 0, real columns at 1..num_col */
  item.pszText = iupwinStrToSystem(value ? value : "");
  ListView_SetItem(list_view, &item);
}

char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return NULL;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return NULL;

  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (iupStrBoolean(virtualmode))
  {
    /* Virtual mode: call VALUE_CB */
    sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
    if (value_cb)
      return value_cb(ih, lin, col);
    return NULL;
  }
  else
  {
    /* Normal mode: return from cell_values */
    return data->cell_values[lin-1][col-1];
  }
}

/****************************************************************************
 * Column Operations
 ****************************************************************************/

void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return;

  if (col < 1 || col > ih->data->num_col)
    return;

  /* Store title */
  if (data->col_titles[col-1])
    free(data->col_titles[col-1]);
  data->col_titles[col-1] = title ? iupStrDup(title) : NULL;

  /* Update ListView column header with alignment */
  LVCOLUMN lvc;
  ZeroMemory(&lvc, sizeof(LVCOLUMN));
  lvc.mask = LVCF_TEXT | LVCF_FMT;
  lvc.pszText = iupwinStrToSystem(title ? title : "");

  /* Apply alignment from ALIGNMENT attribute */
  {
    /* Check for ALIGNMENT attribute */
    char name[50];
    sprintf(name, "ALIGNMENT%d", col);
    char* align_str = iupAttribGet(ih, name);

    if (align_str)
    {
      if (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT"))
        lvc.fmt = LVCFMT_RIGHT;
      else if (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER"))
        lvc.fmt = LVCFMT_CENTER;
      else
        lvc.fmt = LVCFMT_LEFT;
    }
    else
    {
      /* Default: left-aligned */
      lvc.fmt = LVCFMT_LEFT;
    }
  }

  ListView_SetColumn(list_view, col, &lvc);
}

char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);

  if (!data)
    return NULL;

  if (col < 1 || col > ih->data->num_col)
    return NULL;

  return data->col_titles[col-1];
}

void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return;

  if (col < 1 || col > ih->data->num_col)
    return;

  /* Mark column as having explicit width set */
  data->col_widths[col-1] = width;
  data->col_width_set[col-1] = TRUE;

  /* Set fixed width */
  ListView_SetColumnWidth(list_view, col, width);

  /* Re-adjust last column if needed */
  winTableAdjustColumnWidths(ih);
}

int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return 0;

  if (col < 1 || col > ih->data->num_col)
    return 0;

  return ListView_GetColumnWidth(list_view, col);
}

/****************************************************************************
 * Selection
 ****************************************************************************/

static void winTableInvalidateCell(HWND list_view, int lin, int col)
{
  if (lin <= 0 || col <= 0)
    return;

  RECT rc;
  ListView_GetSubItemRect(list_view, lin - 1, col, LVIR_BOUNDS, &rc);
  InvalidateRect(list_view, &rc, FALSE);
}

void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return;

  if (lin < 0 || col < 0)
    return;

  if (lin > ih->data->num_lin || col > ih->data->num_col)
    return;

  /* Invalidate old focused cell to remove focus rect */
  winTableInvalidateCell(list_view, data->current_row, data->current_col);

  data->current_row = lin;
  data->current_col = col;

  if (lin == 0 || col == 0)
  {
    /* Clear selection */
    ListView_SetItemState(list_view, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    return;
  }

  /* Suppress ENTERITEM_CB during programmatic selection */
  data->suppress_callbacks = 1;

  /* Set focus and selection on the row */
  ListView_SetItemState(list_view, lin - 1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

  /* Ensure visible */
  ListView_EnsureVisible(list_view, lin - 1, FALSE);

  /* Invalidate new focused cell to draw focus rect */
  winTableInvalidateCell(list_view, lin, col);

  data->suppress_callbacks = 0;
}

void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
  {
    *lin = 0;
    *col = 0;
    return;
  }

  *lin = data->current_row;
  *col = data->current_col;
}

/****************************************************************************
 * Scrolling
 ****************************************************************************/

void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  HWND list_view = winTableGetListView(ih);

  if (!list_view)
    return;

  if (lin < 1 || lin > ih->data->num_lin)
    return;

  ListView_EnsureVisible(list_view, lin - 1, FALSE);
}

/****************************************************************************
 * Display
 ****************************************************************************/

void iupdrvTableRedraw(Ihandle* ih)
{
  HWND list_view = winTableGetListView(ih);
  if (list_view)
    InvalidateRect(list_view, NULL, TRUE);
}

void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return;

  data->show_grid = show ? TRUE : FALSE;

  /* Invalidate to redraw with/without custom grid lines */
  InvalidateRect(list_view, NULL, TRUE);
}

int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  /* ListView with WS_BORDER has 1px border on each side = 2px total */
  return 2;
}

/****************************************************************************
 * Table Structure
 ****************************************************************************/

void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return;

  int old_num_lin = ih->data->num_lin;

  if (num_lin == old_num_lin)
    return;

  /* Check if in virtual mode */
  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (iupStrBoolean(virtualmode))
  {
    /* Virtual mode: just update item count */
    ListView_SetItemCountEx(list_view, num_lin, LVSICF_NOINVALIDATEALL);
  }
  else
  {
    /* Normal mode: add/remove actual items */
    if (num_lin < old_num_lin)
    {
      /* Delete excess rows */
      for (int i = num_lin; i < old_num_lin; i++)
        ListView_DeleteItem(list_view, num_lin);
    }
    else
    {
      /* Add new rows */
      for (int i = old_num_lin; i < num_lin; i++)
      {
        LVITEM item;
        ZeroMemory(&item, sizeof(LVITEM));
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.pszText = iupwinStrToSystem("");
        ListView_InsertItem(list_view, &item);
      }
    }
  }

  /* Reallocate cell storage if in normal mode */
  if (!iupStrBoolean(virtualmode))
  {
    char*** new_cell_values = (char***)calloc(num_lin, sizeof(char**));

    for (int i = 0; i < num_lin; i++)
    {
      new_cell_values[i] = (char**)calloc(ih->data->num_col, sizeof(char*));

      if (i < old_num_lin)
      {
        /* Copy existing rows */
        for (int j = 0; j < ih->data->num_col; j++)
          new_cell_values[i][j] = data->cell_values[i][j];
      }
    }

    /* Free old rows that are removed */
    for (int i = num_lin; i < old_num_lin; i++)
    {
      for (int j = 0; j < ih->data->num_col; j++)
        winTableFreeCell(&data->cell_values[i][j]);
      free(data->cell_values[i]);
    }

    if (data->cell_values)
      free(data->cell_values);

    data->cell_values = new_cell_values;
  }

  /* Update the core structure's num_lin */
  ih->data->num_lin = num_lin;
}

void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return;

  int old_num_col = ih->data->num_col;

  if (num_col == old_num_col)
    return;

  if (num_col < old_num_col)
  {
    /* Delete excess columns (skip dummy column at index 0) */
    for (int i = num_col; i < old_num_col; i++)
      ListView_DeleteColumn(list_view, num_col + 1);
  }
  else
  {
    /* Add new columns (account for dummy column at index 0) */
    for (int i = old_num_col; i < num_col; i++)
    {
      LVCOLUMN lvc;
      ZeroMemory(&lvc, sizeof(LVCOLUMN));
      lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
      lvc.pszText = iupwinStrToSystem("");
      lvc.cx = 100;  /* Default width */
      lvc.fmt = LVCFMT_LEFT;  /* Default alignment */
      ListView_InsertColumn(list_view, i + 1, &lvc);
    }
  }

  /* Reallocate column metadata */
  int* new_col_widths = (int*)calloc(num_col, sizeof(int));
  BOOL* new_col_width_set = (BOOL*)calloc(num_col, sizeof(BOOL));
  char** new_col_titles = (char**)calloc(num_col, sizeof(char*));

  for (int i = 0; i < num_col; i++)
  {
    if (i < old_num_col)
    {
      new_col_widths[i] = data->col_widths[i];
      new_col_width_set[i] = data->col_width_set[i];
      new_col_titles[i] = data->col_titles[i];
    }
    else
    {
      new_col_widths[i] = 100;
      new_col_width_set[i] = FALSE;
      new_col_titles[i] = NULL;
    }
  }

  /* Free removed column titles */
  for (int i = num_col; i < old_num_col; i++)
  {
    if (data->col_titles[i])
      free(data->col_titles[i]);
  }

  if (data->col_widths)
    free(data->col_widths);
  if (data->col_width_set)
    free(data->col_width_set);
  if (data->col_titles)
    free(data->col_titles);

  data->col_widths = new_col_widths;
  data->col_width_set = new_col_width_set;
  data->col_titles = new_col_titles;

  /* Reallocate cell storage if in normal mode */
  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (!iupStrBoolean(virtualmode))
  {
    for (int i = 0; i < ih->data->num_lin; i++)
    {
      char** new_row = (char**)calloc(num_col, sizeof(char*));

      for (int j = 0; j < num_col; j++)
      {
        if (j < old_num_col)
          new_row[j] = data->cell_values[i][j];
        else
          new_row[j] = NULL;
      }

      /* Free removed cells */
      for (int j = num_col; j < old_num_col; j++)
        winTableFreeCell(&data->cell_values[i][j]);

      if (data->cell_values[i])
        free(data->cell_values[i]);

      data->cell_values[i] = new_row;
    }
  }
}

void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return;

  if (pos < 0)
    pos = ih->data->num_lin;  /* Append */

  if (pos > ih->data->num_lin)
    pos = ih->data->num_lin;

  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (iupStrBoolean(virtualmode))
  {
    /* Virtual mode: just update item count */
    ListView_SetItemCountEx(list_view, ih->data->num_lin + 1, LVSICF_NOINVALIDATEALL);
  }
  else
  {
    /* Normal mode: insert actual item at position */
    LVITEM item;
    ZeroMemory(&item, sizeof(LVITEM));
    item.mask = LVIF_TEXT;
    item.iItem = pos;
    item.pszText = iupwinStrToSystem("");
    ListView_InsertItem(list_view, &item);

    /* Reallocate cell storage */
    int new_num_lin = ih->data->num_lin + 1;
    char*** new_cell_values = (char***)calloc(new_num_lin, sizeof(char**));

    for (int i = 0; i < new_num_lin; i++)
    {
      new_cell_values[i] = (char**)calloc(ih->data->num_col, sizeof(char*));

      if (i < pos)
      {
        /* Copy rows before insertion point */
        for (int j = 0; j < ih->data->num_col; j++)
          new_cell_values[i][j] = data->cell_values[i][j];
      }
      else if (i > pos)
      {
        /* Copy rows after insertion point (shifted down by 1) */
        for (int j = 0; j < ih->data->num_col; j++)
          new_cell_values[i][j] = data->cell_values[i - 1][j];
      }
    }

    /* Free old row pointers (but not cell contents, they were moved) */
    for (int i = 0; i < ih->data->num_lin; i++)
      free(data->cell_values[i]);

    if (data->cell_values)
      free(data->cell_values);

    data->cell_values = new_cell_values;
  }

  /* Update line count */
  ih->data->num_lin++;
}

void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (!data || !list_view)
    return;

  if (pos < 1 || pos > ih->data->num_lin)
    return;

  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (iupStrBoolean(virtualmode))
  {
    /* Virtual mode: just update item count */
    ListView_SetItemCountEx(list_view, ih->data->num_lin - 1, LVSICF_NOINVALIDATEALL);
  }
  else
  {
    /* Normal mode: delete actual item */
    ListView_DeleteItem(list_view, pos - 1);

    /* Free cells in the deleted row */
    for (int j = 0; j < ih->data->num_col; j++)
      winTableFreeCell(&data->cell_values[pos - 1][j]);

    /* Reallocate cell storage */
    int new_num_lin = ih->data->num_lin - 1;
    char*** new_cell_values = (char***)calloc(new_num_lin, sizeof(char**));

    for (int i = 0; i < new_num_lin; i++)
    {
      new_cell_values[i] = (char**)calloc(ih->data->num_col, sizeof(char*));

      if (i < pos - 1)
      {
        /* Copy rows before deletion point */
        for (int j = 0; j < ih->data->num_col; j++)
          new_cell_values[i][j] = data->cell_values[i][j];
      }
      else
      {
        /* Copy rows after deletion point (shifted up by 1) */
        for (int j = 0; j < ih->data->num_col; j++)
          new_cell_values[i][j] = data->cell_values[i + 1][j];
      }
    }

    /* Free old row pointers (but not cell contents, they were moved) */
    for (int i = 0; i < ih->data->num_lin; i++)
      free(data->cell_values[i]);

    if (data->cell_values)
      free(data->cell_values);

    data->cell_values = new_cell_values;
  }

  /* Update line count */
  ih->data->num_lin--;
}

void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  HWND list_view = winTableGetListView(ih);

  if (!list_view)
    return;

  if (pos < 0)
    pos = ih->data->num_col;  /* Append */

  if (pos > ih->data->num_col)
    pos = ih->data->num_col;

  /* Insert column at position (account for dummy column at index 0) */
  LVCOLUMN lvc;
  ZeroMemory(&lvc, sizeof(LVCOLUMN));
  lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
  lvc.pszText = iupwinStrToSystem("");
  lvc.cx = 100;
  lvc.fmt = LVCFMT_LEFT;  /* Default alignment */
  ListView_InsertColumn(list_view, pos + 1, &lvc);

  ih->data->num_col++;
  iupdrvTableSetNumCol(ih, ih->data->num_col);
}

void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  HWND list_view = winTableGetListView(ih);

  if (!list_view)
    return;

  if (pos < 1 || pos > ih->data->num_col)
    return;

  ListView_DeleteColumn(list_view, pos);
  ih->data->num_col--;
  iupdrvTableSetNumCol(ih, ih->data->num_col);
}

/****************************************************************************
 * Attribute Handlers
 ****************************************************************************/

static int winTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  HWND list_view = winTableGetListView(ih);

  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
  {
    ih->data->sortable = 0;

    /* Clear sort state and arrows when disabling sorting */
    if (data && list_view)
    {
      data->sort_column = 0;
      data->sort_ascending = 0;

      /* Remove all sort arrows from headers */
      HWND header = ListView_GetHeader(list_view);
      if (header)
      {
        for (int i = 0; i < ih->data->num_col; i++)
        {
          HDITEM hdi;
          ZeroMemory(&hdi, sizeof(HDITEM));
          hdi.mask = HDI_FORMAT;

          if (Header_GetItem(header, i, &hdi))
          {
            /* Clear sort arrow flags */
            hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
            Header_SetItem(header, i, &hdi);
          }
        }
      }
    }
  }

  return 0;
}

static int winTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  HWND list_view = winTableGetListView(ih);

  if (iupStrBoolean(value))
    ih->data->allow_reorder = 1;
  else
    ih->data->allow_reorder = 0;

  if (list_view)
  {
    HWND header = ListView_GetHeader(list_view);
    if (header)
    {
      DWORD style = GetWindowLong(header, GWL_STYLE);
      if (ih->data->allow_reorder)
        style |= HDS_DRAGDROP;
      else
        style &= ~HDS_DRAGDROP;
      SetWindowLong(header, GWL_STYLE, style);
    }
  }

  return 0;
}

static int winTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  HWND list_view = winTableGetListView(ih);

  if (iupStrBoolean(value))
    ih->data->user_resize = 1;
  else
    ih->data->user_resize = 0;

  /* Apply HDS_NOSIZING style to header control */
  if (list_view)
  {
    HWND header = ListView_GetHeader(list_view);
    if (header)
    {
      DWORD style = GetWindowLong(header, GWL_STYLE);

      if (ih->data->user_resize)
      {
        /* Enable resizing: remove HDS_NOSIZING */
        style &= ~HDS_NOSIZING;
      }
      else
      {
        /* Disable resizing: add HDS_NOSIZING */
        style |= HDS_NOSIZING;
      }

      SetWindowLong(header, GWL_STYLE, style);
    }
  }

  return 0;
}

static int winTableSetAlignmentAttrib(Ihandle* ih, int col, const char* value)
{
  HWND list_view = winTableGetListView(ih);

  if (!list_view || col < 1 || col > ih->data->num_col)
    return 0;

  /* Get current column format to preserve other flags */
  LVCOLUMN lvc;
  ZeroMemory(&lvc, sizeof(LVCOLUMN));
  lvc.mask = LVCF_FMT;

  if (!ListView_GetColumn(list_view, col, &lvc))
    return 0;

  /* Clear alignment bits (LVCFMT_LEFT, LVCFMT_RIGHT, LVCFMT_CENTER) */
  lvc.fmt &= ~(LVCFMT_LEFT | LVCFMT_RIGHT | LVCFMT_CENTER);

  /* Set new alignment */
  if (iupStrEqualNoCase(value, "ARIGHT") || iupStrEqualNoCase(value, "RIGHT"))
    lvc.fmt |= LVCFMT_RIGHT;
  else if (iupStrEqualNoCase(value, "ACENTER") || iupStrEqualNoCase(value, "CENTER"))
    lvc.fmt |= LVCFMT_CENTER;
  else
    lvc.fmt |= LVCFMT_LEFT;

  ListView_SetColumn(list_view, col, &lvc);

  return 1;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static int winTableCallEnterItemCB(Ihandle* ih, int lin, int col)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (cb)
    return cb(ih, lin, col);
  return IUP_DEFAULT;
}

static int winTableCallClickCB(Ihandle* ih, int lin, int col, char* status)
{
  IFniis cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
  if (cb)
    return cb(ih, lin, col, status);
  return IUP_DEFAULT;
}

static void winTableStartEdit(Ihandle* ih, int lin, int col);

static int winTableNotifyCallback(Ihandle* ih, void* msg_info, int* result)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);
  NMHDR* nmhdr = (NMHDR*)msg_info;

  if (!data)
    return 0;

  /* Check if this notification is from the header control */
  HWND header = ListView_GetHeader(data->list_view);
  if (nmhdr->hwndFrom == header)
  {
    /* Handle header notifications */
    switch (nmhdr->code)
    {
      case HDN_BEGINDRAG:
      {
        LPNMHEADER pnmh = (LPNMHEADER)msg_info;

        /* Never allow dragging the dummy column at index 0 */
        if (pnmh->iItem == 0)
        {
          *result = TRUE;  /* Cancel drag */
          return 1;
        }

        /* Header column drag started, allow if ALLOWREORDER=YES */
        if (ih->data->allow_reorder)
        {
          *result = FALSE;  /* Allow drag */
          return 1;
        }
        else
        {
          *result = TRUE;  /* Cancel drag */
          return 1;
        }
      }

      case HDN_ENDDRAG:
      {
        /* Header column drag ended, allow auto-reorder */
        if (ih->data->allow_reorder)
        {
          *result = FALSE;  /* Allow automatic reorder */
          return 1;
        }
        else
        {
          *result = TRUE;  /* Cancel reorder */
          return 1;
        }
      }
    }
  }

  /* Handle ListView notifications */
  switch (nmhdr->code)
  {
    case LVN_GETDISPINFO:
    {
      /* Virtual mode: provide text on demand */
      NMLVDISPINFO* plvdi = (NMLVDISPINFO*)msg_info;

      if (plvdi->item.mask & LVIF_TEXT)
      {
        if (plvdi->item.iSubItem == 0)
        {
          /* Dummy column - return empty string */
          plvdi->item.pszText[0] = 0;
          break;
        }

        int lin = plvdi->item.iItem + 1;  /* Convert to 1-based */
        int col = plvdi->item.iSubItem;  /* Already matches IUP column (dummy at 0) */

        /* Get text from VALUE_CB or cell storage */
        char* value = iupdrvTableGetCellValue(ih, lin, col);

        if (value && *value)
        {
          /* Copy to display buffer */
          TCHAR* tvalue = iupwinStrToSystem(value);
          lstrcpyn(plvdi->item.pszText, tvalue, plvdi->item.cchTextMax);
        }
        else
        {
          plvdi->item.pszText[0] = 0;
        }
      }
      break;
    }

    case LVN_ITEMCHANGED:
    {
      LPNMLISTVIEW pnmv = (LPNMLISTVIEW)msg_info;

      /* Check if selection changed */
      if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED) && !(pnmv->uOldState & LVIS_SELECTED))
      {
        int lin = pnmv->iItem + 1;  /* Convert to 1-based */

        /* Update row (column will be updated in NM_CLICK) */
        data->current_row = lin;
      }
      break;
    }

    case NM_CLICK:
    {
      LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)msg_info;

      if (pnmia->iItem >= 0 && pnmia->iSubItem > 0)  /* Skip dummy column at 0 */
      {
        int lin = pnmia->iItem + 1;  /* Convert to 1-based */
        int col = pnmia->iSubItem;  /* Already matches IUP column (dummy at 0) */

        /* Update focused cell */
        if (lin != data->current_row || col != data->current_col)
        {
          /* Invalidate old focused cell */
          winTableInvalidateCell(winTableGetListView(ih), data->current_row, data->current_col);

          /* Update focus position */
          data->current_row = lin;
          data->current_col = col;

          /* Invalidate new focused cell */
          winTableInvalidateCell(winTableGetListView(ih), lin, col);
        }

        char status[IUPKEY_STATUS_SIZE] = "";
        iupwinButtonKeySetStatus(0, status, 0);

        /* Call CLICK_CB */
        winTableCallClickCB(ih, lin, col, status);

        /* Call ENTERITEM_CB on every click (not suppressed) */
        if (!data->suppress_callbacks)
          winTableCallEnterItemCB(ih, lin, col);
      }
      break;
    }

    case NM_DBLCLK:
    {
      LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)msg_info;

      if (pnmia->iItem >= 0)
      {
        int lin = pnmia->iItem + 1;
        int col = (pnmia->iSubItem > 0) ? pnmia->iSubItem : 1;  /* Skip dummy at 0, default to column 1 */

        winTableStartEdit(ih, lin, col);
      }
      break;
    }

    case LVN_COLUMNCLICK:
    {
      if (ih->data->sortable)
      {
        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)msg_info;
        if (pnmv->iSubItem == 0)
          break;  /* Skip dummy column */

        int col = pnmv->iSubItem;  /* Already matches IUP column (dummy at 0) */

        /* Perform sorting (updates sort state, sorts ListView, updates arrow) */
        winTableSort(ih, col);
      }
      break;
    }

    case NM_CUSTOMDRAW:
    {
      LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)msg_info;

      switch (lplvcd->nmcd.dwDrawStage)
      {
        case CDDS_PREPAINT:
          *result = CDRF_NOTIFYITEMDRAW;
          return 1;

        case CDDS_ITEMPREPAINT:
          *result = CDRF_NOTIFYSUBITEMDRAW;
          return 1;

        case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
        {
          /* Skip dummy column at index 0 */
          if (lplvcd->iSubItem == 0)
          {
            *result = CDRF_DODEFAULT;
            return 1;
          }

          int lin = (int)lplvcd->nmcd.dwItemSpec + 1;  /* Convert to 1-based */
          int col = lplvcd->iSubItem;  /* iSubItem already matches IUP column (dummy at 0) */
          int needs_postpaint = 0;

          /* Check if this row is selected (selection takes priority over custom colors) */
          UINT itemState = ListView_GetItemState(data->list_view, lplvcd->nmcd.dwItemSpec, LVIS_SELECTED);
          int is_row_selected = (itemState & LVIS_SELECTED) != 0;

          /* Background color: per-cell > per-column > per-row > alternating */
          char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);
          if (!bgcolor)
            bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);
          if (!bgcolor)
            bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);

          /* Check for alternating row colors */
          if (!bgcolor)
          {
            char* alternate_color = iupAttribGet(ih, "ALTERNATECOLOR");
            if (iupStrBoolean(alternate_color))
            {
              if (lin % 2 == 0)
                bgcolor = iupAttribGet(ih, "EVENROWCOLOR");
              else
                bgcolor = iupAttribGet(ih, "ODDROWCOLOR");
            }
          }

          /* Apply background color (but not when row is selected) */
          if (!is_row_selected)
          {
            if (bgcolor && *bgcolor)
            {
              unsigned char r, g, b;
              if (iupStrToRGB(bgcolor, &r, &g, &b))
              {
                lplvcd->clrTextBk = RGB(r, g, b);
              }
              else
              {
                lplvcd->clrTextBk = CLR_DEFAULT;
              }
            }
            else
            {
              lplvcd->clrTextBk = CLR_DEFAULT;
            }
          }

          /* Foreground color: per-cell > per-column > per-row */
          if (!is_row_selected)
          {
            char* fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);
            if (!fgcolor)
              fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);
            if (!fgcolor)
              fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);

            if (fgcolor && *fgcolor)
            {
              unsigned char r, g, b;
              if (iupStrToRGB(fgcolor, &r, &g, &b))
              {
                lplvcd->clrText = RGB(r, g, b);
              }
              else
              {
                lplvcd->clrText = CLR_DEFAULT;
              }
            }
            else
            {
              lplvcd->clrText = CLR_DEFAULT;
            }
          }

          /* Font: per-cell > per-column > per-row */
          char* font = iupAttribGetId2(ih, "FONT", lin, col);
          if (!font)
            font = iupAttribGetId2(ih, "FONT", 0, col);
          if (!font)
            font = iupAttribGetId2(ih, "FONT", lin, 0);

          /* Apply font */
          HFONT hFont = NULL;
          HFONT hOldFont = NULL;
          int font_changed = 0;
          if (font && *font)
          {
            hFont = iupwinGetHFont(font);
            if (hFont)
            {
              hOldFont = (HFONT)SelectObject(lplvcd->nmcd.hdc, hFont);
              font_changed = 1;
            }
          }

          /* Need postpaint for grid lines, focused cell, or font restoration */
          if (data->show_grid)
            needs_postpaint = 1;
          else if (lin == data->current_row && col == data->current_col)
            needs_postpaint = 1;
          else if (font_changed)
            needs_postpaint = 1;  /* Need postpaint to restore font */

          /* Store old font for restoration in postpaint */
          if (font_changed)
            data->hfont = hOldFont;

          /* Determine return value */
          if (needs_postpaint && font_changed)
            *result = CDRF_NOTIFYPOSTPAINT | CDRF_NEWFONT;
          else if (needs_postpaint)
            *result = CDRF_NOTIFYPOSTPAINT;
          else
            *result = CDRF_DODEFAULT;

          return 1;
        }

        case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM:
        {
          /* Skip dummy column at index 0 */
          if (lplvcd->iSubItem == 0)
          {
            *result = CDRF_DODEFAULT;
            return 1;
          }

          int lin = (int)lplvcd->nmcd.dwItemSpec + 1;
          int col = lplvcd->iSubItem;  /* iSubItem already matches IUP column (dummy at 0) */
          HDC hdc = lplvcd->nmcd.hdc;
          RECT rc = lplvcd->nmcd.rc;

          /* Draw custom grid lines (only for actual items) */
          if (data->show_grid && lin <= ih->data->num_lin && col <= ih->data->num_col)
          {
            HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DFACE));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

            /* Draw right border */
            MoveToEx(hdc, rc.right - 1, rc.top, NULL);
            LineTo(hdc, rc.right - 1, rc.bottom);

            /* Draw bottom border */
            MoveToEx(hdc, rc.left, rc.bottom - 1, NULL);
            LineTo(hdc, rc.right, rc.bottom - 1);

            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
          }

          /* Draw focus rectangle if this is the focused cell and table has focus */
          if (lin == data->current_row && col == data->current_col &&
              iupAttribGetBoolean(ih, "FOCUSRECT") &&
              GetFocus() == data->list_view)
          {
            /* Create a dashed pen */
            HPEN hPen = CreatePen(PS_DOT, 1, GetSysColor(COLOR_GRAYTEXT));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

            /* Draw dashed rectangle with minimal inset */
            Rectangle(hdc, rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1);

            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
          }

          /* Restore original font if it was changed */
          if (data->hfont)
          {
            SelectObject(hdc, data->hfont);
            data->hfont = NULL;
          }

          *result = CDRF_DODEFAULT;
          return 1;
        }
      }
      break;
    }
  }

  return 0;  /* Not handled */
}

/****************************************************************************
 * Cell Editing
 ****************************************************************************/

static BOOL winTableIsCellEditable(Ihandle* ih, int lin, int col)
{
  if (lin < 1 || col < 1)
    return FALSE;

  char name[50];
  sprintf(name, "EDITABLE%d", col);
  char* editable = iupAttribGet(ih, name);
  if (!editable)
    editable = iupAttribGet(ih, "EDITABLE");

  return iupStrBoolean(editable);
}

static LRESULT CALLBACK winTableEditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

static void winTableEndEdit(Ihandle* ih, BOOL save)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);

  if (!data || !data->edit_control)
    return;

  int lin = data->edit_row;
  int col = data->edit_col;

  char buffer[4096];
  GetWindowTextA(data->edit_control, buffer, sizeof(buffer));

  /* Call EDITEND_CB, allow application to validate/reject edit */
  IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb)
  {
    int ret = editend_cb(ih, lin, col, buffer, save ? 1 : 0);
    if (ret == IUP_IGNORE && save)
    {
      /* Application rejected the edit, keep editor open */
      data->edit_ending = FALSE;
      SetFocus(data->edit_control);
      SendMessage(data->edit_control, EM_SETSEL, 0, -1);
      return;
    }
  }

  if (save)
  {
    /* Get old value for comparison, must copy before it gets modified */
    char* old_text_ptr = iupdrvTableGetCellValue(ih, lin, col);
    char* old_text = old_text_ptr ? iupStrDup(old_text_ptr) : NULL;

    IFniis cb = (IFniis)IupGetCallback(ih, "EDITION_CB");
    if (cb)
    {
      int ret = cb(ih, lin, col, buffer);
      if (ret == IUP_IGNORE)
      {
        if (old_text)
          free(old_text);
        DestroyWindow(data->edit_control);
        data->edit_control = NULL;
        data->edit_row = 0;
        data->edit_col = 0;
        data->edit_ending = FALSE;
        return;
      }
    }

    iupdrvTableSetCellValue(ih, lin, col, buffer);

    /* Call VALUECHANGED_CB only if text actually changed */
    int text_changed = 0;
    if (!old_text && buffer && *buffer)
      text_changed = 1;  /* NULL → non-empty */
    else if (old_text && !buffer)
      text_changed = 1;  /* non-empty → NULL */
    else if (old_text && buffer && strcmp(old_text, buffer) != 0)
      text_changed = 1;  /* different text */

    if (text_changed)
    {
      IFnii value_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
      if (value_cb)
        value_cb(ih, lin, col);
    }

    if (old_text)
      free(old_text);
  }

  DestroyWindow(data->edit_control);
  data->edit_control = NULL;
  data->edit_row = 0;
  data->edit_col = 0;
  data->edit_ending = FALSE;

  SetFocus(data->list_view);
}

static LRESULT CALLBACK winTableEditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
  Ihandle* ih = (Ihandle*)dwRefData;
  IwinTableData* data = IWIN_TABLE_DATA(ih);

  switch (msg)
  {
    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;

    case WM_KEYDOWN:
      if (wp == VK_RETURN)
      {
        data->edit_ending = TRUE;
        winTableEndEdit(ih, TRUE);
        return 0;
      }
      else if (wp == VK_ESCAPE)
      {
        data->edit_ending = TRUE;
        winTableEndEdit(ih, FALSE);
        return 0;
      }
      break;

    case WM_KILLFOCUS:
      if (!data->edit_ending)
        winTableEndEdit(ih, TRUE);
      return 0;

    case WM_NCDESTROY:
      RemoveWindowSubclass(hwnd, winTableEditWndProc, uIdSubclass);
      break;
  }

  return DefSubclassProc(hwnd, msg, wp, lp);
}

static void winTableStartEdit(Ihandle* ih, int lin, int col)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);

  if (!data || !data->list_view)
    return;

  if (!winTableIsCellEditable(ih, lin, col))
    return;

  /* Call EDITBEGIN_CB, allow application to block editing */
  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    int ret = editbegin_cb(ih, lin, col);
    if (ret == IUP_IGNORE)
      return;  /* Block editing */
  }

  if (data->edit_control)
    winTableEndEdit(ih, TRUE);

  RECT rect;
  ListView_GetSubItemRect(data->list_view, lin - 1, col, LVIR_LABEL, &rect);

  const char* value = iupdrvTableGetCellValue(ih, lin, col);

  data->edit_control = CreateWindowExA(
    0, "EDIT", value ? value : "",
    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
    rect.left, rect.top,
    rect.right - rect.left, rect.bottom - rect.top,
    data->list_view, NULL, NULL, NULL);

  if (data->edit_control)
  {
    data->edit_row = lin;
    data->edit_col = col;
    data->edit_ending = FALSE;

    HFONT hFont = (HFONT)SendMessage(data->list_view, WM_GETFONT, 0, 0);
    if (hFont)
      SendMessage(data->edit_control, WM_SETFONT, (WPARAM)hFont, TRUE);

    SetWindowSubclass(data->edit_control, winTableEditWndProc, 0, (DWORD_PTR)ih);

    SetFocus(data->edit_control);
    SendMessage(data->edit_control, EM_SETSEL, 0, -1);
  }
}

/****************************************************************************
 * Keyboard Handling
 ****************************************************************************/

static int winTableKeyProc(Ihandle* ih, HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT* result)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);

  if (!data)
    return 0;

  if (msg == WM_KEYDOWN)
  {
    int lin = data->current_row;
    int col = data->current_col;
    BOOL handled = FALSE;

    /* If editing, let edit control handle keys */
    if (data->edit_control)
      return 0;

    switch (wp)
    {
      case VK_UP:
        /* Move to previous row */
        if (lin > 1)
        {
          iupdrvTableSetFocusCell(ih, lin - 1, col);

          IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
          if (enteritem_cb)
            enteritem_cb(ih, lin - 1, col);

          handled = TRUE;
        }
        break;

      case VK_DOWN:
        /* Move to next row */
        if (lin < ih->data->num_lin)
        {
          iupdrvTableSetFocusCell(ih, lin + 1, col);

          IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
          if (enteritem_cb)
            enteritem_cb(ih, lin + 1, col);

          handled = TRUE;
        }
        break;


      case VK_LEFT:
        /* Move to previous column */
        if (col > 1)
        {
          iupdrvTableSetFocusCell(ih, lin, col - 1);

          IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
          if (enteritem_cb)
            enteritem_cb(ih, lin, col - 1);

          handled = TRUE;
        }
        break;

      case VK_RIGHT:
        /* Move to next column */
        if (col < ih->data->num_col)
        {
          iupdrvTableSetFocusCell(ih, lin, col + 1);

          IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
          if (enteritem_cb)
            enteritem_cb(ih, lin, col + 1);

          handled = TRUE;
        }
        break;

      case VK_TAB:
      {
        BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (shift)
        {
          /* Shift+Tab: Move to previous cell */
          if (col > 1)
          {
            iupdrvTableSetFocusCell(ih, lin, col - 1);

            IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
            if (enteritem_cb)
              enteritem_cb(ih, lin, col - 1);

            handled = TRUE;
          }
          else if (lin > 1)
          {
            /* At start of row, move to last column of previous row */
            iupdrvTableSetFocusCell(ih, lin - 1, ih->data->num_col);

            IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
            if (enteritem_cb)
              enteritem_cb(ih, lin - 1, ih->data->num_col);

            handled = TRUE;
          }
        }
        else
        {
          /* Tab: Move to next cell */
          if (col < ih->data->num_col)
          {
            iupdrvTableSetFocusCell(ih, lin, col + 1);

            IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
            if (enteritem_cb)
              enteritem_cb(ih, lin, col + 1);

            handled = TRUE;
          }
          else if (lin < ih->data->num_lin)
          {
            /* At end of row, move to first column of next row */
            iupdrvTableSetFocusCell(ih, lin + 1, 1);

            IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
            if (enteritem_cb)
              enteritem_cb(ih, lin + 1, 1);

            handled = TRUE;
          }
        }
        break;
      }

      case VK_RETURN:
        winTableStartEdit(ih, lin, col);
        handled = TRUE;
        break;

      case VK_F2:
        /* Also start editing with F2 */
        winTableStartEdit(ih, lin, col);
        handled = TRUE;
        break;

      case 'C':
        /* Copy current cell to clipboard */
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
          const char* value = iupdrvTableGetCellValue(ih, lin, col);
          if (value && *value)
          {
            IupSetGlobal("CLIPBOARD", value);
            handled = TRUE;
          }
        }
        break;

      case 'V':
        /* Paste from clipboard to current cell */
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
          /* Check if cell is editable */
          char name[50];
          sprintf(name, "EDITABLE%d", col);
          char* editable = iupAttribGet(ih, name);
          if (!editable)
            editable = iupAttribGet(ih, "EDITABLE");

          if (iupStrBoolean(editable))
          {
            const char* text = IupGetGlobal("CLIPBOARD");
            if (text && *text)
            {
              /* Get old value for comparison, must copy before it gets modified */
              char* old_text_ptr = iupdrvTableGetCellValue(ih, lin, col);
              char* old_text = old_text_ptr ? iupStrDup(old_text_ptr) : NULL;

              /* Set the cell value */
              iupdrvTableSetCellValue(ih, lin, col, text);

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
                IFnii value_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
                if (value_cb)
                  value_cb(ih, lin, col);
              }

              if (old_text)
                free(old_text);

              handled = TRUE;
            }
          }
        }
        break;
    }

    if (handled)
    {
      *result = 0;
      return 1;
    }
  }

  (void)hwnd;
  (void)lp;
  return 0;
}

static LRESULT CALLBACK winTableListViewWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  LRESULT result = 0;
  WNDPROC oldProc;
  Ihandle* ih;

  ih = iupwinHandleGet(hwnd);
  if (!iupObjectCheck(ih))
    return DefWindowProc(hwnd, msg, wp, lp);

  /* Retrieve the control previous procedure for subclassing */
  oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_LISTVIEWOLDPROC_CB");

  /* Handle WM_NOTIFY from header control */
  if (msg == WM_NOTIFY)
  {
    NMHDR* nmhdr = (NMHDR*)lp;
    IwinTableData* data = IWIN_TABLE_DATA(ih);

    if (data)
    {
      HWND header = ListView_GetHeader(data->list_view);

      /* Check if notification is from header control */
      if (nmhdr->hwndFrom == header)
      {
        switch (nmhdr->code)
        {
          case HDN_BEGINDRAG:
          {
            LPNMHEADER pnmh = (LPNMHEADER)lp;

            /* Never allow dragging the dummy column at index 0 */
            if (pnmh->iItem == 0)
              return TRUE;  /* Cancel drag */

            /* Allow or block drag based on ALLOWREORDER attribute */
            return ih->data->allow_reorder ? FALSE : TRUE;
          }

          case HDN_ENDDRAG:
          {
            /* Allow or block reorder based on ALLOWREORDER attribute */
            if (!ih->data->allow_reorder)
              return TRUE;

            /* Let Windows handle the header reorder */
            CallWindowProc(oldProc, hwnd, msg, wp, lp);

            /* Now synchronize the ListView column order with the header */
            int num_col = ih->data->num_col;
            if (num_col > 0)
            {
              /* Order array must include dummy column at index 0 */
              int total_cols = num_col + 1;
              int* order = (int*)malloc(total_cols * sizeof(int));
              if (order)
              {
                /* Get the new header order (includes dummy + real columns) */
                Header_GetOrderArray(header, total_cols, order);

                /* Apply it to the ListView columns */
                ListView_SetColumnOrderArray(data->list_view, total_cols, order);

                free(order);
              }
            }

            return FALSE;
          }
        }
      }
    }
  }

  /* Handle focus changes to redraw FOCUSRECT */
  if (msg == WM_SETFOCUS || msg == WM_KILLFOCUS)
  {
    IwinTableData* data = IWIN_TABLE_DATA(ih);
    if (data && data->current_row > 0 && data->current_col > 0)
    {
      /* Invalidate focused cell to redraw/remove FOCUSRECT */
      winTableInvalidateCell(hwnd, data->current_row, data->current_col);
    }
  }

  /* Handle keyboard messages */
  if (winTableKeyProc(ih, hwnd, msg, wp, lp, &result))
    return result;

  /* Call original ListView window procedure */
  return CallWindowProc(oldProc, hwnd, msg, wp, lp);
}

/****************************************************************************
 * Widget Lifecycle
 ****************************************************************************/

static int winTableMapMethod(Ihandle* ih)
{
  DWORD dwStyle, dwExStyle;

  if (!ih->parent)
    return IUP_ERROR;

  /* Allocate driver data */
  IwinTableData* data = (IwinTableData*)calloc(1, sizeof(IwinTableData));
  ih->data->native_data = data;

  /* Get initial dimensions */
  int num_col = ih->data->num_col;
  int num_lin = ih->data->num_lin;

  /* Allocate column metadata */
  data->col_widths = (int*)calloc(num_col, sizeof(int));
  data->col_width_set = (BOOL*)calloc(num_col, sizeof(BOOL));
  data->col_titles = (char**)calloc(num_col, sizeof(char*));

  /* Initialize with default widths (will be auto-sized later) */
  for (int i = 0; i < num_col; i++)
  {
    data->col_widths[i] = 100;  /* Default width */
    data->col_width_set[i] = FALSE;  /* No explicit width set yet */
  }

  /* Allocate cell storage if not in virtual mode */
  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (!iupStrBoolean(virtualmode))
  {
    data->cell_values = (char***)calloc(num_lin, sizeof(char**));
    for (int i = 0; i < num_lin; i++)
      data->cell_values[i] = (char**)calloc(num_col, sizeof(char*));
  }

  /* Initialize state */
  data->current_row = 0;
  data->current_col = 0;
  data->sort_column = 0;
  data->sort_ascending = 0;  /* 0 = not sorted */
  data->show_grid = iupAttribGetBoolean(ih, "SHOWGRID");  /* Read from attribute (default YES) */
  data->suppress_callbacks = 0;
  data->hfont = NULL;

  /* Create ListView control */
  /* Note: NOT adding WS_VISIBLE initially, will show after first layout */
  dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;

  /* Add LVS_OWNERDATA for virtual mode */
  if (iupStrBoolean(virtualmode))
    dwStyle |= LVS_OWNERDATA;

  dwExStyle = 0;

  if (!iupwinCreateWindow(ih, WC_LISTVIEW, dwExStyle, dwStyle, NULL))
    return IUP_ERROR;

  data->list_view = ih->handle;

  /* Set extended styles (no LVS_EX_GRIDLINES - we'll draw custom grid) */
  DWORD exStyle = LVS_EX_FULLROWSELECT;
  ListView_SetExtendedListViewStyle(data->list_view, exStyle);

  /* Create dummy column at index 0 to allow alignment of first real column */
  LVCOLUMN lvc_dummy;
  ZeroMemory(&lvc_dummy, sizeof(LVCOLUMN));
  lvc_dummy.mask = LVCF_WIDTH;
  lvc_dummy.cx = 0;
  ListView_InsertColumn(data->list_view, 0, &lvc_dummy);

  /* Create real columns at indices 1..num_col */
  for (int i = 0; i < num_col; i++)
  {
    LVCOLUMN lvc;
    ZeroMemory(&lvc, sizeof(LVCOLUMN));
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    lvc.pszText = iupwinStrToSystem("");
    lvc.cx = data->col_widths[i];

    /* Default: left-aligned (alignment will be set later when titles are set) */
    lvc.fmt = LVCFMT_LEFT;

    ListView_InsertColumn(data->list_view, i + 1, &lvc);
  }

  /* Create rows */
  if (iupStrBoolean(virtualmode))
  {
    /* Virtual mode: set item count, don't insert items */
    ListView_SetItemCountEx(data->list_view, num_lin, LVSICF_NOINVALIDATEALL);
  }
  else
  {
    /* Normal mode: insert actual items */
    for (int i = 0; i < num_lin; i++)
    {
      LVITEM item;
      ZeroMemory(&item, sizeof(LVITEM));
      item.mask = LVIF_TEXT;
      item.iItem = i;
      item.pszText = iupwinStrToSystem("");
      ListView_InsertItem(data->list_view, &item);
    }
  }

  /* Register notify callback for WM_NOTIFY messages */
  IupSetCallback(ih, "_IUPWIN_NOTIFY_CB", (Icallback)winTableNotifyCallback);

  /* Subclass the ListView control for keyboard handling */
  iupwinHandleAdd(ih, data->list_view);
  IupSetCallback(ih, "_IUPWIN_LISTVIEWOLDPROC_CB", (Icallback)GetWindowLongPtr(data->list_view, GWLP_WNDPROC));
  SetWindowLongPtr(data->list_view, GWLP_WNDPROC, (LONG_PTR)winTableListViewWndProc);

  /* Apply initial USERRESIZE state to header control */
  HWND header = ListView_GetHeader(data->list_view);
  if (header)
  {
    /* Apply user_resize state (default is 0 = disabled) */
    if (!ih->data->user_resize)
    {
      /* Disable resizing by default */
      DWORD style = GetWindowLong(header, GWL_STYLE);
      style |= HDS_NOSIZING;
      SetWindowLong(header, GWL_STYLE, style);
    }
  }

  /* Process FOCUSCELL attribute if set before mapping */
  char* focuscell = iupAttribGet(ih, "FOCUSCELL");
  if (focuscell)
  {
    int lin, col;
    if (iupStrToIntInt(focuscell, &lin, &col, ':') == 2)
    {
      iupdrvTableSetFocusCell(ih, lin, col);

      /* Call ENTERITEM_CB for initial focus */
      winTableCallEnterItemCB(ih, lin, col);
    }
  }

  return IUP_NOERROR;
}

static void winTableLayoutUpdateMethod(Ihandle* ih)
{
  HWND list_view = winTableGetListView(ih);
  BOOL was_visible = list_view ? IsWindowVisible(list_view) : FALSE;

  /* Call base implementation first to position and size the control */
  iupdrvBaseLayoutUpdateMethod(ih);

  /* Now adjust column widths based on the new size */
  if (list_view)
  {
    RECT rect;
    GetClientRect(list_view, &rect);
    int width = rect.right - rect.left;

    /* Only adjust if width is reasonable (> 100px) */
    if (width > 100)
    {
      /* Adjust column widths based on actual ListView size */
      winTableAdjustColumnWidths(ih);

      /* Show ListView after first proper layout to avoid resize flicker */
      if (!was_visible)
      {
        ShowWindow(list_view, SW_SHOW);
      }
    }
  }
}

static void winTableUnMapMethod(Ihandle* ih)
{
  IwinTableData* data = IWIN_TABLE_DATA(ih);

  if (!data)
  {
    iupdrvBaseUnMapMethod(ih);
    return;
  }

  /* Free cell storage */
  if (data->cell_values)
  {
    for (int i = 0; i < ih->data->num_lin; i++)
    {
      if (data->cell_values[i])
      {
        for (int j = 0; j < ih->data->num_col; j++)
          winTableFreeCell(&data->cell_values[i][j]);
        free(data->cell_values[i]);
      }
    }
    free(data->cell_values);
  }

  /* Free column metadata */
  if (data->col_titles)
  {
    for (int i = 0; i < ih->data->num_col; i++)
    {
      if (data->col_titles[i])
        free(data->col_titles[i]);
    }
    free(data->col_titles);
  }

  if (data->col_widths)
    free(data->col_widths);

  if (data->col_width_set)
    free(data->col_width_set);

  /* Destroy edit control if active */
  if (data->edit_control)
  {
    DestroyWindow(data->edit_control);
    data->edit_control = NULL;
  }

  /* Destroy ListView */
  if (data->list_view)
    DestroyWindow(data->list_view);

  /* Free driver data */
  free(data);
  ih->data->native_data = NULL;

  iupdrvBaseUnMapMethod(ih);
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

void iupdrvTableInitClass(Iclass* ic)
{
  /* Set Map/UnMap/LayoutUpdate methods */
  ic->Map = winTableMapMethod;
  ic->UnMap = winTableUnMapMethod;
  ic->LayoutUpdate = winTableLayoutUpdateMethod;

  /* Replace core SET handlers */
  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, winTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, winTableSetAllowReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, winTableSetUserResizeAttrib);

  /* Register FOCUSRECT attribute */
  iupClassRegisterAttribute(ic, "FOCUSRECT", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  /* Register ALIGNMENT attribute (per-column: ALIGNMENT1, ALIGNMENT2, etc.) */
  /* Allow before and after mapping, alignment is applied when titles are set */
  iupClassRegisterAttributeId(ic, "ALIGNMENT", NULL, (IattribSetIdFunc)winTableSetAlignmentAttrib, IUPAF_NO_INHERIT);
}
