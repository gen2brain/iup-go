/** \file
 * \brief iupgl control for iOS / CocoaTouch (UIKit + EAGL).
 *
 * OpenGL ES is deprecated since iOS 12 but still functional; deprecation
 * warnings are silenced for the whole TU.
 *
 * See Copyright Notice in "iup.h"
 */

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#import <UIKit/UIKit.h>
#import <QuartzCore/CAEAGLLayer.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupgl.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_canvas.h"

#include "iupcocoatouch_drv.h"


@interface IupGLView : UIView
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) CGSize previousSize;
@end

@implementation IupGLView
+ (Class)layerClass { return [CAEAGLLayer class]; }

- (void)layoutSubviews
{
	[super layoutSubviews];

	CGSize size = self.bounds.size;
	if (CGSizeEqualToSize(size, _previousSize)) return;
	_previousSize = size;

	if (!_ihandle) return;
	IFnii resize_cb = (IFnii)IupGetCallback(_ihandle, "RESIZE_CB");
	if (resize_cb && !_ihandle->data->inside_resize)
	{
		/* RESIZE_CB carries backing-buffer pixels, not points. */
		CGFloat scale = self.contentScaleFactor;
		_ihandle->data->inside_resize = 1;
		resize_cb(_ihandle, (int)(size.width * scale), (int)(size.height * scale));
		_ihandle->data->inside_resize = 0;
	}

	/* CAEAGLLayer has no implicit paint trigger; dispatch ACTION on every layout. */
	Icallback action_cb = IupGetCallback(_ihandle, "ACTION");
	if (action_cb) action_cb(_ihandle);
}
@end


typedef struct _IGlControlData
{
	IupGLView* view;
	EAGLContext* context;
	GLuint framebuffer;
	GLuint color_renderbuffer;
	GLuint depth_renderbuffer;     /* depth or packed depth+stencil */
	int    has_stencil;
	int    backing_w, backing_h;
} IGlControlData;


static int cocoaTouchGLCanvasDefaultResize_CB(Ihandle* ih, int width, int height)
{
	(void)ih; (void)width; (void)height;
	/* The CAEAGLLayer drives the actual drawable size; framebuffer is rebuilt
	   lazily on the next IupGLMakeCurrent so the GL viewport tracks bounds. */
	return IUP_DEFAULT;
}

static int cocoaTouchGLCanvasCreateMethod(Ihandle* ih, void** params)
{
	(void)params;
	IGlControlData* gldata = (IGlControlData*)calloc(1, sizeof(IGlControlData));
	iupAttribSet(ih, "_IUP_GLCONTROLDATA", (char*)gldata);
	IupSetCallback(ih, "RESIZE_CB", (Icallback)cocoaTouchGLCanvasDefaultResize_CB);
	return IUP_NOERROR;
}

static EAGLRenderingAPI cocoaTouchGLChooseAPI(Ihandle* ih)
{
	char* version_attr = iupAttribGetStr(ih, "CONTEXTVERSION");
	if (version_attr)
	{
		int major = 0, minor = 0;
		iupStrToIntInt(version_attr, &major, &minor, '.');
		if (major <= 1) return kEAGLRenderingAPIOpenGLES1;
		if (major == 2) return kEAGLRenderingAPIOpenGLES2;
	}
	return kEAGLRenderingAPIOpenGLES3;
}

static void cocoaTouchGLConfigureLayer(IupGLView* view, Ihandle* ih)
{
	CAEAGLLayer* layer = (CAEAGLLayer*)view.layer;
	layer.opaque = YES;
	view.contentScaleFactor = [UIScreen mainScreen].scale;

	NSString* color_format = kEAGLColorFormatRGBA8;
	if (iupStrEqualNoCase(iupAttribGetStr(ih, "COLOR_FORMAT"), "RGB565"))
		color_format = kEAGLColorFormatRGB565;

	BOOL retained = iupAttribGetBoolean(ih, "RETAINEDBACKING") ? YES : NO;

	layer.drawableProperties = @{
		kEAGLDrawablePropertyColorFormat: color_format,
		kEAGLDrawablePropertyRetainedBacking: [NSNumber numberWithBool:retained]
	};
}

static void cocoaTouchGLDeleteFramebuffer(IGlControlData* gldata)
{
	if (gldata->framebuffer)        { glDeleteFramebuffers(1, &gldata->framebuffer);        gldata->framebuffer = 0; }
	if (gldata->color_renderbuffer) { glDeleteRenderbuffers(1, &gldata->color_renderbuffer); gldata->color_renderbuffer = 0; }
	if (gldata->depth_renderbuffer) { glDeleteRenderbuffers(1, &gldata->depth_renderbuffer); gldata->depth_renderbuffer = 0; }
	gldata->backing_w = gldata->backing_h = 0;
	gldata->has_stencil = 0;
}

/* Lazy framebuffer (re)creation tracking the layer's drawable size. */
static void cocoaTouchGLEnsureFramebuffer(Ihandle* ih, IGlControlData* gldata)
{
	if (!gldata->context || !gldata->view) return;

	CAEAGLLayer* layer = (CAEAGLLayer*)gldata->view.layer;
	CGFloat scale = gldata->view.contentScaleFactor;
	int w = (int)(layer.bounds.size.width  * scale);
	int h = (int)(layer.bounds.size.height * scale);
	if (w <= 0 || h <= 0) return;

	if (gldata->framebuffer && gldata->backing_w == w && gldata->backing_h == h)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gldata->framebuffer);
		return;
	}

	cocoaTouchGLDeleteFramebuffer(gldata);

	glGenFramebuffers(1, &gldata->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gldata->framebuffer);

	glGenRenderbuffers(1, &gldata->color_renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, gldata->color_renderbuffer);
	if (![gldata->context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer])
	{
		iupAttribSet(ih, "ERROR", "Failed to bind color renderbuffer to layer.");
		cocoaTouchGLDeleteFramebuffer(gldata);
		return;
	}
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, gldata->color_renderbuffer);

	GLint backing_w = w, backing_h = h;
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,  &backing_w);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backing_h);

	int depth_bits   = iupAttribGetInt(ih, "DEPTH_SIZE");
	int stencil_bits = iupAttribGetInt(ih, "STENCIL_SIZE");
	if (depth_bits == 0 && stencil_bits == 0) depth_bits = 16;

	if (depth_bits > 0 || stencil_bits > 0)
	{
		GLenum format;
		if (stencil_bits > 0)
		{
			format = GL_DEPTH24_STENCIL8;
			gldata->has_stencil = 1;
		}
		else
		{
			format = (depth_bits > 16) ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT16;
			gldata->has_stencil = 0;
		}

		glGenRenderbuffers(1, &gldata->depth_renderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, gldata->depth_renderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, format, backing_w, backing_h);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gldata->depth_renderbuffer);
		if (gldata->has_stencil)
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gldata->depth_renderbuffer);
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		iupAttribSet(ih, "ERROR", "OpenGL ES framebuffer is incomplete.");
		cocoaTouchGLDeleteFramebuffer(gldata);
		return;
	}

	gldata->backing_w = backing_w;
	gldata->backing_h = backing_h;
	glBindRenderbuffer(GL_RENDERBUFFER, gldata->color_renderbuffer);
}

static int cocoaTouchGLCanvasMapMethod(Ihandle* ih)
{
	IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
	if (!gldata) return IUP_ERROR;

	iupAttribSet(ih, "ERROR", NULL);

	EAGLRenderingAPI api = cocoaTouchGLChooseAPI(ih);
	gldata->context = [[EAGLContext alloc] initWithAPI:api];
	if (!gldata->context && api == kEAGLRenderingAPIOpenGLES3)
		gldata->context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	if (!gldata->context)
	{
		iupAttribSet(ih, "ERROR", "Could not create OpenGL ES context.");
		return IUP_NOERROR;
	}

	IupGLView* view = [[IupGLView alloc] initWithFrame:CGRectMake(0, 0, ih->currentwidth, ih->currentheight)];
	view.ihandle = ih;
	cocoaTouchGLConfigureLayer(view, ih);

	gldata->view = view;
	ih->handle = view;
	iupCocoaTouchAddToParent(ih);

	iupAttribSet(ih, "CONTEXT", (char*)gldata->context);
	return IUP_NOERROR;
}

static void cocoaTouchGLCanvasUnMapMethod(Ihandle* ih)
{
	IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
	if (!gldata) return;

	/* Bind our context so the renderbuffer deletes hit it. */
	if (gldata->context && [EAGLContext currentContext] != gldata->context)
		[EAGLContext setCurrentContext:gldata->context];

	cocoaTouchGLDeleteFramebuffer(gldata);

	if ([EAGLContext currentContext] == gldata->context)
		[EAGLContext setCurrentContext:nil];

	[gldata->context release];
	gldata->context = nil;

	if (gldata->view)
	{
		[gldata->view removeFromSuperview];
		[gldata->view release];
		gldata->view = nil;
	}

	ih->handle = NULL;
	iupAttribSet(ih, "CONTEXT", NULL);
}

static void cocoaTouchGLCanvasDestroy(Ihandle* ih)
{
	IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
	if (gldata && gldata->view) cocoaTouchGLCanvasUnMapMethod(ih);
	if (gldata) free(gldata);
	iupAttribSet(ih, "_IUP_GLCONTROLDATA", NULL);
}

void iupGlCanvasInitClass(Iclass* ic)
{
	ic->Create  = cocoaTouchGLCanvasCreateMethod;
	ic->Destroy = cocoaTouchGLCanvasDestroy;
	ic->Map     = cocoaTouchGLCanvasMapMethod;
	ic->UnMap   = cocoaTouchGLCanvasUnMapMethod;

	iupClassRegisterAttribute(ic, "RETAINEDBACKING", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "COLOR_FORMAT",    NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

IUPGL_API int IupGLIsCurrent(Ihandle* ih)
{
	iupASSERT(iupObjectCheck(ih));
	if (!iupObjectCheck(ih)) return 0;
	if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas")) return 0;

	IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
	if (!gldata || !gldata->context) return 0;
	return [EAGLContext currentContext] == gldata->context ? 1 : 0;
}

IUPGL_API void IupGLMakeCurrent(Ihandle* ih)
{
	iupASSERT(iupObjectCheck(ih));
	if (!iupObjectCheck(ih)) return;
	if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas")) return;

	IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
	if (!gldata || !gldata->context)
	{
		iupAttribSet(ih, "ERROR", "Context not available: canvas not mapped or context creation failed.");
		return;
	}

	if (![EAGLContext setCurrentContext:gldata->context])
	{
		iupAttribSet(ih, "ERROR", "Failed to make OpenGL ES context current.");
		return;
	}

	cocoaTouchGLEnsureFramebuffer(ih, gldata);
	if (gldata->framebuffer)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, gldata->framebuffer);
		glViewport(0, 0, gldata->backing_w, gldata->backing_h);
	}

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

IUPGL_API void IupGLSwapBuffers(Ihandle* ih)
{
	iupASSERT(iupObjectCheck(ih));
	if (!iupObjectCheck(ih)) return;
	if (ih->iclass->nativetype != IUP_TYPECANVAS || !IupClassMatch(ih, "glcanvas")) return;

	IGlControlData* gldata = (IGlControlData*)iupAttribGet(ih, "_IUP_GLCONTROLDATA");
	if (!gldata || !gldata->context || !gldata->color_renderbuffer) return;

	Icallback cb = IupGetCallback(ih, "SWAPBUFFERS_CB");
	if (cb) cb(ih);

	glBindRenderbuffer(GL_RENDERBUFFER, gldata->color_renderbuffer);
	[gldata->context presentRenderbuffer:GL_RENDERBUFFER];
}

/* Indexed color and display-list fonts do not exist in OpenGL ES. */
IUPGL_API void IupGLPalette(Ihandle* ih, int index, float r, float g, float b)
{ (void)ih; (void)index; (void)r; (void)g; (void)b; }

IUPGL_API void IupGLUseFont(Ihandle* ih, int first, int count, int list_base)
{ (void)ih; (void)first; (void)count; (void)list_base; }

IUPGL_API void IupGLWait(int gl)
{
	if (gl) glFinish();
}
