/** \file
 * \brief WinUI Driver - Text Control
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
#include "iup_array.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_text.h"
#include "iup_register.h"
#include "iup_childtree.h"
#include "iup_focus.h"
#include "iup_key.h"
#include "iup_drvinfo.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Text.h>

using namespace winrt;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Text;
using namespace Windows::Foundation;
using namespace Microsoft::UI::Dispatching;


static ScrollViewer winuiTextFindScrollViewer(DependencyObject parent)
{
  int count = VisualTreeHelper::GetChildrenCount(parent);
  for (int i = 0; i < count; i++)
  {
    auto child = VisualTreeHelper::GetChild(parent, i);
    auto sv = child.try_as<ScrollViewer>();
    if (sv)
      return sv;

    sv = winuiTextFindScrollViewer(child);
    if (sv)
      return sv;
  }
  return nullptr;
}

static TextBox winuiFindTextBox(DependencyObject parent)
{
  int count = VisualTreeHelper::GetChildrenCount(parent);
  for (int i = 0; i < count; i++)
  {
    auto child = VisualTreeHelper::GetChild(parent, i);
    auto tb = child.try_as<TextBox>();
    if (tb)
      return tb;
    tb = winuiFindTextBox(child);
    if (tb)
      return tb;
  }
  return nullptr;
}

static bool winuiRemoveDeleteButton(DependencyObject parent)
{
  int count = VisualTreeHelper::GetChildrenCount(parent);
  for (int i = 0; i < count; i++)
  {
    auto child = VisualTreeHelper::GetChild(parent, i);
    auto fe = child.try_as<FrameworkElement>();
    if (fe && fe.Name() == L"DeleteButton")
    {
      auto panel = parent.try_as<Panel>();
      if (panel)
      {
        auto uiElem = child.try_as<UIElement>();
        if (uiElem)
        {
          uint32_t index;
          if (panel.Children().IndexOf(uiElem, index))
          {
            panel.Children().RemoveAt(index);
            return true;
          }
        }
      }
      return false;
    }
    if (winuiRemoveDeleteButton(child))
      return true;
  }
  return false;
}

static void winuiTextSetScrollBarArrowCursors(DependencyObject parent)
{
  using namespace Microsoft::UI::Xaml::Controls::Primitives;

  int count = VisualTreeHelper::GetChildrenCount(parent);
  for (int i = 0; i < count; i++)
  {
    auto child = VisualTreeHelper::GetChild(parent, i);
    auto sb = child.try_as<ScrollBar>();
    if (sb)
    {
      auto protectedUI = sb.try_as<IUIElementProtected>();
      if (protectedUI)
        protectedUI.ProtectedCursor(Microsoft::UI::Input::InputSystemCursor::Create(Microsoft::UI::Input::InputSystemCursorShape::Arrow));
    }
    else
      winuiTextSetScrollBarArrowCursors(child);
  }
}

static void winuiTextCallValueChanged(Ihandle* ih)
{
  if (ih->data->disable_callbacks)
    return;

  if (iupAttribGet(ih, "_IUPWINUI_IGNORE_VALUECHANGED"))
  {
    iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", NULL);
    return;
  }

  IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (cb)
  {
    int ret = cb(ih);
    if (ret == IUP_CLOSE)
      IupExitLoop();
  }
}

static void winuiTextBoxTextChanged(Ihandle* ih)
{
  if (ih->data->disable_callbacks)
  {
    iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", NULL);
    return;
  }

  winuiTextCallValueChanged(ih);
}

static void winuiTextBeforeTextChanging(Ihandle* ih, TextBox const& sender, TextBoxBeforeTextChangingEventArgs const& args)
{
  if (ih->data->disable_callbacks)
    return;

  IFnis cb = (IFnis)IupGetCallback(ih, "ACTION");
  if (!cb && !ih->data->mask && !ih->data->nc)
    return;

  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux)
    return;

  hstring newHStr = args.NewText();
  char* temp_new = iupwinuiHStringToString(newHStr);
  char* new_value = iupStrDup(temp_new ? temp_new : "");

  if (ih->data->nc && (int)strlen(new_value) > ih->data->nc)
  {
    free(new_value);
    args.Cancel(true);
    return;
  }

  if (ih->data->mask && iupMaskCheck((Imask*)ih->data->mask, new_value) == 0)
  {
    IFns fail_cb = (IFns)IupGetCallback(ih, "MASKFAIL_CB");
    if (fail_cb) fail_cb(ih, new_value);
    free(new_value);
    args.Cancel(true);
    return;
  }

  if (cb)
  {
    int oldLen = (int)aux->savedText.size();
    int newLen = (int)newHStr.size();

    const wchar_t* oldStr = aux->savedText.c_str();
    const wchar_t* newStr = newHStr.c_str();
    int insertPos = 0;
    while (insertPos < oldLen && insertPos < newLen && oldStr[insertPos] == newStr[insertPos])
      insertPos++;

    int key = 0;
    if (newLen == oldLen + 1 && insertPos < newLen)
    {
      wchar_t wch = newStr[insertPos];
      if (wch > 0 && wch < 128)
        key = (int)wch;
    }

    int cb_ret = cb(ih, key, new_value);
    if (cb_ret == IUP_IGNORE)
    {
      free(new_value);
      args.Cancel(true);
      return;
    }
    else if (cb_ret == IUP_CLOSE)
    {
      IupExitLoop();
      free(new_value);
      args.Cancel(true);
      return;
    }
    else if (cb_ret != 0 && key != 0 && cb_ret != IUP_DEFAULT && cb_ret != IUP_CONTINUE)
    {
      args.Cancel(true);

      int replaceCh = cb_ret;
      void* dq_ptr = iupwinuiGetDispatcherQueue();
      if (dq_ptr)
      {
        Windows::Foundation::IInspectable dq_obj{nullptr};
        winrt::copy_from_abi(dq_obj, dq_ptr);
        DispatcherQueue dq = dq_obj.as<DispatcherQueue>();

        dq.TryEnqueue([ih, replaceCh, insertPos]()
        {
          if (!iupObjectCheck(ih))
            return;

          IupWinUITextAux* a = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
          if (!a)
            return;

          TextBox tb = winuiGetHandle<TextBox>(ih);
          if (!tb)
            return;

          std::wstring text(a->savedText);
          text.insert(text.begin() + insertPos, (wchar_t)replaceCh);

          iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", "1");
          ih->data->disable_callbacks = 1;
          tb.Text(hstring(text));
          tb.SelectionStart(insertPos + 1);
          tb.SelectionLength(0);
          ih->data->disable_callbacks = 0;

          a->savedText = text;
        });
      }

      free(new_value);
      return;
    }
  }

  aux->savedText = std::wstring(newHStr.c_str(), newHStr.size());
  free(new_value);
}

static void winuiTextSpinValueChanged(Ihandle* ih, NumberBox const& nb, NumberBoxValueChangedEventArgs const& args)
{
  (void)nb;

  if (ih->data->disable_callbacks)
  {
    iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", NULL);
    return;
  }

  if (iupAttribGet(ih, "_IUPWINUI_IGNORE_VALUECHANGED"))
  {
    iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", NULL);
    return;
  }

  int newValue = (int)args.NewValue();
  int oldValue = (int)args.OldValue();

  IFni cb = (IFni)IupGetCallback(ih, "SPIN_CB");
  if (cb)
  {
    int ret = cb(ih, newValue);
    if (ret == IUP_IGNORE)
    {
      NumberBox numberBox = winuiGetHandle<NumberBox>(ih);
      if (numberBox)
      {
        ih->data->disable_callbacks = 1;
        numberBox.Value((double)oldValue);
        ih->data->disable_callbacks = 0;
      }
      return;
    }
  }

  winuiTextCallValueChanged(ih);
}

static int winuiTextSetValueAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux)
    return 0;

  iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", "1");
  ih->data->disable_callbacks = 1;

  if (aux->isSpin)
  {
    NumberBox nb = winuiGetHandle<NumberBox>(ih);
    if (nb)
    {
      int val = 0;
      if (value)
        iupStrToInt(value, &val);
      nb.Value((double)val);
    }
  }
  else if (aux->isPassword)
  {
    PasswordBox pb = winuiGetHandle<PasswordBox>(ih);
    if (pb)
      pb.Password(iupwinuiStringToHString(value ? value : ""));
  }
  else if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.Document().SetText(TextSetOptions::None, iupwinuiStringToHString(value ? value : ""));
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.Text(iupwinuiStringToHString(value ? value : ""));
    aux->savedText = iupwinuiStringToWString(value ? value : "");
  }

  ih->data->disable_callbacks = 0;

  return 0;
}

static char* winuiTextGetValueAttrib(Ihandle* ih)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux)
    return NULL;

  if (aux->isSpin)
  {
    NumberBox nb = winuiGetHandle<NumberBox>(ih);
    if (nb)
      return iupStrReturnInt((int)nb.Value());
  }
  else if (aux->isPassword)
  {
    PasswordBox pb = winuiGetHandle<PasswordBox>(ih);
    if (pb)
      return iupwinuiHStringToString(pb.Password());
  }
  else if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
    {
      hstring text;
      reb.Document().GetText(TextGetOptions::None, text);
      uint32_t len = text.size();
      if (len > 0 && text.c_str()[len - 1] == L'\r')
        return iupwinuiHStringToString(hstring(std::wstring_view(text.c_str(), len - 1)));
      return iupwinuiHStringToString(text);
    }
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
    {
      char* str = iupwinuiHStringToString(tb.Text());
      return str ? str : (char*)"";
    }
  }

  return NULL;
}

static int winuiTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin)
    return 0;

  if (!value)
    value = "";

  iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", "1");
  ih->data->disable_callbacks = 1;

  if (aux->isPassword)
  {
    PasswordBox pb = winuiGetHandle<PasswordBox>(ih);
    if (pb)
    {
      std::wstring newText(pb.Password().c_str());
      newText += iupwinuiStringToHString(value).c_str();
      pb.Password(hstring(newText));
    }
  }
  else if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
    {
      hstring currentText;
      reb.Document().GetText(TextGetOptions::None, currentText);

      std::wstring appendStr;
      if (ih->data->append_newline && currentText.size() > 0)
        appendStr = L"\r";
      appendStr += iupwinuiStringToHString(value).c_str();

      int32_t endPos = (int32_t)currentText.size();
      auto range = reb.Document().GetRange(endPos, endPos);
      range.SetText(TextSetOptions::None, hstring(appendStr));

      reb.UpdateLayout();
      ScrollViewer sv = winuiTextFindScrollViewer(reb);
      if (sv)
        sv.ChangeView(nullptr, sv.ScrollableHeight(), nullptr);
    }
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
    {
      std::wstring newText(tb.Text().c_str());

      if (aux->isMultiline && ih->data->append_newline && !newText.empty())
        newText += L"\r";

      newText += iupwinuiStringToHString(value).c_str();
      tb.Text(hstring(newText));

      if (aux->isMultiline)
      {
        tb.UpdateLayout();
        ScrollViewer sv = winuiTextFindScrollViewer(tb);
        if (sv)
          sv.ChangeView(nullptr, sv.ScrollableHeight(), nullptr);
      }
    }
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static int winuiTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isPassword)
    return 0;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.IsReadOnly(iupStrBoolean(value));
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.IsReadOnly(iupStrBoolean(value));
  }

  return 0;
}

static int winuiTextSetActiveAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux)
    return 0;

  bool enabled = iupStrBoolean(value);

  if (aux->isSpin)
  {
    NumberBox nb = winuiGetHandle<NumberBox>(ih);
    if (nb)
      nb.IsEnabled(enabled);
  }
  else if (aux->isPassword)
  {
    PasswordBox pb = winuiGetHandle<PasswordBox>(ih);
    if (pb)
      pb.IsEnabled(enabled);
  }
  else if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.IsEnabled(enabled);
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.IsEnabled(enabled);
  }

  return 0;
}

static int winuiTextMapMethod(Ihandle* ih)
{
  IupWinUITextAux* aux = new IupWinUITextAux();
  int isPassword = iupAttribGetBoolean(ih, "PASSWORD");
  int isMultiline = ih->data->is_multiline;
  int isSpin = !isMultiline && !isPassword && iupAttribGetBoolean(ih, "SPIN");

  aux->isPassword = isPassword ? true : false;
  aux->isMultiline = isMultiline ? true : false;
  aux->isSpin = isSpin ? true : false;

  const char* value = iupAttribGet(ih, "VALUE");

  if (aux->isSpin)
  {
    NumberBox nb;
    nb.HorizontalAlignment(HorizontalAlignment::Left);
    nb.VerticalAlignment(VerticalAlignment::Top);
    nb.SpinButtonPlacementMode(NumberBoxSpinButtonPlacementMode::Inline);

    int spinMin = iupAttribGetInt(ih, "SPINMIN");
    int spinMax = iupAttribGetInt(ih, "SPINMAX");
    int spinInc = iupAttribGetInt(ih, "SPININC");
    int spinValue = iupAttribGetInt(ih, "SPINVALUE");

    if (spinMax == 0 && spinMin == 0)
      spinMax = 100;
    if (spinInc == 0)
      spinInc = 1;

    nb.Minimum((double)spinMin);
    nb.Maximum((double)spinMax);
    nb.SmallChange((double)spinInc);
    nb.LargeChange((double)spinInc);

    if (value)
    {
      int val = 0;
      iupStrToInt(value, &val);
      nb.Value((double)val);
    }
    else
    {
      nb.Value((double)spinValue);
    }

    aux->valueChangedToken = nb.ValueChanged([ih](NumberBox const& sender, NumberBoxValueChangedEventArgs const& args) {
      winuiTextSpinValueChanged(ih, sender, args);
    });

    aux->gotFocusToken = nb.GotFocus([ih](IInspectable const&, RoutedEventArgs const&) {
      iupSetCurrentFocus(ih);
      iupCallGetFocusCb(ih);
    });

    aux->lostFocusToken = nb.LostFocus([ih](IInspectable const&, RoutedEventArgs const&) {
      iupCallKillFocusCb(ih);
    });

    aux->keyDownToken = nb.PreviewKeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    iupwinuiUpdateControlFont(ih, nb);

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(nb);

    nb.Loaded([](IInspectable const& sender, RoutedEventArgs const&) {
      TextBox tb = winuiFindTextBox(sender.as<DependencyObject>());
      if (tb)
      {
        tb.ApplyTemplate();
        winuiRemoveDeleteButton(tb.as<DependencyObject>());
      }
    });

    winuiStoreHandle(ih, nb);
  }
  else if (aux->isPassword)
  {
    PasswordBox pb = PasswordBox();
    pb.HorizontalAlignment(HorizontalAlignment::Left);
    pb.VerticalAlignment(VerticalAlignment::Top);

    if (value)
      pb.Password(iupwinuiStringToHString(value));

    aux->textChangedToken = pb.PasswordChanged([ih](IInspectable const&, RoutedEventArgs const&) {
      winuiTextCallValueChanged(ih);
    });

    aux->gotFocusToken = pb.GotFocus([ih](IInspectable const&, RoutedEventArgs const&) {
      iupSetCurrentFocus(ih);
      iupCallGetFocusCb(ih);
    });

    aux->lostFocusToken = pb.LostFocus([ih](IInspectable const&, RoutedEventArgs const&) {
      iupCallKillFocusCb(ih);
    });

    aux->keyDownToken = pb.PreviewKeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    iupwinuiUpdateControlFont(ih, pb);

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(pb);

    winuiStoreHandle(ih, pb);
  }
  else if (ih->data->has_formatting && isMultiline)
  {
    RichEditBox reb;
    reb.HorizontalAlignment(HorizontalAlignment::Left);
    reb.VerticalAlignment(VerticalAlignment::Top);

    if (iupAttribGetBoolean(ih, "WORDWRAP"))
    {
      reb.TextWrapping(TextWrapping::Wrap);
      ih->data->sb &= ~IUP_SB_HORIZ;
    }
    else
      reb.TextWrapping(TextWrapping::NoWrap);

    if (ih->data->sb & IUP_SB_HORIZ)
    {
      if (iupAttribGetBoolean(ih, "AUTOHIDE"))
        ScrollViewer::SetHorizontalScrollBarVisibility(reb, ScrollBarVisibility::Auto);
      else
        ScrollViewer::SetHorizontalScrollBarVisibility(reb, ScrollBarVisibility::Visible);
    }
    else
      ScrollViewer::SetHorizontalScrollBarVisibility(reb, ScrollBarVisibility::Disabled);

    if (ih->data->sb & IUP_SB_VERT)
    {
      if (iupAttribGetBoolean(ih, "AUTOHIDE"))
        ScrollViewer::SetVerticalScrollBarVisibility(reb, ScrollBarVisibility::Auto);
      else
        ScrollViewer::SetVerticalScrollBarVisibility(reb, ScrollBarVisibility::Visible);
    }
    else
      ScrollViewer::SetVerticalScrollBarVisibility(reb, ScrollBarVisibility::Disabled);

    aux->textChangedToken = reb.TextChanged([ih](IInspectable const&, RoutedEventArgs const&) {
      winuiTextCallValueChanged(ih);
    });

    aux->gotFocusToken = reb.GotFocus([ih](IInspectable const&, RoutedEventArgs const&) {
      iupSetCurrentFocus(ih);
      iupCallGetFocusCb(ih);
    });

    aux->lostFocusToken = reb.LostFocus([ih](IInspectable const&, RoutedEventArgs const&) {
      iupCallKillFocusCb(ih);
    });

    aux->keyDownToken = reb.PreviewKeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    iupwinuiUpdateControlFont(ih, reb);

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(reb);

    reb.ApplyTemplate();

    reb.Loaded([](IInspectable const& sender, RoutedEventArgs const&) {
      winuiTextSetScrollBarArrowCursors(sender.as<DependencyObject>());
    });

    winuiStoreHandle(ih, reb);
    aux->isFormatted = true;
  }
  else
  {
    TextBox tb = TextBox();
    tb.HorizontalAlignment(HorizontalAlignment::Left);
    tb.VerticalAlignment(VerticalAlignment::Top);

    if (aux->isMultiline)
    {
      tb.AcceptsReturn(true);

      if (iupAttribGetBoolean(ih, "WORDWRAP"))
      {
        tb.TextWrapping(TextWrapping::Wrap);
        ih->data->sb &= ~IUP_SB_HORIZ;
      }
      else
        tb.TextWrapping(TextWrapping::NoWrap);

      if (ih->data->sb & IUP_SB_HORIZ)
      {
        if (iupAttribGetBoolean(ih, "AUTOHIDE"))
          ScrollViewer::SetHorizontalScrollBarVisibility(tb, ScrollBarVisibility::Auto);
        else
          ScrollViewer::SetHorizontalScrollBarVisibility(tb, ScrollBarVisibility::Visible);
      }
      else
        ScrollViewer::SetHorizontalScrollBarVisibility(tb, ScrollBarVisibility::Disabled);

      if (ih->data->sb & IUP_SB_VERT)
      {
        if (iupAttribGetBoolean(ih, "AUTOHIDE"))
          ScrollViewer::SetVerticalScrollBarVisibility(tb, ScrollBarVisibility::Auto);
        else
          ScrollViewer::SetVerticalScrollBarVisibility(tb, ScrollBarVisibility::Visible);
      }
      else
        ScrollViewer::SetVerticalScrollBarVisibility(tb, ScrollBarVisibility::Disabled);

      tb.Loaded([](IInspectable const& sender, RoutedEventArgs const&) {
        winuiTextSetScrollBarArrowCursors(sender.as<DependencyObject>());
      });
    }

    if (!iupAttribGetBoolean(ih, "BORDER"))
    {
      tb.BorderThickness(Thickness{0, 0, 0, 0});
      tb.Padding(Thickness{0, 0, 0, 0});
    }

    int isReadOnly = iupAttribGetBoolean(ih, "READONLY");
    tb.IsReadOnly(isReadOnly ? true : false);

    if (value)
    {
      tb.Text(iupwinuiStringToHString(value));
      aux->savedText = iupwinuiStringToWString(value);
    }

    aux->beforeTextChangingToken = tb.BeforeTextChanging([ih](TextBox const& sender, TextBoxBeforeTextChangingEventArgs const& args) {
      winuiTextBeforeTextChanging(ih, sender, args);
    });

    aux->textChangedToken = tb.TextChanged([ih](IInspectable const&, TextChangedEventArgs const&) {
      winuiTextBoxTextChanged(ih);
    });

    aux->gotFocusToken = tb.GotFocus([ih](IInspectable const&, RoutedEventArgs const&) {
      iupSetCurrentFocus(ih);
      iupCallGetFocusCb(ih);
    });

    aux->lostFocusToken = tb.LostFocus([ih](IInspectable const&, RoutedEventArgs const&) {
      iupCallKillFocusCb(ih);
    });

    aux->keyDownToken = tb.PreviewKeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    iupwinuiUpdateControlFont(ih, tb);

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(tb);

    winuiStoreHandle(ih, tb);
  }

  winuiSetAux(ih, IUPWINUI_TEXT_AUX, aux);

  iupTextUpdateFormatTags(ih);

  return IUP_NOERROR;
}

static void winuiTextUnMapMethod(Ihandle* ih)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);

  if (ih->handle && aux)
  {
    if (aux->isSpin)
    {
      NumberBox nb = winuiGetHandle<NumberBox>(ih);
      if (nb)
      {
        if (aux->valueChangedToken)
          nb.ValueChanged(aux->valueChangedToken);
        if (aux->gotFocusToken)
          nb.GotFocus(aux->gotFocusToken);
        if (aux->lostFocusToken)
          nb.LostFocus(aux->lostFocusToken);
        if (aux->keyDownToken)
          nb.PreviewKeyDown(aux->keyDownToken);
      }
    }
    else if (aux->isPassword)
    {
      PasswordBox pb = winuiGetHandle<PasswordBox>(ih);
      if (pb)
      {
        if (aux->textChangedToken)
          pb.PasswordChanged(aux->textChangedToken);
        if (aux->gotFocusToken)
          pb.GotFocus(aux->gotFocusToken);
        if (aux->lostFocusToken)
          pb.LostFocus(aux->lostFocusToken);
        if (aux->keyDownToken)
          pb.PreviewKeyDown(aux->keyDownToken);
      }
    }
    else if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
      {
        if (aux->textChangedToken)
          reb.TextChanged(aux->textChangedToken);
        if (aux->gotFocusToken)
          reb.GotFocus(aux->gotFocusToken);
        if (aux->lostFocusToken)
          reb.LostFocus(aux->lostFocusToken);
        if (aux->keyDownToken)
          reb.PreviewKeyDown(aux->keyDownToken);
      }
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
      {
        if (aux->beforeTextChangingToken)
          tb.BeforeTextChanging(aux->beforeTextChangingToken);
        if (aux->textChangedToken)
          tb.TextChanged(aux->textChangedToken);
        if (aux->gotFocusToken)
          tb.GotFocus(aux->gotFocusToken);
        if (aux->lostFocusToken)
          tb.LostFocus(aux->lostFocusToken);
        if (aux->keyDownToken)
          tb.PreviewKeyDown(aux->keyDownToken);
      }
    }
  }

  bool isSpin = aux ? aux->isSpin : false;
  bool isPassword = aux ? aux->isPassword : false;
  bool isFormatted = aux ? aux->isFormatted : false;
  winuiFreeAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);

  if (isSpin)
    winuiReleaseHandle<NumberBox>(ih);
  else if (isPassword)
    winuiReleaseHandle<PasswordBox>(ih);
  else if (isFormatted)
    winuiReleaseHandle<RichEditBox>(ih);
  else
    winuiReleaseHandle<TextBox>(ih);
}

static int winuiTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || !aux->isSpin)
    return 1;

  int val = 0;
  iupStrToInt(value, &val);

  NumberBox nb = winuiGetHandle<NumberBox>(ih);
  if (nb)
    nb.Minimum((double)val);

  return 1;
}

static int winuiTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || !aux->isSpin)
    return 1;

  int val = 100;
  iupStrToInt(value, &val);

  NumberBox nb = winuiGetHandle<NumberBox>(ih);
  if (nb)
    nb.Maximum((double)val);

  return 1;
}

static int winuiTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || !aux->isSpin)
    return 1;

  int val = 1;
  iupStrToInt(value, &val);

  NumberBox nb = winuiGetHandle<NumberBox>(ih);
  if (nb)
  {
    nb.SmallChange((double)val);
    nb.LargeChange((double)val);
  }

  return 1;
}

static int winuiTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || !aux->isSpin)
    return 1;

  int val = 0;
  iupStrToInt(value, &val);

  NumberBox nb = winuiGetHandle<NumberBox>(ih);
  if (nb)
  {
    iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", "1");
    ih->data->disable_callbacks = 1;
    nb.Value((double)val);
    ih->data->disable_callbacks = 0;
  }

  return 1;
}

static char* winuiTextGetSpinValueAttrib(Ihandle* ih)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || !aux->isSpin)
    return NULL;

  NumberBox nb = winuiGetHandle<NumberBox>(ih);
  if (nb)
    return iupStrReturnInt((int)nb.Value());

  return NULL;
}

static float winui_multiline_border_height = -1;

static void winuiTextMeasureMultilineMetrics(void)
{
  if (winui_multiline_border_height >= 0)
    return;

  float border_v = 0;
  auto resources = Application::Current().Resources();

  auto borderKey = box_value(L"TextControlBorderThemeThickness");
  if (resources.HasKey(borderKey))
  {
    auto bt = unbox_value<Thickness>(resources.Lookup(borderKey));
    border_v += (float)(bt.Top + bt.Bottom);
  }

  auto paddingKey = box_value(L"TextControlThemePadding");
  if (resources.HasKey(paddingKey))
  {
    auto pd = unbox_value<Thickness>(resources.Lookup(paddingKey));
    border_v += (float)(pd.Top + pd.Bottom);
  }

  winui_multiline_border_height = border_v;
  if (winui_multiline_border_height < 2) winui_multiline_border_height = 6;
}

extern "C" void iupdrvTextAddBorders(Ihandle* ih, int* x, int* y)
{
  if (ih->data && ih->data->is_multiline)
  {
    int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");

    winuiTextMeasureMultilineMetrics();

    *x += 6;

    if (visiblelines > 0)
    {
      int char_height;
      iupdrvFontGetCharSize(ih, NULL, &char_height);

      float line_height_f = iupwinuiFontGetMultilineLineHeightF(ih);

      *y -= char_height * visiblelines;
      *y += (int)ceil(winui_multiline_border_height + line_height_f * visiblelines);
    }
    else
    {
      *y += (int)ceil(winui_multiline_border_height);
    }
  }
  else
  {
    *x += 6;
    *y += 6;
  }
}

extern "C" void iupdrvTextAddSpin(Ihandle* ih, int* w, int h)
{
  (void)ih;
  (void)h;
  *w += 64;  /* Two spin buttons at 32px each (from NumberBoxSpinButtonStyle MinWidth) */
}

static int winuiTextLinColToPos(ITextDocument const& doc, int lin, int col);

static int winuiTextStringLinColToPos(const wchar_t* text, int textLen, int lin, int col)
{
  int currentLine = 1;
  int pos = 0;

  while (currentLine < lin && pos < textLen)
  {
    if (text[pos] == L'\r')
      currentLine++;
    pos++;
  }

  pos += (col - 1);

  if (pos > textLen)
    pos = textLen;
  if (pos < 0)
    pos = 0;

  return pos;
}

static void winuiTextStringPosToLinCol(const wchar_t* text, int textLen, int pos, int* lin, int* col)
{
  if (pos > textLen)
    pos = textLen;
  if (pos < 0)
    pos = 0;

  int currentLine = 1;
  int lineStart = 0;

  for (int i = 0; i < pos; i++)
  {
    if (text[i] == L'\r')
    {
      currentLine++;
      lineStart = i + 1;
    }
  }

  *lin = currentLine;
  *col = pos - lineStart + 1;
}

extern "C" void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int* pos)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux)
  {
    *pos = 0;
    return;
  }

  if (!ih->data->is_multiline)
  {
    *pos = col - 1;
    if (*pos < 0) *pos = 0;
    return;
  }

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      *pos = winuiTextLinColToPos(reb.Document(), lin, col);
    else
      *pos = 0;
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
    {
      hstring text = tb.Text();
      *pos = winuiTextStringLinColToPos(text.c_str(), (int)text.size(), lin, col);
    }
    else
      *pos = 0;
  }
}

extern "C" void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int* lin, int* col)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux)
  {
    *lin = 0;
    *col = 0;
    return;
  }

  if (!ih->data->is_multiline)
  {
    *lin = 1;
    *col = pos + 1;
    return;
  }

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
    {
      hstring text;
      reb.Document().GetText(TextGetOptions::None, text);
      winuiTextStringPosToLinCol(text.c_str(), (int)text.size(), pos, lin, col);
    }
    else
    {
      *lin = 0;
      *col = 0;
    }
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
    {
      hstring text = tb.Text();
      winuiTextStringPosToLinCol(text.c_str(), (int)text.size(), pos, lin, col);
    }
    else
    {
      *lin = 0;
      *col = 0;
    }
  }
}

extern "C" void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  (void)ih;
  return NULL;
}

extern "C" void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  (void)ih;
  (void)state;
}

static int winuiTextLinColToPos(ITextDocument const& doc, int lin, int col)
{
  hstring text;
  doc.GetText(TextGetOptions::None, text);
  std::wstring_view wtext(text.c_str(), text.size());

  int currentLine = 1;
  int pos = 0;

  while (currentLine < lin && pos < (int)wtext.size())
  {
    if (wtext[pos] == L'\r')
      currentLine++;
    pos++;
  }

  pos += (col - 1);

  if (pos > (int)wtext.size())
    pos = (int)wtext.size();
  if (pos < 0)
    pos = 0;

  return pos;
}

static void winuiTextParseCharacterFormat(Ihandle* formattag, ITextRange const& range)
{
  auto cf = range.CharacterFormat();
  bool changed = false;
  char* val;

  val = iupAttribGet(formattag, "BGCOLOR");
  if (val)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(val, &r, &g, &b))
    {
      Windows::UI::Color color;
      color.A = 255; color.R = r; color.G = g; color.B = b;
      cf.BackgroundColor(color);
      changed = true;
    }
  }

  val = iupAttribGet(formattag, "FGCOLOR");
  if (val)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(val, &r, &g, &b))
    {
      Windows::UI::Color color;
      color.A = 255; color.R = r; color.G = g; color.B = b;
      cf.ForegroundColor(color);
      changed = true;
    }
  }

  val = iupAttribGet(formattag, "ITALIC");
  if (val)
  {
    cf.Italic(iupStrBoolean(val) ? FormatEffect::On : FormatEffect::Off);
    changed = true;
  }

  val = iupAttribGet(formattag, "STRIKEOUT");
  if (val)
  {
    cf.Strikethrough(iupStrBoolean(val) ? FormatEffect::On : FormatEffect::Off);
    changed = true;
  }

  val = iupAttribGet(formattag, "UNDERLINE");
  if (val)
  {
    if (iupStrEqualNoCase(val, "SINGLE"))
      cf.Underline(UnderlineType::Single);
    else if (iupStrEqualNoCase(val, "DOUBLE"))
      cf.Underline(UnderlineType::Double);
    else if (iupStrEqualNoCase(val, "DOTTED"))
      cf.Underline(UnderlineType::Dotted);
    else
      cf.Underline(UnderlineType::None);
    changed = true;
  }

  val = iupAttribGet(formattag, "WEIGHT");
  if (val)
  {
    int weight = 400;
    if (iupStrEqualNoCase(val, "EXTRALIGHT")) weight = 200;
    else if (iupStrEqualNoCase(val, "LIGHT")) weight = 300;
    else if (iupStrEqualNoCase(val, "NORMAL")) weight = 400;
    else if (iupStrEqualNoCase(val, "SEMIBOLD")) weight = 600;
    else if (iupStrEqualNoCase(val, "BOLD")) weight = 700;
    else if (iupStrEqualNoCase(val, "EXTRABOLD")) weight = 800;
    else if (iupStrEqualNoCase(val, "HEAVY")) weight = 900;
    cf.Weight(weight);
    changed = true;
  }

  val = iupAttribGet(formattag, "FONTFACE");
  if (val)
  {
    cf.Name(iupwinuiStringToHString(val));
    changed = true;
  }

  val = iupAttribGet(formattag, "FONTSIZE");
  if (val)
  {
    int size = 0;
    if (iupStrToInt(val, &size))
    {
      float fontSize;
      if (size > 0)
        fontSize = (float)size;
      else
        fontSize = (float)(-size) * 72.0f / (float)iupdrvGetScreenDpi();

      char* fontscale = iupAttribGet(formattag, "FONTSCALE");
      if (fontscale)
      {
        double scale = 0;
        if (iupStrEqualNoCase(fontscale, "XX-SMALL")) scale = 0.5787;
        else if (iupStrEqualNoCase(fontscale, "X-SMALL")) scale = 0.6944;
        else if (iupStrEqualNoCase(fontscale, "SMALL")) scale = 0.8333;
        else if (iupStrEqualNoCase(fontscale, "MEDIUM")) scale = 1.0;
        else if (iupStrEqualNoCase(fontscale, "LARGE")) scale = 1.2;
        else if (iupStrEqualNoCase(fontscale, "X-LARGE")) scale = 1.44;
        else if (iupStrEqualNoCase(fontscale, "XX-LARGE")) scale = 1.728;
        else iupStrToDouble(fontscale, &scale);

        if (scale > 0)
          fontSize = (float)(fontSize * scale);
      }

      cf.Size(fontSize);
      changed = true;
    }
  }

  val = iupAttribGet(formattag, "RISE");
  if (val)
  {
    if (iupStrEqualNoCase(val, "SUPERSCRIPT"))
    {
      cf.Superscript(FormatEffect::On);
      cf.Subscript(FormatEffect::Off);
    }
    else if (iupStrEqualNoCase(val, "SUBSCRIPT"))
    {
      cf.Subscript(FormatEffect::On);
      cf.Superscript(FormatEffect::Off);
    }
    else
    {
      int rise = 0;
      iupStrToInt(val, &rise);
      cf.Position((float)rise);
      cf.Superscript(FormatEffect::Off);
      cf.Subscript(FormatEffect::Off);
    }
    changed = true;
  }

  val = iupAttribGet(formattag, "PROTECTED");
  if (val)
  {
    cf.ProtectedText(iupStrBoolean(val) ? FormatEffect::On : FormatEffect::Off);
    changed = true;
  }

  val = iupAttribGet(formattag, "SMALLCAPS");
  if (val)
  {
    cf.SmallCaps(iupStrBoolean(val) ? FormatEffect::On : FormatEffect::Off);
    changed = true;
  }

  if (changed)
    range.CharacterFormat(cf);
}

static void winuiTextParseParagraphFormat(Ihandle* formattag, ITextRange const& range)
{
  auto pf = range.ParagraphFormat();
  bool changed = false;
  char* val;
  float pixelToPoint = 72.0f / (float)iupdrvGetScreenDpi();

  val = iupAttribGet(formattag, "UNITS");
  if (val && iupStrEqualNoCase(val, "TWIPS"))
    pixelToPoint = 1.0f / 20.0f;

  val = iupAttribGet(formattag, "ALIGNMENT");
  if (val)
  {
    if (iupStrEqualNoCase(val, "JUSTIFY"))
      pf.Alignment(ParagraphAlignment::Justify);
    else if (iupStrEqualNoCase(val, "RIGHT"))
      pf.Alignment(ParagraphAlignment::Right);
    else if (iupStrEqualNoCase(val, "CENTER"))
      pf.Alignment(ParagraphAlignment::Center);
    else
      pf.Alignment(ParagraphAlignment::Left);
    changed = true;
  }

  val = iupAttribGet(formattag, "INDENT");
  if (val)
  {
    int ival = 0;
    if (iupStrToInt(val, &ival))
    {
      float leftIndent = (float)ival * pixelToPoint;

      char* indentRight = iupAttribGet(formattag, "INDENTRIGHT");
      int irval = ival;
      if (indentRight)
        iupStrToInt(indentRight, &irval);
      float rightIndent = (float)irval * pixelToPoint;

      float firstLineIndent = 0;
      char* indentOffset = iupAttribGet(formattag, "INDENTOFFSET");
      if (indentOffset)
      {
        int ioval = 0;
        iupStrToInt(indentOffset, &ioval);
        firstLineIndent = (float)ioval * pixelToPoint;
      }

      pf.SetIndents(firstLineIndent, leftIndent, rightIndent);
      changed = true;
    }
  }

  val = iupAttribGet(formattag, "SPACEBEFORE");
  if (val)
  {
    int ival = 0;
    if (iupStrToInt(val, &ival))
    {
      pf.SpaceBefore((float)ival * pixelToPoint);
      changed = true;
    }
  }

  val = iupAttribGet(formattag, "SPACEAFTER");
  if (val)
  {
    int ival = 0;
    if (iupStrToInt(val, &ival))
    {
      pf.SpaceAfter((float)ival * pixelToPoint);
      changed = true;
    }
  }

  val = iupAttribGet(formattag, "LINESPACING");
  if (val)
  {
    if (iupStrEqualNoCase(val, "SINGLE"))
      pf.SetLineSpacing(LineSpacingRule::Multiple, 1.0f);
    else if (iupStrEqualNoCase(val, "ONEHALF"))
      pf.SetLineSpacing(LineSpacingRule::Multiple, 1.5f);
    else if (iupStrEqualNoCase(val, "DOUBLE"))
      pf.SetLineSpacing(LineSpacingRule::Multiple, 2.0f);
    else
    {
      int ival = 0;
      if (iupStrToInt(val, &ival))
        pf.SetLineSpacing(LineSpacingRule::AtLeast, (float)ival * pixelToPoint);
    }
    changed = true;
  }

  val = iupAttribGet(formattag, "NUMBERING");
  if (val)
  {
    if (iupStrEqualNoCase(val, "BULLET"))
      pf.ListType(MarkerType::Bullet);
    else if (iupStrEqualNoCase(val, "ARABIC"))
      pf.ListType(MarkerType::Arabic);
    else if (iupStrEqualNoCase(val, "LCLETTER"))
      pf.ListType(MarkerType::LowercaseEnglishLetter);
    else if (iupStrEqualNoCase(val, "UCLETTER"))
      pf.ListType(MarkerType::UppercaseEnglishLetter);
    else if (iupStrEqualNoCase(val, "LCROMAN"))
      pf.ListType(MarkerType::LowercaseRoman);
    else if (iupStrEqualNoCase(val, "UCROMAN"))
      pf.ListType(MarkerType::UppercaseRoman);
    else
      pf.ListType(MarkerType::None);

    char* style = iupAttribGet(formattag, "NUMBERINGSTYLE");
    if (style)
    {
      if (iupStrEqualNoCase(style, "RIGHTPARENTHESIS"))
        pf.ListStyle(MarkerStyle::Parenthesis);
      else if (iupStrEqualNoCase(style, "PARENTHESES"))
        pf.ListStyle(MarkerStyle::Parentheses);
      else if (iupStrEqualNoCase(style, "PERIOD"))
        pf.ListStyle(MarkerStyle::Period);
      else if (iupStrEqualNoCase(style, "NONUMBER"))
        pf.ListStyle(MarkerStyle::NoNumber);
      else
        pf.ListStyle(MarkerStyle::Plain);
    }

    char* numberingtab = iupAttribGet(formattag, "NUMBERINGTAB");
    if (numberingtab)
    {
      int tabval = 0;
      if (iupStrToInt(numberingtab, &tabval))
        pf.ListTab((float)tabval * pixelToPoint);
    }

    changed = true;
  }

  if (changed)
    range.ParagraphFormat(pf);
}

extern "C" void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || !aux->isFormatted)
    return;

  RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
  if (!reb)
    return;

  (void)bulk;

  auto doc = reb.Document();

  ITextRange range{nullptr};

  char* selection = iupAttribGet(formattag, "SELECTION");
  if (selection)
  {
    if (iupStrEqualNoCase(selection, "ALL"))
    {
      hstring text;
      doc.GetText(TextGetOptions::None, text);
      range = doc.GetRange(0, (int32_t)text.size());
    }
    else if (!iupStrEqualNoCase(selection, "NONE"))
    {
      int lin_start = 1, col_start = 1, lin_end = 1, col_end = 1;
      if (sscanf(selection, "%d,%d:%d,%d", &lin_start, &col_start, &lin_end, &col_end) == 4)
      {
        int start_pos = winuiTextLinColToPos(doc, lin_start, col_start);
        int end_pos = winuiTextLinColToPos(doc, lin_end, col_end);
        range = doc.GetRange(start_pos, end_pos);
      }
    }
  }
  else
  {
    char* selectionpos = iupAttribGet(formattag, "SELECTIONPOS");
    if (selectionpos)
    {
      if (iupStrEqualNoCase(selectionpos, "ALL"))
      {
        hstring text;
        doc.GetText(TextGetOptions::None, text);
        range = doc.GetRange(0, (int32_t)text.size());
      }
      else if (!iupStrEqualNoCase(selectionpos, "NONE"))
      {
        int start = 0, end = 0;
        iupStrToIntInt(selectionpos, &start, &end, ':');
        range = doc.GetRange(start, end);
      }
    }
    else
    {
      range = doc.Selection().GetClone();
    }
  }

  if (!range)
    return;

  winuiTextParseCharacterFormat(formattag, range);
  winuiTextParseParagraphFormat(formattag, range);
}

static int winui_singleline_extra_h = -1;

static void winuiTextMeasureSingleLineMetrics(void)
{
  if (winui_singleline_extra_h >= 0)
    return;

  TextBox tb;
  tb.BorderThickness(Thickness{0, 0, 0, 0});
  tb.Padding(Thickness{0, 0, 0, 0});
  tb.Text(L"Wj");
  tb.Measure(Size(10000, 10000));
  int textbox_height = (int)ceil(tb.DesiredSize().Height);

  TextBlock tblock;
  tblock.Text(L"Wj");
  tblock.Measure(Size(10000, 10000));
  int text_height = (int)ceil(tblock.DesiredSize().Height);

  winui_singleline_extra_h = textbox_height - text_height;
  if (winui_singleline_extra_h < 0) winui_singleline_extra_h = 0;
}

extern "C" void iupdrvTextAddExtraPadding(Ihandle* ih, int* x, int* y)
{
  (void)ih;
  winuiTextMeasureSingleLineMetrics();
  *x += winui_singleline_extra_h;
  *y += winui_singleline_extra_h;
}

static char* winuiTextGetSelectedTextAttrib(Ihandle* ih)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return NULL;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
    {
      auto sel = reb.Document().Selection();
      if (sel.StartPosition() != sel.EndPosition())
      {
        hstring text;
        sel.GetText(TextGetOptions::None, text);
        return iupwinuiHStringToString(text);
      }
    }
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
    {
      int start = tb.SelectionStart();
      int len = tb.SelectionLength();
      if (len > 0)
      {
        hstring fullText = tb.Text();
        std::wstring selected(fullText.c_str() + start, len);
        return iupwinuiHStringToString(hstring(selected));
      }
    }
  }

  return NULL;
}

static int winuiTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;

  if (!value)
    value = "";

  ih->data->disable_callbacks = 1;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
    {
      auto sel = reb.Document().Selection();
      if (sel.StartPosition() != sel.EndPosition())
        sel.SetText(TextSetOptions::None, iupwinuiStringToHString(value));
    }
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
    {
      int start = tb.SelectionStart();
      int len = tb.SelectionLength();
      if (len > 0)
      {
        std::wstring fullText(tb.Text().c_str());
        std::wstring replacement = iupwinuiStringToWString(value);
        fullText.replace(start, len, replacement);
        iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", "1");
        tb.Text(hstring(fullText));
        tb.Select(start + (int)replacement.size(), 0);
        aux->savedText = fullText;
      }
    }
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static char* winuiTextGetSelectionAttrib(Ihandle* ih)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return NULL;

  int start, end;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (!reb) return NULL;
    auto sel = reb.Document().Selection();
    start = sel.StartPosition();
    end = sel.EndPosition();
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (!tb) return NULL;
    start = tb.SelectionStart();
    end = start + tb.SelectionLength();
  }

  if (start == end)
    return NULL;

  if (ih->data->is_multiline)
  {
    int lin1, col1, lin2, col2;
    iupdrvTextConvertPosToLinCol(ih, start, &lin1, &col1);
    iupdrvTextConvertPosToLinCol(ih, end, &lin2, &col2);
    return iupStrReturnStrf("%d,%d:%d,%d", lin1, col1, lin2, col2);
  }
  else
    return iupStrReturnIntInt(start + 1, end + 1, ':');
}

static int winuiTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;
  if (!value)
    return 0;

  if (iupStrEqualNoCase(value, "NONE"))
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
      {
        auto sel = reb.Document().Selection();
        sel.SetRange(sel.StartPosition(), sel.StartPosition());
      }
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
        tb.Select(tb.SelectionStart(), 0);
    }
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
      {
        hstring text;
        reb.Document().GetText(TextGetOptions::None, text);
        reb.Document().Selection().SetRange(0, (int32_t)text.size());
      }
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
        tb.SelectAll();
    }
    return 0;
  }

  if (ih->data->is_multiline)
  {
    int lin1 = 1, col1 = 1, lin2 = 1, col2 = 1;
    if (sscanf(value, "%d,%d:%d,%d", &lin1, &col1, &lin2, &col2) != 4)
      return 0;

    int start, end;
    iupdrvTextConvertLinColToPos(ih, lin1, col1, &start);
    iupdrvTextConvertLinColToPos(ih, lin2, col2, &end);

    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
        reb.Document().Selection().SetRange(start, end);
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
        tb.Select(start, end - start);
    }
  }
  else
  {
    int start = 1, end = 1;
    if (iupStrToIntInt(value, &start, &end, ':') != 2)
      return 0;
    start--;
    end--;
    if (start < 0) start = 0;
    if (end < 0) end = 0;

    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.Select(start, end - start);
  }

  return 0;
}

static char* winuiTextGetSelectionPosAttrib(Ihandle* ih)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return NULL;

  int start, end;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (!reb) return NULL;
    auto sel = reb.Document().Selection();
    start = sel.StartPosition();
    end = sel.EndPosition();
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (!tb) return NULL;
    start = tb.SelectionStart();
    end = start + tb.SelectionLength();
  }

  if (start == end)
    return NULL;

  return iupStrReturnIntInt(start, end, ':');
}

static int winuiTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;
  if (!value)
    return 0;

  if (iupStrEqualNoCase(value, "NONE"))
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
      {
        auto sel = reb.Document().Selection();
        sel.SetRange(sel.StartPosition(), sel.StartPosition());
      }
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
        tb.Select(tb.SelectionStart(), 0);
    }
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
      {
        hstring text;
        reb.Document().GetText(TextGetOptions::None, text);
        reb.Document().Selection().SetRange(0, (int32_t)text.size());
      }
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
        tb.SelectAll();
    }
    return 0;
  }

  int start = 0, end = 0;
  if (iupStrToIntInt(value, &start, &end, ':') != 2)
    return 0;
  if (start < 0) start = 0;
  if (end < 0) end = 0;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.Document().Selection().SetRange(start, end);
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.Select(start, end - start);
  }

  return 0;
}

static char* winuiTextGetCaretAttrib(Ihandle* ih)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return NULL;

  int pos;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (!reb) return NULL;
    pos = reb.Document().Selection().StartPosition();
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (!tb) return NULL;
    pos = tb.SelectionStart();
  }

  if (ih->data->is_multiline)
  {
    int lin, col;
    iupdrvTextConvertPosToLinCol(ih, pos, &lin, &col);
    return iupStrReturnIntInt(lin, col, ',');
  }
  else
    return iupStrReturnIntInt(1, pos + 1, ',');
}

static int winuiTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;
  if (!value)
    return 0;

  int lin = 1, col = 1;
  iupStrToIntInt(value, &lin, &col, ',');
  if (lin < 1) lin = 1;
  if (col < 1) col = 1;

  int pos;
  if (ih->data->is_multiline)
    iupdrvTextConvertLinColToPos(ih, lin, col, &pos);
  else
    pos = col - 1;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.Document().Selection().SetRange(pos, pos);
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.Select(pos, 0);
  }

  return 0;
}

static char* winuiTextGetCaretPosAttrib(Ihandle* ih)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return NULL;

  int pos;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (!reb) return NULL;
    pos = reb.Document().Selection().StartPosition();
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (!tb) return NULL;
    pos = tb.SelectionStart();
  }

  return iupStrReturnInt(pos);
}

static int winuiTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;
  if (!value)
    return 0;

  int pos = 0;
  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.Document().Selection().SetRange(pos, pos);
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.Select(pos, 0);
  }

  return 0;
}

static int winuiTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value)
    return 0;

  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;

  ih->data->disable_callbacks = 1;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.Document().Selection().SetText(TextSetOptions::None, iupwinuiStringToHString(value));
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
    {
      int pos = tb.SelectionStart();
      int len = tb.SelectionLength();
      std::wstring fullText(tb.Text().c_str());
      std::wstring insertText = iupwinuiStringToWString(value);
      fullText.replace(pos, len, insertText);
      iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", "1");
      tb.Text(hstring(fullText));
      tb.Select(pos + (int)insertText.size(), 0);
      aux->savedText = fullText;
    }
  }

  ih->data->disable_callbacks = 0;
  return 0;
}

static char* winuiTextGetCountAttrib(Ihandle* ih)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return NULL;

  int count;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (!reb) return NULL;
    hstring text;
    reb.Document().GetText(TextGetOptions::None, text);
    count = (int)text.size();
    if (count > 0 && text.c_str()[count - 1] == L'\r')
      count--;
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (!tb) return NULL;
    count = (int)tb.Text().size();
  }

  return iupStrReturnInt(count);
}

static char* winuiTextGetLineCountAttrib(Ihandle* ih)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux)
    return NULL;

  if (!ih->data->is_multiline)
    return iupStrReturnInt(1);

  int lineCount = 1;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (!reb) return NULL;
    hstring text;
    reb.Document().GetText(TextGetOptions::None, text);
    int len = (int)text.size();
    if (len > 0 && text.c_str()[len - 1] == L'\r')
      len--;
    for (int i = 0; i < len; i++)
    {
      if (text.c_str()[i] == L'\r')
        lineCount++;
    }
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (!tb) return NULL;
    hstring text = tb.Text();
    for (uint32_t i = 0; i < text.size(); i++)
    {
      if (text.c_str()[i] == L'\r')
        lineCount++;
    }
  }

  return iupStrReturnInt(lineCount);
}

static int winuiTextSetNCAttrib(Ihandle* ih, const char* value)
{
  int nc = 0;
  if (value)
    iupStrToInt(value, &nc);
  ih->data->nc = nc;

  if (!ih->handle)
    return 1;

  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isFormatted)
    return 0;

  if (!ih->data->is_multiline)
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.MaxLength(nc);
  }

  return 0;
}

static void winuiTextCopyToClipboard(const wchar_t* text)
{
  if (!OpenClipboard(NULL))
    return;
  EmptyClipboard();
  size_t len = wcslen(text) + 1;
  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));
  if (hMem)
  {
    wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
    if (pMem)
    {
      memcpy(pMem, text, len * sizeof(wchar_t));
      GlobalUnlock(hMem);
      SetClipboardData(CF_UNICODETEXT, hMem);
    }
  }
  CloseClipboard();
}

static wchar_t* winuiTextGetFromClipboard(void)
{
  if (!OpenClipboard(NULL))
    return NULL;
  HANDLE hData = GetClipboardData(CF_UNICODETEXT);
  wchar_t* result = NULL;
  if (hData)
  {
    wchar_t* pszText = (wchar_t*)GlobalLock(hData);
    if (pszText)
    {
      size_t len = wcslen(pszText) + 1;
      result = (wchar_t*)malloc(len * sizeof(wchar_t));
      if (result)
        memcpy(result, pszText, len * sizeof(wchar_t));
      GlobalUnlock(hData);
    }
  }
  CloseClipboard();
  return result;
}

static int winuiTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;

  if (iupStrEqualNoCase(value, "COPY"))
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
        reb.Document().Selection().Copy();
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
      {
        int start = tb.SelectionStart();
        int len = tb.SelectionLength();
        if (len > 0)
        {
          hstring text = tb.Text();
          std::wstring selected(text.c_str() + start, len);
          winuiTextCopyToClipboard(selected.c_str());
        }
      }
    }
  }
  else if (iupStrEqualNoCase(value, "CUT"))
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
        reb.Document().Selection().Cut();
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
      {
        int start = tb.SelectionStart();
        int len = tb.SelectionLength();
        if (len > 0)
        {
          hstring text = tb.Text();
          std::wstring selected(text.c_str() + start, len);
          winuiTextCopyToClipboard(selected.c_str());
          ih->data->disable_callbacks = 1;
          iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", "1");
          std::wstring fullText(text.c_str());
          fullText.erase(start, len);
          tb.Text(hstring(fullText));
          tb.Select(start, 0);
          aux->savedText = fullText;
          ih->data->disable_callbacks = 0;
        }
      }
    }
  }
  else if (iupStrEqualNoCase(value, "PASTE"))
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
        reb.Document().Selection().Paste(0);
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
      {
        wchar_t* pasteText = winuiTextGetFromClipboard();
        if (pasteText)
        {
          int start = tb.SelectionStart();
          int len = tb.SelectionLength();
          std::wstring fullText(tb.Text().c_str());
          std::wstring paste(pasteText);
          ih->data->disable_callbacks = 1;
          iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", "1");
          fullText.replace(start, len, paste);
          tb.Text(hstring(fullText));
          tb.Select(start + (int)paste.size(), 0);
          aux->savedText = fullText;
          ih->data->disable_callbacks = 0;
          free(pasteText);
        }
      }
    }
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
        reb.Document().Selection().SetText(TextSetOptions::None, L"");
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
      {
        int start = tb.SelectionStart();
        int len = tb.SelectionLength();
        if (len > 0)
        {
          ih->data->disable_callbacks = 1;
          iupAttribSet(ih, "_IUPWINUI_IGNORE_VALUECHANGED", "1");
          std::wstring fullText(tb.Text().c_str());
          fullText.erase(start, len);
          tb.Text(hstring(fullText));
          tb.Select(start, 0);
          aux->savedText = fullText;
          ih->data->disable_callbacks = 0;
        }
      }
    }
  }
  else if (iupStrEqualNoCase(value, "UNDO"))
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
        reb.Document().Undo();
    }
  }
  else if (iupStrEqualNoCase(value, "REDO"))
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
        reb.Document().Redo();
    }
  }

  return 0;
}

static int winuiTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;
  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    iupStrToIntInt(value, &lin, &col, ',');
    if (lin < 1) lin = 1;
    if (col < 1) col = 1;

    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
      {
        int pos = winuiTextLinColToPos(reb.Document(), lin, col);
        auto range = reb.Document().GetRange(pos, pos);
        range.ScrollIntoView(PointOptions::Start);
      }
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
      {
        ScrollViewer sv = winuiTextFindScrollViewer(tb);
        if (sv)
        {
          float lineHeight = iupwinuiFontGetMultilineLineHeightF(ih);
          double yOffset = (lin - 1) * lineHeight;
          sv.ChangeView(nullptr, yOffset, nullptr);
        }
      }
    }
  }
  else
  {
    int pos = 1;
    iupStrToInt(value, &pos);
    if (pos < 1) pos = 1;
    pos--;

    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.Select(pos, 0);
  }

  return 0;
}

static int winuiTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;
  if (!value)
    return 0;

  int pos = 0;
  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  if (ih->data->is_multiline)
  {
    if (aux->isFormatted)
    {
      RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
      if (reb)
      {
        auto range = reb.Document().GetRange(pos, pos);
        range.ScrollIntoView(PointOptions::Start);
      }
    }
    else
    {
      TextBox tb = winuiGetHandle<TextBox>(ih);
      if (tb)
      {
        int lin, col;
        hstring text = tb.Text();
        winuiTextStringPosToLinCol(text.c_str(), (int)text.size(), pos, &lin, &col);
        ScrollViewer sv = winuiTextFindScrollViewer(tb);
        if (sv)
        {
          float lineHeight = iupwinuiFontGetMultilineLineHeightF(ih);
          double yOffset = (lin - 1) * lineHeight;
          sv.ChangeView(nullptr, yOffset, nullptr);
        }
      }
    }
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.Select(pos, 0);
  }

  return 0;
}

static int winuiTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;

  TextAlignment align = TextAlignment::Left;
  if (iupStrEqualNoCase(value, "ARIGHT"))
    align = TextAlignment::Right;
  else if (iupStrEqualNoCase(value, "ACENTER"))
    align = TextAlignment::Center;

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.TextAlignment(align);
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.TextAlignment(align);
  }

  return 1;
}

static int winuiTextSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux)
    return 0;

  Windows::UI::Color color;
  color.A = 255; color.R = r; color.G = g; color.B = b;
  SolidColorBrush brush(color);

  if (aux->isSpin)
  {
    NumberBox nb = winuiGetHandle<NumberBox>(ih);
    if (nb)
      nb.Background(brush);
  }
  else if (aux->isPassword)
  {
    PasswordBox pb = winuiGetHandle<PasswordBox>(ih);
    if (pb)
      pb.Background(brush);
  }
  else if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.Background(brush);
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.Background(brush);
  }

  return 1;
}

static int winuiTextSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux)
    return 0;

  Windows::UI::Color color;
  color.A = 255; color.R = r; color.G = g; color.B = b;
  SolidColorBrush brush(color);

  if (aux->isSpin)
  {
    NumberBox nb = winuiGetHandle<NumberBox>(ih);
    if (nb)
      nb.Foreground(brush);
  }
  else if (aux->isPassword)
  {
    PasswordBox pb = winuiGetHandle<PasswordBox>(ih);
    if (pb)
      pb.Foreground(brush);
  }
  else if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.Foreground(brush);
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.Foreground(brush);
  }

  return 1;
}

static int winuiTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &(ih->data->horiz_padding), &(ih->data->vert_padding), 'x');

  if (!ih->handle)
    return 1;

  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;

  Thickness padding = {(double)ih->data->horiz_padding, (double)ih->data->vert_padding,
                       (double)ih->data->horiz_padding, (double)ih->data->vert_padding};

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
      reb.Padding(padding);
  }
  else
  {
    TextBox tb = winuiGetHandle<TextBox>(ih);
    if (tb)
      tb.Padding(padding);
  }

  return 0;
}

static int winuiTextSetCueBannerAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || aux->isSpin || aux->isPassword)
    return 0;

  if (ih->data->is_multiline)
    return 0;

  TextBox tb = winuiGetHandle<TextBox>(ih);
  if (tb)
    tb.PlaceholderText(iupwinuiStringToHString(value ? value : ""));

  return 1;
}

static int winuiTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || !ih->data->is_multiline)
    return 0;

  int tabsize = 8;
  iupStrToInt(value, &tabsize);

  if (aux->isFormatted)
  {
    RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
    if (reb)
    {
      int charwidth;
      iupdrvFontGetCharSize(ih, &charwidth, NULL);
      float tabStopPoints = (float)(tabsize * charwidth) * 72.0f / (float)iupdrvGetScreenDpi();
      reb.Document().DefaultTabStop(tabStopPoints);
    }
  }

  return 1;
}

static int winuiTextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
  IupWinUITextAux* aux = winuiGetAux<IupWinUITextAux>(ih, IUPWINUI_TEXT_AUX);
  if (!aux || !aux->isFormatted)
    return 0;

  RichEditBox reb = winuiGetHandle<RichEditBox>(ih);
  if (!reb)
    return 0;

  auto doc = reb.Document();
  auto defaultCF = doc.GetDefaultCharacterFormat();
  auto defaultPF = doc.GetDefaultParagraphFormat();

  if (iupStrEqualNoCase(value, "ALL"))
  {
    hstring text;
    doc.GetText(TextGetOptions::None, text);
    auto range = doc.GetRange(0, (int32_t)text.size());
    range.CharacterFormat().SetClone(defaultCF);
    range.ParagraphFormat().SetClone(defaultPF);
  }
  else
  {
    auto sel = doc.Selection();
    if (sel.StartPosition() != sel.EndPosition())
    {
      auto range = sel.GetClone();
      range.CharacterFormat().SetClone(defaultCF);
      range.ParagraphFormat().SetClone(defaultPF);
    }
  }

  return 0;
}

extern "C" void iupdrvTextInitClass(Iclass* ic)
{
  ic->Map = winuiTextMapMethod;
  ic->UnMap = winuiTextUnMapMethod;

  iupClassRegisterAttribute(ic, "VALUE", winuiTextGetValueAttrib, winuiTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, winuiTextSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", NULL, winuiTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACTIVE", NULL, winuiTextSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "SPINMIN", NULL, winuiTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, winuiTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, winuiTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", winuiTextGetSpinValueAttrib, winuiTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_IHANDLE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SELECTEDTEXT", winuiTextGetSelectedTextAttrib, winuiTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", winuiTextGetSelectionAttrib, winuiTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", winuiTextGetSelectionPosAttrib, winuiTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CARET", winuiTextGetCaretAttrib, winuiTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", winuiTextGetCaretPosAttrib, winuiTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "INSERT", NULL, winuiTextSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, winuiTextSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COUNT", winuiTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", winuiTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, winuiTextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, winuiTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, winuiTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, winuiTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winuiTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winuiTextSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, winuiTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, winuiTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, winuiTextSetTabSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, winuiTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
