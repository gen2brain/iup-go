#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_func.h"
#include "iup_layout.h"
#include "iup_childtree.h"
#include "iup_array.h"



static char* iStrGetNoReserved(const char* p_name)
{
  static char name[128];
  strcpy(name, p_name);
  iupStrReplaceReserved(name, '_');
  return name;
}

static int iExportHasReserved(const char* name, int check_num)
{
  char c;

  if (*name == 0)  /* empty string must be in quotes */
    return 1;

  c = *name;
  if (check_num && c >= '1' && c <= '9') /* first character can NOT be a number */
    return 1;
  
  while (*name)
  {
    c = *name;

    /* can only has letters or numbers as characters, or underscore */
    if ( c < '0' || 
        (c > '9' && c < 'A') ||
        (c > 'Z' && c < 'a' && c != '_') ||
         c > 'z')
      return 1;

    name++;
  }

  return 0;
}

static void iExportWriteAttrib(FILE* file, const char* name, const char* value, const char* indent, int export_format)
{
  char attribname[1024];
  const char* old_value = value;
  value = iupStrConvertToC(value);

  if (export_format == IUP_LAYOUT_EXPORT_LUA)  /* Lua */
  {
    iupStrLower(attribname, name);
    if (iExportHasReserved(attribname, 1))
      fprintf(file, "%s[\"%s\"] = \"%s\",\n", indent, attribname, value);
    else
      fprintf(file, "%s%s = \"%s\",\n", indent, attribname, value);
  }
  else if (export_format == IUP_LAYOUT_EXPORT_LED) /* LED */
  {
    if (iExportHasReserved(name, 0))  /* can not be saved in LED */
    {
      if (old_value != value)
        free((char*)value);
      return;
    }

    iupStrUpper(attribname, name);
    if (iExportHasReserved(value, 0))
      fprintf(file, "%s=\"%s\", ", attribname, value);
    else
      fprintf(file, "%s=%s, ", attribname, value);
  }
  else   /* C */
    fprintf(file, "%s\"%s\", \"%s\",\n", indent, name, value);

  if (old_value != value)
    free((char*)value);
}

static int iExportCheckAttribExportedInConstructor(Ihandle* ih, const char* check_name)
{
  const char *format = ih->iclass->format;
  if (!format || format[0] == 0)
    return 0;
  else
  {
    int i, num_format = (int)strlen(format);

    for (i = 0; i < num_format; i++)
    {
      if (format[i] == 's' || format[i] == 'a')
      {
        const char* name = NULL;
        if (i == 0)
          name = ih->iclass->format_attr;
        if (!name)
        {
          if (format[i] == 'a')
            name = "ACTION";
          else
            name = "TITLE";
        }

        if (name && iupStrEqual(name, check_name))
          return 1;
      }
    }

    return 0;
  }
}

static void iExportSavedAttribs(FILE* file, Ihandle* ih, const char* indent, int export_format)
{
  int i, attr_count;
  char **attr_names;
  char localIndent[1024];
  int wcount = 0;

  strcpy(localIndent, indent);
  strcat(localIndent, "  ");

  attr_count = iupAttribGetAllSaved(ih, NULL, 0);
  attr_names = (char **)malloc(attr_count * sizeof(char *));

  attr_count = iupAttribGetAllSaved(ih, attr_names, attr_count);
  for (i = 0; i < attr_count; i++)
  {
    if (export_format == IUP_LAYOUT_EXPORT_LED && iExportCheckAttribExportedInConstructor(ih, attr_names[i]))
      continue;

    if (export_format != IUP_LAYOUT_EXPORT_LUA || 
        !iupClassObjectAttribIsCallback(ih, attr_names[i])) /* do NOT save callbacks in Lua */
    {
      char* value = iupAttribGetLocal(ih, attr_names[i]);  /* do NOT check for inherited values */
      if (value)
      {
        iExportWriteAttrib(file, attr_names[i], value, localIndent, export_format);
        wcount++;
      }
    }
  }

  if (export_format == IUP_LAYOUT_EXPORT_LED) /* LED */
  {
    if (wcount != 0)
      fseek(file, -2, SEEK_CUR);  /* remove last comma ',' and space */
    else
      fseek(file, -1, SEEK_CUR);/* remove "[" */

    if (wcount != 0)
      fprintf(file, "]"); /* end of attributes (no new line) */
  }

  free(attr_names);
}

static void iExportChangedAttribs(FILE* file, Ihandle* ih, const char* indent, int export_format)
{
  int i, wcount = 0, attr_count, has_attrib_id = ih->iclass->has_attrib_id, start_id = 0,
    total_count = IupGetClassAttributes(ih->iclass->name, NULL, 0);
  char **attr_names = (char **)malloc(total_count * sizeof(char *));

  if (IupClassMatch(ih, "tree") || /* tree can only set id attributes after map, so they can not be saved */
      IupClassMatch(ih, "cells"))  /* cells does not have any savable id attributes */
    has_attrib_id = 0;

  if (IupClassMatch(ih, "list") || IupClassMatch(ih, "flatlist"))
    start_id = 1;

  attr_count = IupGetClassAttributes(ih->iclass->name, attr_names, total_count);
  for (i = 0; i < attr_count; i++)
  {
    char *name = attr_names[i];
    char* value = iupAttribGetLocal(ih, name);  /* do NOT check for inherited values */
    char* def_value;
    int flags;

    iupClassGetAttribNameInfo(ih->iclass, name, &def_value, &flags);

    if (value && iupLayoutAttributeHasChanged(ih, name, value, def_value, flags))
    {
      char* str = iupStrConvertToC(value);

      iExportWriteAttrib(file, name, str, indent, export_format);
      wcount++;

      if (str != value)
        free(str);
    }

    if (has_attrib_id && flags&IUPAF_HAS_ID)
    {
      flags &= ~IUPAF_HAS_ID; /* clear flag so the next function call can work (it will check for read-only, write-only, no-string and no-save) */
      if (iupLayoutAttributeHasChanged(ih, name, "XXX", NULL, flags)) /* use a non null and non empty value just to pass the other tests */
      {
        if (iupStrEqual(name, "IDVALUE"))
          name = "";

        if (flags&IUPAF_HAS_ID2)
        {
          int lin, col,
            numcol = IupGetInt(ih, "NUMCOL") + 1,
            numlin = IupGetInt(ih, "NUMLIN") + 1;
          for (lin = 0; lin < numlin; lin++)
          {
            for (col = 0; col < numcol; col++)
            {
              value = IupGetAttributeId2(ih, name, lin, col);
              if (value && value[0] && !iupATTRIB_ISINTERNAL(value))
              {
                char str[50];
                sprintf(str, "%s%d:%d", name, lin, col);
                iExportWriteAttrib(file, str, value, indent, export_format);
                wcount++;
              }
            }
          }
        }
        else
        {
          int id, count = IupGetInt(ih, "COUNT");
          for (id = start_id; id < count + start_id; id++)
          {
            value = IupGetAttributeId(ih, name, id);
            if (value && value[0] && !iupATTRIB_ISINTERNAL(value))
            {
              char str[50];
              sprintf(str, "%s%d", name, id);
              iExportWriteAttrib(file, str, value, indent, export_format);
              wcount++;
            }
          }
        }
      }
    }
  }

  if (export_format != IUP_LAYOUT_EXPORT_LUA)  /* LED or C - additional step for callbacks */
  {
    int cb_count = total_count - attr_count;
    IupGetClassCallbacks(ih->iclass->name, attr_names, cb_count);
    for (i = 0; i < cb_count; i++)
    {
      char* cb_name = iupGetCallbackName(ih, attr_names[i]);
      if (cb_name && cb_name[0] && !iupATTRIB_ISINTERNAL(cb_name))
      {
        iExportWriteAttrib(file, attr_names[i], cb_name, indent, export_format);
        wcount++;
      }
    }
  }

  if (export_format == IUP_LAYOUT_EXPORT_LED ) /* LED */
  {
    if (wcount != 0)
      fseek(file, -2, SEEK_CUR);  /* remove last comma ',' and space */
    else
      fseek(file, -1, SEEK_CUR);/* remove "[" */

    if (wcount != 0)
      fprintf(file, "]"); /* end of attributes (no new line) */
  }

  free(attr_names);
}

static char* iExportGetName(Ihandle* ih)
{
  char* name = IupGetName(ih);
  if (name && iupATTRIB_ISINTERNAL(name))
    name = NULL;
  return name;
}

static void iExportElementLED(FILE* file, Ihandle* ih, const char* indent, int saved_info)
{
  int i, count;
  const char* format = ih->iclass->format;
  char classname[100];

  char* name = iExportGetName(ih);

  iupStrUpper(classname, ih->iclass->name);
  if (name)
    fprintf(file, "%s = %s[", iStrGetNoReserved(name), classname);  /* start of attributes */
  else
    fprintf(file, "%s%s[", indent, classname);

  if (saved_info)
    iExportSavedAttribs(file, ih, indent, IUP_LAYOUT_EXPORT_LED);
  else
    iExportChangedAttribs(file, ih, indent, IUP_LAYOUT_EXPORT_LED);

  if (!format || format[0] == 0)
    fprintf(file, "()");
  else
  {
    if (*format == 'g')  /* a multi child container (can only has 1 format) */
    {
      Ihandle *child;
      char localIndent[1024];

      strcpy(localIndent, indent);
      strcat(localIndent, "  ");

      fprintf(file, "(\n");

      if (ih->iclass->childtype == IUP_CHILDNONE) /* IupNormalizer exception */
      {
        child = (Ihandle*)IupGetAttribute(ih, "FIRST_CONTROL_HANDLE");
        while (child)
        {
          if (!(child->flags & IUP_INTERNAL))
          {
            char* childname = iExportGetName(child);
            if (!childname)
              iExportElementLED(file, child, localIndent, saved_info);   /* here process the ones that does NOT have names */
            else
              fprintf(file, "%s%s", localIndent, childname);

            child = (Ihandle*)IupGetAttribute(ih, "NEXT_CONTROL_HANDLE");

            if (child)
              fprintf(file, ",\n");
          }
          else
            child = (Ihandle*)IupGetAttribute(ih, "NEXT_CONTROL_HANDLE");
        }
      }
      else
      {
        for (child = ih->firstchild; child; child = child->brother)
        {
          if (!(child->flags & IUP_INTERNAL))
          {
            char* childname = iExportGetName(child);
            if (!childname)
              iExportElementLED(file, child, localIndent, saved_info);   /* here process the ones that does NOT have names */
            else
              fprintf(file, "%s%s", localIndent, childname);

            if (child->brother)
              fprintf(file, ",\n");
          }
        }
      }

      fprintf(file, "\n%s)", indent);
    }
    else
    {
      count = (int)strlen(format);

      fprintf(file, "(");

      for (i = 0; i < count; i++)
      {
        if (format[i] == 's')
        {
          char* value;
          const char* attribname = NULL;
          if (i == 0)
            attribname = ih->iclass->format_attr;
          if (!attribname)
            attribname = "TITLE";

          value = iupAttribGetLocal(ih, attribname);  /* do NOT check for inherited values */
          if (value)
            fprintf(file, "\"%s\"", value); /* always with quotes in constructor */
          else
            fprintf(file, "\"\"");  /* empty string */
        }
        else if (format[i] == 'a')
        {
          char* cb_name = NULL;
          if (i == 0 && ih->iclass->format_attr)
            cb_name = iupGetCallbackName(ih, ih->iclass->format_attr);
          if (!cb_name)
            cb_name = iupGetCallbackName(ih, "ACTION");

          if (cb_name && cb_name[0] && !iupATTRIB_ISINTERNAL(cb_name))
            fprintf(file, "%s", cb_name);
          else
            fprintf(file, "do_nothing");  /* dummy name */
        }
        else if (format[i] == 'h')
        {
          Ihandle *child;

          if (ih->iclass->childtype == IUP_CHILDNONE)  /* IupAnimatedLabel and IupDropButton exceptions */
          {
            child = (Ihandle*)IupGetAttribute(ih, "FIRST_CONTROL_HANDLE");
            while (child)
            {
              if (!(child->flags & IUP_INTERNAL))
              {
                char* childname = iExportGetName(child);
                if (!childname)
                {
                  char localIndent[1024];

                  strcpy(localIndent, indent);
                  strcat(localIndent, "  ");

                  fprintf(file, "\n");
                  iExportElementLED(file, child, localIndent, saved_info);   /* here process the ones that does NOT have names */
                }
                else
                  fprintf(file, "%s", childname);

                break; /* only one child */
              }

              child = (Ihandle*)IupGetAttribute(ih, "NEXT_CONTROL_HANDLE");
            }
          }
          else
          {
            for (child = ih->firstchild; child; child = child->brother)
            {
              if (!(child->flags & IUP_INTERNAL))
              {
                char* childname = iExportGetName(child);
                if (!childname)
                {
                  char localIndent[1024];

                  strcpy(localIndent, indent);
                  strcat(localIndent, "  ");

                  fprintf(file, "\n");
                  iExportElementLED(file, child, localIndent, saved_info);   /* here process the ones that does NOT have names */
                }
                else
                  fprintf(file, "%s", childname);

                break; /* only one child */
              }
            }
          }
        }

        if (i != count - 1)
          fprintf(file, ", ");
      }

      fprintf(file, ")");
    }
  }

  if (name)
    fprintf(file, "\n\n");
}

static void iExportElementLua(FILE* file, Ihandle* ih, const char *indent, int saved_info)
{
  Ihandle *child;
  char* name = iExportGetName(ih);

  if (name)
    fprintf(file, "%s_lc.%s = ", indent, iStrGetNoReserved(name));
  else
    fprintf(file, "%s", indent);

  fprintf(file, "iup.%s{\n", ih->iclass->name);

  if (ih->iclass->childtype != IUP_CHILDNONE)
  {
    char localIndent[1024];

    strcpy(localIndent, indent);
    strcat(localIndent, "  ");

    for (child = ih->firstchild; child; child = child->brother)
    {
      char* childName = iExportGetName(child);
      if (childName)
      {
        if (iupAttribGet(child, "_IUP_EXPORT_LUA_SAVED")) /* saved in the same scope */
          fprintf(file, "%s_lc.%s,\n", localIndent, childName);
        else
          fprintf(file, "%siup.GetHandle(\"%s\"),\n", localIndent, childName);
      }
      else
      {
        iExportElementLua(file, child, localIndent, saved_info);
        fprintf(file, ",\n");
      }
    }
  }
  else  /* childtype == IUP_CHILDNONE */
  {
    if (ih->iclass->format && (ih->iclass->format[0] == 'h' ||  /* IupAnimatedLabel and IupDropButton exceptions */
                               ih->iclass->format[0] == 'g'))    /* IupNormalizer exception */
    {
      char localIndent[1024];

      strcpy(localIndent, indent);
      strcat(localIndent, "  ");

      child = (Ihandle*)IupGetAttribute(ih, "FIRST_CONTROL_HANDLE");
      while (child)
      {
        char* childName = iExportGetName(child);
        if (childName)
        {
          if (iupAttribGet(child, "_IUP_EXPORT_LUA_SAVED")) /* saved in the same scope */
            fprintf(file, "%s_lc.%s,\n", localIndent, childName);
          else
            fprintf(file, "%siup.GetHandle(\"%s\"),\n", localIndent, childName);
        }
        else
        {
          iExportElementLua(file, child, localIndent, saved_info);
          fprintf(file, ",\n");
        }

        child = (Ihandle*)IupGetAttribute(ih, "NEXT_CONTROL_HANDLE");
      }
    }
  }

  if (saved_info)
    iExportSavedAttribs(file, ih, indent, IUP_LAYOUT_EXPORT_LUA);
  else
    iExportChangedAttribs(file, ih, indent, IUP_LAYOUT_EXPORT_LUA);

  fprintf(file, "%s}", indent);
}

static void iExportElementC(FILE* file, Ihandle* ih, const char *indent, const char* terminator, int saved_info)
{
  char* name = iExportGetName(ih);

  if (ih->iclass->childtype != IUP_CHILDNONE)
  {
    Ihandle *child;
    char localIndent[1024];

    if (name)
      fprintf(file, "%sIupSetAtt(\"%s\", IupCreatep(\"%s\", \n", indent, name, ih->iclass->name);
    else
      fprintf(file, "%sIupSetAtt(NULL, IupCreatep(\"%s\", \n", indent, ih->iclass->name);

    strcpy(localIndent, indent);
    strcat(localIndent, "    ");  /* indent twice for children */

    for (child = ih->firstchild; child; child = child->brother)
    {
      char* childName = iExportGetName(child);
      if (childName)
        fprintf(file, "%sIupGetHandle(\"%s\"),\n", localIndent, childName);
      else 
        iExportElementC(file, child, localIndent, ",", saved_info); /* no ; */
    }

    fprintf(file, "%sNULL),\n", localIndent);  /* IupCreatep */
  }
  else /* childtype == IUP_CHILDNONE */
  {
    if (ih->iclass->format && (ih->iclass->format[0] == 'h' ||  /* IupAnimatedLabel and IupDropButton exceptions */
                               ih->iclass->format[0] == 'g'))    /* IupNormalizer exception */
    {
      Ihandle *child;
      char localIndent[1024];

      if (name)
        fprintf(file, "%sIupSetAtt(\"%s\", IupCreatep(\"%s\", \n", indent, name, ih->iclass->name);
      else
        fprintf(file, "%sIupSetAtt(NULL, IupCreatep(\"%s\", \n", indent, ih->iclass->name);

      strcpy(localIndent, indent);
      strcat(localIndent, "  ");

      child = (Ihandle*)IupGetAttribute(ih, "FIRST_CONTROL_HANDLE");
      while (child)
      {
        char* childName = iExportGetName(child);
        if (childName)
          fprintf(file, "%sIupGetHandle(\"%s\"),\n", localIndent, childName);
        else
          iExportElementC(file, child, localIndent, ",", saved_info); /* no ; */

        child = (Ihandle*)IupGetAttribute(ih, "NEXT_CONTROL_HANDLE");
      }

      fprintf(file, "%sNULL),\n", localIndent);  /* IupCreatep */
    }
    else
    {
      if (name)
        fprintf(file, "%sIupSetAtt(\"%s\", IupCreate(\"%s\"), \n", indent, name, ih->iclass->name);
      else
        fprintf(file, "%sIupSetAtt(NULL, IupCreate(\"%s\"), \n", indent, ih->iclass->name);
    }
  }

  if (saved_info)
    iExportSavedAttribs(file, ih, indent, IUP_LAYOUT_EXPORT_C);
  else
    iExportChangedAttribs(file, ih, indent, IUP_LAYOUT_EXPORT_C);

  fprintf(file, "%s  NULL)%s\n", indent, terminator);  /* IupSetAtt */

  if (terminator[0] == ';')
    fprintf(file, "\n");
}

IUP_SDK_API void iupLayoutExportNamedElemList(FILE* file, Ihandle* *named_elem, int count, int export_format, int saved_info)
{
  int i, first = 1;
  Ihandle *elem;

  for (i = 0; i < count; i++)
  {
    elem = named_elem[i];

    if (elem->iclass->nativetype != IUP_TYPEIMAGE)
    {
      char* name = IupGetName(elem);

      if (export_format == IUP_LAYOUT_EXPORT_LUA)
      {
        if (first)
        {
          fprintf(file, "  local _lc = {}\n\n"); /* use a single local variable to avoid the 200 limit in Lua */
          first = 0;
        }

        iExportElementLua(file, elem, "  ", saved_info);
        fprintf(file, "\n");
      }
      else if (export_format == IUP_LAYOUT_EXPORT_C)
        iExportElementC(file, elem, "  ", ";", saved_info);
      else
        iExportElementLED(file, elem, "  ", saved_info);

      if (export_format == IUP_LAYOUT_EXPORT_LUA && name)
      {
        fprintf(file, "  iup.SetHandle(\"%s\", _lc.%s)\n\n", name, iStrGetNoReserved(name));
        iupAttribSet(elem, "_IUP_EXPORT_LUA_SAVED", "1");
      }
    }
  }

  if (export_format == IUP_LAYOUT_EXPORT_LUA)
  {
    for (i = 0; i < count; i++)
    {
      elem = named_elem[i];
      if (IupGetName(elem))
        iupAttribSet(elem, "_IUP_EXPORT_LUA_SAVED", NULL);
    }
  }
}

IUP_SDK_API void iupLayoutExportNamedImageList(FILE* file, Ihandle* *named_elem, int count, int export_format)
{
  int i;
  Ihandle *elem;

  for (i = 0; i < count; i++)
  {
    elem = named_elem[i];

    if (elem->iclass->nativetype == IUP_TYPEIMAGE)
    {
      char* format[] = { "LUA", "C", "LED" };
      char* name = IupGetName(elem);

      iupImageExportToFile(elem, file, format[export_format], name);

      if (export_format == IUP_LAYOUT_EXPORT_LUA && name)
        iupAttribSet(elem, "_IUP_EXPORT_LUA_SAVED", "1");
    }
  }
}

IUP_SDK_API void iupLayoutExportNamedImageListSetHandle(FILE* file, Ihandle* *named_elem, int count, int export_format)
{
  int i;
  Ihandle *elem;

  if (export_format == IUP_LAYOUT_EXPORT_LUA || export_format == IUP_LAYOUT_EXPORT_C)
  {
    for (i = 0; i < count; i++)
    {
      elem = named_elem[i];

      if (elem->iclass->nativetype == IUP_TYPEIMAGE)
      {
        char* name = IupGetName(elem);
        if (name)
        {
          if (export_format == IUP_LAYOUT_EXPORT_LUA)
            fprintf(file, "  iup.SetHandle(\"%s\", create_image_%s())\n", name, iStrGetNoReserved(name));
          else
            fprintf(file, "  IupSetHandle(\"%s\", create_image_%s());\n", name, iStrGetNoReserved(name));
        }
      }
    }
  }
}


/******************************************************************************/


static int iExportImagePrintBuffer(Iarray *buffer, const char *format, va_list arglist)
{
  char str[100];
  int count, len;
  char *data;

  count = iupArrayCount(buffer);

  len = vsprintf(str, format, arglist);

  if (len < 0)
    return len;

  iupArrayAdd(buffer, len);

  data = (char*)iupArrayGetData(buffer);

  memcpy(data + count, str, len);

  return len;
}

static int iExportImagePrint(FILE *file, Iarray *buffer, char *format, ...)
{
  int len;
  va_list arglist;
  va_start(arglist, format);

  if (file)
    len = vfprintf(file, format, arglist);
  else
    len = iExportImagePrintBuffer(buffer, format, arglist);

  va_end(arglist);

  return len;
}

static int iExportSaveImageC(const char* filename, Ihandle* ih, const char* name, FILE* packfile, Iarray* buffer)
{
  int y, x, width, height, channels, linesize;
  unsigned char* data;
  FILE* file = NULL;

  data = (unsigned char*)iupAttribGet(ih, "WID");
  if (!data)
    return 0;

  if (packfile)
    file = packfile;
  else if (filename)
    file = fopen(filename, "wb");

  if (!file && !buffer)
    return 0;

  width = IupGetInt(ih, "WIDTH");
  height = IupGetInt(ih, "HEIGHT");
  channels = IupGetInt(ih, "CHANNELS");
  linesize = width * channels;

  if (iExportImagePrint(file, buffer, "static Ihandle* create_image_%s(void)\n", name) < 0)
  {
    if (filename)
      fclose(file);
    return 0;
  }
  iExportImagePrint(file, buffer, "{\n");

  if (IupGetInt(NULL, "IMAGEEXPORT_STATIC"))
    iExportImagePrint(file, buffer, "  static unsigned char imgdata[] = {\n");
  else
    iExportImagePrint(file, buffer, "  unsigned char imgdata[] = {\n");

  for (y = 0; y < height; y++)
  {
    iExportImagePrint(file, buffer, "    ");

    for (x = 0; x < linesize; x++)
    {
      if (x != 0)
        iExportImagePrint(file, buffer, ", ");

      iExportImagePrint(file, buffer, "%d", (int)data[y*linesize + x]);
    }

    if (y == height - 1)
      iExportImagePrint(file, buffer, "};\n\n");
    else
      iExportImagePrint(file, buffer, ",\n");
  }

  if (channels == 1)
  {
    int c;
    char* color;

    iExportImagePrint(file, buffer, "  Ihandle* image = IupImage(%d, %d, imgdata);\n\n", width, height);

    for (c = 0; c < 256; c++)
    {
      color = IupGetAttributeId(ih, "", c);
      if (!color)
        break;

      iExportImagePrint(file, buffer, "  IupSetAttribute(image, \"%d\", \"%s\");\n", c, color);
    }

    iExportImagePrint(file, buffer, "\n");
  }
  else if (channels == 3)
    iExportImagePrint(file, buffer, "  Ihandle* image = IupImageRGB(%d, %d, imgdata);\n", width, height);
  else /* channels == 4 */
    iExportImagePrint(file, buffer, "  Ihandle* image = IupImageRGBA(%d, %d, imgdata);\n", width, height);

  iExportImagePrint(file, buffer, "  return image;\n");

  iExportImagePrint(file, buffer, "}\n\n");

  if (filename)
    fclose(file);

  return 1;
}

static int iExportSaveImageLua(const char* filename, Ihandle* ih, const char* name, FILE* packfile, Iarray* buffer)
{
  int y, x, width, height, channels, linesize;
  unsigned char* data;
  FILE* file = NULL;

  data = (unsigned char*)iupAttribGet(ih, "WID");
  if (!data)
    return 0;

  if (packfile)
    file = packfile;
  else if (filename)
    file = fopen(filename, "wb");

  if (!file && !buffer)
    return 0;

  width = IupGetInt(ih, "WIDTH");
  height = IupGetInt(ih, "HEIGHT");
  channels = IupGetInt(ih, "CHANNELS");
  linesize = width * channels;

  if (iExportImagePrint(file, buffer, "function create_image_%s()\n", name) < 0)
  {
    if (!packfile)
      fclose(file);
    return 0;
  }

  if (channels == 1)
    iExportImagePrint(file, buffer, "  local %s = iup.image{\n", name);
  else if (channels == 3)
    iExportImagePrint(file, buffer, "  local %s = iup.imagergb{\n", name);
  else /* channels == 4 */
    iExportImagePrint(file, buffer, "  local %s = iup.imagergba{\n", name);

  iExportImagePrint(file, buffer, "    width = %d,\n", width);
  iExportImagePrint(file, buffer, "    height = %d,\n", height);
  iExportImagePrint(file, buffer, "    pixels = {\n");

  for (y = 0; y < height; y++)
  {
    iExportImagePrint(file, buffer, "      ");
    for (x = 0; x < linesize; x++)
    {
      iExportImagePrint(file, buffer, "%d, ", (int)data[y*linesize + x]);
    }
    iExportImagePrint(file, buffer, "\n");
  }

  iExportImagePrint(file, buffer, "    },\n");

  if (channels == 1)
  {
    int c;
    char* color;
    unsigned char r, g, b;

    iExportImagePrint(file, buffer, "    colors = {\n");

    for (c = 0; c < 256; c++)
    {
      color = IupGetAttributeId(ih, "", c);
      if (!color)
        break;

      /* don't use index, this Lua constructor assumes index=0 the first item in the array */
      if (iupStrEqualNoCase(color, "BGCOLOR"))
        iExportImagePrint(file, buffer, "      \"BGCOLOR\",\n"); 
      else
      {
        iupStrToRGB(color, &r, &g, &b);
        iExportImagePrint(file, buffer, "      \"%d %d %d\",\n", (int)r, (int)g, (int)b);
      }
    }

    iExportImagePrint(file, buffer, "    }\n");
  }

  iExportImagePrint(file, buffer, "  }\n");

  iExportImagePrint(file, buffer, "  return %s\n", name);
  iExportImagePrint(file, buffer, "end\n\n");

  if (filename)
    fclose(file);

  return 1;
}

static int iExportSaveImageLED(const char* filename, Ihandle* ih, const char* name, FILE* packfile, Iarray* buffer)
{
  int y, x, width, height, channels, linesize;
  unsigned char* data;
  FILE* file = NULL;

  data = (unsigned char*)iupAttribGet(ih, "WID");
  if (!data)
    return 0;

  if (packfile)
    file = packfile;
  else if (filename)
    file = fopen(filename, "wb");

  if (!file && !buffer)
    return 0;

  width = IupGetInt(ih, "WIDTH");
  height = IupGetInt(ih, "HEIGHT");
  channels = IupGetInt(ih, "CHANNELS");
  linesize = width * channels;

  if (channels == 1)
  {
    int c;
    unsigned char r, g, b;
    char* color;

    if (iExportImagePrint(file, buffer, "%s = IMAGE[\n", name) < 0)
    {
      if (filename)
        fclose(file);
      return 0;
    }

    for (c = 0; c < 256; c++)
    {
      color = IupGetAttributeId(ih, "", c);
      if (!color)
      {
        if (c < 16)
          continue;
        else
          break;
      }

      if (c != 0)
        iExportImagePrint(file, buffer, ",\n");

      if (iupStrEqualNoCase(color, "BGCOLOR"))
        iExportImagePrint(file, buffer, "  %d = \"BGCOLOR\"", c);
      else
      {
        iupStrToRGB(color, &r, &g, &b);
        iExportImagePrint(file, buffer, "  %d = \"%d %d %d\"", c, (int)r, (int)g, (int)b);
      }
    }
    iExportImagePrint(file, buffer, "]\n  ");
  }
  else if (channels == 3)
  {
    if (iExportImagePrint(file, buffer, "%s = IMAGERGB", name) < 0)
    {
      if (filename)
        fclose(file);
      return 0;
    }
  }
  else /* channels == 4 */
  {
    if (iExportImagePrint(file, buffer, "%s = IMAGERGBA", name) < 0)
    {
      if (filename)
        fclose(file);
      return 0;
    }
  }

  iExportImagePrint(file, buffer, "(%d, %d,\n", width, height);

  for (y = 0; y < height; y++)
  {
    iExportImagePrint(file, buffer, "  ");
    for (x = 0; x < linesize; x++)
    {
      if (y == height - 1 && x == linesize - 1)
        iExportImagePrint(file, buffer, "%d", (int)data[y*linesize + x]);
      else
        iExportImagePrint(file, buffer, "%d, ", (int)data[y*linesize + x]);
    }
    iExportImagePrint(file, buffer, "\n");
  }

  iExportImagePrint(file, buffer, ")\n\n");

  if (filename)
    fclose(file);

  return 1;
}

IUP_API int IupSaveImageAsText(Ihandle* ih, const char* filename, const char* format, const char* p_name)
{
  int ret = 0;
  char name[128];

  if (!p_name)
  {
    p_name = IupGetName(ih);
    if (!p_name)
      p_name = "image";
  }

  strcpy(name, p_name);
  iupStrReplaceReserved(name, '_');

  if (iupStrEqualNoCase(format, "LED"))
    ret = iExportSaveImageLED(filename, ih, name, NULL, NULL);
  else if (iupStrEqualNoCase(format, "LUA"))
    ret = iExportSaveImageLua(filename, ih, name, NULL, NULL);
  else if (iupStrEqualNoCase(format, "C"))
    ret = iExportSaveImageC(filename, ih, name, NULL, NULL);
  return ret;
}

IUP_SDK_API int iupImageExportToFile(Ihandle* ih, FILE* packfile, const char* format, const char* p_name)
{
  int ret = 0;
  char name[128];

  if (!p_name)
  {
    p_name = IupGetName(ih);
    if (!p_name)
      p_name = "image";
  }

  strcpy(name, p_name);
  iupStrReplaceReserved(name, '_');

  if (iupStrEqualNoCase(format, "LED"))
    ret = iExportSaveImageLED(NULL, ih, name, packfile, NULL);
  else if (iupStrEqualNoCase(format, "LUA"))
    ret = iExportSaveImageLua(NULL, ih, name, packfile, NULL);
  else if (iupStrEqualNoCase(format, "C"))
    ret = iExportSaveImageC(NULL, ih, name, packfile, NULL);
  return ret;
}

IUP_SDK_API int iupImageExportToString(Ihandle* ih, char **str, const char* format, const char* p_name)
{
  Iarray *buffer = iupArrayCreate(1024, sizeof(char));
  int ret = 0, count;
  char name[128];

  if (!p_name)
  {
    p_name = IupGetName(ih);
    if (!p_name)
      p_name = "image";
  }

  strcpy(name, p_name);
  iupStrReplaceReserved(name, '_');

  if (iupStrEqualNoCase(format, "LED"))
    ret = iExportSaveImageLED(NULL, ih, name, NULL, buffer);
  else if (iupStrEqualNoCase(format, "LUA"))
    ret = iExportSaveImageLua(NULL, ih, name, NULL, buffer);
  else if (iupStrEqualNoCase(format, "C"))
    ret = iExportSaveImageC(NULL, ih, name, NULL, buffer);

  count = iupArrayCount(buffer);
  *str = iupArrayInc(buffer);
  (*str)[count] = 0;

  *str = iupArrayReleaseData(buffer);

  iupArrayDestroy(buffer);

  return ret;
}

