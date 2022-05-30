/** \file
 * \brief MAC Font mapping
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdlib.h>
#include <stdio.h>

#import <Cocoa/Cocoa.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_array.h"
#include "iup_attrib.h"
#include "iup_object.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_assert.h"

#include "iupcocoa_drv.h"

//#include "iupmac_info.h"
#include "IupCocoaFont.h"

@implementation IupCocoaFont

// In manual mode (non-ARC), we are still responsible for releasing sythesized properties, even when there is no explicit ivar.
- (void) dealloc
{
	[self setNativeFont:nil];
	[self setIupFontName:nil];
	[self setTypeFace:nil];
	[self setAttributeDictionary:nil];
	[super dealloc];
}

@end

// We keep all the fonts in a data structure so we can release them on shutdown because IUP doesn't have an explicit life-cycle for individual fonts.
// Even though I believe Apple caches NSFont, because we have our own additional object wrapper with additional properties, we might as well do our own caching.
static NSMutableDictionary<NSString*, IupCocoaFont*>* s_mapOfFonts = nil;
// This is for easy access to our system font since it is used so often.
static IupCocoaFont* s_systemFont = nil;
static IupCocoaFont* s_labelFont = nil;
#ifdef IUPCOCOA_USE_SEPARATE_DEFAULT_FONT
	static IupCocoaFont* s_defaultFont = nil;
#endif

static IupCocoaFont* cocoaCreateIupCocoaFontFromNSFont(NSFont* ns_font)
{
	IupCocoaFont* the_font = [[IupCocoaFont alloc] init];
	[the_font autorelease];
	
	
	NSMutableDictionary* attribute_dict = [[NSMutableDictionary alloc] init];
	[attribute_dict autorelease];
	
	
	
	[attribute_dict setValue:ns_font forKey:NSFontAttributeName];

	[the_font setNativeFont:ns_font];
	[the_font setAttributeDictionary:attribute_dict];

	/*
	familyName: .AppleSystemUIFont
	fontName: .AppleSystemUIFont
	displayName: System Font Regular
	*/
	int font_size = (int)[ns_font pointSize];
	NSString* ns_font_name = [ns_font fontName];
	// Remove leading dot?
	//ns_font_name = [ns_font_name stringByReplacingOccurrencesOfString:@"." withString:@""];
	NSString* ns_iup_font_name = [NSString stringWithFormat:@"%@, %d", ns_font_name, font_size];
//		NSLog(@"ns_iup_font_name: %@", ns_iup_font_name);
	[the_font setIupFontName:ns_iup_font_name];
	[the_font setFontSize:font_size];
	[the_font setTypeFace:ns_font_name];
//	[the_font setTraitMask:0];


	NSLayoutManager *lm = [[NSLayoutManager alloc] init];
	int char_height = iupROUND([lm defaultLineHeightForFont:ns_font]);
	[the_font setCharHeight:char_height];
	[lm release];

	// https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/TextLayout/Tasks/StringHeight.html#//apple_ref/doc/uid/20001809-CJBGBIBB
	// defaultLineHeightForFont: 16
	// boundingRectForFont: (width = 21.099609375, height = 17.6337890625)
	NSRect rect = [ns_font boundingRectForFont];
	int char_width = iupROUND(rect.size.width);
	[the_font setCharWidth:char_width];


	return the_font;
}

static IupCocoaFont* cocoaGetSystemFont()
{
	if(nil == s_systemFont)
	{
		NSFont* ns_font = [NSFont systemFontOfSize:0];
		IupCocoaFont* iup_font = cocoaCreateIupCocoaFontFromNSFont(ns_font);
		s_systemFont = [iup_font retain];
		
		// Add to global map
		NSCAssert(s_mapOfFonts, @"s_mapOfFonts must be initialized");
		[s_mapOfFonts setObject:s_systemFont forKey:[iup_font iupFontName]];
	}
	return s_systemFont;
}

static IupCocoaFont* cocoaGetLabelFont()
{
	if(nil == s_labelFont)
	{
		NSFont* ns_font = [NSFont labelFontOfSize:0];
		IupCocoaFont* iup_font = cocoaCreateIupCocoaFontFromNSFont(ns_font);
		s_labelFont = [iup_font retain];
		
		// Add to global map
		NSCAssert(s_mapOfFonts, @"s_mapOfFonts must be initialized");
		[s_mapOfFonts setObject:s_labelFont forKey:[iup_font iupFontName]];
	}
	return s_labelFont;
}

#ifdef IUPCOCOA_USE_SEPARATE_DEFAULT_FONT
static IupCocoaFont* cocoaGetDefaultFont()
{
	if(nil == s_defaultFont)
	{
		NSFont* ns_font = [NSFont systemFontOfSize:0];
		IupCocoaFont* iup_font = cocoaCreateIupCocoaFontFromNSFont(ns_font);
		s_defaultFont = [iup_font retain];
		
		[iup_font setDefaultFont:true];
		[iup_font setTypeFace:@"Default"];
		[iup_font setIupFontName:@"Default, 13"];

		// Add to global map
		NSCAssert(s_mapOfFonts, @"s_mapOfFonts must be initialized");
		[s_mapOfFonts setObject:s_defaultFont forKey:[iup_font iupFontName]];
	}
	return s_defaultFont;
}
#endif

char* iupdrvGetSystemFont(void)
{
	static char system_font_cstr[200] = "";
	if(system_font_cstr[0] == '\0')
	{
#ifdef IUPCOCOA_USE_SEPARATE_DEFAULT_FONT
	    // This actually gets the "default font", which is different than the system font (and the label font)
		if(nil == s_defaultFont)
		{
			// make sure font system was initialized
			iupdrvFontInit();
			cocoaGetDefaultFont();
		}
	    strlcpy(system_font_cstr, [[s_defaultFont iupFontName] UTF8String], 200);
#else
		if(nil == s_systemFont)
		{
			// make sure font system was initialized
			iupdrvFontInit();
			cocoaGetSystemFont();
		}
	    strlcpy(system_font_cstr, [[s_systemFont iupFontName] UTF8String], 200);
#endif
	}
	return (char*)system_font_cstr;
}

static IupCocoaFont* cocoaFindFont(const char* iup_font_name)
{
	char type_face[50] = "";
	int font_size = 0;

	int is_bold = 0;
	int is_italic = 0;
    int is_underline = 0;
	int is_strikeout = 0;

	if(NULL == iup_font_name)
	{
		// return system font?
		return NULL;
	}

	NSString* ns_iup_font_name = [NSString stringWithUTF8String:iup_font_name];

	// Check our cache
	IupCocoaFont* the_font = [s_mapOfFonts objectForKey:ns_iup_font_name];
	if(nil != the_font)
	{
		return the_font;
	}


	if(!iupGetFontInfo(iup_font_name, type_face, &font_size, &is_bold, &is_italic, &is_underline, &is_strikeout))
	{
		return NULL;
	}
	
	NSString* ns_type_face = [NSString stringWithUTF8String:type_face];
	NSFont* ns_font = [NSFont fontWithName:ns_type_face size:(CGFloat)font_size];
	
	if(nil == ns_font)
	{
		return NULL;
	}

	the_font = [[IupCocoaFont alloc] init];
	[the_font autorelease];
	
	[s_mapOfFonts setObject:the_font forKey:ns_iup_font_name];

	

	NSFontTraitMask trait_mask = 0;
  	if(is_italic)
	{
		trait_mask |= NSItalicFontMask;
	}
	if(is_bold)
	{
		trait_mask |= NSBoldFontMask;
	}
	
	NSMutableDictionary* attribute_dict = [[NSMutableDictionary alloc] init];
	[attribute_dict autorelease];

	bool uses_attributes = false;
	if(is_underline)
	{
  		[attribute_dict setValue:[NSNumber numberWithInt:NSUnderlinePatternSolid|NSUnderlineStyleSingle]
  			forKey:NSUnderlineStyleAttributeName];
		uses_attributes = true;
	}
	if(is_strikeout)
	{
  		[attribute_dict setValue:[NSNumber numberWithInt:YES]
			forKey:NSStrikethroughStyleAttributeName];
		uses_attributes = true;
	}
	
	ns_font = [[NSFontManager sharedFontManager] convertFont:ns_font toHaveTrait:trait_mask];
	if(nil == ns_font)
	{
	    return NULL;
	}
	
 	[attribute_dict setValue:ns_font forKey:NSFontAttributeName];

	[the_font setNativeFont:ns_font];
	[the_font setAttributeDictionary:attribute_dict];
	[the_font setUsesAttributes:uses_attributes];
	[the_font setIupFontName:ns_iup_font_name];
	[the_font setFontSize:font_size];
	[the_font setTypeFace:ns_type_face];
//	[the_font setTraitMask:trait_mask];


	NSLayoutManager *lm = [[NSLayoutManager alloc] init];
	int char_height = iupROUND([lm defaultLineHeightForFont:ns_font]);
	[the_font setCharHeight:char_height];
	[lm release];
	
	// https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/TextLayout/Tasks/StringHeight.html#//apple_ref/doc/uid/20001809-CJBGBIBB
	// defaultLineHeightForFont: 16
	// boundingRectForFont: (width = 21.099609375, height = 17.6337890625)
	NSRect rect = [ns_font boundingRectForFont];
	int char_width = iupROUND(rect.size.width);
	[the_font setCharWidth:char_width];


	return the_font;
}

static IupCocoaFont* cocoaFontCreateNativeFont(Ihandle* ih, const char* value)
{
	IupCocoaFont* the_font = cocoaFindFont(value);
	if(nil == the_font)
	{
		iupERROR1("Failed to create Font: %s", value);
		return NULL;
	}

	iupAttribSet(ih, "_IUP_COCOAFONT", (char*)the_font);
	return the_font;
}

static IupCocoaFont* cocoaFontGet(Ihandle* ih)
{
	IupCocoaFont* the_font = (IupCocoaFont*)iupAttribGet(ih, "_IUP_COCOAFONT");
// I think we should support nil because not all widgets use the default system font. (Some use the label font.)
// We don't want to accidentally force the system font when it shouldn't have been used.
// But in practice, I am unable to implement this because IUP wants to set a font in the core.
// And because IUP computes sizes based on character width & height, we can't leave this undefined.
#if 1
	if(nil == the_font)
	{
	    the_font = cocoaFontCreateNativeFont(ih, iupGetFontValue(ih));
		if(nil == the_font)
		{
			the_font = cocoaFontCreateNativeFont(ih, IupGetGlobal("DEFAULTFONT"));
		}
	}
#endif
	return the_font;
}

// version for external files to use
IupCocoaFont* iupCocoaGetFont(Ihandle* ih)
{
	return cocoaFontGet(ih);
}

int iupdrvSetFontAttrib(Ihandle* ih, const char* value)
{
	IupCocoaFont* iup_font = cocoaFontCreateNativeFont(ih, value);
/*
	if(nil == iup_font)
	{
		return 0;
	}
*/
	/* If FONT is changed, must update the SIZE attribute */
	iupBaseUpdateAttribFromFont(ih);
	
	/* FONT attribute must be able to be set before mapping,
	 so the font is enable for size calculation. */
	if (ih->handle && (ih->iclass->nativetype != IUP_TYPEVOID))
	{
		id the_widget = (id)ih->handle;
		if([the_widget respondsToSelector:@selector(setFont:)])
		{
			// WARNING: There is a potential bug here.
			// IUP is forcing us to provide a default font.
			// But Cocoa uses at least two different fonts depending on circumstances (system & label)
			// The safest thing would be to setFont:nil, but the way IUP is structured, it is not easy to leave a nil value.
			// And because IUP computes sizes based on character width & height, we can't leave this undefined.
			// For now
#ifdef IUPCOCOA_USE_SEPARATE_DEFAULT_FONT
			if(![iup_font isDefaultFont])
			{
				[the_widget setFont:[iup_font nativeFont]];
			}
			else
			{
				// This is for trying to detect the DEFAULT font case
				// While layout might be wrong, the font won't get set so we won't accidentally see the wrong size.
			}
#else
			[the_widget setFont:[iup_font nativeFont]];
#endif

		}
	}
	return 1;
}


static void helperFontGetMultiLineStringSize(IupCocoaFont* iup_font, const char* str, int *w, int *h)
{
	int num_lin;
	int max_w;
	int max_h;
	
	if(!iup_font)
	{
		if (w) *w = 0;
		if (h) *h = 0;
		return;
	}

	if(!str)
	{
		if (w) *w = 0;
		if (h) *h = [iup_font charHeight];
		return;
	}

	max_w = 0;
	max_h = 0;
	num_lin = 1;
	if(str[0])
	{
		
		int len;
		const char *nextstr;
		const char *curstr = str;
		NSMutableArray* array_of_nsstring_lines = [[NSMutableArray alloc] init];
		
		do
		{
			nextstr = iupStrNextLine(curstr, &len);
			NSString *str = [[NSString alloc] initWithBytes:curstr length:len encoding:NSUTF8StringEncoding];
			/*
			 NSSize size = [str sizeWithAttributes: macfont->attributes];
			 //		NSSize size = [str sizeWithAttributes:[NSDictionary dictionaryWithObjectsAndKeys:[NSFont systemFontOfSize:0], NSFontAttributeName, nil]];
			 */
			if(nil != str)
			{
				[array_of_nsstring_lines addObject:str];
				[str release];
			}
			else
			{
				[array_of_nsstring_lines addObject:@""];
			}
			
			//		max_w = iupMAX(max_w, size.width + 0.5);
			
			curstr = nextstr;
			if (*nextstr)
			{
				num_lin++;
			}
		} while(*nextstr);
		
		NSString* joined_string = [array_of_nsstring_lines componentsJoinedByString:@"\n"];
		// 10.11+
		// boundingRectWithSize:options:attributes:context:
		//
		// sizeWithFont:constrainedToSize:lineBreakMode: on iOS, but deprecated in iOS7
		NSSize size = [joined_string sizeWithAttributes:[iup_font attributeDictionary]];
		
		max_w = (int)ceil(size.width);
		max_h = (int)ceil(size.height);
		
		[array_of_nsstring_lines release];
	}
	
	if (w)
	{
		*w = max_w;
	}
	//	if (h) *h = (macfont->charheight*num_lin)+0.5;
	if (h)
	{
		*h = max_h;
	}
}

void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int *w, int *h)
{
	/* This won't work because other callbacks assume the string size and then add padding on top of it.
	id native_object = ih->handle;
	if([native_object respondsToSelector:@selector(sizeToFit)])
   {
//	   [native_object sizeToFit];
	   NSRect the_rect = [native_object frame];
	 
	   if (w) *w = the_rect.size.width;
	   if (h) *h = the_rect.size.height;
	 
	   return;
   }
*/
	IupCocoaFont* iup_font = cocoaFontGet(ih);
	helperFontGetMultiLineStringSize(iup_font, str, w, h);
}

int iupdrvFontGetStringWidth(Ihandle* ih, const char* str)
{
	size_t len;
	char* line_end;
	
	if(!str || str[0]==0)
	{
		return 0;
	}
	
	IupCocoaFont* iup_font = cocoaFontGet(ih);
	if(nil == iup_font)
	{
		return 0;
	}
	
	line_end = strchr(str, '\n');
	if (line_end)
	{
		len = line_end-str;
	}
	else
	{
		len = strlen(str);
	}
	
	NSString* ns_str = [[NSString alloc] initWithBytes:str length:len encoding:NSUTF8StringEncoding];
	
	// 10.11+
	// boundingRectWithSize:options:attributes:context:
	//
	// sizeWithFont:constrainedToSize:lineBreakMode: on iOS, but deprecated in iOS7
	NSSize size = [ns_str sizeWithAttributes:[iup_font attributeDictionary]];
	[ns_str release];
	
	return (int)ceil(size.width);
}

void iupdrvFontGetCharSize(Ihandle* ih, int *charwidth, int *charheight)
{
	IupCocoaFont* iup_font = cocoaFontGet(ih);
	if(!iup_font)
	{
		if (charwidth)  *charwidth = 0;
		if (charheight) *charheight = 0;
		return;
	}
	
	if (charwidth)  *charwidth = [iup_font charWidth];
	if (charheight) *charheight = [iup_font charHeight];
}

void iupdrvFontGetTextSize(const char* font_name, const char* str, int len, int *w, int *h)
{
	IupCocoaFont* the_font = cocoaFindFont(font_name);
	if(the_font)
	{
		// FIXME: quick and dirty fix to get around Iup internal API changes
		helperFontGetMultiLineStringSize(the_font, str, w, h);
	}
}

void iupdrvFontInit(void)
{
	if(nil == s_mapOfFonts)
	{
		s_mapOfFonts = [[NSMutableDictionary alloc] init];
	}
	if(nil == s_systemFont)
	{
		// this will set s_systemFont
		cocoaGetSystemFont();
	}
	if(nil == s_labelFont)
	{
		// this will set s_systemFont
		cocoaGetLabelFont();
	}
#ifdef IUPCOCOA_USE_SEPARATE_DEFAULT_FONT
	if(nil == s_defaultFont)
	{
		// this will set s_systemFont
		cocoaGetDefaultFont();
	}
#endif

}

void iupdrvFontFinish(void)
{
 	// This will release all the fonts we've allocated.
	[s_mapOfFonts release];
	s_mapOfFonts = nil;
	[s_systemFont release];
	s_systemFont = nil;
	[s_labelFont release];
	s_labelFont = nil;
#ifdef IUPCOCOA_USE_SEPARATE_DEFAULT_FONT
	[s_defaultFont release];
	s_defaultFont = nil;
#endif
}
