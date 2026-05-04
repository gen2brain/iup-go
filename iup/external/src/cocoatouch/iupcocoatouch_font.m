/** \file
 * \brief Font (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#import <UIKit/UIKit.h>
#import <CoreText/CoreText.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_attrib.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_object.h"

#include "iupcocoatouch_drv.h"


@implementation IupCocoaTouchFont

- (void)dealloc
{
	[self setNativeFont:nil];
	[self setIupFontName:nil];
	[self setTypeFace:nil];
	[self setAttributeDictionary:nil];
	[super dealloc];
}

@end


static NSMutableDictionary<NSString*, IupCocoaTouchFont*>* s_fontCache = nil;
static IupCocoaTouchFont* s_systemFont = nil;

/* max horizontal advance across printable ASCII; UIFont isn't toll-free with CT*, so go via PostScript name */
static int cocoaTouchFontMaxAdvance(UIFont* ui_font)
{
	if (!ui_font) return 0;

	NSString* fname = [ui_font fontName];
	if ([fname hasPrefix:@"."]) return 0;

	CTFontRef ct = CTFontCreateWithName(
		(__bridge CFStringRef)fname,
		[ui_font pointSize], NULL);
	if (!ct) return 0;

	UniChar chars[95];
	for (int i = 0; i < 95; i++) chars[i] = (UniChar)(0x20 + i);

	CGGlyph glyphs[95] = {0};
	if (!CTFontGetGlyphsForCharacters(ct, chars, glyphs, 95))
	{
		CFRelease(ct);
		return 0;
	}

	CGSize advances[95];
	CTFontGetAdvancesForGlyphs(ct, kCTFontOrientationHorizontal, glyphs, advances, 95);
	CFRelease(ct);

	double max = 0;
	for (int i = 0; i < 95; i++)
	{
		if (advances[i].width > max) max = advances[i].width;
	}
	return (int)ceil(max);
}

static IupCocoaTouchFont* cocoaTouchCreateFontFromUIFont(UIFont* ui_font)
{
	if (!ui_font) return nil;

	IupCocoaTouchFont* font = [[[IupCocoaTouchFont alloc] init] autorelease];

	NSMutableDictionary* attrs = [NSMutableDictionary dictionaryWithObject:ui_font forKey:NSFontAttributeName];

	[font setNativeFont:ui_font];
	[font setAttributeDictionary:attrs];

	int font_size = (int)[ui_font pointSize];
	[font setFontSize:font_size];
	[font setTypeFace:[ui_font fontName]];
	[font setIupFontName:[NSString stringWithFormat:@"%@, %d", [ui_font fontName], font_size]];

	/* UIFont.lineHeight = ascender + descender + leading */
	[font setCharHeight:(int)ceil([ui_font lineHeight])];

	int char_width = (int)ceil([@"x" sizeWithAttributes:attrs].width);
	if (char_width <= 0) char_width = font_size / 2;
	[font setCharWidth:char_width];

	int max_width = cocoaTouchFontMaxAdvance(ui_font);
	if (max_width < char_width) max_width = char_width;
	[font setMaxWidth:max_width];

	[font setAscent:(int)ceil([ui_font ascender])];
	[font setDescent:(int)ceil(-[ui_font descender])];

	return font;
}

static IupCocoaTouchFont* cocoaTouchGetSystemFont(void)
{
	if (s_systemFont != nil) return s_systemFont;

	UIFont* ui_font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
	if (!ui_font) ui_font = [UIFont systemFontOfSize:[UIFont labelFontSize]];

	IupCocoaTouchFont* font = cocoaTouchCreateFontFromUIFont(ui_font);
	/* strong-hold only; the generic cache would leak on Dynamic Type flips */
	s_systemFont = [font retain];
	return s_systemFont;
}

IUP_SDK_API char* iupdrvGetSystemFont(void)
{
	static char buffer[200] = "";
	if (s_systemFont == nil)
	{
		iupdrvFontInit();
		cocoaTouchGetSystemFont();
	}
	/* refresh every call; Dynamic Type can flip s_systemFont mid-session */
	if (s_systemFont) strlcpy(buffer, [[s_systemFont iupFontName] UTF8String], sizeof(buffer));
	return buffer;
}

static BOOL cocoaTouchFontIsSystemName(const char* name)
{
	if (!name || !*name) return YES;
	if (name[0] == '.') return YES;
	return iupStrEqualNoCase(name, "System") ? YES : NO;
}

/* system font traits go through dedicated factories, not symbolicTraits */
static UIFont* cocoaTouchFontBuildSystem(CGFloat size, int is_bold, int is_italic)
{
	if (is_bold && is_italic)
	{
		UIFont* base = [UIFont systemFontOfSize:size weight:UIFontWeightBold];
		UIFontDescriptor* desc = [[base fontDescriptor]
			fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitItalic | UIFontDescriptorTraitBold];
		UIFont* styled = desc ? [UIFont fontWithDescriptor:desc size:size] : nil;
		return styled ? styled : base;
	}
	if (is_bold)   return [UIFont boldSystemFontOfSize:size];
	if (is_italic) return [UIFont italicSystemFontOfSize:size];
	return [UIFont systemFontOfSize:size];
}

static UIFont* cocoaTouchFontApplyTraits(UIFont* base, CGFloat size, int is_bold, int is_italic)
{
	if (!base || (!is_bold && !is_italic)) return base;

	UIFontDescriptorSymbolicTraits traits = 0;
	if (is_bold)   traits |= UIFontDescriptorTraitBold;
	if (is_italic) traits |= UIFontDescriptorTraitItalic;

	UIFontDescriptor* descriptor = [[base fontDescriptor] fontDescriptorWithSymbolicTraits:traits];
	if (!descriptor) return base;
	UIFont* styled = [UIFont fontWithDescriptor:descriptor size:size];
	return styled ? styled : base;
}

IUP_DRV_API IupCocoaTouchFont* iupCocoaTouchFindFont(const char* iup_font_name)
{
	if (!iup_font_name) return nil;

	/* lazy-init guards pre-iupdrvFontInit calls */
	if (s_fontCache == nil) s_fontCache = [[NSMutableDictionary alloc] init];

	NSString* key = [NSString stringWithUTF8String:iup_font_name];
	IupCocoaTouchFont* cached = [s_fontCache objectForKey:key];
	if (cached) return cached;

	char type_face[50] = "";
	int font_size = 0;
	int is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
	if (!iupGetFontInfo(iup_font_name, type_face, &font_size, &is_bold, &is_italic, &is_underline, &is_strikeout))
	{
		return nil;
	}

	/* negative IUP sizes are pixels; convert to points via screen DPI */
	CGFloat point_size;
	if (font_size < 0)
	{
		double dpi = iupdrvGetScreenDpi();
		if (dpi <= 0) dpi = 72.0;
		point_size = (CGFloat)((-font_size * 72.0) / dpi);
	}
	else
	{
		point_size = (CGFloat)font_size;
	}
	if (point_size == 0) return nil;

	const char* mapped = iupFontGetMacName(type_face);
	if (mapped) strlcpy(type_face, mapped, sizeof(type_face));

	UIFont* ui_font;
	if (cocoaTouchFontIsSystemName(type_face))
	{
		ui_font = cocoaTouchFontBuildSystem(point_size, is_bold, is_italic);
	}
	else
	{
		NSString* name = [NSString stringWithUTF8String:type_face];
		ui_font = [UIFont fontWithName:name size:point_size];
		if (!ui_font) ui_font = cocoaTouchFontBuildSystem(point_size, is_bold, is_italic);
		else          ui_font = cocoaTouchFontApplyTraits(ui_font, point_size, is_bold, is_italic);
	}

	IupCocoaTouchFont* font = cocoaTouchCreateFontFromUIFont(ui_font);
	if (!font) return nil;
	[font setIupFontName:key];

	BOOL uses_attributes = NO;
	NSMutableDictionary* attrs = [font attributeDictionary];
	if (is_underline)
	{
		[attrs setObject:[NSNumber numberWithInt:NSUnderlinePatternSolid | NSUnderlineStyleSingle]
		          forKey:NSUnderlineStyleAttributeName];
		uses_attributes = YES;
	}
	if (is_strikeout)
	{
		[attrs setObject:[NSNumber numberWithInt:NSUnderlinePatternSolid | NSUnderlineStyleSingle]
		          forKey:NSStrikethroughStyleAttributeName];
		uses_attributes = YES;
	}
	[font setUsesAttributes:uses_attributes];

	[s_fontCache setObject:font forKey:key];
	return font;
}

IUP_DRV_API IupCocoaTouchFont* iupCocoaTouchGetFont(Ihandle* ih)
{
	IupCocoaTouchFont* font = iupCocoaTouchFindFont(iupGetFontValue(ih));
	if (!font) font = iupCocoaTouchFindFont(IupGetGlobal("DEFAULTFONT"));
	if (!font) font = cocoaTouchGetSystemFont();
	return font;
}

IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchFont* font = iupCocoaTouchFindFont(value);
	if (!font || ![font nativeFont]) return 0;

	iupBaseUpdateAttribFromFont(ih);

	if (ih->handle && ih->iclass->nativetype != IUP_TYPEVOID)
	{
		id widget = (id)ih->handle;
		UIFont* native_font = [font nativeFont];

		if ([widget isKindOfClass:[UITextView class]])
		{
			UITextView* tv = (UITextView*)widget;
			BOOL has_formatting = iupStrBoolean(IupGetAttribute(ih, "FORMATTING")) ? YES : NO;
			if (has_formatting && tv.text.length > 0)
			{
				/* tv.font setter wipes per-range fonts; snapshot+restore keeps format tags */
				NSAttributedString* saved = [tv.attributedText copy];
				tv.font = native_font;
				tv.attributedText = saved;
				[saved release];
			}
			else
			{
				tv.font = native_font;
				if (tv.text.length > 0)
				{
					NSMutableAttributedString* attr = [[NSMutableAttributedString alloc] initWithAttributedString:tv.attributedText];
					[attr addAttribute:NSFontAttributeName value:native_font range:NSMakeRange(0, attr.length)];
					tv.attributedText = attr;
					[attr release];
				}
			}
		}
		else if ([widget respondsToSelector:@selector(setFont:)])
		{
			[widget setFont:native_font];
		}
	}
	return 1;
}

static void cocoaTouchFontGetTextSize(IupCocoaTouchFont* font, const char* str, int len, int* w, int* h)
{
	if (!font)
	{
		if (w) *w = 0;
		if (h) *h = 0;
		return;
	}
	if (!str || len == 0 || str[0] == '\0')
	{
		if (w) *w = 0;
		if (h) *h = [font charHeight];
		return;
	}

	int max_w = 0;
	int line_count = iupStrLineCount(str, len);

	int remaining = len;
	const char* cursor = str;
	while (remaining > 0 && *cursor)
	{
		int line_len = 0;
		const char* next = iupStrNextLine(cursor, &line_len);
		if (line_len > 0)
		{
			NSString* line = [[NSString alloc] initWithBytes:cursor length:line_len encoding:NSUTF8StringEncoding];
			CGSize size = [line sizeWithAttributes:[font attributeDictionary]];
			int line_w = (int)ceil(size.width) + 4;
			if (line_w > max_w) max_w = line_w;
			[line release];
		}
		remaining -= (int)(next - cursor);
		cursor = next;
	}

	if (w) *w = max_w;
	if (h) *h = [font charHeight] * line_count;
}

/* CTLine measures the parsed-markup string by typographic advance */
static BOOL cocoaTouchFontGetMarkupSize(Ihandle* ih, const char* str, int* w, int* h)
{
	if (!ih || !str || !iupAttribGetBoolean(ih, "MARKUP")) return NO;
	IupCocoaTouchFont* iup_font = iupCocoaTouchGetFont(ih);
	UIFont* font = (iup_font && iup_font.nativeFont) ? iup_font.nativeFont : [UIFont systemFontOfSize:UIFont.systemFontSize];
	NSAttributedString* attr = iupCocoaTouchParseMarkup(str, font, nil);
	if (!attr) return NO;
	CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)attr);
	CGFloat ascent = 0, descent = 0, leading = 0;
	double ctw = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
	CFRelease(line);
	if (w) *w = (int)ceil(ctw);
	if (h) *h = (int)ceil(ascent + descent + leading);
	return YES;
}

IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int* w, int* h)
{
	if (cocoaTouchFontGetMarkupSize(ih, str, w, h)) return;
	cocoaTouchFontGetTextSize(iupCocoaTouchGetFont(ih), str, str ? (int)strlen(str) : 0, w, h);
}

IUP_SDK_API void iupdrvFontGetTextSize(const char* font_name, const char* str, int len, int* w, int* h)
{
	cocoaTouchFontGetTextSize(iupCocoaTouchFindFont(font_name), str, len, w, h);
}

IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
	if (!str || !*str) return 0;
	int w = 0;
	if (cocoaTouchFontGetMarkupSize(ih, str, &w, NULL)) return w;
	IupCocoaTouchFont* font = iupCocoaTouchGetFont(ih);
	if (!font) return 0;

	/* first line only; multi-line variant exists for wrapped width */
	const char* newline = strchr(str, '\n');
	int len = newline ? (int)(newline - str) : (int)strlen(str);
	cocoaTouchFontGetTextSize(font, str, len, &w, NULL);
	return w;
}

IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int* charwidth, int* charheight)
{
	IupCocoaTouchFont* font = iupCocoaTouchGetFont(ih);
	if (!font)
	{
		if (charwidth)  *charwidth  = 0;
		if (charheight) *charheight = 0;
		return;
	}
	if (charwidth)  *charwidth  = [font charWidth];
	if (charheight) *charheight = [font charHeight];
}

IUP_SDK_API void iupdrvFontGetFontDim(const char* font_name, int* max_width, int* line_height, int* ascent, int* descent)
{
	IupCocoaTouchFont* font = iupCocoaTouchFindFont(font_name);
	if (!font) return;
	if (max_width)   *max_width   = [font maxWidth];
	if (line_height) *line_height = [font charHeight];
	if (ascent)      *ascent      = [font ascent];
	if (descent)     *descent     = [font descent];
}

static int cocoaTouchFontFamilyCompare(const void* a, const void* b)
{
	return iupStrCompare(*(const char**)a, *(const char**)b, 0, 1);
}

IUP_SDK_API int iupdrvFontGetFamilyList(char*** list)
{
	@autoreleasepool
	{
		NSArray<NSString*>* families = [UIFont familyNames];
		int count = (int)[families count];
		if (count == 0)
		{
			*list = NULL;
			return 0;
		}
		*list = (char**)malloc((size_t)count * sizeof(char*));
		for (int i = 0; i < count; i++)
		{
			(*list)[i] = iupStrDup([[families objectAtIndex:i] UTF8String]);
		}
		qsort(*list, count, sizeof(char*), cocoaTouchFontFamilyCompare);
		return count;
	}
}

/* drops cached system font on Dynamic Type / content-size-category change */
static id s_contentSizeObserver = nil;

IUP_SDK_API void iupdrvFontInit(void)
{
	if (s_fontCache == nil)
	{
		s_fontCache = [[NSMutableDictionary alloc] init];
	}
	if (s_systemFont == nil)
	{
		cocoaTouchGetSystemFont();
	}
	if (s_contentSizeObserver == nil)
	{
		s_contentSizeObserver = [[[NSNotificationCenter defaultCenter]
			addObserverForName:UIContentSizeCategoryDidChangeNotification
			             object:nil
			              queue:[NSOperationQueue mainQueue]
			         usingBlock:^(NSNotification* note) {
				(void)note;
				[s_systemFont release];
				s_systemFont = nil;
			}] retain];
	}
}

IUP_SDK_API void iupdrvFontFinish(void)
{
	if (s_contentSizeObserver)
	{
		[[NSNotificationCenter defaultCenter] removeObserver:s_contentSizeObserver];
		[s_contentSizeObserver release];
		s_contentSizeObserver = nil;
	}
	[s_fontCache release];
	s_fontCache = nil;
	[s_systemFont release];
	s_systemFont = nil;
}
