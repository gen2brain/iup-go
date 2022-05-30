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

int IupExecute(const char *filename, const char* parameters)
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

int IupExecuteWait(const char *filename, const char* parameters)
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

int IupHelp(const char *url)
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
