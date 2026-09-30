/** \file
 * \brief macOS Driver TIPS management
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_drv.h"
#include "iup_str.h"

#include "iupcocoa_drv.h"


static const void* IUP_COCOA_TOOLTIP_OWNER_KEY = @"IUP_COCOA_TOOLTIP_OWNER_KEY";

static NSPanel* cocoa_tip_window = nil;
static NSTextField* cocoa_tip_label = nil;
static Ihandle* cocoa_tip_ih = NULL;

static int cocoaTipsIsStyled(Ihandle* ih)
{
  return iupAttribGetInherit(ih, "TIPBGCOLOR") || iupAttribGetInherit(ih, "TIPFGCOLOR") || iupAttribGetInherit(ih, "TIPFONT");
}

static int cocoaTipsIsCustom(Ihandle* ih)
{
  return cocoaTipsIsStyled(ih) || iupAttribGetBoolean(ih, "TIPMARKUP");
}

static NSColor* cocoaTipsColor(const char* value, NSColor* default_color)
{
  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
    return [NSColor colorWithSRGBRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
  return default_color;
}

static NSFont* cocoaTipsFont(Ihandle* ih, int styled)
{
  const char* value = iupAttribGetInherit(ih, "TIPFONT");
  IupCocoaFont* iup_font;

  if (!styled || (value && iupStrEqualNoCase(value, "SYSTEM")))
    return [NSFont toolTipsFontOfSize:0];

  if (!value)
    value = iupAttribGetStr(ih, "FONT");

  iup_font = iupcocoaFindFont(value);
  return iup_font ? [iup_font nativeFont] : [NSFont toolTipsFontOfSize:0];
}

static NSAttributedString* cocoaTipsMarkupString(Ihandle* ih, const char* tip, NSFont* font, NSColor* fg)
{
  NSMutableAttributedString* str = iupcocoaBuildMarkupAttributedString(ih, tip);
  NSFont* base_font = [[iupcocoaGetFont(ih) nativeFont] retain];
  NSUInteger index, length;
  NSRange range;

  if (!str)
  {
    [base_font release];
    return nil;
  }

  length = [str length];
  for (index = 0; index < length; index = NSMaxRange(range))
  {
    if (![str attribute:NSForegroundColorAttributeName atIndex:index effectiveRange:&range])
      [str addAttribute:NSForegroundColorAttributeName value:fg range:range];
  }

  if (font && base_font && ![[font fontName] isEqualToString:[base_font fontName]])
  {
    NSFontManager* font_manager = [NSFontManager sharedFontManager];
    CGFloat scale = [font pointSize] / [base_font pointSize];

    for (index = 0; index < length; index = NSMaxRange(range))
    {
      NSFont* run_font = [str attribute:NSFontAttributeName atIndex:index effectiveRange:&range];
      NSFont* new_font;
      if (!run_font)
        run_font = base_font;
      new_font = [font_manager fontWithFamily:[font familyName] traits:[font_manager traitsOfFont:run_font] weight:5 size:[run_font pointSize] * scale];
      if (new_font)
        [str addAttribute:NSFontAttributeName value:new_font range:range];
    }
  }

  [base_font release];
  return [str autorelease];
}

static void cocoaTipsHide(Ihandle* ih)
{
  if (ih && ih != cocoa_tip_ih)
    return;

  if (cocoa_tip_window)
    [cocoa_tip_window orderOut:nil];
  cocoa_tip_ih = NULL;
}

static void cocoaTipsShow(Ihandle* ih, NSPoint screen_point)
{
  const char* tip = iupAttribGet(ih, "TIP");
  CGFloat pad_x = 6, pad_y = 3;
  int styled;
  NSColor* fg;
  NSFont* font;
  NSAttributedString* markup;
  NSSize text_size;
  NSRect frame;
  NSScreen* screen = nil;

  if (!tip || !*tip)
    return;

  if (!cocoa_tip_window)
  {
    cocoa_tip_window = [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, 10, 10)
                                                   styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                                                     backing:NSBackingStoreBuffered
                                                       defer:YES];
    [cocoa_tip_window setLevel:NSPopUpMenuWindowLevel];
    [cocoa_tip_window setIgnoresMouseEvents:YES];
    [cocoa_tip_window setHasShadow:YES];
    [cocoa_tip_window setReleasedWhenClosed:NO];

    cocoa_tip_label = [[NSTextField alloc] initWithFrame:NSZeroRect];
    [cocoa_tip_label setBezeled:NO];
    [cocoa_tip_label setBordered:NO];
    [cocoa_tip_label setEditable:NO];
    [cocoa_tip_label setSelectable:NO];
    [cocoa_tip_label setDrawsBackground:NO];
    [[cocoa_tip_window contentView] addSubview:cocoa_tip_label];
  }

  styled = cocoaTipsIsStyled(ih);
  fg = styled ? cocoaTipsColor(iupAttribGetStr(ih, "TIPFGCOLOR"), [NSColor labelColor]) : [NSColor labelColor];
  font = cocoaTipsFont(ih, styled || iupAttribGetBoolean(ih, "TIPMARKUP"));
  [cocoa_tip_window setBackgroundColor:styled ? cocoaTipsColor(iupAttribGetStr(ih, "TIPBGCOLOR"), [NSColor windowBackgroundColor]) : [NSColor windowBackgroundColor]];
  [cocoa_tip_label setTextColor:fg];
  [cocoa_tip_label setFont:font];

  markup = iupAttribGetBoolean(ih, "TIPMARKUP") ? cocoaTipsMarkupString(ih, tip, iupAttribGetInherit(ih, "TIPFONT") ? font : nil, fg) : nil;
  if (markup)
    [cocoa_tip_label setAttributedStringValue:markup];
  else
    [cocoa_tip_label setStringValue:[NSString stringWithUTF8String:tip]];

  text_size = [[cocoa_tip_label cell] cellSize];
  frame = NSMakeRect(screen_point.x, screen_point.y - 20 - (text_size.height + 2 * pad_y),
                     ceil(text_size.width + 2 * pad_x), ceil(text_size.height + 2 * pad_y));

  for (NSScreen* s in [NSScreen screens])
  {
    if (NSPointInRect(screen_point, [s frame]))
    {
      screen = s;
      break;
    }
  }
  if (!screen)
    screen = [NSScreen mainScreen];
  if (screen)
  {
    NSRect visible = [screen visibleFrame];
    if (NSMaxX(frame) > NSMaxX(visible))
      frame.origin.x = NSMaxX(visible) - frame.size.width;
    if (frame.origin.x < NSMinX(visible))
      frame.origin.x = NSMinX(visible);
    if (frame.origin.y < NSMinY(visible))
      frame.origin.y = screen_point.y + 4;
  }

  [cocoa_tip_window setFrame:frame display:NO];
  [cocoa_tip_label setFrame:NSMakeRect(pad_x, pad_y, text_size.width, text_size.height)];
  [cocoa_tip_window orderFront:nil];
  cocoa_tip_ih = ih;
}

static void cocoaTipsCallTipsCb(Ihandle* ih, NSView* view, NSPoint point)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "TIPS_CB");
  if (cb)
  {
    int x = (int)point.x;
    int y = (int)point.y;

    if (![view isFlipped])
      y = (int)([view bounds].size.height - point.y);

    cb(ih, x, y);
  }
}

@interface IupCocoaToolTipOwner : NSObject
@property (nonatomic, assign) Ihandle* ihandle;
@property (nonatomic, assign) NSView* view;
@property (nonatomic, assign) NSToolTipTag toolTipTag;
@property (nonatomic, assign) NSTrackingRectTag trackingTag;
@property (nonatomic, retain) NSTimer* showTimer;
@end

@implementation IupCocoaToolTipOwner

- (void)dealloc
{
  [_showTimer invalidate];
  [_showTimer release];
  [super dealloc];
}

- (NSString*)view:(NSView*)view stringForToolTip:(NSToolTipTag)tag point:(NSPoint)point userData:(void*)data
{
  if (!self.ihandle || !iupObjectCheck(self.ihandle))
    return nil;

  if (cocoaTipsIsCustom(self.ihandle))
    return nil;

  cocoaTipsCallTipsCb(self.ihandle, view, point);

  const char* tip_cstr = iupAttribGet(self.ihandle, "TIP");
  if (!tip_cstr)
    return nil;

  return [NSString stringWithUTF8String:tip_cstr];
}

- (void)cancelTimer
{
  [self.showTimer invalidate];
  self.showTimer = nil;
}

- (void)showTimerFired:(NSTimer*)timer
{
  Ihandle* ih = self.ihandle;
  NSPoint screen_point = [NSEvent mouseLocation];
  NSRect window_rect;
  NSPoint point;

  self.showTimer = nil;

  if (!ih || !iupObjectCheck(ih) || ![self.view window])
    return;

  window_rect = [[self.view window] convertRectFromScreen:NSMakeRect(screen_point.x, screen_point.y, 0, 0)];
  point = [self.view convertPoint:window_rect.origin fromView:nil];
  cocoaTipsCallTipsCb(ih, self.view, point);

  cocoaTipsShow(ih, screen_point);
}

- (void)mouseEntered:(NSEvent*)event
{
  [self cancelTimer];

  if (!self.ihandle || !iupObjectCheck(self.ihandle) || !cocoaTipsIsCustom(self.ihandle))
    return;

  self.showTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(showTimerFired:) userInfo:nil repeats:NO];
}

- (void)mouseExited:(NSEvent*)event
{
  [self cancelTimer];
  cocoaTipsHide(self.ihandle);
}

@end

static void cocoaTipsRemoveTracking(NSView* the_view, IupCocoaToolTipOwner* owner)
{
  [owner cancelTimer];

  if (owner.trackingTag != 0)
  {
    [the_view removeTrackingRect:owner.trackingTag];
    owner.trackingTag = 0;
  }
}

IUP_DRV_API void iupcocoaTipsDestroy(Ihandle* ih)
{
  NSView* the_view = iupcocoaGetRootView(ih);

  cocoaTipsHide(ih);

  if (!the_view) return;

  IupCocoaToolTipOwner* owner = objc_getAssociatedObject(the_view, IUP_COCOA_TOOLTIP_OWNER_KEY);
  if (owner)
  {
    if (owner.toolTipTag != 0)
    {
      [the_view removeToolTip:owner.toolTipTag];
    }
    cocoaTipsRemoveTracking(the_view, owner);
    owner.ihandle = NULL;
    objc_setAssociatedObject(the_view, IUP_COCOA_TOOLTIP_OWNER_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
}

static void iupCocoaTipsUpdateForView(NSView* the_view, IupCocoaToolTipOwner* owner)
{
  NSRect tip_rect;
  const char* tiprect_value = iupAttribGet(owner.ihandle, "TIPRECT");

  if (owner.toolTipTag != 0)
  {
    [the_view removeToolTip:owner.toolTipTag];
    owner.toolTipTag = 0;
  }
  cocoaTipsRemoveTracking(the_view, owner);

  if (tiprect_value)
  {
    int x1, y1, x2, y2;
    if (iupStrToRect(tiprect_value, &x1, &y1, &x2, &y2))
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

  owner.view = the_view;
  owner.toolTipTag = [the_view addToolTipRect:tip_rect
                                        owner:owner
                                     userData:NULL];

  owner.trackingTag = [the_view addTrackingRect:tip_rect owner:owner userData:NULL assumeInside:NO];
}

IUP_DRV_API void iupcocoaUpdateTip(Ihandle* ih)
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

IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
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

  if (!iupAttribGet(ih, "ACCESSIBLEDESCRIPTION"))
    iupdrvSetAccessibleDescription(ih, value);

  return 1;
}

IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    cocoaTipsShow(ih, [NSEvent mouseLocation]);
  else
    cocoaTipsHide(ih);
  return 0;
}

IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(cocoa_tip_ih == ih && cocoa_tip_window && [cocoa_tip_window isVisible]);
}
