## CONTROLSIZE [macOS Only]

Size variant of the native controls.
When FONT is not defined for the element or its parents, the control uses the system font of the matching size.
It is applied when the element is mapped.

### Value

"MINI", "SMALL", "REGULAR" or "LARGE".

Default: "REGULAR".

### Notes

The attribute is inherited, so setting it in a dialog or a container affects all the controls inside.
Labels only change their font.

### Affects

All controls that have visual representation, except menus.

### See Also

[FONT](iup_font.md)
