/** \file
 * \brief WinUI Driver TIPS management
 *
 * Uses native ToolTip/ToolTipService for tooltip display.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_markup.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::Foundation;


static void winuiTipSetText(Ihandle* ih, TextBlock const& tb, const char* text)
{
  if (iupAttribGetBoolean(ih, "TIPMARKUP"))
    iupwinuiApplyMarkupToTextBlock(tb, text);
  else
    tb.Text(iupwinuiStringToHString(text));
}

static ToolTip winuiTipCreateStyled(Ihandle* ih, const char* text)
{
  unsigned char r, g, b;
  const char* value;

  ToolTip tip;

  TextBlock tb;
  tb.TextWrapping(TextWrapping::Wrap);
  tb.MaxWidth(400);
  winuiTipSetText(ih, tb, text);

  value = iupAttribGet(ih, "TIPFONT");
  if (value && !iupStrEqualNoCase(value, "SYSTEM"))
  {
    char typeface[50];
    int size, is_bold, is_italic, is_underline, is_strikeout;
    if (iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    {
      wchar_t wtypeface[50];
      MultiByteToWideChar(CP_UTF8, 0, typeface, -1, wtypeface, 50);
      tb.FontFamily(FontFamily(wtypeface));

      if (size < 0)
        tb.FontSize(static_cast<double>(-size));
      else
      {
        HDC hdc = GetDC(nullptr);
        auto dpi = static_cast<double>(GetDeviceCaps(hdc, LOGPIXELSY));
        ReleaseDC(nullptr, hdc);
        tb.FontSize(static_cast<double>(size) * dpi / 72.0);
      }

      tb.FontWeight(is_bold ?
        Windows::UI::Text::FontWeights::Bold() :
        Windows::UI::Text::FontWeights::Normal());
      tb.FontStyle(is_italic ?
        Windows::UI::Text::FontStyle::Italic :
        Windows::UI::Text::FontStyle::Normal);
    }
  }

  value = iupAttribGet(ih, "TIPFGCOLOR");
  if (value && iupStrToRGB(value, &r, &g, &b))
  {
    Windows::UI::Color c; c.A = 255; c.R = r; c.G = g; c.B = b;
    tb.Foreground(SolidColorBrush(c));
  }

  value = iupAttribGet(ih, "TIPBGCOLOR");
  if (value && iupStrToRGB(value, &r, &g, &b))
  {
    Windows::UI::Color c; c.A = 255; c.R = r; c.G = g; c.B = b;
    tip.Background(SolidColorBrush(c));
  }

  tip.Content(tb);
  return tip;
}

static ToolTip winuiTipGetToolTip(Ihandle* ih)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return nullptr;

  auto elem = winuiGetHandle<DependencyObject>(ih);
  if (!elem)
    return nullptr;

  IInspectable obj = ToolTipService::GetToolTip(elem);
  if (!obj)
    return nullptr;

  return obj.try_as<ToolTip>();
}


static void winuiTipClose(DependencyObject elem)
{
  IInspectable obj = ToolTipService::GetToolTip(elem);
  if (obj)
  {
    ToolTip tt = obj.try_as<ToolTip>();
    if (tt && tt.IsOpen())
      tt.IsOpen(false);
  }
}

IUP_DRV_API void iupwinuiTipsDestroy(Ihandle* ih)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return;

  auto elem = winuiGetHandle<DependencyObject>(ih);
  if (elem)
  {
    winuiTipClose(elem);
    ToolTipService::SetToolTip(elem, nullptr);
  }
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return 0;

  auto elem = winuiGetHandle<DependencyObject>(ih);
  if (!elem)
    return 0;

  winuiTipClose(elem);

  if (value)
  {
    int need_styled = iupAttribGet(ih, "TIPFONT") ||
                      iupAttribGet(ih, "TIPBGCOLOR") ||
                      iupAttribGet(ih, "TIPFGCOLOR") ||
                      iupAttribGet(ih, "TIPRECT") ||
                      iupAttribGetBoolean(ih, "TIPMARKUP");

    auto tips_cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "TIPS_CB"));

    if (need_styled || tips_cb)
    {
      ToolTip tip = winuiTipCreateStyled(ih, value);

      if (tips_cb || iupAttribGet(ih, "TIPRECT"))
      {
        tip.Opened([ih](IInspectable const&, RoutedEventArgs const&) {
          if (!iupObjectCheck(ih))
            return;

          int x, y;
          iupdrvGetCursorPos(&x, &y);
          iupdrvScreenToClient(ih, &x, &y);

          const char* rect = iupAttribGet(ih, "TIPRECT");
          if (rect)
          {
            int x1, y1, x2, y2;
            if (sscanf(rect, "%d %d %d %d", &x1, &y1, &x2, &y2) == 4 &&
                (x < x1 || x > x2 || y < y1 || y > y2))
            {
              auto owner = winuiGetHandle<DependencyObject>(ih);
              if (owner)
                winuiTipClose(owner);
              return;
            }
          }

          auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "TIPS_CB"));
          if (cb)
            cb(ih, x, y);

          const char* tip_text = iupAttribGet(ih, "TIP");
          if (tip_text)
          {
            auto elem2 = winuiGetHandle<DependencyObject>(ih);
            if (elem2)
            {
              IInspectable obj = ToolTipService::GetToolTip(elem2);
              ToolTip tt = obj ? obj.try_as<ToolTip>() : nullptr;
              if (tt)
              {
                TextBlock tb = tt.Content().try_as<TextBlock>();
                if (tb)
                  winuiTipSetText(ih, tb, tip_text);
              }
            }
          }
        });
      }

      ToolTipService::SetToolTip(elem, tip);
    }
    else
    {
      ToolTipService::SetToolTip(elem, box_value(iupwinuiStringToHString(value)));
    }
  }
  else
  {
    ToolTipService::SetToolTip(elem, nullptr);
  }

  if (!iupAttribGet(ih, "ACCESSIBLEDESCRIPTION"))
  {
    if (value && iupAttribGetBoolean(ih, "TIPMARKUP"))
    {
      char* plain = iupMarkupStripTags(value);
      iupdrvSetAccessibleDescription(ih, plain);
      free(plain);
    }
    else
      iupdrvSetAccessibleDescription(ih, value);
  }

  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return 0;

  auto elem = winuiGetHandle<DependencyObject>(ih);
  if (!elem)
    return 0;

  if (iupStrBoolean(value))
  {
    const char* tip = iupAttribGet(ih, "TIP");
    if (!tip)
    {
      winuiTipClose(elem);
      return 0;
    }

    IInspectable obj = ToolTipService::GetToolTip(elem);
    ToolTip tt = obj ? obj.try_as<ToolTip>() : nullptr;

    if (!tt)
    {
      tt = winuiTipCreateStyled(ih, tip);
      ToolTipService::SetToolTip(elem, tt);
    }

    int sx, sy;
    iupdrvGetCursorPos(&sx, &sy);

    Ihandle* dialog = IupGetDialog(ih);
    POINT pt;
    pt.x = sx;
    pt.y = sy;
    ScreenToClient(reinterpret_cast<HWND>(dialog->handle), &pt);

    UIElement uiElem = elem.try_as<UIElement>();
    if (uiElem)
    {
      auto transform = uiElem.TransformToVisual(nullptr);
      auto origin = transform.TransformPoint({0, 0});

      float cx = static_cast<float>(pt.x) - origin.X;
      float cy = static_cast<float>(pt.y) - origin.Y;

      tt.PlacementRect(Windows::Foundation::Rect{cx, cy, 1.0f, 1.0f});
      tt.Placement(Controls::Primitives::PlacementMode::Bottom);
    }

    tt.IsOpen(true);
  }
  else
  {
    winuiTipClose(elem);
  }

  return 0;
}

extern "C" IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  ToolTip tt = winuiTipGetToolTip(ih);
  if (tt)
    return iupStrReturnBoolean(tt.IsOpen() ? 1 : 0);

  return nullptr;
}
