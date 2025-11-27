/** \file
 * \brief iupmatrix control
 * change number of columns or lines
 *
 * See Copyright Notice in "iup.h"
 */

/**************************************************************************/
/*  Functions to change the number of lines and columns of the matrix,    */
/*  after it has been created.                                            */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"


#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"

#include "iupmat_def.h"
#include "iupmat_edit.h"
#include "iupmat_mem.h"
#include "iupmat_numlc.h"
#include "iupmat_draw.h"
#include "iupmat_aux.h"


/* Always preserve these attributes here because they are actually stored in the hash table.
   Some of there also have flags, but flags are only used to signal that an attribute was set 
   and we must be consistent between flags and hash table.

   BGCOLOR
   FGCOLOR
   FONT
   FRAMEHORIZCOLOR
   FRAMEVERTCOLOR
   MASK
   ALIGN
   MERGED
   SORTSIGN
   ALIGNMENT
   RASTERWIDTH   
   WIDTH
   RASTERHEIGHT  
   HEIGHT
   NUMERICFORMAT
   NUMERICFORMATTITLE
   TYPE

   Obs: L:C is never stored in the hash table.
         MARK is stored in the hash table when in callback mode and the MARK*_CB callbacks are not defined.
*/

#define IMAT_NUM_ATTRIB_LINE 9
#define IMAT_NUM_ATTRIB_LINE_ONLY 3
static char* imatrix_lin_attrib[IMAT_NUM_ATTRIB_LINE] = {
  "RASTERHEIGHT",  /* only the line */
  "HEIGHT",
  "LINEALIGNMENT",
  "BGCOLOR",  /* use "*" in place of column */
  "FGCOLOR",
  "FONT",
  "TYPE",
  "FRAMEHORIZCOLOR",
  "FRAMEVERTCOLOR"};

#define IMAT_NUM_ATTRIB_COL 12
#define IMAT_NUM_ATTRIB_COL_ONLY 6
static char* imatrix_col_attrib[IMAT_NUM_ATTRIB_COL] = { 
  "NUMERICFORMAT",   /* only the column */
  "NUMERICFORMATTITLE",
  "SORTSIGN",
  "ALIGNMENT",
  "RASTERWIDTH",
  "WIDTH",
  "BGCOLOR",  /* use "*" in place of line */
  "FGCOLOR",
  "FONT",
  "TYPE",
  "FRAMEVERTCOLOR",
  "FRAMEHORIZCOLOR"};

#define IMAT_NUM_ATTRIB_CELL 11
static char* imatrix_cell_attrib[IMAT_NUM_ATTRIB_CELL] = { 
  "BGCOLOR",   /* all use L:C */
  "FGCOLOR",
  "FONT",
  "TYPE",
  "MASK",
  "ALIGN",
  "TOGGLEVALUE",
  "FRAMEHORIZCOLOR",
  "FRAMEVERTCOLOR",
  "MARK",    /* must be at last */
  "MERGED"}; /* must be at last */

void iupMatrixCopyLinAttributes(Ihandle* ih, int lin1, int lin2)
{
  int a, col;
  char* value;

  /* Update the line attributes */
  for(a = 0; a < IMAT_NUM_ATTRIB_LINE; a++)
  {
    if (a < IMAT_NUM_ATTRIB_LINE_ONLY)
    {
      value = iupAttribGetId(ih, imatrix_lin_attrib[a], lin1);
      iupAttribSetStrId(ih, imatrix_lin_attrib[a], lin2, value);
    }
    else
    {
      value = iupAttribGetId2(ih, imatrix_lin_attrib[a], lin1, IUP_INVALID_ID);
      iupAttribSetStrId2(ih, imatrix_lin_attrib[a], lin2, IUP_INVALID_ID, value);
    }
  }

  /* Update the cell attributes */
  for(a = 0; a < IMAT_NUM_ATTRIB_CELL; a++)
  {
    for(col = 0; col < ih->data->columns.num; col++)
    {
      value = iupAttribGetId2(ih, imatrix_cell_attrib[a], lin1, col);
      iupAttribSetStrId2(ih, imatrix_cell_attrib[a], lin2, col, value);
    }
  }
}

void iupMatrixCopyColAttributes(Ihandle* ih, int col1, int col2)
{
  int a, lin;
  char* value;

  /* Update the column attributes */
  for(a = 0; a < IMAT_NUM_ATTRIB_COL; a++)
  {
    if (a < IMAT_NUM_ATTRIB_COL_ONLY)
    {
      value = iupAttribGetId(ih, imatrix_col_attrib[a], col1);
      iupAttribSetStrId(ih, imatrix_col_attrib[a], col2, value);
    }
    else
    {
      value = iupAttribGetId2(ih, imatrix_col_attrib[a], IUP_INVALID_ID, col1);
      iupAttribSetStrId2(ih, imatrix_col_attrib[a], IUP_INVALID_ID, col2, value);
    }
  }

  /* Update the cell attributes */
  for(a = 0; a < IMAT_NUM_ATTRIB_CELL; a++)
  {
    for(lin = 0; lin < ih->data->lines.num; lin++)
    {
      value = iupAttribGetId2(ih, imatrix_cell_attrib[a], lin, col1);
      iupAttribSetStrId2(ih, imatrix_cell_attrib[a], lin, col2, value);
    }
  }
}

static void iMatrixCheckMergedLin(Ihandle* ih, int lin, int add)
{
  int merged;
  for (merged = 1; merged <= ih->data->merge_info_max; merged++)
  {
    ImatMergedData* merged_data = ih->data->merge_info + (merged - 1);
    if (merged_data->used)
    {
      if (add)
      {
        /* this is a new line */
        if (lin < merged_data->start_lin)
        {
          merged_data->start_lin++;
          merged_data->end_lin++;
        }
        else if (lin <= merged_data->end_lin)
          merged_data->end_lin++;
      }
      else
      {
        /* this line was removed */
        if (lin < merged_data->start_lin)
        {
          /* just move the range */
          merged_data->start_lin--;
          merged_data->end_lin--;
        }
        else if (lin <= merged_data->end_lin)
        {
          /* shrink the range, check if not a range anymore */
          merged_data->end_lin--;

          if (merged_data->start_lin == merged_data->end_lin &&
              merged_data->start_col == merged_data->end_col)
          {
            merged_data->used = 0;
            ih->data->merge_info_count--;

            iupAttribSetId2(ih, "MERGED", merged_data->start_lin, merged_data->start_col, NULL);
          }
        }
      }
    }
  }
}

static void iMatrixCheckMergedCol(Ihandle* ih, int col, int add)
{
  int merged;
  for (merged = 1; merged <= ih->data->merge_info_max; merged++)
  {
    ImatMergedData* merged_data = ih->data->merge_info + (merged - 1);
    if (merged_data->used)
    {
      if (add)
      {
        /* this is a new col */
        if (col < merged_data->start_col)
        {
          merged_data->start_col++;
          merged_data->end_col++;
        }
        else if (col <= merged_data->end_col)
          merged_data->end_col++;
      }
      else
      {
        /* this col was removed */
        if (col < merged_data->start_col)
        {
          /* just move the range */
          merged_data->start_col--;
          merged_data->end_col--;
        }
        else if (col <= merged_data->end_col)
        {
          /* shrink the range, check if not a range anymore */
          merged_data->end_col--;

          if (merged_data->start_lin == merged_data->end_lin &&
              merged_data->start_col == merged_data->end_col)
          {
            merged_data->used = 0;
            ih->data->merge_info_count--;

            iupAttribSetId2(ih, "MERGED", merged_data->start_lin, merged_data->start_col, NULL);
          }
        }
      }
    }
  }
}

static void iMatrixClearLinAttributes(Ihandle* ih, int lin, int add)
{
  int a, col, num_attrib_cell;

  for(a = 0; a < IMAT_NUM_ATTRIB_LINE; a++)
  {
    if (a < IMAT_NUM_ATTRIB_LINE_ONLY)
      iupAttribSetId(ih, imatrix_lin_attrib[a], lin, NULL);
    else
      iupAttribSetId2(ih, imatrix_lin_attrib[a], lin, IUP_INVALID_ID, NULL);
  }

  num_attrib_cell = IMAT_NUM_ATTRIB_CELL;
  if (add)
    num_attrib_cell -= 2; /* do not clear MARK and MERGED */

  for (a = 0; a < num_attrib_cell; a++)
  {
    for(col = 0; col < ih->data->columns.num; col++)
      iupAttribSetId2(ih, imatrix_cell_attrib[a], lin, col, NULL);
  }
}

static void iMatrixClearColAttributes(Ihandle* ih, int col, int add)
{
  int a, lin, num_attrib_cell;

  for(a = 0; a < IMAT_NUM_ATTRIB_COL; a++)
  {
    if (a < IMAT_NUM_ATTRIB_COL_ONLY)
      iupAttribSetId(ih, imatrix_col_attrib[a], col, NULL);
    else
      iupAttribSetId2(ih, imatrix_col_attrib[a], IUP_INVALID_ID, col, NULL);
  }

  num_attrib_cell = IMAT_NUM_ATTRIB_CELL;
  if (add)
    num_attrib_cell -= 2; /* do not clear MARK and MERGED */

  for (a = 0; a < num_attrib_cell; a++)
  {
    for(lin = 0; lin < ih->data->lines.num; lin++)
      iupAttribSetId2(ih, imatrix_cell_attrib[a], lin, col, NULL);
  }
}

static void iMatrixUpdateLineAttributes(Ihandle* ih, int base, int count, int add)
{
  int lin;

  /* Here the size of the matrix is already updated */

  if (add)  /* ADD */
  {
    /* copy the attributes of the moved cells, from base+count to num */
    /*   do it in reverse order to avoid overlapping */
    /* then clear the new space starting from base to base+count */

    for(lin = ih->data->lines.num-1; lin >= base+count; lin--)
      iupMatrixCopyLinAttributes(ih, lin-count, lin);  /* from lin1 to lin2 */

    for(lin = base; lin < base+count; lin++)
      iMatrixClearLinAttributes(ih, lin, 1);
  }
  else  /* DEL */
  {
    /* copy the attributes of the moved cells from base+count to base */
    /* then clear the remaining space starting at num */

    for(lin = base; lin < ih->data->lines.num; lin++)
      iupMatrixCopyLinAttributes(ih, lin+count, lin);

    for(lin = ih->data->lines.num; lin < ih->data->lines.num+count; lin++)
      iMatrixClearLinAttributes(ih, lin, 0);
  }

  /* check if a merged range was changed */
  if (ih->data->merge_info_count)
  {
    for (lin = base; lin < base + count; lin++)
      iMatrixCheckMergedLin(ih, lin, add);
  }
}

static void iMatrixUpdateColumnAttributes(Ihandle* ih, int base, int count, int add)
{
  int col;

  /* Here the size of the matrix is already updated */

  if (add)  /* ADD */
  {
    /* update the attributes of the moved cells, from base+count to num */
    /*   do it in reverse order to avoid overlapping */
    /* then clear the new space starting from base to base+count */

    for(col = ih->data->columns.num-1; col >= base+count; col--)
      iupMatrixCopyColAttributes(ih, col-count, col);

    for(col = base; col < base+count; col++)
      iMatrixClearColAttributes(ih, col, 1);
  }
  else   /* DEL */
  {
    /* copy the attributes of the moved cells from base+count to base */
    /* then clear the remaining space starting at num */

    for(col = base; col < ih->data->columns.num; col++)
      iupMatrixCopyColAttributes(ih, col+count, col);

    for(col = ih->data->columns.num; col < ih->data->columns.num+count; col++)
      iMatrixClearColAttributes(ih, col, 0);
  }

  if (ih->data->merge_info_count)
  {
    for (col = base; col < base + count; col++)
      iMatrixCheckMergedCol(ih, col, add);
  }
}

/**************************************************************************/
/* Exported functions                                                     */
/**************************************************************************/

int iupMatrixGetStartEnd(const char* value, int *base, int *count, int max, int del)
{
  int ret;

  if (!value)
    return 0;

  *base = 0; 
  *count = 1;

  ret = iupStrToIntInt(value, base, count, '-');
  if (ret <= 0 || ret > 2)
    return 0;
  if (ret == 1)
    *count = 1;

  if (*count <= 0)
    return 0;

  if (del)
  {
    if (max==0)
      return 0;

    if (*base <= 0)  /* the first valid element is always 1 */
      *base = 1;

    /* when del, base can be at the last element */
    if (*base > max-1)
      *base = max-1;

    /* when del, count must be inside the existent range */
    if (*base + *count > max)
      *count = max - *base;
  }
  else
  {
    (*base)++; /* add after the given index, so increment to position the base */

    if (*base <= 0)  /* the first valid element is always 1 */
      *base = 1;

    /* when add, base can be just after the last element but not more */
    if (*base > max)
      *base = max;

    /* when add, count can be any positive value */
  }

  return 1;
}

int iupMatrixSetAddLinAttrib(Ihandle* ih, const char* value)
{
  int base, focus_cell_changed=0, count, lines_num = ih->data->lines.num;

  if (!ih->handle)  /* do not do the action before map */
    return 0;       /* allowing this method to be called before map will avoid its storage in the hash table */

  if (!iupMatrixGetStartEnd(value, &base, &count, lines_num, 0))
    return 0;

  /* if the focus cell is after the inserted area */
  if (ih->data->lines.focus_cell >= base)
  {
    /* leave of the edition mode */
    iupMatrixEditHide(ih);

    /* move it to the same cell */
    ih->data->lines.focus_cell += count;

    if (ih->data->lines.focus_cell >= lines_num+count)
      ih->data->lines.focus_cell = lines_num+count-1;

    focus_cell_changed = 1;
  }

  iupMatrixMemReAllocLines(ih, lines_num, lines_num+count, base);

  ih->data->lines.num += count;
  ih->data->need_calcsize = 1;

  if (base < lines_num)  /* If before the last line. */
    iMatrixUpdateLineAttributes(ih, base, count, 1);

  if (focus_cell_changed)
    iupMatrixAuxCallEnterCellCb(ih);

  iupMatrixDraw(ih, 1);
  return 0;
}

int iupMatrixSetDelLinAttrib(Ihandle* ih, const char* value)
{
  int base, focus_cell_changed=0, count, lines_num = ih->data->lines.num;

  if (!ih->handle)  /* do not do the action before map */
    return 0;       /* allowing this method to be called before map will avoid its storage in the hash table */

  if (!iupMatrixGetStartEnd(value, &base, &count, lines_num, 1))
    return 0;

  /* if the focus cell is after the removed area */
  if (ih->data->lines.focus_cell >= base)
  {
    /* leave of the edition mode */
    iupMatrixEditHide(ih);

    /* if the focus cell is inside the removed area */
    if (ih->data->lines.focus_cell <= base+count-1)
      ih->data->lines.focus_cell = base;   /* move it to the first existent cell */
    else
      ih->data->lines.focus_cell -= count; /* move it to the same cell */

    if (ih->data->lines.focus_cell >= lines_num-count)
      ih->data->lines.focus_cell = lines_num-count-1;
    if (ih->data->lines.focus_cell <= 0)
      ih->data->lines.focus_cell = 1;

    focus_cell_changed = 1;
  }

  iupMatrixMemReAllocLines(ih, lines_num, lines_num-count, base);

  ih->data->lines.num -= count;
  ih->data->need_calcsize = 1;

  if (base < lines_num)  /* If before the last line. (always true when deleting) */
    iMatrixUpdateLineAttributes(ih, base, count, 0);

  if (focus_cell_changed)
    iupMatrixAuxCallEnterCellCb(ih);

  iupMatrixDraw(ih, 1);
  return 0;
}

int iupMatrixSetAddColAttrib(Ihandle* ih, const char* value)
{
  int base, focus_cell_changed=0, count, columns_num = ih->data->columns.num;

  if (!ih->handle)  /* do not do the action before map */
    return 0;       /* allowing this method to be called before map will avoid its storage in the hash table */

  if (!iupMatrixGetStartEnd(value, &base, &count, columns_num, 0))
    return 0;

  /* if the focus cell is after the inserted area */
  if (ih->data->columns.focus_cell >= base)
  {
    /* leave the edition mode */
    iupMatrixEditHide(ih);

    /* move it to the same cell */
    ih->data->columns.focus_cell += count;

    if (ih->data->columns.focus_cell >= columns_num+count)
      ih->data->columns.focus_cell = columns_num+count-1;

    focus_cell_changed = 1;
  }

  iupMatrixMemReAllocColumns(ih, columns_num, columns_num+count, base);

  ih->data->columns.num += count;
  ih->data->need_calcsize = 1;

  if (base < columns_num)  /* If before the last column. */
    iMatrixUpdateColumnAttributes(ih, base, count, 1);

  if (focus_cell_changed)
    iupMatrixAuxCallEnterCellCb(ih);

  iupMatrixDraw(ih, 1);
  return 0;
}

int iupMatrixSetDelColAttrib(Ihandle* ih, const char* value)
{
  int focus_cell_changed=0, base, count, columns_num = ih->data->columns.num;

  if (!ih->handle)  /* do not do the action before map */
    return 0;       /* allowing this method to be called before map will avoid its storage in the hash table */

  if (!iupMatrixGetStartEnd(value, &base, &count, columns_num, 1))
    return 0;

  /* if the focus cell is after the removed area */
  if (ih->data->columns.focus_cell >= base)
  {
    /* leave the edition mode */
    iupMatrixEditHide(ih);

    /* if the focus cell is inside the removed area */
    if (ih->data->columns.focus_cell <= base+count-1)
      ih->data->columns.focus_cell = base;    /* move it to the first existent cell */
    else
      ih->data->columns.focus_cell -= count;  /* move it to the same cell */

    if (ih->data->columns.focus_cell >= columns_num-count)
      ih->data->columns.focus_cell = columns_num-count-1;
    if (ih->data->columns.focus_cell <= 0)
      ih->data->columns.focus_cell = 1;

    focus_cell_changed = 1;
  }

  iupMatrixMemReAllocColumns(ih, columns_num, columns_num-count, base);

  ih->data->columns.num -= count;
  ih->data->need_calcsize = 1;

  if (base < columns_num)  /* If before the last column. (always true when deleting) */
    iMatrixUpdateColumnAttributes(ih, base, count, 0);

  if (focus_cell_changed)
    iupMatrixAuxCallEnterCellCb(ih);

  iupMatrixDraw(ih, 1);
  return 0;
}

int iupMatrixSetNumLinAttrib(Ihandle* ih, const char* value)
{
  int num = 0;
  if (iupStrToInt(value, &num))
  {
    int old_focus_cell;

    if (num < 0) num = 0;

    num++; /* add room for title line */

    /* can be set before map */
    if (ih->handle)
    {
      int base;  /* base is after the end */
      if (num >= ih->data->lines.num) /* add or alloc */
        base = ih->data->lines.num;   
      else
        base = num;
      iupMatrixMemReAllocLines(ih, ih->data->lines.num, num, base);  
    }

    ih->data->lines.num = num;  
    if (ih->data->lines.num_noscroll >= ih->data->lines.num)
      ih->data->lines.num_noscroll = ih->data->lines.num - 1;
    if (ih->data->lines.num_noscroll < 1)
      ih->data->lines.num_noscroll = 1;

    ih->data->need_calcsize = 1;

    old_focus_cell = ih->data->lines.focus_cell;

    if (ih->data->lines.focus_cell >= ih->data->lines.num)
      ih->data->lines.focus_cell = ih->data->lines.num-1;
    if (ih->data->lines.focus_cell <= 0)
      ih->data->lines.focus_cell = 1;

    if (old_focus_cell != ih->data->lines.focus_cell)
      iupMatrixAuxCallEnterCellCb(ih);

    if (ih->handle)
      iupMatrixDraw(ih, 1);
  }

  return 0;
}

int iupMatrixSetNumColAttrib(Ihandle* ih, const char* value)
{
  int num = 0;
  if (iupStrToInt(value, &num))
  {
    int old_focus_cell;

    if (num < 0) num = 0;

    num++; /* add room for title column */

    /* can be set before map */
    if (ih->handle)
    {
      int base;  /* base is after the end */
      if (num >= ih->data->columns.num) /* add or alloc */
        base = ih->data->columns.num;
      else
        base = num;
      iupMatrixMemReAllocColumns(ih, ih->data->columns.num, num, base);
    }

    ih->data->columns.num = num;
    if (ih->data->columns.num_noscroll >= ih->data->columns.num)
      ih->data->columns.num_noscroll = ih->data->columns.num - 1;
    if (ih->data->columns.num_noscroll < 1)
      ih->data->columns.num_noscroll = 1;

    ih->data->need_calcsize = 1;

    old_focus_cell = ih->data->lines.focus_cell;

    if (ih->data->columns.focus_cell >= ih->data->columns.num)
      ih->data->columns.focus_cell = ih->data->columns.num-1;
    if (ih->data->columns.focus_cell <= 0)
      ih->data->columns.focus_cell = 1;

    if (old_focus_cell != ih->data->columns.focus_cell)
      iupMatrixAuxCallEnterCellCb(ih);

    if (ih->handle)
      iupMatrixDraw(ih, 1);
  }

  return 0;
}
