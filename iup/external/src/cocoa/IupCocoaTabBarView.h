/** \file
 * \brief TabBar Control
 *
 * Custom IupCocoaTabBarView implementation.
 *
 * Based on ENTabBarView created by Aaron Elkins.
 * ENTabBarView is released under MIT license.
 */

#import <Cocoa/Cocoa.h>

#define kTabBarViewHeight 32
#define kWidthOfTabList 24
#define kTabBarSidePadding 16
#define kHeightOfTabList 28
#define kMaxTabCellWidth 180
#define kMinTabCellWidth 50
#define kTabCellHeight 28
#define kCloseButtonWidth 8
#define kTabCellPadding 12

@protocol IupCocoaTabBarViewDelegate;
@class IupCocoaTabCell;

typedef NS_ENUM(NSUInteger, IupCocoaTabBarOrientation) {
  IupCocoaTabBarHorizontal,
  IupCocoaTabBarVertical
};

typedef NS_ENUM(NSUInteger, IupCocoaTabPosition) {
  IupCocoaTabPositionTop,
  IupCocoaTabPositionBottom,
  IupCocoaTabPositionLeft,
  IupCocoaTabPositionRight
};

typedef NS_ENUM(NSUInteger, IupCocoaTabTextOrientation) {
  IupCocoaTabTextHorizontal,
  IupCocoaTabTextVertical
};

@interface IupCocoaTabBarView : NSView<NSMenuDelegate, NSDraggingSource>
{
  NSMutableArray* tabs;
  NSBezierPath* tabListControlPath;
  NSTrackingArea* trackingArea;
  NSMenu* menu;

  BOOL isDragging;
  BOOL isSettling;
  NSInteger destinationIndex;
  NSInteger sourceIndex;
  IupCocoaTabCell* draggingTab;
  NSPoint dragOrigin;
  CGFloat dragGrabOffset;
  NSTimer* dragTimer;

  IupCocoaTabCell* selectedTab;
  NSFont* tabFont;
  NSColor* bgColor;
  NSColor* tabBGColor;
  NSColor* tabActivedBGColor;
  NSColor* tabBorderColor;
  NSColor* tabTitleColor;
  NSColor* tabActivedTitleColor;
  NSColor* smallControlColor;
  id<IupCocoaTabBarViewDelegate> delegate;

  IupCocoaTabBarOrientation orientation;
  IupCocoaTabPosition tabPosition;
  IupCocoaTabTextOrientation textOrientation;
  BOOL allowsDragging;
  BOOL allowsTabListMenu;
  BOOL showsCloseButtonOnHover;
  BOOL allowsAddingTabsByDoubleClick;
  BOOL enabled;
  BOOL usesMaterialBackground;
  NSMutableArray* accessibilityElements;
}

@property(nonatomic, retain) NSFont* tabFont;
@property(nonatomic, retain) NSMutableArray* tabs;
@property(nonatomic, retain) IupCocoaTabCell* selectedTab;
@property(nonatomic, retain) NSColor* bgColor;
@property(nonatomic, retain) NSColor* tabBGColor;
@property(nonatomic, retain) NSColor* tabActivedBGColor;
@property(nonatomic, retain) NSColor* tabBorderColor;
@property(nonatomic, retain) NSColor* tabTitleColor;
@property(nonatomic, retain) NSColor* tabActivedTitleColor;
@property(nonatomic, retain) NSColor* smallControlColor;
@property(nonatomic, assign) id<IupCocoaTabBarViewDelegate> delegate;
@property(nonatomic, assign) IupCocoaTabBarOrientation orientation;
@property(nonatomic, assign) IupCocoaTabPosition tabPosition;
@property(nonatomic, assign) IupCocoaTabTextOrientation textOrientation;
@property(nonatomic, assign) BOOL allowsDragging;
@property(nonatomic, assign) BOOL allowsTabListMenu;
@property(nonatomic, assign) BOOL showsCloseButtonOnHover;
@property(nonatomic, assign) BOOL allowsAddingTabsByDoubleClick;
@property(nonatomic, assign, getter=isEnabled) BOOL enabled;
@property(nonatomic, assign) BOOL usesMaterialBackground;

- (id)addTabViewWithTitle:(NSString*)title;
- (id)addTabViewWithTitle:(NSString*)title image:(NSImage*)image;
- (void)redraw;
- (void)removeTabCell:(IupCocoaTabCell*)tabCell;
@end

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

@protocol IupCocoaTabBarViewDelegate <NSObject>

@optional

- (void)tabWillActive:(IupCocoaTabCell*)tab;
- (void)tabDidActived:(IupCocoaTabCell*)tab;

- (BOOL)tabWillClose:(IupCocoaTabCell*)tab;
- (void)tabDidClosed:(IupCocoaTabCell*)tab;

- (void)tabWillBeCreated:(IupCocoaTabCell*)tab;
- (void)tabDidBeCreated:(IupCocoaTabCell*)tab;
- (BOOL)tabWillReorderFromIndex:(NSInteger)sourceIdx toIndex:(NSInteger)destinationIdx;
@end

@interface IupCocoaTabCell : NSObject
{
  NSBezierPath* path;
  BOOL canDrawCloseButton;
  NSString* title;
  NSMutableAttributedString* titleAttributedString;
  NSRect frame;
  IupCocoaTabBarView* tabBarView;
  BOOL isActived;
  BOOL isDraggingTab;
  NSImage* image;
  BOOL hasCloseButton;
  BOOL isHovered;
  BOOL isPressed;
  NSPoint displayOrigin;
  BOOL hasDisplayOrigin;
}

@property(nonatomic, assign) BOOL canDrawCloseButton;
@property(nonatomic, copy) NSString* title;
@property(nonatomic, retain) NSImage* image;
@property(nonatomic, retain) NSMutableAttributedString* titleAttributedString;
@property(readonly) NSBezierPath* path;
@property(nonatomic, assign) NSRect frame;
@property(nonatomic, assign) IupCocoaTabBarView* tabBarView;
@property(nonatomic, assign) BOOL isActived;
@property(nonatomic, assign) BOOL isDraggingTab;
@property(nonatomic, assign) BOOL hasCloseButton;
@property(nonatomic, assign) NSPoint displayOrigin;
@property(nonatomic, assign) BOOL hasDisplayOrigin;

+ (id)tabCellWithTabBarView:(IupCocoaTabBarView*)tabBarView title:(NSString*)aTittle image:(NSImage*)anImage;
- (void)setAsActiveTab;
- (void)draw;

- (void)mouseDown:(NSEvent*)theEvent;
- (void)mouseMoved:(NSEvent*)theEvent;
- (void)setIsHovered:(BOOL)flag;
- (void)setIsPressed:(BOOL)flag;
@end

