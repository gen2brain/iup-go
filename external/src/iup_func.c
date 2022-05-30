/** \file
 * \brief function table manager
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h> 

#include "iup.h"

#include "iup_str.h"
#include "iup_table.h"
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

int iupGetFunctions(char** names, int n)
{
  int count = iupTableCount(ifunc_table);
  char * name;
  int i = 0;

  if (n == 0 || n == -1)
    return count;

  name = iupTableFirst(ifunc_table);
  while (name)
  {
    if (!iupATTRIB_ISINTERNAL(name))
    {
      names[i] = name;
      i++;
      if (i == n)
        break;
    }

    name = iupTableNext(ifunc_table);
  }

  return i;
}
