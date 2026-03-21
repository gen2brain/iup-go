## LDESTROY_CB

Called at the end of the destroy process, after all children were destroyed and the element was detached and unmapped.

### Callback

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

### Notes

Used for language binding implementations to release memory allocated by the binding for the element.

### Affects

All.
