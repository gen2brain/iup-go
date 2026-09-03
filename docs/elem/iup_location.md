## IupLocation

Creates a geolocation source. Reports the device position through a callback while active.
Each element should be destroyed using [IupDestroy](../func/iup_destroy.md).

Not supported in Haiku.

### Creation

    Ihandle* IupLocation(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**ACTIVE** (non-inheritable): Starts position updates when "YES" and stops them when "NO". Default: "NO".
Starting asks the user for permission if the platform requires it, see PERMISSION_CB.
Updates are stopped when the element is destroyed.

**ACCURACY** (non-inheritable): Requested precision. Can be "COARSE" or "FINE". Default: "COARSE".
"FINE" uses the satellite receiver where one exists.

**INTERVAL** (non-inheritable): Minimum time between two updates in milliseconds. Default: "1000".

**DISTANCE** (non-inheritable): Minimum movement between two updates in meters. Default: "0".

**LATITUDE**, **LONGITUDE** (read-only): Last reported position in decimal degrees.

**ALTITUDE** (read-only): Last reported altitude in meters above sea level, or NULL when the fix has none.

**HORIZONTALACCURACY** (read-only): Radius of the last fix in meters, 68% confidence.

**SPEED** (read-only): Ground speed of the last fix in meters per second, or NULL when the fix has none.

**HEADING** (read-only): Direction of travel of the last fix in degrees clockwise from true north, or NULL when the fix has none.

**TIMESTAMP** (read-only): Time of the last fix in milliseconds since 1970-01-01 UTC.

**AVAILABLE** (read-only): Returns "YES" if the platform has a location service, "NO" otherwise.
On Linux and BSD it needs the GeoClue2 service on the system bus.

**PERMISSION** (read-only): Returns "GRANTED", "DENIED", "PROMPT" (not asked yet) or "UNAVAILABLE".

### Callbacks

**LOCATION_CB**: Called on every position update, after the read-only attributes are set.

    int function(Ihandle *ih, double latitude, double longitude);

**ih**: identifier of the element that activated the event.\
**latitude**, **longitude**: position in decimal degrees.

**Returns**: IUP_CLOSE will be processed.

**PERMISSION_CB**: Called when the user answers the permission request.

    int function(Ihandle *ih, int granted);

**ih**: identifier of the element that activated the event.\
**granted**: 1 when access was granted, 0 when it was denied.

**Returns**: IUP_CLOSE will be processed.

**ERROR_CB**: Called when the service cannot start or stops delivering updates.

    int function(Ihandle *ih, const char *message);

**ih**: identifier of the element that activated the event.\
**message**: description of the failure.

**Returns**: IUP_CLOSE will be processed.

All three callbacks are called from the main loop on every driver.

### Notes

The application must declare the platform permission, IUP cannot do it:

- Android: `ACCESS_COARSE_LOCATION` and, for ACCURACY=FINE, `ACCESS_FINE_LOCATION` in the application manifest. The runtime prompt is shown by IUP.
- iOS and macOS: `NSLocationWhenInUseUsageDescription` in Info.plist. Without it the request fails silently and PERMISSION stays "PROMPT".
- WebAssembly: the page must be served from a secure context (HTTPS or localhost).
- Linux and BSD: GeoClue2 identifies the application by its desktop file, the [APPID](../attrib/iup_globals.md#appid) global is used as the desktop id and an authorization agent must be running.

Win32 uses the Windows Location API, deprecated since Windows 8 but present through Windows 11.

### Examples

[location.go](../../examples/location/location.go)

### See Also

[IupNotify](iup_notify.md), [IupTimer](iup_timer.md)
