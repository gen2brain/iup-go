/** \file
 * \brief iupmatrix control
 * mouse events
 *
 * See Copyright Notice in "iup.h"
 */

/**************************************************************************/
/* Functions to handle mouse events                                       */
/**************************************************************************/

#include <stdlib.h>
#include <time.h>

#include "iup.h"
#include "iupcbs.h"


#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_flatscrollbar.h"

#include "iupmat_def.h"
#include "iupmat_colres.h"
#include "iupmat_aux.h"
#include "iupmat_mouse.h"
#include "iupmat_key.h"
#include "iupmat_mark.h"
#include "iupmat_edit.h"
#include "iupmat_draw.h"
#include "iupmat_getset.h"
#include "iupmat_scroll.h"


#define IMAT_DRAG_SCROLL_DELTA 5

static void iMatrixMouseCallMoveCb(Ihandle* ih, int lin, int col)
{
  IFnii cb;

  if (!ih->data->edit_hide_onfocus && ih->data->editing)
  {
    cb = (IFnii)IupGetCallback(ih, "EDITMOUSEMOVE_CB");
    if (cb)
      cb(ih, lin, col);
  }

  cb = (IFnii)IupGetCallback(ih, "MOUSEMOVE_CB");
  if (cb)
    cb(ih, lin, col);
}

static int iMatrixMouseCallClickCb(Ihandle* ih, int press, int lin, int col, char* status)
{
  IFniis cb;

  if (!ih->data->edit_hide_onfocus && ih->data->editing)
  {
    if (press)
      cb = (IFniis)IupGetCallback(ih, "EDITCLICK_CB");
    else
      cb = (IFniis)IupGetCallback(ih, "EDITRELEASE_CB");

    if (cb)
      cb(ih, lin, col, status);
  }

  if (press)
    cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
  else
    cb = (IFniis)IupGetCallback(ih, "RELEASE_CB");

  if (cb)
    return cb(ih, lin, col, status);

  return IUP_DEFAULT;
}

static void iMatrixMouseEdit(Ihandle* ih, int x, int y)
{
  if (iupMatrixEditShowXY(ih, x, y))
  {
    iupMatrixMarkBlockReset(ih);

    if (ih->data->datah == ih->data->droph)
      IupSetAttribute(ih->data->datah, "SHOWDROPDOWN", "YES");

    if (IupGetGlobal("MOTIFVERSION"))
    {
      /* Sequence of focus_cb in Motif from here:
            Matrix-Focus(0) - ok
            Edit-KillFocus - weird, must avoid using _IUPMAT_DOUBLECLICK
         Since OpenMotif version 2.2.3 this is not necessary anymore. */
      if (atoi(IupGetGlobal("MOTIFNUMBER")) < 2203) 
        iupAttribSet(ih, "_IUPMAT_DOUBLECLICK", "1");
    }
  }

  /* reset mouse flags */
  ih->data->button1edit = 0;
}

static int iMatrixIsDropArea(Ihandle* ih, int lin, int col, int x, int y)
{
  IFnii dropcheck_cb = (IFnii)IupGetCallback(ih, "DROPCHECK_CB");
  if (dropcheck_cb)
  {
    int ret = dropcheck_cb(ih, lin, col);
    if (ret != IUP_IGNORE)
    {
      int x1, y1, x2, y2;

      iupMatrixGetVisibleCellDim(ih, lin, col, &x1, &y1, &x2, &y2);
      x2 += x1;  /* iupMatrixGetVisibleCellDim returns w and h */
      y2 += y1;

      if (ret == IUP_DEFAULT)
        iupMatrixDrawSetDropFeedbackArea(&x1, &y1, &x2, &y2);
      else if (ret == IUP_CONTINUE)
        iupMatrixDrawSetToggleFeedbackArea(iupAttribGetBoolean(ih, "TOGGLECENTERED"), &x1, &y1, &x2, &y2);

      if (x > x1 && x < x2 &&
          y > y1 && y < y2)
      {
        if (ret == IUP_DEFAULT)  /* dropdown */
          return 1;
        else if (ret == IUP_CONTINUE)  /* toggle */
          return -1;
      }
    }
  }
  return 0;
}

static void iMatrixMouseLeftPress(Ihandle* ih, int lin, int col, int shift, int ctrl, int is_double, int x, int y)
{
  if (is_double)
  {
    iupMatrixMarkBlockReset(ih);

    if (lin==0 || col==0)
      return;

    /* if a double click NOT in the current cell */
    if (lin != ih->data->lines.focus_cell || col != ih->data->columns.focus_cell)
    {
      /* leave the previous cell if the matrix previously had the focus */
      if (ih->data->has_focus && iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
        return;

      ih->data->lines.focus_cell = lin;
      ih->data->columns.focus_cell = col;

      iupMatrixAuxCallEnterCellCb(ih);
    }
    
    ih->data->button1edit = 1; /* prepare for edit */
  }
  else /* single click */
  {
    if (shift && ih->data->mark_multiple && ih->data->mark_mode != IMAT_MARK_NO)
    {
      iupMatrixMarkBlockInc(ih, lin, col);
    }
    else
    {
      if (lin>0 && col>0 && !(ih->data->noscroll_as_title && (lin < ih->data->lines.num_noscroll || col < ih->data->columns.num_noscroll)))
      {
        int ret;

        if (iupMatrixAuxCallLeaveCellCb(ih) == IUP_IGNORE)
          return;

        ih->data->lines.focus_cell = lin;
        ih->data->columns.focus_cell = col;

        ret = iMatrixIsDropArea(ih, lin, col, x, y);

        /* process mark before EnterCell */
        if (!ret && ih->data->mark_mode != IMAT_MARK_NO)
          iupMatrixMarkBlockSet(ih, ctrl, lin, col);

        iupMatrixAuxCallEnterCellCb(ih);

        if (ret == 1) /* dropdown */
        {
          ih->data->button1edit = 1; /* prepare for edit */
        }
        else if (ret == -1) /* toggle */
        {
          int togglevalue;     
          IFniii togglevalue_cb;

          if (iupAttribGetBoolean(ih, "READONLY"))
            return;

          togglevalue_cb = (IFniii)IupGetCallback(ih, "TOGGLEVALUE_CB");

          if (iupAttribGetBoolean(ih, "TOGGLECENTERED"))
          {
            char* value = iupMatrixGetValueDisplay(ih, lin, col);
            togglevalue = !iupStrBoolean(value); /* invert value */
            iupMatrixSetValue(ih, lin, col, togglevalue ? "1" : "0", -1);
          }
          else
          {
            togglevalue = !iupAttribGetIntId2(ih, "TOGGLEVALUE", lin, col);  /* invert value */
            iupAttribSetIntId2(ih, "TOGGLEVALUE", lin, col, togglevalue);
          }

          iupMatrixDrawCells(ih, lin, col, lin, col);
          if (togglevalue_cb) 
            togglevalue_cb(ih, lin, col, togglevalue);
        }
      }
      else
      {
        /* only process marks here if at titles */
        if (ih->data->mark_mode != IMAT_MARK_NO && iupAttribGetBoolean(ih, "MARKATTITLE"))
        {
          iupMatrixMarkBlockSet(ih, ctrl, lin, col);

          if (ih->data->merge_info_count)
          {
            int merged = iupMatrixGetMerged(ih, lin, col);
            if (merged)
            {
              int endLin, endCol;
              iupMatrixGetMergedRect(ih, merged, NULL, &endLin, NULL, &endCol);

              if (lin == 0 || (ih->data->noscroll_as_title && lin < ih->data->lines.num_noscroll))
                iupMatrixMarkBlockInc(ih, 0, endCol);
              else
                iupMatrixMarkBlockInc(ih, endLin, 0);
            }
          }
        }
      }
    }
  }
}

int iupMatrixMouseButton_CB(Ihandle* ih, int button, int press, int x, int y, char* status)
{
  int lin=-1, col=-1;

  if (!iupMatrixIsValid(ih, 0))
    return IUP_IGNORE;

  if (press)
  {
    ih->data->button1edit = 0;

    /* Sometimes the edit Focus callback is not called when the user clicks in the parent canvas,
    so we have to compensate that. */

    if (ih->data->edit_hide_onfocus)
    {
      ih->data->edit_hidden_byfocus = 1;
      iupMatrixEditHide(ih);
      ih->data->edit_hidden_byfocus = 0;
    }

    ih->data->has_focus = 1;
  }

  iupMatrixGetCellFromXY(ih, x, y, &lin, &col);

  ih->data->button1press = 0;

  if (button == IUP_BUTTON1)
  {
    if (press)
    {
      ih->data->button1press = 1;

      iupMatrixKeyResetHomeEndCount(ih);

      if (iupMatrixColResStart(ih, x, y))
      {
        iupMatrixMarkBlockReset(ih);
        return IUP_DEFAULT;  /* Resize of the width a of a column was started */
      }

      if (lin!=-1 && col!=-1)
        iMatrixMouseLeftPress(ih, lin, col, iup_isshift(status), iup_iscontrol(status), iup_isdouble(status), x, y);
    }
    else
    {
      if (iupMatrixColResIsResizing(ih))  /* If it was made a column resize, finish it */
        iupMatrixColResFinish(ih, x);

      if (ih->data->button1edit)           /* edit only when releasing the button */
        iMatrixMouseEdit(ih, x, y);
    }
  }
  else
    iupMatrixMarkBlockReset(ih);

  if (lin!=-1 && col!=-1)
  {
    if (iMatrixMouseCallClickCb(ih, press, lin, col, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  iupMatrixDrawUpdate(ih);
  return IUP_DEFAULT;
}

static void iMatrixMouseResetCursor(Ihandle* ih)
{
  char *cursor = iupAttribGet(ih, "_IUPMAT_CURSOR");
  if (cursor)
  {
    IupStoreAttribute(ih, "CURSOR", cursor);
    iupAttribSet(ih, "_IUPMAT_CURSOR", NULL);
  }
}

static void iMatrixMouseSetCursor(Ihandle* ih, const char* name)
{
  if (!iupAttribGet(ih, "_IUPMAT_CURSOR"))
    iupAttribSetStr(ih, "_IUPMAT_CURSOR", IupGetAttribute(ih, "CURSOR"));
  IupSetAttribute(ih, "CURSOR", name);
}

int iupMatrixMouseMove_CB(Ihandle* ih, int x, int y, char *status)
{
  int lin, col, has_lincol;

  if (!iupMatrixIsValid(ih, 0))
    return IUP_DEFAULT;

  iupFlatScrollBarMotionUpdate(ih, x, y);

  has_lincol = iupMatrixGetCellFromXY(ih, x, y, &lin, &col);

  if (iup_isbutton1(status) && ih->data->button1press && ih->data->mark_block && ih->data->mark_multiple && ih->data->mark_mode != IMAT_MARK_NO)
  {
    if ((x < ih->data->columns.dt[0].size || x < IMAT_DRAG_SCROLL_DELTA) && (ih->data->columns.first > ih->data->columns.num_noscroll))
      iupMATRIX_ScrollLeft(ih);
    else if ((x > iupMatrixGetWidth(ih) - IMAT_DRAG_SCROLL_DELTA) && (ih->data->columns.last < ih->data->columns.num - 1))
      iupMATRIX_ScrollRight(ih);

    if ((y < ih->data->lines.dt[0].size || y < IMAT_DRAG_SCROLL_DELTA) && (ih->data->lines.first > ih->data->lines.num_noscroll))
      iupMATRIX_ScrollUp(ih);
    else if ((y > iupMatrixGetHeight(ih) - IMAT_DRAG_SCROLL_DELTA) && (ih->data->lines.last < ih->data->lines.num - 1))
      iupMATRIX_ScrollDown(ih);

    if (has_lincol)
    {
      iupMatrixMarkBlockInc(ih, lin, col);

      if (ih->data->merge_info_count)
      {
        int merged = iupMatrixGetMerged(ih, lin, col);
        if (merged)
        {
          int endLin, endCol;
          iupMatrixGetMergedRect(ih, merged, NULL, &endLin, NULL, &endCol);

          if (lin == 0 || (ih->data->noscroll_as_title && lin < ih->data->lines.num_noscroll))
            iupMatrixMarkBlockInc(ih, 0, endCol);
          else
            iupMatrixMarkBlockInc(ih, endLin, 0);
        }
      }

      iupMatrixDrawUpdate(ih);

      iMatrixMouseCallMoveCb(ih, lin, col);
    }
    return IUP_DEFAULT;
  }
  else if(iupMatrixColResIsResizing(ih)) /* Make a resize in a column size */
    iupMatrixColResMove(ih, x);
  else if (has_lincol && iMatrixIsDropArea(ih, lin, col, x, y) != 0)
    iMatrixMouseSetCursor(ih, "ARROW");
  else if (iupMatrixColResCheckChangeCursor(ih, x, y))
    iMatrixMouseSetCursor(ih, "RESIZE_W");
  else 
    iMatrixMouseResetCursor(ih);

  if (has_lincol)
    iMatrixMouseCallMoveCb(ih, lin, col);

  return IUP_DEFAULT;
}
