/** \file
* \brief Web Browser Control
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"
#include "iupweb.h"

#include "iup_object.h"
#include "iup_register.h"
#include "iup_webbrowser.h"

#ifdef IUPWEB_USE_DLOPEN
extern int IupGtkWebBrowserDLOpen(void);
#endif

IUPWEB_API int IupWebBrowserOpen(void)
{
  if (!IupIsOpened())
    return IUP_ERROR;

#ifdef IUPWEB_USE_DLOPEN
  if (IupGtkWebBrowserDLOpen() == IUP_ERROR)
    return IUP_ERROR;
#endif

  if (IupGetGlobal("_IUP_WEBBROWSER_OPEN"))
    return IUP_OPENED;

  iupRegisterClass(iupWebBrowserNewClass());

  IupSetGlobal("_IUP_WEBBROWSER_OPEN", "1");
  return IUP_NOERROR;
}

IUPWEB_API Ihandle *IupWebBrowser(void)
{
  return IupCreate("webbrowser");
}
