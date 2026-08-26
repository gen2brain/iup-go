/** \file
 * \brief Tree Control (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_key.h"
#include "iup_tree.h"

#include "iupcocoatouch_drv.h"

#define ITREE_INDENTATION_DEFAULT 20

static int cocoaTouchTreeIndentation(Ihandle* ih)
{
	int indent;
	char* value = ih ? iupAttribGet(ih, "INDENTATION") : NULL;
	if (value && iupStrToInt(value, &indent) && indent >= 0)
		return indent;
	return ITREE_INDENTATION_DEFAULT;
}


@class IupCocoaTouchTreeNode;
@class IupCocoaTouchTreeView;

@interface IupCocoaTouchTreeNode : NSObject
@property(nonatomic, assign) int kind;
@property(nonatomic, retain) NSString* title;
@property(nonatomic, assign) int depth;
@property(nonatomic, assign) BOOL expanded;
@property(nonatomic, assign) IupCocoaTouchTreeNode* parent;
@property(nonatomic, retain) NSMutableArray<IupCocoaTouchTreeNode*>* children;
@property(nonatomic, retain) UIColor* textColor;
@property(nonatomic, retain) UIFont* font;
@property(nonatomic, retain) UIImage* image;
@property(nonatomic, retain) UIImage* imageExpanded;
@property(nonatomic, assign) BOOL marked;
@property(nonatomic, assign) int toggleValue;
@property(nonatomic, assign) BOOL toggleVisible;
@property(nonatomic, assign) void* userdata;
@end

@implementation IupCocoaTouchTreeNode

- (instancetype)init
{
	self = [super init];
	if (self)
	{
		_kind = ITREE_LEAF;
		_expanded = YES;
		_toggleVisible = YES;
		_children = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[_title release];
	[_children release];
	[_textColor release];
	[_font release];
	[_image release];
	[_imageExpanded release];
	[super dealloc];
}

@end


@interface IupCocoaTouchTreeView : UIView <UITableViewDataSource, UITableViewDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, retain) UITableView* tableView;
@property(nonatomic, retain) NSMutableArray<IupCocoaTouchTreeNode*>* rootNodes;
@property(nonatomic, retain) NSMutableArray<IupCocoaTouchTreeNode*>* flatNodes;
@property(nonatomic, retain) UIImage* defaultLeafImage;
@property(nonatomic, retain) UIImage* defaultCollapsedImage;
@property(nonatomic, retain) UIImage* defaultExpandedImage;
@property(nonatomic, retain) UIColor* bgColor;
@property(nonatomic, retain) UIColor* fgColor;
@property(nonatomic, retain) UIFont*  defaultFont;
@property(nonatomic, assign) int    focusNodeId;
@property(nonatomic, assign) int    markStartId;
@end


static IupCocoaTouchTreeView* cocoaTouchTreeGetView(Ihandle* ih)
{
	if (!ih || !ih->handle) return nil;
	id h = ih->handle;
	if (![h isKindOfClass:[IupCocoaTouchTreeView class]]) return nil;
	return (IupCocoaTouchTreeView*)h;
}

static IupCocoaTouchTreeNode* cocoaTouchTreeNodeFromId(Ihandle* ih, int id_)
{
	InodeHandle* h = iupTreeGetNode(ih, id_);
	if (!h) return nil;
	id node = (id)h;
	if (![node isKindOfClass:[IupCocoaTouchTreeNode class]]) return nil;
	return (IupCocoaTouchTreeNode*)node;
}

static void cocoaTouchTreeAppendVisible(NSMutableArray* out, IupCocoaTouchTreeNode* node)
{
	[out addObject:node];
	if (node.kind == ITREE_BRANCH && node.expanded)
	{
		for (IupCocoaTouchTreeNode* c in node.children)
			cocoaTouchTreeAppendVisible(out, c);
	}
}

static void cocoaTouchTreeRebuildFlat(IupCocoaTouchTreeView* view)
{
	NSMutableArray* flat = [NSMutableArray array];
	for (IupCocoaTouchTreeNode* r in view.rootNodes)
		cocoaTouchTreeAppendVisible(flat, r);
	view.flatNodes = flat;
}

static int cocoaTouchTreeVisibleRow(IupCocoaTouchTreeView* view, IupCocoaTouchTreeNode* node)
{
	NSUInteger idx = [view.flatNodes indexOfObjectIdenticalTo:node];
	return (idx == NSNotFound) ? -1 : (int)idx;
}

static int cocoaTouchTreeSubtreeCount(IupCocoaTouchTreeNode* node)
{
	int c = 0;
	for (IupCocoaTouchTreeNode* child in node.children)
	{
		c++;
		c += cocoaTouchTreeSubtreeCount(child);
	}
	return c;
}

static int cocoaTouchTreeIndexInParent(IupCocoaTouchTreeNode* node)
{
	if (!node.parent) return -1;
	NSUInteger idx = [node.parent.children indexOfObjectIdenticalTo:node];
	return (idx == NSNotFound) ? -1 : (int)idx;
}


@interface IupCocoaTouchTreeCell : UITableViewCell
@property(nonatomic, retain) UIButton* disclosure;
@property(nonatomic, retain) UIButton* toggle;
@property(nonatomic, retain) UIImageView* icon;
@property(nonatomic, retain) UILabel* titleLabel;
@property(nonatomic, assign) IupCocoaTouchTreeView* owner;
@property(nonatomic, assign) IupCocoaTouchTreeNode* node;
@end

@implementation IupCocoaTouchTreeCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString*)reuseIdentifier
{
	self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:reuseIdentifier];
	if (self)
	{
		_disclosure = [[UIButton buttonWithType:UIButtonTypeCustom] retain];
		_disclosure.autoresizingMask = UIViewAutoresizingNone;
		[_disclosure addTarget:self action:@selector(disclosureTapped:) forControlEvents:UIControlEventTouchUpInside];
		[self.contentView addSubview:_disclosure];

		_toggle = [[UIButton buttonWithType:UIButtonTypeCustom] retain];
		_toggle.autoresizingMask = UIViewAutoresizingNone;
		[_toggle addTarget:self action:@selector(toggleTapped:) forControlEvents:UIControlEventTouchUpInside];
		[self.contentView addSubview:_toggle];

		_icon = [[UIImageView alloc] init];
		_icon.contentMode = UIViewContentModeScaleAspectFit;
		[self.contentView addSubview:_icon];

		_titleLabel = [[UILabel alloc] init];
		_titleLabel.font = [UIFont systemFontOfSize:[UIFont systemFontSize]];
		_titleLabel.textColor = [UIColor labelColor];
		[self.contentView addSubview:_titleLabel];

		self.selectionStyle = UITableViewCellSelectionStyleDefault;
	}
	return self;
}

- (void)dealloc
{
	[_disclosure release];
	[_toggle release];
	[_icon release];
	[_titleLabel release];
	[super dealloc];
}

- (void)layoutSubviews
{
	[super layoutSubviews];

	Ihandle* ih = _owner ? _owner.ihandle : NULL;
	CGFloat per_level = cocoaTouchTreeIndentation(ih);
	BOOL hide_buttons = NO;
	if (ih)
		hide_buttons = iupAttribGetBoolean(ih, "HIDEBUTTONS") ? YES : NO;

	CGFloat depth = self.node ? self.node.depth : 0;
	CGFloat indent = 16 + depth * per_level;
	CGFloat h = self.contentView.bounds.size.height;
	CGFloat w = self.contentView.bounds.size.width;
	CGFloat gap = 6;
	CGFloat disclosureW = hide_buttons ? 0 : 22;
	CGFloat iconW = 20;
	CGFloat toggleW = _toggle.hidden ? 0 : 24;

	_disclosure.hidden = hide_buttons;
	_disclosure.frame = CGRectMake(indent, 0, disclosureW, h);
	CGFloat toggle_x = indent + disclosureW + (hide_buttons ? 0 : gap);
	_toggle.frame = CGRectMake(toggle_x, (h - toggleW)/2, toggleW, toggleW);
	CGFloat icon_x = toggle_x + toggleW + (toggleW > 0 ? gap : 0);
	_icon.frame = CGRectMake(icon_x, (h - iconW)/2, iconW, iconW);
	CGFloat text_x = icon_x + iconW + gap;
	_titleLabel.frame = CGRectMake(text_x, 0, w - text_x - 8, h);
}

- (void)toggleTapped:(UIButton*)sender
{
	(void)sender;
	if (!_owner || !_node) return;
	Ihandle* ih = _owner.ihandle;
	if (!ih) return;

	int id_ = iupTreeFindNodeId(ih, (InodeHandle*)_node);
	if (id_ < 0) return;

	if (ih->data->show_toggle == 2)
	{
		if (_node.toggleValue == 1)       _node.toggleValue = -1;
		else if (_node.toggleValue == -1) _node.toggleValue = 0;
		else                              _node.toggleValue = 1;
	}
	else
		_node.toggleValue = (_node.toggleValue == 1) ? 0 : 1;

	IFnii cb = (IFnii)IupGetCallback(ih, "TOGGLEVALUE_CB");
	if (cb && cb(ih, id_, _node.toggleValue) == IUP_CLOSE)
		IupExitLoop();

	if (iupAttribGetBoolean(ih, "MARKWHENTOGGLE"))
		IupSetAttributeId(ih, "MARKED", id_, _node.toggleValue > 0 ? "YES" : "NO");

	int row = cocoaTouchTreeVisibleRow(_owner, _node);
	if (row >= 0)
		[_owner.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:row inSection:0]]
		                        withRowAnimation:UITableViewRowAnimationNone];
}

- (void)disclosureTapped:(UIButton*)sender
{
	(void)sender;
	if (!_owner || !_node || _node.kind != ITREE_BRANCH) return;
	Ihandle* ih = _owner.ihandle;
	if (!ih) return;

	int id_ = iupTreeFindNodeId(ih, (InodeHandle*)_node);
	if (id_ < 0) return;

	IFni cb;
	if (_node.expanded)
	{
		cb = (IFni)IupGetCallback(ih, "BRANCHCLOSE_CB");
		if (cb && cb(ih, id_) == IUP_IGNORE) return;
		_node.expanded = NO;
	}
	else
	{
		cb = (IFni)IupGetCallback(ih, "BRANCHOPEN_CB");
		if (cb && cb(ih, id_) == IUP_IGNORE) return;
		_node.expanded = YES;
	}

	cocoaTouchTreeRebuildFlat(_owner);
	[_owner.tableView reloadData];
}

@end


@implementation IupCocoaTouchTreeView

- (instancetype)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self)
	{
		_rootNodes = [[NSMutableArray alloc] init];
		_flatNodes = [[NSMutableArray alloc] init];
		_focusNodeId = -1;
		_markStartId = -1;

		_tableView = [[UITableView alloc] initWithFrame:self.bounds style:UITableViewStylePlain];
		_tableView.dataSource = self;
		_tableView.delegate = self;
		_tableView.rowHeight = 32;
		_tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
		_tableView.autoresizingMask = UIViewAutoresizingNone;
		[_tableView registerClass:[IupCocoaTouchTreeCell class] forCellReuseIdentifier:@"iupnode"];
		[self addSubview:_tableView];

		_defaultLeafImage      = [[UIImage systemImageNamed:@"doc"] retain];
		_defaultCollapsedImage = [[UIImage systemImageNamed:@"folder"] retain];
		_defaultExpandedImage  = [[UIImage systemImageNamed:@"folder.fill"] retain];

		UITapGestureRecognizer* dbl = [[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleDoubleTap:)] autorelease];
		dbl.numberOfTapsRequired = 2;
		dbl.cancelsTouchesInView = NO;
		[_tableView addGestureRecognizer:dbl];

		UILongPressGestureRecognizer* lp = [[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)] autorelease];
		lp.minimumPressDuration = 0.5;
		lp.cancelsTouchesInView = NO;
		[_tableView addGestureRecognizer:lp];
	}
	return self;
}

- (void)dealloc
{
	_tableView.delegate = nil;
	_tableView.dataSource = nil;
	[_tableView release];
	[_rootNodes release];
	[_flatNodes release];
	[_defaultLeafImage release];
	[_defaultCollapsedImage release];
	[_defaultExpandedImage release];
	[_bgColor release];
	[_fgColor release];
	[_defaultFont release];
	[super dealloc];
}

- (void)layoutSubviews
{
	[super layoutSubviews];
	_tableView.frame = self.bounds;
}

- (void)handleDoubleTap:(UITapGestureRecognizer*)g
{
	if (g.state != UIGestureRecognizerStateEnded) return;
	CGPoint p = [g locationInView:_tableView];
	NSIndexPath* ip = [_tableView indexPathForRowAtPoint:p];
	if (!ip || ip.row < 0 || (NSUInteger)ip.row >= _flatNodes.count) return;

	IupCocoaTouchTreeNode* node = _flatNodes[ip.row];
	int id_ = iupTreeFindNodeId(_ihandle, (InodeHandle*)node);
	if (id_ < 0) return;

	if (node.kind == ITREE_LEAF)
	{
		IFni cb = (IFni)IupGetCallback(_ihandle, "EXECUTELEAF_CB");
		if (cb) cb(_ihandle, id_);
	}
	else
	{
		IFni cb = (IFni)IupGetCallback(_ihandle, "EXECUTEBRANCH_CB");
		if (cb) cb(_ihandle, id_);
	}
}

- (void)handleLongPress:(UILongPressGestureRecognizer*)g
{
	if (g.state != UIGestureRecognizerStateBegan) return;
	CGPoint p = [g locationInView:_tableView];
	NSIndexPath* ip = [_tableView indexPathForRowAtPoint:p];
	if (!ip || ip.row < 0 || (NSUInteger)ip.row >= _flatNodes.count) return;

	IupCocoaTouchTreeNode* node = _flatNodes[ip.row];
	int id_ = iupTreeFindNodeId(_ihandle, (InodeHandle*)node);
	if (id_ < 0) return;

	IFni cb = (IFni)IupGetCallback(_ihandle, "RIGHTCLICK_CB");
	if (cb) cb(_ihandle, id_);
}

- (NSInteger)tableView:(UITableView*)tableView numberOfRowsInSection:(NSInteger)section
{
	(void)tableView; (void)section;
	return _flatNodes.count;
}

- (UITableViewCell*)tableView:(UITableView*)tableView cellForRowAtIndexPath:(NSIndexPath*)indexPath
{
	IupCocoaTouchTreeCell* cell = [tableView dequeueReusableCellWithIdentifier:@"iupnode" forIndexPath:indexPath];
	if ((NSUInteger)indexPath.row >= _flatNodes.count) return cell;

	IupCocoaTouchTreeNode* node = _flatNodes[indexPath.row];
	cell.node = node;
	cell.owner = self;

	cell.titleLabel.text = node.title ? node.title : @"";
	cell.titleLabel.font = node.font ? node.font : (_defaultFont ? _defaultFont : cell.titleLabel.font);
	cell.titleLabel.textColor = node.textColor ? node.textColor : (_fgColor ? _fgColor : [UIColor labelColor]);

	if (node.marked && iupTreeFindNodeId(_ihandle, (InodeHandle*)node) != _focusNodeId)
		cell.backgroundColor = [UIColor systemFillColor];
	else if (_bgColor)
		cell.backgroundColor = _bgColor;
	else
		cell.backgroundColor = [UIColor clearColor];
	cell.contentView.backgroundColor = [UIColor clearColor];

	if (_ihandle && _ihandle->data->show_toggle && node.toggleVisible)
	{
		NSString* sym = @"square";
		if (node.toggleValue == 1) sym = @"checkmark.square.fill";
		else if (node.toggleValue == -1) sym = @"minus.square.fill";
		[cell.toggle setImage:[UIImage systemImageNamed:sym] forState:UIControlStateNormal];
		cell.toggle.hidden = NO;
	}
	else
	{
		[cell.toggle setImage:nil forState:UIControlStateNormal];
		cell.toggle.hidden = YES;
	}

	UIImage* icon = node.image;
	if (node.kind == ITREE_BRANCH && node.expanded && node.imageExpanded)
		icon = node.imageExpanded;
	if (!icon)
	{
		if (node.kind == ITREE_LEAF) icon = _defaultLeafImage;
		else icon = node.expanded ? _defaultExpandedImage : _defaultCollapsedImage;
	}
	cell.icon.image = icon;
	cell.icon.hidden = (icon == nil);

	if (node.kind == ITREE_BRANCH)
	{
		NSString* sym = node.expanded ? @"chevron.down" : @"chevron.right";
		UIImage* arrow = [UIImage systemImageNamed:sym];
		[cell.disclosure setImage:arrow forState:UIControlStateNormal];
		cell.disclosure.hidden = NO;
	}
	else
	{
		[cell.disclosure setImage:nil forState:UIControlStateNormal];
		cell.disclosure.hidden = YES;
	}

	[cell setNeedsLayout];
	return cell;
}

- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath
{
	if ((NSUInteger)indexPath.row >= _flatNodes.count) return;

	IupCocoaTouchTreeNode* node = _flatNodes[indexPath.row];
	int id_ = iupTreeFindNodeId(_ihandle, (InodeHandle*)node);
	if (id_ < 0) return;

	_focusNodeId = id_;

	if (_ihandle->data->mark_mode == ITREE_MARK_MULTIPLE)
	{
		NSMutableArray<NSIndexPath*>* stale = [NSMutableArray array];
		for (NSUInteger r = 0; r < _flatNodes.count; r++)
		{
			if (_flatNodes[r] != node && [_flatNodes[r] marked])
				[stale addObject:[NSIndexPath indexPathForRow:(NSInteger)r inSection:0]];
		}
		for (int i = 0; i < _ihandle->data->node_count; i++)
		{
			IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)_ihandle->data->node_cache[i].node_handle;
			if (n) n.marked = (n == node) ? YES : NO;
		}
		if (stale.count)
			[tableView reloadRowsAtIndexPaths:stale withRowAnimation:UITableViewRowAnimationNone];
	}

	IFnii cb = (IFnii)IupGetCallback(_ihandle, "SELECTION_CB");
	if (cb) cb(_ihandle, id_, 1);
}

- (void)tableView:(UITableView*)tableView didDeselectRowAtIndexPath:(NSIndexPath*)indexPath
{
	(void)tableView;
	if ((NSUInteger)indexPath.row >= _flatNodes.count) return;

	IupCocoaTouchTreeNode* node = _flatNodes[indexPath.row];
	int id_ = iupTreeFindNodeId(_ihandle, (InodeHandle*)node);
	if (id_ < 0) return;

	IFnii cb = (IFnii)IupGetCallback(_ihandle, "SELECTION_CB");
	if (cb) cb(_ihandle, id_, 0);
}

@end


static void cocoaTouchTreeReleaseSubtree(IupCocoaTouchTreeNode* node)
{
	for (IupCocoaTouchTreeNode* c in node.children)
		cocoaTouchTreeReleaseSubtree(c);
	[node.children removeAllObjects];
}

IUP_SDK_API void iupdrvTreeAddNode(Ihandle* ih, int prev_id, int kind, const char* title, int add)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return;

	InodeHandle* prev_handle = iupTreeGetNode(ih, prev_id);
	if (!prev_handle && prev_id != -1) return;

	IupCocoaTouchTreeNode* prev_node = (IupCocoaTouchTreeNode*)prev_handle;
	int kind_prev = prev_node ? prev_node.kind : 0;

	IupCocoaTouchTreeNode* new_node = [[[IupCocoaTouchTreeNode alloc] init] autorelease];
	new_node.kind = kind;
	new_node.title = title ? [NSString stringWithUTF8String:title] : @"";
	new_node.expanded = ih->data->add_expanded ? YES : NO;

	if (!prev_node)
	{
		new_node.depth = 0;
		new_node.parent = nil;
		[view.rootNodes addObject:new_node];
	}
	else if (add && prev_node.kind == ITREE_BRANCH)
	{
		new_node.depth = prev_node.depth + 1;
		new_node.parent = prev_node;
		[prev_node.children insertObject:new_node atIndex:0];
	}
	else
	{
		IupCocoaTouchTreeNode* parent = prev_node.parent;
		new_node.depth = prev_node.depth;
		new_node.parent = parent;
		if (parent)
		{
			int idx = cocoaTouchTreeIndexInParent(prev_node);
			[parent.children insertObject:new_node atIndex:(NSUInteger)(idx + 1)];
		}
		else
		{
			NSUInteger idx = [view.rootNodes indexOfObjectIdenticalTo:prev_node];
			[view.rootNodes insertObject:new_node atIndex:(idx == NSNotFound ? view.rootNodes.count : idx + 1)];
		}
	}

	/* +1 to balance the raw pointer in node_cache */
	[new_node retain];

	iupTreeAddToCache(ih, add, kind_prev, prev_handle, (InodeHandle*)new_node);

	cocoaTouchTreeRebuildFlat(view);
	[view.tableView reloadData];
}

IUP_SDK_API int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
	(void)ih;
	if (!node_handle) return 0;
	IupCocoaTouchTreeNode* node = (IupCocoaTouchTreeNode*)node_handle;
	return cocoaTouchTreeSubtreeCount(node);
}

IUP_SDK_API InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return NULL;
	if (view.focusNodeId < 0 || view.focusNodeId >= ih->data->node_count)
	{
		if (ih->data->node_count > 0) return ih->data->node_cache[0].node_handle;
		return NULL;
	}
	return ih->data->node_cache[view.focusNodeId].node_handle;
}

IUP_SDK_API void iupdrvTreeUpdateMarkMode(Ihandle* ih)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return;
	/* touch has no modifier key to extend a selection */
	view.tableView.allowsMultipleSelection = NO;
	[view.tableView reloadData];
}

static IupCocoaTouchTreeNode* cocoaTouchTreeCopySubtree(Ihandle* dst_ih, IupCocoaTouchTreeNode* src_node, int prev_id, int add)
{
	const char* title = src_node.title ? [src_node.title UTF8String] : "";
	iupdrvTreeAddNode(dst_ih, prev_id, src_node.kind, title, add);
	int new_id = iupAttribGetInt(dst_ih, "LASTADDNODE");
	IupCocoaTouchTreeNode* new_node = cocoaTouchTreeNodeFromId(dst_ih, new_id);
	if (!new_node) return nil;

	new_node.expanded     = src_node.expanded;
	new_node.textColor    = src_node.textColor;
	new_node.font         = src_node.font;
	new_node.image        = src_node.image;
	new_node.imageExpanded = src_node.imageExpanded;

	IupCocoaTouchTreeNode* prev_child = new_node;
	int first = 1;
	for (IupCocoaTouchTreeNode* child in src_node.children)
	{
		int pid = iupTreeFindNodeId(dst_ih, (InodeHandle*)prev_child);
		if (pid < 0) break;
		IupCocoaTouchTreeNode* added = cocoaTouchTreeCopySubtree(dst_ih, child, pid, first);
		if (added) prev_child = added;
		first = 0;
	}
	return new_node;
}

static void cocoaTouchTreeUpdateDepth(IupCocoaTouchTreeNode* node, int depth)
{
	node.depth = depth;
	for (IupCocoaTouchTreeNode* child in node.children)
		cocoaTouchTreeUpdateDepth(child, depth + 1);
}

static int cocoaTouchTreeCopyMoveNode(Ihandle* ih, int id_, const char* value, int is_copy)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 0;

	IupCocoaTouchTreeNode* src = cocoaTouchTreeNodeFromId(ih, id_);
	InodeHandle* dst_handle = iupTreeGetNodeFromString(ih, value);
	if (!src || !dst_handle) return 0;

	IupCocoaTouchTreeNode* dst = (IupCocoaTouchTreeNode*)dst_handle;
	if (src == dst) return 0;

	for (IupCocoaTouchTreeNode* p = dst; p; p = p.parent)
	{
		if (p == src) return 0;
	}

	int id_src = id_;
	int id_dst = iupTreeFindNodeId(ih, dst_handle);
	if (id_dst < 0) return 0;

	int position = 0;
	int id_new = id_dst + 1;
	if (dst.kind == ITREE_BRANCH)
	{
		if (dst.expanded)
			position = 1;
		else
			id_new += cocoaTouchTreeSubtreeCount(dst);
	}

	if (is_copy)
	{
		cocoaTouchTreeCopySubtree(ih, src, id_dst, position);
		cocoaTouchTreeRebuildFlat(view);
		[view.tableView reloadData];
		return 0;
	}

	if (id_new == id_src) return 0;

	int old_count = ih->data->node_count;
	int count = 1 + cocoaTouchTreeSubtreeCount(src);

	[src retain];
	if (src.parent) [src.parent.children removeObjectIdenticalTo:src];
	else            [view.rootNodes removeObjectIdenticalTo:src];

	if (position == 1)
	{
		src.parent = dst;
		[dst.children insertObject:src atIndex:0];
	}
	else if (dst.parent)
	{
		src.parent = dst.parent;
		int idx = cocoaTouchTreeIndexInParent(dst);
		[dst.parent.children insertObject:src atIndex:(NSUInteger)(idx + 1)];
	}
	else
	{
		src.parent = nil;
		NSUInteger idx = [view.rootNodes indexOfObjectIdenticalTo:dst];
		[view.rootNodes insertObject:src atIndex:(idx == NSNotFound ? view.rootNodes.count : idx + 1)];
	}
	[src release];

	cocoaTouchTreeUpdateDepth(src, src.parent ? src.parent.depth + 1 : 0);

	/* iupTreeCopyMoveCache expects the count to already include the moved block */
	ih->data->node_count = old_count + count;
	iupTreeCopyMoveCache(ih, id_src, id_new, count, 0);
	ih->data->node_count = old_count;

	cocoaTouchTreeRebuildFlat(view);
	[view.tableView reloadData];
	return 0;
}

static int cocoaTouchTreeSetMoveNodeAttrib(Ihandle* ih, int id_, const char* value)
{
	return cocoaTouchTreeCopyMoveNode(ih, id_, value, 0);
}

static int cocoaTouchTreeSetCopyNodeAttrib(Ihandle* ih, int id_, const char* value)
{
	return cocoaTouchTreeCopyMoveNode(ih, id_, value, 1);
}

IUP_SDK_API void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle* item_src, InodeHandle* item_dst)
{
	(void)src;
	IupCocoaTouchTreeView* dst_view = cocoaTouchTreeGetView(dst);
	if (!dst_view) return;
	IupCocoaTouchTreeNode* src_node = (IupCocoaTouchTreeNode*)item_src;
	IupCocoaTouchTreeNode* dst_node = (IupCocoaTouchTreeNode*)item_dst;
	if (!src_node || !dst_node) return;

	int prev_id = iupTreeFindNodeId(dst, (InodeHandle*)dst_node);
	if (prev_id < 0) return;

	int add = (dst_node.kind == ITREE_BRANCH && dst_node.expanded) ? 1 : 0;
	cocoaTouchTreeCopySubtree(dst, src_node, prev_id, add);
}

static int cocoaTouchTreeSetTitleIdAttrib(Ihandle* ih, int id_, const char* value)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;
	node.title = value ? [NSString stringWithUTF8String:value] : @"";
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (view) [view.tableView reloadData];
	return 0;
}

static char* cocoaTouchTreeGetTitleIdAttrib(Ihandle* ih, int id_)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node || !node.title) return NULL;
	return iupStrReturnStr([node.title UTF8String]);
}

static int cocoaTouchTreeSetStateIdAttrib(Ihandle* ih, int id_, const char* value)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node || node.kind != ITREE_BRANCH) return 0;
	node.expanded = iupStrEqualNoCase(value, "EXPANDED") ? YES : NO;
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (view)
	{
		cocoaTouchTreeRebuildFlat(view);
		[view.tableView reloadData];
	}
	return 0;
}

static char* cocoaTouchTreeGetStateIdAttrib(Ihandle* ih, int id_)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node || node.kind != ITREE_BRANCH) return NULL;
	return node.expanded ? "EXPANDED" : "COLLAPSED";
}

static char* cocoaTouchTreeGetKindIdAttrib(Ihandle* ih, int id_)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return NULL;
	return (node.kind == ITREE_LEAF) ? "LEAF" : "BRANCH";
}

static char* cocoaTouchTreeGetDepthIdAttrib(Ihandle* ih, int id_)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return NULL;
	return iupStrReturnInt(node.depth);
}

static char* cocoaTouchTreeGetChildCountIdAttrib(Ihandle* ih, int id_)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return NULL;
	return iupStrReturnInt((int)node.children.count);
}

static char* cocoaTouchTreeGetParentIdAttrib(Ihandle* ih, int id_)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node || !node.parent) return NULL;
	int pid = iupTreeFindNodeId(ih, (InodeHandle*)node.parent);
	return (pid >= 0) ? iupStrReturnInt(pid) : NULL;
}

static char* cocoaTouchTreeGetColorIdAttrib(Ihandle* ih, int id_)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node || !node.textColor) return NULL;
	return iupCocoaTouchColorFromNative(node.textColor);
}

static int cocoaTouchTreeSetColorIdAttrib(Ihandle* ih, int id_, const char* value)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;
	node.textColor = value ? iupCocoaTouchToNativeColor(value) : nil;
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (view) [view.tableView reloadData];
	return 0;
}

static int cocoaTouchTreeSetTitleFontIdAttrib(Ihandle* ih, int id_, const char* value)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;
	if (!value) { node.font = nil; }
	else
	{
		IupCocoaTouchFont* f = iupCocoaTouchFindFont(value);
		node.font = f ? f.nativeFont : nil;
	}
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (view) [view.tableView reloadData];
	return 0;
}

static int cocoaTouchTreeSetImageIdAttrib(Ihandle* ih, int id_, const char* value)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;
	UIImage* img = nil;
	if (value)
	{
		void* raw = iupImageGetImage(value, ih, 0, NULL);
		if (raw && [(id)raw isKindOfClass:[UIImage class]]) img = (UIImage*)raw;
	}
	node.image = img;
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (view) [view.tableView reloadData];
	return 0;
}

static int cocoaTouchTreeSetImageExpandedIdAttrib(Ihandle* ih, int id_, const char* value)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;
	UIImage* img = nil;
	if (value)
	{
		void* raw = iupImageGetImage(value, ih, 0, NULL);
		if (raw && [(id)raw isKindOfClass:[UIImage class]]) img = (UIImage*)raw;
	}
	node.imageExpanded = img;
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (view) [view.tableView reloadData];
	return 0;
}

static UIImage* cocoaTouchTreeResolveImage(Ihandle* ih, const char* value)
{
	if (!value) return nil;
	void* raw = iupImageGetImage(value, ih, 0, NULL);
	if (raw && [(id)raw isKindOfClass:[UIImage class]]) return (UIImage*)raw;
	return nil;
}

static int cocoaTouchTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 1;
	view.defaultLeafImage = cocoaTouchTreeResolveImage(ih, value);
	[view.tableView reloadData];
	return 1;
}

static int cocoaTouchTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 1;
	view.defaultCollapsedImage = cocoaTouchTreeResolveImage(ih, value);
	[view.tableView reloadData];
	return 1;
}

static int cocoaTouchTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 1;
	view.defaultExpandedImage = cocoaTouchTreeResolveImage(ih, value);
	[view.tableView reloadData];
	return 1;
}

static int cocoaTouchTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 0;
	UIColor* c = iupCocoaTouchToNativeColor(value);
	if (!c) return 0;
	view.bgColor = c;
	view.tableView.backgroundColor = c;
	view.backgroundColor = c;
	[view.tableView reloadData];
	return 1;
}

static int cocoaTouchTreeSetFgColorAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 0;
	UIColor* c = iupCocoaTouchToNativeColor(value);
	if (!c) return 0;
	view.fgColor = c;
	[view.tableView reloadData];
	return 1;
}

static int cocoaTouchTreeSetFontAttrib(Ihandle* ih, const char* value)
{
	if (!iupdrvSetFontAttrib(ih, value)) return 0;
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 1;
	IupCocoaTouchFont* f = iupCocoaTouchGetFont(ih);
	view.defaultFont = (f ? f.nativeFont : nil);
	[view.tableView reloadData];
	return 1;
}

static char* cocoaTouchTreeGetValueAttrib(Ihandle* ih)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return iupStrReturnInt(0);
	return iupStrReturnInt(view.focusNodeId < 0 ? 0 : view.focusNodeId);
}

static int cocoaTouchTreeSetValueAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view || !value) return 0;

	if (iupStrEqualNoCase(value, "CLEAR"))
	{
		view.focusNodeId = -1;
		for (NSIndexPath* ip in [view.tableView indexPathsForSelectedRows])
			[view.tableView deselectRowAtIndexPath:ip animated:NO];
		return 0;
	}

	int id_ = -1;
	if (iupStrToInt(value, &id_))
	{
	}
	else if (iupStrEqualNoCase(value, "ROOT") || iupStrEqualNoCase(value, "FIRST") || iupStrEqualNoCase(value, "HOME"))
	{
		id_ = 0;
	}
	else if (iupStrEqualNoCase(value, "LAST"))
	{
		id_ = ih->data->node_count - 1;
	}
	else if (iupStrEqualNoCase(value, "NEXT"))
	{
		id_ = view.focusNodeId + 1;
		if (id_ >= ih->data->node_count) id_ = ih->data->node_count - 1;
	}
	else if (iupStrEqualNoCase(value, "PREVIOUS"))
	{
		id_ = view.focusNodeId - 1;
		if (id_ < 0) id_ = 0;
	}
	else
	{
		return 0;
	}

	if (id_ < 0 || id_ >= ih->data->node_count) return 0;

	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;

	/* expand ancestors so the row is visible */
	IupCocoaTouchTreeNode* a = node.parent;
	while (a) { a.expanded = YES; a = a.parent; }
	cocoaTouchTreeRebuildFlat(view);
	[view.tableView reloadData];

	int row = cocoaTouchTreeVisibleRow(view, node);
	if (row < 0) return 0;

	view.focusNodeId = id_;
	NSIndexPath* ip = [NSIndexPath indexPathForRow:row inSection:0];
	[view.tableView selectRowAtIndexPath:ip animated:NO scrollPosition:UITableViewScrollPositionMiddle];
	return 0;
}

static char* cocoaTouchTreeGetRootCountAttrib(Ihandle* ih)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	return iupStrReturnInt(view ? (int)view.rootNodes.count : 0);
}

static int cocoaTouchTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view || !value) return 0;
	int id_;
	if (!iupStrToInt(value, &id_)) return 0;
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;
	int row = cocoaTouchTreeVisibleRow(view, node);
	if (row < 0) return 0;
	[view.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:row inSection:0]
	                      atScrollPosition:UITableViewScrollPositionTop animated:NO];
	return 0;
}

static int cocoaTouchTreeSetAddExpandedAttrib(Ihandle* ih, const char* value)
{
	ih->data->add_expanded = iupStrBoolean(value);
	return 1;
}

static int cocoaTouchTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 0;
	BOOL exp = iupStrBoolean(value) ? YES : NO;
	for (int i = 0; i < ih->data->node_count; i++)
	{
		IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
		if (n && n.kind == ITREE_BRANCH) n.expanded = exp;
	}
	cocoaTouchTreeRebuildFlat(view);
	[view.tableView reloadData];
	return 0;
}

static int cocoaTouchTreeSetMarkedIdAttrib(Ihandle* ih, int id_, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 0;
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;
	BOOL mark = iupStrBoolean(value) ? YES : NO;
	node.marked = mark;

	int row = cocoaTouchTreeVisibleRow(view, node);
	if (row < 0) return 0;
	NSIndexPath* ip = [NSIndexPath indexPathForRow:row inSection:0];
	[view.tableView reloadRowsAtIndexPaths:@[ip] withRowAnimation:UITableViewRowAnimationNone];
	return 0;
}

static char* cocoaTouchTreeGetMarkedIdAttrib(Ihandle* ih, int id_)
{
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return NULL;
	return iupStrReturnBoolean(node.marked);
}

static char* cocoaTouchTreeGetToggleValueAttrib(Ihandle* ih, int id_)
{
	if (!ih->data->show_toggle) return NULL;
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return NULL;
	if (node.toggleValue == -1) return "NOTDEF";
	return node.toggleValue ? "ON" : "OFF";
}

static int cocoaTouchTreeSetToggleValueAttrib(Ihandle* ih, int id_, const char* value)
{
	if (!ih->data->show_toggle) return 0;
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;

	if (ih->data->show_toggle == 2 && iupStrEqualNoCase(value, "NOTDEF"))
		node.toggleValue = -1;
	else
		node.toggleValue = iupStrBoolean(value) ? 1 : 0;

	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	int row = view ? cocoaTouchTreeVisibleRow(view, node) : -1;
	if (row >= 0)
		[view.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:row inSection:0]]
		                      withRowAnimation:UITableViewRowAnimationNone];
	return 0;
}

static char* cocoaTouchTreeGetToggleVisibleAttrib(Ihandle* ih, int id_)
{
	if (!ih->data->show_toggle) return NULL;
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return NULL;
	return iupStrReturnBoolean(node.toggleVisible);
}

static int cocoaTouchTreeSetToggleVisibleAttrib(Ihandle* ih, int id_, const char* value)
{
	if (!ih->data->show_toggle) return 0;
	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;

	node.toggleVisible = iupStrBoolean(value) ? YES : NO;

	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	int row = view ? cocoaTouchTreeVisibleRow(view, node) : -1;
	if (row >= 0)
		[view.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:row inSection:0]]
		                      withRowAnimation:UITableViewRowAnimationNone];
	return 0;
}

static void cocoaTouchTreeMarkRange(Ihandle* ih, int from, int to)
{
	if (from > to) { int t = from; from = to; to = t; }
	if (from < 0) from = 0;
	for (int i = from; i <= to && i < ih->data->node_count; i++)
	{
		IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
		if (n) n.marked = YES;
	}
}

static int cocoaTouchTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view || !value) return 0;
	if (ih->data->mark_mode == ITREE_MARK_SINGLE) return 0;

	if (iupStrEqualNoCase(value, "CLEARALL") || iupStrEqualNoCase(value, "MARKALL"))
	{
		BOOL mark = iupStrEqualNoCase(value, "MARKALL") ? YES : NO;
		for (int i = 0; i < ih->data->node_count; i++)
		{
			IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
			if (n) n.marked = mark;
		}
	}
	else if (iupStrEqualNoCase(value, "INVERTALL"))
	{
		for (int i = 0; i < ih->data->node_count; i++)
		{
			IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
			if (n) n.marked = !n.marked;
		}
	}
	else if (iupStrEqualNoCase(value, "BLOCK"))
	{
		cocoaTouchTreeMarkRange(ih, view.focusNodeId, view.markStartId);
	}
	else if (iupStrEqualPartial(value, "INVERT"))
	{
		int id_ = view.focusNodeId;
		iupStrToInt(value + strlen("INVERT"), &id_);
		IupCocoaTouchTreeNode* n = cocoaTouchTreeNodeFromId(ih, id_);
		if (n) n.marked = !n.marked;
	}
	else
	{
		int from = -1, to = -1;
		if (iupStrToIntInt(value, &from, &to, '-') != 2)
			return 0;
		cocoaTouchTreeMarkRange(ih, from, to);
	}

	[view.tableView reloadData];
	return 0;
}

static char* cocoaTouchTreeGetMarkedNodesAttrib(Ihandle* ih)
{
	char* str = iupStrGetMemory(ih->data->node_count + 1);
	for (int i = 0; i < ih->data->node_count; i++)
	{
		IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
		str[i] = (n && n.marked) ? '+' : '-';
	}
	str[ih->data->node_count] = 0;
	return str;
}

static int cocoaTouchTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view || !value) return 0;

	int count = (int)strlen(value);
	if (count > ih->data->node_count) count = ih->data->node_count;
	for (int i = 0; i < count; i++)
	{
		IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
		if (n) n.marked = (value[i] == '+') ? YES : NO;
	}
	[view.tableView reloadData];
	return 0;
}

static int cocoaTouchTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view || !value) return 0;
	int id_;
	if (!iupStrToInt(value, &id_)) return 0;
	view.markStartId = id_;
	return 0;
}

static int cocoaTouchTreeSetMarkModeAttrib(Ihandle* ih, const char* value)
{
	ih->data->mark_mode = iupStrEqualNoCase(value, "MULTIPLE") ? ITREE_MARK_MULTIPLE : ITREE_MARK_SINGLE;
	iupdrvTreeUpdateMarkMode(ih);
	return 1;
}

static int cocoaTouchTreeSetDelNodeAttrib(Ihandle* ih, int id_, const char* value)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 0;

	int old_count = ih->data->node_count;
	int is_all      = value && iupStrEqualNoCase(value, "ALL");
	int is_children = value && iupStrEqualNoCase(value, "CHILDREN");
	int is_selected = value && iupStrEqualNoCase(value, "SELECTED");
	int is_marked   = value && iupStrEqualNoCase(value, "MARKED");

	IFns remove_cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");

	if (is_marked)
	{
		NSMutableArray<IupCocoaTouchTreeNode*>* nodes = [NSMutableArray array];
		for (int i = 0; i < old_count; i++)
		{
			IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
			if (n && n.marked) [nodes addObject:n];
		}
		if (nodes.count == 0 && ih->data->mark_mode == ITREE_MARK_SINGLE)
		{
			IupCocoaTouchTreeNode* f = cocoaTouchTreeNodeFromId(ih, view.focusNodeId);
			if (f) [nodes addObject:f];
		}
		/* delete in descending id order so removals don't shift the rest */
		[nodes sortUsingComparator:^NSComparisonResult(IupCocoaTouchTreeNode* a, IupCocoaTouchTreeNode* b) {
			int ia = iupTreeFindNodeId(ih, (InodeHandle*)a);
			int ib = iupTreeFindNodeId(ih, (InodeHandle*)b);
			if (ia > ib) return NSOrderedAscending;
			if (ia < ib) return NSOrderedDescending;
			return NSOrderedSame;
		}];
		for (IupCocoaTouchTreeNode* n in nodes)
		{
			int cur_id = iupTreeFindNodeId(ih, (InodeHandle*)n);
			if (cur_id < 0) continue;
			int desc = iupdrvTreeTotalChildCount(ih, (InodeHandle*)n);
			int count = desc + 1;
			if (remove_cb)
			{
				for (int i = cur_id; i < cur_id + count && i < ih->data->node_count; i++)
					remove_cb(ih, (char*)ih->data->node_cache[i].userdata);
			}
			cocoaTouchTreeReleaseSubtree(n);
			if (n.parent) [n.parent.children removeObjectIdenticalTo:n];
			else          [view.rootNodes removeObjectIdenticalTo:n];
			for (int i = cur_id; i < cur_id + count && i < ih->data->node_count; i++)
			{
				IupCocoaTouchTreeNode* m = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
				[m release];
			}
			ih->data->node_count -= count;
			iupTreeDelFromCache(ih, cur_id, count);
		}
		cocoaTouchTreeRebuildFlat(view);
		[view.tableView reloadData];
		return 0;
	}

	if (is_all)
	{
		if (remove_cb)
		{
			for (int i = 0; i < old_count; i++)
				remove_cb(ih, (char*)ih->data->node_cache[i].userdata);
		}
		for (int i = 0; i < old_count; i++)
		{
			IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
			[n release];
			ih->data->node_cache[i].node_handle = NULL;
		}
		[view.rootNodes removeAllObjects];
		ih->data->node_count = 0;
		iupAttribSet(ih, "LASTADDNODE", NULL);
	}
	else
	{
		InodeHandle* target = NULL;
		int start_id = id_;
		if (is_selected)
		{
			target = iupdrvTreeGetFocusNode(ih);
			if (target) start_id = iupTreeFindNodeId(ih, target);
		}
		else
		{
			target = iupTreeGetNode(ih, id_);
		}
		if (!target) return 0;

		int desc = iupdrvTreeTotalChildCount(ih, target);
		int count = is_children ? desc : (desc + 1);
		int del_first = is_children ? (start_id + 1) : start_id;

		if (remove_cb)
		{
			for (int i = del_first; i < del_first + count && i < old_count; i++)
				remove_cb(ih, (char*)ih->data->node_cache[i].userdata);
		}

		IupCocoaTouchTreeNode* target_node = (IupCocoaTouchTreeNode*)target;
		if (is_children)
		{
			for (IupCocoaTouchTreeNode* c in [[target_node.children copy] autorelease])
				cocoaTouchTreeReleaseSubtree(c);
			[target_node.children removeAllObjects];
		}
		else
		{
			cocoaTouchTreeReleaseSubtree(target_node);
			if (target_node.parent)
				[target_node.parent.children removeObjectIdenticalTo:target_node];
			else
				[view.rootNodes removeObjectIdenticalTo:target_node];
		}

		for (int i = del_first; i < del_first + count && i < old_count; i++)
		{
			IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
			[n release];
		}

		ih->data->node_count = old_count - count;
		iupTreeDelFromCache(ih, del_first, count);
	}

	cocoaTouchTreeRebuildFlat(view);
	[view.tableView reloadData];
	return 0;
}

static int cocoaTouchTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return 0;
	int id_ = view.focusNodeId;
	if (id_ < 0) return 0;

	IupCocoaTouchTreeNode* node = cocoaTouchTreeNodeFromId(ih, id_);
	if (!node) return 0;

	IFni show_cb = (IFni)IupGetCallback(ih, "SHOWRENAME_CB");
	if (show_cb && show_cb(ih, id_) == IUP_IGNORE) return 0;

	UIViewController* top = iupCocoaTouchFindTopPresentedViewController();
	if (!top) return 0;

	Ihandle* ih_ref = ih;
	int id_ref = id_;
	UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Rename"
	                                                                message:nil
	                                                         preferredStyle:UIAlertControllerStyleAlert];
	NSString* current = node.title ? node.title : @"";
	[alert addTextFieldWithConfigurationHandler:^(UITextField* tf) {
		tf.text = current;
		tf.clearButtonMode = UITextFieldViewModeWhileEditing;
	}];
	[alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
	[alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* a) {
		(void)a;
		UITextField* tf = alert.textFields.firstObject;
		NSString* new_title = tf.text ? tf.text : @"";
		IFnis rename_cb = (IFnis)IupGetCallback(ih_ref, "RENAME_CB");
		int ret = IUP_DEFAULT;
		if (rename_cb) ret = rename_cb(ih_ref, id_ref, (char*)[new_title UTF8String]);
		if (ret != IUP_IGNORE)
		{
			IupCocoaTouchTreeNode* n2 = cocoaTouchTreeNodeFromId(ih_ref, id_ref);
			if (n2) n2.title = new_title;
			IupCocoaTouchTreeView* v2 = cocoaTouchTreeGetView(ih_ref);
			if (v2) [v2.tableView reloadData];
		}
	}]];
	[top presentViewController:alert animated:YES completion:nil];
	return 0;
}

static const void* IUP_COCOATOUCH_TREE_DND_KEY = "IUP_COCOATOUCH_TREE_DND_KEY";

@interface IupCocoaTouchTreeDnDDelegate : NSObject <UITableViewDragDelegate, UITableViewDropDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, copy)   NSArray<NSString*>* dragTypes;
@property(nonatomic, copy)   NSArray<NSString*>* dropTypes;
@end

static int cocoaTouchTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view || !view.tableView) return -1;
	NSIndexPath* ip = [view.tableView indexPathForRowAtPoint:CGPointMake((CGFloat)x, (CGFloat)y)];
	if (!ip || ip.row < 0 || (NSUInteger)ip.row >= [view.flatNodes count]) return -1;
	IupCocoaTouchTreeNode* node = [view.flatNodes objectAtIndex:(NSUInteger)ip.row];
	return iupTreeFindNodeId(ih, (InodeHandle*)node);
}

@implementation IupCocoaTouchTreeDnDDelegate

- (NSArray<UIDragItem*>*)tableView:(UITableView*)tableView itemsForBeginningDragSession:(id<UIDragSession>)session atIndexPath:(NSIndexPath*)indexPath
{
	(void)session;
	if (!_ihandle || !iupObjectCheck(_ihandle) || [_dragTypes count] == 0) return @[];

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
	/* defer so DROPDATA_CB reads _IUP_TREE_SOURCEID first */
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
	if (!_ihandle || !iupObjectCheck(_ihandle) || [_dropTypes count] == 0) return NO;
	return [self firstMatchingUTI:session] != nil;
}

- (UITableViewDropProposal*)tableView:(UITableView*)tableView dropSessionDidUpdate:(id<UIDropSession>)session withDestinationIndexPath:(NSIndexPath*)destinationIndexPath
{
	(void)tableView; (void)destinationIndexPath;
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
	IFnsViii drop_cb = (IFnsViii)IupGetCallback(_ihandle, "DROPDATA_CB");
	if (!drop_cb) return;

	id<UIDropSession> session = coordinator.session;
	NSString* match = [self firstMatchingUTI:session];
	if (!match) return;

	/* UIKit drops gap-above-N, IUP wants insert-after; anchor on row N-1 */
	int drop_x, drop_y;
	NSIndexPath* dst = coordinator.destinationIndexPath;
	NSInteger anchor_row = -1;
	if (dst) anchor_row = dst.row - 1;
	if (anchor_row < 0)
	{
		NSInteger n = [tableView numberOfRowsInSection:0];
		if (n <= 0) return;
		anchor_row = 0;
	}
	CGRect rect = [tableView rectForRowAtIndexPath:[NSIndexPath indexPathForRow:anchor_row inSection:0]];
	drop_x = (int)CGRectGetMidX(rect);
	drop_y = (int)CGRectGetMidY(rect);

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

static void cocoaTouchTreeUpdateDragDrop(Ihandle* ih)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view || !view.tableView) return;
	UITableView* table = view.tableView;

	BOOL want_drag = iupAttribGetBoolean(ih, "DRAGSOURCE");
	BOOL want_drop = iupAttribGetBoolean(ih, "DROPTARGET");

	IupCocoaTouchTreeDnDDelegate* delegate = objc_getAssociatedObject(table, IUP_COCOATOUCH_TREE_DND_KEY);

	if (!want_drag && !want_drop)
	{
		if (delegate)
		{
			table.dragInteractionEnabled = NO;
			table.dragDelegate = nil;
			table.dropDelegate = nil;
			objc_setAssociatedObject(table, IUP_COCOATOUCH_TREE_DND_KEY, nil, OBJC_ASSOCIATION_RETAIN);
		}
		return;
	}

	if (!delegate)
	{
		delegate = [[IupCocoaTouchTreeDnDDelegate alloc] init];
		delegate.ihandle = ih;
		objc_setAssociatedObject(table, IUP_COCOATOUCH_TREE_DND_KEY, delegate, OBJC_ASSOCIATION_RETAIN);
		[delegate release];
	}

	delegate.dragTypes = iupCocoaTouchDragParseTypes(iupAttribGet(ih, "DRAGTYPES"));
	delegate.dropTypes = iupCocoaTouchDragParseTypes(iupAttribGet(ih, "DROPTYPES"));

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

static int cocoaTouchTreeSetDragSourceAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "DRAGSOURCE", value);
	cocoaTouchTreeUpdateDragDrop(ih);
	return 1;
}

static int cocoaTouchTreeSetDropTargetAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "DROPTARGET", value);
	cocoaTouchTreeUpdateDragDrop(ih);
	return 1;
}

static int cocoaTouchTreeSetDragTypesAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "DRAGTYPES", value);
	cocoaTouchTreeUpdateDragDrop(ih);
	return 1;
}

static int cocoaTouchTreeSetDropTypesAttrib(Ihandle* ih, const char* value)
{
	iupAttribSetStr(ih, "DROPTYPES", value);
	cocoaTouchTreeUpdateDragDrop(ih);
	return 1;
}

static int cocoaTouchTreeMapMethod(Ihandle* ih)
{
	IupCocoaTouchTreeView* view = [[IupCocoaTouchTreeView alloc] initWithFrame:CGRectZero];
	view.ihandle = ih;
	ih->handle = view;
	objc_setAssociatedObject(view, IHANDLE_ASSOCIATED_OBJ_KEY, (id)ih, OBJC_ASSOCIATION_ASSIGN);

	iupCocoaTouchAddToParent(ih);

	iupdrvTreeUpdateMarkMode(ih);

	if (ih->data->spacing > 0)
		view.tableView.rowHeight = 32 + 2 * ih->data->spacing;

	if (iupAttribGetInt(ih, "ADDROOT"))
		iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

	IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)cocoaTouchTreeConvertXYToPos);
	cocoaTouchTreeUpdateDragDrop(ih);

	return IUP_NOERROR;
}

static void cocoaTouchTreeUnMapMethod(Ihandle* ih)
{
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (!view) return;

	if (view.tableView)
	{
		view.tableView.dragDelegate = nil;
		view.tableView.dropDelegate = nil;
		objc_setAssociatedObject(view.tableView, IUP_COCOATOUCH_TREE_DND_KEY, nil, OBJC_ASSOCIATION_RETAIN);
	}

	IFns remove_cb = (IFns)IupGetCallback(ih, "NODEREMOVED_CB");
	if (ih->data && ih->data->node_cache)
	{
		for (int i = 0; i < ih->data->node_count; i++)
		{
			if (remove_cb)
				remove_cb(ih, (char*)ih->data->node_cache[i].userdata);
			IupCocoaTouchTreeNode* n = (IupCocoaTouchTreeNode*)ih->data->node_cache[i].node_handle;
			[n release];
			ih->data->node_cache[i].node_handle = NULL;
		}
	}
	[view.rootNodes removeAllObjects];

	iupCocoaTouchRemoveFromParent(ih);
	[view release];
	ih->handle = NULL;
}

IUP_SDK_API void iupdrvTreeAddBorders(Ihandle* ih, int *w, int *h)
{
	(void)ih;
	*w += 40;  /* disclosure indicator + 1 indent step */
	*h += 4;
}

static int cocoaTouchTreeSetActiveAttrib(Ihandle* ih, const char* value)
{
	iupBaseSetActiveAttrib(ih, value);

	UIView* view = (UIView*)ih->handle;
	if ([view isKindOfClass:[UIView class]])
		view.alpha = iupStrBoolean(value) ? 1.0 : 0.5;

	return 1;
}

static char* cocoaTouchTreeGetIndentationAttrib(Ihandle* ih)
{
	return iupStrReturnInt(cocoaTouchTreeIndentation(ih));
}

static int cocoaTouchTreeSetIndentationAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (view) [view.tableView reloadData];
	return 1; /* store; the cell reads INDENTATION when laying out */
}

static int cocoaTouchTreeSetSpacingAttrib(Ihandle* ih, const char* value)
{
	iupStrToInt(value, &ih->data->spacing);
	if (ih->data->spacing < 0) ih->data->spacing = 0;

	IupCocoaTouchTreeView* view = cocoaTouchTreeGetView(ih);
	if (view)
	{
		view.tableView.rowHeight = 32 + 2 * ih->data->spacing;
		[view.tableView reloadData];
		return 0;
	}
	return 1;
}

IUP_SDK_API void iupdrvTreeInitClass(Iclass* ic)
{
	ic->Map = cocoaTouchTreeMapMethod;
	ic->UnMap = cocoaTouchTreeUnMapMethod;
	ic->LayoutUpdate = iupdrvBaseLayoutUpdateMethod;

	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, cocoaTouchTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, cocoaTouchTreeSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FONT", NULL, cocoaTouchTreeSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

	iupClassRegisterAttribute(ic, "ADDEXPANDED", NULL, cocoaTouchTreeSetAddExpandedAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "EXPANDALL", NULL, cocoaTouchTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "VALUE", cocoaTouchTreeGetValueAttrib, cocoaTouchTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ROOTCOUNT", cocoaTouchTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "TOPITEM", NULL, cocoaTouchTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

	iupClassRegisterAttributeId(ic, "TITLE", cocoaTouchTreeGetTitleIdAttrib, cocoaTouchTreeSetTitleIdAttrib, IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "STATE", cocoaTouchTreeGetStateIdAttrib, cocoaTouchTreeSetStateIdAttrib, IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "KIND", cocoaTouchTreeGetKindIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "DEPTH", cocoaTouchTreeGetDepthIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "CHILDCOUNT", cocoaTouchTreeGetChildCountIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "PARENT", cocoaTouchTreeGetParentIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

	iupClassRegisterAttributeId(ic, "MOVENODE", NULL, cocoaTouchTreeSetMoveNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "COPYNODE", NULL, cocoaTouchTreeSetCopyNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "DELNODE", NULL, cocoaTouchTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

	iupClassRegisterAttributeId(ic, "MARKED", cocoaTouchTreeGetMarkedIdAttrib, cocoaTouchTreeSetMarkedIdAttrib, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MARK", NULL, cocoaTouchTreeSetMarkAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "STARTING", NULL, cocoaTouchTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MARKSTART", NULL, cocoaTouchTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "TOGGLEVALUE", cocoaTouchTreeGetToggleValueAttrib, cocoaTouchTreeSetToggleValueAttrib, IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "TOGGLEVISIBLE", cocoaTouchTreeGetToggleVisibleAttrib, cocoaTouchTreeSetToggleVisibleAttrib, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MARKWHENTOGGLE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MARKEDNODES", cocoaTouchTreeGetMarkedNodesAttrib, cocoaTouchTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "MARKMODE", NULL, cocoaTouchTreeSetMarkModeAttrib, IUPAF_SAMEASSYSTEM, "SINGLE", IUPAF_NOT_MAPPED);

	iupClassRegisterAttributeId(ic, "COLOR", cocoaTouchTreeGetColorIdAttrib, cocoaTouchTreeSetColorIdAttrib, IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "TITLEFONT", NULL, cocoaTouchTreeSetTitleFontIdAttrib, IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "IMAGE", NULL, cocoaTouchTreeSetImageIdAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
	iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, cocoaTouchTreeSetImageExpandedIdAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "IMAGELEAF", NULL, cocoaTouchTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, cocoaTouchTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", NULL, cocoaTouchTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "RENAME", NULL, cocoaTouchTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

	iupClassRegisterAttribute(ic, "SPACING", iupTreeGetSpacingAttrib, cocoaTouchTreeSetSpacingAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "INDENTATION", cocoaTouchTreeGetIndentationAttrib, cocoaTouchTreeSetIndentationAttrib, NULL, NULL, IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "HIDELINES", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "HIDEBUTTONS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "INFOTIP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SHOWDRAGDROP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "SHOWRENAME", NULL, NULL, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT);

	iupClassRegisterReplaceAttribFunc(ic, "DRAGSOURCE", NULL, cocoaTouchTreeSetDragSourceAttrib);
	iupClassRegisterReplaceAttribFunc(ic, "DRAGTYPES",  NULL, cocoaTouchTreeSetDragTypesAttrib);
	iupClassRegisterReplaceAttribFunc(ic, "DROPTARGET", NULL, cocoaTouchTreeSetDropTargetAttrib);
	iupClassRegisterReplaceAttribFunc(ic, "DROPTYPES",  NULL, cocoaTouchTreeSetDropTypesAttrib);
	iupClassRegisterReplaceAttribFunc(ic, "ACTIVE",     NULL, cocoaTouchTreeSetActiveAttrib);
}
