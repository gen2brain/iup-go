/** \file
 * \brief Motif Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <Xm/Xm.h>
#include <Xm/ScrollBar.h>
#include <Xm/DragDrop.h>
#include <X11/cursorfont.h>

#include "iup.h"
#include "iupkey.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_key.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_image.h"

#include "iupmot_color.h"
#include "iupmot_drv.h"


static void motDoNothing(Widget w, XEvent*  evt, String* params, Cardinal* num_params)
{
  (void)w;
  (void)evt;
  (void)params;
  (void)num_params;
}

void iupmotDisableDragSource(Widget w)
{
  char dragTranslations[] = "#override <Btn2Down>: iupDoNothing()";
  static int do_nothing_rec = 0;
  if (!do_nothing_rec)
  {
    XtActionsRec rec = {"iupDoNothing", (XtActionProc)motDoNothing};
    XtAppAddActions(iupmot_appcontext, &rec, 1);
    do_nothing_rec = 1;
  }
  XtOverrideTranslations(w, XtParseTranslationTable(dragTranslations));
}

static void motDropTransferProc(Widget dropTransfer, Ihandle* ih, Atom *selType, Atom *typeAtom,
                                XtPointer targetData, unsigned long *length, int format)
{
  IFnsViii cbDropData;

  if(!targetData || !(*length))
    return;

  cbDropData = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
  if(cbDropData)
  {
    /* TODO should we check for incompatible targets here? */
    char* type = XGetAtomName(iupmot_display, *typeAtom);
    int x = iupAttribGetInt(ih, "_IUPMOT_DROP_X");
    int y = iupAttribGetInt(ih, "_IUPMOT_DROP_Y");
    
    cbDropData(ih, type, (void*)targetData, (int)*length, x, y);

    iupAttribSet(ih, "_IUPMOT_DROP_X", NULL);
    iupAttribSet(ih, "_IUPMOT_DROP_Y", NULL);
  }

  (void)dropTransfer;
  (void)format;
  (void)selType;
}

static void motDragProc(Widget dropTarget, XtPointer clientData, XmDragProcCallbackStruct* cbs)
{
  (void)clientData;
  if(cbs->reason == XmCR_DROP_SITE_MOTION_MESSAGE)
  {
    Ihandle* ih = NULL;
    int x = cbs->x;
    int y = cbs->y;
    IFniis cbDropMotion;

    XtVaGetValues(dropTarget, XmNuserData, &ih, NULL);  

    cbDropMotion = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");

    if(cbDropMotion)
    {
      int z;
      Window window;
      unsigned int state;
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

      XQueryPointer(iupmot_display, DefaultRootWindow(iupmot_display), &window, &window, &z, &z, &z, &z, &state);
      iupmotButtonKeySetStatus(state, 0, status, 0);

      cbDropMotion(ih, x, y, status);
    }
  }
}

static void motDropProc(Widget dropTarget, XtPointer clientData, XmDropProcCallbackStruct* dropData)
{
  XmDropTransferEntryRec transferList[2];
  Arg args[20];
  int i, j, num_args;
  Widget dragContext, dropTransfer;
  Cardinal numDragTypes, numDropTypes;
  Atom *dragTypesList, *dropTypesList;
  Atom atomItem;
  Boolean found = False;
  Ihandle *ih = NULL;

  /* this is called before drag data is processed */ 
  dragContext = dropData->dragContext;

  /* Getting drop types */
  num_args = 0;
  iupMOT_SETARG(args, num_args, XmNimportTargets, &dropTypesList);
  iupMOT_SETARG(args, num_args, XmNnumImportTargets, &numDropTypes);
  XmDropSiteRetrieve (dropTarget, args, num_args);
  if(!numDropTypes)  /* no type registered */
    return;

  /* Getting drag types */
  XtVaGetValues(dragContext, XmNexportTargets, &dragTypesList,
                             XmNnumExportTargets, &numDragTypes,
                             NULL);
  if(!numDragTypes)  /* no type registered */
    return;

  /* Checking the type compatibility */ 
  for (i = 0; i < (int)numDragTypes; i++) 
  {
    for (j = 0; j < (int)numDropTypes; j++) 
    {
      if(iupStrEqualNoCase(XGetAtomName(iupmot_display, dragTypesList[i]),
                           XGetAtomName(iupmot_display, dropTypesList[j])))
      {
        atomItem = dropTypesList[j];
        found = True;
        break;
      }
    }
    if(found == True)
      break;
  }

  num_args = 0;
  if ((!found) || 
      (dropData->dropAction != XmDROP) ||  
      (dropData->operation != XmDROP_COPY && dropData->operation != XmDROP_MOVE)) 
  {
    iupMOT_SETARG(args, num_args, XmNtransferStatus, XmTRANSFER_FAILURE);
    iupMOT_SETARG(args, num_args, XmNnumDropTransfers, 0);
  }
  else 
  {
    XtVaGetValues(dropTarget, XmNuserData, &ih, NULL);

    iupAttribSetInt(ih, "_IUPMOT_DROP_X", (int)dropData->x);
    iupAttribSetInt(ih, "_IUPMOT_DROP_Y", (int)dropData->y);

    /* set up transfer requests for drop site */
    transferList[0].target = atomItem;
    transferList[0].client_data = (XtPointer)ih;

    iupMOT_SETARG(args, num_args, XmNdropTransfers, transferList);
    iupMOT_SETARG(args, num_args, XmNnumDropTransfers, 1);
    iupMOT_SETARG(args, num_args, XmNtransferProc, motDropTransferProc);
  }

  /* creates a XmDropTransfer (not used here) */
  dropTransfer = XmDropTransferStart(dragContext, args, num_args);

  (void)dropTransfer;
  (void)clientData;
}

static Boolean motDragConvertProc(Widget dragContext, Atom *selection, Atom *target, Atom *typeReturn,
                                  XtPointer *valueReturn, unsigned long *lengthReturn, int *formatReturn)
{
  Atom atomMotifDrop = XInternAtom(iupmot_display, "_MOTIF_DROP", False);
  Ihandle *ih = NULL;
  IFnsVi cbDragData;
  IFns cbDragDataSize;

  /* check if we are dealing with a drop */
  if (*selection != atomMotifDrop)
    return False;

  XtVaGetValues(dragContext, XmNclientData, &ih, NULL);

  cbDragData = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");
  cbDragDataSize = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
  if(cbDragData && cbDragDataSize)
  {
    void* sourceData;
    /* TODO should we check for incompatible targets here? */
    char* type = XGetAtomName(iupmot_display, *target);
    int size = cbDragDataSize(ih, type);
    if (size <= 0)
      return False;

    sourceData = XtMalloc(size);  /* data will be released by the system */
    
    /* fill data */
    cbDragData(ih, type, sourceData, size);

    /* format the value for transfer */
    *typeReturn = *target;
    *valueReturn = (XtPointer)sourceData;
    *lengthReturn = size;
    *formatReturn = 8;

    return True;
  }

  return False;
}

static void motDropFinishCallback(Widget dragContext, Ihandle *ih, XmDropFinishCallbackStruct *callData)
{
  IFni cbDrag = (IFni)IupGetCallback(ih, "DRAGEND_CB");
  if(cbDrag)
  {
    int remove = -1;
    if (callData->dropAction==XmDROP && 
        callData->completionStatus==XmDROP_SUCCESS)
    {
      if (callData->operation==XmDROP_MOVE)
        remove = 1;
      else if (callData->operation==XmDROP_COPY)
        remove = 0;
    }
    cbDrag(ih, remove);
  }

  (void)dragContext;
}

static void motDragStart(Widget dragSource, XButtonEvent* evt, String* params, Cardinal* num_params)
{
  Widget dragContext;
  Arg args[20];
  int num_args = 0;
  Atom *dragTypesList;
  Cardinal dragTypesListCount;
  Ihandle* ih = NULL;

  XtVaGetValues(dragSource, XmNuserData, &ih, NULL);

  dragTypesList = (Atom*)iupAttribGet(ih, "_IUPMOT_DRAG_TARGETLIST");
  dragTypesListCount =  (Cardinal)iupAttribGetInt(ih, "_IUPMOT_DRAG_TARGETLIST_COUNT");
  if (!dragTypesList)
    return;

  /* specify resources for DragContext for the transfer */
  num_args = 0;
  iupMOT_SETARG(args, num_args, XmNexportTargets, dragTypesList);
  iupMOT_SETARG(args, num_args, XmNnumExportTargets, dragTypesListCount);
  iupMOT_SETARG(args, num_args, XmNdragOperations, iupAttribGetBoolean(ih, "DRAGSOURCEMOVE")? XmDROP_MOVE|XmDROP_COPY: XmDROP_COPY);
  iupMOT_SETARG(args, num_args, XmNconvertProc, motDragConvertProc);
  iupMOT_SETARG(args, num_args, XmNinvalidCursorForeground, iupmotColorGetPixel(255, 0, 0));
  iupMOT_SETARG(args, num_args, XmNclientData, ih);

  /* creates a XmDragContext */
  dragContext = XmDragStart(dragSource, (XEvent*)evt, args, num_args);

  if(dragContext)
  {
    IFnii cbDragBegin;

    XtAddCallback(dragContext, XmNdropFinishCallback, (XtCallbackProc)motDropFinishCallback, (XtPointer)ih);

    cbDragBegin = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
    if(cbDragBegin)
    {
      if (cbDragBegin(ih, evt->x, evt->y) == IUP_IGNORE)
        XmDragCancel(dragContext);
    }
  }

  (void)params;
  (void)num_params;
}

static Atom* motCreateTargetList(const char *value, int *count)
{
  int count_alloc = 10;
  Atom *targetlist = (Atom*)XtMalloc(sizeof(Atom) * count_alloc);
  char valueCopy[256];
  char valueTemp1[256];
  char valueTemp2[256];

  *count = 0;

  strcpy(valueCopy, value);
  while (iupStrToStrStr(valueCopy, valueTemp1, valueTemp2, ',') > 0)
  {
    targetlist[*count] = XInternAtom(iupmot_display, (char*)valueTemp1, False);
    (*count)++;

    if (iupStrEqualNoCase(valueTemp2, valueTemp1))
      break;

    if (*count == count_alloc)
    {
      count_alloc += 10;
      targetlist = (Atom*)XtRealloc((char*)targetlist, sizeof(Atom) * count_alloc);
    }

    strcpy(valueCopy, valueTemp2);
  }

  if (*count == 0)
  {
    XtFree((char*)targetlist);
    return NULL;
  }

  return targetlist;
}


/******************************************************************************************/


static int motSetDropTypesAttrib(Ihandle* ih, const char* value)
{
  int count = 0;
  Atom *targetlist = (Atom*)iupAttribGet(ih, "_IUPMOT_DROP_TARGETLIST");
  if (targetlist)
  {
    XtFree((char*)targetlist);
    iupAttribSet(ih, "_IUPMOT_DROP_TARGETLIST", NULL);
    iupAttribSet(ih, "_IUPMOT_DROP_TARGETLIST_COUNT", NULL);
  }

  if(!value)
    return 0;

  targetlist = motCreateTargetList(value, &count);
  if (targetlist)
  {
    iupAttribSet(ih, "_IUPMOT_DROP_TARGETLIST", (char*)targetlist);
    iupAttribSetInt(ih, "_IUPMOT_DROP_TARGETLIST_COUNT", count);
  }

  return 1;
}

static int motSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  Widget w = (Widget)iupAttribGet(ih, "_IUPMOT_DND_WIDGET");
  if (!w)
    w = ih->handle;

  if(iupStrBoolean(value))
  {
    Atom *dropTypesList = (Atom*)iupAttribGet(ih, "_IUPMOT_DROP_TARGETLIST");
    Cardinal numDropTypes = (Cardinal)iupAttribGetInt(ih, "_IUPMOT_DROP_TARGETLIST_COUNT");
    Arg args[20];
    int num_args = 0;

    if (!dropTypesList)
      return 0;

    iupMOT_SETARG(args, num_args, XmNimportTargets, dropTypesList);
    iupMOT_SETARG(args, num_args, XmNnumImportTargets, numDropTypes);
    iupMOT_SETARG(args, num_args, XmNdropProc, motDropProc);
    iupMOT_SETARG(args, num_args, XmNdragProc, motDragProc);
    XmDropSiteUpdate(w, args, num_args);

    XtVaSetValues(w, XmNuserData, ih, NULL);  /* Warning: always check if this affects other controls */
  }
  else
    XmDropSiteUnregister(w);

  return 1;
}

static int motSetDragTypesAttrib(Ihandle* ih, const char* value)
{
  int count = 0;
  Atom *targetlist = (Atom*)iupAttribGet(ih, "_IUPMOT_DRAG_TARGETLIST");
  if (targetlist)
  {
    XtFree((char*)targetlist);
    iupAttribSet(ih, "_IUPMOT_DRAG_TARGETLIST", NULL);
    iupAttribSet(ih, "_IUPMOT_DRAG_TARGETLIST_COUNT", NULL);
  }

  if(!value)
    return 0;

  targetlist = motCreateTargetList(value, &count);
  if (targetlist)
  {
    iupAttribSet(ih, "_IUPMOT_DRAG_TARGETLIST", (char*)targetlist);
    iupAttribSetInt(ih, "_IUPMOT_DRAG_TARGETLIST_COUNT", count);
  }

  return 1;
}

static int motSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  Widget w = (Widget)iupAttribGet(ih, "_IUPMOT_DND_WIDGET");
  if (!w)
    w = ih->handle;

  if(iupStrBoolean(value))
  {
    char dragTranslations[] = "#override <Btn2Down>: iupStartDrag()";
    static int do_rec = 0;

    if (!do_rec)
    {
      XtActionsRec rec = {"iupStartDrag", (XtActionProc)motDragStart};
      XtAppAddActions(iupmot_appcontext, &rec, 1);
      do_rec = 1;
    }
    XtOverrideTranslations(w, XtParseTranslationTable(dragTranslations));

    XtVaSetValues(w, XmNuserData, ih, NULL);  /* Warning: always check if this affects other controls */
  }
  else
    iupmotDisableDragSource(w);

  return 1;
}


/******************************************************************************************/


void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  /* Not Supported */
  /* iupClassRegisterCallback(ic, "DROPFILES_CB", "siii"); */

  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");

  iupClassRegisterAttribute(ic, "DRAGTYPES",  NULL, motSetDragTypesAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES",  NULL, motSetDropTypesAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, motSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", NULL, motSetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}

/*TODO:
  Drop on IupList with Edit box, only works on edit box
*/
