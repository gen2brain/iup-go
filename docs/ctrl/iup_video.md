## IupVideo

Creates a canvas that plays a video file or stream, with its audio. Also plays audio-only sources.
It inherits from [IupCanvas](../elem/iup_canvas.md). The element owns the ACTION callback and draws the video itself, scaled to fit while keeping the aspect ratio.

In WebAssembly, when the browser blocks autoplay, PLAY takes effect at the first click or key press.

### Initialization and Usage

The **IupMediaOpen** function must be called after **IupOpen**.
The "iupmedia.h" file must also be included in the source code.
The program must be linked to the media library (iupmedia).

### Creation

    Ihandle* IupVideo(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

Inherits all attributes and callbacks of the [IupCanvas](../elem/iup_canvas.md), but redefines a few attributes.

**FILE** (non-inheritable): Path of a local file to play. Setting it stops the current playback, clears URL and loads the new source asynchronously, stopped at the start. READY_CB is called when it is ready.
NULL unloads the source.

**URL** (non-inheritable): URI of the source to play, such as an "http", "https" or "file" URL. Same behavior as FILE, and setting it clears FILE.

**BACKEND** (non-inheritable): Playback engine. Can be "AUTO", "NATIVE", "MPV", "VLC" or "GSTREAMER". Default: "AUTO".
Ignored while a source is open.
"NATIVE" is the system player: Media Foundation in Windows, AVFoundation in macOS and iOS, MediaPlayer in Android and the HTML video element in WebAssembly.
In Windows, "NATIVE" requires Windows 8 or newer.
"MPV" (libmpv 2), "VLC" (libVLC 3 or 4) and "GSTREAMER" (GStreamer 1.x) are available in Windows, macOS, Linux, FreeBSD and Haiku, loaded at runtime when installed. "AUTO" tries them in that order, after "NATIVE" in Windows and macOS.
In Linux, FreeBSD and Haiku, libVLC 4 is used when both versions are installed.
In Windows, libmpv-2.dll is loaded from the application folder or PATH, libvlc.dll from the application folder, PATH or the VLC installation, and GStreamer from PATH or the GSTREAMER_1_0_ROOT_MSVC_X86_64, GSTREAMER_1_0_ROOT_MSVC_ARM64, GSTREAMER_1_0_ROOT_MINGW_X86_64 or GSTREAMER_1_0_ROOT_X86_64 installation.
In macOS, libmpv and GStreamer are loaded from the library search path, Homebrew (/opt/homebrew/lib or /usr/local/lib) or GStreamer.framework, and libVLC from the library search path or VLC.app in /Applications or ~/Applications.

**BACKENDUSED** (read-only): Engine playing the current source, or NULL when none is open.

**AUDIOURL** (non-inheritable): URI of a separate audio stream played with the source in place of its own audio, such as an "http", "https" or "file" URL. Used when FILE or URL loads the source, so set it before them. NULL plays the audio of the source.
Supported by "VLC", "MPV" and "NATIVE" in macOS and iOS. "AUTO" skips the other engines; when no engine selected by BACKEND supports it, STATE becomes "ERROR".

**MPVCONFIG** (non-inheritable): Loads the user mpv configuration (mpv.conf) when "MPV" loads the source. Can be "YES" or "NO". Default: "NO".

**MPVOPTIONS** (non-inheritable): Additional mpv options applied when "MPV" loads the source, after the configuration loaded by MPVCONFIG. Options are separated by spaces, each "name=value", "name" (same as "name=yes") or "no-name" (same as "name=no"), with an optional leading "--". Double quotes keep spaces in a value. Unknown options are ignored.
The options IupVideo needs to render and control playback, such as "vo", "pause", "idle" and "keep-open", cannot be changed.
IupVideo sets "sws-fast=yes" before MPVCONFIG and MPVOPTIONS apply, so both can change it.

**VLCCONFIG** (non-inheritable): Loads the user VLC configuration (vlcrc) when "VLC" loads the source. Can be "YES" or "NO". Default: "NO".
VLC writes the configuration file again when the source is closed; options from VLCOPTIONS are not saved to it.

**VLCOPTIONS** (non-inheritable): Additional VLC options applied when "VLC" loads the source, after the configuration loaded by VLCCONFIG. Options are separated by spaces, each "name=value", "name" or "no-name", with an optional leading "--". Double quotes keep spaces in a value. When VLC rejects an option, the source fails to load and STATE becomes "ERROR".
The options IupVideo needs to render and control playback, such as "no-sub-autodetect-file", cannot be changed.
IupVideo sets "swscale-mode=0" before VLCOPTIONS apply, so VLCOPTIONS can change it.

**AVAILABLE** (read-only): Returns "YES" if an engine selected by BACKEND can be loaded, "NO" otherwise. While AUDIOURL is set, only engines that support it count. Which formats play depends on the engine and its installed codecs.

**PLAY** (write-only): Starts playback, or resumes it after PAUSE. While the source is loading, playback starts when it is ready. After the end, playback restarts from the beginning.
Ignored when no source is set and in the "ERROR" state.

**PAUSE** (write-only): Pauses playback keeping the current position. Ignored when no source is set.

**STOP** (write-only): Pauses playback and rewinds to the start.

**STATE** (read-only): Returns "EMPTY" (no source), "LOADING", "STOPPED", "PLAYING", "PAUSED", "BUFFERING", "ENDED" or "ERROR".
While the element is not mapped, a set source is "STOPPED". Unmapping closes the engine; mapping again reloads the source stopped.
While "PLAYING" or "BUFFERING" a source with video, the screen saver and display sleep are inhibited.
In Linux and FreeBSD this requires the org.freedesktop.ScreenSaver D-Bus service or an X11 session.

**POSITION** (non-inheritable): Playback position in seconds, as a floating point number. Setting it seeks asynchronously, as selected by SEEKMODE. Ignored when the source is not seekable.

**SEEKMODE** (non-inheritable): How setting POSITION seeks. Can be "EXACT" (to the given position) or "FAST" (to a nearby keyframe). Default: "EXACT".
With "VLC" 3, with AUDIOURL in "MPV" and "VLC", and in WebAssembly in browsers without HTMLMediaElement.fastSeek, "FAST" seeks like "EXACT".

**DURATION** (read-only): Length of the source in seconds, as a floating point number, or "0" when unknown.

**SEEKABLE** (read-only): Returns "YES" if the source supports seeking, "NO" otherwise.

**LOOP** (non-inheritable): Restarts seekable sources from the beginning when the end is reached. Can be "YES" or "NO". Default: "NO".
While "YES" and the source is seekable, PLAYEND_CB is not called.

**VOLUME** (non-inheritable): Playback volume from 0 (silent) to 100. Default: "100".

**MUTE** (non-inheritable): Silences the audio keeping VOLUME. Can be "YES" or "NO". Default: "NO".

**AUDIODEVICE** (non-inheritable): Index of the audio output device, from 0. Index 0 is the system default output. Default: "0".
Changing it during playback moves the audio to the new device.
Not supported in iOS. In Android it requires Android 9.

**AUDIODEVICECOUNT** (read-only): Number of audio output devices of the engine selected by BACKEND, counting the default at index 0. Each query lists the devices again.

**AUDIODEVICENAMEid** (read-only): Name of the audio output device at index "id", or NULL. AUDIODEVICENAME0 is "Default".
In WebAssembly the devices are listed only after media access has been granted and when the browser can select the output device.

**BRIGHTNESS**, **CONTRAST**, **SATURATION**, **GAMMA** (non-inheritable): Picture adjustments from -100 to 100. "0" leaves the picture unchanged. Default: "0".
Not supported with BACKEND "NATIVE" in Windows and Android.

**HASVIDEO**, **HASAUDIO** (read-only): Return "YES" if the source has a playable video or audio stream, "NO" otherwise.
In WebAssembly, HASAUDIO can return "NO" until playback starts.

**VIDEOSIZE** (read-only): Display size of the video in pixels, as "widthxheight", after rotation and with the pixel aspect ratio applied. Returns "0x0" when unknown.

**BGCOLOR**: Color of the area around the picture when the canvas aspect ratio differs from the video. Default: "0 0 0".

**BORDER**: Default: "NO".

**CANFOCUS**: Default: "NO".

**SUBTITLE** (non-inheritable): Selected subtitle track. Can be "AUTO", "OFF", "EXTERNAL" or a one-based embedded track index. Default: "AUTO".
"AUTO" shows the embedded track flagged as default or forced in the source, and nothing when no track is flagged. "EXTERNAL" shows the subtitles loaded with SUBTITLEFILE or SUBTITLEDATA, and requires them. "OFF" hides subtitles and keeps them loaded.
In GStreamer, "AUTO" follows only the default flag. In Windows, Android and WebAssembly, "AUTO" shows no embedded track.
Can be changed while playing or paused without reloading the source. Changing FILE or URL sets a track index or "EXTERNAL" back to "AUTO"; "OFF" is kept.
Embedded formats depend on the engine. Bitmap subtitles are shown only when the engine draws them.
In Windows, embedded subtitles require Windows 10 or newer.

**SUBTITLECOUNT** (read-only): Number of embedded subtitle tracks of the current source. External subtitles are not included. Can change after READY_CB.
In macOS and iOS, forced-only tracks are included.

**SUBTITLENAMEid**, **SUBTITLELANGUAGEid** (read-only): Name and language of the embedded subtitle track id, starting at 1. Can become available after READY_CB.

**SUBTITLEFILE** (non-inheritable): Path of a local UTF-8 SRT or WebVTT file. Loads it as the external subtitles and sets SUBTITLE to "EXTERNAL", keeping the position and the playback state. NULL unloads the external subtitles.
In Android, can also be a "content:" URI.

**SUBTITLEDATA** (write-only): UTF-8 SRT or WebVTT contents. Same as SUBTITLEFILE, without a file. The contents are copied.
The limit is 16 MiB. Overlapping cues are shown together. Markup is reduced to plain text; WebVTT positioning, regions and styles are ignored.
Changing FILE or URL unloads the external subtitles. Unmapping keeps them.
When loading fails, the previous subtitles are kept, SUBTITLEERROR is set and ERROR_CB is called. STATE does not change.

**SUBTITLEDELAY** (non-inheritable): Subtitle offset in seconds, from -86400 to 86400. Positive values show subtitles later. Default: "0".
In macOS, iOS and Android, applies only to external subtitles.

**SUBTITLEFONTSIZE** (non-inheritable): Font size in points of the subtitles drawn by the element for a picture 540 units high, from 6 to 96. The drawn size follows the height of the displayed picture. Default: "18".

**SUBTITLECOLOR** (non-inheritable): Text color of the subtitles drawn by the element. Default: "255 255 255".

**SUBTITLETEXT** (read-only): Current subtitle text, or NULL between cues. NULL when the engine draws the embedded subtitles itself.

**SUBTITLEERROR** (read-only): Last subtitle loading or selection error, or NULL. Cleared by a successful load or selection.

With "NATIVE" in Win32, Cocoa, iOS, Android and WebAssembly the engine draws into a native view inside the canvas.
With "NATIVE" in other Windows drivers it draws into a child window placed over the canvas, above sibling controls.
Otherwise the decoded frames are drawn on the canvas.

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
Also called for subtitle errors, which do not change STATE and do not call STATECHANGED_CB.

    int function(Ihandle *ih, const char *message);

**ih**: identifier of the element that activated the event.\
**message**: description of the failure.

**Returns**: IUP_CLOSE will be processed.

All callbacks are called from the main loop.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupCanvas](../elem/iup_canvas.md), [IupAudio](iup_audio.md), [IupCamera](iup_camera.md)
