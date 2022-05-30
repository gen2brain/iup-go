/** \file
 * \brief Motif IupFileDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <errno.h>

#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/FileSB.h>
#include <Xm/Protocols.h>
#include <Xm/SelectioB.h>
#include <Xm/MessageB.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/Frame.h>
#include <Xm/List.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_dialog.h"
#include "iup_strmessage.h"
#include "iup_drvinfo.h"
#include "iup_array.h"
#include "iup_predialogs.h"
#include "iup_key.h"

#include "iupmot_drv.h"


enum {IUP_DIALOGOPEN, IUP_DIALOGSAVE, IUP_DIALOGDIR};


static void motFileDlgAskUserCBclose(Widget w, XtPointer client_data, XtPointer call_data)
{
  int *ret_code = (int*)client_data;
  if (!ret_code) return;
  (void)call_data;
  (void)w;
  *ret_code = -1;
}

static void motFileDlgAskUserCallback(Widget w, int* ret_code, XmSelectionBoxCallbackStruct* cbs)
{
  (void)w;
  if (cbs->reason == XmCR_OK)
    *ret_code = 1;
  else
    *ret_code = -1;
}

static int motFileDlgAskUser(Widget parent, const char* message)
{
  Widget questionbox;
  Arg args[3];
  int ret_code = 0;
  XmString title;

  XtSetArg(args[0], XmNautoUnmanage, False);
  XtSetArg(args[1], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
  XtSetArg(args[2], XmNnoResize, True);
  questionbox = XmCreateQuestionDialog(parent, "filedlg_question", args, 3);
  iupmotSetXmString(questionbox, XmNmessageString, message);
  XtVaGetValues(parent, XmNdialogTitle, &title, NULL);
  XtVaSetValues(questionbox, XmNdialogTitle, title, NULL);

  XmAddWMProtocolCallback(XtParent(questionbox), iupmot_wm_deletewindow, motFileDlgAskUserCBclose, (XtPointer)&ret_code);

  XtAddCallback(questionbox, XmNokCallback, (XtCallbackProc)motFileDlgAskUserCallback, (XtPointer)&ret_code);
  XtAddCallback(questionbox, XmNcancelCallback, (XtCallbackProc)motFileDlgAskUserCallback, (XtPointer)&ret_code);
  XtUnmanageChild(XmMessageBoxGetChild(questionbox, XmDIALOG_HELP_BUTTON));
  XtManageChild(questionbox);

  while (ret_code == 0)
    XtAppProcessEvent(iupmot_appcontext, XtIMAll);

  XtDestroyWidget(XtParent(questionbox));

  if (ret_code == 1)
    return 1;
  else
    return 0;
}

static int motFileDlgCheckValue(Ihandle* ih, Widget filebox)
{
  char* value = iupAttribGet(ih, "VALUE");
  int dialogtype = iupAttribGetInt(ih, "_IUPDLG_DIALOGTYPE");

  if (dialogtype == IUP_DIALOGDIR)
  {
    if (!iupUnixIsDirectory(value))               /* if does not exist or not a directory */
    {
      IupMessageError(ih, "IUP_INVALIDDIR");
      return 0;
    }
  }
  else if (!iupAttribGetBoolean(ih, "MULTIPLEFILES"))
  {
    if (iupUnixIsDirectory(value))  /* selected a directory */
    {
      IupMessageError(ih, "IUP_INVALIDDIR");
      return 0;
    }
    else if (!iupUnixIsFile(value))      /* not a file == new file */
    {
      value = iupAttribGet(ih, "ALLOWNEW");
      if (!value)
      {
        if (dialogtype == IUP_DIALOGSAVE)
          value = "YES";
        else
          value = "NO";
      }

      if (!iupStrBoolean(value))
      {
        IupMessageError(ih, "IUP_FILENOTEXIST");
        return 0;
      }
    }
    else if (dialogtype == IUP_DIALOGSAVE && !iupAttribGetInt(ih, "NOOVERWRITEPROMPT"))
    {
      if (!motFileDlgAskUser(filebox, IupGetLanguageString("IUP_FILEOVERWRITE")))
        return 0;
    }
  }

  return 1;
}

static void motFileDlgCBclose(Widget w, XtPointer client_data, XtPointer call_data)
{
  Ihandle *ih = (Ihandle*)client_data;
  if (!ih) return;
  (void)call_data;
  (void)w;
  iupAttribSet(ih, "VALUE", NULL);
  iupAttribSet(ih, "STATUS", "-1");
  iupAttribSet(ih, "_IUP_WM_DELETE", "1");
}

static int motFileDlgGetMultipleFiles(Ihandle* ih, const char* dir, Widget wList)
{
  int *pos, sel_count, dir_len;
  char *filename;
  XmString* items;

  if (!XmListGetSelectedPos(wList, &pos, &sel_count))
    return 0;

  dir_len = (int)strlen(dir);

  XtVaGetValues(wList, XmNitems, &items, NULL);

  /* check if just one file is selected */
  if (sel_count == 1)
  {
    filename = iupmotGetXmString(items[pos[0] - 1]);  /* XmListGetSelectedPos starts at 1 */
    if (filename)
    {
      iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);  /* same as directory, includes last separator */

      if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
        dir_len = 0;

      iupAttribSetStrId(ih, "MULTIVALUE", 1, filename + dir_len);

      iupAttribSetStr(ih, "VALUE", filename);  /* here value is not separated by '|' */

      iupAttribSetInt(ih, "MULTIVALUECOUNT", 2);

      XtFree(filename);
    }
  }
  else
  {
    Iarray* names_array = iupArrayCreate(1024, sizeof(char));  /* just set an initial size, but count is 0 */
    char *all_names;
    int i, cur_len, count = 0;

    int len = dir_len;
    if (dir[dir_len - 1] == '/') len--;  /* remove last '/' */

    all_names = iupArrayAdd(names_array, len + 1);
    memcpy(all_names, dir, len);  /* does NOT includes last separator */
    all_names[len] = '|';

    iupAttribSetStrId(ih, "MULTIVALUE", 0, dir);  /* same as directory, includes last separator */
    count++;

    if (iupAttribGetBoolean(ih, "MULTIVALUEPATH"))
      dir_len = 0;

    for (i = 0; i < sel_count; i++)
    {
      filename = iupmotGetXmString(items[pos[i] - 1]);  /* XmListGetSelectedPos starts at 1 */
      if (filename)
      {
        len = (int)strlen(filename) - dir_len;

        cur_len = iupArrayCount(names_array);

        all_names = iupArrayAdd(names_array, len + 1);
        memcpy(all_names + cur_len, filename + dir_len, len);
        all_names[cur_len + len] = '|';

        iupAttribSetStrId(ih, "MULTIVALUE", count, filename + dir_len);
        count++;

        XtFree(filename);
      }
    }

    iupAttribSetInt(ih, "MULTIVALUECOUNT", count);

    cur_len = iupArrayCount(names_array);
    all_names = iupArrayInc(names_array);
    all_names[cur_len + 1] = 0;

    /* replace VALUE with the correct sequence */
    iupAttribSetStr(ih, "VALUE", all_names);

    iupArrayDestroy(names_array);
  }

  XtFree((char*)pos);
  return 1;
}

static char* motFileCheckExt(Ihandle* ih, const char* filename)
{
  char* ext = iupAttribGet(ih, "EXTDEFAULT");
  if (ext)
  {
    int len = (int)strlen(filename);
    int ext_len = (int)strlen(ext);
    if (filename[len - ext_len - 1] != '.')
    {
      char* new_filename = XtMalloc(len + ext_len + 1 + 1);
      memcpy(new_filename, filename, len);
      new_filename[len] = '.';
      memcpy(new_filename + len + 1, ext, ext_len);
      new_filename[len + ext_len + 1] = 0;
      return new_filename;
    }
  }

  return (char*)filename;
}

static void motFileDlgCallback(Widget filebox, Ihandle* ih, XmFileSelectionBoxCallbackStruct* call_data)
{
  if (call_data->reason == XmCR_OK)
  {
    int dialogtype = iupAttribGetInt(ih, "_IUPDLG_DIALOGTYPE");
    char* filename;
    XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &filename);
    filename = motFileCheckExt(ih, filename);
    iupAttribSetStr(ih, "VALUE", filename);  /* this will be replaced for multiple files */
    XtFree(filename);

    if (!motFileDlgCheckValue(ih, filebox))
      return;  /* do not update STATUS */

    if (dialogtype == IUP_DIALOGDIR)
    {
      iupAttribSet(ih, "STATUS", "0");
      iupAttribSet(ih, "FILEEXIST", NULL);
    }
    else if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
    {
      Widget wList = XmFileSelectionBoxGetChild(filebox, XmDIALOG_LIST);

      /* here value obtained above contains exactly the directory */
      char* dir = iupAttribGet(ih, "VALUE");
      iupAttribSetStr(ih, "DIRECTORY", dir);

      if (!motFileDlgGetMultipleFiles(ih, iupAttribGet(ih, "DIRECTORY"), wList))
      {
        IupMessageError(ih, "IUP_FILENOTEXIST");
        return; /* do not update STATUS */
      }

      iupAttribSet(ih, "STATUS", "0");
      iupAttribSet(ih, "FILEEXIST", "YES");
    }
    else
    {
      IFnss file_cb = (IFnss)IupGetCallback(ih, "FILE_CB");
      filename = iupAttribGet(ih, "VALUE");
      if (file_cb)
      {
        int ret = file_cb(ih, filename, "OK");
        if (ret == IUP_IGNORE || ret == IUP_CONTINUE)
        {
          if (ret == IUP_CONTINUE)
          {
            char* file = iupAttribGet(ih, "FILE");
            if (file)
            {
              iupmotSetXmString(filebox, XmNdirSpec, "");
              iupmotSetXmString(filebox, XmNdirSpec, file);
            }
          }

          return;  /* do not update STATUS */
        }
      }

      /* store the directory */
      {
        char* dir = iupStrFileGetPath(filename);
        iupAttribSetStr(ih, "DIRECTORY", dir);
        free(dir);
      }

      if (iupUnixIsFile(filename))  /* check if file exists */
      {
        iupAttribSet(ih, "FILEEXIST", "YES");
        iupAttribSet(ih, "STATUS", "0");
      }
      else
      {
        iupAttribSet(ih, "FILEEXIST", "NO");
        iupAttribSet(ih, "STATUS", "1");
      }
    }

    if (!iupAttribGetBoolean(ih, "NOCHANGEDIR"))  /* do change the current directory */
    {
      /* XmFileSelection does not change the current directory */
      XmString xm_dir;
      char* dir;
      XtVaGetValues(filebox, XmNdirectory, &xm_dir, NULL);
      XmStringGetLtoR(xm_dir, XmSTRING_DEFAULT_CHARSET, &dir);
      iupdrvSetCurrentDirectory(dir);
      XtFree(dir);
    }
  }
  else if (call_data->reason == XmCR_CANCEL)
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "FILEEXIST", NULL);
    iupAttribSet(ih, "STATUS", "-1");
  }
}

static void motFileDlgHelpCallback(Widget w, Ihandle *ih, XtPointer call_data)
{
  Icallback cb = IupGetCallback(ih, "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
  {
    iupAttribSet(ih, "VALUE", NULL);
    iupAttribSet(ih, "FILEEXIST", NULL);
    iupAttribSet(ih, "STATUS", "-1");
  }

  (void)call_data;
  (void)w;
}

typedef struct _ImotPromt
{
  XmString xm_dir;
  int ret_code;
} ImotPromt;

static void motFileDlgPromptCBclose(Widget w, ImotPromt* prompt, XtPointer call_data)
{
  (void)call_data;
  (void)w;
  prompt->ret_code = 1;
}

static void motFileDlgPromptCallback(Widget w, ImotPromt* prompt, XmSelectionBoxCallbackStruct* cbs)
{
  (void)w;
  if (cbs->reason == XmCR_OK)
    prompt->xm_dir = XmStringCopy(cbs->value);

  prompt->ret_code = 1;
}

static XmString motFileDlgPrompt(Widget parent, const char* message)
{
  Widget promptbox;
  Arg args[2];
  ImotPromt prompt;
  XmString title;

  XtSetArg(args[0], XmNautoUnmanage, False);
  XtSetArg(args[1], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
  promptbox = XmCreatePromptDialog(parent, "filedlg_prompt", args, 2);
  iupmotSetXmString(promptbox, XmNselectionLabelString, message);
  XtVaGetValues(parent, XmNdialogTitle, &title, NULL);
  XtVaSetValues(promptbox, XmNdialogTitle, title, NULL);

  prompt.ret_code = 0;
  prompt.xm_dir = NULL;

  XmAddWMProtocolCallback(XtParent(promptbox), iupmot_wm_deletewindow, (XtCallbackProc)motFileDlgPromptCBclose, (XtPointer)&prompt);

  XtAddCallback(promptbox, XmNokCallback, (XtCallbackProc)motFileDlgPromptCallback, (XtPointer)&prompt);
  XtAddCallback(promptbox, XmNcancelCallback, (XtCallbackProc)motFileDlgPromptCallback, (XtPointer)&prompt);
  XtUnmanageChild(XmSelectionBoxGetChild(promptbox, XmDIALOG_HELP_BUTTON));
  XtManageChild(promptbox);

  while (prompt.ret_code == 0)
    XtAppProcessEvent(iupmot_appcontext, XtIMAll);

  XtDestroyWidget(XtParent(promptbox));

  return prompt.xm_dir;
}

static void motFileDlgNewFolderCallback(Widget w, Widget filebox, XtPointer call_data)
{
  XmString xm_new_dir = motFileDlgPrompt(filebox, IupGetLanguageString("IUP_NAMENEWFOLDER"));
  if (xm_new_dir)
  {
    XmString xm_dir;
    XtVaGetValues(filebox, XmNdirectory, &xm_dir, NULL);
    xm_dir = XmStringConcat(xm_dir, xm_new_dir);

    {
      char* dir;
      XmStringGetLtoR(xm_dir, XmSTRING_DEFAULT_CHARSET, &dir);
      iupUnixMakeDirectory(dir);
      XtFree(dir);
    }

    XtVaSetValues(filebox, XmNdirectory, xm_dir, NULL);

    XmStringFree(xm_dir);
    XmStringFree(xm_new_dir);
  }

  (void)call_data;
  (void)w;
}

static void motFileDlgPreviewCanvasInputCallback(Widget w, Ihandle *ih, XtPointer call_data)
{
  XEvent *evt = ((XmDrawingAreaCallbackStruct*)call_data)->event;

  if (!XtWindow(w) || !ih) return;

  switch (evt->type)
  {
  case ButtonPress:
  case ButtonRelease:
  {
    XButtonEvent *but_evt = (XButtonEvent*)evt;
    Boolean cont = True;
    iupmotButtonPressReleaseEvent(w, ih, evt, &cont);
    if (cont == False)
      return;

    if ((evt->type == ButtonPress) && (but_evt->button == Button4 || but_evt->button == Button5))
    {
      IFnfiis wcb = (IFnfiis)IupGetCallback(ih, "WHEEL_CB");
      if (wcb)
      {
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        int delta = but_evt->button == Button4 ? 1 : -1;
        iupmotButtonKeySetStatus(but_evt->state, but_evt->button, status, 0);

        wcb(ih, (float)delta, but_evt->x, but_evt->y, status);
      }
    }
  }
  break;
  }
}

static void motFileDlgPreviewCanvasResizeCallback(Widget w, Ihandle *ih, XtPointer call_data)
{
  Dimension width, height;
  XtVaGetValues(w, XmNwidth, &width,
                   XmNheight, &height,
                   NULL);

  iupAttribSetInt(ih, "PREVIEWWIDTH", width);
  iupAttribSetInt(ih, "PREVIEWHEIGHT", height);

  (void)call_data;
}

static void motFileDlgUpdatePreviewGLCanvas(Ihandle* ih)
{
  Ihandle* glcanvas = IupGetAttributeHandle(ih, "PREVIEWGLCANVAS");
  if (glcanvas)
  {
    iupAttribSet(glcanvas, "XWINDOW", iupAttribGet(ih, "XWINDOW"));
    glcanvas->iclass->Map(glcanvas);
  }
}

static void motFileDlgPreviewCanvasInit(Ihandle *ih, Widget w)
{
  XSetWindowAttributes attrs;
  GC gc = XCreateGC(iupmot_display, XtWindow(w), 0, NULL);
  iupAttribSet(ih, "PREVIEWDC", (char*)gc);
  iupAttribSet(ih, "WID", (char*)w);

  iupAttribSet(ih, "XWINDOW", (char*)XtWindow(w));
  iupAttribSet(ih, "XDISPLAY", (char*)iupmot_display);
  motFileDlgUpdatePreviewGLCanvas(ih);

  attrs.bit_gravity = ForgetGravity; /* For the DrawingArea widget gets Expose events when you resize it to be smaller. */
  attrs.background_pixmap = None;
  XChangeWindowAttributes(iupmot_display, XtWindow(w), CWBitGravity|CWBackPixmap, &attrs);
}

static void motFileDlgPreviewCanvasExposeCallback(Widget w, Ihandle *ih, XtPointer call_data)
{
  Widget filebox = (Widget)iupAttribGet(ih, "_IUPDLG_FILEBOX");
  char* filename;
  XmString xm_file;
  IFnss cb;

  if (!iupAttribGet(ih, "PREVIEWDC"))
    motFileDlgPreviewCanvasInit(ih, w);

  XtVaGetValues(filebox, XmNdirSpec, &xm_file, NULL);
  XmStringGetLtoR(xm_file, XmSTRING_DEFAULT_CHARSET, &filename);

  /* callback here always exists */
  cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (iupUnixIsFile(filename))
    cb(ih, filename, "PAINT");
  else
    cb(ih, NULL, "PAINT");

  XtFree(filename);
  (void)call_data;
  (void)w;
}

static void motFileDlgBrowseSelectionCallback(Widget w, Ihandle* ih, XmListCallbackStruct* list_data)
{
  char* filename;
  IFnss cb;

  XmStringGetLtoR(list_data->item, XmSTRING_DEFAULT_CHARSET, &filename);

  /* callback here always exists */
  cb = (IFnss)IupGetCallback(ih, "FILE_CB");
  if (iupUnixIsFile(filename))
    cb(ih, filename, "SELECT");
  else
    cb(ih, filename, "OTHER");

  XtFree(filename);
  (void)w;
}

static int motFileDlgPopup(Ihandle* ih, int x, int y)
{
  InativeHandle* parent = iupDialogGetNativeParent(ih);
  Widget filebox, dialog;
  int dialogtype, style = XmDIALOG_FULL_APPLICATION_MODAL;
  IFnss file_cb = NULL;
  Widget preview_canvas = NULL;
  char* value;

  iupAttribSetInt(ih, "_IUPDLG_X", x);   /* used in iupDialogUpdatePosition */
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  value = iupAttribGetStr(ih, "DIALOGTYPE");
  if (iupStrEqualNoCase(value, "SAVE"))
    dialogtype = IUP_DIALOGSAVE;
  else if (iupStrEqualNoCase(value, "DIR"))
    dialogtype = IUP_DIALOGDIR;
  else
    dialogtype = IUP_DIALOGOPEN;
  iupAttribSetInt(ih, "_IUPDLG_DIALOGTYPE", dialogtype);

  if (parent)
  {
    filebox = XmCreateFileSelectionDialog(parent, "filedialog", NULL, 0);
    dialog = XtParent(filebox);
  }
  else
  {
    dialog = XtAppCreateShell(NULL, "filedialog", topLevelShellWidgetClass, iupmot_display, NULL, 0);
    filebox = XmCreateFileSelectionBox(dialog, "filebox", NULL, 0);
    style = XmDIALOG_MODELESS;
    XtVaSetValues(dialog,
      XmNmwmInputMode, MWM_INPUT_FULL_APPLICATION_MODAL,
      XmNmappedWhenManaged, False,
      XmNsaveUnder, True,
      NULL);
  }
  if (!filebox)
    return IUP_NOERROR;

  if (!iupAttribGetBoolean(ih, "SHOWHIDDEN"))
    XtVaSetValues(filebox, XmNfileFilterStyle, XmFILTER_HIDDEN_FILES, NULL);

  value = iupAttribGet(ih, "TITLE");
  if (!value)
  {
    if (dialogtype == IUP_DIALOGSAVE)
      value = "IUP_SAVEAS";
    else if (dialogtype == IUP_DIALOGOPEN)
      value = "IUP_OPEN";
    else
      value = "IUP_SELECTDIR";
    value = IupGetLanguageString(value);
    iupAttribSet(ih, "TITLE", value);
  }
  iupmotSetXmString(filebox, XmNdialogTitle, value);

  XtVaSetValues(filebox,
                XmNdialogStyle, style,
                XmNautoUnmanage, False,
                XmNresizePolicy, XmRESIZE_GROW,
                NULL);

  if (dialogtype == IUP_DIALOGDIR)
    XtVaSetValues(filebox, XmNfileTypeMask, XmFILE_DIRECTORY, NULL);

  /* just check for the path inside FILE */
  value = iupAttribGet(ih, "FILE");
  if (value && value[0] == '/')
  {
    char* dir = iupStrFileGetPath(value);
    iupAttribSetStr(ih, "DIRECTORY", dir);
    free(dir);
  }

  /* set XmNdirectory before XmNpattern and before XmNdirSpec */

  value = iupAttribGet(ih, "DIRECTORY");
  if (value)
    iupmotSetXmString(filebox, XmNdirectory, value);

  value = iupAttribGet(ih, "FILTER");
  if (value)
  {
    char *filter = value;
    char *p = strchr(value, ';');
    if (p) 
    {
      /* Use only the first filter */
      int size = (int)(p-value);
      filter = (char*)malloc(size+1);
      memcpy(filter, value, size);
      filter[size] = 0;
    }

    iupmotSetXmString(filebox, XmNpattern, filter);

    if (filter != value) 
      free(filter);
  }

  value = iupAttribGet(ih, "FILE");
  if (value)
  {
    char* file = value;

    if (value[0] != '/')  /* if does not contains a full path, then add the directory */
    {
      char* cur_dir = NULL;
      char* dir = iupAttribGet(ih, "DIRECTORY");
      if (!dir)
      {
        cur_dir = iupdrvGetCurrentDirectory();
        dir = cur_dir;
      }

      file = iupStrFileMakeFileName(dir, value);

      if (cur_dir)
        free(cur_dir);
    }

    /* clear value before setting. Do not know why we have to do this, 
       but if not cleared it will fail to set the XmNdirSpec value. */
    iupmotSetXmString(filebox, XmNdirSpec, "");
    iupmotSetXmString(filebox, XmNdirSpec, file);

    if (file != value)
      free(file);
  }

  if (!IupGetCallback(ih, "HELP_CB"))
    XtUnmanageChild(XmFileSelectionBoxGetChild(filebox, XmDIALOG_HELP_BUTTON));

  XtAddCallback(filebox, XmNokCallback, (XtCallbackProc)motFileDlgCallback, (XtPointer)ih);
  XtAddCallback(filebox, XmNcancelCallback, (XtCallbackProc)motFileDlgCallback, (XtPointer)ih);
  XtAddCallback(filebox, XmNhelpCallback, (XtCallbackProc)motFileDlgHelpCallback, (XtPointer)ih);

  if (dialogtype == IUP_DIALOGDIR)
  {
    Widget new_folder = XtVaCreateManagedWidget("new_folder", xmPushButtonWidgetClass, filebox, 
                                                XmNlabelType, XmSTRING, 
                                                NULL);
    iupmotSetXmString(new_folder, XmNlabelString, IupGetLanguageString("IUP_CREATEFOLDER"));
    XtAddCallback(new_folder, XmNactivateCallback, (XtCallbackProc)motFileDlgNewFolderCallback, (XtPointer)filebox);
  }
  else
  {
    file_cb = (IFnss)IupGetCallback(ih, "FILE_CB");
    if (file_cb)
    {
      Widget list = XmFileSelectionBoxGetChild(filebox, XmDIALOG_LIST);
      XtAddCallback(list, XmNbrowseSelectionCallback, (XtCallbackProc)motFileDlgBrowseSelectionCallback, (XtPointer)ih);
      list = XmFileSelectionBoxGetChild(filebox, XmDIALOG_DIR_LIST);
      XtAddCallback(list, XmNbrowseSelectionCallback, (XtCallbackProc)motFileDlgBrowseSelectionCallback, (XtPointer)ih);

      if (iupAttribGetBoolean(ih, "SHOWPREVIEW"))
      {
        Widget frame = XtVaCreateManagedWidget("preview_canvas", xmFrameWidgetClass, filebox, 
                                                        XmNshadowType, XmSHADOW_ETCHED_IN,
                                                        NULL);

        preview_canvas = XtVaCreateManagedWidget("preview_canvas", xmDrawingAreaWidgetClass, frame, 
                                                        XmNwidth, 180, 
                                                        XmNheight, 150,
                                                        XmNresizePolicy, XmRESIZE_GROW,
                                                        NULL);

        XtAddCallback(preview_canvas, XmNexposeCallback, (XtCallbackProc)motFileDlgPreviewCanvasExposeCallback, (XtPointer)ih);
        XtAddCallback(preview_canvas, XmNresizeCallback, (XtCallbackProc)motFileDlgPreviewCanvasResizeCallback,  (XtPointer)ih);
        XtAddCallback(preview_canvas, XmNinputCallback, (XtCallbackProc)motFileDlgPreviewCanvasInputCallback, (XtPointer)ih);
        XtAddEventHandler(preview_canvas, PointerMotionMask, False, (XtEventHandler)iupmotPointerMotionEvent, (XtPointer)ih);

        iupAttribSet(ih, "_IUPDLG_FILEBOX", (char*)filebox);
      }
    }

    if (iupAttribGetBoolean(ih, "MULTIPLEFILES"))
    {
      Widget wList = XmFileSelectionBoxGetChild(filebox, XmDIALOG_LIST);
      XtVaSetValues(wList, XmNselectionPolicy, XmEXTENDED_SELECT, NULL);

      if (file_cb)
        XtAddCallback(wList, XmNextendedSelectionCallback, (XtCallbackProc)motFileDlgBrowseSelectionCallback, (XtPointer)ih);
    }
  }

  XmAddWMProtocolCallback(dialog, iupmot_wm_deletewindow, motFileDlgCBclose, (XtPointer)ih);
  XtManageChild(filebox);

  XtRealizeWidget(dialog);
  ih->handle = dialog;
  iupDialogUpdatePosition(ih);
  ih->handle = NULL;  /* reset handle */

  if (file_cb)
  {
    if (preview_canvas)
      motFileDlgPreviewCanvasInit(ih, preview_canvas);

    file_cb(ih, NULL, "INIT");
  }

  if (ih->userwidth && ih->userheight)
  {
    XtVaSetValues(dialog,
      XmNwidth, (XtArgVal)(ih->userwidth),
      XmNheight, (XtArgVal)(ih->userheight),
      NULL);
  }

  if (style == XmDIALOG_MODELESS)
    XtPopup(dialog, XtGrabExclusive);

  /* while the user hasn't provided an answer, simulate main loop.
  ** The answer changes as soon as the user selects one of the
  ** buttons and the callback routine changes its value. */
  iupAttribSet(ih, "STATUS", NULL);
  while (iupAttribGet(ih, "STATUS") == NULL)
    XtAppProcessEvent(iupmot_appcontext, XtIMAll);

  if (file_cb)
  {
    if (preview_canvas)
      XFreeGC(iupmot_display, (GC)iupAttribGet(ih, "PREVIEWDC"));

    file_cb(ih, NULL, "FINISH");
  }

  if (!iupAttribGet(ih, "_IUP_WM_DELETE"))
  {
    XtUnmanageChild(filebox);

    if (style == XmDIALOG_MODELESS)
    {
      XtPopdown(dialog);
      XtDestroyWidget(dialog);
    }
  }

  return IUP_NOERROR;
}

void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = motFileDlgPopup;
}
