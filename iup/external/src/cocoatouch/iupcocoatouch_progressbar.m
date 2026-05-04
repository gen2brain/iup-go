/** \file
 * \brief Progress bar Control (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_progressbar.h"

#include "iupcocoatouch_drv.h"


@interface IupCocoaTouchProgressMarqueeDelegate : NSObject <CALayerDelegate>
@property(unsafe_unretained) UIProgressView* progressView;
@property(readonly) CGFloat slugWidth;
@property(readonly) CGFloat animationDistance;
@end

@implementation IupCocoaTouchProgressMarqueeDelegate
- (CGFloat)slugWidth
{
	return self.progressView.bounds.size.width / 10;
}

- (CGFloat)animationDistance
{
	return 2 * self.slugWidth;
}

- (void)drawLayer:(CALayer*)layer inContext:(CGContextRef)context
{
	(void)layer;
	CGColorRef color = [self.progressView.progressTintColor CGColor];
	if (!color)
	{
		color = [self.progressView.tintColor CGColor];
	}
	CGContextSetFillColorWithColor(context, color);

	CGFloat x = 0.0;
	CGFloat overall_width = self.progressView.bounds.size.width;
	while (x < overall_width + self.animationDistance)
	{
		CGFloat slug_w = self.slugWidth;
		CGContextFillRect(context, CGRectMake(x, 0.0, slug_w, self.progressView.bounds.size.height));
		x += 2 * slug_w;
	}
}
@end


@interface IupCocoaTouchProgressView : UIProgressView
@property(nonatomic, assign) BOOL indeterminate;
@property(nonatomic, unsafe_unretained) CALayer* marqueeLayer;
@property(nonatomic, unsafe_unretained) NSTimer* animationTimer;
@property(nonatomic, assign) CGFloat baseClassZPositionMin;
@property(nonatomic, assign) CGFloat baseClassZPositionMax;
@property(nonatomic, strong) IupCocoaTouchProgressMarqueeDelegate* marqueeDelegate;
@end

@implementation IupCocoaTouchProgressView

@synthesize indeterminate = _indeterminate;

- (void)dealloc
{
	[_marqueeDelegate release];
	[super dealloc];
}

- (void)doInitialSetup
{
	self.clipsToBounds = YES;
	self.progress = 0.0;

	/* sniff the base class's track/fill zPositions so the marquee layers behind them */
	self.baseClassZPositionMin = 0.0;
	self.baseClassZPositionMax = 0.0;
	for (CALayer* sublayer in self.layer.sublayers)
	{
		if (sublayer.zPosition < self.baseClassZPositionMin) self.baseClassZPositionMin = sublayer.zPosition;
		if (sublayer.zPosition > self.baseClassZPositionMax) self.baseClassZPositionMax = sublayer.zPosition;
	}

	self.marqueeDelegate = [[[IupCocoaTouchProgressMarqueeDelegate alloc] init] autorelease];
	self.marqueeDelegate.progressView = self;

	CALayer* marquee = [[[CALayer alloc] init] autorelease];
	self.marqueeLayer = marquee;
	marquee.zPosition = self.baseClassZPositionMin - 1;
	marquee.hidden = YES;
	[self.layer addSublayer:marquee];
	[self reframeMarqueeLayer];
	marquee.delegate = self.marqueeDelegate;
	[marquee setNeedsDisplay];
}

- (void)reframeMarqueeLayer
{
	CGRect frame = self.bounds;
	CGFloat dist = self.marqueeDelegate.animationDistance;
	frame.origin.x -= dist;
	frame.size.width += dist;
	self.marqueeLayer.frame = frame;
}

- (instancetype)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self)
	{
		[self doInitialSetup];
	}
	return self;
}

- (instancetype)initWithCoder:(NSCoder*)coder
{
	self = [super initWithCoder:coder];
	if (self)
	{
		[self doInitialSetup];
	}
	return self;
}

- (void)setBounds:(CGRect)bounds
{
	[super setBounds:bounds];
	[self reframeMarqueeLayer];
}

- (void)doOneAnimationCycleAtMediaTime:(NSTimeInterval)media_time duration:(NSTimeInterval)duration
{
	CGFloat x = self.marqueeLayer.position.x;
	CABasicAnimation* animation = [CABasicAnimation animationWithKeyPath:@"position.x"];
	animation.beginTime = media_time;
	animation.fromValue = [NSNumber numberWithFloat:x];
	animation.toValue   = [NSNumber numberWithFloat:x + self.marqueeDelegate.animationDistance];
	animation.duration  = duration;
	[self.marqueeLayer addAnimation:animation forKey:nil];
}

- (void)startAnimation
{
	NSTimeInterval duration = 0.5;
	self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:duration
		repeats:YES
		block:^(NSTimer* timer)
		{
			(void)timer;
			[self doOneAnimationCycleAtMediaTime:CACurrentMediaTime() duration:duration];
		}];
}

- (void)stopAnimation
{
	[self.animationTimer invalidate];
	self.animationTimer = nil;
	[self.marqueeLayer removeAllAnimations];
}

- (void)setIndeterminate:(BOOL)indeterminate
{
	_indeterminate = indeterminate;
	if (indeterminate)
	{
		self.progress = 0.0;
		[self startAnimation];
		self.marqueeLayer.zPosition = self.baseClassZPositionMax + 1.0;
		self.marqueeLayer.hidden = NO;
	}
	else
	{
		[self stopAnimation];
		self.marqueeLayer.zPosition = self.baseClassZPositionMin - 1.0;
		self.marqueeLayer.hidden = YES;
	}
	[self setNeedsDisplay];
}

@end

static void cocoaTouchProgressBarApplyDashed(Ihandle* ih);

@interface IupCocoaTouchProgressContainer : UIView
@property(nonatomic, retain) IupCocoaTouchProgressView* bar;
@property(nonatomic, retain) UIActivityIndicatorView* spinner;
@property(nonatomic, assign) BOOL isVertical;
@end

@implementation IupCocoaTouchProgressContainer
- (void)dealloc { [_bar release]; [_spinner release]; [super dealloc]; }
- (void)layoutSubviews
{
	[super layoutSubviews];
	CGFloat w = self.bounds.size.width;
	CGFloat h = self.bounds.size.height;
	if (self.spinner)
	{
		self.spinner.center = CGPointMake(w * 0.5, h * 0.5);
		return;
	}
	/* keep native ~4pt thickness so vertical and horizontal look identical */
	const CGFloat natural_thickness = 4.0;
	if (self.isVertical)
	{
		self.bar.transform = CGAffineTransformIdentity;
		self.bar.bounds = CGRectMake(0, 0, h, natural_thickness);
		self.bar.center = CGPointMake(w * 0.5, h * 0.5);
		self.bar.transform = CGAffineTransformMakeRotation(-M_PI / 2.0);
	}
	else
	{
		self.bar.transform = CGAffineTransformIdentity;
		self.bar.bounds = CGRectMake(0, 0, w, natural_thickness);
		self.bar.center = CGPointMake(w * 0.5, h * 0.5);
	}
}
@end

static IupCocoaTouchProgressView* cocoaTouchProgressBarGetBar(Ihandle* ih)
{
	id handle = ih ? (id)ih->handle : nil;
	return [handle isKindOfClass:[IupCocoaTouchProgressContainer class]]
		? ((IupCocoaTouchProgressContainer*)handle).bar
		: (IupCocoaTouchProgressView*)handle;
}

static UIActivityIndicatorView* cocoaTouchProgressBarGetSpinner(Ihandle* ih)
{
	id handle = ih ? (id)ih->handle : nil;
	return [handle isKindOfClass:[IupCocoaTouchProgressContainer class]]
		? ((IupCocoaTouchProgressContainer*)handle).spinner
		: nil;
}

static int cocoaTouchProgressBarMapMethod(Ihandle* ih)
{
	IupCocoaTouchProgressContainer* container = [[IupCocoaTouchProgressContainer alloc] initWithFrame:CGRectMake(0, 0, 200, 40)];
	container.isVertical = iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL");

	if (iupAttribGetBoolean(ih, "CIRCULAR"))
	{
		UIActivityIndicatorView* spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleLarge];
		spinner.hidesWhenStopped = NO;
		[spinner startAnimating];
		[container addSubview:spinner];
		container.spinner = spinner;
		[spinner release];
		ih->data->marquee = 1;
	}
	else
	{
		IupCocoaTouchProgressView* bar = [[IupCocoaTouchProgressView alloc] initWithFrame:CGRectMake(0, 0, 200, 40)];
		bar.autoresizingMask = UIViewAutoresizingNone;
		[container addSubview:bar];
		container.bar = bar;
		[bar release];

		if (iupAttribGetBoolean(ih, "MARQUEE"))
		{
			ih->data->marquee = 1;
			[bar setIndeterminate:YES];
		}
		else
		{
			ih->data->marquee = 0;
			[bar setIndeterminate:NO];
		}
	}

	ih->handle = container;
	iupCocoaTouchAddToParent(ih);

	if (!iupAttribGetBoolean(ih, "CIRCULAR") && iupAttribGetBoolean(ih, "DASHED"))
		cocoaTouchProgressBarApplyDashed(ih);

	return IUP_NOERROR;
}

static char* cocoaTouchProgressBarGetValueAttrib(Ihandle* ih)
{
	IupCocoaTouchProgressView* bar = cocoaTouchProgressBarGetBar(ih);
	return iupStrReturnDouble([bar progress]);
}

static int cocoaTouchProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchProgressView* bar = cocoaTouchProgressBarGetBar(ih);
	if (!bar) return 0;
	if (ih->data->marquee)
	{
		return 0;
	}
	if (!value)
	{
		ih->data->value = 0;
	}
	else
	{
		iupStrToDouble(value, &(ih->data->value));
	}
	iProgressBarCropValue(ih);
	float range = (float)(ih->data->vmax - ih->data->vmin);
	if (range > 0.0f)
	{
		[bar setProgress:(float)((ih->data->value - ih->data->vmin) / range)];
	}
	return 0;
}

/* tileable stripe used as progressImage for DASHED=YES */
static UIImage* cocoaTouchProgressBarDashedImage(UIColor* color)
{
	if (!color) return nil;
	CGFloat dash = 4.0;
	CGFloat gap  = 2.0;
	CGFloat height = 8.0;  /* arbitrary; image resizes to the bar's track */
	CGSize size = CGSizeMake(dash + gap, height);

	UIGraphicsBeginImageContextWithOptions(size, NO, 0);
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextSetFillColorWithColor(ctx, color.CGColor);
	CGContextFillRect(ctx, CGRectMake(0, 0, dash, height));
	UIImage* img = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	return [img resizableImageWithCapInsets:UIEdgeInsetsZero resizingMode:UIImageResizingModeTile];
}

static void cocoaTouchProgressBarApplyDashed(Ihandle* ih)
{
	IupCocoaTouchProgressView* bar = cocoaTouchProgressBarGetBar(ih);
	if (!bar) return;
	if (iupAttribGetBoolean(ih, "DASHED"))
	{
		UIColor* c = bar.progressTintColor ?: bar.tintColor;
		bar.progressImage = cocoaTouchProgressBarDashedImage(c);
	}
	else
	{
		bar.progressImage = nil;
	}
}

static int cocoaTouchProgressBarSetDashedAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	cocoaTouchProgressBarApplyDashed(ih);
	return 1;
}

static char* cocoaTouchProgressBarGetFgColorAttrib(Ihandle* ih)
{
	return iupCocoaTouchColorFromNative([cocoaTouchProgressBarGetBar(ih) progressTintColor]);
}

static int cocoaTouchProgressBarSetFgColorAttrib(Ihandle* ih, const char* color_str)
{
	UIColor* color = iupCocoaTouchToNativeColor(color_str);
	if (!color) return IUP_ERROR;
	UIActivityIndicatorView* spinner = cocoaTouchProgressBarGetSpinner(ih);
	if (spinner)
	{
		spinner.color = color;
		return IUP_NOERROR;
	}
	[cocoaTouchProgressBarGetBar(ih) setProgressTintColor:color];
	/* dashed fill bakes the color into the tile; rebuild it */
	if (iupAttribGetBoolean(ih, "DASHED")) cocoaTouchProgressBarApplyDashed(ih);
	return IUP_NOERROR;
}

static char* cocoaTouchProgressBarGetBgColorAttrib(Ihandle* ih)
{
	return iupCocoaTouchColorFromNative([cocoaTouchProgressBarGetBar(ih) trackTintColor]);
}

static int cocoaTouchProgressBarSetBgColorAttrib(Ihandle* ih, const char* color_str)
{
	UIColor* color = iupCocoaTouchToNativeColor(color_str);
	if (color)
	{
		[cocoaTouchProgressBarGetBar(ih) setTrackTintColor:color];
		return IUP_NOERROR;
	}
	return IUP_ERROR;
}

static char* cocoaTouchProgressBarGetMarqueeAttrib(Ihandle* ih)
{
	return iupStrReturnBoolean([cocoaTouchProgressBarGetBar(ih) indeterminate]);
}

static int cocoaTouchProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
	int on = iupStrBoolean(value) ? 1 : 0;
	ih->data->marquee = on;
	UIActivityIndicatorView* spinner = cocoaTouchProgressBarGetSpinner(ih);
	if (spinner)
	{
		if (on) [spinner startAnimating]; else [spinner stopAnimating];
		return 1;
	}
	[cocoaTouchProgressBarGetBar(ih) setIndeterminate:on ? YES : NO];
	return 1;
}

static char* cocoaTouchProgressBarGetOrientationAttrib(Ihandle* ih)
{
	id handle = ih ? (id)ih->handle : nil;
	if ([handle isKindOfClass:[IupCocoaTouchProgressContainer class]])
		return ((IupCocoaTouchProgressContainer*)handle).isVertical ? "VERTICAL" : "HORIZONTAL";
	return "HORIZONTAL";
}

IUP_SDK_API void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
{
	if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
	{
		if (w) *w = 4;
		if (h) *h = 80;
	}
	else
	{
		if (w) *w = 80;
		if (h) *h = 4;
	}
}

IUP_SDK_API void iupdrvProgressBarInitClass(Iclass* ic)
{
	ic->Map = cocoaTouchProgressBarMapMethod;
	ic->UnMap = iupdrvBaseUnMapMethod;

	iupClassRegisterAttribute(ic, "BGCOLOR", cocoaTouchProgressBarGetBgColorAttrib, cocoaTouchProgressBarSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", cocoaTouchProgressBarGetFgColorAttrib, cocoaTouchProgressBarSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "VALUE", cocoaTouchProgressBarGetValueAttrib, cocoaTouchProgressBarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ORIENTATION", cocoaTouchProgressBarGetOrientationAttrib, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MARQUEE", cocoaTouchProgressBarGetMarqueeAttrib, cocoaTouchProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DASHED", NULL, cocoaTouchProgressBarSetDashedAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
