## IupSetLanguageString

Associates a name with a string as an auxiliary method for Internationalization of applications.

### Parameters/Return

    void IupSetLanguageString(const char *name, const char *value); 
    void IupStoreLanguageString(const char *name, const char *value);


**name**: name of the string.\
**value**: string value.

### Notes

**IupStoreLanguageString** will duplicate the string internally.
**IupSetLanguageString** will store the pointer.

Elements that have pre-defined texts use this function when the current language is changed using **IupSetLanguage**.

IUP will **not** store strings for several languages at the same time, it will store only for the current language.
When **IupSetLanguage** is called only the internal pre-defined strings are replaced in the internal database.
The application must register again all its strings for the new language.

If a dialog is created with string names associations and the associations are about to be changed, then the dialog must be destroyed **before** the associations are changed, then created again.

Associations are retrieved using the **IupGetLanguageString** function.
But to simplify the usage of the string names associations attributes set with regular **IupSetStr*** functions, can use the prefix "_@" to indicate a string name and not the actual string.
**IupSetAttribute*** functions cannot be used because they simply store a pointer that may not be a string.

### Examples

    // If Language is Englih 
    IupSetLanguageString("IUP_CANCEL", "Cancel");
      or
    // If Language is Portuguese
    IupSetLanguageString("IUP_CANCEL", "Cancelar");

    // Then when setting a button title use:
    Ihandle* button_cancel = IupButton(IupGetLanguageString("IUP_CANCEL"), NULL);
      or
    Ihandle* button_cancel = IupButton("_@IUP_CANCEL", NULL);
      or
    IupSetStrAttribute(button_cancel, "TITLE", "_@IUP_CANCEL");

### See Also

[IupGetLanguageString](iup_getlanguagestring.md), [IupSetLanguagePack](iup_setlanguagepack.md)
