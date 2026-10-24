## IupFdWatch

Creates a file descriptor watch, which invokes a callback when the descriptor has data or can accept data.
The main loop waits on the descriptor, so the application does not need a timer to poll it.
Each watch should be destroyed using [IupDestroy](../func/iup_destroy.md).

### Creation

    Ihandle* IupFdWatch(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**FD**: The file descriptor to watch. In Windows it must be a socket, other handles are not supported.
Must be set before the watch is started.

**MODE**: When the callback is called. Possible values: "READ" (default), "WRITE" or "READWRITE".

**RUN**: Starts and stops the watch. Possible values: "YES" or "NO". Returns the current watch state.
Remains "NO" when FD was not set, or in WebAssembly where descriptors do not exist.

### Callbacks

**ACTION_CB**: Called every time the descriptor is ready.
In "READ" mode the callback must read the available data, otherwise it is called again immediately.
In "WRITE" mode the callback is called while the descriptor accepts data, so stop the watch with RUN=NO when there is nothing left to write.

    int function(Ihandle *ih);

> **ih**: identifier of the element that activated the event.
>
> **Returns**: IUP_CLOSE will be processed.

### Notes

In Windows, WinUI, Haiku and GNUstep the main loop cannot wait on a descriptor.
There a single internal thread waits on it, and the callback is called on the main thread like in the other drivers.
Those drivers use POSTMESSAGE_CB of the element for that, so do not set it on a watch.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupTimer](iup_timer.md), [IupThread](iup_thread.md), [IupPostMessage](../func/iup_postmessage.md)
