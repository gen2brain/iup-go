## IupMicrophone

Creates an audio capture source. Delivers the captured samples to a callback and can record them to a WAV file.
It is not a visual element. Each element should be destroyed using [IupDestroy](../func/iup_destroy.md).

Samples are 16-bit signed integers, interleaved by channel.

Not supported in Haiku.

### Initialization and Usage

The **IupMediaOpen** function must be called after **IupOpen**.
The "iupmedia.h" file must also be included in the source code.
The program must be linked to the media library (iupmedia).

### Creation

    Ihandle* IupMicrophone(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**DEVICE** (non-inheritable): Index of the microphone, from 0. Index 0 is the system default input. Default: "0".
Changing it while running restarts the capture on the new microphone.

**DEVICECOUNT** (read-only): Number of microphones found.

**DEVICENAMEid** (read-only): Name of the microphone at index "id", or NULL.
In WebAssembly it is "Microphone N" until microphone access has been granted.

**RUN** (non-inheritable): Starts the capture when "YES" and stops it when "NO". Default: "NO".
Starting asks the user for permission if the platform requires it, see PERMISSION_CB.
The capture is stopped when the element is destroyed.

**CHANNELS** (non-inheritable): Requested channel count. Default: "1".
While running, returns the actual channel count.

**SAMPLERATE** (non-inheritable): Requested sample rate in Hz. Default: "44100".
While running, returns the actual sample rate.

**FILE** (non-inheritable): Path of a WAV file that receives the captured samples. Recording starts when both FILE is set and the capture is running, and the file is closed when FILE is set to NULL or the capture stops.
If the file cannot be created or written, ERROR_CB is called and the capture continues without it.

**LEVEL** (read-only): Peak amplitude of the last delivered block, from 0 to 100. Returns "0" when not running.

**AVAILABLE** (read-only): Returns "YES" if the platform has audio capture support, "NO" otherwise.

**PERMISSION** (read-only): Returns "GRANTED", "DENIED", "PROMPT" (not asked yet) or "UNAVAILABLE".

### Callbacks

**SAMPLES_CB**: Called for every captured block of samples, after it was written to FILE.

    int function(Ihandle *ih, int frames, int channels, short* samples);

**ih**: identifier of the element that activated the event.\
**frames**: number of frames in the block. A frame holds one sample per channel.\
**channels**: channel count.\
**samples**: interleaved 16-bit samples, frames x channels values. Valid only during the callback.

**Returns**: IUP_CLOSE will be processed.

**PERMISSION_CB**: Called when the user answers the permission request.

    int function(Ihandle *ih, int granted);

**ih**: identifier of the element that activated the event.\
**granted**: 1 when access was granted, 0 when it was denied.

**Returns**: IUP_CLOSE will be processed.

**ERROR_CB**: Called when the microphone cannot be opened or stops delivering samples, which stops the capture, and when FILE cannot be written.

    int function(Ihandle *ih, const char *message);

**ih**: identifier of the element that activated the event.\
**message**: description of the failure.

**Returns**: IUP_CLOSE will be processed.

All three callbacks are called from the main loop.

### Notes

The application must declare the platform permission, IUP cannot do it:

- Android: `RECORD_AUDIO` in the application manifest. The runtime prompt is shown by IUP.
- iOS and macOS: `NSMicrophoneUsageDescription` in Info.plist. Without it the request fails silently and PERMISSION stays "PROMPT".
- WebAssembly: the page must be a secure context. The browser shows its own prompt.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupAudio](iup_audio.md), [IupCamera](iup_camera.md)
