/** \file
 * \brief Haiku IupHelp / IupExecute
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" {
#include "iup.h"
#include "iup_str.h"
}

extern char** environ;

static int haikuSpawn(const char* filename, const char* parameters, int wait)
{
  if (!filename || !*filename)
    return -1;

  pid_t pid;
  int status;
  char** argv = NULL;
  char* params_copy = NULL;

  if (parameters && parameters[0])
  {
    int argc = 0;
    {
      char* tmp = iupStrDup(parameters);
      char* rest = tmp;
      while (strtok_r(rest, " \t", &rest)) argc++;
      free(tmp);
    }
    argv = (char**)malloc(sizeof(char*) * (argc + 2));
    argv[0] = (char*)filename;
    params_copy = iupStrDup(parameters);
    int i = 1;
    char* rest = params_copy;
    char* tok;
    while ((tok = strtok_r(rest, " \t", &rest)) != NULL)
      argv[i++] = tok;
    argv[i] = NULL;
  }
  else
  {
    argv = (char**)malloc(sizeof(char*) * 2);
    argv[0] = (char*)filename;
    argv[1] = NULL;
  }

  status = posix_spawnp(&pid, filename, NULL, NULL, argv, environ);

  free(argv);
  free(params_copy);

  if (status != 0)
    return (status == ENOENT) ? -2 : -1;

  if (wait)
  {
    int wstatus;
    if (waitpid(pid, &wstatus, 0) == -1)
      return -1;
  }
  return 1;
}

extern "C" IUP_API int IupExecute(const char* filename, const char* parameters)
{
  return haikuSpawn(filename, parameters, 0);
}

extern "C" IUP_API int IupExecuteWait(const char* filename, const char* parameters)
{
  return haikuSpawn(filename, parameters, 1);
}

extern "C" IUP_API int IupHelp(const char* url)
{
  /* Haiku's /bin/open dispatches by MIME type, same as macOS. */
  const char* browser = getenv("IUP_HELPAPP");
  if (!browser)
    browser = IupGetGlobal("HELPAPP");
  if (!browser)
    browser = "open";

  /* URL is one argv entry; IupExecute would tokenize on whitespace. */
  char* argv[3];
  argv[0] = (char*)browser;
  argv[1] = url ? (char*)url : NULL;
  argv[2] = NULL;

  pid_t pid;
  int status = posix_spawnp(&pid, browser, NULL, NULL, argv, environ);
  if (status != 0)
    return (status == ENOENT) ? -2 : -1;
  return 1;
}
