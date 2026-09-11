## ACTIVE

Activates or inhibits user interaction.

### Value

"YES" (active), "NO" (inactive).

Default: "YES"

### Notes

An interface element is only active if its native parent is also active.

ACTIVE can also be set for controls that do not have user interaction because they may have a visual feedback to indicate the inactive state.

An inactive dialog ignores its close box.
In GTK, GTK 4, Qt, Motif, FLTK and WebAssembly it can still be moved, resized and raised; in Win32 and macOS the whole window ignores mouse input, including the title bar.

### Affects

All controls that have visual representation.
