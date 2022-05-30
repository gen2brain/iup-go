/** \file
 * \brief List Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Cocoa/Cocoa.h>
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
#include "iup_mask.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_list.h"

#include "iupcocoa_drv.h"

static const CGFloat kIupCocoaDefaultWidthNSPopUpButton = 1.0;
static const CGFloat kIupCocoaDefaultHeightNSPopUpButton = 26.0;
static const CGFloat kIupCocoaDefaultWidthNSComboBox = 1.0;
static const CGFloat kIupCocoaDefaultHeightNSComboBox = 26.0;
static const CGFloat kIupCocoaDefaultHeightNSTableViewCell = 17.0;

// the point of this is we have a unique memory address for an identifier
static const void* IUP_COCOA_LIST_POPUPBUTTON_RECEIVER_OBJ_KEY = "IUP_COCOA_LIST_POPUPBUTTON_RECEIVER_OBJ_KEY";
static const void* IUP_COCOA_LIST_COMBOBOX_RECEIVER_OBJ_KEY = "IUP_COCOA_LIST_COMBOBOX_RECEIVER_OBJ_KEY";
static const void* IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY = "IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY";

typedef enum
{
	IUPCOCOALISTSUBTYPE_UNKNOWN = -1,
	IUPCOCOALISTSUBTYPE_DROPDOWN,
	IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN,
	IUPCOCOALISTSUBTYPE_EDITBOX,
	IUPCOCOALISTSUBTYPE_MULTIPLELIST,
	IUPCOCOALISTSUBTYPE_SINGLELIST // not official, but could be the basis for mobile-style listviews
} IupCocoaListSubType;

/*
Each IUP list subtype requires a completely different Cocoa native widget.
This function provides a consistent and centralized way to distinguish which subtype we need.
*/
static IupCocoaListSubType cocoaListGetSubType(Ihandle* ih)
{
	if(ih->data->is_dropdown)
	{
		if(ih->data->has_editbox)
		{
			return IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN;
		}
		else
		{
			return IUPCOCOALISTSUBTYPE_DROPDOWN;
		}
	}
	else
	{
		if(ih->data->has_editbox)
		{
			return IUPCOCOALISTSUBTYPE_EDITBOX;
		}
		else if(ih->data->is_multiple)
		{
			return IUPCOCOALISTSUBTYPE_MULTIPLELIST;

		}
		else
		{
			return IUPCOCOALISTSUBTYPE_SINGLELIST;
		}
	}
	return IUPCOCOALISTSUBTYPE_UNKNOWN;
}

/*
The normal convention is to put the native widget in ih->handle,
but because we have to embed NSTableView in NSScrollView for some cases,
ih->handle is simply the root_view, but not necessarily the base "real" widget.
So this helper function returns the base widget (e.g. NSTableView instead of NSScrollView)
in which we can use to invoke all the important methods we need to call.
IUPCOCOALISTSUBTYPE_DROPDOWN returns NSPopUpButton
IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN returns NSComboBox
IUPCOCOALISTSUBTYPE_MULTIPLELIST returns NSTableView
IUPCOCOALISTSUBTYPE_SINGLELIST returns NSTableView

*/
static NSView* cocoaListGetBaseWidget(Ihandle* ih)
{
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	NSView* root_widget = (NSView*)ih->handle;
	
	if(nil == root_widget)
	{
		NSLog(@"Warning: cocoaListGetBaseWidget is nil");
		return nil;
	}
	 
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			NSCAssert([root_widget isKindOfClass:[NSPopUpButton class]], @"Expecting a NSPopUpButton");
			return root_widget;
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			NSCAssert([root_widget isKindOfClass:[NSComboBox class]], @"Expecting a NSComboBox");
			return root_widget;
			break;
		}
		// both these cases may have an NSScrollView at the root
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			if([root_widget isKindOfClass:[NSTableView class]])
			{
				return root_widget;
			}
			else if([root_widget isKindOfClass:[NSScrollView class]])
			{
				NSView* base_widget = [(NSScrollView*)root_widget documentView];
				return base_widget;
			}
			else
			{
				NSCAssert(0, @"Unexpected subtype");
				return root_widget;

			}

			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			return root_widget;
			break;
		}
		default:
		{
			NSCAssert(0, @"Unexpected subtype");
			return root_widget;
			break;
		}
	}
	

}

// The sole reason was subclass NSComboBox is so we can suppor the contextual menu
@interface IupCocoaComboBox : NSComboBox
- (NSMenu *)textView:(NSTextView *)text_view menu:(NSMenu *)the_menu forEvent:(NSEvent *)the_event atIndex:(NSUInteger)char_index;
@end

@implementation IupCocoaComboBox
// WARNING: This was the only way I could figure out how to correctly override the contextual menu for NSTextField.
// I tried setting the menu (on demand) of the field editor, but I ended up getting lots of missing items.
// I tried creating my own field editor and setting a custom delegate for this method on it, but it never got invoked. I think Cocoa may ignore my delegate when used as a field editor.
// I found a mention on the internet that vaguely suggested subclassing NSTextField and implementing this method.
// I have not found documentation about this method in NSTextField, only as a delegate protocol for NSTextView.
// This might mean this is private API so this may have to be disabled.
- (NSMenu *)textView:(NSTextView *)text_view menu:(NSMenu *)the_menu forEvent:(NSEvent *)the_event atIndex:(NSUInteger)char_index
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
	Ihandle* menu_ih = (Ihandle*)iupAttribGet(ih, "_COCOA_CONTEXT_MENU_IH");
	if(NULL != menu_ih)
	{
		NSMenu* user_menu = (NSMenu*)menu_ih->handle;
		iupCocoaCommonBaseAppendMenuItems(the_menu, user_menu);
		// It appears that Cocoa may append "Services" after our entries if something is selected, so we want another separator.
		NSRange selected_range = [text_view selectedRange];
		if(selected_range.length > 0)
		{
			[the_menu addItem:[NSMenuItem separatorItem]];
		}
	}
	return the_menu;
}
@end


@interface IupCocoaListPopupButtonReceiver : NSObject
- (IBAction) onSelectionChanged:(id)the_sender;
@end

@implementation IupCocoaListPopupButtonReceiver

/*
- (void) dealloc
{
	[super dealloc];
}
*/


- (IBAction) onSelectionChanged:(id)the_sender;
{
//	Icallback callback_function;
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);


	
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
			if(cb)
			{
				NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
				NSInteger index_of_selected_item = [popup_button indexOfSelectedItem];
				int adjusted_index = (int)(index_of_selected_item+1); /* IUP starts at 1 */
				iupListSingleCallActionCb(ih, cb, adjusted_index);
				
				iupBaseCallValueChangedCb(ih);

			}
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{

			break;
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		{
			iupBaseCallValueChangedCb(ih);

			break;
		}
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			iupBaseCallValueChangedCb(ih);

			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}
	


}

@end


// We are not using NSComboBoxDataSource
@interface IupCocoaListComboBoxReceiver : NSObject <NSComboBoxDelegate>


// NSComboBoxDelegate
/* Notifications */
//- (void)comboBoxWillPopUp:(NSNotification *)notification;
//- (void)comboBoxWillDismiss:(NSNotification *)notification;
- (void) comboBoxSelectionDidChange:(NSNotification*)the_notification;
//- (void)comboBoxSelectionIsChanging:(NSNotification *)notification;


@end

@implementation IupCocoaListComboBoxReceiver

/*
 - (void) dealloc
 {
	[super dealloc];
 }
 */


- (void) comboBoxSelectionDidChange:(NSNotification*)the_notification
{
	NSComboBox* combo_box = [the_notification object];
	
	NSLog(@"combo_box frame: %@", NSStringFromRect([combo_box frame]));
	
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(combo_box, IHANDLE_ASSOCIATED_OBJ_KEY);
	
	IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
	if(cb)
	{

		
		NSInteger index_of_selected_item = [combo_box indexOfSelectedItem];
		int adjusted_index = (int)(index_of_selected_item+1); /* IUP starts at 1 */
		iupListSingleCallActionCb(ih, cb, adjusted_index);

	}
	

	/*
	if (!ih->data->has_editbox)
	{
		iupBaseCallValueChangedCb(ih);
	}
	
	*/
	
	
}

@end


// We are not using NSComboBoxDataSource
@interface IupCocoaListTableViewReceiver : NSObject <NSTableViewDelegate, NSTableViewDataSource>
{
	NSMutableArray* dataArray;
}

- (NSMutableArray*) dataArray;


// NSTableViewDataSource
- (NSInteger) numberOfRowsInTableView:(NSTableView*)table_view;
/* This method is required for the "Cell Based" TableView, and is optional for the "View Based" TableView. If implemented in the latter case, the value will be set to the view at a given row/column if the view responds to -setObjectValue: (such as NSControl and NSTableCellView).
 */
//- (nullable id) tableView:(NSTableView*)table_view objectValueForTableColumn:(nullable NSTableColumn*)table_column row:(NSInteger)the_row;
- (nullable NSView*) tableView:(NSTableView*)table_view viewForTableColumn:(NSTableColumn*)table_column row:(NSInteger)the_row;


// NSTableViewDelegate

- (void) tableViewSelectionDidChange:(NSNotification*)the_notification;

@end

@implementation IupCocoaListTableViewReceiver

- (instancetype) init
{
	self = [super init];
	if(self)
	{
		dataArray = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void) dealloc
{
	[dataArray release];
	[super dealloc];
}

- (NSMutableArray*) dataArray
{
	return dataArray;
}


- (NSInteger) numberOfRowsInTableView:(NSTableView*)table_view
{
	IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
	NSMutableArray* data_array = [list_receiver dataArray];
	
	return [data_array count];
	
}


- (nullable NSView*) tableView:(NSTableView*)table_view viewForTableColumn:(NSTableColumn*)table_column row:(NSInteger)the_row
{
	IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
	NSMutableArray* data_array = [list_receiver dataArray];
	
	NSString* string_item = [data_array objectAtIndex:the_row];
	
	
	
	// Get an existing cell with the MyView identifier if it exists
    NSTextField* the_result = [table_view makeViewWithIdentifier:@"IupCocoaListTableViewCell" owner:self];
 
    // There is no existing cell to reuse so create a new one
    if(nil == the_result)
	{
 
		// Create the new NSTextField with a frame of the {0,0} with the width of the table.
		// Note that the height of the frame is not really relevant, because the row height will modify the height.
//		the_result = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, kIupCocoaDefaultWidthNSPopUpButton, kIupCocoaDefaultHeightNSPopUpButton)];
		the_result = [[NSTextField alloc] initWithFrame:NSZeroRect];
		[the_result setBezeled:NO];
		[the_result setDrawsBackground:NO];
		[the_result setEditable:NO];
		//			[the_label setSelectable:NO];
		// TODO: FEATURE: I think this is really convenient for users
		[the_result setSelectable:YES];
		
		// The identifier of the NSTextField instance is set to MyView.
		// This allows the cell to be reused.
		[the_result setIdentifier:@"IupCocoaListTableViewCell"];
		[the_result setFont:[NSFont systemFontOfSize:0.0]];
	}
 
	// result is now guaranteed to be valid, either as a reused cell
	// or as a new cell, so set the stringValue of the cell to the
	// nameArray value at row
	[the_result setStringValue:string_item];
 
	// Return the result
	return the_result;
 

	
	
	
}


- (void) tableViewSelectionDidChange:(NSNotification*)the_notification
{
	NSTableView* table_view = [the_notification object];
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(table_view, IHANDLE_ASSOCIATED_OBJ_KEY);
	
	IFnsii cb = (IFnsii)IupGetCallback(ih, "ACTION");
	if(cb)
	{
		
		
		NSInteger index_of_selected_item = [table_view selectedRow];
		int adjusted_index = (int)(index_of_selected_item+1); /* IUP starts at 1 */
		iupListSingleCallActionCb(ih, cb, adjusted_index);
		
	}
	
	
	/*
	 if (!ih->data->has_editbox)
	 {
		iupBaseCallValueChangedCb(ih);
	 }
	 
	 */
	
	
}

@end


void iupdrvListAddItemSpace(Ihandle* ih, int* h)
{
//	*h += 2;
	
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
			break;
			
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			break;
			
		}
			
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}

}

void iupdrvListAddBorders(Ihandle* ih, int *x, int *y)
{
//	NSLog(@"iupdrvListAddBorders <%d, %d>", *x, *y);
#if 0
  int border_size = 2*10;
  (*x) += border_size;
  (*y) += border_size;

  if (ih->data->is_dropdown)
  {
    (*x) += 5; /* extra space for the dropdown button */

    if (ih->data->has_editbox)
	{
      (*x) += 5; /* another extra space for the dropdown button */
		(*y) += 7; /* extra padding space */
	}
    else
    {
      (*y) += 4; /* extra padding space */
      (*x) += 4; /* extra padding space */
    }
  }
  else
  {
    if (ih->data->is_multiple)
	{
		(*x) += 2*80; /* internal border between editbox and list */

      (*y) += 2*40; /* internal border between editbox and list */
		
		(*x) = 200;
		(*y) = 200;
	}
  }
#else
	// WARNING: I discovered this can be called before Map, hence we have no nativehandle. Thus cocoaListGetBaseWidget will return nil.
	
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
//			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
			// Heights are fixed in Interface Builder. You generally aren't supposed to mess with the widget heights or font sizes.
			if(*y < (int)kIupCocoaDefaultHeightNSPopUpButton)
			{
				*y = (int)kIupCocoaDefaultHeightNSPopUpButton;
			}
			*x += 4; // a regular label seems to get 2 padding on each size
			*x += 33; // the difference between a label and popup is 33 in Interface Builder
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
//			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
			// Heights are fixed in Interface Builder. You generally aren't supposed to mess with the widget heights or font sizes.
			if(*y < (int)kIupCocoaDefaultHeightNSComboBox)
			{
				*y = (int)kIupCocoaDefaultHeightNSComboBox;
			}
			*x += 4; // a regular label seems to get 2 padding on each size
			*x += 24; // the difference between a label and combobox is 24 in Interface Builder
			break;
			
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			// FIXME: cocoaListGetBaseWidget might return nil. Could using NIBs help get us a valie tablie view to get rowHeight and other things?
			
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			

			// rectOfRow would tell us the height of a specific row
			// The actual -rectOfRow: is equal to the -rowHeight plus the intercellSpacing.height. The default value is 17.0 for applications linked on 10.5 and higher (the height acceptable for [NSFont systemFontSize]). The default value is 16.0 for 10.4 and lower.
			
			// I don't know which row we're at, but we'll assume they are all the same height.
			// rectOfRow is not helpful if the table is empty, so use intercellSpacing.height+rowHeight
			CGFloat row_height = [table_view rowHeight] + [table_view intercellSpacing].height;
			
			
			int visible_lines = 0; // 5 is the default according to the docs
			if(iupAttribGet(ih, "VISIBLELINES"))
			{
				visible_lines = iupAttribGetInt(ih, "VISIBLELINES");
			}
			else
			{
				visible_lines = (int)[table_view numberOfRows];
			}
			
			// Note: Don't add any extra padding here or the table will get clipped.
			CGFloat view_height = row_height * (CGFloat)visible_lines;
			
			*y = iupROUND(view_height);
			
			
			*x += 4; // a regular label seems to get 2 padding on each size
			*x += 17; // the difference between a label and table is 17 in Interface Builder
			
			break;
			
		}
			
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}
#endif
	
}

int iupdrvListGetCount(Ihandle* ih)
{
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
			NSInteger number_of_items = [[popup_button menu] numberOfItems];
			return (int)number_of_items;
			
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
			NSInteger number_of_items = [combo_box numberOfItems];
			return (int)number_of_items;
			
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			NSInteger number_of_items = [table_view numberOfRows];
			return (int)number_of_items;
			
		}

		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}
	return 0;

	
}

void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
			NSString* ns_string = [NSString stringWithUTF8String:value];
			[[popup_button menu] addItemWithTitle:ns_string action:nil keyEquivalent:@""];
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
			NSString* ns_string = [NSString stringWithUTF8String:value];
			[combo_box addItemWithObjectValue:ns_string];
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
			NSMutableArray* data_array = [list_receiver dataArray];
			
			NSString* ns_string = [NSString stringWithUTF8String:value];
			[data_array addObject:ns_string];
	
#if 0
			// reloadData will do the job, but may lose user's currently selected items
			// reloadDataForRowIndexes might be better and more optimized,
			// however, I don't know how IUP's SORT feature interacts with everything. (Append may no longer be put in the last row. (and not to mention that I haven't actually figured out how to make it work.)
			[table_view reloadData];
#else
			NSUInteger data_count = [data_array count];
			NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:data_count-1];
			[table_view insertRowsAtIndexes:index_set withAnimation:NSTableViewAnimationEffectNone];
			
//			[table_view reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:data_count-1] columnIndexes:[NSIndexSet indexSetWithIndex:0]];
//			[table_view reloadDataForRowIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, data_count)] columnIndexes:[NSIndexSet indexSetWithIndex:0]];



#endif
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}

	
}

void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
			NSString* ns_string = [NSString stringWithUTF8String:value];
			[[popup_button menu] insertItemWithTitle:ns_string action:nil keyEquivalent:@"" atIndex:pos];
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
			NSString* ns_string = [NSString stringWithUTF8String:value];
			[combo_box insertItemWithObjectValue:ns_string atIndex:pos];
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
			NSMutableArray* data_array = [list_receiver dataArray];
			
			NSString* ns_string = [NSString stringWithUTF8String:value];
			[data_array insertObject:ns_string atIndex:pos];

#if 0
			// reloadData will do the job, but may lose user's currently selected items
			// reloadDataForRowIndexes might be better and more optimized,
			// however, I don't know how IUP's SORT feature interacts with everything. (Append may no longer be put in the last row. (and not to mention that I haven't actually figured out how to make it work.)
			[table_view reloadData];
#else
			NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:pos];
			[table_view insertRowsAtIndexes:index_set withAnimation:NSTableViewAnimationEffectNone];
			//			[table_view reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:data_count-1] columnIndexes:[NSIndexSet indexSetWithIndex:0]];
//			[table_view reloadDataForRowIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, data_count)] columnIndexes:[NSIndexSet indexSetWithIndex:0]];
#endif
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}

	
}

void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
			[[popup_button menu] removeItemAtIndex:pos];
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
			[combo_box removeItemAtIndex:pos];
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
			NSMutableArray* data_array = [list_receiver dataArray];
			
			[data_array removeObjectAtIndex:pos];
			
#if 0
			// reloadData will do the job, but may lose user's currently selected items
			// reloadDataForRowIndexes might be better and more optimized,
			// however, I don't know how IUP's SORT feature interacts with everything. (Append may no longer be put in the last row. (and not to mention that I haven't actually figured out how to make it work.)
			[table_view reloadData];
#else
			NSIndexSet* index_set = [NSIndexSet indexSetWithIndex:pos];
			[table_view removeRowsAtIndexes:index_set withAnimation:NSTableViewAnimationEffectNone];
#endif
			
//			NSUInteger data_count = [data_array count];
//			[table_view reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:data_count-1] columnIndexes:[NSIndexSet indexSetWithIndex:0]];
//			[table_view reloadDataForRowIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, data_count)] columnIndexes:[NSIndexSet indexSetWithIndex:0]];

			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}

}

void iupdrvListRemoveAllItems(Ihandle* ih)
{
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
			[[popup_button menu] removeAllItems];
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
			[combo_box removeAllItems];
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
			NSMutableArray* data_array = [list_receiver dataArray];
			
			[data_array removeAllObjects];
			
			// reloadData will do the job, but may lose user's currently selected items
			// reloadDataForRowIndexes might be better and more optimized,
			// however, I don't know how IUP's SORT feature interacts with everything. (Append may no longer be put in the last row. (and not to mention that I haven't actually figured out how to make it work.)
			[table_view reloadData];
			
			//			NSUInteger data_count = [data_array count];
			//			[table_view reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:data_count-1] columnIndexes:[NSIndexSet indexSetWithIndex:0]];
			//			[table_view reloadDataForRowIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, data_count)] columnIndexes:[NSIndexSet indexSetWithIndex:0]];
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}

	
}


void* iupdrvListGetImageHandle(Ihandle* ih, int id)
{

	
    return NULL;

}

int iupdrvListSetImageHandle(Ihandle* ih, int id, void* hImage)
{

	
  return 0;
}


// FIXME: I don't know what this is actually supposed to do and don't know how to trigger it in the tests.
static char* cocoaListGetIdValueAttrib(Ihandle* ih, int id_value)
{
	int pos = iupListGetPosAttrib(ih, id_value);
	if(pos >= 0)
	{

		IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
		switch(sub_type)
		{
		  case IUPCOCOALISTSUBTYPE_DROPDOWN:
		  {
			  NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
			  NSString* ns_string = [popup_button itemTitleAtIndex:pos];
			  
			  const char* c_str = [ns_string UTF8String];
			  char* iup_str = iupStrReturnStr(c_str);
			  return iup_str;
			  
			  break;
		  }
		  case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		  {
			  NSComboBox* popup_button = (NSComboBox*)cocoaListGetBaseWidget(ih);
			  id object_value = [popup_button itemObjectValueAtIndex:pos];
			  NSString* ns_string = nil;
			  
			  if([object_value isKindOfClass:[NSString class]])
			  {
				  ns_string = (NSString*)object_value;
			  }
			  else
			  {
				  ns_string = [object_value stringValue];
				  
			  }
			  
			  const char* c_str = [ns_string UTF8String];
			  char* iup_str = iupStrReturnStr(c_str);
			  return iup_str;
			  
			  break;
		  }
		  case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		  case IUPCOCOALISTSUBTYPE_SINGLELIST:
		  {
			  NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			  IupCocoaListTableViewReceiver* list_receiver = objc_getAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
			  NSMutableArray* data_array = [list_receiver dataArray];
			  id object_value = [data_array objectAtIndex:pos];
			  NSString* ns_string = nil;
			  
			  if([object_value isKindOfClass:[NSString class]])
			  {
				  ns_string = (NSString*)object_value;
			  }
			  else
			  {
				  ns_string = [object_value stringValue];
				  
			  }
			  
			  const char* c_str = [ns_string UTF8String];
			  char* iup_str = iupStrReturnStr(c_str);
			  return iup_str;
			  
			  break;
		  }
		  case IUPCOCOALISTSUBTYPE_EDITBOX:
		  {
			  break;
		  }
		  default:
		  {
			  break;
		  }
		}

	}
	return NULL;
}


static char* cocoaListGetValueAttrib(Ihandle* ih)
{
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
			NSInteger index_of_selected_item = [popup_button indexOfSelectedItem];
			int adjusted_index = (int)(index_of_selected_item+1); /* IUP starts at 1 */
			return iupStrReturnInt(adjusted_index);
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
			NSString* ns_string = [combo_box stringValue];
			
			const char* c_str = [ns_string UTF8String];
			char* iup_str = iupStrReturnStr(c_str);
			return iup_str;
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			size_t count;
			
			NSCAssert([table_view numberOfRows] == iupdrvListGetCount(ih), @"count is inconsistent");
			count = (size_t)[table_view numberOfRows];
			
			char* ret_str = iupStrGetMemory((int)(count+1));
			memset(ret_str, '-', count);
			ret_str[count]='\0';


			NSIndexSet* selected_index = [table_view selectedRowIndexes];
			NSUInteger selected_i = [selected_index firstIndex];
			while(selected_i != NSNotFound)
			{
				ret_str[selected_i] = '+';
				// get the next index in the set
				selected_i = [selected_index indexGreaterThanIndex:selected_i];
			}
		
			return ret_str;
			

			break;
		}
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			NSInteger index_of_selected_item = [table_view selectedRow];
			int adjusted_index = (int)(index_of_selected_item+1); /* IUP starts at 1 */
			return iupStrReturnInt(adjusted_index);
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}
	
	return NULL;
}

static int cocoaListSetValueAttrib(Ihandle* ih, const char* value)
{
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);

			int iup_pos;
			if(iupStrToInt(value, &iup_pos)==1)
			{
				NSInteger adjusted_index = (NSInteger)(iup_pos-1); /* IUP starts at 1 */
				[popup_button selectItemAtIndex:adjusted_index];
			}
			else
			{
				[popup_button selectItemAtIndex:-1];
			}
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
			NSString* ns_string = nil;
			if(NULL == value)
			{
				ns_string = @"";
			}
			else
			{
				ns_string = [NSString stringWithUTF8String:value];
			}

			[combo_box setStringValue:ns_string];
			[combo_box selectItemWithObjectValue:ns_string];
			
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		{
			
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			
			/* Clear all selections */
			[table_view deselectAll:nil];
			
			if(NULL != value)
			{
				/* User has changed a multiple selection on a simple list. */
				size_t len;
				size_t count;
				
				count = (size_t)[table_view numberOfRows];
				len = strlen(value);
				if(len < count)
				{
					count = (NSInteger)len;
				}
				
				/* update selection list */
				for(size_t i = 0; i<count; i++)
				{
					if (value[i]=='+')
					{
						[table_view selectRowIndexes:[NSIndexSet indexSetWithIndex:i] byExtendingSelection:YES];
					}
				}
			}
			
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			
			/* Clear all selections */
			[table_view deselectAll:nil];
			
			int iup_pos;
			if(iupStrToInt(value, &iup_pos)==1)
			{
				NSInteger adjusted_index = (NSInteger)(iup_pos-1); /* IUP starts at 1 */
				[table_view selectRowIndexes:[NSIndexSet indexSetWithIndex:adjusted_index] byExtendingSelection:NO];
			}
			else
			{
				[table_view deselectAll:nil];
			}
			
			
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}
	
	
	return 0;
}

static int cocoaListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
	if (ih->data->is_dropdown)
	{
#if 0
		if (iupStrBoolean(value))
			gtk_combo_box_popup((GtkComboBox*)ih->handle);
		else
			gtk_combo_box_popdown((GtkComboBox*)ih->handle);
#endif
	}
	return 0;
}



static int cocoaListSetInsertAttrib(Ihandle* ih, const char* value)
{
//	gint pos;
//	GtkEntry* entry;
	if (!ih->data->has_editbox)
		return 0;
	if (!value)
		return 0;

#if 0
	iupAttribSet(ih, "_IUPGTK_DISABLE_TEXT_CB", "1");  /* disable callbacks */
	entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK_ENTRY");
	pos = gtk_editable_get_position(GTK_EDITABLE(entry));
	gtk_editable_insert_text(GTK_EDITABLE(entry), iupgtkStrConvertToSystem(value), -1, &pos);
	iupAttribSet(ih, "_IUPGTK_DISABLE_TEXT_CB", NULL);
#endif
	
	return 0;
}

static int cocoaListSetAppendAttrib(Ihandle* ih, const char* value)
{
#if 0
	if (ih->data->has_editbox)
	{
		GtkEntry* entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK_ENTRY");
		gint pos = (gint)strlen(gtk_entry_get_text(entry))+1;
		iupAttribSet(ih, "_IUPGTK_DISABLE_TEXT_CB", "1"); /* disable callbacks */
		gtk_editable_insert_text(GTK_EDITABLE(entry), iupgtkStrConvertToSystem(value), -1, &pos);
		iupAttribSet(ih, "_IUPGTK_DISABLE_TEXT_CB", NULL);
	}
#endif
	return 0;
}



static int cocoaListSetSelectionAttrib(Ihandle* ih, const char* value)
{
#if 0
  int start=1, end=1;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return 0;
  if (!value)
    return 0;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK_ENTRY");
  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, 0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<1 || end<1) 
    return 0;

  start--; /* IUP starts at 1 */
  end--;

  gtk_editable_select_region(GTK_EDITABLE(entry), start, end);
#endif
  return 0;
}

static char* cocoaListGetSelectionAttrib(Ihandle* ih)
{
#if 0
int start, end;
  GtkEntry* entry;
  if (!ih->data->has_editbox)
    return NULL;

  entry = (GtkEntry*)iupAttribGet(ih, "_IUPGTK_ENTRY");
  if (gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &start, &end))
  {
    start++; /* IUP starts at 1 */
    end++;
    return iupStrReturnIntInt((int)start, (int)end, ':');
  }
#endif
  return NULL;
}

static int cocoaListSetContextMenuAttrib(Ihandle* ih, const char* value)
{
	Ihandle* menu_ih = (Ihandle*)value;
	
	
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			// We can't support this. This doesn't work. It replaces the contents of the dropdown menu with the contextual menu.
			// Official documentation on @property menu:
			// "Overrides behavior of NSView.  This is the menu for the popup, not a context menu.  PopUpButtons do not have context menus."
//			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);
//			iupCocoaCommonBaseSetContextMenuForWidget(ih, popup_button, menu_ih);
			NSLog(@"WARNING: CONTEXTMENU not available for DROPDOWN (NSPopUpButton)");
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			// I don't know how to support this. This gets into the field editor problem.
			// I only was able to solve the NSTextField by overriding a special method callback directly in a subclass instead of using the field editor's NSTextView delegate (which didn't work for field editors).
			// I might be able to subclass the NSComboBox because it is a NSTextField, but I'm worried it may intefere with the dropdown part like with NSPopUpButton.
			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);
			// We can't use iupCocoaCommonBaseSetContextMenuForWidget() because field editors are shared.
			// We will override textView:menu:forEvent:atIndex: and inject the menu options there.
			if(NULL != menu_ih)
			{
				// FIXME: The Menu might not be IupMap'd yet. (Presumably because we do not attach it directly to a dialog in this case.)
				// I think calling IupMap() is the correct thing to do and fixes the problem.
				// But this should be reviewed.
				if(NULL == menu_ih->handle)
				{
					IupMap(menu_ih);
				}
			}
			
			// We use the same trick as we do with NSTextField in IupText.
			// Save the menu_ih so we can access it in the callback
			iupAttribSet(ih, "_COCOA_CONTEXT_MENU_IH", (const char*)menu_ih);
			
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		{
			
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			iupCocoaCommonBaseSetContextMenuForWidget(ih, table_view, menu_ih);
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			iupCocoaCommonBaseSetContextMenuForWidget(ih, table_view, menu_ih);

			
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			NSLog(@"WARNING: CONTEXTMENU not available for EDITBOX");
			break;
		}
		default:
		{
			break;
		}
	}
	
	return 1;
}


/*
// I might be able to avoid using this.
static void cocoaListLayoutUpdateMethod(Ihandle *ih)
{
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			NSPopUpButton* popup_button = (NSPopUpButton*)cocoaListGetBaseWidget(ih);

			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			NSComboBox* combo_box = (NSComboBox*)cocoaListGetBaseWidget(ih);

			break;
		}
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		{
			NSTableView* table_view = (NSTableView*)cocoaListGetBaseWidget(ih);
			
			// rectOfRow would tell us the height of a specific row
			// The actual -rectOfRow: is equal to the -rowHeight plus the intercellSpacing.height. The default value is 17.0 for applications linked on 10.5 and higher (the height acceptable for [NSFont systemFontSize]). The default value is 16.0 for 10.4 and lower.
			
			// I don't know which row we're at, but we'll assume they are all the same height.
			// rectOfRow is not helpful if the table is empty, so use intercellSpacing.height+rowHeight
			CGFloat row_height = [table_view rowHeight] + [table_view intercellSpacing].height;
			
			
			int visible_lines = 0; // 5 is the default according to the docs
			if(iupAttribGet(ih, "VISIBLELINES"))
			{
				visible_lines = iupAttribGetInt(ih, "VISIBLELINES");
				

				
				
			}
			else
			{
				visible_lines = (int)[table_view numberOfRows];
			}

			CGFloat view_height = row_height * (CGFloat)visible_lines;

			
			{
				NSRect new_frame = [table_view frame];
				CGFloat old_height = new_frame.size.height;
				CGFloat old_y = new_frame.origin.y;
				
				CGFloat diff_y = view_height - old_height;
				
				
				
				
				//NSRect parent_rect = [parent_view frame];
				

								//			 parent_rect.size.height - ih->y - ih->currentheight,

				
				
				
				new_frame.origin.y = old_y - diff_y;

				new_frame.size.height = view_height;
				new_frame.size.width = ih->naturalwidth;

				
			[table_view setFrame:new_frame];
			}
			
			if([(NSObject*)ih->handle isKindOfClass:[NSScrollView class]])
			{
				NSScrollView* scroll_view = (NSScrollView*)ih->handle;
				NSRect new_frame = [table_view frame];
				
				
				CGFloat old_height = new_frame.size.height;
				CGFloat old_y = new_frame.origin.y;
				
				CGFloat diff_y = view_height - old_height;
				
				
				new_frame.origin.y = old_y - diff_y;
				new_frame.size.height = view_height;
				new_frame.size.width = ih->naturalwidth;


				[scroll_view setFrame:new_frame];
				
			}
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}
	
//	iupdrvBaseLayoutUpdateMethod(ih);

}
*/

static int cocoaListMapMethod(Ihandle* ih)
{

	/*
	Iup’s "list" has 4 different modes.
	
	On Mac, these will need to be 4 completely different widgets.
	
	Dropdown:			NSPopUpButton
	Editbox+Dropdown:	NSComboBox
	Multiple:			NSTableView
	Editbox:			No obvious native widget
	*/

	
	NSView* root_view = nil;
	NSView* main_view = nil;
	
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			//NSPopUpButton* popup_button = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
			NSPopUpButton* popup_button = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, kIupCocoaDefaultWidthNSPopUpButton, kIupCocoaDefaultHeightNSPopUpButton) pullsDown:NO];
			root_view = popup_button;
			main_view = root_view;
			
			// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars.
			objc_setAssociatedObject(popup_button, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
			// I also need to track the memory of the buttion action receiver.
			// I prefer to keep the Ihandle the actual NSView instead of the receiver because it makes the rest of the implementation easier if the handle is always an NSView (or very small set of things, e.g. NSWindow, NSView, CALayer).
			// So with only one pointer to deal with, this means we need our Toggle to hold a reference to the receiver object.
			// This is generally not good Cocoa as Toggles don't retain their receivers, but this seems like the best option.
			// Be careful of retain cycles.
			IupCocoaListPopupButtonReceiver* list_receiver = [[IupCocoaListPopupButtonReceiver alloc] init];
			[popup_button setTarget:list_receiver];
			[popup_button setAction:@selector(onSelectionChanged:)];
			// I *think* is we use RETAIN, the object will be released automatically when the Toggle is freed.
			// However, the fact that this is tricky and I had to look up the rules (not to mention worrying about retain cycles)
			// makes me think I should just explicitly manage the memory so everybody is aware of what's going on.
			objc_setAssociatedObject(popup_button, IUP_COCOA_LIST_POPUPBUTTON_RECEIVER_OBJ_KEY, (id)list_receiver, OBJC_ASSOCIATION_ASSIGN);
			

			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			// NSComboBox height is very specific. This number (30) plus the stuff going on in iupdrvListAddBorders affects the final height.
			NSComboBox* combo_box = [[IupCocoaComboBox alloc] initWithFrame:NSMakeRect(0, 0, kIupCocoaDefaultWidthNSComboBox, kIupCocoaDefaultHeightNSComboBox)];
//			NSComboBox* combo_box = [[NSComboBox alloc] initWithFrame:NSZeroRect];
			// WEIRD: I am getting a smaller font size (12 instead of 13) when creating programmatically instead of Interface Builder.
			// Explicitly setting to 13.0 here fixes that. 0.0 is slightly off and causes the vertical alignment to be too low and if you select-drag, the text will scroll by 1 pixel.
			[combo_box setFont:[NSFont systemFontOfSize:13.0]];

			
			
			root_view = combo_box;
			main_view = root_view;

			// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars.
			objc_setAssociatedObject(combo_box, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
			// I also need to track the memory of the buttion action receiver.
			// I prefer to keep the Ihandle the actual NSView instead of the receiver because it makes the rest of the implementation easier if the handle is always an NSView (or very small set of things, e.g. NSWindow, NSView, CALayer).
			// So with only one pointer to deal with, this means we need our Toggle to hold a reference to the receiver object.
			// This is generally not good Cocoa as Toggles don't retain their receivers, but this seems like the best option.
			// Be careful of retain cycles.
			IupCocoaListComboBoxReceiver* list_receiver = [[IupCocoaListComboBoxReceiver alloc] init];
			[combo_box setDelegate:list_receiver];
			
			// I *think* is we use RETAIN, the object will be released automatically when the Toggle is freed.
			// However, the fact that this is tricky and I had to look up the rules (not to mention worrying about retain cycles)
			// makes me think I should just explicitly manage the memory so everybody is aware of what's going on.
			objc_setAssociatedObject(combo_box, IUP_COCOA_LIST_COMBOBOX_RECEIVER_OBJ_KEY, (id)list_receiver, OBJC_ASSOCIATION_ASSIGN);
			
			break;
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		{
//			NSTableView* table_view = [[NSTableView alloc] initWithFrame:NSMakeRect(10, 10, 100, 100)];
			NSTableView* table_view = [[NSTableView alloc] initWithFrame:NSZeroRect];
			NSTableColumn* first_column = [[NSTableColumn alloc] initWithIdentifier:@"IupList"];
			[table_view addTableColumn:first_column];
			
			[table_view setHeaderView:nil];
			
			// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars.
			objc_setAssociatedObject(table_view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
			// I also need to track the memory of the buttion action receiver.
			// I prefer to keep the Ihandle the actual NSView instead of the receiver because it makes the rest of the implementation easier if the handle is always an NSView (or very small set of things, e.g. NSWindow, NSView, CALayer).
			// So with only one pointer to deal with, this means we need our Toggle to hold a reference to the receiver object.
			// This is generally not good Cocoa as Toggles don't retain their receivers, but this seems like the best option.
			// Be careful of retain cycles.
			IupCocoaListTableViewReceiver* list_receiver = [[IupCocoaListTableViewReceiver alloc] init];
			[table_view setDataSource:list_receiver];
			[table_view setDelegate:list_receiver];
			
			[table_view setAllowsMultipleSelection:YES];
			
			// I *think* is we use RETAIN, the object will be released automatically when the Toggle is freed.
			// However, the fact that this is tricky and I had to look up the rules (not to mention worrying about retain cycles)
			// makes me think I should just explicitly manage the memory so everybody is aware of what's going on.
			objc_setAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY, (id)list_receiver, OBJC_ASSOCIATION_ASSIGN);
			
			
//			NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSMakeRect(10, 10, 100, 100)];
			NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSZeroRect];
			[scroll_view setDocumentView:table_view];
			[table_view release];
			root_view = scroll_view;
			main_view = table_view;
			[scroll_view setHasVerticalScroller:YES];

			break;
		}
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
//			NSTableView* table_view = [[NSTableView alloc] initWithFrame:NSMakeRect(10, 10, 100, 100)];
			NSTableView* table_view = [[NSTableView alloc] initWithFrame:NSZeroRect];
			NSTableColumn* first_column = [[NSTableColumn alloc] initWithIdentifier:@"IupList"];
			[table_view addTableColumn:first_column];
			
			[table_view setHeaderView:nil];
			
			// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars.
			objc_setAssociatedObject(table_view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
			// I also need to track the memory of the buttion action receiver.
			// I prefer to keep the Ihandle the actual NSView instead of the receiver because it makes the rest of the implementation easier if the handle is always an NSView (or very small set of things, e.g. NSWindow, NSView, CALayer).
			// So with only one pointer to deal with, this means we need our Toggle to hold a reference to the receiver object.
			// This is generally not good Cocoa as Toggles don't retain their receivers, but this seems like the best option.
			// Be careful of retain cycles.
			IupCocoaListTableViewReceiver* list_receiver = [[IupCocoaListTableViewReceiver alloc] init];
			[table_view setDataSource:list_receiver];
			[table_view setDelegate:list_receiver];
			
			[table_view setAllowsMultipleSelection:NO];

			
			// I *think* is we use RETAIN, the object will be released automatically when the Toggle is freed.
			// However, the fact that this is tricky and I had to look up the rules (not to mention worrying about retain cycles)
			// makes me think I should just explicitly manage the memory so everybody is aware of what's going on.
			objc_setAssociatedObject(table_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY, (id)list_receiver, OBJC_ASSOCIATION_ASSIGN);
			
			
//			NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSMakeRect(10, 10, 100, 100)];
			NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSZeroRect];
			[scroll_view setDocumentView:table_view];
			[table_view release];
			root_view = scroll_view;
			main_view = table_view;
			[scroll_view setHasVerticalScroller:YES];

			
			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			NSLog(@"IupList Editbox subtype not available");
			break;
		}
		default:
		{
			break;
		}
	}
	
	
	
	
	/* Enable internal drag and drop support */
	if(ih->data->show_dragdrop && !ih->data->is_dropdown && !ih->data->is_multiple)
	{

	}
	
	if (iupAttribGetBoolean(ih, "SORT"))
	{
//		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), IUPGTK_LIST_TEXT, GTK_SORT_ASCENDING);
	}
	/* add to the parent, all GTK controls must call this. */
//	iupgtkAddToParent(ih);
	
	
	ih->handle = root_view;
	iupCocoaSetAssociatedViews(ih, main_view, root_view);


	// All Cocoa views shoud call this to add the new view to the parent view.
	iupCocoaAddToParent(ih);

	

	/* configure for DRAG&DROP */
	if (IupGetCallback(ih, "DROPFILES_CB"))
	{
//		iupAttribSet(ih, "DROPFILESTARGET", "YES");
	}
	
//	IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)cocoaListConvertXYToPos);
	
	iupListSetInitialItems(ih);
	
	/* update a mnemonic in a label if necessary */
//	iupgtkUpdateMnemonic(ih);
	
	return IUP_NOERROR;
}


static void cocoaListUnMapMethod(Ihandle* ih)
{
	// Some views may have an NSScrollView as the root and the real "base" widget is underneath.
	// So by convention:
	//	- Use cocoaListGetBaseWidget() to get the real "base" widget.
	//	- Only the root_view should be released (we will not hold an extra retain on the base widget)
	//	- Associated values are tied to the "base" widget
	
	NSView* root_view = ih->handle;
	NSView* base_view = cocoaListGetBaseWidget(ih);
	id list_receiver = nil;

	// Destroy the context menu ih it exists
	{
		Ihandle* context_menu_ih = (Ihandle*)iupCocoaCommonBaseGetContextMenuAttrib(ih);
		if(NULL != context_menu_ih)
		{
			IupDestroy(context_menu_ih);
		}
		iupCocoaCommonBaseSetContextMenuAttrib(ih, NULL);
	}
	
	
	IupCocoaListSubType sub_type = cocoaListGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOALISTSUBTYPE_DROPDOWN:
		{
			list_receiver = objc_getAssociatedObject(base_view, IUP_COCOA_LIST_POPUPBUTTON_RECEIVER_OBJ_KEY);
			objc_setAssociatedObject(base_view, IUP_COCOA_LIST_POPUPBUTTON_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOXDROPDOWN:
		{
			list_receiver = objc_getAssociatedObject(base_view, IUP_COCOA_LIST_COMBOBOX_RECEIVER_OBJ_KEY);
			objc_setAssociatedObject(base_view, IUP_COCOA_LIST_COMBOBOX_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

			break;
		}
		case IUPCOCOALISTSUBTYPE_MULTIPLELIST:
		{
			list_receiver = objc_getAssociatedObject(base_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
			objc_setAssociatedObject(base_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

			break;
		}
		case IUPCOCOALISTSUBTYPE_SINGLELIST:
		{
			list_receiver = objc_getAssociatedObject(base_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY);
			objc_setAssociatedObject(base_view, IUP_COCOA_LIST_TABLEVIEW_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

			break;
		}
		case IUPCOCOALISTSUBTYPE_EDITBOX:
		{
			break;
		}
		default:
		{
			break;
		}
	}
	
	
	
	

	[list_receiver release];

	iupCocoaRemoveFromParent(ih);
	iupCocoaSetAssociatedViews(ih, nil, nil);
	// Only release the root_view; don't release base_view
	[root_view release];
	ih->handle = NULL;
	
}


void iupdrvListInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
	ic->Map = cocoaListMapMethod;
	ic->UnMap = cocoaListUnMapMethod;
//	ic->LayoutUpdate = cocoaListLayoutUpdateMethod;

#if 0

  /* Driver Dependent Attribute functions */

  /* Overwrite Common */
	
	iupClassRegisterAttribute(ic, "FONT", NULL, winListSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */
	
	/* Visual */
	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_NOT_MAPPED);
	
	/* Special */
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_NOT_MAPPED);
	

#endif
  /* IupList only */

	
  iupClassRegisterAttributeId(ic, "IDVALUE", cocoaListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", cocoaListGetValueAttrib, cocoaListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, cocoaListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
#if 0

	iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_DEFAULT);
	/*OLD*/iupClassRegisterAttribute(ic, "VISIBLE_ITEMS", NULL, NULL, IUPAF_SAMEASSYSTEM, "5", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, winListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
	
	
	iupClassRegisterAttribute(ic, "TOPITEM", NULL, cocoaListSetTopItemAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, cocoaListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", cocoaListGetSelectedTextAttrib, cocoaListSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
#endif
	
  iupClassRegisterAttribute(ic, "SELECTION", cocoaListGetSelectionAttrib, cocoaListSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
#if 0
  iupClassRegisterAttribute(ic, "SELECTIONPOS", cocoaListGetSelectionPosAttrib, cocoaListSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", cocoaListGetCaretAttrib, cocoaListSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", cocoaListGetCaretPosAttrib, cocoaListSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
#endif
	
  iupClassRegisterAttribute(ic, "INSERT", NULL, cocoaListSetInsertAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, cocoaListSetAppendAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
#if 0
  iupClassRegisterAttribute(ic, "READONLY", cocoaListGetReadOnlyAttrib, cocoaListSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, cocoaListSetNCAttrib, NULL, NULL, IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, cocoaListSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, cocoaListSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, cocoaListSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "IMAGE", NULL, cocoaListSetImageAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CUEBANNER", NULL, winListSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FILTER", NULL, winListSetFilterAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
  /* Not Supported */
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
#endif

	/* New API for view specific contextual menus (Mac only) */
	// FIXME: NSTableView is going to require some work.
	iupClassRegisterAttribute(ic, "CONTEXTMENU", iupCocoaCommonBaseGetContextMenuAttrib, cocoaListSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

}
