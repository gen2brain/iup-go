## K_ANY

Action generated when a keyboard event occurs.

### Callback

    int function(Ihandle *ih, int c);

**ih**: identifier of the element that activated the event.\
**c**: identifier of typed key. Please refer to the [Keyboard Codes](../attrib/iup_keyboard_codes.md) table for a list of possible values.

**Returns**: If IUP_IGNORE is returned the key is ignored and not processed by the control and not propagated.
If returns IUP_CONTINUE, the key will be processed and the event will be propagated to the parent of the element receiving it, this is the default behavior.
If returns IUP_DEFAULT the key is processed but it is not propagated. IUP_CLOSE will be processed.

### Notes

Keyboard callbacks depend on the keyboard usage of the control with the focus.
So if you return IUP_IGNORE the control will usually not process the key.
But be aware that sometimes the control process the key in another event so even returning IUP_IGNORE the key can get processed.
Although it will not be propagated.

**IMPORTANT**: The callbacks "K_*" of the dialog or native containers depend on the IUP_CONTINUE return value to work while the control is in focus.

If the callback does not exists it is automatically propagated to the parent of the element.

#### K_* callbacks

All defined keys are also callbacks of any element, called when the respective key is activated.
For example: "K_cC" is also a callback activated when the user press Ctrl+C, when the focus is at the element or at a children with focus.
This is the way an application can create shortcut keys, also called hot keys.

### Affects

All elements with keyboard interaction.
