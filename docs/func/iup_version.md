## IupVersion

Returns a string with the IUP version number.

### Parameters/Return

    char* IupVersion(void);
    int IupVersionNumber(void);
    void IupVersionShow(void);

**Returns:** the version number including the bug fix. The defines only includes the major and minor numbers.
For example, "2.7.1".

**IupVersionShow** shows a popup dialog with IUP version information and more.
This is a debug dialog with lots of information of additional libraries too.

### Definitions

```
IUP_NAME            "IUP - Portable User Interface"
IUP_COPYRIGHT       "Copyright (C) 1994-2011 Tecgraf/PUC-Rio."
IUP_DESCRIPTION     "Multi-platform toolkit for building graphical user interfaces."
IUP_VERSION         "3.5"
IUP_VERSION_NUMBER  305000
IUP_VERSION_DATE    "2011/04/26"
```
