/** \file
 * \brief WebAssembly Toggle
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_image.h"
#include "iup_toggle.h"

#include "iupwasm_drv.h"


EM_JS(void, iupwasmJsToggleImpress, (int id, int imgId), { globalThis.__iupApply({ op: 'toggleimpress', id: id, imgId: imgId }); })
EM_JS(void, iupwasmJsToggleImageAlign, (int id, const char* h, const char* v), { globalThis.__iupApply({ op: 'togglealign', id: id, h: UTF8ToString(h), v: UTF8ToString(v) }); })
EM_JS(void, iupwasmJsToggleRightButton, (int id, int on), { globalThis.__iupApply({ op: 'togglerightbtn', id: id, on: on }); })
EM_JS(void, iupwasmJsToggle3State, (int id, int on), { globalThis.__iupApply({ op: 'toggle3state', id: id, on: on }); })

static int wasmToggleSet3StateAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && !ih->data->is_radio && ih->data->type != IUP_TOGGLE_IMAGE)
    iupwasmJsToggle3State(id, iupStrBoolean(value));
  return 1;
}

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchToggle(int id, int state)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFni cb;
  if (!ih)
    return;
  cb = (IFni)IupGetCallback(ih, "ACTION");
  if (cb)
  {
    if (cb(ih, state) == IUP_CLOSE)
      IupExitLoop();
  }
  if (iupObjectCheck(ih))
    iupBaseCallValueChangedCb(ih);
}

static int wasmToggleSetValueAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
  {
    if (iupStrEqualNoCase(value, "NOTDEF"))
      iupwasmJsToggleSetValue(id, -1);
    else
      iupwasmJsToggleSetValue(id, iupStrBoolean(value));
  }
  return 1;
}

static char* wasmToggleGetValueAttrib(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  return iupStrReturnChecked(id ? iupwasmJsToggleGetValue(id) : 0);
}

static int wasmToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
  {
    if (iupAttribGetBoolean(ih, "MARKUP"))
      iupwasmJsSetMarkup(id, value);
    else
    {
      char* clean = iupwasmStripMnemonic(value);
      iupwasmJsToggleSetText(id, clean);
      free(clean);
    }
  }
  return 1;
}

static int wasmToggleSetMarkupAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  char* title = iupAttribGet(ih, "TITLE");
  if (id && title && iupStrBoolean(value))
    iupwasmJsSetMarkup(id, title);
  return 1;
}

static int wasmToggleSetImageAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  void* img = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (id && img)
    iupwasmJsSetImage(id, (int)(intptr_t)img);
  return 1;
}

static int wasmToggleSetImpressAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  void* img = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (id)
    iupwasmJsToggleImpress(id, img ? (int)(intptr_t)img : 0);
  return 1;
}

static int wasmToggleSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  void* img = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (id && ih->data->type == IUP_TOGGLE_IMAGE)
    iupwasmJsSetInactiveImage(id, img ? (int)(intptr_t)img : 0, 1);
  return 1;
}

static int wasmToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  int h = 0, v = 0;
  iupStrToIntInt(value, &h, &v, 'x');
  if (id)
    iupwasmJsSetPadding(id, h, v);
  return 1;
}

static int wasmToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  char halign[32] = "ACENTER", valign[32] = "ACENTER";
  iupStrToStrStr(value, halign, sizeof(halign), valign, sizeof(valign), ':');
  if (id)
    iupwasmJsToggleImageAlign(id, halign, valign);
  return 1;
}

static int wasmToggleSetRightButtonAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsToggleRightButton(id, iupStrBoolean(value));
  return 1;
}

static int wasmToggleMapMethod(Ihandle* ih)
{
  char* image;
  int id;
  Ihandle* radio = iupRadioFindToggleParent(ih);

  ih->data->is_radio = (radio != NULL);
  image = iupAttribGet(ih, "IMAGE");
  ih->data->type = image ? IUP_TOGGLE_IMAGE : IUP_TOGGLE_TEXT;

  id = iupwasmJsCreateToggle(ih->data->is_radio, (int)(intptr_t)radio);
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);

  if (ih->data->type == IUP_TOGGLE_IMAGE)
  {
    void* img = iupImageGetImage(image, ih, 0, NULL);
    char* impress = iupAttribGet(ih, "IMPRESS");
    void* pimg = impress ? iupImageGetImage(impress, ih, 0, NULL) : NULL;
    iupwasmJsToggleImageMode(id);
    if (img)
      iupwasmJsSetImage(id, (int)(intptr_t)img);
    if (pimg)
      iupwasmJsToggleImpress(id, (int)(intptr_t)pimg);
  }
  else if (!ih->data->is_radio && iupAttribGetBoolean(ih, "SWITCH"))
  {
    iupwasmJsToggleSwitchMode(id);
  }
  else
  {
    char* title = iupAttribGet(ih, "TITLE");
    if (title)
    {
      char* clean = iupwasmStripMnemonic(title);
      iupwasmJsToggleSetText(id, clean);
      free(clean);
    }
  }

  if (iupAttribGetBoolean(ih, "VALUE"))
    iupwasmJsToggleSetValue(id, 1);

  iupwasmJsToggleWire(id);
  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvToggleAddCheckBox(Ihandle* ih, int *x, int *y, const char* str)
{
  (void)ih;
  (void)str;
  if (x) *x += 20;
  if (y && *y < 18) *y = 18;
}

IUP_SDK_API void iupdrvToggleAddSwitch(Ihandle* ih, int *x, int *y, const char* str)
{
  (void)ih;
  (void)str;
  if (x) *x += 40;
  if (y && *y < 20) *y = 20;
}

IUP_SDK_API void iupdrvToggleAddBorders(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  if (x) *x += 6;
  if (y) *y += 6;
}

IUP_SDK_API void iupdrvToggleInitClass(Iclass* ic)
{
  ic->Map = wasmToggleMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, wasmToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", wasmToggleGetValueAttrib, wasmToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, wasmToggleSetMarkupAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, wasmToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, wasmToggleSetImpressAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, wasmToggleSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", NULL, wasmToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, wasmToggleSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ACENTER:ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, wasmToggleSetRightButtonAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "3STATE", NULL, wasmToggleSet3StateAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
