/** \file
 * \brief Focus management (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include <stdlib.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_focus.h"
#include "iup_drv.h"

#include "iupcocoatouch_drv.h"

#import "IupViewController.h"


static UIView* cocoaTouchFocusViewFromHandle(Ihandle* ih)
{
	/* TYPEVOID containers carry sentinel (void*)-1; don't message-send it */
	if (!ih || !ih->handle || ih->handle == (void*)-1) return nil;
	id h = ih->handle;
	if ([h isKindOfClass:[UIView class]]) return (UIView*)h;
	if ([h isKindOfClass:[IupViewController class]])
	{
		IupCocoaTouchFixed* fixed = [(IupViewController*)h clientArea];
		if (fixed) return fixed;
		return [(UIViewController*)h view];
	}
	if ([h isKindOfClass:[UIViewController class]]) return [(UIViewController*)h view];
	return nil;
}

/* DFS for first CANFOCUS descendant whose view can become first responder */
static Ihandle* cocoaTouchFocusFirstFocusable(Ihandle* ih)
{
	if (!ih) return NULL;
	for (Ihandle* child = ih->firstchild; child; child = child->brother)
	{
		/* skip null and the TYPEVOID sentinel; recurse through them */
		if (!child->handle || child->handle == (void*)-1)
		{
			Ihandle* nested = cocoaTouchFocusFirstFocusable(child);
			if (nested) return nested;
			continue;
		}

		const char* can = iupAttribGet(child, "_IUPCOCOATOUCH_CANFOCUS");
		if (can && !iupStrBoolean(can)) { /* CANFOCUS=NO */ }
		else
		{
			UIView* v = cocoaTouchFocusViewFromHandle(child);
			if (v && [v canBecomeFirstResponder]) return child;
		}

		Ihandle* nested = cocoaTouchFocusFirstFocusable(child);
		if (nested) return nested;
	}
	return NULL;
}

IUP_SDK_API void iupdrvSetFocus(Ihandle* ih)
{
	if (!ih) return;

	UIView* view = cocoaTouchFocusViewFromHandle(ih);
	if (!view) return;

	if (iupAttribGet(ih, "_IUPCOCOATOUCH_CANFOCUS")
	    && !iupAttribGetBoolean(ih, "_IUPCOCOATOUCH_CANFOCUS"))
		return;

	if ([view isFirstResponder]) return;

	if (![view canBecomeFirstResponder])
	{
		/* dialogs/containers delegate to first focusable descendant */
		Ihandle* next = cocoaTouchFocusFirstFocusable(ih);
		if (next && next != ih) iupdrvSetFocus(next);
		return;
	}

	if ([view becomeFirstResponder])
		iupCallGetFocusCb(ih);
}
