## IupUser

Creates a user element in IUP, which is not associated to any interface element.
It is used to map an external element to a IUP element.
Its use is usually for additional elements, but you can use it to create an Ihandle* to store private attributes.

It is also a void container. Its children can be dynamically added using [IupAppend](../func/iup_append.md) or [IupInsert](../func/iup_insert.md).

### Creation

    Ihandle* IupUser(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**CLEARATTRIBUTES** (write-only, non-inheritable): it will clear all attributes stored internally and remove all references.
