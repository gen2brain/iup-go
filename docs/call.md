# Events and Callbacks

IUP is a graphics interface library, so most of the time it waits for an event to occur, such as a button click or a mouse leaving a window.
The application can inform IUP which callback to be called, informing that an event has taken place.
Hence, events are handled through callbacks, which are just functions that the application register in IUP.

The events are processed only when IUP has control of the application.
After the application creates and shows a dialog, it must return the control to IUP so it can process incoming events.
This is done in the IUP main event loop. And it is usually done once at the application "main" function.
One exception is the display of modal dialogs.
These dialogs will have their own event loop, and the previously shown dialogs will stop receiving events until the modal dialog returns.

## Guide

### Using

Callbacks are used by the application to receive notifications from the system that the user or the system itself has interacted with the user interface of the application.
On the other hand, attributes are used by the application to communicate with the user interface system.

Even though callbacks have different purposes from attributes, they are also associated to an element by means of a name.

The OLD method to associate a function to a callback, the application must employ the **IupSetAttribute** function, linking the action to a name (passed as a string).
From this point on, this name will refer to a callback.
By means of function **IupSetFunction**, the user connects this name to the callback.

For example:

    int myButton_action(Ihandle* self);
    ...
    IupSetAttribute(myButton, "ACTION", "my_button_action");
    IupSetFunction("my_button_action", (Icallback)myButton_action);

In the NEW method, the application does not needs a global name, it directly sets the callback using the attribute name using **IupSetCallback**.
For example:

    int myButton_action(Ihandle* self);
    ...
    IupSetCallback(myButton, "ACTION", (Icallback)myButton_action);

The new method is more efficient and more secure, because there is no risk of a name conflict.

Although enabled in old versions, callbacks do NOT have **inheritance** like attributes.

All callbacks receive at least the element which activated the action as a parameter (self).

The callbacks implemented by the application must return one of the following values:

- IUP_DEFAULT: Proceeds normally with user interaction. In case other return values do not apply, the callback should return this value.
- IUP_CLOSE: Call **IupExitLoop** after return. Depending on the state of the application it will close all windows and exit the application. Applies only to some actions.
- IUP_IGNORE: Makes the native system ignore that callback action. Applies only to some actions.
- IUP_CONTINUE: Makes the element to ignore the callback and pass the treatment of the execution to the parent element. Applies only to some actions.

Only some callbacks support the last 3 return values. Check each callback documentation.
When nothing is documented, then only IUP_DEFAULT is supported.

An important detail when using callbacks is that they are only called when the user actually executes an action over an element.
A callback is not called when the programmer sets a value via **IupSetAttribute**.
For instance: when the programmer changes a selected item on a list, no callback is called.

The order of callback calling is system-dependent.
For instance, the RESIZE_CB and the SHOW_CB are called in different order in Win32 and in X-Windows when the dialog is shown for the first time.

To help the definition of callbacks in C, the header "iupcbs.h" can be used, there are typedefs for all the callbacks.

### Main Loop

IUP is an event-oriented interface system, so it will keep a loop "waiting" for the user to interact with the application.
For this loop to occur, the application must call the **IupMainLoop** function, which is generally used right before **IupClose**.

When the application is closed by returning IUP_CLOSE in a callback, calling **IupExitLoop** or by hiding the last visible dialog, the function **IupMainLoop** will return.

The **IupLoopStep** and the **IupFlush** functions force the processing of incoming events while inside an application callback.
