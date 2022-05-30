/** \file
 * \brief Calendar Control
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>
#include <limits.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"

#include "iupcocoa_drv.h"


static const void* IUP_COCOA_CALENDAR_DELEGATE_OBJ_KEY = "IUP_COCOA_CALENDAR_DELEGATE_OBJ_KEY";


@interface IupCocoaCalendarDelegate : NSObject
@end

@implementation IupCocoaCalendarDelegate
//	The user interacted with the date picker control so update the date/time examples.
- (IBAction) myDatePickerAction:(id)the_sender
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);
	iupBaseCallValueChangedCb(ih);
}
@end



static int cocoaCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
	NSDatePicker* date_picker = (NSDatePicker*)ih->handle;

	if(iupStrEqualNoCase(value, "TODAY"))
	{
		[date_picker setDateValue:[NSDate date]];
	}
	else
	{
		NSDateFormatter* date_formatter = [[NSDateFormatter alloc] init];
		[date_formatter setDateFormat:@"YYYY/MM/dd"];
		if(value && (value[0] != '\0'))
		{
			NSString* date_string = [NSString stringWithUTF8String:value];
			NSDate* parsed_date = [date_formatter dateFromString:date_string];
			[date_picker setDateValue:parsed_date];
		}
	}
	return 0; /* do not store value in hash table */
}

static char* cocoaCalendarGetIupStrFromDate(NSDate* the_date)
{
	NSDateFormatter* date_formatter = [[NSDateFormatter alloc] init];
	[date_formatter setDateFormat:@"YYYY/MM/dd"];
	
	NSString* date_string = [date_formatter stringFromDate:the_date];
	
	char* c_date_string = iupStrReturnStr([date_string UTF8String]);
	[date_formatter release];
	
	return c_date_string;
}

static char* cocoaCalendarGetValueAttrib(Ihandle* ih)
{
	NSDatePicker* date_picker = (NSDatePicker*)ih->handle;
	NSDate* the_date = [date_picker dateValue];
	return cocoaCalendarGetIupStrFromDate(the_date);
}

static char* cocoaCalendarGetTodayAttrib(Ihandle* ih)
{
	NSDate* today_date = [NSDate date];
	return cocoaCalendarGetIupStrFromDate(today_date);
}




static void cocoaCalendarComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
	(void)children_expand; /* unset if not a container */

	if(ih->handle)
	{
		NSDatePicker* date_picker = (NSDatePicker*)ih->handle;
		NSRect frame_rect = [date_picker frame];
		*w = frame_rect.size.width;
		*h = frame_rect.size.height;
	}
	else
	{
		// These are what I always get on 10.12
		*w = 139;
		*h = 148;
	}
}

static int cocoaCalendarMapMethod(Ihandle* ih)
{
//	NSRect frame = NSMakeRect(0, 0, 139, 148);
//	NSDatePicker* date_picker = [[NSDatePicker alloc] initWithFrame:frame];
	NSDatePicker* date_picker = [[NSDatePicker alloc] initWithFrame:NSZeroRect];
	[date_picker setDatePickerStyle:NSClockAndCalendarDatePickerStyle];	// set our desired picker style
	[date_picker setDatePickerElements:NSYearMonthDayDatePickerElementFlag];
	// always set the date/time to TODAY
	// note that our delete override might block this...
	[date_picker setDateValue:[NSDate date]];

	
	IupCocoaCalendarDelegate* calendar_delegate = [[IupCocoaCalendarDelegate alloc] init];
//	[date_picker setDelegate:calendar_delegate];
	[date_picker setTarget:calendar_delegate];
	[date_picker setAction:@selector(myDatePickerAction:)];
	objc_setAssociatedObject(date_picker, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
	objc_setAssociatedObject(date_picker, IUP_COCOA_CALENDAR_DELEGATE_OBJ_KEY, calendar_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[calendar_delegate release];
	
	
	[date_picker sizeToFit];
//	NSRect fit_frame = [date_picker frame];
//	NSLog(@"size %@", NSStringFromRect(fit_frame));
	
	
	
	ih->handle = date_picker;
	iupCocoaSetAssociatedViews(ih, date_picker, date_picker);
	// All Cocoa views shoud call this to add the new view to the parent view.
	iupCocoaAddToParent(ih);

	
	return IUP_NOERROR;
}

static void cocoaCalendarUnMapMethod(Ihandle* ih)
{
	id date_picker = ih->handle;
	
	// Destroy the context menu ih it exists
	{
		Ihandle* context_menu_ih = (Ihandle*)iupCocoaCommonBaseGetContextMenuAttrib(ih);
		if(NULL != context_menu_ih)
		{
			IupDestroy(context_menu_ih);
		}
		iupCocoaCommonBaseSetContextMenuAttrib(ih, NULL);
	}
	
	iupCocoaRemoveFromParent(ih);
	iupCocoaSetAssociatedViews(ih, nil, nil);

	[date_picker release];
	ih->handle = NULL;
	
}

Iclass* iupCalendarNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "calendar";
  ic->format = NULL; /* none */
  ic->nativetype = IUP_TYPECONTROL;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iupCalendarNewClass;

	ic->Map = cocoaCalendarMapMethod;
	ic->UnMap = cocoaCalendarUnMapMethod;
	ic->ComputeNaturalSize = cocoaCalendarComputeNaturalSizeMethod;
	ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;


  /* Callbacks */
  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  /* Common Callbacks */
  iupBaseRegisterCommonCallbacks(ic);

  /* Common */
  iupBaseRegisterCommonAttrib(ic);

  /* Visual */
  iupBaseRegisterVisualAttrib(ic);

  /* IupCalendar only */
  iupClassRegisterAttribute(ic, "VALUE", cocoaCalendarGetValueAttrib, cocoaCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
//  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, gtkCalendarSetWeekNumbersAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", cocoaCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

	/* New API for view specific contextual menus (Mac only) */
	// Hmmmm. Context menus don't seem to work on NSDatePicker.
//	iupClassRegisterAttribute(ic, "CONTEXTMENU", iupCocoaCommonBaseGetContextMenuAttrib, iupCocoaCommonBaseSetContextMenuAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupCocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);

  return ic;
}

Ihandle *IupCalendar(void)
{
  return IupCreate("calendar");
}
