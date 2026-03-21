## IDLE_ACTION

Predefined IUP action, generated when there are no events or messages to be processed.
Often used to perform background operations.

### Callback

    int function(void);

**Returns**: if IUP_CLOSE is returned the current loop will be closed and the callback will be removed.
If IUP_IGNORE is returned the callback is removed and normal processing continues.

### Notes

The Idle callback will be called whenever there are no messages left to be processed.
But this occurs more frequent than expected, for example if you move the mouse over the application the idle callback will be called many times because the mouse move message is processed so fast that the Idle will be called before another mouse move message is schedule to processing.

So this callback changes the message loop to a more CPU consuming one.
It is important that you set it to NULL when not using it.
Or the application will be consuming CPU even if the callback is doing nothing.

It can only be set using **IupSetFunction(**name, func**)**.

#### Long Time Operations

If you create a loop or an operation that takes a long time to complete inside a callback of your application then the user interface message loop processing is interrupted until the callback returns, so the user can not click on any control of the application.
But there are ways to handle that:

- call **IupLoopStep** or **IupFlush** inside the application callback when it is performing long time operations. This will allow the user to click on a cancel button for instance, because the user interface message loop will be processed.
- split the operation in several parts that are processed by the **Idle** function when no messages are left to be processed for the user interface message loop. This will make a heavy use of the CPU, even if the callback is doing nothing.
- split the operation in several parts but use a **Timer** to process each part.

If you just want to do something simple as a background redraw of an **IupCanvas**, then a better idea is to handle the "idle" state yourself.
For example, register a timer for a small time like 500ms, and reset the timer in all the mouse and keyboard callbacks of the **IupCanvas**.
If the timer is trigged then you are in idle state.
If the **IupCanvas** loses its focus then stop the timer.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupSetFunction](../func/iup_setfunction.md), [IupTimer](../elem/iup_timer.md).
