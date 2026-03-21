## IupPlayInput

Reproduces all mouse and keyboard inputs from a given file.

### Parameters/Return

    int IupPlayInput(const char *filename);

**filename**: name of the file to be played.
NULL will stop playing. "" will pause and restart a file already being played.

Returns: IUP_NOERROR if successful, IUP_ERROR if failed to open the file for writing.
If already playing

### Notes

The file must have been saved using the **IupRecordInput** function.
Record mode will be automatically detected.

This function will start the play and return the control to the application.
If the file ends, all internal memory used to play the file will be automatically released.

It uses the MOUSEBUTTON global attribute to reproduce the events.
**IMPORTANT**: See the documentation of the [MOUSEBUTTON](../attrib/iup_globals.md) attribute for further details and current limitations.

The file must had been generated in the same operating system.
Screen size differences can exist, but if different themes are used then mouse precision will be affected.

### See Also

[MOUSEBUTTON](../attrib/iup_globals.md), [IupRecordInput](iup_recordinput.md)
