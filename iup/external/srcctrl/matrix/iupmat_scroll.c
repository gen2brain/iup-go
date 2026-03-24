/** \file
 * \brief iupmatrix control
 * scrolling
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_stdcontrols.h"

#include "iupmat_def.h"
#include "iupmat_scroll.h"
#include "iupmat_aux.h"
#include "iupmat_edit.h"
#include "iupmat_draw.h"


/* Macros used by "dir" parameter of the iMatrixScrollLine and iMatrixScrollColumn functions */
#define IMAT_SCROLL_LEFT    0
#define IMAT_SCROLL_RIGHT   1
#define IMAT_SCROLL_UP      2
#define IMAT_SCROLL_DOWN    3

/**************************************************************************/
/* Private functions                                                      */
/**************************************************************************/

static void iMatrixScrollToVisible(ImatLinColData* p, int index)
{
  /* The work here is just to position first and first_offset, 
     so "index" is between "first" and "last". */
     
  /* It is called only for discrete scrolling, 
     so first_offset usually will be set to 0. */

  /* already visible, change nothing */
  if (index > p->first && index < p->last)
    return;
  if (index < p->num_noscroll)
    return;

  /* scroll to visible, means position the cell so the start at left is visible */

  if (index <= p->first)
  {
    p->first = index;
    p->first_offset = 0;
    return;
  }
  else /* (index >= p->last) */
  {
    p->last = index;

    /* adjust "first" according to "last" */
    iupMatrixAuxAdjustFirstFromLast(p);
  }
}

/* Callback to report to the user which visualization area of
   the matrix changed. */
static void iMatrixScrollCallScrollTopCb(Ihandle* ih)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "SCROLLTOP_CB");
  if (cb) 
    cb(ih, ih->data->lines.first, ih->data->columns.first);
}

static int iMatrixScrollGetNextNonEmpty(Ihandle* ih, int m, int index, int scrollkey)
{
  ImatLinColData* p;

  if (m == IMAT_PROCESS_LIN)
    p = &(ih->data->lines);
  else
    p = &(ih->data->columns);

  /* get the next non-empty cell */
  while(index < p->num && p->dt[index].size == 0)
    index++;

  if (index > p->num-1)
  {
    int noscroll = p->num_noscroll;
    if (scrollkey)
      noscroll = 1;

    if (p->num == noscroll)
      return noscroll;
    else
      return p->num-1;
  }
  else
    return index;
}

static int iMatrixScrollGetPrevNonEmpty(Ihandle* ih, int m, int index, int scrollkey)
{
  ImatLinColData* p;
  int noscroll;

  if (m == IMAT_PROCESS_LIN)
    p = &(ih->data->lines);
  else
    p = &(ih->data->columns);

  /* get the previous non-empty cell */
  while (index > 0 && p->dt[index].size == 0)
    index--;

  noscroll = p->num_noscroll;
  if (scrollkey)
    noscroll = 1;

  if (index < noscroll)
    return noscroll;
  else
    return index;
}

static void iMatrixScrollSetFocusScrollToVisible(Ihandle* ih, int lin, int col)
{
  /* moving focus and eventually scrolling */
  ih->data->lines.focus_cell = lin;
  ih->data->columns.focus_cell = col;

  /* set for both because focus maybe hidden */
  iMatrixScrollToVisible(&ih->data->columns, ih->data->columns.focus_cell);
  iMatrixScrollToVisible(&ih->data->lines, ih->data->lines.focus_cell);
}

static void iMatrixScrollSetFocusScrollToVisibleLinCol(Ihandle* ih, int m, int index)
{
  if (m == IMAT_PROCESS_COL)
    iMatrixScrollSetFocusScrollToVisible(ih, ih->data->lines.focus_cell, index);
  else
    iMatrixScrollSetFocusScrollToVisible(ih, index, ih->data->columns.focus_cell);
}

/**************************************************************************/
/*                      Exported functions                                */
/**************************************************************************/

void iupMatrixScrollToVisible(Ihandle* ih, int lin, int col)
{
  int old_lines_first = ih->data->lines.first;
  int old_columns_first = ih->data->columns.first;
  int old_lines_first_offset = ih->data->lines.first_offset;
  int old_columns_first_offset = ih->data->columns.first_offset;

  iMatrixScrollToVisible(&ih->data->columns, col);
  iMatrixScrollToVisible(&ih->data->lines, lin);

  if ((ih->data->lines.first != old_lines_first || ih->data->lines.first_offset != old_lines_first_offset) ||
      (ih->data->columns.first != old_columns_first || ih->data->columns.first_offset != old_columns_first_offset))
  {
    /* when "first" is changed must update scroll pos */
    if (ih->data->columns.first != old_columns_first || ih->data->columns.first_offset != old_columns_first_offset)
      iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_COL);

    if (ih->data->lines.first != old_lines_first || ih->data->lines.first_offset != old_lines_first_offset)
      iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_LIN);

    iMatrixScrollCallScrollTopCb(ih);

    if (!ih->data->edit_hide_onfocus && ih->data->editing)
        iupMatrixEditUpdatePos(ih);

    iupMatrixDraw(ih, 1);
  }
}

void iupMatrixScrollMove(iupMatrixScrollMoveFunc func, Ihandle* ih, int mode, int m)
{
  int old_lines_first = ih->data->lines.first;
  int old_columns_first = ih->data->columns.first;
  int old_lines_first_offset = ih->data->lines.first_offset;
  int old_columns_first_offset = ih->data->columns.first_offset;

  if (ih->data->edit_hide_onfocus)
  {
    ih->data->edit_hidden_byfocus = 1;
    iupMatrixEditHide(ih);
    ih->data->edit_hidden_byfocus = 0;
  }

  func(ih, mode, m);

  if (ih->data->lines.first != old_lines_first || ih->data->lines.first_offset != old_lines_first_offset ||
      ih->data->columns.first != old_columns_first || ih->data->columns.first_offset != old_columns_first_offset)
  {
    /* when "first" is changed must update scroll pos */
    if (ih->data->columns.first != old_columns_first || ih->data->columns.first_offset != old_columns_first_offset)
      iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_COL);

    if (ih->data->lines.first != old_lines_first || ih->data->lines.first_offset != old_lines_first_offset)
      iupMatrixAuxUpdateScrollPos(ih, IMAT_PROCESS_LIN);

    iMatrixScrollCallScrollTopCb(ih);

    if (!ih->data->edit_hide_onfocus && ih->data->editing)
        iupMatrixEditUpdatePos(ih);

    iupMatrixDraw(ih, 0);
  }
}

/************************************************************************************/

/* This function is called when the "home" key is pressed.
   In the first time, go to the beginning of the line.
   In the second time, go to the beginning of the page.
   In the third time, go to the beginning of the matrix.
   -> mode and pos : NOT USED.
*/
void iupMatrixScrollHomeFunc(Ihandle* ih, int unused_mode, int unused_m)
{
  (void)unused_m;
  (void)unused_mode;

  /* called only for mode==IMAT_SCROLLKEY */
  /* moving focus and eventually scrolling */

  if(ih->data->homekeycount == 0)  /* go to the beginning of the line */
  {
    int col = iMatrixScrollGetNextNonEmpty(ih, IMAT_PROCESS_COL, 1, 1);
    iMatrixScrollSetFocusScrollToVisibleLinCol(ih, IMAT_PROCESS_COL, col);
  }
  else if(ih->data->homekeycount == 1)   /* go to the beginning of the visible page */
  {
    iMatrixScrollSetFocusScrollToVisible(ih, ih->data->lines.first, ih->data->columns.first);
  }
  else if(ih->data->homekeycount == 2)   /* go to the beginning of the matrix 1:1 */
  {
    int lin = iMatrixScrollGetNextNonEmpty(ih, IMAT_PROCESS_LIN, 1, 1);
    int col = iMatrixScrollGetNextNonEmpty(ih, IMAT_PROCESS_COL, 1, 1);
    iMatrixScrollSetFocusScrollToVisible(ih, lin, col);
  }
}

/* This function is called when the "end" key is pressed.
   In the first time, go to the end of the line.
   In the second time, go to the end of the page.
   In the third time, go to the end of the matrix.
   -> mode and pos : NOT USED.
*/
void iupMatrixScrollEndFunc(Ihandle* ih, int unused_mode, int unused_m)
{
  (void)unused_m;
  (void)unused_mode;

  /* called only for mode==IMAT_SCROLLKEY */
  /* moving focus and eventually scrolling */

  if(ih->data->endkeycount == 0)  /* go to the end of the line */
  {
    int col = iMatrixScrollGetPrevNonEmpty(ih, IMAT_PROCESS_COL, ih->data->columns.num - 1, 1);
    iMatrixScrollSetFocusScrollToVisibleLinCol(ih, IMAT_PROCESS_COL, col);
  }
  else if(ih->data->endkeycount == 1)   /* go to the end of the visible page */
  {
    iMatrixScrollSetFocusScrollToVisible(ih, ih->data->lines.last, ih->data->columns.last);
  }
  else if(ih->data->endkeycount == 2)   /* go to the end of the matrix */
  {
    int lin = iMatrixScrollGetPrevNonEmpty(ih, IMAT_PROCESS_LIN, ih->data->lines.num - 1, 1);
    int col = iMatrixScrollGetPrevNonEmpty(ih, IMAT_PROCESS_COL, ih->data->columns.num - 1, 1);
    iMatrixScrollSetFocusScrollToVisible(ih, lin, col);
  }
}

/* This function is called to move a cell to the left or up.
   -> mode : indicate if the command was from the keyboard or the scrollbar. If scrollbar,
             do not change the focus.
   -> pos  : NOT USED
   -> m    : define the mode of operation: lines or columns [IMAT_PROCESS_LIN|IMAT_PROCESS_COL]
*/
void iupMatrixScrollLeftUpFunc(Ihandle* ih, int mode, int m)
{
  ImatLinColData* p;

  if(m == IMAT_PROCESS_LIN)
    p = &(ih->data->lines);
  else
    p = &(ih->data->columns);

  if (mode == IMAT_SCROLLKEY)
  {
    /* moving focus and eventually scrolling */
    int next = iMatrixScrollGetPrevNonEmpty(ih, m, p->focus_cell - 1, 1);
    iMatrixScrollSetFocusScrollToVisibleLinCol(ih, m, next);
  }
  else  /* IMAT_SCROLLBAR */
  {
    /* always scrolling without changing focus */
    p->first = iMatrixScrollGetPrevNonEmpty(ih, m, p->first - 1, 0);
    p->first_offset = 0;
  }
}

/* This function is called to move a cell to the right or down.
   -> mode : indicate if the command from the keyboard or the scrollbar. If scrollbar,
             do not change the focus.
   -> pos  : NOT USED
   -> m    : define the mode of operation: lines or columns [IMAT_PROCESS_LIN|IMAT_PROCESS_COL]
*/
void iupMatrixScrollRightDownFunc(Ihandle* ih, int mode, int m)
{
  ImatLinColData* p;

  if(m == IMAT_PROCESS_LIN)
    p = &(ih->data->lines);
  else
    p = &(ih->data->columns);

  if (mode == IMAT_SCROLLKEY)
  {
    /* moving focus and eventually scrolling */
    int next = iMatrixScrollGetNextNonEmpty(ih, m, p->focus_cell + 1, 1);
    iMatrixScrollSetFocusScrollToVisibleLinCol(ih, m, next);
  }
  else  /* IMAT_SCROLLBAR */
  {
    /* always scrolling without changing focus */
    p->first = iMatrixScrollGetNextNonEmpty(ih, m, p->first + 1, 0);
    p->first_offset = 0;
  }
}

/* This function is called to move a page to the left or up.
   -> mode : indicate if the command was from the keyboard or the scrollbar. If scrollbar,
             do not change the focus.
   -> pos  : NOT USED
   -> m    : define the mode of operation: lines (PgLeft) or columns (PgUp) [IMAT_PROCESS_LIN|IMAT_PROCESS_COL]
*/
void iupMatrixScrollPgLeftUpFunc(Ihandle* ih, int mode, int m)
{
  ImatLinColData* p;

  if(m == IMAT_PROCESS_LIN)
    p = &(ih->data->lines);
  else
    p = &(ih->data->columns);

  if (mode == IMAT_SCROLLKEY)
  {
    /* moving focus and eventually scrolling */
    int next = iMatrixScrollGetPrevNonEmpty(ih, m, p->focus_cell - (p->last - p->first), 1);
    iMatrixScrollSetFocusScrollToVisibleLinCol(ih, m, next);
  }
  else  /* IMAT_SCROLLBAR */
  {
    /* always scrolling without changing focus */
    p->first = iMatrixScrollGetPrevNonEmpty(ih, m, p->first - (p->last - p->first), 0);
    p->first_offset = 0;
  }
}

/* This function is called to move a page to the right or down.
   -> mode : indicate if the command was from the keyboard or the scrollbar. If scrollbar,
             do not change the focus.
   -> pos  : NOT USED
   -> m    : define the mode of operation: lines (PgDown) or columns (PgRight) [IMAT_PROCESS_LIN|IMAT_PROCESS_COL]
*/
void iupMatrixScrollPgRightDownFunc(Ihandle* ih, int mode, int m)
{
  ImatLinColData* p;

  if(m == IMAT_PROCESS_LIN)
    p = &(ih->data->lines);
  else
    p = &(ih->data->columns);

  if (mode == IMAT_SCROLLKEY)
  {
    /* moving focus and eventually scrolling */
    int next = iMatrixScrollGetNextNonEmpty(ih, m, p->focus_cell + (p->last - p->first), 1);
    iMatrixScrollSetFocusScrollToVisibleLinCol(ih, m, next);
  }
  else  /* IMAT_SCROLLBAR */
  {
    /* always scrolling without changing focus */
    p->first = iMatrixScrollGetPrevNonEmpty(ih, m, p->first + (p->last - p->first), 0);
    p->first_offset = 0;
  }
}

void iupMatrixScrollCrFunc(Ihandle* ih, int unused_mode, int unused_m)
{
  int m;
  int oldlin = ih->data->lines.focus_cell;
  int oldcol = ih->data->columns.focus_cell;

  /* m is decided according to editnext */
  (void)unused_m;

  /* called only for mode==IMAT_SCROLLKEY */
  (void)unused_mode;

  if (ih->data->editnext == IMAT_EDITNEXT_NONE)
    return;

  if (ih->data->editnext == IMAT_EDITNEXT_LIN || 
      ih->data->editnext == IMAT_EDITNEXT_LINCR)
    m = IMAT_PROCESS_LIN;
  else
    m = IMAT_PROCESS_COL;

  /* try the normal processing of next cell down (next line/next column) */
  iupMatrixScrollRightDownFunc(ih, IMAT_SCROLLKEY, m);

  if(ih->data->lines.focus_cell == oldlin && ih->data->columns.focus_cell == oldcol)
  {
    /* If focus was not changed, it was because it is in the last line of the column/last column of the line.
       Go to the next column of the same line/next line of the same column. */
    switch (ih->data->editnext)
    {
    case IMAT_EDITNEXT_LIN:
      /* next col, same line (stay at the last line) */
      iupMatrixScrollRightDownFunc(ih, IMAT_SCROLLKEY, IMAT_PROCESS_COL);
      break;
    case IMAT_EDITNEXT_COL: 
      /* next line, same col (stay at the last col) */
      iupMatrixScrollRightDownFunc(ih, IMAT_SCROLLKEY, IMAT_PROCESS_LIN);
      break;
    case IMAT_EDITNEXT_LINCR: 
      /* next col */
      iupMatrixScrollRightDownFunc(ih, IMAT_SCROLLKEY, IMAT_PROCESS_COL);

      /* if successfully changed the col, then go to first line */
      if (ih->data->columns.focus_cell != oldcol)
      {
        int lin = iMatrixScrollGetNextNonEmpty(ih, IMAT_PROCESS_LIN, 1, 1);
        iMatrixScrollSetFocusScrollToVisibleLinCol(ih, IMAT_PROCESS_LIN, lin);
      }
      break;
    case IMAT_EDITNEXT_COLCR: 
      /* next line */
      iupMatrixScrollRightDownFunc(ih, IMAT_SCROLLKEY, IMAT_PROCESS_LIN);

      /* if successfully changed the line, then go to first col */
      if (ih->data->lines.focus_cell != oldlin)
      {
        int col = iMatrixScrollGetNextNonEmpty(ih, IMAT_PROCESS_COL, 1, 1);
        iMatrixScrollSetFocusScrollToVisibleLinCol(ih, IMAT_PROCESS_COL, col);
      }
      break;
    }
  }
}

/* This function is called when a drag is performed in the scrollbar.
   -> x    : scrollbar thumb position, value between 0 and 1
   -> mode : NOT USED
   -> m    : define the mode of operation: lines or columns [IMAT_PROCESS_LIN|IMAT_PROCESS_COL]
*/
void iupMatrixScrollPosFunc(Ihandle* ih, int mode, int m)
{
  int scroll_pos;
  ImatLinColData* p;
  (void)mode;

  if (m == IMAT_PROCESS_LIN)
  {
    p = &(ih->data->lines);
    scroll_pos = IupGetInt(ih, "POSY");
  }
  else
  {
    p = &(ih->data->columns);
    scroll_pos = IupGetInt(ih, "POSX");
  }

  if (p->num == p->num_noscroll)
  {
    p->first = p->num_noscroll;
    p->first_offset = 0;
    return;
  }

  /* position first and first_offset, according to scroll pos */
  iupMatrixAuxAdjustFirstFromScrollPos(p, scroll_pos);
}

/************************************************************************************/

int iupMatrixScroll_CB(Ihandle* ih, int action, float posx, float posy)
{
  (void)posx;
  (void)posy;

  if (!iupMatrixIsValid(ih, 0))
    return IUP_DEFAULT;

  /* Ignore scroll events when DX/DY is being set programmatically during CalcSizes.
     Setting DX/DY changes native scrollbar config which fires SCROLL_CB (IUP_SBPOSH/SBPOSV),
     which would read POSX/POSY and trigger another ACTION callback, creating a cascade. */
  if (ih->data->inside_scroll_update)
    return IUP_DEFAULT;

  switch(action)
  {
    case IUP_SBUP      : iupMATRIX_ScrollUp(ih);       break;
    case IUP_SBDN      : iupMATRIX_ScrollDown(ih);     break;
    case IUP_SBPGUP    : iupMATRIX_ScrollPgUp(ih);     break;
    case IUP_SBPGDN    : iupMATRIX_ScrollPgDown(ih);   break;
    case IUP_SBRIGHT   : iupMATRIX_ScrollRight(ih);    break;
    case IUP_SBLEFT    : iupMATRIX_ScrollLeft(ih);     break;
    case IUP_SBPGRIGHT : iupMATRIX_ScrollPgRight(ih);  break;
    case IUP_SBPGLEFT  : iupMATRIX_ScrollPgLeft(ih);   break;
    case IUP_SBPOSV    : iupMATRIX_ScrollPosVer(ih,posy); break;
    case IUP_SBPOSH    : iupMATRIX_ScrollPosHor(ih,posx); break;
    case IUP_SBDRAGV   : iupMATRIX_ScrollPosVer(ih,posy); break;
    case IUP_SBDRAGH   : iupMATRIX_ScrollPosHor(ih,posx); break;
  }

  iupMatrixDrawUpdate(ih);

  return IUP_DEFAULT;
}
