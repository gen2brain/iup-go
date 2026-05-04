/** \file
 * \brief Tooltips (iOS UIKit). Long-press on the view shows a popover with the tip text.
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_str.h"


static const void* IUPCOCOATOUCH_TIP_TEXT_KEY    = &IUPCOCOATOUCH_TIP_TEXT_KEY;
static const void* IUPCOCOATOUCH_TIP_GESTURE_KEY = &IUPCOCOATOUCH_TIP_GESTURE_KEY;

extern UIViewController* iupCocoaTouchFindTopPresentedViewController(void);


@interface IupCocoaTouchTipController : UIViewController <UIPopoverPresentationControllerDelegate>
@property(nonatomic, copy) NSString* tipText;
@end

@implementation IupCocoaTouchTipController

- (void)viewDidLoad
{
	[super viewDidLoad];

	UILabel* lbl = [[UILabel alloc] init];
	lbl.numberOfLines = 0;
	lbl.text = self.tipText;
	lbl.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
	lbl.textColor = [UIColor labelColor];
	lbl.translatesAutoresizingMaskIntoConstraints = NO;
	[self.view addSubview:lbl];

	const CGFloat hpad = 12.0, vpad = 8.0;
	[NSLayoutConstraint activateConstraints:@[
		[lbl.leadingAnchor  constraintEqualToAnchor:self.view.leadingAnchor  constant:hpad],
		[lbl.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-hpad],
		[lbl.topAnchor      constraintEqualToAnchor:self.view.topAnchor      constant:vpad],
		[lbl.bottomAnchor   constraintEqualToAnchor:self.view.bottomAnchor   constant:-vpad],
	]];

	CGFloat maxw = 280.0;
	CGSize size = [lbl sizeThatFits:CGSizeMake(maxw - 2*hpad, CGFLOAT_MAX)];
	self.preferredContentSize = CGSizeMake(size.width + 2*hpad, size.height + 2*vpad);
	[lbl release];
}

/* force popover on iPhone too; otherwise UIKit adapts to fullscreen */
- (UIModalPresentationStyle)adaptivePresentationStyleForPresentationController:(UIPresentationController*)controller traitCollection:(UITraitCollection*)traitCollection
{
	(void)controller; (void)traitCollection;
	return UIModalPresentationNone;
}

@end


static UIViewController* cocoaTouchTipFindHostVC(UIView* view)
{
	UIResponder* r = view;
	while (r && ![r isKindOfClass:[UIViewController class]]) r = [r nextResponder];
	UIViewController* host = (UIViewController*)r;
	while (host.presentedViewController) host = host.presentedViewController;
	if (!host) host = iupCocoaTouchFindTopPresentedViewController();
	return host;
}

static void cocoaTouchTipShow(UIView* view, NSString* text, CGPoint anchor)
{
	if (!view || text.length == 0) return;
	UIViewController* host = cocoaTouchTipFindHostVC(view);
	if (!host) return;

	IupCocoaTouchTipController* vc = [[IupCocoaTouchTipController alloc] init];
	vc.tipText = text;
	vc.modalPresentationStyle = UIModalPresentationPopover;

	UIPopoverPresentationController* ppc = vc.popoverPresentationController;
	ppc.sourceView = view;
	ppc.sourceRect = CGRectMake(anchor.x, anchor.y, 1.0, 1.0);
	ppc.permittedArrowDirections = 0;
	ppc.delegate = vc;

	[host presentViewController:vc animated:YES completion:nil];

	IupCocoaTouchTipController* weak_vc = vc;
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
		if (weak_vc.presentingViewController)
			[weak_vc dismissViewControllerAnimated:YES completion:nil];
	});

	[vc release];
}


@interface IupCocoaTouchTipGesture : UILongPressGestureRecognizer
@end

@implementation IupCocoaTouchTipGesture
@end


static void cocoaTouchTipOnLongPress(UILongPressGestureRecognizer* g)
{
	if (g.state != UIGestureRecognizerStateBegan) return;
	UIView* v = g.view;
	if (!v) return;
	NSString* text = objc_getAssociatedObject(v, IUPCOCOATOUCH_TIP_TEXT_KEY);
	if (text.length == 0) return;
	CGPoint p = [g locationInView:v];
	cocoaTouchTipShow(v, text, p);
}


@interface IupCocoaTouchTipTarget : NSObject
+ (instancetype)shared;
- (void)handle:(UILongPressGestureRecognizer*)g;
@end

@implementation IupCocoaTouchTipTarget
+ (instancetype)shared
{
	static IupCocoaTouchTipTarget* s;
	static dispatch_once_t once;
	dispatch_once(&once, ^{ s = [[self alloc] init]; });
	return s;
}
- (void)handle:(UILongPressGestureRecognizer*)g { cocoaTouchTipOnLongPress(g); }
@end


IUP_SDK_API int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
	id handle = ih ? (id)ih->handle : nil;
	if (![handle isKindOfClass:[UIView class]])
	{
		if ([handle respondsToSelector:@selector(setAccessibilityHint:)])
			[handle setAccessibilityHint:(value && *value) ? [NSString stringWithUTF8String:value] : nil];
		return 1;
	}

	UIView* view = (UIView*)handle;
	NSString* text = (value && *value) ? [NSString stringWithUTF8String:value] : nil;
	objc_setAssociatedObject(view, IUPCOCOATOUCH_TIP_TEXT_KEY, text, OBJC_ASSOCIATION_COPY_NONATOMIC);
	[view setAccessibilityHint:text];

	IupCocoaTouchTipGesture* existing = objc_getAssociatedObject(view, IUPCOCOATOUCH_TIP_GESTURE_KEY);
	if (text && !existing)
	{
		IupCocoaTouchTipGesture* g = [[IupCocoaTouchTipGesture alloc] initWithTarget:[IupCocoaTouchTipTarget shared] action:@selector(handle:)];
		g.minimumPressDuration = 0.5;
		[view addGestureRecognizer:g];
		objc_setAssociatedObject(view, IUPCOCOATOUCH_TIP_GESTURE_KEY, g, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
		[g release];
	}
	else if (!text && existing)
	{
		[view removeGestureRecognizer:existing];
		objc_setAssociatedObject(view, IUPCOCOATOUCH_TIP_GESTURE_KEY, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	}
	return 1;
}

IUP_SDK_API int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
	if (!ih || !ih->handle) return 0;
	id handle = (id)ih->handle;
	if (![handle isKindOfClass:[UIView class]]) return 0;
	if (!iupStrBoolean(value)) return 0;

	UIView* view = (UIView*)handle;
	NSString* text = objc_getAssociatedObject(view, IUPCOCOATOUCH_TIP_TEXT_KEY);
	if (text.length == 0) return 0;
	cocoaTouchTipShow(view, text, CGPointMake(view.bounds.size.width * 0.5, view.bounds.size.height * 0.5));
	return 0;
}

IUP_SDK_API char* iupdrvBaseGetTipVisibleAttrib(Ihandle* ih)
{
	(void)ih;
	return NULL;
}
