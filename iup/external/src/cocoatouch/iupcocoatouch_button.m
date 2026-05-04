/** \file
 * \brief Button Control (iOS UIKit, UIButton.Configuration)
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
#include "iup_image.h"
#include "iup_button.h"

#include "iupcocoatouch_drv.h"


@interface IupCocoaTouchButton : UIButton
@property(nonatomic, assign) Ihandle* ihandle;
/* drops a duplicate touchesBegan that iOS re-delivers during a modal pump */
@property(nonatomic, assign) UITouch* activeTouch;
/* re-entry guard for nested-run-loop pumps from inside ACTION CB */
@property(nonatomic, assign) BOOL clickInFlight;
@end

@implementation IupCocoaTouchButton

- (instancetype)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self)
	{
		self.exclusiveTouch = YES;
	}
	return self;
}

- (void)fireButtonCallback:(NSSet<UITouch*>*)touches event:(UIEvent*)event pressed:(int)pressed
{
	if (!_ihandle) return;
	IFniiiis cb = (IFniiiis)IupGetCallback(_ihandle, "BUTTON_CB");
	if (!cb) return;
	UITouch* touch = [touches anyObject];
	CGPoint p = [touch locationInView:self];
	UIKeyModifierFlags mods = event ? [event modifierFlags] : 0;
	char status[IUPKEY_STATUS_SIZE];
	iupCocoaTouchButtonKeySetStatus(event, mods, pressed ? 1 : 0, 0, status);
	if (cb(_ihandle, IUP_BUTTON1, pressed, (int)p.x, (int)p.y, status) == IUP_CLOSE)
		IupExitLoop();
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesBegan:touches withEvent:event];
	if (_activeTouch) return;
	_activeTouch = [touches anyObject];
	[self fireButtonCallback:touches event:event pressed:1];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesEnded:touches withEvent:event];
	if (!_activeTouch || ![touches containsObject:_activeTouch]) return;
	_activeTouch = nil;
	[self fireButtonCallback:touches event:event pressed:0];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesCancelled:touches withEvent:event];
	if (!_activeTouch || ![touches containsObject:_activeTouch]) return;
	_activeTouch = nil;
	[self fireButtonCallback:touches event:event pressed:0];
}

@end


static const void* IUP_COCOATOUCH_BUTTON_RECEIVER_OBJ_KEY = "IUP_COCOATOUCH_BUTTON_RECEIVER_OBJ_KEY";

@interface IupCocoaTouchButtonReceiver : NSObject
- (IBAction)myButtonClickAction:(id)sender;
@end

@implementation IupCocoaTouchButtonReceiver
- (IBAction)myButtonClickAction:(id)sender
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(sender, IHANDLE_ASSOCIATED_OBJ_KEY);
	if (!ih || !iupObjectCheck(ih)) return;
	IupCocoaTouchButton* btn = [sender isKindOfClass:[IupCocoaTouchButton class]] ? (IupCocoaTouchButton*)sender : nil;
	if (btn.clickInFlight) return;
	btn.clickInFlight = YES;
	Icallback cb = IupGetCallback(ih, "ACTION");
	int ret = cb ? cb(ih) : IUP_DEFAULT;
	btn.clickInFlight = NO;
	if (ret == IUP_CLOSE) IupExitLoop();
}
@end


static IupCocoaTouchButton* cocoaTouchButtonGet(Ihandle* ih)
{
	if ([(id)ih->handle isKindOfClass:[IupCocoaTouchButton class]])
		return (IupCocoaTouchButton*)ih->handle;
	return nil;
}

static int cocoaTouchButtonImagePositionFromStr(const char* value)
{
	if (!value) return IUP_IMGPOS_LEFT;
	if (iupStrEqualNoCase(value, "RIGHT"))  return IUP_IMGPOS_RIGHT;
	if (iupStrEqualNoCase(value, "TOP"))    return IUP_IMGPOS_TOP;
	if (iupStrEqualNoCase(value, "BOTTOM")) return IUP_IMGPOS_BOTTOM;
	return IUP_IMGPOS_LEFT;
}

static NSDirectionalRectEdge cocoaTouchButtonImageEdge(int img_pos)
{
	switch (img_pos)
	{
		case IUP_IMGPOS_RIGHT:  return NSDirectionalRectEdgeTrailing;
		case IUP_IMGPOS_TOP:    return NSDirectionalRectEdgeTop;
		case IUP_IMGPOS_BOTTOM: return NSDirectionalRectEdgeBottom;
		default:                return NSDirectionalRectEdgeLeading;
	}
}

static UIControlContentHorizontalAlignment cocoaTouchButtonHAlign(int horiz)
{
	switch (horiz)
	{
		case IUP_ALIGN_ALEFT:  return UIControlContentHorizontalAlignmentLeft;
		case IUP_ALIGN_ARIGHT: return UIControlContentHorizontalAlignmentRight;
		default:               return UIControlContentHorizontalAlignmentCenter;
	}
}

static UIControlContentVerticalAlignment cocoaTouchButtonVAlign(int vert)
{
	switch (vert)
	{
		case IUP_ALIGN_ATOP:    return UIControlContentVerticalAlignmentTop;
		case IUP_ALIGN_ABOTTOM: return UIControlContentVerticalAlignmentBottom;
		default:                return UIControlContentVerticalAlignmentCenter;
	}
}

static UIImage* cocoaTouchButtonLoadImage(Ihandle* ih, const char* name, int make_inactive)
{
	if (!name) return nil;
	return (UIImage*)iupImageGetImage(name, ih, make_inactive, NULL);
}

static NSAttributedString* cocoaTouchButtonAttributedTitle(Ihandle* ih, IupCocoaTouchButton* button, const char* value)
{
	if (!value || !*value) return nil;
	char* stripped = iupStrProcessMnemonic(value, NULL, 0);
	const char* display = stripped ? stripped : value;
	NSString* ns = [NSString stringWithUTF8String:display];

	IupCocoaTouchFont* iup_font = iupCocoaTouchGetFont(ih);
	UIFont* font = (iup_font && iup_font.nativeFont) ? iup_font.nativeFont : [UIFont systemFontOfSize:UIFont.buttonFontSize];

	NSAttributedString* result = nil;
	if (iupAttribGetBoolean(ih, "MARKUP"))
		result = iupCocoaTouchParseMarkup(display, font, nil);
	if (!result)
		result = [[[NSAttributedString alloc] initWithString:ns attributes:@{ NSFontAttributeName: font }] autorelease];

	(void)button;
	if (stripped && stripped != value) free(stripped);
	return result;
}

static UIButtonConfiguration* cocoaTouchButtonMakeConfig(Ihandle* ih)
{
	BOOL flat          = iupAttribGetBoolean(ih, "FLAT");
	BOOL show_default  = iupAttribGetBoolean(ih, "SHOWASDEFAULT");
	BOOL borderless    = (ih->data->type & IUP_BUTTON_IMAGE) &&
	                     iupAttribGet(ih, "IMPRESS") &&
	                     !iupAttribGetBoolean(ih, "IMPRESSBORDER");
	const char* style  = iupAttribGet(ih, "BUTTONSTYLE");

	UIButtonConfiguration* cfg;
	if (style)
	{
		if      (iupStrEqualNoCase(style, "TONAL"))    cfg = [UIButtonConfiguration tintedButtonConfiguration];
		else if (iupStrEqualNoCase(style, "OUTLINED")) cfg = [UIButtonConfiguration borderedButtonConfiguration];
		else if (iupStrEqualNoCase(style, "ELEVATED")) cfg = [UIButtonConfiguration grayButtonConfiguration];
		else if (iupStrEqualNoCase(style, "TEXT"))     cfg = [UIButtonConfiguration plainButtonConfiguration];
		else                                           cfg = [UIButtonConfiguration filledButtonConfiguration];
	}
	else if (show_default)
		cfg = [UIButtonConfiguration filledButtonConfiguration];
	else if (flat || borderless)
		cfg = [UIButtonConfiguration plainButtonConfiguration];
	else
		cfg = [UIButtonConfiguration tintedButtonConfiguration];

	const char* corner = iupAttribGet(ih, "CORNERSTYLE");
	if      (iupStrEqualNoCase(corner, "SMALL"))   cfg.cornerStyle = UIButtonConfigurationCornerStyleSmall;
	else if (iupStrEqualNoCase(corner, "MEDIUM"))  cfg.cornerStyle = UIButtonConfigurationCornerStyleMedium;
	else if (iupStrEqualNoCase(corner, "LARGE"))   cfg.cornerStyle = UIButtonConfigurationCornerStyleLarge;
	else if (iupStrEqualNoCase(corner, "CAPSULE")) cfg.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
	cfg.imagePlacement = cocoaTouchButtonImageEdge(ih->data->img_position);
	cfg.imagePadding   = ih->data->spacing > 0 ? ih->data->spacing : 4;
	cfg.titleLineBreakMode = NSLineBreakByTruncatingTail;

	/* contentInsets adds onto styling's hidden defaults; zero them so PADDING is literal */
	if (borderless)
		cfg.contentInsets = NSDirectionalEdgeInsetsMake(-7, -12, -7, -12);

	if (iupAttribGet(ih, "PADDING"))
	{
		int dh = (flat || borderless) ? 12 : 14;
		int dv = 7;
		cfg.contentInsets = NSDirectionalEdgeInsetsMake(
		    ih->data->vert_padding - dv, ih->data->horiz_padding - dh,
		    ih->data->vert_padding - dv, ih->data->horiz_padding - dh);
	}

	switch (ih->data->horiz_alignment)
	{
		case IUP_ALIGN_ALEFT:  cfg.titleAlignment = UIButtonConfigurationTitleAlignmentLeading;  break;
		case IUP_ALIGN_ARIGHT: cfg.titleAlignment = UIButtonConfigurationTitleAlignmentTrailing; break;
		default:               cfg.titleAlignment = UIButtonConfigurationTitleAlignmentCenter;   break;
	}

	const char* fg = iupAttribGet(ih, "FGCOLOR");
	if (fg)
	{
		UIColor* c = iupCocoaTouchToNativeColor(fg);
		if (c) cfg.baseForegroundColor = c;
	}
	const char* bg = iupAttribGet(ih, "BGCOLOR");
	if (bg)
	{
		UIColor* c = iupCocoaTouchToNativeColor(bg);
		if (c) cfg.baseBackgroundColor = c;
	}

	return cfg;
}

static void cocoaTouchButtonRefresh(Ihandle* ih)
{
	IupCocoaTouchButton* button = cocoaTouchButtonGet(ih);
	if (!button) return;

	UIButtonConfiguration* cfg = cocoaTouchButtonMakeConfig(ih);

	if (ih->data->type & IUP_BUTTON_TEXT)
	{
		const char* title = iupAttribGet(ih, "TITLE");
		cfg.attributedTitle = cocoaTouchButtonAttributedTitle(ih, button, title);
	}

	if (ih->data->type & IUP_BUTTON_IMAGE)
	{
		const char* img = iupAttribGet(ih, "IMAGE");
		if (img) cfg.image = cocoaTouchButtonLoadImage(ih, img, 0);
	}

	button.configuration = cfg;

	[button setContentHorizontalAlignment:cocoaTouchButtonHAlign(ih->data->horiz_alignment)];
	[button setContentVerticalAlignment:cocoaTouchButtonVAlign(ih->data->vert_alignment)];

	const char* impress  = iupAttribGet(ih, "IMPRESS");
	const char* inactive = iupAttribGet(ih, "IMINACTIVE");
	UIImage* impress_img  = impress ? cocoaTouchButtonLoadImage(ih, impress, 0) : nil;
	UIImage* inactive_img = inactive ? cocoaTouchButtonLoadImage(ih, inactive, 0)
	                       : ((ih->data->type & IUP_BUTTON_IMAGE) && iupAttribGet(ih, "IMAGE")
	                          ? cocoaTouchButtonLoadImage(ih, iupAttribGet(ih, "IMAGE"), 1) : nil);
	UIImage* normal_img   = (ih->data->type & IUP_BUTTON_IMAGE) ? cocoaTouchButtonLoadImage(ih, iupAttribGet(ih, "IMAGE"), 0) : nil;

	button.configurationUpdateHandler = ^(UIButton* b) {
		UIButtonConfiguration* updated = [b.configuration copy];
		if (b.state & UIControlStateDisabled && inactive_img)
			updated.image = inactive_img;
		else if (b.state & UIControlStateHighlighted && impress_img)
			updated.image = impress_img;
		else if (normal_img)
			updated.image = normal_img;
		b.configuration = updated;
		[updated release];
	};
}

IUP_SDK_API void iupdrvButtonAddBorders(Ihandle* ih, int* x, int* y)
{
	/* PADDING overrides native insets; reserve them only if PADDING is unset */
	if (iupAttribGet(ih, "PADDING")) return;

	BOOL flat = iupAttribGetBoolean(ih, "FLAT");
	int hpad = flat ? 12 : 14;
	int vpad = 7;
	if (x) *x += 2 * hpad;
	if (y) *y += 2 * vpad;
}

static int cocoaTouchButtonSetTitleAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchButton* button = cocoaTouchButtonGet(ih);
	if (!button || !(ih->data->type & IUP_BUTTON_TEXT)) return 1;
	UIButtonConfiguration* cfg = cocoaTouchButtonMakeConfig(ih);
	cfg.attributedTitle = cocoaTouchButtonAttributedTitle(ih, button, value);
	if (ih->data->type & IUP_BUTTON_IMAGE)
	{
		const char* img = iupAttribGet(ih, "IMAGE");
		if (img) cfg.image = cocoaTouchButtonLoadImage(ih, img, 0);
	}
	button.configuration = cfg;
	return 1;
}

static int cocoaTouchButtonSetFontAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	cocoaTouchButtonRefresh(ih);
	return 1;
}

static int cocoaTouchButtonSetAlignmentAttrib(Ihandle* ih, const char* value)
{
	char h[30], v[30];
	iupStrToStrStr(value, h, sizeof(h), v, sizeof(v), ':');

	int horiz = IUP_ALIGN_ACENTER;
	int vert  = IUP_ALIGN_ACENTER;
	if (iupStrEqualNoCase(h, "ALEFT"))        horiz = IUP_ALIGN_ALEFT;
	else if (iupStrEqualNoCase(h, "ARIGHT"))  horiz = IUP_ALIGN_ARIGHT;
	if (iupStrEqualNoCase(v, "ATOP"))         vert = IUP_ALIGN_ATOP;
	else if (iupStrEqualNoCase(v, "ABOTTOM")) vert = IUP_ALIGN_ABOTTOM;

	ih->data->horiz_alignment = horiz;
	ih->data->vert_alignment  = vert;
	cocoaTouchButtonRefresh(ih);
	return 1;
}

static char* cocoaTouchButtonGetAlignmentAttrib(Ihandle* ih)
{
	const char* h[3] = { "ALEFT", "ACENTER", "ARIGHT" };
	const char* v[3] = { "ATOP",  "ACENTER", "ABOTTOM" };
	int horiz = ih->data->horiz_alignment;
	int vert  = ih->data->vert_alignment;
	if (horiz < IUP_ALIGN_ALEFT || horiz > IUP_ALIGN_ARIGHT)  horiz = IUP_ALIGN_ACENTER;
	if (vert  < IUP_ALIGN_ATOP  || vert  > IUP_ALIGN_ABOTTOM) vert  = IUP_ALIGN_ACENTER;
	return iupStrReturnStrf("%s:%s", h[horiz], v[vert]);
}

static int cocoaTouchButtonSetPaddingAttrib(Ihandle* ih, const char* value)
{
	if (iupStrEqual(value, "DEFAULTBUTTONPADDING"))
		value = IupGetGlobal("DEFAULTBUTTONPADDING");
	iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
	cocoaTouchButtonRefresh(ih);
	return 0;
}

static int cocoaTouchButtonSetSpacingAttrib(Ihandle* ih, const char* value)
{
	int spacing = 2;
	iupStrToInt(value, &spacing);
	ih->data->spacing = spacing;
	cocoaTouchButtonRefresh(ih);
	return 0;
}

static int cocoaTouchButtonSetImagePositionAttrib(Ihandle* ih, const char* value)
{
	ih->data->img_position = cocoaTouchButtonImagePositionFromStr(value);
	cocoaTouchButtonRefresh(ih);
	return 1;
}

static int cocoaTouchButtonSetFgColorAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	cocoaTouchButtonRefresh(ih);
	return 1;
}

static int cocoaTouchButtonSetBgColorAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	cocoaTouchButtonRefresh(ih);
	return 1;
}

static int cocoaTouchButtonSetButtonStyleAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "BUTTONSTYLE", value);
	cocoaTouchButtonRefresh(ih);
	return 0;
}

static int cocoaTouchButtonSetCornerStyleAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "CORNERSTYLE", value);
	cocoaTouchButtonRefresh(ih);
	return 0;
}

static int cocoaTouchButtonSetImageAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	if (!(ih->data->type & IUP_BUTTON_IMAGE)) return 0;
	cocoaTouchButtonRefresh(ih);
	return 1;
}

static int cocoaTouchButtonSetImInactiveAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	if (!(ih->data->type & IUP_BUTTON_IMAGE)) return 0;
	cocoaTouchButtonRefresh(ih);
	return 1;
}

static int cocoaTouchButtonSetImpressAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	if (!(ih->data->type & IUP_BUTTON_IMAGE)) return 0;
	cocoaTouchButtonRefresh(ih);
	return 1;
}

static int cocoaTouchButtonSetActiveAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchButton* button = cocoaTouchButtonGet(ih);
	if (button) [button setEnabled:iupStrBoolean(value) ? YES : NO];
	return iupBaseSetActiveAttrib(ih, value);
}

static int cocoaTouchButtonSetShowAsDefaultAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	cocoaTouchButtonRefresh(ih);
	return 1;
}

static int cocoaTouchButtonMapMethod(Ihandle* ih)
{
	IupCocoaTouchButton* button = [[IupCocoaTouchButton alloc] initWithFrame:CGRectZero];
	[button setIhandle:ih];

	char* image_name = iupAttribGet(ih, "IMAGE");
	char* title      = iupAttribGet(ih, "TITLE");
	BOOL has_image   = (image_name != NULL);
	BOOL has_text    = (title != NULL && *title != 0);

	ih->data->type = 0;
	if (has_image) ih->data->type |= IUP_BUTTON_IMAGE;
	if (has_text)  ih->data->type |= IUP_BUTTON_TEXT;
	if (ih->data->type == 0) ih->data->type = IUP_BUTTON_TEXT;

	if (ih->data->spacing <= 0) ih->data->spacing = 2;

	ih->handle = button;
	objc_setAssociatedObject(button, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

	cocoaTouchButtonRefresh(ih);

	IupCocoaTouchButtonReceiver* receiver = [[IupCocoaTouchButtonReceiver alloc] init];
	[button addTarget:receiver action:@selector(myButtonClickAction:) forControlEvents:UIControlEventTouchUpInside];
	objc_setAssociatedObject(button, IUP_COCOATOUCH_BUTTON_RECEIVER_OBJ_KEY, receiver, OBJC_ASSOCIATION_RETAIN);
	[receiver release];

	iupCocoaTouchAddToParent(ih);

	if (!iupAttribGetBoolean(ih, "ACTIVE")) [button setEnabled:NO];

	return IUP_NOERROR;
}

static void cocoaTouchButtonUnMapMethod(Ihandle* ih)
{
	id handle = ih->handle;
	if ([handle isKindOfClass:[IupCocoaTouchButton class]])
	{
		IupCocoaTouchButton* button = (IupCocoaTouchButton*)handle;
		[button setIhandle:NULL];
		button.configurationUpdateHandler = nil;
		objc_setAssociatedObject(button, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(button, IUP_COCOATOUCH_BUTTON_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	}
	iupdrvBaseUnMapMethod(ih);
}

IUP_SDK_API void iupdrvButtonInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchButtonMapMethod;
	ic->UnMap = cocoaTouchButtonUnMapMethod;

	iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, cocoaTouchButtonSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTouchButtonSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTouchButtonSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FONT", NULL, cocoaTouchButtonSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

	iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaTouchButtonSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ALIGNMENT", cocoaTouchButtonGetAlignmentAttrib, cocoaTouchButtonSetAlignmentAttrib, "ACENTER:ACENTER", NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaTouchButtonSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, cocoaTouchButtonSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMPRESS", NULL, cocoaTouchButtonSetImpressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMPRESSBORDER", NULL, NULL, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGEPOSITION", NULL, cocoaTouchButtonSetImagePositionAttrib, IUPAF_SAMEASSYSTEM, "LEFT", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "PADDING", iupButtonGetPaddingAttrib, cocoaTouchButtonSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
	iupClassRegisterAttribute(ic, "SPACING", NULL, cocoaTouchButtonSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "2", IUPAF_NOT_MAPPED);
	iupClassRegisterAttribute(ic, "SHOWASDEFAULT", NULL, cocoaTouchButtonSetShowAsDefaultAttrib, NULL, NULL, IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "FLAT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "BUTTONSTYLE", NULL, cocoaTouchButtonSetButtonStyleAttrib, IUPAF_SAMEASSYSTEM, "FILLED", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CORNERSTYLE", NULL, cocoaTouchButtonSetCornerStyleAttrib, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_NO_DEFAULTVALUE);
	iupClassRegisterAttribute(ic, "CANFOCUS", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
}
