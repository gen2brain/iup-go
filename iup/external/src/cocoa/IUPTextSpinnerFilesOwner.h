//
//  IUPTextSpinnerFilesOwner.h
//  iup
//
//  Created by Eric Wing on 5/23/17.
//  Copyright © 2017 Tecgraf, PUC-Rio, Brazil. All rights reserved.
//

#import <Cocoa/Cocoa.h>

// for Cocoa Bindings
@interface IUPStepperObject : NSObject
{
//	NSInteger stepperValue;
	NSNumber* stepperValue;
}
//@property(nonatomic, assign) NSInteger stepperValue;
@property(nonatomic, retain) NSNumber* stepperValue;
@end

// for Cocoa Bindings
@interface IUPStepperObjectController : NSObjectController
@end

// for Interface Builder
@interface IUPTextSpinnerFilesOwner : NSObject
{
	IBOutlet NSStackView* stackView;
	IBOutlet NSStepper* stepperView;
	IBOutlet NSTextField* textField;
	IBOutlet IUPStepperObject* stepperObject;
	IBOutlet IUPStepperObjectController* stepperObjectController;
}
@property(nonatomic, readonly, weak) NSStackView* stackView;
@property(nonatomic, readonly, weak) NSStepper* stepperView;
@property(nonatomic, readonly, weak) NSTextField* textField;
@property(nonatomic, readonly, weak) IUPStepperObject* stepperObject;
@property(nonatomic, readonly, weak) IUPStepperObjectController* stepperObjectController;
@end


// For us to hold all the widgets
@interface IUPTextSpinnerContainer : NSObject

@property(nonatomic, retain) NSStepper* stepperView;
@property(nonatomic, retain) NSTextField* textField;
@property(nonatomic, retain) IUPStepperObject* stepperObject;
@property(nonatomic, retain) IUPStepperObjectController* stepperObjectController;

@end
