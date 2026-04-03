/** \file
 * \brief IupFontDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_drvfont.h"
#include "iup_childtree.h"


IUP_API Ihandle* IupFontDlg(void)
{
  return IupCreate("fontdlg");
}

/******************************************************************************
 * Custom Font Dialog (used by drivers without a native font dialog)
 *****************************************************************************/

static void iFontDlgInitFamilyList(Ihandle* ih)
{
  Ihandle* list1 = IupGetDialogChild(ih, "LIST1");
  char** families;
  int i, count;

  if (!list1) return;

  count = iupdrvFontGetFamilyList(&families);
  if (count == 0) return;

  for (i = 0; i < count; i++)
  {
    IupStoreAttributeId(list1, "", i + 1, families[i]);
    free(families[i]);
  }
  IupSetAttributeId(list1, "", count + 1, NULL);

  free(families);
}

static void iFontDlgInitSizeList(Ihandle* ih)
{
  Ihandle* list3 = IupGetDialogChild(ih, "LIST3");
  static int common_sizes[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 };
  int i, count = sizeof(common_sizes) / sizeof(common_sizes[0]);

  if (!list3) return;

  for (i = 0; i < count; i++)
    IupSetIntId(list3, "", i + 1, common_sizes[i]);
  IupSetAttributeId(list3, "", count + 1, NULL);
}

static int iFontDlgList1_CB(Ihandle* list1, char* name, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* sample = IupGetDialogChild(list1, "SAMPLE");
    char* style = IupGetAttribute(sample, "FONTSTYLE");
    char* size = IupGetAttribute(sample, "FONTSIZE");
    IupSetfAttribute(sample, "FONT", "%s, %s %s", name, style, size);
  }
  return IUP_DEFAULT;
}

static int iFontDlgList2_CB(Ihandle* list2, char* name, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* sample = IupGetDialogChild(list2, "SAMPLE");
    IupStoreAttribute(sample, "FONTSTYLE", name);
  }
  return IUP_DEFAULT;
}

static int iFontDlgList3_CB(Ihandle* list3, char* name, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* sample = IupGetDialogChild(list3, "SAMPLE");
    IupStoreAttribute(sample, "FONTSIZE", name);
  }
  return IUP_DEFAULT;
}

static int iFontDlgSizeEdit_CB(Ihandle* list3, int c, char* new_value)
{
  (void)c;
  if (new_value && new_value[0])
  {
    int size = 0;
    if (iupStrToInt(new_value, &size) && size > 0)
    {
      Ihandle* sample = IupGetDialogChild(list3, "SAMPLE");
      IupStoreAttribute(sample, "FONTSIZE", new_value);
    }
  }
  return IUP_DEFAULT;
}

static int iFontDlgButtonOK_CB(Ihandle* self)
{
  Ihandle* dlg = IupGetDialog(self);
  Ihandle* sample = IupGetDialogChild(dlg, "SAMPLE");
  const char* font = IupGetAttribute(sample, "FONT");

  iupAttribSetStr(dlg, "VALUE", font);
  iupAttribSet(dlg, "STATUS", "1");

  return IUP_CLOSE;
}

static int iFontDlgButtonCancel_CB(Ihandle* self)
{
  Ihandle* dlg = IupGetDialog(self);
  iupAttribSet(dlg, "STATUS", "0");
  return IUP_CLOSE;
}

static int iFontDlgButtonHelp_CB(Ihandle* self)
{
  Icallback cb = IupGetCallback(IupGetDialog(self), "HELP_CB");
  if (cb && cb(IupGetDialog(self)) == IUP_CLOSE)
    return IUP_CLOSE;
  return IUP_DEFAULT;
}

static int iFontDlgShow_CB(Ihandle* ih, int state)
{
  if (state == IUP_SHOW)
  {
    Ihandle* sample = IupGetDialogChild(ih, "SAMPLE");
    Ihandle* list1 = IupGetDialogChild(ih, "LIST1");
    Ihandle* list2 = IupGetDialogChild(ih, "LIST2");
    Ihandle* list3 = IupGetDialogChild(ih, "LIST3");

    char* previewtext = iupAttribGet(ih, "PREVIEWTEXT");
    if (previewtext)
      IupStoreAttribute(sample, "TITLE", previewtext);

    {
      const char* value = iupAttribGet(ih, "VALUE");
      char fontface[256] = "";
      char fontstyle[64] = "";
      int fontsize;
      const char* tmp;

      if (!value)
        value = IupGetGlobal("DEFAULTFONT");

      IupStoreAttribute(sample, "FONT", value);

      tmp = IupGetAttribute(sample, "FONTFACE");
      if (tmp) iupStrCopyN(fontface, sizeof(fontface), tmp);
      tmp = IupGetAttribute(sample, "FONTSTYLE");
      if (tmp) iupStrCopyN(fontstyle, sizeof(fontstyle), tmp);
      fontsize = IupGetInt(sample, "FONTSIZE");

      if (fontface[0])
      {
        int i, count = IupGetInt(list1, "COUNT");
        IupStoreAttribute(list1, "VALUE", fontface);
        for (i = 1; i <= count; i++)
        {
          const char* item = IupGetAttributeId(list1, "", i);
          if (item && iupStrEqualNoCase(item, fontface))
          {
            IupSetInt(list1, "TOPITEM", i);
            break;
          }
        }
      }

      if (fontstyle[0])
      {
        if (iupStrEqualNoCase(fontstyle, "Bold Italic"))
          IupSetAttribute(list2, "VALUE", "Bold Italic");
        else if (iupStrEqualNoCase(fontstyle, "Bold"))
          IupSetAttribute(list2, "VALUE", "Bold");
        else if (iupStrEqualNoCase(fontstyle, "Italic"))
          IupSetAttribute(list2, "VALUE", "Italic");
        else
          IupSetAttribute(list2, "VALUE", "Normal");
      }
      else
        IupSetAttribute(list2, "VALUE", "Normal");

      if (fontsize > 0)
      {
        int i, count = IupGetInt(list3, "COUNT");
        IupSetInt(list3, "VALUE", fontsize);
        for (i = 1; i <= count; i++)
        {
          int sz = IupGetIntId(list3, "", i);
          if (sz == fontsize)
          {
            IupSetInt(list3, "TOPITEM", i);
            break;
          }
        }
      }
    }

    if (!IupGetCallback(ih, "HELP_CB"))
    {
      Ihandle* help_bt = IupGetDialogChild(ih, "HELPBUT");
      IupSetAttribute(help_bt, "VISIBLE", "NO");
    }

  }
  return IUP_DEFAULT;
}

static int iFontDlgMapMethod(Ihandle* ih)
{
  iFontDlgInitFamilyList(ih);
  iFontDlgInitSizeList(ih);
  return IUP_NOERROR;
}

static int iFontDlgCreateMethod(Ihandle* ih, void** params)
{
  Ihandle *ok_bt, *cancel_bt, *help_bt;
  Ihandle *lin1, *lin2, *list1, *list2, *list3;

  ok_bt = IupButton("_@IUP_OK", NULL);
  IupSetStrAttribute(ok_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(ok_bt, "ACTION", (Icallback)iFontDlgButtonOK_CB);
  IupSetAttributeHandle(ih, "DEFAULTENTER", ok_bt);

  cancel_bt = IupButton("_@IUP_CANCEL", NULL);
  IupSetStrAttribute(cancel_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(cancel_bt, "ACTION", (Icallback)iFontDlgButtonCancel_CB);
  IupSetAttributeHandle(ih, "DEFAULTESC", cancel_bt);

  help_bt = IupButton("_@IUP_HELP", NULL);
  IupSetStrAttribute(help_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetAttribute(help_bt, "NAME", "HELPBUT");
  IupSetCallback(help_bt, "ACTION", (Icallback)iFontDlgButtonHelp_CB);

  list1 = IupList(NULL);
  IupSetAttribute(list1, "EDITBOX", "YES");
  IupSetAttribute(list1, "READONLY", "YES");
  IupSetAttribute(list1, "EXPAND", "YES");
  IupSetCallback(list1, "ACTION", (Icallback)iFontDlgList1_CB);
  IupSetAttribute(list1, "VISIBLELINES", "5");
  IupSetAttribute(list1, "VISIBLECOLUMNS", "20");
  IupSetAttribute(list1, "NAME", "LIST1");
  IupSetAttribute(list1, "SORT", "YES");

  list2 = IupList(NULL);
  IupSetAttribute(list2, "EDITBOX", "YES");
  IupSetAttribute(list2, "READONLY", "YES");
  IupSetAttribute(list2, "EXPAND", "VERTICAL");
  IupSetCallback(list2, "ACTION", (Icallback)iFontDlgList2_CB);
  IupSetAttribute(list2, "VISIBLELINES", "5");
  IupSetAttribute(list2, "VISIBLECOLUMNS", "11");
  IupSetAttribute(list2, "NAME", "LIST2");
  IupSetAttribute(list2, "1", "Normal");
  IupSetAttribute(list2, "2", "Bold");
  IupSetAttribute(list2, "3", "Italic");
  IupSetAttribute(list2, "4", "Bold Italic");
  IupSetAttribute(list2, "5", NULL);

  list3 = IupList(NULL);
  IupSetAttribute(list3, "EDITBOX", "YES");
  IupSetAttribute(list3, "EXPAND", "VERTICAL");
  IupSetCallback(list3, "ACTION", (Icallback)iFontDlgList3_CB);
  IupSetCallback(list3, "EDIT_CB", (Icallback)iFontDlgSizeEdit_CB);
  IupSetAttribute(list3, "VISIBLELINES", "5");
  IupSetAttribute(list3, "VISIBLECOLUMNS", "6");
  IupSetAttribute(list3, "NAME", "LIST3");
  IupSetAttribute(list3, "MASKINT", "1:72");

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

  {
    Ihandle* sample = IupSetAttributes(IupLabel(NULL), "NAME=SAMPLE, TITLE=\"AaBbCcXxYyZz\", EXPAND=YES, RASTERSIZE=x60, ALIGNMENT=ACENTER:ACENTER");
    IupSetStrAttribute(sample, "BGCOLOR", IupGetGlobal("TXTBGCOLOR"));

    iupChildTreeAppend(ih, IupSetAttributes(IupVbox(lin1,
      IupSetAttributes(IupVbox(
        IupLabel("_@IUP_SAMPLE"),
        IupFrame(sample),
        NULL), "MARGIN=0x0, GAP=0"),
      IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"),
      lin2,
      NULL), "MARGIN=10x10, GAP=10"));
  }

  IupSetCallback(ih, "SHOW_CB", (Icallback)iFontDlgShow_CB);
  IupSetAttribute(ih, "SHRINK", "YES");

  (void)params;
  return IUP_NOERROR;
}

/******************************************************************************
 * Class Registration
 *****************************************************************************/

Iclass* iupFontDlgNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("dialog"));

  ic->name = "fontdlg";
  ic->cons = "FontDlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;

  ic->New = iupFontDlgNewClass;

  if (iupStrEqualNoCase(IupGetGlobal("DRIVER"), "Motif") ||
      iupStrEqualNoCase(IupGetGlobal("DRIVER"), "FLTK") ||
      iupStrEqualNoCase(IupGetGlobal("DRIVER"), "EFL") ||
      iupStrEqualNoCase(IupGetGlobal("DRIVER"), "WinUI"))
  {
    ic->Create = iFontDlgCreateMethod;
    ic->Map = iFontDlgMapMethod;

    iupClassRegisterAttribute(ic, "PREVIEWTEXT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  }
  else
  {
    /* reset not used native dialog methods */
    ic->parent->LayoutUpdate = NULL;
    ic->parent->SetChildrenPosition = NULL;
    ic->parent->Map = NULL;
    ic->parent->UnMap = NULL;

    iupdrvFontDlgInitClass(ic);
  }

  /* IupFontDialog only */
  iupClassRegisterAttribute(ic, "STATUS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "VALUE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  return ic;
}
