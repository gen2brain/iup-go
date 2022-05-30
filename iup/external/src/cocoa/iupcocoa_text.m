/** \file
 * \brief Text Control
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
#include "iup_image.h"
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_array.h"
#include "iup_text.h"
#include "iup_childtree.h"

#include "iupcocoa_drv.h"
#import "IUPTextSpinnerFilesOwner.h"

// the point of this is we have a unique memory address for an identifier
static const void* IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY = "IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY";
static const void* IUP_COCOA_TEXT_DELEGATE_OBJ_KEY = "IUP_COCOA_TEXT_DELEGATE_OBJ_KEY";


#if __APPLE__
	#define USE_NSSTACKVIEW_TEXTFIELD_CONTAINER 1
#else // Intended for GNUStep because it lacks NSStackView
	// USE_CONTAINERVIEW_TEXTFIELD_CONTAINER was intended to try to build an equivalent to the NSStackView approach using the old autoresizingMask, but I can't get it to work correctly in the complicated IupGrid situtation.
	// The default fallback case is actually better than this container approach.
//	#define USE_CONTAINERVIEW_TEXTFIELD_CONTAINER 1
#endif



typedef enum
{
	IupCocoaTextSubType_UNKNOWN = -1,
	IUPCOCOATEXTSUBTYPE_FIELD,
	IUPCOCOATEXTSUBTYPE_VIEW,
	IUPCOCOATEXTSUBTYPE_STEPPER,
} IupCocoaTextSubType;

/*
 Each IUP list subtype requires a completely different Cocoa native widget.
 This function provides a consistent and centralized way to distinguish which subtype we need.
 */
static IupCocoaTextSubType cocoaTextGetSubType(Ihandle* ih)
{
	if(ih->data->is_multiline)
	{
		return IUPCOCOATEXTSUBTYPE_VIEW;
	}
	else if(iupAttribGetBoolean(ih, "SPIN"))
	{
		return IUPCOCOATEXTSUBTYPE_STEPPER;
	}
	else
	{
		return IUPCOCOATEXTSUBTYPE_FIELD;
	}
	return IupCocoaTextSubType_UNKNOWN;
}

static NSView* cocoaTextGetRootView(Ihandle* ih)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSView* root_container_view = (NSView*)ih->handle;
			NSCAssert([root_container_view isKindOfClass:[NSView class]], @"Expected NSView");
			return root_container_view;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSView* root_container_view = (NSView*)ih->handle;
			NSCAssert([root_container_view isKindOfClass:[NSView class]], @"Expected NSView");
			return root_container_view;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSStackView* root_container_view = (NSView*)ih->handle;
			NSCAssert([root_container_view isKindOfClass:[NSStackView class]], @"Expected NSStackView");
			return root_container_view;
		}
		default:
		{
			break;
		}
	}
	return nil;
}


static NSTextField* cocoaTextGetTextField(Ihandle* ih)
{
#ifdef USE_NSSTACKVIEW_TEXTFIELD_CONTAINER
	NSStackView* root_container_view = (NSStackView*)ih->handle;
	NSTextField* text_field = [[root_container_view views] firstObject];
	NSCAssert([text_field isKindOfClass:[NSTextField class]], @"Expected NSTextField");
	return text_field;
#elif defined(USE_CONTAINERVIEW_TEXTFIELD_CONTAINER)
	NSView* root_container_view = (NSView*)ih->handle;
	NSTextField* text_field = [[root_container_view subviews] firstObject];
	NSCAssert([text_field isKindOfClass:[NSTextField class]], @"Expected NSTextField");
	return text_field;
#else
	NSTextField* root_container_view = (NSTextField*)ih->handle;
	NSCAssert([root_container_view isKindOfClass:[NSTextField class]], @"Expected NSTextField");
	return root_container_view;
#endif

}

static NSTextView* cocoaTextGetTextView(Ihandle* ih)
{
	NSScrollView* root_container_view = (NSScrollView*)cocoaTextGetRootView(ih);
	NSCAssert([root_container_view isKindOfClass:[NSScrollView class]], @"Expected NSScrollView");
	NSTextView* text_view = [root_container_view documentView];
	NSCAssert([text_view isKindOfClass:[NSTextView class]], @"Expected NSTextView");
	return text_view;
}

static NSStepper* cocoaTextGetStepperView(Ihandle* ih)
{
#if 0
	NSStackView* root_container_view = (NSStackView*)ih->handle;
	NSStepper* stepper_view = [[root_container_view views] lastObject];
	NSCAssert([stepper_view isKindOfClass:[NSStepper class]], @"Expected NSStepper");
	return stepper_view;
#else
	IUPTextSpinnerContainer* spinner_container = (IUPTextSpinnerContainer*)objc_getAssociatedObject((id)ih->handle, IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY);
	NSCAssert([spinner_container isKindOfClass:[IUPTextSpinnerContainer class]], @"Expected IUPTextSpinnerContainer");
	return [spinner_container stepperView];
#endif
}

static NSTextField* cocoaTextGetStepperTextField(Ihandle* ih)
{
#if 0
	NSStackView* root_container_view = (NSStackView*)ih->handle;
	NSTextField* text_field = [[root_container_view views] firstObject];
	NSCAssert([text_field isKindOfClass:[NSTextField class]], @"Expected NSTextField");
	return text_field;
#else
	IUPTextSpinnerContainer* spinner_container = (IUPTextSpinnerContainer*)objc_getAssociatedObject((id)ih->handle, IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY);
	NSCAssert([spinner_container isKindOfClass:[IUPTextSpinnerContainer class]], @"Expected IUPTextSpinnerContainer");
	return [spinner_container textField];
#endif
}

static IUPStepperObject* cocoaTextGetStepperObject(Ihandle* ih)
{
	IUPTextSpinnerContainer* spinner_container = (IUPTextSpinnerContainer*)objc_getAssociatedObject((id)ih->handle, IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY);
	NSCAssert([spinner_container isKindOfClass:[IUPTextSpinnerContainer class]], @"Expected IUPTextSpinnerContainer");
	return [spinner_container stepperObject];
}

static IUPStepperObjectController* cocoaTextGetStepperObjectController(Ihandle* ih)
{
	IUPTextSpinnerContainer* spinner_container = (IUPTextSpinnerContainer*)objc_getAssociatedObject((id)ih->handle, IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY);
	NSCAssert([spinner_container isKindOfClass:[IUPTextSpinnerContainer class]], @"Expected IUPTextSpinnerContainer");
	return [spinner_container stepperObjectController];
}



// This is shared between IupCocoaTextField & IupCocoaSecureTextField
static BOOL IupCocoaTextFieldActionCallbackHelper(Ihandle* ih, NSRange change_range, NSString* replacement_string)
{
	if(ih->data->disable_callbacks)
	{
		return YES;
	}
	
	
	IFnis action_cb = (IFnis)IupGetCallback(ih, "ACTION");
	int ret_val;
	
	if(NULL != action_cb)
	{
		
		// FIXME: Converting to UTF8String may break the start/end ranges
		int start_pos = (int)change_range.location;
		// FIXME: I don't know what end_pos is. I thought I needed to subtract 1, but not doing so makes things work
//		int end_pos = (int)(start_pos + change_range.length - 1);
		int end_pos = (int)(start_pos + change_range.length);
		
		if(end_pos < start_pos)
		{
			end_pos = start_pos;
		}
		
		// Note: I'm assuming we're always in UTF8 mode
		
		//int iupEditCallActionCb(Ihandle* ih, IFnis cb, const char* insert_value, int start, int end, void *mask, int nc, int remove_dir, int utf8)
		// FIXME: remove_direction???: 1 backwards, 0 forwards, -1???. I don't know what this means.
		const char* c_str = [replacement_string UTF8String];
		
		// I think iupEditCallActionCb assumes a delete is NULL for the insert_value and "" will break it.
		if(0 == strlen(c_str))
		{
			c_str = NULL;
		}
		
		ret_val = iupEditCallActionCb(ih, action_cb, c_str, start_pos, end_pos, ih->data->mask, ih->data->nc, 1, YES);

		// FIXME: I don't understand the documentation return value rules.
		if(0 == ret_val)
		{
			return YES;
		}
		else if(-1 != ret_val)
		{
			return NO;
		}
		
	}
	return YES;
}


// Helper function for NSTextField and NSSecureTextField overrides of textView:menu:forEvent:atIndex: to inject user menu items into the context menu.
static void cocoaTextFieldOverrideContextMenuHelper(Ihandle* ih, NSTextView* text_view, NSMenu* the_menu)
{
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
}

@interface IupCocoaTextField : NSTextField
@end

@implementation IupCocoaTextField

/*
- (void) controlTextDidBeginEditing:(NSNotification*)notification
{
}

- (void) controlTextDidEndEditing:(NSNotification*)notification
{
}
*/

// I'm in dangerous territory here and could break things like undo
// http://www.cocoabuilder.com/archive/cocoa/309009-equivalent-of-uitextfield-textfield-shouldchangecharactersinrange-replacementstring-for-nstextfield.html
// Also, I'm not sure if this is public or private API. If the latter, we need to remove it.
- (BOOL) textView:(NSTextView*)text_view shouldChangeTextInRange:(NSRange)change_range replacementString:(NSString*)replacement_string
{
//	NSLog(@"textView shouldChangeTextInRange replacement: %@", replacement_string);
	
	BOOL ret_flag;
	if([super textView:text_view shouldChangeTextInRange:change_range replacementString:replacement_string])
	{
		id the_delegate = [self delegate];
		if([the_delegate respondsToSelector:@selector(textView:shouldChangeTextInRange:replacementString:)])
		{
			ret_flag = [the_delegate textView:text_view shouldChangeTextInRange:change_range replacementString:replacement_string];
			if(NO == ret_flag)
			{
				// If something returned NO, I don't think a user callback can do anything.
				return NO;
			}
		}
	}
	else
	{
		// If something returned NO, I don't think a user callback can do anything.
		return NO;
	}
	
	
	NSTextField* text_field = self;
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);
	
	ret_flag = IupCocoaTextFieldActionCallbackHelper(ih, change_range, replacement_string);
	if(YES == ret_flag)
	{
	    [text_view didChangeText];
	}
	return ret_flag;
}

// WARNING: This was the only way I could figure out how to correctly override the contextual menu for NSTextField.
// I tried setting the menu (on demand) of the field editor, but I ended up getting lots of missing items.
// I tried creating my own field editor and setting a custom delegate for this method on it, but it never got invoked. I think Cocoa may ignore my delegate when used as a field editor.
// I found a mention on the internet that vaguely suggested subclassing NSTextField and implementing this method.
// I have not found documentation about this method in NSTextField, only as a delegate protocol for NSTextView.
// This might mean this is private API so this may have to be disabled.
// This seems to work though.
- (NSMenu *)textView:(NSTextView *)text_view menu:(NSMenu *)the_menu forEvent:(NSEvent *)the_event atIndex:(NSUInteger)char_index
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
	cocoaTextFieldOverrideContextMenuHelper(ih, text_view, the_menu);
	return the_menu;
}
@end


@interface IupCocoaSecureTextField : NSSecureTextField
@end

@implementation IupCocoaSecureTextField

/*
 - (void) controlTextDidBeginEditing:(NSNotification*)notification
 {
 }
 
 - (void) controlTextDidEndEditing:(NSNotification*)notification
 {
 }
 */

// I'm in dangerous territory here and could break things like undo
// http://www.cocoabuilder.com/archive/cocoa/309009-equivalent-of-uitextfield-textfield-shouldchangecharactersinrange-replacementstring-for-nstextfield.html
// Also, I'm not sure if this is public or private API. If the latter, we need to remove it.
- (BOOL) textView:(NSTextView*)text_view shouldChangeTextInRange:(NSRange)change_range replacementString:(NSString*)replacement_string
{
	//	NSLog(@"textView shouldChangeTextInRange replacement: %@", replacement_string);
	
	BOOL ret_flag;
	if([super textView:text_view shouldChangeTextInRange:change_range replacementString:replacement_string])
	{
		id the_delegate = [self delegate];
		if([the_delegate respondsToSelector:@selector(textView:shouldChangeTextInRange:replacementString:)])
		{
			ret_flag = [the_delegate textView:text_view shouldChangeTextInRange:change_range replacementString:replacement_string];
			if(NO == ret_flag)
			{
				// If something returned NO, I don't think a user callback can do anything.
				return NO;
			}
		}
	}
	else
	{
		// If something returned NO, I don't think a user callback can do anything.
		return NO;
	}
	
	
	NSTextField* text_field = self;
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);
	
	ret_flag = IupCocoaTextFieldActionCallbackHelper(ih, change_range, replacement_string);
	if(YES == ret_flag)
	{
	    [text_view didChangeText];
	}
	return ret_flag;
}

// WARNING: This was the only way I could figure out how to correctly override the contextual menu for NSTextField.
// I tried setting the menu (on demand) of the field editor, but I ended up getting lots of missing items.
// I tried creating my own field editor and setting a custom delegate for this method on it, but it never got invoked. I think Cocoa may ignore my delegate when used as a field editor.
// I found a mention on the internet that vaguely suggested subclassing NSTextField and implementing this method.
// I have not found documentation about this method in NSTextField, only as a delegate protocol for NSTextView.
// This might mean this is private API so this may have to be disabled.
// This seems to work though.
- (NSMenu *)textView:(NSTextView *)text_view menu:(NSMenu *)the_menu forEvent:(NSEvent *)the_event atIndex:(NSUInteger)char_index
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
	cocoaTextFieldOverrideContextMenuHelper(ih, text_view, the_menu);
	return the_menu;
}

@end

@interface IupCocoaTextFieldDelegate : NSObject <NSControlTextEditingDelegate, NSTextFieldDelegate>
@end

@implementation IupCocoaTextFieldDelegate
// We could have reused textView:shouldChangeTextInRange:replacementString:, but I have multiple concerns we may not be able to keep it.
// So we use this for the VALUECHANGED_CB
- (void) controlTextDidChange:(NSNotification*)the_notification
{
	NSTextField* text_field = [the_notification object];
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY);
//	NSLog(@"controlTextDidChange: stringValue == %@", [text_field stringValue]);

	if(ih->data->disable_callbacks)
	{
		return;
	}
	
	IFn value_changed_callback = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
	if(NULL != value_changed_callback)
	{
		int ret_val = value_changed_callback(ih);
		(void)ret_val;
	}
}

@end

// NOTE: This callback is unfinished. Need to understand SPIN_CB rules better.
// Inherit from IupCocoaTextFieldDelegate to get VALUECHANGED_CB
@interface IupCocoaTextSpinnerDelegate : IupCocoaTextFieldDelegate
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTextSpinnerDelegate

/*
- (BOOL)control:(NSControl*)the_control textShouldEndEditing:(NSText *)field_editor;
{
//	NSLog(@"control: %@", the_control);
//	NSLog(@"textShouldEndEditing: %@", field_editor);
}
*/

- (IBAction) mySpinnerClickAction:(id)the_sender
{
	IFni callback_function;
	Ihandle* ih = [self ihandle];
	
	callback_function = (IFni)IupGetCallback(ih, "SPIN_CB");
	if(callback_function)
	{
		IUPStepperObject* stepper_object = cocoaTextGetStepperObject(ih);
		NSNumber* ns_number = [stepper_object stepperValue];
		
		int current_value = [ns_number intValue];
		
		int ret_val = callback_function(ih, current_value);
		
		if(IUP_IGNORE == ret_val)
		{
			// We can't do anything with this
		}
	}
}


@end


// Inherit from IupCocoaTextFieldDelegate to get VALUECHANGED_CB
@interface IupCocoaTextViewDelegate : IupCocoaTextFieldDelegate <NSTextViewDelegate>
{
	NSUndoManager* undoManager;
}
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTextViewDelegate

- (instancetype) init
{
	self = [super init];
	if(nil != self)
	{
		undoManager = [[NSUndoManager alloc] init];
	}
	return self;
}

- (void) dealloc
{
	[undoManager release];
	[super dealloc];
}

- (NSUndoManager*) undoManagerForTextView:(NSTextView*)text_view
{
	return undoManager;
}

- (BOOL) textView:(NSTextView*)text_view shouldChangeTextInRange:(NSRange)change_range replacementString:(NSString*)replacement_string
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(text_view, IHANDLE_ASSOCIATED_OBJ_KEY);
	BOOL ret_flag = IupCocoaTextFieldActionCallbackHelper(ih, change_range, replacement_string);
	if(YES == ret_flag)
	{
	    [text_view didChangeText];
	}
	return ret_flag;
}



// forward declaration
static bool cocoaTextComputeLineColumnFromRangeForTextView(NSTextView* text_view, NSRange native_selection_range, NSUInteger* out_start_line, NSUInteger* out_start_column, NSUInteger* out_end_line, NSUInteger* out_end_column);

// For CARET_CB
- (void) textViewDidChangeSelection:(NSNotification*)the_notification
{
	IFniii callback_function;
	Ihandle* ih = [self ihandle];
	
	callback_function = (IFniii)IupGetCallback(ih, "CARET_CB");
	if(callback_function)
	{
		Ihandle* ih = [self ihandle];
		NSTextView* text_view = cocoaTextGetTextView(ih);

		NSRange cursor_range = [[[text_view selectedRanges] lastObject] rangeValue];
		if(NSNotFound == cursor_range.location)
		{
			// what do we do?
			NSTextStorage* text_storage = [text_view textStorage];
			cursor_range.location = [text_storage length];
			cursor_range.length = 0;
		}
		
		NSUInteger lin_start=1;
		NSUInteger col_start=1;
		NSUInteger lin_end=1;
		NSUInteger col_end=1;
		bool did_find_range = cocoaTextComputeLineColumnFromRangeForTextView(text_view, cursor_range, &lin_start, &col_start, &lin_end, &col_end);
		if(!did_find_range)
		{
		}
		// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
		int ret_val = callback_function(ih, (int)lin_start, (int)col_start, (int)cursor_range.location);
		// This API doesn't claim to do anything with return values



	}

}

@end


void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{

	
}

void iupdrvTextAddBorders(Ihandle* ih, int *x, int *y)
{
	// TODO: Originally, we wanted to lock down the height for NSTextFields so Iup wouldn't try to grow it.
	// This broke multiline NSTextView's.
	// We needed the Ihandle* to distinguish, but the API did not provide it.
	// We've since changed the API to get this.
	// However, we also employed NSStackView to prevent the NSTextField from increasing its height.
	// This code seems to work for now, but probably should be heavily tested under more conditions.
	
	// if(ih->data->is_multiline)



	// WARNING: This is still a hack. SCROLLBAR_PADDING ws randomly guessed to get by for now.
	// BEWARE: The Cocoa widget may not exist yet when this is called. We will get an ih, but ih->handle is not established. This will make things harder.
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
//			NSTextView* text_view = cocoaTextGetTextView(ih);

	
			//	*y = *y+10;
			//	*x = *x+4;
				// NSTextField is all guess work about how to scale for any font size.
				// Throw away ih->currentheight because it will EXPAND it.
				// But for the standard font, we get fontheight=16 and we want the height to be 22

			
			//	int font_height = 16;
				int font_height = *y;
			
			//	iupdrvFontGetCharSize(ih, NULL, &font_height);
				const CGFloat HEIGHT_PADDING = 6.0;
				const CGFloat SCROLLBAR_PADDING = 8.0;

				*y = font_height + HEIGHT_PADDING + SCROLLBAR_PADDING;
			//	*y = 16;

				const CGFloat WIDTH_PADDING = 12.0;
			
			
				*x = *x - WIDTH_PADDING;

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			// Neither the field nor the cell work. I think I must change the field editor.
//			NSTextField* text_field = cocoaTextGetTextField(ih);


			
			//	*y = *y+10;
			//	*x = *x+4;
				// NSTextField is all guess work about how to scale for any font size.
				// Throw away ih->currentheight because it will EXPAND it.
				// But for the standard font, we get fontheight=16 and we want the height to be 22

			
			//	int font_height = 16;
				int font_height = *y;
			
			//	iupdrvFontGetCharSize(ih, NULL, &font_height);
				const CGFloat HEIGHT_PADDING = 6.0;
			
				*y = font_height + HEIGHT_PADDING;
			//	*y = 16;

				const CGFloat WIDTH_PADDING = 12.0;
			
			
				*x = *x - WIDTH_PADDING;

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
		
		
	
			//	*y = *y+10;
			//	*x = *x+4;
				// NSTextField is all guess work about how to scale for any font size.
				// Throw away ih->currentheight because it will EXPAND it.
				// But for the standard font, we get fontheight=16 and we want the height to be 22

			
			//	int font_height = 16;
				int font_height = *y;
			
			//	iupdrvFontGetCharSize(ih, NULL, &font_height);
				const CGFloat HEIGHT_PADDING = 6.0;
			
				*y = font_height + HEIGHT_PADDING;
			//	*y = 16;

				const CGFloat WIDTH_PADDING = 12.0;
			
			
				*x = *x - WIDTH_PADDING;
	
			break;
		}
		default:
		{
			break;
		}
	}
	
	
	

}

// GOTCHA: Modern characters may be multiple bytes (e.g. emoji characters).
// Because of this, [string length] isn't correct, because it tells us the number of bytes, not characters.
// The correct thing to do is to iterate through the string and count the glyphs.
// But it probably is more expensive than what people think when they call this routine.
// See https://www.objc.io/issues/9-strings/unicode/
// NSString *s = @"The weather on \U0001F30D is \U0001F31E today.";
// The weather on 🌍 is 🌞 today.
static NSUInteger cocoaTextCountGlyphsInString(NSString* text_string)
{
	// Which of these two algorithms is better?
	
	
//	NSLog(@"length: %zu\n%@", [text_string length], text_string);
#if 0
	NSUInteger glyph_count = 0;
	NSUInteger index = 0;
	NSUInteger raw_length = [text_string length];
	while (index < raw_length)
	{
		NSRange the_range = [text_string rangeOfComposedCharacterSequenceAtIndex:index];
		glyph_count++;
		index += the_range.length;
	}
	return glyph_count;
#else
	NSRange full_range = NSMakeRange(0, [text_string length]);
	// Remember __block let's us modify this outside variable inside the block
	// https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/Blocks/Articles/bxVariables.html#//apple_ref/doc/uid/TP40007502-CH6-SW1
	__block int glyph_count = 0;
	[text_string enumerateSubstringsInRange:full_range
		options:NSStringEnumerationByComposedCharacterSequences
		usingBlock:^(NSString* substring, NSRange substring_range,
		NSRange enclosing_range, BOOL* stop)
		{
			glyph_count++;
		}
	 ];
	 return glyph_count;
#endif
	
}

// This formatter exists solely to support the NC feature.
@interface IupFormatter : NSFormatter
@property(nonatomic, assign) Ihandle* ihandle;
@end

// I only use isPartialStringValid for "NC"
@implementation IupFormatter

// required. pass-through behavior
- (NSString*) stringForObjectValue:(id)obj_val
{
    return obj_val;
}


// required. pass-through behavior
- (BOOL)getObjectValue:(id*)out_obj_result forString:(NSString*)the_string errorDescription:(NSString**)the_error
{
	if(the_error)
    {
		*the_error = nil;
	}
    if(out_obj_result)
    {
		*out_obj_result = [the_string retain];
	}
    return YES;
}


- (BOOL)isPartialStringValid:(NSString*)partial_string newEditingString:(NSString**)new_string errorDescription:(NSString**)the_error
{
    if(new_string)
    {
		*new_string = nil;
	}
	if(the_error)
    {
		*the_error = nil;
	}

	// empty string is okay
    if([partial_string length] == 0)
    {
        return YES;
	}
	// Make sure to limit the length if the IUP "NC" feature is in use
	Ihandle* ih = [self ihandle];
	if(ih->data->nc > 0)
	{
		// I think this is supposed to be a glyph count instead of a byte count.
		// So we need to do extra work.
		NSUInteger glyph_count = cocoaTextCountGlyphsInString(partial_string);
		if(glyph_count > ih->data->nc)
		{
			return NO;
		}
	}
	return YES;
}

@end

// This formatter exists to support the FILTER feature and the "NC" feature.
@interface IupNumberFormatter : NSNumberFormatter
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupNumberFormatter

- (BOOL)isPartialStringValid:(NSString*)partial_string newEditingString:(NSString**)new_string errorDescription:(NSString**)the_error
{
    if(new_string)
    {
		*new_string = nil;
	}
	if(the_error)
    {
		*the_error = nil;
	}

	// empty string is okay
    if([partial_string length] == 0)
    {
        return YES;
	}
	// Make sure to limit the length if the IUP "NC" feature is in use
	Ihandle* ih = [self ihandle];
	if(ih->data->nc > 0)
	{
		// I think this is supposed to be a glyph count instead of a byte count.
		// So we need to do extra work.
		NSUInteger glyph_count = cocoaTextCountGlyphsInString(partial_string);
		if(glyph_count > ih->data->nc)
		{
			return NO;
		}
	}

	NSMutableCharacterSet* allowed_character_set = [[NSCharacterSet decimalDigitCharacterSet] mutableCopy];
	[allowed_character_set autorelease];
	// Allow scientific notation, decimal points, and positive and negative. Also allow people to enter commas as both decimals and separators. Space is also separator.
	if(NSNumberFormatterNoStyle == [self numberStyle])
	{
		// Turns out NSNumberFormatterNoStyle is very restrictive.
		// No leading + allowed (- is okay)
		// No grouping separators
		/*
		NSString* grouping_separator = [self groupingSeparator]; // doesb't seem to include space
		[allowed_character_set addCharactersInString:grouping_separator];
		[allowed_character_set addCharactersInString:@"+- "];
		*/
		[allowed_character_set addCharactersInString:@"-"];
	}
	else
	{
		// () is another way to express negative, but doesn't seem to work with our supported modes
		// + only works with e, 1e+2 and not +5. We don't need it.
		[allowed_character_set addCharactersInString:@".eE-, "];
	}
	NSCharacterSet* disallowed_character_set = [allowed_character_set invertedSet];

	
	// TODO: FIXME: I'm being lazy.
	// This will allow invalid strings like 100-111, 23+eee-34,
	// We may also want to specialize this for decimal, integer, scientific modes
    if([partial_string rangeOfCharacterFromSet:disallowed_character_set].location != NSNotFound)
    {
        return NO;
    }

	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			// NSNumberFormatter is more resilant to of fancy numbers (e.g. commas, locales, etc) than NSString integerValue
			// And although the documentation says we only support INTEGERs,
			// because we let people muck with FILTER,
			// we may eventually end up in a more complicated case.
			NSNumberFormatter* conversion_formatter = [[NSNumberFormatter alloc] init];
			[conversion_formatter autorelease];
			[conversion_formatter setNumberStyle:[self numberStyle]];
			NSNumber* ns_number = [conversion_formatter numberFromString:partial_string];

			double current_number = [ns_number doubleValue];
			NSStepper* stepper_view = cocoaTextGetStepperView(ih);
			double max_value = [stepper_view maxValue];
			if(current_number > max_value)
			{
				return NO;
			}
			double min_value = [stepper_view minValue];
			if(current_number < min_value)
			{
				return NO;
			}
			
			
			
			
			break;
		}
		default:
		{
			break;
		}
	}
	
	



    return YES;
}


@end


// Only for NSTextField.
// I do not have a good solution for NSTextView
static int cocoaTextSetNCAttrib(Ihandle* ih, const char* value)
{
	if(!iupStrToInt(value, &ih->data->nc))
	{
		ih->data->nc = 0;
	}
	
	if(ih->handle)
	{
		IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
		switch(sub_type)
		{
			case IUPCOCOATEXTSUBTYPE_VIEW:
			{
				
				break;
			}
			case IUPCOCOATEXTSUBTYPE_FIELD:
			{
				NSTextField* text_field = cocoaTextGetTextField(ih);
				// If we already attached a formatter, it should already have the code to check for NC.
				// We don't want to replace an IupNumberFormatter if it is already there, otherwise we lose the number formatting part.
				if([text_field formatter])
				{
					return 0;
				}
				
				IupFormatter* nc_formatter = [[IupFormatter alloc] init];
				[nc_formatter autorelease];
				[nc_formatter setIhandle:ih];
				[text_field setFormatter:nc_formatter];
				return 0;
				
				break;
			}
			case IUPCOCOATEXTSUBTYPE_STEPPER:
			{
				// Leave the existing the Number formatter alone
				return 0;
				
				break;
			}
			default:
			{
				break;
			}
		}
		
		return 0;
		
	}
	else
	{
		return 1; /* store until not mapped, when mapped will be set again */
	}
}

// Only for NSTextField. LOWERCASE, UPPERCASE not supported.
// Introducing INTEGER, SCIENTIFC, CURRENCY
// WATCH OUT: Be careful with the "NC" feature because the formatters may overwrite each other.
static int cocoaTextSetFilterAttrib(Ihandle* ih, const char* value)
{
  	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
	
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);

			// remove the formatter
			if(NULL == value)
			{
				// If "NC" is being used, we need to add back a formatter for that
				if(ih->data->nc > 0)
				{
					IupFormatter* nc_formatter = [[IupFormatter alloc] init];
					[nc_formatter autorelease];
					[nc_formatter setIhandle:ih];
					[text_field setFormatter:nc_formatter];
				}
				else
				{
					[text_field setFormatter:nil];
				}
				return 1;
			}


			if(iupStrEqualNoCase(value, "NUMBER"))
			{
				IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
				[number_formatter autorelease];
				[number_formatter setIhandle:ih];
//				[number_formatter setFormattingContext:NSFormattingContextDynamic];
				[number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
				[number_formatter setPartialStringValidationEnabled:YES];
				[number_formatter setNumberStyle:NSNumberFormatterDecimalStyle];
				[text_field setFormatter:number_formatter];
				return 1;
			}
			if(iupStrEqualNoCase(value, "INTEGER"))
			{
				IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
				[number_formatter autorelease];
				[number_formatter setIhandle:ih];
//				[number_formatter setFormattingContext:NSFormattingContextDynamic];
				[number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
				[number_formatter setPartialStringValidationEnabled:YES];
				[number_formatter setNumberStyle:NSNumberFormatterNoStyle];
				[text_field setFormatter:number_formatter];
				return 1;
			}
			if(iupStrEqualNoCase(value, "SCIENTIFIC"))
			{
				IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
				[number_formatter autorelease];
				[number_formatter setIhandle:ih];
//				[number_formatter setFormattingContext:NSFormattingContextDynamic];
				[number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
				[number_formatter setPartialStringValidationEnabled:YES];
				[number_formatter setNumberStyle:NSNumberFormatterScientificStyle];
				[text_field setFormatter:number_formatter];
				return 1;
			}
			// requires the user to input a currency symbol which is kind of a pain
/*
			if(iupStrEqualNoCase(value, "CURRENCY"))
			{
				IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
				[number_formatter autorelease];
				[number_formatter setIhandle:ih];
//				[number_formatter setFormattingContext:NSFormattingContextDynamic];
				[number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
				[number_formatter setPartialStringValidationEnabled:YES];
				[number_formatter setNumberStyle:NSNumberFormatterCurrencyStyle];
				[text_field setFormatter:number_formatter];
				return 1;
			}
*/
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			// I'm not completely sure I want to allow this for stepper because this will overwrite the NumberFormatter I set in Interface Builder
			
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
	
			// TODO: We should restore the interface builder formatter, but I'm lazy.
			if(NULL == value)
			{
				IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
				[number_formatter autorelease];
				[number_formatter setIhandle:ih];
//				[number_formatter setFormattingContext:NSFormattingContextDynamic];
				[number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
				[number_formatter setPartialStringValidationEnabled:YES];
				[number_formatter setNumberStyle:NSNumberFormatterDecimalStyle];
				[text_field setFormatter:number_formatter];

				return 1;
			}
			
			if(iupStrEqualNoCase(value, "NUMBER"))
			{
				IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
				[number_formatter autorelease];
				[number_formatter setIhandle:ih];
//				[number_formatter setFormattingContext:NSFormattingContextDynamic];
				[number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
				[number_formatter setPartialStringValidationEnabled:YES];
				[number_formatter setNumberStyle:NSNumberFormatterDecimalStyle];
				[text_field setFormatter:number_formatter];
				return 1;
			}
			if(iupStrEqualNoCase(value, "INTEGER"))
			{
				IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
				[number_formatter autorelease];
				[number_formatter setIhandle:ih];
//				[number_formatter setFormattingContext:NSFormattingContextDynamic];
				[number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
				[number_formatter setPartialStringValidationEnabled:YES];
				[number_formatter setNumberStyle:NSNumberFormatterNoStyle];
				[text_field setFormatter:number_formatter];
				return 1;
			}
			if(iupStrEqualNoCase(value, "SCIENTIFIC"))
			{
				IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
				[number_formatter autorelease];
				[number_formatter setIhandle:ih];
//				[number_formatter setFormattingContext:NSFormattingContextDynamic];
				[number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
				[number_formatter setPartialStringValidationEnabled:YES];
				[number_formatter setNumberStyle:NSNumberFormatterScientificStyle];
				[text_field setFormatter:number_formatter];
				return 1;
			}
			// requires the user to input a currency symbol which is kind of a pain
/*
			if(iupStrEqualNoCase(value, "CURRENCY"))
			{
				IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
				[number_formatter autorelease];
				[number_formatter setIhandle:ih];
//				[number_formatter setFormattingContext:NSFormattingContextDynamic];
				[number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
				[number_formatter setPartialStringValidationEnabled:YES];
				[number_formatter setNumberStyle:NSNumberFormatterCurrencyStyle];
				[text_field setFormatter:number_formatter];
				return 1;
			}
*/

			break;
		}
		default:
		{
			break;
		}
	}
	
	return 0;
}

// This is currently broken.
// I don't really understand how setDefaultTabInterval is supposed to work.
// Also, I have to be very carefuly about not clobbering other attributes.
// I tried as create-only, which avoids the clobbering problem, but it still doesn't resolve the first issue.
/*
static int cocoaTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
	if(!ih->data->is_multiline)
	{
		return 0;
	}

  	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			IupCocoaFont* iup_font = iupCocoaGetFont(ih);
			NSTextStorage* text_storage = [text_view textStorage];
			
			
			NSRange full_range = NSMakeRange(0, [text_storage length]);
			
			if((full_range.location == 0) && (full_range.length==0))
			{
				NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
				[undo_manager beginUndoGrouping];
				//		[text_storage beginEditing];
				
				// Try setTypingAttributes out of desperation?
				NSMutableDictionary* typing_attributes = [[text_view typingAttributes] mutableCopy];
				[typing_attributes autorelease];
				NSMutableParagraphStyle* paragraph_style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
				[paragraph_style autorelease];
				[(NSMutableDictionary*)typing_attributes setObject:paragraph_style forKey:NSParagraphStyleAttributeName];
				[text_view setTypingAttributes:typing_attributes];
				
				
				//		[text_storage endEditing];
				[text_view didChangeText];
				[undo_manager endUndoGrouping];
				return 1;
			}
			
			
			
			
			NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
			[undo_manager beginUndoGrouping];
			[text_storage beginEditing];
			
			
			NSString* all_string = [text_storage string];
			
			// The problem seems to be that in order to change the TextLines attribute for the entire block, I need to re-set the attribute.
			// So I have to be very careful about applying attributes and not clobbering them.
			// Try going paragraph by paragraph to add the attribute so I don't clobber it with a global value.
			
			
			[all_string enumerateSubstringsInRange:full_range options:NSStringEnumerationByParagraphs usingBlock:
			 ^(NSString * _Nullable substring, NSRange substring_range, NSRange enclosing_range, BOOL * _Nonnull stop)
			 {
				 *stop = NO;
				 				NSLog(@"substring:%@", substring);
				 				NSLog(@"substringRange:%@", NSStringFromRange(substring_range));
				 				NSLog(@"enclosingRange:%@", NSStringFromRange(enclosing_range));
				 
				 NSDictionary<NSAttributedStringKey, id>* text_storage_attributes = nil;
				 NSMutableParagraphStyle* paragraph_style = nil;
				 
				 text_storage_attributes = [[text_storage attributedSubstringFromRange:enclosing_range] attributesAtIndex:0 effectiveRange:NULL];
				 paragraph_style = [[text_storage_attributes objectForKey:NSParagraphStyleAttributeName] mutableCopy];
				 [paragraph_style autorelease];
				 if(nil == paragraph_style)
				 {
					 				NSLog(@"nil == paragraph_style");
					 paragraph_style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
					 [paragraph_style autorelease];
				 }
				 
				 int tab_size_int = 8;
				 iupStrToInt(value, &tab_size_int);
				 
				 // WARNING: Cocoa uses points, not characters.
				 // FIXME/TODO: Should probably ask for font of beginning of line instead of the main font.
				 CGFloat tab_size_float = (CGFloat)(tab_size_int * [iup_font charWidth]);
				 
				 [paragraph_style setDefaultTabInterval:tab_size_float];
				 //			[paragraph_style setDefaultTabInterval:20 * [iup_font charWidth]];
				 [text_storage addAttribute:NSParagraphStyleAttributeName value:paragraph_style range:enclosing_range];
				 
				 
		
				 
			 }
			 ];
			
			[text_storage endEditing];

			// Try setTypingAttributes for end
			{
				NSMutableDictionary* typing_attributes = [[text_view typingAttributes] mutableCopy];
				[typing_attributes autorelease];
				NSMutableParagraphStyle* paragraph_style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
				[paragraph_style autorelease];
				[(NSMutableDictionary*)typing_attributes setObject:paragraph_style forKey:NSParagraphStyleAttributeName];
				[text_view setTypingAttributes:typing_attributes];
			}

			[text_view didChangeText];
			[undo_manager endUndoGrouping];
			return 1;

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			break;


		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
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
*/

static int cocoaTextSetValueAttrib(Ihandle* ih, const char* value)
{
	NSString* ns_string;
	
	if(NULL == value)
	{
		ns_string = @"";
	}
	else
	{
		ns_string = [NSString stringWithUTF8String:value];
	}
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			IupCocoaFont* iup_font = iupCocoaGetFont(ih);
			NSTextStorage* text_storage = [text_view textStorage];
			
			NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
			[undo_manager beginUndoGrouping];
			[text_storage beginEditing];
			
			
			NSAttributedString* attributed_string = [[NSAttributedString alloc] initWithString:ns_string attributes:[iup_font attributeDictionary]];
			[attributed_string autorelease];
			
			
			ih->data->disable_callbacks = 1;
			[text_view shouldChangeTextInRange:NSMakeRange(0, [text_storage length]) replacementString:ns_string];
			[text_storage setAttributedString:attributed_string];
			ih->data->disable_callbacks = 0;
			
			[text_storage endEditing];
			
			// We must call both shouldChangeTextInRange and didChangeText to keep the undo manager consistent
			[text_view didChangeText];
			[text_storage endEditing];
			[undo_manager endUndoGrouping];

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSCAssert([text_field isKindOfClass:[NSTextField class]], @"Expected NSTextField");
			IupCocoaFont* iup_font = iupCocoaGetFont(ih);
			if([iup_font usesAttributes])
			{
				NSAttributedString* attributed_string = [[NSAttributedString alloc] initWithString:ns_string attributes:[iup_font attributeDictionary]];
				[text_field setAttributedStringValue:attributed_string];
				[attributed_string release];
			
			}
			else
			{
				[text_field setStringValue:ns_string];
			}
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSCAssert([text_field isKindOfClass:[NSTextField class]], @"Expected NSTextField");
			
			// NSNumberFormatter is more resilant to of fancy numbers (e.g. commas, locales, etc) than NSString integerValue
			// And although the documentation says we only support INTEGERs,
			// because we let people muck with FILTER,
			// we may eventually end up in a more complicated case.
			NSNumberFormatter* conversion_formatter = [[NSNumberFormatter alloc] init];
			[conversion_formatter autorelease];
			NSNumberFormatter* text_field_formatter = (NSNumberFormatter*)[text_field formatter];
			[conversion_formatter setNumberStyle:[text_field_formatter numberStyle]];
			NSNumber* ns_number = [conversion_formatter numberFromString:ns_string];

			double current_number = [ns_number doubleValue];
			NSStepper* stepper_view = cocoaTextGetStepperView(ih);
			double max_value = [stepper_view maxValue];
			if(current_number > max_value)
			{
				current_number = max_value;
			}
			double min_value = [stepper_view minValue];
			if(current_number < min_value)
			{
				current_number = min_value;
			}

			// Do we need to worry about NSAttributedString?
			if(iupAttribGetBoolean(ih, "SPINAUTO"))
			{
				// With Cocoa Bindings, we must set the model or controller, not the view.
				// Otherwise, if we set the view directly,
				// when we click on the stepper, it will have a stale version of the world
				// and increment from the stale value instead of the value seen in the field.
				NSNumber* number_to_set = [NSNumber numberWithDouble:current_number];
				IUPStepperObject* stepper_object = cocoaTextGetStepperObject(ih);
				[stepper_object setStepperValue:number_to_set];
			}
			else
			{
				ns_string = [NSString stringWithFormat:@"%lf", current_number];
				[text_field setStringValue:ns_string];
			}
			
			
			
			break;
		}
		default:
		{
			break;
		}
	}
	
	return 0;
}

static char* cocoaTextGetValueAttrib(Ihandle* ih)
{
	char* value = NULL;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			
			NSString* ns_string = [[text_view textStorage] string];
			value = iupStrReturnStr([ns_string UTF8String]);
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			
			NSString* ns_string = [text_field stringValue];
			value = iupStrReturnStr([ns_string UTF8String]);
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			
			NSString* ns_string = [text_field stringValue];
			value = iupStrReturnStr([ns_string UTF8String]);
			
			break;
		}
		default:
		{
			break;
		}
	}
	
	if(NULL == value)
	{
		value = "";
	}
	
	return value;
}


static int cocoaTextSetBgColorAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			
			NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
			[undo_manager beginUndoGrouping];
			
			unsigned char r, g, b;
			if(iupStrToRGB(value, &r, &g, &b))
			{
				CGFloat red = r/255.0;
				CGFloat green = g/255.0;
				CGFloat blue = b/255.0;
				
				NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
				[text_view setBackgroundColor:the_color];
			}
			else
			{
				NSColor* the_color = [NSColor textBackgroundColor];
				[text_view setBackgroundColor:the_color];
			}
	
			
			// We must call both shouldChangeTextInRange and didChangeText to keep the undo manager consistent
			[text_view didChangeText];
			[undo_manager endUndoGrouping];

			return 1;

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			
			unsigned char r, g, b;
			if(iupStrToRGB(value, &r, &g, &b))
			{
				CGFloat red = r/255.0;
				CGFloat green = g/255.0;
				CGFloat blue = b/255.0;
				
				NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
				[text_field setBackgroundColor:the_color];
			}
			else
			{
				[text_field setBackgroundColor:nil];
			}
			return 1;

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			
			unsigned char r, g, b;
			if(iupStrToRGB(value, &r, &g, &b))
			{
				CGFloat red = r/255.0;
				CGFloat green = g/255.0;
				CGFloat blue = b/255.0;
				
				NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
				[text_field setBackgroundColor:the_color];
			}
			else
			{
				[text_field setBackgroundColor:nil];
			}

			return 1;

			break;
		}
		default:
		{
			break;
		}
	}
	
	return 0;
}

static int cocoaTextSetFgColorAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			// WARNING: FORMATTING is better to use than this
			NSTextView* text_view = cocoaTextGetTextView(ih);
			
			NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
			[undo_manager beginUndoGrouping];
			
			unsigned char r, g, b;
			if(iupStrToRGB(value, &r, &g, &b))
			{
				CGFloat red = r/255.0;
				CGFloat green = g/255.0;
				CGFloat blue = b/255.0;
				
				NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
				// setTextColor is provided by NSText
				[text_view setTextColor:the_color];
			}
			else
			{
				NSColor* the_color = [NSColor textBackgroundColor];
				[text_view setTextColor:the_color];
			}
	
			
			// We must call both shouldChangeTextInRange and didChangeText to keep the undo manager consistent
			[text_view didChangeText];
			[undo_manager endUndoGrouping];

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			
			unsigned char r, g, b;
			if(iupStrToRGB(value, &r, &g, &b))
			{
				CGFloat red = r/255.0;
				CGFloat green = g/255.0;
				CGFloat blue = b/255.0;
				
				NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
				[text_field setTextColor:the_color];
			}
			else
			{
				[text_field setTextColor:nil];
			}
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			
			unsigned char r, g, b;
			if(iupStrToRGB(value, &r, &g, &b))
			{
				CGFloat red = r/255.0;
				CGFloat green = g/255.0;
				CGFloat blue = b/255.0;
				
				NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
				[text_field setTextColor:the_color];
			}
			else
			{
				[text_field setTextColor:nil];
			}
			
			break;
		}
		default:
		{
			break;
		}
	}
	
	return iupdrvBaseSetBgColorAttrib(ih, value);
}


/// For the provided start_line, start_column, end_line, end_column, get the native NSRange for the selection.
static bool cocoaTextComputeRangeFromLineColumnForTextView(NSTextView* text_view, NSUInteger start_line, NSUInteger start_column, NSUInteger end_line, NSUInteger end_column, NSRange* out_range)
{
	*out_range = NSMakeRange(0, 0);

	if(end_line < start_line)
	{
		return false;
	}
	else if((end_line == start_line) && (end_column < start_column))
	{
		return false;
	}

	NSLayoutManager* layout_manager = [text_view layoutManager];
	NSUInteger number_of_glyphs = [layout_manager numberOfGlyphs];
	if(0 == number_of_glyphs)
	{
		if((start_line <= 1) && (start_column <= 1))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	NSUInteger number_of_lines;
	NSUInteger index;

	NSRange line_range = NSMakeRange(0, 0);

	bool found_start_line = false;
	bool found_start_column = false;
	NSUInteger selection_start_index = 0;

	for(number_of_lines = 0, index = 0; index < number_of_glyphs;)
	{
		NSUInteger last_index = index; // this is for a workaround where end_line==start_line
		NSRange last_range = line_range; // this is for a workaround where end_line==start_line
		
		(void) [layout_manager lineFragmentRectForGlyphAtIndex:index
			effectiveRange:&line_range];
		index = NSMaxRange(line_range);
		number_of_lines++;
		if(number_of_lines == start_line)
		{
			found_start_line = true;
			// line_range.length will tell us how manay glyphs there are on this line
			if(line_range.length >= start_column)
			{
				found_start_column = true;
				// The line_range.length may have overshot, so we need to subtract out the extra glyphs
				NSUInteger overshot_count = line_range.length - (start_column-1);
	
				// index contains our total running count of all glyphs, which is what we need to specify the range for text selection
				selection_start_index = index - overshot_count;
			}
			else
			{
				// Decision: If there are not enough columns on this line,
				// do we start the selection at the end of the line
				// or do we abort?
				found_start_column = false;
				selection_start_index = index;
			}
			
			// Edge case: end_line is the same line as start_line
			// The problem is we handle the end positions in the next block, but advance to the next line earlier in this loop.
			// The easiest workaround to do is back up the counters and re-do the next line in the next block if this is the case.
			if(start_line == end_line)
			{
				number_of_lines--;
				line_range = last_range;
				index = last_index;
			}
			
			break;
		}
	}

	if(!found_start_line)
	{
		NSLog(@"Failed to find startLine of %lu. Number of lines is %lu", start_line, number_of_lines);
		return false;
	}

	// If we comment out this block, we're going to allow being too short on columns.
/*
	if(!found_start_column)
	{
		NSLog(@"Failed to find startColumn line of %lu. Number of columns at line %lu is %lu", start_column, number_of_lines, line_range.location);
		return false;
	}
*/

	// Do end
	bool found_end_line = false;
	bool found_end_column = false;
	NSUInteger selection_end_index = 0;

	for(; index < number_of_glyphs;)
	{
		(void) [layout_manager lineFragmentRectForGlyphAtIndex:index
			effectiveRange:&line_range];
		index = NSMaxRange(line_range);
		number_of_lines++;
		if(number_of_lines == end_line)
		{
			found_end_line = true;
			// line_range.length will tell us how manay glyphs there are on this line
			if(line_range.length >= end_column)
			{
				found_start_column = true;
				// The line_range.length may have overshot, so we need to subtract out the extra glyphs
				NSUInteger overshot_count = line_range.length - (end_column-1);
	
				// index contains our total running count of all glyphs, which is what we need to specify the range for text selection
				selection_end_index = index - overshot_count;
			}
			else
			{
				// Decision: If there are not enough columns on this line,
				// do we start the selection at the end of the line
				// or do we abort?
				found_end_column = false;
				selection_end_index = index;
			}
			break;
		}
	}

	// Decision: If there are not enough lines,
	// do we select up to the end,
	// or do we abort?
	if(!found_end_line)
	{
#if 0
		NSLog(@"Failed to find startLine of %lu. Number of lines is %lu", start_line, number_of_lines);
		return false;
#else
		selection_end_index = index;
#endif
	}

	// If we comment out this block, we're going to allow being too short on columns.
/*
	if(!found_start_column)
	{
		NSLog(@"Failed to find startColumn line of %lu. Number of columns at line %lu is %lu", start_column, number_of_lines, line_range.length);
		return false;
	}
*/

	NSRange selection_range = NSMakeRange(selection_start_index, selection_end_index-selection_start_index);
//	[textView setSelectedRange:selection_range];
	*out_range = selection_range;

	return true;
}

/// For a provided native_selection_range, get the start_line, start_column, end_line, end_column
/// @param native_selection_range This is a range that something like [text_view selectedRange] would return
static bool cocoaTextComputeLineColumnFromRangeForTextView(NSTextView* text_view, NSRange native_selection_range, NSUInteger* out_start_line, NSUInteger* out_start_column, NSUInteger* out_end_line, NSUInteger* out_end_column)
{
	*out_start_line = 1;
	*out_start_column = 1;
	*out_end_line = 1;
	*out_end_column = 1;

	NSUInteger start_line = 1;
	NSUInteger start_column = 1;
	NSUInteger end_line = 1;
	NSUInteger end_column = 1;


	NSLayoutManager* layout_manager = [text_view layoutManager];
	NSUInteger number_of_glyphs = [layout_manager numberOfGlyphs];
	if(0 == number_of_glyphs)
	{
		if(native_selection_range.location == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	NSUInteger number_of_lines;
	NSUInteger index;
	NSRange line_range = NSMakeRange(0, 0);



	bool found_start_line = false;
	bool found_start_column = false;
	NSUInteger selection_start_index = native_selection_range.location;
	NSUInteger selection_end_index = native_selection_range.location + native_selection_range.length;

	for(number_of_lines = 0, index = 0; index < number_of_glyphs;)
	{
//		NSUInteger last_index = index; // this is for a workaround where end_line==start_line
//		NSRange last_range = line_range; // this is for a workaround where end_line==start_line
		
		(void) [layout_manager lineFragmentRectForGlyphAtIndex:index
			effectiveRange:&line_range];
		index = NSMaxRange(line_range);
		number_of_lines++;
		if(index >= selection_start_index)
		{
			found_start_line = true;
			start_line = number_of_lines;
			
			found_start_column = true;
			// Getting a full line at a time may have overshot, so we need to subtract out the extra glyphs
			NSUInteger overshot_count = index - (selection_start_index);

			// line_range.length will tell us how manay glyphs there are on this lin
			start_column = line_range.length - overshot_count + 1;


			// Edge case: end_line is the same line as start_line
			// The problem is we handle the end positions in the next block, but advance to the next line earlier in this loop.
			// So just handle it here and return immediately.
			if(index >= selection_end_index)
			{
				NSUInteger overshot_end_count = index - (selection_end_index);
				end_column = line_range.length - overshot_end_count + 1;
				end_line = start_line;
				
				*out_start_line = start_line;
				*out_start_column = start_column;
				*out_end_line = end_line;
				*out_end_column = end_column;
				return true;
			}
			break;
		}
	}

	if(!found_start_line)
	{
		NSLog(@"Failed to find start position at %lu. Number of glyphs is %lu", selection_start_index, number_of_glyphs);
		return false;
	}

	// If we comment out this block, we're going to allow being too short on columns.
/*
	if(!found_start_column)
	{
		NSLog(@"Failed to find startColumn line of %lu. Number of columns at line %lu is %lu", start_column, number_of_lines, line_range.location);
		return false;
	}
*/

	// Do end
	bool found_end_line = false;
	bool found_end_column = false;

	for(; index < number_of_glyphs;)
	{
		(void) [layout_manager lineFragmentRectForGlyphAtIndex:index
			effectiveRange:&line_range];
		index = NSMaxRange(line_range);
		number_of_lines++;
		if(index >= selection_end_index)
		{
			found_end_line = true;
			end_line = number_of_lines;
			
			found_end_column = true;

			// Getting a full line at a time may have overshot, so we need to subtract out the extra glyphs
			NSUInteger overshot_count = index - (selection_end_index);

			// line_range.length will tell us how manay glyphs there are on this lin
			end_column = line_range.length - overshot_count + 1;
			break;
		}
	}

	// Decision: If there are not enough lines,
	// do we select up to the end,
	// or do we abort?
	if(!found_end_line)
	{
#if 0
		NSLog(@"Failed to find end position of %lu. Number of gylphs is %lu", selection_end_index, number_of_glyphs);
		return false;
#else
		end_line = number_of_lines;
		end_column = line_range.length;
#endif
	}



	*out_start_line = start_line;
	*out_start_column = start_column;
	*out_end_line = end_line;
	*out_end_column = end_column;
	return true;
}

void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
//		NSLog(@"iupdrvTextConvertLinColToPos");
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			NSUInteger lin_start = lin;
			NSUInteger col_start = col;
			NSUInteger lin_end = lin;
			NSUInteger col_end = col;
			NSRange native_selection_range = NSMakeRange(0, 0);
			
			bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &native_selection_range);
			if(did_find_range)
			{
				if(pos)
				{
					// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
					*pos = (int)native_selection_range.location;
				}
			}
			return;
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			// This should be trivial, but it isn't quite...
			// Without newlines, it should be pos = col -1
			// Because the user can add newlines in a NSTextField (option-return), we may need to do some heroics.

			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];

			if(![field_editor isKindOfClass:[NSTextView class]])
			{
				if(pos)
				{
					if(col > 0)
					{
						*pos = col - 1;
					}
					else
					{
						// What do we do here?
						*pos = 0;
					}
				}
				return;
			}
			
			NSTextView* text_view = (NSTextView*)field_editor;
			NSUInteger lin_start = lin;
			NSUInteger col_start = col;
			NSUInteger lin_end = lin;
			NSUInteger col_end = col;
			NSRange native_selection_range = NSMakeRange(0, 0);
			
			bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &native_selection_range);
			if(did_find_range)
			{
				if(pos)
				{
					// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
					*pos = (int)native_selection_range.location;
				}
			}
			return;
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			// This should be trivial, but it isn't quite...
			// Without newlines, it should be pos = col -1
			// Because the user can add newlines in a NSTextField (option-return), we may need to do some heroics.

			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];

			if(![field_editor isKindOfClass:[NSTextView class]])
			{
				if(pos)
				{
					if(col > 0)
					{
						*pos = col - 1;
					}
					else
					{
						// What do we do here?
						*pos = 0;
					}
				}
				return;
			}
			
			NSTextView* text_view = (NSTextView*)field_editor;
			NSUInteger lin_start = lin;
			NSUInteger col_start = col;
			NSUInteger lin_end = lin;
			NSUInteger col_end = col;
			NSRange native_selection_range = NSMakeRange(0, 0);
			
			bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &native_selection_range);
			if(did_find_range)
			{
				if(pos)
				{
					// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
					*pos = (int)native_selection_range.location;
				}
			}
			return;

			break;
		}
		default:
		{
			break;
		}
	}

}

void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{

//	NSLog(@"iupdrvTextConvertPosToLinCol");

	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			// Use selectedRanges to get an array of multiple selections if we ever have to handle that
			NSRange range_position = NSMakeRange(pos, 0);

			
			NSUInteger lin_start=1;
			NSUInteger col_start=1;
			NSUInteger lin_end=1;
			NSUInteger col_end=1;
			bool did_find_range = cocoaTextComputeLineColumnFromRangeForTextView(text_view, range_position, &lin_start, &col_start, &lin_end, &col_end);
			if(did_find_range)
			{
				if(lin)
				{
					// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
					*lin = (int)lin_start;
				}
				if(col)
				{
					// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
					*col = (int)col_start;
				}
			}
			return;
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			// This should be trivial, but it isn't quite...
			// Without newlines, it should be pos = col -1
			// Because the user can add newlines in a NSTextField (option-return), we may need to do some heroics.

			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];

			if(![field_editor isKindOfClass:[NSTextView class]])
			{
				if(lin)
				{
					*lin = 1;
				}
				if(col)
				{
					*col = pos + 1;
				}

				return;
			}
			
			
			NSTextView* text_view = (NSTextView*)field_editor;
			NSUInteger lin_start=1;
			NSUInteger col_start=1;
			NSUInteger lin_end=1;
			NSUInteger col_end=1;
			NSRange range_position = NSMakeRange(pos, 0);
			bool did_find_range = cocoaTextComputeLineColumnFromRangeForTextView(text_view, range_position, &lin_start, &col_start, &lin_end, &col_end);
			if(did_find_range)
			{
				if(lin)
				{
					// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
					*lin = (int)lin_start;
				}
				if(col)
				{
					// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
					*col = (int)col_start;
				}
			}
			return;
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{

			// This should be trivial, but it isn't quite...
			// Without newlines, it should be pos = col -1
			// Because the user can add newlines in a NSTextField (option-return), we may need to do some heroics.

			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];

			if(![field_editor isKindOfClass:[NSTextView class]])
			{
				if(lin)
				{
					*lin = 1;
				}
				if(col)
				{
					*col = pos + 1;
				}

				return;
			}
			
			
			NSTextView* text_view = (NSTextView*)field_editor;
			NSUInteger lin_start=1;
			NSUInteger col_start=1;
			NSUInteger lin_end=1;
			NSUInteger col_end=1;
			NSRange range_position = NSMakeRange(pos, 0);
			bool did_find_range = cocoaTextComputeLineColumnFromRangeForTextView(text_view, range_position, &lin_start, &col_start, &lin_end, &col_end);
			if(did_find_range)
			{
				if(lin)
				{
					// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
					*lin = (int)lin_start;
				}
				if(col)
				{
					// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
					*col = (int)col_start;
				}
			}
			return;
			
			
			break;
		}
		default:
		{
			break;
		}
	}
}


// FIXME: I don't know if this is ever called.
void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
//	NSLog(@"iupdrvTextAddFormatTagStartBulk");
	NSTextView* text_view = cocoaTextGetTextView(ih);
	NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
	[undo_manager beginUndoGrouping];
	
	NSTextStorage* text_storage = [text_view textStorage];
	// I'm not sure if this is safe. The LayoutManager will throw an exception if a layout is done while beginEditing
	[text_storage beginEditing];
	return NULL;
}

// FIXME: I don't know if this is ever called.
void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
//	NSLog(@"iupdrvTextAddFormatTagStopBulk");
	
	NSTextView* text_view = cocoaTextGetTextView(ih);
	NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
	
	NSTextStorage* text_storage = [text_view textStorage];
	[text_storage endEditing];
	//	  [text_view didChangeText];
	[undo_manager endUndoGrouping];

}

// You must pass in a valid paragraph_style. This routine will alter it depending on which Iup attributes were set.
// Called by cocoaTextParseParagraphFormat to apply format tags.
// But broken out so it can also be used for setTypingAttributes:
static bool cocoaTextParseParagraphAttributes(NSMutableParagraphStyle* paragraph_style, Ihandle* formattag)
{
	bool needs_paragraph_style = false;
	char* format;
	
	format = iupAttribGet(formattag, "ALIGNMENT");
	if(format)
	{
		if(iupStrEqualNoCase(format, "JUSTIFY"))
		{
			[paragraph_style setAlignment:NSTextAlignmentJustified];
		}
		else if(iupStrEqualNoCase(format, "RIGHT"))
		{
			[paragraph_style setAlignment:NSTextAlignmentRight];
		}
		else if(iupStrEqualNoCase(format, "CENTER"))
		{
			[paragraph_style setAlignment:NSTextAlignmentCenter];
		}
		else /* "LEFT" */
		{
			[paragraph_style setAlignment:NSTextAlignmentLeft];
		}
		needs_paragraph_style = true;
	}
	
	format = iupAttribGet(formattag, "INDENT");
	if(format)
	{
		int val = 0;
		if(iupStrToInt(format, &val))
		{
			[paragraph_style setFirstLineHeadIndent:(CGFloat)val];
			needs_paragraph_style = true;
		}
		
		format = iupAttribGet(formattag, "INDENTRIGHT");
		if(format && iupStrToInt(format, &val))
		{
			// The Iup/Windows implementation seems to use the value relative to the right margin.
			// In Cocoa, positive values are the distance from the left margin, and negative values are from the right.
			// NOTE: Cocoa uses head/tail, not left/right because it supports right-to-left languages.
			[paragraph_style setTailIndent:(CGFloat)(-val)];
			needs_paragraph_style = true;
		}
		
		format = iupAttribGet(formattag, "INDENTOFFSET");
		if(format && iupStrToInt(format, &val))
		{
			[paragraph_style setHeadIndent:(CGFloat)(val)];
			needs_paragraph_style = true;
		}
	}
	
	
	format = iupAttribGet(formattag, "LINESPACING");
	if(format)
	{
		double val = 0;
		
		if(iupStrEqualNoCase(format, "SINGLE"))
		{
			[paragraph_style setLineSpacing:1.0];
		}
		else if(iupStrEqualNoCase(format, "ONEHALF"))
		{
			[paragraph_style setLineSpacing:1.5];
		}
		else if(iupStrEqualNoCase(format, "DOUBLE"))
		{
			[paragraph_style setLineSpacing:2.0];
		}
		else if(iupStrToDouble(format, &val))
		{
			[paragraph_style setLineSpacing:val];
		}
		needs_paragraph_style = true;
	}
	
	format = iupAttribGet(formattag, "SPACEBEFORE");
	if(format)
	{
		double val = 0;
		if(iupStrToDouble(format, &val))
		{
			[paragraph_style setParagraphSpacingBefore:val];
			needs_paragraph_style = true;
		}
	}
	
	format = iupAttribGet(formattag, "SPACEAFTER");
	if(format)
	{
		double val = 0;
		if(iupStrToDouble(format, &val))
		{
			[paragraph_style setParagraphSpacing:val];
			needs_paragraph_style = true;
		}
	}
	
	format = iupAttribGet(formattag, "TABSARRAY");
	if(format)
	{
		int pos = 0;
		char* str;
		NSMutableArray* tab_array = [NSMutableArray array];
		
		while(format)
		{
			str = iupStrDupUntil((char**)&format, ' ');
			if (!str) break;
			pos = atoi(str);
			free(str);
			
			str = iupStrDupUntil((char**)&format, ' ');
			if (!str) break;
			
			NSTextTab* text_tab = nil;
			// NOTE: DECIMAL is not supported.
			// Cocoa additioanlly supports natural and justified.
			if(iupStrEqualNoCase(str, "LEFT"))
			{
				text_tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentLeft location:(CGFloat)pos options:[NSDictionary dictionary]];
			}
			else if(iupStrEqualNoCase(str, "RIGHT"))
			{
				text_tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentRight location:(CGFloat)pos options:[NSDictionary dictionary]];
			}
			else if(iupStrEqualNoCase(str, "CENTER"))
			{
				text_tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentCenter location:(CGFloat)pos options:[NSDictionary dictionary]];
			}
			else /* fallback for unsupported options */
			{
				text_tab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentLeft location:(CGFloat)pos options:[NSDictionary dictionary]];
			}
			free(str);
			
			[tab_array addObject:text_tab];
			[text_tab release];
		}
		
		[paragraph_style setTabStops:tab_array];
		needs_paragraph_style = true;
	}

	return needs_paragraph_style;
}

// TODO/FIXME: I still get cases where I clobber formatting when (maybe?) I should not be clobbering. (The Iup spec is a bit undefined on this.)
// Example: The text case with 3 lines of containing "First Line" "Second Line Big Big Big" "Third Line"
// If you then set a color format for the selection of the 3 lines, you lose italics, underline, strikethrough. (But font size is preserved.)
// I'm thinking maybe the algorithm should be changed to use any enumeration APIs Cocoa may provide.
// Since of blindly applying the attributes for the selected block,
// we enumerate over the range and try to pick up each individual piece and try to preserve existing attributes for each section.
// Then we apply the new attribute over each sub-section.
// This will require modifcation to the calling algorthm, because it sets the attribute for the entire section from the caller.
// That will need to be pushed down into here.
// See:
// - (void)enumerateAttributesInRange:(NSRange)enumerationRange options:(NSAttributedStringEnumerationOptions)opts usingBlock:(void (NS_NOESCAPE ^)(NSDictionary<NSAttributedStringKey, id> *attrs, NSRange range, BOOL *stop))block API_AVAILABLE(macos(10.6), ios(4.0), watchos(2.0), tvos(9.0));
// And see the Bullet/Numbering code because it has started moving towards that model already.
static bool cocoaTextParseParagraphFormat(Ihandle* ih, Ihandle* formattag, NSTextView* text_view, NSRange selection_range)
{
	NSTextStorage* text_storage = [text_view textStorage];
	NSString* all_string = [text_storage string];
	__block bool did_change_attribute = false;
	// The problem seems to be that in order to change the TextLines attribute for the entire block, I need to re-set the attribute.
	// So I have to be very careful about applying attributes and not clobbering them.
	
	[text_storage beginEditing];
	
	
	[all_string enumerateSubstringsInRange:selection_range options:NSStringEnumerationByParagraphs usingBlock:
	 ^(NSString * _Nullable substring, NSRange substring_range, NSRange enclosing_range, BOOL * _Nonnull stop)
		{
			*stop = NO;
			// NSLog(@"substring:%@", substring);
			// NSLog(@"substringRange:%@", NSStringFromRange(substring_range));
			// NSLog(@"enclosingRange:%@", NSStringFromRange(enclosing_range));
			
			// If the selection block starts in the middle of the paragraph instead of the beginning,
			// we have to decide whether we should start as-is,
			// or back-up to the beginning.
			// We will use getParagraphStart to back up to the beginning.
			NSUInteger start_paragraph_index;
			NSUInteger end_paragraph_index;
			NSUInteger contents_end_paragraph_index;
			[all_string getParagraphStart:&start_paragraph_index end:&end_paragraph_index contentsEnd:&contents_end_paragraph_index forRange:enclosing_range];
			
			// NSLog(@"start_paragraph_index:%lu", start_paragraph_index);
			// NSLog(@"end_paragraph_index:%lu", end_paragraph_index);
			// NSLog(@"contents_end_paragraph_index:%lu", contents_end_paragraph_index);
			
			
			// end_paragraph_index seems to include the newline, which we want
			NSRange paragraph_range = NSMakeRange(start_paragraph_index, end_paragraph_index-start_paragraph_index);
			// NSLog(@"paragraph_range:%@", NSStringFromRange(paragraph_range));
			
			
			NSAttributedString* current_paragraph = [text_storage attributedSubstringFromRange:paragraph_range];
			//					NSLog(@"current_line: %@", current_line);
			NSDictionary<NSAttributedStringKey, id>* current_paragraph_attributes = [current_paragraph attributesAtIndex:0 effectiveRange:NULL];
			
			// Assumption: I'm going to assume I don't need to worry about substrings with different attributes because I am only changing the paragraph attribute which should apply to the whole paragraph.
			// That's why I'm ignoring the efffectiveRange: result here.
			// If I'm wrong, this code should either duplicate the cocoaTextParseCharacterFormat,
			// or be merged directly into cocoaTextParseParagraphFormat since if I'm wrong,
			// that implies there is no point of having distinct paragraph attributes anyway.
			
			
			// We only care about the ParagraphStyle attribute.
			// Get the existing one so we don't clobber existing properties, or if it doesn't exist, create a default one.
			NSMutableParagraphStyle* paragraph_style = nil;
			paragraph_style = [[current_paragraph_attributes objectForKey:NSParagraphStyleAttributeName] mutableCopy];
			if(nil == paragraph_style)
			{
				paragraph_style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
			}
			[paragraph_style autorelease];
			
			
			bool needs_paragraph_style = cocoaTextParseParagraphAttributes(paragraph_style, formattag);
			if(needs_paragraph_style)
			{
				[text_view shouldChangeTextInRange:paragraph_range replacementString:nil];
				[text_storage addAttribute:NSParagraphStyleAttributeName value:paragraph_style range:paragraph_range];
				did_change_attribute = true;
			}
		}
	];
	
	[text_storage endEditing];
	
	if(did_change_attribute)
	{
		[text_view didChangeText];
	}
	return did_change_attribute;
	
}


const NSUInteger kIupNumberingStyleNone = 0;
const NSUInteger kIupNumberingStyleRightParenthesis = 1;
const NSUInteger kIupNumberingStyleParenthesis = 2;
const NSUInteger kIupNumberingStylePeriod = 3;
const NSUInteger kIupNumberingStyleNonNumber = 4;

@interface IupNumberingTextList : NSTextList

@property(nonatomic, assign) NSUInteger numberingStyle;

// override
- (NSString*) markerForItemNumber:(NSInteger)item_num;

// convenience method
- (NSString*) markerWithTabsForItemNumber:(NSInteger)item_num;

@end

@implementation IupNumberingTextList

// We need to override this because when the user hits return to insert a new line,
// any custom formatting manually added to the marker will get discarded.
// Adding the custom formatting here should preserve our customizations.
- (NSString*) markerForItemNumber:(NSInteger)item_num
{
	NSString* base_string = [super markerForItemNumber:item_num];
	NSString* customized_marker = nil;
	switch([self numberingStyle])
	{
		case kIupNumberingStyleRightParenthesis:
		{
			customized_marker = [NSString stringWithFormat:@"%@)", base_string];
			break;
		}
		case kIupNumberingStyleParenthesis:
		{
			customized_marker = [NSString stringWithFormat:@"(%@)", base_string];
			break;
		}
		case kIupNumberingStylePeriod:
		{
			customized_marker = [NSString stringWithFormat:@"%@.", base_string];
			break;
		}
		case kIupNumberingStyleNonNumber:
		{
			customized_marker = @"";
			break;
		}
		
		case kIupNumberingStyleNone:
		default:
		{
			customized_marker = base_string;
			break;
		}
	}
	
	return customized_marker;
}

// When we use this to format (create) a list, we are required to add a leading and trailing tab.
// Apple's documentation doesn't state this, but this appears to be a requirement.
// However, I cannot put it directly in markerForItemNumber because it seems to screw up things
// when I hit return to insert new lines in a list. It appears in that case, Apple is trying to add tabs,
// and then gets too many and it thinks it needs to created new nested lists and gets screwy.
- (NSString*) markerWithTabsForItemNumber:(NSInteger)item_num
{
	NSString* base_string = [self markerForItemNumber:item_num];
	NSString* customized_marker = nil;
	customized_marker = [NSString stringWithFormat:@"\t%@\t", base_string];
	return customized_marker;
}

@end

/*
NOTES: Not supporting NUMBERINGTAB because following TextEdit.app conventions, we use two tabs (before & after the marker), so the API doesn't fit.
Using TABSARRAY seems to be the correct way to control indentation for this (looking at TextEdit.app).
Additionally, because we must manually inject the markers into the text, a variable number of tabs makes it harder to remove formatting because we already must use regex to find patterns since we don't necessarily know the marker type to remove.
Long term, we might like to support nested lists, which NUMBERINGTAB will make harder to detect since it could confuse the number of tabs to look for.
*/
static bool cocoaTextParseBulletNumberListFormat(Ihandle* ih, Ihandle* formattag, NSTextView* text_view, NSRange selection_range)
{
	// Apple doesn't provide these string constants until 10.13, so we provide our own copy.
	static NSString* IupNSTextListMarkerBox = @"{box}";
	static NSString* IupNSTextListMarkerCheck = @"{check}";
	static NSString* IupNSTextListMarkerCircle = @"{circle}";
	static NSString* IupNSTextListMarkerDiamond = @"{diamond}";
	static NSString* IupNSTextListMarkerDisc = @"{disc}";
	static NSString* IupNSTextListMarkerHyphen = @"{hyphen}";
	static NSString* IupNSTextListMarkerSquare = @"{square}";
	static NSString* IupNSTextListMarkerLowercaseHexadecimal = @"{lower-hexadecimal}";
	static NSString* IupNSTextListMarkerUppercaseHexadecimal = @"{upper-hexadecimal}";
	static NSString* IupNSTextListMarkerOctal = @"{octal}";
	static NSString* IupNSTextListMarkerLowercaseAlpha = @"{lower-alpha}";
	static NSString* IupNSTextListMarkerUppercaseAlpha = @"{upper-alpha}";
	static NSString* IupNSTextListMarkerLowercaseLatin = @"{lower-latin}"; // seems to be the same as alpha
	static NSString* IupNSTextListMarkerUppercaseLatin = @"{upper-latin}"; // seems to be the same as alpha
	static NSString* IupNSTextListMarkerLowercaseRoman = @"{lower-roman}";
	static NSString* IupNSTextListMarkerUppercaseRoman = @"{upper-roman}";
	static NSString* IupNSTextListMarkerDecimal = @"{decimal}";
	
	char* format;
	bool use_list = false;
	NSString* which_list_marker = nil;

	format = iupAttribGet(formattag, "NUMBERING");
	if(format)
	{
		use_list = true; // will unset in NONE case
		if(iupStrEqualNoCase(format, "BULLET"))
		{
			which_list_marker = IupNSTextListMarkerDisc;
		}
		else if(iupStrEqualNoCase(format, "ARABIC"))
		{
			which_list_marker = IupNSTextListMarkerDecimal;
		}
		else if(iupStrEqualNoCase(format, "LCLETTER"))
		{
			which_list_marker = IupNSTextListMarkerLowercaseAlpha;
		}
		else if(iupStrEqualNoCase(format, "UCLETTER"))
		{
			which_list_marker = IupNSTextListMarkerUppercaseAlpha;
		}
		else if(iupStrEqualNoCase(format, "LCROMAN"))
		{
			which_list_marker = IupNSTextListMarkerLowercaseRoman;
		}
		else if(iupStrEqualNoCase(format, "UCROMAN"))
		{
			which_list_marker = IupNSTextListMarkerUppercaseRoman;
		}
		else if(iupStrEqualNoCase(format, "NONE"))
		{
			use_list = false;
		}
		else if(iupStrEqualNoCase(format, "BOX"))
		{
			which_list_marker = IupNSTextListMarkerBox;
		}
		else if(iupStrEqualNoCase(format, "CHECK"))
		{
			which_list_marker = IupNSTextListMarkerCheck;
		}
		else if(iupStrEqualNoCase(format, "CIRCLE"))
		{
			which_list_marker = IupNSTextListMarkerCircle;
		}
		else if(iupStrEqualNoCase(format, "DIAMOND"))
		{
			which_list_marker = IupNSTextListMarkerDiamond;
		}
		else if(iupStrEqualNoCase(format, "HYPHEN"))
		{
			which_list_marker = IupNSTextListMarkerHyphen;
		}
		else if(iupStrEqualNoCase(format, "SQUARE"))
		{
			which_list_marker = IupNSTextListMarkerSquare;
		}
		else if(iupStrEqualNoCase(format, "LCHEX"))
		{
			which_list_marker = IupNSTextListMarkerLowercaseHexadecimal;
		}
		else if(iupStrEqualNoCase(format, "UCHEX"))
		{
			which_list_marker = IupNSTextListMarkerUppercaseHexadecimal;
		}
		else if(iupStrEqualNoCase(format, "OCTAL"))
		{
			which_list_marker = IupNSTextListMarkerOctal;
		}
		else if(iupStrEqualNoCase(format, "LCLATIN"))
		{
			which_list_marker = IupNSTextListMarkerLowercaseLatin;
		}
		else if(iupStrEqualNoCase(format, "UCLATIN"))
		{
			which_list_marker = IupNSTextListMarkerUppercaseLatin;
		}
		else
		{
			use_list = false;
		}
	
		if(use_list)
		{


			NSUInteger which_number_style = kIupNumberingStyleNone;
			
			format = iupAttribGet(formattag, "NUMBERINGSTYLE");
			if(format)
			{
				if(iupStrEqualNoCase(format, "RIGHTPARENTHESIS"))
				{
					which_number_style = kIupNumberingStyleRightParenthesis;
				}
				else if(iupStrEqualNoCase(format, "PARENTHESES"))
				{
					which_number_style = kIupNumberingStyleParenthesis;
				}
				else if(iupStrEqualNoCase(format, "PERIOD"))
				{
					which_number_style = kIupNumberingStylePeriod;
				}
				else if(iupStrEqualNoCase(format, "NONUMBER"))
				{
					which_number_style = kIupNumberingStyleNonNumber;
				}
				else if(iupStrEqualNoCase(format, "NONE"))
				{
					which_number_style = kIupNumberingStyleNone;
				}
				else
				{
					which_number_style = kIupNumberingStyleNone;
				}
			}



			NSTextStorage* text_storage = [text_view textStorage];
			NSString* all_string = [text_storage string];

			// The problem seems to be that in order to change the TextLines attribute for the entire block, I need to re-set the attribute.
			// So I have to be very careful about applying attributes and not clobbering them.
			
			[text_storage beginEditing];

			
			__block NSUInteger applied_paragraph_start = NSUIntegerMax;
			__block NSUInteger applied_paragraph_end = 0;
			__block NSMutableArray<NSTextList*>* array_of_text_lists = [NSMutableArray array];
			__block NSUInteger item_count = 1;
			[all_string enumerateSubstringsInRange:selection_range options:NSStringEnumerationByParagraphs usingBlock:
				^(NSString * _Nullable substring, NSRange substring_range, NSRange enclosing_range, BOOL * _Nonnull stop)
				{
					*stop = NO;
	//				NSLog(@"substring:%@", substring);
	//				NSLog(@"substringRange:%@", NSStringFromRange(substring_range));
	//				NSLog(@"enclosingRange:%@", NSStringFromRange(enclosing_range));

					// If the selection block starts in the middle of the paragraph instead of the beginning,
					// we have to decide whether we should start as-is,
					// or back-up to the beginning.
					// TextEdit.app backs up to the beginning, so we will use getParagraphStart to back up to the beginning.
					NSUInteger start_paragraph_index;
					NSUInteger end_paragraph_index;
					NSUInteger contents_end_paragraph_index;
					[all_string getParagraphStart:&start_paragraph_index end:&end_paragraph_index contentsEnd:&contents_end_paragraph_index forRange:enclosing_range];

	//				NSLog(@"start_paragraph_index:%lu", start_paragraph_index);
	//				NSLog(@"end_paragraph_index:%lu", end_paragraph_index);
	//				NSLog(@"contents_end_paragraph_index:%lu", contents_end_paragraph_index);

					if(applied_paragraph_start > start_paragraph_index)
					{
						applied_paragraph_start = start_paragraph_index;
					}
					if(applied_paragraph_end < end_paragraph_index)
					{
						applied_paragraph_end = end_paragraph_index;
					}

					// end_paragraph_index seems to include the newline, which we want
					NSRange paragraph_range = NSMakeRange(start_paragraph_index, end_paragraph_index-start_paragraph_index);
//					NSLog(@"paragraph_range:%@", NSStringFromRange(paragraph_range));


					IupNumberingTextList* text_list = [[IupNumberingTextList alloc] initWithMarkerFormat:which_list_marker options:0];
					[text_list autorelease];
					[text_list setNumberingStyle:which_number_style];
					[array_of_text_lists addObject:text_list];
					
					NSMutableAttributedString* current_line = [[text_storage attributedSubstringFromRange:paragraph_range] mutableCopy];
					[current_line autorelease];
//					NSLog(@"current_line: %@", current_line);
					NSDictionary<NSAttributedStringKey, id>* current_line_attributes = [current_line attributesAtIndex:0 effectiveRange:NULL];

//					NSLog(@"current_line_attributes: %@", current_line_attributes);

					NSString* marker_with_style_prefix = nil;
					marker_with_style_prefix = [text_list markerWithTabsForItemNumber:item_count];
					
					NSAttributedString* attribued_prefix = [[NSAttributedString alloc] initWithString:marker_with_style_prefix attributes:current_line_attributes];
					[attribued_prefix autorelease];
					[current_line insertAttributedString:attribued_prefix atIndex:0];
//					[current_line fixAttributesInRange:NSMakeRange(0, [current_line length])];

					ih->data->disable_callbacks = 1;
					// For undo manager
					[text_view shouldChangeTextInRange:paragraph_range replacementString:[current_line string]];
					ih->data->disable_callbacks = 0;
					[text_storage replaceCharactersInRange:paragraph_range withAttributedString:current_line];



					item_count++;
				
				}
			];


			if(applied_paragraph_start == NSUIntegerMax)
			{
				applied_paragraph_start = selection_range.location;
			}
			if(applied_paragraph_end == 0)
			{
				applied_paragraph_end = selection_range.location + selection_range.length;
			}

			
			NSRange applied_paragraph_range = NSMakeRange(applied_paragraph_start, applied_paragraph_end-applied_paragraph_start);
			NSMutableDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[[text_storage attributedSubstringFromRange:applied_paragraph_range] attributesAtIndex:0 effectiveRange:NULL] mutableCopy];
			[text_storage_attributes autorelease];
			NSMutableParagraphStyle* paragraph_style = [[text_storage_attributes objectForKey:NSParagraphStyleAttributeName] mutableCopy];
			[paragraph_style autorelease];
			if(nil == paragraph_style)
			{
//				NSLog(@"nil == paragraph_style");
				paragraph_style = [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
				[paragraph_style autorelease];
			}
			
			// Be careful here:
			// It seems I must do this last, otherwise the list doesn't take effect.
			// (You should be able to add a new line in-between the lines, and it should automatically
			// insert a new bullet and renumber all the lines accordingly.)
			// But applying this last seems to blow away some attributes.
			// Use addAttribute and avoid setAttributes
			// In the text.c test,
			// I lose the strike-through and italics on "Second Line"
			// I lose the alignment on "Third Line" (this is not completely unreasonable)
			[paragraph_style setTextLists:array_of_text_lists];
			[text_storage addAttribute:NSParagraphStyleAttributeName value:paragraph_style range:applied_paragraph_range];


			[text_storage endEditing];
			[text_view didChangeText];

			
		}
		else // we attempt to remove list formatting
		{
			NSTextStorage* text_storage = [text_view textStorage];
			NSString* all_string = [text_storage string];

			// The problem seems to be that in order to remove the TextLines attribute for the entire block, I need to re-set the attribute.
			// So I have to be very careful about applying attributes and not clobbering them.

			
			[text_storage beginEditing];

			__block NSUInteger applied_paragraph_start = NSUIntegerMax;
			__block NSUInteger applied_paragraph_end = 0;
			[all_string enumerateSubstringsInRange:selection_range options:NSStringEnumerationByParagraphs usingBlock:
				^(NSString * _Nullable substring, NSRange substring_range, NSRange enclosing_range, BOOL * _Nonnull stop)
				{
					*stop = NO;
	//				NSLog(@"substring:%@", substring);
	//				NSLog(@"substringRange:%@", NSStringFromRange(substring_range));
	//				NSLog(@"enclosingRange:%@", NSStringFromRange(enclosing_range));

					// If the selection block starts in the middle of the paragraph instead of the beginning,
					// we have to decide whether we should start as-is,
					// or back-up to the beginning.
					// TextEdit.app backs up to the beginning, so we will use getParagraphStart to back up to the beginning.
					NSUInteger start_paragraph_index;
					NSUInteger end_paragraph_index;
					NSUInteger contents_end_paragraph_index;
					[all_string getParagraphStart:&start_paragraph_index end:&end_paragraph_index contentsEnd:&contents_end_paragraph_index forRange:enclosing_range];
					if(applied_paragraph_start > start_paragraph_index)
					{
						applied_paragraph_start = start_paragraph_index;
					}
					if(applied_paragraph_end < end_paragraph_index)
					{
						applied_paragraph_end = end_paragraph_index;
					}
	//				NSLog(@"start_paragraph_index:%lu", start_paragraph_index);
	//				NSLog(@"end_paragraph_index:%lu", end_paragraph_index);
	//				NSLog(@"contents_end_paragraph_index:%lu", contents_end_paragraph_index);

					// end_paragraph_index seems to include the newline, which we want
					NSRange paragraph_range = NSMakeRange(start_paragraph_index, end_paragraph_index-start_paragraph_index);
				//	NSRange paragraph_range = NSMakeRange(start_paragraph_index, contents_end_paragraph_index-start_paragraph_index);
	//				NSLog(@"paragraph_range:%@", NSStringFromRange(paragraph_range));


					NSMutableAttributedString* current_line = [[text_storage attributedSubstringFromRange:paragraph_range] mutableCopy];
					[current_line autorelease];
					NSString* current_line_nsstr = [current_line string];
	//				NSLog(@"current_line bef:%@", current_line);

					NSString* marker_prefix_pattern = @"^\t.*?\t";
					NSError* ns_error = nil;
					NSRegularExpression* reg_ex = [NSRegularExpression
						regularExpressionWithPattern:marker_prefix_pattern
						options:NSRegularExpressionAnchorsMatchLines
						error:&ns_error
					];
					
					NSArray<NSTextCheckingResult*>* regex_matches = [reg_ex matchesInString:current_line_nsstr
						options:kNilOptions
						range:NSMakeRange(0, [current_line_nsstr length])
					];
					
					// There should only be 1 match right now since we anchor with ^
					// But maybe in the future we can leverage the loop to look at nested lists
					for(NSTextCheckingResult* match in regex_matches)
					{
	//					NSLog(@"match:%@", match);
	//					NSLog(@"match range:%@", NSStringFromRange([match range]));
						[current_line deleteCharactersInRange:[match range]];
	//					NSLog(@"current_line aft:%@", current_line);
					}

					// Must be [current_line string] and not current_line_nsstr because we mutated current_line and current_line_nsstr may be out of date
					ih->data->disable_callbacks = 1;
					[text_view shouldChangeTextInRange:paragraph_range replacementString:[current_line string]];
					ih->data->disable_callbacks = 0;
					[text_storage replaceCharactersInRange:paragraph_range withAttributedString:current_line];


				}
			];
			NSRange applied_paragraph_range = NSMakeRange(applied_paragraph_start, applied_paragraph_end-applied_paragraph_start);
			NSMutableDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[[text_storage attributedSubstringFromRange:applied_paragraph_range] attributesAtIndex:0 effectiveRange:NULL] mutableCopy];
			[text_storage_attributes autorelease];
			NSMutableParagraphStyle* paragraph_style = [[text_storage_attributes objectForKey:NSParagraphStyleAttributeName] mutableCopy];
			[paragraph_style autorelease];
			if(nil == paragraph_style)
			{
				// There was no TextList, so there is nothing to remove.
//				NSLog(@"Nothing to remove");
			}
			else
			{
				// Can't set nil, so set empty array
				[paragraph_style setTextLists:[NSArray array]];
				/*
				// Be careful here:
				// Use removeAttribute and not setAttributes
				*/
//				NSLog(@"removed textlist");

				[text_storage removeAttribute:NSParagraphStyleAttributeName range:applied_paragraph_range];

			}

			[text_storage endEditing];
			[text_view didChangeText];
		}
		return true;
	} // end if NUMBERING
	return false;
}



static NSFont* cocoaTextChangeFontWeight(NSFont* start_font, int font_target_weight)
{
	// Ugh: This has no effect.
	// [attribute_dict setValue:[NSNumber numberWithFloat:weight_scale] forKey:NSFontWeightTrait];
	
	// I don't want to create a font from scratch because there are warnings about using FamilyName for the default/system font.
	// And I just don't want to accidentally miss any properties.
	// But this leaves us only with this curious API which will attempt to bump up/down the weight by 1 level at a time.
	// If it can't go any higher/lower, it returns back the original font.
	// The levels are not well defined, so this is very imprecise.
	
	NSFontManager* font_manager = [NSFontManager sharedFontManager];
	NSFont* target_font = start_font;

	NSInteger current_weight = [font_manager weightOfFont:target_font];
	if(current_weight < font_target_weight)
	{
		// need to go up
		while(current_weight < font_target_weight)
		{
			NSFont* result_font;
			result_font = [font_manager convertWeight:YES ofFont:target_font];
			current_weight = [font_manager weightOfFont:target_font];
			
			if(result_font == target_font)
			{
				// unable to convert more
				break;
			}
			else
			{
				target_font = result_font;
			}
		}
	}
	else if(current_weight > font_target_weight)
	{
		// need to go down
		while(current_weight > font_target_weight)
		{
			NSFont* result_font;
			result_font = [font_manager convertWeight:NO ofFont:target_font];
			current_weight = [font_manager weightOfFont:target_font];
			
			if(result_font == target_font)
			{
				// unable to convert more
				break;
			}
			else
			{
				target_font = result_font;
			}
		}
	}
	else
	{
		// already at target weight
	}
	return target_font;
}


// This returns a non-nil dictionary only for the setTypingAttributes: case (change formatting at the end of the document).
// Calls may merge that dictionary with whatever to call setTypingAttributes:
// Otherwise it changes the formatting at the selected range and returns nil.
//
// Be careful about clobbering:
// Example: The text case with 3 lines of containing "First Line" "Second Line Big Big Big" "Third Line" with different sizes, strike through, underline, colors, alignments in different sections.
// If you then set a color format for the selection of the 3 lines,
// you don't want to lose italics, underline, strikethrough, font size, alignment (paragraph attribute)
//
// This is like the 3rd time I've re-written this.
// The problem has been clobbering previous properties that have been set earlier.
// I generally don't want to lose the old properties if they are orthogonal to the ones being set.
// This is compounded when the user selects a range that spans characters that have applied different attributes.
// So this algorithm now attempts to traverse the substrings broken up by different attribute types,
// and uses addAttributes to only add/overwrite changes to the substring to try to avoid clobbering.
static NSMutableDictionary* cocoaTextParseCharacterFormat(Ihandle* ih, Ihandle* formattag, NSTextView* text_view, NSRange selection_range)
{
	char* format;
	bool did_change_attribute = false;
	bool did_change_font_size = false;
	bool did_change_font_family = false;
	bool did_change_font_traits = false;
	bool did_change_font_weight = false;

	bool did_change_font_fgcolor = false;
	bool did_change_font_bgcolor = false;

	// This is a bonus for calling removeObject:forKey: if the user provided NULL for a color and there was a color already set.
	bool needs_add_font_fgcolor = false;
	bool needs_add_font_bgcolor = false;


	NSString* font_family_name = nil;
	CGFloat font_size = 0.0;
	NSFontTraitMask trait_mask = 0;
	int font_target_weight = 5;
	NSMutableDictionary* attribute_dict = [NSMutableDictionary dictionary];
	

	format = iupAttribGet(formattag, "FONTSIZE");
	if(format)
	{
		int font_size_int = 0;
		iupStrToInt(format, &font_size_int);
		// less than 0 is in "pixels",
		// but Apple platforms don't really support this because of all the things they do for resolution independent scaling (e.g. retina)
		// All things must be in points.
		if(font_size_int < 0)
		{
		
		}
		else
		{
			font_size = (CGFloat)font_size_int;
			did_change_font_size = true;
			did_change_attribute = true;
		}
	}
	
	format = iupAttribGet(formattag, "FONTFACE");
	if(format)
	{
		/* Map standard names to native names */
		const char* mapped_name = iupFontGetMacName(format);
		if(mapped_name)
		{
			font_family_name = [NSString stringWithUTF8String:mapped_name];
		}
		else
		{
			font_family_name = [NSString stringWithUTF8String:format];
		}
		
//		target_font = [font_manager convertFont:target_font toFamily:font_family_name];
		did_change_attribute = true;
		did_change_font_family = true;
	}

	format = iupAttribGet(formattag, "RISE");
	if(format)
	{
		// For Mac, NSSuperscriptAttributeName
		// iOS kCTSuperscriptAttributeName
		// Pass in a negative value for subscript.
    	if(iupStrEqualNoCase(format, "SUPERSCRIPT"))
		{
	  		[attribute_dict setValue:[NSNumber numberWithInt:1]
				forKey:NSSuperscriptAttributeName];
			// I don't know how small to make the font. IUP uses X-SMALL on GTK.
			font_size = font_size * 0.6444444444444;
			did_change_font_size = true;
		}
		else if(iupStrEqualNoCase(format, "SUBSCRIPT"))
		{
	  		[attribute_dict setValue:[NSNumber numberWithInt:-1]
				forKey:NSSuperscriptAttributeName];
			// I don't know how small to make the font. IUP uses X-SMALL on GTK.
			font_size = font_size * 0.6444444444444;
			did_change_font_size = true;
		}
		else
		{
			int offset_val = 0;
			iupStrToInt(format, &offset_val);
			[attribute_dict setValue:[NSNumber numberWithDouble:(double)offset_val]
				forKey:NSBaselineOffsetAttributeName];
			// user is expected to set the font size for this case
		}
		did_change_attribute = true;
	}


	format = iupAttribGet(formattag, "FONTSCALE");
	if(format)
	{
		double fval = 0;
		if (iupStrEqualNoCase(format, "XX-SMALL"))
			fval = 0.5787037037037;
		else if (iupStrEqualNoCase(format, "X-SMALL"))
			fval = 0.6444444444444;
		else if (iupStrEqualNoCase(format, "SMALL"))
			fval = 0.8333333333333;
		else if (iupStrEqualNoCase(format, "MEDIUM"))
			fval = 1.0;
		else if (iupStrEqualNoCase(format, "LARGE"))
			fval = 1.2;
		else if (iupStrEqualNoCase(format, "X-LARGE"))
			fval = 1.4399999999999;
		else if (iupStrEqualNoCase(format, "XX-LARGE"))
			fval = 1.728;
		else
			iupStrToDouble(format, &fval);
		
		if(fval > 0)
		{
			font_size = font_size * fval;
			did_change_font_size = true;
			did_change_attribute = true;
		}
	}


	

	format = iupAttribGet(formattag, "ITALIC");
	if(format)
	{
		if(iupStrBoolean(format))
		{
			trait_mask |= NSItalicFontMask;
			did_change_attribute = true;
			did_change_font_traits = true;
		}
	}


	format = iupAttribGet(formattag, "UNDERLINE");
	if(format)
	{
		// TODO: Apple supports DOTTED (and DASHED) independently of SINGLE/DOUBLE.
		if(iupStrEqualNoCase(format, "SINGLE"))
		{
			[attribute_dict setValue:[NSNumber numberWithInt:NSUnderlinePatternSolid|NSUnderlineStyleSingle]
				forKey:NSUnderlineStyleAttributeName];
		}
		else if(iupStrEqualNoCase(format, "DOUBLE"))
		{
			[attribute_dict setValue:[NSNumber numberWithInt:NSUnderlinePatternSolid|NSUnderlineStyleDouble]
				forKey:NSUnderlineStyleAttributeName];
		}
		else if(iupStrEqualNoCase(format, "DOTTED"))
		{
			[attribute_dict setValue:[NSNumber numberWithInt:NSUnderlineStylePatternDot|NSUnderlineStyleSingle]
				forKey:NSUnderlineStyleAttributeName];
		}
		else /* "NONE" */
		{
			[attribute_dict setValue:[NSNumber numberWithInt:NSUnderlineStyleNone]
				forKey:NSUnderlineStyleAttributeName];
		}
		did_change_attribute = true;
	}


	format = iupAttribGet(formattag, "STRIKEOUT");
	if(format)
	{
		if(iupStrBoolean(format))
		{
	  		[attribute_dict setValue:[NSNumber numberWithInt:YES]
				forKey:NSStrikethroughStyleAttributeName];
		}
		else
		{
			[attribute_dict setValue:[NSNumber numberWithInt:NO]
				forKey:NSStrikethroughStyleAttributeName];
		}
		did_change_attribute = true;
	}

	format = iupAttribGet(formattag, "WEIGHT");
	if(format)
	{
		/*
		Apple Terminology						ISO Equivalent
		1. ultralight
		2. thin W1. ultralight
		3. light, extralight 					W2. extralight
		4. book									W3. light
		5. regular, plain, display, roman		W4. semilight
		6. medium								W5. medium
		7. demi, demibold
		8. semi, semibold						W6. semibold
		9. bold									W7. bold
		10. extra, extrabold					W8. extrabold
		11. heavy, heavyface
		12. black, super						W9. ultrabold
		13. ultra, ultrablack, fat
		14. extrablack, obese, nord
		
		The NSFontManager implementation of this method refuses to convert a font’s weight
		if it can’t maintain all other traits, such as italic and condensed.
		You might wish to override this method to allow a looser interpretation of weight conversion
		*/
		// NSFontWeightTrait: The valid value range is from -1.0 to 1.0. The value of 0.0 corresponds to the regular or medium font weight.
//		CGFloat weight_scale = 0;
		// weightOfFont: An approximation of the weight of the given font, where 0 indicates the lightest possible weight, 5 indicates a normal or book weight, and 9 or more indicates a bold or heavier weight.
		int target_weight = 5;
		
		if(iupStrEqualNoCase(format, "EXTRALIGHT"))
		{
//			weight_scale = -1.0;
			target_weight = 0;
		}
		else if(iupStrEqualNoCase(format, "LIGHT"))
		{
//			weight_scale = -0.5;
			target_weight = 3;
		}
		else if(iupStrEqualNoCase(format, "SEMIBOLD"))
		{
//			weight_scale = 0.25;
			target_weight = 6;
		}
		else if(iupStrEqualNoCase(format, "BOLD"))
		{
//			weight_scale = 0.50;
			target_weight = 8;
		}
		else if (iupStrEqualNoCase(format, "EXTRABOLD"))
		{
//			weight_scale = 0.75;
			target_weight = 10;
		}
		else if(iupStrEqualNoCase(format, "HEAVY"))
		{
//			weight_scale = 1.0;
			target_weight = 11;
		}
		else /* "NORMAL" */
		{
//			weight_scale = 0.0;
			target_weight = 5;
		}
		font_target_weight = target_weight;

		did_change_attribute = true;
		did_change_font_weight = true;

	}
	
	
	
	format = iupAttribGet(formattag, "FGCOLOR");
	if(format)
	{
		unsigned char r, g, b;
		if(iupStrToRGB(format, &r, &g, &b))
		{
			CGFloat red = r/255.0;
			CGFloat green = g/255.0;
			CGFloat blue = b/255.0;

			NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
			[attribute_dict setValue:the_color
				forKey:NSForegroundColorAttributeName];

			needs_add_font_fgcolor = true;
		}
		else
		{
			// removing is unnecessary with the current algorthm since it shouldn't be in there, but also won't hurt
//			[attribute_dict removeObjectForKey:NSForegroundColorAttributeName];
			needs_add_font_fgcolor = false;
		}
		did_change_attribute = true;
		did_change_font_fgcolor = true;
	}
	
	format = iupAttribGet(formattag, "BGCOLOR");
	if(format)
	{
		unsigned char r, g, b;
		if(iupStrToRGB(format, &r, &g, &b))
		{
			CGFloat red = r/255.0;
			CGFloat green = g/255.0;
			CGFloat blue = b/255.0;

			NSColor* the_color = [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:1.0];
			[attribute_dict setObject:the_color
				forKey:NSBackgroundColorAttributeName];
				needs_add_font_bgcolor = true;
		}
		else
		{
			// removing is unnecessary with the current algorthm since it shouldn't be in there, but also won't hurt
//			[attribute_dict removeObjectForKey:NSBackgroundColorAttributeName];
			needs_add_font_bgcolor = false;
		}
		did_change_attribute = true;
		did_change_font_bgcolor = true;
	}
	
	// Disabled color (not actually disabled)
	// Watch-out, FGCOLOR can conflict with this because they both use NSForegroundColorAttributeName
	format = iupAttribGet(formattag, "DISABLED");
	if(format)
	{
		if(iupStrBoolean(format))
		{
			// Is it secondarySelectedControlColor or disabledControlTextColor?
			NSColor* the_color = [NSColor disabledControlTextColor];
//			NSColor* the_color = [NSColor secondarySelectedControlColor];
			[attribute_dict setValue:the_color
				forKey:NSForegroundColorAttributeName];
			did_change_font_fgcolor = true;
			needs_add_font_fgcolor = true;
		}
		else
		{
			// If the user specified FGCOLOR, then it was already set above.
			if(iupAttribGet(formattag, "FGCOLOR"))
			{
			
			}
			else
			{
				// this removes the FGCOLOR if it was set
				[attribute_dict removeObjectForKey:NSForegroundColorAttributeName];
				did_change_font_fgcolor = true;
				needs_add_font_fgcolor = false;
			}

		}
		did_change_attribute = true;
	}
	
	
	
	
	

	if(did_change_attribute)
	{
		NSFontManager* font_manager = [NSFontManager sharedFontManager];
		IupCocoaFont* iup_font = iupCocoaGetFont(ih);

		NSTextStorage* text_storage = [text_view textStorage];
		
		
		// Ugh: I need to handle two separate cases that don't quite fit.
		// Case 1: Apply formatting to existing strings.
		// Case 2: Apply formatting to all new text (setTypingAttributes:)
		// So I have a bunch of code duplication.

		
		if(selection_range.location < [text_storage length])
		{
			[text_storage beginEditing];
			ih->data->disable_callbacks = 1;
			//		NSString* text_string = [text_storage string];
			
			
			// This loop will iterate through all the sub-strings with different attributes
			// We are using attributesAtIndex:effectiveRange:
			// where the return value of effectiveRange: will tell us how far the current substring goes until the next attribute change.
			// We then apply the new attributes to each piece at a time.
			NSUInteger loc = selection_range.location;
			NSUInteger end = selection_range.length + loc;
			while(loc < end)
			{	/* Run through the string in terms of attachment runs */
				NSRange attrib_range;	/* Attachment attribute run */
				
				NSRange sub_range = {loc, end-loc};
				NSAttributedString* current_substring = [text_storage attributedSubstringFromRange:sub_range];
				NSDictionary<NSAttributedStringKey, id>* current_substring_attributes = [current_substring attributesAtIndex:0 effectiveRange:&attrib_range];
		
				NSFont* target_font = nil;

				if(did_change_font_family || did_change_font_traits || did_change_font_size || did_change_font_weight)
				{
					NSFont* base_font = [current_substring_attributes objectForKey:NSFontAttributeName];
					if(nil == base_font)
					{
						base_font = [iup_font nativeFont];
					}
					target_font = base_font;
					
					if(did_change_font_family)
					{
						target_font = [font_manager convertFont:target_font toFamily:font_family_name];
					}
					if(did_change_font_size)
					{
						target_font = [font_manager convertFont:target_font toSize:font_size];
					}
					if(did_change_font_traits)
					{
						target_font = [font_manager convertFont:target_font toHaveTrait:trait_mask];
					}
					
					
					
					if(did_change_font_weight)
					{
						target_font = cocoaTextChangeFontWeight(target_font, font_target_weight);
					}
					
					[attribute_dict setObject:target_font forKey:NSFontAttributeName];
					
				}
				NSRange adjusted_range = { loc, attrib_range.length };
				[text_view shouldChangeTextInRange:adjusted_range replacementString:nil];
				[text_storage addAttributes:attribute_dict range:adjusted_range];
				
				
				// This is going above & beyond the IUP required spec.
				// I am removing color attributes if the user passed NULL (or illegal values)
				if(did_change_font_fgcolor && !needs_add_font_fgcolor)
				{
					[text_storage removeAttribute:NSForegroundColorAttributeName range:adjusted_range];
				}
				if(did_change_font_bgcolor && !needs_add_font_bgcolor)
				{
					[text_storage removeAttribute:NSBackgroundColorAttributeName range:adjusted_range];
				}
				
				
				loc += attrib_range.length;
				
			}
			
			ih->data->disable_callbacks = 0;
			[text_storage endEditing];
			[text_view didChangeText];
			
			// We used the dictionary and no longer need it.
			// Returning a non-nil dictionary is used by case 2:
			attribute_dict = nil;
			
		}
		else // for setTypingAttributes:
		{
			// Instead of setting the typingAttributes here, we are going to prepare an attribute dictionary to return.
			// The reason is that the paragraph attribute handler passes back modifications,
			// so returning a dictionary that can be later merged with that one makes things easier.


			NSDictionary<NSAttributedStringKey, id>* current_substring_attributes = [text_view typingAttributes];
		
			NSFont* target_font = nil;

			if(did_change_font_family || did_change_font_traits || did_change_font_size || did_change_font_weight)
			{
				NSFont* base_font = [current_substring_attributes objectForKey:NSFontAttributeName];
				if(nil == base_font)
				{
					base_font = [iup_font nativeFont];
				}
				target_font = base_font;
				
				if(did_change_font_family)
				{
					target_font = [font_manager convertFont:target_font toFamily:font_family_name];
				}
				if(did_change_font_size)
				{
					target_font = [font_manager convertFont:target_font toSize:font_size];
				}
				if(did_change_font_traits)
				{
					target_font = [font_manager convertFont:target_font toHaveTrait:trait_mask];
				}
				
				
				
				if(did_change_font_weight)
				{
					target_font = cocoaTextChangeFontWeight(target_font, font_target_weight);
				}
				
				[attribute_dict setObject:target_font forKey:NSFontAttributeName];
			}
	
		}
		
	}
	
	if(did_change_attribute)
	{
		return attribute_dict;
	}
	else
	{
		return nil;
	}
}

void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
	if(!ih->data->is_multiline)
	{
		return;
	}
	if(!ih->data->has_formatting)
	{
		return;
	}
	NSTextView* text_view = cocoaTextGetTextView(ih);


	char* iup_selection = NULL;
	NSRange native_selection_range = {0, 0};
	iup_selection = iupAttribGet(formattag, "SELECTION");
	if(iup_selection)
	{
		int ret_val;
		NSUInteger lin_start=1;
		NSUInteger col_start=1;
		NSUInteger lin_end=1;
		NSUInteger col_end=1;

		ret_val = sscanf(iup_selection, "%lu,%lu:%lu,%lu", &lin_start, &col_start, &lin_end, &col_end);
		if(ret_val != 4)
		{
			return;
		}
		if(lin_start<1 || col_start<1 || lin_end<1 || col_end<1)
		{
			return;
		}
		
		bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &native_selection_range);
		if(!did_find_range)
		{
			return;
		}
	}
	else
	{
		char* iup_selection_pos = iupAttribGet(formattag, "SELECTIONPOS");
		if(iup_selection_pos)
		{
			// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
			int start_int = 0;
			int end_int = 0;

			if(iupStrToIntInt(iup_selection_pos, &start_int, &end_int, ':')!=2)
			{
				return;
			}
      		if(start_int<0 || end_int<0)
      		{
		        return;
			}
			NSUInteger start = (NSUInteger)start_int;
			NSUInteger end = (NSUInteger)end_int;
			native_selection_range = NSMakeRange(start, end-start);
		}
		else
		{

			NSTextStorage* text_storage = [text_view textStorage];
			native_selection_range = NSMakeRange([text_storage length], 0);

			NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
			[undo_manager beginUndoGrouping];
			
			NSDictionary* typing_attributes = [text_view typingAttributes];
			NSMutableParagraphStyle* paragraph_style = [[typing_attributes objectForKey:NSParagraphStyleAttributeName] mutableCopy];
			if(nil == paragraph_style)
			{
				paragraph_style = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
			}
			[paragraph_style autorelease];
			bool needs_paragraph_style_change = cocoaTextParseParagraphAttributes(paragraph_style, formattag);
			

			NSMutableDictionary* new_character_attributes = cocoaTextParseCharacterFormat(ih, formattag, text_view, native_selection_range);
			
			if(new_character_attributes || needs_paragraph_style_change)
			{
				// Merge all the attributes together
				NSMutableDictionary* merged_attributes = [typing_attributes mutableCopy];
				[merged_attributes autorelease];
				
				if(new_character_attributes != nil)
				{
					// This should overwrite any new properties with the old, while retaining the unchanged attributes
					[merged_attributes addEntriesFromDictionary:new_character_attributes];
				}
				if(needs_paragraph_style_change)
				{
					[merged_attributes setObject:paragraph_style forKey:NSParagraphStyleAttributeName];
				}
			
				[text_view setTypingAttributes:merged_attributes];
				typing_attributes = merged_attributes;
			}
			

			// Append a newline so we have a fresh line to enable bullet lists
			// If we don't, the previous line gets converted into a list which is often not what I think people expect.
			NSAttributedString* attributed_append_string = [[NSAttributedString alloc] initWithString:@"\n" attributes:typing_attributes];
			[attributed_append_string autorelease];
			
			// We need to mark the proposed change range (before we change it) in order to call shouldChangeTextInRange:
			NSRange change_range = NSMakeRange([text_storage length], 0);
			ih->data->disable_callbacks = 1;
			[text_view shouldChangeTextInRange:change_range replacementString:[attributed_append_string string]];
			[text_storage beginEditing];
			[text_storage appendAttributedString:attributed_append_string];
			  ih->data->disable_callbacks = 0;
			[text_storage endEditing];
		
		
			// We need a length of least 1 or an exception gets thrown inside cocoaTextParseBulletNumberListFormat
			// Hence why we append a newline.
			native_selection_range = NSMakeRange([text_storage length]-1, 1);
//			native_selection_range = NSMakeRange(change_range.location, 1);
			cocoaTextParseBulletNumberListFormat(ih, formattag, text_view, native_selection_range);
			
			// The result of the above yields an extra blank line after the bullet.
			// So another hack is the delete the final newline
			change_range = NSMakeRange([text_storage length]-1, 1);
			ih->data->disable_callbacks = 1;
			[text_view shouldChangeTextInRange:change_range replacementString:@""];
			[text_storage beginEditing];
			[text_storage deleteCharactersInRange:change_range];
			  ih->data->disable_callbacks = 0;
			[text_storage endEditing];
		
		
			
			[undo_manager endUndoGrouping];
			
			// Return immediately. The fall-through code is for selection-only
			return;
			
		}
	}
	
	
	IupCocoaFont* iup_font = iupCocoaGetFont(ih);

	// Use the current set font as the baseline. We will modify a local copy of its attributes from there.
	NSMutableDictionary* attribute_dict = [[iup_font attributeDictionary] mutableCopy];
	[attribute_dict autorelease];
	NSTextStorage* text_storage = [text_view textStorage];
	// Get the attributes (if any) for the selected range, and merge it into our copy of the font's attributes.
	// This will overwrite/merge the current attributes into our font copy attributes.
	NSDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[text_storage attributedSubstringFromRange:native_selection_range] attributesAtIndex:0 effectiveRange:NULL];
	[attribute_dict addEntriesFromDictionary:text_storage_attributes];


	
//	NSLog(@"iupdrvTextAddFormatTag: %@", NSStringFromRange(native_selection_range));
	NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
	[undo_manager beginUndoGrouping];
	
//	[text_storage beginEditing];
	// Only the bullet/number feature changes the text. nil should express attribute-only change
	[text_view shouldChangeTextInRange:native_selection_range replacementString:nil];


	// cocoaTextParseBulletNumberListFormat actually re-adjusts its ranges to fit the list block.
	// So it doesn't use the attribute_dict for our current range.
	// I originally had a lot of problems with attributes clobbering each other, so order used to matter.
	// I think some of those issues are improved, so order may not be as important now.
	cocoaTextParseBulletNumberListFormat(ih, formattag, text_view, native_selection_range);

	cocoaTextParseParagraphFormat(ih, formattag, text_view, native_selection_range);
	cocoaTextParseCharacterFormat(ih, formattag, text_view, native_selection_range);

	
//	[text_storage endEditing];

	[text_view didChangeText];
	[undo_manager endUndoGrouping];
	

}



static char* cocoaTextGetFormattingAttrib(Ihandle* ih)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			
			NSCAssert(([cocoaTextGetTextView(ih) isRichText] == ih->data->has_formatting), @"Expecting isRichText==ih->data->has_formatting");
			return iupStrReturnBoolean(ih->data->has_formatting);
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		default:
		{
			break;
		}
	}
	
  return iupStrReturnBoolean(false);
}

static int cocoaTextSetFormattingAttrib(Ihandle* ih, const char* value)
{
	if(!ih->handle)
	{
		// This is before map.
		// Let the variable be set, so we can read it inside Map and do the right thing.
		ih->data->has_formatting = iupStrBoolean(value);
		return 0;
	}

	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			bool enable_formatting = iupStrBoolean(value);
			
			if(enable_formatting)
			{
				NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
//				[undo_manager beginUndoGrouping];
				[text_view setRichText:enable_formatting];
//				[text_view setImportsGraphics:enable_formatting];
//				[undo_manager endUndoGrouping];
				
				// I can't seem to undo setRichText:
				// NOTE: If this could be made to work, to correctly implement this,
				// I would need to override setRichText: to keep ih->data->has_formatting in sync for the undo.
				// But since I can't make it work, reset the undo stack.
				[undo_manager removeAllActions];
				
			}
			else
			{
				NSTextView* text_view = cocoaTextGetTextView(ih);
				NSRange selection_range;
				NSTextStorage* text_storage = [text_view textStorage];
				selection_range = NSMakeRange(0, [text_storage length]);
				
				NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
//				[undo_manager beginUndoGrouping];
				[text_storage beginEditing];
				
				ih->data->disable_callbacks = 1;
				[text_view shouldChangeTextInRange:selection_range replacementString:nil];
				[text_storage setAttributes:nil range:selection_range];
				ih->data->disable_callbacks = 0;
				
				[text_storage endEditing];
				
				IupCocoaFont* iup_font = iupCocoaGetFont(ih);

				[text_view setTypingAttributes:[iup_font attributeDictionary]];
	//			[text_view setImportsGraphics:enable_formatting];
				[text_view setRichText:enable_formatting];
			
				// We must call both shouldChangeTextInRange and didChangeText to keep the undo manager consistent
				[text_view didChangeText];
				[text_storage endEditing];
//				[undo_manager endUndoGrouping];

				// I can't seem to undo setRichText:
				// NOTE: If this could be made to work, to correctly implement this,
				// I would need to override setRichText: to keep ih->data->has_formatting in sync for the undo.
				// But since I can't make it work, reset the undo stack.
				[undo_manager removeAllActions];
			}
			
			
			ih->data->has_formatting = enable_formatting;
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		default:
		{
			ih->data->has_formatting = 0;
			break;
		}
	}
	
	return 0;
}


static int cocoaTextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
	if(!ih->data->is_multiline)
	{
		return 0;
	}
	
	NSTextView* text_view = cocoaTextGetTextView(ih);
	NSRange selection_range;
	NSTextStorage* text_storage = [text_view textStorage];
	
	if(iupStrEqualNoCase(value, "ALL"))
	{
		selection_range = NSMakeRange(0, [text_storage length]);
	}
	else
	{
		// Use selectedRanges to get an array of multiple selections if we ever have to handle that
		selection_range = [text_view selectedRange];
		if(NSNotFound == selection_range.location)
		{
			return 0;
		}
	}
	
	NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
	[undo_manager beginUndoGrouping];
	[text_storage beginEditing];
	
	ih->data->disable_callbacks = 1;
	[text_view shouldChangeTextInRange:selection_range replacementString:nil];
	[text_storage setAttributes:nil range:selection_range];
	ih->data->disable_callbacks = 0;
	
	[text_storage endEditing];
	
	// We must call both shouldChangeTextInRange and didChangeText to keep the undo manager consistent
	[text_view didChangeText];
	[text_storage endEditing];
	[undo_manager endUndoGrouping];
	
	return 0;
}


static int cocoaTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			
			// NSText provides these methods
			if(iupStrEqualNoCase(value, "ARIGHT"))
			{
				[text_view setAlignment:NSTextAlignmentRight];
			}
			else if (iupStrEqualNoCase(value, "ACENTER"))
			{
				[text_view setAlignment:NSTextAlignmentCenter];
			}
			else /* "ALEFT" */
			{
				[text_view setAlignment:NSTextAlignmentLeft];
			}
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			// NSControl provides these methods
			if(iupStrEqualNoCase(value, "ARIGHT"))
			{
				[text_field setAlignment:NSTextAlignmentRight];
			}
			else if (iupStrEqualNoCase(value, "ACENTER"))
			{
				[text_field setAlignment:NSTextAlignmentCenter];
			}
			else /* "ALEFT" */
			{
				[text_field setAlignment:NSTextAlignmentLeft];
			}

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			// NSControl provides these methods
			if(iupStrEqualNoCase(value, "ARIGHT"))
			{
				[text_field setAlignment:NSTextAlignmentRight];
			}
			else if (iupStrEqualNoCase(value, "ACENTER"))
			{
				[text_field setAlignment:NSTextAlignmentCenter];
			}
			else /* "ALEFT" */
			{
				[text_field setAlignment:NSTextAlignmentLeft];
			}

			break;
		}
		default:
		{
			break;
		}
	}

	return 1;
}




static char* cocoaTextGetSelectedTextAttrib(Ihandle* ih)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			// Note: use selectedRanges if we need to support multiple selection
			NSRange selected_range = [text_view selectedRange];
			NSString* selected_string = [[[text_view textStorage] string] substringWithRange:selected_range];
			return iupStrReturnStr([selected_string UTF8String]);
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSRange selected_range = [field_editor selectedRange];
			NSString* selected_string = [[field_editor string] substringWithRange:selected_range];
			return iupStrReturnStr([selected_string UTF8String]);
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSRange selected_range = [field_editor selectedRange];
			NSString* selected_string = [[field_editor string] substringWithRange:selected_range];
			return iupStrReturnStr([selected_string UTF8String]);
			break;
		}
		default:
		{
			break;
		}
	}
	
	return NULL;
}

static int cocoaTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
	if(NULL == value)
	{
		return 0;
	}
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			// Note: use selectedRanges if we need to support multiple selection
			NSRange selected_range = [text_view selectedRange];
			// make sure something is selected
			if((NSNotFound == selected_range.location) || (0 == selected_range.length))
			{
				return 0;
			}
			
			// We can't use NSTextView insertText because
			// if this is used to programmatically alter text while the widget is disabled,
			// then the insert will fail.
			// So we must alter NSTextStorage directly.
			
			NSTextStorage* text_storage = [text_view textStorage];
			IupCocoaFont* iup_font = iupCocoaGetFont(ih);

			// Use the current set font as the baseline. We will modify a local copy of its attributes from there.
			NSMutableDictionary* attribute_dict = [[iup_font attributeDictionary] mutableCopy];
			[attribute_dict autorelease];
			// Set the selection range to be at the very last character.
			// Get the attributes (if any) for the selected range, and merge it into our copy of the font's attributes.
			// This will overwrite/merge the current attributes into our font copy attributes.
			// Needs to be 1 character or it raises an exception
			NSDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[text_storage attributedSubstringFromRange:selected_range] attributesAtIndex:0 effectiveRange:NULL];
			[attribute_dict addEntriesFromDictionary:text_storage_attributes];
			
			NSString* ns_insert_string = [NSString stringWithUTF8String:value];
			
			NSAttributedString* attributed_insert_string = [[NSAttributedString alloc] initWithString:ns_insert_string attributes:attribute_dict];
			[attributed_insert_string autorelease];
			
			
			NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
			[undo_manager beginUndoGrouping];
			
			ih->data->disable_callbacks = 1;
			[text_view shouldChangeTextInRange:selected_range replacementString:[attributed_insert_string string]];
			[text_storage beginEditing];
			
			[text_storage replaceCharactersInRange:selected_range withAttributedString:attributed_insert_string];
			
			ih->data->disable_callbacks = 0;
			
			[text_storage endEditing];
			
			// We must call both shouldChangeTextInRange and didChangeText to keep the undo manager consistent
			[text_view didChangeText];
			[undo_manager endUndoGrouping];


			return 0;
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSRange selected_range = [field_editor selectedRange];
			if((NSNotFound == selected_range.location) || (0 == selected_range.length))
			{
				return 0;
			}
			
			NSString* ns_string = [NSString stringWithUTF8String:value];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			[(NSTextView*)field_editor insertText:ns_string replacementRange:selected_range];
			return 0;
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSRange selected_range = [field_editor selectedRange];
			if((NSNotFound == selected_range.location) || (0 == selected_range.length))
			{
				return 0;
			}
			
			NSString* ns_string = [NSString stringWithUTF8String:value];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			[(NSTextView*)field_editor insertText:ns_string replacementRange:selected_range];
			return 0;
			
			break;
		}
		default:
		{
			break;
		}
	}
	return 0;
}

static int cocoaTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
	NSTextView* text_view = nil;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			text_view = cocoaTextGetTextView(ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;
			

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;

			break;
		}
		default:
		{
			return 0;
			break;
		}
	}
	
	

	NSRange selection_range = NSMakeRange(0, 0);
	
	
	if (!value || iupStrEqualNoCase(value, "NONE"))
	{
		//			start = 0;
		//			end = 0;
	}
	else if (iupStrEqualNoCase(value, "ALL"))
	{
		NSTextStorage* text_storage = [text_view textStorage];
		NSUInteger start;
		NSUInteger end;
		start = 0;
		end = [text_storage length];
		selection_range = NSMakeRange(start, end);
	}
	else
	{
		int ret_val;
		NSUInteger lin_start=1;
		NSUInteger col_start=1;
		NSUInteger lin_end=1;
		NSUInteger col_end=1;
		
		
		switch(sub_type)
		{
			case IUPCOCOATEXTSUBTYPE_VIEW:
			{
				ret_val = sscanf(value, "%lu,%lu:%lu,%lu", &lin_start, &col_start, &lin_end, &col_end);
				if(ret_val != 4)
				{
					return 0;
				}
				if(lin_start<1 || col_start<1 || lin_end<1 || col_end<1)
				{
					return 0;
				}
				break;
			}
			case IUPCOCOATEXTSUBTYPE_FIELD:
			case IUPCOCOATEXTSUBTYPE_STEPPER:
			{
				int col_start_int = 1;
				int col_end_int = 1;
				if(iupStrToIntInt(value, &col_start_int, &col_end_int, ':')!=2)
				{
					return 0;
				}
				col_start = col_start_int;
				col_end = col_end_int;

				if(col_start<0 || col_end<0)
				{
					return 0;
				}
				break;
			}
			default:
			{
				return 0;
				break;
			}
		}
		
		bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &selection_range);
		if(!did_find_range)
		{
			return 0;
		}
	}
	
	[text_view setSelectedRange:selection_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];

	return 0;
}

static char* cocoaTextGetSelectionAttrib(Ihandle* ih)
{
	NSTextView* text_view = nil;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			text_view = cocoaTextGetTextView(ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;
			

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;

			break;
		}
		default:
		{
			return NULL;
			break;
		}
	}


	// Use selectedRanges to get an array of multiple selections if we ever have to handle that
	NSRange selected_range = [text_view selectedRange];
	if(NSNotFound == selected_range.location)
	{
		return NULL;
	}
	
	NSUInteger lin_start=1;
	NSUInteger col_start=1;
	NSUInteger lin_end=1;
	NSUInteger col_end=1;
	bool did_find_range = cocoaTextComputeLineColumnFromRangeForTextView(text_view, selected_range, &lin_start, &col_start, &lin_end, &col_end);
	if(!did_find_range)
	{
		return NULL;
	}
	

	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			return iupStrReturnStrf("%lu,%lu:%lu,%lu", lin_start, col_start, lin_end, col_end);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			// FIXME: There is an edge case where a \n is snuck into the NSTextField. This will break things.
			// This algorithm may be overkill.
			if(lin_end > 1)
			{
				did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, 1, col_start, 1, col_end, &selected_range);
				if(did_find_range)
				{
					// pos is 0 indexed, col is 1 indexed, so we need to add 1
					col_start = selected_range.location + 1;
					col_end = col_start + selected_range.length;
				}
				else
				{
					return NULL;
				}
			}
			
			// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
			return iupStrReturnIntInt((int)col_start, (int)col_end, ':');

			break;
		}
		default:
		{
			return NULL;
			break;
		}
	}
	

	return NULL;
}


static int cocoaTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
	NSTextView* text_view = nil;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			text_view = cocoaTextGetTextView(ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;
			

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;

			break;
		}
		default:
		{
			return 0;
			break;
		}
	}

	NSUInteger start;
	NSUInteger end;
	NSRange selection_range = NSMakeRange(0, 0);
	
	
	if (!value || iupStrEqualNoCase(value, "NONE"))
	{
		//			start = 0;
		//			end = 0;
	}
	else if (iupStrEqualNoCase(value, "ALL"))
	{
		NSTextStorage* text_storage = [text_view textStorage];
		
		start = 0;
		end = [text_storage length];
		selection_range = NSMakeRange(start, end);
	}
	else
	{
		// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
		int start_int = 0;
		int end_int = 0;
		
		if(iupStrToIntInt(value, &start_int, &end_int, ':')!=2)
		{
			return 0;
		}
		if(start_int<0 || end_int<0)
		{
			return 0;
		}
		start = (NSUInteger)start_int;
		end = (NSUInteger)end_int;
		selection_range = NSMakeRange(start, end-start);
	}
	
	[text_view setSelectedRange:selection_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];
	


	return 0;
}

static char* cocoaTextGetSelectionPosAttrib(Ihandle* ih)
{
	NSTextView* text_view = nil;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			text_view = cocoaTextGetTextView(ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;
			

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;

			break;
		}
		default:
		{
			return NULL;
			break;
		}
	}
	
	// Use selectedRanges to get an array of multiple selections if we ever have to handle that
	NSRange selected_range = [text_view selectedRange];
	if(NSNotFound == selected_range.location)
	{
		return NULL;
	}
	NSUInteger start = selected_range.location;
	NSUInteger end = selected_range.location + selected_range.length;
	
	// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
	return iupStrReturnIntInt((int)start, (int)end, ':');
	
}

// TODO: This is almost identical to Caret. (Caret also sets the cursor.) Should make Caret call this.
static int cocoaTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
	NSTextView* text_view = nil;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			text_view = cocoaTextGetTextView(ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;
			

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;

			break;
		}
		default:
		{
			return 0;
			break;
		}
	}
	
	
	NSRange cursor_range = NSMakeRange(0, 0);
	
	// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
	int start_int = 0;
	int end_int = 0;
	
	if(iupStrToIntInt(value, &start_int, &end_int, ':')!=2)
	{
		return 0;
	}
	if(start_int<0 || end_int<0)
	{
		return 0;
	}
	NSUInteger lin_start=start_int;
	NSUInteger col_start=end_int;
	NSUInteger lin_end=start_int;
	NSUInteger col_end=end_int;
	bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &cursor_range);
	if(did_find_range)
	{
		[text_view scrollRangeToVisible:cursor_range];
	}
	

	return 0;
}

// TODO: This is almost identical to Caret. (Caret also sets the cursor.) Should make Caret call this.
static int cocoaTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
	NSTextView* text_view = nil;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			text_view = cocoaTextGetTextView(ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;
			

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;

			break;
		}
		default:
		{
			return 0;
			break;
		}
	}
	
	NSRange cursor_range = NSMakeRange(0, 0);
	
	// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
	int pos = 0;
	if(!iupStrToInt(value, &pos))
	{
		return 0;
	}
	if(pos<0)
	{
		return 0;
	}
	cursor_range = NSMakeRange(pos, 0);
	

	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			[text_view scrollRangeToVisible:cursor_range];
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{

			break;
		}
		default:
		{
			return 0;
			break;
		}
	}
	return 0;
}

// TODO: This is almost identical to ScrollTo. (Caret also sets the cursor.) Should make Caret call ScrollTo.
static int cocoaTextSetCaretAttrib(Ihandle* ih, const char* value)
{
	NSTextView* text_view = nil;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			text_view = cocoaTextGetTextView(ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;
			

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;

			break;
		}
		default:
		{
			return 0;
			break;
		}
	}
	
	
	NSRange cursor_range = NSMakeRange(0, 0);
	
	// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
	int start_int = 0;
	int end_int = 0;
	
	if(iupStrToIntInt(value, &start_int, &end_int, ':')!=2)
	{
		return 0;
	}
	if(start_int<0 || end_int<0)
	{
		return 0;
	}
	NSUInteger lin_start=start_int;
	NSUInteger col_start=end_int;
	NSUInteger lin_end=start_int;
	NSUInteger col_end=end_int;
	bool did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, lin_start, col_start, lin_end, col_end, &cursor_range);
	if(did_find_range)
	{
		[text_view setSelectedRange:cursor_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];
		[text_view scrollRangeToVisible:cursor_range];
	}
	

	return 0;
}

static char* cocoaTextGetCaretAttrib(Ihandle* ih)
{
	NSTextView* text_view = nil;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			text_view = cocoaTextGetTextView(ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;
			

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;

			break;
		}
		default:
		{
			return NULL;
			break;
		}
	}


	NSRange cursor_range = [[[text_view selectedRanges] lastObject] rangeValue];
	if(NSNotFound == cursor_range.location)
	{
		// what do we do?
		return NULL;
	}
	
	NSUInteger lin_start=1;
	NSUInteger col_start=1;
	NSUInteger lin_end=1;
	NSUInteger col_end=1;
	bool did_find_range = cocoaTextComputeLineColumnFromRangeForTextView(text_view, cursor_range, &lin_start, &col_start, &lin_end, &col_end);
	if(!did_find_range)
	{
		return NULL;
	}
	
	[text_view scrollRangeToVisible:cursor_range];


	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
			return iupStrReturnIntInt((int)lin_start, (int)col_start, ':');

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			// FIXME: There is an edge case where a \n is snuck into the NSTextField. This will break things.
			// This algorithm may be overkill.
			if(lin_end > 1)
			{
				did_find_range = cocoaTextComputeRangeFromLineColumnForTextView(text_view, 1, col_start, 1, col_end, &cursor_range);
				if(did_find_range)
				{
					// pos is 0 indexed, col is 1 indexed, so we need to add 1
					col_start = cursor_range.location + 1;
//					col_end = col_start + cursor_range.length;
				}
				else
				{
					return NULL;
				}
			}
			
			// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
   			 return iupStrReturnInt((int)col_start);

			break;
		}
		default:
		{
			return NULL;
			break;
		}
	}
	
	return NULL;
}

// TODO: This is almost identical to ScrollPos. (Caret also sets the cursor.) Should make Caret call ScrollPos.
static int cocoaTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
	NSTextView* text_view = nil;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			text_view = cocoaTextGetTextView(ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;
			

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;

			break;
		}
		default:
		{
			return 0;
			break;
		}
	}
	
	NSRange cursor_range = NSMakeRange(0, 0);
	
	// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
	int pos = 0;
	if(!iupStrToInt(value, &pos))
	{
		return 0;
	}
	if(pos<0)
	{
		return 0;
	}
	cursor_range = NSMakeRange(pos, 0);
	
	[text_view setSelectedRange:cursor_range affinity:NSSelectionAffinityDownstream stillSelecting:NO];

	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			[text_view scrollRangeToVisible:cursor_range];
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{

			break;
		}
		default:
		{
			return 0;
			break;
		}
	}
	return 0;
}

static char* cocoaTextGetCaretPosAttrib(Ihandle* ih)
{
	NSTextView* text_view = nil;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			text_view = cocoaTextGetTextView(ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;
			

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			text_view = (NSTextView*)field_editor;

			break;
		}
		default:
		{
			return NULL;
			break;
		}
	}
	
	NSRange cursor_range = [[[text_view selectedRanges] lastObject] rangeValue];
	if(NSNotFound == cursor_range.location)
	{
		// what do we do?
		return NULL;
	}
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			[text_view scrollRangeToVisible:cursor_range];
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{

			break;
		}
		default:
		{
			return NULL;
			break;
		}
	}
	// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
	return iupStrReturnInt((int)cursor_range.location);
}


static char* cocoaTextGetLineValueAttrib(Ihandle* ih)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);

			
			NSRange cursor_range = [[[text_view selectedRanges] lastObject] rangeValue];
			if(NSNotFound == cursor_range.location)
			{
				// what do we do?
				return NULL;
			}
			
			// We don't want the entire range, just the start cursor position (in case the user has selected multiple lines)
			cursor_range.length = 0;
			
			NSTextStorage* text_storage = [text_view textStorage];
			NSString* text_string = [text_storage string];
			NSUInteger start_index = 0;
			NSUInteger contents_end_index = 0;
			
			[text_string getLineStart:&start_index end:NULL contentsEnd:&contents_end_index forRange:cursor_range];
			
			NSRange line_range = NSMakeRange(start_index, contents_end_index - start_index);
			NSString* selected_string = [text_string substringWithRange:line_range];
			return iupStrReturnStr([selected_string UTF8String]);
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			// I originally tried to do the overkill thing and worry about injected newlines.
			// So I used the field editor and tried to share the textview code.
			// But unless the field is selected, there is no string in the field editor, so that approach won't work.
			return cocoaTextGetValueAttrib(ih);
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			// I originally tried to do the overkill thing and worry about injected newlines.
			// So I used the field editor and tried to share the textview code.
			// But unless the field is selected, there is no string in the field editor, so that approach won't work.
			return cocoaTextGetValueAttrib(ih);
			break;
		}
		default:
		{
			return NULL;
			break;
		}
	}
	
	
}

static int cocoaTextSetCueBannerAttrib(Ihandle *ih, const char *value)
{
	NSString* ns_string;
	
	if(NULL == value)
	{
		ns_string = @"";
	}
	else
	{
		ns_string = [NSString stringWithUTF8String:value];
	}
	
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			[text_field setPlaceholderString:ns_string];

			return  1;
			
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			[text_field setPlaceholderString:ns_string];
			break;
		}
		default:
		{
			break;
		}
	}
	

	return 0;
}




// NSControl's setEnabled didn't seem to do anything on NSTextField. Also, NSTextView isn't an NSControl.
static int cocoaTextSetActiveAttrib(Ihandle* ih, const char* value)
{
	BOOL is_active = (BOOL)iupStrBoolean(value);
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			[text_view setSelectable:is_active];

			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			[text_field setSelectable:is_active];
			// This doesn't seem to have any visible effect
			[text_field setEnabled:is_active];

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
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

static char* cocoaTextGetActiveAttrib(Ihandle* ih)
{
	int is_active = true;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			is_active = [text_view isSelectable];
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
//			is_active = [text_field isEditable];
			is_active = [text_field isSelectable];

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
//			is_active = [text_field isEditable];
			is_active = [text_field isSelectable];
			break;
		}
		default:
		{
			break;
		}
	}

	return iupStrReturnBoolean(is_active);
}

static int cocoaTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
	BOOL is_editable = !(BOOL)iupStrBoolean(value);
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			[text_view setEditable:is_editable];
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			[text_field setEditable:is_editable];
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			[text_field setEditable:is_editable];
			break;
		}
		default:
		{
			break;
		}
	}
	
	return 0;
}

static char* cocoaTextGetReadOnlyAttrib(Ihandle* ih)
{
	int is_editable = true;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			is_editable = [text_view isEditable];
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			is_editable = [text_field isEditable];
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			is_editable = [text_field isEditable];
			break;
		}
		default:
		{
			break;
		}
	}

	return iupStrReturnBoolean(!is_editable);
}

static int cocoaTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)  /* do not do the action before map */
  {
    	return 0;
  }
	
	if(value == NULL)
	{
		return 0;
	}
	
  ih->data->disable_callbacks = 1;
  if(ih->data->is_multiline)
  {
	  NSTextView* text_view = cocoaTextGetTextView(ih);

	  NSTextStorage* text_storage = [text_view textStorage];

	  
	  NSString* ns_append_string = nil;
		IupCocoaFont* iup_font = iupCocoaGetFont(ih);

		// NSTextStorage is a NSMutableAttributedString
//	  NSMutableString* mutable_string = [text_storage mutableString];
	  
	  if(ih->data->append_newline && ([text_storage length] > 0))
	  {
		  ns_append_string = [NSString stringWithFormat:@"\n%s", value];


	  }
	  else
	  {
		  ns_append_string = [NSString stringWithUTF8String:value];

	  }
	  
	  
	  // We need to mark the proposed change range (before we change it) in order to call shouldChangeTextInRange:
	  NSRange change_range = NSMakeRange([text_storage length], 0);
	  
	  
	  // Use the current set font as the baseline. We will modify a local copy of its attributes from there.
	  NSMutableDictionary* attribute_dict = [[iup_font attributeDictionary] mutableCopy];
	  [attribute_dict autorelease];
	  // Set the selection range to be at the very last character.
	  // Get the attributes (if any) for the selected range, and merge it into our copy of the font's attributes.
	  // This will overwrite/merge the current attributes into our font copy attributes.
	  // Needs to be 1 character or it raises an exception

		
	  // This throws an exception if:
	  // - We are the last character (we must use -1)
	  // - But then we have a problem if the view is empty
	  NSDictionary<NSAttributedStringKey, id>* text_storage_attributes = nil;
	  if(change_range.location == 0)
	  {
		  text_storage_attributes = [iup_font attributeDictionary];
	  }
	  else
	  {
		  text_storage_attributes = [text_storage attributesAtIndex:change_range.location-1 effectiveRange:NULL];
	  }
	  
	  [attribute_dict addEntriesFromDictionary:text_storage_attributes];

	  
	  NSAttributedString* attributed_append_string = [[NSAttributedString alloc] initWithString:ns_append_string attributes:attribute_dict];
	  [attributed_append_string autorelease];

	  NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
	  [undo_manager beginUndoGrouping];
	  
	  ih->data->disable_callbacks = 1;
  	  [text_view shouldChangeTextInRange:change_range replacementString:[attributed_append_string string]];
	  [text_storage beginEditing];

	  [text_storage appendAttributedString:attributed_append_string];

	  ih->data->disable_callbacks = 0;

	  [text_storage endEditing];

		// We must call both shouldChangeTextInRange and didChangeText to keep the undo manager consistent
	  [text_view didChangeText];
	  [undo_manager endUndoGrouping];



  }
  else
  {
	  
	  IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	  switch(sub_type)
	  {
		  case IUPCOCOATEXTSUBTYPE_FIELD:
		  {
			  NSTextField* text_field = cocoaTextGetTextField(ih);
			  IupCocoaFont* iup_font = iupCocoaGetFont(ih);
			  // I don't know if the old string is attributed or not, so I need to assume it is
			  NSMutableAttributedString* old_string_value = [[[text_field attributedStringValue] mutableCopy] autorelease];
			  
			  NSString* ns_append_string = [NSString stringWithUTF8String:value];
			  
			  
			  NSAttributedString* attributed_append_string = [[NSAttributedString alloc] initWithString:ns_append_string attributes:[iup_font attributeDictionary]];
			  [attributed_append_string autorelease];
			  
			  [old_string_value appendAttributedString:attributed_append_string];
			  
			  
			  
			  break;
		  }
		  default:
		  {
			  break;
		  }
	  }
  }
	
  ih->data->disable_callbacks = 0;
  return 0;
}


static int cocoaTextSetInsertAttrib(Ihandle* ih, const char* value)
{
	if(NULL == ih->handle)  /* do not do the action before map */
	{
		return 0;
	}
	if(NULL == value)
	{
		return 0;
	}

	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			NSTextStorage* text_storage = [text_view textStorage];

			NSRange insertion_point = [[[text_view selectedRanges] lastObject] rangeValue];
			if(NSNotFound == insertion_point.location)
			{
				// I guess we append if there is no cursor location?
				// A bit of a hack...cocoaTextSetAppendAttrib does extra stuff I don't want, so this will temporarily disable those features.
				int saved_newline = ih->data->append_newline;
				ih->data->append_newline = 0;
				int ret_val = cocoaTextSetAppendAttrib(ih, value);
				ih->data->append_newline = saved_newline;
				return ret_val;
			}
			else if([text_storage length] == insertion_point.location)
			{
				// We are appending
				// A bit of a hack...cocoaTextSetAppendAttrib does extra stuff I don't want, so this will temporarily disable those features.
				int saved_newline = ih->data->append_newline;
				ih->data->append_newline = 0;
				int ret_val = cocoaTextSetAppendAttrib(ih, value);
				ih->data->append_newline = saved_newline;
				return ret_val;
			}
			else if(insertion_point.length > 0)
			{
				// We do selected text
				return cocoaTextSetSelectedTextAttrib(ih, value);
			}
			
			// We can't use NSTextView insertText because
			// if this is used to programmatically alter text while the widget is disabled,
			// then the insert will fail.
			// So we must alter NSTextStorage directly.
			
			IupCocoaFont* iup_font = iupCocoaGetFont(ih);

			// Use the current set font as the baseline. We will modify a local copy of its attributes from there.
			NSMutableDictionary* attribute_dict = [[iup_font attributeDictionary] mutableCopy];
			[attribute_dict autorelease];
			// Set the selection range to be at the very last character.
			// Get the attributes (if any) for the selected range, and merge it into our copy of the font's attributes.
			// This will overwrite/merge the current attributes into our font copy attributes.
			// Needs to be 1 character or it raises an exception
			// We already know our location is not at the end and length==0
			NSDictionary<NSAttributedStringKey, id>* text_storage_attributes = [[text_storage attributedSubstringFromRange:NSMakeRange(insertion_point.location, 1)] attributesAtIndex:0 effectiveRange:NULL];
			[attribute_dict addEntriesFromDictionary:text_storage_attributes];
			
			NSString* ns_insert_string = [NSString stringWithUTF8String:value];
			
			NSAttributedString* attributed_insert_string = [[NSAttributedString alloc] initWithString:ns_insert_string attributes:attribute_dict];
			[attributed_insert_string autorelease];
			
			
			NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
			[undo_manager beginUndoGrouping];
			
			ih->data->disable_callbacks = 1;
			[text_view shouldChangeTextInRange:insertion_point replacementString:[attributed_insert_string string]];
			[text_storage beginEditing];
			
			[text_storage insertAttributedString:attributed_insert_string atIndex:insertion_point.location];
			
			ih->data->disable_callbacks = 0;
			
			[text_storage endEditing];
			
			// We must call both shouldChangeTextInRange and didChangeText to keep the undo manager consistent
			[text_view didChangeText];
			[undo_manager endUndoGrouping];
			
			

			return 0;
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			NSText* field_editor = [[text_field window] fieldEditor:YES forObject:text_field];
			NSRange selected_range = [field_editor selectedRange];
			if(NSNotFound == selected_range.location)
			{
				// I guess we append if there is no cursor location?
				return cocoaTextSetAppendAttrib(ih, value);
			}
			else if(selected_range.length > 0)
			{
				// We do selected text
				return cocoaTextSetSelectedTextAttrib(ih, value);
			}
			
			NSString* ns_string = [NSString stringWithUTF8String:value];
			NSCAssert([field_editor isKindOfClass:[NSTextView class]], @"Expected that the field editor is a NSTextView");
			[(NSTextView*)field_editor insertText:ns_string replacementRange:selected_range];
			return 0;
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
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

// UNDO, REDO only work for NSTextView
// New feature: CLEARUNDO
static int cocoaTextSetClipboardAttrib(Ihandle* ih, const char* value)
{
	ih->data->disable_callbacks = 1;
	
	NSView* the_view = nil;
	
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			the_view = text_view;
			
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			the_view = text_field;
			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			the_view = text_field;
			break;
		}
		default:
		{
			break;
		}
	}
	
	
	if(iupStrEqualNoCase(value, "COPY"))
	{
		[[NSApplication sharedApplication] sendAction:@selector(copy:) to:the_view from:nil];
		
	}
	else if(iupStrEqualNoCase(value, "CUT"))
	{
		[[NSApplication sharedApplication] sendAction:@selector(cut:) to:the_view from:nil];
		
	}
	else if(iupStrEqualNoCase(value, "PASTE"))
	{
		[[NSApplication sharedApplication] sendAction:@selector(paste:) to:the_view from:nil];
		
	}
	else if(iupStrEqualNoCase(value, "CLEAR"))
	{
		[[NSPasteboard generalPasteboard] clearContents];
	}
	else if(iupStrEqualNoCase(value, "UNDO"))
	{
		switch(sub_type)
		{
			case IUPCOCOATEXTSUBTYPE_VIEW:
			{
				NSTextView* text_view = (NSTextView*)the_view;
				NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
				[undo_manager undo];
				
				break;
			}
			case IUPCOCOATEXTSUBTYPE_FIELD:
			case IUPCOCOATEXTSUBTYPE_STEPPER:
			{
				// I haven't been able to make this work
//				[the_view becomeFirstResponder];
//				[[NSApplication sharedApplication] sendAction:@selector(undo:) to:nil from:nil];
				
				break;
			}
			default:
			{
				break;
			}
		}
	}
	else if(iupStrEqualNoCase(value, "REDO"))
	{
		switch(sub_type)
		{
			case IUPCOCOATEXTSUBTYPE_VIEW:
			{
				NSTextView* text_view = (NSTextView*)the_view;
				NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
				[undo_manager redo];
				
				break;
			}
			case IUPCOCOATEXTSUBTYPE_FIELD:
			case IUPCOCOATEXTSUBTYPE_STEPPER:
			{
				// I haven't been able to make this work
//				[the_view becomeFirstResponder];
//				[[NSApplication sharedApplication] sendAction:@selector(redo:) to:nil from:nil];
				
				break;
			}
			default:
			{
				break;
			}
		}
	}
	else if(iupStrEqualNoCase(value, "CLEARUNDO"))
	{
		switch(sub_type)
		{
			case IUPCOCOATEXTSUBTYPE_VIEW:
			{
				NSTextView* text_view = (NSTextView*)the_view;
				NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
				[undo_manager removeAllActions];
				
				break;
			}
			case IUPCOCOATEXTSUBTYPE_FIELD:
			case IUPCOCOATEXTSUBTYPE_STEPPER:
			{

				break;
			}
			default:
			{
				break;
			}
		}
	}
	
  ih->data->disable_callbacks = 0;
  return 0;
}



static char* cocoaTextGetCountAttrib(Ihandle* ih)
{
	NSString* text_string = nil;

	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			NSTextStorage* text_storage = [text_view textStorage];
			text_string = [text_storage string];
			

			
			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			NSTextField* text_field = cocoaTextGetTextField(ih);
			text_string = [text_field stringValue];

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			text_string = [text_field stringValue];
			break;
		}
		default:
		{
			break;
		}
	}
	
	if((nil == text_string) || ([text_string length] == 0))
	{
		return iupStrReturnInt(0);
	}
	
	// GOTCHA: Modern characters may be multiple bytes (e.g. emoji characters).
	// Because of this, [string length] isn't correct, because it tells us the number of bytes, not characters.
	// The correct thing to do is to iterate through the string and count the glyphs.
	// But it probably is more expensive than what people think when they call this routine.
	// See https://www.objc.io/issues/9-strings/unicode/
	// NSString *s = @"The weather on \U0001F30D is \U0001F31E today.";
	// The weather on 🌍 is 🌞 today.

//	NSLog(@"length: %zu\n%@", [text_string length], text_string);

	NSUInteger glyph_count = cocoaTextCountGlyphsInString(text_string);

	// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
	return iupStrReturnInt((int)glyph_count);
}

// This includes wrapped lines in the count.
// https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/TextLayout/Tasks/CountLines.html
static char* cocoaTextGetLineCountAttrib(Ihandle* ih)
{
	if(!ih->data->is_multiline)
	{
		return "1";
	}
	
	NSTextView* text_view = cocoaTextGetTextView(ih);
	
	NSLayoutManager* layout_manager = [text_view layoutManager];
	NSUInteger number_of_lines;
	NSUInteger index;
	
	NSUInteger number_of_glyphs = [layout_manager numberOfGlyphs];
	NSRange line_range = NSMakeRange(0, 0);
	
	for(number_of_lines = 0, index = 0; index < number_of_glyphs;)
	{
		(void) [layout_manager lineFragmentRectForGlyphAtIndex:index effectiveRange:&line_range];
		index = NSMaxRange(line_range);
	}
	
	// FIXME: Iup is artificially constraining us to 32-bit by not supporting 64-bit variants.
	return iupStrReturnInt((int)number_of_lines);
}


/************************************** Begin SPIN *****************************************************/

static int cocoaTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			// Neither the field nor the cell work. I think I must change the field editor.
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			
			int int_value = 0;
			if(iupStrToInt(value, &int_value))
			{
				ih->data->disable_callbacks = 1;

				NSStepper* stepper_view = cocoaTextGetStepperView(ih);
				double max_value = [stepper_view maxValue];
				if((double)int_value > max_value)
				{
					int_value = (int)max_value;
				}
				double min_value = [stepper_view minValue];
				if((double)int_value < min_value)
				{
					int_value = (int)min_value;
				}
				
				if(iupAttribGetBoolean(ih, "SPINAUTO"))
				{
					// With Cocoa Bindings, we must set the model or controller, not the view.
					// Otherwise, if we set the view directly,
					// when we click on the stepper, it will have a stale version of the world
					// and increment from the stale value instead of the value seen in the field.
					NSNumber* number_to_set = [NSNumber numberWithInt:int_value];
					IUPStepperObject* stepper_object = cocoaTextGetStepperObject(ih);
					[stepper_object setStepperValue:number_to_set];
				}
				else
				{
					NSString* ns_string = [NSString stringWithFormat:@"%d", int_value];
					[text_field setStringValue:ns_string];
				}

				ih->data->disable_callbacks = 0;

			}
			
			break;
		}
		default:
		{
			break;
		}
	}
	
	return 1;
}



static char* cocoaTextGetSpinValueAttrib(Ihandle* ih)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			// Neither the field nor the cell work. I think I must change the field editor.
			NSTextField* text_field = cocoaTextGetStepperTextField(ih);
			NSString* ns_string = [text_field stringValue];
			return iupStrReturnStr([ns_string UTF8String]);
			
			break;
		}
		default:
		{
			break;
		}
	}
	
	return NULL;
}


static int cocoaTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			int new_min;
			if(iupStrToInt(value, &new_min))
			{
				ih->data->disable_callbacks = 1;

				// TODO: What if min > max?
				// TODO: What if current value is outside new range?
				NSStepper* stepper_view = cocoaTextGetStepperView(ih);
				[stepper_view setMinValue:(double)new_min];

				ih->data->disable_callbacks = 0;
			}
			
			break;
		}
		default:
		{
			break;
		}
	}

	return 1;
}

static int cocoaTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			int new_max;
			if(iupStrToInt(value, &new_max))
			{
				ih->data->disable_callbacks = 1;
				
				// TODO: What if min > max?
				// TODO: What if current value is outside new range?
				NSStepper* stepper_view = cocoaTextGetStepperView(ih);
				[stepper_view setMaxValue:(double)new_max];

				ih->data->disable_callbacks = 0;
			}
			
			break;
		}
		default:
		{
			break;
		}
	}

	return 1;
}

static int cocoaTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			int new_inc;
			if(iupStrToInt(value, &new_inc))
			{
				ih->data->disable_callbacks = 1;
				
				NSStepper* stepper_view = cocoaTextGetStepperView(ih);
				[stepper_view setIncrement:(double)new_inc];

				ih->data->disable_callbacks = 0;
			}
			
			break;
		}
		default:
		{
			break;
		}
	}

	return 1;
}



/************************************** End SPIN *****************************************************/




// Need to override because the position offsets are wrong otherwise.
static void cocoaTextLayoutUpdateMethod(Ihandle* ih)
{
	// This may be called before we have a native parent
	InativeHandle* parent_native_handle_direct = iupChildTreeGetNativeParentHandle(ih);
	NSView* parent_view = nil;
	
	// This may be called before we have a native parent, in which case it returns -1
	if((void*)-1 == parent_native_handle_direct)
	{
		return;
	}
	
	NSObject* parent_native_handle = (NSObject*)parent_native_handle_direct;
	

	
	if([parent_native_handle isKindOfClass:[NSWindow class]])
	{
		NSWindow* parent_window = (NSWindow*)parent_native_handle;
		parent_view = [parent_window contentView];
	}
	/* // Part of NSBox experiment
	 else if([parent_native_handle isKindOfClass:[NSBox class]])
	 {
		NSBox* box_view = (NSBox*)parent_native_handle;
		parent_view = [box_view contentView];
		
		CGFloat diff_width = NSWidth([box_view frame]) - NSWidth([parent_view frame]);
		CGFloat diff_height = NSHeight([box_view frame]) - NSHeight([parent_view frame]);
	 
		current_width = current_width - diff_width;
		current_height = current_height - diff_height;
	 }
	 */
	else if([parent_native_handle isKindOfClass:[NSView class]])
	{
		parent_view = (NSView*)parent_native_handle;
	}
	else
	{
		NSCAssert(1, @"Unexpected type for parent widget");
		@throw @"Unexpected type for parent widget";
	}
	
	NSRect parent_rect = [parent_view frame];

	NSRect the_rect;
	id child_handle = ih->handle;
	NSView* the_view = nil;
	if(!ih->data->is_multiline)
	{
//		the_view = (NSView*)child_handle;
		
//		NSTextField* text_field = (NSTextField*)child_handle;


		CGFloat current_width = (CGFloat)ih->currentwidth;
		CGFloat current_height = (CGFloat)ih->currentheight;
		
		NSLog(@"old ih->currentheight=%d", ih->currentheight);
		CGFloat pos_x = (CGFloat)ih->x;
		// Need to invert y-axis, and also need to shift/account for height of widget because that is also lower-left instead of upper-left.
//		CGFloat pos_y = parent_rect.size.height - (CGFloat)ih->y - (CGFloat)ih->currentheight;
		
		// NSTextField is all guess work about how to scale for any font size.
		// Throw away ih->currentheight because it will EXPAND it.
		// But for the standard font, we get fontheight=16 and we want the height to be 22
		int font_height = 0;
		iupdrvFontGetCharSize(ih, NULL, &font_height);
		const CGFloat HEIGHT_PADDING = 6.0;
		const CGFloat WIDTH_PADDING = 12.0;

		current_height = font_height + HEIGHT_PADDING;

		current_width = current_width - WIDTH_PADDING;

		// Need to invert y-axis, and also need to shift/account for height of widget because that is also lower-left instead of upper-left.
		CGFloat pos_y = parent_rect.size.height - (CGFloat)ih->y - current_height;

		
		pos_x = pos_x + WIDTH_PADDING*0.5;
//		pos_y = pos_y - HEIGHT_PADDING*0.5; // I don't realy understand the logic of this offset, particularly the -1
//		pos_y = pos_y - HEIGHT_PADDING*0.5 - 1; // I don't realy understand the logic of this offset, particularly the -1
//		pos_y = pos_y + HEIGHT_PADDING*0.5 - 1; // I don't realy understand the logic of this offset, particularly the -1
		
		the_rect = NSMakeRect(
									 pos_x,
									 // Need to invert y-axis, and also need to shift/account for height of widget because that is also lower-left instead of upper-left.
									 pos_y,
									 current_width,
									 current_height
									 );
		
//		ih->currentheight = current_height;
//		ih->currentwidth = current_width;

//		ih->x = pos_x;
//		ih->y = pos_y;
		
		// for padding
		// drat, data is private and requires a per-widget header
		{
			
			char* padding_str = iupAttribGet(ih, "PADDING");
			if((NULL != padding_str) && (padding_str[0] != '\0'))
			{
				int horiz_padding = 0;
				int vert_padding = 0;
				iupStrToIntInt(padding_str, &horiz_padding, &vert_padding, 'x');
				
				NSRect old_frame = the_rect;
				NSRect new_frame;
				new_frame.origin.x = old_frame.origin.x + (CGFloat)horiz_padding*0.5;
				new_frame.origin.y = old_frame.origin.y + (CGFloat)vert_padding*0.5;
				new_frame.size.width = old_frame.size.width - (CGFloat)horiz_padding;
				new_frame.size.height = old_frame.size.height - (CGFloat)vert_padding;
				
				the_rect = new_frame;
			}
			
		}
		
	}
	else
	{
//		NSTextView* text_view = (NSTextView*)child_handle;
		
		
		the_rect = NSMakeRect(
									 ih->x,
									 // Need to invert y-axis, and also need to shift/account for height of widget because that is also lower-left instead of upper-left.
									 parent_rect.size.height - ih->y - ih->currentheight,
									 ih->currentwidth,
									 ih->currentheight
									 );
		
		// for padding
		// drat, data is private and requires a per-widget header
		{
			
			char* padding_str = iupAttribGet(ih, "PADDING");
			if((NULL != padding_str) && (padding_str[0] != '\0'))
			{
				int horiz_padding = 0;
				int vert_padding = 0;
				iupStrToIntInt(padding_str, &horiz_padding, &vert_padding, 'x');
				
				NSRect old_frame = the_rect;
				NSRect new_frame;
				new_frame.origin.x = old_frame.origin.x + (CGFloat)horiz_padding*0.5;
				new_frame.origin.y = old_frame.origin.y + (CGFloat)vert_padding*0.5;
				new_frame.size.width = old_frame.size.width - (CGFloat)horiz_padding;
				new_frame.size.height = old_frame.size.height - (CGFloat)vert_padding;
				
				the_rect = new_frame;
			}
			
		}

	}

	
	
	//	iupgtkNativeContainerMove((GtkWidget*)parent, widget, x, y);
	
	//	iupgtkSetPosSize(GTK_CONTAINER(parent), widget, ih->x, ih->y, ih->currentwidth, ih->currentheight);
	
	/*
	 CGSize fitting_size = [the_view fittingSize];
	 ih->currentwidth = fitting_size.width;
	 ih->currentheight = fitting_size.height;
	 */
	
	
#if 0 // experiment to try to handle NSBox directly instead of using cocoaFrameGetInnerNativeContainerHandleMethod. I think cocoaFrameGetInnerNativeContainerHandleMethod is better, but
#else
	

	

	
	
	[the_view setFrame:the_rect];
#endif
	
}


static int cocoaTextSetContextMenuAttrib(Ihandle* ih, const char* value)
{
	Ihandle* menu_ih = (Ihandle*)value;
	
	IupCocoaTextSubType sub_type = cocoaTextGetSubType(ih);
	switch(sub_type)
	{
		case IUPCOCOATEXTSUBTYPE_VIEW:
		{
			NSTextView* text_view = cocoaTextGetTextView(ih);
			
			iupCocoaCommonBaseSetContextMenuForWidget(ih, text_view, menu_ih);


			break;
		}
		case IUPCOCOATEXTSUBTYPE_FIELD:
		{
			// Neither the field nor the cell work. I think I must change the field editor.
//			NSTextField* text_field = cocoaTextGetTextField(ih);

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
			// Save the menu_ih so we can access it in the callback
			iupAttribSet(ih, "_COCOA_CONTEXT_MENU_IH", (const char*)menu_ih);

			break;
		}
		case IUPCOCOATEXTSUBTYPE_STEPPER:
		{
			// Neither the field nor the cell work. I think I must change the field editor.
//			NSTextField* text_field = cocoaTextGetStepperTextField(ih);

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
			// Save the menu_ih so we can access it in the callback
			iupAttribSet(ih, "_COCOA_CONTEXT_MENU_IH", (const char*)menu_ih);
			break;
		}
		default:
		{
			break;
		}
	}
	
	
	
	
	return 1;
}


static int cocoaTextMapMethod(Ihandle* ih)
{
	NSView* root_view = nil;
	NSView* main_view = nil;

	
	if (ih->data->is_multiline)
	{
		// We need to put the textview in a scrollview
		// https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/TextUILayer/Tasks/TextInScrollView.html

		NSScrollView* scroll_view = [[NSScrollView alloc] initWithFrame:NSZeroRect];
		NSSize scrollview_content_size = [scroll_view contentSize];
		
		[scroll_view setBorderType:NSNoBorder];
		[scroll_view setHasVerticalScroller:YES];
		[scroll_view setHasHorizontalScroller:NO];
//		[scroll_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
		
		
		NSTextView* text_view = [[NSTextView alloc] initWithFrame:NSZeroRect];
//		NSTextView* text_view = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 400, 400)];

		
		text_view = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, scrollview_content_size.width, scrollview_content_size.height)];

		[text_view setMinSize:NSMakeSize(0.0, 0.0)];
		[text_view setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
		[text_view setVerticallyResizable:YES];
		[text_view setHorizontallyResizable:NO];
		// This NSViewWidthSizable I still need despite disabling the others. Otherwise using SHRINK with horizontal scrollbars continues to grow and never shrink.
		[text_view setAutoresizingMask:NSViewWidthSizable];
//		[[text_view textContainer] setContainerSize:NSMakeSize(scrollview_content_size.width, FLT_MAX)];
//		[[text_view textContainer] setWidthTracksTextView:YES];
		// Needed to allow things like Cmd-E (put in search buffer), Cmd-G (find next), and the standard Find panel. Even if you don't want the standard find panel, a broken cmd-e/cmd-g is bad.
		[text_view setUsesFindPanel:YES];
		[text_view setAllowsUndo:YES];

		[scroll_view setDocumentView:text_view];
		[text_view release];


		
		
		root_view = scroll_view;
		main_view = text_view;

		

		int wordwrap = 0;


		// Make sure the text view is synchronized with the variable.
		[text_view setRichText:ih->data->has_formatting];
//		[text_view setImportsGraphics:ih->data->has_formatting];
		[text_view setImportsGraphics:NO];

		if (iupAttribGetBoolean(ih, "WORDWRAP"))
		{
			wordwrap = 1;
			ih->data->sb &= ~IUP_SB_HORIZ;  /* must remove the horizontal scroolbar */
			
			
//			[[text_view enclosingScrollView] setHasHorizontalScroller:YES];
			[[text_view enclosingScrollView] setHasHorizontalScroller:NO];
			[text_view setHorizontallyResizable:YES];
//			[text_view setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
			[[text_view textContainer] setContainerSize:NSMakeSize(FLT_MAX, FLT_MAX)];
			[[text_view textContainer] setWidthTracksTextView:YES];

		}
		else
		{
			/*
			NSSize layout_size = [text_view maxSize];
			layout_size.width = layout_size.height;
			[text_view setMaxSize:layout_size];
			[[text_view textContainer] setWidthTracksTextView:NO];
			[[text_view textContainer] setContainerSize:layout_size];
			*/
			
			[[text_view enclosingScrollView] setHasHorizontalScroller:YES];
			[text_view setHorizontallyResizable:NO];
//			[text_view setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
//			[[text_view textContainer] setContainerSize:NSMakeSize(FLT_MAX, FLT_MAX)];
//			[[text_view textContainer] setWidthTracksTextView:YES];
			
		}
		
		
		IupCocoaTextViewDelegate* text_view_delegate = [[IupCocoaTextViewDelegate alloc] init];
		[text_view setDelegate:text_view_delegate];
		[text_view_delegate setIhandle:ih];
		objc_setAssociatedObject(text_view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
		// putting on the text_field just for storage. This is to make it consistent with the regular NSTextField (non-spinner) case.
		objc_setAssociatedObject(text_view, IUP_COCOA_TEXT_DELEGATE_OBJ_KEY, (id)text_view_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
		[text_view_delegate release];
		
		


		
	}
	else if(iupAttribGetBoolean(ih, "SPIN"))
	{
		// TODO: NSStepper

#if 0
//		NSView* container_view = [[NSView alloc] initWithFrame:NSZeroRect];
//		NSView* container_view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 200, 27)];
		NSStackView* container_view = [[NSStackView alloc] initWithFrame:NSZeroRect];
		[container_view setSpacing:4.0];
		
		NSRect stepper_rect = NSMakeRect(0, 0, 19, 27);
		NSStepper* stepper_view = [[NSStepper alloc] initWithFrame:stepper_rect];
		IupCocoaTextField* text_field = [[IupCocoaTextField alloc] initWithFrame:NSZeroRect];
//		NSTextField* text_field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200-19, 22)];

		
//		[container_view addSubview:text_field];
//		[text_field addSubview:stepper_view];

		[container_view addView:text_field inGravity:NSStackViewGravityLeading];
		[container_view addView:stepper_view inGravity:NSStackViewGravityTrailing];
		[stepper_view release];
		[text_field release];
		
//		[container_view setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
//		[text_field setAutoresizingMask:(NSViewWidthSizable)];
		
		root_view = container_view;
		main_view = text_field;

#else

		// The NIB file has setup the NSStackView and also the Cocoa Bindings to connect the textfield and stepper.
		
		NSBundle* framework_bundle = [NSBundle bundleWithIdentifier:@"br.puc-rio.tecgraf.iup"];
		NSNib* text_spinner_nib = nil;
		if(!iupAttribGetBoolean(ih, "SPINAUTO"))
		{
			text_spinner_nib = [[NSNib alloc] initWithNibNamed:@"IupTextSpinnerNoBindings" bundle:framework_bundle];
		}
		else
		{
			text_spinner_nib = [[NSNib alloc] initWithNibNamed:@"IupTextSpinner" bundle:framework_bundle];
		}
		


		NSArray* top_level_objects = nil;
		IUPTextSpinnerFilesOwner* files_owner = [[IUPTextSpinnerFilesOwner alloc] init];
		IUPTextSpinnerContainer* text_spinner_container = [[IUPTextSpinnerContainer alloc] init];

		// We can either fish out the objects we want (and retain them) in the top_level_objects,
		// or for my convenience, I made a File's Owner object and wired up all the major elements so I can retrive them easily.
		// Note: top_level_objects must be explicitly retained if I want to keep it around. (I don't need to keep it around so I don't retain it.))
		[text_spinner_nib instantiateWithOwner:files_owner topLevelObjects:&top_level_objects];
		
		/*
		Messy: There are multiple objects including an NSObject and NSObjectController in the NIB.
		I originally thought about making a NSObject subclass which holds all the components and make it the ih->handle.
		But a non-NSView as the ih->handle breaks an implicit assumption used by the common core.
		So instead, we'll make the NSStackView the root container object for the ih->handle.
		But we will still make a NSObject subclass that holds the remaining components.
		We will use setAssociatedObject to keep these objects connected, while avoiding the need to subclass NSStackView just to hold this property.
		*/
		NSStackView* stack_view = [files_owner stackView];
		NSStepper* stepper_view = [files_owner stepperView];
		NSTextField* text_field = [files_owner textField];
		IUPStepperObject* stepper_object = [files_owner stepperObject];
		IUPStepperObjectController* stepper_object_controller = [files_owner stepperObjectController];

		[text_spinner_container setStepperView:stepper_view];
		[text_spinner_container setTextField:text_field];
		[text_spinner_container setStepperObject:stepper_object];
		[text_spinner_container setStepperObjectController:stepper_object_controller];
		
		// can't turn off decimals in IB, so do it here.
		{
			NSNumberFormatter* number_formatter = [text_field formatter];
			[number_formatter setMaximumFractionDigits:0];
		}

		// We're going to use OBJC_ASSOCIATION_RETAIN because I do believe it will do the right thing for us.
		objc_setAssociatedObject(stack_view, IUP_COCOA_TEXT_SPINNERCONTAINER_OBJ_KEY, (id)text_spinner_container, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

		
		root_view = [stack_view retain];
		// This is one of those cases where main_view doesn't fully make sense. nil might be just as appropriate since we will need to manually handle this case any way.
		main_view = text_field;

		[text_spinner_container release];
		[files_owner release];
		// do not release top_level_objects because instantiateWithOwner did not retain (is is essentially autoreleased);
		[text_spinner_nib release];
		
		
		// Attach a number formatter since this is always a number ("INTEGER").
		// Also supports "NC"
		IupNumberFormatter* number_formatter = [[IupNumberFormatter alloc] init];
		[number_formatter autorelease];
		[number_formatter setIhandle:ih];
//		[number_formatter setFormattingContext:NSFormattingContextDynamic];
		[number_formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
		[number_formatter setPartialStringValidationEnabled:YES];
		[number_formatter setNumberStyle:NSNumberFormatterNoStyle];
		[text_field setFormatter:number_formatter];
		
		
		
		
		[stepper_view setValueWraps:(BOOL)iupAttribGetBoolean(ih, "SPINWRAP")];
		
		
		/*
		// I'm using objc_setAssociatedObject/objc_getAssociatedObject because it allows me to avoid making subclasses just to hold ivars.
		objc_setAssociatedObject(the_toggle, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
		// I also need to track the memory of the buttion action receiver.
		// I prefer to keep the Ihandle the actual NSView instead of the receiver because it makes the rest of the implementation easier if the handle is always an NSView (or very small set of things, e.g. NSWindow, NSView, CALayer).
		// So with only one pointer to deal with, this means we need our Toggle to hold a reference to the receiver object.
		// This is generally not good Cocoa as Toggles don't retain their receivers, but this seems like the best option.
		// Be careful of retain cycles.
		IupCocoaToggleReceiver* toggle_receiver = [[IupCocoaToggleReceiver alloc] init];
		[the_toggle setTarget:toggle_receiver];
		[the_toggle setAction:@selector(myToggleClickAction:)];
		// I *think* is we use RETAIN, the object will be released automatically when the Toggle is freed.
		// However, the fact that this is tricky and I had to look up the rules (not to mention worrying about retain cycles)
		// makes me think I should just explicitly manage the memory so everybody is aware of what's going on.
		objc_setAssociatedObject(the_toggle, IUP_COCOA_TOGGLE_RECEIVER_OBJ_KEY, (id)toggle_receiver, OBJC_ASSOCIATION_ASSIGN);
*/
		IupCocoaTextSpinnerDelegate* text_spinner_delegate = [[IupCocoaTextSpinnerDelegate alloc] init];
//		[stepper_view setDelegate:text_spinner_delegate];
		[text_field setDelegate:text_spinner_delegate];
		[text_spinner_delegate setIhandle:ih];

		[stepper_view setTarget:text_spinner_delegate];
		[stepper_view setAction:@selector(mySpinnerClickAction:)];
		

		objc_setAssociatedObject(stepper_view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
		// putting on the text_field just for storage. This is to make it consistent with the regular NSTextField (non-spinner) case.
		objc_setAssociatedObject(text_field, IUP_COCOA_TEXT_DELEGATE_OBJ_KEY, (id)text_spinner_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
		[text_spinner_delegate release];
		

#endif
		

		
		

		/*
			gtk_spin_button_set_numeric((GtkSpinButton*)ih->handle, FALSE);
			gtk_spin_button_set_digits((GtkSpinButton*)ih->handle, 0);
			
			gtk_spin_button_set_wrap((GtkSpinButton*)ih->handle, iupAttribGetBoolean(ih, "SPINWRAP"));
			
			g_signal_connect(G_OBJECT(ih->handle), "value-changed", G_CALLBACK(gtkTextSpinValueChanged), ih);
			g_signal_connect(G_OBJECT(ih->handle), "output", G_CALLBACK(gtkTextSpinOutput), ih);
			
			if (!iupAttribGetBoolean(ih, "SPINAUTO"))
			{
		 g_signal_connect(G_OBJECT(ih->handle), "input", G_CALLBACK(gtkTextSpinInput), ih);
		 iupAttribSet(ih, "_IUPGTK_SPIN_NOAUTO", "1");
			}
		 */
		/* formatting is never supported when MULTILINE=NO */
		ih->data->has_formatting = 0;
		
		// We must not allow IUP to EXPAND the height of the NSTextField so unset the bit flag if it is set.
		ih->expand = ih->expand & ~IUP_EXPAND_HEIGHT;
		
	}
	else
	{
		NSTextField* text_field;

		// IMPORTANT: Secure text fields are not togglable in Cocoa
		// It might be fakeable, however, since this is security related, mucking with it is ill-advised.
		// Also Mac App Store may reject ill-advised things.
		if(iupAttribGetBoolean(ih, "PASSWORD"))
		{

			text_field = [[IupCocoaSecureTextField alloc] initWithFrame:NSZeroRect];
			//text_field = [[NSSecureTextField alloc] initWithFrame:NSMakeRect(0, 0, 140, 40)];
			[text_field setFont:[NSFont systemFontOfSize:0.0]];

		}
		else
		{
			
			//text_field = [[NSTextField alloc] initWithFrame:NSZeroRect];
			text_field = [[IupCocoaTextField alloc] initWithFrame:NSMakeRect(50, 50, 140, 40)];
			[text_field setFont:[NSFont systemFontOfSize:0.0]];

			
		}
		
		IupCocoaTextFieldDelegate* text_field_delegate = [[IupCocoaTextFieldDelegate alloc] init];
		[text_field setDelegate:text_field_delegate];
		objc_setAssociatedObject(text_field, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(text_field, IUP_COCOA_TEXT_DELEGATE_OBJ_KEY, (id)text_field_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
		[text_field_delegate release];

		
		// We must not allow IUP to EXPAND the height of the NSTextField so unset the bit flag if it is set.
		ih->expand = ih->expand & ~IUP_EXPAND_HEIGHT;
		
		/* formatting is never supported when MULTILINE=NO */
		ih->data->has_formatting = 0;
		
		
#ifdef USE_NSSTACKVIEW_TEXTFIELD_CONTAINER
		NSStackView* stack_view = [[NSStackView alloc] initWithFrame:NSZeroRect];
		[stack_view setOrientation:NSUserInterfaceLayoutOrientationVertical];
		[stack_view setAlignment:NSLayoutAttributeCenterX];
		[stack_view setDistribution:NSStackViewDistributionEqualCentering]; // requires 10.11. In a complicated IB test, I needed this to keep the widget vertically centerd. But in practice, I'm not sure if this is needed. Maybe we can use respondsToSelector if 10.9/10.10 compat is needed.
		[stack_view addView:text_field inGravity:NSStackViewGravityCenter];
		[text_field release];
		root_view = stack_view;
		main_view = text_field;
#elif defined(USE_CONTAINERVIEW_TEXTFIELD_CONTAINER)
		NSView* container_view = [[NSView alloc] initWithFrame:NSZeroRect];
		// Warning: This gets clobbered by AddParent
		[container_view setAutoresizingMask:NSViewMinXMargin|NSViewMaxXMargin|NSViewMinYMargin|NSViewMaxYMargin|NSViewWidthSizable|NSViewHeightSizable];
		[text_field setAutoresizingMask:NSViewMinXMargin|NSViewMaxXMargin|NSViewMinYMargin|NSViewMaxYMargin|NSViewWidthSizable];

		[container_view addSubview:text_field];
		[text_field release];
		root_view = container_view;
		main_view = text_field;
#else
		root_view = text_field;
		main_view = text_field;

#endif
		

	}
	
	
	
	
	
	ih->handle = root_view;
	iupCocoaSetAssociatedViews(ih, main_view, root_view);


	// All Cocoa views shoud call this to add the new view to the parent view.
	iupCocoaAddToParent(ih);
	

#if 0
	/* configure for DRAG&DROP */
	if (IupGetCallback(ih, "DROPFILES_CB"))
		iupAttribSet(ih, "DROPFILESTARGET", "YES");
	
	/* update a mnemonic in a label if necessary */
	iupgtkUpdateMnemonic(ih);
#endif

	if(ih->data->formattags)
	{
		iupTextUpdateFormatTags(ih);
	}
	
	if(ih->data->is_multiline)
	{
		// Reset the undo manager so the user can't undo the initial setup.
		NSTextView* text_view = cocoaTextGetTextView(ih);
		NSUndoManager* undo_manager = [[text_view delegate] undoManagerForTextView:text_view];
		[undo_manager removeAllActions];
	}
	
	return IUP_NOERROR;
}


static void cocoaTextUnMapMethod(Ihandle* ih)
{
	id the_view = ih->handle;
	/*
	id text_receiver = objc_getAssociatedObject(the_view, IUP_COCOA_TOGGLE_RECEIVER_OBJ_KEY);
	objc_setAssociatedObject(the_view, IUP_COCOA_TOGGLE_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	[text_receiver release];
	*/
	
	// Destroy the context menu ih it exists
	{
		Ihandle* context_menu_ih = (Ihandle*)iupCocoaCommonBaseGetContextMenuAttrib(ih);
		if(NULL != context_menu_ih)
		{
			IupDestroy(context_menu_ih);
		}
		iupCocoaCommonBaseSetContextMenuAttrib(ih, NULL);
	}
	
	// Because we used retain for the delegates, they should automatically release
	
	iupCocoaRemoveFromParent(ih);
	iupCocoaSetAssociatedViews(ih, nil, nil);
	[the_view release];
	ih->handle = NULL;
}


// We need to override iTextComputeNaturalSizeMethod because the metrics only compute the raw string size and not the textbox area around it.
// On Mac, using the standard computation, we only see 3 'WWWW' instead of 5, presumably because the widget is including whitespace area for the surrounding box.
static void cocoaTextComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
	int natural_w = 0,
	natural_h = 0,
	visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS"),
	visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
	(void)children_expand; /* unset if not a container */
	int single_char_width;
	
	/* Since the contents can be changed by the user, the size can not be dependent on it. */
	iupdrvFontGetCharSize(ih, NULL, &natural_h);  /* one line height */
	natural_w = iupdrvFontGetStringWidth(ih, "WWWWWWWWWW");

	// Cocoa adjustment here: Let's make the width slightly bigger.
	// Since we see 3 out of 5 WWW's, let's pad with two more character widths.
	single_char_width = natural_w / 10;

	
	natural_w = (visiblecolumns*natural_w)/10;
	
	
	// Cocoa adjustment here
	natural_w += (single_char_width * 2);
	
	
	if (ih->data->is_multiline)
	{
		natural_h = visiblelines*natural_h;
	}
	else
	{
		natural_h = 22;
	}
	
	/* compute the borders space */
	if (iupAttribGetBoolean(ih, "BORDER"))
		iupdrvTextAddBorders(ih, &natural_w, &natural_h);
	
	if (iupAttribGetBoolean(ih, "SPIN"))
		iupdrvTextAddSpin(ih, &natural_w, natural_h);
	
	natural_w += 2*ih->data->horiz_padding;
	natural_h += 2*ih->data->vert_padding;
	
	/* add scrollbar */
	if (ih->data->is_multiline && ih->data->sb)
	{
		int sb_size = iupdrvGetScrollbarSize();
		if (ih->data->sb & IUP_SB_HORIZ)
			natural_h += sb_size;  /* sb horizontal affects vertical size */
		if (ih->data->sb & IUP_SB_VERT)
			natural_w += sb_size;  /* sb vertical affects horizontal size */
	}
	
	*w = natural_w;
	*h = natural_h;
	
	Ihandle* child;

	for (child = ih->firstchild; child; child = child->brother)
	{

		
		/* update child natural size first */
		if (!(child->flags & IUP_FLOATING_IGNORE))
			iupBaseComputeNaturalSize(child);
	}
	
}


void iupdrvTextInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = cocoaTextMapMethod;
	ic->UnMap = cocoaTextUnMapMethod;

//	ic->LayoutUpdate = cocoaTextLayoutUpdateMethod;
	ic->ComputeNaturalSize = cocoaTextComputeNaturalSizeMethod;


  /* Driver Dependent Attribute functions */

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTextSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);

  // need to override active behavior for text
  iupClassRegisterAttribute(ic, "ACTIVE", cocoaTextGetActiveAttrib, cocoaTextSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

#if 0
  /* Special */

  /* IupText only */
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, gtkTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
#endif
  iupClassRegisterAttribute(ic, "VALUE", cocoaTextGetValueAttrib, cocoaTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LINEVALUE", cocoaTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SELECTEDTEXT", cocoaTextGetSelectedTextAttrib, cocoaTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", cocoaTextGetSelectionAttrib, cocoaTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", cocoaTextGetSelectionPosAttrib, cocoaTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", cocoaTextGetCaretAttrib, cocoaTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", cocoaTextGetCaretPosAttrib, cocoaTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
	
  iupClassRegisterAttribute(ic, "INSERT", NULL, cocoaTextSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, cocoaTextSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "READONLY", cocoaTextGetReadOnlyAttrib, cocoaTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, cocoaTextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, cocoaTextSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, cocoaTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, cocoaTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SPINMIN", NULL, cocoaTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, cocoaTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, cocoaTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", cocoaTextGetSpinValueAttrib, cocoaTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "COUNT", cocoaTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LINECOUNT", cocoaTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* IupText Windows and GTK only */
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", cocoaTextGetFormattingAttrib, cocoaTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, cocoaTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, cocoaTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
#if 0
  iupClassRegisterAttribute(ic, "OVERWRITE", gtkTextGetOverwriteAttrib, gtkTextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
// FIXME:
//  iupClassRegisterAttribute(ic, "TABSIZE", NULL, cocoaTextSetTabSizeAttrib, "8", NULL, IUPAF_DEFAULT);  /* force new default value */
#endif
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, cocoaTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);


  iupClassRegisterAttribute(ic, "FILTER", NULL, cocoaTextSetFilterAttrib, NULL, NULL, IUPAF_NO_INHERIT);

	/* New API for view specific contextual menus (Mac only) */
	iupClassRegisterAttribute(ic, "CONTEXTMENU", iupCocoaCommonBaseGetContextMenuAttrib, cocoaTextSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	// TODO: LAYERBACKED
	// TODO: ALLOWSUNDO
	// TODO: CLEARUNDO/REMOVEALLACTIONS
}
