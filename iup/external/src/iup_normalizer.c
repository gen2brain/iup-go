/** \file
 * \brief Normalizer Element.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_array.h"
#include "iup_stdcontrols.h"
#include "iup_normalizer.h"
#include "iup_varg.h"


enum {NORMALIZE_NONE, NORMALIZE_WIDTH, NORMALIZE_HEIGHT};

struct _IcontrolData 
{
  Iarray* ih_array;
  int ret_control;  /* for first/next attributes */
};

int iupNormalizeGetNormalizeSize(const char* value)
{
  if (!value)
    return NORMALIZE_NONE;
  if (iupStrEqualNoCase(value, "HORIZONTAL"))
    return NORMALIZE_WIDTH;
  if (iupStrEqualNoCase(value, "VERTICAL"))
    return NORMALIZE_HEIGHT;
  if (iupStrEqualNoCase(value, "BOTH"))
    return NORMALIZE_WIDTH|NORMALIZE_HEIGHT;
  return NORMALIZE_NONE;
}

char* iupNormalizeGetNormalizeSizeStr(int normalize)
{
  char* int2str[] = {"NONE", "HORIZONTAL", "VERTICAL", "BOTH"};
  return int2str[normalize];
}

void iupNormalizeSizeBoxChild(Ihandle *ih, int normalize, int children_natural_maxwidth, int children_natural_maxheight)
{
  /* It is called from Vbox and Hbox ComputeNaturalSizeMethod after the natural size is calculated */
  /* reset the natural width and/or height */
  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!(child->flags & IUP_FLOATING) && 
        (child->iclass->nativetype != IUP_TYPEVOID || 
         !IupClassMatch(child, "fill")))
    {
      if (normalize & NORMALIZE_WIDTH) 
        child->naturalwidth = children_natural_maxwidth;
      if (normalize & NORMALIZE_HEIGHT)
        child->naturalheight = children_natural_maxheight;
    }
  }
}

static int iNormalizerSetNormalizeAttrib(Ihandle* ih, const char* value)
{
  int i, count;
  Ihandle** ih_list;
  Ihandle* ih_control;
  int natural_maxwidth = 0, natural_maxheight = 0;
  int normalize = iupNormalizeGetNormalizeSize(value);
  if (!normalize)
    return 1;

  count = iupArrayCount(ih->data->ih_array);
  ih_list = (Ihandle**)iupArrayGetData(ih->data->ih_array);

  for (i = 0; i < count; i++)
  {
    ih_control = ih_list[i];
    iupBaseComputeNaturalSize(ih_control);
    natural_maxwidth = iupMAX(natural_maxwidth, ih_control->naturalwidth);
    natural_maxheight = iupMAX(natural_maxheight, ih_control->naturalheight);
  }

  for (i = 0; i < count; i++)
  {
    ih_control = ih_list[i];
    if (!(ih_control->flags & IUP_FLOATING) && 
        (ih_control->iclass->nativetype != IUP_TYPEVOID || 
         !IupClassMatch(ih_control, "fill")))
    {
      if (normalize & NORMALIZE_WIDTH)
        ih_control->userwidth = natural_maxwidth;
      if (normalize & NORMALIZE_HEIGHT)
        ih_control->userheight = natural_maxheight;
    }
  }
  return 1;
}

static int iNormalizerSetAddControlHandleAttrib(Ihandle* ih, const char* value)
{
  Ihandle* ih_control = (Ihandle*)value;
  if (iupObjectCheck(ih_control))
  {
    Ihandle** ih_list = (Ihandle**)iupArrayInc(ih->data->ih_array);
    int count = iupArrayCount(ih->data->ih_array);
    ih_list[count - 1] = ih_control;
  }
  return 0;
}

static int iNormalizerSetAddControlAttrib(Ihandle* ih, const char* value)
{
  return iNormalizerSetAddControlHandleAttrib(ih, (char*)IupGetHandle(value));
}

static int iNormalizerSetDelControlHandleAttrib(Ihandle* ih, const char* value)
{
  Ihandle* ih_control = (Ihandle*)value;
  if (iupObjectCheck(ih_control))
  {
    int i, count = iupArrayCount(ih->data->ih_array);
    Ihandle** ih_list = (Ihandle**)iupArrayGetData(ih->data->ih_array);

    for (i = 0; i < count; i++)
    {
      if (ih_list[i] == ih_control)
      {
        iupArrayRemove(ih->data->ih_array, i, 1);
        return 0;
      }
    }
  }
  return 0;
}

static int iNormalizerSetDelControlAttrib(Ihandle* ih, const char* value)
{
  return iNormalizerSetDelControlHandleAttrib(ih, (char*)IupGetHandle(value));
}

static char* iNormalizerGetFirstControlHandleAttrib(Ihandle* ih)
{
  int count = iupArrayCount(ih->data->ih_array);
  Ihandle** ih_list = (Ihandle**)iupArrayGetData(ih->data->ih_array);

  if (count == 0)
    return NULL;

  ih->data->ret_control = 0;
  return (char*)ih_list[ih->data->ret_control];
}

static char* iNormalizerGetNextControlHandleAttrib(Ihandle* ih)
{
  int count = iupArrayCount(ih->data->ih_array);
  Ihandle** ih_list = (Ihandle**)iupArrayGetData(ih->data->ih_array);

  if (count == 0 || ih->data->ret_control >= count - 1)
    return NULL;

  ih->data->ret_control++;
  return (char*)ih_list[ih->data->ret_control];
}

/*******************************************************************************/


static void iNormalizerComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)w;
  (void)h;
  (void)children_expand;
  iNormalizerSetNormalizeAttrib(ih, iupAttribGetStr(ih, "NORMALIZE"));
}

static int iNormalizerCreateMethod(Ihandle* ih, void** params)
{
  ih->data = iupALLOCCTRLDATA();
  ih->data->ih_array = iupArrayCreate(10, sizeof(Ihandle*));

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    Ihandle** ih_list;
    int i = 0;
    while (*iparams) 
    {
      ih_list = (Ihandle**)iupArrayInc(ih->data->ih_array);
      ih_list[i] = *iparams;
      i++;
      iparams++;
    }
  }

  return IUP_NOERROR;
}

static void iNormalizerDestroy(Ihandle* ih)
{
  iupArrayDestroy(ih->data->ih_array);
}

Iclass* iupNormalizerNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "normalizer";
  ic->format = "g"; /* array of Ihandle */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  /* Class functions */
  ic->New = iupNormalizerNewClass;
  ic->Create = iNormalizerCreateMethod;
  ic->Map = iupBaseTypeVoidMapMethod;
  ic->ComputeNaturalSize = iNormalizerComputeNaturalSizeMethod;
  ic->Destroy = iNormalizerDestroy;

  /* Base Callbacks */
  iupBaseRegisterBaseCallbacks(ic);

  iupClassRegisterAttribute(ic, "NORMALIZE", NULL, iNormalizerSetNormalizeAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDCONTROL_HANDLE", NULL, iNormalizerSetAddControlHandleAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_IHANDLE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDCONTROL", NULL, iNormalizerSetAddControlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DELCONTROL_HANDLE", NULL, iNormalizerSetDelControlHandleAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_IHANDLE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DELCONTROL", NULL, iNormalizerSetDelControlAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_IHANDLENAME | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FIRST_CONTROL_HANDLE", iNormalizerGetFirstControlHandleAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_IHANDLE | IUPAF_NO_STRING);
  iupClassRegisterAttribute(ic, "NEXT_CONTROL_HANDLE", iNormalizerGetNextControlHandleAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT | IUPAF_IHANDLE | IUPAF_NO_STRING);

  return ic;
}

IUP_API Ihandle* IupNormalizerv(Ihandle **ih_list)
{
  return IupCreatev("normalizer", (void**)ih_list);
}

IUP_API Ihandle* IupNormalizerV(Ihandle* ih_first, va_list arglist)
{
  return IupCreateV("normalizer", ih_first, arglist);
}

IUP_API Ihandle* IupNormalizer(Ihandle* ih_first, ...)
{
  Ihandle *ih;

  va_list arglist;
  va_start(arglist, ih_first);
  ih = IupCreateV("normalizer", ih_first, arglist);
  va_end(arglist);

  return ih;
}
