/** \file
 * \brief Terminal Control pseudo-terminal backend.
 *
 * See Copyright Notice in "iup.h"
 */

#if (defined(__linux__) || defined(__ANDROID__)) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_terminal.h"

#if defined(WIN32) || defined(_WIN32)
#define ITERM_PTY_WIN
#elif defined(__EMSCRIPTEN__)
/* no pty */
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if !TARGET_OS_IPHONE
#define ITERM_PTY_POSIX
#endif
#else
#define ITERM_PTY_POSIX
#endif

static void itermPtySetError(char* error, int error_size, const char* text)
{
  if (error && error_size > 0)
  {
    strncpy(error, text, error_size - 1);
    error[error_size - 1] = 0;
  }
}

static int itermPtySameHost(const char* host, const char* local)
{
  int n;
  if (!host[0] || iupStrEqualNoCase(host, "localhost"))
    return 1;
  if (!local)
    return 0;
  for (n = 0; host[n] && host[n] != '.' && local[n] && local[n] != '.'; n++)
  {
    if (iup_tolower(host[n]) != iup_tolower(local[n]))
      return 0;
  }
  return (!host[n] || host[n] == '.') && (!local[n] || local[n] == '.');
}

#ifdef ITERM_PTY_POSIX

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>

#define ITERM_PTY_MAXARGS 64

static char** itermPtySplitArgs(const char* cmd, char** copy_out)
{
  char** argv;
  char* copy;
  char* p;
  int argc = 0;

  copy = iupStrDup(cmd);
  if (!copy)
    return NULL;

  argv = (char**)malloc(sizeof(char*) * (ITERM_PTY_MAXARGS + 1));
  if (!argv)
  {
    free(copy);
    return NULL;
  }

  p = copy;
  while (*p && argc < ITERM_PTY_MAXARGS)
  {
    char* out;
    while (*p == ' ' || *p == '\t')
      p++;
    if (!*p)
      break;

    out = p;
    argv[argc++] = out;
    while (*p && *p != ' ' && *p != '\t')
    {
      if (*p == '"')
      {
        p++;
        while (*p && *p != '"')
          *out++ = *p++;
        if (*p)
          p++;
      }
      else
        *out++ = *p++;
    }
    if (*p)
      p++;
    *out = 0;
  }

  argv[argc] = NULL;
  if (argc == 0)
  {
    free(argv);
    free(copy);
    return NULL;
  }

  *copy_out = copy;
  return argv;
}

static void itermPtyFreeArgs(char** argv, char* copy)
{
  free(argv);
  free(copy);
}

struct _ItermPty
{
  int fd;
  pid_t pid;
  int exited;
  int status;
  char* shell_dir;
};

static const char* iterm_bash_rc =
  "[ -r \"$HOME/.bashrc\" ] && . \"$HOME/.bashrc\"\n"
  "__iupterm_prompt() {\n"
  "  local s=$?\n"
  "  printf '\\033]133;D;%s\\007\\033]7;file://%s%s\\007\\033]133;A\\007' \"$s\" \"$HOSTNAME\" \"${PWD//\\%/%25}\"\n"
  "  return $s\n"
  "}\n"
  "PROMPT_COMMAND=\"__iupterm_prompt${PROMPT_COMMAND:+;$PROMPT_COMMAND}\"\n"
  "PS0=\"\\[\\033]133;C\\007\\]$PS0\"\n";

static const char* iterm_zsh_env =
  "if [[ -n \"$IUPTERM_ZDOTDIR\" ]]; then ZDOTDIR=\"$IUPTERM_ZDOTDIR\"; else unset ZDOTDIR; fi\n"
  "unset IUPTERM_ZDOTDIR\n"
  "[[ -r \"${ZDOTDIR:-$HOME}/.zshenv\" ]] && source \"${ZDOTDIR:-$HOME}/.zshenv\"\n"
  "if [[ -o interactive ]]; then\n"
  "  __iupterm_precmd() {\n"
  "    local s=$?\n"
  "    printf '\\033]133;D;%s\\007\\033]7;file://%s%s\\007\\033]133;A\\007' \"$s\" \"$HOST\" \"${PWD//\\%/%25}\"\n"
  "  }\n"
  "  __iupterm_preexec() { printf '\\033]133;C\\007'; }\n"
  "  autoload -Uz add-zsh-hook\n"
  "  add-zsh-hook precmd __iupterm_precmd\n"
  "  add-zsh-hook preexec __iupterm_preexec\n"
  "fi\n";

static const char* itermPtyBaseName(const char* path)
{
  const char* slash = strrchr(path, '/');
  return slash ? slash + 1 : path;
}

static char* itermPtyShellFile(const char* dir, const char* name)
{
  char* path = (char*)malloc(strlen(dir) + strlen(name) + 2);
  if (path)
    sprintf(path, "%s/%s", dir, name);
  return path;
}

static void itermPtyShellRemove(char* dir)
{
  static const char* files[] = { "bashrc", ".zshenv" };
  int i;
  if (!dir)
    return;
  for (i = 0; i < 2; i++)
  {
    char* path = itermPtyShellFile(dir, files[i]);
    if (path)
    {
      unlink(path);
      free(path);
    }
  }
  rmdir(dir);
  free(dir);
}

static char* itermPtyShellSetup(const char* shell)
{
  const char* tmp = getenv("TMPDIR");
  const char* name = strcmp(shell, "bash") == 0 ? "bashrc" : ".zshenv";
  const char* text = strcmp(shell, "bash") == 0 ? iterm_bash_rc : iterm_zsh_env;
  char* dir;
  char* path;
  FILE* file;

  if (!tmp || !tmp[0])
    tmp = "/tmp";
  dir = (char*)malloc(strlen(tmp) + 32);
  if (!dir)
    return NULL;
  sprintf(dir, "%s/iupterm-XXXXXX", tmp);
  if (!mkdtemp(dir))
  {
    free(dir);
    return NULL;
  }

  path = itermPtyShellFile(dir, name);
  file = path ? fopen(path, "w") : NULL;
  free(path);
  if (!file)
  {
    itermPtyShellRemove(dir);
    return NULL;
  }
  fputs(text, file);
  fclose(file);
  return dir;
}

int iupTermPtyAvailable(void)
{
  return 1;
}

static void itermPtyChildSetup(int slave, const char* term_name)
{
  sigset_t mask;
  int i;

  setsid();
#ifdef TIOCSCTTY
  ioctl(slave, TIOCSCTTY, 0);
#endif

  dup2(slave, 0);
  dup2(slave, 1);
  dup2(slave, 2);
  if (slave > 2)
    close(slave);

  for (i = 1; i < NSIG; i++)
    signal(i, SIG_DFL);
  sigemptyset(&mask);
  sigprocmask(SIG_SETMASK, &mask, NULL);

  setenv("TERM", term_name, 1);
  setenv("COLORTERM", "truecolor", 1);
  unsetenv("LINES");
  unsetenv("COLUMNS");
}

ItermPty* iupTermPtyStart(const char* cmd, const char* term_name, int cols, int rows, int shell_integration, char* error, int error_size)
{
  ItermPty* pty;
  char** argv;
  char* argv_copy = NULL;
  struct winsize ws;
  int master, slave, errpipe[2];
  char* name;
  pid_t pid;
  char* shell_dir = NULL;
  const char* shell = NULL;
  char* bash_argv[4] = { NULL, NULL, NULL, NULL };

  if (!cmd || !cmd[0])
  {
    cmd = getenv("SHELL");
    if (!cmd || !cmd[0])
    {
#ifdef __ANDROID__
      cmd = "/system/bin/sh";
#else
      cmd = "/bin/sh";
#endif
    }
  }
  if (!term_name || !term_name[0])
    term_name = "xterm-256color";

  argv = itermPtySplitArgs(cmd, &argv_copy);
  if (!argv)
  {
    itermPtySetError(error, error_size, "invalid command line");
    return NULL;
  }

  master = posix_openpt(O_RDWR | O_NOCTTY);
  if (master < 0 || grantpt(master) < 0 || unlockpt(master) < 0)
  {
    itermPtySetError(error, error_size, strerror(errno));
    if (master >= 0) close(master);
    itermPtyFreeArgs(argv, argv_copy);
    return NULL;
  }

  name = ptsname(master);
  if (!name)
  {
    itermPtySetError(error, error_size, strerror(errno));
    close(master);
    itermPtyFreeArgs(argv, argv_copy);
    return NULL;
  }

  slave = open(name, O_RDWR | O_NOCTTY);
  if (slave < 0)
  {
    itermPtySetError(error, error_size, strerror(errno));
    close(master);
    itermPtyFreeArgs(argv, argv_copy);
    return NULL;
  }

  ws.ws_col = (unsigned short)(cols > 0 ? cols : 80);
  ws.ws_row = (unsigned short)(rows > 0 ? rows : 24);
  ws.ws_xpixel = 0;
  ws.ws_ypixel = 0;
  ioctl(master, TIOCSWINSZ, &ws);

  if (pipe(errpipe) < 0)
  {
    itermPtySetError(error, error_size, strerror(errno));
    close(slave);
    close(master);
    itermPtyFreeArgs(argv, argv_copy);
    return NULL;
  }
  fcntl(errpipe[1], F_SETFD, FD_CLOEXEC);

  if (shell_integration && argv[0] && !argv[1])
  {
    shell = itermPtyBaseName(argv[0]);
    if (strcmp(shell, "bash") == 0 || strcmp(shell, "zsh") == 0)
      shell_dir = itermPtyShellSetup(shell);
    if (shell_dir && strcmp(shell, "bash") == 0)
    {
      bash_argv[0] = argv[0];
      bash_argv[1] = (char*)"--rcfile";
      bash_argv[2] = itermPtyShellFile(shell_dir, "bashrc");
      bash_argv[3] = NULL;
    }
  }

  pid = fork();
  if (pid < 0)
  {
    itermPtySetError(error, error_size, strerror(errno));
    close(errpipe[0]);
    close(errpipe[1]);
    close(slave);
    close(master);
    itermPtyFreeArgs(argv, argv_copy);
    itermPtyShellRemove(shell_dir);
    free(bash_argv[2]);
    return NULL;
  }

  if (pid == 0)
  {
    int err;
    close(errpipe[0]);
    close(master);
    itermPtyChildSetup(slave, term_name);
    if (shell_dir && strcmp(shell, "bash") == 0)
      argv = bash_argv;
    else if (shell_dir)
    {
      const char* zdotdir = getenv("ZDOTDIR");
      if (zdotdir)
        setenv("IUPTERM_ZDOTDIR", zdotdir, 1);
      setenv("ZDOTDIR", shell_dir, 1);
    }
    execvp(argv[0], argv);
    err = errno;
    if (write(errpipe[1], &err, sizeof(err)) < 0)
      err = 0;
    _exit(127);
  }

  close(errpipe[1]);
  close(slave);
  free(bash_argv[2]);

  {
    int child_errno = 0;
    if (read(errpipe[0], &child_errno, sizeof(child_errno)) == (ssize_t)sizeof(child_errno))
    {
      int status;
      close(errpipe[0]);
      close(master);
      waitpid(pid, &status, 0);
      itermPtySetError(error, error_size, strerror(child_errno));
      itermPtyFreeArgs(argv, argv_copy);
      itermPtyShellRemove(shell_dir);
      return NULL;
    }
    close(errpipe[0]);
  }

  itermPtyFreeArgs(argv, argv_copy);

  fcntl(master, F_SETFL, fcntl(master, F_GETFL, 0) | O_NONBLOCK);
  fcntl(master, F_SETFD, FD_CLOEXEC);

  pty = (ItermPty*)calloc(1, sizeof(ItermPty));
  if (!pty)
  {
    itermPtySetError(error, error_size, "out of memory");
    close(master);
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
    itermPtyShellRemove(shell_dir);
    return NULL;
  }

  pty->fd = master;
  pty->pid = pid;
  pty->status = 0;
  pty->shell_dir = shell_dir;
  return pty;
}

int iupTermPtyRead(ItermPty* pty, char* buf, int max)
{
  ssize_t n;

  if (!pty || pty->fd < 0)
    return -1;

  n = read(pty->fd, buf, (size_t)max);
  if (n > 0)
    return (int)n;
  if (n == 0)
    return -1;
  if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
    return 0;
  return -1;
}

int iupTermPtyWrite(ItermPty* pty, const char* buf, int len)
{
  int total = 0, blocked = 0;

  if (!pty || pty->fd < 0)
    return -1;

  while (total < len)
  {
    ssize_t n = write(pty->fd, buf + total, (size_t)(len - total));
    if (n > 0)
    {
      total += (int)n;
      blocked = 0;
      continue;
    }
    if (n < 0 && errno == EINTR)
      continue;
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
      struct timespec ts;
      /* a child that stopped reading must not freeze the main loop */
      if (++blocked > 100)
        break;
      ts.tv_sec = 0;
      ts.tv_nsec = 1000000;
      nanosleep(&ts, NULL);
      continue;
    }
    return total > 0 ? total : -1;
  }
  return total;
}

void iupTermPtyResize(ItermPty* pty, int cols, int rows)
{
  struct winsize ws;

  if (!pty || pty->fd < 0 || cols <= 0 || rows <= 0)
    return;

  ws.ws_col = (unsigned short)cols;
  ws.ws_row = (unsigned short)rows;
  ws.ws_xpixel = 0;
  ws.ws_ypixel = 0;
  ioctl(pty->fd, TIOCSWINSZ, &ws);
}

int iupTermPtyCheckExit(ItermPty* pty, int* status)
{
  int st = 0;
  pid_t r;

  if (!pty)
    return 1;
  if (pty->exited)
  {
    if (status) *status = pty->status;
    return 1;
  }

  r = waitpid(pty->pid, &st, WNOHANG);
  if (r != pty->pid)
    return 0;

  pty->exited = 1;
  if (WIFEXITED(st))
    pty->status = WEXITSTATUS(st);
  else if (WIFSIGNALED(st))
    pty->status = 128 + WTERMSIG(st);
  else
    pty->status = 0;

  if (status) *status = pty->status;
  return 1;
}

long iupTermPtyGetPid(ItermPty* pty)
{
  if (!pty || pty->exited)
    return -1;
  return (long)pty->pid;
}

int iupTermPtyHostIsLocal(const char* host)
{
  char local[256];
  if (gethostname(local, sizeof(local)) != 0)
    return itermPtySameHost(host, NULL);
  local[sizeof(local) - 1] = 0;
  return itermPtySameHost(host, local);
}

int iupTermPtyFd(ItermPty* pty)
{
  return pty ? pty->fd : -1;
}

void iupTermPtyKill(ItermPty* pty, int force)
{
  if (!pty || pty->exited)
    return;
  kill(pty->pid, force ? SIGKILL : SIGHUP);
}

void iupTermPtyClose(ItermPty* pty)
{
  if (!pty)
    return;

  if (!pty->exited)
  {
    int i;
    kill(pty->pid, SIGHUP);
    for (i = 0; i < 20 && !iupTermPtyCheckExit(pty, NULL); i++)
    {
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 5000000;
      nanosleep(&ts, NULL);
    }
    if (!pty->exited)
    {
      kill(pty->pid, SIGKILL);
      waitpid(pty->pid, NULL, 0);
      pty->exited = 1;
    }
  }

  if (pty->fd >= 0)
    close(pty->fd);
  itermPtyShellRemove(pty->shell_dir);
  free(pty);
}

#elif defined(ITERM_PTY_WIN)

#define ITERM_PTY_PIPE_SIZE (256*1024)

#include <windows.h>

#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif

typedef void* ItermHPCON;
typedef HRESULT (WINAPI* ItermCreatePseudoConsole)(COORD, HANDLE, HANDLE, DWORD, ItermHPCON*);
typedef HRESULT (WINAPI* ItermResizePseudoConsole)(ItermHPCON, COORD);
typedef void (WINAPI* ItermClosePseudoConsole)(ItermHPCON);

struct _ItermPty
{
  ItermHPCON hpc;
  HANDLE in_write;
  HANDLE out_read;
  HANDLE process;
  DWORD pid;
  int exited;
  int status;
};

static ItermCreatePseudoConsole iterm_create_pc = NULL;
static ItermResizePseudoConsole iterm_resize_pc = NULL;
static ItermClosePseudoConsole iterm_close_pc = NULL;

static int itermPtyLoadConPty(void)
{
  static int loaded = 0;

  if (!loaded)
  {
    HMODULE kernel = GetModuleHandleA("kernel32.dll");
    loaded = 1;
    if (kernel)
    {
      iterm_create_pc = (ItermCreatePseudoConsole)(void*)GetProcAddress(kernel, "CreatePseudoConsole");
      iterm_resize_pc = (ItermResizePseudoConsole)(void*)GetProcAddress(kernel, "ResizePseudoConsole");
      iterm_close_pc = (ItermClosePseudoConsole)(void*)GetProcAddress(kernel, "ClosePseudoConsole");
    }
  }
  return iterm_create_pc && iterm_resize_pc && iterm_close_pc;
}

int iupTermPtyAvailable(void)
{
  return itermPtyLoadConPty();
}

static const DWORD iterm_std_ids[3] = { STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };

static void itermPtyStdHandlesSave(HANDLE* saved)
{
  int i;
  for (i = 0; i < 3; i++)
  {
    saved[i] = GetStdHandle(iterm_std_ids[i]);
    SetStdHandle(iterm_std_ids[i], NULL);
  }
}

static void itermPtyStdHandlesRestore(HANDLE* saved)
{
  int i;
  for (i = 0; i < 3; i++)
    SetStdHandle(iterm_std_ids[i], saved[i]);
}

ItermPty* iupTermPtyStart(const char* cmd, const char* term_name, int cols, int rows, int shell_integration, char* error, int error_size)
{
  ItermPty* pty;
  STARTUPINFOEXA si;
  PROCESS_INFORMATION pi;
  LPPROC_THREAD_ATTRIBUTE_LIST attrs = NULL;
  HANDLE in_read = NULL, in_write = NULL, out_read = NULL, out_write = NULL;
  ItermHPCON hpc = NULL;
  SIZE_T attr_size = 0;
  COORD size;
  char* cmdline;
  char comspec[MAX_PATH];
  HANDLE std_handle[3];
  BOOL started;

  (void)shell_integration;
  if (!itermPtyLoadConPty())
  {
    itermPtySetError(error, error_size, "ConPTY not available (needs Windows 10 1809 or later)");
    return NULL;
  }

  if (!cmd || !cmd[0])
  {
    if (GetEnvironmentVariableA("COMSPEC", comspec, MAX_PATH) > 0)
      cmd = comspec;
    else
      cmd = "cmd.exe";
  }
  if (!term_name || !term_name[0])
    term_name = "xterm-256color";

  if (!CreatePipe(&in_read, &in_write, NULL, 0) || !CreatePipe(&out_read, &out_write, NULL, ITERM_PTY_PIPE_SIZE))
  {
    itermPtySetError(error, error_size, "cannot create pipes");
    goto fail;
  }

  size.X = (SHORT)(cols > 0 ? cols : 80);
  size.Y = (SHORT)(rows > 0 ? rows : 24);
  if (iterm_create_pc(size, in_read, out_write, 0, &hpc) != S_OK)
  {
    itermPtySetError(error, error_size, "CreatePseudoConsole failed");
    goto fail;
  }

  CloseHandle(in_read);
  CloseHandle(out_write);
  in_read = NULL;
  out_write = NULL;

  memset(&si, 0, sizeof(si));
  si.StartupInfo.cb = sizeof(si);

  InitializeProcThreadAttributeList(NULL, 1, 0, &attr_size);
  attrs = (LPPROC_THREAD_ATTRIBUTE_LIST)malloc(attr_size);
  if (!attrs || !InitializeProcThreadAttributeList(attrs, 1, 0, &attr_size) ||
      !UpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hpc, sizeof(hpc), NULL, NULL))
  {
    itermPtySetError(error, error_size, "cannot set up process attributes");
    goto fail;
  }
  si.lpAttributeList = attrs;

  SetEnvironmentVariableA("TERM", term_name);
  SetEnvironmentVariableA("COLORTERM", "truecolor");

  cmdline = iupStrDup(cmd);
  memset(&pi, 0, sizeof(pi));

  /* the child copies our standard handles, so a redirected stdout would win over the pseudo console */
  itermPtyStdHandlesSave(std_handle);
  started = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                           EXTENDED_STARTUPINFO_PRESENT,
                           NULL, NULL, &si.StartupInfo, &pi);
  itermPtyStdHandlesRestore(std_handle);

  if (!started)
  {
    free(cmdline);
    itermPtySetError(error, error_size, "cannot start the command");
    goto fail;
  }
  free(cmdline);

  DeleteProcThreadAttributeList(attrs);
  free(attrs);
  CloseHandle(pi.hThread);

  pty = (ItermPty*)calloc(1, sizeof(ItermPty));
  if (!pty)
  {
    itermPtySetError(error, error_size, "out of memory");
    TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hProcess);
    iterm_close_pc(hpc);
    CloseHandle(in_write);
    CloseHandle(out_read);
    return NULL;
  }

  pty->hpc = hpc;
  pty->in_write = in_write;
  pty->out_read = out_read;
  pty->process = pi.hProcess;
  pty->pid = pi.dwProcessId;
  return pty;

fail:
  if (attrs)
  {
    DeleteProcThreadAttributeList(attrs);
    free(attrs);
  }
  if (hpc) iterm_close_pc(hpc);
  if (in_read) CloseHandle(in_read);
  if (in_write) CloseHandle(in_write);
  if (out_read) CloseHandle(out_read);
  if (out_write) CloseHandle(out_write);
  return NULL;
}

int iupTermPtyRead(ItermPty* pty, char* buf, int max)
{
  DWORD avail = 0, got = 0;

  if (!pty || !pty->out_read)
    return -1;

  if (!PeekNamedPipe(pty->out_read, NULL, 0, NULL, &avail, NULL))
    return -1;

  /* the host keeps the write end open, so the pipe alone never reports the child leaving */
  if (avail == 0)
    return iupTermPtyCheckExit(pty, NULL) ? -1 : 0;
  if (avail > (DWORD)max)
    avail = (DWORD)max;

  if (!ReadFile(pty->out_read, buf, avail, &got, NULL) || got == 0)
    return -1;
  return (int)got;
}

int iupTermPtyWrite(ItermPty* pty, const char* buf, int len)
{
  DWORD written = 0;

  if (!pty || !pty->in_write)
    return -1;
  if (!WriteFile(pty->in_write, buf, (DWORD)len, &written, NULL))
    return -1;
  return (int)written;
}

void iupTermPtyResize(ItermPty* pty, int cols, int rows)
{
  COORD size;

  if (!pty || !pty->hpc || cols <= 0 || rows <= 0)
    return;

  size.X = (SHORT)cols;
  size.Y = (SHORT)rows;
  iterm_resize_pc(pty->hpc, size);
}

int iupTermPtyCheckExit(ItermPty* pty, int* status)
{
  DWORD code = 0;

  if (!pty)
    return 1;
  if (pty->exited)
  {
    if (status) *status = pty->status;
    return 1;
  }

  if (!GetExitCodeProcess(pty->process, &code) || code == STILL_ACTIVE)
    return 0;

  pty->exited = 1;
  pty->status = (int)code;
  if (status) *status = pty->status;
  return 1;
}

long iupTermPtyGetPid(ItermPty* pty)
{
  if (!pty || pty->exited)
    return -1;
  return (long)pty->pid;
}

int iupTermPtyHostIsLocal(const char* host)
{
  char local[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD size = sizeof(local);
  if (!GetComputerNameA(local, &size))
    return itermPtySameHost(host, NULL);
  return itermPtySameHost(host, local);
}

int iupTermPtyFd(ItermPty* pty)
{
  (void)pty;
  return -1;
}

void iupTermPtyKill(ItermPty* pty, int force)
{
  (void)force;
  if (!pty || pty->exited)
    return;
  TerminateProcess(pty->process, 1);
}

void iupTermPtyClose(ItermPty* pty)
{
  if (!pty)
    return;

  if (pty->in_write) CloseHandle(pty->in_write);

  /* ClosePseudoConsole blocks while the host still has output to hand over */
  if (pty->out_read)
  {
    char drain[4096];
    int i;
    for (i = 0; i < 64; i++)
    {
      DWORD avail = 0, got = 0;
      if (!PeekNamedPipe(pty->out_read, NULL, 0, NULL, &avail, NULL) || avail == 0)
        break;
      if (!ReadFile(pty->out_read, drain, avail > sizeof(drain) ? sizeof(drain) : avail, &got, NULL))
        break;
    }
  }

  if (pty->hpc) iterm_close_pc(pty->hpc);
  if (pty->out_read) CloseHandle(pty->out_read);

  if (!pty->exited)
  {
    WaitForSingleObject(pty->process, 200);
    if (!iupTermPtyCheckExit(pty, NULL))
      TerminateProcess(pty->process, 1);
  }
  CloseHandle(pty->process);
  free(pty);
}

#else

int iupTermPtyAvailable(void)
{
  return 0;
}

ItermPty* iupTermPtyStart(const char* cmd, const char* term_name, int cols, int rows, int shell_integration, char* error, int error_size)
{
  (void)cmd; (void)term_name; (void)cols; (void)rows; (void)shell_integration;
  itermPtySetError(error, error_size, "no pseudo-terminal support on this platform");
  return NULL;
}

int iupTermPtyRead(ItermPty* pty, char* buf, int max) { (void)pty; (void)buf; (void)max; return -1; }
int iupTermPtyWrite(ItermPty* pty, const char* buf, int len) { (void)pty; (void)buf; (void)len; return -1; }
void iupTermPtyResize(ItermPty* pty, int cols, int rows) { (void)pty; (void)cols; (void)rows; }
int iupTermPtyCheckExit(ItermPty* pty, int* status) { (void)pty; if (status) *status = 0; return 1; }
long iupTermPtyGetPid(ItermPty* pty) { (void)pty; return -1; }
int iupTermPtyHostIsLocal(const char* host) { return itermPtySameHost(host, NULL); }
int iupTermPtyFd(ItermPty* pty) { (void)pty; return -1; }
void iupTermPtyKill(ItermPty* pty, int force) { (void)pty; (void)force; }
void iupTermPtyClose(ItermPty* pty) { (void)pty; }

#endif
