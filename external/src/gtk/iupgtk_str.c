/** \file
 * \brief GTK Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>             
#include <limits.h>             

#include <gtk/gtk.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"

#include "iupgtk_drv.h"


static int iupgtk_utf8mode = 0;

void iupgtkStrSetUTF8Mode(int utf8mode)
{
  iupgtk_utf8mode = utf8mode;
}

int iupgtkStrGetUTF8Mode(void)
{
  return iupgtk_utf8mode;
}

static char* gtkStrToUTF8(const char *str, int len, const char* charset)
{
  return g_convert(str, len, "UTF-8", charset, NULL, NULL, NULL);
}

static char* gtkStrFromUTF8(const char *str, const char* charset)
{
  return g_convert(str, -1, charset, "UTF-8", NULL, NULL, NULL);
}

static char* gtkLastConvertUTF8 = NULL;

void iupgtkStrRelease(void)
{
  if (gtkLastConvertUTF8)
    g_free(gtkLastConvertUTF8);
}

char* iupgtkStrConvertToSystemLen(const char* str, int *len)  /* From IUP (current locale) to GTK */
{
  const char *charset = NULL;

  if (!str || *str == 0 || iupgtk_utf8mode)
    return (char*)str;

  if (g_get_charset(&charset)==TRUE)  /* current locale is already UTF-8 */
  {
    if (g_utf8_validate(str, *len, NULL))
      return (char*)str;
    else
      charset = "ISO8859-1";    /* if string is not UTF-8, assume ISO8859-1 */
  }
  else
  {
    if (!charset || iupStrIsAscii(str))
      return (char*)str;
  }

  if (gtkLastConvertUTF8)
    g_free(gtkLastConvertUTF8);
  gtkLastConvertUTF8 = gtkStrToUTF8(str, *len, charset);
  if (!gtkLastConvertUTF8) return (char*)str;
  *len = (int)strlen(gtkLastConvertUTF8);
  return gtkLastConvertUTF8;
}

IUP_DRV_API char* iupgtkStrConvertToSystem(const char* str)  /* From IUP (current locale) to GTK */
{
  const char *charset = NULL;

  if (!str || *str == 0 || iupgtk_utf8mode)
    return (char*)str;

  if (g_get_charset(&charset)==TRUE)  /* current locale is already UTF-8 */
  {
    if (g_utf8_validate(str, -1, NULL))
      return (char*)str;
    else
      charset = "ISO8859-1";    /* if string is not UTF-8, assume ISO8859-1 */
  }
  else
  {
    if (!charset || iupStrIsAscii(str))
      return (char*)str;
  }

  if (gtkLastConvertUTF8)
    g_free(gtkLastConvertUTF8);
  gtkLastConvertUTF8 = gtkStrToUTF8(str, -1, charset);
  if (!gtkLastConvertUTF8) return (char*)str;
  return gtkLastConvertUTF8;
}

char* iupgtkStrConvertFromSystem(const char* str)  /* From GTK to IUP (current locale) */
{
  const gchar *charset = NULL;

  if (!str || *str == 0 || iupgtk_utf8mode)
    return (char*)str;

  if (g_get_charset(&charset)==TRUE)  /* current locale is already UTF-8 */
  {
    if (g_utf8_validate(str, -1, NULL))
      return (char*)str;
    else
      charset = "ISO8859-1";    /* if string is not UTF-8, assume ISO8859-1 */
  }
  else
  {
    if (!charset || iupStrIsAscii(str))
      return (char*)str;
  }

  if (gtkLastConvertUTF8)
    g_free(gtkLastConvertUTF8);
  gtkLastConvertUTF8 = gtkStrFromUTF8(str, charset);
  if (!gtkLastConvertUTF8) return (char*)str;
  return gtkLastConvertUTF8;
}

static gboolean gtkGetFilenameCharset(const gchar **filename_charset)
{
  const gchar **charsets = NULL;
  gboolean is_utf8 = FALSE;
  
#if GTK_CHECK_VERSION(2, 6, 0)
  is_utf8 = g_get_filename_charsets(&charsets);
#endif

  if (filename_charset && charsets)
    *filename_charset = charsets[0];
  
  return is_utf8;
}

char* iupgtkStrConvertToFilename(const char* str)   /* From IUP (current locale) to Filename */
{
  const gchar *charset = NULL;

  if (!str || *str == 0 || iupgtk_utf8mode)
    return (char*)str;

  if (gtkGetFilenameCharset(&charset)==TRUE)  /* current locale is already UTF-8 */
  {
    if (g_utf8_validate(str, -1, NULL))
      return (char*)str;
    else
      charset = "ISO8859-1";    /* if string is not UTF-8, assume ISO8859-1 */
  }
  else
  {
    if (!charset || iupStrIsAscii(str))
      return (char*)str;
  }

  if (gtkLastConvertUTF8)
    g_free(gtkLastConvertUTF8);
  gtkLastConvertUTF8 = gtkStrFromUTF8(str, charset);
  if (!gtkLastConvertUTF8) return (char*)str;
  return gtkLastConvertUTF8;
}

char* iupgtkStrConvertFromFilename(const char* str)   /* From Filename to IUP */
{
  const char *charset = NULL;

  if (!str || *str == 0 || iupgtk_utf8mode)
    return (char*)str;

  if (gtkGetFilenameCharset(&charset)==TRUE)  /* current locale is already UTF-8 */
  {
    if (g_utf8_validate(str, -1, NULL))
      return (char*)str;
    else
      charset = "ISO8859-1";    /* if string is not UTF-8, assume ISO8859-1 */
  }
  else
  {
    if (!charset || iupStrIsAscii(str))
      return (char*)str;
  }

  if (gtkLastConvertUTF8)
    g_free(gtkLastConvertUTF8);
  gtkLastConvertUTF8 = gtkStrToUTF8(str, -1, charset);
  if (!gtkLastConvertUTF8) return (char*)str;
  return gtkLastConvertUTF8;
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
    char* g_buffer;

    const char *charset = NULL;
    if (g_get_charset(&charset) == TRUE)  /* current locale is already UTF-8 */
    {
      if (g_utf8_validate(str, len, NULL))
        return iupStrCopyToUtf8Buffer(str, len, utf8_buffer, utf8_buffer_max);
      else
        charset = "ISO8859-1";  /* if string is not UTF-8, assume ISO8859-1 */
    }

    if (!charset)
      charset = "ISO8859-1";  /* if charset not found, assume ISO8859-1 */

    g_buffer = g_convert(str, len, "UTF-8", charset, NULL, NULL, NULL);
    if (!g_buffer)
      return iupStrCopyToUtf8Buffer(str, len, utf8_buffer, utf8_buffer_max);

    mlen = (int)strlen(g_buffer);
    utf8_buffer = iupCheckUtf8Buffer(utf8_buffer, utf8_buffer_max, mlen);
    memcpy(utf8_buffer, g_buffer, mlen);
    utf8_buffer[mlen] = 0;

    g_free(g_buffer);
  }

  return utf8_buffer;
}

