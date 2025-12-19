/** \file
 * \brief Progress bar Control
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

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
#include "iup_progressbar.h"
#include "iup_drv.h"

#include "iupcocoa_drv.h"


void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
{
  static int horiz_min_w = -1, horiz_min_h = -1;
  static int vert_min_w = -1, vert_min_h = -1;

  if (horiz_min_w < 0)
  {
    NSProgressIndicator* temp_horiz = [[NSProgressIndicator alloc] init];
    NSProgressIndicator* temp_vert = [[NSProgressIndicator alloc] init];

    [temp_horiz setStyle:NSProgressIndicatorStyleBar];
    [temp_vert setStyle:NSProgressIndicatorStyleBar];

    NSSize horiz_size = [temp_horiz fittingSize];
    NSSize vert_size = [temp_vert fittingSize];

    horiz_min_w = (int)horiz_size.width;
    horiz_min_h = (int)horiz_size.height;
    vert_min_w = (int)vert_size.width;
    vert_min_h = (int)vert_size.height;

    /* NSProgressIndicator has flexible width, so use reasonable defaults */
    if (horiz_min_w < 1) horiz_min_w = 100;
    if (horiz_min_h < 1) horiz_min_h = 20;
    if (vert_min_w < 1) vert_min_w = 20;
    if (vert_min_h < 1) vert_min_h = 100;

    [temp_horiz release];
    [temp_vert release];
  }

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    *w = vert_min_w;
    *h = vert_min_h;
  }
  else
  {
    *w = horiz_min_w;
    *h = horiz_min_h;
  }
}

/*
   NOTE: Vertical NSProgressIndicator has a rendering bug in Cocoa when simply rotated.
   WORKAROUND: Use a hierarchy: root container → transform view (rotated) → progress bar.
   This approach avoids the rendering bug without requiring layer-backed views.
   The transform view is offset to compensate for coordinate system changes after rotation.
*/

/* The offset needed to position the rotated progress bar correctly within its container */
#define VERTICAL_PROGRESSBAR_OFFSET 6.0

static NSView* cocoaProgressBarGetRootView(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return nil;

  NSView* root_container_view = (NSView*)ih->handle;
  NSCAssert([root_container_view isKindOfClass:[NSView class]], @"Expected NSView");
  return root_container_view;
}

static NSView* cocoaProgressBarGetTransformView(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return nil;

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    NSView* root_container_view = cocoaProgressBarGetRootView(ih);
    if (!root_container_view)
      return nil;

    NSView* intermediate_transform_view = [[root_container_view subviews] firstObject];
    NSCAssert([intermediate_transform_view isKindOfClass:[NSView class]], @"Expected NSView");
    return intermediate_transform_view;
  }
  else
  {
    return cocoaProgressBarGetRootView(ih);
  }
}

static NSProgressIndicator* cocoaProgressBarGetProgressIndicator(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return nil;

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    NSView* intermediate_transform_view = cocoaProgressBarGetTransformView(ih);
    if (!intermediate_transform_view)
      return nil;

    NSProgressIndicator* progress_bar = [[intermediate_transform_view subviews] firstObject];
    NSCAssert([progress_bar isKindOfClass:[NSProgressIndicator class]], @"Expected NSProgressIndicator");
    return progress_bar;
  }
  else
  {
    NSProgressIndicator* progress_indicator = (NSProgressIndicator*)ih->handle;
    NSCAssert([progress_indicator isKindOfClass:[NSProgressIndicator class]], @"Expected NSProgressIndicator");
    return progress_indicator;
  }
}

static int cocoaProgressBarSetValueAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->marquee)
    return 0;

  NSProgressIndicator* progress_bar = cocoaProgressBarGetProgressIndicator(ih);
  if (!progress_bar)
    return 0;

  if (!value)
    ih->data->value = 0;
  else
    iupStrToDouble(value, &(ih->data->value));

  iProgressBarCropValue(ih);

  [progress_bar setMinValue:ih->data->vmin];
  [progress_bar setMaxValue:ih->data->vmax];
  [progress_bar setDoubleValue:ih->data->value];
  [progress_bar displayIfNeeded];

  return 0;
}

static int cocoaProgressBarSetMarqueeAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->marquee)
    return 0;

  NSProgressIndicator* progress_bar = cocoaProgressBarGetProgressIndicator(ih);
  if (!progress_bar)
    return 0;

  if (iupStrBoolean(value))
    [progress_bar startAnimation:nil];
  else
    [progress_bar stopAnimation:nil];

  return 1;
}

static int cocoaProgressBarMapMethod(Ihandle* ih)
{
  int initial_width = 200;
  int initial_height = 30;

  IupGetIntInt(ih, "RASTERSIZE", &initial_width, &initial_height);
  if (0 == initial_width)
    initial_width = 200;
  if (0 == initial_height)
    initial_height = 30;

  NSRect initial_rect = NSMakeRect(0, 0, initial_width, initial_height);
  NSProgressIndicator* progress_indicator = [[NSProgressIndicator alloc] initWithFrame:initial_rect];
  NSView* container_view = nil;

  [progress_indicator setUsesThreadedAnimation:YES];

  if (iupAttribGetBoolean(ih, "MARQUEE"))
  {
    ih->data->marquee = 1;
    [progress_indicator setIndeterminate:YES];
  }
  else
  {
    ih->data->marquee = 0;
    [progress_indicator setIndeterminate:NO];
  }

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    int max_dim = iupMAX(initial_width, initial_height);
    NSRect container_rect = NSMakeRect(0, 0, max_dim, max_dim);

    container_view = [[NSView alloc] initWithFrame:container_rect];
    NSView* transform_view = [[NSView alloc] initWithFrame:container_rect];

    [container_view addSubview:transform_view];
    [transform_view addSubview:progress_indicator];
    [transform_view release];
    [progress_indicator release];

    NSSize widget_size = container_rect.size;

    CGFloat x_offset = -container_rect.size.width + (initial_rect.size.height - VERTICAL_PROGRESSBAR_OFFSET);
    [transform_view setFrame:NSMakeRect(x_offset, 0.0, widget_size.width, widget_size.height)];

    if (ih->userheight < ih->userwidth)
    {
      int tmp = ih->userheight;
      ih->userheight = ih->userwidth;
      ih->userwidth = tmp;
    }

    [transform_view setFrameCenterRotation:90.0];

    ih->expand = ih->expand & ~IUP_EXPAND_WIDTH;
  }
  else
  {
    container_view = progress_indicator;
    [progress_indicator setAutoresizingMask:(NSViewWidthSizable)];

    ih->expand = ih->expand & ~IUP_EXPAND_HEIGHT;
  }

  ih->handle = container_view;
  iupcocoaSetAssociatedViews(ih, progress_indicator, container_view);
  iupcocoaAddToParent(ih);

  return IUP_NOERROR;
}

void iupdrvProgressBarInitClass(Iclass* ic)
{
  ic->Map = cocoaProgressBarMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, cocoaProgressBarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE", NULL, cocoaProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
