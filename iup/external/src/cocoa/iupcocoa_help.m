/** \file
 * \brief Cocoa Driver IupHelp
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <spawn.h>
#include <errno.h>

#include "iup.h"

#include "iup_str.h"

extern char** environ;

static char* iupCocoaFindExecutable(const char* filename)
{
  const char* path;
  size_t name_len;
  struct stat st;

  if (strchr(filename, '/'))
    return iupStrDup(filename);

  path = getenv("PATH");
  if (!path)
    path = "/bin:/usr/bin";

  name_len = strlen(filename);
  while (1)
  {
    const char* end = strchr(path, ':');
    size_t dir_len = end ? (size_t)(end - path) : strlen(path);
    char* full = (char*)malloc(dir_len + name_len + 3);

    if (dir_len == 0)
      strcpy(full, "./");
    else
    {
      memcpy(full, path, dir_len);
      full[dir_len] = '/';
      full[dir_len + 1] = 0;
    }
    strcat(full, filename);

    if (access(full, X_OK) == 0 && stat(full, &st) == 0 && S_ISREG(st.st_mode))
      return full;

    free(full);
    if (!end)
      return NULL;
    path = end + 1;
  }
}

static int iupCocoaSpawnDetached(char** argv)
{
  char* path;
  int fds[2], err = 0;
  ssize_t n;
  pid_t pid;

  path = iupCocoaFindExecutable(argv[0]);
  if (!path)
    return -2;

  if (pipe(fds) != 0)
  {
    free(path);
    return -1;
  }
  fcntl(fds[0], F_SETFD, FD_CLOEXEC);
  fcntl(fds[1], F_SETFD, FD_CLOEXEC);

  pid = fork();
  if (pid == 0)
  {
    pid_t child;

    close(fds[0]);
    child = fork();
    if (child == 0)
    {
      execve(path, argv, environ);
      err = errno;
      n = write(fds[1], &err, sizeof(err));
      _exit(127);
    }
    if (child == -1)
    {
      err = errno;
      n = write(fds[1], &err, sizeof(err));
    }
    _exit(0);
  }

  close(fds[1]);

  if (pid == -1)
  {
    close(fds[0]);
    free(path);
    return -1;
  }

  do
    n = read(fds[0], &err, sizeof(err));
  while (n == -1 && errno == EINTR);
  close(fds[0]);

  while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
    ;

  free(path);

  if (n == (ssize_t)sizeof(err))
    return (err == ENOENT) ? -2 : -1;
  if (n != 0)
    return -1;
  return 1;
}

static int iupCocoaSpawnWait(char** argv)
{
  pid_t pid;
  int status = posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ);

  if (status != 0)
    return (status == ENOENT) ? -2 : -1;

  while (waitpid(pid, &status, 0) == -1)
  {
    if (errno != EINTR)
      return -1;
  }

  if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
    return -2;

  return 1;
}

static int iupCocoaSpawn(const char* filename, const char* parameters, int wait)
{
  char** argv;
  int ret;

  if (!filename || !filename[0])
    return -1;

  argv = iupStrSplitCommandLine(filename, parameters);
  if (!argv)
    return -1;

  ret = wait ? iupCocoaSpawnWait(argv) : iupCocoaSpawnDetached(argv);
  free(argv);
  return ret;
}

IUP_API int IupExecute(const char* filename, const char* parameters)
{
  return iupCocoaSpawn(filename, parameters, 0);
}

IUP_API int IupExecuteWait(const char* filename, const char* parameters)
{
  return iupCocoaSpawn(filename, parameters, 1);
}

IUP_API int IupHelp(const char* url)
{
  char* argv[3];

  argv[0] = "open";
  argv[1] = (char*)url;
  argv[2] = NULL;
  return iupCocoaSpawnDetached(argv);
}
