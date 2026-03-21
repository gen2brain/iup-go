## FLOATING (non-inheritable)

If an element has FLOATING=YES, then its size and position will be ignored by the layout processing in **IupVbox**, **IupHbox** and **IupZbox**.
But the element size and position will still be updated in the native system allowing the usage of [SIZE](iup_size.md)/[RASTERSIZE](iup_rastersize.md) and [POSITION](iup_position.md) to manually position and size the element.
And must ensure that the element will be on top of others using [ZORDER](iup_zorder.md), if there is overlap.

This is useful when you do not want that an invisible element to be computed in the box size.

If the value IGNORE is used then it will behave as YES, but also it will not update the size and position in the native system.

### Value

"YES", "IGNORE" or "NO".

Default: "NO".

### Affects

All elements, except menus.

### See Also

[IupZbox](../elem/iup_zbox.md), [IupVBox](../elem/iup_vbox.md), [IupHBox](../elem/iup_hbox.md)
