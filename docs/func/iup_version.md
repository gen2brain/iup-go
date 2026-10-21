## IupVersion

Returns a string with the IUP version number.

### Parameters/Return

    char* IupVersion(void);
    int IupVersionNumber(void);
    void IupVersionShow(void);

**Returns:** the version number including the bug fix. The defines only includes the major and minor numbers.
For example, "3.32".

**IupVersionShow** shows a popup dialog with the IUP version and date, the DRIVER, SYSTEM, SYSTEMVERSION, toolkit version, SCREENSIZE, SCREENDPI and DEFAULTFONT globals, and the additional IUP libraries opened: IupControls, IupGLCanvas with the OpenGL vendor, renderer and version, IupWebBrowser, IupPlot and IupMedia.
It is also shown by IupOpen when the environment variable IUP_VERSION is set to YES.
