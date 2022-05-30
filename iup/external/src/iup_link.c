/** \file
 * \brief Link Button Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"


static int iLinkButton_CB(Ihandle* ih, int button, int pressed, int x, int y, char* status)
{
  if (button==IUP_BUTTON1 && pressed)
  {
    IFns cb = (IFns)IupGetCallback(ih, "ACTION");
    char* url = iupAttribGetStr(ih, "URL");
    if (cb)
    {
      int ret = cb(ih, url);
      if (ret == IUP_CLOSE) 
        IupExitLoop();
      else if (ret == IUP_DEFAULT && url)
        IupHelp(url);
    }
    else
      IupHelp(url);
  }

  (void)x;
  (void)y;
  (void)status;
  return IUP_DEFAULT;
}

static int iLinkEnterWindow_CB(Ihandle* ih)
{
  IupSetAttribute(ih, "CURSOR", "HAND");
  return IUP_DEFAULT;
}

static int iLinkLeaveWindow_CB(Ihandle* ih)
{
  IupSetAttribute(ih, "CURSOR", "ARROW");
  return IUP_DEFAULT;
}

static int iLinkMapMethod(Ihandle* ih)
{
  IupSetAttribute(ih, "FONTSTYLE", "Underline");
  return IUP_NOERROR;
}

static int iLinkCreateMethod(Ihandle* ih, void **params)
{
  if (params)
  {
    if (params[0]) iupAttribSetStr(ih, "URL", (char*)(params[0]));
    if (params[1]) iupAttribSetStr(ih, "TITLE", (char*)(params[1]));
  }

  IupSetCallback(ih, "BUTTON_CB", (Icallback)iLinkButton_CB);
  IupSetCallback(ih, "ENTERWINDOW_CB", iLinkEnterWindow_CB);
  IupSetCallback(ih, "LEAVEWINDOW_CB", iLinkLeaveWindow_CB);

  return IUP_NOERROR; 
}

Iclass* iupLinkNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("label"));

  ic->name = "link";
  ic->format = "ss"; /* two strings */
  ic->format_attr = "URL";  /* must handle TITLE manually */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupLinkNewClass;
  ic->Create = iLinkCreateMethod;
  ic->Map = iLinkMapMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "ACTION", "s");

  /* attributes */
  iupClassRegisterAttribute(ic, "URL", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CURSOR", NULL, iupdrvBaseSetCursorAttrib, IUPAF_SAMEASSYSTEM, "ARROW", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribDef(ic, "FGCOLOR", "LINKFGCOLOR", NULL);

  return ic;
}

IUP_API Ihandle* IupLink(const char *url, const char * title)
{
  void *params[3];
  params[0] = (void*)url;
  params[1] = (void*)title;
  params[2] = NULL;
  return IupCreatev("link", params);
}
