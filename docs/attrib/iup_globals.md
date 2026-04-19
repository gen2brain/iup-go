## Global Attributes

Global attributes are not associated with any particular element.
They represent and affect the global behavior of the toolkit.

To access global attributes, use the [IupGetGlobal](../func/iup_getglobal.md) and [IupSetGlobal](../func/iup_setglobal.md) functions.
In C, the functions [IupGetAttribute](../func/iup_getattribute.md) and [IupSetAttribute](../func/iup_setattribute.md) can also be used if you set the element handle to NULL.

------------------------------------------------------------------------

## General 

### LANGUAGE

The language used by some pre-defined dialogs.

Can have the values ENGLISH, PORTUGUESE, SPANISH, CZECH, RUSSIAN, GERMAN, FRENCH, CHINESE, JAPANESE or ITALIAN. Default: ENGLISH.
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

Available drivers: "Win32", "WinUI", "GTK", "GTK4", "Motif", "Qt", "FLTK", "EFL" and "Cocoa".

### APPID

Application identifier used by the desktop environment.
In GTK/Wayland, it maps to the XDG desktop file ID.
Also used by [IupConfig](../func/iup_config.md) as a last fallback when neither APP_NAME nor APPNAME is set.
Supported in GTK, GTK 4, Qt, FLTK, EFL and WinUI.

### APPNAME

Application name used by the system.
In Windows, it is used for the taskbar and tray. In macOS, it is used for the dock.
Also used by [IupConfig](../func/iup_config.md) as a fallback when APP_NAME is not set (APPNAME is checked first, then APPID).
Supported in Windows, macOS, Qt, EFL and WinUI.

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

### LASTERROR [Windows Only] (read-only)

If an error is found, returns a string with the system error description.
Available in Win32 and WinUI.

### UTF8MODE

By default, IUP uses strings in the current locale (See [FONT](iup_font.md) attribute).
To use UTF-8 strings, set this attribute to Yes. Default: NO.
Not supported in Motif and WinUI.

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

### SB_BGCOLOR [GTK, GTK 4 and Motif Only]

By default, the scrollbars will not be affected by the BGCOLOR in native controls.
If set to Yes, the system will try to render scrollbars in the same color of the BGCOLOR, but notice that this may affect scrollbars visibility.
This affects **IupCanvas**, **IupList**, **IupText** and **IupTree**.

### SHOWMENUIMAGES

Force the display of images in menus. Default: Yes.
Supported in GTK, GTK 4, Qt, EFL and WinUI.

### OVERLAYSCROLLBAR [GTK Only]

Allow the overlay scrollbar in **IupCanvas** to use a minimum space.
By default, IUP will use a regular scrollbar space even when overlay scrollbar is enabled in the system.
Supported in GTK 3 and GTK 4.

### GLOBALMENU [GTK Only]

Flag indicating that GTK is using a global menu instead of a per window menu.

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

### SINGLEINSTANCE

Restricts the number of instances of the application by using a name to identify it.
The first dialog that has a COPYDATA_CB callback will receive the command line of the second instance.
When consulted returns NULL if inside the second instance.
So usually in the application initialization after **IupOpen**, set SINGLEINSTANCE and then consult its value, if NULL abort the second instance by calling **IupClose** and returning from *main*.

In Windows uses a named mutex for detection and WM_COPYDATA for communication. In Linux/Unix uses D-Bus session bus name ownership. In macOS uses CFMessagePort.

## System Mouse and Keyboard

### CURSORPOS

Controls and returns the cursor position in absolute coordinates relative to the origin of the main screen.
The origin of the main screen is at the top-left corner, in Windows it is affected by the position of the Start Menu when it is at the top or left side of the screen.
Accept values in the format "X**x**Y" (in C "%dx%d), example "200x200".
In GTK and Motif also generates mouse motion messages.

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

### AUTOREPEAT [Motif, FLTK and EFL Only]

Turns on/off ("YES" or "NO") the auto-repeat of keyboard keys in the whole system.
May be used as an optimization in high performance applications.

### INPUTCALLBACKS

Turn on/off ("YES" or "NO") the global callbacks used to intercept global mouse and keyboard events.
The callbacks must be set using the [IupSetFunction](../func/iup_setfunction.md) function using the following names: **GLOBAL**[**KEYPRESS_CB**](../call/iup_keypress_cb.md), **GLOBAL**[**MOTION_CB**](../call/iup_motion_cb.md), **GLOBAL**[**BUTTON_CB**](../call/iup_button_cb.md) and **GLOBAL**[**WHEEL_CB**](../call/iup_wheel_cb.md) (Windows Only).
Their parameters are the same as the standard callbacks, but without the **Ihandle*** parameter.

## System Information

### SYSTEM (read-only)

Informs the current operating system. On UNIX, it is equivalent to the command "uname -s" (sysname). Some known names:

> - "macOS"
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
> - "Win10"

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

### DARKMODE (read-only)

Returns "1" if the system is currently in dark mode, "0" otherwise.

### WINDOWING (read-only)

Returns the native windowing system in use.
Can be: "DWM", "X11", "WAYLAND" or "QUARTZ".

### GTKVERSION (read-only) [GTK Only]

Returns the run time version of the GTK toolkit.
This is the version being used at the time of the IupOpen function was called by the application.
Available in GTK 3 and GTK 4.

### GTKDEVVERSION (read-only) [GTK Only]

Returns the development version of the GTK toolkit.
This is the version at the time the IUP library was compiled.
Available in GTK 3 and GTK 4.

### MOTIFVERSION (read-only) [Motif Only]

Returns the version of the run time Motif.

### MOTIFNUMBER (read-only) [Motif Only]

Returns the number of the Motif Version if full form, e.x: 2.2.3 = "2203".

### QTVERSION (read-only) [Qt Only]

Returns the run time version of the Qt toolkit.

### QTDEVVERSION (read-only) [Qt Only]

Returns the development version of the Qt toolkit.
This is the version at the time the IUP library was compiled.

### QTSTYLE (read-only) [Qt Only]

Returns the name of the current Qt widget style.

### FLTKVERSION (read-only) [FLTK Only]

Returns the run time version of the FLTK toolkit.

### EFLVERSION (read-only) [EFL Only]

Returns the run time version of the EFL toolkit.

### FLTKTHEME [FLTK Only]

Sets the FLTK visual theme. Can be set before or after creating dialogs.
Can also be set via the environment variable `IUP_FLTKTHEME`.

Accepted values:
- `"none"` or `"base"` - default FLTK appearance
- `"gtk+"` - GTK+ like appearance
- `"plastic"` - shiny plastic appearance
- `"gleam"` - modern gradient appearance
- `"oxy"` - oxygen-inspired appearance

When read, returns the current theme name (returns `"none"` if using the default).

### EFLTHEME [EFL Only]

Sets the EFL theme. Must be set before creating any dialogs.
Can also be set via the environment variable `IUP_EFLTHEME`.
The value is a theme name or path as accepted by `elm_theme_set`.

### EFLTHEMEDATA [EFL Only]

Sets embedded theme data from memory. Used together with EFLTHEMEDATALEN to load a theme from a binary buffer instead of a file. Set EFLTHEMEDATALEN first, then set EFLTHEMEDATA to the pointer to the `.edj` data. Can only be set once.

### EFLTHEMEDATALEN [EFL Only]

Sets the byte length of the embedded theme data buffer. Must be set before EFLTHEMEDATA. Can only be set once.

### EFLACCEL [EFL Only]

Sets the acceleration preference for new EFL windows. Must be set before creating any dialogs, or via the environment variable `IUP_EFLACCEL`.

Accepted values:
- `"gl"`, `"opengl"` - use OpenGL acceleration
- `"3d"` - use 3D acceleration
- `"hw"`, `"hardware"`, `"accel"` - use any hardware acceleration (best available)
- `"none"` - use software rendering

Additional depth, stencil, and MSAA options can be appended with colon separators. For example: `"gl:depth24:stencil8:msaa_mid"`. MSAA options are `"msaa"`, `"msaa_low"`, `"msaa_mid"` and `"msaa_high"`.

When read, returns the current acceleration preference string.

### EFLENGINE (read-only) [EFL Only]

Returns the name of the actual EFL rendering engine in use (e.g. `"opengl_x11"`, `"software_x11"`, `"wayland_egl"`, `"wayland_shm"`).
Only available after a dialog has been created, since the engine is selected at window creation time.

### GNUSTEPTHEME [GNUstep Only]

Sets the active GNUstep theme. Can be set before or after creating dialogs. Can also be set via the environment variable `IUP_GNUSTEPTHEME`. Only available when the Cocoa driver is built against GNUstep (Linux/BSD); on macOS the attribute is silently ignored.

The value is a theme name (e.g. `"Silver"`, `"Neos"`), or an absolute path to a `.theme` bundle. Setting the value to `NULL` or empty reverts to the built-in GNUstep theme.

Setting this attribute also refreshes the default color globals (`DLGBGCOLOR`, `DLGFGCOLOR`, etc.) from the newly activated theme, and writes the chosen name to the `GSTheme` key in the application's user defaults so the theme survives `NSUserDefaultsDidChangeNotification` (otherwise dragging a menu, resizing, or other operations that touch defaults would revert the theme to the built-in one).

When read, returns the current theme name, or NULL if the default theme is active.

### GSKRENDERER [GTK4 Only]

Sets the GSK renderer used by GTK4 for drawing. Must be set **before** any dialog is created (before `IupShow`). Can also be set via the `GSK_RENDERER` environment variable, but this attribute takes priority.

Possible values: `"gl"` (or `"opengl"`), `"vulkan"`, `"cairo"`.

When read, returns the renderer name that was set, or NULL if using the automatic default.

### WINUIVERSION (read-only) [WinUI Only]

Returns the WinUI version.

### WL_DISPLAY (read-only) [Wayland Only]

Returns the Wayland display (wl_display*).
Available in GTK, GTK 4, Qt, FLTK and EFL when running on Wayland.

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

### XSERVERVENDOR (read-only) [X11 Only]

X-Windows Server Vendor string.
Available in GTK, GTK 4, FLTK and Motif.

### XVENDORRELEASE (read-only) [X11 Only]

X-Windows Server Vendor release number.
Available in GTK, GTK 4, FLTK and Motif.

## Screen Information

### FULLSIZE (read-only)

Returns the full-screen size in pixels.

String in the "*width*x*height*" format.

### SCREENSIZE (read-only)

Returns the screen size in pixels available for dialogs, i.e., not including menu bars, task bars, etc.
In Motif, uses the `_NET_WORKAREA` property if available from the window manager, otherwise has the same value as the FULLSIZE attribute.
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

Returns the Desktop Window Manager Composition flag. Returns "YES" or "NO". Windows only.
Returns NULL if not supported.

### VIRTUALSCREEN (read-only)

Returns the virtual screen position and size in pixels.
It is the virtual space defined by all monitors in the system.
Not supported in Motif.

String in the "*x y width height*" format.

### MONITORSCOUNT (read-only)

Returns the number of monitors.
Not supported in Motif.

### MONITORSINFO (read-only)

Returns the position and size in pixels of all monitors.
Each monitor information is terminated by a "\n" character.
Not supported in Motif.

String in the "*x y width height*\n*x y width height*\n..." format.

## System Data 

### HINSTANCE (read-only) [Windows Only]

Returns a handle (HINSTANCE) that identifies the application in the native system.

### DLL_HINSTANCE [Windows Only]

Changes and returns a handle (HINSTANCE) that identifies the DLL where resources are stored.

### APPSHELL (read-only) [Motif Only]

Returns the shell Widget created by XtOpenApplication.
Also available in GTK and GTK 4.

### XDISPLAY (read-only) [X11 Only]

Returns the X-Windows Display.
Available in GTK, GTK 4, Qt (Qt6 only), FLTK, EFL and Motif.

### XSCREEN (read-only) [X11 Only]

Returns the X-Windows Screen.
Available in GTK, GTK 4, FLTK, EFL and Motif.

## Default Attributes

### DLGBGCOLOR

The default background color for all elements that have the background similar of the dialog.

### DLGFGCOLOR

The default foreground color for all elements that have text over the background of the dialog or similar.
Usually is "0 0 0" - black.

### MENUBGCOLOR

The default menu background color. Usually is "255 255 255" - white.
Supported in Windows, macOS, Qt, EFL and WinUI.

### MENUFGCOLOR

The system default menu foreground color. Usually is "0 0 0" - black.
Supported in Windows, macOS, Qt, EFL and WinUI.

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

### ACCENTCOLOR

The system accent color (read-only). This is the user-selected theme accent color
used for primary buttons, toggles, progress bars, focus indicators, and similar
UI elements. Unlike TXTHLCOLOR (which is the text selection background), this
reflects the toolkit's notion of the platform accent.

On some platforms this value coincides with TXTHLCOLOR because the toolkit derives text selection from the same source.

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
