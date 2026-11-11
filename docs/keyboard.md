# Keyboard

## Guide

### Focus

The application can control the focus using the functions **IupGetFocus** and **IupSetFocus**.
When the focus is changed, the application is notified through the callbacks [GETFOCUS_CB](call/iup_getfocus_cb.md) and [KILLFOCUS_CB](call/iup_killfocus_cb.md).

Keyboard navigation in the dialog uses the "Tab" and "Shift+Tab" keys to change the keyboard focus from one control to another.
When the focus is at an **IupMultiline** control, the combination "Ctrl+Tab" must be used to change focus, because "Tab" is a valid input for the text.
All IUP interactive controls have Tab stops, but the navigation order is related to the order the controls are placed in the dialog and cannot be changed.
The order is the same implemented by the functions **IupNextField** and **IupPreviousField**.
Controls that are not mapped, inactive, invisible or have CANFOCUS=NO are skipped.
To remove the Tab stop from a control, use the CANFOCUS attribute.
In Android, touch does not move the focus to buttons, toggles and sliders; there the focus changes only through keyboard or D-pad navigation and IupSetFocus.

The Up and Down arrows also navigate between buttons, flat buttons and toggles.
When an **IupToggle** is inside an **IupRadio**, the "Tab" keys will navigate only to the selected toggle.

In Windows and WinUI, the focus feedback only appears after the user presses a key (except for the **IupText** where the feedback is the caret).
When a control is clicked before any key is pressed, the focus feedback will NOT be shown, although the control will be in focus.
**IupMatrix** and other additional controls will always show their focus feedback.
In GTK, GTK 4, Qt, Motif and EFL, the focus feedback is always shown for the control that has the focus.
In macOS, focus ring visibility depends on the system keyboard navigation preference.

### Enter and Esc

Two keys are also important in keyboard navigation: "Enter" and "Esc".
They are only effective if the application defines the attributes DEFAULTENTER and DEFAULTESC of the [IupDialog](dlg/iup_dialog.md).
These attributes configure buttons to be activated when the respective key is pressed.
"Enter" is a valid key for the **IupMultiline**, so the combination "Ctrl+Enter" must be used instead.
If the focus is at a button, then the Enter key will activate that button independent of the DEFAULTENTER attribute.
"Esc" first hides an [IupPopover](elem/iup_popover.md) with AUTOHIDE=YES, before DEFAULTESC.

### Mnemonics and Shortcuts

A "&" in the TITLE of a control defines a mnemonic: the next character is underlined and Alt+character activates the control.
A mnemonic on an **IupLabel** applies to the next interactive control: a button or toggle is activated, any other control gets the focus.
Mnemonics of hidden or inactive controls, including controls in a tab that is not the current one, are ignored.
See [TITLE](attrib/iup_title.md).

The text after a "\t" in the TITLE of an [IupMenuItem](elem/iup_menuitem.md) in the menu bar is its shortcut, for example "Save\tCtrl+S".
Pressing that key in the dialog activates the item from any control, and the key is not delivered to the K_* callbacks.

### Key Callbacks

Usually, the application will process keyboard input in the **IupCanvas** using the [KEYPRESS_CB](call/iup_keypress_cb.md) callback.
Composed text input in the **IupCanvas** is reported by [TEXTINPUT_CB](call/iup_textinput_cb.md).
The [K_ANY](call/iup_k_any.md) callback can be used for all the controls, but it does not report the press state; it is called only when the key is pressed.
Both callbacks use the key codification explained in [Keyboard Codes](attrib/iup_keyboard_codes.md).
These codes are also used in the ACTION callbacks of **IupText** and **IupMultiline**.

All the keyboard codes can also be used as callback names to implement application hot keys, for example "K_cS".
The K_* callback of the key, or K_ANY when that is not defined, is called for the element with the focus and then for each of its parents, until one returns a value other than IUP_CONTINUE.
So a hot key defined in the dialog works from any control.
K_ANY is called before the Tab, Enter and Esc navigation, so returning IUP_IGNORE there also blocks it.
