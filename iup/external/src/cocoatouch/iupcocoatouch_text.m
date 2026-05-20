/** \file
 * \brief Text Control (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_mask.h"
#include "iup_focus.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_text.h"
#include "iup_key.h"

#include "iupcocoatouch_drv.h"


static const void* IUPCOCOATOUCH_TEXT_DELEGATE_KEY = "IUPCOCOATOUCH_TEXT_DELEGATE";

/* hardware-keyboard presses route to iupCocoaTouchKeyEvent for KEYPRESS_CB / K_ANY */
@interface IupCocoaTouchTextField : UITextField
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTouchTextField

- (void)pressesBegan:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	BOOL handled = NO;
	for (UIPress* p in presses)
		if (iupCocoaTouchKeyEvent(_ihandle, p, true)) handled = YES;
	if (!handled) [super pressesBegan:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	BOOL handled = NO;
	for (UIPress* p in presses)
		if (iupCocoaTouchKeyEvent(_ihandle, p, false)) handled = YES;
	if (!handled) [super pressesEnded:presses withEvent:event];
}

@end


@interface IupCocoaTouchTextView : UITextView
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTouchTextView

- (void)pressesBegan:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	BOOL handled = NO;
	for (UIPress* p in presses)
		if (iupCocoaTouchKeyEvent(_ihandle, p, true)) handled = YES;
	if (!handled) [super pressesBegan:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	BOOL handled = NO;
	for (UIPress* p in presses)
		if (iupCocoaTouchKeyEvent(_ihandle, p, false)) handled = YES;
	if (!handled) [super pressesEnded:presses withEvent:event];
}

@end


@interface IupCocoaTouchSpinView : UIView
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, retain) UITextField* field;
@property(nonatomic, retain) UIStepper* stepper;
@end

@implementation IupCocoaTouchSpinView

- (instancetype)initWithFrame:(CGRect)frame ihandle:(Ihandle*)ih
{
	self = [super initWithFrame:frame];
	if (self)
	{
		_ihandle = ih;
		IupCocoaTouchTextField* f = [[IupCocoaTouchTextField alloc] initWithFrame:CGRectZero];
		f.ihandle = ih;
		f.borderStyle = UITextBorderStyleRoundedRect;
		f.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
		_field = f;
		[self addSubview:f];

		_stepper = [[UIStepper alloc] initWithFrame:CGRectZero];
		[self addSubview:_stepper];
	}
	return self;
}

- (void)dealloc
{
	[_field release];
	[_stepper release];
	[super dealloc];
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	CGFloat w = self.bounds.size.width;
	CGFloat h = self.bounds.size.height;
	CGSize stepper_size = [_stepper intrinsicContentSize];
	CGFloat stepper_w = stepper_size.width > 0 ? stepper_size.width : 94.0;
	CGFloat gap = 4.0;
	CGFloat field_w = w - stepper_w - gap;
	if (field_w < 0) field_w = 0;
	_field.frame = CGRectMake(0, 0, field_w, h);
	_stepper.frame = CGRectMake(field_w + gap, (h - stepper_size.height)/2, stepper_w, stepper_size.height);
}

@end


@interface IupCocoaTouchTextDelegate : NSObject <UITextFieldDelegate, UITextViewDelegate, UIGestureRecognizerDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@end


static IupCocoaTouchSpinView* cocoaTouchTextSpin(Ihandle* ih)
{
	if (!ih) return nil;
	id h = ih->handle;
	return [h isKindOfClass:[IupCocoaTouchSpinView class]] ? (IupCocoaTouchSpinView*)h : nil;
}

static UITextField* cocoaTouchTextField(Ihandle* ih)
{
	if (!ih) return nil;
	id h = ih->handle;
	if ([h isKindOfClass:[UITextField class]]) return (UITextField*)h;
	IupCocoaTouchSpinView* spin = cocoaTouchTextSpin(ih);
	return spin ? spin.field : nil;
}

static UITextView* cocoaTouchTextView(Ihandle* ih)
{
	if (!ih) return nil;
	id h = ih->handle;
	return [h isKindOfClass:[UITextView class]] ? (UITextView*)h : nil;
}

static UIView* cocoaTouchTextEditor(Ihandle* ih)
{
	UITextField* f = cocoaTouchTextField(ih);
	if (f) return f;
	return cocoaTouchTextView(ih);
}

static NSString* cocoaTouchTextGetString(Ihandle* ih)
{
	UITextField* f = cocoaTouchTextField(ih);
	if (f) return [f text] ? [f text] : @"";
	UITextView* v = cocoaTouchTextView(ih);
	if (v) return [v text] ? [v text] : @"";
	return @"";
}

static void cocoaTouchTextSetString(Ihandle* ih, NSString* value)
{
	UITextField* f = cocoaTouchTextField(ih);
	if (f) { [f setText:value]; return; }
	UITextView* v = cocoaTouchTextView(ih);
	if (v) [v setText:value];
}

static NSRange cocoaTouchTextFieldSelection(UITextField* f)
{
	UITextRange* r = f.selectedTextRange;
	if (!r) return NSMakeRange(0, 0);
	NSInteger s = [f offsetFromPosition:f.beginningOfDocument toPosition:r.start];
	NSInteger e = [f offsetFromPosition:f.beginningOfDocument toPosition:r.end];
	if (s < 0) s = 0;
	if (e < s) e = s;
	return NSMakeRange((NSUInteger)s, (NSUInteger)(e - s));
}

static void cocoaTouchTextFieldSetSelection(UITextField* f, NSUInteger start, NSUInteger end)
{
	NSUInteger len = [[f text] length];
	if (start > len) start = len;
	if (end   > len) end   = len;
	if (end < start) end = start;
	UITextPosition* s = [f positionFromPosition:f.beginningOfDocument offset:(NSInteger)start];
	UITextPosition* e = [f positionFromPosition:f.beginningOfDocument offset:(NSInteger)end];
	if (s && e) f.selectedTextRange = [f textRangeFromPosition:s toPosition:e];
}

static NSRange cocoaTouchTextSelection(Ihandle* ih)
{
	UITextField* f = cocoaTouchTextField(ih);
	if (f) return cocoaTouchTextFieldSelection(f);
	UITextView* v = cocoaTouchTextView(ih);
	if (v) return [v selectedRange];
	return NSMakeRange(0, 0);
}

static void cocoaTouchTextSetSelection(Ihandle* ih, NSUInteger start, NSUInteger end)
{
	UITextField* f = cocoaTouchTextField(ih);
	if (f) { cocoaTouchTextFieldSetSelection(f, start, end); return; }
	UITextView* v = cocoaTouchTextView(ih);
	if (!v) return;
	NSUInteger len = [[v text] length];
	if (start > len) start = len;
	if (end   > len) end   = len;
	if (end < start) end = start;
	[v setSelectedRange:NSMakeRange(start, end - start)];
}

IUP_SDK_API void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int* pos)
{
	if (!pos) return;
	*pos = 0;
	NSString* s = cocoaTouchTextGetString(ih);
	if (!s) return;

	if (lin < 1) lin = 1;
	if (col < 1) col = 1;
	int cur_lin = 1, cur_col = 1;
	NSUInteger len = [s length];
	for (NSUInteger i = 0; i < len; i++)
	{
		if (cur_lin == lin && cur_col == col)
		{
			*pos = (int)i;
			return;
		}
		unichar c = [s characterAtIndex:i];
		if (c == '\n')
		{
			if (cur_lin == lin) { *pos = (int)i; return; }
			cur_lin++; cur_col = 1;
		}
		else cur_col++;
	}
	*pos = (int)len;
}

IUP_SDK_API void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int* lin, int* col)
{
	if (lin) *lin = 1;
	if (col) *col = 1;
	NSString* s = cocoaTouchTextGetString(ih);
	if (!s || pos <= 0) return;
	NSUInteger len = [s length];
	if ((NSUInteger)pos > len) pos = (int)len;
	int l = 1, c = 1;
	for (int i = 0; i < pos; i++)
	{
		unichar ch = [s characterAtIndex:(NSUInteger)i];
		if (ch == '\n') { l++; c = 1; }
		else             c++;
	}
	if (lin) *lin = l;
	if (col) *col = c;
}

IUP_SDK_API void iupdrvTextAddExtraPadding(Ihandle* ih, int* w, int* h)
{
	(void)ih;
	if (w) *w += 16;
	if (h) *h += 12;
}

IUP_SDK_API void iupdrvTextAddSpin(Ihandle* ih, int* w, int h)
{
	(void)h;
	if (!ih || !iupAttribGetBoolean(ih, "SPIN")) return;
	if (w) *w += 94;  /* UIStepper intrinsic width */
}

IUP_SDK_API void iupdrvTextAddBorders(Ihandle* ih, int* x, int* y)
{
	(void)ih;
	if (x) *x += 2;
	if (y) *y += 2;
}

/* bulk begin/end builds up a mutable attributed string; commit once on end */
IUP_SDK_API void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
	UITextView* tv = cocoaTouchTextView(ih);
	if (!tv) return NULL;
	NSMutableAttributedString* bulk = [[NSMutableAttributedString alloc] initWithAttributedString:tv.attributedText];
	iupAttribSet(ih, "_IUPCOCOATOUCH_BULK_ATTR", (char*)bulk);
	return bulk;
}

IUP_SDK_API void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
	UITextView* tv = cocoaTouchTextView(ih);
	NSMutableAttributedString* bulk = (NSMutableAttributedString*)state;
	iupAttribSet(ih, "_IUPCOCOATOUCH_BULK_ATTR", NULL);
	if (tv && bulk)
	{
		tv.attributedText = bulk;
		/* grow ih to rendered content so the outer scroll handles overflow */
		tv.scrollEnabled = NO;
		CGFloat fit_w = tv.frame.size.width > 0 ? tv.frame.size.width : tv.contentSize.width;
		CGSize fit = [tv sizeThatFits:CGSizeMake(fit_w, CGFLOAT_MAX)];
		int new_h = (int)ceil(fit.height);
		if (new_h > ih->currentheight)
		{
			ih->userheight = new_h;
			ih->expand &= ~IUP_EXPAND_HEIGHT;
			IupRefresh(ih);
		}
	}
	[bulk release];
}

static NSRange cocoaTouchTextResolveFormatRange(Ihandle* ih, Ihandle* tag, NSUInteger text_len)
{
	const char* sel = iupAttribGet(tag, "SELECTION");
	if (sel)
	{
		if (ih->data->is_multiline)
		{
			int l1 = 1, c1 = 1, l2 = 1, c2 = 1;
			if (sscanf(sel, "%d,%d:%d,%d", &l1, &c1, &l2, &c2) == 4)
			{
				int s = 0, e = 0;
				iupdrvTextConvertLinColToPos(ih, l1, c1, &s);
				iupdrvTextConvertLinColToPos(ih, l2, c2, &e);
				if (s < 0) s = 0; if (e < s) e = s;
				if ((NSUInteger)e > text_len) e = (int)text_len;
				return NSMakeRange((NSUInteger)s, (NSUInteger)(e - s));
			}
		}
		int s = 0, e = 0;
		if (iupStrToIntInt(sel, &s, &e, ':') == 2)
		{
			s--; e--;  /* 1-based */
			if (s < 0) s = 0; if (e < s) e = s;
			if ((NSUInteger)e > text_len) e = (int)text_len;
			return NSMakeRange((NSUInteger)s, (NSUInteger)(e - s));
		}
	}

	const char* selpos = iupAttribGet(tag, "SELECTIONPOS");
	if (selpos)
	{
		int s = 0, e = 0;
		if (iupStrToIntInt(selpos, &s, &e, ':') == 2)
		{
			if (s < 0) s = 0; if (e < s) e = s;
			if ((NSUInteger)e > text_len) e = (int)text_len;
			return NSMakeRange((NSUInteger)s, (NSUInteger)(e - s));
		}
	}

	return NSMakeRange(0, text_len);
}

static BOOL cocoaTouchTextFontIsSystem(UIFont* font)
{
	if (!font) return YES;
	NSString* name = font.fontName;
	if (!name) return YES;
	return [name hasPrefix:@"."] || [name caseInsensitiveCompare:@"System"] == NSOrderedSame;
}

static CGFloat cocoaTouchTextFontScaleFactor(const char* scale);

static UIFont* cocoaTouchTextTagBuildFont(Ihandle* tag, UIFont* base)
{
	const char* face = iupAttribGet(tag, "FONTFACE");
	const char* size_s = iupAttribGet(tag, "FONTSIZE");
	const char* weight = iupAttribGet(tag, "WEIGHT");
	const char* italic = iupAttribGet(tag, "ITALIC");
	const char* style = iupAttribGet(tag, "FONTSTYLE");
	const char* scale_s = iupAttribGet(tag, "FONTSCALE");

	if (!face && !size_s && !weight && !italic && !style && !scale_s)
		return base;

	UIFont* out = base;
	CGFloat size = base ? base.pointSize : [UIFont systemFontSize];
	if (size_s) { int v = 0; if (iupStrToInt(size_s, &v) && v > 0) size = (CGFloat)v; }
	if (scale_s) size *= cocoaTouchTextFontScaleFactor(scale_s);

	BOOL is_bold = NO;
	BOOL is_italic = NO;
	if (style)
	{
		if (iupStrEqualNoCase(style, "BOLD"))          is_bold = YES;
		else if (iupStrEqualNoCase(style, "ITALIC"))   is_italic = YES;
		else if (iupStrEqualNoCase(style, "BOLD ITALIC") || iupStrEqualNoCase(style, "BOLDITALIC"))
		{ is_bold = YES; is_italic = YES; }
	}
	if (weight)
	{
		if (iupStrEqualNoCase(weight, "BOLD") || iupStrEqualNoCase(weight, "SEMIBOLD") || iupStrEqualNoCase(weight, "BLACK"))
			is_bold = YES;
	}
	if (italic) is_italic = iupStrBoolean(italic) ? YES : NO;

	BOOL named_face = (face && face[0] != '.' && !iupStrEqualNoCase(face, "System"));
	if (named_face)
	{
		UIFont* named = [UIFont fontWithName:[NSString stringWithUTF8String:face] size:size];
		if (named) out = named;
	}

	BOOL use_system_factory = !named_face && cocoaTouchTextFontIsSystem(out);

	if (use_system_factory)
	{
		if (is_bold && is_italic)
		{
			UIFont* sys_bold = [UIFont systemFontOfSize:size weight:UIFontWeightBold];
			UIFontDescriptor* fd = [[sys_bold fontDescriptor]
				fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitBold | UIFontDescriptorTraitItalic];
			UIFont* styled = fd ? [UIFont fontWithDescriptor:fd size:size] : nil;
			out = styled ?: sys_bold;
		}
		else if (is_bold)
		{
			out = [UIFont boldSystemFontOfSize:size];
		}
		else if (is_italic)
		{
			out = [UIFont italicSystemFontOfSize:size];
		}
		else
		{
			out = [UIFont systemFontOfSize:size];
		}
	}
	else if (is_bold || is_italic)
	{
		UIFontDescriptorSymbolicTraits t = 0;
		if (is_bold)   t |= UIFontDescriptorTraitBold;
		if (is_italic) t |= UIFontDescriptorTraitItalic;
		UIFontDescriptor* fd = [[out fontDescriptor] fontDescriptorWithSymbolicTraits:t];
		if (fd) { UIFont* styled = [UIFont fontWithDescriptor:fd size:size]; if (styled) out = styled; }
	}
	else if (size_s)
	{
		out = [UIFont fontWithDescriptor:out.fontDescriptor size:size];
	}
	return out;
}

static NSParagraphStyle* cocoaTouchTextTagBuildParagraph(Ihandle* tag)
{
	const char* align = iupAttribGet(tag, "ALIGNMENT");
	const char* indent_s = iupAttribGet(tag, "INDENT");
	const char* space_before_s = iupAttribGet(tag, "SPACEBEFORE");
	const char* space_after_s = iupAttribGet(tag, "SPACEAFTER");
	const char* line_spacing_s = iupAttribGet(tag, "LINESPACING");
	if (!align && !indent_s && !space_before_s && !space_after_s && !line_spacing_s) return nil;

	NSMutableParagraphStyle* ps = [[[NSMutableParagraphStyle alloc] init] autorelease];
	if (align)
	{
		if (iupStrEqualNoCase(align, "RIGHT"))   ps.alignment = NSTextAlignmentRight;
		else if (iupStrEqualNoCase(align, "CENTER")) ps.alignment = NSTextAlignmentCenter;
		else if (iupStrEqualNoCase(align, "JUSTIFY")) ps.alignment = NSTextAlignmentJustified;
		else                                         ps.alignment = NSTextAlignmentLeft;
	}
	int v = 0;
	if (indent_s && iupStrToInt(indent_s, &v))        ps.firstLineHeadIndent = (CGFloat)v;
	if (space_before_s && iupStrToInt(space_before_s, &v)) ps.paragraphSpacingBefore = (CGFloat)v;
	if (space_after_s  && iupStrToInt(space_after_s,  &v)) ps.paragraphSpacing       = (CGFloat)v;
	if (line_spacing_s)
	{
		double d = 0; if (iupStrToDouble(line_spacing_s, &d)) ps.lineHeightMultiple = (CGFloat)d;
	}
	return ps;
}

static CGFloat cocoaTouchTextFontScaleFactor(const char* scale)
{
	if (iupStrEqualNoCase(scale, "XX-SMALL")) return 0.5787037037037;
	if (iupStrEqualNoCase(scale, "X-SMALL"))  return 0.6444444444444;
	if (iupStrEqualNoCase(scale, "SMALL"))    return 0.8333333333333;
	if (iupStrEqualNoCase(scale, "MEDIUM"))   return 1.0;
	if (iupStrEqualNoCase(scale, "LARGE"))    return 1.2;
	if (iupStrEqualNoCase(scale, "X-LARGE"))  return 1.4399999999999;
	if (iupStrEqualNoCase(scale, "XX-LARGE")) return 1.728;
	double v = 0;
	if (iupStrToDouble(scale, &v) && v > 0) return (CGFloat)v;
	return 1.0;
}

static void cocoaTouchTextApplyTagToAttributes(Ihandle* tag, NSMutableDictionary* attrs)
{
	UIFont* base = attrs[NSFontAttributeName];
	if (!base) base = [UIFont systemFontOfSize:[UIFont systemFontSize]];
	UIFont* styled = cocoaTouchTextTagBuildFont(tag, base);
	if (styled) attrs[NSFontAttributeName] = styled;

	const char* rise = iupAttribGet(tag, "RISE");
	if (rise)
	{
		UIFont* f = attrs[NSFontAttributeName];
		if (iupStrEqualNoCase(rise, "SUPERSCRIPT"))
		{
			if (f)
			{
				attrs[NSFontAttributeName] = [UIFont fontWithDescriptor:f.fontDescriptor size:f.pointSize * 0.6444444444444];
				attrs[NSBaselineOffsetAttributeName] = @(f.pointSize * 0.4);
			}
		}
		else if (iupStrEqualNoCase(rise, "SUBSCRIPT"))
		{
			if (f)
			{
				attrs[NSFontAttributeName] = [UIFont fontWithDescriptor:f.fontDescriptor size:f.pointSize * 0.6444444444444];
				attrs[NSBaselineOffsetAttributeName] = @(-f.pointSize * 0.2);
			}
		}
		else
		{
			int v = 0;
			iupStrToInt(rise, &v);
			attrs[NSBaselineOffsetAttributeName] = @((double)v);
		}
	}

	const char* fg = iupAttribGet(tag, "FGCOLOR");
	if (fg) { UIColor* c = iupCocoaTouchToNativeColor(fg); if (c) attrs[NSForegroundColorAttributeName] = c; }

	const char* bg = iupAttribGet(tag, "BGCOLOR");
	if (bg) { UIColor* c = iupCocoaTouchToNativeColor(bg); if (c) attrs[NSBackgroundColorAttributeName] = c; }

	const char* under = iupAttribGet(tag, "UNDERLINE");
	if (under)
	{
		int style = NSUnderlineStyleNone;
		if (iupStrEqualNoCase(under, "SINGLE"))      style = NSUnderlineStyleSingle;
		else if (iupStrEqualNoCase(under, "DOUBLE")) style = NSUnderlineStyleDouble;
		else if (iupStrEqualNoCase(under, "DOTTED") || iupStrEqualNoCase(under, "DOT"))
			style = NSUnderlineStyleSingle | NSUnderlinePatternDot;
		else if (iupStrBoolean(under))               style = NSUnderlineStyleSingle;
		attrs[NSUnderlineStyleAttributeName] = @(style);
	}

	const char* strike = iupAttribGet(tag, "STRIKEOUT");
	if (strike)
	{
		int style = iupStrBoolean(strike) ? NSUnderlineStyleSingle : NSUnderlineStyleNone;
		attrs[NSStrikethroughStyleAttributeName] = @(style);
	}

	const char* link = iupAttribGet(tag, "LINK");
	if (link)
	{
		NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:link]];
		if (url) attrs[NSLinkAttributeName] = url;
		if (!iupAttribGet(tag, "FGCOLOR")) attrs[NSForegroundColorAttributeName] = [UIColor systemBlueColor];
		if (!iupAttribGet(tag, "UNDERLINE")) attrs[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
	}

	NSParagraphStyle* ps = cocoaTouchTextTagBuildParagraph(tag);
	if (ps) attrs[NSParagraphStyleAttributeName] = ps;
}

static NSAttributedString* cocoaTouchTextBuildImageAttachment(Ihandle* ih, Ihandle* tag)
{
	const char* image_name = iupAttribGet(tag, "IMAGE");
	if (!image_name) return nil;
	UIImage* img = (UIImage*)iupImageGetImage(image_name, ih, 0, NULL);
	if (!img) return nil;
	int img_w = 0, img_h = 0, new_w = 0, new_h = 0;
	iupImageGetInfo(image_name, &img_w, &img_h, NULL);
	const char* aw = iupAttribGet(tag, "WIDTH");
	const char* ah = iupAttribGet(tag, "HEIGHT");
	if (aw) iupStrToInt(aw, &new_w);
	if (ah) iupStrToInt(ah, &new_h);
	NSTextAttachment* att = [[[NSTextAttachment alloc] init] autorelease];
	att.image = img;
	if ((new_w > 0 && new_w != img_w) || (new_h > 0 && new_h != img_h))
	{
		if (new_w <= 0) new_w = img_w;
		if (new_h <= 0) new_h = img_h;
		att.bounds = CGRectMake(0, 0, new_w, new_h);
	}
	return [NSAttributedString attributedStringWithAttachment:att];
}

static NSString* cocoaTouchTextRomanNumeral(int n, BOOL upper)
{
	if (n < 1) return @"";
	static const struct { int v; const char* sym; } roman[] = {
		{1000,"M"},{900,"CM"},{500,"D"},{400,"CD"},{100,"C"},{90,"XC"},
		{50,"L"},{40,"XL"},{10,"X"},{9,"IX"},{5,"V"},{4,"IV"},{1,"I"}
	};
	NSMutableString* s = [NSMutableString string];
	for (int i = 0; i < 13; i++)
	{
		while (n >= roman[i].v) { [s appendFormat:@"%s", roman[i].sym]; n -= roman[i].v; }
	}
	return upper ? s : [s lowercaseString];
}

static NSString* cocoaTouchTextBuildNumberingMarker(const char* numbering, const char* style, int ordinal)
{
	BOOL is_bullet = NO;
	NSString* core = nil;
	if      (iupStrEqualNoCase(numbering, "BULLET"))   { is_bullet = YES; core = @"•"; }
	else if (iupStrEqualNoCase(numbering, "ARABIC"))   core = [NSString stringWithFormat:@"%d", ordinal];
	else if (iupStrEqualNoCase(numbering, "LCLETTER")) core = [NSString stringWithFormat:@"%c", 'a' + ((ordinal - 1) % 26)];
	else if (iupStrEqualNoCase(numbering, "UCLETTER")) core = [NSString stringWithFormat:@"%c", 'A' + ((ordinal - 1) % 26)];
	else if (iupStrEqualNoCase(numbering, "LCROMAN"))  core = cocoaTouchTextRomanNumeral(ordinal, NO);
	else if (iupStrEqualNoCase(numbering, "UCROMAN"))  core = cocoaTouchTextRomanNumeral(ordinal, YES);
	if (!core) return @"";
	if (is_bullet) return @"• ";
	if (style && iupStrEqualNoCase(style, "NONUMBER")) return @"";
	if (style && iupStrEqualNoCase(style, "RIGHTPARENTHESIS")) return [NSString stringWithFormat:@"%@) ", core];
	if (style && iupStrEqualNoCase(style, "PARENTHESES"))      return [NSString stringWithFormat:@"(%@) ", core];
	if (style && iupStrEqualNoCase(style, "PERIOD"))           return [NSString stringWithFormat:@"%@. ", core];
	return [NSString stringWithFormat:@"%@ ", core];
}

IUP_SDK_API void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
	UITextView* tv = cocoaTouchTextView(ih);
	if (!tv) return;

	NSMutableAttributedString* attr_string = (NSMutableAttributedString*)iupAttribGet(ih, "_IUPCOCOATOUCH_BULK_ATTR");
	BOOL is_bulk_path = (bulk && attr_string);
	if (!is_bulk_path)
	{
		attr_string = [[[NSMutableAttributedString alloc] initWithAttributedString:tv.attributedText] autorelease];
	}

	NSUInteger text_len = attr_string.length;
	NSRange range = cocoaTouchTextResolveFormatRange(ih, formattag, text_len);
	if (range.length == 0) return;
	if (range.location + range.length > text_len) return;

	if (iupAttribGet(formattag, "IMAGE"))
	{
		NSAttributedString* img_attr = cocoaTouchTextBuildImageAttachment(ih, formattag);
		if (img_attr)
		{
			[attr_string replaceCharactersInRange:range withAttributedString:img_attr];
			if (!is_bulk_path) tv.attributedText = attr_string;
		}
		return;
	}

	NSMutableDictionary* patch = [NSMutableDictionary dictionary];
	if (range.length > 0 && range.location < text_len)
	{
		NSDictionary* existing = [attr_string attributesAtIndex:range.location effectiveRange:NULL];
		UIFont* existing_font = existing[NSFontAttributeName];
		if (existing_font) patch[NSFontAttributeName] = existing_font;
	}
	cocoaTouchTextApplyTagToAttributes(formattag, patch);
	[attr_string addAttributes:patch range:range];

	const char* numbering = iupAttribGet(formattag, "NUMBERING");
	if (numbering && !iupStrEqualNoCase(numbering, "NONE"))
	{
		const char* num_style = iupAttribGet(formattag, "NUMBERINGSTYLE");
		int ordinal = 1;
		NSUInteger pos = range.location;
		NSUInteger end = range.location + range.length;
		while (pos < end && pos < attr_string.length)
		{
			NSUInteger para_start = 0, para_end = 0, contents_end = 0;
			[[attr_string string] getParagraphStart:&para_start end:&para_end contentsEnd:&contents_end forRange:NSMakeRange(pos, 0)];
			NSString* marker = cocoaTouchTextBuildNumberingMarker(numbering, num_style, ordinal);
			if (marker.length > 0)
			{
				NSDictionary* attrs_at = [attr_string attributesAtIndex:para_start effectiveRange:NULL];
				NSAttributedString* marker_attr = [[[NSAttributedString alloc] initWithString:marker attributes:attrs_at] autorelease];
				[attr_string insertAttributedString:marker_attr atIndex:para_start];
				end += marker.length;
				pos = para_end + marker.length;
			}
			else pos = para_end;
			ordinal++;
		}
	}

	if (!is_bulk_path)
	{
		tv.attributedText = attr_string;
	}
}

static int cocoaTouchTextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	UITextView* tv = cocoaTouchTextView(ih);
	if (!tv) return 0;

	UIFont* base_font = tv.font ? tv.font : [UIFont systemFontOfSize:[UIFont systemFontSize]];
	UIColor* base_color = tv.textColor ? tv.textColor : [UIColor labelColor];
	NSDictionary* attrs = @{
		NSFontAttributeName: base_font,
		NSForegroundColorAttributeName: base_color
	};

	NSMutableAttributedString* bulk = (NSMutableAttributedString*)iupAttribGet(ih, "_IUPCOCOATOUCH_BULK_ATTR");
	NSString* plain = bulk ? bulk.string : (tv.text ?: @"");
	NSAttributedString* reset = [[[NSAttributedString alloc] initWithString:plain attributes:attrs] autorelease];
	if (bulk)
		[bulk setAttributedString:reset];
	else
		tv.attributedText = reset;
	return 0;
}

static NSString* cocoaTouchTextApplyFilter(Ihandle* ih, NSString* text)
{
	if (!text) return text;
	const char* filter = iupAttribGet(ih, "FILTER");
	if (!filter) return text;
	if (iupStrEqualNoCase(filter, "UPPERCASE")) return [text uppercaseString];
	if (iupStrEqualNoCase(filter, "LOWERCASE")) return [text lowercaseString];
	return text;
}

static NSRange cocoaTouchOverwriteRange(Ihandle* ih, NSString* current, NSRange range, NSString* replacement)
{
	if (!iupAttribGetBoolean(ih, "OVERWRITE")) return range;
	if (range.length != 0) return range;
	if (replacement.length != 1) return range;
	unichar ch = [replacement characterAtIndex:0];
	if (ch == '\n' || ch == '\r') return range;
	if (range.location >= current.length) return range;
	unichar next = [current characterAtIndex:range.location];
	if (next == '\n' || next == '\r') return range;
	return NSMakeRange(range.location, 1);
}

/* returns the replacement to actually apply (NC+MASK filtered), or nil to block */
static NSString* cocoaTouchTextValidateEdit(Ihandle* ih, NSString* current, NSRange range, NSString* replacement)
{
	const char* filter = iupAttribGet(ih, "FILTER");
	if (filter && replacement.length > 0 && iupStrEqualNoCase(filter, "NUMBER"))
	{
		NSCharacterSet* non_digit = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
		if ([replacement rangeOfCharacterFromSet:non_digit].location != NSNotFound)
			return nil;
	}

	NSString* filtered = cocoaTouchTextApplyFilter(ih, replacement);

	if (ih->data->nc > 0)
	{
		NSUInteger new_len = current.length - range.length + filtered.length;
		if ((int)new_len > ih->data->nc) return nil;
	}

	if (ih->data->mask)
	{
		NSString* proposed = [current stringByReplacingCharactersInRange:range withString:filtered];
		if (!iupMaskCheck(ih->data->mask, [proposed UTF8String]))
			return nil;
	}
	return filtered;
}

@implementation IupCocoaTouchTextDelegate

- (BOOL)textField:(UITextField*)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString*)replacement
{
	if (!_ihandle) return YES;

	NSString* current = textField.text ? textField.text : @"";
	NSRange effective = cocoaTouchOverwriteRange(_ihandle, current, range, replacement);
	NSString* applied = cocoaTouchTextValidateEdit(_ihandle, current, effective, replacement);
	if (!applied) return NO;

	if (effective.length != range.length || ![applied isEqualToString:replacement])
	{
		NSString* new_text = [current stringByReplacingCharactersInRange:effective withString:applied];
		textField.text = new_text;
		cocoaTouchTextFieldSetSelection(textField, effective.location + applied.length, effective.location + applied.length);
		return NO;
	}

	IFnis cb = (IFnis)IupGetCallback(_ihandle, "ACTION");
	if (cb)
	{
		const char* applied_utf8 = [applied UTF8String];
		int ret = iupEditCallActionCb(_ihandle, cb, applied_utf8,
			(int)range.location, (int)(range.location + range.length),
			_ihandle->data->mask, _ihandle->data->nc, 0, 1);
		if (ret == 0) return NO;
		if (ret != -1 && applied_utf8 && applied_utf8[0])
		{
			char rep[2] = { (char)ret, 0 };
			NSString* repstr = [NSString stringWithUTF8String:rep];
			NSString* new_text = [current stringByReplacingCharactersInRange:range withString:repstr];
			textField.text = new_text;
			cocoaTouchTextFieldSetSelection(textField, range.location + 1, range.location + 1);
			return NO;
		}
	}
	return YES;
}

- (void)textFieldDidBeginEditing:(UITextField*)textField
{
	(void)textField;
	if (_ihandle) iupCallGetFocusCb(_ihandle);
}

/* soft-keyboard Return -> K_ANY chain, then DEFAULTENTER if unclaimed */
- (BOOL)textFieldShouldReturn:(UITextField*)textField
{
	(void)textField;
	if (!_ihandle) return YES;
	int consumed = 0;
	int ret = iupKeyCallKeyCb(_ihandle, K_CR);
	if (ret == IUP_CLOSE) IupExitLoop();
	if (ret == IUP_IGNORE) consumed = 1;
	if (!consumed) iupKeyProcessNavigation(_ihandle, K_CR, 0);
	return YES;
}

- (void)textFieldDidEndEditing:(UITextField*)textField
{
	(void)textField;
	if (!_ihandle) return;
	iupCallKillFocusCb(_ihandle);
}

- (void)onFieldEditingChanged:(UITextField*)textField
{
	(void)textField;
	if (!_ihandle) return;
	IFn cb = (IFn)IupGetCallback(_ihandle, "VALUECHANGED_CB");
	if (cb && cb(_ihandle) == IUP_CLOSE) IupExitLoop();
}

- (void)textFieldDidChangeSelection:(UITextField*)textField
{
	if (!_ihandle) return;
	IFnii cb = (IFnii)IupGetCallback(_ihandle, "CARET_CB");
	if (!cb) return;
	NSRange sel = cocoaTouchTextFieldSelection(textField);
	int pos = (int)sel.location;
	int lin, col;
	iupdrvTextConvertPosToLinCol(_ihandle, pos, &lin, &col);
	if (cb(_ihandle, lin, col) == IUP_CLOSE) IupExitLoop();
}

- (BOOL)textView:(UITextView*)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString*)replacement
{
	if (!_ihandle) return YES;
	NSString* current = textView.text ? textView.text : @"";
	NSRange effective = cocoaTouchOverwriteRange(_ihandle, current, range, replacement);
	NSString* applied = cocoaTouchTextValidateEdit(_ihandle, current, effective, replacement);
	if (!applied) return NO;

	if (effective.length != range.length || ![applied isEqualToString:replacement])
	{
		NSString* new_text = [current stringByReplacingCharactersInRange:effective withString:applied];
		textView.text = new_text;
		[textView setSelectedRange:NSMakeRange(effective.location + applied.length, 0)];
		return NO;
	}

	IFnis cb = (IFnis)IupGetCallback(_ihandle, "ACTION");
	if (cb)
	{
		const char* applied_utf8 = [applied UTF8String];
		int ret = iupEditCallActionCb(_ihandle, cb, applied_utf8,
			(int)range.location, (int)(range.location + range.length),
			_ihandle->data->mask, _ihandle->data->nc, 0, 1);
		if (ret == 0) return NO;
		if (ret != -1 && applied_utf8 && applied_utf8[0])
		{
			char rep[2] = { (char)ret, 0 };
			NSString* repstr = [NSString stringWithUTF8String:rep];
			NSString* new_text = [current stringByReplacingCharactersInRange:range withString:repstr];
			textView.text = new_text;
			[textView setSelectedRange:NSMakeRange(range.location + 1, 0)];
			return NO;
		}
	}
	return YES;
}

- (void)textViewDidBeginEditing:(UITextView*)textView
{
	(void)textView;
	if (_ihandle) iupCallGetFocusCb(_ihandle);
}

- (void)textViewDidEndEditing:(UITextView*)textView
{
	(void)textView;
	if (!_ihandle) return;
	iupCallKillFocusCb(_ihandle);
}

- (void)textViewDidChange:(UITextView*)textView
{
	(void)textView;
	if (!_ihandle) return;
	IFn cb = (IFn)IupGetCallback(_ihandle, "VALUECHANGED_CB");
	if (cb && cb(_ihandle) == IUP_CLOSE) IupExitLoop();
}

- (void)textViewDidChangeSelection:(UITextView*)textView
{
	if (!_ihandle) return;
	IFnii cb = (IFnii)IupGetCallback(_ihandle, "CARET_CB");
	if (!cb) return;
	NSRange sel = textView.selectedRange;
	int lin, col;
	iupdrvTextConvertPosToLinCol(_ihandle, (int)sel.location, &lin, &col);
	if (cb(_ihandle, lin, col) == IUP_CLOSE) IupExitLoop();
}

- (void)onStepperChanged:(UIStepper*)stepper
{
	if (!_ihandle) return;
	int v = (int)stepper.value;

	if (iupAttribGetBoolean(_ihandle, "SPINAUTO"))
	{
		UITextField* field = cocoaTouchTextField(_ihandle);
		if (field) [field setText:[NSString stringWithFormat:@"%d", v]];
	}

	IFni cb = (IFni)IupGetCallback(_ihandle, "SPIN_CB");
	if (cb && cb(_ihandle, v) == IUP_CLOSE) IupExitLoop();
}

- (BOOL)textView:(UITextView*)textView shouldInteractWithURL:(NSURL*)url inRange:(NSRange)range interaction:(UITextItemInteraction)interaction
{
	(void)textView; (void)range; (void)interaction;
	if (!_ihandle) return YES;
	IFns cb = (IFns)IupGetCallback(_ihandle, "LINK_CB");
	if (cb && cb(_ihandle, (char*)[url.absoluteString UTF8String]) == IUP_CLOSE) IupExitLoop();
	return NO;
}

/* editable UITextView eats taps for caret placement; tap gesture fires LINK_CB on link ranges */
- (void)onTextViewTap:(UITapGestureRecognizer*)gr
{
	if (!_ihandle) return;
	UITextView* tv = (UITextView*)gr.view;
	CGPoint pt = [gr locationInView:tv];
	pt.x -= tv.textContainerInset.left;
	pt.y -= tv.textContainerInset.top;
	NSUInteger idx = [tv.layoutManager characterIndexForPoint:pt
	                                          inTextContainer:tv.textContainer
	                 fractionOfDistanceBetweenInsertionPoints:NULL];
	if (idx >= tv.attributedText.length) return;
	NSURL* url = [tv.attributedText attribute:NSLinkAttributeName atIndex:idx effectiveRange:NULL];
	if (!url) return;
	const char* url_c = [url.absoluteString UTF8String];
	IFns cb = (IFns)IupGetCallback(_ihandle, "LINK_CB");
	int rc = IUP_DEFAULT;
	if (cb) rc = cb(_ihandle, (char*)url_c);
	if (rc == IUP_CLOSE) IupExitLoop();
	else if (rc == IUP_DEFAULT) IupHelp(url_c);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gr shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)other
{
	(void)gr; (void)other;
	return YES;
}

@end


static IupCocoaTouchTextDelegate* cocoaTouchTextGetDelegate(id view)
{
	return (IupCocoaTouchTextDelegate*)objc_getAssociatedObject(view, IUPCOCOATOUCH_TEXT_DELEGATE_KEY);
}

static int cocoaTouchTextSetValueAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchSpinView* spin = cocoaTouchTextSpin(ih);
	if (spin && iupAttribGetBoolean(ih, "SPINAUTO"))
	{
		int v = 0;
		iupStrToInt(value, &v);
		spin.stepper.value = (double)v;
		spin.field.text = [NSString stringWithFormat:@"%d", (int)spin.stepper.value];
		return 0;
	}
	NSString* s = value ? [NSString stringWithUTF8String:value] : @"";
	cocoaTouchTextSetString(ih, s);
	return 0;
}

static char* cocoaTouchTextGetValueAttrib(Ihandle* ih)
{
	NSString* s = cocoaTouchTextGetString(ih);
	return s ? iupStrReturnStr([s UTF8String]) : "";
}

static int cocoaTouchTextSetCueBannerAttrib(Ihandle* ih, const char* value)
{
	UITextField* f = cocoaTouchTextField(ih);
	if (!f) return 0;
	f.placeholder = value ? [NSString stringWithUTF8String:value] : @"";
	return 1;
}

static int cocoaTouchTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
	BOOL read_only = iupStrBoolean(value) ? YES : NO;
	UITextField* f = cocoaTouchTextField(ih);
	if (f) { f.enabled = !read_only; return 0; }
	UITextView* v = cocoaTouchTextView(ih);
	if (v) v.editable = !read_only;
	return 0;
}

static char* cocoaTouchTextGetReadOnlyAttrib(Ihandle* ih)
{
	UITextField* f = cocoaTouchTextField(ih);
	if (f) return iupStrReturnBoolean(!f.enabled);
	UITextView* v = cocoaTouchTextView(ih);
	if (v) return iupStrReturnBoolean(!v.editable);
	return "NO";
}

static int cocoaTouchTextSetPasswordAttrib(Ihandle* ih, const char* value)
{
	UITextField* f = cocoaTouchTextField(ih);
	if (!f) return 0;
	f.secureTextEntry = iupStrBoolean(value) ? YES : NO;
	return 1;
}

static int cocoaTouchTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
	NSTextAlignment a = NSTextAlignmentLeft;
	if (iupStrEqualNoCase(value, "ARIGHT") || iupStrEqualNoCase(value, "RIGHT"))  a = NSTextAlignmentRight;
	else if (iupStrEqualNoCase(value, "ACENTER") || iupStrEqualNoCase(value, "CENTER")) a = NSTextAlignmentCenter;

	UITextField* f = cocoaTouchTextField(ih);
	if (f) { f.textAlignment = a; return 1; }
	UITextView* v = cocoaTouchTextView(ih);
	if (v) v.textAlignment = a;
	return 1;
}

static int cocoaTouchTextSetBorderAttrib(Ihandle* ih, const char* value)
{
	BOOL on = iupStrBoolean(value) ? YES : NO;
	UITextField* f = cocoaTouchTextField(ih);
	if (f)
	{
		f.borderStyle = on ? UITextBorderStyleRoundedRect : UITextBorderStyleNone;
		return 1;
	}
	UITextView* v = cocoaTouchTextView(ih);
	if (v)
	{
		v.layer.borderWidth = on ? 1.0 : 0.0;
		v.layer.borderColor = on ? [UIColor separatorColor].CGColor : nil;
		v.layer.cornerRadius = on ? 5.0 : 0.0;
		if (on)
		{
			iupCocoaTouchRegisterThemeRefresh(v, ^(UIView* view) {
				view.layer.borderColor = [UIColor separatorColor].CGColor;
			});
		}
	}
	return 1;
}

static int cocoaTouchTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
	int hp = 0, vp = 0;
	iupStrToIntInt(value, &hp, &vp, 'x');
	ih->data->horiz_padding = hp;
	ih->data->vert_padding  = vp;

	UITextView* v = cocoaTouchTextView(ih);
	if (v) v.textContainerInset = UIEdgeInsetsMake(vp, hp, vp, hp);
	return 0;
}

static int cocoaTouchTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
	UITextView* v = cocoaTouchTextView(ih);
	if (!v) return 0;
	int tabs = 0;
	if (!iupStrToInt(value, &tabs) || tabs <= 0) return 0;

	UIFont* font = v.font;
	if (!font) return 0;
	CGFloat space_width = [@" " sizeWithAttributes:@{NSFontAttributeName: font}].width;

	NSMutableParagraphStyle* ps = [[[NSMutableParagraphStyle alloc] init] autorelease];
	ps.defaultTabInterval = space_width * tabs;
	ps.tabStops = @[];

	NSMutableDictionary* attrs = [[v.typingAttributes mutableCopy] autorelease];
	attrs[NSParagraphStyleAttributeName] = ps;
	v.typingAttributes = attrs;
	return 1;
}

static char* cocoaTouchTextGetCountAttrib(Ihandle* ih)
{
	NSString* s = cocoaTouchTextGetString(ih);
	return iupStrReturnInt((int)s.length);
}

static char* cocoaTouchTextGetLineCountAttrib(Ihandle* ih)
{
	if (!ih->data->is_multiline) return "1";
	NSString* s = cocoaTouchTextGetString(ih);
	NSUInteger len = s.length;
	int count = 1;
	for (NSUInteger i = 0; i < len; i++)
	{
		if ([s characterAtIndex:i] == '\n') count++;
	}
	return iupStrReturnInt(count);
}

static char* cocoaTouchTextGetLineValueAttrib(Ihandle* ih)
{
	NSString* s = cocoaTouchTextGetString(ih);
	if (!ih->data->is_multiline) return iupStrReturnStr([s UTF8String]);
	NSRange r = cocoaTouchTextSelection(ih);
	NSUInteger caret = r.location;
	if (caret > s.length) caret = s.length;
	NSRange line = [s lineRangeForRange:NSMakeRange(caret, 0)];
	if (line.length > 0 && [s characterAtIndex:line.location + line.length - 1] == '\n')
		line.length--;
	NSString* sub = [s substringWithRange:line];
	return iupStrReturnStr([sub UTF8String]);
}

static char* cocoaTouchTextGetSelectedTextAttrib(Ihandle* ih)
{
	NSRange r = cocoaTouchTextSelection(ih);
	if (r.length == 0) return NULL;
	NSString* s = cocoaTouchTextGetString(ih);
	if (r.location + r.length > s.length) return NULL;
	NSString* sub = [s substringWithRange:r];
	return iupStrReturnStr([sub UTF8String]);
}

static int cocoaTouchTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
	NSRange r = cocoaTouchTextSelection(ih);
	NSString* current = cocoaTouchTextGetString(ih);
	NSString* replacement = value ? [NSString stringWithUTF8String:value] : @"";
	if (r.location > current.length) r.location = current.length;
	if (r.location + r.length > current.length) r.length = current.length - r.location;

	NSString* new_text = [current stringByReplacingCharactersInRange:r withString:replacement];
	cocoaTouchTextSetString(ih, new_text);
	cocoaTouchTextSetSelection(ih, r.location + replacement.length, r.location + replacement.length);
	return 0;
}

static char* cocoaTouchTextGetScrollVisibleAttrib(Ihandle* ih)
{
	if (!ih->data->is_multiline) return "NO";
	UITextView* v = cocoaTouchTextView(ih);
	if (!v) return "NO";
	if (!v.scrollEnabled) return "NO";
	CGSize content = v.contentSize;
	CGSize frame = v.bounds.size;
	int sb_h = 0, sb_v = 0;
	if (v.showsHorizontalScrollIndicator && content.width  > frame.width)  sb_h = 1;
	if (v.showsVerticalScrollIndicator   && content.height > frame.height) sb_v = 1;
	if (sb_h && sb_v) return "YES";
	if (sb_h) return "HORIZONTAL";
	if (sb_v) return "VERTICAL";
	return "NO";
}

static char* cocoaTouchTextGetSelectionAttrib(Ihandle* ih)
{
	NSRange r = cocoaTouchTextSelection(ih);
	if (r.length == 0) return NULL;
	if (ih->data->is_multiline)
	{
		int lin1, col1, lin2, col2;
		iupdrvTextConvertPosToLinCol(ih, (int)r.location, &lin1, &col1);
		iupdrvTextConvertPosToLinCol(ih, (int)(r.location + r.length), &lin2, &col2);
		return iupStrReturnStrf("%d,%d:%d,%d", lin1, col1, lin2, col2);
	}
	return iupStrReturnIntInt((int)r.location + 1, (int)(r.location + r.length) + 1, ':');
}

static int cocoaTouchTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
	if (!value || iupStrEqualNoCase(value, "NONE"))
	{
		NSUInteger len = cocoaTouchTextGetString(ih).length;
		cocoaTouchTextSetSelection(ih, len, len);
		return 0;
	}
	if (iupStrEqualNoCase(value, "ALL"))
	{
		NSUInteger len = cocoaTouchTextGetString(ih).length;
		cocoaTouchTextSetSelection(ih, 0, len);
		return 0;
	}

	int s = 0, e = 0;
	if (ih->data->is_multiline)
	{
		int lin1 = 1, col1 = 1, lin2 = 1, col2 = 1;
		if (sscanf(value, "%d,%d:%d,%d", &lin1, &col1, &lin2, &col2) != 4) return 0;
		iupdrvTextConvertLinColToPos(ih, lin1, col1, &s);
		iupdrvTextConvertLinColToPos(ih, lin2, col2, &e);
	}
	else
	{
		if (iupStrToIntInt(value, &s, &e, ':') != 2) return 0;
		s--; e--;  /* 1-based → 0-based */
	}
	if (s < 0) s = 0;
	if (e < s) e = s;
	cocoaTouchTextSetSelection(ih, (NSUInteger)s, (NSUInteger)e);
	return 0;
}

static char* cocoaTouchTextGetSelectionPosAttrib(Ihandle* ih)
{
	NSRange r = cocoaTouchTextSelection(ih);
	if (r.length == 0) return NULL;
	return iupStrReturnIntInt((int)r.location, (int)(r.location + r.length), ':');
}

static int cocoaTouchTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
	if (!value || iupStrEqualNoCase(value, "NONE"))
	{
		NSUInteger len = cocoaTouchTextGetString(ih).length;
		cocoaTouchTextSetSelection(ih, len, len);
		return 0;
	}
	if (iupStrEqualNoCase(value, "ALL"))
	{
		NSUInteger len = cocoaTouchTextGetString(ih).length;
		cocoaTouchTextSetSelection(ih, 0, len);
		return 0;
	}
	int s = 0, e = 0;
	if (iupStrToIntInt(value, &s, &e, ':') != 2) return 0;
	if (s < 0) s = 0;
	if (e < s) e = s;
	cocoaTouchTextSetSelection(ih, (NSUInteger)s, (NSUInteger)e);
	return 0;
}

static char* cocoaTouchTextGetCaretAttrib(Ihandle* ih)
{
	NSRange r = cocoaTouchTextSelection(ih);
	int pos = (int)r.location;
	if (ih->data->is_multiline)
	{
		int lin, col;
		iupdrvTextConvertPosToLinCol(ih, pos, &lin, &col);
		return iupStrReturnIntInt(lin, col, ',');
	}
	return iupStrReturnInt(pos + 1);
}

static int cocoaTouchTextSetCaretAttrib(Ihandle* ih, const char* value)
{
	if (!value) return 0;
	int pos = 0;
	if (ih->data->is_multiline)
	{
		int lin = 1, col = 1;
		if (sscanf(value, "%d,%d", &lin, &col) < 1) return 0;
		iupdrvTextConvertLinColToPos(ih, lin, col, &pos);
	}
	else
	{
		if (!iupStrToInt(value, &pos)) return 0;
		pos--;
		if (pos < 0) pos = 0;
	}
	cocoaTouchTextSetSelection(ih, (NSUInteger)pos, (NSUInteger)pos);
	return 0;
}

static char* cocoaTouchTextGetCaretPosAttrib(Ihandle* ih)
{
	NSRange r = cocoaTouchTextSelection(ih);
	return iupStrReturnInt((int)r.location);
}

static int cocoaTouchTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
	int pos = 0;
	if (!value || !iupStrToInt(value, &pos)) return 0;
	if (pos < 0) pos = 0;
	cocoaTouchTextSetSelection(ih, (NSUInteger)pos, (NSUInteger)pos);
	return 0;
}

static int cocoaTouchTextSetInsertAttrib(Ihandle* ih, const char* value)
{
	if (!value) return 0;
	NSRange r = cocoaTouchTextSelection(ih);
	NSString* ins = [NSString stringWithUTF8String:value];
	NSString* current = cocoaTouchTextGetString(ih);
	NSString* new_text = [current stringByReplacingCharactersInRange:r withString:ins];
	cocoaTouchTextSetString(ih, new_text);
	NSUInteger new_caret = r.location + ins.length;
	cocoaTouchTextSetSelection(ih, new_caret, new_caret);
	return 0;
}

static int cocoaTouchTextSetAppendAttrib(Ihandle* ih, const char* value)
{
	if (!value) return 0;
	NSString* current = cocoaTouchTextGetString(ih);
	NSString* append = [NSString stringWithUTF8String:value];
	NSString* sep = @"";
	if (current.length > 0 && ih->data->is_multiline && ih->data->append_newline)
		sep = @"\n";
	NSString* new_text = [[current stringByAppendingString:sep] stringByAppendingString:append];
	cocoaTouchTextSetString(ih, new_text);

	UITextView* v = cocoaTouchTextView(ih);
	if (v) [v scrollRangeToVisible:NSMakeRange(new_text.length, 0)];
	return 0;
}

static int cocoaTouchTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
	if (!value) return 0;
	int pos = 0;
	if (ih->data->is_multiline)
	{
		int lin = 1, col = 1;
		if (sscanf(value, "%d,%d", &lin, &col) >= 1)
			iupdrvTextConvertLinColToPos(ih, lin, col, &pos);
	}
	else
	{
		iupStrToInt(value, &pos);
		pos--;
		if (pos < 0) pos = 0;
	}
	UITextView* v = cocoaTouchTextView(ih);
	if (v)
	{
		[v scrollRangeToVisible:NSMakeRange((NSUInteger)pos, 0)];
		return 0;
	}
	UITextField* f = cocoaTouchTextField(ih);
	if (f) cocoaTouchTextFieldSetSelection(f, (NSUInteger)pos, (NSUInteger)pos);
	return 0;
}

static int cocoaTouchTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
	int pos = 0;
	if (!value || !iupStrToInt(value, &pos)) return 0;
	if (pos < 0) pos = 0;
	UITextView* v = cocoaTouchTextView(ih);
	if (v) { [v scrollRangeToVisible:NSMakeRange((NSUInteger)pos, 0)]; return 0; }
	UITextField* f = cocoaTouchTextField(ih);
	if (f) cocoaTouchTextFieldSetSelection(f, (NSUInteger)pos, (NSUInteger)pos);
	return 0;
}

static int cocoaTouchTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
	UIPasteboard* pb = [UIPasteboard generalPasteboard];
	if (iupStrEqualNoCase(value, "COPY"))
	{
		char* sel = cocoaTouchTextGetSelectedTextAttrib(ih);
		if (sel) [pb setString:[NSString stringWithUTF8String:sel]];
	}
	else if (iupStrEqualNoCase(value, "CUT"))
	{
		char* sel = cocoaTouchTextGetSelectedTextAttrib(ih);
		if (sel)
		{
			[pb setString:[NSString stringWithUTF8String:sel]];
			cocoaTouchTextSetSelectedTextAttrib(ih, "");
		}
	}
	else if (iupStrEqualNoCase(value, "PASTE"))
	{
		NSString* clip = [pb string];
		if (clip) cocoaTouchTextSetSelectedTextAttrib(ih, [clip UTF8String]);
	}
	else if (iupStrEqualNoCase(value, "CLEAR"))
	{
		cocoaTouchTextSetSelectedTextAttrib(ih, "");
	}
	else if (iupStrEqualNoCase(value, "UNDO") || iupStrEqualNoCase(value, "REDO") || iupStrEqualNoCase(value, "CLEARUNDO"))
	{
		UIResponder* responder = (UIResponder*)cocoaTouchTextView(ih);
		if (!responder) responder = (UIResponder*)cocoaTouchTextField(ih);
		NSUndoManager* mgr = responder ? responder.undoManager : nil;
		if (!mgr) return 0;
		if (iupStrEqualNoCase(value, "UNDO"))
		{
			if ([mgr canUndo]) [mgr undo];
		}
		else if (iupStrEqualNoCase(value, "REDO"))
		{
			if ([mgr canRedo]) [mgr redo];
		}
		else
		{
			[mgr removeAllActions];
		}
	}
	return 0;
}

static int cocoaTouchTextSetFilterAttrib(Ihandle* ih, const char* value)
{
	UITextField* f = cocoaTouchTextField(ih);
	if (f && value && iupStrEqualNoCase(value, "NUMBER"))
	{
		f.keyboardType = UIKeyboardTypeNumbersAndPunctuation;
	}
	return 1;
}

static int cocoaTouchTextSetNCAttrib(Ihandle* ih, const char* value)
{
	int nc = 0;
	if (value) iupStrToInt(value, &nc);
	if (nc < 0) nc = 0;
	ih->data->nc = nc;
	return 1;
}

static int cocoaTouchTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchSpinView* spin = cocoaTouchTextSpin(ih);
	if (!spin || !value) return 0;
	int v = 0;
	iupStrToInt(value, &v);
	spin.stepper.value = (double)v;
	if (iupAttribGetBoolean(ih, "SPINAUTO"))
	{
		spin.field.text = [NSString stringWithFormat:@"%d", v];
	}
	return 0;
}

static char* cocoaTouchTextGetSpinValueAttrib(Ihandle* ih)
{
	IupCocoaTouchSpinView* spin = cocoaTouchTextSpin(ih);
	if (!spin) return NULL;
	return iupStrReturnInt((int)spin.stepper.value);
}

static int cocoaTouchTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchSpinView* spin = cocoaTouchTextSpin(ih);
	if (!spin) return 1;
	int v = 0;
	if (value) iupStrToInt(value, &v);
	spin.stepper.minimumValue = (double)v;
	return 1;
}

static int cocoaTouchTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchSpinView* spin = cocoaTouchTextSpin(ih);
	if (!spin) return 1;
	int v = 100;
	if (value) iupStrToInt(value, &v);
	spin.stepper.maximumValue = (double)v;
	return 1;
}

static int cocoaTouchTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchSpinView* spin = cocoaTouchTextSpin(ih);
	if (!spin) return 1;
	int v = 1;
	if (value) iupStrToInt(value, &v);
	if (v < 1) v = 1;
	spin.stepper.stepValue = (double)v;
	return 1;
}

static int cocoaTouchTextSetSpinWrapAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchSpinView* spin = cocoaTouchTextSpin(ih);
	if (!spin) return 1;
	spin.stepper.wraps = iupStrBoolean(value) ? YES : NO;
	return 1;
}

/* --- map / unmap ------------------------------------------------------ */

static void cocoaTouchTextWireFieldDelegate(UITextField* field, Ihandle* ih)
{
	IupCocoaTouchTextDelegate* delegate = [[IupCocoaTouchTextDelegate alloc] init];
	delegate.ihandle = ih;
	field.delegate = delegate;
	[field addTarget:delegate action:@selector(onFieldEditingChanged:) forControlEvents:UIControlEventEditingChanged];
	objc_setAssociatedObject(field, IUPCOCOATOUCH_TEXT_DELEGATE_KEY, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[delegate release];
}

static void cocoaTouchTextWireViewDelegate(UITextView* view, Ihandle* ih)
{
	IupCocoaTouchTextDelegate* delegate = [[IupCocoaTouchTextDelegate alloc] init];
	delegate.ihandle = ih;
	view.delegate = delegate;
	objc_setAssociatedObject(view, IUPCOCOATOUCH_TEXT_DELEGATE_KEY, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	UITapGestureRecognizer* tap = [[UITapGestureRecognizer alloc] initWithTarget:delegate action:@selector(onTextViewTap:)];
	tap.delegate = delegate;
	[view addGestureRecognizer:tap];
	[tap release];
	[delegate release];
}

static int cocoaTouchTextMapMethod(Ihandle* ih)
{
	UIView* view = nil;

	if (ih->data->is_multiline)
	{
		IupCocoaTouchTextView* tv = [[IupCocoaTouchTextView alloc] initWithFrame:CGRectZero];
		tv.ihandle = ih;
		tv.font = [UIFont systemFontOfSize:[UIFont systemFontSize]];
		tv.textContainerInset = UIEdgeInsetsZero;
		ih->data->has_formatting = 1;
		if (iupAttribGetBoolean(ih, "WORDWRAP")) ih->data->sb &= ~IUP_SB_HORIZ;
		cocoaTouchTextWireViewDelegate(tv, ih);
		view = tv;
	}
	else if (iupAttribGetBoolean(ih, "SPIN"))
	{
		IupCocoaTouchSpinView* spin = [[IupCocoaTouchSpinView alloc] initWithFrame:CGRectZero ihandle:ih];
		ih->data->has_formatting = 0;
		cocoaTouchTextWireFieldDelegate(spin.field, ih);

		IupCocoaTouchTextDelegate* delegate = cocoaTouchTextGetDelegate(spin.field);
		[spin.stepper addTarget:delegate action:@selector(onStepperChanged:)
		          forControlEvents:UIControlEventValueChanged];

		if (!iupAttribGet(ih, "SPINAUTO")) iupAttribSet(ih, "SPINAUTO", "YES");

		int spin_min = 0, spin_max = 100, spin_inc = 1;
		iupStrToInt(iupAttribGetStr(ih, "SPINMIN"), &spin_min);
		iupStrToInt(iupAttribGetStr(ih, "SPINMAX"), &spin_max);
		iupStrToInt(iupAttribGetStr(ih, "SPININC"), &spin_inc);
		if (spin_max <= spin_min) spin_max = spin_min + 1;

		/* Expand-first ordering keeps UIStepper's min < max invariant satisfied. */
		if (spin_max > spin.stepper.maximumValue)
		{
			spin.stepper.maximumValue = (double)spin_max;
			spin.stepper.minimumValue = (double)spin_min;
		}
		else
		{
			spin.stepper.minimumValue = (double)spin_min;
			spin.stepper.maximumValue = (double)spin_max;
		}
		spin.stepper.stepValue = (double)(spin_inc > 0 ? spin_inc : 1);

		const char* val = iupAttribGet(ih, "VALUE");
		int v = spin_min;
		if (val && *val) iupStrToInt(val, &v);
		if (v < spin_min) v = spin_min;
		if (v > spin_max) v = spin_max;
		spin.stepper.value = (double)v;
		spin.field.text = [NSString stringWithFormat:@"%d", v];

		view = spin;
	}
	else
	{
		IupCocoaTouchTextField* field = [[IupCocoaTouchTextField alloc] initWithFrame:CGRectZero];
		field.ihandle = ih;
		field.borderStyle = UITextBorderStyleRoundedRect;
		if (iupAttribGetBoolean(ih, "PASSWORD")) [field setSecureTextEntry:YES];
		ih->data->has_formatting = 0;
		cocoaTouchTextWireFieldDelegate(field, ih);
		view = field;
	}

	ih->handle = view;
	iupCocoaTouchAddToParent(ih);

	if (ih->data->formattags) iupTextUpdateFormatTags(ih);

	return IUP_NOERROR;
}

IUP_SDK_API void iupdrvTextInitClass(Iclass* ic)
{
	ic->Map = cocoaTouchTextMapMethod;
	ic->UnMap = iupdrvBaseUnMapMethod;

	iupClassRegisterAttribute(ic, "VALUE", cocoaTouchTextGetValueAttrib, cocoaTouchTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "READONLY", cocoaTouchTextGetReadOnlyAttrib, cocoaTouchTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "CUEBANNER", NULL, cocoaTouchTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "PASSWORD", NULL, cocoaTouchTextSetPasswordAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, cocoaTouchTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "BORDER", NULL, cocoaTouchTextSetBorderAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, cocoaTouchTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TABSIZE", NULL, cocoaTouchTextSetTabSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, cocoaTouchTextSetNCAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FILTER", NULL, cocoaTouchTextSetFilterAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "COUNT", cocoaTouchTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "LINECOUNT", cocoaTouchTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SCROLLVISIBLE", cocoaTouchTextGetScrollVisibleAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "LINEVALUE", cocoaTouchTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "SELECTEDTEXT",cocoaTouchTextGetSelectedTextAttrib, cocoaTouchTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SELECTION", cocoaTouchTextGetSelectionAttrib, cocoaTouchTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SELECTIONPOS",cocoaTouchTextGetSelectionPosAttrib, cocoaTouchTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CARET", cocoaTouchTextGetCaretAttrib, cocoaTouchTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CARETPOS", cocoaTouchTextGetCaretPosAttrib, cocoaTouchTextSetCaretPosAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "INSERT", NULL, cocoaTouchTextSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "APPEND", NULL, cocoaTouchTextSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SCROLLTO", NULL, cocoaTouchTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, cocoaTouchTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, cocoaTouchTextSetClipboardAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "SPINVALUE", cocoaTouchTextGetSpinValueAttrib, cocoaTouchTextSetSpinValueAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SPINMIN", NULL, cocoaTouchTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SPINMAX", NULL, cocoaTouchTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SPININC", NULL, cocoaTouchTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SPINWRAP", NULL, cocoaTouchTextSetSpinWrapAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SPINAUTO", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SPINALIGN", NULL, NULL, IUPAF_SAMEASSYSTEM, "RIGHT", IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_IHANDLE|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, cocoaTouchTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

	/* UITextField / UITextView have no overwrite mode. */
	iupClassRegisterAttribute(ic, "OVERWRITE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
