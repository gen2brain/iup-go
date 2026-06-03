/** \file
 * \brief WebAssembly IupNotify
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdint.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_notify.h"


EM_JS(int, iupwasmJsNotifyAvailable, (void), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'notifyavailable' });
  return (typeof Notification !== "undefined") ? 1 : 0;
})

EM_JS(int, iupwasmJsNotifyPermission, (void), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'notifypermission' });
  if (typeof Notification === "undefined") return 2;
  if (Notification.permission === "granted") return 1;
  if (Notification.permission === "denied") return 2;
  return 0;
})

EM_JS(void, iupwasmJsNotifyShow, (int ihptr, int id, const char* title, const char* body, int silent), {
  globalThis.__iupApply({ op: 'notifyshow', ihptr: ihptr, id: id, title: UTF8ToString(title), body: UTF8ToString(body), silent: silent });
})

EM_JS(void, iupwasmJsNotifyClose, (int id), {
  globalThis.__iupApply({ op: 'notifyclose', id: id });
})


EMSCRIPTEN_KEEPALIVE void iupwasmNotifyEvent(int ihptr, int id, int type)
{
  Ihandle* ih = (Ihandle*)(intptr_t)ihptr;
  if (!iupObjectCheck(ih))
    return;

  if (type == 0)
  {
    IFni cb = (IFni)IupGetCallback(ih, "NOTIFY_CB");
    if (cb && cb(ih, id) == IUP_CLOSE)
      IupExitLoop();
  }
  else if (type == 1)
  {
    IFni cb = (IFni)IupGetCallback(ih, "CLOSE_CB");
    if (cb && cb(ih, id) == IUP_CLOSE)
      IupExitLoop();
  }
  else
  {
    IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (cb)
      cb(ih, (char*)"notification error");
  }
}

IUP_SDK_API int iupdrvNotifyShow(Ihandle* ih)
{
  int perm = iupwasmJsNotifyPermission();
  int id = iupAttribGetInt(ih, "_IUPWASM_NOTIFY_ID");
  const char* title = IupGetAttribute(ih, "TITLE");
  const char* body = IupGetAttribute(ih, "BODY");

  if (perm == 2)
  {
    IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
    if (cb)
      cb(ih, (char*)"notifications not permitted");
    return 0;
  }

  if (!id)
  {
    static int s_next = 1;
    id = s_next++;
    iupAttribSetInt(ih, "_IUPWASM_NOTIFY_ID", id);
    iupAttribSetInt(ih, "ID", id);
  }

  iupwasmJsNotifyShow((int)(intptr_t)ih, id, title ? title : "", body ? body : "", iupAttribGetBoolean(ih, "SILENT"));
  return 1;
}

IUP_SDK_API int iupdrvNotifyClose(Ihandle* ih)
{
  int id = iupAttribGetInt(ih, "_IUPWASM_NOTIFY_ID");
  if (id)
    iupwasmJsNotifyClose(id);
  return 1;
}

IUP_SDK_API void iupdrvNotifyDestroy(Ihandle* ih)
{
  iupdrvNotifyClose(ih);
}

IUP_SDK_API int iupdrvNotifyIsAvailable(void)
{
  return iupwasmJsNotifyAvailable();
}

IUP_SDK_API void iupdrvNotifyInitClass(Iclass* ic)
{
  (void)ic;
}
