#ifndef __IUPCOCOA_CANVASVIEW_H 
#define __IUPCOCOA_CANVASVIEW_H

#import <Cocoa/Cocoa.h>

#include <stdbool.h>
#include "iup.h"

struct _IdrawCanvas;

// I'm undecided if we should subclass NSView or NSControl.
// IUP wants to use Canvas for fake controls and the enabled property is useful.
// However the other properties (so far) don't seem that useful.
// I have been worried about subtle behaviors like key loops and input that I may not be aware of that NSControl may handle,
// but so far tracking down key loop problems, I've found that NSControl did nothing to help.
// So far NSView seems to be sufficient, as long as I implement the enabled property and make sure none of the other code checks for [NSControl class].
// (I had to replace some instances of that with respondsToSelector() on isEnabled/setEnabled: in Common.)
// UPDATE: I found one edge-case where NSControl is better than NSView.
// In the flatbutton test, if you tab over to make one of the canvas buttons focus. Then click the disable all widgets button.
// In the NSView case, the focus ring gets stuck around the disabled widget (both native focus ring and IUP focus ring).
// But in the NSControl case, the focus ring goes away.
@interface IupCocoaCanvasView : NSControl
{
	Ihandle* _ih;
	struct _IdrawCanvas* _dc;
	bool _isCurrentKeyWindow;
	bool _isCurrentFirstResponder;
	bool _startedDrag;
}
@property(nonatomic, assign) Ihandle* ih;
@property(nonatomic, assign) struct _IdrawCanvas* dc;
@property(nonatomic, assign, getter=isCurrentKeyWindow, setter=setCurrentKeyWindow:) bool currentKeyWindow;
@property(nonatomic, assign, getter=isCurrentFirstResponder, setter=setCurrentFirstResponder:) bool currentFirstResponder;
@property(nonatomic, assign) bool startedDrag;
//@property(nonatomic, assign, getter=isEnabled, setter=setEnabled:) BOOL enabled; // provided by NSControl, must define if we are NSView
@property(nonatomic, assign) bool useNativeFocusRing;
@property(nonatomic, assign) struct CGSize previousSize;
- (void) updateFocus;
- (NSGraphicsContext*) graphicsContext;
- (CGContextRef) CGContext;

@property(nonatomic, copy) NSColor* backgroundColor;

@end

#endif /* __IUPCOCOA_CANVASVIEW_H */

