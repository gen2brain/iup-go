/** \file
 * \brief Windows Driver WinMain
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <stdlib.h> /* declaration of __argc and __argv */

#include "iup.h"
              
              
extern int main(int, char **);

/* save this handle in DllMain only, use to load resources from the DLL. */
#ifndef IUP_STUB
HINSTANCE iupwin_dll_hinstance = 0;
#endif

#ifdef IUP_DLL 
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  (void)fdwReason;
  (void)lpvReserved;

  iupwin_dll_hinstance = hinstDLL;

  return TRUE;
}
#else
/* this module is always linked in the makefile, 
   But it must not define WinMain if building the DLL */
int WINAPI WinMain (HINSTANCE hinst, HINSTANCE hprev, LPSTR cmdline, int ncmdshow)
{
  (void)hinst;  /* NOT used */
  (void)hprev;
  (void)cmdline;
  (void)ncmdshow;

  /* WinMain is NOT called for Console applications */
  
  return main(__argc, __argv);
}
#endif
