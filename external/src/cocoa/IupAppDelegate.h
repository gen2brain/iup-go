//
//  IupAppDelegate.h
//  iup
//
//  Created by Eric Wing on 12/23/16.
//  Copyright © 2016 Tecgraf, PUC-Rio, Brazil. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface IupAppDelegate : NSObject <NSApplicationDelegate>

@property(nonatomic, assign, getter=isInitialized) bool initialized;
@property(nonatomic, copy) NSArray<NSString*>* deferredOpenFiles;
@property(nonatomic, copy) NSString* deferredOpenURL;

@end
