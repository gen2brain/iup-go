/** \file
 * \brief MAC Driver IupHelp
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_str.h"

IUP_API int IupHelp(const char *url)
{
  char *cmd;
  int ret;
  char *browser = getenv("IUP_HELPAPP");
  if (!browser) 
    browser = IupGetGlobal("HELPAPP");
    
  if (!browser) 
  { 
    char* system = IupGetGlobal("SYSTEM"); 
    if (iupStrEqualNoCase(system, "Snow Leopard") ||
        iupStrEqualNoCase(system, "Leopard") ||
        iupStrEqualNoCase(system, "Tiger") ||
        iupStrEqualNoCase(system, "Panther"))
      browser = "safari";
    else if (iupStrEqualNoCase(system, "Jaguar") ||
        iupStrEqualNoCase(system, "Puma") ||
        iupStrEqualNoCase(system, "Cheetah"))
      browser = "iexplore";	  
    else  /* MacOS */
      browser = "netscape";
  }
  
  cmd = (char*)malloc(sizeof(char)*(strlen(url)+strlen(browser)+3));
  sprintf(cmd, "open -a %s %s &", browser, url);
  ret = system(cmd); 
  free(cmd);
  if (ret == -1)
    return -1;
  return 1;
}
