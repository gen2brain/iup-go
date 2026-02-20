/** \file
 * \brief WinUI Driver - Val/Slider Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cmath>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_val.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Input;
using namespace Windows::Foundation;


#define WINUI_VAL_MAX 10000.0

struct IupWinUIValAux
{
  event_token valueChangedToken;
  event_token pointerPressedToken;
  event_token pointerReleasedToken;
  event_token keyDownToken;
  bool button_pressed;
  bool ignore_changed;

  IupWinUIValAux() : valueChangedToken{}, pointerPressedToken{}, pointerReleasedToken{},
                     keyDownToken{}, button_pressed(false), ignore_changed(false) {}
};

#define IUPWINUI_VAL_AUX "_IUPWINUI_VAL_AUX"

static void winuiValUpdateValue(Ihandle* ih, bool button_release)
{
  IupWinUIValAux* aux = winuiGetAux<IupWinUIValAux>(ih, IUPWINUI_VAL_AUX);
  if (!aux || aux->ignore_changed)
    return;

  double old_val = ih->data->val;

  Slider slider = winuiGetHandle<Slider>(ih);
  if (!slider)
    return;

  double sliderVal = slider.Value();
  if (ih->data->inverted)
    sliderVal = WINUI_VAL_MAX - sliderVal;

  ih->data->val = (sliderVal / WINUI_VAL_MAX) * (ih->data->vmax - ih->data->vmin) + ih->data->vmin;
  iupValCropValue(ih);

  IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    if (ih->data->val == old_val)
      return;

    if (cb(ih) == IUP_CLOSE)
      IupExitLoop();
  }
  else
  {
    IFnd cb_old = NULL;

    if (button_release)
      cb_old = (IFnd)IupGetCallback(ih, "BUTTON_RELEASE_CB");
    else if (aux->button_pressed)
      cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
    else
      cb_old = (IFnd)IupGetCallback(ih, "BUTTON_PRESS_CB");

    if (cb_old)
      cb_old(ih, ih->data->val);
  }
}

static int winuiValSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    Slider slider = winuiGetHandle<Slider>(ih);
    if (slider)
    {
      IupWinUIValAux* aux = winuiGetAux<IupWinUIValAux>(ih, IUPWINUI_VAL_AUX);
      double sliderVal;

      iupValCropValue(ih);

      sliderVal = ((ih->data->val - ih->data->vmin) / (ih->data->vmax - ih->data->vmin)) * WINUI_VAL_MAX;
      if (ih->data->inverted)
        sliderVal = WINUI_VAL_MAX - sliderVal;

      if (aux) aux->ignore_changed = true;
      slider.Value(sliderVal);
      if (aux) aux->ignore_changed = false;
    }
  }
  return 0;
}

static int winuiValSetStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->step), 0.01))
  {
    Slider slider = winuiGetHandle<Slider>(ih);
    if (slider)
    {
      double stepFreq = ih->data->step * WINUI_VAL_MAX;
      slider.StepFrequency(stepFreq);
      slider.SmallChange(stepFreq);
    }
  }
  return 0;
}

static int winuiValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
  {
    Slider slider = winuiGetHandle<Slider>(ih);
    if (slider)
    {
      double largeChange = ih->data->pagestep * WINUI_VAL_MAX;
      slider.LargeChange(largeChange);
    }
  }
  return 0;
}

static int winuiValMapMethod(Ihandle* ih)
{
  IupWinUIValAux* aux = new IupWinUIValAux();

  Slider slider = Slider();
  slider.HorizontalAlignment(HorizontalAlignment::Left);
  slider.VerticalAlignment(VerticalAlignment::Top);

  slider.Minimum(0);
  slider.Maximum(WINUI_VAL_MAX);
  slider.IsThumbToolTipEnabled(false);

  if (ih->data->orientation == IVAL_HORIZONTAL)
    slider.Orientation(Orientation::Horizontal);
  else
    slider.Orientation(Orientation::Vertical);

  if (ih->data->inverted)
    slider.IsDirectionReversed(true);

  double stepFreq = ih->data->step * WINUI_VAL_MAX;
  slider.StepFrequency(stepFreq);
  slider.SmallChange(stepFreq);

  double largeChange = ih->data->pagestep * WINUI_VAL_MAX;
  slider.LargeChange(largeChange);

  aux->valueChangedToken = slider.ValueChanged([ih](IInspectable const&, RangeBaseValueChangedEventArgs const&) {
    winuiValUpdateValue(ih, false);
  });

  aux->pointerPressedToken = slider.PointerPressed([ih](IInspectable const&, PointerRoutedEventArgs const&) {
    IupWinUIValAux* a = winuiGetAux<IupWinUIValAux>(ih, IUPWINUI_VAL_AUX);
    if (a) a->button_pressed = true;
  });

  aux->pointerReleasedToken = slider.PointerReleased([ih](IInspectable const&, PointerRoutedEventArgs const&) {
    IupWinUIValAux* a = winuiGetAux<IupWinUIValAux>(ih, IUPWINUI_VAL_AUX);
    if (a && a->button_pressed)
    {
      a->button_pressed = false;
      winuiValUpdateValue(ih, true);
    }
  });

  aux->keyDownToken = slider.PreviewKeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
    if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
      args.Handled(true);
  });

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(slider);

  winuiSetAux(ih, IUPWINUI_VAL_AUX, aux);
  winuiStoreHandle(ih, slider);
  return IUP_NOERROR;
}

static void winuiValUnMapMethod(Ihandle* ih)
{
  IupWinUIValAux* aux = winuiGetAux<IupWinUIValAux>(ih, IUPWINUI_VAL_AUX);

  if (ih->handle && aux)
  {
    Slider slider = winuiGetHandle<Slider>(ih);
    if (slider)
    {
      if (aux->valueChangedToken)
        slider.ValueChanged(aux->valueChangedToken);
      if (aux->pointerPressedToken)
        slider.PointerPressed(aux->pointerPressedToken);
      if (aux->pointerReleasedToken)
        slider.PointerReleased(aux->pointerReleasedToken);
      if (aux->keyDownToken)
        slider.PreviewKeyDown(aux->keyDownToken);
    }
    winuiReleaseHandle<Slider>(ih);
  }

  winuiFreeAux<IupWinUIValAux>(ih, IUPWINUI_VAL_AUX);
  ih->handle = NULL;
}

extern "C" void iupdrvValGetMinSize(Ihandle* ih, int* w, int* h)
{
  if (ih->data->orientation == IVAL_HORIZONTAL)
  {
    *w = 100;
    *h = 32;
  }
  else
  {
    *w = 32;
    *h = 100;
  }
}

extern "C" void iupdrvValInitClass(Iclass* ic)
{
  ic->Map = winuiValMapMethod;
  ic->UnMap = winuiValUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, winuiValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, winuiValSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, winuiValSetStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "SHOWTICKS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
