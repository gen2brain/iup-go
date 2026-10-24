/** \file
 * \brief File Descriptor Watch for the GTK4 Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_fdwatch.h"


static gboolean gtk4FdWatchProc(GIOChannel* source, GIOCondition condition, gpointer data)
{
  Ihandle* ih = (Ihandle*)data;

  (void)source;
  (void)condition;

  if (!iupObjectCheck(ih))
    return FALSE;

  iupFdWatchCallAction(ih);
  return TRUE;
}

IUP_SDK_API int iupdrvFdWatchStart(Ihandle* ih)
{
  int mode = iupFdWatchGetMode(ih);
  GIOCondition condition = 0;
  GIOChannel* channel;

  if (ih->serial > 0)
    return 1;

  if (mode & IUP_FDWATCH_READ)
    condition |= G_IO_IN | G_IO_HUP | G_IO_ERR;
  if (mode & IUP_FDWATCH_WRITE)
    condition |= G_IO_OUT;

#ifdef G_OS_WIN32
  channel = g_io_channel_win32_new_socket(iupFdWatchGetFd(ih));
#else
  channel = g_io_channel_unix_new(iupFdWatchGetFd(ih));
#endif
  if (!channel)
    return 0;

  ih->serial = g_io_add_watch(channel, condition, gtk4FdWatchProc, (gpointer)ih);
  g_io_channel_unref(channel);
  return ih->serial > 0;
}

IUP_SDK_API void iupdrvFdWatchStop(Ihandle* ih)
{
  if (ih->serial > 0)
  {
    g_source_remove(ih->serial);
    ih->serial = -1;
  }
}

IUP_SDK_API void iupdrvFdWatchInitClass(Iclass* ic)
{
  (void)ic;
}
