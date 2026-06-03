/** \file
 * \brief WebAssembly Message Loop
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_loop.h"
#include "iup_attrib.h"
#include "iup_object.h"
#include "iup_dlglist.h"
#include "iup_dialog.h"
#include "iup_drv.h"

#include "iupwasm_drv.h"


static IFidle wasm_idle_cb = NULL;
static int wasm_loop_level = 0;

#define WASM_MODAL_MAX 16
static Ihandle* wasm_modal_stack[WASM_MODAL_MAX];
static int wasm_modal_top = 0;

EM_JS(void, iupwasmJsSetIdle, (int on), {
  if (globalThis.__iupIdle) { clearTimeout(globalThis.__iupIdle); globalThis.__iupIdle = 0; }
  if (on) {
    var tick = function() {
      globalThis.__iupIdle = Module._iupwasmDispatchIdle() ? setTimeout(tick, 0) : 0;
    };
    globalThis.__iupIdle = setTimeout(tick, 0);
  }
})

IUP_SDK_API void iupdrvSetIdleFunction(Icallback f)
{
  wasm_idle_cb = (IFidle)f;
  iupwasmJsSetIdle(f ? 1 : 0);
}

EMSCRIPTEN_KEEPALIVE int iupwasmDispatchIdle(void)
{
  int ret;
  if (!wasm_idle_cb)
    return 0;
  ret = wasm_idle_cb();
  if (ret == IUP_CLOSE)
  {
    wasm_idle_cb = NULL;
    IupExitLoop();
    return 0;
  }
  if (ret == IUP_IGNORE)
  {
    wasm_idle_cb = NULL;
    return 0;
  }
  return 1;
}

EM_JS(void, iupwasmJsRunPump, (void), {
  if (globalThis.__iupRunPump) globalThis.__iupRunPump();
})

EM_JS(void, iupwasmJsExitLoop, (void), {
  if (globalThis.__iupExitLoop) globalThis.__iupExitLoop();
})

void IupExitLoop(void)
{
  iupwasmJsExitLoop();
}

int IupMainLoopLevel(void)
{
  return wasm_loop_level;
}

/* A nested IupMainLoop is a modal; back its MODAL dialog (none at top level). */
static void wasmModalBackdropPush(void)
{
  Ihandle* found = NULL;
  Ihandle* ih;
  for (ih = iupDlgListFirst(); ih; ih = iupDlgListNext())
  {
    int marked = 0, k;
    if (!ih->handle || !iupdrvDialogIsVisible(ih) || !iupAttribGetBoolean(ih, "MODAL"))
      continue;
    for (k = 0; k < wasm_modal_top; k++)
      if (wasm_modal_stack[k] == ih) { marked = 1; break; }
    if (!marked)
      found = ih;
  }
  if (wasm_modal_top < WASM_MODAL_MAX)
    wasm_modal_stack[wasm_modal_top++] = found;
  if (found)
    iupwasmDialogModalShow(found, 1);
}

static void wasmModalBackdropPop(void)
{
  Ihandle* ih;
  if (wasm_modal_top <= 0)
    return;
  ih = wasm_modal_stack[--wasm_modal_top];
  if (ih && iupObjectCheck(ih))
    iupwasmDialogModalShow(ih, 0);
}

/* Blocks: top-level for a C app, every modal for both. Go keeps its top-level
   loop on the async JS event loop and never calls this. */
int IupMainLoop(void)
{
  static int called_entry = 0;
  if (!called_entry)
  {
    called_entry = 1;
    iupLoopCallEntryCb();
  }

  wasm_loop_level++;
  wasmModalBackdropPush();
  iupwasmJsRunPump();
  wasmModalBackdropPop();
  wasm_loop_level--;
  return IUP_NOERROR;
}

int IupLoopStepWait(void)
{
  return IUP_DEFAULT;
}

int IupLoopStep(void)
{
  return IUP_DEFAULT;
}

void IupFlush(void)
{
}

EM_JS(void, iupwasmJsPostMessage, (Ihandle* ih, const char* s, int i, double d, void* p), {
  self.postMessage({ __iupPost: 1, ih: ih, s: s ? UTF8ToString(s) : "", i: i, d: d, p: p });
})

EMSCRIPTEN_KEEPALIVE void iupwasmDispatchPost(Ihandle* ih, const char* s, int i, double d, void* p)
{
  IFnsidv cb;
  if (!iupObjectCheck(ih))
    return;
  cb = (IFnsidv)IupGetCallback(ih, "POSTMESSAGE_CB");
  if (cb)
    cb(ih, (char*)s, i, d, p);
}

#ifdef __EMSCRIPTEN_PTHREADS__
#include <emscripten/threading.h>

typedef struct WasmPost { Ihandle* ih; char* s; int i; double d; void* p; struct WasmPost* next; } WasmPost;
static WasmPost* wasm_post_head = NULL;
static WasmPost* wasm_post_tail = NULL;
static pthread_mutex_t wasm_post_mutex = PTHREAD_MUTEX_INITIALIZER;

static void wasmPostEnqueue(Ihandle* ih, const char* s, int i, double d, void* p)
{
  WasmPost* n = (WasmPost*)malloc(sizeof(WasmPost));
  n->ih = ih; n->s = s ? strdup(s) : NULL; n->i = i; n->d = d; n->p = p; n->next = NULL;
  pthread_mutex_lock(&wasm_post_mutex);
  if (wasm_post_tail) wasm_post_tail->next = n; else wasm_post_head = n;
  wasm_post_tail = n;
  pthread_mutex_unlock(&wasm_post_mutex);
}

EMSCRIPTEN_KEEPALIVE void iupwasmDrainPosts(void)
{
  for (;;)
  {
    WasmPost* n;
    pthread_mutex_lock(&wasm_post_mutex);
    n = wasm_post_head;
    if (n) { wasm_post_head = n->next; if (!wasm_post_head) wasm_post_tail = NULL; }
    pthread_mutex_unlock(&wasm_post_mutex);
    if (!n) break;
    iupwasmDispatchPost(n->ih, n->s, n->i, n->d, n->p);
    free(n->s);
    free(n);
  }
}

void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  if (emscripten_is_main_runtime_thread())
    iupwasmJsPostMessage(ih, s, i, d, p);
  else
    wasmPostEnqueue(ih, s, i, d, p);
}

#else

EMSCRIPTEN_KEEPALIVE void iupwasmDrainPosts(void) { }

void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p)
{
  iupwasmJsPostMessage(ih, s, i, d, p);
}

#endif
