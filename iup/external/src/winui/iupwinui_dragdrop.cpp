/** \file
 * \brief WinUI Driver - Drag and Drop
 *
 * Uses WinUI UIElement drag-drop events for DROPFILESTARGET and custom drag-drop.
 *
 * WinUI's DataPackage custom formats don't survive the OLE DnD bridge for
 * non-standard data types (PropertyValue). For custom drag-drop (DRAGSOURCE/
 * DROPTARGET), the actual data is transferred through in-process storage while
 * the DataPackage text carries the format type string for matching.
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_class.h"
#include "iup_image.h"
}

#include "iupwinui_drv.h"

#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Graphics.Imaging.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::Storage;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;


static DataPackageOperation winuiDropAcceptedOperation(DragEventArgs const& e)
{
  DataPackageOperation allowed = e.AllowedOperations();
  bool copy_allowed = (allowed & DataPackageOperation::Copy) == DataPackageOperation::Copy;
  bool move_allowed = (allowed & DataPackageOperation::Move) == DataPackageOperation::Move;

  if (move_allowed && (!copy_allowed || !(GetKeyState(VK_CONTROL) & 0x8000)))
    return DataPackageOperation::Move;
  if (copy_allowed)
    return DataPackageOperation::Copy;
  return DataPackageOperation::None;
}

/****************************************************************************
 * Drop Files Target (DROPFILESTARGET attribute)
 ****************************************************************************/

static void winuiDropFilesDragOver(IInspectable const&, DragEventArgs const& e)
{
  auto view = e.DataView();
  if (view.Contains(StandardDataFormats::StorageItems()))
    e.AcceptedOperation(DataPackageOperation::Copy);
}

static void winuiDropFilesDrop(Ihandle* ih, IInspectable const& sender, DragEventArgs const& e)
{
  auto cb = reinterpret_cast<IFnsiii>(IupGetCallback(ih, "DROPFILES_CB"));
  if (!cb)
    return;

  auto view = e.DataView();
  if (!view.Contains(StandardDataFormats::StorageItems()))
    return;

  auto items = view.GetStorageItemsAsync().get();
  if (!items)
    return;

  int count = static_cast<int>(items.Size());
  double scale = iupwinuiGetScale(ih);
  auto pos = e.GetPosition(sender.try_as<UIElement>());

  for (int i = 0; i < count; i++)
  {
    auto item = items.GetAt(i);
    auto path = item.Path();
    if (path.empty())
      continue;

    char* filename = iupwinuiHStringToString(path);
    if (cb(ih, filename, count - i - 1, static_cast<int>(pos.X * scale), static_cast<int>(pos.Y * scale)) == IUP_IGNORE)
      break;
  }
}

static UIElement winuiGetDropFilesElement(Ihandle* ih)
{
  if (ih->iclass->nativetype == IUP_TYPEDIALOG)
  {
    auto* aux = winuiGetAux<IupWinUIDialogAux>(ih, IUPWINUI_DIALOG_AUX);
    if (aux && aux->rootPanel)
      return aux->rootPanel.try_as<UIElement>();
    return nullptr;
  }

  return winuiGetHandle<UIElement>(ih);
}

static int winuiSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;

  UIElement elem = winuiGetDropFilesElement(ih);
  if (!elem)
    return 1;

  auto* dragOverToken = reinterpret_cast<event_token*>(iupAttribGet(ih, "_IUPWINUI_DRAGOVER_TOKEN"));
  auto* dropToken = reinterpret_cast<event_token*>(iupAttribGet(ih, "_IUPWINUI_DROP_TOKEN"));

  if (dragOverToken)
  {
    elem.DragOver(*dragOverToken);
    delete dragOverToken;
    iupAttribSet(ih, "_IUPWINUI_DRAGOVER_TOKEN", nullptr);
  }
  if (dropToken)
  {
    elem.Drop(*dropToken);
    delete dropToken;
    iupAttribSet(ih, "_IUPWINUI_DROP_TOKEN", nullptr);
  }

  if (iupStrBoolean(value))
  {
    elem.AllowDrop(true);

    dragOverToken = new event_token();
    *dragOverToken = elem.DragOver(winuiDropFilesDragOver);
    iupAttribSet(ih, "_IUPWINUI_DRAGOVER_TOKEN", reinterpret_cast<char*>(dragOverToken));

    dropToken = new event_token();
    *dropToken = elem.Drop([ih](IInspectable const& sender, DragEventArgs const& args) {
      winuiDropFilesDrop(ih, sender, args);
    });
    iupAttribSet(ih, "_IUPWINUI_DROP_TOKEN", reinterpret_cast<char*>(dropToken));
  }
  else
  {
    elem.AllowDrop(false);
  }

  return 1;
}

/****************************************************************************
 * Custom Drag and Drop (DRAGSOURCE / DROPTARGET attributes)
 *
 * Drag data lives in statics for the duration of the drag; the DataPackage text
 * carries only the type string.
 ****************************************************************************/

static void* winui_drag_data = nullptr;
static int winui_drag_data_size = 0;
static char winui_drag_type[256] = "";

void winuiDragDataCleanup()
{
  if (winui_drag_data)
  {
    free(winui_drag_data);
    winui_drag_data = nullptr;
  }
  winui_drag_data_size = 0;
  winui_drag_type[0] = '\0';
}

void winuiDragSetInProcessData(const char* type, void* data, int size)
{
  winuiDragDataCleanup();
  winui_drag_data = data;
  winui_drag_data_size = size;
  strncpy(winui_drag_type, type, sizeof(winui_drag_type) - 1);
  winui_drag_type[sizeof(winui_drag_type) - 1] = '\0';
}

static void winuiDragSetCursor(Ihandle* ih, DragStartingEventArgs const& e)
{
  char* name = iupAttribGet(ih, "DRAGCURSOR");
  if (!name)
    return;

  void* handle = iupImageGetImage(name, ih, 0, nullptr);
  auto bitmap = winuiGetBitmapFromHandle(handle);
  if (!bitmap)
    return;

  /* the IUP image is already BGRA premultiplied */
  auto softwareBitmap = Windows::Graphics::Imaging::SoftwareBitmap::CreateCopyFromBuffer(
    bitmap.PixelBuffer(),
    Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
    bitmap.PixelWidth(), bitmap.PixelHeight(),
    Windows::Graphics::Imaging::BitmapAlphaMode::Premultiplied);

  if (softwareBitmap)
    e.DragUI().SetContentFromSoftwareBitmap(softwareBitmap);
}

static void winuiDragStartingHandler(Ihandle* ih, DragStartingEventArgs const& e, UIElement const& posRelativeTo)
{
  auto dragbegin_cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "DRAGBEGIN_CB"));
  if (dragbegin_cb)
  {
    double scale = iupwinuiGetScale(ih);
    auto pos = e.GetPosition(posRelativeTo);
    int ret = dragbegin_cb(ih, static_cast<int>(pos.X * scale), static_cast<int>(pos.Y * scale));
    if (ret == IUP_IGNORE)
    {
      e.Cancel(true);
      return;
    }
  }

  winuiDragSetCursor(ih, e);

  IFns datasize_cb = reinterpret_cast<IFns>(IupGetCallback(ih, "DRAGDATASIZE_CB"));
  auto dragdata_cb = reinterpret_cast<IFnsVi>(IupGetCallback(ih, "DRAGDATA_CB"));

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
    e.Data().RequestedOperation(DataPackageOperation::Move | DataPackageOperation::Copy);
  else
    e.Data().RequestedOperation(DataPackageOperation::Copy);
}

static void winuiDropCompletedHandler(Ihandle* ih, DropCompletedEventArgs const& e)
{
  IFni dragend_cb = reinterpret_cast<IFni>(IupGetCallback(ih, "DRAGEND_CB"));
  if (dragend_cb)
  {
    int del = (e.DropResult() == DataPackageOperation::Move) ? 1 : ((e.DropResult() == DataPackageOperation::Copy) ? 0 : -1);
    dragend_cb(ih, del);
  }

  winuiDragDataCleanup();
}

static int winuiSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;

  IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, ih->handle);
  UIElement elem = obj.try_as<UIElement>();
  if (!elem)
    return 1;

  bool enable = iupStrBoolean(value) ? true : false;

  auto* dragStartToken = reinterpret_cast<event_token*>(iupAttribGet(ih, "_IUPWINUI_DRAGSTART_TOKEN"));
  if (dragStartToken)
  {
    elem.DragStarting(*dragStartToken);
    delete dragStartToken;
    iupAttribSet(ih, "_IUPWINUI_DRAGSTART_TOKEN", nullptr);
  }

  auto* dropCompletedToken = reinterpret_cast<event_token*>(iupAttribGet(ih, "_IUPWINUI_DROPCOMPLETED_TOKEN"));
  if (dropCompletedToken)
  {
    elem.DropCompleted(*dropCompletedToken);
    delete dropCompletedToken;
    iupAttribSet(ih, "_IUPWINUI_DROPCOMPLETED_TOKEN", nullptr);
  }

  if (enable)
  {
    elem.CanDrag(true);

    dragStartToken = new event_token();
    *dragStartToken = elem.DragStarting([ih](UIElement const& sender, DragStartingEventArgs const& e) {
      winuiDragStartingHandler(ih, e, sender);
    });
    iupAttribSet(ih, "_IUPWINUI_DRAGSTART_TOKEN", reinterpret_cast<char*>(dragStartToken));

    dropCompletedToken = new event_token();
    *dropCompletedToken = elem.DropCompleted([ih](UIElement const&, DropCompletedEventArgs const& e) {
      winuiDropCompletedHandler(ih, e);
    });
    iupAttribSet(ih, "_IUPWINUI_DROPCOMPLETED_TOKEN", reinterpret_cast<char*>(dropCompletedToken));
  }
  else
  {
    elem.CanDrag(false);
  }

  return 1;
}

void winuiDropTargetRemoveHandlers(Ihandle* ih, UIElement const& elem)
{
  void* abi = iupAttribGet(ih, "_IUPWINUI_CUSTOMDRAGOVER_HANDLER");
  if (abi)
  {
    IInspectable handler{nullptr};
    winrt::attach_abi(handler, abi);
    elem.RemoveHandler(UIElement::DragEnterEvent(), handler);
    elem.RemoveHandler(UIElement::DragOverEvent(), handler);
    iupAttribSet(ih, "_IUPWINUI_CUSTOMDRAGOVER_HANDLER", nullptr);
  }

  abi = iupAttribGet(ih, "_IUPWINUI_CUSTOMDROP_HANDLER");
  if (abi)
  {
    IInspectable handler{nullptr};
    winrt::attach_abi(handler, abi);
    elem.RemoveHandler(UIElement::DropEvent(), handler);
    iupAttribSet(ih, "_IUPWINUI_CUSTOMDROP_HANDLER", nullptr);
  }
}

static int winuiSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;

  IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, ih->handle);
  UIElement elem = obj.try_as<UIElement>();
  if (!elem)
    return 1;

  winuiDropTargetRemoveHandlers(ih, elem);

  if (iupStrBoolean(value))
  {
    elem.AllowDrop(true);

    IInspectable dragOverHandler = winrt::box_value(DragEventHandler([ih](IInspectable const& sender, DragEventArgs const& e) {
      char* drop_types = iupAttribGet(ih, "DROPTYPES");
      if (!drop_types)
        return;

      if (winui_drag_type[0] == '\0' || strcmp(winui_drag_type, drop_types) != 0)
        return;

      auto dropmotion_cb = reinterpret_cast<IFniis>(IupGetCallback(ih, "DROPMOTION_CB"));
      if (dropmotion_cb)
      {
        char status[20] = "";
        iupdrvGetKeyState(status);
        double scale = iupwinuiGetScale(ih);
        auto pos = e.GetPosition(sender.try_as<UIElement>());
        dropmotion_cb(ih, static_cast<int>(pos.X * scale), static_cast<int>(pos.Y * scale), status);
      }

      e.AcceptedOperation(winuiDropAcceptedOperation(e));
      e.Handled(true);
    }));
    elem.AddHandler(UIElement::DragEnterEvent(), dragOverHandler, true);
    elem.AddHandler(UIElement::DragOverEvent(), dragOverHandler, true);
    iupAttribSet(ih, "_IUPWINUI_CUSTOMDRAGOVER_HANDLER", static_cast<char*>(winrt::detach_abi(dragOverHandler)));

    IInspectable dropHandler = winrt::box_value(DragEventHandler([ih](IInspectable const& sender, DragEventArgs const& e) {
      auto dropdata_cb = reinterpret_cast<IFnsViii>(IupGetCallback(ih, "DROPDATA_CB"));
      if (!dropdata_cb)
        return;

      char* drop_types = iupAttribGet(ih, "DROPTYPES");
      if (!drop_types)
        return;

      if (!winui_drag_data || winui_drag_type[0] == '\0' || strcmp(winui_drag_type, drop_types) != 0)
        return;

      double scale = iupwinuiGetScale(ih);
      auto pos = e.GetPosition(sender.try_as<UIElement>());
      dropdata_cb(ih, winui_drag_type, winui_drag_data, winui_drag_data_size, static_cast<int>(pos.X * scale), static_cast<int>(pos.Y * scale));

      e.Handled(true);
    }));
    elem.AddHandler(UIElement::DropEvent(), dropHandler, true);
    iupAttribSet(ih, "_IUPWINUI_CUSTOMDROP_HANDLER", static_cast<char*>(winrt::detach_abi(dropHandler)));
  }
  else
  {
    elem.AllowDrop(false);
  }

  return 1;
}

extern "C" IUP_SDK_API void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");

  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");

  iupClassRegisterAttribute(ic, "DRAGTYPES", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", nullptr, winuiSetDragSourceAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", nullptr, winuiSetDropTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSOR", nullptr, nullptr, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", nullptr, winuiSetDropFilesTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", nullptr, winuiSetDropFilesTargetAttrib, nullptr, nullptr, IUPAF_NO_INHERIT);
}
