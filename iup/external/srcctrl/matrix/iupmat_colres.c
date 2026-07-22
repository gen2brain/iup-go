/** \file
 * \brief iupmatrix column resize and reorder
 *
 * See Copyright Notice in "iup.h"
 */

/*******************************************************************/
/* Interactive Column Resize Functions and WIDTH change            */
/*******************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"


#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"

#include "iupmat_def.h"
#include "iupmat_colres.h"
#include "iupmat_draw.h"
#include "iupmat_aux.h"
#include "iupmat_edit.h"
#include "iupmat_getset.h"

#define IMAT_COLRES_TOL       3

static int iMatrixGetColResCheck(Ihandle* ih, int x, int y)
{
  if (ih->data->lines.dt[0].size &&
      y < ih->data->lines.dt[0].size &&
      iupAttribGetBoolean(ih, "RESIZEMATRIX"))
  {
    int x_col = 0, col;

    /* Check if it is in the non-scrollable columns */
    for(col = 0; col < ih->data->columns.num_noscroll; col++)
    {
      x_col += ih->data->columns.dt[col].size;
      if (abs(x_col-x) < IMAT_COLRES_TOL)
        return col;
    }

    /* Check if it is in the visible columns */
    for(col = ih->data->columns.first; col <= ih->data->columns.last; col++)
    {
      x_col += ih->data->columns.dt[col].size;
      if (col == ih->data->columns.first)
        x_col -= ih->data->columns.first_offset;

      if (abs(x_col-x) < IMAT_COLRES_TOL)
        return col;
    }
  }

  return -1;
}

/* Verify if the mouse is in the intersection between two of column titles,
   if so the resize is started */
int iupMatrixColResStart(Ihandle* ih, int x, int y)
{
  int col = iMatrixGetColResCheck(ih, x, y);
  if (col != -1)
  {
    const char* color_str = iupAttribGetStr(ih, "RESIZEMATRIXCOLOR");
    unsigned char r, g, b;

    /* Parse color string to RGB and encode as long for compatibility */
    iupStrToRGB(color_str, &r, &g, &b);
    ih->data->colres_color = ((long)r << 16) | ((long)g << 8) | (long)b;

    ih->data->colres_drag_col_start_x = x;
    ih->data->colres_dragging =  1;
    ih->data->colres_drag_col = col;
    return 1;
  }
  else
    return 0;
}

void iupMatrixColResFinish(Ihandle* ih, int x)
{
  int min_width = iupAttribGetIntId(ih, "MINCOLWIDTH", ih->data->colres_drag_col);
  int delta = x - ih->data->colres_drag_col_start_x;
  int width = ih->data->columns.dt[ih->data->colres_drag_col].size + delta;

  if (min_width == 0)
    min_width = iupAttribGetInt(ih, "MINCOLWIDTHDEF");

  width -= IMAT_PADDING_W + IMAT_FRAME_W;

  if (width < min_width)
    width = min_width;

  ih->data->colres_dragging = 0;
  ih->data->colres_feedback = 0;

  iupAttribSetIntId(ih, "RASTERWIDTH", ih->data->colres_drag_col, width);
  iupAttribSetId(ih, "WIDTH", ih->data->colres_drag_col, NULL);

  ih->data->need_calcsize = 1;

  if (!ih->data->edit_hide_onfocus && ih->data->editing)
  {
    iupMatrixAuxCalcSizes(ih);
    iupMatrixEditUpdatePos(ih);
  }

  iupMatrixDraw(ih, 0);

  {
    IFni cb = (IFni)IupGetCallback(ih, "COLRESIZE_CB");
    if (cb)
      cb(ih, ih->data->colres_drag_col);
  }
}

/* Change the column width interactively, just change the line in the screen.
   When the user finishes the drag, the iupMatrixColResFinish function is called
   to truly change the column width. */
void iupMatrixColResMove(Ihandle* ih, int x)
{
  int y1, y2;

  int delta = x - ih->data->colres_drag_col_start_x;
  int width = ih->data->columns.dt[ih->data->colres_drag_col].size + delta;
  if (width < 0)
    return;

  y1 = ih->data->lines.dt[0].size;  /* from the bottom of the line of titles */
  y2 = iupMatrixGetHeight(ih) - 1;             /* to the bottom of the matrix */

  if (ih->data->colres_drag)
  {
    iupMatrixColResFinish(ih, x);

    iupMatrixDrawUpdate(ih);

    ih->data->colres_dragging = 1; /* keep dragging */
    ih->data->colres_drag_col_start_x = x;
  }
  else
  {
    ih->data->colres_feedback = 1;
    ih->data->colres_x = x;
    /* IupDraw uses top-down coordinates natively, no Y inversion needed */
    ih->data->colres_y1 = y1;
    ih->data->colres_y2 = y2;

    iupMatrixDrawUpdate(ih);
  }
}

/* Change the cursor when it passes over a group of the column titles. */
int iupMatrixColResCheckChangeCursor(Ihandle* ih, int x, int y)
{
  int col = iMatrixGetColResCheck(ih, x, y);
  if (col != -1)
    return 1;
  else /* It is in the empty area after the last column, or inside a cell */
    return 0;
}

int iupMatrixColResIsResizing(Ihandle* ih)
{
  return ih->data->colres_dragging;
}


/*******************************************************************/
/* Interactive Column Reorder Functions                            */
/*******************************************************************/

#define IMAT_COLMOVE_TOL 4

static int iMatrixColMoveTargetCol(Ihandle* ih, int x)
{
  int lin, col;
  int ytitle = ih->data->lines.dt[0].size / 2;

  if (iupMatrixGetCellFromXY(ih, x, ytitle, &lin, &col) && col >= 1)
    return col;

  if (x < ih->data->columns.dt[0].size)
    return (ih->data->columns.num_noscroll > 1) ? ih->data->columns.num_noscroll : 1;

  return ih->data->columns.num - 1;
}

int iupMatrixColMoveStart(Ihandle* ih, int x, int y)
{
  int lin, col;

  if (!iupAttribGetBoolean(ih, "ALLOWREORDER"))
    return 0;

  if (ih->data->lines.dt[0].size == 0 || y >= ih->data->lines.dt[0].size)
    return 0;

  if (!iupMatrixGetCellFromXY(ih, x, y, &lin, &col) || lin != 0 || col < 1)
    return 0;

  ih->data->colmove_dragging = 1;
  ih->data->colmove_feedback = 0;
  ih->data->colmove_src_col = col;
  ih->data->colmove_dst_col = col;
  ih->data->colmove_start_x = x;
  return 1;
}

void iupMatrixColMoveMove(Ihandle* ih, int x)
{
  int col, cx, cy, cw, ch;

  if (!ih->data->colmove_feedback && abs(x - ih->data->colmove_start_x) < IMAT_COLMOVE_TOL)
    return;

  col = iMatrixColMoveTargetCol(ih, x);
  ih->data->colmove_dst_col = col;

  if (iupMatrixGetVisibleCellDim(ih, 0, col, &cx, &cy, &cw, &ch))
  {
    ih->data->colmove_feedback = 1;
    ih->data->colmove_x = (ih->data->colmove_src_col < col) ? cx + cw : cx;
    ih->data->colmove_y1 = cy;
    ih->data->colmove_y2 = cy + ch;
    iupMatrixDrawUpdate(ih);
  }
}

int iupMatrixColMoveFinish(Ihandle* ih)
{
  int src = ih->data->colmove_src_col;
  int dst = ih->data->colmove_dst_col;
  int moved = ih->data->colmove_feedback;

  ih->data->colmove_dragging = 0;
  ih->data->colmove_feedback = 0;

  if (!moved)
    return 0;

  if (src != dst && dst >= 1)
  {
    int ret = IUP_DEFAULT;
    IFnii cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
    if (cb)
      ret = cb(ih, src, dst);

    if (ret != IUP_IGNORE && !ih->data->callback_mode)
      IupSetIntId(ih, "MOVECOL", src, dst);
  }

  iupMatrixDraw(ih, 1);
  return 1;
}

int iupMatrixColMoveIsMoving(Ihandle* ih)
{
  return ih->data->colmove_dragging;
}

