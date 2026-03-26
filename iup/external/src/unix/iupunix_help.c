/** \file
 * \brief Motif Driver IupHelp
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#include <errno.h>

#include "iup.h"

#include "iup_str.h"

#include "iupunix_portal.h"

extern char **environ;

static int iupUnixSpawn(const char *filename, const char* parameters, int wait)
{
  char** argv = NULL;
  int argc = 0;
  pid_t pid;
  int status, i;

  if (parameters && parameters[0])
  {
    char* params_copy = iupStrDup(parameters);
    char* token;
    char* rest = params_copy;

    /* count tokens */
    {
      char* tmp = iupStrDup(parameters);
      char* tmp_rest = tmp;
      while (strtok_r(tmp_rest, " \t", &tmp_rest))
        argc++;
      free(tmp);
    }

    argv = (char**)malloc(sizeof(char*) * (argc + 2));
    argv[0] = (char*)filename;
    i = 1;
    while ((token = strtok_r(rest, " \t", &rest)) != NULL)
      argv[i++] = token;
    argv[i] = NULL;

    status = posix_spawnp(&pid, filename, NULL, NULL, argv, environ);

    free(argv);
    free(params_copy);
  }
  else
  {
    argv = (char**)malloc(sizeof(char*) * 2);
    argv[0] = (char*)filename;
    argv[1] = NULL;

    status = posix_spawnp(&pid, filename, NULL, NULL, argv, environ);

    free(argv);
  }

  if (status != 0)
  {
    if (status == ENOENT)
      return -2;
    return -1;
  }

  if (wait)
  {
    if (waitpid(pid, &status, 0) == -1)
      return -1;
  }

  return 1;
}

IUP_API int IupExecute(const char *filename, const char* parameters)
{
  return iupUnixSpawn(filename, parameters, 0);
}

IUP_API int IupExecuteWait(const char *filename, const char* parameters)
{
  return iupUnixSpawn(filename, parameters, 1);
}

IUP_API int IupHelp(const char *url)
{
  char *browser;

  if (iupUnixPortalHelp(url) == 1)
    return 1;

  browser = getenv("IUP_HELPAPP");
  if (!browser)
    browser = IupGetGlobal("HELPAPP");

  if (!browser)
  {
    char* system = IupGetGlobal("SYSTEM");
    if (iupStrEqualNoCase(system, "macOS"))
      browser = "open";
    else
      browser = "xdg-open";
  }

  return IupExecute(browser, url);
}
