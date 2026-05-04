/** \file
 * \brief Label Control (iOS UIKit)
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
#include "iup_label.h"
#include "iup_drv.h"

#include "iupcocoatouch_drv.h"


static void cocoaTouchLabelFireButtonCb(UIView* view, NSSet<UITouch*>* touches, UIEvent* event, int pressed)
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(view, IHANDLE_ASSOCIATED_OBJ_KEY);
	if (!ih || !iupObjectCheck(ih)) return;
	IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
	if (!cb) return;
	UITouch* touch = [touches anyObject];
	CGPoint p = touch ? [touch locationInView:view] : CGPointZero;
	UIKeyModifierFlags mods = event ? [event modifierFlags] : 0;
	char status[IUPKEY_STATUS_SIZE];
	iupCocoaTouchButtonKeySetStatus(event, mods, pressed ? 1 : 0, 0, status);
	if (cb(ih, IUP_BUTTON1, pressed, (int)p.x, (int)p.y, status) == IUP_CLOSE)
		IupExitLoop();
}


/* UILabel + IUP padding + independent H/V alignment, re-anchored in drawTextInRect */
@interface IupCocoaTouchLabelView : UILabel
@property(nonatomic, assign) UIEdgeInsets textInsets;
/* IUP_ALIGN_ATOP / ACENTER / ABOTTOM */
@property(nonatomic, assign) int verticalAlignment;
/* drops a re-entrant touchesBegan during a nested-run-loop pump from BUTTON_CB */
@property(nonatomic, assign) UITouch* activeTouch;
@end

@implementation IupCocoaTouchLabelView

- (instancetype)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self)
	{
		_textInsets = UIEdgeInsetsZero;
		_verticalAlignment = IUP_ALIGN_ACENTER;
		self.backgroundColor = [UIColor clearColor];
	}
	return self;
}

- (CGSize)intrinsicContentSize
{
	CGSize s = [super intrinsicContentSize];
	s.width  += _textInsets.left + _textInsets.right;
	s.height += _textInsets.top  + _textInsets.bottom;
	return s;
}

- (CGRect)textRectForBounds:(CGRect)bounds limitedToNumberOfLines:(NSInteger)numberOfLines
{
	CGRect inner = UIEdgeInsetsInsetRect(bounds, _textInsets);
	CGRect tr = [super textRectForBounds:inner limitedToNumberOfLines:numberOfLines];
	switch (_verticalAlignment)
	{
		case IUP_ALIGN_ATOP:
			tr.origin.y = inner.origin.y;
			break;
		case IUP_ALIGN_ABOTTOM:
			tr.origin.y = CGRectGetMaxY(inner) - tr.size.height;
			break;
		case IUP_ALIGN_ACENTER:
		default:
			tr.origin.y = inner.origin.y + (inner.size.height - tr.size.height) / 2.0;
			break;
	}
	return tr;
}

- (void)drawTextInRect:(CGRect)rect
{
	CGRect tr = [self textRectForBounds:self.bounds limitedToNumberOfLines:self.numberOfLines];
	[super drawTextInRect:tr];
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesBegan:touches withEvent:event];
	if (_activeTouch) return;
	_activeTouch = [touches anyObject];
	cocoaTouchLabelFireButtonCb(self, touches, event, 1);
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesEnded:touches withEvent:event];
	if (!_activeTouch || ![touches containsObject:_activeTouch]) return;
	_activeTouch = nil;
	cocoaTouchLabelFireButtonCb(self, touches, event, 0);
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesCancelled:touches withEvent:event];
	if (!_activeTouch || ![touches containsObject:_activeTouch]) return;
	_activeTouch = nil;
	cocoaTouchLabelFireButtonCb(self, touches, event, 0);
}

@end


/* image label gets the same BUTTON_CB dispatch as the text label */
@interface IupCocoaTouchLabelImageView : UIImageView
@property(nonatomic, assign) UITouch* activeTouch;
@end

@implementation IupCocoaTouchLabelImageView

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesBegan:touches withEvent:event];
	if (_activeTouch) return;
	_activeTouch = [touches anyObject];
	cocoaTouchLabelFireButtonCb(self, touches, event, 1);
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesEnded:touches withEvent:event];
	if (!_activeTouch || ![touches containsObject:_activeTouch]) return;
	_activeTouch = nil;
	cocoaTouchLabelFireButtonCb(self, touches, event, 0);
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesCancelled:touches withEvent:event];
	if (!_activeTouch || ![touches containsObject:_activeTouch]) return;
	_activeTouch = nil;
	cocoaTouchLabelFireButtonCb(self, touches, event, 0);
}

@end


/* SELECTABLE=YES long-press target -> copy text to pasteboard */
@interface IupCocoaTouchLabelSelectTarget : NSObject
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) UILongPressGestureRecognizer* recognizer;
@end

@implementation IupCocoaTouchLabelSelectTarget

- (void)copyToPasteboard
{
	if (!_ihandle) return;
	IupCocoaTouchLabelView* label = (IupCocoaTouchLabelView*)_ihandle->handle;
	if (![label isKindOfClass:[IupCocoaTouchLabelView class]]) return;
	NSString* text = [label text];
	if (text.length == 0 && label.attributedText) text = label.attributedText.string;
	if (text.length > 0) [[UIPasteboard generalPasteboard] setString:text];
}

- (void)onLongPress:(UILongPressGestureRecognizer*)gr
{
	if (gr.state != UIGestureRecognizerStateBegan) return;
	[self copyToPasteboard];
}

@end


static const void* IUPCOCOATOUCH_LABEL_SELECT_KEY = "IUPCOCOATOUCH_LABEL_SELECT";


static int cocoaTouchLabelType(Ihandle* ih)
{
	return ih->handle ? ih->data->type : iupLabelGetTypeBeforeMap(ih);
}

static IupCocoaTouchLabelView* cocoaTouchLabelGetText(Ihandle* ih)
{
	if (cocoaTouchLabelType(ih) == IUP_LABEL_TEXT && [(id)ih->handle isKindOfClass:[IupCocoaTouchLabelView class]])
	{
		return (IupCocoaTouchLabelView*)ih->handle;
	}
	return nil;
}

static UIImageView* cocoaTouchLabelGetImage(Ihandle* ih)
{
	if (cocoaTouchLabelType(ih) == IUP_LABEL_IMAGE && [(id)ih->handle isKindOfClass:[IupCocoaTouchLabelImageView class]])
	{
		return (UIImageView*)ih->handle;
	}
	return nil;
}

static NSTextAlignment cocoaTouchLabelHorizAlignmentFromIup(int horiz)
{
	switch (horiz)
	{
		case IUP_ALIGN_ARIGHT:  return NSTextAlignmentRight;
		case IUP_ALIGN_ACENTER: return NSTextAlignmentCenter;
		default:                return NSTextAlignmentLeft;
	}
}

static UIViewContentMode cocoaTouchLabelImageContentMode(int horiz, int vert)
{
	if (horiz == IUP_ALIGN_ARIGHT)
	{
		if (vert == IUP_ALIGN_ATOP)    return UIViewContentModeTopRight;
		if (vert == IUP_ALIGN_ABOTTOM) return UIViewContentModeBottomRight;
		return UIViewContentModeRight;
	}
	if (horiz == IUP_ALIGN_ACENTER)
	{
		if (vert == IUP_ALIGN_ATOP)    return UIViewContentModeTop;
		if (vert == IUP_ALIGN_ABOTTOM) return UIViewContentModeBottom;
		return UIViewContentModeCenter;
	}
	if (vert == IUP_ALIGN_ATOP)    return UIViewContentModeTopLeft;
	if (vert == IUP_ALIGN_ABOTTOM) return UIViewContentModeBottomLeft;
	return UIViewContentModeLeft;
}

static UIColor* cocoaTouchLabelUserFgColor(Ihandle* ih, UIColor* fallback)
{
	char* user = iupAttribGet(ih, "_IUPCOCOATOUCH_USER_FGCOLOR");
	UIColor* c = iupCocoaTouchToNativeColor(user);
	return c ? c : fallback;
}

static UIColor* cocoaTouchLabelDefaultTextColor(void)
{
	return [UIColor labelColor];
}

static UIColor* cocoaTouchLabelDisabledTextColor(void)
{
	return [UIColor placeholderTextColor];
}

/* 0-based mnemonic pos in the stripped title (-1 if none, "&&" escapes) */
static int cocoaTouchLabelMnemonicPos(const char* raw)
{
	if (!raw || !strchr(raw, '&')) return -1;
	int pos = 0;
	while (*raw)
	{
		if (*raw == '&')
		{
			raw++;
			if (*raw == '&') { pos++; raw++; continue; }
			return pos;
		}
		raw++; pos++;
	}
	return -1;
}

static void cocoaTouchLabelApplyTitle(IupCocoaTouchLabelView* label, Ihandle* ih, const char* value)
{
	if (!value) { [label setText:@""]; [label setAttributedText:nil]; return; }

	char* stripped = iupStrProcessMnemonic(value, NULL, 0);
	const char* display = stripped ? stripped : value;
	NSString* title = [NSString stringWithUTF8String:display];

	NSAttributedString* markup = nil;
	if (iupAttribGetBoolean(ih, "MARKUP"))
	{
		IupCocoaTouchFont* iup_base = iupCocoaTouchGetFont(ih);
		UIFont* base_font = (iup_base && iup_base.nativeFont) ? iup_base.nativeFont : [UIFont systemFontOfSize:[UIFont systemFontSize]];
		UIColor* base_color = label.textColor ?: [UIColor labelColor];
		markup = iupCocoaTouchParseMarkup(display, base_font, base_color);
	}

	IupCocoaTouchFont* iup_font = iupCocoaTouchGetFont(ih);
	BOOL font_has_attrs = iup_font && [iup_font usesAttributes];
	int mnemonic = markup ? -1 : cocoaTouchLabelMnemonicPos(value);

	if (!markup && (font_has_attrs || (mnemonic >= 0 && mnemonic < (int)title.length)))
	{
		NSMutableAttributedString* attr = [[[NSMutableAttributedString alloc]
			initWithString:title] autorelease];
		UIFont* base_font = label.font ?: [UIFont systemFontOfSize:[UIFont systemFontSize]];
		UIColor* base_color = label.textColor ?: [UIColor labelColor];
		NSRange full = NSMakeRange(0, title.length);
		[attr addAttribute:NSFontAttributeName value:base_font range:full];
		[attr addAttribute:NSForegroundColorAttributeName value:base_color range:full];

		if (font_has_attrs)
		{
			NSDictionary* font_dict = [iup_font attributeDictionary];
			for (NSString* key in font_dict)
			{
				if ([key isEqualToString:NSFontAttributeName]) continue;
				if ([key isEqualToString:NSForegroundColorAttributeName]) continue;
				[attr addAttribute:key value:font_dict[key] range:full];
			}
		}

		if (mnemonic >= 0 && mnemonic < (int)title.length)
		{
			[attr addAttribute:NSUnderlineStyleAttributeName value:@(NSUnderlineStyleSingle)
			             range:NSMakeRange((NSUInteger)mnemonic, 1)];
		}
		markup = attr;
	}

	if (stripped && stripped != value) free(stripped);

	if (markup) [label setAttributedText:markup];
	else        { [label setAttributedText:nil]; [label setText:title ?: @""]; }
}

static void cocoaTouchLabelApplyActiveColor(Ihandle* ih)
{
	IupCocoaTouchLabelView* label = cocoaTouchLabelGetText(ih);
	if (!label)
	{
		return;
	}
	UIColor* color;
	if (iupdrvIsActive(ih))
	{
		color = cocoaTouchLabelUserFgColor(ih, cocoaTouchLabelDefaultTextColor());
	}
	else
	{
		color = cocoaTouchLabelDisabledTextColor();
	}
	[label setTextColor:color];
	/* attributedText overrides textColor; re-seed so FGCOLOR sticks */
	if (label.attributedText.length > 0)
	{
		char* title = iupAttribGet(ih, "TITLE");
		if (title) cocoaTouchLabelApplyTitle(label, ih, title);
	}
}

static void cocoaTouchLabelApplyWrapMode(IupCocoaTouchLabelView* label, BOOL wordwrap, BOOL ellipsis)
{
	/* numberOfLines=0 = unlimited; '\n' still breaks without WORDWRAP/ELLIPSIS */
	if (wordwrap)
	{
		[label setNumberOfLines:0];
		[label setLineBreakMode:ellipsis ? NSLineBreakByTruncatingTail : NSLineBreakByWordWrapping];
	}
	else if (ellipsis)
	{
		[label setNumberOfLines:1];
		[label setLineBreakMode:NSLineBreakByTruncatingTail];
	}
	else
	{
		[label setNumberOfLines:0];
		[label setLineBreakMode:NSLineBreakByClipping];
	}
}

static void cocoaTouchLabelApplyPadding(Ihandle* ih)
{
	IupCocoaTouchLabelView* label = cocoaTouchLabelGetText(ih);
	if (!label)
	{
		return;
	}
	[label setTextInsets:UIEdgeInsetsMake(ih->data->vert_padding,  ih->data->horiz_padding,
	                                      ih->data->vert_padding,  ih->data->horiz_padding)];
	[label setNeedsDisplay];
}

IUP_SDK_API void iupdrvLabelAddExtraPadding(Ihandle* ih, int* x, int* y)
{
	/* text labels inset via textRectForBounds; image/separator need it added here */
	if (!ih) return;
	int type = cocoaTouchLabelType(ih);
	if (type != IUP_LABEL_IMAGE && type != IUP_LABEL_SEP_HORIZ && type != IUP_LABEL_SEP_VERT)
		return;
	if (x) *x += 2 * ih->data->horiz_padding;
	if (y) *y += 2 * ih->data->vert_padding;
}

static int cocoaTouchLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
	iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
	if (ih->handle)
	{
		cocoaTouchLabelApplyPadding(ih);
	}
	return 1;
}

static int cocoaTouchLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchLabelView* label = cocoaTouchLabelGetText(ih);
	if (!label) return 0;
	cocoaTouchLabelApplyTitle(label, ih, value);
	return 1;
}

static char* cocoaTouchLabelGetAlignmentAttrib(Ihandle* ih)
{
	int type = cocoaTouchLabelType(ih);
	if (type == IUP_LABEL_SEP_HORIZ || type == IUP_LABEL_SEP_VERT)
	{
		return NULL;
	}
	const char* horiz_names[3] = { "ALEFT", "ACENTER", "ARIGHT" };
	const char* vert_names[3]  = { "ATOP",  "ACENTER", "ABOTTOM" };
	int horiz = ih->data->horiz_alignment;
	int vert  = ih->data->vert_alignment;
	if (horiz < IUP_ALIGN_ALEFT || horiz > IUP_ALIGN_ARIGHT)   horiz = IUP_ALIGN_ACENTER;
	if (vert  < IUP_ALIGN_ATOP  || vert  > IUP_ALIGN_ABOTTOM) vert  = IUP_ALIGN_ACENTER;
	return iupStrReturnStrf("%s:%s", horiz_names[horiz], vert_names[vert]);
}

static int cocoaTouchLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
	int type = cocoaTouchLabelType(ih);
	if (type == IUP_LABEL_SEP_HORIZ || type == IUP_LABEL_SEP_VERT)
	{
		return 0;
	}

	char h[30], v[30];
	iupStrToStrStr(value, h, sizeof(h), v, sizeof(v), ':');

	int horiz = ih->data->horiz_alignment;
	int vert  = ih->data->vert_alignment;
	if (iupStrEqualNoCase(h, "ARIGHT"))       horiz = IUP_ALIGN_ARIGHT;
	else if (iupStrEqualNoCase(h, "ACENTER")) horiz = IUP_ALIGN_ACENTER;
	else if (iupStrEqualNoCase(h, "ALEFT"))   horiz = IUP_ALIGN_ALEFT;

	if (iupStrEqualNoCase(v, "ABOTTOM"))      vert = IUP_ALIGN_ABOTTOM;
	else if (iupStrEqualNoCase(v, "ACENTER")) vert = IUP_ALIGN_ACENTER;
	else if (iupStrEqualNoCase(v, "ATOP"))    vert = IUP_ALIGN_ATOP;

	ih->data->horiz_alignment = horiz;
	ih->data->vert_alignment  = vert;

	if (type == IUP_LABEL_TEXT)
	{
		IupCocoaTouchLabelView* label = cocoaTouchLabelGetText(ih);
		if (label)
		{
			[label setTextAlignment:cocoaTouchLabelHorizAlignmentFromIup(horiz)];
			[label setVerticalAlignment:vert];
			[label setNeedsDisplay];
		}
	}
	else if (type == IUP_LABEL_IMAGE)
	{
		UIImageView* image = cocoaTouchLabelGetImage(ih);
		if (image)
		{
			[image setContentMode:cocoaTouchLabelImageContentMode(horiz, vert)];
		}
	}
	return 1;
}

static int cocoaTouchLabelSetWordWrapAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchLabelView* label = cocoaTouchLabelGetText(ih);
	if (!label)
	{
		return 0;
	}
	BOOL wordwrap = iupStrBoolean(value) ? YES : NO;
	BOOL ellipsis = iupStrBoolean(iupAttribGet(ih, "ELLIPSIS")) ? YES : NO;
	cocoaTouchLabelApplyWrapMode(label, wordwrap, ellipsis);
	return 1;
}

static int cocoaTouchLabelSetEllipsisAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchLabelView* label = cocoaTouchLabelGetText(ih);
	if (!label)
	{
		return 0;
	}
	BOOL ellipsis = iupStrBoolean(value) ? YES : NO;
	BOOL wordwrap = iupStrBoolean(iupAttribGet(ih, "WORDWRAP")) ? YES : NO;
	cocoaTouchLabelApplyWrapMode(label, wordwrap, ellipsis);
	return 1;
}

static int cocoaTouchLabelSetImageAttrib(Ihandle* ih, const char* value)
{
	UIImageView* image_view = cocoaTouchLabelGetImage(ih);
	if (!image_view)
	{
		return 0;
	}
	const char* name = value;
	int make_inactive = 0;
	if (!iupdrvIsActive(ih))
	{
		const char* inactive = iupAttribGet(ih, "IMINACTIVE");
		if (inactive) name = inactive;
		else          make_inactive = 1;
	}
	if (!name)
	{
		[image_view setImage:nil];
		return 1;
	}
	UIImage* bitmap = iupImageGetImage(name, ih, make_inactive, NULL);
	[image_view setImage:bitmap];
	return 1;
}

static int cocoaTouchLabelSetImInactiveAttrib(Ihandle* ih, const char* value)
{
	UIImageView* image_view = cocoaTouchLabelGetImage(ih);
	if (!image_view || iupdrvIsActive(ih))
	{
		return 1;
	}
	if (value)
	{
		[image_view setImage:iupImageGetImage(value, ih, 0, NULL)];
	}
	else
	{
		const char* name = iupAttribGet(ih, "IMAGE");
		if (name)
		{
			[image_view setImage:iupImageGetImage(name, ih, 1, NULL)];
		}
	}
	return 1;
}

static int cocoaTouchLabelSetBgColorAttrib(Ihandle* ih, const char* value)
{
	UIColor* color = iupCocoaTouchToNativeColor(value);
	if (!color || ![(id)ih->handle isKindOfClass:[UIView class]])
	{
		return 0;
	}
	[(UIView*)ih->handle setBackgroundColor:color];
	return 1;
}

static int cocoaTouchLabelSetFgColorAttrib(Ihandle* ih, const char* value)
{
	int type = cocoaTouchLabelType(ih);
	if (type == IUP_LABEL_SEP_HORIZ || type == IUP_LABEL_SEP_VERT)
	{
		return 0;
	}

	unsigned char r, g, b;
	if (!iupStrToRGB(value, &r, &g, &b))
	{
		return 0;
	}
	iupAttribSetStr(ih, "_IUPCOCOATOUCH_USER_FGCOLOR", value);
	cocoaTouchLabelApplyActiveColor(ih);
	return 1;
}

static int cocoaTouchLabelSetSelectableAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchLabelView* label = cocoaTouchLabelGetText(ih);
	if (!label) return 1;

	IupCocoaTouchLabelSelectTarget* existing = objc_getAssociatedObject(label, IUPCOCOATOUCH_LABEL_SELECT_KEY);
	BOOL want = iupStrBoolean(value) ? YES : NO;

	if (!want)
	{
		if (existing)
		{
			if (existing.recognizer) [label removeGestureRecognizer:existing.recognizer];
			objc_setAssociatedObject(label, IUPCOCOATOUCH_LABEL_SELECT_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
		}
		return 1;
	}

	if (existing) return 1;

	IupCocoaTouchLabelSelectTarget* target = [[[IupCocoaTouchLabelSelectTarget alloc] init] autorelease];
	target.ihandle = ih;

	UILongPressGestureRecognizer* lp = [[[UILongPressGestureRecognizer alloc]
		initWithTarget:target action:@selector(onLongPress:)] autorelease];
	[label addGestureRecognizer:lp];
	target.recognizer = lp;

	objc_setAssociatedObject(label, IUPCOCOATOUCH_LABEL_SELECT_KEY, target, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	return 1;
}

static int cocoaTouchLabelSetFontAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchLabelView* label = cocoaTouchLabelGetText(ih);
	if (!label || !value)
	{
		return 0;
	}
	IupCocoaTouchFont* font = iupCocoaTouchFindFont(value);
	if (!font || ![font nativeFont])
	{
		return 0;
	}
	[label setFont:[font nativeFont]];
	cocoaTouchLabelApplyActiveColor(ih);
	/* re-emit title so the font's underline/strikeout attrs take effect */
	char* title = iupAttribGet(ih, "TITLE");
	if (title) cocoaTouchLabelApplyTitle(label, ih, title);
	return 1;
}

static int cocoaTouchLabelSetActiveAttrib(Ihandle* ih, const char* value)
{
	int active = iupStrBoolean(value) ? 1 : 0;
	int type = cocoaTouchLabelType(ih);

	if (type == IUP_LABEL_TEXT)
	{
		cocoaTouchLabelApplyActiveColor(ih);
	}
	else if (type == IUP_LABEL_IMAGE)
	{
		UIImageView* image_view = cocoaTouchLabelGetImage(ih);
		if (image_view)
		{
			const char* image_name = iupAttribGet(ih, "IMAGE");
			int make_inactive = 0;
			if (!active)
			{
				const char* inactive = iupAttribGet(ih, "IMINACTIVE");
				if (inactive) image_name = inactive;
				else          make_inactive = 1;
			}
			if (image_name)
			{
				[image_view setImage:iupImageGetImage(image_name, ih, make_inactive, NULL)];
			}
		}
	}
	return iupBaseSetActiveAttrib(ih, value);
}

static UIColor* cocoaTouchSeparatorColor(void)
{
	return [UIColor separatorColor];
}

static int cocoaTouchLabelMapMethod(Ihandle* ih)
{
	UIView* widget = nil;
	char* value;

	if ((value = iupAttribGet(ih, "SEPARATOR")) != NULL)
	{
		UIView* sep = [[UIView alloc] initWithFrame:CGRectZero];
		[sep setBackgroundColor:cocoaTouchSeparatorColor()];
		if (iupStrEqualNoCase(value, "HORIZONTAL"))
		{
			ih->data->type = IUP_LABEL_SEP_HORIZ;
		}
		else
		{
			ih->data->type = IUP_LABEL_SEP_VERT;
		}
		widget = sep;
	}
	else if ((value = iupAttribGet(ih, "IMAGE")) != NULL)
	{
		ih->data->type = IUP_LABEL_IMAGE;

		const char* name = value;
		int make_inactive = 0;
		if (!iupdrvIsActive(ih))
		{
			const char* inactive = iupAttribGet(ih, "IMINACTIVE");
			if (inactive) name = inactive;
			else          make_inactive = 1;
		}
		UIImage* bitmap = iupImageGetImage(name, ih, make_inactive, NULL);

		IupCocoaTouchLabelImageView* image_view = [[IupCocoaTouchLabelImageView alloc] initWithImage:bitmap];
		[image_view setContentMode:cocoaTouchLabelImageContentMode(ih->data->horiz_alignment, ih->data->vert_alignment)];
		[image_view setUserInteractionEnabled:YES];
		widget = image_view;
	}
	else
	{
		ih->data->type = IUP_LABEL_TEXT;

		IupCocoaTouchLabelView* label = [[IupCocoaTouchLabelView alloc] initWithFrame:CGRectZero];
		[label setUserInteractionEnabled:YES];

		IupCocoaTouchFont* font = iupCocoaTouchGetFont(ih);
		if (font && [font nativeFont])
		{
			[label setFont:[font nativeFont]];
		}

		[label setTextAlignment:cocoaTouchLabelHorizAlignmentFromIup(ih->data->horiz_alignment)];
		[label setVerticalAlignment:ih->data->vert_alignment];
		[label setTextInsets:UIEdgeInsetsMake(ih->data->vert_padding,  ih->data->horiz_padding,
		                                      ih->data->vert_padding,  ih->data->horiz_padding)];

		BOOL wordwrap = iupStrBoolean(iupAttribGet(ih, "WORDWRAP")) ? YES : NO;
		BOOL ellipsis = iupStrBoolean(iupAttribGet(ih, "ELLIPSIS")) ? YES : NO;
		cocoaTouchLabelApplyWrapMode(label, wordwrap, ellipsis);

		char* title = iupAttribGet(ih, "TITLE");
		if (title && *title) cocoaTouchLabelApplyTitle(label, ih, title);

		widget = label;
	}

	if (!widget)
	{
		return IUP_ERROR;
	}

	ih->handle = widget;
	objc_setAssociatedObject(widget, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

	iupCocoaTouchAddToParent(ih);

	/* FGCOLOR setter needs ih->handle; apply now that it exists */
	if (ih->data->type == IUP_LABEL_TEXT)
	{
		cocoaTouchLabelApplyActiveColor(ih);
	}

	return IUP_NOERROR;
}

IUP_SDK_API void iupdrvLabelInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchLabelMapMethod;
	ic->UnMap = iupdrvBaseUnMapMethod;

	/* Common overrides */
	iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, cocoaTouchLabelSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "BGCOLOR", iupBaseNativeParentGetBgColorAttrib, cocoaTouchLabelSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTouchLabelSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FONT", NULL, cocoaTouchLabelSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

	/* IupLabel core */
	iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaTouchLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ALIGNMENT", cocoaTouchLabelGetAlignmentAttrib, cocoaTouchLabelSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT:ACENTER", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaTouchLabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMINACTIVE",NULL, cocoaTouchLabelSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, cocoaTouchLabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
	iupClassRegisterAttribute(ic, "WORDWRAP", NULL, cocoaTouchLabelSetWordWrapAttrib, NULL, NULL, IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, cocoaTouchLabelSetEllipsisAttrib, NULL, NULL, IUPAF_DEFAULT);

	/* MARKUP=YES wires a Pango subset: b/i/u/s + <span foreground="#..."> */
	iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);

	/* SELECTABLE=YES installs a long-press that copies title to the pasteboard */
	iupClassRegisterAttribute(ic, "SELECTABLE", NULL, cocoaTouchLabelSetSelectableAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
