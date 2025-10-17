/** \file
 * \brief Canvas Control for Cocoa
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_canvas.h"
#include "iup_key.h"
#include "iup_class.h"
#include "iup_focus.h"

#include "iupcocoa_draw.h"
#import "iupcocoa_canvas.h"
#include "iupcocoa_drv.h"
#import "iupcocoa_dragdrop.h"


static int cocoaCanvasSetBeginDragAttrib(Ihandle* ih, const char* value);
static void cocoaCanvasLayoutUpdateMethod(Ihandle *ih);
static int cocoaCanvasSetDXAttrib(Ihandle* ih, const char* value);
static int cocoaCanvasSetDYAttrib(Ihandle* ih, const char* value);
static void cocoaCanvasComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *expand);

@implementation IupCocoaCanvasView

- (instancetype) initWithFrame:(NSRect)frame_rect ih:(Ihandle*)ih
{
  self = [super initWithFrame:frame_rect];
  if(self)
  {
    _ih = ih;
    [self setEnabled:YES];
  }
  return self;
}

- (void) dealloc
{
  NSNotificationCenter* notification_center = [NSNotificationCenter defaultCenter];
  [notification_center removeObserver:self];
  [self setBackgroundColor:nil]; // Releases the color object
  [super dealloc];
}

- (BOOL) isFlipped
{
  return YES;
}

- (void) viewWillMoveToWindow:(NSWindow*)newWindow
{
  [super viewWillMoveToWindow:newWindow];

  // Remove observers from the old window before moving to a new one (or to nil).
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

  // Add observers to the new window.
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
  }
}

- (void) drawRect:(NSRect)dirty_rect
{
  IFnff call_back = (IFnff)IupGetCallback(_ih, "ACTION");

  if (call_back)
  {
    // To ensure cross-platform compatibility with IUP's standard behavior,
    // the drawing context provided to the ACTION callback must be configured such that
    // (0,0) corresponds to the top-left corner of the VISIBLE area.
    // The coordinate system must be Top-Down (which it is, as the view is flipped).

    NSGraphicsContext* context = [NSGraphicsContext currentContext];
    [context saveGraphicsState];

    // Get the scroll offset (top-left visible coordinate in virtual space).
    // We use the stored IUP POSX/POSY, which are synchronized with the visible rect.
    double xmin = iupAttribGetDouble(_ih, "XMIN");
    double ymin = iupAttribGetDouble(_ih, "YMIN");
    double offsetX = _ih->data->posx - xmin;
    double offsetY = _ih->data->posy - ymin;

    // Apply the translation: T(x_app) = x_virt = x_app + OffsetX.
    // This ensures that when the application draws at (V - POS), it appears correctly on screen.
    CGContextRef cgContext = [context CGContext];
    CGContextTranslateCTM(cgContext, offsetX, offsetY);

    // Set the CLIPRECT attribute, relative to the visible area (0,0).
    // The dirty_rect is in virtual coordinates, so we translate it back.
    NSRect clip_rect = dirty_rect;
    clip_rect.origin.x -= offsetX;
    clip_rect.origin.y -= offsetY;

    // IUP CLIPRECT definition: If the rectangle is empty, the attribute returns NULL.
    if (clip_rect.size.width <= 0 || clip_rect.size.height <= 0)
    {
      iupAttribSet(_ih, "CLIPRECT", NULL);
    }
    else
    {
      // IUP uses inclusive integer coordinates for the end point (x2, y2).
      // We use floor/ceil to ensure we cover the pixel area correctly and subtract 1 for inclusive end coordinate.
      double x1 = floor(clip_rect.origin.x);
      double y1 = floor(clip_rect.origin.y);
      double x2 = ceil(clip_rect.origin.x + clip_rect.size.width) - 1;
      double y2 = ceil(clip_rect.origin.y + clip_rect.size.height) - 1;

      // Ensure x2>=x1 and y2>=y1 due to floating point precision or edge cases.
      if (x2 < x1) x2 = x1;
      if (y2 < y1) y2 = y1;

      iupAttribSetStrf(_ih, "CLIPRECT", "%.0f %.0f %.0f %.0f", x1, y1, x2, y2);
    }

    // The view is flipped, so the graphics context's coordinate system is
    // already top-down, matching IUP. The ACTION callback receives posx/posy
    // to handle scrolling offsets.
    call_back(_ih, (float)_ih->data->posx, (float)_ih->data->posy);

    iupAttribSet(_ih, "CLIPRECT", NULL);
    [context restoreGraphicsState];
  }
  else
  {
    // If there is no ACTION callback, we are responsible for drawing the background.
    // This drawing happens in virtual coordinates (no translation needed).
    if ([self backgroundColor])
    {
      [[self backgroundColor] set];
      NSRectFill(dirty_rect);
    }
  }
}

- (void) frameDidChangeNotification:(NSNotification*)the_notification
{
  NSRect view_rect = NSZeroRect; // Use view_rect to hold the size
  id notification_object = [the_notification object];

  // The RESIZE_CB should report the size of the visible area.
  // The notification object is either the IupCocoaCanvasView itself (standalone)
  // or the NSClipView (when inside an NSScrollView), as configured in cocoaCanvasMapMethod.

  if ([notification_object isKindOfClass:[NSView class]])
  {
    // When inside a scroll view (observing NSClipView), we must use its bounds size,
    // as it correctly represents the visible area regardless of scrollbar presence (tiling).
    if ([notification_object isKindOfClass:[NSClipView class]])
    {
      view_rect.size = [(NSClipView*)notification_object bounds].size;
    }
    else
    {
      // Standalone canvas, use the frame size.
      view_rect = [(NSView*)notification_object frame];
    }
  }
  else
  {
    // Should not happen based on how observers are registered.
    NSLog(@"frameDidChangeNotification: unexpected notification object.");
    return;
  }

  CGSize previous_size = [self previousSize];

  if(CGSizeEqualToSize(previous_size, view_rect.size))
  {
    // The view was moved, but not resized, OR the resize was already handled synchronously by cocoaCanvasLayoutUpdateMethod.
    return;
  }

  [self setPreviousSize:view_rect.size];

  IFnii call_back = (IFnii)IupGetCallback(_ih, "RESIZE_CB");
  if(call_back && !_ih->data->inside_resize)
  {
    // Set inside_resize flag to prevent recursion.
    _ih->data->inside_resize = 1;

    // Use iupROUND instead of casting to (int) to avoid truncation issues with fractional sizes (e.g. HiDPI).
    call_back(_ih, iupROUND(view_rect.size.width), iupROUND(view_rect.size.height));

    _ih->data->inside_resize = 0;
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

- (void) flagsChanged:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  unsigned short mac_key_code = [the_event keyCode];
  if(!iupCocoaModifierEvent(_ih, the_event, (int)mac_key_code))
  {
    [super flagsChanged:the_event];
  }
}

- (void) keyDown:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  unsigned short mac_key_code = [the_event keyCode];
  if(!iupCocoaKeyEvent(_ih, the_event, (int)mac_key_code, true))
  {
    [super keyDown:the_event];
  }
}

- (void) keyUp:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  unsigned short mac_key_code = [the_event keyCode];
  if(!iupCocoaKeyEvent(_ih, the_event, (int)mac_key_code, false))
  {
    [super keyUp:the_event];
  }
}

- (void) mouseDown:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupCocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, true))
  {
    [super mouseDown:the_event];
  }
}

- (void) mouseDragged:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupCocoaCommonBaseHandleMouseMotionCallback(_ih, the_event, self))
  {
    [super mouseDragged:the_event];
  }

  // Handle automatic drag initiation if enabled
  if(([the_event associatedEventsMask] & NSLeftMouseDragged) && ![self startedDrag])
  {
    IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(_ih);
    if([drag_source_data isDragSourceEnabled] && iupAttribGetBoolean(_ih, "AUTOBEGINDRAG"))
    {
      cocoaCanvasSetBeginDragAttrib(_ih, NULL);
    }
  }
}

- (void) mouseUp:(NSEvent*)the_event
{
  [self setStartedDrag:false];
  if(![self isEnabled]) return;

  if(!iupCocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, false))
  {
    [super mouseUp:the_event];
  }
}

- (void) rightMouseDown:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupCocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, true))
  {
    [super rightMouseDown:the_event];
  }
}

- (void) rightMouseDragged:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupCocoaCommonBaseHandleMouseMotionCallback(_ih, the_event, self))
  {
    [super rightMouseDragged:the_event];
  }
}

- (void) rightMouseUp:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupCocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, false))
  {
    [super rightMouseUp:the_event];
  }
}

- (void) otherMouseDown:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupCocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, true))
  {
    [super otherMouseDown:the_event];
  }
}

- (void) otherMouseDragged:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupCocoaCommonBaseHandleMouseMotionCallback(_ih, the_event, self))
  {
    [super otherMouseDragged:the_event];
  }
}

- (void) otherMouseUp:(NSEvent*)the_event
{
  if(![self isEnabled]) return;

  if(!iupCocoaCommonBaseHandleMouseButtonCallback(_ih, the_event, self, false))
  {
    [super otherMouseUp:the_event];
  }
}

- (void) _updateIupScrollState
{
  if (!_ih) return;

  NSClipView* clip_view = [[self enclosingScrollView] contentView];
  if (![clip_view isKindOfClass:[NSClipView class]]) return;

  NSRect visible_rect = [clip_view documentVisibleRect];

  double old_posx = _ih->data->posx;
  double old_posy = _ih->data->posy;

  double ymin = iupAttribGetDouble(_ih, "YMIN");
  double xmin = iupAttribGetDouble(_ih, "XMIN");

  double new_posx = xmin + visible_rect.origin.x;
  double new_posy = ymin + visible_rect.origin.y;

  double xmax = iupAttribGetDouble(_ih, "XMAX");
  double dx = iupAttribGetDouble(_ih, "DX");
  double max_posx = xmax - dx;

  double ymax = iupAttribGetDouble(_ih, "YMAX");
  double dy = iupAttribGetDouble(_ih, "DY");
  double max_posy = ymax - dy;

  if (max_posx < xmin) max_posx = xmin;
  if (max_posy < ymin) max_posy = ymin;

  if (new_posx < xmin) new_posx = xmin;
  if (new_posx > max_posx) new_posx = max_posx;
  if (new_posy < ymin) new_posy = ymin;
  if (new_posy > max_posy) new_posy = max_posy;

  double delta_x = new_posx - old_posx;
  double delta_y = new_posy - old_posy;

  if (fabs(delta_x) < 1e-6 && fabs(delta_y) < 1e-6)
  {
    return;
  }

  _ih->data->posx = new_posx;
  _ih->data->posy = new_posy;

  if (iupAttribGet(_ih, "_IUPCOCOA_UPDATING_SCROLL_POS"))
  {
    return;
  }

  IFniff scroll_cb = (IFniff)IupGetCallback(_ih, "SCROLL_CB");
  if (scroll_cb)
  {
    int op = (fabs(delta_y) >= fabs(delta_x)) ? IUP_SBPOSV : IUP_SBPOSH;
    scroll_cb(_ih, op, (float)_ih->data->posx, (float)_ih->data->posy);
  }
  else
  {
    IFnff action_cb = (IFnff)IupGetCallback(_ih, "ACTION");
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
    iupCocoaCommonBaseScrollWheelCallback(_ih, the_event, self);
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
        IFnff action_cb = (IFnff)IupGetCallback(_ih, "ACTION");
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
        IFnff action_cb = (IFnff)IupGetCallback(_ih, "ACTION");
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

@end


static NSScrollView* cocoaCanvasGetScrollView(Ihandle* ih)
{
  if(iupAttribGetBoolean(ih, "_IUPCOCOA_CANVAS_HAS_SCROLLBAR"))
  {
    NSScrollView* scroll_view = (NSScrollView*)ih->handle;
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
    NSColor* the_color = [NSColor colorWithCalibratedRed:(CGFloat)r/255.0 green:(CGFloat)g/255.0 blue:(CGFloat)b/255.0 alpha:1.0];
    [canvas_view setBackgroundColor:the_color];
    [canvas_view setNeedsDisplay:YES];
  }
  return 1;
}

static int cocoaCanvasSetDXAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_HORIZ)
  {
    NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);
    if (!scroll_view) return 0;

    iupAttribSet(ih, "SB_RESIZE", NULL);

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
        if ([scroll_view hasHorizontalScroller])
        {
          [scroll_view setHasHorizontalScroller:NO];
          iupAttribSet(ih, "SB_RESIZE", "YES");
        }
      }
      else
      {
        if(![scroll_view hasHorizontalScroller])
        {
          [scroll_view setHasHorizontalScroller:YES];
          iupAttribSet(ih, "SB_RESIZE", "YES");
        }
        [[scroll_view horizontalScroller] setEnabled:NO];
      }
      iupAttribSet(ih, "XHIDDEN", "YES");
      if (ih->data->posx != xmin) IupSetDouble(ih, "POSX", xmin);
    }
    else
    {
      if (![scroll_view hasHorizontalScroller])
      {
        [scroll_view setHasHorizontalScroller:YES];
        iupAttribSet(ih, "SB_RESIZE", "YES");
      }
      [[scroll_view horizontalScroller] setEnabled:YES];
      iupAttribSet(ih, "XHIDDEN", "NO");
    }

    if (iupAttribGet(ih, "SB_RESIZE"))
    {
      [scroll_view tile];
      [scroll_view setNeedsDisplay:YES];
    }
  }
  return 1;
}

static int cocoaCanvasSetDYAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->sb & IUP_SB_VERT)
  {
    NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);
    if (!scroll_view) return 0;

    iupAttribSet(ih, "SB_RESIZE", NULL);

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
        if ([scroll_view hasVerticalScroller])
        {
          [scroll_view setHasVerticalScroller:NO];
          iupAttribSet(ih, "SB_RESIZE", "YES");
        }
      }
      else
      {
        if(![scroll_view hasVerticalScroller])
        {
          [scroll_view setHasVerticalScroller:YES];
          iupAttribSet(ih, "SB_RESIZE", "YES");
        }
        [[scroll_view verticalScroller] setEnabled:NO];
      }
      iupAttribSet(ih, "YHIDDEN", "YES");
      if (ih->data->posy != ymin) IupSetDouble(ih, "POSY", ymin);
    }
    else
    {
      if (![scroll_view hasVerticalScroller])
      {
        [scroll_view setHasVerticalScroller:YES];
        iupAttribSet(ih, "SB_RESIZE", "YES");
      }
      [[scroll_view verticalScroller] setEnabled:YES];
      iupAttribSet(ih, "YHIDDEN", "NO");
    }

    if (iupAttribGet(ih, "SB_RESIZE"))
    {
      [scroll_view tile];
      [scroll_view setNeedsDisplay:YES];
    }
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
    // Ensure max_posx is valid (>= xmin) before clamping.
    if (max_posx < xmin) max_posx = xmin;
    posx = max_posx;
  }

  ih->data->posx = posx;

  // Calculate Cocoa origin.x. Since the view is flipped (Top-Down), Offset = POSX - XMIN.
  double cocoa_origin_x = posx - xmin;
  NSClipView* clip_view = [scroll_view contentView];
  NSPoint current_origin = [clip_view bounds].origin;

  if (fabs(current_origin.x - cocoa_origin_x) > 1e-9)
  {
    iupAttribSet(ih, "_IUPCOCOA_UPDATING_SCROLL_POS", "1");

    current_origin.x = cocoa_origin_x;
    [clip_view setBoundsOrigin:current_origin];
    [scroll_view reflectScrolledClipView:clip_view];

    iupAttribSet(ih, "_IUPCOCOA_UPDATING_SCROLL_POS", NULL);
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
    // Ensure max_posy is valid (>= ymin) before clamping.
    if (max_posy < ymin) max_posy = ymin;
    posy = max_posy;
  }

  ih->data->posy = posy;

  // Convert IUP's top-down posy into Cocoa's origin.y.
  // Since the view is flipped (isFlipped=YES), the coordinate system is Top-Down.
  // Offset = POSY - YMIN.
  double cocoa_origin_y = posy - ymin;

  NSClipView* clip_view = [scroll_view contentView];
  NSPoint current_origin = [clip_view bounds].origin;

  if (fabs(current_origin.y - cocoa_origin_y) > 1e-9)
  {
    iupAttribSet(ih, "_IUPCOCOA_UPDATING_SCROLL_POS", "1");

    current_origin.y = cocoa_origin_y;
    [clip_view setBoundsOrigin:current_origin];
    [scroll_view reflectScrolledClipView:clip_view];

    iupAttribSet(ih, "_IUPCOCOA_UPDATING_SCROLL_POS", NULL);
  }

  return 1;
}

static int cocoaCanvasSetBeginDragAttrib(Ihandle* ih, const char* value)
{
  IupSourceDragAssociatedData* drag_source_data = cocoaSourceDragGetAssociatedData(ih);
  if(![drag_source_data isDragSourceEnabled]) return 1;

  NSDraggingItem* dragging_item = [drag_source_data defaultDraggingItem];
  NSView* main_view = [drag_source_data mainView];

  // For file promises, ensure the user info (image representation) is set.
  if([drag_source_data usesFilePromise] && ![drag_source_data hasFilePromiseCallback])
  {
    NSFilePromiseProvider* file_promise = (NSFilePromiseProvider*)[dragging_item item];
    NSDraggingImageComponent* image_component = [[dragging_item imageComponents] firstObject];
    if(image_component)
    {
      [file_promise setUserInfo:[image_component contents]];
    }
    else // Fallback to a PDF representation of the view
    {
      NSData* pdf_data = [main_view dataWithPDFInsideRect:[main_view bounds]];
      NSImage* image_data = [[NSImage alloc] initWithData:pdf_data];
      [file_promise setUserInfo:image_data];
      [image_data release];
    }
  }

  NSEvent* the_event = [[NSApplication sharedApplication] currentEvent];
  [main_view beginDraggingSessionWithItems:@[dragging_item] event:the_event source:drag_source_data];

  (void)value;
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

static int cocoaCanvasSetNativeFocusRingAttrib(Ihandle* ih, const char* value)
{
  IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
  [canvas_view setUseNativeFocusRing:(bool)iupStrBoolean(value)];
  return 1;
}

static int cocoaCanvasSetContextMenuAttrib(Ihandle* ih, const char* value)
{
  Ihandle* menu_ih = (Ihandle*)value;
  IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
  iupCocoaCommonBaseSetContextMenuForWidget(ih, canvas_view, menu_ih);
  return 1;
}

static int cocoaCanvasMapMethod(Ihandle* ih)
{
  NSView* root_view = nil;
  IupCocoaCanvasView* canvas_view = [[IupCocoaCanvasView alloc] initWithFrame:NSZeroRect ih:ih];
  iupAttribSet(ih, "_IUPCOCOA_CANVAS_VIEW", (char*)canvas_view);

  ih->data->sb = iupBaseGetScrollbar(ih);

  NSNotificationCenter* notification_center = [NSNotificationCenter defaultCenter];

  if (ih->data->sb)
  {
    NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSZeroRect];
    NSClipView* clip_view = [scroll_view contentView];

    /* Explicitly prevent the document view from resizing automatically with the scroll view.
       Its size is managed manually in cocoaCanvasLayoutUpdateMethod based on XMAX/YMAX.
       If the document view resizes to fit the content area, scrolling will not occur. */
    [canvas_view setAutoresizingMask:NSViewNotSizable];

    [scroll_view setDocumentView:canvas_view];
    [canvas_view release]; // scroll_view now owns canvas_view

    // For a canvas with custom drawing via SCROLL_CB,
    // we must disable bit-blit scrolling (copiesOnScroll=NO) to force a full redraw on each scroll step.
    if (IupGetCallback(ih, "SCROLL_CB"))
    {
      [clip_view setCopiesOnScroll:NO];
    }

    // Disable automatic scrollbar hiding and let IUP manage it explicitly
    // based on DX/DY and AUTOHIDE attributes.
    [scroll_view setAutohidesScrollers:NO];

    [scroll_view setHasHorizontalScroller:(ih->data->sb & IUP_SB_HORIZ)];
    [scroll_view setHorizontalScrollElasticity:NSScrollElasticityNone];
    [scroll_view setHasVerticalScroller:(ih->data->sb & IUP_SB_VERT)];
    [scroll_view setVerticalScrollElasticity:NSScrollElasticityNone];
    [scroll_view setBorderType:iupAttribGetBoolean(ih, "BORDER") ? NSBezelBorder : NSNoBorder];
    [scroll_view setDrawsBackground:NO];

    // Register for resize notifications (RESIZE_CB).
    [clip_view setPostsFrameChangedNotifications:YES];
    [notification_center addObserver:canvas_view
                            selector:@selector(frameDidChangeNotification:)
                                name:NSViewFrameDidChangeNotification
                              object:clip_view];

    // Register for scroll notifications to trigger SCROLL_CB.
    [clip_view setPostsBoundsChangedNotifications:YES];
    [notification_center addObserver:canvas_view
                            selector:@selector(boundsDidChangeNotification:)
                                name:NSViewBoundsDidChangeNotification
                              object:clip_view];

    root_view = scroll_view;
    iupAttribSet(ih, "_IUPCOCOA_CANVAS_HAS_SCROLLBAR", "1");

    // The actual sizing of the document view is deferred to cocoaCanvasLayoutUpdateMethod,
    // which runs after the parent scroll view has its final size from the layout engine.
  }
  else
  {
    // Standalone canvas setup.
    // Observe frame changes on the canvas_view itself (RESIZE_CB).
    [canvas_view setPostsFrameChangedNotifications:YES];
    [notification_center addObserver:canvas_view
                            selector:@selector(frameDidChangeNotification:)
                                name:NSViewFrameDidChangeNotification
                              object:canvas_view];

    [canvas_view setPostsBoundsChangedNotifications:YES];
    root_view = canvas_view;
  }

  ih->handle = root_view;
  iupCocoaSetAssociatedViews(ih, canvas_view, root_view);
  iupCocoaAddToParent(ih);

  // Setup Drag and Drop
  IupSourceDragAssociatedData* source_drag = cocoaSourceDragCreateAssociatedData(ih, canvas_view, root_view);
  cocoaTargetDropCreateAssociatedData(ih, canvas_view, root_view);
  [source_drag setDefaultFilePromiseName:@"IupCanvas.png"];

  // Set initial BGCOLOR
  cocoaCanvasSetBgColorAttrib(ih, iupAttribGet(ih, "BGCOLOR"));

  // Set initial Scrollbar state
  cocoaCanvasSetDXAttrib(ih, NULL);
  cocoaCanvasSetDYAttrib(ih, NULL);

  return IUP_NOERROR;
}

static void cocoaCanvasUnMapMethod(Ihandle* ih)
{
  id root_view = ih->handle;
  IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);

  if (canvas_view)
  {
    [[NSNotificationCenter defaultCenter] removeObserver:canvas_view];
  }

  cocoaTargetDropDestroyAssociatedData(ih);
  cocoaSourceDragDestroyAssociatedData(ih);

  Ihandle* context_menu_ih = (Ihandle*)iupCocoaCommonBaseGetContextMenuAttrib(ih);
  if(context_menu_ih)
  {
    IupDestroy(context_menu_ih);
  }
  iupCocoaCommonBaseSetContextMenuAttrib(ih, NULL);

  iupCocoaRemoveFromParent(ih);
  iupCocoaSetAssociatedViews(ih, nil, nil);
  [root_view release];
  ih->handle = NULL;
  iupAttribSet(ih, "_IUPCOCOA_CANVAS_VIEW", NULL);
}

static void cocoaCanvasLayoutUpdateMethod(Ihandle *ih)
{
  // This triggers synchronous layout and frame change notifications,
  // ensuring RESIZE_CB is called and DX/DY are updated before we proceed.
  iupdrvBaseLayoutUpdateMethod(ih);

  if (ih->data->sb)
  {
    IupCocoaCanvasView* canvas_view = cocoaCanvasGetCanvasView(ih);
    NSScrollView* scroll_view = cocoaCanvasGetScrollView(ih);

    double xmin = iupAttribGetDouble(ih, "XMIN");
    double xmax = iupAttribGetDouble(ih, "XMAX");
    double ymin = iupAttribGetDouble(ih, "YMIN");
    double ymax = iupAttribGetDouble(ih, "YMAX");

    double virtual_width = xmax - xmin;
    double virtual_height = ymax - ymin;

    // Set document size to virtual size.
    // The visible area (DX/DY) determines how much of the document is shown at once.
    double content_width = virtual_width;
    double content_height = virtual_height;

    // Handle default virtual space
    if (virtual_width <= 1.0)
    {
      NSRect contentViewBounds = [[scroll_view contentView] bounds];
      content_width = contentViewBounds.size.width;
    }
    if (virtual_height <= 1.0)
    {
      NSRect contentViewBounds = [[scroll_view contentView] bounds];
      content_height = contentViewBounds.size.height;
    }

    // Ensure minimum size
    if (content_width < 1) content_width = 1;
    if (content_height < 1) content_height = 1;

    NSSize new_size = NSMakeSize(content_width, content_height);
    NSSize currentCanvasSize = [canvas_view frame].size;

    if (!NSEqualSizes([canvas_view frame].size, new_size))
    {
      [canvas_view setFrameSize:new_size];

      // After changing the document size, update scroll position to ensure it stays within valid range
      cocoaCanvasSetPosXAttrib(ih, iupAttribGet(ih, "POSX"));
      cocoaCanvasSetPosYAttrib(ih, iupAttribGet(ih, "POSY"));
    }

    // Update scrollbar visibility/enabled state based on new layout.
    cocoaCanvasSetDXAttrib(ih, NULL);
    cocoaCanvasSetDYAttrib(ih, NULL);
  }
}

static void cocoaCanvasComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *expand)
{
  /*
     IUP core layout computes natural size from SIZE and RASTERSIZE
     and stores it in ih->naturalwidth, ih->naturalheight.
     */
  int natural_w = ih->naturalwidth;
  int natural_h = ih->naturalheight;

  /* If no size is specified, use a default minimum size */
  if (natural_w == 0) natural_w = 1;
  if (natural_h == 0) natural_h = 1;

  *w = natural_w;
  *h = natural_h;
  *expand = iupAttribGetBoolean(ih, "EXPAND");

  /* Add space for scrollbars if they are visible */
  if (ih->data->sb)
  {
    int sb_size = iupdrvGetScrollbarSize();
    if ((ih->data->sb & IUP_SB_VERT) && !iupAttribGetBoolean(ih, "YHIDDEN"))
      *w += sb_size;
    if ((ih->data->sb & IUP_SB_HORIZ) && !iupAttribGetBoolean(ih, "XHIDDEN"))
      *h += sb_size;
  }
}

void iupdrvCanvasInitClass(Iclass* ic)
{
  // Driver Dependent Class functions
  ic->Map = cocoaCanvasMapMethod;
  ic->UnMap = cocoaCanvasUnMapMethod;
  ic->LayoutUpdate = cocoaCanvasLayoutUpdateMethod;
  ic->ComputeNaturalSize = cocoaCanvasComputeNaturalSizeMethod;

  // Visual
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaCanvasSetBgColorAttrib, "232 232 232", NULL, IUPAF_DEFAULT);

  // IupCanvas only
  iupClassRegisterAttribute(ic, "DRAWSIZE", cocoaCanvasGetDrawSizeAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  // Scrollbar attributes
  iupClassRegisterAttribute(ic, "DX", NULL, cocoaCanvasSetDXAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DY", NULL, cocoaCanvasSetDYAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSX", iupCanvasGetPosXAttrib, cocoaCanvasSetPosXAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "POSY", iupCanvasGetPosYAttrib, cocoaCanvasSetPosYAttrib, "0", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "XAUTOHIDE", NULL, NULL, "YES", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "YAUTOHIDE", NULL, NULL, "YES", NULL, IUPAF_NO_INHERIT);

  // Platform specific
  iupClassRegisterAttribute(ic, "DRAWABLE", cocoaCanvasGetDrawableAttrib, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CGCONTEXT", cocoaCanvasGetCGContextAttrib, NULL, NULL, NULL, IUPAF_NO_STRING|IUPAF_NO_INHERIT);

  // Focus ring support
  iupClassRegisterAttribute(ic, "NATIVEFOCUSRING", cocoaCanvasGetNativeFocusRingAttrib, cocoaCanvasSetNativeFocusRingAttrib, "NO", NULL, IUPAF_NO_INHERIT);

  // Drag and drop
  iupClassRegisterAttribute(ic, "DRAGINITIATE", NULL, cocoaCanvasSetBeginDragAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AUTOBEGINDRAG", NULL, NULL, "NO", NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SENDACTION", NULL, iupCocoaCommonBaseSetSendActionAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  // Not Supported
  iupClassRegisterAttribute(ic, "BACKINGSTORE", NULL, NULL, "YES", NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

  // Layer backing
  iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupCocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}
