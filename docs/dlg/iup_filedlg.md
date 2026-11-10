## IupFileDlg

Creates the File Dialog element. It is a predefined dialog for selecting files or a directory.
The dialog can be shown with the IupPopup function only.

### Creation

    Ihandle* IupFileDlg(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**ALLOWNEW**: Indicates if non-existent file names are accepted.
If equals "NO" and the user specifies a non-existing file, an alert dialog is shown.
Default: if the dialog is of type "OPEN", default is "NO"; if the dialog is of type "SAVE", default is "YES".
Not used when DIALOGTYPE=DIR.

**DIALOGTYPE**: Type of dialog (Open, Save or Directory). Can have values "OPEN", "SAVE" or "DIR".
Default: "OPEN".

> In Windows, when DIALOGTYPE=DIR the dialog shown is not the same dialog for OPEN and SAVE; this new dialog does not have the Help button neither filters. Also, this new dialog needs CoInitializeEx with COINIT_APARTMENTTHREADED (done in **IupOpen**), if the COM library was initialized with COINIT_MULTITHREADED prior to **IupOpen** then the new dialog will have limited functionality. In other drivers, the dialog is the same, but it only allows the user to select a directory.

**DIRECTORY**: Initial directory. When consulted after the dialog is closed and the user pressed the OK button, it will contain the directory of the selected file.
When set the last separator does not need to be specified, but when get the returned value will always contain the last separator.

> In Windows, if not defined and the application has used the dialog in the past, the path most recently used is selected as the initial directory. However, if an application is not run for a long time, its saved selected path is discarded. Also if not defined and the current directory contains any files of the specified filter types, the initial directory is the current directory. Otherwise, the initial directory is the "My Documents" directory of the current user. Otherwise, the initial directory is the Desktop folder.
>
> In other drivers, if not defined, the dialog opens in the current directory.

**EXTFILTER**: Defines several file filters. It has priority over FILTERINFO and FILTER.
Must be a text with the format "FilterInfo1|Filter1|FilterInfo2|Filter2|...". The list ends with character '|'.
Example: "Text files|*.txt;*.doc|Image files|*.gif;*.jpg;*.bmp|".
It is recommended to always add a less restrictive filter to the filter list, for example, "All Files|*.*".

**EXTDEFAULT**: default extension to be used if selected file does not have an extension.
The inspected extension will consider having the same number of characters of the default extension.
It must NOT include the period ".".

**FILE**: Name of the file initially shown in the "File Name" field in the dialog.
If contains a full path, then it is used as the initial directory and **DIRECTORY** is ignored.

**FILEEXIST** (read-only): Indicates if the file defined by the FILE attribute exists or not.
It is only valid if the user has pressed OK in the dialog.
Not set when DIALOGTYPE=DIR or **MULTIPLEFILES=YES**.

**FILTER**: String containing a list of file filters separated by ';' without spaces.
Example: "*.C;*.txt;test.*". In Motif, only the first filter is used.

**FILTERINFO**: Filter's description.
If not defined the filter itself will be used as its description.
Not supported in WebAssembly.

**FILTERUSED**: the index of the filter in EXTFILTER to use starting at 1.
It returns the selection made by the user. Set only if EXTFILTER is defined.
Not supported in WebAssembly.

**MULTIPLEFILES**: Allows the user to select multiple files when DIALOGTYPE=OPEN.
Can be "YES" or "NO". Default: "NO".
In QML, only one file can be selected when the platform has no native file dialog for Qt Quick.

**NOCHANGEDIR**: Indicates if the current working directory must be restored after the user navigation.
Default: "YES".

**NOOVERWRITEPROMPT**: do not prompt to overwrite an existent file when in "SAVE" dialog.
Default: "NO".

**NOPLACESBAR** [Win32 Only]: do not show the places bar.

[PARENTDIALOG](../attrib/iup_parentdialog.md): Makes the dialog be treated as a child of the specified dialog.

**PORTAL** [Unix Only]: When set to "YES", forces the use of the XDG Desktop Portal for the file dialog via D-Bus, instead of the native toolkit dialog.
When the global attribute SANDBOX is set, the portal is used automatically.
If the portal is not available, falls back to the native dialog.
Supported in GTK 3, FLTK, Motif, EFL and QML.
In GTK 4, the native GtkFileDialog already uses portals when appropriate.

**SHOWEDITBOX** [Win32 Only]: Show an edit box in the directory selection dialog (DIALOGTYPE=DIR).

**SHOWHIDDEN**: Show hidden files. Default: NO.
Not supported in Haiku, Android, iOS, WebAssembly and QML.

**SHOWPREVIEW**: A preview area is shown inside the file dialog. Can have values "YES" or "NO". Default: "NO".
Valid only if the FILE_CB callback is defined, use it to retrieve the file name and the necessary attributes to paint the preview area.
Supported in Win32, GTK 3, Motif, FLTK, macOS and Qt.

> Attributes that are valid inside the FILE_CB callback when status="PAINT":\
> **PREVIEWDC** (read-only): Returns the Device Context (HDC in Windows and GC in Unix)\
> **PREVIEWWIDTH** and **PREVIEWHEIGHT**: Returns the width and the height of the client rectangle for the preview area. Can also be set before the dialog is shown to define the size in pixels of the preview area (default is driver dependent). In Win32, only the height is used, the width follows the file list.\
>   Also the attributes WID, HWND, XWINDOW and XDISPLAY are valid and are relative to the preview area.
>
> If the attribute PREVIEWGLCANVAS is defined, then it is used as the name of an existent **IupGLCanvas** control to be mapped internally to the preview canvas. It is not a full **IupGLCanvas**: only the **IupGLCanvas** auxiliary functions and OpenGL calls can be used on the preview canvas. No **IupCanvas** attributes or callbacks are available.
> Supported in Windows (Win32), GTK 3, Motif, macOS and Qt (X11 only).

**STATUS** (read-only): Indicates the status of the selection made:

> "1": New file.\
> "0": Normal, existing file or directory.\
> "-1": Operation cancelled.

[TITLE](../attrib/iup_title.md): Dialog's title.

**VALUE** (read-only): Name of the selected file(s), or NULL if no file was selected.
If FILE is not defined this is used as the initial value.
When MULTIPLEFILES=YES it contains the path (but NOT the same value returned in DIRECTORY, it does not contain the last separator) and several file names separated by the '\|' character.
The file list ends with character '\|'.
When the user selects just one file, the directory and the file are not separated by '\|'.
For example:

    "/tecgraf/iup/test|a.txt|b.txt|c.txt|" (MULTIPLEFILES=YES and more than one file is selected)
    "/tecgraf/iup/test/a.txt" (only one file is selected)

**MULTIVALUECOUNT** (read-only): number of returned values when MULTIPLEFILES=YES.
It always includes the path, so if only 1 file is selected its value is 2.

**MULTIVALUEid** (read-only): almost the same sequence returned in VALUE when MULTIPLEFILES=YES but split in several attributes.
MULTIVALUE0 contains the path (same value returned in DIRECTORY), and MULTIVALUE1, MULTIVALUE2, ... contains each file name without the path.

**MULTIVALUEPATH**: force a full path in MULTIVALUE and VALUE attributes when MULTIPLEFILES=YES (id=0 will still contain the path of the first file).
In Windows and Motif, only files in the same folder may be selected, but in GTK, when using the "Recent Files", files from different folders can be selected.

**VALUEURI** [Android and iOS Only] (read-only) (non-inheritable): the URI of the selected file, the first one when MULTIPLEFILES=YES.
In Android, it is a `content://` URI that stays valid after the application restarts. Use it with `IupConfigRecentUpdate`.
In iOS, it is the `file://` URL of VALUE in OPEN mode and of the selected location in SAVE mode.

### Callbacks

**FILE_CB**: Action generated when a file is selected. Not called when DIALOGTYPE=DIR.
When MULTIPLEFILES=YES it is called only for one file.

    int function(Ihandle *ih, const char* file_name, const char* status);

**ih**: identifier of the element that activated the event.\
**file_name**: name of the file selected.\
**status**: describes the action. Can be:

> - "INIT" - when the dialog has started. file_name is NULL.
> - "FINISH" - when the dialog is closed. file_name is NULL.
> - "SELECT" - a file has been selected.
> - "OTHER" - an invalid file or a directory is selected. file_name is the one selected.
> - "OK" - the user pressed the OK button. If returns IUP_IGNORE, the action is refused and the dialog is not closed, if returns IUP_CONTINUE does the same, but if the FILE attribute is defined the current filename is updated.
> - "PAINT" - the preview area must be redrawn.\
>   Used only when SHOWPREVIEW=YES. If an invalid file or a directory is selected, file_name is NULL.
> - "FILTER" - when a filter is changed. [Windows Only]\
>   FILTERUSED attribute will be updated to reflect the change. If returns IUP_CONTINUE, the FILE attribute if defined will update the current filename.

[HELP_CB](../call/iup_help_cb.md): Action generated when the Help button is pressed.
Not supported in iOS, FLTK, EFL, Android, Haiku, WebAssembly and QML.

[BUTTON_CB](../call/iup_button_cb.md): Action generated when any mouse button is pressed or released over the preview canvas.

[MOTION_CB](../call/iup_motion_cb.md): Action generated when the mouse is moved over the preview canvas.

[WHEEL_CB](../call/iup_wheel_cb.md): Action generated when the mouse wheel is rotated over the preview canvas.

### Notes

The **IupFileDlg** is a native pre-defined dialog that is not altered by [IupSetLanguage](../func/iup_setlanguage.md).

The dialog is mapped only inside **IupPopup**, **IupMap** does nothing.

The [IupGetFile](iup_getfile.md) function creates and shows an **IupFileDlg**.

In Windows, the FILE and the DIRECTORY attributes also accept strings containing "/" as path separators, but the VALUE attribute will always return strings using the "\\" character.

In Windows, the dialog will be modal relative only to its parent or to the active dialog.

In Windows, when using UTF-8 strings (UTF8MODE=YES), attributes that return file names are still using the current locale, because the standard file I/O functions, like fopen, use ANSI file names.
To use UTF-8 file names, set the global attribute [UTF8MODE_FILE](../attrib/iup_globals.md) to YES.
It can be set before **IupPopup** and reset after, so only that call returns UTF-8.

In Windows (Win32), if FILE_CB and HELP_CB are not defined, and x,y are IUP_CENTER or IUP_CURRENT then it will use the newer Explorer interface.

When saving a file, the overwrite check is done before the FILE_CB callback is called with status=OK.
To add an extension inside FILE_CB with status=OK, the application must check whether the file with the extension exists and ask the user to replace it; if not, set FILE and return IUP_CONTINUE to keep the dialog open.

#### XDG Desktop Portals

In Unix, the file dialog can use the XDG Desktop Portal (org.freedesktop.portal.FileChooser) via D-Bus instead of the native toolkit dialog, see PORTAL.

Portal support is available in GTK 3, FLTK, Motif, EFL and QML.
In GTK 4, the native GtkFileDialog already integrates with portals when running in a sandboxed environment.
In Qt, QFileDialog automatically uses the XDG Desktop Portal through the Qt platform integration when running in a sandboxed environment.

#### Native Implementations

The underlying native widget per driver:

- **Win32**: GetOpenFileName / GetSaveFileName (and IFileDialog for DIALOGTYPE=DIR).
- **WinUI**: IFileOpenDialog / IFileSaveDialog (COM).
- **GTK 3**: GtkFileChooserDialog.
- **GTK 4**: GtkFileDialog.
- **Motif**: XmFileSelectionDialog.
- **Cocoa**: NSOpenPanel / NSSavePanel.
- **Cocoa Touch**: UIDocumentPickerViewController.
- **Qt**: QFileDialog.
- **QML**: FileDialog / FolderDialog.
- **FLTK**: Fl_File_Chooser.
- **EFL**: elm_fileselector.
- **Android**: Storage Access Framework (`ACTION_OPEN_DOCUMENT` / `ACTION_CREATE_DOCUMENT`).
- **Haiku**: BFilePanel.
- **WebAssembly**: the browser file picker; OPEN reads one file, SAVE downloads. No directory selection or filesystem paths. EXTFILTER and FILTER are mapped to the picker extension list, `*.*` accepting everything.

In Android and iOS, VALUE is a copy of the selected file, named as the file, in the application cache (Android) or temporary directory (iOS). In SAVE mode VALUE is an empty file that is copied to the selected location when the dialog is destroyed. DIALOGTYPE=DIR is not supported.
The picker uses one filter: the FILTERUSED group of EXTFILTER, or FILTER. All files are shown when a pattern is not `*.ext` or an extension has no known file type.

### Examples

    Ihandle *dlg = IupFileDlg();

    IupSetAttribute(dlg, "DIALOGTYPE", "OPEN");
    IupSetAttribute(dlg, "TITLE", "IupFileDlg Test");
    IupSetAttributes(dlg, "FILTER = \"*.bmp\", FILTERINFO = \"Bitmap Files\"");
    IupSetCallback(dlg, "HELP_CB", (Icallback)help_cb);

    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);

    if (IupGetInt(dlg, "STATUS") != -1)
    {
      printf("OK\n");
      printf("  VALUE(%s)\n", IupGetAttribute(dlg, "VALUE"));
    }
    else
      printf("CANCEL\n");

    IupDestroy(dlg);

|                                    |                                  |                                   |                                   |
|------------------------------------|----------------------------------|-----------------------------------|-----------------------------------|
| GTK                                | Qt                               | Win32                             | macOS                             |
| ![](../images/iupfiledlg_gtk3.png) | ![](../images/iupfiledlg_qt.png) | ![](../images/iupfiledlg_win.png) | ![](../images/iupfiledlg_mac.png) |

[Browse for Example Files](../../examples/)

### See Also

[IupMessage](iup_message.md), [IupListDialog](iup_listdialog.md), [IupAlarm](iup_alarm.md), [IupGetFile](iup_getfile.md), [IupPopup](../func/iup_popup.md)
