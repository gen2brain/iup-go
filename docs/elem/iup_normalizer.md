## IupNormalizer

Creates a void container that does not affect the dialog layout.
It acts by normalizing all the controls in a list, so their natural size becomes the biggest natural size amongst them.
All natural widths will be set to the biggest width, and all natural heights will be set to the biggest height.
The controls of the list must be inside a valid container in the dialog.

### Creation

    Ihandle* IupNormalizer(Ihandle *ih_first, ...);
    Ihandle* IupNormalizerV(Ihandle* ih_first, va_list arglist);
    Ihandle* IupNormalizerv(Ihandle **ih_list);

**ih_first**, ... : List of the identifiers that will be normalized.
NULL must be used to define the end of the list in C.
It can be empty, but in C must have at least the NULL terminator..

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**NORMALIZE** (non-inheritable): normalization direction. Can be HORIZONTAL, VERTICAL or BOTH.
These are the same values of the NORMALIZESIZE attribute. Default: HORIZONTAL.

**ADDCONTROL** (non-inheritable, write-only): Adds a control to the normalizer.
The value passed must be the name of an element.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an element to a name.

**ADDCONTROL_HANDLE** (non-inheritable, write-only): Adds a control to the normalizer.
The value passed must be a handle of an element.

**DELCONTROL** (non-inheritable, write-only): Removes a control from the normalizer.
The value passed must be the name of an element.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an element to a name.

**DELCONTROL_HANDLE** (non-inheritable, write-only): Removes a control from the normalizer.
The value passed must be a handle of an element.

### Attributes (at any Control)

[FLOATING](../attrib/iup_floating.md) (non-inheritable) **(at children only)**: If a child of a container has FLOATING=YES then its size and position will be ignored by the layout processing.
Default: "NO".

**NORMALIZERGROUP** (non-inheritable) **(at controls only)**: name of a normalizer element to which to automatically add the control.
If an element with that name does not exist, then one is created.
The normalizer can later be retrieved using IupGetHandle.

### Notes

A normalizer is the same effect as the NORMALIZESIZE attribute of the **IupVbox** and **IupHbox** controls, but it can be used for elements with different parents, it changes the **User** size of the elements.

It is NOT necessary to add the normalizer to a dialog hierarchy.
Every time the NORMALIZE attribute is set, a normalization occurs.
But if the normalizer is added to a dialog hierarchy, then whenever the **Natural** size is calculated a normalization occurs, so add it to the hierarchy before the elements you want to normalize or its normalization will be late during the layout computation.

The elements do NOT need to be children of the same parent, do NOT need to be mapped, and do NOT need to be in a complete hierarchy of a dialog.

The elements are NOT children of the normalizer, so **IupAppend**, **IupInsert** and **IupDetach** cannot be used.
To add or remove elements, use the **ADDCONTROL** and **DELCONTROL** attributes.

Notice that the NORMALIZERGROUP attribute can simplify a lot the process of creating a normalizer, so you do not need to list several elements from different parts of the dialog.

### Examples

Here **IupNormalizer** is used to normalize the horizontal size of several labels that are in different containers.
Since it needs to be done once only the **IupNormalizer** is destroyed just after it is initialized.

    IupDestroy(IupSetAttributes(IupNormalizer(IupGetChild(hsi_vb, 0),  /* Hue Label */
                                              IupGetChild(hsi_vb, 1),  /* Saturation Label */
                                              IupGetChild(hsi_vb, 2),  /* Intensity Label */
                                              IupGetChild(clr_vb, 0),  /* Opacity Label */
                                              IupGetChild(clr_vb, 1),  /* Hexa Label */
                                              NULL), "NORMALIZE=HORIZONTAL"));

The following case use the internal normalizer in an Hbox:

    button_box = IupHbox(
        IupFill(),
        button_ok,
        button_cancel,
        button_help,
        NULL);
    IupSetAttribute(button_box, "NORMALIZESIZE", "HORIZONTAL");

### See Also

[IupHbox](../elem/iup_hbox.md), [IupVbox](../elem/iup_vbox.md), [IupGridBox](../elem/iup_gridbox.md)
