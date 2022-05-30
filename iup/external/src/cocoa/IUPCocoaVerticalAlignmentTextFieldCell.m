//
// See discussion at:
// http://stackoverflow.com/questions/11775128/set-text-vertical-center-in-nstextfield
// But we have to implement a more general version to deal with different vertical alignments.
//

/*
// Update:
Summary:
I have been trying to create my own subclass of NSTextFieldCell that supports the notion of vertical alignment. The goal is for a layout/designer to  provide an oversized area for text to be dropped into to handle different arbitrary sentences of different/unknown lengths. And that widget will automatically vertically align as specified if there is much more space than actual text.

There is pretty much one technique on the internet that everybody seems to use for this.

This generally works, except when I enable setSelectable:YES on the NSTextField. Then the rendering positioning gets messed up.

Typically, the initial render will start by putting the text in one location. (I think it is the right location.) But the moment I click on the window or resize or do something to trigger a redraw, the text position jumps to another location.

I think the first location is the correct location, and the other location is the location that Cocoa would put it if it bypassed/ignored my subclass modifications.

Steps to Reproduce:
I have a attached an Xcode project that demonstrates the problem.
- Build and run it.
- See the text on screen.
- Click the text and see it suddenly jump to a different location.

Basically, I created a subclass of NSTextFieldCell and am using it. I also enabled setSelected:YES on my NSTextField.


Expected Results:

The text should be positioned where I say it should be in my Cell subclass, and should not jump between the two different locations.
This should work with setSelected: in any configuration.

See attached InitialPosition.png

Actual Results:
After the initial render, the text jumps to the location that seems to ignore my positioning changes, when setSelected:YES is picked.

See attached JumpedPosition.png

Version/Build:
10.13.6 (17G65)
Xcode  9.4.1 (9F2000)

I have also seen this in 10.12 and I maybe 10.11. (I keep putting off filing this bug.)

Configuration:
I am running an iMac 20" mid 2010.
Also reproduced on Macbook Air 11" mid 2010


Apple Response:
September 18 2018, 7:47 PM
Engineering has the following feedback for you:

If you are supporting selectable text fields, this is not sufficient for what you are trying to do.

During selection, the field editor is rendering the text+selection,
see: https://developer.apple.com/library/archive/documentation/TextFonts/Conceptual/CocoaTextArchitecture/TextFieldsAndViews/TextFieldsAndViews.html#//apple_ref/doc/uid/TP40009459-CH8-BBCFEBHA

So you also need to provide your own field editor so that you can control its positioning to match the cell.
You could probably use a stock NSTextView for this.
In your NSTextFieldCell subclass override -(NSTextView *)fieldEditorForView:(NSView *)controlView and return your own NSTextView instance there.

Though really, what you’re trying to do would probably be better served by setting up the app layouts with appropriate
Auto Layout constraints that adjust the size and positioning of the text field.
We are now closing this bug report.  If you have questions or comments about the resolution, please update your bug report with that information so we can respond.
*/

#import "IUPCocoaVerticalAlignmentTextFieldCell.h"

@implementation IUPCocoaVerticalAlignmentTextFieldCell
@synthesize alignmentMode = alignmentMode;
@synthesize useWordWrap = useWordWrap;
@synthesize useEllipsis = useEllipsis;

- (NSRect) titleRectForBounds:(NSRect)cell_frame
{
	NSAttributedString* attributed_string = [self attributedStringValue];
	//	NSSize string_size = [attributed_string size];
	
	NSSize string_bounding_size = cell_frame.size;
	
	NSStringDrawingOptions string_draw_options = NSStringDrawingUsesLineFragmentOrigin;
	
	if(NO == [self useWordWrap])
	{
		string_bounding_size.width = CGFLOAT_MAX;
	}
	if([self useEllipsis])
	{
		string_draw_options |= NSStringDrawingTruncatesLastVisibleLine;
		
	}
	
	// 10.11+
	/*
	CGRect rect = [myLabel.text boundingRectWithSize:CGSizeMake(myLabel.frame.size.width, CGFLOAT_MAX)
											 options:NSStringDrawingUsesLineFragmentOrigin
										  attributes:@{NSFontAttributeName: myLabel.font}
											 context:nil];
	*/
	// Deprecated in 10.11.
	// TODO: Allow for font change (needs 10.11 API)
	NSRect string_rect = [attributed_string boundingRectWithSize:string_bounding_size options:string_draw_options];
	CGFloat string_height = string_rect.size.height;
	
	NSRect title_rect = [super titleRectForBounds:cell_frame];
	
	switch(alignmentMode)
	{
		case IUPTextVerticalAlignmentCenter:
		{
			title_rect.origin.y = cell_frame.origin.y + (cell_frame.size.height - string_height) * 0.5;
			
			// There is some discussion to change the height if the text height is greater that the available height.
			// But I think that would screw up IUP's sizing mechanisms.
			
			break;
		}
		case IUPTextVerticalAlignmentTop:
		{
			// this is the normal Cocoa behavior so we don't have to do anything
			break;
		}
		case IUPTextVerticalAlignmentBottom:
		{
			title_rect.origin.y = cell_frame.origin.y + (cell_frame.size.height - string_height);
			break;
		}
		default:
		{
			break;
		}
	}

	return title_rect;
}

- (void) drawInteriorWithFrame:(NSRect)cell_frame inView:(NSView*)control_view
{
	[super drawInteriorWithFrame:[self titleRectForBounds:cell_frame] inView:control_view];
}

- (void) selectWithFrame:(NSRect)cell_frame inView:(NSView*)control_view editor:(NSText*)text_obj delegate:(nullable id)the_delegate start:(NSInteger)sel_start length:(NSInteger)sel_length
{
	[super selectWithFrame:[self titleRectForBounds:cell_frame] inView:control_view editor:text_obj delegate:the_delegate start:sel_start length:sel_length];
}

@end
