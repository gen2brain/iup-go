## BUTTON_CB

Action generated when a mouse button is pressed or released.

### Callback

    int function(Ihandle* ih, int button, int pressed, int x, int y, char* status);

**ih**: identifies the element that activated the event.\
**button**: identifies the activated mouse button:

> IUP_BUTTON1 - left mouse button (button 1);\
> IUP_BUTTON2 - middle mouse button (button 2);\
> IUP_BUTTON3 - right mouse button (button 3).

**pressed**: indicates the state of the button:

> 0 - mouse button was released;\
> 1 - mouse button was pressed.

**x**, **y**: position in the canvas where the event has occurred, in pixels.\
**status**: status of the mouse buttons and some keyboard keys at the moment the event is generated.
The following macros must be used for verification:

> iup_isshift(**status**)\
> iup_iscontrol(**status**)\
> iup_isbutton1(**status**)\
> iup_isbutton2(**status**)\
> iup_isbutton3(**status**)\
> iup_isbutton4(status)\
> iup_isbutton5(status)\
> iup_isdouble(**status**)\
> iup_isalt(status)\
> iup_issys(status)
>
> They return 1 if the respective key or button is pressed, and 0 otherwise.

**Returns**: IUP_CLOSE will be processed.
On some controls if IUP_IGNORE is returned the action is ignored (this is system-dependent).

### Notes

This callback can be used to customize a button behavior.
For a standard button behavior, use the ACTION callback of the **IupButton**.

For a single click, the callback is called twice, one for pressed=1 and one for pressed=0.
Only after both calls, the ACTION callback is called.
In Windows, if a dialog is shown or popup in any situation, there could be unpredictable results because the native system still has processing to be done even after the callback is called.

A double click is preceded by two single clicks, one for pressed=1 and one for pressed=0, and followed by a press=0, all three without the double click flag set.
In GTK, it is preceded by an additional two single clicks sequence.
For example, for one double click all the following calls are made:

    BUTTON_CB(but=1 (1), x=154, y=83 [  1       ])
    BUTTON_CB(but=1 (0), x=154, y=83 [  1       ])
    BUTTON_CB(but=1 (1), x=154, y=83 [  1       ])     (in GTK only)
    BUTTON_CB(but=1 (0), x=154, y=83 [  1       ])     (in GTK only)
    BUTTON_CB(but=1 (1), x=154, y=83 [  1  D    ])
    BUTTON_CB(but=1 (0), x=154, y=83 [  1       ])

Between press and release all mouse events are redirected only to this control, even if the cursor moves outside the element.
So the BUTTON_CB callback when released and the MOTION_CB callback can be called with coordinates outside the element rectangle.

### Affects

[IupCanvas](../elem/iup_canvas.md), [IupButton](../elem/iup_button.md), [IupText](../elem/iup_text.md), [IupList](../elem/iup_list.md), [IupGLCanvas](../ctrl/iup_glcanvas.md)
