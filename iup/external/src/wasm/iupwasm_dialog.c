/** \file
 * \brief WebAssembly Dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_drv.h"
#include "iup_image.h"
#define _IUPDLG_PRIVATE
#include "iup_dialog.h"

#include "iupwasm_drv.h"


EM_JS(void, iupwasmJsSetFavicon, (int imgId), {
  globalThis.__iupApply({ op: 'favicon', imgId: imgId });
})

EM_JS(void, iupwasmJsFullscreen, (int id, int on), {
  globalThis.__iupApply({ op: 'fullscreen', id: id, on: on });
})

EM_JS(void, iupwasmJsSetOpacity, (int id, double opacity), {
  globalThis.__iupApply({ op: 'opacity', id: id, opacity: opacity });
})

EM_JS(void, iupwasmJsBringFront, (int id), { globalThis.__iupApply({ op: 'bringfront', id: id }); })

EM_JS(void, iupwasmJsDialogSetBgImage, (int id, int imgId, int zoom), {
  globalThis.__iupApply({ op: 'dlgbgimage', id: id, imgId: imgId, zoom: zoom });
})

EM_JS(int, iupwasmJsDialogIsActive, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'dlgisactive', id: id });
  return (document.hasFocus() && globalThis.__iupActiveDialog === id) ? 1 : 0;
})

EM_JS(void, iupwasmJsDialogModal, (int id, int on), {
  globalThis.__iupApply({ op: 'dlgmodal', id: id, on: on });
})

EM_JS(void, iupwasmJsDialogResizable, (int id, int on), {
  globalThis.__iupApply({ op: 'dlgresize', id: id, on: on });
})

EMSCRIPTEN_KEEPALIVE void iupwasmDialogModalShow(Ihandle* ih, int on)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsDialogModal(id, on);
}

EMSCRIPTEN_KEEPALIVE void iupwasmDialogResize(int id, int w, int h)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnii cb;

  if (!ih || (ih->currentwidth == w && ih->currentheight == h))
    return;

  ih->currentwidth = w;
  ih->currentheight = h;

  cb = (IFnii)IupGetCallback(ih, "RESIZE_CB");
  if (!cb || cb(ih, w, h) != IUP_IGNORE)
  {
    if (!iupAttribGet(ih, "_IUPWASM_INRESIZE"))
    {
      iupAttribSet(ih, "_IUPWASM_INRESIZE", "1");
      IupRefresh(ih);
      iupAttribSet(ih, "_IUPWASM_INRESIZE", NULL);
    }
  }
}

static int wasmDialogSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle && value)
    iupwasmJsSetDocTitle(value);
  return 1;
}

static int wasmDialogSetResizeAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsDialogResizable(id, iupStrBoolean(value));
  return 1;
}

static int wasmDialogSetIconAttrib(Ihandle* ih, const char* value)
{
  void* handle = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (handle)
    iupwasmJsSetFavicon((int)(intptr_t)handle);
  return 1;
}

static int wasmDialogSetFullScreenAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsFullscreen(id, iupStrBoolean(value));
  return 1;
}

static int wasmDialogSetOpacityAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  int op;
  if (id && iupStrToInt(value, &op))
    iupwasmJsSetOpacity(id, op / 255.0);
  return 1;
}

static int wasmDialogSetBringFrontAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && iupStrBoolean(value))
    iupwasmJsBringFront(id);
  return 0;
}

static int wasmDialogSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  unsigned char r, g, b;
  if (!id || !value)
    return 1;
  if (iupStrToRGB(value, &r, &g, &b))
    iupwasmJsSetBgColor(id, r, g, b);
  else
  {
    void* img = iupImageGetImage(value, ih, 0, NULL);
    if (img)
      iupwasmJsDialogSetBgImage(id, (int)(intptr_t)img, iupAttribGetBoolean(ih, "BACKIMAGEZOOM"));
  }
  return 1;
}

static char* wasmDialogGetActiveWindowAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  return iupStrReturnBoolean(id ? iupwasmJsDialogIsActive(id) : 0);
}

static int wasmDialogMapMethod(Ihandle* ih)
{
  int id;
  iupwasmInstallTheme();  /* pick up global colors set before this dialog */
  id = iupwasmJsCreate("div");
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);

  iupwasmJsDialogStyle(id);
  iupwasmJsDialogResizable(id, IupGetInt(ih, "RESIZE"));
  iupwasmJsAddToBody(id);

  return IUP_NOERROR;
}

static void wasmDialogLayoutUpdateMethod(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsSetPos(id, iupAttribGetInt(ih, "_IUPWASM_POSX"), iupAttribGetInt(ih, "_IUPWASM_POSY"),
                    ih->currentwidth, ih->currentheight);
}

IUP_SDK_API int iupdrvDialogIsVisible(Ihandle* ih)
{
  return iupdrvIsVisible(ih);
}

IUP_SDK_API void iupdrvDialogGetSize(Ihandle* ih, InativeHandle* handle, int *w, int *h)
{
  (void)handle;
  if (w) *w = ih->currentwidth;
  if (h) *h = ih->currentheight;
}

IUP_SDK_API void iupdrvDialogSetVisible(Ihandle* ih, int visible)
{
  int id = iupwasmIdOf(ih);
  if (visible)
    ih->data->show_state = IUP_SHOW;
  if (id)
  {
    iupwasmSetVisibleState(id, visible);
    iupwasmJsSetVisible(id, visible);
    if (visible)
    {
      EM_ASM({ globalThis.__iupActiveDialog = $0; }, id);
      /* ScrollBox only constrains after a 2nd layout pass: first pass reports child-natural size, then 0 once sized */
      if (iupAttribGet(ih, "_IUPWASM_HASSCROLL") && !iupAttribGet(ih, "_IUPWASM_RELAID"))
      {
        iupAttribSet(ih, "_IUPWASM_RELAID", "1");
        IupRefresh(ih);
      }
    }
  }
}

IUP_SDK_API void iupdrvDialogGetPosition(Ihandle* ih, InativeHandle* handle, int *x, int *y)
{
  (void)handle;
  if (x) *x = ih ? iupAttribGetInt(ih, "_IUPWASM_POSX") : 0;
  if (y) *y = ih ? iupAttribGetInt(ih, "_IUPWASM_POSY") : 0;
}

IUP_SDK_API int iupdrvDialogSetPlacement(Ihandle* ih)
{
  (void)ih;
  return 0;
}

IUP_SDK_API void iupdrvDialogSetPosition(Ihandle *ih, int x, int y)
{
  int id = iupwasmIdOf(ih);
  iupAttribSetInt(ih, "_IUPWASM_POSX", x);
  iupAttribSetInt(ih, "_IUPWASM_POSY", y);
  if (id)
    iupwasmJsSetPos(id, x, y, ih->currentwidth, ih->currentheight);
}

IUP_SDK_API void iupdrvDialogGetDecoration(Ihandle* ih, int *border, int *caption, int *menu)
{
  int menu_h = (ih->data->menu) ? iupdrvMenuGetMenuBarSize(ih->data->menu) : 0;

  if (border) *border = 0;
  if (caption) *caption = 0;
  if (menu) *menu = menu_h;

  /* menubar is an absolute overlay with no native chrome; CHILDOFFSET shifts content below it */
  if (menu_h)
    iupAttribSetStrf(ih, "CHILDOFFSET", "0x%d", menu_h);
  else if (iupAttribGet(ih, "CHILDOFFSET"))
    iupAttribSet(ih, "CHILDOFFSET", NULL);
}

IUP_SDK_API void iupdrvDialogSetParent(Ihandle* ih, InativeHandle* native_parent)
{
  (void)ih;
  (void)native_parent;
}

IUP_SDK_API void iupdrvDialogInitClass(Iclass* ic)
{
  ic->Map = wasmDialogMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = wasmDialogLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, wasmDialogSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, "DLGBGCOLOR", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ICON", NULL, wasmDialogSetIconAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FULLSCREEN", NULL, wasmDialogSetFullScreenAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITY", NULL, wasmDialogSetOpacityAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BRINGFRONT", NULL, wasmDialogSetBringFrontAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, wasmDialogSetBackgroundAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ACTIVEWINDOW", wasmDialogGetActiveWindowAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RESIZE", NULL, wasmDialogSetResizeAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  /* No OS window manager in a browser tab. */
  iupClassRegisterAttribute(ic, "MAXIMIZED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINIMIZED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPMOST", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETITLEBAR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUSTOMFRAME", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVEUNDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COMPOSITED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CONTROL", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HELPBUTTON", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOOLBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OPACITYIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHAPEIMAGE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAXBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MINBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MENUBOX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BORDER", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDETASKBAR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
}
