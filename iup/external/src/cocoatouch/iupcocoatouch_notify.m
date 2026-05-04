/** \file
 * \brief Notify (iOS UIKit, UNUserNotificationCenter)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <UserNotifications/UserNotifications.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_notify.h"
#include "iup_image.h"

#include "iupcocoatouch_drv.h"


#define IUPCOCOATOUCH_NOTIFY_CATEGORY @"IUP_NOTIFY_CATEGORY"

#define IUPCOCOATOUCH_TOAST_KEY              "_IUPCOCOATOUCH_NOTIFY_TOAST"
#define IUPCOCOATOUCH_TOAST_DEFAULT_TIMEOUT  3500
#define IUPCOCOATOUCH_TOAST_FADE_DURATION    0.25
#define IUPCOCOATOUCH_TOAST_CORNER_RADIUS    14.0
#define IUPCOCOATOUCH_TOAST_HORIZ_PADDING    16.0
#define IUPCOCOATOUCH_TOAST_VERT_PADDING     12.0
#define IUPCOCOATOUCH_TOAST_BOTTOM_INSET     80.0
#define IUPCOCOATOUCH_TOAST_MAX_WIDTH_RATIO  0.88
#define IUPCOCOATOUCH_TOAST_ALPHA            0.92

@interface IupCocoaTouchNotifyDelegate : NSObject <UNUserNotificationCenterDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTouchNotifyDelegate

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler
{
	(void)center; (void)notification;
	UNNotificationPresentationOptions options =
		UNNotificationPresentationOptionSound | UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList;
	completionHandler(options);
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
didReceiveNotificationResponse:(UNNotificationResponse*)response
         withCompletionHandler:(void (^)(void))completionHandler
{
	(void)center;
	Ihandle* ih = _ihandle;
	if (!ih) { completionHandler(); return; }

	NSString* action_id = response.actionIdentifier;
	if ([action_id isEqualToString:UNNotificationDismissActionIdentifier])
	{
		IFni close_cb = (IFni)IupGetCallback(ih, "CLOSE_CB");
		if (close_cb)
		{
			int ret = close_cb(ih, 2);
			if (ret == IUP_CLOSE) IupExitLoop();
		}
		completionHandler();
		return;
	}

	int action = 0;
	if (![action_id isEqualToString:UNNotificationDefaultActionIdentifier]
	    && [action_id hasPrefix:@"action"])
	{
		action = [[action_id substringFromIndex:6] intValue];
	}

	IFni notify_cb = (IFni)IupGetCallback(ih, "NOTIFY_CB");
	if (notify_cb)
	{
		int ret = notify_cb(ih, action);
		if (ret == IUP_CLOSE) IupExitLoop();
	}
	completionHandler();
}

@end


@interface IupCocoaTouchToastView : UIView
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, retain) NSTimer* dismissTimer;
@property(nonatomic, assign) BOOL dismissing;
@property(nonatomic, retain) UIVisualEffectView* card;
@end

static UIBlurEffect* cocoaTouchToastBlurFor(UITraitCollection* trait)
{
	BOOL dark = (trait.userInterfaceStyle == UIUserInterfaceStyleDark);
	return [UIBlurEffect effectWithStyle:(dark ? UIBlurEffectStyleSystemThickMaterialLight
	                                            : UIBlurEffectStyleSystemThickMaterialDark)];
}

static void cocoaTouchToastDismiss(Ihandle* ih, int reason);

@implementation IupCocoaTouchToastView

- (instancetype)initWithTitle:(NSString*)title body:(NSString*)body icon:(UIImage*)icon ihandle:(Ihandle*)ih
{
	self = [super initWithFrame:CGRectZero];
	if (!self) return nil;

	_ihandle = ih;
	self.translatesAutoresizingMaskIntoConstraints = NO;
	self.layer.cornerRadius = IUPCOCOATOUCH_TOAST_CORNER_RADIUS;
	self.layer.shadowColor = [UIColor blackColor].CGColor;
	self.layer.shadowOpacity = 0.18;
	self.layer.shadowRadius = 12.0;
	self.layer.shadowOffset = CGSizeMake(0.0, 4.0);

	/* inverted-contrast labels: white on light system (over dark blur), black on dark */
	UIColor* primary_text = [UIColor colorWithDynamicProvider:^UIColor*(UITraitCollection* trait) {
		return (trait.userInterfaceStyle == UIUserInterfaceStyleDark) ? [UIColor blackColor] : [UIColor whiteColor];
	}];
	UIColor* secondary_text = [UIColor colorWithDynamicProvider:^UIColor*(UITraitCollection* trait) {
		BOOL dark = (trait.userInterfaceStyle == UIUserInterfaceStyleDark);
		return [(dark ? [UIColor blackColor] : [UIColor whiteColor]) colorWithAlphaComponent:0.7];
	}];

	UIVisualEffectView* card = [[UIVisualEffectView alloc] initWithEffect:cocoaTouchToastBlurFor(self.traitCollection)];
	card.translatesAutoresizingMaskIntoConstraints = NO;
	card.layer.cornerRadius = IUPCOCOATOUCH_TOAST_CORNER_RADIUS;
	card.layer.masksToBounds = YES;
	[self addSubview:card];
	_card = [card retain];

	UIStackView* textStack = [[UIStackView alloc] init];
	textStack.axis = UILayoutConstraintAxisVertical;
	textStack.alignment = UIStackViewAlignmentLeading;
	textStack.spacing = 2.0;
	textStack.translatesAutoresizingMaskIntoConstraints = NO;

	BOOL has_title = (title.length > 0);
	BOOL has_body  = (body.length > 0);

	if (has_title)
	{
		UILabel* lbl = [[UILabel alloc] init];
		lbl.text = title;
		lbl.font = [UIFont systemFontOfSize:17.0 weight:UIFontWeightSemibold];
		lbl.textColor = primary_text;
		lbl.numberOfLines = 0;
		[textStack addArrangedSubview:lbl];
		[lbl release];
	}
	if (has_body)
	{
		UILabel* lbl = [[UILabel alloc] init];
		lbl.text = body;
		lbl.font = [UIFont systemFontOfSize:16.0];
		lbl.textColor = has_title ? secondary_text : primary_text;
		lbl.numberOfLines = 0;
		[textStack addArrangedSubview:lbl];
		[lbl release];
	}

	UIView* root;
	if (icon)
	{
		UIImageView* iv = [[UIImageView alloc] initWithImage:icon];
		iv.contentMode = UIViewContentModeScaleAspectFit;
		iv.translatesAutoresizingMaskIntoConstraints = NO;
		[iv.widthAnchor constraintEqualToConstant:32].active = YES;
		[iv.heightAnchor constraintEqualToConstant:32].active = YES;

		UIStackView* row = [[UIStackView alloc] initWithArrangedSubviews:@[iv, textStack]];
		row.axis = UILayoutConstraintAxisHorizontal;
		row.alignment = UIStackViewAlignmentCenter;
		row.spacing = 12.0;
		row.translatesAutoresizingMaskIntoConstraints = NO;
		root = row;
		[iv release];
		[textStack release];
	}
	else
	{
		root = textStack;
	}
	[card.contentView addSubview:root];

	[NSLayoutConstraint activateConstraints:@[
		[card.topAnchor      constraintEqualToAnchor:self.topAnchor],
		[card.bottomAnchor   constraintEqualToAnchor:self.bottomAnchor],
		[card.leadingAnchor  constraintEqualToAnchor:self.leadingAnchor],
		[card.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],

		[root.topAnchor      constraintEqualToAnchor:card.contentView.topAnchor      constant:IUPCOCOATOUCH_TOAST_VERT_PADDING],
		[root.bottomAnchor   constraintEqualToAnchor:card.contentView.bottomAnchor   constant:-IUPCOCOATOUCH_TOAST_VERT_PADDING],
		[root.leadingAnchor  constraintEqualToAnchor:card.contentView.leadingAnchor  constant:IUPCOCOATOUCH_TOAST_HORIZ_PADDING],
		[root.trailingAnchor constraintEqualToAnchor:card.contentView.trailingAnchor constant:-IUPCOCOATOUCH_TOAST_HORIZ_PADDING],
	]];

	[root release];
	[card release];

	UITapGestureRecognizer* tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTap:)];
	[self addGestureRecognizer:tap];
	[tap release];
	return self;
}

- (void)handleTap:(UITapGestureRecognizer*)g
{
	(void)g;
	if (_ihandle && iupObjectCheck(_ihandle))
		cocoaTouchToastDismiss(_ihandle, 2);
}

- (void)traitCollectionDidChange:(UITraitCollection*)previous
{
	[super traitCollectionDidChange:previous];
	if (previous && previous.userInterfaceStyle == self.traitCollection.userInterfaceStyle) return;
	[UIView animateWithDuration:0.2 animations:^{
		_card.effect = cocoaTouchToastBlurFor(self.traitCollection);
	}];
}

- (void)dealloc
{
	[_dismissTimer invalidate];
	[_dismissTimer release];
	[_card release];
	[super dealloc];
}

@end

static UIView* cocoaTouchToastHostView(void)
{
	UIWindow* window = iupCocoaTouchFindCurrentWindow();
	return window;
}

static int cocoaTouchToastShow(Ihandle* ih)
{
	UIView* host = cocoaTouchToastHostView();
	if (!host) return 0;

	const char* title = IupGetAttribute(ih, "TITLE");
	const char* body  = IupGetAttribute(ih, "BODY");
	NSString* tt = (title && *title) ? [NSString stringWithUTF8String:title] : nil;
	NSString* bb = (body  && *body)  ? [NSString stringWithUTF8String:body]  : nil;
	if (!tt.length && !bb.length) return 0;

	UIImage* icon_image = nil;
	const char* icon_name = IupGetAttribute(ih, "ICON");
	if (icon_name && *icon_name)
	{
		void* raw = iupImageGetImage(icon_name, ih, 0, NULL);
		if (raw && [(id)raw isKindOfClass:[UIImage class]])
			icon_image = (UIImage*)raw;
	}

	/* drop any in-flight toast for this Ihandle, no animation */
	IupCocoaTouchToastView* prior = (IupCocoaTouchToastView*)iupAttribGet(ih, IUPCOCOATOUCH_TOAST_KEY);
	if (prior)
	{
		[prior.dismissTimer invalidate];
		prior.dismissTimer = nil;
		[prior removeFromSuperview];
		[prior release];
		iupAttribSet(ih, IUPCOCOATOUCH_TOAST_KEY, NULL);
	}

	IupCocoaTouchToastView* toast = [[IupCocoaTouchToastView alloc] initWithTitle:tt body:bb icon:icon_image ihandle:ih];
	[host addSubview:toast];

	UILayoutGuide* safe = host.safeAreaLayoutGuide;
	[NSLayoutConstraint activateConstraints:@[
		[toast.centerXAnchor constraintEqualToAnchor:host.centerXAnchor],
		[toast.bottomAnchor  constraintEqualToAnchor:safe.bottomAnchor constant:-IUPCOCOATOUCH_TOAST_BOTTOM_INSET],
		[toast.widthAnchor   constraintLessThanOrEqualToAnchor:host.widthAnchor multiplier:IUPCOCOATOUCH_TOAST_MAX_WIDTH_RATIO],
	]];

	toast.alpha = 0.0;
	toast.transform = CGAffineTransformMakeScale(0.92, 0.92);
	[UIView animateWithDuration:IUPCOCOATOUCH_TOAST_FADE_DURATION
	                      delay:0.0
	     usingSpringWithDamping:0.85
	      initialSpringVelocity:0.4
	                    options:UIViewAnimationOptionAllowUserInteraction
	                 animations:^{
		toast.alpha = IUPCOCOATOUCH_TOAST_ALPHA;
		toast.transform = CGAffineTransformIdentity;
	} completion:nil];

	iupAttribSet(ih, IUPCOCOATOUCH_TOAST_KEY, (char*)toast);

	int timeout_ms = IupGetInt(ih, "TIMEOUT");
	if (timeout_ms <= 0) timeout_ms = IUPCOCOATOUCH_TOAST_DEFAULT_TIMEOUT;
	NSTimeInterval interval = (NSTimeInterval)timeout_ms / 1000.0;

	Ihandle* ih_ref = ih;
	NSTimer* timer = [NSTimer scheduledTimerWithTimeInterval:interval
	                                                 repeats:NO
	                                                   block:^(NSTimer* t) {
		(void)t;
		if (iupObjectCheck(ih_ref)) cocoaTouchToastDismiss(ih_ref, 1);
	}];
	toast.dismissTimer = timer;
	return 1;
}

static void cocoaTouchToastDismiss(Ihandle* ih, int reason)
{
	IupCocoaTouchToastView* toast = (IupCocoaTouchToastView*)iupAttribGet(ih, IUPCOCOATOUCH_TOAST_KEY);
	if (!toast || toast.dismissing) return;
	toast.dismissing = YES;

	[toast.dismissTimer invalidate];
	toast.dismissTimer = nil;
	iupAttribSet(ih, IUPCOCOATOUCH_TOAST_KEY, NULL);

	[UIView animateWithDuration:IUPCOCOATOUCH_TOAST_FADE_DURATION
	                      delay:0.0
	                    options:UIViewAnimationOptionCurveEaseIn | UIViewAnimationOptionBeginFromCurrentState
	                 animations:^{
		toast.alpha = 0.0;
	} completion:^(BOOL finished) {
		(void)finished;
		[toast removeFromSuperview];
		Ihandle* ih_ref = toast.ihandle;
		[toast release];

		if (ih_ref && iupObjectCheck(ih_ref))
		{
			IFni cb = (IFni)IupGetCallback(ih_ref, "CLOSE_CB");
			if (cb)
			{
				int ret = cb(ih_ref, reason);
				if (ret == IUP_CLOSE) IupExitLoop();
			}
		}
	}];
}


typedef struct _IupCocoaTouchNotify {
	Ihandle* ih;
	NSString* identifier;
	NSString* tempImagePath;
	IupCocoaTouchNotifyDelegate* delegate;
} IupCocoaTouchNotify;

static IupCocoaTouchNotify* cocoaTouchNotifyGet(Ihandle* ih)
{
	return (IupCocoaTouchNotify*)iupAttribGet(ih, "_IUPCOCOATOUCH_NOTIFY");
}

static IupCocoaTouchNotify* cocoaTouchNotifyCreate(Ihandle* ih)
{
	IupCocoaTouchNotify* n = (IupCocoaTouchNotify*)calloc(1, sizeof(IupCocoaTouchNotify));
	n->ih = ih;
	n->delegate = [[IupCocoaTouchNotifyDelegate alloc] init];
	n->delegate.ihandle = ih;
	[UNUserNotificationCenter currentNotificationCenter].delegate = n->delegate;
	iupAttribSet(ih, "_IUPCOCOATOUCH_NOTIFY", (char*)n);
	return n;
}

static void cocoaTouchNotifyFreeImagePath(IupCocoaTouchNotify* n)
{
	if (n->tempImagePath)
	{
		[[NSFileManager defaultManager] removeItemAtPath:n->tempImagePath error:nil];
		[n->tempImagePath release];
		n->tempImagePath = nil;
	}
}

static void cocoaTouchNotifyFree(IupCocoaTouchNotify* n)
{
	if (!n) return;
	if (n->identifier)
	{
		[[UNUserNotificationCenter currentNotificationCenter]
		    removeDeliveredNotificationsWithIdentifiers:@[n->identifier]];
		[n->identifier release];
	}
	cocoaTouchNotifyFreeImagePath(n);
	[n->delegate release];
	free(n);
}

static NSString* cocoaTouchNotifyWriteImagePng(Ihandle* ih, const char* value)
{
	if (!value || !*value) return nil;
	void* raw = iupImageGetImage(value, ih, 0, NULL);
	if (!raw || ![(id)raw isKindOfClass:[UIImage class]]) return nil;

	NSData* png = UIImagePNGRepresentation((UIImage*)raw);
	if (!png) return nil;

	NSString* path = [NSTemporaryDirectory() stringByAppendingPathComponent:[[NSUUID UUID] UUIDString]];
	path = [path stringByAppendingPathExtension:@"png"];
	if (![png writeToFile:path atomically:YES]) return nil;
	return [path retain];
}

static void cocoaTouchNotifyFireError(Ihandle* ih, NSString* msg)
{
	IFns cb = (IFns)IupGetCallback(ih, "ERROR_CB");
	if (cb) cb(ih, msg ? (char*)[msg UTF8String] : (char*)"Unknown notification error");
}

static UNAuthorizationStatus cocoaTouchNotifyQueryStatus(void)
{
	__block UNAuthorizationStatus status = UNAuthorizationStatusNotDetermined;
	dispatch_semaphore_t sem = dispatch_semaphore_create(0);
	[[UNUserNotificationCenter currentNotificationCenter]
	    getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* s) {
		status = s.authorizationStatus;
		dispatch_semaphore_signal(sem);
	}];
	dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC));
	return status;
}


static void cocoaTouchNotifyRegisterCategory(Ihandle* ih)
{
	NSMutableArray* actions = [NSMutableArray array];
	for (int i = 1; i <= 4; i++)
	{
		char name[16];
		snprintf(name, sizeof(name), "ACTION%d", i);
		const char* title = IupGetAttribute(ih, name);
		if (!title) continue;

		NSString* identifier = [NSString stringWithFormat:@"action%d", i];
		UNNotificationAction* act = [UNNotificationAction
		    actionWithIdentifier:identifier
		                   title:[NSString stringWithUTF8String:title]
		                 options:UNNotificationActionOptionForeground];
		[actions addObject:act];
	}

	UNNotificationCategory* cat = [UNNotificationCategory
	    categoryWithIdentifier:IUPCOCOATOUCH_NOTIFY_CATEGORY
	                   actions:actions
	         intentIdentifiers:@[]
	                   options:UNNotificationCategoryOptionCustomDismissAction];
	[[UNUserNotificationCenter currentNotificationCenter]
	    setNotificationCategories:[NSSet setWithObject:cat]];
}

static char* cocoaTouchNotifyGetPermissionAttrib(Ihandle* ih)
{
	(void)ih;
	switch (cocoaTouchNotifyQueryStatus())
	{
		case UNAuthorizationStatusAuthorized:
		case UNAuthorizationStatusProvisional:
			return "GRANTED";
		case UNAuthorizationStatusDenied:
			return "DENIED";
		default:
			return "NOTDETERMINED";
	}
}

static int cocoaTouchNotifySetRequestPermissionAttrib(Ihandle* ih, const char* value)
{
	if (!iupStrBoolean(value)) return 0;

	Ihandle* ih_ref = ih;
	[[UNUserNotificationCenter currentNotificationCenter]
	    requestAuthorizationWithOptions:(UNAuthorizationOptionAlert |
	                                     UNAuthorizationOptionSound |
	                                     UNAuthorizationOptionBadge)
	                  completionHandler:^(BOOL granted, NSError* error) {
		(void)granted;
		if (error)
		{
			dispatch_async(dispatch_get_main_queue(), ^{
				if (!ih_ref || !iupObjectCheck(ih_ref)) return;
				cocoaTouchNotifyFireError(ih_ref, [error localizedDescription]);
			});
		}
	}];
	return 0;
}

IUP_SDK_API int iupdrvNotifyIsAvailable(void)
{
	return NSClassFromString(@"UNUserNotificationCenter") ? 1 : 0;
}

IUP_SDK_API int iupdrvNotifyShow(Ihandle* ih)
{
	if (!ih) return 0;

	const char* style = IupGetAttribute(ih, "STYLE");
	if (style && iupStrEqualNoCase(style, "TOAST"))
		return cocoaTouchToastShow(ih);

	if (!iupdrvNotifyIsAvailable())
	{
		cocoaTouchNotifyFireError(ih, @"Notifications unavailable on this iOS runtime");
		return 0;
	}

	UNAuthorizationStatus status = cocoaTouchNotifyQueryStatus();
	if (status == UNAuthorizationStatusDenied)
	{
		cocoaTouchNotifyFireError(ih, @"Notification permission denied. Enable in Settings.");
		return 0;
	}
	if (status == UNAuthorizationStatusNotDetermined)
	{
		cocoaTouchNotifyFireError(ih, @"Notification permission not requested. Set REQUESTPERMISSION=YES at launch.");
		return 0;
	}

	IupCocoaTouchNotify* n = cocoaTouchNotifyGet(ih);
	if (!n) n = cocoaTouchNotifyCreate(ih);

	cocoaTouchNotifyRegisterCategory(ih);

	UNMutableNotificationContent* content = [[[UNMutableNotificationContent alloc] init] autorelease];

	const char* title    = IupGetAttribute(ih, "TITLE");
	const char* body     = IupGetAttribute(ih, "BODY");
	const char* subtitle = IupGetAttribute(ih, "SUBTITLE");
	const char* thread   = IupGetAttribute(ih, "THREADID");
	int silent           = IupGetInt(ih, "SILENT");

	content.title    = title    ? [NSString stringWithUTF8String:title]    : @"";
	content.body     = body     ? [NSString stringWithUTF8String:body]     : @"";
	if (subtitle) content.subtitle = [NSString stringWithUTF8String:subtitle];
	if (thread)   content.threadIdentifier = [NSString stringWithUTF8String:thread];
	if (!silent)  content.sound = [UNNotificationSound defaultSound];
	content.categoryIdentifier = IUPCOCOATOUCH_NOTIFY_CATEGORY;

	cocoaTouchNotifyFreeImagePath(n);
	const char* image = IupGetAttribute(ih, "IMAGE");
	if (image && *image)
	{
		n->tempImagePath = cocoaTouchNotifyWriteImagePng(ih, image);
		if (n->tempImagePath)
		{
			NSURL* url = [NSURL fileURLWithPath:n->tempImagePath];
			NSError* err = nil;
			UNNotificationAttachment* att = [UNNotificationAttachment
			    attachmentWithIdentifier:@"image" URL:url options:nil error:&err];
			if (att && !err) content.attachments = @[att];
		}
	}

	if (n->identifier)
	{
		[[UNUserNotificationCenter currentNotificationCenter]
		    removeDeliveredNotificationsWithIdentifiers:@[n->identifier]];
		[n->identifier release];
	}
	n->identifier = [[[NSUUID UUID] UUIDString] retain];

	UNNotificationRequest* req = [UNNotificationRequest
	    requestWithIdentifier:n->identifier content:content trigger:nil];

	Ihandle* ih_ref = ih;
	[[UNUserNotificationCenter currentNotificationCenter]
	    addNotificationRequest:req
	     withCompletionHandler:^(NSError* error) {
		if (error)
		{
			dispatch_async(dispatch_get_main_queue(), ^{
				if (!ih_ref || !iupObjectCheck(ih_ref)) return;
				cocoaTouchNotifyFireError(ih_ref, [error localizedDescription]);
			});
		}
	}];
	return 1;
}

IUP_SDK_API int iupdrvNotifyClose(Ihandle* ih)
{
	if (!ih) return 0;

	if (iupAttribGet(ih, IUPCOCOATOUCH_TOAST_KEY))
	{
		cocoaTouchToastDismiss(ih, 3);
		return 1;
	}

	IupCocoaTouchNotify* n = cocoaTouchNotifyGet(ih);
	if (!n || !n->identifier) return 0;

	UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
	[center removeDeliveredNotificationsWithIdentifiers:@[n->identifier]];
	[center removePendingNotificationRequestsWithIdentifiers:@[n->identifier]];

	IFni cb = (IFni)IupGetCallback(ih, "CLOSE_CB");
	if (cb)
	{
		int ret = cb(ih, 3);
		if (ret == IUP_CLOSE) IupExitLoop();
	}
	return 1;
}

IUP_SDK_API void iupdrvNotifyDestroy(Ihandle* ih)
{
	if (!ih) return;

	IupCocoaTouchToastView* toast = (IupCocoaTouchToastView*)iupAttribGet(ih, IUPCOCOATOUCH_TOAST_KEY);
	if (toast)
	{
		[toast.dismissTimer invalidate];
		toast.dismissTimer = nil;
		toast.ihandle = NULL;
		[toast removeFromSuperview];
		[toast release];
		iupAttribSet(ih, IUPCOCOATOUCH_TOAST_KEY, NULL);
	}

	IupCocoaTouchNotify* n = cocoaTouchNotifyGet(ih);
	if (!n) return;
	cocoaTouchNotifyFree(n);
	iupAttribSet(ih, "_IUPCOCOATOUCH_NOTIFY", NULL);
}

IUP_SDK_API void iupdrvNotifyInitClass(Iclass* ic)
{
	iupClassRegisterAttribute(ic, "SUBTITLE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "THREADID", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "PERMISSION", cocoaTouchNotifyGetPermissionAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "REQUESTPERMISSION", NULL, cocoaTouchNotifySetRequestPermissionAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	/* STYLE=TOAST is an in-app overlay; default routes through UNUserNotificationCenter */
	iupClassRegisterAttribute(ic, "STYLE", NULL, NULL, IUPAF_SAMEASSYSTEM, "NOTIFICATION", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	/* TIMEOUT is TOAST-only; UNUserNotificationCenter doesn't expose it */
}
