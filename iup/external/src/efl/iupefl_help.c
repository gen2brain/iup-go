/** \file
 * \brief EFL Driver IupHelp for non Windows systems
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

#include "iupefl_drv.h"

#include "iupunix_portal.h"

extern char **environ;

static int iupEflBuildArgv(const char *filename, const char *parameters, char*** out_argv, char** out_params_copy)
{
  int argc = 0, i;
  char** argv;

  *out_params_copy = NULL;

  if (parameters && parameters[0])
  {
    char* params_copy = iupStrDup(parameters);
    char* token;
    char* rest = params_copy;

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

    *out_argv = argv;
    *out_params_copy = params_copy;
    return argc + 1;
  }
  else
  {
    argv = (char**)malloc(sizeof(char*) * 2);
    argv[0] = (char*)filename;
    argv[1] = NULL;

    *out_argv = argv;
    return 1;
  }
}

IUP_API int IupExecute(const char *filename, const char* parameters)
{
  int ret = 1;
  char** argv;
  char* params_copy;
  pid_t pid;
  int status;

  iupEflBuildArgv(filename, parameters, &argv, &params_copy);

  status = posix_spawnp(&pid, filename, NULL, NULL, argv, environ);

  free(argv);
  free(params_copy);

  if (status != 0)
    return -1;

  return ret;
}

IUP_API int IupExecuteWait(const char *filename, const char* parameters)
{
  int ret = 1;
  char** argv;
  char* params_copy;
  pid_t pid;
  int status;

  iupEflBuildArgv(filename, parameters, &argv, &params_copy);

  status = posix_spawnp(&pid, filename, NULL, NULL, argv, environ);

  free(argv);
  free(params_copy);

  if (status != 0)
    return -1;

  if (waitpid(pid, &status, 0) == -1)
    return -1;

  if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    ret = -1;

  return ret;
}

IUP_API int IupHelp(const char* url)
{
  char *browser;

  if (iupUnixPortalHelp(url) == 1)
    return 1;

  browser = getenv("IUP_HELPAPP");
  if (!browser)
    browser = IupGetGlobal("HELPAPP");

  if (!browser)
  {
    char* system_name = IupGetGlobal("SYSTEM");
    if (iupStrEqualNoCase(system_name, "macOS"))
      browser = "open";
    else
      browser = "xdg-open";
  }

  return IupExecute(browser, url);
}
