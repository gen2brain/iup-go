/** \file
 * \brief IupPopover (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <stdlib.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_layout.h"
#include "iup_popover.h"

#include "iupcocoatouch_drv.h"


static const void* IUPCOCOATOUCH_POPOVER_DELEGATE_KEY = @"IUPCOCOATOUCH_POPOVER_DELEGATE";

@interface IupCocoaTouchPopoverVC : UIViewController
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, retain) IupCocoaTouchFixed* clientArea;
@end

@implementation IupCocoaTouchPopoverVC

- (void)loadView
{
	CGRect initial = CGRectMake(0, 0, 200, 200);
	if (_ihandle && _ihandle->currentwidth > 0 && _ihandle->currentheight > 0)
		initial = CGRectMake(0, 0, _ihandle->currentwidth, _ihandle->currentheight);
	IupCocoaTouchFixed* fixed = [[IupCocoaTouchFixed alloc] initWithFrame:initial];
	fixed.ihandle = _ihandle;
	fixed.backgroundColor = [UIColor clearColor];
	self.view = fixed;
	self.clientArea = fixed;
	[fixed release];
}

- (void)viewDidLayoutSubviews
{
	[super viewDidLayoutSubviews];
	if (!_ihandle || !_ihandle->firstchild) return;
	UIEdgeInsets sa = self.view.safeAreaInsets;
	_ihandle->iclass->SetChildrenPosition(_ihandle, (int)sa.left, (int)sa.top);
	iupLayoutUpdate(_ihandle->firstchild);
}

- (void)dealloc
{
	[_clientArea release];
	[super dealloc];
}

@end


@interface IupCocoaTouchPopoverDelegate : NSObject <UIPopoverPresentationControllerDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTouchPopoverDelegate

- (UIModalPresentationStyle)adaptivePresentationStyleForPresentationController:(UIPresentationController*)controller
                                                       traitCollection:(UITraitCollection*)traitCollection
{
	(void)controller; (void)traitCollection;
	return UIModalPresentationNone;
}

- (BOOL)presentationControllerShouldDismiss:(UIPresentationController*)presentationController
{
	(void)presentationController;
	return _ihandle ? (iupAttribGetBoolean(_ihandle, "AUTOHIDE") ? YES : NO) : YES;
}

- (void)presentationControllerDidDismiss:(UIPresentationController*)presentationController
{
	(void)presentationController;
	if (!_ihandle) return;
	IFni cb = (IFni)IupGetCallback(_ihandle, "SHOW_CB");
	if (cb) cb(_ihandle, IUP_HIDE);
}

@end


static IupCocoaTouchPopoverVC* cocoaTouchPopoverVC(Ihandle* ih)
{
	if (!ih || !ih->handle) return nil;
	id h = ih->handle;
	if (![h isKindOfClass:[IupCocoaTouchPopoverVC class]]) return nil;
	return (IupCocoaTouchPopoverVC*)h;
}

static UIView* cocoaTouchPopoverAnchorView(Ihandle* anchor_ih)
{
	if (!anchor_ih || !anchor_ih->handle) return nil;
	id h = anchor_ih->handle;
	if ([h isKindOfClass:[UIView class]]) return (UIView*)h;
	if ([h isKindOfClass:[UIViewController class]]) return [(UIViewController*)h view];
	return nil;
}

static UIPopoverArrowDirection cocoaTouchPopoverArrowDirection(int position)
{
	switch (position)
	{
		case IUP_POPOVER_TOP:
		case IUP_POPOVER_TOPLEFT:
		case IUP_POPOVER_TOPRIGHT:
			return UIPopoverArrowDirectionDown;
		case IUP_POPOVER_LEFT:
		case IUP_POPOVER_LEFTTOP:
		case IUP_POPOVER_LEFTBOTTOM:
			return UIPopoverArrowDirectionRight;
		case IUP_POPOVER_RIGHT:
		case IUP_POPOVER_RIGHTTOP:
		case IUP_POPOVER_RIGHTBOTTOM:
			return UIPopoverArrowDirectionLeft;
		case IUP_POPOVER_BOTTOM:
		case IUP_POPOVER_BOTTOMLEFT:
		case IUP_POPOVER_BOTTOMRIGHT:
		default:
			return UIPopoverArrowDirectionUp;
	}
}

static CGRect cocoaTouchPopoverSourceRect(int position, CGRect bounds)
{
	/* corner-bias variants narrow the anchor edge so the arrow pins to a corner */
	switch (position)
	{
		case IUP_POPOVER_BOTTOMLEFT:
		case IUP_POPOVER_TOPLEFT:
			return CGRectMake(0, 0, 1, bounds.size.height);
		case IUP_POPOVER_BOTTOMRIGHT:
		case IUP_POPOVER_TOPRIGHT:
			return CGRectMake(bounds.size.width - 1, 0, 1, bounds.size.height);
		case IUP_POPOVER_LEFTTOP:
		case IUP_POPOVER_RIGHTTOP:
			return CGRectMake(0, 0, bounds.size.width, 1);
		case IUP_POPOVER_LEFTBOTTOM:
		case IUP_POPOVER_RIGHTBOTTOM:
			return CGRectMake(0, bounds.size.height - 1, bounds.size.width, 1);
		default:
			return bounds;
	}
}

static int cocoaTouchPopoverSetVisibleAttrib(Ihandle* ih, const char* value)
{
	if (iupStrBoolean(value))
	{
		Ihandle* anchor = (Ihandle*)iupAttribGet(ih, "_IUP_POPOVER_ANCHOR");
		UIView* anchor_view = cocoaTouchPopoverAnchorView(anchor);
		if (!anchor_view) return 0;

		if (!ih->handle)
		{
			if (IupMap(ih) == IUP_ERROR) return 0;
		}
		IupCocoaTouchPopoverVC* vc = cocoaTouchPopoverVC(ih);
		if (!vc) return 0;

		if (ih->firstchild)
		{
			iupLayoutCompute(ih);
			if (vc.clientArea)
				vc.clientArea.frame = CGRectMake(0, 0, ih->currentwidth, ih->currentheight);
			vc.preferredContentSize = CGSizeMake(ih->currentwidth, ih->currentheight);
			iupLayoutUpdate(ih);
		}

		int position = iupPopoverGetPosition(ih);
		UIPopoverPresentationController* ppc = [vc popoverPresentationController];
		if (ppc)
		{
			ppc.sourceView = anchor_view;
			ppc.sourceRect = cocoaTouchPopoverSourceRect(position, [anchor_view bounds]);
			ppc.permittedArrowDirections = cocoaTouchPopoverArrowDirection(position);
			ppc.passthroughViews = @[anchor_view];
			ppc.backgroundColor = [UIColor systemBackgroundColor];
			IupCocoaTouchPopoverDelegate* delegate = objc_getAssociatedObject(vc, IUPCOCOATOUCH_POPOVER_DELEGATE_KEY);
			ppc.delegate = delegate;
		}

		UIViewController* host = iupCocoaTouchFindTopPresentedViewController();
		if (!host) host = iupCocoaTouchFindCurrentRootViewController();
		if (!host) return 0;

		iupAttribSet(ih, "_IUPCOCOATOUCH_POPOVER_VISIBLE", "1");
		[host presentViewController:vc animated:YES completion:^{
			IFni cb = (IFni)IupGetCallback(ih, "SHOW_CB");
			if (cb) cb(ih, IUP_SHOW);
		}];
	}
	else
	{
		IupCocoaTouchPopoverVC* vc = cocoaTouchPopoverVC(ih);
		if (vc && vc.presentingViewController)
		{
			[vc dismissViewControllerAnimated:YES completion:^{
				iupAttribSet(ih, "_IUPCOCOATOUCH_POPOVER_VISIBLE", NULL);
				IFni cb = (IFni)IupGetCallback(ih, "SHOW_CB");
				if (cb) cb(ih, IUP_HIDE);
			}];
		}
	}
	return 0;
}

static char* cocoaTouchPopoverGetVisibleAttrib(Ihandle* ih)
{
	IupCocoaTouchPopoverVC* vc = cocoaTouchPopoverVC(ih);
	if (!vc) return "NO";
	return iupStrReturnBoolean(vc.presentingViewController != nil);
}

static void cocoaTouchPopoverLayoutUpdateMethod(Ihandle* ih)
{
	if (ih->firstchild)
	{
		ih->iclass->SetChildrenPosition(ih, 0, 0);
		iupLayoutUpdate(ih->firstchild);
	}
}

static void* cocoaTouchPopoverGetInnerNativeContainerHandleMethod(Ihandle* ih, Ihandle* child)
{
	(void)child;
	IupCocoaTouchPopoverVC* vc = cocoaTouchPopoverVC(ih);
	return vc ? (void*)vc.clientArea : NULL;
}

static int cocoaTouchPopoverMapMethod(Ihandle* ih)
{
	IupCocoaTouchPopoverVC* vc = [[IupCocoaTouchPopoverVC alloc] init];
	vc.ihandle = ih;
	vc.modalPresentationStyle = UIModalPresentationPopover;
	ih->handle = vc;

	IupCocoaTouchPopoverDelegate* delegate = [[IupCocoaTouchPopoverDelegate alloc] init];
	delegate.ihandle = ih;
	objc_setAssociatedObject(vc, IUPCOCOATOUCH_POPOVER_DELEGATE_KEY, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[delegate release];

	objc_setAssociatedObject(vc, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

	(void)vc.view;

	return IUP_NOERROR;
}

static void cocoaTouchPopoverUnMapMethod(Ihandle* ih)
{
	IupCocoaTouchPopoverVC* vc = cocoaTouchPopoverVC(ih);
	if (!vc) return;

	if (vc.presentingViewController)
		[vc dismissViewControllerAnimated:NO completion:nil];

	[vc popoverPresentationController].delegate = nil;
	objc_setAssociatedObject(vc, IUPCOCOATOUCH_POPOVER_DELEGATE_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

	vc.ihandle = NULL;
	[vc release];
	ih->handle = NULL;
}

IUP_SDK_API void iupdrvPopoverInitClass(Iclass* ic)
{
	ic->Map = cocoaTouchPopoverMapMethod;
	ic->UnMap = cocoaTouchPopoverUnMapMethod;
	ic->LayoutUpdate = cocoaTouchPopoverLayoutUpdateMethod;
	ic->GetInnerNativeContainerHandle = cocoaTouchPopoverGetInnerNativeContainerHandleMethod;

	iupClassRegisterAttribute(ic, "VISIBLE", cocoaTouchPopoverGetVisibleAttrib, cocoaTouchPopoverSetVisibleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "ARROW", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "OFFSETX", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "OFFSETY", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
