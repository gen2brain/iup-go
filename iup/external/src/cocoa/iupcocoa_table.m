/** \file
 * \brief Table Control for Cocoa
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_table.h"
#include "iup_tablecontrol.h"

#include "iupcocoa_drv.h"


/* Forward declarations */
@class IupCocoaTableDataSource;
@class IupCocoaTableDelegate;
@class IupCocoaTableView;

static const void* IUP_COCOA_TABLE_DATASOURCE_KEY = "IUP_COCOA_TABLE_DATASOURCE_KEY";
static const void* IUP_COCOA_TABLE_DELEGATE_KEY = "IUP_COCOA_TABLE_DELEGATE_KEY";

/* Static keys for associated objects to track editing state */
static char kEditingRowKey;
static char kEditingColKey;
static char kEditEndedKey;
static char kEditBeginCalledKey;
static char kTargetHeightKey;
static char kVisibleColumnsKey;

/* ========================================================================= */
/* Data Structures                                                           */
/* ========================================================================= */

typedef struct _IcocoaTableColumnInfo {
  NSString* title;
  CGFloat width;
  NSTextAlignment alignment;
  BOOL sortable;
  BOOL resizable;
} IcocoaTableColumnInfo;

typedef struct _IcocoaTableData {
  NSMutableArray* data_array;           /* Array of row arrays */
  NSMutableDictionary* column_info;     /* Column metadata by index */
  NSMutableDictionary* cell_attributes; /* Cell-specific attributes (colors, fonts, images) */
  BOOL is_virtual_mode;                 /* Using VALUE_CB callback */
  BOOL editable;                        /* Cells are editable */
  int sort_column;                      /* Currently sorted column (-1 = none) */
  BOOL sort_ascending;                  /* Sort order */
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
@end

@implementation IupCocoaTableCellView

@synthesize isFocusedCell;
@synthesize ih;

- (void)layout
{
  [super layout];

  /* Position text field centered vertically */
  if (self.textField)
  {
    CGFloat cellWidth = self.bounds.size.width;

    /* Get the table view and ask for current cell frame */
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

    /* Get text field's natural height */
    CGFloat textHeight = [self.textField intrinsicContentSize].height;

    /* Center vertically */
    CGFloat yOffset = floor((self.bounds.size.height - textHeight) / 2.0);

    /* Check alignment - CENTER needs full width, LEFT/RIGHT need padding */
    NSTextAlignment alignment = [self.textField alignment];
    NSRect textFrame;

    if (alignment == NSTextAlignmentCenter)
    {
      /* CENTER: Use full cell width */
      textFrame = NSMakeRect(0.0, yOffset, cellWidth, textHeight);
    }
    else
    {
      /* LEFT/RIGHT: Add 2px padding so text doesn't touch cell borders */
      textFrame = NSMakeRect(2.0, yOffset, cellWidth - 6.0, textHeight);
    }

    [self.textField setFrame:textFrame];
  }
}

- (void)drawRect:(NSRect)dirtyRect
{
  [super drawRect:dirtyRect];

  /* Draw dashed focus rectangle if this is the focused cell and FOCUSRECT=YES */
  if (isFocusedCell && ih && iupAttribGetBoolean(ih, "FOCUSRECT"))
  {
    /* Get the table view - in view-based table: cell -> row view -> table view */
    NSView* rowView = [self superview];  /* NSTableRowView */
    NSTableView* tableView = nil;
    if (rowView)
      tableView = (NSTableView*)[rowView superview];  /* NSTableView */

    if (tableView && [tableView isKindOfClass:[NSTableView class]])
    {
      /* Only draw if the table control (or one of its subviews) has focus */
      NSWindow* window = [self window];
      NSResponder* firstResp = [window firstResponder];

      /* Check if first responder is the table or a descendant of it */
      BOOL tableHasFocus = NO;
      if ([firstResp isEqual:tableView])
        tableHasFocus = YES;
      else if ([firstResp isKindOfClass:[NSView class]] && [(NSView*)firstResp isDescendantOf:tableView])
        tableHasFocus = YES;
      else if ([firstResp isKindOfClass:[NSText class]])
      {
        /* Check if first responder is a field editor (NSText) with delegate inside table */
        NSText* text = (NSText*)firstResp;
        id<NSTextDelegate> textDelegate = [text delegate];
        if (textDelegate && [textDelegate isKindOfClass:[NSView class]])
        {
          if ([(NSView*)textDelegate isDescendantOf:tableView])
            tableHasFocus = YES;
        }
      }

      if (tableHasFocus)
      {
        NSBezierPath* path = [NSBezierPath bezierPathWithRect:NSInsetRect(self.bounds, 1.5, 1.5)];

        /* Set dash pattern */
        CGFloat dashPattern[] = {2.0, 2.0};
        [path setLineDash:dashPattern count:2 phase:0.0];
        [path setLineWidth:1.0];

        /* Use labelColor, adapts to light/dark mode automatically */
        [[NSColor labelColor] setStroke];
        [path stroke];
      }
    }
  }
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
  table_data->current_row = 0;   /* No focus at start */
  table_data->current_col = 0;   /* No focus at start */
  table_data->previous_selected_row = -1;  /* No previous selection */
  table_data->previous_focused_row = 0;    /* No previous focus */
  table_data->previous_focused_col = 0;    /* No previous focus */

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

/* Forward declaration */
static NSString* cocoaTableGetCellValue(Ihandle* ih, int lin, int col);

static CGFloat cocoaTableCalculateColumnWidth(Ihandle* ih, int col_index, NSFont* font)
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  CGFloat max_width = 0.0;  /* Will be calculated from content */
  int max_rows_to_check = (ih->data->num_lin > 100) ? 100 : ih->data->num_lin;  /* Limit for performance */

  /* Measure column title */
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
        CGFloat title_width = title_size.width + 20.0;  /* Padding for sort indicator */
        if (title_width > max_width)
          max_width = title_width;
      }
    }
  }

  /* Measure cell content (check first N rows) */
  for (int lin = 0; lin < max_rows_to_check; lin++)
  {
    NSString* cell_value = cocoaTableGetCellValue(ih, lin, col_index);
    if (cell_value && [cell_value length] > 0)
    {
      NSDictionary* attrs = @{NSFontAttributeName: font};
      NSSize cell_size = [cell_value sizeWithAttributes:attrs];
      CGFloat cell_width = cell_size.width + 16.0;  /* Add padding (8px left + 8px right) */
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

  /* Check virtual mode first */
  if (table_data->is_virtual_mode)
  {
    sIFnii value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
    if (value_cb)
    {
      char* value = value_cb(ih, lin + 1, col + 1);  /* IUP uses 1-based indices */
      if (value)
        return [NSString stringWithUTF8String:value];
      return @"";
    }
  }

  /* Use internal storage */
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

  /* Ensure row exists */
  while (lin >= [table_data->data_array count])
  {
    NSMutableArray* row = [[NSMutableArray alloc] init];
    [table_data->data_array addObject:row];
    [row release];
  }

  NSMutableArray* row = [table_data->data_array objectAtIndex:lin];

  /* Ensure column exists in this row */
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

  /* Check per-row background color */
  char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, 0);

  /* If no specific bgcolor, check for alternating row colors */
  if (!bgcolor)
  {
    /* Check ALTERNATECOLOR flag first */
    char* alternate = iupAttribGet(ih, "ALTERNATECOLOR");
    if (iupStrBoolean(alternate))
    {
      /* ALTERNATECOLOR is YES - use EVENROWCOLOR or ODDROWCOLOR based on row number */
      char* even_color = iupAttribGet(ih, "EVENROWCOLOR");
      char* odd_color = iupAttribGet(ih, "ODDROWCOLOR");

      if (lin % 2 == 0)
        bgcolor = even_color;
      else
        bgcolor = odd_color;

      /* If custom colors are defined, return them */
      /* If no custom colors, return nil to use native NSTableView alternating colors */
    }
  }

  if (bgcolor && *bgcolor)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(bgcolor, &r, &g, &b))
    {
      return [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
    }
  }

  return nil;  /* No background color */
}

static void cocoaTableApplyCellColors(Ihandle* ih, NSTableCellView* cellView, int lin, int col, BOOL isSelected)
{
  /* lin and col are 1-based IUP indices */
  NSTextField* textField = cellView.textField;

  /* Background color, only apply if row is NOT selected */
  if (!isSelected)
  {
    /* Background color - hierarchy: L:C > :C > L:0 */
    char* bgcolor = iupAttribGetId2(ih, "BGCOLOR", lin, col);  /* Per-cell */
    if (!bgcolor)
      bgcolor = iupAttribGetId2(ih, "BGCOLOR", 0, col);  /* Per-column */

    if (bgcolor && *bgcolor)
    {
      /* Cell has specific background color */
      unsigned char r, g, b;
      if (iupStrToRGB(bgcolor, &r, &g, &b))
      {
        NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
        [cellView setWantsLayer:YES];
        [[cellView layer] setBackgroundColor:[color CGColor]];
      }
    }
    else
    {
      /* No cell-specific color, clear layer background so row background shows through */
      if ([cellView wantsLayer])
      {
        [[cellView layer] setBackgroundColor:nil];
      }
    }
  }
  else
  {
    /* Row is selected, clear layer background so selection highlight shows */
    if ([cellView wantsLayer])
    {
      [[cellView layer] setBackgroundColor:nil];
    }
  }

  /* Foreground color - hierarchy: L:C > :C > L:0 */
  char* fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, col);
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", 0, col);  /* Per-column */
  if (!fgcolor)
    fgcolor = iupAttribGetId2(ih, "FGCOLOR", lin, 0);  /* Per-row */

  if (fgcolor && *fgcolor)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(fgcolor, &r, &g, &b))
    {
      NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
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
  /* lin and col are 1-based IUP indices */

  char* font = iupAttribGetId2(ih, "FONT", lin, col);
  if (!font)
    font = iupAttribGetId2(ih, "FONT", 0, col);  /* Per-column */
  if (!font)
    font = iupAttribGetId2(ih, "FONT", lin, 0);  /* Per-row */

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
    /* Reset to default font when no FONT attribute found */
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
      /* Use system font as final fallback */
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

/* Override headerRectOfColumn to make the last column stretch to fill remaining space */
- (NSRect)headerRectOfColumn:(NSInteger)column
{
  NSRect rect = [super headerRectOfColumn:column];
  NSTableView* tableView = [self tableView];

  if (!tableView)
    return rect;

  NSInteger numColumns = [tableView numberOfColumns];

  /* If this is the last column, extend it ONLY if no explicit width was set */
  if (column == numColumns - 1)
  {
    /* Get Ihandle from associated object */
    Ihandle* ih = (Ihandle*)objc_getAssociatedObject(tableView, "_IUPCOCOA_IHANDLE");
    if (ih)
    {
      /* Check if last column has explicit width set OR if stretching is disabled */
      const char* last_col_width_set = iupAttribGet(ih, "_IUP_TABLE_LAST_COL_WIDTH_SET");
      if ((last_col_width_set && iupStrBoolean(last_col_width_set)) || !ih->data->stretch_last)
      {
        /* Explicit width set or stretching disabled */
        return rect;
      }
    }

    CGFloat headerWidth = NSWidth([self bounds]);
    CGFloat rectMaxX = NSMaxX(rect);

    /* Extend the header rect to fill remaining space */
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

/* Override mouseDown to capture clicked column */
- (void)mouseDown:(NSEvent*)event
{
  /* Get the column that was clicked BEFORE calling super, which changes selection */
  NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
  NSInteger clickedCol = [self columnAtPoint:point];
  NSInteger clickedRow = [self rowAtPoint:point];

  /* Save old focused cell for redraw */
  IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
  int old_focused_col = 0;
  int old_focused_row = 0;
  if (table_data)
  {
    old_focused_col = table_data->current_col;
    old_focused_row = table_data->current_row;
  }

  /* Store the clicked column in our data structure so tableViewSelectionDidChange can use it */
  if (table_data)
  {
    /* Store as 1-based (IUP convention) */
    table_data->current_col = (clickedCol >= 0) ? (int)(clickedCol + 1) : 1;
    table_data->current_row = (clickedRow >= 0) ? (int)(clickedRow + 1) : 0;
  }

  /* If column changed but row stayed the same, we need to reload cells for focus rectangle
   * AND trigger ENTERITEM_CB since tableViewSelectionDidChange won't be called */
  if (table_data && old_focused_row == table_data->current_row && old_focused_col != table_data->current_col)
  {
    /* Same row, different column - reload just the old and new focused cells */
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

    /* Trigger ENTERITEM_CB since we changed column within the same row */
    IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
    if (enteritem_cb)
    {
      enteritem_cb(ih, table_data->current_row, table_data->current_col);
    }
  }

  /* Let NSTableView handle the click normally */
  [super mouseDown:event];
}

/* Override frameOfCellAtColumn to make last column cells stretch to fill remaining space */
- (NSRect)frameOfCellAtColumn:(NSInteger)column row:(NSInteger)row
{
  NSRect rect = [super frameOfCellAtColumn:column row:row];

  NSInteger numColumns = [self numberOfColumns];

  /* If this is the last column, extend it ONLY if no explicit width was set */
  if (column == numColumns - 1)
  {
    /* Get Ihandle from associated object */
    Ihandle* table_ih = (Ihandle*)objc_getAssociatedObject(self, "_IUPCOCOA_IHANDLE");
    if (table_ih)
    {
      /* Check if last column has explicit width set OR if stretching is disabled */
      const char* last_col_width_set = iupAttribGet(table_ih, "_IUP_TABLE_LAST_COL_WIDTH_SET");
      if ((last_col_width_set && iupStrBoolean(last_col_width_set)) || !table_ih->data->stretch_last)
      {
        /* Explicit width set or stretching disabled */
        return rect;
      }
    }

    /* Get scroll view and use documentVisibleRect for consistent width */
    NSScrollView* scrollView = [self enclosingScrollView];
    CGFloat tableWidth = scrollView ? NSWidth(scrollView.documentVisibleRect) : NSWidth([self visibleRect]);
    CGFloat rectMaxX = NSMaxX(rect);

    /* Extend the cell rect to fill remaining space */
    if (rectMaxX < tableWidth)
    {
      rect.size.width += (tableWidth - rectMaxX);
    }
  }

  return rect;
}

/* Override performKeyEquivalent to handle Cmd+C and Cmd+V shortcuts */
- (BOOL)performKeyEquivalent:(NSEvent*)event
{
  NSString* chars = [event charactersIgnoringModifiers];
  if ([chars length] == 0)
    return [super performKeyEquivalent:event];

  unichar ch = [chars characterAtIndex:0];
  NSUInteger modifiers = [event modifierFlags];

  /* Handle Cmd+C */
  if ((modifiers & NSEventModifierFlagCommand) && (ch == 'c' || ch == 'C'))
  {
    /* Use tracked column from our data structure */
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
    return YES;  /* We handled the event */
  }

  /* Handle Cmd+V */
  if ((modifiers & NSEventModifierFlagCommand) && (ch == 'v' || ch == 'V'))
  {
    /* Use tracked column from our data structure */
    IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
    NSInteger selectedRow = [self selectedRow];
    int selectedColumn = (table_data && table_data->current_col > 0) ? table_data->current_col - 1 : 0;

    if (selectedRow >= 0 && selectedColumn >= 0)
    {
      /* Check if cell is editable */
      int col_1based = selectedColumn + 1;
      char name[50];
      sprintf(name, "EDITABLE%d", col_1based);
      char* editable = iupAttribGet(ih, name);
      if (!editable)
        editable = iupAttribGet(ih, "EDITABLE");

      if (iupStrBoolean(editable))
      {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        NSString* text = [pasteboard stringForType:NSPasteboardTypeString];
        if (text)
        {
          const char* new_value = [text UTF8String];

          /* Get old value for comparison - must copy before it gets modified */
          char* old_value_ptr = iupdrvTableGetCellValue(ih, (int)selectedRow + 1, col_1based);
          char* old_value = old_value_ptr ? iupStrDup(old_value_ptr) : NULL;

          cocoaTableSetCellValue(ih, (int)selectedRow, selectedColumn, new_value);

          /* Reload cell */
          NSIndexSet* rowSet = [NSIndexSet indexSetWithIndex:selectedRow];
          NSIndexSet* colSet = [NSIndexSet indexSetWithIndex:selectedColumn];
          [self reloadDataForRowIndexes:rowSet columnIndexes:colSet];

          /* Call VALUECHANGED_CB only if text actually changed */
          int text_changed = 0;
          if (!old_value && new_value && *new_value)
            text_changed = 1;  /* NULL -> non-empty */
          else if (old_value && !new_value)
            text_changed = 1;  /* non-empty -> NULL */
          else if (old_value && new_value && strcmp(old_value, new_value) != 0)
            text_changed = 1;  /* different text */

          if (text_changed)
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
    return YES;  /* We handled the event */
  }

  /* Let default handling process other key equivalents */
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

  /* Handle Tab/Shift+Tab for column navigation */
  if (ch == NSTabCharacter || ch == NSBackTabCharacter)
  {
    /* Save old focused cell before Tab moves focus */
    int old_focused_col = table_data ? table_data->current_col : 0;
    int old_focused_row = table_data ? table_data->current_row : 0;
    NSInteger currentRow = [self selectedRow];

    /* Let NSTableView handle Tab navigation first */
    [super keyDown:event];

    /* Now update our tracked column based on Tab direction */
    if (table_data && currentRow >= 0)
    {
      int num_cols = ih->data->num_col;

      if (ch == NSTabCharacter)
      {
        /* Tab: Move to next column */
        table_data->current_col++;
        if (table_data->current_col > num_cols)
          table_data->current_col = 1;
      }
      else
      {
        /* Shift+Tab: Move to previous column */
        table_data->current_col--;
        if (table_data->current_col < 1)
          table_data->current_col = num_cols;
      }

      /* Check if row changed */
      NSInteger newRow = [self selectedRow];
      BOOL row_changed = (newRow != currentRow);

      /* Reload cells to update focus rectangle */
      if (row_changed)
      {
        /* Row changed, reload both old and new rows */
        NSMutableIndexSet* rowsToReload = [NSMutableIndexSet indexSet];
        if (currentRow >= 0)
          [rowsToReload addIndex:currentRow];
        if (newRow >= 0)
          [rowsToReload addIndex:newRow];

        NSIndexSet* allCols = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, num_cols)];
        [self reloadDataForRowIndexes:rowsToReload columnIndexes:allCols];
      }
      else
      {
        /* Same row, different column, reload just the affected cells */
        NSMutableIndexSet* colsToReload = [NSMutableIndexSet indexSet];
        if (old_focused_col > 0)
          [colsToReload addIndex:(old_focused_col - 1)];
        if (table_data->current_col > 0)
          [colsToReload addIndex:(table_data->current_col - 1)];

        if ([colsToReload count] > 0 && currentRow >= 0)
        {
          NSIndexSet* rowSet = [NSIndexSet indexSetWithIndex:currentRow];
          [self reloadDataForRowIndexes:rowSet columnIndexes:colsToReload];
        }
      }

      /* Trigger ENTERITEM_CB */
      IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
      if (enteritem_cb)
      {
        enteritem_cb(ih, (int)newRow + 1, table_data->current_col);
      }
    }
    return;
  }

  /* Handle Enter key */
  if (ch == NSCarriageReturnCharacter || ch == NSEnterCharacter)
  {
    IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
    NSInteger selectedRow = [self selectedRow];
    int col_1based = (table_data && table_data->current_col > 0) ? table_data->current_col : 1;
    NSInteger selectedColumn = col_1based - 1;  /* Convert to 0-based */

    if (selectedRow >= 0 && selectedColumn >= 0)
    {
      char name[50];
      sprintf(name, "EDITABLE%d", col_1based);
      char* editable_str = iupAttribGet(ih, name);
      if (!editable_str)
        editable_str = iupAttribGet(ih, "EDITABLE");

      if (iupStrBoolean(editable_str))
      {
        NSTableColumn* column = [[self tableColumns] objectAtIndex:selectedColumn];
        if (column)
        {
          /* Call EDITBEGIN_CB before starting edit */
          int lin = (int)selectedRow + 1;
          IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
          if (editbegin_cb)
          {
            int ret = editbegin_cb(ih, lin, col_1based);
            if (ret == IUP_IGNORE)
              return;
          }

          /* Store edit position for EndEditing */
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

  /* Handle Left/Right arrow keys for column navigation */
  if (table_data)
  {
    int num_cols = ih->data->num_col;
    BOOL col_changed = NO;
    int old_focused_col = table_data->current_col;
    NSInteger currentRow = [self selectedRow];

    if (ch == NSLeftArrowFunctionKey)
    {
      /* Move to previous column */
      if (table_data->current_col > 1)
      {
        table_data->current_col--;
        col_changed = YES;
      }
    }
    else if (ch == NSRightArrowFunctionKey)
    {
      /* Move to next column */
      if (table_data->current_col < num_cols)
      {
        table_data->current_col++;
        col_changed = YES;
      }
    }

    /* If column changed, reload cells to update focus rectangle */
    if (col_changed && currentRow >= 0)
    {
      NSMutableIndexSet* colsToReload = [NSMutableIndexSet indexSet];
      if (old_focused_col > 0)
        [colsToReload addIndex:(old_focused_col - 1)];  /* Old focused column */
      if (table_data->current_col > 0)
        [colsToReload addIndex:(table_data->current_col - 1)];  /* New focused column */

      if ([colsToReload count] > 0)
      {
        NSIndexSet* rowSet = [NSIndexSet indexSetWithIndex:currentRow];
        [self reloadDataForRowIndexes:rowSet columnIndexes:colsToReload];
      }

      /* Trigger ENTERITEM_CB */
      IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
      if (enteritem_cb)
      {
        enteritem_cb(ih, (int)currentRow + 1, table_data->current_col);
      }
      /* Don't call super, we've handled the arrow key */
      return;
    }
  }

  /* Let NSTableView handle Up/Down arrow keys */
  [super keyDown:event];
}

- (BOOL)becomeFirstResponder
{
  BOOL result = [super becomeFirstResponder];
  if (result)
  {
    /* Redraw focused cell to show FOCUSRECT */
    IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
    if (table_data && table_data->current_row > 0 && table_data->current_col > 0)
    {
      NSInteger row_0based = table_data->current_row - 1;
      NSInteger col_0based = table_data->current_col - 1;

      if (row_0based >= 0 && row_0based < [self numberOfRows] &&
          col_0based >= 0 && col_0based < [self numberOfColumns])
      {
        NSIndexSet* rowIndexes = [NSIndexSet indexSetWithIndex:row_0based];
        NSIndexSet* colIndexes = [NSIndexSet indexSetWithIndex:col_0based];
        [self reloadDataForRowIndexes:rowIndexes columnIndexes:colIndexes];
      }
    }
  }
  return result;
}

- (BOOL)resignFirstResponder
{
  /* Redraw focused cell to remove FOCUSRECT */
  IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
  if (table_data && table_data->current_row > 0 && table_data->current_col > 0)
  {
    NSInteger row_0based = table_data->current_row - 1;
    NSInteger col_0based = table_data->current_col - 1;

    if (row_0based >= 0 && row_0based < [self numberOfRows] &&
        col_0based >= 0 && col_0based < [self numberOfColumns])
    {
      NSIndexSet* rowIndexes = [NSIndexSet indexSetWithIndex:row_0based];
      NSIndexSet* colIndexes = [NSIndexSet indexSetWithIndex:col_0based];
      [self reloadDataForRowIndexes:rowIndexes columnIndexes:colIndexes];
    }
  }
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

/* Sorting support */
- (void)tableView:(NSTableView*)tableView sortDescriptorsDidChange:(NSArray*)oldDescriptors
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (!table_data)
    return;

  NSArray* newDescriptors = [tableView sortDescriptors];
  if ([newDescriptors count] == 0)
    return;

  NSSortDescriptor* descriptor = [newDescriptors objectAtIndex:0];
  NSString* key = [descriptor key];
  int col_index = [key intValue];

  /* Check if user wants to handle sorting */
  IFni sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
  int sort_result = IUP_DEFAULT;

  if (sort_cb)
    sort_result = sort_cb(ih, col_index + 1);  /* Convert to 1-based */

  /* If callback returned DEFAULT, perform automatic sorting */
  /* If callback returned IGNORE, user handled sorting themselves */
  if (sort_result == IUP_DEFAULT)
  {
    /* Perform automatic internal sorting */
    if (!table_data->is_virtual_mode)
    {
      /* In non-virtual mode, sort internal data array */
      [table_data->data_array sortUsingComparator:^NSComparisonResult(NSMutableArray* row1, NSMutableArray* row2) {
        NSString* val1 = (col_index < [row1 count]) ? [row1 objectAtIndex:col_index] : @"";
        NSString* val2 = (col_index < [row2 count]) ? [row2 objectAtIndex:col_index] : @"";

        if ([descriptor ascending])
          return [val1 compare:val2];
        else
          return [val2 compare:val1];
      }];

      [tableView reloadData];
    }
  }
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
  /* Only draw custom background if row is NOT selected */
  if (customBackgroundColor && !self.selected)
  {
    [customBackgroundColor setFill];
    NSRectFill(dirtyRect);
  }
  else if (!self.selected)
  {
    /* Not selected, no custom color - use default */
    [super drawBackgroundInRect:dirtyRect];
  }
}

- (void)drawSelectionInRect:(NSRect)dirtyRect
{
  /* Draw selection highlight when row is selected */
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

    /* Register for NSControl text editing notifications */
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

/* View-based table: create cell view */
- (NSView*)tableView:(NSTableView*)tableView
    viewForTableColumn:(NSTableColumn*)tableColumn
    row:(NSInteger)row
{
  /* Reuse cell views with identifier */
  NSString* identifier = [tableColumn identifier];
  IupCocoaTableCellView* cellView = (IupCocoaTableCellView*)[tableView makeViewWithIdentifier:identifier owner:self];

  if (!cellView)
  {
    cellView = [[IupCocoaTableCellView alloc] initWithFrame:NSZeroRect];
    [cellView setIdentifier:identifier];
    [cellView setIh:ih];

    /* Create text field, positioned via layout method */
    NSTextField* textField = [[NSTextField alloc] initWithFrame:NSZeroRect];
    [textField setBordered:NO];
    [textField setDrawsBackground:NO];
    [textField setBezeled:NO];
    [[textField cell] setLineBreakMode:NSLineBreakByTruncatingTail];
    [textField setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

    [cellView setTextField:textField];
    [cellView addSubview:textField];
    [textField release];

    [cellView autorelease];
  }

  /* Get cell value */
  NSInteger col = [identifier integerValue];
  NSString* value = cocoaTableGetCellValue(ih, (int)row, (int)col);
  cellView.textField.stringValue = value;

  /* Check if cell is editable - convert to 1-based */
  int col_1based = (int)col + 1;
  char name[50];
  sprintf(name, "EDITABLE%d", col_1based);
  char* editable_str = iupAttribGet(ih, name);
  if (!editable_str)
    editable_str = iupAttribGet(ih, "EDITABLE");

  BOOL editable = iupStrBoolean(editable_str);
  [cellView.textField setEditable:editable];

  /* Set delegate every time due to cell reuse */
  [cellView.textField setDelegate:self];

  /* Apply column alignment - MUST be set every time due to cell reuse */
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  NSNumber* colKey = @(col);
  NSValue* colInfoValue = [table_data->column_info objectForKey:colKey];

  NSTextAlignment alignment = NSTextAlignmentLeft; /* Default */

  if (colInfoValue)
  {
    /* Column info exists - use it */
    IcocoaTableColumnInfo colInfo;
    [colInfoValue getValue:&colInfo];
    alignment = colInfo.alignment;
  }
  else
  {
    /* Column info doesn't exist yet (early rendering before NUMCOL completes).
     * Read alignment attribute directly from IUP attributes as fallback. */
    char align_name[50];
    sprintf(align_name, "ALIGNMENT%d", col_1based);
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

  /* Check if this cell is the focused cell (1-based coordinates in table_data) */
  int row_1based = (int)row + 1;
  BOOL isFocused = (table_data->current_row == row_1based && table_data->current_col == col_1based);

  /* Always explicitly set the flag (not just when focused) to handle cell reuse. */
  if ([cellView isFocusedCell] != isFocused)
  {
    [cellView setIsFocusedCell:isFocused];
    [cellView setNeedsDisplay:YES];  /* Only redraw if focus state actually changed */
  }

  /* Apply cell-specific colors and fonts (1-based indices for IUP, 0-based row for NSTableView) */
  BOOL isSelected = [tableView isRowSelected:row];
  cocoaTableApplyCellColors(ih, cellView, (int)row + 1, col_1based, isSelected);
  cocoaTableApplyCellFont(ih, cellView.textField, (int)row + 1, col_1based);

  /* Explicitly set textField frame based on CURRENT cell frame */
  NSRect cellFrame = [tableView frameOfCellAtColumn:col row:row];
  CGFloat textHeight = [cellView.textField intrinsicContentSize].height;
  CGFloat yOffset = floor((cellFrame.size.height - textHeight) / 2.0);
  /* Reuse the 'alignment' variable that was set earlier */
  NSRect textFrame;

  if (alignment == NSTextAlignmentCenter)
  {
    /* CENTER: Use full cell width */
    textFrame = NSMakeRect(0.0, yOffset, cellFrame.size.width, textHeight);
  }
  else
  {
    /* LEFT/RIGHT: Add 2px padding so text doesn't touch cell borders */
    textFrame = NSMakeRect(2.0, yOffset, cellFrame.size.width - 6.0, textHeight);
  }

  [cellView.textField setFrame:textFrame];

  return cellView;
}

/* Row view for custom row backgrounds */
- (NSTableRowView*)tableView:(NSTableView*)tableView rowViewForRow:(NSInteger)row
{
  NSColor* bgcolor = cocoaTableGetRowBackgroundColor(ih, (int)row);

  /* If no background color, return nil to use default row view */
  if (!bgcolor)
    return nil;

  /* Use row view reuse pattern with identifier */
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

/* Selection changed callback */
- (void)tableViewSelectionDidChange:(NSNotification*)notification
{
  NSTableView* tableView = [notification object];
  NSInteger selectedRow = [tableView selectedRow];

  /* Get the tracked column from our data structure.
   * This was set by mouseDown when the user clicked, or by keyDown when using arrow keys.
   * NSTableView doesn't track "current column" in row-selection mode, so we track it manually. */
  IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
  int col = 1;  /* Default to first column */
  if (table_data && table_data->current_col > 0)
    col = table_data->current_col;

  /* Save previous focused cell BEFORE updating current */
  int old_focused_row = 0;
  int old_focused_col = 0;
  if (table_data)
  {
    old_focused_row = table_data->current_row;
    old_focused_col = table_data->current_col;
  }

  /* Update current row */
  if (table_data)
    table_data->current_row = (int)(selectedRow + 1);

  /* Reload affected rows and cells. Do this FIRST before setting selection */
  if (table_data)
  {
    NSMutableIndexSet* rowsToReload = [NSMutableIndexSet indexSet];

    /* Reload previous selected row to restore custom backgrounds */
    if (table_data->previous_selected_row >= 0 && table_data->previous_selected_row < [tableView numberOfRows])
      [rowsToReload addIndex:table_data->previous_selected_row];

    /* Reload old and new focused cells for dashed border */
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

    /* Perform reload */
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

    /* Update tracking variables */
    table_data->previous_selected_row = (int)selectedRow;
    table_data->previous_focused_row = old_focused_row;
    table_data->previous_focused_col = old_focused_col;
  }

  /* Trigger ENTERITEM_CB callback with correct row and column */
  IFnii enteritem_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (enteritem_cb)
  {
    enteritem_cb(ih, (int)selectedRow + 1, col);
  }
}

/* Control cell editing based on EDITABLE attribute */
- (BOOL)tableView:(NSTableView*)tableView shouldEditTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row
{
  /* Check if the cell is editable */
  NSInteger colIndex = [[tableView tableColumns] indexOfObject:tableColumn];
  int col_1based = (int)colIndex + 1;

  /* Check column-specific EDITABLE attribute */
  char name[50];
  sprintf(name, "EDITABLE%d", col_1based);
  char* editable_str = iupAttribGet(ih, name);
  if (!editable_str)
    editable_str = iupAttribGet(ih, "EDITABLE");

  if (!iupStrBoolean(editable_str))
  {
    return NO;
  }

  /* Call EDITBEGIN_CB here - allows blocking the edit */
  int lin = (int)row + 1;
  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    int ret = editbegin_cb(ih, lin, col_1based);
    if (ret == IUP_IGNORE)
    {
      return NO;  /* Block editing */
    }
  }

  /* Store edit position */
  objc_setAssociatedObject(tableView, &kEditingRowKey, @(row), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditingColKey, @(colIndex), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditEndedKey, @(NO), OBJC_ASSOCIATION_RETAIN);  /* Reset flag for new edit */

  return YES;
}

/* Action triggered when user double-clicks on table */
- (void)tableViewAction:(id)sender
{
  NSTableView* tableView = (NSTableView*)sender;

  /* Get the currently selected row and column */
  NSInteger clickedRow = [tableView clickedRow];
  NSInteger clickedCol = [tableView clickedColumn];

  /* Convert to 1-based indices (IUP convention) */
  int lin = (clickedRow >= 0) ? (int)(clickedRow + 1) : 0;
  int col = (clickedCol >= 0) ? (int)(clickedCol + 1) : 0;

  /* Call CLICK_CB callback if registered */
  IFniis click_cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
  if (click_cb && lin > 0 && col > 0)
  {
    char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;

    /* Get current event to extract modifier keys and button state */
    NSEvent* currentEvent = [NSApp currentEvent];
    if (currentEvent)
      iupcocoaButtonKeySetStatus(currentEvent, status);

    click_cb(ih, lin, col, status);
  }

  /* Check if cell is editable and start editing */
  if (clickedRow >= 0 && clickedCol >= 0)
  {
    char name[50];
    sprintf(name, "EDITABLE%d", col);
    char* editable_str = iupAttribGet(ih, name);
    if (!editable_str)
      editable_str = iupAttribGet(ih, "EDITABLE");

    if (iupStrBoolean(editable_str))
    {
      /* Call EDITBEGIN_CB before starting edit */
      IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
      if (editbegin_cb)
      {
        int ret = editbegin_cb(ih, lin, col);
        if (ret == IUP_IGNORE)
          return;
      }

      /* Store edit position for EndEditing */
      objc_setAssociatedObject(tableView, &kEditingRowKey, @(clickedRow), OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditingColKey, @(clickedCol), OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditEndedKey, @(NO), OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditBeginCalledKey, @(YES), OBJC_ASSOCIATION_RETAIN);

      /* Start editing programmatically */
      [tableView editColumn:clickedCol row:clickedRow withEvent:nil select:YES];
    }
  }
}

/* NSControlTextEditingDelegate methods - these receive the NSControl directly */
- (BOOL)control:(NSControl*)control textShouldBeginEditing:(NSText*)fieldEditor
{
  /* This method is reliably called for view-based tables when editing starts */
  NSTableView* tableView = cocoaTableGetTableView(ih);

  /* Check if we already handled this in tableView:shouldEditTableColumn:row: */
  NSNumber* existingRow = objc_getAssociatedObject(tableView, &kEditingRowKey);
  if (existingRow)
      return YES;

  NSInteger row = [tableView rowForView:control];
  NSInteger col = [tableView columnForView:control];

  /* Fallback: If rowForView failed (view hierarchy in flux), try to use tracked selection */
  if (row < 0 || col < 0) {
      IcocoaTableData* table_data = ICOCOA_TABLE_DATA(ih);
      if (table_data && table_data->current_row > 0 && table_data->current_col > 0) {
          row = table_data->current_row - 1;
          col = table_data->current_col - 1;
      }
  }

  /* Valid row/col? */
  if (row < 0 || col < 0) return YES;

  int lin = (int)row + 1;
  int col_1based = (int)col + 1;

  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    /* If callback returns IGNORE, return NO to prevent editing */
    if (editbegin_cb(ih, lin, col_1based) == IUP_IGNORE)
      return NO;
  }

  /* Store context for EndEditing */
  objc_setAssociatedObject(tableView, &kEditingRowKey, @(row), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditingColKey, @(col), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditEndedKey, @(NO), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditBeginCalledKey, @(YES), OBJC_ASSOCIATION_RETAIN);

  return YES;
}

- (BOOL)control:(NSControl*)control textShouldEndEditing:(NSText*)fieldEditor
{
  /* We handle validation and callbacks in controlTextDidEndEditing: */
  return YES;
}

/* Intercept Escape key to handle edit cancellation */
- (BOOL)control:(NSControl*)control textView:(NSTextView*)textView doCommandBySelector:(SEL)commandSelector
{
  if (commandSelector == @selector(cancelOperation:))
  {
    NSTableView* tableView = cocoaTableGetTableView(ih);
    NSTextField* textField = (NSTextField*)control;

    /* Get row/col for this edit */
    NSInteger row = [tableView rowForView:textField];
    NSInteger col = [tableView columnForView:textField];

    /* Fallback to stored keys */
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

      /* Get current value */
      NSString* currentValue = [textField stringValue];
      const char* value = [currentValue UTF8String];

      /* Only call EDITEND_CB if EDITBEGIN_CB was previously called */
      NSNumber* editBeginCalled = objc_getAssociatedObject(tableView, &kEditBeginCalledKey);

      if (editBeginCalled && [editBeginCalled boolValue])
      {
        /* Call EDITEND_CB with apply=0 (cancelled) */
        IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
        if (editend_cb)
          editend_cb(ih, lin, col_1based, (char*)value, 0);
      }

      /* Mark edit as ended to prevent duplicate callback */
      objc_setAssociatedObject(tableView, &kEditEndedKey, @(YES), OBJC_ASSOCIATION_RETAIN);

      /* Clear keys */
      objc_setAssociatedObject(tableView, &kEditingRowKey, nil, OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditingColKey, nil, OBJC_ASSOCIATION_RETAIN);
      objc_setAssociatedObject(tableView, &kEditBeginCalledKey, nil, OBJC_ASSOCIATION_RETAIN);

      /* Revert to original value */
      NSString* original = cocoaTableGetCellValue(ih, (int)row, (int)col);
      if (original) [textField setStringValue:original];
    }

    /* Let Cocoa end the editing */
    return NO;  /* Let default behavior proceed */
  }

  return NO;  /* Let Cocoa handle other commands */
}

/* Notification methods */
- (void)controlTextDidBeginEditing:(NSNotification*)notification
{
  NSTableView* tableView = cocoaTableGetTableView(ih);

  /* Verify the text field belongs to this table */
  NSTextField* textField = [notification object];
  NSInteger row = [tableView rowForView:textField];
  NSInteger col = [tableView columnForView:textField];
  if (row < 0 || col < 0)
    return;

  /* Check if keys already set (by delegate methods). If so, we are good. */
  NSNumber* existingRow = objc_getAssociatedObject(tableView, &kEditingRowKey);
  if (existingRow)
      return;

  /* Call EDITBEGIN_CB */
  int lin = (int)row + 1;
  int col_1based = (int)col + 1;

  IFnii editbegin_cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (editbegin_cb)
  {
    editbegin_cb(ih, lin, col_1based);
  }

  /* Store keys for EndEditing */
  objc_setAssociatedObject(tableView, &kEditingRowKey, @(row), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditingColKey, @(col), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditEndedKey, @(NO), OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditBeginCalledKey, @(YES), OBJC_ASSOCIATION_RETAIN);
}

- (void)controlTextDidEndEditing:(NSNotification*)notification
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  NSTextField* textField = [notification object];

  /* Verify the text field belongs to this table */
  NSInteger row = [tableView rowForView:textField];
  NSInteger col = [tableView columnForView:textField];
  if (row < 0 || col < 0)
  {
    /* Text field is not in this table. If we had an active edit session that was
       interrupted (e.g. user clicked outside), clean up the editing state. */
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

  /* Check if we've already processed this edit end */
  NSNumber* editEnded = objc_getAssociatedObject(tableView, &kEditEndedKey);
  if (editEnded && [editEnded boolValue])
      return;

  /* Use stored keys as fallback if view lookup returned valid but different position
     (e.g. scrolling/recycling caused cell reuse) */
  NSNumber *nRow = objc_getAssociatedObject(tableView, &kEditingRowKey);
  NSNumber *nCol = objc_getAssociatedObject(tableView, &kEditingColKey);
  if (nRow && nCol) {
      row = [nRow integerValue];
      col = [nCol integerValue];
  }

  /* Mark that we've processed this edit end */
  objc_setAssociatedObject(tableView, &kEditEndedKey, @(YES), OBJC_ASSOCIATION_RETAIN);

  /* Clear keys immediately to reset state */
  objc_setAssociatedObject(tableView, &kEditingRowKey, nil, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditingColKey, nil, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, &kEditBeginCalledKey, nil, OBJC_ASSOCIATION_RETAIN);

  if (row < 0 || col < 0) return;

  int lin = (int)row + 1;
  int col_1based = (int)col + 1;

  /* Get value from the NSTextField directly */
  NSString* newValStr = [textField stringValue];
  const char* value = [newValStr UTF8String];

  /* Check cancellation (ESC key) */
  int apply = 1; /* Default: accepted */
  NSDictionary* userInfo = [notification userInfo];
  if (userInfo) {
      NSNumber* movement = [userInfo objectForKey:@"NSTextMovement"];
      /* NSCancelTextMovement = 0x17 (ESC key) */
      if (movement && [movement intValue] == 0x17)
          apply = 0;
  }

  /* Call EDITEND_CB - allow application to validate/reject edit */
  IFniisi editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb)
  {
    int ret = editend_cb(ih, lin, col_1based, (char*)value, apply);
    if (ret == IUP_IGNORE)
    {
      /* Application rejected the edit.
         Since this is 'DidEnd', the field editor is already closing.
         We revert the visual value to what it was before. */
       NSString* original = cocoaTableGetCellValue(ih, (int)row, (int)col);
       if (original) [textField setStringValue:original];
       return;
    }
  }

  /* Only update cell if edit was accepted (not cancelled with ESC) */
  if (apply)
  {
    /* Get old value for comparison - must copy before it gets modified */
    char* old_value_ptr = iupdrvTableGetCellValue(ih, lin, col_1based);
    char* old_value = old_value_ptr ? iupStrDup(old_value_ptr) : NULL;

    /* Update cell value in data model */
    cocoaTableSetCellValue(ih, (int)row, (int)col, value);

    /* Trigger VALUECHANGED_CB callback only if text actually changed */
    int text_changed = 0;
    if (!old_value && value && *value)
      text_changed = 1;  /* NULL -> non-empty */
    else if (old_value && !value)
      text_changed = 1;  /* non-empty -> NULL */
    else if (old_value && value && strcmp(old_value, value) != 0)
      text_changed = 1;  /* different text */

    if (text_changed)
    {
      IFnii valuechanged_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
      if (valuechanged_cb)
        valuechanged_cb(ih, lin, col_1based);
    }

    if (old_value)
      free(old_value);
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

    /* Remove all existing columns */
    NSArray* columns = [tableView tableColumns];
    for (NSTableColumn* col in columns)
    {
      [tableView removeTableColumn:col];
    }

    /* Create new columns */
    for (int i = 0; i < num_col; i++)
    {
      NSString* identifier = [NSString stringWithFormat:@"%d", i];
      NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:identifier];

      [column.headerCell setStringValue:[NSString stringWithFormat:@"Col %d", i + 1]];

      /* Read ALIGNMENTn attribute to apply to entire column (header + cells) */
      char align_name[50];
      sprintf(align_name, "ALIGNMENT%d", i + 1);
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

      /* Apply alignment to header cell */
      [column.headerCell setAlignment:alignment];

      /* Check if this column has explicit width set (RASTERWIDTHn or WIDTHn) */
      char width_name[50];
      sprintf(width_name, "RASTERWIDTH%d", i + 1);
      char* width_str = iupAttribGet(ih, width_name);
      if (!width_str)
      {
        sprintf(width_name, "WIDTH%d", i + 1);
        width_str = iupAttribGet(ih, width_name);
      }

      /* Last column: stretch ONLY if no explicit width was set AND STRETCHLAST=YES */
      if (i == num_col - 1)
      {
        if (width_str || !ih->data->stretch_last)
        {
          /* Explicit width set or stretching disabled, don't stretch */
          iupAttribSet(ih, "_IUP_TABLE_LAST_COL_WIDTH_SET", "YES");

          if (ih->data->user_resize)
            [column setResizingMask:NSTableColumnUserResizingMask];  /* Fixed but user can resize */
          else
            [column setResizingMask:NSTableColumnNoResizing];  /* Fixed and locked */

          /* Set the explicit width if provided */
          if (width_str)
          {
            int width_val = 0;
            if (iupStrToInt(width_str, &width_val) && width_val > 0)
              [column setWidth:width_val];
          }
        }
        else
        {
          /* No explicit width and stretching enabled, stretch to fill */
          iupAttribSet(ih, "_IUP_TABLE_LAST_COL_WIDTH_SET", "NO");

          if (ih->data->user_resize)
            [column setResizingMask:NSTableColumnUserResizingMask | NSTableColumnAutoresizingMask];
          else
            [column setResizingMask:NSTableColumnAutoresizingMask];  /* Stretch but not user-resizable */
        }
      }
      else if (width_str)
      {
        /* Column has explicit width: FIXED size, no auto-resize, optionally user-resizable */
        if (ih->data->user_resize)
          [column setResizingMask:NSTableColumnUserResizingMask];  /* Fixed but user can resize */
        else
          [column setResizingMask:NSTableColumnNoResizing];  /* Fixed and locked */
      }
      else
      {
        /* Column without explicit width: Size to fit header initially, no auto-resize */
        /* Will auto-adjust when data is added via reloadData */
        [column setResizingMask:NSTableColumnNoResizing];
        [column sizeToFit];
      }

      /* Set up sort descriptor for sorting support */
      if (ih->data->sortable)
      {
        NSSortDescriptor* sortDescriptor = [[NSSortDescriptor alloc] initWithKey:identifier ascending:YES];
        [column setSortDescriptorPrototype:sortDescriptor];
        [sortDescriptor release];
      }

      [tableView addTableColumn:column];
      [column release];

      /* Store column info with correct alignment */
      IcocoaTableColumnInfo colInfo;
      colInfo.title = [[NSString stringWithFormat:@"Col %d", i + 1] retain];
      colInfo.width = 80.0;
      colInfo.alignment = alignment;  /* Reuse alignment from header setup */
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

  /* Update all columns to enable/disable sorting */
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

  /* Apply to all columns */
  NSArray* columns = [tableView tableColumns];
  NSUInteger col_count = [columns count];
  for (NSUInteger i = 0; i < col_count; i++)
  {
    NSTableColumn* column = [columns objectAtIndex:i];

    /* Last column always has autoresizing for stretch */
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

void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  if (num_col < 0)
    num_col = 0;

  ih->data->num_col = num_col;

  if (ih->handle)
    cocoaTableSetNumColAttrib(ih, iupStrReturnInt(num_col));
}

void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  if (num_lin < 0)
    num_lin = 0;

  ih->data->num_lin = num_lin;

  if (ih->handle)
    cocoaTableSetNumLinAttrib(ih, iupStrReturnInt(num_lin));
}

void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  iupdrvTableSetNumCol(ih, ih->data->num_col + 1);
}

void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  if (ih->data->num_col > 0)
    iupdrvTableSetNumCol(ih, ih->data->num_col - 1);
}

void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  iupdrvTableSetNumLin(ih, ih->data->num_lin + 1);
}

void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  if (ih->data->num_lin > 0)
    iupdrvTableSetNumLin(ih, ih->data->num_lin - 1);
}

/* ========================================================================= */
/* Driver Functions - Cell Operations                                       */
/* ========================================================================= */

void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (!table_data)
    return;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return;

  /* Store in internal data (convert to 0-based) */
  cocoaTableSetCellValue(ih, lin - 1, col - 1, value);

  /* Update the view */
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (tableView)
  {
    NSIndexSet* rowSet = [NSIndexSet indexSetWithIndex:(lin - 1)];
    NSIndexSet* colSet = [NSIndexSet indexSetWithIndex:(col - 1)];
    [tableView reloadDataForRowIndexes:rowSet columnIndexes:colSet];
  }
}

char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (!table_data)
    return NULL;

  if (lin < 1 || lin > ih->data->num_lin || col < 1 || col > ih->data->num_col)
    return NULL;

  /* Get from internal data (convert to 0-based) */
  NSString* value = cocoaTableGetCellValue(ih, lin - 1, col - 1);
  if (value && [value length] > 0)
  {
    return iupStrReturnStr([value UTF8String]);
  }

  return NULL;
}

/* ========================================================================= */
/* Driver Functions - Column Operations                                     */
/* ========================================================================= */

void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
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

    /* Calculate column width for columns without explicit width */
    int col_index = col - 1;

    /* Check if this column has explicit width */
    char width_name[50];
    sprintf(width_name, "RASTERWIDTH%d", col);
    char* width_str = iupAttribGet(ih, width_name);
    if (!width_str)
    {
      sprintf(width_name, "WIDTH%d", col);
      width_str = iupAttribGet(ih, width_name);
    }

    /* Only calculate width for columns without explicit width and not the last column (which stretches) */
    if (!width_str && col_index < ih->data->num_col - 1)
    {
      NSFont* font = [NSFont systemFontOfSize:[NSFont systemFontSize]];
      CGFloat calculated_width = cocoaTableCalculateColumnWidth(ih, col_index, font);
      [column setWidth:calculated_width];
    }
  }
}

char* iupdrvTableGetColTitle(Ihandle* ih, int col)
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

void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
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
    int col_index = col - 1;  /* 0-based */
    int num_cols = ih->data->num_col;

    /* Set the width */
    [column setWidth:width];

    /* Apply appropriate resizing mask */
    if (col_index == num_cols - 1)
    {
      /* Last column: mark as having explicit width set */
      iupAttribSet(ih, "_IUP_TABLE_LAST_COL_WIDTH_SET", "YES");

      /* Set FIXED size, no stretching */
      if (ih->data->user_resize)
        [column setResizingMask:NSTableColumnUserResizingMask];  /* Fixed but user can resize */
      else
        [column setResizingMask:NSTableColumnNoResizing];  /* Fixed and locked */
    }
    else
    {
      /* Non-last columns with explicit width: FIXED size, optionally user-resizable */
      if (ih->data->user_resize)
        [column setResizingMask:NSTableColumnUserResizingMask];  /* Fixed but user can resize */
      else
        [column setResizingMask:NSTableColumnNoResizing];  /* Fixed and locked */
    }
  }
}

int iupdrvTableGetColWidth(Ihandle* ih, int col)
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

void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
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
    table_data->current_col = col;

  [tableView scrollRowToVisible:row];
  [tableView scrollColumnToVisible:column];
  [tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
}

void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
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

void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
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

void iupdrvTableRedraw(Ihandle* ih)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (tableView)
    [tableView reloadData];
}

void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  NSTableView* tableView = cocoaTableGetTableView(ih);
  if (!tableView)
    return;

  if (show)
    [tableView setGridStyleMask:(NSTableViewSolidVerticalGridLineMask | NSTableViewSolidHorizontalGridLineMask)];
  else
    [tableView setGridStyleMask:NSTableViewGridNone];
}

int iupdrvTableGetBorderWidth(Ihandle* ih)
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

  /* Add a column */
  NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"test"];
  [column setWidth:100];
  [[column headerCell] setStringValue:@"Test"];
  [temp_table addTableColumn:column];

  /* Get row height */
  cocoa_table_row_height = (int)[temp_table rowHeight];

  if (cocoa_table_row_height <= 0)
  {
    /* Fallback: use font metrics */
    int charheight;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    cocoa_table_row_height = charheight + 4;
  }

  /* Measure header height */
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

int iupdrvTableGetRowHeight(Ihandle* ih)
{
  NSTableView* table_view = cocoaTableGetTableView(ih);

  /* If table is mapped, get actual row height */
  if (table_view)
  {
    CGFloat row_height = [table_view rowHeight];
    if (row_height > 0)
      return (int)row_height;
  }

  cocoaTableMeasureRowMetrics(ih);
  return cocoa_table_row_height;
}

int iupdrvTableGetHeaderHeight(Ihandle* ih)
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

void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
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
      /* Fallback for border */
      *w += sb_size + 2;
      *h += 2;
    }
  }
  else
  {
    /* Fallback */
    *w += sb_size + 2;
    *h += 2;
  }

  /* Add horizontal scrollbar height when VISIBLECOLUMNS causes it to appear */
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

  /* Initialize internal data structure */
  cocoaTableInitializeData(ih);

  /* Check for virtual mode */
  value = iupAttribGet(ih, "VIRTUALMODE");
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (iupStrBoolean(value))
    table_data->is_virtual_mode = YES;

  /* Create NSScrollView */
  NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
  [scrollView setHasVerticalScroller:YES];
  [scrollView setHasHorizontalScroller:NO];


  [scrollView setBorderType:NSBezelBorder];

  /* Create custom NSTableView with keyboard support */
  IupCocoaTableView* tableView = [[IupCocoaTableView alloc] initWithIhandle:ih];

  /* Set custom header view to eliminate empty space at end of headers */
  IupNarrowTableHeaderView* headerView = [[IupNarrowTableHeaderView alloc] init];
  [tableView setHeaderView:headerView];
  [headerView release];

  /* Remove intercell spacing */
  [tableView setIntercellSpacing:NSMakeSize(0, 0)];

  /* Configure table view */
  /* Only use native alternating row colors if ALTERNATECOLOR=YES and no custom colors are set */
  /* If custom EVENROWCOLOR/ODDROWCOLOR are defined, the cell rendering code will handle them */
  char* alternate_color = iupAttribGetStr(ih, "ALTERNATECOLOR");
  if (iupStrBoolean(alternate_color))
  {
    /* Check if custom row colors are defined */
    char* even_color = iupAttribGetStr(ih, "EVENROWCOLOR");
    char* odd_color = iupAttribGetStr(ih, "ODDROWCOLOR");

    /* Use native alternating colors only if no custom colors are specified */
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

  /* Set selection mode */
  char* selmode = iupAttribGetStr(ih, "SELECTIONMODE");
  if (!selmode)
    selmode = "SINGLE";  /* Default */

  if (iupStrEqualNoCase(selmode, "NONE"))
    [tableView setAllowsEmptySelection:NO];
  else if (iupStrEqualNoCase(selmode, "MULTIPLE"))
    [tableView setAllowsMultipleSelection:YES];
  else  /* SINGLE */
    [tableView setAllowsMultipleSelection:NO];

  /* Last column fills available space */
  /* Use UniformColumnAutoresizingStyle as the base, then override per-column */
  [tableView setColumnAutoresizingStyle:NSTableViewNoColumnAutoresizing];

  /* Create DataSource and Delegate */
  IupCocoaTableDataSource* dataSource = [[IupCocoaTableDataSource alloc] initWithIhandle:ih];
  IupCocoaTableDelegate* delegate = [[IupCocoaTableDelegate alloc] initWithIhandle:ih];

  [tableView setDataSource:dataSource];
  [tableView setDelegate:delegate];
  [tableView setDoubleAction:@selector(tableViewAction:)];
  [tableView setTarget:delegate];

  /* Store references */
  objc_setAssociatedObject(tableView, IUP_COCOA_TABLE_DATASOURCE_KEY, dataSource, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, IUP_COCOA_TABLE_DELEGATE_KEY, delegate, OBJC_ASSOCIATION_RETAIN);
  objc_setAssociatedObject(tableView, "_IUPCOCOA_IHANDLE", (void*)ih, OBJC_ASSOCIATION_ASSIGN);

  [dataSource release];
  [delegate release];

  /* Connect table to scroll view */
  [scrollView setDocumentView:tableView];
  [tableView release];

  /* Set handles BEFORE calling iupcocoaSetAssociatedViews */
  ih->handle = scrollView;
  iupAttribSet(ih, "_IUPCOCOA_TABLEVIEW", (char*)tableView);

  /* Set associated views so iupcocoaGetMainView returns the NSTableView for focus */
  iupcocoaSetAssociatedViews(ih, tableView, scrollView);

  /* Set SHOWGRID - must be after tableView is stored in attribute */
  iupdrvTableSetShowGrid(ih, iupAttribGetBoolean(ih, "SHOWGRID"));

  /* Configure initial number of columns from ih->data (set by core before mapping) */
  if (ih->data->num_col > 0)
    iupdrvTableSetNumCol(ih, ih->data->num_col);

  /* Configure initial number of rows from ih->data (set by core before mapping) */
  if (ih->data->num_lin > 0)
    iupdrvTableSetNumLin(ih, ih->data->num_lin);

  /* Add to parent */
  iupcocoaAddToParent(ih);

  /* Store target height for VISIBLELINES clamping in LayoutUpdate */
  int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
  if (visiblelines > 0)
  {
    int row_height = iupdrvTableGetRowHeight(ih);
    int header_height = iupdrvTableGetHeaderHeight(ih);
    int sb_size = iupdrvGetScrollbarSize();

    /* Only add horizontal scrollbar height if it will actually be visible */
    int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
    int need_horiz_sb = (visiblecolumns > 0 && ih->data->num_col > visiblecolumns);
    int horiz_sb_height = need_horiz_sb ? sb_size : 0;

    int target_height = header_height + (row_height * visiblelines) + horiz_sb_height + 2;
    objc_setAssociatedObject(scrollView, &kTargetHeightKey, @(target_height), OBJC_ASSOCIATION_RETAIN);
  }

  /* Store VISIBLECOLUMNS for width clamping in LayoutUpdate */
  int visiblecolumns = iupAttribGetInt(ih, "VISIBLECOLUMNS");
  if (visiblecolumns > 0)
    objc_setAssociatedObject(scrollView, &kVisibleColumnsKey, @(visiblecolumns), OBJC_ASSOCIATION_RETAIN);

  return IUP_NOERROR;
}

static void cocoaTableUnMapMethod(Ihandle* ih)
{
  /* Free internal data */
  cocoaTableFreeData(ih);

  /* Clear associated views */
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
  /* Table handles its own layout through NSTableView/NSScrollView */
  NSScrollView* scroll_view = (NSScrollView*)ih->handle;
  if (!scroll_view) return;

  NSView* parent_view = [scroll_view superview];
  if (!parent_view) return;

  int width = ih->currentwidth;
  int height = ih->currentheight;

  /* If VISIBLELINES is set, clamp height to target */
  NSNumber* targetHeightNum = objc_getAssociatedObject(scroll_view, &kTargetHeightKey);
  if (targetHeightNum)
  {
    int target_height = [targetHeightNum intValue];
    if (target_height > 0 && height > target_height)
      height = target_height;
  }

  /* If VISIBLECOLUMNS is set, clamp width to show exactly N columns */
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
            col_width = 80;  /* fallback to default */
          cols_width += col_width;
        }

        int sb_size = iupdrvGetScrollbarSize();

        /* Get actual border from scroll view */
        NSSize contentSz = [scroll_view contentSize];
        NSRect frameSz = [scroll_view frame];
        int border_w = (int)(NSWidth(frameSz) - contentSz.width);

        /* Only add vertical scrollbar width if it will actually be visible */
        int visiblelines = iupAttribGetInt(ih, "VISIBLELINES");
        int need_vert_sb = (visiblelines > 0 && ih->data->num_lin > visiblelines);
        int vert_sb_width = need_vert_sb ? sb_size : 0;

        /* Use ceiling to ensure we have enough space for columns */
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

    /* Control horizontal scrollbar visibility based on whether content is wider than available space */
    NSScrollView* scroll_view = (NSScrollView*)ih->handle;
    NSTableView* tableView = (NSTableView*)[scroll_view documentView];

    [scroll_view setHasHorizontalScroller:(total_width > documentVisibleRect.size.width ? YES : NO)];

    /* Update NSTableView frame to match the available content area width. */
    NSRect newDocumentVisibleRect = [scroll_view documentVisibleRect];
    NSRect tableFrame = [tableView frame];
    tableFrame.size.width = newDocumentVisibleRect.size.width;
    [tableView setFrame:tableFrame];
  }

  /* Force table reload after layout completes for NORMAL mode only. */
  /* Virtual mode cells are already reloaded constantly via scrolling */
  IcocoaTableData* table_data = cocoaTableGetData(ih);
  if (table_data && !table_data->is_virtual_mode)
  {
    NSTableView* tableView = (NSTableView*)[scroll_view documentView];
    if (tableView && [tableView isKindOfClass:[NSTableView class]])
    {
      [tableView reloadData];
    }
  }
}

/* ========================================================================= */
/* Class Registration                                                        */
/* ========================================================================= */

void iupdrvTableInitClass(Iclass* ic)
{
  /* Driver Dependent Class methods */
  ic->Map = cocoaTableMapMethod;
  ic->UnMap = cocoaTableUnMapMethod;
  ic->LayoutUpdate = cocoaTableLayoutUpdateMethod;

  iupClassRegisterAttribute(ic, "FONT", NULL, iupdrvSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NO_SAVE|IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, NULL, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FOCUSRECT", NULL, NULL, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);

  /* Replace core SET handlers to update native widget */
  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, cocoaTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, cocoaTableSetReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, cocoaTableSetUserResizeAttrib);
}
