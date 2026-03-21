## IupGetAttribute

Returns the name of an interface element attribute.
See also the [Attributes Guide](../attrib.md) section.

### Parameters/Return

    char *IupGetAttribute(Ihandle *ih, const char *name);
    char *IupGetAttributeId(Ihandle *ih, const char *name, int id);
    char* IupGetAttributeId2(Ihandle* ih, const char* name, int lin, int col);

**ih**: Identifier of the interface element. If NULL will retrieve from the global environment.\
**name**: name of the attribute.\
**id, lin, col**: used when the attribute has additional ids.

**Returns:** the attribute value or NULL if the attribute is not set or does not exist.\

### Utility Functions

These functions can also be used to get attributes from the element:

    int    IupGetInt   (Ihandle* ih, const char* name);
    int    IupGetIntInt(Ihandle* ih, const char* name, int *i1, int *i2);
    int    IupGetInt2  (Ihandle* ih, const char* name);
    float  IupGetFloat (Ihandle* ih, const char* name);
    double IupGetDouble(Ihandle* ih, const char* name);
    void   IupGetRGB   (Ihandle *ih, const char* name, unsigned char *r, unsigned char *g, unsigned char *b);
    void   IupGetRGBA  (Ihandle *ih, const char* name, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a);

    int    IupGetIntId   (Ihandle* ih, const char* name, int id);          
    float  IupGetFloatId (Ihandle* ih, const char* name, int id);
    double IupGetDoubleId(Ihandle* ih, const char* name, int id);
    void   IupGetRGBId   (Ihandle *ih, const char* name, int id, unsigned char *r, unsigned char *g, unsigned char *b);

    int    IupGetIntId2   (Ihandle* ih, const char* name, int lin, int col);
    float  IupGetFloatId2 (Ihandle* ih, const char* name, int lin, int col);
    double IupGetDoubleId2(Ihandle* ih, const char* name, int lin, int col);
    void   IupGetRGBId2   (Ihandle *ih, const char* name, int lin, int col, unsigned char *r, unsigned char *g, unsigned char *b);

**IupGetIntInt** retrieves two integers separated by 'x', ':' or ',' and returns the number of returned values (0, 1 or 2).
**IupGetInt2** returns just the second value.

### Notes

See the [Attributes Guide](../attrib.md) for more details.

The returned value is not necessarily the same pointer used by the application to define the attribute value.
The pointers of internal IUP attributes returned by **IupGetAttribute** should **never** be freed or changed, except when it is a custom application pointer that was stored using **IupSetAttribute** and allocated by the application.

 The returned pointer can be used safely even if **IupGetGlobal** or **IupGetAttribute** are called several times.
But not too many times, because it is an internal buffer and after IUP may reuse it after around 50 calls.


### Examples

[Browse for Example Files](../../examples/)

### See Also

[IupSetAttribute](iup_setattribute.md), [IupSetAttributes](iup_setattributes.md), [IupGetHandle](iup_gethandle.md), [IupSetGlobal](iup_setglobal.md), [IupGetGlobal](iup_getglobal.md)
