## IupMatrix Attributes (all non-inheritable, with exceptions)

### General Attributes

**CURSOR**: Default cursor used by the matrix. The default cursor is a symbol that looks like a cross.
If you need to refer to this default cursor, use the name "IupMatrixCrossCursor".

**DROPIMAGE**: drop image name. Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
See also [IupImage](../elem/iup_image.md). By default an internal image will be used.

**FOCUSCELL**: Defines the current cell.
Two numbers in the "***L***:***C***" format, (***L***>0 and ***C***>0, a title cell cannot be the current cell).
Default: "1:1".

**FLAT**: removes the 3D appearance from the matrix.

[FLATSCROLLBAR](../attrib/iup_flatscrollbar.md): enable the flat scrollbars. Can be set only before map.
SCROLLBAR is set to NO. Can be Yes, Vertical or Horizontal. Default: not defined.

**HIDEFOCUS**: do not show the focus mark when drawing the matrix. Default is NO.

**HIDDENTEXTMARKS**: when a text is greater than cell space, it is normally cropped, but when set to YES a "..." mark will be added at the crop point to indicate that there is more text not visible.
Default: NO.

**HLCOLOR**: the overlay color for the selected cells. Default: TXTHLCOLOR global attribute.
If set to "" will only use the attenuation process.
The color is composited using HLCOLORALPHA attribute as alpha value (default is 128).

**ORIGIN**: Scroll the visible area to the given cell. Returns the cell at the top-left corner.
To scroll to a line or a column, use a value such as "L:*" or "*:C" (where L>0 and C>0).
L and C cannot be a non scrollable cell either.

**ORIGINOFFSET**: complements the ORIGIN attribute by specifying the drag offset of the top left cell.
Returns the current value. Has the format "X:Y" or "%d:%d" in C.
When changing this attribute must change also ORIGIN right after.

**READONLY**: disables the editing of all cells. EDITION_CB and VALUE_EDIT_CB will not be called anymore.
The L:C attribute will still be able to change the cell value.

**SHOWFILLVALUE**: enable the display of the numeric percentage in the cell when TYPE* is FILL.
Default: NO.

**TOGGLECENTERED**: center the toggle and use the cell value in place of TOGGLEVALUE*L:C*.
No text will be drawn.

**TOGGLEIMAGEON/TOGGLEIMAGEOFF**: toggle image name.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
See also [IupImage](../elem/iup_image.md). By default an internal image will be used.

**TYPECOLORINACTIVE**: when inactive the color of the cell for TYPE*=COLOR will be attenuated as everything else.
Default: Yes.

> 
>
> ------------------------------------------------------------------------

[ACTIVE](../attrib/iup_active.md), [EXPAND](../attrib/iup_expand.md), [FONT](../attrib/iup_font.md), [SCREENPOSITION](../attrib/iup_screenposition.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [WID](../attrib/iup_wid.md), [TIP](../attrib/iup_tip.md), [SIZE](../attrib/iup_size.md), [RASTERSIZE](../attrib/iup_rastersize.md), [ZORDER](../attrib/iup_zorder.md), [VISIBLE](../attrib/iup_visible.md): also accepted. 

### Cell Attributes (no redraw)

(These attributes are only updated in the display when you set the REDRAW attribute.)

***L*****:*C***: Text of the cell located in line L and column C, where L and C are integer numbers.\
***L*:0**: Title of line L.\
**0:*C***: Title of column C.\
**0:0**: Title of the area between the line and column titles.

These are valid only in normal mode.

**ALIGN*L*:*C***: Alignment of the cell value in line L and column C.
Values are in the format: "linalign:colalign", where linalign can be "ATOP", "ACENTER" or "ABOTTOM", and colalign can be "ALEFT", "ACENTER" or "ARIGHT".
Default will use ALIGNMENT* and LINEALIGMENT*.

**TYPE*L*:*C***: Type of the cell value in line L and column C.\
**TYPE*:*C***: Type of column C.\
**TYPE*L*:***: Type of line L.

It can be TEXT, COLOR, FILL, or IMAGE. When type is COLOR, the cell value is interpreted as a color and a rectangle with the color is drawn inside the cell instead of the text (the FGCOLOR of the cell is ignored).
When type is FILL, the cell value is interpreted as percentage and a rectangle showing the percentage in the FGCOLOR is drawn like in **IupGauge** and **IupProgressBar**.
When type is IMAGE, the cell value is interpreted as an image name, and if an image exists with that name is drawn (the name cannot be of a Windows resource or GTK stock image).
Only TEXT and IMAGE are affected by alignment attributes. Default: TEXT.

**BGCOLOR**: Background color of the matrix. (inheritable)\
**BGCOLOR*:*C***: Background color of column C.\
**BGCOLOR*L*:***: Background color of line L.\
**BGCOLOR*L*:*C***: Background color of the cell in line L and column C.

When more than one attribute are defined, the background color will be selected following this priority: BGCOLORL:C, BGCOLORL:*, BGCOLOR*:C, and last BGCOLOR. (L or C >= 0)\
Default BGCOLOR is the global attribute TXTBGCOLOR for cells and the parent's BGCOLOR for titles.\
Since the matrix control can be larger than the matrix itself, the empty area will always be filled with the parent's BGCOLOR.

**FGCOLOR**: Text color. (inheritable)\
**FGCOLOR*:*C***: Text color of column C.**\
FGCOLOR*L*:***: Text color of line L.\
**FGCOLOR*L*:*C***: Text color of the cell in line L and column C.

When more than one attribute are defined, the text color of a cell will be selected following this priority: FGCOLORL:C, FGCOLORL:*, FGCOLOR*:C, and last FGCOLOR. (L or C >= 0)\
Default FGCOLOR is the global attribute TXTFGCOLOR for cells or the global attribute DLGFGCOLOR for titles.

**[FONT](../attrib/iup_font.md)**: Character font of the text. (inheritable)\
**FONT*L*:***: Text font of the cells in line L.\
**FONT*:*C***: Text font of the cells in column C.\
**FONT*L*:*C***: Text font of the cell in line L and column C.

This attribute must be set before the control is showed.
It affects the calculation of the size of all the matrix cells.
The cell size is always calculated from the base FONT attribute.
FONTSTYLE*L***:***C* and FONTSIZE*L***:***C* can also be used to set FONT changing only the font style or size.

**FRAMECOLOR**: Sets the color to be used in the frame lines. (inheritable)\
**FRAMEVERTCOLOR*L:C***: Color of the vertical right frame line of the cell.
When not defined the FRAMEVERTCOLOR**:C* is used.
For a title column cell (col=0) defines right and left frames, except if FRAMETITLEVERTCOLOR*L***:***C*  is defined.
If value is "BGCOLOR" the frame line is not drawn.\
**FRAMEVERTCOLOR**:C***: same as FRAMEVERTCOLOR*L:C* but for all the cells of the column C.
When not defined the FRAMECOLORL:C is used.**\
FRAMEVERTCOLOR*L:****: same as FRAMEVERTCOLOR*L:C* but for all the cells of the line L.
When not defined the FRAMECOLOR*:C is used.**\
FRAMETITLEVERTCOLOR*L:******0***: color of the vertical left frame line of the cell.
When not defined the FRAMEVERTCOLOR*L:0*  is used. "L" can also be "*"**\
FRAMEHORIZCOLOR*L:C***: color of the horizontal bottom frame line of the cell.
When not defined the FRAMEHORIZCOLOR*L:** is used.
For a title line cell (lin=0) defines bottom and top frames, except if FRAMETITLEHORIZCOLOR*L***:***C*  is defined.
If value is "BGCOLOR" the frame line is not drawn.\
**FRAMEHORIZCOLOR*L:****: same as FRAMEHORIZCOLOR*L:C* but for all the cells of the line L.
When not defined the FRAMECOLORL:C is used.\
**FRAMEHORIZCOLOR**:C***: same as FRAMEHORIZCOLOR*L:C* but for all the cells of the column C.
When not defined the FRAMECOLORL:* is used.\
**FRAMETITLEHORIZCOLOR*0:C***: color of the horizontal top frame line of the cell.
When not defined the FRAMEHORIZCOLOR*0:C* is used. "C" can also be "*".

**FRAMETITLEHIGHLIGHT**: by default, the title cells will have a bright line at left and top to configure a raise appearance.
Can be Yes or No. Default: Yes.

**FRAMEBORDER**: show a fixed border (non scrollable) of 1 pixel around the matrix visible area using FRAMECOLOR.
It is drawn after the matrix cells are drawn.
Drawn only when the scrollbars are visible, and only up to matrix total size.
Default: No.

**RESIZEMATRIXCOLOR**: color used by the column resize feedback. Default: "102 102 102".

**TOGGLEVALUE*L*:*C*** : value of the toggle inside the cell.
The toggle is shown only if the DROPCHECK_CB returns IUP_CONTINUE for the cell.
When the toggle is interactively change the TOGGLEVALUE_CB callback is called.

**VALUE**: Allows setting or verifying the value of the current cell.
Is the same as obtaining the current cell line and column from FOCUSCELL attribute, and then using them to access the "L:C" attribute.
But when updated or retrieved during cell editing, the edit control will be updated or consulted instead of the matrix cell.
When retrieved inside the EDITION_CB callback when mode is 0, then the return value is the new value that will be updated in the cell.

------------------------------------------------------------------------

**CELL*L*:*C*** (read-only): Returns the displayed cell value.
Returns NULL if the cell does not exist, or it is not visible, or the element is not mapped.

**CELLBGCOLOR*L*:*C*** (read-only): Returns the actual cell background color, including lin and col variations, callback returned values, mark and active state modifications.
Returns NULL if the cell does not exist, or it is not visible, or the element is not mapped.

**CELLFGCOLOR*L*:*C*** (read-only): Returns the actual cell foreground color, including lin and col variations, callback returned values, mark state modifications.
Returns NULL if the cell does not exist, or it is not visible, or the element is not mapped.

**CELLFONT*L*:*C*** (read-only): Returns the actual cell font, including lin and col variations, callback returned values.
Returns NULL if the cell does not exist, or it is not visible, or the element is not mapped.

**CELL*TYPEL*:*C*** (read-only): Returns the actual cell type, including lin and col variations, callback returned values.
Returns NULL if the cell does not exist, or it is not visible, or the element is not mapped.

**CELLFRAMEHORIZCOLOR*L*:*C*** (read-only): Returns the actual cell frame horizontal color, including lin and col variations.
Returns NULL if the cell does not exist, or it is not visible, the element is not mapped, or the color is transparent.

**CELLFRAMEVERTCOLOR*L*:*C*** (read-only): Returns the actual cell frame vertical color, including lin and col variations.
Returns NULL if the cell does not exist, or it is not visible, the element is not mapped, or the color is transparent.

**CELLALIGNMENT*L*:*C*** (read-only): Returns the actual cell text aligment, including lin and col variations.
Returns NULL if the cell does not exist, or it is not visible, or the element is not mapped.

**CELLOFFSET*L*:*C*** (read-only): Returns the cell computed offset in pixels from the top-left corner of the matrix, in the format "XxY" or "%dx%d" in C.
Returns NULL if the cell does not exist, or it is not visible, or the element is not mapped.
It will only return a valid result if the cell has already been displayed.
They are similar to the parameters of the DRAW_CB callback, but they do NOT include the decorations.

**CELLSIZE*L*:*C*** (read-only): Returns the cell computed size in pixels, in the format "WxH" or "%dx%d" in C.
Returns NULL if the cell does not exist, or the element is not mapped.
It will only return a valid result if the cell has already been displayed.
They are similar to the parameters of the DRAW_CB callback, but they do NOT include the decorations.

### Column/Line Only Attributes (no redraw)

**ALIGNMENT*****C*** : Horizontal alignment of the cells in column C (C >= 0) for lines that are greater than 0.
Can be: "ALEFT", "ACENTER" or "ARIGHT". Default: "ALEFT" for C=0 and "ACENTER" for C>0.
Before checking the default value it will check the "**ALIGNMENT**" attribute value.
If the text does not fit in the cell, then the alignment is changed to ALEFT.

**ALIGNMENTLIN0:** Horizontal alignment of all the cells in line 0.
Default is "ACENTER".

**LINEALIGNMENT*L***: Vertical alignment of the cells in line L (L >= 0) for all columns.
Can be: "ATOP", "ACENTER" or "ABOTTOM". Default is "ACENTER".

**SORTSIGN*****C*** : Shows a sort sign (up or down arrow) in the column C (C >= 0) title.
Possible values: "UP", "DOWN" and "NO". Default: NO.

**SORTIMAGEDOWN/SORTIMAGEUP**: sort sign image name.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
See also [IupImage](../elem/iup_image.md). By default, an internal image will be used.

### Size Attributes

**LIMITEXPAND**: limit expansion to the maximum size that shows all cells.
This will set the MAXSIZE attribute to match the natural size of the matrix when all cells are visible.
When the scrollbars have *AUTOHIDE=Yes, the maximum size will not include the scrollbars.

**RESIZEMATRIX**: Defines if the width of a column can be interactively changed.
When this is possible, the user can change the size of a column by dragging the column title right border.
Possible values: "YES" or "NO". Default: "NO" (does not allow interactive width change).
The minimum size is 0 by default, the column is then hidden, but it can be controlled by the MINCOLWIDTHid and MINCOLWIDTHDEF attributes.

**RESIZEDRAG**: Resize the column while dragging.
By default, the column is resized only when the mouse button is released, the resize feedback is a simple vertical line.
Works only when RESIZEMATRIX=Yes. Default: NO.

**USETITLESIZE:** Use the title size to define the cell size if necessary. See WIDTHn and HEIGHTn.
Default: NO.

### Column Size Attributes

For all columns if WIDTHn is not defined, then RASTERWIDTHn is used.
If also not defined, then depending on the circumstances, a logic is used to find the column width.

If it is the title column (n=0), then if USETITLESIZE=YES or not in callback mode, it will search for the maximum width among the titles of all lines.
Finally if the conditions are not true or the maximum width of the column is 0, then the column of line titles is hidden.

If it is a regular column (n>0), then if USETITLESIZE=YES, then it will use the width of the title of the column.
Finally if the condition is not true or the width of the title of the column is 0, then the default value WIDTHDEF is used.

**RASTERWIDTHn**: Same as WIDTHn but in pixels. Has lower priority than WIDTHn.
The returned value is the actual computed size.

**WIDTHn**: Width of column n in SIZE units, where n is the number of the column (n>=0).
If the width value is 0, the column will not be shown on the screen.
It does not include the decoration size occupied by the frame lines.
The returned value is the actual computed size.

**WIDTHDEF**: Default column width in SIZE units. Not used for the title column.
Default: 80 (width corresponding to 20 characters).

**MINCOLWIDTHid**: when the column is interactively resized controls the minimum width of the given column.
If not defined MINCOLWIDTHDEF is used.

### Line Size Attributes

For all lines if HEIGHTn is not defined, then RASTERHEIGHTn is used.
If also not defined, then depending on the circumstances, a logic is used to find the line height.

If it is the title line (n=0), then if USETITLESIZE=YES or not in callback mode, it will search for the maximum height among the titles of all columns.
Finally, if the conditions are not true or the maximum height of the line is 0, then the line of column titles is hidden.

If it is a regular line (n>0), then if USETITLESIZE=YES, then it will use the height of the title of the line.
Finally, if the condition is not true or the height of the title of the line is 0, then the default value HEIGHTDEF is used.

**HEIGHTn**: Height of line n in SIZE units, where n is the number of the line (n>=0).
If the height value is 0, the line will not be shown on the screen.
It does not include the decoration size occupied by the frame lines.
The returned value is the actual computed size.

**HEIGHTDEF**: Default line height in SIZE units. Not used for the title line.
Default: 8 (height corresponding to 1 line).

**RASTERHEIGHTn**: Same as HEIGHTn but in pixels. Has lower priority than HEIGHTn.
The returned value is the actual computed size.

### Number of Cells Attributes

When lines or columns are added or removed the existing cell, line and column attributes are preserved, except custom application attributes.

**ADDCOL** (write-only): Adds a new column to the matrix after the specified column.
To insert a column at the top of the spreadsheet, value 0 must be used.
To add more than one column, use format "***C-C***", where the first number corresponds to the base column and the second number corresponds to the number of columns to be added.
It can be used in normal operation mode or in callback mode, but in callback mode will not update cell values this must be done by the application.
Can NOT add a title column. Ignored if set before map.

**ADDLIN** (write-only): Adds a new line to the matrix after the specified line.
To insert a line at the top of the spreadsheet, value 0 must be used.
To add more than one line, use format "***L-L***", where the first number corresponds to the base line and the second number corresponds to the number of lines to be added.
It can be used in normal operation mode or in callback mode, but in callback mode will not update cell values this must be done by the application.
Can NOT add a title line. Ignored if set before map.

**DELCOL** (write-only): Removes the given column from the matrix.
To remove more than one column, use format "***C-C***", where the first number corresponds to the base column and the second number corresponds to the number of columns to be removed.
It can be used in normal operation mode or in callback mode, but in callback mode will not update cell values this must be done by the application.
Can NOT remove a title column, ***C***>0. Ignored if set before map.

**DELLIN** (write-only): Removes the given line from the matrix.
To remove more than one line, use format "***L-L***", where the first number corresponds to the base line and the second number corresponds to the number of lines to be removed.
It can be used in normal operation mode or in callback mode, but in callback mode will not update cell values this must be done by the application.
Can NOT remove a title line, ***L***>0. Ignored if set before map.

**NUMCOL**: Defines the number of columns in the matrix. Must be an integer number.
Default: "0". It does not include the title column.
If changed after map will add empty cells or discard cells at the end.

**NUMCOL_VISIBLE**: When set defines the number of visible columns to be counted when calculating the **Natural** size, not counting the title column.
Not used elsewhere. The **Natural** size will always include the title column if any.
Can be greater than the actual number of columns, so room will be reserved for adding new columns without the need to resize the matrix.
Also, it will always use the first columns of the matrix, except if **NUMCOL_VISIBLE_LAST**=YES then it will use the last columns.
The remaining columns will be accessible only by using the scrollbar.
IMPORTANT: When retrieved returns the current number of visible columns, not including the non scrollable columns.
Default: "4".

**NUMCOL_NOSCROLL**: Number of columns that are non scrollable, not counting the title column.
Default: "0". It does not affect the NUMCOL_VISIBLE attribute behavior nor Natural size computation.
It will always use the first columns of the matrix.
The cells appearance will be the same of ordinary cells, and they can also receive the focus and be edited.
Must be less than the total number of columns.

**NUMLIN**: Defines the number of lines in the matrix. Must be an integer number.
Default: "0". It does not include the title line.
If changed after map will add empty cells or discard cells at the end.

**NUMLIN_VISIBLE**: When set defines the number of visible lines to be counted when calculating the **Natural** size, not counting the title line.
Not used elsewhere. The **Natural** size will always include the title line if any.
Can be greater than the actual number of lines, so room will be reserved for adding new lines without the need to resize the matrix.
Also, it will always use the first lines of the matrix, except if **NUMLIN_VISIBLE_LAST**=YES then it will use the last lines.
The remaining lines will be accessible only by using the scrollbar.
IMPORTANT: When retrieved returns the current number of visible lines, not including the non scrollable lines.
Default: "3".

**NUMLIN_NOSCROLL**: Number of lines that are non scrollable, not counting the title line.
Default: "0". It does not affect the NUMLIN_VISIBLE attribute behavior nor Natural size computation.
It will always use the first lines of the matrix.
The cells appearance will be the same of ordinary cells, and they can also receive the focus and be edited.  Must be less than the total number of lines.

**NOSCROLLASTITLE**: Non scrollable lines and columns to look and behave as title cells.
Can be Yes or No. Default: No.

### Mark Attributes

**MARKAREA**: Defines if the area to be **interactively** marked by the user must be continuous or not, valid only if MARKMULTIPLE=YES.
Possible values: "CONTINUOUS" or "NOT_CONTINUOUS". Default: "CONTINUOUS".

**MARKATTITLE**: a click at a title will mark a full line or a full column if they can be marked.
Default: "Yes".

**MARKMODE**: Defines the entity that can be marked: none, lines, columns, (lines **or** columns), and cells.
Possible values: "NO", "LIN", "COL", "LINCOL" or "CELL". Default: "NO" (no mark).

**MARK*L:C*** (no redraw): marks a cell, a line or a column depending on MARKMODE, and returns cell, line or column mark state also according to MARKMODE.
Can be "1" or "0". If MARKMODE=LIN,COL,LINCOL use 0 to mark only the other element (ex: "0:3" set/get for column 3).
Even when MARKMODE=LIN,COL,LINCOL you can specify a single cell address.

**MARKED**: String of '0' or '1' characters, informing which cells are marked (indicated by value '1').
Use NULL to clear all marks, returns NULL if no marks.
The format of this character vector depends on the value of the MARKMODE attribute: if its value is CELL, the vector will have NUMLIN x NUMCOL positions, corresponding to all the cells in the matrix starting with all the cells of the first line, then the second line and so on.
If its value is LIN, the vector will begin with letter 'L' and will have further NUMLIN positions, each one corresponding to a line in the matrix.
If its value is COL, the vector will begin with letter 'C' and will have further NUMCOL positions, each one corresponding to a column in the matrix.
If its value is LINCOL, the first letter, which can be either 'L' or 'C', will indicate which of the above formats is being used.
If you change the other mark attributes, the marked cells are cleared.
When setting the attribute the LIN and COL notation can be used even if MARKMODE=CELL.
MULTIPLE and AREA are NOT considered when setting MARKED or MARKL:C.

**MARKMULTIPLE**: Defines if more than one entity defined by MARKMODE can be **interactively** marked.
Possible values: "YES" or "NO". Default: "NO".

### Merge Attributes MERGE*L*:*C*: merge a range of cells starting from the given "lin:col" (in id), and ending at the given "lin:col" (in value). Title cells can also be merge but only among them, i.e. in the line of column titles (L=0) can only merge columns, and in the column of line titles (C=0) can only merge lines. The corner cell (0:0) can not be merged with any other cell. Only cells that are not already merged can be merged into a range. Returns if the given cell belongs to a merged range, can be "Yes" or "No".

**MERGESPLIT** (write-only): split a merged range. value is a cell "lin:col" than belongs to the range, any cell of the range can be used.

**MERGEDSTART*L*:*C***  (read-only): returns the start cell of the range given a cell that belongs to the range, any cell of the range can be used.

**MERGEDEND*L*:*C***  (read-only): returns the end cell of the range given a cell that belongs to the range, any cell of the range can be used.

### Action Attributes

**CLEARATTRIB** (write-only): Clear all cell attributes if ALL, all attributes except titles if CONTENTS, and all selected cell attributes if MARKED.
When ALL is specified, all lines and column attributes are also cleared.\
**CLEARATTRIB*L*:*C*** (write-only): Clear all cell attributes in an interval starting at the specified cell.
Its value defines the end cell in the "L:C" format, the default is the last cell.\
**CLEARATTRIB*L*:*** (write-only): the cell attributes in line L.
Its value defines a column inclusive interval in the "C1-C2" format. The default is 0 and the last column.
When a full line is specified, all line attributes are also cleared.\
**CLEARATTRIB*:*C*** *(write-only)*: the cell attributes in column C.
Its value defines a line inclusive interval in the "L1-L2" format. The default is 0 and the last line.
When a full column is specified, all column attributes are also cleared, including ALIGNMENT and SORTSIGN.

In all cases, attributes are set to NULL.
Only the attributes FONT*, BGCOLOR*, FGCOLOR*, FRAMEHORIZCOLOR*, FRAMEVERTCOLOR*, ALIGNMENTLIN0, LINEALIGNMENT*, ALIGNMENT* and SORTSIGN* are affected.
In callback mode will not call the user callbacks.

**CLEARVALUE** (write-only): Clear all values if ALL, all values except titles if CONTENTS, and all selected cell values if MARKED.\
**CLEARVALUE*L*:*C*** (write-only): Clear all values in an interval starting at the specified cell.
Its value defines the end cell in the "L:C" format, the default is the last cell.\
**CLEARVALUE*L*:*** (write-only): the values in line L.
Its value defines a column inclusive interval in the "C1-C2" format.
The default is 0 and the last column.\
**CLEARVALUE*:*C*** (write-only): the values in column C.
Its value defines a line inclusive interval in the "L1-L2" format.
The default is 0 and the last line.

In all cases, values are set to NULL. Works also in callback mode.

**COPYCOL*C*** (write-only): copy the values and attributes from column C to the given column (value is the number of a column).

**COPYLIN*L*** (write-only): copy the values and attributes from line L to the given line (value is the number of a line).

**FITTOSIZE** (write-only): Force lines and/or columns sizes so the matrix visible size fits in its current size.
NUMCOL_VISIBLE and NUMLIN_VISIBLE are considered when fitting, and they are not changed, only the RASTERWIDTHn and RASTERHEIGHTn attributes are changed.
But if any of the RASTERWIDTHn and RASTERHEIGHTn attributes where already set, then they will not be changed.
If the matrix is resized then it must be set again to obtain the same result, but before doing that set to NULL all the RASTERWIDTHn and RASTERHEIGHTn attributes that you want to be changed.
Can be LINES, COLUMNS or YES (meaning both).

**FITTOTEXT** (write-only): Fit the RASTERWIDTHn or the RASTERHEIGHTn attribute for the given column or line, so that it will fit the largest text in the column or the highest text in the line.
The number of the column or line must be preceded by a character identifying its type, "C" for columns and "L" for lines.
For example "C5"=column 5 or "L3"=line 3.
If FITMAXWIDTHn or FITMAXHEIGHTn are set for the column or line they are used as maximum limit for the size.

**MOVECOL*C*** (write-only): move the values and attributes from column C to the given column (value is the number of a column).
Internally will use ADDCOL+COPYCOL+DELCOL to perform the move so it is limited to those attributes restrictions.
It can be used in normal operation mode or in callback mode, but in callback mode will not update cell values, this must be done by the application.

**MOVELIN*L*** (write-only): move the values and attributes from line L to the given line (value is the number of a line).
Internally will use ADDLIN+COPYLIN+DELLIN to perform the move so it is limited to those attributes restrictions.
It can be used in normal operation mode or in callback mode, but in callback mode will not update cell values; this must be done by the application.

**REDRAW** (write-only): The user can inform the matrix that the data has changed, and it must be redrawn.
Values:

"ALL": Redraws the whole matrix.\
"L%d": Redraws the given line (e. g.: "L3" redraws line 3)\
"L%d-%d": Redraws the lines in the given region (e.g.: "L2-4" redraws lines 2, 3 and 4)\
"C%d": Redraws the given column (e.g.: "C3" redraws column 3)\
"C%d-%d": Redraws the columns in the given region (e.g: "C2-4" redraws columns 2, 3 and 4)

No redraw is done when the application sets the attributes: L:C, ALIGNMENTc, BGCOLOR*, FGCOLOR*, FONT*, VALUE, FRAME*COLOR, MARKL:C.
Global and size attributes always automatically redraw the matrix.

**SHOW** (write-only): If necessary, scroll the visible area to make the given cell visible.
To scroll to a line or a column, use a value such as "***L***:*" or "*:***C***" (where ***L***>0 and ***C***>0).

### Editing Attributes

**EDITMODE**: When set to YES, programmatically puts the current cell in edition mode, allowing the user to modify its value.
When consulted informs if the editing control is visible (text or dropdown).
Possible values: "YES" or "NO".

**EDITALIGN**: sets the text box alignment to the column alignment when editing a cell value.
Default: No.

**EDITCELL** (read-only): returns the current cell being edited ("L:C"), or NULL if none.
Can also be used during interaction while editing is being performed and EDITHIDEONFOCUS=NO.

**EDITFITVALUE**: enable a text box larger than the cell size of necessary, according to the cell font and cell current value.
While editing if more room is necessary it will grow to the right.

**EDITHIDEONFOCUS**: when editing a cell if text box loses its focus, then editing ends.
Default: Yes. When set to NO editing will continue and the matrix can be scrolled, also when pressing Esc or Enter if the focus is at the matrix it has the same effect as if pressed at the text box.

**EDITING** (read-only): returns Yes if the editing process is active for text or dropdown.
It is set to Yes after EDITION_CB, after MENUDROP_CB,  before DROP_CB, and before the editing control is made visible.
Set to NO when editing is about to end, after EDITION_CB and after the value has been updated, but before the editing control is made invisible.

**EDITNEXT**: controls how the next cell after editing is chosen. Can be LIN, COL, LINCR, COLCR.
Default: LIN.

LIN   - go to the next line at the same column, if at the last line then go to the next column at the same line;\
LINCR - go to the next line at the same column, if at the last line then go to the next column at the first line;\
COL   - go to the next column at the same line, if at the last column then go to the next line at the same column;\
COLCR - go to the next column at the same line, if at the last column then go to the next line at the first column;\
NONE  - stay in the same cell.

**EDITTEXT** (read-only): returns Yes if the editing is being done by a text box.

**EDITVALUE** (read-only): returns Yes if the display cell value being consulted will be used for a text box initial value.
Useful for being consulted inside the translate and numeric callbacks.

### Text Editing Attributes

**CARET**: Allows specifying and verifying the caret position of the text box in edition mode.

**INSERT**: inserts a text at the caret position of the text box in edition mode.

**MASK*L:C***  or **MASK*L:**** or **MASK**:C***: Defines a mask that will filter text input.
All [MASK](../attrib/iup_mask.md) auxiliary attributes are also available by adding the line and column at the end of the attribute name.

**MULTILINE**: allows the edition of multiple lines. Use Shift+Enter to add lines.
Enter will end the editing.

**SELECTION**: Allows specifying and verifying a selection interval of the text box in edition mode.

### Canvas Attributes (inheritable) BORDER: Changed to NO.

**SCROLLBAR**: Changed to YES.
