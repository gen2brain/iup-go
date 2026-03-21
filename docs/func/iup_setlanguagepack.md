## IupSetLanguagePack

Sets a pack of associations between names and string values.
Internally will call **IupSetLanguageString** for each name in the pack.

### Parameters/Return

    void IupSetLanguagePack(Ihandle* ih); 


**ih**: pack of name-value association.
It is simply a **IupUser** element with several attributes set.

### Notes

After setting the pack, it can be destroyed.

The existent associations will not be removed. But if the new ones have the same names, the old ones will be replaced.
If set to NULL will remove all current associations.

### See Also

[IupGetLanguageString](iup_getlanguagestring.md), [IupSetLanguageString](iup_setlanguagestring.md), [IupUser](../elem/iup_user.md)
