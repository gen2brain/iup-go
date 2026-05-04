/** \file
 * \brief Frame Control (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_frame.h"

#include "iupcocoatouch_drv.h"


#define IUPCOCOATOUCH_FRAME_BORDER_WIDTH  1
#define IUPCOCOATOUCH_FRAME_CORNER_RADIUS 8.0
#define IUPCOCOATOUCH_FRAME_TITLE_INSET   0

@interface IupCocoaTouchFrameView : UIView
@property(nonatomic, retain) UILabel* titleLabel;
@property(nonatomic, retain) UIColor* strokeColor;
@property(nonatomic, retain) UIColor* fillColor;
@end

@implementation IupCocoaTouchFrameView

- (void)dealloc
{
	[_titleLabel release];
	[_strokeColor release];
	[_fillColor release];
	[super dealloc];
}

- (CGFloat)titleHeight
{
	if (!_titleLabel || _titleLabel.text.length == 0) return 0.0;
	return ceil([_titleLabel sizeThatFits:CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX)].height);
}

- (void)drawRect:(CGRect)rect
{
	(void)rect;
	CGFloat stroke = IUPCOCOATOUCH_FRAME_BORDER_WIDTH;
	CGFloat title_h = [self titleHeight];
	CGFloat half = stroke / 2.0;

	CGRect border = CGRectMake(half, title_h + half,
	                           self.bounds.size.width - stroke,
	                           self.bounds.size.height - title_h - stroke);
	UIBezierPath* path = [UIBezierPath bezierPathWithRoundedRect:border
	                                                cornerRadius:IUPCOCOATOUCH_FRAME_CORNER_RADIUS];
	if (_fillColor)
	{
		[_fillColor setFill];
		[path fill];
	}
	[(_strokeColor ?: [UIColor separatorColor]) setStroke];
	path.lineWidth = stroke;
	[path stroke];
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	if (!_titleLabel || _titleLabel.text.length == 0) return;
	CGSize size = [_titleLabel sizeThatFits:CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX)];
	_titleLabel.frame = CGRectMake(IUPCOCOATOUCH_FRAME_TITLE_INSET, 0,
	                               MIN(size.width, self.bounds.size.width - 2 * IUPCOCOATOUCH_FRAME_TITLE_INSET),
	                               size.height);
}

@end


static IupCocoaTouchFrameView* cocoaTouchFrameGet(Ihandle* ih)
{
	if ([(id)ih->handle isKindOfClass:[IupCocoaTouchFrameView class]])
		return (IupCocoaTouchFrameView*)ih->handle;
	return nil;
}

/* subtle stroke that follows the theme: label color at 25% alpha */
static UIColor* cocoaTouchFrameDefaultStrokeColor(void)
{
	return [[UIColor labelColor] colorWithAlphaComponent:0.25];
}

static UIColor* cocoaTouchFrameDefaultTitleColor(void)
{
	return [UIColor labelColor];
}

/* default to Footnote-sized (~13pt) title unless FONT is explicitly set */
static UIFont* cocoaTouchFrameTitleFont(Ihandle* ih, IupCocoaTouchFont* font)
{
	if (!font || !font.nativeFont) return nil;
	UIFont* native = font.nativeFont;
	const char* current = iupGetFontValue(ih);
	const char* defaults = IupGetGlobal("DEFAULTFONT");
	if (!current || !defaults || !iupStrEqual(current, defaults)) return native;
	return [native fontWithSize:MAX(11.0, native.pointSize - 4.0)];
}

static void cocoaTouchFrameApplyTitle(Ihandle* ih, IupCocoaTouchFrameView* frame, NSString* title)
{
	if (!frame.titleLabel)
	{
		frame.titleLabel = [[[UILabel alloc] initWithFrame:CGRectZero] autorelease];
		frame.titleLabel.backgroundColor = [UIColor clearColor];
		frame.titleLabel.textColor = cocoaTouchFrameDefaultTitleColor();
		[frame addSubview:frame.titleLabel];
	}

	UIFont* title_font = cocoaTouchFrameTitleFont(ih, iupCocoaTouchGetFont(ih));
	if (title_font) frame.titleLabel.font = title_font;

	frame.titleLabel.text = title ?: @"";
	[frame setNeedsLayout];
	[frame setNeedsDisplay];
}

static int cocoaTouchFrameMapMethod(Ihandle* ih)
{
	IupCocoaTouchFrameView* frame = [[IupCocoaTouchFrameView alloc] initWithFrame:CGRectZero];
	frame.backgroundColor = [UIColor clearColor];
	frame.opaque = NO;
	frame.strokeColor = cocoaTouchFrameDefaultStrokeColor();

	const char* title = iupAttribGet(ih, "TITLE");

	if (title && *title)
	{
		iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
		cocoaTouchFrameApplyTitle(ih, frame, [NSString stringWithUTF8String:title]);
	}

	ih->handle = frame;
	iupCocoaTouchAddToParent(ih);

	frame.isAccessibilityElement = NO;
	if (frame.titleLabel) frame.titleLabel.accessibilityTraits |= UIAccessibilityTraitHeader;
	return IUP_NOERROR;
}

IUP_SDK_API void iupdrvFrameGetDecorOffset(Ihandle* ih, int* x, int* y)
{
	(void)ih;
	if (x) *x = IUPCOCOATOUCH_FRAME_BORDER_WIDTH;
	if (y) *y = IUPCOCOATOUCH_FRAME_BORDER_WIDTH;
}

IUP_SDK_API int iupdrvFrameHasClientOffset(Ihandle* ih)
{
	(void)ih;
	return 1;
}

static int cocoaTouchFrameTitleHeightPx(Ihandle* ih)
{
	UIFont* title_font = cocoaTouchFrameTitleFont(ih, iupCocoaTouchGetFont(ih));
	if (title_font) return (int)ceil(title_font.lineHeight);
	int char_h = 0;
	iupdrvFontGetCharSize(ih, NULL, &char_h);
	return char_h;
}

IUP_SDK_API int iupdrvFrameGetTitleHeight(Ihandle* ih, int* h)
{
	if (h) *h = cocoaTouchFrameTitleHeightPx(ih);
	return 1;
}

IUP_SDK_API int iupdrvFrameGetDecorSize(Ihandle* ih, int* w, int* h)
{
	if (w) *w = 2 * IUPCOCOATOUCH_FRAME_BORDER_WIDTH;
	if (h)
	{
		*h = 2 * IUPCOCOATOUCH_FRAME_BORDER_WIDTH;
		if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE") || iupAttribGet(ih, "TITLE"))
			*h += cocoaTouchFrameTitleHeightPx(ih);
	}
	return 1;
}

static int cocoaTouchFrameSetFgColorAttrib(Ihandle* ih, const char* color_str)
{
	UIColor* color = iupCocoaTouchToNativeColor(color_str);
	IupCocoaTouchFrameView* frame = cocoaTouchFrameGet(ih);
	if (!color || !frame) return IUP_ERROR;
	if (frame.titleLabel) frame.titleLabel.textColor = color;
	return IUP_NOERROR;
}

static char* cocoaTouchFrameGetFgColorAttrib(Ihandle* ih)
{
	IupCocoaTouchFrameView* frame = cocoaTouchFrameGet(ih);
	if (!frame || !frame.titleLabel) return NULL;
	return iupCocoaTouchColorFromNative(frame.titleLabel.textColor);
}

static int cocoaTouchFrameSetBgColorAttrib(Ihandle* ih, const char* color_str)
{
	IupCocoaTouchFrameView* frame = cocoaTouchFrameGet(ih);
	if (!frame) return 0;

	/* DLGBGCOLOR default: stay transparent so the dialog's dynamic bg shows */
	if (color_str && iupStrEqual(color_str, IupGetGlobal("DLGBGCOLOR")))
	{
		frame.fillColor = nil;
		iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", NULL);
		[frame setNeedsDisplay];
		return 1;
	}

	UIColor* color = iupCocoaTouchToNativeColor(color_str);
	if (!color) return 0;
	frame.fillColor = color;
	iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");
	[frame setNeedsDisplay];
	return 1;
}

static int cocoaTouchFrameSetFrameColorAttrib(Ihandle* ih, const char* color_str)
{
	IupCocoaTouchFrameView* frame = cocoaTouchFrameGet(ih);
	if (!frame) return 0;
	UIColor* color = iupCocoaTouchToNativeColor(color_str);
	frame.strokeColor = color ?: cocoaTouchFrameDefaultStrokeColor();
	[frame setNeedsDisplay];
	return 1;
}

static char* cocoaTouchFrameGetTitleAttrib(Ihandle* ih)
{
	IupCocoaTouchFrameView* frame = cocoaTouchFrameGet(ih);
	if (!frame) return NULL;
	NSString* title = [[frame titleLabel] text];
	return title ? iupStrReturnStr([title UTF8String]) : NULL;
}

static int cocoaTouchFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchFrameView* frame = cocoaTouchFrameGet(ih);
	if (!frame) return 0;
	NSString* title = (value && *value) ? [NSString stringWithUTF8String:value] : nil;
	if (title)
	{
		iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
		cocoaTouchFrameApplyTitle(ih, frame, title);
	}
	else if (frame.titleLabel)
	{
		[frame.titleLabel removeFromSuperview];
		frame.titleLabel = nil;
		iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", NULL);
		[frame setNeedsDisplay];
	}
	return IUP_NOERROR;
}

static int cocoaTouchFrameSetFontAttrib(Ihandle* ih, const char* value)
{
	if (!iupdrvSetFontAttrib(ih, value)) return 0;
	IupCocoaTouchFrameView* frame = cocoaTouchFrameGet(ih);
	if (frame && frame.titleLabel)
	{
		UIFont* title_font = cocoaTouchFrameTitleFont(ih, iupCocoaTouchGetFont(ih));
		if (title_font) frame.titleLabel.font = title_font;
		[frame setNeedsLayout];
		[frame setNeedsDisplay];
	}
	return 1;
}

IUP_SDK_API void iupdrvFrameInitClass(Iclass* ic)
{
	ic->Map = cocoaTouchFrameMapMethod;
	ic->UnMap = iupdrvBaseUnMapMethod;

	iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, cocoaTouchFrameSetBgColorAttrib, "DLGBGCOLOR", NULL, IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", cocoaTouchFrameGetFgColorAttrib, cocoaTouchFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FRAMECOLOR", NULL, cocoaTouchFrameSetFrameColorAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FONT", NULL, cocoaTouchFrameSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
	iupClassRegisterAttribute(ic, "TITLE", cocoaTouchFrameGetTitleAttrib, cocoaTouchFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "SUNKEN", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
