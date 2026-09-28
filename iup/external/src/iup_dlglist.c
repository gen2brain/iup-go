/** \file
 * \brief list of all created dialogs
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>

#include "iup.h"

#include "iup_dlglist.h"
#include "iup_object.h"
#include "iup_assert.h"
#include "iup_attrib.h"


typedef struct Idiallst_
{
  Ihandle* ih;
  struct Idiallst_* next;
} Idiallst;

static Idiallst* idlglist = NULL;  /* list of all created dialogs */
static int idlg_count = 0;

IUP_SDK_API void iupDlgListAdd(Ihandle* ih)
{
  if (ih)
  {
    Idiallst* p=(Idiallst*)malloc(sizeof(Idiallst));
    if (!p)
      return;
    p->ih = ih;
    p->next = idlglist;
    idlglist = p;
    idlg_count++;
  }
}

IUP_SDK_API void iupDlgListRemove(Ihandle* ih)
{
  if (!idlglist || !ih)
    return;

  if (idlglist->ih == ih)    /* ih is header */
  {
    Idiallst* p = idlglist->next;
    free(idlglist);
    idlglist = p;
    idlg_count--;
  }
  else
  {
    Idiallst* p;    /* current pointer */
    Idiallst* b;    /* before pointer */
    for (b = idlglist, p = idlglist->next; p; b = p, p = p->next)
    {
      if (p->ih == ih)
      {
        b->next = p->next;
        free (p);
        idlg_count--;
        return;
      }
    }
  }
}

static Idiallst* idlg_first = NULL;

IUP_SDK_API int iupDlgListCount(void)
{
  return idlg_count;
}

IUP_SDK_API Ihandle* iupDlgListFirst(void)
{
  idlg_first = idlglist;
  return iupDlgListNext();
}

IUP_SDK_API Ihandle* iupDlgListNext(void)
{
  Ihandle* ih = NULL;
  if (idlg_first)
  {
    ih = idlg_first->ih;
    idlg_first = idlg_first->next;
  }
  return ih;
}

static int idlg_nvisiblewin = 0;

IUP_SDK_API void iupDlgListVisibleInc(void)
{
  iupASSERT(idlg_nvisiblewin < idlg_count);
  if (idlg_nvisiblewin == idlg_count)
    return;
  idlg_nvisiblewin++;
}

IUP_SDK_API void iupDlgListVisibleDec(void)
{
  iupASSERT(idlg_nvisiblewin > 0);
  idlg_nvisiblewin--;
}

IUP_SDK_API int iupDlgListVisibleCount(void)
{
  return idlg_nvisiblewin;
}

static Ihandle* iDlgListNextToDestroy(const char* name, void* value)
{
  Idiallst* list;
  for (list = idlglist; list; list = list->next)
  {
    Ihandle* ih = list->ih;
    if (!iupObjectCheck(ih) || iupAttribGet(ih, "_IUP_DLGLIST_DESTROY"))
      continue;
    if (name && !((value && iupAttribGet(ih, name) == value) || (!value && iupAttribGet(ih, name))))
      continue;
    return ih;
  }
  return NULL;
}

static void iDlgListDestroy(const char* name, void* value)
{
  Ihandle* ih;
  while ((ih = iDlgListNextToDestroy(name, value)) != NULL)
  {
    iupAttribSet(ih, "_IUP_DLGLIST_DESTROY", "1");
    IupDestroy(ih);   /* this also removes it, and any dialog it owns, from the list */
  }
}

void iupDlgListDestroyAll(void)
{
  iDlgListDestroy(NULL, NULL);
}

IUP_SDK_API void iupDlgListDestroySelected(const char* name, void* value)
{
  iDlgListDestroy(name, value);
}
