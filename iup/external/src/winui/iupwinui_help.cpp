/** \file
 * \brief WinUI Driver - Help System
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iup_str.h"
}

#include "iupwinui_drv.h"

extern "C" int IupExecute(const char* filename, const char* parameters)
{
  INT_PTR err;

  if (!filename)
    return -1;

  err = (INT_PTR)ShellExecuteA(GetDesktopWindow(), "open", filename, parameters, NULL, SW_SHOWNORMAL);
  if (err <= 32)
  {
    switch (err)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      return -2;
    default:
      return -1;
    }
  }

  return 1;
}

extern "C" int IupExecuteWait(const char* filename, const char* parameters)
{
  SHELLEXECUTEINFOA info;

  if (!filename)
    return -1;

  ZeroMemory(&info, sizeof(SHELLEXECUTEINFOA));
  info.cbSize = sizeof(SHELLEXECUTEINFOA);
  info.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE;
  info.hwnd = GetDesktopWindow();
  info.lpVerb = "open";
  info.lpFile = filename;
  info.lpParameters = parameters;
  info.nShow = SW_SHOWNORMAL;

  if (!ShellExecuteExA(&info))
  {
    INT_PTR err = (INT_PTR)info.hInstApp;
    switch (err)
    {
    case SE_ERR_FNF:
    case SE_ERR_PNF:
      return -2;
    default:
      return -1;
    }
  }

  WaitForSingleObject(info.hProcess, INFINITE);
  CloseHandle(info.hProcess);

  return 1;
}

extern "C" int IupHelp(const char* url)
{
  return IupExecute(url, NULL);
}

extern "C" void IupLog(const char* type, const char* format, ...)
{
  va_list arglist;
  char msg[1024];
  int len;

  if (!format)
    return;

  va_start(arglist, format);
  len = vsnprintf(msg, sizeof(msg), format, arglist);
  va_end(arglist);

  if (len > 0)
  {
    OutputDebugStringA(msg);

    if (type && iupStrEqualNoCase(type, "ERROR"))
      fprintf(stderr, "%s", msg);
    else
      fprintf(stdout, "%s", msg);
  }
}
