## IupPostMessage

Sends data to an element, that will be received by a callback when the main loop regain control.

It is expected to be thread safe.

### Parameters/Return

    void IupPostMessage(Ihandle* ih, const char* s, int i, double d, void* p);

**ih**: identifier of the interface element.\
**s**: string. Can be NULL. It will be internally duplicated if not NULL.\
**i**: integer number.\
**d**: floating point number.\
**p**: generic pointer.

### POSTMESSAGE_CB Callback

    int function(Ihandle *ih,  const char* s, int i, double d, void* p);

**ih**: identifier of the element that activated the event.\
**s**: string.\
**i**: integer number.\
**d**: floating point number.\
**p**: generic pointer.

### Notes

The variables are stored when the function is called, to be later passed to the callback.
It will work even for non-native elements.

If IupPostMessage is called, the callback must be defined or there will be a memory leak.

### Affects

All controls.
