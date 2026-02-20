/** \file
 * \brief WinUI Driver - Popover Control
 *
 * Uses WinUI Flyout for anchored floating content.
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
#include "iup_class.h"
#include "iup_layout.h"
#include "iup_popover.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Windows::Foundation;


static FlyoutPlacementMode winuiPopoverGetPlacement(Ihandle* ih)
{
  char* position = iupAttribGet(ih, "POSITION");
  if (iupStrEqualNoCase(position, "TOP"))
    return FlyoutPlacementMode::Top;
  if (iupStrEqualNoCase(position, "LEFT"))
    return FlyoutPlacementMode::Left;
  if (iupStrEqualNoCase(position, "RIGHT"))
    return FlyoutPlacementMode::Right;
  return FlyoutPlacementMode::Bottom;
}

static void winuiPopoverCallShowCB(Ihandle* ih, int state)
{
  IFni cb = (IFni)IupGetCallback(ih, "SHOW_CB");
  if (cb)
  {
    if (cb(ih, state) == IUP_CLOSE)
      IupExitLoop();
  }
}

static int winuiPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    if (!anchor || !anchor->handle)
      return 0;

    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    IupWinUIPopoverAux* aux = winuiGetAux<IupWinUIPopoverAux>(ih, IUPWINUI_POPOVER_AUX);
    if (!aux || !aux->flyout)
      return 0;

    FrameworkElement anchorElem = winuiGetHandle<FrameworkElement>(anchor);
    if (!anchorElem)
      return 0;

    if (ih->firstchild)
    {
      iupLayoutCompute(ih);
      iupLayoutUpdate(ih);
    }

    aux->flyout.Placement(winuiPopoverGetPlacement(ih));

    if (!iupAttribGetBoolean(ih, "AUTOHIDE"))
    {
      Ihandle* dialog = IupGetDialog(anchor);
      if (dialog)
      {
        IupWinUIDialogAux* dlgAux = winuiGetAux<IupWinUIDialogAux>(dialog, IUPWINUI_DIALOG_AUX);
        if (dlgAux && dlgAux->rootPanel)
          aux->flyout.OverlayInputPassThroughElement(dlgAux->rootPanel);
      }
    }

    aux->flyout.ShowAt(anchorElem);
    aux->isVisible = true;

    winuiPopoverCallShowCB(ih, 1);
  }
  else
  {
    IupWinUIPopoverAux* aux = winuiGetAux<IupWinUIPopoverAux>(ih, IUPWINUI_POPOVER_AUX);
    if (aux && aux->isVisible)
    {
      aux->programmaticClose = true;
      aux->flyout.Hide();
      aux->programmaticClose = false;
      aux->isVisible = false;
      winuiPopoverCallShowCB(ih, 0);
    }
  }

  return 0;
}

static char* winuiPopoverGetVisibleAttrib(Ihandle* ih)
{
  IupWinUIPopoverAux* aux = winuiGetAux<IupWinUIPopoverAux>(ih, IUPWINUI_POPOVER_AUX);
  if (aux)
    return iupStrReturnBoolean(aux->isVisible);
  return iupStrReturnBoolean(0);
}

static int winuiPopoverMapMethod(Ihandle* ih)
{
  IupWinUIPopoverAux* aux = new IupWinUIPopoverAux();

  aux->flyout = Flyout();
  aux->innerCanvas = Canvas();

  aux->flyout.Content(aux->innerCanvas);
  aux->flyout.Placement(winuiPopoverGetPlacement(ih));

  aux->closedToken = aux->flyout.Closed([ih](IInspectable const&, IInspectable const&) {
    IupWinUIPopoverAux* a = winuiGetAux<IupWinUIPopoverAux>(ih, IUPWINUI_POPOVER_AUX);
    if (a && a->isVisible)
    {
      a->isVisible = false;
      winuiPopoverCallShowCB(ih, 0);
    }
  });

  aux->closingToken = aux->flyout.Closing([ih](FlyoutBase const&, FlyoutBaseClosingEventArgs const& args) {
    IupWinUIPopoverAux* a = winuiGetAux<IupWinUIPopoverAux>(ih, IUPWINUI_POPOVER_AUX);
    if (a && a->programmaticClose)
      return;
    if (!iupAttribGetBoolean(ih, "AUTOHIDE"))
      args.Cancel(true);
  });

  winuiSetAux(ih, IUPWINUI_POPOVER_AUX, aux);
  Canvas canvasCopy = aux->innerCanvas;
  winuiStoreHandle(ih, canvasCopy);
  return IUP_NOERROR;
}

static void winuiPopoverUnMapMethod(Ihandle* ih)
{
  IupWinUIPopoverAux* aux = winuiGetAux<IupWinUIPopoverAux>(ih, IUPWINUI_POPOVER_AUX);

  if (aux)
  {
    if (aux->flyout)
    {
      if (aux->closingToken)
        aux->flyout.Closing(aux->closingToken);
      if (aux->closedToken)
        aux->flyout.Closed(aux->closedToken);
    }

    if (aux->isVisible)
      aux->flyout.Hide();

    winuiReleaseHandle<Canvas>(ih);
  }

  winuiFreeAux<IupWinUIPopoverAux>(ih, IUPWINUI_POPOVER_AUX);
  ih->handle = NULL;
}

static void winuiPopoverLayoutUpdateMethod(Ihandle* ih)
{
  IupWinUIPopoverAux* aux = winuiGetAux<IupWinUIPopoverAux>(ih, IUPWINUI_POPOVER_AUX);
  if (!aux || !aux->innerCanvas)
    return;

  aux->innerCanvas.Width(ih->currentwidth);
  aux->innerCanvas.Height(ih->currentheight);

  if (ih->firstchild)
  {
    ih->iclass->SetChildrenPosition(ih, 0, 0);
    iupLayoutUpdate(ih->firstchild);
  }
}

static void* winuiPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return ih->handle;
}

extern "C" void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = winuiPopoverMapMethod;
  ic->UnMap = winuiPopoverUnMapMethod;
  ic->LayoutUpdate = winuiPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = winuiPopoverGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "VISIBLE", winuiPopoverGetVisibleAttrib, winuiPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
