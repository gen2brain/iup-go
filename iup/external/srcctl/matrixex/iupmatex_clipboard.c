/** \file
 * \brief IupMatrix Expansion Library.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_array.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_matrixex.h"


static void iMatrixExMarkedCellLimits(const char* marked, int num_lin, int num_col, int *lin1, int *lin2, int *col1, int *col2)
{
  /* use only for MARKMODE=CELL */
  int lin, col;

  *lin1 = num_lin;
  *lin2 = 1;
  *col1 = num_col;
  *col2 = 1;

  for(lin = 1; lin <= num_lin; ++lin)
  {
    for(col = 1; col <= num_col; ++col)
    {
      int pos = (lin-1) * num_col + (col-1);  /* marked array does not include titles */
      if(marked[pos] == '1')
      {
        if(lin < *lin1) *lin1 = lin;
        if(lin > *lin2) *lin2 = lin;
        if(col < *col1) *col1 = col;
        if(col > *col2) *col2 = col;
      }
    }
  }
}

static void iMatrixExCellMarkedStart(const char* marked, int num_lin, int num_col, int *lin1, int *col1)
{
  int lin, col;

  if (*marked == 'C')
  {
    marked++;

    *col1 = 1;

    for(lin = 1; lin <= num_lin; ++lin)
    {
      if (marked[lin] == '1')
      {
        *lin1 = lin;
        return;
      }
    }
  }
  else if (*marked == 'L')
  {
    marked++;

    *lin1 = 1;

    for(col = 1; col <= num_col; ++col)
    {
      if (marked[col] == '1')
      {
        *col1 = col;
        return;
      }
    }
  }
  else
  {
    for(lin = 1; lin <= num_lin; ++lin)
    {
      for(col = 1; col <= num_col; ++col)
      {
        int pos = (lin-1) * num_col + (col-1);  /* marked array does not include titles */
        if (marked[pos] == '1')
        {
          *lin1 = lin;
          *col1 = col;
          return;
        }
      }
    }
  }
}

static int iMatrixExMarkedCellConsistent(const char* marked, int num_lin, int num_col)
{
  int lin, col, selected;
  const char *l1=NULL, *ll=NULL;

  /* the selected column pattern must be consistent along lines */

  for(lin = 1; lin <= num_lin; ++lin)
  {
    selected = 0;

    for(col = 1; col <= num_col; ++col)
    {
      int pos = (lin-1) * num_col + (col-1);  /* marked array does not include titles */
      if (marked[pos] == '1')
      {
        ll = &marked[(lin-1)*num_col];

        if (l1==NULL)
          l1 = ll;

        selected = 1;
        break;
      }
    }

    if (selected && ll!=l1)
    {
      if (strncmp(l1,ll,(size_t)num_col)!=0)
        return 0;
    }
  }

  return 1;
}

static void iMatrixExArrayAddChar(Iarray* data, char c)
{
  int last_count = iupArrayCount(data);
  char* str_data = (char*)iupArrayInc(data);
  str_data[last_count] = c;
}

static void iMatrixExArrayAddStr(Iarray* data, char* str)
{
  int add_count = (int)strlen(str);
  int last_count = iupArrayCount(data);
  char* str_data = (char*)iupArrayAdd(data, add_count);
  memcpy(str_data+last_count, str, add_count);
}

static void iMatrixExArrayAddCell(ImatExData* matex_data, Iarray* data, int lin, int col, char sep)
{
  char* value = iupMatrixExGetCellValue(matex_data->ih, lin, col, 1);  /* get displayed value */

  if (value)
  {
    int add_sep = 0;
    if (strchr(value, sep))
      add_sep = 1;

    if (add_sep)
      iMatrixExArrayAddChar(data, '\"');

    iMatrixExArrayAddStr(data, value);

    if (add_sep)
      iMatrixExArrayAddChar(data, '\"');
  }
  else
    iMatrixExArrayAddChar(data, ' ');
}

static void iMatrixExCopyGetDataMarkedCol(ImatExData* matex_data, Iarray* data, const char* marked, int num_lin, int num_col, char sep)
{
  int lin, col;
  int add_sep;

  for(lin = 1; lin <= num_lin; ++lin)  /* always organize data in lines */
  {
    add_sep = 0;

    if (iupMatrixExIsLineVisible(matex_data->ih, lin))
    {
      for(col = 1; col <= num_col; ++col)
      {
        /* only marked columns */
        if (marked[col-1] == '1' && iupMatrixExIsColumnVisible(matex_data->ih, col))
        {
          if (add_sep)
            iMatrixExArrayAddChar(data, sep);

          iMatrixExArrayAddCell(matex_data, data, lin, col, sep);
          add_sep = 1;
        }
      }

      iMatrixExArrayAddChar(data, '\n');
    }
  }
}

static void iMatrixExCopyGetDataMarkedLin(ImatExData* matex_data, Iarray* data, const char* marked, int num_lin, int num_col, char sep)
{
  int lin, col;
  int add_sep;

  for(lin = 1; lin <= num_lin; ++lin)
  {
    add_sep = 0;

    /* only marked lines */
    if (marked[lin - 1] == '1' && iupMatrixExIsLineVisible(matex_data->ih, lin))
    {
      for(col = 1; col <= num_col; ++col)    /* all columns */
      {
        if (iupMatrixExIsColumnVisible(matex_data->ih, col))
        {
          if (add_sep)
            iMatrixExArrayAddChar(data, sep);

          iMatrixExArrayAddCell(matex_data, data, lin, col, sep);
          add_sep = 1;
        }
      }

      iMatrixExArrayAddChar(data, '\n');
    }
  }
}

static void iMatrixExCopyGetDataMarkedCell(ImatExData* matex_data, Iarray* data, const char* marked, int lin1, int col1, int lin2, int col2, int num_col, int keep_struct, char sep)
{
  int lin, col;
  int add_sep;

  for(lin = lin1; lin <= lin2; ++lin)
  {
    add_sep = 0;

    if (iupMatrixExIsLineVisible(matex_data->ih, lin))
    {
      for(col = col1; col <= col2; ++col)
      {
        if (iupMatrixExIsColumnVisible(matex_data->ih, col))
        {
          int pos = (lin - 1) * num_col + (col - 1);  /* marked array does not include titles */
          if (marked[pos] == '1')
          {
            if (add_sep)
              iMatrixExArrayAddChar(data, sep);

            iMatrixExArrayAddCell(matex_data, data, lin, col, sep);
            add_sep = 1;
          }
          else if (keep_struct)
          {
            if (add_sep)
              iMatrixExArrayAddChar(data, sep);

            iMatrixExArrayAddChar(data, ' ');
            add_sep = 1;
          }
        }
      }

      iMatrixExArrayAddChar(data, '\n');
    }
  }
}

static void iMatrixExCopyGetData(ImatExData* matex_data, Iarray* data, int lin1, int col1, int lin2, int col2, char sep)
{
  int lin, col;
  int add_sep;

  for(lin = lin1; lin <= lin2; ++lin)
  {
    add_sep = 0;

    if (iupMatrixExIsLineVisible(matex_data->ih, lin))
    {
      for(col = col1; col <= col2; ++col)
      {
        if (iupMatrixExIsColumnVisible(matex_data->ih, col))
        {
          if (add_sep)
            iMatrixExArrayAddChar(data, sep);

          iMatrixExArrayAddCell(matex_data, data, lin, col, sep);

          add_sep = 1;
        }
      }

      iMatrixExArrayAddChar(data, '\n');
    }
  }
}

static void iMatrixExCopyData(ImatExData* matex_data, Iarray* data, const char* value)
{
  int num_lin, num_col;
  char sep;

  if (!value)
    return;

  sep = *(iupAttribGetStr(matex_data->ih, "TEXTSEPARATOR"));

  /* reset error state */
  iupAttribSet(matex_data->ih, "LASTERROR", NULL);

  num_lin = IupGetInt(matex_data->ih, "NUMLIN");
  num_col = IupGetInt(matex_data->ih, "NUMCOL");

  if (iupStrEqualNoCase(value, "MARKED"))
  {
    char *marked = IupGetAttribute(matex_data->ih,"MARKED");
    if (!marked)  /* no marked cells */
    {
      iupAttribSet(matex_data->ih, "LASTERROR", "IUP_ERRORNOSELECTION");
      return;
    }

    if (*marked == 'C')
    {
      marked++;

      iMatrixExCopyGetDataMarkedCol(matex_data, data, marked, num_lin, num_col, sep);
    }
    else if (*marked == 'L')
    {
      marked++;

      iMatrixExCopyGetDataMarkedLin(matex_data, data, marked, num_lin, num_col, sep);
    }
    else
    {
      int lin1, lin2, col1, col2;
      int keep_struct = IupGetInt(matex_data->ih, "COPYKEEPSTRUCT");

      iMatrixExMarkedCellLimits(marked, num_lin, num_col, &lin1, &lin2, &col1, &col2);

      /* check consistency only when not keeping structure */
      if (!keep_struct && !iMatrixExMarkedCellConsistent(marked, num_lin, num_col))
      {
        iupAttribSet(matex_data->ih, "LASTERROR", "IUP_ERRORINVALIDSELECTION");
        return;
      }

      iMatrixExCopyGetDataMarkedCell(matex_data, data, marked, lin1, col1, lin2, col2, num_col, keep_struct, sep);
    }
  }
  else 
  {
    int lin1=1, lin2=num_lin, 
        col1=1, col2=num_col;

    if (!iupStrEqualNoCase(value, "ALL"))
    {
      sscanf(value, "%d:%d-%d:%d", &lin1, &col1, &lin2, &col2);
      iupMatrixExCheckLimitsOrder(&lin1, &lin2, 1, num_lin);
      iupMatrixExCheckLimitsOrder(&col1, &col2, 1, num_col);
    }

    iMatrixExCopyGetData(matex_data, data, lin1, col1, lin2, col2, sep);
  }
}

static int iMatrixExSetCopyAttrib(Ihandle *ih, const char* value)
{
  ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
  Iarray* data =  iupArrayCreate(100, sizeof(char));

  iMatrixExCopyData(matex_data, data, value);

  if (iupArrayCount(data)!=0)
  {
    Ihandle* clipboard;

    iMatrixExArrayAddChar(data, '\0');

    clipboard = IupClipboard();
    IupSetAttribute(clipboard, "TEXT", NULL);  /* clear all data from clipboard */
    IupSetAttribute(clipboard, "TEXT", (char*)iupArrayGetData(data));
    IupDestroy(clipboard);
  }

  iupArrayDestroy(data);
  return 0;
}

static int iMatrixExSetCopyDataAttrib(Ihandle *ih, const char* value)
{
  if (!value)
    iupAttribSet(ih, "COPYDATA", NULL);
  else
  {
    ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
    Iarray* data =  iupArrayCreate(100, sizeof(char));

    iMatrixExCopyData(matex_data, data, value);

    if (iupArrayCount(data)!=0)
    {
      iMatrixExArrayAddChar(data, '\0');

      iupAttribSetStr(ih, "COPYDATA", (char*)iupArrayGetData(data));
    }

    iupArrayDestroy(data);
  }
  return 0;
}

static int iMatrixExStrGetDataSize(const char* data, int *num_lin, int *num_col, char *sep)
{
  int len = (int)strlen(data);
  *num_lin = iupStrLineCount(data, len);
  if (data[len-1] == '\n')
    (*num_lin)--;  /* avoid an empty last line */

  if (*num_lin == 0)
    return 0;

  if (*sep != 0)
    *num_col = iupStrCountChar(data, *sep);
  else
  {
    /* try to guess the separator */
    *sep = '\t';
    *num_col = iupStrCountChar(data, *sep);
    if (*num_col == 0)
    {
      *sep = ';';
      *num_col = iupStrCountChar(data, *sep);
    }
    if (*num_col == 0)
    {
      *sep = ' ';
      *num_col = iupStrCountChar(data, *sep);
    }
  }

  /* If there is no column separator for the last column, so add it */
  if (!((data[len-1] == '\n' && data[len-2] == *sep) ||
        (data[len-1] == *sep)))
    *num_col += *num_lin;

  if (*num_col == 0)
    return 0;

  if ((*num_col)%(*num_lin)!=0)
    return 0;

  *num_col = (*num_col)/(*num_lin);
  return 1;
}

static char* iMatrixExStrCopyData(char* value, int *value_max_size, const char* data, int value_len)
{
  if (*value_max_size < value_len)
  {
    *value_max_size = value_len+10;
    value = realloc(value, *value_max_size);
  }
  memcpy(value, data, value_len);
  value[value_len] = 0;
  return value;
}

static void iMatrixExPasteSetData(Ihandle *ih, const char* data, int data_num_lin, int data_num_col, char sep, int start_lin, int start_col, int num_lin, int num_col, const char* busyname)
{
  ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
  int lin, col, len, l, c;
  char* value = NULL;
  int value_max_size = 0, value_len;

  iupMatrixExBusyStart(matex_data, data_num_lin*data_num_col, busyname);

  lin = start_lin;
  l = 0;
  while (lin <= num_lin && l<data_num_lin && *data)
  {
    if (iupMatrixExIsLineVisible(ih, lin))
    {
      const char* next_line = iupStrNextLine(data, &len); l++;

      col = start_col;
      c = 0;
      while (len && col <= num_col && c<data_num_col)
      {
        if (iupMatrixExIsColumnVisible(ih, col))
        {
          const char* next_value = iupStrNextValue(data, len, &value_len, sep);  c++;

          if (value_len)
          {
            if (data[0] == '\"' && data[value_len - 1] == '\"')
            {
              data++;
              value_len -= 2;
              len -= 2;
            }

            if (value_len)
            {
              value = iMatrixExStrCopyData(value, &value_max_size, data, value_len);
              iupMatrixExSetCellValue(matex_data->ih, lin, col, value);
            }
          }
          else
            iupMatrixExSetCellValue(matex_data->ih, lin, col, "");

          data = next_value;
          len -= value_len+1;

          if (!iupMatrixExBusyInc(matex_data))
          {
            if (value) 
              free(value);
            return;
          }
        }

        col++;
      }

      data = next_line;
    }

    lin++;
  }

  iupMatrixExBusyEnd(matex_data);

  if (value)
    free(value);

  iupBaseCallValueChangedCb(matex_data->ih);
}

static int iMatrixExGetVisibleNumLin(Ihandle *ih, int start_lin, int data_num_lin)
{
  int lin, vis_num_lin = data_num_lin;
  for (lin=start_lin; lin < start_lin+data_num_lin; lin++)
  {
    if (!iupMatrixExIsLineVisible(ih, lin))
      vis_num_lin++;
  }
  return vis_num_lin;
}

static int iMatrixExGetVisibleNumCol(Ihandle *ih, int start_col, int data_num_col)
{
  int col, vis_num_col = data_num_col;
  for (col=start_col; col < start_col+data_num_col; col++)
  {
    if (!iupMatrixExIsColumnVisible(ih, col))
      vis_num_col++;
  }
  return vis_num_col;
}

static void iMatrixExPasteData(Ihandle *ih, const char* data, int lin, int col, const char* busyname)
{
  int num_lin, num_col, skip_lines,
      data_num_lin, data_num_col;
  char sep=0, *str_sep;
  IFnii pastesize_cb;

  /* reset error state */
  iupAttribSet(ih, "LASTERROR", NULL);

  if (!data || data[0]==0)
  {
    iupAttribSet(ih, "LASTERROR", "IUP_ERRORNOTEXT");
    return;
  }

  skip_lines = IupGetInt(ih, "TEXTSKIPLINES");
  if (skip_lines)
  {
    int i;

    for (i=0; i<skip_lines && *data; i++)
    {
      int len;
      const char *next_line = iupStrNextLine(data, &len);
      data = (char*)next_line;
    }

    if (i != skip_lines)
    {
      iupAttribSet(ih, "LASTERROR", "IUP_ERRORNOTEXT");
      return;
    }
  }

  str_sep = iupAttribGet(ih, "TEXTSEPARATOR");  /* don't check for the default value */
  if (str_sep) sep = *str_sep;

  if (!iMatrixExStrGetDataSize(data, &data_num_lin, &data_num_col, &sep))
  {
    iupAttribSet(ih, "LASTERROR", "IUP_ERRORINVALIDDATA");
    return;
  }

  num_lin = IupGetInt(ih, "NUMLIN");
  num_col = IupGetInt(ih, "NUMCOL");

  pastesize_cb = (IFnii)IupGetCallback(ih, "PASTESIZE_CB");
  if (pastesize_cb)
  {
    int vis_num_lin = iMatrixExGetVisibleNumLin(ih, lin, data_num_lin);
    int vis_num_col = iMatrixExGetVisibleNumCol(ih, col, data_num_col);
    if (lin+vis_num_lin>num_lin ||
        col+vis_num_col>num_col)
    {
      int ret = pastesize_cb(ih, lin+vis_num_lin, col+vis_num_col);
      if (ret == IUP_IGNORE)
        return;
      else if (ret == IUP_CONTINUE)
      {
        if (lin+vis_num_lin>num_lin) IupSetInt(ih, "NUMLIN", lin+vis_num_lin);
        if (col+vis_num_col>num_col) IupSetInt(ih, "NUMCOL", col+vis_num_col);
      }
    }
  }

  iMatrixExPasteSetData(ih, data, data_num_lin, data_num_col, sep, lin, col, num_lin, num_col, busyname);
}

static int iMatrixExSetPasteAttrib(Ihandle *ih, const char* value)
{
  int lin=0, col=0;

  Ihandle* clipboard = IupClipboard();
  char* data = IupGetAttribute(clipboard, "TEXT");
  IupDestroy(clipboard);

  if (iupStrEqualNoCase(value, "FOCUS"))
    IupGetIntInt(ih, "FOCUSCELL", &lin, &col);
  else if (iupStrEqualNoCase(value, "MARKED"))
  {
    char *marked = IupGetAttribute(ih,"MARKED");
    if (marked)
    {
      int num_lin = IupGetInt(ih, "NUMLIN");
      int num_col = IupGetInt(ih, "NUMCOL");
      iMatrixExCellMarkedStart(marked, num_lin, num_col, &lin, &col);
    }
  }
  else
  {
    if (iupStrToIntInt(value, &lin, &col, ':')!=2)
      return 0;
  }

  iMatrixExPasteData(ih, data, lin, col, "PASTECLIP");
  return 0;
}

static int iMatrixExSetPasteDataAttrib(Ihandle *ih, const char* data)
{
  int lin=0, col=0;
  IupGetIntInt(ih, "FOCUSCELL", &lin, &col);
  iMatrixExPasteData(ih, data, lin, col, "PASTEDATA");
  return 0;
}

static char* iMatrixReadFile(const char* filename)
{
  char* data;
  long size;
  FILE *file = fopen(filename, "rb");
  if (!file)
    return NULL;

  fseek(file, 0, SEEK_END);
  size = ftell(file);
  if (size <= 0)
  {
    fclose(file);
    return NULL;
  }

  data = (char*)malloc(size + 1);
  if (!data)
  {
    fclose(file);
    return NULL;
  }

  fseek(file, 0, SEEK_SET);
  fread(data, size, 1, file);
  data[size] = 0;
  fclose(file);

  return data;
}

static int iMatrixExSetPasteFileAttrib(Ihandle *ih, const char* value)
{
  char* data, *paste_at;
  int lin = 0, col = 0;

  paste_at = iupAttribGet(ih, "PASTEFILEAT");
  if (paste_at)
  {
    if (iupStrEqualNoCase(paste_at, "FOCUS"))
      IupGetIntInt(ih, "FOCUSCELL", &lin, &col);
    else
    {
      if (iupStrToIntInt(paste_at, &lin, &col, ':') != 2)
        return 0;
    }
  }

  data = iMatrixReadFile(value);
  if (!data)
  {
    iupAttribSet(ih, "LASTERROR", "IUP_ERRORFILEOPEN");
    return 0;
  }

  iMatrixExPasteData(ih, data, lin, col, "PASTEFILE");

  free(data);
  return 0;
}

void iupMatrixExRegisterClipboard(Iclass* ic)
{
  iupClassRegisterCallback(ic, "PASTESIZE_CB", "ii");

  iupClassRegisterAttribute(ic, "PASTE", NULL, iMatrixExSetPasteAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTEDATA", NULL, iMatrixExSetPasteDataAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PASTEFILE", NULL, iMatrixExSetPasteFileAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COPY", NULL, iMatrixExSetCopyAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPYDATA", NULL, iMatrixExSetCopyDataAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COPYKEEPSTRUCT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LASTERROR", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TEXTSEPARATOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "\t", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTSKIPLINES", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
