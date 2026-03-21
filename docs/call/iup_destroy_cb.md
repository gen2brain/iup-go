## DESTROY_CB

Called right **before** an element is destroyed.

### Callback

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

### Notes

If the dialog is visible, then it is hidden before it is destroyed.
The callback will be called right **after** it is hidden.

The callback will be called **before** all other destroy procedures.

For instance, if the element has children, then it is called **before** the children are destroyed.
If the element is mapped, it is called before the element is unmapped, so before UNMAP_CB.

For language binding implementations, use the callback [LDESTROY_CB](iup_ldestroy_cb.md).

### Affects

All.
