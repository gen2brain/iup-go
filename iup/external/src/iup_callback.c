/** \file
 * \brief get/set callback
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h> 
#include <stdarg.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_assert.h"
#include "iup_varg.h"

  
char* iupGetCallbackName(Ihandle *ih, const char *name)
{
  void* value;
  Icallback func = (Icallback)iupTableGetFunc(ih->attrib, name, &value);

  if (!func && value)
  {
    /* if not a IUPTABLE_FUNCPOINTER then it is an old fashion name */
    return value;
  }

  return NULL;
}

IUP_API Icallback IupGetCallback(Ihandle *ih, const char *name)
{
  Icallback func = NULL;
  void* value;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  iupASSERT(name!=NULL);
  if (!name) 
    return NULL;

  func = (Icallback)iupTableGetFunc(ih->attrib, name, &value);

  if (!func && value)
  {
    /* if not a IUPTABLE_FUNCPOINTER then it is an old fashion name */
    func = IupGetFunction((const char*)value);
  }

  return func;
}

IUP_API Icallback IupSetCallback(Ihandle *ih, const char *name, Icallback func)
{
  Icallback old_func = NULL;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  iupASSERT(name!=NULL);
  if (!name) 
    return NULL;

  if (!func)
    iupTableRemove(ih->attrib, name);
  else
  {
    void* value;
    old_func = (Icallback)iupTableGetFunc(ih->attrib, name, &value);
    if (!old_func && value)
      old_func = IupGetFunction((const char*)value);

    iupTableSetFunc(ih->attrib, name, (Ifunc)func);
  }

  return old_func;
}

IUP_API Ihandle* IupSetCallbacksV(Ihandle* ih, const char *name, Icallback func, va_list arglist)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  IupSetCallback(ih, name, func);

  name = va_arg(arglist, const char*);
  while (name)
  {
    func = va_arg(arglist, Icallback);
    IupSetCallback(ih, name, func);

    name = va_arg(arglist, const char*);
  }

  return ih;
}

IUP_API Ihandle* IupSetCallbacks(Ihandle* ih, const char *name, Icallback func, ...)
{
  va_list arglist;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  va_start(arglist, func);
  IupSetCallbacksV(ih, name, func, arglist);
  va_end (arglist);
  return ih;
}
