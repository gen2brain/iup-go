## SHRINK

If this attribute is defined, the elements inside the dialog will try to adjust their sizes even when the dialog's size is smaller than its natural size.

See the [Layout Guide](../layout.md) for more details on sizes.

### Value

"YES" or "NO".

Default: "NO".

### Notes

When the user changes the size of the dialog, the elements are automatically re-distributed inside the dialog.
Some elements even have their size changed if the EXPAND attribute is active.
When this size is smaller than a minimum limit in which all elements still fit the dialog, the elements' distribution is no longer modified.
Actually, the virtual size of the dialog remains larger than its actual size on the screen, and some elements to the right and bottom are hidden by the borders of the dialog.

The SHRINK attribute offers an alternative to this behavior.
It makes the elements continue to rearrange, even if they must overlapi

The results of this new rearrangement may vary according to the elements' distribution on the dialog.

Shrink will be effective only for containers.
For regular elements the current size will be set for a smaller value only if EXPAND is set.

See the [Layout Guide](../layout.md) for more details on sizes.

### Affects

[IupDialog](../dlg/iup_dialog.md)
