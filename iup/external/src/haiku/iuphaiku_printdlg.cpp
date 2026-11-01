/** \file
 * \brief IupPrintDlg - Haiku driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <PrintJob.h>
#include <View.h>
#include <Window.h>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_stdcontrols.h"
}

#define IUPHAIKU_PRINT_SCALE (96.0f / 72.0f)

class IupHaikuPrintView : public BView
{
public:
  IupHaikuPrintView(BRect frame, Ihandle* ih)
    : BView(frame, "iup_print", B_FOLLOW_NONE, B_WILL_DRAW), ih(ih), page(1), width(0), height(0), ret(IUP_DEFAULT)
  {
  }

  void Draw(BRect update_rect) override
  {
    (void)update_rect;
    SetScale(1.0f / IUPHAIKU_PRINT_SCALE);
    ret = iupPrintDlgDrawPage(ih, this, page, width, height);
  }

  Ihandle* ih;
  int page;
  int width, height;
  int ret;
};

static int haikuPrintDlgPopup(Ihandle* ih, int x, int y)
{
  BPrintJob job(iupPrintDlgGetJobName(ih));
  int status = 1;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  if (job.ConfigJob() != B_OK)
  {
    iupAttribSet(ih, "STATUS", "0");
    return IUP_NOERROR;
  }

  BRect printable = job.PrintableRect();
  int width = static_cast<int>(printable.Width() * IUPHAIKU_PRINT_SCALE + 0.5f);
  int height = static_cast<int>(printable.Height() * IUPHAIKU_PRINT_SCALE + 0.5f);
  int count = iupPrintDlgPaginate(ih, width, height);
  int first = job.FirstPage() > 0 ? static_cast<int>(job.FirstPage()) : 1;
  int last = job.LastPage() > 0 && job.LastPage() < count ? static_cast<int>(job.LastPage()) : count;

  iupAttribSet(ih, "ORIENTATION", width > height ? "LANDSCAPE" : "PORTRAIT");

  BRect frame(0, 0, printable.Width(), printable.Height());
  auto* window = new BWindow(frame.OffsetByCopy(-10000, -10000), "iup_print", B_NO_BORDER_WINDOW_LOOK,
                             B_NORMAL_WINDOW_FEEL, B_AVOID_FOCUS);
  auto* view = new IupHaikuPrintView(frame, ih);
  view->width = width;
  view->height = height;
  window->AddChild(view);
  window->Hide();
  window->Show();

  job.BeginJob();

  for (int page = first; page <= last; page++)
  {
    view->page = page;
    job.DrawView(view, frame, BPoint(0, 0));
    job.SpoolPage();

    if (!job.CanContinue())
    {
      status = -1;
      break;
    }

    if (view->ret == IUP_CLOSE)
    {
      status = 0;
      break;
    }
  }

  if (status == 1)
    job.CommitJob();
  else
    job.CancelJob();

  window->Lock();
  window->Quit();

  iupAttribSet(ih, "STATUS", status == 1 ? "1" : (status == 0 ? "0" : "-1"));
  return IUP_NOERROR;
}

extern "C" IUP_SDK_API void iupdrvPrintDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = haikuPrintDlgPopup;
}
