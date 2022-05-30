/** \file
 * \brief String Utilities
 *
 * See Copyright Notice in "iup.h"
 */

 
#include <string.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <limits.h>  
#include <stdarg.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_strmessage.h"
#include "iup_table.h"
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
#include "iup_lng_portuguese_utf8.h"
#include "iup_lng_spanish.h"
#include "iup_lng_spanish_utf8.h"
#ifdef IUP_CZECH
#include "iup_lng_czech_utf8.h"
#endif
#ifdef IUP_RUSSIAN
#include "iup_lng_russian_utf8.h"
#endif

static void iStrMessageRegisterInternal(const char* language)
{
  Ihandle* lng = NULL;

  if (iupStrEqualNoCase(language, "ENGLISH"))
  {
    lng = iup_load_lng_english();
  }
  else if (iupStrEqualNoCase(language, "PORTUGUESE"))
  {
    if (IupGetInt(NULL, "UTF8MODE"))
      lng = iup_load_lng_portuguese_utf8();
    else
      lng = iup_load_lng_portuguese();
  }
  else if (iupStrEqualNoCase(language, "SPANISH"))
  {
    if (IupGetInt(NULL, "UTF8MODE"))
      lng = iup_load_lng_spanish_utf8();
    else
      lng = iup_load_lng_spanish();
  }
  /* To add a custom language */
#ifdef IUP_CZECH
  else if (iupStrEqualNoCase(language, "CZECH"))
  {
    if (IupGetInt(NULL, "UTF8MODE"))
      lng = iup_load_lng_czech_utf8();
  }
#endif
#ifdef IUP_RUSSIAN
  else if (iupStrEqualNoCase(language, "RUSSIAN"))
  {
    if (IupGetInt(NULL, "UTF8MODE"))
      lng = iup_load_lng_russian_utf8();
  }
#endif

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

#if 0
void iupSaveLanguagePack(const char* filename)
{
  char *name, *value;
  int utf8mode = IupGetInt(NULL, "UTF8MODE");
  char* lng = IupGetLanguage();

  FILE* file = fopen(filename, "wb");

  fprintf(file, "%s%s = user[\n", lng, utf8mode? "_UTF8": "");

  name = iupTableFirst(istrmessage_table);
  while (name)
  {
    value = (char*)iupTableGetCurr(istrmessage_table);

    fprintf(file, "  %s = \"%s\",\n", name, value);

    name = iupTableNext(istrmessage_table);
  }

  fprintf(file, "NULL = NULL\n");
  fprintf(file, "]()\n");

  fclose(file);
}
#endif
