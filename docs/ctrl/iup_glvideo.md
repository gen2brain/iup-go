## IupGLVideo

Creates an OpenGL canvas that plays a video file or stream, with its audio.
It inherits from [IupGLCanvas](iup_glcanvas.md) and has the attributes and callbacks of [IupVideo](iup_video.md).
The engine decodes and draws the video on the GPU, scaled to fit while keeping the aspect ratio.

The only engine is "MPV" (libmpv 2), loaded at runtime when installed.
Not supported in Haiku, Android, iOS and WebAssembly: no source is loaded and STATE becomes "ERROR".

### Initialization and Usage

The **IupGLCanvasOpen** and **IupMediaOpen** functions must be called after **IupOpen**.
The "iupmedia.h" file must also be included in the source code.
The program must be linked to the media library (iupmedia) and to the GL library (iupgl).

### Creation

    Ihandle* IupGLVideo(void);

**Returns:** the identifier of the created element, or NULL if an error occurs. Also NULL when **IupGLCanvasOpen** or **IupMediaOpen** was not called.

### Attributes

Inherits all attributes and callbacks of the [IupGLCanvas](iup_glcanvas.md), but redefines a few attributes.
All attributes of the [IupVideo](iup_video.md) are supported, with these differences.

**BACKEND** (non-inheritable): Can be "AUTO" or "MPV". With any other engine no source is loaded and STATE becomes "ERROR".

**AVAILABLE** (read-only): Returns "YES" if libmpv can be loaded, "NO" otherwise.

**MPVOPTIONS** (non-inheritable): IupGLVideo sets only "hwdec=auto" before MPVCONFIG and MPVOPTIONS apply.

**BUFFER**: Default: "DOUBLE".

### Callbacks

Inherits all callbacks of the [IupGLCanvas](iup_glcanvas.md), except ACTION and RESIZE_CB, which are used internally.
All callbacks of the [IupVideo](iup_video.md) are supported.

### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupGLCanvas](iup_glcanvas.md), [IupVideo](iup_video.md)
