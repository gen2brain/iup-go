/** \file
 * \brief Table Control for Cocoa
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_table.h"

#include "iupcocoa_drv.h"


@class IupCocoaTableDataSource;
@class IupCocoaTableDelegate;
@class IupCocoaTableView;

static const void* IUP_COCOA_TABLE_DATASOURCE_KEY = "IUP_COCOA_TABLE_DATASOURCE_KEY";
static const void* IUP_COCOA_TABLE_DELEGATE_KEY = "IUP_COCOA_TABLE_DELEGATE_KEY";

static char kEditingRowKey;
static char kEditingColKey;
static char kEditEndedKey;
static char kEditBeginCalledKey;
static char kTargetHeightKey;
static char kVisibleColumnsKey;

/* ========================================================================= */
/* Data Structures                                                           */
/* ========================================================================= */

static NSString* const kIupTableRowPasteboardType = @"com.iup.table.row";

typedef struct _IcocoaTableColumnInfo {
  NSString* title;
  CGFloat width;
  NSTextAlignment alignment;
  BOOL sortable;
  BOOL resizable;
} IcocoaTableColumnInfo;

typedef struct _IcocoaTableData {
  NSMutableArray* data_array;
  NSMutableDictionary* column_info;
  NSMutableDictionary* cell_attributes;
  BOOL is_virtual_mode;                 /* Using VALUE_CB callback */
  BOOL editable;
  int sort_column;                      /* Currently sorted column (-1 = none) */
  BOOL sort_ascending;
  int current_row;                      /* Currently focused row (1-based, 0=none) */
  int current_col;                      /* Currently focused column (1-based, 0=none) */
  int previous_selected_row;            /* Previously selected row (0-based, -1=none) for redraw */
  int previous_focused_row;             /* Previously focused row (1-based, 0=none) for focus rectangle redraw */
  int previous_focused_col;             /* Previously focused column (1-based, 0=none) for focus rectangle redraw */
} IcocoaTableData;

#define ICOCOA_TABLE_DATA(ih) ((IcocoaTableData*)(ih->data->native_data))

/* ========================================================================= */
/* Custom NSTableCellView with Focus Rectangle                              */
/* ========================================================================= */

@interface IupCocoaTableCellView : NSTableCellView
{
  BOOL isFocusedCell;
  Ihandle* ih;
}
@property (nonatomic, assign) BOOL isFocusedCell;
@property (nonatomic, assign) Ihandle* ih;
@property (nonatomic, retain) NSColor* customBackgroundColor;
@end

@implementation IupCocoaTableCellView

@synthesize isFocusedCell;
@synthesize ih;
@synthesize customBackgroundColor;

- (void)dealloc
{
  [customBackgroundColor release];
  [super dealloc];
}

- (void)layout
{
  [super layout];

  CGFloat cellWidth = self.bounds.size.width;

  NSTableView* tableView = (NSTableView*)[self superview];
  if (tableView && [tableView isKindOfClass:[NSTableView class]])
  {
    NSInteger row = [tableView rowForView:self];
    NSInteger column = [tableView columnForView:self];
    if (row >= 0 && column >= 0)
    {
      NSRect cellFrame = [tableView frameOfCellAtColumn:column row:row];
      cellWidth = cellFrame.size.width;
    }
  }

  CGFloat cellHeight = self.bounds.size.height;
  CGFloat xStart = 2.0;

  if (self.imageView && ![self.imageView isHidden])
  {
    CGFloat imgSize = 16.0;
    CGFloat imgY = floor((cellHeight - imgSize) / 2.0);
    [self.imageView setFrame:NSMakeRect(xStart, imgY, imgSize, imgSize)];
    xStart += imgSize + 4.0;
  }

  if (self.textField)
  {
    CGFloat textHeight = [self.textField intrinsicContentSize].height;
#ifdef GNUSTEP
    /* GNUstep NSTextField inherits NSView's (-1,-1) intrinsicContentSize; fill the cell. */
    if (textHeight <= 0 || textHeight > cellHeight) textHeight = cellHeight;
#endif
    CGFloat yOffset = floor((cellHeight - textHeight) / 2.0);

    BOOL hasImage = (self.imageView && ![self.imageView isHidden]);
    NSTextAlignment alignment = [self.textField alignment];
    NSRect textFrame;

    if (!hasImage && alignment == NSTextAlignmentCenter)
    {
      CGFloat tw = cellWidth; if (tw < 0) tw = 0;
      textFrame = NSMakeRect(0.0, yOffset, tw, textHeight);
    }
    else
    {
      CGFloat tw = cellWidth - xStart - 4.0; if (tw < 0) tw = 0;
      textFrame = NSMakeRect(xStart, yOffset, tw, textHeight);
    }

    [self.textField setFrame:textFrame];
  }
}

#ifdef GNUSTEP
/* NSView -layout never fires without Auto Layout; drive it from setFrame. */
- (void) setFrame:(NSRect)frame
{
  frame = iupcocoaClampRect(frame);
  if ([[self superview] isKindOfClass:[NSTableRowView class]])
    frame.origin.y = 0;
  [super setFrame:frame];
  [self layout];
}
#endif

- (void)drawRect:(NSRect)dirtyRect
{
#ifdef GNUSTEP
  iupcocoaGnustepFillCellRect(self, dirtyRect, self.customBackgroundColor);
#else
  if (self.customBackgroundColor)
  {
    /* clipsToBounds is NO, so filling dirtyRect would paint over the neighbouring cells */
    [self.customBackgroundColor setFill];
    NSRectFill(NSIntersectionRect(dirtyRect, [self bounds]));
  }
#endif
  [super drawRect:dirtyRect];

  if (isFocusedCell && ih && iupAttribGetBoolean(ih, "FOCUSRECT"))
  {
    NSView* rowView = [self superview];
    NSTableView* tableView = nil;
    if (rowView)
      tableView = (NSTableView*)[rowView superview];

    if (tableView && [tableView isKindOfClass:[NSTableView class]])
    {
      NSWindow* window = [self window];
      NSResponder* firstResp = [window firstResponder];

      BOOL tableHasFocus = NO;
      if ([firstResp isEqual:tableView])
        tableHasFocus = YES;
      else if ([firstResp isKindOfClass:[NSView class]] && [(NSView*)firstResp isDescendantOf:tableView])
        tableHasFocus = YES;
      else if ([firstResp isKindOfClass:[NSText class]])
      {
        NSText* text = (NSText*)firstResp;
        id textDelegate = [text delegate];
        if (textDelegate && [textDelegate isKindOfClass:[NSView class]])
        {
          if ([(NSView*)textDelegate isDescendantOf:tableView])
            tableHasFocus = YES;
        }
      }

      if (tableHasFocus)
      {
        NSBezierPath* path = [NSBezierPath bezierPathWithRect:NSInsetRect(self.bounds, 1.5, 1.5)];

        CGFloat dashPattern[] = {2.0, 2.0};
        [path setLineDash:dashPattern count:2 phase:0.0];
        [path setLineWidth:1.0];

        [[NSColor labelColor] setStroke];
        [path stroke];
      }
    }
  }
}

@end

/* ========================================================================= */
/* Custom NSTableHeaderCell for proper sort indicator handling               */
/* ========================================================================= */

@interface IupCocoaTableHeaderCell : NSTableHeaderCell
@end

@implementation IupCocoaTableHeaderCell

- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView*)controlView
{
  BOOL is_sorted = NO;
  BOOL ascending = YES;

  if ([controlView isKindOfClass:[NSTableHeaderView class]])
  {
    NSTableView* tableView = [(NSTableHeaderView*)controlView tableView];
    NSArray* descriptors = [tableView sortDescriptors];
    if ([descriptors count] > 0)
    {
      NSSortDescriptor* descriptor = [descriptors objectAtIndex:0];
      NSString* sortKey = [descriptor key];

      NSArray* columns = [tableView tableColumns];
      for (NSTableColumn* col in columns)
      {
        if ([col headerCell] == self && [[col identifier] isEqualToString:sortKey])
        {
          is_sorted = YES;
          ascending = [descriptor ascending];
          break;
        }
      }
    }
  }

  CGFloat arrow_space = 0;
  if (is_sorted)
  {
    NSRect sortRect = [self sortIndicatorRectForBounds:cellFrame];
    arrow_space = sortRect.size.width;
  }

  NSRect titleRect = cellFrame;
  titleRect.origin.x += 4.0;
  titleRect.size.width -= 8.0 + arrow_space;
  if (titleRect.size.width < 0)
    titleRect.size.width = 0;

  NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
  [style setLineBreakMode:NSLineBreakByTruncatingTail];
  [style setAlignment:[self alignment]];

  NSDictionary* attrs = @{
    NSFontAttributeName: [self font],
    NSParagraphStyleAttributeName: style,
    NSForegroundColorAttributeName: [NSColor headerTextColor]
  };
  [style release];

  [[self stringValue] drawInRect:titleRect withAttributes:attrs];

  if (is_sorted)
    [self drawSortIndicatorWithFrame:cellFrame inView:controlView ascending:ascending priority:0];
}

@end

/* ========================================================================= */
/* Helper Functions                                                          */
/* ========================================================================= */

static NSTableView* cocoaTableGetTableView(Ihandle* ih)
{
  NSTableView* table_view = (NSTableView*)iupAttribGet(ih, "_IUPCOCOA_TABLEVIEW");
  return table_view;
}

static NSScrollView* cocoaTableGetScrollView(Ihandle* ih)
{
  return (NSScrollView*)ih->handle;
}

static IcocoaTableData* cocoaTableGetData(Ihandle* ih)
{
  return ICOCOA_TABLE_DATA(ih);
}

static void cocoaTableInitializeData(Ihandle* ih)
{
  IcocoaTableData* table_data = calloc(1, sizeof(IcocoaTableData));

  table_data->data_array = [[NSMutableArray alloc] init];
  table_data->column_info = [[NSMutableDictionary alloc] init];
  table_data->cell_attributes = [[NSMutableDictionary alloc] init];
  table_data->is_virtual_mode = NO;
  table_data->editable = NO;
  table_data->sort_column = -1;
  table_data->sort_ascending = YES;
  table_data->current_row = 0;
  table_data->current_col = 0;
  table_data->previous_selected_row = -1;
  table_data->previous_focused_row = 0;
  table_data->previous_focused_col = 0;

  ih->data->native_data = table_data;
}

static void cocoaTableFreeData(Ihandle* ih)
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (table_data)
  {
    [table_data->data_array release];
    [table_data->column_info release];
    [table_data->cell_attributes release];
    free(table_data);
    ih->data->native_data = NULL;
  }
}

static NSString* cocoaTableGetCellValue(Ihandle* ih, int lin, int col);

static BOOL cocoaTableIsCellEditable(Ihandle* ih, int col_1based)
{
  char name[50];
  snprintf(name, sizeof(name), "EDITABLE%d", col_1based);
  char* editable_str = iupAttribGet(ih, name);
  if (!editable_str)
    editable_str = iupAttribGet(ih, "EDITABLE");
  return iupStrBoolean(editable_str);
}

static void cocoaTableReloadFocusedCell(NSTableView* tableView, IcocoaTableData* table_data)
{
  if (table_data && table_data->current_row > 0 && table_data->current_col > 0)
  {
    NSInteger row_0based = table_data->current_row - 1;
    NSInteger col_0based = table_data->current_col - 1;

    if (row_0based >= 0 && row_0based < [tableView numberOfRows] &&
        col_0based >= 0 && col_0based < [tableView numberOfColumns])
    {
      NSIndexSet* rowIndexes = [NSIndexSet indexSetWithIndex:row_0based];
      NSIndexSet* colIndexes = [NSIndexSet indexSetWithIndex:col_0based];
      [tableView reloadDataForRowIndexes:rowIndexes columnIndexes:colIndexes];
    }
  }
}

static int cocoaTableValueChanged(const char* old_value, const char* new_value)
{
  if (!old_value && new_value && *new_value)
    return 1;
  if (old_value && !new_value)
    return 1;
  if (old_value && new_value && strcmp(old_value, new_value) != 0)
    return 1;
  return 0;
}

static CGFloat cocoaTableCalculateColumnWidth(Ihandle* ih, int col_index, NSFont* font)
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  CGFloat max_width = 0.0;
  int max_rows_to_check = (ih->data->num_lin > 100) ? 100 : ih->data->num_lin;

  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (tableView)
  {
    NSArray* columns = [tableView tableColumns];
    if (col_index < [columns count])
    {
      NSTableColumn* column = [columns objectAtIndex:col_index];
      NSString* title = [column.headerCell stringValue];
      if (title && [title length] > 0)
      {
        NSDictionary* attrs = @{NSFontAttributeName: font};
        NSSize title_size = [title sizeWithAttributes:attrs];
        CGFloat title_width = title_size.width + 8.0;
        if (ih->data->sortable)
          title_width += 12.0;
        if (title_width > max_width)
          max_width = title_width;
      }
    }
  }

  CGFloat image_extra = 0.0;
  if (ih->data->show_image)
    image_extra = 16.0 + 4.0;

  for (int lin = 0; lin < max_rows_to_check; lin++)
  {
    NSString* cell_value = cocoaTableGetCellValue(ih, lin, col_index);
    if (cell_value && [cell_value length] > 0)
    {
      NSDictionary* attrs = @{NSFontAttributeName: font};
      NSSize cell_size = [cell_value sizeWithAttributes:attrs];
      CGFloat cell_width = cell_size.width + 16.0 + image_extra;
      if (cell_width > max_width)
      {
        max_width = cell_width;
      }
    }
  }

  return max_width;
}

static NSString* cocoaTableGetCellValue(Ihandle* ih, int lin, int col)
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);

  if (table_data->is_virtual_mode)
  {
    sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
    if (value_cb)
    {
      char* value = value_cb(ih, lin + 1, col + 1);
      if (value)
        return [NSString stringWithUTF8String:value];
      return @"";
    }
  }

  if (lin >= 0 && lin < [table_data->data_array count])
  {
    NSMutableArray* row = [table_data->data_array objectAtIndex:lin];
    if (col >= 0 && col < [row count])
    {
      id obj = [row objectAtIndex:col];
      if ([obj isKindOfClass:[NSString class]])
        return (NSString*)obj;
    }
  }

  return @"";
}

static void cocoaTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);

  while (lin >= [table_data->data_array count])
  {
    NSMutableArray* row = [[NSMutableArray alloc] init];
    [table_data->data_array addObject:row];
    [row release];
  }

  NSMutableArray* row = [table_data->data_array objectAtIndex:lin];

  while (col >= [row count])
  {
    [row addObject:@""];
  }

  NSString* str = value ? [NSString stringWithUTF8String:value] : @"";
  [row replaceObjectAtIndex:col withObject:str];
}

static NSColor* cocoaTableGetRowBackgroundColor(Ihandle* ih, int row)
{
  /* row is 0-based NSTableView row index, lin is 1-based IUP index */
  int lin = row + 1;

  char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);

  if (!bgcolor)
  {
    char* alternate = iupAttribGet(ih, "ALTERNATECOLOR");
    if (iupStrBoolean(alternate))
    {
      char* even_color = iupAttribGet(ih, "EVENROWCOLOR");
      char* odd_color = iupAttribGet(ih, "ODDROWCOLOR");

      if (lin % 2 == 0)
        bgcolor = even_color;
      else
        bgcolor = odd_color;

    }
  }

  if (bgcolor && *bgcolor)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(bgcolor, &r, &g, &b))
    {
      return [NSColor colorWithSRGBRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
    }
  }

  return nil;
}

static void cocoaTableApplyCellColors(Ihandle* ih, NSTableCellView* cellView, int lin, int col, BOOL isSelected)
{
  /* lin and col are 1-based IUP indices */
  NSTextField* textField = cellView.textField;

  IupCocoaTableCellView* iupCellView = (IupCocoaTableCellView*)cellView;
  if (!isSelected)
  {
    /* Background color - hierarchy: L:C > :C > L:0 */
    char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);
    if (!bgcolor)
      bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);

    if (bgcolor && *bgcolor)
    {
      unsigned char r, g, b;
      if (iupStrToRGB(bgcolor, &r, &g, &b))
      {
        iupCellView.customBackgroundColor = [NSColor colorWithSRGBRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
      }
    }
    else
    {
      iupCellView.customBackgroundColor = nil;
    }
  }
  else
  {
    iupCellView.customBackgroundColor = nil;
  }
  [iupCellView setNeedsDisplay:YES];

  /* Foreground color - hierarchy: L:C > :C > L:0 */
  char* fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);

  if (fgcolor && *fgcolor)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(fgcolor, &r, &g, &b))
    {
      NSColor* color = [NSColor colorWithSRGBRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
      [textField setTextColor:color];
    }
  }
  else
  {
    [textField setTextColor:[NSColor controlTextColor]];
  }
}

/* Apply cell font - hierarchy: L:C > 0:C > L:0 */
static void cocoaTableApplyCellFont(Ihandle* ih, NSTextField* textField, int lin, int col)
{
  char* font = iupAttribGetId2(ih, "FONT", lin, col);
  if (!font)
    font = iupAttribGetId2(ih, "FONT", 0, col);
  if (!font)
    font = iupAttribGetId2(ih, "FONT", lin, 0);

  if (font && *font)
  {
    IupCocoaFont* iup_font = iupcocoaFindFont(font);
    if (iup_font)
    {
      NSFont* ns_font = [iup_font nativeFont];
      if (ns_font)
        [textField setFont:ns_font];
    }
  }
  else
  {
    char* default_font = iupAttribGetStr(ih, "FONT");
    if (default_font && *default_font)
    {
      IupCocoaFont* iup_font = iupcocoaFindFont(default_font);
      if (iup_font)
      {
        NSFont* ns_font = [iup_font nativeFont];
        if (ns_font)
          [textField setFont:ns_font];
      }
    }
    else
    {
      [textField setFont:[NSFont systemFontOfSize:[NSFont systemFontSize]]];
    }
  }
}

/* ========================================================================= */
/* Custom Table Header View                                                  */
/* ========================================================================= */

@interface IupNarrowTableHeaderView : NSTableHeaderView
@end

@implementation IupNarrowTableHeaderView

- (NSRect)headerRectOfColumn:(NSInteger)column
{
  NSRect rect = [super headerRectOfColumn:column];
  NSTableView* tableView = [self tableView];

  if (!tableView)
    return rect;

  NSInteger numColumns = [tableView numberOfColumns];

  if (column == numColumns - 1)
  {
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(tableView, "_IUPCOCOA_IHANDLE");
    if (ih)
    {
      const char* last_col_width_set = iupAttribGet(ih, "_IUP_TABLE_LAST_COL_WIDTH_SET");
      if ((last_col_width_set && iupStrBoolean(last_col_width_set)) || !ih->data->stretch_last)
      {
        return rect;
      }
    }

    CGFloat headerWidth = NSWidth([self bounds]);
    CGFloat rectMaxX = NSMaxX(rect);

    if (rectMaxX < headerWidth)
    {
      rect.size.width += (headerWidth - rectMaxX);
    }
  }

  return rect;
}

@end

/* ========================================================================= */
/* Custom Table View                                                        */
/* ========================================================================= */

@interface IupCocoaTableView : NSTableView
{
  Ihandle* ih;
}
- (id)initWithIhandle:(Ihandle*)ihandle;
@end

@implementation IupCocoaTableView

- (id)initWithIhandle:(Ihandle*)ihandle
{
  self = [super initWithFrame:NSZeroRect];
  if (self)
  {
    ih = ihandle;
  }
  return self;
}

- (void)mouseDown:(NSEvent*)event
{
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  NSInteger clickedCol = [self columnAtPoint:point];
  NSInteger clickedRow = [self rowAtPoint:point];

  IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
  int old_focused_col = 0;
  int old_focused_row = 0;
  if (table_data)
  {
    old_focused_col = table_data->current_col;
    old_focused_row = table_data->current_row;
  }

  if (table_data)
  {
    table_data->current_col = (clickedCol >= 0) ? (int)(clickedCol + 1) : 1;
    table_data->current_row = (clickedRow >= 0) ? (int)(clickedRow + 1) : 0;
  }

  if (table_data && old_focused_row == table_data->current_row && old_focused_col != table_data->current_col)
  {
    if (old_focused_row > 0)
    {
      NSMutableIndexSet* colsToReload = [NSMutableIndexSet indexSet];
      if (old_focused_col > 0)
        [colsToReload addIndex:(old_focused_col - 1)];
      if (table_data->current_col > 0)
        [colsToReload addIndex:(table_data->current_col - 1)];

      if ([colsToReload count] > 0)
      {
        NSIndexSet* rowSet = [NSIndexSet indexSetWithIndex:(old_focused_row - 1)];
        [self reloadDataForRowIndexes:rowSet columnIndexes:colsToReload];
      }
    }

    IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enteritem_cb)
    {
      enteritem_cb(ih, table_data->current_row, table_data->current_col);
    }
  }

  [super mouseDown:event];
}

- (NSRect)frameOfCellAtColumn:(NSInteger)column row:(NSInteger)row
{
  NSRect rect = [super frameOfCellAtColumn:column row:row];

  NSInteger numColumns = [self numberOfColumns];

  if (column == numColumns - 1)
  {
    Ihandle* table_ih = (Ihandle*)objc_getAssociatedObject(self, "_IUPCOCOA_IHANDLE");
    if (table_ih)
    {
      const char* last_col_width_set = iupAttribGet(table_ih, "_IUP_TABLE_LAST_COL_WIDTH_SET");
      if ((last_col_width_set && iupStrBoolean(last_col_width_set)) || !table_ih->data->stretch_last)
      {
        return rect;
      }
    }

    NSScrollView* scrollView = [self enclosingScrollView];
    CGFloat tableWidth = scrollView ? NSWidth(scrollView.documentVisibleRect) : NSWidth([self visibleRect]);
    CGFloat rectMaxX = NSMaxX(rect);

    if (rectMaxX < tableWidth)
    {
      rect.size.width += (tableWidth - rectMaxX);
    }
  }

  return rect;
}

- (BOOL)performKeyEquivalent:(NSEvent*)event
{
  NSString* chars = [event charactersIgnoringModifiers];
  if ([chars length] == 0)
    return [super performKeyEquivalent:event];

  unichar ch = [chars characterAtIndex:0];
  NSUInteger modifiers = [event modifierFlags];

  if ((modifiers & NSEventModifierFlagCommand) && (ch == 'c' || ch == 'C'))
  {
    IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
    NSInteger selectedRow = [self selectedRow];
    int selectedColumn = (table_data && table_data->current_col > 0) ? table_data->current_col - 1 : 0;

    if (selectedRow >= 0 && selectedColumn >= 0)
    {
      NSString* value = cocoaTableGetCellValue(ih, (int)selectedRow, selectedColumn);
      NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
      [pasteboard clearContents];
      [pasteboard setString:value forType:NSPasteboardTypeString];
    }
    return YES;
  }

  if ((modifiers & NSEventModifierFlagCommand) && (ch == 'v' || ch == 'V'))
  {
    IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
    NSInteger selectedRow = [self selectedRow];
    int selectedColumn = (table_data && table_data->current_col > 0) ? table_data->current_col - 1 : 0;

    if (selectedRow >= 0 && selectedColumn >= 0)
    {
      int col_1based = selectedColumn + 1;
      if (cocoaTableIsCellEditable(ih, col_1based))
      {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        NSString* text = [pasteboard stringForType:NSPasteboardTypeString];
        if (text)
        {
          const char* new_value = [text UTF8String];

          char* old_value_ptr = iupdrvTableGetCellValue(ih, (int)selectedRow + 1, col_1based);
          char* old_value = old_value_ptr ? iupStrDup(old_value_ptr) : NULL;

          cocoaTableSetCellValue(ih, (int)selectedRow, selectedColumn, new_value);

          NSIndexSet* rowSet = [NSIndexSet indexSetWithIndex:selectedRow];
          NSIndexSet* colSet = [NSIndexSet indexSetWithIndex:selectedColumn];
          [self reloadDataForRowIndexes:rowSet columnIndexes:colSet];

          if (cocoaTableValueChanged(old_value, new_value))
          {
            IFnii cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
            if (cb)
              cb(ih, (int)selectedRow + 1, col_1based);
          }

          if (old_value)
            free(old_value);
        }
      }
    }
    return YES;
  }

  return [super performKeyEquivalent:event];
}

- (void)keyDown:(NSEvent*)event
{
  NSString* chars = [event charactersIgnoringModifiers];
  if ([chars length] == 0)
  {
    [super keyDown:event];
    return;
  }

  unichar ch = [chars characterAtIndex:0];
  IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);

  if (ch == NSTabCharacter || ch == NSBackTabCharacter)
  {
    int mac_key_code = [event keyCode];
    if (!iupcocoaKeyEvent(ih, event, mac_key_code, true))
      [super keyDown:event];
    return;
  }

  if (ch == NSCarriageReturnCharacter || ch == NSEnterCharacter)
  {
    IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
    NSInteger selectedRow = [self selectedRow];
    int col_1based = (table_data && table_data->current_col > 0) ? table_data->current_col : 1;
    NSInteger selectedColumn = col_1based - 1;

    if (selectedRow >= 0 && selectedColumn >= 0)
    {
      if (cocoaTableIsCellEditable(ih, col_1based))
      {
        NSTableColumn* column = [[self tableColumns] objectAtIndex:selectedColumn];
        if (column)
        {
          int lin = (int)selectedRow + 1;
          IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
          if (editbegin_cb)
          {
            int ret = editbegin_cb(ih, lin, col_1based);
            if (ret == IUP_IGNORE)
              return;
          }

          objc_setAssociatedObject(self, &kEditingRowKey, @(selectedRow), OBJC_ASSOCIATION_RETAIN);
          objc_setAssociatedObject(self, &kEditingColKey, @(selectedColumn), OBJC_ASSOCIATION_RETAIN);
          objc_setAssociatedObject(self, &kEditEndedKey, @(NO), OBJC_ASSOCIATION_RETAIN);
          objc_setAssociatedObject(self, &kEditBeginCalledKey, @(YES), OBJC_ASSOCIATION_RETAIN);

          [self editColumn:selectedColumn row:selectedRow withEvent:event select:YES];
        }
      }
    }
    return;
  }

  if (table_data)
  {
    int num_cols = ih->data->num_col;
    BOOL col_changed = NO;
    int old_focused_col = table_data->current_col;
    NSInteger currentRow = [self selectedRow];

    if (ch == NSLeftArrowFunctionKey)
    {
      if (table_data->current_col > 1)
      {
        table_data->current_col--;
        col_changed = YES;
      }
    }
    else if (ch == NSRightArrowFunctionKey)
    {
      if (table_data->current_col < num_cols)
      {
        table_data->current_col++;
        col_changed = YES;
      }
    }

    if (col_changed && currentRow >= 0)
    {
      NSMutableIndexSet* colsToReload = [NSMutableIndexSet indexSet];
      if (old_focused_col > 0)
        [colsToReload addIndex:(old_focused_col - 1)];
      if (table_data->current_col > 0)
        [colsToReload addIndex:(table_data->current_col - 1)];

      if ([colsToReload count] > 0)
      {
        NSIndexSet* rowSet = [NSIndexSet indexSetWithIndex:currentRow];
        [self reloadDataForRowIndexes:rowSet columnIndexes:colsToReload];
      }

      IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
      if (enteritem_cb)
      {
        enteritem_cb(ih, (int)currentRow + 1, table_data->current_col);
      }
      return;
    }
  }

  [super keyDown:event];
}

- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if (result)
  {
    cocoaTableReloadFocusedCell(self, ICOCOA_TABLE_DATA(ih));
    iupcocoaFocusIn(ih);
  }
  return result;
}

- (BOOL)resignFirstResponder
{
  cocoaTableReloadFocusedCell(self, ICOCOA_TABLE_DATA(ih));
  iupcocoaFocusOut(ih);
  return [super resignFirstResponder];
}

@end

/* ========================================================================= */
/* NSTableViewDataSource Protocol Implementation                             */
/* ========================================================================= */

@interface IupCocoaTableDataSource : NSObject <NSTableViewDataSource>
{
  Ihandle* ih;
}
- (id)initWithIhandle:(Ihandle*)ihandle;
@end

@implementation IupCocoaTableDataSource

- (id)initWithIhandle:(Ihandle*)ihandle
{
  self = [super init];
  if (self)
  {
    ih = ihandle;
  }
  return self;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView
{
  return ih->data->num_lin;
}

- (id)tableView:(NSTableView*)tableView
    objectValueForTableColumn:(NSTableColumn*)tableColumn
    row:(NSInteger)row
{
  NSInteger col = [[tableColumn identifier] integerValue];
  return cocoaTableGetCellValue(ih, (int)row, (int)col);
}

- (id<NSPasteboardWriting>)tableView:(NSTableView*)tableView pasteboardWriterForRow:(NSInteger)row
{
  if (!ih->data->show_dragdrop)
    return nil;
  NSPasteboardItem* item = [[[NSPasteboardItem alloc] init] autorelease];
  [item setString:[NSString stringWithFormat:@"%ld", (long)row] forType:kIupTableRowPasteboardType];
  return item;
}

- (NSDragOperation)tableView:(NSTableView*)tableView validateDrop:(id<NSDraggingInfo>)info
    proposedRow:(NSInteger)row proposedDropOperation:(NSTableViewDropOperation)dropOperation
{
  if (!ih->data->show_dragdrop)
    return NSDragOperationNone;
  if (dropOperation == NSTableViewDropOn)
    [tableView setDropRow:row dropOperation:NSTableViewDropAbove];
  return NSDragOperationMove;
}

- (BOOL)tableView:(NSTableView*)tableView acceptDrop:(id<NSDraggingInfo>)info
    row:(NSInteger)row dropOperation:(NSTableViewDropOperation)dropOperation
{
  NSString* str = [[info draggingPasteboard] stringForType:kIupTableRowPasteboardType];
  if (!str)
    return NO;

  int from = [str intValue];   /* 0-based source */
  int drop0 = (int)row;        /* 0-based insert-before */
  int is_ctrl = 0;
  if (iupTableCallDragDropCb(ih, from, drop0, &is_ctrl) != IUP_CONTINUE)
    return NO;

  int count = ih->data->num_lin;
  int to = (drop0 > from) ? drop0 - 1 : drop0;
  if (to >= count)
    to = count - 1;
  if (from == to || from < 0 || to < 0)
    return NO;

  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (table_data && table_data->data_array && !table_data->is_virtual_mode &&
      from < (int)[table_data->data_array count])
  {
    NSMutableArray* rowData = [[table_data->data_array objectAtIndex:from] retain];
    [table_data->data_array removeObjectAtIndex:from];
    [table_data->data_array insertObject:rowData atIndex:to];
    [rowData release];
  }

  iupTableMoveLinAttribs(ih, from + 1, to + 1);

  if (table_data && table_data->current_row > 0)
    table_data->current_row = to + 1;

  [tableView reloadData];
  [tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:to] byExtendingSelection:NO];

  return YES;
}

- (void)tableView:(NSTableView*)tableView sortDescriptorsDidChange:(NSArray*)oldDescriptors
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (!table_data || iupAttribGet(ih, "_IUPCOCOA_SORTBUSY"))
    return;

  NSArray* newDescriptors = [tableView sortDescriptors];
  if ([newDescriptors count] == 0)
    return;

  NSSortDescriptor* descriptor = [newDescriptors objectAtIndex:0];
  NSString* key = [descriptor key];
  int col_index = [key intValue];

  IFni sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
  if (sort_cb && sort_cb(ih, col_index + 1) == IUP_IGNORE)
  {
    /* NSTableView already moved its own indicator, so put it back */
    iupAttribSet(ih, "_IUPCOCOA_SORTBUSY", "1");
    [tableView setSortDescriptors:oldDescriptors];
    iupAttribSet(ih, "_IUPCOCOA_SORTBUSY", NULL);
    return;
  }

  if (table_data->is_virtual_mode)
    return;

  [table_data->data_array sortUsingComparator:^NSComparisonResult(NSMutableArray* row1, NSMutableArray* row2) {
    NSString* val1 = (col_index < [row1 count]) ? [row1 objectAtIndex:col_index] : @"";
    NSString* val2 = (col_index < [row2 count]) ? [row2 objectAtIndex:col_index] : @"";

    int cmp = iupStrCompare([val1 UTF8String], [val2 UTF8String], 0, 1);
    if (![descriptor ascending])
      cmp = -cmp;

    return cmp < 0 ? NSOrderedAscending : (cmp > 0 ? NSOrderedDescending : NSOrderedSame);
  }];

  [tableView reloadData];
}

@end

/* ========================================================================= */
/* Custom Table Row View for Background Colors                              */
/* ========================================================================= */

@interface IupCocoaTableRowView : NSTableRowView
{
  NSColor* customBackgroundColor;
}
- (void)setCustomBackgroundColor:(NSColor*)color;
@end

@implementation IupCocoaTableRowView

- (void)setCustomBackgroundColor:(NSColor*)color
{
  [customBackgroundColor release];
  customBackgroundColor = [color retain];
}

- (void)dealloc
{
  [customBackgroundColor release];
  [super dealloc];
}

- (void)setSelected:(BOOL)selected
{
  [super setSelected:selected];
  [self setNeedsDisplay:YES];
}

- (void)drawBackgroundInRect:(NSRect)dirtyRect
{
  if (customBackgroundColor && ![self isSelected])
  {
    [customBackgroundColor setFill];
    NSRectFill(dirtyRect);
  }
  else if (![self isSelected])
  {
    [super drawBackgroundInRect:dirtyRect];
  }
}

- (void)drawSelectionInRect:(NSRect)dirtyRect
{
  if (self.selectionHighlightStyle != NSTableViewSelectionHighlightStyleNone)
  {
    [[NSColor selectedContentBackgroundColor] setFill];
    NSRectFill(self.bounds);
  }
}

@end

/* ========================================================================= */
/* NSTableViewDelegate Protocol Implementation                               */
/* ========================================================================= */

@interface IupCocoaTableDelegate : NSObject <NSTableViewDelegate, NSTextFieldDelegate>
{
  Ihandle* ih;
}
- (id)initWithIhandle:(Ihandle*)ihandle;
@end

@implementation IupCocoaTableDelegate

- (id)initWithIhandle:(Ihandle*)ihandle
{
  self = [super init];
  if (self)
  {
    ih = ihandle;

    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(controlTextDidBeginEditing:)
                   name:NSControlTextDidBeginEditingNotification
                 object:nil];
    [center addObserver:self
               selector:@selector(controlTextDidEndEditing:)
                   name:NSControlTextDidEndEditingNotification
                 object:nil];
  }
  return self;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (NSView*)tableView:(NSTableView*)tableView
    viewForTableColumn:(NSTableColumn*)tableColumn
    row:(NSInteger)row
{
  NSString* identifier = [tableColumn identifier];
  IupCocoaTableCellView* cellView = (IupCocoaTableCellView*)[tableView makeViewWithIdentifier:identifier owner:self];

  if (!cellView)
  {
    cellView = [[IupCocoaTableCellView alloc] initWithFrame:NSZeroRect];
    [cellView setIdentifier:identifier];
    [cellView setIh:ih];

    NSImageView* imageView = [[NSImageView alloc] initWithFrame:NSZeroRect];
    [imageView setImageFrameStyle:NSImageFrameNone];
    [imageView setImageAlignment:NSImageAlignCenter];
    [imageView setImageScaling:NSImageScaleProportionallyDown];
    [imageView setEditable:NO];
    [imageView setHidden:YES];
    [cellView setImageView:imageView];
    [cellView addSubview:imageView];
    [imageView release];

    NSTextField* textField = [[NSTextField alloc] initWithFrame:NSZeroRect];
    [textField setBordered:NO];
    [textField setDrawsBackground:NO];
    [textField setBezeled:NO];
    [[textField cell] setLineBreakMode:NSLineBreakByTruncatingTail];
    [textField setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
#ifdef GNUSTEP
    /* a selectable NSTextField swallows clicks on GNUstep and the table never sees them */
    [textField setSelectable:NO];
#endif

    [cellView setTextField:textField];
    [cellView addSubview:textField];
    [textField release];

    [cellView autorelease];
  }

  NSInteger col = [identifier integerValue];
  NSString* value = cocoaTableGetCellValue(ih, (int)row, (int)col);
  cellView.textField.stringValue = value;

  int col_1based = (int)col + 1;
  int row_1based = (int)row + 1;
  BOOL hasImage = NO;

  if (ih->data->show_image)
  {
    NSImage* ns_image = nil;
    IcocoaTableData* td = cocoaTableGetData(ih);

    if (td->is_virtual_mode)
    {
      char* image_name = iupTableGetCellImageCb(ih, row_1based, col_1based);
      if (image_name)
        ns_image = (NSImage*)iupImageGetImage(image_name, ih, 0, NULL);
    }
    else
    {
      char* image_name = iupAttribGetId2(ih, "_IUPCOCOA_CELLIMAGE", row_1based, col_1based);
      if (image_name)
        ns_image = (NSImage*)iupImageGetImage(image_name, ih, 0, NULL);
    }

    if (ns_image)
    {
      if (ih->data->fit_image)
        [[cellView imageView] setImageScaling:NSImageScaleProportionallyDown];
      else
        [[cellView imageView] setImageScaling:NSImageScaleNone];

      [[cellView imageView] setImage:ns_image];
      [[cellView imageView] setHidden:NO];
      hasImage = YES;
    }
    else
    {
      [[cellView imageView] setImage:nil];
      [[cellView imageView] setHidden:YES];
    }
  }
  else
  {
    [[cellView imageView] setImage:nil];
    [[cellView imageView] setHidden:YES];
  }

  [cellView.textField setEditable:cocoaTableIsCellEditable(ih, col_1based)];

  /* Set delegate every time due to cell reuse */
  [cellView.textField setDelegate:self];

  IcocoaTableData* table_data = cocoaTableGetData(ih);
  NSNumber* colKey = @(col);
  NSValue* colInfoValue = [table_data->column_info objectForKey:colKey];

  NSTextAlignment alignment = NSTextAlignmentLeft; /* Default */

  if (colInfoValue)
  {
    IcocoaTableColumnInfo colInfo;
    [colInfoValue getValue:&colInfo];
    alignment = colInfo.alignment;
  }
  else
  {
    /* early rendering happens before NUMCOL completes, so read the attribute directly */
    char align_name[50];
    snprintf(align_name, sizeof(align_name), "ALIGNMENT%d", col_1based);
    char* align_str = iupAttribGet(ih, align_name);

    if (align_str)
    {
      if (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT"))
        alignment = NSTextAlignmentRight;
      else if (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER"))
        alignment = NSTextAlignmentCenter;
      else if (iupStrEqualNoCase(align_str, "ALEFT") || iupStrEqualNoCase(align_str, "LEFT"))
        alignment = NSTextAlignmentLeft;
    }
  }

  cellView.textField.alignment = alignment;

  BOOL isFocused = (table_data->current_row == row_1based && table_data->current_col == col_1based);

  if ([cellView isFocusedCell] != isFocused)
  {
    [cellView setIsFocusedCell:isFocused];
    [cellView setNeedsDisplay:YES];
  }

  BOOL isSelected = [tableView isRowSelected:row];
  cocoaTableApplyCellColors(ih, cellView, (int)row + 1, col_1based, isSelected);
  cocoaTableApplyCellFont(ih, cellView.textField, (int)row + 1, col_1based);

  NSRect cellFrame = [tableView frameOfCellAtColumn:col row:row];
  CGFloat xStart = 2.0;

  if (hasImage)
  {
    CGFloat imgSize = 16.0;
    CGFloat imgY = floor((cellFrame.size.height - imgSize) / 2.0);
    [[cellView imageView] setFrame:NSMakeRect(xStart, imgY, imgSize, imgSize)];
    xStart += imgSize + 4.0;
  }

  CGFloat textHeight = [cellView.textField intrinsicContentSize].height;
#ifdef GNUSTEP
  if (textHeight <= 0 || textHeight > cellFrame.size.height) textHeight = cellFrame.size.height;
#endif
  CGFloat yOffset = floor((cellFrame.size.height - textHeight) / 2.0);
  NSRect textFrame;

  if (!hasImage && alignment == NSTextAlignmentCenter)
  {
    CGFloat tw = cellFrame.size.width; if (tw < 0) tw = 0;
    textFrame = NSMakeRect(0.0, yOffset, tw, textHeight);
  }
  else
  {
    CGFloat tw = cellFrame.size.width - xStart - 4.0; if (tw < 0) tw = 0;
    textFrame = NSMakeRect(xStart, yOffset, tw, textHeight);
  }

  [cellView.textField setFrame:textFrame];

  return cellView;
}

- (NSTableRowView*)tableView:(NSTableView*)tableView rowViewForRow:(NSInteger)row
{
  NSColor* bgcolor = cocoaTableGetRowBackgroundColor(ih, (int)row);

  if (!bgcolor)
    return nil;

  static NSString* const kRowViewIdentifier = @"IupCocoaTableRowView";
  IupCocoaTableRowView* rowView = [tableView makeViewWithIdentifier:kRowViewIdentifier owner:self];

  if (!rowView)
  {
    rowView = [[IupCocoaTableRowView alloc] initWithFrame:NSZeroRect];
    rowView.identifier = kRowViewIdentifier;
    [rowView autorelease];
  }

  [rowView setCustomBackgroundColor:bgcolor];
  return rowView;
}

- (void)tableViewSelectionDidChange:(NSNotification*)notification
{
  NSTableView* tableView = [notification object];
  NSInteger selectedRow = [tableView selectedRow];

  /* NSTableView tracks no current column in row-selection mode, so it is tracked manually */
  IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
  int col = 1;
  if (table_data && table_data->current_col > 0)
    col = table_data->current_col;

  int old_focused_row = 0;
  int old_focused_col = 0;
  if (table_data)
  {
    old_focused_row = table_data->current_row;
    old_focused_col = table_data->current_col;
  }

  if (table_data)
    table_data->current_row = (int)(selectedRow + 1);

  if (table_data)
  {
    NSMutableIndexSet* rowsToReload = [NSMutableIndexSet indexSet];

    if (table_data->previous_selected_row >= 0 && table_data->previous_selected_row < [tableView numberOfRows])
      [rowsToReload addIndex:table_data->previous_selected_row];

    if (old_focused_row > 0)
    {
      int old_focused_row_0based = old_focused_row - 1;
      if (old_focused_row_0based >= 0 && old_focused_row_0based < [tableView numberOfRows])
        [rowsToReload addIndex:old_focused_row_0based];
    }
    if (table_data->current_row > 0)
    {
      int current_row_0based = table_data->current_row - 1;
      if (current_row_0based >= 0 && current_row_0based < [tableView numberOfRows])
        [rowsToReload addIndex:current_row_0based];
    }

    if ([rowsToReload count] > 0)
    {
      NSIndexSet* colIndexes = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [tableView numberOfColumns])];
      [tableView reloadDataForRowIndexes:rowsToReload columnIndexes:colIndexes];
    }

    /* Defer setting selection until next run loop cycle when row views exist */
    if (selectedRow >= 0)
    {
      NSInteger row = selectedRow;
      dispatch_async(dispatch_get_main_queue(), ^{
        NSTableRowView* rowView = [tableView rowViewAtRow:row makeIfNecessary:NO];
        if (rowView)
        {
          [rowView setSelected:YES];
          [rowView setNeedsDisplay:YES];
        }
      });
    }

    table_data->previous_selected_row = (int)selectedRow;
    table_data->previous_focused_row = old_focused_row;
    table_data->previous_focused_col = old_focused_col;
  }

  IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (enteritem_cb)
  {
    enteritem_cb(ih, (int)selectedRow + 1, col);
  }
}

- (BOOL)tableView:(NSTableView*)tableView shouldEditTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row
{
  NSInteger colIndex = [[tableView tableColumns] indexOfObject:tableColumn];
  int col_1based = (int)colIndex + 1;

  if (!cocoaTableIsCellEditable(ih, col_1based))
  {
    return NO;
  }

  int lin = (int)row + 1;
  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    int ret = editbegin_cb(ih, lin, col_1based);
    if (ret == IUP_IGNORE)
    {
      return NO;
    }
  }

  objc_setAssociatedObject(tableView, &kEditingRowKey, @(row), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditingColKey, @(colIndex), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditEndedKey, @(NO), OBJC_ASSOCIATION_RETAIN);

  return YES;
}

- (void)tableViewAction:(id)sender
{
  NSTableView* tableView = (NSTableView*)sender;

  NSInteger clickedRow = [tableView clickedRow];
  NSInteger clickedCol = [tableView clickedColumn];

  int lin = (clickedRow >= 0) ? (int)(clickedRow + 1) : 0;
  int col = (clickedCol >= 0) ? (int)(clickedCol + 1) : 0;

  IFniis click_cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
  if (click_cb && lin > 0 && col > 0)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

    NSEvent* currentEvent = [NSApp currentEvent];
    if (currentEvent)
      iupcocoaButtonKeySetStatus(currentEvent, status);

    click_cb(ih, lin, col, status);
  }

  if (clickedRow >= 0 && clickedCol >= 0)
  {
    if (cocoaTableIsCellEditable(ih, col))
    {
      IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
      if (editbegin_cb)
      {
        int ret = editbegin_cb(ih, lin, col);
        if (ret == IUP_IGNORE)
          return;
      }

      objc_setAssociatedObject(tableView, &kEditingRowKey, @(clickedRow), OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditingColKey, @(clickedCol), OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditEndedKey, @(NO), OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditBeginCalledKey, @(YES), OBJC_ASSOCIATION_RETAIN);

      [tableView editColumn:clickedCol row:clickedRow withEvent:nil select:YES];
    }
  }
}

- (BOOL)control:(NSControl*)control textShouldBeginEditing:(NSText*)fieldEditor
{
  NSTableView* tableView = cocoaTableGetTableView(ih);

  NSNumber* existingRow = objc_getAssociatedObject(tableView, &kEditingRowKey);
  if (existingRow)
      return YES;

  NSInteger row = [tableView rowForView:control];
  NSInteger col = [tableView columnForView:control];

  if (row < 0 || col < 0) {
      IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
      if (table_data && table_data->current_row > 0 && table_data->current_col > 0) {
          row = table_data->current_row - 1;
          col = table_data->current_col - 1;
      }
  }

  if (row < 0 || col < 0) return YES;

  int lin = (int)row + 1;
  int col_1based = (int)col + 1;

  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    if (editbegin_cb(ih, lin, col_1based) == IUP_IGNORE)
      return NO;
  }

  objc_setAssociatedObject(tableView, &kEditingRowKey, @(row), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditingColKey, @(col), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditEndedKey, @(NO), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditBeginCalledKey, @(YES), OBJC_ASSOCIATION_RETAIN);

  return YES;
}

- (BOOL)control:(NSControl*)control textShouldEndEditing:(NSText*)fieldEditor
{
  return YES;
}

- (BOOL)control:(NSControl*)control textView:(NSTextView*)textView doCommandBySelector:(SEL)commandSelector
{
  if (commandSelector == @selector(cancelOperation:))
  {
    NSTableView* tableView = cocoaTableGetTableView(ih);
    NSTextField* textField = (NSTextField*)control;

    NSInteger row = [tableView rowForView:textField];
    NSInteger col = [tableView columnForView:textField];

    if (row < 0 || col < 0) {
      NSNumber *nRow = objc_getAssociatedObject(tableView, &kEditingRowKey);
      NSNumber *nCol = objc_getAssociatedObject(tableView, &kEditingColKey);
      if (nRow && nCol) {
        row = [nRow integerValue];
        col = [nCol integerValue];
      }
    }

    if (row >= 0 && col >= 0)
    {
      int lin = (int)row + 1;
      int col_1based = (int)col + 1;

      NSString* currentValue = [textField stringValue];
      const char* value = [currentValue UTF8String];

      NSNumber* editBeginCalled = objc_getAssociatedObject(tableView, &kEditBeginCalledKey);

      if (editBeginCalled && [editBeginCalled boolValue])
      {
        IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
        if (editend_cb)
          editend_cb(ih, lin, col_1based, (char*)value, 0);
      }

      objc_setAssociatedObject(tableView, &kEditEndedKey, @(YES), OBJC_ASSOCIATION_RETAIN);

      objc_setAssociatedObject(tableView, &kEditingRowKey, nil, OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditingColKey, nil, OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditBeginCalledKey, nil, OBJC_ASSOCIATION_RETAIN);

      NSString* original = cocoaTableGetCellValue(ih, (int)row, (int)col);
      if (original) [textField setStringValue:original];
    }

    return NO;
  }

  return NO;
}

- (void)controlTextDidBeginEditing:(NSNotification*)notification
{
  NSTableView* tableView = cocoaTableGetTableView(ih);

  NSTextField* textField = [notification object];
  NSInteger row = [tableView rowForView:textField];
  NSInteger col = [tableView columnForView:textField];
  if (row < 0 || col < 0)
    return;

  NSNumber* existingRow = objc_getAssociatedObject(tableView, &kEditingRowKey);
  if (existingRow)
      return;

  int lin = (int)row + 1;
  int col_1based = (int)col + 1;

  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    editbegin_cb(ih, lin, col_1based);
  }

  objc_setAssociatedObject(tableView, &kEditingRowKey, @(row), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditingColKey, @(col), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditEndedKey, @(NO), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditBeginCalledKey, @(YES), OBJC_ASSOCIATION_RETAIN);
}

- (void)controlTextDidEndEditing:(NSNotification*)notification
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  NSTextField* textField = [notification object];

  NSInteger row = [tableView rowForView:textField];
  NSInteger col = [tableView columnForView:textField];
  if (row < 0 || col < 0)
  {
    NSNumber* editBeginCalled = objc_getAssociatedObject(tableView, &kEditBeginCalledKey);
    if (editBeginCalled && [editBeginCalled boolValue])
    {
      objc_setAssociatedObject(tableView, &kEditEndedKey, @(YES), OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditingRowKey, nil, OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditingColKey, nil, OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditBeginCalledKey, nil, OBJC_ASSOCIATION_RETAIN);
    }
    return;
  }

  NSNumber* editEnded = objc_getAssociatedObject(tableView, &kEditEndedKey);
  if (editEnded && [editEnded boolValue])
      return;

  NSNumber *nRow = objc_getAssociatedObject(tableView, &kEditingRowKey);
  NSNumber *nCol = objc_getAssociatedObject(tableView, &kEditingColKey);
  if (nRow && nCol) {
      row = [nRow integerValue];
      col = [nCol integerValue];
  }

  objc_setAssociatedObject(tableView, &kEditEndedKey, @(YES), OBJC_ASSOCIATION_RETAIN);

  objc_setAssociatedObject(tableView, &kEditingRowKey, nil, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditingColKey, nil, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditBeginCalledKey, nil, OBJC_ASSOCIATION_RETAIN);

  if (row < 0 || col < 0) return;

  int lin = (int)row + 1;
  int col_1based = (int)col + 1;

  NSString* newValStr = [textField stringValue];
  const char* value = [newValStr UTF8String];

  int apply = 1;
  NSDictionary* userInfo = [notification userInfo];
  if (userInfo) {
      NSNumber* movement = [userInfo objectForKey:@"NSTextMovement"];
      /* NSCancelTextMovement = 0x17 (ESC key) */
      if (movement && [movement intValue] == 0x17)
          apply = 0;
  }

  IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb)
  {
    int ret = editend_cb(ih, lin, col_1based, (char*)value, apply);
    if (ret == IUP_IGNORE)
    {
      /* the field editor is already closing here, so only the visual value can be reverted */
       NSString* original = cocoaTableGetCellValue(ih, (int)row, (int)col);
       if (original) [textField setStringValue:original];
       return;
    }
  }

  if (apply)
  {
    char* old_value_ptr = iupdrvTableGetCellValue(ih, lin, col_1based);
    char* old_value = old_value_ptr ? iupStrDup(old_value_ptr) : NULL;

    cocoaTableSetCellValue(ih, (int)row, (int)col, value);

    if (cocoaTableValueChanged(old_value, value))
    {
      IFnii valuechanged_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
      if (valuechanged_cb)
        valuechanged_cb(ih, lin, col_1based);
    }

    if (old_value)
      free(old_value);
  }
}

- (void)tableView:(NSTableView *)tableView didDragTableColumn:(NSTableColumn *)tableColumn
{
  if (!ih) return;

  NSArray* columns = [tableView tableColumns];
  NSInteger new_native_pos = [columns indexOfObject:tableColumn];
  if (new_native_pos == NSNotFound) return;

  NSNumber* stored = objc_getAssociatedObject(tableColumn, "iup_col");
  int old_pos = stored ? [stored intValue] : 0;
  if (old_pos <= 0) return;

  int new_pos = (int)new_native_pos + 1;
  if (old_pos == new_pos) return;

  IFnii cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
  if (cb)
    cb(ih, old_pos, new_pos);

  for (NSUInteger i = 0; i < [columns count]; i++)
  {
    NSTableColumn* col = [columns objectAtIndex:i];
    objc_setAssociatedObject(col, "iup_col", [NSNumber numberWithInt:(int)i + 1], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
}

@end

/* ========================================================================= */
/* Attribute Get/Set Functions                                               */
/* ========================================================================= */

static int cocoaTableSetNumColAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  int num_col = 0;
  if (iupStrToInt(value, &num_col))
  {
    IcocoaTableData* table_data = cocoaTableGetData(ih);
    NSTableView* tableView = cocoaTableGetTableView(ih);

    NSArray* columns = [tableView tableColumns];
    for (NSTableColumn* col in columns)
    {
      [tableView removeTableColumn:col];
    }

    for (int i = 0; i < num_col; i++)
    {
      NSString* identifier = [NSString stringWithFormat:@"%d", i];
      NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:identifier];

      IupCocoaTableHeaderCell* headerCell = [[IupCocoaTableHeaderCell alloc] initTextCell:[NSString stringWithFormat:@"Col %d", i + 1]];
      [column setHeaderCell:headerCell];
      [headerCell release];

      char align_name[50];
      snprintf(align_name, sizeof(align_name), "ALIGNMENT%d", i + 1);
      char* align_str = iupAttribGet(ih, align_name);

      NSTextAlignment alignment = NSTextAlignmentLeft; /* Default */
      if (align_str)
      {
        if (iupStrEqualNoCase(align_str, "ARIGHT") || iupStrEqualNoCase(align_str, "RIGHT"))
          alignment = NSTextAlignmentRight;
        else if (iupStrEqualNoCase(align_str, "ACENTER") || iupStrEqualNoCase(align_str, "CENTER"))
          alignment = NSTextAlignmentCenter;
        else if (iupStrEqualNoCase(align_str, "ALEFT") || iupStrEqualNoCase(align_str, "LEFT"))
          alignment = NSTextAlignmentLeft;
      }

      [column.headerCell setAlignment:alignment];

      char width_name[50];
      snprintf(width_name, sizeof(width_name), "RASTERWIDTH%d", i + 1);
      char* width_str = iupAttribGet(ih, width_name);
      if (!width_str)
      {
        snprintf(width_name, sizeof(width_name), "WIDTH%d", i + 1);
        width_str = iupAttribGet(ih, width_name);
      }

      /* WIDTH%d is purged from the hash post-map; record an explicit-width marker for later passes. */
      char expwidth_name[50];
      snprintf(expwidth_name, sizeof(expwidth_name), "_IUP_TABLE_EXPWIDTH%d", i + 1);
      iupAttribSetStr(ih, expwidth_name, width_str ? "1" : NULL);

      if (i == num_col - 1)
      {
        if (width_str || !ih->data->stretch_last)
        {
          iupAttribSet(ih, "_IUP_TABLE_LAST_COL_WIDTH_SET", "YES");

          if (ih->data->user_resize)
            [column setResizingMask:NSTableColumnUserResizingMask];
          else
            [column setResizingMask:NSTableColumnNoResizing];

          if (width_str)
          {
            int width_val = 0;
            if (iupStrToInt(width_str, &width_val) && width_val > 0)
              [column setWidth:width_val];
          }
        }
        else
        {
          iupAttribSet(ih, "_IUP_TABLE_LAST_COL_WIDTH_SET", "NO");

          if (ih->data->user_resize)
            [column setResizingMask:NSTableColumnUserResizingMask | NSTableColumnAutoresizingMask];
          else
            [column setResizingMask:NSTableColumnAutoresizingMask];
        }
      }
      else if (width_str)
      {
        if (ih->data->user_resize)
          [column setResizingMask:NSTableColumnUserResizingMask];
        else
          [column setResizingMask:NSTableColumnNoResizing];

        int width_val = 0;
        if (iupStrToInt(width_str, &width_val) && width_val > 0)
          [column setWidth:width_val];
      }
      else
      {
        [column setResizingMask:NSTableColumnNoResizing];
        [column sizeToFit];
      }

      if (ih->data->sortable)
      {
        NSSortDescriptor* sortDescriptor = [[NSSortDescriptor alloc] initWithKey:identifier ascending:YES];
        [column setSortDescriptorPrototype:sortDescriptor];
        [sortDescriptor release];
      }

      [tableView addTableColumn:column];
      objc_setAssociatedObject(column, "iup_col", [NSNumber numberWithInt:i + 1], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
      [column release];

      IcocoaTableColumnInfo colInfo;
      colInfo.title = [[NSString stringWithFormat:@"Col %d", i + 1] retain];
      colInfo.width = 80.0;
      colInfo.alignment = alignment;
      colInfo.sortable = ih->data->sortable ? YES : NO;
      colInfo.resizable = ih->data->user_resize ? YES : NO;

      NSValue* colInfoValue = [NSValue valueWithBytes:&colInfo objCType:@encode(IcocoaTableColumnInfo)];
      [table_data->column_info setObject:colInfoValue forKey:@(i)];
    }

    ih->data->num_col = num_col;
    return 1;
  }

  return 0;
}

static int cocoaTableSetNumLinAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)
    return 0;

  int num_lin = 0;
  if (iupStrToInt(value, &num_lin))
  {
    ih->data->num_lin = num_lin;

    NSTableView* tableView = cocoaTableGetTableView(ih);
    [tableView reloadData];
    return 1;
  }

  return 0;
}

/* ========================================================================= */
/* Driver-Specific Attribute Handlers                                       */
/* ========================================================================= */

static int cocoaTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  BOOL sortable = iupStrBoolean(value);
  ih->data->sortable = sortable;

  if (!ih->handle)
    return 0;

  NSTableView* tableView = cocoaTableGetTableView(ih);

  NSArray* columns = [tableView tableColumns];
  for (NSTableColumn* column in columns)
  {
    if (sortable)
    {
      NSString* identifier = [column identifier];
      NSSortDescriptor* sortDescriptor = [[NSSortDescriptor alloc] initWithKey:identifier ascending:YES];
      [column setSortDescriptorPrototype:sortDescriptor];
      [sortDescriptor release];
    }
    else
    {
      [column setSortDescriptorPrototype:nil];
    }
  }

  return 1;
}

static int cocoaTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  BOOL resizable = iupStrBoolean(value);
  ih->data->user_resize = resizable;

  if (!ih->handle)
    return 0;

  NSTableView* tableView = cocoaTableGetTableView(ih);

  NSArray* columns = [tableView tableColumns];
  NSUInteger col_count = [columns count];
  for (NSUInteger i = 0; i < col_count; i++)
  {
    NSTableColumn* column = [columns objectAtIndex:i];

    if (i == col_count - 1)
    {
      if (resizable)
        [column setResizingMask:NSTableColumnUserResizingMask | NSTableColumnAutoresizingMask];
      else
        [column setResizingMask:NSTableColumnAutoresizingMask];
    }
    else
    {
      if (resizable)
        [column setResizingMask:NSTableColumnUserResizingMask];
      else
        [column setResizingMask:NSTableColumnNoResizing];
    }
  }

  return 1;
}

static int cocoaTableSetReorderAttrib(Ihandle* ih, const char* value)
{
  BOOL reorder = iupStrBoolean(value);
  ih->data->allow_reorder = reorder;

  if (!ih->handle)
    return 0;

  NSTableView* tableView = cocoaTableGetTableView(ih);

  [tableView setAllowsColumnReordering:reorder];

  return 1;
}

/* ========================================================================= */
/* Driver Functions - Table Structure                                       */
/* ========================================================================= */

IUP_SDK_API void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  if (num_col < 0)
    num_col = 0;

  ih->data->num_col = num_col;

  if (ih->handle)
    cocoaTableSetNumColAttrib(ih, iupStrReturnInt(num_col));
}

IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  if (num_lin < 0)
    num_lin = 0;

  ih->data->num_lin = num_lin;

  if (ih->handle)
    cocoaTableSetNumLinAttrib(ih, iupStrReturnInt(num_lin));
}

IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  iupdrvTableSetNumCol(ih, ih->data->num_col + 1);
}

IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  if (ih->data->num_col > 0)
    iupdrvTableSetNumCol(ih, ih->data->num_col - 1);
}

IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  iupdrvTableSetNumLin(ih, ih->data->num_lin + 1);
}

IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  if (ih->data->num_lin > 0)
    iupdrvTableSetNumLin(ih, ih->data->num_lin - 1);
}

/* ========================================================================= */
/* Driver Functions - Cell Operations                                       */
/* ========================================================================= */

IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (!table_data)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  cocoaTableSetCellValue(ih, lin - 1, col - 1, value);

  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (tableView)
  {
    NSIndexSet* rowSet = [NSIndexSet indexSetWithIndex:(lin - 1)];
    NSIndexSet* colSet = [NSIndexSet indexSetWithIndex:(col - 1)];
    [tableView reloadDataForRowIndexes:rowSet columnIndexes:colSet];
  }
}

IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (!table_data)
    return NULL;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return NULL;

  NSString* value = cocoaTableGetCellValue(ih, lin - 1, col - 1);
  if (value && [value length] > 0)
  {
    return iupStrReturnStr([value UTF8String]);
  }

  return NULL;
}

IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (!table_data)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  if (table_data->is_virtual_mode)
    return;

  iupAttribSetStrId2(ih, "_IUPCOCOA_CELLIMAGE", lin, col, image);

  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (tableView)
  {
    [tableView reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:lin - 1]
                         columnIndexes:[NSIndexSet indexSetWithIndex:col - 1]];
  }
}

/* ========================================================================= */
/* Driver Functions - Column Operations                                     */
/* ========================================================================= */

IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (!tableView)
    return;

  if (col < 1 || col > ih->data->num_col)
    return;

  NSArray* columns = [tableView tableColumns];
  if (col - 1 < [columns count])
  {
    NSTableColumn* column = [columns objectAtIndex:(col - 1)];
    NSString* titleStr = title ? [NSString stringWithUTF8String:title] : @"";
    [column.headerCell setStringValue:titleStr];

    int col_index = col - 1;

    /* explicit-width marker (see cocoaTableSetNumColAttrib), not WIDTH%d */
    char expwidth_name[50];
    snprintf(expwidth_name, sizeof(expwidth_name), "_IUP_TABLE_EXPWIDTH%d", col);
    int has_explicit_width = iupAttribGet(ih, expwidth_name) != NULL;

    if (!has_explicit_width && col_index < ih->data->num_col - 1)
    {
      NSFont* font = [NSFont systemFontOfSize:[NSFont systemFontSize]];
      CGFloat calculated_width = cocoaTableCalculateColumnWidth(ih, col_index, font);
      [column setWidth:calculated_width];
    }
  }
}

IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (!tableView)
    return NULL;

  if (col < 1 || col > ih->data->num_col)
    return NULL;

  NSArray* columns = [tableView tableColumns];
  if (col - 1 < [columns count])
  {
    NSTableColumn* column = [columns objectAtIndex:(col - 1)];
    NSString* title = [column.headerCell stringValue];
    if (title && [title length] > 0)
    {
      return iupStrReturnStr([title UTF8String]);
    }
  }

  return NULL;
}

IUP_SDK_API void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (!tableView)
    return;

  if (col < 1 || col > ih->data->num_col)
    return;

  NSArray* columns = [tableView tableColumns];
  if (col - 1 < [columns count])
  {
    NSTableColumn* column = [columns objectAtIndex:(col - 1)];

    [column setWidth:width];

    char expwidth_name[50];
    snprintf(expwidth_name, sizeof(expwidth_name), "_IUP_TABLE_EXPWIDTH%d", col);
    iupAttribSetStr(ih, expwidth_name, "1");

    if (col - 1 == ih->data->num_col - 1)
      iupAttribSet(ih, "_IUP_TABLE_LAST_COL_WIDTH_SET", "YES");

    if (ih->data->user_resize)
      [column setResizingMask:NSTableColumnUserResizingMask];
    else
      [column setResizingMask:NSTableColumnNoResizing];
  }
}

IUP_SDK_API int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (!tableView)
    return 0;

  if (col < 1 || col > ih->data->num_col)
    return 0;

  NSArray* columns = [tableView tableColumns];
  if (col - 1 < [columns count])
  {
    NSTableColumn* column = [columns objectAtIndex:(col - 1)];
    return (int)[column width];
  }

  return 0;
}

/* ========================================================================= */
/* Driver Functions - Selection                                             */
/* ========================================================================= */

IUP_SDK_API void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (!tableView)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  int row = lin - 1;
  int column = col - 1;

  IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
  if (table_data)
  {
    table_data->current_row = lin;
    table_data->current_col = col;
  }

  [tableView scrollRowToVisible:row];
  [tableView scrollColumnToVisible:column];
  [tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
}

IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (!tableView)
  {
    *lin = 0;
    *col = 0;
    return;
  }

  NSInteger selectedRow = [tableView selectedRow];
  NSInteger selectedColumn = [tableView selectedColumn];

  *lin = (selectedRow >= 0) ? (int)selectedRow + 1 : 0;
  *col = (selectedColumn >= 0) ? (int)selectedColumn + 1 : 0;
}

/* ========================================================================= */
/* Driver Functions - Scrolling & Display                                   */
/* ========================================================================= */

IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (!tableView)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  int row = lin - 1;
  int column = col - 1;

  [tableView scrollRowToVisible:row];
  [tableView scrollColumnToVisible:column];
}

IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (tableView)
  {
    [tableView reloadData];
  }
}

IUP_SDK_API void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (!tableView)
    return;

  if (show)
    [tableView setGridStyleMask:(NSTableViewSolidVerticalGridLineMask | NSTableViewSolidHorizontalGridLineMask)];
  else
    [tableView setGridStyleMask:NSTableViewGridNone];

#ifdef GNUSTEP
  /* GNUstep -setGridStyleMask: is a no-op and _drawsGrid defaults to YES; drive directly. */
  [tableView setDrawsGrid:(show ? YES : NO)];
#endif
}

IUP_SDK_API int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  /* NSScrollView with NSBezelBorder has 1px border on each side */
  return 2;
}

static int cocoa_table_row_height = -1;
static int cocoa_table_header_height = -1;

static void cocoaTableMeasureRowMetrics(Ihandle* ih)
{
  if (cocoa_table_row_height >= 0)
    return;

  NSTableView* temp_table = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 200, 200)];

  NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"test"];
  [column setWidth:100];
  [[column headerCell] setStringValue:@"Test"];
  [temp_table addTableColumn:column];

  cocoa_table_row_height = (int)[temp_table rowHeight];

  if (cocoa_table_row_height <= 0)
  {
    int charheight;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    cocoa_table_row_height = charheight + 4;
  }

  NSTableHeaderView* header = [temp_table headerView];
  if (header)
  {
    NSRect header_frame = [header frame];
    cocoa_table_header_height = (int)NSHeight(header_frame);

    if (cocoa_table_header_height <= 0)
    {
      NSSize header_size = [header intrinsicContentSize];
      cocoa_table_header_height = (int)header_size.height;
    }
  }

  if (cocoa_table_header_height <= 0)
  {
    int charheight;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    cocoa_table_header_height = charheight + 6;
  }

  [column release];
  [temp_table release];
}

IUP_SDK_API int iupdrvTableGetRowHeight(Ihandle* ih)
{
  NSTableView* table_view = cocoaTableGetTableView(ih);

  if (table_view)
  {
    CGFloat row_height = [table_view rowHeight];
    if (row_height > 0)
      return (int)row_height;
  }

  cocoaTableMeasureRowMetrics(ih);
  return cocoa_table_row_height;
}

IUP_SDK_API int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  NSTableView* table_view = cocoaTableGetTableView(ih);

  if (table_view)
  {
    NSTableHeaderView* header = [table_view headerView];
    if (header)
    {
      NSRect frame = [header frame];
      int height = (int)NSHeight(frame);
      if (height > 0)
        return height;
    }
  }

  cocoaTableMeasureRowMetrics(ih);
  return cocoa_table_header_height;
}

IUP_SDK_API void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  NSScrollView* scroll = cocoaTableGetScrollView(ih);
  int sb_size = iupdrvGetScrollbarSize();

  if (scroll)
  {
    NSRect frame = [scroll frame];
    NSSize content = [scroll contentSize];
    int border_x = (int)(NSWidth(frame) - content.width);
    int border_y = (int)(NSHeight(frame) - content.height);

    if (border_x > 0 && border_y > 0)
    {
      *w += border_x;
      *h += border_y;
    }
    else
    {
      *w += sb_size + 2;
      *h += 2;
    }
  }
  else
  {
    *w += sb_size + 2;
    *h += 2;
  }

  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0 && ih->data->num_col > visiblecolumns)
    *h += sb_size;
}

/* ========================================================================= */
/* Widget Creation - MapMethod                                               */
/* ========================================================================= */

static int cocoaTableMapMethod(Ihandle* ih)
{
  char* value;

  cocoaTableInitializeData(ih);

  value = iupAttribGet(ih, "VIRTUALMODE");
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (iupStrBoolean(value))
    table_data->is_virtual_mode = YES;

  NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
  [scrollView setHasVerticalScroller:YES];
  [scrollView setHasHorizontalScroller:NO];
  [scrollView setAutohidesScrollers:YES];
  [scrollView setBorderType:NSBezelBorder];

  IupCocoaTableView* tableView = [[IupCocoaTableView alloc] initWithIhandle:ih];

  IupNarrowTableHeaderView* headerView = [[IupNarrowTableHeaderView alloc] init];
#ifdef GNUSTEP
  /* GNUstep does not size a custom header view, and a zero height collapses the header clip view */
  [headerView setFrameSize:NSMakeSize(0, 22)];
#endif
  [tableView setHeaderView:headerView];
  [headerView release];

  [tableView setIntercellSpacing:NSMakeSize(0, 0)];
#ifdef GNUSTEP
  /* per-column widths would be overridden by autoresizesAllColumnsToFit */
  [tableView setDrawsGrid:NO];
#endif

  char* alternate_color = iupAttribGetStr(ih, "ALTERNATECOLOR");
  if (iupStrBoolean(alternate_color))
  {
    char* even_color = iupAttribGetStr(ih, "EVENROWCOLOR");
    char* odd_color = iupAttribGetStr(ih, "ODDROWCOLOR");

    if (!even_color && !odd_color)
      [tableView setUsesAlternatingRowBackgroundColors:YES];
    else
      [tableView setUsesAlternatingRowBackgroundColors:NO];
  }
  else
  {
    [tableView setUsesAlternatingRowBackgroundColors:NO];
  }

  [tableView setAllowsColumnReordering:(ih->data->allow_reorder ? YES : NO)];

  char* selmode = iupAttribGetStr(ih, "SELECTIONMODE");
  if (!selmode)
    selmode = "SINGLE";  /* Default */

  if (iupStrEqualNoCase(selmode, "NONE"))
    [tableView setAllowsEmptySelection:NO];
  else if (iupStrEqualNoCase(selmode, "MULTIPLE"))
    [tableView setAllowsMultipleSelection:YES];
  else
    [tableView setAllowsMultipleSelection:NO];

  [tableView setColumnAutoresizingStyle:NSTableViewNoColumnAutoresizing];

  IupCocoaTableDataSource* dataSource = [[IupCocoaTableDataSource alloc] initWithIhandle:ih];
  IupCocoaTableDelegate* delegate = [[IupCocoaTableDelegate alloc] initWithIhandle:ih];

  [tableView setDataSource:dataSource];
  [tableView setDelegate:delegate];
  [tableView setDoubleAction:@selector(tableViewAction:)];
  [tableView setTarget:delegate];

  if (ih->data->show_dragdrop)
  {
    [tableView registerForDraggedTypes:@[kIupTableRowPasteboardType]];
    [tableView setDraggingSourceOperationMask:NSDragOperationMove forLocal:YES];
  }

  objc_setAssociatedObject(tableView, IUP_COCOA_TABLE_DATASOURCE_KEY, dataSource, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, IUP_COCOA_TABLE_DELEGATE_KEY, delegate, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, "_IUPCOCOA_IHANDLE", (void*)ih, OBJC_ASSOCIATION_ASSIGN);

  [dataSource release];
  [delegate release];

  [scrollView setDocumentView:tableView];
  [tableView release];

  /* Set handles BEFORE calling iupcocoaSetAssociatedViews */
  ih->handle = scrollView;
  iupAttribSet(ih, "_IUPCOCOA_TABLEVIEW", (char*)tableView);

  iupcocoaSetAssociatedViews(ih, tableView, scrollView);

  /* Set SHOWGRID - must be after tableView is stored in attribute */
  iupdrvTableSetShowGrid(ih, iupAttribGetBoolean(ih, "SHOWGRID"));

  if (ih->data->num_col > 0)
    iupdrvTableSetNumCol(ih, ih->data->num_col);

  if (ih->data->num_lin > 0)
    iupdrvTableSetNumLin(ih, ih->data->num_lin);

  iupcocoaAddToParent(ih);

  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    int row_height = iupdrvTableGetRowHeight(ih);
    int header_height = iupdrvTableGetHeaderHeight(ih);
    int sb_size = iupdrvGetScrollbarSize();

    int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    int need_horiz_sb = (visiblecolumns > 0 && ih->data->num_col > visiblecolumns);
    int horiz_sb_height = need_horiz_sb ? sb_size : 0;

    int target_height = header_height + (row_height * visiblelines) + horiz_sb_height + 2;
    objc_setAssociatedObject(scrollView, &kTargetHeightKey, @(target_height), OBJC_ASSOCIATION_RETAIN);
  }

  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0)
    objc_setAssociatedObject(scrollView, &kVisibleColumnsKey, @(visiblecolumns), OBJC_ASSOCIATION_RETAIN);

  return IUP_NOERROR;
}

static void cocoaTableUnMapMethod(Ihandle* ih)
{
  cocoaTableFreeData(ih);

  iupcocoaSetAssociatedViews(ih, nil, nil);

  /* Manually destroy the scroll view - don't call base method which expects simple NSView */
  if (ih->handle)
  {
    NSScrollView* scroll_view = (NSScrollView*)ih->handle;
    NSTableView* tableView = [scroll_view documentView];
    if (tableView)
    {
      [tableView setTarget:nil];
      [tableView setDataSource:nil];
      [tableView setDelegate:nil];
      objc_setAssociatedObject(tableView, IUP_COCOA_TABLE_DATASOURCE_KEY, nil, OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, IUP_COCOA_TABLE_DELEGATE_KEY, nil, OBJC_ASSOCIATION_RETAIN);
    }
    [scroll_view removeFromSuperview];
    [scroll_view release];
    ih->handle = NULL;
  }
}

static void cocoaTableLayoutUpdateMethod(Ihandle* ih)
{
  NSScrollView* scroll_view = (NSScrollView*)ih->handle;
  if (!scroll_view) return;

  NSView* parent_view = [scroll_view superview];
  if (!parent_view) return;

  int width = ih->currentwidth;
  int height = ih->currentheight;

  NSNumber* targetHeightNum = objc_getAssociatedObject(scroll_view, &kTargetHeightKey);
  if (targetHeightNum)
  {
    int target_height = [targetHeightNum intValue];
    if (target_height > 0 && height > target_height)
      height = target_height;
  }

  NSNumber* visColNum = objc_getAssociatedObject(scroll_view, &kVisibleColumnsKey);
  if (visColNum)
  {
    int visible_columns = [visColNum intValue];
    if (visible_columns > 0)
    {
      NSTableView* tableView = (NSTableView*)[scroll_view documentView];
      if (tableView && [tableView isKindOfClass:[NSTableView class]])
      {
        NSArray* columns = [tableView tableColumns];
        int num_cols = visible_columns;
        if (num_cols > (int)[columns count])
          num_cols = (int)[columns count];

        CGFloat cols_width = 0;
        for (int c = 0; c < num_cols; c++)
        {
          NSTableColumn* column = [columns objectAtIndex:c];
          CGFloat col_width = [column width];
          if (col_width <= 0)
            col_width = 80;
          cols_width += col_width;
        }

        int sb_size = iupdrvGetScrollbarSize();

        NSSize contentSz = [scroll_view contentSize];
        NSRect frameSz = [scroll_view frame];
        int border_w = (int)(NSWidth(frameSz) - contentSz.width);

        int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
        int need_vert_sb = (visiblelines > 0 && ih->data->num_lin > visiblelines);
        int vert_sb_width = need_vert_sb ? sb_size : 0;

        int cols_width_int = (int)ceil(cols_width);
        int target_width = cols_width_int + vert_sb_width + border_w;
        if (width > target_width)
          width = target_width;
      }
    }
  }

  NSRect parent_bounds = [parent_view bounds];
  NSRect child_rect;

  if ([parent_view isFlipped])
  {
    child_rect = NSMakeRect(ih->x, ih->y, width, height);
  }
  else
  {
    child_rect = NSMakeRect(ih->x, parent_bounds.size.height - ih->y - height, width, height);
  }

  [scroll_view setFrame:child_rect];

  NSRect documentVisibleRect = [scroll_view documentVisibleRect];

  NSTableView* tableView = (NSTableView*)[scroll_view documentView];
  if (tableView && [tableView isKindOfClass:[NSTableView class]])
  {
    NSArray* columns = [tableView tableColumns];
    CGFloat total_width = 0;
    for (NSTableColumn* column in columns)
    {
      CGFloat col_width = [column width];
      total_width += col_width;
    }

    [scroll_view setHasHorizontalScroller:(total_width > documentVisibleRect.size.width)];

    NSRect newDocumentVisibleRect = [scroll_view documentVisibleRect];
    NSRect tableFrame = [tableView frame];
    tableFrame.size.width = newDocumentVisibleRect.size.width;
    [tableView setFrame:tableFrame];
  }

}

/* ========================================================================= */
/* Class Registration                                                        */
/* ========================================================================= */

static int cocoaTableSetActiveAttrib(Ihandle* ih, const char* value)
{
  iupBaseSetActiveAttrib(ih, value);

  NSScrollView* scrollView = cocoaTableGetScrollView(ih);
  if (scrollView)
    [scrollView setAlphaValue:iupStrBoolean(value) ? 1.0 : 0.5];

  return 1;
}

IUP_SDK_API void iupdrvTableInitClass(Iclass* ic)
{
  /* Driver Dependent Class methods */
  ic->Map = cocoaTableMapMethod;
  ic->UnMap = cocoaTableUnMapMethod;
  ic->LayoutUpdate = cocoaTableLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FOCUSRECT", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, cocoaTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, cocoaTableSetReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, cocoaTableSetUserResizeAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ACTIVE", NULL, cocoaTableSetActiveAttrib);
}
