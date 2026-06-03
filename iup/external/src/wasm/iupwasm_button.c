/** \file
 * \brief WebAssembly Button
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_image.h"
#include "iup_button.h"

#include "iupwasm_drv.h"


static int wasmButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
  {
    if (iupAttribGetBoolean(ih, "MARKUP"))
      iupwasmJsSetMarkup(id, value);
    else
    {
      char* clean = iupwasmStripMnemonic(value);
      iupwasmJsButtonSetText(id, clean);
      free(clean);
    }
  }
  return 1;
}

static int wasmButtonSetMarkupAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  char* title = iupAttribGet(ih, "TITLE");
  if (id && title && iupStrBoolean(value))
    iupwasmJsSetMarkup(id, title);
  return 1;
}

static int wasmButtonIsFlat(Ihandle* ih)
{
  return iupAttribGet(ih, "IMAGE") && iupAttribGet(ih, "IMPRESS") &&
         !iupAttribGetBoolean(ih, "IMPRESSBORDER");
}

static int wasmButtonSetImageAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  void* img = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (id && img)
    iupwasmJsButtonSetImage(id, (int)(intptr_t)img, ih->data->spacing);
  return 1;
}

static int wasmButtonSetImpressAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  void* img = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (id)
    iupwasmJsButtonSetImpress(id, img ? (int)(intptr_t)img : 0);
  return 1;
}

static int wasmButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  void* img = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (id)
    iupwasmJsSetInactiveImage(id, img ? (int)(intptr_t)img : 0, 0);
  return 1;
}

static int wasmButtonSetDefaultAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsButtonDefault(id, iupStrBoolean(value));
  return 1;
}

static int wasmButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  int h = 0, v = 0;
  iupStrToIntInt(value, &h, &v, 'x');
  if (id)
    iupwasmJsSetPadding(id, h, v);
  return 1;
}

static int wasmButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsButtonAlign(id, iupwasmCssAlign(value));
  return 1;
}

static int wasmButtonMapMethod(Ihandle* ih)
{
  char* title;
  char* image;
  int id = iupwasmJsCreate("button");
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  iupwasmJsButtonStyle(id);

  title = iupAttribGet(ih, "TITLE");
  image = iupAttribGet(ih, "IMAGE");

  if (image)
  {
    ih->data->type = IUP_BUTTON_IMAGE;
    if (title && *title)
      ih->data->type |= IUP_BUTTON_TEXT;
  }
  else
    ih->data->type = IUP_BUTTON_TEXT;

  if (title)
  {
    char* clean = iupwasmStripMnemonic(title);
    iupwasmJsButtonSetText(id, clean);
    free(clean);
  }

  if (image)
  {
    void* img = iupImageGetImage(image, ih, 0, NULL);
    if (img)
      iupwasmJsButtonSetImage(id, (int)(intptr_t)img, ih->data->spacing);
  }

  if (wasmButtonIsFlat(ih))
    iupwasmJsButtonFlat(id);

  {
    char* impress = iupAttribGet(ih, "IMPRESS");
    void* pimg = impress ? iupImageGetImage(impress, ih, 0, NULL) : NULL;
    if (pimg)
      iupwasmJsButtonSetImpress(id, (int)(intptr_t)pimg);
  }

  iupwasmJsWireClick(id);
  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvButtonAddBorders(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  if (x) *x += 16;
  if (y) *y += 8;
}

IUP_SDK_API void iupdrvButtonInitClass(Iclass* ic)
{
  ic->Map = wasmButtonMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, wasmButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, wasmButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMPRESS", NULL, wasmButtonSetImpressAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, wasmButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWASDEFAULT", NULL, wasmButtonSetDefaultAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", NULL, wasmButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, wasmButtonSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ACENTER:ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, wasmButtonSetMarkupAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
}
