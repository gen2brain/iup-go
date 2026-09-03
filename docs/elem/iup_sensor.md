## IupSensor

Creates a motion sensor source. Reports readings of one device sensor through a callback while active.
Each element should be destroyed using [IupDestroy](../func/iup_destroy.md).

Supported in Win32, WinUI, Android, iOS and WebAssembly.

### Creation

    Ihandle* IupSensor(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**TYPE** (non-inheritable): Sensor to read. Can be "ACCELEROMETER", "GYROSCOPE", "MAGNETOMETER",
"GRAVITY", "LINEARACCELERATION", "ORIENTATION" or "COMPASS". Default: "ACCELEROMETER".
Can be changed only while ACTIVE=NO.
GRAVITY and LINEARACCELERATION are not supported in Win32.
MAGNETOMETER is not supported in WebAssembly.

**ACTIVE** (non-inheritable): Starts readings when "YES" and stops them when "NO". Default: "NO".
Readings are stopped when the element is destroyed.

**INTERVAL** (non-inheritable): Minimum time between two readings in milliseconds. Default: "200".

**X**, **Y**, **Z** (read-only): Last reading, see TYPE for the meaning and units of each axis.

**VALUE** (read-only): Last reading as "x y z".

**TIMESTAMP** (read-only): Time of the last reading in milliseconds since 1970-01-01 UTC.

**AVAILABLE** (read-only): Returns "YES" if the device has the sensor selected by TYPE, "NO" otherwise.

**PERMISSION** (read-only): Returns "GRANTED", "DENIED", "PROMPT" (not asked yet) or "UNAVAILABLE".
Only WebAssembly on iOS asks for permission.

### Readings

Axes follow the device: X to the right, Y to the top and Z out of the screen, with the device held in its
natural portrait orientation.

| TYPE               | X, Y, Z                                                                     | Unit  |
|--------------------|-----------------------------------------------------------------------------|-------|
| ACCELEROMETER      | Acceleration including gravity. At rest reads about 9.81 on the axis pointing up. | m/s²  |
| GYROSCOPE          | Rate of rotation around each axis, positive counter-clockwise.              | rad/s |
| MAGNETOMETER       | Ambient magnetic field.                                                     | µT    |
| GRAVITY            | Gravity component of the acceleration, length about 9.81.                   | m/s²  |
| LINEARACCELERATION | Acceleration without gravity. At rest reads 0.                              | m/s²  |
| ORIENTATION        | Azimuth (rotation around Z, 0 at magnetic north), pitch (around X) and roll (around Y). | degrees |
| COMPASS            | X heading from magnetic north, Y heading from true north or -1 when unknown, Z accuracy or -1 when unknown. | degrees |

### Callbacks

**SENSOR_CB**: Called on every reading, after the read-only attributes are set.

    int function(Ihandle *ih, double x, double y, double z);

**ih**: identifier of the element that activated the event.\
**x**, **y**, **z**: the reading.

**Returns**: IUP_CLOSE will be processed.

**PERMISSION_CB**: Called when the user answers the permission request.

    int function(Ihandle *ih, int granted);

**ih**: identifier of the element that activated the event.\
**granted**: 1 when access was granted, 0 when it was denied.

**Returns**: IUP_CLOSE will be processed.

**ERROR_CB**: Called when the sensor cannot start or stops delivering readings.

    int function(Ihandle *ih, const char *message);

**ih**: identifier of the element that activated the event.\
**message**: description of the failure.

**Returns**: IUP_CLOSE will be processed.

All three callbacks are called from the main loop on every driver.

### Notes

WebAssembly needs a secure context (HTTPS or localhost). On iOS the browser asks for permission the first
time ACTIVE is set to YES, and the request is only accepted from a user event such as a button ACTION.

Win32 uses the Windows Sensor API, deprecated since Windows 8 but present through Windows 11.

### Examples

[sensor.go](../../examples/sensor/sensor.go)

### See Also

[IupLocation](iup_location.md), [IupTimer](iup_timer.md)
