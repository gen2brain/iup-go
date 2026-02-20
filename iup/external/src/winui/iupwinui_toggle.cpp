/** \file
 * \brief WinUI Driver - Toggle Control
 *
 * Uses CheckBox for text toggles, RadioButton for radio groups,
 * ToggleButton for image toggles, ToggleSwitch for SWITCH mode.
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
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_toggle.h"
#include "iup_register.h"
#include "iup_childtree.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;


static void winuiToggleCallAction(Ihandle* ih, int checked)
{
  IFni cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih, checked);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }
}

static int winuiToggleGetChecked(Ihandle* ih)
{
  IupWinUIToggleAux* aux = winuiGetAux<IupWinUIToggleAux>(ih, IUPWINUI_TOGGLE_AUX);
  if (!aux)
    return 0;

  switch (aux->controlType)
  {
  case IUPWINUI_TOGGLE_TOGGLESWITCH:
    {
      ToggleSwitch ts = winuiGetHandle<ToggleSwitch>(ih);
      if (ts)
        return ts.IsOn() ? 1 : 0;
    }
    break;
  case IUPWINUI_TOGGLE_TOGGLEBUTTON:
    {
      ToggleButton tb = winuiGetHandle<ToggleButton>(ih);
      if (tb)
      {
        auto isChecked = tb.IsChecked();
        if (isChecked)
          return isChecked.Value() ? 1 : 0;
      }
    }
    break;
  case IUPWINUI_TOGGLE_RADIOBUTTON:
    {
      RadioButton rb = winuiGetHandle<RadioButton>(ih);
      if (rb)
        return rb.IsChecked() ? 1 : 0;
    }
    break;
  case IUPWINUI_TOGGLE_CHECKBOX:
  default:
    {
      CheckBox cb = winuiGetHandle<CheckBox>(ih);
      if (cb)
      {
        auto isChecked = cb.IsChecked();
        if (isChecked)
          return isChecked.Value() ? 1 : 0;
      }
    }
    break;
  }

  return 0;
}

static int winuiToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  IupWinUIToggleAux* aux = winuiGetAux<IupWinUIToggleAux>(ih, IUPWINUI_TOGGLE_AUX);
  if (!aux)
    return 0;

  bool checked = iupStrBoolean(value);

  switch (aux->controlType)
  {
  case IUPWINUI_TOGGLE_TOGGLESWITCH:
    {
      ToggleSwitch ts = winuiGetHandle<ToggleSwitch>(ih);
      if (ts)
        ts.IsOn(checked);
    }
    break;
  case IUPWINUI_TOGGLE_TOGGLEBUTTON:
    {
      ToggleButton tb = winuiGetHandle<ToggleButton>(ih);
      if (tb)
        tb.IsChecked(checked);
    }
    break;
  case IUPWINUI_TOGGLE_RADIOBUTTON:
    {
      RadioButton rb = winuiGetHandle<RadioButton>(ih);
      if (rb)
        rb.IsChecked(checked);
    }
    break;
  case IUPWINUI_TOGGLE_CHECKBOX:
  default:
    {
      CheckBox cb = winuiGetHandle<CheckBox>(ih);
      if (cb)
        cb.IsChecked(checked);
    }
    break;
  }

  return 0;
}

static char* winuiToggleGetValueAttrib(Ihandle* ih)
{
  return winuiToggleGetChecked(ih) ? (char*)"ON" : (char*)"OFF";
}

static void winuiToggleSetImageContent(Ihandle* ih, const char* name, int make_inactive)
{
  IupWinUIToggleAux* aux = winuiGetAux<IupWinUIToggleAux>(ih, IUPWINUI_TOGGLE_AUX);
  if (!aux || !name)
    return;

  if (aux->controlType != IUPWINUI_TOGGLE_TOGGLEBUTTON)
    return;

  ToggleButton tb = winuiGetHandle<ToggleButton>(ih);
  if (!tb)
    return;

  void* imghandle = iupImageGetImage(name, ih, make_inactive, NULL);
  WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
  if (!bitmap)
    return;

  Image image;
  image.Source(bitmap);
  image.Stretch(Media::Stretch::None);

  const char* title = iupAttribGet(ih, "TITLE");
  if (title && title[0])
  {
    StackPanel sp;
    sp.Orientation(Orientation::Horizontal);
    sp.Children().Append(image);

    TextBlock txtBlock;
    txtBlock.Text(iupwinuiStringToHString(title));
    txtBlock.Margin(ThicknessHelper::FromLengths(4, 0, 0, 0));
    txtBlock.VerticalAlignment(VerticalAlignment::Center);
    sp.Children().Append(txtBlock);

    tb.Content(sp);
  }
  else
  {
    tb.Content(image);
  }
}

static int winuiToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  IupWinUIToggleAux* aux = winuiGetAux<IupWinUIToggleAux>(ih, IUPWINUI_TOGGLE_AUX);
  if (!aux)
    return 1;

  hstring title = iupwinuiProcessMnemonic(value, NULL);

  switch (aux->controlType)
  {
  case IUPWINUI_TOGGLE_TOGGLESWITCH:
    {
      ToggleSwitch ts = winuiGetHandle<ToggleSwitch>(ih);
      if (ts)
        ts.Header(box_value(title));
    }
    break;
  case IUPWINUI_TOGGLE_TOGGLEBUTTON:
    {
      if (ih->data->type == IUP_TOGGLE_IMAGE)
      {
        const char* imagename = iupAttribGet(ih, "IMAGE");
        if (imagename)
          winuiToggleSetImageContent(ih, imagename, 0);
      }
      else
      {
        ToggleButton tb = winuiGetHandle<ToggleButton>(ih);
        if (tb)
          tb.Content(box_value(title));
      }
    }
    break;
  case IUPWINUI_TOGGLE_RADIOBUTTON:
    {
      RadioButton rb = winuiGetHandle<RadioButton>(ih);
      if (rb)
        rb.Content(box_value(title));
    }
    break;
  case IUPWINUI_TOGGLE_CHECKBOX:
  default:
    {
      CheckBox cb = winuiGetHandle<CheckBox>(ih);
      if (cb)
        cb.Content(box_value(title));
    }
    break;
  }

  return 1;
}

static char* winuiToggleGetTitleAttrib(Ihandle* ih)
{
  IupWinUIToggleAux* aux = winuiGetAux<IupWinUIToggleAux>(ih, IUPWINUI_TOGGLE_AUX);
  if (!aux)
    return NULL;

  IInspectable content{nullptr};

  switch (aux->controlType)
  {
  case IUPWINUI_TOGGLE_TOGGLESWITCH:
    {
      ToggleSwitch ts = winuiGetHandle<ToggleSwitch>(ih);
      if (ts)
        content = ts.Header();
    }
    break;
  case IUPWINUI_TOGGLE_TOGGLEBUTTON:
    {
      ToggleButton tb = winuiGetHandle<ToggleButton>(ih);
      if (tb)
        content = tb.Content();
    }
    break;
  case IUPWINUI_TOGGLE_RADIOBUTTON:
    {
      RadioButton rb = winuiGetHandle<RadioButton>(ih);
      if (rb)
        content = rb.Content();
    }
    break;
  case IUPWINUI_TOGGLE_CHECKBOX:
  default:
    {
      CheckBox cb = winuiGetHandle<CheckBox>(ih);
      if (cb)
        content = cb.Content();
    }
    break;
  }

  if (content)
    return iupwinuiHStringToString(unbox_value<hstring>(content));

  return NULL;
}

static int winuiToggleSetActiveAttrib(Ihandle* ih, const char* value)
{
  IupWinUIToggleAux* aux = winuiGetAux<IupWinUIToggleAux>(ih, IUPWINUI_TOGGLE_AUX);
  if (!aux)
    return 0;

  bool enabled = iupStrBoolean(value);

  switch (aux->controlType)
  {
  case IUPWINUI_TOGGLE_TOGGLESWITCH:
    {
      ToggleSwitch ts = winuiGetHandle<ToggleSwitch>(ih);
      if (ts)
        ts.IsEnabled(enabled);
    }
    break;
  case IUPWINUI_TOGGLE_TOGGLEBUTTON:
    {
      ToggleButton tb = winuiGetHandle<ToggleButton>(ih);
      if (tb)
        tb.IsEnabled(enabled);
    }
    break;
  case IUPWINUI_TOGGLE_RADIOBUTTON:
    {
      RadioButton rb = winuiGetHandle<RadioButton>(ih);
      if (rb)
        rb.IsEnabled(enabled);
    }
    break;
  case IUPWINUI_TOGGLE_CHECKBOX:
  default:
    {
      CheckBox cb = winuiGetHandle<CheckBox>(ih);
      if (cb)
        cb.IsEnabled(enabled);
    }
    break;
  }

  return 0;
}

static int winuiToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (iupAttribGetBoolean(ih, "SWITCH"))
    return 0;

  if (ih->data->type != IUP_TOGGLE_TEXT)
    return 0;

  if (!ih->handle)
    return 1;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupWinUIToggleAux* aux = winuiGetAux<IupWinUIToggleAux>(ih, IUPWINUI_TOGGLE_AUX);
  if (!aux)
    return 0;

  Windows::UI::Color color;
  color.A = 255;
  color.R = r;
  color.G = g;
  color.B = b;
  SolidColorBrush brush(color);

  switch (aux->controlType)
  {
  case IUPWINUI_TOGGLE_CHECKBOX:
    {
      CheckBox cb = winuiGetHandle<CheckBox>(ih);
      if (cb)
      {
        cb.Foreground(brush);
        cb.Resources().Insert(box_value(L"CheckBoxForegroundUnchecked"), brush);
        cb.Resources().Insert(box_value(L"CheckBoxForegroundUncheckedPointerOver"), brush);
        cb.Resources().Insert(box_value(L"CheckBoxForegroundUncheckedPressed"), brush);
        cb.Resources().Insert(box_value(L"CheckBoxForegroundChecked"), brush);
        cb.Resources().Insert(box_value(L"CheckBoxForegroundCheckedPointerOver"), brush);
        cb.Resources().Insert(box_value(L"CheckBoxForegroundCheckedPressed"), brush);
        cb.Resources().Insert(box_value(L"CheckBoxForegroundIndeterminate"), brush);
        cb.Resources().Insert(box_value(L"CheckBoxForegroundIndeterminatePointerOver"), brush);
        cb.Resources().Insert(box_value(L"CheckBoxForegroundIndeterminatePressed"), brush);
      }
    }
    break;
  case IUPWINUI_TOGGLE_RADIOBUTTON:
    {
      RadioButton rb = winuiGetHandle<RadioButton>(ih);
      if (rb)
      {
        rb.Foreground(brush);
        rb.Resources().Insert(box_value(L"RadioButtonForeground"), brush);
        rb.Resources().Insert(box_value(L"RadioButtonForegroundPointerOver"), brush);
        rb.Resources().Insert(box_value(L"RadioButtonForegroundPressed"), brush);
      }
    }
    break;
  default:
    break;
  }

  return 1;
}

static int winuiToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (iupdrvIsActive(ih))
      winuiToggleSetImageContent(ih, value, 0);
    else
    {
      char* iminactive = iupAttribGet(ih, "IMINACTIVE");
      if (iminactive)
        winuiToggleSetImageContent(ih, iminactive, 0);
      else
        winuiToggleSetImageContent(ih, value, 1);
    }
  }
  return 1;
}

static int winuiToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        winuiToggleSetImageContent(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        winuiToggleSetImageContent(ih, name, 1);
      }
    }
  }
  return 1;
}

static int winuiToggleSetImPressAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->type == IUP_TOGGLE_IMAGE)
    iupdrvRedrawNow(ih);
  return 1;
}

static int winuiToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle && ih->data->type == IUP_TOGGLE_IMAGE)
    iupdrvRedrawNow(ih);

  return 0;
}

static int winuiToggleMapMethod(Ihandle* ih)
{
  IupWinUIToggleAux* aux = new IupWinUIToggleAux();

  const char* title = iupAttribGet(ih, "TITLE");
  const char* image = iupAttribGet(ih, "IMAGE");
  int isSwitch = iupAttribGetBoolean(ih, "SWITCH");

  Ihandle* radio = iupRadioFindToggleParent(ih);
  if (radio)
    ih->data->is_radio = 1;

  if (image)
    ih->data->type = IUP_TOGGLE_IMAGE;
  else
    ih->data->type = IUP_TOGGLE_TEXT;

  if (isSwitch && !ih->data->is_radio)
  {
    aux->controlType = IUPWINUI_TOGGLE_TOGGLESWITCH;

    ToggleSwitch ts;
    ts.HorizontalAlignment(HorizontalAlignment::Left);
    ts.VerticalAlignment(VerticalAlignment::Top);
    ts.OnContent(nullptr);
    ts.OffContent(nullptr);

    if (title)
      ts.Header(box_value(iupwinuiProcessMnemonic(title, NULL)));

    aux->toggledToken = ts.Toggled([ih](IInspectable const&, RoutedEventArgs const&) {
      ToggleSwitch t = winuiGetHandle<ToggleSwitch>(ih);
      if (t)
        winuiToggleCallAction(ih, t.IsOn() ? 1 : 0);
    });

    aux->keyDownToken = ts.PreviewKeyDown([ih](IInspectable const&, Input::KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(ts);

    winuiStoreHandle(ih, ts);
  }
  else if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    aux->controlType = IUPWINUI_TOGGLE_TOGGLEBUTTON;

    ToggleButton tb;
    tb.HorizontalAlignment(HorizontalAlignment::Left);
    tb.VerticalAlignment(VerticalAlignment::Top);

    if (!title)
      tb.Padding(Thickness{0, 0, 0, 0});

    if (image)
    {
      void* imghandle = iupImageGetImage(image, ih, 0, NULL);
      WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
      if (bitmap)
      {
        Image img;
        img.Source(bitmap);
        img.Stretch(Media::Stretch::None);

        if (title)
        {
          StackPanel sp;
          sp.Orientation(Orientation::Horizontal);
          sp.Children().Append(img);

          TextBlock txtBlock;
          txtBlock.Text(iupwinuiStringToHString(title));
          txtBlock.Margin(ThicknessHelper::FromLengths(4, 0, 0, 0));
          txtBlock.VerticalAlignment(VerticalAlignment::Center);
          sp.Children().Append(txtBlock);

          tb.Content(sp);
        }
        else
        {
          tb.Content(img);
        }
      }
    }

    aux->clickToken = tb.Click([ih](IInspectable const&, RoutedEventArgs const&) {
      ToggleButton t = winuiGetHandle<ToggleButton>(ih);
      if (t)
      {
        auto isChecked = t.IsChecked();
        int checked = (isChecked && isChecked.Value()) ? 1 : 0;

        char* impress = iupAttribGet(ih, "IMPRESS");
        if (impress)
        {
          if (checked)
            winuiToggleSetImageContent(ih, impress, 0);
          else
          {
            char* image = iupAttribGet(ih, "IMAGE");
            winuiToggleSetImageContent(ih, image, 0);
          }
        }

        winuiToggleCallAction(ih, checked);
      }
    });

    aux->keyDownToken = tb.PreviewKeyDown([ih](IInspectable const&, Input::KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(tb);

    winuiStoreHandle(ih, tb);
  }
  else if (ih->data->is_radio)
  {
    aux->controlType = IUPWINUI_TOGGLE_RADIOBUTTON;

    RadioButton rb;
    rb.HorizontalAlignment(HorizontalAlignment::Left);
    rb.VerticalAlignment(VerticalAlignment::Top);

    char groupName[50];
    snprintf(groupName, sizeof(groupName), "radio_%p", (void*)radio);
    rb.GroupName(iupwinuiStringToHString(groupName));

    if (title)
      rb.Content(box_value(iupwinuiProcessMnemonic(title, NULL)));

    aux->checkedToken = rb.Checked([ih](IInspectable const&, RoutedEventArgs const&) {
      winuiToggleCallAction(ih, 1);
    });

    aux->uncheckedToken = rb.Unchecked([ih](IInspectable const&, RoutedEventArgs const&) {
      winuiToggleCallAction(ih, 0);
    });

    aux->keyDownToken = rb.PreviewKeyDown([ih](IInspectable const&, Input::KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    if (!iupAttribGet(radio, "_IUPWINUI_LASTTOGGLE"))
    {
      iupAttribSet(ih, "VALUE", "ON");
    }
    iupAttribSet(radio, "_IUPWINUI_LASTTOGGLE", (char*)ih);

    if (!iupAttribGetHandleName(ih))
      iupAttribSetHandleName(ih);

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(rb);

    winuiStoreHandle(ih, rb);
  }
  else
  {
    aux->controlType = IUPWINUI_TOGGLE_CHECKBOX;

    CheckBox cb;
    cb.HorizontalAlignment(HorizontalAlignment::Left);
    cb.VerticalAlignment(VerticalAlignment::Top);

    if (title)
      cb.Content(box_value(iupwinuiProcessMnemonic(title, NULL)));

    aux->checkedToken = cb.Checked([ih](IInspectable const&, RoutedEventArgs const&) {
      winuiToggleCallAction(ih, 1);
    });

    aux->uncheckedToken = cb.Unchecked([ih](IInspectable const&, RoutedEventArgs const&) {
      winuiToggleCallAction(ih, 0);
    });

    aux->keyDownToken = cb.PreviewKeyDown([ih](IInspectable const&, Input::KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(cb);

    winuiStoreHandle(ih, cb);
  }

  winuiSetAux(ih, IUPWINUI_TOGGLE_AUX, aux);
  return IUP_NOERROR;
}

static void winuiToggleUnMapMethod(Ihandle* ih)
{
  IupWinUIToggleAux* aux = winuiGetAux<IupWinUIToggleAux>(ih, IUPWINUI_TOGGLE_AUX);

  if (ih->handle && aux)
  {
    switch (aux->controlType)
    {
    case IUPWINUI_TOGGLE_TOGGLESWITCH:
      {
        ToggleSwitch ts = winuiGetHandle<ToggleSwitch>(ih);
        if (ts)
        {
          if (aux->toggledToken)
            ts.Toggled(aux->toggledToken);
          if (aux->keyDownToken)
            ts.PreviewKeyDown(aux->keyDownToken);
        }
        winuiReleaseHandle<ToggleSwitch>(ih);
      }
      break;
    case IUPWINUI_TOGGLE_TOGGLEBUTTON:
      {
        ToggleButton tb = winuiGetHandle<ToggleButton>(ih);
        if (tb)
        {
          if (aux->clickToken)
            tb.Click(aux->clickToken);
          if (aux->keyDownToken)
            tb.PreviewKeyDown(aux->keyDownToken);
        }
        winuiReleaseHandle<ToggleButton>(ih);
      }
      break;
    case IUPWINUI_TOGGLE_RADIOBUTTON:
      {
        RadioButton rb = winuiGetHandle<RadioButton>(ih);
        if (rb)
        {
          if (aux->checkedToken)
            rb.Checked(aux->checkedToken);
          if (aux->uncheckedToken)
            rb.Unchecked(aux->uncheckedToken);
          if (aux->keyDownToken)
            rb.PreviewKeyDown(aux->keyDownToken);
        }
        winuiReleaseHandle<RadioButton>(ih);
      }
      break;
    case IUPWINUI_TOGGLE_CHECKBOX:
    default:
      {
        CheckBox cb = winuiGetHandle<CheckBox>(ih);
        if (cb)
        {
          if (aux->checkedToken)
            cb.Checked(aux->checkedToken);
          if (aux->uncheckedToken)
            cb.Unchecked(aux->uncheckedToken);
          if (aux->keyDownToken)
            cb.PreviewKeyDown(aux->keyDownToken);
        }
        winuiReleaseHandle<CheckBox>(ih);
      }
      break;
    }
  }

  winuiFreeAux<IupWinUIToggleAux>(ih, IUPWINUI_TOGGLE_AUX);
  ih->handle = NULL;
}

extern "C" void iupdrvToggleAddBorders(Ihandle* ih, int* x, int* y)
{
  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    iupdrvButtonAddBorders(ih, x, y);
  }
  else
  {
    *x += 8;
    *y += 5;
  }
}

extern "C" void iupdrvToggleAddCheckBox(Ihandle* ih, int* x, int* y, const char* str)
{
  (void)ih;
  *x += 20;
  if (*y < 32)
    *y = 32;

  if (str && str[0])
    *x += 8;
}

extern "C" void iupdrvToggleAddSwitch(Ihandle* ih, int* x, int* y, const char* str)
{
  (void)ih;
  *x += 44;
  if (*y < 30)
    *y = 30;

  if (str && str[0])
    *x += 10;
}

extern "C" void iupdrvToggleInitClass(Iclass* ic)
{
  ic->Map = winuiToggleMapMethod;
  ic->UnMap = winuiToggleUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winuiToggleSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "TITLE", winuiToggleGetTitleAttrib, winuiToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupToggle only */
  iupClassRegisterAttribute(ic, "VALUE", winuiToggleGetValueAttrib, winuiToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", NULL, winuiToggleSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winuiToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, winuiToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, winuiToggleSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, winuiToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
}
