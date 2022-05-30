/** \file
 * \brief Windows Driver IupHelp
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <shellapi.h>

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"
#include "iupwin_str.h"

#ifndef SEE_MASK_NOASYNC
#define SEE_MASK_NOASYNC 0x00000100
#endif

IUP_API int IupExecute(const char *filename, const char* parameters)
{
  int err = (int)ShellExecute(GetDesktopWindow(), TEXT("open"), iupwinStrToSystemFilename(filename), iupwinStrToSystemFilename(parameters), NULL, SW_SHOWNORMAL);
  if (err <= 32)
  {
    switch (err)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      return -2; /* File not found */
    default:
      return -1; /* Generic error */
    }
  }
  return 1;
}

IUP_API int IupExecuteWait(const char *filename, const char* parameters)
{                                          
  SHELLEXECUTEINFO ExecInfo;
  memset(&ExecInfo, 0, sizeof(SHELLEXECUTEINFO));

  ExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
  ExecInfo.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE;
  ExecInfo.hwnd = GetDesktopWindow();
  ExecInfo.lpVerb = TEXT("open");
  ExecInfo.lpFile = iupwinStrToSystemFilename(filename);
  ExecInfo.lpParameters = iupwinStrToSystemFilename(parameters);
  ExecInfo.nShow = SW_SHOWNORMAL;

  if (!ShellExecuteEx(&ExecInfo))
  {
    int err = (int)ExecInfo.hInstApp;
    switch (err)
    {
    case SE_ERR_FNF:
    case SE_ERR_PNF:
      return -2; /* File not found */
    default:
      return -1; /* Generic error */
    }
  }

  WaitForSingleObject(ExecInfo.hProcess, INFINITE);

  return 1;
}

IUP_API int IupHelp(const char* url)
{
  return IupExecute(url, NULL);
}
