/** \file
 * \brief IupMatrix Expansion Library.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "iup.h"
#include "iupcbs.h"
#include "iupcontrols.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_register.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_matrixex.h"


static int iMatrixExBusyProgressCancel_CB(Ihandle* ih)
{
  ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
  matex_data->busy_progress_abort = 1;
  return IUP_DEFAULT;
}

static void iMatrixExBusyShowProgress(ImatExData* matex_data, int count, const char* busyname)
{
  int x, y;

  if (!matex_data->busy_progress_dlg)
  {
    matex_data->busy_progress_dlg = IupProgressDlg();
    IupSetCallback(matex_data->busy_progress_dlg, "CANCEL_CB", iMatrixExBusyProgressCancel_CB);
    IupSetAttributeHandle(matex_data->busy_progress_dlg, "PARENTDIALOG", IupGetDialog(matex_data->ih));
    IupSetAttribute(matex_data->busy_progress_dlg, "_IUP_MATEX_DATA", (char*)matex_data);

    IupMap(matex_data->busy_progress_dlg); /* to compute dialog size */
  }
  
  if (busyname)
  {
    char str[50] = "IUP_";
    strcat(str, busyname);
    iupStrReplace(str, ':', '_');
    if (iupStrEqual(busyname, "UNDO") || iupStrEqual(busyname, "REDO"))
      strcat(str, "NAME");  /* To avoid conflict with the menu item string */
    busyname = IupGetLanguageString(str);
    if (!busyname)
      busyname = str+4;
    IupStoreAttribute(matex_data->busy_progress_dlg, "DESCRIPTION", busyname);
  }

  IupSetInt(matex_data->busy_progress_dlg, "TOTALCOUNT", count);
  IupSetAttribute(matex_data->busy_progress_dlg, "COUNT", "0");

  IupSetAttribute(IupGetDialog(matex_data->ih),"ACTIVE","NO");

  IupRefresh(matex_data->busy_progress_dlg);

  x = IupGetInt(matex_data->ih, "X") + (matex_data->ih->currentwidth-matex_data->busy_progress_dlg->currentwidth)/2;
  y = IupGetInt(matex_data->ih, "Y") + (matex_data->ih->currentheight-matex_data->busy_progress_dlg->currentheight)/2;
  IupShowXY(matex_data->busy_progress_dlg, x, y);
}

void iupMatrixExBusyStart(ImatExData* matex_data, int count, const char* busyname)
{
  /* can not start a new one if already busy */
  iupASSERT(!matex_data->busy);
  if (matex_data->busy)
    return;

  matex_data->busy = 1;
  matex_data->busy_count = 0;
  matex_data->busy_undo_block = 0;

  IupStoreAttribute(matex_data->ih, "_IUPMATEX_OLDCURSOR", IupGetAttribute(matex_data->ih, "CURSOR"));
  IupSetAttribute(matex_data->ih, "CURSOR", "BUSY");

  matex_data->busy_cb = (IFniis)IupGetCallback(matex_data->ih, "BUSY_CB");
  if (matex_data->busy_cb)
    matex_data->busy_cb(matex_data->ih, 1, count, (char*)busyname);

  if (iupAttribGetBoolean(matex_data->ih, "BUSYPROGRESS"))
  {
    iMatrixExBusyShowProgress(matex_data, count, busyname);

    matex_data->busy_progress_abort = 0;
    matex_data->busy = 2;
  }

  if (iupStrBoolean(iupAttribGetClassObject(matex_data->ih, "UNDOREDO")))
  {
    matex_data->busy_undo_block = 1;
    iupMatrixExUndoPushBegin(matex_data, busyname);
  }
}

int iupMatrixExBusyInc(ImatExData* matex_data)
{
  if (matex_data->busy)
  {
    matex_data->busy_count++;

    if (matex_data->busy_cb)
    {
      int ret = matex_data->busy_cb(matex_data->ih, 2, matex_data->busy_count, NULL);
      if (ret == IUP_IGNORE)
      {
        iupMatrixExBusyEnd(matex_data);
        return 0;
      }
    }

    if (matex_data->busy == 2)
    {
      IupSetAttribute(matex_data->busy_progress_dlg, "INC", NULL);

      if (matex_data->busy_progress_abort)
      {
        iupMatrixExBusyEnd(matex_data);
        return 0;
      }
    }
  }
  return 1;
}

void iupMatrixExBusyEnd(ImatExData* matex_data)
{
  if (matex_data->busy)
  {
    IupStoreAttribute(matex_data->ih, "CURSOR", IupGetAttribute(matex_data->ih, "_IUPMATEX_OLDCURSOR"));
    IupSetAttribute(matex_data->ih, "_IUPMATEX_OLDCURSOR", NULL);

    if (matex_data->busy_cb)
      matex_data->busy_cb(matex_data->ih, 0, 0, NULL);

    if (matex_data->busy == 2)
    {
      IupHide(matex_data->busy_progress_dlg);
      IupSetAttribute(IupGetDialog(matex_data->ih),"ACTIVE","Yes");
      IupSetFocus(matex_data->ih);
    }

    if (matex_data->busy_undo_block)
      iupMatrixExUndoPushEnd(matex_data);

    matex_data->busy_count = 0;
    matex_data->busy_cb = NULL;
    matex_data->busy = 0;
    matex_data->busy_undo_block = 0;

    IupSetAttribute(matex_data->ih,"REDRAW","ALL");
  }
}

static int iMatrixSetBusyAttrib(Ihandle* ih, const char* value)
{
  /* can only be canceled */
  ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
  if (matex_data->busy && !iupStrBoolean(value))
    iupMatrixExBusyEnd(matex_data);
  return 0;
}

static char* iMatrixGetBusyAttrib(Ihandle* ih)
{
  ImatExData* matex_data = (ImatExData*)iupAttribGet(ih, "_IUP_MATEX_DATA");
  return iupStrReturnBoolean (matex_data->busy); 
}

void iupMatrixExRegisterBusy(Iclass* ic)
{
  iupClassRegisterCallback(ic, "BUSY_CB", "iis");

  iupClassRegisterAttribute(ic, "BUSY", iMatrixGetBusyAttrib, iMatrixSetBusyAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BUSYPROGRESS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
