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
#include "iup_stdcontrols.h"
#include "iup_controls.h"
#include "iup_register.h"

#include "iupmat_def.h"
#include "iupmat_getset.h"
#include "iupmat_draw.h"
#include "iupmat_aux.h"


/* Exported to IupMatrixEx */
char* iupMatrixExGetCellValue(Ihandle* ih, int lin, int col, int display)
{
  if (display)
    return iupMatrixGetValueDisplay(ih, lin, col);  /* Display value */
  else
    return iupMatrixGetValue(ih, lin, col);  /* Internal value (Maximum Precision) */
}

/* Exported to IupMatrixEx */
void iupMatrixExSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{  
  iupMatrixModifyValue(ih, lin, col, value);
}

static int iMatrixInitNumericColumns(Ihandle* ih, int col)
{
  if (!iupMATRIX_CHECK_COL(ih, col))
    return 0;

  if (!ih->data->numeric_columns)
    ih->data->numeric_columns = (ImatNumericData*)calloc(ih->data->columns.num_alloc, sizeof(ImatNumericData));

  return 1;
}

static int iMatrixSetNumericFlag(Ihandle* ih, int col, unsigned char attr, int set)
{
  if (!iMatrixInitNumericColumns(ih, col))
    return 0;

  if (set)
    ih->data->numeric_columns[col].flags |= attr;
  else
    ih->data->numeric_columns[col].flags &= ~attr;

  return 1;
}

static int iMatrixSetNumericQuantityIndexAttrib(Ihandle* ih, int col, const char* value)
{
  int quantity;
  int set = iupStrToInt(value, &quantity);  /* if NULL or fail will unset */
  if (iMatrixSetNumericFlag(ih, col, IMAT_IS_NUMERIC, set) && set)
  {
    /* no need to check for a valid quantity since it is done by the actual unit implementation */
    ih->data->numeric_columns[col].quantity = (unsigned char)quantity;
    ih->data->numeric_convert_func = (ImatNumericConvertFunc)IupGetCallback(ih, "NUMERICCONVERT_FUNC");  /* valid only during set of NUMERICQUANTITY */
    return 1;
  }
  return 0;
}

static int iMatrixSetNumericFormatAttrib(Ihandle* ih, int col, const char* value)
{
  return iMatrixSetNumericFlag(ih, col, IMAT_HAS_FORMAT, value!=NULL);
}

static int iMatrixSetNumericFormatPrecisionAttrib(Ihandle* ih, int col, const char* value)
{
  int precision;
  if (iupStrToInt(value, &precision))
  {
    if (col == IUP_INVALID_ID)
      IupSetStrf(ih, "NUMERICFORMATDEF", "%%.%df", precision);
    else
      IupSetStrfId(ih, "NUMERICFORMAT", col, "%%.%df", precision);
  }
  else
  {
    if (col == IUP_INVALID_ID)
      IupSetAttribute(ih, "NUMERICFORMATDEF", NULL);
    else
      IupSetAttributeId(ih, "NUMERICFORMAT", col, NULL);
  }
  return 0;
}

static char* iMatrixGetNumericDecimalSymbolAttrib(Ihandle* ih)
{
  char* value = iupAttribGet(ih, "NUMERICDECIMALSYMBOL");
  if (!value)
    value = IupGetGlobal("DEFAULTDECIMALSYMBOL");
  return value;
}

static char* iMatrixGetNumericFormatPrecisionAttrib(Ihandle* ih, int col)
{
  int precision;
  char* value = iupAttribGetId(ih, "NUMERICFORMAT", col);
  if (!value)
    value = iupMatrixGetNumericFormatDef(ih);

  if (!value)
    return NULL;

  precision = iupStrGetFormatPrecision(value);
  if (precision == -1)
    return NULL;

  return iupStrReturnInt(precision);
}

static int iMatrixSetNumericFormatTitleAttrib(Ihandle* ih, int col, const char* value)
{
  return iMatrixSetNumericFlag(ih, col, IMAT_HAS_FORMATTITLE, value!=NULL);
}

static int iMatrixSetNumericUnitIndexAttrib(Ihandle* ih, int col, const char* value)
{
  int unit;
  if (iupStrToInt(value, &unit))
  {
    if (!iMatrixInitNumericColumns(ih, col))
      return 0;

    /* no need to check for a valid unit since it is done by the actual unit implementation */
    ih->data->numeric_columns[col].unit = (unsigned char)unit;
    return 1;
  }
  return 0;
}

static int iMatrixSetNumericUnitShownIndexAttrib(Ihandle* ih, int col, const char* value)
{
  int unit_shown;
  if (iupStrToInt(value, &unit_shown))
  {
    if (!iMatrixInitNumericColumns(ih, col))
      return 0;

    /* no need to check for a valid unit since it is done by the actual unit implementation */
    ih->data->numeric_columns[col].unit_shown = (unsigned char)unit_shown;
    return 1;
  }
  return 0;
}

static IFniii   iMatrixQSort_sort_cb = NULL;
static Ihandle* iMatrixQSort_ih = NULL;
static int      iMatrixQSort_col = 0;

static int iMatrixCompareCallbackFunc(const void* elem1, const void* elem2)
{
  int lin1 = *((int*)elem1);
  int lin2 = *((int*)elem2);
  return iMatrixQSort_sort_cb(iMatrixQSort_ih, iMatrixQSort_col, lin1, lin2);
}

typedef struct _ImatSortNumber {
  int lin;
  double number;
} ImatSortNumber;

static int iMatrixCompareNumberFunc(const void* elem1, const void* elem2)
{
  ImatSortNumber* num1 = (ImatSortNumber*)elem1;
  ImatSortNumber* num2 = (ImatSortNumber*)elem2;
  if (num1->number < num2->number)
    return -1;
  if (num1->number > num2->number)
    return 1;
  return 0;
}

static int iMatrixQSort_utf8 = 0;
static int iMatrixQSort_casesensitive = 1;

typedef struct _ImatSortText {
  int lin;
  char* text;
} ImatSortText;

static int iMatrixCompareTextFunc(const void* elem1, const void* elem2)
{
  ImatSortText* txt1 = (ImatSortText*)elem1;
  ImatSortText* txt2 = (ImatSortText*)elem2;
  if ((!txt1->text || txt1->text[0]==0) && (!txt2->text || txt2->text[0]==0)) return 0;
  if (!txt1->text || txt1->text[0]==0) return -1;    /* txt1 < txt2 => -1 */
  if (!txt2->text || txt2->text[0]==0) return 1;
  return iupStrCompare(txt1->text, txt2->text, iMatrixQSort_casesensitive, iMatrixQSort_utf8);
}

static int iMatrixSetSortColumnAttrib(Ihandle* ih, int col, const char* value)
{
  int lines_num = ih->data->lines.num;
  int lin, lin1=1, lin2=lines_num-1;   /* ALL */
  int ascending;
  int* sort_line_index;
  IFniii sort_cb;

  /* Notice that sort_line_index[0] is always 0 */

  if (!ih->data->sort_line_index)
    ih->data->sort_line_index = (int*)calloc(ih->data->lines.num_alloc, sizeof(int));

  sort_line_index = ih->data->sort_line_index;

  if (!ih->data->sort_has_index)
  {
    for (lin = 0; lin < lines_num; lin++)
      sort_line_index[lin] = lin;
  }

  if (iupStrEqualNoCase(value, "RESET"))
  {
    ih->data->sort_has_index = 0;
    iupAttribSetId(ih, "SORTSIGN", ih->data->last_sort_col, NULL);
    ih->data->last_sort_col = 0;

    iupMatrixDraw(ih, 1);
    iupAttribSet(ih, "SORTCOLUMNINTERVAL", NULL);
    return 0;
  }

  if (iupStrEqualNoCase(value, "INVERT"))
  {
    int l1, l2;

    if (!ih->data->sort_has_index)
      return 0;

    IupGetIntInt(ih, "SORTCOLUMNINTERVAL", &lin1, &lin2);

    for (l1=lin1,l2=lin2; l1<l2; ++l1,--l2)
    {
      int tmp = sort_line_index[l1];
      sort_line_index[l1] = sort_line_index[l2];
      sort_line_index[l2] = tmp;
    }

    if (iupStrEqualNoCase(iupAttribGetId(ih, "SORTSIGN", ih->data->last_sort_col), "UP"))
      iupAttribSetId(ih, "SORTSIGN", ih->data->last_sort_col, "DOWN");
    else
      iupAttribSetId(ih, "SORTSIGN", ih->data->last_sort_col, "UP");

    iupMatrixDraw(ih, 1);
    return 0;
  }

  if (!iupMATRIX_CHECK_COL(ih, col))
    return 0;

  if (!iupStrEqualNoCase(value, "ALL"))
    iupStrToIntInt(value, &lin1, &lin2, '-');

  iupAttribSetStrf(ih, "SORTCOLUMNINTERVAL", "%d,%d", lin1, lin2);

  if (lin1 < 1) lin1 = 1;
  if (lin2 < lin1) lin2 = lin1;

  ascending = iupStrEqualNoCase(iupAttribGetStr(ih, "SORTCOLUMNORDER"), "ASCENDING");
  
  sort_cb = (IFniii)IupGetCallback(ih, "SORTCOLUMNCOMPARE_CB");
  if (sort_cb)
  {
    iMatrixQSort_sort_cb = sort_cb;
    iMatrixQSort_ih = ih;
    iMatrixQSort_col = col;
    qsort(sort_line_index+lin1,lin2-lin1+1,sizeof(int),iMatrixCompareCallbackFunc);
  }
  else
  {
    if (ih->data->numeric_columns && ih->data->numeric_columns[col].flags & IMAT_IS_NUMERIC)
    {
      ImatSortNumber* sort_line_number = (ImatSortNumber*)malloc((lin2-lin1+1)*sizeof(ImatSortNumber));

      for (lin=lin1; lin<=lin2; lin++)
      {
        sort_line_number[lin-lin1].lin = sort_line_index[lin];
        sort_line_number[lin-lin1].number = iupMatrixGetValueNumeric(ih, lin, col);
      }

      qsort(sort_line_number,lin2-lin1+1,sizeof(ImatSortNumber), iMatrixCompareNumberFunc);

      for (lin=lin1; lin<=lin2; lin++)
      {
        if (ascending)
          sort_line_index[lin] = sort_line_number[lin-lin1].lin;
        else
          sort_line_index[lin2 - (lin-lin1)] = sort_line_number[lin-lin1].lin;
      }

      free(sort_line_number);
    }
    else
    {
      ImatSortText* sort_line_text = (ImatSortText*)malloc((lin2-lin1+1)*sizeof(ImatSortText));

      for (lin=lin1; lin<=lin2; lin++)
      {
        sort_line_text[lin-lin1].lin = sort_line_index[lin];
        sort_line_text[lin-lin1].text = iupMatrixGetValueDisplay(ih, lin, col);

        if (ih->data->callback_mode)
          sort_line_text[lin-lin1].text = iupStrDup(sort_line_text[lin-lin1].text);
      }

      iMatrixQSort_utf8 = IupGetInt(NULL, "UTF8MODE");
      iMatrixQSort_casesensitive = iupAttribGetInt(ih, "SORTCOLUMNCASESENSITIVE");
      qsort(sort_line_text,lin2-lin1+1,sizeof(ImatSortText), iMatrixCompareTextFunc);

      for (lin=lin1; lin<=lin2; lin++)
      {
        if (ascending)
          sort_line_index[lin] = sort_line_text[lin-lin1].lin;
        else
          sort_line_index[lin2 - (lin-lin1)] = sort_line_text[lin-lin1].lin;

        if (ih->data->callback_mode && sort_line_text[lin-lin1].text)
          free(sort_line_text[lin-lin1].text);
      }

      free(sort_line_text);
    }
  }

  iupAttribSetId(ih, "SORTSIGN", ih->data->last_sort_col, NULL);
  if (ascending)
    iupAttribSetId(ih, "SORTSIGN", col, "DOWN");
  else
    iupAttribSetId(ih, "SORTSIGN", col, "UP");

  ih->data->sort_has_index = 1;
  ih->data->last_sort_col = col;
  iupMatrixDraw(ih, 1);
  return 0;
}

static char* iMatrixGetLastSortColumnAttrib(Ihandle* ih)
{
  return iupStrReturnInt(ih->data->last_sort_col);
}

static char* iMatrixGetSortLineIndexAttrib(Ihandle* ih, int lin)
{
  if (lin > 0 && lin < ih->data->lines.num && ih->data->sort_has_index)
    return iupStrReturnInt(ih->data->sort_line_index[lin]);
  else
    return NULL;
}

static int iMatrixSetUndoRedoAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->undo_redo = 1;
  else 
    ih->data->undo_redo = 0;
  return 0;
}

static char* iMatrixGetUndoRedoAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->undo_redo); 
}

void iupMatrixRegisterEx(Iclass* ic)
{
  /* Undocumented features, will be exposed in IupMatrixEx */

  /* IupMatrixEx Attributes - Numeric Columns */
  iupClassRegisterAttributeId(ic, "NUMERICQUANTITYINDEX", NULL, iMatrixSetNumericQuantityIndexAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NUMERICFORMAT", NULL, iMatrixSetNumericFormatAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NUMERICFORMATPRECISION", iMatrixGetNumericFormatPrecisionAttrib, iMatrixSetNumericFormatPrecisionAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NUMERICFORMATTITLE", NULL, iMatrixSetNumericFormatTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NUMERICUNITINDEX", NULL, iMatrixSetNumericUnitIndexAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NUMERICUNITSHOWNINDEX", NULL, iMatrixSetNumericUnitShownIndexAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMERICFORMATDEF", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMERICDECIMALSYMBOL", iMatrixGetNumericDecimalSymbolAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterCallback(ic, "NUMERICGETVALUE_CB", "ii=d");
  iupClassRegisterCallback(ic, "NUMERICSETVALUE_CB", "iid");

  /* IupMatrixEx Attributes - Sort Columns */
  iupClassRegisterAttribute(ic, "LASTSORTCOLUMN", iMatrixGetLastSortColumnAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SORTCOLUMN", NULL, iMatrixSetSortColumnAttrib, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SORTCOLUMNORDER", NULL, NULL, IUPAF_SAMEASSYSTEM, "ASCENDING",  IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SORTCOLUMNCASESENSITIVE", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SORTCOLUMNINTERVAL", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SORTLINEINDEX", iMatrixGetSortLineIndexAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterCallback(ic, "SORTCOLUMNCOMPARE_CB", "iii");

  /* IupMatrixEx Attributes - Undo/Redo */
  iupClassRegisterAttribute(ic, "UNDOREDO", iMatrixGetUndoRedoAttrib, iMatrixSetUndoRedoAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
}

