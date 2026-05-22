/** \file
 * \brief function table manager
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_hashtable.h"
#include "iup_func.h"
#include "iup_drv.h"
#include "iup_assert.h"
#include "iup_attrib.h"


static Itable *ifunc_table = NULL;   /* the function hash table indexed by the name string */


void iupFuncInit(void)
{
  ifunc_table = iupTableCreate(IUPTABLE_STRINGINDEXED);
}

void iupFuncFinish(void)
{
  iupTableDestroy(ifunc_table);
  ifunc_table = NULL;
}

IUP_API Icallback IupGetFunction(const char *name)
{
  void* value;

  iupASSERT(name != NULL);
  if (!name)
    return NULL;

  return (Icallback)iupTableGetFunc(ifunc_table, name, &value);
}

IUP_API Icallback IupSetFunction(const char *name, Icallback func)
{
  void* value;
  Icallback old_func;

  iupASSERT(name != NULL);
  if (!name)
    return NULL;

  old_func = (Icallback)iupTableGetFunc(ifunc_table, name, &value);

  if (!func)
    iupTableRemove(ifunc_table, name);
  else
    iupTableSetFunc(ifunc_table, name, (Ifunc)func);

  /* notifies the driver if changing the Idle */
  if (iupStrEqual(name, "IDLE_ACTION"))
    iupdrvSetIdleFunction(func);

  return old_func;
}

/* Well-known names IUP itself dispatches through the function table. */
static const char* known_functions[] = {
  "ENTRY_POINT",
  "IDLE_ACTION",
  "GLOBALKEYPRESS_CB",
  "GLOBALBUTTON_CB",
  "GLOBALMOTION_CB",
  "GLOBALWHEEL_CB"
};
#define KNOWN_FUNCTIONS_COUNT ((int)(sizeof(known_functions)/sizeof(known_functions[0])))

static int iFuncIsKnown(const char* name)
{
  int i;
  for (i = 0; i < KNOWN_FUNCTIONS_COUNT; i++)
    if (iupStrEqual(name, known_functions[i]))
      return 1;
  return 0;
}

IUP_API int IupGetAllFunctions(char** names, int n)
{
  int total = KNOWN_FUNCTIONS_COUNT;
  char* name;
  int written;

  name = iupTableFirst(ifunc_table);
  while (name)
  {
    if (!iupATTRIB_ISINTERNAL(name) && !iFuncIsKnown(name))
      total++;
    name = iupTableNext(ifunc_table);
  }

  if (!names || n == 0 || n == -1)
    return total;

  written = 0;
  while (written < KNOWN_FUNCTIONS_COUNT && written < n)
  {
    names[written] = (char*)known_functions[written];
    written++;
  }
  name = iupTableFirst(ifunc_table);
  while (name && written < n)
  {
    if (!iupATTRIB_ISINTERNAL(name) && !iFuncIsKnown(name))
    {
      names[written] = name;
      written++;
    }
    name = iupTableNext(ifunc_table);
  }
  return written;
}

int iupGetFunctions(char** names, int n)
{
  return IupGetAllFunctions(names, n);
}
