/** \file
 * \brief Cocoa Driver IupHelp
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#include "iup.h"


IUP_API int IupExecute(const char *filename, const char* parameters)
{
  char* cmd;
  int ret;

  if (parameters && *parameters)
  {
    /* Allocate for filename, parameters, and " & \0" */
    size_t cmd_size = strlen(filename) + strlen(parameters) + 4;
    cmd = (char*)malloc(cmd_size);
    snprintf(cmd, cmd_size, "%s %s &", filename, parameters);
  }
  else
  {
    /* Allocate for filename and " &\0" */
    size_t cmd_size = strlen(filename) + 3;
    cmd = (char*)malloc(cmd_size);
    snprintf(cmd, cmd_size, "%s &", filename);
  }

  ret = system(cmd);
  free(cmd);

  if (ret == -1)
    return -1;

  /* The shell typically returns 127 if the command is not found. */
  if (WIFEXITED(ret) && WEXITSTATUS(ret) == 127)
    return -2;

  return 1;
}

IUP_API int IupExecuteWait(const char *filename, const char* parameters)
{
  char* cmd;
  int ret;

  if (parameters && *parameters)
  {
    /* Allocate for filename, parameters, and " \0" */
    size_t cmd_size = strlen(filename) + strlen(parameters) + 2;
    cmd = (char*)malloc(cmd_size);
    snprintf(cmd, cmd_size, "%s %s", filename, parameters);
  }
  else
  {
    ret = system(filename);
    if (ret == -1)
      return -1;
    if (WIFEXITED(ret) && WEXITSTATUS(ret) == 127)
      return -2;
    return 1;
  }

  ret = system(cmd);
  free(cmd);

  if (ret == -1)
    return -1;

  if (WIFEXITED(ret) && WEXITSTATUS(ret) == 127)
    return -2;

  return 1;
}

IUP_API int IupHelp(const char* url)
{
  /* On macOS, the "open" command is the standard way to open a file or URL in the default application. */
  return IupExecute("open", url);
}