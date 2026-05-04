/** \file
 * \brief DatePick Control (iOS UIKit, UIDatePicker compact style)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_register.h"

#include "iupcocoatouch_drv.h"


static const void* IUP_COCOATOUCH_DATEPICK_TARGET_OBJ_KEY = "IUP_COCOATOUCH_DATEPICK_TARGET_OBJ_KEY";

@interface IupCocoaTouchDatePickTarget : NSObject
- (void)onDateChanged:(id)sender;
@end

@implementation IupCocoaTouchDatePickTarget
- (void)onDateChanged:(id)sender
{
	Ihandle* ih = (Ihandle*)objc_getAssociatedObject(sender, IHANDLE_ASSOCIATED_OBJ_KEY);
	if (!ih || !iupObjectCheck(ih)) return;
	iupBaseCallValueChangedCb(ih);
}
@end


static UIDatePicker* cocoaTouchDatePickGet(Ihandle* ih)
{
	if ([(id)ih->handle isKindOfClass:[UIDatePicker class]])
		return (UIDatePicker*)ih->handle;
	return nil;
}

static char* cocoaTouchDatePickFormatDate(NSDate* date)
{
	NSDateComponents* comps = [[NSCalendar currentCalendar]
		components:(NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay)
		fromDate:date];
	return iupStrReturnStrf("%ld/%ld/%ld",
		(long)[comps year], (long)[comps month], (long)[comps day]);
}

static NSDate* cocoaTouchDatePickParseDate(const char* value)
{
	if (!value || iupStrEqualNoCase(value, "TODAY")) return [NSDate date];

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

static int cocoaTouchDatePickSetValueAttrib(Ihandle* ih, const char* value)
{
	UIDatePicker* picker = cocoaTouchDatePickGet(ih);
	if (!picker) return 0;
	NSDate* parsed = cocoaTouchDatePickParseDate(value ? value : "TODAY");
	if (parsed) [picker setDate:parsed animated:NO];
	return 0;
}

static char* cocoaTouchDatePickGetValueAttrib(Ihandle* ih)
{
	UIDatePicker* picker = cocoaTouchDatePickGet(ih);
	if (!picker) return NULL;
	return cocoaTouchDatePickFormatDate([picker date]);
}

static char* cocoaTouchDatePickGetTodayAttrib(Ihandle* ih)
{
	(void)ih;
	return cocoaTouchDatePickFormatDate([NSDate date]);
}

static void cocoaTouchDatePickComputeNaturalSize(Ihandle* ih, int* w, int* h, int* children_expand)
{
	(void)children_expand;
	UIDatePicker* picker = cocoaTouchDatePickGet(ih);
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

	/* pre-map fallback matching compact mode's chip size */
	*w = 120;
	*h = 34;
}

static int cocoaTouchDatePickMapMethod(Ihandle* ih)
{
	UIDatePicker* picker = [[UIDatePicker alloc] initWithFrame:CGRectZero];
	[picker setDatePickerMode:UIDatePickerModeDate];
	[picker setPreferredDatePickerStyle:UIDatePickerStyleCompact];
	[picker setDate:[NSDate date] animated:NO];

	IupCocoaTouchDatePickTarget* target = [[IupCocoaTouchDatePickTarget alloc] init];
	[picker addTarget:target action:@selector(onDateChanged:) forControlEvents:UIControlEventValueChanged];

	objc_setAssociatedObject(picker, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
	objc_setAssociatedObject(picker, IUP_COCOATOUCH_DATEPICK_TARGET_OBJ_KEY, target, OBJC_ASSOCIATION_RETAIN);
	[target release];

	ih->handle = picker;
	iupCocoaTouchAddToParent(ih);
	return IUP_NOERROR;
}

static void cocoaTouchDatePickUnMapMethod(Ihandle* ih)
{
	UIDatePicker* picker = cocoaTouchDatePickGet(ih);
	if (picker)
	{
		id target = objc_getAssociatedObject(picker, IUP_COCOATOUCH_DATEPICK_TARGET_OBJ_KEY);
		if (target) [picker removeTarget:target action:NULL forControlEvents:UIControlEventValueChanged];
		objc_setAssociatedObject(picker, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(picker, IUP_COCOATOUCH_DATEPICK_TARGET_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	}
	iupdrvBaseUnMapMethod(ih);
}

Iclass* iupDatePickNewClass(void)
{
	Iclass* ic = iupClassNew(NULL);

	ic->name = "datepick";
	ic->cons = "DatePick";
	ic->format = NULL;
	ic->nativetype = IUP_TYPECONTROL;
	ic->childtype = IUP_CHILDNONE;
	ic->is_interactive = 1;

	ic->New = iupDatePickNewClass;
	ic->Map = cocoaTouchDatePickMapMethod;
	ic->UnMap = cocoaTouchDatePickUnMapMethod;
	ic->ComputeNaturalSize = cocoaTouchDatePickComputeNaturalSize;
	ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

	iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");
	iupBaseRegisterCommonCallbacks(ic);
	iupBaseRegisterCommonAttrib(ic);
	iupBaseRegisterVisualAttrib(ic);

	iupClassRegisterAttribute(ic, "VALUE", cocoaTouchDatePickGetValueAttrib, cocoaTouchDatePickSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TODAY", cocoaTouchDatePickGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);

	/* compact label uses locale's medium-date style; no override path */
	iupClassRegisterAttribute(ic, "ORDER",           NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SEPARATOR",       NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ZEROPRECED",      NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MONTHSHORTNAMES", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FORMAT",          NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "CALENDARWEEKNUMBERS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SHOWDROPDOWN",        NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

	return ic;
}

Ihandle* IupDatePick(void)
{
	return IupCreate("datepick");
}
