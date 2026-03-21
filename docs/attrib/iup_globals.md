## Global Attributes

Global attributes are not associated with any particular element.
They represent and affect the global behavior of the toolkit.

To access global attributes, use the [IupGetGlobal](../func/iup_getglobal.md) and [IupSetGlobal](../func/iup_setglobal.md) functions.
In C, the functions [IupGetAttribute](../func/iup_getattribute.md) and [IupSetAttribute](../func/iup_setattribute.md) can also be used if you set the element handle to NULL.

------------------------------------------------------------------------

## General 

### LANGUAGE

The language used by some pre-defined dialogs.

Can have the values of ENGLISH, SPANISH and PORTUGUESE. Default: ENGLISH.
It can also be set by [IupSetLanguage](../func/iup_setlanguage.md).

### VERSION (read-only)

Returns the name of IUP's version.

The value follows the "major.minor.micro" format, major referring to broader changes, minor referring to smaller changes, and micro referring to corrections only.
Ex.: "1.7.2".

### COPYRIGHT (read-only)

Returns the IUP's copyright.

 Ex: "Copyright (C) 1994-2014 Tecgraf/PUC-Rio".

### DRIVER (read-only)

Informs the current driver being used.

Two drivers are available now, one for each platform: "GTK", "Motif" and "Win32".

## System Control

### LOCKLOOP

When the last visible dialog is closed, the **IupExitLoop** function is called.
To avoid that, set LOCKLOOP=YES before hiding the last dialog. Possible values: "YES" or "NO".
Default: "NO".

### EXITLOOP

Disable the **IupExitLoop** function when **IupMainLoopLevel** is 1.
Used when the application runs secondary dialogs that behave as full applications but sharing the same IUP environment.
Possible values: "YES" or "NO". Default: "YES".

### CUSTOMQUITMESSAGE [Windows Only]

Enable a custom quit message instead of using WM_QUIT.

### LASTERROR [Windows Only]  (read-only)

If an error is found, returns a string with the system error description.

### UTF8MODE [Windows and GTK Only]

By default, IUP uses strings in the current locale (See [FONT](iup_font.md) attribute).
To use UTF-8 strings, set this attribute to Yes. Default: NO.

### UTF8MODE_FILE [Windows Only]

By default, IUP uses file names in the current locale, even when UTF8MODE=Yes.
To use UTF-8 file names in Windows, set this attribute to Yes. Default: NO.

The main places affected by this attribute are **IupFileDlg** attributes, such as VALUE, FILE and DIRECTORY, and the DROPFILES_CB callback.

Notice that IUP use the **fopen** based functions to read and write files.
In Windows **fopen** expects the filename string in the **ANSI** encoding by default.
If your filename, including the path, has characters that cannot be converted to ANSI, **fopen** will fail to open the file.
In Windows, we could use **_wfopen** combined with UTF-8, but this is a Microsoft only function and most of **fopen** usage in these libraries are in portable modules.
*This is an IUP limitation in Windows.*

The simple workaround is to not use special characters in folders or files name in Windows...
Legacy applications will also have the same problem.

Another option is to call:

     setlocale(LC_ALL, ".UTF8"); 

But it will work for **fopen** only in Visual Studio 2017 or newer Microsoft compilers (**setlocale** will return NULL on other compilers).
**fopen** will successfully open the file if the filename is a UTF-8 string with special characters.
So you will be able to set both UTF8MODE and UTF8MODE_FILE to YES.

If you decide to use this feature, another interesting option is to set the console code page to UTF-8 executing "chcp 65001" on the command line.
This will allow your **printf** output to be properly displayed when using UTF-8 strings.
This feature actually works for all Microsoft compilers in Windows, and for MingW, even when **setlocale** returns NULL.
Notice that some font packages must be installed for this to fully work for all characters (for instance, Chinese, Japanese and Korean, along with some symbols too).

As a complement, from **fopen** documentation in MSDN: "You can use either forward slashes (/) or backslashes (\\) as the directory separators in a path".

### DEFAULTPRECISION

The default number of decimal places used in floating point output by some controls (**IupMatrixEx** and **IupGetParam**).
Local attributes may overwrite the default. Default: 2.

### DEFAULTDECIMALSYMBOL

Symbol used for decimal separator in numeric values used in floating point output by some controls (**IupMatrixEx, IupGetParam** and **IupPlot**).
Can be "." or "," only. Default uses the one defined by the system locale.

### SB_BGCOLOR [GTK and Motif Only]

By default, the scrollbars will not be affected by the BGCOLOR in native controls.
If set to Yes, the system will try to render scrollbars in the same color of the BGCOLOR, but notice that this may affect scrollbars visibility.
This affects **IupCanvas**, **IupList**, **IupText** and **IupTree**.

### SHOWMENUIMAGES [GTK Only]

Force the display of images in menus. Default: Yes

### OVERLAYSCROLLBAR [GTK Only]

Allow the overlay scrollbar in **IupCanvas** to use a minimum space.
By default, IUP will use a regular scrollbar space even when overlay scrollbar is enabled in the system.

### GLOBALMENU [GTK Only]

Flag indicating that GTK is using a global menu instead of a per window menu.

### GLOBALLAYOUTDLGKEY

Flag to enable the global keys Alt+Ctrl+Shift+L to display the **IupLayoutDialog**.

### GLOBALLAYOUTRESIZEKEY

Flag to enable the global keys Ctrl+'+' and Ctrl+'-' that change the FONTSIZE and refresh the layout of the dialog.
If element sizes are NOT set using RASTERSIZE, their sizes will be automatically increased and decreased.
Images are not changed.

### IMAGEAUTOSCALE

If defined, automatically scale all images, except stock images, by a given real factor.
If "DPI" value is used, then the factor will be automatically calculated from the ratio between screen resolution and IMAGESDPI.
The minimum resulted size when automatically resized is 24 pixels height.

### IMAGESDPI

Defines the resolution of the images of the application. Common values are 96, 144, 192, and 288 DPI.
Default: 96. Used when IMAGEAUTOSCALE=DPI.

### IMAGESTOCKAUTOSCALE

Stock images are automatically scaled by default. Default: Yes.

### IMAGESTOCKSIZE

Force the size for stock images by controlling the image height.
If that image size is not available, the stock image is resized to match the given size.
By default, the size will be automatically calculated from the screen resolution: if res <= 144 DPI size = 24, if 192 DPI size = 32, else size = 48.
The minimum resulted size when automatically resized is 24 pixels height.

### PROCESSWINDOWSGHOSTING [Windows Only]

If set to NO will disable the window ghosting feature for the duration of the process, cannot be enabled again.
When disabled, the application dialogs cannot be moved or resized while application that is not responding, also the "Not Responding" display at the application tittle bar will not be done anymore.

### SINGLEINSTANCE [Windows Only]

Restricts the number of instances of the application by using a name to identify it.
The value must also be a partial match to the title of a dialog that will receive the COPYDATA_CB callback with the command line of the second instance.
When consulted returns NULL if inside the second instance.
So usually in the application initialization after **IupOpen**, set SINGLEINSTANCE and then consult its value, if NULL abort the second instance by calling **IupClose** and returning from *main*.

## System Mouse and Keyboard

### CURSORPOS

Controls and returns the cursor position in absolute coordinates relative to the origin of the main screen.
The origin of the main screen is at the top-left corner, in Windows it is affected by the position of the Start Menu when it is at the top or left side of the screen.
Accept values in the format "X**x**Y" (in C "%dx%d), example "200x200".
In GTK and Motif also generates mouse motion messages. (since GTK 2.8)

### MOUSEBUTTON (write-only)

Simulates a mouse button press, release or motion at the given coordinates.
The position is in absolute coordinates relative to the top-left corner of the screen.
Accept values in the format "X**x**Y button state" (in C "%dx%d %c %d"), example "200x200 1 1".
**button** can be one of the IUP_BUTTON1, ... definitions.
**state** can be 2=double click, 1=pressed, 0=released, or -1=motion. The cursor position is always updated.
In Windows button can be 'W' and state=delta, so a wheel button scroll is simulated.

**IMPORTANT**: **not fully working**. In Windows and GTK, menu items are not activated.
Although submenus open, menu items even in the menu bar are not activated.
In Windows, inside the **IupFileDlg** dialog, clicks in the folder navigation list are not correctly interpreted.
In Motif click and drag operations are not performed.

### SHIFTKEY (read-only)

Returns the state of the Shift keys (left and right). Possible values: "ON" or "OFF".

### CONTROLKEY (read-only)

Returns the state of the Control keys (left and right). Possible values: "ON" or "OFF".

### MODKEYSTATE (read-only)

Returns the state of the keyboard modifier keys: Shift, Ctrl, Alt and sYs(Win/Apple).
In the format of 4 characters: "SCAY". When not pressed, the respective letter is replaced by a space " ".

### KEYPRESS (write-only)

Sends a key press message to the element with the focus. The value is a key code.
See the [Keyboard Codes](iup_keyboard_codes.md) table for a list of the possible values.

### KEYRELEASE (write-only)

Sends a key release message to the element with the focus. The value is a key code.
See the [Keyboard Codes](iup_keyboard_codes.md) table for a list of the possible values.

### KEY (write-only)

Sends a key press and a key release messages to the element with the focus. The value is a key code.
See the [Keyboard Codes](iup_keyboard_codes.md) table for a list of the possible values.

### AUTOREPEAT [Motif Only]

Turns on/off  ("YES" or "NO") the auto-repeat of keyboard keys in the whole system.
May be used as an optimization in high performance applications.

### INPUTCALLBACKS

Turn on/off ("YES" or "NO") the global callbacks used to intercept global mouse and keyboard events.
The callbacks must be set using the [IupSetFunction](../func/iup_setfunction.md) function using the following names: **GLOBAL**[**KEYPRESS_CB**](../call/iup_keypress_cb.md), **GLOBAL**[**MOTION_CB**](../call/iup_motion_cb.md), **GLOBAL**[**BUTTON_CB**](../call/iup_button_cb.md) and **GLOBAL**[**WHEEL_CB**](../call/iup_wheel_cb.md) (Windows Only).
Their parameters are the same as the standard callbacks, but without the **Ihandle*** parameter.

## System Information

### SYSTEM (read-only)

Informs the current operating system. On UNIX, it is equivalent to the command "uname -s" (sysname).
On Windows, it identifies if you are on Windows 2000, Windows XP or Windows Vista. Some known names:

> - "MacOS"
> - "FreeBSD"
> - "Linux"
> - "SunOS"
> - "Solaris"
> - "IRIX"
> - "AIX"
> - "HP-UX"
> - "Win2K"
> - "WinXP"
> - "Vista"
> - "Win7"
> - "Win8"
>
> Notice that "Windows 8.1" will normally be detected as "Windows 8", unless a special Manifest is used. See [MSDN](http://msdn.microsoft.com/EN-US/library/windows/desktop/dn481241(v=vs.85).aspx) for more information.

### SYSTEMVERSION (read-only)

Informs the current operating system version number.

On UNIX, it is equivalent to the command  "uname -r" (release).
On Windows, it identifies the system version number and service pack version.
On macOS is system version.

### SYSTEMLANGUAGE (read-only)

Returns a text with a description of the system language.

### SYSTEMLOCALE (read-only)

Returns a text with a description of the system locale.

### SCROLLBARSIZE (read-only)

Returns the width of the vertical scrollbar (the same as the height of the horizontal scrollbar).

### COMCTL32VER6 (read-only) [Windows Only]

Returns Yes or No if the Windows common controls are using Visual Styles or not.

### GTKVERSION (read-only) [GTK Only]

Returns the run time version of the GTK toolkit.
This is the version being used at the time of the IupOpen function was called by the application.

### GTKDEVVERSION (read-only) [GTK Only]

Returns the development version of the GTK toolkit.
This is the version at the time the IUP library was compiled.

### MOTIFVERSION (read-only) [Motif Only]

Returns the version of the run time Motif.

### MOTIFNUMBER (read-only) [Motif Only]

Returns the number of the Motif Version if full form, e.x: 2.2.3 = "2203".

### COMPUTERNAME (read-only)

Returns the hostname.

### TOUCHREADY (read-only) [Windows Only]

Informs if touch is supported by the current user interface.

### USERNAME (read-only)

Returns the user logged in.

### EXEFILENAME (read-only)

Returns the filename of the executable with full path.
Depending on how the program is executed the argv[0] not always has the full executable path.

### GL_VERSION (read-only)

Returns the OpenGL version. Available only after the first call to [IupGLMakeCurrent](../ctrl/iup_glcanvas.md).

### GL_VENDOR (read-only)

Returns the OpenGL vendor information. Available only after the first call to [IupGLMakeCurrent](../ctrl/iup_glcanvas.md).

### GL_RENDERER (read-only)

Returns the OpenGL renderer information.
Available only after the first call to [IupGLMakeCurrent](../ctrl/iup_glcanvas.md).

### XSERVERVENDOR (read-only) [GTK and Motif Only]

X-Windows Server Vendor string.

### XVENDORRELEASE (read-only) [GTK and Motif Only]

X-Windows Server Vendor release number.

## Screen Information

### FULLSIZE (read-only)

Returns the full-screen size in pixels.

String in the "*width*x*height*" format.

### SCREENSIZE (read-only)

Returns the screen size in pixels available for dialogs, i.e., not including menu bars, task bars, etc.
In Motif has the same value as the FULLSIZE attribute.
The main screen size does not include additional monitors.

String in the "*width*x*height*" format.

### SCREENDEPTH (read-only)

Returns the screen depth in bits per pixel.

### SCREENDPI (read-only)

Returns a real value with the screen resolution in pixels per inch (same as dots per inch - DPI).

### TRUECOLORCANVAS (read-only)

Indicates if the display allows creating TrueColor (> 8bpp) **IupCanvas** controls, even if PseudoColor is the default, i.e., even if SCREENDEPTH<=8.
Returns "YES" or "NO". Useful in Motif.

### DWM_COMPOSITION (read-only) [Windows Only]

Returns the Desktop Window Manager Composition flag. Returns "YES" or "NO". Works only in Windows Vista and newer.
Returns NULL if not supported.

### VIRTUALSCREEN (read-only) [Windows and GTK Only]

Returns the virtual screen position and size in pixels.
It is the virtual space defined by all monitors in the system.

String in the "*x y width height*" format.

### MONITORSCOUNT (read-only) [Windows and GTK Only]

Returns the number of monitors.

### MONITORSINFO (read-only) [Windows and GTK Only]

Returns the position and size in pixels of all monitors.
Each monitor information is terminated by a "\n" character.

String in the "*x y width height*\n*x y width height*\n..." format.

## System Data 

### HINSTANCE (read-only) [Windows Only]

Returns a handle (HINSTANCE) that identifies the application in the native system.

### DLL_HINSTANCE [Windows Only]

Changes and returns a handle (HINSTANCE) that identifies the DLL where resources are stored.

### APPSHELL (read-only) [Motif Only]

Returns the shell Widget created by XtOpenApplication.

### XDISPLAY (read-only) [GTK and Motif Only]

Returns the X-Windows Display.

### XSCREEN (read-only) [GTK and Motif Only]

Returns the X-Windows Screen.

## Default Attributes

### DLGBGCOLOR

The default background color for all elements that have the background similar of the dialog.

### DLGFGCOLOR

The default foreground color for all elements that have text over the background of the dialog or similar.
Usually is "0 0 0" - black.

### MENUBGCOLOR  [Windows Only]

The default menu background color. Usually is "255 255 255" - white.

### MENUFGCOLOR  [Windows Only]

The system default menu foreground color. Usually is "0 0 0" - black.

### TXTBGCOLOR

The default background color for editable text, also used by lists and tree.
Usually is "255 255 255" - white.

### TXTFGCOLOR

The default foreground color for editable text, also used by lists and tree.
Usually is "0 0 0" - black.

### TXTHLCOLOR

The default highlight color for editable text, also used by lists and tree.
The highlight color is used when the text is selected. Usually is "0 0 0" in Motif, and "51 153 255" in Windows.
It can be changed only in **IupTree**, and only in Windows and Motif.
But it can be used for drawing selected areas in custom controls.

### LINKFGCOLOR

The default foreground color for linked text. In GTK and Motif is "0 0 238".

### DEFAULTFONT

The default font used by all elements, except for menus.

### DEFAULTFONTFACE

Auxiliary attribute to retrieve and set the default font face used by all elements.
It retrieves the typeface from DEFAULTFONT. When changed will actually change the DEFAULTFONT.

### DEFAULTFONTSIZE

Auxiliary attribute to retrieve and set the default font size used by all elements.
It retrieves the size from DEFAULTFONT. When changed will actually change the DEFAULTFONT.

### DEFAULTFONTSTYLE

Auxiliary attribute to retrieve and set the default font style used by all elements.
It retrieves the style from DEFAULTFONT. When changed will actually change the DEFAULTFONT.

### DEFAULTBUTTONPADDING

Default button padding used in pre-defined dialogs. Default: 12x4".

### DEFAULTTHEME

Applies a default theme for all controls. See [THEME](iup_theme.md) attribute for more information.
