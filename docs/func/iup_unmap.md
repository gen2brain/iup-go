## IupUnmap Unmap the element from the native system. It will also unmap all its children.

It will NOT **detach** the element from its parent, and it will NOT **destroy** the IUP element.

### Parameters/Return

    void IupUnmap(Ihandle* ih);

**ih**: Identifier of an interface element.

### Notes

When the element is mapped, some attributes are stored only in the native system.
If the element is **unmaped** those attributes are lost.
Use the function [IupSaveClassAttributes](iup_saveclassattributes.md) when you want to **unmap** the element and keep its attributes.

The UNMAP_CB callback is called before the element is actually unmapped from the native system.

### See Also

[IupAppend](iup_append.md), [IupDetach](iup_detach.md), [IupMap](iup_map.md), [IupCreate](iup_create.md), [IupDestroy](iup_destroy.md)

 
