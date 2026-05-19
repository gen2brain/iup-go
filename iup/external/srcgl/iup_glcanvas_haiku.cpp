/** \file
 * \brief IupGLCanvas for Haiku (BGLView).
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <Application.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <Messenger.h>
#include <SupportDefs.h>
#include <Window.h>

#include <GL/gl.h>
#include <GLView.h>

extern "C" {
#include "iup.h"
#include "iupgl.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
}

#include "iuphaiku_drv.h"


/* BGLView Lock/Unlock is thread-bound; marshal Draw/FrameResized to be_app. */
#define IUPHAIKU_GL_REDRAW_MSG  'IGlR'
#define IUPHAIKU_GL_RESIZE_MSG  'IGlS'

class IupHaikuGLView;

typedef struct _IGlControlData
{
  IupHaikuGLView* view;
  BHandler* bridge;
  thread_id locker;  /* -1 = unlocked */
  int vsync;
} IGlControlData;


class IupHaikuGLView : public BGLView
{
public:
  IupHaikuGLView(Ihandle* ih, ulong options)
    : BGLView(BRect(0, 0, 0, 0), "iup_glcanvas", B_FOLLOW_NONE,
              B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE,
              options),
      fIhandle(ih)
  {
    BView::SetViewColor(B_TRANSPARENT_COLOR);
  }

  void SetIhandle(Ihandle* ih) { fIhandle = ih; }

  void Draw(BRect dirty) override
  {
    if (fIhandle && iupObjectCheck(fIhandle))
    {
      IGlControlData* gldata = (IGlControlData*)iupAttribGet(fIhandle, "_IUP_GLCONTROLDATA");
      if (gldata && gldata->bridge)
      {
        BMessage m(IUPHAIKU_GL_REDRAW_MSG);
        m.AddRect("dirty", dirty);
        BMessage reply;
        BMessenger(gldata->bridge).SendMessage(&m, &reply);
      }
    }
    BGLView::Draw(dirty);
  }

  void FrameResized(float new_w, float new_h) override
  {
    BGLView::FrameResized(new_w, new_h);
    if (!fIhandle || !iupObjectCheck(fIhandle)) return;
    IGlControlData* gldata = (IGlControlData*)iupAttribGet(fIhandle, "_IUP_GLCONTROLDATA");
    if (!gldata || !gldata->bridge) return;
    BMessage m(IUPHAIKU_GL_RESIZE_MSG);
    m.AddFloat("w", new_w);
    m.AddFloat("h", new_h);
    BMessenger(gldata->bridge).SendMessage(&m);
  }

  void AttachedToWindow() override
  {
    BGLView::AttachedToWindow();
    iuphaikuCanvasOnAttachedToWindow(fIhandle, this);
  }

  void MakeFocus(bool focused = true) override
  {
    BGLView::MakeFocus(focused);
    iuphaikuCanvasOnMakeFocus(fIhandle, this, focused);
  }

  void MouseDown(BPoint where) override
  {
    iuphaikuCanvasOnMouseDown(fIhandle, this, where);
  }

  void MouseUp(BPoint where) override
  {
    iuphaikuCanvasOnMouseUp(fIhandle, this, where);
  }

  void MouseMoved(BPoint where, uint32 transit, const BMessage* drag) override
  {
    iuphaikuCanvasOnMouseMoved(fIhandle, this, where, transit, drag);
  }

  void KeyDown(const char* bytes, int32 numBytes) override
  {
    if (!iuphaikuCanvasOnKeyDown(fIhandle, this, bytes, numBytes))
      BGLView::KeyDown(bytes, numBytes);
  }

  void KeyUp(const char* bytes, int32 numBytes) override
  {
    if (!iuphaikuCanvasOnKeyUp(fIhandle, this, bytes, numBytes))
      BGLView::KeyUp(bytes, numBytes);
  }

  void MessageReceived(BMessage* msg) override
  {
    if (!iuphaikuCanvasOnMessageReceived(fIhandle, this, msg))
      BGLView::MessageReceived(msg);
  }

private:
  Ihandle* fIhandle;
};


static IGlControlData* s_current_gl = NULL;

static void haikuGLReleaseLock(IGlControlData* gldata)
{
  if (!gldata || !gldata->view) return;
  if (gldata->locker == -1) return;
  if (gldata->locker != find_thread(NULL)) return;
  gldata->view->UnlockGL();
  gldata->locker = -1;
  if (s_current_gl == gldata) s_current_gl = NULL;
}


class IupHaikuGLBridge : public BHandler
{
public:
  explicit IupHaikuGLBridge(Ihandle* ih) : BHandler("iup_glbridge"), fIhandle(ih) {}

  void MessageReceived(BMessage* msg) override
  {
    if (!msg) { BHandler::MessageReceived(msg); return; }

    if (msg->what == IUPHAIKU_GL_REDRAW_MSG)
    {
      if (fIhandle && iupObjectCheck(fIhandle))
      {
        IGlControlData* gldata = (IGlControlData*)iupAttribGet(fIhandle, "_IUP_GLCONTROLDATA");
        if (gldata && gldata->view)
        {
          /* Drop stale lock so ACTION's MakeCurrent rebinds to current bounds. */
          haikuGLReleaseLock(gldata);
          BRect dirty;
          msg->FindRect("dirty", &dirty);
          iuphaikuCanvasOnDraw(fIhandle, gldata->view, dirty);
        }
      }
      msg->SendReply('IGlr');
      return;
    }

    if (msg->what == IUPHAIKU_GL_RESIZE_MSG)
    {
      if (fIhandle && iupObjectCheck(fIhandle))
      {
        IGlControlData* gldata = (IGlControlData*)iupAttribGet(fIhandle, "_IUP_GLCONTROLDATA");
        if (gldata && gldata->view)
        {
          haikuGLReleaseLock(gldata);
          float w = 0, h = 0;
          msg->FindFloat("w", &w);
          msg->FindFloat("h", &h);
          iuphaikuCanvasOnFrameResized(fIhandle, gldata->view, w, h);
        }
      }
      return;
    }

    BHandler::MessageReceived(msg);
  }

private:
  Ihandle* fIhandle;
};


static int haikuGLDefaultResize_CB(Ihandle* ih, int width, int height)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->view) return IUP_DEFAULT;

  IupGLMakeCurrent(ih);
  glViewport(0, 0, width, height);
  return IUP_DEFAULT;
}

static int haikuGLCanvasCreateMethod(Ihandle* ih, void** params)
{
  (void)params;
  IGlControlData* gldata = (IGlControlData*)malloc(sizeof(IGlControlData));
  memset(gldata, 0, sizeof(IGlControlData));
  gldata->vsync = 1;
  gldata->locker = -1;
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", (char*)gldata);

  IupSetCallback(ih, "RESIZE_CB", (Icallback)haikuGLDefaultResize_CB);

  return IUP_NOERROR;
}

static ulong haikuGLBuildOptions(Ihandle* ih)
{
  ulong opts = BGL_RGB;

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "COLOR"), "INDEX"))
    iupAttribSet(ih, "ERROR", "WARNING: Indexed color mode not supported on Haiku. Using RGBA instead.");

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "BUFFER"), "DOUBLE"))
    opts |= BGL_DOUBLE;

  if (iupAttribGetInt(ih, "DEPTH_SIZE") > 0 || !iupAttribGet(ih, "DEPTH_SIZE"))
    opts |= BGL_DEPTH;

  if (iupAttribGetInt(ih, "STENCIL_SIZE") > 0)
    opts |= BGL_STENCIL;

  if (iupAttribGetInt(ih, "ALPHA_SIZE") > 0)
    opts |= BGL_ALPHA;

  if (iupAttribGetInt(ih, "ACCUM_RED_SIZE") > 0 ||
      iupAttribGetInt(ih, "ACCUM_GREEN_SIZE") > 0 ||
      iupAttribGetInt(ih, "ACCUM_BLUE_SIZE") > 0 ||
      iupAttribGetInt(ih, "ACCUM_ALPHA_SIZE") > 0)
    opts |= BGL_ACCUM;

  Ihandle* shared = IupGetAttributeHandle(ih, "SHAREDCONTEXT");
  if (shared && IupClassMatch(shared, "glcanvas"))
    opts |= BGL_SHARE_CONTEXT;

  return opts;
}

static int haikuGLCanvasMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  iupAttribSet(ih, "ERROR", NULL);

  /* Parent (canvas) Map ran first; replace its BView with a BGLView. */
  BView* canvas_view = (BView*)ih->handle;
  if (canvas_view)
  {
    {
      LooperLockGuard guard(canvas_view->Looper());
      canvas_view->RemoveSelf();
    }
    delete canvas_view;
    ih->handle = NULL;
  }

  ulong opts = haikuGLBuildOptions(ih);
  gldata->view = new IupHaikuGLView(ih, opts);
  ih->handle = (InativeHandle*)gldata->view;

  if (be_app)
  {
    gldata->bridge = new IupHaikuGLBridge(ih);
    LooperLockGuard guard(be_app);
    be_app->AddHandler(gldata->bridge);
  }

  iuphaikuAddToParent(ih);

  if (iupAttribGet(ih, "VSYNC"))
    gldata->vsync = iupStrBoolean(iupAttribGetStr(ih, "VSYNC")) ? 1 : 0;

  iupAttribSet(ih, "CONTEXT", (char*)gldata->view);

  return IUP_NOERROR;
}

static void haikuGLCanvasUnMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata) return;

  haikuGLReleaseLock(gldata);

  if (gldata->bridge)
  {
    if (be_app)
    {
      LooperLockGuard guard(be_app);
      be_app->RemoveHandler(gldata->bridge);
    }
    delete gldata->bridge;
    gldata->bridge = NULL;
  }

  if (gldata->view)
  {
    gldata->view->SetIhandle(NULL);
    gldata->view = NULL;
  }

  iupdrvBaseUnMapMethod(ih);

  iupAttribSet(ih, "CONTEXT", NULL);
}

static void haikuGLCanvasDestroy(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (gldata)
  {
    free(gldata);
    iupAttribSet(ih, "_IUP_GLCONTROLDATA", NULL);
  }
}

static int haikuGLCanvasSetVSyncAttrib(Ihandle* ih, const char* value)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (gldata)
    gldata->vsync = iupStrBoolean(value) ? 1 : 0;
  return 1;
}

IUPGL_API int IupGLIsCurrent(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih)) return 0;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas"))
    return 0;

  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->view) return 0;

  return (gldata->locker == find_thread(NULL)) ? 1 : 0;
}

IUPGL_API void IupGLMakeCurrent(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih)) return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas"))
    return;

  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->view)
  {
    iupAttribSet(ih, "ERROR", "Context not available: canvas not mapped.");
    return;
  }

  if (!gldata->view->Window())
    return;

  thread_id me = find_thread(NULL);

  if (gldata->locker == me)
  {
    if (s_current_gl && s_current_gl != gldata)
      haikuGLReleaseLock(s_current_gl);
    s_current_gl = gldata;
    iupAttribSet(ih, "ERROR", NULL);
    return;
  }

  if (gldata->locker != -1)
  {
    iupAttribSet(ih, "ERROR", "GL context locked by another thread.");
    return;
  }

  if (s_current_gl && s_current_gl != gldata)
    haikuGLReleaseLock(s_current_gl);

  gldata->view->LockGL();
  gldata->locker = me;
  s_current_gl = gldata;

  iupAttribSet(ih, "ERROR", NULL);

  if (!IupGetGlobal("GL_VERSION"))
  {
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);

    if (vendor)   IupSetStrGlobal("GL_VENDOR", vendor);
    if (renderer) IupSetStrGlobal("GL_RENDERER", renderer);
    if (version)  IupSetStrGlobal("GL_VERSION", version);
  }
}

IUPGL_API void IupGLSwapBuffers(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih)) return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas"))
    return;

  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->view || !gldata->view->Window()) return;

  Icallback cb = IupGetCallback(ih, "SWAPBUFFERS_CB");
  if (cb) cb(ih);

  gldata->view->SwapBuffers(gldata->vsync != 0);

  haikuGLReleaseLock(gldata);
}

IUPGL_API void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
{
  (void)ih; (void)index; (void)r; (void)g; (void)b;
}

IUPGL_API void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
{
  (void)ih; (void)first; (void)count; (void)list_base;
}

IUPGL_API void IupGLWait(int gl)
{
  if (gl) glFinish();
}

extern "C" void iupGlCanvasInitClass(Iclass* ic)
{
  ic->Create = haikuGLCanvasCreateMethod;
  ic->Destroy = haikuGLCanvasDestroy;
  ic->Map = haikuGLCanvasMapMethod;
  ic->UnMap = haikuGLCanvasUnMapMethod;

  iupClassRegisterAttribute(ic, "VSYNC", NULL, haikuGLCanvasSetVSyncAttrib, "YES", NULL, IUPAF_NO_INHERIT);
}
