#ifndef IUPCOCOATREETOGGLETABLECELLVIEW_H
#define IUPCOCOATREETOGGLETABLECELLVIEW_H

#import <Cocoa/Cocoa.h>


@interface IupCocoaTreeToggleTableCellView : NSTableCellView
@property(retain, nonatomic) IBOutlet NSButton* checkBox;
@end

#endif