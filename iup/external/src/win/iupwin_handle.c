/** \file
 * \brief HWND to Ihandle* table
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  

#include <windows.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_hashtable.h"
#include "iupcbs.h" 

#include "iupwin_handle.h"  


static Itable* winhandle_table; /* table indexed by HWND containing Ihandle* address */

IUP_DRV_API Ihandle* iupwinHandleGet(InativeHandle* handle)
{
  Ihandle* ih;
  if (!handle)
    return NULL;
  ih = (Ihandle*)iupTableGet(winhandle_table, (char*)handle);
  if (ih && !iupObjectCheck(ih))
    return NULL;
  return ih;
}

IUP_DRV_API void iupwinHandleAdd(Ihandle *ih, InativeHandle* handle)
{
  IFvs cb;

  iupTableSet(winhandle_table, (char*)handle, ih, IUPTABLE_POINTER);

  cb = (IFvs)IupGetFunction("HANDLEADD_CB");
  if (cb)
  {
    char name[1024];
    GetClassNameA((HWND)handle, name, 1024);
    cb(handle, name);
  }
}

IUP_DRV_API void iupwinHandleRemove(InativeHandle* handle)
{
  IFvs cb;

  iupTableRemove(winhandle_table, (char*)handle);

  cb = (IFvs)IupGetFunction("HANDLEREMOVE_CB");
  if (cb)
  {
    char name[1024];
    GetClassNameA((HWND)handle, name, 1024);
    cb(handle, name);
  }
}

IUP_DRV_API void iupwinHandleInit(void)
{
  winhandle_table = iupTableCreate(IUPTABLE_POINTERINDEXED);
}

IUP_DRV_API void iupwinHandleFinish(void)
{
  iupTableDestroy(winhandle_table);
  winhandle_table = NULL;
}
