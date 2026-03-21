## IupReparent

Moves an interface element from one position in the hierarchy tree to another.

Both **new_parent** and **child** must be mapped or unmapped at the same time.

If **ref_child** is NULL, then it will *append* the **child** to the **new_parent**.
If **ref_child** is NOT NULL, then it will *insert* **child** before **ref_child** inside the **new_parent**.

### Parameters/Return

    int IupReparent(Ihandle* child, Ihandle* new_parent, Ihandle* ref_child);

**child**: Identifier of the element to be moved.\
**new_parent**: Identifier of the new parent.\
**ref_child**: Identifier of the element to be used as reference, where the child will be inserted before it.
Can be NULL.

**Returns:** IUP_NOERROR if successfully, IUP_ERROR if failed.

### Notes

This function is faster and easier than doing the sequence **unmap**, **detach**, **append/insert** and **map**.

The elements are NOT immediately repositioned.
Call **IupRefresh** for the container (or any other element in the dialog) to update the dialog layout.

Motif does not support the re-parent function, but we simulate a re-parent doing a **unmap**/**map** sequence.
But some attributes may be lost during the operation, mostly attributes that are id dependent.

### See Also

[IupAppend](iup_append.md), [IupInsert](iup_insert.md), [IupDetach](iup_detach.md), [IupMap](iup_map.md), [IupUnmap](iup_unmap.md), [IupRefresh](iup_refresh.md)
