## IupTimer

Creates a timer which periodically invokes a callback when the time is up.
Each timer should be destroyed using [IupDestroy](../func/iup_destroy.md).

### Creation

    Ihandle* IupTimer(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**TIME**: The time interval in milliseconds. In Windows the minimum value is 10ms.

**TOLERANCE** [Win32, macOS and iOS Only]: How late, in milliseconds, the system may fire the timer, so it can group the timer with other wakeups and save energy.
Used when the timer is started with RUN=YES. Default: 0 (no tolerance).
In Win32 it needs Windows 8 or newer.

**RUN**: Starts and stops the timer. Possible values: "YES" or "NO". Returns the current timer state.
If you have multiple threads start the timer in the main thread.

**ELAPSEDTIME** (read-only): Returns the time elapsed since the timer was started, in milliseconds.
Returns NULL if not running.

**WID** (read-only): Returns the native serial number of the timer. Returns -1 if not running.
A timer is mapped only when it is running.

### Callbacks

**ACTION_CB**: Called every time the defined time interval is reached.
To stop the callback from being called simply stop de timer with RUN=NO.

    int function(Ihandle *ih);

> **ih**: identifier of the element that activated the event.
>
> **Returns**: IUP_CLOSE will be processed.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupNotify](iup_notify.md), [IupUser](iup_user.md)
