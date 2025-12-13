#import <Cocoa/Cocoa.h>


typedef NS_ENUM(NSUInteger, IUPTextVerticalAlignment)
{
  IUPTextVerticalAlignmentCenter = 0,
  IUPTextVerticalAlignmentTop = 1,
  IUPTextVerticalAlignmentBottom = 2
};

/**
 * A custom NSTextFieldCell that supports vertical alignment of its text.
 * Standard cell properties like `lineBreakMode` should be used to control
 * text wrapping and truncation.
 */
@interface IUPCocoaVerticalAlignmentTextFieldCell : NSTextFieldCell

@property(assign, nonatomic) IUPTextVerticalAlignment alignmentMode;
@property(assign, nonatomic) BOOL useWordWrap;
@property(assign, nonatomic) BOOL useEllipsis;

@end