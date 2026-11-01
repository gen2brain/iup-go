/** \file
 * \brief IupPrintDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_predialogs.h"
#include "iup_childtree.h"
#include "iup_drvinfo.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"


IUP_API Ihandle* IupPrintDlg(void)
{
  return IupCreate("printdlg");
}

IUP_SDK_API const char* iupPrintDlgGetJobName(Ihandle* ih)
{
  const char* name = iupAttribGet(ih, "JOBNAME");
  if (!name)
    name = iupAttribGet(ih, "TITLE");
  if (!name)
    name = "IUP";
  return name;
}

IUP_SDK_API int iupPrintDlgPaginate(Ihandle* ih, int width, int height)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "PAGINATE_CB");
  int count;

  iupAttribSetStrf(ih, "PAGESIZE", "%dx%d", width, height);

  if (cb)
    count = cb(ih, width, height);
  else
    count = iupAttribGetInt(ih, "PAGECOUNT");

  if (count < 1)
    count = 1;
  return count;
}

IUP_SDK_API int iupPrintDlgDrawPage(Ihandle* ih, void* context, int page, int width, int height)
{
  IFniii cb = (IFniii)IupGetCallback(ih, "PRINTPAGE_CB");
  int ret = IUP_DEFAULT;

  if (!cb)
    return IUP_DEFAULT;

  iupAttribSet(ih, "_IUP_PRINT_CONTEXT", (char*)context);
  iupAttribSetInt(ih, "_IUP_PRINT_WIDTH", width);
  iupAttribSetInt(ih, "_IUP_PRINT_HEIGHT", height);

  ret = cb(ih, page, width, height);

  iupAttribSet(ih, "_IUP_PRINT_CONTEXT", NULL);
  iupAttribSet(ih, "_IUP_PRINT_WIDTH", NULL);
  iupAttribSet(ih, "_IUP_PRINT_HEIGHT", NULL);

  return ret;
}

IUP_SDK_API int iupPrintDlgPageSelected(Ihandle* ih, int page)
{
  const char* pages = iupAttribGet(ih, "_IUP_PRINT_PAGES");
  char* end;

  if (!pages)
    return 1;

  while (*pages)
  {
    int first, last;

    while (*pages == ' ' || *pages == ',')
      pages++;
    if (!*pages)
      break;

    if (*pages == '-')
      first = 1;
    else
    {
      first = (int)strtol(pages, &end, 10);
      if (end == pages)
        return 0;
      pages = end;
    }
    last = first;

    while (*pages == ' ')
      pages++;
    if (*pages == '-')
    {
      pages++;
      last = (int)strtol(pages, &end, 10);
      if (end == pages)
        last = page;
      pages = end;
    }

    if (page >= first && page <= last)
      return 1;

    while (*pages && *pages != ',')
      pages++;
  }

  return 0;
}

/******************************************************************************
 * Custom Print Dialog
 *****************************************************************************/

static const IprintDlgDriver* iprintdlg_driver = NULL;

static int iPrintDlgIsFile(Ihandle* printer_list)
{
  return IupGetInt(printer_list, "VALUE") == IupGetInt(printer_list, "COUNT");
}

static char* iPrintDlgGetPrinter(Ihandle* printer_list)
{
  if (iPrintDlgIsFile(printer_list))
    return NULL;
  return IupGetAttributeId(printer_list, "", IupGetInt(printer_list, "VALUE"));
}

static void iPrintDlgUpdatePrinter(Ihandle* ih)
{
  Ihandle* printer_list = IupGetDialogChild(ih, "PRINTER");
  Ihandle* paper_list = IupGetDialogChild(ih, "PAPER");
  int is_file = iPrintDlgIsFile(printer_list);

  IupSetAttribute(paper_list, "1", NULL);
  iprintdlg_driver->FillPapers(paper_list, iPrintDlgGetPrinter(printer_list));

  IupSetAttribute(IupGetDialogChild(ih, "FILENAME"), "ACTIVE", is_file ? "YES" : "NO");
  IupSetAttribute(IupGetDialogChild(ih, "FILEBUT"), "ACTIVE", is_file ? "YES" : "NO");
}

static int iPrintDlgPrinter_CB(Ihandle* list, char* text, int item, int state)
{
  (void)text;
  (void)item;
  if (state == 1)
    iPrintDlgUpdatePrinter(IupGetDialog(list));
  return IUP_DEFAULT;
}

static int iPrintDlgPages_CB(Ihandle* toggle, int state)
{
  Ihandle* pages = IupGetDialogChild(toggle, "PAGES");
  IupSetAttribute(pages, "ACTIVE", state ? "YES" : "NO");
  if (state)
    IupSetFocus(pages);
  return IUP_DEFAULT;
}

static int iPrintDlgButtonFile_CB(Ihandle* self)
{
  Ihandle* dlg = IupGetDialog(self);
  Ihandle* filename = IupGetDialogChild(dlg, "FILENAME");
  Ihandle* filedlg = IupFileDlg();

  IupSetAttribute(filedlg, "DIALOGTYPE", "SAVE");
  IupSetAttribute(filedlg, "EXTFILTER", "PDF|*.pdf|");
  IupSetStrAttribute(filedlg, "FILE", IupGetAttribute(filename, "VALUE"));
  IupSetAttributeHandle(filedlg, "PARENTDIALOG", dlg);

  IupPopup(filedlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  if (IupGetInt(filedlg, "STATUS") != -1)
    IupSetStrAttribute(filename, "VALUE", IupGetAttribute(filedlg, "VALUE"));

  IupDestroy(filedlg);
  return IUP_DEFAULT;
}

static int iPrintDlgPagesValid(const char* pages)
{
  int digits = 0;

  for (; *pages; pages++)
  {
    if (*pages >= '0' && *pages <= '9')
      digits = 1;
    else if (*pages != ' ' && *pages != ',' && *pages != '-')
      return 0;
  }

  return digits;
}

static int iPrintDlgButtonPrint_CB(Ihandle* self)
{
  Ihandle* dlg = IupGetDialog(self);
  Ihandle* printer_list = IupGetDialogChild(dlg, "PRINTER");
  Ihandle* filename = IupGetDialogChild(dlg, "FILENAME");
  Ihandle* pages = IupGetDialogChild(dlg, "PAGES");
  char* printer = iupStrDup(iPrintDlgGetPrinter(printer_list));
  char* file = iupStrDup(IupGetAttribute(filename, "VALUE"));
  char* paper = iupStrDup(IupGetAttribute(IupGetDialogChild(dlg, "PAPER"), "VALUESTRING"));
  int landscape = IupGetInt(IupGetDialogChild(dlg, "ORIENTATION"), "VALUE") == 2;
  int copies = IupGetInt(IupGetDialogChild(dlg, "COPIES"), "SPINVALUE");
  int ret = IUP_DEFAULT;

  if (!printer && (!file || !*file))
    IupSetFocus(filename);
  else if (IupGetInt(IupGetDialogChild(dlg, "RANGEPAGES"), "VALUE") && !iPrintDlgPagesValid(IupGetAttribute(pages, "VALUE")))
    IupSetFocus(pages);
  else
  {
    int status;

    if (IupGetInt(IupGetDialogChild(dlg, "RANGEPAGES"), "VALUE"))
      iupAttribSetStr(dlg, "_IUP_PRINT_PAGES", IupGetAttribute(pages, "VALUE"));
    else
      iupAttribSet(dlg, "_IUP_PRINT_PAGES", NULL);

    iupAttribSet(dlg, "ORIENTATION", landscape ? "LANDSCAPE" : "PORTRAIT");

    IupSetAttribute(dlg, "ACTIVE", "NO");
    status = iprintdlg_driver->Print(dlg, printer, file, paper, landscape, copies > 0 ? copies : 1);
    IupSetAttribute(dlg, "ACTIVE", "YES");

    iupAttribSet(dlg, "_IUP_PRINT_PAGES", NULL);
    iupAttribSet(dlg, "STATUS", status == 1 ? "1" : (status == 0 ? "0" : "-1"));
    ret = IUP_CLOSE;
  }

  free(printer);
  free(file);
  free(paper);
  return ret;
}

static int iPrintDlgButtonCancel_CB(Ihandle* self)
{
  iupAttribSet(IupGetDialog(self), "STATUS", "0");
  return IUP_CLOSE;
}

static int iPrintDlgShow_CB(Ihandle* ih, int state)
{
  if (state == IUP_SHOW)
  {
    Ihandle* filename = IupGetDialogChild(ih, "FILENAME");

    iupAttribSet(ih, "STATUS", "0");

    if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "LANDSCAPE"))
      IupSetAttribute(IupGetDialogChild(ih, "ORIENTATION"), "VALUE", "2");
    else
      IupSetAttribute(IupGetDialogChild(ih, "ORIENTATION"), "VALUE", "1");

    if (!IupGetAttribute(filename, "VALUE") || !*IupGetAttribute(filename, "VALUE"))
    {
      char* title = iupStrReturnStrf("%s.pdf", iupPrintDlgGetJobName(ih));
      char* file = iupStrFileMakeFileName(iupdrvGetCurrentDirectory(), title);
      IupSetStrAttribute(filename, "VALUE", file ? file : title);
      free(file);
    }

    iPrintDlgUpdatePrinter(ih);
  }
  return IUP_DEFAULT;
}

static Ihandle* iPrintDlgLabel(const char* title)
{
  return IupSetAttributes(IupLabel(title), "ALIGNMENT=ARIGHT:ACENTER, EXPAND=HORIZONTAL");
}

static int iPrintDlgCreateMethod(Ihandle* ih, void** params)
{
  Ihandle *print_bt, *cancel_bt, *buttons, *printer_list, *filename, *file_bt, *paper_list, *orient_list;
  Ihandle *all_tg, *range_tg, *pages, *copies, *grid;

  print_bt = IupButton("_@IUP_PRINT");
  IupSetStrAttribute(print_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(print_bt, "ACTION", (Icallback)iPrintDlgButtonPrint_CB);
  IupSetAttributeHandle(ih, "DEFAULTENTER", print_bt);

  cancel_bt = IupButton("_@IUP_CANCEL");
  IupSetStrAttribute(cancel_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(cancel_bt, "ACTION", (Icallback)iPrintDlgButtonCancel_CB);
  IupSetAttributeHandle(ih, "DEFAULTESC", cancel_bt);

  printer_list = IupList();
  IupSetAttribute(printer_list, "DROPDOWN", "YES");
  IupSetAttribute(printer_list, "EXPAND", "HORIZONTAL");
  IupSetAttribute(printer_list, "NAME", "PRINTER");
  IupSetCallback(printer_list, "ACTION", (Icallback)iPrintDlgPrinter_CB);

  filename = IupText();
  IupSetAttribute(filename, "EXPAND", "HORIZONTAL");
  IupSetAttribute(filename, "VISIBLECOLUMNS", "25");
  IupSetAttribute(filename, "NAME", "FILENAME");

  file_bt = IupButton("...");
  IupSetAttribute(file_bt, "NAME", "FILEBUT");
  IupSetCallback(file_bt, "ACTION", (Icallback)iPrintDlgButtonFile_CB);

  paper_list = IupList();
  IupSetAttribute(paper_list, "DROPDOWN", "YES");
  IupSetAttribute(paper_list, "EXPAND", "HORIZONTAL");
  IupSetAttribute(paper_list, "NAME", "PAPER");

  orient_list = IupList();
  IupSetAttribute(orient_list, "DROPDOWN", "YES");
  IupSetAttribute(orient_list, "EXPAND", "HORIZONTAL");
  IupSetAttribute(orient_list, "NAME", "ORIENTATION");
  IupSetStrAttribute(orient_list, "1", "_@IUP_PORTRAIT");
  IupSetStrAttribute(orient_list, "2", "_@IUP_LANDSCAPE");
  IupSetAttribute(orient_list, "VALUE", "1");

  all_tg = IupToggle("_@IUP_ALLPAGES");
  range_tg = IupToggle("_@IUP_PAGERANGE");
  IupSetAttribute(range_tg, "NAME", "RANGEPAGES");
  IupSetCallback(range_tg, "ACTION", (Icallback)iPrintDlgPages_CB);

  pages = IupText();
  IupSetAttribute(pages, "EXPAND", "HORIZONTAL");
  IupSetAttribute(pages, "NAME", "PAGES");
  IupSetAttribute(pages, "ACTIVE", "NO");

  copies = IupText();
  IupSetAttribute(copies, "SPIN", "YES");
  IupSetAttribute(copies, "SPINMIN", "1");
  IupSetAttribute(copies, "SPINMAX", "999");
  IupSetAttribute(copies, "SPINVALUE", "1");
  IupSetAttribute(copies, "VISIBLECOLUMNS", "4");
  IupSetAttribute(copies, "NAME", "COPIES");

  iprintdlg_driver->FillPrinters(printer_list);
  IupSetStrAttributeId(printer_list, "", IupGetInt(printer_list, "COUNT") + 1, "_@IUP_PRINTTOFILE");
  if (IupGetInt(printer_list, "VALUE") <= 0)
    IupSetInt(printer_list, "VALUE", IupGetInt(printer_list, "COUNT"));
  iprintdlg_driver->FillPapers(paper_list, iPrintDlgGetPrinter(printer_list));

  grid = IupGridBox(
    iPrintDlgLabel("_@IUP_PRINTER"), printer_list,
    iPrintDlgLabel("_@IUP_FILE"), IupSetAttributes(IupHbox(filename, file_bt, NULL), "MARGIN=0x0, GAP=5, ALIGNMENT=ACENTER"),
    iPrintDlgLabel("_@IUP_PAPERSIZE"), paper_list,
    iPrintDlgLabel("_@IUP_ORIENTATION"), orient_list,
    iPrintDlgLabel("_@IUP_PAGES"), IupRadio(IupSetAttributes(IupHbox(all_tg, range_tg, pages, NULL), "MARGIN=0x0, GAP=10, ALIGNMENT=ACENTER")),
    iPrintDlgLabel("_@IUP_COPIES"), copies,
    NULL);
  IupSetAttribute(grid, "NUMDIV", "2");
  IupSetAttribute(grid, "ORIENTATION", "HORIZONTAL");
  IupSetAttribute(grid, "SIZECOL", "-1");
  IupSetAttribute(grid, "SIZELIN", "-1");
  IupSetAttribute(grid, "ALIGNMENTLIN", "ACENTER");
  IupSetAttribute(grid, "GAPLIN", "8");
  IupSetAttribute(grid, "GAPCOL", "10");
  IupSetAttribute(grid, "MARGIN", "0x0");

  if (iupDialogButtonOrder() == IUP_BUTTON_ORDER_CANCEL_FIRST)
    buttons = IupHbox(IupFill(), cancel_bt, print_bt, NULL);
  else
    buttons = IupHbox(IupFill(), print_bt, cancel_bt, NULL);
  IupSetAttribute(buttons, "GAP", "5");
  IupSetAttribute(buttons, "MARGIN", "0x0");
  IupSetAttribute(buttons, "NORMALIZESIZE", "HORIZONTAL");

  iupChildTreeAppend(ih, IupSetAttributes(IupVbox(grid,
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"),
    buttons,
    NULL), "MARGIN=10x10, GAP=10"));

  IupSetStrAttribute(ih, "TITLE", "_@IUP_PRINTDLG");
  IupSetAttribute(ih, "RESIZE", "NO");
  IupSetAttribute(ih, "MINBOX", "NO");
  IupSetAttribute(ih, "MAXBOX", "NO");
  IupSetCallback(ih, "SHOW_CB", (Icallback)iPrintDlgShow_CB);

  (void)params;
  return IUP_NOERROR;
}

IUP_SDK_API void iupPrintDlgCustomInitClass(Iclass* ic, const IprintDlgDriver* driver)
{
  iprintdlg_driver = driver;
  ic->Create = iPrintDlgCreateMethod;
}

static int iPrintDlgPopupNotSupported(Ihandle* ih, int x, int y)
{
  (void)x;
  (void)y;
  iupAttribSet(ih, "STATUS", "-1");
  return IUP_ERROR;
}

Iclass* iupPrintDlgNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("dialog"));

  ic->name = "printdlg";
  ic->cons = "PrintDlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;

  ic->New = iupPrintDlgNewClass;

  iupdrvPrintDlgInitClass(ic);

  if (!ic->Create)
  {
    ic->parent->LayoutUpdate = NULL;
    ic->parent->SetChildrenPosition = NULL;
    ic->parent->Map = NULL;
    ic->parent->UnMap = NULL;

    if (!ic->DlgPopup)
      ic->DlgPopup = iPrintDlgPopupNotSupported;
  }

  iupClassRegisterCallback(ic, "PAGINATE_CB", "ii");
  iupClassRegisterCallback(ic, "PRINTPAGE_CB", "iii");

  iupClassRegisterAttribute(ic, "JOBNAME", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "PORTRAIT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGECOUNT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "STATUS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);

  iupDrawRegisterAttributes(ic);

  return ic;
}
