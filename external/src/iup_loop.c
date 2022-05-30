/** \file
* \brief Main Loop Utility Functions
*
* See Copyright Notice in "iup.h"
*/
#include "iup.h"
#include "iupcbs.h"

void iupLoopCallEntryCb(void)
{
  IFentry entry_callback = (IFentry)IupGetFunction("ENTRY_POINT");
  if (entry_callback)
  {
    entry_callback();
  }
}

void iupLoopCallExitCb(void)
{
  IFentry exit_callback = (IFentry)IupGetFunction("EXIT_CB");
  if (exit_callback)
  {
    exit_callback();
  }
}

