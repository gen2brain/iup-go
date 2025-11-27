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


static void iMatrixExFindDialogUpdateResult(ImatExData* matex_data, Ihandle* ih_button)
{
  char* last_found = IupGetAttribute(matex_data->ih, "_IUP_LAST_FOUND");
  if (!last_found)
    IupSetStrAttribute(IupGetDialogChild(ih_button, "RESULT"), "TITLE", "_@IUP_NOT_FOUND");
  else
  {
    char *lin_title, *col_title;
    int lin, col;
    iupStrToIntInt(last_found, &lin, &col, ':');
    lin_title = IupGetAttributeId2(matex_data->ih, "", lin, 0);
    col_title = IupGetAttributeId2(matex_data->ih, "", 0, col);
    if (lin_title && col_title)
      IupSetfAttribute(IupGetDialogChild(ih_button, "RESULT"), "TITLE", "%s %s (%s, %s)", IupGetLanguageString("IUP_FOUND_AT"), last_found, lin_title, col_title);
    else if (lin_title)
      IupSetfAttribute(IupGetDialogChild(ih_button, "RESULT"), "TITLE", "%s %s (%s, %d)", IupGetLanguageString("IUP_FOUND_AT"), last_found, lin_title, col);
    else if (col_title)
      IupSetfAttribute(IupGetDialogChild(ih_button, "RESULT"), "TITLE", "%s %s (%d, %s)", IupGetLanguageString("IUP_FOUND_AT"), last_found, lin, col_title);
    else
      IupSetfAttribute(IupGetDialogChild(ih_button, "RESULT"), "TITLE", "%s %s", IupGetLanguageString("IUP_FOUND_AT"), last_found);
  }
}

static int iMatrixExFindDialogFindNext_CB(Ihandle* ih_button)
{
  Ihandle* text = IupGetDialogChild(ih_button, "FINDTEXT");
  char* find = IupGetAttribute(text, "VALUE");
  if (find && find[0]!=0)
  {
    ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_button, "MATRIX_EX_DATA");
    int matchcase = IupGetInt(IupGetDialogChild(ih_button, "MATCHCASE"), "VALUE");
    int matchwholecell = IupGetInt(IupGetDialogChild(ih_button, "MATCHWHOLECELL"), "VALUE");
    int searchbyrow = IupGetInt(IupGetDialogChild(ih_button, "SEARCHBYROW"), "VALUE");

    iupAttribSet(matex_data->ih, "FINDMATCHCASE", matchcase? "Yes": "No");
    iupAttribSet(matex_data->ih, "FINDMATCHWHOLECELL", matchwholecell? "Yes": "No");
    iupAttribSet(matex_data->ih, "FINDDIRECTION", searchbyrow? "RIGHTBOTTOM": "BOTTOMRIGHT");  /* Forward */

    IupSetAttribute(IupGetDialogChild(ih_button, "RESULT"), "TITLE", "");

    IupSetStrAttribute(matex_data->ih, "FIND", find);

    iMatrixExFindDialogUpdateResult(matex_data, ih_button);
  }
  return IUP_DEFAULT;
}

static int iMatrixExFindDialogFindPrevious_CB(Ihandle* ih_button)
{
  Ihandle* text = IupGetDialogChild(ih_button, "FINDTEXT");
  char* find = IupGetAttribute(text, "VALUE");
  if (find && find[0]!=0)
  {
    ImatExData* matex_data = (ImatExData*)IupGetAttribute(ih_button, "MATRIX_EX_DATA");
    int matchcase = IupGetInt(IupGetDialogChild(ih_button, "MATCHCASE"), "VALUE");
    int matchwholecell = IupGetInt(IupGetDialogChild(ih_button, "MATCHWHOLECELL"), "VALUE");
    int searchbyrow = IupGetInt(IupGetDialogChild(ih_button, "SEARCHBYROW"), "VALUE");

    iupAttribSet(matex_data->ih, "FINDMATCHCASE", matchcase? "Yes": "No");
    iupAttribSet(matex_data->ih, "FINDMATCHWHOLECELL", matchwholecell? "Yes": "No");
    iupAttribSet(matex_data->ih, "FINDDIRECTION", searchbyrow? "LEFTTOP": "TOPLEFT");  /* Backward */

    IupSetAttribute(IupGetDialogChild(ih_button, "RESULT"), "TITLE", "");

    IupSetStrAttribute(matex_data->ih, "FIND", find);

    iMatrixExFindDialogUpdateResult(matex_data, ih_button);
  }
  return IUP_DEFAULT;
}

static int iMatrixExFindDialogClose_CB(Ihandle* ih_button)
{
  IupHide(IupGetDialog(ih_button));
  return IUP_DEFAULT;
}

static void iMatrixExFindCreateDialog(ImatExData* matex_data)
{
  Ihandle *text_box, *options_box, *find_next, *find_prev, *result_box,
          *dlg, *close, *dlg_box, *button_box, *parent;

  text_box = IupSetAttributes(IupHbox(
    IupLabel("_@IUP_FIND_WHAT"), 
    IupSetAttributes(IupText(NULL), "EXPAND=HORIZONTAL, NAME=FINDTEXT"),
    NULL), "ALIGNMENT=ACENTER");
  IupSetAttribute(text_box,"MARGIN","0x0");

  options_box = IupHbox(
    IupVbox(
      IupSetAttributes(IupToggle("_@IUP_MATCH_CASE", NULL), "NAME=MATCHCASE"),
      IupSetAttributes(IupToggle("_@IUP_MATCH_WHOLE_CELL", NULL), "NAME=MATCHWHOLECELL"),
      NULL),
    IupSetAttributes(IupFrame(IupRadio(IupHbox(
      IupSetAttributes(IupToggle("_@IUP_BY_ROW", NULL), "NAME=SEARCHBYROW"),
      IupSetAttributes(IupToggle("_@IUP_BY_COL", NULL), "NAME=SEARCHBYCOL"),
      NULL))), "TITLE=_@IUP_SEARCH, MARGIN=5x5"),
    NULL);
  IupSetAttribute(options_box,"MARGIN","0x5");

  find_prev = IupButton("_@IUP_FIND_PREVIOUS", NULL);
  IupSetStrAttribute(find_prev, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(find_prev, "ACTION", (Icallback)iMatrixExFindDialogFindPrevious_CB);

  find_next = IupButton("_@IUP_FIND_NEXT", NULL);
  IupSetStrAttribute(find_next, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(find_next, "ACTION", (Icallback)iMatrixExFindDialogFindNext_CB);

  close = IupButton("_@IUP_CLOSE", NULL);
  IupSetStrAttribute(close, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(close, "ACTION", (Icallback)iMatrixExFindDialogClose_CB);

  button_box = IupHbox(
    IupFill(), 
    find_prev,
    find_next,
    close,
    NULL);
  IupSetAttribute(button_box,"MARGIN","0x0");
  IupSetAttribute(button_box, "NORMALIZESIZE", "HORIZONTAL");

  result_box = IupHbox(
    IupSetAttributes(IupLabel(""), "EXPAND=HORIZONTAL, NAME=RESULT"),
    NULL);
  IupSetAttribute(result_box,"MARGIN","0x0");

  dlg_box = IupVbox(
    text_box,
    options_box,
    button_box,
    result_box,
    NULL);
  IupSetAttribute(dlg_box,"MARGIN","10x10");
  IupSetAttribute(dlg_box,"GAP","5");

  dlg = IupDialog(dlg_box);

  parent = IupGetDialog(matex_data->ih);

  IupSetStrAttribute(dlg,"TITLE","_@IUP_FIND");
  IupSetAttribute(dlg,"MINBOX","NO");
  IupSetAttribute(dlg,"MAXBOX","NO");
  IupSetAttribute(dlg, "TOOLBOX","YES");
  IupSetAttributeHandle(dlg,"DEFAULTENTER", find_next);
  IupSetAttributeHandle(dlg,"DEFAULTESC", close);
  IupSetAttributeHandle(dlg,"PARENTDIALOG", parent);

  IupSetAttribute(dlg, "MATRIX_EX_DATA", (char*)matex_data);  /* do not use "_IUP_MATEX_DATA" to enable inheritance */

  if (IupGetAttribute(parent, "ICON"))
    IupSetStrAttribute(dlg,"ICON", IupGetAttribute(parent, "ICON"));
  else
    IupSetStrAttribute(dlg,"ICON", IupGetGlobal("ICON"));

  matex_data->find_dlg = dlg;
}

static void iMatrixExFindInitDialog(ImatExData* matex_data)
{
  char* direction;

  if (!(matex_data->find_dlg))
    iMatrixExFindCreateDialog(matex_data);

  IupSetStrAttribute(IupGetDialogChild(matex_data->find_dlg, "MATCHCASE"), "VALUE", iupAttribGetStr(matex_data->ih, "FINDMATCHCASE"));
  IupSetStrAttribute(IupGetDialogChild(matex_data->find_dlg, "MATCHWHOLECELL"), "VALUE", iupAttribGetStr(matex_data->ih, "FINDMATCHWHOLECELL"));
  IupSetAttribute(IupGetDialogChild(matex_data->find_dlg, "RESULT"), "TITLE", "");

  direction = iupAttribGetStr(matex_data->ih, "FINDDIRECTION");
  if (iupStrEqualNoCase(direction, "LEFTTOP") || iupStrEqualNoCase(direction, "RIGHTBOTTOM"))
    IupSetAttribute(IupGetDialogChild(matex_data->find_dlg, "SEARCHBYROW"), "VALUE", "Yes");
  else
    IupSetAttribute(IupGetDialogChild(matex_data->find_dlg, "SEARCHBYCOL"), "VALUE", "Yes");
}

void iupMatrixExFindShowDialog(ImatExData* matex_data)
{
  iMatrixExFindInitDialog(matex_data);

  /* Only position the dialog at the first time, 
     then let the user choose the dialog position on screen */
  if (!(matex_data->find_dlg->handle))
  {
    int x, y;
    iupMatrixExGetDialogPosition(matex_data, &x, &y);
    IupShowXY(matex_data->find_dlg, x, y);
  }
  else
    IupShow(matex_data->find_dlg);
}

static int iMatrixMatch(Ihandle *ih, const char* findvalue, int lin, int col, int matchcase, int matchwholecell, int utf8)
{
  char* value = iupMatrixExGetCellValue(ih, lin, col, 1);  /* get displayed value */
  if (!value || value[0] == 0)
    return 0;

  if (matchwholecell)
    return iupStrCompareEqual(value, findvalue, matchcase, utf8, 0);
  else
    return iupStrCompareFind(value, findvalue, matchcase, utf8);  /* search only for the first occurrence */
}

static int iMatrixExSetFind(Ihandle *ih, const char* value, int inc, int flip, int matchcase, int matchwholecell, int find_col, int *lin, int *col, int search_cur_cell)
{
  int utf8 = IupGetInt(NULL, "UTF8MODE");
  int num_lin = IupGetInt(ih, "NUMLIN");
  int num_col = IupGetInt(ih, "NUMCOL");
  int count = (num_lin+1)*(num_col+1);
  int pos, start_pos, match = 0;
  char* find_value;

  find_value = iupStrDup(value);

  if (search_cur_cell)  /* search the current cell */
  {
    /* the FOCUSCELL is always visible and not a title */
    if (iMatrixMatch(ih, find_value, *lin, *col, matchcase, matchwholecell, utf8))
    {
      free(find_value);
      return 1;
    }
  }

  if (flip)
    pos = (*col)*(num_lin+1) + *lin;
  else
    pos = (*lin)*(num_col+1) + *col;

  start_pos = pos;
  do 
  {
    pos += inc;

    if (pos < 0) pos = count-1;   /* if less than first cell, go to last */
    if (pos > count-1) pos = 0;   /* if more than last cell, go to first */

    if (flip)
    {
      *lin = pos % (num_lin+1);
      *col = pos / (num_lin+1);
    }
    else
    {
      *lin = pos / (num_col+1);
      *col = pos % (num_col+1);
    }

    if (*lin == 0 || *col == 0)  /* don't search on titles */
      continue;

    if (find_col != 0)
    {
      if (pos == start_pos)
      {
        free(find_value);
        return 0;
      }

      if (*col != find_col)
        continue;
    }

    if (pos == start_pos)
    {
      if (!search_cur_cell)
      {
        if (iMatrixMatch(ih, find_value, *lin, *col, matchcase, matchwholecell, utf8))
        {
          free(find_value);
          return 1;
        }
      }

      free(find_value);
      return 0;
    }

    if (!iupMatrixExIsLineVisible(ih, *lin) || !iupMatrixExIsColumnVisible(ih, *col))
      continue;

    match = iMatrixMatch(ih, find_value, *lin, *col, matchcase, matchwholecell, utf8);

  } while (!match);

  free(find_value);
  return 1;
}

static int iMatrixExSetFindAttrib(Ihandle *ih, const char* value)
{
  int lin=1, col=1, search_cur_cell = 0, find_col;
  int inc, flip, matchcase, matchwholecell;
  char* direction;

  if (!value || value[0]==0)
    return 0;

  IupGetIntInt(ih, "FOCUSCELL", &lin, &col);

  find_col = iupAttribGetInt(ih, "FINDCOL");

  direction = iupAttribGetStr(ih, "FINDDIRECTION");
  if (iupStrEqualNoCase(direction, "LEFTTOP"))
  {
    flip = 0;
    inc = -1;
  }
  else if (iupStrEqualNoCase(direction, "TOPLEFT"))
  {
    flip = 1;
    inc = -1;
  }
  else if (iupStrEqualNoCase(direction, "BOTTOMRIGHT"))
  {
    flip = 1;
    inc = +1;
  }
  else  /* RIGHTBOTTOM */
  {
    flip = 0;
    inc = +1;
  }
  matchcase = iupAttribGetInt(ih, "FINDMATCHCASE");
  matchwholecell = iupAttribGetInt(ih, "FINDMATCHWHOLECELL");

  {
    int last_lin=0, last_col=0;
    IupGetIntInt(ih, "_IUP_LAST_FOUND", &last_lin, &last_col);
    if (last_lin!=lin || last_col!=col)
      search_cur_cell = 1;  /* search in the current cell */
  }

  if (iMatrixExSetFind(ih, value, inc, flip, matchcase, matchwholecell, find_col, &lin, &col, search_cur_cell))
  {
    IupSetfAttribute(ih,"FOCUSCELL", "%d:%d", lin, col);
    IupSetfAttribute(ih,"SHOW", "%d:%d", lin, col);
    IupSetAttribute(ih,"MARKED",NULL);
    IupSetAttributeId2(ih,"MARK",lin,col,"1");
    IupSetfAttribute(ih,"REDRAW","L%d", lin);
    IupSetfAttribute(ih,"_IUP_LAST_FOUND", "%d:%d", lin, col);
    IupSetFocus(ih);
  }
  else
    IupSetAttribute(ih,"_IUP_LAST_FOUND", NULL);

  return 1;
}

void iupMatrixExRegisterFind(Iclass* ic)
{
  iupClassRegisterAttribute(ic, "FIND", NULL, iMatrixExSetFindAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FINDDIRECTION", NULL, NULL, IUPAF_SAMEASSYSTEM, "RIGHTBOTTOM", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FINDMATCHCASE", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FINDMATCHWHOLECELL", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FINDCOL", NULL, NULL, IUPAF_SAMEASSYSTEM, NULL, IUPAF_NO_INHERIT);
}
