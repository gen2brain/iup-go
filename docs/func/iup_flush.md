## IupFlush

Processes all pending messages in the message queue.

### Parameters/Return

    void IupFlush(void);

### Notes

When you change an attribute of a certain element, the change may not take place immediately.
For this update to occur faster than usual, call **IupFlush** after the attribute is changed.

*Important*: A call to this function may cause other callbacks to be processed before it returns.

In Motif, if the X server sent an event which is not yet in the event queue, after a call to **IupFlush** the queue might not be empty.
