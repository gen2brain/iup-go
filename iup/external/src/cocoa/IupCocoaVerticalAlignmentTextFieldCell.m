#import "IupCocoaVerticalAlignmentTextFieldCell.h"


@implementation IUPCocoaVerticalAlignmentTextFieldCell

- (NSRect)titleRectForBounds:(NSRect)cellFrame
{
  /* Get the default rect for drawing the text. */
  NSRect titleRect = [super titleRectForBounds:cellFrame];

  /* Top alignment is the default Cocoa behavior, so we can exit early. */
  if (self.alignmentMode == IUPTextVerticalAlignmentTop) {
    return titleRect;
  }

  NSAttributedString *attrString = self.attributedStringValue;
  if (attrString.length == 0) {
    return titleRect;
  }

  /* Calculate the actual height of the text given the available width. */
  /* The attributed string respects the cell's lineBreakMode property. */
  NSRect textBoundingRect = [attrString boundingRectWithSize:NSMakeSize(titleRect.size.width, CGFLOAT_MAX)
                                                     options:NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading];

  CGFloat textHeight = NSHeight(textBoundingRect);

  /* If the text is taller than or fits perfectly in the available space, no alignment is needed. */
  if (textHeight >= titleRect.size.height) {
    return titleRect;
  }

  /* Create a new rect to position the text. */
  NSRect newTitleRect = titleRect;

  /* Adjust the vertical origin based on the desired alignment. */
  switch (self.alignmentMode) {
    case IUPTextVerticalAlignmentCenter:
      newTitleRect.origin.y += (newTitleRect.size.height - textHeight) / 2.0;
      break;
    case IUPTextVerticalAlignmentBottom:
      newTitleRect.origin.y += (newTitleRect.size.height - textHeight);
      break;
    case IUPTextVerticalAlignmentTop:
      /* Already handled. */
      break;
  }

  /* When a field is selected, a "field editor" (an NSTextView) is placed over the cell. */
  /* By setting the height of our drawing rect to the text's actual height, we ensure */
  /* the field editor is created with a tight frame. */
  newTitleRect.size.height = textHeight;

  return newTitleRect;
}

- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
  NSRect titleRect = [self titleRectForBounds:cellFrame];
  [super drawInteriorWithFrame:titleRect inView:controlView];
}

- (void)selectWithFrame:(NSRect)rect inView:(NSView *)controlView editor:(NSText *)editor delegate:(id)delegate start:(NSInteger)start length:(NSInteger)length
{
  /* Pass our vertically-aligned rect to the superclass to position the field editor. */
  [super selectWithFrame:[self titleRectForBounds:rect] inView:controlView editor:editor delegate:delegate start:start length:length];
}

@end
