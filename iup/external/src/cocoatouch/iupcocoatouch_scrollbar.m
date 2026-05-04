/** \file
 * \brief Scrollbar (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_scrollbar.h"

#include "iupcocoatouch_drv.h"


@interface IupCocoaTouchScrollbar : UIView
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, retain) UIView* track;
@property(nonatomic, retain) UIView* thumb;
@property(nonatomic, assign) CGFloat dragAnchor;
@property(nonatomic, assign) double  dragStartVal;
- (void)refreshFromData;
@end


static void cocoaTouchScrollbarFire(Ihandle* ih, int op, double old_val)
{
	IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
	if (scroll_cb)
	{
		float posx = 0, posy = 0;
		if (ih->data->orientation == ISCROLLBAR_HORIZONTAL) posx = (float)ih->data->val;
		else                                                 posy = (float)ih->data->val;
		scroll_cb(ih, op, posx, posy);
	}

	IFn value_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
	if (value_cb && ih->data->val != old_val)
		value_cb(ih);
}

@implementation IupCocoaTouchScrollbar

- (instancetype)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self)
	{
		self.backgroundColor = [UIColor clearColor];
		self.autoresizingMask = UIViewAutoresizingNone;

		_track = [[UIView alloc] init];
		_track.backgroundColor = [UIColor colorWithWhite:0.85 alpha:1.0];
		_track.layer.cornerRadius = 4;
		_track.userInteractionEnabled = NO;
		[self addSubview:_track];

		_thumb = [[UIView alloc] init];
		_thumb.backgroundColor = [UIColor colorWithWhite:0.45 alpha:1.0];
		_thumb.layer.cornerRadius = 4;
		[self addSubview:_thumb];

		UIPanGestureRecognizer* pan = [[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)] autorelease];
		[_thumb addGestureRecognizer:pan];

		UITapGestureRecognizer* tap = [[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTap:)] autorelease];
		[self addGestureRecognizer:tap];
	}
	return self;
}

- (void)dealloc
{
	[_track release];
	[_thumb release];
	[super dealloc];
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	CGRect b = self.bounds;
	BOOL horiz = (_ihandle && _ihandle->data->orientation == ISCROLLBAR_HORIZONTAL);
	if (horiz)
		_track.frame = CGRectMake(0, (b.size.height - 8)/2, b.size.width, 8);
	else
		_track.frame = CGRectMake((b.size.width - 8)/2, 0, 8, b.size.height);
	[self refreshFromData];
}

- (CGRect)thumbRectForState:(double)val
{
	CGRect b = self.bounds;
	Ihandle* ih = _ihandle;
	if (!ih) return CGRectZero;

	BOOL horiz = (ih->data->orientation == ISCROLLBAR_HORIZONTAL);
	CGFloat length = horiz ? b.size.width : b.size.height;
	CGFloat thickness = horiz ? b.size.height : b.size.width;
	CGFloat thumb_cross = thickness;

	double range = ih->data->vmax - ih->data->vmin;
	if (range <= 0) range = 1.0;

	double proportion = ih->data->pagesize / range;
	if (proportion > 1.0) proportion = 1.0;
	if (proportion < 0.05) proportion = 0.05;

	CGFloat thumb_len = (CGFloat)(proportion * length);
	CGFloat track_len = length - thumb_len;

	double span = (ih->data->vmax - ih->data->pagesize) - ih->data->vmin;
	double norm = 0;
	if (span > 0) norm = (val - ih->data->vmin) / span;
	if (norm < 0) norm = 0;
	if (norm > 1) norm = 1;
	if (ih->data->inverted) norm = 1.0 - norm;

	CGFloat offset = (CGFloat)(norm * track_len);
	if (horiz)
		return CGRectMake(offset, 0, thumb_len, thumb_cross);
	return CGRectMake(0, offset, thumb_cross, thumb_len);
}

- (void)refreshFromData
{
	if (!_ihandle) return;
	_thumb.frame = [self thumbRectForState:_ihandle->data->val];
}

- (double)valueFromLocation:(CGPoint)loc withThumbLength:(CGFloat)thumb_len
{
	Ihandle* ih = _ihandle;
	if (!ih) return 0;
	BOOL horiz = (ih->data->orientation == ISCROLLBAR_HORIZONTAL);
	CGFloat length = horiz ? self.bounds.size.width : self.bounds.size.height;
	CGFloat track_len = length - thumb_len;
	CGFloat offset = horiz ? loc.x : loc.y;
	double norm = 0;
	if (track_len > 0) norm = offset / track_len;
	if (norm < 0) norm = 0;
	if (norm > 1) norm = 1;
	if (ih->data->inverted) norm = 1.0 - norm;
	double span = (ih->data->vmax - ih->data->pagesize) - ih->data->vmin;
	if (span < 0) span = 0;
	return ih->data->vmin + norm * span;
}

- (void)handleTap:(UITapGestureRecognizer*)g
{
	Ihandle* ih = _ihandle;
	if (!ih || g.state != UIGestureRecognizerStateEnded) return;

	CGPoint p = [g locationInView:self];
	CGRect thumb_rect = _thumb.frame;
	if (CGRectContainsPoint(thumb_rect, p)) return;

	BOOL horiz = (ih->data->orientation == ISCROLLBAR_HORIZONTAL);
	CGFloat tap_pos = horiz ? p.x : p.y;
	CGFloat thumb_pos = horiz ? thumb_rect.origin.x : thumb_rect.origin.y;
	int before = (tap_pos < thumb_pos);
	if (ih->data->inverted) before = !before;

	double old_val = ih->data->val;
	double step = ih->data->pagestep;
	if (before) ih->data->val -= step;
	else        ih->data->val += step;
	iupScrollbarCropValue(ih);

	int op;
	if (horiz) op = before ? IUP_SBPGLEFT : IUP_SBPGRIGHT;
	else       op = before ? IUP_SBPGUP   : IUP_SBPGDN;

	[self refreshFromData];
	cocoaTouchScrollbarFire(ih, op, old_val);
}

- (void)handlePan:(UIPanGestureRecognizer*)g
{
	Ihandle* ih = _ihandle;
	if (!ih) return;

	BOOL horiz = (ih->data->orientation == ISCROLLBAR_HORIZONTAL);
	CGPoint loc = [g locationInView:self];

	if (g.state == UIGestureRecognizerStateBegan)
	{
		_dragAnchor = horiz ? (loc.x - _thumb.frame.origin.x) : (loc.y - _thumb.frame.origin.y);
		_dragStartVal = ih->data->val;
		return;
	}

	if (g.state != UIGestureRecognizerStateChanged && g.state != UIGestureRecognizerStateEnded) return;

	CGPoint thumb_origin = CGPointMake(horiz ? (loc.x - _dragAnchor) : 0,
	                                   horiz ? 0 : (loc.y - _dragAnchor));
	double old_val = ih->data->val;
	ih->data->val = [self valueFromLocation:thumb_origin withThumbLength:(horiz ? _thumb.frame.size.width : _thumb.frame.size.height)];
	iupScrollbarCropValue(ih);

	[self refreshFromData];

	int op = horiz ? IUP_SBDRAGH : IUP_SBDRAGV;
	cocoaTouchScrollbarFire(ih, op, old_val);
}

@end


IUP_SDK_API void iupdrvScrollbarGetMinSize(Ihandle* ih, int* w, int* h)
{
	int thumb = 20;
	int len = 100;
	if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
	{
		if (w) *w = len;
		if (h) *h = thumb;
	}
	else
	{
		if (w) *w = thumb;
		if (h) *h = len;
	}
}

static int cocoaTouchScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
	if (iupStrToDouble(value, &(ih->data->val)))
	{
		iupScrollbarCropValue(ih);
		if (ih->handle && [(id)ih->handle isKindOfClass:[IupCocoaTouchScrollbar class]])
			[(IupCocoaTouchScrollbar*)ih->handle refreshFromData];
	}
	return 0;
}

static int cocoaTouchScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
	iupStrToDoubleDef(value, &(ih->data->linestep), 0.01);
	return 0;
}

static int cocoaTouchScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
	iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1);
	return 0;
}

static int cocoaTouchScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
	if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
	{
		iupScrollbarCropValue(ih);
		if (ih->handle && [(id)ih->handle isKindOfClass:[IupCocoaTouchScrollbar class]])
			[(IupCocoaTouchScrollbar*)ih->handle refreshFromData];
	}
	return 0;
}

static int cocoaTouchScrollbarMapMethod(Ihandle* ih)
{
	IupCocoaTouchScrollbar* view = [[IupCocoaTouchScrollbar alloc] initWithFrame:CGRectZero];
	view.ihandle = ih;
	ih->handle = view;
	objc_setAssociatedObject(view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
	iupCocoaTouchAddToParent(ih);
	[view refreshFromData];
	return IUP_NOERROR;
}

IUP_SDK_API void iupdrvScrollbarInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchScrollbarMapMethod;
	ic->UnMap = iupdrvBaseUnMapMethod;

	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

	iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, cocoaTouchScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, cocoaTouchScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, cocoaTouchScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, cocoaTouchScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
