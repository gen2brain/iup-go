/** \file
 * \brief Motif Base Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Xm/Xm.h>
#include <Xm/Text.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"

#include "iupmot_drv.h"


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
  return (char*)XmStringUnparse(str, NULL, XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, XmOUTPUT_ALL);
}

char* iupmotReturnXmString(XmString str)
{
  char* text = iupmotGetXmString(str);
  char* ret = iupStrReturnStr(text);
  XtFree(text);
  return ret;
}

/* TODO: UTF8 support would start here... */

XmString iupmotStringCreate(const char *value)
{
  return XmStringCreateLocalized((String)value);
}

void iupmotSetTitle(Widget w, const char *value)
{
  XtVaSetValues(w, XmNtitle, value, 
                   XmNiconName, value, 
                   NULL);
}

void iupmotTextSetString(Widget w, const char *value)
{
  XmTextSetString(w, (char*)value);
}

#ifdef USE_ICONV
#include <iconv.h>
#endif

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

#ifdef USE_ICONV
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

