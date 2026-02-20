/** \file
 * \brief WinUI Driver - Frame Container
 *
 * Emulates a GroupBox using Canvas + Border + TextBlock.
 * WinUI 3 doesn't have a native GroupBox control.
 *
 * Structure:
 *   Canvas (outerCanvas / innerCanvas / ih->handle)
 *     Border (frameBorder - visual frame border + background)
 *     Border (titleBorder - wraps TextBlock with opaque background)
 *       TextBlock (titleBlock - title text)
 *     [IUP children appended here]
 *
 * The frameBorder starts at Y = titleHeight/2 so the title text
 * sits ON the border line, matching Win32/GTK/Qt GroupBox behavior.
 * The titleBorder has an opaque background matching the dialog
 * background color, creating the visual gap in the border.
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdlib>
#include <cstring>
#include <cmath>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_frame.h"
#include "iup_register.h"
#include "iup_childtree.h"
#include "iup_classbase.h"
#include "iup_layout.h"
}

#include "iupwinui_drv.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::Foundation;
using namespace Windows::UI;
using namespace Windows::UI::Text;


static int winui_frame_border_width = 1;
static int winui_frame_title_height = 0;
static int winui_frame_measured = 0;

static void winuiFrameMeasureDecor(void)
{
  if (winui_frame_measured)
    return;

  TextBlock tempText;
  tempText.Text(L"Tj");
  tempText.Measure(Size(10000, 10000));

  Size textSize = tempText.DesiredSize();
  winui_frame_title_height = (int)ceil(textSize.Height);

  if (winui_frame_title_height < 16)
    winui_frame_title_height = 16;

  winui_frame_measured = 1;
}

static void winuiFrameUpdateTitleBackground(Ihandle* ih)
{
  IupWinUIFrameAux* aux = winuiGetAux<IupWinUIFrameAux>(ih, IUPWINUI_FRAME_AUX);
  if (!aux || !aux->titleBorder)
    return;

  unsigned char r, g, b;
  const char* bgcolor = IupGetGlobal("DLGBGCOLOR");
  if (iupStrToRGB(bgcolor, &r, &g, &b))
  {
    Color color;
    color.A = 255;
    color.R = r;
    color.G = g;
    color.B = b;
    aux->titleBorder.Background(SolidColorBrush(color));
  }
}

static int winuiFrameSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  IupWinUIFrameAux* aux = winuiGetAux<IupWinUIFrameAux>(ih, IUPWINUI_FRAME_AUX);
  if (aux && aux->titleBlock)
    iupwinuiUpdateTextBlockFont(ih, aux->titleBlock);

  return 1;
}

static int winuiFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  IupWinUIFrameAux* aux = winuiGetAux<IupWinUIFrameAux>(ih, IUPWINUI_FRAME_AUX);
  if (!aux)
    return 1;

  if (value && value[0])
  {
    aux->hasTitle = true;

    if (aux->titleBlock)
    {
      aux->titleBlock.Text(iupwinuiStringToHString(value));
      aux->titleBlock.Visibility(Visibility::Visible);
    }
    if (aux->titleBorder)
      aux->titleBorder.Visibility(Visibility::Visible);
  }
  else
  {
    aux->hasTitle = false;

    if (aux->titleBlock)
    {
      aux->titleBlock.Text(L"");
      aux->titleBlock.Visibility(Visibility::Collapsed);
    }
    if (aux->titleBorder)
      aux->titleBorder.Visibility(Visibility::Collapsed);
  }

  return 1;
}

static int winuiFrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    IupWinUIFrameAux* aux = winuiGetAux<IupWinUIFrameAux>(ih, IUPWINUI_FRAME_AUX);
    if (aux && aux->frameBorder)
    {
      Color color;
      color.A = 255;
      color.R = r;
      color.G = g;
      color.B = b;
      aux->frameBorder.Background(SolidColorBrush(color));
    }
  }
  return 1;
}

void winuiFrameUpdateBorderColor(Ihandle* ih)
{
  IupWinUIFrameAux* aux = winuiGetAux<IupWinUIFrameAux>(ih, IUPWINUI_FRAME_AUX);
  if (!aux || !aux->frameBorder)
    return;

  unsigned char r, g, b;
  const char* fgcolor = iupAttribGetStr(ih, "FGCOLOR");
  if (!fgcolor)
    fgcolor = IupGetGlobal("DLGFGCOLOR");
  if (!iupStrToRGB(fgcolor, &r, &g, &b))
    return;

  Color borderColor;
  borderColor.A = 64;
  borderColor.R = r;
  borderColor.G = g;
  borderColor.B = b;
  aux->frameBorder.BorderBrush(SolidColorBrush(borderColor));

  winuiFrameUpdateTitleBackground(ih);
}

static int winuiFrameMapMethod(Ihandle* ih)
{
  winuiFrameMeasureDecor();

  const char* title = iupAttribGet(ih, "TITLE");

  IupWinUIFrameAux* aux = new IupWinUIFrameAux();
  aux->hasTitle = (title && title[0]) ? true : false;

  aux->innerCanvas = Canvas();

  aux->frameBorder = Border();
  aux->frameBorder.BorderThickness(Thickness{1, 1, 1, 1});
  aux->frameBorder.CornerRadius(CornerRadius{4, 4, 4, 4});

  aux->titleBlock = TextBlock();

  aux->titleBorder = Border();
  aux->titleBorder.Padding(Thickness{3, 0, 3, 0});
  aux->titleBorder.Child(aux->titleBlock);

  if (title && title[0])
  {
    aux->titleBlock.Text(iupwinuiStringToHString(title));
    aux->titleBlock.Visibility(Visibility::Visible);
    aux->titleBorder.Visibility(Visibility::Visible);
    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
  }
  else
  {
    aux->titleBlock.Text(L"");
    aux->titleBlock.Visibility(Visibility::Collapsed);
    aux->titleBorder.Visibility(Visibility::Collapsed);

    if (iupAttribGet(ih, "BGCOLOR"))
      iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");
  }

  aux->innerCanvas.Children().Append(aux->frameBorder);
  aux->innerCanvas.Children().Append(aux->titleBorder);

  Canvas parentCanvas = iupwinuiGetParentCanvas(ih);
  if (parentCanvas)
    parentCanvas.Children().Append(aux->innerCanvas);

  winuiSetAux(ih, IUPWINUI_FRAME_AUX, aux);

  Canvas canvasRef = aux->innerCanvas;
  winuiStoreHandle(ih, canvasRef);
  /* aux->innerCanvas remains valid; canvasRef was consumed by detach_abi */

  winuiFrameUpdateBorderColor(ih);
  iupwinuiUpdateTextBlockFont(ih, aux->titleBlock);

  return IUP_NOERROR;
}

static void winuiFrameUnMapMethod(Ihandle* ih)
{
  winuiFreeAux<IupWinUIFrameAux>(ih, IUPWINUI_FRAME_AUX);
  winuiReleaseHandle<Canvas>(ih);
}

static void winuiFrameUpdateDescendants(Ihandle* ih)
{
  Ihandle* child = ih->firstchild;
  while (child)
  {
    if (child->handle)
      iupLayoutUpdate(child);
    else
      winuiFrameUpdateDescendants(child);
    child = child->brother;
  }
}

static void winuiFrameLayoutUpdateMethod(Ihandle* ih)
{
  IupWinUIFrameAux* aux = winuiGetAux<IupWinUIFrameAux>(ih, IUPWINUI_FRAME_AUX);
  if (!aux)
    return;

  Canvas outerCanvas = winuiGetHandle<Canvas>(ih);
  if (!outerCanvas)
    return;

  Canvas::SetLeft(outerCanvas, ih->x);
  Canvas::SetTop(outerCanvas, ih->y);

  if (ih->currentwidth > 0)
    outerCanvas.Width(ih->currentwidth);
  if (ih->currentheight > 0)
    outerCanvas.Height(ih->currentheight);

  int titleOffset = aux->hasTitle ? winui_frame_title_height / 2 : 0;

  if (aux->frameBorder)
  {
    Canvas::SetLeft(aux->frameBorder, 0);
    Canvas::SetTop(aux->frameBorder, titleOffset);

    if (ih->currentwidth > 0)
      aux->frameBorder.Width(ih->currentwidth);
    if (ih->currentheight > titleOffset)
      aux->frameBorder.Height(ih->currentheight - titleOffset);
  }

  if (aux->hasTitle && aux->titleBorder)
  {
    Canvas::SetLeft(aux->titleBorder, 8);
    Canvas::SetTop(aux->titleBorder, 0);
  }

  winuiFrameUpdateDescendants(ih);
}

static void* winuiFrameGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return ih->handle;
}

static int winuiFrameSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    IupWinUIFrameAux* aux = winuiGetAux<IupWinUIFrameAux>(ih, IUPWINUI_FRAME_AUX);
    if (aux && aux->titleBlock)
    {
      Color color;
      color.A = 255;
      color.R = r;
      color.G = g;
      color.B = b;
      aux->titleBlock.Foreground(SolidColorBrush(color));
    }

    winuiFrameUpdateBorderColor(ih);
  }
  return 1;
}

extern "C" int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 1;
}

extern "C" void iupdrvFrameGetDecorOffset(Ihandle* ih, int* x, int* y)
{
  (void)ih;
  *x = winui_frame_border_width + 4;
  *y = winui_frame_border_width + 4;
}

extern "C" int iupdrvFrameGetDecorSize(Ihandle* ih, int* w, int* h)
{
  if (!winui_frame_measured)
    winuiFrameMeasureDecor();

  *w = 2 * (winui_frame_border_width + 4);

  const char* title = iupAttribGet(ih, "TITLE");
  if (title && title[0])
    *h = winui_frame_title_height / 2 + 2 * (winui_frame_border_width + 4);
  else
    *h = 2 * (winui_frame_border_width + 4);

  return 1;
}

extern "C" int iupdrvFrameGetTitleHeight(Ihandle* ih, int* h)
{
  const char* title = iupAttribGet(ih, "TITLE");
  if (!title || !title[0])
  {
    *h = 0;
    return 1;
  }

  if (!winui_frame_measured)
    winuiFrameMeasureDecor();

  *h = winui_frame_title_height / 2;
  return 1;
}

extern "C" void iupdrvFrameInitClass(Iclass* ic)
{
  ic->Map = winuiFrameMapMethod;
  ic->UnMap = winuiFrameUnMapMethod;
  ic->LayoutUpdate = winuiFrameLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = winuiFrameGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, winuiFrameSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winuiFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winuiFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, winuiFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}
