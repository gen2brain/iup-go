## ACCESSIBLETITLE (non-inheritable)

Accessible name announced by screen readers for the element.
Gives an image-only control or a custom-drawn flat control a spoken label, or overrides the label otherwise derived from TITLE.
Not supported in Motif, FLTK, EFL and Haiku.

### Value

Text.

## ACCESSIBLEDESCRIPTION (non-inheritable)

Accessible description announced by screen readers after the name (help or hint text).
When not set, the TIP text is used instead.
Not supported in Win32, Motif, FLTK, EFL and Haiku.
In Android, it maps to the tooltip text, announced by the screen reader (API 26+).

### Value

Text.

### Affects

All controls that have visual representation, except menus.

### Notes

For flat controls (IupFlatButton, IupFlatLabel and similar) the accessible name is set automatically from the drawn text, so ACCESSIBLETITLE is only needed to change it or to label an image-only control that has no text.

### See Also

[TIP](iup_tip.md)
