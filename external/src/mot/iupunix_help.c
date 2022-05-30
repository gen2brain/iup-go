/** \file
 * \brief Motif Driver IupHelp
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_str.h"

IUP_API int IupExecute(const char *filename, const char* parameters)
{
  int ret;
  if (parameters)
  {
    char* cmd = (char*)malloc(sizeof(char)*(strlen(filename) + strlen(parameters) + 3));
    sprintf(cmd, "%s %s &", filename, parameters);
    ret = system(cmd);
    free(cmd);
  }
  else
  {
    char* cmd = (char*)malloc(sizeof(char)*(strlen(filename) + 3));
    sprintf(cmd, "%s &", filename);
    ret = system(cmd);
    free(cmd);
  }
  if (ret == -1)
    return -1;
  return 1;
}

IUP_API int IupExecuteWait(const char *filename, const char* parameters)
{
  int ret;
  if (parameters)
  {
    char* cmd = (char*)malloc(sizeof(char)*(strlen(filename) + strlen(parameters) + 3));
    sprintf(cmd, "%s %s", filename, parameters);
    ret = system(cmd);
    free(cmd);
  }
  else
    ret = system(filename);
  if (ret == -1)
    return -1;
  return 1;
}

IUP_API int IupHelp(const char *url)
{
  char *browser = getenv("IUP_HELPAPP");
  if (!browser) 
    browser = IupGetGlobal("HELPAPP");
    
  if (!browser) 
  { 
    char* system = IupGetGlobal("SYSTEM"); 
    if (iupStrEqualNoCase(system, "Linux") ||
        iupStrEqualNoCase(system, "FreeBSD"))
      browser = "firefox";
    else if (iupStrEqualNoCase(system, "MacOS"))
      browser = "safari";
    else if (iupStrEqualPartial(system, "CYGWIN"))
      browser = "iexplore";
    else  
      browser = "netscape";
  }
  
  return IupExecute(browser, url);
}
