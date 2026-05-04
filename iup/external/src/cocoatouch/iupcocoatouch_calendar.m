/** \file
 * \brief Calendar Control (iOS UIKit, UIDatePicker)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <stdio.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_str.h"

#include "iupcocoatouch_drv.h"


static const void* IUP_COCOATOUCH_CALENDAR_TARGET_OBJ_KEY = "IUP_COCOATOUCH_CALENDAR_TARGET_OBJ_KEY";

@interface IupCocoaTouchCalendarTarget : NSObject
- (void)onDateChanged:(id)sender;
@end

@implementation IupCocoaTouchCalendarTarget
- (void)onDateChanged:(id)sender
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(sender, IHANDLE_ASSOCIATED_OBJ_KEY);
	if (!ih || !iupObjectCheck(ih))
	{
		return;
	}
	iupBaseCallValueChangedCb(ih);
}
@end


static UIDatePicker* cocoaTouchCalendarGet(Ihandle* ih)
{
	if ([(id)ih->handle isKindOfClass:[UIDatePicker class]])
	{
		return (UIDatePicker*)ih->handle;
	}
	return nil;
}

static char* cocoaTouchCalendarFormatDate(NSDate* date)
{
	NSDateComponents* comps = [[NSCalendar currentCalendar]
		components:(NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay)
		fromDate:date];
	return iupStrReturnStrf("%ld/%ld/%ld",
		(long)[comps year], (long)[comps month], (long)[comps day]);
}

/* parse "YYYY/MM/DD" or "TODAY"; nil on fail */
static NSDate* cocoaTouchCalendarParseDate(const char* value)
{
	if (!value) return nil;
	if (iupStrEqualNoCase(value, "TODAY")) return [NSDate date];

	int year = 0, month = 0, day = 0;
	if (sscanf(value, "%d/%d/%d", &year, &month, &day) != 3) return nil;

	if (month < 1) month = 1; else if (month > 12) month = 12;
	if (day   < 1) day   = 1; else if (day   > 31) day   = 31;

	NSDateComponents* comps = [[[NSDateComponents alloc] init] autorelease];
	comps.year = year;
	comps.month = month;
	comps.day = day;
	return [[NSCalendar currentCalendar] dateFromComponents:comps];
}

static int cocoaTouchCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
	UIDatePicker* picker = cocoaTouchCalendarGet(ih);
	if (!picker) return 0;
	NSDate* parsed = cocoaTouchCalendarParseDate(value ? value : "TODAY");
	if (parsed) [picker setDate:parsed animated:NO];
	return 0;
}

static int cocoaTouchCalendarSetMinDateAttrib(Ihandle* ih, const char* value)
{
	UIDatePicker* picker = cocoaTouchCalendarGet(ih);
	if (!picker) return 1;
	picker.minimumDate = value ? cocoaTouchCalendarParseDate(value) : nil;
	return 1;
}

static int cocoaTouchCalendarSetMaxDateAttrib(Ihandle* ih, const char* value)
{
	UIDatePicker* picker = cocoaTouchCalendarGet(ih);
	if (!picker) return 1;
	picker.maximumDate = value ? cocoaTouchCalendarParseDate(value) : nil;
	return 1;
}

static char* cocoaTouchCalendarGetMinDateAttrib(Ihandle* ih)
{
	UIDatePicker* picker = cocoaTouchCalendarGet(ih);
	return (picker && picker.minimumDate) ? cocoaTouchCalendarFormatDate(picker.minimumDate) : NULL;
}

static char* cocoaTouchCalendarGetMaxDateAttrib(Ihandle* ih)
{
	UIDatePicker* picker = cocoaTouchCalendarGet(ih);
	return (picker && picker.maximumDate) ? cocoaTouchCalendarFormatDate(picker.maximumDate) : NULL;
}

static int cocoaTouchCalendarSetStyleAttrib(Ihandle* ih, const char* value)
{
	UIDatePicker* picker = cocoaTouchCalendarGet(ih);
	if (!picker) return 1;

	if (iupStrEqualNoCase(value, "WHEELS"))
		picker.preferredDatePickerStyle = UIDatePickerStyleWheels;
	else if (iupStrEqualNoCase(value, "COMPACT"))
		picker.preferredDatePickerStyle = UIDatePickerStyleCompact;
	else if (iupStrEqualNoCase(value, "AUTOMATIC"))
		picker.preferredDatePickerStyle = UIDatePickerStyleAutomatic;
	else
		picker.preferredDatePickerStyle = UIDatePickerStyleInline;
	return 1;
}

static char* cocoaTouchCalendarGetValueAttrib(Ihandle* ih)
{
	UIDatePicker* picker = cocoaTouchCalendarGet(ih);
	if (!picker) return NULL;
	return cocoaTouchCalendarFormatDate([picker date]);
}

static char* cocoaTouchCalendarGetTodayAttrib(Ihandle* ih)
{
	(void)ih;
	return cocoaTouchCalendarFormatDate([NSDate date]);
}

static void cocoaTouchCalendarComputeNaturalSize(Ihandle* ih, int* w, int* h, int* children_expand)
{
	(void)children_expand;

	UIDatePicker* picker = cocoaTouchCalendarGet(ih);
	if (picker)
	{
		CGSize size = [picker intrinsicContentSize];
		if (size.width <= 0 || size.height <= 0)
			size = [picker systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
		if (size.width > 0 && size.height > 0)
		{
			*w = iupROUND(size.width);
			*h = iupROUND(size.height);
			return;
		}
	}

	/* pre-map estimate, cached so we don't spawn a probe picker per IUP_CALENDAR */
	static int cached_w = -1;
	static int cached_h = -1;
	if (cached_w < 0)
	{
		UIDatePicker* temp = [[UIDatePicker alloc] initWithFrame:CGRectZero];
		[temp setDatePickerMode:UIDatePickerModeDate];
		[temp setPreferredDatePickerStyle:UIDatePickerStyleInline];
		CGSize size = [temp intrinsicContentSize];
		if (size.width <= 0 || size.height <= 0)
			size = [temp systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
		cached_w = iupROUND(size.width);
		cached_h = iupROUND(size.height);
		if (cached_w <= 0) cached_w = 320;
		if (cached_h <= 0) cached_h = 360;
		[temp release];
	}
	*w = cached_w;
	*h = cached_h;
}

static int cocoaTouchCalendarMapMethod(Ihandle* ih)
{
	UIDatePicker* picker = [[UIDatePicker alloc] initWithFrame:CGRectZero];
	[picker setDatePickerMode:UIDatePickerModeDate];
	[picker setPreferredDatePickerStyle:UIDatePickerStyleInline];

	[picker setDate:[NSDate date] animated:NO];

	IupCocoaTouchCalendarTarget* target = [[IupCocoaTouchCalendarTarget alloc] init];
	[picker addTarget:target action:@selector(onDateChanged:) forControlEvents:UIControlEventValueChanged];

	objc_setAssociatedObject(picker, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
	objc_setAssociatedObject(picker, IUP_COCOATOUCH_CALENDAR_TARGET_OBJ_KEY, target, OBJC_ASSOCIATION_RETAIN);
	[target release];

	ih->handle = picker;
	iupCocoaTouchAddToParent(ih);
	return IUP_NOERROR;
}

static void cocoaTouchCalendarUnMapMethod(Ihandle* ih)
{
	UIDatePicker* picker = cocoaTouchCalendarGet(ih);
	if (picker)
	{
		id target = objc_getAssociatedObject(picker, IUP_COCOATOUCH_CALENDAR_TARGET_OBJ_KEY);
		if (target)
		{
			[picker removeTarget:target action:NULL forControlEvents:UIControlEventValueChanged];
		}
		objc_setAssociatedObject(picker, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(picker, IUP_COCOATOUCH_CALENDAR_TARGET_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	}
	iupdrvBaseUnMapMethod(ih);
}

Iclass* iupCalendarNewClass(void)
{
	Iclass* ic = iupClassNew(NULL);

	ic->name = "calendar";
	ic->format = NULL;
	ic->nativetype = IUP_TYPECONTROL;
	ic->childtype = IUP_CHILDNONE;
	ic->is_interactive = 1;

	ic->New = iupCalendarNewClass;
	ic->Map = cocoaTouchCalendarMapMethod;
	ic->UnMap = cocoaTouchCalendarUnMapMethod;
	ic->ComputeNaturalSize = cocoaTouchCalendarComputeNaturalSize;
	ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

	iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

	iupBaseRegisterCommonCallbacks(ic);
	iupBaseRegisterCommonAttrib(ic);
	iupBaseRegisterVisualAttrib(ic);

	iupClassRegisterAttribute(ic, "VALUE", cocoaTouchCalendarGetValueAttrib, cocoaTouchCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TODAY", cocoaTouchCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

	/* MINDATE/MAXDATE -> min/maximumDate (cocoatouch extension) */
	iupClassRegisterAttribute(ic, "MINDATE", cocoaTouchCalendarGetMinDateAttrib, cocoaTouchCalendarSetMinDateAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MAXDATE", cocoaTouchCalendarGetMaxDateAttrib, cocoaTouchCalendarSetMaxDateAttrib, NULL, NULL, IUPAF_NO_INHERIT);

	/* STYLE: INLINE (default), COMPACT, WHEELS, AUTOMATIC */
	iupClassRegisterAttribute(ic, "STYLE", NULL, cocoaTouchCalendarSetStyleAttrib, IUPAF_SAMEASSYSTEM, "INLINE", IUPAF_NO_INHERIT);

	/* no week-numbers mode on UIDatePicker */
	iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

	return ic;
}

Ihandle* IupCalendar(void)
{
	return IupCreate("calendar");
}
