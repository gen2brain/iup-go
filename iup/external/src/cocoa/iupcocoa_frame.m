/** \file
 * \brief Frame Control
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_frame.h"

#include "iupcocoa_drv.h"


void iupdrvFrameGetDecorOffset(Ihandle* ih, int *x, int *y)
{
  /* NSBox automatically positions its contentView to accommodate border and title.
     Children are laid out relative to contentView, so offset is (0,0). */
  (void)ih;
  *x = 0;
  *y = 0;
}

int iupdrvFrameHasClientOffset(Ihandle* ih)
{
  (void)ih;
  return 0;
}

int iupdrvFrameGetTitleHeight(Ihandle* ih, int *h)
{
  if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE") || iupAttribGet(ih, "TITLE"))
  {
    if (ih->handle)
    {
      NSBox* the_frame = (NSBox*)ih->handle;
      NSFont* title_font = [the_frame titleFont];
      if (title_font)
      {
        CGFloat font_height = [title_font boundingRectForFont].size.height;
        *h = (int)(font_height + 4);  /* Add some padding */
      }
      else
      {
        *h = 16;  /* Default title height */
      }
    }
    else
    {
      *h = 16; /* Default/estimate if handle not created yet */
    }
  }
  else
  {
    *h = 0;
  }
  return 1;
}

int iupdrvFrameGetDecorSize(Ihandle* ih, int *w, int *h)
{
  NSBox* tempBox = [[[NSBox alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)] autorelease];

  [tempBox setBoxType:NSBoxPrimary];

  const char* title = iupAttribGet(ih, "TITLE");
  if (title && *title)
  {
    [tempBox setTitle:[NSString stringWithUTF8String:title]];
    [tempBox setTitlePosition:NSAtTop];
    [tempBox setBorderType:NSLineBorder];

    IupCocoaFont* iup_font = iupCocoaGetFont(ih);
    if (iup_font)
    {
      NSFont* font = [iup_font nativeFont];
      if (font)
        [tempBox setTitleFont:font];
    }
  }
  else
  {
    [tempBox setTitle:@""];
    [tempBox setTitlePosition:NSNoTitle];

    if (iupAttribGetBoolean(ih, "SUNKEN"))
      [tempBox setBorderType:NSGrooveBorder];
    else
      [tempBox setBorderType:NSLineBorder];
  }

  NSRect boxFrame = [tempBox frame];
  NSView* contentView = [tempBox contentView];
  NSRect contentFrame = [contentView frame];

  *w = (int)lroundf(boxFrame.size.width - contentFrame.size.width);
  *h = (int)lroundf(boxFrame.size.height - contentFrame.size.height);

  return 1;
}

static void* cocoaFrameGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
  (void)child;
  NSBox* the_frame = ih->handle;
  return [the_frame contentView];
}

static int cocoaFrameSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  if (!iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
    return 0;

  NSBox* the_frame = (NSBox*)ih->handle;

  if (value && *value)
  {
    NSString* ns_string = [NSString stringWithUTF8String:value];
    [the_frame setTitle:ns_string];
    [the_frame setTitlePosition:NSAtTop];
  }
  else
  {
    [the_frame setTitle:@""];
    [the_frame setTitlePosition:NSNoTitle];
  }

  iupdrvPostRedraw(ih);
  return 1;
}

static int cocoaFrameSetBgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;

  NSBox* the_frame = (NSBox*)ih->handle;
  NSView* content_view = [the_frame contentView];
  unsigned char r, g, b;

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
  {
    value = iupBaseNativeParentGetBgColor(ih);
  }

  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];

  if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
  {
    [the_frame setBoxType:NSBoxCustom];
    [the_frame setFillColor:color];
  }

  if (content_view)
  {
    if (![content_view wantsLayer])
    {
      [content_view setWantsLayer:YES];
    }

    if ([content_view layer])
    {
      [[content_view layer] setBackgroundColor:[color CGColor]];
    }
  }

  if (iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
    return 1;
  else
    return 0;
}

static int cocoaFrameSetFgColorAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 1;

  NSBox* the_frame = (NSBox*)ih->handle;
  NSString* title = [the_frame title];

  if (!title || [title length] == 0)
    return 0;

  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];

  NSTextFieldCell* title_cell = [the_frame titleCell];
  if (title_cell)
  {
    [title_cell setTextColor:color];
    iupdrvPostRedraw(ih);
    return 1;
  }

  return 0;
}

static int cocoaFrameSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
  {
    return 0;
  }

  if (ih->handle)
  {
    NSBox* the_frame = (NSBox*)ih->handle;
    IupCocoaFont* iup_font = iupCocoaGetFont(ih);
    if(iup_font)
    {
      NSFont* font = [iup_font nativeFont];
      if (font)
      {
        [the_frame setTitleFont:font];
        iupdrvPostRedraw(ih);
      }
    }
  }
  return 1;
}

static int cocoaFrameSetSunkenAttrib(Ihandle* ih, const char* value)
{
  /* SUNKEN only applies to frames without titles */
  if (iupAttribGetStr(ih, "_IUPFRAME_HAS_TITLE"))
    return 0;

  if (!ih->handle)
    return 1;

  NSBox* the_frame = (NSBox*)ih->handle;

  if (iupStrBoolean(value))
    [the_frame setBorderType:NSGrooveBorder];  /* Sunken appearance */
  else
    [the_frame setBorderType:NSLineBorder];    /* Flat line border */

  return 1;
}

static int cocoaFrameMapMethod(Ihandle* ih)
{
  char* title;

  if (!ih->parent)
  {
    return IUP_ERROR;
  }

  NSBox* the_frame = [[NSBox alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
  ih->handle = the_frame;

  [the_frame setBoxType:NSBoxPrimary];

  title = iupAttribGet(ih, "TITLE");
  if (title)
  {
    iupAttribSet(ih, "_IUPFRAME_HAS_TITLE", "1");
  }
  else
  {
    if (iupAttribGet(ih, "BGCOLOR") || iupAttribGet(ih, "BACKCOLOR"))
    {
      iupAttribSet(ih, "_IUPFRAME_HAS_BGCOLOR", "1");
    }
  }

  if (iupAttribGet(ih, "_IUPFRAME_HAS_TITLE"))
  {
    /* Show title at top with line border */
    if (title && *title)
    {
      [the_frame setTitle:[NSString stringWithUTF8String:title]];
    }
    else
    {
      [the_frame setTitle:@""];
    }
    [the_frame setTitlePosition:NSAtTop];
    [the_frame setBorderType:NSLineBorder];

    /* Apply font to title if specified */
    IupCocoaFont* iup_font = iupCocoaGetFont(ih);
    if (iup_font)
    {
      NSFont* font = [iup_font nativeFont];
      if (font)
      {
        [the_frame setTitleFont:font];
      }
    }
  }
  else
  {
    /* Box frame: no title, border depends on SUNKEN attribute */
    [the_frame setTitle:@""];
    [the_frame setTitlePosition:NSNoTitle];

    if (iupAttribGetBoolean(ih, "SUNKEN"))
      [the_frame setBorderType:NSGrooveBorder];
    else
      [the_frame setBorderType:NSLineBorder];
  }

  iupCocoaSetAssociatedViews(ih, [the_frame contentView], the_frame);
  iupCocoaAddToParent(ih);

  if (!iupAttribGet(ih, "_IUPFRAME_HAS_BGCOLOR"))
  {
    cocoaFrameSetBgColorAttrib(ih, NULL);
  }

  return IUP_NOERROR;
}

void iupdrvFrameInitClass(Iclass* ic)
{
  ic->Map = cocoaFrameMapMethod;
  ic->GetInnerNativeContainerHandle = cocoaFrameGetInnerNativeContainerHandleMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, cocoaFrameSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BGCOLOR", iupFrameGetBgColorAttrib, cocoaFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BACKCOLOR", iupFrameGetBgColorAttrib, cocoaFrameSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SUNKEN", NULL, cocoaFrameSetSunkenAttrib, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaFrameSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaFrameSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
}
