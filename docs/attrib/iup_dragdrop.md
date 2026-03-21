## DRAG & DROP

When enabled, allow the use of callbacks for controlling the drag and drop handling.

The user starts a drag and drop transfer by pressing the mouse button over the data (Windows and GTK: left button; Motif: middle button) which is referred to as the drag source.
The data can be dropped in any location that has been registered as a drop target.
The drop occurs when the user releases the mouse button.
This can be done inside a control, from one control to another in the same dialog, in different dialogs of the same application, or between different applications (the other application does NOT need to be implemented with IUP).

In IUP, a drag and drop transfer can result in the data being moved or copied.
A **copy** operation is enabled with the CTRL key pressed. A **move** operation is enabled with the SHIFT key pressed.
A move operation will be possible only if the attribute DRAGSOURCEMOVE is Yes.
When no key is pressed, the default operation is **copy** when DRAGSOURCEMOVE=No and **move** when DRAGSOURCEMOVE=Yes.
The user can cancel a drag at any time by pressing the ESCAPE key.

Steps to use the Drag & Drop support in an IUP application:

- **AT SOURCE:**
  - Enable the element as source using the attribute DRAGSOURCE=YES;
  - Define the data types supported by the element for the drag operation using the DRAGTYPES attribute;
  - Register the required callbacks DRAGBEGIN_CB, DRAGDATASIZE_CB and DRAGDATA_CB for drag handling.
DRAGEND_CB is the only optional drag callback, all other callbacks and attributes must be set.
- **AT TARGET:**
  - Enable the element as target using the attribute DROPTARGET=YES;
  - Define the data types supported by the element for the drop using the DROPTYPES attribute;
  - Register the required callback DROPDATA_CB for handling the data received.
This callback and all the drop target attributes must be set too.
DROPMOTION_CB is the only optional drop callback.

### Affects

[IupLabel](../elem/iup_label.md), [IupText](../elem/iup_text.md), [IupList](../elem/iup_list.md), [IupTree](../elem/iup_tree.md), [IupCanvas](../elem/iup_canvas.md) and [IupDialog](../dlg/iup_dialog.md).

### Attributes at Drag Source

**DRAGC******URSOR (non-inheritable):** name of an image to be used as cursor during drag.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
See also [IupImage](../elem/iup_image.md). DRAGSOURCE** (non-inheritable): Set up a control as a source for drag operations.
Default: NO.

**DRAGTYPES** (non-inheritable): A list of data types that are supported by the source.
Accepts a string with one or more names separated by commas. See Notes below for a list of known names.
Must be set.

**DRAGSOURCEMOVE (non-inheritable)**: Enables the move action. Default: NO (only copy is enabled).

### Attributes at Drop Target

**DROPTARGET** (non-inheritable)**: ** Set up a control as a destination for drop operations.
Default: NO.

**DROPTYPES (non-inheritable)**: A list of data types that are supported by the target.
Accepts a string with one or more names separated by commas. See the Notes below for a list of known names.
Must be set.

### Callbacks at Drag Source (Must be set when DRAGSOURCE=Yes)

**DRAGBEGIN_CB**: notifies a source that drag started.
It is called when the mouse starts a drag operation.

    int function(Ihandle* ih, int x, int y)

**ih**: identifier of the element that activated the event.\
**x, y**: cursor position relative to the top-left corner of the element.

**Returns**: If IUP_IGNORE is returned, the drag is aborted.

**DRAGDATASIZE_CB**: request for size of drag data from source.
It is called when the data is dropped, before the **DRAGDATA_CB** callback.

    int function(Ihandle* ih, char* type)

**ih**: identifier of the element that activated the event.\
**type**: type of the data. It is one of the registered types in **DRAGTYPES****.
Returns**: the size in bytes for the data.
It will be used to allocate the buffer size for the data in transfer.

**DRAGDATA_CB**: request for drag data from source. It is called when the data is dropped.

    int function(Ihandle* ih, char* type, void* data, int size)

**ih**: identifier of the element that activated the event.\
**type**: type of the data. It is one of the registered types in **DRAGTYPES****.**\
**data**: buffer to be filled by the application..\
**size**: buffer size in bytes. The same value returned by **DRAGDATASIZE_CB.
DRAGEND_CB**: notifies source that drag is done. The only drag callback that is **optional**.
It is called after the data has been dropped.

    int function(Ihandle* ih, int action)

**ih**: identifier of the element that activated the event.\
**action**: action performed by the operation (1 = move, 0 = copy, -1 = drag failed or aborted)

If action is 1, it is the responsibility of the application to remove the data from source.

### Callbacks at Drop Target (Must be set when DROPTARGET=Yes)

**DROPDATA_CB**: source has sent target the requested data. It is called when the data is dropped.
If both drag and drop were in the same application, it would be called after the **DRAGDATA_CB** callback.

    int function(Ihandle* ih, char* type, void* data, int size, int x, int y)

**ih**: identifier of the element that activated the event.\
**type**: type of the data. It is one of the registered types in **DROPTYPES****.**\
**data**: content data received in the drop operation..\
**size**: data size in bytes.\
**x, y**: cursor position relative to the top-left corner of the element.

**DROPMOTION_CB**: notifies destination about drag pointer motion. The only drop callback that is **optional**.
It is called when the mouse moves over any valid drop site.

    int function(Ihandle *ih, int x, int y, char *status);

**ih**: identifier of the element that activated the event.\
**x**, **y**: position in the canvas where the event has occurred, in pixels.\
**status**: status of mouse buttons and certain keyboard keys at the moment the event was generated.
The same macros used for [BUTTON_CB](../call/iup_button_cb.md) can be used for this status.

### Notes

Drag and Drop support can be set independently.
A control can have a drop without drag support and vice versa.

Here are some common Drag&Drop types defined by existing applications:

- "TEXT" used for regular text without formatting. Automatically translated to CF_TEXT in Windows.
- content MIME types, like "text/uri-list", "text/html", "image/png", "image/jpeg", "image/bmp" and "image/gif".
- "UTF8_STRING" in GTK and "UNICODETEXT" in Windows.
- "COMPOUND_TEXT" in GTK and "Rich Text Format" in Windows.
- "BITMAP" and "DIB" in Windows. Automatically translated to CF_BITMAP and CF_DIB.

### Examples

[list2.c](../../examples/C/list2.c)
