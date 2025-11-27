/** \file
 * \brief IupMatrix Expansion Library.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>


#include "iup.h"
#include "iupcbs.h"
#include "iupcontrols.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_predialogs.h"

#include "iup_matrixex.h"


void iupMatrixExGetDialogPosition(ImatExData* matex_data, int *x, int *y)
{
  /* return a dialog position aligned with the bottom-right corner of the focus cell,
     and it will make sure that the focus cell is visible */
  int cx, cy, cw, ch, lin, col;
  char attrib[50];
  IupGetIntInt(matex_data->ih, "SCREENPOSITION", x, y);
  IupGetIntInt(matex_data->ih, "FOCUSCELL", &lin, &col);
  IupSetfAttribute(matex_data->ih,"SHOW", "%d:%d", lin, col);
  sprintf(attrib, "CELLOFFSET%d:%d", lin, col);
  IupGetIntInt(matex_data->ih, attrib, &cx, &cy);
  sprintf(attrib, "CELLSIZE%d:%d", lin, col);
  IupGetIntInt(matex_data->ih, attrib, &cw, &ch);
  *x += cx + cw;
  *y += cy + ch;
}

static void iMatrixListShowLastError(Ihandle* ih)
{
  char* lasterror = iupAttribGet(ih, "LASTERROR");
  if (lasterror)
    IupMessageError(IupGetDialog(ih), lasterror);
}

static void iMatrixExSelectAll(Ihandle *ih)
{
  char* markmode = IupGetAttribute(ih, "MARKMODE");

  if (iupStrEqualNoCasePartial(markmode, "LIN")) /* matches also "LINCOL" */
  {
    int lin, num_lin = IupGetInt(ih, "NUMLIN");
    char* marked = malloc(num_lin+2);

    marked[0] = 'L';
    for(lin = 1; lin <= num_lin; ++lin)
      marked[lin] = '1';
    marked[lin] = 0;

    IupSetStrAttribute(ih, "MARKED", marked);

    free(marked);
  }
  else if (iupStrEqualNoCase(markmode, "COL"))
  {
    int col, num_col = IupGetInt(ih, "NUMCOL");
    char* marked = malloc(num_col+2);

    marked[0] = 'C';
    for(col = 1; col <= num_col; ++col)
      marked[col] = '1';
    marked[col] = 0;

    IupSetStrAttribute(ih, "MARKED", marked);

    free(marked);
  }
  else if (iupStrEqualNoCase(markmode, "CELL"))
  {
    int num_col = IupGetInt(ih, "NUMCOL");
    int num_lin = IupGetInt(ih, "NUMLIN");
    int pos, count = num_lin*num_col;
    char* marked = malloc(count+1);

    for(pos = 0; pos < count; pos++)
      marked[pos] = '1';
    marked[pos] = 0;

    IupSetStrAttribute(ih, "MARKED", marked);

    free(marked);
  }
}

void iupMatrixExCheckLimitsOrder(int *v1, int *v2, int min, int max)
{
  if (*v1<min) *v1 = min;
  if (*v2<min) *v2 = min;
  if (*v1>max) *v1 = max;
  if (*v2>max) *v2 = max;
  if (*v1>*v2) {int v=*v1; *v1=*v2; *v2=v;}
}

static int iMatrixExSetFreezeAttrib(Ihandle *ih, const char* value)
{
  int freeze, lin, col;
  int flin, fcol;

  if (iupStrBoolean(value))
  {
    IupGetIntInt(ih, "FOCUSCELL", &lin, &col);
    freeze = 1;
  }
  else
  {
    if (iupStrToIntInt(value, &lin, &col, ':')==2)
      freeze = 1;
    else
      freeze = 0;
  }

  /* clear the previous freeze first */
  flin = IupGetInt(ih,"NUMLIN_NOSCROLL");
  fcol = IupGetInt(ih,"NUMCOL_NOSCROLL");
  if (flin) IupSetAttributeId2(ih,"FRAMEHORIZCOLOR", flin, IUP_INVALID_ID, NULL);
  if (fcol) IupSetAttributeId2(ih,"FRAMEVERTCOLOR", IUP_INVALID_ID, fcol, NULL);

  if (!freeze)
  {
    IupSetAttribute(ih,"NUMLIN_NOSCROLL","0");
    IupSetAttribute(ih,"NUMCOL_NOSCROLL","0");
    IupSetAttribute(ih,"SHOW","1:1");
  }
  else
  {
    char* fzcolor = IupGetAttribute(ih, "FREEZECOLOR");

    IupSetInt(ih,"NUMLIN_NOSCROLL",lin);
    IupSetInt(ih,"NUMCOL_NOSCROLL",col);  

    IupSetStrAttributeId2(ih,"FRAMEHORIZCOLOR", lin, IUP_INVALID_ID, fzcolor);
    IupSetStrAttributeId2(ih,"FRAMEVERTCOLOR",IUP_INVALID_ID, col, fzcolor);
  }

  IupSetAttribute(ih, "REDRAW", "ALL");
  return 1;  /* store freeze state */
}

static char* iMatrixExFileDlg(ImatExData* matex_data, int save, const char* title, const char* filter, const char* info, const char* extfilter)
{
  Ihandle* dlg = IupFileDlg();

  char* last_filename = IupGetAttribute(matex_data->ih, "LASTFILENAME");
  if (last_filename)
    IupSetStrAttribute(dlg, "FILE", last_filename);

  IupSetAttribute(dlg,"DIALOGTYPE", save? "SAVE": "OPEN");
  IupSetStrAttribute(dlg, "TITLE", title);
  IupSetStrAttribute(dlg,"FILTER", filter);
  IupSetStrAttribute(dlg,"FILTERINFO", info);
  IupSetStrAttribute(dlg,"EXTFILTER", extfilter);  /* Windows and GTK only, but more flexible */
  IupSetStrAttribute(dlg, "DIRECTORY", iupAttribGet(matex_data->ih, "FILEDIRECTORY"));
  IupSetAttributeHandle(dlg, "PARENTDIALOG", IupGetDialog(matex_data->ih));

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  if (IupGetInt(dlg,"STATUS")!=-1)
  {
    char* value = IupGetAttribute(dlg, "VALUE");
    value = iupStrReturnStr(value);
    IupDestroy(dlg);
    return value;
  }

  IupDestroy(dlg);
  return NULL;
}

static int iMatrixExItemExport_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  char *filter, *info, *extfilter, *filename;

  if (iupStrEqual(IupGetAttribute(ih_item, "FILEFORMAT"), "LaTeX"))
  {
    filter = "*.tex";  /* Motif does not support EXTFILTER, so define both */
    info = "LaTeX file (table format)";
    extfilter = "LaTeX file (table format)|*.tex|All Files|*.*|";
  }
  else if (iupStrEqual(IupGetAttribute(ih_item, "FILEFORMAT"), "HTML"))
  {
    filter = "*.html;*.htm";
    info = "HTML file (table format)";
    extfilter = "HTML file (table format)|*.html;*.htm|All Files|*.*|";
  }
  else
  {
    filter = "*.txt";
    info = "Text file";
    extfilter = "Text file|*.txt;*.csv|All Files|*.*|";
  }

  filename = iMatrixExFileDlg(matex_data, 1, "_@IUP_EXPORT", filter, info, extfilter);
  if (filename)
  {
    IupSetStrAttribute(matex_data->ih, "FILEFORMAT", IupGetAttribute(ih_item, "FILEFORMAT"));
    IupSetStrAttribute(matex_data->ih, "COPYFILE", filename);
    IupSetStrAttribute(matex_data->ih, "LASTFILENAME", filename);

    iMatrixListShowLastError(matex_data->ih);
  }
  else
    IupSetAttribute(matex_data->ih, "LASTFILENAME", NULL);

  return IUP_DEFAULT;
}

static int iMatrixExItemImport_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  char *filter, *info, *extfilter, *filename;

  filter = "*.txt";
  info = "Text file";
  extfilter = "Text file|*.txt|All Files|*.*|";

  filename = iMatrixExFileDlg(matex_data, 0, "_@IUP_IMPORT", filter, info, extfilter);
  if (filename)
  {
    IupSetStrAttribute(matex_data->ih, "PASTEFILE", filename);
    IupSetStrAttribute(matex_data->ih, "LASTFILENAME", filename);

    iMatrixListShowLastError(matex_data->ih);
  }
  else
    IupSetAttribute(matex_data->ih, "LASTFILENAME", NULL);

  return IUP_DEFAULT;
}

static int setparent_param_cb(Ihandle* param_dialog, int param_index, void* user_data)
{
  if (param_index == IUP_GETPARAM_MAP)
  {
    Ihandle* ih = (Ihandle*)user_data;
    IupSetAttributeHandle(param_dialog, "PARENTDIALOG", ih);
  }

  return 1;
}

static int iMatrixExItemSettings_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int sep_index = 0, decimal_sep_index = 0, decimal_sep_old, decimals, decimals_old;
  char sep_other[5] = "", *decimal_symbol;

  char sep = *(IupGetAttribute(matex_data->ih, "TEXTSEPARATOR"));
  if (sep == ';') sep_index = 1;
  else if (sep == ' ') sep_index = 2;

  decimal_symbol = IupGetAttribute(matex_data->ih, "NUMERICDECIMALSYMBOL");  /* this will also check for global "DEFAULTDECIMALSYMBOL" */
  if (decimal_symbol)
  {
    if (decimal_symbol[0] == ',')  /* else is '.' */
      decimal_sep_index = 1;
  }
  else  /* if not defined get from system */
  {
    struct lconv * locale_info = localeconv();
    if (locale_info->decimal_point[0] == ',')  /* else is '.' */
      decimal_sep_index = 1;
  }
  decimal_sep_old = decimal_sep_index;

  decimals = IupGetInt(matex_data->ih, "NUMERICFORMATPRECISION");  /* get the value for the whole matrix */
  decimals_old = decimals;

  if (IupGetParam("_@IUP_SETTINGSDLG", setparent_param_cb, IupGetDialog(matex_data->ih),
                  "_@IUP_TEXTSEPARATOR%l|Tab|\";\"|\" \"|\n"
                  "_@IUP_OTHERTEXTSEPARATOR%s[^0-9]\n"
                  "_@IUP_DECIMALS%i[0]\n"
                  "_@IUP_DECIMALSYMBOL%l|.|,|\n",
                  &sep_index, sep_other, &decimals, &decimal_sep_index, NULL))
  {
    if (sep_other[0] != 0)
      IupSetStrAttribute(matex_data->ih, "TEXTSEPARATOR", sep_other);
    else
    {
      const char* sep_str[] = { "\t", ";", " " };
      IupSetStrAttribute(matex_data->ih, "TEXTSEPARATOR", sep_str[sep_index]);
    }

    /* avoid changing the application defined locale if not changed */
    if (decimal_sep_old != decimal_sep_index)
    {
      const char* decimal_str[] = { ".", "," };
      IupSetAttribute(matex_data->ih, "NUMERICDECIMALSYMBOL", decimal_str[decimal_sep_index]);
    }

    if (decimals_old != decimals)
      IupSetInt(matex_data->ih, "NUMERICFORMATPRECISION", decimals);

    IupSetAttribute(matex_data->ih, "REDRAW", "ALL");
  }

  return IUP_DEFAULT;
}

static int iMatrixExItemCut_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "COPY", "MARKED");
  IupSetAttribute(matex_data->ih, "CLEARVALUE", "MARKED");
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemCopy_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "COPY", "MARKED");
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemPaste_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  if (IupGetAttribute(matex_data->ih, "MARKED"))
    IupSetAttribute(matex_data->ih, "PASTE", "MARKED");
  else
    IupSetAttribute(matex_data->ih, "PASTE", "FOCUS");
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemDel_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "CLEARVALUE", "MARKED");
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemSelectAll_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  iMatrixExSelectAll(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExFindLin(Ihandle* ih, const char* line);

static int iMatrixExItemCopyColTo_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  char* value = IupGetAttribute(ih_item, "COPYCOLTO");
  int lin, col;

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);
  IupSetfAttribute(matex_data->ih, "FOCUSCELL", "%d:%d", lin, col);

  if (iupStrEqual(value, "INTERVAL"))
  {
    if (iupAttribGetInt(matex_data->ih, "CELLBYTITLE"))
    {
      char line1[50] = "";
      char line2[50] = "";

      char* last_lin1 = iupAttribGet(matex_data->ih, "_IUP_LAST_COPYTO_LIN1");
      char* last_lin2 = iupAttribGet(matex_data->ih, "_IUP_LAST_COPYTO_LIN2");
      iupStrCopyN(line1, 50, last_lin1);
      iupStrCopyN(line2, 50, last_lin2);

      if (IupGetParam("_@IUP_COPYTOINTERVAL", setparent_param_cb, IupGetDialog(matex_data->ih), "_@IUP_LINESTART%s\n_@IUP_LINEEND%s\n", line1, line2, NULL))
      {
        int lin1 = iMatrixExFindLin(matex_data->ih, line1);
        int lin2 = iMatrixExFindLin(matex_data->ih, line2);

        IupSetStrfId2(matex_data->ih, "COPYCOLTO", lin, col, "%d-%d", lin1, lin2);

        iupAttribSetStr(matex_data->ih, "_IUP_LAST_COPYTO_LIN1", line1);
        iupAttribSetStr(matex_data->ih, "_IUP_LAST_COPYTO_LIN2", line2);
      }
    }
    else
    {
      int lin1 = iupAttribGetInt(matex_data->ih, "_IUP_LAST_COPYTO_LIN1");
      int lin2 = iupAttribGetInt(matex_data->ih, "_IUP_LAST_COPYTO_LIN2");

      if (IupGetParam("_@IUP_COPYTOINTERVAL", setparent_param_cb, IupGetDialog(matex_data->ih), "_@IUP_LINESTART%i[1,,]\n_@IUP_LINEEND%i[1,,]\n", &lin1, &lin2, NULL))
      {
        IupSetStrfId2(matex_data->ih, "COPYCOLTO", lin, col, "%d-%d", lin1, lin2);

        iupAttribSetInt(matex_data->ih, "_IUP_LAST_COPYTO_LIN1", lin1);
        iupAttribSetInt(matex_data->ih, "_IUP_LAST_COPYTO_LIN2", lin2);
      }
    }
  }
  else
    IupSetStrAttributeId2(matex_data->ih, "COPYCOLTO", lin, col, value);

  return IUP_DEFAULT;
}

static int iMatrixExItemUndo_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "UNDO", NULL);  /* 1 level */
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemRedo_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetAttribute(matex_data->ih, "REDO", NULL);  /* 1 level */
  iMatrixListShowLastError(matex_data->ih);
  return IUP_DEFAULT;
}

static int iMatrixExItemUndoList_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  if (!matex_data)  /* will be called also by the shortcut key */
   matex_data = (ImatExData*)iupAttribGet(ih_item, "_IUP_MATEX_DATA");
  iupMatrixExUndoShowDialog(matex_data);
  return IUP_DEFAULT;
}

static int iMatrixExItemFind_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetStrAttribute(matex_data->ih, "FOCUSCELL", IupGetAttribute(ih_item, "MENUCONTEXT_CELL"));
  iupMatrixExFindShowDialog(matex_data);
  return IUP_DEFAULT;
}

static int iMatrixExFindLin(Ihandle* ih, const char* line)
{
  int lin, num_lin = IupGetInt(ih, "NUMLIN");

  for (lin = 1; lin <= num_lin; lin++)
  {
    char* value = iupMatrixExGetCellValue(ih, lin, 0, 1);  /* get displayed value */
    if (value && value[0] != 0)
    {
      if (iupStrEqual(value, line))
        return lin;
    }
  }

  return 1;
}

static int iMatrixExFindCol(Ihandle* ih, const char* column)
{
  int col, num_col = IupGetInt(ih, "NUMCOL");

  for (col = 1; col <= num_col; col++)
  {
    char* value = iupMatrixExGetCellValue(ih, 0, col, 1);  /* get displayed value */
    if (value && value[0] != 0)
    {
      if (iupStrEqual(value, column))
        return col;
    }
  }

  return 1;
}

static int iMatrixExItemGoTo_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  if (!matex_data)  /* will be called also by the shortcut key */
   matex_data = (ImatExData*)iupAttribGet(ih_item, "_IUP_MATEX_DATA");

  if (iupAttribGetInt(matex_data->ih, "CELLBYTITLE"))
  {
    char line[50] = "";
    char column[50] = "";

    char* last_lin = iupAttribGet(matex_data->ih, "_IUP_LAST_GOTO_LIN");
    char* last_col = iupAttribGet(matex_data->ih, "_IUP_LAST_GOTO_COL");
    iupStrCopyN(line, 50, last_lin);
    iupStrCopyN(column, 50, last_col);

    if (IupGetParam("_@IUP_GOTO", setparent_param_cb, IupGetDialog(matex_data->ih), "_@IUP_LINE%s\n_@IUP_COLUMN%s\n", line, column, NULL))
    {
      int lin = iMatrixExFindLin(matex_data->ih, line);
      int col = iMatrixExFindCol(matex_data->ih, column);

      IupSetfAttribute(matex_data->ih, "SHOW", "%d:%d", lin, col);
      IupSetfAttribute(matex_data->ih, "FOCUSCELL", "%d:%d", lin, col);

      iupAttribSetStr(matex_data->ih, "_IUP_LAST_GOTO_LIN", line);
      iupAttribSetStr(matex_data->ih, "_IUP_LAST_GOTO_COL", column);
    }
  }
  else
  {
    int lin = iupAttribGetInt(matex_data->ih, "_IUP_LAST_GOTO_LIN");
    int col = iupAttribGetInt(matex_data->ih, "_IUP_LAST_GOTO_COL");

    if (IupGetParam("_@IUP_GOTO", setparent_param_cb, IupGetDialog(matex_data->ih), "_@IUP_LINE%i[1,,]\n_@IUP_COLUMN%i[1,,]\n", &lin, &col, NULL))
    {
      IupSetStrf(matex_data->ih, "SHOW", "%d:%d", lin, col);
      IupSetStrf(matex_data->ih, "FOCUSCELL", "%d:%d", lin, col);

      iupAttribSetInt(matex_data->ih, "_IUP_LAST_GOTO_LIN", lin);
      iupAttribSetInt(matex_data->ih, "_IUP_LAST_GOTO_COL", col);
    }
  }

  return IUP_DEFAULT;
}

static int iMatrixExItemSort_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  IupSetStrAttribute(matex_data->ih, "FOCUSCELL", IupGetAttribute(ih_item, "MENUCONTEXT_CELL"));
  iupMatrixExSortShowDialog(matex_data);
  return IUP_DEFAULT;
}

static int iMatrixExItemFreeze_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int lin, col, flin, fcol, ret;

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);

  ret = IupGetIntInt(matex_data->ih, "FREEZE", &flin, &fcol);
  if (ret!=2 || lin!=flin || col!=fcol)
    IupSetfAttribute(matex_data->ih, "FREEZE", "%d:%d", lin, col);
  else
    IupSetAttribute(matex_data->ih, "FREEZE", NULL);
  return IUP_DEFAULT;
}

static int iMatrixExItemHideCol_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int lin, col;

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);

  IupSetAttributeId(matex_data->ih, "VISIBLECOL", col, "No");

  return IUP_DEFAULT;
}

static int iMatrixExItemShowCol_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int col, num_col = IupGetInt(matex_data->ih, "NUMCOL");

  for(col = 1; col <= num_col; ++col)
  {
    if (!IupGetIntId(matex_data->ih, "VISIBLECOL", col))
      IupSetAttributeId(matex_data->ih, "VISIBLECOL", col, "Yes");
  }

  return IUP_DEFAULT;
}

static int iMatrixExItemHideLin_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int lin, col;

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);

  IupSetAttributeId(matex_data->ih, "VISIBLELIN", lin, "No");

  return IUP_DEFAULT;
}

static int iMatrixExItemShowLin_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int lin, num_lin = IupGetInt(matex_data->ih, "NUMLIN");

  for(lin = 1; lin <= num_lin; ++lin)
  {
    if (!IupGetIntId(matex_data->ih, "VISIBLELIN", lin))
      IupSetAttributeId(matex_data->ih, "VISIBLELIN", lin, "Yes");
  }

  return IUP_DEFAULT;
}

static void iMatrixExInitUnitList(ImatExData* matex_data, int col, char* list_str, int old_unit)
{
  int i, count, len = 0;
  char* unit_name;

  count = IupGetIntId(matex_data->ih, "NUMERICUNITCOUNT", col);
  for (i=0; i<count; i++)
  {
    IupSetIntId(matex_data->ih, "NUMERICUNITSHOWNINDEX", col, i);
    unit_name = IupGetAttributeId(matex_data->ih, "NUMERICUNITSYMBOLSHOWN", col);
    len += sprintf(list_str+len, "%s|", unit_name);
  }

  IupSetIntId(matex_data->ih, "NUMERICUNITSHOWNINDEX", col, old_unit);
}

static int iMatrixExItemNumericUnits_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int unit, decimals;
  int lin, col;
  char format[200], list_str[200] = "|";

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);

  decimals = IupGetIntId(matex_data->ih, "NUMERICFORMATPRECISION", col);
  unit = IupGetIntId(matex_data->ih, "NUMERICUNITSHOWNINDEX", col);

  iMatrixExInitUnitList(matex_data, col, list_str+1, unit);

  sprintf(format, "_@IUP_UNITS%%l%s\n_@IUP_DECIMALS%%i[0]\n", list_str);

  if (IupGetParam("_@IUP_COLUMNUNITS", setparent_param_cb, IupGetDialog(matex_data->ih), format, &unit, &decimals, NULL))
  {
    IupSetIntId(matex_data->ih, "NUMERICUNITSHOWNINDEX", col, unit);
    IupSetIntId(matex_data->ih, "NUMERICFORMATPRECISION", col, decimals);
    IupSetfAttribute(matex_data->ih, "REDRAW", "C%d", col);
  }

  return IUP_DEFAULT;
}

static int iMatrixExItemNumericDecimals_CB(Ihandle* ih_item)
{
  ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_item, "MATRIX_EX_DATA");
  int decimals;
  int lin, col;

  IupGetIntInt(ih_item, "MENUCONTEXT_CELL", &lin, &col);

  decimals = IupGetIntId(matex_data->ih, "NUMERICFORMATPRECISION", col);

  if (IupGetParam("_@IUP_COLUMNDECIMALS", setparent_param_cb, IupGetDialog(matex_data->ih), "_@IUP_DECIMALS%i[0]\n", &decimals, NULL))
  {
    IupSetIntId(matex_data->ih, "NUMERICFORMATPRECISION", col, decimals);
    IupSetfAttribute(matex_data->ih, "REDRAW", "C%d", col);
  }

  return IUP_DEFAULT;
}

static Ihandle* iMatrixExCreateMenuContext(Ihandle* ih, int lin, int col)
{
  int readonly = IupGetInt(ih, "READONLY");

  Ihandle* menu = IupMenu(NULL);

  /************************** General ****************************/

  IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_SETTINGSDLG", NULL), "IMAGE=IUP_ToolsSettings"), "ACTION", iMatrixExItemSettings_CB, NULL));
  IupAppend(menu, IupSeparator());

  /************************** File ****************************/

  IupAppend(menu, IupSetAttributes(IupSubmenu("_@IUP_EXPORT",
      IupMenu(
        IupSetCallbacks(IupSetAttributes(IupItem("Txt..." , NULL), "FILEFORMAT=TXT"),    "ACTION", iMatrixExItemExport_CB, NULL),
        IupSetCallbacks(IupSetAttributes(IupItem("LaTeX...", NULL), "FILEFORMAT=LaTeX"), "ACTION", iMatrixExItemExport_CB, NULL),
        IupSetCallbacks(IupSetAttributes(IupItem("Html..." , NULL), "FILEFORMAT=HTML"),  "ACTION", iMatrixExItemExport_CB, NULL),
        NULL)), "IMAGE=IUP_FileOpen"));

  if (!readonly)
  {
    IupAppend(menu, IupSetAttributes(IupSubmenu("_@IUP_IMPORT",
        IupMenu(
          IupSetCallbacks(IupItem("Txt...",  NULL), "ACTION", iMatrixExItemImport_CB, NULL),
          NULL)), "IMAGE=IUP_FileSave"));
  }

  IupAppend(menu, IupSeparator());

  /************************** Edit - Undo ****************************/

  if (!readonly)
  {
    Ihandle *undo, *redo, *undolist;
    IupAppend(menu, undo = IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_UNDOAC", NULL), "IMAGE=IUP_EditUndo"), "ACTION", iMatrixExItemUndo_CB, NULL));
    IupAppend(menu, redo = IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_REDOAC", NULL), "IMAGE=IUP_EditRedo"), "ACTION", iMatrixExItemRedo_CB, NULL));
    IupAppend(menu, undolist = IupSetCallbacks(IupItem("_@IUP_UNDOLISTDLG", NULL), "ACTION", iMatrixExItemUndoList_CB, NULL));

    if (!IupGetInt(ih, "UNDO"))
      IupSetAttribute(undo, "ACTIVE", "No");
    if (!IupGetInt(ih, "REDO"))
      IupSetAttribute(redo, "ACTIVE", "No");
    if (!IupGetInt(ih, "UNDO") && !IupGetInt(ih, "REDO"))
      IupSetAttribute(undolist, "ACTIVE", "No");

    IupAppend(menu, IupSeparator());
  }

  /************************** Edit - Clipboard ****************************/

  if (!readonly)
    IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_CUTAC", NULL), "IMAGE=IUP_EditCut"),  "ACTION", iMatrixExItemCut_CB, NULL));
  IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_COPYAC",  NULL), "IMAGE=IUP_EditCopy"), "ACTION", iMatrixExItemCopy_CB, NULL));
  if (!readonly)
  {
    IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_PASTEAC", NULL), "IMAGE=IUP_EditPaste"), "ACTION", iMatrixExItemPaste_CB, NULL));
    IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_ERASEAC", NULL), "IMAGE=IUP_EditErase"), "ACTION", iMatrixExItemDel_CB, NULL));
  }
  IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_SELECTALLAC", NULL), "ACTION", iMatrixExItemSelectAll_CB, NULL));
  IupAppend(menu, IupSeparator());

  /************************** Edit - Find ****************************/

  IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_FINDDLG", NULL), "IMAGE=IUP_EditFind"), "ACTION", iMatrixExItemFind_CB, NULL));
  IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_GOTODLG", NULL), "ACTION", iMatrixExItemGoTo_CB, NULL));
  IupAppend(menu, IupSeparator());

  /************************** View ****************************/

  IupAppend(menu, IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_SORTDLG", NULL), "IMAGE=IUP_ToolsSortAscend"), "ACTION", iMatrixExItemSort_CB, NULL));

  {
    int flin, fcol, ret;
    ret = IupGetIntInt(ih, "FREEZE", &flin, &fcol);
    if (ret!=2 || lin!=flin || col!=fcol)
      IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_FREEZE", NULL), "ACTION", iMatrixExItemFreeze_CB, NULL));
    else
      IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_UNFREEZE", NULL), "ACTION", iMatrixExItemFreeze_CB, NULL));
  }

  IupAppend(menu, IupSubmenu("_@IUP_VISIBILITY",
    IupMenu(
      IupSetCallbacks(IupItem("_@IUP_HIDECOLUMN", NULL),        "ACTION", iMatrixExItemHideCol_CB, NULL),
      IupSetCallbacks(IupItem("_@IUP_SHOWHIDDENCOLUMNS", NULL), "ACTION", iMatrixExItemShowCol_CB, NULL),
      IupSetCallbacks(IupItem("_@IUP_HIDELINE", NULL),          "ACTION", iMatrixExItemHideLin_CB, NULL),
      IupSetCallbacks(IupItem("_@IUP_SHOWHIDDENLINES", NULL),   "ACTION", iMatrixExItemShowLin_CB, NULL),
    NULL)));

  if (IupGetAttributeId(ih, "NUMERICQUANTITY", col))
  {
    if (IupGetIntId(ih, "NUMERICQUANTITYINDEX", col))  /* not None */
      IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_COLUMNUNITSDLG", NULL),   "ACTION", iMatrixExItemNumericUnits_CB, NULL));
    else
      IupAppend(menu, IupSetCallbacks(IupItem("_@IUP_COLUMNDECIMALSDLG", NULL),   "ACTION", iMatrixExItemNumericDecimals_CB, NULL));
  }

  /************************** Data ****************************/

  if (!readonly)
  {
    IupAppend(menu, IupSeparator());

    IupAppend(menu, IupSubmenu("_@IUP_COPYTOSAMECOLUMN",
        IupMenu(
          IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_ALLLINES"      , NULL),  "COPYCOLTO=ALL"),      "ACTION", iMatrixExItemCopyColTo_CB, NULL),     
          IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_HERETOTOP"    , NULL),   "COPYCOLTO=TOP"),      "ACTION", iMatrixExItemCopyColTo_CB, NULL),     
          IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_HERETOBOTTOM" , NULL),   "COPYCOLTO=BOTTOM"),   "ACTION", iMatrixExItemCopyColTo_CB, NULL),     
          IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_INTERVALDLG"    , NULL), "COPYCOLTO=INTERVAL"), "ACTION", iMatrixExItemCopyColTo_CB, NULL),     
          IupSetCallbacks(IupSetAttributes(IupItem("_@IUP_SELECTEDLINES" , NULL),  "COPYCOLTO=MARKED"),   "ACTION", iMatrixExItemCopyColTo_CB, NULL),
          NULL)));
  }

  return menu;
}

static int iMatrixSetShowMenuContextAttribId(Ihandle *ih, int lin, int col, const char* value)
{
  ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
  Ihandle* menu = iMatrixExCreateMenuContext(ih, lin, col);
  IFnnii menucontext_cb;
  int x = 0, y = 0;

  iupStrToIntInt(value, &x, &y, ',');

  IupSetAttribute(menu, "MATRIX_EX_DATA", (char*)matex_data);  /* do not use "_IUP_MATEX_DATA" to enable inheritance */
  IupSetfAttribute(menu, "MENUCONTEXT_CELL", "%d:%d", lin, col);

  menucontext_cb = (IFnnii)IupGetCallback(ih, "MENUCONTEXT_CB");
  if (menucontext_cb)
  {
    int ret = menucontext_cb(ih, menu, lin, col);
    if (ret == IUP_IGNORE)
    {
      IupDestroy(menu);
      return 0;
    }
  }

  IupPopup(menu, x, y);

  menucontext_cb = (IFnnii)IupGetCallback(ih, "MENUCONTEXTCLOSE_CB");
  if (menucontext_cb) menucontext_cb(ih, menu, lin, col);

  IupDestroy(menu);
  return 0;
}

static int iMatrixExSetShowDialogAttrib(Ihandle *ih, const char* value)
{
  ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
  int readonly = IupGetInt(ih, "READONLY");

  IupSetAttribute(ih, "MATRIX_EX_DATA", (char*)matex_data);  /* do not use "_IUP_MATEX_DATA" to enable inheritance */
  IupSetStrAttribute(ih, "MENUCONTEXT_CELL", IupGetAttribute(ih, "FOCUSCELL"));

  if (iupStrEqualNoCase(value, "SETTINGS"))
    iMatrixExItemSettings_CB(ih);
  else if (iupStrEqualNoCase(value, "EXPORT_TXT"))
  {
    IupSetAttribute(ih, "FILEFORMAT", "TXT");
    iMatrixExItemExport_CB(ih);
    IupSetAttribute(ih, "FILEFORMAT", NULL);
  }
  else if (iupStrEqualNoCase(value, "EXPORT_LATEX"))
  {
    IupSetAttribute(ih, "FILEFORMAT", "LaTeX");
    iMatrixExItemExport_CB(ih);
    IupSetAttribute(ih, "FILEFORMAT", NULL);
  }
  else if (iupStrEqualNoCase(value, "EXPORT_HTML"))
  {
    IupSetAttribute(ih, "FILEFORMAT", "HTML");
    iMatrixExItemExport_CB(ih);
    IupSetAttribute(ih, "FILEFORMAT", NULL);
  }
  else if (!readonly && iupStrEqualNoCase(value, "IMPORT_TXT"))
    iMatrixExItemImport_CB(ih);
  else if (!readonly && iupStrEqualNoCase(value, "UNDOLIST"))
    iMatrixExItemUndoList_CB(ih);
  else if (iupStrEqualNoCase(value, "FIND"))
    iMatrixExItemFind_CB(ih);
  else if (iupStrEqualNoCase(value, "GOTO"))
    iMatrixExItemGoTo_CB(ih);
  else if (iupStrEqualNoCase(value, "SORT"))
    iMatrixExItemSort_CB(ih);
  else if (!readonly && iupStrEqualNoCase(value, "COPYCOLTO_INTERVAL"))
  {
    IupSetAttribute(ih, "COPYCOLTO", "INTERVAL");
    iMatrixExItemCopyColTo_CB(ih);
    IupSetAttribute(ih, "COPYCOLTO", NULL);
  }

  IupSetAttribute(ih, "MENUCONTEXT_CELL", NULL);
  IupSetAttribute(ih, "MATRIX_EX_DATA", NULL);

  return 0;
}

static IFniiiis iMatrixOriginalButton_CB = NULL;

static int iMatrixExButton_CB(Ihandle* ih, int b, int press, int x, int y, char* r)
{
  if (iMatrixOriginalButton_CB(ih, b, press, x, y, r)==IUP_IGNORE)
    return IUP_IGNORE;

  if (b == IUP_BUTTON3 && press && IupGetInt(ih, "MENUCONTEXT"))
  {
    int pos = IupConvertXYToPos(ih, x, y);
    if (pos >= 0)
    {
      int lin, col;
      int sx, sy;
      char position[100];

      IupTextConvertPosToLinCol(ih, pos, &lin, &col);

      IupGetIntInt(ih, "SCREENPOSITION", &sx, &sy);
      sprintf(position, "%d,%d", sx + x, sy + y);

      iMatrixSetShowMenuContextAttribId(ih, lin, col, position);
    }
  }

  return IUP_DEFAULT;
}

static IFnii iMatrixOriginalKeyPress_CB = NULL;

static int iMatrixExKeyPress_CB(Ihandle* ih, int c, int press)
{
  if (!press)
  {
    switch (c)
    {
    case K_cT: 
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "PORTUGUESE"))
      {
        iMatrixExSelectAll(ih);
        return IUP_IGNORE;
      }
      else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "SPANISH"))
      {
        iMatrixExSelectAll(ih);
        return IUP_IGNORE;
      }
      break;
    case K_cA:
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "ENGLISH"))
      {
        iMatrixExSelectAll(ih);
        return IUP_IGNORE;
      }
      break;
    case K_cV:
      IupSetAttribute(ih, "PASTE", "FOCUS");
      iMatrixListShowLastError(ih);
      return IUP_IGNORE;
    case K_cX: 
      IupSetAttribute(ih, "COPY", "MARKED");
      iMatrixListShowLastError(ih);
      IupSetAttribute(ih, "CLEARVALUE", "MARKED");
      return IUP_IGNORE;
    case K_cC: 
      IupSetAttribute(ih, "COPY", "MARKED");
      iMatrixListShowLastError(ih);
      return IUP_IGNORE;
    case K_cZ: 
      IupSetAttribute(ih, "UNDO", NULL);  /* 1 level */
      return IUP_IGNORE;
    case K_cR: 
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "PORTUGUESE"))
      {
        IupSetAttribute(ih, "REDO", NULL);  /* 1 level */
        return IUP_IGNORE;
      }
      else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "SPANISH"))
      {
        IupSetAttribute(ih, "REDO", NULL);  /* 1 level */
        return IUP_IGNORE;
      }
      break;
    case K_cY:
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "ENGLISH"))
      {
        IupSetAttribute(ih, "REDO", NULL);  /* 1 level */
        return IUP_IGNORE;
      }
      break;
    case K_cU:
      {
        iMatrixExItemUndoList_CB(ih);
        return IUP_IGNORE;
      }
    case K_F3: 
      {
        char* find = IupGetAttribute(ih, "FIND");
        if (find)
        {
          /* invert the direction if not a "forward" one */
          char* direction = iupAttribGet(ih, "FINDDIRECTION");
          if (iupStrEqualNoCase(direction, "LEFTTOP"))
            iupAttribSet(ih, "FINDDIRECTION", "RIGHTBOTTOM");
          else if (iupStrEqualNoCase(direction, "TOPLEFT"))
            iupAttribSet(ih, "FINDDIRECTION", "BOTTOMRIGHT");

          IupSetStrAttribute(ih, "FIND", find);
        }
        return IUP_IGNORE;
      }
    case K_sF3: 
      {
        char* find = IupGetAttribute(ih, "FIND");
        if (find)
        {
          /* invert the direction if not a "reverse" one */
          char* direction = iupAttribGet(ih, "FINDDIRECTION");
          if (iupStrEqualNoCase(direction, "RIGHTBOTTOM"))
            iupAttribSet(ih, "FINDDIRECTION", "LEFTTOP");
          else if (iupStrEqualNoCase(direction, "BOTTOMRIGHT"))
            iupAttribSet(ih, "FINDDIRECTION", "TOPLEFT");

          IupSetStrAttribute(ih, "FIND", find);
        }
        return IUP_IGNORE;
      }
    case K_cL: 
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "PORTUGUESE"))
      {
        ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
        iupMatrixExFindShowDialog(matex_data);
        return IUP_IGNORE;
      }
      else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "SPANISH"))
      {
        ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
        iupMatrixExFindShowDialog(matex_data);
        return IUP_IGNORE;
      }
      break;
    case K_cF: 
      if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "ENGLISH"))
      {
        ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
        iupMatrixExFindShowDialog(matex_data);
        return IUP_IGNORE;
      }
      break;
    case K_mF3:
      {
        ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
        iupMatrixExFindShowDialog(matex_data);
        return IUP_IGNORE;
      }
    case K_cG: 
      {
        iMatrixExItemGoTo_CB(ih);
        return IUP_IGNORE;
      }
    case K_ESC: 
      {
        ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
        if (matex_data->find_dlg && IupGetInt(matex_data->find_dlg, "VISIBLE"))
        {
          IupHide(matex_data->find_dlg);
          return IUP_IGNORE;
        }
        break;
      }
    }
  }

  return iMatrixOriginalKeyPress_CB(ih, c, press);
}

static int iMatrixExCreateMethod(Ihandle* ih, void **params)
{
  ImatExData* matex_data = (ImatExData*)malloc(sizeof(ImatExData));
  memset(matex_data, 0, sizeof(ImatExData));

  iupAttribSet(ih, "_IUP_MATEX_DATA", (char*)matex_data);
  matex_data->ih = ih;

  if (!iMatrixOriginalKeyPress_CB) iMatrixOriginalKeyPress_CB = (IFnii)IupGetCallback(ih, "KEYPRESS_CB");
  IupSetCallback(ih, "KEYPRESS_CB", (Icallback)iMatrixExKeyPress_CB);

  if (!iMatrixOriginalButton_CB) iMatrixOriginalButton_CB = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iMatrixExButton_CB);

  (void)params;
  return IUP_NOERROR;
}

static void iMatrixExDestroyMethod(Ihandle* ih)
{
  ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");

  if (matex_data->busy_progress_dlg)
    IupDestroy(matex_data->busy_progress_dlg);

  if (matex_data->find_dlg)
    IupDestroy(matex_data->find_dlg);

  if (matex_data->undo_stack)
  {
    iupAttribSetClassObject(ih, "UNDOCLEAR", NULL);
    iupArrayDestroy(matex_data->undo_stack);
  }

  free(matex_data);
}

#include "iup_lng_english_matrix.h"
#include "iup_lng_portuguese_matrix.h"
#include "iup_lng_portuguese_matrix_utf8.h"
#include "iup_lng_spanish_matrix.h"
#include "iup_lng_spanish_matrix_utf8.h"

static void iMatrixExSetClassUpdate(Iclass* ic)
{
  Ihandle* lng = NULL;

  (void)ic;

  if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "ENGLISH"))
  {
    lng = iup_load_lng_english_matrix();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "PORTUGUESE"))
  {
    if (IupGetInt(NULL, "UTF8MODE"))
      lng = iup_load_lng_portuguese_matrix_utf8();
    else
      lng = iup_load_lng_portuguese_matrix();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "SPANISH"))
  {
    if (IupGetInt(NULL, "UTF8MODE"))
      lng = iup_load_lng_spanish_matrix_utf8();
    else
      lng = iup_load_lng_spanish_matrix();
  }

  if (lng)
  {
    IupSetLanguagePack(lng);
    IupDestroy(lng);
  }
}

Iclass* iupMatrixExNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("matrix"));

  ic->name = "matrixex";
  ic->cons = "MatrixEx";
  ic->format = "";
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 2;   /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->New = iupMatrixExNewClass;
  ic->Create  = iMatrixExCreateMethod;
  ic->Destroy  = iMatrixExDestroyMethod;
  
  iupClassRegisterAttribute(ic, "FREEZE", NULL, iMatrixExSetFreezeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FREEZECOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "0 0 255", IUPAF_NO_INHERIT);

  iupClassRegisterCallback(ic, "MENUCONTEXT_CB", "nii");
  iupClassRegisterCallback(ic, "MENUCONTEXTCLOSE_CB", "nii");
  iupClassRegisterAttribute(ic, "MENUCONTEXT", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId2(ic, "SHOWMENUCONTEXT", NULL, iMatrixSetShowMenuContextAttribId, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDIALOG", NULL, iMatrixExSetShowDialogAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLASSUPDATE", NULL, (IattribSetFunc)iMatrixExSetClassUpdate, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iMatrixExSetClassUpdate(ic);

  iupMatrixExRegisterClipboard(ic);
  iupMatrixExRegisterBusy(ic);
  iupMatrixExRegisterVisible(ic);
  iupMatrixExRegisterExport(ic);
  iupMatrixExRegisterCopy(ic);
  iupMatrixExRegisterUnits(ic);
  iupMatrixExRegisterUndo(ic);
  iupMatrixExRegisterFind(ic);
  iupMatrixExRegisterSort(ic);

  return ic;
}

Ihandle* IupMatrixEx(void)
{
  return IupCreate("matrixex");
}
