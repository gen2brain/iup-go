/** \file
 * \brief macOS Driver TIPS management
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_str.h"
#include "iup_attrib.h"
#include "iupcocoa_drv.h"


static const void* IUP_COCOA_TOOLTIP_OWNER_KEY = @"IUP_COCOA_TOOLTIP_OWNER_KEY";

@interface IupCocoaToolTipOwner : NSObject
@property (nonatomic, assign) Ihandle* ihandle;
@property (nonatomic, assign) NSToolTipTag toolTipTag;
@end

@implementation IupCocoaToolTipOwner

- (NSString *)view:(NSView *)view stringForToolTip:(NSToolTipTag)tag point:(NSPoint)point userData:(void *)data
{
  if (!self.ihandle || !iupObjectCheck(self.ihandle))
  {
    return nil;
  }

  IFnii cb = (IFnii)IupGetCallback(self.ihandle, "TIPS_CB");
  if (cb)
  {
    int x = (int)point.x;
    int y = (int)point.y;

    if (![view isFlipped])
    {
      NSRect view_bounds = [view bounds];
      y = (int)(view_bounds.size.height - point.y);
    }

    cb(self.ihandle, x, y);
  }

  const char* tip_cstr = iupAttribGet(self.ihandle, "TIP");
  if (!tip_cstr)
  {
    return nil;
  }

  return [NSString stringWithUTF8String:tip_cstr];
}

@end

void iupcocoaTipsDestroy(Ihandle* ih)
{
  NSView* the_view = iupcocoaGetRootView(ih);
  if (!the_view) return;

  IupCocoaToolTipOwner* owner = objc_getAssociatedObject(the_view, IUP_COCOA_TOOLTIP_OWNER_KEY);
  if (owner)
  {
    if (owner.toolTipTag != 0)
    {
      [the_view removeToolTip:owner.toolTipTag];
    }
    objc_setAssociatedObject(the_view, IUP_COCOA_TOOLTIP_OWNER_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
}

static void iupCocoaTipsUpdateForView(NSView* the_view, IupCocoaToolTipOwner* owner)
{
  if (owner.toolTipTag != 0)
  {
    [the_view removeToolTip:owner.toolTipTag];
    owner.toolTipTag = 0;
  }

  NSRect tip_rect;
  const char* tiprect_value = iupAttribGet(owner.ihandle, "TIPRECT");

  if (tiprect_value)
  {
    int x1, y1, x2, y2;
    if (sscanf(tiprect_value, "%d %d %d %d", &x1, &y1, &x2, &y2) == 4)
    {
      tip_rect = NSMakeRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);

      if (![the_view isFlipped])
      {
        NSRect view_bounds = [the_view bounds];
        tip_rect.origin.y = view_bounds.size.height - tip_rect.origin.y - tip_rect.size.height;
      }
    }
    else
    {
      tip_rect = [the_view bounds];
    }
  }
  else
  {
    tip_rect = [the_view bounds];
  }

  owner.toolTipTag = [the_view addToolTipRect:tip_rect
                                        owner:owner
                                     userData:NULL];
}

void cocoaUpdateTip(Ihandle* ih)
{
  if (!ih) return;

  NSView* the_view = iupcocoaGetRootView(ih);
  if (!the_view) return;

  IupCocoaToolTipOwner* owner = objc_getAssociatedObject(the_view, IUP_COCOA_TOOLTIP_OWNER_KEY);
  if (owner)
  {
    iupCocoaTipsUpdateForView(the_view, owner);
  }
}

int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  NSView* the_view = iupcocoaGetRootView(ih);
  if (!the_view)
  {
    return 1;
  }

  if (value && *value)
  {
    IupCocoaToolTipOwner* owner = objc_getAssociatedObject(the_view, IUP_COCOA_TOOLTIP_OWNER_KEY);
    if (!owner)
    {
      owner = [[IupCocoaToolTipOwner alloc] init];
      owner.ihandle = ih;

      objc_setAssociatedObject(the_view, IUP_COCOA_TOOLTIP_OWNER_KEY, owner, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
      [owner release];
    }

    iupCocoaTipsUpdateForView(the_view, owner);
  }
  else
  {
    iupcocoaTipsDestroy(ih);
  }

  return 1;
}

int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  (void)ih;
  return NULL;
}

int cocoaBaseSetTipRectAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  cocoaUpdateTip(ih);
  return 0;
}

int cocoaBaseSetTipBgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

int cocoaBaseSetTipFgColorAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}

int cocoaBaseSetTipFontAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  (void)value;
  return 0;
}
