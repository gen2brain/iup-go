/** \file
 * \brief iupgl control for macOS/Cocoa
 *
 * See Copyright Notice in "iup.h"
 */

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/CGLRenderers.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupgl.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_assert.h"
#include "iup_register.h"
#include "iup_canvas.h"

@interface IupGLContext : NSOpenGLContext
{
  int32_t dirty;
}
- (void)scheduleUpdate;
- (void)updateIfNeeded;
- (void)explicitUpdate;
@end

@implementation IupGLContext

- (id)initWithFormat:(NSOpenGLPixelFormat *)format shareContext:(NSOpenGLContext *)share
{
  self = [super initWithFormat:format shareContext:share];
  if (self) {
    dirty = 0;
  }
  return self;
}

- (void)scheduleUpdate
{
  OSAtomicIncrement32Barrier(&dirty);
}

- (void)updateIfNeeded
{
  int32_t value = OSAtomicCompareAndSwap32Barrier(dirty, 0, &dirty) ? dirty : 0;
  if (value > 0) {
    [self explicitUpdate];
  }
}

- (void)update
{
  [self scheduleUpdate];
  [self updateIfNeeded];
}

- (void)explicitUpdate
{
  if ([NSThread isMainThread]) {
    [super update];
  } else {
    dispatch_sync(dispatch_get_main_queue(), ^{
      [super update];
    });
  }
}

@end

typedef struct _IGlControlData
{
  NSView* canvas_view;
  IupGLContext* context;
  NSOpenGLPixelFormat* pixel_format;
} IGlControlData;

static int cocoaGLCanvasDefaultResize_CB(Ihandle *ih, int width, int height)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (gldata && gldata->context) {

    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    /* Make sure context is current before calling GL functions */
    if ([NSOpenGLContext currentContext] != gldata->context)
    {
      [gldata->context makeCurrentContext];
    }

    [gldata->context update];

    /* Set viewport to match view size */
    glViewport(0, 0, width, height);

    [pool drain];
  }
  return IUP_DEFAULT;
}

static int cocoaGLCanvasCreateMethod(Ihandle* ih, void** params)
{
  IGlControlData* gldata;
  (void)params;

  gldata = (IGlControlData*)malloc(sizeof(IGlControlData));
  memset(gldata, 0, sizeof(IGlControlData));
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", (char*)gldata);

  IupSetCallback(ih, "RESIZE_CB", (Icallback)cocoaGLCanvasDefaultResize_CB);

  return IUP_NOERROR;
}

static void buildSoftwareAttributes(Ihandle* ih, NSOpenGLPixelFormatAttribute* attrs, int* n_ptr, BOOL request_core, NSOpenGLPixelFormatAttribute rendererID)
{
  int n = 0;

  /* When using software rendering, NSOpenGLPFANoRecovery prevents automatic renderer switching */
  attrs[n++] = NSOpenGLPFANoRecovery;

  /* Allow offline renderers */
  attrs[n++] = NSOpenGLPFAAllowOfflineRenderers;

  /* Renderer ID if specified */
  if (rendererID != 0)
  {
    attrs[n++] = NSOpenGLPFARendererID;
    attrs[n++] = rendererID;
  }

  /* Profile selection */
  attrs[n++] = NSOpenGLPFAOpenGLProfile;
  if (request_core)
  {
    NSOpenGLPixelFormatAttribute core_profile = 0;
    char* version_attr = iupAttribGetStr(ih, "CONTEXTVERSION");

    if (version_attr)
    {
      if (strcmp(version_attr, "4.1") == 0)
      {
        if (@available(macOS 10.10, *))
          core_profile = NSOpenGLProfileVersion4_1Core;
      }
      else if (strcmp(version_attr, "3.2") == 0)
      {
        if (@available(macOS 10.7, *))
          core_profile = NSOpenGLProfileVersion3_2Core;
      }
    }

    if (core_profile == 0 && !version_attr) /* No version, or unsupported, default to best available */
    {
      /* Default Core: Prefer 4.1 if available, otherwise 3.2 */
      if (@available(macOS 10.10, *))
        core_profile = NSOpenGLProfileVersion4_1Core;
      else if (@available(macOS 10.7, *))
        core_profile = NSOpenGLProfileVersion3_2Core;
    }

    if (core_profile != 0)
    {
      attrs[n++] = core_profile;
    }
    else
    {
      /* Core profile requested but not available. Force failure by using an invalid profile ID. */
      attrs[n++] = 0x9999;
    }
  }
  else
  {
    attrs[n++] = NSOpenGLProfileVersionLegacy;
  }

  /* Check for indexed color mode */
  /* NOTE: Software renderer also doesn't support indexed color on macOS */
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "COLOR"), "INDEX"))
  {
    iupAttribSet(ih, "ERROR", "WARNING: Indexed color mode not supported on macOS. Using RGBA instead.");
  }

  attrs[n++] = NSOpenGLPFAColorSize;
  attrs[n++] = 24;

  int alpha_size = iupAttribGetInt(ih, "ALPHA_SIZE");
  if (alpha_size > 0)
  {
    attrs[n++] = NSOpenGLPFAAlphaSize;
    attrs[n++] = alpha_size;
  }

  int depth_size = iupAttribGetInt(ih, "DEPTH_SIZE");
  attrs[n++] = NSOpenGLPFADepthSize;
  attrs[n++] = (depth_size > 0) ? depth_size : 16;

  int stencil_size = iupAttribGetInt(ih, "STENCIL_SIZE");
  if (stencil_size > 0)
  {
    attrs[n++] = NSOpenGLPFAStencilSize;
    attrs[n++] = stencil_size;
  }

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "BUFFER"), "DOUBLE"))
  {
    attrs[n++] = NSOpenGLPFADoubleBuffer;
  }

  attrs[n] = 0;
  *n_ptr = n;
}

static NSOpenGLPixelFormat* cocoaGLCreatePixelFormatSoftware(Ihandle* ih)
{
  NSOpenGLPixelFormatAttribute attrs[32];
  int n = 0;
  NSOpenGLPixelFormat* pixelFormat = nil;

  char* profile_attr = iupAttribGetStr(ih, "CONTEXTPROFILE");
  char* version_attr = iupAttribGetStr(ih, "CONTEXTVERSION");
  int arbcontext = iupAttribGetBoolean(ih, "ARBCONTEXT");
  int major = 0, minor = 0;
  BOOL core_requested = NO;

  if (version_attr)
  {
    iupStrToIntInt(version_attr, &major, &minor, '.');
  }

  if (profile_attr)
  {
    if (iupStrEqualNoCase(profile_attr, "CORE"))
      core_requested = YES;
    else if (iupStrEqualNoCase(profile_attr, "COMPATIBILITY"))
      core_requested = NO;
  }
  else if (arbcontext)
  {
    core_requested = YES;
  }
  else if (major > 3 || (major == 3 && minor >= 2))
  {
    core_requested = YES;
  }
  /* else: core_requested remains NO (default) */

  if (core_requested)
  {
    /* Try modern software renderer (kCGLRendererGenericFloatID) */
    buildSoftwareAttributes(ih, attrs, &n, YES, (NSOpenGLPixelFormatAttribute)kCGLRendererGenericFloatID);
    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (pixelFormat) return pixelFormat;

    /* If Core was requested and failed, try any software renderer (rendererID=0) that supports Core. */
    buildSoftwareAttributes(ih, attrs, &n, YES, 0);
    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (pixelFormat) return pixelFormat;

    /* All Core software fallbacks failed */
    if (!iupAttribGet(ih, "ERROR"))
    {
      iupAttribSet(ih, "ERROR", "Failed to create Core software OpenGL context.");
    }
    return nil;
  }
  else
  {
    /* Legacy profile: Try older renderer first. */
#ifdef kCGLRendererAppleSWID
    buildSoftwareAttributes(ih, attrs, &n, NO, (NSOpenGLPixelFormatAttribute)kCGLRendererAppleSWID);
    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (pixelFormat) return pixelFormat;
#endif

    /* If old one fails or isn't defined, try modern one (kCGLRendererGenericFloatID) */
    buildSoftwareAttributes(ih, attrs, &n, NO, (NSOpenGLPixelFormatAttribute)kCGLRendererGenericFloatID);
    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (pixelFormat) return pixelFormat;

    /* Last resort: any available software renderer without specific ID */
    buildSoftwareAttributes(ih, attrs, &n, NO, 0);
    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (pixelFormat) return pixelFormat;
  }

  /* All software fallbacks failed */
  if (!iupAttribGet(ih, "ERROR"))
  {
    iupAttribSet(ih, "ERROR", "Failed to create Legacy software OpenGL context.");
  }
  return nil;
}

static NSOpenGLPixelFormat* cocoaGLCreatePixelFormat(Ihandle* ih)
{
  NSOpenGLPixelFormatAttribute attrs[50];
  int n = 0;
  int number;
  BOOL force_software = NO;

  char* profile_attr = iupAttribGetStr(ih, "CONTEXTPROFILE");
  char* version_attr = iupAttribGetStr(ih, "CONTEXTVERSION");
  int arbcontext = iupAttribGetBoolean(ih, "ARBCONTEXT");

  BOOL request_core = NO;
  int major = 0, minor = 0;

  if (version_attr)
  {
    iupStrToIntInt(version_attr, &major, &minor, '.');
  }

  if (profile_attr)
  {
    if (iupStrEqualNoCase(profile_attr, "CORE"))
    {
      request_core = YES;
    }
    else if (iupStrEqualNoCase(profile_attr, "COMPATIBILITY"))
    {
      request_core = NO;
    }
  }
  else if (arbcontext)
  {
    request_core = YES;
  }
  else if (major > 3 || (major == 3 && minor >= 2))
  {
    request_core = YES;
  }
  /* else: request_core remains NO (default) */


  const char* env_force_software = getenv("IUP_GL_FORCE_SOFTWARE");
  if (env_force_software && (env_force_software[0] == '1' || iupStrEqualNoCase(env_force_software, "YES") || iupStrEqualNoCase(env_force_software, "TRUE"))) {
    force_software = YES;
  }

  if (force_software) {
    return cocoaGLCreatePixelFormatSoftware(ih);
  }

  attrs[n++] = NSOpenGLPFAOpenGLProfile;
  if (request_core)
  {
    NSOpenGLPixelFormatAttribute core_profile = 0;

    if (major == 4 && minor == 1)
    {
      if (@available(macOS 10.10, *))
        core_profile = NSOpenGLProfileVersion4_1Core;
    }
    else if (major == 3 && minor == 2)
    {
      if (@available(macOS 10.7, *))
        core_profile = NSOpenGLProfileVersion3_2Core;
    }

    if (core_profile == 0) /* No version, or unsupported, default to best available */
    {
      if (@available(macOS 10.10, *))
        core_profile = NSOpenGLProfileVersion4_1Core;
      else if (@available(macOS 10.7, *))
        core_profile = NSOpenGLProfileVersion3_2Core;
      else
        core_profile = 0x9999; /* Force failure if no core profile is available */
    }

    attrs[n++] = core_profile;
  }
  else
  {
    attrs[n++] = NSOpenGLProfileVersionLegacy;
  }

  attrs[n++] = NSOpenGLPFAAccelerated;
  attrs[n++] = NSOpenGLPFAAllowOfflineRenderers;

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "BUFFER"), "DOUBLE"))
    attrs[n++] = NSOpenGLPFADoubleBuffer;

  if (iupAttribGetBoolean(ih, "STEREO"))
    attrs[n++] = NSOpenGLPFAStereo;

  /* Check for indexed color mode (rare, mostly for legacy compatibility) */
  /* NOTE: Modern macOS/NSOpenGL doesn't support indexed color mode.
   * We'll accept the attribute but force RGBA mode. */
  if (iupStrEqualNoCase(iupAttribGetStr(ih, "COLOR"), "INDEX"))
  {
    /* Indexed color not supported on modern macOS OpenGL.
     * Fall back to RGBA with warning in ERROR attribute. */
    iupAttribSet(ih, "ERROR", "WARNING: Indexed color mode not supported on macOS. Using RGBA instead.");
    /* BUFFER_SIZE attribute is ignored in RGBA mode */
  }

  /* RGBA color setup (default and only supported mode on macOS) */
  attrs[n++] = NSOpenGLPFAColorSize;
  number = 24;

  int red_size = iupAttribGetInt(ih, "RED_SIZE");
  int green_size = iupAttribGetInt(ih, "GREEN_SIZE");
  int blue_size = iupAttribGetInt(ih, "BLUE_SIZE");

  if (red_size > 0 || green_size > 0 || blue_size > 0)
  {
    number = 0;
    if (red_size > 0) number += red_size;
    else number += 8;
    if (green_size > 0) number += green_size;
    else number += 8;
    if (blue_size > 0) number += blue_size;
    else number += 8;
  }
  attrs[n++] = number;

  number = iupAttribGetInt(ih, "ALPHA_SIZE");
  if (number > 0)
  {
    attrs[n++] = NSOpenGLPFAAlphaSize;
    attrs[n++] = number;
  }

  number = iupAttribGetInt(ih, "DEPTH_SIZE");
  attrs[n++] = NSOpenGLPFADepthSize;
  attrs[n++] = (number > 0) ? number : 16;

  number = iupAttribGetInt(ih, "STENCIL_SIZE");
  if (number > 0)
  {
    attrs[n++] = NSOpenGLPFAStencilSize;
    attrs[n++] = number;
  }

  number = iupAttribGetInt(ih, "ACCUM_RED_SIZE");
  if (number > 0)
  {
    attrs[n++] = NSOpenGLPFAAccumSize;
    int accum = number;

    number = iupAttribGetInt(ih, "ACCUM_GREEN_SIZE");
    if (number > 0) accum += number;

    number = iupAttribGetInt(ih, "ACCUM_BLUE_SIZE");
    if (number > 0) accum += number;

    number = iupAttribGetInt(ih, "ACCUM_ALPHA_SIZE");
    if (number > 0) accum += number;

    attrs[n++] = accum;
  }

  number = iupAttribGetInt(ih, "SAMPLE_BUFFERS");
  if (number > 0)
  {
    attrs[n++] = NSOpenGLPFASampleBuffers;
    attrs[n++] = number;

    number = iupAttribGetInt(ih, "SAMPLES");
    if (number > 0)
    {
      attrs[n++] = NSOpenGLPFASamples;
      attrs[n++] = number;
    }
  }

  attrs[n] = 0;

  NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];

  if (!pixelFormat && iupAttribGetBoolean(ih, "STEREO"))
  {
    /* try removing stereo */
    int i;
    BOOL found = NO;
    for (i = 0; i < n; i++)
    {
      if (attrs[i] == NSOpenGLPFAStereo)
      {
        int j;
        for (j = i; j < n - 1; j++)
          attrs[j] = attrs[j + 1];
        n--;
        attrs[n] = 0;
        found = YES;
        break;
      }
    }

    if (found)
    {
        pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        if (pixelFormat)
          iupAttribSet(ih, "STEREO", "NO");
    }
  }

  if (!pixelFormat)
  {
    /* try software rendering as a fallback */
    pixelFormat = cocoaGLCreatePixelFormatSoftware(ih);
  }

  if (!pixelFormat && !iupAttribGet(ih, "ERROR"))
  {
    if (request_core)
      iupAttribSet(ih, "ERROR", "Failed to create OpenGL Core profile context.");
    else
      iupAttribSet(ih, "ERROR", "Failed to create OpenGL Legacy profile context.");
  }

  return pixelFormat;
}

static int cocoaGLCanvasMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  iupAttribSet(ih, "ERROR", NULL);

  /* Get the canvas view that was already created by Canvas map */
  gldata->canvas_view = (NSView*)iupAttribGet(ih, "NSVIEW");
  if (!gldata->canvas_view)
    gldata->canvas_view = (NSView*)IupGetAttribute(ih, "NSVIEW");

  if (!gldata->canvas_view)
  {
    iupAttribSet(ih, "ERROR", "Could not get native view handle.");
    [pool drain];
    return IUP_NOERROR;
  }

  /* For Retina/HiDPI displays: ensure GL surface matches backing store resolution */
  if ([gldata->canvas_view respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)])
  {
    [gldata->canvas_view setWantsBestResolutionOpenGLSurface:YES];
  }

  gldata->pixel_format = cocoaGLCreatePixelFormat(ih);

  if (!gldata->pixel_format)
  {
    if (!iupAttribGet(ih, "ERROR"))
    {
      iupAttribSet(ih, "ERROR", "No appropriate pixel format found.");
    }
    [pool drain];
    return IUP_NOERROR;
  }

  /* Create OpenGL context and attach to canvas view */
  gldata->context = [[IupGLContext alloc] initWithFormat:gldata->pixel_format shareContext:nil];
  if (!gldata->context)
  {
    iupAttribSet(ih, "ERROR", "Could not create OpenGL context.");
    [gldata->pixel_format release];
    gldata->pixel_format = nil;
    [pool drain];
    return IUP_NOERROR;
  }

  /* Make context current for GL queries, but no drawable attached yet */
  [gldata->context makeCurrentContext];

  /* Set vsync based on attribute, default to ON */
  GLint swapInt = 1;
  char* vsync = iupAttribGetStr(ih, "VSYNC");
  if (vsync)
    swapInt = iupStrBoolean(vsync) ? 1 : 0;

  [gldata->context setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

  /* TODO: Handle SHAREDCONTEXT attribute if needed in the future */

  iupAttribSet(ih, "CONTEXT", (char*)gldata->context);

  [pool drain];
  return IUP_NOERROR;
}

static void cocoaGLCanvasUnMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (!gldata)
    return;

  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  if (gldata->context)
  {
    if ([NSOpenGLContext currentContext] == gldata->context)
      [NSOpenGLContext clearCurrentContext];

    [gldata->context clearDrawable];
    [gldata->context release];
    gldata->context = nil;
  }

  if (gldata->pixel_format)
  {
    [gldata->pixel_format release];
    gldata->pixel_format = nil;
  }

  gldata->canvas_view = nil;  /* Don't release - owned by Canvas */

  [pool drain];

  memset(gldata, 0, sizeof(IGlControlData));
}

static void cocoaGLCanvasDestroy(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (gldata && gldata->canvas_view)
  {
      cocoaGLCanvasUnMapMethod(ih);
  }

  if (gldata)
  {
    free(gldata);
  }
  iupAttribSet(ih, "_IUP_GLCONTROLDATA", NULL);
}

void iupdrvGlCanvasInitClass(Iclass* ic)
{
  ic->Create = cocoaGLCanvasCreateMethod;
  ic->Destroy = cocoaGLCanvasDestroy;
  ic->Map = cocoaGLCanvasMapMethod;
  ic->UnMap = cocoaGLCanvasUnMapMethod;

  iupClassRegisterAttribute(ic, "VSYNC", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

int IupGLIsCurrent(Ihandle* ih)
{
  IGlControlData* gldata;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return 0;

  /* must be an IupGLCanvas */
  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return 0;

  /* must be mapped */
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->context)
    return 0;

  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  int result = ([NSOpenGLContext currentContext] == gldata->context) ? 1 : 0;
  [pool drain];

  return result;
}

void IupGLMakeCurrent(Ihandle* ih)
{
  IGlControlData* gldata;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas"))
    return;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->context || !gldata->canvas_view)
  {
    iupAttribSet(ih, "ERROR", "Context not available: canvas not mapped or context creation failed.");
    return;
  }

  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  [gldata->context makeCurrentContext];

  NSOpenGLContext* verifyCtx = [NSOpenGLContext currentContext];

  if ([NSOpenGLContext currentContext] == gldata->context)
  {
      iupAttribSet(ih, "ERROR", NULL);

      if (!IupGetGlobal("GL_VERSION"))
      {
        const char* vendor = (const char*)glGetString(GL_VENDOR);
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        const char* version = (const char*)glGetString(GL_VERSION);

        if (vendor && renderer && version) {
            IupSetStrGlobal("GL_VENDOR", vendor);
            IupSetStrGlobal("GL_RENDERER", renderer);
            IupSetStrGlobal("GL_VERSION", version);
        }
      }
  }
  else
  {
    iupAttribSet(ih, "ERROR", "Failed to make context current.");
  }

  [pool drain];
}

void IupGLSwapBuffers(Ihandle* ih)
{
  IGlControlData* gldata;
  Icallback cb;

  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas"))
    return;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->context || !gldata->canvas_view)
    return;

  cb = IupGetCallback(ih, "SWAPBUFFERS_CB");
  if (cb)
    cb(ih);

  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  [gldata->context flushBuffer];

  [pool drain];
}

void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
{
  /* Palette/indexed color mode is not supported on modern macOS/OpenGL */
  (void)ih;
  (void)index;
  (void)r;
  (void)g;
  (void)b;
}

void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
{
  /* Display list fonts are deprecated in modern OpenGL/macOS and not supported in Core Profile. */
  (void)ih;
  (void)first;
  (void)count;
  (void)list_base;
}

void IupGLWait(int gl)
{
  if (gl)
    glFinish(); /* Wait for GL commands to complete */
  else
  {
    /* No-op: Cocoa has no direct equivalent to GdiFlush() or glXWaitX() for non-GL window system synchronization. */
  }
}
