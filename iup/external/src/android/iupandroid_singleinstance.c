/** \file
 * \brief Single Instance Control
 *
 * Android enforces single-instance via Activity launchMode in the manifest
 * (singleTask / singleInstance route new launches to onNewIntent instead of
 * spawning a second process). From IUP's view every launch is "the first".
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup_singleinstance.h"


int iupdrvSingleInstanceSet(const char* name)
{
  (void)name;
  return 0;
}

void iupdrvSingleInstanceClose(void)
{
}
