## IupSetLanguagePack

Sets a pack of associations between names and string values.
Internally will call **IupSetLanguageString** for each name in the pack.

### Parameters/Return

    void IupSetLanguagePack(Ihandle* ih);

**ih**: pack of name-value associations.
It is an **IupUser** element with several attributes set.

### Notes

After setting the pack, it can be destroyed.

The existing associations will not be removed. If the new ones have the same names, the old ones will be replaced.
If **ih** is NULL, all current associations are removed.

### See Also

[IupGetLanguageString](iup_getlanguagestring.md), [IupSetLanguageString](iup_setlanguagestring.md), [IupUser](../elem/iup_user.md)
