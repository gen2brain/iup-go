/** \file
 * \brief WinUI Driver - Tree Control
 *
 * Uses WinUI TreeView control.
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
#include "iup_tree.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_register.h"
#include "iup_childtree.h"
}

#include "iupwinui_drv.h"

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Text;
using namespace Windows::ApplicationModel::DataTransfer;


static TreeView winuiTreeGetTreeView(Ihandle* ih)
{
  return winuiGetHandle<TreeView>(ih);
}

static int winuiTreeFindNodeId(Ihandle* ih, TreeViewNode const& node)
{
  if (!node)
    return -1;

  return iupTreeFindNodeId(ih, (InodeHandle*)winrt::get_abi(node));
}

static void winuiTreeReleaseCacheNodes(Ihandle* ih, int id, int count)
{
  for (int i = 0; i < count; i++)
  {
    InodeHandle* handle = ih->data->node_cache[id + i].node_handle;
    if (handle)
    {
      IInspectable release{nullptr};
      winrt::attach_abi(release, handle);
    }
  }
}

static TreeViewNode winuiTreeGetNode(Ihandle* ih, int id)
{
  InodeHandle* handle = iupTreeGetNode(ih, id);
  if (!handle)
    return nullptr;

  TreeViewNode node{nullptr};
  winrt::copy_from_abi(node, handle);
  return node;
}

static IVector<TreeViewNode> winuiTreeGetSiblings(Ihandle* ih, TreeViewNode const& node)
{
  TreeViewNode parent = node.Parent();
  if (parent)
    return parent.Children();
  return winuiTreeGetTreeView(ih).RootNodes();
}

static void winuiTreeSetNodeProperty(TreeViewNode const& node, hstring const& key, IInspectable const& value)
{
  auto oldPs = node.Content().try_as<Windows::Foundation::Collections::PropertySet>();
  if (!oldPs)
    return;

  Windows::Foundation::Collections::PropertySet newPs;
  for (auto const& pair : oldPs)
    newPs.Insert(pair.Key(), pair.Value());
  newPs.Insert(key, value);
  node.Content(newPs);
}

/****************************************************************************
 * Node Content Helpers
 ****************************************************************************/

static IInspectable winuiTreeCreateNodeContent(Ihandle* ih, const char* title, void* imghandle)
{
  Windows::Foundation::Collections::PropertySet ps;
  ps.Insert(L"Title", box_value(iupwinuiStringToHString(title ? title : "")));

  if (imghandle)
  {
    IInspectable obj{nullptr};
    winrt::copy_from_abi(obj, imghandle);
    ps.Insert(L"ImageSource", obj);
  }

  if (ih->data->show_toggle)
  {
    ps.Insert(L"ToggleChecked", box_value(false));
    ps.Insert(L"ToggleVisible", box_value(Visibility::Visible));
  }

  return ps;
}

static void winuiTreeSetNodeImage(TreeViewNode const& node, void* imghandle)
{
  auto content = node.Content();
  auto oldPs = content.try_as<Windows::Foundation::Collections::PropertySet>();
  if (!oldPs)
    return;

  Windows::Foundation::Collections::PropertySet newPs;
  if (oldPs.HasKey(L"Title"))
    newPs.Insert(L"Title", oldPs.Lookup(L"Title"));

  if (imghandle)
  {
    IInspectable obj{nullptr};
    winrt::copy_from_abi(obj, imghandle);
    newPs.Insert(L"ImageSource", obj);
  }

  node.Content(newPs);
}

/****************************************************************************
 * Item Template with Font
 ****************************************************************************/

static void winuiTreeApplyItemTemplate(Ihandle* ih, TreeView const& treeView)
{
  TextBlock tempTb;
  iupwinuiUpdateTextBlockFont(ih, tempTb);

  int fontSize = (int)tempTb.FontSize();
  hstring fontFamily = tempTb.FontFamily().Source();
  bool isBold = tempTb.FontWeight().Weight >= FontWeights::Bold().Weight;
  bool isItalic = tempTb.FontStyle() == winrt::Windows::UI::Text::FontStyle::Italic;

  std::wstring xaml =
    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
    L" xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
    L"<StackPanel Orientation='Horizontal' Spacing='4'>";

  if (ih->data->show_toggle)
  {
    xaml += L"<CheckBox IsChecked='{Binding Content[ToggleChecked]}'"
            L" Visibility='{Binding Content[ToggleVisible], FallbackValue=Visible}'"
            L" MinWidth='0' Padding='0' VerticalAlignment='Center'";
    if (ih->data->show_toggle == 2)
      xaml += L" IsThreeState='True'";
    xaml += L"/>";
  }

  xaml += L"<Image Source='{Binding Content[ImageSource]}' Stretch='None' VerticalAlignment='Center'/>"
          L"<TextBlock Text='{Binding Content[Title]}' VerticalAlignment='Center'";

  unsigned char fr, fg, fb;
  const char* fgcolor = iupAttribGetStr(ih, "FGCOLOR");
  if (!fgcolor)
    fgcolor = IupGetGlobal("TXTFGCOLOR");
  if (iupStrToRGB(fgcolor, &fr, &fg, &fb))
  {
    wchar_t hexcolor[10];
    swprintf(hexcolor, 10, L"#%02X%02X%02X", fr, fg, fb);
    xaml += L" Foreground='{Binding Content[Color], FallbackValue=";
    xaml += hexcolor;
    xaml += L"}'";
  }
  else
  {
    xaml += L" Foreground='{Binding Content[Color]}'";
  }

  xaml += L" FontSize='{Binding Content[NodeFontSize], FallbackValue=";
  xaml += std::to_wstring(fontSize);
  xaml += L"}'";

  xaml += L" FontFamily='{Binding Content[NodeFontFamily], FallbackValue=";
  xaml += std::wstring(fontFamily);
  xaml += L"}'";

  xaml += L" FontWeight='{Binding Content[NodeFontWeight], FallbackValue=";
  xaml += isBold ? L"Bold" : L"Normal";
  xaml += L"}'";

  xaml += L" FontStyle='{Binding Content[NodeFontStyle], FallbackValue=";
  xaml += isItalic ? L"Italic" : L"Normal";
  xaml += L"}'";

  xaml += L"/></StackPanel></DataTemplate>";

  auto itemTemplate = XamlReader::Load(hstring(xaml)).as<DataTemplate>();
  treeView.ItemTemplate(itemTemplate);
}

/****************************************************************************
 * Image Update Helpers
 ****************************************************************************/

static void winuiTreeUpdateImages(Ihandle* ih, int mode)
{
  for (int i = 0; i < ih->data->node_count; i++)
  {
    TreeViewNode node = winuiTreeGetNode(ih, i);
    if (!node)
      continue;

    bool isBranch = node.HasChildren() || node.HasUnrealizedChildren();

    if (isBranch)
    {
      if (node.IsExpanded())
      {
        if (mode == ITREE_UPDATEIMAGE_EXPANDED)
          winuiTreeSetNodeImage(node, ih->data->def_image_expanded);
      }
      else
      {
        if (mode == ITREE_UPDATEIMAGE_COLLAPSED)
          winuiTreeSetNodeImage(node, ih->data->def_image_collapsed);
      }
    }
    else
    {
      if (mode == ITREE_UPDATEIMAGE_LEAF)
        winuiTreeSetNodeImage(node, ih->data->def_image_leaf);
    }
  }
}

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

static void winuiTreeExpandingHandler(Ihandle* ih, TreeViewNode const& node)
{
  if (node.HasUnrealizedChildren())
    node.HasUnrealizedChildren(false);

  int id = winuiTreeFindNodeId(ih, node);
  if (id < 0)
    return;

  IFni cb = (IFni)IupGetCallback(ih, "BRANCHOPEN_CB");
  if (cb)
  {
    int ret = cb(ih, id);
    if (ret == IUP_IGNORE)
    {
      node.IsExpanded(false);
      return;
    }
  }

  char* expanded_img = iupAttribGetId(ih, "_IUPWINUI_TREE_IMGEXP", id);
  if (expanded_img)
  {
    void* imghandle = iupImageGetImage(expanded_img, ih, 0, NULL);
    if (imghandle)
      winuiTreeSetNodeImage(node, imghandle);
  }
  else if (ih->data->def_image_expanded)
    winuiTreeSetNodeImage(node, ih->data->def_image_expanded);
}

static void winuiTreeCollapsedHandler(Ihandle* ih, TreeViewNode const& node)
{
  int id = winuiTreeFindNodeId(ih, node);
  if (id < 0)
    return;

  IFni cb = (IFni)IupGetCallback(ih, "BRANCHCLOSE_CB");
  if (cb)
    cb(ih, id);

  if (ih->data->def_image_collapsed)
    winuiTreeSetNodeImage(node, ih->data->def_image_collapsed);
}

static void winuiTreeItemInvokedHandler(Ihandle* ih, TreeViewNode const& node)
{
  if (!node)
    return;

  int id = winuiTreeFindNodeId(ih, node);
  if (id < 0)
    return;

  bool isBranch = node.HasChildren() || node.HasUnrealizedChildren();

  if (isBranch)
  {
    IFni cb = (IFni)IupGetCallback(ih, "EXECUTEBRANCH_CB");
    if (cb)
      cb(ih, id);
  }
  else
  {
    IFni cb = (IFni)IupGetCallback(ih, "EXECUTELEAF_CB");
    if (cb)
      cb(ih, id);
  }
}

static void winuiTreeSelectionChangedHandler(Ihandle* ih)
{
  IupWinUITreeAux* aux = winuiGetAux<IupWinUITreeAux>(ih, IUPWINUI_TREE_AUX);
  if (!aux)
    return;

  if (aux->ignoreChange)
    return;

  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return;

  IFnii cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  if (!cb)
    return;

  auto selectedNodes = treeView.SelectedNodes();
  if (selectedNodes.Size() > 0)
  {
    TreeViewNode node = selectedNodes.GetAt(0);
    int id = winuiTreeFindNodeId(ih, node);
    if (id >= 0)
      cb(ih, id, 1);
  }
}

/****************************************************************************
 * Navigation Attribute Handlers
 ****************************************************************************/

static char* winuiTreeGetDepthAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return NULL;

  int depth = 0;
  TreeViewNode parent = node.Parent();
  while (parent)
  {
    depth++;
    parent = parent.Parent();
  }

  return iupStrReturnInt(depth);
}

static char* winuiTreeGetParentAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return NULL;

  TreeViewNode parent = node.Parent();
  if (!parent)
    return NULL;

  int parentId = winuiTreeFindNodeId(ih, parent);
  if (parentId < 0)
    return NULL;

  return iupStrReturnInt(parentId);
}

static char* winuiTreeGetNextAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return NULL;

  auto siblings = winuiTreeGetSiblings(ih, node);
  uint32_t index;
  if (!siblings.IndexOf(node, index))
    return NULL;

  if (index + 1 >= siblings.Size())
    return NULL;

  TreeViewNode next = siblings.GetAt(index + 1);
  int nextId = winuiTreeFindNodeId(ih, next);
  if (nextId < 0)
    return NULL;

  return iupStrReturnInt(nextId);
}

static char* winuiTreeGetPreviousAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return NULL;

  auto siblings = winuiTreeGetSiblings(ih, node);
  uint32_t index;
  if (!siblings.IndexOf(node, index))
    return NULL;

  if (index == 0)
    return NULL;

  TreeViewNode prev = siblings.GetAt(index - 1);
  int prevId = winuiTreeFindNodeId(ih, prev);
  if (prevId < 0)
    return NULL;

  return iupStrReturnInt(prevId);
}

static char* winuiTreeGetFirstAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return NULL;

  auto siblings = winuiTreeGetSiblings(ih, node);
  if (siblings.Size() == 0)
    return NULL;

  TreeViewNode first = siblings.GetAt(0);
  int firstId = winuiTreeFindNodeId(ih, first);
  if (firstId < 0)
    return NULL;

  return iupStrReturnInt(firstId);
}

static char* winuiTreeGetLastAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return NULL;

  auto siblings = winuiTreeGetSiblings(ih, node);
  if (siblings.Size() == 0)
    return NULL;

  TreeViewNode last = siblings.GetAt(siblings.Size() - 1);
  int lastId = winuiTreeFindNodeId(ih, last);
  if (lastId < 0)
    return NULL;

  return iupStrReturnInt(lastId);
}

static char* winuiTreeGetRootCountAttrib(Ihandle* ih)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return NULL;

  return iupStrReturnInt((int)treeView.RootNodes().Size());
}

/****************************************************************************
 * EXPANDALL / TOPITEM
 ****************************************************************************/

static void winuiTreeExpandAllRec(TreeViewNode const& node, bool expand)
{
  auto children = node.Children();
  for (uint32_t i = 0; i < children.Size(); i++)
  {
    TreeViewNode child = children.GetAt(i);
    if (child.HasChildren() || child.HasUnrealizedChildren())
    {
      if (expand)
      {
        child.IsExpanded(true);
        winuiTreeExpandAllRec(child, expand);
      }
      else
      {
        winuiTreeExpandAllRec(child, expand);
        child.IsExpanded(false);
      }
    }
  }
}

static int winuiTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return 0;

  bool expand = iupStrBoolean(value) ? true : false;

  auto rootNodes = treeView.RootNodes();
  for (uint32_t i = 0; i < rootNodes.Size(); i++)
  {
    TreeViewNode root = rootNodes.GetAt(i);
    if (root.HasChildren() || root.HasUnrealizedChildren())
    {
      if (expand)
      {
        root.IsExpanded(true);
        winuiTreeExpandAllRec(root, expand);
      }
      else
      {
        winuiTreeExpandAllRec(root, expand);
        root.IsExpanded(false);
      }
    }
  }

  return 0;
}

static int winuiTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return 0;

  int id;
  if (!iupStrToInt(value, &id))
    return 0;

  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return 0;

  TreeViewNode parent = node.Parent();
  while (parent)
  {
    parent.IsExpanded(true);
    parent = parent.Parent();
  }

  DependencyObject container = treeView.ContainerFromNode(node);
  if (container)
  {
    UIElement elem = container.try_as<UIElement>();
    if (elem)
      elem.StartBringIntoView();
  }

  return 0;
}

/****************************************************************************
 * Attribute Handlers
 ****************************************************************************/

static int winuiTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return 0;

  IupWinUITreeAux* aux = winuiGetAux<IupWinUITreeAux>(ih, IUPWINUI_TREE_AUX);
  if (aux)
    aux->ignoreChange = true;

  if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
  {
    if (treeView.RootNodes().Size() > 0)
    {
      treeView.SelectedNodes().Clear();
      treeView.SelectedNodes().Append(treeView.RootNodes().GetAt(0));
    }
  }
  else if (iupStrEqualNoCase(value, "LAST"))
  {
    int count = ih->data->node_count;
    if (count > 0)
    {
      TreeViewNode node = winuiTreeGetNode(ih, count - 1);
      if (node)
      {
        treeView.SelectedNodes().Clear();
        treeView.SelectedNodes().Append(node);
      }
    }
  }
  else if (iupStrEqualNoCase(value, "CLEARALL"))
  {
    treeView.SelectedNodes().Clear();
  }
  else
  {
    int id;
    if (iupStrToInt(value, &id))
    {
      TreeViewNode node = winuiTreeGetNode(ih, id);
      if (node)
      {
        treeView.SelectedNodes().Clear();
        treeView.SelectedNodes().Append(node);
      }
    }
  }

  if (aux)
    aux->ignoreChange = false;

  return 0;
}

static char* winuiTreeGetValueAttrib(Ihandle* ih)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return NULL;

  auto selectedNodes = treeView.SelectedNodes();
  if (selectedNodes.Size() > 0)
  {
    TreeViewNode node = selectedNodes.GetAt(0);
    int id = winuiTreeFindNodeId(ih, node);
    if (id >= 0)
      return iupStrReturnInt(id);
  }

  return NULL;
}

static int winuiTreeSetTitleAttrib(Ihandle* ih, int id, const char* value)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (node)
  {
    auto oldPs = node.Content().try_as<Windows::Foundation::Collections::PropertySet>();
    if (oldPs)
    {
      Windows::Foundation::Collections::PropertySet newPs;
      newPs.Insert(L"Title", box_value(iupwinuiStringToHString(value)));
      if (oldPs.HasKey(L"ImageSource"))
        newPs.Insert(L"ImageSource", oldPs.Lookup(L"ImageSource"));
      node.Content(newPs);
    }
  }
  return 0;
}

static char* winuiTreeGetTitleAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (node)
  {
    auto ps = node.Content().try_as<Windows::Foundation::Collections::PropertySet>();
    if (ps && ps.HasKey(L"Title"))
      return iupwinuiHStringToString(unbox_value<hstring>(ps.Lookup(L"Title")));
  }
  return NULL;
}

static int winuiTreeSetStateAttrib(Ihandle* ih, int id, const char* value)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (node && (node.HasChildren() || node.HasUnrealizedChildren()))
  {
    if (iupStrEqualNoCase(value, "EXPANDED"))
      node.IsExpanded(true);
    else
      node.IsExpanded(false);
  }
  return 0;
}

static char* winuiTreeGetStateAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (node && (node.HasChildren() || node.HasUnrealizedChildren()))
  {
    if (node.IsExpanded())
      return (char*)"EXPANDED";
    else
      return (char*)"COLLAPSED";
  }
  return NULL;
}

static char* winuiTreeGetKindAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (node)
  {
    if (node.HasChildren() || node.HasUnrealizedChildren())
      return (char*)"BRANCH";
    else
      return (char*)"LEAF";
  }
  return NULL;
}

static char* winuiTreeGetChildCountAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (node)
    return iupStrReturnInt((int)node.Children().Size());
  return NULL;
}

static int winuiTreeSetMarkedAttrib(Ihandle* ih, int id, const char* value)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return 0;

  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return 0;

  IupWinUITreeAux* aux = winuiGetAux<IupWinUITreeAux>(ih, IUPWINUI_TREE_AUX);
  if (aux)
    aux->ignoreChange = true;

  if (iupStrBoolean(value))
  {
    uint32_t index;
    if (!treeView.SelectedNodes().IndexOf(node, index))
      treeView.SelectedNodes().Append(node);
  }
  else
  {
    uint32_t index;
    if (treeView.SelectedNodes().IndexOf(node, index))
      treeView.SelectedNodes().RemoveAt(index);
  }

  if (aux)
    aux->ignoreChange = false;

  return 0;
}

static char* winuiTreeGetMarkedAttrib(Ihandle* ih, int id)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return NULL;

  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return NULL;

  uint32_t index;
  if (treeView.SelectedNodes().IndexOf(node, index))
    return (char*)"YES";
  else
    return (char*)"NO";
}

static int winuiTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  int id;
  if (!iupStrToInt(value, &id))
    return 0;

  iupAttribSetInt(ih, "_IUPTREE_MARKSTART_NODE", id);
  return 1;
}

static void winuiTreeSelectNode(Ihandle* ih, TreeView const& treeView, TreeViewNode const& node, int select)
{
  uint32_t index;
  bool found = treeView.SelectedNodes().IndexOf(node, index);

  if (select == -1)
  {
    if (found)
      treeView.SelectedNodes().RemoveAt(index);
    else
      treeView.SelectedNodes().Append(node);
  }
  else if (select)
  {
    if (!found)
      treeView.SelectedNodes().Append(node);
  }
  else
  {
    if (found)
      treeView.SelectedNodes().RemoveAt(index);
  }
}

static void winuiTreeSelectRange(Ihandle* ih, TreeView const& treeView, int id1, int id2)
{
  int start = id1 < id2 ? id1 : id2;
  int end = id1 > id2 ? id1 : id2;

  for (int i = start; i <= end; i++)
  {
    TreeViewNode node = winuiTreeGetNode(ih, i);
    if (node)
      winuiTreeSelectNode(ih, treeView, node, 1);
  }
}

static int winuiTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
    return 0;

  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return 0;

  IupWinUITreeAux* aux = winuiGetAux<IupWinUITreeAux>(ih, IUPWINUI_TREE_AUX);
  if (aux)
    aux->ignoreChange = true;

  if (iupStrEqualNoCase(value, "BLOCK"))
  {
    int markStartId = iupAttribGetInt(ih, "_IUPTREE_MARKSTART_NODE");
    char* valueStr = IupGetAttribute(ih, "VALUE");
    int focusId = 0;
    if (valueStr)
      iupStrToInt(valueStr, &focusId);
    winuiTreeSelectRange(ih, treeView, markStartId, focusId);
  }
  else if (iupStrEqualNoCase(value, "CLEARALL"))
  {
    treeView.SelectedNodes().Clear();
  }
  else if (iupStrEqualNoCase(value, "MARKALL"))
  {
    for (int i = 0; i < ih->data->node_count; i++)
    {
      TreeViewNode node = winuiTreeGetNode(ih, i);
      if (node)
        winuiTreeSelectNode(ih, treeView, node, 1);
    }
  }
  else if (iupStrEqualNoCase(value, "INVERTALL"))
  {
    for (int i = 0; i < ih->data->node_count; i++)
    {
      TreeViewNode node = winuiTreeGetNode(ih, i);
      if (node)
        winuiTreeSelectNode(ih, treeView, node, -1);
    }
  }
  else if (iupStrEqualPartial(value, "INVERT"))
  {
    InodeHandle* nodeHandle = iupTreeGetNodeFromString(ih, &value[strlen("INVERT")]);
    if (nodeHandle)
    {
      TreeViewNode node{nullptr};
      winrt::copy_from_abi(node, nodeHandle);
      if (node)
        winuiTreeSelectNode(ih, treeView, node, -1);
    }
  }
  else
  {
    char str1[50], str2[50];
    if (iupStrToStrStr(value, str1, str2, '-') == 2)
    {
      int id1 = 0, id2 = 0;
      iupStrToInt(str1, &id1);
      iupStrToInt(str2, &id2);
      winuiTreeSelectRange(ih, treeView, id1, id2);
    }
  }

  if (aux)
    aux->ignoreChange = false;

  return 1;
}

static char* winuiTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return NULL;

  int count = ih->data->node_count;
  char* str = iupStrGetMemory(count + 1);

  auto selectedNodes = treeView.SelectedNodes();

  for (int i = 0; i < count; i++)
  {
    TreeViewNode node = winuiTreeGetNode(ih, i);
    if (node)
    {
      uint32_t index;
      str[i] = selectedNodes.IndexOf(node, index) ? '+' : '-';
    }
    else
      str[i] = '-';
  }

  str[count] = 0;
  return str;
}

static int winuiTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->mark_mode == ITREE_MARK_SINGLE || !value)
    return 0;

  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return 0;

  IupWinUITreeAux* aux = winuiGetAux<IupWinUITreeAux>(ih, IUPWINUI_TREE_AUX);
  if (aux)
    aux->ignoreChange = true;

  int count = ih->data->node_count;
  int len = (int)strlen(value);

  for (int i = 0; i < count && i < len; i++)
  {
    TreeViewNode node = winuiTreeGetNode(ih, i);
    if (node)
      winuiTreeSelectNode(ih, treeView, node, value[i] == '+' ? 1 : 0);
  }

  if (aux)
    aux->ignoreChange = false;

  return 0;
}

static int winuiTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return 0;

  if (iupStrEqualNoCase(value, "ALL"))
  {
    int old_count = ih->data->node_count;
    treeView.RootNodes().Clear();
    winuiTreeReleaseCacheNodes(ih, 0, old_count);
    ih->data->node_count = 0;
    iupTreeDelFromCache(ih, 0, old_count);
    return 0;
  }

  if (iupStrEqualNoCase(value, "CHILDREN"))
  {
    TreeViewNode node = winuiTreeGetNode(ih, id);
    if (node)
    {
      int childCount = iupdrvTreeTotalChildCount(ih, (InodeHandle*)winrt::get_abi(node));
      node.Children().Clear();
      node.HasUnrealizedChildren(true);
      if (childCount > 0)
      {
        winuiTreeReleaseCacheNodes(ih, id + 1, childCount);
        ih->data->node_count -= childCount;
        iupTreeDelFromCache(ih, id + 1, childCount);
      }
    }
    return 0;
  }

  if (iupStrEqualNoCase(value, "MARKED"))
  {
    auto selectedNodes = treeView.SelectedNodes();
    while (selectedNodes.Size() > 0)
    {
      TreeViewNode node = selectedNodes.GetAt(0);
      int nodeId = winuiTreeFindNodeId(ih, node);
      if (nodeId >= 0)
      {
        int count = iupdrvTreeTotalChildCount(ih, (InodeHandle*)winrt::get_abi(node)) + 1;

        TreeViewNode parent = node.Parent();
        if (parent)
        {
          uint32_t idx;
          if (parent.Children().IndexOf(node, idx))
            parent.Children().RemoveAt(idx);
        }
        else
        {
          uint32_t idx;
          if (treeView.RootNodes().IndexOf(node, idx))
            treeView.RootNodes().RemoveAt(idx);
        }

        winuiTreeReleaseCacheNodes(ih, nodeId, count);
        ih->data->node_count -= count;
        iupTreeDelFromCache(ih, nodeId, count);
      }
      else
      {
        break;
      }
    }
    return 0;
  }

  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (node)
  {
    int count = iupdrvTreeTotalChildCount(ih, (InodeHandle*)winrt::get_abi(node)) + 1;

    TreeViewNode parent = node.Parent();
    if (parent)
    {
      uint32_t idx;
      if (parent.Children().IndexOf(node, idx))
        parent.Children().RemoveAt(idx);
    }
    else
    {
      uint32_t idx;
      if (treeView.RootNodes().IndexOf(node, idx))
        treeView.RootNodes().RemoveAt(idx);
    }

    winuiTreeReleaseCacheNodes(ih, id, count);
    ih->data->node_count -= count;
    iupTreeDelFromCache(ih, id, count);
  }

  return 0;
}

/****************************************************************************
 * Rename (In-Place Editing)
 ****************************************************************************/

static bool winuiTreeFindTitleControls(DependencyObject const& parent, Panel& outPanel, TextBlock& outTextBlock)
{
  int count = Media::VisualTreeHelper::GetChildrenCount(parent);
  for (int i = 0; i < count; i++)
  {
    auto child = Media::VisualTreeHelper::GetChild(parent, i);
    auto panel = child.try_as<StackPanel>();
    if (panel && panel.Orientation() == Orientation::Horizontal)
    {
      for (uint32_t j = 0; j < panel.Children().Size(); j++)
      {
        auto tb = panel.Children().GetAt(j).try_as<TextBlock>();
        if (tb)
        {
          outPanel = panel;
          outTextBlock = tb;
          return true;
        }
      }
    }
    if (winuiTreeFindTitleControls(child, outPanel, outTextBlock))
      return true;
  }
  return false;
}

static void winuiTreeFinishRenameEditing(Ihandle* ih, TextBox const& editBox, bool commit)
{
  int id = iupAttribGetInt(ih, "_IUPWINUI_TREE_RENAME_ID");

  if (commit)
  {
    hstring newText = editBox.Text();
    char* newTitle = iupwinuiHStringToString(newText);

    IFnis cbRename = (IFnis)IupGetCallback(ih, "RENAME_CB");
    if (cbRename)
    {
      if (cbRename(ih, id, newTitle) != IUP_IGNORE)
        IupSetAttributeId(ih, "TITLE", id, newTitle);
    }
    else
      IupSetAttributeId(ih, "TITLE", id, newTitle);
  }

  void* tbPtr = (void*)iupAttribGet(ih, "_IUPWINUI_TREE_RENAME_TEXTBLOCK");
  if (tbPtr)
  {
    TextBlock titleBlock{nullptr};
    winrt::copy_from_abi(titleBlock, tbPtr);
    if (titleBlock)
      titleBlock.Visibility(Visibility::Visible);
    iupAttribSet(ih, "_IUPWINUI_TREE_RENAME_TEXTBLOCK", NULL);
  }

  auto parent = Media::VisualTreeHelper::GetParent(editBox);
  if (parent)
  {
    auto panel = parent.try_as<Panel>();
    if (panel)
    {
      uint32_t idx;
      if (panel.Children().IndexOf(editBox, idx))
        panel.Children().RemoveAt(idx);
    }
  }

  iupAttribSet(ih, "_IUPWINUI_TREE_RENAME_ID", NULL);

  TreeView treeView = winuiTreeGetTreeView(ih);
  if (treeView)
    treeView.Focus(FocusState::Programmatic);
}

static int winuiTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->show_rename)
    return 0;

  int id = IupGetInt(ih, "VALUE");
  if (id < 0)
    return 0;

  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return 0;

  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return 0;

  DependencyObject container = treeView.ContainerFromNode(node);
  if (!container)
    return 0;

  TreeViewItem tvi = container.try_as<TreeViewItem>();
  if (!tvi)
    return 0;

  IFni cbShow = (IFni)IupGetCallback(ih, "SHOWRENAME_CB");
  if (cbShow && cbShow(ih, id) == IUP_IGNORE)
    return 0;

  Panel titlePanel{nullptr};
  TextBlock titleBlock{nullptr};
  if (!winuiTreeFindTitleControls(tvi, titlePanel, titleBlock))
    return 0;

  char* currentTitle = IupGetAttributeId(ih, "TITLE", id);

  iupAttribSetInt(ih, "_IUPWINUI_TREE_RENAME_ID", id);

  uint32_t tbIndex;
  titlePanel.Children().IndexOf(titleBlock, tbIndex);

  titleBlock.Visibility(Visibility::Collapsed);

  void* tbPtr = nullptr;
  winrt::copy_to_abi(titleBlock, tbPtr);
  iupAttribSet(ih, "_IUPWINUI_TREE_RENAME_TEXTBLOCK", (char*)tbPtr);

  TextBox editBox;
  editBox.Text(iupwinuiStringToHString(currentTitle ? currentTitle : ""));
  iupwinuiUpdateControlFont(ih, editBox);
  editBox.MinWidth(titleBlock.ActualWidth() + 20);

  titlePanel.Children().InsertAt(tbIndex, editBox);
  editBox.Focus(FocusState::Programmatic);
  editBox.SelectAll();

  editBox.KeyDown([ih, editBox](IInspectable const&, KeyRoutedEventArgs const& args) {
    if (args.Key() == Windows::System::VirtualKey::Enter)
    {
      winuiTreeFinishRenameEditing(ih, editBox, true);
      args.Handled(true);
    }
    else if (args.Key() == Windows::System::VirtualKey::Escape)
    {
      winuiTreeFinishRenameEditing(ih, editBox, false);
      args.Handled(true);
    }
  });

  editBox.LostFocus([ih, editBox](IInspectable const&, RoutedEventArgs const&) {
    if (iupAttribGet(ih, "_IUPWINUI_TREE_RENAME_ID"))
      winuiTreeFinishRenameEditing(ih, editBox, true);
  });

  (void)value;
  return 0;
}

/****************************************************************************
 * Image Attribute Handlers
 ****************************************************************************/

static int winuiTreeSetImageAttrib(Ihandle* ih, int id, const char* value)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return 0;

  if (value)
  {
    void* imghandle = iupImageGetImage(value, ih, 0, NULL);
    if (imghandle)
      winuiTreeSetNodeImage(node, imghandle);
  }
  else
  {
    bool isBranch = node.HasChildren() || node.HasUnrealizedChildren();
    void* def_image;
    if (isBranch)
      def_image = node.IsExpanded() ? ih->data->def_image_expanded : ih->data->def_image_collapsed;
    else
      def_image = ih->data->def_image_leaf;
    winuiTreeSetNodeImage(node, def_image);
  }

  return 1;
}

static int winuiTreeSetImageExpandedAttrib(Ihandle* ih, int id, const char* value)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return 0;

  iupAttribSetStrId(ih, "_IUPWINUI_TREE_IMGEXP", id, value);

  if (node.IsExpanded() && value)
  {
    void* imghandle = iupImageGetImage(value, ih, 0, NULL);
    if (imghandle)
      winuiTreeSetNodeImage(node, imghandle);
  }

  return 1;
}

static int winuiTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_leaf = iupImageGetImage(value, ih, 0, NULL);
  winuiTreeUpdateImages(ih, ITREE_UPDATEIMAGE_LEAF);
  return 1;
}

static int winuiTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_collapsed = iupImageGetImage(value, ih, 0, NULL);
  winuiTreeUpdateImages(ih, ITREE_UPDATEIMAGE_COLLAPSED);
  return 1;
}

static int winuiTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  ih->data->def_image_expanded = iupImageGetImage(value, ih, 0, NULL);
  winuiTreeUpdateImages(ih, ITREE_UPDATEIMAGE_EXPANDED);
  return 1;
}

/****************************************************************************
 * Per-Node Styling (COLOR, TITLEFONT)
 ****************************************************************************/

static int winuiTreeSetColorAttrib(Ihandle* ih, int id, const char* value)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return 0;

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    Windows::UI::Color color;
    color.A = 255;
    color.R = r;
    color.G = g;
    color.B = b;
    Media::SolidColorBrush brush(color);
    winuiTreeSetNodeProperty(node, L"Color", brush);
  }

  return 0;
}

static char* winuiTreeGetColorAttrib(Ihandle* ih, int id)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return NULL;

  auto ps = node.Content().try_as<Windows::Foundation::Collections::PropertySet>();
  if (!ps || !ps.HasKey(L"Color"))
    return NULL;

  auto brush = ps.Lookup(L"Color").try_as<Media::SolidColorBrush>();
  if (!brush)
    return NULL;

  auto color = brush.Color();
  return iupStrReturnRGB(color.R, color.G, color.B);
}

static int winuiTreeSetTitleFontAttrib(Ihandle* ih, int id, const char* value)
{
  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return 0;

  auto oldPs = node.Content().try_as<Windows::Foundation::Collections::PropertySet>();
  if (!oldPs)
    return 0;

  Windows::Foundation::Collections::PropertySet newPs;
  for (auto const& pair : oldPs)
  {
    auto key = pair.Key();
    if (key != L"NodeFontFamily" && key != L"NodeFontSize" &&
        key != L"NodeFontWeight" && key != L"NodeFontStyle")
      newPs.Insert(key, pair.Value());
  }

  if (value && value[0] != 0)
  {
    char typeface[1024];
    int size, is_bold, is_italic, is_underline, is_strikeout;
    if (!iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
      return 0;

    iupAttribSetStrId(ih, "_IUPWINUI_TREE_TITLEFONT", id, value);

    newPs.Insert(L"NodeFontFamily", box_value(iupwinuiStringToHString(typeface)));
    newPs.Insert(L"NodeFontSize", box_value((double)size));
    newPs.Insert(L"NodeFontWeight", box_value(iupwinuiStringToHString(is_bold ? "Bold" : "Normal")));
    newPs.Insert(L"NodeFontStyle", box_value(iupwinuiStringToHString(is_italic ? "Italic" : "Normal")));
  }
  else
    iupAttribSetStrId(ih, "_IUPWINUI_TREE_TITLEFONT", id, NULL);

  node.Content(newPs);
  return 0;
}

static char* winuiTreeGetTitleFontAttrib(Ihandle* ih, int id)
{
  return iupAttribGetId(ih, "_IUPWINUI_TREE_TITLEFONT", id);
}

/****************************************************************************
 * TOGGLEVALUE / TOGGLEVISIBLE
 ****************************************************************************/

static int winuiTreeSetToggleValueAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->data->show_toggle)
    return 0;

  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return 0;

  if (ih->data->show_toggle == 2 && iupStrEqualNoCase(value, "NOTDEF"))
  {
    auto ref = winrt::Windows::Foundation::IReference<bool>(nullptr);
    winuiTreeSetNodeProperty(node, L"ToggleChecked", ref);
  }
  else
  {
    bool checked = iupStrEqualNoCase(value, "ON") ? true : false;
    winuiTreeSetNodeProperty(node, L"ToggleChecked", box_value(checked));
  }

  return 0;
}

static char* winuiTreeGetToggleValueAttrib(Ihandle* ih, int id)
{
  if (!ih->data->show_toggle)
    return NULL;

  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return NULL;

  auto ps = node.Content().try_as<Windows::Foundation::Collections::PropertySet>();
  if (!ps || !ps.HasKey(L"ToggleChecked"))
    return NULL;

  auto val = ps.Lookup(L"ToggleChecked");
  if (!val)
    return iupStrReturnChecked(-1);

  auto ref = val.try_as<Windows::Foundation::IReference<bool>>();
  if (!ref)
    return iupStrReturnChecked(-1);

  return iupStrReturnChecked(ref.Value() ? 1 : 0);
}

static int winuiTreeSetToggleVisibleAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->data->show_toggle)
    return 0;

  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return 0;

  Visibility vis = iupStrBoolean(value) ? Visibility::Visible : Visibility::Collapsed;
  winuiTreeSetNodeProperty(node, L"ToggleVisible", box_value(vis));

  return 0;
}

static char* winuiTreeGetToggleVisibleAttrib(Ihandle* ih, int id)
{
  if (!ih->data->show_toggle)
    return NULL;

  TreeViewNode node = winuiTreeGetNode(ih, id);
  if (!node)
    return NULL;

  auto ps = node.Content().try_as<Windows::Foundation::Collections::PropertySet>();
  if (!ps || !ps.HasKey(L"ToggleVisible"))
    return (char*)"YES";

  auto val = ps.Lookup(L"ToggleVisible");
  auto ref = val.try_as<Windows::Foundation::IReference<Visibility>>();
  if (ref && ref.Value() == Visibility::Collapsed)
    return (char*)"NO";

  return (char*)"YES";
}

/****************************************************************************
 * BGCOLOR / FGCOLOR
 ****************************************************************************/

static int winuiTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  iupwinuiSetBgColor(ih->handle, r, g, b);
  return 1;
}

static int winuiTreeSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  TreeView treeView = winuiTreeGetTreeView(ih);
  if (treeView)
  {
    Windows::UI::Color color;
    color.A = 255;
    color.R = r;
    color.G = g;
    color.B = b;
    treeView.Foreground(Media::SolidColorBrush(color));
  }

  return 0;
}

void winuiTreeRefreshThemeColors(Ihandle* ih)
{
  winuiTreeSetBgColorAttrib(ih, IupGetGlobal("TXTBGCOLOR"));

  TreeView treeView = winuiTreeGetTreeView(ih);
  if (treeView)
    winuiTreeApplyItemTemplate(ih, treeView);
}

/****************************************************************************
 * INDENTATION / SPACING
 ****************************************************************************/

static int winuiTreeSetIndentationAttrib(Ihandle* ih, const char* value)
{
  int indent;
  if (!iupStrToInt(value, &indent))
    return 0;

  TreeView treeView = winuiTreeGetTreeView(ih);
  if (treeView)
  {
    auto key = box_value(L"TreeViewItemIndentation");
    treeView.Resources().Insert(key, box_value((double)indent));
  }

  return 0;
}

static char* winuiTreeGetIndentationAttrib(Ihandle* ih)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return NULL;

  auto key = box_value(L"TreeViewItemIndentation");
  if (treeView.Resources().HasKey(key))
  {
    auto val = treeView.Resources().Lookup(key);
    auto ref = val.try_as<Windows::Foundation::IReference<double>>();
    if (ref)
      return iupStrReturnInt((int)ref.Value());
  }

  return NULL;
}

static void winuiTreeUpdateSpacingResources(Ihandle* ih, TreeView treeView)
{
  int spacing = ih->data->spacing;
  double minHeight = spacing > 0 ? (double)(spacing * 2) : 0.0;
  Thickness margin = {4, (double)spacing, 4, (double)spacing};
  Thickness padding = {0, 0, 0, 0};

  treeView.Resources().Insert(box_value(L"TreeViewItemMinHeight"), box_value(minHeight));
  treeView.Resources().Insert(box_value(L"TreeViewItemPresenterMargin"), box_value(margin));
  treeView.Resources().Insert(box_value(L"TreeViewItemPresenterPadding"), box_value(padding));
}

static int winuiTreeSetSpacingAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrToInt(value, &ih->data->spacing))
    return 1;

  if (ih->handle)
  {
    TreeView treeView = winuiTreeGetTreeView(ih);
    if (treeView)
      winuiTreeUpdateSpacingResources(ih, treeView);
    return 0;
  }

  return 1;
}

/****************************************************************************
 * Font Attribute Handler
 ****************************************************************************/

static int winuiTreeSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  TreeView treeView = winuiGetHandle<TreeView>(ih);
  if (treeView)
    winuiTreeApplyItemTemplate(ih, treeView);

  return 1;
}

static int winuiTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return -1;

  Point pt{(float)x, (float)y};
  auto elements = Media::VisualTreeHelper::FindElementsInHostCoordinates(pt, treeView);

  for (auto const& elem : elements)
  {
    auto tvi = elem.try_as<Controls::TreeViewItem>();
    if (tvi)
    {
      TreeViewNode node = treeView.NodeFromContainer(tvi);
      if (node)
        return winuiTreeFindNodeId(ih, node);
    }
  }

  return -1;
}

/****************************************************************************
 * WinUI Default Images (modern style)
 ****************************************************************************/

#define ITREE_IMG_WIDTH   16
#define ITREE_IMG_HEIGHT  16

#define OO  0,0,0,0
#define FE  198,138,22,255
#define FH  255,218,90,255
#define FB  255,193,7,255
#define FM  255,179,0,255
#define FD  245,163,0,255
#define FI  232,200,96,255
#define FF  255,213,79,255
#define GE  176,176,176,255
#define GW  255,255,255,255
#define GF  0,120,212,255
#define GL  91,168,224,255
#define GT  208,208,208,255

static unsigned char winui_img_leaf[ITREE_IMG_WIDTH * ITREE_IMG_HEIGHT * 4] =
{
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, GE, GE, GE, GE, GE, GE, GE, GE, GE, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GL, GL, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GL, GF, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GE, GE, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GE, GE, GE, GE, GE, GE, GE, GE, GE, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO
};

static unsigned char winui_img_collapsed[ITREE_IMG_WIDTH * ITREE_IMG_HEIGHT * 4] =
{
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, FE, FE, FE, FE, FE, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, FE, FH, FH, FH, FH, FH, FE, OO, OO, OO, OO, OO, OO, OO, OO,
  FE, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FE, OO, OO,
  FE, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FE, OO, OO,
  FE, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FE, OO, OO,
  FE, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FE, OO, OO,
  FE, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FE, OO, OO,
  FE, FM, FM, FM, FM, FM, FM, FM, FM, FM, FM, FM, FM, FE, OO, OO,
  FE, FM, FM, FM, FM, FM, FM, FM, FM, FM, FM, FM, FM, FE, OO, OO,
  FE, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FE, OO, OO,
  FE, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FE, OO, OO,
  FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO
};

static unsigned char winui_img_expanded[ITREE_IMG_WIDTH * ITREE_IMG_HEIGHT * 4] =
{
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, FE, FE, FE, FE, FE, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, FE, FH, FH, FH, FH, FH, FE, OO, OO, OO, OO, OO, OO, OO, OO,
  FE, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FH, FE, OO, OO,
  FE, FI, FI, FI, FI, FI, FI, FI, FI, FI, FI, FI, FI, FE, OO, OO,
  FE, FI, FI, FI, FI, FI, FI, FI, FI, FI, FI, FI, FI, FE, OO, OO,
  FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, OO,
  FE, FF, FF, FF, FF, FF, FF, FF, FF, FF, FF, FF, FF, FF, FE, OO,
  FE, FF, FF, FF, FF, FF, FF, FF, FF, FF, FF, FF, FF, FF, FE, OO,
  FE, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FB, FE, OO,
  FE, FM, FM, FM, FM, FM, FM, FM, FM, FM, FM, FM, FM, FM, FE, OO,
  FE, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FD, FE, OO,
  FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, FE, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO
};

static unsigned char winui_img_blank[ITREE_IMG_WIDTH * ITREE_IMG_HEIGHT * 4] =
{
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, GE, GE, GE, GE, GE, GE, GE, GE, GE, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GE, GE, GE, GE, GE, GE, GE, GE, GE, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO
};

static unsigned char winui_img_paper[ITREE_IMG_WIDTH * ITREE_IMG_HEIGHT * 4] =
{
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, GE, GE, GE, GE, GE, GE, GE, GE, GE, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GL, GL, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GL, GF, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GE, GE, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GT, GT, GT, GT, GT, GT, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GT, GT, GT, GT, GT, GT, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GT, GT, GT, GT, GT, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GW, GW, GW, GW, GW, GW, GW, GW, GE, OO, OO, OO,
  OO, OO, OO, GE, GE, GE, GE, GE, GE, GE, GE, GE, GE, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO,
  OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO, OO
};

#undef OO
#undef FE
#undef FH
#undef FB
#undef FM
#undef FD
#undef FI
#undef FF
#undef GE
#undef GW
#undef GF
#undef GL
#undef GT

static void winuiTreeInitializeImages(void)
{
  if (IupGetHandle("IMGLEAF_WINUI"))
    return;

  Ihandle* image_leaf = IupImageRGBA(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, winui_img_leaf);
  Ihandle* image_collapsed = IupImageRGBA(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, winui_img_collapsed);
  Ihandle* image_expanded = IupImageRGBA(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, winui_img_expanded);
  Ihandle* image_blank = IupImageRGBA(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, winui_img_blank);
  Ihandle* image_paper = IupImageRGBA(ITREE_IMG_WIDTH, ITREE_IMG_HEIGHT, winui_img_paper);

  IupSetHandle("IMGLEAF_WINUI", image_leaf);
  IupSetHandle("IMGCOLLAPSED_WINUI", image_collapsed);
  IupSetHandle("IMGEXPANDED_WINUI", image_expanded);
  IupSetHandle("IMGBLANK_WINUI", image_blank);
  IupSetHandle("IMGPAPER_WINUI", image_paper);
}

#undef ITREE_IMG_WIDTH
#undef ITREE_IMG_HEIGHT

static void winuiTreeInitDefaultImages(Ihandle* ih)
{
  char* img_name;

  img_name = iupAttribGetStr(ih, "IMAGELEAF");
  if (img_name && !iupStrEqualNoCase(img_name, "IMGLEAF"))
    ih->data->def_image_leaf = iupImageGetImage(img_name, ih, 0, NULL);
  else
    ih->data->def_image_leaf = iupImageGetImage("IMGLEAF_WINUI", ih, 0, NULL);

  img_name = iupAttribGetStr(ih, "IMAGEBRANCHCOLLAPSED");
  if (img_name && !iupStrEqualNoCase(img_name, "IMGCOLLAPSED"))
    ih->data->def_image_collapsed = iupImageGetImage(img_name, ih, 0, NULL);
  else
    ih->data->def_image_collapsed = iupImageGetImage("IMGCOLLAPSED_WINUI", ih, 0, NULL);

  img_name = iupAttribGetStr(ih, "IMAGEBRANCHEXPANDED");
  if (img_name && !iupStrEqualNoCase(img_name, "IMGEXPANDED"))
    ih->data->def_image_expanded = iupImageGetImage(img_name, ih, 0, NULL);
  else
    ih->data->def_image_expanded = iupImageGetImage("IMGEXPANDED_WINUI", ih, 0, NULL);
}

/****************************************************************************
 * Map / UnMap
 ****************************************************************************/

static int winuiTreeMapMethod(Ihandle* ih)
{
  IupWinUITreeAux* aux = new IupWinUITreeAux();

  TreeView treeView;
  treeView.HorizontalAlignment(HorizontalAlignment::Left);
  treeView.VerticalAlignment(VerticalAlignment::Top);

  if (ih->data->mark_mode == ITREE_MARK_MULTIPLE)
    treeView.SelectionMode(TreeViewSelectionMode::Multiple);
  else
    treeView.SelectionMode(TreeViewSelectionMode::Single);

  winuiTreeUpdateSpacingResources(ih, treeView);

  aux->expandingToken = treeView.Expanding([ih](TreeView const&, TreeViewExpandingEventArgs const& args) {
    winuiTreeExpandingHandler(ih, args.Node());
  });

  aux->collapsedToken = treeView.Collapsed([ih](TreeView const&, TreeViewCollapsedEventArgs const& args) {
    winuiTreeCollapsedHandler(ih, args.Node());
  });

  aux->itemInvokedToken = treeView.ItemInvoked([ih](TreeView const&, TreeViewItemInvokedEventArgs const& args) {
    TreeViewNode node = args.InvokedItem().try_as<TreeViewNode>();
    winuiTreeItemInvokedHandler(ih, node);
  });

  aux->selectionChangedToken = treeView.SelectionChanged([ih](TreeView const&, TreeViewSelectionChangedEventArgs const&) {
    winuiTreeSelectionChangedHandler(ih);
  });

  aux->rightTappedToken = treeView.RightTapped([ih](IInspectable const&, RightTappedRoutedEventArgs const& args) {
    TreeView tv = winuiTreeGetTreeView(ih);
    if (tv)
    {
      DependencyObject source = args.OriginalSource().try_as<DependencyObject>();
      while (source)
      {
        auto tvi = source.try_as<Controls::TreeViewItem>();
        if (tvi)
        {
          TreeViewNode node = tv.NodeFromContainer(tvi);
          if (node)
          {
            int id = winuiTreeFindNodeId(ih, node);
            if (id >= 0)
            {
              IupWinUITreeAux* a = winuiGetAux<IupWinUITreeAux>(ih, IUPWINUI_TREE_AUX);
              if (a) a->ignoreChange = true;
              tv.SelectedNodes().Clear();
              tv.SelectedNodes().Append(node);
              if (a) a->ignoreChange = false;

              IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
              if (cb)
                cb(ih, id);
            }
          }
          break;
        }
        source = Media::VisualTreeHelper::GetParent(source);
      }
    }
    args.Handled(true);
  });

  aux->keyDownToken = treeView.KeyDown([ih](IInspectable const&, KeyRoutedEventArgs const& args) {
    int code = iupwinuiKeyDecode((int)args.Key());
    if (code)
    {
      if (code == K_F2 && ih->data->show_rename)
      {
        winuiTreeSetRenameAttrib(ih, NULL);
        args.Handled(true);
        return;
      }

      int ret = iupKeyCallKeyCb(ih, code);
      if (ret == IUP_CLOSE)
      {
        IupExitLoop();
        args.Handled(true);
        return;
      }
      if (ret == IUP_IGNORE)
      {
        args.Handled(true);
        return;
      }
    }
  });

  aux->doubleTappedToken = treeView.DoubleTapped([ih](IInspectable const&, DoubleTappedRoutedEventArgs const& args) {
    if (!ih->data->show_rename)
      return;

    TreeView tv = winuiTreeGetTreeView(ih);
    if (!tv)
      return;

    DependencyObject source = args.OriginalSource().try_as<DependencyObject>();
    while (source)
    {
      auto tvi = source.try_as<Controls::TreeViewItem>();
      if (tvi)
      {
        TreeViewNode node = tv.NodeFromContainer(tvi);
        if (node)
        {
          int id = winuiTreeFindNodeId(ih, node);
          if (id >= 0)
            winuiTreeSetRenameAttrib(ih, NULL);
        }
        break;
      }
      source = Media::VisualTreeHelper::GetParent(source);
    }
  });

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(treeView);

  winuiSetAux(ih, IUPWINUI_TREE_AUX, aux);

  winuiTreeApplyItemTemplate(ih, treeView);

  winuiStoreHandle(ih, treeView);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)winuiTreeConvertXYToPos);

  winuiTreeInitDefaultImages(ih);

  if (iupAttribGetInt(ih, "ADDROOT"))
    iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

  winuiTreeSetBgColorAttrib(ih, IupGetGlobal("TXTBGCOLOR"));

  return IUP_NOERROR;
}

static void winuiTreeUnMapMethod(Ihandle* ih)
{
  IupWinUITreeAux* aux = winuiGetAux<IupWinUITreeAux>(ih, IUPWINUI_TREE_AUX);

  if (ih->handle && aux)
  {
    TreeView treeView = winuiTreeGetTreeView(ih);
    if (treeView)
    {
      if (aux->dragItemsStartingToken)
        treeView.DragItemsStarting(aux->dragItemsStartingToken);
      if (aux->dragItemsCompletedToken)
        treeView.DragItemsCompleted(aux->dragItemsCompletedToken);

      event_token* token;
      token = (event_token*)iupAttribGet(ih, "_IUPWINUI_CUSTOMDRAGOVER_TOKEN");
      if (token) { treeView.DragOver(*token); delete token; iupAttribSet(ih, "_IUPWINUI_CUSTOMDRAGOVER_TOKEN", NULL); }
      token = (event_token*)iupAttribGet(ih, "_IUPWINUI_CUSTOMDROP_TOKEN");
      if (token) { treeView.Drop(*token); delete token; iupAttribSet(ih, "_IUPWINUI_CUSTOMDROP_TOKEN", NULL); }
      token = (event_token*)iupAttribGet(ih, "_IUPWINUI_DRAGOVER_TOKEN");
      if (token) { treeView.DragOver(*token); delete token; iupAttribSet(ih, "_IUPWINUI_DRAGOVER_TOKEN", NULL); }
      token = (event_token*)iupAttribGet(ih, "_IUPWINUI_DROP_TOKEN");
      if (token) { treeView.Drop(*token); delete token; iupAttribSet(ih, "_IUPWINUI_DROP_TOKEN", NULL); }

      if (aux->expandingToken)
        treeView.Expanding(aux->expandingToken);
      if (aux->collapsedToken)
        treeView.Collapsed(aux->collapsedToken);
      if (aux->itemInvokedToken)
        treeView.ItemInvoked(aux->itemInvokedToken);
      if (aux->selectionChangedToken)
        treeView.SelectionChanged(aux->selectionChangedToken);
      if (aux->rightTappedToken)
        treeView.RightTapped(aux->rightTappedToken);
      if (aux->keyDownToken)
        treeView.KeyDown(aux->keyDownToken);
      if (aux->doubleTappedToken)
        treeView.DoubleTapped(aux->doubleTappedToken);
    }

    void* tbPtr = (void*)iupAttribGet(ih, "_IUPWINUI_TREE_RENAME_TEXTBLOCK");
    if (tbPtr)
    {
      IInspectable release{nullptr};
      winrt::attach_abi(release, tbPtr);
      iupAttribSet(ih, "_IUPWINUI_TREE_RENAME_TEXTBLOCK", NULL);
      iupAttribSet(ih, "_IUPWINUI_TREE_RENAME_ID", NULL);
    }

    winuiTreeReleaseCacheNodes(ih, 0, ih->data->node_count);

    winuiReleaseHandle<TreeView>(ih);
  }

  winuiFreeAux<IupWinUITreeAux>(ih, IUPWINUI_TREE_AUX);
  ih->handle = NULL;
}

/****************************************************************************
 * Node Operations
 ****************************************************************************/

extern "C" void iupdrvTreeAddNode(Ihandle* ih, int id, int kind, const char* title, int add)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return;

  TreeViewNode prevNode{nullptr};
  int kindPrev = ITREE_LEAF;

  if (id >= 0)
  {
    prevNode = winuiTreeGetNode(ih, id);
    if (!prevNode)
      return;

    if (prevNode.HasChildren() || prevNode.HasUnrealizedChildren())
      kindPrev = ITREE_BRANCH;
  }

  TreeViewNode newNode;

  void* def_image = (kind == ITREE_BRANCH) ? ih->data->def_image_collapsed : ih->data->def_image_leaf;
  newNode.Content(winuiTreeCreateNodeContent(ih, title, def_image));

  if (kind == ITREE_BRANCH)
    newNode.HasUnrealizedChildren(true);

  if (!prevNode)
  {
    treeView.RootNodes().InsertAt(0, newNode);
  }
  else if (add && kindPrev == ITREE_BRANCH)
  {
    prevNode.Children().InsertAt(0, newNode);
    prevNode.HasUnrealizedChildren(false);
    if (ih->data->add_expanded)
      prevNode.IsExpanded(true);
  }
  else
  {
    TreeViewNode parent = prevNode.Parent();
    if (parent)
    {
      uint32_t index = 0;
      parent.Children().IndexOf(prevNode, index);
      parent.Children().InsertAt(index + 1, newNode);
    }
    else
    {
      uint32_t index = 0;
      treeView.RootNodes().IndexOf(prevNode, index);
      treeView.RootNodes().InsertAt(index + 1, newNode);
    }
  }

  void* nodePtr = nullptr;
  winrt::copy_to_abi(newNode, nodePtr);

  iupTreeAddToCache(ih, add, kindPrev,
                    prevNode ? (InodeHandle*)winrt::get_abi(prevNode) : NULL,
                    (InodeHandle*)nodePtr);
}

extern "C" InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return NULL;

  auto selectedNodes = treeView.SelectedNodes();
  if (selectedNodes.Size() > 0)
  {
    TreeViewNode node = selectedNodes.GetAt(0);
    return (InodeHandle*)winrt::get_abi(node);
  }

  return NULL;
}

static int winuiTreeCountChildrenRec(TreeViewNode const& node)
{
  int count = 0;
  auto children = node.Children();
  for (uint32_t i = 0; i < children.Size(); i++)
  {
    count++;
    count += winuiTreeCountChildrenRec(children.GetAt(i));
  }
  return count;
}

extern "C" int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
  (void)ih;

  if (!node_handle)
    return 0;

  TreeViewNode node{nullptr};
  winrt::copy_from_abi(node, node_handle);

  if (!node)
    return 0;

  return winuiTreeCountChildrenRec(node);
}

extern "C" void iupdrvTreeUpdateMarkMode(Ihandle* ih)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return;

  if (ih->data->mark_mode == ITREE_MARK_MULTIPLE)
    treeView.SelectionMode(TreeViewSelectionMode::Multiple);
  else
    treeView.SelectionMode(TreeViewSelectionMode::Single);
}

/****************************************************************************
 * Drag & Drop Node Copy
 ****************************************************************************/

static void winuiTreeChildRebuildCacheRec(Ihandle* ih, TreeViewNode const& node, int* id)
{
  auto children = node.Children();
  for (uint32_t i = 0; i < children.Size(); i++)
  {
    TreeViewNode child = children.GetAt(i);
    (*id)++;

    void* ptr = nullptr;
    winrt::copy_to_abi(child, ptr);
    ih->data->node_cache[*id].node_handle = (InodeHandle*)ptr;

    winuiTreeChildRebuildCacheRec(ih, child, id);
  }
}

static void winuiTreeRebuildNodeCache(Ihandle* ih, int id, TreeViewNode const& node)
{
  void* ptr = nullptr;
  winrt::copy_to_abi(node, ptr);
  ih->data->node_cache[id].node_handle = (InodeHandle*)ptr;
  winuiTreeChildRebuildCacheRec(ih, node, &id);
}

static TreeViewNode winuiTreeCopyNodeRec(Ihandle* dst, TreeViewNode const& srcNode, TreeViewNode const& dstParent, uint32_t position, TreeView const& dstTreeView)
{
  auto content = srcNode.Content();
  auto ps = content.try_as<Windows::Foundation::Collections::PropertySet>();

  TreeViewNode newNode;

  if (ps)
  {
    Windows::Foundation::Collections::PropertySet newPs;
    for (auto const& pair : ps)
      newPs.Insert(pair.Key(), pair.Value());
    newNode.Content(newPs);
  }
  else
  {
    newNode.Content(content);
  }

  bool isBranch = srcNode.HasChildren() || srcNode.HasUnrealizedChildren();
  if (isBranch)
    newNode.HasUnrealizedChildren(true);

  if (dstParent)
  {
    dstParent.Children().InsertAt(position, newNode);
    dstParent.HasUnrealizedChildren(false);
  }
  else
  {
    dstTreeView.RootNodes().InsertAt(position, newNode);
  }

  dst->data->node_count++;

  auto children = srcNode.Children();
  for (uint32_t i = 0; i < children.Size(); i++)
    winuiTreeCopyNodeRec(dst, children.GetAt(i), newNode, i, dstTreeView);

  if (isBranch && srcNode.IsExpanded())
    newNode.IsExpanded(true);

  return newNode;
}

extern "C" void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle* itemSrc, InodeHandle* itemDst)
{
  TreeView dstTreeView = winuiTreeGetTreeView(dst);
  if (!dstTreeView)
    return;

  TreeViewNode srcNode{nullptr};
  winrt::copy_from_abi(srcNode, itemSrc);
  if (!srcNode)
    return;

  TreeViewNode dstNode{nullptr};
  winrt::copy_from_abi(dstNode, itemDst);
  if (!dstNode)
    return;

  int id_dst = iupTreeFindNodeId(dst, itemDst);
  int id_new = id_dst + 1;
  int old_count = dst->data->node_count;

  TreeViewNode dstParent{nullptr};
  uint32_t position = 0;
  bool dstIsBranch = dstNode.HasChildren() || dstNode.HasUnrealizedChildren();

  if (dstIsBranch && dstNode.IsExpanded())
  {
    dstParent = dstNode;
    position = 0;
  }
  else
  {
    if (dstIsBranch)
    {
      int child_count = iupdrvTreeTotalChildCount(dst, itemDst);
      id_new += child_count;
    }

    dstParent = dstNode.Parent();
    if (dstParent)
    {
      uint32_t idx = 0;
      dstParent.Children().IndexOf(dstNode, idx);
      position = idx + 1;
    }
    else
    {
      uint32_t idx = 0;
      dstTreeView.RootNodes().IndexOf(dstNode, idx);
      position = idx + 1;
    }
  }

  TreeViewNode newNode = winuiTreeCopyNodeRec(dst, srcNode, dstParent, position, dstTreeView);

  int count = dst->data->node_count - old_count;
  iupTreeCopyMoveCache(dst, id_dst, id_new, count, 1);
  winuiTreeRebuildNodeCache(dst, id_new, newNode);

  (void)src;
}

/****************************************************************************
 * MOVENODE / COPYNODE
 ****************************************************************************/

static int winuiTreeCopyMoveNode(Ihandle* ih, int id_src, int id_dst, int is_copy)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return 0;

  TreeViewNode srcNode = winuiTreeGetNode(ih, id_src);
  if (!srcNode)
    return 0;

  TreeViewNode dstNode = winuiTreeGetNode(ih, id_dst);
  if (!dstNode)
    return 0;

  TreeViewNode ancestor = dstNode.Parent();
  while (ancestor)
  {
    if (winuiTreeFindNodeId(ih, ancestor) == id_src)
      return 0;
    ancestor = ancestor.Parent();
  }

  int id_new = id_dst + 1;
  int old_count = ih->data->node_count;

  TreeViewNode dstParent{nullptr};
  uint32_t position = 0;
  bool dstIsBranch = dstNode.HasChildren() || dstNode.HasUnrealizedChildren();

  if (dstIsBranch && dstNode.IsExpanded())
  {
    dstParent = dstNode;
    position = 0;
  }
  else
  {
    if (dstIsBranch)
    {
      int child_count = iupdrvTreeTotalChildCount(ih, (InodeHandle*)winrt::get_abi(dstNode));
      id_new += child_count;
    }

    dstParent = dstNode.Parent();
    if (dstParent)
    {
      uint32_t idx = 0;
      dstParent.Children().IndexOf(dstNode, idx);
      position = idx + 1;
    }
    else
    {
      uint32_t idx = 0;
      treeView.RootNodes().IndexOf(dstNode, idx);
      position = idx + 1;
    }
  }

  if (!is_copy && id_new == id_src)
    return 0;

  TreeViewNode newNode = winuiTreeCopyNodeRec(ih, srcNode, dstParent, position, treeView);

  int count = ih->data->node_count - old_count;
  iupTreeCopyMoveCache(ih, id_src, id_new, count, is_copy);

  if (!is_copy)
  {
    TreeViewNode srcParent = srcNode.Parent();
    if (srcParent)
    {
      uint32_t idx;
      if (srcParent.Children().IndexOf(srcNode, idx))
        srcParent.Children().RemoveAt(idx);
    }
    else
    {
      uint32_t idx;
      if (treeView.RootNodes().IndexOf(srcNode, idx))
        treeView.RootNodes().RemoveAt(idx);
    }

    ih->data->node_count = old_count;

    if (id_new > id_src)
      id_new -= count;

    winuiTreeReleaseCacheNodes(ih, id_new, count);
  }

  winuiTreeRebuildNodeCache(ih, id_new, newNode);

  return 1;
}

static int winuiTreeSetMoveNodeAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)
    return 0;

  int id_dst;
  if (!iupStrToInt(value, &id_dst))
    return 0;

  winuiTreeCopyMoveNode(ih, id, id_dst, 0);

  return 0;
}

static int winuiTreeSetCopyNodeAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)
    return 0;

  int id_dst;
  if (!iupStrToInt(value, &id_dst))
    return 0;

  winuiTreeCopyMoveNode(ih, id, id_dst, 1);

  return 0;
}

/****************************************************************************
 * Tree-Specific Drag Source
 *
 * TreeView intercepts drag gestures at the item level, so UIElement::CanDrag
 * and UIElement::DragStarting never fire. Use TreeView::CanDragItems and
 * TreeView::DragItemsStarting instead.
 ****************************************************************************/

static int winuiTreeSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  TreeView treeView = winuiTreeGetTreeView(ih);
  if (!treeView)
    return 1;

  IupWinUITreeAux* aux = winuiGetAux<IupWinUITreeAux>(ih, IUPWINUI_TREE_AUX);
  if (!aux)
    return 1;

  if (aux->dragItemsStartingToken)
  {
    treeView.DragItemsStarting(aux->dragItemsStartingToken);
    aux->dragItemsStartingToken = {};
  }
  if (aux->dragItemsCompletedToken)
  {
    treeView.DragItemsCompleted(aux->dragItemsCompletedToken);
    aux->dragItemsCompletedToken = {};
  }

  if (iupStrBoolean(value))
  {
    treeView.CanDragItems(true);

    aux->dragItemsStartingToken = treeView.DragItemsStarting([ih](TreeView const& tv, TreeViewDragItemsStartingEventArgs const& e) {
      char* drag_types = iupAttribGet(ih, "DRAGTYPES");
      if (!drag_types)
        return;

      auto items = e.Items();
      if (items.Size() == 0)
        return;

      TreeViewNode node{nullptr};
      auto item = items.GetAt(0);
      node = item.try_as<TreeViewNode>();
      if (!node)
      {
        for (int i = 0; i < ih->data->node_count; i++)
        {
          TreeViewNode n = winuiTreeGetNode(ih, i);
          if (n && n.Content() == item)
          {
            node = n;
            break;
          }
        }
      }

      if (!node)
        return;

      int id = iupTreeFindNodeId(ih, (InodeHandle*)winrt::get_abi(node));
      if (id < 0)
        return;

      iupAttribSetInt(ih, "_IUP_TREE_SOURCEID", id);

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
    });

    aux->dragItemsCompletedToken = treeView.DragItemsCompleted([ih](TreeView const&, TreeViewDragItemsCompletedEventArgs const& e) {
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
    treeView.CanDragItems(false);
  }

  return 1;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" void iupdrvTreeInitClass(Iclass* ic)
{
  winuiTreeInitializeImages();

  ic->Map = winuiTreeMapMethod;
  ic->UnMap = winuiTreeUnMapMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, winuiTreeSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "VALUE", winuiTreeGetValueAttrib, winuiTreeSetValueAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "TITLE", winuiTreeGetTitleAttrib, winuiTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "STATE", winuiTreeGetStateAttrib, winuiTreeSetStateAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND", winuiTreeGetKindAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "CHILDCOUNT", winuiTreeGetChildCountAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MARKED", winuiTreeGetMarkedAttrib, winuiTreeSetMarkedAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "DEPTH", winuiTreeGetDepthAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PARENT", winuiTreeGetParentAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", winuiTreeGetNextAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", winuiTreeGetPreviousAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FIRST", winuiTreeGetFirstAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", winuiTreeGetLastAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", winuiTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "EXPANDALL", NULL, winuiTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, winuiTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MARK", NULL, winuiTreeSetMarkAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STARTING", NULL, winuiTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKSTART", NULL, winuiTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKEDNODES", winuiTreeGetMarkedNodesAttrib, winuiTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKWHENTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "DELNODE", NULL, winuiTreeSetDelNodeAttrib, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAME", NULL, winuiTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", NULL, winuiTreeSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, winuiTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGELEAF", NULL, winuiTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, winuiTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", NULL, winuiTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "COLOR", winuiTreeGetColorAttrib, winuiTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT", winuiTreeGetTitleFontAttrib, winuiTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winuiTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winuiTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", winuiTreeGetToggleValueAttrib, winuiTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", winuiTreeGetToggleVisibleAttrib, winuiTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "MOVENODE", NULL, winuiTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", NULL, winuiTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "INDENTATION", winuiTreeGetIndentationAttrib, winuiTreeSetIndentationAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, winuiTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, winuiTreeSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
