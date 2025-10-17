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
    [[self openGLContext] makeCurrentContext];

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

  /* Enable vsync by default */
  GLint swapInt = 1;
  [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

  self.contextReady = YES;

  /* Trigger initial resize callback after context is ready. */
  [self triggerResizeCB];
}

- (void)reshape
{
  [super reshape];

  /* We rely on contextReady to ensure initialization happened, but allow calls even if not ready yet during startup phase */
  if (self.ih) {
    [[self openGLContext] makeCurrentContext];
    [[self openGLContext] update];

    /* Notify IUP about the resize */
    /* We only trigger the callback if contextReady is true to maintain consistency with triggerResizeCB logic */
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

  /* Trigger the IUP ACTION callback for rendering.
     IupGLCanvas inherits from IupCanvas, ACTION signature is IFnff (Ihandle*, float, float). */
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

/* lockFocus/unlockFocus overrides ensure the context is current if Cocoa tries to draw the view outside drawRect (e.g. manual lockFocus). */
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

  /* When using software rendering (forced or as fallback), adding NSOpenGLPFANoRecovery is crucial.
     It improves stability by preventing the OS from automatically switching renderers if the initial renderer encounters issues. */
  attrs[n++] = NSOpenGLPFANoRecovery;

  /* Renderer ID if specified */
  if (rendererID != 0)
  {
    attrs[n++] = NSOpenGLPFARendererID;
    attrs[n++] = rendererID;
  }

  /* Allow offline renderers is generally good practice for compatibility. */
  attrs[n++] = NSOpenGLPFAAllowOfflineRenderers;

  attrs[n++] = NSOpenGLPFAOpenGLProfile;
  if (request_core)
  {
    /* Request Core profile. */
    if (@available(macOS 10.10, *)) {
      attrs[n++] = NSOpenGLProfileVersion4_1Core;
    } else if (@available(macOS 10.7, *)) {
      attrs[n++] = NSOpenGLProfileVersion3_2Core;
    } else {
        /* Core requested but macOS < 10.7. */
#ifdef NSOpenGLProfileVersion3_2Core
        /* Try 3.2 Core if SDK supports it. */
        attrs[n++] = NSOpenGLProfileVersion3_2Core;
#else
        /* SDK too old for Core, must use Legacy. */
        attrs[n++] = NSOpenGLProfileVersionLegacy;
#endif
    }
  }
  else
  {
    /* Request Legacy profile. */
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
  if (stencil_size > 0) {
    attrs[n++] = NSOpenGLPFAStencilSize;
    attrs[n++] = stencil_size;
  }

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "BUFFER"), "DOUBLE")) {
    attrs[n++] = NSOpenGLPFADoubleBuffer;
  }

  /* Terminator */
  attrs[n] = 0;
  *n_ptr = n;
}

/* Implementation of software pixel format creation */
static NSOpenGLPixelFormat* cocoaGLCreatePixelFormatSoftware(Ihandle* ih)
{
  NSOpenGLPixelFormatAttribute attrs[32];
  int n = 0;
  NSOpenGLPixelFormat* pixelFormat = NULL;

  char* profile_attr = iupAttribGetStr(ih, "CONTEXTPROFILE");
  BOOL initial_core_requested = (profile_attr && iupStrEqualNoCase(profile_attr, "CORE"));

  /* Default to Legacy (try_core = NO) unless Core is explicitly requested. */
  BOOL try_core = initial_core_requested;

  /* kCGLRendererGenericFloatID is the recommended modern software renderer. */
  buildSoftwareAttributes(ih, attrs, &n, try_core, (NSOpenGLPixelFormatAttribute)kCGLRendererGenericFloatID);
  pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
  if (pixelFormat) return pixelFormat;

  /* If Core was tried and failed, try Legacy with Generic Float. */
  if (try_core && !pixelFormat)
  {
    if (!iupAttribGet(ih, "WARNING")) {
        iupAttribSet(ih, "WARNING", "Software Core profile (GenericFloat) failed, attempting Legacy profile (GenericFloat).");
    }
    buildSoftwareAttributes(ih, attrs, &n, NO /* Legacy */, (NSOpenGLPixelFormatAttribute)kCGLRendererGenericFloatID);
    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (pixelFormat) return pixelFormat;
  }

  /* Check if kCGLRendererAppleSWID is available (deprecated in newer SDKs) */
#ifdef kCGLRendererAppleSWID
  if (!pixelFormat)
  {
      if (!iupAttribGet(ih, "WARNING")) {
          iupAttribSet(ih, "WARNING", "Modern software renderer (GenericFloat) failed, attempting older software renderer (AppleSW).");
      }
      buildSoftwareAttributes(ih, attrs, &n, NO /* Legacy */, (NSOpenGLPixelFormatAttribute)kCGLRendererAppleSWID);
      pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
  }
#endif

  if (pixelFormat) return pixelFormat;

  /* No specific RendererID (system choice), Legacy profile. */
  /* This allows the system to choose any available unaccelerated renderer if specific IDs failed. */
  if (!pixelFormat)
  {
      if (!iupAttribGet(ih, "WARNING")) {
          iupAttribSet(ih, "WARNING", "Specific software renderers failed, attempting any available unaccelerated renderer (Legacy).");
      }
      /* Pass 0 for RendererID. buildSoftwareAttributes handles adding NSOpenGLPFANoRecovery. */
      buildSoftwareAttributes(ih, attrs, &n, NO /* Legacy */, 0);
      pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
  }

  if (initial_core_requested && !pixelFormat)
  {
      if (!iupAttribGet(ih, "ERROR")) {
        iupAttribSet(ih, "ERROR", "Failed to create requested software Core profile context, and all fallbacks failed.");
      }
  }

  return pixelFormat;
}


static NSOpenGLPixelFormat* cocoaGLCreatePixelFormat(Ihandle* ih)
{
  NSOpenGLPixelFormatAttribute attrs[50];
  int n = 0;
  int number;
  BOOL force_software = NO;

  char* profile_attr = iupAttribGetStr(ih, "CONTEXTPROFILE");

  /* Check for environment variables to force specific rendering modes */
  const char* env_force_software = getenv("IUP_GL_FORCE_SOFTWARE");
  if (env_force_software && (env_force_software[0] == '1' || iupStrEqualNoCase(env_force_software, "YES") || iupStrEqualNoCase(env_force_software, "TRUE"))) {
    force_software = YES;
  }

  /* If software rendering is forced, use software path */
  if (force_software) {
    return cocoaGLCreatePixelFormatSoftware(ih);
  }

  /* Hardware accelerated path (default) */

  attrs[n++] = NSOpenGLPFAOpenGLProfile;

  /* Check for context profile requests. macOS supports Core (3.2+) and Legacy (2.1). */
  /* We use profile_attr determined earlier. */

  if (profile_attr && iupStrEqualNoCase(profile_attr, "CORE"))
  {
    /* Request Core profile. Prefer 4.1 if available, otherwise 3.2. */
    if (@available(macOS 10.10, *))
      attrs[n++] = NSOpenGLProfileVersion4_1Core;
    else if (@available(macOS 10.7, *))
      attrs[n++] = NSOpenGLProfileVersion3_2Core;
    else
    {
        /* Core profile requested but not supported on macOS < 10.7.
           We set 3.2 Core anyway (if defined in SDK), and the creation will fail later if unsupported. */
#ifdef NSOpenGLProfileVersion3_2Core
        attrs[n++] = NSOpenGLProfileVersion3_2Core;
#else
        /* If SDK is too old, we must use Legacy. If Core was explicitly requested, this will fail later. */
        attrs[n++] = NSOpenGLProfileVersionLegacy;
#endif
    }
  }
  else if (profile_attr && (iupStrEqualNoCase(profile_attr, "LEGACY") || iupStrEqualNoCase(profile_attr, "COMPATIBILITY")))
  {
    attrs[n++] = NSOpenGLProfileVersionLegacy;
  }
  else
  {
    /* Default behavior: Use Legacy for better compatibility */
    attrs[n++] = NSOpenGLProfileVersionLegacy;
  }

  /* Request hardware acceleration */
  attrs[n++] = NSOpenGLPFAAccelerated;

  /* Allow offline renderers (e.g. switchable graphics) */
  attrs[n++] = NSOpenGLPFAAllowOfflineRenderers;

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "BUFFER"), "DOUBLE"))
    attrs[n++] = NSOpenGLPFADoubleBuffer;

  if (iupAttribGetBoolean(ih, "STEREO"))
    attrs[n++] = NSOpenGLPFAStereo;

  attrs[n++] = NSOpenGLPFAColorSize;
  number = 24; /* default RGB888 */

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
  if (number > 0)
  {
    attrs[n++] = NSOpenGLPFADepthSize;
    attrs[n++] = number;
  }
  else
  {
    attrs[n++] = NSOpenGLPFADepthSize;
    attrs[n++] = 16; /* A reasonable default */
  }

  number = iupAttribGetInt(ih, "STENCIL_SIZE");
  if (number > 0)
  {
    attrs[n++] = NSOpenGLPFAStencilSize;
    attrs[n++] = number;
  }

  /* accumulation buffer (deprecated in Core profile, but allowed in Legacy) */
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

  /* Terminator */
  attrs[n] = 0;

  NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];

  if (!pixelFormat && iupAttribGetBoolean(ih, "STEREO"))
  {
    /* Try again without stereo */
    int i;
    BOOL found = NO;
    for (i = 0; i < n; i++)
    {
      if (attrs[i] == NSOpenGLPFAStereo)
      {
        /* Remove stereo attribute by shifting array */
        int j;
        for (j = i; j < n - 1; j++)
          attrs[j] = attrs[j + 1];
        n--;
        attrs[n] = 0; /* Update terminator */
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
    /* Remove NSOpenGLPFAAccelerated and try again */
    int i;
    BOOL found = NO;
    for (i = 0; i < n; i++)
    {
      if (attrs[i] == NSOpenGLPFAAccelerated)
      {
        /* Remove accelerated attribute by shifting array */
        int j;
        for (j = i; j < n - 1; j++)
          attrs[j] = attrs[j + 1];
        n--;
        attrs[n] = 0; /* Update terminator */
        found = YES;
        break;
      }
    }

    if (found)
    {
        pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        if (pixelFormat)
          iupAttribSet(ih, "WARNING", "Hardware acceleration not available, using unaccelerated context");
    }
  }

  if (!pixelFormat)
  {
    if (!iupAttribGet(ih, "WARNING")) {
        iupAttribSet(ih, "WARNING", "Hardware context creation failed, attempting software rendering");
    }

    pixelFormat = cocoaGLCreatePixelFormatSoftware(ih);
  }

  return pixelFormat;
}

static int cocoaGLCanvasMapMethod(Ihandle* ih)
{
  IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  /* MRC requires Autorelease Pool for Cocoa operations */
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  iupAttribSet(ih, "ERROR", NULL);
  iupAttribSet(ih, "WARNING", NULL);

  /* the IupCanvas is already mapped, get the native view */
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
    if (!iupAttribGet(ih, "ERROR")) {
        iupAttribSet(ih, "ERROR", "No appropriate pixel format found (check WARNING attribute for details).");
    }
    [pool drain];
    return IUP_NOERROR;
  }

  NSOpenGLContext* sharedContext = nil;
  Ihandle* ih_shared = IupGetAttributeHandle(ih, "SHAREDCONTEXT");
  if (ih_shared && IupClassMatch(ih_shared, "glcanvas"))
  {
    IGlControlData* shared_gldata = (IGlControlData*)iupAttribGet(ih_shared, "_IUP_GLCONTROLDATA");
    if (shared_gldata && shared_gldata->context)
      sharedContext = shared_gldata->context;
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

  /* Get context from the view. NSOpenGLView creates it automatically. */
  gldata->context = [gldata->gl_view openGLContext]; /* Weak reference */

  /* Handle shared context. If specified, we must replace the context created by NSOpenGLView. */
  if (sharedContext && gldata->context && sharedContext != gldata->context)
  {
    NSOpenGLContext* newContext = [[NSOpenGLContext alloc]
                                    initWithFormat:gldata->pixel_format
                                    shareContext:sharedContext];

    if (newContext)
    {
      if ([NSOpenGLContext currentContext] == gldata->context) {
          [NSOpenGLContext clearCurrentContext];
      }

      [gldata->gl_view setOpenGLContext:newContext];
      [newContext setView:gldata->gl_view];

      /* Update our weak reference */
      gldata->context = newContext;

      /* Release ownership from this scope (balancing alloc/init) - Crucial for MRC */
      [newContext release];
    }
    else
    {
        iupAttribSet(ih, "WARNING", "Could not create shared OpenGL context. Continuing with non-shared context.");
    }
  }

  if (!gldata->context)
  {
    iupAttribSet(ih, "ERROR", "Could not get or create OpenGL context.");
    [gldata->gl_view release];
    gldata->gl_view = nil;
    [gldata->pixel_format release];
    gldata->pixel_format = nil;
    [pool drain];
    return IUP_NOERROR;
  }

  [gldata->context makeCurrentContext];

  if ([NSOpenGLContext currentContext] != gldata->context)
  {
    iupAttribSet(ih, "ERROR", "Could not make newly created OpenGL context current.");
    /* Cleanup context (it might be partially initialized) */
    gldata->context = nil; /* Weak reference cleared */
    [gldata->gl_view clearGLContext];

    [gldata->gl_view release];
    gldata->gl_view = nil;
    [gldata->pixel_format release];
    gldata->pixel_format = nil;
    [pool drain];
    return IUP_NOERROR;
  }


  const GLubyte* renderer = glGetString(GL_RENDERER);
  const GLubyte* version = glGetString(GL_VERSION);

  if (renderer && version) {
    /* Additional info if using software rendering */
    if (strstr((const char*)renderer, "Software") ||
        strstr((const char*)renderer, "Generic") ||
        strstr((const char*)renderer, "llvmpipe") ||
        strstr((const char*)renderer, "Apple Software")) {
      if (!iupAttribGet(ih, "WARNING")) {
        iupAttribSet(ih, "WARNING", "Using software renderer - performance may be limited");
      }
    }
  } else {
      iupAttribSet(ih, "ERROR", "OpenGL context created but unstable (failed to retrieve renderer info).");

      [NSOpenGLContext clearCurrentContext];
      gldata->context = nil; /* Weak reference cleared */
      [gldata->gl_view clearGLContext];

      [gldata->gl_view release];
      gldata->gl_view = nil;
      [gldata->pixel_format release];
      gldata->pixel_format = nil;
      [pool drain];
      return IUP_NOERROR;
  }

  /* Add as subview - this will trigger prepareOpenGL when added to window hierarchy */
  [gldata->parent_view addSubview:gldata->gl_view];
  /* parent_view retains gl_view. */

  /* Store context pointer for IUP (as opaque pointer) */
  iupAttribSet(ih, "CONTEXT", (char*)gldata->context);

  char* vsync = iupAttribGetStr(ih, "VSYNC");
  if (vsync)
  {
    GLint swapInt = iupStrBoolean(vsync) ? 1 : 0;
    [gldata->context setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
  }

  iupAttribSet(ih, "ERROR", NULL);

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

    gldata->context = nil; /* Clear weak reference */
  }

  if (gldata->gl_view)
  {
    /* Mark context as not ready to prevent callbacks during destruction */
    gldata->gl_view.contextReady = NO;

    [gldata->gl_view removeFromSuperview];
    [gldata->gl_view clearGLContext]; /* Explicitly clear context association */
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

  /* Register WARNING attribute to report non-fatal issues */
  iupClassRegisterAttribute(ic, "WARNING", NULL, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  /* VSYNC attribute */
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

  /* must be an IupGLCanvas */
  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return;

  /* must be mapped */
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->context || !gldata->gl_view)
    return;

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
    iupAttribSet(ih, "ERROR", "Failed to make OpenGL context current.");
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

  /* must be an IupGLCanvas */
  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "glcanvas"))
    return;

  /* must be mapped */
  gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
  if (!gldata || !gldata->context)
    return;

  cb = IupGetCallback(ih, "SWAPBUFFERS_CB");
  if (cb)
    cb(ih);

  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  /* flushBuffer swaps buffers if double buffered, or flushes if single buffered */
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
    glFlush(); /* Flush GL commands */
}
