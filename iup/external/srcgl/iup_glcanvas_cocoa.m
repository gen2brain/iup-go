/** \file
 * \brief iupgl control for macOS/Cocoa
 *
 * See Copyright Notice in "iup.h"
 */

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


@class IupGLView;

typedef struct _IGlControlData
{
  NSView* parent_view;
  IupGLView* gl_view;
  NSOpenGLContext* context; /* Weak reference, owned by gl_view */
  NSOpenGLPixelFormat* pixel_format;
} IGlControlData;

@interface IupGLView : NSOpenGLView
{
  Ihandle* ih;
  BOOL contextReady;
}
@property (nonatomic, assign) Ihandle* ih;
@property (nonatomic, assign) BOOL contextReady;
@end

@implementation IupGLView
@synthesize ih;
@synthesize contextReady;

- (id)initWithFrame:(NSRect)frameRect pixelFormat:(NSOpenGLPixelFormat*)format
{
  self = [super initWithFrame:frameRect pixelFormat:format];
  if (self) {
    self.contextReady = NO;
    self.ih = NULL;

    /* Opt-in for high resolution backing store (Retina) (macOS 10.7+) */
    if ([self respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)]) {
      [self setWantsBestResolutionOpenGLSurface:YES];
    }
  }
  return self;
}

- (void)dealloc
{
    self.ih = NULL;
    [super dealloc];
}

- (void)triggerResizeCB
{
  if (self.contextReady && self.ih) {
    NSRect bounds = [self bounds];
    /* Use backing scale factor for high-DPI support. RESIZE_CB expects pixels. */
    NSRect backingBounds = [self convertRectToBacking:bounds];
    int width = (int)backingBounds.size.width;
    int height = (int)backingBounds.size.height;

    IFnii cb = (IFnii)IupGetCallback(self.ih, "RESIZE_CB");
    if (cb) {
      cb(self.ih, width, height);
    }
  }
}

- (void)prepareOpenGL
{
  [super prepareOpenGL];

  [[self openGLContext] makeCurrentContext];

  /* Set vsync based on attribute, default to ON */
  GLint swapInt = 1;
  if (self.ih)
  {
    char* vsync = iupAttribGetStr(self.ih, "VSYNC");
    if (vsync)
    {
      swapInt = iupStrBoolean(vsync) ? 1 : 0;
    }
  }
  [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

  self.contextReady = YES;

  /* Trigger initial resize callback after context is ready. */
  [self triggerResizeCB];
}

- (void)reshape
{
  [super reshape];

  if (self.ih) {
    [[self openGLContext] makeCurrentContext];
    [[self openGLContext] update]; /* Important for view resizes */

    /* Notify IUP about the resize. triggerResizeCB will check if context is ready. */
    [self triggerResizeCB];
  }
}

- (void)drawRect:(NSRect)dirtyRect
{
  (void)dirtyRect;

  if (!self.contextReady || !self.ih) {
    return;
  }

  [[self openGLContext] makeCurrentContext];

  /* Trigger the IUP ACTION callback for rendering. */
  IFnff cb = (IFnff)IupGetCallback(self.ih, "ACTION");
  if (cb)
  {
    /* For GLCanvas, posx and posy (related to scrolling) are typically 0. */
    if (cb(self.ih, 0.0f, 0.0f) == IUP_CLOSE)
    {
        IupExitLoop();
    }
  }
  /* Application is responsible for calling IupGLSwapBuffers() if double buffered. */
}

- (BOOL)isOpaque
{
  return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    if ([super becomeFirstResponder]) {
        if (self.ih) {
            IFni cb = (IFni)IupGetCallback(self.ih, "FOCUS_CB");
            if (cb) cb(self.ih, 1);
        }
        return YES;
    }
    return NO;
}

- (BOOL)resignFirstResponder
{
    if ([super resignFirstResponder]) {
        if (self.ih) {
            IFni cb = (IFni)IupGetCallback(self.ih, "FOCUS_CB");
            if (cb) cb(self.ih, 0);
        }
        return YES;
    }
    return NO;
}

- (void)lockFocus
{
  NSOpenGLContext* context = [self openGLContext];
  [super lockFocus];
  if (context)
  {
    if ([context view] != self) {
      [context setView:self];
    }
    [context makeCurrentContext];
  }
}

- (void)unlockFocus
{
  [super unlockFocus];
}

@end

static int cocoaGLCanvasDefaultResize_CB(Ihandle *ih, int width, int height)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (gldata && gldata->context && gldata->gl_view && gldata->gl_view.contextReady) {
    IupGLMakeCurrent(ih);
    glViewport(0, 0, width, height);
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

  gldata->parent_view = (NSView*)iupAttribGet(ih, "NSVIEW");
  if (!gldata->parent_view)
    gldata->parent_view = (NSView*)IupGetAttribute(ih, "NSVIEW");

  if (!gldata->parent_view)
  {
    iupAttribSet(ih, "ERROR", "Could not get native view handle.");
    [pool drain];
    return IUP_NOERROR;
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

  NSRect frame = [gldata->parent_view bounds];
  gldata->gl_view = [[IupGLView alloc] initWithFrame:frame pixelFormat:gldata->pixel_format];

  if (!gldata->gl_view)
  {
    iupAttribSet(ih, "ERROR", "Could not create OpenGL view.");
    [gldata->pixel_format release];
    gldata->pixel_format = nil;
    [pool drain];
    return IUP_NOERROR;
  }

  [gldata->gl_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  gldata->gl_view.ih = ih;

  NSOpenGLContext* sharedContext = nil;
  Ihandle* ih_shared = IupGetAttributeHandle(ih, "SHAREDCONTEXT");
  if (ih_shared && IupClassMatch(ih_shared, "glcanvas"))
  {
    IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
    if (shared_gldata && shared_gldata->context)
      sharedContext = shared_gldata->context;
  }

  if (sharedContext)
  {
    NSOpenGLContext* newContext = [[NSOpenGLContext alloc]
                                    initWithFormat:gldata->pixel_format
                                    shareContext:sharedContext];

    if (newContext)
    {
      [gldata->gl_view setOpenGLContext:newContext];
      [newContext setView:gldata->gl_view];
      gldata->context = newContext;
      [newContext release];
    }
    else
    {
      iupAttribSet(ih, "ERROR", "Failed to create shared OpenGL context.");
      [gldata->gl_view release];
      gldata->gl_view = nil;
      [gldata->pixel_format release];
      gldata->pixel_format = nil;
      [pool drain];
      return IUP_NOERROR;
    }
  }
  else
  {
    gldata->context = [gldata->gl_view openGLContext];
  }

  if (!gldata->context)
  {
    iupAttribSet(ih, "ERROR", "Could not create or retrieve OpenGL context.");
    [gldata->gl_view release];
    gldata->gl_view = nil;
    [gldata->pixel_format release];
    gldata->pixel_format = nil;
    [pool drain];
    return IUP_NOERROR;
  }

  [gldata->parent_view addSubview:gldata->gl_view];

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

    gldata->context = nil;
  }

  if (gldata->gl_view)
  {
    gldata->gl_view.contextReady = NO;
    gldata->gl_view.ih = NULL;
    [gldata->gl_view removeFromSuperview];
    [gldata->gl_view clearGLContext];
    [gldata->gl_view release];
    gldata->gl_view = nil;
  }

  if (gldata->pixel_format)
  {
    [gldata->pixel_format release];
    gldata->pixel_format = nil;
  }

  [pool drain];

  memset(gldata, 0, sizeof(IGlControlData));
}

static void cocoaGLCanvasDestroy(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");

  if (gldata && gldata->gl_view)
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

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->context || !gldata->gl_view)
  {
    iupAttribSet(ih, "ERROR", "Context not available: canvas not mapped or context creation failed.");
    return;
  }

  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  if ([gldata->context view] != gldata->gl_view) {
    [gldata->context setView:gldata->gl_view];
  }

  [gldata->context makeCurrentContext];

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

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return;

  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->context || !gldata->gl_view)
    return;

  cb = IupGetCallback(ih, "SWAPBUFFERS_CB");
  if (cb)
    cb(ih);

  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  if ([gldata->context view] != gldata->gl_view) {
    [gldata->context setView:gldata->gl_view];
  }

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
