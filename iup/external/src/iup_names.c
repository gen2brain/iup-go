/** \file
 * \brief Ihandle <-> Name table manager.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <memory.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_table.h"
#include "iup_names.h"
#include "iup_object.h"
#include "iup_class.h"
#include "iup_assert.h"
#include "iup_attrib.h"
#include "iup_str.h"


/* An Ihandle* may have many different handle names. 
   Do not confuse with the NAME attribute. */

static Itable *inames_strtable = NULL;   /* table indexed by name containing Ihandle* address */

void iupNamesInit(void)
{
  inames_strtable = iupTableCreate(IUPTABLE_STRINGINDEXED);
}

void iupNamesFinish(void)
{
  iupTableDestroy(inames_strtable);
  inames_strtable = NULL;
}

static Ihandle* iNameGetTopParent(Ihandle* ih)
{
  Ihandle* parent = ih;
  while (parent->parent)
    parent = parent->parent;
  return parent;
}

static int iNameCheckArray(Ihandle** ih_array, int count, Ihandle* ih)
{
  int i;
  for (i = 0; i < count; i++)
  {
    if (ih_array[i] == ih)
      return 0;
  }
  return 1;
}

void iupNamesDestroyHandles(void)
{
  char *name;
  Ihandle** ih_array, *ih;
  int count, i = 0;

  count = iupTableCount(inames_strtable);
  if (!count)
    return;

  ih_array = (Ihandle**)malloc(count * sizeof(Ihandle*));
  memset(ih_array, 0, count * sizeof(Ihandle*));

  /* store the handles before updating so we can remove elements in the loop */
  name = iupTableFirst(inames_strtable);
  while (name)
  {
    ih = (Ihandle*)iupTableGetCurr(inames_strtable);
    if (iupObjectCheck(ih))   /* here must be a handle */
    {
      /* only need to destroy the top parent handle */
      ih = iNameGetTopParent(ih);

      /* check if already in the array */
      if (iNameCheckArray(ih_array, i, ih))
      {
        ih_array[i] = ih;
        i++;
      }
    }
    name = iupTableNext(inames_strtable);
  }

  count = i;
  for (i = 0; i < count; i++)
  {
    if (iupObjectCheck(ih_array[i]))  /* here must be a handle */
      IupDestroy(ih_array[i]);
  }

  free(ih_array);
}

void iupRemoveNames(Ihandle* ih)
{
  /* called from IupDestroy */
  char *name;

  /* ih here is an Ihandle* */

  /* check for at least one name (the last one set) */
  name = iupAttribGetHandleName(ih);
  if (name)
    iupTableRemove(inames_strtable, name);

  /* clear also the NAME attribute */
  iupBaseSetNameAttrib(ih, NULL);

  /* Do NOT search for other names, this would implying in checking in all store names.
     So, some names may have left invalid on the handle names database. */
}

IUP_API Ihandle *IupGetHandle(const char *name)
{
  if (!name) /* no iupASSERT needed here */
    return NULL;
  return (Ihandle*)iupTableGet (inames_strtable, name);
}

int iupNamesFindAll(Ihandle *ih, char** names, int n)
{
  int i = 0;
  char* name = iupTableFirst(inames_strtable);
  while (name)
  {
    if ((Ihandle*)iupTableGetCurr(inames_strtable) == ih)
    {
      if (names)
        names[i] = name;

      i++;
      if (i == n && n != 0 && n != -1)
        break;
    }

    name = iupTableNext(inames_strtable);
  }

  return i;
}

static char* iNameFindHandle(Ihandle *ih)
{
  /* search for a name */
  char* name = iupTableFirst(inames_strtable);
  while (name)
  {
    /* return the first one found */
    if ((Ihandle*)iupTableGetCurr(inames_strtable) == ih)
      return name;

    name = iupTableNext(inames_strtable);
  }
  return NULL;
}

IUP_API Ihandle* IupSetHandle(const char *name, Ihandle *ih)
{
  Ihandle *old_ih;

  iupASSERT(name!=NULL);
  if (!name)
    return NULL;

  /* ih here can be also an user pointer, not just an Ihandle* */

  /* we do not check if the handle already has names, it may has many different names */

  old_ih = iupTableGet(inames_strtable, name);

  if (ih != NULL)
  {
    iupTableSet(inames_strtable, name, ih, IUPTABLE_POINTER);

    /* save the name in the cache if it is a valid handle */
    if (iupObjectCheck(ih))
      iupAttribSetStr(ih, "HANDLENAME", name);
  }
  else
  {
    iupTableRemove(inames_strtable, name);

    /* clear the name from the cache if it is a valid handle */
    if (iupObjectCheck(old_ih))
    {
      char* last_name = iupAttribGet(old_ih, "HANDLENAME");
      if (last_name && iupStrEqual(last_name, name))
      {
        iupAttribSet(old_ih, "HANDLENAME", NULL);  /* remove also from the cache */

        last_name = iNameFindHandle(old_ih);
        if (last_name)
          iupAttribSetStr(old_ih, "HANDLENAME", last_name);  /* if found another name save it in the cache */
      }
    }
  }

  return old_ih;
}

IUP_API char* IupGetName(Ihandle* ih)
{
  if (!ih) /* no iupASSERT needed here */
    return NULL;

  /* ih here can be also an user pointer, not just an Ihandle* */

  if (iupObjectCheck(ih)) /* if ih is an Ihandle* */
  {
    /* check for a name */
    return iupAttribGetHandleName(ih);
  }

  /* search for a name */
  return iNameFindHandle(ih);
}

IUP_API int IupGetAllNames(char** names, int n)
{
  int i = 0;
  char* name;

  if (!names || n == 0 || n == -1)
    return iupTableCount(inames_strtable);

  name = iupTableFirst(inames_strtable);
  while (name)
  {
    names[i] = name;
    i++;
    if (i == n)
      break;

    name = iupTableNext(inames_strtable);
  }
  return i;
}

static int iNamesCountDialogs(void)
{
  int i = 0;
  char* name = iupTableFirst(inames_strtable);
  while (name)
  {
    Ihandle* dlg = (Ihandle*)iupTableGetCurr(inames_strtable);
    if (iupObjectCheck(dlg) &&  /* here must be a handle */
        dlg->iclass->nativetype == IUP_TYPEDIALOG)
      i++;

    name = iupTableNext(inames_strtable);
  }
  return i;
}

IUP_API int IupGetAllDialogs(char** names, int n)
{
  int i = 0;
  char* name;

  if (!names || n==0 || n==-1)
    return iNamesCountDialogs();

  name = iupTableFirst(inames_strtable);
  while (name)
  {
    Ihandle* dlg = (Ihandle*)iupTableGetCurr(inames_strtable);
    if (iupObjectCheck(dlg) &&  /* here must be a handle */
        dlg->iclass->nativetype == IUP_TYPEDIALOG)
    {
      names[i] = name;
      i++;
      if (i == n)
        break;
    }

    name = iupTableNext(inames_strtable);
  }
  return i;
}

