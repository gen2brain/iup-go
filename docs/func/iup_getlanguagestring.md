## IupGetLanguageString

Returns a language dependent string. The string must have been associated with the name using the **IupSetLanguageString** or **IupSetLanguagePack** functions.

### Parameters/Return

    char* IupGetLanguageString(const char* name); 

**Returns:** a string associated with the name.

### Notes

If the association is not found, returns the **name** itself.

See [IupSetLanguageString](iup_setlanguagestring.md) for an example.

### See Also

[IupSetLanguageString](iup_setlanguagestring.md), [IupSetLanguagePack](iup_setlanguagepack.md).
