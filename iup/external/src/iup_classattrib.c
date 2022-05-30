/** \file
 * \brief Ihandle Class Attribute Management
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
#include "iup_register.h"
#include "iup_globalattrib.h"


typedef struct _IattribFunc
{
  IattribGetFunc get;
  IattribSetFunc set;
  const char* default_value;
  const char* system_default;
  int call_global_default;
  int flags;
} IattribFunc;


int iupClassIsGlobalDefault(const char* name, int colors)
{
  if (!colors && iupStrEqual(name, "DEFAULTFONT"))
    return 1;
  if (iupStrEqual(name, "DLGBGCOLOR"))
    return 1;
  if (iupStrEqual(name, "DLGFGCOLOR"))
    return 1;
  if (iupStrEqual(name, "TXTBGCOLOR"))
    return 1;
  if (iupStrEqual(name, "TXTFGCOLOR"))
    return 1;
  if (iupStrEqual(name, "TXTHLCOLOR"))
    return 1;
  if (iupStrEqual(name, "LINKFGCOLOR"))
    return 1;
  if (iupStrEqual(name, "MENUBGCOLOR"))
    return 1;
  return 0;
}

/* '*' is used in IupMatrix to indicate a full line or column
   ':' is the regular separator for Lin:Col specification
   '-' the minus sign, so we can specify negative values */
#define IUP_CHECKIDSEP(_str) (*(_str) == '*' || *(_str) == ':' || *(_str) == '-')

static const char* iClassFindId(const char* name)
{
  while(*name)
  {
    if (*name >= '0' && *name <= '9')
      return name;
    if (IUP_CHECKIDSEP(name))
      return name;

    name++;
  }
  return NULL;
}

static const char* iClassCutNameId(const char* name, const char* name_id)
{
  static char str[100];
  int len = (int)(name_id - name);
  if (len == 0)
    return NULL;

  memcpy(str, name, len);
  str[len] = 0;
  return str;
}


static char* iClassGetDefaultValue(IattribFunc* afunc)
{
  if (afunc->call_global_default)
    return IupGetGlobal(afunc->default_value);
  else
    return (char*)afunc->default_value;
}

int iupClassObjectSetAttributeId2(Ihandle* ih, const char* name, int id1, int id2, const char* value)
{
  IattribFunc* afunc;

  if (ih->iclass->has_attrib_id!=2)
    return 1;  /* function not found, default to string */

  if (name[0]==0)
    name = "IDVALUE";  /* pure numbers are used as attributes in IupList and IupMatrix, 
                          translate them into IDVALUE. */
  afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, name);
  if (afunc)
  {         
    if (afunc->flags & IUPAF_READONLY)
    {
      if (afunc->flags & IUPAF_NO_STRING)
        return -1;  /* value is NOT a string, can NOT call iupAttribSetStr */
      return 0;
    }

    if (afunc->set && 
        (ih->handle || afunc->flags & IUPAF_NOT_MAPPED))
    {
      /* id numbered attributes have default value NULL always */

      if (afunc->flags & IUPAF_HAS_ID2)
      {
        IattribSetId2Func id2_set = (IattribSetId2Func)afunc->set;
        return id2_set(ih, id1, id2, value);
      }
      else if (afunc->flags & IUPAF_HAS_ID)
      {
        IattribSetIdFunc id_set = (IattribSetIdFunc)afunc->set;
        return id_set(ih, id1, value);  /* id2 is ignored */
      }
    }

    if (afunc->flags & IUPAF_NO_STRING)
      return -1; /* value is NOT a string, can NOT call iupAttribSetStr */
  }

  return 1;  /* function not found, default to string */
}

int iupClassObjectSetAttributeId(Ihandle* ih, const char* name, int id, const char * value)
{
  IattribFunc* afunc;

  if (ih->iclass->has_attrib_id==0)
    return 1;  /* function not found, default to string */

  if (name[0]==0)
    name = "IDVALUE";  /* pure numbers are used as attributes in IupList and IupMatrix, 
                          translate them into IDVALUE. */
  afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, name);
  if (afunc && afunc->flags & IUPAF_HAS_ID)
  {         
    if (afunc->flags & IUPAF_READONLY)
    {
      if (afunc->flags & IUPAF_NO_STRING)
        return -1;  /* value is NOT a string, can NOT call iupAttribSetStr */
      return 0;
    }

    if (afunc->set && 
        !(afunc->flags & IUPAF_HAS_ID2) &&
        (ih->handle || afunc->flags & IUPAF_NOT_MAPPED))
    {
      /* id numbered attributes have default value NULL always */
      IattribSetIdFunc id_set = (IattribSetIdFunc)afunc->set;
      return id_set(ih, id, value);
    }

    if (afunc->flags & IUPAF_NO_STRING)
      return -1; /* value is NOT a string, can NOT call iupAttribSetStr */
  }

  return 1;  /* function not found, default to string */
}

int iupClassObjectSetAttribute(Ihandle* ih, const char* name, const char * value, int *inherit)
{
  IattribFunc* afunc;

  if (ih->iclass->has_attrib_id!=0)
  {
    const char* name_id = iClassFindId(name);
    if (name_id)
    {
      const char* partial_name = iClassCutNameId(name, name_id);
      if (!partial_name)
        partial_name = "IDVALUE";  /* pure numbers are used as attributes in IupList and IupMatrix, 
                                      translate them into IDVALUE. */
      afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, partial_name);
      if (afunc && afunc->flags & IUPAF_HAS_ID)
      {         
        *inherit = 0;       /* id numbered attributes are NON inheritable always */

        if (afunc->flags & IUPAF_READONLY)
        {
          if (afunc->flags & IUPAF_NO_STRING)
            return -1;  /* value is NOT a string, can NOT call iupAttribSetStr */
          return 0;
        }

        if (afunc->set && (ih->handle || afunc->flags & IUPAF_NOT_MAPPED))
        {
          if (afunc->flags & IUPAF_HAS_ID2)
          {
            IattribSetId2Func id2_set = (IattribSetId2Func)afunc->set;
            int id1=IUP_INVALID_ID, id2=IUP_INVALID_ID;
            iupStrToIntInt(name_id, &id1, &id2, ':');  /* ignore errors because of '*' ids */
            return id2_set(ih, id1, id2, value);
          }
          else
          {
            IattribSetIdFunc id_set = (IattribSetIdFunc)afunc->set;
            int id=IUP_INVALID_ID;
            if (iupStrToInt(name_id, &id))
              return id_set(ih, id, value);
          }
        }

        if (afunc->flags & IUPAF_NO_STRING)
          return -1; /* value is NOT a string, can NOT call iupAttribSetStr */

        return 1; /* if the function exists, then must return here */
      }
    }
  }

  /* if not has_attrib_id, or not found an ID, or not found the partial name, check using the full name */

  afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, name);
  *inherit = 1; /* default is inheritable */
  if (afunc)
  {
    *inherit = !(afunc->flags & IUPAF_NO_INHERIT) &&   /* is inheritable */
               !(afunc->flags & IUPAF_NO_STRING);      /* is a string */

    if (afunc->flags & IUPAF_READONLY)
    {
      if (afunc->flags & IUPAF_NO_STRING)
        return -1;  /* value is NOT a string, can NOT call iupAttribSetStr */
      return 0;
    }

    if (afunc->set && (ih->handle || afunc->flags & IUPAF_NOT_MAPPED))
    {
      int ret;
      if (!value)
      {
        /* inheritable attributes when reset must check the parent value */
        if (*inherit && ih->parent)   
          value = iupAttribGetInherit(ih->parent, name); 

        if (!value)
          value = iClassGetDefaultValue(afunc);
      }

      if (afunc->flags & IUPAF_HAS_ID2)
      {
        IattribSetId2Func id2_set = (IattribSetId2Func)afunc->set;
        return id2_set(ih, IUP_INVALID_ID, IUP_INVALID_ID, value);  /* empty Id */
      }
      else if (afunc->flags & IUPAF_HAS_ID)
      {
        IattribSetIdFunc id_set = (IattribSetIdFunc)afunc->set;
        return id_set(ih, IUP_INVALID_ID, value);  /* empty Id */
      }
      else
        ret = afunc->set(ih, value);

      if (ret == 1 && afunc->flags & IUPAF_NO_STRING)
        return -1;  /* value is NOT a string, can NOT call iupAttribSetStr */

      if (*inherit)
        return 1;   /* inheritable attributes are always stored in the hash table, */
      else          /* to indicate that they are set at the control.               */
        return ret;
    }
  }

  return 1;  /* function not found, default to string */
}

char* iupClassObjectGetAttributeId2(Ihandle* ih, const char* name, int id1, int id2)
{
  IattribFunc* afunc;

  if (ih->iclass->has_attrib_id!=2)
    return NULL;

  if (name[0]==0)
    name = "IDVALUE";  /* pure numbers are used as attributes in IupList and IupMatrix, 
                          translate them into IDVALUE. */
  afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, name);
  if (afunc)
  {
    if (afunc->flags & IUPAF_WRITEONLY)
      return NULL;

    if (afunc->get && 
        (ih->handle || afunc->flags & IUPAF_NOT_MAPPED))
    {
      if (afunc->flags & IUPAF_HAS_ID2)
      {
        IattribGetId2Func id2_get = (IattribGetId2Func)afunc->get;
        return id2_get(ih, id1, id2);
      }
      else if (afunc->flags & IUPAF_HAS_ID)
      {
        IattribGetIdFunc id_get = (IattribGetIdFunc)afunc->get;
        return id_get(ih, id1);  /* id2 is ignored */
      }
    }
  }

  return NULL;
}

char* iupClassObjectGetAttributeId(Ihandle* ih, const char* name, int id)
{
  IattribFunc* afunc;

  if (ih->iclass->has_attrib_id==0)
    return NULL;

  if (name[0]==0)
    name = "IDVALUE";  /* pure numbers are used as attributes in IupList and IupMatrix, 
                          translate them into IDVALUE. */
  afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, name);
  if (afunc && afunc->flags & IUPAF_HAS_ID)
  {
    if (afunc->flags & IUPAF_WRITEONLY)
      return NULL;

    if (afunc->get && 
        !(afunc->flags & IUPAF_HAS_ID2) &&
        (ih->handle || afunc->flags & IUPAF_NOT_MAPPED))
    {
      IattribGetIdFunc id_get = (IattribGetIdFunc)afunc->get;
      return id_get(ih, id);
    }
  }

  return NULL;
}

char* iupClassObjectGetAttribute(Ihandle* ih, const char* name, char* *def_value, int *inherit)
{
  IattribFunc* afunc;

  if (ih->iclass->has_attrib_id!=0)
  {
    const char* name_id = iClassFindId(name);
    if (name_id)
    {
      const char* partial_name = iClassCutNameId(name, name_id);
      if (!partial_name)
        partial_name = "IDVALUE";  /* pure numbers are used as attributes in IupList and IupMatrix, 
                                      translate them into IDVALUE. */
      afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, partial_name);
      if (afunc && afunc->flags & IUPAF_HAS_ID)
      {
        *def_value = NULL;  /* id numbered attributes have default value NULL always */
        *inherit = 0;       /* id numbered attributes are NON inheritable always */

        if (afunc->flags & IUPAF_WRITEONLY)
          return NULL;

        if (afunc->get && (ih->handle || afunc->flags & IUPAF_NOT_MAPPED))
        {
          if (afunc->flags & IUPAF_HAS_ID2)
          {
            IattribGetId2Func id2_get = (IattribGetId2Func)afunc->get;
            int id1=IUP_INVALID_ID, id2=IUP_INVALID_ID;
            iupStrToIntInt(name_id, &id1, &id2, ':');
            return id2_get(ih, id1, id2);
          }
          else
          {
            IattribGetIdFunc id_get = (IattribGetIdFunc)afunc->get;
            int id=IUP_INVALID_ID;
            if (iupStrToInt(name_id, &id))
              return id_get(ih, id);
          }
        }
        else
          return NULL;      /* if the function exists, then must return here */
      }
    }
  }

  /* if not has_attrib_id, or not found an ID, or not found the partial name, check using the full name */

  afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, name);
  *def_value = NULL;
  *inherit = 1; /* default is inheritable */
  if (afunc)
  {
    *def_value = iClassGetDefaultValue(afunc);
    *inherit = !(afunc->flags & IUPAF_NO_INHERIT) &&   /* is inheritable */
               !(afunc->flags & IUPAF_NO_STRING);      /* is a string */

    if (afunc->flags & IUPAF_WRITEONLY)
      return NULL;

    if (afunc->get && (ih->handle || afunc->flags & IUPAF_NOT_MAPPED))
    {
      if (afunc->flags & IUPAF_HAS_ID2)
      {
        IattribGetId2Func id2_get = (IattribGetId2Func)afunc->get;
        return id2_get(ih, IUP_INVALID_ID, IUP_INVALID_ID);  /* empty Id */
      }
      else if (afunc->flags & IUPAF_HAS_ID)
      {
        IattribGetIdFunc id_get = (IattribGetIdFunc)afunc->get;
        return id_get(ih, IUP_INVALID_ID);  /* empty Id */
      }
      else
        return afunc->get(ih);
    }
  }
  return NULL;
}

void iupClassObjectGetAttributeInfo(Ihandle* ih, const char* name, char* *def_value, int *inherit)
{
  IattribFunc* afunc;

  if (ih->iclass->has_attrib_id!=0)
  {
    const char* name_id = iClassFindId(name);
    if (name_id)
    {
      const char* partial_name = iClassCutNameId(name, name_id);
      if (!partial_name)
        partial_name = "IDVALUE";  /* pure numbers are used as attributes in IupList and IupMatrix, 
                                      translate them into IDVALUE. */
      afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, partial_name);
      if (afunc && afunc->flags & IUPAF_HAS_ID)
      {
        *def_value = NULL;  /* id numbered attributes have default value NULL always */
        *inherit = 0;       /* id numbered attributes are NON inheritable always */
         return;      /* if the function exists, then must return here */
      }
    }
  }

  /* if not has_attrib_id, or not found an ID, or not found the partial name, check using the full name */

  afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, name);
  *def_value = NULL;
  *inherit = 1; /* default is inheritable */
  if (afunc)
  {
    *def_value = iClassGetDefaultValue(afunc);
    *inherit = !(afunc->flags & IUPAF_NO_INHERIT) &&  /* is inheritable */
               !(afunc->flags & IUPAF_NO_STRING);     /* is a string */
  }
}

void iupClassGetAttribNameInfo(Iclass* ic, const char* name, char* *def_value, int *flags)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  *def_value = NULL;
  *flags = 0;
  if (afunc)
  {
    *flags = afunc->flags;
    *def_value = iClassGetDefaultValue(afunc);
  }
}

int iupClassObjectCurAttribIsInherit(Iclass* ic)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGetCurr(ic->attrib_func);
  if (afunc && !(afunc->flags & IUPAF_NO_INHERIT))
    return 1;
  return 0;
}

int iupClassObjectAttribCanCopy(Ihandle* ih, const char* name)
{
  Iclass* ic = ih->iclass;
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  if (afunc && !(afunc->flags & IUPAF_NO_STRING) &&  /* is a string */
               !(afunc->flags & IUPAF_READONLY) &&   /* not read-only */
               !(afunc->flags & IUPAF_WRITEONLY) &&  /* not write-only */
               !(afunc->flags & IUPAF_CALLBACK))     /* not a callback */
    return 1;
  return 0;
}

int iupClassObjectAttribIsNotString(Ihandle* ih, const char* name)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, name);
  if (afunc && (afunc->flags & IUPAF_NO_STRING || afunc->flags & IUPAF_CALLBACK || afunc->flags & IUPAF_IHANDLE))
    return 1;
  return 0;
}

int iupClassObjectAttribIsIhandle(Ihandle* ih, const char* name)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, name);
  if (afunc && (afunc->flags & IUPAF_IHANDLE))
    return 1;
  return 0;
}

IUP_SDK_API int iupClassObjectAttribIsCallback(Ihandle* ih, const char* name)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ih->iclass->attrib_func, name);
  if (afunc && (afunc->flags & IUPAF_CALLBACK))
    return 1;
  return 0;
}

int iupClassAttribIsRegistered(Iclass* ic, const char* name)
{
  IattribFunc* afunc = NULL;

  if (ic->has_attrib_id!=0)
  {
    const char* name_id = iClassFindId(name);
    if (name_id)
    {
      const char* partial_name = iClassCutNameId(name, name_id);
      if (!partial_name)
        partial_name = "IDVALUE";  /* pure numbers are used as attributes in IupList and IupMatrix, 
                                      translate them into IDVALUE. */
      afunc = (IattribFunc*)iupTableGet(ic->attrib_func, partial_name);
    }
  }

  if (!afunc)
    afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);

  if (afunc)
    return 1;

  return 0;
}

IUP_SDK_API void iupClassRegisterAttribute(Iclass* ic, const char* name,
                               IattribGetFunc _get, IattribSetFunc _set, 
                               const char* _default_value, const char* _system_default, int _flags)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  if (afunc)
    free(afunc);  /* overwrite a previous registration */

  afunc = (IattribFunc*)malloc(sizeof(IattribFunc));
  afunc->get = _get;
  afunc->set = _set;
  if (_default_value == IUPAF_SAMEASSYSTEM)
    afunc->default_value = _system_default;
  else
    afunc->default_value = _default_value;
  afunc->system_default = _system_default;
  afunc->flags = _flags;

  if (iupClassIsGlobalDefault(afunc->default_value, 0))
    afunc->call_global_default = 1;
  else
    afunc->call_global_default = 0;

  iupTableSet(ic->attrib_func, name, (void*)afunc, IUPTABLE_POINTER);
}

IUP_SDK_API void iupClassRegisterAttributeId(Iclass* ic, const char* name,
                               IattribGetIdFunc _get, IattribSetIdFunc _set, 
                               int _flags)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  if (afunc)
    free(afunc);  /* overwrite a previous registration */

  afunc = (IattribFunc*)malloc(sizeof(IattribFunc));
  afunc->get = (IattribGetFunc)_get;
  afunc->set = (IattribSetFunc)_set;
  afunc->default_value = NULL;
  afunc->system_default = NULL;
  afunc->flags = _flags|IUPAF_HAS_ID|IUPAF_NO_INHERIT|IUPAF_NO_DEFAULTVALUE;
  afunc->call_global_default = 0;

  iupTableSet(ic->attrib_func, name, (void*)afunc, IUPTABLE_POINTER);
}

IUP_SDK_API void iupClassRegisterAttributeId2(Iclass* ic, const char* name,
                               IattribGetId2Func _get, IattribSetId2Func _set, 
                               int _flags)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  if (afunc)
    free(afunc);  /* overwrite a previous registration */

  afunc = (IattribFunc*)malloc(sizeof(IattribFunc));
  afunc->get = (IattribGetFunc)_get;
  afunc->set = (IattribSetFunc)_set;
  afunc->default_value = NULL;
  afunc->system_default = NULL;
  afunc->flags = _flags|IUPAF_HAS_ID2|IUPAF_HAS_ID|IUPAF_NO_INHERIT|IUPAF_NO_DEFAULTVALUE;
  afunc->call_global_default = 0;

  iupTableSet(ic->attrib_func, name, (void*)afunc, IUPTABLE_POINTER);
}

IUP_SDK_API void iupClassRegisterGetAttribute(Iclass* ic, const char* name,
                                  IattribGetFunc *_get, IattribSetFunc *_set, 
                                  const char* *_default_value, const char* *_system_default, int *_flags)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  if (afunc)
  {
    if (_get) *_get = afunc->get;
    if (_set) *_set = afunc->set;
    if (_default_value) *_default_value = afunc->default_value;
    if (_system_default) *_system_default = afunc->system_default;
    if (_flags) *_flags = afunc->flags;
  }
}

IUP_SDK_API void iupClassRegisterReplaceAttribFunc(Iclass* ic, const char* name, IattribGetFunc _get, IattribSetFunc _set)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  if (afunc)
  {
    if (_get) afunc->get = _get;
    if (_set) afunc->set = _set;
  }
}

IUP_SDK_API void iupClassRegisterReplaceAttribDef(Iclass* ic, const char* name, const char* _default_value, const char* _system_default)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  if (afunc)
  {
    if (_default_value == IUPAF_SAMEASSYSTEM)
      afunc->default_value = _system_default;
    else
      afunc->default_value = _default_value;
    afunc->system_default = _system_default;

    if (iupClassIsGlobalDefault(afunc->default_value, 0))
      afunc->call_global_default = 1;
    else
      afunc->call_global_default = 0;
  }
}

IUP_SDK_API void iupClassRegisterReplaceAttribFlags(Iclass* ic, const char* name, int _flags)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  if (afunc)
    afunc->flags = _flags;
}

IUP_SDK_API void iupClassRegisterCallback(Iclass* ic, const char* name, const char* format)
{
  /* Since attributes and callbacks do not conflict 
     we can use the same structure to store the callback format using the default_value. */
  iupClassRegisterAttribute(ic, name, NULL, NULL, format, NULL, IUPAF_NO_INHERIT|IUPAF_CALLBACK);
}

IUP_SDK_API char* iupClassCallbackGetFormat(Iclass* ic, const char* name)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  if (afunc)
    return (char*)afunc->default_value;
  return NULL;
}

IUP_API int IupGetClassAttributes(const char* classname, char** names, int n)
{
  Iclass* ic;
  int i = 0;
  char* name;
  IattribFunc* afunc;

  iupASSERT(classname!=NULL);
  if (!classname)
    return 0;

  ic = iupRegisterFindClass(classname);
  if (!ic)
    return -1;

  if (!names || n==0 || n==-1)
    return iupTableCount(ic->attrib_func);

  name = iupTableFirst(ic->attrib_func);
  while (name)
  {
    afunc = (IattribFunc*)iupTableGetCurr(ic->attrib_func);

    if (!(afunc->flags&IUPAF_CALLBACK))
    {
      names[i] = name;
      i++;
      if (i == n)
        break;
    }

    name = iupTableNext(ic->attrib_func);
  }

  return i;
}

IUP_API int IupGetClassCallbacks(const char* classname, char** names, int n)
{
  Iclass* ic;
  int i = 0;
  char* name;
  IattribFunc* afunc;

  iupASSERT(classname!=NULL);
  if (!classname)
    return 0;

  ic = iupRegisterFindClass(classname);
  if (!ic)
    return -1;

  if (!names || n == 0 || n == -1)
    return iupTableCount(ic->attrib_func);

  name = iupTableFirst(ic->attrib_func);
  while (name)
  {
    afunc = (IattribFunc*)iupTableGetCurr(ic->attrib_func);

    if (afunc->flags&IUPAF_CALLBACK)
    {
      names[i] = name;
      i++;
      if (i == n)
        break;
    }

    name = iupTableNext(ic->attrib_func);
  }

  return i;
}

IUP_API void IupSetClassDefaultAttribute(const char* classname, const char *name, const char* default_value)
{
  Iclass* ic;
  IattribFunc* afunc;

  iupASSERT(classname!=NULL);
  if (!classname)
    return;

  iupASSERT(name!=NULL);
  if (!name)
    return;

  ic = iupRegisterFindClass(name);
  if (!ic)
    return;

  afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
  if (afunc && !(afunc->flags & IUPAF_NO_DEFAULTVALUE) &&   /* can has default */
               !(afunc->flags & IUPAF_NO_STRING) &&     /* is a string */
               !(afunc->flags & IUPAF_HAS_ID))
  {
    if (default_value == IUPAF_SAMEASSYSTEM)
      afunc->default_value = afunc->system_default;
    else
      afunc->default_value = default_value;

    if (iupClassIsGlobalDefault(afunc->default_value, 0))
      afunc->call_global_default = 1;
    else
      afunc->call_global_default = 0;
  }
  else if (!afunc && default_value)
    iupClassRegisterAttribute(ic, name, NULL, NULL, default_value, NULL, IUPAF_DEFAULT);
}

IUP_API void IupSaveClassAttributes(Ihandle* ih)
{
  int has_attrib_id, start_id = 0;
  Iclass* ic;
  char *name;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  ic = ih->iclass;

  has_attrib_id = ic->has_attrib_id;
  if (iupClassMatch(ic, "tree") || /* tree can only set id attributes after map, so they can not be saved */
      iupClassMatch(ic, "cells")) /* cells does not have any saveable id attributes */
    has_attrib_id = 0;  

  if (iupClassMatch(ic, "list"))
    start_id = 1;

  name = iupTableFirst(ic->attrib_func);
  while (name)
  {
    IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
    if (afunc && !(afunc->flags & IUPAF_NO_STRING) &&  /* is a string */
                 !(afunc->flags & IUPAF_READONLY) &&   /* not read-only */
                 !(afunc->flags & IUPAF_WRITEONLY) &&  /* not write-only */
                 !(afunc->flags & IUPAF_CALLBACK))     /* not a callback */
    {
      if ((afunc->flags&IUPAF_NO_SAVE) && iupBaseNoSaveCheck(ih, name))  /* can not be saved */
      {
        name = iupTableNext(ic->attrib_func);
        continue;
      }

      if (!(afunc->flags & IUPAF_HAS_ID))     /* no ID */
      {
        int inherit;
        char *def_value;
        char *value = iupClassObjectGetAttribute(ih, name, &def_value, &inherit);
        if (value && value[0])    /* NOT NULL and not empty */
        {
          if ((def_value && iupStrEqualNoCase(def_value, value)) ||  /* equal to the default value */
              (!def_value && iupStrFalse(value)))   /* default=NULL and value=NO */
          {
            name = iupTableNext(ic->attrib_func);
            continue;
          }

          if (!iupStrEqualNoCase(value, iupAttribGet(ih, name)))     /* NOT already stored */
            iupAttribSetStr(ih, name, value);
        }
      }
      else if (has_attrib_id)
      {
        char *value;

        if (iupStrEqual(name, "IDVALUE"))
          name = "";

        if (afunc->flags&IUPAF_HAS_ID2)
        {
          int lin, col, 
              numcol = IupGetInt(ih, "NUMCOL")+1,
              numlin = IupGetInt(ih, "NUMLIN")+1;
          for (lin=0; lin<numlin; lin++)
          {
            for (col=0; col<numcol; col++)
            {
              value = iupClassObjectGetAttributeId2(ih, name, lin, col);
              if (value && value[0])  /* NOT NULL and not empty */
              {
                if (!iupStrEqualNoCase(value, iupAttribGetId2(ih, name, lin, col)))     /* NOT already stored */
                  iupAttribSetStrId2(ih, name, lin, col, value);
              }
            }
          }
        }
        else
        {
          int id, count = IupGetInt(ih, "COUNT");
          for (id=start_id; id<count+start_id; id++)
          {
            value = iupClassObjectGetAttributeId(ih, name, id);
            if (value && value[0])  /* NOT NULL and not empty */
            {
              if (!iupStrEqualNoCase(value, iupAttribGetId(ih, name, id)))     /* NOT already stored */
                iupAttribSetStrId(ih, name, id, value);
            }
          }
        }
      }
    }

    name = iupTableNext(ic->attrib_func);
  }
}

IUP_API void IupCopyClassAttributes(Ihandle* src_ih, Ihandle* dst_ih)
{
  int has_attrib_id, start_id = 0;
  Iclass* ic;
  char *name;

  iupASSERT(iupObjectCheck(src_ih));
  if (!iupObjectCheck(src_ih))
    return;

  iupASSERT(iupObjectCheck(dst_ih));
  if (!iupObjectCheck(dst_ih))
    return;

  if (!IupClassMatch(dst_ih, src_ih->iclass->name))
    return;

  ic = src_ih->iclass;

  has_attrib_id = ic->has_attrib_id;
  if (iupClassMatch(ic, "tree") ||  /* tree can only set id attributes after map, so they can not be saved */
      iupClassMatch(ic, "cells"))   /* cells does not have any savable id attributes */
    has_attrib_id = 0;  

  if (iupClassMatch(ic, "list"))
    start_id = 1;

  name = iupTableFirst(ic->attrib_func);
  while (name)
  {
    IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
    if (afunc && !(afunc->flags & IUPAF_NO_STRING) &&  /* is a string */
                 !(afunc->flags & IUPAF_READONLY) &&   /* not read-only */
                 !(afunc->flags & IUPAF_WRITEONLY) &&  /* not write-only */
                 !(afunc->flags & IUPAF_CALLBACK))     /* not a callback */
    {
      if ((afunc->flags&IUPAF_NO_SAVE) && iupBaseNoSaveCheck(src_ih, name))  /* can not be saved */
      {
        name = iupTableNext(ic->attrib_func);
        continue;
      }

      if (!(afunc->flags & IUPAF_HAS_ID))     /* no ID */
      {
        char *value = IupGetAttribute(src_ih, name);
        if (value && value[0])    /* NOT NULL and not empty */
        {
          if (!iupStrEqualNoCase(value, IupGetAttribute(dst_ih, name)))     /* NOT already equal */
            IupStoreAttribute(dst_ih, name, value);
        }
      }
      else if (has_attrib_id)
      {
        char *value;

        if (iupStrEqual(name, "IDVALUE"))
          name = "";

        if (afunc->flags&IUPAF_HAS_ID2)
        {
          int lin, col, 
              numcol = IupGetInt(src_ih, "NUMCOL")+1,
              numlin = IupGetInt(src_ih, "NUMLIN")+1;
          for (lin=0; lin<numlin; lin++)
          {
            for (col=0; col<numcol; col++)
            {
              value = IupGetAttributeId2(src_ih, name, lin, col);
              if (value && value[0])  /* NOT NULL and not empty */
              {
                if (!iupStrEqualNoCase(value, IupGetAttributeId2(dst_ih, name, lin, col)))     /* NOT already stored */
                  IupStoreAttributeId2(dst_ih, name, lin, col, value);
              }
            }
          }
        }
        else
        {
          int id, count = IupGetInt(src_ih, "COUNT");
          for (id=start_id; id<count+start_id; id++)
          {
            value = IupGetAttributeId(src_ih, name, id);
            if (value && value[0])  /* NOT NULL and not empty */
            {
              if (!iupStrEqualNoCase(value, IupGetAttributeId(dst_ih, name, id)))     /* NOT already stored */
                IupStoreAttributeId(dst_ih, name, id, value);
            }
          }
        }
      }
    }

    name = iupTableNext(ic->attrib_func);
  }

  /* loop again over all registered attributes to check for different values,
     in the previous loop some attribute my set another attribute during the loop */
  name = iupTableFirst(ic->attrib_func);
  while (name)
  {
    IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, name);
    if (afunc && !(afunc->flags & IUPAF_NO_STRING) &&   /* is a string */
                 !(afunc->flags & IUPAF_READONLY) &&    /* not read-only */
                 !(afunc->flags & IUPAF_WRITEONLY) &&   /* not write-only */
                 !(afunc->flags & IUPAF_HAS_ID) &&      /* no ID */
                 !(afunc->flags & IUPAF_CALLBACK))      /* not a callback */
    {
      char *value = IupGetAttribute(src_ih, name);
      if (value &&     /* NOT NULL */
          !iupStrEqualNoCase(value, IupGetAttribute(dst_ih, name)))     /* NOT already stored */
        IupStoreAttribute(dst_ih, name, value);
    }

    name = iupTableNext(ic->attrib_func);
  }
}

void iupClassObjectEnsureDefaultAttributes(Ihandle* ih)
{
  Iclass* ic;
  char *name;

  ic = ih->iclass;

  name = iupTableFirst(ic->attrib_func);
  while (name)
  {
    IattribFunc* afunc = (IattribFunc*)iupTableGetCurr(ic->attrib_func);
    if (afunc && afunc->set && 
        (afunc->default_value || afunc->system_default) &&
        !(afunc->flags & IUPAF_NO_DEFAULTVALUE) &&   /* can has default */
        !(afunc->flags & IUPAF_NO_STRING) &&       /* is a string */
        !(afunc->flags & IUPAF_HAS_ID))
    {
      if ((!iupStrEqualNoCase(afunc->default_value, afunc->system_default)) || 
          (afunc->call_global_default && afunc->default_value && iupGlobalDefaultColorChanged(afunc->default_value)))
      {
        if ((!ih->handle &&  (afunc->flags & IUPAF_NOT_MAPPED)) ||
             (ih->handle && !(afunc->flags & IUPAF_NOT_MAPPED)))
        {
          char* value = iupAttribGet(ih, name);
          if (!value)  /* if set will be updated later */
            afunc->set(ih, iClassGetDefaultValue(afunc));
        }
      }
    }

    name = iupTableNext(ic->attrib_func);
  }
}

void iupClassUpdate(Iclass* ic)
{
  IattribFunc* afunc = (IattribFunc*)iupTableGet(ic->attrib_func, "CLASSUPDATE");
  if (afunc && afunc->set)
    afunc->set((Ihandle*)ic, NULL);
}
