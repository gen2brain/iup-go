## MAP_CB

Called right after an element is mapped and its attributes updated in [IupMap](../func/iup_map.md).

When the element is a dialog, it is called after the layout is updated.
For all other elements, it is called before the layout is updated, so the element current size will still be 0x0 during MAP_CB.

### Callback

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

### Affects

All elements that have a native representation.
