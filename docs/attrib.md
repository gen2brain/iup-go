# Attributes

Attributes are used to change or consult properties of elements.
Each element has a set of attributes that affect it, and each attribute can work differently for each element.
Depending on the element, its value can be computed or simply verified.
Also, it can be internally stored or not.

Attribute names are always upper case, lower case names will not work.
But attribute values like "YES", "NO", "TOP", are case-insensitive, so "Yes", "no", "top", and other variations will work.

If not defined, their value can be inherited from the parent container.

## Guide

### Using

Attributes are a way to send and obtain information to and from elements.
They are used by the application to communicate with the user interface system, on the other hand callbacks are used by the application to receive notifications from the system that the user or the system itself has interacted with the user interface of the application.

There are several functions to access attributes, see the documentation of the [IupSetAttribute](func/iup_setattribute.md) and [IupGetAttribute](func/iup_getattribute.md) for more options.

When an attribute is modified (**Set**) it is stored internally at the hash table of the control only if the control class allows it.
If the value is NULL, the attribute will also be removed from the hash table and the default value will be used if there is one defined.
Finally, the attribute is updated for the children of the control if they do not have the attribute defined in their own hash table.
Here is a pseudocode:

    IupSetAttribute(ih, name, value)
    {
      if ih.SetClassAttribute(name, value)==store then
        ih.SetHashTableAttribute(name, value)
      endif

      if (ih.IsInheritable(name))
        -- NotifyChildren
        for each child of ih do
           if not child.GetHashTableAttribute(name) then
            child.SetClassAttribute(name, value)
            child.NotifyChildren(name, value)
           endif
        endfor
      endif
    }

When an attribute is retrieved (**Get**), it will first be checked at the control class.
If not defined, then it checks in the hash table.
If not defined, it checks its parent hash table and so forth, until it reaches the dialog.
And finally, if still not defined then a default value is returned (the default value can also be NULL).

    value = IupGetAttribute(ih, name)
    {
      value = ih.GetClassAttribute(name)

      if not value then
        value = ih.GetHashTableAttribute(name)
      endif

      if not value and ih.isInheritable(name) then
        parent = ih.parent
        while (parent and not value)
          value = parent.GetHashTableAttribute(name)
          parent = parent.parent
        endwhile
      endif

      if not value then
        value = ih.GetDefaultAttribute(name)
      endif
    }

Notice that the parent recursion is done only at the parent hash table; the parent control class is not consulted.

The control class can update or retrieve a value even if the control is not mapped.
When the control is not mapped and its implementation cannot process the attribute, then the attribute is simply stored in the hash table.
After the element is mapped, its attributes are re-processed to be updated in the native system, and they can be removed from the hash table at that time.

All this flexibility turns the attribute system very complex with several nuances.
If the attribute is checked before the control is mapped and just after, its value can be completely different.
Depending on how the attribute is stored, its inheritance can be completely ignored.

Attribute **names** are always upper case, lower case names will not work.
But attribute **values** like "YES", "NO", "TOP", are case-insensitive, so "Yes", "no", "top", and other variations will work.

Boolean attributes accept the values "1", "YES" or "ON" for **true**, and "0", "NO" or "OFF" for **false**.
But they will return the value described in the documentation.
You can also use **IupSetInt** with 1 for **true** and 0 for **false**.
**IupGetInt** will return 1 for any of the **true** values, and 0 for any of the **false** values.

Floating point numbers when stored as strings use the application locale for decimal separator.
Notice that by default C applications use the C locale, not the current system locale, in this case decimal separator is ".".

Combination of values in a single attribute is common, but there are no specific definitions on how they can be combined.
Although all attributes that represent sizes using width and height adopt the "WxH" definition, for example, "640x480".
Position usually adopts "x,y" definition, range is usually "x1-x2" but can also be "x1:x2", so there are variations that for compatibility reasons were maintained.
Cell specification is always "lin:col".

Color values are in the "R G B A" format. Each component from 0 to 255.
Alpha is optional and assumed to be 255 if not specified.
But it is only supported in custom controls drawn by IUP, example IupGauge, IupDial, all IupFlat* controls, and only when using OpenGL, Cairo or Direct2D draw drivers.
Alpha is never supported when using X11, GDI or GDK draw drivers.
Color values can also be specified in hexadecimal notation in the format "#RRGGBBAA".

With **IupSetAttribute** you can also store application pointers that can be strings or not.
This can be very useful, for instance, used inside **callbacks**.
For example, by storing a C pointer of an application defined structure, the application can retrieve this pointer inside the callback through function **IupGetAttribute**.
Therefore, even if the callbacks are global functions, the same callback can be used for several objects, even of different types.

Some controls, like **IupList**, **IupTree**, **IupTabs** and **IupMatrix**, have ids associated with some attributes, so its value will affect only the respective id item in the control.
For example, "TITLE3" will set the TITLE attribute for item 3.
To set that kind of attribute **IupSetAttribute** can be used, but **IupSetAttributeId** can also be used specially if the id is a variable.

There are attributes common to all the elements.
In some cases, common attributes behave differently in different elements, but in such cases, there are comments in the documentation of the element explaining the different behavior.

### Inheritance

Elements included in other elements can inherit their attributes.
There is an **inheritance** mechanism inside a given child tree.

This means, for example, that if you set the "MARGIN" attribute of a Vbox containing several other elements, including other Vboxes, all the elements depending on the attribute "MARGIN" will be affected, **except** for those who the "MARGIN" attribute is already defined.

Please note that not all attributes are inherited.
As general rules, the following attributes are **NON**-inheritable always:

- Essential attributes like VALUE, TITLE, SIZE, RASTERSIZE, X and Y
- Id numbered attributes (like "1" or "MARK1:1")
- Handle names (like "CURSOR", "IMAGE" and "MENU")
- Pointers that are not strings (like WID)
- Read-only or write-only attributes
- Internal attributes that starts with "_IUP"

Inheritable attributes are stored in the hash table, so the IupGet/SetAttribute logic can work, even if the control class stores it internally.
But when you change an attribute to NULL, then its value is removed from the hash table and the default value if any is passed to the native system.

When consulted, the attribute is first checked at the control class. If not defined, then it checks in the hash table.
If not defined in its hash table, the attribute will be inherited from its parent's hash table and so forth, until it reaches the root child (usually the dialog).
But if still then the attribute is not defined, a default value for the element is returned (the default value can also be NULL).

When changed, the attribute change is propagated to all children except for those who the attribute is already defined in the hash table.

But some attributes can be marked as **non-inheritable** at the control class.

**Non-inheritable** attributes of the element are not propagated to its children.
If an attribute is not marked as **non-inheritable** at the element, it is propagated as expected, but if marked as **non-inheritable** at a child, that child will ignore the propagated value.

Since Vbox, Hbox, and other containers have only a few registered attributes; by default, an unknown attribute is treated as inheritable, that's why it will be automatically propagated.

An example: the IMAGE attribute of a Label is **non-inheritable**, so when checked at the Label it will return NULL if not defined, and the Label parent tree will not be consulted.
If you change the IMAGE attribute at a Vbox that contains several Labels, the child Labels will not be affected.

### Availability

Although attributes can be changed and retrieved at any time, there are exceptions and some rules that must be followed according to the documentation of the attribute:

- **read-only**: the attribute cannot be changed. Ignored when set.
- **write-only**: the attribute cannot be retrieved. Normally used for action attributes. Returns NULL, or eventually some value set before the element was mapped.
- **creation-only**: it will be used only when the element is mapped on the native system. So set it before the element is mapped. Ignored when set after the element is mapped.
