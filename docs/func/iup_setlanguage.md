## IupSetLanguage

Sets the language name used by some pre-defined dialogs.
It can also be changed using the global attribute LANGUAGE.

### Parameters/Return

    void IupSetLanguage(const char *name);

**name**: Language name to be used. Can have one of the following values:

- "ENGLISH"
- "PORTUGUESE"
- "SPANISH"
- "CZECH"
- "RUSSIAN"
- "GERMAN"
- "FRENCH"
- "CHINESE"
- "JAPANESE"
- "ITALIAN"

Default: "ENGLISH".

### Affects

All elements that have pre-defined texts.
The pre-defined texts will be stored using [IupSetLanguageString](iup_setlanguagestring.md).

The native dialogs like **IupFileDlg** will always be displayed in the system language.

When the language is not supported (its pack of pre-defined strings is not defined), the new language name is still stored and the currently defined strings are not changed, so the application can set its own strings.

The pre-defined string names are:

    IUP_ALLPAGES
    IUP_APPLY
    IUP_ATTENTION
    IUP_BLUE
    IUP_CANCEL
    IUP_CLOSE
    IUP_CONTINUE
    IUP_COPIES
    IUP_COPY
    IUP_COPYTOCLIPBOARD
    IUP_CREATEFOLDER
    IUP_ERROR
    IUP_ERRORFILEOPEN
    IUP_ERRORFILESAVE
    IUP_EXIT
    IUP_FALSE
    IUP_FAMILY
    IUP_FILE
    IUP_FILEISDIR
    IUP_FILENOTEXIST
    IUP_FILEOVERWRITE
    IUP_FONTDLG
    IUP_GETCOLOR
    IUP_GREEN
    IUP_HELP
    IUP_HUE
    IUP_INTENSITY
    IUP_INVALIDDIR
    IUP_LANDSCAPE
    IUP_NAMENEWFOLDER
    IUP_NO
    IUP_NOTFOUND
    IUP_OK
    IUP_OPACITY
    IUP_OPEN
    IUP_ORIENTATION
    IUP_PAGERANGE
    IUP_PAGES
    IUP_PALETTE
    IUP_PAPERSIZE
    IUP_PORTRAIT
    IUP_PRINT
    IUP_PRINTDLG
    IUP_PRINTER
    IUP_PRINTTOFILE
    IUP_RED
    IUP_RESET
    IUP_RETRY
    IUP_SAMPLE
    IUP_SATURATION
    IUP_SAVEAS
    IUP_SELECTDIR
    IUP_SIZE
    IUP_STYLE
    IUP_TRUE
    IUP_YES

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

[IupGetLanguage](iup_getlanguage.md), [IupSetLanguageString](iup_setlanguagestring.md), [LANGUAGE](../attrib/iup_globals.md#language)
