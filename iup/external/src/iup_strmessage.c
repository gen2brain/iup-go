/** \file
 * \brief String Utilities
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"
#include "iup_strmessage.h"
#include "iup_hashtable.h"
#include "iup_register.h"


static Itable *istrmessage_table = NULL;   /* the message hash table indexed by the name string */

void iupStrMessageInit(void)
{
  istrmessage_table = iupTableCreate(IUPTABLE_STRINGINDEXED);
}

void iupStrMessageFinish(void)
{
  iupTableDestroy(istrmessage_table);
  istrmessage_table = NULL;
}

IUP_API char* IupGetLanguageString(const char* name)
{
  char* value;
  if (!name) return NULL;
  value = (char*)iupTableGet(istrmessage_table, name);
  if (!value)
    return (char*)name;
  return value;
}

IUP_API void IupSetLanguageString(const char* name, const char* str)
{
  iupTableSet(istrmessage_table, name, (char*)str, IUPTABLE_POINTER);
}

IUP_API void IupStoreLanguageString(const char* name, const char* str)
{
  iupTableSet(istrmessage_table, name, (char*)str, IUPTABLE_STRING);
}

IUP_API void IupSetLanguagePack(Ihandle* ih)
{
  if (!ih)
    iupTableClear(istrmessage_table);
  else
  {
    char *name, *value;

    name = iupTableFirst(ih->attrib);
    while (name)
    {
      value = (char*)iupTableGetCurr(ih->attrib);

      if (iupTableGetCurrType(ih->attrib)==IUPTABLE_STRING)
        iupTableSet(istrmessage_table, name, value, IUPTABLE_STRING);
      else
        iupTableSet(istrmessage_table, name, value, IUPTABLE_POINTER);

      name = iupTableNext(ih->attrib);
    }
  }
}

IUP_API void IupSetLanguage(const char *language)
{
  IupStoreGlobal("LANGUAGE", language);
}

IUP_API char *IupGetLanguage(void)
{
  return IupGetGlobal("LANGUAGE");
}

/**********************************************************************************/

static void iupSetLngAttV(const char* first, va_list arglist)
{
  const char *name, *str;
  name = first;
  while (name)
  {
    str = va_arg(arglist, const char*);

    IupSetLanguageString(name, str);

    name = va_arg(arglist, const char*);
  }
}

static void iupSetLngAtt(const char* first, ...)
{
  va_list arglist;
  va_start(arglist, first);
  iupSetLngAttV(first, arglist);
  va_end(arglist);
}

#include "iup_lng_english.h"
#include "iup_lng_portuguese.h"
#include "iup_lng_spanish.h"
#include "iup_lng_czech.h"
#include "iup_lng_russian.h"
#include "iup_lng_german.h"
#include "iup_lng_french.h"
#include "iup_lng_chinese.h"
#include "iup_lng_japanese.h"
#include "iup_lng_italian.h"

static void iStrMessageRegisterInternal(const char* language)
{
  Ihandle* lng = NULL;

  if (iupStrEqualNoCase(language, "ENGLISH"))
  {
    lng = iup_load_lng_english();
  }
  else if (iupStrEqualNoCase(language, "PORTUGUESE"))
  {
    lng = iup_load_lng_portuguese();
  }
  else if (iupStrEqualNoCase(language, "SPANISH"))
  {
    lng = iup_load_lng_spanish();
  }
  else if (iupStrEqualNoCase(language, "CZECH"))
  {
    lng = iup_load_lng_czech();
  }
  else if (iupStrEqualNoCase(language, "RUSSIAN"))
  {
    lng = iup_load_lng_russian();
  }
  else if (iupStrEqualNoCase(language, "GERMAN"))
  {
    lng = iup_load_lng_german();
  }
  else if (iupStrEqualNoCase(language, "FRENCH"))
  {
    lng = iup_load_lng_french();
  }
  else if (iupStrEqualNoCase(language, "CHINESE"))
  {
    lng = iup_load_lng_chinese();
  }
  else if (iupStrEqualNoCase(language, "JAPANESE"))
  {
    lng = iup_load_lng_japanese();
  }
  else if (iupStrEqualNoCase(language, "ITALIAN"))
  {
    lng = iup_load_lng_italian();
  }

  if (lng)
  {
    IupSetLanguagePack(lng);
    IupDestroy(lng);
  }
}

void iupStrMessageUpdateLanguage(const char* language)
{
  /* called after the global attribute is changed */

  iStrMessageRegisterInternal(language);

  iupRegisterUpdateClasses();
}
