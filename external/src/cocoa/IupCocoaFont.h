#ifndef IUPCOCOAFONT_H
#define IUPCOCOAFONT_H

#import <Cocoa/Cocoa.h>
#include <stdbool.h>

//
// This is an experiment to use a separate default font that is distinct from the system font.
// This is because there is a system font and label font, and I would like to avoid accidentally overriding places where the label font should be used.
// However, in practice, this is really hard to do with IUP because IUP tries to do layout based on cached character sizes.
// If we don't know the font, we can't compute these metrics.
// So I think this experiment is a dead-end.
// But I'm keeping it around as a quick way to toggle because it may allow me to see where I accidentally overrided the label font.
// #define IUPCOCOA_USE_SEPARATE_DEFAULT_FONT 1


// This class encapsulates all the native font attributes we need for IUP.
// It contains the native NSFont nativeFont and caches other attributes that are commonly needed (and/or difficult to extract from the NSFont).
@interface IupCocoaFont : NSObject

@property(nonatomic, retain) NSFont* nativeFont;
@property(nonatomic, copy) NSString* iupFontName;
@property(nonatomic, copy) NSString* typeFace;
//@property(nonatomic, retain) NSMutableDictionary* attributeDictionary;
@property(nonatomic, copy) NSDictionary* attributeDictionary;
@property(nonatomic, assign) bool usesAttributes;
@property(nonatomic, assign) int fontSize;
//@property(nonatomic, assign) NSFontTraitMask traitMask;
@property(nonatomic, assign) int charWidth;
@property(nonatomic, assign) int charHeight;

#ifdef IUPCOCOA_USE_SEPARATE_DEFAULT_FONT
	@property(nonatomic, assign, getter=isDefaultFont) bool defaultFont;
#endif

@end

#endif /* IUPCOCOAFONT_H */

