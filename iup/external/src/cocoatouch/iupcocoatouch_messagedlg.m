/** \file
 * \brief IupMessageDlg (iOS UIKit)
 *
 * UIAlertController lifecycle (presentation + dismiss + action handler) does
 * not progress reliably inside a nested CFRunLoop pump, which is what the
 * synchronous IupPopup contract requires. We render an alert ourselves out
 * of plain UIView + UIButton so the touch + action chain fires through the
 * standard UIControl mechanism, which DOES survive nested loops.
 *
 * Look uses system-provided values (UIBlurEffect.systemMaterial, UIFont
 * preferredFont, UIColor.label/separator/systemBlue) so it adapts to dark
 * mode and Dynamic Type automatically.
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <CoreFoundation/CoreFoundation.h>

#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"

#include "iupcocoatouch_drv.h"

#import "IupViewController.h"


#define IUPCOCOATOUCH_MSGDLG_PENDING (-1)
#define IUPCOCOATOUCH_MSGDLG_HELP    (-2)

/* HIG: 270pt width, 14pt radius; rest derived from text-style lineHeight */
static const CGFloat IUP_ALERT_WIDTH         = 270.0;
static const CGFloat IUP_ALERT_CORNER_RADIUS = 14.0;
static const CGFloat IUP_ALERT_SCREEN_MARGIN = 24.0;


typedef struct
{
	NSString* title;
	int response;
	BOOL is_cancel;
} cocoaTouchMessageDlgButton;


@interface IupCocoaTouchAlertView : UIView <UIGestureRecognizerDelegate>
@property(nonatomic, copy) void (^onTap)(int response);
@property(nonatomic, retain) UIVisualEffectView* card;
@property(nonatomic, assign) int backdropResponse;
@property(nonatomic, assign) BOOL hasBackdropResponse;
@end


@implementation IupCocoaTouchAlertView

- (instancetype)initWithTitle:(NSString*)title
                      message:(NSString*)message
                    iconImage:(UIImage*)icon_image
                      buttons:(const cocoaTouchMessageDlgButton*)buttons
                        count:(int)n_buttons
                  defaultIdx:(int)default_idx
                  buttonStyle:(NSString*)button_style
                  cornerStyle:(NSString*)corner_style
                     onTap:(void (^)(int))onTap
{
	self = [super initWithFrame:UIScreen.mainScreen.bounds];
	if (!self) return nil;
	_onTap = [onTap copy];
	self.backgroundColor = [UIColor.blackColor colorWithAlphaComponent:0.4];
	self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

	/* SystemMaterial matches iOS 26 Liquid Glass UIAlertController */
	UIBlurEffect* blur = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
	UIVisualEffectView* card = [[UIVisualEffectView alloc] initWithEffect:blur];
	card.layer.cornerRadius = IUP_ALERT_CORNER_RADIUS;
	card.layer.masksToBounds = YES;
	[self addSubview:card];
	_card = [card retain];
	[card release];

	UIVibrancyEffect* vibrancy = [UIVibrancyEffect effectForBlurEffect:blur style:UIVibrancyEffectStyleLabel];
	UIVisualEffectView* vibrancyView = [[UIVisualEffectView alloc] initWithEffect:vibrancy];
	vibrancyView.frame = card.bounds;
	vibrancyView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	[card.contentView addSubview:vibrancyView];
	[vibrancyView release];

	/* lone_font fires when only title OR only message is set */
	UIFont* lone_font      = [UIFont systemFontOfSize:[UIFont buttonFontSize]];
	UIFont* title_font     = [UIFont systemFontOfSize:[UIFont buttonFontSize] weight:UIFontWeightSemibold];
	UIFont* message_font   = [UIFont systemFontOfSize:[UIFont smallSystemFontSize] + 1];
	UIFont* button_font    = [UIFont systemFontOfSize:[UIFont buttonFontSize]];
	UIFont* default_btn_font = [UIFont systemFontOfSize:[UIFont buttonFontSize] weight:UIFontWeightSemibold];
	CGFloat button_line_h  = ceil(button_font.lineHeight);

	CGFloat side_pad     = 16.0;
	CGFloat top_pad      = 16.0;
	CGFloat content_w    = IUP_ALERT_WIDTH - 2 * side_pad;
	CGFloat y            = top_pad;

	UIView* content = card.contentView;
	UIView* vibrancy_content = ((UIVisualEffectView*)[card.contentView.subviews firstObject]).contentView;

	if (icon_image)
	{
		UIImageView* iv = [[UIImageView alloc] initWithImage:icon_image];
		iv.contentMode = UIViewContentModeScaleAspectFit;
		CGSize icon_size = icon_image.size;
		iv.frame = CGRectMake(round((IUP_ALERT_WIDTH - icon_size.width) / 2.0), y, icon_size.width, icon_size.height);
		[content addSubview:iv];
		[iv release];
		y += ceil(icon_size.height) + 8.0;
	}

	/* one-set: lone_font; both: semibold title + smaller message */
	BOOL has_both = (title.length > 0 && message.length > 0);

	if (title.length > 0 || message.length > 0)
	{
		NSString* text = title.length > 0 ? title : message;
		UILabel* lbl = [[UILabel alloc] init];
		lbl.text = text;
		lbl.font = has_both ? title_font : lone_font;
		lbl.textColor = UIColor.labelColor;
		lbl.textAlignment = NSTextAlignmentCenter;
		lbl.numberOfLines = 0;
		CGSize sz = [lbl sizeThatFits:CGSizeMake(content_w, CGFLOAT_MAX)];
		lbl.frame = CGRectMake(side_pad, y, content_w, ceil(sz.height));
		[vibrancy_content addSubview:lbl];
		y += ceil(sz.height);
		[lbl release];
	}

	if (has_both)
	{
		y += 3.0;
		UILabel* lbl = [[UILabel alloc] init];
		lbl.text = message;
		lbl.font = message_font;
		lbl.textColor = UIColor.labelColor;
		lbl.textAlignment = NSTextAlignmentCenter;
		lbl.numberOfLines = 0;
		CGSize sz = [lbl sizeThatFits:CGSizeMake(content_w, CGFLOAT_MAX)];
		lbl.frame = CGRectMake(side_pad, y, content_w, ceil(sz.height));
		[vibrancy_content addSubview:lbl];
		y += ceil(sz.height);
		[lbl release];
	}

	y += top_pad;

	(void)button_line_h;
	CGFloat button_h    = 50.0;
	CGFloat button_gap  = 8.0;
	CGFloat button_area_w = IUP_ALERT_WIDTH - 2 * side_pad;

	int order[8];
	int order_count = 0;
	int cancel_idx = -1;
	for (int i = 0; i < n_buttons; i++)
		if (buttons[i].is_cancel) { cancel_idx = i; break; }

	if (cancel_idx >= 0) {
		_backdropResponse = buttons[cancel_idx].response;
		_hasBackdropResponse = YES;
	}

	if (n_buttons == 2 && cancel_idx >= 0)
	{
		order[0] = cancel_idx;
		order[1] = (cancel_idx == 0) ? 1 : 0;
		order_count = 2;
	}
	else if (cancel_idx >= 0)
	{
		for (int i = 0; i < n_buttons; i++)
			if (i != cancel_idx) order[order_count++] = i;
		order[order_count++] = cancel_idx;
	}
	else
	{
		for (int i = 0; i < n_buttons; i++) order[order_count++] = i;
	}

	UIButton* (^make_button)(int) = ^UIButton*(int idx) {
		BOOL is_default = (idx + 1 == default_idx);
		UIButtonConfiguration* cfg;
		if (button_style.length)
		{
			if      ([button_style caseInsensitiveCompare:@"TONAL"]    == NSOrderedSame) cfg = [UIButtonConfiguration tintedButtonConfiguration];
			else if ([button_style caseInsensitiveCompare:@"OUTLINED"] == NSOrderedSame) cfg = [UIButtonConfiguration borderedButtonConfiguration];
			else if ([button_style caseInsensitiveCompare:@"ELEVATED"] == NSOrderedSame) cfg = [UIButtonConfiguration grayButtonConfiguration];
			else if ([button_style caseInsensitiveCompare:@"TEXT"]     == NSOrderedSame) cfg = [UIButtonConfiguration plainButtonConfiguration];
			else                                                                         cfg = [UIButtonConfiguration filledButtonConfiguration];
		}
		else
		{
			cfg = is_default
			    ? [UIButtonConfiguration filledButtonConfiguration]
			    : [UIButtonConfiguration grayButtonConfiguration];
		}
		cfg.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
		if (corner_style.length)
		{
			if      ([corner_style caseInsensitiveCompare:@"SMALL"]  == NSOrderedSame) cfg.cornerStyle = UIButtonConfigurationCornerStyleSmall;
			else if ([corner_style caseInsensitiveCompare:@"MEDIUM"] == NSOrderedSame) cfg.cornerStyle = UIButtonConfigurationCornerStyleMedium;
			else if ([corner_style caseInsensitiveCompare:@"LARGE"]  == NSOrderedSame) cfg.cornerStyle = UIButtonConfigurationCornerStyleLarge;
		}
		UIFont* f = is_default ? default_btn_font : button_font;
		NSAttributedString* attr = [[[NSAttributedString alloc]
		    initWithString:buttons[idx].title
		        attributes:@{ NSFontAttributeName: f }] autorelease];
		cfg.attributedTitle = attr;
		UIButton* b = [UIButton buttonWithConfiguration:cfg primaryAction:nil];
		b.tag = buttons[idx].response;
		[b addTarget:self action:@selector(buttonTapped:) forControlEvents:UIControlEventTouchUpInside];
		return b;
	};

	if (n_buttons == 2)
	{
		CGFloat half = (button_area_w - button_gap) / 2.0;
		for (int slot = 0; slot < 2; slot++)
		{
			int i = order[slot];
			UIButton* b = make_button(i);
			CGFloat x = side_pad + (slot == 0 ? 0 : half + button_gap);
			b.frame = CGRectMake(x, y, half, button_h);
			[content addSubview:b];
		}
		y += button_h;
	}
	else
	{
		for (int slot = 0; slot < order_count; slot++)
		{
			int i = order[slot];
			UIButton* b = make_button(i);
			b.frame = CGRectMake(side_pad, y, button_area_w, button_h);
			[content addSubview:b];
			y += button_h;
			if (slot + 1 < order_count) y += button_gap;
		}
	}

	y += side_pad;

	CGFloat card_h = y;
	CGRect bounds = self.bounds;
	card.frame = CGRectMake(round((bounds.size.width - IUP_ALERT_WIDTH) / 2.0),
	                        round((bounds.size.height - card_h) / 2.0),
	                        IUP_ALERT_WIDTH, card_h);

	if (_hasBackdropResponse)
	{
		UITapGestureRecognizer* tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(backdropTapped:)];
		tap.delegate = self;
		[self addGestureRecognizer:tap];
		[tap release];
	}

	return self;
}

- (void)backdropTapped:(UITapGestureRecognizer*)g
{
	(void)g;
	if (_hasBackdropResponse && _onTap) _onTap(_backdropResponse);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gr shouldReceiveTouch:(UITouch*)touch
{
	(void)gr;
	CGPoint p = [touch locationInView:self];
	return !CGRectContainsPoint(_card.frame, p);
}

- (void)dealloc
{
	[_onTap release];
	[_card release];
	[super dealloc];
}

- (void)buttonTapped:(UIButton*)sender
{
	int resp = (int)sender.tag;
	if (_onTap) _onTap(resp);
}

- (void)showInWindow:(UIWindow*)window
{
	self.frame = window.bounds;
	[window addSubview:self];
	self.alpha = 0.0;
	_card.transform = CGAffineTransformMakeScale(1.15, 1.15);
	[UIView animateWithDuration:0.25
	                      delay:0.0
	     usingSpringWithDamping:0.85
	      initialSpringVelocity:0.0
	                    options:UIViewAnimationOptionCurveEaseOut
	                 animations:^{
		self.alpha = 1.0;
		_card.transform = CGAffineTransformIdentity;
	} completion:nil];
}

- (void)dismissAnimated
{
	[UIView animateWithDuration:0.20
	                 animations:^{
		self.alpha = 0.0;
		_card.transform = CGAffineTransformMakeScale(0.95, 0.95);
	} completion:^(BOOL finished) {
		(void)finished;
		[self removeFromSuperview];
	}];
}

@end


static UIWindow* cocoaTouchMessageDlgResolveWindow(Ihandle* ih)
{
	Ihandle* parent_ih = IupGetAttributeHandle(ih, "PARENTDIALOG");
	if (parent_ih && parent_ih->handle && [(id)parent_ih->handle isKindOfClass:[IupViewController class]])
	{
		UIViewController* vc = (IupViewController*)parent_ih->handle;
		if (vc.view.window) return vc.view.window;
	}
	UIViewController* top = iupCocoaTouchFindTopPresentedViewController();
	if (top.view.window) return top.view.window;
	for (UIScene* scene in UIApplication.sharedApplication.connectedScenes)
	{
		if ([scene isKindOfClass:[UIWindowScene class]])
		{
			for (UIWindow* w in ((UIWindowScene*)scene).windows)
				if (w.isKeyWindow) return w;
			for (UIWindow* w in ((UIWindowScene*)scene).windows)
				return w;
		}
	}
	return nil;
}

static void cocoaTouchMessageDlgSetCancelResponse(Ihandle* ih, const char* buttons_str)
{
	if (iupStrEqualNoCase(buttons_str, "YESNOCANCEL"))
		iupAttribSet(ih, "BUTTONRESPONSE", "3");
	else if (iupStrEqualNoCase(buttons_str, "OK"))
		iupAttribSet(ih, "BUTTONRESPONSE", "1");
	else
		iupAttribSet(ih, "BUTTONRESPONSE", "2");
}

static int cocoaTouchMessageDlgFillButtons(const char* buttons_str, cocoaTouchMessageDlgButton* out, int max_out)
{
	int count = 0;
	if (iupStrEqualNoCase(buttons_str, "OKCANCEL"))
	{
		out[count++] = (cocoaTouchMessageDlgButton){ @"OK",     1, NO };
		out[count++] = (cocoaTouchMessageDlgButton){ @"Cancel", 2, YES };
	}
	else if (iupStrEqualNoCase(buttons_str, "RETRYCANCEL"))
	{
		const char* retry = IupGetLanguageString("IUP_RETRY");
		out[count++] = (cocoaTouchMessageDlgButton){ retry ? [NSString stringWithUTF8String:retry] : @"Retry", 1, NO };
		out[count++] = (cocoaTouchMessageDlgButton){ @"Cancel", 2, YES };
	}
	else if (iupStrEqualNoCase(buttons_str, "YESNO"))
	{
		out[count++] = (cocoaTouchMessageDlgButton){ @"Yes", 1, NO };
		out[count++] = (cocoaTouchMessageDlgButton){ @"No",  2, NO };
	}
	else if (iupStrEqualNoCase(buttons_str, "YESNOCANCEL"))
	{
		out[count++] = (cocoaTouchMessageDlgButton){ @"Yes",    1, NO };
		out[count++] = (cocoaTouchMessageDlgButton){ @"No",     2, NO };
		out[count++] = (cocoaTouchMessageDlgButton){ @"Cancel", 3, YES };
	}
	else
	{
		out[count++] = (cocoaTouchMessageDlgButton){ @"OK", 1, NO };
	}
	(void)max_out;
	return count;
}

static int cocoaTouchMessageDlgPopup(Ihandle* ih, int x, int y)
{
	(void)x; (void)y;

	const char* title_str   = iupAttribGet(ih, "TITLE");
	const char* value_str   = iupAttribGet(ih, "VALUE");
	const char* buttons_str = iupAttribGetStr(ih, "BUTTONS");
	int button_def          = iupAttribGetInt(ih, "BUTTONDEFAULT");
	int has_help            = IupGetCallback(ih, "HELP_CB") ? 1 : 0;
	if (button_def < 1) button_def = 1;

	cocoaTouchMessageDlgButton buttons[8];
	int n_buttons = cocoaTouchMessageDlgFillButtons(buttons_str, buttons, 8);
	if (has_help && n_buttons < 8)
	{
		const char* help = IupGetLanguageString("IUP_HELP");
		buttons[n_buttons++] = (cocoaTouchMessageDlgButton){
			help ? [NSString stringWithUTF8String:help] : @"Help",
			IUPCOCOATOUCH_MSGDLG_HELP, NO };
	}

	int response = IUPCOCOATOUCH_MSGDLG_PENDING;
	int* response_ref = &response;
	CFRunLoopRef rl = CFRunLoopGetCurrent();

	BOOL keep_showing = YES;
	while (keep_showing)
	{
		keep_showing = NO;
		response = IUPCOCOATOUCH_MSGDLG_PENDING;

		UIWindow* window = cocoaTouchMessageDlgResolveWindow(ih);
		if (!window)
		{
			cocoaTouchMessageDlgSetCancelResponse(ih, buttons_str);
			return IUP_NOERROR;
		}

		NSString* title_ns   = title_str ? [NSString stringWithUTF8String:title_str] : @"";
		NSString* message_ns = value_str ? [NSString stringWithUTF8String:value_str] : @"";

		const char* dlg_type = iupAttribGetStr(ih, "DIALOGTYPE");
		NSString* icon_name  = nil;
		UIColor*  icon_color = nil;
		if (iupStrEqualNoCase(dlg_type, "ERROR"))       { icon_name = @"xmark.octagon.fill";        icon_color = UIColor.systemRedColor; }
		else if (iupStrEqualNoCase(dlg_type, "WARNING"))     { icon_name = @"exclamationmark.triangle.fill"; icon_color = UIColor.systemOrangeColor; }
		else if (iupStrEqualNoCase(dlg_type, "QUESTION"))    { icon_name = @"questionmark.circle.fill";       icon_color = UIColor.systemBlueColor; }
		else if (iupStrEqualNoCase(dlg_type, "INFORMATION")) { icon_name = @"info.circle.fill";              icon_color = UIColor.systemBlueColor; }

		UIImage* icon_image = nil;
		if (icon_name)
		{
			UIImageSymbolConfiguration* sym = [UIImageSymbolConfiguration configurationWithPointSize:36 weight:UIImageSymbolWeightRegular];
			UIImage* base = [UIImage systemImageNamed:icon_name withConfiguration:sym];
			icon_image = [base imageWithTintColor:icon_color renderingMode:UIImageRenderingModeAlwaysOriginal];
		}

		const char* btn_style_c    = iupAttribGet(ih, "BUTTONSTYLE");
		const char* corner_style_c = iupAttribGet(ih, "CORNERSTYLE");
		NSString* btn_style_ns    = btn_style_c    ? [NSString stringWithUTF8String:btn_style_c]    : nil;
		NSString* corner_style_ns = corner_style_c ? [NSString stringWithUTF8String:corner_style_c] : nil;

		IupCocoaTouchAlertView* alert = [[IupCocoaTouchAlertView alloc]
		    initWithTitle:title_ns
		          message:message_ns
		        iconImage:icon_image
		          buttons:buttons
		            count:n_buttons
		       defaultIdx:button_def
		      buttonStyle:btn_style_ns
		      cornerStyle:corner_style_ns
		            onTap:^(int resp) {
			*response_ref = resp;
			CFRunLoopStop(rl);
		}];

		[alert showInWindow:window];

		while (response == IUPCOCOATOUCH_MSGDLG_PENDING)
		{
			@autoreleasepool {
				CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, true);
			}
		}

		[alert dismissAnimated];
		[alert release];

		if (response == IUPCOCOATOUCH_MSGDLG_HELP)
		{
			Icallback cb = IupGetCallback(ih, "HELP_CB");
			if (cb && cb(ih) == IUP_CLOSE)
			{
				cocoaTouchMessageDlgSetCancelResponse(ih, buttons_str);
				return IUP_NOERROR;
			}
			keep_showing = YES;
		}
	}

	char buf[16];
	snprintf(buf, sizeof(buf), "%d", response);
	iupAttribSetStr(ih, "BUTTONRESPONSE", buf);
	return IUP_NOERROR;
}

static char* cocoaTouchMessageDlgGetAutoModalAttrib(Ihandle* ih)
{
	(void)ih;
	return iupStrReturnBoolean(1);
}

IUP_SDK_API void iupdrvMessageDlgInitClass(Iclass* ic)
{
	ic->DlgPopup = cocoaTouchMessageDlgPopup;

	iupClassRegisterAttribute(ic, "AUTOMODAL", cocoaTouchMessageDlgGetAutoModalAttrib, NULL, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "BUTTONSTYLE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_DEFAULTVALUE);
	iupClassRegisterAttribute(ic, "CORNERSTYLE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT|IUPAF_NO_DEFAULTVALUE);
}
