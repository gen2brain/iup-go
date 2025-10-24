/** \file
 * \brief TabBar
 *
 * Custom IupCocoaTabBarView implementation.
 *
 * Based on ENTabBarView created by Aaron Elkins.
 * ENTabBarView is released under MIT license.
 */

#include "IupCocoaTabBarView.h"

@interface IupCocoaTabBarView (Expose)
- (NSRect)tabRectFromIndex:(NSUInteger)index;
- (NSRect)rectForTabListControl;
- (BOOL)isBlankAreaOfTabBarViewInPoint:(NSPoint)p;
- (NSMenu *)tabsMenu;
- (void)popupMenuDidChoosed:(NSMenuItem*)item;
- (BOOL)validateMenuItem:(NSMenuItem*)menuItem;
- (IupCocoaTabCell*)tabCellInPoint:(NSPoint)p;
- (NSInteger)destinationCellIndexFromPoint:(NSPoint)p;
- (void)exchangeTabWithIndex:(NSUInteger)One withTabIndex:(NSUInteger)two;
@end


@implementation IupCocoaTabBarView (Expose)
  - (NSRect)tabRectFromIndex:(NSUInteger)index
{
  if (self.orientation == IupCocoaTabBarHorizontal)
  {
    NSUInteger tabListWidth = self.allowsTabListMenu ? kWidthOfTabList : 0;
    CGFloat leftPadding = 4; // Add a small padding to disconnect the first tab
    CGFloat rightPadding = 12; // This was the existing implicit padding
    NSUInteger totalWidthOfTabBarView = [self frame].size.width - tabListWidth - leftPadding - rightPadding;
    NSUInteger averageWidth = 0;

    if ([tabs count] > 0)
    {
      averageWidth = (NSUInteger)(totalWidthOfTabBarView / [tabs count]);
      averageWidth = averageWidth <= kMinTabCellWidth ? kMinTabCellWidth : averageWidth;
      averageWidth = averageWidth >= kMaxTabCellWidth ? kMaxTabCellWidth : averageWidth;
    }

    CGFloat x = tabListWidth + leftPadding + (index * averageWidth);
    CGFloat y = 0; // Default for IupCocoaTabPositionTop
    // If position is bottom, tabs are aligned to the top of the bar
    if (self.tabPosition == IupCocoaTabPositionBottom)
    {
      y = [self frame].size.height - kTabCellHeight;
    }
    CGFloat width = averageWidth;
    CGFloat height = kTabCellHeight;

    NSRect rect = NSMakeRect(x, y, width, height);
    return rect;
  }
  else // IupCocoaTabBarVertical
  {
    NSUInteger tabListHeight = self.allowsTabListMenu ? kHeightOfTabList : 0;
    CGFloat viewHeight = [self frame].size.height;

    CGFloat x = 0;
    // Since the view is NOT flipped, y=0 is at the bottom.
    // We must calculate the y-origin (bottom-left) from the top.
    CGFloat y = viewHeight - tabListHeight - ((index + 1) * kTabCellHeight);
    CGFloat width = [self frame].size.width; // Use the full width of the tab bar
    CGFloat height = kTabCellHeight; // Use fixed kTabCellHeight

    NSRect rect = NSMakeRect(x, y, width, height);
    return rect;
  }
}

// Rect for left most (horizontal) or top (vertical) tab list control
- (NSRect)rectForTabListControl
{
  if (!self.allowsTabListMenu)
  {
    return NSZeroRect;
  }

  NSRect rect;
  if (self.orientation == IupCocoaTabBarHorizontal)
  {
    rect = NSMakeRect(0, 0, kWidthOfTabList, kHeightOfTabList);
    rect = CGRectInset(rect, 6, 10);
  }
  else
  {
    rect = NSMakeRect(0, 0, [self frame].size.width, kHeightOfTabList);
    rect = CGRectInset(rect, 10, 6);
  }
  return rect;
}

- (BOOL)isBlankAreaOfTabBarViewInPoint:(NSPoint)p
{
  if (self.allowsTabListMenu)
  {
    // Check tab list control
    NSRect rect = [self rectForTabListControl];
    if (NSPointInRect(p, rect))
    {
      return NO;
    }
  }

  // Check all tabs path
  NSUInteger index = 0;
  for (index = 0; index < [tabs count]; ++index)
  {
    IupCocoaTabCell *tab = [tabs objectAtIndex:index];
    NSRect rect = [tab frame];
    if (NSPointInRect(p, rect))
    {
      return NO;
    }
  }

  return YES;
}

- (NSMenu *)tabsMenu
{
  [menu release];
  menu = [[NSMenu alloc] initWithTitle:@"Contextual Menu"];
  NSUInteger index = 0;
  for (index = 0; index < [tabs count]; index++)
  {
    IupCocoaTabCell *tab = [tabs objectAtIndex:index];
    [menu insertItemWithTitle:[tab title] action:@selector(popupMenuDidChoosed:) keyEquivalent:@"" atIndex:index];
  }
  return menu; // Return the retained ivar
}

- (void)popupMenuDidChoosed:(NSMenuItem*)item
{
  NSUInteger index = [menu indexOfItem:item];
  if (index != -1)
  {
    IupCocoaTabCell *tab = [tabs objectAtIndex:index];
    NSRect tabRect = [tab frame];
    NSRect tabBarViewRect = [self bounds];
    // If the selected tab is not fully shown in the tabbar view, we
    // then exchange it with first(0 index) tab, and then active it.
    if (!CGRectContainsRect(tabBarViewRect, tabRect))
    {
      NSUInteger tabIndex = [[self tabs] indexOfObject:tab];
      [self exchangeTabWithIndex:tabIndex withTabIndex:0];
    }
    [tab setAsActiveTab];
  }
}

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
  return YES;
}

- (IupCocoaTabCell*)tabCellInPoint:(NSPoint)p
{
  NSUInteger index = 0;
  for (index = 0; index < [tabs count]; index++)
  {
    IupCocoaTabCell *tab = [tabs objectAtIndex:index];
    NSRect rect = [tab frame];
    if (NSPointInRect(p, rect))
    {
      return tab;
    }
  }
  return nil;
}

- (NSInteger)destinationCellIndexFromPoint:(NSPoint)p
{
  NSInteger index = 0;
  for (index = 0; index < [tabs count]; index++)
  {
    IupCocoaTabCell *tab = [tabs objectAtIndex:index];
    NSRect rect = [tab frame];

    if (self.orientation == IupCocoaTabBarHorizontal)
    {
      CGFloat midX = NSMidX(rect);
      CGFloat minX = NSMinX(rect);
      CGFloat maxX = NSMaxX(rect);
      CGFloat minY = NSMinY(rect);
      CGFloat w = rect.size.width;
      CGFloat h = rect.size.height;

      NSInteger ret;
      NSRect firstRect = NSMakeRect(minX, minY, w/2, h);
      NSRect secondeRect = NSMakeRect(midX, minY, w/2, h);
      if (NSPointInRect(p, firstRect))
      {
        ret = index; // Use current index, not index-1, to ensure correct insertion point
        ret = ret >= 0 ? ret : 0;
        if (destinationIndex != -1 && index == destinationIndex)
        {
          return destinationIndex;
        }
        return ret;
      }
      else if (NSPointInRect(p, secondeRect))
      {
        ret = index + 1;
        if (destinationIndex != -1 && index > destinationIndex)
        {
          ret -= 1;
        }
        else if (destinationIndex != -1 && index == destinationIndex)
        {
          return destinationIndex;
        }
        return ret;
      }

      if (p.x > maxX && index == [tabs count] - 1)
      {
        if (destinationIndex != -1 && index >= destinationIndex)
        {
          return index;
        }
        else
        {
          return index + 1;
        }
      }
    }
    else // Vertical
    {
      CGFloat midY = NSMidY(rect);
      CGFloat minY = NSMinY(rect);
      CGFloat maxY = NSMaxY(rect);
      CGFloat minX = NSMinX(rect);
      CGFloat w = rect.size.width;
      CGFloat h = rect.size.height;

      NSInteger ret;
      NSRect firstRect = NSMakeRect(minX, minY, w, h/2);
      NSRect secondeRect = NSMakeRect(minX, midY, w, h/2);
      if (NSPointInRect(p, firstRect))
      {
        ret = index;
        ret = ret >= 0 ? ret : 0;
        if (destinationIndex != -1 && index == destinationIndex)
        {
          return destinationIndex;
        }
        return ret;
      }
      else if (NSPointInRect(p, secondeRect))
      {
        ret = index + 1;
        if (destinationIndex != -1 && index > destinationIndex)
        {
          ret -= 1;
        }
        else if (destinationIndex != -1 && index == destinationIndex)
        {
          return destinationIndex;
        }
        return ret;
      }

      if (p.y > maxY && index == [tabs count] - 1)
      {
        if (destinationIndex != -1 && index >= destinationIndex)
        {
          return index;
        }
        else
        {
          return index + 1;
        }
      }
    }
  }
  return -1;
}

- (void)exchangeTabWithIndex:(NSUInteger)one withTabIndex:(NSUInteger)two
{
  NSMutableArray *allTabs = [self tabs];
  [allTabs exchangeObjectAtIndex:one withObjectAtIndex:two];
}
@end


@implementation IupCocoaTabBarView

@synthesize tabs;
@synthesize bgColor;
@synthesize tabBGColor;
@synthesize tabActivedBGColor;
@synthesize tabBorderColor;
@synthesize tabTitleColor;
@synthesize tabActivedTitleColor;
@synthesize smallControlColor;
@synthesize tabFont;
@synthesize delegate;
@synthesize selectedTab;
@synthesize orientation;
@synthesize tabPosition;
@synthesize allowsDragging;
@synthesize allowsTabListMenu;
@synthesize showsCloseButtonOnHover;
@synthesize allowsAddingTabsByDoubleClick;

- (id)initWithFrame:(NSRect)frame
{
  self = [super initWithFrame:frame];
  if (self)
  {
    tabs = [[NSMutableArray alloc] init]; // Alloc and retain

    // Give all colors a default value if none given
    // Use system-adaptive colors for dark/light mode
    bgColor = [[NSColor controlBackgroundColor] retain];
    tabBGColor = [[NSColor underPageBackgroundColor] retain]; // Use for inactive tabs
    tabActivedBGColor = [[NSColor windowBackgroundColor] retain];
    tabBorderColor = [[NSColor separatorColor] retain];
    tabTitleColor = [[NSColor secondaryLabelColor] retain];
    tabActivedTitleColor = [[NSColor labelColor] retain];
    smallControlColor = [[NSColor secondaryLabelColor] retain];

    // Font
    tabFont = [[NSFont fontWithName:@"Lucida Grande" size:11] retain];

    destinationIndex = -1;
    sourceIndex = -1;
    isDragging = NO;

    orientation = IupCocoaTabBarHorizontal; // Default orientation
    tabPosition = IupCocoaTabPositionTop; // Default position
    allowsDragging = NO; // Disabled by default
    allowsTabListMenu = NO; // Disabled by default
    showsCloseButtonOnHover = NO; // Default to persistent close buttons
    allowsAddingTabsByDoubleClick = NO; // Disabled by default
  }
  return self;
}

- (void)dealloc
{
  [tabs release];
  [bgColor release];
  [tabBGColor release];
  [tabActivedBGColor release];
  [tabBorderColor release];
  [tabTitleColor release];
  [tabActivedTitleColor release];
  [smallControlColor release];
  [tabFont release];

  [tabListControlPath release];
  [trackingArea release];
  [menu release];
  [draggingTab release];
  [draggingImage release];

  [selectedTab release];

  [super dealloc];
}

-(void)awakeFromNib
{
  if (tabs == nil)
  {
    tabs = [[NSMutableArray alloc] init];
  }
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize
{
  // The frame is now managed by the IupTabsRootView's layout method,
  // which correctly positions the bar based on TABTYPE.
  // We just need to mark for display and layout.
  [self setNeedsDisplay:YES];
  [self setNeedsLayout:YES];
}

- (void)redraw
{
  [self setNeedsDisplay:YES];
  [self resizeWithOldSuperviewSize:NSZeroSize];
}

- (void)removeTabCell:(IupCocoaTabCell*)tabCell
{
  NSUInteger index = [[self tabs] indexOfObject:tabCell];
  if (index > 0)
  {
    index--;
  }
  [[self tabs] removeObject:tabCell];
  if ([tabs count] > 0)
  {
    IupCocoaTabCell *nextTab = [tabs objectAtIndex:index];
    [nextTab setAsActiveTab];
  }
  [self redraw];
}

- (void)mouseUp:(NSEvent*)event
{
  if (self.allowsAddingTabsByDoubleClick && event.clickCount == 2) // We capture user double click on tabbar view
  {
    NSPoint p =[event locationInWindow];
    p = [self convertPoint:p fromView:[[self window] contentView]];
    if ([self isBlankAreaOfTabBarViewInPoint:p])
    {
      IupCocoaTabCell *tab = [self addTabViewWithTitle:@"Untitled"];
      [tab setAsActiveTab];
      [self redraw];
    }
  }
}

- (void)drawRect:(NSRect)dirtyRect
{
  [super drawRect:dirtyRect];

  // Drawing background color of Tab bar view.
  [bgColor set];
  NSRect rect = [self frame];
  rect.origin = NSZeroPoint;
  NSRectFill(rect);


  // Draw tab list control
  if (self.allowsTabListMenu)
  {
    [tabListControlPath release];
    tabListControlPath = [[NSBezierPath bezierPath] retain];
    NSRect tabListRect = [self rectForTabListControl];
    tabListRect = NSIntegralRect(tabListRect);
    int maxY = NSMaxY(tabListRect);
    int minY = NSMinY(tabListRect);
    int minX = NSMinX(tabListRect);
    int maxX = NSMaxX(tabListRect);
    int midX = NSMidX(tabListRect);

    NSPoint p1 = NSMakePoint(midX, minY);
    NSPoint p2 = NSMakePoint(minX, maxY);
    NSPoint p3 = NSMakePoint(maxX, maxY);

    [tabListControlPath moveToPoint:p1];
    [tabListControlPath lineToPoint:p2];
    [tabListControlPath lineToPoint:p3];
    [tabListControlPath lineToPoint:p1];

    // Use tab active background color to set tab list triangle
    [[self tabActivedBGColor] set];
    [tabListControlPath fill];
  }

  // Drawing border line
  NSPoint start;
  NSPoint end;
  [NSBezierPath setDefaultLineWidth:1.0];
  [tabBorderColor set];

  switch(self.tabPosition)
  {
    case IupCocoaTabPositionTop:
      // Border at bottom of view
      start = NSMakePoint(0, 1);
      end = NSMakePoint(NSMaxX(rect), 1);
      break;
    case IupCocoaTabPositionBottom:
      // Border at top of view
      start = NSMakePoint(0, NSMaxY(rect) - 1);
      end = NSMakePoint(NSMaxX(rect), NSMaxY(rect) - 1);
      break;
    case IupCocoaTabPositionLeft:
      // Border at right of view
      start = NSMakePoint(NSMaxX(rect) - 1, 0);
      end = NSMakePoint(NSMaxX(rect) - 1, NSMaxY(rect));
      break;
    case IupCocoaTabPositionRight:
      // Border at left of view
      start = NSMakePoint(1, 0);
      end = NSMakePoint(1, NSMaxY(rect));
      break;
  }
  [NSBezierPath strokeLineFromPoint:start toPoint:end];

  // Reset all tool tips
  [self removeAllToolTips];
  NSInteger index = 0;
  for (index = 0; index < [tabs count]; ++index)
  {
    NSRect rect = [self tabRectFromIndex:index];
    IupCocoaTabCell *tab = [tabs objectAtIndex:index];
    [tab setFrame:rect];
    [self setToolTip:[tab title]];
    [self addToolTipRect:[tab frame] owner:[tab title] userData:nil];
    [tab draw];
  }
}

- (id)addTabViewWithTitle:(NSString *)title
{
  return [self addTabViewWithTitle:title image:nil];
}

- (id)addTabViewWithTitle:(NSString *)title image:(NSImage *)image
{
  IupCocoaTabCell *tab = [IupCocoaTabCell tabCellWithTabBarView:self title:title image:image];

  if ([[self delegate] respondsToSelector:@selector(tabWillBeCreated:)])
  {
    [[self delegate] tabWillBeCreated:tab];
  }

  [tabs addObject:tab];

  // If the new tab(add it to last) is not fully shown in the tabbar view, we
  // then exchange it with first(0 index) tab, and then set it as active.
  NSUInteger tabIndex = [[self tabs] indexOfObject:tab];
  NSRect tabBarViewRect = [self bounds];
  NSRect tabRect = [self tabRectFromIndex:tabIndex];
  if (!CGRectContainsRect(tabBarViewRect, tabRect))
  {
    [self exchangeTabWithIndex:tabIndex withTabIndex:0];
  }

  [tab setAsActiveTab];

  if ([[self delegate] respondsToSelector:@selector(tabDidBeCreated:)])
  {
    [[self delegate] tabDidBeCreated:tab];
  }

  [self redraw];
  return tab;
}

- (void)mouseDown:(NSEvent *)theEvent
{
  NSPoint p = [theEvent locationInWindow];
  p = [self convertPoint:p fromView:[[self window] contentView]];

  /* Check if tabs list control clicked */
  if (self.allowsTabListMenu)
  {
    NSRect rectOfTabList;
    if (self.orientation == IupCocoaTabBarHorizontal)
    {
      rectOfTabList = NSMakeRect(0, 0, kWidthOfTabList, kHeightOfTabList);
    }
    else
    {
      rectOfTabList = NSMakeRect(0, 0, [self frame].size.width, kHeightOfTabList);
    }

    if (NSPointInRect(p, rectOfTabList))
    {
      [NSMenu popUpContextMenu:[self tabsMenu] withEvent:theEvent forView:self];
    }
  }


  /* Switch active tab */
  NSUInteger index = 0;
  for (index = 0; index < [tabs count]; ++ index)
  {
    IupCocoaTabCell *tab = [tabs objectAtIndex:index];
    if (NSPointInRect(p, [tab frame])) // Use frame, not path
    {
      [tab setAsActiveTab];
    }

    // forward mouse down to tab cell
    [tab mouseDown:theEvent];
  };

  [self redraw];

  isDragging = NO;
}

- (void)mouseMoved:(NSEvent *)theEvent
{
  NSPoint p = [theEvent locationInWindow];
  p = [self convertPoint:p fromView:[[self window] contentView]];

  /* Switch active tab */
  NSUInteger index = 0;
  for (index = 0; index < [tabs count]; ++ index)
  {
    IupCocoaTabCell *tab = [tabs objectAtIndex:index];
    // forward mouse moved to tab cell
    [tab mouseMoved:theEvent];
  };

  [self redraw];
}

- (void)setFrame:(NSRect)frame
{
  [super setFrame:frame];
  [self removeTrackingArea:trackingArea];
  [trackingArea release];

  NSTrackingAreaOptions options = (NSTrackingActiveAlways | NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);
  trackingArea = [[NSTrackingArea alloc] initWithRect:[self frame] options:options owner:self userInfo:nil]; // Create and retain new one
  [self addTrackingArea:trackingArea];
}

- (void)setBounds:(NSRect)bounds
{
  [super setBounds:bounds];
  [self removeTrackingArea:trackingArea];
  [trackingArea release];

  NSTrackingAreaOptions options = (NSTrackingActiveAlways | NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);
  trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options:options owner:self userInfo:nil]; // Create and retain new one
  [self addTrackingArea:trackingArea];
}

- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)flag
{
  return NSDragOperationNone;
}

- (void)mouseDragged:(NSEvent *)theEvent
{
  if (!self.allowsDragging)
  {
    [super mouseDragged:theEvent];
    return;
  }

  NSPoint p = [theEvent locationInWindow];
  p = [self convertPoint:p fromView:[[self window] contentView]];
  if (!isDragging)
  {
    [draggingTab release];
    draggingTab = [[self tabCellInPoint:p] retain];
    isDragging = YES;
    if (draggingTab != nil)
    {
      [draggingImage release];
      draggingImage = [[IupCocoaTabImage imageWithIupCocoaTabCell:draggingTab] retain];
      [draggingTab setIsDraggingTab:YES];
      // Save source index
      sourceIndex = [tabs indexOfObject:draggingTab];
      [tabs removeObject:draggingTab];
      [self redraw];

      // Modern drag-and-drop requires a pasteboard item.
      NSPasteboardItem *pbItem = [[[NSPasteboardItem alloc] init] autorelease];
      // Set dummy data to satisfy the API.
      [pbItem setData:[NSData data] forType:NSPasteboardTypeString];

      NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];
      [pasteboard clearContents];
      [pasteboard writeObjects:@[pbItem]];
    }
  }

  if (draggingTab == nil)
  {
    return ;
  }

  NSSize offset = NSMakeSize(0.0, 0.0);

  p.x -= (draggingTab.frame.size.width / 2);
  p.y -= (draggingTab.frame.size.height / 2);

  // Get the pasteboard we prepared at the start of the drag.
  NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];

  [self dragImage:draggingImage at:p offset:offset event:theEvent pasteboard:pasteboard source:self slideBack:NO];
}

- (void)draggedImage:(NSImage *)image movedTo:(NSPoint)screenPoint
{
  screenPoint.x += (draggingTab.frame.size.width / 2);
  screenPoint.y += (draggingTab.frame.size.height / 2);
  NSPoint windowLocation = [[self window] convertScreenToBase:screenPoint];
  NSPoint viewPoint = [self convertPoint:windowLocation fromView:nil];

  NSRect tabBarViewRect = [self bounds];

  if (!NSPointInRect(viewPoint, tabBarViewRect))
  {
    [tabs removeObject:draggingTab];
    destinationIndex = -1;
    [self redraw];
    return ;
  }

  destinationIndex = [self destinationCellIndexFromPoint:viewPoint];
  if (destinationIndex == -1) return ;

  [tabs removeObject:draggingTab];
  [tabs insertObject:draggingTab atIndex:destinationIndex];
  [self redraw];
}

- (void)draggedImage:(NSImage *)image endedAt:(NSPoint)screenPoint operation:(NSDragOperation)operation
{
  screenPoint.x += (draggingTab.frame.size.width / 2);
  screenPoint.y += (draggingTab.frame.size.height / 2);
  NSPoint windowLocation = [[self window] convertScreenToBase:screenPoint];
  NSPoint viewPoint = [self convertPoint:windowLocation fromView:nil];

  NSRect tabBarViewRect = [self bounds];
  if (!NSPointInRect(viewPoint, tabBarViewRect))
  {
    [draggingTab setIsDraggingTab:NO];
    [tabs removeObject:draggingTab];
    [tabs insertObject:draggingTab atIndex:sourceIndex];

    [draggingTab release];
    draggingTab = nil;
    [draggingImage release];
    draggingImage = nil;

    [self redraw];
    sourceIndex = -1;
    destinationIndex = -1;
    isDragging = NO;
    return ;
  }

  destinationIndex = [self destinationCellIndexFromPoint:viewPoint];
  if (destinationIndex == -1)
  {
    // Dropped in blank area, restore to source index
    [tabs removeObject:draggingTab];
    [tabs insertObject:draggingTab atIndex:sourceIndex];
  }
  else
  {
    [draggingTab setIsDraggingTab:NO];
    [tabs removeObject:draggingTab];
    [tabs insertObject:draggingTab atIndex:destinationIndex];
  }

  [draggingTab release];
  draggingTab = nil;
  [draggingImage release];
  draggingImage = nil;

  [self redraw];

  destinationIndex = -1;
  sourceIndex = -1;
  isDragging = NO;
}

- (void)setTabFont:(NSFont *)newFont
{
  if (tabFont != newFont)
  {
    [tabFont release];
    tabFont = [newFont retain];
  }
}

- (void)setTabs:(NSMutableArray *)newTabs
{
  if (tabs != newTabs)
  {
    [tabs release];
    tabs = [newTabs retain];
  }
}

- (void)setSelectedTab:(IupCocoaTabCell *)newSelectedTab
{
  if (selectedTab != newSelectedTab)
  {
    [selectedTab release];
    selectedTab = [newSelectedTab retain];
  }
}

- (void)setBgColor:(NSColor *)newBgColor
{
  if (bgColor != newBgColor)
  {
    [bgColor release];
    bgColor = [newBgColor retain];
  }
}

- (void)setTabBGColor:(NSColor *)newTabBGColor
{
  if (tabBGColor != newTabBGColor)
  {
    [tabBGColor release];
    tabBGColor = [newTabBGColor retain];
  }
}

- (void)setTabActivedBGColor:(NSColor *)newTabActivedBGColor
{
  if (tabActivedBGColor != newTabActivedBGColor)
  {
    [tabActivedBGColor release];
    tabActivedBGColor = [newTabActivedBGColor retain];
  }
}

- (void)setTabBorderColor:(NSColor *)newTabBorderColor
{
  if (tabBorderColor != newTabBorderColor)
  {
    [tabBorderColor release];
    tabBorderColor = [newTabBorderColor retain];
  }
}

- (void)setTabTitleColor:(NSColor *)newTabTitleColor
{
  if (tabTitleColor != newTabTitleColor)
  {
    [tabTitleColor release];
    tabTitleColor = [newTabTitleColor retain];
  }
}

- (void)setTabActivedTitleColor:(NSColor *)newTabActivedTitleColor
{
  if (tabActivedTitleColor != newTabActivedTitleColor)
  {
    [tabActivedTitleColor release];
    tabActivedTitleColor = [newTabActivedTitleColor retain];
  }
}

- (void)setSmallControlColor:(NSColor *)newSmallControlColor
{
  if (smallControlColor != newSmallControlColor)
  {
    [smallControlColor release];
    smallControlColor = [newSmallControlColor retain];
  }
}

@end


@implementation IupCocoaTabCell

@synthesize path;
@synthesize frame;
@synthesize isActived;
@synthesize tabBarView;
@synthesize title;
@synthesize titleAttributedString;
@synthesize canDrawCloseButton;
@synthesize isDraggingTab;
@synthesize image;
@synthesize hasCloseButton;

- (void)dealloc
{
  [path release];
  [title release];
  [titleAttributedString release];
  [image release];
  [super dealloc];
}

+ (id)tabCellWithTabBarView:(IupCocoaTabBarView*)tabBarView title:(NSString*)aTittle image:(NSImage*)anImage
{
  IupCocoaTabCell *tabCell = [[[IupCocoaTabCell alloc] init] autorelease];
  [tabCell setTabBarView:tabBarView];
  [tabCell setIsActived:NO];
  [tabCell setTitle:aTittle];
  [tabCell setImage:anImage];
  [tabCell setCanDrawCloseButton:NO];
  [tabCell setHasCloseButton:NO];
  [tabCell setIsDraggingTab:NO];
  return tabCell;
}

- (NSRect)closeButtonRect
{
  NSRect tabRect = [self frame];
  NSRect rect;

  if ([[self tabBarView] orientation] == IupCocoaTabBarHorizontal)
  {
    CGFloat maxX = NSMaxX(tabRect);
    CGFloat minY = NSMinY(tabRect);
    rect = NSMakeRect(maxX - tabRect.size.height, minY, tabRect.size.height, tabRect.size.height);
  }
  else // Vertical
  {
    CGFloat maxX = NSMaxX(tabRect);
    CGFloat minY = NSMinY(tabRect);
    CGFloat buttonSize = kTabCellHeight;
    rect = NSMakeRect(maxX - buttonSize, minY, buttonSize, buttonSize);
  }

  rect = CGRectInset(rect, kCloseButtonWidth + 2, kCloseButtonWidth + 2);
  return rect;
}

- (void)drawCloseButton
{
  NSRect closeButtonRect = [self closeButtonRect];
  NSBezierPath *closeButtonPath = [NSBezierPath bezierPath];
  CGFloat minX = NSMinX(closeButtonRect);
  CGFloat maxX = NSMaxX(closeButtonRect);
  CGFloat minY = NSMinY(closeButtonRect);
  CGFloat maxY = NSMaxY(closeButtonRect);

  NSPoint leftBottomPoint = NSMakePoint(minX, minY);
  NSPoint leftTopPoint = NSMakePoint(minX, maxY);
  NSPoint rightBottomPoint = NSMakePoint(maxX, minY);
  NSPoint rightTopPoint = NSMakePoint(maxX, maxY);

  [closeButtonPath moveToPoint:leftBottomPoint];
  [closeButtonPath lineToPoint:rightTopPoint];

  [closeButtonPath moveToPoint:leftTopPoint];
  [closeButtonPath lineToPoint:rightBottomPoint];

  [closeButtonPath setLineWidth:2.0];
  [[[self tabBarView] smallControlColor] set];

  [closeButtonPath stroke];
}

// tab cell draw itself in this method, called in TabBarView's drawRect method
- (void)draw
{
  NSRect rect = [self frame];
  [path release];
  path = [[NSBezierPath bezierPathWithRect:rect] retain]; // Use simple rectangle
  [path setLineWidth:1.0]; // Use 1px for separators

  if ([self isActived])
  {
    [[[self tabBarView] tabActivedBGColor] set];
  }
  else
  {
    [[[self tabBarView] tabBGColor] set];
  }
  [path fill];

  // Draw separators instead of full borders
  [[[self tabBarView] tabBorderColor] set];
  NSUInteger myIndex = [[[self tabBarView] tabs] indexOfObject:self];
  NSUInteger tabCount = [[[self tabBarView] tabs] count];

  if ([[self tabBarView] orientation] == IupCocoaTabBarHorizontal)
  {
    // Draw vertical separator on the right, if not the last tab
    if (myIndex < tabCount - 1)
    {
      [NSBezierPath strokeLineFromPoint:NSMakePoint(NSMaxX(rect), NSMinY(rect))
                              toPoint:NSMakePoint(NSMaxX(rect), NSMaxY(rect))];
    }
  }
  else // Vertical
  {
    // Draw horizontal separator at the bottom, if not the last tab
    // (Last tab is index count-1, which is visually at the bottom)
    if (myIndex < tabCount - 1)
    {
      [NSBezierPath strokeLineFromPoint:NSMakePoint(NSMinX(rect), NSMinY(rect))
                              toPoint:NSMakePoint(NSMaxX(rect), NSMinY(rect))];
    }
  }


  NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
  NSColor *fontColor = [self isActived] ? [[self tabBarView] tabActivedTitleColor] : [[self tabBarView] tabTitleColor];
  NSMutableParagraphStyle* p = [[[NSMutableParagraphStyle alloc] init] autorelease];

  // Both horizontal and vertical tabs use centered text
  p.alignment = kCTTextAlignmentCenter;
  p.lineBreakMode = NSLineBreakByTruncatingTail;

  [attrs setObject:[[self tabBarView] tabFont] forKey:NSFontAttributeName];
  [attrs setObject:fontColor forKey:NSForegroundColorAttributeName];
  [attrs setObject:p forKey:NSParagraphStyleAttributeName];

  NSString *currentTitle = [self title];
  if (currentTitle == nil)
  {
    currentTitle = @"";
  }

  NSMutableAttributedString *mas = [[[NSMutableAttributedString alloc] initWithString:currentTitle attributes:attrs] autorelease];

  [self setTitleAttributedString:mas];

  CGFloat leftOffset = 0.0;
  if (self.image)
  {
    CGFloat imageSize = 16.0;
    CGFloat imagePadding = 4.0;
    NSRect tabFrame = [self frame];

    NSRect imageRect;
    imageRect.size = NSMakeSize(imageSize, imageSize);

    // The layout *within* a tab is always horizontal (Image | Text | Close)
    // regardless of the tab bar's orientation.
    imageRect.origin.x = tabFrame.origin.x + 8.0;
    imageRect.origin.y = tabFrame.origin.y + (tabFrame.size.height - imageSize) / 2.0;
    leftOffset = 8.0 + imageSize + imagePadding;

    [self.image drawInRect:imageRect
                  fromRect:NSZeroRect
                 operation:NSCompositeSourceOver
                  fraction:1.0];
  }

  NSRect titleRect = [self frame];
  CGFloat fontHeight = self.titleAttributedString.size.height;

  // The text drawing logic is also identical for both orientations.
  // The text is vertically centered and positioned between the image and the close button.
  int yOffset = (titleRect.size.height - fontHeight) / 2.0;
  titleRect.size.height = fontHeight;
  titleRect.origin.y += yOffset;

  CGFloat leftInset = (leftOffset > 0.0) ? leftOffset : 10.0;
  CGFloat rightInset;

  if ([self hasCloseButton])
  {
    // Reserve a square on the right for the close button.
    // The button rect is (width - height, y, height, height).
    rightInset = [self frame].size.height;
  }
  else
  {
    rightInset = 10.0;
  }

  titleRect.origin.x += leftInset;
  titleRect.size.width -= (leftInset + rightInset);

  [self.titleAttributedString drawInRect:titleRect];

  if ([self hasCloseButton])
  {
    if (![[self tabBarView] showsCloseButtonOnHover])
    {
      [self drawCloseButton];
    }
    else
    {
      if ([self canDrawCloseButton])
      {
        [self drawCloseButton];
      }
    }
  }
}

- (void)setAsActiveTab
{
  if ([[self tabBarView] selectedTab] == self)
  {
    return ;
  }

  NSUInteger index = 0;
  NSMutableArray *tabs = [[self tabBarView] tabs];
  for (index = 0; index < [tabs count]; ++ index)
  {
    IupCocoaTabCell *tab = [tabs objectAtIndex:index];
    [tab setIsActived:NO];
  }

  // Call delegate protocol methods
  if ([[[self tabBarView] delegate] respondsToSelector:@selector(tabWillActive:)])
  {
    [[[self tabBarView] delegate] tabWillActive:self];
  }

  [self setIsActived:YES];

  if ([[[self tabBarView] delegate] respondsToSelector:@selector(tabDidActived:)])
  {
    [[[self tabBarView] delegate] tabDidActived:self];
  }
  [[self tabBarView] setSelectedTab:self];
}

- (void)mouseDown:(NSEvent *)theEvent
{
  if (![self hasCloseButton])
  {
    return;
  }

  NSPoint p = [theEvent locationInWindow];
  p = [[self tabBarView] convertPoint:p fromView:[[[self tabBarView] window] contentView]];

  if (NSPointInRect(p ,[self closeButtonRect]))
  {
    // Delete this tab cell
    id delegate = [[self tabBarView] delegate];
    if ([delegate respondsToSelector:@selector(tabWillClose:)])
    {
      [delegate tabWillClose:self];
    }
    [[self tabBarView] removeTabCell:self];
    if ([delegate respondsToSelector:@selector(tabDidClosed:)])
    {
      [delegate tabDidClosed:self];
    }
  }

}

- (void)mouseMoved:(NSEvent *)theEvent
{
  if (![self hasCloseButton] || ![[self tabBarView] showsCloseButtonOnHover])
  {
    if (canDrawCloseButton) // Ensure it's turned off if mode changed
    {
      canDrawCloseButton = NO;
    }
    return;
  }

  NSPoint p = [theEvent locationInWindow];
  p = [[self tabBarView] convertPoint:p fromView:[[[self tabBarView] window] contentView]];

  if (NSPointInRect(p ,[self closeButtonRect]))
  {
    canDrawCloseButton = YES;
  }
  else
  {
    canDrawCloseButton = NO;
  }
}

- (void)setTitle:(NSString *)newTitle
{
  if (title != newTitle)
  {
    [title release];
    title = [newTitle copy];
  }
}

- (void)setImage:(NSImage *)newImage
{
  if (image != newImage)
  {
    [image release];
    image = [newImage retain];
  }
}

- (void)setTitleAttributedString:(NSMutableAttributedString *)newTitleAttributedString
{
  if (titleAttributedString != newTitleAttributedString)
  {
    [titleAttributedString release];
    titleAttributedString = [newTitleAttributedString retain];
  }
}

@end


@implementation IupCocoaTabImage
@synthesize tab;

- (void)dealloc
{
  [tab release];
  [super dealloc];
}

+ (id)imageWithIupCocoaTabCell:(IupCocoaTabCell*)tabCell
{
  NSRect rect = NSMakeRect(0, 0, tabCell.frame.size.width, tabCell.frame.size.height);
  IupCocoaTabImage *image = [[[IupCocoaTabImage alloc] initWithSize:rect.size] autorelease];
  [image setTab:tabCell];

  // Reset tab cell's frame
  [[image tab] setFrame:rect];

  [image lockFocus];

  // Transparent
  [[NSColor clearColor] set];
  NSRectFill(rect);

  [[image tab] draw];

  [image unlockFocus];
  return image;
}

- (void)setTab:(IupCocoaTabCell *)newTab
{
  if (tab != newTab)
  {
    [tab release];
    tab = [newTab retain];
  }
}
@end
