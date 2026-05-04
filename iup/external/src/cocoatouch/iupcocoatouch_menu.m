/** \file
 * \brief Menu / MenuItem / Submenu / Separator (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_drv.h"
#include "iup_menu.h"

#include "iupcocoatouch_drv.h"
#import "IupViewController.h"


static UIView* cocoaTouchMenuCreateMarker(void)
{
	UIView* v = [[UIView alloc] initWithFrame:CGRectZero];
	[v setHidden:YES];
	return v;
}


static NSString* cocoaTouchMenuItemDisplayTitle(Ihandle* ih)
{
	const char* raw = iupAttribGet(ih, "TITLE");
	if (!raw || !*raw) return @"";

	char* with_mnem = iupMenuProcessTitle(ih, raw);
	char dummy = 0;
	char* stripped = iupStrProcessMnemonic(with_mnem, &dummy, 0);
	const char* src = stripped ? stripped : (with_mnem ? with_mnem : raw);

	const char* tab = strchr(src, '\t');
	NSString* result = tab
		? [[[NSString alloc] initWithBytes:src length:(NSUInteger)(tab - src) encoding:NSUTF8StringEncoding] autorelease]
		: [NSString stringWithUTF8String:src];

	if (stripped && stripped != with_mnem && stripped != raw) free(stripped);
	if (with_mnem && with_mnem != raw) free(with_mnem);
	return result ? result : @"";
}


static void cocoaTouchMenuApplyRadioGroup(Ihandle* item_ih)
{
	Ihandle* menu = item_ih ? item_ih->parent : NULL;
	if (!menu || !iupAttribGetBoolean(menu, "RADIO")) return;

	int count = IupGetChildCount(menu);
	for (int i = 0; i < count; i++)
	{
		Ihandle* sib = IupGetChild(menu, i);
		if (sib == item_ih || !sib || !sib->iclass) continue;
		if (!iupStrEqual(sib->iclass->name, "menuitem")) continue;
		iupAttribSetStr(sib, "VALUE", "OFF");
	}
	iupAttribSetStr(item_ih, "VALUE", "ON");
}


static void cocoaTouchMenuToggleState(Ihandle* item_ih)
{
	Ihandle* menu = item_ih ? item_ih->parent : NULL;
	int radio = menu && iupAttribGetBoolean(menu, "RADIO");
	int autotoggle = iupAttribGetBoolean(item_ih, "AUTOTOGGLE");

	if (radio)
		cocoaTouchMenuApplyRadioGroup(item_ih);
	else if (autotoggle)
	{
		const char* cur = iupAttribGet(item_ih, "VALUE");
		iupAttribSetStr(item_ih, "VALUE",
			iupStrEqualNoCase(cur, "ON") ? "OFF" : "ON");
	}
}

/* UIMenu fires the handler post-dismiss, so cb can present + pump synchronously */
static void cocoaTouchMenuFireActionDirect(Ihandle* item_ih)
{
	if (!item_ih || !iupObjectCheck(item_ih)) return;
	cocoaTouchMenuToggleState(item_ih);
	Icallback cb = IupGetCallback(item_ih, "ACTION");
	if (cb && cb(item_ih) == IUP_CLOSE) IupExitLoop();
}

/* fired by action-sheet button tap; flips the pump's done flag */
static void cocoaTouchMenuFireActionAfterDismiss(Ihandle* item_ih)
{
	Ihandle* menu_ih = item_ih ? item_ih->parent : NULL;
	if (item_ih && iupObjectCheck(item_ih))
	{
		cocoaTouchMenuToggleState(item_ih);
		Icallback cb = IupGetCallback(item_ih, "ACTION");
		if (cb) cb(item_ih);
	}
	if (menu_ih && iupObjectCheck(menu_ih))
	{
		BOOL* done_ptr = (BOOL*)iupAttribGet(menu_ih, "_IUPCOCOA_MENU_DONE_PTR");
		if (done_ptr) *done_ptr = YES;
	}
}

static void cocoaTouchMenuFireOpen(Ihandle* menu_ih)
{
	if (!menu_ih || !iupObjectCheck(menu_ih)) return;
	Icallback cb = IupGetCallback(menu_ih, "OPEN_CB");
	if (cb && cb(menu_ih) == IUP_CLOSE) IupExitLoop();
}

static void cocoaTouchMenuFireClose(Ihandle* menu_ih)
{
	if (!menu_ih || !iupObjectCheck(menu_ih)) return;
	Icallback cb = IupGetCallback(menu_ih, "MENUCLOSE_CB");
	if (cb && cb(menu_ih) == IUP_CLOSE) IupExitLoop();
}


static UIViewController* cocoaTouchMenuHostViewController(Ihandle* menu_ih)
{
	Ihandle* cur = menu_ih;
	while (cur)
	{
		if ([(id)cur->handle isKindOfClass:[UIViewController class]])
		{
			return (UIViewController*)cur->handle;
		}
		cur = cur->parent;
	}
	return iupCocoaTouchFindTopPresentedViewController();
}

static UIImage* cocoaTouchMenuResolveImage(Ihandle* item_ih)
{
	const char* name = iupAttribGet(item_ih, "IMAGE");
	if (!name) name = iupAttribGet(item_ih, "TITLEIMAGE");
	if (!name) return nil;
	return (UIImage*)iupImageGetImage(name, item_ih, 0, NULL);
}


static UIMenu* cocoaTouchMenuBuildUIMenu(Ihandle* menu_ih);

static UIMenuElement* cocoaTouchMenuBuildChildElement(Ihandle* child)
{
	if (!child || !child->iclass || !child->iclass->name) return nil;
	const char* cname = child->iclass->name;

	if (iupStrEqual(cname, "menuseparator"))
	{
		/* empty displayInline section renders as a divider */
		return [UIMenu menuWithTitle:@""
		                       image:nil
		                  identifier:nil
		                     options:UIMenuOptionsDisplayInline
		                    children:@[]];
	}

	if (iupStrEqual(cname, "submenu"))
	{
		Ihandle* sub = IupGetChild(child, 0);
		if (!sub) return nil;
		UIMenu* nested = cocoaTouchMenuBuildUIMenu(sub);
		if (!nested) return nil;
		NSString* title = cocoaTouchMenuItemDisplayTitle(child);
		/* seed a disabled placeholder so empty submenus don't get hidden */
		NSArray<UIMenuElement*>* children = nested.children;
		if (children.count == 0)
		{
			UIAction* empty = [UIAction actionWithTitle:@"(empty)" image:nil identifier:nil
			                                    handler:^(UIAction* a) { (void)a; }];
			empty.attributes = UIMenuElementAttributesDisabled;
			children = @[empty];
		}
		return [UIMenu menuWithTitle:title
		                       image:cocoaTouchMenuResolveImage(child)
		                  identifier:nil
		                     options:0
		                    children:children];
	}

	if (iupStrEqual(cname, "menuitem"))
	{
		NSString* title = cocoaTouchMenuItemDisplayTitle(child);
		UIImage* image = cocoaTouchMenuResolveImage(child);
		Ihandle* child_capture = child;
		UIAction* action = [UIAction actionWithTitle:title
		                                       image:image
		                                  identifier:nil
		                                     handler:^(UIAction* a) {
			(void)a;
			cocoaTouchMenuFireActionDirect(child_capture);
		}];
		if (iupAttribGet(child, "ACTIVE") && !iupAttribGetBoolean(child, "ACTIVE"))
		{
			action.attributes = UIMenuElementAttributesDisabled;
		}
		const char* value = iupAttribGet(child, "VALUE");
		if (value && iupStrEqualNoCase(value, "ON") && !iupAttribGetBoolean(child, "HIDEMARK"))
		{
			action.state = UIMenuElementStateOn;
		}
		return action;
	}

	return nil;
}

/* recent entries live as menu attribs, not children; render inline */
static void cocoaTouchMenuAppendRecentUIActions(NSMutableArray<UIMenuElement*>* elements, Ihandle* menu_ih)
{
	int count = iupAttribGetInt(menu_ih, "_IUP_RECENT_COUNT");
	if (count <= 0) return;

	Icallback cb = (Icallback)iupAttribGet(menu_ih, "_IUP_RECENT_CB");

	for (int i = 0; i < count; i++)
	{
		char attr[32];
		snprintf(attr, sizeof(attr), "_IUP_RECENT_FILE%d", i);
		const char* path = iupAttribGet(menu_ih, attr);
		if (!path || !*path) continue;

		NSString* title = [NSString stringWithUTF8String:path];
		Ihandle* menu_capture = menu_ih;
		int index_capture = i;
		Icallback cb_capture = cb;

		UIAction* action = [UIAction actionWithTitle:title
		                                       image:nil
		                                  identifier:nil
		                                     handler:^(UIAction* a) {
			(void)a;
			if (!iupObjectCheck(menu_capture)) return;
			char slot[32];
			snprintf(slot, sizeof(slot), "_IUP_RECENT_FILE%d", index_capture);
			iupAttribSetStr(menu_capture, "TITLE", iupAttribGet(menu_capture, slot));
			if (cb_capture && cb_capture(menu_capture) == IUP_CLOSE) IupExitLoop();
		}];
		[elements addObject:action];
	}
}

static UIMenu* cocoaTouchMenuBuildUIMenu(Ihandle* menu_ih)
{
	if (!menu_ih) return nil;

	NSMutableArray<UIMenuElement*>* elements = [NSMutableArray array];
	cocoaTouchMenuAppendRecentUIActions(elements, menu_ih);
	int count = IupGetChildCount(menu_ih);
	for (int i = 0; i < count; i++)
	{
		UIMenuElement* element = cocoaTouchMenuBuildChildElement(IupGetChild(menu_ih, i));
		if (element) [elements addObject:element];
	}

	return [UIMenu menuWithTitle:@""
	                       image:nil
	                  identifier:nil
	                     options:0
	                    children:elements];
}


/* custom action sheet; UIAlertController doesn't survive a nested CFRunLoop pump */
@interface IupCocoaTouchActionSheetView : UIView <UIGestureRecognizerDelegate>
@property(nonatomic, copy) void (^onPick)(int index);
@property(nonatomic, copy) void (^onCancel)(void);
@property(nonatomic, retain) UIView* itemsCard;
@property(nonatomic, retain) UIView* cancelCard;
@end

@implementation IupCocoaTouchActionSheetView

static UIButton* iupCocoaTouchMakeRow(NSString* title, BOOL bold, BOOL enabled, UIImage* image, NSInteger tag, id target, SEL action)
{
	UIButtonConfiguration* cfg = [UIButtonConfiguration plainButtonConfiguration];
	NSDictionary* attrs = @{ NSFontAttributeName: bold ? [UIFont systemFontOfSize:18 weight:UIFontWeightSemibold]
	                                                   : [UIFont systemFontOfSize:18] };
	cfg.attributedTitle = [[[NSAttributedString alloc] initWithString:title attributes:attrs] autorelease];
	if (image)
	{
		cfg.image = image;
		cfg.imagePlacement = NSDirectionalRectEdgeLeading;
		cfg.imagePadding = 8;
	}
	cfg.contentInsets = NSDirectionalEdgeInsetsMake(14, 16, 14, 16);
	UIButton* btn = [UIButton buttonWithConfiguration:cfg primaryAction:nil];
	btn.translatesAutoresizingMaskIntoConstraints = NO;
	btn.enabled = enabled;
	btn.tag = tag;
	[btn addTarget:target action:action forControlEvents:UIControlEventTouchUpInside];
	[btn.heightAnchor constraintEqualToConstant:50].active = YES;
	return btn;
}

- (instancetype)initWithTitles:(NSArray<NSString*>*)titles
                        images:(NSArray*)images
                       enabled:(NSArray<NSNumber*>*)enabled
                   cancelTitle:(NSString*)cancelTitle
                        onPick:(void (^)(int))onPick
                      onCancel:(void (^)(void))onCancel
{
	self = [super initWithFrame:UIScreen.mainScreen.bounds];
	if (!self) return nil;
	_onPick = [onPick copy];
	_onCancel = [onCancel copy];
	self.backgroundColor = [UIColor.blackColor colorWithAlphaComponent:0.45];
	self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

	UITapGestureRecognizer* tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(backdropTapped:)];
	tap.delegate = self;
	[self addGestureRecognizer:tap];
	[tap release];

	UIView* items = [[UIView alloc] init];
	items.backgroundColor = [UIColor secondarySystemBackgroundColor];
	items.layer.cornerRadius = 14;
	items.layer.masksToBounds = YES;
	items.translatesAutoresizingMaskIntoConstraints = NO;
	[self addSubview:items];
	_itemsCard = [items retain];
	[items release];

	UIView* prev = nil;
	for (NSUInteger i = 0; i < titles.count; i++)
	{
		BOOL en = !enabled || i >= enabled.count || [enabled[i] boolValue];
		UIImage* img = (images && i < images.count && ![images[i] isKindOfClass:[NSNull class]]) ? images[i] : nil;
		UIButton* row = iupCocoaTouchMakeRow(titles[i], NO, en, img, (NSInteger)i, self, @selector(itemTapped:));
		[_itemsCard addSubview:row];
		[row.leadingAnchor constraintEqualToAnchor:_itemsCard.leadingAnchor].active = YES;
		[row.trailingAnchor constraintEqualToAnchor:_itemsCard.trailingAnchor].active = YES;
		if (prev == nil)
		{
			[row.topAnchor constraintEqualToAnchor:_itemsCard.topAnchor].active = YES;
		}
		else
		{
			UIView* sep = [[UIView alloc] init];
			sep.backgroundColor = [UIColor separatorColor];
			sep.translatesAutoresizingMaskIntoConstraints = NO;
			[_itemsCard addSubview:sep];
			[sep.leadingAnchor constraintEqualToAnchor:_itemsCard.leadingAnchor constant:16].active = YES;
			[sep.trailingAnchor constraintEqualToAnchor:_itemsCard.trailingAnchor].active = YES;
			[sep.heightAnchor constraintEqualToConstant:0.5].active = YES;
			[sep.topAnchor constraintEqualToAnchor:prev.bottomAnchor].active = YES;
			[row.topAnchor constraintEqualToAnchor:sep.bottomAnchor].active = YES;
			[sep release];
		}
		prev = row;
	}
	if (prev) [prev.bottomAnchor constraintEqualToAnchor:_itemsCard.bottomAnchor].active = YES;

	UIView* cancel = [[UIView alloc] init];
	cancel.backgroundColor = [UIColor secondarySystemBackgroundColor];
	cancel.layer.cornerRadius = 14;
	cancel.layer.masksToBounds = YES;
	cancel.translatesAutoresizingMaskIntoConstraints = NO;
	[self addSubview:cancel];
	_cancelCard = [cancel retain];
	[cancel release];

	UIButton* cancelBtn = iupCocoaTouchMakeRow(cancelTitle, YES, YES, nil, -1, self, @selector(cancelTapped:));
	[_cancelCard addSubview:cancelBtn];
	[NSLayoutConstraint activateConstraints:@[
		[cancelBtn.topAnchor constraintEqualToAnchor:_cancelCard.topAnchor],
		[cancelBtn.bottomAnchor constraintEqualToAnchor:_cancelCard.bottomAnchor],
		[cancelBtn.leadingAnchor constraintEqualToAnchor:_cancelCard.leadingAnchor],
		[cancelBtn.trailingAnchor constraintEqualToAnchor:_cancelCard.trailingAnchor],
	]];

	UILayoutGuide* g = self.safeAreaLayoutGuide;
	[NSLayoutConstraint activateConstraints:@[
		[_itemsCard.leadingAnchor constraintEqualToAnchor:g.leadingAnchor constant:10],
		[_itemsCard.trailingAnchor constraintEqualToAnchor:g.trailingAnchor constant:-10],
		[_cancelCard.leadingAnchor constraintEqualToAnchor:g.leadingAnchor constant:10],
		[_cancelCard.trailingAnchor constraintEqualToAnchor:g.trailingAnchor constant:-10],
		[_itemsCard.bottomAnchor constraintEqualToAnchor:_cancelCard.topAnchor constant:-8],
		[_cancelCard.bottomAnchor constraintEqualToAnchor:g.bottomAnchor constant:-8],
	]];
	return self;
}

- (void)dealloc
{
	[_onPick release];
	[_onCancel release];
	[_itemsCard release];
	[_cancelCard release];
	[super dealloc];
}

- (void)itemTapped:(UIButton*)sender
{
	if (_onPick) _onPick((int)sender.tag);
}

- (void)cancelTapped:(UIButton*)sender
{
	(void)sender;
	if (_onCancel) _onCancel();
}

- (void)backdropTapped:(UITapGestureRecognizer*)g
{
	(void)g;
	if (_onCancel) _onCancel();
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gr shouldReceiveTouch:(UITouch*)touch
{
	(void)gr;
	CGPoint p = [touch locationInView:self];
	if (CGRectContainsPoint(_itemsCard.frame, p)) return NO;
	if (CGRectContainsPoint(_cancelCard.frame, p)) return NO;
	return YES;
}

- (void)showInWindow:(UIWindow*)window
{
	self.frame = window.bounds;
	[window addSubview:self];
	[self layoutIfNeeded];
	self.alpha = 0.0;
	CGAffineTransform slide = CGAffineTransformMakeTranslation(0, 80);
	_itemsCard.transform = slide;
	_cancelCard.transform = slide;
	[UIView animateWithDuration:0.25
	                      delay:0.0
	     usingSpringWithDamping:0.9
	      initialSpringVelocity:0.0
	                    options:UIViewAnimationOptionCurveEaseOut
	                 animations:^{
		self.alpha = 1.0;
		_itemsCard.transform = CGAffineTransformIdentity;
		_cancelCard.transform = CGAffineTransformIdentity;
	} completion:nil];
}

- (void)dismissAnimated
{
	[UIView animateWithDuration:0.20
	                 animations:^{
		self.alpha = 0.0;
		CGAffineTransform slide = CGAffineTransformMakeTranslation(0, 80);
		_itemsCard.transform = slide;
		_cancelCard.transform = slide;
	} completion:^(BOOL done) { (void)done; [self removeFromSuperview]; }];
}

@end


static void cocoaTouchMenuPresentSheet(Ihandle* menu_ih, UIViewController* host, CGPoint anchor);

static NSString* cocoaTouchMenuDecorateTitle(Ihandle* item_ih)
{
	NSString* base = cocoaTouchMenuItemDisplayTitle(item_ih);
	if (iupAttribGetBoolean(item_ih, "HIDEMARK")) return base;
	const char* value = iupAttribGet(item_ih, "VALUE");
	if (!value) return base;
	if (iupStrEqualNoCase(value, "ON")) return [@"✓  " stringByAppendingString:base];
	return base;
}


static void cocoaTouchMenuPresentSheet(Ihandle* menu_ih, UIViewController* host, CGPoint anchor)
{
	(void)anchor;
	if (!host) return;
	cocoaTouchMenuFireOpen(menu_ih);

	NSMutableArray* titles  = [NSMutableArray array];
	NSMutableArray* images  = [NSMutableArray array];
	NSMutableArray* enabled = [NSMutableArray array];
	NSMutableArray* rows    = [NSMutableArray array];

	int recent_count = iupAttribGetInt(menu_ih, "_IUP_RECENT_COUNT");
	for (int i = 0; i < recent_count; i++)
	{
		char attr[32]; snprintf(attr, sizeof(attr), "_IUP_RECENT_FILE%d", i);
		const char* path = iupAttribGet(menu_ih, attr);
		if (!path || !*path) continue;
		[titles addObject:[NSString stringWithUTF8String:path]];
		[images addObject:[NSNull null]];
		[enabled addObject:@YES];
		[rows addObject:[NSValue valueWithPointer:NULL]];
	}

	int count = IupGetChildCount(menu_ih);
	for (int i = 0; i < count; i++)
	{
		Ihandle* child = IupGetChild(menu_ih, i);
		if (!child || !child->iclass || !child->iclass->name) continue;
		const char* cname = child->iclass->name;
		if (iupStrEqual(cname, "menuseparator")) continue;
		if (!iupStrEqual(cname, "menuitem") && !iupStrEqual(cname, "submenu")) continue;
		NSString* t = iupStrEqual(cname, "menuitem") ? cocoaTouchMenuDecorateTitle(child) : cocoaTouchMenuItemDisplayTitle(child);
		[titles addObject:t];
		UIImage* img = cocoaTouchMenuResolveImage(child);
		[images addObject:img ?: (id)[NSNull null]];
		BOOL en = !(iupAttribGet(child, "ACTIVE") && !iupAttribGetBoolean(child, "ACTIVE"));
		[enabled addObject:@(en)];
		[rows addObject:[NSValue valueWithPointer:child]];
	}

	UIWindow* window = [[host view] window];
	if (!window) return;

	__block IupCocoaTouchActionSheetView* view = nil;
	int recent_n = recent_count;
	Ihandle* menu_capture = menu_ih;

	view = [[IupCocoaTouchActionSheetView alloc]
		initWithTitles:titles
		        images:images
		       enabled:enabled
		   cancelTitle:@"Cancel"
		        onPick:^(int idx) {
			[view dismissAnimated];
			BOOL* done_ptr = iupObjectCheck(menu_capture)
			    ? (BOOL*)iupAttribGet(menu_capture, "_IUPCOCOA_MENU_DONE_PTR") : NULL;
			if (idx < recent_n)
			{
				if (iupObjectCheck(menu_capture))
				{
					Icallback rcb = (Icallback)iupAttribGet(menu_capture, "_IUP_RECENT_CB");
					char attr[32]; snprintf(attr, sizeof(attr), "_IUP_RECENT_FILE%d", idx);
					iupAttribSetStr(menu_capture, "TITLE", iupAttribGet(menu_capture, attr));
					if (rcb) rcb(menu_capture);
				}
				if (done_ptr) *done_ptr = YES;
				return;
			}
			Ihandle* row = (Ihandle*)[rows[idx] pointerValue];
			if (row && iupObjectCheck(row))
			{
				if (iupStrEqual(row->iclass->name, "submenu"))
				{
					Ihandle* child_menu = IupGetChild(row, 0);
					dispatch_async(dispatch_get_main_queue(), ^{
						if (child_menu && iupObjectCheck(child_menu))
							cocoaTouchMenuPresentSheet(child_menu, host, CGPointZero);
						else if (done_ptr) *done_ptr = YES;
					});
					return;
				}
				cocoaTouchMenuFireActionAfterDismiss(row);
			}
			if (done_ptr) *done_ptr = YES;
		}
		      onCancel:^{
			[view dismissAnimated];
			if (iupObjectCheck(menu_capture))
			{
				cocoaTouchMenuFireClose(menu_capture);
				BOOL* done_ptr = (BOOL*)iupAttribGet(menu_capture, "_IUPCOCOA_MENU_DONE_PTR");
				if (done_ptr) *done_ptr = YES;
			}
		}];

	[view showInWindow:window];
	[view release];
}


/* tracks which menu_ih owns each bar-button slot for targeted refresh */
static const void* IUPCOCOATOUCH_BARBUTTON_MENU_IH_KEY = "IUPCOCOATOUCH_BARBUTTON_MENU_IH_KEY";

static UIBarButtonItem* cocoaTouchMenuMakeBarButton(Ihandle* menu_ih, NSString* iconName)
{
	UIMenu* menu = cocoaTouchMenuBuildUIMenu(menu_ih);
	UIImage* icon = [UIImage systemImageNamed:iconName];
	UIBarButtonItem* item = [[[UIBarButtonItem alloc] initWithImage:icon menu:menu] autorelease];
	objc_setAssociatedObject(item, IUPCOCOATOUCH_BARBUTTON_MENU_IH_KEY, (id)menu_ih, OBJC_ASSOCIATION_ASSIGN);
	return item;
}

IUP_DRV_API void iupCocoaTouchDialogAttachDrawerMenu(Ihandle* dialog_ih, Ihandle* drawer_ih)
{
	if (!dialog_ih || ![(id)dialog_ih->handle isKindOfClass:[IupViewController class]]) return;
	IupViewController* vc = (IupViewController*)dialog_ih->handle;
	if (drawer_ih)
		vc.navigationItem.leftBarButtonItem = cocoaTouchMenuMakeBarButton(drawer_ih, @"line.3.horizontal");
	else
		vc.navigationItem.leftBarButtonItem = nil;
}

static void cocoaTouchMenuInstallMenuBarButton(Ihandle* menu_ih)
{
	Ihandle* dialog_ih = menu_ih ? menu_ih->parent : NULL;
	if (!dialog_ih || ![(id)dialog_ih->handle isKindOfClass:[IupViewController class]]) return;
	IupViewController* vc = (IupViewController*)dialog_ih->handle;
	vc.navigationItem.rightBarButtonItem = cocoaTouchMenuMakeBarButton(menu_ih, @"ellipsis.circle");
}

static void cocoaTouchMenuRemoveMenuBarButton(Ihandle* menu_ih)
{
	Ihandle* dialog_ih = menu_ih ? menu_ih->parent : NULL;
	if (!dialog_ih || ![(id)dialog_ih->handle isKindOfClass:[IupViewController class]]) return;
	IupViewController* vc = (IupViewController*)dialog_ih->handle;
	UIBarButtonItem* right = vc.navigationItem.rightBarButtonItem;
	if (right && objc_getAssociatedObject(right, IUPCOCOATOUCH_BARBUTTON_MENU_IH_KEY) == (id)menu_ih)
		vc.navigationItem.rightBarButtonItem = nil;
	UIBarButtonItem* left = vc.navigationItem.leftBarButtonItem;
	if (left && objc_getAssociatedObject(left, IUPCOCOATOUCH_BARBUTTON_MENU_IH_KEY) == (id)menu_ih)
		vc.navigationItem.leftBarButtonItem = nil;
}


IUP_SDK_API int iupdrvMenuPopup(Ihandle* ih, int x, int y)
{
	if (!ih) return IUP_ERROR;

	UIViewController* host = cocoaTouchMenuHostViewController(ih);
	if (!host) return IUP_ERROR;

	UIWindow* window = [[host view] window];
	CGPoint anchor = CGPointMake(x, y);
	if (window)
	{
		CGPoint window_pt = [window convertPoint:anchor fromWindow:nil];
		anchor = [[host view] convertPoint:window_pt fromView:nil];
	}

	cocoaTouchMenuPresentSheet(ih, host, anchor);

	__block BOOL menu_done = NO;
	iupAttribSet(ih, "_IUPCOCOA_MENU_DONE_PTR", (char*)&menu_done);

	while (!menu_done)
	{
		@autoreleasepool {
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);
		}
	}

	iupAttribSet(ih, "_IUPCOCOA_MENU_DONE_PTR", NULL);
	return IUP_NOERROR;
}

IUP_SDK_API int iupdrvMenuGetMenuBarSize(Ihandle* ih)
{
	(void)ih;
	return 0;
}


static int cocoaTouchMenuMapMethod(Ihandle* ih)
{
	ih->handle = cocoaTouchMenuCreateMarker();
	if (iupMenuIsMenuBar(ih))
	{
		cocoaTouchMenuInstallMenuBarButton(ih);
	}
	return IUP_NOERROR;
}

static void cocoaTouchMenuUnMapMethod(Ihandle* ih)
{
	if (iupMenuIsMenuBar(ih))
	{
		cocoaTouchMenuRemoveMenuBarButton(ih);
	}
	iupdrvBaseUnMapMethod(ih);
}

static int cocoaTouchMenuItemMapMethod(Ihandle* ih)
{
	ih->handle = cocoaTouchMenuCreateMarker();
	return IUP_NOERROR;
}

/* rebuild the bar-button UIMenu after any item attribute change so disables/VALUE flips show */
static void cocoaTouchMenuInvalidateAncestor(Ihandle* item_ih)
{
	Ihandle* menu = item_ih ? item_ih->parent : NULL;
	while (menu && menu->parent && menu->parent->iclass &&
	       menu->parent->iclass->nativetype != IUP_TYPEDIALOG)
	{
		menu = menu->parent;
	}
	if (!menu) return;
	Ihandle* dialog_ih = menu->parent;

	/* drawer has no IUP parent; locate it by walking dialogs and matching the left bar-button */
	if (!dialog_ih)
	{
		extern UIWindow* iupCocoaTouchFindCurrentWindow(void);
		UIWindow* w = iupCocoaTouchFindCurrentWindow();
		UIViewController* root = w ? [w rootViewController] : nil;
		while (root)
		{
			IupViewController* vc = nil;
			if ([root isKindOfClass:[UINavigationController class]])
				vc = (IupViewController*)[(UINavigationController*)root topViewController];
			else if ([root isKindOfClass:[IupViewController class]])
				vc = (IupViewController*)root;
			UIBarButtonItem* left = vc ? vc.navigationItem.leftBarButtonItem : nil;
			if (left && objc_getAssociatedObject(left, IUPCOCOATOUCH_BARBUTTON_MENU_IH_KEY) == (id)menu)
			{
				left.menu = cocoaTouchMenuBuildUIMenu(menu);
				return;
			}
			root = root.presentedViewController;
		}
		return;
	}

	if (![(id)dialog_ih->handle isKindOfClass:[IupViewController class]]) return;
	IupViewController* vc = (IupViewController*)dialog_ih->handle;
	UIBarButtonItem* right = vc.navigationItem.rightBarButtonItem;
	if (right && objc_getAssociatedObject(right, IUPCOCOATOUCH_BARBUTTON_MENU_IH_KEY) == (id)menu)
		right.menu = cocoaTouchMenuBuildUIMenu(menu);
}

static int cocoaTouchMenuItemSetTitleAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	cocoaTouchMenuInvalidateAncestor(ih);
	return 1;
}

static int cocoaTouchMenuItemSetActiveAttrib(Ihandle* ih, const char* value)
{
	iupBaseSetActiveAttrib(ih, value);
	cocoaTouchMenuInvalidateAncestor(ih);
	return 1;
}

static int cocoaTouchMenuItemSetValueAttrib(Ihandle* ih, const char* value)
{
	/* store first so invalidate/getters see the new value, then return 1 to keep in hash */
	iupAttribSetStr(ih, "VALUE", iupStrBoolean(value) ? "ON" : "OFF");
	cocoaTouchMenuInvalidateAncestor(ih);
	return 1;
}


IUP_SDK_API int iupdrvRecentMenuInit(Ihandle* menu, int max_recent, Icallback recent_cb)
{
	if (!menu) return -1;
	iupAttribSetInt(menu, "_IUP_RECENT_MAX",  max_recent);
	iupAttribSet(menu,    "_IUP_RECENT_CB",   (char*)recent_cb);
	iupAttribSetInt(menu, "_IUP_RECENT_COUNT", 0);
	return 0;
}

IUP_SDK_API int iupdrvRecentMenuUpdate(Ihandle* menu, const char** filenames, int count, Icallback recent_cb)
{
	if (!menu) return -1;

	int max_recent = iupAttribGetInt(menu, "_IUP_RECENT_MAX");
	int existing   = iupAttribGetInt(menu, "_IUP_RECENT_COUNT");
	if (max_recent > 0 && count > max_recent) count = max_recent;

	iupAttribSet(menu, "_IUP_RECENT_CB", (char*)recent_cb);

	int i;
	for (i = 0; i < count; i++)
	{
		char attr[32];
		snprintf(attr, sizeof(attr), "_IUP_RECENT_FILE%d", i);
		iupAttribSetStr(menu, attr, filenames[i]);
	}
	for (; i < existing; i++)
	{
		char attr[32];
		snprintf(attr, sizeof(attr), "_IUP_RECENT_FILE%d", i);
		iupAttribSet(menu, attr, NULL);
	}
	iupAttribSetInt(menu, "_IUP_RECENT_COUNT", count);
	cocoaTouchMenuInvalidateAncestor(menu);
	return 0;
}


IUP_SDK_API void iupdrvMenuInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchMenuMapMethod;
	ic->UnMap = cocoaTouchMenuUnMapMethod;
}

IUP_SDK_API void iupdrvMenuItemInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchMenuItemMapMethod;
	ic->UnMap = iupdrvBaseUnMapMethod;

	iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaTouchMenuItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "VALUE", NULL, cocoaTouchMenuItemSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, cocoaTouchMenuItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "AUTOTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "HIDEMARK", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMPRESS", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "KEY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

IUP_SDK_API void iupdrvSubmenuInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchMenuItemMapMethod;
	ic->UnMap = iupdrvBaseUnMapMethod;

	iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaTouchMenuItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGE", NULL, NULL, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, cocoaTouchMenuItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "KEY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

IUP_SDK_API void iupdrvMenuSeparatorInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchMenuItemMapMethod;
	ic->UnMap = iupdrvBaseUnMapMethod;
}
