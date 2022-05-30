/** \file
* \brief Tabs Control
*
* See Copyright Notice in "iup.h"
*/

/*
Uses NSTabViewController which requires 10.10.
*/

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_image.h"
#include "iup_tabs.h"

#include "iup_drvfont.h"
#include "iupcocoa_drv.h"

// Need to subclass to get at delegate callbacks for NSTabView.
@interface IupTabViewController : NSTabViewController
@property(nonatomic, assign) NSUInteger previousSelectedIndex;
@end

@implementation IupTabViewController

- (Ihandle*) ihandle
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
	return ih;
}

- (void) tabView:(NSTabView*)tab_view didSelectTabViewItem:(nullable NSTabViewItem*)tab_view_item
{
	[super tabView:tab_view didSelectTabViewItem:tab_view_item];
	Ihandle* ih = [self ihandle];
	
	
	IFnnn cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
	// Watch out: We must not be didSelectTabViewItem or we get the current instead of previous
	// willSelectTabViewItem did not work any better, even if I queried before calling super
//	int prev_pos = iupdrvTabsGetCurrentTab(ih);
	NSUInteger prev_pos = [self previousSelectedIndex];
	
	NSArray<NSTabViewItem*>* array_of_items = [self tabViewItems];
	int pos = (int)[array_of_items indexOfObject:tab_view_item];
	
	Ihandle* child = IupGetChild(ih, pos);
	Ihandle* prev_child = IupGetChild(ih, (int)prev_pos);
	

/*
	if (iupAttribGet(ih, "_IUPCOCOA_IGNORE_SWITCHPAGE"))
		return;
 
	NSView* tab_content_view = (NSView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
	NSView* prev_tab_content_view = (NSView*)iupAttribGet(prev_child, "_IUPTAB_CONTAINER");
 
	if (tab_content_view) gtk_widget_show(tab_content_view);   // show new page, if any
	if (prev_tab_container) gtk_widget_hide(prev_tab_container);  // hide previous page, if any
*/

	if(iupAttribGet(ih, "_IUPCOCOA_IGNORE_CHANGE"))
	{
		[self setPreviousSelectedIndex:(NSUInteger)pos];
		return;
	}
	
	if(cb)
	{
		cb(ih, child, prev_child);
	}
	else
	{
		IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
		if(cb2)
		{
			cb2(ih, pos, (int)prev_pos);
		}
	}

	[self setPreviousSelectedIndex:(NSUInteger)pos];
}


@end

static IupTabViewController* cocoaGetTabViewController(Ihandle* ih)
{
	if(ih && ih->handle)
	{
		IupTabViewController* tab_view_controller = (IupTabViewController*)ih->handle;
		NSCAssert([tab_view_controller isKindOfClass:[IupTabViewController class]], @"Expected IupTabViewController");
		return tab_view_controller;
	}
	else
	{
		NSCAssert(1, @"Expected ih->handle");
	}
	return nil;
}

// IUP seems to use this value as a boolean, not a width or height.
// Additionally, Iup's compute natural size is completely useless for us and this API conveys no useful information for us.
// So this is never called.
int iupdrvTabsExtraDecor(Ihandle* ih)
{
	(void)ih;
	return 1; // FIXME: Total guess
}

int iupdrvTabsExtraMargin(void)
{
  return 0;
}

int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
	(void)ih;
	return 1;
}

void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
	IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
	NSArray<NSTabViewItem*>* array_of_items = [tab_view_controller tabViewItems];
	if(pos >= [array_of_items count])
	{
		return;
	}
	
	// Doc: "It is not called when the current tab is programmatically changed or removed"
	iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
	[tab_view_controller setSelectedTabViewItemIndex:pos];
	iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);
}

int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
	IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
	return (int)[tab_view_controller selectedTabViewItemIndex];
}

// Hidden tabs are not supported
int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
	return 1;
}

static int cocoaTabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
	NSTabViewControllerTabStyle new_style = NSTabViewControllerTabStyleSegmentedControlOnTop;
	
	if(iupStrEqualNoCase(value, "BOTTOM"))
	{
		ih->data->type = ITABS_BOTTOM;
		new_style = NSTabViewControllerTabStyleSegmentedControlOnBottom;
	}
	else if(iupStrEqualNoCase(value, "LEFT"))
	{
		
	}
	else if(iupStrEqualNoCase(value, "RIGHT"))
	{
	}
	else if(iupStrEqualNoCase(value, "TOP"))
	{
		ih->data->type = ITABS_TOP;
		new_style = NSTabViewControllerTabStyleSegmentedControlOnTop;
	}
	else /* "TOP" */
	{
		ih->data->type = ITABS_TOP;
		new_style = NSTabViewControllerTabStyleSegmentedControlOnTop;
	}
	
	
	if(ih->handle)
	{
		IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
		[tab_view_controller setTabStyle:new_style];
		
		// FIXME:
		// I'm having problems where BOTTOM gets clipped.
		// Refresh isn't helping.
		//IupRefresh(ih);
	}
	
	return 0;
}

static int cocoaTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
	// Setting this to make the default getter iupTabsGetTitleAttrib() work
	Ihandle* child = IupGetChild(ih, pos);
	if(child)
	{
		iupAttribSetStr(child, "TABTITLE", value);
	}
	IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
	
	NSArray<NSTabViewItem*>* array_of_items = [tab_view_controller tabViewItems];
	if(pos >= [array_of_items count])
	{
		return 0;
	}
	
	NSTabViewItem* tab_view_item = [array_of_items objectAtIndex:pos];
	
	NSString* tab_title = nil;
	if(value)
	{
		char* stripped_str = iupStrProcessMnemonic(value, NULL, 0);   /* remove & */
		tab_title = [NSString stringWithUTF8String:stripped_str];
	}
	else
	{
		tab_title = @"";
	}
	
	[tab_view_item setLabel:tab_title];
	
	


	return 0;
}

static int cocoaTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
	Ihandle* child = IupGetChild(ih, pos);
	if(child)
	{
		iupAttribSetStr(child, "TABIMAGE", value);
	}
	
	IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
	NSArray* array_of_items = [tab_view_controller tabViewItems];
	NSTabViewItem* tab_view_item = [array_of_items objectAtIndex:pos];
	
	NSImage* bitmap_image = nil;
	if(value)
	{
		bitmap_image = iupImageGetImage(value, ih, 0, NULL);
	}
	else
	{
		
	}
	[tab_view_item setImage:bitmap_image];
	
	return 1;
}

/* This doesn't seem to work even though there is a font property.
Even though the property is not marked read-only, the comments describe it as a "getter"
My attempts to set this property seem to be a no-op
*/
/*
static int cocoaTabsSetStandardFontAttrib(Ihandle* ih, const char* value)
{
	if(!iupdrvSetFontAttrib(ih, value))
	{
		return 0;
	}
	
	if(ih->handle)
	{
		IupCocoaFont* iup_font = iupCocoaGetFont(ih);

		IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
		NSTabView* tab_view = [tab_view_controller tabView];

		[tab_view setFont:[iup_font nativeFont]];
	}
	
	return 1;
}
*/

// I think this should work, but I don't see any results. Apple bug?
#define IUPCOCOA_ENABLE_TABTIP 0
#if IUPCOCOA_ENABLE_TABTIP
static int cocoaTabsSetTabTipAttrib(Ihandle* ih, int pos, const char* value)
{
		// Setting this to make the default getter iupTabsGetTitleAttrib() work
	Ihandle* child = IupGetChild(ih, pos);
	if(child)
	{
		iupAttribSetStr(child, "TABTIP", value);
	}
	IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
	
	NSArray<NSTabViewItem*>* array_of_items = [tab_view_controller tabViewItems];
	if(pos >= [array_of_items count])
	{
		return 0;
	}
	
	NSTabViewItem* tab_view_item = [array_of_items objectAtIndex:pos];
	
	NSString* tab_tip = nil;
	if(value)
	{
		tab_tip = [NSString stringWithUTF8String:value];
	}
	else
	{
		tab_tip = nil;
	}
	
	[tab_view_item setToolTip:tab_tip];

	
	return 0;
}

static char* cocoaTabsGetTabTipAttrib(Ihandle* ih, int pos)
{
	Ihandle* child = IupGetChild(ih, pos);
	if (child)
	{
		return iupAttribGet(child, "TABTIP");
	}
	else
	{
		return NULL;
	}
}
#endif


static int cocoaTabsSetContextMenuAttrib(Ihandle* ih, const char* value)
{
	IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
	NSView* tab_view = [tab_view_controller tabView];
	
	Ihandle* menu_ih = (Ihandle*)value;
	id ih_widget_to_attach_menu_to = tab_view;
	iupCocoaCommonBaseSetContextMenuForWidget(ih, ih_widget_to_attach_menu_to, menu_ih);
	
	return 1;
}



static void cocoaTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  /* make sure it has at least one name */
	if (!iupAttribGetHandleName(child))
	{
		iupAttribSetHandleName(child);
	}
	
	if(ih->handle)
	{
		IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
		
		int pos = IupGetChildPos(ih, child);
		
		// It appears I am required to setup a NSViewController for each tab.
		// Since I do not have a nib, I am also required to alloc a NSView for it.
		NSViewController* view_controller = [[[NSViewController alloc] initWithNibName:nil bundle:nil] autorelease];
#if 1
		NSView* content_view = [[[NSView alloc] initWithFrame:NSZeroRect] autorelease];
#else
		// For Debugging: Paints the content view red to help debug layout regions.
		NSView* content_view = [[[NSBox alloc] initWithFrame:NSZeroRect] autorelease];
		[content_view setBoxType:NSBoxCustom];
		[content_view setFillColor:[NSColor redColor]];
		[content_view setTitlePosition:NSNoTitle];
#endif

		[view_controller setView:content_view];
		NSTabViewItem* tab_item = [NSTabViewItem tabViewItemWithViewController:view_controller];

		// adding the tab view on the active tab will trigger a change callback that I don't think we are supposed to fire:
		// Doc: "It is not called when the current tab is programmatically changed or removed"
		iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
		[tab_view_controller insertTabViewItem:tab_item atIndex:pos];
		//	[tab_view_controller addTabViewItem:tab_item];
		//	NSView* content_view = [tab_item view];
		iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);

		// IMPORTANT: We are putting the content_view in here and not the ViewController or TabViewItem
		// This is because the rest of IUP "just works" when there is a regular view for the map key.
		// And the system will correctly add, layout, and remove this view.
		iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)content_view);
		
		
		// Apply attributes that have been set before map
		char* tab_attribute = NULL;
		tab_attribute = iupAttribGet(child, "TABTITLE");
		cocoaTabsSetTabTitleAttrib(ih, pos, tab_attribute);

		tab_attribute = iupAttribGet(child, "TABIMAGE");
		cocoaTabsSetTabImageAttrib(ih, pos, tab_attribute);

#if IUPCOCOA_ENABLE_TABTIP
		tab_attribute = iupAttribGet(child, "TABTIP");
		cocoaTabsSetTabTipAttrib(ih, pos, tab_attribute);
#endif

		// Need to force a re-layout to fit the new tab
		//IupRefresh(ih);

	}
}

static void cocoaTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
	// NSLog(@"cocoaTabsChildRemovedMethod: %p, %p, %d", ih, child, pos);
	if(ih->handle)
	{
		// Strange: This function gets called back multiple times under some circumstances.
		// If this is added as one of the tabs:
		//   vboxB = IupVbox(IupLabel("Label BBB"), IupButton("Button BBB", "cbChildButton"), NULL);
		// this triggers multiple callbacks.
		// But if this is instead added as the tab,
		//   vboxB = IupFrame(IupVbox(IupLabel("Label BBB"), IupButton("Button BBB", "cbChildButton"), NULL));
		// I only get one callback.
		// The multiple callbacks screw me up because I end up deleting other tabs.
		// The other implementations have this check for "_IUPTAB_CONTAINER" which seems to avoid the multiple deletion issue.
		// I don't really understand why things work this way, but it seems to solve the problem.
		NSView* content_view = (NSView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
		if(content_view)
		{
		
			IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);
			
			// I think this will make Iup switch the current tab to something else so when we remove the tab, both the native & IUP will agree which tab number they are on
	//		iupTabsCheckCurrentTab(ih, pos, 1);
			iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
			
			NSArray* array_of_items = [tab_view_controller tabViewItems];
			NSTabViewItem* tab_view_item = [array_of_items objectAtIndex:pos];
	
			// removing the tab view on the active tab will trigger a change callback that I don't think we are supposed to fire:
			// Doc: "It is not called when the current tab is programmatically changed or removed"
			iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", "1");
			[tab_view_controller removeTabViewItem:tab_view_item];
			iupAttribSet(ih, "_IUPCOCOA_IGNORE_CHANGE", NULL);

			[tab_view_controller setPreviousSelectedIndex:[tab_view_controller selectedTabViewItemIndex]];

			// Need to force a re-layout to refit the tabs
			//IupRefresh(ih);
		}
	}
}

static int cocoaTabsMapMethod(Ihandle* ih)
{
//	NSTabView* tab_control = [[NSTabView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
	IupTabViewController* tab_view_controller = [[IupTabViewController alloc] init];
	// We don't want to see "NSViewController" in the tab by default
	[tab_view_controller setCanPropagateSelectedChildViewControllerTitle:NO];

	// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars.
	objc_setAssociatedObject(tab_view_controller, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);


	

//	ih->handle = tab_control;
	ih->handle = tab_view_controller;
	
	// This is one of those cases where this may not make any sense and nil might be an acceptable value.
//	iupCocoaSetAssociatedViews(ih, nil, nil);
	iupCocoaSetAssociatedViews(ih, [tab_view_controller view], [tab_view_controller view]);

	
	// sanity checks
/*
	ih->data->is_multiline = 0;
	ih->data->show_close = 0;
	ih->data->vert_padding = 0;
	ih->data->horiz_padding = 0;
*/

	// Need to apply the style if the user has set it already
	cocoaTabsSetTabTypeAttrib(ih, iupTabsGetTabTypeAttrib(ih));
	
	// All Cocoa views shoud call this to add the new view to the parent view.
	iupCocoaAddToParent(ih);
	

//	cocoaTabsSetStandardFontAttrib(ih, iupGetFontValue(ih));

	
	/* Create pages and tabs */
	if(ih->firstchild)
	{
		Ihandle* child;
		Ihandle* current_child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");
		
		for(child = ih->firstchild; child; child = child->brother)
		{
			cocoaTabsChildAddedMethod(ih, child);
		}
		
		if(current_child)
		{
			IupSetAttribute(ih, "VALUE_HANDLE", (char*)current_child);
			
			/* current value is now given by the native system */
			iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", NULL);
		}
	}
	
	
	
	return IUP_NOERROR;
}

static void cocoaTabsUnMapMethod(Ihandle* ih)
{
	id tab_control = ih->handle;

	// Destroy the context menu ih it exists
	{
		Ihandle* context_menu_ih = (Ihandle*)iupCocoaCommonBaseGetContextMenuAttrib(ih);
		if(NULL != context_menu_ih)
		{
			IupDestroy(context_menu_ih);
		}
		cocoaTabsSetContextMenuAttrib(ih, NULL);
	}

	iupCocoaRemoveFromParent(ih);
	iupCocoaSetAssociatedViews(ih, nil, nil);
	[tab_control release];
	ih->handle = NULL;
	
}





static int cocoaTabsComputeFullTabBarWidth(Ihandle* ih)
{
	int running_width = 0;
	int pos;
	char *tabtitle, *tabimage;
	Ihandle* child;

	// For the standard font, our result comes out at 25,16, which overshoots too much for this.
	/*
		int char_width;
		int char_height;
		iupdrvFontGetCharSize(ih, &char_width, &char_height);
	 NSLog(@"cocoaTabsGetCombinedWidth char_width:%d, char_height:%d", char_width, char_height);
	 */
	for(pos = 0, child = ih->firstchild; child; child = child->brother, pos++)
	{
		tabtitle = iupAttribGetId(ih, "TABTITLE", pos);
		if (!tabtitle) tabtitle = iupAttribGet(child, "TABTITLE");
		tabimage = iupAttribGetId(ih, "TABIMAGE", pos);
		if (!tabimage) tabimage = iupAttribGet(child, "TABIMAGE");

		

		bool has_image = true;
		
		int current_tab_width = 0;
		if(tabtitle)
		{
			char* stripped_str = iupStrProcessMnemonic(tabtitle, NULL, 0);   /* remove & */

			// Apple doesn't let us change the font, so we know exactly
			/*
			NSString* ns_string = [NSString stringWithUTF8String:stripped_str];
			NSFont* tab_font = [NSFont messageFontOfSize:0];
			NSSize string_size = [ns_string sizeWithFont:myFont
                           constrainedToSize:maximumSize
                               lineBreakMode:self.myLabel.lineBreakMode];
			*/
			

			int current_text_width = iupdrvFontGetStringWidth(ih, stripped_str);
			current_tab_width += current_text_width;
//			NSLog(@"%s current_text_width:%d, current_tab_width:%d", tabtitle, current_text_width, current_tab_width);
			
			
		}
		
		if(tabimage)
		{
			void* img = iupImageGetImage(tabimage, ih, 0, NULL);
			if(img)
			{
				has_image = true;
				int image_w = 16;
				int image_h = 16;
				
				// Getting the real image dimensions won't work. Apple seems to be resizing the image to fit in the tab bar.
				// It seems to be about 16x16
				iupdrvImageGetInfo(img, &image_w, &image_h, NULL);
				current_tab_width += image_w;

//				NSLog(@"%s, image_w:%d, current_tab_width:%d", tabimage, image_w, current_tab_width);
				// There is some padding to account for between the image and text
//				current_tab_width += char_width;
				
			}
		}

		if(has_image && tabtitle)
		{
			// There is about 1 character space bewtween the image and text.
			// I measured about 10 pixels. Our character width over-estimates too much.
			// Update: I was coming in over, so I dropped by 1
			current_tab_width += 9;
		
			// There is also about a 7-8 pixel lead-in, and a 10 pixel trail around the whole thing
			// Update: I was coming in over, so I dropped by 1
			current_tab_width += 7 + 9;
		}
		else if(has_image)
		{
			// There is an about 10 pixel lead-in and 10 pixel trail.
			// Update: I was coming in over, so I dropped by 1
			current_tab_width += 9 + 9;

		}
		else if(tabtitle)
		{
			// There is an about 14 pixel lead-in and 14 pixel trail.
			// Update: I was coming in over, so I dropped by 1
			current_tab_width += 13 + 13;
		}
		else
		{
			// I'm seeing about 18 pixels in the empty NULL title
			current_tab_width += 9 + 9;
		}
		
	
		running_width += current_tab_width;
//		NSLog(@"bottom-loop running_width:%d, current_tab_width:%d", running_width, current_tab_width);

  }

	// With my current numbers, I'm coming in about perfect, but the window edge is exactly on the tab bar end.
	// So let's add a pixel or two to give it a little breathing room.
	
	running_width += 2;

  return running_width;
}

static void cocoaTabsComputeNaturalSize(Ihandle* ih, int *w, int *h, int *children_expand)
{
	Ihandle* child;
	int children_naturalwidth, children_naturalheight;
//  int decorwidth, decorheight;
	
	/* calculate total children natural size (even for hidden children) */
	children_naturalwidth = 0;
	children_naturalheight = 0;
	
	for(child = ih->firstchild; child; child = child->brother)
	{
		/* update child natural size first */
		iupBaseComputeNaturalSize(child);
		
		*children_expand |= child->expand;
		children_naturalwidth = iupMAX(children_naturalwidth, child->naturalwidth);
		children_naturalheight = iupMAX(children_naturalheight, child->naturalheight);
	}

	// IUP's common core implementation for iTabsGetDecorSize is completely useless for us.
	// It focuses on the wrong things for us, has the wrong assumptions,
	// and doesn't seem to understand that the tab bar itself needs a minimum width.
	// (Maybe it assumes the tabs wrap, which is not the case for Cocoa.)
	// So this
//  iTabsGetDecorSize(ih, &decorwidth, &decorheight);
//  *w = children_naturalwidth + decorwidth;
//  *h = children_naturalheight + decorheight;
//	NSLog(@"iupClassObjectComputeNaturalSize children_naturalwidth:%d, children_naturalheight:%d", children_naturalwidth, children_naturalheight);

	IupTabViewController* tab_view_controller = cocoaGetTabViewController(ih);

	NSTabView* tab_view = [tab_view_controller tabView];
	
	// It seems that this might give us the correct height. But the width doesn't seem helpful.
	// Apple allows truncating the strings into the tabs with ellipses.
	// So trying to compute the natural size with that value isn't working out.
	NSRect the_rect = [tab_view frame];
	//NSLog(@"cocoaTabsComputeNaturalSize the_rect: %@", NSStringFromRect(the_rect));

/*
	// this is 0,0
	NSSize minimum_size = [tab_view minimumSize];
	NSLog(@"cocoaTabsComputeNaturalSize minimum_size: %@", NSStringFromSize(minimum_size));
	// this is 0,0
	NSSize preferredContentSize = [tab_view_controller preferredContentSize];
	NSLog(@"cocoaTabsComputeNaturalSize preferredContentSize: %@", NSStringFromSize(preferredContentSize));
	// This is 20,20
	NSSize preferredMinimumSize = [tab_view_controller preferredMinimumSize];
	NSLog(@"cocoaTabsComputeNaturalSize v: %@", NSStringFromSize(preferredMinimumSize));
*/

	// The above built-in values does't seem to include the segmented-bar space as part of the size.
	// I don't know an official way to compute the size.
	// So we have to do it the hard way and brute force an estimate.
	int max_tab_bar_width = cocoaTabsComputeFullTabBarWidth(ih);
	//NSLog(@"cocoaTabsComputeNaturalSize max_tab_width: %d", max_tab_bar_width);

	// We also need to account for the tab bar height and padding.
	// The bar height seems to be about 21 pixels
	// There also seems to be about 4 pixels of space above, and 6 pixels below.
	// UPDATE: The frame on tabView seems to get us the height we need. My values seem to be bigger than what frame says.
//	int add_tab_height = 21 + 4 + 6;

	int final_w;
	int final_h;
	
	final_w = iupMAX(children_naturalwidth, max_tab_bar_width);
	
//	final_h = children_naturalwidth + add_tab_height;
	final_h = iupMAX(children_naturalheight, the_rect.size.height);

//	NSLog(@"cocoaTabsComputeNaturalSize final w:%d h:%d", final_w, final_h);

	*w = final_w;
	*h = final_h;
}


void iupdrvTabsInitClass(Iclass* ic)
{

  /* Driver Dependent Class functions */
	ic->Map = cocoaTabsMapMethod;
	ic->UnMap = cocoaTabsUnMapMethod;
	ic->ChildAdded     = cocoaTabsChildAddedMethod;
	ic->ChildRemoved   = cocoaTabsChildRemovedMethod;
	//	ic->LayoutUpdate = cocoaTabsLayoutUpdateMethod;
	ic->ComputeNaturalSize = cocoaTabsComputeNaturalSize;

  /* IupTabs only */
  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, cocoaTabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, cocoaTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, cocoaTabsSetTabImageAttrib, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);


  /* Common */
	/* This doesn't seem to work even though there is a font property.
	 Even though the property is not marked read-only, the comments describe it as a "getter"
	 My attempts to set this property seem to be a no-op
	 */
//  iupClassRegisterAttribute(ic, "STANDARDFONT", NULL, cocoaTabsSetStandardFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);

#if 0

  /* Driver Dependent Attribute functions */

	// Not supported (there is no close button for tabs)
  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");


  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, gtkTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, gtkTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  /* IupTabs only */
  iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, gtkTabsSetTabOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, gtkTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupTabsGetPaddingAttrib, gtkTabsSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
#endif

  /* NOT supported */
  iupClassRegisterAttribute(ic, "STANDARDFONT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "TABORIENTATION", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "TABVISIBLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);
  iupClassRegisterAttribute(ic, "PADDING", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED);

	// New API:
	//I think this should work, but I am not getting results. Apple bug?
#if IUPCOCOA_ENABLE_TABTIP
  iupClassRegisterAttributeId(ic, "TABTIP", cocoaTabsGetTabTipAttrib, cocoaTabsSetTabTipAttrib, IUPAF_NO_INHERIT);
#endif

	/* New API for view specific contextual menus (Mac only) */
	iupClassRegisterAttribute(ic, "CONTEXTMENU", iupCocoaCommonBaseGetContextMenuAttrib, cocoaTabsSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupCocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);

	
}

