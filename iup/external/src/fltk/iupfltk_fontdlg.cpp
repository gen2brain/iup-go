/** \file
 * \brief IupFontDlg pre-defined dialog - FLTK implementation
 *
 * Custom font dialog built with IUP controls. Enumerates system fonts
 * using Fl::set_fonts() / Fl::get_font_name().
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_dialog.h"
#include "iup_strmessage.h"
#include "iup_childtree.h"
#include "iup_drvfont.h"
}

#include "iupfltk_drv.h"


static void fltkFontDlgInitFamilyList(Ihandle* ih)
{
  Ihandle* list1 = IupGetDialogChild(ih, "LIST1");
  if (!list1) return;

  Fl_Font count = Fl::set_fonts(NULL);

  char prev[256] = "";
  int j = 1;

  for (Fl_Font i = 0; i < count; i++)
  {
    int attr = 0;
    const char* name = Fl::get_font_name(i, &attr);
    if (!name || !name[0]) continue;

    if (attr != 0) continue;

    if (iupStrEqual(name, prev)) continue;

    IupStoreAttributeId(list1, "", j, name);
    iupStrCopyN(prev, sizeof(prev), name);
    j++;
  }
  IupSetAttributeId(list1, "", j, NULL);
}

static void fltkFontDlgInitSizeList(Ihandle* ih)
{
  Ihandle* list3 = IupGetDialogChild(ih, "LIST3");
  if (!list3) return;

  static int common_sizes[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 };
  int count = sizeof(common_sizes) / sizeof(common_sizes[0]);

  for (int i = 0; i < count; i++)
    IupSetIntId(list3, "", i + 1, common_sizes[i]);
  IupSetAttributeId(list3, "", count + 1, NULL);
}

static int fltkFontDlgList1_CB(Ihandle* list1, char* name, int item, int state)
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

static int fltkFontDlgList2_CB(Ihandle* list2, char* name, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* sample = IupGetDialogChild(list2, "SAMPLE");
    IupStoreAttribute(sample, "FONTSTYLE", name);
  }
  return IUP_DEFAULT;
}

static int fltkFontDlgList3_CB(Ihandle* list3, char* name, int item, int state)
{
  (void)item;
  if (state)
  {
    Ihandle* sample = IupGetDialogChild(list3, "SAMPLE");
    IupStoreAttribute(sample, "FONTSIZE", name);
  }
  return IUP_DEFAULT;
}

static int fltkFontDlgButtonOK_CB(Ihandle* self)
{
  Ihandle* dlg = IupGetDialog(self);
  Ihandle* sample = IupGetDialogChild(dlg, "SAMPLE");
  const char* font = IupGetAttribute(sample, "FONT");

  iupAttribSetStr(dlg, "VALUE", font);
  iupAttribSet(dlg, "STATUS", "1");

  return IUP_CLOSE;
}

static int fltkFontDlgButtonCancel_CB(Ihandle* self)
{
  Ihandle* dlg = IupGetDialog(self);
  iupAttribSet(dlg, "STATUS", "0");
  return IUP_CLOSE;
}

static int fltkFontDlgButtonHelp_CB(Ihandle* self)
{
  Icallback cb = IupGetCallback(IupGetDialog(self), "HELP_CB");
  if (cb && cb(IupGetDialog(self)) == IUP_CLOSE)
    return IUP_CLOSE;
  return IUP_DEFAULT;
}

static int fltkFontDlgShow_CB(Ihandle* ih, int state)
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

    const char* value = iupAttribGet(ih, "VALUE");
    if (value)
    {
      IupStoreAttribute(sample, "FONT", value);

      const char* fontface = IupGetAttribute(sample, "FONTFACE");
      const char* fontstyle = IupGetAttribute(sample, "FONTSTYLE");
      int fontsize = IupGetInt(sample, "FONTSIZE");

      if (fontface)
      {
        int i, count = IupGetInt(list1, "COUNT");
        for (i = 1; i <= count; i++)
        {
          const char* item = IupGetAttributeId(list1, "", i);
          if (item && iupStrEqualNoCase(item, fontface))
          {
            IupSetInt(list1, "VALUE", i);
            IupSetInt(list1, "TOPITEM", i);
            break;
          }
        }
      }

      if (fontstyle)
      {
        if (iupStrEqualNoCase(fontstyle, "Bold Italic"))
          IupSetAttribute(list2, "VALUE", "4");
        else if (iupStrEqualNoCase(fontstyle, "Bold"))
          IupSetAttribute(list2, "VALUE", "2");
        else if (iupStrEqualNoCase(fontstyle, "Italic"))
          IupSetAttribute(list2, "VALUE", "3");
        else
          IupSetAttribute(list2, "VALUE", "1");
      }

      if (fontsize > 0)
      {
        int i, count = IupGetInt(list3, "COUNT");
        for (i = 1; i <= count; i++)
        {
          int sz = IupGetIntId(list3, "", i);
          if (sz == fontsize)
          {
            IupSetInt(list3, "VALUE", i);
            IupSetInt(list3, "TOPITEM", i);
            break;
          }
        }
      }
    }

    Ihandle* help_bt = IupGetDialogChild(ih, "HELPBUT");
    if (!IupGetCallback(ih, "HELP_CB"))
      IupSetAttribute(help_bt, "VISIBLE", "NO");
  }
  return IUP_DEFAULT;
}

static int fltkFontDlgMapMethod(Ihandle* ih)
{
  fltkFontDlgInitFamilyList(ih);
  fltkFontDlgInitSizeList(ih);
  return IUP_NOERROR;
}

static int fltkFontDlgCreateMethod(Ihandle* ih, void** params)
{
  Ihandle *ok_bt, *cancel_bt, *help_bt;
  Ihandle *lin1, *lin2, *list1, *list2, *list3;

  ok_bt = IupButton("_@IUP_OK", NULL);
  IupSetStrAttribute(ok_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(ok_bt, "ACTION", (Icallback)fltkFontDlgButtonOK_CB);
  IupSetAttributeHandle(ih, "DEFAULTENTER", ok_bt);

  cancel_bt = IupButton("_@IUP_CANCEL", NULL);
  IupSetStrAttribute(cancel_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(cancel_bt, "ACTION", (Icallback)fltkFontDlgButtonCancel_CB);
  IupSetAttributeHandle(ih, "DEFAULTESC", cancel_bt);

  help_bt = IupButton("_@IUP_HELP", NULL);
  IupSetStrAttribute(help_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetAttribute(help_bt, "NAME", "HELPBUT");
  IupSetCallback(help_bt, "ACTION", (Icallback)fltkFontDlgButtonHelp_CB);

  list1 = IupList(NULL);
  IupSetAttribute(list1, "EXPAND", "YES");
  IupSetCallback(list1, "ACTION", (Icallback)fltkFontDlgList1_CB);
  IupSetAttribute(list1, "VISIBLELINES", "8");
  IupSetAttribute(list1, "VISIBLECOLUMNS", "20");
  IupSetAttribute(list1, "NAME", "LIST1");
  IupSetAttribute(list1, "SORT", "YES");

  list2 = IupList(NULL);
  IupSetAttribute(list2, "EXPAND", "VERTICAL");
  IupSetCallback(list2, "ACTION", (Icallback)fltkFontDlgList2_CB);
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
  IupSetCallback(list3, "ACTION", (Icallback)fltkFontDlgList3_CB);
  IupSetAttribute(list3, "VISIBLELINES", "8");
  IupSetAttribute(list3, "VISIBLECOLUMNS", "6");
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

  iupChildTreeAppend(ih, IupSetAttributes(IupVbox(lin1,
    IupSetAttributes(IupVbox(
      IupLabel("_@IUP_SAMPLE"),
      IupFrame(IupSetAttributes(IupLabel(NULL), "NAME=SAMPLE, TITLE=\"abcdefghijk ABCDEFGHIJK\", EXPAND=HORIZONTAL, RASTERSIZE=x40")),
      NULL), "MARGIN=0x0, GAP=0"),
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"),
    lin2,
    NULL), "MARGIN=10x10, GAP=10"));

  IupSetCallback(ih, "SHOW_CB", (Icallback)fltkFontDlgShow_CB);

  (void)params;
  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvFontDlgInitClass(Iclass* ic)
{
  ic->Create = fltkFontDlgCreateMethod;
  ic->Map = fltkFontDlgMapMethod;
}
