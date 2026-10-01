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
		IupSetStrAttribute(sib, "VALUE", "OFF");
	}
	IupSetStrAttribute(item_ih, "VALUE", "ON");
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

static void cocoaTouchMenuFireOpen(Ihandle* menu_ih)
{
	if (!menu_ih || !iupObjectCheck(menu_ih)) return;
	Icallback cb = IupGetCallback(menu_ih, "MENUOPEN_CB");
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
	const char* name = NULL;
	if (iupAttribGetBoolean(item_ih, "VALUE"))
		name = iupAttribGet(item_ih, "IMPRESS");
	if (!name) name = iupAttribGet(item_ih, "IMAGE");
	if (!name) name = iupAttribGet(item_ih, "TITLEIMAGE");
	if (!name) return nil;
	return (UIImage*)iupImageGetImage(name, item_ih, 0, NULL);
}


static UIMenu* cocoaTouchMenuBuildUIMenu(Ihandle* menu_ih);

static UIMenuElement* cocoaTouchMenuBuildChildElement(Ihandle* child)
{
	if (!child || !child->iclass || !child->iclass->name) return nil;
	const char* cname = child->iclass->name;

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

	NSMutableArray<NSMutableArray<UIMenuElement*>*>* sections = [NSMutableArray array];
	NSMutableArray<UIMenuElement*>* section = [NSMutableArray array];
	cocoaTouchMenuAppendRecentUIActions(section, menu_ih);
	int count = IupGetChildCount(menu_ih);
	for (int i = 0; i < count; i++)
	{
		Ihandle* child = IupGetChild(menu_ih, i);
		if (child && child->iclass && iupStrEqual(child->iclass->name, "menuseparator"))
		{
			if (section.count)
			{
				[sections addObject:section];
				section = [NSMutableArray array];
			}
			continue;
		}
		UIMenuElement* element = cocoaTouchMenuBuildChildElement(child);
		if (element) [section addObject:element];
	}
	if (section.count) [sections addObject:section];

	/* UIKit draws a divider between inline sections */
	NSMutableArray<UIMenuElement*>* elements = [NSMutableArray array];
	if (sections.count == 1)
		[elements addObjectsFromArray:sections[0]];
	else
	{
		for (NSArray<UIMenuElement*>* items in sections)
			[elements addObject:[UIMenu menuWithTitle:@"" image:nil identifier:nil options:UIMenuOptionsDisplayInline children:items]];
	}

	return [UIMenu menuWithTitle:@""
	                       image:nil
	                  identifier:nil
	                     options:0
	                    children:elements];
}


@interface IupCocoaTouchMenuRow : NSObject
@property(nonatomic, assign) Ihandle* item;
@property(nonatomic, assign) int recentIndex;
@property(nonatomic, assign) BOOL isBack;
@end

@implementation IupCocoaTouchMenuRow
@end

@interface IupCocoaTouchMenuPopupVC : UIViewController <UITableViewDataSource, UITableViewDelegate, UIPopoverPresentationControllerDelegate>
@property(nonatomic, assign) Ihandle* picked;
@property(nonatomic, assign) int pickedRecent;
@property(nonatomic, assign) Ihandle* pickedRecentMenu;
@property(nonatomic, assign) BOOL done;
- (instancetype)initWithMenu:(Ihandle*)menu_ih;
- (NSArray<NSValue*>*)openMenus;
- (void)placeAt:(CGPoint)anchor inView:(UIView*)view;
@end

@implementation IupCocoaTouchMenuPopupVC
{
	NSMutableArray<NSValue*>* _levels;
	NSMutableArray<NSArray<IupCocoaTouchMenuRow*>*>* _sections;
	BOOL _hasCheck;
	UITableView* _table;
	NSIndexPath* _focused;
	UIView* _sourceView;
	CGPoint _anchor;
}

- (instancetype)initWithMenu:(Ihandle*)menu_ih
{
	self = [super initWithNibName:nil bundle:nil];
	if (self)
	{
		_levels = [[NSMutableArray alloc] initWithObjects:[NSValue valueWithPointer:menu_ih], nil];
		_sections = [[NSMutableArray alloc] init];
		_pickedRecent = -1;
		self.modalPresentationStyle = UIModalPresentationPopover;
	}
	return self;
}

- (void)dealloc
{
	[_levels release];
	[_sections release];
	[_table release];
	[_focused release];
	[super dealloc];
}

- (NSArray<NSValue*>*)openMenus
{
	return _levels;
}

- (Ihandle*)currentMenu
{
	return (Ihandle*)[[_levels lastObject] pointerValue];
}

static BOOL cocoaTouchMenuRowEnabled(IupCocoaTouchMenuRow* row)
{
	if (row.isBack || row.recentIndex >= 0) return YES;
	return !(iupAttribGet(row.item, "ACTIVE") && !iupAttribGetBoolean(row.item, "ACTIVE"));
}

static IupCocoaTouchMenuRow* cocoaTouchMenuNewRow(Ihandle* item, int recent_index, BOOL is_back)
{
	IupCocoaTouchMenuRow* row = [[[IupCocoaTouchMenuRow alloc] init] autorelease];
	row.item = item;
	row.recentIndex = recent_index;
	row.isBack = is_back;
	return row;
}

- (void)rebuildRows
{
	Ihandle* menu_ih = [self currentMenu];
	[_sections removeAllObjects];
	_hasCheck = iupAttribGetBoolean(menu_ih, "RADIO") ? YES : NO;

	NSMutableArray<IupCocoaTouchMenuRow*>* section = [NSMutableArray array];
	if (_levels.count > 1)
	{
		[section addObject:cocoaTouchMenuNewRow(menu_ih->parent, -1, YES)];
		[_sections addObject:section];
		section = [NSMutableArray array];
	}

	int recent_count = iupAttribGetInt(menu_ih, "_IUP_RECENT_COUNT");
	for (int i = 0; i < recent_count; i++)
	{
		char attr[32];
		snprintf(attr, sizeof(attr), "_IUP_RECENT_FILE%d", i);
		const char* path = iupAttribGet(menu_ih, attr);
		if (path && *path)
			[section addObject:cocoaTouchMenuNewRow(NULL, i, NO)];
	}

	int count = IupGetChildCount(menu_ih);
	for (int i = 0; i < count; i++)
	{
		Ihandle* child = IupGetChild(menu_ih, i);
		if (!child || !child->iclass || !child->iclass->name) continue;
		const char* cname = child->iclass->name;
		if (iupStrEqual(cname, "menuseparator"))
		{
			if (section.count)
			{
				[_sections addObject:section];
				section = [NSMutableArray array];
			}
			continue;
		}
		if (!iupStrEqual(cname, "menuitem") && !iupStrEqual(cname, "submenu")) continue;
		if (iupStrEqual(cname, "menuitem") && iupAttribGet(child, "VALUE") && !iupAttribGetBoolean(child, "HIDEMARK"))
			_hasCheck = YES;
		[section addObject:cocoaTouchMenuNewRow(child, -1, NO)];
	}
	if (section.count)
		[_sections addObject:section];
}

- (NSString*)titleForRow:(IupCocoaTouchMenuRow*)row
{
	if (row.recentIndex >= 0)
	{
		char attr[32];
		snprintf(attr, sizeof(attr), "_IUP_RECENT_FILE%d", row.recentIndex);
		const char* path = iupAttribGet([self currentMenu], attr);
		return path ? [NSString stringWithUTF8String:path] : @"";
	}
	return cocoaTouchMenuItemDisplayTitle(row.item);
}

- (void)updatePreferredSize
{
	UIFont* font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
	UIFont* bold = [UIFont boldSystemFontOfSize:font.pointSize];
	CGFloat text_w = 0;
	CGFloat height = 0;
	for (NSUInteger s = 0; s < _sections.count; s++)
	{
		if (s > 0) height += 8;
		for (IupCocoaTouchMenuRow* row in _sections[s])
		{
			NSDictionary* attrs = @{ NSFontAttributeName: row.isBack ? bold : font };
			CGFloat w = ceil([[self titleForRow:row] sizeWithAttributes:attrs].width);
			if (w > text_w) text_w = w;
			height += 44;
		}
	}

	/* leading inset and check or back column, trailing icon or chevron */
	CGFloat width = text_w + 16 + ((_hasCheck || _levels.count > 1) ? 32 : 0) + 44;
	UIWindow* window = iupCocoaTouchFindCurrentWindow();
	CGFloat max_w = window ? MIN(320, window.bounds.size.width - 32) : 320;
	CGFloat max_h = window ? window.bounds.size.height * 0.7 : 480;
	CGSize size = CGSizeMake(MAX(200, MIN(width, max_w)), MIN(height + 16, max_h));
	[self placeForSize:size];
	self.preferredContentSize = size;
}

- (void)placeAt:(CGPoint)anchor inView:(UIView*)view
{
	_sourceView = view;
	_anchor = anchor;
	[self placeForSize:self.preferredContentSize];
}

/* x,y is the top-left of the menu, it flips above the point when only that side fits;
   with no arrow direction the popover has no arrow and is centered on the source rect */
- (void)placeForSize:(CGSize)size
{
	if (!_sourceView) return;
	CGRect safe = CGRectInset(UIEdgeInsetsInsetRect(_sourceView.bounds, _sourceView.safeAreaInsets), 8, 8);
	CGFloat below = CGRectGetMaxY(safe) - _anchor.y;
	CGFloat above = _anchor.y - CGRectGetMinY(safe);
	BOOL place_below = below >= size.height || (above < size.height && below >= above);
	CGFloat left = MAX(CGRectGetMinX(safe), MIN(_anchor.x, CGRectGetMaxX(safe) - size.width));
	CGFloat top = place_below ? _anchor.y : _anchor.y - size.height;
	top = MAX(CGRectGetMinY(safe), MIN(top, CGRectGetMaxY(safe) - size.height));

	UIPopoverPresentationController* ppc = [self popoverPresentationController];
	ppc.sourceView = _sourceView;
	ppc.sourceRect = CGRectMake(left + size.width / 2, top + size.height / 2, 1, 1);
	ppc.permittedArrowDirections = 0;
}

- (void)loadView
{
	_table = [[UITableView alloc] initWithFrame:CGRectZero style:UITableViewStylePlain];
	_table.dataSource = self;
	_table.delegate = self;
	_table.backgroundColor = [UIColor clearColor];
	_table.rowHeight = 44;
	_table.alwaysBounceVertical = NO;
	_table.contentInset = UIEdgeInsetsMake(8, 0, 8, 0);
	_table.sectionHeaderTopPadding = 0;
	_table.separatorInset = UIEdgeInsetsMake(0, 16, 0, 0);
	[_table registerClass:[UITableViewCell class] forCellReuseIdentifier:@"row"];
	self.view = _table;
	[self rebuildRows];
	[self updatePreferredSize];
}

- (void)viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];
	/* keyboard navigation, but a focused text input keeps its focus */
	if (![iupCocoaTouchKeyFirstResponder() conformsToProtocol:@protocol(UIKeyInput)])
		[self becomeFirstResponder];
}

- (BOOL)canBecomeFirstResponder
{
	return YES;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView*)tableView
{
	(void)tableView;
	return (NSInteger)_sections.count;
}

- (NSInteger)tableView:(UITableView*)tableView numberOfRowsInSection:(NSInteger)section
{
	(void)tableView;
	return (NSInteger)_sections[(NSUInteger)section].count;
}

- (CGFloat)tableView:(UITableView*)tableView heightForHeaderInSection:(NSInteger)section
{
	(void)tableView;
	return section > 0 ? 8 : 0;
}

- (UIView*)tableView:(UITableView*)tableView viewForHeaderInSection:(NSInteger)section
{
	(void)tableView;
	if (section == 0) return nil;
	UIView* gap = [[[UIView alloc] init] autorelease];
	gap.backgroundColor = [UIColor tertiarySystemFillColor];
	return gap;
}

- (IupCocoaTouchMenuRow*)rowAt:(NSIndexPath*)path
{
	return _sections[(NSUInteger)path.section][(NSUInteger)path.row];
}

- (UITableViewCell*)tableView:(UITableView*)tableView cellForRowAtIndexPath:(NSIndexPath*)path
{
	UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:@"row" forIndexPath:path];
	IupCocoaTouchMenuRow* row = [self rowAt:path];
	BOOL enabled = cocoaTouchMenuRowEnabled(row);
	BOOL is_submenu = row.item && !row.isBack && iupStrEqual(row.item->iclass->name, "submenu");

	UIListContentConfiguration* content = [UIListContentConfiguration cellConfiguration];
	content.text = [self titleForRow:row];
	content.textProperties.numberOfLines = 1;
	content.textProperties.color = enabled ? [UIColor labelColor] : [UIColor tertiaryLabelColor];
	content.imageProperties.reservedLayoutSize = CGSizeMake(20, 20);
	content.imageProperties.tintColor = [UIColor labelColor];
	if (row.isBack)
	{
		content.textProperties.font = [UIFont boldSystemFontOfSize:[UIFont preferredFontForTextStyle:UIFontTextStyleBody].pointSize];
		content.image = [UIImage systemImageNamed:@"chevron.backward"];
	}
	else if (_hasCheck || _levels.count > 1)
	{
		const char* value = row.item ? iupAttribGet(row.item, "VALUE") : NULL;
		BOOL checked = value && iupStrEqualNoCase(value, "ON") && !iupAttribGetBoolean(row.item, "HIDEMARK");
		content.image = [UIImage systemImageNamed:@"checkmark"];
		if (!checked)
			content.imageProperties.tintColor = [UIColor clearColor];
	}
	cell.contentConfiguration = content;
	cell.backgroundColor = [UIColor clearColor];
	BOOL last = (NSUInteger)path.row + 1 == _sections[(NSUInteger)path.section].count;
	cell.separatorInset = UIEdgeInsetsMake(0, last ? 10000 : 16, 0, 0);
	cell.selectionStyle = enabled ? UITableViewCellSelectionStyleDefault : UITableViewCellSelectionStyleNone;

	UIImage* image = (row.item && !row.isBack) ? cocoaTouchMenuResolveImage(row.item) : nil;
	if (is_submenu)
	{
		UIImageView* chevron = [[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"chevron.forward"]] autorelease];
		chevron.tintColor = enabled ? [UIColor secondaryLabelColor] : [UIColor tertiaryLabelColor];
		cell.accessoryView = chevron;
	}
	else if (image)
	{
		UIImageView* icon = [[[UIImageView alloc] initWithImage:image] autorelease];
		icon.frame = CGRectMake(0, 0, 22, 22);
		icon.contentMode = UIViewContentModeScaleAspectFit;
		icon.alpha = enabled ? 1.0 : 0.4;
		cell.accessoryView = icon;
	}
	else
		cell.accessoryView = nil;
	return cell;
}

- (NSIndexPath*)tableView:(UITableView*)tableView willSelectRowAtIndexPath:(NSIndexPath*)path
{
	(void)tableView;
	return cocoaTouchMenuRowEnabled([self rowAt:path]) ? path : nil;
}

- (void)reloadLevel
{
	[_focused release];
	_focused = nil;
	[self rebuildRows];
	[_table reloadData];
	[self updatePreferredSize];
}

- (void)finish
{
	[self dismissViewControllerAnimated:YES completion:^{ self.done = YES; }];
}

- (void)activateRow:(IupCocoaTouchMenuRow*)row
{
	if (!cocoaTouchMenuRowEnabled(row)) return;

	if (row.isBack)
	{
		cocoaTouchMenuFireClose([self currentMenu]);
		[_levels removeLastObject];
		[self reloadLevel];
		return;
	}

	if (row.recentIndex >= 0)
	{
		_pickedRecent = row.recentIndex;
		_pickedRecentMenu = [self currentMenu];
		[self finish];
		return;
	}

	if (iupStrEqual(row.item->iclass->name, "submenu"))
	{
		Ihandle* sub = IupGetChild(row.item, 0);
		if (!sub) return;
		[_levels addObject:[NSValue valueWithPointer:sub]];
		cocoaTouchMenuFireOpen(sub);
		[self reloadLevel];
		return;
	}

	_picked = row.item;
	[self finish];
}

- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)path
{
	[tableView deselectRowAtIndexPath:path animated:YES];
	[self activateRow:[self rowAt:path]];
}

- (void)moveFocus:(int)delta
{
	NSMutableArray<NSIndexPath*>* paths = [NSMutableArray array];
	for (NSUInteger s = 0; s < _sections.count; s++)
		for (NSUInteger r = 0; r < _sections[s].count; r++)
			if (cocoaTouchMenuRowEnabled(_sections[s][r]))
				[paths addObject:[NSIndexPath indexPathForRow:(NSInteger)r inSection:(NSInteger)s]];
	if (!paths.count) return;

	NSUInteger idx = _focused ? [paths indexOfObject:_focused] : NSNotFound;
	if (idx == NSNotFound)
		idx = delta > 0 ? 0 : paths.count - 1;
	else
		idx = (idx + paths.count + (NSUInteger)(delta > 0 ? 1 : paths.count - 1)) % paths.count;

	[_focused release];
	_focused = [paths[idx] retain];
	[_table selectRowAtIndexPath:_focused animated:NO scrollPosition:UITableViewScrollPositionNone];
}

- (void)pressesBegan:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	BOOL handled = NO;
	for (UIPress* press in presses)
	{
		switch ([[press key] keyCode])
		{
			case UIKeyboardHIDUsageKeyboardEscape:
				[self finish];
				handled = YES;
				break;
			case UIKeyboardHIDUsageKeyboardDownArrow:
				[self moveFocus:1];
				handled = YES;
				break;
			case UIKeyboardHIDUsageKeyboardUpArrow:
				[self moveFocus:-1];
				handled = YES;
				break;
			case UIKeyboardHIDUsageKeyboardLeftArrow:
				if (_levels.count > 1)
					[self activateRow:[self rowAt:[NSIndexPath indexPathForRow:0 inSection:0]]];
				handled = YES;
				break;
			case UIKeyboardHIDUsageKeyboardRightArrow:
			case UIKeyboardHIDUsageKeyboardReturnOrEnter:
			case UIKeyboardHIDUsageKeypadEnter:
				if (_focused)
				{
					IupCocoaTouchMenuRow* row = [self rowAt:_focused];
					BOOL is_submenu = row.item && !row.isBack && iupStrEqual(row.item->iclass->name, "submenu");
					if (is_submenu || [[press key] keyCode] != UIKeyboardHIDUsageKeyboardRightArrow)
						[self activateRow:row];
				}
				handled = YES;
				break;
			default:
				break;
		}
	}
	if (!handled)
		[super pressesBegan:presses withEvent:event];
}

- (UIModalPresentationStyle)adaptivePresentationStyleForPresentationController:(UIPresentationController*)controller traitCollection:(UITraitCollection*)traitCollection
{
	(void)controller; (void)traitCollection;
	return UIModalPresentationNone;
}

- (void)presentationControllerDidDismiss:(UIPresentationController*)presentationController
{
	(void)presentationController;
	self.done = YES;
}

@end


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

	UIView* view = [host view];
	CGPoint anchor = CGPointMake(x, y);
	UIWindow* window = [view window];
	if (window)
		anchor = [view convertPoint:[window convertPoint:anchor fromWindow:nil] fromView:nil];

	IupCocoaTouchMenuPopupVC* vc = [[IupCocoaTouchMenuPopupVC alloc] initWithMenu:ih];
	[vc loadViewIfNeeded];
	[vc placeAt:anchor inView:view];

	UIPopoverPresentationController* ppc = [vc popoverPresentationController];
	ppc.delegate = vc;

	cocoaTouchMenuFireOpen(ih);
	[host presentViewController:vc animated:YES completion:nil];

	while (!vc.done)
	{
		@autoreleasepool {
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);
		}
	}

	NSArray<NSValue*>* open_menus = [[[vc openMenus] copy] autorelease];
	for (NSInteger i = (NSInteger)open_menus.count - 1; i >= 0; i--)
		cocoaTouchMenuFireClose((Ihandle*)[open_menus[(NSUInteger)i] pointerValue]);

	Ihandle* picked = vc.picked;
	int picked_recent = vc.pickedRecent;
	Ihandle* recent_menu = vc.pickedRecentMenu;
	ppc.delegate = nil;
	[vc release];

	if (picked_recent >= 0 && iupObjectCheck(recent_menu))
	{
		char attr[32];
		snprintf(attr, sizeof(attr), "_IUP_RECENT_FILE%d", picked_recent);
		iupAttribSetStr(recent_menu, "TITLE", iupAttribGet(recent_menu, attr));
		Icallback cb = (Icallback)iupAttribGet(recent_menu, "_IUP_RECENT_CB");
		if (cb && cb(recent_menu) == IUP_CLOSE) IupExitLoop();
	}
	else if (picked)
		cocoaTouchMenuFireActionDirect(picked);

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

static int cocoaTouchMenuItemSetImageAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "IMAGE", value);
	cocoaTouchMenuInvalidateAncestor(ih);
	return 1;
}

static int cocoaTouchMenuItemSetImpressAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "IMPRESS", value);
	cocoaTouchMenuInvalidateAncestor(ih);
	return 1;
}

static int cocoaTouchMenuItemSetTitleImageAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "TITLEIMAGE", value);
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
	iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaTouchMenuItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMPRESS", NULL, cocoaTouchMenuItemSetImpressAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TITLEIMAGE", NULL, cocoaTouchMenuItemSetTitleImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "KEY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

IUP_SDK_API void iupdrvSubmenuInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchMenuItemMapMethod;
	ic->UnMap = iupdrvBaseUnMapMethod;

	iupClassRegisterAttribute(ic, "TITLE", NULL, cocoaTouchMenuItemSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGE", NULL, cocoaTouchMenuItemSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, cocoaTouchMenuItemSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "KEY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}

IUP_SDK_API void iupdrvMenuSeparatorInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchMenuItemMapMethod;
	ic->UnMap = iupdrvBaseUnMapMethod;
}
