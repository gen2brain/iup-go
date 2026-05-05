## IupRecordInput

Records all mouse and keyboard inputs in a file for later reproduction.

### Parameters/Return

    int IupRecordInput(const char *filename, int mode);

**filename**: name of the file to be saved. NULL will stop recording.\
**mode**: flag for controlling the file generation. Can be: IUP_RECBINARY or IUP_RECTEXT.

**Returns:** IUP_NOERROR if successful, IUP_ERROR if failed to open the file for writing.

### Notes

Any existing file will be replaced.

Must stop recording before exiting the application.

It uses the global callbacks enabled by the INPUTCALLBACKS global attribute.

Mouse position is relative to the top left corner of the screen, and it is independent of the controls and dialogs being manipulated.

**IupPlayInput** can use the generated file to reproduce the same events.

### Driver Support

Implemented on every driver. Playback support is more restricted, see [IupPlayInput](iup_playinput.md).

### See Also

[INPUTCALLBACKS](../attrib/iup_globals.md#inputcallbacks), [IupPlayInput](iup_playinput.md)
