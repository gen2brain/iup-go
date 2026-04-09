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
#include "iup_progressbar.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::Foundation;


#define WINUI_PB_MAX 100.0

static int winuiProgressBarSetOrientationAttrib(Ihandle* ih, const char* value)
{
  int min_w, min_h;

  if (ih->handle)
    return 0;

  if (iupStrEqualNoCase(value, "VERTICAL"))
    iupAttribSet(ih, "_IUPWINUI_PB_VERTICAL", "1");
  else
    iupAttribSet(ih, "_IUPWINUI_PB_VERTICAL", NULL);

  iupdrvProgressBarGetMinSize(ih, &min_w, &min_h);

  if (iupStrEqualNoCase(value, "VERTICAL"))
    IupSetfAttribute(ih, "RASTERSIZE", "%dx%d", min_w, 200);
  else
    IupSetfAttribute(ih, "RASTERSIZE", "%dx%d", 200, min_h);

  return 0;
}

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
      double range = ih->data->vmax - ih->data->vmin;
      if (range != 0)
        pb.Value(((ih->data->value - ih->data->vmin) / range) * WINUI_PB_MAX);
      else
        pb.Value(0);
    }
  }

  return 0;
}

static int winuiProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  ih->data->marquee = iupStrBoolean(value);

  ProgressBar pb = winuiGetHandle<ProgressBar>(ih);
  if (pb)
    pb.IsIndeterminate(ih->data->marquee ? true : false);
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

  if (iupAttribGet(ih, "_IUPWINUI_PB_VERTICAL"))
  {
    RotateTransform rot = RotateTransform();
    rot.Angle(-90);
    pb.RenderTransform(rot);
  }

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(pb);

  winuiStoreHandle(ih, pb);
  return IUP_NOERROR;
}

static void winuiProgressBarLayoutUpdateMethod(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return;

  ProgressBar pb = winuiGetHandle<ProgressBar>(ih);
  if (!pb)
    return;

  if (iupAttribGet(ih, "_IUPWINUI_PB_VERTICAL"))
  {
    Canvas::SetLeft(pb, ih->x);
    Canvas::SetTop(pb, ih->y + ih->currentheight);
    pb.Width(ih->currentheight);
    pb.Height(ih->currentwidth);
  }
  else
  {
    Canvas::SetLeft(pb, ih->x);
    Canvas::SetTop(pb, ih->y);
    if (ih->currentwidth > 0)
      pb.Width(ih->currentwidth);
    if (ih->currentheight > 0)
      pb.Height(ih->currentheight);
  }
}

static void winuiProgressBarUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
    winuiReleaseHandle<ProgressBar>(ih);
  ih->handle = NULL;
}

extern "C" IUP_SDK_API void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
{
  if (iupAttribGet(ih, "_IUPWINUI_PB_VERTICAL"))
  {
    *w = 16;
    *h = 80;
  }
  else
  {
    *w = 100;
    *h = 16;
  }
}

extern "C" IUP_SDK_API void iupdrvProgressBarInitClass(Iclass* ic)
{
  ic->Map = winuiProgressBarMapMethod;
  ic->UnMap = winuiProgressBarUnMapMethod;
  ic->LayoutUpdate = winuiProgressBarLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, winuiProgressBarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, winuiProgressBarSetOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE", NULL, winuiProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
