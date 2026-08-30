## TEXTINPUT_CB

Action generated when the user commits text input: a plain character, the result of a dead-key
composition, or an input method (IME) commit. Not generated for control and navigation keys,
nor for Ctrl and Alt combinations; those are reported by [K_ANY](../call/iup_k_any.md).

### Callback

    int function(Ihandle *ih, char *text);

**ih**: identifier of the element that activated the event.\
**text**: committed text in UTF-8. Can contain more than one character.

**Returns**: If IUP_IGNORE is returned the text is considered consumed and the K_ANY callback
is not generated for the originating key.

### Notes

In EFL dead keys and IME composition need an Ecore_IMF module: it is selected automatically under
Wayland, and from the ECORE_IMF_MODULE environment variable on X11. Without a module each key
commits its own character.
In GNUstep dead keys and IME composition are not composed; each key commits its own character.
In macOS the Option modifier produces text according to the keyboard layout.
In WASM IME composition is reported at composition end.
In Android and iOS the soft keyboard is shown when a control that defines the callback is touched.

### Affects

[IupCanvas](../elem/iup_canvas.md), [IupTerminal](../elem/iup_terminal.md)
