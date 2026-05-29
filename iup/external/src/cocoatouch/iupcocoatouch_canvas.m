/** \file
 * \brief Canvas Control (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_key.h"
#include "iup_canvas.h"
#include "iup_focus.h"
#include "iup_class.h"

#include "iupcocoatouch_drv.h"
#include "iupcocoatouch_draw.h"


/* canvas drives its own drag when BUTTON_CB+MOTION_CB are both set; pause ancestor scroll then */
static BOOL cocoaTouchCanvasIsDragHandle(Ihandle* ih)
{
	if (!ih) return NO;
	return (IupGetCallback(ih, "BUTTON_CB") && IupGetCallback(ih, "MOTION_CB")) ? YES : NO;
}


@interface IupCocoaTouchCanvasView : UIView <UIGestureRecognizerDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) CGSize previousSize;
@property(nonatomic, assign) BOOL canFocus;
@property(nonatomic, retain) UIColor* customBackground;
@property(nonatomic, retain) UIPanGestureRecognizer* scrollGesture;
@property(nonatomic, retain) UILongPressGestureRecognizer* longPressGesture;
@property(nonatomic, retain) UIPinchGestureRecognizer* pinchGesture;
@property(nonatomic, retain) UIRotationGestureRecognizer* rotateGesture;
@property(nonatomic, retain) UIPanGestureRecognizer* panGesture;
@property(nonatomic, retain) UITapGestureRecognizer* singleTapGesture;
@property(nonatomic, retain) UITapGestureRecognizer* doubleTapGesture;
@property(nonatomic, retain) NSMutableArray<UISwipeGestureRecognizer*>* swipeGestures;
@property(nonatomic, retain) NSMutableArray<UIGestureRecognizer*>* pausedAncestorPans;
@end

/* observe touches without stealing them, so raw TOUCH/BUTTON/MOTION still fire */
static void cocoaTouchCanvasPrepGesture(UIGestureRecognizer* g, id delegate)
{
	g.cancelsTouchesInView = NO;
	g.delaysTouchesBegan = NO;
	g.delaysTouchesEnded = NO;
	g.delegate = delegate;
}

static int cocoaTouchGestureState(UIGestureRecognizerState s)
{
	switch (s)
	{
		case UIGestureRecognizerStateBegan:   return IUP_GESTURE_BEGIN;
		case UIGestureRecognizerStateChanged: return IUP_GESTURE_CHANGED;
		case UIGestureRecognizerStateEnded:   return IUP_GESTURE_END;
		default:                              return IUP_GESTURE_CANCEL;
	}
}

static void cocoaTouchFireGesture(Ihandle* ih, int gesture, int state, int x, int y, double v1, double v2)
{
	IFniiiidd cb = (IFniiiidd)IupGetCallback(ih, "GESTURE_CB");
	if (cb && cb(ih, gesture, state, x, y, v1, v2) == IUP_CLOSE) IupExitLoop();
}

@implementation IupCocoaTouchCanvasView

- (instancetype)initWithFrame:(CGRect)frame ihandle:(Ihandle*)ih
{
	self = [super initWithFrame:frame];
	if (self)
	{
		_ihandle = ih;
		_previousSize = CGSizeZero;
		_canFocus = YES;
		self.contentMode = UIViewContentModeRedraw;
		self.multipleTouchEnabled = YES;
		self.opaque = NO;
		self.backgroundColor = [UIColor clearColor];
		self.clipsToBounds = YES;

		/* trackpad/wheel only; direct touches go through touchesBegan/Moved/Ended */
		_scrollGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onScroll:)];
		_scrollGesture.minimumNumberOfTouches = 0;
		_scrollGesture.maximumNumberOfTouches = 0;
		_scrollGesture.delegate = self;
		_scrollGesture.allowedScrollTypesMask = UIScrollTypeMaskAll;
		[self addGestureRecognizer:_scrollGesture];

		/* long-press dispatches IUP_BUTTON3 so desktop right-click apps work via touch */
		_longPressGesture = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(onLongPress:)];
		_longPressGesture.delegate = self;
		[self addGestureRecognizer:_longPressGesture];

		_pinchGesture = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(onPinch:)];
		cocoaTouchCanvasPrepGesture(_pinchGesture, self);
		[self addGestureRecognizer:_pinchGesture];

		_rotateGesture = [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(onRotate:)];
		cocoaTouchCanvasPrepGesture(_rotateGesture, self);
		[self addGestureRecognizer:_rotateGesture];

		/* two fingers: one-finger drag stays BUTTON_CB+MOTION_CB */
		_panGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPan:)];
		_panGesture.minimumNumberOfTouches = 2;
		_panGesture.maximumNumberOfTouches = 2;
		cocoaTouchCanvasPrepGesture(_panGesture, self);
		[self addGestureRecognizer:_panGesture];

		_doubleTapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTap:)];
		_doubleTapGesture.numberOfTapsRequired = 2;
		cocoaTouchCanvasPrepGesture(_doubleTapGesture, self);
		[self addGestureRecognizer:_doubleTapGesture];

		_singleTapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTap:)];
		_singleTapGesture.numberOfTapsRequired = 1;
		[_singleTapGesture requireGestureRecognizerToFail:_doubleTapGesture];
		[_singleTapGesture requireGestureRecognizerToFail:_longPressGesture];
		cocoaTouchCanvasPrepGesture(_singleTapGesture, self);
		[self addGestureRecognizer:_singleTapGesture];

		_swipeGestures = [[NSMutableArray alloc] init];
		UISwipeGestureRecognizerDirection dirs[4] = {
			UISwipeGestureRecognizerDirectionRight, UISwipeGestureRecognizerDirectionLeft,
			UISwipeGestureRecognizerDirectionUp,    UISwipeGestureRecognizerDirectionDown };
		for (int i = 0; i < 4; i++)
		{
			UISwipeGestureRecognizer* sw = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(onSwipe:)];
			sw.direction = dirs[i];
			cocoaTouchCanvasPrepGesture(sw, self);
			[self addGestureRecognizer:sw];
			[_swipeGestures addObject:sw];
			[sw release];
		}
	}
	return self;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)a shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)b
{
	/* pinch, rotate and two-finger pan must run together; tap/long-press/swipe stay exclusive */
	NSArray* combo = @[_pinchGesture, _rotateGesture, _panGesture];
	return ([combo containsObject:a] && [combo containsObject:b]) ? YES : NO;
}

- (void)dealloc
{
	[_customBackground release];
	[_scrollGesture release];
	[_longPressGesture release];
	[_pinchGesture release];
	[_rotateGesture release];
	[_panGesture release];
	[_singleTapGesture release];
	[_doubleTapGesture release];
	[_swipeGestures release];
	[_pausedAncestorPans release];
	[super dealloc];
}

- (void)onLongPress:(UILongPressGestureRecognizer*)g
{
	if (!_ihandle) return;
	if (g.state != UIGestureRecognizerStateBegan) return;

	CGPoint p = [g locationInView:self];
	cocoaTouchFireGesture(_ihandle, IUP_GESTURE_LONGPRESS, IUP_GESTURE_END, (int)p.x, (int)p.y, 0.0, 0.0);

	IFniiiis cb = (IFniiiis)IupGetCallback(_ihandle, "BUTTON_CB");
	if (!cb) return;
	char status[IUPKEY_STATUS_SIZE];
	iupCocoaTouchButtonKeySetStatus(nil, 0, 1, 0, status);
	if (cb(_ihandle, IUP_BUTTON3, 1, (int)p.x, (int)p.y, status) == IUP_CLOSE) IupExitLoop();
	iupCocoaTouchButtonKeySetStatus(nil, 0, 0, 0, status);
	if (cb(_ihandle, IUP_BUTTON3, 0, (int)p.x, (int)p.y, status) == IUP_CLOSE) IupExitLoop();
}

- (void)onPinch:(UIPinchGestureRecognizer*)g
{
	CGPoint p = [g locationInView:self];
	cocoaTouchFireGesture(_ihandle, IUP_GESTURE_PINCH, cocoaTouchGestureState(g.state), (int)p.x, (int)p.y, (double)g.scale, 0.0);
}

- (void)onRotate:(UIRotationGestureRecognizer*)g
{
	CGPoint p = [g locationInView:self];
	/* degrees, clockwise-positive to match the Android two-finger angle */
	double deg = (double)g.rotation * 180.0 / M_PI;
	cocoaTouchFireGesture(_ihandle, IUP_GESTURE_ROTATE, cocoaTouchGestureState(g.state), (int)p.x, (int)p.y, deg, 0.0);
}

- (void)onPan:(UIPanGestureRecognizer*)g
{
	CGPoint p = [g locationInView:self];
	CGPoint t = [g translationInView:self];
	cocoaTouchFireGesture(_ihandle, IUP_GESTURE_PAN, cocoaTouchGestureState(g.state), (int)p.x, (int)p.y, (double)t.x, (double)t.y);
}

- (void)onTap:(UITapGestureRecognizer*)g
{
	CGPoint p = [g locationInView:self];
	cocoaTouchFireGesture(_ihandle, IUP_GESTURE_TAP, IUP_GESTURE_END, (int)p.x, (int)p.y, (double)g.numberOfTapsRequired, 0.0);
}

- (void)onSwipe:(UISwipeGestureRecognizer*)g
{
	CGPoint p = [g locationInView:self];
	int dir;
	switch (g.direction)
	{
		case UISwipeGestureRecognizerDirectionLeft: dir = IUP_GESTURE_SWIPE_LEFT; break;
		case UISwipeGestureRecognizerDirectionUp:   dir = IUP_GESTURE_SWIPE_UP;   break;
		case UISwipeGestureRecognizerDirectionDown: dir = IUP_GESTURE_SWIPE_DOWN; break;
		default:                                    dir = IUP_GESTURE_SWIPE_RIGHT; break;
	}
	cocoaTouchFireGesture(_ihandle, IUP_GESTURE_SWIPE, IUP_GESTURE_END, (int)p.x, (int)p.y, (double)dir, 0.0);
}

- (BOOL)canBecomeFirstResponder
{
	return _canFocus && _ihandle != NULL;
}

- (BOOL)becomeFirstResponder
{
	BOOL ok = [super becomeFirstResponder];
	if (ok && _ihandle)
	{
		iupCallGetFocusCb(_ihandle);
		IFni cb = (IFni)IupGetCallback(_ihandle, "FOCUS_CB");
		if (cb && cb(_ihandle, 1) == IUP_CLOSE)
		{
			IupExitLoop();
		}
	}
	return ok;
}

- (BOOL)resignFirstResponder
{
	BOOL ok = [super resignFirstResponder];
	if (ok && _ihandle)
	{
		iupCallKillFocusCb(_ihandle);
		IFni cb = (IFni)IupGetCallback(_ihandle, "FOCUS_CB");
		if (cb && cb(_ihandle, 0) == IUP_CLOSE)
		{
			IupExitLoop();
		}
	}
	return ok;
}

- (void)layoutSubviews
{
	[super layoutSubviews];

	CGSize size = self.bounds.size;
	if (CGSizeEqualToSize(size, _previousSize)) return;
	_previousSize = size;

	/* bounds changed; cached CGBitmapContext is stale */
	iupCocoaTouchCanvasReleaseBuffer(_ihandle);

	if (!_ihandle) return;

	IFnii cb = (IFnii)IupGetCallback(_ihandle, "RESIZE_CB");
	if (cb && !_ihandle->data->inside_resize)
	{
		_ihandle->data->inside_resize = 1;
		cb(_ihandle, (int)size.width, (int)size.height);
		_ihandle->data->inside_resize = 0;
	}

	[self setNeedsDisplay];
}

- (void)drawRect:(CGRect)rect
{
	if (!_ihandle) return;

	CGContextRef view_ctx = UIGraphicsGetCurrentContext();

	IFn cb = (IFn)IupGetCallback(_ihandle, "ACTION");
	if (cb)
	{
		iupAttribSetStrf(_ihandle, "CLIPRECT", "%.0f %.0f %.0f %.0f",
			(double)rect.origin.x,
			(double)rect.origin.y,
			(double)(rect.origin.x + rect.size.width  - 1),
			(double)(rect.origin.y + rect.size.height - 1));

		if (cb(_ihandle) == IUP_CLOSE)
		{
			IupExitLoop();
		}
		iupAttribSet(_ihandle, "CLIPRECT", NULL);
	}

	size_t pixel_w = 0, pixel_h = 0;
	CGFloat px_scale = 1.0;
	unsigned char* pixels = iupCocoaTouchCanvasGetPixels(_ihandle, &pixel_w, &pixel_h, &px_scale);
	if (pixels && view_ctx && pixel_w > 0 && pixel_h > 0)
	{
		CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, pixels, pixel_w * pixel_h * 4, NULL);
		CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
		CGImageRef snapshot = CGImageCreate(pixel_w, pixel_h, 8, 32, pixel_w * 4, cs,
			kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big,
			provider, NULL, false, kCGRenderingIntentDefault);
		CGColorSpaceRelease(cs);
		CGDataProviderRelease(provider);
		if (snapshot)
		{
			CGContextSaveGState(view_ctx);
			/* extra Y flip: offscreen pixels are Y-down (IUP) and drawRect ctx is also Y-down */
			CGContextTranslateCTM(view_ctx, 0, self.bounds.size.height);
			CGContextScaleCTM(view_ctx, 1.0, -1.0);
			CGContextDrawImage(view_ctx, self.bounds, snapshot);
			CGContextRestoreGState(view_ctx);
			CGImageRelease(snapshot);
			iupAttribSet(_ihandle, "_IUPCOCOATOUCH_BUFFER_PENDING", NULL);
			return;
		}
	}

	/* no buffer yet: paint background so the view isn't transparent */
	if (_customBackground && view_ctx)
	{
		CGContextSetFillColorWithColor(view_ctx, [_customBackground CGColor]);
		CGContextFillRect(view_ctx, rect);
	}
}

static UIKeyModifierFlags cocoaTouchCanvasModifiers(UIEvent* event)
{
	return event ? [event modifierFlags] : 0;
}

static void cocoaTouchCanvasFireButton(Ihandle* ih, UITouch* touch, UIEvent* event, UIView* view, int pressed)
{
	if (!ih || !touch) return;
	IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
	if (!cb) return;

	CGPoint p = [touch locationInView:view];
	char status[IUPKEY_STATUS_SIZE];
	iupCocoaTouchButtonKeySetStatus(event, cocoaTouchCanvasModifiers(event), pressed ? 1 : 0, 0, status);
	if (cb(ih, IUP_BUTTON1, pressed, (int)p.x, (int)p.y, status) == IUP_CLOSE) IupExitLoop();
}

static void cocoaTouchCanvasFireMotion(Ihandle* ih, UITouch* touch, UIEvent* event, UIView* view, int pressed)
{
	if (!ih || !touch) return;
	IFniis cb = (IFniis)IupGetCallback(ih, "MOTION_CB");
	if (!cb) return;

	CGPoint p = [touch locationInView:view];
	char status[IUPKEY_STATUS_SIZE];
	iupCocoaTouchButtonKeySetStatus(event, cocoaTouchCanvasModifiers(event), pressed ? 1 : 0, 0, status);
	if (cb(ih, (int)p.x, (int)p.y, status) == IUP_CLOSE) IupExitLoop();
}

/* fires TOUCH_CB per touch + one MULTITOUCH_CB for the batch; phase is 'D'/'U'/'M' */
- (void)dispatchTouchBatch:(NSSet<UITouch*>*)touches phase:(char)phase_char
{
	if (!_ihandle) return;

	IFniiis  single_cb = (IFniiis)IupGetCallback(_ihandle, "TOUCH_CB");
	IFniIIII multi_cb  = (IFniIIII)IupGetCallback(_ihandle, "MULTITOUCH_CB");
	if (!single_cb && !multi_cb) return;

	NSArray* list = [touches allObjects];
	int count = (int)list.count;
	if (count == 0) return;

	int* ids    = multi_cb ? (int*)malloc(sizeof(int) * count) : NULL;
	int* xs     = multi_cb ? (int*)malloc(sizeof(int) * count) : NULL;
	int* ys     = multi_cb ? (int*)malloc(sizeof(int) * count) : NULL;
	int* states = multi_cb ? (int*)malloc(sizeof(int) * count) : NULL;

	const char* phase_str = (phase_char == 'D') ? "DOWN" : ((phase_char == 'U') ? "UP" : "MOVE");

	for (int i = 0; i < count; i++)
	{
		UITouch* t = list[i];
		CGPoint p = [t locationInView:self];
		int tid = (int)([t hash] & 0x7FFFFFFFu);

		if (single_cb)
		{
			if (single_cb(_ihandle, tid, (int)p.x, (int)p.y, (char*)phase_str) == IUP_CLOSE)
			{
				IupExitLoop();
				break;
			}
		}
		if (multi_cb)
		{
			ids[i]    = tid;
			xs[i]     = (int)p.x;
			ys[i]     = (int)p.y;
			states[i] = phase_char;
		}
	}

	if (multi_cb)
	{
		if (multi_cb(_ihandle, count, ids, xs, ys, states) == IUP_CLOSE) IupExitLoop();
		free(ids); free(xs); free(ys); free(states);
	}
}

/* disable every ancestor UIPanGestureRecognizer so the canvas owns the drag gesture */
- (void)pauseAncestorPanGestures
{
	if (_pausedAncestorPans) return;
	if (!cocoaTouchCanvasIsDragHandle(_ihandle)) return;
	_pausedAncestorPans = [[NSMutableArray alloc] init];
	for (UIView* v = self.superview; v; v = v.superview)
	{
		for (UIGestureRecognizer* gr in v.gestureRecognizers)
		{
			if ([gr isKindOfClass:[UIPanGestureRecognizer class]] && gr.enabled)
			{
				gr.enabled = NO;
				[_pausedAncestorPans addObject:gr];
			}
		}
	}
	for (UIGestureRecognizer* gr in self.window.gestureRecognizers)
	{
		if ([gr isKindOfClass:[UIPanGestureRecognizer class]] && gr.enabled)
		{
			gr.enabled = NO;
			[_pausedAncestorPans addObject:gr];
		}
	}
}

- (void)resumeAncestorPanGestures
{
	if (!_pausedAncestorPans) return;
	for (UIGestureRecognizer* gr in _pausedAncestorPans)
		gr.enabled = YES;
	[_pausedAncestorPans release];
	_pausedAncestorPans = nil;
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesBegan:touches withEvent:event];
	if (_canFocus && !self.isFirstResponder) [self becomeFirstResponder];

	[self pauseAncestorPanGestures];

	UITouch* t = [touches anyObject];
	cocoaTouchCanvasFireButton(_ihandle, t, event, self, 1);
	cocoaTouchCanvasFireMotion(_ihandle, t, event, self, 1);
	[self dispatchTouchBatch:touches phase:'D'];

	IFn cb = (IFn)IupGetCallback(_ihandle, "ENTERWINDOW_CB");
	if (cb && cb(_ihandle) == IUP_CLOSE) IupExitLoop();
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesMoved:touches withEvent:event];
	cocoaTouchCanvasFireMotion(_ihandle, [touches anyObject], event, self, 1);
	[self dispatchTouchBatch:touches phase:'M'];
}


- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesEnded:touches withEvent:event];
	UITouch* t = [touches anyObject];
	cocoaTouchCanvasFireButton(_ihandle, t, event, self, 0);
	cocoaTouchCanvasFireMotion(_ihandle, t, event, self, 0);
	[self dispatchTouchBatch:touches phase:'U'];
	[self resumeAncestorPanGestures];

	IFn cb = (IFn)IupGetCallback(_ihandle, "LEAVEWINDOW_CB");
	if (cb && cb(_ihandle) == IUP_CLOSE) IupExitLoop();
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesCancelled:touches withEvent:event];
	UITouch* t = [touches anyObject];
	cocoaTouchCanvasFireButton(_ihandle, t, event, self, 0);
	[self dispatchTouchBatch:touches phase:'U'];
	[self resumeAncestorPanGestures];

	IFn cb = (IFn)IupGetCallback(_ihandle, "LEAVEWINDOW_CB");
	if (cb && cb(_ihandle) == IUP_CLOSE) IupExitLoop();
}

- (void)pressesBegan:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	BOOL handled = NO;
	for (UIPress* press in presses)
	{
		if (iupCocoaTouchKeyEvent(_ihandle, press, true)) handled = YES;
	}
	if (!handled) [super pressesBegan:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	BOOL handled = NO;
	for (UIPress* press in presses)
	{
		if (iupCocoaTouchKeyEvent(_ihandle, press, false)) handled = YES;
	}
	if (!handled) [super pressesEnded:presses withEvent:event];
}

- (void)onScroll:(UIPanGestureRecognizer*)gr
{
	if (!_ihandle) return;
	IFnfiis cb = (IFnfiis)IupGetCallback(_ihandle, "WHEEL_CB");
	if (!cb) return;

	CGPoint t = [gr translationInView:self];
	CGPoint p = [gr locationInView:self];
	/* reset translation after each fire so we get per-tick deltas */
	[gr setTranslation:CGPointZero inView:self];

	/* IUP delta is positive = forward/up; UIPan y grows down */
	float delta = (float)(-t.y / 10.0);
	if (delta == 0.0f) return;

	char status[IUPKEY_STATUS_SIZE];
	memcpy(status, IUPKEY_STATUS_INIT, IUPKEY_STATUS_SIZE);
	if (cb(_ihandle, delta, (int)p.x, (int)p.y, status) == IUP_CLOSE) IupExitLoop();
}

@end


static IupCocoaTouchCanvasView* cocoaTouchCanvasGet(Ihandle* ih)
{
	if ([(id)ih->handle isKindOfClass:[IupCocoaTouchCanvasView class]])
		return (IupCocoaTouchCanvasView*)ih->handle;
	return nil;
}

static int cocoaTouchCanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchCanvasView* view = cocoaTouchCanvasGet(ih);
	if (!view) return 0;

	UIColor* color = iupCocoaTouchToNativeColor(value);
	view.customBackground = color;
	view.backgroundColor = color;
	[view setNeedsDisplay];
	return 1;
}

static char* cocoaTouchCanvasGetDrawSizeAttrib(Ihandle* ih)
{
	IupCocoaTouchCanvasView* view = cocoaTouchCanvasGet(ih);
	if (!view) return NULL;
	CGSize s = [view bounds].size;
	return iupStrReturnIntInt((int)s.width, (int)s.height, 'x');
}

/* No native canvas scrollbar on iOS; the setters track POSX/POSY and fire SCROLL_CB on a position change. */
static int cocoaTouchCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
	(void)ih; (void)value;
	return 1;
}

static int cocoaTouchCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
	(void)ih; (void)value;
	return 1;
}

static int cocoaTouchCanvasSetPosXAttrib(Ihandle* ih, const char* value)
{
	double xmin = iupAttribGetDouble(ih, "XMIN");
	double xmax = iupAttribGetDouble(ih, "XMAX");
	double dx   = iupAttribGetDouble(ih, "DX");
	double posx;
	if (!iupStrToDouble(value, &posx)) return 1;
	if (dx >= xmax - xmin) return 0;
	if (posx < xmin) posx = xmin;
	if (posx > (xmax - dx)) posx = xmax - dx;
	ih->data->posx = posx;

	IFniff cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
	if (cb) cb(ih, IUP_SBPOSH, (float)posx, (float)ih->data->posy);
	return 1;
}

static int cocoaTouchCanvasSetPosYAttrib(Ihandle* ih, const char* value)
{
	double ymin = iupAttribGetDouble(ih, "YMIN");
	double ymax = iupAttribGetDouble(ih, "YMAX");
	double dy   = iupAttribGetDouble(ih, "DY");
	double posy;
	if (!iupStrToDouble(value, &posy)) return 1;
	if (dy >= ymax - ymin) return 0;
	if (posy < ymin) posy = ymin;
	if (posy > (ymax - dy)) posy = ymax - dy;
	ih->data->posy = posy;

	IFniff cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
	if (cb) cb(ih, IUP_SBPOSV, (float)ih->data->posx, (float)posy);
	return 1;
}

static int cocoaTouchCanvasSetBorderAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchCanvasView* view = cocoaTouchCanvasGet(ih);
	if (!view) return 1;
	if (iupStrBoolean(value))
	{
		view.layer.borderWidth = 1.0;
		/* layer.borderColor is a CGColor snapshot; re-snapshot on theme flip. */
		view.layer.borderColor = [UIColor separatorColor].CGColor;
		iupCocoaTouchRegisterThemeRefresh(view, ^(UIView* v) {
			v.layer.borderColor = [UIColor separatorColor].CGColor;
		});
	}
	else
	{
		view.layer.borderWidth = 0.0;
		view.layer.borderColor = nil;
	}
	return 1;
}

static char* cocoaTouchCanvasGetCgContextAttrib(Ihandle* ih)
{
	/* Pointer-as-string, same shape desktop drivers use for HDC/CGContextRef. */
	CGContextRef ctx = (CGContextRef)iupAttribGet(ih, "_IUPCOCOATOUCH_CANVAS_CGCONTEXT");
	return iupStrReturnStrf("%p", ctx);
}

static char* cocoaTouchCanvasGetUiViewAttrib(Ihandle* ih)
{
	return iupStrReturnStrf("%p", ih->handle);
}

static char* cocoaTouchCanvasGetDrawableAttrib(Ihandle* ih)
{
	return iupStrReturnStrf("%p", ih->handle);
}


static int cocoaTouchCanvasMapMethod(Ihandle* ih)
{
	IupCocoaTouchCanvasView* view = [[IupCocoaTouchCanvasView alloc] initWithFrame:CGRectZero ihandle:ih];

	BOOL can_focus = iupAttribGetBoolean(ih, "CANFOCUS") ? YES : NO;
	view.canFocus = can_focus;

	ih->handle = view;
	objc_setAssociatedObject(view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

	iupCocoaTouchAddToParent(ih);

	/* VoiceOver: pass touches through to custom canvas interactions */
	view.accessibilityTraits = UIAccessibilityTraitAllowsDirectInteraction;

	iupCocoaTouchRegisterThemeRefresh(view, ^(UIView* v) { [v setNeedsDisplay]; });

	iupAttribSet(ih, "DRAWDRIVER", "COCOATOUCH");
	return IUP_NOERROR;
}

static void cocoaTouchCanvasUnMapMethod(Ihandle* ih)
{
	IupCocoaTouchCanvasView* view = cocoaTouchCanvasGet(ih);
	if (view)
	{
		[view setIhandle:NULL];
		objc_setAssociatedObject(view, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	}
	iupCocoaTouchCanvasReleaseBuffer(ih);
	iupdrvBaseUnMapMethod(ih);
}

IUP_SDK_API void iupdrvCanvasInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchCanvasMapMethod;
	ic->UnMap = cocoaTouchCanvasUnMapMethod;

	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTouchCanvasSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "BORDER", NULL, cocoaTouchCanvasSetBorderAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "DRAWSIZE", cocoaTouchCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DRAWDRIVER", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, cocoaTouchCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, cocoaTouchCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "DX", NULL, cocoaTouchCanvasSetDXAttrib, "0.1", NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DY", NULL, cocoaTouchCanvasSetDYAttrib, "0.1", NULL, IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "CANFOCUS", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	/* Native handle access, parallel to HDC on Win32 and CGContext on Cocoa. */
	iupClassRegisterAttribute(ic, "CGCONTEXT", cocoaTouchCanvasGetCgContextAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "UIVIEW", cocoaTouchCanvasGetUiViewAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "DRAWABLE", cocoaTouchCanvasGetDrawableAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

	/* iOS has no native scrollbars on plain UIView; POSX/POSY/DX/DY still let IUP track scroll state without chrome. */
	iupClassRegisterAttribute(ic, "SCROLLBAR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "XHIDDEN", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "YHIDDEN", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "WHEELDROPFOCUS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "GESTURE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "NATIVEFOCUSRING",NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

	iupClassRegisterCallback(ic, "GESTURE_CB", "iiiidd");
}
