/** \file
 * \brief IupFontDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Xm/Xm.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_dialog.h"
#include "iup_strmessage.h"
#include "iup_childtree.h"

#include "iupmot_drv.h"

static int motFontDlgCompareStr(const void *a, const void *b)
{
  /* skip foundry */
  char* f1 = *(char**)a;
  char* f2 = *(char**)b;
  f1++; /* skip first '-' */
  f1 = strchr(f1, '-')+1; /* skip second '-' */
  f2++; /* skip first '-' */
  f2 = strchr(f2, '-')+1; /* skip second '-' */
  return strcmp(f1 , f2);
}

static void motFontGetFamily(const char* font_str, char* family)
{
  int len;
  font_str++; /* skip first '-' */
  font_str = strchr(font_str, '-')+1; /* skip second '-' */
  len = (int)(strchr(font_str, '-') - font_str); /* put terminator on third */
  memcpy(family, font_str, len);
  family[len] = 0;
}

static void motFontDlgInitFamilyList(Ihandle* ih)
{
  int i, j = 1, count = 0;
  char prev[1024]="", family[1024];
  Ihandle* list1 = IupGetDialogChild(ih, "LIST1");            /* this will reduce the initial number */
  char**font_list_str = XListFonts(iupmot_display, "-*-*-medium-r-*-*-0-0-*-*-*-*-*-*", 32767, &count);
  char*backup_list[32767];

  if (!font_list_str)
    return;

  memcpy(backup_list, font_list_str, count*sizeof(char*));
  qsort(font_list_str, count, sizeof(char*), motFontDlgCompareStr);

  for (i=0; i<count; i++)
  {
    motFontGetFamily(font_list_str[i], family);

    if (!iupStrEqual(family, prev))  /* avoid duplicates */
    {
      IupStoreAttributeId(list1, "", j, family);
      strcpy(prev, family);
      j++;
    }
  }
  IupSetAttributeId(list1, "", j, NULL);

  memcpy(font_list_str, backup_list, count*sizeof(char*));
  XFreeFontNames(font_list_str);
}

static int motGetFontSize(const char* font_name)
{
  int i = 0;
  while (i < 8)
  {
    font_name = strchr(font_name, '-')+1;
    i++;
  }

  *(strchr(font_name, '-')) = 0;
  return atoi(font_name)/10;  /* deci-points to points */
}

static int motFontDlgCompareSize(const void *a, const void *b)
{
  int sz1 = *((int*)a);
  int sz2 = *((int*)b);
  return sz1-sz2;
}

static void motFontDlgInitSizeList(Ihandle* ih, const char* fontface, int size)
{
  int i, j = 1, selected=-1, count = 0, sz, prev_sz = -1;
  char pattern[1024];
  Ihandle* list3 = IupGetDialogChild(ih, "LIST3");
  int size_list[32767];
  char**font_list_str;

  sprintf(pattern,"-*-%s-medium-r-*-*-*-*-*-*-*-*-*-*", fontface);
  font_list_str = XListFonts(iupmot_display, pattern, 32767, &count);
  if (!font_list_str)
    return;

  for (i=0; i<count; i++)
    size_list[i] = motGetFontSize(font_list_str[i]);

  qsort(size_list, count, sizeof(int), motFontDlgCompareSize);

  for (i=0; i<count; i++)
  {
    sz = size_list[i];
    if (sz && sz != prev_sz)  /* avoid duplicates, and zero */
    {
      if (sz == size)
        selected = j;
      IupSetIntId(list3, "", j, sz);
      prev_sz = sz;
      j++;
    }
  }
  IupSetAttributeId(list3, "", j, NULL);

  if (selected != -1)
  {
    IupSetInt(list3, "VALUE", selected);
    IupSetInt(list3, "TOPITEM", selected);
  }

  XFreeFontNames(font_list_str);
}

static void motFontDlgSelectFontFace(Ihandle* ih, char* fontface, int select)
{
  Ihandle* sample = IupGetDialogChild(ih, "SAMPLE");
  char* style = IupGetAttribute(sample, "FONTSTYLE");
  int size = IupGetInt(sample, "FONTSIZE");
  Ihandle* list1 = IupGetDialogChild(ih, "LIST1");
  Ihandle* list2 = IupGetDialogChild(ih, "LIST2");
  int is_bold, is_italic;

  motFontDlgInitSizeList(ih, fontface, size);

  if (select)
  {
    /* find the fontface in the list and select it */
    int i, count = IupGetInt(list1, "COUNT");
    for (i=0; i<count; i++)
    {
      if (iupStrEqualNoCase(fontface, IupGetAttributeId(list1, "", i+1)))
      {
        IupSetInt(list1, "VALUE", i+1);
        IupSetInt(list1, "TOPITEM", i+1);
        break;
      }
    }
  }

  is_bold = strstr(style, "Bold")!=NULL;
  is_italic = strstr(style, "Italic")!=NULL;
  if (is_bold && is_italic)
    IupSetAttribute(list2, "VALUE", "4");
  else if (is_italic)
    IupSetAttribute(list2, "VALUE", "3");
  else if (is_bold)
    IupSetAttribute(list2, "VALUE", "2");
  else 
    IupSetAttribute(list2, "VALUE", "1");
}

static int motFontDlgButtonOK_CB(Ihandle* ih)
{
  Ihandle* sample = IupGetDialogChild(ih, "SAMPLE");
  iupAttribSetStr(IupGetDialog(ih), "VALUE", IupGetAttribute(sample, "FONT"));
  iupAttribSet(IupGetDialog(ih), "STATUS", "1");
  return IUP_CLOSE;
}

static int motFontDlgButtonCancel_CB(Ihandle* ih)
{
  iupAttribSet(IupGetDialog(ih), "STATUS", NULL);
  return IUP_CLOSE;
}

static int motFontDlgButtonHelp_CB(Ihandle* ih)
{
  Icallback cb = IupGetCallback(IupGetDialog(ih), "HELP_CB");
  if (cb && cb(ih) == IUP_CLOSE)
  {
    iupAttribSet(IupGetDialog(ih), "STATUS", NULL);
    return IUP_CLOSE;
  }
  return IUP_DEFAULT;
}

static int motFontDlgShow_CB(Ihandle* ih, int state)
{
  if (state == IUP_SHOW)
  {
    Ihandle* sample = IupGetDialogChild(ih, "SAMPLE");
    char* value = iupAttribGet(ih, "PREVIEWTEXT");
    if (value)
      IupStoreAttribute(sample, "TITLE", value);

    value = iupAttribGet(ih, "VALUE");
    if (value)
    {
      IupStoreAttribute(sample, "FONT", value);
      value = IupGetAttribute(sample, "FONTFACE");

      motFontDlgSelectFontFace(ih, value, 1);
    }

    if (!IupGetCallback(ih, "HELP_CB"))
    {
      Ihandle* help_bt = IupGetDialogChild(ih, "HELPBUT");
      IupSetAttribute(help_bt, "VISIBLE", "NO");
    }
  }
  return IUP_DEFAULT;
}

static int motFontDlgList1_CB(Ihandle *list1, char *name, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* sample = IupGetDialogChild(list1, "SAMPLE");
    char* style = IupGetAttribute(sample, "FONTSTYLE");
    char* size = IupGetAttribute(sample, "FONTSIZE");
    IupSetfAttribute(sample, "FONT", "%s, %s %s", name, style, size);

    motFontDlgSelectFontFace(IupGetDialog(list1), name, 0);
  }
  return IUP_DEFAULT;
}

static int motFontDlgList2_CB(Ihandle *list2, char *name, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* sample = IupGetDialogChild(list2, "SAMPLE");
    IupStoreAttribute(sample, "FONTSTYLE", name);
  }
  return IUP_DEFAULT;
}

static int motFontDlgList3_CB(Ihandle *list3, char *name, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* sample = IupGetDialogChild(list3, "SAMPLE");
    IupStoreAttribute(sample, "FONTSIZE", name);
  }
  return IUP_DEFAULT;
}

static int motFontDlgMapMethod(Ihandle* ih)
{
  motFontDlgInitFamilyList(ih);
  return IUP_NOERROR;
}

static int motFontDlgCreateMethod(Ihandle* ih, void** params)
{
  Ihandle *ok_bt, *cancel_bt, *help_bt;
  Ihandle *lin1, *lin2, *list1, *list2, *list3;

  ok_bt = IupButton("_@IUP_OK", NULL);
  IupSetStrAttribute(ok_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback (ok_bt, "ACTION", (Icallback)motFontDlgButtonOK_CB);
  IupSetAttributeHandle(ih, "DEFAULTENTER", ok_bt);

  cancel_bt = IupButton("_@IUP_CANCEL", NULL);
  IupSetStrAttribute(cancel_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback (cancel_bt, "ACTION", (Icallback)motFontDlgButtonCancel_CB);
  IupSetAttributeHandle(ih, "DEFAULTESC", cancel_bt);

  help_bt = IupButton("_@IUP_HELP", NULL);            /* Help Button */
  IupSetStrAttribute(help_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetAttribute(help_bt, "NAME", "HELPBUT");
  IupSetCallback (help_bt, "ACTION", (Icallback)motFontDlgButtonHelp_CB);

  list1 = IupList(NULL);
  IupSetAttribute(list1, "EXPAND", "YES");
  IupSetCallback(list1, "ACTION", (Icallback)motFontDlgList1_CB);
  IupSetAttribute(list1, "VISIBLELINES", "8");
  IupSetAttribute(list1, "VISIBLECOLUMNS", "20");
  IupSetAttribute(list1, "NAME", "LIST1");
  IupSetAttribute(list1, "SORT", "YES");
  
  list2 = IupList(NULL);
  IupSetAttribute(list2, "EXPAND", "VERTICAL");
  IupSetCallback(list2, "ACTION", (Icallback)motFontDlgList2_CB);
  IupSetAttribute(list2, "VISIBLELINES", "8");
  IupSetAttribute(list2, "VISIBLECOLUMNS", "11");
  IupSetAttribute(list2, "NAME", "LIST2");
  IupSetAttribute(list2, "1", "Normal");
  IupSetAttribute(list2, "2", "Bold");
  IupSetAttribute(list2, "3", "Italic");
  IupSetAttribute(list2, "4", "Bold Italic");
  IupSetAttribute(list2, "5", NULL);
  
  list3 = IupList(NULL);
  IupSetAttribute(list3, "EXPAND", "VERTICAL");
  IupSetCallback(list3, "ACTION", (Icallback)motFontDlgList3_CB);
  IupSetAttribute(list3, "VISIBLELINES", "8");
  IupSetAttribute(list3, "VISIBLECOLUMNS", "11");
  IupSetAttribute(list3, "NAME", "LIST3");
  
  lin1 = IupHbox(
           IupSetAttributes(IupVbox(IupLabel("_@IUP_FAMILY"), list1, NULL), "GAP=0"),
           IupSetAttributes(IupVbox(IupLabel("_@IUP_STYLE"), list2, NULL), "GAP=0"),
           IupSetAttributes(IupVbox(IupLabel("_@IUP_SIZE"), list3, NULL), "GAP=0"),
           NULL);
  IupSetAttribute(lin1, "GAP", "10");
  IupSetAttribute(lin1, "MARGIN", "0x0");

  lin2 = IupHbox(IupFill(), ok_bt, cancel_bt, help_bt, NULL);
  IupSetAttribute(lin2, "GAP", "5");
  IupSetAttribute(lin2, "MARGIN", "0x0");
  IupSetAttribute(lin2, "NORMALIZESIZE", "HORIZONTAL");

  /* Do not use IupAppend because we set childtype=IUP_CHILDNONE */
  iupChildTreeAppend(ih, IupSetAttributes(IupVbox(lin1, 
                      IupSetAttributes(IupVbox(
                        IupLabel("_@IUP_SAMPLE"), 
                        IupFrame(IupSetAttributes(IupLabel(NULL), "NAME=SAMPLE, TITLE=\"abcdefghijk ABCDEFGHIJK\", EXPAND=HORIZONTAL, RASTERSIZE=x40")), 
                        NULL), "MARGIN=0x0, GAP=0"),
                      IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"), 
                      lin2, 
                      NULL), "MARGIN=10x10, GAP=10"));

  IupSetCallback(ih, "SHOW_CB", (Icallback)motFontDlgShow_CB);

  (void)params;
  return IUP_NOERROR;
}

void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->Create = motFontDlgCreateMethod;
  ic->Map = motFontDlgMapMethod;
}
