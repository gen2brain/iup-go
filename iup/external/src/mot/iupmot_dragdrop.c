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
#include <X11/Xatom.h>

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

IUP_DRV_API void iupmotDisableDragSource(Widget w)
{
  static XtTranslations drag_translations = NULL;
  static int do_nothing_rec = 0;
  if (!do_nothing_rec)
  {
    XtActionsRec rec = {"iupDoNothing", (XtActionProc)motDoNothing};
    XtAppAddActions(iupmot_appcontext, &rec, 1);
    drag_translations = XtParseTranslationTable("#override <Btn2Down>: iupDoNothing()");
    do_nothing_rec = 1;
  }
  XtOverrideTranslations(w, drag_translations);
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

static void motDragCursorFinishCallback(Widget dragContext, XtPointer clientData, XtPointer callData)
{
  Widget sourceIcon = NULL, operationIcon = NULL;

  XtVaGetValues(dragContext, XmNsourcePixmapIcon, &sourceIcon, NULL);
  if (sourceIcon)
    XtDestroyWidget(sourceIcon);

  XtVaGetValues(dragContext, XmNoperationCursorIcon, &operationIcon, NULL);
  if (operationIcon)
    XtDestroyWidget(operationIcon);

  (void)clientData;
  (void)callData;
}

static Widget motCreateDragIcon(Widget w, const char* image_name, Ihandle* ih)
{
  Pixmap pixmap, mask;
  Arg args[10];
  int num_args = 0;

  pixmap = (Pixmap)iupImageGetImage(image_name, ih, 0, NULL);
  if (!pixmap)
    return NULL;

  mask = iupmotImageGetMask(image_name);

  iupMOT_SETARG(args, num_args, XmNpixmap, pixmap);
  if (mask)
    iupMOT_SETARG(args, num_args, XmNmask, mask);

  return XmCreateDragIcon(w, "drag_icon", args, num_args);
}

static void motDragStartFromEvent(Widget dragSource, Ihandle* ih, XEvent* evt)
{
  Widget dragContext;
  Widget drag_icon = NULL;
  Arg args[20];
  int num_args = 0;
  Atom *dragTypesList;
  Cardinal dragTypesListCount;
  char* value;

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

  value = iupAttribGet(ih, "DRAGCURSOR");
  if (value)
  {
    drag_icon = motCreateDragIcon(dragSource, value, ih);
    if (drag_icon)
      iupMOT_SETARG(args, num_args, XmNsourcePixmapIcon, drag_icon);
  }

  value = iupAttribGet(ih, "DRAGCURSORCOPY");
  if (value)
  {
    Widget copy_icon = motCreateDragIcon(dragSource, value, ih);
    if (copy_icon)
      iupMOT_SETARG(args, num_args, XmNoperationCursorIcon, copy_icon);
  }

  /* creates a XmDragContext */
  dragContext = XmDragStart(dragSource, evt, args, num_args);

  if(dragContext)
  {
    IFnii cbDragBegin;

    XtAddCallback(dragContext, XmNdropFinishCallback, (XtCallbackProc)motDropFinishCallback, (XtPointer)ih);

    if (drag_icon)
      XtAddCallback(dragContext, XmNdragDropFinishCallback, (XtCallbackProc)motDragCursorFinishCallback, NULL);

    cbDragBegin = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
    if(cbDragBegin)
    {
      if (cbDragBegin(ih, evt->xbutton.x, evt->xbutton.y) == IUP_IGNORE)
        XmDragCancel(dragContext);
    }
  }
}

#define MOT_DRAG_THRESHOLD 5

static void motDragSourceButtonHandler(Widget w, XtPointer client_data, XEvent* evt, Boolean* cont)
{
  Ihandle* ih = (Ihandle*)client_data;

  if (evt->type == ButtonPress && evt->xbutton.button == Button1)
  {
    iupAttribSetInt(ih, "_IUPMOT_DRAG_START_X", evt->xbutton.x);
    iupAttribSetInt(ih, "_IUPMOT_DRAG_START_Y", evt->xbutton.y);
    iupAttribSet(ih, "_IUPMOT_DRAG_PENDING", "1");
  }
  else if (evt->type == ButtonRelease && evt->xbutton.button == Button1)
  {
    iupAttribSet(ih, "_IUPMOT_DRAG_PENDING", NULL);
  }

  (void)cont;
  (void)w;
}

static void motDragSourceMotionHandler(Widget w, XtPointer client_data, XEvent* evt, Boolean* cont)
{
  Ihandle* ih = (Ihandle*)client_data;
  int startX, startY, dx, dy;

  if (!iupAttribGet(ih, "_IUPMOT_DRAG_PENDING"))
    return;

  startX = iupAttribGetInt(ih, "_IUPMOT_DRAG_START_X");
  startY = iupAttribGetInt(ih, "_IUPMOT_DRAG_START_Y");
  dx = evt->xmotion.x - startX;
  dy = evt->xmotion.y - startY;

  if (dx*dx + dy*dy > MOT_DRAG_THRESHOLD * MOT_DRAG_THRESHOLD)
  {
    XEvent press_event;

    iupAttribSet(ih, "_IUPMOT_DRAG_PENDING", NULL);

    /* Synthesize a ButtonPress event at the start position for XmDragStart */
    memset(&press_event, 0, sizeof(press_event));
    press_event.xbutton.type = ButtonPress;
    press_event.xbutton.display = evt->xmotion.display;
    press_event.xbutton.window = evt->xmotion.window;
    press_event.xbutton.root = evt->xmotion.root;
    press_event.xbutton.time = evt->xmotion.time;
    press_event.xbutton.x = startX;
    press_event.xbutton.y = startY;
    press_event.xbutton.x_root = evt->xmotion.x_root - dx;
    press_event.xbutton.y_root = evt->xmotion.y_root - dy;
    press_event.xbutton.button = Button1;
    press_event.xbutton.state = evt->xmotion.state;

    motDragStartFromEvent(w, ih, &press_event);
  }

  (void)cont;
}

static Atom* motCreateTargetList(const char *value, int *count)
{
  int count_alloc = 10;
  Atom *targetlist = (Atom*)XtMalloc(sizeof(Atom) * count_alloc);
  char valueCopy[256];
  char valueTemp1[256];
  char valueTemp2[256];

  *count = 0;

  iupStrCopyN(valueCopy, sizeof(valueCopy), value);
  while (iupStrToStrStr(valueCopy, valueTemp1, sizeof(valueTemp1), valueTemp2, sizeof(valueTemp2), ',') > 0)
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

    iupStrCopyN(valueCopy, sizeof(valueCopy), valueTemp2);
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
    XtAddEventHandler(w, ButtonPressMask|ButtonReleaseMask, False, motDragSourceButtonHandler, (XtPointer)ih);
    XtAddEventHandler(w, PointerMotionMask, False, motDragSourceMotionHandler, (XtPointer)ih);
  }
  else
  {
    XtRemoveEventHandler(w, ButtonPressMask|ButtonReleaseMask, False, motDragSourceButtonHandler, (XtPointer)ih);
    XtRemoveEventHandler(w, PointerMotionMask, False, motDragSourceMotionHandler, (XtPointer)ih);
    iupAttribSet(ih, "_IUPMOT_DRAG_PENDING", NULL);
  }

  return 1;
}

/******************************************************************************************/
/*                          XDND Protocol Support                                       */
/******************************************************************************************/

#define XDND_VERSION 5

static struct {
  Atom XdndAware;
  Atom XdndEnter;
  Atom XdndPosition;
  Atom XdndStatus;
  Atom XdndLeave;
  Atom XdndDrop;
  Atom XdndFinished;
  Atom XdndSelection;
  Atom XdndTypeList;
  Atom XdndActionCopy;
  Atom text_uri_list;
  int initialized;
} xdnd_atoms = {0};

typedef struct _IupmotXdndState {
  Window source;
  long version;
  Atom format;
  Widget shell;
} IupmotXdndState;

static Ihandle* mot_xdnd_target_ih = NULL;

static void motXdndInitAtoms(void)
{
  if (xdnd_atoms.initialized)
    return;

  xdnd_atoms.XdndAware     = XInternAtom(iupmot_display, "XdndAware", False);
  xdnd_atoms.XdndEnter     = XInternAtom(iupmot_display, "XdndEnter", False);
  xdnd_atoms.XdndPosition  = XInternAtom(iupmot_display, "XdndPosition", False);
  xdnd_atoms.XdndStatus    = XInternAtom(iupmot_display, "XdndStatus", False);
  xdnd_atoms.XdndLeave     = XInternAtom(iupmot_display, "XdndLeave", False);
  xdnd_atoms.XdndDrop      = XInternAtom(iupmot_display, "XdndDrop", False);
  xdnd_atoms.XdndFinished  = XInternAtom(iupmot_display, "XdndFinished", False);
  xdnd_atoms.XdndSelection = XInternAtom(iupmot_display, "XdndSelection", False);
  xdnd_atoms.XdndTypeList  = XInternAtom(iupmot_display, "XdndTypeList", False);
  xdnd_atoms.XdndActionCopy = XInternAtom(iupmot_display, "XdndActionCopy", False);
  xdnd_atoms.text_uri_list = XInternAtom(iupmot_display, "text/uri-list", False);

  xdnd_atoms.initialized = 1;
}

static int motXdndHexDigit(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static void motXdndDecodeURI(const char* src, char* dst, int dst_size)
{
  int i = 0;
  while (*src && i < dst_size - 1)
  {
    if (src[0] == '%' && src[1] && src[2])
    {
      int hi = motXdndHexDigit(src[1]);
      int lo = motXdndHexDigit(src[2]);
      if (hi >= 0 && lo >= 0)
      {
        dst[i++] = (char)((hi << 4) | lo);
        src += 3;
        continue;
      }
    }
    dst[i++] = *src++;
  }
  dst[i] = '\0';
}

static void motXdndCallDropFilesCB(Ihandle* ih, char* data, unsigned long length, int x, int y)
{
  IFnsiii cb;
  char* dataCopy;
  char* line;
  char* savePtr = NULL;
  char decoded[4096];
  int count = 0;
  int remaining;
  unsigned long i;

  cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
  if (!cb)
    return;

  dataCopy = (char*)malloc(length + 1);
  if (!dataCopy)
    return;
  memcpy(dataCopy, data, length);
  dataCopy[length] = '\0';

  for (i = 0; i < length; i++)
  {
    if (dataCopy[i] == '\n')
      count++;
  }
  if (length > 0 && dataCopy[length - 1] != '\n')
    count++;

  remaining = count - 1;
  line = strtok_r(dataCopy, "\r\n", &savePtr);

  while (line)
  {
    char* filename = line;

    if (filename[0] == '#')
    {
      line = strtok_r(NULL, "\r\n", &savePtr);
      continue;
    }

    if (strncmp(filename, "file://", 7) == 0)
      filename += 7;

    motXdndDecodeURI(filename, decoded, sizeof(decoded));

    if (cb(ih, decoded, remaining, x, y) == IUP_IGNORE)
      break;

    remaining--;
    line = strtok_r(NULL, "\r\n", &savePtr);
  }

  free(dataCopy);
}

static void motXdndSendFinished(IupmotXdndState* state, Window target_window, int success)
{
  if (state->version >= 2)
  {
    XEvent reply;
    memset(&reply, 0, sizeof(reply));
    reply.xclient.type = ClientMessage;
    reply.xclient.display = iupmot_display;
    reply.xclient.window = state->source;
    reply.xclient.message_type = xdnd_atoms.XdndFinished;
    reply.xclient.format = 32;
    reply.xclient.data.l[0] = (long)target_window;
    reply.xclient.data.l[1] = success ? 1 : 0;
    reply.xclient.data.l[2] = success ? (long)xdnd_atoms.XdndActionCopy : (long)None;

    XSendEvent(iupmot_display, state->source, False, NoEventMask, &reply);
    XFlush(iupmot_display);
  }
}

static void motXdndHandleEnter(Ihandle* ih, XClientMessageEvent* evt)
{
  IupmotXdndState* state = (IupmotXdndState*)iupAttribGet(ih, "_IUPMOT_XDND_STATE");
  long version;
  int has_list;
  Atom* formats = NULL;
  unsigned long count = 0;
  unsigned long i;

  if (!state)
    return;

  state->source = (Window)evt->data.l[0];
  version = (evt->data.l[1] >> 24) & 0xFF;
  state->version = version;
  state->format = None;

  if (version > XDND_VERSION)
    return;

  has_list = evt->data.l[1] & 1;

  if (has_list)
  {
    Atom actual_type;
    int actual_format;
    unsigned long bytes_after;
    unsigned char* prop_data = NULL;

    if (XGetWindowProperty(iupmot_display, state->source, xdnd_atoms.XdndTypeList,
                           0, 1024, False, XA_ATOM,
                           &actual_type, &actual_format, &count, &bytes_after,
                           &prop_data) == Success && prop_data)
    {
      formats = (Atom*)prop_data;
    }
  }
  else
  {
    static Atom inline_formats[3];
    count = 0;
    if (evt->data.l[2]) inline_formats[count++] = (Atom)evt->data.l[2];
    if (evt->data.l[3]) inline_formats[count++] = (Atom)evt->data.l[3];
    if (evt->data.l[4]) inline_formats[count++] = (Atom)evt->data.l[4];
    formats = inline_formats;
  }

  for (i = 0; i < count; i++)
  {
    if (formats[i] == xdnd_atoms.text_uri_list)
    {
      state->format = xdnd_atoms.text_uri_list;
      break;
    }
  }

  if (has_list && formats)
    XFree(formats);
}

static void motXdndHandlePosition(Ihandle* ih, Widget w, XClientMessageEvent* evt)
{
  IupmotXdndState* state = (IupmotXdndState*)iupAttribGet(ih, "_IUPMOT_XDND_STATE");
  Window target_window;
  int xabs, yabs, xpos, ypos;
  Window child;
  XEvent reply;

  if (!state || state->version > XDND_VERSION)
    return;

  target_window = XtWindow(state->shell);

  xabs = (int)((evt->data.l[2] >> 16) & 0xFFFF);
  yabs = (int)(evt->data.l[2] & 0xFFFF);

  XTranslateCoordinates(iupmot_display, DefaultRootWindow(iupmot_display), target_window, xabs, yabs, &xpos, &ypos, &child);

  iupAttribSetInt(ih, "_IUPMOT_XDND_DROP_X", xpos);
  iupAttribSetInt(ih, "_IUPMOT_XDND_DROP_Y", ypos);

  memset(&reply, 0, sizeof(reply));
  reply.xclient.type = ClientMessage;
  reply.xclient.display = iupmot_display;
  reply.xclient.window = state->source;
  reply.xclient.message_type = xdnd_atoms.XdndStatus;
  reply.xclient.format = 32;
  reply.xclient.data.l[0] = (long)target_window;
  reply.xclient.data.l[1] = (state->format != None) ? 1 : 0;
  reply.xclient.data.l[2] = 0;
  reply.xclient.data.l[3] = 0;
  reply.xclient.data.l[4] = (state->format != None && state->version >= 2) ? (long)xdnd_atoms.XdndActionCopy : (long)None;

  XSendEvent(iupmot_display, state->source, False, NoEventMask, &reply);
  XFlush(iupmot_display);

  (void)w;
}

static Bool motXdndSelectionPredicate(Display *display, XEvent *event, XPointer arg)
{
  (void)display;
  (void)arg;
  return (event->type == SelectionNotify &&
          event->xselection.selection == xdnd_atoms.XdndSelection);
}

static void motXdndHandleDrop(Ihandle* ih, Widget w, XClientMessageEvent* evt)
{
  IupmotXdndState* state = (IupmotXdndState*)iupAttribGet(ih, "_IUPMOT_XDND_STATE");
  Window target_window;

  if (!state || state->version > XDND_VERSION)
    return;

  target_window = XtWindow(state->shell);

  if (state->format != None)
  {
    Time timestamp = (state->version >= 1) ? (Time)evt->data.l[2] : CurrentTime;
    XEvent sel_event;
    int success = 0;

    XConvertSelection(iupmot_display, xdnd_atoms.XdndSelection, state->format, xdnd_atoms.XdndSelection, target_window, timestamp);
    XFlush(iupmot_display);

    /* Wait synchronously for SelectionNotify, bypassing Xt's selection dispatch */
    XIfEvent(iupmot_display, &sel_event, motXdndSelectionPredicate, NULL);

    {
      Atom actual_type;
      int actual_format;
      unsigned long count, bytes_after;
      unsigned char* data = NULL;

      if (XGetWindowProperty(iupmot_display, sel_event.xselection.requestor,
                             sel_event.xselection.property,
                             0, 1024 * 1024, True, AnyPropertyType,
                             &actual_type, &actual_format, &count, &bytes_after,
                             &data) == Success && data && count > 0)
      {
        int x = iupAttribGetInt(ih, "_IUPMOT_XDND_DROP_X");
        int y = iupAttribGetInt(ih, "_IUPMOT_XDND_DROP_Y");

        motXdndCallDropFilesCB(ih, (char*)data, count, x, y);
        success = 1;
      }

      if (data)
        XFree(data);
    }

    motXdndSendFinished(state, target_window, success);
  }
  else
    motXdndSendFinished(state, target_window, 0);

  state->source = None;
  state->version = 0;
  state->format = None;
  iupAttribSet(ih, "_IUPMOT_XDND_DROP_X", NULL);
  iupAttribSet(ih, "_IUPMOT_XDND_DROP_Y", NULL);

  (void)w;
}

static void motXdndHandleLeave(Ihandle* ih)
{
  IupmotXdndState* state = (IupmotXdndState*)iupAttribGet(ih, "_IUPMOT_XDND_STATE");
  if (!state)
    return;

  state->source = None;
  state->version = 0;
  state->format = None;

  iupAttribSet(ih, "_IUPMOT_XDND_DROP_X", NULL);
  iupAttribSet(ih, "_IUPMOT_XDND_DROP_Y", NULL);
}

static void motXdndAction(Widget w, XEvent* event, String* params, Cardinal* num_params)
{
  Ihandle* ih = mot_xdnd_target_ih;
  XClientMessageEvent* evt;

  if (!event || event->type != ClientMessage)
    return;
  if (!ih)
    return;

  evt = &event->xclient;

  if (evt->message_type == xdnd_atoms.XdndEnter)
    motXdndHandleEnter(ih, evt);
  else if (evt->message_type == xdnd_atoms.XdndPosition)
    motXdndHandlePosition(ih, w, evt);
  else if (evt->message_type == xdnd_atoms.XdndDrop)
    motXdndHandleDrop(ih, w, evt);
  else if (evt->message_type == xdnd_atoms.XdndLeave)
    motXdndHandleLeave(ih);

  (void)params;
  (void)num_params;
}

static int motSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  Widget w = (Widget)iupAttribGet(ih, "_IUPMOT_DND_WIDGET");
  if (!w)
    w = ih->handle;

  motXdndInitAtoms();

  if (iupStrBoolean(value))
  {
    IupmotXdndState* state;
    Widget shell;
    Window xwin;
    Atom version = XDND_VERSION;

    if (iupAttribGet(ih, "_IUPMOT_XDND_STATE"))
      return 1;

    shell = w;
    while (!XtIsShell(shell))
      shell = XtParent(shell);

    xwin = XtWindow(shell);
    if (!xwin)
      return 0;

    state = (IupmotXdndState*)calloc(1, sizeof(IupmotXdndState));
    state->shell = shell;
    iupAttribSet(ih, "_IUPMOT_XDND_STATE", (char*)state);

    /* Set XdndAware on both widget window and shell window */
    XChangeProperty(iupmot_display, xwin, xdnd_atoms.XdndAware, XA_ATOM, 32, PropModeReplace, (unsigned char*)&version, 1);
    if (XtWindow(w) != xwin)
      XChangeProperty(iupmot_display, XtWindow(w), xdnd_atoms.XdndAware, XA_ATOM, 32, PropModeReplace, (unsigned char*)&version, 1);

    /* Remove Motif DND receiver info so file managers use XDND instead of Motif DND */
    {
      Atom motif_receiver = XInternAtom(iupmot_display, "_MOTIF_DRAG_RECEIVER_INFO", True);
      if (motif_receiver != None)
        XDeleteProperty(iupmot_display, xwin, motif_receiver);
    }

    /* Register Xt action and translation table for XDND messages */
    {
      static int action_registered = 0;
      if (!action_registered)
      {
        XtActionsRec rec = {"iupXdndAction", (XtActionProc)motXdndAction};
        XtAppAddActions(iupmot_appcontext, &rec, 1);
        action_registered = 1;
      }
    }
    XtOverrideTranslations(shell, XtParseTranslationTable(
      "<Message>XdndEnter:    iupXdndAction()\n"
      "<Message>XdndPosition: iupXdndAction()\n"
      "<Message>XdndLeave:    iupXdndAction()\n"
      "<Message>XdndDrop:     iupXdndAction()"));

    mot_xdnd_target_ih = ih;
  }
  else
  {
    IupmotXdndState* state = (IupmotXdndState*)iupAttribGet(ih, "_IUPMOT_XDND_STATE");
    if (state)
    {
      Widget shell = state->shell;
      Window xwin = XtWindow(shell);

      if (xwin)
      {
        XDeleteProperty(iupmot_display, xwin, xdnd_atoms.XdndAware);
        if (XtWindow(w) != xwin)
          XDeleteProperty(iupmot_display, XtWindow(w), xdnd_atoms.XdndAware);
      }

      free(state);
      iupAttribSet(ih, "_IUPMOT_XDND_STATE", NULL);
      mot_xdnd_target_ih = NULL;
    }
  }

  return 1;
}

/******************************************************************************************/

IUP_SDK_API void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");

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
  iupClassRegisterAttribute(ic, "DRAGCURSOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, motSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, motSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
