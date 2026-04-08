/** \file
 * \brief Progress bar Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_progressbar.h"

#include "iupcocoa_drv.h"


IUP_SDK_API void iupdrvProgressBarGetMinSize(Ihandle* ih, int* w, int* h)
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

    horiz_min_w = (int)ceilf(horiz_size.width);
    horiz_min_h = (int)ceilf(horiz_size.height);
    vert_min_w = (int)ceilf(vert_size.width);
    vert_min_h = (int)ceilf(vert_size.height);

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

static NSProgressIndicator* cocoaProgressBarGetProgressIndicator(Ihandle* ih)
{
  if (!ih || !ih->handle)
    return nil;

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    NSView* container_view = (NSView*)ih->handle;
    NSProgressIndicator* progress_bar = (NSProgressIndicator*)[[container_view subviews] firstObject];
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

static void cocoaProgressBarUpdateVerticalLayout(NSView* container_view, NSProgressIndicator* progress_bar)
{
  NSRect container_bounds = [container_view bounds];
  CGFloat container_w = container_bounds.size.width;
  CGFloat container_h = container_bounds.size.height;

  /* The progress indicator is horizontal but rotated -90 degrees via setFrameCenterRotation.
     Set it with swapped dimensions, centered in the container.
     After rotation the visual width becomes container_w and visual height becomes container_h. */
  CGFloat indicator_w = container_h;
  CGFloat indicator_h = container_w;
  CGFloat x = (container_w - indicator_w) / 2.0;
  CGFloat y = (container_h - indicator_h) / 2.0;

  [progress_bar setFrameRotation:0];
  [progress_bar setFrame:NSMakeRect(x, y, indicator_w, indicator_h)];
  [progress_bar setFrameCenterRotation:90.0];
}

static void cocoaProgressBarLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "VERTICAL"))
  {
    NSView* container_view = (NSView*)ih->handle;
    NSProgressIndicator* progress_bar = cocoaProgressBarGetProgressIndicator(ih);
    if (container_view && progress_bar)
      cocoaProgressBarUpdateVerticalLayout(container_view, progress_bar);
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
    NSView* container_view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, initial_width, initial_height)];

    [container_view addSubview:progress_indicator];
    [progress_indicator release];

    cocoaProgressBarUpdateVerticalLayout(container_view, progress_indicator);

    if (ih->userheight < ih->userwidth)
    {
      int tmp = ih->userheight;
      ih->userheight = ih->userwidth;
      ih->userwidth = tmp;
    }

    ih->handle = container_view;
    iupcocoaSetAssociatedViews(ih, progress_indicator, container_view);

    ih->expand = ih->expand & ~IUP_EXPAND_WIDTH;
  }
  else
  {
    [progress_indicator setAutoresizingMask:(NSViewWidthSizable)];

    ih->handle = progress_indicator;
    iupcocoaSetAssociatedViews(ih, progress_indicator, (NSView*)progress_indicator);

    ih->expand = ih->expand & ~IUP_EXPAND_HEIGHT;
  }

  iupcocoaAddToParent(ih);

  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvProgressBarInitClass(Iclass* ic)
{
  ic->Map = cocoaProgressBarMapMethod;
  ic->LayoutUpdate = cocoaProgressBarLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, iupdrvBaseSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "VALUE", iProgressBarGetValueAttrib, cocoaProgressBarSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARQUEE", NULL, cocoaProgressBarSetMarqueeAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DASHED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
