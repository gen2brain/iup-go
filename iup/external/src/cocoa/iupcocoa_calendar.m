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

- (IBAction)myDatePickerAction:(id)the_sender
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(the_sender, IHANDLE_ASSOCIATED_OBJ_KEY);
  iupBaseCallValueChangedCb(ih);
}

@end

static int cocoaCalendarSetValueAttrib(Ihandle* ih, const char* value)
{
  NSDatePicker* date_picker = (NSDatePicker*)ih->handle;

  if (iupStrEqualNoCase(value, "TODAY"))
  {
    [date_picker setDateValue:[NSDate date]];
  }
  else if (value)
  {
    int year, month, day;
    if (sscanf(value, "%d/%d/%d", &year, &month, &day) == 3)
    {
      if (month < 1) month = 1;
      if (month > 12) month = 12;
      if (day < 1) day = 1;
      if (day > 31) day = 31;

      NSDateComponents* components = [[NSDateComponents alloc] init];
      [components setYear:year];
      [components setMonth:month];
      [components setDay:day];

      NSCalendar* calendar = [NSCalendar currentCalendar];
      NSDate* parsed_date = [calendar dateFromComponents:components];
      [components release];

      if (parsed_date)
      {
        [date_picker setDateValue:parsed_date];
      }
    }
  }

  return 0;
}

static char* cocoaCalendarGetIupStrFromDate(NSDate* the_date)
{
  NSDateComponents* components = [[NSCalendar currentCalendar] components:NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay
                                                                 fromDate:the_date];

  char* result = iupStrReturnStrf("%ld/%ld/%ld",
      (long)[components year],
      (long)[components month],
      (long)[components day]);

  return result;
}

static char* cocoaCalendarGetValueAttrib(Ihandle* ih)
{
  NSDatePicker* date_picker = (NSDatePicker*)ih->handle;
  NSDate* the_date = [date_picker dateValue];
  return cocoaCalendarGetIupStrFromDate(the_date);
}

static char* cocoaCalendarGetTodayAttrib(Ihandle* ih)
{
  (void)ih;
  NSDate* today_date = [NSDate date];
  return cocoaCalendarGetIupStrFromDate(today_date);
}

static int cocoaCalendarSetWeekNumbersAttrib(Ihandle* ih, const char* value)
{
  /* NSDatePicker does not support week numbers on macOS */
  (void)ih;
  (void)value;
  return 0;
}

static void cocoaCalendarComputeNaturalSizeMethod(Ihandle* ih, int *w, int *h, int *children_expand)
{
  (void)children_expand;

  if (ih->handle)
  {
    NSDatePicker* date_picker = (NSDatePicker*)ih->handle;
    NSRect frame_rect = [date_picker frame];
    *w = iupROUND(frame_rect.size.width);
    *h = iupROUND(frame_rect.size.height);
  }
  else
  {
    iupdrvFontGetMultiLineStringSize(ih, "W8W", w, h);

    *h += 2;
    (*w) *= 7;
    (*h) *= 8;
    *h += 2;

    iupdrvTextAddBorders(ih, w, h);
  }
}

static int cocoaCalendarMapMethod(Ihandle* ih)
{
  NSDatePicker* date_picker = [[NSDatePicker alloc] initWithFrame:NSZeroRect];

  if (@available(macOS 10.15.4, *)) {
    [date_picker setDatePickerStyle:NSDatePickerStyleClockAndCalendar];
  } else {
    [date_picker setDatePickerStyle:NSClockAndCalendarDatePickerStyle];
  }

  [date_picker setDatePickerElements:NSDatePickerElementFlagYearMonthDay];
  [date_picker setDateValue:[NSDate date]];
  [date_picker setBezeled:YES];
  [date_picker setBordered:YES];

  IupCocoaCalendarDelegate* calendar_delegate = [[IupCocoaCalendarDelegate alloc] init];
  [date_picker setTarget:calendar_delegate];
  [date_picker setAction:@selector(myDatePickerAction:)];

  objc_setAssociatedObject(date_picker, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);
  objc_setAssociatedObject(date_picker, IUP_COCOA_CALENDAR_DELEGATE_OBJ_KEY, calendar_delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [calendar_delegate release];

  [date_picker sizeToFit];

  ih->handle = date_picker;
  iupCocoaSetAssociatedViews(ih, date_picker, date_picker);

  iupCocoaAddToParent(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    [date_picker setRefusesFirstResponder:YES];
    iupCocoaSetCanFocus(ih, 0);
  }
  else
  {
    iupCocoaSetCanFocus(ih, 1);
  }

  return IUP_NOERROR;
}

static void cocoaCalendarUnMapMethod(Ihandle* ih)
{
  if (!ih->handle)
    return;

  id date_picker = ih->handle;

  objc_setAssociatedObject(date_picker, IUP_COCOA_CALENDAR_DELEGATE_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  objc_setAssociatedObject(date_picker, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

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
  ic->Map = cocoaCalendarMapMethod;
  ic->UnMap = cocoaCalendarUnMapMethod;
  ic->ComputeNaturalSize = cocoaCalendarComputeNaturalSizeMethod;
  ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

  iupClassRegisterCallback(ic, "VALUECHANGED_CB", "");

  iupBaseRegisterCommonCallbacks(ic);
  iupBaseRegisterCommonAttrib(ic);
  iupBaseRegisterVisualAttrib(ic);

  iupClassRegisterAttribute(ic, "VALUE", cocoaCalendarGetValueAttrib, cocoaCalendarSetValueAttrib, NULL, "TODAY", IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "WEEKNUMBERS", NULL, cocoaCalendarSetWeekNumbersAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TODAY", cocoaCalendarGetTodayAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_READONLY | IUPAF_NO_DEFAULTVALUE | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupCocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);

  return ic;
}

Ihandle* IupCalendar(void)
{
  return IupCreate("calendar");
}
