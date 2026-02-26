/** \file
 * \brief Scrollbar Control
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
#include "iup_scrollbar.h"
#include "iup_drv.h"

#include "iupcocoa_drv.h"


static const void* IUP_COCOA_SCROLLBAR_RECEIVER_OBJ_KEY = "IUP_COCOA_SCROLLBAR_RECEIVER_OBJ_KEY";

@interface IupCocoaScrollbarReceiver : NSObject
- (IBAction)scrollbarAction:(id)sender;
@end

@implementation IupCocoaScrollbarReceiver

- (IBAction)scrollbarAction:(id)sender
{
  Ihandle* ih = (Ihandle*)objc_getAssociatedObject(sender, IHANDLE_ASSOCIATED_OBJ_KEY);
  if (!ih)
    return;

  NSScroller* scroller = (NSScroller*)sender;
  double old_val = ih->data->val;
  double range = ih->data->vmax - ih->data->vmin;
  double max_pos = ih->data->vmax - ih->data->pagesize;
  int op = -1;

  NSScrollerPart part = [scroller hitPart];

  switch (part)
  {
  case NSScrollerKnob:
    {
      double knob_val = [scroller doubleValue];
      ih->data->val = knob_val * (max_pos - ih->data->vmin) + ih->data->vmin;
      if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
        op = IUP_SBDRAGH;
      else
        op = IUP_SBDRAGV;
      break;
    }
  case NSScrollerDecrementLine:
    ih->data->val -= ih->data->linestep;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      op = IUP_SBLEFT;
    else
      op = IUP_SBUP;
    break;
  case NSScrollerIncrementLine:
    ih->data->val += ih->data->linestep;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      op = IUP_SBRIGHT;
    else
      op = IUP_SBDN;
    break;
  case NSScrollerDecrementPage:
    ih->data->val -= ih->data->pagestep;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      op = IUP_SBPGLEFT;
    else
      op = IUP_SBPGUP;
    break;
  case NSScrollerIncrementPage:
    ih->data->val += ih->data->pagestep;
    if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
      op = IUP_SBPGRIGHT;
    else
      op = IUP_SBPGDN;
    break;
  default:
    return;
  }

  iupScrollbarCropValue(ih);

  if (part != NSScrollerKnob)
  {
    double new_knob_val = 0;
    if (max_pos > ih->data->vmin)
      new_knob_val = (ih->data->val - ih->data->vmin) / (max_pos - ih->data->vmin);
    [scroller setDoubleValue:new_knob_val];
  }

  {
    IFniff scroll_cb = (IFniff)IupGetCallback(ih, "SCROLL_CB");
    if (scroll_cb)
    {
      float posx = 0, posy = 0;
      if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
        posx = (float)ih->data->val;
      else
        posy = (float)ih->data->val;

      scroll_cb(ih, op, posx, posy);
    }
  }

  {
    IFn valuechanged_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
    if (valuechanged_cb)
    {
      if (ih->data->val != old_val)
        valuechanged_cb(ih);
    }
  }
}

@end


void iupdrvScrollbarGetMinSize(Ihandle* ih, int *w, int *h)
{
  CGFloat sb_size = [NSScroller scrollerWidthForControlSize:NSControlSizeRegular scrollerStyle:NSScrollerStyleLegacy];
  int isb_size = (int)sb_size;
  if (isb_size < 15) isb_size = 15;

  if (ih->data->orientation == ISCROLLBAR_HORIZONTAL)
  {
    *w = 3 * isb_size;
    *h = isb_size;
  }
  else
  {
    *w = isb_size;
    *h = 3 * isb_size;
  }
}

static void cocoaScrollbarUpdateNative(Ihandle* ih)
{
  NSScroller* scroller = ih->handle;
  double max_pos = ih->data->vmax - ih->data->pagesize;
  double range = ih->data->vmax - ih->data->vmin;
  double knob_proportion, knob_val;

  if (range <= 0)
    return;

  knob_proportion = ih->data->pagesize / range;
  if (knob_proportion > 1.0) knob_proportion = 1.0;
  if (knob_proportion < 0.01) knob_proportion = 0.01;

  knob_val = 0;
  if (max_pos > ih->data->vmin)
    knob_val = (ih->data->val - ih->data->vmin) / (max_pos - ih->data->vmin);
  if (knob_val < 0.0) knob_val = 0.0;
  if (knob_val > 1.0) knob_val = 1.0;

  [scroller setKnobProportion:knob_proportion];
  [scroller setDoubleValue:knob_val];
}

static int cocoaScrollbarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDouble(value, &(ih->data->val)))
  {
    iupScrollbarCropValue(ih);
    cocoaScrollbarUpdateNative(ih);
  }
  return 0;
}

static int cocoaScrollbarSetLineStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->linestep), 0.01);
  return 0;
}

static int cocoaScrollbarSetPageStepAttrib(Ihandle* ih, const char* value)
{
  iupStrToDoubleDef(value, &(ih->data->pagestep), 0.1);
  return 0;
}

static int cocoaScrollbarSetPageSizeAttrib(Ihandle* ih, const char* value)
{
  if (iupStrToDoubleDef(value, &(ih->data->pagesize), 0.1))
  {
    iupScrollbarCropValue(ih);
    cocoaScrollbarUpdateNative(ih);
  }
  return 0;
}

static int cocoaScrollbarMapMethod(Ihandle* ih)
{
  NSScroller* scroller = [[NSScroller alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
  ih->handle = scroller;

  [scroller setScrollerStyle:NSScrollerStyleLegacy];
  [scroller setEnabled:YES];

  objc_setAssociatedObject(scroller, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

  IupCocoaScrollbarReceiver* receiver = [[IupCocoaScrollbarReceiver alloc] init];
  [scroller setTarget:receiver];
  [scroller setAction:@selector(scrollbarAction:)];

  objc_setAssociatedObject(scroller, IUP_COCOA_SCROLLBAR_RECEIVER_OBJ_KEY, receiver, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  [receiver release];

  iupcocoaSetAssociatedViews(ih, scroller, scroller);
  iupcocoaAddToParent(ih);

  cocoaScrollbarUpdateNative(ih);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    [scroller setRefusesFirstResponder:YES];
    iupcocoaSetCanFocus(ih, 0);
  }
  else
  {
    iupcocoaSetCanFocus(ih, 1);
  }

  return IUP_NOERROR;
}

static void cocoaScrollbarUnMapMethod(Ihandle* ih)
{
  if (ih->handle)
  {
    NSScroller* scroller = ih->handle;

    iupcocoaTipsDestroy(ih);

    [scroller setTarget:nil];
    objc_setAssociatedObject(scroller, IUP_COCOA_SCROLLBAR_RECEIVER_OBJ_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(scroller, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

    iupcocoaRemoveFromParent(ih);
    iupcocoaSetAssociatedViews(ih, nil, nil);
    [scroller release];
    ih->handle = NULL;
  }
}

void iupdrvScrollbarInitClass(Iclass* ic)
{
  ic->Map = cocoaScrollbarMapMethod;
  ic->UnMap = cocoaScrollbarUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "VALUE", iupScrollbarGetValueAttrib, cocoaScrollbarSetValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINESTEP", iupScrollbarGetLineStepAttrib, cocoaScrollbarSetLineStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESTEP", iupScrollbarGetPageStepAttrib, cocoaScrollbarSetPageStepAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", iupScrollbarGetPageSizeAttrib, cocoaScrollbarSetPageSizeAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "LAYERBACKED", iupCocoaCommonBaseGetLayerBackedAttrib, iupcocoaCommonBaseSetLayerBackedAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE);
}
