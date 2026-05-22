/** \file
 * \brief Toggle Control (iOS UIKit)
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
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_toggle.h"

#include "iupcocoatouch_drv.h"


static const void* IUP_COCOATOUCH_TOGGLE_TARGET_OBJ_KEY = "IUP_COCOATOUCH_TOGGLE_TARGET_OBJ_KEY";

typedef NS_ENUM(NSInteger, IupCocoaTouchToggleValue) {
	IupCocoaTouchToggleOff      = 0,
	IupCocoaTouchToggleOn       = 1,
	IupCocoaTouchToggleNotDef   = 2,
};

static IupCocoaTouchToggleValue cocoaTouchToggleValueFromStr(const char* value, IupCocoaTouchToggleValue current, BOOL three_state)
{
	if (!value) return IupCocoaTouchToggleOff;
	if (iupStrEqualNoCase(value, "TOGGLE"))
	{
		if (three_state)
		{
			return (IupCocoaTouchToggleValue)((current + 1) % 3);
		}
		return current == IupCocoaTouchToggleOn ? IupCocoaTouchToggleOff : IupCocoaTouchToggleOn;
	}
	if (three_state && iupStrEqualNoCase(value, "NOTDEF")) return IupCocoaTouchToggleNotDef;
	if (iupStrBoolean(value)) return IupCocoaTouchToggleOn;
	return IupCocoaTouchToggleOff;
}

static const char* cocoaTouchToggleValueToStr(IupCocoaTouchToggleValue value)
{
	switch (value)
	{
		case IupCocoaTouchToggleOn:     return "ON";
		case IupCocoaTouchToggleNotDef: return "NOTDEF";
		default:                         return "OFF";
	}
}


static UIButtonConfiguration* cocoaTouchToggleMakeBaseConfig(void)
{
	UIButtonConfiguration* cfg = [UIButtonConfiguration plainButtonConfiguration];
	cfg.imagePlacement = NSDirectionalRectEdgeLeading;
	cfg.imagePadding = 8;
	return cfg;
}

/* probe with a real title so SF symbol auto-scales like at runtime, then subtract title width */
static int cocoaTouchToggleCheckOverhead(void)
{
	static int overhead = -1;
	if (overhead < 0)
	{
		@autoreleasepool {
			IupCocoaTouchFont* iup_font = iupCocoaTouchFindFont(IupGetGlobal("DEFAULTFONT"));
			UIFont* font = (iup_font && iup_font.nativeFont) ? iup_font.nativeFont : [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
			NSDictionary* attrs = @{ NSFontAttributeName: font };
			NSString* sample = @"Mg";
			CGFloat text_w = [sample sizeWithAttributes:attrs].width;
			NSAttributedString* title = [[[NSAttributedString alloc] initWithString:sample attributes:attrs] autorelease];
			/* widest indicator (radio selected) + margin so a spaced title never wraps */
			UIButtonConfiguration* cfg = cocoaTouchToggleMakeBaseConfig();
			cfg.image = [UIImage systemImageNamed:@"largecircle.fill.circle"];
			cfg.attributedTitle = title;
			UIButton* probe = [UIButton buttonWithConfiguration:cfg primaryAction:nil];
			CGSize size = [probe intrinsicContentSize];
			overhead = (size.width > text_w) ? (int)ceil(size.width - text_w) + 3 : 30;
			if (overhead < 24) overhead = 24;
		}
	}
	return overhead;
}

static void cocoaTouchToggleSwitchSize(int* w, int* h)
{
	static int sw = -1, sh = -1;
	if (sw < 0)
	{
		@autoreleasepool {
			UISwitch* probe = [[UISwitch alloc] initWithFrame:CGRectZero];
			CGSize size = [probe intrinsicContentSize];
			sw = (size.width  > 0) ? (int)ceil(size.width)  : 61;
			sh = (size.height > 0) ? (int)ceil(size.height) : 28;
			[probe release];
		}
	}
	if (w) *w = sw;
	if (h) *h = sh;
}


@interface IupCocoaTouchToggleButton : UIButton
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) IupCocoaTouchToggleValue state_value;
@property(nonatomic, assign) BOOL isRadio;
@property(nonatomic, assign) BOOL is3State;
- (void)refreshIndicator;
@end

@implementation IupCocoaTouchToggleButton

- (instancetype)initWithIhandle:(Ihandle*)ih
{
	self = [super initWithFrame:CGRectZero];
	if (self)
	{
		_ihandle = ih;
		_state_value = IupCocoaTouchToggleOff;
		self.configuration = cocoaTouchToggleMakeBaseConfig();
		[self setContentHorizontalAlignment:UIControlContentHorizontalAlignmentLeft];
	}
	return self;
}

- (NSString*)symbolNameForState:(IupCocoaTouchToggleValue)state
{
	if (_isRadio)
	{
		return (state == IupCocoaTouchToggleOn) ? @"largecircle.fill.circle" : @"circle";
	}
	switch (state)
	{
		case IupCocoaTouchToggleOn:     return @"checkmark.square.fill";
		case IupCocoaTouchToggleNotDef: return @"minus.square";
		default:                         return @"square";
	}
}

- (UIImage*)indicatorImage
{
	UIImage* icon = [UIImage systemImageNamed:[self symbolNameForState:_state_value]];
	if (!icon) return nil;
	BOOL active = (_state_value != IupCocoaTouchToggleOff);
	UIColor* color = active ? self.tintColor : [UIColor secondaryLabelColor];
	return [icon imageWithTintColor:color renderingMode:UIImageRenderingModeAlwaysOriginal];
}

- (NSString*)indicatorFallbackGlyph
{
	const unichar on_char  = _isRadio ? 0x25C9 : 0x2611;
	const unichar off_char = _isRadio ? 0x25CB : 0x2610;
	const unichar mix_char = 0x2612;
	unichar glyph = (_state_value == IupCocoaTouchToggleOn)      ? on_char
	              : (_state_value == IupCocoaTouchToggleNotDef) ? mix_char
	                                                            : off_char;
	return [NSString stringWithFormat:@"%C", glyph];
}

- (void)refreshIndicator
{
	UIButtonConfiguration* cfg = [self.configuration copy] ?: [[UIButtonConfiguration plainButtonConfiguration] copy];
	cfg.image = [self indicatorImage];
	if (!cfg.baseForegroundColor)
		cfg.baseForegroundColor = [UIColor labelColor];
	self.configuration = cfg;
	[cfg release];
}

@end


@interface IupCocoaTouchToggleTarget : NSObject
- (void)onSwitchChanged:(id)sender;
- (void)onButtonTapped:(id)sender;
@end


static IupCocoaTouchToggleButton* cocoaTouchToggleGetButton(Ihandle* ih)
{
	if ([(id)ih->handle isKindOfClass:[IupCocoaTouchToggleButton class]])
	{
		return (IupCocoaTouchToggleButton*)ih->handle;
	}
	return nil;
}

static UISwitch* cocoaTouchToggleGetSwitch(Ihandle* ih)
{
	if ([(id)ih->handle isKindOfClass:[UISwitch class]])
	{
		return (UISwitch*)ih->handle;
	}
	return nil;
}

static UIButton* cocoaTouchToggleGetImageButton(Ihandle* ih)
{
	if (ih->data->type == IUP_TOGGLE_IMAGE && [(id)ih->handle isKindOfClass:[UIButton class]]
	    && ![(id)ih->handle isKindOfClass:[IupCocoaTouchToggleButton class]])
	{
		return (UIButton*)ih->handle;
	}
	return nil;
}

/* previous "on" toggle is parked on the IupRadio parent for O(1) deactivation */
static void cocoaTouchToggleDeactivatePrevious(Ihandle* ih)
{
	Ihandle* radio = iupRadioFindToggleParent(ih);
	if (!radio) return;
	Ihandle* previous = (Ihandle*)iupAttribGet(radio, "_IUPCOCOATOUCH_RADIO_ACTIVE");
	if (previous && previous != ih && iupObjectCheck(previous))
	{
		IupSetAttribute(previous, "VALUE", "OFF");
	}
	iupAttribSet(radio, "_IUPCOCOATOUCH_RADIO_ACTIVE", (char*)ih);
}

static IupCocoaTouchToggleValue cocoaTouchToggleReadValue(Ihandle* ih)
{
	IupCocoaTouchToggleButton* button = cocoaTouchToggleGetButton(ih);
	if (button) return button.state_value;
	UISwitch* sw = cocoaTouchToggleGetSwitch(ih);
	if (sw) return [sw isOn] ? IupCocoaTouchToggleOn : IupCocoaTouchToggleOff;
	UIButton* img_btn = cocoaTouchToggleGetImageButton(ih);
	if (img_btn) return [img_btn isSelected] ? IupCocoaTouchToggleOn : IupCocoaTouchToggleOff;
	return IupCocoaTouchToggleOff;
}

static void cocoaTouchToggleLoadImages(Ihandle* ih, UIButton* button)
{
	const char* image    = iupAttribGet(ih, "IMAGE");
	const char* inactive = iupAttribGet(ih, "IMINACTIVE");
	const char* impress  = iupAttribGet(ih, "IMPRESS");

	UIImage* normal_img   = image    ? (UIImage*)iupImageGetImage(image,    ih, 0, NULL) : nil;
	UIImage* impress_img  = impress  ? (UIImage*)iupImageGetImage(impress,  ih, 0, NULL) : nil;
	UIImage* inactive_img = inactive ? (UIImage*)iupImageGetImage(inactive, ih, 0, NULL)
	                       : (image  ? (UIImage*)iupImageGetImage(image,    ih, 1, NULL) : nil);

	UIButtonConfiguration* cfg = [button.configuration copy];
	cfg.image = normal_img;
	button.configuration = cfg;
	[cfg release];

	button.configurationUpdateHandler = ^(UIButton* b) {
		UIButtonConfiguration* updated = [b.configuration copy];
		if ((b.state & UIControlStateDisabled) && inactive_img)
			updated.image = inactive_img;
		else if ((b.state & (UIControlStateSelected | UIControlStateHighlighted)) && impress_img)
			updated.image = impress_img;
		else
			updated.image = normal_img;
		b.configuration = updated;
		[updated release];
	};
}

static void cocoaTouchToggleApplyValue(Ihandle* ih, IupCocoaTouchToggleValue new_value)
{
	IupCocoaTouchToggleButton* button = cocoaTouchToggleGetButton(ih);
	if (button)
	{
		button.state_value = new_value;
		[button refreshIndicator];
		/* VoiceOver reads "selected" trait after the label */
		UIAccessibilityTraits t = button.accessibilityTraits;
		if (new_value == IupCocoaTouchToggleOn) t |=  UIAccessibilityTraitSelected;
		else                                     t &= ~UIAccessibilityTraitSelected;
		button.accessibilityTraits = t;
	}
	UISwitch* sw = cocoaTouchToggleGetSwitch(ih);
	if (sw)
	{
		[sw setOn:(new_value == IupCocoaTouchToggleOn) animated:NO];
	}
	UIButton* img_btn = cocoaTouchToggleGetImageButton(ih);
	if (img_btn)
	{
		[img_btn setSelected:(new_value == IupCocoaTouchToggleOn)];
		if (ih->data->flat)
			img_btn.layer.borderWidth = (new_value == IupCocoaTouchToggleOn) ? 1.0 : 0.0;
	}
}

/* fires ACTION + VALUECHANGED_CB, plus radio-sibling deactivate on turn-on */
static void cocoaTouchToggleFireCallbacks(Ihandle* ih, IupCocoaTouchToggleValue new_value)
{
	int state = (new_value == IupCocoaTouchToggleOn) ? 1 : 0;

	if (new_value == IupCocoaTouchToggleOn && ih->data->is_radio
	    && !iupAttribGetBoolean(ih, "IGNORERADIO"))
	{
		cocoaTouchToggleDeactivatePrevious(ih);
	}

	IFni action = (IFni)IupGetCallback(ih, "ACTION");
	if (action && action(ih, state) == IUP_CLOSE)
	{
		IupExitLoop();
	}
	IFn changed = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
	if (changed && changed(ih) == IUP_CLOSE)
	{
		IupExitLoop();
	}
}


@implementation IupCocoaTouchToggleTarget

- (void)onSwitchChanged:(id)sender
{
	UISwitch* sw = (UISwitch*)sender;
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(sender, IHANDLE_ASSOCIATED_OBJ_KEY);
	if (!ih || !iupObjectCheck(ih)) return;
	IupCocoaTouchToggleValue new_value = [sw isOn] ? IupCocoaTouchToggleOn : IupCocoaTouchToggleOff;
	cocoaTouchToggleFireCallbacks(ih, new_value);
}

- (void)onButtonTapped:(id)sender
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(sender, IHANDLE_ASSOCIATED_OBJ_KEY);
	if (!ih || !iupObjectCheck(ih)) return;

	IupCocoaTouchToggleButton* button = cocoaTouchToggleGetButton(ih);
	UIButton* img_btn = cocoaTouchToggleGetImageButton(ih);

	IupCocoaTouchToggleValue current = cocoaTouchToggleReadValue(ih);
	IupCocoaTouchToggleValue new_value = current;

	if (button)
	{
		if (button.isRadio)
		{
			/* radio: tapping a selected one stays on, tapping another flips on */
			if (current != IupCocoaTouchToggleOn)
			{
				new_value = IupCocoaTouchToggleOn;
			}
			else
			{
				return;
			}
		}
		else if (button.is3State)
		{
			new_value = (IupCocoaTouchToggleValue)((current + 1) % 3);
		}
		else
		{
			new_value = (current == IupCocoaTouchToggleOn) ? IupCocoaTouchToggleOff : IupCocoaTouchToggleOn;
		}
	}
	else if (img_btn)
	{
		new_value = (current == IupCocoaTouchToggleOn) ? IupCocoaTouchToggleOff : IupCocoaTouchToggleOn;
	}

	cocoaTouchToggleApplyValue(ih, new_value);
	cocoaTouchToggleFireCallbacks(ih, new_value);
}

@end



IUP_SDK_API void iupdrvToggleAddBorders(Ihandle* ih, int* x, int* y)
{
	(void)ih;
	/* HIG minimum tappable is 44pt; pad image toggles to that */
	if (x) *x += 28;
	if (y) *y += 28;
}

IUP_SDK_API void iupdrvToggleAddCheckBox(Ihandle* ih, int* x, int* y, const char* str)
{
	(void)ih; (void)str;
	if (x) *x += cocoaTouchToggleCheckOverhead();
	if (y) *y += 4;
}

IUP_SDK_API void iupdrvToggleAddSwitch(Ihandle* ih, int* x, int* y, const char* str)
{
	(void)ih; (void)str;
	int sw_w = 0, sw_h = 0;
	cocoaTouchToggleSwitchSize(&sw_w, &sw_h);
	if (x) *x += sw_w;
	if (y && *y < sw_h) *y = sw_h;
}



static int cocoaTouchToggleSetValueAttrib(Ihandle* ih, const char* value)
{
	BOOL three_state = iupAttribGetBoolean(ih, "3STATE") ? YES : NO;
	IupCocoaTouchToggleValue current = cocoaTouchToggleReadValue(ih);
	IupCocoaTouchToggleValue new_value = cocoaTouchToggleValueFromStr(value, current, three_state);
	cocoaTouchToggleApplyValue(ih, new_value);

	/* turning a radio ON via VALUE also flips its siblings off */
	if (new_value == IupCocoaTouchToggleOn && ih->data->is_radio
	    && !iupAttribGetBoolean(ih, "IGNORERADIO"))
	{
		cocoaTouchToggleDeactivatePrevious(ih);
	}
	return 0;
}

static char* cocoaTouchToggleGetValueAttrib(Ihandle* ih)
{
	return iupStrReturnStr(cocoaTouchToggleValueToStr(cocoaTouchToggleReadValue(ih)));
}

static int cocoaTouchToggleSetTitleAttrib(Ihandle* ih, const char* value)
{
	UISwitch* sw = cocoaTouchToggleGetSwitch(ih);
	if (sw) return 1;
	UIButton* img_btn = cocoaTouchToggleGetImageButton(ih);
	if (img_btn) return 1;

	IupCocoaTouchToggleButton* button = cocoaTouchToggleGetButton(ih);
	if (!button) return 0;

	const char* src = value ? value : "";
	int has_markup = iupAttribGetBoolean(ih, "MARKUP");
	/* MARKUP=YES: don't strip mnemonics so '&' inside attribute values survive. */
	char* stripped = (src && !has_markup) ? iupStrProcessMnemonic(src, NULL, 0) : NULL;
	const char* display = stripped ? stripped : src;

	IupCocoaTouchFont* iup_font = iupCocoaTouchGetFont(ih);
	UIFont* base_font = (iup_font && iup_font.nativeFont)
	    ? iup_font.nativeFont
	    : [UIFont systemFontOfSize:UIFont.buttonFontSize];

	NSAttributedString* attributed = nil;
	if (has_markup)
		attributed = iupCocoaTouchParseMarkup(display, base_font, nil);
	if (!attributed)
	{
		NSString* ns = [NSString stringWithUTF8String:display ?: ""];
		attributed = [[[NSAttributedString alloc] initWithString:ns ?: @"" attributes:@{ NSFontAttributeName: base_font }] autorelease];
	}

	if (stripped && stripped != src) free(stripped);

	/* Single-shot config matching iupcocoatouch_button.m: copying-and-reassigning UIButtonConfiguration drops attributedTitle on iOS 16+. */
	UIButtonConfiguration* cfg = cocoaTouchToggleMakeBaseConfig();
	cfg.image = [button indicatorImage];
	cfg.baseForegroundColor = button.configuration.baseForegroundColor ?: [UIColor labelColor];
	cfg.attributedTitle = attributed;
	button.configuration = cfg;
	return 1;
}

static int cocoaTouchToggleSetImageAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	UIButton* img_btn = cocoaTouchToggleGetImageButton(ih);
	if (!img_btn) return 0;
	cocoaTouchToggleLoadImages(ih, img_btn);
	return 1;
}

static int cocoaTouchToggleSetAlignmentAttrib(Ihandle* ih, const char* value)
{
	if (ih->data->type != IUP_TOGGLE_IMAGE)
		return 0;

	char h[30], v[30];
	iupStrToStrStr(value, h, sizeof(h), v, sizeof(v), ':');

	UIControlContentHorizontalAlignment ha = UIControlContentHorizontalAlignmentCenter;
	UIControlContentVerticalAlignment   va = UIControlContentVerticalAlignmentCenter;
	if (iupStrEqualNoCase(h, "ALEFT"))       ha = UIControlContentHorizontalAlignmentLeft;
	else if (iupStrEqualNoCase(h, "ARIGHT")) ha = UIControlContentHorizontalAlignmentRight;
	if (iupStrEqualNoCase(v, "ATOP"))        va = UIControlContentVerticalAlignmentTop;
	else if (iupStrEqualNoCase(v, "ABOTTOM")) va = UIControlContentVerticalAlignmentBottom;

	UIButton* img_btn = cocoaTouchToggleGetImageButton(ih);
	if (img_btn)
	{
		img_btn.contentHorizontalAlignment = ha;
		img_btn.contentVerticalAlignment = va;
		return 1;
	}
	IupCocoaTouchToggleButton* button = cocoaTouchToggleGetButton(ih);
	if (button)
	{
		button.contentHorizontalAlignment = ha;
		button.contentVerticalAlignment = va;
		return 1;
	}
	return 1;
}

static int cocoaTouchToggleSetRightButtonAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchToggleButton* button = cocoaTouchToggleGetButton(ih);
	if (!button) return 1;
	/* flip semantic direction so the indicator moves to trailing, title stays leading */
	button.semanticContentAttribute = iupStrBoolean(value)
		? UISemanticContentAttributeForceRightToLeft
		: UISemanticContentAttributeForceLeftToRight;
	return 1;
}

static int cocoaTouchToggleSetFontAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchFont* font = value ? iupCocoaTouchFindFont(value) : nil;
	if (!font || ![font nativeFont]) return 0;

	IupCocoaTouchToggleButton* button = cocoaTouchToggleGetButton(ih);
	if (button) { [button.titleLabel setFont:[font nativeFont]]; return 1; }
	UIButton* img_btn = cocoaTouchToggleGetImageButton(ih);
	if (img_btn) { [img_btn.titleLabel setFont:[font nativeFont]]; return 1; }
	return 0;
}

static int cocoaTouchToggleSetFgColorAttrib(Ihandle* ih, const char* value)
{
	UIColor* color = iupCocoaTouchToNativeColor(value);
	if (!color) return 0;

	IupCocoaTouchToggleButton* button = cocoaTouchToggleGetButton(ih);
	if (button)
	{
		/* UIButtonConfiguration owns the title color via baseForegroundColor; setTitleColor:forState: is ignored. tintColor is what refreshIndicator bakes into the SF-symbol image. */
		UIButtonConfiguration* cfg = [button.configuration copy];
		cfg.baseForegroundColor = color;
		button.configuration = cfg;
		[cfg release];
		[button setTintColor:color];
		[button refreshIndicator];
		return 1;
	}
	UIButton* img_btn = cocoaTouchToggleGetImageButton(ih);
	if (img_btn)
	{
		[img_btn setTitleColor:color forState:UIControlStateNormal];
		return 1;
	}
	UISwitch* sw = cocoaTouchToggleGetSwitch(ih);
	if (sw)
	{
		[sw setOnTintColor:color];
		return 1;
	}
	return 0;
}

static int cocoaTouchToggleSetBgColorAttrib(Ihandle* ih, const char* value)
{
	UIColor* color = iupCocoaTouchToNativeColor(value);
	if (!color || ![(id)ih->handle isKindOfClass:[UIView class]]) return 0;
	[(UIView*)ih->handle setBackgroundColor:color];
	return 1;
}

static int cocoaTouchToggleSetPaddingAttrib(Ihandle* ih, const char* value)
{
	if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
	{
		value = IupGetGlobal("DEFAULTBUTTONPADDING");
	}
	iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');

	UIButton* any_btn = cocoaTouchToggleGetButton(ih);
	if (!any_btn) any_btn = cocoaTouchToggleGetImageButton(ih);
	if (any_btn)
	{
		UIButtonConfiguration* cfg = [any_btn.configuration copy];
		if (cfg)
		{
			cfg.contentInsets = NSDirectionalEdgeInsetsMake(
			    ih->data->vert_padding, ih->data->horiz_padding,
			    ih->data->vert_padding, ih->data->horiz_padding);
			any_btn.configuration = cfg;
			[cfg release];
		}
	}
	return 0;
}



static int cocoaTouchToggleMapMethod(Ihandle* ih)
{
	Ihandle* radio = iupRadioFindToggleParent(ih);
	ih->data->is_radio = radio ? 1 : 0;
	if (radio && !iupAttribGet(radio, "_IUPCOCOATOUCH_RADIO_FIRST_SEEN"))
	{
		iupAttribSet(radio, "_IUPCOCOATOUCH_RADIO_FIRST_SEEN", "1");
		if (!iupAttribGet(ih, "VALUE"))
			iupAttribSet(ih, "VALUE", "ON");
	}

	BOOL is_switch = iupAttribGetBoolean(ih, "SWITCH") ? YES : NO;
	const char* image_name = iupAttribGet(ih, "IMAGE");
	BOOL has_image = (image_name != NULL && *image_name);

	ih->data->type = has_image ? IUP_TOGGLE_IMAGE : IUP_TOGGLE_TEXT;

	IupCocoaTouchToggleTarget* target = [[IupCocoaTouchToggleTarget alloc] init];
	UIView* view;

	if (is_switch && !has_image && !ih->data->is_radio)
	{
		UISwitch* sw = [[UISwitch alloc] initWithFrame:CGRectZero];
		[sw addTarget:target action:@selector(onSwitchChanged:) forControlEvents:UIControlEventValueChanged];
		view = sw;
	}
	else if (has_image)
	{
		/* plain config avoids the tint-fill from filled/tinted styles */
		UIButtonConfiguration* cfg = [UIButtonConfiguration plainButtonConfiguration];
		UIButton* button = [UIButton buttonWithConfiguration:cfg primaryAction:nil];
		cocoaTouchToggleLoadImages(ih, button);
		button.layer.cornerRadius = 6.0;
		button.layer.borderColor = [[UIColor systemBlueColor] CGColor];
		/* flat: no border until checked (touch has no hover) */
		button.layer.borderWidth = ih->data->flat ? 0.0 : 1.0;
		[button addTarget:target action:@selector(onButtonTapped:) forControlEvents:UIControlEventTouchUpInside];
		view = button;
	}
	else
	{
		IupCocoaTouchToggleButton* tb = [[IupCocoaTouchToggleButton alloc] initWithIhandle:ih];
		tb.isRadio = ih->data->is_radio ? YES : NO;
		tb.is3State = (iupAttribGetBoolean(ih, "3STATE") && !tb.isRadio) ? YES : NO;

		/* TITLE is replayed through cocoaTouchToggleSetTitleAttrib post-map; that path also handles MARKUP=YES. Setting cfg.title here would write the raw markup string into the configuration first and could survive across the later attributedTitle assignment. */

		IupCocoaTouchFont* font = iupCocoaTouchGetFont(ih);
		if (font && [font nativeFont])
		{
			[tb.titleLabel setFont:[font nativeFont]];
		}

		[tb setTitleColor:[UIColor labelColor] forState:UIControlStateNormal];
		[tb setTintColor:[UIColor systemBlueColor]];

		[tb refreshIndicator];
		[tb addTarget:target action:@selector(onButtonTapped:) forControlEvents:UIControlEventTouchUpInside];
		view = tb;
	}

	ih->handle = view;
	objc_setAssociatedObject(view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
	objc_setAssociatedObject(view, IUP_COCOATOUCH_TOGGLE_TARGET_OBJ_KEY, target, OBJC_ASSOCIATION_RETAIN);
	[target release];

	iupCocoaTouchAddToParent(ih);

	if (!iupAttribGetBoolean(ih, "ACTIVE"))
	{
		if ([view respondsToSelector:@selector(setEnabled:)])
		{
			[(id)view setEnabled:NO];
		}
	}

	return IUP_NOERROR;
}

static void cocoaTouchToggleUnMapMethod(Ihandle* ih)
{
	if (ih && ih->handle)
	{
		if ([(id)ih->handle isKindOfClass:[UIControl class]])
		{
			[(UIControl*)ih->handle removeTarget:nil action:NULL forControlEvents:UIControlEventAllEvents];
		}
		objc_setAssociatedObject((id)ih->handle, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject((id)ih->handle, IUP_COCOATOUCH_TOGGLE_TARGET_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

		/* Drop radio-active pointer if this toggle owned it. */
		Ihandle* radio = iupRadioFindToggleParent(ih);
		if (radio && iupAttribGet(radio, "_IUPCOCOATOUCH_RADIO_ACTIVE") == (char*)ih)
		{
			iupAttribSet(radio, "_IUPCOCOATOUCH_RADIO_ACTIVE", NULL);
		}
	}
	iupdrvBaseUnMapMethod(ih);
}



IUP_SDK_API void iupdrvToggleInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchToggleMapMethod;
	ic->UnMap = cocoaTouchToggleUnMapMethod;

	iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, iupBaseSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTouchToggleSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTouchToggleSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FONT", NULL, cocoaTouchToggleSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

	iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaTouchToggleSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "VALUE", cocoaTouchToggleGetValueAttrib, cocoaTouchToggleSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "PADDING", iupToggleGetPaddingAttrib, cocoaTouchToggleSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

	iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaTouchToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, cocoaTouchToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMPRESS", NULL, cocoaTouchToggleSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	/* MARKUP=YES wires a Pango subset: b/i/u/s + <span foreground="#..."> */
	iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

	/* 3STATE / SWITCH are creation-only; consulted at Map */
	iupClassRegisterAttribute(ic, "3STATE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SWITCH", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IGNORERADIO", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "RIGHTBUTTON", NULL, cocoaTouchToggleSetRightButtonAttrib, NULL, NULL, IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, cocoaTouchToggleSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
}
