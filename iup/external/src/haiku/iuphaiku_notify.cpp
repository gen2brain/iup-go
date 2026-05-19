/** \file
 * \brief Haiku Notify (BNotification)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>

#include <Bitmap.h>
#include <Notification.h>
#include <Rect.h>
#include <String.h>
#include <View.h>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_notify.h"
}


static int haikuNotifyStubMap(Ihandle* ih)
{
  BView* v = new BView(BRect(0, 0, 0, 0), "iup_notify_stub", B_FOLLOW_NONE, B_WILL_DRAW);
  ih->handle = (InativeHandle*)v;
  return IUP_NOERROR;
}

static notification_type haikuNotifyType(const char* style)
{
  if (!style) return B_INFORMATION_NOTIFICATION;
  if (iupStrEqualNoCase(style, "ERROR"))    return B_ERROR_NOTIFICATION;
  if (iupStrEqualNoCase(style, "WARNING"))  return B_IMPORTANT_NOTIFICATION;
  if (iupStrEqualNoCase(style, "INFO"))     return B_INFORMATION_NOTIFICATION;
  if (iupStrEqualNoCase(style, "PROGRESS")) return B_PROGRESS_NOTIFICATION;
  return B_INFORMATION_NOTIFICATION;
}

extern "C" IUP_SDK_API int iupdrvNotifyShow(Ihandle* ih)
{
  const char* style = iupAttribGet(ih, "STYLE");
  BNotification n(haikuNotifyType(style));

  const char* title = iupAttribGet(ih, "TITLE");
  if (title) n.SetTitle(title);

  /* IUP standardized on BODY for notification text; older code may still set
   * MESSAGE - fall back to that. */
  const char* body = iupAttribGet(ih, "BODY");
  if (!body) body = iupAttribGet(ih, "MESSAGE");
  if (body) n.SetContent(body);

  /* IMAGE first (user-supplied IUP image handle), fall back to ICON
   * (stock-name lookup via the registered image table). */
  const char* image = iupAttribGet(ih, "IMAGE");
  BBitmap* bm = image ? (BBitmap*)iupImageGetImage(image, ih, 0, NULL) : NULL;
  if (!bm)
  {
    const char* icon = iupAttribGet(ih, "ICON");
    if (icon) bm = (BBitmap*)iupImageGetIcon(icon);
  }
  if (bm) n.SetIcon(bm);

  /* GROUP / ID let Haiku coalesce or replace prior notifications. */
  const char* group = iupAttribGet(ih, "GROUP");
  if (group) n.SetGroup(group);

  const char* msg_id = iupAttribGet(ih, "MESSAGEID");
  if (!msg_id) msg_id = iupAttribGet(ih, "ID");
  if (msg_id) n.SetMessageID(msg_id);

  /* PROGRESS only meaningful with STYLE=PROGRESS - 0.0..1.0. */
  if (style && iupStrEqualNoCase(style, "PROGRESS"))
  {
    double prog = 0.0;
    char* progv = iupAttribGet(ih, "PROGRESS");
    if (progv && iupStrToDouble(progv, &prog)) n.SetProgress((float)prog);
  }

  int timeout_ms = iupAttribGetInt(ih, "TIMEOUT");
  bigtime_t timeout_us = (timeout_ms > 0) ? (bigtime_t)timeout_ms * 1000 : -1;
  return (n.Send(timeout_us) == B_OK) ? 1 : 0;
}

extern "C" IUP_SDK_API int iupdrvNotifyClose(Ihandle* /*ih*/)
{
  /* BNotification dismisses itself at timeout / user click; no explicit close. */
  return 0;
}

extern "C" IUP_SDK_API void iupdrvNotifyDestroy(Ihandle* /*ih*/) {}

extern "C" IUP_SDK_API int iupdrvNotifyIsAvailable(void) { return 1; }

extern "C" IUP_SDK_API void iupdrvNotifyInitClass(Iclass* ic)
{
  ic->Map = haikuNotifyStubMap;
}
