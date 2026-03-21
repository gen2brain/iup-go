## FLATSCROLLBAR

Complementary attributes when a flat scrollbar is used (a drawn scrollbar).

Used in [IupFlatScrollBox](../elem/iup_flatscrollbox.md), [IupFlatList](../elem/iup_flatlist.md), [IupFlatTree](../elem/iup_flattree.md) and in [IupMatrix](../ctrl/iup_matrix.md) when FLATSCROLLBAR=Yes is defined.

### Attributes (non-inheritable)

**SB_BACKCOLOR** (non-inheritable): color used as a background for the scrollbar.
By default, it will inherit from BGCOLOR.

**SB_FORECOLOR** (non-inheritable): handler and arrow color. Default: "220 220 220".
Used instead of FGCOLOR to avoid inheritance problems.

**SB_HIGHCOLOR** (non-inheritable): handler and arrow color when highlight. Default: "132 132 132".

**SB_PRESSCOLOR** (non-inheritable): handler and arrow color when pressed. Default: "96 96 96".

**SCROLLBARSIZE** (non-inheritable): The width of the vertical scrollbar or the height of the horizontal scrollbar.
Default: 15.

**SHOWARROWS** (non-inheritable): Allow to show or hide the arrows. Default: Yes.

**SHOWFLOATING** (non-inheritable): the scrollbar is shown only when used, over the space it occupied.
Move the mouse over the scrollbar area to show the scrollbars.
They are automatically hidden after not being used by the time defined in FLOATINGDELAY.

**SHOWTRANSPARENT** (non-inheritable): This makes the flat scrollbar semi transparent and only interactive through its handler.
It implies in SHOWARROWS=NO and SHOWFLOATING=Yes.

**FLOATINGDELAY** (non-inheritable): time to hide the scrollbar when SHOWFLOATING=Yes in milliseconds.
Default: 2000.

**ARROWIMAGES** (non-inheritable): replace the drawn arrows by the following images.

**SB_IMAGELEFT, SB_IMAGERIGHT, SB_IMAGETOP, SB_IMAGEBOTTOM** (non-inheritable): Arrow image name (the attribute is relative to where the arrow is pointing).
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
See also [IupImage](../elem/iup_image.md).
IMPORTANT = all images must be square with side equals to SCROLLBARSIZE.

**SB_IMAGE*HIGHLIGHT** (non-inheritable): Arrow image name of the element in highlight state.
If it is not defined then the IMAGE* is used.

**SB_IMAGE*INACTIVE** (non-inheritable): Arrow image name of the element when inactive.
If it is not defined then the IMAGE* is used and its colors will be replaced by a modified version creating the disabled effect.

**SB_IMAGE*PRESS** (non-inheritable): Arrow image name of the element in pressed state.
If it is not defined then the IMAGE* is used.

### Affects

[IupFlatScrollBox](../elem/iup_flatscrollbox.md), [IupMatrix](../ctrl/iup_matrix.md)

### Notes

When SHOWFLOATING=Yes, the natural size of the **IupMatrix** is reduced because it will not include the scrollbars area.
But notice that when vertically scrolling the last column or horizontally scrolling the last line the visibility or the cells are reduced because the scrollbar is show above the cells.

The flat scrollbar does not support the XMIN nor YMIN attributes. They are considered to be 0 always.
The XAUTOHIDE and YAUTOHIDE are considered to be YES always.

Also, all numeric attributes are integer numbers.

### See Also

[SCROLLBAR](iup_scrollbar.md), [POSX](iup_posx.md), [XMAX](iup_xmax.md), [DX](iup_dx.md), [POSY](iup_posy.md), [YMAX](iup_ymax.md), [DY](iup_dy.md)
