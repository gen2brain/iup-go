/** \file
 * \brief IupPopover control for Cocoa
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

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_childtree.h"
#include "iup_class.h"
#include "iup_register.h"

#include "iupcocoa_drv.h"

#include "iup_popover.h"


static const void* POPOVER_DELEGATE_KEY = @"POPOVER_DELEGATE_KEY";

/* Margin to prevent content clipping at NSPopover's rounded corners */
#define COCOA_POPOVER_CORNER_MARGIN 8


@interface IupCocoaPopoverDelegate : NSObject <NSPopoverDelegate>
{
  Ihandle* _ih;
}
- (instancetype)initWithIhandle:(Ihandle*)ih;
@end

@implementation IupCocoaPopoverDelegate

- (instancetype)initWithIhandle:(Ihandle*)ih
{
  self = [super init];
  if (self)
  {
    _ih = ih;
  }
  return self;
}

- (void)popoverDidClose:(NSNotification *)notification
{
  IFni show_cb = (IFni)IupGetCallback(_ih, "SHOW_CB");
  if (show_cb)
    show_cb(_ih, IUP_HIDE);
}

- (void)popoverWillShow:(NSNotification *)notification
{
  IFni show_cb = (IFni)IupGetCallback(_ih, "SHOW_CB");
  if (show_cb)
    show_cb(_ih, IUP_SHOW);
}

- (BOOL)popoverShouldDetach:(NSPopover *)popover
{
  return NO;
}

@end


@interface IupCocoaPopoverContentView : NSView
{
  Ihandle* _ih;
}
- (instancetype)initWithIhandle:(Ihandle*)ih;
@end

@implementation IupCocoaPopoverContentView

- (instancetype)initWithIhandle:(Ihandle*)ih
{
  self = [super initWithFrame:NSZeroRect];
  if (self)
  {
    _ih = ih;
  }
  return self;
}

- (BOOL)isFlipped
{
  return YES;
}

@end


@interface IupCocoaPopoverViewController : NSViewController
{
  Ihandle* _ih;
}
- (instancetype)initWithIhandle:(Ihandle*)ih;
@end

@implementation IupCocoaPopoverViewController

- (instancetype)initWithIhandle:(Ihandle*)ih
{
  self = [super initWithNibName:nil bundle:nil];
  if (self)
  {
    _ih = ih;
  }
  return self;
}

- (void)loadView
{
  IupCocoaPopoverContentView* content_view = [[IupCocoaPopoverContentView alloc] initWithIhandle:_ih];
  self.view = content_view;
  [content_view release];
}

@end


static int cocoaPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
  NSPopover* popover;

  if (iupStrBoolean(value))
  {
    Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
    NSView* anchor_view;

    if (!anchor || !anchor->handle)
      return 0;

    /* Map if not yet mapped */
    if (!ih->handle)
    {
      if (IupMap(ih) == IUP_ERROR)
        return 0;
    }

    popover = (NSPopover*)ih->handle;

    anchor_view = iupcocoaGetMainView(anchor);
    if (!anchor_view)
      anchor_view = iupcocoaGetRootView(anchor);

    if (anchor_view)
    {
      NSRectEdge edge = NSRectEdgeMaxY;

      const char* pos = iupAttribGetStr(ih, "POSITION");
      if (iupStrEqualNoCase(pos, "TOP"))
        edge = NSRectEdgeMinY;
      else if (iupStrEqualNoCase(pos, "LEFT"))
        edge = NSRectEdgeMinX;
      else if (iupStrEqualNoCase(pos, "RIGHT"))
        edge = NSRectEdgeMaxX;
      else
        edge = NSRectEdgeMaxY;

      int autohide = iupAttribGetBoolean(ih, "AUTOHIDE");
      [popover setBehavior:autohide ? NSPopoverBehaviorTransient : NSPopoverBehaviorApplicationDefined];

      if (ih->firstchild)
      {
        int margin = COCOA_POPOVER_CORNER_MARGIN;

        iupLayoutCompute(ih);

        NSSize content_size = NSMakeSize(ih->currentwidth + 2 * margin, ih->currentheight + 2 * margin);
        [popover setContentSize:content_size];

        NSView* content_view = (NSView*)iupAttribGet(ih, "_IUP_COCOA_CONTENT_VIEW");
        if (content_view)
          [content_view setFrameSize:content_size];

        iupLayoutUpdate(ih);
      }

      [popover showRelativeToRect:[anchor_view bounds] ofView:anchor_view preferredEdge:edge];
    }
  }
  else
  {
    if (ih->handle)
    {
      popover = (NSPopover*)ih->handle;
      [popover performClose:nil];
    }
  }

  return 0;
}

static char* cocoaPopoverGetVisibleAttrib(Ihandle* ih)
{
  NSPopover* popover;

  if (!ih->handle)
    return "NO";

  popover = (NSPopover*)ih->handle;
  return iupStrReturnBoolean([popover isShown]);
}

static void cocoaPopoverLayoutUpdateMethod(Ihandle* ih)
{
  if (ih->firstchild)
  {
    int margin = COCOA_POPOVER_CORNER_MARGIN;
    ih->iclass->SetChildrenPosition(ih, margin, margin);
    iupLayoutUpdate(ih->firstchild);
  }
}

static int cocoaPopoverMapMethod(Ihandle* ih)
{
  NSPopover* popover;
  IupCocoaPopoverViewController* view_controller;
  IupCocoaPopoverDelegate* delegate;

  popover = [[NSPopover alloc] init];
  if (!popover)
    return IUP_ERROR;

  view_controller = [[IupCocoaPopoverViewController alloc] initWithIhandle:ih];
  [popover setContentViewController:view_controller];
  [view_controller release];

  delegate = [[IupCocoaPopoverDelegate alloc] initWithIhandle:ih];
  [popover setDelegate:delegate];
  objc_setAssociatedObject(popover, POPOVER_DELEGATE_KEY, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

  int show_arrow = iupAttribGetBoolean(ih, "ARROW");
  if (!show_arrow)
  {
    [popover setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameAqua]];
  }

  ih->handle = popover;

  iupAttribSet(ih, "_IUP_COCOA_CONTENT_VIEW", (char*)[view_controller view]);

  return IUP_NOERROR;
}

static void cocoaPopoverUnMapMethod(Ihandle* ih)
{
  NSPopover* popover = (NSPopover*)ih->handle;

  if (popover)
  {
    if ([popover isShown])
      [popover performClose:nil];

    IupCocoaPopoverDelegate* delegate = objc_getAssociatedObject(popover, POPOVER_DELEGATE_KEY);
    if (delegate)
    {
      [popover setDelegate:nil];
      objc_setAssociatedObject(popover, POPOVER_DELEGATE_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
      [delegate release];
    }

    [popover release];
  }

  ih->handle = NULL;
}

static void* cocoaPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  return iupAttribGet(ih, "_IUP_COCOA_CONTENT_VIEW");
}

void iupdrvPopoverInitClass(Iclass* ic)
{
  ic->Map = cocoaPopoverMapMethod;
  ic->UnMap = cocoaPopoverUnMapMethod;
  ic->LayoutUpdate = cocoaPopoverLayoutUpdateMethod;
  ic->GetInnerNativeContainerHandle = cocoaPopoverGetInnerNativeContainerHandleMethod;

  /* Override VISIBLE attribute, NOT_MAPPED because setter handles mapping */
  iupClassRegisterAttribute(ic, "VISIBLE", cocoaPopoverGetVisibleAttrib, cocoaPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
