## TITLE (non-inheritable)

Element’s title. It is often used to modify some static text of the element (which cannot be changed by the user).

### Value

Text.

Default: ""

### Notes

The '\n' character usually is accepted for line change (except for menus).

The "&" character can be used to define a MNEMONIC, use "&&" to show the "&" character instead of defining a mnemonic.

If a mnemonic is defined, then the character relative to it is underlined and a key is associated so that when pressed together with the Alt key activates the control.

In GTK, if you define a mnemonic using "&" and the string has an underscore, then make sure that the mnemonic comes before the underscore.

If the MARKUP attribute is defined, then the title string can contain markup commands using a Pango-like subset (`<b>`, `<i>`, `<u>`, `<s>`, `<sub>`, `<sup>`, `<big>`, `<small>`, `<span>`).
Works only if a mnemonic is NOT defined in the title. Not valid for menus. Not supported in Win32 and Motif (markup tags are stripped and plain text is displayed).

### Affects

All elements with an associated text.

### See Also

[FONT](iup_font.md)
