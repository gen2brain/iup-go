/** \file
 * \brief Ihandle management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>  
#include <stdlib.h>  
#include <memory.h>  

#include "iup.h"

#include "iup_object.h"
#include "iup_assert.h"
#include "iup_register.h"
#include "iup_names.h"
#include "iup_varg.h"
#include "iup_focus.h"
#include "iup_attrib.h"


static Ihandle* iHandleCreate(void)
{
  Ihandle *ih = (Ihandle*)malloc(sizeof(Ihandle));
  memset(ih, 0, sizeof(Ihandle));

  ih->sig[0] = 'I';
  ih->sig[1] = 'U';
  ih->sig[2] = 'P';
  ih->sig[3] = 0;

  ih->serial = -1;

  ih->attrib = iupTableCreate(IUPTABLE_STRINGINDEXED);

  return ih;
}

static void iHandleDestroy(Ihandle* ih)
{
  iupTableDestroy(ih->attrib);
  memset(ih, 0, sizeof(Ihandle));
  free(ih);
}

IUP_SDK_API int iupObjectCheck(Ihandle* ih)
{
  char* sig = (char*)ih;

  if (!ih) return 0;  

  if (sig[0] != 'I' ||
      sig[1] != 'U' ||
      sig[2] != 'P' ||
      sig[3] != 0)
    return 0;

  return 1;
}

IUP_SDK_API Ihandle* iupObjectCreate(Iclass* iclass, void** params)
{
  char* name;

  /* create the base handle structure */
  Ihandle* ih = iHandleCreate();

  ih->iclass = iclass;

  /* create the element */
  if (iupClassObjectCreate(ih, params) == IUP_ERROR)
  {
    iupERROR1("IUP object creation failed (%s).", iclass->name);
    iHandleDestroy(ih);
    return NULL;
  }

  /* ensure attributes default values, at this time only the ones that can be set before map */
  iupClassObjectEnsureDefaultAttributes(ih);

  name = IupGetGlobal("DEFAULTTHEME");
  if (name)
  {
    Ihandle* theme = IupGetHandle(name);
    if (theme)
      iupAttribSetTheme(ih, theme);
  }

  return ih;
}

IUP_SDK_API void** iupObjectGetParamList(void* first, va_list arglist)
{
  const int INITIAL_NUMBER = 50;
  void **params;
  void *param;
  int max_count = INITIAL_NUMBER, count = 0;

  params = (void **) malloc (sizeof (void *) * INITIAL_NUMBER);

  param = first;

  while (param != NULL)
  {
    params[count] = param;
    count++;

    /* check if needs to allocate memory */
    if (count >= max_count)
    {
      void **new_params = NULL;

      max_count += INITIAL_NUMBER;

      new_params = (void **) realloc (params, sizeof (void *) * max_count);

      params = new_params;
    }

    param = va_arg (arglist, void*);
  }
  params[count] = NULL;

  return params;
}

IUP_API Ihandle* IupCreatev(const char *name, void **params)
{
  Iclass *ic;
  iupASSERT(name!=NULL);
  ic = iupRegisterFindClass(name);
  if (ic)
    return iupObjectCreate(ic, params);
  else
    return NULL;
}

IUP_API Ihandle *IupCreateV(const char *name, void* first, va_list arglist)
{
  void **params;
  Ihandle *ih;

  iupASSERT(name != NULL);

  params = iupObjectGetParamList(first, arglist);
  ih = IupCreatev(name, params);
  free(params);

  return ih;
}

IUP_API Ihandle *IupCreatep(const char *name, void* first, ...)
{
  va_list arglist;
  Ihandle *ih;

  iupASSERT(name!=NULL);

  va_start(arglist, first);
  ih = IupCreateV(name, first, arglist);
  va_end(arglist);

  return ih;
}

IUP_API Ihandle* IupCreate(const char *name)
{
  iupASSERT(name!=NULL);
  return IupCreatev(name, NULL);
}

IUP_API void IupDestroy(Ihandle *ih)
{
  Icallback cb;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  /* Hide before destroy to avoid children redraw */
  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
    IupHide(ih);

  cb = IupGetCallback(ih, "DESTROY_CB");
  if (cb) cb(ih);

  /* Destroy all its children.
     Just need to remove the first child,
     IupDetach will update firstchild. */
  while (ih->firstchild)
    IupDestroy(ih->firstchild);

  /* unmap if mapped and remove from its parent child list */
  IupDetach(ih);

  cb = IupGetCallback(ih, "LDESTROY_CB");  /* for language bindings */
  if (cb) cb(ih);

  /* check if the element had the focus */
  iupResetCurrentFocus(ih);

  /* removes names associated with the element */
  iupRemoveNames(ih);

  /* destroy the element */
  iupClassObjectDestroy(ih);

  /* destroy the private data */
  if (ih->data)
    free(ih->data);

  /* destroy the base handle structure */
  iHandleDestroy(ih);
}
