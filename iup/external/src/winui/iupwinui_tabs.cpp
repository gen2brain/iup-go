/** \file
 * \brief WinUI Driver - Tabs Control
 *
 * Uses WinUI TabView control.
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
#include "iup_tabs.h"
#include "iup_image.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;


#define IUPWINUI_TABS_AUX "_IUPWINUI_TABS_AUX"
#define IUPWINUI_TABITEMNATIVE "_IUPWINUI_TABITEMNATIVE"
#define IUPWINUI_TABIMAGE_NATIVE "_IUPWINUI_TABIMAGE_NATIVE"
#define IUPWINUI_TABLABEL_NATIVE "_IUPWINUI_TABLABEL_NATIVE"

struct IupWinUITabsAux
{
  event_token selectionChangedToken;
  event_token tabCloseToken;
  event_token keyDownToken;
  int ignoreChange;
  int previousIndex;

  IupWinUITabsAux() : selectionChangedToken{}, tabCloseToken{}, keyDownToken{}, ignoreChange(0), previousIndex(0) {}
};

static TabView winuiTabsGetTabView(Ihandle* ih)
{
  return winuiGetHandle<TabView>(ih);
}

static void winuiTabsSelectionChanged(Ihandle* ih, IInspectable const& sender, SelectionChangedEventArgs const& args)
{
  (void)sender;
  (void)args;

  IupWinUITabsAux* aux = winuiGetAux<IupWinUITabsAux>(ih, IUPWINUI_TABS_AUX);
  if (!aux || aux->ignoreChange)
    return;

  TabView tabView = winuiTabsGetTabView(ih);
  if (!tabView)
    return;

  int newIndex = tabView.SelectedIndex();
  int oldIndex = aux->previousIndex;

  if (newIndex == oldIndex)
    return;

  Ihandle* newChild = IupGetChild(ih, newIndex);
  Ihandle* oldChild = IupGetChild(ih, oldIndex);

  IFnnn cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
  if (cb)
    cb(ih, newChild, oldChild);

  IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
  if (cb2)
    cb2(ih, newIndex, oldIndex);

  aux->previousIndex = newIndex;
}

static void winuiTabsCloseRequested(Ihandle* ih, TabView const& sender, TabViewTabCloseRequestedEventArgs const& args)
{
  (void)sender;

  TabViewItem item = args.Tab();
  if (!item)
    return;

  TabView tabView = winuiTabsGetTabView(ih);
  if (!tabView)
    return;

  int pos = -1;
  uint32_t count = tabView.TabItems().Size();
  for (uint32_t i = 0; i < count; i++)
  {
    auto tabItem = tabView.TabItems().GetAt(i).try_as<TabViewItem>();
    if (tabItem == item)
    {
      pos = (int)i;
      break;
    }
  }

  if (pos < 0)
    return;

  IFni cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
  if (cb)
  {
    int ret = cb(ih, pos);
    if (ret == IUP_CONTINUE)
    {
      Ihandle* child = IupGetChild(ih, pos);
      if (child)
      {
        IupDetach(child);
        IupDestroy(child);
      }
    }
  }
}

static Controls::Image winuiTabsGetTabImage(Ihandle* child)
{
  void* ptr = (void*)iupAttribGet(child, IUPWINUI_TABIMAGE_NATIVE);
  if (!ptr)
    return nullptr;

  IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, ptr);
  return obj.try_as<Controls::Image>();
}

static TextBlock winuiTabsGetTabLabel(Ihandle* child)
{
  void* ptr = (void*)iupAttribGet(child, IUPWINUI_TABLABEL_NATIVE);
  if (!ptr)
    return nullptr;

  IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, ptr);
  return obj.try_as<TextBlock>();
}

static void winuiTabsSetItemIcon(Ihandle* child, const char* tabimage, Ihandle* ih)
{
  Controls::Image img = winuiTabsGetTabImage(child);
  if (!img)
    return;

  if (!tabimage)
  {
    img.Visibility(Visibility::Collapsed);
    return;
  }

  void* imghandle = iupImageGetImage(tabimage, ih, 0, NULL);
  WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
  if (!bitmap)
    return;

  img.Source(bitmap);
  img.Visibility(Visibility::Visible);
}

static TabViewItem winuiTabsGetTabViewItem(Ihandle* child)
{
  void* ptr = (void*)iupAttribGet(child, IUPWINUI_TABITEMNATIVE);
  if (!ptr)
    return nullptr;

  IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, ptr);
  return obj.try_as<TabViewItem>();
}

static void winuiTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (!ih->handle)
    return;

  TabView tabView = winuiTabsGetTabView(ih);
  if (!tabView)
    return;

  TabViewItem item;

  int pos = IupGetChildPos(ih, child);

  StackPanel headerPanel;
  headerPanel.Orientation(Orientation::Horizontal);
  headerPanel.Spacing(4);

  Controls::Image tabImage;
  tabImage.Stretch(Stretch::None);
  tabImage.VerticalAlignment(VerticalAlignment::Center);
  tabImage.Visibility(Visibility::Collapsed);
  headerPanel.Children().Append(tabImage);

  char* title = iupAttribGetId(ih, "TABTITLE", pos);
  if (!title) title = iupAttribGet(child, "TABTITLE");

  TextBlock tabLabel;
  if (title)
    tabLabel.Text(iupwinuiStringToHString(title));
  else
    tabLabel.Text(hstring(L"Tab"));
  tabLabel.VerticalAlignment(VerticalAlignment::Center);

  if (ih->data->horiz_padding || ih->data->vert_padding)
    tabLabel.Margin(ThicknessHelper::FromLengths(ih->data->horiz_padding, ih->data->vert_padding, ih->data->horiz_padding, ih->data->vert_padding));

  tabLabel.FontWeight(Windows::UI::Text::FontWeights::SemiBold());
  tabLabel.Measure(Size(10000.0f, 10000.0f));
  tabLabel.MinWidth(tabLabel.DesiredSize().Width);
  tabLabel.ClearValue(TextBlock::FontWeightProperty());

  headerPanel.Children().Append(tabLabel);

  item.Header(headerPanel);

  void* imgPtr = nullptr;
  winrt::copy_to_abi(tabImage, imgPtr);
  iupAttribSet(child, IUPWINUI_TABIMAGE_NATIVE, (char*)imgPtr);

  void* lblPtr = nullptr;
  winrt::copy_to_abi(tabLabel, lblPtr);
  iupAttribSet(child, IUPWINUI_TABLABEL_NATIVE, (char*)lblPtr);

  char* tabimage = iupAttribGetId(ih, "TABIMAGE", pos);
  if (!tabimage) tabimage = iupAttribGet(child, "TABIMAGE");
  if (tabimage)
    winuiTabsSetItemIcon(child, tabimage, ih);

  if (iupAttribGetBoolean(ih, "SHOWCLOSE"))
    item.IsClosable(true);
  else
    item.IsClosable(false);

  Canvas contentCanvas;
  contentCanvas.HorizontalAlignment(HorizontalAlignment::Stretch);
  contentCanvas.VerticalAlignment(VerticalAlignment::Stretch);
  item.Content(contentCanvas);

  void* canvasPtr = nullptr;
  winrt::copy_to_abi(contentCanvas, canvasPtr);
  iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)canvasPtr);

  void* itemPtr = nullptr;
  winrt::copy_to_abi(item, itemPtr);
  iupAttribSet(child, IUPWINUI_TABITEMNATIVE, (char*)itemPtr);

  iupAttribSetHandleName(child);

  tabView.TabItems().Append(item);
}

static int winuiTabsMapMethod(Ihandle* ih)
{
  TabView tabView;
  tabView.HorizontalAlignment(HorizontalAlignment::Left);
  tabView.VerticalAlignment(VerticalAlignment::Top);

  tabView.IsAddTabButtonVisible(false);
  tabView.TabWidthMode(TabViewWidthMode::SizeToContent);
  tabView.CanReorderTabs(false);

  IupWinUITabsAux* aux = new IupWinUITabsAux();

  aux->selectionChangedToken = tabView.SelectionChanged([ih](IInspectable const& sender, SelectionChangedEventArgs const& args) {
    winuiTabsSelectionChanged(ih, sender, args);
  });

  aux->tabCloseToken = tabView.TabCloseRequested([ih](TabView const& sender, TabViewTabCloseRequestedEventArgs const& args) {
    winuiTabsCloseRequested(ih, sender, args);
  });

  aux->keyDownToken = tabView.PreviewKeyDown([ih](IInspectable const&, Input::KeyRoutedEventArgs const& args) {
    if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
      args.Handled(true);
  });

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(tabView);

  winuiSetAux(ih, IUPWINUI_TABS_AUX, aux);
  winuiStoreHandle(ih, tabView);

  Ihandle* child = ih->firstchild;
  while (child)
  {
    winuiTabsChildAddedMethod(ih, child);
    child = child->brother;
  }

  return IUP_NOERROR;
}

static void winuiTabsUnMapMethod(Ihandle* ih)
{
  IupWinUITabsAux* aux = winuiGetAux<IupWinUITabsAux>(ih, IUPWINUI_TABS_AUX);
  if (aux)
  {
    TabView tabView = winuiTabsGetTabView(ih);
    if (tabView)
    {
      tabView.SelectionChanged(aux->selectionChangedToken);
      tabView.TabCloseRequested(aux->tabCloseToken);
      if (aux->keyDownToken)
        tabView.PreviewKeyDown(aux->keyDownToken);
    }
    delete aux;
    iupAttribSet(ih, IUPWINUI_TABS_AUX, nullptr);
  }

  if (ih->handle)
    winuiReleaseHandle<TabView>(ih);
  ih->handle = nullptr;
}

static void winuiTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (!ih->handle)
    return;

  TabView tabView = winuiTabsGetTabView(ih);
  if (!tabView)
    return;

  iupTabsCheckCurrentTab(ih, pos, 1);

  if (pos >= 0 && (uint32_t)pos < tabView.TabItems().Size())
    tabView.TabItems().RemoveAt(pos);

  iupAttribSet(child, "_IUPTAB_CONTAINER", nullptr);
  iupAttribSet(child, IUPWINUI_TABITEMNATIVE, nullptr);
  iupAttribSet(child, IUPWINUI_TABIMAGE_NATIVE, nullptr);
  iupAttribSet(child, IUPWINUI_TABLABEL_NATIVE, nullptr);
}

static int winuiTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (!child)
    return 1;

  TextBlock tabLabel = winuiTabsGetTabLabel(child);
  if (tabLabel)
  {
    tabLabel.Text(iupwinuiStringToHString(value));

    tabLabel.FontWeight(Windows::UI::Text::FontWeights::SemiBold());
    tabLabel.Measure(Size(10000.0f, 10000.0f));
    tabLabel.MinWidth(tabLabel.DesiredSize().Width);
    tabLabel.ClearValue(TextBlock::FontWeightProperty());
  }

  return 1;
}

static int winuiTabsSetShowCloseAttrib(Ihandle* ih, const char* value)
{
  TabView tabView = winuiTabsGetTabView(ih);
  if (!tabView)
    return 1;

  bool closable = iupStrBoolean(value) ? true : false;

  uint32_t count = tabView.TabItems().Size();
  for (uint32_t i = 0; i < count; i++)
  {
    auto item = tabView.TabItems().GetAt(i).try_as<TabViewItem>();
    if (item)
      item.IsClosable(closable);
  }

  return 1;
}

static int winuiTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (!child)
    return 1;

  winuiTabsSetItemIcon(child, value, ih);

  return 1;
}

extern "C" int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  TabView tabView = winuiTabsGetTabView(ih);
  if (!tabView)
    return 0;
  return tabView.SelectedIndex();
}

extern "C" void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  TabView tabView = winuiTabsGetTabView(ih);
  if (!tabView)
    return;

  IupWinUITabsAux* aux = winuiGetAux<IupWinUITabsAux>(ih, IUPWINUI_TABS_AUX);
  if (aux)
    aux->ignoreChange = 1;

  tabView.SelectedIndex(pos);

  if (aux)
  {
    aux->ignoreChange = 0;
    aux->previousIndex = pos;
  }
}

extern "C" int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  (void)pos;
  TabViewItem item = winuiTabsGetTabViewItem(child);
  if (item)
    return item.Visibility() == Visibility::Visible ? 1 : 0;
  return 1;
}

extern "C" int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

extern "C" int iupdrvTabsExtraMargin(void)
{
  return 0;
}

extern "C" int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  (void)ih;
  return 1;
}

extern "C" void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
  int w = 0, h = 0;

  if (tab_title)
    iupdrvFontGetMultiLineStringSize(ih, tab_title, &w, &h);

  if (tab_image)
  {
    void* img = iupImageGetImage(tab_image, ih, 0, NULL);
    if (img)
    {
      int img_w = 0, img_h = 0;
      iupdrvImageGetInfo(img, &img_w, &img_h, NULL);
      w += img_w;
      if (img_h > h) h = img_h;
    }
  }

  w += 24;
  h += 10;

  if (w < 90) w = 90;
  if (h < 32) h = 32;

  *tab_width = w;
  *tab_height = h;
}

static int winuiTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (!child)
    return 0;

  TabViewItem item = winuiTabsGetTabViewItem(child);
  if (item)
    item.Visibility(iupStrBoolean(value) ? Visibility::Visible : Visibility::Collapsed);

  return 0;
}

static void winuiTabsUpdatePagePadding(Ihandle* ih)
{
  Ihandle* child;
  Thickness margin = ThicknessHelper::FromLengths(ih->data->horiz_padding, ih->data->vert_padding, ih->data->horiz_padding, ih->data->vert_padding);

  for (child = ih->firstchild; child; child = child->brother)
  {
    TextBlock tabLabel = winuiTabsGetTabLabel(child);
    if (tabLabel)
      tabLabel.Margin(margin);
  }
}

static int winuiTabsSetTabPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

  if (ih->handle)
  {
    winuiTabsUpdatePagePadding(ih);
    return 0;
  }
  else
    return 1;
}

static int winuiTabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  Windows::UI::Color color;
  color.A = 255;
  color.R = r;
  color.G = g;
  color.B = b;
  SolidColorBrush brush(color);

  Ihandle* child;
  for (child = ih->firstchild; child; child = child->brother)
  {
    TextBlock tabLabel = winuiTabsGetTabLabel(child);
    if (tabLabel)
      tabLabel.Foreground(brush);
  }

  return 1;
}

static int winuiTabsSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  TabView tabView = winuiTabsGetTabView(ih);
  if (tabView)
    tabView.CanReorderTabs(iupStrBoolean(value) ? true : false);
  return 1;
}

extern "C" void iupdrvTabsInitClass(Iclass* ic)
{
  ic->Map = winuiTabsMapMethod;
  ic->UnMap = winuiTabsUnMapMethod;
  ic->ChildAdded = winuiTabsChildAddedMethod;
  ic->ChildRemoved = winuiTabsChildRemovedMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winuiTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  /* IupTabs only */
  iupClassRegisterAttributeId(ic, "TABTITLE", NULL, winuiTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, winuiTabsSetTabImageAttrib, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, winuiTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABPADDING", iupTabsGetTabPaddingAttrib, winuiTabsSetTabPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWCLOSE", NULL, winuiTabsSetShowCloseAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, winuiTabsSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "TABTYPE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTILINE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
