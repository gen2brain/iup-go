## IupSaveClassAttributes

Saves all registered attributes on the internal hash table.

### Parameters/Return

    void IupSaveClassAttributes(Ihandle* ih);

**ih**: identifier of the interface element.

### Notes

When the element is mapped, some attributes are stored only in the native system.
If the element is **unmapped** those attributes are lost.
Use this function to **unmap** the element and keep its attributes.

It does not save id dependent attributes, like those which have a complementary number.
For example, items in a **IupList**, **IupTree** or **IupMatrix**.

### See Also

[IupGetClassAttributes](iup_getclassattributes.md), [IupGetClassName](iup_getclassname.md), [IupGetClassType](iup_getclasstype.md), [IupGetAllAttributes](iup_getallattributes.md), [IupCopyClassAttributes](iup_copyclassattributes.md)
