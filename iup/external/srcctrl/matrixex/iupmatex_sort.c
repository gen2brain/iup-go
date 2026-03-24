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
#include "iupcontrols.h"

#include "iup_array.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_matrixex.h"


static void iMatrixExSortUpdateSelection(ImatExData* matex_data, int sort_col, int lin1, int lin2)
{
  char* markmode = IupGetAttribute(matex_data->ih, "MARKMODE");

  if (iupStrEqualNoCase(markmode, "COL") || iupStrEqualNoCase(markmode, "LINCOL"))
  {
    int col, num_col = IupGetInt(matex_data->ih, "NUMCOL");
    char* marked = malloc(num_col+2);

    marked[0] = 'C';
    for(col = 1; col <= num_col; ++col)
      marked[col] = '0';
    marked[col] = 0;

    marked[sort_col] = '1';

    IupSetStrAttribute(matex_data->ih, "MARKED", marked);
    IupSetAttribute(matex_data->ih, "REDRAW", "ALL");

    free(marked);
  }
  else if (iupStrEqualNoCase(markmode, "CELL"))
  {
    int num_col = IupGetInt(matex_data->ih, "NUMCOL");
    int num_lin = IupGetInt(matex_data->ih, "NUMLIN");
    int pos, lin, col, count = num_lin*num_col;   /* marked does not include titles */
    char* marked = malloc(count+1);

    for(pos = 0; pos < count; pos++)
    {
      lin = (pos / num_col) + 1;
      col = (pos % num_col) + 1;
      if (col == sort_col && lin >= lin1 && lin <= lin2)
        marked[pos] = '1';
      else
        marked[pos] = '0';
    }
    marked[pos] = 0;

    IupSetStrAttribute(matex_data->ih, "MARKED", marked);
    IupSetAttribute(matex_data->ih, "REDRAW", "ALL");

    free(marked);
  }
}

static int iMatrixExSortDialogSort_CB(Ihandle* ih_button)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_button, "MATRIX_EX_DATA");
  Ihandle* ih_matrix = IupGetDialogChild(ih_button, "INTERVAL");
  int col = IupGetIntId2(ih_matrix, "", 1, 1);

  int casesensitive = IupGetInt(IupGetDialogChild(ih_button, "CASESENSITIVE"), "VALUE");
  int ascending = IupGetInt(IupGetDialogChild(ih_button, "ASCENDING"), "VALUE");

  iupAttribSet(matex_data->ih, "SORTCOLUMNCASESENSITIVE", casesensitive? "Yes": "No");
  iupAttribSet(matex_data->ih, "SORTCOLUMNORDER", ascending? "ASCENDING": "DESCENDING");

  if (IupGetIntId2(ih_matrix, "TOGGLEVALUE", 2, 1))
    IupSetAttributeId(matex_data->ih, "SORTCOLUMN", col, "ALL");
  else
  {
    int lin1 = IupGetIntId2(ih_matrix, "", 3, 1);
    int lin2 = IupGetIntId2(ih_matrix, "", 4, 1);
    IupSetfAttributeId(matex_data->ih, "SORTCOLUMN", col, "%d-%d", lin1, lin2);
  }

  return IUP_DEFAULT;
}

static int iMatrixExSortDialogReset_CB(Ihandle* ih_button)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_button, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "SORTCOLUMN", "RESET");
  return IUP_DEFAULT;
}

static int iMatrixExSortDialogInvert_CB(Ihandle* ih_button)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_button, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "SORTCOLUMN", "INVERT");
  return IUP_DEFAULT;
}

static int iMatrixExSortDialogClose_CB(Ihandle* ih_button)
{
  (void)ih_button;
  return IUP_CLOSE;
}

static int iMatrixExSortToggleValue_CB(Ihandle *ih_matrix, int lin, int col, int status)
{
  if (lin!=2 || col!=1)
    return IUP_DEFAULT;

  if (status) /* All Lines? disable lin1 && lin2 */
  {
    ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_matrix, "MATRIX_EX_DATA");
    int num_lin = IupGetInt(matex_data->ih, "NUMLIN");
    IupSetStrAttributeId2(ih_matrix, "BGCOLOR", 3, 1, "220 220 220");
    IupSetStrAttributeId2(ih_matrix, "BGCOLOR", 4, 1, "220 220 220");
    IupSetIntId2(ih_matrix, "", 3, 1, 1);
    IupSetIntId2(ih_matrix, "", 4, 1, num_lin);
    iMatrixExSortUpdateSelection(matex_data, col, 1, num_lin);
  }
  else
  {
    IupSetStrAttributeId2(ih_matrix, "BGCOLOR", 3, 1, NULL);
    IupSetStrAttributeId2(ih_matrix, "BGCOLOR", 4, 1, NULL);
  }

  IupSetAttribute(ih_matrix, "REDRAW", "ALL");

  return IUP_DEFAULT;
}

static int iMatrixExSortDropCheck_CB(Ihandle *ih_matrix, int lin, int col)
{
  (void)ih_matrix;

  if (lin==2 && col==1)
    return IUP_CONTINUE;

  return IUP_IGNORE;
}

static int iMatrixExSortEdition_CB(Ihandle *ih_matrix, int lin, int col, int mode, int update)
{
  if (mode==1)
  {
    if (lin==2 && col==1)
      return IUP_IGNORE;
    else if ((lin==3 && col==1) ||
             (lin==4 && col==1))
    {
      if (IupGetIntId2(ih_matrix, "TOGGLEVALUE", 2, 1))
        return IUP_IGNORE;
    }
  }
  else /* mode==0 */
  {
    if (lin==1 && col==1)
    {
      ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_matrix, "MATRIX_EX_DATA");
      int num_col = IupGetInt(matex_data->ih, "NUMCOL");
      col = IupGetInt(ih_matrix, "VALUE");
      if (col < 1 || col > num_col)
        return IUP_IGNORE;
      else
      {
        int lin1 = IupGetIntId2(ih_matrix, "", 3, 1);
        int lin2 = IupGetIntId2(ih_matrix, "", 4, 1);
        iMatrixExSortUpdateSelection(matex_data, col, lin1, lin2);
      }
    }
    else if ((lin==3 && col==1) ||
             (lin==4 && col==1))
    {
      int lin1, lin2;
      ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_matrix, "MATRIX_EX_DATA");
      int num_lin = IupGetInt(matex_data->ih, "NUMLIN");
      if (lin==3)
        lin1 = IupGetInt(ih_matrix, "VALUE");
      else
        lin1 = IupGetIntId2(ih_matrix, "", 3, 1);
      if (lin==4)
        lin2 = IupGetInt(ih_matrix, "VALUE");
      else
        lin2 = IupGetIntId2(ih_matrix, "", 4, 1);
      if (lin1 < 1 || lin1 > lin2 || lin2 > num_lin)
        return IUP_IGNORE;
      else
      {
        col = IupGetIntId2(ih_matrix, "", 1, 1);
        iMatrixExSortUpdateSelection(matex_data, col, lin1, lin2);
      }
    }
  }

  (void)update;
  return IUP_DEFAULT;
}

static Ihandle* iMatrixExSortCreateDialog(ImatExData* matex_data)
{
  Ihandle *ih_matrix, *matrix_box, *options_box, *reset, *sort, *invert,
          *dlg, *close, *dlg_box, *button_box, *parent;

  ih_matrix = IupSetAttributes(IupMatrix(NULL),
    "NUMLIN=4, "
    "NUMCOL=1, "
    "NUMLIN_VISIBLE=4, "
    "NUMCOL_VISIBLE=1, "
    "ALIGNMENT0=ALEFT, "
    "ALIGNMENT1=ARIGHT, "
    "SCROLLBAR=NO, "
    "USETITLESIZE=Yes, "
    "NAME=INTERVAL, "
    "MASK1:1=/d+, "
    "MASK3:1=/d+, "
    "MASK4:1=/d+, "
    "HEIGHT0=0, "
    "WIDTH1=40");
  IupSetStrAttributeId2(ih_matrix, "", 1, 0, "_@IUP_COLUMN");
  IupSetStrAttributeId2(ih_matrix, "", 2, 0, "_@IUP_ALLLINES");
  IupSetStrAttributeId2(ih_matrix, "", 3, 0, "_@IUP_FIRSTLINE");
  IupSetStrAttributeId2(ih_matrix, "", 4, 0, "_@IUP_LASTLINE");
  IupSetCallback(ih_matrix, "TOGGLEVALUE_CB", (Icallback)iMatrixExSortToggleValue_CB);
  IupSetCallback(ih_matrix, "DROPCHECK_CB", (Icallback)iMatrixExSortDropCheck_CB);
  IupSetCallback(ih_matrix, "EDITION_CB", (Icallback)iMatrixExSortEdition_CB);

  matrix_box = IupVbox(
      ih_matrix,
    NULL);

  options_box = IupVbox(
    IupSetAttributes(IupFrame(IupRadio(IupVbox(
      IupSetAttributes(IupToggle("_@IUP_ASCENDING", NULL), "NAME=ASCENDING"),
      IupSetAttributes(IupToggle("_@IUP_DESCENDING", NULL), "NAME=DESCENDING"),
      NULL))), "TITLE=_@IUP_ORDER, GAP=5, MARGIN=5x5"),
    IupSetAttributes(IupToggle("_@IUP_CASESENSITIVE", NULL), "NAME=CASESENSITIVE"),
    NULL);
  IupSetAttribute(options_box,"GAP","10");

  sort = IupButton("_@IUP_SORT", NULL);
  IupSetStrAttribute(sort, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(sort, "ACTION", (Icallback)iMatrixExSortDialogSort_CB);

  invert = IupButton("_@IUP_INVERT", NULL);
  IupSetStrAttribute(invert, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(invert, "ACTION", (Icallback)iMatrixExSortDialogInvert_CB);
  IupSetStrAttribute(invert,"TIP" ,"_@IUP_INVERT_TIP");

  reset = IupButton("_@IUP_RESET", NULL);
  IupSetStrAttribute(reset, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(reset, "ACTION", (Icallback)iMatrixExSortDialogReset_CB);

  close = IupButton("_@IUP_CLOSE", NULL);
  IupSetStrAttribute(close, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(close, "ACTION", (Icallback)iMatrixExSortDialogClose_CB);

  button_box = IupHbox(
    IupFill(),
    sort,
    invert,
    reset,
    close,
    NULL);
  IupSetAttribute(button_box,"MARGIN","0x0");
  IupSetAttribute(button_box, "NORMALIZESIZE", "HORIZONTAL");
  IupSetAttribute(button_box,"GAP","5");

  dlg_box = IupVbox(
    IupSetAttributes(IupHbox(
      matrix_box,
      options_box,
      NULL), "MARGIN=0x0, GAP=15"),
    button_box,
    NULL);
  IupSetAttribute(dlg_box,"MARGIN","10x10");
  IupSetAttribute(dlg_box,"GAP","15");

  dlg = IupDialog(dlg_box);

  parent = IupGetDialog(matex_data->ih);

  IupSetStrAttribute(dlg,"TITLE","_@IUP_SORTBYCOLUMN");
  IupSetAttribute(dlg,"MINBOX","NO");
  IupSetAttribute(dlg,"MAXBOX","NO");
  IupSetAttribute(dlg,"BORDER","NO");
  IupSetAttribute(dlg,"RESIZE","NO");
  IupSetAttribute(dlg, "TOOLBOX","YES");
  IupSetAttributeHandle(dlg,"DEFAULTENTER", sort);
  IupSetAttributeHandle(dlg,"DEFAULTESC", close);
  IupSetAttributeHandle(dlg,"PARENTDIALOG", parent);

  IupSetAttribute(dlg, "MATRIX_EX_DATA", (char*)matex_data);  /* do not use "_IUP_MATEX_DATA" to enable inheritance */

  if (IupGetAttribute(parent, "ICON"))
    IupSetStrAttribute(dlg,"ICON", IupGetAttribute(parent, "ICON"));
  else
    IupSetStrAttribute(dlg,"ICON", IupGetGlobal("ICON"));

  return dlg;
}

void iupMatrixExSortShowDialog(ImatExData* matex_data)
{
  int x, y, col, lin1, lin2, num_lin;
  Ihandle* ih_matrix;
  Ihandle* dlg_sort = iMatrixExSortCreateDialog(matex_data);
           
  IupSetStrAttribute(IupGetDialogChild(dlg_sort, "CASESENSITIVE"), "VALUE", iupAttribGetStr(matex_data->ih, "SORTCOLUMNCASESENSITIVE"));

  if (iupStrEqualNoCase(iupAttribGetStr(matex_data->ih, "SORTCOLUMNORDER"), "DESCENDING"))
    IupSetAttribute(IupGetDialogChild(dlg_sort, "DESCENDING"), "VALUE", "Yes");
  else
    IupSetAttribute(IupGetDialogChild(dlg_sort, "ASCENDING"), "VALUE", "Yes");

  ih_matrix = IupGetDialogChild(dlg_sort, "INTERVAL");

  col = IupGetInt2(matex_data->ih, "FOCUSCELL");
  IupSetIntId2(ih_matrix, "", 1, 1, col);
  num_lin = IupGetInt(matex_data->ih, "NUMLIN");
  lin1 = 1;
  lin2 = num_lin;
  IupGetIntInt(matex_data->ih, "SORTCOLUMNINTERVAL", &lin1, &lin2);
  if (lin1 < 1) lin1 = 1;
  if (lin2 > num_lin) lin2 = num_lin;
  if (lin1 > lin2) lin1 = lin2;
  if (lin1==1 && lin2==num_lin)
  {
    IupSetStrAttributeId2(ih_matrix, "TOGGLEVALUE", 2, 1, "Yes");
    IupSetStrAttributeId2(ih_matrix, "BGCOLOR", 3, 1, "220 220 220");
    IupSetStrAttributeId2(ih_matrix, "BGCOLOR", 4, 1, "220 220 220");
  }
  else
    IupSetStrAttributeId2(ih_matrix, "TOGGLEVALUE", 2, 1, "No");
  IupSetIntId2(ih_matrix, "", 3, 1, lin1);
  IupSetIntId2(ih_matrix, "", 4, 1, lin2);
  iMatrixExSortUpdateSelection(matex_data, col, lin1, lin2);
  
  iupMatrixExGetDialogPosition(matex_data, &x, &y);
  IupPopup(dlg_sort, x, y);
  IupDestroy(dlg_sort);
}

void iupMatrixExRegisterSort(Iclass* ic)
{
  /* Defined in IupMatrix - Exported
    SORTCOLUMN   (RESET, INVERT, lin1-lin2)
    SORTCOLUMNORDER  (ASCENDING, DESCENDING)
    SORTCOLUMNCASESENSITIVE (Yes, No) */

  (void)ic;
}
