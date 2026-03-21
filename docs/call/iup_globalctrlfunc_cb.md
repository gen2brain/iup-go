## GLOBALCTRLFUNC_CB

Global callback for a restricted set of keys. Called only when the Ctrl+F? key combinations are pressed.
It was designed to be used for internal application debugging porpoises only.
Do not use it for release code.

### Callback

    void function(int c);

**c**: identifier of typed key. Please refer to the [Keyboard Codes](../attrib/iup_keyboard_codes.md) table for a list of possible values.

### Notes

It can only be set using **IupSetFunction(**name, func**)**.

### See Also

[IupSetFunction](../func/iup_setfunction.md)
