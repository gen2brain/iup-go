/** \file
 * \brief IupPrintDlg - FLTK driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Printer.H>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_stdcontrols.h"
}

#define IUPFLTK_PRINT_SCALE (96.0f / 72.0f)

static int fltkPrintDlgPopup(Ihandle* ih, int x, int y)
{
  Fl_Printer printer;
  int page_count = iupAttribGetInt(ih, "PAGECOUNT");
  int from = 1, to = 0, page, count, pt_w = 0, pt_h = 0, width, height;
  char* error = nullptr;
  int status = 1;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  int ret = printer.begin_job(page_count > 0 ? page_count : 0, &from, &to, &error);
  if (ret != 0)
  {
    delete[] error;
    iupAttribSet(ih, "STATUS", ret == 1 ? "0" : "-1");
    return IUP_NOERROR;
  }

  printer.printable_rect(&pt_w, &pt_h);
  width = static_cast<int>(pt_w * IUPFLTK_PRINT_SCALE + 0.5f);
  height = static_cast<int>(pt_h * IUPFLTK_PRINT_SCALE + 0.5f);
  iupAttribSet(ih, "ORIENTATION", width > height ? "LANDSCAPE" : "PORTRAIT");

  count = iupPrintDlgPaginate(ih, width, height);
  if (from < 1)
    from = 1;
  if (to < 1 || to > count)
    to = count;

  for (page = from; page <= to; page++)
  {
    if (printer.begin_page() != 0)
    {
      status = -1;
      break;
    }

    printer.scale(1.0f / IUPFLTK_PRINT_SCALE);
    ret = iupPrintDlgDrawPage(ih, &printer, page, width, height);

    if (printer.end_page() != 0)
    {
      status = -1;
      break;
    }

    if (ret == IUP_CLOSE)
    {
      status = 0;
      break;
    }
  }

  printer.end_job();
  iupAttribSet(ih, "STATUS", status == 1 ? "1" : (status == 0 ? "0" : "-1"));
  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvPrintDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = fltkPrintDlgPopup;
}
