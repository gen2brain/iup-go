/** \file
 * \brief WinUI Driver - Button Control
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
#include "iup_classbase.h"
#include "iup_button.h"
#include "iup_key.h"
#include "iup_register.h"
#include "iup_childtree.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;


static int winui_button_padding_x = 25;
static int winui_button_padding_y = 14;

static void winuiButtonClickHandler(Ihandle* ih)
{
  IFn cb = (IFn)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }
}

static int winuiButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  Button btn = winuiGetHandle<Button>(ih);
  if (btn)
  {
    char c = 0;
    hstring text = iupwinuiProcessMnemonic(value, &c);
    Border border = btn.Content().try_as<Border>();
    if (border)
    {
      TextBlock tb = border.Child().try_as<TextBlock>();
      if (tb)
        tb.Text(text);
    }
    if (c)
    {
      wchar_t wc = (wchar_t)c;
      btn.AccessKey(hstring(&wc, 1));
    }
  }
  return 1;
}

static char* winuiButtonGetTitleAttrib(Ihandle* ih)
{
  Button btn = winuiGetHandle<Button>(ih);
  if (btn)
  {
    Border border = btn.Content().try_as<Border>();
    if (border)
    {
      TextBlock tb = border.Child().try_as<TextBlock>();
      if (tb)
        return iupwinuiHStringToString(tb.Text());
    }
  }
  return NULL;
}

static int winuiButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
  Button btn = winuiGetHandle<Button>(ih);
  if (btn)
    btn.IsEnabled(iupStrBoolean(value));

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    int make_inactive = !iupStrBoolean(value);
    char* name = iupAttribGet(ih, "IMINACTIVE");
    if (!name)
      name = iupAttribGet(ih, "IMAGE");
    if (name)
    {
      void* imghandle = iupImageGetImage(name, ih, make_inactive, NULL);
      WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
      if (bitmap)
      {
        Border border = btn.Content().try_as<Border>();
        Image image = border ? border.Child().try_as<Image>() : nullptr;
        if (image)
          image.Source(bitmap);
      }
    }
  }

  return 0;
}

static void winuiButtonSetImage(Ihandle* ih, const char* name, int make_inactive)
{
  Button btn = winuiGetHandle<Button>(ih);
  if (!btn || !name)
    return;

  void* imghandle = iupImageGetImage(name, ih, make_inactive, NULL);
  WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
  if (!bitmap)
    return;

  Border border = btn.Content().try_as<Border>();
  if (border)
  {
    auto child = border.Child();
    if (child)
    {
      Image existingImage = child.try_as<Image>();
      if (!existingImage)
      {
        StackPanel sp = child.try_as<StackPanel>();
        if (sp && sp.Children().Size() > 0)
          existingImage = sp.Children().GetAt(0).try_as<Image>();
      }
      if (existingImage)
      {
        existingImage.Source(bitmap);
        return;
      }
    }
  }

  Image image;
  image.Source(bitmap);
  image.Stretch(Media::Stretch::None);

  if (!border)
  {
    border = Border();
    border.Background(SolidColorBrush(Microsoft::UI::Colors::Transparent()));
    btn.Content(border);
  }

  const char* title = iupAttribGet(ih, "TITLE");
  if ((ih->data->type & IUP_BUTTON_TEXT) && title && title[0])
  {
    StackPanel sp;
    sp.Orientation(Orientation::Horizontal);
    sp.Children().Append(image);

    char c = 0;
    hstring text = iupwinuiProcessMnemonic(title, &c);

    TextBlock tb;
    tb.Text(text);
    tb.Margin(ThicknessHelper::FromLengths(ih->data->spacing, 0, 0, 0));
    tb.VerticalAlignment(VerticalAlignment::Center);
    sp.Children().Append(tb);

    border.Child(sp);
    if (c)
    {
      wchar_t wc = (wchar_t)c;
      btn.AccessKey(hstring(&wc, 1));
    }
  }
  else
  {
    border.Child(image);
  }
}

static int winuiButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (iupdrvIsActive(ih))
      winuiButtonSetImage(ih, value, 0);
    else
    {
      char* iminactive = iupAttribGet(ih, "IMINACTIVE");
      if (iminactive)
        winuiButtonSetImage(ih, iminactive, 0);
      else
        winuiButtonSetImage(ih, value, 1);
    }
  }
  return 1;
}

static int winuiButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    if (!iupdrvIsActive(ih))
    {
      if (value)
        winuiButtonSetImage(ih, value, 0);
      else
      {
        char* name = iupAttribGet(ih, "IMAGE");
        winuiButtonSetImage(ih, name, 1);
      }
    }
  }
  return 1;
}

static int winuiButtonSetImPressAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->data->type & IUP_BUTTON_IMAGE)
    iupdrvRedrawNow(ih);
  return 1;
}

static TextBlock winuiButtonGetTextBlock(Ihandle* ih)
{
  Button btn = winuiGetHandle<Button>(ih);
  if (!btn)
    return nullptr;

  Border border = btn.Content().try_as<Border>();
  if (!border)
    return nullptr;

  auto child = border.Child();
  if (!child)
    return nullptr;

  TextBlock tb = child.try_as<TextBlock>();
  if (tb)
    return tb;

  StackPanel sp = child.try_as<StackPanel>();
  if (sp)
  {
    for (uint32_t i = 0; i < sp.Children().Size(); i++)
    {
      tb = sp.Children().GetAt(i).try_as<TextBlock>();
      if (tb)
        return tb;
    }
  }

  return nullptr;
}

static int winuiButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (!ih->handle)
    return 1;

  Windows::UI::Color color;
  color.A = 255;
  color.R = r;
  color.G = g;
  color.B = b;
  SolidColorBrush brush(color);

  Button btn = winuiGetHandle<Button>(ih);
  if (btn)
  {
    btn.Foreground(brush);
    btn.Resources().Insert(box_value(L"ButtonForeground"), brush);
    btn.Resources().Insert(box_value(L"ButtonForegroundPointerOver"), brush);
    btn.Resources().Insert(box_value(L"ButtonForegroundPressed"), brush);
  }

  TextBlock tb = winuiButtonGetTextBlock(ih);
  if (tb)
    tb.Foreground(brush);

  return 1;
}

static int winuiButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
    value = IupGetGlobal("DEFAULTBUTTONPADDING");

  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    Button btn = winuiGetHandle<Button>(ih);
    if (btn)
      btn.Padding(ThicknessHelper::FromLengths(ih->data->horiz_padding, ih->data->vert_padding, ih->data->horiz_padding, ih->data->vert_padding));
  }

  return 0;
}

static int winuiButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  char value1[30], value2[30];

  iupStrToStrStr(value, value1, value2, ':');

  if (iupStrEqualNoCase(value1, "ARIGHT"))
    ih->data->horiz_alignment = IUP_ALIGN_ARIGHT;
  else if (iupStrEqualNoCase(value1, "ALEFT"))
    ih->data->horiz_alignment = IUP_ALIGN_ALEFT;
  else
    ih->data->horiz_alignment = IUP_ALIGN_ACENTER;

  if (iupStrEqualNoCase(value2, "ABOTTOM"))
    ih->data->vert_alignment = IUP_ALIGN_ABOTTOM;
  else if (iupStrEqualNoCase(value2, "ATOP"))
    ih->data->vert_alignment = IUP_ALIGN_ATOP;
  else
    ih->data->vert_alignment = IUP_ALIGN_ACENTER;

  if (ih->handle)
  {
    Button btn = winuiGetHandle<Button>(ih);
    if (btn)
    {
      HorizontalAlignment halign = HorizontalAlignment::Center;
      if (ih->data->horiz_alignment == IUP_ALIGN_ALEFT)
        halign = HorizontalAlignment::Left;
      else if (ih->data->horiz_alignment == IUP_ALIGN_ARIGHT)
        halign = HorizontalAlignment::Right;
      btn.HorizontalContentAlignment(halign);

      VerticalAlignment valign = VerticalAlignment::Center;
      if (ih->data->vert_alignment == IUP_ALIGN_ATOP)
        valign = VerticalAlignment::Top;
      else if (ih->data->vert_alignment == IUP_ALIGN_ABOTTOM)
        valign = VerticalAlignment::Bottom;
      btn.VerticalContentAlignment(valign);
    }
  }

  return 1;
}

static char* winuiButtonGetAlignmentAttrib(Ihandle* ih)
{
  const char* horiz_str = "ACENTER";
  const char* vert_str = "ACENTER";

  if (ih->data->horiz_alignment == IUP_ALIGN_ARIGHT)
    horiz_str = "ARIGHT";
  else if (ih->data->horiz_alignment == IUP_ALIGN_ALEFT)
    horiz_str = "ALEFT";

  if (ih->data->vert_alignment == IUP_ALIGN_ABOTTOM)
    vert_str = "ABOTTOM";
  else if (ih->data->vert_alignment == IUP_ALIGN_ATOP)
    vert_str = "ATOP";

  return iupStrReturnStrf("%s:%s", horiz_str, vert_str);
}

static void winuiButtonPointerPressed(Ihandle* ih)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* impress = iupAttribGet(ih, "IMPRESS");
    if (impress)
      winuiButtonSetImage(ih, impress, 0);
  }
}

static void winuiButtonPointerReleased(Ihandle* ih)
{
  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    char* impress = iupAttribGet(ih, "IMPRESS");
    if (impress)
    {
      char* image = iupAttribGet(ih, "IMAGE");
      winuiButtonSetImage(ih, image, 0);
    }
  }
}

static int winuiButtonMapMethod(Ihandle* ih)
{
  const char* title = iupAttribGet(ih, "TITLE");
  const char* image = iupAttribGet(ih, "IMAGE");

  if (image)
  {
    ih->data->type = IUP_BUTTON_IMAGE;
    if (title)
      ih->data->type |= IUP_BUTTON_TEXT;
  }

  Button btn = Button();
  btn.HorizontalAlignment(HorizontalAlignment::Left);
  btn.VerticalAlignment(VerticalAlignment::Top);

  char* impress = iupAttribGet(ih, "IMPRESS");
  if ((ih->data->type & IUP_BUTTON_IMAGE && impress && !iupAttribGetBoolean(ih, "IMPRESSBORDER")) || iupAttribGetBoolean(ih, "FLAT"))
  {
    btn.BorderThickness(Thickness{0, 0, 0, 0});
    btn.Padding(Thickness{0, 0, 0, 0});
    btn.Background(SolidColorBrush(Microsoft::UI::Colors::Transparent()));
  }

  if (ih->data->type == IUP_BUTTON_IMAGE)
    btn.Padding(Thickness{0, 0, 0, 0});

  Border contentBorder;
  contentBorder.Background(SolidColorBrush(Microsoft::UI::Colors::Transparent()));

  if (ih->data->type & IUP_BUTTON_IMAGE)
  {
    void* imghandle = iupImageGetImage(image, ih, 0, NULL);
    WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
    if (bitmap)
    {
      Image img = Image();
      img.Source(bitmap);
      img.Stretch(Media::Stretch::None);

      if (ih->data->type & IUP_BUTTON_TEXT)
      {
        StackPanel sp;
        sp.Orientation(Orientation::Horizontal);
        sp.Children().Append(img);

        char c = 0;
        hstring text = iupwinuiProcessMnemonic(title, &c);

        TextBlock tb;
        tb.Text(text);
        tb.Margin(ThicknessHelper::FromLengths(ih->data->spacing, 0, 0, 0));
        tb.VerticalAlignment(VerticalAlignment::Center);
        sp.Children().Append(tb);

        contentBorder.Child(sp);
        if (c)
        {
          wchar_t wc = (wchar_t)c;
          btn.AccessKey(hstring(&wc, 1));
        }
      }
      else
      {
        contentBorder.Child(img);
      }
    }
  }
  else if (title)
  {
    char c = 0;
    hstring text = iupwinuiProcessMnemonic(title, &c);
    TextBlock tb;
    tb.Text(text);
    contentBorder.Child(tb);
    if (c)
    {
      wchar_t wc = (wchar_t)c;
      btn.AccessKey(hstring(&wc, 1));
    }
  }

  btn.Content(contentBorder);

  IupWinUIButtonAux* aux = new IupWinUIButtonAux();
  aux->clickToken = btn.Click([ih](IInspectable const&, RoutedEventArgs const&) {
    winuiButtonClickHandler(ih);
  });

  if (impress && (ih->data->type & IUP_BUTTON_IMAGE))
  {
    aux->pointerPressedToken = contentBorder.PointerPressed([ih](IInspectable const&, Input::PointerRoutedEventArgs const&) {
      winuiButtonPointerPressed(ih);
    });

    aux->captureLostToken = btn.PointerCaptureLost([ih](IInspectable const&, Input::PointerRoutedEventArgs const&) {
      winuiButtonPointerReleased(ih);
    });
  }

  aux->buttonPressedToken = contentBorder.PointerPressed([ih](IInspectable const&, Input::PointerRoutedEventArgs const& args) {
    IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
    if (cb)
    {
      Button b = winuiGetHandle<Button>(ih);
      auto point = args.GetCurrentPoint(b);
      auto props = point.Properties();
      int button = iupwinuiGetPointerButton(props);
      int x = (int)point.Position().X;
      int y = (int)point.Position().Y;
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus(iupwinuiGetModifierKeys(), button, status, 0);
      int ret = cb(ih, button, 1, x, y, status);
      if (ret == IUP_CLOSE)
        IupExitLoop();
    }
  });

  aux->buttonReleasedToken = btn.PointerReleased([ih](IInspectable const&, Input::PointerRoutedEventArgs const& args) {
    IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
    if (cb)
    {
      Button b = winuiGetHandle<Button>(ih);
      auto point = args.GetCurrentPoint(b);
      auto props = point.Properties();
      int button = iupwinuiGetPointerReleasedButton(props);
      int x = (int)point.Position().X;
      int y = (int)point.Position().Y;
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus(iupwinuiGetModifierKeys(), button, status, 0);
      int ret = cb(ih, button, 0, x, y, status);
      if (ret == IUP_CLOSE)
        IupExitLoop();
    }
  });

  aux->buttonCaptureLostToken = btn.PointerCaptureLost([ih](IInspectable const&, Input::PointerRoutedEventArgs const& args) {
    IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
    if (cb)
    {
      Button b = winuiGetHandle<Button>(ih);
      auto point = args.GetCurrentPoint(b);
      auto props = point.Properties();
      int button = iupwinuiGetPointerReleasedButton(props);
      int x = (int)point.Position().X;
      int y = (int)point.Position().Y;
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      iupwinuiButtonKeySetStatus(iupwinuiGetModifierKeys(), button, status, 0);
      int ret = cb(ih, button, 0, x, y, status);
      if (ret == IUP_CLOSE)
        IupExitLoop();
    }
  });

  aux->keyDownToken = btn.PreviewKeyDown([ih](IInspectable const&, Input::KeyRoutedEventArgs const& args) {
    if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
      args.Handled(true);
  });

  winuiSetAux(ih, IUPWINUI_BUTTON_AUX, aux);

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(btn);

  winuiStoreHandle(ih, btn);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupwinuiSetCanFocus(ih->handle, 0);

  return IUP_NOERROR;
}

static void winuiButtonUnMapMethod(Ihandle* ih)
{
  IupWinUIButtonAux* aux = winuiGetAux<IupWinUIButtonAux>(ih, IUPWINUI_BUTTON_AUX);

  if (ih->handle && aux)
  {
    Button btn = winuiGetHandle<Button>(ih);
    if (btn)
    {
      if (aux->clickToken)
        btn.Click(aux->clickToken);
      if (aux->keyDownToken)
        btn.PreviewKeyDown(aux->keyDownToken);
      Border contentBorder = btn.Content().try_as<Border>();
      if (contentBorder)
      {
        if (aux->pointerPressedToken)
          contentBorder.PointerPressed(aux->pointerPressedToken);
        if (aux->buttonPressedToken)
          contentBorder.PointerPressed(aux->buttonPressedToken);
      }
      if (aux->buttonReleasedToken)
        btn.PointerReleased(aux->buttonReleasedToken);
      if (aux->buttonCaptureLostToken)
        btn.PointerCaptureLost(aux->buttonCaptureLostToken);
      if (aux->captureLostToken)
        btn.PointerCaptureLost(aux->captureLostToken);
    }
    winuiReleaseHandle<Button>(ih);
  }

  winuiFreeAux<IupWinUIButtonAux>(ih, IUPWINUI_BUTTON_AUX);
  ih->handle = NULL;
}

extern "C" void iupdrvButtonAddBorders(Ihandle* ih, int* x, int* y)
{
  int border_size = 8;

  if (ih)
  {
    char* image = iupAttribGet(ih, "IMAGE");
    char* title = iupAttribGet(ih, "TITLE");
    char* bgcolor = iupAttribGet(ih, "BGCOLOR");
    int has_image = (image != NULL);
    int has_text = (title != NULL && *title != 0);

    if (!has_image && !has_text && bgcolor != NULL)
    {
      int charwidth, charheight;
      iupdrvFontGetCharSize(ih, &charwidth, &charheight);
      *x += charwidth * 4 + border_size;
      *y += charheight + border_size;
      return;
    }

    if (has_image && !has_text)
    {
      *x += border_size;
      *y += border_size;
      return;
    }
  }

  *x += winui_button_padding_x;
  *y += winui_button_padding_y;
}

extern "C" void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = winuiButtonMapMethod;
  ic->UnMap = winuiButtonUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", NULL, winuiButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winuiButtonSetFgColorAttrib, "DLGFGCOLOR", NULL, IUPAF_NOT_MAPPED);

  /* Special */
  iupClassRegisterAttribute(ic, "TITLE", winuiButtonGetTitleAttrib, winuiButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupButton only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", winuiButtonGetAlignmentAttrib, winuiButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winuiButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, winuiButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, winuiButtonSetImPressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, winuiButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
}
