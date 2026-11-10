## FLOATING (non-inheritable)

If an element has FLOATING=YES, then its size and position will be ignored by the layout processing in **IupVbox**, **IupHbox**, **IupZbox**, **IupGridBox** and **IupTabs**.
The element size and position are still updated in the native system, allowing the usage of [SIZE](iup_size.md)/[RASTERSIZE](iup_rastersize.md) and [POSITION](iup_position.md) to manually position and size the element.
If there is overlap, the application must ensure that the element is on top of others using [ZORDER](iup_zorder.md).

This is useful to keep an invisible element out of the box size computation.

If the value IGNORE is used then it behaves as YES, but it also does not update the size and position in the native system.

**IupMultiBox** supports only IGNORE.

### Value

"YES", "IGNORE" or "NO".

Default: "NO".

### Affects

All elements, except menus.

### See Also

[IupZbox](../elem/iup_zbox.md), [IupVBox](../elem/iup_vbox.md), [IupHBox](../elem/iup_hbox.md), [IupGridBox](../elem/iup_gridbox.md), [IupMultiBox](../elem/iup_multibox.md), [IupTabs](../elem/iup_tabs.md)
