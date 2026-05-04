## TITLE (non-inheritable)

Element’s title. It is often used to modify some static text of the element (which cannot be changed by the user).

### Value

Text.

Default: ""

### Notes

The '\n' character usually is accepted for line change (except for menus).

The "&" character can be used to define a MNEMONIC, use "&&" to show the "&" character instead of defining a mnemonic.

If a mnemonic is defined, the character relative to it is underlined and Alt+key activates the control.
Supported in Win32, WinUI, GTK, GTK 4, Qt, FLTK, EFL and Motif.
On macOS, Android and iOS the "&" is stripped from the displayed text and no shortcut is registered (these platforms have no Alt-mnemonic convention). On iOS an external keyboard with an Alt key still triggers the mnemonic.

In GTK, if you define a mnemonic using "&" and the string has an underscore, then make sure that the mnemonic comes before the underscore.

If the MARKUP attribute is defined, then the title string can contain markup commands using a Pango-like subset (`<b>`, `<i>`, `<u>`, `<s>`, `<sub>`, `<sup>`, `<big>`, `<small>`, `<span>`).
Works only if a mnemonic is NOT defined in the title. Not valid for menus. Not supported in Win32, FLTK and Motif (markup tags are stripped and plain text is displayed).

### Affects

All elements with an associated text.

### See Also

[FONT](iup_font.md)
