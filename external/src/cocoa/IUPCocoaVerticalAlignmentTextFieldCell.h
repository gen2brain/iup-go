
#import <Cocoa/Cocoa.h>

typedef NS_ENUM(NSUInteger, IUPTextVerticalAlignment)
{
	IUPTextVerticalAlignmentCenter = 0,
	IUPTextVerticalAlignmentTop = 1,
	IUPTextVerticalAlignmentBottom = 2
};

@interface IUPCocoaVerticalAlignmentTextFieldCell : NSTextFieldCell

@property(assign, nonatomic) IUPTextVerticalAlignment alignmentMode;
// TODO: We might need specific modes if the public API exposes them
@property(assign, nonatomic) BOOL useWordWrap;
// TODO: We might need specific modes is the public API exposes them
@property(assign, nonatomic) BOOL useEllipsis;

@end
