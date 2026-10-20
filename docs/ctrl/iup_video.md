## IupVideo

Creates a canvas that plays a video file or stream, with its audio. Also plays audio-only sources.
It inherits from [IupCanvas](../elem/iup_canvas.md). The element owns the ACTION callback and draws the video itself, scaled to fit while keeping the aspect ratio.

In WebAssembly, when the browser blocks autoplay, PLAY takes effect at the first click or key press.

### Initialization and Usage

The **IupMediaOpen** function must be called after **IupOpen**.
The "iupmedia.h" file must also be included in the source code.
The program must be linked to the media library (iupmedia). In Go it needs the `media` build tag.

### Creation

    Ihandle* IupVideo(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

Inherits all attributes and callbacks of the [IupCanvas](../elem/iup_canvas.md), but redefines a few attributes.

**FILE** (non-inheritable): Path of a local file to play. Setting it stops the current playback, clears URL and loads the new source asynchronously, stopped at the start. READY_CB is called when it is ready.
NULL unloads the source.

**URL** (non-inheritable): URI of the source to play, such as an "http", "https" or "file" URL. Same behavior as FILE, and setting it clears FILE.

**BACKEND** (non-inheritable): Playback engine. Can be "AUTO", "NATIVE", "VLC", "GSTREAMER" or "MPV". Default: "AUTO".
Ignored while a source is open.
"NATIVE" is the system player: Media Foundation in Windows, AVFoundation in macOS and iOS, MediaPlayer in Android and the HTML video element in WebAssembly.
In Windows, "NATIVE" requires Windows 8 or newer.
"VLC" (libVLC 3), "GSTREAMER" (GStreamer 1.x) and "MPV" (libmpv 2) are available in Linux, FreeBSD and Haiku, loaded at runtime when installed. "AUTO" tries them in that order.

**BACKENDUSED** (read-only): Engine playing the current source, or NULL when none is open.

**AVAILABLE** (read-only): Returns "YES" if an engine selected by BACKEND can be loaded, "NO" otherwise. Which formats play depends on the engine and its installed codecs.

**PLAY** (write-only): Starts playback, or resumes it after PAUSE. While the source is loading, playback starts when it is ready. After the end, playback restarts from the beginning.
Ignored when no source is set and in the "ERROR" state.

**PAUSE** (write-only): Pauses playback keeping the current position. Ignored when no source is set.

**STOP** (write-only): Pauses playback and rewinds to the start.

**STATE** (read-only): Returns "EMPTY" (no source), "LOADING", "STOPPED", "PLAYING", "PAUSED", "BUFFERING", "ENDED" or "ERROR".
While the element is not mapped, a set source is "STOPPED". Unmapping closes the engine; mapping again reloads the source stopped.

**POSITION** (non-inheritable): Playback position in seconds, as a floating point number. Setting it seeks asynchronously. Ignored when the source is not seekable.

**DURATION** (read-only): Length of the source in seconds, as a floating point number, or "0" when unknown.

**SEEKABLE** (read-only): Returns "YES" if the source supports seeking, "NO" otherwise.

**LOOP** (non-inheritable): Restarts seekable sources from the beginning when the end is reached. Can be "YES" or "NO". Default: "NO".
While "YES" and the source is seekable, PLAYEND_CB is not called.

**VOLUME** (non-inheritable): Playback volume from 0 (silent) to 100. Default: "100".

**MUTE** (non-inheritable): Silences the audio keeping VOLUME. Can be "YES" or "NO". Default: "NO".

**HASVIDEO**, **HASAUDIO** (read-only): Return "YES" if the source has a playable video or audio stream, "NO" otherwise.
In WebAssembly, HASAUDIO can return "NO" until playback starts.

**VIDEOSIZE** (read-only): Display size of the video in pixels, as "widthxheight", after rotation and with the pixel aspect ratio applied. Returns "0x0" when unknown.

**BGCOLOR**: Color of the area around the picture when the canvas aspect ratio differs from the video. Default: "0 0 0".

**BORDER**: Default: "NO".

**CANFOCUS**: Default: "NO".

In Win32, Cocoa, iOS, Android and WebAssembly the engine draws into a native view inside the canvas.
In other Windows drivers it draws into a child window placed over the canvas, above sibling controls.
In other drivers the decoded frames are drawn on the canvas.

### Callbacks

Inherits all callbacks of the [IupCanvas](../elem/iup_canvas.md), except ACTION, which is used internally.

**READY_CB**: Called when the source is loaded and ready to play.

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

**Returns**: IUP_CLOSE will be processed.

**STATECHANGED_CB**: Called when STATE changes.

    int function(Ihandle *ih, const char *state);

**ih**: identifier of the element that activated the event.\
**state**: the new STATE value.

**Returns**: IUP_CLOSE will be processed.

**PLAYEND_CB**: Called when playback reaches the end of the source. Not called after STOP, nor while LOOP is "YES" and the source is seekable.

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

**Returns**: IUP_CLOSE will be processed.

**ERROR_CB**: Called when the source cannot be loaded or playback fails. STATECHANGED_CB with "ERROR" is called before it.

    int function(Ihandle *ih, const char *message);

**ih**: identifier of the element that activated the event.\
**message**: description of the failure.

**Returns**: IUP_CLOSE will be processed.

All callbacks are called from the main loop.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupCanvas](../elem/iup_canvas.md), [IupAudio](iup_audio.md), [IupCamera](iup_camera.md)
