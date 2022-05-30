/** \file
 * \brief Tree Control
 *
 * See Copyright Notice in iup.h
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
#include "iup_stdcontrols.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_tree.h"

#include "iup_drvinfo.h"

#include "iupcocoa_drv.h"
#import "IupCocoaTreeToggleTableCellView.h"

// TODO:
// Removing the disclosure triangle, Disable indenting
// https://stackoverflow.com/questions/4251790/nsoutlineview-remove-disclosure-triangle-and-indent

// the point of this is we have a unique memory address for an identifier
static const void* IUP_COCOA_TREE_DELEGATE_OBJ_KEY = "IUP_COCOA_TREE_DELEGATE_OBJ_KEY";

static  NSString* const IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE = @"iupcocoa.outlineview.dragdrop";



static NSView* cocoaTreeGetRootView(Ihandle* ih)
{
	NSView* root_container_view = (NSView*)ih->handle;
	NSCAssert([root_container_view isKindOfClass:[NSView class]], @"Expected NSView");
	return root_container_view;
}

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

@class IupCocoaTreeItem;

// Be very careful about retain cycles. Nothing in this class retains anything at the moment.
// IupCocoaTreeItem retains this.
@interface IupCocoaTreeToggleReceiver : NSObject
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) IupCocoaTreeItem* treeItem; // back pointer to tree item (weak)
- (IBAction) myToggleClickAction:(id)the_sender;
@end

@implementation IupCocoaTreeToggleReceiver

// all properties are assign, so we don't need to override dealloc
/*
 - (void) dealloc
 {
	[super dealloc];
 }
 */

- (IBAction) myToggleClickAction:(id)the_sender;
{
//	Icallback callback_function;
//	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);
	Ihandle* ih = [self ihandle];
	IupCocoaTreeItem* tree_item = [self treeItem];
	NSControlStateValue new_state = [the_sender state];
	
	int item_id = iupTreeFindNodeId(ih, (InodeHandle*)tree_item);

	IFnii action_callback_function = (IFnii)IupGetCallback(ih, "TOGGLEVALUE_CB");
	if(action_callback_function)
	{
		if(action_callback_function(ih, item_id, (int)new_state) == IUP_CLOSE)
		{
			IupExitLoop();
		}
	}
	
}

@end



@interface IupCocoaTreeItem : NSObject
{
	IupCocoaTreeItem* parentItem;
	NSMutableArray* childrenArray;
	NSString* title;
	int kind; // ITREE_BRANCH ITREE_LEAF
	NSControlStateValue checkBoxState;
	bool checkBoxHidden;
	BOOL isDeleted;
	NSImage* bitmapImage;
	NSImage* collapsedImage;
	NSTableCellView* tableCellView; // kind of a hack to force layout in heightOf
	
	IupCocoaTreeToggleReceiver* toggleReceiver; // For TOGGLE_CB callbacks. cloning is tricky because of (maybe) different ih, so be careful.
}

@property(nonatomic, assign) int kind;
@property(nonatomic, assign) NSControlStateValue checkBoxState;
@property(nonatomic, assign, getter=isCheckBoxHidden) bool checkBoxHidden;
@property(nonatomic, copy) NSString* title;
@property(nonatomic, weak) IupCocoaTreeItem* parentItem;
@property(nonatomic, assign) BOOL isDeleted;
@property(nonatomic, retain) NSImage* bitmapImage;
@property(nonatomic, retain) NSImage* collapsedImage;
@property(nonatomic, weak) NSTableCellView* tableCellView; // this is kind of a hack to force layout in heightOf. I'm not sure if I want to keep a strong reference because I don't know if there is a possible circular reference here.
@property(nonatomic, retain) IupCocoaTreeToggleReceiver* toggleReceiver; // optional, depending if toggle is used

- (instancetype) cloneWithNewParentItem:(IupCocoaTreeItem*)new_parent_item ihandle:(Ihandle*)ih;

- (IupCocoaTreeItem*) childAtIndex:(NSUInteger)the_index;

@end

// forward declaration needed
static void cocoaTreeReloadItem(IupCocoaTreeItem* tree_item, NSOutlineView* outline_view);


@implementation IupCocoaTreeItem

@synthesize kind = kind;
@synthesize checkBoxState = checkBoxState;
@synthesize checkBoxHidden = checkBoxHidden;
@synthesize title = title;
@synthesize parentItem = parentItem;
@synthesize isDeleted = isDeleted;
@synthesize bitmapImage = bitmapImage; // is the expandedImage for branches
@synthesize collapsedImage = collapsedImage;
@synthesize tableCellView = tableCellView;
@synthesize toggleReceiver = toggleReceiver;


// Creates, caches, and returns the array of children
// Loads children incrementally
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
	NSArray* tmp = [self childrenArray];
	return [tmp count];
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
	//[tableCellView releae];
	tableCellView = nil; // weak ref
	
	[toggleReceiver release];
	
	[bitmapImage release];
	[collapsedImage release];

	[childrenArray release];
	[title release];
	parentItem = nil; // weak ref
	[super dealloc];
}

- (instancetype) cloneWithNewParentItem:(IupCocoaTreeItem*)new_parent_item ihandle:(Ihandle*)ih
{
	IupCocoaTreeItem* new_copy = [[IupCocoaTreeItem alloc] init];
	[new_copy setParentItem:new_parent_item];
	[new_copy setTableCellView:nil];
	[new_copy setIsDeleted:NO];
	
	[new_copy setBitmapImage:[self bitmapImage]];
	[new_copy setCollapsedImage:[self collapsedImage]];

	[new_copy setTitle:[self title]]; // this is a copy property
	[new_copy setKind:[self kind]];
	[new_copy setCheckBoxState:[self checkBoxState]];
	[new_copy setCheckBoxHidden:[self isCheckBoxHidden]];

	NSMutableArray* new_children_array = [[NSMutableArray alloc] init];
	[new_copy setChildrenArray:new_children_array];
	[new_children_array autorelease];
	
	for(IupCocoaTreeItem* original_item in childrenArray)
	{
		IupCocoaTreeItem* child_copy = [original_item cloneWithNewParentItem:new_copy ihandle:ih];
		[new_children_array addObject:child_copy];
	}

	IupCocoaTreeToggleReceiver* new_toggle_receiver = [[IupCocoaTreeToggleReceiver alloc] init];
//	IupCocoaTreeToggleReceiver* original_toggle_receiver = [self toggleReceiver];
	[new_toggle_receiver setTreeItem:new_copy];
	[new_toggle_receiver setIhandle:ih];
	[new_copy setToggleReceiver:new_toggle_receiver];

	return new_copy;
}




@end

/*
@interface IupCocoaTreeRoot : NSObject
{
	// Array of IupCocoaTreeItems
	NSMutableArray* topLevelObjects;
}
@end

@implementation IupCocoaTreeRoot

@end
*/

// We need to override NSOutlineView in order to implement things like keyDown for k_any
@interface IupCocoaOutlineView : NSOutlineView
{
	Ihandle* _ih;
	NSImage* leafImage;
	NSImage* expandedImage;
	NSImage* collapsedImage;
}
@property(nonatomic, assign) Ihandle* ih;
@property(nonatomic, retain) NSImage* leafImage;
@property(nonatomic, retain) NSImage* expandedImage;
@property(nonatomic, retain) NSImage* collapsedImage;

// My preference would be to use a zeroing weak reference, but I think that requires ARC
@property(nonatomic, retain) IupCocoaTreeItem* markStartNode;
@end

@implementation IupCocoaOutlineView
@synthesize ih = _ih;
@synthesize leafImage = leafImage;
@synthesize expandedImage = expandedImage;
@synthesize collapsedImage = collapsedImage;

- (void) dealloc
{
	[leafImage release];
	[expandedImage release];
	[collapsedImage release];
	[super dealloc];
}
//find which keys are being pressed from the_event object. You will then map these to the Iup keycodes.
//You invoke the user callback (3), following the usual Iup invoke callback conventions. This too should eventually be factored in a way we can call this for all widgets.


//////// Keyboard stuff

- (void) flagsChanged:(NSEvent*)the_event
{
//	NSLog(@"flagsChanged: %@", the_event);
//	NSLog(@"modifierFlags: 0x%X", [the_event modifierFlags]);
/*
    NSEventModifierFlagCapsLock           = 1 << 16, // Set if Caps Lock key is pressed.
    NSEventModifierFlagShift              = 1 << 17, // Set if Shift key is pressed.
    NSEventModifierFlagControl            = 1 << 18, // Set if Control key is pressed.
    NSEventModifierFlagOption             = 1 << 19, // Set if Option or Alternate key is pressed.
    NSEventModifierFlagCommand            = 1 << 20, // Set if Command key is pressed.
    NSEventModifierFlagNumericPad         = 1 << 21, // Set if any key in the numeric keypad is pressed.
    NSEventModifierFlagHelp               = 1 << 22, // Set if the Help key is pressed.
    NSEventModifierFlagFunction           = 1 << 23, // Set if any function key is pressed.
*/
	Ihandle* ih = [self ih];
    unsigned short mac_key_code = [the_event keyCode];
//    NSLog(@"mac_key_code : %d", mac_key_code);
	bool should_not_propagate = iupCocoaModifierEvent(ih, the_event, (int)mac_key_code);
	if(!should_not_propagate)
	{
		[super flagsChanged:the_event];
	}
}

- (void) keyDown:(NSEvent*)the_event
{
    // gets ihandle
    Ihandle* ih = [self ih];
//	NSLog(@"keyDown: %@", the_event);
    unsigned short mac_key_code = [the_event keyCode];
//    NSLog(@"keydown string: %d", mac_key_code);

	bool should_not_propagate = iupCocoaKeyEvent(ih, the_event, (int)mac_key_code, true);
	if(!should_not_propagate)
	{
		[super keyDown:the_event];
	}
}

- (void) keyUp:(NSEvent*)the_event
{
	Ihandle* ih = [self ih];
    unsigned short mac_key_code = [the_event keyCode];
	bool should_not_propagate = iupCocoaKeyEvent(ih, the_event, (int)mac_key_code, false);
	if(!should_not_propagate)
	{
		[super keyUp:the_event];
	}
}

@end


// We are not using NSComboBoxDataSource
@interface IupCocoaTreeDelegate : NSObject <NSOutlineViewDataSource, NSOutlineViewDelegate>
{
	NSMutableArray* treeRootTopLevelObjects;
	
//	NSMutableArray* orderedArrayOfSelections; // TODO: If we decide selection order is important enough and worth the risks of edge cases missing updates (like delnode)
	NSIndexSet* previousSelections;
	
}
@property(nonatomic, copy) NSArray* treeRootTopLevelObjects; // This is intended for external read-only access to iterate through all items, such as changing the branch/leaf images
- (NSUInteger) insertChild:(IupCocoaTreeItem*)tree_item_child withParent:(IupCocoaTreeItem*)tree_item_parent;
- (NSUInteger) insertChild:(IupCocoaTreeItem*)tree_item_child withParent:(IupCocoaTreeItem*)tree_item_parent targetChildIndex:(NSInteger)target_child_index;
- (NSUInteger) insertPeer:(IupCocoaTreeItem*)tree_item_new withSibling:(IupCocoaTreeItem*)tree_item_prev;
- (void) insertAtRoot:(IupCocoaTreeItem*)tree_item_new;
- (void) removeAllObjects;
- (NSIndexSet*) removeAllChildrenForItem:(IupCocoaTreeItem*)tree_item;
- (NSUInteger) removeItem:(IupCocoaTreeItem*)tree_item;
- (void) moveItem:(IupCocoaTreeItem*)tree_item targetParent:(IupCocoaTreeItem*)tree_item_parent targetChildIndex:(NSInteger)target_child_index;

//- (NSMutableArray*) dataArray;

// NSOutlineViewDataSource
- (NSInteger) outlineView:(NSOutlineView*)outline_view numberOfChildrenOfItem:(nullable id)the_item;
//- (id) outlineView:(NSOutlineView*)outline_view child:(NSInteger)index ofItem:(nullable id)the_item;
- (BOOL) outlineView:(NSOutlineView*)outline_view isItemExpandable:(id)the_item;
// NSOutlineViewDelegate
- (nullable NSView *)outlineView:(NSOutlineView*)outline_view viewForTableColumn:(nullable NSTableColumn*)table_column item:(id)the_item;
// NSOutlineViewDelegate
- (void) outlineViewSelectionDidChange:(NSNotification*)the_notification;
// NSOutlineViewDelegate, for CANFOCUS
- (NSIndexSet*) outlineView:(NSOutlineView*)outline_view selectionIndexesForProposedSelection:(NSIndexSet*)proposed_selection_indexes;

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
/*
static NSUInteger Helper_CountAllItems(IupCocoaTreeDelegate* tree_delegate)
{
	NSUInteger counter = 0;
	for(IupCocoaTreeItem* a_item in [tree_delegate treeRootTopLevelObjects])
	{
		counter += Helper_RecursivelyCountItems(a_item);
	}
	return counter;
}
*/

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

/*
static NSUInteger Helper_RecursivelyCountLeafs(IupCocoaTreeItem* the_item)
{
	// If this is a leaf, then there are no children to descend into and count
	if(ITREE_LEAF == [the_item kind])
	{
		return 1;
	}

	// else we are a branch
	NSUInteger counter = 0;
	for(IupCocoaTreeItem* a_item in [the_item childrenArray])
	{
		counter += Helper_RecursivelyCountLeafs(a_item);
	}
	return counter;
}
*/

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
//		NSLog(@"Helper_RecursivelyFindFlatIndexofTreeItemInOutlineView a_item=%@, flat_index=%d", [a_item title], *out_flat_index);
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

// This is a helper function that traverses through a NSOutlineView data source delegate looking for a certain item, and returns the array index this would be in for the ordering of the Iup node_cache
static NSInteger Helper_FindFlatIndexofTreeItemInOutlineView(IupCocoaTreeDelegate* tree_delegate, IupCocoaTreeItem* tree_item, NSInteger* out_flat_index)
{
	bool is_found = false;
	for(IupCocoaTreeItem* a_item in [tree_delegate treeRootTopLevelObjects])
	{
//		NSLog(@"Helper_FindFlatIndexofTreeItemInOutlineView a_item=%@, flat_index=%d", [a_item title], *out_flat_index);
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
@synthesize treeRootTopLevelObjects = treeRootTopLevelObjects;

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
	return 0; // always index 0 since we always insert in the first position
}

- (NSUInteger) insertChild:(IupCocoaTreeItem*)tree_item_child withParent:(IupCocoaTreeItem*)tree_item_parent targetChildIndex:(NSInteger)target_child_index
{
	// IUP always inserts the child in the first position, not the last, when in this parent/child relationship
	[[tree_item_parent childrenArray] insertObject:tree_item_child atIndex:target_child_index];
	[tree_item_child setParentItem:tree_item_parent];
	return target_child_index;
}

- (NSUInteger) insertPeer:(IupCocoaTreeItem*)tree_item_new withSibling:(IupCocoaTreeItem*)tree_item_prev
{
	IupCocoaTreeItem* tree_item_parent = [tree_item_prev parentItem];
	if(nil != tree_item_parent)
	{
		[tree_item_new setParentItem:tree_item_parent];
		// insert the new node after reference node
		NSMutableArray* children_array = [tree_item_parent childrenArray];
		NSUInteger prev_index = [children_array indexOfObject:tree_item_prev];
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
		// we are adding a peer to the root (case is ADDROOT=NO)
		NSUInteger target_index = 0;

		NSUInteger object_index = [treeRootTopLevelObjects indexOfObject:tree_item_prev];
		if(object_index != NSNotFound)
		{
			// insert after the previous (reference) node
			target_index = object_index + 1;
		}
	
		[treeRootTopLevelObjects insertObject:tree_item_new atIndex:target_index];
		return target_index;
	}
}

- (void) insertAtRoot:(IupCocoaTreeItem*)tree_item_new
{
	// IUP always inserts the child in the first position, not the last
	[treeRootTopLevelObjects insertObject:tree_item_new atIndex:0];
}

- (void) removeAllObjects
{
	[treeRootTopLevelObjects removeAllObjects];
}

// Returns the indexes of the top-level children that get removed
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

// This is a helper for removeItem:
// This is a special helper because when using fast enumeration in removeItem: we can't change the parent's childrenArray to remove this node.
// So in this helper, we don't try and assume removeItem: will handle that last step itself.
// So this method is here so we can clear the children data and update the count.
- (void) removeRecursiveChildItemHelper:(IupCocoaTreeItem*)tree_item
{
	// First, if this node has any children, recursively traverse through all the children and remove them.
	NSMutableArray* children_array = [tree_item childrenArray];
	for(IupCocoaTreeItem* an_item in children_array)
	{
		[self removeRecursiveChildItemHelper:an_item];
	}
	// clear the children array so in case there is another reference that is still using this pointer, it will have updated info that there are no children.
	[children_array removeAllObjects];
	[tree_item setIsDeleted:YES];
}

- (NSUInteger) removeItem:(IupCocoaTreeItem*)tree_item
{
	if(nil == tree_item)
	{
		return NSNotFound;
	}
	// If we already removed this item, the parentItem is nil.
	if(YES == [tree_item isDeleted])
	{
		return NSNotFound;
	}
	
	// First, if this node has any children, recursively traverse through all the children and remove them.
	NSMutableArray* children_array = [tree_item childrenArray];
	for(IupCocoaTreeItem* an_item in children_array)
	{
		[self removeRecursiveChildItemHelper:an_item];
	}
	// clear the children array so in case there is another reference that is still using this pointer, it will have updated info that there are no children.
	[children_array removeAllObjects];

	// now remove this node by going to the parent and removing this from the parent's childrenArray
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
		// this is top level node
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

	// remove from the old location (don't call removeItem: because it kills the children)
	if(parent_tree_item)
	{
		[[parent_tree_item childrenArray] removeObject:tree_item];
	}
	else
	{
		[treeRootTopLevelObjects removeObject:tree_item];
	}
	
	// insert to the new location
	if(target_parent_tree_item)
	{
		[[target_parent_tree_item childrenArray] insertObject:tree_item atIndex:target_child_index];
	}
	else
	{
		[treeRootTopLevelObjects insertObject:tree_item atIndex:target_child_index];
	}

	// Now that the node is moved, it has a new parent
	[tree_item setParentItem:target_parent_tree_item];
}

- (NSInteger) outlineView:(NSOutlineView*)outline_view numberOfChildrenOfItem:(nullable id)the_item
{
	// FIXME: temp placeholder
	// FIXME: temp placeholder
	if(nil == the_item)
	{
		NSInteger the_count = [treeRootTopLevelObjects count];
		return the_count;
	}
	else
	{
		NSInteger the_count = [the_item numberOfChildren];
		return the_count;
	}
}

- (id) outlineView:(NSOutlineView*)outline_view child:(NSInteger)the_index ofItem:(nullable id)the_item
{
	// FIXME: temp placeholder
	if(nil == the_item)
	{
//		return nil;
//		IupCocoaTreeItem* dummy = [[[IupCocoaTreeItem alloc] init] autorelease];
// return dummy;
		IupCocoaTreeItem* tree_item = [treeRootTopLevelObjects objectAtIndex:the_index];
		return tree_item;
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
		
#if 0
		// This is the basic implementation.
		// The problem is if you add children after this gets called, this happens too late.
		// The workaround would be to use dispatch_async, but that causes a delay and flicker.
		// One other possible solution to to force a reload on the parent item on delete and add node events.
		if([tree_item numberOfChildren] > 0)
		{
			return YES;
		}
		else
		{
			return NO;
		}
#else
		// We are preferring this implementation over the numberOfChildren > 0
		// because when we first add a branch without children, expandItem won't work.
		// The workaround for that is to use dispatch_async, but this causes a delay and flicker.
		// Since IUP makes users declare the difference between a leaf & branch, we can assume all branches should be expandable.
		// And we have that information immediately.
		// The downside "bug" with this implementation is empty branches always show the triangle which is not typical.
		// One other possible solution is to go back to the above or do a hybrid, and try the reload parent idea.
		if([tree_item kind] == ITREE_BRANCH)
		{
#if 0
			// UPDATE1: This might work now an only show the triangle when has children, due to all the other changes I made with adding/deleting.
			// UPDATE2: UPDATE1 seemed to work, but it caused a different problem. For moving (reordering) nodes via drag-and-drop,
			// if a branch is empty, we are unable to move nodes under it.
			// So I think I must keep this path disabled unless there is a different workaround.
			if([tree_item numberOfChildren] > 0)
			{
				return YES;
			}
			else
			{
				return NO;
			}
#else
			return YES;
#endif
		}
		else
		{
			return NO;
		}
#endif
}

/* // Not needed for View based NSOutlineView
- (nullable id)outlineView:(NSOutlineView *)outline_view objectValueForTableColumn:(nullable NSTableColumn *)table_column byItem:(nullable id)the_item
{
	//return (the_item == nil) ? @"/" : @"lower";
	if(nil == the_item)
	{
		return @"Hello World";
	}
	else
	{
		IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)the_item;
		NSCAssert([tree_item isKindOfClass:[IupCocoaTreeItem class]], @"Expected IupCocoaTreeItem");
		return [tree_item title];
	}
	
}
*/


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

// WARNING: This method needs to be fast for performance.
// I'm worried the image support is already a bit too complicated because it allows for any image size. But I think it will be fine for desktop.
- (CGFloat) outlineView:(NSOutlineView*)outline_view heightOfRowByItem:(id)the_item
{
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)the_item;
	CGFloat text_height = 17.0;
	
	// TODO: Height needs to also account for font if the user changed it
	CGFloat image_width = 0.0;
	CGFloat image_height = 0.0;
	NSImage* active_image = helperGetActiveImageForTreeItem(tree_item, (IupCocoaOutlineView*)outline_view, &image_width, &image_height);
	
	
	// This is a ugly hack keeping the table cell view in the item.
	// All my other attempts to get the table cell view have failed.
	// DOC: If you call viewAtColumn:row:makeIfNecessary: or rowViewAtRow:makeIfNecessary: within your implementation of this method, an exception is thrown.
	// Perhaps I could compute the raw string height if I know the font and hard code a value for the textfield padding.
	NSTableCellView* table_cell_view = [tree_item tableCellView];

	NSTextField* text_field = [table_cell_view textField];
	NSSize text_field_size = { 0, 0 };
	if(text_field)
	{
		text_field_size = [text_field fittingSize];
	}
	
	if(text_field_size.height > text_height)
	{
		text_height = text_field_size.height;
	}
	else if(text_field_size.height == 0)
	{
		// don't allow 0
	}
	else
	{
		// should I allow smaller text heights?
		text_height = text_field_size.height;
	}


	if(active_image)
	{
		if(image_height < text_height)
		{
			return text_height;
		}
		else
		{
			return image_height;
		}
	}
	else
	{
		return text_height;
	}

}

// NSOutlineViewDelegate
// WARNING: This is another method that should be fast for performance.
- (nullable NSView *)outlineView:(NSOutlineView*)outline_view viewForTableColumn:(nullable NSTableColumn*)table_column item:(id)the_item
{
	Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)the_item;
	NSCAssert([tree_item isKindOfClass:[IupCocoaTreeItem class]], @"Expected IupCocoaTreeItem");
	NSString* string_item = [tree_item title];


	NSTableCellView* table_cell_view = nil;
	
	BOOL is_enabled = [outline_view isEnabled];

	
	// 0 for no toggle, 1 for toggle, 2 for toggle with 3-state
	if(ih->data->show_toggle > 0)
	{
		// Get an existing cell with the MyView identifier if it exists
		table_cell_view = [outline_view makeViewWithIdentifier:@"IupCocoaTreeToggleTableCellView" owner:self];
		if(nil == table_cell_view)
		{
			table_cell_view = [[IupCocoaTreeToggleTableCellView alloc] initWithFrame:NSZeroRect];

			// The identifier of the NSTextField instance is set to MyView.
			// This allows the cell to be reused.
			
			[table_cell_view setIdentifier:@"IupCocoaTreeToggleTableCellView"];
		}
		IupCocoaTreeToggleTableCellView* toggle_cell_view = (IupCocoaTreeToggleTableCellView*)table_cell_view;

		NSButton* check_box = [toggle_cell_view checkBox];
		if(ih->data->show_toggle == 2)
		{
			[check_box setAllowsMixedState:YES];
		}
		else
		{
			[check_box setAllowsMixedState:NO];
		}
		NSControlStateValue check_box_value = [tree_item checkBoxState];
		[check_box setState:check_box_value];
		
		bool check_box_hidden = [tree_item isCheckBoxHidden];
		[check_box setHidden:check_box_hidden];
		
		// Set up the TOGGLE_CB. This requires use to use a delegate object, hence a lot of relationships to set up.
		IupCocoaTreeToggleReceiver* toggle_receiver = [tree_item toggleReceiver];
		if(nil == toggle_receiver)
		{
			toggle_receiver = [[IupCocoaTreeToggleReceiver alloc] init];
			[toggle_receiver setTreeItem:tree_item];
			[tree_item setToggleReceiver:toggle_receiver];
		}
		[toggle_receiver setIhandle:ih];
		[check_box setTarget:toggle_receiver];
		[check_box setAction:@selector(myToggleClickAction:)];

		[check_box setEnabled:is_enabled];
		
	}
	else
	{
		table_cell_view = [outline_view makeViewWithIdentifier:@"IupCocoaTreeTableCellView" owner:self];
		// There is no existing cell to reuse so create a new one
		if(nil == table_cell_view)
		{
			table_cell_view = [[NSTableCellView alloc] initWithFrame:NSZeroRect];

			// The identifier of the NSTextField instance is set to MyView.
			// This allows the cell to be reused.
			
			[table_cell_view setIdentifier:@"IupCocoaTreeTableCellView"];
			
			IupCocoaTreeToggleReceiver* toggle_receiver = [tree_item toggleReceiver];
			if(nil != toggle_receiver)
			{
				[toggle_receiver setTreeItem:nil];
				[toggle_receiver setIhandle:nil];
				[tree_item setToggleReceiver:nil];
			}
		}
	
	}
	

	



	// table_cell_view is now guaranteed to be valid, either as a reused cell
	// or as a new cell, so set the stringValue of the cell to the
	// nameArray value at row
	NSTextField* text_field = nil;
	text_field = [table_cell_view textField];
	NSImageView* image_view = nil;
	image_view = [table_cell_view imageView];
	[text_field setStringValue:string_item];

	BOOL show_rename = (BOOL)ih->data->show_rename;
	[text_field setEditable:show_rename];


	CGFloat image_width = 0.0;
	CGFloat image_height = 0.0;
	NSImage* active_image = helperGetActiveImageForTreeItem(tree_item, (IupCocoaOutlineView*)outline_view, &image_width, &image_height);
	
	if(nil == active_image)
	{
		[image_view setHidden:YES];
		[image_view setImage:nil];
	}
	else
	{
		[image_view setHidden:NO];
		[image_view setImage:active_image];
	}
	
	
	[text_field setEnabled:is_enabled];
	[image_view setEnabled:is_enabled];

	[tree_item setTableCellView:table_cell_view]; // kind of a hack. We need it to compute the size in heightOf
 
	// Return the result
	return table_cell_view;
}

// I think it is a really bad idea to change images based on expanded or closed
// because this creates another potential reload (which loses selection data)
// and another potential performance bottleneck.
// This is not a typical Mac/Cocoa behavior.
// But the IUP API demands it.
// There is an optimization here to not swap images if the image is the same.
// However, this is a pointer comparison and requires both the user and IUP implementation to not accidentally load the same image twice
// or create two separate object wrappers around the same image.
// I recommend we add something to the official API documation that separate images is a bad idea.
- (void) outlineViewItemWillExpand:(NSNotification*)the_notification
{
	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)[the_notification object];
		NSDictionary* user_info = [the_notification userInfo];
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)[user_info objectForKey:@"NSObject"];
//	Ihandle* ih = [outline_view ih];
	
	if(nil == tree_item)
	{
		return;
	}
	


	NSImage* expanded_image = [tree_item bitmapImage];
	NSImage* collapsed_image = [tree_item collapsedImage];
	NSImage* fallback_expanded_image = [outline_view expandedImage];
	NSImage* fallback_collapsed_image = [outline_view collapsedImage];

	if(expanded_image || fallback_expanded_image)
	{
		NSImage* which_expanded_image = nil;
		if(expanded_image)
		{
			which_expanded_image = expanded_image;
		}
		else
		{
			which_expanded_image = fallback_expanded_image;
		}
		NSImage* which_collapsed_image = nil;
		if(collapsed_image)
		{
			which_collapsed_image = collapsed_image;
		}
		else
		{
			which_collapsed_image = fallback_collapsed_image;
		}

		// Only reload if the expanded and collapsed images are different
		// (I'm worried that switching images is not a typical Mac behavior and may hurt built-in performance optimizations.
		// Also, reloading may reset selection which is not nice.)
		// Note: I've been using retain, hoping that we just have a simple pointer comparison and it will avoid doing slow pixel comparisons.
		if(![which_expanded_image isEqual:which_collapsed_image])
		{
			cocoaTreeReloadItem(tree_item, outline_view);
		}
	}
	

}

// I think it is a really bad idea to change images based on expanded or closed
// because this creates another potential reload (which loses selection data)
// and another potential performance bottleneck.
// This is not a typical Mac/Cocoa behavior.
// But the IUP API demands it.
// There is an optimization here to not swap images if the image is the same.
// However, this is a pointer comparison and requires both the user and IUP implementation to not accidentally load the same image twice
// or create two separate object wrappers around the same image.
// I recommend we add something to the official API documation that separate images is a bad idea.
- (void) outlineViewItemWillCollapse:(NSNotification*)the_notification
{

	IupCocoaOutlineView* outline_view = [the_notification object];
	NSDictionary* user_info = [the_notification userInfo];
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)[user_info objectForKey:@"NSObject"];
//	Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];
	
	if(nil == tree_item)
	{
		return;
	}
	
	
	
	NSImage* expanded_image = [tree_item bitmapImage];
	NSImage* collapsed_image = [tree_item collapsedImage];
	NSImage* fallback_expanded_image = [outline_view expandedImage];
	NSImage* fallback_collapsed_image = [outline_view collapsedImage];

	if(collapsed_image || fallback_collapsed_image)
	{
		NSImage* which_expanded_image = nil;
		if(expanded_image)
		{
			which_expanded_image = expanded_image;
		}
		else
		{
			which_expanded_image = fallback_expanded_image;
		}
		NSImage* which_collapsed_image = nil;
		if(collapsed_image)
		{
			which_collapsed_image = collapsed_image;
		}
		else
		{
			which_collapsed_image = fallback_collapsed_image;
		}

		// Only reload if the expanded and collapsed images are different
		// (I'm worried that switching images is not a typical Mac behavior and may hurt built-in performance optimizations.
		// Also, reloading may reset selection which is not nice.)
		// Note: I've been using retain, hoping that we just have a simple pointer comparison and it will avoid doing slow pixel comparisons.
		if(![which_expanded_image isEqual:which_collapsed_image])
		{
			cocoaTreeReloadItem(tree_item, outline_view);
		}
	}
	


}

- (void) handleSelectionDidChange:(NSOutlineView*)outline_view
{
	// Rules:
	// If we are in single selection mode, we use the single_cb
	// If we are in multiple selection mode, then one of the following:
	// - If it is a single selection, then use single_cb
	// - If it is a multiple selection, then use multi_cb and skip single_cb
	//     - but if multi_cb is not defined, invoke multiple callbacks of single_cb
	// Also, we need to remember that to do multi-unselection_cb.
	// - This fires if there was a multi-selection from the last time.
	
	// Additional notes:
	
	// (1) Multi_cb is supposed to be a contiguous range for a single action.
	// While I expect that will be the typical case on Mac,
	// I do worry that Apple may have or introduce some built-in key-shortcut (e.g. cmd-a selects all) that can create a non-contiguous selection.
	// For example, what if there is an "invert selection" option?
	// So say you pick the middle item. Then you invert it which gets you everything except the middle item.
	// You now have a discontinuous selection created in one-shot.

	// (2) Delete nodes and selection.
	// I asked Scuri if we are supposed to trigger a selection callback when nodes are deleted,
	// since this will alter the list of selected items.
	// He says that IUP does not do a callback for this case.
	// But this is why I broke this into a helper method, in case it needs to be called directly instead of just on Apple's selection notification.
	// (Apple does not seem to give selection notification callbacks for changes caused by delete or reloadData either.)

	NSCAssert([outline_view isKindOfClass:[IupCocoaOutlineView class]], @"Expected IupCocoaOutlineView");
	Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];
	if(NULL == ih)
	{
		return;
	}
	
	if(iupAttribGet(ih, "_IUPTREE_IGNORE_SELECTION_CB"))
	{
		return;
	}


	// May not be the best way to determine callback type since the user can change this on the fly.
//	BOOL in_mulitple_selection_mode = [outline_view allowsMultipleSelection];
	IFnii single_selection_cb = (IFnii)IupGetCallback(ih, "SELECTION_CB");
    IFnIi multi_selection_cb = (IFnIi)IupGetCallback(ih, "MULTISELECTION_CB");
    IFnIi multi_unselection_cb = (IFnIi)IupGetCallback(ih, "MULTIUNSELECTION_CB");

	// No sense doing any work if callbacks are not set.
	// NOTE: setting previousSelection will also be skipped in this case.
	if((single_selection_cb == NULL) && (multi_selection_cb == NULL) && (multi_unselection_cb == NULL))
	{
		return;
	}

#if 0
	// debug: print all currently selected items
	{
		NSIndexSet* selected_index = [outline_view selectedRowIndexes];
		NSUInteger selected_i = [selected_index firstIndex];
		while(selected_i != NSNotFound)
		{
			id selected_item = [outline_view itemAtRow:selected_i];
			NSLog(@"all selected_item: %@", [selected_item title]);
			// get the next index in the set
			selected_i = [selected_index indexGreaterThanIndex:selected_i];
		}
	}
#endif
	
	// First handle the unselections
	{
		// We will get a copy of the previous selections.
		NSMutableIndexSet* unselected_set = [previousSelections mutableCopy];
		if(unselected_set != nil)
		{
			// Then remove the current selections from the previous selections.
			// This will leave the unselected items.
			[unselected_set removeIndexes:[outline_view selectedRowIndexes]];
			
			NSUInteger number_of_items = [unselected_set count];

			// If the previous selection had more than 1, it was a multi-selection.
			// That means we need to do a multi-unselection to balance it.
			// This is safer than testing for whether the outlineview is in multi-mode or not because the user could have changed it on the fly.
			if(number_of_items == 0)
			{
				// do nothing
			}
			else if([previousSelections count] > 1)
			{
				if((NULL != multi_unselection_cb) || (NULL != single_selection_cb))
				{
					// We are in the multiple unselection case
					// VLA
					int array_of_ids[number_of_items];
					NSUInteger selected_i = [unselected_set firstIndex];
					size_t i = 0;
					while(selected_i != NSNotFound)
					{
						NSCAssert(i<number_of_items, @"Overflow: More indexes than expected.");

						array_of_ids[i] = (int)selected_i;
						i++;
						// id selected_item = [outline_view itemAtRow:selected_i];
						//	NSLog(@"removed selected_item: %@", [selected_item title]);
						// get the next index in the set
						selected_i = [unselected_set indexGreaterThanIndex:selected_i];
					}

					if(NULL != multi_unselection_cb)
					{
						multi_unselection_cb(ih, array_of_ids, (int)number_of_items);
					}
					else if(NULL != single_selection_cb)
					{
						for(size_t j=0; j<number_of_items; j++)
						{
							single_selection_cb(ih, array_of_ids[j], 1);
						}
					}
				}

			
			}
			else
			{
				// We are in the single unselection case
				// This should be 0, but just in case there are more, use the first one.
				if(number_of_items > 0)
				{
					if(NULL != single_selection_cb)
					{
						NSUInteger selected_i = [unselected_set firstIndex];
			           	single_selection_cb(ih, (int)selected_i, 0);
					}
				}
				// else do nothing since there was nothing unselected

			}
			
			[unselected_set release];
		}
	} // end unselections
	
	
	
	// handle the selections
	{
		// Get a copy of all the current selections
		NSMutableIndexSet* added_selected_set = [[outline_view selectedRowIndexes] mutableCopy];
		// Subtract out the previousSelections from the current selections which leaves just the newly added selections.
		[added_selected_set removeIndexes:previousSelections];

		if(added_selected_set != nil)
		{
			NSUInteger number_of_items = [added_selected_set count];

			if(number_of_items == 0)
			{
				// do nothing
			}
			else if(number_of_items > 1)
			{
				if((NULL != multi_selection_cb) || (NULL != single_selection_cb))
				{
					// We are in the multiple selection case
					// VLA
					int array_of_ids[number_of_items];
					NSUInteger selected_i = [added_selected_set firstIndex];
					size_t i = 0;
					while(selected_i != NSNotFound)
					{
						NSCAssert(i<number_of_items, @"Overflow: More indexes than expected.");

						array_of_ids[i] = (int)selected_i;
						i++;
						// id selected_item = [outline_view itemAtRow:selected_i];
						// NSLog(@"added selected_item: %@", [selected_item title]);
						// get the next index in the set
						selected_i = [added_selected_set indexGreaterThanIndex:selected_i];
					}

					if(NULL != multi_selection_cb)
					{
						multi_selection_cb(ih, array_of_ids, (int)number_of_items);
					}
					else if(NULL != single_selection_cb)
					{
						for(size_t j=0; j<number_of_items; j++)
						{
							single_selection_cb(ih, array_of_ids[j], 1);
						}
					}
				}
			}
			else // number_of_items == 1
			{
				// We are in the single selection case

				if(NULL != single_selection_cb)
				{
					NSUInteger selected_i = [added_selected_set firstIndex];
					single_selection_cb(ih, (int)selected_i, 1);
				}
			}
			
			[added_selected_set release];
		}
	} // end selections
	
	
	// Release the old previousSelections and save the new/current selections as previousSelections for the next time this is called.
	[previousSelections release];
	previousSelections = [[outline_view selectedRowIndexes] copy];
}

- (void) outlineViewSelectionDidChange:(NSNotification*)the_notification
{
	NSOutlineView* outline_view = [the_notification object];
	[self handleSelectionDidChange:outline_view];

}

- (NSIndexSet*) outlineView:(NSOutlineView*)outline_view selectionIndexesForProposedSelection:(NSIndexSet*)proposed_selection_indexes
{
	Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];
	if(iupAttribGetBoolean(ih, "CANFOCUS"))
	{
		return proposed_selection_indexes;
	}
	else
	{
		return nil;
	}
}

@end // IupCocoaTreeDelegate


/*****************************************************************************/
/* DRAG AND DROP                                                             */
/*****************************************************************************/
// We need to make a separate delegate subclass because there is no way to toggle the drag-and-drop behavior directly.
// Those who want drag-and-drop must use a delegate instance of this subclass type,
// whereas those who don't must use the base class.

@interface IupCocoaTreeDragDropDelegate : IupCocoaTreeDelegate
{
	id itemBeingDragged; // This is used to compare whether the item being dragged ends up dragging onto itself so we can reject it in validation

}
@property(nonatomic, weak) id itemBeingDragged;
@end


@implementation IupCocoaTreeDragDropDelegate

@synthesize itemBeingDragged = itemBeingDragged;

// Drag and drop
// Cocoa makes you do overkill for moving/copying nodes via drag-and-drop and you must implement full blown pasteboard support.
// General idea here:
// https://stackoverflow.com/questions/42315288/how-to-create-nspasteboardwriting-for-drag-and-drop-in-nscollectionview
// Also, suggestions for putting a pointer in NSData
// https://stackoverflow.com/questions/38890174/use-nsvalue-to-wrap-a-c-pointer

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
    // No dragging if <some condition isn't met>
	if(![the_item isKindOfClass:[IupCocoaTreeItem class]])
	{
		return nil;
	}
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)the_item;
	
	
//    Book *book = (Book *)(((NSTreeNode *)item).representedObject);
//    NSString *identifier = book.title;
	
    NSPasteboardItem* paste_board_item = [[NSPasteboardItem alloc] init];
    [paste_board_item autorelease];
//    NSString* unique_id_string = [NSString stringWithFormat:@"%p", tree_item];
//    [paste_board_item setString:unique_id_string forType:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE];

	NSValue* pointer_value = [NSValue valueWithPointer:tree_item];
	NSData* data_value = helperDataWithValue(pointer_value);
	
	// archivedDataWithRootObject throws an exception using a pointer
//	NSData* data_value = [NSKeyedArchiver archivedDataWithRootObject:pointer_value];
    [paste_board_item setData:data_value forType:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE];
    return paste_board_item;
}


- (void)outlineView:(NSOutlineView *)outlineView draggingSession:(NSDraggingSession *)session willBeginAtPoint:(NSPoint)screenPoint forItems:(NSArray *)dragged_items
{
	[self setItemBeingDragged:nil];

    // If only one item is being dragged, mark it so we can reorder it with a special pboard indicator
    if([dragged_items count] == 1)
    {
    	[self setItemBeingDragged:[dragged_items lastObject]];
	}
}


static int helperCallDragDropCb(Ihandle* ih, IupCocoaTreeItem* tree_item_drag, IupCocoaTreeItem* tree_item_drop, NSInteger child_index, bool is_copy)
{
	IFniiii drag_drop_cb = (IFniiii)IupGetCallback(ih, "DRAGDROP_CB");
	int is_shift = 0;

	// Cocoa has higher-level APIs to determine move vs. copy to support track pads and acessibility.
	// Also Cocoa uses 'Option', not Control for copy-drag.
	// So I will not use the key states.
	// (They also aren't implemented yet as I do this.)
	// For consistency, I think I rather always pass is_shift=0 because we aren't reading the keyboard for control,
	// and shift doesn't do anything natively for this operation.
  /*
  char key[5];
  iupdrvGetKeyState(key);
  if (key[0] == 'S')
    is_shift = 1;
  if (key[1] == 'C')
    *is_ctrl = 1;
  else
    *is_ctrl = 0;
*/

	if(drag_drop_cb)
	{
		int drag_id = iupTreeFindNodeId(ih, (InodeHandle*)tree_item_drag);
		int drop_id = iupTreeFindNodeId(ih, (InodeHandle*)tree_item_drop) + (int)child_index;
		return drag_drop_cb(ih, drag_id, drop_id, is_shift, (int)is_copy);
	}

	return IUP_CONTINUE; /* allow to move by default if callback not defined */
}

- (NSDragOperation)outlineView:(NSOutlineView *)outline_view validateDrop:(id < NSDraggingInfo >)drag_info proposedItem:(id)target_item proposedChildIndex:(NSInteger)child_index
{
//NSLog(@"%@", NSStringFromSelector(_cmd));

	NSArray<NSPasteboardType>* drag_types = [[drag_info draggingPasteboard] types];
	Ihandle* ih = [(IupCocoaOutlineView*)outline_view ih];

    if([drag_types containsObject:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE])
    {
		// If the sender is ourselves, then we accept it as a move or copy, depending on the modifier key
        if([drag_info draggingSource] == outline_view)
        {
        	// Since this is callback shared with DRAGDROPTREE, do an extra check to make sure this feature is on.
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
					// This seems to prevent copying under a leaf which would force us to convert branches to leaves, which I don't think IUP allows.
					return NSDragOperationNone;
				}
            }
            else
            {
            	BOOL can_drag = child_index >= 0 && target_item;

				if(can_drag)
				{
					//	NSInteger index_of_target_item = [outline_view rowForItem:target_item];

					// This code (from Apple example) seems to be to prevent dragging a branch as a child of itself (which will throw an exception if allowed).
					if([self itemBeingDragged])
					{
						// We have a single item being dragged to move; validate if we can move it or not
						// A move is only valid if the target isn't a child of the thing being dragged. We validate that now
						id item_walker = target_item;
						while(item_walker)
						{
							if(item_walker == [self itemBeingDragged])
							{
								return NSDragOperationNone; // Can't do it!
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
						// For multiple items, what do we do?
						return NSDragOperationNone;
					}
				}
				else
				{
					return NSDragOperationNone;
				}
			}
		}
		// Dragging an item from one NSOutlineView to another NSOutlineView
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
					// special check for dragging onto an empty outlineview
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
					// This seems to prevent copying under a leaf which would force us to convert branches to leaves, which I don't think IUP allows.
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
					// special check for dragging onto an empty outlineview
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
/* AUXILIAR FUNCTIONS                                                        */
/*****************************************************************************/

#if 0
static void cocoaTreeChildRebuildCacheRec(Ihandle* ih, IupCocoaTreeItem* tree_item, int* object_id)
{
	for(IupCocoaTreeItem* a_item in [tree_item childrenArray])
	{
    (*object_id)++;
    ih->data->node_cache[*object_id].node_handle = tree_item; // I don't think should be retained

    /* go recursive to children */
    cocoaTreeChildRebuildCacheRec(ih, a_item, object_id);

		
 	 }
}

static void cocoaTreeRebuildNodeCache(Ihandle* ih, int object_id, IupCocoaTreeItem* tree_item)
{
  ih->data->node_cache[object_id].node_handle = tree_item;
  cocoaTreeChildRebuildCacheRec(ih, tree_item, &object_id);
}
#endif

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

// This will compile-in a runtime check to make sure the data source delegate and the Iup node_cache are consistent.
// The check is kind of slow, so it should only be used for debugging.
// define to 1 to use. define to 0 to disable.
#define IUPCOCOA_TREE_TEST_VALIDATE_NODE_CACHE 0
#if IUPCOCOA_TREE_TEST_VALIDATE_NODE_CACHE

// This is a helper function that traverses through a NSOutlineView data source delegate looking for a certain item, and returns the array index this would be in for the ordering of the Iup node_cache
static NSArray<IupCocoaTreeItem*>* testValidateCocoaTreeCreateFlatItemArray(IupCocoaTreeDelegate* tree_delegate)
{
	NSMutableArray* flattened_array_of_items = [[[NSMutableArray alloc] init] autorelease];
	
	for(IupCocoaTreeItem* a_item in [tree_delegate treeRootTopLevelObjects])
	{
		[flattened_array_of_items addObject:a_item];
		iupCocoaTreeRecursivelyCreateFlatItemArray(a_item, flattened_array_of_items);
	}
	return flattened_array_of_items;
}

// Returns true if passes test
static bool testValidateCocoaTreeNodeCache(Ihandle* ih, IupCocoaTreeDelegate* tree_delegate)
{
	NSArray<IupCocoaTreeItem*>* flattened_array_of_items = testValidateCocoaTreeCreateFlatItemArray(tree_delegate);
	bool found_error = false;

	if(ih->data->node_count != [flattened_array_of_items count])
	{
		found_error = true;
		NSLog(@"INTERNAL ERROR in testValidateCocoaTreeNodeCache: Number of elements in data source does not equal the number in the node_cache.");
	}

	int i = 0;
	for(IupCocoaTreeItem* a_item in flattened_array_of_items)
	{
		IupCocoaTreeItem* node_cache_item = (IupCocoaTreeItem*)ih->data->node_cache[i].node_handle;
		if(![a_item isEqual:node_cache_item])
		{
			found_error = true;
		}
		i++;
	}

	if(found_error)
	{
		i = 0;
		NSLog(@"INTERNAL ERROR in testValidateCocoaTreeNodeCache: Arrays do not match");
		NSLog(@"DataSource:");
		for(IupCocoaTreeItem* a_item in flattened_array_of_items)
		{
			NSLog(@"DataSource: [%d] = %@", i, [a_item title]);
			i++;
		}
		NSLog(@"node_cache:");
		for(i=0; i<ih->data->node_count;i++)
		{
			IupCocoaTreeItem* node_cache_item = (IupCocoaTreeItem*)ih->data->node_cache[i].node_handle;
			NSLog(@"node_cache: [%d] = %@", i, [node_cache_item title]);
		}
		
		NSLog(@"Interleaved:");
		i = 0;
		for(IupCocoaTreeItem* a_item in flattened_array_of_items)
		{
			NSLog(@"DataSource: [%d] = %@ [%p]", i, [a_item title], a_item);
			IupCocoaTreeItem* node_cache_item = (IupCocoaTreeItem*)ih->data->node_cache[i].node_handle;
			NSLog(@"node_cache: [%d] = %@ [%p]", i, [node_cache_item title], node_cache_item);
			i++;
		}

		NSCAssert(0, @"INTERNAL ERROR in testValidateCocoaTreeNodeCache: Arrays do not match");
	}
	else
	{
		NSLog(@"testValidateCocoaTreeNodeCache passed.");
	}
	

	return !found_error;
}

#endif

// iupTreeCopyMoveCache doesn't work for Cocoa.
// I think the main problem is NSOutlineView allows the user to insert into positions that IUP does not support.
// There are ambiguities about whether something is added as a sibling or child.
// IUP overspecifies the UI interaction rules which don't apply to NSOutlineView.
// And NSOutlineView provides UI that does distinguish between sibling or child and the user has fine control over where it is dropped.
// So I think the problem is that we cannot express this in terms of IUP.
// Additionally, iupTreeCopyMoveCache seems to imply I copied the nodes, and then will delete.
// But Cocoa directly moved, so there is another impedience mismatch.
//
// (The bigger problem I think is we shouldn't have a node_cache.
// "The two biggest problems in computer science are naming things and cache invalidation."
// Because Cocoa already has two representations (the NSOutlineView and the data source delegate),
// with a different internal layout than the third IUP node_cache, it is really hard to keep these in sync.
// But for now, we are stuck with the node_cache.
// So I need to create my own variants of iupTreeCopyMoveCache to properly update the node_cache so it stays in sync with
// how Cocoa allows the user to manipulate the NSOutlineView.)
static void iupCocoaTreeMoveCache(Ihandle* ih, int flat_index_before, int flat_index_after, int count_of_nodes_to_move)
{
	// I need to rearrange the internal IUP node_cache array.
	// Just in case the table is huge and we are moving a huge number of elements, I don't want to blow the stack.
	// TODO: The node_cache does seem to have a notion of extra elements beyond the ones in use, which we might be able to use, but I don't know the internal use of it well enough to be sure I won't break other assumptions by growing it and using it.
	bool is_malloc = false;
	InodeData* tmp_array = NULL;
	if(count_of_nodes_to_move > 128) // 128 is a number I pulled out of the air
	{
		is_malloc = false;
		tmp_array = (InodeData*)alloca(count_of_nodes_to_move*sizeof(InodeData));
	}
	else
	{
		is_malloc = true;
		tmp_array = (InodeData*)malloc(count_of_nodes_to_move*sizeof(InodeData));
	}

	/*
		Exmaple 1a: Let's move 7,8,9 to go after 4 (move higher address to lower)
		node_cache: [0 1 2 3 4 5 6 7 8 9]
	 
	 	1) We need to copy 7,8,9 to the tmp_array to save the original values
	 	tmp_array: [7 8 9]
	 
	 	2) memmove items after 4 and before 7,8,9 to the right by the number of items to move (3). (*Edge Case: Do we need a bounds check?)
	 	node_cache: [0 1 2 3 4 5 6 7 5 6]
	 
	 	3) copy 7 8 9 after 4
	 	node_cache: [0 1 2 3 4 7 8 9 5 6]
	 
	 
		Exmaple 1b: Let's move 5,6,7 to go after 0 (move higher address to lower)
		node_cache: [0 1 2 3 4 5 6 7 8 9]
	 
	 	1) We need to copy 5,6,7 to the tmp_array to save the original values
	 	tmp_array: [5 6 7]
	 
	 	2) memmove items after 0 and before 5,6,7 to the right by the number of items to move (3)
	 	node_cache: [0 1 2 3 1 2 3 4 8 9]
	 
	 	3) copy 5,6,7 after the 0
	 	node_cache: [0 5 6 7 1 2 3 4 8 9]
	 
	 
		Exmaple 2a: Let's move 0,1,2 to go after 9 (move lower address to higher)
		node_cache: [0 1 2 3 4 5 6 7 8 9]
	 
	 	1) We need to copy 0,1,2 to the tmp_array to save the original values
	 	tmp_array: [0 1 2]
	 
	 	2) memmove everything left the number of elements being moved (3)
	 	node_cache: [3 4 5 6 7 8 9 7 8 9]
	 
	 	3) copy 0,1,2 after the 7 8 9
	 	node_cache: [3 4 5 6 7 8 9 0 1 2]
	 
	 
		Exmaple 2b: Let's move 1,2,3 to go after 5 (move lower address to higher)
		node_cache: [0 1 2 3 4 5 6 7 8 9]
	 
	 	1) We need to copy 1,2,3 to the tmp_array to save the original values
	 	tmp_array: [1 2 3]
	 
	 	2) memmove everything before the 6 and after the 1,2,3 to the left, the number of elements being moved (3)
	 	node_cache: [0 4 5 3 4 5 6 7 8 9]
	 
	 	3) copy 1,2,3 after the 5
	 	node_cache: [0 4 5 1 2 3 6 7 8 9]


		Exmaple 2c: Let's move 1,2,3,4 to go after 7 (move lower address to higher)
		node_cache: [0 1 2 3 4 5 6 7 8 9]
	 
	 	1) We need to copy 1,2,3,4 to the tmp_array to save the original values
	 	tmp_array: [1 2 3 4]
	 
	 	2) memmove everything after the 1,2,3,4 and before the 8 to the left, the number of elements being moved (3)
	 	node_cache: [0 X X X X 5 6 7 Y Y]
	 	node_cache: [0 5 6 7 4 5 6 7 8 9]

	 	3) copy 1,2,3,4 after the newly moved 7 (i.e. end of the just moved ranged)
	 	node_cache: [0 5 6 7 1 2 3 4 8 9]
	 
	 	Algorithm:
	 	- If we are moving from a higher address to a lower address,
	 		- Copy the elements we want to move to tmp_array
	 		- move all elements that are right of target point but before our moving elements to the right, the number of positions being moved
		 	- Copy back the elements from the tmp_array to the target position
	 	- Else if we are moving from a lower address to a higher address,
	 		- Copy the elements we want to move to tmp_array
	 		- move all elements that are right of our start elements but before the target point to the left, the number of positions being moved
	 		- Copy back the elements from the tmp_array to the specified target position
	*/
	
#if 0
		NSLog(@"node_count %d", ih->data->node_count);
		for(size_t j=0; j<ih->data->node_count; j++)
		{
			NSLog(@"bef: node_cache[%zu]=0x%p, %@", j, &ih->data->node_cache[j], [(IupCocoaTreeItem*)(ih->data->node_cache[j].node_handle) title]);
		}
#endif


	if(flat_index_before > flat_index_after) // moving from a higher address to a lower
	{
//		NSLog(@"moving from a higher address to a lower");

		memcpy(tmp_array, ih->data->node_cache+flat_index_before, count_of_nodes_to_move*sizeof(InodeData));

		int number_of_nodes_to_move_over = flat_index_before - flat_index_after;
//		NSLog(@"number_of_nodes_to_move_over:%d", number_of_nodes_to_move_over);

		memmove(ih->data->node_cache+flat_index_after+count_of_nodes_to_move, ih->data->node_cache+flat_index_after, number_of_nodes_to_move_over*sizeof(InodeData));
		memcpy(ih->data->node_cache+(flat_index_after), tmp_array, count_of_nodes_to_move*sizeof(InodeData));
	}
	else // moving from a lower address to a higher
	{
//		NSLog(@"moving from a lower address to a higher");

		memcpy(tmp_array, ih->data->node_cache+flat_index_before, count_of_nodes_to_move*sizeof(InodeData));

		int number_of_nodes_to_move_over = flat_index_after - flat_index_before;
//		NSLog(@"number_of_nodes_to_move_over:%d", number_of_nodes_to_move_over);

		memmove(ih->data->node_cache+flat_index_before, ih->data->node_cache+flat_index_before+count_of_nodes_to_move, number_of_nodes_to_move_over*sizeof(InodeData));
		memcpy(ih->data->node_cache+(flat_index_before+number_of_nodes_to_move_over), tmp_array, count_of_nodes_to_move*sizeof(InodeData));
	}


	if(is_malloc)
	{
		free(tmp_array);
	}
	tmp_array = NULL;

#if 0
	for(size_t j=0; j<ih->data->node_count; j++)
	{
		NSLog(@"aft: node_cache[%zu]=0x%p, %@", j, &ih->data->node_cache[j], [(IupCocoaTreeItem*)(ih->data->node_cache[j].node_handle) title]);
	}
#endif

	iupAttribSet(ih, "LASTADDNODE", NULL);

}


// iupTreeCopyMoveCache doesn't work for Cocoa.
// I think the main problem is NSOutlineView allows the user to insert into positions that IUP does not support.
// There are ambiguities about whether something is added as a sibling or child.
// IUP overspecifies the UI interaction rules which don't apply to NSOutlineView.
// And NSOutlineView provides UI that does distinguish between sibling or child and the user has fine control over where it is dropped.
// So I think the problem is that we cannot express this in terms of IUP.
// Additionally, iupTreeCopyMoveCache seems to imply I copied the nodes, and then will delete.
// But Cocoa directly moved, so there is another impedience mismatch.
//
// (The bigger problem I think is we shouldn't have a node_cache.
// "The two biggest problems in computer science are naming things and cache invalidation."
// Because Cocoa already has two representations (the NSOutlineView and the data source delegate),
// with a different internal layout than the third IUP node_cache, it is really hard to keep these in sync.
// But for now, we are stuck with the node_cache.
// So I need to create my own variants of iupTreeCopyMoveCache to properly update the node_cache so it stays in sync with
// how Cocoa allows the user to manipulate the NSOutlineView.)
static void iupCocoaTreeCopyCache(Ihandle* ih, int flat_index_source, int flat_index_target, int count_of_nodes_to_copy, IupCocoaTreeItem* new_copy_tree_item)
{
	/*
		Exmaple 1a: Let's copy 7,8,9 to go after 4 (copy higher address to lower)
		node_cache: [0 1 2 3 4 5 6 7 8 9]
	 
	 	1) Increase the size of the array by the number of elements to copy
	 	node_cache:  [0 1 2 3 4 5 6 7 8 9 X X X]
	 
	 	2) memmove items after 4 to the right by the number of elements to copy
	 	node_cache: [0 1 2 3 4 5 6 7 5 6 7 8 9]
	 
	 	3) copy 7 8 9 which is at its (original position + number of elements to copy), to after 4
	 	node_cache: [0 1 2 3 4 7 8 9 5 6 7 8 9]
	 
	 
		Exmaple 1b: Let's copy 5,6,7 to go after 0 (copy higher address to lower)
		node_cache: [0 1 2 3 4 5 6 7 8 9]
	 
	 	1) Increase the size of the array by the number of elements to copy
	 	node_cache: [0 1 2 3 4 5 6 7 8 9 X X X]

	 	2) memmove items after 0 to the right by the number of elements to copy
	 	node_cache: [0 1 2 3 1 2 3 4 5 6 7 8 9]
	 
	 	3) copy 5,6,7 which is at its (original position + number of elements to copy), to after the 0
	 	node_cache: [0 5 6 7 1 2 3 4 5 6 7 8 9]

	 
		Exmaple 2a: Let's copy 0,1,2 to go after 9 (copy lower address to higher)
		node_cache: [0 1 2 3 4 5 6 7 8 9]
	 
	 	1) Increase the size of the array by the number of elements to copy
	 	node_cache: [0 1 2 3 4 5 6 7 8 9 X X X]
	 
	 	2) memmove items after 9 to the right by the number of elements to copy. (*Edge Case: We don't have any items after 9)
	 	node_cache: [0 1 2 3 4 5 6 7 8 9 X X X]
	 
	 	3) copy 0,1,2 which is at its (original position), to after the 9
	 	node_cache: [0 1 2 3 4 5 6 7 8 9 0 1 2]

	 
		Exmaple 2b: Let's copy 1,2,3 to go after 5 (copy lower address to higher)
		node_cache: [0 1 2 3 4 5 6 7 8 9]
	 
	 	1) Increase the size of the array by the number of elements to copy
	 	node_cache: [0 1 2 3 4 5 6 7 8 9 X X X]
	 
	 	2) memmove items after 5 to the right by the number of elements to copy.
	 	node_cache: [0 1 2 3 4 5 6 7 8 6 7 8 9]

		3) copy 0,1,2 which is at its (original position), to after the 5
	 	node_cache: [0 1 2 3 4 5 1 2 3 6 7 8 9]


		Exmaple 2c: Let's copy 1,2,3,4 to go after 7 (copy lower address to higher)
		node_cache: [0 1 2 3 4 5 6 7 8 9]
	 
	 	1) Increase the size of the array by the number of elements to copy
	 	node_cache: [0 1 2 3 4 5 6 7 8 9 X X X X]
	 
	 	2) memmove items after 5 to the right by the number of elements to copy.
	 	node_cache: [0 1 2 3 4 5 6 7 8 9 6 7 8 9]

		3) copy 0,1,2,4 which is at its (original position), to after the 7
	 	node_cache: [0 1 2 3 4 5 6 7 1 2 3 4 8 9]

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
	
#if 0
		NSLog(@"node_count %d", ih->data->node_count);
		for(size_t j=0; j<ih->data->node_count; j++)
		{
			NSLog(@"bef: node_cache[%zu]=0x%p, %@", j, &ih->data->node_cache[j], [(IupCocoaTreeItem*)(ih->data->node_cache[j].node_handle) title]);
		}
#endif

	int original_node_count = ih->data->node_count;
	// We must change the node_count and let IUP resize its array as needed.
	ih->data->node_count += count_of_nodes_to_copy;
	// iupTreeIncCacheMem expects that the node_count contains the new size
	iupTreeIncCacheMem(ih);

	
//	NSLog(@"flat_index_source: %d", flat_index_source);
//	NSLog(@"flat_index_target: %d", flat_index_target);


	if(flat_index_source > flat_index_target) // copying from a higher address to a lower
	{
//		NSLog(@"copying from a higher address to a lower");
		int number_of_nodes_to_move_over = (original_node_count-1) - flat_index_target + 1; // we add +1 because we are inserting before the target
//		NSLog(@"number_of_nodes_to_move_over: %d", number_of_nodes_to_move_over);
	
//		NSLog(@"bef: node_cache[%d] %@", flat_index_target+count_of_nodes_to_copy,[(IupCocoaTreeItem*)(ih->data->node_cache[flat_index_target+count_of_nodes_to_copy].node_handle) title]);
//		NSLog(@"bef: node_cache[%d] %@", flat_index_target,[(IupCocoaTreeItem*)(ih->data->node_cache[flat_index_target].node_handle) title]);
	
		memmove(ih->data->node_cache+flat_index_target+count_of_nodes_to_copy, ih->data->node_cache+flat_index_target, number_of_nodes_to_move_over*sizeof(InodeData));

//		NSLog(@"mid: node_cache[%d] %@", flat_index_target,[(IupCocoaTreeItem*)(ih->data->node_cache[flat_index_target].node_handle) title]);
//		NSLog(@"mid: node_cache[%d] %@", flat_index_source+count_of_nodes_to_copy,[(IupCocoaTreeItem*)(ih->data->node_cache[flat_index_source+count_of_nodes_to_copy].node_handle) title]);
	
		memmove(ih->data->node_cache+flat_index_target, ih->data->node_cache+flat_index_source+count_of_nodes_to_copy, count_of_nodes_to_copy*sizeof(InodeData));


	}
	else // copying from a lower address to a higher
	{
//		NSLog(@"copying from a lower address to a higher");
		int number_of_nodes_to_move_over = (original_node_count-1) - flat_index_source;
//		NSLog(@"number_of_nodes_to_move_over: %d", number_of_nodes_to_move_over);
	
		memmove(ih->data->node_cache+flat_index_target+count_of_nodes_to_copy, ih->data->node_cache+flat_index_target, number_of_nodes_to_move_over*sizeof(InodeData));
		memmove(ih->data->node_cache+flat_index_target, ih->data->node_cache+flat_index_source, count_of_nodes_to_copy*sizeof(InodeData));

	}

	// The mem-copies above created new nodes in the node_cache for our insertion, but their pointers to the node_handle point to the original objects instead of the new copies of the objects.
	// So we need to go through those items, and change their node_handle pointers to point to the new copies of each object.
	NSArray<IupCocoaTreeItem*>* flattened_array_of_new_items = iupCocoaTreeCreateFlatItemArray(new_copy_tree_item);
	int k=flat_index_target;
	for(IupCocoaTreeItem* a_item in flattened_array_of_new_items)
	{
//		NSLog(@"Fix Pointers: old:%p, new:%p\n%@\n%@\n", ih->data->node_cache[k].node_handle, a_item, [(IupCocoaTreeItem*)ih->data->node_cache[k].node_handle title], [(IupCocoaTreeItem*)a_item title]);
		ih->data->node_cache[k].node_handle = a_item;
		k++;
	}
	


#if 0
	for(size_t j=0; j<ih->data->node_count; j++)
	{
		NSLog(@"aft: node_cache[%zu]=0x%p, %@", j, &ih->data->node_cache[j], [(IupCocoaTreeItem*)(ih->data->node_cache[j].node_handle) title]);
	}
#endif

	iupAttribSet(ih, "LASTADDNODE", NULL);

}

// iupTreeCopyMoveCache doesn't work for Cocoa.
// I think the main problem is NSOutlineView allows the user to insert into positions that IUP does not support.
// There are ambiguities about whether something is added as a sibling or child.
// IUP overspecifies the UI interaction rules which don't apply to NSOutlineView.
// And NSOutlineView provides UI that does distinguish between sibling or child and the user has fine control over where it is dropped.
// So I think the problem is that we cannot express this in terms of IUP.
// Additionally, iupTreeCopyMoveCache seems to imply I copied the nodes, and then will delete.
// But Cocoa directly moved, so there is another impedience mismatch.
//
// (The bigger problem I think is we shouldn't have a node_cache.
// "The two biggest problems in computer science are naming things and cache invalidation."
// Because Cocoa already has two representations (the NSOutlineView and the data source delegate),
// with a different internal layout than the third IUP node_cache, it is really hard to keep these in sync.
// But for now, we are stuck with the node_cache.
// So I need to create my own variants of iupTreeCopyMoveCache to properly update the node_cache so it stays in sync with
// how Cocoa allows the user to manipulate the NSOutlineView.)
static void iupCocoaTreeCrossInsertCache(Ihandle* source_ih, Ihandle* target_ih, int flat_index_source, int flat_index_target, int count_of_nodes_to_copy, IupCocoaTreeItem* new_copy_tree_item)
{

#if 0
		NSLog(@"node_count %d", ih->data->node_count);
		for(size_t j=0; j<target_ih->data->node_count; j++)
		{
			NSLog(@"bef: node_cache[%zu]=0x%p, %@", j, &target_ih->data->node_cache[j], [(IupCocoaTreeItem*)(target_ih->data->node_cache[j].node_handle) title]);
		}
#endif

	int original_node_count = target_ih->data->node_count;
	// We must change the node_count and let IUP resize its array as needed.
	target_ih->data->node_count += count_of_nodes_to_copy;
	// iupTreeIncCacheMem expects that the node_count contains the new size
	iupTreeIncCacheMem(target_ih);

	
//	NSLog(@"flat_index_target: %d", flat_index_target);

	


	int number_of_nodes_to_move_over = (original_node_count-1) - flat_index_target + 1; // we add +1 because we are inserting before the target
//	NSLog(@"number_of_nodes_to_move_over: %d", number_of_nodes_to_move_over);
	
	memmove(target_ih->data->node_cache+flat_index_target+count_of_nodes_to_copy, target_ih->data->node_cache+flat_index_target, number_of_nodes_to_move_over*sizeof(InodeData));
	
	memcpy(target_ih->data->node_cache+flat_index_target, source_ih->data->node_cache+flat_index_source, count_of_nodes_to_copy*sizeof(InodeData));


	// The mem-copies above created new nodes in the node_cache for our insertion, but their pointers to the node_handle point to the original objects instead of the new copies of the objects.
	// So we need to go through those items, and change their node_handle pointers to point to the new copies of each object.
	NSArray<IupCocoaTreeItem*>* flattened_array_of_new_items = iupCocoaTreeCreateFlatItemArray(new_copy_tree_item);
	int k=flat_index_target;
	for(IupCocoaTreeItem* a_item in flattened_array_of_new_items)
	{
//		NSLog(@"Fix Pointers: old:%p, new:%p\n%@\n%@\n", target_ih->data->node_cache[k].node_handle, a_item, [(IupCocoaTreeItem*)target_ih->data->node_cache[k].node_handle title], [(IupCocoaTreeItem*)a_item title]);
		target_ih->data->node_cache[k].node_handle = a_item;
		k++;
	}
	


#if 0
	for(size_t j=0; j<target_ih->data->node_count; j++)
	{
		NSLog(@"aft: node_cache[%zu]=0x%p, %@", j, &target_ih->data->node_cache[j], [(IupCocoaTreeItem*)(target_ih->data->node_cache[j].node_handle) title]);
	}
#endif

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
	
	// These conversions don't work fully because of the sibling/child ambiguity.
//	int id_dst = iupTreeFindNodeId(ih, (InodeHandle*)parent_target_tree_item) + (int)target_child_index;
//	int id_src = iupTreeFindNodeId(ih, (InodeHandle*)tree_item);
//	int id_new = id_dst+1; /* contains the position for a copy operation */
	
	
//	int count_of_all_nodes = (int)Helper_CountAllItems(data_source_delegate);
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
	
	// IUP doc:
	// If the destination node is a branch and it is expanded,
	// then the specified node is inserted as the first child of the destination node.
	// If the branch is not expanded or the destination node is a leaf,
	// then it is inserted as the next brother of the leaf.
	
	// Me: I don't think this works for Cocoa because the NSOutlineView allows the user direct control over insertion points that distinguish between sibling and child.
	// I don't think it is possible to conform to the IUP doc, nor do I think it would be a good idea even if we could because it would break native behavior users expect on Cocoa.
	
	
	NSInteger adjusted_index = target_child_index;

	// NSLog(@"parent_tree: %@, parent_target: %@", [parent_tree_item title], [parent_target_tree_item title]);

	// If the node is being moved under the same immediate parent,
	// we need to subtract 1 if
	// the current placement of the node is earlier than the target.
	// because the node's current placement counts against us
	// and target_child_index is +1 too much.
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

//NSLog(@"adjusted_index: %d, target_child_index: %d, object_index: %d", adjusted_index, target_child_index, object_index);
//NSLog(@"tree_item: %@", [tree_item title]);
//NSLog(@"parent_target_tree_item: %@", [parent_target_tree_item title]);
//NSLog(@"parent_tree_item: %@", [parent_tree_item title]);

	// Quick exit if the user didn't actually change the location
	if( (object_index == adjusted_index)
		&& [parent_target_tree_item isEqual:parent_tree_item]
	)
	{
		return;
	}


	NSInteger flat_index_before = 0;
	// This will get an index that is compatible with the node_cache
	bool is_found = Helper_FindFlatIndexofTreeItemInOutlineView(data_source_delegate, tree_item, &flat_index_before);
//	NSLog(@"flat_index_before: %d, is_found: %d", flat_index_before, is_found);
	NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");

#if 0
		{
			NSArray* testarray = testValidateCocoaTreeCreateFlatItemArray(data_source_delegate);
			int k=0;
			for(IupCocoaTreeItem* a_item in testarray)
			{
				NSLog(@"bef DataSource: [%d] = %@", k, [a_item title]);
				k++;
			}
		}
#endif

	// update the data source
	[data_source_delegate moveItem:tree_item targetParent:parent_target_tree_item targetChildIndex:adjusted_index];
	[outline_view moveItemAtIndex:object_index inParent:parent_tree_item toIndex:adjusted_index inParent:parent_target_tree_item];

#if 0
		{
			NSArray* testarray = testValidateCocoaTreeCreateFlatItemArray(data_source_delegate);
			int k=0;
			for(IupCocoaTreeItem* a_item in testarray)
			{
				NSLog(@"aft DataSource: [%d] = %@", k, [a_item title]);
				k++;
			}
		}
#endif

	NSInteger flat_index_after = 0;
	// This will get an index that is compatible with the node_cache
	is_found = Helper_FindFlatIndexofTreeItemInOutlineView(data_source_delegate, tree_item, &flat_index_after);
//	NSLog(@"flat_index_after: %d, is_found: %d", flat_index_after, is_found);
	NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");


	iupCocoaTreeMoveCache(ih, (int)flat_index_before, (int)flat_index_after, count_of_nodes_to_move);
	

#if IUPCOCOA_TREE_TEST_VALIDATE_NODE_CACHE
	testValidateCocoaTreeNodeCache(ih, data_source_delegate);
#endif

}

static void helperExpandedItemArrayRecursive(IupCocoaTreeItem* original_item, NSOutlineView* original_tree, NSMutableArray<NSNumber*>* expanded_info_array)
{
	if([original_tree isExpandable:original_item])
	{
		// BUG: Apple seems to return NO if a parent is collapsed, even if the actual item is expanded.
		// But when Apple expands the parent, it remembers and restores the child state.
		// This is a problem for us because we are trying to copy the state to apply to new copies of the node.
		// I don't know how to workaround this.
		// Tested 10.13.6
		BOOL is_expanded = [original_tree isItemExpanded:original_item];
		[expanded_info_array addObject:[NSNumber numberWithBool:is_expanded]];
//		NSLog(@"%@ expand state is: %d", [original_item title], is_expanded);
	}
	else
	{
		[expanded_info_array addObject:[NSNumber numberWithBool:NO]];
//		NSLog(@"%@ expand state is: %d", [original_item title], 0);
	}
	
	NSArray* original_children_array = [original_item childrenArray];
//	NSUInteger array_count = [original_children_array count];

	for(IupCocoaTreeItem* a_item in original_children_array)
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
//		NSLog(@"[%lu] %@ expand state is: %d", (unsigned long)*in_out_current_index, [copy_item title], [is_expandable boolValue]);
		if([is_expandable boolValue])
		{
			[copy_tree expandItem:copy_item];
		}
		else
		{
			[copy_tree collapseItem:copy_item];
		}
	}
	else
	{
//		NSLog(@"[%lu] %@ expand state is: %d", (unsigned long)*in_out_current_index, [copy_item title], 0);
	}

	*in_out_current_index = *in_out_current_index+1;

	NSArray* copy_children_array = [copy_item childrenArray];
	NSUInteger array_count = [copy_children_array count];

	for(NSInteger i=0; i<array_count; i++)
	{
		IupCocoaTreeItem* next_copy_item = [copy_children_array objectAtIndex:i];
		helperSyncExpandedItemsRecursive(in_out_current_index, expanded_info_array, next_copy_item, copy_tree);
	}
}
static void helperSyncExpandedItems(NSArray<NSNumber*>* expanded_info_array, IupCocoaTreeItem* copy_item, NSOutlineView* copy_tree)
{
	NSUInteger current_index = 0;
	helperSyncExpandedItemsRecursive(&current_index, expanded_info_array, copy_item, copy_tree);
}

// tree_item should be a copy
static void helperCopyAndInsertNode(IupCocoaOutlineView* outline_view, IupCocoaTreeItem* tree_item, IupCocoaTreeItem* parent_target_tree_item, NSInteger target_child_index, NSTableViewAnimationOptions copy_insert_animation)
{
	if(!tree_item)
	{
		return;
	}
	IupCocoaOutlineView* iup_outline_view = (IupCocoaOutlineView*)outline_view;
	IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
	Ihandle* ih = [iup_outline_view ih];
	
//	int id_src = iupTreeFindNodeId(ih, (InodeHandle*)parent_target_tree_item) + (int)target_child_index;
//	int id_dst = iupTreeFindNodeId(ih, (InodeHandle*)tree_item);
//	int id_new = id_dst+1; /* contains the position for a copy operation */
//	int count_of_all_nodes = (int)Helper_CountAllItems(data_source_delegate);

	
	
	//IupCocoaTreeItem* new_copy_tree_item = [[IupCocoaTreeItem alloc] init];
	IupCocoaTreeItem* new_copy_tree_item = [tree_item cloneWithNewParentItem:parent_target_tree_item ihandle:ih];
		// If the destination node is a branch and it is expanded,
	// then the specified node is inserted as the first child of the destination node.
	// If the branch is not expanded or the destination node is a leaf,
	// then it is inserted as the next brother of the leaf.
	
//	BOOL is_target_expanded = NO;
//	int target_kind = [target_tree_item kind];
	
	// Unlike move, we don't seem to need to subtract 1.
//	NSInteger adjusted_index = target_child_index - 1;
	NSInteger adjusted_index = target_child_index;
	if(adjusted_index < 0)
	{
		adjusted_index = 0;
	}
	
	
	// Save the expanded information for the tree_item so we can later apply it to the copies.
	// We need to do this now because if we insert a copy into the tree_item as a child, they will be out-of-sync.
	NSArray<NSNumber*>* array_of_expanded_info = helperExpandedItemArray(tree_item, outline_view);
	
	NSInteger flat_index_source = 0;
	// This will get an index that is compatible with the node_cache
	bool is_found = Helper_FindFlatIndexofTreeItemInOutlineView(data_source_delegate, tree_item, &flat_index_source);
//	NSLog(@"flat_index_before: %d, is_found: %d", flat_index_before, is_found);
	NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");
	
	// update the data source
	[data_source_delegate insertChild:new_copy_tree_item withParent:parent_target_tree_item targetChildIndex:adjusted_index];

	// directly update the outlineview so we don't have to reloadData
	NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:adjusted_index];
	[outline_view insertItemsAtIndexes:index_set inParent:parent_target_tree_item withAnimation:copy_insert_animation];
	
	// Make the new copy match the expanded structure of the original since the Cocoa NSOutlineView is so visual and live-drags.
	// Anything else just looks wrong.
	// I think IUP has slightly different rules, but I think this is the right thing to do for Cocoa.
	helperSyncExpandedItems(array_of_expanded_info, new_copy_tree_item, outline_view);
	
	
	NSInteger flat_index_target = 0;
	// This will get an index that is compatible with the node_cache
	is_found = Helper_FindFlatIndexofTreeItemInOutlineView(data_source_delegate, new_copy_tree_item, &flat_index_target);
//	NSLog(@"flat_index_after: %d, is_found: %d", flat_index_after, is_found);
	NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");

	
	int count_of_nodes_to_copy = (int)Helper_RecursivelyCountItems(new_copy_tree_item);
	
//	iupTreeCopyMoveCache(ih, id_src, id_new, count_of_new_nodes, true);
	iupCocoaTreeCopyCache(ih, (int)flat_index_source, (int)flat_index_target, count_of_nodes_to_copy, new_copy_tree_item);

	
	
//	cocoaTreeRebuildNodeCache(ih, id_new, tree_item);
	
#if IUPCOCOA_TREE_TEST_VALIDATE_NODE_CACHE
	testValidateCocoaTreeNodeCache(ih, data_source_delegate);
#endif


}

// tree_item should be a copy
static void helperCrossCopyNode(NSOutlineView* source_outline_view, IupCocoaTreeItem* source_tree_item, NSOutlineView* target_outline_view, IupCocoaTreeItem* parent_target_tree_item, NSInteger target_child_index, NSTableViewAnimationOptions copy_insert_animation)
{
	if(!source_tree_item)
	{
		return;
	}
	IupCocoaOutlineView* iup_source_outline_view = (IupCocoaOutlineView*)source_outline_view;
	IupCocoaTreeDelegate* source_data_delegate = (IupCocoaTreeDelegate*)[source_outline_view dataSource];
	Ihandle* source_ih = [iup_source_outline_view ih];
	
	IupCocoaOutlineView* iup_target_outline_view = (IupCocoaOutlineView*)target_outline_view;
	IupCocoaTreeDelegate* target_data_delegate = (IupCocoaTreeDelegate*)[target_outline_view dataSource];
	Ihandle* target_ih = [iup_target_outline_view ih];
	
	
	//IupCocoaTreeItem* new_copy_tree_item = [[IupCocoaTreeItem alloc] init];
	IupCocoaTreeItem* new_copy_tree_item = [source_tree_item cloneWithNewParentItem:parent_target_tree_item ihandle:target_ih];
		// If the destination node is a branch and it is expanded,
	// then the specified node is inserted as the first child of the destination node.
	// If the branch is not expanded or the destination node is a leaf,
	// then it is inserted as the next brother of the leaf.
	
//	BOOL is_target_expanded = NO;
//	int target_kind = [target_tree_item kind];
	
	// Save the expanded information for the tree_item so we can later apply it to the copies.
	// We need to do this now because if we insert a copy into the tree_item as a child, they will be out-of-sync.
	NSArray<NSNumber*>* array_of_expanded_info = helperExpandedItemArray(source_tree_item, source_outline_view);
	
	
	// Special edge case: the target outlineview is completely empty.
	if((-1 == target_child_index)
		&& (nil == parent_target_tree_item)
		&& (0 == [[target_data_delegate treeRootTopLevelObjects] count])
	)
	{
		//  add the new node at root
		[target_data_delegate insertAtRoot:new_copy_tree_item];
//		[outline_view insertItemsAtIndexes:[NSIndexSet indexSetWithIndex:0] inParent:nil withAnimation:NSTableViewAnimationEffectNone];
		[iup_target_outline_view insertItemsAtIndexes:[NSIndexSet indexSetWithIndex:0] inParent:nil withAnimation:NSTableViewAnimationEffectGap];
	}
	else // normal case
	{
		
		// Unlike move, we don't seem to need to subtract 1.
	//	NSInteger adjusted_index = target_child_index - 1;
		NSInteger adjusted_index = target_child_index;
		if(adjusted_index < 0)
		{
			adjusted_index = 0;
		}
		
		// update the data source
		[target_data_delegate insertChild:new_copy_tree_item withParent:parent_target_tree_item targetChildIndex:adjusted_index];

		// directly update the outlineview so we don't have to reloadData
		NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:adjusted_index];
		[target_outline_view insertItemsAtIndexes:index_set inParent:parent_target_tree_item withAnimation:copy_insert_animation];
	
	}
	

//	NSInteger flat_index_source = 0;
	// This will get an index that is compatible with the node_cache
//	bool is_found = Helper_FindFlatIndexofTreeItemInOutlineView(data_source_delegate, tree_item, &flat_index_source);
//	NSLog(@"flat_index_before: %d, is_found: %d", flat_index_before, is_found);
//	NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");
	

	// Make the new copy match the expanded structure of the original since the Cocoa NSOutlineView is so visual and live-drags.
	// Anything else just looks wrong.
	// I think IUP has slightly different rules, but I think this is the right thing to do for Cocoa.
	helperSyncExpandedItems(array_of_expanded_info, new_copy_tree_item, target_outline_view);
	
	
	NSInteger flat_index_target = 0;
	// This will get an index that is compatible with the node_cache
	bool is_found = Helper_FindFlatIndexofTreeItemInOutlineView(target_data_delegate, new_copy_tree_item, &flat_index_target);
//	NSLog(@"flat_index_after: %d, is_found: %d", flat_index_after, is_found);
	NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");
	
	NSInteger flat_index_source = 0;
	// This will get an index that is compatible with the node_cache
	is_found = Helper_FindFlatIndexofTreeItemInOutlineView(source_data_delegate, source_tree_item, &flat_index_source);
//	NSLog(@"flat_index_after: %d, is_found: %d", flat_index_after, is_found);
	NSCAssert(is_found, @"Internal error: Could not find moved node in outline view");
	
	int count_of_nodes_to_copy = (int)Helper_RecursivelyCountItems(new_copy_tree_item);
	

	iupCocoaTreeCrossInsertCache(source_ih, target_ih, (int)flat_index_source, (int)flat_index_target, count_of_nodes_to_copy, new_copy_tree_item);

	
//	cocoaTreeRebuildNodeCache(ih, id_new, tree_item);
	
#if IUPCOCOA_TREE_TEST_VALIDATE_NODE_CACHE
	testValidateCocoaTreeNodeCache(target_ih, target_data_delegate);
#endif


}

static IupCocoaTreeItem* helperRecursiveIsPointerValid(intptr_t look_for_pointer, IupCocoaTreeItem* current_item)
{
	if(look_for_pointer == (intptr_t)current_item)
	{
		return current_item;
	}
	
	for(IupCocoaTreeItem* a_item in [current_item childrenArray])
	{
		IupCocoaTreeItem* is_found = helperRecursiveIsPointerValid(look_for_pointer, a_item);
		if(is_found)
		{
			return is_found;
		}
	}
	return NULL;
}

static IupCocoaTreeItem* helperIsPointerValid(intptr_t look_for_pointer, IupCocoaTreeDelegate* tree_delegate)
{
	for(IupCocoaTreeItem* tree_item in [tree_delegate treeRootTopLevelObjects])
	{
		IupCocoaTreeItem* is_found = helperRecursiveIsPointerValid(look_for_pointer, tree_item);
		if(is_found)
		{
			return is_found;
		}
	}
	return NULL;
}

// need forward declaration
static void cocoaTreeRemoveNodeData(Ihandle* ih, IupCocoaTreeItem* tree_item, int call_cb);
- (BOOL) outlineView:(NSOutlineView*)outline_view acceptDrop:(id <NSDraggingInfo>)drag_info item:(id)parent_target_tree_item childIndex:(NSInteger)target_child_index
{
//NSLog(@"%@", NSStringFromSelector(_cmd));

	if([drag_info draggingSource] == outline_view)
	{
		[self setItemBeingDragged:nil];
		
		NSPasteboard* paste_board = [drag_info draggingPasteboard];
		NSData* data_value = [paste_board dataForType:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE];
		if(nil == data_value)
		{
			return NO;
		}
		
	//	NSValue* pointer_value = [NSKeyedUnarchiver unarchiveObjectWithData:data_value];
	//	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)[pointer_value pointerValue];

		intptr_t decoded_integer;
		[data_value getBytes:&decoded_integer length:sizeof(intptr_t)];

		// Could be a wild pointer at this point?
		// We can iterate through all the nodes to find it if we need to be extra safe.
	//	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)decoded_integer;
		IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
		IupCocoaTreeItem* tree_item = helperIsPointerValid(decoded_integer, data_source_delegate);
		if(nil == tree_item)
		{
			return NO;
		}


		// NOTE: Apple has this in their example. I still don't understand what this does.
		// If it was a drop "on", then we add it at the start
		if(target_child_index == NSOutlineViewDropOnItemIndex)
		{
			target_child_index = 0;
		}
		[outline_view beginUpdates];


		// Are we copying the data or moving something?
		if([drag_info draggingSourceOperationMask] == NSDragOperationCopy)
		{
			// Yes, this is an insert from the pasteboard (even if it is a copy of itemBeingDragged)
			helperCopyAndInsertNode((IupCocoaOutlineView*)outline_view, tree_item, parent_target_tree_item, target_child_index, NSTableViewAnimationEffectGap);
		}
		else
		{
			helperMoveNode((IupCocoaOutlineView*)outline_view, tree_item, parent_target_tree_item, target_child_index);
		}

		[outline_view endUpdates];

		return YES;
	
	}
	// For dragging between two different NSOutlineViews
	else
	{
		NSPasteboard* paste_board = [drag_info draggingPasteboard];
		NSData* data_value = [paste_board dataForType:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE];
		if(nil == data_value)
		{
			return NO;
		}
	
	//	NSValue* pointer_value = [NSKeyedUnarchiver unarchiveObjectWithData:data_value];
	//	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)[pointer_value pointerValue];

		intptr_t decoded_integer;
		[data_value getBytes:&decoded_integer length:sizeof(intptr_t)];


		// FIXME: Because I'm using raw pointers, this won't work for dragging between two different NSOutlineViews from two different IUP-based programs.
		// To fix this, I need to properly implement serialization/deserialization of IupCocoaTreeItem.
		IupCocoaTreeDragDropDelegate* source_data_source_delegate = (IupCocoaTreeDragDropDelegate*)[[drag_info draggingSource] dataSource];

		[self setItemBeingDragged:nil];
		[source_data_source_delegate setItemBeingDragged:nil];

		IupCocoaTreeItem* tree_item = helperIsPointerValid(decoded_integer, source_data_source_delegate);
		if(nil == tree_item)
		{
			return NO;
		}


/*
		// NOTE: Apple has this in their example. I still don't understand what this does.
		// If it was a drop "on", then we add it at the start
		// UPDATE: I need -1 to tell me when the user drags onto an empty outlineview.
		// NSOutlineViewDropOnItemIndex is -1.
		// So I am not going to use this code for now.
		// If I need it, then I need to do a check on the target data source to see if it is empty first.
		// parent_target_tree_item is also nil in this case.
		if(target_child_index == NSOutlineViewDropOnItemIndex)
		{
			target_child_index = 0;
		}
*/

		// Are we copying the data or moving something?
		if([drag_info draggingSourceOperationMask] == NSDragOperationCopy)
		{
			// Yes, this is an insert from the pasteboard (even if it is a copy of itemBeingDragged)
			[outline_view beginUpdates];
			helperCrossCopyNode([drag_info draggingSource], tree_item, outline_view, parent_target_tree_item, target_child_index, NSTableViewAnimationEffectGap);
			[outline_view endUpdates];
			
			
#if IUPCOCOA_TREE_TEST_VALIDATE_NODE_CACHE
	IupCocoaOutlineView* iup_outline_view = (IupCocoaOutlineView*)outline_view;
	IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
	Ihandle* ih = [iup_outline_view ih];
	testValidateCocoaTreeNodeCache(ih, data_source_delegate);
#endif
		}
		else
		{
			[outline_view beginUpdates];
//			helperCopyAndInsertNode((IupCocoaOutlineView*)outline_view, tree_item, parent_target_tree_item, target_child_index, NSTableViewAnimationEffectGap);
			helperCrossCopyNode([drag_info draggingSource], tree_item, outline_view, parent_target_tree_item, target_child_index, NSTableViewAnimationEffectGap);
			[outline_view endUpdates];

			// remove node from source outlineview
			NSOutlineView* source_outline_view = [drag_info draggingSource];
			IupCocoaOutlineView* source_iup_outline_view = (IupCocoaOutlineView*)source_outline_view;
			Ihandle* source_ih = [source_iup_outline_view ih];

			[source_iup_outline_view beginUpdates];
	
			cocoaTreeRemoveNodeData(source_ih, tree_item, 0);
			NSUInteger target_index = [source_data_source_delegate removeItem:tree_item];
			NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:target_index];
//			[source_iup_outline_view removeItemsAtIndexes:index_set inParent:[tree_item parentItem] withAnimation:NSTableViewAnimationEffectNone];
//			[source_iup_outline_view removeItemsAtIndexes:index_set inParent:[tree_item parentItem] withAnimation:NSTableViewAnimationEffectFade];
			[source_iup_outline_view removeItemsAtIndexes:index_set inParent:[tree_item parentItem] withAnimation:NSTableViewAnimationEffectGap];

			// removeItem: doesn't seem to update. Need to call reloadData.
//			[source_iup_outline_view reloadData];

			[source_iup_outline_view endUpdates];
			

#if IUPCOCOA_TREE_TEST_VALIDATE_NODE_CACHE
	testValidateCocoaTreeNodeCache(source_ih, source_data_source_delegate);
	
	IupCocoaOutlineView* target_iup_outline_view = (IupCocoaOutlineView*)outline_view;
	IupCocoaTreeDelegate* target_data_source_delegate = (IupCocoaTreeDelegate*)[target_iup_outline_view dataSource];
	Ihandle* target_ih = [target_iup_outline_view ih];
	testValidateCocoaTreeNodeCache(target_ih, target_data_source_delegate);
#endif
		}


		return YES;
	
	}
}

@end // IupCocoaTreeDragDropDelegate

/*****************************************************************************/
/* ADDING ITEMS                                                              */
/*****************************************************************************/

static void cocoaTreeReloadItem(IupCocoaTreeItem* tree_item, NSOutlineView* outline_view)
{
	NSOperatingSystemVersion macosx_1012 = { 10, 12, 0 };
	
	// isOperatingSystemAtLeastVersion officially requires 10.10+, but seems available on 10.9
	if([[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:macosx_1012])
	{
		// Starting in 10.12, reloadItem: was fixed to do the right thing. Must link to 10.12 SDK or higher (which you should always link to the lastest on Mac anyway)
		[outline_view reloadItem:tree_item];
	}
	else
	{
//		[outline_view reloadData];
		NSInteger row_index = [outline_view rowForItem:tree_item];
		NSIndexSet* row_index_set = [NSIndexSet indexSetWithIndex:row_index];
		NSIndexSet* column_index_set = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [outline_view numberOfColumns])];
		[outline_view reloadDataForRowIndexes:row_index_set columnIndexes:column_index_set];
	}
}

void iupdrvTreeAddNode(Ihandle* ih, int prev_id, int kind, const char* title, int add)
{
/*
 
 id is the number identifier of a reference node, the reference node is used to position the new node.
 
 kind is the new node type, if it is a branch or a leaf.
 
 add means appending a node at the end of the branch, if 0 means inserting the node in the branch
 
 If the reference node exists then
 if (reference node is a branch and appending)
 insert the new node after the reference node, as first child
 else
 insert the new node after reference node
 else
 add the new node at root
*/
	NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
	IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
	
	

	
	
	InodeHandle* inode_prev = iupTreeGetNode(ih, prev_id);
	

	/* the previous node is not necessary only
	 if adding the root in an empty tree or before the root. */
	// UPDATE: This check is breaking things for me.
/*
	if (!inode_prev && prev_id!=-1)
	{
		return;
	}
*/

	if (!title)
	{
		title = "";
	}
	

	
	IupCocoaTreeItem* tree_item_new = [[IupCocoaTreeItem alloc] init];
	[tree_item_new setKind:kind];
	NSString* ns_title = [NSString stringWithUTF8String:title];
	[tree_item_new setTitle:ns_title];
//	InodeHandle* inode_new = (InodeHandle*)calloc(1, sizeof(InodeHandle));
	InodeHandle* inode_new = (InodeHandle*)tree_item_new; // NOTE: retain count is 1 from alloc. We are not going to retain it again.
	
	//  If the reference node exists then
	if(inode_prev)
	{
		IupCocoaTreeItem* tree_item_prev = inode_prev;
		int kind_prev = [tree_item_prev kind];
	
		
		// if (reference node is a branch and appending)
		if((ITREE_BRANCH == kind_prev) && (1 == add))
		{
			// insert the new node after the reference node, as first child
			/* depth+1 */
			// IUP always inserts the child in the first position, not the last
			// update the data source
			NSUInteger target_index = [data_source_delegate insertChild:tree_item_new withParent:tree_item_prev];

			// directly update the outlineview so we don't have to reloadData
			NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:target_index];
//			[outline_view insertItemsAtIndexes:index_set inParent:tree_item_prev withAnimation:NSTableViewAnimationEffectNone];
			[outline_view insertItemsAtIndexes:index_set inParent:tree_item_prev withAnimation:NSTableViewAnimationEffectGap];

		}
		else
		{
			// insert the new node after reference node
			/* same depth */
			
			// update the data source
			NSUInteger target_index = [data_source_delegate insertPeer:tree_item_new withSibling:tree_item_prev];

			// directly update the outlineview so we don't have to reloadData
			// It is okay if the parent is nil. This also handles the case where ADDROOT=NO and we are adding another top-level node.
			IupCocoaTreeItem* tree_item_parent = [tree_item_prev parentItem];
			NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:target_index];
//			[outline_view insertItemsAtIndexes:index_set inParent:tree_item_parent withAnimation:NSTableViewAnimationEffectNone];
			[outline_view insertItemsAtIndexes:index_set inParent:tree_item_parent withAnimation:NSTableViewAnimationEffectGap];

		}

		iupTreeAddToCache(ih, add, kind_prev, inode_prev, inode_new);

	}
	else
	{
		//  add the new node at root
		[data_source_delegate insertAtRoot:tree_item_new];
		// directly update the outlineview so we don't have to reloadData
//		[outline_view insertItemsAtIndexes:[NSIndexSet indexSetWithIndex:0] inParent:nil withAnimation:NSTableViewAnimationEffectNone];
		[outline_view insertItemsAtIndexes:[NSIndexSet indexSetWithIndex:0] inParent:nil withAnimation:NSTableViewAnimationEffectGap];

		iupTreeAddToCache(ih, 0, 0, NULL, inode_new);

	}
	// We don't need to reloadData if we update the outline view directly.
//	[outline_view reloadData];

	if(ITREE_BRANCH == kind)
	{
		BOOL should_expand = IupGetInt(ih, "ADDEXPANDED");
		if(should_expand)
		{
			// Just in case we do have children already, expand now which may skip the animation delay.
			//[outline_view expandItem:tree_item_new];
			[outline_view expandItem:tree_item_new expandChildren:YES];
#if 0
			// Tricky: This wasn't working until I added dispatch_async.
			// I think the problem is that when I expand a branch, the children may not be added yet.
			// So if there are no children at the time, my expand request gets ignored.
			// The dispatch_async will force the expand to happen on the next event loop pass, after any children have been added from this loop.
			// So the expand will work now that the children exist.
			// UPDATE: This is now fixed by using
			// - (BOOL) outlineView:(NSOutlineView*)outline_view isItemExpandable:(id)the_item
			// and making it rely on "kind" to determine if expandable instead of number of children.
			dispatch_async(dispatch_get_main_queue(), ^{
//				[outline_view expandItem:tree_item_new];
				[outline_view expandItem:tree_item_new expandChildren:YES];
				}
			);
#endif
		}
	}
	
	// make sure to release since it should now be retained by the data_source_delegate
	[tree_item_new release];
	
	
#if IUPCOCOA_TREE_TEST_VALIDATE_NODE_CACHE
	testValidateCocoaTreeNodeCache(ih, data_source_delegate);
#endif
}



int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{

	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)node_handle;
//	NSUInteger number_of_items = [tree_item numberOfChildren];

	NSUInteger number_of_items = Helper_RecursivelyCountItems(tree_item);
	// subtract one because we don't include 'this' node as part of the children count
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


// FIXME: Why does the GTK version look so different?
void iupdrvTreeUpdateMarkMode(Ihandle *ih)
{
	NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
	if(ih->data->mark_mode==ITREE_MARK_MULTIPLE)
	{
		[outline_view setAllowsMultipleSelection:YES];
	}
	else
	{
		[outline_view setAllowsMultipleSelection:NO];
	}
}


// FIXME: This has something to do with drag-and-drop between two different trees.
// However, this interface doesn't match up well with the native Cocoa implementation for this, which is implemented.
// I don't know how this will be triggered, if at all since the drag-and-drop is handled elsewhere.
// For now, this is a no-op.
void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle* item_src, InodeHandle* item_dst)
{

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
  int object_id = iupTreeFindNodeId(ih, tree_item);
  int start_id = object_id;



  if (cb)
    cocoaTreeRemoveNodeDataRec(ih, tree_item, cb, &object_id);
  else
  {
    int removed_count = iupdrvTreeTotalChildCount(ih, tree_item)+1;
    ih->data->node_count -= removed_count;
  }

  iupTreeDelFromCache(ih, start_id, old_count-ih->data->node_count);
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

static int cocoaTreeSetDelNodeAttrib(Ihandle* ih, int node_id, const char* value)
{
	if (!ih->handle)  /* do not do the action before map */
		return 0;

	NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
	IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];


	if (iupStrEqualNoCase(value, "ALL"))
	{
		NSUInteger number_of_root_items = [[data_source_delegate treeRootTopLevelObjects] count];
		if(number_of_root_items > 0)
		{
			cocoaTreeRemoveAllNodeData(ih, 1);

			[data_source_delegate removeAllObjects];

			// Scuri says not required to handle for delete.
			// [data_source_delegate handleSelectionDidChange:outline_view];

			// If there are multiple nodes at the root (ADDROOT=NO), it seems easier to reloadData than to hunt down and remove each node.
			[outline_view reloadData];

		}

		return 0;
	}
	if (iupStrEqualNoCase(value, "SELECTED"))  /* selected here means the reference node */
	{

		IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)iupTreeGetNode(ih, node_id);

		if(!tree_item)
		{
			return 0;
		}
		NSCAssert([tree_item isKindOfClass:[IupCocoaTreeItem class]], @"expecting class IupCocoaTreeItem");

		[outline_view beginUpdates];
    cocoaTreeRemoveNodeData(ih, tree_item, 1);

		IupCocoaTreeItem* parent_tree_item = [tree_item parentItem]; // get parent before removing because it may nil out the parent in removeItem
		NSUInteger target_index = [data_source_delegate removeItem:tree_item];
		//	[outline_view reloadData];

		if(NSNotFound != target_index)
		{
			NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:target_index];
			// I don't think the animations are working for remove
//			[outline_view removeItemsAtIndexes:index_set inParent:parent_tree_item withAnimation:NSTableViewAnimationEffectNone];
//			[outline_view removeItemsAtIndexes:index_set inParent:parent_tree_item withAnimation:NSTableViewAnimationEffectFade];
			[outline_view removeItemsAtIndexes:index_set inParent:parent_tree_item withAnimation:NSTableViewAnimationEffectGap];
		}

		// Scuri says not required to handle for delete.
		// [data_source_delegate handleSelectionDidChange:outline_view];

		[outline_view endUpdates];

	}
	else if(iupStrEqualNoCase(value, "CHILDREN"))  /* children of the reference node */
	{
		IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)iupTreeGetNode(ih, node_id);

		if(!tree_item)
		{
			return 0;
		}
		NSCAssert([tree_item isKindOfClass:[IupCocoaTreeItem class]], @"expecting class IupCocoaTreeItem");
		
		[outline_view beginUpdates];
    cocoaTreeRemoveNodeData(ih, tree_item, 1);

		NSIndexSet* index_set = [data_source_delegate removeAllChildrenForItem:tree_item];
//		[outline_view removeItemsAtIndexes:index_set inParent:tree_item withAnimation:NSTableViewAnimationEffectNone];
//		[outline_view removeItemsAtIndexes:index_set inParent:tree_item withAnimation:NSTableViewAnimationEffectFade];
		[outline_view removeItemsAtIndexes:index_set inParent:tree_item withAnimation:NSTableViewAnimationEffectGap];

		//[outline_view reloadData];

		// Scuri says not required to handle for delete.
		// [data_source_delegate handleSelectionDidChange:outline_view];

		[outline_view endUpdates];

		return 0;
		
	}
	else if(iupStrEqualNoCase(value, "MARKED"))  /* Delete the array of marked nodes */
	{
		[outline_view beginUpdates];
		NSIndexSet* selected_index = [outline_view selectedRowIndexes];

		// TODO: For multiple selection, I should figure out how to handle the case where
		// both the parent and children are selected.
		// The naive solution might double delete, or index positions may shift.
		// I can use reloadData to work around some of this,
		// but I lose animations and other potential UI state with it.
		// For now, I can special case single selection so it can avoid reloadData.
		
		if([selected_index count] == 1)
		{
			NSUInteger selected_i = [selected_index firstIndex];
			id selected_item = [outline_view itemAtRow:selected_i];
			cocoaTreeRemoveNodeData(ih, selected_item, 1);
			IupCocoaTreeItem* parent_tree_item  = [(IupCocoaTreeItem*)selected_item parentItem];
			NSUInteger target_index = [data_source_delegate removeItem:selected_item];
			
			NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:target_index];
//			[outline_view removeItemsAtIndexes:index_set inParent:parent_tree_item withAnimation:NSTableViewAnimationEffectFade];
			[outline_view removeItemsAtIndexes:index_set inParent:parent_tree_item withAnimation:NSTableViewAnimationEffectGap];
		}
		else
		{
			NSUInteger selected_i = [selected_index firstIndex];
			while(selected_i != NSNotFound)
			{
				id selected_item = [outline_view itemAtRow:selected_i];
				cocoaTreeRemoveNodeData(ih, selected_item, 1);

				// I can't figure out how to make this work correctly when you select both parents and its children to be deleted.
				// Use reloadData for now.
	#if 0
				IupCocoaTreeItem* parent_tree_item  = [(IupCocoaTreeItem*)selected_item parentItem];
				NSUInteger target_index = [data_source_delegate removeItem:selected_item];
				
				NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:target_index];
	//			[outline_view removeItemsAtIndexes:index_set inParent:parent_tree_item withAnimation:NSTableViewAnimationEffectFade];
				[outline_view removeItemsAtIndexes:index_set inParent:parent_tree_item withAnimation:NSTableViewAnimationEffectGap];
	#else
				[data_source_delegate removeItem:selected_item];
	#endif

				
				// get the next index in the set
				selected_i = [selected_index indexGreaterThanIndex:selected_i];
			}

			// WARNING: I seem to get a crash somewhere else when the outlineview reloads later if I both call remoeItemsAtIndexes and reloadData. This may imply some other bug.
			[outline_view reloadData];

		}
		// Scuri says not required to handle for delete.
		// [data_source_delegate handleSelectionDidChange:outline_view];
		[outline_view endUpdates];

	}
	
#if IUPCOCOA_TREE_TEST_VALIDATE_NODE_CACHE
	testValidateCocoaTreeNodeCache(ih, data_source_delegate);
#endif


	return 0;
}


static int cocoaTreeSetMoveNodeAttrib(Ihandle* ih, int node_id, const char* value)
{


	if(!ih->handle)  /* do not do the action before map */
	{
		return 0;
	}

	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
	IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)iupTreeGetNode(ih, node_id);

	if(!tree_item)
	{
		return 0;
	}
	IupCocoaTreeItem* parent_tree_item  = [tree_item parentItem];

	IupCocoaTreeItem* target_tree_item = (IupCocoaTreeItem*)iupTreeGetNodeFromString(ih, value);
	IupCocoaTreeItem* parent_target_tree_item = [target_tree_item parentItem];

	/* If Drag item is an ancestor of Drop item then return */
	if([tree_item isEqual:target_tree_item])
	{
		return 0;
	}
#if 0
	if([tree_item isEqual:parent_target_tree_item])
	{
		return 0;
	}
#endif

	NSUInteger object_index;
	if(parent_tree_item)
	{
		object_index = [[parent_tree_item childrenArray] indexOfObject:tree_item];

	}
	else
	{
		object_index = [[data_source_delegate treeRootTopLevelObjects] indexOfObject:tree_item];
	}

	// If the destination node is a branch and it is expanded,
	// then the specified node is inserted as the first child of the destination node.
	// If the branch is not expanded or the destination node is a leaf,
	// then it is inserted as the next brother of the leaf.
	
	BOOL is_target_expanded = NO;
	int target_kind = [target_tree_item kind];
	if(ITREE_BRANCH == target_kind)
	{
		is_target_expanded = [outline_view isItemExpanded:target_tree_item];


		if(is_target_expanded)
		{
		

			// update the data source
			
			helperMoveNode(outline_view, tree_item, parent_target_tree_item, 0);


		}
		else
		{
			NSUInteger target_index;
			if(parent_target_tree_item)
			{
				target_index = [[parent_target_tree_item childrenArray] indexOfObject:target_tree_item];
			}
			else
			{
				target_index = [[data_source_delegate treeRootTopLevelObjects] indexOfObject:target_tree_item];
			}
			// update the data source
			helperMoveNode(outline_view, tree_item, parent_target_tree_item, target_index);

		}
	}
	else
	{
		NSUInteger target_index;
		if(parent_target_tree_item)
		{
			target_index = [[parent_target_tree_item childrenArray] indexOfObject:target_tree_item];
		}
		else
		{
			target_index = [[data_source_delegate treeRootTopLevelObjects] indexOfObject:target_tree_item];
		}
		// update the data source
		helperMoveNode(outline_view, tree_item, parent_target_tree_item, target_index);

	
	}
	
	


  return 0;
}

static int cocoaTreeSetCopyNodeAttrib(Ihandle* ih, int node_id, const char* value)
{


	if(!ih->handle)  /* do not do the action before map */
	{
		return 0;
	}

	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
	IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];

	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)iupTreeGetNode(ih, node_id);

	if(!tree_item)
	{
		return 0;
	}
	IupCocoaTreeItem* parent_tree_item  = [tree_item parentItem];

	IupCocoaTreeItem* target_tree_item = (IupCocoaTreeItem*)iupTreeGetNodeFromString(ih, value);
	IupCocoaTreeItem* parent_target_tree_item = [target_tree_item parentItem];



	NSUInteger object_index;
	if(parent_tree_item)
	{
		object_index = [[parent_tree_item childrenArray] indexOfObject:tree_item];

	}
	else
	{
		object_index = [[data_source_delegate treeRootTopLevelObjects] indexOfObject:tree_item];
	}

	// If the destination node is a branch and it is expanded,
	// then the specified node is inserted as the first child of the destination node.
	// If the branch is not expanded or the destination node is a leaf,
	// then it is inserted as the next brother of the leaf.
	
	BOOL is_target_expanded = NO;
	int target_kind = [target_tree_item kind];
	if(ITREE_BRANCH == target_kind)
	{
		is_target_expanded = [outline_view isItemExpanded:target_tree_item];


		if(is_target_expanded)
		{
			// update the data source
			helperCopyAndInsertNode(outline_view, tree_item, parent_target_tree_item, 0, NSTableViewAnimationEffectGap);
		}
		else
		{
			NSUInteger target_index;
			if(parent_target_tree_item)
			{
				target_index = [[parent_target_tree_item childrenArray] indexOfObject:target_tree_item];
			}
			else
			{
				target_index = [[data_source_delegate treeRootTopLevelObjects] indexOfObject:target_tree_item];
			}
			// update the data source
			helperCopyAndInsertNode(outline_view, tree_item, parent_target_tree_item, target_index, NSTableViewAnimationEffectGap);
		}
	}
	else
	{
		NSUInteger target_index;
		if(parent_target_tree_item)
		{
			target_index = [[parent_target_tree_item childrenArray] indexOfObject:target_tree_item];
		}
		else
		{
			target_index = [[data_source_delegate treeRootTopLevelObjects] indexOfObject:target_tree_item];
		}
		// update the data source
		helperCopyAndInsertNode(outline_view, tree_item, parent_target_tree_item, target_index, NSTableViewAnimationEffectGap);
	}
	return 0;
}
/*****************************************************************************/
/* MANIPULATING IMAGES                                                       */
/*****************************************************************************/

/* Apple has a built-in Folder image we are supposed to use.
	Also, the default IUP one looks awful on Mac. (The resolution is too low and the colors are wrong.)
 	So we will override the default IUP one here.
 	IUP has different images for expanded and collaspsed, but that is not a normal thing for Mac.
 	Additionally, there is a performance cost and possible issues with losing selections due to reloading items when we switch on Mac.
 	And Apple does not provide an 'open' folder image.
	So we will make the expanded and collapsed images point to the same image here.
	This will allow us to get back a little performance by default.
 	TODO: Redesign the core iup_tree.c code to not load the default images for performance.
*/
static void helperLoadReplacementDefaultImages()
{
	NSImage* ns_folder_image = [NSImage imageNamed:NSImageNameFolder];

	// The default IUP image 16x16.
	// The default table cell height accomodates height=16 perfectly.
	// But the Apple icon is currently 32x32 (subject to get bigger over time).
	// So we will scale the image down to 16x16.
	CGRect resize_rect = { 0, 0, 16, 16 };
	
	CGImageRef cg_image = [ns_folder_image CGImageForProposedRect:&resize_rect context:nil hints:nil];
//	CGImageRef cg_image = [ns_folder_image CGImageForProposedRect:nil context:nil hints:nil];

	NSUInteger bytes_per_row  = resize_rect.size.width * 4; // rgba
  	NSUInteger total_bytes  = bytes_per_row * resize_rect.size.height;

  	void* copy_pixel_buffer = malloc(total_bytes);
	CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
	
	 CGContextRef offscreen_context_ref = CGBitmapContextCreate(
		copy_pixel_buffer,
		resize_rect.size.width,
		resize_rect.size.height,
		8, // bits per component
		bytes_per_row,
		color_space,
		kCGImageAlphaPremultipliedLast
	);
	CGColorSpaceRelease(color_space);
	CGContextDrawImage(offscreen_context_ref, resize_rect, cg_image);
	
//	CFDataRef raw_data = CGDataProviderCopyData(CGImageGetDataProvider(offscreen_context_ref));
//	unsigned char* raw_data = CGBitmapContextGetData(offscreen_context_ref);

//	CFIndex data_length = CFDataGetLength(raw_data);
  	Ihandle* ih_folder_image = IupImageRGBA(resize_rect.size.width, resize_rect.size.height, (unsigned char*)copy_pixel_buffer);
//	CFRelease(raw_data);
	CFRelease(offscreen_context_ref);
	free(copy_pixel_buffer);
	// Copied from collapsed_image. I don't know if these colors make sense, or if they are ever used.
#if 1
	IupSetAttribute(ih_folder_image, "0", "BGCOLOR");
	IupSetAttribute(ih_folder_image, "1", "255 206 156");
	IupSetAttribute(ih_folder_image, "2", "156 156 0");
	IupSetAttribute(ih_folder_image, "3", "0 0 0");
	IupSetAttribute(ih_folder_image, "4", "206 206 99");
	IupSetAttribute(ih_folder_image, "5", "255 255 206");
	IupSetAttribute(ih_folder_image, "6", "247 247 247");
	IupSetAttribute(ih_folder_image, "7", "255 255 156");
#endif

	IupSetHandle("IMGCOLLAPSED", ih_folder_image);
	IupSetHandle("IMGEXPANDED",  ih_folder_image);
}

static void helperReplaceDefaultImages(Ihandle* ih, IupCocoaOutlineView* outline_view)
{
	// This was loaded in helperLoadReplacementDefaultImages.
	// Round-tripping the image through IUP is intentional in case IUP does any additional transformations.
	NSImage* ns_folder_image_roundtrip = iupImageGetImage(iupAttribGetStr(ih, "IMAGEBRANCHCOLLAPSED"), ih, 0, NULL);
	
	// We use the same pointer for both. This allows for an optimization to kick in to not reload items when expand/collapse happens.
	[outline_view setCollapsedImage:ns_folder_image_roundtrip];
	[outline_view setExpandedImage:ns_folder_image_roundtrip];

}

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
	
	// Oh how ironic and tragic.
	// I combined expanded and leaf into the same variable and made collapsed separate.
	// But IUP combined collpased and leaf into the same variable and made expanded separate.
	[tree_item setBitmapImage:bitmap_image];
	cocoaTreeReloadItem(tree_item, outline_view);

	return 1;
}

static int cocoaTreeSetImageAttrib(Ihandle* ih, int node_id, const char* value)
{
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)iupTreeGetNode(ih, node_id);
	NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

	NSImage* bitmap_image = helperGetImage(ih, node_id, value, tree_item);

	
	// Oh how ironic and tragic.
	// I combined expanded and leaf into the same variable and made collapsed separate.
	// But IUP combined collpased and leaf into the same variable and made expanded separate.
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
		// only need to reload if the user hasn't overridden with a custom-per-node image.
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
	// only need to reload if the user hasn't overridden with a custom-per-node image.
	if([tree_item kind] == ITREE_BRANCH)
	{
		// only need to reload if the user hasn't overridden with a custom-per-node image.
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
		// only need to reload if the user hasn't overridden with a custom-per-node image.
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
	if(NULL == inode_handle)
	{
		return NULL;
	}
	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	if([outline_view isExpandable:tree_item])
	{
		// BUG: Apple seems to return NO if a parent is collapsed, even if the actual item is expanded.
		// But when Apple expands the parent, it remembers and restores the child state.
		// This is a problem for us because we are trying to copy the state to apply to new copies of the node.
		// I don't know how to workaround this.
		// Tested 10.13.6
		BOOL is_expanded = [outline_view isItemExpanded:tree_item];
		if(is_expanded)
		{
			return "EXPANDED";
		}
		else
		{
			return "COLLAPSED";
		}
	}
	else
	{
		return NULL;
	}
}


static int cocoaTreeSetStateAttrib(Ihandle* ih, int item_id, const char* value)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return 0;
	}
	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	if([outline_view isExpandable:tree_item])
	{
		if(iupStrEqualNoCase(value, "EXPANDED"))
		{
			[outline_view expandItem:tree_item];
		}
		else
		{
			[outline_view collapseItem:tree_item];
		}
	}
	return 0;
}


static char* cocoaTreeGetDepthAttrib(Ihandle* ih, int item_id)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	IupCocoaTreeDelegate* tree_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
	NSUInteger out_depth_counter = 0;
	bool did_find = Helper_CountDepth(tree_delegate, tree_item, &out_depth_counter);
	if(did_find)
	{
		return iupStrReturnInt((int)out_depth_counter);
	}
	else
	{
	    return NULL;
	}
}

static char* cocoaTreeGetKindAttrib(Ihandle* ih, int item_id)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	
	if([tree_item kind] == ITREE_LEAF)
	{
		return "LEAF";
	}
	else
	{
		return "BRANCH";
	}
}

static char* cocoaTreeGetParentAttrib(Ihandle* ih, int item_id)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	IupCocoaTreeItem* parent_item = [tree_item parentItem];
	int parent_id = iupTreeFindNodeId(ih, (InodeHandle*)parent_item);
	return iupStrReturnInt(parent_id);
}

// returns the next brother (same depth) of the specified node. Returns NULLs if at last child node of the parent (at the same depth)
static char* cocoaTreeGetNextAttrib(Ihandle* ih, int item_id)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	IupCocoaTreeItem* parent_item = [tree_item parentItem];
	if(nil == parent_item)
	{
		return NULL;
	}
	NSArray* children_array = [parent_item childrenArray];
	
	bool found_current_item = false;
	NSUInteger index_count = 0;
	for(IupCocoaTreeItem* a_item in children_array)
	{
		index_count += 1;
		if([tree_item isEqual:a_item])
		{
			found_current_item = true;
			break;
		}
	}

	if(index_count >= [children_array count])
	{
		// our item was the last node, so return NULL
		return NULL;
	}
	IupCocoaTreeItem* next_item = [children_array objectAtIndex:index_count];
	int next_id = iupTreeFindNodeId(ih, (InodeHandle*)next_item);
	return iupStrReturnInt(next_id);
}

// returns the previous brother (same depth) of the specified node. Returns NULLs if at first child node of the parent (at the same depth).
static char* cocoaTreeGetPreviousAttrib(Ihandle* ih, int item_id)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	IupCocoaTreeItem* parent_item = [tree_item parentItem];
	if(nil == parent_item)
	{
		return NULL;
	}
	NSArray* children_array = [parent_item childrenArray];
	
	bool found_current_item = false;
	NSUInteger index_count = 0;
	for(IupCocoaTreeItem* a_item in children_array)
	{
		if([tree_item isEqual:a_item])
		{
			found_current_item = true;
			break;
		}
		index_count += 1;
	}

	if(index_count == 0)
	{
		// our item was the first node, so return NULL
		return NULL;
	}
	IupCocoaTreeItem* previous_item = [children_array objectAtIndex:index_count-1];
	int previous_id = iupTreeFindNodeId(ih, (InodeHandle*)previous_item);
	return iupStrReturnInt(previous_id);
}

static char* cocoaTreeGetFirstAttrib(Ihandle* ih, int item_id)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	IupCocoaTreeItem* parent_item = [tree_item parentItem];
	if(nil == parent_item)
	{
		return NULL;
	}
	NSArray* children_array = [parent_item childrenArray];
	IupCocoaTreeItem* next_item = [children_array firstObject];
	int next_id = iupTreeFindNodeId(ih, (InodeHandle*)next_item);
	return iupStrReturnInt(next_id);
}

static char* cocoaTreeGetLastAttrib(Ihandle* ih, int item_id)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	IupCocoaTreeItem* parent_item = [tree_item parentItem];
	if(nil == parent_item)
	{
		return NULL;
	}
	NSArray* children_array = [parent_item childrenArray];
	IupCocoaTreeItem* next_item = [children_array lastObject];
	int next_id = iupTreeFindNodeId(ih, (InodeHandle*)next_item);
	return iupStrReturnInt(next_id);
}

static char* cocoaTreeGetTitleAttrib(Ihandle* ih, int item_id)
{
//	NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
//	IupCocoaTreeDelegate* data_source_delegate = (IupCocoaTreeDelegate*)[outline_view dataSource];
	
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);

	if(inode_handle)
	{
		IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
		NSCAssert([tree_item isKindOfClass:[IupCocoaTreeItem class]], @"expecting class IupCocoaTreeItem");
		NSString* ns_title = [tree_item title];
		return iupStrReturnStr([ns_title UTF8String]);
	}
	else
	{
		return NULL;
	}
}

static int cocoaTreeSetTitleAttrib(Ihandle* ih, int item_id, const char* value)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	
	if(inode_handle)
	{
		NSString* ns_title = @"";
		if(value)
		{
			ns_title = [NSString stringWithUTF8String:value];
		}
		
		IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
		NSCAssert([tree_item isKindOfClass:[IupCocoaTreeItem class]], @"expecting class IupCocoaTreeItem");
		[tree_item setTitle:ns_title];
		NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);

		cocoaTreeReloadItem(tree_item, outline_view);
	}

	return 0;
}


// I think this is supposed to return the number nodes immediately under the root node (not recursive).
static char* cocoaTreeGetRootCountAttrib(Ihandle* ih)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, 0);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	
	
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	
	// If this is a leaf, then there are no children to descend into and count
	if(ITREE_LEAF == [tree_item kind])
	{
		return iupStrReturnInt(0);
	}

	return iupStrReturnInt((int)[[tree_item childrenArray] count]);
}

// returns the immediate children count of the specified branch. It does not count children of child that are branches.
static char* cocoaTreeGetChildCountAttrib(Ihandle* ih, int item_id)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	
	
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	
	// If this is a leaf, then there are no children to descend into and count
	if(ITREE_LEAF == [tree_item kind])
	{
		return iupStrReturnInt(0);
	}

	
	// else we are a branch
	int leaf_counter = 0;
	for(IupCocoaTreeItem* a_item in [tree_item childrenArray])
	{
		if(ITREE_LEAF == [a_item kind])
		{
			leaf_counter++;
		}
	}
	return iupStrReturnInt(leaf_counter);
}


static char* cocoaTreeGetMarkedAttrib(Ihandle* ih, int item_id)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	// If id is empty or invalid, then the focus node is used as reference node.
	if(NULL == inode_handle)
	{
		IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
		NSIndexSet* selected_index = [outline_view selectedRowIndexes];
		NSUInteger selected_i = [selected_index firstIndex];
		if(selected_i != NSNotFound)
		{
			return iupStrReturnBoolean(true);
		}
		else
		{
			return iupStrReturnBoolean(false);
		}
	}
	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	
	NSIndexSet* selected_index = [outline_view selectedRowIndexes];
	NSUInteger selected_i = [selected_index firstIndex];
	while(selected_i != NSNotFound)
	{
		IupCocoaTreeItem* selected_item = [outline_view itemAtRow:selected_i];
		if([selected_item isEqual:tree_item])
		{
			return iupStrReturnBoolean(true);
		}
		// get the next index in the set
		selected_i = [selected_index indexGreaterThanIndex:selected_i];
	}
	return iupStrReturnBoolean(false);
}

// TODO: We might need to disable selection callbacks while doing this.
static int cocoaTreeSetMarkedAttrib(Ihandle* ih, int item_id, const char* value)
{
	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	bool should_select = iupStrBoolean(value);
	
	// If id is empty or invalid, then the focus node is used as reference node.
	if(NULL == inode_handle)
	{
		// On Mac, our focus node is the selected node.
		// If should_select, then it is already selected, so there is nothing to do.
		if(should_select)
		{
			return 0;
		}
		// Otherwise, I guess we deselect it?
		// not sure what to do with multiple selection
		IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
		[outline_view deselectAll:nil];
	}
	
	
	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	NSInteger item_row = [outline_view rowForItem:tree_item];

	iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
	if(should_select)
	{
		NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:item_row];
		[outline_view selectRowIndexes:index_set byExtendingSelection:NO];
	}
	else
	{
		[outline_view deselectRow:item_row];
	}
	iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
	return 0;
}

// TODO: We might need to disable selection callbacks while doing this.
static int cocoaTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
	if(ih->data->mark_mode==ITREE_MARK_SINGLE)
	{
		return 0;
	}
	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);

	
	if(iupStrEqualNoCase(value, "BLOCK"))
	{
	/*
    GtkTreeIter iterItem1, iterItem2;
    GtkTreePath* pathFocus;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ih->handle), &pathFocus, NULL);
    gtk_tree_model_get_iter(model, &iterItem1, pathFocus);
    gtk_tree_path_free(pathFocus);

    gtkTreeIterInit(ih, &iterItem2, iupAttribGet(ih, "_IUPTREE_MARKSTART_NODE"));

    gtkTreeSelectRange(ih, model, selection, &iterItem1, &iterItem2, 0);
    */
    	IupCocoaTreeItem* mark_start_node = (IupCocoaTreeItem*)iupAttribGet(ih, "_IUPTREE_MARKSTART_NODE");
		if(nil == mark_start_node)
		{
			
			return 0;

		}
		// There might be a possibility that mark_start_node is not in the NSOutlineView because the user removed it, but we still have a pointer to it.
		// So make sure it is in the tree.
		NSInteger mark_start_node_row = [outline_view rowForItem:mark_start_node];
		if(mark_start_node_row <= 0)
		{
			return 0;
		}
		// I don't know what the "focus" node should be if there are multiple selections.
		NSIndexSet* selected_index = [outline_view selectedRowIndexes];
		NSUInteger selected_i = [selected_index firstIndex];
		if(selected_i == NSNotFound)
		{
			return 0;
		}

		if(selected_i == mark_start_node_row)
		{
			// only one item is needed, and it is already selected
			return 1;
		}

		NSUInteger start_row;
		NSUInteger number_of_rows;
		if(mark_start_node_row > selected_i)
		{
			start_row = selected_i;
			number_of_rows = mark_start_node_row - selected_i + 1; // +1 to include the end node
		}
		else
		{
			start_row = mark_start_node_row;
			number_of_rows = selected_i - mark_start_node_row + 1; // +1 to include the end node
		}

		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
		NSIndexSet* index_set = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(start_row, number_of_rows)];
		[outline_view selectRowIndexes:index_set byExtendingSelection:NO];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);

	}
	else if(iupStrEqualNoCase(value, "CLEARALL"))
	{
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
		[outline_view deselectAll:nil];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
	}
	else if(iupStrEqualNoCase(value, "MARKALL"))
	{
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
		[outline_view selectAll:nil];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
	}
	else if(iupStrEqualNoCase(value, "INVERTALL")) /* INVERTALL *MUST* appear before INVERT, or else INVERTALL will never be called. */
	{
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
		NSIndexSet* selected_index = [outline_view selectedRowIndexes];
		[outline_view selectAll:nil];
		NSUInteger selected_i = [selected_index firstIndex];
		while(selected_i != NSNotFound)
		{
			[outline_view deselectRow:selected_i];
			// get the next index in the set
			selected_i = [selected_index indexGreaterThanIndex:selected_i];
		}
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
	}
	else if(iupStrEqualPartial(value, "INVERT"))
	{
    	/* iupStrEqualPartial allows the use of "INVERTid" form */
    	const char* id_string = &value[strlen("INVERT")];
		InodeHandle* inode_handle = iupTreeGetNodeFromString(ih, id_string);
		
		// If id is empty or invalid, then the focus node is used as reference node.
		if(NULL == inode_handle)
		{
			// Conundrum: if nothing is selected, there is no focus node, so there is nothing to select.
			// Do nothing for that case, so we only worry about deselecting?
			// Or do we select all?
			NSIndexSet* selected_index = [outline_view selectedRowIndexes];
			NSUInteger selected_i = [selected_index firstIndex];
			while(selected_i != NSNotFound)
			{
				[outline_view deselectRow:selected_i];
				// get the next index in the set
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
			while(selected_i != NSNotFound)
			{
				IupCocoaTreeItem* selected_item = [outline_view itemAtRow:selected_i];
				if([selected_item isEqual:tree_item])
				{
					was_item_selected = true;
					break;
				}
				// get the next index in the set
				selected_i = [selected_index indexGreaterThanIndex:selected_i];
			}
			
			iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
			if(was_item_selected)
			{
				[outline_view deselectRow:item_row];
			}
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
		// I don't know what this case is, but it was in the GTK code.
		// This is a guess translation to Cocoa.
 
//		GtkTreeIter iterItem1, iterItem2;
		char str1[50], str2[50];
		if(iupStrToStrStr(value, str1, str2, '-') != 2)
		{
			return 0;
		}
//		if (!gtkTreeFindNodeFromString(ih, str1, &iterItem1))
//			return 0;
//		if (!gtkTreeFindNodeFromString(ih, str2, &iterItem2))
//			return 0;
//		gtkTreeSelectRange(ih, model, selection, &iterItem1, &iterItem2, 0);

		InodeHandle* inode_handle1 = iupTreeGetNodeFromString(ih, str1);
		InodeHandle* inode_handle2 = iupTreeGetNodeFromString(ih, str2);
		NSInteger item_row1 = [outline_view rowForItem:inode_handle1];
		NSInteger item_row2 = [outline_view rowForItem:inode_handle2];

		NSUInteger start_row;
		NSUInteger number_of_rows;
		if(item_row2 > item_row1)
		{
			start_row = item_row1;
			number_of_rows = item_row2 - item_row1 + 1; // +1 to include the end node
		}
		else
		{
			start_row = item_row2;
			number_of_rows = item_row1 - item_row2 + 1; // +1 to include the end node
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
	if(NULL == inode_handle)
	{
		[outline_view setMarkStartNode:nil];
		return 0;
	}
	
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;

	// I'm concerned about dangling pointers or memory leaks.
	// Because we are keeping a raw pointer indefinately, it could go away and we are left with a dangling pointer.
	// We could retain it, but we need to be sure to release it.
	// An alternative solution is to use a weak reference that automatically nils when the object is dealloc'd.
	// Unfortunately, I think zeroing weak references are only available with ARC.
	// So we will ratain it in the outline_view so it should be released when the outline_view goes away in the worse case.
	[outline_view setMarkStartNode:tree_item];
	iupAttribSet(ih, "_IUPTREE_MARKSTART_NODE", (char*)tree_item);

	return 1;
}

static char* cocoaTreeGetMarkedNodesAttrib(Ihandle* ih)
{
	char* str = iupStrGetMemory(ih->data->node_count+1);
	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);

	NSIndexSet* selected_index = [outline_view selectedRowIndexes];

	// This is O(n^2).
	// Iterate through all the nodes,
	// 		foreach node, check the entire set of selected nodes to learn if it is selected
	for(size_t i=0; i<ih->data->node_count; i++)
	{
		IupCocoaTreeItem* iter_node = (IupCocoaTreeItem*)ih->data->node_cache[i].node_handle;
		
		NSUInteger selected_i = [selected_index firstIndex];
		bool is_found = false;
		while(selected_i != NSNotFound)
		{
			IupCocoaTreeItem* selected_item = (IupCocoaTreeItem*)[outline_view itemAtRow:selected_i];
			if([iter_node isEqual:selected_item])
			{
				is_found = true;
				break;
			}
			// get the next index in the set
			selected_i = [selected_index indexGreaterThanIndex:selected_i];
		}
		if(is_found)
		{
			str[i] = '+';
		}
		else
		{
			str[i] = '-';
		}
	}
	str[ih->data->node_count] = 0;
	return str;
}

static int cocoaTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
	int count;

	if(ih->data->mark_mode==ITREE_MARK_SINGLE || !value)
	{
		return 0;
	}

	count = (int)strlen(value);
	if(count > ih->data->node_count)
	{
		count = ih->data->node_count;
	}


	iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
	
	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
	[outline_view deselectAll:nil];

	for(int i=0; i<count; i++)
	{
		if(value[i] == '+')
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
	if(nil == inode_handle)
	{
		return 0;
	}

	// First, make sure all the parent nodes that contain this item are expanded.
	// WATCH OUT: Apple ignores expanding nodes where the parent is collapsed.
	// So even though we need to go bottom-up to find all the parents,
	// we must then go top-down to expand all the nodes.
	// Use a temporary array to store all the parents
	{
		NSMutableArray<IupCocoaTreeItem*>* parent_node_array = [[NSMutableArray alloc] initWithCapacity:ih->data->node_count];
		{
			IupCocoaTreeItem* parent_tree_item = [(IupCocoaTreeItem*)inode_handle parentItem];
			while(parent_tree_item != nil)
			{
				// NOTE: I could check for isExpandable: and isItemExpanded:,
				// but I have become distrustful of this information with collapsed parents.
				// So I'm just going to always expand every parent.
				[parent_node_array insertObject:parent_tree_item atIndex:0];
				parent_tree_item = [parent_tree_item parentItem];
			}
		}
		for(IupCocoaTreeItem* a_item in parent_node_array)
		{
			// NOTE: I could check for isExpandable: and isItemExpanded:,
			// but I have become distrustful of this information with collapsed parents.
			// So I'm just going to always expand every parent.
			[outline_view expandItem:a_item];
		}
		[parent_node_array release];
	}

	// Now scroll to the item
	NSInteger item_row = [outline_view rowForItem:inode_handle];
	if(item_row < 0)
	{
		return 0;
	}
	[outline_view scrollRowToVisible:item_row];

	return 0;
}

static int cocoaTreeSetValueAttrib(Ihandle* ih, const char* value)
{
	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
	NSUInteger number_of_visible_rows = [outline_view numberOfRows];
	
	if(0 == number_of_visible_rows)
	{
		return 0;
	}

	NSUInteger new_selected_row;
	


	if(iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST"))
	{
		new_selected_row = 0;
		[outline_view scrollRowToVisible:0];
		NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:0];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
		[outline_view selectRowIndexes:index_set byExtendingSelection:NO];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
	}
	else if(iupStrEqualNoCase(value, "LAST"))
	{
		new_selected_row = number_of_visible_rows-1;
		[outline_view scrollRowToVisible:new_selected_row];
		NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_selected_row];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
		[outline_view selectRowIndexes:index_set byExtendingSelection:NO];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
	}
	else if(iupStrEqualNoCase(value, "PGUP"))
	{
		NSIndexSet* selected_index = [outline_view selectedRowIndexes];
		NSUInteger selected_i = [selected_index firstIndex];
		if(selected_i == NSNotFound)
		{
			return 0;
		}
		if(selected_i < 10)
		{
			new_selected_row = 0;
		}
		else
		{
			new_selected_row = selected_i - 10;
		}
		[outline_view scrollRowToVisible:new_selected_row];
		NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_selected_row];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
		[outline_view selectRowIndexes:index_set byExtendingSelection:NO];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
	}
	else if(iupStrEqualNoCase(value, "PGDN"))
	{
		NSIndexSet* selected_index = [outline_view selectedRowIndexes];
		NSUInteger selected_i = [selected_index firstIndex];
		if(selected_i == NSNotFound)
		{
			return 0;
		}
		if((selected_i+10) > (number_of_visible_rows-1))
		{
			new_selected_row = number_of_visible_rows-1;
		}
		else
		{
			new_selected_row = selected_i+10;
		}
		[outline_view scrollRowToVisible:new_selected_row];
		NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_selected_row];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
		[outline_view selectRowIndexes:index_set byExtendingSelection:NO];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
	}
	else if(iupStrEqualNoCase(value, "NEXT"))
	{
		NSIndexSet* selected_index = [outline_view selectedRowIndexes];
		NSUInteger selected_i = [selected_index firstIndex];
		if(selected_i == NSNotFound)
		{
			return 0;
		}
		if((selected_i+1) > (number_of_visible_rows-1))
		{
			new_selected_row = number_of_visible_rows-1;
		}
		else
		{
			new_selected_row = selected_i+1;
		}
		[outline_view scrollRowToVisible:new_selected_row];
		NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:new_selected_row];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", "1");
		[outline_view selectRowIndexes:index_set byExtendingSelection:NO];
		iupAttribSet(ih, "_IUPTREE_IGNORE_SELECTION_CB", NULL);
	}
	else if(iupStrEqualNoCase(value, "PREVIOUS"))
	{
		NSIndexSet* selected_index = [outline_view selectedRowIndexes];
		NSUInteger selected_i = [selected_index firstIndex];
		if(selected_i == NSNotFound)
		{
			return 0;
		}
		if(selected_i < 1)
		{
			new_selected_row = 0;
		}
		else
		{
			new_selected_row = selected_i - 1;
		}
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


static char* cocoaTreeGetValueAttrib(Ihandle* ih)
{
	NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
	NSIndexSet* selected_index = [outline_view selectedRowIndexes];
	NSUInteger selected_i = [selected_index firstIndex];
	if(selected_i != NSNotFound)
	{
		InodeHandle* selected_item = (InodeHandle*)[outline_view itemAtRow:selected_i];
		int object_id = iupTreeFindNodeId(ih, selected_item);
		return iupStrReturnInt(object_id);
	}
	else
	{
		if(ih->data->node_count > 0)
		{
			return "0"; /* default VALUE is root */
		}
		else
		{
			return "-1";
		}
	}
}


static char* cocoaTreeGetToggleValueAttrib(Ihandle* ih, int item_id)
{
	if(ih->data->show_toggle < 1)
	{
		return NULL;
	}
  	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	// it happens that iupStrReturnChecked uses the same values for mixed, off, and on
	NSControlStateValue check_box_state = [tree_item checkBoxState];
	return iupStrReturnChecked((int)check_box_state);
}

static int cocoaTreeSetToggleValueAttrib(Ihandle* ih, int item_id, const char* value)
{
	if(ih->data->show_toggle < 1)
	{
		return 0;
	}
  	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return 0;
	}
	
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	NSControlStateValue check_box_state;
	
	if((ih->data->show_toggle == 2) && iupStrEqualNoCase(value, "NOTDEF"))
	{
		check_box_state = NSControlStateValueMixed;
	}
	else if(iupStrEqualNoCase(value, "ON"))
	{
		check_box_state = NSControlStateValueOn;
	}
	else if(iupStrEqualNoCase(value, "OFF"))
	{
		check_box_state = NSControlStateValueOff;
	}
	else
	{
		return 0;
	}
	
	[tree_item setCheckBoxState:check_box_state];

	NSTableCellView* table_cell_view = [tree_item tableCellView];
	// Because the user might try to dynamically switch the type after creation (not allowed by spec), we should add a safety check.
	// Also, I think it might be possible that the item is off-screen so there is no actual cell available to set
	if((nil != table_cell_view) && [table_cell_view respondsToSelector:@selector(checkBox)])
	{
		IupCocoaTreeToggleTableCellView* toggle_cell_view = (IupCocoaTreeToggleTableCellView*)table_cell_view;
		NSButton* check_box = [toggle_cell_view checkBox];
		[check_box setState:check_box_state];
	}
	return 0;
}

static char* cocoaTreeGetToggleVisibleAttrib(Ihandle* ih, int item_id)
{
	if(ih->data->show_toggle < 1)
	{
		return NULL;
	}
  	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return NULL;
	}
	
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	// it happens that iupStrReturnChecked uses the same values for mixed, off, and on
	bool check_box_hidden = [tree_item isCheckBoxHidden];
	return iupStrReturnBoolean((int)!check_box_hidden);
}

static int cocoaTreeSetToggleVisibleAttrib(Ihandle* ih, int item_id, const char* value)
{
	if(ih->data->show_toggle < 1)
	{
		return 0;
	}
  	InodeHandle* inode_handle = iupTreeGetNode(ih, item_id);
	if(NULL == inode_handle)
	{
		return 0;
	}
	
	IupCocoaTreeItem* tree_item = (IupCocoaTreeItem*)inode_handle;
	bool check_box_hidden = !iupStrBoolean(value);
	
	
	[tree_item setCheckBoxHidden:check_box_hidden];

	NSTableCellView* table_cell_view = [tree_item tableCellView];
	// Because the user might try to dynamically switch the type after creation (not allowed by spec), we should add a safety check.
	// Also, I think it might be possible that the item is off-screen so there is no actual cell available to set
	if((nil != table_cell_view) && [table_cell_view respondsToSelector:@selector(checkBox)])
	{
		IupCocoaTreeToggleTableCellView* toggle_cell_view = (IupCocoaTreeToggleTableCellView*)table_cell_view;
		NSButton* check_box = [toggle_cell_view checkBox];
		[check_box setHidden:!check_box_hidden];
	}
	return 0;
}




static int cocoaTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
	NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
	bool should_expand = iupStrBoolean(value);
	
	if(should_expand)
	{
		// item=nil should do all root items
		[outline_view expandItem:nil expandChildren:YES];
	}
	else
	{
		// item=nil should do all root items
		[outline_view collapseItem:nil collapseChildren:YES];
	}

  return 0;
}

static int cocoaTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
	bool show_rename = iupStrBoolean(value);
	ih->data->show_rename = show_rename;
	
	// Because this is needed for Map, we might not have a handle yet
	if(ih->handle && ih->data->show_rename)
	{
		NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
		
		NSIndexSet* selected_index = [outline_view selectedRowIndexes];
		NSUInteger selected_i = [selected_index firstIndex];
		if(selected_i != NSNotFound)
		{
			id selected_item = [outline_view itemAtRow:selected_i];
			NSTableCellView* table_cell_view = [selected_item tableCellView];
			NSTextField* text_field = [table_cell_view textField];
			[text_field becomeFirstResponder];
		}
	}
	(void)value;
	return 0;
}

static int cocoaTreeSetShowRenameAttrib(Ihandle* ih, const char* value)
{
	bool show_rename = iupStrBoolean(value);
	ih->data->show_rename = show_rename;
	
	// Because this is needed for Map, we might not have a handle yet
	if(ih->handle)
	{
		NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
		// I don't know how to traverse through all active cells.
		// Apple may only loads cells that are currently visible.
		// It appears that reloadData triggers all the cells to be recreated so we can call setEditable there.
		[outline_view reloadData];
	}
	return 0;
}

// TODO: Individual cells and maybe columns may be able to get their own context menus.
// For now, we do the simple thing of applying a singular menu for the entire OutlineView.
// Several ideas (unconfirmed):
// - https://stackoverflow.com/questions/1309602/how-do-you-add-context-senstive-menu-to-nsoutlineview-ie-right-click-menu
// - attach a menu to each cell view or row view
// - override menuForEvent: in the NSOutlineView subclass
// - 10.11 APIs? https://stackoverflow.com/questions/12494489/nstableview-right-clicked-row-index
static int cocoaTreeSetContextMenuAttrib(Ihandle* ih, const char* value)
{
	Ihandle* menu_ih = (Ihandle*)value;
	NSOutlineView* outline_view = cocoaTreeGetOutlineView(ih);
	iupCocoaCommonBaseSetContextMenuForWidget(ih, outline_view, menu_ih);
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
	[cocoaTreeGetRootView(ih) setWantsLayer:layer_state];
	[cocoaTreeGetOutlineView(ih) setWantsLayer:layer_state];

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
	if(was_enabled == enable)
	{
		return 0;
	}
	[outline_view setEnabled:enable];
	[outline_view reloadData];

	
	return 0;
}


static int cocoaTreeMapMethod(Ihandle* ih)
{
	NSBundle* framework_bundle = [NSBundle bundleWithIdentifier:@"br.puc-rio.tecgraf.iup"];
	
	NSNib* outline_nib = nil;
	
	// 0 for no toggle, 1 for toggle, 2 for toggle with 3-state
	if(ih->data->show_toggle > 0)
	{
		outline_nib = [[NSNib alloc] initWithNibNamed:@"IupCocoaOutlineViewToggle" bundle:framework_bundle];
	}
	else
	{
		outline_nib = [[NSNib alloc] initWithNibNamed:@"IupCocoaOutlineView" bundle:framework_bundle];
	}

	
	NSArray* top_level_objects = nil;
	
	
	IupCocoaOutlineView* outline_view = nil;
	NSScrollView* scroll_view = nil;
	
	if([outline_nib instantiateWithOwner:nil topLevelObjects:&top_level_objects])
	{
		for(id current_object in top_level_objects)
		{

			if([current_object isKindOfClass:[NSScrollView class]])
			{
				scroll_view = current_object;
				break;
			}
		}
	}
	
	outline_view = (IupCocoaOutlineView*)[scroll_view documentView];
	NSCAssert([outline_view isKindOfClass:[IupCocoaOutlineView class]], @"Expected IupCocoaOutlineView");
	
	// ScrollView is expected to hold on to all the other objects we need
	[scroll_view retain];
	[outline_nib release];
	
	// We need a way to get the ih during Cocoa callbacks, such as for selection changed notifications.
	[outline_view setIh:ih];


	IupCocoaTreeDelegate* tree_delegate = nil;
	// This line is not working. (Why not?) Use ih->data->show_dragdrop instead.
//	if(iupAttribGetInt(ih, "SHOWDRAGDROP")==1)
	if(ih->data->show_dragdrop || IupGetInt(ih, "DRAGDROPTREE"))
	{
		tree_delegate = [[IupCocoaTreeDragDropDelegate alloc] init];
	}
	else
	{
		tree_delegate = [[IupCocoaTreeDelegate alloc] init];

	}
	
	[outline_view setDataSource:tree_delegate];
	[outline_view setDelegate:tree_delegate];
	

	
	// We're going to use OBJC_ASSOCIATION_RETAIN because I do believe it will do the right thing for us.
	// I'm attaching to the scrollview instead of the outline view because I'm a little worried about circular references and I'm hoping this helps a little
	objc_setAssociatedObject(scroll_view, IUP_COCOA_TREE_DELEGATE_OBJ_KEY, (id)tree_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[tree_delegate release];
	
	ih->handle = scroll_view;
	iupCocoaSetAssociatedViews(ih, outline_view, scroll_view);

	
	
	// All Cocoa views shoud call this to add the new view to the parent view.
	iupCocoaAddToParent(ih);
	
	
	
	// turn off the header
	[outline_view setHeaderView:nil];

	/* Initialize the default images */
	NSImage* leaf_image = iupImageGetImage(iupAttribGetStr(ih, "IMAGELEAF"), ih, 0, NULL);
//	NSImage* collapsed_image = iupImageGetImage(iupAttribGetStr(ih, "IMAGEBRANCHCOLLAPSED"), ih, 0);
//	NSImage* expanded_image = iupImageGetImage(iupAttribGetStr(ih, "IMAGEBRANCHEXPANDED"), ih, 0);




	[outline_view setLeafImage:leaf_image];

#if 1
	helperReplaceDefaultImages(ih, outline_view);
#else
	NSImage* collapsed_image = [NSImage imageNamed:NSImageNameFolder];
//	NSImage* expanded_image = [NSImage imageNamed:NSImageNameFolder];
	NSImage* expanded_image = collapsed_image;
	
	[outline_view setCollapsedImage:collapsed_image];
	[outline_view setExpandedImage:expanded_image];
#endif

	if (iupAttribGetInt(ih, "ADDROOT"))
	{
		iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);
	}
	
	/* configure for DRAG&DROP of files */
	if (IupGetCallback(ih, "DROPFILES_CB"))
	{
		iupAttribSet(ih, "DROPFILESTARGET", "YES");
	}
	
//	IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)cocoaTreeConvertXYToPos);
	
	iupdrvTreeUpdateMarkMode(ih);

	
	
	// Registering a custom type so we can do internal drag-and-drop (reordering)
	[outline_view registerForDraggedTypes:[NSArray arrayWithObjects:IUPCOCOA_OUTLINEVIEW_DRAGANDDROP_TYPE, nil]];

	
	
	return IUP_NOERROR;
}

static void cocoaTreeUnMapMethod(Ihandle* ih)
{
	id root_view = ih->handle;
	
	// Destroy the context menu ih it exists
	{
		Ihandle* context_menu_ih = (Ihandle*)iupCocoaCommonBaseGetContextMenuAttrib(ih);
		if(NULL != context_menu_ih)
		{
			IupDestroy(context_menu_ih);
		}
		iupCocoaCommonBaseSetContextMenuAttrib(ih, NULL);
	}


	IupCocoaOutlineView* outline_view = (IupCocoaOutlineView*)cocoaTreeGetOutlineView(ih);
	[outline_view setMarkStartNode:nil];


	iupCocoaSetAssociatedViews(ih, nil, nil);
	iupCocoaRemoveFromParent(ih);
	[root_view release];
	ih->handle = NULL;
}




void iupdrvTreeInitClass(Iclass* ic)
{
	/* Driver Dependent Class functions */
	ic->Map = cocoaTreeMapMethod;
	ic->UnMap = cocoaTreeUnMapMethod;
#if 0
	
	/* Visual */
	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
#endif
	/* IupTree Attributes - GENERAL */
	iupClassRegisterAttribute(ic, "EXPANDALL", NULL, cocoaTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
#if 0
	iupClassRegisterAttribute(ic, "INDENTATION", cocoaTreeGetIndentationAttrib, cocoaTreeSetIndentationAttrib, NULL, NULL, IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, cocoaTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
#endif
	iupClassRegisterAttribute(ic, "TOPITEM", NULL, cocoaTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

	/* IupTree Attributes - IMAGES */
	iupClassRegisterAttributeId(ic, "IMAGE", NULL, cocoaTreeSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, cocoaTreeSetImageExpandedAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	
	iupClassRegisterAttribute(ic, "IMAGELEAF",            NULL, cocoaTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, cocoaTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED",  NULL, cocoaTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);


	/* IupTree Attributes - NODES */
	iupClassRegisterAttributeId(ic, "STATE",  cocoaTreeGetStateAttrib,  cocoaTreeSetStateAttrib, IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "DEPTH",  cocoaTreeGetDepthAttrib,  NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "KIND",   cocoaTreeGetKindAttrib,   NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "PARENT", cocoaTreeGetParentAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "NEXT", cocoaTreeGetNextAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "LAST", cocoaTreeGetLastAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "PREVIOUS", cocoaTreeGetPreviousAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "FIRST", cocoaTreeGetFirstAttrib, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
 #if 0
	iupClassRegisterAttributeId(ic, "COLOR",  cocoaTreeGetColorAttrib,  cocoaTreeSetColorAttrib, IUPAF_NO_INHERIT);
#endif
	
	iupClassRegisterAttributeId(ic, "TITLE",  cocoaTreeGetTitleAttrib,  cocoaTreeSetTitleAttrib, IUPAF_NO_INHERIT);
	
	iupClassRegisterAttributeId(ic, "TOGGLEVALUE", cocoaTreeGetToggleValueAttrib, cocoaTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", cocoaTreeGetToggleVisibleAttrib, cocoaTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);

	/* Change the set method for Cocoa */
	iupClassRegisterReplaceAttribFunc(ic, "SHOWRENAME", NULL, cocoaTreeSetShowRenameAttrib);

	iupClassRegisterAttribute(ic, "ROOTCOUNT", cocoaTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "CHILDCOUNT", cocoaTreeGetChildCountAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
#if 0
	iupClassRegisterAttributeId(ic, "TITLEFONT",  cocoaTreeGetTitleFontAttrib,  cocoaTreeSetTitleFontAttrib, IUPAF_NO_INHERIT);
#endif

	/* IupTree Attributes - MARKS */
	iupClassRegisterAttributeId(ic, "MARKED", cocoaTreeGetMarkedAttrib, cocoaTreeSetMarkedAttrib, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute  (ic, "MARK",      NULL, cocoaTreeSetMarkAttrib,      NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute  (ic, "STARTING",  NULL, cocoaTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute  (ic, "MARKSTART", NULL, cocoaTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute  (ic, "MARKEDNODES", cocoaTreeGetMarkedNodesAttrib, cocoaTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
#if 0
	iupClassRegisterAttribute(ic, "MARKWHENTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
#endif

	iupClassRegisterAttribute  (ic, "VALUE", cocoaTreeGetValueAttrib, cocoaTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	// The default implementation does a bunch of things that I don't think do anything useful for Cocoa. So I'm overriding/disabling the default implementation.
//	iupClassRegisterAttribute(ic, "DRAGDROPTREE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterReplaceAttribFunc(ic, "DRAGDROPTREE", NULL, NULL);

	iupClassRegisterReplaceAttribFunc(ic, "ACTIVE", cocoaTreeGetActiveAttrib, cocoaTreeSetActiveAttrib);

	/* IupTree Attributes - ACTION */
	iupClassRegisterAttributeId(ic, "DELNODE", NULL, cocoaTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "RENAME", NULL, cocoaTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "MOVENODE", NULL, cocoaTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "COPYNODE", NULL, cocoaTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	
#if 0
	/* IupTree Attributes - GTK Only */
	iupClassRegisterAttribute  (ic, "RUBBERBAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
#endif

	/* New API for view specific contextual menus (Mac only) */
	iupClassRegisterAttribute(ic, "CONTEXTMENU", iupCocoaCommonBaseGetContextMenuAttrib, cocoaTreeSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "LAYERBACKED", cocoaTreeGetLayerBackedAttrib, cocoaTreeSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);


	helperLoadReplacementDefaultImages();
}
