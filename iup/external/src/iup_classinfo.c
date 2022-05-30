#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "iup.h"

#include "iup_class.h"
#include "iup_register.h"
#include "iup_str.h"


static int compare_names(const void *a, const void *b)
{
  return strcmp( * ( char** ) a, * ( char** ) b );
}

static char* getClassParameters(const char* format, const char* format_attr)
{
  static char str[200], *pstr;
  pstr = &str[0];

  str[0] = 0;  /* clear for the case where there are no parameters */

  if (format && format[0]!=0)
  {
    int i, count = (int)strlen(format);
    if (count > 10) count = 10;

    for (i=0; i<count; i++)
    {
      char* fstr = NULL;

      switch(format[i])
      {
      case 'b': fstr = "unsigned char"; break;   /* unused */
      case 'j': fstr = "int*"; break;            /* unused */
      case 'f': fstr = "float"; break;           /* unused */
      case 'i': fstr = (i == 0)? "int w": "int h"; break;     /* used in IupImage* only */
      case 'c': fstr = "const unsigned char* pixels"; break;  /* used in IupImage* only */
      case 's': fstr = "const char* "; break;  /* usually is for TITLE, but depends on format_attr */
      case 'a': fstr = "const char* action"; break; /* name of the ACTION callback */
      case 'h': fstr = "Ihandle* ih"; break;
      case 'g': fstr = "Ihandle** ih_array"; break;  /* when used there are no other parameters */
      }

      if (fstr)
      {
        if (i!=0)
          pstr += sprintf(pstr, "%s", ", ");

        pstr += sprintf(pstr, "%s", fstr);

        if (format[i] == 's')
        {
          if (i == 0 && format_attr)
          {
            char attr[100];
            iupStrLower(attr, format_attr);
            pstr += sprintf(pstr, "%s", attr);
          }
          else
            pstr += sprintf(pstr, "%s", "title");
        }
      }
      else
      {
        IupMessagef("Internal Error:", "Invalid class format: %s", format);
        return NULL;
      }
    }
  }

  return str;
}

static char* getCallbackReturn(const char* format)
{
  char* fstr = NULL;

  switch(format[0])
  {
  case 'c': fstr = "unsigned char"; break;
  case 'i': fstr = "int"; break;
  case 'I': fstr = "int*"; break;
  case 'f': fstr = "float"; break;
  case 'd': fstr = "double"; break;
  case 's': fstr = "char*"; break;
  case 'V': fstr = "void*"; break;
  case 'C': fstr = "cdCanvas*"; break;
  case 'n': fstr = "Ihandle*"; break;
  }

  if (fstr)
    return fstr;
  else
  {
    IupMessagef("Internal Error:", "Invalid callback return format: %s", format);
    return NULL;
  }
}

static char* getCallbackParameters(const char* format)
{
  static char str[200], *pstr;
  pstr = &str[0];

  pstr += sprintf(pstr, "%s", "Ihandle*");  /* First parameter in all callbacks */

  if (format && format[0]!=0)
  {
    int i, count = (int)strlen(format);
    if (count > 10) count = 10;

    for (i=0; i<count; i++)
    {
      char* fstr = NULL;

      switch(format[i])
      {
      case 'c': fstr = "unsigned char"; break;
      case 'i': fstr = "int"; break;
      case 'I': fstr = "int*"; break;
      case 'f': fstr = "float"; break;
      case 'd': fstr = "double"; break;
      case 's': fstr = "char*"; break;
      case 'V': fstr = "void*"; break;
      case 'C': fstr = "cdCanvas*"; break;
      case 'n': fstr = "Ihandle*"; break;
      case '=': return str;
      }

      if (!fstr)
      {
        IupMessagef("Internal Error:", "Invalid callback format: %s", format);
        return NULL;
      }

      pstr += sprintf(pstr, ", %s", fstr);
    }
  }

  return str;
}

/* Update callback labels */
static int callbacksList_ActionCB (Ihandle *ih, char *callName, int pos, int state)
{
  if (state == 1)
  {
    Ihandle* listClasses = IupGetDialogChild(ih, "listClasses");
    Ihandle* txtInfo = IupGetDialogChild(ih, "txtInfo");
    char* className = IupGetAttribute(listClasses, IupGetAttribute(listClasses, "VALUE"));
    Iclass* ic = iupRegisterFindClass(className);
    char* format = iupClassCallbackGetFormat(ic, callName);
    const char* ret = strchr(format, '=');
    if (ret!=NULL)
      IupSetfAttribute(txtInfo, "VALUE", "%s %s(%s);", getCallbackReturn(ret + 1), callName, getCallbackParameters(format));
    else
      IupSetfAttribute(txtInfo, "VALUE", "int %s(%s);", callName, getCallbackParameters(format));
  }

  (void)pos;
  return IUP_DEFAULT;
}

/* Update attribute labels */
static int attributesList_ActionCB (Ihandle *ih, char *attribName, int pos, int state)
{
  if (state == 1)
  {
    Ihandle* listClasses = IupGetDialogChild(ih, "listClasses");
    Ihandle* txtInfo = IupGetDialogChild(ih, "txtInfo");
    char* className = IupGetAttribute(listClasses, IupGetAttribute(listClasses, "VALUE"));
    Iclass* ic = iupRegisterFindClass(className);

    if (iupClassAttribIsRegistered(ic, attribName))
    {
      char* def_value;
      int flags;
      iupClassGetAttribNameInfo(ic, attribName, &def_value, &flags);

      IupSetfAttribute(txtInfo, "VALUE", "Attribute Name: %s\n"
                                         "Default Value: %s\n"
                                         "Flags:\n"
                                         "%s"
                                         "%s"
                                         "%s"
                                         "%s"
                                         "%s"
                                         "%s",
                                         attribName,
                                         def_value==NULL? "NULL": def_value,
                                         flags&(IUPAF_NO_INHERIT|IUPAF_NO_STRING)? "  Is Inheritable\n": "  NON Inheritable\n",
                                         flags&IUPAF_NO_STRING? "  NOT a String\n": "",
                                         flags&IUPAF_HAS_ID? "  Has ID\n": "",
                                         flags&IUPAF_READONLY? "  Read-Only\n": (flags&IUPAF_WRITEONLY? "  Write-Only\n": ""),
                                         flags&IUPAF_IHANDLENAME? "  Ihandle* name\n": "",
                                         flags&IUPAF_NOT_SUPPORTED? "  NOT SUPPORTED in this driver\n": "");
    }
    else
      IupSetAttribute(txtInfo, "VALUE", "Custom Attribute");
  }

  (void)pos;
  return IUP_DEFAULT;
}

static char* getNativeType(InativeType nativetype)
{
  char* str[] = { "void", "control", "canvas", "dialog", "image", "menu", "other" }; 
  return str[nativetype];
}

static const char* getChildType(int childtype)
{
  if (childtype > IUP_CHILDMANY)
  {
    static char buf[100];
    sprintf(buf, "%d CHILDREN", childtype-IUP_CHILDMANY);
    return buf;
  }
  else
  {
    static const char * str[] = {"NO CHILD", "MANY CHILDREN"};
    return str[childtype];
  }
}

IUP_SDK_API void iupClassInfoGetDesc(Iclass* ic, Ihandle* ih, const char* attrib_name)
{
  char constructor[50];

  if (ic->cons)
    strcpy(constructor, ic->cons);
  else
  {
    strcpy(constructor, ic->name);
    constructor[0] = (char)toupper(constructor[0]);
  }

  IupSetfAttribute(ih, attrib_name, "Ihandle* Iup%s(%s);\n"
                   "Class Name: %s\n"
                   "Native Type: %s\n"
                   "Container Type: %s\n"
                   "Flags:\n"
                   "%s"
                   "%s",
                   constructor,
                   getClassParameters(ic->format, ic->format_attr),
                   ic->name,
                   getNativeType(ic->nativetype),
                   getChildType(ic->childtype),
                   ic->is_interactive ? "  Is Keyboard Interactive\n" : "  NOT Keyboard Interactive\n",
                   ic->has_attrib_id ? "  Has Id Attributes\n" : "");
}

static int classesList_ActionCB (Ihandle *ih, char *className, int pos, int state)
{
  if (state == 1)
  {
    Iclass* ic;
    int i, total_n, attr_n, cb_n;
    Ihandle* listAttributes = IupGetDialogChild(ih, "listAttributes");
    Ihandle* listCallbacks = IupGetDialogChild(ih, "listCallbacks");
    Ihandle* txtInfo = IupGetDialogChild(ih, "txtInfo");
    char **attr_names;
    
    total_n = IupGetClassAttributes(className, NULL, -1); /* total include callbacks */
    attr_names = (char **)malloc(total_n * sizeof(char *));

    /************ attributes ************/

    attr_n = IupGetClassAttributes(className, attr_names, total_n);
    qsort(attr_names, attr_n, sizeof(char*), compare_names);

    /* Clear lists */
    IupSetAttribute(listAttributes, "REMOVEITEM", NULL);

    /* Populate lists */
    for (i = 0; i < attr_n; i++)
      IupSetAttribute(listAttributes, "APPENDITEM", attr_names[i]);

    /************ callbacks ************/

    IupSetAttribute(listCallbacks, "REMOVEITEM", NULL);

    cb_n = IupGetClassCallbacks(className, attr_names, total_n);
    if (cb_n > 0)
    {
      qsort(attr_names, cb_n, sizeof(char*), compare_names);

      for (i = 0; i < cb_n; i++)
        IupSetAttribute(listCallbacks, "APPENDITEM", attr_names[i]);
    }

    /***********************************/

    ic = iupRegisterFindClass(className);

    iupClassInfoGetDesc(ic, txtInfo, "VALUE");

    free(attr_names);
  }

  (void)pos;
  return IUP_DEFAULT;
}

static void PopulateListOfClasses(Ihandle* ih)
{
  Ihandle* listClasses = IupGetDialogChild(ih, "listClasses");
  int i, num_classes;
  char **list;

  num_classes = IupGetAllClasses(NULL, -1);
  list = (char **)malloc(num_classes * sizeof(char *));
  IupGetAllClasses(list, num_classes);

  qsort(list, num_classes, sizeof(char*), compare_names);

  for (i = 0; i < num_classes; i++)
    IupSetAttribute(listClasses, "APPENDITEM", list[i]);

  free(list);
}

IUP_SDK_API void iupClassInfoShowHelp(const char* className)
{
  char url[1024];
  char* folder = "elem";
  char* sep = "";

  if (strstr(className, "dlg") || iupStrEqual(className, "dialog"))
    folder = "dlg";
  else if (iupStrEqualPartial(className, "matrix") ||
            iupStrEqualPartial(className, "mgl") ||
            iupStrEqual(className, "plot") ||
            iupStrEqual(className, "scintilla") ||
            iupStrEqual(className, "cells") ||
            iupStrEqual(className, "glbackgroundbox") ||
            iupStrEqual(className, "glcanvas") ||
            iupStrEqual(className, "olecontrol") ||
            iupStrEqual(className, "tuioclient") ||
            iupStrEqual(className, "webbrowser"))
            folder = "ctrl";
  else if (className[0] == 'g' && className[1] == 'l')
    folder = "gl";

  if (iupStrEqualPartial(className, "mgl") ||
      iupStrEqual(className, "plot") ||
      iupStrEqual(className, "scintilla"))
      sep = "_";

  /* filename fixes */
  if (iupStrEqualPartial(className, "imagergb"))
    className = "image";
  else if (iupStrEqual(className, "spinbox"))
    className = "spin";
  else if (iupStrEqual(className, "olecontrol"))
    className = "ole";
  else if (iupStrEqual(className, "tuioclient"))
    className = "tuio";
  else if (iupStrEqual(className, "webbrowser"))
    className = "web";

  /* sprintf(url, "http://www.tecgraf.puc-rio.br/iup/en/%s/iup%s%s.html", folder, sep, className); -- direct page version */
  sprintf(url, "http://www.tecgraf.puc-rio.br/iup/index.html?url=%s/iup%s%s.html", folder, sep, className);

  IupHelp(url);
}

static int button_help_CB(Ihandle* ih)
{
  Ihandle* listClasses = IupGetDialogChild(ih, "listClasses");
  char* className = IupGetAttribute(listClasses, IupGetAttribute(listClasses, "VALUE"));
  if (!className)
    IupMessageError(IupGetDialog(ih), "Select a class from the list first!");
  else
    iupClassInfoShowHelp(className);
  return IUP_DEFAULT;
}

static int button_ok_CB(Ihandle* ih)
{
  IupHide(IupGetDialog(ih));
  return IUP_DEFAULT;
}

IUP_API Ihandle* IupClassInfoDialog(Ihandle* parent)
{
  Ihandle *dialog, *box, *buttons, *listClasses, *listAttributes, *listCallbacks, *txtInfo, *ok_bt, *help_bt;
  
  listClasses    = IupList(NULL);  /* list of registered classes */
  listAttributes = IupList(NULL);  /* list of attributes of the selected class */
  listCallbacks  = IupList(NULL);  /* list of  callbacks of the selected class */

  IupSetAttributes(listClasses,    "NAME=listClasses, SIZE=70x85, EXPAND=VERTICAL");
  IupSetAttributes(listAttributes, "NAME=listAttributes, SIZE=120x85, EXPAND=VERTICAL");
  IupSetAttributes(listCallbacks,  "NAME=listCallbacks, SIZE=120x85, EXPAND=VERTICAL");

  IupSetCallback(listClasses,    "ACTION", (Icallback)    classesList_ActionCB);
  IupSetCallback(listAttributes, "ACTION", (Icallback) attributesList_ActionCB);
  IupSetCallback(listCallbacks,  "ACTION", (Icallback)  callbacksList_ActionCB);

  txtInfo = IupText(NULL);
  IupSetAttribute(txtInfo, "VISIBLELINES", "7");
  IupSetAttribute(txtInfo, "MULTILINE", "YES");
  IupSetAttribute(txtInfo, "SCROLLBAR", "NO");
  IupSetAttribute(txtInfo, "EXPAND", "HORIZONTAL");
  IupSetAttribute(txtInfo, "NAME", "txtInfo");

  ok_bt = IupButton("Close", NULL);
  IupSetStrAttribute(ok_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(ok_bt, "ACTION", (Icallback)button_ok_CB);

  help_bt = IupButton("Class Help", NULL);
  IupSetStrAttribute(help_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(help_bt, "ACTION", (Icallback)button_help_CB);

  buttons = IupHbox(IupFill(), ok_bt, help_bt, NULL);
  IupSetAttribute(buttons, "MARGIN", "0x0");
  IupSetAttribute(buttons, "NORMALIZESIZE", "HORIZONTAL");

  box = IupVbox(
            IupHbox(
              IupSetAttributes(IupFrame(IupVbox(listClasses,    NULL)), "TITLE=Classes:"),
              IupSetAttributes(IupFrame(IupVbox(listAttributes, NULL)), "TITLE=Attributes:"),
              IupSetAttributes(IupFrame(IupVbox(listCallbacks,  NULL)), "TITLE=Callbacks:"),
              NULL),
            IupHbox(
              IupSetAttributes(IupFrame(IupHbox(txtInfo, NULL)), "TITLE=Info:"),
              NULL),
            buttons,
            NULL);

	IupSetAttributes(box,"MARGIN=8x8, GAP=4");

  dialog = IupDialog(box);
  IupSetAttribute(dialog, "RESIZE", "NO");
  IupSetAttribute(dialog, "MAXBOX", "NO");
  IupSetAttribute(dialog, "MINBOX", "NO");
  IupSetAttributeHandle(dialog, "DEFAULTENTER", ok_bt);
  IupSetAttributeHandle(dialog, "DEFAULTESC", ok_bt);
  if (parent) IupSetAttributeHandle(dialog, "PARENTDIALOG", parent);

	IupSetAttribute(dialog, "TITLE", "Iup Classes Information");

  IupMap(dialog);

  PopulateListOfClasses(dialog);

  return dialog;
}
