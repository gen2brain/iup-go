/** \file
 * \brief IupPrintDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"

#include "iupcocoa_drv.h"

#ifndef GNUSTEP

#define IUPCOCOA_PRINT_SCALE (96.0 / 72.0)

@interface IupCocoaPrintView : NSView
{
@public
  Ihandle* ih;
  NSSize page_size;
  int page_width, page_height;
  BOOL canceled;
}
@end

@implementation IupCocoaPrintView

- (BOOL)isFlipped
{
  return YES;
}

- (BOOL)knowsPageRange:(NSRangePointer)range
{
  NSPrintInfo* info = [[NSPrintOperation currentOperation] printInfo];
  NSRect bounds = [info imageablePageBounds];
  NSSize paper = [info paperSize];
  int count;

  [info setLeftMargin:bounds.origin.x];
  [info setBottomMargin:bounds.origin.y];
  [info setRightMargin:paper.width - NSMaxX(bounds)];
  [info setTopMargin:paper.height - NSMaxY(bounds)];

  page_size = bounds.size;
  page_width = iupRound(page_size.width * IUPCOCOA_PRINT_SCALE);
  page_height = iupRound(page_size.height * IUPCOCOA_PRINT_SCALE);

  count = iupPrintDlgPaginate(ih, page_width, page_height);
  [self setFrame:NSMakeRect(0, 0, page_size.width, page_size.height * count)];

  range->location = 1;
  range->length = count;
  return YES;
}

- (NSRect)rectForPage:(NSInteger)page
{
  return NSMakeRect(0, (page - 1) * page_size.height, page_size.width, page_size.height);
}

- (void)drawRect:(NSRect)dirtyRect
{
  NSPrintOperation* op = [NSPrintOperation currentOperation];
  NSInteger page;
  CGContextRef cg;

  (void)dirtyRect;
  if (!op || canceled)
    return;

  page = [op currentPage];
  cg = [[NSGraphicsContext currentContext] CGContext];

  CGContextSaveGState(cg);
  CGContextTranslateCTM(cg, 0, (page - 1) * page_size.height);
  CGContextScaleCTM(cg, 1.0 / IUPCOCOA_PRINT_SCALE, 1.0 / IUPCOCOA_PRINT_SCALE);

  if (iupPrintDlgDrawPage(ih, cg, (int)page, page_width, page_height) == IUP_CLOSE)
  {
    canceled = YES;
    [[op printInfo] setJobDisposition:NSPrintCancelJob];
  }

  CGContextRestoreGState(cg);
}

@end

static int cocoaPrintDlgPopup(Ihandle* ih, int x, int y)
{
  @autoreleasepool
  {
    NSPrintInfo* info = [[[NSPrintInfo sharedPrintInfo] copy] autorelease];
    IupCocoaPrintView* view;
    NSPrintOperation* op;
    NSPrintPanel* panel;
    BOOL ok;

    iupAttribSetInt(ih, "_IUPDLG_X", x);
    iupAttribSetInt(ih, "_IUPDLG_Y", y);

    if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "LANDSCAPE"))
      [info setOrientation:NSPaperOrientationLandscape];
    else
      [info setOrientation:NSPaperOrientationPortrait];
    [info setHorizontallyCentered:NO];
    [info setVerticallyCentered:NO];

    view = [[IupCocoaPrintView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];
    view->ih = ih;

    op = [NSPrintOperation printOperationWithView:view printInfo:info];
    [op setJobTitle:[NSString stringWithUTF8String:iupPrintDlgGetJobName(ih)]];
    [op setShowsPrintPanel:YES];
    [op setShowsProgressPanel:YES];

    panel = [op printPanel];
    [panel setOptions:[panel options] | NSPrintPanelShowsCopies | NSPrintPanelShowsPageRange |
                      NSPrintPanelShowsPaperSize | NSPrintPanelShowsOrientation | NSPrintPanelShowsPreview];

    ok = [op runOperation];

    if ([[op printInfo] orientation] == NSPaperOrientationLandscape)
      iupAttribSet(ih, "ORIENTATION", "LANDSCAPE");
    else
      iupAttribSet(ih, "ORIENTATION", "PORTRAIT");

    iupAttribSet(ih, "STATUS", (ok && !view->canceled) ? "1" : "0");

    [view release];
  }

  return IUP_NOERROR;
}

#endif

IUP_SDK_API void iupdrvPrintDlgInitClass(Iclass* ic)
{
#ifndef GNUSTEP
  ic->DlgPopup = cocoaPrintDlgPopup;
#else
  (void)ic;
#endif
}
