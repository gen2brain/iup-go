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
#include <cstdio>
#include <string>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_class.h"
#include "iup_key.h"
}

#include "iupwinui_drv.h"

#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Storage.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::Storage;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;


/****************************************************************************
 * Drop Files Target (DROPFILESTARGET attribute)
 ****************************************************************************/

static void winuiDropFilesDragOver(IInspectable const&, DragEventArgs const& e)
{
  auto view = e.DataView();
  if (view.Contains(StandardDataFormats::StorageItems()))
    e.AcceptedOperation(DataPackageOperation::Copy);
}

static void winuiDropFilesDrop(Ihandle* ih, IInspectable const&, DragEventArgs const& e)
{
  IFnsiii cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
  if (!cb)
    return;

  auto view = e.DataView();
  if (!view.Contains(StandardDataFormats::StorageItems()))
    return;

  auto items = view.GetStorageItemsAsync().get();
  if (!items)
    return;

  int count = (int)items.Size();
  auto pos = e.GetPosition(nullptr);

  for (int i = 0; i < count; i++)
  {
    auto item = items.GetAt(i);
    auto path = item.Path();
    if (path.empty())
      continue;

    char* filename = iupwinuiHStringToString(path);
    if (cb(ih, filename, count - i - 1, (int)pos.X, (int)pos.Y) == IUP_IGNORE)
      break;
  }
}

static int winuiSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;

  IInspectable obj{nullptr};
  winrt::copy_from_abi(obj, ih->handle);
  UIElement elem = obj.try_as<UIElement>();
  if (!elem)
    return 1;

  event_token* dragOverToken = (event_token*)iupAttribGet(ih, "_IUPWINUI_DRAGOVER_TOKEN");
  event_token* dropToken = (event_token*)iupAttribGet(ih, "_IUPWINUI_DROP_TOKEN");

  if (dragOverToken)
  {
    elem.DragOver(*dragOverToken);
    delete dragOverToken;
    iupAttribSet(ih, "_IUPWINUI_DRAGOVER_TOKEN", NULL);
  }
  if (dropToken)
  {
    elem.Drop(*dropToken);
    delete dropToken;
    iupAttribSet(ih, "_IUPWINUI_DROP_TOKEN", NULL);
  }

  if (iupStrBoolean(value))
  {
    elem.AllowDrop(true);

    dragOverToken = new event_token();
    *dragOverToken = elem.DragOver(winuiDropFilesDragOver);
    iupAttribSet(ih, "_IUPWINUI_DRAGOVER_TOKEN", (char*)dragOverToken);

    dropToken = new event_token();
    *dropToken = elem.Drop([ih](IInspectable const& sender, DragEventArgs const& args) {
      winuiDropFilesDrop(ih, sender, args);
    });
    iupAttribSet(ih, "_IUPWINUI_DROP_TOKEN", (char*)dropToken);
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
 * In-process data transfer: drag data is stored in statics during
 * the drag operation. DataPackage text carries the type string.
 ****************************************************************************/

static void* winui_drag_data = NULL;
static int winui_drag_data_size = 0;
static char winui_drag_type[256] = "";

void winuiDragDataCleanup(void)
{
  if (winui_drag_data)
  {
    free(winui_drag_data);
    winui_drag_data = NULL;
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


static void winuiDragStartingHandler(Ihandle* ih, DragStartingEventArgs const& e, UIElement const& posRelativeTo)
{
  IFnii dragbegin_cb = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
  if (dragbegin_cb)
  {
    auto pos = e.GetPosition(posRelativeTo);
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

        winuiDragDataCleanup();
        winui_drag_data = data;
        winui_drag_data_size = size;
        strncpy(winui_drag_type, drag_types, sizeof(winui_drag_type) - 1);
        winui_drag_type[sizeof(winui_drag_type) - 1] = '\0';

        e.Data().SetText(iupwinuiStringToHString(drag_types));
      }
    }
  }

  char* move = iupAttribGet(ih, "DRAGSOURCEMOVE");
  if (iupStrBoolean(move))
    e.Data().RequestedOperation(DataPackageOperation::Move);
  else
    e.Data().RequestedOperation(DataPackageOperation::Copy);
}

static void winuiDropCompletedHandler(Ihandle* ih, DropCompletedEventArgs const& e)
{
  IFni dragend_cb = (IFni)IupGetCallback(ih, "DRAGEND_CB");
  if (dragend_cb)
  {
    int del = (e.DropResult() == DataPackageOperation::Move) ? 1 : 0;
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

  event_token* dragStartToken = (event_token*)iupAttribGet(ih, "_IUPWINUI_DRAGSTART_TOKEN");
  if (dragStartToken)
  {
    elem.DragStarting(*dragStartToken);
    delete dragStartToken;
    iupAttribSet(ih, "_IUPWINUI_DRAGSTART_TOKEN", NULL);
  }

  event_token* dropCompletedToken = (event_token*)iupAttribGet(ih, "_IUPWINUI_DROPCOMPLETED_TOKEN");
  if (dropCompletedToken)
  {
    elem.DropCompleted(*dropCompletedToken);
    delete dropCompletedToken;
    iupAttribSet(ih, "_IUPWINUI_DROPCOMPLETED_TOKEN", NULL);
  }

  if (enable)
  {
    elem.CanDrag(true);

    dragStartToken = new event_token();
    *dragStartToken = elem.DragStarting([ih](UIElement const& sender, DragStartingEventArgs const& e) {
      winuiDragStartingHandler(ih, e, sender);
    });
    iupAttribSet(ih, "_IUPWINUI_DRAGSTART_TOKEN", (char*)dragStartToken);

    dropCompletedToken = new event_token();
    *dropCompletedToken = elem.DropCompleted([ih](UIElement const&, DropCompletedEventArgs const& e) {
      winuiDropCompletedHandler(ih, e);
    });
    iupAttribSet(ih, "_IUPWINUI_DROPCOMPLETED_TOKEN", (char*)dropCompletedToken);
  }
  else
  {
    elem.CanDrag(false);
  }

  return 1;
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

  event_token* dragOverToken = (event_token*)iupAttribGet(ih, "_IUPWINUI_CUSTOMDRAGOVER_TOKEN");
  event_token* dropToken = (event_token*)iupAttribGet(ih, "_IUPWINUI_CUSTOMDROP_TOKEN");

  if (dragOverToken)
  {
    elem.DragOver(*dragOverToken);
    delete dragOverToken;
    iupAttribSet(ih, "_IUPWINUI_CUSTOMDRAGOVER_TOKEN", NULL);
  }
  if (dropToken)
  {
    elem.Drop(*dropToken);
    delete dropToken;
    iupAttribSet(ih, "_IUPWINUI_CUSTOMDROP_TOKEN", NULL);
  }

  if (iupStrBoolean(value))
  {
    elem.AllowDrop(true);

    dragOverToken = new event_token();
    *dragOverToken = elem.DragOver([ih](IInspectable const&, DragEventArgs const& e) {
      char* drop_types = iupAttribGet(ih, "DROPTYPES");
      if (!drop_types)
        return;

      if (winui_drag_type[0] == '\0' || strcmp(winui_drag_type, drop_types) != 0)
        return;

      IFniis dropmotion_cb = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
      if (dropmotion_cb)
      {
        char status[20] = "";
        iupdrvGetKeyState(status);
        auto pos = e.GetPosition(nullptr);
        dropmotion_cb(ih, (int)pos.X, (int)pos.Y, status);
      }

      e.AcceptedOperation(DataPackageOperation::Copy | DataPackageOperation::Move);
      e.Handled(true);
    });
    iupAttribSet(ih, "_IUPWINUI_CUSTOMDRAGOVER_TOKEN", (char*)dragOverToken);

    dropToken = new event_token();
    *dropToken = elem.Drop([ih](IInspectable const&, DragEventArgs const& e) {
      IFnsViii dropdata_cb = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
      if (!dropdata_cb)
        return;

      char* drop_types = iupAttribGet(ih, "DROPTYPES");
      if (!drop_types)
        return;

      if (!winui_drag_data || winui_drag_type[0] == '\0' || strcmp(winui_drag_type, drop_types) != 0)
        return;

      auto pos = e.GetPosition(nullptr);
      dropdata_cb(ih, winui_drag_type, winui_drag_data, winui_drag_data_size, (int)pos.X, (int)pos.Y);

      e.Handled(true);
    });
    iupAttribSet(ih, "_IUPWINUI_CUSTOMDROP_TOKEN", (char*)dropToken);
  }
  else
  {
    elem.AllowDrop(false);
  }

  return 1;
}

extern "C" void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");

  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");

  iupClassRegisterAttribute(ic, "DRAGTYPES", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, winuiSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", NULL, winuiSetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, winuiSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, winuiSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
