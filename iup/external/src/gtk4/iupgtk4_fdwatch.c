/** \file
 * \brief File Descriptor Watch for the GTK4 Driver.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <glib-unix.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_fdwatch.h"


static gboolean gtk4FdWatchProc(gint fd, GIOCondition condition, gpointer data)
{
  Ihandle* ih = (Ihandle*)data;

  (void)fd;
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

  if (ih->serial > 0)
    return 1;

  if (mode & IUP_FDWATCH_READ)
    condition |= G_IO_IN | G_IO_HUP | G_IO_ERR;
  if (mode & IUP_FDWATCH_WRITE)
    condition |= G_IO_OUT;

  ih->serial = g_unix_fd_add(iupFdWatchGetFd(ih), condition, gtk4FdWatchProc, (gpointer)ih);
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
