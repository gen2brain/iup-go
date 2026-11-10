## IDLE_ACTION

Predefined IUP action, generated when there are no events or messages to be processed.
Often used to perform background operations.

### Callback

    int function(void);

**Returns**: if IUP_CLOSE is returned the current loop will be closed and the callback will be removed.
If IUP_IGNORE is returned the callback is removed and normal processing continues.

### Notes

The Idle callback will be called whenever there are no messages left to be processed.
But this occurs more frequently than expected, for example when the mouse moves over the application the idle callback will be called many times, because each mouse move message is processed before the next one is scheduled.

So this callback changes the message loop to a more CPU consuming one.
It must be set to NULL when not in use.
Otherwise the application will be consuming CPU even if the callback is doing nothing.

It can only be set using **IupSetFunction**("IDLE_ACTION", func).

#### Long Time Operations

A loop or an operation that takes a long time to complete inside an application callback interrupts the user interface message loop processing until the callback returns, so the user cannot click on any control of the application.
Ways to handle that:

- call **IupLoopStep** or **IupFlush** inside the application callback when it is performing long time operations. This will allow the user to click on a cancel button for instance, because the user interface message loop will be processed.
- split the operation in several parts that are processed by the **Idle** function when no messages are left to be processed for the user interface message loop. This will make a heavy use of the CPU, even if the callback is doing nothing.
- split the operation in several parts but use a **Timer** to process each part.

For something simple as a background redraw of an **IupCanvas**, handling the "idle" state in the application is a better choice.
For example, register a timer for a small time like 500ms, and reset the timer in all the mouse and keyboard callbacks of the **IupCanvas**.
If the timer is triggered then the canvas is in idle state.
If the **IupCanvas** loses its focus then stop the timer.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupSetFunction](../func/iup_setfunction.md), [IupTimer](../elem/iup_timer.md).
