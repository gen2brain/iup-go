# Events and Callbacks

IUP is a graphics interface library, so most of the time it waits for an event to occur, such as a button click or a mouse leaving a window.
The application informs IUP which callback is called when an event takes place.
Events are handled through callbacks, which are functions that the application registers in IUP.

The events are processed only when IUP has control of the application.
After the application creates and shows a dialog, it must return the control to IUP so it can process incoming events.
This is done in the IUP main event loop, usually once at the application "main" function.
One exception is the display of modal dialogs.
These dialogs will have their own event loop, and the previously shown dialogs will stop receiving events until the modal dialog returns.

## Guide

### Using

Callbacks are used by the application to receive notifications from the system that the user or the system itself has interacted with the user interface of the application.

Callbacks are also associated to an element by means of a name.

To associate a function to a callback, the application directly sets the callback using the attribute name with **IupSetCallback**.
For example:

    int myButton_action(Ihandle* self);
    ...
    IupSetCallback(myButton, "ACTION", (Icallback)myButton_action);

Callbacks do NOT have **inheritance** like attributes.

All element callbacks receive at least the element which activated the action as a parameter (self).
Global callbacks, such as IDLE_ACTION, ENTRY_POINT and EXIT_CB, are set with **IupSetFunction** and receive no element.

The callbacks implemented by the application must return one of the following values:

- IUP_DEFAULT: Proceeds normally with user interaction. In case other return values do not apply, the callback should return this value.
- IUP_CLOSE: Calls **IupExitLoop** after return, which ends the innermost running loop (**IupMainLoop** or a modal **IupPopup**). Applies only to some actions.
- IUP_IGNORE: Makes the native system ignore that callback action. Applies only to some actions.
- IUP_CONTINUE: Makes the element ignore the callback and pass the treatment of the execution to the parent element. Applies only to some actions.

Only some callbacks support the last 3 return values. Check each callback documentation.
When nothing is documented, then only IUP_DEFAULT is supported.

Callbacks that report a change made by the user are not called when the application sets the value via **IupSetAttribute**.
For instance, when the application changes a selected item on a list, no callback is called.
Callbacks of the element life cycle, such as MAP_CB, SHOW_CB, RESIZE_CB, UNMAP_CB and DESTROY_CB, are also called from application calls.

The order of callback calling is system-dependent.
For instance, the order of RESIZE_CB and SHOW_CB when the dialog is shown for the first time differs between drivers.

To help the definition of callbacks in C, the header "iupcbs.h" can be used; it has typedefs for all the callbacks.

### Main Loop

IUP is an event-oriented interface system, so it will keep a loop "waiting" for the user to interact with the application.
For this loop to occur, the application must call the **IupMainLoop** function, which is generally used right before **IupClose**.

When the application is closed by returning IUP_CLOSE in a callback, calling **IupExitLoop** or by hiding the last visible dialog, the function **IupMainLoop** will return.
Hiding the last visible dialog does not end the loop when the global LOCKLOOP is YES.
In Android, iOS and WebAssembly the platform owns the loop; see [IupMainLoop](func/iup_mainloop.md) and [ENTRY_POINT](call/iup_entry_point.md).

The **IupLoopStep**, **IupLoopStepWait** and **IupFlush** functions force the processing of incoming events while inside an application callback.
**IupLoopStep** and **IupLoopStepWait** return IUP_CLOSE after a callback returned IUP_CLOSE or **IupExitLoop** was called, so a loop that calls them must check that and stop.

### Threads

IUP is not thread-safe; it must be used only from the thread that called **IupOpen**.
Other threads use [IupPostMessage](func/iup_postmessage.md) to deliver work to the main thread, where it is processed by POSTMESSAGE_CB.
