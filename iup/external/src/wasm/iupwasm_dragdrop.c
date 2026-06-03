/** \file
 * \brief WebAssembly Drag and Drop
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_classbase.h"

#include "iupwasm_drv.h"


EM_JS(void, iupwasmJsDragSource, (int id, int enable), {
  globalThis.__iupApply({ op: 'dragsource', id: id, enable: enable });
})

EM_JS(void, iupwasmJsDropTarget, (int id, int enable), {
  globalThis.__iupApply({ op: 'droptarget', id: id, enable: enable });
})

EM_JS(void, iupwasmJsDropFilesTarget, (int id), {
  globalThis.__iupApply({ op: 'dropfilestarget', id: id });
})


EMSCRIPTEN_KEEPALIVE void iupwasmDndDragBegin(int id, int x, int y)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnii cb;
  if (!ih) return;
  cb = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
  if (cb) cb(ih, x, y);
}

EMSCRIPTEN_KEEPALIVE void iupwasmDndDragEnd(int id, int action)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFni cb;
  if (!ih) return;
  cb = (IFni)IupGetCallback(ih, "DRAGEND_CB");
  if (cb) cb(ih, action);
}

EMSCRIPTEN_KEEPALIVE void iupwasmDndDropMotion(int id, int x, int y)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFniis cb;
  if (!ih) return;
  cb = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
  if (cb) cb(ih, x, y, (char*)"");
}

EMSCRIPTEN_KEEPALIVE void iupwasmDndDrop(int id, const char* data, int size, int x, int y)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnsViii cb;
  char* type;
  if (!ih) return;
  cb = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
  type = iupAttribGet(ih, "DROPTYPES");
  if (cb) cb(ih, type ? type : (char*)"", (void*)data, size, x, y);
}

EMSCRIPTEN_KEEPALIVE void iupwasmDndDropFile(int id, const char* name, int num, int x, int y)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnsiii cb;
  if (!ih) return;
  cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
  if (cb) cb(ih, (char*)name, num, x, y);
}


static int wasmSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
  {
    if (IupClassMatch(ih, "list"))
      iupwasmJsListDndSource(id, iupStrBoolean(value));
    else if (IupClassMatch(ih, "tree"))
      iupwasmJsTreeDndSource(id, iupStrBoolean(value));
    else
      iupwasmJsDragSource(id, iupStrBoolean(value));
  }
  return 1;
}

static int wasmSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
  {
    if (IupClassMatch(ih, "list"))
      iupwasmJsListDndTarget(id, iupStrBoolean(value));
    else if (IupClassMatch(ih, "tree"))
      iupwasmJsTreeDndTarget(id, iupStrBoolean(value));
    else
      iupwasmJsDropTarget(id, iupStrBoolean(value));
  }
  return 1;
}

static int wasmSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id && iupStrBoolean(value))
    iupwasmJsDropFilesTarget(id);
  return 1;
}

IUP_SDK_API void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");
  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");

  iupClassRegisterAttribute(ic, "DRAGTYPES",  NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES",  NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, wasmSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", NULL, wasmSetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, wasmSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, wasmSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
