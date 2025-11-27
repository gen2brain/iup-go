/** \file
 * \brief iupmatrix control
 * cell selection
 *
 * See Copyright Notice in "iup.h"
 */

/**************************************************************************/
/*  Functions to cell selection (mark)                                    */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"


#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"

#include "iupmat_def.h"
#include "iupmat_mark.h"
#include "iupmat_getset.h"
#include "iupmat_draw.h"


static void iMatrixMarkLinSet(Ihandle* ih, int lin, int mark)
{
  /* called when MARKMODE=LIN */
  if (mark==-1)
    mark = !(ih->data->lines.dt[lin].flags & IMAT_IS_MARKED);

  if (mark)
    ih->data->lines.dt[lin].flags |= IMAT_IS_MARKED;
  else
    ih->data->lines.dt[lin].flags &= ~IMAT_IS_MARKED;
}

static void iMatrixMarkColSet(Ihandle* ih, int col, int mark)
{
  /* called when MARKMODE=COL */
  if (mark==-1)
    mark = !(ih->data->columns.dt[col].flags & IMAT_IS_MARKED);

  if (mark)
    ih->data->columns.dt[col].flags |= IMAT_IS_MARKED;
  else
    ih->data->columns.dt[col].flags &= ~IMAT_IS_MARKED;
}

static int iMatrixSetMarkCell(Ihandle* ih, int lin, int col, int mark, IFniii markedit_cb)
{
  if (ih->data->callback_mode)
  {
    if (markedit_cb && !ih->data->inside_markedit_cb) /* allow MARK to be set from inside the callback */
    {
      ih->data->inside_markedit_cb = 1;
      markedit_cb(ih, lin, col, mark);  /* called only here */
      ih->data->inside_markedit_cb = 0;
    }
    else
    {
      if (mark)
      {
        iupAttribSetId2(ih, "MARK", lin, col, "1");
        return 1;
      }
      else
        iupAttribSetId2(ih, "MARK", lin, col, NULL);
    }
  }
  else
  {
    if (mark)
      ih->data->cells[lin][col].flags |= IMAT_IS_MARKED;
    else
      ih->data->cells[lin][col].flags &= ~IMAT_IS_MARKED;
  }

  return 0;
}

static void iMatrixMarkCell(Ihandle* ih, int lin, int col, int mark, IFniii markedit_cb, IFnii mark_cb)
{
  /* called only when MARKMODE=CELL */
  if (mark == -1)
    mark = !iupMatrixGetMark(ih, lin, col, mark_cb);

  iMatrixSetMarkCell(ih, lin, col, mark, markedit_cb);
}

int iupMatrixGetMark(Ihandle* ih, int lin, int col, IFnii mark_cb)
{
  if (ih->data->mark_mode == IMAT_MARK_NO)
    return 0;

  if (ih->data->mark_mode == IMAT_MARK_CELL)
  {
    if (lin == 0 || col == 0)  /* title cell can NOT have a mark */
      return 0;

    if (ih->data->callback_mode)
    {
      if (mark_cb)
        return mark_cb(ih, lin, col);
      else
        return iupAttribGetIntId2(ih, "MARK", lin, col);
    }
    else
      return ih->data->cells[lin][col].flags & IMAT_IS_MARKED;
  }
  else
  {
    if (ih->data->mark_mode & IMAT_MARK_LIN &&
        lin>0 &&
        ih->data->lines.dt[lin].flags & IMAT_IS_MARKED)
        return 1;

    if (ih->data->mark_mode & IMAT_MARK_COL &&
        col>0 &&
        ih->data->columns.dt[col].flags & IMAT_IS_MARKED)
        return 1;

    return 0;
  }
}

static void iMatrixMarkItem(Ihandle* ih, int lin1, int col1, int mark, IFniii markedit_cb, IFnii mark_cb)
{
  if (ih->data->mark_full1 == IMAT_PROCESS_LIN)
  {
    if (ih->data->mark_mode == IMAT_MARK_CELL)
    {
      int col;
      for (col = 1; col < ih->data->columns.num; col++)
        iMatrixMarkCell(ih, lin1, col, mark, markedit_cb, mark_cb);
    }
    else
    {
      iMatrixMarkLinSet(ih, lin1, mark);
      iupMatrixDrawTitleLines(ih, lin1, lin1);
      if (ih->data->columns.num_noscroll>1)
        iupMatrixDrawCells(ih, lin1, 1, lin1, ih->data->columns.num_noscroll-1);
    }

    iupMatrixDrawCells(ih, lin1, 1, lin1, ih->data->columns.num-1);
  }
  else if (ih->data->mark_full1 == IMAT_PROCESS_COL)
  {
    if (ih->data->mark_mode == IMAT_MARK_CELL)
    {
      int lin;
      for(lin = 1; lin < ih->data->lines.num; lin++)
        iMatrixMarkCell(ih, lin, col1, mark, markedit_cb, mark_cb);
    }
    else
    {
      iMatrixMarkColSet(ih, col1, mark);
      iupMatrixDrawTitleColumns(ih, col1, col1);
      if (ih->data->lines.num_noscroll>1)
        iupMatrixDrawCells(ih, 1, col1, ih->data->lines.num_noscroll-1, col1);
    }

    iupMatrixDrawCells(ih, 1, col1, ih->data->lines.num-1, col1);
  }
  else if (ih->data->mark_mode == IMAT_MARK_CELL)
  {
    iMatrixMarkCell(ih, lin1, col1, mark, markedit_cb, mark_cb);
    iupMatrixDrawCells(ih, lin1, col1, lin1, col1);
  }
}

static void iMatrixMarkBlock(Ihandle* ih, int lin1, int col1, int lin2, int col2, int mark, IFniii markedit_cb, IFnii mark_cb)
{
  int lin, col;

  if (lin1 > lin2) {int t = lin1; lin1 = lin2; lin2 = t;}
  if (col1 > col2) {int t = col1; col1 = col2; col2 = t;}

  if (ih->data->mark_full1 == IMAT_PROCESS_LIN)
  {
    for(lin=lin1; lin<=lin2; lin++)
      iMatrixMarkItem(ih, lin, 0, mark, markedit_cb, mark_cb);
  }
  else if(ih->data->mark_full1 == IMAT_PROCESS_COL)
  {
    for(col=col1; col<=col2; col++)
      iMatrixMarkItem(ih, 0, col, mark, markedit_cb, mark_cb);
  }
  else if (ih->data->mark_mode == IMAT_MARK_CELL)
  {
    for(lin=lin1; lin<=lin2; lin++)
    {
      for(col=col1; col<=col2; col++)
        iMatrixMarkItem(ih, lin, col, mark, markedit_cb, mark_cb);
    }
  }
}

void iupMatrixMarkBlockInc(Ihandle* ih, int lin2, int col2)
{
  /* called only when "shift" is pressed or click and drag 
     and MARKMULTIPLE=YES.
     Called many times for each selection increase. */
  IFniii markedit_cb = NULL;
  IFnii mark_cb = NULL;

  iupMatrixPrepareDrawData(ih);

  ih->data->mark_full2 = 0;

  if ((lin2 == 0 && col2 == 0) || (ih->data->noscroll_as_title && lin2 < ih->data->lines.num_noscroll && col2 < ih->data->columns.num_noscroll))
    return;
  /* If it was pointing for a column title... */
  else if (lin2 == 0 || (ih->data->noscroll_as_title && lin2 < ih->data->lines.num_noscroll))
  {
    if ((ih->data->mark_mode == IMAT_MARK_CELL && ih->data->mark_multiple) || 
         ih->data->mark_mode & IMAT_MARK_COL)
      ih->data->mark_full2 = IMAT_PROCESS_COL;
  }
  /* If it was pointing for a line title... */
  else if (col2 == 0 || (ih->data->noscroll_as_title && col2 < ih->data->columns.num_noscroll))
  {
    if ((ih->data->mark_mode == IMAT_MARK_CELL && ih->data->mark_multiple) || 
         ih->data->mark_mode & IMAT_MARK_LIN)
      ih->data->mark_full2 = IMAT_PROCESS_LIN;
  }

  /* if new block is not the same, does nothing */
  if (ih->data->mark_full1 != ih->data->mark_full2)
    return;

  if (ih->data->mark_mode == IMAT_MARK_CELL && ih->data->callback_mode)
  {
    markedit_cb = (IFniii)IupGetCallback(ih, "MARKEDIT_CB");
    mark_cb = (IFnii)IupGetCallback(ih, "MARK_CB");
  }

  if (ih->data->mark_lin1 != -1 && ih->data->mark_col1 != -1)
  {
    /* Unmark previous block */
    if (ih->data->mark_lin2 != -1 && ih->data->mark_col2 != -1)
      iMatrixMarkBlock(ih, ih->data->mark_lin1, ih->data->mark_col1, ih->data->mark_lin2, ih->data->mark_col2, 0, markedit_cb, mark_cb);

    ih->data->mark_lin2 = lin2;
    ih->data->mark_col2 = col2;

    /* Unmark new block */
    iMatrixMarkBlock(ih, ih->data->mark_lin1, ih->data->mark_col1, ih->data->mark_lin2, ih->data->mark_col2, 1, markedit_cb, mark_cb);
  }
}

void iupMatrixMarkBlockReset(Ihandle* ih)
{
  ih->data->mark_lin1 = -1;
  ih->data->mark_col1 = -1;
  ih->data->mark_lin2 = -1;
  ih->data->mark_col2 = -1;

  ih->data->mark_full1 = 0;
  ih->data->mark_full2 = 0;

  ih->data->mark_block = 0;
}

void iupMatrixMarkBlockSet(Ihandle* ih, int ctrl, int lin1, int col1)
{
  int mark = 1, mark_full_all;
  IFniii markedit_cb = NULL;
  IFnii mark_cb = NULL;

  iupMatrixMarkBlockReset(ih);
  iupMatrixPrepareDrawData(ih);

  if (!ih->data->mark_multiple || ih->data->mark_continuous || !ctrl)
  {
    iupMatrixMarkClearAll(ih, 1);
    iupMatrixDraw(ih, 0);
  }
  else
    mark = -1; /* toggle mark state */

  ih->data->mark_full1 = 0;
  mark_full_all = 0;

  if ((lin1 == 0 && col1 == 0) || (ih->data->noscroll_as_title && lin1 < ih->data->lines.num_noscroll && col1 < ih->data->columns.num_noscroll))
  {
    if ((ih->data->mark_mode == IMAT_MARK_CELL && ih->data->mark_multiple) ||
        ih->data->mark_mode == IMAT_MARK_COL ||
        ih->data->mark_mode == IMAT_MARK_LIN)
      mark_full_all = 1;
  }
  /* If it was pointing for a column title... */
  else if (lin1 == 0 || (ih->data->noscroll_as_title && lin1 < ih->data->lines.num_noscroll))
  {
    if ((ih->data->mark_mode == IMAT_MARK_CELL && ih->data->mark_multiple) || 
         ih->data->mark_mode & IMAT_MARK_COL)
      ih->data->mark_full1 = IMAT_PROCESS_COL;
  }
  /* If it was pointing for a line title... */
  else if (col1 == 0 || (ih->data->noscroll_as_title && col1 < ih->data->columns.num_noscroll))
  {
    if ((ih->data->mark_mode == IMAT_MARK_CELL && ih->data->mark_multiple) || 
         ih->data->mark_mode & IMAT_MARK_LIN)
      ih->data->mark_full1 = IMAT_PROCESS_LIN;
  }

  if (ih->data->mark_mode == IMAT_MARK_CELL && ih->data->callback_mode)
  {
    markedit_cb = (IFniii)IupGetCallback(ih, "MARKEDIT_CB");
    mark_cb = (IFnii)IupGetCallback(ih, "MARK_CB");
  }

  if (mark_full_all)
  {
    int lin, col;

    if (ih->data->mark_mode == IMAT_MARK_CELL)
    {
      for (col = 1; col < ih->data->columns.num; col++)
      {
        for(lin = 1; lin < ih->data->lines.num; lin++)
          iMatrixMarkCell(ih, lin, col, mark, markedit_cb, mark_cb);
      }
    }
    else if (ih->data->mark_mode == IMAT_MARK_LIN)
    {
      for(lin = 1; lin < ih->data->lines.num; lin++)
        iMatrixMarkLinSet(ih, lin, mark);

      iupMatrixDrawTitleLines(ih, 1, ih->data->lines.num-1);
    }
    else if (ih->data->mark_mode == IMAT_MARK_COL)
    {
      for (col = 1; col < ih->data->columns.num; col++)
        iMatrixMarkColSet(ih, col, mark);

      iupMatrixDrawTitleColumns(ih, 1, ih->data->columns.num-1);
    }

    iupMatrixDrawCells(ih, 1, 1, ih->data->lines.num-1, ih->data->columns.num-1);
  }
  else
    iMatrixMarkItem(ih, lin1, col1, mark, markedit_cb, mark_cb);

  ih->data->mark_lin1 = lin1;
  ih->data->mark_col1 = col1;
  ih->data->mark_block = 1;
}

static void iMatrixMarkAllLinCol(ImatLinColData *p, int mark)
{
  int i;
  for(i = 1; i < p->num; i++)
  {
    if (mark)
      p->dt[i].flags |= IMAT_IS_MARKED;
    else
      p->dt[i].flags &= ~IMAT_IS_MARKED;
  }
}

void iupMatrixMarkClearAll(Ihandle* ih, int check)
{
  /* "!check" is used to clear all marks independent from MARKMODE */

  if (ih->data->mark_mode == IMAT_MARK_CELL || !check)
  {
    int lin, col;
    IFniii markedit_cb = NULL;

    if (check)
      markedit_cb = (IFniii)IupGetCallback(ih, "MARKEDIT_CB");

    for(lin = 1; lin < ih->data->lines.num; lin++)
    {
      for(col = 1; col < ih->data->columns.num; col++)
        iMatrixMarkCell(ih, lin, col, 0, markedit_cb, NULL);
    }
  }

  if (ih->data->mark_mode & IMAT_MARK_LIN || !check)
    iMatrixMarkAllLinCol(&(ih->data->lines), 0);

  if (ih->data->mark_mode & IMAT_MARK_COL || !check)
    iMatrixMarkAllLinCol(&(ih->data->columns), 0);
}

int iupMatrixColumnIsMarked(Ihandle* ih, int col)
{
  if (col == 0 ||  /* Line titles are never marked... */
      !(ih->data->mark_mode & IMAT_MARK_COL))
    return 0;

  return ih->data->columns.dt[col].flags & IMAT_IS_MARKED;
}

int iupMatrixLineIsMarked(Ihandle* ih, int lin)
{
  if (lin == 0 || /* Column titles are never marked... */
      !(ih->data->mark_mode & IMAT_MARK_LIN))
    return 0;

  return ih->data->lines.dt[lin].flags & IMAT_IS_MARKED;
}

int iupMatrixSetMarkedAttrib(Ihandle* ih, const char* value)
{
  int lin, col, mark;
  IFniii markedit_cb;

  if (ih->data->mark_mode == IMAT_MARK_NO)
    return 0;

  if (!value)
    iupMatrixMarkClearAll(ih, 1);
  else if (*value == 'C' || *value == 'c')  /* columns */
  {
    if (ih->data->mark_mode == IMAT_MARK_LIN)
      return 0;

    value++; /* skip C mark */
    if ((int)strlen(value) != ih->data->columns.num-1)
      return 0;

    markedit_cb = (IFniii)IupGetCallback(ih, "MARKEDIT_CB");

    for(col = 1; col < ih->data->columns.num; col++)
    {
      if (*value++ == '1')
        mark = 1;
      else
        mark = 0;

      /* mark all the cells for that column */
      if (ih->data->mark_mode == IMAT_MARK_CELL)
      {
        for(lin = 1; lin < ih->data->lines.num; lin++)
          iMatrixMarkCell(ih, lin, col, mark, markedit_cb, NULL);
      }
      else
        iMatrixMarkColSet(ih, col, mark);
    }

    if (ih->data->mark_mode & IMAT_MARK_LIN)
      iMatrixMarkAllLinCol(&(ih->data->lines), 0);
  }
  else if (*value == 'L' || *value == 'l')  /* lines */
  {
    if (ih->data->mark_mode == IMAT_MARK_COL)
      return 0;

    value++; /* skip L mark */
    if ((int)strlen(value) != ih->data->lines.num-1)
      return 0;

    markedit_cb = (IFniii)IupGetCallback(ih, "MARKEDIT_CB");

    for(lin = 1; lin < ih->data->lines.num; lin++)
    {
      if (*value++ == '1')
        mark = 1;
      else
        mark = 0;

      /* Mark all the cells for that line */
      if (ih->data->mark_mode == IMAT_MARK_CELL)
      {
        for(col = 1; col < ih->data->columns.num; col++)
          iMatrixMarkCell(ih, lin, col, mark, markedit_cb, NULL);
      }
      else
        iMatrixMarkLinSet(ih, lin, mark);
    }

    if (ih->data->mark_mode & IMAT_MARK_COL)
      iMatrixMarkAllLinCol(&(ih->data->columns), 0);
  }
  else if (ih->data->mark_mode == IMAT_MARK_CELL)  /* cells */
  {
    if ((int)strlen(value) != (ih->data->lines.num-1)*(ih->data->columns.num-1))
      return 0;

    markedit_cb = (IFniii)IupGetCallback(ih, "MARKEDIT_CB");

    for(lin = 1; lin < ih->data->lines.num; lin++)
    {
      for(col = 1; col < ih->data->columns.num; col++)
      {
        if (*value++ == '1')
          mark = 1;
        else
          mark = 0;

        iMatrixMarkCell(ih, lin, col, mark, markedit_cb, NULL);
      }
    }
  }

  if (ih->handle)
    iupMatrixDraw(ih, 1);

  return 0;
}

char* iupMatrixGetMarkedAttrib(Ihandle* ih)
{
  int lin, col, size;
  IFnii mark_cb;
  char* p, *value = NULL;
  int exist_mark = 0;           /* Show if there is someone marked */

  if (ih->data->mark_mode == IMAT_MARK_NO)
    return NULL;

  mark_cb = (IFnii)IupGetCallback(ih, "MARK_CB");

  if (ih->data->mark_mode == IMAT_MARK_CELL)
  {
    size = (ih->data->lines.num-1) * (ih->data->columns.num-1) + 1;
    value = iupStrGetMemory(size);
    p = value;

    for(lin = 1; lin < ih->data->lines.num; lin++)
    {
      for(col = 1; col < ih->data->columns.num; col++)
      {
         if (iupMatrixGetMark(ih, lin, col, mark_cb))
         {
           exist_mark = 1;
           *p++ = '1';
         }
         else
           *p++ = '0';
      }
    }
    *p = 0;
  }
  else
  {
    int marked_lines = 0, marked_cols = 0;

    if (ih->data->mark_mode == IMAT_MARK_LINCOL) /* must find which format to return */
    {
      /* look for a marked column */
      for(col = 1; col < ih->data->columns.num; col++)
      {
        if (ih->data->columns.dt[col].flags & IMAT_IS_MARKED)
        {
          marked_cols = 1; /* at least one column is marked */
          break;
        }
      }

      if (!marked_cols)
        marked_lines = 1;
    }
    else if (ih->data->mark_mode == IMAT_MARK_LIN)
      marked_lines = 1;
    else if (ih->data->mark_mode == IMAT_MARK_COL)
      marked_cols = 1;

    if (marked_lines)
    {
      size = 1 + (ih->data->lines.num-1) + 1;
      value = iupStrGetMemory(size);
      p = value;

      *p++ = 'L';

      for(lin = 1; lin < ih->data->lines.num; lin++)
      {
        if (ih->data->lines.dt[lin].flags & IMAT_IS_MARKED)
        {
          exist_mark = 1;
          *p++ = '1';
        }
        else
         *p++ = '0';
      }
      *p = 0;
    }
    else if (marked_cols)
    {
      size = 1 + (ih->data->columns.num-1) + 1;
      value = iupStrGetMemory(size);
      p = value;

      *p++ = 'C';

      for(col = 1; col < ih->data->columns.num; col++)
      {
        if (ih->data->columns.dt[col].flags & IMAT_IS_MARKED)
        {
          exist_mark = 1;
          *p++ = '1';
        }
        else
         *p++ = '0';
      }
      *p = 0;
    }
  }

  return exist_mark? value: NULL;
}

int iupMatrixSetMarkAttrib(Ihandle* ih, int lin, int col, const char* value)
{
  if (ih->data->mark_mode == IMAT_MARK_NO)
    return 0;

  if (lin >= 0 && col >= 0)  /* both are specified */
  {
    if (!iupMatrixCheckCellPos(ih, lin, col))
      return 0;

    if (ih->data->mark_mode == IMAT_MARK_CELL)
    {
      IFniii markedit_cb = (IFniii)IupGetCallback(ih, "MARKEDIT_CB");
      int mark, ret = 0;

      if (lin == 0 || col == 0) /* title can NOT have a mark */
        return 0;

      mark = iupStrBoolean(value);

      if (iMatrixSetMarkCell(ih, lin, col, mark, markedit_cb))
        ret = 1;

      if (ih->handle)
      {
        /* This assumes that the matrix has been draw completely previously */
        iupMatrixPrepareDrawData(ih);
        iupMatrixDrawCells(ih, lin, col, lin, col);
      }

      return ret;
    }
    else
    {
      int mark = iupStrBoolean(value);

      if (ih->data->mark_mode & IMAT_MARK_LIN && lin>0)
      {
        if (mark)
          ih->data->lines.dt[lin].flags |= IMAT_IS_MARKED;
        else
          ih->data->lines.dt[lin].flags &= ~IMAT_IS_MARKED;
      }

      if (ih->data->mark_mode & IMAT_MARK_COL && col>0)
      {
        if (mark)
          ih->data->columns.dt[col].flags |= IMAT_IS_MARKED;
        else
          ih->data->columns.dt[col].flags &= ~IMAT_IS_MARKED;
      }

      if (ih->handle)
      {
        /* This assumes that the matrix has been drawn completely previously */
        iupMatrixPrepareDrawData(ih);
        iupMatrixDrawCells(ih, lin, col, lin, col);
      }
    }
  }

  return 0;
}

char* iupMatrixGetMarkAttrib(Ihandle* ih, int lin, int col)
{
  if (ih->data->mark_mode == IMAT_MARK_NO)
    return "0";

  if (lin >= 0 && col >= 0)  /* both are specified */
  {
    IFnii mark_cb = (IFnii)IupGetCallback(ih, "MARK_CB");

    if (!iupMatrixCheckCellPos(ih, lin, col))
      return NULL;

    if (iupMatrixGetMark(ih, lin, col, mark_cb))
      return "1";

    return "0";
  }

  return NULL;
}
