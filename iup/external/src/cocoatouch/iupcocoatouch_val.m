/** \file
 * \brief Valuator Control (iOS UIKit, UISlider)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <stdio.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_val.h"

#include "iupcocoatouch_drv.h"


static const void* IUP_COCOATOUCH_VAL_TARGET_OBJ_KEY = "IUP_COCOATOUCH_VAL_TARGET_OBJ_KEY";

/* UISlider is horizontal-only; vertical is a 90 deg CTM rotation */
@interface IupCocoaTouchValSlider : UISlider
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) BOOL vertical;
@property(nonatomic, assign) BOOL inverted;
@property(nonatomic, assign) int showTicks;
@property(nonatomic, retain) NSMutableArray<UIView*>* tickViews;
- (void)refreshTicks;
@end

@implementation IupCocoaTouchValSlider

- (void)dealloc
{
	[_tickViews release];
	[super dealloc];
}

- (void)applyOrientationTransform
{
	/* rotate -PI/2 so VERTICAL+INVERTED=YES puts min at the bottom (IUP default) */
	CGAffineTransform t = CGAffineTransformIdentity;

	if (_vertical)
	{
		CGFloat angle = _inverted ? -M_PI_2 : M_PI_2;
		t = CGAffineTransformMakeRotation(angle);
	}
	else if (_inverted)
	{
		t = CGAffineTransformMakeScale(-1.0, 1.0);
	}
	self.transform = t;
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	[self refreshTicks];
}

- (void)refreshTicks
{
	if (!_tickViews) _tickViews = [[NSMutableArray alloc] init];
	for (UIView* v in _tickViews) [v removeFromSuperview];
	[_tickViews removeAllObjects];

	int n = _showTicks;
	if (n < 2) return;

	CGRect track = [self trackRectForBounds:self.bounds];
	CGFloat tick_len = 6.0;
	UIColor* color = [UIColor labelColor];
	for (int i = 0; i < n; i++)
	{
		CGFloat t = (CGFloat)i / (CGFloat)(n - 1);
		CGFloat cx = track.origin.x + t * track.size.width;
		CGRect r = CGRectMake(cx - 0.5, CGRectGetMaxY(track) + 2, 1.0, tick_len);
		UIView* tick = [[UIView alloc] initWithFrame:r];
		tick.backgroundColor = color;
		tick.userInteractionEnabled = NO;
		[self addSubview:tick];
		[_tickViews addObject:tick];
		[tick release];
	}
}

@end


@interface IupCocoaTouchValTarget : NSObject
@end

@implementation IupCocoaTouchValTarget

- (void)onValueChanged:(id)sender
{
	if (![sender isKindOfClass:[IupCocoaTouchValSlider class]]) return;
	Ihandle* ih = ((IupCocoaTouchValSlider*)sender).ihandle;
	if (!ih || !iupObjectCheck(ih)) return;
	ih->data->val = [(IupCocoaTouchValSlider*)sender value];
	iupValCropValue(ih);

	IFn cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
	if (cb) { if (cb(ih) == IUP_CLOSE) IupExitLoop(); return; }

	IFnd cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
	if (cb_old && cb_old(ih, ih->data->val) == IUP_CLOSE) IupExitLoop();
}

- (void)onTouchDown:(id)sender
{
	if (![sender isKindOfClass:[IupCocoaTouchValSlider class]]) return;
	Ihandle* ih = ((IupCocoaTouchValSlider*)sender).ihandle;
	if (!ih || !iupObjectCheck(ih)) return;
	IFnd cb = (IFnd)IupGetCallback(ih, "BUTTON_PRESS_CB");
	if (cb && cb(ih, ih->data->val) == IUP_CLOSE) IupExitLoop();
}

- (void)onTouchUp:(id)sender
{
	if (![sender isKindOfClass:[IupCocoaTouchValSlider class]]) return;
	Ihandle* ih = ((IupCocoaTouchValSlider*)sender).ihandle;
	if (!ih || !iupObjectCheck(ih)) return;
	IFnd cb = (IFnd)IupGetCallback(ih, "BUTTON_RELEASE_CB");
	if (cb && cb(ih, ih->data->val) == IUP_CLOSE) IupExitLoop();
}

@end


static IupCocoaTouchValSlider* cocoaTouchValGet(Ihandle* ih)
{
	if ([(id)ih->handle isKindOfClass:[IupCocoaTouchValSlider class]])
	{
		return (IupCocoaTouchValSlider*)ih->handle;
	}
	return nil;
}

/* MIN/MAX setters don't push to native; this re-syncs from ih->data */
static void cocoaTouchValSyncFromData(Ihandle* ih)
{
	IupCocoaTouchValSlider* slider = cocoaTouchValGet(ih);
	if (!slider) return;

	float vmin = (float)ih->data->vmin;
	float vmax = (float)ih->data->vmax;
	if (vmax < vmin) { float t = vmin; vmin = vmax; vmax = t; }

	[slider setMinimumValue:vmin];
	[slider setMaximumValue:vmax];
	[slider setValue:(float)ih->data->val animated:NO];
}

static int cocoaTouchValSetValueAttrib(Ihandle* ih, const char* value)
{
	double v;
	if (value && iupStrToDouble(value, &v))
	{
		ih->data->val = v;
		iupValCropValue(ih);
	}
	cocoaTouchValSyncFromData(ih);
	return 0;
}

static int cocoaTouchValSetStepAttrib(Ihandle* ih, const char* value)
{
	double s;
	if (value && iupStrToDouble(value, &s) && s > 0 && s < 1)
	{
		ih->data->step = s;
	}
	return 0;
}

static int cocoaTouchValSetPageStepAttrib(Ihandle* ih, const char* value)
{
	double s;
	if (value && iupStrToDouble(value, &s) && s > 0 && s < 1)
	{
		ih->data->pagestep = s;
	}
	return 0;
}

static int cocoaTouchValSetInvertedAttrib(Ihandle* ih, const char* value)
{
	int inv = iupStrBoolean(value) ? 1 : 0;
	ih->data->inverted = inv;

	IupCocoaTouchValSlider* slider = cocoaTouchValGet(ih);
	if (slider)
	{
		slider.inverted = inv ? YES : NO;
		[slider applyOrientationTransform];
	}
	return 1;
}

static int cocoaTouchValSetBgColorAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchValSlider* slider = cocoaTouchValGet(ih);
	UIColor* color = iupCocoaTouchToNativeColor(value);
	if (!slider || !color)
	{
		return 0;
	}
	/* BGCOLOR -> trailing (post-value) track */
	[slider setMaximumTrackTintColor:color];
	return 1;
}

static int cocoaTouchValSetFgColorAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchValSlider* slider = cocoaTouchValGet(ih);
	UIColor* color = iupCocoaTouchToNativeColor(value);
	if (!slider || !color)
	{
		return 0;
	}
	[slider setMinimumTrackTintColor:color];
	[slider setThumbTintColor:color];
	return 1;
}

IUP_SDK_API void iupdrvValGetMinSize(Ihandle* ih, int* w, int* h)
{
	int major = 180;
	int minor = 31;
	if (iupAttribGetInt(ih, "SHOWTICKS") > 0) minor += 10;
	if (ih->data->orientation == IVAL_HORIZONTAL)
	{
		*w = major; *h = minor;
	}
	else
	{
		*w = minor; *h = major;
	}
}

static int cocoaTouchValCreateMethod(Ihandle* ih, void** params)
{
	const char* orientation = (params && params[0]) ? (const char*)params[0] : "HORIZONTAL";

	ih->data = iupALLOCCTRLDATA();
	if (iupStrEqualNoCase(orientation, "VERTICAL"))
	{
		ih->data->orientation = IVAL_VERTICAL;
		ih->data->inverted = 1;
	}
	else
		ih->data->orientation = IVAL_HORIZONTAL;

	ih->data->vmax = 1.00;
	ih->data->step = 0.01;
	ih->data->pagestep = 0.10;
	return IUP_NOERROR;
}

static void cocoaTouchValComputeNaturalSize(Ihandle* ih, int* w, int* h, int* children_expand)
{
	int min_w = 0, min_h = 0;
	iupdrvValGetMinSize(ih, &min_w, &min_h);
	*w = iupdrvScaleNaturalPx(min_w);
	*h = iupdrvScaleNaturalPx(min_h);
	(void)children_expand;
}

static int cocoaTouchValSetShowTicksAttrib(Ihandle* ih, const char* value)
{
	int n = 0;
	if (value) iupStrToInt(value, &n);
	if (n < 0) n = 0;
	ih->data->show_ticks = n;
	IupCocoaTouchValSlider* slider = cocoaTouchValGet(ih);
	if (slider)
	{
		slider.showTicks = n;
		[slider refreshTicks];
	}
	return 1;
}

static int cocoaTouchValMapMethod(Ihandle* ih)
{
	IupCocoaTouchValSlider* slider = [[IupCocoaTouchValSlider alloc] initWithFrame:CGRectZero];
	slider.ihandle = ih;
	slider.vertical = (ih->data->orientation == IVAL_VERTICAL) ? YES : NO;
	slider.inverted = ih->data->inverted ? YES : NO;
	slider.showTicks = ih->data->show_ticks;

	[slider applyOrientationTransform];

	/* seed native min/max/value from ih->data after MIN/MAX/VALUE replay */
	cocoaTouchValSyncFromData(ih);

	IupCocoaTouchValTarget* target = [[IupCocoaTouchValTarget alloc] init];
	[slider addTarget:target action:@selector(onValueChanged:) forControlEvents:UIControlEventValueChanged];
	[slider addTarget:target action:@selector(onTouchDown:)    forControlEvents:UIControlEventTouchDown];
	[slider addTarget:target action:@selector(onTouchUp:)      forControlEvents:UIControlEventTouchUpInside];
	[slider addTarget:target action:@selector(onTouchUp:)      forControlEvents:UIControlEventTouchUpOutside];
	[slider addTarget:target action:@selector(onTouchUp:)      forControlEvents:UIControlEventTouchCancel];

	objc_setAssociatedObject(slider, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
	objc_setAssociatedObject(slider, IUP_COCOATOUCH_VAL_TARGET_OBJ_KEY, target, OBJC_ASSOCIATION_RETAIN);
	[target release];

	ih->handle = slider;
	iupCocoaTouchAddToParent(ih);
	return IUP_NOERROR;
}

static void cocoaTouchValUnMapMethod(Ihandle* ih)
{
	IupCocoaTouchValSlider* slider = cocoaTouchValGet(ih);
	if (slider)
	{
		[slider setIhandle:NULL];
		id target = objc_getAssociatedObject(slider, IUP_COCOATOUCH_VAL_TARGET_OBJ_KEY);
		if (target)
		{
			[slider removeTarget:target action:NULL forControlEvents:UIControlEventAllEvents];
		}
		objc_setAssociatedObject(slider, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(slider, IUP_COCOATOUCH_VAL_TARGET_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	}
	iupdrvBaseUnMapMethod(ih);
}

IUP_SDK_API void iupdrvValInitClass(Iclass* ic)
{
	ic->Create = cocoaTouchValCreateMethod;
	ic->ComputeNaturalSize = cocoaTouchValComputeNaturalSize;
	ic->Map   = cocoaTouchValMapMethod;
	ic->UnMap = cocoaTouchValUnMapMethod;

	iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
	iupClassRegisterCallback(ic, "BUTTON_PRESS_CB", "");
	iupClassRegisterCallback(ic, "BUTTON_RELEASE_CB", "");

	iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTouchValSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTouchValSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

	iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, cocoaTouchValSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, cocoaTouchValSetStepAttrib, IUPAF_SAMEASSYSTEM, "0.01", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, cocoaTouchValSetPageStepAttrib, IUPAF_SAMEASSYSTEM, "0.1", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "INVERTED", NULL, cocoaTouchValSetInvertedAttrib, NULL, NULL, IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "SHOWTICKS", iupValGetShowTicksAttrib, cocoaTouchValSetShowTicksAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TICKSPOS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

}
