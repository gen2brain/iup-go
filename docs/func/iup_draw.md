## IupDraw

Functions to draw in an [IupCanvas](../elem/iup_canvas.md) or an [IupBackgroundBox](../elem/iup_backgroundbox.md), from inside their ACTION callback.
To redraw at any other time, call [IupUpdate](../func/iup_update.md) or [IupRedraw](../func/iup_redraw.md).

In C/C++ the functions are declared in "iupdraw.h".

Internally, IupDraw uses several drawing APIs depending on the platform:
- **Windows (Win32)**: Direct2D or GDI+ via WinDrawLib
- **Windows (WinUI)**: Direct2D
- **GTK 3 / GTK 4**: Cairo
- **macOS**: CoreGraphics
- **iOS**: CoreGraphics
- **Qt**: QPainter
- **FLTK**: FLTK offscreen drawing (fl_draw)
- **EFL**: Efl.Canvas.VG (vector graphics)
- **Motif**: X11 (Xlib + XRender)
- **Android**: android.graphics.Canvas
- **Haiku**: BView attached to an offscreen BBitmap (Interface Kit)
- **WebAssembly**: Canvas 2D

The canvas has a read-only attribute called **DRAWDRIVER** that returns the active backend: D2D, GDI+, CAIRO, COCOA, COCOATOUCH, QT, FLTK, EFL_VG, X11, ANDROID, HAIKU or CANVAS2D.

### Control

All other functions can be called only between calls to DrawBegin and DrawEnd.

    void IupDrawBegin(Ihandle* ih);

Initialize the drawing process.

    void IupDrawEnd(Ihandle* ih);

Terminates the drawing process and actually draw on screen.

    void IupDrawSetClipRect(Ihandle* ih, int x1, int y1, int x2, int y2);

Defines a rectangular clipping region.

    void IupDrawSetClipRoundedRect(Ihandle* ih, int x1, int y1, int x2, int y2, int corner_radius);

Defines a rounded rectangular clipping region.

    void IupDrawResetClip(Ihandle* ih);

Reset the clipping area to none.

    void IupDrawGetClipRect(Ihandle* ih, int *x1, int *y1, int *x2, int *y2);

Returns the previous rectangular clipping region set by IupDrawSetClipRect, if clipping was reset returns 0 in all values.

### Primitives

The primitives color is controlled by the attribute **DRAWCOLOR**.  Default: "0 0 0".
The alpha component is also supported, if not specified 255 (opaque) is assumed.
In Motif alpha requires the X11 RENDER extension (text alpha only in the Xft build).
In FLTK alpha blending requires the Cairo-based build, otherwise colors are pre-blended with the theme background.

Rectangle, Arc, Ellipse and Polygon can be filled or stroked. When stroked, the line style can be continuous, dashed or dotted.
These are controlled by the attribute **DRAWSTYLE**.
It can have values: FILL, STROKE, STROKE_DASH, STROKE_DOT, STROKE_DASH_DOT or STROKE_DASH_DOT_DOT.
Default: STROKE. The FILL value when set before the DrawLine has the same effect as STROKE.

The line width default is 1, but it can be controlled by the **DRAWLINEWIDTH** attribute.

    void IupDrawParentBackground(Ihandle* ih);

Fills the canvas with the native parent background color.

    void IupDrawLine(Ihandle* ih, int x1, int y1, int x2, int y2);

Draws a line including start and end points.

    void IupDrawRectangle(Ihandle* ih, int x1, int y1, int x2, int y2);

Draws a rectangle including start and end points.

    void IupDrawRoundedRectangle(Ihandle* ih, int x1, int y1, int x2, int y2, int corner_radius);

Draws a rounded rectangle with the given corner radius.

    void IupDrawArc(Ihandle* ih, int x1, int y1, int x2, int y2, double a1, double a2);

Draws an arc inside a rectangle between the two angles in degrees.
When filled will draw a pie shape with the vertex at the center of the rectangle.
Angles are counter-clock wise relative to the 3 o'clock position.

    void IupDrawEllipse(Ihandle* ih, int x1, int y1, int x2, int y2);

Draws an ellipse inscribed in the given rectangle.

    void IupDrawPolygon(Ihandle* ih, int* points, int count);

Draws a polygon. Coordinates are stored in the array in the sequence: x1, y1, x2, y2, ...

    void IupDrawPixel(Ihandle* ih, int x, int y);

Draws a single pixel at the given position.

    void IupDrawBezier(Ihandle* ih, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

Draws a cubic Bezier curve from (x1,y1) to (x4,y4) with control points (x2,y2) and (x3,y3).

    void IupDrawQuadraticBezier(Ihandle* ih, int x1, int y1, int x2, int y2, int x3, int y3);

Draws a quadratic Bezier curve from (x1,y1) to (x3,y3) with control point (x2,y2).

### Gradients

Gradient colors are passed as parameters, not controlled by DRAWCOLOR. Both colors also accept the alpha component in the "R G B A" format for translucent gradients.

    void IupDrawLinearGradient(Ihandle* ih, int x1, int y1, int x2, int y2, float angle, const char* color1, const char* color2);

Draws a linear gradient fill in the given rectangle.
The angle is in degrees and defines the gradient direction.

    void IupDrawRadialGradient(Ihandle* ih, int cx, int cy, int radius, const char* colorCenter, const char* colorEdge);

Draws a radial gradient fill centered at (cx, cy) with the given radius.

    void IupDrawLinearGradientStops(Ihandle* ih, int x1, int y1, int x2, int y2, float angle, const char** colors, const float* offsets, int count);
    void IupDrawRadialGradientStops(Ihandle* ih, int cx, int cy, int radius, const char** colors, const float* offsets, int count);

Same as the two-color functions with **count** color stops, from 2 to 64. **offsets** is NULL or an array of **count** ascending positions in the 0-1 range. NULL uses evenly spaced stops. Colors accept an alpha component.

Two stops at the same offset produce a hard color edge.
In Qt, the last color at a duplicate offset is used.

### Paths and Sources

A path contains line, curve and arc segments. It is kept until IupDrawPathBegin or IupDrawEnd.

    void IupDrawPathBegin(Ihandle* ih);

Creates an empty current path.

    void IupDrawPathMoveTo(Ihandle* ih, int x, int y);

Starts a new subpath at (x, y).

    void IupDrawPathLineTo(Ihandle* ih, int x, int y);

Adds a line from the current point to (x, y). With no current point, starts a new subpath at (x, y).

    void IupDrawPathCurveTo(Ihandle* ih, int x1, int y1, int x2, int y2, int x3, int y3);

Adds a cubic Bezier curve to (x3, y3), with control points (x1, y1) and (x2, y2).

    void IupDrawPathQuadTo(Ihandle* ih, int x1, int y1, int x2, int y2);

Adds a quadratic Bezier curve to (x2, y2), with control point (x1, y1).

    void IupDrawPathArcTo(Ihandle* ih, int cx, int cy, int rx, int ry, double a1, double a2);

Adds an elliptical arc centered at (cx, cy), with horizontal radius **rx** and vertical radius **ry**. The angles are in degrees and counter-clockwise relative to the 3 o'clock position. A line connects the current point to the start of the arc.

    void IupDrawPathClose(Ihandle* ih);

Closes the current subpath with a line to its starting point.

    void IupDrawPathFill(Ihandle* ih, int rule);

Fills the current path with the current source. Open subpaths are closed before filling. **rule** can be IUP_DRAW_RULE_WINDING or IUP_DRAW_RULE_EVENODD.

    void IupDrawPathStroke(Ihandle* ih);

Strokes the current path with the current source. **DRAWSTYLE** controls the line style and **DRAWLINEWIDTH** controls the line width.

    void IupDrawSetClipPath(Ihandle* ih, int rule);

Sets the current path as the clipping region, replacing the previous clipping region. **rule** can be IUP_DRAW_RULE_WINDING or IUP_DRAW_RULE_EVENODD. IupDrawResetClip removes the clipping region.

    void IupDrawSetSourceSolid(Ihandle* ih, const char* color);

Sets a solid "R G B [A]" source for path drawing. Also sets **DRAWCOLOR**.

    void IupDrawSetSourceLinearGradient(Ihandle* ih, int x1, int y1, int x2, int y2, float angle, const char** colors, const float* offsets, int count);
    void IupDrawSetSourceRadialGradient(Ihandle* ih, int cx, int cy, int radius, const char** colors, const float* offsets, int count);

Sets a linear or radial gradient source for path drawing. The gradient geometry, colors, offsets and count have the same meaning as IupDrawLinearGradientStops and IupDrawRadialGradientStops.

    void IupDrawResetSource(Ihandle* ih);

Resets the current source to **DRAWCOLOR**.

### Text and Images

    void IupDrawText(Ihandle* ih, const char* str, int len, int x, int y, int w, int h);

Draws a text in the given position using the font defined by **DRAWFONT**, if not defined, then use [FONT](../attrib/iup_font.md).
The size of the string is used only in C. Can be -1 so **strlen** is used internally.
The coordinates are relative to the top-left corner of the text.
Strings with multiple lines are accepted using '\n" as line separator.
Horizontal text alignment inside the given box can be controlled using **DRAWTEXTALIGNMENT** attribute: ALEFT (default), ARIGHT and ACENTER options.
The lines of a multiple line text are also aligned against each other.
For single line texts, if the text is larger than its box and **DRAWTEXTWRAP**=YES, then the line will be automatically broken in multiple lines.
Notice that this is done internally by the system, the element natural size will still use only a single line.
For the remaining lines to be visible, the element should use EXPAND=VERTICAL or set a SIZE/RASTERSIZE with enough height for the wrapped lines.
If the text is larger that its box and **DRAWTEXTELLIPSIS**=YES, an ellipsis ("...") will be placed near the last visible part of the text and replace the invisible part.
It will be ignored when WRAP=YES.
w and h are optional and can be -1 or 0, the text size will be used, so WRAP nor ELLIPSIS will not produce any changes.
The text is not automatically clipped to the rectangle, if **DRAWTEXTCLIP**=YES it will be clipped but depending on the driver may affect the clipping set by IupDrawSetClipRect.
The text can be drawn in any angle using **DRAWTEXTORIENTATION**, in degrees and counterclockwise, its layout is not centered inside the given rectangle when text is oriented, to center the layout use **DRAWTEXTLAYOUTCENTER**=YES.
In Motif only the Xft build draws non-Latin text, the default build is 8 bit.

    void IupDrawImage(Ihandle* ih, const char* name, int x, int y, int w, int h);

Draws an image given its name. The coordinates are relative to the top-left corner of the image.
The image name follows the same behavior as the IMAGE attribute used by many controls.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
See also [IupImage](../elem/iup_image.md).
The **DRAWMAKEINACTIVE** attribute can be used to force the image to be drawn with an inactive state appearance.
The **DRAWBGCOLOR** can be used to control the inactive state background color or when transparency is flattened.
w and h are optional and can be -1 or 0, then the source size will be used, and no zoom will be performed.

The **DRAWIMAGESRCRECT** attribute selects a source sub-rectangle in the format "X Y W H" (in image pixels), so a region of the image can be drawn without creating a new image. Default: NULL (whole image).

The **DRAWIMAGETINT** attribute is a color "R G B [A]" that replaces the image color keeping its alpha channel, recoloring a monochrome image while preserving its shape. The tint alpha component is multiplied into the image alpha. Default: NULL (no tint).

The **DRAWIMAGEOPACITY** attribute scales the opacity of the drawn image (0-255). Default: 255 (opaque).

The **DRAWIMAGEQUALITY** attribute controls the interpolation used when the image is scaled. Can be: LINEAR or NEAREST. Default: LINEAR.

    void IupDrawSelectRect(Ihandle* ih, int x1, int y1, int x2, int y2);

Draws a selection rectangle.

    void IupDrawFocusRect(Ihandle* ih, int x1, int y1, int x2, int y2);

Draws a focus rectangle.

### Information

    void IupDrawGetSize(Ihandle* ih, int *w, int *h);

Returns the drawing area size. In C unwanted values can be NULL.
Valid only between IupDrawBegin and IupDrawEnd, outside them read the canvas DRAWSIZE attribute.

    void IupDrawGetTextSize(Ihandle* ih, const char* str, int len, int *w, int *h);

Returns the given text size using the font defined by DRAWFONT, if not defined then use [FONT](../attrib/iup_font.md).
In C, unwanted values can be NULL, and if len is -1 the string must be 0 terminated, and len will be calculated using strlen.
In WinUI the size can differ from the natural size of a control with the same text.

    void IupDrawGetTextMetrics(Ihandle* ih, int *ascent, int *descent, int *line_height);

Returns the font vertical metrics for the font defined by DRAWFONT, if not defined then use [FONT](../attrib/iup_font.md).
**ascent** is the baseline-to-top distance, **descent** the baseline-to-bottom distance, and **line_height** the line spacing. IupDrawText places the baseline at y+ascent. In C, unwanted values can be NULL.

    void IupDrawGetImageInfo(const char* name, int *w, int *h, int *bpp);

Returns the given image size and bits per pixel. bpp can be 8, 24 or 32.
In C unwanted values can be NULL.

    Ihandle* IupDrawGetImage(Ihandle* ih);

Returns the offscreen drawing buffer as an IupImage.
Must be called between IupDrawBegin and IupDrawEnd.

    char* IupDrawGetSvg(Ihandle* ih);

Returns an SVG string representation of the drawing.
Calls the canvas ACTION callback to draw the SVG, so it must be called outside IupDrawBegin and IupDrawEnd.
The returned string must be freed with `free`.

### Example

    static int canvas_action(Ihandle *ih)
    {
      int w, h;

      IupDrawBegin(ih);

      IupDrawGetSize(ih, &w, &h);

      /* white background */
      IupSetAttribute(ih, "DRAWCOLOR", "255 255 255");
      IupSetAttribute(ih, "DRAWSTYLE", "FILL");
      IupDrawRectangle(ih, 0, 0, w - 1, h - 1);

      /* Guide Lines */
      IupSetAttribute(ih, "DRAWCOLOR", "255 0 0");
      IupSetAttribute(ih, "DRAWSTYLE", "STROKE");
      IupDrawLine(ih, 10, 5, 10, 19);
      IupDrawLine(ih, 14, 5, 14, 19);
      IupDrawLine(ih, 5, 10, 19, 10);
      IupDrawLine(ih, 5, 14, 19, 14);

      /* Stroke Rectangle, must cover guide lines */
      IupSetAttribute(ih, "DRAWCOLOR", "0 0 0");
      IupSetAttribute(ih, "DRAWSTYLE", "STROKE");
      IupDrawRectangle(ih, 10, 10, 14, 14);

      /* Guide Lines */
      IupSetAttribute(ih, "DRAWCOLOR", "255 0 0");
      IupDrawLine(ih, 10, 5 + 30, 10, 19 + 30);
      IupDrawLine(ih, 14, 5 + 30, 14, 19 + 30);
      IupDrawLine(ih, 5, 10 + 30, 19, 10 + 30);
      IupDrawLine(ih, 5, 14 + 30, 19, 14 + 30);

      /* Fill Rectangle, must cover guide lines */
      IupSetAttribute(ih, "DRAWCOLOR", "0 0 0");
      IupSetAttribute(ih, "DRAWSTYLE", "FILL");
      IupDrawRectangle(ih, 10, 10 + 30, 14, 14 + 30);

      IupSetAttribute(ih, "DRAWCOLOR", "255 0 0");
      IupDrawRectangle(ih, 30, 10, 50, 30);

      IupSetAttribute(ih, "DRAWCOLOR", "0 0 0");
      IupDrawArc(ih, 30, 10, 50, 30, 0, 360);

      IupSetAttribute(ih, "DRAWCOLOR", "255 0 0");
      IupDrawRectangle(ih, 60, 10, 80, 30);

      IupSetAttribute(ih, "DRAWCOLOR", "0 0 0");
      IupSetAttribute(ih, "DRAWSTYLE", "FILL");
      IupDrawArc(ih, 60, 10, 80, 30, 0, 360);

      IupSetAttribute(ih, "DRAWCOLOR", "255 0 0");
      IupDrawRectangle(ih, 30, 10 + 30, 50, 30 + 30);

      IupSetAttribute(ih, "DRAWCOLOR", "0 0 0");
      IupDrawArc(ih, 30, 10 + 30, 50, 30 + 30, 45, 135);

      IupSetAttribute(ih, "DRAWCOLOR", "255 0 0");
      IupDrawRectangle(ih, 60, 10 + 30, 80, 30 + 30);

      IupSetAttribute(ih, "DRAWCOLOR", "0 0 0");
      IupSetAttribute(ih, "DRAWSTYLE", "FILL");
      IupDrawArc(ih, 60, 10 + 30, 80, 30 + 30, 45, 135);

      IupSetAttribute(ih, "DRAWCOLOR", "255 0 0");
      IupDrawLine(ih, 20, 70 - 2, 20, 70 + 2);
      IupDrawLine(ih, 20 - 2, 70, 20 + 2, 70);

      IupSetAttribute(ih, "DRAWCOLOR", "0 0 0");
      IupSetAttribute(ih, "DRAWFONT", "Helvetica, -30");
      IupDrawGetTextSize(ih, "Text", -1, &w, &h);
      IupSetAttribute(ih, "DRAWSTYLE", "STROKE");
      IupDrawRectangle(ih, 20, 70, 20 + w, 70 + h);
      IupDrawText(ih, "Text", 0, 20, 70, -1, -1);
      IupSetAttribute(ih, "DRAWTEXTORIENTATION", "0");

      IupSetAttribute(ih, "DRAWSTYLE", "STROKE");
      IupDrawLine(ih, 10, 110, 100, 110);
      IupSetAttribute(ih, "DRAWSTYLE", "STROKE_DASH");
      IupDrawLine(ih, 10, 110 + 5, 100, 110 + 5);
      IupSetAttribute(ih, "DRAWSTYLE", "STROKE_DOT");
      IupDrawLine(ih, 10, 110 + 10, 100, 110 + 10);
      IupSetAttribute(ih, "DRAWSTYLE", "STROKE_DASH_DOT");
      IupDrawLine(ih, 10, 110 + 15, 100, 110 + 15);
      IupSetAttribute(ih, "DRAWSTYLE", "STROKE_DASH_DOT_DOT");
      IupDrawLine(ih, 10, 110 + 20, 100, 110 + 20);

      IupDrawImage(ih, "Test8bpp", 110, 10, -1, -1);
      IupDrawImage(ih, "Test24bpp", 110, 40, -1, -1);
      IupDrawImage(ih, "Test32bpp", 110, 70, -1, -1);

      IupSetAttribute(ih, "DRAWFONT", "Helvetica, Bold -15");
      IupDrawText(ih, IupGetAttribute(ih, "DRAWDRIVER"), -1, 70, 135, -1, -1);

      IupDrawEnd(ih);
      return IUP_DEFAULT;
    }


### See Also

[IupCanvas](../elem/iup_canvas.md), [IupBackgroundBox](../elem/iup_backgroundbox.md), [IupUpdate](../func/iup_update.md), [IupRedraw](iup_draw.md)
