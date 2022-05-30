//
//  IUPTextSpinnerFilesOwner.m
//  iup
//
//  Created by Eric Wing on 5/23/17.
//  Copyright © 2017 Tecgraf, PUC-Rio, Brazil. All rights reserved.
//

#import "IUPTextSpinnerFilesOwner.h"
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

