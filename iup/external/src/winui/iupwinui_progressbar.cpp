/** \file
 * \brief WinUI Driver - ProgressBar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_progressbar.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Windows::Foundation;


#define WINUI_PB_MAX 100.0

static int winuiProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    ih->data->value = 0;
  else
    iupStrToDouble(value, &(ih->data->value));

  iProgressBarCropValue(ih);

  if (!ih->data->marquee)
  {
    ProgressBar pb = winuiGetHandle<ProgressBar>(ih);
    if (pb)
    {
      double factor = (ih->data->value - ih->data->vmin) / (ih->data->vmax - ih->data->vmin);
      pb.Value(factor * WINUI_PB_MAX);
    }
  }

  return 0;
}

static int winuiProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  ProgressBar pb = winuiGetHandle<ProgressBar>(ih);
  if (pb)
    pb.IsIndeterminate(iupStrBoolean(value) ? true : false);
  return 1;
}

static int winuiProgressBarMapMethod(Ihandle* ih)
{
  ProgressBar pb = ProgressBar();
  pb.HorizontalAlignment(HorizontalAlignment::Left);
  pb.VerticalAlignment(VerticalAlignment::Top);
  pb.Maximum(WINUI_PB_MAX);
  pb.Value(0);

  if (iupAttribGetBoolean(ih, "MARQUEE"))
  {
    pb.IsIndeterminate(true);
    ih->data->marquee = 1;
  }

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(pb);

  winuiStoreHandle(ih, pb);
  return IUP_NOERROR;
}

static void winuiProgressBarUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
    winuiReleaseHandle<ProgressBar>(ih);
  ih->handle = NULL;
}

extern "C" void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
{
  (void)ih;
  *w = 100;
  *h = 16;
}

extern "C" void iupdrvProgressBarInitClass(Iclass* ic)
{
  ic->Map = winuiProgressBarMapMethod;
  ic->UnMap = winuiProgressBarUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, winuiProgressBarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE", NULL, winuiProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
