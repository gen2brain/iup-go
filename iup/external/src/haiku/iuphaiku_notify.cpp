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
  auto* v = new BView(BRect(0, 0, 0, 0), "iup_notify_stub", B_FOLLOW_NONE, B_WILL_DRAW);
  ih->handle = reinterpret_cast<InativeHandle*>(v);
  return IUP_NOERROR;
}

static int haiku_notify_last_id = 0;

extern "C" IUP_SDK_API int iupdrvNotifyShow(Ihandle* ih)
{
  const char* progv = iupAttribGet(ih, "PROGRESS");
  int progress = -1;
  if (progv && !iupStrToInt(progv, &progress))
    progress = -1;

  notification_type type = B_INFORMATION_NOTIFICATION;
  if (progress >= 0)
    type = B_PROGRESS_NOTIFICATION;
  else if (iupStrEqualNoCase(iupAttribGet(ih, "URGENCY"), "HIGH"))
    type = B_IMPORTANT_NOTIFICATION;

  BNotification n(type);

  const char* title = iupAttribGet(ih, "TITLE");
  if (title) n.SetTitle(title);

  const char* body = iupAttribGet(ih, "BODY");
  if (body) n.SetContent(body);

  const char* image = iupAttribGet(ih, "IMAGE");
  BBitmap* bm = image ? static_cast<BBitmap*>(iupImageGetImage(image, ih, 0, nullptr)) : nullptr;
  if (!bm)
  {
    const char* icon = iupAttribGet(ih, "ICON");
    if (icon) bm = static_cast<BBitmap*>(iupImageGetIcon(icon));
  }
  if (bm) n.SetIcon(bm);

  const char* thread_id = iupAttribGet(ih, "THREADID");
  if (thread_id) n.SetGroup(thread_id);

  int id = iupAttribGetInt(ih, "_IUPHAIKU_NOTIFY_ID");
  if (id == 0)
  {
    id = ++haiku_notify_last_id;
    iupAttribSetInt(ih, "_IUPHAIKU_NOTIFY_ID", id);
  }
  BString message_id;
  message_id.SetToFormat("iup-notify-%d", id);
  n.SetMessageID(message_id.String());

  if (progress >= 0)
    n.SetProgress(static_cast<float>(progress > 100 ? 100 : progress) / 100.0f);

  int timeout_ms = iupAttribGetInt(ih, "TIMEOUT");
  bigtime_t timeout_us = (timeout_ms > 0) ? static_cast<bigtime_t>(timeout_ms) * 1000 : -1;
  if (n.Send(timeout_us) != B_OK)
    return 0;

  iupAttribSetInt(ih, "ID", id);
  return 1;
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
