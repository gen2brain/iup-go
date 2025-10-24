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
#define kHeightOfTabList 28
#define kMaxTabCellWidth 180
#define kMinTabCellWidth 100
#define kTabCellHeight 28
#define kBorderWidth 2
#define kCloseButtonWidth 8
#define kDeltaXfromLeftAndRight 2

@protocol IupCocoaTabBarViewDelegate;
@class IupCocoaTabCell;
@class IupCocoaTabImage;

typedef NS_ENUM(NSUInteger, IupCocoaTabBarOrientation) {
  IupCocoaTabBarHorizontal,
  IupCocoaTabBarVertical
};

// New enum for border position
typedef NS_ENUM(NSUInteger, IupCocoaTabPosition) {
  IupCocoaTabPositionTop,
  IupCocoaTabPositionBottom,
  IupCocoaTabPositionLeft,
  IupCocoaTabPositionRight
};

@interface IupCocoaTabBarView : NSView<NSMenuDelegate>
{
  NSMutableArray* tabs;
  NSBezierPath* tabListControlPath;
  NSTrackingArea* trackingArea;
  NSMenu* menu;

  BOOL isDragging;
  NSInteger destinationIndex;
  NSInteger sourceIndex;
  IupCocoaTabCell* draggingTab;
  IupCocoaTabImage* draggingImage;

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
  IupCocoaTabPosition tabPosition; // New ivar
  BOOL allowsDragging;
  BOOL allowsTabListMenu; // New property
  BOOL showsCloseButtonOnHover;
  BOOL allowsAddingTabsByDoubleClick;
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
@property(nonatomic, assign) IupCocoaTabPosition tabPosition; // New property
@property(nonatomic, assign) BOOL allowsDragging;
@property(nonatomic, assign) BOOL allowsTabListMenu;
@property(nonatomic, assign) BOOL showsCloseButtonOnHover;
@property(nonatomic, assign) BOOL allowsAddingTabsByDoubleClick;

- (id)addTabViewWithTitle:(NSString*)title;
- (id)addTabViewWithTitle:(NSString*)title image:(NSImage*)image;
- (void)redraw;
- (void)removeTabCell:(IupCocoaTabCell*)tabCell;
@end

// IupCocoaTabBarView Delegates methods
// Implement these methods to intercept your code
//
@protocol IupCocoaTabBarViewDelegate <NSObject>

@optional

/*
 * Usually we store tabs (IupCocoaTabCell*) as a key in a NSDitionary,
 * And other object as a value to identify which tab in the foll-
 * owing events.
 * If you need to know the tab index(order), you can get it like
 * this:
 * NSInteger index = [[tabBarView tabs] indexOfObject:tab]];
 * Here **tabBarView** is an instance of IupCocoaTabBarView
 */

- (void)tabWillActive:(IupCocoaTabCell*)tab;
- (void)tabDidActived:(IupCocoaTabCell*)tab;

- (void)tabWillClose:(IupCocoaTabCell*)tab;
- (void)tabDidClosed:(IupCocoaTabCell*)tab;

- (void)tabWillBeCreated:(IupCocoaTabCell*)tab;
- (void)tabDidBeCreated:(IupCocoaTabCell*)tab;
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

+ (id)tabCellWithTabBarView:(IupCocoaTabBarView*)tabBarView title:(NSString*)aTittle image:(NSImage*)anImage;
- (void)setAsActiveTab;
- (void)draw;

/* forward mouse event to tab */
- (void)mouseDown:(NSEvent*)theEvent;
- (void)mouseMoved:(NSEvent*)theEvent;
@end

@interface IupCocoaTabImage : NSImage
{
  IupCocoaTabCell* tab;
}

@property(nonatomic, retain) IupCocoaTabCell* tab;
+ (id)imageWithIupCocoaTabCell:(IupCocoaTabCell*)tabCell;
@end
