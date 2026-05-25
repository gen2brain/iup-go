/** \file
 * \brief List Control (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_mask.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_list.h"

#include "iupcocoatouch_drv.h"


static const void* IUP_COCOATOUCH_LIST_CTRL_OBJ_KEY = "IUP_COCOATOUCH_LIST_CTRL_OBJ_KEY";
static const void* IUP_COCOATOUCH_LIST_DND_KEY      = "IUP_COCOATOUCH_LIST_DND_KEY";

@class IupCocoaTouchListEditView;

@interface IupCocoaTouchListController : NSObject <UITableViewDataSource, UITableViewDelegate, UITextFieldDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, retain) NSMutableArray<NSString*>* items;
/* parallel to items, NSNull when no image */
@property(nonatomic, retain) NSMutableArray* itemImages;
@property(nonatomic, assign) NSInteger selectedIndex;  /* 0-based, -1 = none, dropdown only */
@property(nonatomic, retain) UIColor* cellTextColor;
@property(nonatomic, retain) UIFont* cellFont;
/* set only when has_editbox */
@property(nonatomic, assign) UITextField* field;
/* suppress synthetic EDIT_CB when filling field from row tap or VALUE setter */
@property(nonatomic, assign) BOOL suppressEditCb;
@end

/* EDITBOX standalone: vertical [UITextField, UITableView] container */
@interface IupCocoaTouchListEditView : UIView
@property(nonatomic, retain) UITextField* field;
@property(nonatomic, retain) UITableView* table;
@end

@interface IupCocoaTouchListDnDDelegate : NSObject <UITableViewDragDelegate, UITableViewDropDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, copy)   NSArray<NSString*>* dragTypes;
@property(nonatomic, copy)   NSArray<NSString*>* dropTypes;
@property(nonatomic, assign) BOOL reorder;
@end

#define IUP_COCOATOUCH_LIST_REORDER_MARKER @"_IUPLIST_REORDER"

@interface IupCocoaTouchListTableView : UITableView
@property(nonatomic, assign) Ihandle* ihandle;
@end

@implementation IupCocoaTouchListTableView
- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesBegan:touches withEvent:event];
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	IFniiiis cb = (IFniiiis)IupGetCallback(_ihandle, "BUTTON_CB");
	if (cb)
	{
		CGPoint p = [[touches anyObject] locationInView:self];
		char status[IUPKEY_STATUS_SIZE];
		iupCocoaTouchButtonKeySetStatus(event, event ? [event modifierFlags] : 0, 1, 0, status);
		if (cb(_ihandle, IUP_BUTTON1, 1, (int)p.x, (int)p.y, status) == IUP_CLOSE) IupExitLoop();
	}
}
- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesMoved:touches withEvent:event];
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	IFniis cb = (IFniis)IupGetCallback(_ihandle, "MOTION_CB");
	if (cb)
	{
		CGPoint p = [[touches anyObject] locationInView:self];
		char status[IUPKEY_STATUS_SIZE];
		iupCocoaTouchButtonKeySetStatus(event, event ? [event modifierFlags] : 0, 1, 0, status);
		if (cb(_ihandle, (int)p.x, (int)p.y, status) == IUP_CLOSE) IupExitLoop();
	}
}
- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[super touchesEnded:touches withEvent:event];
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	IFniiiis cb = (IFniiiis)IupGetCallback(_ihandle, "BUTTON_CB");
	if (cb)
	{
		CGPoint p = [[touches anyObject] locationInView:self];
		char status[IUPKEY_STATUS_SIZE];
		iupCocoaTouchButtonKeySetStatus(event, event ? [event modifierFlags] : 0, 0, 0, status);
		if (cb(_ihandle, IUP_BUTTON1, 0, (int)p.x, (int)p.y, status) == IUP_CLOSE) IupExitLoop();
	}
}
@end

/* scale src so its longest side = target_pt, preserving aspect (FITIMAGE=YES) */
static UIImage* cocoaTouchListFitImage(UIImage* src, CGFloat target_pt)
{
	if (!src || target_pt <= 0) return src;
	CGSize sz = src.size;
	if (sz.width <= 0 || sz.height <= 0) return src;
	CGFloat max_side = MAX(sz.width, sz.height);
	if (max_side == target_pt) return src;
	CGFloat scale = target_pt / max_side;
	CGSize new_sz = CGSizeMake(round(sz.width * scale), round(sz.height * scale));
	UIGraphicsImageRendererFormat* fmt = [UIGraphicsImageRendererFormat preferredFormat];
	UIGraphicsImageRenderer* r = [[[UIGraphicsImageRenderer alloc] initWithSize:new_sz format:fmt] autorelease];
	return [r imageWithActions:^(UIGraphicsImageRendererContext* _Nonnull ctx) {
		(void)ctx;
		[src drawInRect:CGRectMake(0, 0, new_sz.width, new_sz.height)];
	}];
}

/* sets dropdown display to "[image] text" or just text; cfg.image keeps the trailing chevron */
static void cocoaTouchListSetDropdownDisplay(UIButton* button, NSString* text, UIImage* item_img, UIFont* font)
{
	if (!button) return;
	if (!text) text = @"";
	if (!item_img)
	{
		[button setAttributedTitle:nil forState:UIControlStateNormal];
		[button setTitle:text forState:UIControlStateNormal];
		return;
	}
	NSMutableAttributedString* attr = [[[NSMutableAttributedString alloc] init] autorelease];
	NSTextAttachment* att = [[[NSTextAttachment alloc] init] autorelease];
	att.image = [item_img imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
	CGFloat ih = font.lineHeight;
	CGFloat iw = att.image.size.height > 0 ? att.image.size.width * ih / att.image.size.height : ih;
	att.bounds = CGRectMake(0, font.descender, iw, ih);
	[attr appendAttributedString:[NSAttributedString attributedStringWithAttachment:att]];
	[attr appendAttributedString:[[[NSAttributedString alloc] initWithString:@"  "] autorelease]];
	[attr appendAttributedString:[[[NSAttributedString alloc] initWithString:text
		attributes:@{ NSFontAttributeName: font }] autorelease]];
	[button setTitle:nil forState:UIControlStateNormal];
	[button setAttributedTitle:attr forState:UIControlStateNormal];
}

@implementation IupCocoaTouchListController

- (instancetype)init
{
	self = [super init];
	if (self)
	{
		_items = [[NSMutableArray alloc] init];
		_itemImages = [[NSMutableArray alloc] init];
		_selectedIndex = -1;
	}
	return self;
}

- (void)dealloc
{
	[_items release];
	[_itemImages release];
	[_cellTextColor release];
	[_cellFont release];
	[super dealloc];
}

- (NSInteger)tableView:(UITableView*)tableView numberOfRowsInSection:(NSInteger)section
{
	(void)tableView; (void)section;
	if (_ihandle && _ihandle->data && _ihandle->data->is_virtual)
		return _ihandle->data->item_count;
	return (NSInteger)[_items count];
}

- (UITableViewCell*)tableView:(UITableView*)tableView cellForRowAtIndexPath:(NSIndexPath*)indexPath
{
	static NSString* const cell_id = @"IupCocoaTouchListCell";
	UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:cell_id];
	if (!cell)
	{
		cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:cell_id] autorelease];
	}
	NSUInteger row = (NSUInteger)[indexPath row];

	NSString* text = nil;
	UIImage* img = nil;
	BOOL virtual_mode = (_ihandle && _ihandle->data && _ihandle->data->is_virtual);
	if (virtual_mode)
	{
		char* cstr = iupListGetItemValueCb(_ihandle, (int)row + 1);
		text = cstr ? [NSString stringWithUTF8String:cstr] : @"";
		if (_ihandle->data->show_image)
		{
			char* image_name = iupListGetItemImageCb(_ihandle, (int)row + 1);
			if (image_name && *image_name)
			{
				void* raw = iupImageGetImage(image_name, _ihandle, 0, NULL);
				if (raw && [(id)raw isKindOfClass:[UIImage class]]) img = (UIImage*)raw;
			}
		}
	}
	else
	{
		text = (row < [_items count]) ? [_items objectAtIndex:row] : @"";
		if (row < [_itemImages count])
		{
			id entry = [_itemImages objectAtIndex:row];
			if ([entry isKindOfClass:[UIImage class]]) img = (UIImage*)entry;
		}
	}

	cell.textLabel.text = text;
	if (_cellTextColor) cell.textLabel.textColor = _cellTextColor;
	if (_cellFont)      cell.textLabel.font      = _cellFont;

	if (img && _ihandle && _ihandle->data && _ihandle->data->fit_image)
	{
		UIFont* f = _cellFont ?: cell.textLabel.font;
		img = cocoaTouchListFitImage(img, f.lineHeight);
	}
	cell.imageView.image = img;
	return cell;
}

/* posv[] is 0-based and lists the full current selection */
- (void)dispatchMultiSelection:(UITableView*)tableView
{
	IFnsii cb = (IFnsii)IupGetCallback(_ihandle, "ACTION");
	IFns mcb = (IFns)IupGetCallback(_ihandle, "MULTISELECT_CB");
	if (!cb && !mcb) return;
	NSArray<NSIndexPath*>* sel = [tableView indexPathsForSelectedRows];
	int n = (int)[sel count];
	if (n == 0)
	{
		iupListMultipleCallActionCb(_ihandle, cb, mcb, NULL, 0);
		return;
	}
	int* posv = (int*)malloc(sizeof(int) * (NSUInteger)n);
	if (!posv) return;
	for (int i = 0; i < n; i++) posv[i] = (int)[[sel objectAtIndex:(NSUInteger)i] row];
	iupListMultipleCallActionCb(_ihandle, cb, mcb, posv, n);
	free(posv);
}

- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath
{
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	int pos_1based = (int)[indexPath row] + 1;

	/* EDITBOX standalone: row tap mirrors text into the entry without firing EDIT_CB */
	if (_field && _ihandle->data && _ihandle->data->has_editbox)
	{
		NSString* text = (NSUInteger)[indexPath row] < [_items count]
			? [_items objectAtIndex:(NSUInteger)[indexPath row]] : @"";
		_suppressEditCb = YES;
		_field.text = text;
		_suppressEditCb = NO;
	}

	if (_ihandle->data->is_multiple)
	{
		[self dispatchMultiSelection:tableView];
	}
	else
	{
		IFnsii cb = (IFnsii)IupGetCallback(_ihandle, "ACTION");
		if (cb) iupListSingleCallActionCb(_ihandle, cb, pos_1based);
	}

	IFn changed_cb = (IFn)IupGetCallback(_ihandle, "VALUECHANGED_CB");
	if (changed_cb && changed_cb(_ihandle) == IUP_CLOSE)
	{
		IupExitLoop();
	}
}

- (void)tableView:(UITableView*)tableView didDeselectRowAtIndexPath:(NSIndexPath*)indexPath
{
	(void)indexPath;
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	if (!_ihandle->data->is_multiple) return;
	[self dispatchMultiSelection:tableView];

	IFn changed_cb = (IFn)IupGetCallback(_ihandle, "VALUECHANGED_CB");
	if (changed_cb && changed_cb(_ihandle) == IUP_CLOSE)
	{
		IupExitLoop();
	}
}

- (void)textFieldDidChange:(UITextField*)field
{
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	if (_suppressEditCb) return;
	NSString* full = field.text ?: @"";
	IFnis edit_cb = (IFnis)IupGetCallback(_ihandle, "EDIT_CB");
	if (edit_cb) edit_cb(_ihandle, 0, (char*)[full UTF8String]);
	IFn changed_cb = (IFn)IupGetCallback(_ihandle, "VALUECHANGED_CB");
	if (changed_cb && changed_cb(_ihandle) == IUP_CLOSE) IupExitLoop();
}

- (void)textFieldDidChangeSelection:(UITextField*)field
{
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	IFniii cb = (IFniii)IupGetCallback(_ihandle, "CARET_CB");
	if (!cb) return;
	UITextRange* sel = field.selectedTextRange;
	if (!sel) return;
	int pos = (int)[field offsetFromPosition:field.beginningOfDocument toPosition:sel.start];
	cb(_ihandle, 1, pos + 1, pos);
}

- (void)handleDoubleTap:(UITapGestureRecognizer*)gr
{
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	UITableView* table = (UITableView*)gr.view;
	NSIndexPath* ip = [table indexPathForRowAtPoint:[gr locationInView:table]];
	if (!ip) return;
	IFnis cb = (IFnis)IupGetCallback(_ihandle, "DBLCLICK_CB");
	if (cb) iupListSingleCallDblClickCb(_ihandle, cb, (int)[ip row] + 1);
}

- (BOOL)textFieldShouldReturn:(UITextField*)textField
{
	[textField resignFirstResponder];
	return YES;
}

@end


@implementation IupCocoaTouchListEditView

- (void)dealloc
{
	[_field release];
	[_table release];
	[super dealloc];
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	CGFloat w = self.bounds.size.width;
	CGFloat h = self.bounds.size.height;
	CGFloat fh = 36;  /* matches the 40 pt slot in iupdrvListAddBorders */
	if (_field) _field.frame = CGRectMake(0, 0, w, fh);
	CGFloat gap = _field ? 4 : 0;
	if (_table) _table.frame = CGRectMake(0, fh + gap, w, MAX(0, h - fh - gap));
}

@end


static IupCocoaTouchListController* cocoaTouchListGetController(Ihandle* ih)
{
	if (!ih || !ih->handle) return nil;
	return objc_getAssociatedObject((id)ih->handle, IUP_COCOATOUCH_LIST_CTRL_OBJ_KEY);
}

static UITableView* cocoaTouchListGetTable(Ihandle* ih)
{
	if (!ih || !ih->handle) return nil;
	id h = (id)ih->handle;
	if ([h isKindOfClass:[UITableView class]]) return (UITableView*)h;
	if ([h isKindOfClass:[IupCocoaTouchListEditView class]]) return ((IupCocoaTouchListEditView*)h).table;
	return nil;
}

static UIButton* cocoaTouchListGetDropdown(Ihandle* ih)
{
	if (!ih || !ih->handle) return nil;
	id h = (id)ih->handle;
	if ([h isKindOfClass:[UIButton class]]) return (UIButton*)h;
	if ([h isKindOfClass:[UITextField class]])
	{
		UIView* rv = ((UITextField*)h).rightView;
		if ([rv isKindOfClass:[UIButton class]]) return (UIButton*)rv;
	}
	return nil;
}

static UITextField* cocoaTouchListGetField(Ihandle* ih)
{
	if (!ih || !ih->handle) return nil;
	id h = (id)ih->handle;
	if ([h isKindOfClass:[UITextField class]]) return (UITextField*)h;
	if ([h isKindOfClass:[IupCocoaTouchListEditView class]]) return ((IupCocoaTouchListEditView*)h).field;
	return nil;
}

static void cocoaTouchListReloadTable(Ihandle* ih)
{
	UITableView* table = cocoaTouchListGetTable(ih);
	if (table) [table reloadData];
}

static int cocoaTouchListConvertXYToPos(Ihandle* ih, int x, int y)
{
	UITableView* table = cocoaTouchListGetTable(ih);
	if (!table) return -1;
	NSIndexPath* ip = [table indexPathForRowAtPoint:CGPointMake((CGFloat)x, (CGFloat)y)];
	if (!ip) return -1;
	return (int)ip.row + 1;
}

static void cocoaTouchListReorder(Ihandle* ih, int drag_id, int drop_id)
{
	int is_ctrl = 0;
	if (iupListCallDragDropCb(ih, drag_id, drop_id, &is_ctrl) != IUP_CONTINUE) return;

	int count = iupdrvListGetCount(ih);
	char* text = iupStrDup(IupGetAttributeId(ih, "", drag_id + 1));
	int insert_at = (drop_id < 0 || drop_id >= count) ? count : drop_id;
	iupdrvListInsertItem(ih, insert_at, text ? text : "");

	int adj_src = drag_id > insert_at ? drag_id + 1 : drag_id;
	int new_pos = insert_at;
	if (!is_ctrl)
	{
		iupdrvListRemoveItem(ih, adj_src);
		if (adj_src < insert_at) new_pos = insert_at - 1;
	}
	iupAttribSetInt(ih, "_IUPLIST_OLDVALUE", new_pos + 1);
	IupSetInt(ih, "VALUE", new_pos + 1);
	if (text) free(text);
}

@implementation IupCocoaTouchListDnDDelegate

- (NSArray<UIDragItem*>*)tableView:(UITableView*)tableView itemsForBeginningDragSession:(id<UIDragSession>)session atIndexPath:(NSIndexPath*)indexPath
{
	(void)session;
	if (!_ihandle || !iupObjectCheck(_ihandle)) return @[];

	if (_reorder)
	{
		UIDragItem* item = [[[UIDragItem alloc] initWithItemProvider:[[[NSItemProvider alloc] init] autorelease]] autorelease];
		item.localObject = @{ IUP_COCOATOUCH_LIST_REORDER_MARKER : @([indexPath row]) };
		return @[item];
	}

	if ([_dragTypes count] == 0) return @[];

	CGRect rect = [tableView rectForRowAtIndexPath:indexPath];
	int x = (int)CGRectGetMidX(rect);
	int y = (int)CGRectGetMidY(rect);

	IFnii begin_cb = (IFnii)IupGetCallback(_ihandle, "DRAGBEGIN_CB");
	if (begin_cb && begin_cb(_ihandle, x, y) == IUP_IGNORE) return @[];

	NSItemProvider* provider = [[[NSItemProvider alloc] init] autorelease];
	Ihandle* ih = _ihandle;

	for (NSString* uti in _dragTypes)
	{
		NSString* uti_copy = [[uti copy] autorelease];
		[provider registerDataRepresentationForTypeIdentifier:uti_copy
			visibility:NSItemProviderRepresentationVisibilityAll
			loadHandler:^NSProgress*(void (^completion)(NSData*, NSError*)) {
				if (!iupObjectCheck(ih)) { completion(nil, nil); return nil; }
				IFns size_cb = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
				IFnsVi data_cb = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");
				if (!size_cb || !data_cb) { completion(nil, nil); return nil; }
				char type_cstr[128];
				strlcpy(type_cstr, [uti_copy UTF8String], sizeof(type_cstr));
				int size = size_cb(ih, type_cstr);
				if (size <= 0) { completion(nil, nil); return nil; }
				NSMutableData* buf = [NSMutableData dataWithLength:(NSUInteger)size];
				data_cb(ih, type_cstr, [buf mutableBytes], size);
				completion(buf, nil);
				return nil;
			}
		];
	}

	UIDragItem* drag_item = [[[UIDragItem alloc] initWithItemProvider:provider] autorelease];
	if (iupAttribGetBoolean(_ihandle, "DRAGSOURCEMOVE")) drag_item.localObject = @"MOVE";
	return @[drag_item];
}

- (void)tableView:(UITableView*)tableView dragSessionDidEnd:(id<UIDragSession>)session
{
	(void)tableView; (void)session;
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	Ihandle* ih = _ihandle;
	/* Defer so DROPDATA_CB consumes _IUP_LIST_SOURCEPOS first. */
	dispatch_async(dispatch_get_main_queue(), ^{
		if (!iupObjectCheck(ih)) return;
		IFni end_cb = (IFni)IupGetCallback(ih, "DRAGEND_CB");
		if (!end_cb) return;
		int action = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE") ? 1 : 0;
		if (end_cb(ih, action) == IUP_CLOSE) IupExitLoop();
	});
}

- (NSString*)firstMatchingUTI:(id<UIDropSession>)session
{
	for (NSString* uti in _dropTypes)
	{
		if ([session hasItemsConformingToTypeIdentifiers:@[uti]]) return uti;
	}
	return nil;
}

- (BOOL)tableView:(UITableView*)tableView canHandleDropSession:(id<UIDropSession>)session
{
	(void)tableView;
	if (!_ihandle || !iupObjectCheck(_ihandle)) return NO;
	if (_reorder && session.localDragSession) return YES;
	if ([_dropTypes count] == 0) return NO;
	return [self firstMatchingUTI:session] != nil;
}

- (UITableViewDropProposal*)tableView:(UITableView*)tableView dropSessionDidUpdate:(id<UIDropSession>)session withDestinationIndexPath:(NSIndexPath*)destinationIndexPath
{
	(void)tableView; (void)destinationIndexPath;
	if (_reorder && session.localDragSession)
		return [[[UITableViewDropProposal alloc] initWithDropOperation:UIDropOperationMove intent:UITableViewDropIntentInsertAtDestinationIndexPath] autorelease];

	UIDropOperation op;
	if ([self firstMatchingUTI:session] == nil)
	{
		op = UIDropOperationForbidden;
	}
	else
	{
		BOOL want_move = NO;
		for (UIDragItem* item in session.localDragSession.items)
		{
			if ([item.localObject isEqual:@"MOVE"]) { want_move = YES; break; }
		}
		op = want_move ? UIDropOperationMove : UIDropOperationCopy;
	}

	if (_ihandle && iupObjectCheck(_ihandle))
	{
		IFniis motion_cb = (IFniis)IupGetCallback(_ihandle, "DROPMOTION_CB");
		if (motion_cb)
		{
			CGPoint pt = [session locationInView:tableView];
			char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
			if (motion_cb(_ihandle, (int)pt.x, (int)pt.y, status) == IUP_CLOSE) IupExitLoop();
		}
	}

	return [[[UITableViewDropProposal alloc] initWithDropOperation:op intent:UITableViewDropIntentInsertAtDestinationIndexPath] autorelease];
}

- (void)tableView:(UITableView*)tableView performDropWithCoordinator:(id<UITableViewDropCoordinator>)coordinator
{
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;

	if (_reorder && coordinator.session.localDragSession)
	{
		UIDragItem* item = coordinator.items.firstObject.dragItem;
		NSDictionary* marker = [item.localObject isKindOfClass:[NSDictionary class]] ? (NSDictionary*)item.localObject : nil;
		NSNumber* src = marker ? marker[IUP_COCOATOUCH_LIST_REORDER_MARKER] : nil;
		if (!src) return;
		NSIndexPath* dst = coordinator.destinationIndexPath;
		int drag_id = [src intValue];
		int drop_id = dst ? (int)[dst row] : -1;
		cocoaTouchListReorder(_ihandle, drag_id, drop_id);
		return;
	}

	IFnsViii drop_cb = (IFnsViii)IupGetCallback(_ihandle, "DROPDATA_CB");
	if (!drop_cb) return;

	id<UIDropSession> session = coordinator.session;
	NSString* match = [self firstMatchingUTI:session];
	if (!match) return;

	NSIndexPath* dst = coordinator.destinationIndexPath;
	int drop_x, drop_y;
	if (dst)
	{
		CGRect rect = [tableView rectForRowAtIndexPath:dst];
		drop_x = (int)CGRectGetMidX(rect);
		drop_y = (int)CGRectGetMidY(rect);
	}
	else
	{
		CGPoint pt = [session locationInView:tableView];
		drop_x = (int)pt.x;
		drop_y = (int)pt.y;
	}

	Ihandle* ih = _ihandle;
	NSString* type_copy = [[match copy] autorelease];

	for (id<UITableViewDropItem> drop_item in coordinator.items)
	{
		NSItemProvider* provider = drop_item.dragItem.itemProvider;
		if (![provider hasItemConformingToTypeIdentifier:type_copy]) continue;

		[provider loadDataRepresentationForTypeIdentifier:type_copy
			completionHandler:^(NSData* data, NSError* error) {
				if (error || !data) return;
				dispatch_async(dispatch_get_main_queue(), ^{
					if (!iupObjectCheck(ih)) return;
					char type_cstr[128];
					strlcpy(type_cstr, [type_copy UTF8String], sizeof(type_cstr));
					drop_cb(ih, type_cstr, (void*)[data bytes], (int)[data length], drop_x, drop_y);
				});
			}];
	}
}

- (void)dealloc
{
	[_dragTypes release];
	[_dropTypes release];
	[super dealloc];
}

@end

static void cocoaTouchListUpdateDragDrop(Ihandle* ih)
{
	UITableView* table = cocoaTouchListGetTable(ih);
	if (!table) return;

	BOOL want_reorder = ih->data->show_dragdrop ? YES : NO;
	BOOL want_drag = iupAttribGetBoolean(ih, "DRAGSOURCE") || want_reorder;
	BOOL want_drop = iupAttribGetBoolean(ih, "DROPTARGET") || want_reorder;

	IupCocoaTouchListDnDDelegate* delegate = objc_getAssociatedObject(table, IUP_COCOATOUCH_LIST_DND_KEY);

	if (!want_drag && !want_drop)
	{
		if (delegate)
		{
			table.dragInteractionEnabled = NO;
			table.dragDelegate = nil;
			table.dropDelegate = nil;
			objc_setAssociatedObject(table, IUP_COCOATOUCH_LIST_DND_KEY, nil, OBJC_ASSOCIATION_RETAIN);
		}
		return;
	}

	if (!delegate)
	{
		delegate = [[IupCocoaTouchListDnDDelegate alloc] init];
		delegate.ihandle = ih;
		objc_setAssociatedObject(table, IUP_COCOATOUCH_LIST_DND_KEY, delegate, OBJC_ASSOCIATION_RETAIN);
		[delegate release];
	}

	delegate.dragTypes = iupCocoaTouchDragParseTypes(iupAttribGet(ih, "DRAGTYPES"));
	delegate.dropTypes = iupCocoaTouchDragParseTypes(iupAttribGet(ih, "DROPTYPES"));
	delegate.reorder = want_reorder;

	if (want_drag)
	{
		table.dragInteractionEnabled = YES;
		table.dragDelegate = delegate;
	}
	else
	{
		table.dragInteractionEnabled = NO;
		table.dragDelegate = nil;
	}

	table.dropDelegate = want_drop ? delegate : nil;
}

static int cocoaTouchListSetDragSourceAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "DRAGSOURCE", value);
	cocoaTouchListUpdateDragDrop(ih);
	return 1;
}

static int cocoaTouchListSetDropTargetAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "DROPTARGET", value);
	cocoaTouchListUpdateDragDrop(ih);
	return 1;
}

static int cocoaTouchListSetDragTypesAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "DRAGTYPES", value);
	cocoaTouchListUpdateDragDrop(ih);
	return 1;
}

static int cocoaTouchListSetDropTypesAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "DROPTYPES", value);
	cocoaTouchListUpdateDragDrop(ih);
	return 1;
}

static void cocoaTouchListRebuildDropdownMenu(Ihandle* ih)
{
	UIButton* button = cocoaTouchListGetDropdown(ih);
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!button || !ctrl) return;
	UITextField* field = cocoaTouchListGetField(ih);

	NSUInteger count = [ctrl.items count];
	NSString* title = @"";
	UIImage* selected_img = nil;
	if (ctrl.selectedIndex >= 0 && (NSUInteger)ctrl.selectedIndex < count)
	{
		title = [ctrl.items objectAtIndex:(NSUInteger)ctrl.selectedIndex];
		if ((NSUInteger)ctrl.selectedIndex < [ctrl.itemImages count])
		{
			id e = [ctrl.itemImages objectAtIndex:(NSUInteger)ctrl.selectedIndex];
			if ([e isKindOfClass:[UIImage class]]) selected_img = (UIImage*)e;
		}
	}
	UIFont* menu_font = ctrl.cellFont ?: [UIFont systemFontOfSize:[UIFont buttonFontSize]];
	UIImage* selected_fit = (selected_img && ih->data && ih->data->fit_image)
		? cocoaTouchListFitImage(selected_img, menu_font.lineHeight) : selected_img;
	if (field)
	{
		ctrl.suppressEditCb = YES;
		field.text = (ctrl.selectedIndex >= 0) ? title : @"";
		ctrl.suppressEditCb = NO;
	}
	else
	{
		cocoaTouchListSetDropdownDisplay(button, title, selected_fit, menu_font);
	}
	NSMutableArray<UIAction*>* actions = [NSMutableArray arrayWithCapacity:count];
	for (NSUInteger i = 0; i < count; i++)
	{
		NSString* item = [ctrl.items objectAtIndex:i];
		NSUInteger index = i;
		UIImage* item_img = nil;
		if (i < [ctrl.itemImages count])
		{
			id entry = [ctrl.itemImages objectAtIndex:i];
			if ([entry isKindOfClass:[UIImage class]])
			{
				item_img = (UIImage*)entry;
				if (ih->data && ih->data->fit_image)
					item_img = cocoaTouchListFitImage(item_img, menu_font.lineHeight);
				/* UIMenu/UIAction default to template rendering and would tint the bitmap flat. */
				item_img = [item_img imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
			}
		}
		UIImage* tap_img = item_img;
		__block UITextField* field_ref = field;
		UIAction* action = [UIAction actionWithTitle:item image:item_img identifier:nil
			handler:^(UIAction* a) {
				(void)a;
				if (!iupObjectCheck(ih)) return;
				/* UIMenu fires the handler on every tap, including the already-checked item; skip when nothing changed. */
				BOOL changed = (ctrl.selectedIndex != (NSInteger)index);
				ctrl.selectedIndex = (NSInteger)index;
				if (field_ref)
				{
					ctrl.suppressEditCb = YES;
					field_ref.text = item;
					ctrl.suppressEditCb = NO;
				}
				else
				{
					cocoaTouchListSetDropdownDisplay(button, item, tap_img, menu_font);
				}
				/* UIMenu actions are immutable; rebuild so the checkmark moves with selectedIndex on the next open. */
				cocoaTouchListRebuildDropdownMenu(ih);
				if (!changed) return;
				IFnsii action_cb = (IFnsii)IupGetCallback(ih, "ACTION");
				if (action_cb) iupListSingleCallActionCb(ih, action_cb, (int)index + 1);
				IFn changed_cb = (IFn)IupGetCallback(ih, "VALUECHANGED_CB");
				if (changed_cb && changed_cb(ih) == IUP_CLOSE) IupExitLoop();
			}];
		if ((NSInteger)i == ctrl.selectedIndex)
		{
			[action setState:UIMenuElementStateOn];
		}
		[actions addObject:action];
	}
	UIMenu* menu = [UIMenu menuWithTitle:@"" children:actions];
	[button setMenu:menu];
	[button setShowsMenuAsPrimaryAction:YES];
}

IUP_SDK_API int iupdrvListGetCount(Ihandle* ih)
{
	if (ih->data->is_virtual) return ih->data->item_count;
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	return ctrl ? (int)[ctrl.items count] : 0;
}

/* SORT: ascending insert position. */
static NSUInteger cocoaTouchListSortPos(NSArray* items, const char* value)
{
	NSUInteger n = [items count];
	for (NSUInteger i = 0; i < n; i++)
	{
		const char* t = [(NSString*)items[i] UTF8String];
		if (iupStrCompare(t ? t : "", value, 0, 1) > 0) return i;
	}
	return n;
}

IUP_SDK_API void iupdrvListAppendItem(Ihandle* ih, const char* value)
{
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl) return;
	NSString* text = value ? [NSString stringWithUTF8String:value] : @"";
	if (iupAttribGetBoolean(ih, "SORT"))
	{
		NSUInteger pos = cocoaTouchListSortPos(ctrl.items, value);
		[ctrl.items insertObject:(text ?: @"") atIndex:pos];
		[ctrl.itemImages insertObject:[NSNull null] atIndex:pos];
	}
	else
	{
		[ctrl.items addObject:text ?: @""];
		[ctrl.itemImages addObject:[NSNull null]];
	}
	if (ih->data->is_dropdown) cocoaTouchListRebuildDropdownMenu(ih);
	else                       cocoaTouchListReloadTable(ih);
}

IUP_SDK_API void iupdrvListInsertItem(Ihandle* ih, int pos, const char* value)
{
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl) return;
	NSUInteger insert_at = iupAttribGetBoolean(ih, "SORT") ? cocoaTouchListSortPos(ctrl.items, value) : (NSUInteger)pos;
	if (insert_at > [ctrl.items count]) insert_at = [ctrl.items count];
	NSString* text = value ? [NSString stringWithUTF8String:value] : @"";
	[ctrl.items insertObject:(text ?: @"") atIndex:insert_at];
	[ctrl.itemImages insertObject:[NSNull null] atIndex:insert_at];
	if (ih->data->is_dropdown) cocoaTouchListRebuildDropdownMenu(ih);
	else                       cocoaTouchListReloadTable(ih);
}

IUP_SDK_API void iupdrvListRemoveItem(Ihandle* ih, int pos)
{
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl) return;
	NSUInteger index = (NSUInteger)pos;
	if (index >= [ctrl.items count]) return;
	[ctrl.items removeObjectAtIndex:index];
	if (index < [ctrl.itemImages count]) [ctrl.itemImages removeObjectAtIndex:index];

	if (ih->data->is_dropdown)
	{
		if (ctrl.selectedIndex == (NSInteger)index) ctrl.selectedIndex = -1;
		else if (ctrl.selectedIndex > (NSInteger)index) ctrl.selectedIndex--;
		cocoaTouchListRebuildDropdownMenu(ih);
	}
	else
	{
		cocoaTouchListReloadTable(ih);
	}
}

IUP_SDK_API void iupdrvListRemoveAllItems(Ihandle* ih)
{
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl) return;
	[ctrl.items removeAllObjects];
	[ctrl.itemImages removeAllObjects];
	ctrl.selectedIndex = -1;
	if (ih->data->is_dropdown) cocoaTouchListRebuildDropdownMenu(ih);
	else                       cocoaTouchListReloadTable(ih);
}

static char* cocoaTouchListGetImageNativeHandleAttribId(Ihandle* ih, int id)
{
	if (!ih->data->show_image) return NULL;
	return (char*)iupdrvListGetImageHandle(ih, id);
}

/* pos is 1-based here, 0-based in the Insert/Remove/SetImageHandle siblings. */
IUP_SDK_API void* iupdrvListGetImageHandle(Ihandle* ih, int pos)
{
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl || pos < 1 || (NSUInteger)pos > [ctrl.itemImages count]) return NULL;
	id entry = [ctrl.itemImages objectAtIndex:(NSUInteger)(pos - 1)];
	return [entry isKindOfClass:[UIImage class]] ? (void*)entry : NULL;
}

IUP_SDK_API int iupdrvListSetImageHandle(Ihandle* ih, int pos, void* hImage)
{
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl || pos < 0) return 0;
	NSUInteger index = (NSUInteger)pos;
	while ([ctrl.itemImages count] <= index) [ctrl.itemImages addObject:[NSNull null]];
	id entry = hImage ? (id)hImage : (id)[NSNull null];
	[ctrl.itemImages replaceObjectAtIndex:index withObject:entry];
	if (ih->data->is_dropdown) cocoaTouchListRebuildDropdownMenu(ih);
	else                      cocoaTouchListReloadTable(ih);
	return 1;
}

IUP_SDK_API void iupdrvListSetItemCount(Ihandle* ih, int count)
{
	(void)count;
	if (!ih->data->is_virtual) return;
	UITableView* table = cocoaTouchListGetTable(ih);
	if (table) [table reloadData];
}

IUP_SDK_API void iupdrvListAddBorders(Ihandle* ih, int* x, int* y)
{
	if (ih->data->is_dropdown)
	{
		/* UIButton chevron 24 + content-insets 16+16 + safety. */
		if (x) *x += 64;
		if (y) *y += 12;
	}
	else
	{
		/* UITableView default cell content margin 16+16. */
		if (x) *x += 32;
		if (y) *y += 8;
	}

	if (x && ih->data->show_image && (ih->data->maximg_h > 0 || iupAttribGetBoolean(ih, "DROPTARGET")))
	{
		IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
		UIFont* f = ctrl.cellFont ?: [UIFont systemFontOfSize:[UIFont labelFontSize]];
		int target_h = (int)ceil(f.lineHeight);
		int rendered_w = (ih->data->maximg_h > 0)
			? (ih->data->fit_image ? ih->data->maximg_w * target_h / ih->data->maximg_h : ih->data->maximg_w)
			: target_h;
		int delta = rendered_w - ih->data->maximg_w;
		if (delta < 0) delta = 0;
		*x += delta + 8 + 8;
	}

	/* EDITBOX non-dropdown: VISIBLELINES counts the entry, swap one row for the 36+4 pt field. */
	if (ih->data->has_editbox && !ih->data->is_dropdown && y)
	{
		int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
		if (visiblelines > 0)
		{
			int char_w, char_h;
			iupdrvFontGetCharSize(ih, &char_w, &char_h);
			int item_h = char_h;
			iupdrvListAddItemSpace(ih, &item_h);
			*y -= item_h;
		}
		*y += 40;
	}
}

IUP_SDK_API void iupdrvListAddItemSpace(Ihandle* ih, int* h)
{
	(void)ih;
	/* Match the rowHeight set in the UITableView. */
	UIFont* base = [UIFont systemFontOfSize:[UIFont labelFontSize]];
	int row_h = (int)ceil(base.lineHeight) + 12;
	if (h && *h < row_h) *h = row_h;
}

static char* cocoaTouchListGetValueAttrib(Ihandle* ih)
{
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl) return NULL;

	if (ih->data->has_editbox)
	{
		UITextField* field = cocoaTouchListGetField(ih);
		NSString* s = field ? (field.text ?: @"") : @"";
		return iupStrReturnStr([s UTF8String]);
	}

	if (ih->data->is_dropdown)
	{
		return iupStrReturnInt((int)ctrl.selectedIndex + 1);  /* 1-based, 0 = none */
	}

	UITableView* table = cocoaTouchListGetTable(ih);
	if (!table) return NULL;

	if (ih->data->is_multiple)
	{
		NSUInteger count = ih->data->is_virtual ? (NSUInteger)ih->data->item_count : [ctrl.items count];
		char* buf = iupStrGetMemory((int)count + 1);
		NSArray<NSIndexPath*>* sel = [table indexPathsForSelectedRows];
		NSMutableIndexSet* selected = [NSMutableIndexSet indexSet];
		for (NSIndexPath* ip in sel) [selected addIndex:(NSUInteger)[ip row]];
		for (NSUInteger i = 0; i < count; i++)
		{
			buf[i] = [selected containsIndex:i] ? '+' : '-';
		}
		buf[count] = '\0';
		return buf;
	}

	NSIndexPath* selected = [table indexPathForSelectedRow];
	return iupStrReturnInt(selected ? (int)[selected row] + 1 : 0);
}

static int cocoaTouchListSetValueAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl) return 0;
	NSUInteger count = ih->data->is_virtual ? (NSUInteger)ih->data->item_count : [ctrl.items count];

	if (ih->data->has_editbox)
	{
		UITextField* field = cocoaTouchListGetField(ih);
		if (field)
		{
			ctrl.suppressEditCb = YES;
			field.text = value ? [NSString stringWithUTF8String:value] : @"";
			ctrl.suppressEditCb = NO;
		}
		return 0;
	}

	if (ih->data->is_dropdown)
	{
		int pos = 0;
		if (value) iupStrToInt(value, &pos);
		if (pos < 0 || pos > (int)count) pos = 0;
		ctrl.selectedIndex = pos > 0 ? (NSInteger)(pos - 1) : -1;
		cocoaTouchListRebuildDropdownMenu(ih);
		return 0;
	}

	UITableView* table = cocoaTouchListGetTable(ih);
	if (!table) return 0;

	if (ih->data->is_multiple)
	{
		const char* mask = value ? value : "";
		NSMutableArray<NSIndexPath*>* current = [[[table indexPathsForSelectedRows] mutableCopy] autorelease];
		for (NSIndexPath* ip in current)
		{
			[table deselectRowAtIndexPath:ip animated:NO];
		}
		size_t mlen = strlen(mask);
		for (NSUInteger i = 0; i < count; i++)
		{
			if (i < mlen && mask[i] == '+')
			{
				[table selectRowAtIndexPath:[NSIndexPath indexPathForRow:(NSInteger)i inSection:0]
				                   animated:NO
				             scrollPosition:UITableViewScrollPositionNone];
			}
		}
		return 0;
	}

	int pos = 0;
	if (value) iupStrToInt(value, &pos);
	NSIndexPath* current = [table indexPathForSelectedRow];
	if (current) [table deselectRowAtIndexPath:current animated:NO];
	if (pos >= 1 && (NSUInteger)pos <= count)
	{
		[table selectRowAtIndexPath:[NSIndexPath indexPathForRow:pos - 1 inSection:0]
		                   animated:NO
		             scrollPosition:UITableViewScrollPositionNone];
	}
	return 0;
}

static int cocoaTouchListSetTopItemAttrib(Ihandle* ih, const char* value)
{
	UITableView* table = cocoaTouchListGetTable(ih);
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!table || !ctrl) return 0;
	int pos = 1;
	if (value) iupStrToInt(value, &pos);
	NSUInteger total = ih->data->is_virtual ? (NSUInteger)ih->data->item_count : [ctrl.items count];
	if (pos < 1 || (NSUInteger)pos > total) return 0;
	[table scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:pos - 1 inSection:0]
	                atScrollPosition:UITableViewScrollPositionTop animated:NO];
	return 0;
}

static int cocoaTouchListSetShowDropdownAttrib(Ihandle* ih, const char* value)
{
	UIButton* button = cocoaTouchListGetDropdown(ih);
	if (!button) return 0;
	/* No public API to open a UIMenu programmatically; requires a user tap. */
	(void)button; (void)value;
	return 0;
}

static char* cocoaTouchListGetIdValueAttrib(Ihandle* ih, int pos)
{
	if (ih->data->is_virtual)
	{
		if (pos < 1 || pos > ih->data->item_count) return NULL;
		return iupListGetItemValueCb(ih, pos);
	}
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl) return NULL;
	if (pos < 1 || (NSUInteger)pos > [ctrl.items count]) return NULL;
	NSString* text = [ctrl.items objectAtIndex:(NSUInteger)(pos - 1)];
	return iupStrReturnStr([text UTF8String]);
}

static int cocoaTouchListSetBgColorAttrib(Ihandle* ih, const char* value)
{
	UIColor* color = iupCocoaTouchToNativeColor(value);
	if (!color) return 0;
	UITableView* table = cocoaTouchListGetTable(ih);
	if (table) [table setBackgroundColor:color];
	UIButton* button = cocoaTouchListGetDropdown(ih);
	if (button) [button setBackgroundColor:color];
	UITextField* field = cocoaTouchListGetField(ih);
	if (field) [field setBackgroundColor:color];
	return 1;
}

static int cocoaTouchListSetFgColorAttrib(Ihandle* ih, const char* value)
{
	UIColor* color = iupCocoaTouchToNativeColor(value);
	if (!color) return 0;
	UIButton* button = cocoaTouchListGetDropdown(ih);
	if (button) [button setTitleColor:color forState:UIControlStateNormal];
	UITextField* field = cocoaTouchListGetField(ih);
	if (field) [field setTextColor:color];

	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (ctrl)
	{
		ctrl.cellTextColor = color;
		cocoaTouchListReloadTable(ih);
	}
	return 1;
}

static int cocoaTouchListSetFontAttrib(Ihandle* ih, const char* value)
{
	if (!iupdrvSetFontAttrib(ih, value)) return 0;
	IupCocoaTouchFont* iup_font = iupCocoaTouchGetFont(ih);
	if (!iup_font) return 1;
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	UIButton* button = cocoaTouchListGetDropdown(ih);
	if (button) [button.titleLabel setFont:iup_font.nativeFont];
	UITextField* field = cocoaTouchListGetField(ih);
	if (field) [field setFont:iup_font.nativeFont];
	if (ctrl)
	{
		ctrl.cellFont = iup_font.nativeFont;
		cocoaTouchListReloadTable(ih);
	}
	return 1;
}

static char* cocoaTouchListGetSelectedTextAttrib(Ihandle* ih)
{
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl) return NULL;

	if (ih->data->is_dropdown)
	{
		if (ctrl.selectedIndex < 0 || (NSUInteger)ctrl.selectedIndex >= ctrl.items.count) return NULL;
		return iupStrReturnStr([[ctrl.items objectAtIndex:(NSUInteger)ctrl.selectedIndex] UTF8String]);
	}

	UITableView* table = cocoaTouchListGetTable(ih);
	if (!table) return NULL;
	NSIndexPath* sel = [table indexPathForSelectedRow];
	if (!sel) return NULL;
	NSUInteger row = (NSUInteger)sel.row;
	if (ih->data->is_virtual) return iupListGetItemValueCb(ih, (int)row + 1);
	if (row >= ctrl.items.count) return NULL;
	return iupStrReturnStr([[ctrl.items objectAtIndex:row] UTF8String]);
}

static char* cocoaTouchListGetValueStringAttrib(Ihandle* ih)
{
	return cocoaTouchListGetSelectedTextAttrib(ih);
}

static int cocoaTouchListSetValueStringAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchListController* ctrl = cocoaTouchListGetController(ih);
	if (!ctrl || !value) return 0;
	NSString* needle = [NSString stringWithUTF8String:value];
	NSUInteger count = ctrl.items.count;
	for (NSUInteger i = 0; i < count; i++)
	{
		if ([[ctrl.items objectAtIndex:i] isEqualToString:needle])
		{
			char pos_str[16];
			snprintf(pos_str, sizeof(pos_str), "%d", (int)(i + 1));
			return cocoaTouchListSetValueAttrib(ih, pos_str);
		}
	}
	return 0;
}

static int cocoaTouchListSetImageIdAttrib(Ihandle* ih, int item_id, const char* value)
{
	int pos = iupListGetPosAttrib(ih, item_id);
	if (pos < 0) return 0;
	UIImage* img = nil;
	if (value && *value)
	{
		void* raw = iupImageGetImage(value, ih, 0, NULL);
		if (raw && [(id)raw isKindOfClass:[UIImage class]]) img = (UIImage*)raw;
	}
	iupdrvListSetImageHandle(ih, pos, (void*)img);
	return 1;
}

static int cocoaTouchListSetSpacingAttrib(Ihandle* ih, const char* value)
{
	iupStrToInt(value, &ih->data->spacing);
	return 1;
}

static int cocoaTouchListSetPaddingAttrib(Ihandle* ih, const char* value)
{
	iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
	return 1;
}

/* +1 retained EDITBOX entry; caller owns the ref. */
static UITextField* cocoaTouchListBuildEditField(Ihandle* ih, IupCocoaTouchListController* ctrl)
{
	(void)ih;
	UITextField* field = [[UITextField alloc] initWithFrame:CGRectZero];
	field.borderStyle = UITextBorderStyleRoundedRect;
	field.delegate = ctrl;
	field.clearButtonMode = UITextFieldViewModeWhileEditing;
	field.autocorrectionType = UITextAutocorrectionTypeNo;
	field.autocapitalizationType = UITextAutocapitalizationTypeNone;
	[field addTarget:ctrl action:@selector(textFieldDidChange:) forControlEvents:UIControlEventEditingChanged];
	return field;
}

static UIView* cocoaTouchListCreateTable(Ihandle* ih, IupCocoaTouchListController* ctrl)
{
	IupCocoaTouchListTableView* table = [[IupCocoaTouchListTableView alloc] initWithFrame:CGRectZero style:UITableViewStylePlain];
	table.ihandle = ih;
	[table setDataSource:ctrl];
	[table setDelegate:ctrl];
	[table setAllowsMultipleSelection:(ih->data->is_multiple ? YES : NO)];
	UIFont* base = [UIFont systemFontOfSize:[UIFont labelFontSize]];
	[table setRowHeight:ceil(base.lineHeight) + 12];

	UITapGestureRecognizer* dtap = [[UITapGestureRecognizer alloc] initWithTarget:ctrl action:@selector(handleDoubleTap:)];
	dtap.numberOfTapsRequired = 2;
	[table addGestureRecognizer:dtap];
	[dtap release];

	if (!ih->data->has_editbox) return table;

	/* Hand both subviews to the wrapper, drop our +1 refs. */
	IupCocoaTouchListEditView* wrapper = [[IupCocoaTouchListEditView alloc] initWithFrame:CGRectZero];
	UITextField* field = cocoaTouchListBuildEditField(ih, ctrl);
	wrapper.field = field;
	wrapper.table = table;
	[field release];
	[table release];
	[wrapper addSubview:wrapper.field];
	[wrapper addSubview:wrapper.table];
	ctrl.field = wrapper.field;
	return wrapper;
}

static UIView* cocoaTouchListCreateDropdown(Ihandle* ih, IupCocoaTouchListController* ctrl)
{
	UIImage* chevron = [UIImage systemImageNamed:@"chevron.up.chevron.down"];

	if (ih->data->has_editbox)
	{
		UITextField* field = cocoaTouchListBuildEditField(ih, ctrl);
		UIButton* trigger = [UIButton buttonWithType:UIButtonTypeSystem];
		[trigger setImage:chevron forState:UIControlStateNormal];
		trigger.frame = CGRectMake(0, 0, 36, 36);
		[trigger setShowsMenuAsPrimaryAction:YES];
		field.rightView = trigger;
		field.rightViewMode = UITextFieldViewModeAlways;
		ctrl.field = field;
		return field;
	}

	UIButton* button;
	{
		UIButtonConfiguration* cfg = [UIButtonConfiguration grayButtonConfiguration];
		[cfg setCornerStyle:UIButtonConfigurationCornerStyleMedium];
		NSOperatingSystemVersion ver = [[NSProcessInfo processInfo] operatingSystemVersion];
		if (ver.majorVersion >= 16)
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
			cfg.indicator = UIButtonConfigurationIndicatorPopup;
#pragma clang diagnostic pop
		}
		else if (chevron)
		{
			cfg.image = chevron;
			cfg.imagePlacement = NSDirectionalRectEdgeTrailing;
			cfg.imagePadding = 8;
		}
		button = [UIButton buttonWithConfiguration:cfg primaryAction:nil];
	}
	button.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeading;
	[button setTitle:@"" forState:UIControlStateNormal];
	return [button retain];
}

static int cocoaTouchListMapMethod(Ihandle* ih)
{
	IupCocoaTouchListController* ctrl = [[IupCocoaTouchListController alloc] init];
	ctrl.ihandle = ih;

	UIView* view;
	if (ih->data->is_dropdown)
	{
		view = cocoaTouchListCreateDropdown(ih, ctrl);
	}
	else
	{
		view = cocoaTouchListCreateTable(ih, ctrl);
	}

	ih->handle = view;
	objc_setAssociatedObject(view, IHANDLE_ASSOCIATED_OBJ_KEY,       (id)ih, OBJC_ASSOCIATION_ASSIGN);
	objc_setAssociatedObject(view, IUP_COCOATOUCH_LIST_CTRL_OBJ_KEY, ctrl,   OBJC_ASSOCIATION_RETAIN);
	[ctrl release];

	iupCocoaTouchAddToParent(ih);

	iupListSetInitialItems(ih);

	if (ih->data->is_virtual && ih->data->item_count > 0)
		iupdrvListSetItemCount(ih, ih->data->item_count);

	if (ih->data->is_dropdown)
	{
		cocoaTouchListRebuildDropdownMenu(ih);
	}
	else
	{
		IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)cocoaTouchListConvertXYToPos);
		cocoaTouchListUpdateDragDrop(ih);
	}

	return IUP_NOERROR;
}

static void cocoaTouchListUnMapMethod(Ihandle* ih)
{
	if (ih && ih->handle)
	{
		UITableView* table = cocoaTouchListGetTable(ih);
		if (table)
		{
			[table setDataSource:nil];
			[table setDelegate:nil];
			[table setDragDelegate:nil];
			[table setDropDelegate:nil];
			objc_setAssociatedObject(table, IUP_COCOATOUCH_LIST_DND_KEY, nil, OBJC_ASSOCIATION_RETAIN);
		}
		UITextField* field = cocoaTouchListGetField(ih);
		if (field)
		{
			[field setDelegate:nil];
		}
		objc_setAssociatedObject((id)ih->handle, IHANDLE_ASSOCIATED_OBJ_KEY,       nil, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject((id)ih->handle, IUP_COCOATOUCH_LIST_CTRL_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	}
	iupdrvBaseUnMapMethod(ih);
}

IUP_SDK_API void iupdrvListInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchListMapMethod;
	ic->UnMap = cocoaTouchListUnMapMethod;

	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTouchListSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTouchListSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FONT", NULL, cocoaTouchListSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

	iupClassRegisterAttributeId(ic, "IDVALUE", cocoaTouchListGetIdValueAttrib, iupListSetIdValueAttrib, IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "VALUE", cocoaTouchListGetValueAttrib, cocoaTouchListSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "VALUESTRING", cocoaTouchListGetValueStringAttrib, cocoaTouchListSetValueStringAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SELECTEDTEXT", cocoaTouchListGetSelectedTextAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TOPITEM", NULL, cocoaTouchListSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SHOWDROPDOWN", NULL, cocoaTouchListSetShowDropdownAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "SPACING", iupListGetSpacingAttrib, cocoaTouchListSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
	iupClassRegisterAttribute(ic, "PADDING", iupListGetPaddingAttrib, cocoaTouchListSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

	/* EDITBOX text attributes: VALUE + EDIT_CB are wired, the rest stay as hash storage. */
	iupClassRegisterAttribute(ic, "READONLY", NULL, NULL, NULL, NULL, IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "NC", iupListGetNCAttrib, NULL, NULL, NULL, IUPAF_NOT_MAPPED);
	iupClassRegisterAttribute(ic, "SELECTION", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SELECTIONPOS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CARET", NULL, NULL, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CARETPOS", NULL, NULL, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "INSERT", NULL, NULL, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "APPEND", NULL, NULL, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SCROLLTO", NULL, NULL, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, NULL, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "CUEBANNER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FILTER", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

	iupClassRegisterAttributeId(ic, "IMAGE", NULL, cocoaTouchListSetImageIdAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "IMAGENATIVEHANDLE", cocoaTouchListGetImageNativeHandleAttribId, NULL, IUPAF_NO_STRING|IUPAF_READONLY|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "DROPEXPAND", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	/* iOS scroll indicators are transient; there is no always-visible mode */
	iupClassRegisterAttribute(ic, "AUTOHIDE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SCROLLVISIBLE",NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "VISIBLEITEMS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);

	iupClassRegisterReplaceAttribFunc(ic, "DRAGSOURCE", NULL, cocoaTouchListSetDragSourceAttrib);
	iupClassRegisterReplaceAttribFunc(ic, "DRAGTYPES",  NULL, cocoaTouchListSetDragTypesAttrib);
	iupClassRegisterReplaceAttribFunc(ic, "DROPTARGET", NULL, cocoaTouchListSetDropTargetAttrib);
	iupClassRegisterReplaceAttribFunc(ic, "DROPTYPES",  NULL, cocoaTouchListSetDropTypesAttrib);
}
