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
