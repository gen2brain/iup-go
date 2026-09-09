/** \file
 * \brief Canvas Control for Cocoa
 *
 * See Copyright Notice in "iup.h"
 */


#include <stdio.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_canvas.h"
#include "iup_key.h"
#include "iupkey.h"
#include "iup_class.h"
#include "iup_focus.h"

#include "iupcocoa_drv.h"
#include "iupcocoa_draw.h"
#include "iupcocoa_dragdrop.h"


static void cocoaCanvasLayoutUpdateMethod(Ihandle *ih);
static int cocoaCanvasSetDXAttrib(Ihandle* ih, const char* value);
static int cocoaCanvasSetDYAttrib(Ihandle* ih, const char* value);
static void cocoaCanvasComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *expand);

@interface IupCocoaCanvasView : NSControl <NSTextInputClient>

@property(nonatomic, assign) Ihandle* ih;

@property(nonatomic, retain) NSString* textInputMarked;
@property(nonatomic, assign) bool textInputConsumed;

@property(nonatomic, assign, getter=isCurrentKeyWindow) bool currentKeyWindow;
@property(nonatomic, assign, getter=isCurrentFirstResponder) bool currentFirstResponder;

@property(nonatomic, assign) bool startedDrag;
@property(nonatomic, assign) NSPoint dragStartPoint;

@property(nonatomic, assign) CGSize previousSize;

@property(nonatomic, copy) NSColor* backgroundColor;

@property(nonatomic, assign) bool useNativeFocusRing;

- (instancetype) initWithFrame:(NSRect)frame_rect ih:(Ihandle*)ih;

- (void) updateFocus;

@end


@interface IupCocoaFixedView : NSView
@end

@implementation IupCocoaFixedView

- (BOOL) isFlipped
{
  return YES;
}

@end


@interface IupLogicalScrollClipView : NSClipView
@end

@implementation IupLogicalScrollClipView

- (void) setBoundsOrigin:(NSPoint)newOrigin
{
  /* the clip view never moves; scrolling is logical through POSX/POSY */
  [super setBoundsOrigin:NSZeroPoint];
}

- (NSRect) constrainBoundsRect:(NSRect)proposedBounds
{
  NSRect constrained = proposedBounds;
  constrained.origin = NSZeroPoint;
  return constrained;
}

@end

static void cocoaCanvasDrawBuffer(NSBitmapImageRep* buffer, NSRect bounds)
{
  NSGraphicsContext* context = [NSGraphicsContext currentContext];
  [context saveGraphicsState];

  [buffer drawInRect:bounds
            fromRect:NSMakeRect(0, 0, bounds.size.width, bounds.size.height)
           operation:NSCompositingOperationCopy
            fraction:1.0
      respectFlipped:YES
               hints:nil];

  [context restoreGraphicsState];
}

#ifndef GNUSTEP
static int cocoaCanvasGestureState(NSGestureRecognizerState s)
{
  switch (s)
  {
    case NSGestureRecognizerStateBegan:   return IUP_GESTURE_BEGIN;
    case NSGestureRecognizerStateChanged: return IUP_GESTURE_CHANGED;
    case NSGestureRecognizerStateEnded:   return IUP_GESTURE_END;
    default:                              return IUP_GESTURE_CANCEL;
  }
}
#endif /* !GNUSTEP */

#ifndef GNUSTEP
static void cocoaCanvasFireGesture(Ihandle* ih, int gesture, int state, int x, int y, double v1, double v2)
{
  IFniiiidd cb = (IFniiiidd)IupGetCallback(ih, "GESTURE_CB");
  if (cb && cb(ih, gesture, state, x, y, v1, v2) == IUP_CLOSE)
    IupExitLoop();
}
#endif /* !GNUSTEP */

@implementation IupCocoaCanvasView

- (instancetype) initWithFrame:(NSRect)frame_rect ih:(Ihandle*)ih
{
  self = [super initWithFrame:frame_rect];
  if(self)
  {
    _ih = ih;
    [self setEnabled:YES];

#ifndef GNUSTEP
    /* trackpad gestures; pan is scroll (WHEEL_CB) and tap/long-press are mouse (BUTTON_CB) on the desktop */
    NSMagnificationGestureRecognizer* magnify = [[NSMagnificationGestureRecognizer alloc] initWithTarget:self action:@selector(onMagnify:)];
    [self addGestureRecognizer:magnify];
    [magnify release];

    NSRotationGestureRecognizer* rotate = [[NSRotationGestureRecognizer alloc] initWithTarget:self action:@selector(onRotate:)];
    [self addGestureRecognizer:rotate];
    [rotate release];
#endif /* !GNUSTEP */
  }
  return self;
}

#ifndef GNUSTEP
- (void) onMagnify:(NSMagnificationGestureRecognizer*)g
{
  NSPoint p = [g locationInView:self];
  cocoaCanvasFireGesture(_ih, IUP_GESTURE_PINCH, cocoaCanvasGestureState([g state]), (int)p.x, (int)p.y, 1.0 + (double)[g magnification], 0.0);
}

- (void) onRotate:(NSRotationGestureRecognizer*)g
{
  NSPoint p = [g locationInView:self];
  double deg = (double)[g rotation] * 180.0 / M_PI;
  cocoaCanvasFireGesture(_ih, IUP_GESTURE_ROTATE, cocoaCanvasGestureState([g state]), (int)p.x, (int)p.y, deg, 0.0);
}

- (void) swipeWithEvent:(NSEvent*)the_event
{
  if (!_ih || !IupGetCallback(_ih, "GESTURE_CB"))
  {
    [super swipeWithEvent:the_event];
    return;
  }
  NSPoint p = [self convertPoint:[the_event locationInWindow] fromView:nil];
  CGFloat dx = [the_event deltaX];
  CGFloat dy = [the_event deltaY];
  int dir;
  /* AppKit swipe: +deltaX = left, +deltaY = up */
  if (fabs(dx) > fabs(dy)) dir = dx > 0 ? IUP_GESTURE_SWIPE_LEFT : IUP_GESTURE_SWIPE_RIGHT;
  else                     dir = dy > 0 ? IUP_GESTURE_SWIPE_UP : IUP_GESTURE_SWIPE_DOWN;
  cocoaCanvasFireGesture(_ih, IUP_GESTURE_SWIPE, IUP_GESTURE_END, (int)p.x, (int)p.y, (double)dir, 0.0);
}
#endif /* !GNUSTEP */

- (void) dealloc
{
  NSNotificationCenter* notification_center = [NSNotificationCenter defaultCenter];
  [notification_center removeObserver:self];
  [self setBackgroundColor:nil];
  [self setTextInputMarked:nil];

  [super dealloc];
}

- (BOOL) isFlipped
{
  if (iupAttribGet(_ih, "_IUP_GLCONTROLDATA"))
    return NO;
  return YES;
}

- (BOOL) isOpaque
{
  if (iupAttribGet(_ih, "_IUP_GLCONTROLDATA"))
    return YES;
  return NO;
}

- (void) viewWillMoveToWindow:(NSWindow*)newWindow
{
  [super viewWillMoveToWindow:newWindow];

  NSNotificationCenter* notification_center = [NSNotificationCenter defaultCenter];
  if([self window])
  {
    [notification_center removeObserver:self name:NSWindowDidBecomeKeyNotification object:[self window]];
    [notification_center removeObserver:self name:NSWindowDidResignKeyNotification object:[self window]];
  }
}

- (void) viewDidMoveToWindow
{
  [super viewDidMoveToWindow];

  NSNotificationCenter* notification_center = [NSNotificationCenter defaultCenter];
  if([self window])
  {
    [notification_center addObserver:self
                            selector:@selector(windowDidBecomeKeyNotification:)
                                name:NSWindowDidBecomeKeyNotification
                              object:[self window]
    ];
    [notification_center addObserver:self
                            selector:@selector(windowDidResignKeyNotification:)
                                name:NSWindowDidResignKeyNotification
                              object:[self window]
    ];

    /* updateTrackingAreas is not called until the frame changes */
    [self updateTrackingAreas];
  }
}

- (void) drawRect:(NSRect)dirty_rect
{
  NSRect bounds = [self bounds];

  if (iupAttribGet(_ih, "_IUP_GLCONTROLDATA"))
  {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NSOpenGLContext* gl_context = (NSOpenGLContext*)iupAttribGet(_ih, "CONTEXT");
    if (gl_context)
    {
      [gl_context clearDrawable];
      [gl_context setView:self];
      [gl_context update];
    }
#pragma clang diagnostic pop

    IFn cb = (IFn)IupGetCallback(_ih, "ACTION");
    if (cb && !(_ih->data->inside_resize))
    {
      iupAttribSetStrf(_ih, "CLIPRECT", "%.0f %.0f %.0f %.0f",
                       dirty_rect.origin.x, dirty_rect.origin.y,
                       dirty_rect.origin.x + dirty_rect.size.width - 1,
                       dirty_rect.origin.y + dirty_rect.size.height - 1);
      cb(_ih);
      iupAttribSet(_ih, "CLIPRECT", NULL);
    }
    return;
  }

  if (bounds.size.width <= 0 || bounds.size.height <= 0)
    return;

  if (iupAttribGet(_ih, "_IUPCOCOA_BUFFER_PENDING"))
  {
    NSBitmapImageRep* buffer = (NSBitmapImageRep*)iupAttribGet(_ih, "_IUPCOCOA_CANVAS_BUFFER");

    if (buffer)
    {
      if ([buffer pixelsWide] == (NSInteger)bounds.size.width &&
          [buffer pixelsHigh] == (NSInteger)bounds.size.height)
      {
        cocoaCanvasDrawBuffer(buffer, bounds);
        iupAttribSet(_ih, "_IUPCOCOA_BUFFER_PENDING", NULL);
        return;
      }
    }
  }

  IFn call_back = (IFn)IupGetCallback(_ih, "ACTION");

  if (call_back)
  {
    NSRect clip_rect = NSIntersectionRect(dirty_rect, bounds);

    if (clip_rect.size.width <= 0 || clip_rect.size.height <= 0)
    {
      iupAttribSet(_ih, "CLIPRECT", NULL);
    }
    else
    {
      double x1 = floor(clip_rect.origin.x);
      double y1 = floor(clip_rect.origin.y);
      double x2 = ceil(clip_rect.origin.x + clip_rect.size.width) - 1;
      double y2 = ceil(clip_rect.origin.y + clip_rect.size.height) - 1;

      if (x2 < x1) x2 = x1;
      if (y2 < y1) y2 = y1;

      iupAttribSetStrf(_ih, "CLIPRECT", "%.0f %.0f %.0f %.0f", x1, y1, x2, y2);
    }

    CGContextRef cgContext = [[NSGraphicsContext currentContext] CGContext];
    iupAttribSet(_ih, "CGCONTEXT", (char*)cgContext);

    call_back(_ih);

    iupAttribSet(_ih, "CGCONTEXT", NULL);
    iupAttribSet(_ih, "CLIPRECT", NULL);

    {
      NSBitmapImageRep* buffer = (NSBitmapImageRep*)iupAttribGet(_ih, "_IUPCOCOA_CANVAS_BUFFER");
      if (buffer)
        cocoaCanvasDrawBuffer(buffer, bounds);
    }
  }
  else
  {
    if ([self backgroundColor])
    {
      NSRect fill_rect = NSIntersectionRect(dirty_rect, [self bounds]);
      if (!NSIsEmptyRect(fill_rect))
      {
        [[self backgroundColor] set];
        NSRectFill(fill_rect);
      }
    }
  }
}

- (void) frameDidChangeNotification:(NSNotification*)the_notification
{
  NSRect view_rect = NSZeroRect;
  id notification_object = [the_notification object];

  if ([notification_object isKindOfClass:[NSView class]])
  {
    /* the clip view bounds are the visible area regardless of scrollbar tiling */
    if ([notification_object isKindOfClass:[NSClipView class]])
    {
      view_rect.size = [(NSClipView*)notification_object bounds].size;
    }
    else
    {
      view_rect = [(NSView*)notification_object frame];
    }
  }
  else
  {
    return;
  }

  CGSize previous_size = [self previousSize];

  if(CGSizeEqualToSize(previous_size, view_rect.size))
  {
    return;
  }

  [self setPreviousSize:view_rect.size];

  if (iupAttribGet(_ih, "_IUP_GLCONTROLDATA") && !iupAttribGet(_ih, "_IUPCOCOA_GL_VIEW_ATTACHED"))
  {
    if (view_rect.size.width > 0 && view_rect.size.height > 0)
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      NSOpenGLContext* gl_context = (NSOpenGLContext*)iupAttribGet(_ih, "CONTEXT");
      if (gl_context)
      {
        if ([NSThread isMainThread])
          [gl_context setView:self];
        else
          dispatch_sync(dispatch_get_main_queue(), ^{ [gl_context setView:self]; });
        iupAttribSet(_ih, "_IUPCOCOA_GL_VIEW_ATTACHED", "1");
      }
#pragma clang diagnostic pop
    }
  }

  IFnii call_back = (IFnii)IupGetCallback(_ih, "RESIZE_CB");
  if(call_back && !_ih->data->inside_resize)
  {
    _ih->data->inside_resize = 1;

    int width, height;
    if (iupAttribGet(_ih, "_IUP_GLCONTROLDATA"))
    {
      NSRect backing_bounds = [self convertRectToBacking:[self bounds]];
      width = iupROUND(backing_bounds.size.width);
      height = iupROUND(backing_bounds.size.height);
    }
    else
    {
      width = iupROUND(view_rect.size.width);
      height = iupROUND(view_rect.size.height);
    }

    call_back(_ih, width, height);

    _ih->data->inside_resize = 0;
  }
}

- (void) globalFrameDidChangeNotification:(NSNotification*)the_notification
{
  if (_ih && iupAttribGet(_ih, "_IUP_GLCONTROLDATA"))
  {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NSOpenGLContext* gl_context = (NSOpenGLContext*)iupAttribGet(_ih, "CONTEXT");
    if (gl_context)
    {
      [gl_context makeCurrentContext];
      [gl_context update];
    }
#pragma clang diagnostic pop
  }
}

- (void) windowDidBecomeKeyNotification:(NSNotification*)the_notification
{
  [self setCurrentKeyWindow:true];
  [self updateFocus];
}

- (void) windowDidResignKeyNotification:(NSNotification*)the_notification
{
  [self setCurrentKeyWindow:false];
  [self updateFocus];
}

- (BOOL) acceptsFirstResponder
{
  return [self isEnabled];
}

- (BOOL) needsPanelToBecomeKey
{
  return YES;
}

- (BOOL) canBecomeKeyView
{
  return [self isEnabled];
}

- (BOOL) becomeFirstResponder
{
  if (![self isEnabled])
  {
    [self setCurrentFirstResponder:false];
    [self updateFocus];
    return NO;
  }

  BOOL accepted = [super becomeFirstResponder];
  if(accepted)
  {
    [self setCurrentFirstResponder:true];
    [self updateFocus];
  }
  return accepted;
}

- (BOOL) resignFirstResponder
{
  BOOL ret_val = [super resignFirstResponder];
  if(ret_val)
  {
    [self setCurrentFirstResponder:false];
    [self updateFocus];
  }
  return ret_val;
}

- (void) drawFocusRingMask
{
  if([self useNativeFocusRing])
  {
    NSRectFill([self bounds]);
  }
  else
  {
    NSRectFill(NSZeroRect);
  }
}

- (NSRect) focusRingMaskBounds
{
  if([self useNativeFocusRing])
  {
    return [self bounds];
  }
  return NSZeroRect;
}

- (void) updateFocus
{
  if([self isCurrentFirstResponder] && [[self window] isKeyWindow])
  {
    iupCallGetFocusCb(_ih);
  }
  else
  {
    iupCallKillFocusCb(_ih);
  }
  [self setNeedsDisplay:YES];
}

- (BOOL) acceptsFirstMouse:(NSEvent *)theEvent
{
  return YES;
}

- (void) updateTrackingAreas
{
  [super updateTrackingAreas];

  for (NSTrackingArea* area in [self trackingAreas])
  {
    if ([area owner] == self)
    {
      [self removeTrackingArea:area];
    }
  }

  NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
                                  NSTrackingMouseMoved |
                                  NSTrackingActiveInKeyWindow |
                                  NSTrackingInVisibleRect;
  NSTrackingArea* tracking_area = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                               options:options
                                                                 owner:self
                                                              userInfo:nil];
  [self addTrackingArea:tracking_area];
  [tracking_area release];
}

- (void) mouseEntered:(NSEvent*)the_event
{
  if (![self isEnabled]) return;

  IFn cb = (IFn)IupGetCallback(_ih, "ENTERWINDOW_CB");
  if (cb)
  {
    if (cb(_ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

- (void) mouseExited:(NSEvent*)the_event
{
  if (![self isEnabled]) return;

  IFn cb = (IFn)IupGetCallback(_ih, "LEAVEWINDOW_CB");
  if (cb)
  {
    if (cb(_ih) == IUP_CLOSE)
      IupExitLoop();
  }
}

- (void) flagsChanged:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  unsigned short mac_key_code = [the_event keyCode];
  if(!iupcocoaModifierEvent(_ih, the_event, (int)mac_key_code))
  {
    [super flagsChanged:the_event];
  }
}

/* Option composes a character, which is right for text but hides Meta from a terminal */
static BOOL cocoaCanvasOptionIsMeta(Ihandle* ih, NSEvent* the_event)
{
  NSEventModifierFlags flags;

  if (!iupAttribGetBoolean(ih, "OPTIONASMETA"))
    return NO;

  flags = [the_event modifierFlags];
  return (flags & NSEventModifierFlagOption) && !(flags & NSEventModifierFlagCommand);
}

- (void) keyDown:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if (IupGetCallback(_ih, "TEXTINPUT_CB") && !cocoaCanvasOptionIsMeta(_ih, the_event) &&
      !iup_isKeyPadXkey(iupcocoaKeyDecodeEvent(the_event, (int)[the_event keyCode])))
  {
    [self setTextInputConsumed:false];
#ifdef GNUSTEP
    /* the input manager never calls back into insertText:, so feed it the characters directly */
    [self insertText:[the_event characters] replacementRange:NSMakeRange(NSNotFound, 0)];
#else
    [[self inputContext] handleEvent:the_event];
#endif
    if ([self textInputConsumed])
      return;
    if ([self hasMarkedText])
      return;
  }

  unsigned short mac_key_code = [the_event keyCode];
  if(!iupcocoaKeyEvent(_ih, the_event, (int)mac_key_code, true))
  {
    [super keyDown:the_event];
  }
}

- (void) insertText:(id)a_string replacementRange:(NSRange)replacement_range
{
  NSString* text = [a_string isKindOfClass:[NSAttributedString class]] ? [a_string string] : a_string;
  (void)replacement_range;
  [self setTextInputMarked:nil];
  if ([text length] == 0)
    return;
  if ([text length] == 1 && ([text characterAtIndex:0] < 0x20 || [text characterAtIndex:0] == 0x7F))
    return;
  if (iupKeyCallTextInputCb(_ih, [text UTF8String]) == IUP_IGNORE)
    [self setTextInputConsumed:true];
}

- (void) doCommandBySelector:(SEL)a_selector
{
  (void)a_selector;
}

- (void) setMarkedText:(id)a_string selectedRange:(NSRange)selected_range replacementRange:(NSRange)replacement_range
{
  NSString* text = [a_string isKindOfClass:[NSAttributedString class]] ? [a_string string] : a_string;
  (void)selected_range;
  (void)replacement_range;
  [self setTextInputMarked:([text length] > 0) ? text : nil];
}

- (void) unmarkText
{
  [self setTextInputMarked:nil];
}

- (BOOL) hasMarkedText
{
  return [self textInputMarked] != nil;
}

- (NSRange) markedRange
{
  NSString* marked = [self textInputMarked];
  if (marked)
    return NSMakeRange(0, [marked length]);
  return NSMakeRange(NSNotFound, 0);
}

- (NSRange) selectedRange
{
  return NSMakeRange(0, 0);
}

- (NSArray<NSAttributedStringKey>*) validAttributesForMarkedText
{
  return [NSArray array];
}

- (NSAttributedString*) attributedSubstringForProposedRange:(NSRange)a_range actualRange:(NSRangePointer)actual_range
{
  (void)a_range;
  (void)actual_range;
  return nil;
}

- (NSRect) firstRectForCharacterRange:(NSRange)a_range actualRange:(NSRangePointer)actual_range
{
  NSRect rect = [self convertRect:[self bounds] toView:nil];
  (void)a_range;
  (void)actual_range;
  return [[self window] convertRectToScreen:rect];
}

- (NSUInteger) characterIndexForPoint:(NSPoint)a_point
{
  (void)a_point;
  return 0;
}

- (void) keyUp:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  unsigned short mac_key_code = [the_event keyCode];
  if(!iupcocoaKeyEvent(_ih, the_event, (int)mac_key_code, false))
  {
    [super keyUp:the_event];
  }
}

- (void) mouseDown:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if([cocoaSourceDragGetAssociatedData(_ih) isDragSourceEnabled])
  {
    NSPoint point = [self convertPoint:[the_event locationInWindow] fromView:nil];
    if(![self isFlipped])
      point.y = [self bounds].size.height - point.y;
    [self setDragStartPoint:point];
    [self setStartedDrag:false];
  }

  if(!iupcocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, true))
  {
    [super mouseDown:the_event];
  }
}

- (void) mouseMoved:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupcocoaCommonBaseHandleMouseMotionCallback(_ih, the_event, self))
  {
    [super mouseMoved:the_event];
  }
}

- (void) mouseDragged:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupcocoaCommonBaseHandleMouseMotionCallback(_ih, the_event, self))
  {
    [super mouseDragged:the_event];
  }

  /* start the drag past the threshold (no built-in drag on a custom view) */
  if(![self startedDrag] && [cocoaSourceDragGetAssociatedData(_ih) isDragSourceEnabled])
  {
    NSPoint point = [self convertPoint:[the_event locationInWindow] fromView:nil];
    if(![self isFlipped])
      point.y = [self bounds].size.height - point.y;

    CGFloat dx = point.x - [self dragStartPoint].x;
    CGFloat dy = point.y - [self dragStartPoint].y;
    if(dx*dx + dy*dy >= 25)
    {
      [self setStartedDrag:true];
      IupSetStrf(_ih, "DRAGSTART", "%d,%d", (int)[self dragStartPoint].x, (int)[self dragStartPoint].y);
    }
  }
}

- (void) mouseUp:(NSEvent*)the_event
{
  [self setStartedDrag:false];
  if(![self isEnabled]) return;

  if(!iupcocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, false))
  {
    [super mouseUp:the_event];
  }
}

- (void) rightMouseDown:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupcocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, true))
  {
    [super rightMouseDown:the_event];
  }
}

- (void) rightMouseDragged:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupcocoaCommonBaseHandleMouseMotionCallback(_ih, the_event, self))
  {
    [super rightMouseDragged:the_event];
  }
}

- (void) rightMouseUp:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupcocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, false))
  {
    [super rightMouseUp:the_event];
  }
}

- (void) otherMouseDown:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupcocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, true))
  {
    [super otherMouseDown:the_event];
  }
}

- (void) otherMouseDragged:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupcocoaCommonBaseHandleMouseMotionCallback(_ih, the_event, self))
  {
    [super otherMouseDragged:the_event];
  }
}

- (void) otherMouseUp:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupcocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, false))
  {
    [super otherMouseUp:the_event];
  }
}

- (void) _updateIupScrollState
{
  if (!_ih) return;

  NSScrollView* scroll_view = [self enclosingScrollView];
  if (![scroll_view isKindOfClass:[NSScrollView class]]) return;

  if (iupAttribGet(_ih, "_IUPCOCOA_UPDATING_SCROLL_POS"))
  {
    return;
  }

  double old_posx = _ih->data->posx;
  double old_posy = _ih->data->posy;

  double xmin = iupAttribGetDouble(_ih, "XMIN");
  double xmax = iupAttribGetDouble(_ih, "XMAX");
  double dx = iupAttribGetDouble(_ih, "DX");

  double ymin = iupAttribGetDouble(_ih, "YMIN");
  double ymax = iupAttribGetDouble(_ih, "YMAX");
  double dy = iupAttribGetDouble(_ih, "DY");

  double content_width = xmax - xmin;
  double content_height = ymax - ymin;

  double new_posx = old_posx;
  double new_posy = old_posy;

  if (_ih->data->sb & IUP_SB_HORIZ)
  {
    NSScroller* h_scroller = [scroll_view horizontalScroller];
    if (h_scroller && content_width > dx)
    {
      CGFloat double_value = [h_scroller doubleValue];
      double scrollable_range = content_width - dx;
      new_posx = xmin + (double_value * scrollable_range);

      double max_posx = xmax - dx;
      if (max_posx < xmin) max_posx = xmin;
      if (new_posx < xmin) new_posx = xmin;
      if (new_posx > max_posx) new_posx = max_posx;
    }
  }

  if (_ih->data->sb & IUP_SB_VERT)
  {
    NSScroller* v_scroller = [scroll_view verticalScroller];
    if (v_scroller && content_height > dy)
    {
      CGFloat double_value = [v_scroller doubleValue];
      double scrollable_range = content_height - dy;
      new_posy = ymin + (double_value * scrollable_range);

      double max_posy = ymax - dy;
      if (max_posy < ymin) max_posy = ymin;
      if (new_posy < ymin) new_posy = ymin;
      if (new_posy > max_posy) new_posy = max_posy;
    }
  }

  /* op from a scroller click; cleared so a later move can't reuse it */
  int forced_op = -1;
  if (iupAttribGet(_ih, "_IUPCOCOA_SBOP"))
  {
    forced_op = iupAttribGetInt(_ih, "_IUPCOCOA_SBOP");
    iupAttribSet(_ih, "_IUPCOCOA_SBOP", NULL);
  }

  double delta_x = new_posx - old_posx;
  double delta_y = new_posy - old_posy;

  if (fabs(delta_x) < 1e-6 && fabs(delta_y) < 1e-6)
  {
    return;
  }

  _ih->data->posx = new_posx;
  _ih->data->posy = new_posy;

  IFniff scroll_cb = (IFniff)IupGetCallback(_ih, "SCROLL_CB");
  if (scroll_cb)
  {
    int op = (forced_op >= 0) ? forced_op : ((fabs(delta_y) >= fabs(delta_x)) ? IUP_SBPOSV : IUP_SBPOSH);
    scroll_cb(_ih, op, (float)_ih->data->posx, (float)_ih->data->posy);
  }
  else
  {
    IFn action_cb = (IFn)IupGetCallback(_ih, "ACTION");
    if (action_cb)
    {
      iupdrvRedrawNow(_ih);
    }
  }
}

- (void) scrollWheel:(NSEvent*)the_event
{
  if (![self isEnabled])
  {
    [super scrollWheel:the_event];
    return;
  }

  if (iupAttribGetBoolean(_ih, "WHEELDROPFOCUS"))
  {
    Ihandle* ih_focus = IupGetFocus();
    if (iupObjectCheck(ih_focus))
      iupAttribSetClassObject(ih_focus, "SHOWDROPDOWN", "NO");
  }

  IFnfiis wheel_cb = (IFnfiis)IupGetCallback(_ih, "WHEEL_CB");
  if (wheel_cb)
  {
    iupcocoaCommonBaseScrollWheelCallback(_ih, the_event, self);
    return;
  }

  CGFloat deltaY = [the_event deltaY];
  CGFloat deltaX = [the_event deltaX];
  IFniff scroll_cb = (IFniff)IupGetCallback(_ih, "SCROLL_CB");

  if (fabs(deltaY) > 0.0)
  {
    double dy = iupAttribGetDouble(_ih, "DY");
    if (dy > 0.0)
    {
      double posy = _ih->data->posy;
      posy -= deltaY * dy / 10.0;
      IupSetDouble(_ih, "POSY", posy);

      if (scroll_cb)
      {
        int op = (deltaY > 0) ? IUP_SBUP : IUP_SBDN;
        scroll_cb(_ih, op, (float)_ih->data->posx, (float)_ih->data->posy);
      }
      else
      {
        IFn action_cb = (IFn)IupGetCallback(_ih, "ACTION");
        if (action_cb)
          iupdrvRedrawNow(_ih);
      }
    }
    else
    {
      [super scrollWheel:the_event];
    }
  }
  else if (fabs(deltaX) > 0.0)
  {
    double dx = iupAttribGetDouble(_ih, "DX");
    if (dx > 0.0)
    {
      double posx = _ih->data->posx;
      posx += deltaX * dx / 10.0;
      IupSetDouble(_ih, "POSX", posx);

      if (scroll_cb)
      {
        int op = (deltaX > 0) ? IUP_SBRIGHT : IUP_SBLEFT;
        scroll_cb(_ih, op, (float)_ih->data->posx, (float)_ih->data->posy);
      }
      else
      {
        IFn action_cb = (IFn)IupGetCallback(_ih, "ACTION");
        if (action_cb)
          iupdrvRedrawNow(_ih);
      }
    }
    else
    {
      [super scrollWheel:the_event];
    }
  }
  else
  {
    [super scrollWheel:the_event];
  }
}

- (NSDragOperation) draggingEntered:(id<NSDraggingInfo>)the_sender
{
  IupTargetDropAssociatedData* target_drop_data = cocoaTargetDropGetAssociatedData(_ih);
  NSArray* supported_types = [target_drop_data dropRegisteredTypes];
  NSPasteboard* paste_board = [the_sender draggingPasteboard];

  if([paste_board availableTypeFromArray:supported_types])
  {
    IFniis call_back = (IFniis)IupGetCallback(_ih, "DROPMOTION_CB");
    if(call_back)
    {
      NSPoint window_point = [the_sender draggingLocation];
      NSPoint view_point = [self convertPoint:window_point fromView:nil];

      char mod_status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      call_back(_ih, (int)view_point.x, (int)view_point.y, mod_status);
    }

    return iupAttribGetBoolean(_ih, "DRAGSOURCEMOVE") ? NSDragOperationMove : NSDragOperationCopy;
  }
  return NSDragOperationNone;
}

- (BOOL) performDragOperation:(id<NSDraggingInfo>)the_sender
{
  NSPasteboard* paste_board = [the_sender draggingPasteboard];
  NSPoint drop_point = [self convertPoint:[the_sender draggingLocation] fromView:nil];

  cocoaTargetDropBasePerformDropCallback(_ih, the_sender, paste_board, drop_point);
  return YES;
}

- (void) boundsDidChangeNotification:(NSNotification*)notification
{
  [self _updateIupScrollState];
}

- (void) scrollerAction:(id)sender
{
  NSScrollView* scroll_view = [self enclosingScrollView];
  if ([scroll_view isKindOfClass:[NSScrollView class]] && [sender isKindOfClass:[NSScroller class]])
  {
    NSScroller* scroller = (NSScroller*)sender;
    int is_vert = (scroller == [scroll_view verticalScroller]);
    int op = -1;
    /* track click = page op; macOS has no scroller arrows */
    switch ([scroller hitPart])
    {
      case NSScrollerDecrementPage: op = is_vert ? IUP_SBPGUP : IUP_SBPGLEFT; break;
      case NSScrollerIncrementPage: op = is_vert ? IUP_SBPGDN : IUP_SBPGRIGHT; break;
      default: break;
    }
    if (op >= 0)
      iupAttribSetInt(_ih, "_IUPCOCOA_SBOP", op);
  }
  [self _updateIupScrollState];
}

@end


static NSScrollView* cocoaCanvasGetScrollView(Ihandle* ih)
{
  if(iupAttribGetBoolean(ih, "_IUPCOCOA_CANVAS_HAS_SCROLLBAR"))
  {
    NSScrollView* scroll_view = (NSScrollView*)iupAttribGet(ih, "_IUPCOCOA_CANVAS_ROOT");
    NSCAssert([scroll_view isKindOfClass:[NSScrollView class]], @"Expected NSScrollView");
    return scroll_view;
  }
  return nil;
}

static IupCocoaCanvasView* cocoaCanvasGetCanvasView(Ihandle* ih)
{
  IupCocoaCanvasView* canvas_view = (IupCocoaCanvasView*)iupAttribGet(ih, "_IUPCOCOA_CANVAS_VIEW");
  NSCAssert([canvas_view isKindOfClass:[IupCocoaCanvasView class]], @"Expected IupCocoaCanvasView");
  return canvas_view;
}

static int cocoaCanvasSetBgColorAttrib(Ihandle* ih, const char* value)
{
  IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    NSColor* the_color = [NSColor colorWithSRGBRed:(CGFloat)r/255.0 green:(CGFloat)g/255.0 blue:(CGFloat)b/255.0 alpha:1.0];
    [canvas_view setBackgroundColor:the_color];
    [canvas_view setNeedsDisplay:YES];
  }
  return 1;
}

static int cocoaCanvasSetUpdateRectAttrib(Ihandle* ih, const char* value)
{
  int x1, y1, x2, y2;
  if (value && !iupAttribGet(ih, "_IUP_GLCONTROLDATA") && sscanf(value, "%d %d %d %d", &x1, &y1, &x2, &y2) == 4)
  {
    IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
    [canvas_view setNeedsDisplayInRect:NSMakeRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1)];
  }
  else
    iupdrvPostRedraw(ih);
  return 0;
}

static void cocoaCanvasUpdateDocumentSize(Ihandle* ih)
{
  NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);
  IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
  if (!scroll_view || !canvas_view) return;
  if (ih->currentwidth <= 0 || ih->currentheight <= 0) return;

  NSSize frame_size = NSMakeSize(ih->currentwidth, ih->currentheight);
#ifdef GNUSTEP
  NSSize content_size = [NSScrollView contentSizeForFrameSize:frame_size
                                        hasHorizontalScroller:[scroll_view hasHorizontalScroller]
                                          hasVerticalScroller:[scroll_view hasVerticalScroller]
                                                   borderType:[scroll_view borderType]];
#else
  NSSize content_size = [NSScrollView contentSizeForFrameSize:frame_size
                                      horizontalScrollerClass:([scroll_view hasHorizontalScroller] ? [NSScroller class] : nil)
                                        verticalScrollerClass:([scroll_view hasVerticalScroller] ? [NSScroller class] : nil)
                                                   borderType:[scroll_view borderType]
                                                  controlSize:NSControlSizeRegular
                                                scrollerStyle:[NSScroller preferredScrollerStyle]];
#endif
  if (!NSEqualSizes([canvas_view frame].size, content_size))
    [canvas_view setFrameSize:content_size];
}

static int cocoaCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);
    if (!scroll_view) return 0;

    double dx;
    if (value) iupStrToDouble(value, &dx);
    else dx = iupAttribGetDouble(ih, "DX");

    double xmin = iupAttribGetDouble(ih, "XMIN");
    double xmax = iupAttribGetDouble(ih, "XMAX");
    double content_width = xmax - xmin;

    if (dx >= content_width)
    {
      if (iupAttribGetBoolean(ih, "XAUTOHIDE"))
      {
        [scroll_view setHasHorizontalScroller:NO];
        iupAttribSet(ih, "SB_RESIZE", "YES");
      }
      else
      {
        [scroll_view setHasHorizontalScroller:YES];
        [[scroll_view horizontalScroller] setEnabled:NO];
      }
      iupAttribSet(ih, "XHIDDEN", "YES");
      if (ih->data->posx != xmin) IupSetDouble(ih, "POSX", xmin);
    }
    else
    {
      [scroll_view setHasHorizontalScroller:YES];
      [[scroll_view horizontalScroller] setEnabled:YES];
      iupAttribSet(ih, "SB_RESIZE", "YES");
      iupAttribSet(ih, "XHIDDEN", "NO");

      [scroll_view tile];

      NSScroller* scroller = [scroll_view horizontalScroller];
      if (scroller && content_width > 0)
      {
        CGFloat knob_proportion = (CGFloat)(dx / content_width);
        [scroller setKnobProportion:knob_proportion];
      }

      [scroll_view setNeedsDisplay:YES];
    }
    cocoaCanvasUpdateDocumentSize(ih);
  }
  return 1;
}

static int cocoaCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);
    if (!scroll_view) return 0;

    double dy;
    if (value) iupStrToDouble(value, &dy);
    else dy = iupAttribGetDouble(ih, "DY");

    double ymin = iupAttribGetDouble(ih, "YMIN");
    double ymax = iupAttribGetDouble(ih, "YMAX");
    double content_height = ymax - ymin;

    if (dy >= content_height)
    {
      if (iupAttribGetBoolean(ih, "YAUTOHIDE"))
      {
        [scroll_view setHasVerticalScroller:NO];
        iupAttribSet(ih, "SB_RESIZE", "YES");
      }
      else
      {
        [scroll_view setHasVerticalScroller:YES];
        [[scroll_view verticalScroller] setEnabled:NO];
      }
      iupAttribSet(ih, "YHIDDEN", "YES");
      if (ih->data->posy != ymin) IupSetDouble(ih, "POSY", ymin);
    }
    else
    {
      [scroll_view setHasVerticalScroller:YES];
      [[scroll_view verticalScroller] setEnabled:YES];
      iupAttribSet(ih, "SB_RESIZE", "YES");
      iupAttribSet(ih, "YHIDDEN", "NO");

      [scroll_view tile];

      NSScroller* scroller = [scroll_view verticalScroller];
      if (scroller && content_height > 0)
      {
        CGFloat knob_proportion = (CGFloat)(dy / content_height);
        [scroller setKnobProportion:knob_proportion];
      }

      [scroll_view setNeedsDisplay:YES];
    }
    cocoaCanvasUpdateDocumentSize(ih);
  }
  return 1;
}

static int cocoaCanvasSetPosXAttrib(Ihandle* ih, const char* value)
{
  if (!(ih->data->sb & IUP_SB_HORIZ)) return 1;

  NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);
  if (!scroll_view) return 0;

  double posx;
  if (!iupStrToDouble(value, &posx)) return 1;

  double xmin = iupAttribGetDouble(ih, "XMIN");
  double xmax = iupAttribGetDouble(ih, "XMAX");
  double dx = iupAttribGetDouble(ih, "DX");

  double content_width = xmax - xmin;
  double max_posx = xmax - dx;

  if (dx >= content_width)
  {
    posx = xmin;
  }

  if (posx < xmin) posx = xmin;
  if (posx > max_posx)
  {
    if (max_posx < xmin) max_posx = xmin;
    posx = max_posx;
  }

  ih->data->posx = posx;

  NSScroller* scroller = [scroll_view horizontalScroller];
  if (scroller && content_width > dx && dx > 0)
  {
    double scrollable_range = content_width - dx;
    CGFloat double_value = (CGFloat)((posx - xmin) / scrollable_range);

    if (double_value < 0.0) double_value = 0.0;
    if (double_value > 1.0) double_value = 1.0;

    iupAttribSet(ih, "_IUPCOCOA_UPDATING_SCROLL_POS", "1");
    [scroller setDoubleValue:double_value];
    iupAttribSet(ih, "_IUPCOCOA_UPDATING_SCROLL_POS", NULL);

    IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
    [canvas_view setNeedsDisplay:YES];
  }

  return 1;
}

static int cocoaCanvasSetPosYAttrib(Ihandle* ih, const char* value)
{
  if (!(ih->data->sb & IUP_SB_VERT)) return 1;

  NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);
  if (!scroll_view) return 0;

  double posy;
  if (!iupStrToDouble(value, &posy)) return 1;

  double ymin = iupAttribGetDouble(ih, "YMIN");
  double ymax = iupAttribGetDouble(ih, "YMAX");
  double dy = iupAttribGetDouble(ih, "DY");

  double content_height = ymax - ymin;
  double max_posy = ymax - dy;

  if (dy >= content_height)
  {
    posy = ymin;
  }

  if (posy < ymin) posy = ymin;
  if (posy > max_posy)
  {
    if (max_posy < ymin) max_posy = ymin;
    posy = max_posy;
  }

  ih->data->posy = posy;

  NSScroller* scroller = [scroll_view verticalScroller];
  if (scroller && content_height > dy && dy > 0)
  {
    double scrollable_range = content_height - dy;
    CGFloat double_value = (CGFloat)((posy - ymin) / scrollable_range);

    if (double_value < 0.0) double_value = 0.0;
    if (double_value > 1.0) double_value = 1.0;

    iupAttribSet(ih, "_IUPCOCOA_UPDATING_SCROLL_POS", "1");
    [scroller setDoubleValue:double_value];
    iupAttribSet(ih, "_IUPCOCOA_UPDATING_SCROLL_POS", NULL);

    IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
    [canvas_view setNeedsDisplay:YES];
  }

  return 1;
}

static char* cocoaCanvasGetCGContextAttrib(Ihandle* ih)
{
  (void)ih;
  CGContextRef cg_context = [[NSGraphicsContext currentContext] CGContext];
  return (char*)cg_context;
}

static char* cocoaCanvasGetDrawableAttrib(Ihandle* ih)
{
  return (char*)cocoaCanvasGetCGContextAttrib(ih);
}

static char* cocoaCanvasGetNSViewAttrib(Ihandle* ih)
{
  IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
  return (char*)canvas_view;
}

static char* cocoaCanvasGetDrawSizeAttrib(Ihandle *ih)
{
  IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
  NSRect the_frame = [canvas_view bounds];
  int w = iupROUND(the_frame.size.width);
  int h = iupROUND(the_frame.size.height);
  return iupStrReturnIntInt(w, h, 'x');
}

static char* cocoaCanvasGetNativeFocusRingAttrib(Ihandle* ih)
{
  IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
  return iupStrReturnBoolean([canvas_view useNativeFocusRing]);
}

static char* cocoaCanvasGetScrollVisibleAttrib(Ihandle* ih)
{
  NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);
  if (!scroll_view)
    return "NO";

  int scroll_visible = 0;

  NSScroller* horiz_scroller = [scroll_view horizontalScroller];
  NSScroller* vert_scroller = [scroll_view verticalScroller];

  if (horiz_scroller && ![horiz_scroller isHidden])
    scroll_visible |= 1;
  if (vert_scroller && ![vert_scroller isHidden])
    scroll_visible |= 2;

  if (scroll_visible == 3)
    return "YES";
  else if (scroll_visible == 1)
    return "HORIZONTAL";
  else if (scroll_visible == 2)
    return "VERTICAL";
  else
    return "NO";
}

static int cocoaCanvasSetNativeFocusRingAttrib(Ihandle* ih, const char* value)
{
  IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
  [canvas_view setUseNativeFocusRing:(bool)iupStrBoolean(value)];
  return 1;
}

static int cocoaCanvasMapMethod(Ihandle* ih)
{
  IupCocoaFixedView* extra_parent = [[IupCocoaFixedView alloc] initWithFrame:NSZeroRect];

  NSView* root_view = nil;
  IupCocoaCanvasView* canvas_view = [[IupCocoaCanvasView alloc] initWithFrame:NSZeroRect ih:ih];
  iupAttribSet(ih, "_IUPCOCOA_CANVAS_VIEW", (char*)canvas_view);

  ih->data->sb = iupBaseGetScrollbar(ih);

  if (ih->data->sb ||
      iupStrEqual(ih->iclass->name, "flatscrollbox") ||
      iupStrEqual(ih->iclass->name, "scrollbox"))
  {
    [extra_parent setClipsToBounds:YES];
  }

  NSNotificationCenter* notification_center = [NSNotificationCenter defaultCenter];

  if (ih->data->sb)
  {
    NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSZeroRect];

    IupLogicalScrollClipView* clip_view = [[IupLogicalScrollClipView alloc] initWithFrame:NSZeroRect];
    [clip_view setDocumentView:nil];
    [scroll_view setContentView:clip_view];
    [clip_view release];

    /* the document view size is managed from XMAX/YMAX; autoresizing it would leave nothing to scroll */
    [canvas_view setAutoresizingMask:NSViewNotSizable];

    [scroll_view setDocumentView:canvas_view];
    [canvas_view release];

    [scroll_view setAutohidesScrollers:NO];


    [scroll_view setHasHorizontalScroller:(ih->data->sb & IUP_SB_HORIZ)];
    [scroll_view setHorizontalScrollElasticity:NSScrollElasticityNone];
    [scroll_view setHasVerticalScroller:(ih->data->sb & IUP_SB_VERT)];
    [scroll_view setVerticalScrollElasticity:NSScrollElasticityNone];
    [scroll_view setBorderType:iupAttribGetBoolean(ih, "BORDER") ? NSBezelBorder : NSNoBorder];
    [scroll_view setDrawsBackground:NO];

    if (ih->data->sb & IUP_SB_HORIZ)
    {
      NSScroller* h_scroller = [scroll_view horizontalScroller];
      [h_scroller setTarget:canvas_view];
      [h_scroller setAction:@selector(scrollerAction:)];
    }
    if (ih->data->sb & IUP_SB_VERT)
    {
      NSScroller* v_scroller = [scroll_view verticalScroller];
      [v_scroller setTarget:canvas_view];
      [v_scroller setAction:@selector(scrollerAction:)];
    }

    [scroll_view setPostsFrameChangedNotifications:YES];
    [notification_center addObserver:canvas_view
                            selector:@selector(frameDidChangeNotification:)
                                name:NSViewFrameDidChangeNotification
                              object:scroll_view];

    [notification_center addObserver:canvas_view
                            selector:@selector(globalFrameDidChangeNotification:)
                                name:NSWindowDidMoveNotification
                              object:nil];
    [notification_center addObserver:canvas_view
                            selector:@selector(globalFrameDidChangeNotification:)
                                name:NSWindowDidChangeScreenNotification
                              object:nil];

    [clip_view setPostsBoundsChangedNotifications:YES];
    [notification_center addObserver:canvas_view
                            selector:@selector(boundsDidChangeNotification:)
                                name:NSViewBoundsDidChangeNotification
                              object:clip_view];

    root_view = scroll_view;
    iupAttribSet(ih, "_IUPCOCOA_CANVAS_HAS_SCROLLBAR", "1");
  }
  else
  {
    [canvas_view setPostsFrameChangedNotifications:YES];
    [notification_center addObserver:canvas_view
                            selector:@selector(frameDidChangeNotification:)
                                name:NSViewFrameDidChangeNotification
                              object:canvas_view];

    [notification_center addObserver:canvas_view
                            selector:@selector(globalFrameDidChangeNotification:)
                                name:NSWindowDidMoveNotification
                              object:nil];
    [notification_center addObserver:canvas_view
                            selector:@selector(globalFrameDidChangeNotification:)
                                name:NSWindowDidChangeScreenNotification
                              object:nil];

    [canvas_view setPostsBoundsChangedNotifications:YES];
    root_view = canvas_view;
  }

  [extra_parent addSubview:root_view];

  ih->handle = extra_parent;
  iupAttribSet(ih, "_IUP_EXTRAPARENT", (char*)extra_parent);
  iupAttribSet(ih, "_IUPCOCOA_CANVAS_ROOT", (char*)root_view);

  iupcocoaSetAssociatedViews(ih, canvas_view, extra_parent);

  iupcocoaAddToParent(ih);

  IupSourceDragAssociatedData* source_drag = cocoaSourceDragCreateAssociatedData(ih, canvas_view, root_view);
  cocoaTargetDropCreateAssociatedData(ih, canvas_view, root_view);
  [source_drag setDefaultFilePromiseName:@"IupCanvas.png"];

  cocoaCanvasSetBgColorAttrib(ih, iupAttribGet(ih, "BGCOLOR"));

  cocoaCanvasSetDXAttrib(ih, NULL);
  cocoaCanvasSetDYAttrib(ih, NULL);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  return IUP_NOERROR;
}

static void cocoaCanvasUnMapMethod(Ihandle* ih)
{
  IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);

  if (canvas_view)
  {
    [[NSNotificationCenter defaultCenter] removeObserver:canvas_view];
  }

  if (ih->data->sb)
  {
    NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);
    if (scroll_view)
    {
      NSScroller* h_scroller = [scroll_view horizontalScroller];
      NSScroller* v_scroller = [scroll_view verticalScroller];
      if (h_scroller)
        [h_scroller setTarget:nil];
      if (v_scroller)
        [v_scroller setTarget:nil];
    }
  }

  cocoaTargetDropDestroyAssociatedData(ih);
  cocoaSourceDragDestroyAssociatedData(ih);

  Ihandle* context_menu_ih = (Ihandle*)iupcocoaCommonBaseGetContextMenuAttrib(ih);
  if(context_menu_ih)
  {
    IupDestroy(context_menu_ih);
  }
  iupcocoaCommonBaseSetContextMenuAttrib(ih, NULL);

  NSBitmapImageRep* canvas_buffer = (NSBitmapImageRep*)iupAttribGet(ih, "_IUPCOCOA_CANVAS_BUFFER");
  if (canvas_buffer)
  {
    [canvas_buffer release];
    iupAttribSet(ih, "_IUPCOCOA_CANVAS_BUFFER", NULL);
  }

  iupcocoaRemoveFromParent(ih);
  iupcocoaSetAssociatedViews(ih, nil, nil);

  NSView* extra_parent = (NSView*)ih->handle;
  if (extra_parent)
  {
    [extra_parent release];
  }

  ih->handle = NULL;
  iupAttribSet(ih, "_IUPCOCOA_CANVAS_VIEW", NULL);
  iupAttribSet(ih, "_IUP_EXTRAPARENT", NULL);
  iupAttribSet(ih, "_IUPCOCOA_CANVAS_ROOT", NULL);
}

static void cocoaCanvasLayoutUpdateMethod(Ihandle *ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  NSView* canvas_root = (NSView*)iupAttribGet(ih, "_IUPCOCOA_CANVAS_ROOT");

  if (ih->data->sb)
  {
    /* the document view must be sized first; setting the scroll view frame fires RESIZE_CB */
    cocoaCanvasUpdateDocumentSize(ih);
  }

  if (canvas_root)
  {
    [canvas_root setFrame:NSMakeRect(0, 0, ih->currentwidth, ih->currentheight)];
  }

  if (ih->data->sb)
  {
    cocoaCanvasSetDXAttrib(ih, NULL);
    cocoaCanvasSetDYAttrib(ih, NULL);
  }
}

static void cocoaCanvasComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *expand)
{
  int natural_w = ih->naturalwidth;
  int natural_h = ih->naturalheight;

  if (natural_w == 0) natural_w = 1;
  if (natural_h == 0) natural_h = 1;

  *w = natural_w;
  *h = natural_h;
  *expand = iupAttribGetBoolean(ih, "EXPAND");

  if (ih->data->sb)
  {
    int sb_size = iupdrvGetScrollbarSize();
    if ((ih->data->sb & IUP_SB_VERT) && !iupAttribGetBoolean(ih, "YHIDDEN"))
      *w += sb_size;
    if ((ih->data->sb & IUP_SB_HORIZ) && !iupAttribGetBoolean(ih, "XHIDDEN"))
      *h += sb_size;
  }
}

static void* cocoaCanvasGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  NSView* extra_parent = (NSView*)iupAttribGet(ih, "_IUP_EXTRAPARENT");
  if (extra_parent)
    return (void*)extra_parent;
  return ih->handle;
}

IUP_SDK_API void iupdrvCanvasInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = cocoaCanvasMapMethod;
  ic->UnMap = cocoaCanvasUnMapMethod;
  ic->LayoutUpdate = cocoaCanvasLayoutUpdateMethod;
  ic->ComputeNaturalSize = cocoaCanvasComputeNaturalSizeMethod;
  ic->GetInnerNativeContainerHandle = cocoaCanvasGetInnerNativeContainerHandleMethod;

  iupClassRegisterCallback(ic, "GESTURE_CB", "iiiidd");

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaCanvasSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* IupCanvas only */
  iupClassRegisterAttribute(ic, "DRAWSIZE", cocoaCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "UPDATERECT", NULL, cocoaCanvasSetUpdateRectAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DX", NULL, cocoaCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", NULL, cocoaCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, cocoaCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, cocoaCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XAUTOHIDE", NULL, NULL, "YES", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YAUTOHIDE", NULL, NULL, "YES", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", cocoaCanvasGetScrollVisibleAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAWABLE", cocoaCanvasGetDrawableAttrib, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CGCONTEXT", cocoaCanvasGetCGContextAttrib, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NSVIEW", cocoaCanvasGetNSViewAttrib, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT|IUPAF_READONLY);

  iupClassRegisterAttribute(ic, "NATIVEFOCUSRING", cocoaCanvasGetNativeFocusRingAttrib, cocoaCanvasSetNativeFocusRingAttrib, "NO", NULL, IUPAF_NO_INHERIT);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, "YES", NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOUCH", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
