/** \file
 * \brief IUP Ihandle Class C Interface
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_assert.h"



/*****************************************************************
                        Method Stubs
*****************************************************************/

static int iClassCreate(Iclass* ic, Ihandle* ih, void** params)
{
  int ret = IUP_NOERROR;
  if (ic->parent)
    ret = iClassCreate(ic->parent, ih, params);

  if (ret == IUP_NOERROR && ic->Create)
    ret = ic->Create(ih, params);

  return ret;
}

static int iClassMap(Iclass* ic, Ihandle* ih)
{
  int ret = IUP_NOERROR;
  if (ic->parent)
    ret = iClassMap(ic->parent, ih);

  if (ret == IUP_NOERROR && ic->Map)
    ret = ic->Map(ih);

  return ret;
}

static void iClassUnMap(Iclass* ic, Ihandle* ih)
{
  /* must be before the parent class */
  if (ic->UnMap)
    ic->UnMap(ih);

  if (ic->parent)
    iClassUnMap(ic->parent, ih);
}

static void iClassDestroy(Iclass* ic, Ihandle* ih)
{
  /* must destroy child class before the parent class */
  if (ic->Destroy)
    ic->Destroy(ih);

  if (ic->parent)
    iClassDestroy(ic->parent, ih);
}

static void iClassComputeNaturalSize(Iclass* ic, Ihandle* ih, int *w, int *h, int *children_expand)
{
  if (ic->parent)
    iClassComputeNaturalSize(ic->parent, ih, w, h, children_expand);

  if (ic->ComputeNaturalSize)
    ic->ComputeNaturalSize(ih, w, h, children_expand);
}

static void iClassSetChildrenCurrentSize(Iclass* ic, Ihandle* ih, int shrink)
{
  if (ic->parent)
    iClassSetChildrenCurrentSize(ic->parent, ih, shrink);

  if (ic->SetChildrenCurrentSize)
    ic->SetChildrenCurrentSize(ih, shrink);
}

static void iClassSetChildrenPosition(Iclass* ic, Ihandle* ih, int x, int y)
{
  if (ic->parent)
    iClassSetChildrenPosition(ic->parent, ih, x, y);

  if (ic->SetChildrenPosition)
    ic->SetChildrenPosition(ih, x, y);
}

static void* iClassGetInnerNativeContainerHandle(Iclass* ic, Ihandle* ih, Ihandle* child)
{
  void* container_handle = ih->handle;

  if (ic->parent)
    container_handle = iClassGetInnerNativeContainerHandle(ic->parent, ih, child);

  /* if the class implements the function it will ignore the result of the parent class */

  if (ic->GetInnerNativeContainerHandle)
    container_handle = ic->GetInnerNativeContainerHandle(ih, child);

  return container_handle;
}

static void iClassObjectChildAdded(Iclass* ic, Ihandle* ih, Ihandle* child)
{
  if (ic->parent)
    iClassObjectChildAdded(ic->parent, ih, child);

  if (ic->ChildAdded)
    ic->ChildAdded(ih, child);
}

static void iClassObjectChildRemoved(Iclass* ic, Ihandle* ih, Ihandle* child, int pos)
{
  if (ic->parent)
    iClassObjectChildRemoved(ic->parent, ih, child, pos);

  if (ic->ChildRemoved)
    ic->ChildRemoved(ih, child, pos);
}

static void iClassLayoutUpdate(Iclass* ic, Ihandle *ih)
{
  if (ic->parent)
    iClassLayoutUpdate(ic->parent, ih);

  if (ic->LayoutUpdate)
    ic->LayoutUpdate(ih);
}

static int iClassDlgPopup(Iclass* ic, Ihandle* ih, int x, int y)
{
  /* must be before the parent class */
  if (ic->DlgPopup)
    return ic->DlgPopup(ih, x, y);  /* ignore parent if implemented */

  if (ic->parent)
    return iClassDlgPopup(ic->parent, ih, x, y);

  return IUP_INVALID;  /* means it is not implemented */
}

static int iClassHasDlgPopup(Iclass* ic)
{
  /* must be before the parent class */
  if (ic->DlgPopup)
    return 1;

  if (ic->parent)
    return iClassHasDlgPopup(ic->parent);

  return 0;
}


/*****************************************************************
                     Public Interface
*****************************************************************/


IUP_SDK_API int iupClassObjectCreate(Ihandle* ih, void** params)
{
  return iClassCreate(ih->iclass, ih, params);
}

IUP_SDK_API int iupClassObjectMap(Ihandle* ih)
{
  return iClassMap(ih->iclass, ih);
}

IUP_SDK_API void iupClassObjectUnMap(Ihandle* ih)
{
  iClassUnMap(ih->iclass, ih);
}

IUP_SDK_API void iupClassObjectDestroy(Ihandle* ih)
{
  iClassDestroy(ih->iclass, ih);
}

IUP_SDK_API void iupClassObjectComputeNaturalSize(Ihandle* ih, int *w, int *h, int *children_expand)
{
  iClassComputeNaturalSize(ih->iclass, ih, w, h, children_expand);
}

IUP_SDK_API void iupClassObjectSetChildrenCurrentSize(Ihandle* ih, int shrink)
{
  iClassSetChildrenCurrentSize(ih->iclass, ih, shrink);
}

IUP_SDK_API void iupClassObjectSetChildrenPosition(Ihandle* ih, int x, int y)
{
  iClassSetChildrenPosition(ih->iclass, ih, x, y);
}

IUP_SDK_API void* iupClassObjectGetInnerNativeContainerHandle(Ihandle* ih, Ihandle* child)
{
  return iClassGetInnerNativeContainerHandle(ih->iclass, ih, child);
}

IUP_SDK_API void iupClassObjectChildAdded(Ihandle* ih, Ihandle* child)
{
  iClassObjectChildAdded(ih->iclass, ih, child);
}

IUP_SDK_API void iupClassObjectChildRemoved(Ihandle* ih, Ihandle* child, int pos)
{
  iClassObjectChildRemoved(ih->iclass, ih, child, pos);
}

IUP_SDK_API void iupClassObjectLayoutUpdate(Ihandle *ih)
{
  iClassLayoutUpdate(ih->iclass, ih);
}

IUP_SDK_API int iupClassObjectDlgPopup(Ihandle* ih, int x, int y)
{
  return iClassDlgPopup(ih->iclass, ih, x, y);
}

IUP_SDK_API int iupClassObjectHasDlgPopup(Ihandle* ih)
{
  return iClassHasDlgPopup(ih->iclass);
}

/*****************************************************************
                        Class Definition
*****************************************************************/


static void iClassReleaseAttribFuncTable(Iclass* ic)
{
  char* name = iupTableFirst(ic->attrib_func);
  while (name)
  {
    void* afunc = iupTableGetCurr(ic->attrib_func);
    free(afunc);

    name = iupTableNext(ic->attrib_func);
  }

  iupTableDestroy(ic->attrib_func);
}

IUP_SDK_API Iclass* iupClassNew(Iclass* parent)
{
  Iclass* ic = malloc(sizeof(Iclass));
  memset(ic, 0, sizeof(Iclass));

  if (parent)
  {
    parent = parent->New();
    ic->attrib_func = parent->attrib_func;
    ic->parent = parent;
  }
  else
    ic->attrib_func = iupTableCreate(IUPTABLE_STRINGINDEXED);

  return ic;
}

IUP_SDK_API void iupClassRelease(Iclass* ic)
{
  Iclass* parent;

  /* must call Release only for the actual class */
  if (ic->Release)
    ic->Release(ic);

  /* must free the pointer for all classes, 
     since a new instance is created when we inherit */
  parent = ic->parent;
  while (parent)
  {
    Iclass* ic_tmp = parent;
    parent = parent->parent;
    free(ic_tmp);
  }

  /* attributes functions table is released only once */
  iClassReleaseAttribFuncTable(ic);

  free(ic);
}

IUP_SDK_API int iupClassMatch(Iclass* ic, const char* classname)
{
  /* check for all classes in the hierarchy */
  while (ic)
  {
    if (iupStrEqualNoCase(ic->name, classname))
      return 1;
    ic = ic->parent;
  }
  return 0;
}

/*****************************************************************
                        Main API
*****************************************************************/


IUP_API char* IupGetClassName(Ihandle *ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  return (char*)ih->iclass->name;
}

IUP_API char* IupGetClassType(Ihandle *ih)
{
  static char* type2str[] = { "void", "control", "canvas", "dialog", "image", "menu", "other" };

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return NULL;

  return type2str[ih->iclass->nativetype];
}

IUP_API int IupClassMatch(Ihandle* ih, const char* classname)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return 0;

  return iupClassMatch(ih->iclass, classname);
}
