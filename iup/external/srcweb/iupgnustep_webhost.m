/** \file
 * \brief Web Browser Control - GNUstep host of the native web engine
 *
 * See Copyright Notice in "iup.h"
 */

#import <AppKit/AppKit.h>
#import <GNUstepGUI/GSDisplayServer.h>

#include <stdint.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_drv.h"

#include "iupcocoa_drv.h"
#include "iupunix_x11.h"
#include "iupweb_host.h"


static Display* gnustepWebHostDisplay(void)
{
  return (Display*)[GSCurrentServer() serverDevice];
}

@interface IupGnustepWebHostView : NSView
{
  Ihandle* _ih;
  void* _parent;
  int _rect[4];
  int _clip[4];
}
- (id) initWithIhandle:(Ihandle*)ih;
- (void) detach;
- (void) place;
@end

@implementation IupGnustepWebHostView

- (id) initWithIhandle:(Ihandle*)ih
{
  self = [super initWithFrame:NSMakeRect(0, 0, 10, 10)];
  if (self)
  {
    _ih = ih;
    _clip[2] = -1;
  }
  return self;
}

- (void) detach
{
  _ih = NULL;
}

- (BOOL) isFlipped
{
  return YES;
}

- (BOOL) acceptsFirstResponder
{
  return _ih && iupdrvIsActive(_ih) && iupAttribGetBoolean(_ih, "CANFOCUS");
}

- (BOOL) canBecomeKeyView
{
  return [self acceptsFirstResponder];
}

- (BOOL) becomeFirstResponder
{
  BOOL accepted = [super becomeFirstResponder];
  if (accepted && _ih)
  {
    iupwebHostSetFocus(_ih, 1);
    iupcocoaFocusIn(_ih);
  }
  return accepted;
}

- (BOOL) resignFirstResponder
{
  BOOL accepted = [super resignFirstResponder];
  if (accepted && _ih)
  {
    iupwebHostSetFocus(_ih, 0);
    iupcocoaFocusOut(_ih);
  }
  return accepted;
}

- (void) sendKey:(NSEvent*)event press:(int)press
{
  Display* display = gnustepWebHostDisplay();
  Window root, child;
  int root_x, root_y, x, y;
  unsigned int mask = 0;

  if (!_ih)
    return;

  XQueryPointer(display, XRootWindow(display, XDefaultScreen(display)), &root, &child, &root_x, &root_y, &x, &y, &mask);
  iupwebHostKeyEvent(_ih, press, [event keyCode], mask & ~0x1F00u, (unsigned long)([event timestamp] * 1000));
}

- (void) keyDown:(NSEvent*)event
{
  [self sendKey:event press:1];
}

- (void) keyUp:(NSEvent*)event
{
  [self sendKey:event press:0];
}

- (void) setFrame:(NSRect)frame
{
  [super setFrame:frame];
  [self place];
}

- (void) setFrameOrigin:(NSPoint)origin
{
  [super setFrameOrigin:origin];
  [self place];
}

- (void) setFrameSize:(NSSize)size
{
  [super setFrameSize:size];
  [self place];
}

- (void) viewDidMoveToWindow
{
  [super viewDidMoveToWindow];
  [self place];
}

- (void) setHidden:(BOOL)hidden
{
  [super setHidden:hidden];
  [self place];
}

- (void) place
{
  NSWindow* window = [self window];
  int rect[4] = {0, 0, 0, 0};
  int clip[4] = {0, 0, 0, 0};

  if (!_ih)
    return;

  if (!window || [window windowNumber] == 0 || ![window windowRef])
  {
    _parent = NULL;
    iupwebHostSetParent(_ih, NULL);
    return;
  }

  if ([window windowRef] != _parent)
  {
    _parent = [window windowRef];
    XSync(gnustepWebHostDisplay(), 0);
  }
  iupwebHostSetParent(_ih, _parent);

  if (![self isHiddenOrHasHiddenAncestor])
  {
    float left = 0, right = 0, top = 0, bottom = 0;
    NSRect frame = [window frame];
    NSRect view = [self convertRect:[self bounds] toView:nil];
    NSRect visible = [self convertRect:[self visibleRect] toView:nil];
    Ihandle* parent;
    int height;

    for (parent = _ih->parent; parent && parent->iclass->nativetype != IUP_TYPEDIALOG; parent = parent->parent)
    {
      NSView* parent_view;

      if (parent->iclass->nativetype == IUP_TYPEVOID || !parent->handle)
        continue;

      parent_view = iupcocoaGetMainView(parent);
      if (parent_view && [parent_view window] == window)
        visible = NSIntersectionRect(visible, [parent_view convertRect:[parent_view visibleRect] toView:nil]);
    }

    [GSServerForWindow(window) styleoffsets:&left :&right :&top :&bottom :[window styleMask]];
    height = (int)(frame.size.height - top);

    rect[0] = (int)(view.origin.x - left);
    rect[1] = height - (int)(view.origin.y + view.size.height);
    rect[2] = (int)view.size.width;
    rect[3] = (int)view.size.height;

    if (visible.size.width > 0 && visible.size.height > 0)
    {
      clip[0] = (int)(visible.origin.x - left);
      clip[1] = height - (int)(visible.origin.y + visible.size.height);
      clip[2] = (int)visible.size.width;
      clip[3] = (int)visible.size.height;
    }
  }

  if (memcmp(rect, _rect, sizeof(rect)) == 0 && memcmp(clip, _clip, sizeof(clip)) == 0)
    return;

  memcpy(_rect, rect, sizeof(rect));
  memcpy(_clip, clip, sizeof(clip));
  iupwebHostSetBounds(_ih, rect[0], rect[1], rect[2], rect[3], clip[0], clip[1], clip[2], clip[3]);
}

@end

void* iupwebHostMap(Ihandle* ih)
{
  IupGnustepWebHostView* view;
  Class xserver = NSClassFromString(@"XGServer");

  if (!xserver || ![GSCurrentServer() isKindOfClass:xserver] || !iupX11Open() || !XSync || !XQueryPointer)
    return NULL;

  view = [[IupGnustepWebHostView alloc] initWithIhandle:ih];
  ih->handle = view;
  iupAttribSet(ih, "_IUPCOCOA_RAWKEYS", "1");
  objc_setAssociatedObject(view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
  iupcocoaSetAssociatedViews(ih, view, view);
  iupcocoaAddToParent(ih);

  if (![view window] || [[view window] windowNumber] == 0 || ![[view window] windowRef])
    return NULL;

  XSync(gnustepWebHostDisplay(), 0);
  return [[view window] windowRef];
}

void iupwebHostLayoutUpdate(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  [(IupGnustepWebHostView*)ih->handle place];
}

void iupwebHostUnMap(Ihandle* ih)
{
  if (!ih->handle)
    return;

  [(IupGnustepWebHostView*)ih->handle detach];
  iupdrvBaseUnMapMethod(ih);
}
