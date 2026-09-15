/** \file
 * \brief Haiku IupHelp / IupExecute
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" {
#include "iup.h"
#include "iup_str.h"
}

extern char** environ;

static char* haikuFindExecutable(const char* filename)
{
  if (strchr(filename, '/'))
    return iupStrDup(filename);

  const char* path = getenv("PATH");
  if (!path)
    path = "/bin";

  size_t name_len = strlen(filename);
  while (true)
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

    struct stat st;
    if (access(full, X_OK) == 0 && stat(full, &st) == 0 && S_ISREG(st.st_mode))
      return full;

    free(full);
    if (!end)
      return NULL;
    path = end + 1;
  }
}

static int haikuSpawnDetached(char** argv)
{
  char* path = haikuFindExecutable(argv[0]);
  if (!path)
    return -2;

  int fds[2];
  if (pipe(fds) != 0)
  {
    free(path);
    return -1;
  }
  fcntl(fds[0], F_SETFD, FD_CLOEXEC);
  fcntl(fds[1], F_SETFD, FD_CLOEXEC);

  int err = 0;
  ssize_t n;
  pid_t pid = fork();
  if (pid == 0)
  {
    close(fds[0]);
    pid_t child = fork();
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

static int haikuSpawnWait(char** argv)
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
  return 1;
}

static int haikuSpawn(const char* filename, const char* parameters, int wait)
{
  if (!filename || !*filename)
    return -1;

  char** argv = iupStrSplitCommandLine(filename, parameters);
  if (!argv)
    return -1;

  int ret = wait ? haikuSpawnWait(argv) : haikuSpawnDetached(argv);
  free(argv);
  return ret;
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
  /* /bin/open dispatches by MIME type. */
  const char* browser = getenv("IUP_HELPAPP");
  if (!browser)
    browser = IupGetGlobal("HELPAPP");
  if (!browser)
    browser = "open";

  char* argv[3];
  argv[0] = (char*)browser;
  argv[1] = url ? (char*)url : NULL;
  argv[2] = NULL;
  return haikuSpawnDetached(argv);
}
