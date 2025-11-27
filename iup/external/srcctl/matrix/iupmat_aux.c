/** \file
 * \brief iupmatrix control
 * auxiliary functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"


#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_drv.h"

#include "iupmat_def.h"
#include "iupmat_aux.h"
#include "iupmat_getset.h"
#include "iupmat_numlc.h"


int iupMatrixAuxIsFullVisibleLast(ImatLinColData *p)
{
  int i, sum = 0;

  for(i = p->first; i <= p->last; i++)
  {
    sum += p->dt[i].size;
    if (i==p->first)
      sum -= p->first_offset;
  }

  if (sum > p->current_visible_size)
    return 0;
  else
    return 1;
}

int iupMatrixAuxIsCellStartVisible(Ihandle* ih, int lin, int col)
{
  if (iupMatrixAuxIsCellVisible(ih, lin, col))
  {
    if (col == ih->data->columns.first && ih->data->columns.first_offset!=0)
      return 0;
    if (lin == ih->data->lines.first && ih->data->lines.first_offset!=0)
      return 0;
    if (col == ih->data->columns.last && !iupMatrixAuxIsFullVisibleLast(&ih->data->columns))
      return 0;
    if (lin == ih->data->lines.last && !iupMatrixAuxIsFullVisibleLast(&ih->data->lines))
      return 0;

    return 1;
  }

  return 0;
}

int iupMatrixAuxIsCellVisible(Ihandle* ih, int lin, int col)
{
  if ((col < ih->data->columns.num_noscroll)  &&
      (lin >= ih->data->lines.first) && (lin <= ih->data->lines.last))
    return 1;

  if ((lin < ih->data->lines.num_noscroll) &&
      (col >= ih->data->columns.first) && (col <= ih->data->columns.last))
    return 1;

  if ((lin >= ih->data->lines.first) && (lin <= ih->data->lines.last) &&
      (col >= ih->data->columns.first) && (col <= ih->data->columns.last))
  {
    return 1;
  }

  return 0;
}

void iupMatrixAuxAdjustFirstFromLast(ImatLinColData* p)
{
  int i, sum = 0;

  /* adjust "first" according to "last" */

  i = p->last;
  sum = p->dt[i].size;
  while (i>p->num_noscroll && sum < p->current_visible_size)
  {
    i--;
    sum += p->dt[i].size;
  }

  if (i == p->num_noscroll && sum < p->current_visible_size)
  {
    /* if there are room for everyone then position at start */
    p->first = p->num_noscroll;
    p->first_offset = 0;
  }
  else
  {
    /* the "while" found an index for first */
    p->first = i;

    /* position at the remaing space */
    p->first_offset = sum - p->current_visible_size;
  }
}

void iupMatrixAuxAdjustFirstFromScrollPos(ImatLinColData* p, int scroll_pos)
{
  int index, sp, offset = 0;

  sp = 0;
  for(index = p->num_noscroll; index < p->num; index++)
  {
    sp += p->dt[index].size;
    if (sp > scroll_pos)
    {
      sp -= p->dt[index].size; /* get the previous value */
      offset = scroll_pos - sp;
      break;
    }
  }

  if (index == p->num)
  {
    if (p->num == p->num_noscroll)
    {
      /* did NOT go trough the "for" above */
      offset = scroll_pos;
      index = p->num_noscroll; /* redundant, just for the record */
    }
    else
    {
      /* go all the way trough the "for" above, but still sp < scroll_pos */
      offset = scroll_pos - sp;
      index = p->num-1;
    }
  }

  p->first = index;
  p->first_offset = offset;
}

/* Calculate the size, in pixels, of the invisible columns/lines,
   the left/above of the first column/line.
   In fact the start position of the visible area.
   Depends on the first visible column/line.
   -> m : choose will operate on lines or columns [IMAT_PROCESS_LIN|IMAT_PROCESS_COL]
*/
void iupMatrixAuxUpdateScrollPos(Ihandle* ih, int m)
{
  int i, sb, SB, scroll_pos;
  char* POS;
  ImatLinColData *p;

  sb = iupMatrixGetScrollbar(ih);

  if (m == IMAT_PROCESS_LIN)
  {
    p = &(ih->data->lines);
    SB = IUP_SB_VERT;
    POS = "POSY";
  }
  else
  {
    p = &(ih->data->columns);
    SB = IUP_SB_HORIZ;
    POS = "POSX";
  }

  /* "first" was changed, so update "last" and the scroll pos */

  if (p->total_visible_size <= p->current_visible_size)
  {
    /* the matrix is fully visible */
    p->first = p->num_noscroll;
    p->first_offset = 0;
    p->last = p->num==p->num_noscroll? p->num_noscroll: p->num-1;

    if (sb & SB)
      IupSetAttribute(ih, POS, "0");

    return;
  }

  /* must check if it is a valid position */
  scroll_pos = 0;
  for(i = p->num_noscroll; i < p->first; i++)
    scroll_pos += p->dt[i].size;
  scroll_pos += p->first_offset;

  if (scroll_pos + p->current_visible_size > p->total_visible_size)
  {
    /* invalid condition, must recalculate so it is valid */
    scroll_pos = p->total_visible_size - p->current_visible_size;

    /* position first and first_offset, according to scroll pos */
    iupMatrixAuxAdjustFirstFromScrollPos(p, scroll_pos);
  }

  /* update last */
  iupMatrixAuxUpdateLast(p);

  /* update scroll pos */
  if (sb & SB)
    IupSetInt(ih, POS, scroll_pos);
}

/* Calculate which is the last visible column/line of the matrix.
   Depends on the first visible column/line.  */
void iupMatrixAuxUpdateLast(ImatLinColData *p)
{
  if (p->current_visible_size > 0)
  {
    int i, sum = 0;

    /* Find which is the last column/line.
       Start in the first visible and continue adding the widths
       up to the visible size */
    for(i = p->first; i < p->num; i++)
    {
      sum += p->dt[i].size;
      if (i==p->first)
        sum -= p->first_offset;

      if(sum >= p->current_visible_size)
        break;
    }

    if (i == p->num)
    {
      if (p->num == p->num_noscroll)
        p->last = p->num_noscroll;
      else
        p->last = p->num-1;
    }
    else
      p->last = i;
  }
  else
  {
    /* There is no space for any column, set the last column as 0 */
    p->last = 0;
  }
}

/* Fill the sizes array with the width/heigh of all the columns/lines.
   Calculate the value of total_visible_size */
static void iMatrixAuxFillSizeVec(Ihandle* ih, int m)
{
  int i;
  ImatLinColData *p;

  if (m == IMAT_PROCESS_LIN)
    p = &(ih->data->lines);
  else
    p = &(ih->data->columns);

  /* Calculate total width/height of the matrix and the width/height of each column */
  p->total_visible_size = 0;
  p->total_size = 0;
  for (i = 0; i < p->num; i++)
  {
    if (m == IMAT_PROCESS_LIN)
      p->dt[i].size = iupMatrixGetLineHeight(ih, i, 1);
    else
      p->dt[i].size = iupMatrixGetColumnWidth(ih, i, 1);

    if (i >= p->num_noscroll)
      p->total_visible_size += p->dt[i].size;

    p->total_size += p->dt[i].size;
  }
}

static int iMatrixAuxUpdateVisibleSize(Ihandle* ih, int m)
{
  char *D, *AUTOHIDE, *MAX;
  ImatLinColData *p;
  int canvas_size, fixed_size, i, SB;

  if (m == IMAT_PROCESS_LIN)
  {
    D = "DY";
    MAX = "YMAX";

    /* when configuring the vertical scrollbar check if horizontal scrollbar can be hidden */
    AUTOHIDE = "XAUTOHIDE";
    SB = IUP_SB_HORIZ;

    p = &(ih->data->lines);
    canvas_size = iupMatrixGetHeight(ih);
  }
  else
  {
    D = "DX";
    MAX = "XMAX";

    /* when configuring the horizontal scrollbar check if vertical scrollbar can be hidden */
    AUTOHIDE = "YAUTOHIDE";
    SB = IUP_SB_VERT;

    p = &(ih->data->columns);
    canvas_size = iupMatrixGetWidth(ih);
  }

  /* Subtract scrollbar size if perpendicular scrollbar is visible and overlapping */
  if (m == IMAT_PROCESS_LIN)
  {
    /* Processing vertical (LINES): subtract horizontal scrollbar if visible (overlaps at bottom) */
    if (iupMatrixGetScrollbar(ih) & IUP_SB_HORIZ)
    {
      int sb_size = iupMatrixGetScrollbarSize(ih);
      canvas_size -= sb_size;
    }
  }
  else
  {
    /* Processing horizontal (COLUMNS): subtract vertical scrollbar if visible (overlaps at right) */
    if (iupMatrixGetScrollbar(ih) & IUP_SB_VERT)
    {
      int sb_size = iupMatrixGetScrollbarSize(ih);
      canvas_size -= sb_size;
    }
  }

  fixed_size = 0;
  for (i=0; i<p->num_noscroll; i++)
    fixed_size += p->dt[i].size;

  /* Matrix useful area is the current size minus the non scrollable area */
  p->current_visible_size = canvas_size - fixed_size;
  if (p->current_visible_size > p->total_visible_size)
    p->current_visible_size = p->total_visible_size;

  if (!p->total_visible_size || p->current_visible_size == p->total_visible_size)
  {
    IupSetAttribute(ih, MAX, "0");
    IupSetAttribute(ih, D, "0");  /* this can generate resize+redraw events */
  }
  else
  {
    int sb = iupMatrixGetScrollbar(ih);
    if (ih->data->limit_expand && (sb & SB) && iupAttribGetBoolean(ih, AUTOHIDE))
    {
      /* Must perform an extra check or the scrollbar will be always visible */
      int sb_size = iupMatrixGetScrollbarSize(ih);
      if (p->current_visible_size + sb_size == p->total_visible_size)
        p->current_visible_size = p->total_visible_size;
    }

    IupSetInt(ih, MAX, p->total_visible_size);
    IupSetInt(ih, D, p->current_visible_size);
  }

  return IupGetInt(ih, "SB_RESIZE");
}

int iupMatrixAuxCalcSizes(Ihandle* ih)
{
  int sb_resize_col, sb_resize_lin;
  ih->data->need_calcsize = 0;  /* do it before UpdateVisibleSize */

  iMatrixAuxFillSizeVec(ih, IMAT_PROCESS_COL);
  iMatrixAuxFillSizeVec(ih, IMAT_PROCESS_LIN);

  /* Prevent SCROLL_CB from firing when setting DX/DY programmatically.
     Setting DX/DY changes native scrollbar which triggers SCROLL_CB, creating a cascade of ACTION callbacks. */
  ih->data->inside_scroll_update = 1;

  /* this could change the size of the drawing area,
     and trigger a resize event, then another calcsize. */
  sb_resize_col = iMatrixAuxUpdateVisibleSize(ih, IMAT_PROCESS_COL);
  sb_resize_lin = iMatrixAuxUpdateVisibleSize(ih, IMAT_PROCESS_LIN);

  /* when removing lines the first can be positioned after the last line */
  if (ih->data->lines.first > ih->data->lines.num-1)
  {
    ih->data->lines.first_offset = 0;
    if (ih->data->lines.num == ih->data->lines.num_noscroll)
      ih->data->lines.first = ih->data->lines.num_noscroll;
    else
      ih->data->lines.first = ih->data->lines.num-1;
  }
  if (ih->data->columns.first > ih->data->columns.num-1)
  {
    ih->data->columns.first_offset = 0;
    if (ih->data->columns.num == ih->data->columns.num_noscroll)
      ih->data->columns.first = ih->data->columns.num_noscroll;
    else
      ih->data->columns.first = ih->data->columns.num-1;
  }

  /* make sure scroll pos is consistent - also sets POSX/POSY so keep inside_scroll_update=1 */
  iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_COL);
  iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_LIN);

  ih->data->inside_scroll_update = 0;

  return sb_resize_col || sb_resize_lin;
}

int iupMatrixAuxCallLeaveCellCb(Ihandle* ih)
{
  if (ih->data->columns.num > 1 && ih->data->lines.num > 1)
  {
    IFnii cb = (IFnii)IupGetCallback(ih, "LEAVEITEM_CB");
    if(cb)
      return cb(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell);
  }
  return IUP_DEFAULT;
}

void iupMatrixAuxCallEnterCellCb(Ihandle* ih)
{
  if (ih->data->columns.num > 1 && ih->data->lines.num > 1)
  {
    IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (cb)
      cb(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell);
  }
}

int iupMatrixAuxCallEditionCbLinCol(Ihandle* ih, int lin, int col, int mode, int update)
{
  IFniiii cb;

  if (iupAttribGetBoolean(ih, "READONLY"))
    return IUP_IGNORE;

  cb = (IFniiii)IupGetCallback(ih, "EDITION_CB");
  if (cb)
  {
    if (lin != 0 && ih->data->sort_has_index)
      lin = ih->data->sort_line_index[lin];

    return cb(ih, lin, col, mode, update);
  }
  return IUP_DEFAULT;
}

static void iMatrixAuxCopyValue(Ihandle* ih, int lin1, int col1, int lin2, int col2)
{
  char* value = iupMatrixGetValue(ih, lin1, col1);
  iupMatrixModifyValue(ih, lin2, col2, value);
}

void iupMatrixAuxCopyLin(Ihandle* ih, int from_lin, int to_lin)
{
  int col, columns_num = ih->data->columns.num;

  /* since we can not undo the attribute copy, disable data undo */
  int old_undo = ih->data->undo_redo;
  ih->data->undo_redo = 0;

  for(col = 0; col < columns_num; col++)
    iMatrixAuxCopyValue(ih, from_lin, col, to_lin, col);

  ih->data->undo_redo = old_undo;

  iupBaseCallValueChangedCb(ih);

  iupMatrixCopyLinAttributes(ih, from_lin, to_lin);
}

void iupMatrixAuxCopyCol(Ihandle* ih, int from_col, int to_col)
{
  int lin, lines_num = ih->data->lines.num;

  /* since we can not undo the attribute copy, disable data undo */
  int old_undo = ih->data->undo_redo;
  ih->data->undo_redo = 0;

  for(lin = 0; lin < lines_num; lin++)
    iMatrixAuxCopyValue(ih, lin, from_col, lin, to_col);

  ih->data->undo_redo = old_undo;

  iupBaseCallValueChangedCb(ih);

  iupMatrixCopyColAttributes(ih, from_col, to_col);
}
