#import <Cocoa/Cocoa.h>
#import "IupCocoaTreeToggleTableCellView.h"


@implementation IupCocoaTreeToggleTableCellView

@synthesize checkBox = _checkBox;

- (instancetype)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame:frameRect];
  if (self)
  {
    /* Create the checkbox. */
    /* We use the setter to ensure it is properly retained. */
    self.checkBox = [[[NSButton alloc] initWithFrame:NSZeroRect] autorelease];
    [self.checkBox setButtonType:NSButtonTypeSwitch];
    [self.checkBox setTitle:@""]; /* A checkbox shouldn't have a title of its own */
    [self.checkBox setTranslatesAutoresizingMaskIntoConstraints:NO];
    [self addSubview:self.checkBox];

    /* The superclass (NSTableCellView) has imageView and textField properties. */
    /* We must create them ourselves when not using a XIB. */
    self.imageView = [[[NSImageView alloc] initWithFrame:NSZeroRect] autorelease];
    [self.imageView setTranslatesAutoresizingMaskIntoConstraints:NO];
    [self addSubview:self.imageView];

    self.textField = [[[NSTextField alloc] initWithFrame:NSZeroRect] autorelease];
    [self.textField setTranslatesAutoresizingMaskIntoConstraints:NO];
    [self.textField setBezeled:NO];
    [self.textField setDrawsBackground:NO];
    [self.textField setEditable:NO];
    [self.textField setSelectable:NO];
    [self addSubview:self.textField];

    /* Set up Auto Layout constraints */
    NSDictionary* views = @{
      @"checkBox": self.checkBox,
     @"imageView": self.imageView,
     @"textField": self.textField
    };

         /* Horizontal constraints: Arrange checkbox, image, and text from left to right. */
         [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-2-[checkBox]-5-[imageView]-5-[textField]-2-|"
                      options:0
                      metrics:nil
                        views:views]];

             /* Vertical constraints: Center all subviews vertically within the cell. */
             [self addConstraint:[NSLayoutConstraint constraintWithItem:self.checkBox
                       attribute:NSLayoutAttributeCenterY
                       relatedBy:NSLayoutRelationEqual
                          toItem:self
                       attribute:NSLayoutAttributeCenterY
                      multiplier:1.0
                        constant:0]];

             [self addConstraint:[NSLayoutConstraint constraintWithItem:self.imageView
                       attribute:NSLayoutAttributeCenterY
                       relatedBy:NSLayoutRelationEqual
                          toItem:self
                       attribute:NSLayoutAttributeCenterY
                      multiplier:1.0
                        constant:0]];

             [self addConstraint:[NSLayoutConstraint constraintWithItem:self.textField
                       attribute:NSLayoutAttributeCenterY
                       relatedBy:NSLayoutRelationEqual
                          toItem:self
                       attribute:NSLayoutAttributeCenterY
                      multiplier:1.0
                        constant:0]];
  }
  return self;
}

- (void)dealloc
{
  /* Release the retained checkBox property */
  [_checkBox release];
  _checkBox = nil;
  [super dealloc];
}

@end
