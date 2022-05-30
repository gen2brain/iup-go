/** \file
 * \brief parser for LED. 
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>         

#include "iup.h"

#include "iup_object.h"
#include "iup_ledlex.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_assert.h"
            
            
#define IPARSE_SYMBEXIST       1
#define IPARSE_SYMBNOTDEF      2

static void* iParseExp (void);
static Ihandle* iParseFunction (Iclass *ic);
static int iParseError (int err, char *s);

static int iparse_error = 0;
static int iparse_saveinfo = 0;
#define IPARSE_RETURN_IF_ERROR(_e)        {iparse_error=(_e); if (iparse_error) return NULL;}
#define IPARSE_RETURN_IF_ERROR_FREE(_e, _x)   {iparse_error=(_e); if (iparse_error) { if (_x) free(_x); return NULL;} }



IUP_SDK_API const char* iupLoadLed(const char *filename, const char *buffer, int save_info)
{
  iparse_saveinfo = save_info;

  iparse_error = iupLexStart(filename, buffer);
  if (iparse_error)
  {
    iupLexClose();
    return iupLexGetError();
  }

  while (iupLexLookAhead() != IUPLEX_TK_END)
  {
    (void)iParseExp(); /* ignore return */
    if (iparse_error)
    {
      iupLexClose();
      return iupLexGetError();
    }
  }

  iupLexClose();
  return NULL;
}

IUP_API char* IupLoad(const char *filename)
{
  iupASSERT(filename != NULL);
  if (!filename)
    return "invalid file name";

  return (char*)iupLoadLed(filename, NULL, 0);  /* no save info */
}

IUP_API char* IupLoadBuffer(const char *buffer)
{
  iupASSERT(buffer != NULL);
  if (!buffer)
    return "invalid buffer";

  return (char*)iupLoadLed(NULL, buffer, 0);  /* no save info */
}

static void* iParseExp(void)
{
  char* nm = NULL;
  Ihandle* ih = NULL;

  int match = iupLexSeenMatch(IUPLEX_TK_FUNC, &iparse_error);
  IPARSE_RETURN_IF_ERROR_FREE(iparse_error, nm);

  if (match)
    return iParseFunction(iupLexGetClass());  /* control + attributes + parameters = return ih */

  if (iupLexLookAhead() == IUPLEX_TK_NAME)
  {
    nm = iupLexGetName();
    IPARSE_RETURN_IF_ERROR_FREE(iupLexAdvance(), nm);
  }
  else
  {
    if (iupLexFollowedBy(IUPLEX_TK_ENDP)) /* allow empty containers */
      return NULL;
    else
    {
      iparse_error = iupLexMatch(IUPLEX_TK_NAME);
      return NULL;  /* force iparse_error */
    }
  }

  match = iupLexSeenMatch(IUPLEX_TK_SET, &iparse_error); 
  IPARSE_RETURN_IF_ERROR_FREE(iparse_error, nm);

  if (match) /* new control '=' */
  {
    ih = (Ihandle*)iParseExp();  
    IPARSE_RETURN_IF_ERROR_FREE(iparse_error, nm);
    if (ih)
    {
      if (iparse_saveinfo)
      {
        Ihandle* old_ih = IupGetHandle(nm);
        if (iupObjectCheck(old_ih))  /* error only if old handle is still valid */
          IPARSE_RETURN_IF_ERROR_FREE(iParseError(IPARSE_SYMBEXIST, nm), nm);
      }

      IupSetHandle(nm, ih);

      if (iparse_saveinfo && nm)
      {
        int line = iupLexGetLine();
        iupAttribSetInt(ih, "_IUPLED_LINE", line);  /* not an exact location, but enough for its use */
      }
    }
  }
  else  /* just a name */
  {
    ih = IupGetHandle(nm);
    if (!ih)
    {
      if (!iparse_saveinfo)
      {
        IPARSE_RETURN_IF_ERROR_FREE(iParseError(IPARSE_SYMBNOTDEF, nm), nm);
      }
      else
      {
        /* dummy element to be replaced later */
        ih = IupUser();
        iupAttribSetStr(ih, "_IUPLED_NOTDEF_NAME", nm);
      }
    }
  }

  if (nm) free(nm);
  return ih;
}

static void* iParseControlParam(char type)
{
  switch(type)
  {
  case 'a':
    IPARSE_RETURN_IF_ERROR(iupLexMatch(IUPLEX_TK_NAME));
    return (void*)iupLexGetName();

  case 's':
    IPARSE_RETURN_IF_ERROR(iupLexMatch(IUPLEX_TK_STR));
    return (void*)iupLexGetName();

  case 'b':
  case 'c':
    IPARSE_RETURN_IF_ERROR(iupLexMatch(IUPLEX_TK_NAME));
    return (void*)(unsigned long)iupLexByte();

  case 'i':
  case 'j':
    IPARSE_RETURN_IF_ERROR(iupLexMatch(IUPLEX_TK_NAME));
    return (void*)(unsigned long)iupLexInt();

  case 'f':
    IPARSE_RETURN_IF_ERROR(iupLexMatch(IUPLEX_TK_NAME));
    {
      float f = iupLexFloat();
      unsigned long* l = (unsigned long*)&f;
      return (void*)*l;
    }

  case 'g':
  case 'h':
    {
      void *p = iParseExp();
      IPARSE_RETURN_IF_ERROR(iparse_error);
      return p;
    }

  default:
    return 0;
  }
}

static void iParseRelaseParamArray(void** params, const char *format, int num_format)
{
  int i;

  for (i = 0; params[i] && i<num_format; i++)
  {
    if (format[i] == 'j' ||
        format[i] == 'g' ||
        format[i] == 'c')
        break;

    if (format[i] == 'a' || format[i] == 's')
    {
      if (params[i])
        free(params[i]);   /* iupLexGetName returned a duplicated string */
    }
  }
}

static Ihandle* iParseControl(Iclass *ic)
{
  const char *format = ic->format;
  if (!format || format[0] == 0)
    return iupObjectCreate(ic, NULL); 
  else
  { 
    Ihandle *new_control;
    void** params;
    int i, alloc_arg, num_arg,
      num_format = (int)strlen(format);
                      
    num_arg = num_format;
    alloc_arg = num_arg+20;
    params = (void**)calloc(sizeof(void*), alloc_arg);

    for (i = 0; i < num_arg; )
    {
      char p_format = format[i];

      if (i > 0)
      {
        iparse_error = iupLexMatch(IUPLEX_TK_COMMA);
        if (iparse_error) iParseRelaseParamArray(params, format, num_format);
        IPARSE_RETURN_IF_ERROR_FREE(iparse_error, params);
      }

      if (p_format != 'j' &&    /* not array */
          p_format != 'g' && 
          p_format != 'c')
      {
        params[i] = iParseControlParam(p_format);
        i++;
        if (iparse_error) iParseRelaseParamArray(params, format, num_format);
        IPARSE_RETURN_IF_ERROR_FREE(iparse_error, params);
      }
      else    /* array */
      {
        int match;
        do
        {
          if (num_arg == i)
          {
            num_arg++;
            if (num_arg >= alloc_arg)
            {
              alloc_arg = num_arg+20;
              params = realloc(params, sizeof(void*)*alloc_arg);
            }
          }
          params[i] = iParseControlParam(p_format);
          i++;
          if (iparse_error) iParseRelaseParamArray(params, format, num_format);
          IPARSE_RETURN_IF_ERROR_FREE(iparse_error, params);
          match = iupLexSeenMatch(IUPLEX_TK_COMMA,&iparse_error);
          if (iparse_error) iParseRelaseParamArray(params, format, num_format);
          IPARSE_RETURN_IF_ERROR_FREE(iparse_error, params);
        } while (match); 

        /* after an array of parameters there are no more parameters */
        break;
      }
    }

    params[i] = NULL;
    new_control = iupObjectCreate(ic, params);

    if (iparse_saveinfo)
    {
      for (i = 0; i < num_format; i++)
      {
        if (format[i] == 's' || format[i] == 'a')
        {
          const char* name = NULL;
          if (i == 0)
            name = ic->format_attr;
          if (!name)
          {
            if (format[i] == 'a')
              name = "ACTION";
            else
              name = "TITLE";
          }

          if (name)
          {
            char led_name[200] = "_IUPLED_SAVED_";
            strcat(led_name, name);
            iupAttribSet(new_control, led_name, "1");
          }
        }
      }
    }

    iParseRelaseParamArray(params, format, num_format);

    free(params);
    return new_control;
  }
}

static Ihandle* iParseFunction(Iclass *ic)
{
  Ihandle* ih = NULL;
  char *attr = NULL;

  int match = iupLexSeenMatch(IUPLEX_TK_ATTR,&iparse_error); 
  IPARSE_RETURN_IF_ERROR_FREE(iparse_error, attr);

  if (match)
    attr = iupLexGetName();
  
  IPARSE_RETURN_IF_ERROR_FREE(iupLexMatch(IUPLEX_TK_BEGP), attr);

  ih = iParseControl(ic);
  IPARSE_RETURN_IF_ERROR_FREE(iparse_error, attr);

  if (iparse_saveinfo)
    iupAttribSetStr(ih, "_IUPLED_FILENAME", iupLexFilename());

  if (attr)
  {
    iupAttribParse(ih, attr, iparse_saveinfo);
    free(attr);
    attr = NULL;
  }

  IPARSE_RETURN_IF_ERROR_FREE(iupLexMatch(IUPLEX_TK_ENDP), attr);
  return ih;
}

static int iParseError(int err, char *s)
{
  char msg[256] = "";

  switch (err)
  {
  case IPARSE_SYMBEXIST:
    sprintf(msg, "handle name '%s' already exists", s);
    break;
  case IPARSE_SYMBNOTDEF:
    sprintf(msg, "handle name '%s' not defined", s);
    break;
  }

  return iupLexError(IUPLEX_ERR_PARSE, msg);
}
