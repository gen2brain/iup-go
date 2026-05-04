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

**ICON** (non-inheritable): Image name for the small notification icon shown next to the title.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
See also [IupImage](iup_image.md).
On iOS, ICON is honored only with STYLE=TOAST.

**IMAGE** [Android / iOS Only] (non-inheritable): Image name shown as the expanded content/preview image.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
On Android, TITLE and BODY are reused as the expanded title and summary.

**SUBTITLE** [macOS / iOS Only] (non-inheritable): The notification subtitle, displayed below the title.

#### Action Buttons

**ACTION1**, **ACTION2**, **ACTION3**, **ACTION4** (non-inheritable): Labels for up to four action buttons displayed in the notification.
Not supported in Windows (Win32), but supported in WinUI.

#### Behavior

**TIMEOUT** (non-inheritable): Auto-dismiss timeout in milliseconds.
Default: "-1" (use the system default).
Supported on Linux/Unix, Android, and on iOS with STYLE=TOAST. Not supported in Windows (Win32), WinUI, or macOS.

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

**PERMISSION** [macOS / iOS Only] (read-only) (non-inheritable): Returns the current notification permission status.
Can be "GRANTED", "DENIED", or "NOTDETERMINED".

**REQUESTPERMISSION** [macOS / iOS Only] (write-only) (non-inheritable): Set to "YES" to request notification permission from the user.
Apps should request it once at startup; until granted, SHOW fires ERROR_CB.

**THREADID** [macOS / iOS Only] (non-inheritable): Notification thread identifier used to group related notifications together.

**STYLE** [Android / iOS Only] (non-inheritable): Selects the delivery channel. Values: "NOTIFICATION" (default) or "TOAST" (transient in-app overlay).
TOAST does not require notification permission. TITLE, BODY, ICON, and TIMEOUT are honored; ACTION1-4, SUBTITLE, and other notification-only attributes are ignored.

**PROGRESS** [Android Only] (non-inheritable): Adds a progress bar.
Integer `0` to `100` shows a determinate bar; `"INDETERMINATE"` shows a continuous animation. Re-issuing SHOW with the same Ihandle updates the bar in place.

**CHANNELID** [Android Only] (non-inheritable): Notification channel id, overriding the default channel chosen from URGENCY.
If the channel does not yet exist it is created with IMPORTANCE; existing channels are reused unchanged.

**IMPORTANCE** [Android Only] (non-inheritable): Channel importance used when creating a new CHANNELID.
Values: "NONE", "MIN", "LOW", "DEFAULT", "HIGH", "MAX". Default: "DEFAULT".

### Callbacks

**NOTIFY_CB**: Called when the user interacts with the notification.

    int function(Ihandle *ih, int action_id);

**ih**: identifier of the element that activated the event.\
**action_id**: 0 for the default action (clicking the notification body), 1-4 for ACTION1 through ACTION4 buttons.

**Returns**: IUP_CLOSE will be processed.

**CLOSE_CB**: Called when the notification is closed.

    int function(Ihandle *ih, int reason);

**ih**: identifier of the element that activated the event.\
**reason**: 0 for unknown, 1 for timeout/auto-dismiss, 2 for user-dismissed, 3 for programmatic close (CLOSE attribute).

**Returns**: IUP_CLOSE will be processed.

**ERROR_CB**: Called when an error occurs during notification initialization or display.

    int function(Ihandle *ih, char *error_msg);

**ih**: identifier of the element that activated the event.\
**error_msg**: description of the error.

### Notes

IupNotify does not create a visible IUP element. It is a non-interactive control that interfaces with the operating system's native notification service.

All content attributes should be set before calling SHOW.
The notification is displayed asynchronously and callbacks are dispatched through the IUP event loop.

On Linux/Unix, notifications are sent via D-Bus (org.freedesktop.Notifications). The running notification daemon decides which features are supported (action buttons, persistence, etc.).

In Windows (Win32) uses Shell Notification balloon, in WinUI uses AppNotificationManager, in macOS and iOS uses UNUserNotificationCenter, and in Android uses NotificationManager.

On macOS and iOS, the application must have a valid bundle identifier and the user must grant notification permission. Use REQUESTPERMISSION to prompt and PERMISSION to check.

On Android, apps that use STYLE="NOTIFICATION" must declare `android.permission.POST_NOTIFICATIONS` in their own manifest (API 33+). STYLE="TOAST" needs no permission.

### See Also

[IupTimer](iup_timer.md)
