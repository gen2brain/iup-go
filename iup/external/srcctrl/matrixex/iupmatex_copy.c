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


static void iMatrixExCopyColToSetDataSelected(ImatExData* matex_data, int lin, int col, int num_lin, const char* selection, int selection_count, const char* busyname)
{
  int skip_lin = lin;
  /* Must duplicate the memory to ensure it will be valid for all iterations */
  char* value = iupStrDup(iupMatrixExGetCellValue(matex_data->ih, lin, col, 0));  /* get internal value */

  iupMatrixExBusyStart(matex_data, selection_count, busyname);

  for(lin = 1; lin <= num_lin; ++lin)
  {
    if (lin != skip_lin && selection[lin]=='1' && iupMatrixExIsLineVisible(matex_data->ih, lin))
    {
      iupMatrixExSetCellValue(matex_data->ih, lin, col, value);

      if (!iupMatrixExBusyInc(matex_data))
      {
        free(value);
        return;
      }
    }
  }

  iupMatrixExBusyEnd(matex_data);
  free(value);

  iupBaseCallValueChangedCb(matex_data->ih);
}

static void iMatrixExCopyColToSetData(ImatExData* matex_data, int lin, int col, int lin1, int lin2, const char* busyname)
{
  int skip_lin = lin;
  /* Must duplicate the memory to ensure it will be valid for all iterations */
  char* value = iupStrDup(iupMatrixExGetCellValue(matex_data->ih, lin, col, 0));  /* get internal value */

  iupMatrixExBusyStart(matex_data, lin2-lin1+1, busyname);

  for(lin = lin1; lin <= lin2; ++lin)
  {
    if (lin != skip_lin && iupMatrixExIsLineVisible(matex_data->ih, lin))
    {
      iupMatrixExSetCellValue(matex_data->ih, lin, col, value);

      if (!iupMatrixExBusyInc(matex_data))
      {
        free(value);
        return;
      }
    }
  }

  iupMatrixExBusyEnd(matex_data);
  free(value);

  iupBaseCallValueChangedCb(matex_data->ih);
}

static int iMatrixExGetMarkedLines(Ihandle *ih, int num_lin, int num_col, int col, char* selection, int *selection_count)
{
  char *marked = IupGetAttribute(ih, "MARKED");
  if (!marked)  /* no marked cells */
    return 0;

  selection[0] = '0';
  *selection_count = 0;

  if (*marked == 'C')
    return 0;
  else if (*marked == 'L')
  {
    int lin;

    marked++;

    for (lin=1; lin<=num_lin; lin++)
    {
      if (iupMatrixExIsLineVisible(ih, lin))
      {
        if (marked[lin-1] == '1')
        {
          selection[lin] = '1';
          (*selection_count)++;
        }
        else
          selection[lin] = '0';
      }
      else
        selection[lin] = '0';
    }
  }
  else
  {
    int lin;

    for(lin = 1; lin <= num_lin; ++lin)
    {
      if (iupMatrixExIsLineVisible(ih, lin))
      {
        int pos = (lin-1) * num_col + (col-1);  /* marked array does not include titles */
        if (marked[pos] == '1')
        {
          selection[lin] = '1';
          (*selection_count)++;
        }
        else
          selection[lin] = '0';
      }
      else
        selection[lin] = '0';
    }
  }

  return 1;
}

static int iMatrixExStrGetInterval(const char* interval, int num_lin, char* selection, int *selection_count)
{
  int lin1, lin2, len, value_len, ret;
  char value[100];

  memset(selection, '0', num_lin+1);
  *selection_count = 0;

  len = (int)strlen(interval);

  do
  {
    const char* next_value = iupStrNextValue(interval, len, &value_len, ',');
    if (value_len)
    {
      memcpy(value, interval, value_len);
      value[value_len] = 0;

      ret = iupStrToIntInt(value, &lin1, &lin2, '-');
      if (ret == 1)
      {
        if (lin1<=0) lin1 = 1;
        if (lin1>num_lin) lin1 = num_lin;

        selection[lin1] = '1';
        (*selection_count)++;
      }
      else if (ret == 2)
      {
        int lin;

        iupMatrixExCheckLimitsOrder(&lin1, &lin2, 1, num_lin);

        for(lin = lin1; lin <= lin2; ++lin)
        {
          selection[lin] = '1';
          (*selection_count)++;
        }
      }
      else
        return 0;
    }

    interval = next_value;
    len -= value_len+1;
  } while (value_len);

  return 1;
}

static int iMatrixExSetCopyColToAttribId2(Ihandle *ih, int lin, int col, const char* value)
{
  ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
  int lin1, lin2;
  int num_lin, num_col;
  char* busyname;

  num_lin = IupGetInt(ih, "NUMLIN");
  num_col = IupGetInt(ih, "NUMCOL");

  if (col <= 0 ||
      col > num_col ||
      !iupMatrixExIsColumnVisible(ih, col))
    return 0;

  if (iupStrEqualNoCase(value, "ALL"))
  {
    busyname = "COPYCOLTO:ALL";
    lin1 = 1;
    lin2 = num_lin;
  }
  else if (iupStrEqualNoCase(value, "TOP"))
  {
    busyname = "COPYCOLTO:TOP";
    lin1 = 1;
    lin2 = lin-1;
  }
  else if (iupStrEqualNoCase(value, "BOTTOM"))
  {
    busyname = "COPYCOLTO:BOTTOM";
    lin1 = lin+1;
    lin2 = num_lin;
  }
  else if (iupStrEqualNoCase(value, "MARKED"))
  {
    int selection_count;
    char* selection = (char*)malloc(num_lin+1);

    iupAttribSet(ih, "LASTERROR", NULL);

    if (!iMatrixExGetMarkedLines(ih, num_lin, num_col, col, selection, &selection_count))
    {
      iupAttribSet(ih, "LASTERROR", "IUP_ERRORNOSELECTION");
      free(selection);
      return 0;
    }

    iMatrixExCopyColToSetDataSelected(matex_data, lin, col, num_lin, selection, selection_count, "COPYCOLTO:MARKED");

    free(selection);
    return 0;
  }
  else
  {
    int selection_count;
    char* selection = (char*)malloc(num_lin+1);

    iupAttribSet(ih, "LASTERROR", NULL);

    if (!iMatrixExStrGetInterval(value, num_lin, selection, &selection_count))
    {
      iupAttribSet(ih, "LASTERROR", "IUP_ERRORINVALIDINTERVAL");
      free(selection);
      return 0;
    }

    iMatrixExCopyColToSetDataSelected(matex_data, lin, col, num_lin, selection, selection_count, "COPYCOLTO:INTERVAL");

    free(selection);
    return 0;
  }

  iupMatrixExCheckLimitsOrder(&lin1, &lin2, 1, num_lin);

  iMatrixExCopyColToSetData(matex_data, lin, col, lin1, lin2, busyname);
  return 0;
}

void iupMatrixExRegisterCopy(Iclass* ic)
{
  iupClassRegisterAttributeId2(ic, "COPYCOLTO", NULL, iMatrixExSetCopyColToAttribId2, IUPAF_NO_INHERIT);
}
