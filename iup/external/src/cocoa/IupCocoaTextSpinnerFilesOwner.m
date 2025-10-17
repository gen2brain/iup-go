/** \file
 * \brief IupCocoaTextSpinnerFilesOwner.m
 *
 * See Copyright Notice in "iup.h"
 */

#import "IupCocoaTextSpinnerFilesOwner.h"
#import <Cocoa/Cocoa.h>


@implementation IUPStepperObject
@synthesize stepperValue = stepperValue;

- (void) dealloc
{
  [stepperValue release];
  [super dealloc];
}

@end

@implementation IUPStepperObjectController
@end

@implementation IUPTextSpinnerFilesOwner
@synthesize stackView = stackView;
@synthesize stepperView = stepperView;
@synthesize textField = textField;
@synthesize stepperObject = stepperObject;
@synthesize stepperObjectController = stepperObjectController;
@end

@implementation IUPTextSpinnerContainer
@end

