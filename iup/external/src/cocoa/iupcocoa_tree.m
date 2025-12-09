/** \file
 * \brief Tree Control
 *
 * See Copyright Notice in iup.h
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#import <QuartzCore/QuartzCore.h>

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
#include "iup_stdcontrols.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_tree.h"

#include "iup_drvinfo.h"

#include "iupcocoa_drv.h"
#import "IupCocoaTreeToggleTableCellView.h"


static const void* IUP_COCOA_TREE_DELEGATE_OBJ_KEY = "IUP_COCOA_TREE_DELEGATE_OBJ_KEY";
static const void* IUP_COCOA_TREE_TEXTFIELD_OWNER_KEY = "IUP_COCOA_TREE_TEXTFIELD_OWNER_KEY";
static NSString* const IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE = @"br.puc-rio.tecgraf.iup.outlineview.dragdrop";

static void cocoaTreeUpdateDragDrop(Ihandle* ih);

static NSScrollView* cocoaTreeGetScrollView(Ihandle* ih)
{
  NSScrollView* scroll_view = (NSScrollView*)ih->handle;
  NSCAssert([scroll_view isKindOfClass:[NSScrollView class]], @"Expected NSScrollView");
  return scroll_view;
}

static NSOutlineView* cocoaTreeGetOutlineView(Ihandle* ih)
{
  NSScrollView* scroll_view = cocoaTreeGetScrollView(ih);
  NSOutlineView* outline_view = (NSOutlineView*)[scroll_view documentView];
  NSCAssert([outline_view isKindOfClass:[NSOutlineView class]], @"Expected NSOutlineView");
  return outline_view;
}

/* Custom row view to handle HLCOLOR attribute for selection */
@interface IupCocoaTreeRowView : NSTableRowView
@property(nonatomic, assign) Ihandle* ih;
@end

@implementation IupCocoaTreeRowView
@synthesize ih = _ih;

- (void)drawSelectionInRect:(NSRect)dirtyRect
{
  if (self.ih)
  {
    char* color_str = iupAttribGetStr(self.ih, "HLCOLOR");
    if (color_str)
    {
      unsigned char r, g, b;
      if (iupStrToRGB(color_str, &r, &g, &b))
      {
        NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
        [color set];
        NSRectFill(dirtyRect);
        return;
      }
    }
  }
  /* Fallback to default selection drawing if HLCOLOR is not set or ih is nil */
  [super drawSelectionInRect:dirtyRect];
}
@end

@class IupCocoaTreeItem;

/* Be very careful about retain cycles. Nothing in this class retains anything at the moment. */
/* IupCocoaTreeItem retains this. */
@interface IupCocoaTreeToggleReceiver : NSObject
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) IupCocoaTreeItem* treeItem; /* back pointer to tree item (weak) */
- (IBAction) myToggleClickAction:(id)the_sender;
@end

@implementation IupCocoaTreeToggleReceiver

/* all properties are assign, so we don't need to override dealloc */
- (IBAction) myToggleClickAction:(id)the_sender;
{
  Ihandle* ih = [self ihandle];
  IupCocoaTreeItem* tree_item = [self treeItem];
  NSControlStateValue new_state = [the_sender state];

  int item_id = iupTreeFindNodeId(ih, (InodeHandle*)tree_item);

  IFnii action_callback_function = (IFnii)IupGetCallback(ih, "TOGGLEVALUE_CB");
  if(action_callback_function)
  {
    action_callback_function(ih, item_id, (int)new_state);
  }

  if (iupAttribGetBoolean(ih, "MARKWHENTOGGLE"))
  {
    IupSetAttributeId(ih, "MARKED", item_id, new_state > 0 ? "Yes" : "No");
  }
}

@end

@interface IupCocoaTreeItem : NSObject
{
  IupCocoaTreeItem* parentItem;
  NSMutableArray* childrenArray;
  NSString* title;
  int kind; /* ITREE_BRANCH ITREE_LEAF */
  NSControlStateValue checkBoxState;
  bool checkBoxHidden;
  BOOL isDeleted;
  NSImage* bitmapImage;
  NSImage* collapsedImage;
  NSColor* textColor;
  NSFont* font;
  IupCocoaTreeToggleReceiver* toggleReceiver; /* For TOGGLE_CB callbacks. */
}

@property(nonatomic, assign) int kind;
@property(nonatomic, assign) NSControlStateValue checkBoxState;
@property(nonatomic, assign, getter=isCheckBoxHidden) bool checkBoxHidden;
@property(nonatomic, copy) NSString* title;
@property(nonatomic, weak) IupCocoaTreeItem* parentItem;
@property(nonatomic, assign) BOOL isDeleted;
@property(nonatomic, retain) NSImage* bitmapImage;
@property(nonatomic, retain) NSImage* collapsedImage;
@property(nonatomic, retain) NSColor* textColor;
@property(nonatomic, retain) NSFont* font;
@property(nonatomic, retain) IupCocoaTreeToggleReceiver* toggleReceiver;

- (instancetype) cloneWithNewParentItem:(IupCocoaTreeItem*)new_parent_item ihandle:(Ihandle*)ih;
- (IupCocoaTreeItem*) childAtIndex:(NSUInteger)the_index;

@end

/* Forward declaration needed */
static void cocoaTreeReloadItem(IupCocoaTreeItem* tree_item, NSOutlineView* outline_view);

@implementation IupCocoaTreeItem

@synthesize kind;
@synthesize checkBoxState;
@synthesize checkBoxHidden;
@synthesize title;
@synthesize parentItem;
@synthesize isDeleted;
@synthesize bitmapImage; /* is the expandedImage for branches */
@synthesize collapsedImage;
@synthesize textColor;
@synthesize font;
@synthesize toggleReceiver;

/* Creates, caches, and returns the array of children */
/* Loads children incrementally */
- (NSMutableArray*) childrenArray
{
  return childrenArray;
}

- (void) setChildrenArray:(NSMutableArray*)new_array
{
  if(childrenArray == new_array)
  {
    return;
  }
  [childrenArray release];
  childrenArray = [new_array retain];
}

- (IupCocoaTreeItem*) childAtIndex:(NSUInteger)the_index
{
  return [[self childrenArray] objectAtIndex:the_index];
}

- (NSUInteger) numberOfChildren
{
  return [[self childrenArray] count];
}

- (instancetype) init
{
  self = [super init];
  if(self)
  {
    childrenArray = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void) dealloc
{
  [toggleReceiver release];

  [bitmapImage release];
  [collapsedImage release];
  [textColor release];
  [font release];

  [childrenArray release];
  [title release];
  parentItem = nil; /* weak ref */
  [super dealloc];
}

- (instancetype) cloneWithNewParentItem:(IupCocoaTreeItem*)new_parent_item ihandle:(Ihandle*)ih
{
  IupCocoaTreeItem* new_copy = [[IupCocoaTreeItem alloc] init];
  [new_copy setParentItem:new_parent_item];
  [new_copy setIsDeleted:NO];

  [new_copy setBitmapImage:[self bitmapImage]];
  [new_copy setCollapsedImage:[self collapsedImage]];
  [new_copy setTextColor:[self textColor]];
  [new_copy setFont:[self font]];

  [new_copy setTitle:[self title]]; /* this is a copy property */
  [new_copy setKind:[self kind]];
  [new_copy setCheckBoxState:[self checkBoxState]];
  [new_copy setCheckBoxHidden:[self isCheckBoxHidden]];

  NSMutableArray* new_children_array = [[NSMutableArray alloc] init];
  [new_copy setChildrenArray:new_children_array];
  [new_children_array release];

  for(IupCocoaTreeItem* original_item in childrenArray)
  {
    IupCocoaTreeItem* child_copy = [original_item cloneWithNewParentItem:new_copy ihandle:ih];
    [new_children_array addObject:child_copy];
    [child_copy release];
  }

  IupCocoaTreeToggleReceiver* new_toggle_receiver = [[IupCocoaTreeToggleReceiver alloc] init];
  [new_toggle_receiver setTreeItem:new_copy];
  [new_toggle_receiver setIhandle:ih];
  [new_copy setToggleReceiver:new_toggle_receiver];
  [new_toggle_receiver release];

  return new_copy;
}

@end

/* We need to override NSOutlineView in order to implement things like keyDown for k_any */
@interface IupCocoaOutlineView : NSOutlineView
@property(nonatomic, assign) Ihandle* ih;
@property(nonatomic, retain) NSImage* leafImage;
@property(nonatomic, retain) NSImage* expandedImage;
@property(nonatomic, retain) NSImage* collapsedImage;
@property(nonatomic, retain) IupCocoaTreeItem* markStartNode;
@end

@implementation IupCocoaOutlineView
@synthesize ih = _ih;
@synthesize leafImage;
@synthesize expandedImage;
@synthesize collapsedImage;

- (void) dealloc
{
  [leafImage release];
  [expandedImage release];
  [collapsedImage release];
  [super dealloc];
}

- (NSMenu *)menuForEvent:(NSEvent *)event
{
  /* Check if CONTEXTMENU attribute has been configured by the user */
  if (!iupAttribGet(self.ih, "_IUPCOCOA_CONTEXTMENU_SET"))
  {
    /* Allow default system menu */
    return [super menuForEvent:event];
  }

  /* Retrieve the custom menu */
  Ihandle* menu_ih = (Ihandle*)iupAttribGet(self.ih, "_COCOA_CONTEXT_MENU_IH");
  if (menu_ih && menu_ih->handle)
  {
    return (NSMenu*)menu_ih->handle;
  }

  /* CONTEXTMENU was explicitly set to NULL, disable menu */
  return nil;
}

/* Intercept right-click to handle RIGHTCLICK_CB */
- (void) rightMouseDown:(NSEvent *)event
{
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  NSInteger row = [self rowAtPoint:point];

  if (row >= 0)
  {
    IFni cb = (IFni)IupGetCallback(self.ih, "RIGHTCLICK_CB");
    if (cb)
    {
      IupCocoaTreeItem *item = [self itemAtRow:row];
      int item_id = iupTreeFindNodeId(self.ih, (InodeHandle*)item);
      cb(self.ih, item_id);
      return; /* Don't call super to prevent context menu */
    }
  }

  [super rightMouseDown:event];
}

- (void) mouseDown:(NSEvent *)event
{
  if ([event clickCount] == 2)
  {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    NSInteger row = [self rowAtPoint:point];

    if (row >= 0)
    {
      IupCocoaTreeItem *item = [self itemAtRow:row];
      int kind = [item kind];
      int item_id = iupTreeFindNodeId(self.ih, (InodeHandle*)item);

      if (kind == ITREE_LEAF)
      {
        IFni cb = (IFni)IupGetCallback(self.ih, "EXECUTELEAF_CB");
        if (cb)
        {
          cb(self.ih, item_id);
          return;
        }
      }
      else
      {
        IFni cb = (IFni)IupGetCallback(self.ih, "EXECUTEBRANCH_CB");
        if (cb)
        {
          cb(self.ih, item_id);
          return;
        }
      }
    }
  }

  [super mouseDown:event];
}

- (void) flagsChanged:(NSEvent*)the_event
{
  Ihandle* ih = [self ih];
  unsigned short mac_key_code = [the_event keyCode];
  bool should_not_propagate = iupcocoaModifierEvent(ih, the_event, (int)mac_key_code);
  if(!should_not_propagate)
  {
    [super flagsChanged:the_event];
  }
}

- (void) keyDown:(NSEvent*)the_event
{
  Ihandle* ih = [self ih];

  /* Handle Enter key for EXECUTELEAF/BRANCH callbacks */
  NSString *chars = [the_event characters];
  if ([chars length] > 0)
  {
    unichar aChar = [chars characterAtIndex:0];
    if (aChar == NSEnterCharacter || aChar == NSCarriageReturnCharacter)
    {
      id delegate = [self delegate];
      if ([delegate respondsToSelector:@selector(iupCocoaTreeDoubleClickAction:)])
      {
        [delegate performSelector:@selector(iupCocoaTreeDoubleClickAction:) withObject:self];
        return; /* Consume event */
      }
    }
  }

  int mac_key_code = [the_event keyCode];
  bool should_not_propagate = iupcocoaKeyEvent(ih, the_event, mac_key_code, true);
  if(!should_not_propagate)
  {
    [super keyDown:the_event];
  }
}

- (void) keyUp:(NSEvent*)the_event
{
  Ihandle* ih = [self ih];
  int mac_key_code = [the_event keyCode];
  bool should_not_propagate = iupcocoaKeyEvent(ih, the_event, mac_key_code, false);
  if(!should_not_propagate)
  {
    [super keyUp:the_event];
  }
}

- (void)updateTrackingAreas
{
  [super updateTrackingAreas];

  Ihandle* ih = [self ih];
  if (!ih)
    return;

  if (!IupGetCallback(ih, "TIPS_CB"))
    return;

  for (NSTrackingArea *area in [self trackingAreas]) {
    if ([area owner] == self) {
      [self removeTrackingArea:area];
    }
  }

  NSTrackingAreaOptions options = NSTrackingMouseMoved | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect;
  NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options:options owner:self userInfo:nil];
  [self addTrackingArea:trackingArea];
  [trackingArea release];
}

- (void)mouseMoved:(NSEvent *)event
{
  [super mouseMoved:event];

  Ihandle* ih = [self ih];
  if (!ih)
    return;

  if (!iupAttribGetBoolean(ih, "INFOTIP"))
  {
    IFnii cb = (IFnii)IupGetCallback(ih, "TIPS_CB");
    if (cb)
    {
      NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
      cb(ih, (int)point.x, (int)point.y);
    }
  }
}

- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if (result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
    if (ih)
      iupcocoaFocusIn(ih);
  }
  return result;
}

- (BOOL)resignFirstResponder
{
  BOOL result = [super resignFirstResponder];
  if (result)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
    if (ih)
      iupcocoaFocusOut(ih);
  }
  return result;
}

- (BOOL)acceptsFirstResponder
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    if (iupAttribGet(ih, "_IUPCOCOA_CANFOCUS"))
      return iupAttribGetBoolean(ih, "_IUPCOCOA_CANFOCUS");
    return iupAttribGetBoolean(ih, "CANFOCUS");
  }
  return [super acceptsFirstResponder];
}

@end

@interface IupCocoaTreeTextField : NSTextField <NSTextFieldDelegate, NSTextViewDelegate>
@end

@implementation IupCocoaTreeTextField

- (NSMenu *)textView:(NSTextView *)textView menu:(NSMenu *)menu forEvent:(NSEvent *)event atIndex:(NSUInteger)charIndex
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih)
  {
    IupCocoaOutlineView* outline_view = objc_getAssociatedObject(self, IUP_COCOA_TREE_TEXTFIELD_OWNER_KEY);
    if (outline_view)
    {
      ih = [outline_view ih];
    }
  }

  if (!ih)
  {
    return menu;
  }

  if (!iupAttribGet(ih, "_IUPCOCOA_CONTEXTMENU_SET"))
  {
    return menu;
  }

  Ihandle* menu_ih = (Ihandle*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");
  if (menu_ih && menu_ih->handle)
  {
    return (NSMenu*)menu_ih->handle;
  }

  /* CONTEXTMENU was explicitly set to NULL, disable menu */
  return nil;
}

- (void)controlTextDidEndEditing:(NSNotification *)notification
{
  IupCocoaOutlineView *outlineView = objc_getAssociatedObject(self, IUP_COCOA_TREE_TEXTFIELD_OWNER_KEY);
  if (!outlineView)
    return;

  NSInteger row = [outlineView rowForView:self];
  if (row < 0)
    return;

  Ihandle* ih = [outlineView ih];
  if (!ih)
    return;

  IupCocoaTreeItem* item = [outlineView itemAtRow:row];
  if (!item)
    return;

  NSString* oldTitle = [item title];
  NSString* newTitle = [self stringValue];

  /* No change, nothing to do */
  if ([oldTitle isEqualToString:newTitle])
    return;

  IFnis cb = (IFnis)IupGetCallback(ih, "RENAME_CB");
  if (cb)
  {
    int ret = cb(ih, iupTreeFindNodeId(ih, (InodeHandle*)item), [newTitle UTF8String]);
    if (ret == IUP_IGNORE)
    {
      /* Revert to old title */
      [self setStringValue:oldTitle];
      return;
    }
  }

  /* Update the title */
  [item setTitle:newTitle];
}

- (BOOL)control:(NSControl *)control textShouldBeginEditing:(NSText *)fieldEditor
{
    IupCocoaOutlineView* outline_view = objc_getAssociatedObject(self, IUP_COCOA_TREE_TEXTFIELD_OWNER_KEY);
    if (!outline_view) return YES;

    Ihandle* ih = [outline_view ih];
    if (!ih) return YES;

    NSInteger row = [outline_view rowForView:self];
    if (row < 0) return YES;

    IupCocoaTreeItem* item = [outline_view itemAtRow:row];
    int item_id = iupTreeFindNodeId(ih, (InodeHandle*)item);

    IFni cbShowRename = (IFni)IupGetCallback(ih, "SHOWRENAME_CB");
    if (cbShowRename && cbShowRename(ih, item_id) == IUP_IGNORE)
        return NO;

    return YES;
}

- (void)controlTextDidBeginEditing:(NSNotification *)notification
{
    IupCocoaOutlineView* outline_view = objc_getAssociatedObject(self, IUP_COCOA_TREE_TEXTFIELD_OWNER_KEY);
    if (!outline_view) return;

    Ihandle* ih = [outline_view ih];
    if (!ih) return;

    NSText* fieldEditor = [[notification userInfo] objectForKey:@"NSFieldEditor"];
    if (!fieldEditor) return;

    char* value = iupAttribGetStr(ih, "RENAMECARET");
    if (value)
    {
        int pos = 1;
        if (iupStrToInt(value, &pos))
        {
            if (pos < 1) pos = 1;
            pos--;
            [fieldEditor setSelectedRange:NSMakeRange(pos, 0)];
        }
    }

    value = iupAttribGetStr(ih, "RENAMESELECTION");
    if (value)
    {
        int start = 1, end = 1;
        if (iupStrToIntInt(value, &start, &end, ':') == 2)
        {
            if (start >= 1 && end >= 1)
            {
                start--;
                end--;
                if (end >= start)
                    [fieldEditor setSelectedRange:NSMakeRange(start, end - start)];
            }
        }
    }
}

@end

@interface IupCocoaTreeDelegate : NSObject <NSOutlineViewDataSource, NSOutlineViewDelegate, NSTextFieldDelegate>
{
  NSMutableArray* treeRootTopLevelObjects;
  NSIndexSet* previousSelections;
}
@property(nonatomic, retain) NSMutableArray* treeRootTopLevelObjects;
- (NSUInteger) insertChild:(IupCocoaTreeItem*)tree_item_child withParent:(IupCocoaTreeItem*)tree_item_parent;
- (NSUInteger) insertChild:(IupCocoaTreeItem*)tree_item_child withParent:(IupCocoaTreeItem*)tree_item_parent targetChildIndex:(NSInteger)target_child_index;
- (NSUInteger) insertPeer:(IupCocoaTreeItem*)tree_item_new withSibling:(IupCocoaTreeItem*)tree_item_prev addMode:(int)add;
- (void) insertAtRoot:(IupCocoaTreeItem*)tree_item_new;
- (void) removeAllObjects;
- (NSIndexSet*) removeAllChildrenForItem:(IupCocoaTreeItem*)tree_item;
- (NSUInteger) removeItem:(IupCocoaTreeItem*)tree_item;
- (void) moveItem:(IupCocoaTreeItem*)tree_item targetParent:(IupCocoaTreeItem*)tree_item_parent targetChildIndex:(NSInteger)target_child_index;
- (void) iupCocoaTreeDoubleClickAction:(id)sender;
@end

static NSUInteger Helper_RecursivelyCountItems(IupCocoaTreeItem* the_item)
{
  NSUInteger counter = 1;
  for(IupCocoaTreeItem* a_item in [the_item childrenArray])
  {
    counter += Helper_RecursivelyCountItems(a_item);
  }
  return counter;
}

static bool Helper_RecursivelyCountDepth(IupCocoaTreeItem* current_item, IupCocoaTreeItem* find_item, NSUInteger* depth_counter)
{
  *depth_counter += 1;
  for(IupCocoaTreeItem* a_item in [current_item childrenArray])
  {
    if([a_item isEqual:find_item])
    {
      return true;
    }
    NSUInteger new_depth_counter = *depth_counter;
    bool did_find = Helper_RecursivelyCountDepth(a_item, find_item, &new_depth_counter);
    if(did_find)
    {
      *depth_counter = new_depth_counter;
      return true;
    }
  }
  return false;
}

static bool Helper_CountDepth(IupCocoaTreeDelegate* tree_delegate, IupCocoaTreeItem* find_item, NSUInteger* out_depth_counter)
{
  for(IupCocoaTreeItem* a_item in [tree_delegate treeRootTopLevelObjects])
  {
    if([a_item isEqual:find_item])
    {
      *out_depth_counter = 0;
      return true;
    }
    NSUInteger depth_counter = 0;
    bool did_find = Helper_RecursivelyCountDepth(a_item, find_item, &depth_counter);
    if(did_find)
    {
      *out_depth_counter = depth_counter;
      return true;
    }
  }
  *out_depth_counter = 0;
  return false;
}

static NSInteger Helper_RecursivelyFindFlatIndexofTreeItemInOutlineView(IupCocoaTreeItem* the_item, IupCocoaTreeItem* target_item,NSInteger* out_flat_index)
{
  bool is_found = false;
  for(IupCocoaTreeItem* a_item in [the_item childrenArray])
  {
    if([a_item isEqual:target_item])
    {
      is_found = true;
      break;
    }
    else
    {
      *out_flat_index = *out_flat_index+1;
      is_found = Helper_RecursivelyFindFlatIndexofTreeItemInOutlineView(a_item, target_item, out_flat_index);
    }
    if(is_found)
    {
      break;
    }
  }
  return is_found;
}

/* This is a helper function that traverses through a NSOutlineView data source delegate looking for a certain item, and returns the array index this would be in for the ordering of the Iup node_cache */
static NSInteger Helper_FindFlatIndexofTreeItemInOutlineView(IupCocoaTreeDelegate* tree_delegate, IupCocoaTreeItem* tree_item, NSInteger* out_flat_index)
{
  bool is_found = false;
  for(IupCocoaTreeItem* a_item in [tree_delegate treeRootTopLevelObjects])
  {
    if([a_item isEqual:tree_item])
    {
      is_found = true;
      break;
    }
    else
    {
      *out_flat_index = *out_flat_index+1;
      is_found = Helper_RecursivelyFindFlatIndexofTreeItemInOutlineView(a_item, tree_item, out_flat_index);
    }
    if(is_found)
    {
      break;
    }
  }
  return is_found;
}

@implementation IupCocoaTreeDelegate
@synthesize treeRootTopLevelObjects;

- (instancetype) init
{
  self = [super init];
  if(self)
  {
    treeRootTopLevelObjects = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void) dealloc
{
  [treeRootTopLevelObjects release];
  [previousSelections release];
  [super dealloc];
}

- (NSUInteger) insertChild:(IupCocoaTreeItem*)tree_item_child withParent:(IupCocoaTreeItem*)tree_item_parent
{
  [self insertChild:tree_item_child withParent:tree_item_parent targetChildIndex:0];
  return 0; /* always index 0 since we always insert in the first position */
}

- (NSUInteger) insertChild:(IupCocoaTreeItem*)tree_item_child withParent:(IupCocoaTreeItem*)tree_item_parent targetChildIndex:(NSInteger)target_child_index
{
  [[tree_item_parent childrenArray] insertObject:tree_item_child atIndex:target_child_index];
  [tree_item_child setParentItem:tree_item_parent];
  return target_child_index;
}

- (NSUInteger) insertPeer:(IupCocoaTreeItem*)tree_item_new withSibling:(IupCocoaTreeItem*)tree_item_prev addMode:(int)add
{
  IupCocoaTreeItem* tree_item_parent = [tree_item_prev parentItem];
  if(nil != tree_item_parent)
  {
    [tree_item_new setParentItem:tree_item_parent];
    NSMutableArray* children_array = [tree_item_parent childrenArray];
    NSUInteger prev_index = [children_array indexOfObject:tree_item_prev];

    /* Always insert after the sibling */
    NSUInteger target_index = prev_index + 1;

    if(target_index > [children_array count])
    {
      target_index = [children_array count];
      [children_array addObject:tree_item_new];
    }
    else
    {
      [children_array insertObject:tree_item_new atIndex:target_index];
    }
    return target_index;
  }
  else
  {
    NSUInteger prev_index = [treeRootTopLevelObjects indexOfObject:tree_item_prev];
    NSUInteger target_index;
    if(prev_index != NSNotFound)
    {
      /* Always insert after the sibling */
      target_index = prev_index + 1;
    }
    else
    {
      /* Fallback: append if reference node not found */
      target_index = [treeRootTopLevelObjects count];
    }

    [treeRootTopLevelObjects insertObject:tree_item_new atIndex:target_index];
    return target_index;
  }
}

- (void) insertAtRoot:(IupCocoaTreeItem*)tree_item_new
{
  [treeRootTopLevelObjects addObject:tree_item_new];
}

- (void) removeAllObjects
{
  [treeRootTopLevelObjects removeAllObjects];
}

- (NSIndexSet*) removeAllChildrenForItem:(IupCocoaTreeItem*)tree_item
{
  if(nil == tree_item)
  {
    return nil;
  }

  NSMutableArray* children_array = [tree_item childrenArray];
  NSIndexSet* top_level_children_indexes = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [children_array count])];
  [children_array removeAllObjects];
  return top_level_children_indexes;
}

- (void) removeRecursiveChildItemHelper:(IupCocoaTreeItem*)tree_item
{
  NSMutableArray* children_array = [tree_item childrenArray];
  for(IupCocoaTreeItem* an_item in children_array)
  {
    [self removeRecursiveChildItemHelper:an_item];
  }
  [children_array removeAllObjects];
  [tree_item setIsDeleted:YES];
}

- (NSUInteger) removeItem:(IupCocoaTreeItem*)tree_item
{
  if(nil == tree_item || YES == [tree_item isDeleted])
  {
    return NSNotFound;
  }

  NSMutableArray* children_array = [tree_item childrenArray];
  for(IupCocoaTreeItem* an_item in children_array)
  {
    [self removeRecursiveChildItemHelper:an_item];
  }
  [children_array removeAllObjects];

  IupCocoaTreeItem* tree_item_parent = [tree_item parentItem];
  if(nil != tree_item_parent)
  {
    NSUInteger object_index = [[tree_item_parent childrenArray] indexOfObject:tree_item];
    if(object_index != NSNotFound)
    {
      [[tree_item_parent childrenArray] removeObjectAtIndex:object_index];
    }
    return object_index;
  }
  else
  {
    NSUInteger object_index = [treeRootTopLevelObjects indexOfObject:tree_item];
    if(object_index != NSNotFound)
    {
      [treeRootTopLevelObjects removeObjectAtIndex:object_index];
    }
    return object_index;
  }
}

- (void) moveItem:(IupCocoaTreeItem*)tree_item targetParent:(IupCocoaTreeItem*)target_parent_tree_item targetChildIndex:(NSInteger)target_child_index
{
  IupCocoaTreeItem* parent_tree_item = [tree_item parentItem];

  if(parent_tree_item)
  {
    [[parent_tree_item childrenArray] removeObject:tree_item];
  }
  else
  {
    [treeRootTopLevelObjects removeObject:tree_item];
  }

  if(target_parent_tree_item)
  {
    [[target_parent_tree_item childrenArray] insertObject:tree_item atIndex:target_child_index];
  }
  else
  {
    [treeRootTopLevelObjects insertObject:tree_item atIndex:target_child_index];
  }

  [tree_item setParentItem:target_parent_tree_item];
}

- (NSInteger) outlineView:(NSOutlineView*)outline_view numberOfChildrenOfItem:(nullable id)the_item
{
  if(nil == the_item)
  {
    return [treeRootTopLevelObjects count];
  }
  else
  {
    return [the_item numberOfChildren];
  }
}

- (id) outlineView:(NSOutlineView*)outline_view child:(NSInteger)the_index ofItem:(nullable id)the_item
{
  if(nil == the_item)
  {
    return [treeRootTopLevelObjects objectAtIndex:the_index];
  }
  else
  {
    return [the_item childAtIndex:the_index];
  }
}

- (BOOL) outlineView:(NSOutlineView*)outline_view isItemExpandable:(id)the_item
{
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)the_item;
  NSCAssert([tree_item isKindOfClass:[IupCocoaTreeItem class]], @"Expected IupCocoaTreeItem");

  /* IUP requires explicitly defining nodes as branches, so we can assume all branches */
  /* are expandable, even if they currently have no children. This allows dropping items into an empty branch. */
  if([tree_item kind] == ITREE_BRANCH)
  {
    return YES;
  }
  else
  {
    return NO;
  }
}

static NSImage* helperGetActiveImageForTreeItem(IupCocoaTreeItem* tree_item, IupCocoaOutlineView* outline_view, CGFloat* out_width, CGFloat* out_height)
{
  NSImage* active_image = nil;
  int kind = [tree_item kind];
  if(ITREE_BRANCH == kind)
  {

    if([outline_view isItemExpanded:tree_item])
    {
      active_image = [tree_item bitmapImage];
      if(nil == active_image)
      {
        active_image = [(IupCocoaOutlineView*)outline_view expandedImage];
      }
    }
    else
    {
      active_image = [tree_item collapsedImage];
      if(nil == active_image)
      {
        active_image = [(IupCocoaOutlineView*)outline_view collapsedImage];
      }
    }
  }
  else if(ITREE_LEAF == kind)
  {
    active_image = [tree_item bitmapImage];
    if(nil == active_image)
    {
      active_image = [(IupCocoaOutlineView*)outline_view leafImage];
    }
  }

  NSSize image_size = NSMakeSize(0.0, 0.0);
  if(active_image)
  {
    image_size = [active_image size];
  }

  if(NULL != out_width)
  {
    *out_width = image_size.width;
  }

  if(NULL != out_height)
  {
    *out_height = image_size.height;
  }
  return active_image;
}

/* WARNING: This method needs to be fast for performance. */
- (CGFloat) outlineView:(NSOutlineView*)outline_view heightOfRowByItem:(id)the_item
{
  Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)the_item;
  CGFloat text_height = 17.0; /* Default height */

  CGFloat image_width = 0.0;
  CGFloat image_height = 0.0;
  helperGetActiveImageForTreeItem(tree_item, (IupCocoaOutlineView*)outline_view, &image_width, &image_height);

  /* Use item's custom font if available to calculate text height */
  NSFont *font = [tree_item font];
  if (font)
  {
    text_height = [font capHeight] + 2; /* Approximate height from font */
  }

  CGFloat final_height;
  if(image_height > 0)
  {
    final_height = MAX(text_height, image_height);
  }
  else
  {
    final_height = text_height;
  }

  return final_height + ih->data->spacing;
}

/* WARNING: This is another method that should be fast for performance. */
- (nullable NSView *)outlineView:(NSOutlineView*)outline_view viewForTableColumn:(nullable NSTableColumn*)table_column item:(id)the_item
{
  Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)the_item;
  NSCAssert([tree_item isKindOfClass:[IupCocoaTreeItem class]], @"Expected IupCocoaTreeItem");
  NSString* string_item = [tree_item title];

  NSTableCellView* table_cell_view = nil;
  BOOL is_enabled = [outline_view isEnabled];

  if(ih->data->show_toggle > 0)
  {
    table_cell_view = [outline_view makeViewWithIdentifier:@"IupCocoaTreeToggleTableCellView" owner:self];
    if(nil == table_cell_view)
    {
      table_cell_view = [[[IupCocoaTreeToggleTableCellView alloc] initWithFrame:NSZeroRect] autorelease];
      [table_cell_view setIdentifier:@"IupCocoaTreeToggleTableCellView"];
    }
    IupCocoaTreeToggleTableCellView* toggle_cell_view = (IupCocoaTreeToggleTableCellView*)table_cell_view;

    NSButton* check_box = [toggle_cell_view checkBox];
    if (check_box)
    {
      [check_box setAllowsMixedState:(ih->data->show_toggle == 2)];
      [check_box setState:[tree_item checkBoxState]];
      [check_box setHidden:[tree_item isCheckBoxHidden]];

      IupCocoaTreeToggleReceiver* toggle_receiver = [tree_item toggleReceiver];
      [toggle_receiver setIhandle:ih];
      [check_box setTarget:toggle_receiver];
      [check_box setAction:@selector(myToggleClickAction:)];
      [check_box setEnabled:is_enabled];
    }
  }
  else
  {
    table_cell_view = [outline_view makeViewWithIdentifier:@"IupCocoaTreeTableCellView" owner:self];
    if(nil == table_cell_view)
    {
      table_cell_view = [[[NSTableCellView alloc] initWithFrame:NSZeroRect] autorelease];
      [table_cell_view setIdentifier:@"IupCocoaTreeTableCellView"];

      /* Initialize ImageView */
      NSImageView* image_view = [[NSImageView alloc] initWithFrame:NSZeroRect];
      [image_view setImageScaling:NSImageScaleProportionallyUpOrDown];
      [image_view setTranslatesAutoresizingMaskIntoConstraints:NO];
      [table_cell_view addSubview:image_view];
      [table_cell_view setImageView:image_view];
      [image_view release]; /* Retained by superview and property */

      /* Initialize TextField */
      IupCocoaTreeTextField* text_field = [[IupCocoaTreeTextField alloc] initWithFrame:NSZeroRect];
      [text_field setBezeled:NO];
      [text_field setDrawsBackground:NO];
      [text_field setEditable:NO];
      [text_field setSelectable:YES]; /* Good practice for standard cells */
      [text_field setTranslatesAutoresizingMaskIntoConstraints:NO];

      /* Increase compression resistance to prevent the text field from collapsing when space is tight. */
      [text_field setContentCompressionResistancePriority:NSLayoutPriorityRequired forOrientation:NSLayoutConstraintOrientationHorizontal];

      [table_cell_view addSubview:text_field];
      [table_cell_view setTextField:text_field];
      [text_field release]; /* Retained by superview and property */

      /* Center vertically */
      [NSLayoutConstraint constraintWithItem:image_view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:table_cell_view attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0.0].active = YES;
      [NSLayoutConstraint constraintWithItem:text_field attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:table_cell_view attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0.0].active = YES;

      /* Set a standard size constraint for the image view (e.g., 16x16). */
      [NSLayoutConstraint constraintWithItem:image_view attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:nil attribute:NSLayoutAttributeNotAnAttribute multiplier:1.0 constant:16.0].active = YES;
      [NSLayoutConstraint constraintWithItem:image_view attribute:NSLayoutAttributeHeight relatedBy:NSLayoutRelationEqual toItem:nil attribute:NSLayoutAttributeNotAnAttribute multiplier:1.0 constant:16.0].active = YES;

      /* Horizontal layout: |-[Image]-[Text]-| */
      [NSLayoutConstraint constraintWithItem:image_view attribute:NSLayoutAttributeLeading relatedBy:NSLayoutRelationEqual toItem:table_cell_view attribute:NSLayoutAttributeLeading multiplier:1.0 constant:2.0].active = YES;
      [NSLayoutConstraint constraintWithItem:text_field attribute:NSLayoutAttributeLeading relatedBy:NSLayoutRelationEqual toItem:image_view attribute:NSLayoutAttributeTrailing multiplier:1.0 constant:5.0].active = YES;
      [NSLayoutConstraint constraintWithItem:text_field attribute:NSLayoutAttributeTrailing relatedBy:NSLayoutRelationEqual toItem:table_cell_view attribute:NSLayoutAttributeTrailing multiplier:1.0 constant:-2.0].active = YES;
    }
  }

  IupCocoaTreeTextField* text_field = (IupCocoaTreeTextField*)[table_cell_view textField];
  NSImageView* image_view = [table_cell_view imageView];

  if (text_field)
  {
    objc_setAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
    objc_setAssociatedObject(text_field, IUP_COCOA_TREE_TEXTFIELD_OWNER_KEY, outline_view, OBJC_ASSOCIATION_ASSIGN);

    [text_field setStringValue:string_item];

    [text_field setEditable:(BOOL)ih->data->show_rename];
    if (ih->data->show_rename)
    {
      /* The text field handles its own editing callbacks. */
      [text_field setDelegate:text_field];
    }
    else
    {
      /* Clear delegate when recycling a view if rename is off. */
      [text_field setDelegate:nil];
    }

    /* Apply custom font and color */
    NSFont* item_font = [tree_item font];
    if (!item_font)
    {
      IupCocoaFont* iup_font = iupcocoaGetFont(ih);
      if (!iup_font)
      {
        const char* default_font = IupGetGlobal("DEFAULTFONT");
        iup_font = iupcocoaFindFont(default_font);
      }
      item_font = iup_font ? [iup_font nativeFont] : [NSFont systemFontOfSize:13];
    }
    [text_field setFont:item_font];
    [text_field setTextColor:([tree_item textColor] ?: [NSColor controlTextColor])];
    [text_field setEnabled:is_enabled];
  }

  if (image_view)
  {
    NSImage* active_image = helperGetActiveImageForTreeItem(tree_item, (IupCocoaOutlineView*)outline_view, NULL, NULL);
    /* Hide the image view if the new item doesn't have an image. */
    [image_view setHidden:(nil == active_image)];
    [image_view setImage:active_image];
    [image_view setEnabled:is_enabled];
  }

  return table_cell_view;
}

- (void) outlineViewItemDidExpand:(NSNotification*)the_notification
{
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)[the_notification object];
  NSDictionary* user_info = [the_notification userInfo];
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)[user_info objectForKey:@"NSObject"];

  if(nil == tree_item)
  {
    return;
  }

  NSImage* expanded_image = [tree_item bitmapImage] ?: [outline_view expandedImage];
  NSImage* collapsed_image = [tree_item collapsedImage] ?: [outline_view collapsedImage];

  /* Only reload if the images are different to avoid flicker. */
  if(![expanded_image isEqual:collapsed_image])
  {
    cocoaTreeReloadItem(tree_item, outline_view);
  }
}

- (void) outlineViewItemDidCollapse:(NSNotification*)the_notification
{
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)[the_notification object];
  NSDictionary* user_info = [the_notification userInfo];
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)[user_info objectForKey:@"NSObject"];

  if(nil == tree_item)
  {
    return;
  }

  NSImage* expanded_image = [tree_item bitmapImage] ?: [outline_view expandedImage];
  NSImage* collapsed_image = [tree_item collapsedImage] ?: [outline_view collapsedImage];

  if(![expanded_image isEqual:collapsed_image])
  {
    cocoaTreeReloadItem(tree_item, outline_view);
  }
}

- (void)handleSelectionDidChange:(NSOutlineView*)outline_view
{
  NSCAssert([outline_view isKindOfClass:[IupCocoaOutlineView class]], @"Expected IupCocoaOutlineView");
  Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];

  if (NULL == ih || iupAttribGet(ih, "_IUPTREE_IGNORE_SELECTION_CB"))
    return;

  IFnii single_selection_cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");
  IFnIi multi_selection_cb = (IFnIi)IupGetCallback(ih, "MULTISELECTION_CB");
  IFnIi multi_unselection_cb = (IFnIi)IupGetCallback(ih, "MULTIUNSELECTION_CB");

  if (!single_selection_cb && !multi_selection_cb && !multi_unselection_cb)
    return;

  NSIndexSet* currentSelections = [outline_view selectedRowIndexes];

  if (previousSelections)
  {
    NSMutableIndexSet* unselected_set = [previousSelections mutableCopy];
    [unselected_set removeIndexes:currentSelections];

    if ([unselected_set count] > 0)
    {
      if (multi_unselection_cb)
      {
        NSUInteger count = [unselected_set count];
        int* ids = malloc(sizeof(int) * count);
        if (ids)
        {
          __block int i = 0;
          [unselected_set enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
            if (idx < [outline_view numberOfRows])
            {
              IupCocoaTreeItem *item = [outline_view itemAtRow:idx];
              if (item)
                ids[i++] = iupTreeFindNodeId(ih, (InodeHandle*)item);
            }
          }];

          if (i > 0)
            multi_unselection_cb(ih, ids, i);

          free(ids);
        }
      }
      else if (single_selection_cb)
      {
        [unselected_set enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
          if (idx < [outline_view numberOfRows])
          {
            IupCocoaTreeItem *item = [outline_view itemAtRow:idx];
            if (item)
              single_selection_cb(ih, iupTreeFindNodeId(ih, (InodeHandle*)item), 0);
          }
        }];
      }
    }
    [unselected_set release];
  }

  NSMutableIndexSet* added_selected_set = [currentSelections mutableCopy];
  if (previousSelections)
  {
    [added_selected_set removeIndexes:previousSelections];
  }

  if ([added_selected_set count] > 0)
  {
    if (multi_selection_cb)
    {
      NSUInteger count = [added_selected_set count];
      int* ids = malloc(sizeof(int) * count);
      if (ids)
      {
        __block int i = 0;
        [added_selected_set enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
          if (idx < [outline_view numberOfRows])
          {
            IupCocoaTreeItem *item = [outline_view itemAtRow:idx];
            if (item)
              ids[i++] = iupTreeFindNodeId(ih, (InodeHandle*)item);
          }
        }];

        if (i > 0)
          multi_selection_cb(ih, ids, i);

        free(ids);
      }
    }
    else if (single_selection_cb)
    {
      [added_selected_set enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
        if (idx < [outline_view numberOfRows])
        {
          IupCocoaTreeItem *item = [outline_view itemAtRow:idx];
          if (item)
            single_selection_cb(ih, iupTreeFindNodeId(ih, (InodeHandle*)item), 1);
        }
      }];
    }
  }
  [added_selected_set release];

  [previousSelections release];
  previousSelections = [currentSelections copy];
}

- (void) outlineViewSelectionDidChange:(NSNotification*)the_notification
{
  NSOutlineView* outline_view = [the_notification object];
  [self handleSelectionDidChange:outline_view];
}

- (NSTableRowView *)outlineView:(NSOutlineView *)outlineView rowViewForItem:(id)item
{
  IupCocoaOutlineView* iup_outline_view = (IupCocoaOutlineView*)outlineView;
  IupCocoaTreeRowView* row_view = [outlineView makeViewWithIdentifier:@"IupTreeRow" owner:self];
  if (!row_view) {
    row_view = [[[IupCocoaTreeRowView alloc] init] autorelease];
    row_view.identifier = @"IupTreeRow";
  }
  [row_view setIh:[iup_outline_view ih]];
  return row_view;
}

- (BOOL) outlineView:(NSOutlineView*)outline_view shouldExpandItem:(id)item
{
  IFni cbBranchOpen = (IFni)IupGetCallback([(IupCocoaOutlineView*)outline_view ih], "BRANCHOPEN_CB");
  if (cbBranchOpen)
  {
    IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)item;
    Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];
    int id = iupTreeFindNodeId(ih, (InodeHandle*)tree_item);
    if (cbBranchOpen(ih, id) == IUP_IGNORE)
      return NO;
  }
  return YES;
}

- (BOOL) outlineView:(NSOutlineView*)outline_view shouldCollapseItem:(id)item
{
  IFni cbBranchClose = (IFni)IupGetCallback([(IupCocoaOutlineView*)outline_view ih], "BRANCHCLOSE_CB");
  if (cbBranchClose)
  {
    IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)item;
    Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];
    int id = iupTreeFindNodeId(ih, (InodeHandle*)tree_item);
    if (cbBranchClose(ih, id) == IUP_IGNORE)
      return NO;
  }
  return YES;
}

- (NSIndexSet*) outlineView:(NSOutlineView*)outline_view selectionIndexesForProposedSelection:(NSIndexSet*)proposed_selection_indexes
{
  Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];
  return iupAttribGetBoolean(ih, "CANFOCUS") ? proposed_selection_indexes : nil;
}

- (void) iupCocoaTreeDoubleClickAction:(id)sender
{
  IupCocoaOutlineView *outlineView = (IupCocoaOutlineView*)sender;
  NSInteger clickedRow = [outlineView clickedRow];
  if (clickedRow < 0)
  {
    clickedRow = [outlineView selectedRow];
  }

  if (clickedRow >= 0)
  {
    IupCocoaTreeItem *item = [outlineView itemAtRow:clickedRow];
    Ihandle* ih = [outlineView ih];
    int item_id = iupTreeFindNodeId(ih, (InodeHandle*)item);

    if ([item kind] == ITREE_LEAF)
    {
      IFni cb = (IFni)IupGetCallback(ih, "EXECUTELEAF_CB");
      if (cb) cb(ih, item_id);
    }
    else
    {
      IFni cb = (IFni)IupGetCallback(ih, "EXECUTEBRANCH_CB");
      if (cb) cb(ih, item_id);
    }
  }
}

- (NSString *)outlineView:(NSOutlineView *)outlineView toolTipForCell:(NSCell *)cell rect:(NSRectPointer)rect tableColumn:(NSTableColumn *)tableColumn item:(id)item mouseLocation:(NSPoint)mouseLocation
{
  Ihandle* ih = [outlineView ih];
  if (!ih)
    return nil;

  IFnii cbTips = (IFnii)IupGetCallback(ih, "TIPS_CB");
  if (cbTips)
  {
    /* mouseLocation is already in outline view coordinates */
    int x = (int)mouseLocation.x;
    int y = (int)mouseLocation.y;
    cbTips(ih, x, y);
  }

  /* Check for custom TIP attribute set by TIPS_CB or user */
  char* tip = iupAttribGet(ih, "TIP");
  if (tip)
    return [NSString stringWithUTF8String:tip];

  /* If INFOTIP is not enabled, don't show automatic tooltips */
  if (!iupAttribGetBoolean(ih, "INFOTIP"))
    return nil;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)item;
  NSString* title = [tree_item title];

  if (!title)
    return nil;

  /* Calculate if text is truncated */
  NSTableCellView* cellView = [outlineView viewAtColumn:[outlineView columnWithIdentifier:@"MainColumn"] row:[outlineView rowForItem:item] makeIfNecessary:NO];
  if (!cellView)
    return title;

  NSTextField* textField = [cellView textField];
  if (!textField)
    return title;

  NSSize textSize = [[textField stringValue] sizeWithAttributes:@{NSFontAttributeName: [textField font]}];
  NSRect textRect = [textField frame];

  /* Show tooltip only if text is truncated */
  if (textSize.width > textRect.size.width)
    return title;

  return nil;
}

@end

/*****************************************************************************/
/* DRAG AND DROP                                                             */
/*****************************************************************************/

static void cocoaTreeUpdateDragDrop(Ihandle* ih)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  if (!outline_view) return;

  /* Internal DND (SHOWDRAGDROP=YES) */
  BOOL enable_internal_dnd = ih->data->show_dragdrop;
  /* Cross-tree DND (DRAGDROPTREE=YES) */
  BOOL enable_crosstree_dnd = iupAttribGetBoolean(ih, "DRAGDROPTREE");

  /* DRAGSOURCE: Enabled if internal DND is on OR (cross-tree DND is on AND DRAGSOURCE=YES). */
  BOOL enable_drag_source = enable_internal_dnd || (enable_crosstree_dnd && iupAttribGetBoolean(ih, "DRAGSOURCE"));
  /* DROPTARGET: Enabled if internal DND is on OR (cross-tree DND is on AND DROPTARGET=YES). */
  BOOL enable_drop_target = enable_internal_dnd || (enable_crosstree_dnd && iupAttribGetBoolean(ih, "DROPTARGET"));

  if (enable_drag_source)
  {
    NSDragOperation source_mask = NSDragOperationMove | NSDragOperationCopy;
    /* Enable for local drags (within the application, including between different trees). */
    [outline_view setDraggingSourceOperationMask:source_mask forLocal:YES];

    /* Disable for external drags (to other applications), as we use internal types. */
    [outline_view setDraggingSourceOperationMask:NSDragOperationNone forLocal:NO];

    /* Allow dragging to start. */
    [outline_view setVerticalMotionCanBeginDrag:YES];
  }
  else
  {
    /* Disable drag source entirely. */
    [outline_view setDraggingSourceOperationMask:NSDragOperationNone forLocal:YES];
    [outline_view setDraggingSourceOperationMask:NSDragOperationNone forLocal:NO];
    [outline_view setVerticalMotionCanBeginDrag:NO];
  }

  /* Configure Drop Target */
  /* Register only if the target role is enabled AND DND is active. */
  if (enable_drop_target && (enable_internal_dnd || enable_crosstree_dnd))
  {
    /* Register for the custom IUP tree DND type. */
    [outline_view registerForDraggedTypes:[NSArray arrayWithObjects:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE, nil]];
    /* Use standard feedback style for outline views. */
    [outline_view setDraggingDestinationFeedbackStyle:NSTableViewDraggingDestinationFeedbackStyleRegular];
  }
  else
  {
    /* Disable drop target entirely. */
    [outline_view unregisterDraggedTypes];
  }
}

static int cocoaTreeSetShowDragDropAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    ih->data->show_dragdrop = 1;
  else
    ih->data->show_dragdrop = 0;

  if (ih->handle)
  {
    cocoaTreeUpdateDragDrop(ih);
    return 0;
  }
  return 1;
}

/* Generic handler for DRAGDROPTREE, DRAGSOURCE, DROPTARGET */
static int cocoaTreeSetDndControlAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (ih->handle)
  {
    cocoaTreeUpdateDragDrop(ih);
  }
  return 1;
}

@interface IupCocoaTreeDragDropDelegate : IupCocoaTreeDelegate
{
  /* This is used to compare whether the item being dragged ends up dragging onto itself so we can reject it in validation */
  id itemBeingDragged;
}
@property(nonatomic, weak) id itemBeingDragged;
@end

@implementation IupCocoaTreeDragDropDelegate

@synthesize itemBeingDragged = itemBeingDragged;

static NSData* helperDataWithValue(NSValue* the_value)
{
  NSUInteger size;
  const char* encoding = [the_value objCType];
  NSGetSizeAndAlignment(encoding, &size, NULL);

  void* ptr = malloc(size);
  [the_value getValue:ptr];
  NSData* ret_data = [NSData dataWithBytes:ptr length:size];
  free(ptr);

  return ret_data;
}

- (id <NSPasteboardWriting>)outlineView:(NSOutlineView *)outlineView pasteboardWriterForItem:(id)the_item
{
  if(![the_item isKindOfClass:[IupCocoaTreeItem class]])
  {
    return nil;
  }
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)the_item;

  NSPasteboardItem* paste_board_item = [[NSPasteboardItem alloc] init];
  [paste_board_item autorelease];

  NSValue* pointer_value = [NSValue valueWithPointer:tree_item];
  NSData* data_value = helperDataWithValue(pointer_value);

  [paste_board_item setData:data_value forType:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE];
  return paste_board_item;
}

- (void)outlineView:(NSOutlineView *)outlineView draggingSession:(NSDraggingSession *)session willBeginAtPoint:(NSPoint)screenPoint forItems:(NSArray *)dragged_items
{
  [self setItemBeingDragged:nil];

  /* If only one item is being dragged, mark it so we can reorder it with a special pboard indicator */
  if([dragged_items count] == 1)
  {
    [self setItemBeingDragged:[dragged_items lastObject]];
  }
}

static int helperCallDragDropCb(Ihandle* ih, IupCocoaTreeItem* tree_item_drag, IupCocoaTreeItem* tree_item_drop, NSInteger child_index, bool is_copy)
{
  IFniiii drag_drop_cb = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");
  int is_shift = 0; /* Shift key doesn't have a standard D&D meaning on macOS */

  if(drag_drop_cb)
  {
    int drag_id = iupTreeFindNodeId(ih, (InodeHandle*)tree_item_drag);
    /* The drop_id is the identifier of the node where the item was dropped. */
    /* The actual position (as child or sibling) is determined later. */
    int drop_id = iupTreeFindNodeId(ih, (InodeHandle*)tree_item_drop);
    return drag_drop_cb(ih, drag_id, drop_id, is_shift, (int)is_copy);
  }

  return IUP_CONTINUE; /* allow to move by default if callback not defined */
}

- (NSDragOperation)outlineView:(NSOutlineView *)outline_view validateDrop:(id < NSDraggingInfo >)drag_info proposedItem:(id)target_item proposedChildIndex:(NSInteger)child_index
{
  NSArray<NSPasteboardType>* drag_types = [[drag_info draggingPasteboard] types];
  Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];

  if([drag_types containsObject:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE])
  {
    /* If the sender is ourselves, then we accept it as a move or copy, depending on the modifier key */
    if([drag_info draggingSource] == outline_view)
    {
      /* Since this is callback shared with DRAGDROPTREE, do an extra check to make sure this feature is on. */
      if(!ih->data->show_dragdrop)
      {
        return NSDragOperationNone;
      }

      if([drag_info draggingSourceOperationMask] == NSDragOperationCopy)
      {
        BOOL can_drag = child_index >= 0 && target_item;
        if(can_drag)
        {
          if(IUP_CONTINUE == helperCallDragDropCb(ih, [self itemBeingDragged], target_item, child_index, true))
          {
            return NSDragOperationCopy;
          }
          else
          {
            return NSDragOperationNone;
          }
        }
        else
        {
          return NSDragOperationNone;
        }
      }
      else
      {
        BOOL can_drag = child_index >= 0 && target_item;

        if(can_drag)
        {
          if([self itemBeingDragged])
          {
            /* We have a single item being dragged to move; validate if we can move it or not */
            /* A move is only valid if the target isn't a child of the thing being dragged. We validate that now */
            id item_walker = target_item;
            while(item_walker)
            {
              if(item_walker == [self itemBeingDragged])
              {
                return NSDragOperationNone;
              }
              item_walker = [outline_view parentForItem:item_walker];
            }

            if(IUP_CONTINUE == helperCallDragDropCb(ih, [self itemBeingDragged], target_item, child_index, false))
            {
              return NSDragOperationMove;
            }
            else
            {
              return NSDragOperationNone;
            }
          }
          else
          {
            /* For multiple items, what do we do? */
            return NSDragOperationNone;
          }
        }
        else
        {
          return NSDragOperationNone;
        }
      }
    }
    /* Dragging an item from one NSOutlineView to another NSOutlineView */
    else
    {
      if(0 == IupGetInt(ih, "DRAGDROPTREE"))
      {
        return NSDragOperationNone;
      }


      if([drag_info draggingSourceOperationMask] == NSDragOperationCopy)
      {
        BOOL can_drag = child_index >= 0 && target_item;
        if(can_drag)
        {
          return NSDragOperationCopy;
        }
        else if((-1 == child_index) && (nil == target_item))
        {
          /* special check for dragging onto an empty outlineview */
          IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
          if([[data_source_delegate treeRootTopLevelObjects] count] == 0)
          {
            return NSDragOperationCopy;
          }
          else
          {
            return NSDragOperationNone;
          }
        }
        else
        {
          return NSDragOperationNone;
        }
      }
      else
      {
        BOOL can_drag = child_index >= 0 && target_item;

        if(can_drag)
        {
          return NSDragOperationMove;
        }
        else if((-1 == child_index) && (nil == target_item))
        {
          /* special check for dragging onto an empty outlineview */
          IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
          if([[data_source_delegate treeRootTopLevelObjects] count] == 0)
          {
            return NSDragOperationMove;
          }
          else
          {
            return NSDragOperationNone;
          }
        }
        else
        {
          return NSDragOperationNone;
        }
      }
    }


  }
  return NSDragOperationNone;
}

/*****************************************************************************/
/* DRAGDROPTREE - Cross-tree drag and drop support                          */
/*****************************************************************************/

static int cocoaTreeDropData_CB(Ihandle *ih, char* type, void* data, int len, int x, int y)
{
  int id = IupConvertXYToPos(ih, x, y);
  int is_ctrl = 0;
  char key[5];

  Ihandle* ih_source;
  memcpy((void*)&ih_source, data, len);

  if (!IupClassMatch(ih_source, "tree"))
    return IUP_DEFAULT;

  /* A copy operation is enabled with the CTRL key pressed, or else a move operation will occur.
     A move operation will be possible only if the attribute DRAGSOURCEMOVE is Yes.
     When no key is pressed the default operation is copy when DRAGSOURCEMOVE=No and move when DRAGSOURCEMOVE=Yes. */
  iupdrvGetKeyState(key);
  if (key[1] == 'C')
    is_ctrl = 1;

  /* Here copy/move of multiple selection is not allowed, only a single node and its children. */

  if (ih_source->data->mark_mode == ITREE_MARK_SINGLE)
  {
    int src_id = iupAttribGetInt(ih_source, "_IUPTREE_SOURCEID");
    InodeHandle *itemDst, *itemSrc;

    itemSrc = iupTreeGetNode(ih_source, src_id);
    if (!itemSrc)
      return IUP_DEFAULT;

    itemDst = iupTreeGetNode(ih, id);
    if (!itemDst)
      return IUP_DEFAULT;

    /* Copy the node and its children to the new position */
    iupdrvTreeDragDropCopyNode(ih_source, ih, itemSrc, itemDst);

    if (IupGetInt(ih_source, "DRAGSOURCEMOVE") && !is_ctrl)
      IupSetAttribute(ih_source, "DELNODE0", "MARKED");
  }

  (void)type;
  return IUP_DEFAULT;
}

static int cocoaTreeDragData_CB(Ihandle *ih, char* type, void *data, int len)
{
  int id = iupAttribGetInt(ih, "_IUPTREE_SOURCEID");
  if (id < 0)
    return IUP_DEFAULT;

  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
  {
    /* Single selection */
    IupSetAttributeId(ih, "MARKED", id, "YES");
  }

  /* Copy source handle */
  memcpy(data, (void*)&ih, len);

  (void)type;
  return IUP_DEFAULT;
}

static int cocoaTreeDragDataSize_CB(Ihandle* ih, char* type)
{
  (void)ih;
  (void)type;
  return sizeof(Ihandle*);
}

static int cocoaTreeDragEnd_CB(Ihandle *ih, int del)
{
  iupAttribSetInt(ih, "_IUPTREE_SOURCEID", -1);
  (void)del;
  return IUP_DEFAULT;
}

static int cocoaTreeDragBegin_CB(Ihandle* ih, int x, int y)
{
  int id = IupConvertXYToPos(ih, x, y);
  iupAttribSetInt(ih, "_IUPTREE_SOURCEID", id);
  return IUP_DEFAULT;
}

static int cocoaTreeSetDragDropTreeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
  {
    /* Register callbacks to enable drag and drop between trees */
    IupSetCallback(ih, "DRAGBEGIN_CB",    (Icallback)cocoaTreeDragBegin_CB);
    IupSetCallback(ih, "DRAGDATASIZE_CB", (Icallback)cocoaTreeDragDataSize_CB);
    IupSetCallback(ih, "DRAGDATA_CB",     (Icallback)cocoaTreeDragData_CB);
    IupSetCallback(ih, "DRAGEND_CB",      (Icallback)cocoaTreeDragEnd_CB);
    IupSetCallback(ih, "DROPDATA_CB",     (Icallback)cocoaTreeDropData_CB);
  }
  else
  {
    /* Unregister callbacks */
    IupSetCallback(ih, "DRAGBEGIN_CB",    NULL);
    IupSetCallback(ih, "DRAGDATASIZE_CB", NULL);
    IupSetCallback(ih, "DRAGDATA_CB",     NULL);
    IupSetCallback(ih, "DRAGEND_CB",      NULL);
    IupSetCallback(ih, "DROPDATA_CB",     NULL);
  }

  /* Update DND configuration if already mapped */
  if (ih->handle)
  {
    cocoaTreeUpdateDragDrop(ih);
  }

  return 1;
}

/*****************************************************************************/
/* AUXILIAR FUNCTIONS                                                        */
/*****************************************************************************/

static void iupCocoaTreeRecursivelyCreateFlatItemArray(IupCocoaTreeItem* the_item, NSMutableArray<IupCocoaTreeItem*>* flattened_array_of_items)
{
  for(IupCocoaTreeItem* a_item in [the_item childrenArray])
  {
    [flattened_array_of_items addObject:a_item];
    iupCocoaTreeRecursivelyCreateFlatItemArray(a_item, flattened_array_of_items);
  }
}

static NSArray<IupCocoaTreeItem*>* iupCocoaTreeCreateFlatItemArray(IupCocoaTreeItem* the_item)
{
  NSMutableArray* flattened_array_of_items = [[[NSMutableArray alloc] init] autorelease];
  [flattened_array_of_items addObject:the_item];

  for(IupCocoaTreeItem* a_item in [the_item childrenArray])
  {
    [flattened_array_of_items addObject:a_item];
    iupCocoaTreeRecursivelyCreateFlatItemArray(a_item, flattened_array_of_items);
  }
  return flattened_array_of_items;
}

/* iupTreeCopyMoveCache doesn't work for Cocoa. */
/* The main problem is NSOutlineView allows the user to insert into positions that IUP does not support. */
/* There are ambiguities about whether something is added as a sibling or child. */
/* IUP overspecifies the UI interaction rules which don't apply to NSOutlineView. */
/* And NSOutlineView provides UI that does distinguish between sibling or child and the user has fine control over where it is dropped. */
/* So I think the problem is that we cannot express this in terms of IUP. */
/* Additionally, iupTreeCopyMoveCache seems to imply I copied the nodes, and then will delete. */
/* But Cocoa directly moved, so there is another impedience mismatch. */
/*  */
/* (The bigger problem I think is we shouldn't have a node_cache. */
/* "The two biggest problems in computer science are naming things and cache invalidation." */
/* Because Cocoa already has two representations (the NSOutlineView and the data source delegate), */
/* with a different internal layout than the third IUP node_cache, it is really hard to keep these in sync. */
/* But for now, we are stuck with the node_cache. */
/* So I need to create my own variants of iupTreeCopyMoveCache to properly update the node_cache so it stays in sync with */
/* how Cocoa allows the user to manipulate the NSOutlineView.) */
static void iupCocoaTreeMoveCache(Ihandle* ih, int flat_index_before, int flat_index_after, int count_of_nodes_to_move)
{
  /* I need to rearrange the internal IUP node_cache array.
     For large moves, use heap allocation to avoid stack overflow. */
  bool is_malloc = false;
  InodeData* tmp_array = NULL;

  if(count_of_nodes_to_move > 128)
  {
    is_malloc = true;
    tmp_array = (InodeData*)malloc(count_of_nodes_to_move*sizeof(InodeData));
  }
  else
  {
    is_malloc = false;
    tmp_array = (InodeData*)alloca(count_of_nodes_to_move*sizeof(InodeData));
  }

  /* Algorithm:
   * - If moving from higher to lower address: copy to temp, shift right, copy back
   * - If moving from lower to higher address: copy to temp, shift left, copy back */
  if(flat_index_before > flat_index_after)
  {
    memcpy(tmp_array, ih->data->node_cache+flat_index_before, count_of_nodes_to_move*sizeof(InodeData));

    int number_of_nodes_to_move_over = flat_index_before - flat_index_after;
    memmove(ih->data->node_cache+flat_index_after+count_of_nodes_to_move,
            ih->data->node_cache+flat_index_after,
            number_of_nodes_to_move_over*sizeof(InodeData));
    memcpy(ih->data->node_cache+flat_index_after, tmp_array, count_of_nodes_to_move*sizeof(InodeData));
  }
  else
  {
    memcpy(tmp_array, ih->data->node_cache+flat_index_before, count_of_nodes_to_move*sizeof(InodeData));

    int number_of_nodes_to_move_over = flat_index_after - flat_index_before;
    memmove(ih->data->node_cache+flat_index_before,
            ih->data->node_cache+flat_index_before+count_of_nodes_to_move,
            number_of_nodes_to_move_over*sizeof(InodeData));
    memcpy(ih->data->node_cache+(flat_index_before+number_of_nodes_to_move_over),
            tmp_array, count_of_nodes_to_move*sizeof(InodeData));
  }

  if(is_malloc)
  {
    free(tmp_array);
  }

  iupAttribSet(ih, "LASTADDNODE", NULL);
}

/* iupTreeCopyMoveCache doesn't work for Cocoa. */
/* (See comments in iupCocoaTreeMoveCache for rationale.) */
static void iupCocoaTreeCopyCache(Ihandle* ih, int flat_index_source, int flat_index_target, int count_of_nodes_to_copy, IupCocoaTreeItem* new_copy_tree_item)
{
  /*
Algorithm:
- If we are moving from a higher address to a lower address,
- count the number items between the insert position and end_of_the_array
- Resize the node_cache array
- memmove from the range (insert_position to end_of_array), to the (insert_position+number_of_elements_to_copy)
- memmove from the range (insert_position+number_of_elements_to_copy), to the insert_position
- Else if we are moving from a lower address to a higher address,
- count the number items between the source position and end_of_the_array
- Resize the node_cache array
- memmove from the range (insert_position to end_of_array), to the (insert_position+number_of_elements_to_copy)
- memmove from the range (insert_position), to the insert_position
*/
  int original_node_count = ih->data->node_count;
  /* We must change the node_count and let IUP resize its array as needed. */
  ih->data->node_count += count_of_nodes_to_copy;
  /* iupTreeIncCacheMem expects that the node_count contains the new size */
  iupTreeIncCacheMem(ih);

  if(flat_index_source > flat_index_target) /* copying from a higher address to a lower */
  {
    int number_of_nodes_to_move_over = (original_node_count-1) - flat_index_target + 1; /* we add +1 because we are inserting before the target */
    memmove(ih->data->node_cache+flat_index_target+count_of_nodes_to_copy, ih->data->node_cache+flat_index_target, number_of_nodes_to_move_over*sizeof(InodeData));
    memmove(ih->data->node_cache+flat_index_target, ih->data->node_cache+flat_index_source+count_of_nodes_to_copy, count_of_nodes_to_copy*sizeof(InodeData));
  }
  else /* copying from a lower address to a higher */
  {
    int number_of_nodes_to_move_over = (original_node_count-1) - flat_index_source;
    memmove(ih->data->node_cache+flat_index_target+count_of_nodes_to_copy, ih->data->node_cache+flat_index_target, number_of_nodes_to_move_over*sizeof(InodeData));
    memmove(ih->data->node_cache+flat_index_target, ih->data->node_cache+flat_index_source, count_of_nodes_to_copy*sizeof(InodeData));
  }

  /* The mem-copies above created new nodes in the node_cache for our insertion, but their pointers to the node_handle point to the original objects instead of the new copies of the objects. */
  /* So we need to go through those items, and change their node_handle pointers to point to the new copies of each object. */
  NSArray<IupCocoaTreeItem*>* flattened_array_of_new_items = iupCocoaTreeCreateFlatItemArray(new_copy_tree_item);
  int k=flat_index_target;
  for(IupCocoaTreeItem* a_item in flattened_array_of_new_items)
  {
    ih->data->node_cache[k].node_handle = a_item;
    k++;
  }

  iupAttribSet(ih, "LASTADDNODE", NULL);
}

/* iupTreeCopyMoveCache doesn't work for Cocoa. */
/* (See comments in iupCocoaTreeMoveCache for rationale.) */
static void iupCocoaTreeCrossInsertCache(Ihandle* source_ih, Ihandle* target_ih, int flat_index_source, int flat_index_target, int count_of_nodes_to_copy, IupCocoaTreeItem* new_copy_tree_item)
{
  int original_node_count = target_ih->data->node_count;
  /* We must change the node_count and let IUP resize its array as needed. */
  target_ih->data->node_count += count_of_nodes_to_copy;
  /* iupTreeIncCacheMem expects that the node_count contains the new size */
  iupTreeIncCacheMem(target_ih);

  int number_of_nodes_to_move_over = (original_node_count-1) - flat_index_target + 1; /* we add +1 because we are inserting before the target */

  memmove(target_ih->data->node_cache+flat_index_target+count_of_nodes_to_copy, target_ih->data->node_cache+flat_index_target, number_of_nodes_to_move_over*sizeof(InodeData));
  memcpy(target_ih->data->node_cache+flat_index_target, source_ih->data->node_cache+flat_index_source, count_of_nodes_to_copy*sizeof(InodeData));

  /* The mem-copies above created new nodes in the node_cache for our insertion, but their pointers to the node_handle point to the original objects instead of the new copies of the objects. */
  /* So we need to go through those items, and change their node_handle pointers to point to the new copies of each object. */
  NSArray<IupCocoaTreeItem*>* flattened_array_of_new_items = iupCocoaTreeCreateFlatItemArray(new_copy_tree_item);
  int k=flat_index_target;
  for(IupCocoaTreeItem* a_item in flattened_array_of_new_items)
  {
    target_ih->data->node_cache[k].node_handle = a_item;
    k++;
  }

  iupAttribSet(target_ih, "LASTADDNODE", NULL);
}

static void helperMoveNode(IupCocoaOutlineView* outline_view, IupCocoaTreeItem* tree_item, IupCocoaTreeItem* parent_target_tree_item, NSInteger target_child_index)
{
  if(!tree_item)
  {
    return;
  }
  IupCocoaOutlineView* iup_outline_view = (IupCocoaOutlineView*)outline_view;
  IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
  Ihandle* ih = [iup_outline_view ih];

  int count_of_nodes_to_move = (int)Helper_RecursivelyCountItems(tree_item);
  IupCocoaTreeItem* parent_tree_item  = [tree_item parentItem];

  NSUInteger object_index;
  if(parent_tree_item)
  {
    object_index = [[parent_tree_item childrenArray] indexOfObject:tree_item];

  }
  else
  {
    object_index = [[data_source_delegate treeRootTopLevelObjects] indexOfObject:tree_item];
  }

  NSInteger adjusted_index = target_child_index;

  /* If the node is being moved under the same immediate parent, */
  /* we need to subtract 1 if the current placement of the node is earlier than the target. */
  /* because the node's current placement counts against us and target_child_index is +1 too much. */
  if([parent_tree_item isEqual:parent_target_tree_item])
  {
    if(object_index < target_child_index)
    {
      adjusted_index = target_child_index - 1;
    }
  }
  if(adjusted_index < 0)
  {
    adjusted_index = 0;
  }

  /* Quick exit if the user didn't actually change the location */
  if( (object_index == adjusted_index)
      && [parent_target_tree_item isEqual:parent_tree_item]
    )
  {
    return;
  }

  NSInteger flat_index_before = 0;
  /* This will get an index that is compatible with the node_cache */
  bool is_found = Helper_FindFlatIndexofTreeItemInOutlineView(data_source_delegate, tree_item, &flat_index_before);
  NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");

  /* update the data source */
  [data_source_delegate moveItem:tree_item targetParent:parent_target_tree_item targetChildIndex:adjusted_index];
  [outline_view moveItemAtIndex:object_index inParent:parent_tree_item toIndex:adjusted_index inParent:parent_target_tree_item];

  NSInteger flat_index_after = 0;
  /* This will get an index that is compatible with the node_cache */
  is_found = Helper_FindFlatIndexofTreeItemInOutlineView(data_source_delegate, tree_item, &flat_index_after);
  NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");


  iupCocoaTreeMoveCache(ih, (int)flat_index_before, (int)flat_index_after, count_of_nodes_to_move);
}

static void helperExpandedItemArrayRecursive(IupCocoaTreeItem* original_item, NSOutlineView* original_tree, NSMutableArray<NSNumber*>* expanded_info_array)
{
  if([original_tree isExpandable:original_item])
  {
    /* Note: Cocoa returns NO for expanded state if parent is collapsed, but restores child state when parent is expanded */
    BOOL is_expanded = [original_tree isItemExpanded:original_item];
    [expanded_info_array addObject:[NSNumber numberWithBool:is_expanded]];
  }
  else
  {
    [expanded_info_array addObject:[NSNumber numberWithBool:NO]];
  }

  for(IupCocoaTreeItem* a_item in [original_item childrenArray])
  {
    helperExpandedItemArrayRecursive(a_item, original_tree, expanded_info_array);
  }
}

static NSArray<NSNumber*>* helperExpandedItemArray(IupCocoaTreeItem* original_item, NSOutlineView* original_tree)
{
  NSMutableArray<NSNumber*>* expanded_info_array = [[[NSMutableArray alloc] init] autorelease];
  helperExpandedItemArrayRecursive(original_item, original_tree, expanded_info_array);
  return expanded_info_array;
}

static void helperSyncExpandedItemsRecursive(NSUInteger* in_out_current_index, NSArray<NSNumber*>* expanded_info_array, IupCocoaTreeItem* copy_item, NSOutlineView* copy_tree)
{
  NSNumber* is_expandable = [expanded_info_array objectAtIndex:*in_out_current_index];
  if([copy_tree isExpandable:copy_item])
  {
    if([is_expandable boolValue])
      [copy_tree expandItem:copy_item];
    else
      [copy_tree collapseItem:copy_item];
  }

  *in_out_current_index = *in_out_current_index + 1;

  NSArray* copy_children_array = [copy_item childrenArray];
  for(IupCocoaTreeItem* next_copy_item in copy_children_array)
  {
    helperSyncExpandedItemsRecursive(in_out_current_index, expanded_info_array, next_copy_item, copy_tree);
  }
}

static void helperSyncExpandedItems(NSArray<NSNumber*>* expanded_info_array, IupCocoaTreeItem* copy_item, NSOutlineView* copy_tree)
{
  NSUInteger current_index = 0;
  helperSyncExpandedItemsRecursive(&current_index, expanded_info_array, copy_item, copy_tree);
}

static void helperCopyAndInsertNode(IupCocoaOutlineView* outline_view, IupCocoaTreeItem* tree_item, IupCocoaTreeItem* parent_target_tree_item, NSInteger target_child_index, NSTableViewAnimationOptions copy_insert_animation)
{
  if(!tree_item)
  {
    return;
  }
  IupCocoaOutlineView* iup_outline_view = (IupCocoaOutlineView*)outline_view;
  IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
  Ihandle* ih = [iup_outline_view ih];

  IupCocoaTreeItem* new_copy_tree_item = [tree_item cloneWithNewParentItem:parent_target_tree_item ihandle:ih];

  NSInteger adjusted_index = target_child_index;
  if(adjusted_index < 0)
  {
    adjusted_index = 0;
  }

  /* Save the expanded information for the tree_item so we can later apply it to the copies. */
  /* We need to do this now because if we insert a copy into the tree_item as a child, they will be out-of-sync. */
  NSArray<NSNumber*>* array_of_expanded_info = helperExpandedItemArray(tree_item, outline_view);

  NSInteger flat_index_source = 0;
  /* This will get an index that is compatible with the node_cache */
  bool is_found = Helper_FindFlatIndexofTreeItemInOutlineView(data_source_delegate, tree_item, &flat_index_source);
  NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");

  /* update the data source */
  [data_source_delegate insertChild:new_copy_tree_item withParent:parent_target_tree_item targetChildIndex:adjusted_index];

  /* directly update the outlineview so we don't have to reloadData */
  NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:adjusted_index];
  [outline_view insertItemsAtIndexes:index_set inParent:parent_target_tree_item withAnimation:copy_insert_animation];

  /* Make the new copy match the expanded structure of the original since the Cocoa NSOutlineView is so visual and live-drags. */
  /* Anything else just looks wrong. */
  /* I think IUP has slightly different rules, but I think this is the right thing to do for Cocoa. */
  helperSyncExpandedItems(array_of_expanded_info, new_copy_tree_item, outline_view);


  NSInteger flat_index_target = 0;
  /* This will get an index that is compatible with the node_cache */
  is_found = Helper_FindFlatIndexofTreeItemInOutlineView(data_source_delegate, new_copy_tree_item, &flat_index_target);
  NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");

  int count_of_nodes_to_copy = (int)Helper_RecursivelyCountItems(new_copy_tree_item);

  iupCocoaTreeCopyCache(ih, (int)flat_index_source, (int)flat_index_target, count_of_nodes_to_copy, new_copy_tree_item);

  [new_copy_tree_item release];
}

static void helperCrossCopyNode(NSOutlineView* source_outline_view, IupCocoaTreeItem* source_tree_item, NSOutlineView* target_outline_view, IupCocoaTreeItem* parent_target_tree_item, NSInteger target_child_index, NSTableViewAnimationOptions copy_insert_animation)
{
  if(!source_tree_item)
    return;

  IupCocoaOutlineView* iup_source_outline_view = (IupCocoaOutlineView*)source_outline_view;
  IupCocoaTreeDelegate* source_data_delegate = (IupCocoaTreeDelegate*)[source_outline_view dataSource];
  Ihandle* source_ih = [iup_source_outline_view ih];

  IupCocoaOutlineView* iup_target_outline_view = (IupCocoaOutlineView*)target_outline_view;
  IupCocoaTreeDelegate* target_data_delegate = (IupCocoaTreeDelegate*)[target_outline_view dataSource];
  Ihandle* target_ih = [iup_target_outline_view ih];

  IupCocoaTreeItem* new_copy_tree_item = [source_tree_item cloneWithNewParentItem:parent_target_tree_item ihandle:target_ih];

  /* Save expanded state before inserting to maintain visual consistency */
  NSArray<NSNumber*>* array_of_expanded_info = helperExpandedItemArray(source_tree_item, source_outline_view);

  /* Handle empty target tree special case */
  if((-1 == target_child_index) && (nil == parent_target_tree_item) && (0 == [[target_data_delegate treeRootTopLevelObjects] count]))
  {
    [target_data_delegate insertAtRoot:new_copy_tree_item];
    [iup_target_outline_view insertItemsAtIndexes:[NSIndexSet indexSetWithIndex:0] inParent:nil withAnimation:NSTableViewAnimationEffectGap];
  }
  else
  {
    NSInteger adjusted_index = target_child_index;
    if(adjusted_index < 0)
      adjusted_index = 0;

    [target_data_delegate insertChild:new_copy_tree_item withParent:parent_target_tree_item targetChildIndex:adjusted_index];

    NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:adjusted_index];
    [target_outline_view insertItemsAtIndexes:index_set inParent:parent_target_tree_item withAnimation:copy_insert_animation];
  }

  /* Apply saved expanded state to maintain visual appearance */
  helperSyncExpandedItems(array_of_expanded_info, new_copy_tree_item, target_outline_view);

  NSInteger flat_index_target = 0;
  bool is_found = Helper_FindFlatIndexofTreeItemInOutlineView(target_data_delegate, new_copy_tree_item, &flat_index_target);
  NSCAssert(is_found, @"Internal error: Could not find copied node in outline view");

  NSInteger flat_index_source = 0;
  is_found = Helper_FindFlatIndexofTreeItemInOutlineView(source_data_delegate, source_tree_item, &flat_index_source);
  NSCAssert(is_found, @"Internal error: Could not find source node in outline view");

  int count_of_nodes_to_copy = (int)Helper_RecursivelyCountItems(new_copy_tree_item);

  iupCocoaTreeCrossInsertCache(source_ih, target_ih, (int)flat_index_source, (int)flat_index_target, count_of_nodes_to_copy, new_copy_tree_item);

  [new_copy_tree_item release];
}

static IupCocoaTreeItem* helperRecursiveIsPointerValid(intptr_t look_for_pointer, IupCocoaTreeItem* current_item)
{
  if(look_for_pointer == (intptr_t)current_item)
    return current_item;

  for(IupCocoaTreeItem* a_item in [current_item childrenArray])
  {
    IupCocoaTreeItem* is_found = helperRecursiveIsPointerValid(look_for_pointer, a_item);
    if(is_found)
      return is_found;
  }
  return NULL;
}

static IupCocoaTreeItem* helperIsPointerValid(intptr_t look_for_pointer, IupCocoaTreeDelegate* tree_delegate)
{
  for(IupCocoaTreeItem* tree_item in [tree_delegate treeRootTopLevelObjects])
  {
    IupCocoaTreeItem* is_found = helperRecursiveIsPointerValid(look_for_pointer, tree_item);
    if(is_found)
      return is_found;
  }
  return NULL;
}

/* Need forward declaration */
static void cocoaTreeRemoveNodeData(Ihandle* ih, IupCocoaTreeItem* tree_item, int call_cb);

- (BOOL) outlineView:(NSOutlineView *)outline_view acceptDrop:(id <NSDraggingInfo>)drag_info item:(id)parent_target_tree_item childIndex:(NSInteger)target_child_index
{
  if([drag_info draggingSource] == outline_view)
  {
    /* Same tree drag and drop */
    [self setItemBeingDragged:nil];

    NSPasteboard* paste_board = [drag_info draggingPasteboard];
    NSData* data_value = [paste_board dataForType:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE];
    if(nil == data_value)
    {
      return NO;
    }

    intptr_t decoded_integer;
    [data_value getBytes:&decoded_integer length:sizeof(intptr_t)];

    IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
    IupCocoaTreeItem* tree_item = helperIsPointerValid(decoded_integer, data_source_delegate);
    if(nil == tree_item)
    {
      return NO;
    }

    if(target_child_index == NSOutlineViewDropOnItemIndex)
    {
      target_child_index = 0;
    }

    [outline_view beginUpdates];

    IupCocoaTreeItem* new_tree_item = nil;

    if([drag_info draggingSourceOperationMask] == NSDragOperationCopy)
    {
      helperCopyAndInsertNode((IupCocoaOutlineView*)outline_view, tree_item, parent_target_tree_item, target_child_index, NSTableViewAnimationEffectGap);
      new_tree_item = parent_target_tree_item ? [[parent_target_tree_item childrenArray] objectAtIndex:target_child_index] : [[data_source_delegate treeRootTopLevelObjects] objectAtIndex:target_child_index];
    }
    else
    {
      NSInteger item_row_before = [outline_view rowForItem:tree_item];
      helperMoveNode((IupCocoaOutlineView*)outline_view, tree_item, parent_target_tree_item, target_child_index);
      new_tree_item = tree_item;
    }

    [outline_view endUpdates];

    /* Update selection and focus for the new item */
    if (new_tree_item)
    {
      IupCocoaOutlineView* iup_outline_view = (IupCocoaOutlineView*)outline_view;
      Ihandle* ih = [iup_outline_view ih];
      NSInteger new_row = [outline_view rowForItem:new_tree_item];

      if (new_row >= 0)
      {
        /* Clear all selections */
        iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
        [outline_view deselectAll:nil];

        /* Select the new item */
        NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_row];
        [outline_view selectRowIndexes:index_set byExtendingSelection:NO];

        /* Scroll to make it visible */
        [outline_view scrollRowToVisible:new_row];

        iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
      }
    }

    return YES;
  }
  else
  {
    /* Cross-tree drag and drop */
    NSPasteboard* paste_board = [drag_info draggingPasteboard];
    NSData* data_value = [paste_board dataForType:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE];
    if(nil == data_value)
    {
      return NO;
    }

    intptr_t decoded_integer;
    [data_value getBytes:&decoded_integer length:sizeof(intptr_t)];

    IupCocoaTreeDragDropDelegate* source_data_source_delegate = (IupCocoaTreeDragDropDelegate*)[[drag_info draggingSource] dataSource];

    [self setItemBeingDragged:nil];
    [source_data_source_delegate setItemBeingDragged:nil];

    IupCocoaTreeItem* tree_item = helperIsPointerValid(decoded_integer, source_data_source_delegate);
    if(nil == tree_item)
    {
      return NO;
    }

    NSOutlineView* source_outline_view = (NSOutlineView*)[drag_info draggingSource];
    IupCocoaOutlineView* source_iup_outline_view = (IupCocoaOutlineView*)source_outline_view;
    Ihandle* source_ih = [source_iup_outline_view ih];

    IupCocoaOutlineView* dest_iup_outline_view = (IupCocoaOutlineView*)outline_view;
    Ihandle* dest_ih = [dest_iup_outline_view ih];

    IFniiii cbDragDrop = (IFniiii)IupGetCallback(dest_ih, "DRAGDROP_CB");
    if (cbDragDrop)
    {
      int source_id = iupTreeFindNodeId(source_ih, (InodeHandle*)tree_item);
      int dest_id = parent_target_tree_item ? iupTreeFindNodeId(dest_ih, (InodeHandle*)parent_target_tree_item) : 0;
      int is_shift = 0;
      int is_ctrl = ([drag_info draggingSourceOperationMask] == NSDragOperationCopy) ? 1 : 0;

      if (cbDragDrop(dest_ih, source_id, dest_id, is_shift, is_ctrl) != IUP_CONTINUE)
      {
        return NO;
      }
    }

    BOOL is_copy = ([drag_info draggingSourceOperationMask] == NSDragOperationCopy);
    IupCocoaTreeItem* new_tree_item = nil;

    [outline_view beginUpdates];
    helperCrossCopyNode(source_outline_view, tree_item, outline_view, parent_target_tree_item, target_child_index, NSTableViewAnimationEffectGap);
    [outline_view endUpdates];

    /* Get the newly created item */
    IupCocoaTreeDelegate* dest_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
    if ((-1 == target_child_index) && (nil == parent_target_tree_item) && (0 < [[dest_delegate treeRootTopLevelObjects] count]))
    {
      new_tree_item = [[dest_delegate treeRootTopLevelObjects] objectAtIndex:0];
    }
    else
    {
      NSInteger adjusted_index = target_child_index;
      if (adjusted_index < 0)
        adjusted_index = 0;

      new_tree_item = parent_target_tree_item ? [[parent_target_tree_item childrenArray] objectAtIndex:adjusted_index] : [[dest_delegate treeRootTopLevelObjects] objectAtIndex:adjusted_index];
    }

    /* Update selection and focus for the new item in destination tree */
    if (new_tree_item)
    {
      NSInteger new_row = [outline_view rowForItem:new_tree_item];

      if (new_row >= 0)
      {
        iupAttribSet(dest_ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
        [outline_view deselectAll:nil];

        NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_row];
        [outline_view selectRowIndexes:index_set byExtendingSelection:NO];

        [outline_view scrollRowToVisible:new_row];

        iupAttribSet(dest_ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
      }
    }

    if (!is_copy)
    {
      /* Remove node from source tree for move operation */
      [source_iup_outline_view beginUpdates];

      cocoaTreeRemoveNodeData(source_ih, tree_item, 1);
      NSUInteger target_index = [source_data_source_delegate removeItem:tree_item];
      NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:target_index];
      [source_iup_outline_view removeItemsAtIndexes:index_set inParent:[tree_item parentItem] withAnimation:NSTableViewAnimationEffectGap];

      [source_iup_outline_view endUpdates];
    }

    return YES;
  }
}

@end /* IupCocoaTreeDragDropDelegate */

/*****************************************************************************/
/* ADDING ITEMS                                                              */
/*****************************************************************************/

/* This replicates the functionality of the internal static iTreeAddToCache from iup_tree.c, */
/* but allows specifying the exact insertion ID. */
static void cocoaTreeInsertInCache(Ihandle* ih, int id, InodeHandle* node_handle)
{
  /* iupTreeIncCacheMem must have been called before, and node_count incremented. */

  /* Ensure id is valid within the new count. */
  if (id < 0 || id >= ih->data->node_count)
  {
    /* Should not happen if called correctly. */
    return;
  }

  if (id < ih->data->node_count - 1) /* If not adding at the very end */
  {
    /* open space for the new id */
    /* Calculate the number of existing elements that need to be moved. */
    /* Old count was ih->data->node_count - 1. */
    /* We move elements from index 'id' up to 'old_count - 1'. */
    int remain_count = (ih->data->node_count - 1) - id;

    if (remain_count > 0)
    {
      /* Shift existing elements to the right by one position. */
      memmove(ih->data->node_cache+id+1, ih->data->node_cache+id, remain_count*sizeof(InodeData));
    }
  }

  /* Insert the new node handle and clear userdata. */
  ih->data->node_cache[id].node_handle = node_handle;
  ih->data->node_cache[id].userdata = NULL;
}

void iupdrvTreeAddNode(Ihandle* ih, int prev_id, int kind, const char* title, int add)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  InodeHandle* inode_prev = iupTreeGetNode(ih, prev_id);

  if (!inode_prev && prev_id != -1)
    return;

  if (!title)
    title = "";

  IupCocoaTreeItem* tree_item_new = [[IupCocoaTreeItem alloc] init];
  [tree_item_new setKind:kind];
  NSString* ns_title = [NSString stringWithUTF8String:title];
  [tree_item_new setTitle:ns_title];

  IupCocoaTreeToggleReceiver* toggle_receiver = [[IupCocoaTreeToggleReceiver alloc] init];
  [toggle_receiver setIhandle:ih];
  [toggle_receiver setTreeItem:tree_item_new];
  [tree_item_new setToggleReceiver:toggle_receiver];
  [toggle_receiver release];

  InodeHandle* inode_new = (InodeHandle*)tree_item_new;

  [NSAnimationContext beginGrouping];
  [[NSAnimationContext currentContext] setDuration:0.0];
  [outline_view beginUpdates];

  if(inode_prev)
  {
    IupCocoaTreeItem* tree_item_prev = inode_prev;
    int kind_prev = [tree_item_prev kind];

    if((ITREE_BRANCH == kind_prev) && (1 == add))
    {
      NSUInteger child_count_before = [tree_item_prev numberOfChildren];

      NSUInteger new_index = [data_source_delegate insertChild:tree_item_new withParent:tree_item_prev];

      NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_index];
      [outline_view insertItemsAtIndexes:index_set inParent:tree_item_prev withAnimation:NSTableViewAnimationEffectNone];

      iupTreeAddToCache(ih, add, kind_prev, inode_prev, inode_new);

      if (child_count_before == 0)
      {
        iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", "1");
        if (ih->data->add_expanded)
          [outline_view expandItem:tree_item_prev];
        else
          [outline_view collapseItem:tree_item_prev];
        iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", NULL);
      }
    }
    else
    {
      NSUInteger new_index = [data_source_delegate insertPeer:tree_item_new withSibling:tree_item_prev addMode:add];
      IupCocoaTreeItem* tree_item_parent = [tree_item_prev parentItem];

      NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_index];
      [outline_view insertItemsAtIndexes:index_set inParent:tree_item_parent withAnimation:NSTableViewAnimationEffectNone];

      iupTreeAddToCache(ih, add, kind_prev, inode_prev, inode_new);
    }
  }
  else
  {
    NSUInteger new_index = [[data_source_delegate treeRootTopLevelObjects] count];
    [data_source_delegate insertAtRoot:tree_item_new];

    NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_index];
    [outline_view insertItemsAtIndexes:index_set inParent:nil withAnimation:NSTableViewAnimationEffectNone];

    iupTreeAddToCache(ih, 0, 0, NULL, inode_new);

    if (ih->data->node_count == 1)
    {
        iupAttribSet(ih, "_IUPTREE_MARKSTART_NODE", (char*)inode_new);

        iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

        NSInteger row = [outline_view rowForItem:inode_new];
        if (row >= 0)
        {
            NSIndexSet* row_index_set = [NSIndexSet indexSetWithIndex:row];
            [outline_view selectRowIndexes:row_index_set byExtendingSelection:NO];

            if (ih->data->mark_mode != ITREE_MARK_SINGLE)
            {
                [outline_view deselectRow:row];
            }
        }

        iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    }
  }

  [outline_view endUpdates];
  [NSAnimationContext endGrouping];

  [CATransaction flush];
  [outline_view layoutSubtreeIfNeeded];

  [tree_item_new release];
}

/*****************************************************************************/
/* AUXILIAR FUNCTIONS                                                        */
/*****************************************************************************/

int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)node_handle;
  NSUInteger number_of_items = Helper_RecursivelyCountItems(tree_item);
  /* subtract one because we don't include 'this' node as part of the children count */
  if(number_of_items > 0)
  {
    number_of_items = number_of_items - 1;
  }
  return (int)number_of_items;
}

InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

  id selected_item = [outline_view itemAtRow:[outline_view selectedRow]];

  return (InodeHandle*)selected_item;
}

void iupdrvTreeUpdateMarkMode(Ihandle *ih)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

  if(ih->data->mark_mode == ITREE_MARK_MULTIPLE)
  {
    [outline_view setAllowsMultipleSelection:YES];
  }
  else
  {
    [outline_view setAllowsMultipleSelection:NO];
  }
}

void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle* item_src, InodeHandle* item_dst)
{
  /* This function copies a node from one tree to another.
     The native Cocoa implementation handles this differently through
     the drag and drop delegate methods, so we need to bridge the concepts. */

  if (!src || !dst || !item_src || !item_dst)
    return;

  IupCocoaTreeItem* source_item = (IupCocoaTreeItem*)item_src;
  IupCocoaTreeItem* dest_item = (IupCocoaTreeItem*)item_dst;

  NSOutlineView* dst_outline_view = cocoaTreeGetOutlineView(dst);
  IupCocoaTreeDelegate* dst_delegate = (IupCocoaTreeDelegate*)[dst_outline_view dataSource];

  /* Clone the source item with the destination ihandle */
  IupCocoaTreeItem* new_copy = [source_item cloneWithNewParentItem:nil ihandle:dst];

  /* Determine where to insert based on the destination item type */
  int dest_kind = [dest_item kind];
  IupCocoaTreeItem* parent_item = nil;
  NSInteger insert_index = 0;

  if (dest_kind == ITREE_BRANCH)
  {
    /* Check if branch is expanded */
    if ([dst_outline_view isItemExpanded:dest_item])
    {
      /* Insert as first child of expanded branch */
      parent_item = dest_item;
      insert_index = 0;
    }
    else
    {
      /* Insert as sibling after the collapsed branch */
      parent_item = [dest_item parentItem];
      if (parent_item)
      {
        NSArray* children = [parent_item childrenArray];
        insert_index = [children indexOfObject:dest_item] + 1;
      }
      else
      {
        /* Destination is a root-level collapsed branch */
        NSArray* roots = [dst_delegate treeRootTopLevelObjects];
        insert_index = [roots indexOfObject:dest_item] + 1;
      }
    }
  }
  else
  {
    /* Destination is a leaf - insert as sibling after it */
    parent_item = [dest_item parentItem];
    if (parent_item)
    {
      NSArray* children = [parent_item childrenArray];
      insert_index = [children indexOfObject:dest_item] + 1;
    }
    else
    {
      /* Destination is a root-level leaf */
      NSArray* roots = [dst_delegate treeRootTopLevelObjects];
      insert_index = [roots indexOfObject:dest_item] + 1;
    }
  }

  /* Insert the node */
  if (parent_item)
  {
    [dst_delegate insertChild:new_copy withParent:parent_item targetChildIndex:insert_index];
  }
  else
  {
    /* Insert at root level */
    [[dst_delegate treeRootTopLevelObjects] insertObject:new_copy atIndex:insert_index];
  }

  /* Update the view */
  [dst_outline_view reloadData];

  /* Update the cache */
  int id_dst = iupTreeFindNodeId(dst, dest_item);
  int id_new = id_dst + 1;
  int count = (int)Helper_RecursivelyCountItems(new_copy);
  iupTreeCopyMoveCache(dst, id_dst, id_new, count, 1);

  [new_copy release];
}

static void cocoaTreeReloadItem(IupCocoaTreeItem* tree_item, NSOutlineView* outline_view)
{
  if (!tree_item || !outline_view)
  {
    return;
  }

  /* Find the row index for the given item. */
  /* If the item is not visible (e.g., its parent is collapsed), this will be a negative number. */
  NSInteger row = [outline_view rowForItem:tree_item];

  /* Only proceed if the row is valid and visible. */
  if (row >= 0)
  {
    /* Create an index set for the specific row we want to reload. */
    NSIndexSet* row_index_set = [NSIndexSet indexSetWithIndex:row];

    /* Create an index set for the column. Our tree only has one column at index 0. */
    NSIndexSet* column_index_set = [NSIndexSet indexSetWithIndex:0];

    /* Tell the outline view to reload the data for the specific cell, */
    /* which will cause it to be redrawn with any updated properties. */
    [outline_view reloadDataForRowIndexes:row_index_set columnIndexes:column_index_set];
  }
}

static void cocoaTreeRemoveNodeDataRec(Ihandle* ih, IupCocoaTreeItem* tree_item, IFns cb, int* object_id)
{
  int node_id = *object_id;

  /* Check whether we have child items */
  /* remove from children first */
  for(IupCocoaTreeItem* a_item in [tree_item childrenArray])
  {
    (*object_id)++;
    cocoaTreeRemoveNodeDataRec(ih, a_item, cb, object_id);
  }

  /* actually do it for the node */
  cb(ih, (char*)ih->data->node_cache[node_id].userdata);

  /* update count */
  ih->data->node_count--;
}

static void cocoaTreeRemoveNodeData(Ihandle* ih, IupCocoaTreeItem* tree_item, int call_cb)
{
  IFns cb = call_cb? (IFns)IupGetCallback(ih, "NODEREMOVED_CB"): NULL;
  int old_count = ih->data->node_count;
  int object_id = iupTreeFindNodeId(ih, (InodeHandle*)tree_item);
  int start_id = object_id;

  if (cb)
    cocoaTreeRemoveNodeDataRec(ih, tree_item, cb, &object_id);
  else
  {
    int removed_count = iupdrvTreeTotalChildCount(ih, (InodeHandle*)tree_item) + 1;
    ih->data->node_count -= removed_count;
  }

  iupTreeDelFromCache(ih, start_id, old_count - ih->data->node_count);
}

static void cocoaTreeRemoveAllNodeData(Ihandle* ih, int call_cb)
{
  IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
  int i, old_count = ih->data->node_count;

  if (cb)
  {
    for (i = 0; i < ih->data->node_count; i++)
    {
      cb(ih, (char*)ih->data->node_cache[i].userdata);
    }
  }

  ih->data->node_count = 0;

  iupTreeDelFromCache(ih, 0, old_count);
}

/*****************************************************************************/
/* MANIPULATING IMAGES                                                       */
/*****************************************************************************/

static NSImage* helperGetImage(Ihandle* ih, int node_id, const char* value, IupCocoaTreeItem* tree_item)
{
  if(!tree_item)
  {
    return nil;
  }

  NSImage* bitmap_image = nil;

  if(iupStrEqualNoCase("IMGEMPTY", value))
  {
    bitmap_image = nil;
  }
  else
  {
    bitmap_image = (NSImage*)iupImageGetImage(value, ih, 0, NULL);
  }

  return bitmap_image;
}

static int cocoaTreeSetImageExpandedAttrib(Ihandle* ih, int node_id, const char* value)
{
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)iupTreeGetNode(ih, node_id);
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

  NSImage* bitmap_image = helperGetImage(ih, node_id, value, tree_item);

  [tree_item setBitmapImage:bitmap_image];
  cocoaTreeReloadItem(tree_item, outline_view);

  return 1;
}

static int cocoaTreeSetImageAttrib(Ihandle* ih, int node_id, const char* value)
{
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)iupTreeGetNode(ih, node_id);
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

  NSImage* bitmap_image = helperGetImage(ih, node_id, value, tree_item);

  if([tree_item kind] == ITREE_LEAF)
  {
    [tree_item setBitmapImage:bitmap_image];
  }
  else
  {
    [tree_item setCollapsedImage:bitmap_image];
  }
  cocoaTreeReloadItem(tree_item, outline_view);

  return 0;
}

static void helperSetImageBranchExpanded(IupCocoaOutlineView* outline_view, IupCocoaTreeItem* tree_item, NSImage* ns_image)
{
  for(IupCocoaTreeItem* a_item in [tree_item childrenArray])
  {
    helperSetImageBranchExpanded(outline_view, a_item, ns_image);
  }
  if([tree_item kind] == ITREE_BRANCH)
  {
    /* only need to reload if the user hasn't overridden with a custom-per-node image. */
    if(![tree_item bitmapImage])
    {
      cocoaTreeReloadItem(tree_item, outline_view);
    }
  }
}

static int cocoaTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  NSImage* ns_image = (NSImage*)iupImageGetImage(value, ih, 0, NULL);
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);

  [outline_view beginUpdates];

  [outline_view setExpandedImage:ns_image];

  /* Update all images */
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
  for(IupCocoaTreeItem* tree_item in [tree_delegate treeRootTopLevelObjects])
  {
    helperSetImageBranchExpanded(outline_view, tree_item, ns_image);
  }

  [outline_view endUpdates];

  return 1;
}

static void helperSetImageBranchCollapsed(IupCocoaOutlineView* outline_view, IupCocoaTreeItem* tree_item, NSImage* ns_image)
{
  for(IupCocoaTreeItem* a_item in [tree_item childrenArray])
  {
    helperSetImageBranchCollapsed(outline_view, a_item, ns_image);
  }
  if([tree_item kind] == ITREE_BRANCH)
  {
    /* only need to reload if the user hasn't overridden with a custom-per-node image. */
    if(![tree_item collapsedImage])
    {
      cocoaTreeReloadItem(tree_item, outline_view);
    }
  }
}

static int cocoaTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  NSImage* ns_image = (NSImage*)iupImageGetImage(value, ih, 0, NULL);
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);

  [outline_view beginUpdates];

  [outline_view setCollapsedImage:ns_image];

  /* Update all images */
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
  for(IupCocoaTreeItem* tree_item in [tree_delegate treeRootTopLevelObjects])
  {
    helperSetImageBranchCollapsed(outline_view, tree_item, ns_image);
  }

  [outline_view endUpdates];

  return 1;
}

static void helperSetImageLeaf(IupCocoaOutlineView* outline_view, IupCocoaTreeItem* tree_item, NSImage* ns_image)
{
  for(IupCocoaTreeItem* a_item in [tree_item childrenArray])
  {
    helperSetImageLeaf(outline_view, a_item, ns_image);
  }
  if([tree_item kind] == ITREE_LEAF)
  {
    /* only need to reload if the user hasn't overridden with a custom-per-node image. */
    if(![tree_item bitmapImage])
    {
      cocoaTreeReloadItem(tree_item, outline_view);
    }
  }
}

static int cocoaTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  NSImage* ns_image = (NSImage*)iupImageGetImage(value, ih, 0, NULL);
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);

  [outline_view beginUpdates];

  [outline_view setLeafImage:ns_image];

  /* Update all images */
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
  for(IupCocoaTreeItem* tree_item in [tree_delegate treeRootTopLevelObjects])
  {
    helperSetImageLeaf(outline_view, tree_item, ns_image);
  }

  [outline_view endUpdates];

  return 1;
}

/*****************************************************************************/
/* GET AND SET ATTRIBUTES                                                    */
/*****************************************************************************/

static char* cocoaTreeGetStateAttrib(Ihandle* ih, int item_id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;

  if ([outline_view isExpandable:tree_item])
  {
    BOOL is_expanded = [outline_view isItemExpanded:tree_item];
    if (is_expanded)
      return "EXPANDED";
    else
      return "COLLAPSED";
  }
  else
    return NULL;
}

static int cocoaTreeSetStateAttrib(Ihandle* ih, int id, const char* value)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, id);
  if (NULL == inode_handle)
    return 0;

  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;

  if ([outline_view isExpandable:tree_item])
  {
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:0.0];

    iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", "1");
    if (iupStrEqualNoCase(value, "EXPANDED"))
      [outline_view expandItem:tree_item];
    else
      [outline_view collapseItem:tree_item];
    iupAttribSet(ih, "_IUPTREE_IGNORE_BRANCH_CB", NULL);

    [NSAnimationContext endGrouping];

    [CATransaction flush];
    [outline_view layoutSubtreeIfNeeded];
  }

  return 0;
}

static char* cocoaTreeGetDepthAttrib(Ihandle* ih, int item_id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  NSUInteger out_depth_counter = 0;
  bool did_find = Helper_CountDepth(tree_delegate, tree_item, &out_depth_counter);

  if (did_find)
    return iupStrReturnInt((int)out_depth_counter);
  else
    return NULL;
}

static char* cocoaTreeGetKindAttrib(Ihandle* ih, int item_id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;

  if ([tree_item kind] == ITREE_LEAF)
    return "LEAF";
  else
    return "BRANCH";
}

static char* cocoaTreeGetParentAttrib(Ihandle* ih, int item_id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  IupCocoaTreeItem* parent_item = [tree_item parentItem];

  if (parent_item == nil)
    return NULL;

  int parent_id = iupTreeFindNodeId(ih, (InodeHandle*)parent_item);
  return iupStrReturnInt(parent_id);
}

static char* cocoaTreeGetNextAttrib(Ihandle* ih, int id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  IupCocoaTreeItem* parent_item = [tree_item parentItem];
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  NSArray* siblings_array;
  if (parent_item)
    siblings_array = [parent_item childrenArray];
  else
    siblings_array = [tree_delegate treeRootTopLevelObjects];

  NSUInteger index = [siblings_array indexOfObject:tree_item];
  if (index != NSNotFound && index + 1 < [siblings_array count])
  {
    IupCocoaTreeItem* next_item = [siblings_array objectAtIndex:index + 1];
    int next_id = iupTreeFindNodeId(ih, (InodeHandle*)next_item);
    return iupStrReturnInt(next_id);
  }

  return NULL;
}

static char* cocoaTreeGetPreviousAttrib(Ihandle* ih, int id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  IupCocoaTreeItem* parent_item = [tree_item parentItem];
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  NSArray* siblings_array;
  if (parent_item)
    siblings_array = [parent_item childrenArray];
  else
    siblings_array = [tree_delegate treeRootTopLevelObjects];

  NSUInteger index = [siblings_array indexOfObject:tree_item];
  if (index != NSNotFound && index > 0)
  {
    IupCocoaTreeItem* previous_item = [siblings_array objectAtIndex:index - 1];
    int previous_id = iupTreeFindNodeId(ih, (InodeHandle*)previous_item);
    return iupStrReturnInt(previous_id);
  }

  return NULL;
}

static char* cocoaTreeGetFirstAttrib(Ihandle* ih, int id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  IupCocoaTreeItem* parent_item = [tree_item parentItem];
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  NSArray* siblings_array;
  if (parent_item)
    siblings_array = [parent_item childrenArray];
  else
    siblings_array = [tree_delegate treeRootTopLevelObjects];

  if ([siblings_array count] > 0)
  {
    IupCocoaTreeItem* first_item = [siblings_array objectAtIndex:0];
    int first_id = iupTreeFindNodeId(ih, (InodeHandle*)first_item);
    return iupStrReturnInt(first_id);
  }

  return NULL;
}

static char* cocoaTreeGetLastAttrib(Ihandle* ih, int id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  IupCocoaTreeItem* parent_item = [tree_item parentItem];
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  NSArray* siblings_array;
  if (parent_item)
    siblings_array = [parent_item childrenArray];
  else
    siblings_array = [tree_delegate treeRootTopLevelObjects];

  NSUInteger count = [siblings_array count];
  if (count > 0)
  {
    IupCocoaTreeItem* last_item = [siblings_array objectAtIndex:count - 1];
    int last_id = iupTreeFindNodeId(ih, (InodeHandle*)last_item);
    return iupStrReturnInt(last_id);
  }

  return NULL;
}

static char* cocoaTreeGetTitleAttrib(Ihandle* ih, int item_id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (inode_handle)
  {
    IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
    NSCAssert([tree_item isKindOfClass:[IupCocoaTreeItem class]], @"expecting class IupCocoaTreeItem");
    NSString* ns_title = [tree_item title];
    return iupStrReturnStr([ns_title UTF8String]);
  }
  else
    return NULL;
}

static int cocoaTreeSetTitleAttrib(Ihandle* ih, int item_id, const char* value)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (inode_handle)
  {
    NSString* ns_title = @"";
    if (value)
      ns_title = [NSString stringWithUTF8String:value];

    IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
    NSCAssert([tree_item isKindOfClass:[IupCocoaTreeItem class]], @"expecting class IupCocoaTreeItem");
    [tree_item setTitle:ns_title];

    NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
    cocoaTreeReloadItem(tree_item, outline_view);
  }

  return 0;
}

static char* cocoaTreeGetRootCountAttrib(Ihandle* ih)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  NSArray* root_items = [tree_delegate treeRootTopLevelObjects];
  return iupStrReturnInt((int)[root_items count]);
}

static char* cocoaTreeGetChildCountAttrib(Ihandle* ih, int item_id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;

  return iupStrReturnInt((int)[[tree_item childrenArray] count]);
}

static char* cocoaTreeGetColorAttrib(Ihandle* ih, int id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  NSColor* color = [tree_item textColor];

  if (color == nil)
    return NULL;

  CGFloat r, g, b, a;
  [color getRed:&r green:&g blue:&b alpha:&a];

  return iupStrReturnStrf("%d %d %d", (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
}

static int cocoaTreeSetColorAttrib(Ihandle* ih, int id, const char* value)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, id);
  if (NULL == inode_handle)
    return 0;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

  unsigned char r, g, b;
  if (iupStrToRGB(value, &r, &g, &b))
  {
    NSColor* color = [NSColor colorWithRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
    [tree_item setTextColor:color];
    cocoaTreeReloadItem(tree_item, outline_view);
  }

  return 0;
}

static char* cocoaTreeGetTitleFontAttrib(Ihandle* ih, int id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  NSFont* font = [tree_item font];

  if (font == nil)
    return NULL;

  return iupStrReturnStr([[font fontName] UTF8String]);
}

static int cocoaTreeSetTitleFontAttrib(Ihandle* ih, int id, const char* value)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, id);
  if (NULL == inode_handle)
    return 0;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

  if (value)
  {
    IupCocoaFont* iup_font = iupcocoaFindFont(value);
    if (nil != iup_font)
    {
      [tree_item setFont:[iup_font nativeFont]];
      cocoaTreeReloadItem(tree_item, outline_view);
    }
  }
  else
  {
    /* Setting to nil reverts to the default font handled by viewForTableColumn. */
    [tree_item setFont:nil];
    cocoaTreeReloadItem(tree_item, outline_view);
  }

  return 0;
}

static char* cocoaTreeGetMarkedAttrib(Ihandle* ih, int item_id)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);

  if (NULL == inode_handle)
  {
    IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
    NSIndexSet* selected_index = [outline_view selectedRowIndexes];
    NSUInteger selected_i = [selected_index firstIndex];

    if (selected_i != NSNotFound)
      return iupStrReturnBoolean(true);
    else
      return iupStrReturnBoolean(false);
  }

  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;

  NSIndexSet* selected_index = [outline_view selectedRowIndexes];
  NSUInteger selected_i = [selected_index firstIndex];

  while (selected_i != NSNotFound)
  {
    IupCocoaTreeItem* selected_item = [outline_view itemAtRow:selected_i];
    if ([selected_item isEqual:tree_item])
      return iupStrReturnBoolean(true);

    selected_i = [selected_index indexGreaterThanIndex:selected_i];
  }

  return iupStrReturnBoolean(false);
}

static int cocoaTreeSetMarkedAttrib(Ihandle* ih, int item_id, const char* value)
{
  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  bool should_select = iupStrBoolean(value);

  if (NULL == inode_handle)
  {
    if (should_select)
      return 0;

    IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
    [outline_view deselectAll:nil];
    return 0;
  }

  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  NSInteger item_row = [outline_view rowForItem:tree_item];

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
  if (should_select)
  {
    NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:item_row];
    [outline_view selectRowIndexes:index_set byExtendingSelection:NO];
  }
  else
    [outline_view deselectRow:item_row];

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

  return 0;
}

static int cocoaTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->mark_mode == ITREE_MARK_SINGLE)
    return 0;

  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);

  if (iupStrEqualNoCase(value, "BLOCK"))
  {
    IupCocoaTreeItem* mark_start_node = (IupCocoaTreeItem*)iupAttribGet(ih, "_IUPTREE_MARKSTART_NODE");
    if (nil == mark_start_node)
      return 0;

    NSInteger mark_start_node_row = [outline_view rowForItem:mark_start_node];
    if (mark_start_node_row < 0)
      return 0;

    NSIndexSet* selected_index = [outline_view selectedRowIndexes];
    NSUInteger selected_i = [selected_index firstIndex];
    if (selected_i == NSNotFound)
      return 0;

    if (selected_i == mark_start_node_row)
      return 1;

    NSUInteger start_row;
    NSUInteger number_of_rows;

    if (mark_start_node_row > selected_i)
    {
      start_row = selected_i;
      number_of_rows = mark_start_node_row - selected_i + 1;
    }
    else
    {
      start_row = mark_start_node_row;
      number_of_rows = selected_i - mark_start_node_row + 1;
    }

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    NSIndexSet* index_set = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(start_row, number_of_rows)];
    [outline_view selectRowIndexes:index_set byExtendingSelection:NO];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "CLEARALL"))
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    [outline_view deselectAll:nil];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "MARKALL"))
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    [outline_view selectAll:nil];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "INVERTALL"))
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    NSIndexSet* selected_index = [outline_view selectedRowIndexes];
    [outline_view selectAll:nil];
    NSUInteger selected_i = [selected_index firstIndex];

    while (selected_i != NSNotFound)
    {
      [outline_view deselectRow:selected_i];
      selected_i = [selected_index indexGreaterThanIndex:selected_i];
    }

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualPartial(value, "INVERT"))
  {
    const char* id_string = &value[strlen("INVERT")];
    InodeHandle* inode_handle = iupTreeGetNodeFromString(ih, id_string);

    if (NULL == inode_handle)
    {
      NSIndexSet* selected_index = [outline_view selectedRowIndexes];
      NSUInteger selected_i = [selected_index firstIndex];

      while (selected_i != NSNotFound)
      {
        [outline_view deselectRow:selected_i];
        selected_i = [selected_index indexGreaterThanIndex:selected_i];
      }
    }
    else
    {
      IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
      NSInteger item_row = [outline_view rowForItem:tree_item];
      bool was_item_selected = false;
      NSIndexSet* selected_index = [outline_view selectedRowIndexes];
      NSUInteger selected_i = [selected_index firstIndex];

      while (selected_i != NSNotFound)
      {
        IupCocoaTreeItem* selected_item = [outline_view itemAtRow:selected_i];
        if ([selected_item isEqual:tree_item])
        {
          was_item_selected = true;
          break;
        }
        selected_i = [selected_index indexGreaterThanIndex:selected_i];
      }

      iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

      if (was_item_selected)
        [outline_view deselectRow:item_row];
      else
      {
        NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:item_row];
        [outline_view selectRowIndexes:index_set byExtendingSelection:NO];
      }

      iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
    }
  }
  else
  {
    char str1[50], str2[50];

    if (iupStrToStrStr(value, str1, str2, '-') != 2)
      return 0;

    InodeHandle* inode_handle1 = iupTreeGetNodeFromString(ih, str1);
    InodeHandle* inode_handle2 = iupTreeGetNodeFromString(ih, str2);
    NSInteger item_row1 = [outline_view rowForItem:inode_handle1];
    NSInteger item_row2 = [outline_view rowForItem:inode_handle2];

    NSUInteger start_row;
    NSUInteger number_of_rows;

    if (item_row2 > item_row1)
    {
      start_row = item_row1;
      number_of_rows = item_row2 - item_row1 + 1;
    }
    else
    {
      start_row = item_row2;
      number_of_rows = item_row1 - item_row2 + 1;
    }

    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    NSIndexSet* index_set = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(start_row, number_of_rows)];
    [outline_view selectRowIndexes:index_set byExtendingSelection:NO];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }

  return 1;
}

static int cocoaTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  InodeHandle* inode_handle = iupTreeGetNodeFromString(ih, value);
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);

  if (NULL == inode_handle)
  {
    [outline_view setMarkStartNode:nil];
    return 0;
  }

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  [outline_view setMarkStartNode:tree_item];
  iupAttribSet(ih, "_IUPTREE_MARKSTART_NODE", (char*)tree_item);

  return 1;
}

static char* cocoaTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  char* str = iupStrGetMemory(ih->data->node_count + 1);
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  NSIndexSet* selected_index = [outline_view selectedRowIndexes];

  for (size_t i = 0; i < ih->data->node_count; i++)
  {
    IupCocoaTreeItem* iter_node = (IupCocoaTreeItem*)ih->data->node_cache[i].node_handle;
    NSUInteger selected_i = [selected_index firstIndex];
    bool is_found = false;

    while (selected_i != NSNotFound)
    {
      IupCocoaTreeItem* selected_item = (IupCocoaTreeItem*)[outline_view itemAtRow:selected_i];
      if ([iter_node isEqual:selected_item])
      {
        is_found = true;
        break;
      }
      selected_i = [selected_index indexGreaterThanIndex:selected_i];
    }

    if (is_found)
      str[i] = '+';
    else
      str[i] = '-';
  }

  str[ih->data->node_count] = 0;
  return str;
}

static int cocoaTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  int count, i;

  if (ih->data->mark_mode == ITREE_MARK_SINGLE || !value)
    return 0;

  count = (int)strlen(value);
  if (count > ih->data->node_count)
    count = ih->data->node_count;

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");

  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  [outline_view deselectAll:nil];

  for (i = 0; i < count; i++)
  {
    if (value[i] == '+')
    {
      IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)ih->data->node_cache[i].node_handle;
      NSInteger item_row = [outline_view rowForItem:tree_item];
      NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:item_row];
      [outline_view selectRowIndexes:index_set byExtendingSelection:YES];
    }
  }

  iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

  return 0;
}

static int cocoaTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  InodeHandle* inode_handle = iupTreeGetNodeFromString(ih, value);

  if (nil == inode_handle)
    return 0;

  NSMutableArray<IupCocoaTreeItem*>* parent_node_array = [[NSMutableArray alloc] initWithCapacity:ih->data->node_count];
  {
    IupCocoaTreeItem* parent_tree_item = [(IupCocoaTreeItem*)inode_handle parentItem];
    while (parent_tree_item != nil)
    {
      [parent_node_array insertObject:parent_tree_item atIndex:0];
      parent_tree_item = [parent_tree_item parentItem];
    }
  }

  for (IupCocoaTreeItem* a_item in parent_node_array)
    [outline_view expandItem:a_item];

  [parent_node_array release];

  NSInteger item_row = [outline_view rowForItem:inode_handle];
  if (item_row < 0)
    return 0;

  [outline_view scrollRowToVisible:item_row];

  return 0;
}

static int cocoaTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  NSUInteger number_of_visible_rows = [outline_view numberOfRows];

  if (0 == number_of_visible_rows)
    return 0;

  if (cocoaTreeSetMarkAttrib(ih, value))
    return 0;

  NSUInteger new_selected_row;

  if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
  {
    new_selected_row = 0;
    [outline_view scrollRowToVisible:0];
    NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:0];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    [outline_view selectRowIndexes:index_set byExtendingSelection:NO];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "LAST"))
  {
    new_selected_row = number_of_visible_rows - 1;
    [outline_view scrollRowToVisible:new_selected_row];
    NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_selected_row];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    [outline_view selectRowIndexes:index_set byExtendingSelection:NO];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "PGUP"))
  {
    NSIndexSet* selected_index = [outline_view selectedRowIndexes];
    NSUInteger selected_i = [selected_index firstIndex];
    if (selected_i == NSNotFound)
      return 0;

    if (selected_i < 10)
      new_selected_row = 0;
    else
      new_selected_row = selected_i - 10;

    [outline_view scrollRowToVisible:new_selected_row];
    NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_selected_row];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    [outline_view selectRowIndexes:index_set byExtendingSelection:NO];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "PGDN"))
  {
    NSIndexSet* selected_index = [outline_view selectedRowIndexes];
    NSUInteger selected_i = [selected_index firstIndex];
    if (selected_i == NSNotFound)
      return 0;

    if ((selected_i + 10) > (number_of_visible_rows - 1))
      new_selected_row = number_of_visible_rows - 1;
    else
      new_selected_row = selected_i + 10;

    [outline_view scrollRowToVisible:new_selected_row];
    NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_selected_row];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    [outline_view selectRowIndexes:index_set byExtendingSelection:NO];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "NEXT"))
  {
    NSIndexSet* selected_index = [outline_view selectedRowIndexes];
    NSUInteger selected_i = [selected_index firstIndex];
    if (selected_i == NSNotFound)
      return 0;

    if ((selected_i + 1) > (number_of_visible_rows - 1))
      new_selected_row = number_of_visible_rows - 1;
    else
      new_selected_row = selected_i + 1;

    [outline_view scrollRowToVisible:new_selected_row];
    NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_selected_row];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    [outline_view selectRowIndexes:index_set byExtendingSelection:NO];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "PREVIOUS"))
  {
    NSIndexSet* selected_index = [outline_view selectedRowIndexes];
    NSUInteger selected_i = [selected_index firstIndex];
    if (selected_i == NSNotFound)
      return 0;

    if (selected_i < 1)
      new_selected_row = 0;
    else
      new_selected_row = selected_i - 1;

    [outline_view scrollRowToVisible:new_selected_row];
    NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_selected_row];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    [outline_view selectRowIndexes:index_set byExtendingSelection:NO];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }
  else if (iupStrEqualNoCase(value, "CLEAR"))
  {
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
    [outline_view deselectAll:nil];
    iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
  }

  return 0;
}

static void cocoaTreeCallNodeRemovedAll(Ihandle* ih)
{
  IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
  int old_count = ih->data->node_count;

  if (cb)
  {
    int i;
    for (i = 0; i < ih->data->node_count; i++)
    {
      cb(ih, (char*)ih->data->node_cache[i].userdata);
    }
  }

  ih->data->node_count = 0;
  iupTreeDelFromCache(ih, 0, old_count);
}

static void cocoaTreeCallNodeRemovedRec(Ihandle* ih, IupCocoaTreeItem* tree_item, IFns cb, int* id)
{
  int node_id = *id;

  /* Check whether we have child items */
  /* remove from children first */
  for(IupCocoaTreeItem* a_item in [tree_item childrenArray])
  {
    (*id)++;
    cocoaTreeCallNodeRemovedRec(ih, a_item, cb, id);
  }

  /* actually do it for the node */
  if (cb)
    cb(ih, (char*)ih->data->node_cache[node_id].userdata);

  /* update count */
  ih->data->node_count--;
}

static void cocoaTreeCallNodeRemoved(Ihandle* ih, IupCocoaTreeDelegate* tree_delegate, IupCocoaTreeItem* tree_item)
{
  IFns cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
  int old_count = ih->data->node_count;
  int id = iupTreeFindNodeId(ih, (InodeHandle*)tree_item);
  int start_id = id;

  if (cb)
    cocoaTreeCallNodeRemovedRec(ih, tree_item, cb, &id);
  else
  {
    int removed_count = Helper_RecursivelyCountItems(tree_item);
    ih->data->node_count -= removed_count;
  }

  iupTreeDelFromCache(ih, start_id, old_count - ih->data->node_count);
  (void)tree_delegate;
}

/* Load replacement default images if needed */
/* This is primarily for consistent look across platforms */
static void helperReplaceDefaultImages(Ihandle* ih, IupCocoaOutlineView* outline_view)
{
  NSImage* leaf_image = nil;
  char* leaf_img_name = iupAttribGet(ih, "IMAGELEAF");
  if(leaf_img_name && !iupStrEqualNoCase(leaf_img_name, "IMGLEAF"))
    leaf_image = (NSImage*)iupImageGetImage(leaf_img_name, ih, 0, NULL);
  else
  {
    leaf_image = [NSImage imageNamed:NSImageNameIChatTheaterTemplate];
    [leaf_image setSize:NSMakeSize(16, 16)];
  }
  [outline_view setLeafImage:leaf_image];

  NSImage* collapsed_image = nil;
  char* collapsed_img_name = iupAttribGet(ih, "IMAGEBRANCHCOLLAPSED");
  if(collapsed_img_name && !iupStrEqualNoCase(collapsed_img_name, "IMGCOLLAPSED"))
    collapsed_image = (NSImage*)iupImageGetImage(collapsed_img_name, ih, 0, NULL);
  else
  {
    collapsed_image = [NSImage imageNamed:NSImageNameFolder];
    [collapsed_image setSize:NSMakeSize(16, 16)];
  }
  [outline_view setCollapsedImage:collapsed_image];

  NSImage* expanded_image = nil;
  char* expanded_img_name = iupAttribGet(ih, "IMAGEBRANCHEXPANDED");
  if(expanded_img_name && !iupStrEqualNoCase(expanded_img_name, "IMGEXPANDED"))
    expanded_image = (NSImage*)iupImageGetImage(expanded_img_name, ih, 0, NULL);
  else
  {
    expanded_image = [NSImage imageNamed:NSImageNameFolder]; /* No public "Open Folder" icon, use the same for consistency */
    [expanded_image setSize:NSMakeSize(16, 16)];
  }
  [outline_view setExpandedImage:expanded_image];
}

static char* cocoaTreeGetValueAttrib(Ihandle* ih)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  NSIndexSet* selected_index = [outline_view selectedRowIndexes];
  NSUInteger selected_i = [selected_index firstIndex];

  if (selected_i != NSNotFound)
  {
    InodeHandle* selected_item = (InodeHandle*)[outline_view itemAtRow:selected_i];
    int object_id = iupTreeFindNodeId(ih, selected_item);
    return iupStrReturnInt(object_id);
  }
  else
  {
    if (ih->data->node_count > 0)
      return "0";
    else
      return "-1";
  }
}

static char* cocoaTreeGetToggleValueAttrib(Ihandle* ih, int item_id)
{
  if (ih->data->show_toggle < 1)
    return NULL;

  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  NSControlStateValue check_box_state = [tree_item checkBoxState];
  return iupStrReturnChecked((int)check_box_state);
}

static int cocoaTreeSetToggleValueAttrib(Ihandle* ih, int item_id, const char* value)
{
  if (ih->data->show_toggle < 1)
    return 0;

  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (NULL == inode_handle)
    return 0;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  NSControlStateValue check_box_state;

  if ((ih->data->show_toggle == 2) && iupStrEqualNoCase(value, "NOTDEF"))
    check_box_state = NSControlStateValueMixed;
  else if (iupStrEqualNoCase(value, "ON"))
    check_box_state = NSControlStateValueOn;
  else if (iupStrEqualNoCase(value, "OFF"))
    check_box_state = NSControlStateValueOff;
  else
    return 0;

  [tree_item setCheckBoxState:check_box_state];

  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  cocoaTreeReloadItem(tree_item, outline_view);

  return 0;
}

static char* cocoaTreeGetToggleVisibleAttrib(Ihandle* ih, int item_id)
{
  if (ih->data->show_toggle < 1)
    return NULL;

  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (NULL == inode_handle)
    return NULL;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  bool check_box_hidden = [tree_item isCheckBoxHidden];
  return iupStrReturnBoolean((int)!check_box_hidden);
}

static int cocoaTreeSetToggleVisibleAttrib(Ihandle* ih, int item_id, const char* value)
{
  if (ih->data->show_toggle < 1)
    return 0;

  InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
  if (NULL == inode_handle)
    return 0;

  IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
  bool check_box_hidden = !iupStrBoolean(value);

  [tree_item setCheckBoxHidden:check_box_hidden];

  /* Reload the item to reflect the change in visibility. */
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  cocoaTreeReloadItem(tree_item, outline_view);

  return 0;
}

static int cocoaTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)
    return 0;

  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  if (iupStrEqualNoCase(value, "ALL"))
  {
    cocoaTreeCallNodeRemovedAll(ih);
    [tree_delegate removeAllObjects];
    [outline_view reloadData];
    return 0;
  }

  if (iupStrEqualNoCase(value, "SELECTED"))
  {
    InodeHandle* inode_handle = iupTreeGetNode(ih, id);
    if (!inode_handle)
      return 0;

    IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
    cocoaTreeCallNodeRemoved(ih, tree_delegate, tree_item);
    [tree_delegate removeItem:tree_item];
    [outline_view reloadData];
  }
  else if (iupStrEqualNoCase(value, "CHILDREN"))
  {
    InodeHandle* inode_handle = iupTreeGetNode(ih, id);
    if (!inode_handle)
      return 0;

    IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
    NSIndexSet* removed_indexes = [tree_delegate removeAllChildrenForItem:tree_item];

    if (removed_indexes)
    {
      [outline_view removeItemsAtIndexes:removed_indexes inParent:tree_item withAnimation:NSTableViewAnimationEffectFade];

      NSMutableArray* children_array = [tree_item childrenArray];
      for (IupCocoaTreeItem* child_item in children_array)
      {
        cocoaTreeCallNodeRemoved(ih, tree_delegate, child_item);
      }
    }
  }
  else if (iupStrEqualNoCase(value, "MARKED"))
  {
    NSIndexSet* selected_indexes = [outline_view selectedRowIndexes];
    NSMutableArray* items_to_remove = [NSMutableArray array];

    [selected_indexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
IupCocoaTreeItem* item = [outline_view itemAtRow:idx];
if (item)
  [items_to_remove addObject:item];
    }];

    for (IupCocoaTreeItem* item in items_to_remove)
    {
      cocoaTreeCallNodeRemoved(ih, tree_delegate, item);
      [tree_delegate removeItem:item];
    }

    [outline_view reloadData];
  }

  return 0;
}

static int cocoaTreeSetMoveNodeAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)
    return 0;

  InodeHandle* src_handle = iupTreeGetNode(ih, id);
  if (!src_handle)
    return 0;

  InodeHandle* dst_handle = iupTreeGetNodeFromString(ih, value);
  if (!dst_handle)
    return 0;

  IupCocoaTreeItem* src_item = (IupCocoaTreeItem*)src_handle;
  IupCocoaTreeItem* dst_item = (IupCocoaTreeItem*)dst_handle;

  IupCocoaTreeItem* parent = dst_item;
  while (parent != nil)
  {
    if (parent == src_item)
      return 0;
    parent = [parent parentItem];
  }

  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  IupCocoaTreeItem* dst_parent = [dst_item parentItem];
  NSInteger dst_index = 0;

  if ([dst_item kind] == ITREE_BRANCH && [outline_view isItemExpanded:dst_item])
  {
    dst_parent = dst_item;
    dst_index = 0;
  }
  else if (dst_parent)
  {
    NSArray* siblings = [dst_parent childrenArray];
    dst_index = [siblings indexOfObject:dst_item] + 1;
  }
  else
  {
    NSArray* roots = [tree_delegate treeRootTopLevelObjects];
    dst_index = [roots indexOfObject:dst_item] + 1;
  }

  helperMoveNode((IupCocoaOutlineView*)outline_view, src_item, dst_parent, dst_index);

  return 0;
}

static int cocoaTreeSetCopyNodeAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle)
    return 0;

  InodeHandle* src_handle = iupTreeGetNode(ih, id);
  if (!src_handle)
    return 0;

  InodeHandle* dst_handle = iupTreeGetNodeFromString(ih, value);
  if (!dst_handle)
    return 0;

  IupCocoaTreeItem* src_item = (IupCocoaTreeItem*)src_handle;
  IupCocoaTreeItem* dst_item = (IupCocoaTreeItem*)dst_handle;

  IupCocoaTreeItem* parent = dst_item;
  while (parent != nil)
  {
    if (parent == src_item)
      return 0;
    parent = [parent parentItem];
  }

  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  IupCocoaTreeItem* dst_parent = [dst_item parentItem];
  NSInteger dst_index = 0;

  if ([dst_item kind] == ITREE_BRANCH && [outline_view isItemExpanded:dst_item])
  {
    dst_parent = dst_item;
    dst_index = 0;
  }
  else if (dst_parent)
  {
    NSArray* siblings = [dst_parent childrenArray];
    dst_index = [siblings indexOfObject:dst_item] + 1;
  }
  else
  {
    NSArray* roots = [tree_delegate treeRootTopLevelObjects];
    dst_index = [roots indexOfObject:dst_item] + 1;
  }

  helperCopyAndInsertNode((IupCocoaOutlineView*)outline_view, src_item, dst_parent, dst_index, NSTableViewAnimationEffectGap);

  return 0;
}

static char* cocoaTreeGetIndentationAttrib(Ihandle* ih)
{
  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  CGFloat indent = [outline_view indentationPerLevel];
  return iupStrReturnInt((int)indent);
}

static int cocoaTreeSetIndentationAttrib(Ihandle* ih, const char* value)
{
  int indent;
  if (iupStrToInt(value, &indent))
  {
    IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
    [outline_view setIndentationPerLevel:(CGFloat)indent];
  }

  return 0;
}

static int cocoaTreeSetSpacingAttrib(Ihandle* ih, const char* value)
{
  iupStrToInt(value, &ih->data->spacing);

  if (ih->handle)
  {
    NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
    [outline_view reloadData];
    return 0;
  }
  else
    return 1;
}

static int cocoaTreeSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  NSColor* color = [NSColor colorWithRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

  for (IupCocoaTreeItem* tree_item in [tree_delegate treeRootTopLevelObjects])
  {
    [tree_item setTextColor:color];
  }

  [outline_view reloadData];

  return 1;
}

static int cocoaTreeSetHlColorAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    /* Redraw rows to apply the new highlight color */
    NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
    [outline_view setNeedsDisplay:YES];
  }
  (void)value;
  return 1; /* Mark as stored */
}

static int cocoaTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  NSColor* bgcolor = [NSColor colorWithRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

  [outline_view setBackgroundColor:bgcolor];
  [outline_view setNeedsDisplay:YES];

  return 1;
}

static int cocoaTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  bool should_expand = iupStrBoolean(value);

  if (should_expand)
    [outline_view expandItem:nil expandChildren:YES];
  else
    [outline_view collapseItem:nil collapseChildren:YES];

  return 0;
}

static int cocoaTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
  /* This is an ACTION attribute to start editing */
  if (ih->handle && ih->data->show_rename)
  {
    NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
    NSInteger selected_row = [outline_view selectedRow];

    if (selected_row >= 0)
    {
      /* The view needs to be the first responder to begin editing */
      [[outline_view window] makeFirstResponder:outline_view];
      [outline_view editColumn:0 row:selected_row withEvent:nil select:YES];
    }
  }
  (void)value; /* value is not used for this action */
  return 0;
}

static int cocoaTreeSetShowRenameAttrib(Ihandle* ih, const char* value)
{
  bool show_rename = iupStrBoolean(value);
  ih->data->show_rename = show_rename;

  if (ih->handle)
  {
    NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
    [outline_view reloadData];
  }

  return 0;
}

static int cocoaTreeSetContextMenuAttrib(Ihandle* ih, const char* value)
{
  Ihandle* menu_ih = (Ihandle*)value;
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  iupcocoaCommonBaseSetContextMenuForWidget(ih, outline_view, menu_ih);

  /* Record that the user explicitly set this attribute, even if to NULL. */
  iupAttribSet(ih, "_IUPCOCOA_CONTEXTMENU_SET", "1");

  return 1;
}

static char* cocoaTreeGetLayerBackedAttrib(Ihandle* ih)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  BOOL layer_state = [outline_view wantsLayer];
  return iupStrReturnInt(layer_state);
}

static int cocoaTreeSetLayerBackedAttrib(Ihandle* ih, const char* value)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  bool layer_state = iupStrBoolean(value);
  [outline_view setWantsLayer:layer_state];
  [cocoaTreeGetScrollView(ih) setWantsLayer:layer_state];

  return 1;
}

static char* cocoaTreeGetActiveAttrib(Ihandle *ih)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  BOOL was_enabled = [outline_view isEnabled];
  return iupStrReturnBoolean(was_enabled);
}

static int cocoaTreeSetActiveAttrib(Ihandle* ih, const char* value)
{
  bool enable = iupStrBoolean(value);
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

  BOOL was_enabled = [outline_view isEnabled];
  if (was_enabled == enable)
    return 0;

  [outline_view setEnabled:enable];
  [outline_view reloadData];

  return 0;
}

static char* cocoaTreeGetScrollVisibleAttrib(Ihandle* ih)
{
  NSScrollView* scroll_view = cocoaTreeGetScrollView(ih);
  NSScroller* scroller_h = [scroll_view horizontalScroller];
  NSScroller* scroller_v = [scroll_view verticalScroller];

  int scroll_visible = 0;

  if (scroller_h && ![scroller_h isHidden])
    scroll_visible |= 1;
  if (scroller_v && ![scroller_v isHidden])
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

static int cocoaTreeSetTipAttrib(Ihandle* ih, const char* value)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

  if (value)
    [outline_view setToolTip:[NSString stringWithUTF8String:value]];
  else
    [outline_view setToolTip:nil];

  return 1;
}

static char* cocoaTreeGetTipAttrib(Ihandle* ih)
{
  NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
  NSString* tip = [outline_view toolTip];

  if (tip)
    return iupStrReturnStr([tip UTF8String]);

  return NULL;
}

static int cocoaTreeSetHideLinesAttrib(Ihandle* ih, const char* value)
{
  /* NSOutlineView does not draw connecting lines between nodes like GTK TreeView. */
  (void)ih;
  (void)value;
  return 0;
}

static int cocoaTreeSetHideButtonsAttrib(Ihandle* ih, const char* value)
{
  /* NSOutlineView does not provide a straightforward way to hide disclosure triangles. */
  (void)ih;
  (void)value;
  return 0;
}

static int cocoaTreeMapMethod(Ihandle* ih)
{
  NSNib* outline_nib = nil;

  if (ih->data->show_toggle > 0)
    outline_nib = [NSNib IupCocoaOutlineViewToggle];
  else
    outline_nib = [NSNib IupCocoaOutlineView];

  NSArray* top_level_objects = nil;
  IupCocoaOutlineView* outline_view = nil;
  NSScrollView* scroll_view = nil;

  if ([outline_nib instantiateWithOwner:nil topLevelObjects:&top_level_objects])
  {
    for (id current_object in top_level_objects)
    {
      if ([current_object isKindOfClass:[NSScrollView class]])
      {
        scroll_view = current_object;
        break;
      }
    }
  }

  outline_view = (IupCocoaOutlineView*)[scroll_view documentView];
  NSCAssert([outline_view isKindOfClass:[IupCocoaOutlineView class]], @"Expected IupCocoaOutlineView");

  [scroll_view retain];

  [outline_view setIh:ih];

  IupCocoaTreeDragDropDelegate* tree_delegate = [[IupCocoaTreeDragDropDelegate alloc] init];

  [outline_view setDataSource:tree_delegate];
  [outline_view setDelegate:tree_delegate];

  objc_setAssociatedObject(scroll_view, IUP_COCOA_TREE_DELEGATE_OBJ_KEY, (id)tree_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [tree_delegate release];

  ih->handle = scroll_view;
  iupcocoaSetAssociatedViews(ih, outline_view, scroll_view);

  iupcocoaAddToParent(ih);

  [outline_view setHeaderView:nil];

  if (iupAttribGetBoolean(ih, "HIDELINES"))
  {
    /* NSOutlineView does not draw lines by default */
  }

  if (iupAttribGetBoolean(ih, "HIDEBUTTONS"))
  {
    /* NSOutlineView disclosure triangles cannot be easily hidden */
  }

  NSImage* leaf_image = iupImageGetImage(iupAttribGetStr(ih, "IMAGELEAF"), ih, 0, NULL);
  [outline_view setLeafImage:leaf_image];

  helperReplaceDefaultImages(ih, outline_view);

  if (iupAttribGetInt(ih, "ADDROOT"))
    iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  iupdrvTreeUpdateMarkMode(ih);

  cocoaTreeUpdateDragDrop(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    [outline_view setRefusesFirstResponder:YES];
    iupcocoaSetCanFocus(ih, 0);
  }
  else
  {
    iupcocoaSetCanFocus(ih, 1);
  }

  return IUP_NOERROR;
}

static void cocoaTreeUnMapMethod(Ihandle* ih)
{
  id root_view = ih->handle;

  {
    Ihandle* context_menu_ih = (Ihandle*)iupcocoaCommonBaseGetContextMenuAttrib(ih);
    if (NULL != context_menu_ih)
    {
      IupDestroy(context_menu_ih);
    }
    iupcocoaCommonBaseSetContextMenuAttrib(ih, NULL);
  }

  IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
  [outline_view setMarkStartNode:nil];

  iupcocoaSetAssociatedViews(ih, nil, nil);
  iupcocoaRemoveFromParent(ih);
  [root_view release];
  ih->handle = NULL;
}

void iupdrvTreeInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = cocoaTreeMapMethod;
  ic->UnMap = cocoaTreeUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  /* IupTree Attributes - GENERAL */
  iupClassRegisterAttribute(ic, "EXPANDALL", NULL, cocoaTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INDENTATION", cocoaTreeGetIndentationAttrib, cocoaTreeSetIndentationAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, cocoaTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, cocoaTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SCROLLVISIBLE", cocoaTreeGetScrollVisibleAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* IupTree Attributes - IMAGES */
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, cocoaTreeSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, cocoaTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGELEAF", NULL, cocoaTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, cocoaTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", NULL, cocoaTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

  /* IupTree Attributes - NODES */
  iupClassRegisterAttributeId(ic, "STATE", cocoaTreeGetStateAttrib,  cocoaTreeSetStateAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH", cocoaTreeGetDepthAttrib,  NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND", cocoaTreeGetKindAttrib,   NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PARENT", cocoaTreeGetParentAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", cocoaTreeGetNextAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", cocoaTreeGetLastAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", cocoaTreeGetPreviousAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FIRST", cocoaTreeGetFirstAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COLOR", cocoaTreeGetColorAttrib, cocoaTreeSetColorAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLE", cocoaTreeGetTitleAttrib,  cocoaTreeSetTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVALUE", cocoaTreeGetToggleValueAttrib, cocoaTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", cocoaTreeGetToggleVisibleAttrib, cocoaTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "SHOWRENAME", NULL, cocoaTreeSetShowRenameAttrib);

  iupClassRegisterAttribute(ic, "ROOTCOUNT", cocoaTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "CHILDCOUNT", cocoaTreeGetChildCountAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT", cocoaTreeGetTitleFontAttrib,  cocoaTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);

  /* IupTree Attributes - MARKS */
  iupClassRegisterAttributeId(ic, "MARKED", cocoaTreeGetMarkedAttrib, cocoaTreeSetMarkedAttrib, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "MARK",      NULL, cocoaTreeSetMarkAttrib,      NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "STARTING",  NULL, cocoaTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "MARKSTART", NULL, cocoaTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute  (ic, "MARKEDNODES", cocoaTreeGetMarkedNodesAttrib, cocoaTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "MARKWHENTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute  (ic, "VALUE", cocoaTreeGetValueAttrib, cocoaTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupTree Attributes - ACTION */
  iupClassRegisterAttributeId(ic, "DELNODE", NULL, cocoaTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "RENAME", NULL, cocoaTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MOVENODE", NULL, cocoaTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", NULL, cocoaTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* IupTree Attributes - macOS specific */
  iupClassRegisterAttribute(ic, "RUBBERBAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDELINES", NULL, cocoaTreeSetHideLinesAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "HIDEBUTTONS", NULL, cocoaTreeSetHideButtonsAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INFOTIP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* Tooltip attributes */
  iupClassRegisterAttribute(ic, "TIP", cocoaTreeGetTipAttrib, cocoaTreeSetTipAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* New API for view specific contextual menus */
  iupClassRegisterAttribute(ic, "CONTEXTMENU", iupcocoaCommonBaseGetContextMenuAttrib, cocoaTreeSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LAYERBACKED", cocoaTreeGetLayerBackedAttrib, cocoaTreeSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);

  iupClassRegisterReplaceAttribFunc(ic, "ACTIVE", cocoaTreeGetActiveAttrib, cocoaTreeSetActiveAttrib);

  /* Drag and Drop attributes */
  iupClassRegisterReplaceAttribFunc(ic, "SHOWDRAGDROP", NULL, cocoaTreeSetShowDragDropAttrib);
  iupClassRegisterAttribute(ic, "DRAGDROPTREE", NULL, cocoaTreeSetDragDropTreeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterReplaceAttribFunc(ic, "DRAGSOURCE", NULL, cocoaTreeSetDndControlAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "DROPTARGET", NULL, cocoaTreeSetDndControlAttrib);
}
