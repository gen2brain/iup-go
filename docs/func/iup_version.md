## IupVersion

Returns a string with the IUP version number.

### Parameters/Return

    char* IupVersion(void);
    int IupVersionNumber(void);
    void IupVersionShow(void);

**Returns:** the version, for example "4.0.0". **IupVersionNumber** returns it as major × 100000 + minor × 1000 + patch, 400000 for 4.0.0.

**IupVersionShow** shows a popup dialog with the IUP version and date, the DRIVER, WINDOWING, SYSTEM, SYSTEMVERSION, toolkit version, SCREENSIZE, SCREENDPI and DEFAULTFONT globals, and the OpenGL vendor, renderer and version once an IupGLCanvas was made current.
It is also shown by IupOpen when the environment variable IUP_VERSION is set to YES.
