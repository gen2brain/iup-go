## IupFlush

Processes all pending messages in the message queue.

### Parameters/Return

    void IupFlush(void);

### Notes

When an attribute of an element is changed, the change may not take place immediately.
For this update to occur faster than usual, call **IupFlush** after the attribute is changed.

A call to this function may cause other callbacks to be processed before it returns.

In Motif, if the X server sent an event which is not yet in the event queue, after a call to **IupFlush** the queue might not be empty.

In Android and iOS, it drains messages currently queued on the main thread (same mechanism as [IupLoopStep](iup_loopstep.md)).
