## IupNotify

Creates a desktop notification. Displays a system notification with a title, body text, optional icon, and optional action buttons.
Each notification should be destroyed using [IupDestroy](../func/iup_destroy.md).

### Creation

    Ihandle* IupNotify(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

#### Content

**TITLE** (non-inheritable): The notification title text.

**BODY** (non-inheritable): The notification body text.

**ICON** (non-inheritable): Image name for the notification icon.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
See also [IupImage](iup_image.md).

**SUBTITLE** [macOS Only] (non-inheritable): The notification subtitle, displayed below the title.

#### Action Buttons

**ACTION1**, **ACTION2**, **ACTION3**, **ACTION4** (non-inheritable): Labels for up to four action buttons displayed in the notification.
Not supported in Windows (Win32), but supported in WinUI.

#### Behavior

**TIMEOUT** (non-inheritable): Auto-dismiss timeout in milliseconds.
Default: "-1" (use the system default).
Only supported on Linux/Unix (D-Bus). Not supported in Windows (Win32), WinUI, or macOS.

**SILENT** (non-inheritable): Suppress the notification sound.

#### Control

**SHOW** (write-only) (non-inheritable): Set to "YES" to display the notification.
All content attributes (TITLE, BODY, ICON, etc.) should be set before showing.

**CLOSE** (write-only) (non-inheritable): Set to "YES" to close the notification.

**ID** (read-only) (non-inheritable): Returns the notification identifier assigned by the system after a successful SHOW.

#### Platform-Specific

**URGENCY** [Linux/Unix Only] (non-inheritable): Notification urgency level.
Can be "0" (low), "1" (normal), or "2" (high). Default: "1".

**APPICON** [Linux/Unix Only] (non-inheritable): Application icon name used as a fallback when ICON is not set.

**TRANSIENT** [Linux/Unix Only] (non-inheritable): When set, the notification is transient and may be skipped from the notification history.

**PERMISSION** [macOS Only] (read-only) (non-inheritable): Returns the current notification permission status.
Can be "GRANTED", "DENIED", or "NOTDETERMINED".

**REQUESTPERMISSION** [macOS Only] (write-only) (non-inheritable): Set to "YES" to request notification permission from the user.
On macOS, notifications require explicit user permission.

**THREADID** [macOS Only] (non-inheritable): Notification thread identifier used to group related notifications together.

### Callbacks

**NOTIFY_CB**: Called when the user interacts with the notification.

    int function(Ihandle *ih, int action_id);

**ih**: identifier of the element that activated the event.\
**action_id**: 0 for the default action (clicking the notification body), 1-4 for ACTION1 through ACTION4 buttons.

**Returns**: IUP_CLOSE will be processed.

**CLOSE_CB**: Called when the notification is closed.

    int function(Ihandle *ih, int reason);

**ih**: identifier of the element that activated the event.\
**reason**: 0 for unknown, 1 for timeout/auto-dismiss, 2 for user-dismissed.

**Returns**: IUP_CLOSE will be processed.

**ERROR_CB**: Called when an error occurs during notification initialization or display.

    int function(Ihandle *ih, char *error_msg);

**ih**: identifier of the element that activated the event.\
**error_msg**: description of the error.

### Notes

IupNotify does not create a visible IUP element. It is a non-interactive control that interfaces with the operating system's native notification service.

All content attributes should be set before calling SHOW.
The notification is displayed asynchronously and callbacks are dispatched through the IUP event loop.

On Linux/Unix, notifications are sent via D-Bus using the org.freedesktop.Notifications service.
This is used by all Unix-based drivers (GTK, GTK 4, Qt, Motif, EFL).
The notification server may not support all features; capabilities like action buttons and persistence depend on the running notification daemon.

In Windows (Win32) uses Shell Notification balloon (tray icon), in WinUI uses Windows App SDK toast notifications (AppNotificationManager), and in macOS uses User Notifications framework (UNUserNotificationCenter).

On macOS, the application must have a valid bundle identifier and the user must grant notification permission.
Use REQUESTPERMISSION to prompt the user, and PERMISSION to check the current status.

### See Also

[IupTimer](iup_timer.md)
