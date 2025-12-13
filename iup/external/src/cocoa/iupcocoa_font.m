/** \file
 * \brief MAC Font mapping
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#include <stdlib.h>
#include <stdio.h>

#include "iup.h"

#include "iup_array.h"
#include "iup_assert.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_object.h"
#include "iup_str.h"

#include "iupcocoa_drv.h"

@implementation IupCocoaFont

- (void)dealloc
{
  [self setNativeFont:nil];
  [self setIupFontName:nil];
  [self setTypeFace:nil];
  [self setAttributeDictionary:nil];
  [super dealloc];
}

@end

/* We keep all the fonts in a data structure so we can release them on shutdown. */
static NSMutableDictionary<NSString *, IupCocoaFont *> *s_mapOfFonts = nil;
/* This is for easy access to our system font since it is used so often. */
static IupCocoaFont *s_systemFont = nil;

static IupCocoaFont *cocoaCreateIupCocoaFontFromNSFont(NSFont *ns_font)
{
  IupCocoaFont *the_font = [[IupCocoaFont alloc] init];
  [the_font autorelease];

  NSMutableDictionary *attribute_dict = [[NSMutableDictionary alloc] init];
  [attribute_dict autorelease];
  [attribute_dict setValue:ns_font forKey:NSFontAttributeName];

  [the_font setNativeFont:ns_font];
  [the_font setAttributeDictionary:attribute_dict];

  int font_size = (int)[ns_font pointSize];
  NSString *ns_font_name = [ns_font fontName];
  NSString *ns_iup_font_name = [NSString stringWithFormat:@"%@, %d", ns_font_name, font_size];

  [the_font setIupFontName:ns_iup_font_name];
  [the_font setFontSize:font_size];
  [the_font setTypeFace:ns_font_name];

  /* Use NSTextFieldCell to get the actual line height it uses for rendering. */
  NSTextFieldCell* tempCell = [[NSTextFieldCell alloc] initTextCell:@"Wj"];
  [tempCell setFont:ns_font];
  [tempCell setWraps:YES];
  NSSize singleLineSize = [tempCell cellSizeForBounds:NSMakeRect(0, 0, CGFLOAT_MAX, CGFLOAT_MAX)];
  [tempCell release];
  int char_height = iupROUND(singleLineSize.height);
  [the_font setCharHeight:char_height];

  /* For average char width, use the advancement of a common character like 'x'.
     This is a better approximation than using the font's bounding box. */
  NSGlyph x_glyph = [ns_font glyphWithName:@"x"];
  NSSize x_size = [ns_font advancementForGlyph:x_glyph];
  int char_width = iupROUND(x_size.width);
  [the_font setCharWidth:char_width];

  /* Get dimensions for iupdrvFontGetFontDim */
  int max_width = iupROUND([ns_font maximumAdvancement].width);
  [the_font setMaxWidth:max_width];

  int ascent = iupROUND([ns_font ascender]);
  [the_font setAscent:ascent];

  /* descender is a negative value */
  int descent = iupROUND(-[ns_font descender]);
  [the_font setDescent:descent];

  return the_font;
}

static IupCocoaFont *cocoaGetSystemFont()
{
  if (nil == s_systemFont)
  {
    NSFont *ns_font;

#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
    /* Use modern API on macOS 11+ */
    NSOperatingSystemVersion version = {11, 0, 0};
    if ([[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:version])
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
      ns_font = [NSFont preferredFontForTextStyle:NSFontTextStyleBody options:@{}];
#pragma clang diagnostic pop
    }
    else
#endif
    {
      /* Fall back to legacy API (or when SDK doesn't have macOS 11+ APIs) */
      ns_font = [NSFont messageFontOfSize:0];
    }

    IupCocoaFont *iup_font = cocoaCreateIupCocoaFontFromNSFont(ns_font);
    s_systemFont = [iup_font retain];

    /* Add to global map */
    NSCAssert(s_mapOfFonts, @"s_mapOfFonts must be initialized");
    [s_mapOfFonts setObject:s_systemFont forKey:[iup_font iupFontName]];
  }
  return s_systemFont;
}

IUP_SDK_API char *iupdrvGetSystemFont(void)
{
  static char system_font_cstr[200] = "";
  if (system_font_cstr[0] == '\0')
  {
    if (nil == s_systemFont)
    {
      /* make sure font system was initialized */
      iupdrvFontInit();
      cocoaGetSystemFont();
    }
    strlcpy(system_font_cstr, [[s_systemFont iupFontName] UTF8String], 200);
  }
  return (char *)system_font_cstr;
}

IupCocoaFont *iupcocoaFindFont(const char *iup_font_name)
{
  char type_face[50] = "";
  int font_size = 0;
  int is_bold = 0;
  int is_italic = 0;
  int is_underline = 0;
  int is_strikeout = 0;
  CGFloat final_font_size;

  if (NULL == iup_font_name)
  {
    return NULL;
  }

  NSString *ns_iup_font_name = [NSString stringWithUTF8String:iup_font_name];

  /* Check our cache first */
  IupCocoaFont *the_font = [s_mapOfFonts objectForKey:ns_iup_font_name];
  if (nil != the_font)
  {
    return the_font;
  }

  if (!iupGetFontInfo(iup_font_name, type_face, &font_size, &is_bold, &is_italic, &is_underline, &is_strikeout))
  {
    return NULL;
  }

  /* In IUP, a negative size indicates a value in pixels.
     NSFont works with points, which are resolution-independent.
     Positive sizes are already in points, so we use them directly.
     Negative sizes (pixels) need conversion to points based on DPI. */
  if (font_size < 0)
  {
    /* Convert pixels to points: (pixels × 72) / DPI */
    double dpi = iupdrvGetScreenDpi();
    final_font_size = (CGFloat)((-font_size * 72.0) / dpi);
  }
  else
  {
    final_font_size = (CGFloat)font_size;
  }

  /* A size of 0 is invalid for creating a new font. */
  if (final_font_size == 0)
  {
    return NULL;
  }

  /* Map standard names to native names */
  const char *mapped_name = iupFontGetMacName(type_face);
  if (mapped_name)
  {
    strlcpy(type_face, mapped_name, sizeof(type_face));
  }

  NSString *ns_type_face = [NSString stringWithUTF8String:type_face];
  NSFont *ns_font = [NSFont fontWithName:ns_type_face size:final_font_size];

  /* If the font is not found by name, fall back to the system font with the specified size.
     This increases robustness, similar to font substitution on Windows and GTK/Pango. */
  if (nil == ns_font)
  {
    ns_font = [NSFont systemFontOfSize:final_font_size];
    if (nil == ns_font) /* Should not happen with a valid size, but check just in case. */
    {
      return NULL;
    }
  }

  NSFontTraitMask trait_mask = 0;
  if (is_italic)
  {
    trait_mask |= NSItalicFontMask;
  }
  if (is_bold)
  {
    trait_mask |= NSBoldFontMask;
  }

  if (trait_mask)
  {
    /* Apply bold/italic traits. The font manager will find the correct variant or synthesize one. */
    ns_font = [[NSFontManager sharedFontManager] convertFont:ns_font toHaveTrait:trait_mask];
  }

  if (nil == ns_font)
  {
    return NULL;
  }

  /* Create the IupCocoaFont wrapper and compute its properties */
  the_font = cocoaCreateIupCocoaFontFromNSFont(ns_font);
  [the_font setIupFontName:ns_iup_font_name]; /* Use original IUP name for the key */

  BOOL uses_attributes = NO;
  NSMutableDictionary *attribute_dict = [the_font attributeDictionary];

  if (is_underline)
  {
    [attribute_dict setValue:[NSNumber numberWithInt:NSUnderlinePatternSolid | NSUnderlineStyleSingle] forKey:NSUnderlineStyleAttributeName];
    uses_attributes = YES;
  }
  if (is_strikeout)
  {
    [attribute_dict setValue:[NSNumber numberWithInt:YES] forKey:NSStrikethroughStyleAttributeName];
    uses_attributes = YES;
  }
  [the_font setUsesAttributes:uses_attributes];

  /* Add to cache */
  [s_mapOfFonts setObject:the_font forKey:ns_iup_font_name];

  return the_font;
}

static IupCocoaFont *cocoaFontCreateNativeFont(Ihandle *ih, const char *value)
{
  IupCocoaFont *the_font = iupcocoaFindFont(value);
  if (nil == the_font)
  {
    iupERROR1("Failed to create Font: %s", value);
    return NULL;
  }

  iupAttribSet(ih, "_IUP_COCOAFONT", (char *)the_font);
  return the_font;
}

static IupCocoaFont *cocoaFontGet(Ihandle *ih)
{
  IupCocoaFont *the_font = (IupCocoaFont *)iupAttribGet(ih, "_IUP_COCOAFONT");
  if (nil == the_font)
  {
    the_font = cocoaFontCreateNativeFont(ih, iupGetFontValue(ih));
    if (nil == the_font)
    {
      the_font = cocoaFontCreateNativeFont(ih, IupGetGlobal("DEFAULTFONT"));
    }
  }
  return the_font;
}

IupCocoaFont *iupcocoaGetFont(Ihandle *ih)
{
  return cocoaFontGet(ih);
}

IUP_SDK_API int iupdrvSetFontAttrib(Ihandle *ih, const char *value)
{
  IupCocoaFont *iup_font = cocoaFontCreateNativeFont(ih, value);
  if (nil == iup_font)
  {
    return 0;
  }

  /* If FONT is changed, must update the SIZE attribute */
  iupBaseUpdateAttribFromFont(ih);

  /* FONT attribute must be able to be set before mapping,
     so the font is enable for size calculation. */
  if (ih->handle && (ih->iclass->nativetype != IUP_TYPEVOID))
  {
    id the_widget = (id)ih->handle;
    if ([the_widget respondsToSelector:@selector(setFont:)])
    {
      [the_widget setFont:[iup_font nativeFont]];
    }
  }
  return 1;
}

static void cocoaFontGetTextSize(IupCocoaFont *iup_font, const char *str, int len, int *w, int *h)
{
  int max_w = 0;
  int line_count = 1;

  if (!iup_font)
  {
    if (w) *w = 0;
    if (h) *h = 0;
    return;
  }

  if (!str || str[0] == '\0' || len == 0)
  {
    if (w) *w = 0;
    if (h) *h = [iup_font charHeight];
    return;
  }

  /* Use iupStrLineCount for accurate line counting (same as GTK driver) */
  if (h)
    line_count = iupStrLineCount(str, len);

  if (str[0] && len > 0)
  {
    int l_len;
    const char *nextstr;
    const char *curstr = str;

    do
    {
      nextstr = iupStrNextLine(curstr, &l_len);

      if (l_len > 0)
      {
        NSString *line_str = [[NSString alloc] initWithBytes:curstr length:l_len encoding:NSUTF8StringEncoding];
        NSSize line_size = [line_str sizeWithAttributes:[iup_font attributeDictionary]];
        int line_w = (int)ceil(line_size.width) + 4;
        max_w = iupMAX(max_w, line_w);
        [line_str release];
      }

      curstr = nextstr;
    } while (*nextstr);
  }

  if (w) *w = max_w;
  if (h) *h = [iup_font charHeight] * line_count;
}

IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle *ih, const char *str, int *w, int *h)
{
  IupCocoaFont *iup_font = cocoaFontGet(ih);
  if (iup_font)
  {
    cocoaFontGetTextSize(iup_font, str, str ? (int)strlen(str) : 0, w, h);
  }
}

IUP_SDK_API void iupdrvFontGetTextSize(const char *font_name, const char *str, int len, int *w, int *h)
{
  IupCocoaFont *the_font = iupcocoaFindFont(font_name);
  if (the_font)
  {
    cocoaFontGetTextSize(the_font, str, len, w, h);
  }
}

IUP_SDK_API void iupdrvFontGetFontDim(const char *font, int *max_width, int *line_height, int *ascent, int *descent)
{
  IupCocoaFont *iup_font = iupcocoaFindFont(font);
  if (iup_font)
  {
    if (max_width) *max_width = [iup_font maxWidth];
    if (line_height) *line_height = [iup_font charHeight];
    if (ascent) *ascent = [iup_font ascent];
    if (descent) *descent = [iup_font descent];
  }
}

IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle *ih, const char *str)
{
  int w = 0;
  if (!str || str[0] == 0)
  {
    return 0;
  }

  IupCocoaFont *iup_font = cocoaFontGet(ih);
  if (nil == iup_font)
  {
    return 0;
  }

  /* Measure only the first line */
  const char *line_end = strchr(str, '\n');
  int len = (line_end) ? (int)(line_end - str) : (int)strlen(str);

  cocoaFontGetTextSize(iup_font, str, len, &w, NULL);
  return w;
}

IUP_SDK_API void iupdrvFontGetCharSize(Ihandle *ih, int *charwidth, int *charheight)
{
  IupCocoaFont *iup_font = cocoaFontGet(ih);
  if (!iup_font)
  {
    if (charwidth) *charwidth = 0;
    if (charheight) *charheight = 0;
    return;
  }

  if (charwidth) *charwidth = [iup_font charWidth];
  if (charheight) *charheight = [iup_font charHeight];
}

void iupdrvFontInit(void)
{
  if (nil == s_mapOfFonts)
  {
    s_mapOfFonts = [[NSMutableDictionary alloc] init];
  }
  if (nil == s_systemFont)
  {
    cocoaGetSystemFont();
  }
}

void iupdrvFontFinish(void)
{
  /* This will release all the fonts we've allocated. */
  [s_mapOfFonts release];
  s_mapOfFonts = nil;

  [s_systemFont release];
  s_systemFont = nil;
}
