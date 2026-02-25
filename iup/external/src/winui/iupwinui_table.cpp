/** \file
 * \brief WinUI Driver - Table Control
 *
 * Uses Grid as root container with a StackPanel header row and
 * a ListView for data rows. Each row is a Grid with per-column
 * TextBlocks wrapped in Borders (for grid lines and focus indicator).
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <vector>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_classbase.h"
#include "iup_tablecontrol.h"
#include "iup_image.h"
}

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Windows::Foundation;
using namespace Windows::UI;
using namespace Microsoft::UI::Xaml::Markup;


static void winuiTableApplyCellColors(Ihandle* ih, int lin, int col, Border border, TextBlock tb);

/****************************************************************************
 * Helper Functions
 ****************************************************************************/

static IupWinUITableAux* winuiTableGetAux(Ihandle* ih)
{
  return winuiGetAux<IupWinUITableAux>(ih, IUPWINUI_TABLE_AUX);
}

static ListView winuiTableGetListView(Ihandle* ih)
{
  char* ptr = iupAttribGet(ih, "_IUPWINUI_TABLE_LISTVIEW");
  if (ptr)
  {
    IInspectable obj{nullptr};
    winrt::copy_from_abi(obj, ptr);
    return obj.try_as<ListView>();
  }
  return nullptr;
}

static void winuiTableSetVirtualItems(ListView listView, int count)
{
  std::vector<IInspectable> items(count);
  for (int i = 0; i < count; i++)
    items[i] = box_value(i);
  listView.ItemsSource(winrt::single_threaded_vector<IInspectable>(std::move(items)));
}

static StackPanel winuiTableGetHeader(Ihandle* ih)
{
  char* ptr = iupAttribGet(ih, "_IUPWINUI_TABLE_HEADER");
  if (ptr)
  {
    IInspectable obj{nullptr};
    winrt::copy_from_abi(obj, ptr);
    return obj.try_as<StackPanel>();
  }
  return nullptr;
}

static Grid winuiTableGetRowGrid(Ihandle* ih, int lin)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  ListView listView = winuiTableGetListView(ih);
  if (!listView)
    return nullptr;

  uint32_t index = (uint32_t)(lin - 1);
  if (index >= listView.Items().Size())
    return nullptr;

  if (aux && aux->isVirtual)
  {
    auto container = listView.ContainerFromIndex(index);
    if (container)
    {
      auto lvi = container.try_as<ListViewItem>();
      if (lvi)
      {
        auto presenter = lvi.ContentTemplateRoot().try_as<ContentPresenter>();
        if (presenter)
          return presenter.Content().try_as<Grid>();
      }
    }
    return nullptr;
  }

  return listView.Items().GetAt(index).try_as<Grid>();
}

static Border winuiTableGetCellBorder(Grid rowGrid, int col)
{
  if (!rowGrid)
    return nullptr;

  for (uint32_t i = 0; i < rowGrid.Children().Size(); i++)
  {
    auto border = rowGrid.Children().GetAt(i).try_as<Border>();
    if (border && Grid::GetColumn(border) == col)
      return border;
  }
  return nullptr;
}

static TextBlock winuiTableGetCellTextBlock(Grid rowGrid, int col)
{
  Border border = winuiTableGetCellBorder(rowGrid, col);
  if (!border)
    return nullptr;

  auto child = border.Child();
  if (!child)
    return nullptr;

  auto tb = child.try_as<TextBlock>();
  if (tb)
    return tb;

  auto panel = child.try_as<StackPanel>();
  if (panel)
  {
    for (uint32_t i = 0; i < panel.Children().Size(); i++)
    {
      tb = panel.Children().GetAt(i).try_as<TextBlock>();
      if (tb)
        return tb;
    }
  }
  return nullptr;
}

static Controls::Image winuiTableGetCellImage(Grid rowGrid, int col)
{
  Border border = winuiTableGetCellBorder(rowGrid, col);
  if (!border)
    return nullptr;

  auto panel = border.Child().try_as<StackPanel>();
  if (panel)
  {
    for (uint32_t i = 0; i < panel.Children().Size(); i++)
    {
      auto img = panel.Children().GetAt(i).try_as<Controls::Image>();
      if (img)
        return img;
    }
  }
  return nullptr;
}

static void winuiTableFreeCell(char** cell)
{
  if (cell && *cell)
  {
    free(*cell);
    *cell = NULL;
  }
}

static void winuiTableSetCell(char** cell, const char* value)
{
  if (*cell)
    free(*cell);

  if (value && value[0])
    *cell = iupStrDup(value);
  else
    *cell = NULL;
}

static SolidColorBrush winuiTableGridLineBrush()
{
  if (iupwinuiIsSystemDarkMode())
    return SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 60, 60, 60));
  else
    return SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 200, 200, 200));
}

static SolidColorBrush winuiTableHeaderBgBrush()
{
  unsigned char r, g, b;
  const char* bgcolor = IupGetGlobal("DLGBGCOLOR");
  if (iupStrToRGB(bgcolor, &r, &g, &b))
  {
    if (iupwinuiIsSystemDarkMode())
    {
      r = (r + 20 > 255) ? 255 : r + 20;
      g = (g + 20 > 255) ? 255 : g + 20;
      b = (b + 20 > 255) ? 255 : b + 20;
    }
    else
    {
      r = (r > 20) ? r - 20 : 0;
      g = (g > 20) ? g - 20 : 0;
      b = (b > 20) ? b - 20 : 0;
    }
    return SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, r, g, b));
  }
  return SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 200, 200, 200));
}

static SolidColorBrush winuiTableFocusBrush()
{
  using namespace Windows::UI::ViewManagement;
  UISettings settings;
  auto fg = settings.GetColorValue(UIColorType::Foreground);
  return SolidColorBrush(ColorHelper::FromArgb(255, fg.R, fg.G, fg.B));
}

static SolidColorBrush winuiTableSelectionBrush()
{
  using namespace Windows::UI::ViewManagement;
  UISettings settings;
  auto accent = settings.GetColorValue(UIColorType::Accent);
  return SolidColorBrush(ColorHelper::FromArgb(140, accent.R, accent.G, accent.B));
}

static bool winuiTableIsRowSelected(Ihandle* ih, int lin)
{
  ListView listView = winuiTableGetListView(ih);
  if (!listView)
    return false;

  if (listView.SelectionMode() == ListViewSelectionMode::None)
    return false;

  if (listView.SelectionMode() == ListViewSelectionMode::Single)
    return listView.SelectedIndex() == (int)(lin - 1);

  uint32_t index = (uint32_t)(lin - 1);
  if (index >= listView.Items().Size())
    return false;

  auto item = listView.Items().GetAt(index);
  uint32_t sel_idx;
  return listView.SelectedItems().IndexOf(item, sel_idx);
}

static void winuiTableUpdateRowColors(Ihandle* ih, int lin)
{
  Grid rowGrid = winuiTableGetRowGrid(ih, lin);
  if (!rowGrid)
    return;

  int num_col = ih->data->num_col;
  for (int j = 0; j < num_col; j++)
  {
    Border border = winuiTableGetCellBorder(rowGrid, j);
    TextBlock tb = winuiTableGetCellTextBlock(rowGrid, j);
    if (border && tb)
      winuiTableApplyCellColors(ih, lin, j + 1, border, tb);
  }
}

static void winuiTableUpdateCellFont(Ihandle* ih, TextBlock tb)
{
  iupwinuiUpdateTextBlockFont(ih, tb);
}

static TextAlignment winuiTableGetColumnAlignment(Ihandle* ih, int col)
{
  char name[50];
  sprintf(name, "ALIGNMENT%d", col);
  char* align_str = iupAttribGet(ih, name);
  if (align_str)
  {
    if (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT"))
      return TextAlignment::Right;
    if (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER"))
      return TextAlignment::Center;
  }
  return TextAlignment::Left;
}

/****************************************************************************
 * Row and Header Creation
 ****************************************************************************/

static Grid winuiTableCreateRowGrid(Ihandle* ih, int num_col, bool show_grid)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  Grid grid;

  int row_height = iupdrvTableGetRowHeight(ih);
  grid.Height((double)row_height);

  for (int i = 0; i < num_col; i++)
  {
    ColumnDefinition colDef;
    int width = (aux && aux->col_widths) ? aux->col_widths[i] : 100;
    colDef.Width(GridLength{(double)width, GridUnitType::Pixel});
    grid.ColumnDefinitions().Append(colDef);
  }

  for (int i = 0; i < num_col; i++)
  {
    Border border;
    border.Padding(ThicknessHelper::FromLengths(4, 2, 4, 2));
    Grid::SetColumn(border, i);

    if (show_grid)
      border.BorderThickness(ThicknessHelper::FromLengths(0, 0, 1, 1));
    else
      border.BorderThickness(ThicknessHelper::FromLengths(0, 0, 0, 0));

    border.BorderBrush(winuiTableGridLineBrush());

    TextBlock tb;
    tb.TextTrimming(TextTrimming::CharacterEllipsis);
    winuiTableUpdateCellFont(ih, tb);

    if (ih->data->show_image)
    {
      StackPanel panel;
      panel.Orientation(Orientation::Horizontal);
      panel.Spacing(4);

      Controls::Image img;
      img.Stretch(Stretch::Uniform);
      img.VerticalAlignment(VerticalAlignment::Center);
      img.Visibility(Visibility::Collapsed);
      panel.Children().Append(img);

      tb.VerticalAlignment(VerticalAlignment::Center);
      panel.Children().Append(tb);

      border.Child(panel);
    }
    else
    {
      border.Child(tb);
    }

    grid.Children().Append(border);
  }

  return grid;
}

static void winuiTableUpdateHeaderCell(Ihandle* ih, Border headerBorder, int col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
    return;

  int width = aux->col_widths[col];
  headerBorder.Width((double)width);

  auto tb = headerBorder.Child().try_as<TextBlock>();
  if (tb)
  {
    if (aux->col_titles && aux->col_titles[col])
      tb.Text(iupwinuiStringToHString(aux->col_titles[col]));
    tb.TextAlignment(winuiTableGetColumnAlignment(ih, col + 1));
  }
}

static Border winuiTableCreateHeaderCell(Ihandle* ih, int col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  Border border;

  int width = (aux && aux->col_widths) ? aux->col_widths[col] : 100;
  border.Width((double)width);
  border.Height((double)iupdrvTableGetHeaderHeight(ih));
  border.Padding(ThicknessHelper::FromLengths(4, 4, 4, 4));
  border.BorderThickness(ThicknessHelper::FromLengths(0, 0, 1, 1));
  border.BorderBrush(winuiTableGridLineBrush());
  border.Background(winuiTableHeaderBgBrush());

  TextBlock tb;
  tb.FontWeight(Windows::UI::Text::FontWeights::SemiBold());
  tb.TextTrimming(TextTrimming::CharacterEllipsis);
  winuiTableUpdateCellFont(ih, tb);

  if (aux && aux->col_titles && aux->col_titles[col])
    tb.Text(iupwinuiStringToHString(aux->col_titles[col]));

  border.Child(tb);

  border.Tag(box_value(col));

  return border;
}

/****************************************************************************
 * Cell Image Support
 ****************************************************************************/

static void winuiTableSetCellImageFromName(Ihandle* ih, Grid rowGrid, int col, const char* image_name)
{
  Controls::Image img = winuiTableGetCellImage(rowGrid, col);
  if (!img)
    return;

  if (!image_name)
  {
    img.Source(nullptr);
    img.Visibility(Visibility::Collapsed);
    return;
  }

  void* imghandle = iupImageGetImage(image_name, ih, 0, NULL);
  WriteableBitmap bitmap = winuiGetBitmapFromHandle(imghandle);
  if (!bitmap)
  {
    img.Source(nullptr);
    img.Visibility(Visibility::Collapsed);
    return;
  }

  if (ih->data->fit_image)
  {
    int row_height = iupdrvTableGetRowHeight(ih);
    int available_height = row_height - 5;
    int img_w = bitmap.PixelWidth();
    int img_h = bitmap.PixelHeight();
    if (img_h > available_height && available_height > 0)
    {
      int scaled_width = (img_w * available_height) / img_h;
      img.Width((double)scaled_width);
      img.Height((double)available_height);
    }
    else
    {
      img.Width((double)img_w);
      img.Height((double)img_h);
    }
  }

  img.Source(bitmap);
  img.Visibility(Visibility::Visible);
}

static void winuiTablePopulateCellImages(Ihandle* ih, int lin, Grid rowGrid)
{
  if (!ih->data->show_image)
    return;

  for (int col = 1; col <= ih->data->num_col; col++)
  {
    char* image_name = iupTableGetCellImageCb(ih, lin, col);
    if (!image_name)
      image_name = iupAttribGetId2(ih, "_IUPWINUI_CELLIMAGE", lin, col);

    winuiTableSetCellImageFromName(ih, rowGrid, col - 1, image_name);
  }
}

/****************************************************************************
 * Virtual Mode Support
 ****************************************************************************/

static void winuiTablePopulateVirtualRow(Ihandle* ih, int lin, Grid rowGrid)
{
  if (!rowGrid)
    return;

  sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
  if (!value_cb)
    return;

  for (int col = 1; col <= ih->data->num_col; col++)
  {
    TextBlock tb = winuiTableGetCellTextBlock(rowGrid, col - 1);
    if (tb)
    {
      char* value = value_cb(ih, lin, col);
      tb.Text(iupwinuiStringToHString(value ? value : ""));
    }
  }

  winuiTablePopulateCellImages(ih, lin, rowGrid);
}

static void winuiTablePopulateVirtualContainer(Ihandle* ih, int lin, Primitives::SelectorItem lvi)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
    return;

  auto presenter = lvi.ContentTemplateRoot().try_as<ContentPresenter>();
  if (!presenter)
    return;

  int num_col = ih->data->num_col;

  Grid rowGrid = presenter.Content().try_as<Grid>();
  if (!rowGrid || (int)rowGrid.ColumnDefinitions().Size() != num_col)
  {
    rowGrid = winuiTableCreateRowGrid(ih, num_col, aux->show_grid);
    presenter.Content(rowGrid);
  }

  for (int c = 0; c < num_col && (uint32_t)c < rowGrid.ColumnDefinitions().Size(); c++)
    rowGrid.ColumnDefinitions().GetAt(c).Width(GridLength{(double)aux->col_widths[c], GridUnitType::Pixel});

  winuiTablePopulateVirtualRow(ih, lin, rowGrid);

  for (int j = 0; j < num_col; j++)
  {
    Border border = winuiTableGetCellBorder(rowGrid, j);
    TextBlock tb = winuiTableGetCellTextBlock(rowGrid, j);

    if (border && tb)
      winuiTableApplyCellColors(ih, lin, j + 1, border, tb);

    if (tb)
      tb.TextAlignment(winuiTableGetColumnAlignment(ih, j + 1));

    if (border)
    {
      bool is_focused = (lin == aux->current_row && j + 1 == aux->current_col && iupAttribGetBoolean(ih, "FOCUSRECT"));
      if (is_focused)
      {
        border.BorderThickness(ThicknessHelper::FromLengths(1, 1, 1, 1));
        border.BorderBrush(winuiTableFocusBrush());
        if (aux->show_grid)
          border.Padding(ThicknessHelper::FromLengths(3, 1, 4, 2));
        else
          border.Padding(ThicknessHelper::FromLengths(3, 1, 3, 1));
      }
      else
      {
        if (aux->show_grid)
          border.BorderThickness(ThicknessHelper::FromLengths(0, 0, 1, 1));
        else
          border.BorderThickness(ThicknessHelper::FromLengths(0, 0, 0, 0));
        border.Padding(ThicknessHelper::FromLengths(4, 2, 4, 2));
        border.BorderBrush(winuiTableGridLineBrush());
      }
    }
  }
}

/****************************************************************************
 * Column Width Adjustment
 ****************************************************************************/

static void winuiTableApplyColumnWidthsToRowGrid(Grid rowGrid, IupWinUITableAux* aux, int num_col)
{
  for (int c = 0; c < num_col && (uint32_t)c < rowGrid.ColumnDefinitions().Size(); c++)
    rowGrid.ColumnDefinitions().GetAt(c).Width(GridLength{(double)aux->col_widths[c], GridUnitType::Pixel});
}

static int winuiTableCalculateColumnWidth(Ihandle* ih, int col_index)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  int max_width = 0;
  int max_rows_to_check = (ih->data->num_lin > 100) ? 100 : ih->data->num_lin;
  int iup_col = col_index + 1;
  int charheight = 0;
  int image_extra = 0;

  if (ih->data->show_image)
  {
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    image_extra = charheight + 4;
  }

  /* Measure column title */
  if (aux->col_titles && aux->col_titles[col_index])
  {
    int title_width = iupdrvFontGetStringWidth(ih, aux->col_titles[col_index]);
    title_width += 20;
    if (title_width > max_width)
      max_width = title_width;
  }

  /* Measure cell content */
  for (int lin = 1; lin <= max_rows_to_check; lin++)
  {
    int cell_width = 0;
    char* value = iupdrvTableGetCellValue(ih, lin, iup_col);
    if (value && *value)
      cell_width = iupdrvFontGetStringWidth(ih, value);
    cell_width += 16;

    if (ih->data->show_image)
    {
      char* image_name = iupAttribGetId2(ih, "_IUPWINUI_CELLIMAGE", lin, iup_col);
      if (!image_name)
        image_name = iupTableGetCellImageCb(ih, lin, iup_col);
      if (image_name)
        cell_width += image_extra;
    }

    if (cell_width > max_width)
      max_width = cell_width;
  }

  if (max_width < 50)
    max_width = 50;

  return max_width;
}

static void winuiTableAdjustColumnWidths(Ihandle* ih)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
    return;

  int num_col = ih->data->num_col;
  if (num_col <= 0)
    return;

  double available_width = (double)ih->currentwidth;
  if (available_width <= 0)
    return;

  int used_width = 0;
  for (int i = 0; i < num_col - 1; i++)
  {
    if (!aux->col_width_set[i])
      aux->col_widths[i] = winuiTableCalculateColumnWidth(ih, i);
    used_width += aux->col_widths[i];
  }

  int last_col = num_col - 1;
  if (!aux->col_width_set[last_col])
  {
    int content_width = winuiTableCalculateColumnWidth(ih, last_col);
    if (ih->data->stretch_last)
    {
      int remaining = (int)available_width - used_width;
      if (remaining > content_width)
        aux->col_widths[last_col] = remaining;
      else
        aux->col_widths[last_col] = content_width;
    }
    else
    {
      aux->col_widths[last_col] = content_width;
    }
  }

  StackPanel header = winuiTableGetHeader(ih);
  if (header)
  {
    for (uint32_t i = 0; i < header.Children().Size() && (int)i < num_col; i++)
    {
      auto border = header.Children().GetAt(i).try_as<Border>();
      if (border)
        border.Width((double)aux->col_widths[i]);
    }
  }

  ListView listView = winuiTableGetListView(ih);
  if (!listView)
    return;

  if (aux->isVirtual)
  {
    for (int r = 0; r < ih->data->num_lin; r++)
    {
      auto container = listView.ContainerFromIndex(r);
      if (!container)
        continue;
      auto lvi = container.try_as<ListViewItem>();
      if (!lvi)
        continue;
      auto presenter = lvi.ContentTemplateRoot().try_as<ContentPresenter>();
      if (!presenter)
        continue;
      Grid rowGrid = presenter.Content().try_as<Grid>();
      if (rowGrid)
        winuiTableApplyColumnWidthsToRowGrid(rowGrid, aux, num_col);
    }
  }
  else
  {
    for (uint32_t r = 0; r < listView.Items().Size(); r++)
    {
      auto rowGrid = listView.Items().GetAt(r).try_as<Grid>();
      if (rowGrid)
        winuiTableApplyColumnWidthsToRowGrid(rowGrid, aux, num_col);
    }
  }
}

/****************************************************************************
 * Sorting
 ****************************************************************************/

static void winuiTableUpdateSortArrow(Ihandle* ih, int col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  StackPanel header = winuiTableGetHeader(ih);
  if (!aux || !header)
    return;

  for (uint32_t i = 0; i < header.Children().Size() && (int)i < ih->data->num_col; i++)
  {
    auto border = header.Children().GetAt(i).try_as<Border>();
    if (!border)
      continue;
    auto tb = border.Child().try_as<TextBlock>();
    if (!tb)
      continue;

    const char* title = (aux->col_titles && aux->col_titles[i]) ? aux->col_titles[i] : "";

    if ((int)i == col - 1)
    {
      hstring arrow = (aux->sort_ascending == 1) ? L" \u25B2" : L" \u25BC";
      tb.Text(iupwinuiStringToHString(title) + arrow);
    }
    else
    {
      tb.Text(iupwinuiStringToHString(title));
    }
  }
}

static void winuiTableSortRows(Ihandle* ih, int col, int ascending)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  int num_rows = ih->data->num_lin;
  int num_cols = ih->data->num_col;

  if (!aux->cell_values || num_rows < 2 || col < 1 || col > num_cols)
    return;

  for (int i = 0; i < num_rows - 1; i++)
  {
    for (int j = 0; j < num_rows - i - 1; j++)
    {
      const char* val1 = aux->cell_values[j][col - 1];
      const char* val2 = aux->cell_values[j + 1][col - 1];

      if (!val1) val1 = "";
      if (!val2) val2 = "";

      int cmp = strcmp(val1, val2);
      int should_swap = ascending ? (cmp > 0) : (cmp < 0);

      if (should_swap)
      {
        char** temp_row = aux->cell_values[j];
        aux->cell_values[j] = aux->cell_values[j + 1];
        aux->cell_values[j + 1] = temp_row;
      }
    }
  }
}

static void winuiTableRebuildListViewItems(Ihandle* ih)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  ListView listView = winuiTableGetListView(ih);
  if (!aux || !listView)
    return;

  if (aux->isVirtual)
  {
    winuiTableSetVirtualItems(listView, ih->data->num_lin);
    return;
  }

  listView.Items().Clear();

  for (int i = 0; i < ih->data->num_lin; i++)
  {
    Grid rowGrid = winuiTableCreateRowGrid(ih, ih->data->num_col, aux->show_grid);

    for (int j = 0; j < ih->data->num_col; j++)
    {
      Border border = winuiTableGetCellBorder(rowGrid, j);
      TextBlock tb = winuiTableGetCellTextBlock(rowGrid, j);
      if (tb)
      {
        const char* value = NULL;
        if (aux->cell_values && aux->cell_values[i])
          value = aux->cell_values[i][j];
        if (value)
          tb.Text(iupwinuiStringToHString(value));
        tb.TextAlignment(winuiTableGetColumnAlignment(ih, j + 1));
      }
      if (border && tb)
        winuiTableApplyCellColors(ih, i + 1, j + 1, border, tb);
    }

    winuiTablePopulateCellImages(ih, i + 1, rowGrid);

    listView.Items().Append(rowGrid);
  }
}

static void winuiTableSort(Ihandle* ih, int col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux || col < 1 || col > ih->data->num_col)
    return;

  if (aux->sort_column == col)
  {
    if (aux->sort_ascending == 1)
      aux->sort_ascending = -1;
    else
      aux->sort_ascending = 1;
  }
  else
  {
    aux->sort_column = col;
    aux->sort_ascending = 1;
  }

  winuiTableUpdateSortArrow(ih, col);

  IFni sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
  int sort_result = IUP_DEFAULT;

  if (sort_cb)
    sort_result = sort_cb(ih, col);

  if (sort_result == IUP_DEFAULT)
  {
    char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
    if (!iupStrBoolean(virtualmode))
    {
      winuiTableSortRows(ih, col, (aux->sort_ascending == 1));
      winuiTableRebuildListViewItems(ih);
    }
  }
}

/****************************************************************************
 * Column Reorder
 ****************************************************************************/

static void winuiTableShiftAttrib(Ihandle* ih, const char* fmt, int source, int target)
{
  char src_name[50], dst_name[50];
  sprintf(src_name, fmt, source);
  char* saved = iupAttribGet(ih, src_name);
  saved = saved ? iupStrDup(saved) : NULL;

  if (source < target)
  {
    for (int i = source; i < target; i++)
    {
      char from[50], to[50];
      sprintf(from, fmt, i + 1);
      sprintf(to, fmt, i);
      char* val = iupAttribGet(ih, from);
      if (val)
        iupAttribSetStr(ih, to, val);
      else
        iupAttribSet(ih, to, NULL);
    }
  }
  else
  {
    for (int i = source; i > target; i--)
    {
      char from[50], to[50];
      sprintf(from, fmt, i - 1);
      sprintf(to, fmt, i);
      char* val = iupAttribGet(ih, from);
      if (val)
        iupAttribSetStr(ih, to, val);
      else
        iupAttribSet(ih, to, NULL);
    }
  }

  sprintf(dst_name, fmt, target);
  if (saved)
  {
    iupAttribSetStr(ih, dst_name, saved);
    free(saved);
  }
  else
    iupAttribSet(ih, dst_name, NULL);
}

static void winuiTableShiftAttribLinCol(Ihandle* ih, const char* fmt, int lin, int source, int target)
{
  char src_name[50], dst_name[50];
  sprintf(src_name, fmt, lin, source);
  char* saved = iupAttribGet(ih, src_name);
  saved = saved ? iupStrDup(saved) : NULL;

  if (source < target)
  {
    for (int i = source; i < target; i++)
    {
      char from[50], to[50];
      sprintf(from, fmt, lin, i + 1);
      sprintf(to, fmt, lin, i);
      char* val = iupAttribGet(ih, from);
      if (val)
        iupAttribSetStr(ih, to, val);
      else
        iupAttribSet(ih, to, NULL);
    }
  }
  else
  {
    for (int i = source; i > target; i--)
    {
      char from[50], to[50];
      sprintf(from, fmt, lin, i - 1);
      sprintf(to, fmt, lin, i);
      char* val = iupAttribGet(ih, from);
      if (val)
        iupAttribSetStr(ih, to, val);
      else
        iupAttribSet(ih, to, NULL);
    }
  }

  sprintf(dst_name, fmt, lin, target);
  if (saved)
  {
    iupAttribSetStr(ih, dst_name, saved);
    free(saved);
  }
  else
    iupAttribSet(ih, dst_name, NULL);
}

static void winuiTableMoveColumn(Ihandle* ih, int source, int target)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux || source == target)
    return;

  int src_idx = source - 1;
  int tgt_idx = target - 1;
  int num_lin = ih->data->num_lin;

  int saved_width = aux->col_widths[src_idx];
  bool saved_width_set = aux->col_width_set[src_idx];
  char* saved_title = aux->col_titles[src_idx];

  if (source < target)
  {
    memmove(&aux->col_widths[src_idx], &aux->col_widths[src_idx + 1], (tgt_idx - src_idx) * sizeof(int));
    memmove(&aux->col_width_set[src_idx], &aux->col_width_set[src_idx + 1], (tgt_idx - src_idx) * sizeof(bool));
    memmove(&aux->col_titles[src_idx], &aux->col_titles[src_idx + 1], (tgt_idx - src_idx) * sizeof(char*));
  }
  else
  {
    memmove(&aux->col_widths[tgt_idx + 1], &aux->col_widths[tgt_idx], (src_idx - tgt_idx) * sizeof(int));
    memmove(&aux->col_width_set[tgt_idx + 1], &aux->col_width_set[tgt_idx], (src_idx - tgt_idx) * sizeof(bool));
    memmove(&aux->col_titles[tgt_idx + 1], &aux->col_titles[tgt_idx], (src_idx - tgt_idx) * sizeof(char*));
  }
  aux->col_widths[tgt_idx] = saved_width;
  aux->col_width_set[tgt_idx] = saved_width_set;
  aux->col_titles[tgt_idx] = saved_title;

  if (aux->cell_values)
  {
    for (int lin = 0; lin < num_lin; lin++)
    {
      if (!aux->cell_values[lin])
        continue;
      char* saved_cell = aux->cell_values[lin][src_idx];
      if (source < target)
        memmove(&aux->cell_values[lin][src_idx], &aux->cell_values[lin][src_idx + 1], (tgt_idx - src_idx) * sizeof(char*));
      else
        memmove(&aux->cell_values[lin][tgt_idx + 1], &aux->cell_values[lin][tgt_idx], (src_idx - tgt_idx) * sizeof(char*));
      aux->cell_values[lin][tgt_idx] = saved_cell;
    }
  }

  winuiTableShiftAttrib(ih, "ALIGNMENT%d", source, target);
  winuiTableShiftAttribLinCol(ih, "%d:%d", 0, source, target);
  winuiTableShiftAttribLinCol(ih, "BGCOLOR%d:%d", 0, source, target);
  winuiTableShiftAttribLinCol(ih, "FGCOLOR%d:%d", 0, source, target);
  winuiTableShiftAttribLinCol(ih, "FONT%d:%d", 0, source, target);

  for (int lin = 1; lin <= num_lin; lin++)
  {
    winuiTableShiftAttribLinCol(ih, "CELLVALUE%d:%d", lin, source, target);
    winuiTableShiftAttribLinCol(ih, "BGCOLOR%d:%d", lin, source, target);
    winuiTableShiftAttribLinCol(ih, "FGCOLOR%d:%d", lin, source, target);
    winuiTableShiftAttribLinCol(ih, "FONT%d:%d", lin, source, target);
  }

  if (aux->sort_column == source)
    aux->sort_column = target;
  else if (source < target && aux->sort_column > source && aux->sort_column <= target)
    aux->sort_column--;
  else if (source > target && aux->sort_column >= target && aux->sort_column < source)
    aux->sort_column++;

  if (aux->current_col == source)
    aux->current_col = target;
  else if (source < target && aux->current_col > source && aux->current_col <= target)
    aux->current_col--;
  else if (source > target && aux->current_col >= target && aux->current_col < source)
    aux->current_col++;
}

static void winuiTableRefreshAfterReorder(Ihandle* ih)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
    return;

  StackPanel header = winuiTableGetHeader(ih);
  if (header)
  {
    for (uint32_t i = 0; i < header.Children().Size() && (int)i < ih->data->num_col; i++)
    {
      auto border = header.Children().GetAt(i).try_as<Border>();
      if (border)
      {
        winuiTableUpdateHeaderCell(ih, border, (int)i);
        border.Tag(box_value((int)i));
      }
    }
  }

  if (aux->sort_column > 0)
    winuiTableUpdateSortArrow(ih, aux->sort_column);

  winuiTableRebuildListViewItems(ih);
  winuiTableAdjustColumnWidths(ih);
}

static Border winuiTableGetDragIndicator(Ihandle* ih)
{
  char* ptr = iupAttribGet(ih, "_IUPWINUI_TABLE_DRAG_INDICATOR");
  if (ptr)
  {
    IInspectable obj{nullptr};
    winrt::copy_from_abi(obj, ptr);
    return obj.try_as<Border>();
  }
  return nullptr;
}

static Border winuiTableGetOrCreateDragIndicator(Ihandle* ih)
{
  Border indicator = winuiTableGetDragIndicator(ih);
  if (indicator)
    return indicator;

  indicator = Border();
  indicator.Width(2);
  indicator.Background(SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 0, 120, 215)));
  indicator.HorizontalAlignment(HorizontalAlignment::Left);
  indicator.VerticalAlignment(VerticalAlignment::Stretch);
  indicator.Visibility(Visibility::Collapsed);

  Grid containerGrid = winuiGetHandle<Grid>(ih);
  if (containerGrid)
  {
    Grid::SetRow(indicator, 0);
    containerGrid.Children().Append(indicator);
  }

  void* indicatorPtr = nullptr;
  winrt::copy_to_abi(indicator, indicatorPtr);
  iupAttribSet(ih, "_IUPWINUI_TABLE_DRAG_INDICATOR", (char*)indicatorPtr);

  return indicator;
}

static void winuiTableHideDragIndicator(Ihandle* ih)
{
  Border indicator = winuiTableGetDragIndicator(ih);
  if (indicator)
    indicator.Visibility(Visibility::Collapsed);
}

static int winuiTableFindColumnAtPoint(Ihandle* ih, double pointer_x)
{
  StackPanel header = winuiTableGetHeader(ih);
  if (!header)
    return -1;

  int num_col = ih->data->num_col;

  for (uint32_t i = 0; i < header.Children().Size() && (int)i < num_col; i++)
  {
    auto border = header.Children().GetAt(i).try_as<Border>();
    if (!border)
      continue;

    auto transform = border.TransformToVisual(header);
    auto pt = transform.TransformPoint(Point{0, 0});
    double col_left = pt.X;
    double col_width = border.ActualWidth();

    if (pointer_x >= col_left && pointer_x < col_left + col_width)
      return (int)i + 1;
  }

  return num_col;
}

static int winuiTableFindTargetColumn(Ihandle* ih, double pointer_x)
{
  StackPanel header = winuiTableGetHeader(ih);
  if (!header)
    return -1;

  int num_col = ih->data->num_col;

  for (uint32_t i = 0; i < header.Children().Size() && (int)i < num_col; i++)
  {
    auto border = header.Children().GetAt(i).try_as<Border>();
    if (!border)
      continue;

    auto transform = border.TransformToVisual(header);
    auto pt = transform.TransformPoint(Point{0, 0});
    double col_left = pt.X;
    double col_width = border.ActualWidth();
    double mid_x = col_left + col_width / 2.0;

    if (pointer_x < mid_x)
      return (int)i + 1;
  }

  return num_col;
}

static void winuiTableUpdateDragIndicator(Ihandle* ih, int target)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
    return;

  int source = aux->drag_source_col;

  if (target == source || target < 1 || target > ih->data->num_col)
  {
    winuiTableHideDragIndicator(ih);
    return;
  }

  StackPanel header = winuiTableGetHeader(ih);
  Grid containerGrid = winuiGetHandle<Grid>(ih);
  if (!header || !containerGrid)
    return;

  auto targetBorder = header.Children().GetAt(target - 1).try_as<Border>();
  if (!targetBorder)
    return;

  auto transform = targetBorder.TransformToVisual(containerGrid);
  auto pt = transform.TransformPoint(Point{0, 0});

  double indicator_x;
  if (source < target)
    indicator_x = pt.X + targetBorder.ActualWidth() - 1;
  else
    indicator_x = pt.X;

  Border indicator = winuiTableGetOrCreateDragIndicator(ih);
  indicator.Margin(ThicknessHelper::FromLengths(indicator_x, 0, 0, 0));
  indicator.Visibility(Visibility::Visible);
}

static int winuiTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->allow_reorder = 1;
  else
    ih->data->allow_reorder = 0;

  return 0;
}

/****************************************************************************
 * Column Resize
 ****************************************************************************/

static int winuiTableFindDividerColumn(Ihandle* ih, double pointer_x)
{
  StackPanel header = winuiTableGetHeader(ih);
  if (!header)
    return 0;

  int num_col = ih->data->num_col;

  for (uint32_t i = 0; i < header.Children().Size() && (int)i < num_col; i++)
  {
    auto border = header.Children().GetAt(i).try_as<Border>();
    if (!border)
      continue;

    auto transform = border.TransformToVisual(header);
    auto pt = transform.TransformPoint(Point{0, 0});
    double col_right = pt.X + border.ActualWidth();

    if (pointer_x >= col_right - 4 && pointer_x <= col_right + 4)
      return (int)i + 1;
  }

  return 0;
}

static Border winuiTableGetResizeIndicator(Ihandle* ih)
{
  char* ptr = iupAttribGet(ih, "_IUPWINUI_TABLE_RESIZE_INDICATOR");
  if (ptr)
  {
    IInspectable obj{nullptr};
    winrt::copy_from_abi(obj, ptr);
    return obj.try_as<Border>();
  }
  return nullptr;
}

static Border winuiTableGetOrCreateResizeIndicator(Ihandle* ih)
{
  Border indicator = winuiTableGetResizeIndicator(ih);
  if (indicator)
    return indicator;

  indicator = Border();
  indicator.Width(2);
  indicator.Background(SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 0, 120, 215)));
  indicator.HorizontalAlignment(HorizontalAlignment::Left);
  indicator.VerticalAlignment(VerticalAlignment::Stretch);
  indicator.Visibility(Visibility::Collapsed);

  Grid containerGrid = winuiGetHandle<Grid>(ih);
  if (containerGrid)
  {
    Grid::SetRow(indicator, 0);
    Grid::SetRowSpan(indicator, 2);
    containerGrid.Children().Append(indicator);
  }

  void* indicatorPtr = nullptr;
  winrt::copy_to_abi(indicator, indicatorPtr);
  iupAttribSet(ih, "_IUPWINUI_TABLE_RESIZE_INDICATOR", (char*)indicatorPtr);

  return indicator;
}

static void winuiTableHideResizeIndicator(Ihandle* ih)
{
  Border indicator = winuiTableGetResizeIndicator(ih);
  if (indicator)
    indicator.Visibility(Visibility::Collapsed);
}

static void winuiTableUpdateResizeIndicator(Ihandle* ih, double indicator_x)
{
  Border indicator = winuiTableGetOrCreateResizeIndicator(ih);
  indicator.Margin(ThicknessHelper::FromLengths(indicator_x, 0, 0, 0));
  indicator.Visibility(Visibility::Visible);
}

static void winuiTableSetHeaderCursor(Ihandle* ih, bool resize_cursor)
{
  using namespace Microsoft::UI::Input;

  StackPanel header = winuiTableGetHeader(ih);
  if (!header)
    return;

  auto protectedUI = header.try_as<Microsoft::UI::Xaml::IUIElementProtected>();
  if (!protectedUI)
    return;

  InputSystemCursorShape shape = resize_cursor ? InputSystemCursorShape::SizeWestEast : InputSystemCursorShape::Arrow;
  protectedUI.ProtectedCursor(InputSystemCursor::Create(shape));
}

static int winuiTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->user_resize = 1;
  else
    ih->data->user_resize = 0;

  return 0;
}

/****************************************************************************
 * Focus Visual
 ****************************************************************************/

static void winuiTableClearFocusVisual(Ihandle* ih)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux || aux->current_row <= 0 || aux->current_col <= 0)
    return;

  Grid rowGrid = winuiTableGetRowGrid(ih, aux->current_row);
  if (!rowGrid)
    return;

  Border border = winuiTableGetCellBorder(rowGrid, aux->current_col - 1);
  if (border)
  {
    if (aux->show_grid)
      border.BorderThickness(ThicknessHelper::FromLengths(0, 0, 1, 1));
    else
      border.BorderThickness(ThicknessHelper::FromLengths(0, 0, 0, 0));
    border.Padding(ThicknessHelper::FromLengths(4, 2, 4, 2));
    border.BorderBrush(winuiTableGridLineBrush());
  }
}

static void winuiTableSetFocusVisual(Ihandle* ih, int lin, int col)
{
  if (lin <= 0 || col <= 0)
    return;

  if (!iupAttribGetBoolean(ih, "FOCUSRECT"))
    return;

  Grid rowGrid = winuiTableGetRowGrid(ih, lin);
  if (!rowGrid)
    return;

  Border border = winuiTableGetCellBorder(rowGrid, col - 1);
  if (border)
  {
    IupWinUITableAux* aux = winuiTableGetAux(ih);
    border.BorderThickness(ThicknessHelper::FromLengths(1, 1, 1, 1));
    border.BorderBrush(winuiTableFocusBrush());
    if (aux && aux->show_grid)
      border.Padding(ThicknessHelper::FromLengths(3, 1, 4, 2));
    else
      border.Padding(ThicknessHelper::FromLengths(3, 1, 3, 1));
  }
}

/****************************************************************************
 * Cell Click Detection
 ****************************************************************************/

static void winuiTableGetCellFromPoint(Ihandle* ih, DependencyObject source, int* out_lin, int* out_col)
{
  *out_lin = 0;
  *out_col = 0;

  ListView listView = winuiTableGetListView(ih);
  if (!listView)
    return;

  int col = -1;
  DependencyObject current = source;

  while (current)
  {
    auto border = current.try_as<Border>();
    if (border && col < 0)
    {
      auto parent = VisualTreeHelper::GetParent(border);
      if (parent && parent.try_as<Grid>())
        col = Grid::GetColumn(border);
    }

    auto lvi = current.try_as<ListViewItem>();
    if (lvi)
    {
      int row = listView.IndexFromContainer(lvi);
      if (row >= 0 && col >= 0)
      {
        *out_lin = row + 1;
        *out_col = col + 1;
      }
      return;
    }

    current = VisualTreeHelper::GetParent(current);
  }
}

/****************************************************************************
 * Cell Editing
 ****************************************************************************/

static void winuiTableEndEdit(Ihandle* ih, bool save)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux || !aux->editing)
    return;

  int lin = aux->edit_row;
  int col = aux->edit_col;

  Grid rowGrid = winuiTableGetRowGrid(ih, lin);
  if (!rowGrid)
  {
    aux->editing = false;
    aux->edit_row = 0;
    aux->edit_col = 0;
    return;
  }

  Border cellBorder = winuiTableGetCellBorder(rowGrid, col - 1);
  if (!cellBorder)
  {
    aux->editing = false;
    aux->edit_row = 0;
    aux->edit_col = 0;
    return;
  }

  auto editBox = cellBorder.Child().try_as<TextBox>();
  char* new_value = NULL;
  if (editBox)
    new_value = iupwinuiHStringToString(editBox.Text());

  aux->editing = false;
  aux->edit_row = 0;
  aux->edit_col = 0;

  IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb)
  {
    int ret = editend_cb(ih, lin, col, new_value ? new_value : (char*)"", save ? 1 : 0);
    if (ret == IUP_IGNORE && save)
    {
      aux->editing = true;
      aux->edit_row = lin;
      aux->edit_col = col;
      if (editBox)
      {
        editBox.Focus(FocusState::Programmatic);
        editBox.SelectAll();
      }
      return;
    }
  }

  TextBlock tb;
  tb.TextTrimming(TextTrimming::CharacterEllipsis);
  winuiTableUpdateCellFont(ih, tb);

  if (save)
  {
    char* old_value = iupdrvTableGetCellValue(ih, lin, col);
    char* old_copy = old_value ? iupStrDup(old_value) : NULL;

    IFniis edition_cb = (IFniis)IupGetCallback(ih, "EDITION_CB");
    if (edition_cb)
    {
      int ret = edition_cb(ih, lin, col, new_value ? new_value : (char*)"");
      if (ret == IUP_IGNORE)
      {
        tb.Text(iupwinuiStringToHString(old_copy ? old_copy : ""));
        cellBorder.Child(tb);
        if (old_copy) free(old_copy);
        return;
      }
    }

    iupdrvTableSetCellValue(ih, lin, col, new_value ? new_value : "");

    int text_changed = 0;
    if (!old_copy && new_value && new_value[0])
      text_changed = 1;
    else if (old_copy && !new_value)
      text_changed = 1;
    else if (old_copy && new_value && strcmp(old_copy, new_value) != 0)
      text_changed = 1;

    if (text_changed)
    {
      IFnii value_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
      if (value_cb)
        value_cb(ih, lin, col);
    }

    tb.Text(iupwinuiStringToHString(new_value ? new_value : ""));

    if (old_copy) free(old_copy);
  }
  else
  {
    const char* original = iupdrvTableGetCellValue(ih, lin, col);
    tb.Text(iupwinuiStringToHString(original ? original : ""));
  }

  cellBorder.Child(tb);

  winuiTableSetFocusVisual(ih, lin, col);

  ListView listView = winuiTableGetListView(ih);
  if (listView)
    listView.Focus(FocusState::Programmatic);
}

static bool winuiTableIsCellEditable(Ihandle* ih, int lin, int col)
{
  if (lin < 1 || col < 1)
    return false;

  char name[50];
  sprintf(name, "EDITABLE%d", col);
  char* editable = iupAttribGet(ih, name);
  if (!editable)
    editable = iupAttribGet(ih, "EDITABLE");

  return iupStrBoolean(editable) ? true : false;
}

static void winuiTableStartEdit(Ihandle* ih, int lin, int col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
    return;

  if (!winuiTableIsCellEditable(ih, lin, col))
    return;

  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    int ret = editbegin_cb(ih, lin, col);
    if (ret == IUP_IGNORE)
      return;
  }

  if (aux->editing)
    winuiTableEndEdit(ih, true);

  Grid rowGrid = winuiTableGetRowGrid(ih, lin);
  if (!rowGrid)
    return;

  Border cellBorder = winuiTableGetCellBorder(rowGrid, col - 1);
  if (!cellBorder)
    return;

  const char* value = iupdrvTableGetCellValue(ih, lin, col);

  TextBox editBox;
  editBox.Text(iupwinuiStringToHString(value ? value : ""));
  iupwinuiUpdateControlFont(ih, editBox);
  editBox.Padding(ThicknessHelper::FromLengths(2, 0, 2, 0));
  editBox.MinHeight(0);
  editBox.MinWidth(0);
  editBox.SelectAll();

  editBox.KeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
    IupWinUITableAux* a = winuiTableGetAux(ih);
    if (!a) return;

    if (args.Key() == Windows::System::VirtualKey::Enter)
    {
      winuiTableEndEdit(ih, true);
      args.Handled(true);
    }
    else if (args.Key() == Windows::System::VirtualKey::Escape)
    {
      winuiTableEndEdit(ih, false);
      args.Handled(true);
    }
  });

  editBox.LostFocus([ih](IInspectable const&, RoutedEventArgs const&) {
    IupWinUITableAux* a = winuiTableGetAux(ih);
    if (a && a->editing)
      winuiTableEndEdit(ih, true);
  });

  cellBorder.Child(editBox);
  editBox.Focus(FocusState::Programmatic);

  aux->editing = true;
  aux->edit_row = lin;
  aux->edit_col = col;
}

/****************************************************************************
 * Cell Operations
 ****************************************************************************/

extern "C" void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (!iupStrBoolean(virtualmode))
  {
    if (aux->cell_values)
      winuiTableSetCell(&aux->cell_values[lin - 1][col - 1], value);
  }

  Grid rowGrid = winuiTableGetRowGrid(ih, lin);
  if (rowGrid)
  {
    Border border = winuiTableGetCellBorder(rowGrid, col - 1);
    TextBlock tb = winuiTableGetCellTextBlock(rowGrid, col - 1);
    if (tb)
      tb.Text(iupwinuiStringToHString(value ? value : ""));
    if (border && tb)
      winuiTableApplyCellColors(ih, lin, col, border, tb);
  }
}

extern "C" char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
    return NULL;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return NULL;

  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (iupStrBoolean(virtualmode))
  {
    sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
    if (value_cb)
      return value_cb(ih, lin, col);
    return NULL;
  }

  if (aux->cell_values)
    return aux->cell_values[lin - 1][col - 1];

  return NULL;
}

extern "C" void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  if (!ih->handle)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  iupAttribSetStrId2(ih, "_IUPWINUI_CELLIMAGE", lin, col, image);

  Grid rowGrid = winuiTableGetRowGrid(ih, lin);
  if (rowGrid)
    winuiTableSetCellImageFromName(ih, rowGrid, col - 1, image);
  else
    iupdrvTableRedraw(ih);
}

/****************************************************************************
 * Column Operations
 ****************************************************************************/

extern "C" void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux || col < 1 || col > ih->data->num_col)
    return;

  if (aux->col_titles[col - 1])
    free(aux->col_titles[col - 1]);
  aux->col_titles[col - 1] = title ? iupStrDup(title) : NULL;

  StackPanel header = winuiTableGetHeader(ih);
  if (header && (uint32_t)(col - 1) < header.Children().Size())
  {
    auto border = header.Children().GetAt(col - 1).try_as<Border>();
    if (border)
    {
      auto tb = border.Child().try_as<TextBlock>();
      if (tb)
      {
        tb.Text(iupwinuiStringToHString(title ? title : ""));

        if (aux->sort_column == col)
        {
          hstring arrow = (aux->sort_ascending == 1) ? L" \u25B2" : L" \u25BC";
          tb.Text(tb.Text() + arrow);
        }
      }
    }
  }
}

extern "C" char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux || col < 1 || col > ih->data->num_col)
    return NULL;

  return aux->col_titles[col - 1];
}

extern "C" void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux || col < 1 || col > ih->data->num_col)
    return;

  aux->col_widths[col - 1] = width;
  aux->col_width_set[col - 1] = true;

  StackPanel header = winuiTableGetHeader(ih);
  if (header && (uint32_t)(col - 1) < header.Children().Size())
  {
    auto border = header.Children().GetAt(col - 1).try_as<Border>();
    if (border)
      border.Width((double)width);
  }

  winuiTableAdjustColumnWidths(ih);
}

extern "C" int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux || col < 1 || col > ih->data->num_col)
    return 0;

  return aux->col_widths[col - 1];
}

/****************************************************************************
 * Selection and Focus
 ****************************************************************************/

extern "C" void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  ListView listView = winuiTableGetListView(ih);
  if (!aux || !listView)
    return;

  if (lin < 0 || col < 0)
    return;
  if (lin > ih->data->num_lin || col > ih->data->num_col)
    return;

  winuiTableClearFocusVisual(ih);

  aux->current_row = lin;
  aux->current_col = col;

  if (lin == 0 || col == 0)
  {
    listView.SelectedIndex(-1);
    return;
  }

  aux->suppress_callbacks = true;
  listView.SelectedIndex(lin - 1);
  aux->suppress_callbacks = false;

  listView.ScrollIntoView(listView.Items().GetAt(lin - 1));

  winuiTableSetFocusVisual(ih, lin, col);
}

extern "C" void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
  {
    *lin = 0;
    *col = 0;
    return;
  }

  *lin = aux->current_row;
  *col = aux->current_col;
}

/****************************************************************************
 * Scrolling
 ****************************************************************************/

extern "C" void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  (void)col;
  ListView listView = winuiTableGetListView(ih);
  if (!listView)
    return;

  if (lin < 1 || lin > ih->data->num_lin)
    return;

  listView.ScrollIntoView(listView.Items().GetAt(lin - 1));
}

/****************************************************************************
 * Display
 ****************************************************************************/

extern "C" void iupdrvTableRedraw(Ihandle* ih)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  ListView listView = winuiTableGetListView(ih);
  if (!aux || !listView)
    return;

  if (aux->isVirtual)
  {
    winuiTableRebuildListViewItems(ih);
    listView.UpdateLayout();
    return;
  }

  for (int i = 0; i < ih->data->num_lin; i++)
  {
    Grid rowGrid = winuiTableGetRowGrid(ih, i + 1);
    if (!rowGrid) continue;

    for (int j = 0; j < ih->data->num_col; j++)
    {
      Border border = winuiTableGetCellBorder(rowGrid, j);
      TextBlock tb = winuiTableGetCellTextBlock(rowGrid, j);
      if (border && tb)
        winuiTableApplyCellColors(ih, i + 1, j + 1, border, tb);
    }
  }

  listView.UpdateLayout();
}

void winuiTableRefreshThemeColors(Ihandle* ih)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  ListView listView = winuiTableGetListView(ih);
  if (!aux || !listView)
    return;

  SolidColorBrush gridBrush = winuiTableGridLineBrush();

  StackPanel header = winuiTableGetHeader(ih);
  if (header)
  {
    for (uint32_t i = 0; i < header.Children().Size() && (int)i < ih->data->num_col; i++)
    {
      auto border = header.Children().GetAt(i).try_as<Border>();
      if (border)
      {
        border.Background(winuiTableHeaderBgBrush());
        border.BorderBrush(gridBrush);
      }
    }
  }

  if (aux->isVirtual)
  {
    winuiTableRebuildListViewItems(ih);
  }
  else
  {
    for (int i = 0; i < ih->data->num_lin; i++)
    {
      Grid rowGrid = winuiTableGetRowGrid(ih, i + 1);
      if (!rowGrid) continue;

      for (int j = 0; j < ih->data->num_col; j++)
      {
        Border border = winuiTableGetCellBorder(rowGrid, j);
        TextBlock tb = winuiTableGetCellTextBlock(rowGrid, j);
        if (border)
          border.BorderBrush(gridBrush);
        if (border && tb)
          winuiTableApplyCellColors(ih, i + 1, j + 1, border, tb);
      }
    }
  }

  listView.UpdateLayout();
}

extern "C" void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
    return;

  aux->show_grid = show ? true : false;

  if (aux->isVirtual)
  {
    iupdrvTableRedraw(ih);
    return;
  }

  ListView listView = winuiTableGetListView(ih);
  if (!listView)
    return;

  for (uint32_t r = 0; r < listView.Items().Size(); r++)
  {
    auto rowGrid = listView.Items().GetAt(r).try_as<Grid>();
    if (!rowGrid)
      continue;

    for (uint32_t c = 0; c < rowGrid.Children().Size(); c++)
    {
      auto border = rowGrid.Children().GetAt(c).try_as<Border>();
      if (!border)
        continue;

      bool is_focused = ((int)r + 1 == aux->current_row && Grid::GetColumn(border) == aux->current_col - 1);
      if (is_focused)
        continue;

      if (show)
        border.BorderThickness(ThicknessHelper::FromLengths(0, 0, 1, 1));
      else
        border.BorderThickness(ThicknessHelper::FromLengths(0, 0, 0, 0));
    }
  }
}

/****************************************************************************
 * Sizing
 ****************************************************************************/

extern "C" int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  return 2;
}

static int winuiTableMeasureTextHeight(Ihandle* ih)
{
  TextBlock tb;
  iupwinuiUpdateTextBlockFont(ih, tb);
  tb.Text(L"Wg");
  tb.Measure(Size(10000, 10000));
  return (int)ceil(tb.DesiredSize().Height);
}

extern "C" int iupdrvTableGetRowHeight(Ihandle* ih)
{
  int text_height = winuiTableMeasureTextHeight(ih);
  /* Cell Border: Padding(4,2,4,2) + BorderThickness(0,0,1,1) = 2 top + 2 bottom + 1 grid bottom = 5 */
  int row_height = text_height + 5;

  if (ih->data->show_image)
  {
    int charheight;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    int image_height = charheight + 8;
    if (image_height > row_height)
      row_height = image_height;
  }

  return row_height;
}

extern "C" int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  int text_height = winuiTableMeasureTextHeight(ih);
  /* Header Border: Padding(4,4,4,4) + BorderThickness(0,0,1,1) = 4 top + 4 bottom + 1 grid bottom = 9 */
  return text_height + 9;
}

extern "C" void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  int sb_size = iupdrvGetScrollbarSize();
  *w += sb_size + 2;
  *h += 2;
}

/****************************************************************************
 * Table Structure
 ****************************************************************************/

extern "C" void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  ListView listView = winuiTableGetListView(ih);
  if (!aux || !listView)
    return;

  int old_num_lin = ih->data->num_lin;
  if (num_lin == old_num_lin)
    return;

  if (aux->isVirtual)
  {
    winuiTableSetVirtualItems(listView, num_lin);
    ih->data->num_lin = num_lin;
    return;
  }

  if (num_lin < old_num_lin)
  {
    for (int i = old_num_lin - 1; i >= num_lin; i--)
      listView.Items().RemoveAt(i);
  }
  else
  {
    for (int i = old_num_lin; i < num_lin; i++)
    {
      Grid rowGrid = winuiTableCreateRowGrid(ih, ih->data->num_col, aux->show_grid);
      listView.Items().Append(rowGrid);
    }
  }

  {
    char*** new_cell_values = (char***)calloc(num_lin, sizeof(char**));

    for (int i = 0; i < num_lin; i++)
    {
      new_cell_values[i] = (char**)calloc(ih->data->num_col, sizeof(char*));

      if (i < old_num_lin && aux->cell_values)
      {
        for (int j = 0; j < ih->data->num_col; j++)
          new_cell_values[i][j] = aux->cell_values[i][j];
      }
    }

    for (int i = num_lin; i < old_num_lin; i++)
    {
      if (aux->cell_values && aux->cell_values[i])
      {
        for (int j = 0; j < ih->data->num_col; j++)
          winuiTableFreeCell(&aux->cell_values[i][j]);
        free(aux->cell_values[i]);
      }
    }

    if (aux->cell_values)
      free(aux->cell_values);

    aux->cell_values = new_cell_values;
  }

  ih->data->num_lin = num_lin;
}

extern "C" void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  ListView listView = winuiTableGetListView(ih);
  StackPanel header = winuiTableGetHeader(ih);
  if (!aux || !listView)
    return;

  int old_num_col = ih->data->num_col;
  if (num_col == old_num_col)
    return;

  if (header)
  {
    if (num_col < old_num_col)
    {
      for (int i = old_num_col - 1; i >= num_col; i--)
        header.Children().RemoveAt(i);
    }
    else
    {
      for (int i = old_num_col; i < num_col; i++)
      {
        Border hdr = winuiTableCreateHeaderCell(ih, i);
        hdr.Tapped([ih](IInspectable const& sender, TappedRoutedEventArgs const&) {
          if (!ih->data->sortable) return;
          auto border = sender.try_as<Border>();
          if (!border) return;
          int c = unbox_value<int>(border.Tag()) + 1;
          winuiTableSort(ih, c);
        });
        header.Children().Append(hdr);
      }
    }
  }

  if (!aux->isVirtual)
  {
    for (uint32_t r = 0; r < listView.Items().Size(); r++)
    {
      auto rowGrid = listView.Items().GetAt(r).try_as<Grid>();
      if (!rowGrid)
        continue;

      if (num_col < old_num_col)
      {
        for (int i = old_num_col - 1; i >= num_col; i--)
        {
          if ((uint32_t)i < rowGrid.ColumnDefinitions().Size())
            rowGrid.ColumnDefinitions().RemoveAt(i);
          Border border = winuiTableGetCellBorder(rowGrid, i);
          if (border)
          {
            uint32_t idx;
            if (rowGrid.Children().IndexOf(border, idx))
              rowGrid.Children().RemoveAt(idx);
          }
        }
      }
      else
      {
        for (int i = old_num_col; i < num_col; i++)
        {
          ColumnDefinition colDef;
          colDef.Width(GridLength{100.0, GridUnitType::Pixel});
          rowGrid.ColumnDefinitions().Append(colDef);

          Border border;
          border.Padding(ThicknessHelper::FromLengths(4, 2, 4, 2));
          Grid::SetColumn(border, i);
          if (aux->show_grid)
            border.BorderThickness(ThicknessHelper::FromLengths(0, 0, 1, 1));
          border.BorderBrush(winuiTableGridLineBrush());

          TextBlock tb;
          tb.TextTrimming(TextTrimming::CharacterEllipsis);
          winuiTableUpdateCellFont(ih, tb);
          border.Child(tb);

          rowGrid.Children().Append(border);
        }
      }
    }
  }

  int* new_col_widths = (int*)calloc(num_col, sizeof(int));
  bool* new_col_width_set = (bool*)calloc(num_col, sizeof(bool));
  char** new_col_titles = (char**)calloc(num_col, sizeof(char*));

  for (int i = 0; i < num_col; i++)
  {
    if (i < old_num_col)
    {
      new_col_widths[i] = aux->col_widths[i];
      new_col_width_set[i] = aux->col_width_set[i];
      new_col_titles[i] = aux->col_titles[i];
    }
    else
    {
      new_col_widths[i] = 100;
      new_col_width_set[i] = false;
      new_col_titles[i] = NULL;
    }
  }

  for (int i = num_col; i < old_num_col; i++)
  {
    if (aux->col_titles[i])
      free(aux->col_titles[i]);
  }

  free(aux->col_widths);
  free(aux->col_width_set);
  free(aux->col_titles);

  aux->col_widths = new_col_widths;
  aux->col_width_set = new_col_width_set;
  aux->col_titles = new_col_titles;

  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  if (!iupStrBoolean(virtualmode) && aux->cell_values)
  {
    for (int i = 0; i < ih->data->num_lin; i++)
    {
      char** new_row = (char**)calloc(num_col, sizeof(char*));

      for (int j = 0; j < num_col; j++)
      {
        if (j < old_num_col)
          new_row[j] = aux->cell_values[i][j];
        else
          new_row[j] = NULL;
      }

      for (int j = num_col; j < old_num_col; j++)
        winuiTableFreeCell(&aux->cell_values[i][j]);

      free(aux->cell_values[i]);
      aux->cell_values[i] = new_row;
    }
  }
}

extern "C" void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  ListView listView = winuiTableGetListView(ih);
  if (!aux || !listView)
    return;

  if (pos < 0)
    pos = ih->data->num_lin;
  if (pos > ih->data->num_lin)
    pos = ih->data->num_lin;

  if (aux->isVirtual)
  {
    auto source = listView.ItemsSource().as<Collections::IVector<IInspectable>>();
    source.InsertAt(pos, box_value(pos));
    ih->data->num_lin++;
    return;
  }

  Grid rowGrid = winuiTableCreateRowGrid(ih, ih->data->num_col, aux->show_grid);
  listView.Items().InsertAt(pos, rowGrid);

  {
    int new_num_lin = ih->data->num_lin + 1;
    char*** new_cell_values = (char***)calloc(new_num_lin, sizeof(char**));

    for (int i = 0; i < new_num_lin; i++)
    {
      new_cell_values[i] = (char**)calloc(ih->data->num_col, sizeof(char*));

      if (i < pos && aux->cell_values)
      {
        for (int j = 0; j < ih->data->num_col; j++)
          new_cell_values[i][j] = aux->cell_values[i][j];
      }
      else if (i > pos && aux->cell_values)
      {
        for (int j = 0; j < ih->data->num_col; j++)
          new_cell_values[i][j] = aux->cell_values[i - 1][j];
      }
    }

    if (aux->cell_values)
    {
      for (int i = 0; i < ih->data->num_lin; i++)
        free(aux->cell_values[i]);
      free(aux->cell_values);
    }

    aux->cell_values = new_cell_values;
  }

  ih->data->num_lin++;
}

extern "C" void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  ListView listView = winuiTableGetListView(ih);
  if (!aux || !listView)
    return;

  if (pos < 1 || pos > ih->data->num_lin)
    return;

  if (aux->isVirtual)
  {
    auto source = listView.ItemsSource().as<Collections::IVector<IInspectable>>();
    source.RemoveAt(pos - 1);
    ih->data->num_lin--;
    return;
  }

  listView.Items().RemoveAt(pos - 1);

  if (aux->cell_values)
  {
    for (int j = 0; j < ih->data->num_col; j++)
      winuiTableFreeCell(&aux->cell_values[pos - 1][j]);

    int new_num_lin = ih->data->num_lin - 1;
    char*** new_cell_values = (char***)calloc(new_num_lin, sizeof(char**));

    for (int i = 0; i < new_num_lin; i++)
    {
      new_cell_values[i] = (char**)calloc(ih->data->num_col, sizeof(char*));

      if (i < pos - 1)
      {
        for (int j = 0; j < ih->data->num_col; j++)
          new_cell_values[i][j] = aux->cell_values[i][j];
      }
      else
      {
        for (int j = 0; j < ih->data->num_col; j++)
          new_cell_values[i][j] = aux->cell_values[i + 1][j];
      }
    }

    for (int i = 0; i < ih->data->num_lin; i++)
      free(aux->cell_values[i]);
    free(aux->cell_values);

    aux->cell_values = new_cell_values;
  }

  ih->data->num_lin--;
}

extern "C" void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  if (pos < 0)
    pos = ih->data->num_col;
  if (pos > ih->data->num_col)
    pos = ih->data->num_col;

  ih->data->num_col++;
  iupdrvTableSetNumCol(ih, ih->data->num_col);
}

extern "C" void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  if (pos < 1 || pos > ih->data->num_col)
    return;

  ih->data->num_col--;
  iupdrvTableSetNumCol(ih, ih->data->num_col);
}

/****************************************************************************
 * Custom Colors
 ****************************************************************************/

static void winuiTableApplyCellColors(Ihandle* ih, int lin, int col, Border border, TextBlock tb)
{
  bool is_selected = winuiTableIsRowSelected(ih, lin);

  if (is_selected)
  {
    border.Background(winuiTableSelectionBrush());
    tb.ClearValue(TextBlock::ForegroundProperty());
    return;
  }

  char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);
  if (!bgcolor)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);
  if (!bgcolor)
    bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);

  if (!bgcolor)
  {
    char* alternate_color = iupAttribGet(ih, "ALTERNATECOLOR");
    if (iupStrBoolean(alternate_color))
    {
      if (lin % 2 == 0)
        bgcolor = iupAttribGet(ih, "EVENROWCOLOR");
      else
        bgcolor = iupAttribGet(ih, "ODDROWCOLOR");
    }
  }

  if (bgcolor && *bgcolor)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(bgcolor, &r, &g, &b))
      border.Background(SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, r, g, b)));
    else
      border.ClearValue(Border::BackgroundProperty());
  }
  else
  {
    border.ClearValue(Border::BackgroundProperty());
  }

  char* fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);

  if (fgcolor && *fgcolor)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(fgcolor, &r, &g, &b))
      tb.Foreground(SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, r, g, b)));
    else
      tb.ClearValue(TextBlock::ForegroundProperty());
  }
  else
  {
    tb.ClearValue(TextBlock::ForegroundProperty());
  }
}

/****************************************************************************
 * Keyboard Handling
 ****************************************************************************/

static void winuiTableKeyDown(Ihandle* ih, KeyRoutedEventArgs const& args)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  if (!aux)
    return;

  if (aux->editing)
    return;

  int lin = aux->current_row;
  int col = aux->current_col;
  bool handled = false;

  auto key = args.Key();

  switch (key)
  {
    case Windows::System::VirtualKey::Up:
      if (lin > 1)
      {
        iupdrvTableSetFocusCell(ih, lin - 1, col);
        IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
        if (cb) cb(ih, lin - 1, col);
        handled = true;
      }
      break;

    case Windows::System::VirtualKey::Down:
      if (lin < ih->data->num_lin)
      {
        iupdrvTableSetFocusCell(ih, lin + 1, col);
        IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
        if (cb) cb(ih, lin + 1, col);
        handled = true;
      }
      break;

    case Windows::System::VirtualKey::Left:
      if (col > 1)
      {
        iupdrvTableSetFocusCell(ih, lin, col - 1);
        IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
        if (cb) cb(ih, lin, col - 1);
        handled = true;
      }
      break;

    case Windows::System::VirtualKey::Right:
      if (col < ih->data->num_col)
      {
        iupdrvTableSetFocusCell(ih, lin, col + 1);
        IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
        if (cb) cb(ih, lin, col + 1);
        handled = true;
      }
      break;

    case Windows::System::VirtualKey::Tab:
    {
      bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

      if (shift)
      {
        if (col > 1)
        {
          iupdrvTableSetFocusCell(ih, lin, col - 1);
          IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
          if (cb) cb(ih, lin, col - 1);
          handled = true;
        }
        else if (lin > 1)
        {
          iupdrvTableSetFocusCell(ih, lin - 1, ih->data->num_col);
          IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
          if (cb) cb(ih, lin - 1, ih->data->num_col);
          handled = true;
        }
      }
      else
      {
        if (col < ih->data->num_col)
        {
          iupdrvTableSetFocusCell(ih, lin, col + 1);
          IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
          if (cb) cb(ih, lin, col + 1);
          handled = true;
        }
        else if (lin < ih->data->num_lin)
        {
          iupdrvTableSetFocusCell(ih, lin + 1, 1);
          IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
          if (cb) cb(ih, lin + 1, 1);
          handled = true;
        }
      }
      break;
    }

    case Windows::System::VirtualKey::Enter:
    case Windows::System::VirtualKey::F2:
      winuiTableStartEdit(ih, lin, col);
      handled = true;
      break;

    case Windows::System::VirtualKey::C:
      if (GetKeyState(VK_CONTROL) & 0x8000)
      {
        const char* value = iupdrvTableGetCellValue(ih, lin, col);
        if (value && *value)
        {
          IupSetGlobal("CLIPBOARD", value);
          handled = true;
        }
      }
      break;

    case Windows::System::VirtualKey::V:
      if (GetKeyState(VK_CONTROL) & 0x8000)
      {
        if (winuiTableIsCellEditable(ih, lin, col))
        {
          const char* text = IupGetGlobal("CLIPBOARD");
          if (text && *text)
          {
            char* old_ptr = iupdrvTableGetCellValue(ih, lin, col);
            char* old_copy = old_ptr ? iupStrDup(old_ptr) : NULL;

            iupdrvTableSetCellValue(ih, lin, col, text);

            int changed = 0;
            if (!old_copy && text && *text) changed = 1;
            else if (old_copy && !text) changed = 1;
            else if (old_copy && text && strcmp(old_copy, text) != 0) changed = 1;

            if (changed)
            {
              IFnii value_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
              if (value_cb) value_cb(ih, lin, col);
            }

            if (old_copy) free(old_copy);
            handled = true;
          }
        }
      }
      break;

    default:
      break;
  }

  if (handled)
    args.Handled(true);
}

/****************************************************************************
 * Attribute Handlers
 ****************************************************************************/

static int winuiTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);

  if (iupStrBoolean(value))
    ih->data->sortable = 1;
  else
  {
    ih->data->sortable = 0;

    if (aux)
    {
      aux->sort_column = 0;
      aux->sort_ascending = 0;

      StackPanel header = winuiTableGetHeader(ih);
      if (header)
      {
        for (uint32_t i = 0; i < header.Children().Size(); i++)
        {
          auto border = header.Children().GetAt(i).try_as<Border>();
          if (!border) continue;
          auto tb = border.Child().try_as<TextBlock>();
          if (!tb) continue;

          const char* title = (aux->col_titles && (int)i < ih->data->num_col && aux->col_titles[i]) ? aux->col_titles[i] : "";
          tb.Text(iupwinuiStringToHString(title));
        }
      }
    }
  }

  return 0;
}

static int winuiTableSetAlignmentAttrib(Ihandle* ih, int col, const char* value)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);
  ListView listView = winuiTableGetListView(ih);
  if (!aux || !listView || col < 1 || col > ih->data->num_col)
    return 0;

  TextAlignment align = TextAlignment::Left;
  if (iupStrEqualNoCase(value, "ARIGHT") || iupStrEqualNoCase(value, "RIGHT"))
    align = TextAlignment::Right;
  else if (iupStrEqualNoCase(value, "ACENTER") || iupStrEqualNoCase(value, "CENTER"))
    align = TextAlignment::Center;

  StackPanel headerPanel = winuiTableGetHeader(ih);
  if (headerPanel && (uint32_t)(col - 1) < headerPanel.Children().Size())
  {
    auto hdrBorder = headerPanel.Children().GetAt(col - 1).try_as<Border>();
    if (hdrBorder)
    {
      auto hdrTb = hdrBorder.Child().try_as<TextBlock>();
      if (hdrTb)
        hdrTb.TextAlignment(align);
    }
  }

  if (aux->isVirtual)
  {
    for (int r = 0; r < ih->data->num_lin; r++)
    {
      auto container = listView.ContainerFromIndex(r);
      if (!container) continue;
      auto lvi = container.try_as<ListViewItem>();
      if (!lvi) continue;
      auto presenter = lvi.ContentTemplateRoot().try_as<ContentPresenter>();
      if (!presenter) continue;
      Grid rowGrid = presenter.Content().try_as<Grid>();
      if (!rowGrid) continue;

      TextBlock tb = winuiTableGetCellTextBlock(rowGrid, col - 1);
      if (tb)
        tb.TextAlignment(align);
    }
  }
  else
  {
    for (uint32_t r = 0; r < listView.Items().Size(); r++)
    {
      auto rowGrid = listView.Items().GetAt(r).try_as<Grid>();
      if (!rowGrid) continue;

      TextBlock tb = winuiTableGetCellTextBlock(rowGrid, col - 1);
      if (tb)
        tb.TextAlignment(align);
    }
  }

  return 1;
}

static int winuiTableSetActiveAttrib(Ihandle* ih, const char* value)
{
  ListView listView = winuiTableGetListView(ih);
  if (listView)
    listView.IsEnabled(iupStrBoolean(value) ? true : false);
  return 0;
}

/****************************************************************************
 * Layout Update
 ****************************************************************************/

static void winuiTableLayoutUpdateMethod(Ihandle* ih)
{
  int width = ih->currentwidth;
  int height = ih->currentheight;
  int header_height = iupdrvTableGetHeaderHeight(ih);

  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    int row_height = iupdrvTableGetRowHeight(ih);
    int max_height = header_height + (row_height * visiblelines) + 2;
    if (height > max_height)
      height = max_height;
  }

  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0)
  {
    IupWinUITableAux* aux = winuiTableGetAux(ih);
    if (aux)
    {
      int cols_width = 0;
      int num_cols = visiblecolumns;
      if (num_cols > ih->data->num_col)
        num_cols = ih->data->num_col;

      for (int c = 0; c < num_cols; c++)
        cols_width += aux->col_widths[c];

      int sb_size = iupdrvGetScrollbarSize();
      int need_vert_sb = (visiblelines > 0 && ih->data->num_lin > visiblelines);
      int vert_sb_width = need_vert_sb ? sb_size : 0;
      int max_width = cols_width + vert_sb_width + 2;

      if (width > max_width)
        width = max_width;
    }
  }

  Grid containerGrid = winuiGetHandle<Grid>(ih);
  if (containerGrid)
  {
    Canvas::SetLeft(containerGrid, ih->x);
    Canvas::SetTop(containerGrid, ih->y);
    if (width > 0)
      containerGrid.Width((double)width);
    if (height > 0)
      containerGrid.Height((double)height);
  }

  StackPanel headerPanel = winuiTableGetHeader(ih);
  if (headerPanel)
    headerPanel.Height((double)header_height);

  winuiTableAdjustColumnWidths(ih);
}

/****************************************************************************
 * Map / UnMap
 ****************************************************************************/

static int winuiTableMapMethod(Ihandle* ih)
{
  if (!ih->parent)
    return IUP_ERROR;

  IupWinUITableAux* aux = new IupWinUITableAux();
  winuiSetAux(ih, IUPWINUI_TABLE_AUX, aux);

  int num_col = ih->data->num_col;
  int num_lin = ih->data->num_lin;

  aux->col_widths = (int*)calloc(num_col > 0 ? num_col : 1, sizeof(int));
  aux->col_width_set = (bool*)calloc(num_col > 0 ? num_col : 1, sizeof(bool));
  aux->col_titles = (char**)calloc(num_col > 0 ? num_col : 1, sizeof(char*));

  for (int i = 0; i < num_col; i++)
  {
    aux->col_widths[i] = 100;
    aux->col_width_set[i] = false;
  }

  char* virtualmode = iupAttribGet(ih, "VIRTUALMODE");
  int is_virtual = iupStrBoolean(virtualmode);

  if (!is_virtual && num_lin > 0 && num_col > 0)
  {
    aux->cell_values = (char***)calloc(num_lin, sizeof(char**));
    for (int i = 0; i < num_lin; i++)
      aux->cell_values[i] = (char**)calloc(num_col, sizeof(char*));
  }

  aux->current_row = 0;
  aux->current_col = 0;
  aux->sort_column = 0;
  aux->sort_ascending = 0;
  aux->show_grid = iupAttribGetBoolean(ih, "SHOWGRID") ? true : false;
  aux->suppress_callbacks = false;
  aux->editing = false;

  StackPanel headerPanel;
  headerPanel.Orientation(Orientation::Horizontal);
  headerPanel.HorizontalAlignment(HorizontalAlignment::Left);
  headerPanel.Background(winuiTableHeaderBgBrush());

  for (int i = 0; i < num_col; i++)
  {
    Border hdr = winuiTableCreateHeaderCell(ih, i);

    hdr.Tapped([ih](IInspectable const& sender, TappedRoutedEventArgs const&) {
      if (!ih->data->sortable) return;
      auto border = sender.try_as<Border>();
      if (!border) return;
      int c = unbox_value<int>(border.Tag()) + 1;
      winuiTableSort(ih, c);
    });

    headerPanel.Children().Append(hdr);
  }

  headerPanel.PointerPressed([ih](IInspectable const&, PointerRoutedEventArgs const& args) {
    IupWinUITableAux* a = winuiTableGetAux(ih);
    if (!a)
      return;

    StackPanel header = winuiTableGetHeader(ih);
    if (!header)
      return;

    auto point = args.GetCurrentPoint(header);
    double x = point.Position().X;
    double y = point.Position().Y;

    if (ih->data->user_resize)
    {
      int divider_col = winuiTableFindDividerColumn(ih, x);
      if (divider_col > 0)
      {
        a->resize_col = divider_col;
        a->resize_start_x = x;
        a->resize_start_width = a->col_widths[divider_col - 1];
        header.CapturePointer(args.Pointer());
        args.Handled(true);
        return;
      }
    }

    if (!ih->data->allow_reorder)
      return;

    int col = winuiTableFindColumnAtPoint(ih, x);
    if (col >= 1 && col <= ih->data->num_col)
    {
      a->drag_source_col = col;
      a->drag_target_col = col;
      a->drag_start_x = x;
      a->drag_start_y = y;
      a->dragging = false;
    }
  });

  headerPanel.PointerMoved([ih](IInspectable const&, PointerRoutedEventArgs const& args) {
    IupWinUITableAux* a = winuiTableGetAux(ih);
    if (!a)
      return;

    StackPanel header = winuiTableGetHeader(ih);
    if (!header)
      return;

    auto point = args.GetCurrentPoint(header);
    double x = point.Position().X;

    if (a->resize_col != 0)
    {
      double delta = x - a->resize_start_x;
      int new_width = a->resize_start_width + (int)delta;
      if (new_width < 20)
        new_width = 20;

      auto border = header.Children().GetAt(a->resize_col - 1).try_as<Border>();
      if (border)
      {
        Grid containerGrid = winuiGetHandle<Grid>(ih);
        if (containerGrid)
        {
          auto transform = border.TransformToVisual(containerGrid);
          auto pt = transform.TransformPoint(Point{0, 0});
          winuiTableUpdateResizeIndicator(ih, pt.X + new_width);
        }
      }
      return;
    }

    if (a->drag_source_col == 0)
    {
      if (ih->data->user_resize)
      {
        int divider_col = winuiTableFindDividerColumn(ih, x);
        winuiTableSetHeaderCursor(ih, divider_col > 0);
      }
      return;
    }

    double y = point.Position().Y;

    if (!a->dragging)
    {
      double dx = x - a->drag_start_x;
      double dy = y - a->drag_start_y;
      if (dx * dx + dy * dy < 25)
        return;
      a->dragging = true;
      header.CapturePointer(args.Pointer());
    }

    int target = winuiTableFindTargetColumn(ih, x);
    if (target >= 1 && target != a->drag_target_col)
    {
      a->drag_target_col = target;
      winuiTableUpdateDragIndicator(ih, target);
    }
  });

  headerPanel.PointerReleased([ih](IInspectable const&, PointerRoutedEventArgs const& args) {
    IupWinUITableAux* a = winuiTableGetAux(ih);
    if (!a)
      return;

    StackPanel header = winuiTableGetHeader(ih);

    if (a->resize_col != 0)
    {
      if (header)
        header.ReleasePointerCapture(args.Pointer());

      auto point = args.GetCurrentPoint(header);
      double delta = point.Position().X - a->resize_start_x;
      int new_width = a->resize_start_width + (int)delta;
      if (new_width < 20)
        new_width = 20;

      int col = a->resize_col;
      a->resize_col = 0;

      winuiTableHideResizeIndicator(ih);
      iupdrvTableSetColWidth(ih, col, new_width);

      args.Handled(true);
      return;
    }

    if (a->drag_source_col == 0)
      return;

    if (header)
      header.ReleasePointerCapture(args.Pointer());

    int source = a->drag_source_col;
    int target = a->drag_target_col;
    bool was_dragging = a->dragging;

    a->drag_source_col = 0;
    a->drag_target_col = 0;
    a->dragging = false;

    winuiTableHideDragIndicator(ih);

    if (was_dragging && source != target && source >= 1 && target >= 1)
    {
      winuiTableMoveColumn(ih, source, target);
      winuiTableRefreshAfterReorder(ih);
    }

    if (was_dragging)
      args.Handled(true);
  });

  ListView listView;
  listView.HorizontalAlignment(HorizontalAlignment::Left);
  listView.VerticalAlignment(VerticalAlignment::Top);
  listView.SelectionMode(ListViewSelectionMode::Single);

  char* selmode = iupAttribGet(ih, "SELECTIONMODE");
  if (selmode)
  {
    if (iupStrEqualNoCase(selmode, "MULTIPLE"))
      listView.SelectionMode(ListViewSelectionMode::Extended);
    else if (iupStrEqualNoCase(selmode, "NONE"))
      listView.SelectionMode(ListViewSelectionMode::None);
  }

  Style itemStyle{winrt::xaml_typename<ListViewItem>()};
  itemStyle.Setters().Append(Setter{Control::PaddingProperty(), box_value(Thickness{0, 0, 0, 0})});
  itemStyle.Setters().Append(Setter{FrameworkElement::MinHeightProperty(), box_value(0.0)});
  itemStyle.Setters().Append(Setter{FrameworkElement::MarginProperty(), box_value(Thickness{0, 0, 0, 0})});
  itemStyle.Setters().Append(Setter{Control::HorizontalContentAlignmentProperty(), box_value(HorizontalAlignment::Stretch)});
  listView.ItemContainerStyle(itemStyle);

  if (is_virtual)
  {
    aux->isVirtual = true;

    auto dt = XamlReader::Load(hstring(
      LR"(<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><ContentPresenter HorizontalContentAlignment='Stretch'/></DataTemplate>)")).as<DataTemplate>();
    listView.ItemTemplate(dt);

    winuiTableSetVirtualItems(listView, num_lin);

    aux->containerContentChangingToken = listView.ContainerContentChanging(
      [ih](ListViewBase const&, ContainerContentChangingEventArgs const& args) {
        if (args.InRecycleQueue())
          return;

        int lin = args.ItemIndex() + 1;
        winuiTablePopulateVirtualContainer(ih, lin, args.ItemContainer());

        args.Handled(true);
      });
  }
  else
  {
    for (int i = 0; i < num_lin; i++)
    {
      Grid rowGrid = winuiTableCreateRowGrid(ih, num_col, aux->show_grid);
      listView.Items().Append(rowGrid);
    }
  }

  aux->selectionChangedToken = listView.SelectionChanged([ih](IInspectable const&, SelectionChangedEventArgs const&) {
    IupWinUITableAux* a = winuiTableGetAux(ih);
    if (!a || a->suppress_callbacks)
      return;

    ListView lv = winuiTableGetListView(ih);
    if (!lv) return;

    int old_row = a->current_row;

    int row = lv.SelectedIndex() + 1;
    if (row > 0 && row != a->current_row)
    {
      winuiTableClearFocusVisual(ih);
      a->current_row = row;
      if (a->current_col <= 0)
        a->current_col = 1;

      if (old_row > 0)
        winuiTableUpdateRowColors(ih, old_row);
      winuiTableUpdateRowColors(ih, row);

      winuiTableSetFocusVisual(ih, a->current_row, a->current_col);

      IFnii cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
      if (cb) cb(ih, a->current_row, a->current_col);
    }
  });

  aux->doubleTappedToken = listView.DoubleTapped([ih](IInspectable const&, DoubleTappedRoutedEventArgs const& args) {
    auto source = args.OriginalSource().try_as<DependencyObject>();
    if (!source) return;

    int lin = 0, col = 0;
    winuiTableGetCellFromPoint(ih, source, &lin, &col);
    if (lin > 0 && col > 0)
      winuiTableStartEdit(ih, lin, col);
  });

  aux->keyDownToken = listView.PreviewKeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
    winuiTableKeyDown(ih, args);
  });

  listView.Tapped([ih](IInspectable const&, TappedRoutedEventArgs const& args) {
    IupWinUITableAux* a = winuiTableGetAux(ih);
    if (!a) return;

    auto source = args.OriginalSource().try_as<DependencyObject>();
    if (!source) return;

    int lin = 0, col = 0;
    winuiTableGetCellFromPoint(ih, source, &lin, &col);

    if (lin > 0 && col > 0)
    {
      if (lin != a->current_row || col != a->current_col)
      {
        winuiTableClearFocusVisual(ih);
        a->current_row = lin;
        a->current_col = col;
        winuiTableSetFocusVisual(ih, lin, col);
      }

      char status[IUPKEY_STATUS_SIZE] = "";
      iupwinuiButtonKeySetStatus(0, 0, status, 0);

      IFniis click_cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
      if (click_cb)
        click_cb(ih, lin, col, status);
    }
  });

  aux->sizeChangedToken = listView.SizeChanged([ih](IInspectable const&, SizeChangedEventArgs const&) {
    winuiTableAdjustColumnWidths(ih);
  });

  void* headerPtr = nullptr;
  winrt::copy_to_abi(headerPanel, headerPtr);
  iupAttribSet(ih, "_IUPWINUI_TABLE_HEADER", (char*)headerPtr);

  void* lvPtr = nullptr;
  winrt::copy_to_abi(listView, lvPtr);
  iupAttribSet(ih, "_IUPWINUI_TABLE_LISTVIEW", (char*)lvPtr);

  Grid containerGrid;

  RowDefinition headerRow;
  headerRow.Height(GridLengthHelper::Auto());
  containerGrid.RowDefinitions().Append(headerRow);

  RowDefinition dataRow;
  dataRow.Height(GridLength{1, GridUnitType::Star});
  containerGrid.RowDefinitions().Append(dataRow);

  Grid::SetRow(headerPanel, 0);
  containerGrid.Children().Append(headerPanel);

  Grid::SetRow(listView, 1);
  containerGrid.Children().Append(listView);

  auto gridVisual = ElementCompositionPreview::GetElementVisual(containerGrid);
  gridVisual.Clip(gridVisual.Compositor().CreateInsetClip());

  winuiStoreHandle(ih, containerGrid);

  iupwinuiAddToParent(ih);

  for (int i = 1; i <= num_col; i++)
  {
    char name[50];
    sprintf(name, "TITLE%d", i);
    char* title = iupAttribGet(ih, name);
    if (title)
      iupdrvTableSetColTitle(ih, i, title);

    sprintf(name, "RASTERWIDTH%d", i);
    char* rw = iupAttribGet(ih, name);
    if (!rw)
    {
      sprintf(name, "WIDTH%d", i);
      rw = iupAttribGet(ih, name);
    }
    if (rw)
    {
      int width;
      if (iupStrToInt(rw, &width) && width > 0)
        iupdrvTableSetColWidth(ih, i, width);
    }

    sprintf(name, "ALIGNMENT%d", i);
    char* align = iupAttribGet(ih, name);
    if (align)
      winuiTableSetAlignmentAttrib(ih, i, align);
  }

  char* focuscell = iupAttribGet(ih, "FOCUSCELL");
  if (focuscell)
  {
    int lin, col;
    if (iupStrToIntInt(focuscell, &lin, &col, ':') == 2)
      iupdrvTableSetFocusCell(ih, lin, col);
  }

  return IUP_NOERROR;
}

static void winuiTableUnMapMethod(Ihandle* ih)
{
  IupWinUITableAux* aux = winuiTableGetAux(ih);

  if (!aux)
  {
    winuiReleaseHandle<Grid>(ih);
    return;
  }

  if (aux->editing)
    winuiTableEndEdit(ih, false);

  ListView listView = winuiTableGetListView(ih);
  if (listView)
  {
    listView.SelectionChanged(aux->selectionChangedToken);
    listView.DoubleTapped(aux->doubleTappedToken);
    listView.PreviewKeyDown(aux->keyDownToken);
    listView.SizeChanged(aux->sizeChangedToken);
    if (aux->isVirtual)
      listView.ContainerContentChanging(aux->containerContentChangingToken);
  }

  char* indicatorPtr = iupAttribGet(ih, "_IUPWINUI_TABLE_DRAG_INDICATOR");
  if (indicatorPtr)
  {
    IInspectable release{nullptr};
    winrt::attach_abi(release, indicatorPtr);
    iupAttribSet(ih, "_IUPWINUI_TABLE_DRAG_INDICATOR", nullptr);
  }

  char* headerPtr = iupAttribGet(ih, "_IUPWINUI_TABLE_HEADER");
  if (headerPtr)
  {
    IInspectable release{nullptr};
    winrt::attach_abi(release, headerPtr);
    iupAttribSet(ih, "_IUPWINUI_TABLE_HEADER", nullptr);
  }

  if (aux->cell_values)
  {
    for (int i = 0; i < ih->data->num_lin; i++)
    {
      if (aux->cell_values[i])
      {
        for (int j = 0; j < ih->data->num_col; j++)
          winuiTableFreeCell(&aux->cell_values[i][j]);
        free(aux->cell_values[i]);
      }
    }
    free(aux->cell_values);
  }

  if (aux->col_titles)
  {
    for (int i = 0; i < ih->data->num_col; i++)
    {
      if (aux->col_titles[i])
        free(aux->col_titles[i]);
    }
    free(aux->col_titles);
  }

  if (aux->col_widths)
    free(aux->col_widths);
  if (aux->col_width_set)
    free(aux->col_width_set);

  char* lvPtr = iupAttribGet(ih, "_IUPWINUI_TABLE_LISTVIEW");
  if (lvPtr)
  {
    IInspectable obj{nullptr};
    winrt::attach_abi(obj, lvPtr);
    iupAttribSet(ih, "_IUPWINUI_TABLE_LISTVIEW", nullptr);
  }

  winuiFreeAux<IupWinUITableAux>(ih, IUPWINUI_TABLE_AUX);

  winuiReleaseHandle<Grid>(ih);
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = winuiTableMapMethod;
  ic->UnMap = winuiTableUnMapMethod;
  ic->LayoutUpdate = winuiTableLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SIZE", NULL, NULL, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);

  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, winuiTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, winuiTableSetAllowReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, winuiTableSetUserResizeAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ACTIVE", NULL, winuiTableSetActiveAttrib);

  iupClassRegisterAttribute(ic, "FOCUSRECT", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "ALIGNMENT", NULL, (IattribSetIdFunc)winuiTableSetAlignmentAttrib, IUPAF_NO_INHERIT);
}
