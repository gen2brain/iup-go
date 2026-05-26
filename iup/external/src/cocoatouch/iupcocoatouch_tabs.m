/** \file
 * \brief Tabs Control (iOS UIKit)
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
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_array.h"
#include "iup_tabs.h"

#include "iupcocoatouch_drv.h"

static const CGFloat kIupCocoaTouchTabBarHeight = 32.0;

@interface IupCocoaTouchTabsView : UIView
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, retain) UISegmentedControl* segmentedControl;
@property(nonatomic, retain) UIView* contentArea;
@property(nonatomic, retain) NSMutableArray<UIButton*>* closeButtons;
@end

@implementation IupCocoaTouchTabsView

- (instancetype)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self)
	{
		self.autoresizingMask = UIViewAutoresizingNone;
		self.translatesAutoresizingMaskIntoConstraints = YES;
		self.clipsToBounds = YES;

		_segmentedControl = [[UISegmentedControl alloc] initWithItems:@[]];
		_segmentedControl.autoresizingMask = UIViewAutoresizingNone;
		_segmentedControl.apportionsSegmentWidthsByContent = NO;
		[self addSubview:_segmentedControl];

		_contentArea = [[UIView alloc] initWithFrame:CGRectZero];
		_contentArea.autoresizingMask = UIViewAutoresizingNone;
		_contentArea.clipsToBounds = YES;
		[self addSubview:_contentArea];

		_closeButtons = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[_segmentedControl release];
	[_contentArea release];
	[_closeButtons release];
	[super dealloc];
}

@end


static IupCocoaTouchTabsView* cocoaTouchTabsGetRoot(Ihandle* ih)
{
	if (!ih || !ih->handle) return nil;
	id handle = ih->handle;
	if (![handle isKindOfClass:[IupCocoaTouchTabsView class]]) return nil;
	return (IupCocoaTouchTabsView*)handle;
}

static Iarray* cocoaTouchTabsGetVisibleArray(Ihandle* ih)
{
	Iarray* visible_array = (Iarray*)iupAttribGet(ih, "_IUPCOCOATOUCH_VISIBLEARRAY");
	if (!visible_array)
	{
		int count = IupGetChildCount(ih);
		visible_array = iupArrayCreate(count > 0 ? count : 1, sizeof(int));
		iupAttribSet(ih, "_IUPCOCOATOUCH_VISIBLEARRAY", (char*)visible_array);
	}
	return visible_array;
}

static int cocoaTouchTabsPosFixToNative(Ihandle* ih, int iup_pos)
{
	Iarray* visible_array = cocoaTouchTabsGetVisibleArray(ih);
	int count = iupArrayCount(visible_array);
	int* visible_data = (int*)iupArrayGetData(visible_array);
	if (iup_pos < 0 || iup_pos >= count || !visible_data[iup_pos]) return -1;

	int native_pos = 0;
	for (int i = 0; i < iup_pos; i++)
	{
		if (visible_data[i]) native_pos++;
	}
	return native_pos;
}

static int cocoaTouchTabsPosFixFromNative(Ihandle* ih, int native_pos)
{
	if (native_pos < 0) return -1;
	Iarray* visible_array = cocoaTouchTabsGetVisibleArray(ih);
	int count = iupArrayCount(visible_array);
	int* visible_data = (int*)iupArrayGetData(visible_array);
	int current = -1;
	for (int i = 0; i < count; i++)
	{
		if (visible_data[i]) current++;
		if (current == native_pos) return i;
	}
	return -1;
}

static void cocoaTouchTabsShowOnlyPage(Ihandle* ih, int iup_pos)
{
	for (Ihandle* child = ih->firstchild; child; child = child->brother)
	{
		UIView* container = (UIView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
		if (!container) continue;
		int child_pos = IupGetChildPos(ih, child);
		container.hidden = (child_pos != iup_pos);
	}
}

static NSString* cocoaTouchTabsStrippedTitle(const char* raw)
{
	if (!raw) return @"";
	char* stripped = iupStrProcessMnemonic(raw, NULL, 0);
	NSString* title = stripped ? [NSString stringWithUTF8String:stripped] : @"";
	if (stripped && stripped != raw) free(stripped);
	return title;
}


@interface IupCocoaTouchTabsTarget : NSObject
{
	Ihandle* _ihandle;
	int _previousIupPos;
}
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) int previousIupPos;
- (void)segmentChanged:(UISegmentedControl*)sender;
- (void)closeButtonTapped:(UIButton*)sender;
@end

static void cocoaTouchTabsRebuildCloseButtons(Ihandle* ih);
static BOOL cocoaTouchTabsShouldShowClose(Ihandle* ih, int iup_pos);

@implementation IupCocoaTouchTabsTarget

@synthesize ihandle = _ihandle;
@synthesize previousIupPos = _previousIupPos;

- (void)segmentChanged:(UISegmentedControl*)sender
{
	Ihandle* ih = _ihandle;
	if (!ih) return;
	if (iupAttribGet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE")) return;

	int native_pos = (int)sender.selectedSegmentIndex;
	int iup_pos = cocoaTouchTabsPosFixFromNative(ih, native_pos);
	int prev_pos = _previousIupPos;
	if (iup_pos == prev_pos) return;

	cocoaTouchTabsShowOnlyPage(ih, iup_pos);

	if (iup_pos != -1 && prev_pos != -1)
	{
		Ihandle* child = IupGetChild(ih, iup_pos);
		Ihandle* prev_child = IupGetChild(ih, prev_pos);
		IFnnn cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
		if (cb)
		{
			cb(ih, child, prev_child);
		}
		else
		{
			IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
			if (cb2) cb2(ih, iup_pos, prev_pos);
		}
	}
	_previousIupPos = iup_pos;
}

- (void)closeButtonTapped:(UIButton*)sender
{
	Ihandle* ih = _ihandle;
	if (!ih || !iupObjectCheck(ih)) return;

	int native_pos = (int)sender.tag;
	int iup_pos = cocoaTouchTabsPosFixFromNative(ih, native_pos);
	if (iup_pos < 0) return;

	Ihandle* child = IupGetChild(ih, iup_pos);
	if (!child) return;

	IFni cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
	int ret = cb ? cb(ih, iup_pos) : IUP_DEFAULT;

	if (ret == IUP_IGNORE) return;

	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);

	if (ret == IUP_CONTINUE)
	{
		IupDestroy(child);
		IupRefreshChildren(ih);
		cocoaTouchTabsRebuildCloseButtons(ih);
		if (root) [root setNeedsLayout];
		return;
	}

	Iarray* visible_array = cocoaTouchTabsGetVisibleArray(ih);
	int* visible_data = (int*)iupArrayGetData(visible_array);

	iupTabsCheckCurrentTab(ih, iup_pos, 0);
	visible_data[iup_pos] = 0;

	if (root && (NSUInteger)native_pos < root.segmentedControl.numberOfSegments)
	{
		iupAttribSet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE", "1");
		[root.segmentedControl removeSegmentAtIndex:native_pos animated:YES];
		iupAttribSet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE", NULL);
	}
	UIView* page = (UIView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
	if (page) page.hidden = YES;

	if (root)
	{
		int new_native = (int)root.segmentedControl.selectedSegmentIndex;
		_previousIupPos = (new_native == UISegmentedControlNoSegment) ? -1 : cocoaTouchTabsPosFixFromNative(ih, new_native);
	}

	cocoaTouchTabsRebuildCloseButtons(ih);
	if (root) [root setNeedsLayout];
}

@end


static UIImage* cocoaTouchTabsScaledImage(Ihandle* ih, UIImage* src)
{
	if (!src) return nil;
	int raw_w = (int)src.size.width;
	int raw_h = (int)src.size.height;
	if (raw_w <= 0 || raw_h <= 0) return src;
	int dst_w = raw_w, dst_h = raw_h;
	iupTabsScaleImageSize(ih, raw_w, raw_h, &dst_w, &dst_h);
	if (dst_w == raw_w && dst_h == raw_h) return src;

	CGSize target = CGSizeMake(dst_w, dst_h);
	UIGraphicsImageRenderer* renderer = [[[UIGraphicsImageRenderer alloc] initWithSize:target] autorelease];
	UIImage* scaled = [renderer imageWithActions:^(UIGraphicsImageRendererContext* ctx) {
		(void)ctx;
		[src drawInRect:CGRectMake(0, 0, target.width, target.height)];
	}];
	return [scaled imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
}

/* segments are title-OR-image only; composite [icon][text] into one UIImage when both are set */
static UIImage* cocoaTouchTabsBuildComposite(UISegmentedControl* sc, UIImage* image, NSString* title)
{
	if (!image || title.length == 0) return image;

	NSDictionary* attrs = [sc titleTextAttributesForState:UIControlStateNormal];
	UIFont* font = attrs[NSFontAttributeName] ?: [UIFont systemFontOfSize:[UIFont systemFontSize]];
	UIColor* color = attrs[NSForegroundColorAttributeName] ?: [UIColor labelColor];
	NSDictionary* draw_attrs = @{ NSFontAttributeName: font, NSForegroundColorAttributeName: color };

	const CGFloat spacing = 4.0;
	CGSize text_size = [title sizeWithAttributes:draw_attrs];
	CGSize total = CGSizeMake(image.size.width + spacing + ceil(text_size.width),
	                          MAX(image.size.height, ceil(text_size.height)));

	UIGraphicsImageRenderer* renderer = [[[UIGraphicsImageRenderer alloc] initWithSize:total] autorelease];
	UIImage* composite = [renderer imageWithActions:^(UIGraphicsImageRendererContext* ctx) {
		(void)ctx;
		CGFloat img_y = (total.height - image.size.height) / 2.0;
		[image drawInRect:CGRectMake(0, img_y, image.size.width, image.size.height)];
		CGFloat text_y = (total.height - text_size.height) / 2.0;
		[title drawAtPoint:CGPointMake(image.size.width + spacing, text_y) withAttributes:draw_attrs];
	}];
	return [composite imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
}

static void cocoaTouchTabsInsertSegment(IupCocoaTouchTabsView* root, int native_pos, NSString* title, UIImage* image)
{
	UISegmentedControl* sc = root.segmentedControl;
	if (image && title.length > 0)
	{
		[sc insertSegmentWithImage:cocoaTouchTabsBuildComposite(sc, image, title) atIndex:native_pos animated:NO];
	}
	else if (image)
	{
		[sc insertSegmentWithImage:image atIndex:native_pos animated:NO];
	}
	else
	{
		[sc insertSegmentWithTitle:(title ? title : @"") atIndex:native_pos animated:NO];
	}
}

static int cocoaTouchTabsCreateAndInsertItem(Ihandle* ih, Ihandle* child, int iup_pos)
{
	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root) return -1;

	int native_pos = cocoaTouchTabsPosFixToNative(ih, iup_pos);
	if (native_pos < 0) return -1;

	const char* title = iupAttribGet(child, "TABTITLE");
	if (!title) title = iupAttribGetId(ih, "TABTITLE", iup_pos);

	const char* image_name = iupAttribGet(child, "TABIMAGE");
	if (!image_name) image_name = iupAttribGetId(ih, "TABIMAGE", iup_pos);

	NSString* ns_title = cocoaTouchTabsStrippedTitle(title);
	UIImage* ns_image = nil;
	if (image_name)
	{
		void* img = iupImageGetImage(image_name, ih, 0, NULL);
		if (img && [(id)img isKindOfClass:[UIImage class]])
			ns_image = cocoaTouchTabsScaledImage(ih, (UIImage*)img);
	}

	iupAttribSet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE", "1");
	cocoaTouchTabsInsertSegment(root, native_pos, ns_title, ns_image);
	iupAttribSet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE", NULL);

	UIView* page = [[UIView alloc] initWithFrame:root.contentArea.bounds];
	page.autoresizingMask = UIViewAutoresizingNone;
	page.hidden = YES;
	page.clipsToBounds = YES;
	[root.contentArea addSubview:page];
	iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)page);

	cocoaTouchTabsRebuildCloseButtons(ih);
	[root setNeedsLayout];
	return 0;
}

static void cocoaTouchTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
	if (iupAttribGet(ih, "_IUPTABS_REORDERING")) return;

	if (!iupAttribGetHandleName(child)) iupAttribSetHandleName(child);

	if (!ih->handle) return;

	Iarray* visible_array = cocoaTouchTabsGetVisibleArray(ih);
	int pos = IupGetChildPos(ih, child);
	iupArrayInsert(visible_array, pos, 1);
	((int*)iupArrayGetData(visible_array))[pos] = 1;

	cocoaTouchTabsCreateAndInsertItem(ih, child, pos);
}

static void cocoaTouchTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
	if (iupAttribGet(ih, "_IUPTABS_REORDERING")) return;
	if (!ih->handle) return;

	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	Iarray* visible_array = cocoaTouchTabsGetVisibleArray(ih);
	int* visible_data = (int*)iupArrayGetData(visible_array);

	if (visible_data[pos])
	{
		int native_pos = cocoaTouchTabsPosFixToNative(ih, pos);
		iupTabsCheckCurrentTab(ih, pos, 1);
		if (native_pos >= 0 && root)
		{
			iupAttribSet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE", "1");
			[root.segmentedControl removeSegmentAtIndex:native_pos animated:NO];
			iupAttribSet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE", NULL);
		}
	}

	UIView* page = (UIView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
	if (page)
	{
		[page removeFromSuperview];
		iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
	}

	iupArrayRemove(visible_array, pos, 1);

	cocoaTouchTabsRebuildCloseButtons(ih);
	if (root) [root setNeedsLayout];
}

IUP_SDK_API int iupdrvTabsExtraDecor(Ihandle* ih)
{
	(void)ih;
	return 0;
}

IUP_SDK_API int iupdrvTabsExtraMargin(void)
{
	return 0;
}

IUP_SDK_API int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
	(void)ih;
	return 1;
}

IUP_SDK_API void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root) return;
	int native_pos = cocoaTouchTabsPosFixToNative(ih, pos);
	if (native_pos < 0 || (NSUInteger)native_pos >= root.segmentedControl.numberOfSegments) return;

	iupAttribSet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE", "1");
	root.segmentedControl.selectedSegmentIndex = native_pos;
	iupAttribSet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE", NULL);

	IupCocoaTouchTabsTarget* target = objc_getAssociatedObject(root, @"IUP_TABS_TARGET");
	if (target) target.previousIupPos = pos;

	cocoaTouchTabsShowOnlyPage(ih, pos);
}

IUP_SDK_API int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root) return -1;
	int native_pos = (int)root.segmentedControl.selectedSegmentIndex;
	if (native_pos == UISegmentedControlNoSegment) return -1;
	return cocoaTouchTabsPosFixFromNative(ih, native_pos);
}

IUP_SDK_API int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
	Ihandle* ih = IupGetParent(child);
	if (!ih) return 0;
	Iarray* visible_array = cocoaTouchTabsGetVisibleArray(ih);
	if (pos < 0 || pos >= iupArrayCount(visible_array)) return 0;
	return ((int*)iupArrayGetData(visible_array))[pos];
}

IUP_SDK_API void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
	(void)tab_image;
	int text_w = 0;
	int text_h = 0;
	if (tab_title && *tab_title)
	{
		text_w = iupdrvFontGetStringWidth(ih, tab_title);
		iupdrvFontGetCharSize(ih, NULL, &text_h);
	}
	if (tab_image)
	{
		int iw = 0, ih_img = 0;
		iupImageGetInfo(tab_image, &iw, &ih_img, NULL);
		iupTabsScaleImageSize(ih, iw, ih_img, &iw, &ih_img);
		text_w += iw + (tab_title ? 4 : 0);
		if (ih_img > text_h) text_h = ih_img;
	}
	(void)text_h;
	int extra = 24;
	if (ih->data->show_close) extra += 22;
	if (tab_width)  *tab_width  = text_w + extra;
	if (tab_height) *tab_height = (int)kIupCocoaTouchTabBarHeight - 16;
}

static int cocoaTouchTabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
	/* no vertical segmented control on UIKit; LEFT/RIGHT clamp to TOP */
	if (iupStrEqualNoCase(value, "BOTTOM"))
	{
		ih->data->type = ITABS_BOTTOM;
		ih->data->is_multiline = 0;
	}
	else if (iupStrEqualNoCase(value, "LEFT"))
	{
		ih->data->type = ITABS_TOP;
	}
	else if (iupStrEqualNoCase(value, "RIGHT"))
	{
		ih->data->type = ITABS_TOP;
	}
	else
	{
		ih->data->type = ITABS_TOP;
		ih->data->is_multiline = 0;
	}

	if (ih->handle)
	{
		[(UIView*)ih->handle setNeedsLayout];
	}
	return 0;
}


/* re-apply title+image via composite path; segmented setTitle:/setImage: drops the other side */
static void cocoaTouchTabsApplyTitleImage(Ihandle* ih, int pos)
{
	int native_pos = cocoaTouchTabsPosFixToNative(ih, pos);
	if (native_pos < 0) return;

	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root || (NSUInteger)native_pos >= root.segmentedControl.numberOfSegments) return;

	Ihandle* child = IupGetChild(ih, pos);
	const char* title = child ? iupAttribGet(child, "TABTITLE") : NULL;
	if (!title) title = iupAttribGetId(ih, "TABTITLE", pos);
	const char* image_name = child ? iupAttribGet(child, "TABIMAGE") : NULL;
	if (!image_name) image_name = iupAttribGetId(ih, "TABIMAGE", pos);

	UIImage* img = nil;
	if (image_name)
	{
		void* raw = iupImageGetImage(image_name, ih, 0, NULL);
		if (raw && [(id)raw isKindOfClass:[UIImage class]]) img = cocoaTouchTabsScaledImage(ih, (UIImage*)raw);
	}
	NSString* ns_title = cocoaTouchTabsStrippedTitle(title);

	UISegmentedControl* sc = root.segmentedControl;
	if (img && ns_title.length > 0)
		[sc setImage:cocoaTouchTabsBuildComposite(sc, img, ns_title) forSegmentAtIndex:native_pos];
	else if (img)
		[sc setImage:img forSegmentAtIndex:native_pos];
	else
		[sc setTitle:ns_title forSegmentAtIndex:native_pos];
}

static int cocoaTouchTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
	Ihandle* child = IupGetChild(ih, pos);
	if (child) iupAttribSetStr(child, "TABTITLE", value);
	cocoaTouchTabsApplyTitleImage(ih, pos);
	return 0;
}

static int cocoaTouchTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
	Ihandle* child = IupGetChild(ih, pos);
	if (child) iupAttribSetStr(child, "TABIMAGE", value);
	cocoaTouchTabsApplyTitleImage(ih, pos);
	return 1;
}

static int cocoaTouchTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
	Ihandle* child = IupGetChild(ih, pos);
	if (!child) return 0;

	int is_visible = iupdrvTabsIsTabVisible(child, pos);
	int new_visible = iupStrBoolean(value);
	if (is_visible == new_visible) return 0;

	Iarray* visible_array = cocoaTouchTabsGetVisibleArray(ih);
	int* visible_data = (int*)iupArrayGetData(visible_array);
	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);

	if (new_visible)
	{
		visible_data[pos] = 1;
		cocoaTouchTabsCreateAndInsertItem(ih, child, pos);
	}
	else
	{
		int native_pos = cocoaTouchTabsPosFixToNative(ih, pos);
		iupTabsCheckCurrentTab(ih, pos, 0);
		visible_data[pos] = 0;
		if (native_pos >= 0 && root && (NSUInteger)native_pos < root.segmentedControl.numberOfSegments)
		{
			iupAttribSet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE", "1");
			[root.segmentedControl removeSegmentAtIndex:native_pos animated:NO];
			iupAttribSet(ih, "_IUPCOCOATOUCH_IGNORE_CHANGE", NULL);
		}
		UIView* page = (UIView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
		if (page) page.hidden = YES;
	}
	cocoaTouchTabsRebuildCloseButtons(ih);
	if (root) [root setNeedsLayout];
	return 0;
}

static BOOL cocoaTouchTabsShouldShowClose(Ihandle* ih, int iup_pos)
{
	Ihandle* child = IupGetChild(ih, iup_pos);
	if (child)
	{
		char* per_child = iupAttribGet(child, "SHOWCLOSE");
		if (per_child) return iupStrBoolean(per_child) ? YES : NO;
	}
	char* per_id = iupAttribGetId(ih, "SHOWCLOSE", iup_pos);
	if (per_id) return iupStrBoolean(per_id) ? YES : NO;
	return ih->data->show_close ? YES : NO;
}

static void cocoaTouchTabsLayoutCloseButtons(IupCocoaTouchTabsView* root)
{
	NSUInteger n = root.segmentedControl.numberOfSegments;
	if (n == 0 || root.closeButtons.count == 0) return;
	CGRect bar = root.segmentedControl.frame;
	if (bar.size.width <= 0) return;
	CGFloat seg_w = bar.size.width / (CGFloat)n;
	const CGFloat btn_size = 18.0;
	const CGFloat btn_margin = 4.0;
	for (UIButton* btn in root.closeButtons)
	{
		NSInteger native_pos = btn.tag;
		if (native_pos < 0 || (NSUInteger)native_pos >= n) { btn.hidden = YES; continue; }
		btn.hidden = NO;
		CGFloat seg_x = bar.origin.x + (CGFloat)native_pos * seg_w;
		CGFloat btn_x = seg_x + seg_w - btn_size - btn_margin;
		CGFloat btn_y = bar.origin.y + (bar.size.height - btn_size) / 2.0;
		btn.frame = CGRectMake(btn_x, btn_y, btn_size, btn_size);
	}
}

static void cocoaTouchTabsRebuildCloseButtons(Ihandle* ih)
{
	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root) return;

	for (UIButton* btn in root.closeButtons) [btn removeFromSuperview];
	[root.closeButtons removeAllObjects];

	IupCocoaTouchTabsTarget* target = objc_getAssociatedObject(root, @"IUP_TABS_TARGET");
	if (!target) return;

	Iarray* visible_array = cocoaTouchTabsGetVisibleArray(ih);
	int count = iupArrayCount(visible_array);
	int* visible_data = (int*)iupArrayGetData(visible_array);

	int native_pos = 0;
	UIImage* xmark = [UIImage systemImageNamed:@"xmark.circle.fill"];
	for (int i = 0; i < count; i++)
	{
		if (!visible_data[i]) continue;
		if (cocoaTouchTabsShouldShowClose(ih, i))
		{
			UIButton* btn = [UIButton buttonWithType:UIButtonTypeSystem];
			btn.tag = native_pos;
			[btn setImage:xmark forState:UIControlStateNormal];
			btn.tintColor = [UIColor secondaryLabelColor];
			[btn addTarget:target action:@selector(closeButtonTapped:) forControlEvents:UIControlEventTouchUpInside];
			[root addSubview:btn];
			[root.closeButtons addObject:btn];
		}
		native_pos++;
	}

	cocoaTouchTabsLayoutCloseButtons(root);
}

static int cocoaTouchTabsSetShowCloseAttrib(Ihandle* ih, int pos, const char* value)
{
	if (pos == IUP_INVALID_ID)
	{
		ih->data->show_close = iupStrBoolean(value);
	}
	else
	{
		Ihandle* child = IupGetChild(ih, pos);
		if (child) iupAttribSetStr(child, "SHOWCLOSE", value);
	}
	cocoaTouchTabsRebuildCloseButtons(ih);
	if (ih->handle) [(UIView*)ih->handle setNeedsLayout];
	return 1;
}

static int cocoaTouchTabsSetBgColorAttrib(Ihandle* ih, const char* value)
{
	UIColor* color = iupCocoaTouchToNativeColor(value);
	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root || !color) return 0;
	root.backgroundColor = color;
	root.contentArea.backgroundColor = color;
	return 1;
}

static int cocoaTouchTabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
	UIColor* color = iupCocoaTouchToNativeColor(value);
	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root || !color) return 0;
	NSDictionary* attrs = @{ NSForegroundColorAttributeName: color };
	[root.segmentedControl setTitleTextAttributes:attrs forState:UIControlStateNormal];
	[root.segmentedControl setTitleTextAttributes:attrs forState:UIControlStateSelected];
	return 1;
}

static int cocoaTouchTabsSetFontAttrib(Ihandle* ih, const char* value)
{
	if (!iupdrvSetFontAttrib(ih, value)) return 0;
	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root) return 1;

	IupCocoaTouchFont* font = iupCocoaTouchGetFont(ih);
	if (font && font.nativeFont)
	{
		NSMutableDictionary* attrs = [NSMutableDictionary dictionary];
		attrs[NSFontAttributeName] = font.nativeFont;
		[root.segmentedControl setTitleTextAttributes:attrs forState:UIControlStateNormal];
		[root.segmentedControl setTitleTextAttributes:attrs forState:UIControlStateSelected];
	}
	return 1;
}

@interface IupCocoaTouchTabsReorderGR : NSObject
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) UISegmentedControl* segmentedControl;
@property(nonatomic, assign) NSInteger sourceNativeIndex;
@end

static void cocoaTouchTabsDoReorder(Ihandle* ih, int src_iup_pos, int tgt_iup_pos)
{
	if (src_iup_pos == tgt_iup_pos) return;

	IFnii cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
	if (cb && cb(ih, src_iup_pos, tgt_iup_pos) == IUP_IGNORE) return;

	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root) return;
	UISegmentedControl* sc = root.segmentedControl;

	int src_native = cocoaTouchTabsPosFixToNative(ih, src_iup_pos);
	int tgt_native = cocoaTouchTabsPosFixToNative(ih, tgt_iup_pos);
	if (src_native < 0 || tgt_native < 0) return;

	NSString* title = [sc titleForSegmentAtIndex:src_native] ?: @"";
	UIImage* image = [sc imageForSegmentAtIndex:src_native];

	[sc removeSegmentAtIndex:src_native animated:NO];
	if (image)
	{
		[sc insertSegmentWithImage:image atIndex:tgt_native animated:NO];
		if (title.length > 0) [sc setTitle:nil forSegmentAtIndex:tgt_native];
	}
	else
	{
		[sc insertSegmentWithTitle:title atIndex:tgt_native animated:NO];
	}

	Ihandle* source_child = IupGetChild(ih, src_iup_pos);
	if (source_child)
	{
		Ihandle* ref_child = (src_iup_pos < tgt_iup_pos) ? IupGetChild(ih, tgt_iup_pos + 1)
		                                                 : IupGetChild(ih, tgt_iup_pos);
		iupAttribSet(ih, "_IUPTABS_REORDERING", "1");
		IupReparent(source_child, ih, ref_child);
		iupAttribSet(ih, "_IUPTABS_REORDERING", NULL);
	}

	Iarray* visible_array = cocoaTouchTabsGetVisibleArray(ih);
	int* visible_data = (int*)iupArrayGetData(visible_array);
	int total = iupArrayCount(visible_array);
	if (src_iup_pos >= 0 && src_iup_pos < total && tgt_iup_pos >= 0 && tgt_iup_pos < total)
	{
		int v = visible_data[src_iup_pos];
		if (src_iup_pos < tgt_iup_pos)
		{
			for (int i = src_iup_pos; i < tgt_iup_pos; i++) visible_data[i] = visible_data[i + 1];
		}
		else
		{
			for (int i = src_iup_pos; i > tgt_iup_pos; i--) visible_data[i] = visible_data[i - 1];
		}
		visible_data[tgt_iup_pos] = v;
	}

	[sc setSelectedSegmentIndex:tgt_native];
}

@implementation IupCocoaTouchTabsReorderGR

- (instancetype)init
{
	self = [super init];
	if (self) _sourceNativeIndex = -1;
	return self;
}

- (NSInteger)segmentAtPoint:(CGPoint)pt
{
	UISegmentedControl* sc = self.segmentedControl;
	if (!sc) return -1;
	NSInteger n = sc.numberOfSegments;
	if (n <= 0) return -1;
	CGFloat seg_w = sc.bounds.size.width / (CGFloat)n;
	if (seg_w <= 0) return -1;
	NSInteger idx = (NSInteger)(pt.x / seg_w);
	if (idx < 0) idx = 0;
	if (idx >= n) idx = n - 1;
	return idx;
}

- (void)handleLongPress:(UILongPressGestureRecognizer*)gr
{
	if (!self.ihandle || !iupObjectCheck(self.ihandle)) return;

	if (gr.state == UIGestureRecognizerStateBegan)
	{
		_sourceNativeIndex = [self segmentAtPoint:[gr locationInView:self.segmentedControl]];
	}
	else if (gr.state == UIGestureRecognizerStateEnded)
	{
		NSInteger target = [self segmentAtPoint:[gr locationInView:self.segmentedControl]];
		if (_sourceNativeIndex >= 0 && target >= 0 && _sourceNativeIndex != target)
		{
			int src_iup = cocoaTouchTabsPosFixFromNative(self.ihandle, (int)_sourceNativeIndex);
			int tgt_iup = cocoaTouchTabsPosFixFromNative(self.ihandle, (int)target);
			if (src_iup >= 0 && tgt_iup >= 0)
				cocoaTouchTabsDoReorder(self.ihandle, src_iup, tgt_iup);
		}
		_sourceNativeIndex = -1;
	}
	else if (gr.state == UIGestureRecognizerStateCancelled || gr.state == UIGestureRecognizerStateFailed)
	{
		_sourceNativeIndex = -1;
	}
}

@end

static int cocoaTouchTabsSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root) return 1;
	UISegmentedControl* sc = root.segmentedControl;

	UILongPressGestureRecognizer* existing = objc_getAssociatedObject(sc, @"IUP_TABS_REORDER_GR");

	if (iupStrBoolean(value))
	{
		if (existing) return 1;
		IupCocoaTouchTabsReorderGR* handler = [[[IupCocoaTouchTabsReorderGR alloc] init] autorelease];
		handler.ihandle = ih;
		handler.segmentedControl = sc;
		UILongPressGestureRecognizer* gr = [[[UILongPressGestureRecognizer alloc]
			initWithTarget:handler action:@selector(handleLongPress:)] autorelease];
		gr.minimumPressDuration = 0.4;
		[sc addGestureRecognizer:gr];
		objc_setAssociatedObject(sc, @"IUP_TABS_REORDER_GR",      gr,      OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(sc, @"IUP_TABS_REORDER_HANDLER", handler, OBJC_ASSOCIATION_RETAIN);
	}
	else if (existing)
	{
		[sc removeGestureRecognizer:existing];
		objc_setAssociatedObject(sc, @"IUP_TABS_REORDER_GR",      nil, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(sc, @"IUP_TABS_REORDER_HANDLER", nil, OBJC_ASSOCIATION_RETAIN);
	}
	return 1;
}

static int cocoaTouchTabsMapMethod(Ihandle* ih)
{
	IupCocoaTouchTabsView* root = [[IupCocoaTouchTabsView alloc] initWithFrame:CGRectZero];
	root.ihandle = ih;
	ih->handle = root;
	objc_setAssociatedObject(root, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

	IupCocoaTouchTabsTarget* target = [[IupCocoaTouchTabsTarget alloc] init];
	target.ihandle = ih;
	target.previousIupPos = -1;
	[root.segmentedControl addTarget:target action:@selector(segmentChanged:) forControlEvents:UIControlEventValueChanged];
	objc_setAssociatedObject(root, @"IUP_TABS_TARGET", target, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	[target release];

	cocoaTouchTabsGetVisibleArray(ih);

	iupCocoaTouchAddToParent(ih);

	if (ih->firstchild)
	{
		Ihandle* current_child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");
		int current_pos = -1;
		int i = 0;
		for (Ihandle* c = ih->firstchild; c; c = c->brother)
		{
			if (c == current_child) { current_pos = i; break; }
			i++;
		}

		for (Ihandle* child = ih->firstchild; child; child = child->brother)
		{
			cocoaTouchTabsChildAddedMethod(ih, child);
		}

		if (current_pos == -1) current_pos = 0;
		iupdrvTabsSetCurrentTab(ih, current_pos);
		iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", NULL);
	}

	cocoaTouchTabsRebuildCloseButtons(ih);

	return IUP_NOERROR;
}

static void cocoaTouchTabsUnMapMethod(Ihandle* ih)
{
	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root) return;

	for (Ihandle* child = ih->firstchild; child; child = child->brother)
	{
		UIView* page = (UIView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
		if (page)
		{
			[page removeFromSuperview];
			iupAttribSet(child, "_IUPTAB_CONTAINER", NULL);
		}
	}

	Iarray* visible_array = (Iarray*)iupAttribGet(ih, "_IUPCOCOATOUCH_VISIBLEARRAY");
	if (visible_array)
	{
		iupArrayDestroy(visible_array);
		iupAttribSet(ih, "_IUPCOCOATOUCH_VISIBLEARRAY", NULL);
	}

	objc_setAssociatedObject(root, @"IUP_TABS_TARGET", nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

	iupCocoaTouchRemoveFromParent(ih);
	[root release];
	ih->handle = NULL;
}

static void cocoaTouchTabsLayoutUpdateMethod(Ihandle* ih)
{
	iupdrvBaseLayoutUpdateMethod(ih);

	IupCocoaTouchTabsView* root = cocoaTouchTabsGetRoot(ih);
	if (!root) return;

	CGFloat w = ih->currentwidth;
	CGFloat h = ih->currentheight;
	CGFloat bar_h = kIupCocoaTouchTabBarHeight;
	if (bar_h > h) bar_h = h;

	CGFloat bar_y = (ih->data->type == ITABS_BOTTOM) ? (h - bar_h) : 0;
	CGFloat content_y = (ih->data->type == ITABS_BOTTOM) ? 0 : bar_h;
	CGFloat content_h = h - bar_h;
	if (content_h < 0) content_h = 0;

	root.segmentedControl.frame = CGRectMake(0, bar_y, w, bar_h);
	root.contentArea.frame = CGRectMake(0, content_y, w, content_h);

	CGRect page_rect = CGRectMake(0, 0, w, content_h);
	for (Ihandle* child = ih->firstchild; child; child = child->brother)
	{
		UIView* page = (UIView*)iupAttribGet(child, "_IUPTAB_CONTAINER");
		if (page) page.frame = page_rect;
	}

	cocoaTouchTabsLayoutCloseButtons(root);
}

IUP_SDK_API void iupdrvTabsInitClass(Iclass* ic)
{
	ic->Map = cocoaTouchTabsMapMethod;
	ic->UnMap = cocoaTouchTabsUnMapMethod;
	ic->LayoutUpdate = cocoaTouchTabsLayoutUpdateMethod;
	ic->ChildAdded = cocoaTouchTabsChildAddedMethod;
	ic->ChildRemoved = cocoaTouchTabsChildRemovedMethod;

	iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");

	iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, cocoaTouchTabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, NULL, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_READONLY|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

	iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, cocoaTouchTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "TABIMAGE", NULL, cocoaTouchTabsSetTabImageAttrib, IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, cocoaTouchTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "TABTIP", NULL, NULL, IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "FONT", NULL, cocoaTouchTabsSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);
	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTouchTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTouchTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

	iupClassRegisterAttribute(ic, "TABPADDING", iupTabsGetTabPaddingAttrib, NULL, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MULTILINE", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ALLOWREORDER", NULL, cocoaTouchTabsSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "SHOWCLOSE", NULL, cocoaTouchTabsSetShowCloseAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SHOWCLOSEONHOVER", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_SUPPORTED|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TABLIST", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_SUPPORTED|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
}
