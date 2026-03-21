## IupGLCanvas

Creates an OpenGL canvas (drawing area for OpenGL).
It inherits from [IupCanvas](../elem/iup_canvas.md).

### Initialization and Usage

The **IupGLCanvasOpen** function must be called after a **IupOpen**, so that the control can be used.
The "iupgl.h" file must also be included in the source code.
The program must be linked to the controls library (iupgl), and with the OpenGL library.

To link with the OpenGL libraries in Windows, add: opengl32.lib.
In UNIX add before the X-Windows libraries: -LGL.

### Creation

    Ihandle* IupGLCanvas(const char* action);

**action**: Name of the action generated when the canvas needs to be redrawn. It can be NULL.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

The **IupGLCanvas** element handles all attributes defined for a conventional canvas, see [IupCanvas](../elem/iup_canvas.md).

Apart from these attributes, **IupGLCanvas** handle specific attributes used to define the kind of buffer to be instanced.
Such attributes are all **creation-only** attributes and must be set before the element is mapped on the native system.
After the mapping, specifying these special attributes has no effect.

> 
>
> ------------------------------------------------------------------------

**ACCUM_RED_SIZE**, **ACCUM_GREEN_SIZE**, **ACCUM_BLUE_SIZE** and **ACCUM_ALPHA_SIZE**: Indicate the number of bits for representing the color components in the accumulation buffer.
Value 0 means the accumulation buffer is not necessary. Default is 0.

**ALPHA_SIZE**: Indicates the number of bits for representing each colors alpha component (valid only for RGBA and for hardware that store the alpha component).
Default is "0".

**ARBCONTEXT** (non-inheritable): enable the usage of ARB extension contexts.
If during map the ARB extensions could not be loaded the attribute will be set to NO and the standard context creation will be used.
Default: NO.

**BUFFER**: Indicates if the buffer will be single "SINGLE" or double "DOUBLE". Default is "SINGLE".

**BUFFER_SIZE**: Indicates the number of bits for representing the color indices (valid only for INDEX).
The system default is 8 (256-color palette).

**COLOR**: Indicates the color model to be adopted: "INDEX" or "RGBA". Default is "RGBA".

**COLORMAP** (read-only)**:** Returns "Colormap" in UNIX and "HPALETTE" in Win32, if COLOR=INDEX.

**CONTEXT** (read-only)**:** Returns "GLXContext" in UNIX and "HGLRC" in Win32.

**CONTEXTFLAGS** (non-inheritable): Context flags. Can be DEBUG, FORWARDCOMPATIBLE or DEBUGFORWARDCOMPATIBLE.
Used only when ARBCONTEXT=Yes.

**CONTEXTPROFILE** (non-inheritable): Context profile mask. Can be CORE, COMPATIBILITY or CORECOMPATIBILITY.
Used only when ARBCONTEXT=Yes.

**CONTEXTVERSION** (non-inheritable): Context version number in the format "major.minor".
Used only when ARBCONTEXT=Yes.

**DEPTH_SIZE**: Indicates the number of bits for representing the *z* coordinate in the z-buffer.
Value 0 means the z-buffer is not necessary. 

**ERROR** (read-only): If an error is found during **IupMap** and **IupGLMakeCurrent**, returns a string containing a description of the error in English.
See notes below.

**LASTERROR** (read-only) [Windows Only]: If an error is found, returns a string with the system error description.

**RED_SIZE**, **GREEN_SIZE** and **BLUE_SIZE**: Indicate the number of bits for representing each color component (valid only for RGBA).
The system default is usually 8 for each component (True Color support).

**REFRESHCONTEXT** (write-only) [Windows Only]: action attribute to refresh the internal device context when it is not owned by the window class.
The **IupCanvas** of the Win32 driver will always create a window with an owned DC, but GTK in Windows will not.

**STENCIL_SIZE**: Indicates the number of bits in the stencil buffer.
Value 0 means the stencil buffer is not necessary. Default is 0.

**STEREO**: Creates a stereo GL canvas (special glasses are required to visualize it correctly).
Possible values: "YES" or "NO". Default: "NO".
When this flag is set to Yes but the OpenGL driver does not support it, the map will be successful and STEREO will be set to NO and ERROR will not be set.

**SHAREDCONTEXT**: name of another **IupGLCanvas** that will share its display lists and textures.
That canvas must be mapped before this canvas.

**VISUAL** (read-only)**:** Returns "XVisualInfo*" in UNIX and "HDC" in Win32.

### Callbacks

The **IupGLCanvas** element understands all callbacks defined for a conventional canvas, see [IupCanvas](../elem/iup_canvas.md).

Additionally:

[RESIZE_CB](../call/iup_resize_cb.md): By default the resize callback sets:

    glViewport(0,0,width,height);

**SWAPBUFFERS_CB**`:` action generated when **IupGLSwapBuffers** is called.

    int function(Ihandle* ih); 

**ih**: identifier of the element that activated the event.

### Auxiliary Functions

These are auxiliary functions based on the WGL and XGL extensions.
Check the respective documentations for more information.
ERROR attribute will be set to "Failed to set new current context." if the call failed.
It will reset the ERROR to NULL if successful.

    void IupGLMakeCurrent(Ihandle* ih);

Activates the given canvas as the current OpenGL context. All subsequent OpenGL commands are directed to such canvas.
The first call will set the global attributes GL_VERSION, GL_VENDOR and GL_RENDERER.

    int IupGLIsCurrent(Ihandle* ih);

Returns a non-zero value if the given canvas is the current OpenGL context.

    void IupGLSwapBuffers(Ihandle* ih);

Makes the BACK buffer visible. This function is necessary when a double buffer is used.

    void IupGLPalette(Ihandle* ih, int index, float r, float g, float b);

Defines a color in the color palette. This function is necessary when INDEX color is used.

    void IupGLUseFont(Ihandle* ih, int first, int count, int list_base);

Creates a bitmap display list from the current FONT attribute.
See the documentation of the wglUseFontBitmaps and glXUseXFont functions.

    void IupGLWait(int gl)

If gl is non-zero it will call glFinish or glXWaitGL, else will call GdiFlush or glXWaitX.

### Notes

In Windows XP, if the COMPOSITE attribute is enabled, then the hardware acceleration will be disabled.

The **IupGLCanvas** works with the GTK base driver in Windows and in UNIX (X-Windows).

Not available in our SunOS510x86 pre-compiled binaries just because we were not able to compile OpenGL code in our installation.

Possible ERROR strings during **IupMap**:

    "X server has no OpenGL GLX extension." - OpenGL not supported (UNIX Only)
    "No appropriate visual." - Failed to choose a Visual (UNIX Only) 
    "No appropriate pixel format." - Failed to choose a Pixel Format (Windows Only)
    "Could not create a rendering context." - Failed to create the OpenGL context. (Windows and UNIX)

### Examples

[Browse for Example Files](../../examples/)

![](images/iupglcanvas.png)

### See Also

[IupCanvas](../elem/iup_canvas.md)
