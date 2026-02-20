/** \file
 * \brief WinUI Driver - List Control
 *
 * Uses ListBox for plain lists, ComboBox for dropdown mode,
 * and Grid+TextBox+ListBox for editbox mode.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_image.h"
#include "iup_list.h"
#include "iup_key.h"
#include "iup_classbase.h"
#include "iup_register.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Input;
using namespace Windows::Foundation;
using namespace Windows::UI::Text;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Microsoft::UI::Input;


/****************************************************************************
 * Manual Drag Gesture Tracking for ListBox
 *
 * ListBox captures the pointer for selection, preventing UIElement's
 * automatic drag gesture detection from firing DragStarting.
 * We manually detect press+move and call StartDragAsync to initiate drags.
 ****************************************************************************/

static ListBoxItem s_listDragItem{nullptr};
static PointerPoint s_listDragPointerPoint{nullptr};
static float s_listDragStartX = 0;
static float s_listDragStartY = 0;

static void winuiListClearDragTracking()
{
  s_listDragItem = nullptr;
  s_listDragPointerPoint = nullptr;
  s_listDragStartX = 0;
  s_listDragStartY = 0;
}


static void winuiListUpdateTextBlockFont(Ihandle* ih, TextBlock tb)
{
  iupwinuiUpdateTextBlockFont(ih, tb);
}

static ListBox winuiListGetListBox(Ihandle* ih)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || aux->isDropdown || aux->isVirtual)
    return nullptr;

  if (aux->hasEditbox)
  {
    char* ptr = iupAttribGet(ih, "_IUPWINUI_LISTBOX");
    if (ptr)
    {
      IInspectable obj{nullptr};
      winrt::copy_from_abi(obj, ptr);
      return obj.try_as<ListBox>();
    }
    return nullptr;
  }

  return winuiGetHandle<ListBox>(ih);
}

static ListView winuiListGetListView(Ihandle* ih)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || !aux->isVirtual)
    return nullptr;

  return winuiGetHandle<ListView>(ih);
}

static TextBox winuiListGetTextBox(Ihandle* ih)
{
  char* ptr = iupAttribGet(ih, "_IUPWINUI_TEXTBOX");
  if (ptr)
  {
    IInspectable obj{nullptr};
    winrt::copy_from_abi(obj, ptr);
    return obj.try_as<TextBox>();
  }
  return nullptr;
}

static hstring winuiListGetItemText(IInspectable const& content)
{
  if (!content)
    return hstring{};

  TextBlock tb = content.try_as<TextBlock>();
  if (tb)
    return tb.Text();

  StackPanel sp = content.try_as<StackPanel>();
  if (sp)
  {
    for (uint32_t i = 0; i < sp.Children().Size(); i++)
    {
      tb = sp.Children().GetAt(i).try_as<TextBlock>();
      if (tb)
        return tb.Text();
    }
    return hstring{};
  }

  return unbox_value_or<hstring>(content, hstring{});
}

static void winuiListGetScaledImageSize(Ihandle* ih, void* imghandle, int* w, int* h)
{
  int img_w = 0, img_h = 0;
  iupdrvImageGetInfo(imghandle, &img_w, &img_h, NULL);

  *w = img_w;
  *h = img_h;

  if (ih->data->fit_image && img_h > 0)
  {
    int charheight;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    int available_height = charheight + 2 * ih->data->spacing;

    if (img_h > available_height)
    {
      *w = (img_w * available_height) / img_h;
      *h = available_height;
    }
  }
}

static void winuiListApplyImageSize(Image const& img, int draw_w, int draw_h, int img_w, int img_h)
{
  if (draw_w != img_w || draw_h != img_h)
  {
    img.Width(draw_w);
    img.Height(draw_h);
    img.Stretch(Stretch::Uniform);
  }
  else
  {
    img.ClearValue(FrameworkElement::WidthProperty());
    img.ClearValue(FrameworkElement::HeightProperty());
    img.Stretch(Stretch::None);
  }
}

static void winuiListSetItemImage(Ihandle* ih, ListBoxItem const& item, void* imghandle)
{
  hstring text;
  auto content = item.Content();
  if (content)
    text = winuiListGetItemText(content);

  if (!imghandle)
  {
    TextBlock plainTb;
    plainTb.Text(text);
    winuiListUpdateTextBlockFont(ih, plainTb);
    item.Content(plainTb);
    item.Tag(nullptr);
    return;
  }

  WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
  if (!bitmap)
    return;

  int img_w = bitmap.PixelWidth();
  int img_h = bitmap.PixelHeight();
  int draw_w, draw_h;
  winuiListGetScaledImageSize(ih, imghandle, &draw_w, &draw_h);

  StackPanel existingSp = content ? content.try_as<StackPanel>() : nullptr;
  if (existingSp)
  {
    for (uint32_t i = 0; i < existingSp.Children().Size(); i++)
    {
      Image img = existingSp.Children().GetAt(i).try_as<Image>();
      if (img)
      {
        img.Source(bitmap);
        winuiListApplyImageSize(img, draw_w, draw_h, img_w, img_h);
        item.Tag(box_value((int64_t)(intptr_t)imghandle));
        return;
      }
    }
  }

  StackPanel sp;
  sp.Orientation(Orientation::Horizontal);

  Image img;
  img.Source(bitmap);
  winuiListApplyImageSize(img, draw_w, draw_h, img_w, img_h);
  img.VerticalAlignment(VerticalAlignment::Center);
  sp.Children().Append(img);

  TextBlock tb;
  tb.Text(text);
  tb.Margin(ThicknessHelper::FromLengths(4, 0, 0, 0));
  tb.VerticalAlignment(VerticalAlignment::Center);
  winuiListUpdateTextBlockFont(ih, tb);
  sp.Children().Append(tb);

  item.Content(sp);
  item.Tag(box_value((int64_t)(intptr_t)imghandle));
}

static void winuiListSetComboItemImage(Ihandle* ih, ComboBox const& comboBox, int pos, void* imghandle)
{
  auto currentItem = comboBox.Items().GetAt(pos);
  hstring text = winuiListGetItemText(currentItem);

  if (!imghandle)
  {
    comboBox.Items().SetAt(pos, box_value(text));
    return;
  }

  WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
  if (!bitmap)
    return;

  int img_w = bitmap.PixelWidth();
  int img_h = bitmap.PixelHeight();
  int draw_w, draw_h;
  winuiListGetScaledImageSize(ih, imghandle, &draw_w, &draw_h);

  StackPanel existingSp = currentItem.try_as<StackPanel>();
  if (existingSp)
  {
    for (uint32_t i = 0; i < existingSp.Children().Size(); i++)
    {
      Image img = existingSp.Children().GetAt(i).try_as<Image>();
      if (img)
      {
        img.Source(bitmap);
        winuiListApplyImageSize(img, draw_w, draw_h, img_w, img_h);
        existingSp.Tag(box_value((int64_t)(intptr_t)imghandle));
        return;
      }
    }
  }

  StackPanel sp;
  sp.Orientation(Orientation::Horizontal);

  Image img;
  img.Source(bitmap);
  winuiListApplyImageSize(img, draw_w, draw_h, img_w, img_h);
  img.VerticalAlignment(VerticalAlignment::Center);
  sp.Children().Append(img);

  TextBlock tb;
  tb.Text(text);
  tb.Margin(ThicknessHelper::FromLengths(4, 0, 0, 0));
  tb.VerticalAlignment(VerticalAlignment::Center);
  sp.Children().Append(tb);

  sp.Tag(box_value((int64_t)(intptr_t)imghandle));

  comboBox.Items().SetAt(pos, sp);
}

static void winuiListCallAction(Ihandle* ih, int pos)
{
  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
  if (cb)
    iupListSingleCallActionCb(ih, cb, pos);
}

static void winuiListSelectionChanged(Ihandle* ih)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux)
    return;

  if (aux->ignoreChange)
  {
    aux->ignoreChange = false;
    return;
  }

  if (iupAttribGet(ih, "_IUPLIST_IGNORE_ACTION"))
    return;

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox)
    {
      int pos = comboBox.SelectedIndex() + 1;
      if (pos > 0)
        winuiListCallAction(ih, pos);
    }
  }
  else if (aux->isVirtual)
  {
    ListView listView = winuiListGetListView(ih);
    if (listView)
    {
      int pos = listView.SelectedIndex() + 1;
      if (pos > 0)
        winuiListCallAction(ih, pos);
    }
  }
  else if (!aux->isMultiple)
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
    {
      int pos = listBox.SelectedIndex() + 1;
      if (pos > 0)
      {
        if (aux->hasEditbox)
        {
          auto listItem = listBox.Items().GetAt(pos - 1).try_as<ListBoxItem>();
          if (listItem)
          {
            auto content = listItem.Content();
            if (content)
            {
              hstring text = winuiListGetItemText(content);
              TextBox tb = winuiListGetTextBox(ih);
              if (tb)
              {
                iupAttribSet(ih, "_IUPWINUI_DISABLE_TEXT_CB", "1");
                tb.Text(text);
                iupAttribSet(ih, "_IUPWINUI_DISABLE_TEXT_CB", NULL);
              }
            }
          }
        }
        winuiListCallAction(ih, pos);
      }
    }
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
    {
      IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
      IFns multi_cb = (IFns)IupGetCallback(ih, "MULTISELECT_CB");

      if (cb || multi_cb)
      {
        auto selectedItems = listBox.SelectedItems();
        int sel_count = (int)selectedItems.Size();

        if (sel_count > 0)
        {
          int* pos_array = (int*)malloc(sel_count * sizeof(int));
          if (pos_array)
          {
            for (int i = 0; i < sel_count; i++)
            {
              auto item = selectedItems.GetAt(i);
              uint32_t index;
              if (listBox.Items().IndexOf(item, index))
                pos_array[i] = (int)index + 1;
              else
                pos_array[i] = 0;
            }
            iupListMultipleCallActionCb(ih, cb, multi_cb, pos_array, sel_count);
            free(pos_array);
          }
        }
      }
    }
  }

  iupBaseCallValueChangedCb(ih);
}

static void winuiListDropDownOpened(Ihandle* ih, int state)
{
  IFni cb = (IFni)IupGetCallback(ih, "DROPDOWN_CB");
  if (cb)
    cb(ih, state);
}

static void winuiListDoubleTapped(Ihandle* ih)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || aux->isDropdown)
    return;

  int pos = -1;

  if (aux->isVirtual)
  {
    ListView listView = winuiListGetListView(ih);
    if (listView)
      pos = listView.SelectedIndex();
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
      pos = listBox.SelectedIndex();
  }

  if (pos >= 0)
  {
    IFnis cb = (IFnis)IupGetCallback(ih, "DBLCLICK_CB");
    if (cb)
      iupListSingleCallDblClickCb(ih, cb, pos + 1);
  }
}

static int winuiListSetValueAttrib(Ihandle* ih, const char* value)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux)
    return 0;

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");
  aux->ignoreChange = true;

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox)
    {
      if (aux->hasEditbox && value && !iupStrToInt(value, NULL))
      {
        comboBox.Text(iupwinuiStringToHString(value));
      }
      else
      {
        int pos = 0;
        if (iupStrToInt(value, &pos) && pos > 0 && pos <= (int)comboBox.Items().Size())
          comboBox.SelectedIndex(pos - 1);
        else
          comboBox.SelectedIndex(-1);
      }
    }
  }
  else if (aux->hasEditbox)
  {
    if (value && !iupStrToInt(value, NULL))
    {
      TextBox tb = winuiListGetTextBox(ih);
      if (tb)
        tb.Text(iupwinuiStringToHString(value));
    }
    else
    {
      ListBox listBox = winuiListGetListBox(ih);
      if (listBox)
      {
        int pos = 0;
        if (iupStrToInt(value, &pos) && pos > 0 && pos <= (int)listBox.Items().Size())
          listBox.SelectedIndex(pos - 1);
        else
          listBox.SelectedIndex(-1);
      }
    }
  }
  else if (aux->isVirtual)
  {
    ListView listView = winuiListGetListView(ih);
    if (listView)
    {
      int pos = 0;
      if (iupStrToInt(value, &pos) && pos > 0 && pos <= (int)listView.Items().Size())
        listView.SelectedIndex(pos - 1);
      else
        listView.SelectedIndex(-1);
    }
  }
  else if (!aux->isMultiple)
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
    {
      int pos = 0;
      if (iupStrToInt(value, &pos) && pos > 0 && pos <= (int)listBox.Items().Size())
        listBox.SelectedIndex(pos - 1);
      else
        listBox.SelectedIndex(-1);
    }
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
    {
      listBox.SelectedItems().Clear();

      if (value)
      {
        int len = (int)strlen(value);
        int count = (int)listBox.Items().Size();
        for (int i = 0; i < len && i < count; i++)
        {
          if (value[i] == '+')
            listBox.SelectedItems().Append(listBox.Items().GetAt(i));
        }
      }
    }
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
  return 0;
}

static char* winuiListGetValueAttrib(Ihandle* ih)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux)
    return NULL;

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox)
    {
      if (aux->hasEditbox)
      {
        hstring text = comboBox.Text();
        if (!text.empty())
          return iupwinuiHStringToString(text);
      }
      int pos = comboBox.SelectedIndex();
      if (pos >= 0)
        return iupStrReturnInt(pos + 1);
    }
  }
  else if (aux->hasEditbox)
  {
    TextBox tb = winuiListGetTextBox(ih);
    if (tb)
    {
      hstring text = tb.Text();
      if (!text.empty())
        return iupwinuiHStringToString(text);
    }

    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
    {
      int pos = listBox.SelectedIndex();
      if (pos >= 0)
        return iupStrReturnInt(pos + 1);
    }
  }
  else if (aux->isVirtual)
  {
    ListView listView = winuiListGetListView(ih);
    if (listView)
    {
      int pos = listView.SelectedIndex();
      if (pos >= 0)
        return iupStrReturnInt(pos + 1);
    }
  }
  else if (!aux->isMultiple)
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
    {
      int pos = listBox.SelectedIndex();
      if (pos >= 0)
        return iupStrReturnInt(pos + 1);
    }
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
    {
      int count = (int)listBox.Items().Size();
      char* str = iupStrGetMemory(count + 1);

      for (int i = 0; i < count; i++)
      {
        auto item = listBox.Items().GetAt(i);
        bool selected = false;
        auto selectedItems = listBox.SelectedItems();
        for (uint32_t j = 0; j < selectedItems.Size(); j++)
        {
          if (selectedItems.GetAt(j) == item)
          {
            selected = true;
            break;
          }
        }
        str[i] = selected ? '+' : '-';
      }
      str[count] = 0;
      return str;
    }
  }

  return NULL;
}

static char* winuiListGetIdValueAttrib(Ihandle* ih, int pos)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux)
    return NULL;

  if (aux->isVirtual)
    return iupListGetItemValueCb(ih, pos);

  pos--;

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox && pos >= 0 && (uint32_t)pos < comboBox.Items().Size())
    {
      auto item = comboBox.Items().GetAt(pos);
      hstring text = winuiListGetItemText(item);
      return iupwinuiHStringToString(text);
    }
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox && pos >= 0 && (uint32_t)pos < listBox.Items().Size())
    {
      auto listItem = listBox.Items().GetAt(pos).try_as<ListBoxItem>();
      if (listItem)
      {
        auto content = listItem.Content();
        if (content)
        {
          hstring text = winuiListGetItemText(content);
          return iupwinuiHStringToString(text);
        }
      }
    }
  }

  return NULL;
}

static int winuiListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || !aux->isDropdown)
    return 0;

  ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
  if (comboBox)
    comboBox.IsDropDownOpen(iupStrBoolean(value));

  return 0;
}

static int winuiListSetTopItemAttrib(Ihandle* ih, const char* value)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || aux->isDropdown)
    return 0;

  int pos = 0;
  if (!iupStrToInt(value, &pos) || pos <= 0)
    return 0;

  if (aux->isVirtual)
  {
    ListView listView = winuiListGetListView(ih);
    if (listView && pos <= (int)listView.Items().Size())
      listView.ScrollIntoView(listView.Items().GetAt(pos - 1));
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox && pos <= (int)listBox.Items().Size())
      listBox.ScrollIntoView(listBox.Items().GetAt(pos - 1));
  }

  return 0;
}

/****************************************************************************
 * ListBox Per-Item Drag Support
 *
 * ListBox captures the pointer for selection, preventing UIElement's
 * automatic drag gesture detection (CanDrag) from firing DragStarting.
 * Each item sets CanDrag(true) and hooks PointerPressed for tracking.
 * The ListBox-level PointerMoved handler detects drag threshold and
 * calls StartDragAsync to programmatically initiate the drag, which
 * fires the item's DragStarting handler.
 *
 * ListView (virtual mode) inherits from ListViewBase which supports
 * CanDragItems and DragItemsStarting directly.
 ****************************************************************************/

static void winuiListSetItemDragSource(Ihandle* ih, ListBoxItem const& item)
{
  item.DragStarting([ih](UIElement const&, DragStartingEventArgs const& e) {
    IFnii dragbegin_cb = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
    if (dragbegin_cb)
    {
      auto pos = e.GetPosition(nullptr);
      int ret = dragbegin_cb(ih, (int)pos.X, (int)pos.Y);
      if (ret == IUP_IGNORE)
      {
        e.Cancel(true);
        return;
      }
    }

    IFns datasize_cb = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
    IFnsVi dragdata_cb = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");
    char* drag_types = iupAttribGet(ih, "DRAGTYPES");

    if (drag_types && datasize_cb && dragdata_cb)
    {
      int size = datasize_cb(ih, drag_types);
      if (size > 0)
      {
        void* data = malloc(size);
        if (data)
        {
          dragdata_cb(ih, drag_types, data, size);
          winuiDragSetInProcessData(drag_types, data, size);
          e.Data().SetText(iupwinuiStringToHString(drag_types));
        }
      }
    }

    char* move = iupAttribGet(ih, "DRAGSOURCEMOVE");
    if (iupStrBoolean(move))
      e.Data().RequestedOperation(DataPackageOperation::Move);
    else
      e.Data().RequestedOperation(DataPackageOperation::Copy);
  });

  item.DropCompleted([ih](UIElement const&, DropCompletedEventArgs const& e) {
    IFni dragend_cb = (IFni)IupGetCallback(ih, "DRAGEND_CB");
    if (dragend_cb)
    {
      int del = (e.DropResult() == DataPackageOperation::Move) ? 1 : 0;
      dragend_cb(ih, del);
    }
    winuiDragDataCleanup();
  });
}

static void winuiListSetItemShowDragDrop(Ihandle* ih, ListBoxItem const& item)
{
  item.DragStarting([ih](UIElement const& sender, DragStartingEventArgs const& e) {
    ListBox listBox = winuiListGetListBox(ih);
    if (!listBox)
    {
      e.Cancel(true);
      return;
    }

    auto lbi = sender.try_as<ListBoxItem>();
    uint32_t index;
    if (!lbi || !listBox.Items().IndexOf(lbi, index))
    {
      e.Cancel(true);
      return;
    }

    iupAttribSetInt(ih, "_IUPLIST_DRAGITEM", (int)index);
    e.Data().SetText(L"");
    e.Data().RequestedOperation(DataPackageOperation::Move);
  });
}

static ListBoxItem winuiListCreateItem(Ihandle* ih, const char* value)
{
  ListBoxItem item;

  TextBlock tb;
  tb.Text(iupwinuiStringToHString(value));
  winuiListUpdateTextBlockFont(ih, tb);
  item.Content(tb);

  item.Padding(ThicknessHelper::FromLengths(4, 2, 4, 2));
  item.MinHeight(0);

  if (iupStrBoolean(iupAttribGet(ih, "DRAGSOURCE")))
    winuiListSetItemDragSource(ih, item);
  else if (iupStrBoolean(iupAttribGet(ih, "SHOWDRAGDROP")))
    winuiListSetItemShowDragDrop(ih, item);

  return item;
}

static int winuiListSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  int pos = iupListGetPosAttrib(ih, id);
  if (!ih->data->show_image || pos < 0)
    return 1;

  void* imghandle = NULL;
  if (value)
    imghandle = iupImageGetImage(value, ih, 0, NULL);

  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux)
    return 1;

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox && pos >= 0 && (uint32_t)pos < comboBox.Items().Size())
      winuiListSetComboItemImage(ih, comboBox, pos, imghandle);
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox && pos >= 0 && (uint32_t)pos < listBox.Items().Size())
    {
      auto listItem = listBox.Items().GetAt(pos).try_as<ListBoxItem>();
      if (listItem)
        winuiListSetItemImage(ih, listItem, imghandle);
    }
  }

  return 1;
}

static char* winuiListGetImageNativeHandleAttrib(Ihandle* ih, int id)
{
  int pos = iupListGetPosAttrib(ih, id);
  return (char*)iupdrvListGetImageHandle(ih, pos);
}

static int winuiListConvertXYToPos(Ihandle* ih, int x, int y)
{
  ListBox listBox = winuiListGetListBox(ih);
  if (!listBox)
    return -1;

  Point pt{(float)x, (float)y};
  auto elements = Media::VisualTreeHelper::FindElementsInHostCoordinates(pt, listBox);

  for (auto const& elem : elements)
  {
    auto lbi = elem.try_as<ListBoxItem>();
    if (lbi)
    {
      uint32_t index = 0;
      if (listBox.Items().IndexOf(lbi, index))
        return (int)(index + 1);
    }
  }

  return -1;
}

static int winuiListHitTestItem(ListBox const& listBox, Point hostPt)
{
  auto elements = VisualTreeHelper::FindElementsInHostCoordinates(hostPt, listBox);
  for (auto const& elem : elements)
  {
    auto lbi = elem.try_as<ListBoxItem>();
    if (lbi)
    {
      uint32_t index = 0;
      if (listBox.Items().IndexOf(lbi, index))
        return (int)index;
    }
  }
  return -1;
}

static void winuiListSetupDragTracking(Ihandle* ih, ListBox const& listBox)
{
  listBox.AddHandler(UIElement::PointerPressedEvent(), winrt::box_value(
    PointerEventHandler([listBox](IInspectable const&, PointerRoutedEventArgs const& e) {
      auto hostPt = e.GetCurrentPoint(nullptr);
      Point pt{hostPt.Position().X, hostPt.Position().Y};
      auto elements = VisualTreeHelper::FindElementsInHostCoordinates(pt, listBox);
      for (auto const& elem : elements)
      {
        auto lbi = elem.try_as<ListBoxItem>();
        if (lbi)
        {
          lbi.CanDrag(false);
          s_listDragItem = lbi;
          s_listDragPointerPoint = e.GetCurrentPoint(lbi);
          s_listDragStartX = hostPt.Position().X;
          s_listDragStartY = hostPt.Position().Y;
          return;
        }
      }
    })), true);

  listBox.AddHandler(UIElement::PointerMovedEvent(), winrt::box_value(
    PointerEventHandler([](IInspectable const&, PointerRoutedEventArgs const& e) {
      if (!s_listDragItem)
        return;

      auto pt = e.GetCurrentPoint(nullptr);
      float dx = pt.Position().X - s_listDragStartX;
      float dy = pt.Position().Y - s_listDragStartY;
      if (dx * dx + dy * dy > 16)
      {
        auto item = s_listDragItem;
        auto pointerPoint = s_listDragPointerPoint;
        winuiListClearDragTracking();
        item.CanDrag(true);
        item.StartDragAsync(pointerPoint);
      }
    })), true);

  listBox.AddHandler(UIElement::PointerReleasedEvent(), winrt::box_value(
    PointerEventHandler([](IInspectable const&, PointerRoutedEventArgs const&) {
      winuiListClearDragTracking();
    })), true);
}

static void winuiListEnableDragDrop(Ihandle* ih)
{
  ListBox listBox = winuiListGetListBox(ih);
  if (!listBox)
    return;

  listBox.AllowDrop(true);

  for (uint32_t i = 0; i < listBox.Items().Size(); i++)
  {
    auto item = listBox.Items().GetAt(i).try_as<ListBoxItem>();
    if (item)
      winuiListSetItemShowDragDrop(ih, item);
  }

  winuiListSetupDragTracking(ih, listBox);

  listBox.DragOver([ih](IInspectable const&, DragEventArgs const& e) {
    if (iupAttribGet(ih, "_IUPLIST_DRAGITEM"))
      e.AcceptedOperation(DataPackageOperation::Move);
  });

  listBox.Drop([ih, listBox](IInspectable const&, DragEventArgs const& e) {
    if (!iupAttribGet(ih, "_IUPLIST_DRAGITEM"))
      return;

    int idDrag = iupAttribGetInt(ih, "_IUPLIST_DRAGITEM");
    int idDrop = winuiListHitTestItem(listBox, e.GetPosition(nullptr));
    int is_ctrl;

    if (iupListCallDragDropCb(ih, idDrag, idDrop, &is_ctrl) == IUP_CONTINUE)
    {
      int count = iupdrvListGetCount(ih);

      auto dragItem = listBox.Items().GetAt(idDrag).try_as<ListBoxItem>();
      hstring text;
      if (dragItem)
        text = winuiListGetItemText(dragItem.Content());

      char* ctext = iupwinuiHStringToString(text);

      if (idDrop >= 0 && idDrop < count)
      {
        iupdrvListInsertItem(ih, idDrop, ctext);
        if (idDrag > idDrop)
          idDrag++;
      }
      else
      {
        iupdrvListAppendItem(ih, ctext);
        idDrop = count;
      }

      listBox.SelectedIndex(idDrop);
      iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", idDrop + 1);

      if (!is_ctrl)
        iupdrvListRemoveItem(ih, idDrag);

      iupdrvRedrawNow(ih);
    }

    iupAttribSet(ih, "_IUPLIST_DRAGITEM", NULL);
  });
}

static int winuiListSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || aux->isDropdown)
    return 1;

  if (aux->isVirtual)
  {
    ListView listView = winuiListGetListView(ih);
    if (!listView)
      return 1;

    if (aux->dragItemsStartingToken)
    {
      listView.DragItemsStarting(aux->dragItemsStartingToken);
      aux->dragItemsStartingToken = {};
    }
    if (aux->dragItemsCompletedToken)
    {
      listView.DragItemsCompleted(aux->dragItemsCompletedToken);
      aux->dragItemsCompletedToken = {};
    }

    if (iupStrBoolean(value))
    {
      listView.CanDragItems(true);

      aux->dragItemsStartingToken = listView.DragItemsStarting(
        [ih](IInspectable const&, DragItemsStartingEventArgs const& e) {
          char* drag_types = iupAttribGet(ih, "DRAGTYPES");
          if (!drag_types)
            return;

          auto items = e.Items();
          if (items.Size() == 0)
            return;

          IFnii dragbegin_cb = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
          if (dragbegin_cb)
          {
            int ret = dragbegin_cb(ih, 0, 0);
            if (ret == IUP_IGNORE)
            {
              e.Cancel(true);
              return;
            }
          }

          IFns datasize_cb = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
          IFnsVi dragdata_cb = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");

          if (datasize_cb && dragdata_cb)
          {
            int size = datasize_cb(ih, drag_types);
            if (size > 0)
            {
              void* data = malloc(size);
              if (data)
              {
                dragdata_cb(ih, drag_types, data, size);
                winuiDragSetInProcessData(drag_types, data, size);
              }
            }
          }

          char* move = iupAttribGet(ih, "DRAGSOURCEMOVE");
          if (iupStrBoolean(move))
            e.Data().RequestedOperation(DataPackageOperation::Move);
          else
            e.Data().RequestedOperation(DataPackageOperation::Copy);

          e.Data().SetText(iupwinuiStringToHString(drag_types));
        });

      aux->dragItemsCompletedToken = listView.DragItemsCompleted(
        [ih](ListViewBase const&, DragItemsCompletedEventArgs const& e) {
          IFni dragend_cb = (IFni)IupGetCallback(ih, "DRAGEND_CB");
          if (dragend_cb)
          {
            int del = (e.DropResult() == DataPackageOperation::Move) ? 1 : 0;
            dragend_cb(ih, del);
          }
          winuiDragDataCleanup();
        });
    }
    else
    {
      listView.CanDragItems(false);
    }
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (!listBox)
      return 1;

    bool enable = iupStrBoolean(value) ? true : false;
    for (uint32_t i = 0; i < listBox.Items().Size(); i++)
    {
      auto item = listBox.Items().GetAt(i).try_as<ListBoxItem>();
      if (item)
      {
        if (enable)
          winuiListSetItemDragSource(ih, item);
        else
          item.CanDrag(false);
      }
    }

    if (enable)
      winuiListSetupDragTracking(ih, listBox);
  }

  return 1;
}

static int winuiListMapMethod(Ihandle* ih)
{
  IupWinUIListAux* aux = new IupWinUIListAux();
  aux->isDropdown = ih->data->is_dropdown ? true : false;
  aux->hasEditbox = ih->data->has_editbox ? true : false;
  aux->isMultiple = ih->data->is_multiple ? true : false;

  if (aux->isDropdown)
  {
    ComboBox comboBox;
    comboBox.HorizontalAlignment(HorizontalAlignment::Left);
    comboBox.VerticalAlignment(VerticalAlignment::Top);

    if (aux->hasEditbox)
      comboBox.IsEditable(true);

    aux->selectionChangedToken = comboBox.SelectionChanged([ih](IInspectable const&, SelectionChangedEventArgs const&) {
      winuiListSelectionChanged(ih);
    });

    aux->dropDownOpenedToken = comboBox.DropDownOpened([ih](IInspectable const&, IInspectable const&) {
      winuiListDropDownOpened(ih, 1);
    });

    aux->dropDownClosedToken = comboBox.DropDownClosed([ih](IInspectable const&, IInspectable const&) {
      winuiListDropDownOpened(ih, 0);
    });

    aux->keyDownToken = comboBox.PreviewKeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    iupwinuiUpdateControlFont(ih, comboBox);

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(comboBox);

    winuiStoreHandle(ih, comboBox);
  }
  else if (aux->hasEditbox)
  {
    Grid grid;
    grid.HorizontalAlignment(HorizontalAlignment::Left);
    grid.VerticalAlignment(VerticalAlignment::Top);

    RowDefinition row0;
    row0.Height(GridLengthHelper::Auto());
    RowDefinition row1;
    row1.Height(GridLength{1, GridUnitType::Star});
    grid.RowDefinitions().Append(row0);
    grid.RowDefinitions().Append(row1);

    TextBox textBox;
    textBox.HorizontalAlignment(HorizontalAlignment::Stretch);
    iupwinuiUpdateControlFont(ih, textBox);
    Grid::SetRow(textBox, 0);
    grid.Children().Append(textBox);

    ListBox listBox;
    listBox.HorizontalAlignment(HorizontalAlignment::Stretch);
    listBox.VerticalAlignment(VerticalAlignment::Stretch);
    listBox.SelectionMode(SelectionMode::Single);
    if (ih->data->sb)
      ScrollViewer::SetVerticalScrollBarVisibility(listBox, ScrollBarVisibility::Visible);
    Grid::SetRow(listBox, 1);
    grid.Children().Append(listBox);

    aux->selectionChangedToken = listBox.SelectionChanged([ih](IInspectable const&, SelectionChangedEventArgs const&) {
      winuiListSelectionChanged(ih);
    });

    aux->doubleTappedToken = listBox.DoubleTapped([ih](IInspectable const&, Input::DoubleTappedRoutedEventArgs const&) {
      winuiListDoubleTapped(ih);
    });

    aux->textChangedToken = textBox.TextChanged([ih](IInspectable const&, TextChangedEventArgs const&) {
      if (iupAttribGet(ih, "_IUPWINUI_DISABLE_TEXT_CB"))
        return;
      IFnis cb = (IFnis)IupGetCallback(ih, "EDIT_CB");
      if (cb)
      {
        TextBox tb = winuiListGetTextBox(ih);
        if (tb)
        {
          hstring text = tb.Text();
          cb(ih, 0, iupwinuiHStringToString(text));
        }
      }
    });

    aux->keyDownToken = grid.PreviewKeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    void* lbPtr = nullptr;
    winrt::copy_to_abi(listBox, lbPtr);
    iupAttribSet(ih, "_IUPWINUI_LISTBOX", (char*)lbPtr);

    void* tbPtr = nullptr;
    winrt::copy_to_abi(textBox, tbPtr);
    iupAttribSet(ih, "_IUPWINUI_TEXTBOX", (char*)tbPtr);

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(grid);

    winuiStoreHandle(ih, grid);
  }
  else if (ih->data->is_virtual)
  {
    aux->isVirtual = true;

    ListView listView;
    listView.HorizontalAlignment(HorizontalAlignment::Left);
    listView.VerticalAlignment(VerticalAlignment::Top);

    if (aux->isMultiple)
      listView.SelectionMode(ListViewSelectionMode::Extended);
    else
      listView.SelectionMode(ListViewSelectionMode::Single);

    Style itemStyle{winrt::xaml_typename<ListViewItem>()};
    itemStyle.Setters().Append(Setter{Control::PaddingProperty(), box_value(Thickness{4, 2, 4, 2})});
    itemStyle.Setters().Append(Setter{FrameworkElement::MinHeightProperty(), box_value(0.0)});
    itemStyle.Setters().Append(Setter{Control::HorizontalContentAlignmentProperty(), box_value(HorizontalAlignment::Stretch)});
    listView.ItemContainerStyle(itemStyle);

    auto dt = XamlReader::Load(hstring(
      LR"(<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><ContentPresenter/></DataTemplate>)"
    )).as<DataTemplate>();
    listView.ItemTemplate(dt);
    listView.Height(1);

    if (ih->data->item_count > 0)
    {
      std::vector<IInspectable> items(ih->data->item_count);
      for (int i = 0; i < ih->data->item_count; i++)
        items[i] = box_value(i);
      listView.ItemsSource(winrt::single_threaded_vector<IInspectable>(std::move(items)));
    }

    aux->containerContentChangingToken = listView.ContainerContentChanging(
      [ih](ListViewBase const&, ContainerContentChangingEventArgs const& args) {
        if (args.InRecycleQueue())
          return;

        auto presenter = args.ItemContainer().ContentTemplateRoot().try_as<ContentPresenter>();
        if (!presenter)
          return;

        int pos = args.ItemIndex() + 1;
        char* text = iupListGetItemValueCb(ih, pos);

        if (ih->data->show_image)
        {
          char* image_name = iupListGetItemImageCb(ih, pos);
          if (image_name && *image_name)
          {
            void* imghandle = iupImageGetImage(image_name, ih, 0, NULL);
            WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
            if (bitmap)
            {
              int img_w = 0, img_h = 0;
              int draw_w, draw_h;
              winuiListGetScaledImageSize(ih, imghandle, &draw_w, &draw_h);
              iupdrvImageGetInfo(imghandle, &img_w, &img_h, NULL);

              StackPanel sp;
              sp.Orientation(Orientation::Horizontal);

              Image img;
              img.Source(bitmap);
              winuiListApplyImageSize(img, draw_w, draw_h, img_w, img_h);
              img.VerticalAlignment(VerticalAlignment::Center);
              sp.Children().Append(img);

              TextBlock tb;
              tb.Text(iupwinuiStringToHString(text ? text : ""));
              tb.Margin(ThicknessHelper::FromLengths(4, 0, 0, 0));
              tb.VerticalAlignment(VerticalAlignment::Center);
              winuiListUpdateTextBlockFont(ih, tb);
              sp.Children().Append(tb);

              presenter.Content(sp);
              args.Handled(true);
              return;
            }
          }
        }

        TextBlock tb;
        tb.Text(iupwinuiStringToHString(text ? text : ""));
        winuiListUpdateTextBlockFont(ih, tb);
        presenter.Content(tb);
        args.Handled(true);
      });

    aux->selectionChangedToken = listView.SelectionChanged([ih](IInspectable const&, SelectionChangedEventArgs const&) {
      winuiListSelectionChanged(ih);
    });

    aux->doubleTappedToken = listView.DoubleTapped([ih](IInspectable const&, DoubleTappedRoutedEventArgs const&) {
      winuiListDoubleTapped(ih);
    });

    aux->keyDownToken = listView.PreviewKeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(listView);

    winuiStoreHandle(ih, listView);
  }
  else
  {
    ListBox listBox;
    listBox.HorizontalAlignment(HorizontalAlignment::Left);
    listBox.VerticalAlignment(VerticalAlignment::Top);

    if (ih->data->sb)
      ScrollViewer::SetVerticalScrollBarVisibility(listBox, ScrollBarVisibility::Visible);

    if (aux->isMultiple)
      listBox.SelectionMode(SelectionMode::Extended);
    else
      listBox.SelectionMode(SelectionMode::Single);

    aux->selectionChangedToken = listBox.SelectionChanged([ih](IInspectable const&, SelectionChangedEventArgs const&) {
      winuiListSelectionChanged(ih);
    });

    aux->doubleTappedToken = listBox.DoubleTapped([ih](IInspectable const&, Input::DoubleTappedRoutedEventArgs const&) {
      winuiListDoubleTapped(ih);
    });

    aux->keyDownToken = listBox.PreviewKeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
      if (!iupwinuiKeyEvent(ih, (int)args.Key(), 1))
        args.Handled(true);
    });

    Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
    if (parentCanvas)
      parentCanvas.Children().Append(listBox);

    winuiStoreHandle(ih, listBox);
  }

  winuiSetAux(ih, IUPWINUI_LIST_AUX, aux);

  if (!aux->isDropdown && !aux->isVirtual)
    IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)winuiListConvertXYToPos);

  if (!aux->isVirtual)
    iupListSetInitialItems(ih);

  if (ih->data->show_dragdrop && !aux->isDropdown && !aux->isMultiple)
    winuiListEnableDragDrop(ih);

  return IUP_NOERROR;
}

static void winuiListUnMapMethod(Ihandle* ih)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);

  if (ih->handle && aux)
  {
    if (aux->isDropdown)
    {
      ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
      if (comboBox)
      {
        if (aux->selectionChangedToken)
          comboBox.SelectionChanged(aux->selectionChangedToken);
        if (aux->dropDownOpenedToken)
          comboBox.DropDownOpened(aux->dropDownOpenedToken);
        if (aux->dropDownClosedToken)
          comboBox.DropDownClosed(aux->dropDownClosedToken);
        if (aux->keyDownToken)
          comboBox.PreviewKeyDown(aux->keyDownToken);
      }
      winuiReleaseHandle<ComboBox>(ih);
    }
    else if (aux->hasEditbox)
    {
      ListBox listBox = winuiListGetListBox(ih);
      if (listBox)
      {
        if (aux->selectionChangedToken)
          listBox.SelectionChanged(aux->selectionChangedToken);
        if (aux->doubleTappedToken)
          listBox.DoubleTapped(aux->doubleTappedToken);
      }

      TextBox textBox = winuiListGetTextBox(ih);
      if (textBox)
      {
        if (aux->textChangedToken)
          textBox.TextChanged(aux->textChangedToken);
      }

      if (aux->keyDownToken)
      {
        Grid grid = winuiGetHandle<Grid>(ih);
        if (grid)
          grid.PreviewKeyDown(aux->keyDownToken);
      }

      iupAttribSet(ih, "_IUPWINUI_LISTBOX", nullptr);
      iupAttribSet(ih, "_IUPWINUI_TEXTBOX", nullptr);
      winuiReleaseHandle<Grid>(ih);
    }
    else if (aux->isVirtual)
    {
      ListView listView = winuiGetHandle<ListView>(ih);
      if (listView)
      {
        if (aux->containerContentChangingToken)
          listView.ContainerContentChanging(aux->containerContentChangingToken);
        if (aux->selectionChangedToken)
          listView.SelectionChanged(aux->selectionChangedToken);
        if (aux->doubleTappedToken)
          listView.DoubleTapped(aux->doubleTappedToken);
        if (aux->keyDownToken)
          listView.PreviewKeyDown(aux->keyDownToken);
        if (aux->dragItemsStartingToken)
          listView.DragItemsStarting(aux->dragItemsStartingToken);
        if (aux->dragItemsCompletedToken)
          listView.DragItemsCompleted(aux->dragItemsCompletedToken);
      }
      winuiReleaseHandle<ListView>(ih);
    }
    else
    {
      ListBox listBox = winuiGetHandle<ListBox>(ih);
      if (listBox)
      {
        if (aux->selectionChangedToken)
          listBox.SelectionChanged(aux->selectionChangedToken);
        if (aux->doubleTappedToken)
          listBox.DoubleTapped(aux->doubleTappedToken);
        if (aux->keyDownToken)
          listBox.PreviewKeyDown(aux->keyDownToken);
      }
      winuiReleaseHandle<ListBox>(ih);
    }
  }

  winuiFreeAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  ih->handle = NULL;
}

static int winuiListSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (aux && !aux->isDropdown)
  {
    if (aux->hasEditbox)
    {
      TextBox textBox = winuiListGetTextBox(ih);
      if (textBox)
        iupwinuiUpdateControlFont(ih, textBox);
    }

    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
    {
      for (uint32_t i = 0; i < listBox.Items().Size(); i++)
      {
        ListBoxItem item = listBox.Items().GetAt(i).try_as<ListBoxItem>();
        if (!item)
          continue;

        TextBlock tb = item.Content().try_as<TextBlock>();
        if (tb)
          winuiListUpdateTextBlockFont(ih, tb);
        else
        {
          StackPanel sp = item.Content().try_as<StackPanel>();
          if (sp)
          {
            for (uint32_t j = 0; j < sp.Children().Size(); j++)
            {
              TextBlock stb = sp.Children().GetAt(j).try_as<TextBlock>();
              if (stb)
                winuiListUpdateTextBlockFont(ih, stb);
            }
          }
        }
      }
    }
  }

  return 1;
}

extern "C" void iupdrvListAddBorders(Ihandle* ih, int* w, int* h)
{
  int border = 2;

  *w += 2 * border;

  if (ih->data->is_dropdown)
  {
    *w += 36;
    *h += 5 + 7 + 2;
  }
  else
  {
    *h += 2 * border;
    *w += 24;

    if (ih->data->has_editbox)
      *h += 2 * 3;
  }
}

extern "C" void iupdrvListAddItemSpace(Ihandle* ih, int* h)
{
  if (ih->data->is_dropdown)
    *h += 5 + 7;
  else
    *h += 2 + 2;
}

extern "C" int iupdrvListGetCount(Ihandle* ih)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux)
    return 0;

  if (aux->isVirtual)
    return ih->data->item_count;

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox)
      return (int)comboBox.Items().Size();
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
      return (int)listBox.Items().Size();
  }

  return 0;
}

extern "C" void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || aux->isVirtual)
    return;

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox)
      comboBox.Items().Append(box_value(iupwinuiStringToHString(value)));
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
      listBox.Items().Append(winuiListCreateItem(ih, value));
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
}

extern "C" void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || aux->isVirtual)
    return;

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox)
      comboBox.Items().InsertAt(pos, box_value(iupwinuiStringToHString(value)));
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
      listBox.Items().InsertAt(pos, winuiListCreateItem(ih, value));
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

  iupListUpdateOldValue(ih, pos, 0);
}

extern "C" void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || aux->isVirtual)
    return;

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox && pos >= 0 && (uint32_t)pos < comboBox.Items().Size())
      comboBox.Items().RemoveAt(pos);
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox && pos >= 0 && (uint32_t)pos < listBox.Items().Size())
      listBox.Items().RemoveAt(pos);
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);

  iupListUpdateOldValue(ih, pos, 1);
}

extern "C" void iupdrvListRemoveAllItems(Ihandle* ih)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || aux->isVirtual)
    return;

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", "1");

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox)
      comboBox.Items().Clear();
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox)
      listBox.Items().Clear();
  }

  iupAttribSet(ih, "_IUPLIST_IGNORE_ACTION", NULL);
}

extern "C" void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux)
    return NULL;

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox && id >= 0 && (uint32_t)id < comboBox.Items().Size())
    {
      StackPanel sp = comboBox.Items().GetAt(id).try_as<StackPanel>();
      if (sp && sp.Tag())
      {
        int64_t ptr = unbox_value_or<int64_t>(sp.Tag(), 0);
        if (ptr)
          return (void*)(intptr_t)ptr;
      }
    }
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox && id >= 0 && (uint32_t)id < listBox.Items().Size())
    {
      auto listItem = listBox.Items().GetAt(id).try_as<ListBoxItem>();
      if (listItem && listItem.Tag())
      {
        int64_t ptr = unbox_value_or<int64_t>(listItem.Tag(), 0);
        if (ptr)
          return (void*)(intptr_t)ptr;
      }
    }
  }

  return NULL;
}

extern "C" int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux)
    return 0;

  if (aux->isDropdown)
  {
    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox && id >= 0 && (uint32_t)id < comboBox.Items().Size())
    {
      winuiListSetComboItemImage(ih, comboBox, id, hImage);
      return 1;
    }
  }
  else
  {
    ListBox listBox = winuiListGetListBox(ih);
    if (listBox && id >= 0 && (uint32_t)id < listBox.Items().Size())
    {
      auto listItem = listBox.Items().GetAt(id).try_as<ListBoxItem>();
      if (listItem)
      {
        winuiListSetItemImage(ih, listItem, hImage);
        return 1;
      }
    }
  }

  return 0;
}

extern "C" void iupdrvListSetItemCount(Ihandle* ih, int count)
{
  IupWinUIListAux* aux = winuiGetAux<IupWinUIListAux>(ih, IUPWINUI_LIST_AUX);
  if (!aux || !aux->isVirtual)
    return;

  ListView listView = winuiListGetListView(ih);
  if (!listView)
    return;

  std::vector<IInspectable> items(count);
  for (int i = 0; i < count; i++)
    items[i] = box_value(i);
  listView.ItemsSource(winrt::single_threaded_vector<IInspectable>(std::move(items)));
}

static void winuiListLayoutUpdateMethod(Ihandle* ih)
{
  if (ih->data->is_dropdown)
  {
    int voptions = iupAttribGetInt(ih, "VISIBLEITEMS");
    if (voptions <= 0)
      voptions = 5;

    int charheight;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    iupdrvListAddItemSpace(ih, &charheight);

    ComboBox comboBox = winuiGetHandle<ComboBox>(ih);
    if (comboBox)
      comboBox.MaxDropDownHeight((double)((voptions + 1) * charheight));
  }

  iupdrvBaseLayoutUpdateMethod(ih);
}

extern "C" void iupdrvListInitClass(Iclass* ic)
{
  ic->Map = winuiListMapMethod;
  ic->UnMap = winuiListUnMapMethod;
  ic->LayoutUpdate = winuiListLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, winuiListSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "VALUE", winuiListGetValueAttrib, winuiListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IDVALUE", winuiListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, winuiListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, winuiListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", NULL, winuiListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", winuiListGetImageNativeHandleAttrib, NULL, IUPAF_NO_STRING|IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, winuiListSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
