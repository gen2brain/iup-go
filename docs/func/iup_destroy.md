## IupDestroy

Destroys an interface element and all its children.
Only dialogs, timers, popup menus and images should be normally destroyed, but **detached** controls can also be destroyed.

### Parameters/Return

    void IupDestroy(Ihandle *ih);

**ih**: Identifier of the interface element to be destroyed.

### Notes

It will automatically **unmap** and **detach** the element if necessary, and then **destroy** the element.

This function also deletes the main names associated to the interface element being destroyed, but if it has more than one name, then some names may be left behind.

**Menu** bars associated with dialogs are automatically destroyed when the dialog is destroyed.

**Images** associated with controls are NOT automatically destroyed, because images can be reused in several controls the application must destroy them when they are not used anymore.

All dialogs and all elements that have names are automatically destroyed in **IupClose**.

### See Also

[IupAppend](iup_append.md), [IupDetach](iup_detach.md), [IupMap](iup_map.md), [IupUnmap](iup_unmap.md), [IupCreate](iup_create.md)
