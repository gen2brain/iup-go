## IupSetLanguage

Sets the language name used by some pre-defined dialogs.
It can also be changed using the global attribute LANGUAGE.

### Parameters/Return

    void IupSetLanguage(const char *name); 


**name**: Language name to be used. Can have one of the following values:

- "ENGLISH"
- "PORTUGUESE"
- "SPANISH"

default: "ENGLISH".

### Affects

All elements that have pre-defined texts.
The pre-defined texts will be stored using [IupSetLanguageString](iup_setlanguagestring.md).

The native dialogs like **IupFileDlg** will always be displayed in the system language.

Even if the language is not supported (meaning its pack of pre-defined strings are not defined) the new language name will be successfully stored so you can set your own strings and return a coherent value, and the current defined string will not be changed.

Here is a list of the pre-defined string names:

    IUP_ERROR
    IUP_ATTENTION
    IUP_YES
    IUP_NO
    IUP_INVALIDDIR
    IUP_FILEISDIR
    IUP_FILENOTEXIST
    IUP_FILEOVERWRITE
    IUP_CREATEFOLDER
    IUP_NAMENEWFOLDER
    IUP_SAVEAS
    IUP_OPEN
    IUP_SELECTDIR
    IUP_OK
    IUP_CANCEL
    IUP_RETRY
    IUP_APPLY
    IUP_RESET
    IUP_GETCOLOR
    IUP_HELP
    IUP_RED
    IUP_GREEN
    IUP_BLUE
    IUP_HUE
    IUP_SATURATION
    IUP_INTENSITY
    IUP_OPACITY
    IUP_PALETTE
    IUP_TRUE
    IUP_FALSE
    IUP_FAMILY
    IUP_STYLE
    IUP_SIZE
    IUP_SAMPLE
    IUP_ERRORFILEOPEN
    IUP_ERRORFILESAVE

### Examples

    #include "iup.h"

    void main(void)
    {
      IupOpen();
      IupSetLanguage("ENGLISH"); 
      IupMessage("IUP Language", IupGetLanguage());
      IupClose();
    }

### See Also

[IupGetLanguage](iup_getlanguage.md), [IupSetLanguageString](iup_setlanguagestring.md), [LANGUAGE](../attrib/iup_globals.md)
