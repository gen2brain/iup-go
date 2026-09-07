## IupAudio

Creates an audio player. Plays one sound file through the default output device; several elements play at the same time and are mixed.
It is not a visual element. Each element should be destroyed using [IupDestroy](../func/iup_destroy.md).

Decodes WAV, FLAC, MP3 and Ogg Vorbis files.

Not supported in Haiku and WebAssembly.

### Initialization and Usage

The **IupMediaOpen** function must be called after **IupOpen**.
The "iupmedia.h" file must also be included in the source code.
The program must be linked to the media library (iupmedia).

    int IupMediaOpen(void);

**Returns:** IUP_NOERROR, IUP_OPENED when already open, or IUP_ERROR when IupOpen was not called.

### Creation

    Ihandle* IupAudio(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**FILE** (non-inheritable): Path of the file to play. Setting it stops the current playback and loads the new file, rewound to the start.
If the file cannot be loaded, ERROR_CB is called and FILE returns NULL.
NULL unloads the file.

**PLAY** (write-only): Starts playback, or resumes it after PAUSE. Ignored when no file is loaded.

**PAUSE** (write-only): Pauses playback keeping the current position.

**STOP** (write-only): Stops playback and rewinds to the start.

**STATE** (read-only): Returns "PLAYING", "PAUSED" or "STOPPED".

**VOLUME** (non-inheritable): Playback volume from 0 (silent) to 100. Default: "100".

**PAN** (non-inheritable): Stereo balance from -100 (left) to 100 (right). Default: "0".

**PITCH** (non-inheritable): Playback speed factor, greater than 0. Changes both speed and pitch. Default: "1.0".

**LOOP** (non-inheritable): Restarts from the beginning when the end is reached. Can be "YES" or "NO". Default: "NO".
While "YES", PLAYEND_CB is not called.

**POSITION** (non-inheritable): Playback position in seconds, as a floating point number. Setting it seeks.

**DURATION** (read-only): Length of the loaded file in seconds, as a floating point number, or "0" when unknown.

**CHANNELS**, **SAMPLERATE** (read-only): Channel count and sample rate in Hz of the loaded file.

### Callbacks

**PLAYEND_CB**: Called when playback reaches the end of the file. Not called after STOP, nor while LOOP is "YES".

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

**Returns**: IUP_CLOSE will be processed.

**ERROR_CB**: Called when a file cannot be loaded.

    int function(Ihandle *ih, const char *message);

**ih**: identifier of the element that activated the event.\
**message**: description of the failure.

**Returns**: IUP_CLOSE will be processed.

Both callbacks are called from the main loop.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupTimer](../elem/iup_timer.md)
