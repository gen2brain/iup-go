/** \file
 * \brief WebAssembly Label
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_image.h"
#include "iup_label.h"

#include "iupwasm_drv.h"


EM_JS(void, iupwasmJsLabelSeparator, (int id, int horizontal), {
  globalThis.__iupApply({ op: 'labelsep', id: id, horizontal: horizontal });
})

EM_JS(void, iupwasmJsLabelWrap, (int id, int wrap), {
  globalThis.__iupApply({ op: 'labelwrap', id: id, wrap: wrap });
})

EM_JS(void, iupwasmJsLabelSelectable, (int id, int on), {
  globalThis.__iupApply({ op: 'labelselect', id: id, on: on });
})

EM_JS(void, iupwasmJsLabelEllipsis, (int id, int on), {
  globalThis.__iupApply({ op: 'labelellipsis', id: id, on: on });
})

static int wasmLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && ih->data->type == IUP_LABEL_TEXT)
  {
    if (iupAttribGetBoolean(ih, "MARKUP"))
      iupwasmJsSetMarkup(id, value);
    else
    {
      char* clean = iupwasmStripMnemonic(value);
      iupwasmJsSetText(id, clean);
      free(clean);
    }
  }
  return 1;
}

static int wasmLabelSetMarkupAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  char* title = iupAttribGet(ih, "TITLE");
  if (id && title && ih->data->type == IUP_LABEL_TEXT && iupStrBoolean(value))
    iupwasmJsSetMarkup(id, title);
  return 1;
}

static int wasmLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (ih->data->type != IUP_LABEL_IMAGE)
    return 0;
  if (id)
  {
    void* img = iupImageGetImage(value, ih, 0, NULL);
    if (img)
      iupwasmJsSetImage(id, (int)(intptr_t)img);
  }
  return 1;
}

static int wasmLabelSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  void* img = value ? iupImageGetImage(value, ih, 0, NULL) : NULL;
  if (id && ih->data->type == IUP_LABEL_IMAGE)
    iupwasmJsSetInactiveImage(id, img ? (int)(intptr_t)img : 0, 1);
  return 1;
}

static int wasmLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsSetTextAlign(id, iupwasmCssAlign(value));
  return 1;
}

static int wasmLabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsLabelWrap(id, iupStrBoolean(value));
  return 1;
}

static int wasmLabelSetSelectableAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsLabelSelectable(id, iupStrBoolean(value));
  return 1;
}

static int wasmLabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsLabelEllipsis(id, iupStrBoolean(value));
  return 1;
}

static int wasmLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  int h = 0, v = 0;
  iupStrToIntInt(value, &h, &v, 'x');
  if (id)
    iupwasmJsSetPadding(id, h, v);
  return 1;
}

static int wasmLabelMapMethod(Ihandle* ih)
{
  char* value;
  int id = iupwasmJsCreate("div");
  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);
  iupwasmJsLabelStyle(id);

  value = iupAttribGet(ih, "SEPARATOR");
  if (value)
  {
    int horiz = iupStrEqualNoCase(value, "HORIZONTAL");
    ih->data->type = horiz ? IUP_LABEL_SEP_HORIZ : IUP_LABEL_SEP_VERT;
    iupwasmJsLabelSeparator(id, horiz);
  }
  else if (iupAttribGet(ih, "IMAGE"))
  {
    char* name = iupAttribGet(ih, "IMAGE");
    void* img = iupImageGetImage(name, ih, 0, NULL);
    ih->data->type = IUP_LABEL_IMAGE;
    if (img)
      iupwasmJsSetImage(id, (int)(intptr_t)img);
  }
  else
  {
    char* title = iupAttribGet(ih, "TITLE");
    ih->data->type = IUP_LABEL_TEXT;
    if (title)
    {
      char* clean = iupwasmStripMnemonic(title);
      iupwasmJsSetText(id, clean);
      free(clean);
    }
  }

  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvLabelAddExtraPadding(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  (void)x;
  (void)y;
}

IUP_SDK_API void iupdrvLabelInitClass(Iclass* ic)
{
  ic->Map = wasmLabelMapMethod;
  ic->UnMap = iupdrvBaseUnMapMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "TITLE", NULL, wasmLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, wasmLabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, wasmLabelSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, wasmLabelSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT:ACENTER", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, wasmLabelSetWordWrapAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, wasmLabelSetEllipsisAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTABLE", NULL, wasmLabelSetSelectableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, wasmLabelSetMarkupAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", NULL, wasmLabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, NULL, NULL, IUPAF_DEFAULT);
}
