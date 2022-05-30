/** \file
 * \brief IupProgressDlg pre-defined dialog control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "iup.h"
#include "iupcbs.h"


#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_strmessage.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_childtree.h"


      
typedef struct _IprogressDlgData
{
  Ihandle *progress, 
          *description;  /* label for the secondary description */

  int state,          /* flag indicating if it was interrupted */
      percent,         /* current percent value */
      count, total_count,
      last_clock, last_percent,      /* last time it was updated */
      min_percent, min_clock;
} IprogressDlgData;


static void iProgressDlgSetPercent(IprogressDlgData* progress_data, int percent)
{
  int cur_clock;

  if (progress_data->state == 0)  /* from IDLE */
    progress_data->state = 1;  /* to PROCESSING */

  if (progress_data->state != 1)
    return;

  cur_clock = (int)clock();
  if (cur_clock - progress_data->last_clock > progress_data->min_clock ||  /* significant amount of time */
      progress_data->percent - progress_data->last_percent > progress_data->min_percent)  /* minimum percentage */
  {
    /* avoid duplicate updates */
    if (percent != progress_data->percent)
    {
      IupSetInt(progress_data->progress, "VALUE", percent);
      IupFlush();
      progress_data->last_clock = (int)clock();
      progress_data->last_percent = percent;
    }
  }

  progress_data->percent = percent;

  IupLoopStep();
}

/**************************************************************************************************************/
/*                                     Attributes                                                             */
/**************************************************************************************************************/

static int iProgressDlgSetTotalCountAttrib(Ihandle* ih, const char* value)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  iupStrToInt(value, &(progress_data->total_count));
  if (progress_data->total_count <= 0) progress_data->total_count = 1;
  return 0;
}

static char* iProgressDlgGetTotalCountAttrib(Ihandle *ih)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  return iupStrReturnInt(progress_data->total_count);
}

static int iProgressDlgSetCountAttrib(Ihandle* ih, const char* value)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  if (iupStrToInt(value, &(progress_data->count)) && (progress_data->count >= 0 && progress_data->count <= progress_data->total_count))
    iProgressDlgSetPercent(progress_data, (progress_data->count*100)/progress_data->total_count);
  return 0;
}

static char* iProgressDlgGetCountAttrib(Ihandle *ih)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  return iupStrReturnInt(progress_data->count);
}

static int iProgressDlgSetIncAttrib(Ihandle* ih, const char* value)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  int inc = 1;
  iupStrToInt(value, &inc);
  progress_data->count += inc;
  if (progress_data->count < 0) progress_data->count = 0;
  if (progress_data->count > progress_data->total_count) progress_data->count = progress_data->total_count;
  iProgressDlgSetPercent(progress_data, (progress_data->count*100)/progress_data->total_count);
  return 0;
}

static char* iProgressDlgGetStateAttrib(Ihandle *ih)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  if (progress_data->state==-1)
    return "ABORTED";
  else if (progress_data->state==1)
    return "PROCESSING";
  else if (progress_data->state==2)
    return "UNDEFINED";
  else
    return "IDLE";
}

static void iProgressDlgStopMarquee(IprogressDlgData* progress_data)
{
  Ihandle* zbox = IupGetParent(progress_data->progress);
  if (IupGetInt(zbox, "VALUEPOS")==1)
  {
    Ihandle* marquee = IupGetBrother(progress_data->progress);
    IupSetAttribute(marquee, "MARQUEE", "No");
  }
}

static int iProgressDlgSetStateAttrib(Ihandle *ih, const char* value)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");

  if (iupStrEqualNoCase(value, "ABORTED"))
  {
    iProgressDlgStopMarquee(progress_data);
    progress_data->state = -1;
  }
  else if (iupStrEqualNoCase(value, "PROCESSING"))
  {
    Ihandle* zbox = IupGetParent(progress_data->progress);
    IupSetAttribute(zbox, "VALUEPOS", "0");
    progress_data->state = 1;
  }
  else if (iupStrEqualNoCase(value, "UNDEFINED"))
  {
    Ihandle* zbox = IupGetParent(progress_data->progress);
    Ihandle* marquee = IupGetBrother(progress_data->progress);
    IupSetAttribute(zbox, "VALUEPOS", "1");
    IupSetAttribute(marquee, "MARQUEE", "Yes");
    progress_data->state = 2;
  }
  else  /* IDLE */
  {
    progress_data->state = 0;
    progress_data->percent = 0;
    progress_data->last_percent = 0;
    progress_data->count = 0;
    IupSetAttribute(progress_data->progress, "VALUE", "0");
    iProgressDlgStopMarquee(progress_data);
  }
  return 0;
}

static int iProgressDlgSetPercentAttrib(Ihandle* ih, const char* value)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  int percent;
  if (iupStrToInt(value, &percent) && (percent >=0 && percent <=100))
    iProgressDlgSetPercent(progress_data, percent);
  return 0;
}

static char* iProgressDlgGetPercentAttrib(Ihandle* ih)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  return iupStrReturnInt(progress_data->percent);
}

static int iProgressDlgSetMinPercentAttrib(Ihandle* ih, const char* value)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  int percent;
  if (iupStrToInt(value, &percent) && (percent >= 0 && percent <= 100))
    progress_data->min_percent = percent;
  return 0;
}

static char* iProgressDlgGetMinPercentAttrib(Ihandle* ih)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  return iupStrReturnInt(progress_data->min_percent);
}

static int iProgressDlgSetMinClockAttrib(Ihandle* ih, const char* value)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  int min_clock;
  if (iupStrToInt(value, &min_clock) && (min_clock >= 0 && min_clock <= 100))
    progress_data->min_clock = min_clock;
  return 0;
}

static char* iProgressDlgGetMinClockAttrib(Ihandle* ih)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  return iupStrReturnInt(progress_data->min_clock);
}

static int iProgressDlgSetProgressHeightAttrib(Ihandle* ih, const char* value)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  Ihandle* marquee = IupGetBrother(progress_data->progress);
  IupSetStrf(progress_data->progress, "RASTERSIZE", "250x%s", value);
  IupSetStrf(marquee, "RASTERSIZE", "250x%s", value);
  return 1;
}

static int iProgressDlgSetDescriptionAttrib(Ihandle* ih, const char* value)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  IupStoreAttribute(progress_data->description, "TITLE", (char*)value);
  IupFlush();
  return 0;
}

static char* iProgressDlgGetDescriptionAttrib(Ihandle* ih)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  return IupGetAttribute(progress_data->description, "TITLE");
}

/**************************************************************************************************************/
/*                                     Internal Callbacks                                                     */
/**************************************************************************************************************/

static int iProgressDlgCancel_CB(Ihandle* button)
{
  Ihandle* ih = IupGetDialog(button);
  Icallback cb = IupGetCallback(ih, "CANCEL_CB");
  int ret = IUP_CONTINUE;

  if (cb)
    ret = cb(ih);

  if (ret != IUP_CONTINUE)
    iProgressDlgSetStateAttrib(ih, "ABORTED");

  return ret;
}

/**************************************************************************************************************/
/*                                     Methods                                                                */
/**************************************************************************************************************/

static void iProgressDlgDestroyMethod(Ihandle* ih)
{
  IprogressDlgData* progress_data = (IprogressDlgData*)iupAttribGet(ih, "_IUP_PDLG_DATA");
  free(progress_data);
}

static int iProgressDlgCreateMethod(Ihandle* ih, void** params)
{
  Ihandle *lbl, *progress, *marquee, *cancel, *vbox;
  IprogressDlgData* progress_data = (IprogressDlgData*)malloc(sizeof(IprogressDlgData));
  memset(progress_data, 0, sizeof(IprogressDlgData));
  iupAttribSet(ih, "_IUP_PDLG_DATA", (char*)progress_data);

  lbl = IupLabel("");
  IupSetAttribute(lbl,"EXPAND","YES");

  progress = IupProgressBar();
  IupSetAttribute(progress,"RASTERSIZE","250x30");
  IupSetAttribute(progress,"EXPAND","HORIZONTAL");
  IupSetAttribute(progress,"MAX","100");

  marquee = IupProgressBar();
  IupSetAttribute(marquee,"RASTERSIZE","250x30");
  IupSetAttribute(marquee,"EXPAND","HORIZONTAL");
  IupSetAttribute(marquee,"MARQUEE","Yes");
  IupSetAttribute(marquee,"VISIBLE","No");

  cancel = IupButton("_@IUP_CANCEL", NULL);
  IupSetStrAttribute(cancel, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(cancel, "ACTION", (Icallback) iProgressDlgCancel_CB);
  IupSetAttributeHandle(ih, "DEFAULTESC", cancel);

  vbox = IupVbox(lbl, IupZbox(progress, marquee, NULL), cancel, NULL);
  IupSetAttribute(vbox,"MARGIN","5x5");
  IupSetAttribute(vbox,"GAP","5x5");
  IupSetAttribute(vbox,"ALIGNMENT","ACENTER");

  /* Do not use IupAppend because we set childtype=IUP_CHILDNONE */
  iupChildTreeAppend(ih, vbox);
  IupSetAttribute(ih,"RESIZE","NO");
  IupSetAttribute(ih,"MAXBOX","NO");
  IupSetAttribute(ih,"MINBOX","NO");
  IupSetAttribute(ih,"MENUBOX","NO");

  progress_data->progress = progress;
  progress_data->description = lbl;

  progress_data->total_count = 1;
  progress_data->last_clock = clock();
  progress_data->last_percent = 0;

  progress_data->min_clock = 250;
  progress_data->min_percent = 10;

  (void)params;
  return IUP_NOERROR;
}

Iclass* iupProgressDlgNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("dialog"));

  ic->name = "progressdlg";
  ic->cons = "ProgressDlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;
  ic->childtype = IUP_CHILDNONE;

  ic->New = iupProgressDlgNewClass;
  ic->Create = iProgressDlgCreateMethod;
  ic->Destroy = iProgressDlgDestroyMethod;

  iupClassRegisterCallback(ic, "CANCEL_CB", "");

  iupClassRegisterAttribute(ic, "TOTALCOUNT", iProgressDlgGetTotalCountAttrib, iProgressDlgSetTotalCountAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", iProgressDlgGetCountAttrib, iProgressDlgSetCountAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INC", NULL, iProgressDlgSetIncAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PERCENT", iProgressDlgGetPercentAttrib, iProgressDlgSetPercentAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PROGRESSHEIGHT", NULL, iProgressDlgSetProgressHeightAttrib, IUPAF_SAMEASSYSTEM, "30", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINCLOCK", iProgressDlgGetMinClockAttrib, iProgressDlgSetMinClockAttrib, IUPAF_SAMEASSYSTEM, "250", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINPERCENT", iProgressDlgGetMinPercentAttrib, iProgressDlgSetMinPercentAttrib, IUPAF_SAMEASSYSTEM, "10", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "STATE", iProgressDlgGetStateAttrib, iProgressDlgSetStateAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DESCRIPTION", iProgressDlgGetDescriptionAttrib, iProgressDlgSetDescriptionAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}

IUP_API Ihandle* IupProgressDlg(void)
{
  return IupCreate("progressdlg");
}
