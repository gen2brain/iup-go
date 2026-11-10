## K_ANY

Action generated when a keyboard event occurs.

### Callback

    int function(Ihandle *ih, int c);

**ih**: identifier of the element that activated the event.\
**c**: identifier of typed key. See the [Keyboard Codes](../attrib/iup_keyboard_codes.md) table for the possible values.

**Returns**: If IUP_IGNORE is returned the key is ignored and not processed by the control and not propagated.
If IUP_CONTINUE is returned, the key will be processed and the event will be propagated to the parent of the element receiving it, this is the default behavior.
If IUP_DEFAULT is returned the key is processed but it is not propagated. IUP_CLOSE will be processed.

### Notes

Keyboard callbacks depend on the keyboard usage of the control with the focus.
When IUP_IGNORE is returned the control will usually not process the key.
Some controls process the key in another event, so even when IUP_IGNORE is returned the key can get processed.
It will not be propagated.

In Android and iOS, a text input reports each character typed as a single key, from a hardware or a soft keyboard, including Space and Backspace.
Characters outside Latin-1 and text composed by an input method reach only ACTION.

The callbacks "K_*" of the dialog or native containers depend on the IUP_CONTINUE return value to work while the control is in focus.

If the callback does not exist the event is automatically propagated to the parent of the element.

#### K_* callbacks

All defined keys are also callbacks of any element, called when the respective key is activated.
For example: "K_cC" is also a callback activated when the user presses Ctrl+C, when the focus is at the element or at a child with focus.
This is the way an application can create shortcut keys, also called hot keys.
A key bound as the shortcut of a menu bar item ([IupMenuItem](../elem/iup_menuitem.md) TITLE) activates the item and is not delivered to these callbacks.

### Affects

All elements with keyboard interaction.
