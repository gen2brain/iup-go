## KEYPRESS_CB

Action generated when a key is pressed or released. If the key is pressed and held several calls will occur.
It is called after the callback **K_ANY** is processed.

### Callback

    int function(Ihandle *ih, int c, int press);

**ih**: identifier of the element that activated the event.\
**c**: identifier of typed key. Please refer to the [Keyboard Codes](../attrib/iup_keyboard_codes.md) table for a list of possible values.\
**press**: 1 is the user pressed the key or 0 otherwise.

**Returns**: If IUP_IGNORE is returned the key is ignored by the system.
IUP_CLOSE will be processed.

### Affects

[IupCanvas](../elem/iup_canvas.md)
