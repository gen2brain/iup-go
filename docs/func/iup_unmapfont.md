## IupUnMapFont

Retrieves the name of the old IUP FONT names, given the native font.  See the old [Font Names](../attrib/iup_font.md) table for a list of names. 

### Parameters/Return

    char* IupUnMapFont(const char *driverfont);

**Returns:** the name of the IUP font, given the native font.
If such a font does not exist, the function will return NULL.

### See Also

[IupMapFont](iup_mapfont.md)
