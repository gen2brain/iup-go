/** \file
 * \brief WinUI Driver TIPS management
 *
 * Uses native ToolTip/ToolTipService for tooltip display.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::Foundation;


static ToolTip winuiTipCreateStyled(Ihandle* ih, const char* text)
{
  unsigned char r, g, b;
  const char* value;

  ToolTip tip;

  TextBlock tb;
  tb.TextWrapping(TextWrapping::Wrap);
  tb.MaxWidth(400);
  tb.Text(iupwinuiStringToHString(text));

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
        tb.FontSize((double)(-size));
      else
      {
        HDC hdc = GetDC(NULL);
        double dpi = (double)GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
        tb.FontSize((double)size * dpi / 72.0);
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

  DependencyObject elem = winuiGetHandle<DependencyObject>(ih);
  if (!elem)
    return nullptr;

  IInspectable obj = ToolTipService::GetToolTip(elem);
  if (!obj)
    return nullptr;

  return obj.try_as<ToolTip>();
}


IUP_DRV_API void iupwinuiTipsDestroy(Ihandle* ih)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return;

  DependencyObject elem = winuiGetHandle<DependencyObject>(ih);
  if (elem)
    ToolTipService::SetToolTip(elem, nullptr);
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return 0;

  DependencyObject elem = winuiGetHandle<DependencyObject>(ih);
  if (!elem)
    return 0;

  if (value)
  {
    int need_styled = iupAttribGet(ih, "TIPFONT") ||
                      iupAttribGet(ih, "TIPBGCOLOR") ||
                      iupAttribGet(ih, "TIPFGCOLOR");

    IFnii tips_cb = (IFnii)IupGetCallback(ih, "TIPS_CB");

    if (need_styled || tips_cb)
    {
      ToolTip tip = winuiTipCreateStyled(ih, value);

      if (tips_cb)
      {
        tip.Opened([ih](IInspectable const&, RoutedEventArgs const&) {
          if (!iupObjectCheck(ih))
            return;

          IFnii cb = (IFnii)IupGetCallback(ih, "TIPS_CB");
          if (cb)
          {
            int x, y;
            iupdrvGetCursorPos(&x, &y);
            iupdrvScreenToClient(ih, &x, &y);
            cb(ih, x, y);
          }

          const char* tip_text = iupAttribGet(ih, "TIP");
          if (tip_text)
          {
            DependencyObject elem2 = winuiGetHandle<DependencyObject>(ih);
            if (elem2)
            {
              IInspectable obj = ToolTipService::GetToolTip(elem2);
              ToolTip tt = obj ? obj.try_as<ToolTip>() : nullptr;
              if (tt)
              {
                TextBlock tb = tt.Content().try_as<TextBlock>();
                if (tb)
                  tb.Text(iupwinuiStringToHString(tip_text));
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

  return 1;
}

extern "C" IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  if (!ih || !ih->handle || winuiHandleIsHWND(ih))
    return 0;

  DependencyObject elem = winuiGetHandle<DependencyObject>(ih);
  if (!elem)
    return 0;

  if (iupStrBoolean(value))
  {
    const char* tip = iupAttribGet(ih, "TIP");
    if (!tip)
      return 0;

    IInspectable obj = ToolTipService::GetToolTip(elem);
    ToolTip tt = obj ? obj.try_as<ToolTip>() : nullptr;

    if (!tt)
    {
      tt = ToolTip();
      tt.Content(box_value(iupwinuiStringToHString(tip)));
      ToolTipService::SetToolTip(elem, tt);
    }

    tt.IsOpen(true);
  }
  else
  {
    ToolTip tt = winuiTipGetToolTip(ih);
    if (tt)
      tt.IsOpen(false);
  }

  return 0;
}

extern "C" IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  ToolTip tt = winuiTipGetToolTip(ih);
  if (tt)
    return iupStrReturnBoolean(tt.IsOpen() ? 1 : 0);

  return NULL;
}
