## ICON

Dialog's icon. This icon will be used when the dialog is minimized among other places by the native system.

### Value

Name of a IUP image.

Default: NULL

### Notes

Icon sizes are usually less than or equal to 32x32.

In Windows, the SDK recommends that cursors and icons should be implemented as resources rather than created at run time.
We suggest using an icon with at least 3 images: 16x16 32bpp, 32x32 32 bpp and 48x48 32 bpp.

In WinUI, the icon is set via WM_SETICON on the XAML Islands host HWND (same as Win32).

In macOS, the icon is set as the application icon via NSApp setApplicationIconImage.

In GTK 3, the icon is set using gdk_pixbuf on the window.

In GTK 4, only named theme icons are supported for window icons (via gtk_window_set_icon_name).
The icon name should match a theme icon installed on the system.

In Motif, the icon is set via the X11 ICCCM `XmNiconPixmap` resource on the shell window.

In Qt, the icon is set using QIcon on the window.

In FLTK, the icon is set via `Fl_Window::icon(Fl_RGB_Image*)`.

In EFL, the icon is set via `efl_ui_win_icon_object_set` on the window.

In Android and iOS, the icon is shown in the title bar next to the title.

In Haiku, BWindow has no per-window icon. ICON writes the running executable's `BAppFileInfo` icon (large + mini sizes) and signals Deskbar to refresh its team entry; the result is visible in Deskbar and the Twitcher. Effect is process-wide: setting ICON on multiple dialogs replaces the previous one.

#### Wayland and .desktop Files

On Wayland, the window icon displayed in the taskbar and window switcher is determined by the application's `.desktop` file, not by the ICON attribute set at runtime.
The compositor uses the application ID to find the matching `.desktop` file and its `Icon=` entry.

To ensure your application icon is displayed correctly on Wayland:

1. Create a `.desktop` file (e.g., `com.example.myapp.desktop`) and install it in the appropriate location (typically `~/.local/share/applications/` or `/usr/share/applications/`).
2. Set the `Icon=` field in the `.desktop` file to the name of an icon installed in the icon theme or to an absolute path.
3. Set the global attribute [APPID](iup_globals.md#appid) to match the `.desktop` file name (without the `.desktop` extension) before creating any dialogs:

```
IupSetGlobal("APPID", "com.example.myapp");
```

On X11 and other platforms, the ICON attribute still works as expected.

Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.

### Affects

[IupDialog](../dlg/iup_dialog.md)

### See Also

[IupImage](../elem/iup_image.md)
