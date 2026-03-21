## THEME/NTHEME

Applies a set of attributes to a control.
The THEME attribute is inheritable and the NTHEME attribute is NOT inheritable.

### Value

Name of an **IupUser** element that contains the attributes.
The name is associated in C using [IupSetHandle](../func/iup_sethandle.md).
The name association must be done before setting the attribute. 

### Notes

All attributes in the theme must be strings.

Only attributes that are registered in the element will receive its theme value.

Attributes that are registered as not being strings, read-only, write-only or callbacks will NOT be applied.

The theme can contain a specialized subtheme for the element class.
The element class name will be used with a "IUP" prefix to identify the subtheme.
For instance, if the element is a label, then an attribute called "IUPLABEL" can point to another theme name to be applied at the element additionally to the already applied attributes.

The global attribute [DEFAULTTHEME](iup_globals.md) can be applied to all elements during creation.

### Affects

All controls.

### See Also

[IupUser](../elem/iup_user.md)
