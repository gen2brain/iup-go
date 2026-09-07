## IupCamera

Creates a canvas that shows the live picture of a camera.
It inherits from [IupCanvas](../elem/iup_canvas.md). The element owns the ACTION callback and draws the frames itself, scaled to fit while keeping the aspect ratio.

Not supported in Haiku.

### Initialization and Usage

The **IupMediaOpen** function must be called after **IupOpen**.
The "iupmedia.h" file must also be included in the source code.
The program must be linked to the media library (iupmedia).

### Creation

    Ihandle* IupCamera(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

Inherits all attributes and callbacks of the [IupCanvas](../elem/iup_canvas.md), but redefines a few attributes.

**DEVICE** (non-inheritable): Index of the camera, from 0. Default: "0".
Changing it while running restarts the capture on the new camera.

**DEVICECOUNT** (read-only): Number of cameras found.

**DEVICENAMEid** (read-only): Name of the camera at index "id", or NULL.
In WebAssembly it is "Camera N" until camera access has been granted.

**RUN** (non-inheritable): Starts the capture when "YES" and stops it when "NO". Default: "NO".
Starting asks the user for permission if the platform requires it, see PERMISSION_CB.
The capture is stopped when the element is unmapped or destroyed.

**RESOLUTION** (non-inheritable): Requested frame size as "widthxheight". Default: "640x480".
The closest size the camera offers is used. While running, returns the actual frame size.

**FPS** (non-inheritable): Requested frame rate. Default: "30".
While running, returns the actual frame rate when the camera reports it.

**MIRROR** (non-inheritable): Flips the picture horizontally. Can be "YES" or "NO". Default: "NO".
Affects the display, FRAME_CB and SNAPSHOT.

**SNAPSHOT** (write-only): Saves the last frame to the given file name. The format is taken from the file extension, see [IupImageSave](../func/iup_imagesave.md).
Ignored when no frame was received yet.

**BGCOLOR**: Color of the area around the picture when the canvas aspect ratio differs from the frame. Default: the global attribute DLGBGCOLOR.

**AVAILABLE** (read-only): Returns "YES" if the platform has camera support, "NO" otherwise.

**PERMISSION** (read-only): Returns "GRANTED", "DENIED", "PROMPT" (not asked yet) or "UNAVAILABLE".

### Callbacks

Inherits all callbacks of the [IupCanvas](../elem/iup_canvas.md), except ACTION, which is used internally.

**FRAME_CB**: Called for every captured frame, before it is drawn.

    int function(Ihandle *ih, int width, int height, unsigned char* data);

**ih**: identifier of the element that activated the event.\
**width**, **height**: frame size in pixels.\
**data**: packed RGB pixels, three bytes per pixel, rows from top to bottom without padding. Valid only during the callback.

**Returns**: IUP_IGNORE skips drawing the frame. IUP_CLOSE will be processed.

**PERMISSION_CB**: Called when the user answers the permission request.

    int function(Ihandle *ih, int granted);

**ih**: identifier of the element that activated the event.\
**granted**: 1 when access was granted, 0 when it was denied.

**Returns**: IUP_CLOSE will be processed.

**ERROR_CB**: Called when the camera cannot be opened or stops delivering frames.

    int function(Ihandle *ih, const char *message);

**ih**: identifier of the element that activated the event.\
**message**: description of the failure.

**Returns**: IUP_CLOSE will be processed.

All three callbacks are called from the main loop.

### Notes

The application must declare the platform permission, IUP cannot do it:

- Android: `CAMERA` in the application manifest. The runtime prompt is shown by IUP.
- iOS and macOS: `NSCameraUsageDescription` in Info.plist. Without it the request fails silently and PERMISSION stays "PROMPT".
- WebAssembly: the page must be a secure context. The browser shows its own prompt.

On Linux and BSD the cameras are the Video4Linux2 capture devices; a camera that offers only compressed formats other than MJPEG cannot be used.

On iOS and Android the frames are rotated to the interface orientation in effect when RUN is set.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupCanvas](../elem/iup_canvas.md), [IupAudio](iup_audio.md), [IupMicrophone](iup_microphone.md)
