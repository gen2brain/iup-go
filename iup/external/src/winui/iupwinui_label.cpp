/** \file
 * \brief WinUI Driver - Label Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_label.h"
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
using namespace Windows::UI;
using namespace Windows::UI::Text;


#define IUPWINUI_LABEL_INNER "_IUPWINUI_LABEL_INNER"

static TextBlock winuiLabelGetTextBlock(Ihandle* ih)
{
  char* ptr = iupAttribGet(ih, IUPWINUI_LABEL_INNER);
  if (ptr)
  {
    IInspectable obj{nullptr};
    winrt::copy_from_abi(obj, ptr);
    return obj.try_as<TextBlock>();
  }
  return nullptr;
}

static Image winuiLabelGetImage(Ihandle* ih)
{
  char* ptr = iupAttribGet(ih, IUPWINUI_LABEL_INNER);
  if (ptr)
  {
    IInspectable obj{nullptr};
    winrt::copy_from_abi(obj, ptr);
    return obj.try_as<Image>();
  }
  return nullptr;
}

static int winuiLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    TextBlock textBlock = winuiLabelGetTextBlock(ih);
    if (textBlock)
      textBlock.Text(iupwinuiProcessMnemonic(value, NULL));
  }
  return 1;
}

static char* winuiLabelGetTitleAttrib(Ihandle* ih)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    TextBlock textBlock = winuiLabelGetTextBlock(ih);
    if (textBlock)
      return iupwinuiHStringToString(textBlock.Text());
  }
  return NULL;
}

static int winuiLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    TextBlock textBlock = winuiLabelGetTextBlock(ih);
    if (textBlock)
    {
      char value1[30], value2[30];
      iupStrToStrStr(value, value1, value2, ':');

      if (iupStrEqualNoCase(value1, "ACENTER"))
        textBlock.TextAlignment(TextAlignment::Center);
      else if (iupStrEqualNoCase(value1, "ARIGHT"))
        textBlock.TextAlignment(TextAlignment::Right);
      else
        textBlock.TextAlignment(TextAlignment::Left);

      if (iupStrEqualNoCase(value2, "ABOTTOM"))
        textBlock.VerticalAlignment(VerticalAlignment::Bottom);
      else if (iupStrEqualNoCase(value2, "ATOP"))
        textBlock.VerticalAlignment(VerticalAlignment::Top);
      else
        textBlock.VerticalAlignment(VerticalAlignment::Center);
    }
  }
  return 1;
}

static int winuiLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    Image image = winuiLabelGetImage(ih);
    if (image && value)
    {
      int make_inactive = !iupdrvIsActive(ih);
      void* imghandle = iupImageGetImage(value, ih, make_inactive, NULL);
      WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
      if (bitmap)
        image.Source(bitmap);
    }
  }
  return 1;
}

static int winuiLabelSetActiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    int make_inactive = !iupStrBoolean(value);
    char* name = iupAttribGet(ih, "IMINACTIVE");
    if (!name)
      name = iupAttribGet(ih, "IMAGE");
    if (name)
    {
      Image image = winuiLabelGetImage(ih);
      if (image)
      {
        void* imghandle = iupImageGetImage(name, ih, make_inactive, NULL);
        WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
        if (bitmap)
          image.Source(bitmap);
      }
    }
  }
  return 0;
}

static void winuiLabelUpdateFont(Ihandle* ih)
{
  if (ih->data->type != IUP_LABEL_TEXT)
    return;

  TextBlock textBlock = winuiLabelGetTextBlock(ih);
  if (textBlock)
    iupwinuiUpdateTextBlockFont(ih, textBlock);
}

static int winuiLabelMapMethod(Ihandle* ih)
{
  const char* value = iupAttribGet(ih, "SEPARATOR");
  const char* imagename = iupAttribGet(ih, "IMAGE");

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);

  Border border;
  border.HorizontalAlignment(HorizontalAlignment::Left);
  border.VerticalAlignment(VerticalAlignment::Top);

  if (value)
  {
    if (iupStrEqualNoCase(value, "HORIZONTAL"))
    {
      ih->data->type = IUP_LABEL_SEP_HORIZ;
      border.Height(1);
      border.HorizontalAlignment(HorizontalAlignment::Stretch);
    }
    else
    {
      ih->data->type = IUP_LABEL_SEP_VERT;
      border.Width(1);
      border.VerticalAlignment(VerticalAlignment::Stretch);
    }

    unsigned char r, g, b;
    const char* fgcolor = IupGetGlobal("DLGFGCOLOR");
    if (iupStrToRGB(fgcolor, &r, &g, &b))
    {
      Color color;
      color.A = 48;
      color.R = r;
      color.G = g;
      color.B = b;
      border.Background(SolidColorBrush(color));
    }
  }
  else if (imagename)
  {
    ih->data->type = IUP_LABEL_IMAGE;

    Image image = Image();
    image.Stretch(Media::Stretch::None);

    void* imghandle = iupImageGetImage(imagename, ih, 0, NULL);
    WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
    if (bitmap)
      image.Source(bitmap);

    border.Child(image);

    void* innerPtr = nullptr;
    winrt::copy_to_abi(image, innerPtr);
    iupAttribSet(ih, IUPWINUI_LABEL_INNER, (char*)innerPtr);
  }
  else
  {
    ih->data->type = IUP_LABEL_TEXT;

    TextBlock textBlock = TextBlock();
    textBlock.VerticalAlignment(VerticalAlignment::Center);

    const char* title = iupAttribGet(ih, "TITLE");
    if (title)
      textBlock.Text(iupwinuiProcessMnemonic(title, NULL));

    border.Child(textBlock);

    void* innerPtr = nullptr;
    winrt::copy_to_abi(textBlock, innerPtr);
    iupAttribSet(ih, IUPWINUI_LABEL_INNER, (char*)innerPtr);
  }

  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    border.Background(SolidColorBrush(Microsoft::UI::Colors::Transparent()));

    IupWinUILabelAux* aux = new IupWinUILabelAux();

    aux->pointerPressedToken = border.PointerPressed([ih](IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) {
      IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
      if (cb)
      {
        auto point = args.GetCurrentPoint(winuiGetHandle<Border>(ih));
        auto props = point.Properties();
        int button = iupwinuiGetPointerButton(props);
        int x = (int)point.Position().X;
        int y = (int)point.Position().Y;
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        iupwinuiButtonKeySetStatus(iupwinuiGetModifierKeys(), button, status, 0);
        cb(ih, button, 1, x, y, status);
      }
      args.Handled(true);
    });

    aux->pointerReleasedToken = border.PointerReleased([ih](IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args) {
      IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
      if (cb)
      {
        auto point = args.GetCurrentPoint(winuiGetHandle<Border>(ih));
        auto props = point.Properties();
        int button = iupwinuiGetPointerReleasedButton(props);
        int x = (int)point.Position().X;
        int y = (int)point.Position().Y;
        char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
        iupwinuiButtonKeySetStatus(iupwinuiGetModifierKeys(), button, status, 0);
        cb(ih, button, 0, x, y, status);
      }
      args.Handled(true);
    });

    aux->pointerEnteredToken = border.PointerEntered([ih](IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&) {
      Icallback cb = IupGetCallback(ih, "ENTERWINDOW_CB");
      if (cb)
        cb(ih);
    });

    aux->pointerExitedToken = border.PointerExited([ih](IInspectable const&, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&) {
      Icallback cb = IupGetCallback(ih, "LEAVEWINDOW_CB");
      if (cb)
        cb(ih);
    });

    winuiSetAux(ih, IUPWINUI_LABEL_AUX, aux);
  }

  if (parentCanvas)
    parentCanvas.Children().Append(border);

  winuiStoreHandle(ih, border);

  if (ih->data->type == IUP_LABEL_TEXT)
    winuiLabelUpdateFont(ih);

  return IUP_NOERROR;
}

static void winuiLabelUnMapMethod(Ihandle* ih)
{
  IupWinUILabelAux* aux = winuiGetAux<IupWinUILabelAux>(ih, IUPWINUI_LABEL_AUX);

  if (ih->handle && aux)
  {
    Border border = winuiGetHandle<Border>(ih);
    if (border)
    {
      if (aux->pointerPressedToken)
        border.PointerPressed(aux->pointerPressedToken);
      if (aux->pointerReleasedToken)
        border.PointerReleased(aux->pointerReleasedToken);
      if (aux->pointerEnteredToken)
        border.PointerEntered(aux->pointerEnteredToken);
      if (aux->pointerExitedToken)
        border.PointerExited(aux->pointerExitedToken);
    }
  }

  winuiFreeAux<IupWinUILabelAux>(ih, IUPWINUI_LABEL_AUX);
  iupAttribSet(ih, IUPWINUI_LABEL_INNER, nullptr);

  if (ih->handle)
    winuiReleaseHandle<Border>(ih);

  ih->handle = NULL;
}

static int winuiLabelSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (ih->data->type == IUP_LABEL_TEXT && iupStrToRGB(value, &r, &g, &b))
  {
    TextBlock textBlock = winuiLabelGetTextBlock(ih);
    if (textBlock)
    {
      Color color;
      color.A = 255;
      color.R = r;
      color.G = g;
      color.B = b;
      textBlock.Foreground(SolidColorBrush(color));
    }
  }
  return 1;
}

static int winuiLabelSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_SEP_HORIZ || ih->data->type == IUP_LABEL_SEP_VERT)
    return 1;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Border border = winuiGetHandle<Border>(ih);
  if (border)
  {
    Color color;
    color.A = 255;
    color.R = r;
    color.G = g;
    color.B = b;
    border.Background(SolidColorBrush(color));
  }
  return 1;
}

static int winuiLabelSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  winuiLabelUpdateFont(ih);
  return 1;
}

extern "C" void iupdrvLabelAddExtraPadding(Ihandle* ih, int* x, int* y)
{
  (void)ih;
  (void)x;
  (void)y;
}

extern "C" void iupdrvLabelInitClass(Iclass* ic)
{
  ic->Map = winuiLabelMapMethod;
  ic->UnMap = winuiLabelUnMapMethod;

  iupClassRegisterAttribute(ic, "TITLE", winuiLabelGetTitleAttrib, winuiLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, winuiLabelSetAlignmentAttrib, "ALEFT:ACENTER", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, winuiLabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVE", NULL, winuiLabelSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winuiLabelSetFgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winuiLabelSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FONT", NULL, winuiLabelSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
}
