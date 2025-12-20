/** \file
 * \brief Notify Control - Desktop Notifications
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_notify.h"
#include "iup_attrib.h"

static int iNotifySetShowAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    return iupdrvNotifyShow(ih);
  return 0;
}

static int iNotifySetCloseAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    return iupdrvNotifyClose(ih);
  return 0;
}

static void iNotifyDestroyMethod(Ihandle* ih)
{
  iupdrvNotifyDestroy(ih);
}

/******************************************************************************/

IUP_API Ihandle* IupNotify(void)
{
  return IupCreate("notify");
}

Iclass* iupNotifyNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "notify";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupNotifyNewClass;
  ic->Destroy = iNotifyDestroyMethod;

  /* Callbacks */
  iupClassRegisterCallback(ic, "NOTIFY_CB", "i");
  iupClassRegisterCallback(ic, "CLOSE_CB", "i");
  iupClassRegisterCallback(ic, "ERROR_CB", "s");

  /* Common attributes - content */
  iupClassRegisterAttribute(ic, "TITLE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BODY", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ICON", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Common attributes - behavior */
  iupClassRegisterAttribute(ic, "TIMEOUT", NULL, NULL, IUPAF_SAMEASSYSTEM, "-1", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SILENT", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Action buttons (up to 4) */
  iupClassRegisterAttribute(ic, "ACTION1", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTION2", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTION3", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTION4", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Control attributes */
  iupClassRegisterAttribute(ic, "SHOW", NULL, iNotifySetShowAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLOSE", NULL, iNotifySetCloseAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ID", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  /* Platform-specific attributes registered by driver */
  iupdrvNotifyInitClass(ic);

  return ic;
}
