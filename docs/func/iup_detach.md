## IupDetach

**Detaches** an interface element from its parent.

### Parameters/Return

    void IupDetach(Ihandle *child);

**child**: Identifier of the interface element to be detached.

### Notes

It will automatically call **IupUnmap** to **unmap** the element if necessary, and then **detach** the element.

If left **detached** it is still necessary to call **IupDestroy** to **destroy** the IUP element.

The elements are NOT immediately repositioned.
Call **IupRefresh** for the container (or any other element in the dialog) to update the dialog layout.

When the element is mapped some attributes are stored only in the native system.
If the element is **unmaped** those attributes are lost.
Use the function [IupSaveClassAttributes](iup_saveclassattributes.md) when you want to **unmap** the element and keep its attributes. 

### See Also

[IupAppend](iup_append.md), [IupInsert](iup_insert.md), [IupRefresh](iup_refresh.md), [IupUnmap](iup_unmap.md), [IupCreate](iup_create.md), [IupDestroy](iup_destroy.md)
