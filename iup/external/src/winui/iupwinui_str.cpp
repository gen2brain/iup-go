/** \file
 * \brief WinUI Driver - String Conversion (UTF-8/UTF-16)
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_str.h"
}

#include "iupwinui_drv.h"

using namespace winrt;


hstring iupwinuiStringToHString(const char* str)
{
  if (!str || !str[0])
    return hstring();

  int wlen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
  if (wlen <= 0)
    return hstring();

  std::wstring wstr(wlen - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str, -1, &wstr[0], wlen);

  return hstring(wstr);
}

char* iupwinuiHStringToString(const hstring& str)
{
  if (str.empty())
    return NULL;

  int len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
  if (len <= 0)
    return NULL;

  char* buf = iupStrGetMemory(len);
  WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, buf, len, NULL, NULL);

  return buf;
}

std::wstring iupwinuiStringToWString(const char* str)
{
  if (!str || !str[0])
    return std::wstring();

  int wlen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
  if (wlen <= 0)
    return std::wstring();

  std::wstring wstr(wlen - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str, -1, &wstr[0], wlen);

  return wstr;
}

hstring iupwinuiProcessMnemonic(const char* str, char* c)
{
  if (c)
    *c = 0;

  if (!str || !str[0])
    return hstring();

  char mnemonic = 0;
  char* processed = iupStrProcessMnemonic(str, &mnemonic, -1);
  if (!processed)
    return hstring();

  hstring result = iupwinuiStringToHString(processed);

  if (processed != str)
    free(processed);

  if (c)
    *c = mnemonic;

  return result;
}
