/** \file
 * \brief Windows Unicode Encapsulation
 *
 * See Copyright Notice in "iup.h"
 */
#include <windows.h>
#include <commctrl.h>

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>             

#include "iup_export.h"
#include "iupwin_str.h"
#include "iup_str.h"


/* From MSDN:
- Internally, the ANSI version translates the string to Unicode. 
- The ANSI versions are also less efficient, 
  because the operating system must convert the ANSI strings to Unicode at run time. 

  So, there is no point in doing a lib that calls both ANSI and Unicode API. 
  In the simple and ideal world we need only the Unicode API and to handle the conversion when necessary.
  The same conversion would exist anyway if using the ANSI API.

- The standard file I/O functions, like fopen, use ANSI file names. 
  But if your files have Unicode names, then you may consider using UTF-8 
  so later the application can recover the original Unicode version.
*/
static int iupwin_utf8mode = 0;    /* default is NOT using UTF-8 */
static int iupwin_utf8mode_file = 0;  


void iupwinStrSetUTF8Mode(int utf8mode)
{
#ifdef UNICODE   /* can not set if not Unicode */
  iupwin_utf8mode = utf8mode;
#else
  (void)utf8mode;
#endif
}

void iupwinStrSetUTF8ModeFile(int utf8mode)
{
#ifdef UNICODE   /* can not set if not Unicode */
  iupwin_utf8mode_file = utf8mode;
#else
  (void)utf8mode;
#endif
}

int iupwinStrGetUTF8Mode(void)
{
  return iupwin_utf8mode;
}

int iupwinStrGetUTF8ModeFile(void)
{
  return iupwin_utf8mode_file;
}

static void* winStrGetMemory(int size)
{
#define MAX_BUFFERS 50
  static void* buffers[MAX_BUFFERS];
  static int buffers_sizes[MAX_BUFFERS];
  static int buffers_index = -1;

  if (size == -1) /* Frees memory */
  {
    int i;

    buffers_index = -1;
    for (i = 0; i < MAX_BUFFERS; i++)
    {
      if (buffers[i]) 
      {
        free(buffers[i]);
        buffers[i] = NULL;
      }
      buffers_sizes[i] = 0;
    }
    return NULL;
  }
  else
  {
    void* ret_buffer;

    /* init buffers array */
    if (buffers_index == -1)
    {
      memset(buffers, 0, sizeof(void*)*MAX_BUFFERS);
      memset(buffers_sizes, 0, sizeof(int)*MAX_BUFFERS);
      buffers_index = 0;
    }

    /* first allocation */
    if (!(buffers[buffers_index]))
    {
      buffers_sizes[buffers_index] = size+1;
      buffers[buffers_index] = malloc(buffers_sizes[buffers_index]);
    }
    else if (buffers_sizes[buffers_index] < size+1)  /* reallocate if necessary */
    {
      buffers_sizes[buffers_index] = size+1;
      buffers[buffers_index] = realloc(buffers[buffers_index], buffers_sizes[buffers_index]);
    }

    /* clear memory */
    memset(buffers[buffers_index], 0, buffers_sizes[buffers_index]);
    ret_buffer = buffers[buffers_index];

    buffers_index++;
    if (buffers_index == MAX_BUFFERS)
      buffers_index = 0;

    return ret_buffer;
  }
#undef MAX_BUFFERS
}

void iupwinStrRelease(void)
{
  winStrGetMemory(-1);
}

static void winStrWide2Char(const WCHAR* wstr, char* str, int len)
{
  /* cchWideChar is the wstr number of characters
     cbMultiByte is the size in bytes available in the buffer
     Returns the number of bytes written to the buffer 
  */
  if (iupwin_utf8mode)
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, len, str, 3*len, NULL, NULL);  /* str must has a larger buffer */
  else
    len = WideCharToMultiByte(CP_ACP,  0, wstr, len, str, 3*len, NULL, NULL);

  if (len<0)
    len = 0;

  str[len] = 0;
}

static void winStrChar2Wide(const char* str, WCHAR* wstr, int *len)
{
  /* cbMultiByte is the str size in bytes of the actual string
     cchWideChar is the wstr number in characters available in the buffer
     Returns the number of characters written to the buffer
  */
  if (iupwin_utf8mode)
    *len = MultiByteToWideChar(CP_UTF8, 0, str, *len, wstr, *len);
  else
    *len = MultiByteToWideChar(CP_ACP,  0, str, *len, wstr, *len);

  if (*len<0)
    *len = 0;

  wstr[*len] = 0;
}

IUP_DRV_API WCHAR* iupwinStrChar2Wide(const char* str)
{
  if (str)
  {
    int len = (int)strlen(str);
    WCHAR* wstr = (WCHAR*)malloc((len+1) * sizeof(WCHAR));
    winStrChar2Wide(str, wstr, &len);
    return wstr;
  }

  return NULL;
}

IUP_DRV_API char* iupwinStrWide2Char(const WCHAR* wstr)
{
  if (wstr)
  {
    int len = (int)wcslen(wstr);
    char* str = (char*)malloc((3*len+1) * sizeof(char));   /* str must has a larger buffer */
    winStrWide2Char(wstr, str, len);
    return str;
  }

  return NULL;
}

void iupwinStrCopy(TCHAR* dst_wstr, const char* src_str, int max_size)
{
  if (src_str)
  {
    TCHAR* src_wstr = iupwinStrToSystem(src_str);
    int len = lstrlen(src_wstr)+1;
    if (len > max_size) len = max_size;
    lstrcpyn(dst_wstr, src_wstr, len);
  }
}

IUP_DRV_API TCHAR* iupwinStrToSystemFilename(const char* str)
{
  TCHAR* wstr;
  int old_utf8mode = iupwin_utf8mode;
  if (iupwin_utf8mode)
    iupwin_utf8mode = iupwin_utf8mode_file;
  wstr = iupwinStrToSystem(str);
  iupwin_utf8mode = old_utf8mode;
  return wstr;
}

IUP_DRV_API char* iupwinStrFromSystemFilename(const TCHAR* wstr)
{
  char* str;
  int old_utf8mode = iupwin_utf8mode;
  if (iupwin_utf8mode)
    iupwin_utf8mode = iupwin_utf8mode_file;
  str = iupwinStrFromSystem(wstr);
  iupwin_utf8mode = old_utf8mode;
  return str;
}

IUP_DRV_API TCHAR* iupwinStrToSystem(const char* str)
{
#ifdef UNICODE
  if (str)
  {
    int len = (int)strlen(str);
    WCHAR* wstr = (WCHAR*)winStrGetMemory((len+1) * sizeof(WCHAR));
    winStrChar2Wide(str, wstr, &len);
    return wstr;
  }
  return NULL;
#else
  return (char*)str;
#endif
}

IUP_DRV_API char* iupwinStrFromSystem(const TCHAR* wstr)
{
#ifdef UNICODE
  if (wstr)
  {
    int len = (int)wcslen(wstr);
    char* str = (char*)winStrGetMemory((3*len+1) * sizeof(char));    /* str must has a large buffer because the UTF-8 string can be larger than the original */
    winStrWide2Char(wstr, str, len);
    return str;
  }
  return NULL;
#else
  return (char*)wstr;
#endif
}

IUP_DRV_API TCHAR* iupwinStrToSystemLen(const char* str, int *len)
{
  /* The len here is in bytes always, using UTF-8 or not.
     So, when converted to Unicode must return the actual size in characters. */
#ifdef UNICODE
  if (str)
  {
    WCHAR* wstr = (WCHAR*)winStrGetMemory(((*len)+1) * sizeof(WCHAR));
    winStrChar2Wide(str, wstr, len);
    return wstr;
  }
  return NULL;
#else
  (void)len;
  return (char*)str;
#endif
}

static char* iupCheckUtf8Buffer(char* utf8_buffer, int *utf8_buffer_max, int len)
{
  if (!utf8_buffer)
  {
    utf8_buffer = malloc(len + 1);
    *utf8_buffer_max = len;
  }
  else if (*utf8_buffer_max < len)
  {
    utf8_buffer = realloc(utf8_buffer, len + 1);
    *utf8_buffer_max = len;
  }

  return utf8_buffer;
}

static char* iupStrCopyToUtf8Buffer(const char* str, int len, char* utf8_buffer, int *utf8_buffer_max)
{
  utf8_buffer = iupCheckUtf8Buffer(utf8_buffer, utf8_buffer_max, len);
  memcpy(utf8_buffer, str, len);
  utf8_buffer[len] = 0;
  return utf8_buffer;
}

/* Used in glfont */
IUP_SDK_API char* iupStrConvertToUTF8(const char* str, int len, char* utf8_buffer, int *utf8_buffer_max, int utf8mode)
{
  if (utf8mode || iupStrIsAscii(str)) /* string is already utf8 or is ascii */
    return iupStrCopyToUtf8Buffer(str, len, utf8_buffer, utf8_buffer_max);

  {
    int mlen;
    wchar_t* wstr;
    int wlen = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    if (!wlen)
      return iupStrCopyToUtf8Buffer(str, len, utf8_buffer, utf8_buffer_max);

    wstr = (wchar_t*)calloc((wlen + 1), sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, str, len, wstr, wlen);
    wstr[wlen] = 0;

    mlen = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, NULL, 0, NULL, NULL);
    if (!mlen)
    {
      free(wstr);
      return iupStrCopyToUtf8Buffer(str, len, utf8_buffer, utf8_buffer_max);
    }

    utf8_buffer = iupCheckUtf8Buffer(utf8_buffer, utf8_buffer_max, mlen);

    WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, utf8_buffer, mlen, NULL, NULL);
    utf8_buffer[mlen] = 0;

    free(wstr);
  }

  return utf8_buffer;
}

