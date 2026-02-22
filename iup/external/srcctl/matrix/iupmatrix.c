/** \file
 * \brief IupMatrix control core
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupcontrols.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_controls.h"
#include "iup_register.h"
#include "iup_assert.h"
#include "iup_flatscrollbar.h"

#include "iupmat_def.h"
#include "iupmat_getset.h"
#include "iupmat_scroll.h"
#include "iupmat_aux.h"
#include "iupmat_mem.h"
#include "iupmat_mouse.h"
#include "iupmat_key.h"
#include "iupmat_numlc.h"
#include "iupmat_colres.h"
#include "iupmat_mark.h"
#include "iupmat_edit.h"
#include "iupmat_draw.h"


int iupMatrixIsValid(Ihandle* ih, int check_cells)
{
  /* IupDraw doesn't require explicit canvas - always available after map */
  if (!ih->handle)
    return 0;
  if (check_cells && ((ih->data->columns.num == 0) || (ih->data->lines.num == 0)))
    return 0;
  return 1;
}

static char* iMatrixGetOriginAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->lines.first, ih->data->columns.first, ':');
}

static char* iMatrixGetOriginOffsetAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->lines.first_offset, ih->data->columns.first_offset, ':');
}

static int iMatrixSetOriginAttrib(Ihandle* ih, const char* value)
{
  int lin = IUP_INVALID_ID, col = IUP_INVALID_ID;

  /* Get the parameters. The '*' indicates that want to keep the table in
     the same line or column */
  if (iupStrToIntInt(value, &lin, &col, ':') != 2)
  {
    if (lin != IUP_INVALID_ID)
      col = ih->data->columns.first;
    else if (col != IUP_INVALID_ID)
      lin = ih->data->lines.first;
    else
      return 0;
  }

  /* Check if the cell exists */
  if (!iupMatrixCheckCellPos(ih, lin, col))
    return 0;

  /* Can not be non scrollable cell */
  if ((lin < ih->data->lines.num_noscroll) || (col < ih->data->columns.num_noscroll))
    return 0;
  else
  {
    int redraw = 0;
    int old_lines_first = ih->data->lines.first;
    int old_columns_first = ih->data->columns.first;
    int old_lines_first_offset = ih->data->lines.first_offset;
    int old_columns_first_offset = ih->data->columns.first_offset;

    ih->data->columns.first = col;
    ih->data->columns.first_offset = 0;
    ih->data->lines.first = lin;
    ih->data->lines.first_offset = 0;

    value = iupAttribGet(ih, "ORIGINOFFSET");
    if (value)
    {
      int lin_offset, col_offset;
      if (iupStrToIntInt(value, &lin_offset, &col_offset, ':') == 2)
      {
        if (col_offset < ih->data->columns.dt[col].size)
          ih->data->columns.first_offset = col_offset;
        if (lin_offset < ih->data->lines.dt[lin].size)
          ih->data->lines.first_offset = lin_offset;
      }
    }

    /* when "first" is changed must update scroll pos */
    if (ih->data->columns.first != old_columns_first || ih->data->columns.first_offset != old_columns_first_offset)
    {
      iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_COL);
      redraw = 1;
    }

    if (ih->data->lines.first != old_lines_first || ih->data->lines.first_offset != old_lines_first_offset)
    {
      iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_LIN);
      redraw = 1;
    }

    if (redraw)
      iupMatrixDraw(ih, 1);
  }
  return 0;
}

static int iMatrixSetShowAttrib(Ihandle* ih, const char* value)
{
  int lin = IUP_INVALID_ID, col = IUP_INVALID_ID;

  /* Get the parameters. The '*' indicates that want to keep the table in
     the same line or column */
  if (iupStrToIntInt(value, &lin, &col, ':') != 2)
  {
    if (lin != IUP_INVALID_ID)
      col = ih->data->columns.first;
    else if (col != IUP_INVALID_ID)
      lin = ih->data->lines.first;
    else
      return 0;
  }

  /* Check if the cell exists */
  if (!iupMatrixCheckCellPos(ih, lin, col))
    return 0;

  if (!iupMatrixAuxIsCellStartVisible(ih, lin, col))
    iupMatrixScrollToVisible(ih, lin, col);

  return 0;
}

static int iMatrixSetFocusCellAttrib(Ihandle* ih, const char* value)
{
  int lin = IUP_INVALID_ID, col = IUP_INVALID_ID;
  if (iupStrToIntInt(value, &lin, &col, ':') == 2)
  {
    if (!iupMatrixCheckCellPos(ih, lin, col))
      return 0;

    if (lin == 0 || col == 0) /* title can NOT have the focus */
      return 0;
    if (lin >= ih->data->lines.num || col >= ih->data->columns.num)
      return 0;

    ih->data->lines.focus_cell = lin;
    ih->data->columns.focus_cell = col;

    /* IupDraw: Check if widget is mapped instead of checking cd_canvas */
    if (ih->handle)
      iupMatrixDrawUpdate(ih);
  }

  return 0;
}

static char* iMatrixGetFocusCellAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->lines.focus_cell, ih->data->columns.focus_cell, ':');
}

static int iMatrixSetUseTitleSizeAttrib(Ihandle* ih, const char* value)
{
  /* can be set only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
    ih->data->use_title_size = 1;
  else
    ih->data->use_title_size = 0;

  return 0;
}

static char* iMatrixGetUseTitleSizeAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->use_title_size);
}

static int iMatrixSetResizeDragAttrib(Ihandle* ih, const char* value)
{
  /* can be set only before map */
  if (ih->handle)
    return 0;

  if (iupStrBoolean(value))
    ih->data->colres_drag = 1;
  else
    ih->data->colres_drag = 0;

  return 0;
}

static char* iMatrixGetResizeDragAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->colres_drag);
}

static int iMatrixSetLimitExpandAttrib(Ihandle* ih, const char* value)
{
  /* can be set only before map */
  if (ih->handle)
    return 0;

  ih->data->limit_expand = iupStrBoolean(value);
  return 0;
}

static char* iMatrixGetLimitExpandAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->limit_expand);
}

static int iMatrixSetHiddenTextMarksAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->hidden_text_marks = 1;
  else
    ih->data->hidden_text_marks = 0;
  return 0;
}

static char* iMatrixGetHiddenTextMarksAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->hidden_text_marks);
}

static int iMatrixSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->columns.num <= 1 || ih->data->lines.num <= 1)
    return 0;

  if (ih->data->editing)
    IupStoreAttribute(ih->data->datah, "VALUE", value);
  else
    iupMatrixSetValue(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell, value, 0);

  return 0;
}

static char* iMatrixGetValueAttrib(Ihandle* ih)
{
  if (ih->data->columns.num <= 1 || ih->data->lines.num <= 1)
    return NULL;

  if (ih->data->editing)
    return iupMatrixEditGetValue(ih);
  else
    return iupMatrixGetValue(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell);
}

static int iMatrixSetCaretAttrib(Ihandle* ih, const char* value)
{
  IupStoreAttribute(ih->data->texth, "CARET", value);
  return 1;
}

static int iMatrixSetInsertAttrib(Ihandle* ih, const char* value)
{
  IupStoreAttribute(ih->data->texth, "INSERT", value);
  return 1;
}

static char* iMatrixGetCaretAttrib(Ihandle* ih)
{
  return IupGetAttribute(ih->data->texth, "CARET");
}

static int iMatrixSetSelectionAttrib(Ihandle* ih, const char* value)
{
  IupStoreAttribute(ih->data->texth, "SELECTION", value);
  return 1;
}

static char* iMatrixGetSelectionAttrib(Ihandle* ih)
{
  return IupGetAttribute(ih->data->texth, "SELECTION");
}

static int iMatrixSetMultilineAttrib(Ihandle* ih, const char* value)
{
  IupStoreAttribute(ih->data->texth, "MULTILINE", value);
  if (iupStrBoolean(value))
    IupSetAttribute(ih->data->texth, "SCROLLBAR", "VERTICAL");
  return 1;
}

static char* iMatrixGetMultilineAttrib(Ihandle* ih)
{
  return IupGetAttribute(ih->data->texth, "MULTILINE");
}

static char* iMatrixGetCountAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->lines.num > ih->data->columns.num ? ih->data->lines.num : ih->data->columns.num);
}

static char* iMatrixGetNumLinAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->lines.num - 1);  /* the attribute does not include the title */
}

static char* iMatrixGetNumColAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->columns.num - 1);  /* the attribute does not include the title */
}

static int iMatrixSetMarkModeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "CELL"))
    ih->data->mark_mode = IMAT_MARK_CELL;
  else if (iupStrEqualNoCase(value, "LIN"))
    ih->data->mark_mode = IMAT_MARK_LIN;
  else if (iupStrEqualNoCase(value, "COL"))
    ih->data->mark_mode = IMAT_MARK_COL;
  else if (iupStrEqualNoCase(value, "LINCOL"))
    ih->data->mark_mode = IMAT_MARK_LINCOL;
  else
    ih->data->mark_mode = IMAT_MARK_NO;

  if (ih->handle)
  {
    iupMatrixMarkClearAll(ih, 0);
    iupMatrixDraw(ih, 1);
  }
  return 0;
}

static char* iMatrixGetMarkModeAttrib(Ihandle* ih)
{
  char* mark2str[] = { "NO", "LIN", "COL", "LINCOL", "CELL" };
  return mark2str[ih->data->mark_mode];
}

static int iMatrixSetMarkAreaAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "NOT_CONTINUOUS"))
    ih->data->mark_continuous = 0;
  else
    ih->data->mark_continuous = 1;

  if (ih->handle)
  {
    iupMatrixMarkClearAll(ih, 0);
    iupMatrixDraw(ih, 1);
  }
  return 0;
}

static char* iMatrixGetMarkAreaAttrib(Ihandle* ih)
{
  if (ih->data->mark_continuous)
    return "CONTINUOUS";
  else
    return "NOT_CONTINUOUS";
}

static int iMatrixSetMarkMultipleAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->mark_multiple = 1;
  else
    ih->data->mark_multiple = 0;

  if (ih->handle)
  {
    iupMatrixMarkClearAll(ih, 0);
    iupMatrixDraw(ih, 1);
  }
  return 0;
}

static char* iMatrixGetMarkMultipleAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->mark_multiple);
}

static int iMatrixSetEditHideOnFocusAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->edit_hide_onfocus = 1;
  else
    ih->data->edit_hide_onfocus = 0;
  return 0;
}

static char* iMatrixGetEditHideOnFocusAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->edit_hide_onfocus);
}

static char* iMatrixGetEditCellAttrib(Ihandle* ih)
{
  if (ih->data->editing)
    return iupStrReturnIntInt(ih->data->edit_lin, ih->data->edit_col, ':');
  else
    return NULL;
}

static char* iMatrixGetEditTextAttrib(Ihandle* ih)
{
  if (ih->data->editing)
    return iupStrReturnBoolean(ih->data->datah == ih->data->texth);
  else
    return NULL;
}

static int iMatrixSetEditModeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    iupMatrixEditShow(ih);
  else
  {
    iupMatrixEditHide(ih);
    iupMatrixDrawUpdate(ih);
  }
  return 1;
}

static char* iMatrixGetEditModeAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(iupMatrixEditIsVisible(ih));
}

static char* iMatrixGetEditingAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->editing);
}

static int iMatrixSetEditNextAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "NONE"))
    ih->data->editnext = IMAT_EDITNEXT_NONE;
  else if (iupStrEqualNoCase(value, "COL"))
    ih->data->editnext = IMAT_EDITNEXT_COL;
  else if (iupStrEqualNoCase(value, "COLCR"))
    ih->data->editnext = IMAT_EDITNEXT_COLCR;
  else if (iupStrEqualNoCase(value, "LINCR"))
    ih->data->editnext = IMAT_EDITNEXT_LINCR;
  else
    ih->data->editnext = IMAT_EDITNEXT_LIN;
  return 0;
}

static char* iMatrixGetEditNextAttrib(Ihandle* ih)
{
  switch (ih->data->editnext)
  {
    case IMAT_EDITNEXT_NONE: return "NONE";
    case IMAT_EDITNEXT_COL: return "COL";
    case IMAT_EDITNEXT_LINCR: return "LINCR";
    case IMAT_EDITNEXT_COLCR: return "COLCR";
    default: return "LIN";
  }
}

static int iMatrixHasColWidth(Ihandle* ih, int col)
{
  char* value = iupAttribGetId(ih, "WIDTH", col);
  if (!value)
    value = iupAttribGetId(ih, "RASTERWIDTH", col);
  return (value != NULL);
}

static int iMatrixHasLineHeight(Ihandle* ih, int lin)
{
  char* value = iupAttribGetId(ih, "HEIGHT", lin);
  if (!value)
    value = iupAttribGetId(ih, "RASTERHEIGHT", lin);
  return (value != NULL);
}

static void iMatrixFitLines(Ihandle* ih, int height)
{
  /* expand each line height to fit the matrix height */
  int line_height, lin, empty, has_line_height,
    *empty_lines, empty_num = 0, visible_num, empty_lin_visible = 0;

  /* get only from the hash table or the default value, do NOT get from the actual number of visible lines */
  visible_num = iupAttribGetInt(ih, "NUMLIN_VISIBLE") + 1;  /* include the title line */

  empty_lines = malloc(ih->data->lines.num*sizeof(int));

  /* ignore the lines that already have HEIGHT or RASTERHEIGHT */
  for (lin = 0; lin < ih->data->lines.num; lin++)
  {
    has_line_height = iMatrixHasLineHeight(ih, lin);
    empty = 1;

    /* when lin==0 the line exists if size is defined,
       but also exists if some title text is defined in non callback mode. */
    line_height = iupMatrixGetLineHeight(ih, lin, lin == 0 ? 1 : 0);

    /* the line does not exists */
    if (lin == 0 && !has_line_height && !line_height)
      empty = 0;  /* don't resize this line */

    /* if the height is defined to be 0, it can not be used */
    if (has_line_height && !line_height)
      empty = 0;  /* don't resize this line */

    if (line_height)
    {
      if (lin < visible_num)
        height -= line_height;
    }
    else if (empty)
    {
      if (lin < visible_num)
        empty_lin_visible++;

      empty_lines[empty_num] = lin;
      empty_num++;
    }
  }

  if (empty_num && empty_lin_visible)
  {
    int i;

    line_height = height / empty_lin_visible - (IMAT_PADDING_H + IMAT_FRAME_H);

    for (i = 0; i < empty_num; i++)
      iupAttribSetIntId(ih, "RASTERHEIGHT", empty_lines[i], line_height);
  }

  free(empty_lines);
}

static void iMatrixFitColumns(Ihandle* ih, int width)
{
  /* expand each column width to fit the matrix width */
  int column_width, col, empty, has_col_width,
    *empty_columns, empty_num = 0, visible_num, empty_col_visible = 0;

  /* get only from the hash table or the default value, do NOT get from the actual number of visible columns */
  visible_num = iupAttribGetInt(ih, "NUMCOL_VISIBLE") + 1;  /* include the title column */

  empty_columns = malloc(ih->data->columns.num*sizeof(int));

  /* ignore the columns that already have WIDTH or RASTERWIDTH */
  for (col = 0; col < ih->data->columns.num; col++)
  {
    has_col_width = iMatrixHasColWidth(ih, col);
    empty = 1;

    /* when col==0 the col exists if size is defined,
       but also exists if some title text is defined in non callback mode. */
    column_width = iupMatrixGetColumnWidth(ih, col, col == 0 ? 1 : 0);

    /* the col does not exists */
    if (col == 0 && !has_col_width && !column_width)
      empty = 0;  /* don't resize this col */

    /* if the width is defined to be 0, it can not be used */
    if (has_col_width && !column_width)
      empty = 0;  /* don't resize this col */

    if (column_width)
    {
      if (col < visible_num)
        width -= column_width;
    }
    else if (empty)
    {
      if (col < visible_num)
        empty_col_visible++;

      empty_columns[empty_num] = col;
      empty_num++;
    }
  }

  if (empty_num && empty_col_visible)
  {
    int i;

    column_width = width / empty_col_visible - (IMAT_PADDING_W + IMAT_FRAME_W);

    for (i = 0; i < empty_num; i++)
      iupAttribSetIntId(ih, "RASTERWIDTH", empty_columns[i], column_width);
  }

  free(empty_columns);
}

static int iMatrixSetFitToSizeAttrib(Ihandle* ih, const char* value)
{
  int w, h;
  int sb, sb_w = 0, sb_h = 0, border = 0;

  sb = iupMatrixGetScrollbar(ih);

  /* add scrollbar */
  if (sb)
  {
    int sb_size = iupMatrixGetScrollbarSize(ih);
    if (sb & IUP_SB_HORIZ)
      sb_h += sb_size;  /* sb horizontal affects vertical size */
    if (sb & IUP_SB_VERT)
      sb_w += sb_size;  /* sb vertical affects horizontal size */
  }

  if (iupAttribGetBoolean(ih, "BORDER"))
    border = 1;

  IupGetIntInt(ih, "RASTERSIZE", &w, &h);

  if (iupStrEqualNoCase(value, "LINES"))
    iMatrixFitLines(ih, h - sb_h - 2 * border);
  else if (iupStrEqualNoCase(value, "COLUMNS"))
    iMatrixFitColumns(ih, w - sb_w - 2 * border);
  else
  {
    iMatrixFitLines(ih, h - sb_h - 2 * border);
    iMatrixFitColumns(ih, w - sb_w - 2 * border);
  }

  ih->data->need_calcsize = 1;

  IupUpdate(ih); /* post a redraw, because FITTOSIZE can be set inside a resize_cb */
  return 0;
}

static void iMatrixFitColText(Ihandle* ih, int col)
{
  /* find the largest cel in the col */
  int lin, max_width = 0, max;

  for (lin = 0; lin < ih->data->lines.num; lin++)
  {
    char* title_value = iupMatrixGetValueDisplay(ih, lin, col);
    if (title_value && title_value[0])
    {
      int w;
      iupdrvFontGetMultiLineStringSize(ih, title_value, &w, NULL);
      if (w > max_width)
        max_width = w;
    }
  }

  max = iupAttribGetIntId(ih, "FITMAXWIDTH", col);
  if (max && max > max_width)
    max_width = max;

  iupAttribSetIntId(ih, "RASTERWIDTH", col, max_width);
}

static void iMatrixFitLineText(Ihandle* ih, int line)
{
  /* find the highest cel in the line */
  int col, max_height = 0, max;

  for (col = 0; col < ih->data->columns.num; col++)
  {
    char* title_value = iupMatrixGetValueDisplay(ih, line, col);
    if (title_value && title_value[0])
    {
      int h;
      iupdrvFontGetMultiLineStringSize(ih, title_value, NULL, &h);
      if (h > max_height)
        max_height = h;
    }
  }

  max = iupAttribGetIntId(ih, "FITMAXHEIGHT", line);
  if (max && max > max_height)
    max_height = max;

  iupAttribSetIntId(ih, "RASTERHEIGHT", line, max_height);
}

static int iMatrixSetFitToTextAttrib(Ihandle* ih, const char* value)
{
  if (!value || value[0] == 0)
    return 0;

  if (value[0] == 'C')
  {
    int col;
    if (iupStrToInt(value + 1, &col))
      iMatrixFitColText(ih, col);
  }
  else if (value[0] == 'L')
  {
    int line;
    if (iupStrToInt(value + 1, &line))
      iMatrixFitLineText(ih, line);
  }

  ih->data->need_calcsize = 1;

  IupUpdate(ih); /* post a redraw, because FITTOSIZE can be set inside a resize_cb */
  return 0;
}

static int iMatrixSetNumColNoScrollAttrib(Ihandle* ih, const char* value)
{
  int num = 0;
  if (iupStrToInt(value, &num))
  {
    if (num < 0) num = 0;

    num++; /* add room for title column */

    ih->data->columns.num_noscroll = num;
    if (ih->data->columns.num_noscroll >= ih->data->columns.num)
      ih->data->columns.num_noscroll = ih->data->columns.num - 1;
    if (ih->data->columns.num_noscroll < 1)
      ih->data->columns.num_noscroll = 1;

    if (ih->data->columns.first < ih->data->columns.num_noscroll)
    {
      ih->data->columns.first = ih->data->columns.num_noscroll;
      ih->data->columns.first_offset = 0;

      /* when "first" is changed must update scroll pos */
      iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_COL);
    }
    ih->data->need_calcsize = 1;

    if (ih->handle)
      iupMatrixDraw(ih, 1);
  }

  return 0;
}

static int iMatrixSetNumLinNoScrollAttrib(Ihandle* ih, const char* value)
{
  int num = 0;
  if (iupStrToInt(value, &num))
  {
    if (num < 0) num = 0;

    num++; /* add room for title line */

    ih->data->lines.num_noscroll = num;
    if (ih->data->lines.num_noscroll >= ih->data->lines.num)
      ih->data->lines.num_noscroll = ih->data->lines.num - 1;
    if (ih->data->lines.num_noscroll < 1)
      ih->data->lines.num_noscroll = 1;

    if (ih->data->lines.first < ih->data->lines.num_noscroll)
    {
      ih->data->lines.first = ih->data->lines.num_noscroll;
      ih->data->lines.first_offset = 0;

      /* when "first" is changed must update scroll pos */
      iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_LIN);
    }
    ih->data->need_calcsize = 1;

    if (ih->handle)
      iupMatrixDraw(ih, 1);
  }

  return 0;
}

static int iMatrixSetNoScrollAsTitleAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->noscroll_as_title = 1;
  else
    ih->data->noscroll_as_title = 0;
  return 0;
}

static char* iMatrixGetNoScrollAsTitleAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->noscroll_as_title);
}

static char* iMatrixGetNumColNoScrollAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->columns.num_noscroll - 1);  /* the attribute does not include the title */
}

static char* iMatrixGetNumLinNoScrollAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->lines.num_noscroll - 1);  /* the attribute does not include the title */
}

static int iMatrixSetCopyLinAttrib(Ihandle* ih, int from_lin, const char* value)
{
  int to_lin = 0;
  if (!iupStrToInt(value, &to_lin))
    return 0;

  if (!iupMATRIX_CHECK_LIN(ih, from_lin) || !iupMATRIX_CHECK_LIN(ih, to_lin) || from_lin == to_lin)
    return 0;

  iupMatrixAuxCopyLin(ih, from_lin, to_lin);
  iupMatrixDraw(ih, 1);

  return 0;
}

static int iMatrixSetCopyColAttrib(Ihandle* ih, int from_col, const char* value)
{
  int to_col = 0;
  if (!iupStrToInt(value, &to_col))
    return 0;

  if (!iupMATRIX_CHECK_COL(ih, from_col) || !iupMATRIX_CHECK_COL(ih, to_col) || from_col == to_col)
    return 0;

  iupMatrixAuxCopyCol(ih, from_col, to_col);
  iupMatrixDraw(ih, 1);

  return 0;
}

static int iMatrixSetMoveColAttrib(Ihandle* ih, int from_col, const char* value)
{
  char str[50];
  int to_col = 0;
  if (!iupStrToInt(value, &to_col))
    return 0;

  if (!iupMATRIX_CHECK_COL(ih, from_col) || !iupMATRIX_CHECK_COL(ih, to_col) || from_col == to_col)
    return 0;

  iupMatrixSetAddColAttrib(ih, value);
  if (to_col < from_col)
    from_col++;

  iMatrixSetCopyColAttrib(ih, from_col, value);

  sprintf(str, "%d", from_col);
  iupMatrixSetDelColAttrib(ih, str);
  return 0;
}

static int iMatrixSetMoveLinAttrib(Ihandle* ih, int from_lin, const char* value)
{
  char str[50];
  int to_lin = 0;
  if (!iupStrToInt(value, &to_lin))
    return 0;

  if (!iupMATRIX_CHECK_LIN(ih, from_lin) || !iupMATRIX_CHECK_LIN(ih, to_lin) || from_lin == to_lin)
    return 0;

  iupMatrixSetAddLinAttrib(ih, value);
  if (to_lin < from_lin)
    from_lin++;

  iMatrixSetCopyLinAttrib(ih, from_lin, value);

  sprintf(str, "%d", from_lin);
  iupMatrixSetDelLinAttrib(ih, str);
  return 0;
}

static int iMatrixSetSizeAttrib(Ihandle* ih, int pos, const char* value)
{
  (void)pos;
  (void)value;
  ih->data->need_calcsize = 1;
  IupUpdate(ih);  /* post a redraw */
  return 1;  /* always save in the hash table, so when FONT is changed SIZE can be updated */
}

static char* iMatrixGetWidthAttrib(Ihandle* ih, int col)
{
  return iupMatrixGetSize(ih, col, IMAT_PROCESS_COL, 0);
}

static char* iMatrixGetHeightAttrib(Ihandle* ih, int lin)
{
  return iupMatrixGetSize(ih, lin, IMAT_PROCESS_LIN, 0);
}

static char* iMatrixGetRasterWidthAttrib(Ihandle* ih, int col)
{
  return iupMatrixGetSize(ih, col, IMAT_PROCESS_COL, 1);
}

static char* iMatrixGetRasterHeightAttrib(Ihandle* ih, int lin)
{
  return iupMatrixGetSize(ih, lin, IMAT_PROCESS_LIN, 1);
}

static char* iMatrixGetAlignmentAttrib(Ihandle* ih, int col)
{
  char* align = iupAttribGetId(ih, "ALIGNMENT", col);
  if (!align)
  {
    align = iupAttribGet(ih, "ALIGNMENT");
    if (align)
      return align;
    else
    {
      if (col == 0)
        return "ALEFT";
      else
        return "ACENTER";
    }
  }

  return NULL;
}

static int iMatrixSetIdValueAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
    iupMatrixSetValue(ih, lin, col, value, 0);
  return 0;
}

static char* iMatrixGetIdValueAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
    return iupMatrixGetValue(ih, lin, col);
  return NULL;
}

static char* iMatrixGetCellAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
    return iupMatrixGetValueDisplay(ih, lin, col);
  return NULL;
}

static char* iMatrixGetCellOffsetAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
  {
    int x, y;
    if (iupMatrixGetCellOffset(ih, lin, col, &x, &y))
      return iupStrReturnIntInt(x, y, 'x');
  }
  return NULL;
}

static char* iMatrixGetCellSizeAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
    return iupStrReturnIntInt(ih->data->columns.dt[col].size, ih->data->lines.dt[lin].size, 'x');
  return NULL;
}

static int iMatrixSetNeedRedraw(Ihandle* ih)
{
  ih->data->need_redraw = 1;
  return 1;
}

static int iMatrixSetFlatAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->flat = 1;
  else
    ih->data->flat = 0;

  IupUpdate(ih);  /* post a redraw */
  return 0; /* do not store value in hash table */
}

static char* iMatrixGetFlatAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->flat);
}

static void iMatrixClearAttribFlags(Ihandle* ih, unsigned char *flags, int lin, int col)
{
  int is_marked = (*flags) & IMAT_IS_MARKED;

  if ((*flags) & IMAT_HAS_FONT)
    iupAttribSetId2(ih, "FONT", lin, col, NULL);

  if ((*flags) & IMAT_HAS_FGCOLOR)
    iupAttribSetId2(ih, "FGCOLOR", lin, col, NULL);

  if ((*flags) & IMAT_HAS_BGCOLOR)
    iupAttribSetId2(ih, "BGCOLOR", lin, col, NULL);

  if ((*flags) & IMAT_HAS_FRAMEHORIZCOLOR)
    iupAttribSetId2(ih, "FRAMEHORIZCOLOR", lin, col, NULL);

  if ((*flags) & IMAT_HAS_FRAMEVERTCOLOR)
    iupAttribSetId2(ih, "FRAMEVERTCOLOR", lin, col, NULL);

  if (lin == IUP_INVALID_ID)
  {
    iupAttribSetId(ih, "ALIGNMENT", col, NULL);
    iupAttribSetId(ih, "SORTSIGN", col, NULL);
  }

  if (col == IUP_INVALID_ID)
    iupAttribSetId(ih, "LINEALIGNMENT", lin, NULL);

  if (lin == 0)
    iupAttribSetId(ih, "ALIGNMENTLIN", 0, NULL);

  iupAttribSetId2(ih, "ALIGN", lin, col, NULL);

  *flags = 0; /* clear all flags, except marked state */
  if (is_marked)
    *flags = IMAT_IS_MARKED;
}

static int iMatrixSetClearAttribAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  if (lin == IUP_INVALID_ID && col == IUP_INVALID_ID)
  {
    if (iupStrEqualNoCase(value, "ALL"))
    {
      /* all cells attributes */
      if (!ih->data->callback_mode)
      {
        for (lin = 0; lin < ih->data->lines.num; lin++)
        {
          for (col = 0; col < ih->data->columns.num; col++)
            iMatrixClearAttribFlags(ih, &(ih->data->cells[lin][col].flags), lin, col);
        }
      }

      /* all line attributes */
      for (lin = 0; lin < ih->data->lines.num; lin++)
        iMatrixClearAttribFlags(ih, &(ih->data->lines.dt[lin].flags), lin, IUP_INVALID_ID);

      /* all column attributes */
      for (col = 0; col < ih->data->columns.num; col++)
        iMatrixClearAttribFlags(ih, &(ih->data->columns.dt[col].flags), IUP_INVALID_ID, col);
    }
    else if (iupStrEqualNoCase(value, "CONTENTS"))
    {
      if (!ih->data->callback_mode)
      {
        for (lin = 1; lin < ih->data->lines.num; lin++)
        {
          for (col = 1; col < ih->data->columns.num; col++)
            iMatrixClearAttribFlags(ih, &(ih->data->cells[lin][col].flags), lin, col);
        }
      }
    }
    else if (iupStrEqualNoCase(value, "MARKED"))
    {
      IFnii mark_cb = (IFnii)IupGetCallback(ih, "MARK_CB");

      if (!ih->data->callback_mode)
      {
        for (lin = 1; lin < ih->data->lines.num; lin++)
        {
          for (col = 1; col < ih->data->columns.num; col++)
          {
            if (iupMatrixGetMark(ih, lin, col, mark_cb))
              iMatrixClearAttribFlags(ih, &(ih->data->cells[lin][col].flags), lin, col);
          }
        }
      }
    }
  }
  else
  {
    if (lin == IUP_INVALID_ID)
    {
      int lin1 = 0, lin2 = ih->data->lines.num - 1;
      if (!iupStrEqualNoCase(value, "ALL"))
        iupStrToIntInt(value, &lin1, &lin2, '-');

      if (!iupMATRIX_CHECK_COL(ih, col) ||
          !iupMATRIX_CHECK_LIN(ih, lin1) ||
          !iupMATRIX_CHECK_LIN(ih, lin2))
          return 0;

      if (lin1 == 0 && lin2 == ih->data->lines.num - 1)
        iMatrixClearAttribFlags(ih, &(ih->data->columns.dt[col].flags), IUP_INVALID_ID, col);

      if (!ih->data->callback_mode)
      {
        for (lin = lin1; lin <= lin2; lin++)
          iMatrixClearAttribFlags(ih, &(ih->data->cells[lin][col].flags), lin, col);
      }
    }
    else if (col == IUP_INVALID_ID)
    {
      int col1 = 0, col2 = ih->data->columns.num - 1;
      if (!iupStrEqualNoCase(value, "ALL"))
        iupStrToIntInt(value, &col1, &col2, '-');

      if (!iupMATRIX_CHECK_LIN(ih, lin) ||
          !iupMATRIX_CHECK_COL(ih, col1) ||
          !iupMATRIX_CHECK_COL(ih, col2))
          return 0;

      if (col1 == 0 && col2 == ih->data->columns.num - 1)
        iMatrixClearAttribFlags(ih, &(ih->data->lines.dt[lin].flags), lin, IUP_INVALID_ID);

      if (!ih->data->callback_mode)
      {
        for (col = col1; col <= col2; col++)
          iMatrixClearAttribFlags(ih, &(ih->data->cells[lin][col].flags), lin, col);
      }
    }
    else
    {
      int lin1 = lin, lin2 = ih->data->lines.num - 1;
      int col1 = col, col2 = ih->data->columns.num - 1;
      iupStrToIntInt(value, &lin2, &col2, ':');

      if (!iupMATRIX_CHECK_LIN(ih, lin1) ||
          !iupMATRIX_CHECK_LIN(ih, lin2) ||
          !iupMATRIX_CHECK_COL(ih, col1) ||
          !iupMATRIX_CHECK_COL(ih, col2))
          return 0;

      if (lin1 == 0 && lin2 == ih->data->lines.num - 1)
      {
        for (col = 0; col < ih->data->columns.num; col++)
          iMatrixClearAttribFlags(ih, &(ih->data->columns.dt[col].flags), IUP_INVALID_ID, col);
      }

      if (col1 == 0 && col2 == ih->data->columns.num - 1)
      {
        for (lin = 0; lin < ih->data->lines.num; lin++)
          iMatrixClearAttribFlags(ih, &(ih->data->lines.dt[lin].flags), lin, IUP_INVALID_ID);
      }

      if (!ih->data->callback_mode)
      {
        for (lin = lin1; lin <= lin2; lin++)
        {
          for (col = col1; col <= col2; col++)
            iMatrixClearAttribFlags(ih, &(ih->data->cells[lin][col].flags), lin, col);
        }
      }
    }
  }

  if (ih->handle)
    iupMatrixDraw(ih, 1);

  return 0;
}

static int iMatrixSetClearValueAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  if (ih->data->undo_redo) iupAttribSetClassObject(ih, "UNDOPUSHBEGIN", "CLEARVALUE");

  if (lin == IUP_INVALID_ID && col == IUP_INVALID_ID)
  {
    if (iupStrEqualNoCase(value, "ALL"))
    {
      for (lin = 0; lin < ih->data->lines.num; lin++)
      {
        for (col = 0; col < ih->data->columns.num; col++)
          iupMatrixModifyValue(ih, lin, col, NULL);
      }
    }
    else if (iupStrEqualNoCase(value, "CONTENTS"))
    {
      for (lin = 1; lin < ih->data->lines.num; lin++)
      {
        for (col = 1; col < ih->data->columns.num; col++)
          iupMatrixModifyValue(ih, lin, col, NULL);
      }
    }
    else if (iupStrEqualNoCase(value, "MARKED"))
    {
      IFnii mark_cb = (IFnii)IupGetCallback(ih, "MARK_CB");

      for (lin = 1; lin < ih->data->lines.num; lin++)
      {
        for (col = 1; col < ih->data->columns.num; col++)
        {
          if (iupMatrixGetMark(ih, lin, col, mark_cb))
            iupMatrixModifyValue(ih, lin, col, NULL);
        }
      }
    }
  }
  else
  {
    if (lin == IUP_INVALID_ID)
    {
      int lin1 = 0, lin2 = ih->data->lines.num - 1;
      iupStrToIntInt(value, &lin1, &lin2, '-');

      if (!iupMATRIX_CHECK_COL(ih, col) ||
          !iupMATRIX_CHECK_LIN(ih, lin1) ||
          !iupMATRIX_CHECK_LIN(ih, lin2))
          return 0;

      for (lin = lin1; lin <= lin2; lin++)
        iupMatrixModifyValue(ih, lin, col, NULL);
    }
    else if (col == IUP_INVALID_ID)
    {
      int col1 = 0, col2 = ih->data->columns.num - 1;
      iupStrToIntInt(value, &col1, &col2, '-');

      if (!iupMATRIX_CHECK_LIN(ih, lin) ||
          !iupMATRIX_CHECK_COL(ih, col1) ||
          !iupMATRIX_CHECK_COL(ih, col2))
          return 0;

      for (col = col1; col <= col2; col++)
        iupMatrixModifyValue(ih, lin, col, NULL);
    }
    else
    {
      int lin1 = lin, lin2 = ih->data->lines.num - 1;
      int col1 = col, col2 = ih->data->columns.num - 1;
      iupStrToIntInt(value, &lin2, &col2, ':');

      if (!iupMATRIX_CHECK_LIN(ih, lin1) ||
          !iupMATRIX_CHECK_LIN(ih, lin2) ||
          !iupMATRIX_CHECK_COL(ih, col1) ||
          !iupMATRIX_CHECK_COL(ih, col2))
          return 0;

      for (lin = lin1; lin <= lin2; lin++)
      {
        for (col = col1; col <= col2; col++)
          iupMatrixModifyValue(ih, lin, col, NULL);
      }
    }
  }

  if (ih->data->undo_redo) iupAttribSetClassObject(ih, "UNDOPUSHEND", NULL);

  iupBaseCallValueChangedCb(ih);

  if (ih->handle)
    iupMatrixDraw(ih, 1);

  return 0;
}

static int iMatrixSetShowFillValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->show_fill_value = 1;
  else
    ih->data->show_fill_value = 0;
  return 0;
}

static char* iMatrixGetShowFillValueAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->show_fill_value);
}

static int iMatrixSetAttribFlags(Ihandle* ih, int lin, int col, const char* value, unsigned char attr)
{
  if (lin >= 0 || col >= 0)
  {
    iupMatrixSetCellFlag(ih, lin, col, attr, value != NULL);
    ih->data->need_redraw = 1;
  }
  return 1;
}

static int iMatrixSetBgColorAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  return iMatrixSetAttribFlags(ih, lin, col, value, IMAT_HAS_BGCOLOR);
}

static int iMatrixSetFgColorAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  return iMatrixSetAttribFlags(ih, lin, col, value, IMAT_HAS_FGCOLOR);
}

static int iMatrixSetTypeAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  return iMatrixSetAttribFlags(ih, lin, col, value, IMAT_HAS_TYPE);
}

static char* iMatrixGetFontAttribute(Ihandle* ih, int lin, int col)
{
  char* font = NULL;

  if (lin != IUP_INVALID_ID && col != IUP_INVALID_ID)
    font = iupAttribGetId2(ih, "FONT", lin, col);
  if (!font && lin != IUP_INVALID_ID)
    font = iupAttribGetId2(ih, "FONT", lin, IUP_INVALID_ID);
  if (!font && col != IUP_INVALID_ID)
    font = iupAttribGetId2(ih, "FONT", IUP_INVALID_ID, col);
  if (!font)
    font = IupGetAttribute(ih, "FONT");

  return font;
}

static int iMatrixSetFontStyleAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (!value)
    return 0;

  font = iMatrixGetFontAttribute(ih, lin, col);
  if (!font)
    return 0;

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  IupSetfAttributeId2(ih, "FONT", lin, col, "%s, %s %d", typeface, value, size);

  return 0;
}

static char* iMatrixGetFontStyleAttrib(Ihandle* ih, int lin, int col)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];

  char* font = iMatrixGetFontAttribute(ih, lin, col);
  if (!font)
    return NULL;

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  return iupStrReturnStrf("%s%s%s%s", is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "");
}

static int iMatrixSetFontSizeAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  char* font;

  if (!value)
    return 0;

  font = iMatrixGetFontAttribute(ih, lin, col);
  if (!font)
    return 0;

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return 0;

  IupSetfAttributeId2(ih, "FONT", lin, col, "%s, %s%s%s%s %s", typeface, is_bold ? "Bold " : "", is_italic ? "Italic " : "", is_underline ? "Underline " : "", is_strikeout ? "Strikeout " : "", value);

  return 0;
}

static char* iMatrixGetFontSizeAttrib(Ihandle* ih, int lin, int col)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0,
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];

  char* font = iMatrixGetFontAttribute(ih, lin, col);
  if (!font)
    return NULL;

  if (!iupGetFontInfo(font, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return NULL;

  return iupStrReturnInt(size);
}

static int iMatrixSetFontAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  if (lin == IUP_INVALID_ID && col == IUP_INVALID_ID)  /* empty id */
  {
    if (!value)
      value = IupGetGlobal("DEFAULTFONT");

    if (!iupdrvSetFontAttrib(ih, value))
      return 0;

    return 1;
  }

  return iMatrixSetAttribFlags(ih, lin, col, value, IMAT_HAS_FONT);
}

static char* iMatrixGetFontAttrib(Ihandle* ih, int lin, int col)
{
  if (lin == IUP_INVALID_ID && col == IUP_INVALID_ID)  /* empty id */
  {
    char* value = iupAttribGetInherit(ih, "FONT");
    if (!value)
      return IupGetGlobal("DEFAULTFONT");
    else
      return value;
  }
  return NULL;
}

static int iMatrixSetFrameHorizColorAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  return iMatrixSetAttribFlags(ih, lin, col, value, IMAT_HAS_FRAMEHORIZCOLOR);
}

static int iMatrixSetFrameVertColorAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  return iMatrixSetAttribFlags(ih, lin, col, value, IMAT_HAS_FRAMEVERTCOLOR);
}

static int iMatrixSetMergeAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  int l, c;

  if (lin == 0 && col == 0)
    return 0;

  if (iupStrToIntInt(value, &l, &c, ':') == 2)
  {
    /* merge of title cells can not include regular cells */
    if (lin == 0 && l != 0) 
      return 0;

    if (col == 0 && c != 0)
      return 0;

    /* can not merge with already merged cells */
    if (iupMatrixHasMerged(ih, lin, l, col, c))
      return 0;

    iupMatrixMergeRange(ih, lin, l, col, c);
  }

  return 0;
}

static char* iMatrixGetMergeAttrib(Ihandle* ih, int lin, int col)
{
  int merged = iupMatrixGetMerged(ih, lin, col);
  return iupStrReturnBoolean(merged);
}

static int iMatrixSetMergeSplitAttrib(Ihandle* ih, const char* value)
{
  int lin, col;
  if (iupStrToIntInt(value, &lin, &col, ':') == 2)
  {
    int merged = iupMatrixGetMerged(ih, lin, col);
    if (merged)
      iupMatrixMergeSplitRange(ih, merged);
  }
  return 0;
}

static char* iMatrixGetMergedStartAttrib(Ihandle* ih, int lin, int col)
{
  int merged = iupMatrixGetMerged(ih, lin, col);
  if (merged)
  {
    int startLin, startCol;
    iupMatrixGetMergedRect(ih, merged, &startLin, NULL, &startCol, NULL);
    return iupStrReturnIntInt(startLin, startCol, ':');
  }
  else
    return NULL;
}

static char* iMatrixGetMergedEndAttrib(Ihandle* ih, int lin, int col)
{
  int merged = iupMatrixGetMerged(ih, lin, col);
  if (merged)
  {
    int endLin, endCol;
    iupMatrixGetMergedRect(ih, merged, NULL, &endLin, NULL, &endCol);
    return iupStrReturnIntInt(endLin, endCol, ':');
  }
  else
    return NULL;
}

static char* iMatrixGetBgColorAttrib(Ihandle* ih, int lin, int col)
{
  if (lin == IUP_INVALID_ID && col == IUP_INVALID_ID) /* empty id - return the global default value */
  {
    /* check the hash table */
    char *color = iupAttribGet(ih, "BGCOLOR");

    /* If not defined return the default for normal cells */
    if (!color)
      color = IupGetGlobal("TXTBGCOLOR");

    return color;
  }
  return NULL;
}

static char* iMatrixGetCellBgColorAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
  {
    unsigned char r = 255, g = 255, b = 255;
    int active = iupdrvIsActive(ih);
    char* mark = iupMatrixGetMarkAttrib(ih, lin, col);

    iupMatrixGetBgRGB(ih, lin, col, &r, &g, &b, mark && mark[0] == '1', active);

    return iupStrReturnRGB(r, g, b);
  }
  else
    return NULL;
}

static char* iMatrixGetCellFgColorAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
  {
    unsigned char r = 0, g = 0, b = 0;
    int active = iupdrvIsActive(ih);
    char* mark = iupMatrixGetMarkAttrib(ih, lin, col);

    iupMatrixGetFgRGB(ih, lin, col, &r, &g, &b, mark && mark[0] == '1', active);

    return iupStrReturnRGB(r, g, b);
  }
  else
    return NULL;
}

static char* iMatrixGetCellFontAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
    return iupMatrixGetFont(ih, lin, col);
  else
    return NULL;
}

static char* iMatrixGetCellTypeAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
  {
    int type = iupMatrixGetType(ih, lin, col);
    if (type == IMAT_TYPE_COLOR)
      return "COLOR";
    else if (type == IMAT_TYPE_FILL)
      return "FILL";
    else if (type == IMAT_TYPE_IMAGE)
      return "IMAGE";
    else /* TEXT */
      return "TEXT";
  }
  else
    return NULL;
}

static char* iMatrixGetCellAlignmentAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
  {
    const char* vert_align_str[3] = {"ACENTER", "ATOP", "ABOTTOM"};
    const char* horiz_align_str[3] = { "ACENTER", "ALEFT", "ARIGHT" };
    char* str = iupStrGetMemory(30);
    int col_alignment, lin_alignment;

    if (lin == 0)
      col_alignment = iupMatrixGetColAlignmentLin0(ih);
    else
      col_alignment = iupMatrixGetColAlignment(ih, col);

    lin_alignment = iupMatrixGetLinAlignment(ih, lin);

    iupMatrixGetCellAlign(ih, lin, col, &col_alignment, &lin_alignment);

    sprintf(str, "%s:%s", horiz_align_str[col_alignment], vert_align_str[lin_alignment]);
    return str;
  }
  else
    return NULL;
}

static char* iMatrixGetCellFrameHorizColorAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
  {
    long framecolor;  /* RGB encoded as (R<<16)|(G<<8)|B */
    int transp, check_title = 0;
    if (lin == 0 || col == 0)
      check_title = 1;

    transp = iupMatrixGetFrameHorizColor(ih, lin, col, &framecolor, check_title);
    if (transp)
      return NULL;

    /* Extract RGB from encoded long */
    return iupStrReturnRGB((framecolor >> 16) & 0xFF, (framecolor >> 8) & 0xFF, framecolor & 0xFF);
  }
  else
    return NULL;
}

static char* iMatrixGetCellFrameVertColorAttrib(Ihandle* ih, int lin, int col)
{
  if (iupMatrixCheckCellPos(ih, lin, col))
  {
    long framecolor;  /* RGB encoded as (R<<16)|(G<<8)|B */
    int transp, check_title = 0;
    if (lin == 0 || col == 0)
      check_title = 1;

    transp = iupMatrixGetFrameVertColor(ih, lin, col, &framecolor, check_title);
    if (transp)
      return NULL;

    /* Extract RGB from encoded long */
    return iupStrReturnRGB((framecolor >> 16) & 0xFF, (framecolor >> 8) & 0xFF, framecolor & 0xFF);
  }
  else
    return NULL;
}

static void iMatrixConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  *pos = lin*ih->data->columns.num + col;
}

static void iMatrixConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  *lin = pos / ih->data->columns.num;
  *col = pos % ih->data->columns.num;
}

static int iMatrixConvertXYToPos(Ihandle* ih, int x, int y)
{
  int lin, col;
  if (iupMatrixGetCellFromXY(ih, x, y, &lin, &col))
  {
    int pos;
    iMatrixConvertLinColToPos(ih, lin, col, &pos);
    return pos;
  }
  return -1;
}

static char* iMatrixGetNumColVisibleAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->columns.last - ih->data->columns.first);
}

static char* iMatrixGetNumLinVisibleAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->lines.last - ih->data->lines.first);
}

static int iMatrixSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrBoolean(value) && ih->data->editing)
    iupMatrixEditHide(ih);

  iupBaseSetActiveAttrib(ih, value);
  iupMatrixDraw(ih, 1);
  return 0;
}

static int iMatrixSetVisibleAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrBoolean(value) && ih->data->editing)
    iupMatrixEditHide(ih);

  return iupBaseSetVisibleAttrib(ih, value);
}

static int iMatrixWheel_CB(Ihandle* ih, float delta)
{
  iupFlatScrollBarWheelUpdate(ih, delta);
  return IUP_DEFAULT;
}

static int iMatrixSetFlatScrollbarAttrib(Ihandle* ih, const char* value)
{
  /* can only be set before map */
  if (ih->handle)
    return IUP_DEFAULT;

  if (value && !iupStrEqualNoCase(value, "NO"))
  {
    if (iupFlatScrollBarCreate(ih))
    {
      IupSetAttribute(ih, "SCROLLBAR", "NO");
      IupSetCallback(ih, "WHEEL_CB", (Icallback)iMatrixWheel_CB);
    }
    return 1;
  }
  else
    return 0;
}

/*****************************************************************************/
/*   Callbacks registered to the Canvas                                      */
/*****************************************************************************/

static int iMatrixFocus_CB(Ihandle* ih, int focus)
{
  int rc = IUP_DEFAULT;

  if (!iupMatrixIsValid(ih, 1))
    return IUP_DEFAULT;

  if (IupGetGlobal("MOTIFVERSION"))
  {
    if (iupAttribGet(ih, "_IUPMAT_DROPDOWN") ||  /* from iMatrixEditDropDown_CB, in Motif */
        iupAttribGet(ih, "_IUPMAT_DOUBLECLICK"))  /* from iMatrixMouseLeftPress, in Motif */
        return IUP_DEFAULT;
  }

  ih->data->has_focus = focus;
  iupMatrixDrawUpdate(ih);

  if (focus)
    iupMatrixAuxCallEnterCellCb(ih);
  else
    iupMatrixAuxCallLeaveCellCb(ih);

  return rc;
}

static int iMatrixResize_CB(Ihandle* ih)
{
  int old_w = ih->data->w,
      old_h = ih->data->h;

  /* IupDraw: Get canvas size directly */
  IupGetIntInt(ih, "DRAWSIZE", &(ih->data->w), &(ih->data->h));

  if (old_w != ih->data->w || old_h != ih->data->h)
  {
    ih->data->need_calcsize = 1;

    if (ih->data->edit_hide_onfocus)
    {
      ih->data->edit_hidden_byfocus = 1;
      iupMatrixEditHide(ih);
      ih->data->edit_hidden_byfocus = 0;
    }
  }

  /* IupDraw: Do NOT set DX/DY here - it triggers multiple ACTION callbacks.
   * DX/DY will be set in iupMatrixAuxCalcSizes during first ACTION callback. */

  if (ih->data->columns.num > 0 && ih->data->lines.num > 0)
  {
    IFnii cb = (IFnii)IupGetCallback(ih, "RESIZEMATRIX_CB");
    if (cb) cb(ih, ih->data->w, ih->data->h);
  }

  return IUP_DEFAULT;
}

static int iMatrixRedraw_CB(Ihandle* ih)
{
  /* IupDraw: Check if widget is mapped instead of checking cd_canvas */
  if (!ih->handle)
    return IUP_DEFAULT;

  /* Prevent recursion when resize triggers another redraw
   * (canvas.inside_resize is set by GTK/Qt resize handlers) */
  if (ih->data->canvas.inside_resize)
    return IUP_DEFAULT;

  /* Prevent recursion when CalcSizes sets DX/DY which triggers ACTION again */
  if (ih->data->inside_scroll_update)
    return IUP_DEFAULT;

  /* IupDraw: Always call iupMatrixDrawCB on every ACTION.
   * Unlike CD canvas which had automatic buffering, IupDraw requires us to redraw on every ACTION.
   * The persistent buffer is managed by the driver, but we must draw to it every time. */
  iupMatrixDrawCB(ih);

  return IUP_DEFAULT;
}

/***************************************************************************/

static int iMatrixCreateMethod(Ihandle* ih, void **params)
{
  if (params && params[0])
  {
    char* action_cb = (char*)params[0];
    iupAttribSetStr(ih, "ACTION_CB", action_cb);
  }

  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  /* change the IupCanvas default values */
  iupAttribSet(ih, "SCROLLBAR", "YES");
  iupAttribSet(ih, "BORDER", "NO");
  iupAttribSet(ih, "CURSOR", "IupMatrixCrossCursor");

  /* IupCanvas callbacks */
  IupSetCallback(ih, "ACTION", (Icallback)iMatrixRedraw_CB);
  IupSetCallback(ih, "RESIZE_CB", (Icallback)iMatrixResize_CB);
  IupSetCallback(ih, "FOCUS_CB", (Icallback)iMatrixFocus_CB);
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iupMatrixMouseButton_CB);
  IupSetCallback(ih, "MOTION_CB", (Icallback)iupMatrixMouseMove_CB);
  IupSetCallback(ih, "KEYPRESS_CB", (Icallback)iupMatrixKeyPress_CB);
  IupSetCallback(ih, "SCROLL_CB", (Icallback)iupMatrixScroll_CB);

  /* Create the edit fields */
  iupMatrixEditCreate(ih);

  /* defaults that are non zero */
  ih->data->datah = ih->data->texth;
  ih->data->mark_continuous = 1;
  ih->data->columns.num = 1;
  ih->data->lines.num = 1;
  ih->data->columns.num_noscroll = 1;
  ih->data->lines.num_noscroll = 1;
  ih->data->noscroll_as_title = 0;
  ih->data->need_calcsize = 1;
  ih->data->need_redraw = 1;
  ih->data->lines.first = 1;
  ih->data->columns.first = 1;
  ih->data->lines.focus_cell = 1;
  ih->data->columns.focus_cell = 1;
  ih->data->mark_lin1 = -1;
  ih->data->mark_col1 = -1;
  ih->data->mark_lin2 = -1;
  ih->data->mark_col2 = -1;
  ih->data->edit_hide_onfocus = 1;

  return IUP_NOERROR;
}

static void iMatrixDestroyMethod(Ihandle* ih)
{
  iupFlatScrollBarRelease(ih);
}

static int iMatrixMapMethod(Ihandle* ih)
{
  /* IupDraw: No explicit canvas creation needed - available automatically for canvas controls */

  if (IupGetCallback(ih, "VALUE_CB"))
  {
    ih->data->callback_mode = 1;

    if (!IupGetCallback(ih, "VALUE_EDIT_CB"))
      iupAttribSet(ih, "READONLY", "YES");
  }

  iupMatrixMemAlloc(ih);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)iMatrixConvertXYToPos);
  IupSetCallback(ih, "_IUP_POS2LINCOL_CB", (Icallback)iMatrixConvertPosToLinCol);
  IupSetCallback(ih, "_IUP_LINCOL2POS_CB", (Icallback)iMatrixConvertLinColToPos);

  return IUP_NOERROR;
}

static void iMatrixUnMapMethod(Ihandle* ih)
{
  /* IupDraw: No canvas cleanup needed */
  iupMatrixMemRelease(ih);
}

static int iMatrixGetNaturalWidth(Ihandle* ih, int *full_width)
{
  int width = 0, visible_num, col;

  /* get only from the hash table or the default value, do NOT get from the actual number of visible columns */
  visible_num = iupAttribGetInt(ih, "NUMCOL_VISIBLE") + 1;  /* include the title column */

  if (iupAttribGetInt(ih, "NUMCOL_VISIBLE_LAST"))
  {
    int start = ih->data->columns.num - (visible_num - 1);
    if (start < 1) start = 1;  /* title is computed apart */

    width += iupMatrixGetColumnWidth(ih, 0, 1); /* always compute title */

    for (col = start; col < ih->data->columns.num; col++)
      width += iupMatrixGetColumnWidth(ih, col, 1);

    if (ih->data->limit_expand)
    {
      *full_width = width;
      for (col = 1; col < start; col++)
        (*full_width) += iupMatrixGetColumnWidth(ih, col, 1);
    }
  }
  else
  {
    for (col = 0; col < visible_num; col++)  /* visible_num can be > numcol */
      width += iupMatrixGetColumnWidth(ih, col, 1);

    if (ih->data->limit_expand)
    {
      *full_width = width;
      for (col = visible_num; col < ih->data->columns.num; col++)
        (*full_width) += iupMatrixGetColumnWidth(ih, col, 1);
    }
  }

  return width;
}

static int iMatrixGetNaturalHeight(Ihandle* ih, int *full_height)
{
  int height = 0, visible_num, lin;

  visible_num = iupAttribGetInt(ih, "NUMLIN_VISIBLE") + 1;  /* add the title line */

  if (iupAttribGetInt(ih, "NUMLIN_VISIBLE_LAST"))
  {
    int start = ih->data->lines.num - (visible_num - 1);
    if (start < 1) start = 1;  /* title is computed apart */

    height += iupMatrixGetLineHeight(ih, 0, 1);  /* always compute title */

    for (lin = start; lin < ih->data->lines.num; lin++)
      height += iupMatrixGetLineHeight(ih, lin, 1);

    if (ih->data->limit_expand)
    {
      *full_height = height;
      for (lin = 1; lin < start; lin++)
        (*full_height) += iupMatrixGetLineHeight(ih, lin, 1);
    }
  }
  else
  {
    for (lin = 0; lin < visible_num; lin++)  /* visible_num can be > numlin */
    {
      int line_height = iupMatrixGetLineHeight(ih, lin, 1);
      height += line_height;
    }

    if (ih->data->limit_expand)
    {
      *full_height = height;
      for (lin = visible_num; lin < ih->data->lines.num; lin++)
        (*full_height) += iupMatrixGetLineHeight(ih, lin, 1);
    }
  }

  return height;
}

int iupMatrixGetWidth(Ihandle* ih)
{
  /* IupDraw: DRAWSIZE already represents the drawable area.
     Unlike CD canvas, no scrollbar subtraction needed. */
  return ih->data->w;
}

int iupMatrixGetHeight(Ihandle* ih)
{
  /* IupDraw: DRAWSIZE already represents the drawable area.
     Unlike CD canvas, no scrollbar subtraction needed. */
  return ih->data->h;
}

int iupMatrixGetScrollbar(Ihandle* ih)
{
  int flat = iupFlatScrollBarGet(ih);
  if (flat != IUP_SB_NONE)
    return flat;
  else
  {
    if (!ih->handle)
      ih->data->canvas.sb = iupBaseGetScrollbar(ih);

    return ih->data->canvas.sb;
  }
}

int iupMatrixGetScrollbarSize(Ihandle* ih)
{
  if (iupFlatScrollBarGet(ih) != IUP_SB_NONE)
  {
    if (iupAttribGetBoolean(ih, "SHOWFLOATING"))
      return 0;
    else
      return iupAttribGetInt(ih, "SCROLLBARSIZE");
  }
  else
    return iupdrvGetScrollbarSize();
}

static void iMatrixComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  int sb, sb_w = 0, sb_h = 0, full_width = 0, full_height = 0, border = 0;
  (void)children_expand; /* unset if not name container */

  sb = iupMatrixGetScrollbar(ih);

  /* add scrollbar */
  if (sb)
  {
    int sb_size = iupMatrixGetScrollbarSize(ih);
    if (sb & IUP_SB_HORIZ)
      sb_h += sb_size;  /* sb horizontal affects vertical size */
    if (sb & IUP_SB_VERT)
      sb_w += sb_size;  /* sb vertical affects horizontal size */
  }

  if (iupAttribGetBoolean(ih, "BORDER"))
    border = 1;

  *w = iMatrixGetNaturalWidth(ih, &full_width);
  *h = iMatrixGetNaturalHeight(ih, &full_height);

  if (ih->data->limit_expand)
  {
    /* the maximum size does NOT include the scrollbars when AUTOHIDE=Yes */
    if (!iupAttribGetBoolean(ih, "YAUTOHIDE"))
      full_width += sb_w;
    else if (*w == full_width) /* all columns can be visible, */
      sb_w = 0;                /* so do NOT reserve space for the vertical scrollbar */
    if (!iupAttribGetBoolean(ih, "XAUTOHIDE"))
      full_height += sb_h;
    else if (*h == full_height) /* all lines can be visible, */
      sb_h = 0;                 /* so do NOT reserve space for the horizontal scrollbar */

    full_width += 2 * border;
    full_height += 2 * border;

    IupSetfAttribute(ih, "MAXSIZE", "%dx%d", full_width, full_height);
  }

  *w += sb_w + 2 * border;
  *h += sb_h + 2 * border;
}

static void iMatrixSetChildrenCurrentSizeMethod(Ihandle* ih, int shrink)
{
  if (iupFlatScrollBarGet(ih) != IUP_SB_NONE)
    iupFlatScrollBarSetChildrenCurrentSize(ih, shrink);
}

static void iMatrixSetChildrenPositionMethod(Ihandle* ih, int x, int y)
{
  if (iupFlatScrollBarGet(ih) != IUP_SB_NONE)
    iupFlatScrollBarSetChildrenPosition(ih);

  (void)x;
  (void)y;
}

static void iMatrixCreateCursor(void)
{
  Ihandle *imgcursor;
  unsigned char matrx_img_cur_excel[15 * 15] =
  {
    0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 2, 2, 2, 2, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 2, 2, 2, 2, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 2, 2, 2, 2, 1, 1, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 1, 2, 2, 2, 2, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 2, 2, 2, 2, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

  imgcursor = IupImage(15, 15, matrx_img_cur_excel);
  IupSetAttribute(imgcursor, "0", "BGCOLOR");
  IupSetAttribute(imgcursor, "1", "0 0 0");
  IupSetAttribute(imgcursor, "2", "255 255 255");
  IupSetAttribute(imgcursor, "HOTSPOT", "7:7");     /* Centered Hotspot           */
  IupSetHandle("IupMatrixCrossCursor", imgcursor);
  IupSetHandle("matrx_img_cur_excel", imgcursor);  /* for backward compatibility */
}

Iclass* iupMatrixNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = "matrix";
  ic->format = "a"; /* one ACTION_CB callback name */
  ic->format_attr = "ACTION_CB";
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 2;   /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->New = iupMatrixNewClass;
  ic->Create = iMatrixCreateMethod;
  ic->Destroy = iMatrixDestroyMethod;
  ic->Map = iMatrixMapMethod;
  ic->UnMap = iMatrixUnMapMethod;
  ic->ComputeNaturalSize = iMatrixComputeNaturalSizeMethod;
  ic->SetChildrenCurrentSize = iMatrixSetChildrenCurrentSizeMethod;
  ic->SetChildrenPosition = iMatrixSetChildrenPositionMethod;

  /* Do not need to set base attributes because they are inherited from IupCanvas */

  /* IupMatrix Callbacks */
  /* --- Interaction --- */
  iupClassRegisterCallback(ic, "ACTION_CB", "iiiis");
  iupClassRegisterCallback(ic, "CLICK_CB", "iis");
  iupClassRegisterCallback(ic, "RELEASE_CB", "iis");
  iupClassRegisterCallback(ic, "MOUSEMOVE_CB", "ii");
  iupClassRegisterCallback(ic, "ENTERITEM_CB", "ii");
  iupClassRegisterCallback(ic, "LEAVEITEM_CB", "ii");
  iupClassRegisterCallback(ic, "SCROLLTOP_CB", "ii");
  iupClassRegisterCallback(ic, "COLRESIZE_CB", "i");
  iupClassRegisterCallback(ic, "TOGGLEVALUE_CB", "iii");
  iupClassRegisterCallback(ic, "RESIZEMATRIX_CB", "ii");
  iupClassRegisterCallback(ic, "EDITCLICK_CB", "iis");
  iupClassRegisterCallback(ic, "EDITRELEASE_CB", "iis");
  iupClassRegisterCallback(ic, "EDITMOUSEMOVE_CB", "ii");
  /* --- Drawing --- */
  iupClassRegisterCallback(ic, "BGCOLOR_CB", "iiIII");
  iupClassRegisterCallback(ic, "FGCOLOR_CB", "iiIII");
  iupClassRegisterCallback(ic, "FONT_CB", "ii=s");
  iupClassRegisterCallback(ic, "DRAW_CB", "iiiiiiC");
  iupClassRegisterCallback(ic, "DROPCHECK_CB", "ii");
  iupClassRegisterCallback(ic, "TYPE_CB", "ii=s");
  iupClassRegisterCallback(ic, "TRANSLATEVALUE_CB", "iis=s");
  /* --- Editing --- */
  iupClassRegisterCallback(ic, "MENUDROP_CB", "nii");
  iupClassRegisterCallback(ic, "DROP_CB", "nii");
  iupClassRegisterCallback(ic, "DROPSELECT_CB", "iinsii");
  iupClassRegisterCallback(ic, "EDITION_CB", "iii");
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
  /* --- Callback Mode --- */
  iupClassRegisterCallback(ic, "VALUE_CB", "ii=s");
  iupClassRegisterCallback(ic, "VALUE_EDIT_CB", "iis");
  iupClassRegisterCallback(ic, "MARK_CB", "ii");
  iupClassRegisterCallback(ic, "MARKEDIT_CB", "iii");

  /* Overwrite IupCanvas Attributes */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iMatrixSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "VISIBLE", iupBaseGetVisibleAttrib, iMatrixSetVisibleAttrib, "YES", "NO", IUPAF_NO_SAVE);

  /* Change the Canvas default */
  iupClassRegisterReplaceAttribDef(ic, "CURSOR", "IupMatrixCrossCursor", "ARROW");
  iupClassRegisterReplaceAttribDef(ic, "BORDER", "NO", "YES");
  iupClassRegisterReplaceAttribDef(ic, "SCROLLBAR", "YES", NULL);

  /* IupMatrix Attributes - CELL */
  iupClassRegisterAttributeId2(ic, "IDVALUE", iMatrixGetIdValueAttrib, iMatrixSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FOCUSCELL", iMatrixGetFocusCellAttrib, iMatrixSetFocusCellAttrib, IUPAF_SAMEASSYSTEM, "1:1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT); /* can be NOT mapped */
  /*OLD*/iupClassRegisterAttribute(ic, "FOCUS_CELL", iMatrixGetFocusCellAttrib, iMatrixSetFocusCellAttrib, IUPAF_SAMEASSYSTEM, "1:1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT); /* can be NOT mapped */
  iupClassRegisterAttribute(ic, "VALUE", iMatrixGetValueAttrib, iMatrixSetValueAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "BGCOLOR", iMatrixGetBgColorAttrib, iMatrixSetBgColorAttrib, IUPAF_NOT_MAPPED);
  iupClassRegisterAttributeId2(ic, "FGCOLOR", NULL, iMatrixSetFgColorAttrib, IUPAF_NOT_MAPPED);
  iupClassRegisterAttributeId2(ic, "TYPE", NULL, iMatrixSetTypeAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "FONT", iMatrixGetFontAttrib, iMatrixSetFontAttrib, IUPAF_NOT_MAPPED);
  iupClassRegisterAttributeId2(ic, "FONTSTYLE", iMatrixGetFontStyleAttrib, iMatrixSetFontStyleAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "FONTSIZE", iMatrixGetFontSizeAttrib, iMatrixSetFontSizeAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "FRAMEHORIZCOLOR", NULL, iMatrixSetFrameHorizColorAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "FRAMEVERTCOLOR", NULL, iMatrixSetFrameVertColorAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "FRAMETITLEHORIZCOLOR", NULL, iMatrixSetFrameHorizColorAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "FRAMETITLEVERTCOLOR", NULL, iMatrixSetFrameVertColorAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMECOLOR", NULL, (IattribSetFunc)iMatrixSetNeedRedraw, IUPAF_SAMEASSYSTEM, "100 100 100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMETITLEHIGHLIGHT", NULL, (IattribSetFunc)iMatrixSetNeedRedraw, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FRAMEBORDER", NULL, (IattribSetFunc)iMatrixSetNeedRedraw, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CELLOFFSET", iMatrixGetCellOffsetAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CELLSIZE", iMatrixGetCellSizeAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CELLBGCOLOR", iMatrixGetCellBgColorAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CELLFGCOLOR", iMatrixGetCellFgColorAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CELLFONT", iMatrixGetCellFontAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CELLTYPE", iMatrixGetCellTypeAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CELLFRAMEHORIZCOLOR", iMatrixGetCellFrameHorizColorAttrib, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CELLFRAMEVERTCOLOR", iMatrixGetCellFrameVertColorAttrib, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CELLALIGNMENT", iMatrixGetCellAlignmentAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CELL", iMatrixGetCellAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "TOGGLEVALUE", NULL, (IattribSetId2Func)iMatrixSetNeedRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "ALIGN", NULL, (IattribSetId2Func)iMatrixSetNeedRedraw, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOGGLECENTERED", NULL, (IattribSetFunc)iMatrixSetNeedRedraw, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOGGLEIMAGEON", NULL, (IattribSetFunc)iMatrixSetNeedRedraw, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOGGLEIMAGEOFF", NULL, (IattribSetFunc)iMatrixSetNeedRedraw, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FLAT", iMatrixGetFlatAttrib, iMatrixSetFlatAttrib, NULL, NULL, IUPAF_NOT_MAPPED);

  iupClassRegisterAttributeId2(ic, "MERGE", iMatrixGetMergeAttrib, iMatrixSetMergeAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MERGESPLIT", NULL, iMatrixSetMergeSplitAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "MERGED", NULL, NULL, IUPAF_NO_INHERIT); /* internal, returns the merged range number */
  iupClassRegisterAttributeId2(ic, "MERGEDSTART", iMatrixGetMergedStartAttrib, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "MERGEDEND", iMatrixGetMergedEndAttrib, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* IupMatrix Attributes - COLUMN */
  iupClassRegisterAttributeId(ic, "ALIGNMENT", iMatrixGetAlignmentAttrib, (IattribSetIdFunc)iMatrixSetNeedRedraw, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENTLIN0", NULL, (IattribSetFunc)iMatrixSetNeedRedraw, IUPAF_SAMEASSYSTEM, "ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LINEALIGNMENT", NULL, (IattribSetIdFunc)iMatrixSetNeedRedraw, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SORTSIGN", NULL, (IattribSetIdFunc)iMatrixSetNeedRedraw, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SORTIMAGEDOWN", NULL, (IattribSetFunc)iMatrixSetNeedRedraw, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SORTIMAGEUP", NULL, (IattribSetFunc)iMatrixSetNeedRedraw, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);

  /* IupMatrix Attributes - SIZE */
  iupClassRegisterAttribute(ic, "COUNT", iMatrixGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMLIN", iMatrixGetNumLinAttrib, iupMatrixSetNumLinAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMCOL", iMatrixGetNumColAttrib, iupMatrixSetNumColAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMLIN_NOSCROLL", iMatrixGetNumLinNoScrollAttrib, iMatrixSetNumLinNoScrollAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMCOL_NOSCROLL", iMatrixGetNumColNoScrollAttrib, iMatrixSetNumColNoScrollAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NOSCROLLASTITLE", iMatrixGetNoScrollAsTitleAttrib, iMatrixSetNoScrollAsTitleAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMLIN_VISIBLE", iMatrixGetNumLinVisibleAttrib, NULL, IUPAF_SAMEASSYSTEM, "3", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMCOL_VISIBLE", iMatrixGetNumColVisibleAttrib, NULL, IUPAF_SAMEASSYSTEM, "4", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMLIN_VISIBLE_LAST", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMCOL_VISIBLE_LAST", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WIDTHDEF", NULL, NULL, IUPAF_SAMEASSYSTEM, "80", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HEIGHTDEF", NULL, NULL, IUPAF_SAMEASSYSTEM, "8", IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "WIDTH", iMatrixGetWidthAttrib, iMatrixSetSizeAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "HEIGHT", iMatrixGetHeightAttrib, iMatrixSetSizeAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "RASTERWIDTH", iMatrixGetRasterWidthAttrib, iMatrixSetSizeAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "RASTERHEIGHT", iMatrixGetRasterHeightAttrib, iMatrixSetSizeAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FITTOSIZE", NULL, iMatrixSetFitToSizeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FITTOTEXT", NULL, iMatrixSetFitToTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FITMAXHEIGHT", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FITMAXWIDTH", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYLIN", NULL, iMatrixSetCopyLinAttrib, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYCOL", NULL, iMatrixSetCopyColAttrib, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVELIN", NULL, iMatrixSetMoveLinAttrib, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVECOL", NULL, iMatrixSetMoveColAttrib, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MINCOLWIDTH", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINCOLWIDTHDEF", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* IupMatrix Attributes - MARK */
  iupClassRegisterAttribute(ic, "MARKED", iupMatrixGetMarkedAttrib, iupMatrixSetMarkedAttrib, NULL, NULL, IUPAF_NO_INHERIT);  /* noticed that for MARKED the matrix must be mapped */
  iupClassRegisterAttributeId2(ic, "MARK", iupMatrixGetMarkAttrib, iupMatrixSetMarkAttrib, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);  /* noticed that for MARK the matrix must be mapped */
  /*OLD*/iupClassRegisterAttribute(ic, "MARK_MODE", iMatrixGetMarkModeAttrib, iMatrixSetMarkModeAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKMODE", iMatrixGetMarkModeAttrib, iMatrixSetMarkModeAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AREA", iMatrixGetMarkAreaAttrib, iMatrixSetMarkAreaAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKAREA", iMatrixGetMarkAreaAttrib, iMatrixSetMarkAreaAttrib, IUPAF_SAMEASSYSTEM, "CONTINUOUS", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTIPLE", iMatrixGetMarkMultipleAttrib, iMatrixSetMarkMultipleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKMULTIPLE", iMatrixGetMarkMultipleAttrib, iMatrixSetMarkMultipleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKATTITLE", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HLCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTHLCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HLCOLORALPHA", NULL, NULL, IUPAF_SAMEASSYSTEM, "128", IUPAF_NO_INHERIT);

  /* IupMatrix Attributes - ACTION (only mapped) */
  iupClassRegisterAttribute(ic, "ADDLIN", NULL, iupMatrixSetAddLinAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);  /* allowing these methods to be called before map will avoid its storage in the hash table */
  iupClassRegisterAttribute(ic, "DELLIN", NULL, iupMatrixSetDelLinAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDCOL", NULL, iupMatrixSetAddColAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DELCOL", NULL, iupMatrixSetDelColAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIGIN", iMatrixGetOriginAttrib, iMatrixSetOriginAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIGINOFFSET", iMatrixGetOriginOffsetAttrib, NULL, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOW", NULL, iMatrixSetShowAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  /*OLD*/iupClassRegisterAttribute(ic, "EDIT_MODE", iMatrixGetEditModeAttrib, iMatrixSetEditModeAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EDITMODE", iMatrixGetEditModeAttrib, iMatrixSetEditModeAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EDITING", iMatrixGetEditingAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EDITNEXT", iMatrixGetEditNextAttrib, iMatrixSetEditNextAttrib, IUPAF_SAMEASSYSTEM, "LIN", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REDRAW", NULL, iupMatrixDrawSetRedrawAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CLEARVALUE", NULL, iMatrixSetClearValueAttrib, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "CLEARATTRIB", NULL, iMatrixSetClearAttribAttrib, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  /* IupMatrix Attributes - EDITION */
  iupClassRegisterAttribute(ic, "CARET", iMatrixGetCaretAttrib, iMatrixSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, iMatrixSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", iMatrixGetSelectionAttrib, iMatrixSetSelectionAttrib, NULL, NULL, IUPAF_NO_SAVE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTILINE", iMatrixGetMultilineAttrib, iMatrixSetMultilineAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MASK", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MASKINT", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MASKFLOAT", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MASKNOEMPTY", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MASKCASEI", NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EDITHIDEONFOCUS", iMatrixGetEditHideOnFocusAttrib, iMatrixSetEditHideOnFocusAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EDITCELL", iMatrixGetEditCellAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EDITTEXT", iMatrixGetEditTextAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EDITVALUE", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupMatrix Attributes - GENERAL */
  iupClassRegisterAttribute(ic, "USETITLESIZE", iMatrixGetUseTitleSizeAttrib, iMatrixSetUseTitleSizeAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LIMITEXPAND", iMatrixGetLimitExpandAttrib, iMatrixSetLimitExpandAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDDENTEXTMARKS", iMatrixGetHiddenTextMarksAttrib, iMatrixSetHiddenTextMarksAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "READONLY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZEMATRIX", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZEMATRIXCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "102 102 102", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZEDRAG", iMatrixGetResizeDragAttrib, iMatrixSetResizeDragAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDEFOCUS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWFILLVALUE", iMatrixGetShowFillValueAttrib, iMatrixSetShowFillValueAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TYPECOLORINACTIVE", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);

  /* Flat Scrollbar */
  iupFlatScrollBarRegister(ic);

  iupClassRegisterAttribute(ic, "FLATSCROLLBAR", NULL, iMatrixSetFlatScrollbarAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupMatrixRegisterEx(ic);

  if (!IupGetHandle("IupMatrixCrossCursor"))
    iMatrixCreateCursor();

  /* Change the Canvas flags */
  iupClassRegisterReplaceAttribFlags(ic, "DX", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribFlags(ic, "DY", IUPAF_NO_SAVE | IUPAF_NO_INHERIT);

  return ic;
}

/*****************************************************************************************************/

Ihandle* IupMatrix(const char* action)
{
  void *params[2];
  params[0] = (void*)action;
  params[1] = NULL;
  return IupCreatev("matrix", params);
}

