/** \file
 * \brief UIViewController hosting one IupDialog.
 *
 * See Copyright Notice in "iup.h"
 */

#import "IupViewController.h"

#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"

#include "iupcocoatouch_drv.h"


@implementation IupViewController

- (instancetype)init
{
	self = [super init];
	if (self)
	{
		_allowedOrientations = UIInterfaceOrientationMaskAll;
	}
	return self;
}

- (void)loadView
{
	UIView* root = [[UIView alloc] initWithFrame:CGRectZero];
	[root setBackgroundColor:[UIColor systemBackgroundColor]];
	self.view = root;
	[root release];

	_scrollView = [[UIScrollView alloc] initWithFrame:CGRectZero];
	/* no rubber-band when content fits; bounce-pan would steal canvas drags */
	_scrollView.alwaysBounceVertical = NO;
	_scrollView.showsVerticalScrollIndicator = YES;
	_scrollView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
	/* default 150ms hold made buttons feel like they needed a double-tap */
	_scrollView.delaysContentTouches = NO;
	/* swipe-down dismisses the keyboard so focusing Text on map doesn't trap the user */
	_scrollView.keyboardDismissMode = UIScrollViewKeyboardDismissModeInteractive;
	[root addSubview:_scrollView];

	_clientArea = [[IupCocoaTouchFixed alloc] initWithFrame:CGRectZero];
	[_clientArea setIhandle:_ihandle];
	[_scrollView addSubview:_clientArea];
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
	[nc addObserver:self selector:@selector(keyboardWillChangeFrame:) name:UIKeyboardWillChangeFrameNotification object:nil];
	[nc addObserver:self selector:@selector(keyboardWillHide:)        name:UIKeyboardWillHideNotification        object:nil];
}

/* keyboard overlap as bottom inset, so interactive dismiss engages when content otherwise fits */
- (void)keyboardWillChangeFrame:(NSNotification*)note
{
	CGRect kb_screen = [note.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
	CGRect kb_view   = [self.view convertRect:kb_screen fromView:nil];
	CGRect sv_view   = [self.view convertRect:_scrollView.bounds fromView:_scrollView];
	CGFloat overlap  = CGRectGetMaxY(sv_view) - CGRectGetMinY(kb_view);
	if (overlap < 0) overlap = 0;
	UIEdgeInsets inset = _scrollView.contentInset;
	inset.bottom = overlap;
	_scrollView.contentInset = inset;
	_scrollView.verticalScrollIndicatorInsets = inset;
}

- (void)keyboardWillHide:(NSNotification*)note
{
	(void)note;
	UIEdgeInsets inset = _scrollView.contentInset;
	inset.bottom = 0;
	_scrollView.contentInset = inset;
	_scrollView.verticalScrollIndicatorInsets = inset;
}

- (void)viewDidLayoutSubviews
{
	[super viewDidLayoutSubviews];

	CGRect safe = self.view.safeAreaLayoutGuide.layoutFrame;
	[_scrollView setFrame:safe];

	/* width clamps to viewport; height grows past it so the scrollView pans */
	int viewport_w = iupROUND(safe.size.width);
	int viewport_h = iupROUND(safe.size.height);
	if (_ihandle)
	{
		int target_h = viewport_h;
		if (_ihandle->naturalheight > target_h) target_h = _ihandle->naturalheight;

		if (viewport_w != _ihandle->currentwidth || target_h != _ihandle->currentheight)
		{
			_ihandle->currentwidth  = viewport_w;
			_ihandle->currentheight = target_h;
			/* RESIZE_CB reports the visible client area, not the scroll-content height. */
			IFnii cb = (IFnii)IupGetCallback(_ihandle, "RESIZE_CB");
			if (!cb || cb(_ihandle, viewport_w, viewport_h) != IUP_IGNORE)
				IupRefresh(_ihandle);
		}
	}

	CGFloat content_w = safe.size.width;
	CGFloat content_h = safe.size.height;
	for (UIView* child in _clientArea.subviews)
	{
		CGFloat bottom = CGRectGetMaxY(child.frame);
		if (bottom > content_h) content_h = bottom;
	}

	CGRect client_frame = CGRectMake(0, 0, content_w, content_h);
	if (!CGRectEqualToRect(_clientArea.frame, client_frame))
		[_clientArea setFrame:client_frame];

	CGSize content_size = CGSizeMake(content_w, content_h);
	if (!CGSizeEqualToSize(_scrollView.contentSize, content_size))
		[_scrollView setContentSize:content_size];
}

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection
{
	[super traitCollectionDidChange:previousTraitCollection];
	if (self.traitCollection.userInterfaceStyle != previousTraitCollection.userInterfaceStyle)
		iupCocoaTouchHandleTraitFlip();
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
	[super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];

	[coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
		{
			(void)context;
			if (_ihandle)
			{
				_ihandle->naturalwidth  = iupROUND(size.width);
				_ihandle->naturalheight = iupROUND(size.height);
			}
		}
		completion:nil];
}

- (BOOL)prefersStatusBarHidden
{
	return _hideStatusBar;
}

- (BOOL)prefersHomeIndicatorAutoHidden
{
	return _hideHomeIndicator;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
	return _allowedOrientations ? _allowedOrientations : UIInterfaceOrientationMaskAll;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_clientArea release];
	[_scrollView release];
	[super dealloc];
}

@end
