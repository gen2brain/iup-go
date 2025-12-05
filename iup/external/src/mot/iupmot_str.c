/** \file
 * \brief Motif Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <langinfo.h>

#include <Xm/Xm.h>
#include <Xm/Text.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"

#include "iupmot_drv.h"

#ifdef IUP_USE_ICONV
#include <iconv.h>
#endif

static int iupmot_utf8mode = 0;
static int iupmot_utf8mode_file = 0;
static int iupmot_utf8_supported = -1;
static char* motLastConvertUTF8 = NULL;
static char* mot_current_charset = NULL;

static int motStrCheckUTF8Support(void)
{
  if (iupmot_utf8_supported != -1)
    return iupmot_utf8_supported;

  if (XmVERSION > 2 || (XmVERSION == 2 && XmREVISION >= 3))
    iupmot_utf8_supported = 1;
  else
    iupmot_utf8_supported = 0;

  return iupmot_utf8_supported;
}

static char* motStrGetCurrentCharset(void)
{
  if (!mot_current_charset)
    mot_current_charset = nl_langinfo(CODESET);
  return mot_current_charset;
}

static void motStrResetCharset(void)
{
  mot_current_charset = NULL;
}

static int motStrIsLocaleUTF8(void)
{
  char* charset = motStrGetCurrentCharset();
  return (iupStrEqualNoCase(charset, "UTF-8") ||
          iupStrEqualNoCase(charset, "UTF8"));
}

#ifdef IUP_USE_ICONV
static char* motStrToUTF8(const char* str, int len, const char* charset)
{
  size_t ulen = (size_t)len;
  size_t mlen = ulen * 4;
  char* utf8_buffer;
  char* utf8_ptr;
  const char* str_ptr = str;
  iconv_t cd_iconv;

  if (!charset)
    charset = "ISO-8859-1";

  cd_iconv = iconv_open("UTF-8", charset);
  if (cd_iconv == (iconv_t)-1)
    return NULL;

  utf8_buffer = malloc(mlen + 1);
  utf8_ptr = utf8_buffer;

  if (iconv(cd_iconv, (char**)&str_ptr, &ulen, &utf8_ptr, &mlen) == (size_t)-1)
  {
    free(utf8_buffer);
    iconv_close(cd_iconv);
    return NULL;
  }

  *utf8_ptr = 0;
  iconv_close(cd_iconv);

  return utf8_buffer;
}

static char* motStrFromUTF8(const char* str, const char* charset)
{
  size_t ulen = strlen(str);
  size_t mlen = ulen * 4;
  char* locale_buffer;
  char* locale_ptr;
  const char* str_ptr = str;
  iconv_t cd_iconv;

  if (!charset)
    charset = "ISO-8859-1";

  cd_iconv = iconv_open(charset, "UTF-8");
  if (cd_iconv == (iconv_t)-1)
    return NULL;

  locale_buffer = malloc(mlen + 1);
  locale_ptr = locale_buffer;

  if (iconv(cd_iconv, (char**)&str_ptr, &ulen, &locale_ptr, &mlen) == (size_t)-1)
  {
    free(locale_buffer);
    iconv_close(cd_iconv);
    return NULL;
  }

  *locale_ptr = 0;
  iconv_close(cd_iconv);

  return locale_buffer;
}
#endif

void iupmotStrSetUTF8Mode(int utf8mode)
{
  iupmot_utf8mode = utf8mode;
  if (utf8mode && !motStrIsLocaleUTF8())
  {
    char* result;
    result = setlocale(LC_CTYPE, "en_US.UTF-8");
    if (!result)
      result = setlocale(LC_CTYPE, "C.UTF-8");
    if (!result)
      result = setlocale(LC_CTYPE, "UTF-8");

    if (result)
      motStrResetCharset();
  }
}

void iupmotStrSetUTF8ModeFile(int utf8mode)
{
  iupmot_utf8mode_file = utf8mode;
}

int iupmotStrGetUTF8Mode(void)
{
  return iupmot_utf8mode;
}

int iupmotStrGetUTF8ModeFile(void)
{
  return iupmot_utf8mode_file;
}

void iupmotStrRelease(void)
{
  if (motLastConvertUTF8)
  {
    free(motLastConvertUTF8);
    motLastConvertUTF8 = NULL;
  }
}

char* iupmotStrConvertToSystem(const char* str)
{
  const char* charset = NULL;

  if (!str || *str == 0)
    return (char*)str;

  if (!iupmot_utf8mode)
    return (char*)str;

  if (motStrCheckUTF8Support())
    return (char*)str;

  if (motStrIsLocaleUTF8())
    return (char*)str;

  if (iupStrIsAscii(str))
    return (char*)str;

  charset = motStrGetCurrentCharset();
  if (!charset)
    charset = "ISO-8859-1";

#ifdef IUP_USE_ICONV
  if (motLastConvertUTF8)
    free(motLastConvertUTF8);

  motLastConvertUTF8 = motStrFromUTF8(str, charset);
  if (!motLastConvertUTF8)
    return (char*)str;

  return motLastConvertUTF8;
#else
  return (char*)str;
#endif
}

char* iupmotStrConvertFromSystem(const char* str)
{
  const char* charset = NULL;

  if (!str || *str == 0)
    return (char*)str;

  if (!iupmot_utf8mode)
    return (char*)str;

  if (motStrCheckUTF8Support())
    return (char*)str;

  if (motStrIsLocaleUTF8())
    return (char*)str;

  if (iupStrIsAscii(str))
    return (char*)str;

  charset = motStrGetCurrentCharset();
  if (!charset)
    charset = "ISO-8859-1";

#ifdef IUP_USE_ICONV
  if (motLastConvertUTF8)
    free(motLastConvertUTF8);

  motLastConvertUTF8 = motStrToUTF8(str, strlen(str), charset);
  if (!motLastConvertUTF8)
    return (char*)str;

  return motLastConvertUTF8;
#else
  return (char*)str;
#endif
}

char* iupmotStrConvertToFilename(const char* str)
{
  int old_mode = iupmot_utf8mode;
  char* result;

  if (iupmot_utf8mode)
    iupmot_utf8mode = iupmot_utf8mode_file;

  result = iupmotStrConvertToSystem(str);

  iupmot_utf8mode = old_mode;
  return result;
}

char* iupmotStrConvertFromFilename(const char* str)
{
  int old_mode = iupmot_utf8mode;
  char* result;

  if (iupmot_utf8mode)
    iupmot_utf8mode = iupmot_utf8mode_file;

  result = iupmotStrConvertFromSystem(str);

  iupmot_utf8mode = old_mode;
  return result;
}

void iupmotSetMnemonicTitle(Ihandle *ih, Widget w, int pos, const char* value)
{
  char c;
  char* str;

  if (!value) 
    value = "";

  if (!w)
    w = ih->handle;

  str = iupStrProcessMnemonic(value, &c, -1);  /* remove & and return in c */
  if (str != value)
  {
    KeySym keysym = iupmotKeyCharToKeySym(c);
    XtVaSetValues(w, XmNmnemonic, keysym, NULL);   /* works only for menus, but underlines the letter */

    if (ih->iclass->nativetype != IUP_TYPEMENU)
      iupKeySetMnemonic(ih, c, pos);

    iupmotSetXmString(w, XmNlabelString, str);
    free(str);
  }
  else
  {
    XtVaSetValues (w, XmNmnemonic, NULL, NULL);
    iupmotSetXmString(w, XmNlabelString, str);
  }
}

void iupmotSetXmString(Widget w, const char *resource, const char* value)
{
  XmString xm_str = iupmotStringCreate(value);
  XtVaSetValues(w, resource, xm_str, NULL);
  XmStringFree(xm_str);
}

char* iupmotGetXmString(XmString str)
{
  char* text;

  if (iupmot_utf8mode && motStrCheckUTF8Support())
  {
    text = (char*)XmStringUnparse(str, "UTF-8", XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, XmOUTPUT_ALL);
  }
  else
  {
    text = (char*)XmStringUnparse(str, NULL, XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, XmOUTPUT_ALL);
    text = iupmotStrConvertFromSystem(text);
  }

  return text;
}

char* iupmotReturnXmString(XmString str)
{
  char* text;
  char* converted;
  char* ret;

  if (iupmot_utf8mode && motStrCheckUTF8Support())
  {
    text = (char*)XmStringUnparse(str, "UTF-8", XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, XmOUTPUT_ALL);
    ret = iupStrReturnStr(text);
    XtFree(text);
  }
  else
  {
    text = (char*)XmStringUnparse(str, NULL, XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, XmOUTPUT_ALL);
    converted = iupmotStrConvertFromSystem(text);

    if (converted != text)
    {
      ret = iupStrReturnStr(converted);
      XtFree(text);
    }
    else
    {
      ret = iupStrReturnStr(text);
      XtFree(text);
    }
  }

  return ret;
}

XmString iupmotStringCreate(const char *value)
{
  int use_utf8 = 0;

  if (iupmot_utf8mode)
    use_utf8 = 1;
#ifdef IUP_USE_XFT
  else if (motStrIsLocaleUTF8())
    use_utf8 = 1;
#endif

  if (use_utf8 && motStrCheckUTF8Support())
  {
    return XmStringGenerate((XtPointer)value, "UTF-8", XmCHARSET_TEXT, NULL);
  }
  else
  {
    value = iupmotStrConvertToSystem(value);
    return XmStringCreateLocalized((String)value);
  }
}

void iupmotSetTitle(Widget w, const char *value)
{
  XtVaSetValues(w, XmNtitle, value, 
                   XmNiconName, value, 
                   NULL);
}

void iupmotTextSetString(Widget w, const char *value)
{
  value = iupmotStrConvertToSystem(value);
  XmTextSetString(w, (char*)value);
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

#ifdef IUP_USE_ICONV
  {
    size_t ulen = (size_t)len;
    size_t mlen = ulen * 2;
    iconv_t cd_iconv = iconv_open("UTF-8", "ISO-8859-1");

    if (cd_iconv == (iconv_t)-1)
      return iupStrCopyToUtf8Buffer(str, len, utf8_buffer, utf8_buffer_max);

    utf8_buffer = iupCheckUtf8Buffer(utf8_buffer, utf8_buffer_max, mlen);

    iconv(cd_iconv, (char**)&str, &ulen, &utf8_buffer, &mlen);
    utf8_buffer[mlen] = 0;

    iconv_close(cd_iconv);
  }
#else
  return iupStrCopyToUtf8Buffer(str, len, utf8_buffer, utf8_buffer_max);
#endif

  return utf8_buffer;
}

