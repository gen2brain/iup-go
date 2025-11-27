/** \file
 * \brief iupmatrix control
 * keyboard control
 *
 * See Copyright Notice in "iup.h"
 */

/**************************************************************************/
/* Functions to control keys in the matrix and in the text edition        */
/**************************************************************************/

#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"


#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_stdcontrols.h"

#include "iupmat_def.h"
#include "iupmat_scroll.h"
#include "iupmat_aux.h"
#include "iupmat_getset.h"
#include "iupmat_key.h"
#include "iupmat_mark.h"
#include "iupmat_edit.h"
#include "iupmat_draw.h"


int iupMatrixIsCharacter(int c)
{
  c = iup_XkeyBase(c);

  if (c == K_circum ||
      c == K_grave ||
      c == K_tilde)
    return 0;

  return iup_isprint(c);
}

static void iMatrixKeyCheckMarkStart(Ihandle* ih, int c, int mark_key)
{
  if (c==mark_key && ih->data->mark_multiple && ih->data->mark_mode != IMAT_MARK_NO)
  {
    if (ih->data->mark_lin1==-1 && ih->data->mark_col1==-1)
      iupMatrixMarkBlockSet(ih, 0, ih->data->lines.focus_cell, ih->data->columns.focus_cell);
  }
}

static void iMatrixKeyCheckMarkEnd(Ihandle* ih, int c, int mark_key)
{
  if (ih->data->mark_multiple && ih->data->mark_mode != IMAT_MARK_NO)
  {
    if (c==mark_key)
    {
      if (ih->data->mark_lin1!=-1 && ih->data->mark_col1!=-1)
        iupMatrixMarkBlockInc(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell);
    }
    else
      iupMatrixMarkBlockReset(ih);
  }
}

int iupMatrixProcessKeyPress(Ihandle* ih, int c)
{
  switch (iup_XkeyBase(c))
  {
  case K_LSHIFT:
  case K_RSHIFT:
  case K_LCTRL:
  case K_RCTRL:
    /* won't scroll for shift+ctrl keys */
    return IUP_DEFAULT;
  }

  /* If the focus is not visible, a scroll is done for that the focus to be visible */
  if (!iupMatrixAuxIsCellStartVisible(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell))
    iupMatrixScrollToVisible(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell);

  switch (c)
  {
    case K_cHOME:
    case K_sHOME:
    case K_HOME:
      iMatrixKeyCheckMarkStart(ih, c, K_sHOME);
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMATRIX_ScrollKeyHome(ih);
      ih->data->homekeycount++;
      iupMatrixAuxCallEnterCellCb(ih);
      iMatrixKeyCheckMarkEnd(ih, c, K_sHOME);
      break;

    case K_cEND:
    case K_sEND:
    case K_END:
      iMatrixKeyCheckMarkStart(ih, c, K_sEND);
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMATRIX_ScrollKeyEnd(ih);
      ih->data->endkeycount++;
      iupMatrixAuxCallEnterCellCb(ih);
      iMatrixKeyCheckMarkEnd(ih, c, K_sEND);
      break;

    case K_sTAB:
    case K_TAB:
      return IUP_CONTINUE;  /* do not redraw, but forwards tab processing */

    case K_sLEFT:
    case K_cLEFT:
    case K_LEFT:
      iMatrixKeyCheckMarkStart(ih, c, K_sLEFT);
      if (iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMATRIX_ScrollKeyLeft(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      iMatrixKeyCheckMarkEnd(ih, c, K_sLEFT);
      break;

    case K_cRIGHT:
    case K_sRIGHT:
    case K_RIGHT:
      iMatrixKeyCheckMarkStart(ih, c, K_sRIGHT);
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMATRIX_ScrollKeyRight(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      iMatrixKeyCheckMarkEnd(ih, c, K_sRIGHT);
      break;

    case K_cUP:
    case K_sUP:
    case K_UP:
      iMatrixKeyCheckMarkStart(ih, c, K_sUP);
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMATRIX_ScrollKeyUp(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      iMatrixKeyCheckMarkEnd(ih, c, K_sUP);
      break;

    case K_cDOWN:
    case K_sDOWN:
    case K_DOWN:
      iMatrixKeyCheckMarkStart(ih, c, K_sDOWN);
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMATRIX_ScrollKeyDown(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      iMatrixKeyCheckMarkEnd(ih, c, K_sDOWN);
      break;

    case K_sPGUP:
    case K_PGUP:
      iMatrixKeyCheckMarkStart(ih, c, K_sPGUP);
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMATRIX_ScrollKeyPgUp(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      iMatrixKeyCheckMarkEnd(ih, c, K_sPGUP);
      break;

    case K_sPGDN:
    case K_PGDN:
      iMatrixKeyCheckMarkStart(ih, c, K_sPGDN);
      if(iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        break;
      iupMATRIX_ScrollKeyPgDown(ih);
      iupMatrixAuxCallEnterCellCb(ih);
      iMatrixKeyCheckMarkEnd(ih, c, K_sPGDN);
      break;

    case K_F2:
    case K_SP:
    case K_sCR:
      if (iupMatrixEditShow(ih))
        return IUP_IGNORE; /* do not redraw */
      break;

    case K_CR:
      if (!ih->data->edit_hide_onfocus && ih->data->editing)
        (void)iupMatrixEditConfirm(ih);  /* ignore return value */
      else
      {
        if (iupMatrixEditShow(ih))
          return IUP_IGNORE; /* do not redraw */
        else
          return IUP_DEFAULT; /* allow the dialog to process defaultenter */
      }
      break;

    case K_ESC:
      if (!ih->data->edit_hide_onfocus && ih->data->editing)
        iupMatrixEditAbort(ih);
      else
        return IUP_DEFAULT; /* allow the dialog to process defaultesc */
      break;

    case K_sDEL:
    case K_DEL:
      IupSetAttribute(ih, "CLEARVALUE", "MARKED");
      break;

    default:
    {
      /* if a valid character is pressed enter edition mode */
      if (iupMatrixIsCharacter(c))
      {
        if (iupMatrixEditShow(ih))
        {
          if (ih->data->datah == ih->data->texth)
          {
            char value[2] = { 0, 0 };
            value[0] = (char)c;
            IupStoreAttribute(ih->data->datah, "VALUEMASKED", value);
            IupSetAttribute(ih->data->datah, "CARET", "2");
          }
          return IUP_IGNORE; /* do not redraw */
        }
      }

      iupMatrixDrawUpdate(ih);
      return IUP_DEFAULT;
    }
  }

  iupMatrixDrawUpdate(ih);
  return IUP_IGNORE;  /* ignore processed keys */
}

void iupMatrixKeyResetHomeEndCount(Ihandle* ih)
{
  ih->data->homekeycount = 0;
  ih->data->endkeycount = 0;
}

int iupMatrixKeyPress_CB(Ihandle* ih, int c, int press)
{
  int oldc = c;
  IFniiiis cb;

  if (!iupMatrixIsValid(ih, 1))
    return IUP_DEFAULT;

  /* there are no cells that can get the focus */
  if (ih->data->columns.num <= 1 || ih->data->lines.num <= 1)
    return IUP_DEFAULT;

  if (!press)
    return IUP_DEFAULT;

  cb = (IFniiiis)IupGetCallback(ih, "ACTION_CB");
  if (cb)
  {
    /* if a valid character is pressed will clear the existing cell value and enter edition mode */
    if (iupMatrixIsCharacter(c))
    {
      char value[2]={0,0};
      value[0] = (char)c;
      c = cb(ih, c, ih->data->lines.focus_cell, ih->data->columns.focus_cell, 0, value);
    }
    else
    {
      char* value = iupMatrixGetValue(ih, ih->data->lines.focus_cell, ih->data->columns.focus_cell);
      c = cb(ih, c, ih->data->lines.focus_cell, ih->data->columns.focus_cell, 0, value);
    }

    if (c == IUP_IGNORE || c == IUP_CLOSE || c == IUP_CONTINUE)
      return c;
    else if (c == IUP_DEFAULT)
      c = oldc;
  }

  if (c != K_HOME && c != K_sHOME)
    ih->data->homekeycount = 0;
  if (c != K_END && c != K_sEND)
    ih->data->endkeycount = 0;

  return iupMatrixProcessKeyPress(ih, c);
}
