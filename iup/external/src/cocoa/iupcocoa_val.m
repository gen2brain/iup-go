/** \file
 * \brief Valuator Control
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_val.h"
#include "iup_drv.h"

#include "iupcocoa_drv.h"
#include "iupcocoa_keycodes.h"


static const void* IUP_COCOA_SLIDER_RECEIVER_OBJ_KEY = "IUP_COCOA_SLIDER_RECEIVER_OBJ_KEY";

@interface IupCocoaValSlider : NSSlider
@end

@interface IupCocoaSliderReceiver : NSObject
- (IBAction)sliderAction:(id)sender;
@end

@implementation IupCocoaValSlider

- (BOOL)acceptsFirstResponder
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih)
    return [super acceptsFirstResponder];

  const char* canfocus = iupAttribGet(ih, "_IUPCOCOA_CANFOCUS");
  if (canfocus && iupStrEqualNoCase(canfocus, "NO"))
    return NO;

  return [super acceptsFirstResponder];
}

- (BOOL)becomeFirstResponder
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    iupcocoaFocusIn(ih);
  }
  return [super becomeFirstResponder];
}

- (BOOL)resignFirstResponder
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    iupcocoaFocusOut(ih);
  }
  return [super resignFirstResponder];
}

- (BOOL) needsPanelToBecomeKey
{
  return YES;
}

- (void)mouseDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    iupAttribSet(ih, "_IUPCOCOA_VAL_DRAGGING", "1");
    iupAttribSet(ih, "_IUPCOCOA_VAL_BUTTONRELEASE", NULL);
  }
  [super mouseDown:event];
}

- (void)mouseUp:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (ih)
  {
    iupAttribSet(ih, "_IUPCOCOA_VAL_BUTTONRELEASE", "1");
    iupAttribSet(ih, "_IUPCOCOA_VAL_DRAGGING", NULL);
  }
  [super mouseUp:event];
}

- (void)keyDown:(NSEvent *)event
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(self, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih)
  {
    [super keyDown:event];
    return;
  }

  int keyCode = [event keyCode];
  if (iupcocoaKeyEvent(ih, event, keyCode, true))
    return;

  BOOL handled = NO;

  if (keyCode == kVK_Home)
  {
    if (ih->data->inverted)
      [self setDoubleValue:ih->data->vmax];
    else
      [self setDoubleValue:ih->data->vmin];

    IupCocoaSliderReceiver* receiver = (IupCocoaSliderReceiver*)objc_getAssociatedObject(self, IUP_COCOA_SLIDER_RECEIVER_OBJ_KEY);
    if (receiver)
    {
      iupAttribSet(ih, "_IUPCOCOA_VAL_KEYPRESS", "1");
      [receiver sliderAction:self];
      iupAttribSet(ih, "_IUPCOCOA_VAL_KEYPRESS", NULL);
    }
    handled = YES;
  }
  else if (keyCode == kVK_End)
  {
    if (ih->data->inverted)
      [self setDoubleValue:ih->data->vmin];
    else
      [self setDoubleValue:ih->data->vmax];

    IupCocoaSliderReceiver* receiver = (IupCocoaSliderReceiver*)objc_getAssociatedObject(self, IUP_COCOA_SLIDER_RECEIVER_OBJ_KEY);
    if (receiver)
    {
      iupAttribSet(ih, "_IUPCOCOA_VAL_KEYPRESS", "1");
      [receiver sliderAction:self];
      iupAttribSet(ih, "_IUPCOCOA_VAL_KEYPRESS", NULL);
    }
    handled = YES;
  }

  if (!handled)
  {
    [super keyDown:event];
  }
}

@end


@implementation IupCocoaSliderReceiver

- (IBAction)sliderAction:(id)sender
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(sender, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih)
  {
    return;
  }

  NSSlider* slider = (NSSlider*)sender;
  double slider_val = [slider doubleValue];
  double old_iup_val = ih->data->val;

  ih->data->val = ih->data->inverted ? (ih->data->vmax - slider_val) + ih->data->vmin : slider_val;
  iupValCropValue(ih);

  IFn valuechanged_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
  if (valuechanged_cb)
  {
    if (ih->data->val == old_iup_val)
    {
      return;
    }
    valuechanged_cb(ih);
  }
  else
  {
    IFnd cb_old = NULL;

    if (iupAttribGet(ih, "_IUPCOCOA_VAL_BUTTONRELEASE"))
    {
      cb_old = (IFnd)IupGetCallback(ih, "BUTTON_RELEASE_CB");
      iupAttribSet(ih, "_IUPCOCOA_VAL_BUTTONRELEASE", NULL);
    }
    else if (iupAttribGet(ih, "_IUPCOCOA_VAL_DRAGGING"))
    {
      cb_old = (IFnd)IupGetCallback(ih, "MOUSEMOVE_CB");
    }
    else if (iupAttribGet(ih, "_IUPCOCOA_VAL_KEYPRESS"))
    {
      cb_old = (IFnd)IupGetCallback(ih, "BUTTON_PRESS_CB");
    }
    else
    {
      NSEvent* event = [NSApp currentEvent];
      NSEventType event_type = [event type];

      if (event_type == NSEventTypeKeyDown)
      {
        cb_old = (IFnd)IupGetCallback(ih, "BUTTON_PRESS_CB");
      }
    }

    if (cb_old)
    {
      cb_old(ih, ih->data->val);
    }
  }
}

@end

void iupdrvValGetMinSize(Ihandle* ih, int *w, int *h)
{
  int ticks_size = 0;
  if (iupAttribGetInt(ih, "SHOWTICKS") > 0)
  {
    char* tickspos = iupAttribGetStr(ih, "TICKSPOS");
    if (iupStrEqualNoCase(tickspos, "BOTH"))
    {
      /* NSSlider doesn't support both sides, but account for the user's expectation */
      ticks_size = 2 * 8;
    }
    else
    {
      ticks_size = 8;
    }
  }

  if (ih->data->orientation == IVAL_HORIZONTAL)
  {
    *w = 35;
    *h = 30 + ticks_size;
  }
  else
  {
    *w = 30 + ticks_size;
    *h = 35;
  }
}

static int cocoaValSetValueAttrib(Ihandle* ih, const char* value)
{
  double new_iup_val;

  if (!value)
  {
    /* If value is NULL, use current value (e.g., when MIN/MAX changed) */
    new_iup_val = ih->data->val;
  }
  else if (!iupStrToDouble(value, &new_iup_val))
  {
    return 0;
  }

  NSSlider* slider = ih->handle;

  if (new_iup_val < ih->data->vmin) new_iup_val = ih->data->vmin;
  if (new_iup_val > ih->data->vmax) new_iup_val = ih->data->vmax;

  ih->data->val = new_iup_val;

  /* If inverted, transform the IUP value to the native slider's coordinate space. */
  double native_val = ih->data->inverted ? (ih->data->vmax - new_iup_val) + ih->data->vmin : new_iup_val;
  [slider setDoubleValue:native_val];

  return 0;
}

static int cocoaValSetMaxAttrib(Ihandle* ih, const char* value)
{
  double new_value;
  if (iupStrToDouble(value, &new_value))
  {
    NSSlider* slider = ih->handle;
    ih->data->vmax = new_value;
    [slider setMaxValue:new_value];

    double inc_size = ih->data->step * (ih->data->vmax - ih->data->vmin);
    [slider setIncrementValue:inc_size];

    double page_inc_size = ih->data->pagestep * (ih->data->vmax - ih->data->vmin);
    [slider setAltIncrementValue:page_inc_size];

    cocoaValSetValueAttrib(ih, NULL);
  }
  return 0;
}

static char* cocoaValGetMaxAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmax);
}

static int cocoaValSetMinAttrib(Ihandle* ih, const char* value)
{
  double new_value;
  if (iupStrToDouble(value, &new_value))
  {
    NSSlider* slider = ih->handle;
    ih->data->vmin = new_value;
    [slider setMinValue:new_value];

    double inc_size = ih->data->step * (ih->data->vmax - ih->data->vmin);
    [slider setIncrementValue:inc_size];

    double page_inc_size = ih->data->pagestep * (ih->data->vmax - ih->data->vmin);
    [slider setAltIncrementValue:page_inc_size];

    cocoaValSetValueAttrib(ih, NULL);
  }
  return 0;
}

static char* cocoaValGetMinAttrib(Ihandle* ih)
{
  return iupStrReturnDouble(ih->data->vmin);
}

static int cocoaValSetStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->step), 0.01))
  {
    /* STEP is a fraction of the total range. Convert to an absolute value for Cocoa. */
    double inc_size = ih->data->step * (ih->data->vmax - ih->data->vmin);
    NSSlider* slider = ih->handle;
    /* incrementValue is used for arrow key presses. */
    [slider setIncrementValue:inc_size];
  }
  return 0;
}

static int cocoaValSetPageStepAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1))
  {
    /* PAGESTEP is a fraction of the total range. Convert to an absolute value. */
    double page_inc_size = ih->data->pagestep * (ih->data->vmax - ih->data->vmin);
    NSSlider* slider = ih->handle;
    /* altIncrementValue is used when holding Option key, which is the closest available behavior to PAGESTEP on macOS. */
    [slider setAltIncrementValue:page_inc_size];
  }
  return 0;
}

static int cocoaValSetShowTicksAttrib(Ihandle* ih, const char* value)
{
  int show_ticks = 0;
  if (value)
  {
    show_ticks = atoi(value);
  }
  if (show_ticks < 0) show_ticks = 0;

  ih->data->show_ticks = show_ticks;

  NSSlider* slider = ih->handle;
  [slider setNumberOfTickMarks:show_ticks];

  return 0;
}

static int cocoaValSetStepOnTicksAttrib(Ihandle* ih, const char* value)
{
  NSSlider* slider = ih->handle;
  [slider setAllowsTickMarkValuesOnly:(BOOL)iupStrBoolean(value)];
  return 1; /* Store value */
}

static char* cocoaValGetStepOnTicksAttrib(Ihandle* ih)
{
  NSSlider* slider = ih->handle;
  return iupStrReturnBoolean([slider allowsTickMarkValuesOnly]);
}

static int cocoaValSetTicksPosAttrib(Ihandle* ih, const char* value)
{
  NSSlider* slider = ih->handle;
  BOOL is_vertical = (ih->data->orientation == IVAL_VERTICAL);

  if (iupStrEqualNoCase(value, "BOTH"))
  {
    /* NSSlider doesn't support ticks on both sides, default to NORMAL */
    [slider setTickMarkPosition: is_vertical ? NSTickMarkPositionLeading : NSTickMarkPositionBelow];
  }
  else if (iupStrEqualNoCase(value, "REVERSE"))
  {
    [slider setTickMarkPosition: is_vertical ? NSTickMarkPositionTrailing : NSTickMarkPositionAbove];
  }
  else /* "NORMAL" */
  {
    [slider setTickMarkPosition: is_vertical ? NSTickMarkPositionLeading : NSTickMarkPositionBelow];
  }
  return 1;
}

static char* cocoaValGetInvertedAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->inverted);
}

static int cocoaValSetInvertedAttrib(Ihandle* ih, const char* value)
{
  ih->data->inverted = iupStrBoolean(value);
  /* Re-apply the current value to reflect the inversion */
  cocoaValSetValueAttrib(ih, iupValGetValueAttrib(ih));
  return 0;
}

static int cocoaValMapMethod(Ihandle* ih)
{
  IupCocoaValSlider* slider = [[IupCocoaValSlider alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
  ih->handle = slider;

  if (ih->data->orientation == IVAL_VERTICAL)
  {
    [slider setVertical:YES];
  }

  [slider setContinuous:YES];

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    [slider setRefusesFirstResponder:YES];
    iupcocoaSetCanFocus(ih, 0);
  }
  else
  {
    iupcocoaSetCanFocus(ih, 1);
  }

  objc_setAssociatedObject(slider, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

  IupCocoaSliderReceiver* slider_receiver = [[IupCocoaSliderReceiver alloc] init];
  [slider setTarget:slider_receiver];
  [slider setAction:@selector(sliderAction:)];

  objc_setAssociatedObject(slider, IUP_COCOA_SLIDER_RECEIVER_OBJ_KEY, slider_receiver, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [slider_receiver release];

  iupcocoaSetAssociatedViews(ih, slider, slider);
  iupcocoaAddToParent(ih);

  cocoaValSetMinAttrib(ih, iupAttribGetStr(ih, "MIN"));
  cocoaValSetMaxAttrib(ih, iupAttribGetStr(ih, "MAX"));
  cocoaValSetStepAttrib(ih, NULL);
  cocoaValSetPageStepAttrib(ih, NULL);

  const char* inverted_str = iupAttribGetStr(ih, "INVERTED");
  if (inverted_str)
  {
    ih->data->inverted = iupStrBoolean(inverted_str);
  }

  const char* value_str = iupAttribGetStr(ih, "VALUE");
  if (value_str)
  {
    cocoaValSetValueAttrib(ih, value_str);
  }
  else
  {
    cocoaValSetValueAttrib(ih, "0");
  }

  return IUP_NOERROR;
}

static void cocoaValUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
  {
    NSSlider* slider = ih->handle;

    iupcocoaTipsDestroy(ih);

    [slider setTarget:nil];
    objc_setAssociatedObject(slider, IUP_COCOA_SLIDER_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(slider, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

    iupcocoaRemoveFromParent(ih);
    iupcocoaSetAssociatedViews(ih, nil, nil);
    [slider release];
    ih->handle = NULL;
  }
}

void iupdrvValInitClass(Iclass* ic)
{
  ic->Map = cocoaValMapMethod;
  ic->UnMap = cocoaValUnMapMethod;

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Common */
  iupClassRegisterAttribute(ic, "TIP", NULL, iupdrvBaseSetTipAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupVal */
  iupClassRegisterAttribute(ic, "VALUE", iupValGetValueAttrib, cocoaValSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MAX", cocoaValGetMaxAttrib, cocoaValSetMaxAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MIN", cocoaValGetMinAttrib, cocoaValSetMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STEP", iupValGetStepAttrib, cocoaValSetStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupValGetPageStepAttrib, cocoaValSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INVERTED", cocoaValGetInvertedAttrib, cocoaValSetInvertedAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  /* Ticks */
  iupClassRegisterAttribute(ic, "SHOWTICKS", iupValGetShowTicksAttrib, cocoaValSetShowTicksAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TICKSPOS", NULL, cocoaValSetTicksPosAttrib, "NORMAL", NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "STEPONTICKS", cocoaValGetStepOnTicksAttrib, cocoaValSetStepOnTicksAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_DEFAULT);

  /* macOS Specific */
  iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupcocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);
}
