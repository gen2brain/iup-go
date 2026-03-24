/** \file
 * \brief iupmatrix control memory allocation
 *
 * See Copyright Notice in "iup.h"
 */

/**************************************************************************/
/* Functions to allocate memory                                           */
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
#include "iupmat_mem.h"


static void iMatrixGetInitialValues(Ihandle* ih)
{
  int lin, col;
  char* value;

  for (lin=0; lin<ih->data->lines.num; lin++)
  {
    for (col=0; col<ih->data->columns.num; col++)
    {
      value = iupAttribGetId2(ih, "", lin, col);
      if (value)
      {
        /* get the initial value and remove it from the hash table */

        if (*value)
          ih->data->cells[lin][col].value = iupStrDup(value);

        iupAttribSetId2(ih, "", lin, col, NULL);
      }
    }
  }
}

void iupMatrixMemAlloc(Ihandle* ih)
{
  ih->data->lines.num_alloc = ih->data->lines.num;
  if (ih->data->lines.num_alloc == 1)
    ih->data->lines.num_alloc = 5;

  ih->data->columns.num_alloc = ih->data->columns.num;
  if (ih->data->columns.num_alloc == 1)
    ih->data->columns.num_alloc = 5;

  if (!ih->data->callback_mode)
  {
    int lin;

    ih->data->cells = (ImatCell**)calloc(ih->data->lines.num_alloc, sizeof(ImatCell*));
    for (lin = 0; lin < ih->data->lines.num_alloc; lin++)
      ih->data->cells[lin] = (ImatCell*)calloc(ih->data->columns.num_alloc, sizeof(ImatCell));

    iMatrixGetInitialValues(ih);
  }

  ih->data->lines.dt = (ImatLinCol*)calloc(ih->data->lines.num_alloc, sizeof(ImatLinCol));
  ih->data->columns.dt = (ImatLinCol*)calloc(ih->data->columns.num_alloc, sizeof(ImatLinCol));

  /* numeric_columns is allocated when a NUMERIC* attribute is set */
  /* sort_line_index is allocated when the SORTCOLUMN attribute is set */
}

void iupMatrixMemRelease(Ihandle* ih)
{
  if (ih->data->cells)
  {
    int lin, col;
    for (lin = 0; lin < ih->data->lines.num_alloc; lin++)
    {
      for (col = 0; col < ih->data->columns.num_alloc; col++)
      {
        ImatCell* cell = &(ih->data->cells[lin][col]);
        if (cell->value)
        {
          free(cell->value);
          cell->value = NULL;
        }
      }
      free(ih->data->cells[lin]);
      ih->data->cells[lin] = NULL;
    }
    free(ih->data->cells);
    ih->data->cells = NULL;
  }

  if (ih->data->columns.dt)
  {
    free(ih->data->columns.dt);
    ih->data->columns.dt = NULL;
  }

  if (ih->data->lines.dt)
  {
    free(ih->data->lines.dt);
    ih->data->lines.dt = NULL;
  }

  if (ih->data->numeric_columns)
  {
    free(ih->data->numeric_columns);
    ih->data->numeric_columns = NULL;
  }

  if (ih->data->sort_line_index)
  {
    free(ih->data->sort_line_index);
    ih->data->sort_line_index = NULL;
  }

  if (ih->data->merge_info)
  {
    free(ih->data->merge_info);
    ih->data->merge_info = NULL;
    ih->data->merge_info_max = 0;
    ih->data->merge_info_count = 0;
  }
}

void iupMatrixMemReAllocLines(Ihandle* ih, int old_num, int num, int base)
{
  int end, diff_num, shift_num, lin;

  if (ih->data->undo_redo) iupAttribSetClassObject(ih, "UNDOCLEAR", NULL);

  /* base is the first line where the change started */

  /* If it doesn't have enough lines allocated, then allocate more space */
  if (num > ih->data->lines.num_alloc)  /* this also implicates that num>old_num */
  {
    int old_alloc = ih->data->lines.num_alloc;
    ih->data->lines.num_alloc = num;

    if (!ih->data->callback_mode)
    {
      ih->data->cells = (ImatCell**)realloc(ih->data->cells, ih->data->lines.num_alloc*sizeof(ImatCell*));

      /* new space are allocated at the end, later we need to move the old data and clear the available space */
      for(lin = old_alloc; lin < num; lin++)
        ih->data->cells[lin] = (ImatCell*)calloc(ih->data->columns.num_alloc, sizeof(ImatCell));
    }

    ih->data->lines.dt = (ImatLinCol*)realloc(ih->data->lines.dt, ih->data->lines.num_alloc*sizeof(ImatLinCol));
    if (ih->data->sort_line_index)
      ih->data->sort_line_index = (int*)realloc(ih->data->sort_line_index, ih->data->lines.num_alloc*sizeof(int));
  }

  if (old_num==num)
    return;

  if (num>old_num) /* ADD */
  {
    diff_num = num-old_num;      /* size of the opened space */
    shift_num = old_num-base;    /* size of the data to be moved (base maximum is old_num) */
    end = base+diff_num;

    /* shift the old data, opening space for new data, from base to end */
    /*   do it in reverse order to avoid overlapping */
    if (shift_num)
    {
      if (!ih->data->callback_mode)
        for (lin = shift_num-1; lin >= 0; lin--)   /* all columns, shift_num lines */
          memmove(ih->data->cells[lin+end], ih->data->cells[lin+base], ih->data->columns.num_alloc*sizeof(ImatCell));
      memmove(ih->data->lines.dt+end, ih->data->lines.dt+base, shift_num*sizeof(ImatLinCol));
    }

    /* then clear the new space starting at base */
    if (!ih->data->callback_mode)
      for (lin = 0; lin < diff_num; lin++)        /* all columns, diff_num lines */
        memset(ih->data->cells[lin+base], 0, ih->data->columns.num_alloc*sizeof(ImatCell));
    memset(ih->data->lines.dt+base, 0, diff_num*sizeof(ImatLinCol));

    /* reset sort indices */
    if (ih->data->sort_has_index) ih->data->sort_has_index = 0;
  }
  else /* DEL */
  {
    diff_num = old_num-num;  /* size of the removed space */
    shift_num = num-base;    /* size of the data to be moved */
    end = base+diff_num;

    /* release memory from the opened space */
    if (!ih->data->callback_mode)
    {
      int col;
	  
      for(lin = base; lin < end; lin++)   /* all columns, base-end lines */
      {
        for (col = 0; col < ih->data->columns.num_alloc; col++)
        {
          ImatCell* cell = &(ih->data->cells[lin][col]);
          if (cell->value)
          {
            free(cell->value);
            cell->value = NULL;
          }
          cell->flags = 0;
        }
      }
    }

    /* move the old data to opened space from end to base */
    if (shift_num)
    {
      if (!ih->data->callback_mode)
        for (lin = 0; lin < shift_num; lin++) /* all columns, shift_num lines */
          memmove(ih->data->cells[lin+base], ih->data->cells[lin+end], ih->data->columns.num_alloc*sizeof(ImatCell));
      memmove(ih->data->lines.dt+base, ih->data->lines.dt+end, shift_num*sizeof(ImatLinCol));
    }

    /* then clear the remaining space starting at num */
    if (!ih->data->callback_mode)
      for (lin = 0; lin < diff_num; lin++)   /* all columns, diff_num lines */
        memset(ih->data->cells[lin+num], 0, ih->data->columns.num_alloc*sizeof(ImatCell));
    memset(ih->data->lines.dt+num, 0, diff_num*sizeof(ImatLinCol));

    /* reset sort indices */
    if (ih->data->sort_has_index) ih->data->sort_has_index = 0;
  }
}

void iupMatrixMemReAllocColumns(Ihandle* ih, int old_num, int num, int base)
{
  int lin, end, diff_num, shift_num;

  if (ih->data->undo_redo) iupAttribSetClassObject(ih, "UNDOCLEAR", NULL);

  /* base is the first column where the change started */

  /* If it doesn't have enough columns allocated, then allocate more space */
  if (num > ih->data->columns.num_alloc)  /* this also implicates that also num>old_num */
  {
    ih->data->columns.num_alloc = num;

    /* new space are allocated at the end, later we need to move the old data and clear the available space */

    if (!ih->data->callback_mode)
    {
      for(lin = 0; lin < ih->data->lines.num_alloc; lin++)
        ih->data->cells[lin] = (ImatCell*)realloc(ih->data->cells[lin], ih->data->columns.num_alloc*sizeof(ImatCell));
    }

    ih->data->columns.dt = (ImatLinCol*)realloc(ih->data->columns.dt, ih->data->columns.num_alloc*sizeof(ImatLinCol));
    if (ih->data->numeric_columns)
      ih->data->numeric_columns = (ImatNumericData*)realloc(ih->data->numeric_columns, ih->data->columns.num_alloc*sizeof(ImatNumericData));
  }

  if (old_num==num)
    return;

  if (num>old_num) /* ADD */
  {
    /*   even if (old_num-base)>(num-old_num) memmove will correctly copy the memory */
    /* then clear the opened space starting at base */

    diff_num = num-old_num;     /* size of the opened space */
    shift_num = old_num-base;   /* size of the data to be moved (base maximum is old_num) */
    end = base+diff_num;

    /* shift the old data, opening space for new data, from base to end */
    if (shift_num)
    {
      if (!ih->data->callback_mode)
        for (lin = 0; lin < ih->data->lines.num_alloc; lin++)  /* all lines, shift_num columns */
          memmove(ih->data->cells[lin]+end, ih->data->cells[lin]+base, shift_num*sizeof(ImatCell));
      memmove(ih->data->columns.dt+end, ih->data->columns.dt+base, shift_num*sizeof(ImatLinCol));
      if (ih->data->numeric_columns)
        memmove(ih->data->numeric_columns+end, ih->data->numeric_columns+base, shift_num*sizeof(ImatNumericData));
    }

    /* then clear the opened space starting at base */
    if (!ih->data->callback_mode)
      for (lin = 0; lin < ih->data->lines.num_alloc; lin++)   /* all lines, diff_num columns */
        memset(ih->data->cells[lin]+base, 0, diff_num*sizeof(ImatCell));
    memset(ih->data->columns.dt+base, 0, diff_num*sizeof(ImatLinCol));
    if (ih->data->numeric_columns)
      memset(ih->data->numeric_columns+base, 0, diff_num*sizeof(ImatNumericData));
  }
  else /* DEL */
  {
    diff_num = old_num-num;    /* size of the removed space */
    shift_num = num-base;      /* size of the data to be moved */
    end = base+diff_num;

    /* release memory from the opened space */
    if (!ih->data->callback_mode)
    {
      int col;

      for (lin = 0; lin < ih->data->lines.num_alloc; lin++)  /* all lines, base-end columns */
      {
        for(col = base; col < end; col++)
        {
          ImatCell* cell = &(ih->data->cells[lin][col]);
          if (cell->value)
          {
            free(cell->value);
            cell->value = NULL;
          }
          cell->flags = 0;
        }
      }
    }

    /* move the old data to opened space from end to base */
    /*   even if (num-base)>(old_num-num) memmove will correctly copy the memory */
    if (shift_num)
    {
      if (!ih->data->callback_mode)
        for (lin = 0; lin < ih->data->lines.num_alloc; lin++)  /* all lines, shift_num columns */
          memmove(ih->data->cells[lin]+base, ih->data->cells[lin]+end, shift_num*sizeof(ImatCell));
      memmove(ih->data->columns.dt+base, ih->data->columns.dt+end, shift_num*sizeof(ImatLinCol));
      if (ih->data->numeric_columns)
        memmove(ih->data->numeric_columns+base, ih->data->numeric_columns+end, shift_num*sizeof(ImatNumericData));
    }

    /* then clear the remaining space starting at num */
    if (!ih->data->callback_mode)
      for (lin = 0; lin < ih->data->lines.num_alloc; lin++)   /* all lines, diff_num columns */
        memset(ih->data->cells[lin]+num, 0, diff_num*sizeof(ImatCell));
    memset(ih->data->columns.dt+num, 0, diff_num*sizeof(ImatLinCol));
    if (ih->data->numeric_columns)
      memset(ih->data->numeric_columns+num, 0, diff_num*sizeof(ImatNumericData));
  }
}
