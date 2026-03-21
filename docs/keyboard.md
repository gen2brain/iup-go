## Keyboard

The application can control the focus using the functions **IupGetFocus** and **IupSetFocus**.
When the focus is changed, the application is notified through the callbacks GETFOCUS_CB and KILLFOCUS_CB.

Keyboard navigation in the dialog uses the "Tab" and "Shilf+Tab" keys to change the keyboard focus from one control to another.
The exception is when the focus is at an **IupMultiline** control, to change focus the combination "Ctrl+Tab" must be used, because "Tab" is a valid input for the text.
All IUP interactive controls have Tab stops, but the navigation order is related to the order the controls are placed in the dialog and cannot be changed.
The order is the same implemented by the functions **IupNextField** and **IupPreviousField**.
To remove the Tab stop from a control, use the CANFOCUS attribute.

Arrows can also be used for navigation between buttons and toggles.
This is necessary because when an **IupToggle** is inside an **IupRadio** the "Tab" keys will navigate only to the selected toggle.

In Windows, the focus feedback only appears after the user presses a key (except for the **IupText** where the feedback is the caret).
Before pressing a key if you click on a control, the focus feedback will NOT be shown, although it will be in focus.
**IupMatrix** and other additional controls will always show their focus feedback.
In GTK and Motif, the focus feedback is always shown for the control that has the focus.

Two keys are also important in keyboard navigation: "Enter" and "Esc".
But they are only effective if the application registers the attributes DEFAULTENTER and DEFAULTESC of the [IupDialog](dlg/iup_dialog.md).
These attributes configure buttons to be activated when the respective key is pressed.
Again, "Enter" is a valid key for the Multiline, so the combination "Ctrl+Enter" must be used instead.
If the focus is at a button, then the Enter key will activate that button independent of the DEFAULTENTER attribute.

Usually, the application will process keyboard input in the **IupCanvas** using the [KEYPRESS_CB](elem/iup_canvas.md) callback.
But there is also the [K_ANY](call/iup_k_any.md) callback that can be used for all the controls, but it does not have control of the press state, it is called only when the key is pressed.
Both callbacks use the key codification explained in [Keyboard Codes](attrib/key.md).
These codes are also used in the ACTION callbacks of **IupText** and **IupMultiline**, and in shortcuts using the KEY attribute of **IupItem** and **IupSubmenu**.
Finally, all the keyboard codes can be used as callback names to implement application hot keys.
