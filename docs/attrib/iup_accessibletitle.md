## ACCESSIBLETITLE (non-inheritable)

Accessible name announced by screen readers for the element.
Set it to give an image-only control or a custom-drawn flat control a spoken label, or to override the label otherwise derived from TITLE.
Not supported in Motif, FLTK, EFL and Haiku.

### Value

Text.

## ACCESSIBLEDESCRIPTION (non-inheritable)

Accessible description announced by screen readers after the name (help or hint text).
Not supported in Win32, Motif, FLTK, EFL and Haiku.
On Android it maps to the tooltip text, announced by the screen reader (API 26+).

### Value

Text.

### Notes

For flat controls (IupFlatButton, IupFlatLabel and similar) the accessible name is set automatically from the drawn text, so ACCESSIBLETITLE is only needed to change it or to label an image-only control that has no text.

### Affects

All controls that have visual representation, except menus.

### See Also

[TIP](iup_tip.md)
