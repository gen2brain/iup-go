/** \file
 * \brief Table Control (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_classbase.h"
#include "iup_str.h"
#include "iup_image.h"
#include "iup_table.h"

#include "iupcocoatouch_drv.h"


#define IUPCOCOATOUCH_TABLE_HEADER_SECTION 0
#define IUPCOCOATOUCH_TABLE_BODY_SECTION   1
#define IUPCOCOATOUCH_TABLE_DEFAULT_COL_W  100
#define IUPCOCOATOUCH_TABLE_ROW_HEIGHT     32
#define IUPCOCOATOUCH_TABLE_HEADER_HEIGHT  32
#define IUPCOCOATOUCH_TABLE_CELL_PAD       8
#define IUPCOCOATOUCH_TABLE_IMAGE_SIZE     24

static const void* IUP_COCOATOUCH_TABLE_CTRL_OBJ_KEY = "IUP_COCOATOUCH_TABLE_CTRL_OBJ_KEY";


@interface IupCocoaTouchTableCell : UICollectionViewCell
@property(nonatomic, retain) UILabel* label;
@property(nonatomic, retain) UIImageView* imageView;
@property(nonatomic, retain) UIView* rightSep;
@property(nonatomic, retain) UIView* bottomSep;
@property(nonatomic, retain) NSLayoutConstraint* labelLeading;
@property(nonatomic, assign) BOOL rowSelected;
@property(nonatomic, assign) BOOL cellFocused;
@property(nonatomic, assign) BOOL focusRectEnabled;
- (void)applyState;
- (void)setBackgroundColorForState:(UIColor*)color;
- (void)setImage:(UIImage*)img;
- (void)setSeparatorsHidden:(BOOL)hidden;
@end

@implementation IupCocoaTouchTableCell

- (instancetype)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self)
	{
		_focusRectEnabled = YES;

		_imageView = [[UIImageView alloc] initWithFrame:CGRectZero];
		_imageView.translatesAutoresizingMaskIntoConstraints = NO;
		_imageView.contentMode = UIViewContentModeScaleAspectFit;
		_imageView.hidden = YES;
		[self.contentView addSubview:_imageView];
		[NSLayoutConstraint activateConstraints:@[
			[_imageView.leadingAnchor   constraintEqualToAnchor:self.contentView.leadingAnchor constant:IUPCOCOATOUCH_TABLE_CELL_PAD],
			[_imageView.centerYAnchor   constraintEqualToAnchor:self.contentView.centerYAnchor],
			[_imageView.widthAnchor     constraintEqualToConstant:IUPCOCOATOUCH_TABLE_IMAGE_SIZE],
			[_imageView.heightAnchor    constraintEqualToConstant:IUPCOCOATOUCH_TABLE_IMAGE_SIZE],
		]];

		_label = [[UILabel alloc] initWithFrame:CGRectZero];
		_label.translatesAutoresizingMaskIntoConstraints = NO;
		_label.lineBreakMode = NSLineBreakByTruncatingTail;
		_label.font = [UIFont systemFontOfSize:[UIFont systemFontSize]];
		[self.contentView addSubview:_label];
		_labelLeading = [[_label.leadingAnchor constraintEqualToAnchor:self.contentView.leadingAnchor constant:IUPCOCOATOUCH_TABLE_CELL_PAD] retain];
		[NSLayoutConstraint activateConstraints:@[
			[_label.topAnchor      constraintEqualToAnchor:self.contentView.topAnchor],
			[_label.bottomAnchor   constraintEqualToAnchor:self.contentView.bottomAnchor],
			_labelLeading,
			[_label.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-IUPCOCOATOUCH_TABLE_CELL_PAD],
		]];

		CGFloat hairline = 1.0/[UIScreen mainScreen].scale;
		_rightSep  = [[UIView alloc] initWithFrame:CGRectZero];
		_rightSep.translatesAutoresizingMaskIntoConstraints = NO;
		_rightSep.backgroundColor = [UIColor separatorColor];
		[self.contentView addSubview:_rightSep];
		[NSLayoutConstraint activateConstraints:@[
			[_rightSep.topAnchor      constraintEqualToAnchor:self.contentView.topAnchor],
			[_rightSep.bottomAnchor   constraintEqualToAnchor:self.contentView.bottomAnchor],
			[_rightSep.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor],
			[_rightSep.widthAnchor    constraintEqualToConstant:hairline],
		]];

		_bottomSep = [[UIView alloc] initWithFrame:CGRectZero];
		_bottomSep.translatesAutoresizingMaskIntoConstraints = NO;
		_bottomSep.backgroundColor = [UIColor separatorColor];
		[self.contentView addSubview:_bottomSep];
		[NSLayoutConstraint activateConstraints:@[
			[_bottomSep.leadingAnchor  constraintEqualToAnchor:self.contentView.leadingAnchor],
			[_bottomSep.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor],
			[_bottomSep.bottomAnchor   constraintEqualToAnchor:self.contentView.bottomAnchor],
			[_bottomSep.heightAnchor   constraintEqualToConstant:hairline],
		]];
	}
	return self;
}

- (void)setImage:(UIImage*)img
{
	_imageView.image = img;
	_imageView.hidden = (img == nil);
	_labelLeading.constant = img ? (IUPCOCOATOUCH_TABLE_CELL_PAD + IUPCOCOATOUCH_TABLE_IMAGE_SIZE + IUPCOCOATOUCH_TABLE_CELL_PAD) : IUPCOCOATOUCH_TABLE_CELL_PAD;
}

- (void)setSeparatorsHidden:(BOOL)hidden
{
	_rightSep.hidden = hidden;
	_bottomSep.hidden = hidden;
}

- (void)setBackgroundColorForState:(UIColor*)color
{
	if (_rowSelected)
		self.contentView.backgroundColor = [[UIColor systemBlueColor] colorWithAlphaComponent:0.15];
	else
		self.contentView.backgroundColor = color ?: [UIColor systemBackgroundColor];
}

- (void)applyState
{
	if (_cellFocused && _focusRectEnabled)
	{
		self.contentView.layer.borderColor = [[UIColor systemBlueColor] CGColor];
		self.contentView.layer.borderWidth = 2.0;
	}
	else
	{
		self.contentView.layer.borderColor = NULL;
		self.contentView.layer.borderWidth = 0;
	}
}

- (void)prepareForReuse
{
	[super prepareForReuse];
	_rowSelected = NO;
	_cellFocused = NO;
	[self setImage:nil];
	[self setSeparatorsHidden:NO];
	[self applyState];
}

- (void)dealloc
{
	[_label release];
	[_imageView release];
	[_rightSep release];
	[_bottomSep release];
	[_labelLeading release];
	[super dealloc];
}

@end


@interface IupCocoaTouchTableController : NSObject <UICollectionViewDataSource, UICollectionViewDelegate>
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, retain) NSMutableArray<NSMutableArray<NSString*>*>* cells;
@property(nonatomic, retain) NSMutableArray<NSMutableArray<NSString*>*>* images;
@property(nonatomic, retain) NSMutableArray<NSString*>* headers;
@property(nonatomic, retain) NSMutableArray<NSNumber*>* colWidths;
@property(nonatomic, assign) NSInteger focusLin;
@property(nonatomic, assign) NSInteger focusCol;
@property(nonatomic, assign) BOOL showGrid;
@property(nonatomic, assign) CGFloat lastKnownContainerWidth;
@property(nonatomic, assign) NSInteger sortCol;
@property(nonatomic, assign) BOOL sortAscending;

- (NSInteger)numberOfLines;
- (NSInteger)numberOfColumns;
- (CGFloat)widthForCol:(NSInteger)col container:(CGFloat)container_w;
- (CGFloat)totalWidthFor:(CGFloat)container_w;
- (CGFloat)naturalWidthForCol:(NSInteger)col;
- (NSString*)cellAtLin:(NSInteger)lin col:(NSInteger)col;
- (NSString*)imageAtLin:(NSInteger)lin col:(NSInteger)col;
- (void)setCell:(NSString*)value atLin:(NSInteger)lin col:(NSInteger)col;
- (void)setImage:(NSString*)name atLin:(NSInteger)lin col:(NSInteger)col;
- (void)resizeToLines:(NSInteger)num_lin cols:(NSInteger)num_col;
@end

static UICollectionViewLayout* cocoaTouchTableMakeLayout(IupCocoaTouchTableController* ctrl)
{
	IupCocoaTouchTableController* ctrl_ref = ctrl;
	UICollectionViewCompositionalLayoutSectionProvider provider =
		^NSCollectionLayoutSection*(NSInteger sectionIndex,
		                            id<NSCollectionLayoutEnvironment> env)
		{
			NSInteger num_col = [ctrl_ref numberOfColumns];
			if (num_col <= 0) return nil;
			CGFloat row_h = sectionIndex == IUPCOCOATOUCH_TABLE_HEADER_SECTION
				? IUPCOCOATOUCH_TABLE_HEADER_HEIGHT : IUPCOCOATOUCH_TABLE_ROW_HEIGHT;
			CGFloat container_w = env.container.contentSize.width;
			ctrl_ref.lastKnownContainerWidth = container_w;

			NSMutableArray<NSCollectionLayoutItem*>* items = [NSMutableArray arrayWithCapacity:(NSUInteger)num_col];
			for (NSInteger c = 0; c < num_col; c++)
			{
				CGFloat w = [ctrl_ref widthForCol:c container:container_w];
				NSCollectionLayoutSize* sz = [NSCollectionLayoutSize
					sizeWithWidthDimension:[NSCollectionLayoutDimension absoluteDimension:w]
					      heightDimension:[NSCollectionLayoutDimension absoluteDimension:row_h]];
				[items addObject:[NSCollectionLayoutItem itemWithLayoutSize:sz]];
			}

			NSCollectionLayoutSize* groupSize = [NSCollectionLayoutSize
				sizeWithWidthDimension:[NSCollectionLayoutDimension absoluteDimension:[ctrl_ref totalWidthFor:container_w]]
				      heightDimension:[NSCollectionLayoutDimension absoluteDimension:row_h]];
			NSCollectionLayoutGroup* group = [NSCollectionLayoutGroup
				horizontalGroupWithLayoutSize:groupSize subitems:items];
			NSCollectionLayoutSection* section = [NSCollectionLayoutSection sectionWithGroup:group];
			return section;
		};

	UICollectionViewCompositionalLayoutConfiguration* cfg = [[[UICollectionViewCompositionalLayoutConfiguration alloc] init] autorelease];
	cfg.scrollDirection = UICollectionViewScrollDirectionVertical;
	return [[[UICollectionViewCompositionalLayout alloc] initWithSectionProvider:provider configuration:cfg] autorelease];
}

@implementation IupCocoaTouchTableController

- (instancetype)init
{
	self = [super init];
	if (self)
	{
		_cells     = [[NSMutableArray alloc] init];
		_images    = [[NSMutableArray alloc] init];
		_headers   = [[NSMutableArray alloc] init];
		_colWidths = [[NSMutableArray alloc] init];
		_focusLin = 1;
		_focusCol = 1;
		_showGrid = YES;
		_sortCol = 0;
		_sortAscending = YES;
	}
	return self;
}

- (void)dealloc
{
	[_cells release];
	[_images release];
	[_headers release];
	[_colWidths release];
	[super dealloc];
}

- (NSInteger)numberOfLines   { return (NSInteger)[_cells count]; }
- (NSInteger)numberOfColumns
{
	NSInteger n = (NSInteger)[_headers count];
	for (NSMutableArray* row in _cells)
		if ((NSInteger)[row count] > n) n = (NSInteger)[row count];
	return n;
}

- (CGFloat)naturalWidthForCol:(NSInteger)col
{
	UIFont* font = [UIFont systemFontOfSize:[UIFont systemFontSize]];
	UIFont* hfont = [UIFont boldSystemFontOfSize:[UIFont systemFontSize]];
	CGFloat max_w = 0;
	if ((NSUInteger)col < [_headers count])
	{
		NSString* header = [[_headers objectAtIndex:(NSUInteger)col] stringByAppendingString:@"  ▲"];
		CGSize hs = [header sizeWithAttributes:@{NSFontAttributeName: hfont}];
		if (hs.width > max_w) max_w = hs.width;
	}
	for (NSMutableArray* row in _cells)
	{
		if ((NSUInteger)col >= [row count]) continue;
		CGSize cs = [[row objectAtIndex:(NSUInteger)col] sizeWithAttributes:@{NSFontAttributeName: font}];
		if (cs.width > max_w) max_w = cs.width;
	}
	max_w = ceil(max_w) + 2 * IUPCOCOATOUCH_TABLE_CELL_PAD + 4;
	if (_ihandle && _ihandle->data && _ihandle->data->show_image)
	{
		BOOL has_image_in_col = NO;
		for (NSMutableArray* row in _images)
		{
			if ((NSUInteger)col < [row count] && [[row objectAtIndex:(NSUInteger)col] length] > 0)
			{ has_image_in_col = YES; break; }
		}
		if (has_image_in_col) max_w += IUPCOCOATOUCH_TABLE_IMAGE_SIZE + IUPCOCOATOUCH_TABLE_CELL_PAD;
	}
	if (max_w < 40) max_w = 40;
	return max_w;
}

- (CGFloat)widthForCol:(NSInteger)col container:(CGFloat)container_w
{
	NSInteger n = [self numberOfColumns];
	if (col < 0 || col >= n) return IUPCOCOATOUCH_TABLE_DEFAULT_COL_W;

	int explicit_w = 0;
	if ((NSUInteger)col < [_colWidths count])
		explicit_w = [[_colWidths objectAtIndex:(NSUInteger)col] intValue];

	BOOL stretch_last = _ihandle && _ihandle->data && _ihandle->data->stretch_last;
	BOOL is_last = (col == n - 1);

	CGFloat w = explicit_w > 0 ? (CGFloat)explicit_w : [self naturalWidthForCol:col];

	if (is_last && stretch_last && container_w > 0 && explicit_w == 0)
	{
		CGFloat sum_others = 0;
		for (NSInteger c = 0; c < n - 1; c++)
		{
			int ew = (NSUInteger)c < [_colWidths count] ? [[_colWidths objectAtIndex:(NSUInteger)c] intValue] : 0;
			sum_others += ew > 0 ? (CGFloat)ew : [self naturalWidthForCol:c];
		}
		CGFloat remaining = container_w - sum_others;
		if (remaining > w) w = remaining;
	}
	return w;
}

- (CGFloat)totalWidthFor:(CGFloat)container_w
{
	CGFloat total = 0;
	for (NSInteger c = 0; c < [self numberOfColumns]; c++)
		total += [self widthForCol:c container:container_w];
	return total;
}

- (NSString*)cellAtLin:(NSInteger)lin col:(NSInteger)col
{
	if (_ihandle && iupAttribGetBoolean(_ihandle, "VIRTUALMODE"))
	{
		sIFnii cb = (sIFnii)IupGetCallback(_ihandle, "VALUE_CB");
		if (cb)
		{
			char* v = cb(_ihandle, (int)lin, (int)col);
			return v ? [NSString stringWithUTF8String:v] : @"";
		}
		return @"";
	}
	if (lin < 1 || (NSUInteger)(lin - 1) >= [_cells count]) return @"";
	NSMutableArray* row = [_cells objectAtIndex:(NSUInteger)(lin - 1)];
	if (col < 1 || (NSUInteger)(col - 1) >= [row count]) return @"";
	return [row objectAtIndex:(NSUInteger)(col - 1)];
}

- (NSString*)imageAtLin:(NSInteger)lin col:(NSInteger)col
{
	if (_ihandle && iupAttribGetBoolean(_ihandle, "VIRTUALMODE"))
	{
		sIFnii cb = (sIFnii)IupGetCallback(_ihandle, "IMAGE_CB");
		if (cb)
		{
			char* v = cb(_ihandle, (int)lin, (int)col);
			return v ? [NSString stringWithUTF8String:v] : nil;
		}
		return nil;
	}
	if (lin < 1 || (NSUInteger)(lin - 1) >= [_images count]) return nil;
	NSMutableArray* row = [_images objectAtIndex:(NSUInteger)(lin - 1)];
	if (col < 1 || (NSUInteger)(col - 1) >= [row count]) return nil;
	NSString* name = [row objectAtIndex:(NSUInteger)(col - 1)];
	return [name length] > 0 ? name : nil;
}

- (void)setCell:(NSString*)value atLin:(NSInteger)lin col:(NSInteger)col
{
	if (lin < 1 || col < 1) return;
	while ((NSInteger)[_cells count] < lin)
		[_cells addObject:[NSMutableArray array]];
	NSMutableArray* row = [_cells objectAtIndex:(NSUInteger)(lin - 1)];
	while ((NSInteger)[row count] < col) [row addObject:@""];
	[row replaceObjectAtIndex:(NSUInteger)(col - 1) withObject:(value ?: @"")];
}

- (void)setImage:(NSString*)name atLin:(NSInteger)lin col:(NSInteger)col
{
	if (lin < 1 || col < 1) return;
	while ((NSInteger)[_images count] < lin)
		[_images addObject:[NSMutableArray array]];
	NSMutableArray* row = [_images objectAtIndex:(NSUInteger)(lin - 1)];
	while ((NSInteger)[row count] < col) [row addObject:@""];
	[row replaceObjectAtIndex:(NSUInteger)(col - 1) withObject:(name ?: @"")];
}

- (void)swapColumn:(NSInteger)a with:(NSInteger)b
{
	void (^swapInArray)(NSMutableArray*, NSInteger, NSInteger) = ^(NSMutableArray* arr, NSInteger i, NSInteger j) {
		if (!arr || i < 0 || j < 0 || i >= (NSInteger)arr.count || j >= (NSInteger)arr.count) return;
		[arr exchangeObjectAtIndex:(NSUInteger)i withObjectAtIndex:(NSUInteger)j];
	};
	swapInArray(_headers, a, b);
	swapInArray(_colWidths, a, b);
	for (NSMutableArray* row in _cells)  swapInArray(row, a, b);
	for (NSMutableArray* row in _images) swapInArray(row, a, b);
}

- (void)resizeToLines:(NSInteger)num_lin cols:(NSInteger)num_col
{
	while ((NSInteger)[_cells count] > num_lin) [_cells removeLastObject];
	while ((NSInteger)[_cells count] < num_lin)
	{
		NSMutableArray* row = [NSMutableArray arrayWithCapacity:(NSUInteger)num_col];
		for (NSInteger c = 0; c < num_col; c++) [row addObject:@""];
		[_cells addObject:row];
	}
	for (NSMutableArray* row in _cells)
	{
		while ((NSInteger)[row count] > num_col) [row removeLastObject];
		while ((NSInteger)[row count] < num_col) [row addObject:@""];
	}
	while ((NSInteger)[_images count] > num_lin) [_images removeLastObject];
	while ((NSInteger)[_images count] < num_lin)
	{
		NSMutableArray* row = [NSMutableArray arrayWithCapacity:(NSUInteger)num_col];
		for (NSInteger c = 0; c < num_col; c++) [row addObject:@""];
		[_images addObject:row];
	}
	for (NSMutableArray* row in _images)
	{
		while ((NSInteger)[row count] > num_col) [row removeLastObject];
		while ((NSInteger)[row count] < num_col) [row addObject:@""];
	}
	while ((NSInteger)[_headers count] > num_col) [_headers removeLastObject];
	while ((NSInteger)[_headers count] < num_col) [_headers addObject:@""];
	while ((NSInteger)[_colWidths count] > num_col) [_colWidths removeLastObject];
	while ((NSInteger)[_colWidths count] < num_col) [_colWidths addObject:@0];
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView*)cv
{
	(void)cv;
	return 2;
}

- (NSInteger)collectionView:(UICollectionView*)cv numberOfItemsInSection:(NSInteger)section
{
	(void)cv;
	NSInteger n = [self numberOfColumns];
	if (n <= 0) return 0;
	return section == IUPCOCOATOUCH_TABLE_HEADER_SECTION ? n : n * [self numberOfLines];
}

- (UICollectionViewCell*)collectionView:(UICollectionView*)cv cellForItemAtIndexPath:(NSIndexPath*)ip
{
	IupCocoaTouchTableCell* cell = [cv dequeueReusableCellWithReuseIdentifier:@"iupcell" forIndexPath:ip];
	NSInteger num_col = [self numberOfColumns];
	if (num_col <= 0) { cell.label.text = @""; return cell; }

	NSInteger col = [ip item] % num_col;
	BOOL focus_rect = _ihandle ? iupAttribGetBoolean(_ihandle, "FOCUSRECT") : YES;
	cell.focusRectEnabled = focus_rect;
	[cell setSeparatorsHidden:!_showGrid];

	if ([ip section] == IUPCOCOATOUCH_TABLE_HEADER_SECTION)
	{
		NSString* title = (NSUInteger)col < [_headers count] ? [_headers objectAtIndex:(NSUInteger)col] : @"";
		BOOL sortable = _ihandle && _ihandle->data && _ihandle->data->sortable;
		if (sortable && (col + 1) == _sortCol)
			title = [title stringByAppendingString:_sortAscending ? @"  ▲" : @"  ▼"];
		cell.label.text = title;
		cell.label.font = [UIFont boldSystemFontOfSize:[UIFont systemFontSize]];
		cell.label.textColor = [UIColor secondaryLabelColor];
		const char* halign = _ihandle ? iupAttribGetId(_ihandle, "ALIGNMENT", (int)col + 1) : NULL;
		if (halign && iupStrEqualNoCase(halign, "ARIGHT"))       cell.label.textAlignment = NSTextAlignmentRight;
		else if (halign && iupStrEqualNoCase(halign, "ACENTER")) cell.label.textAlignment = NSTextAlignmentCenter;
		else                                                     cell.label.textAlignment = NSTextAlignmentLeft;
		[cell setImage:nil];
		cell.userInteractionEnabled = sortable;
		cell.rowSelected = NO;
		cell.cellFocused = NO;
		[cell setBackgroundColorForState:[UIColor secondarySystemBackgroundColor]];
		[cell applyState];
	}
	else
	{
		NSInteger lin = [ip item] / num_col;
		cell.label.text = [self cellAtLin:lin + 1 col:col + 1];
		cell.label.font = [UIFont systemFontOfSize:[UIFont systemFontSize]];
		cell.label.textColor = [UIColor labelColor];
		cell.userInteractionEnabled = YES;
		cell.rowSelected = ((lin + 1) == _focusLin);
		cell.cellFocused = cell.rowSelected && ((col + 1) == _focusCol);

		const char* align = _ihandle ? iupAttribGetId(_ihandle, "ALIGNMENT", (int)col + 1) : NULL;
		if (align && iupStrEqualNoCase(align, "ARIGHT"))      cell.label.textAlignment = NSTextAlignmentRight;
		else if (align && iupStrEqualNoCase(align, "ACENTER")) cell.label.textAlignment = NSTextAlignmentCenter;
		else                                                   cell.label.textAlignment = NSTextAlignmentLeft;

		UIImage* img = nil;
		if (_ihandle && _ihandle->data && _ihandle->data->show_image)
		{
			NSString* name = [self imageAtLin:lin + 1 col:col + 1];
			if (name)
			{
				UIImage* native = (UIImage*)iupImageGetImage([name UTF8String], _ihandle, 0, NULL);
				if (native) img = native;
			}
		}
		[cell setImage:img];

		UIColor* bg = nil;
		UIColor* fg = nil;
		if (_ihandle)
		{
			const char* bg_str = iupAttribGetId2(_ihandle, "BGCOLOR", (int)lin + 1, (int)col + 1);
			if (!bg_str) bg_str = iupAttribGetId2(_ihandle, "BGCOLOR", (int)lin + 1, 0);
			if (!bg_str) bg_str = iupAttribGetId2(_ihandle, "BGCOLOR", 0, (int)col + 1);
			if (bg_str) bg = iupCocoaTouchToNativeColor(bg_str);
			else if (iupAttribGetBoolean(_ihandle, "ALTERNATECOLOR"))
			{
				BOOL even = ((lin + 1) % 2) == 0;
				const char* color_str = iupAttribGet(_ihandle, even ? "EVENROWCOLOR" : "ODDROWCOLOR");
				if (color_str) bg = iupCocoaTouchToNativeColor(color_str);
			}

			const char* fg_str = iupAttribGetId2(_ihandle, "FGCOLOR", (int)lin + 1, (int)col + 1);
			if (!fg_str) fg_str = iupAttribGetId2(_ihandle, "FGCOLOR", (int)lin + 1, 0);
			if (!fg_str) fg_str = iupAttribGetId2(_ihandle, "FGCOLOR", 0, (int)col + 1);
			if (fg_str) fg = iupCocoaTouchToNativeColor(fg_str);
		}
		if (fg) cell.label.textColor = fg;
		[cell setBackgroundColorForState:bg];
		[cell applyState];
	}
	return cell;
}

- (BOOL)isColEditable:(NSInteger)col
{
	if (!_ihandle) return NO;
	const char* per_col = iupAttribGetId(_ihandle, "EDITABLE", (int)col);
	if (per_col) return iupStrBoolean(per_col);
	const char* global = iupAttribGet(_ihandle, "EDITABLE");
	return global ? iupStrBoolean(global) : NO;
}

- (void)beginEditAtLin:(NSInteger)lin col:(NSInteger)col fromView:(UIView*)source
{
	IFnii begin_cb = (IFnii)IupGetCallback(_ihandle, "EDITBEGIN_CB");
	if (begin_cb && begin_cb(_ihandle, (int)lin, (int)col) == IUP_IGNORE) return;

	UIViewController* host = iupCocoaTouchFindTopPresentedViewController();
	if (!host) host = iupCocoaTouchFindCurrentRootViewController();
	if (!host) return;

	NSString* current = [self cellAtLin:lin col:col];
	UIAlertController* alert = [UIAlertController
	    alertControllerWithTitle:[NSString stringWithFormat:@"Edit cell %ld:%ld", (long)lin, (long)col]
	                     message:nil
	              preferredStyle:UIAlertControllerStyleAlert];
	[alert addTextFieldWithConfigurationHandler:^(UITextField* tf) {
		tf.text = current;
		tf.clearButtonMode = UITextFieldViewModeWhileEditing;
	}];
	IupCocoaTouchTableController* ctrl_ref = self;
	[alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction* a) {
		(void)a;
		IFniisi end_cb = (IFniisi)IupGetCallback(ctrl_ref.ihandle, "EDITEND_CB");
		if (end_cb) end_cb(ctrl_ref.ihandle, (int)lin, (int)col, (char*)"", 0);
	}]];
	[alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* a) {
		(void)a;
		NSString* new_text = alert.textFields.firstObject.text ?: @"";
		const char* new_c = [new_text UTF8String];
		IFniis edit_cb = (IFniis)IupGetCallback(ctrl_ref.ihandle, "EDITION_CB");
		if (edit_cb) edit_cb(ctrl_ref.ihandle, (int)lin, (int)col, (char*)new_c);
		IFniisi end_cb = (IFniisi)IupGetCallback(ctrl_ref.ihandle, "EDITEND_CB");
		int rc = IUP_DEFAULT;
		if (end_cb) rc = end_cb(ctrl_ref.ihandle, (int)lin, (int)col, (char*)new_c, 1);
		if (rc != IUP_IGNORE && ![current isEqualToString:new_text])
		{
			[ctrl_ref setCell:new_text atLin:lin col:col];
			iupAttribSetStrId2(ctrl_ref.ihandle, "", (int)lin, (int)col, new_c);
			id h = (id)ctrl_ref.ihandle->handle;
			if ([h isKindOfClass:[UICollectionView class]]) [(UICollectionView*)h reloadData];
			IFnii vc_cb = (IFnii)IupGetCallback(ctrl_ref.ihandle, "VALUECHANGED_CB");
			if (vc_cb && vc_cb(ctrl_ref.ihandle, (int)lin, (int)col) == IUP_CLOSE) IupExitLoop();
		}
	}]];
	if (alert.popoverPresentationController)
	{
		alert.popoverPresentationController.sourceView = source;
		alert.popoverPresentationController.sourceRect = source.bounds;
	}
	[host presentViewController:alert animated:YES completion:nil];
}

- (void)sortByCol:(NSInteger)col1 ascending:(BOOL)asc
{
	NSInteger ncols = [self numberOfColumns];
	NSInteger col0 = col1 - 1;
	if (col0 < 0 || col0 >= ncols) return;

	NSMutableArray<NSNumber*>* indices = [NSMutableArray arrayWithCapacity:[_cells count]];
	for (NSUInteger i = 0; i < [_cells count]; i++) [indices addObject:@(i)];
	[indices sortUsingComparator:^NSComparisonResult(NSNumber* a, NSNumber* b) {
		NSMutableArray* ra = [_cells objectAtIndex:[a unsignedIntegerValue]];
		NSMutableArray* rb = [_cells objectAtIndex:[b unsignedIntegerValue]];
		NSString* va = (NSUInteger)col0 < [ra count] ? [ra objectAtIndex:(NSUInteger)col0] : @"";
		NSString* vb = (NSUInteger)col0 < [rb count] ? [rb objectAtIndex:(NSUInteger)col0] : @"";
		double na, nb;
		BOOL na_num = [[NSScanner scannerWithString:va] scanDouble:&na] && [[NSScanner scannerWithString:va] isAtEnd];
		BOOL nb_num = [[NSScanner scannerWithString:vb] scanDouble:&nb] && [[NSScanner scannerWithString:vb] isAtEnd];
		NSComparisonResult r;
		if (na_num && nb_num) r = (na < nb) ? NSOrderedAscending : (na > nb ? NSOrderedDescending : NSOrderedSame);
		else r = [va compare:vb options:NSCaseInsensitiveSearch];
		return asc ? r : -r;
	}];

	NSMutableArray* new_cells  = [NSMutableArray arrayWithCapacity:[_cells count]];
	NSMutableArray* new_images = [NSMutableArray arrayWithCapacity:[_images count]];
	for (NSNumber* idx in indices)
	{
		[new_cells addObject:[_cells objectAtIndex:[idx unsignedIntegerValue]]];
		if ([idx unsignedIntegerValue] < [_images count])
			[new_images addObject:[_images objectAtIndex:[idx unsignedIntegerValue]]];
	}
	[_cells setArray:new_cells];
	[_images setArray:new_images];
}

- (void)collectionView:(UICollectionView*)cv didSelectItemAtIndexPath:(NSIndexPath*)ip
{
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	NSInteger num_col = [self numberOfColumns];
	if (num_col <= 0) return;

	if ([ip section] == IUPCOCOATOUCH_TABLE_HEADER_SECTION)
	{
		if (!_ihandle->data || !_ihandle->data->sortable) return;
		NSInteger col = ([ip item] % num_col) + 1;
		if (_sortCol == col) _sortAscending = !_sortAscending;
		else { _sortCol = col; _sortAscending = YES; }

		IFni cb = (IFni)IupGetCallback(_ihandle, "SORT_CB");
		int rc = IUP_DEFAULT;
		if (cb) rc = cb(_ihandle, (int)col);
		if (rc == IUP_DEFAULT) [self sortByCol:col ascending:_sortAscending];
		else if (rc == IUP_CLOSE) IupExitLoop();
		_focusLin = 0;
		_focusCol = 0;
		for (NSIndexPath* sel in [cv indexPathsForSelectedItems])
			[cv deselectItemAtIndexPath:sel animated:NO];
		[cv reloadData];
		return;
	}
	if ([ip section] != IUPCOCOATOUCH_TABLE_BODY_SECTION) return;

	NSInteger col = [ip item] % num_col;
	NSInteger lin = [ip item] / num_col;
	NSInteger prev_lin = _focusLin;
	_focusLin = lin + 1;
	_focusCol = col + 1;

	NSMutableArray<NSIndexPath*>* reload = [NSMutableArray array];
	for (NSInteger c = 0; c < num_col; c++)
	{
		[reload addObject:[NSIndexPath indexPathForItem:lin * num_col + c
		                                       inSection:IUPCOCOATOUCH_TABLE_BODY_SECTION]];
		if (prev_lin > 0 && prev_lin != _focusLin)
			[reload addObject:[NSIndexPath indexPathForItem:(prev_lin - 1) * num_col + c
			                                       inSection:IUPCOCOATOUCH_TABLE_BODY_SECTION]];
	}
	[cv reloadItemsAtIndexPaths:reload];

	IFniis click_cb = (IFniis)IupGetCallback(_ihandle, "CLICK_CB");
	if (click_cb)
	{
		char status[6] = "1    ";
		if (click_cb(_ihandle, (int)_focusLin, (int)_focusCol, status) == IUP_CLOSE) IupExitLoop();
	}
	IFnii enter_cb = (IFnii)IupGetCallback(_ihandle, "ENTERITEM_CB");
	if (enter_cb && enter_cb(_ihandle, (int)_focusLin, (int)_focusCol) == IUP_CLOSE) IupExitLoop();
}

- (void)onDoubleTap:(UITapGestureRecognizer*)gr
{
	if (!_ihandle || !iupObjectCheck(_ihandle)) return;
	if (gr.state != UIGestureRecognizerStateRecognized) return;
	UICollectionView* cv = (UICollectionView*)gr.view;
	if (![cv isKindOfClass:[UICollectionView class]]) return;
	CGPoint pt = [gr locationInView:cv];
	NSIndexPath* ip = [cv indexPathForItemAtPoint:pt];
	if (!ip || [ip section] != IUPCOCOATOUCH_TABLE_BODY_SECTION) return;
	NSInteger num_col = [self numberOfColumns];
	if (num_col <= 0) return;
	NSInteger col = [ip item] % num_col;
	NSInteger lin = [ip item] / num_col;
	if (![self isColEditable:col + 1]) return;
	UICollectionViewCell* cell = [cv cellForItemAtIndexPath:ip];
	[self beginEditAtLin:lin + 1 col:col + 1 fromView:(cell ?: cv)];
}

@end


static UICollectionView* cocoaTouchTableGet(Ihandle* ih)
{
	if (ih && [(id)ih->handle isKindOfClass:[UICollectionView class]])
		return (UICollectionView*)ih->handle;
	return nil;
}

static IupCocoaTouchTableController* cocoaTouchTableGetController(Ihandle* ih)
{
	UICollectionView* view = cocoaTouchTableGet(ih);
	return view ? objc_getAssociatedObject(view, IUP_COCOATOUCH_TABLE_CTRL_OBJ_KEY) : nil;
}

IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl) return;
	[ctrl resizeToLines:num_lin cols:[ctrl numberOfColumns]];
	ih->data->num_lin = num_lin;
	[cocoaTouchTableGet(ih) reloadData];
}

IUP_SDK_API void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl) return;
	[ctrl resizeToLines:[ctrl numberOfLines] cols:num_col];
	ih->data->num_col = num_col;
	[cocoaTouchTableGet(ih) reloadData];
}

IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl) return;
	NSUInteger index = (NSUInteger)(pos - 1);
	if (index > [ctrl.cells count]) index = [ctrl.cells count];
	NSMutableArray<NSString*>* row = [NSMutableArray arrayWithCapacity:(NSUInteger)[ctrl numberOfColumns]];
	for (NSInteger c = 0; c < [ctrl numberOfColumns]; c++) [row addObject:@""];
	[ctrl.cells insertObject:row atIndex:index];
	ih->data->num_lin = (int)[ctrl.cells count];
	[cocoaTouchTableGet(ih) reloadData];
}

IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl) return;
	NSUInteger index = (NSUInteger)(pos - 1);
	if (index >= [ctrl.cells count]) return;
	[ctrl.cells removeObjectAtIndex:index];
	ih->data->num_lin = (int)[ctrl.cells count];
	[cocoaTouchTableGet(ih) reloadData];
}

IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl) return;
	NSUInteger index = (NSUInteger)(pos - 1);

	for (NSMutableArray* row in ctrl.cells)
	{
		NSUInteger ins = index > [row count] ? [row count] : index;
		[row insertObject:@"" atIndex:ins];
	}
	if (index > [ctrl.headers count]) index = [ctrl.headers count];
	[ctrl.headers insertObject:@"" atIndex:index];
	[ctrl.colWidths insertObject:@0 atIndex:index];
	ih->data->num_col++;
	[cocoaTouchTableGet(ih) reloadData];
}

IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl) return;
	NSUInteger index = (NSUInteger)(pos - 1);
	for (NSMutableArray* row in ctrl.cells)
	{
		if (index < [row count]) [row removeObjectAtIndex:index];
	}
	if (index < [ctrl.headers count])   [ctrl.headers   removeObjectAtIndex:index];
	if (index < [ctrl.colWidths count]) [ctrl.colWidths removeObjectAtIndex:index];
	if (ih->data->num_col > 0) ih->data->num_col--;
	[cocoaTouchTableGet(ih) reloadData];
}

IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl) return;
	NSString* text = value ? [NSString stringWithUTF8String:value] : @"";
	[ctrl setCell:(text ?: @"") atLin:lin col:col];
	UICollectionView* view = cocoaTouchTableGet(ih);
	if (view)
	{
		[view setCollectionViewLayout:cocoaTouchTableMakeLayout(ctrl) animated:NO];
		[view reloadData];
	}
}

IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl) return NULL;
	NSString* text = [ctrl cellAtLin:lin col:col];
	return text ? iupStrReturnStr([text UTF8String]) : NULL;
}

IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl) return;
	[ctrl setImage:(image ? [NSString stringWithUTF8String:image] : nil) atLin:lin col:col];
	UICollectionView* view = cocoaTouchTableGet(ih);
	if (view)
	{
		[view setCollectionViewLayout:cocoaTouchTableMakeLayout(ctrl) animated:NO];
		[view reloadData];
	}
}

IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl || col < 1) return;
	NSUInteger index = (NSUInteger)(col - 1);
	while ([ctrl.headers count] <= index) [ctrl.headers addObject:@""];
	[ctrl.headers replaceObjectAtIndex:index withObject:(title ? [NSString stringWithUTF8String:title] : @"")];
	UICollectionView* view = cocoaTouchTableGet(ih);
	if (view)
	{
		[view setCollectionViewLayout:cocoaTouchTableMakeLayout(ctrl) animated:NO];
		[view reloadData];
	}
}

IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl || col < 1 || (NSUInteger)col > [ctrl.headers count]) return NULL;
	NSString* text = [ctrl.headers objectAtIndex:(NSUInteger)(col - 1)];
	return text ? iupStrReturnStr([text UTF8String]) : NULL;
}

IUP_SDK_API void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl || col < 1) return;
	NSUInteger index = (NSUInteger)(col - 1);
	while ([ctrl.colWidths count] <= index) [ctrl.colWidths addObject:@0];
	[ctrl.colWidths replaceObjectAtIndex:index withObject:[NSNumber numberWithInt:width]];

	UICollectionView* view = cocoaTouchTableGet(ih);
	if (view)
	{
		[view setCollectionViewLayout:cocoaTouchTableMakeLayout(ctrl) animated:NO];
		[view reloadData];
	}
}

IUP_SDK_API int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl || col < 1 || col > [ctrl numberOfColumns]) return 0;
	int explicit_w = (NSUInteger)(col - 1) < [ctrl.colWidths count] ? [[ctrl.colWidths objectAtIndex:(NSUInteger)(col - 1)] intValue] : 0;
	if (explicit_w > 0) return explicit_w;
	return (int)[ctrl widthForCol:(col - 1) container:ctrl.lastKnownContainerWidth];
}

IUP_SDK_API void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	UICollectionView* view = cocoaTouchTableGet(ih);
	if (!ctrl || !view) return;
	ctrl.focusLin = lin;
	ctrl.focusCol = col;
	NSInteger num_col = [ctrl numberOfColumns];
	if (lin >= 1 && lin <= [ctrl numberOfLines] && col >= 1 && col <= num_col)
	{
		NSIndexPath* ip = [NSIndexPath indexPathForItem:(lin - 1) * num_col + (col - 1)
		                                      inSection:IUPCOCOATOUCH_TABLE_BODY_SECTION];
		[view selectItemAtIndexPath:ip animated:NO scrollPosition:UICollectionViewScrollPositionNone];
	}
}

IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (lin) *lin = ctrl ? (int)ctrl.focusLin : 1;
	if (col) *col = ctrl ? (int)ctrl.focusCol : 1;
}

IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	UICollectionView* view = cocoaTouchTableGet(ih);
	if (!ctrl || !view || lin < 1 || lin > [ctrl numberOfLines]) return;
	NSInteger num_col = [ctrl numberOfColumns];
	if (col < 1 || col > num_col) col = 1;
	NSIndexPath* ip = [NSIndexPath indexPathForItem:(lin - 1) * num_col + (col - 1)
	                                      inSection:IUPCOCOATOUCH_TABLE_BODY_SECTION];
	[view scrollToItemAtIndexPath:ip
	             atScrollPosition:UICollectionViewScrollPositionCenteredVertically animated:NO];
}

IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih)
{
	[cocoaTouchTableGet(ih) reloadData];
}

IUP_SDK_API void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	UICollectionView* view = cocoaTouchTableGet(ih);
	if (!ctrl || !view) return;
	ctrl.showGrid = show ? YES : NO;
	[view reloadData];
}

IUP_SDK_API int iupdrvTableGetBorderWidth(Ihandle* ih)
{
	(void)ih;
	return 0;
}

IUP_SDK_API int iupdrvTableGetRowHeight(Ihandle* ih)
{
	(void)ih;
	return IUPCOCOATOUCH_TABLE_ROW_HEIGHT;
}

IUP_SDK_API int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
	(void)ih;
	return IUPCOCOATOUCH_TABLE_HEADER_HEIGHT;
}

IUP_SDK_API void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
	(void)ih; (void)w; (void)h;
}

static void cocoaTouchTableReplayStored(Ihandle* ih)
{
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (!ctrl) return;
	int nlin = ih->data->num_lin;
	int ncol = ih->data->num_col;

	for (int c = 1; c <= ncol; c++)
	{
		char* v = iupAttribGetId(ih, "TITLE", c);
		if (v) iupdrvTableSetColTitle(ih, c, v);

		v = iupAttribGetId(ih, "RASTERWIDTH", c);
		if (!v) v = iupAttribGetId(ih, "WIDTH", c);
		if (v)
		{
			int width;
			if (iupStrToInt(v, &width) && width > 0)
				iupdrvTableSetColWidth(ih, c, width);
		}
	}

	for (int l = 1; l <= nlin; l++)
	{
		for (int c = 1; c <= ncol; c++)
		{
			char* v = iupAttribGetId2(ih, "", l, c);
			if (v) [ctrl setCell:[NSString stringWithUTF8String:v] atLin:l col:c];
			if (ih->data->show_image)
			{
				char* img = iupAttribGetId2(ih, "IMAGE", l, c);
				if (img) [ctrl setImage:[NSString stringWithUTF8String:img] atLin:l col:c];
			}
		}
	}
}

static void cocoaTouchTableSwapAttrib(Ihandle* ih, const char* a, const char* b)
{
	char* va = iupAttribGet(ih, a);
	char* vb = iupAttribGet(ih, b);
	char* tmp = va ? iupStrDup(va) : NULL;
	iupAttribSetStr(ih, a, vb);
	iupAttribSetStr(ih, b, tmp);
	if (tmp) free(tmp);
}

static void cocoaTouchTableSwapColumnAttribs(Ihandle* ih, int col1, int col2)
{
	char a[64], b[64];
	int num_lin = ih->data->num_lin;

	snprintf(a, sizeof(a), "ALIGNMENT%d",  col1); snprintf(b, sizeof(b), "ALIGNMENT%d",  col2); cocoaTouchTableSwapAttrib(ih, a, b);
	snprintf(a, sizeof(a), "0:%d",          col1); snprintf(b, sizeof(b), "0:%d",          col2); cocoaTouchTableSwapAttrib(ih, a, b);
	snprintf(a, sizeof(a), "BGCOLOR0:%d",   col1); snprintf(b, sizeof(b), "BGCOLOR0:%d",   col2); cocoaTouchTableSwapAttrib(ih, a, b);
	snprintf(a, sizeof(a), "FGCOLOR0:%d",   col1); snprintf(b, sizeof(b), "FGCOLOR0:%d",   col2); cocoaTouchTableSwapAttrib(ih, a, b);
	snprintf(a, sizeof(a), "FONT0:%d",      col1); snprintf(b, sizeof(b), "FONT0:%d",      col2); cocoaTouchTableSwapAttrib(ih, a, b);
	snprintf(a, sizeof(a), "IMAGE0:%d",     col1); snprintf(b, sizeof(b), "IMAGE0:%d",     col2); cocoaTouchTableSwapAttrib(ih, a, b);
	snprintf(a, sizeof(a), "WIDTH%d",       col1); snprintf(b, sizeof(b), "WIDTH%d",       col2); cocoaTouchTableSwapAttrib(ih, a, b);
	snprintf(a, sizeof(a), "RASTERWIDTH%d", col1); snprintf(b, sizeof(b), "RASTERWIDTH%d", col2); cocoaTouchTableSwapAttrib(ih, a, b);

	for (int lin = 1; lin <= num_lin; lin++)
	{
		snprintf(a, sizeof(a), "%d:%d",        lin, col1); snprintf(b, sizeof(b), "%d:%d",        lin, col2); cocoaTouchTableSwapAttrib(ih, a, b);
		snprintf(a, sizeof(a), "BGCOLOR%d:%d", lin, col1); snprintf(b, sizeof(b), "BGCOLOR%d:%d", lin, col2); cocoaTouchTableSwapAttrib(ih, a, b);
		snprintf(a, sizeof(a), "FGCOLOR%d:%d", lin, col1); snprintf(b, sizeof(b), "FGCOLOR%d:%d", lin, col2); cocoaTouchTableSwapAttrib(ih, a, b);
		snprintf(a, sizeof(a), "FONT%d:%d",    lin, col1); snprintf(b, sizeof(b), "FONT%d:%d",    lin, col2); cocoaTouchTableSwapAttrib(ih, a, b);
		snprintf(a, sizeof(a), "IMAGE%d:%d",   lin, col1); snprintf(b, sizeof(b), "IMAGE%d:%d",   lin, col2); cocoaTouchTableSwapAttrib(ih, a, b);
	}
}

@interface IupCocoaTouchTableReorderGR : NSObject
@property(nonatomic, assign) Ihandle* ihandle;
@property(nonatomic, assign) UICollectionView* collectionView;
@property(nonatomic, assign) NSInteger sourceCol;
@end

@implementation IupCocoaTouchTableReorderGR

- (instancetype)init { self = [super init]; if (self) _sourceCol = -1; return self; }

- (NSInteger)columnAtPoint:(CGPoint)pt requireHeader:(BOOL)require_header
{
	UICollectionView* cv = self.collectionView;
	if (!cv) return -1;
	NSIndexPath* ip = [cv indexPathForItemAtPoint:pt];
	if (!ip) return -1;
	if (require_header && [ip section] != IUPCOCOATOUCH_TABLE_HEADER_SECTION) return -1;
	IupCocoaTouchTableController* ctrl = objc_getAssociatedObject(cv, IUP_COCOATOUCH_TABLE_CTRL_OBJ_KEY);
	if (!ctrl) return -1;
	NSInteger num_col = [ctrl numberOfColumns];
	if (num_col <= 0) return -1;
	NSInteger col = [ip item] % num_col;
	return col;
}

- (void)handleLongPress:(UILongPressGestureRecognizer*)gr
{
	if (!self.ihandle || !iupObjectCheck(self.ihandle)) return;

	if (gr.state == UIGestureRecognizerStateBegan)
	{
		_sourceCol = [self columnAtPoint:[gr locationInView:self.collectionView] requireHeader:YES];
	}
	else if (gr.state == UIGestureRecognizerStateEnded)
	{
		NSInteger target = [self columnAtPoint:[gr locationInView:self.collectionView] requireHeader:NO];
		if (_sourceCol >= 0 && target >= 0 && _sourceCol != target)
		{
			IFnii cb = (IFnii)IupGetCallback(self.ihandle, "REORDER_CB");
			if (!cb || cb(self.ihandle, (int)_sourceCol + 1, (int)target + 1) != IUP_IGNORE)
			{
				IupCocoaTouchTableController* ctrl = objc_getAssociatedObject(self.collectionView, IUP_COCOATOUCH_TABLE_CTRL_OBJ_KEY);
				[ctrl swapColumn:_sourceCol with:target];
				cocoaTouchTableSwapColumnAttribs(self.ihandle, (int)_sourceCol + 1, (int)target + 1);
				[self.collectionView reloadData];
			}
		}
		_sourceCol = -1;
	}
	else if (gr.state == UIGestureRecognizerStateCancelled || gr.state == UIGestureRecognizerStateFailed)
	{
		_sourceCol = -1;
	}
}

@end

static int cocoaTouchTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
	BOOL enable = iupStrBoolean(value);
	ih->data->allow_reorder = enable ? 1 : 0;

	UICollectionView* cv = cocoaTouchTableGet(ih);
	if (!cv) return 0;

	UILongPressGestureRecognizer* existing = objc_getAssociatedObject(cv, @"IUP_TABLE_REORDER_GR");
	if (enable)
	{
		if (existing) return 0;
		IupCocoaTouchTableReorderGR* h = [[[IupCocoaTouchTableReorderGR alloc] init] autorelease];
		h.ihandle = ih;
		h.collectionView = cv;
		UILongPressGestureRecognizer* gr = [[[UILongPressGestureRecognizer alloc]
			initWithTarget:h action:@selector(handleLongPress:)] autorelease];
		gr.minimumPressDuration = 0.4;
		[cv addGestureRecognizer:gr];
		objc_setAssociatedObject(cv, @"IUP_TABLE_REORDER_GR",      gr, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(cv, @"IUP_TABLE_REORDER_HANDLER", h,  OBJC_ASSOCIATION_RETAIN);
	}
	else if (existing)
	{
		[cv removeGestureRecognizer:existing];
		objc_setAssociatedObject(cv, @"IUP_TABLE_REORDER_GR",      nil, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(cv, @"IUP_TABLE_REORDER_HANDLER", nil, OBJC_ASSOCIATION_RETAIN);
	}
	return 0;
}

static int cocoaTouchTableMapMethod(Ihandle* ih)
{
	IupCocoaTouchTableController* ctrl = [[IupCocoaTouchTableController alloc] init];
	ctrl.ihandle = ih;

	UICollectionView* view = [[UICollectionView alloc] initWithFrame:CGRectZero
	                                            collectionViewLayout:cocoaTouchTableMakeLayout(ctrl)];
	view.backgroundColor = [UIColor systemBackgroundColor];
	view.dataSource = ctrl;
	view.delegate = ctrl;
	view.allowsSelection = YES;
	[view registerClass:[IupCocoaTouchTableCell class] forCellWithReuseIdentifier:@"iupcell"];

	UITapGestureRecognizer* dbl = [[UITapGestureRecognizer alloc] initWithTarget:ctrl action:@selector(onDoubleTap:)];
	dbl.numberOfTapsRequired = 2;
	[view addGestureRecognizer:dbl];
	[dbl release];

	ih->handle = view;
	objc_setAssociatedObject(view, IHANDLE_ASSOCIATED_OBJ_KEY,        (id)ih, OBJC_ASSOCIATION_ASSIGN);
	objc_setAssociatedObject(view, IUP_COCOATOUCH_TABLE_CTRL_OBJ_KEY, ctrl,   OBJC_ASSOCIATION_RETAIN);
	[ctrl release];

	iupCocoaTouchAddToParent(ih);

	if (ih->data->num_lin > 0 || ih->data->num_col > 0)
		[ctrl resizeToLines:ih->data->num_lin cols:ih->data->num_col];

	cocoaTouchTableReplayStored(ih);

	const char* show_grid = iupAttribGet(ih, "SHOWGRID");
	if (show_grid) ctrl.showGrid = iupStrBoolean(show_grid);

	if (ih->data->allow_reorder)
		cocoaTouchTableSetAllowReorderAttrib(ih, "YES");

	[view setCollectionViewLayout:cocoaTouchTableMakeLayout(ctrl) animated:NO];
	[view reloadData];

	return IUP_NOERROR;
}

static void cocoaTouchTableUnMapMethod(Ihandle* ih)
{
	if (ih && ih->handle)
	{
		UICollectionView* view = cocoaTouchTableGet(ih);
		if (view)
		{
			view.dataSource = nil;
			view.delegate = nil;
		}
		objc_setAssociatedObject((id)ih->handle, IHANDLE_ASSOCIATED_OBJ_KEY,        nil, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject((id)ih->handle, IUP_COCOATOUCH_TABLE_CTRL_OBJ_KEY, nil, OBJC_ASSOCIATION_ASSIGN);
	}
	iupdrvBaseUnMapMethod(ih);
}

static int cocoaTouchTableSetReloadAttrib(Ihandle* ih, const char* value)
{
	(void)value;
	UICollectionView* view = cocoaTouchTableGet(ih);
	if (view) [view reloadData];
	return 1;
}

static int cocoaTouchTableSetSortableAttrib(Ihandle* ih, const char* value)
{
	ih->data->sortable = iupStrBoolean(value) ? 1 : 0;
	UICollectionView* view = cocoaTouchTableGet(ih);
	IupCocoaTouchTableController* ctrl = cocoaTouchTableGetController(ih);
	if (view && ctrl)
	{
		NSInteger num_col = [ctrl numberOfColumns];
		NSMutableArray<NSIndexPath*>* paths = [NSMutableArray array];
		for (NSInteger c = 0; c < num_col; c++)
			[paths addObject:[NSIndexPath indexPathForItem:c inSection:IUPCOCOATOUCH_TABLE_HEADER_SECTION]];
		[view reloadItemsAtIndexPaths:paths];
	}
	return 0;
}

IUP_SDK_API void iupdrvTableInitClass(Iclass* ic)
{
	ic->Map   = cocoaTouchTableMapMethod;
	ic->UnMap = cocoaTouchTableUnMapMethod;

	iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "FGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_DEFAULT);
	iupClassRegisterAttribute(ic, "ALTERNATECOLOR", NULL, cocoaTouchTableSetReloadAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "EVENROWCOLOR",   NULL, cocoaTouchTableSetReloadAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "ODDROWCOLOR",    NULL, cocoaTouchTableSetReloadAttrib, NULL, NULL, IUPAF_NO_INHERIT);
	iupClassRegisterAttribute(ic, "FOCUSRECT",      NULL, cocoaTouchTableSetReloadAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
	iupClassRegisterReplaceAttribFunc(ic, "SORTABLE",     NULL, cocoaTouchTableSetSortableAttrib);
	iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, cocoaTouchTableSetAllowReorderAttrib);
}
