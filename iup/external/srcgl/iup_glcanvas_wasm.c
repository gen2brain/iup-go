/** \file
 * \brief iupgl control for WebAssembly (Emscripten WebGL).
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <emscripten.h>
#include <emscripten/html5_webgl.h>
#include <GLES3/gl3.h>

#include "iup.h"
#include "iupgl.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"

#include "iupwasm_drv.h"


typedef struct _IGlControlData
{
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
  char target[32]; /* CSS selector of the canvas, e.g. "#iupglcanvas_7" */
} IGlControlData;

EM_JS(int, iupwasmJsGLIsWorker, (void), { return (typeof document === 'undefined') ? 1 : 0; })

EM_JS(void, iupwasmJsGLSetId, (int id), {
  if (typeof document === 'undefined') return;
  var el = globalThis.__iup.els[id];
  if (el) el.id = "iupglcanvas_" + id;
})

/* writing width/height clears the buffer, so only write on a real change */
EM_JS(void, iupwasmJsGLSyncSize, (int id), {
  if (typeof document === 'undefined') {
    var off = globalThis.__iupLocal && globalThis.__iupLocal[id];
    if (!off) return;
    var ow = Math.max(1, globalThis.__iupReadSync({ op: 'canvasclientw', id: id }));
    var oh = Math.max(1, globalThis.__iupReadSync({ op: 'canvasclienth', id: id }));
    if (off.width !== ow) off.width = ow;
    if (off.height !== oh) off.height = oh;
    return;
  }
  var el = globalThis.__iup.els[id];
  if (!el) return;
  var w = Math.max(1, el.clientWidth || el.width);
  var h = Math.max(1, el.clientHeight || el.height);
  if (el.width !== w) el.width = w;
  if (el.height !== h) el.height = h;
})

/* the worker renders to a local OffscreenCanvas (blitted on swap); hand its context to emscripten's GL layer */
EM_JS(int, iupwasmJsGLRegisterContext, (int id, int alpha, int depth, int stencil, int antialias, int single, int major), {
  globalThis.__iupLocal = globalThis.__iupLocal || {};
  var off = globalThis.__iupLocal[id];
  if (!off) { off = new OffscreenCanvas(1, 1); globalThis.__iupLocal[id] = off; }
  var attrs = { alpha: !!alpha, depth: !!depth, stencil: !!stencil, antialias: !!antialias,
                preserveDrawingBuffer: !!single, majorVersion: major, minorVersion: 0 };
  var ctx = off.getContext(major >= 2 ? "webgl2" : "webgl", attrs);
  if (!ctx && major >= 2) { attrs.majorVersion = 1; ctx = off.getContext("webgl", attrs); }
  if (!ctx) return 0;
  return GL.registerContext(ctx, attrs);
})

static int wasmGLCanvasCreateMethod(Ihandle* ih, void** params)
{
  IGlControlData* gldata;
  (void)params;
  gldata = (IGlControlData*)calloc(1, sizeof(IGlControlData));
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", (char*)gldata);
  return IUP_NOERROR;
}

static void wasmGLCanvasUnMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (gldata && gldata->context)
  {
    emscripten_webgl_destroy_context(gldata->context);
    gldata->context = 0;
    gldata->target[0] = 0;
  }
  iupAttribSet(ih, "CONTEXT", NULL);
}

static void wasmGLCanvasDestroy(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (gldata)
    free(gldata);
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", NULL);
}

void iupGlCanvasInitClass(Iclass* ic)
{
  ic->Create  = wasmGLCanvasCreateMethod;
  ic->Destroy = wasmGLCanvasDestroy;
  ic->UnMap   = wasmGLCanvasUnMapMethod;
}

IUPGL_API void IupGLMakeCurrent(Ihandle* ih)
{
  IGlControlData* gldata;
  int id;
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih)) return;
  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas")) return;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  id = iupwasmIdOf(ih);
  if (!gldata || !id)
  {
    iupAttribSet(ih, "ERROR", "Context not available: canvas not mapped.");
    return;
  }

  if (!gldata->context)
  {
    EmscriptenWebGLContextAttributes attrs;
    char* version;
    emscripten_webgl_init_context_attributes(&attrs);

    attrs.alpha = (iupAttribGetInt(ih, "ALPHA_SIZE") > 0) ? EM_TRUE : EM_FALSE;
    attrs.depth = (iupAttribGetInt(ih, "DEPTH_SIZE") != 0) ? EM_TRUE : EM_FALSE;
    attrs.stencil = (iupAttribGetInt(ih, "STENCIL_SIZE") > 0) ? EM_TRUE : EM_FALSE;
    attrs.antialias = iupAttribGetBoolean(ih, "ANTIALIAS") ? EM_TRUE : EM_FALSE;
    attrs.preserveDrawingBuffer = iupStrEqualNoCase(iupAttribGetStr(ih, "BUFFER"), "SINGLE") ? EM_TRUE : EM_FALSE;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;

    version = iupAttribGetStr(ih, "CONTEXTVERSION");
    if (version)
    {
      int major = 0, minor = 0;
      iupStrToIntInt(version, &major, &minor, '.');
      if (major > 0 && major <= 2)
        attrs.majorVersion = 1;
    }

    if (iupwasmJsGLIsWorker())
    {
      gldata->context = (EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)(intptr_t)iupwasmJsGLRegisterContext(id,
        attrs.alpha, attrs.depth, attrs.stencil, attrs.antialias, attrs.preserveDrawingBuffer, attrs.majorVersion);
    }
    else
    {
      iupwasmJsGLSetId(id);
      snprintf(gldata->target, sizeof(gldata->target), "#iupglcanvas_%d", id);
      gldata->context = emscripten_webgl_create_context(gldata->target, &attrs);
      if (gldata->context <= 0 && attrs.majorVersion == 2)
      {
        attrs.majorVersion = 1;
        gldata->context = emscripten_webgl_create_context(gldata->target, &attrs);
      }
    }

    if (gldata->context <= 0)
    {
      iupAttribSet(ih, "ERROR", "Could not create WebGL context.");
      gldata->context = 0;
      gldata->target[0] = 0;
      return;
    }
    iupAttribSet(ih, "CONTEXT", (char*)(intptr_t)gldata->context);
  }

  if (emscripten_webgl_make_context_current(gldata->context) != EMSCRIPTEN_RESULT_SUCCESS)
  {
    iupAttribSet(ih, "ERROR", "Failed to make WebGL context current.");
    return;
  }

  iupwasmJsGLSyncSize(id);

  if (!IupGetGlobal("GL_VERSION"))
  {
    const char* vendor   = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version  = (const char*)glGetString(GL_VERSION);
    if (vendor)   IupSetStrGlobal("GL_VENDOR",   vendor);
    if (renderer) IupSetStrGlobal("GL_RENDERER", renderer);
    if (version)  IupSetStrGlobal("GL_VERSION",  version);
  }

  iupAttribSet(ih, "ERROR", NULL);
}

IUPGL_API int IupGLIsCurrent(Ihandle* ih)
{
  IGlControlData* gldata;
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih)) return 0;
  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas")) return 0;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->context) return 0;
  return emscripten_webgl_get_current_context() == gldata->context ? 1 : 0;
}

IUPGL_API void IupGLSwapBuffers(Ihandle* ih)
{
  Icallback cb;
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih)) return;
  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas")) return;

  cb = IupGetCallback(ih, "SWAPBUFFERS_CB");
  if (cb) cb(ih);
  iupwasmJsCanvasBlit(iupwasmIdOf(ih));
}

/* IupGLPalette/IupGLUseFont have no WebGL equivalent (no indexed color or display-list fonts). */
IUPGL_API void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
{ (void)ih; (void)index; (void)r; (void)g; (void)b; }

IUPGL_API void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
{ (void)ih; (void)first; (void)count; (void)list_base; }

IUPGL_API void IupGLWait(int gl)
{
  if (gl) glFinish();
  else    glFlush();
}

EMSCRIPTEN_KEEPALIVE int iupwasmGLDomId(Ihandle* ih)
{
  if (!iupObjectCheck(ih) || !IupClassMatch(ih, "glcanvas"))
    return 0;
  return iupwasmIdOf(ih);
}
