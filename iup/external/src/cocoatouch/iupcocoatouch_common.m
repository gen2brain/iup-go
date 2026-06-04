/** \file
 * \brief iOS UIKit driver common helpers and base iupdrv* implementations.
 *
 * See Copyright Notice in "iup.h"
 */

#include <string.h>
#include <unistd.h>

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_drv.h"

#include "iupcocoatouch_drv.h"

#import "IupAppDelegate.h"
#import "IupViewController.h"


IUP_DRV_API const void* IHANDLE_ASSOCIATED_OBJ_KEY = @"IHANDLE_ASSOCIATED_OBJ_KEY";

@implementation IupCocoaTouchFixed

- (instancetype)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self)
	{
		self.backgroundColor = [UIColor clearColor];
		self.clipsToBounds = NO;
		self.autoresizingMask = UIViewAutoresizingNone;
		self.translatesAutoresizingMaskIntoConstraints = YES;
	}
	return self;
}

@end


IUP_DRV_API UIWindow* iupCocoaTouchFindCurrentWindow(void)
{
	id<UIApplicationDelegate> delegate = [[UIApplication sharedApplication] delegate];
	if ([delegate conformsToProtocol:@protocol(IupAppDelegateProtocol)])
		return [(id<IupAppDelegateProtocol>)delegate currentWindow];
	return nil;
}

IUP_DRV_API UIViewController* iupCocoaTouchFindCurrentRootViewController(void)
{
	return [iupCocoaTouchFindCurrentWindow() rootViewController];
}

IUP_DRV_API UIViewController* iupCocoaTouchFindTopPresentedViewController(void)
{
	UIViewController* top = iupCocoaTouchFindCurrentRootViewController();
	while (top != nil
	    && top.presentedViewController != nil
	    && !top.presentedViewController.isBeingDismissed)
	{
		top = top.presentedViewController;
	}
	return top;
}

IUP_DRV_API IupCocoaTouchFixed* iupCocoaTouchDialogGetClientArea(Ihandle* dialog_ih)
{
	if (dialog_ih == NULL)
	{
		return nil;
	}
	id handle = dialog_ih->handle;
	if ([handle isKindOfClass:[IupViewController class]])
	{
		return [(IupViewController*)handle clientArea];
	}
	return nil;
}

IUP_DRV_API void iupCocoaTouchAddToParent(Ihandle* ih)
{
	id child_handle = ih->handle;
	if (![child_handle isKindOfClass:[UIView class]])
	{
		NSCAssert(NO, @"Widget native handle must be a UIView");
		return;
	}
	UIView* child_view = (UIView*)child_handle;

	/* IupTabs stashes its per-child page view here */
	UIView* parent_view = (UIView*)iupAttribGet(ih, "_IUPTAB_CONTAINER");
	if (parent_view && ![parent_view isKindOfClass:[UIView class]])
	{
		parent_view = nil;
	}

	if (!parent_view)
	{
		id parent_native_handle = iupChildTreeGetNativeParentHandle(ih);
		if ([parent_native_handle isKindOfClass:[IupViewController class]])
		{
			parent_view = [(IupViewController*)parent_native_handle clientArea];
		}
		else if ([parent_native_handle isKindOfClass:[UIView class]])
		{
			parent_view = (UIView*)parent_native_handle;
		}
	}

	if (parent_view)
	{
		[parent_view addSubview:child_view];
	}
}

IUP_DRV_API void iupCocoaTouchRemoveFromParent(Ihandle* ih)
{
	id child_handle = ih->handle;
	if ([child_handle isKindOfClass:[UIView class]])
	{
		[(UIView*)child_handle removeFromSuperview];
	}
}

IUP_DRV_API UIColor* iupCocoaTouchToNativeColor(const char* color)
{
	unsigned char r, g, b, a;
	if (iupStrToRGBA(color, &r, &g, &b, &a))
	{
		return [UIColor colorWithRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:a/255.0];
	}
	return nil;
}

IUP_DRV_API char* iupCocoaTouchColorFromNative(UIColor* color)
{
	CGFloat r, g, b, a;
	if ([color getRed:&r green:&g blue:&b alpha:&a])
	{
		return iupStrReturnRGBA((unsigned char)(r*255), (unsigned char)(g*255), (unsigned char)(b*255), (unsigned char)(a*255));
	}
	return NULL;
}

IUP_DRV_API int iupCocoaTouchSetBgColorAttrib(Ihandle* ih, const char* color_str)
{
	UIColor* color = iupCocoaTouchToNativeColor(color_str);
	if (color && [(id)ih->handle isKindOfClass:[UIView class]])
	{
		[(UIView*)ih->handle setBackgroundColor:color];
		return IUP_NOERROR;
	}
	return IUP_ERROR;
}

/* mutates tag_buf with NUL terminators */
static int cocoaTouchMarkupSpanAttr(char* tag_buf, const char* key, char* out_buf, size_t out_size)
{
	char needle[64];
	snprintf(needle, sizeof(needle), "%s=\"", key);
	char* attr = strstr(tag_buf, needle);
	if (!attr) return 0;
	attr += strlen(needle);
	char* close_quote = strchr(attr, '"');
	if (!close_quote) return 0;
	size_t len = (size_t)(close_quote - attr);
	if (len >= out_size) len = out_size - 1;
	memcpy(out_buf, attr, len);
	out_buf[len] = '\0';
	return 1;
}

/* Pango family aliases -> iOS font families */
static NSString* cocoaTouchMarkupResolveFamily(const char* family)
{
	if (!family || !*family) return nil;
	if (iupStrEqualNoCase(family, "Monospace") || iupStrEqualNoCase(family, "Mono"))
		return @"Menlo";
	if (iupStrEqualNoCase(family, "Serif"))
		return @"Times New Roman";
	if (iupStrEqualNoCase(family, "Sans") || iupStrEqualNoCase(family, "Sans-serif") || iupStrEqualNoCase(family, "System"))
		return nil;  /* nil = system font */
	return [NSString stringWithUTF8String:family];
}

/* per-tag markup state (span/big/small/sub/sup/b/i/u/s) */
typedef struct {
	UIFontDescriptorSymbolicTraits traits;
	int underline;
	int strikeout;
	UIColor* fg;
	UIColor* bg;
	NSString* family;       /* nil = inherit base_font's family */
	CGFloat size;           /* 0 = inherit base_font's size, scaled by size_factor */
	CGFloat size_factor;    /* multiplier from <big>/<small> nesting */
	int baseline_offset;    /* >0 superscript, <0 subscript, 0 normal */
} cocoaTouchMarkupState;

#define COCOA_MARKUP_MAX_DEPTH 16

IUP_DRV_API NSAttributedString* iupCocoaTouchParseMarkup(const char* raw, UIFont* base_font, UIColor* base_color)
{
	if (!raw || !*raw || !strchr(raw, '<')) return nil;
	/* base_font may be nil pre-layout (UIButton.Configuration titleLabel.font is unreliable then) */
	if (!base_font) base_font = [UIFont systemFontOfSize:[UIFont systemFontSize]];

	NSMutableAttributedString* out = [[[NSMutableAttributedString alloc] init] autorelease];

	cocoaTouchMarkupState stack[COCOA_MARKUP_MAX_DEPTH];
	int sp = 0;
	stack[0] = (cocoaTouchMarkupState){ 0, 0, 0, base_color, nil, nil, 0, 1.0, 0 };

	const char* p = raw;
	while (*p)
	{
		if (*p != '<')
		{
			const char* start = p;
			while (*p && *p != '<') p++;
			NSString* chunk = [[[NSString alloc] initWithBytes:start length:(NSUInteger)(p - start) encoding:NSUTF8StringEncoding] autorelease];

			cocoaTouchMarkupState* st = &stack[sp];
			CGFloat target_size = (st->size > 0 ? st->size : base_font.pointSize) * st->size_factor;
			NSString* target_family = st->family;

			UIFont* font = base_font;
			if (target_family && target_family.length > 0)
			{
				UIFont* family_font = [UIFont fontWithName:target_family size:target_size];
				if (!family_font)
				{
					UIFontDescriptor* fd = [UIFontDescriptor fontDescriptorWithName:target_family size:target_size];
					if (fd) family_font = [UIFont fontWithDescriptor:fd size:target_size];
				}
				if (family_font) font = family_font;
				else if (target_size != base_font.pointSize) font = [UIFont fontWithDescriptor:base_font.fontDescriptor size:target_size];
			}
			else if (target_size != base_font.pointSize)
			{
				font = [UIFont fontWithDescriptor:base_font.fontDescriptor size:target_size];
			}

			if (st->traits && font)
			{
				UIFontDescriptor* fd = [font.fontDescriptor fontDescriptorWithSymbolicTraits:st->traits | font.fontDescriptor.symbolicTraits];
				if (fd) { UIFont* styled = [UIFont fontWithDescriptor:fd size:font.pointSize]; if (styled) font = styled; }
			}

			NSMutableDictionary* attrs = [NSMutableDictionary dictionary];
			if (font)             attrs[NSFontAttributeName]            = font;
			if (st->fg)           attrs[NSForegroundColorAttributeName] = st->fg;
			if (st->bg)           attrs[NSBackgroundColorAttributeName] = st->bg;
			if (st->underline)    attrs[NSUnderlineStyleAttributeName]  = @(NSUnderlineStyleSingle);
			if (st->strikeout)    attrs[NSStrikethroughStyleAttributeName] = @(NSUnderlineStyleSingle);
			if (st->baseline_offset)
				attrs[NSBaselineOffsetAttributeName] = @(st->baseline_offset);

			NSAttributedString* piece = [[[NSAttributedString alloc] initWithString:chunk attributes:attrs] autorelease];
			[out appendAttributedString:piece];
			continue;
		}

		const char* tag_start = p + 1;
		const char* tag_end = strchr(tag_start, '>');
		if (!tag_end) break;
		size_t tag_len = (size_t)(tag_end - tag_start);
		if (tag_len == 0) { p = tag_end + 1; continue; }

		int closing = 0;
		if (tag_start[0] == '/') { closing = 1; tag_start++; tag_len--; }

		char tag_buf[512];
		if (tag_len >= sizeof(tag_buf)) tag_len = sizeof(tag_buf) - 1;
		memcpy(tag_buf, tag_start, tag_len);
		tag_buf[tag_len] = '\0';

		/* span/big/small/sub/sup push a frame; b/i/u/s flip a flag in the current frame */
		if (iupStrEqualNoCase(tag_buf, "b"))      { if (closing) stack[sp].traits &= ~UIFontDescriptorTraitBold;   else stack[sp].traits |= UIFontDescriptorTraitBold; }
		else if (iupStrEqualNoCase(tag_buf, "i")) { if (closing) stack[sp].traits &= ~UIFontDescriptorTraitItalic; else stack[sp].traits |= UIFontDescriptorTraitItalic; }
		else if (iupStrEqualNoCase(tag_buf, "u")) { stack[sp].underline = closing ? 0 : 1; }
		else if (iupStrEqualNoCase(tag_buf, "s") || iupStrEqualNoCase(tag_buf, "strike")) { stack[sp].strikeout = closing ? 0 : 1; }
		else if (iupStrEqualPartial(tag_buf, "span") || iupStrEqualNoCase(tag_buf, "big") || iupStrEqualNoCase(tag_buf, "small") || iupStrEqualNoCase(tag_buf, "sub") || iupStrEqualNoCase(tag_buf, "sup"))
		{
			if (closing)
			{
				if (sp > 0) sp--;
			}
			else if (sp < COCOA_MARKUP_MAX_DEPTH - 1)
			{
				cocoaTouchMarkupState next = stack[sp];
				sp++;
				stack[sp] = next;
				cocoaTouchMarkupState* st = &stack[sp];

				if (iupStrEqualPartial(tag_buf, "span"))
				{
					char buf[128];
					if (cocoaTouchMarkupSpanAttr(tag_buf, "foreground", buf, sizeof(buf)) || cocoaTouchMarkupSpanAttr(tag_buf, "color", buf, sizeof(buf)))
					{
						UIColor* c = iupCocoaTouchToNativeColor(buf);
						if (c) st->fg = c;
					}
					if (cocoaTouchMarkupSpanAttr(tag_buf, "background", buf, sizeof(buf)) || cocoaTouchMarkupSpanAttr(tag_buf, "bgcolor", buf, sizeof(buf)))
					{
						UIColor* c = iupCocoaTouchToNativeColor(buf);
						if (c) st->bg = c;
					}
					if (cocoaTouchMarkupSpanAttr(tag_buf, "font_family", buf, sizeof(buf)) || cocoaTouchMarkupSpanAttr(tag_buf, "face", buf, sizeof(buf)))
					{
						st->family = cocoaTouchMarkupResolveFamily(buf);
					}
					if (cocoaTouchMarkupSpanAttr(tag_buf, "font_size", buf, sizeof(buf)) || cocoaTouchMarkupSpanAttr(tag_buf, "size", buf, sizeof(buf)))
					{
						double s = atof(buf);
						/* Pango font_size: 1024ths of a point if >= 1024, raw points otherwise */
						if (s >= 1024.0) s /= 1024.0;
						if (s > 0) st->size = (CGFloat)s;
					}
					if (cocoaTouchMarkupSpanAttr(tag_buf, "font_weight", buf, sizeof(buf)) || cocoaTouchMarkupSpanAttr(tag_buf, "weight", buf, sizeof(buf)))
					{
						if (iupStrEqualNoCase(buf, "bold") || iupStrEqualNoCase(buf, "heavy") || iupStrEqualNoCase(buf, "ultrabold") || atoi(buf) >= 600)
							st->traits |= UIFontDescriptorTraitBold;
					}
					if (cocoaTouchMarkupSpanAttr(tag_buf, "font_style", buf, sizeof(buf)) || cocoaTouchMarkupSpanAttr(tag_buf, "style", buf, sizeof(buf)))
					{
						if (iupStrEqualNoCase(buf, "italic") || iupStrEqualNoCase(buf, "oblique"))
							st->traits |= UIFontDescriptorTraitItalic;
					}
					if (cocoaTouchMarkupSpanAttr(tag_buf, "underline", buf, sizeof(buf)) && !iupStrEqualNoCase(buf, "none"))
						st->underline = 1;
					if (cocoaTouchMarkupSpanAttr(tag_buf, "strikethrough", buf, sizeof(buf)) && (iupStrEqualNoCase(buf, "true") || iupStrEqualNoCase(buf, "yes") || atoi(buf) == 1))
						st->strikeout = 1;
				}
				else if (iupStrEqualNoCase(tag_buf, "big"))    st->size_factor *= 1.2;
				else if (iupStrEqualNoCase(tag_buf, "small"))  st->size_factor *= 0.83;
				else if (iupStrEqualNoCase(tag_buf, "sub"))  { st->size_factor *= 0.7; st->baseline_offset = -3; }
				else if (iupStrEqualNoCase(tag_buf, "sup"))  { st->size_factor *= 0.7; st->baseline_offset = 4; }
			}
		}

		p = tag_end + 1;
	}

	return out.length > 0 ? out : nil;
}

IUP_SDK_API void iupdrvActivate(Ihandle* ih)
{
	(void)ih;
}

IUP_SDK_API void iupdrvReparent(Ihandle* ih)
{
	(void)ih;
}

IUP_SDK_API void iupdrvBaseLayoutUpdateMethod(Ihandle* ih)
{
	id handle = ih->handle;
	if (![handle isKindOfClass:[UIView class]])
	{
		return;
	}
	/* x/y are already client-area coords; safe-area offset lives on the Fixed */
	[(UIView*)handle setFrame:CGRectMake(ih->x, ih->y, ih->currentwidth, ih->currentheight)];
}

IUP_SDK_API void iupdrvBaseUnMapMethod(Ihandle* ih)
{
	if (!ih || !ih->handle) return;
	id handle = ih->handle;
	objc_setAssociatedObject(handle, IHANDLE_ASSOCIATED_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);

	if ([handle isKindOfClass:[UIView class]])
		[(UIView*)handle removeFromSuperview];
	else if ([handle isKindOfClass:[UIViewController class]])
		[[(UIViewController*)handle view] removeFromSuperview];

	[handle release];
	ih->handle = NULL;
}

IUP_SDK_API void iupdrvDisplayUpdate(Ihandle* ih)
{
	id handle = ih->handle;
	if ([handle isKindOfClass:[UIView class]])
	{
		[(UIView*)handle setNeedsDisplay];
	}
}

IUP_SDK_API void iupdrvDisplayRedraw(Ihandle* ih)
{
	iupdrvDisplayUpdate(ih);
}

static UIView* cocoaTouchCommonGetView(Ihandle* ih)
{
	if (!ih || !ih->handle) return nil;
	id h = ih->handle;
	if ([h isKindOfClass:[UIView class]]) return (UIView*)h;
	if ([h isKindOfClass:[UIViewController class]]) return [(UIViewController*)h view];
	return nil;
}

IUP_SDK_API void iupdrvScreenToClient(Ihandle* ih, int* x, int* y)
{
	UIView* view = cocoaTouchCommonGetView(ih);
	UIWindow* win = view.window ?: iupCocoaTouchFindCurrentWindow();
	if (!view || !win || !x || !y) return;

	CGPoint window_pt = [win convertPoint:CGPointMake(*x, *y) fromWindow:nil];
	CGPoint view_pt   = [view convertPoint:window_pt fromView:nil];
	*x = iupROUND(view_pt.x);
	*y = iupROUND(view_pt.y);
}

IUP_SDK_API void iupdrvClientToScreen(Ihandle* ih, int* x, int* y)
{
	UIView* view = cocoaTouchCommonGetView(ih);
	UIWindow* win = view.window ?: iupCocoaTouchFindCurrentWindow();
	if (!view || !win || !x || !y) return;

	CGPoint window_pt = [view convertPoint:CGPointMake(*x, *y) toView:nil];
	CGPoint screen_pt = [win convertPoint:window_pt toWindow:nil];
	*x = iupROUND(screen_pt.x);
	*y = iupROUND(screen_pt.y);
}

IUP_SDK_API int iupdrvBaseSetZorderAttrib(Ihandle* ih, const char* value)
{
	UIView* view = cocoaTouchCommonGetView(ih);
	UIView* parent = view ? view.superview : nil;
	if (!parent) return 0;
	if (iupStrEqualNoCase(value, "TOP"))    [parent bringSubviewToFront:view];
	else if (iupStrEqualNoCase(value, "BOTTOM")) [parent sendSubviewToBack:view];
	return 1;
}

IUP_SDK_API void iupdrvSetVisible(Ihandle* ih, int visible)
{
	UIView* view = cocoaTouchCommonGetView(ih);
	if (view) [view setHidden:visible ? NO : YES];
}

IUP_SDK_API int iupdrvIsVisible(Ihandle* ih)
{
	UIView* view = cocoaTouchCommonGetView(ih);
	return view ? !view.isHidden : 1;
}

IUP_SDK_API int iupdrvIsActive(Ihandle* ih)
{
	/* shadow attrib written by iupdrvSetActive; default active when unset */
	const char* v = iupAttribGet(ih, "_IUPCOCOATOUCH_ACTIVE");
	return (!v) ? 1 : iupStrBoolean(v);
}

IUP_SDK_API void iupdrvSetActive(Ihandle* ih, int enable)
{
	iupAttribSet(ih, "_IUPCOCOATOUCH_ACTIVE", enable ? "YES" : "NO");
	id handle = ih->handle;
	if ([handle respondsToSelector:@selector(setEnabled:)])
		[handle setEnabled:enable ? YES : NO];
	else if ([handle isKindOfClass:[UIView class]])
		[(UIView*)handle setUserInteractionEnabled:enable ? YES : NO];
}

IUP_SDK_API char* iupdrvBaseGetXAttrib(Ihandle* ih)
{
	(void)ih;
	return NULL;
}

IUP_SDK_API char* iupdrvBaseGetYAttrib(Ihandle* ih)
{
	(void)ih;
	return NULL;
}

IUP_SDK_API int iupdrvBaseSetBgColorAttrib(Ihandle* ih, const char* value)
{
	return iupCocoaTouchSetBgColorAttrib(ih, value);
}

IUP_SDK_API int iupdrvBaseSetCursorAttrib(Ihandle* ih, const char* value)
{
	(void)ih; (void)value;
	return 0;
}

IUP_SDK_API int iupdrvGetScrollbarSize(void)
{
	return 0;
}

IUP_SDK_API void iupdrvSetAccessibleTitle(Ihandle* ih, const char* title)
{
	id handle = ih->handle;
	if ([handle respondsToSelector:@selector(setAccessibilityLabel:)])
	{
		[handle setAccessibilityLabel:title ? [NSString stringWithUTF8String:title] : nil];
	}
}

IUP_SDK_API void iupdrvSetAccessibleDescription(Ihandle* ih, const char* description)
{
	id handle = ih->handle;
	if ([handle respondsToSelector:@selector(setAccessibilityHint:)])
		[handle setAccessibilityHint:description ? [NSString stringWithUTF8String:description] : nil];
}

IUP_SDK_API void iupdrvBaseRegisterCommonAttrib(Iclass* ic)
{
	(void)ic;
}

IUP_SDK_API void iupdrvBaseRegisterVisualAttrib(Iclass* ic)
{
	(void)ic;
}

IUP_SDK_API void iupdrvPostRedraw(Ihandle* ih)
{
	iupdrvDisplayUpdate(ih);
}

IUP_SDK_API void iupdrvRedrawNow(Ihandle* ih)
{
	iupdrvDisplayUpdate(ih);
}

IUP_SDK_API void iupdrvSendKey(int key, int press)
{
	(void)key; (void)press;
}

IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
	(void)x; (void)y; (void)bt; (void)status;
}

IUP_SDK_API void iupdrvSleep(int time)
{
	/* time is ms per IUP spec */
	if (time > 0) usleep((useconds_t)time * 1000);
}

IUP_SDK_API void iupdrvWarpPointer(int x, int y)
{
	(void)x; (void)y;
}
