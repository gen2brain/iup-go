## IupGetDialog

Returns the handle of the dialog that contains that interface element.
Works also for children of a menu that is associated with a dialog.

### Parameters/Return

    Ihandle* IupGetDialog(Ihandle *ih);

**ih**: Identifier of an interface element.

**Returns:** the handle of the dialog or NULL if not found.
