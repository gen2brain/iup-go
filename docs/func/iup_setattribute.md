## IupSetAttribute

Sets an interface element attribute. See also the [Attributes Guide](../attrib.md) section.

### Parameters/Return

    void IupSetAttribute(Ihandle *ih, const char *name, const char *value);
    void IupSetStrAttribute(Ihandle *ih, const char *name, const char *value);

    void IupSetAttributeId(Ihandle *ih, const char *name, int id, const char *value);
    void IupSetStrAttributeId(Ihandle *ih, const char *name, int id, const char *value);

    void IupSetAttributeId2(Ihandle* ih, const char* name, int lin, int col, const char* value);
    void IupSetStrAttributeId2(Ihandle* ih, const char* name, int lin, int col, const char* value);

**ih**: Identifier of the interface element. If NULL, the attribute is set in the global environment.\
**name**: name of the attribute.\
**id, lin, col**: used when the attribute has additional ids.\
**value**: value of the attribute. If NULL, the default value will be used.

### Utility Functions

These functions can also be used to set attributes from the element:

    void IupSetStrf  (Ihandle* ih, const char* name, const char* format, ...);
    void IupSetStrfV (Ihandle* ih, const char* name, const char* format, va_list arglist);
    void IupSetInt   (Ihandle* ih, const char* name, int value);
    void IupSetFloat (Ihandle* ih, const char* name, float value);
    void IupSetDouble(Ihandle* ih, const char* name, double value);
    void IupSetRGB   (Ihandle *ih, const char* name, unsigned char r, unsigned char g, unsigned char b);
    void IupSetRGBA  (Ihandle *ih, const char* name, unsigned char r, unsigned char g, unsigned char b, unsigned char a);

    void IupSetStrfId  (Ihandle *ih, const char* name, int id, const char* format, ...);
    void IupSetStrfIdV (Ihandle* ih, const char* name, int id, const char* format, va_list arglist);
    void IupSetIntId   (Ihandle* ih, const char* name, int id, int value);
    void IupSetFloatId (Ihandle* ih, const char* name, int id, float value);
    void IupSetDoubleId(Ihandle* ih, const char* name, int id, double value);
    void IupSetRGBId   (Ihandle *ih, const char* name, int id, unsigned char r, unsigned char g, unsigned char b);

    void IupSetStrfId2  (Ihandle* ih, const char* name, int lin, int col, const char* format, ...);
    void IupSetStrfId2V (Ihandle* ih, const char* name, int lin, int col, const char* format, va_list arglist);
    void IupSetIntId2   (Ihandle* ih, const char* name, int lin, int col, int value);
    void IupSetFloatId2 (Ihandle* ih, const char* name, int lin, int col, float value);
    void IupSetDoubleId2(Ihandle* ih, const char* name, int lin, int col, double value);
    void IupSetRGBId2   (Ihandle *ih, const char* name, int lin, int col, unsigned char r, unsigned char g, unsigned char b);

The **IupSetStrf**\* functions (same as **IupSetfAttribute**) use the same format specification as the **sprintf** function in C.
They are useful when several values must be combined into one string.
When passing float values, the format "%.9g" is used to maximize precision.
When passing double values, the format "%.17g" is used to maximize precision.

All the utility functions use the **IupSetStrAttribute**\* functions internally.

### Notes

See the [Attributes Guide](../attrib.md) for more details.

**IupSetAttribute** can store only constant strings (like "Title", "30", etc) or application pointers.
The given value is not duplicated as a string, only a reference is stored.
Application custom attributes can be stored this way, such as a context structure to be used in a callback.

**IupSetStrAttribute** (same as **IupStoreAttribute**) can only store strings.
The given string value will be duplicated internally.

**Id** based attributes are always non-inheritable, so all **IupSet*Id** functions will not propagate the attribute to the children.
**Ids** are usually non-negative values (id >= 0), with a few exceptions.

### Examples

A common mistake when using **IupSetAttribute** is to use local string arrays to set attributes.
For example:

    char value[30];
    sprintf(value, "CODE - %d", i);
    IupSetAttribute(dlg, "BADEXAMPLE", value); // WRONG  (value pointer will be internally stored,
                                               //         but its memory will be released at the end of this scope)
                                               // a common bad practice is to declare value as static
                                               // Use IupSetStrAttribute in this case

    char *data = malloc(30);
    sprintf(data, "%d", i);
    IupSetAttribute(dlg, "EXAMPLE", data);     // correct  (to avoid memory leaks the pointer must be freed
                                               //           after the dialog has been destroyed)

    IupSetAttribute(dlg, "VISIBLE", "YES");    // correct (constant values still exist after this scope)
    IupSetAttribute(text, "VALUE", "Hello!");
    IupSetAttribute(indicator, "VALUE", "ON");

    char attrib[30];
    sprintf(attrib, "MY ITEM (%d)", i);
    IupSetAttribute(dlg, attrib, "Test");      // correct (attribute names are always internally duplicated)

    struct MyData {
      int x;
      int y;
    } myData;

    IupSetAttribute(text, "myData", (char*)&myData);  // WRONG, will work only if myData is a global variable.

    struct MyData* mydata = malloc(sizeof(struct MyData));
    IupSetAttribute(dlg, "MYDATA", (char*)mydata);    // correct (unknown attributes will be stored as pointers)

Defines a radio's initial value:

    Ihandle *portrait = IupToggle("Portrait", NULL);
    Ihandle *landscape = IupToggle("landscape", NULL);
    Ihandle *box = IupVbox(portrait, IupFill(), landscape, NULL);
    Ihandle *mode = IupRadio(box);
    IupSetHandle("landscape", landscape); /* associates a name to initialize the radio */
    IupSetAttribute(mode, "VALUE", "landscape"); /* defines the radio's initial value */

### See Also

[IupGetAttribute](iup_getattribute.md), [IupSetAttributes](iup_setattributes.md), [IupGetAttributes](iup_getattributes.md), [IupSetGlobal](iup_setglobal.md), [IupGetGlobal](iup_getglobal.md)
