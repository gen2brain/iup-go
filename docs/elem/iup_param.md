## IupParam

Construction element used only in [IupParamBox](iup_parambox.md).
It is not mapped in the native system, but it will exist while its **IupParamBox** container exists.

### Creation

    Ihandle* IupParam(const char* format);

**format**: string that describes the parameter. See Notes below.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**LABEL** [read-only]: returns an IUP Ihandle*, the label associated with the parameter.
Valid only after the **IupParamBox** is created.

**CONTROL** [read-only]: returns an IUP Ihandle*, the real control associated with the parameter.
Valid only after the **IupParamBox** is created.

**AUXCONTROL** [read-only]: returns an IUP Ihandle*, the auxiliary control associated with the parameter (for instance Valuators).
Valid only after the **IupParamBox** is created.

**INDEX** [read-only]: returns an integer value associated with the parameter index.
**IupGetInt** can also be used. Valid only after the **IupParamBox** is created.

**VALUE** - the value of the parameter. **IupGetFloat** and **IupGetInt** can also be used.
For the current parameter inside the callback contains the new value that will be applied to the control, to get the old value use the VALUE attribute for the CONTROL returned Ihandle*.

------------------------------------------------------------------------

#### Attributes set during creation, obtained from the format string

**TITLE**: text of the parameter, used as label. For all parameters.

**INDENT**: number of indentation levels.
For all parameters when '\t' is used inside the title area.

**TYPE**: can be BOOLEAN ('b'), LIST ('l'), OPTIONS ('o'), REAL ('A', 'a', 'R', 'r'), STRING ('m', 's'), INTEGER ('i'), DATE ('d'), FILE ('f'), COLOR ('c'), SEPARATOR ('t'), BUTTONNAMES ('u'), PARAMBOX ('x') and HANDLE ('h').
And describe the type of the parameter. For all parameters.

**DATATYPE**: can be INT [int] ('b', 'l', 'o', 'i'), FLOAT [float] ('a', 'r'), DOUBLE [double] ('A', 'R'), STRING [char*] ('m', 's', 'd', 'f', 'n', 'c'), HANDLE [Ihandle*] ('h') or NONE ('u', 't', 'x').
And describe the C data type that must be passed to **IupGetParam** to initialize and receive parameter values.
For all parameters.

**MULTILINE**: can be Yes or No. Defines if the edit box can have more than one line.
For 'm' parameter.

**MAXSTR**: maximum size for the string.
Its default value is 10240 for multiline strings, 4096 for file names, and 512 for other strings.
For 'm', 's', 'd', 'f', 'n' and 'c' parameters.

**ANGLE**: can be Yes or No. defines if the REAL type is an angle. For 'a' or 'A' parameters.

**TRUE**, **FALSE**: boolean names. For 'b' parameter.

**INTERVAL** (Yes/No), **MIN**, **MAX**, **STEP**, **PARTIAL** (Yes/No): optional limits for **integer** and **real** types.
For 'a', 'A', 'i', 'R', and 'r' parameters.

**PRECISION**: numeric precision for real value display. For 'a', 'A', 'r', and 'R' parameters.

**VISIBLECOLUMNS**: number of visible columns for horizontally expandable text elements.
If set the control will not expand anymore. For 's' parameter.

**DIALOGTYPE**, **FILTER**, **DIRECTORY**, **NOCHANGEDIR**, **NOOVERWRITEPROMPT**: used for the FILE parameter dialog.
See [IupFileDlg](../dlg/iup_filedlg.md). For 'f' parameter.

**BUTTON1**, **BUTTON2**, **BUTTON3**: button titles.
Default is "OK/Cancel/Help" for regular **IupGetParam**, and "Apply/Reset/Help" when **IupParamBox** is directly used.
For 'u' parameter.

**0, 1, 2, 3, ...** : list items. For 'l' and 'o' parameters.

**MASK**: mask for the edit box input. For 's' and 'm' parameters.

**TIP**: text of the tip. For all parameters.

**NOFRAME**: do not include the **IupFrame** around the parameter list.
For 'x' parameter.

### Notes

The format string must have the following format, notice the "\n" at the end

"***text***%***x*[*extra*]{tip}**\n", where:

***text*** is a descriptive text, to be placed to the left of the entry field in a label.
It can contain any string, but to contain a '%' must use two characters "%%" to avoid conflict with the type separator.
If it is preceded by n '\t' characters then the parameter will be indented by the same number.

***x*** is the type of the parameter. The valid options are:

**b** = boolean (shows a True/False toggle, use "int" in C)**\
i** = integer (shows a integer number filtered text box, use "int" in C)**\
r** = real (shows a real number filtered text box, use "float" in C)**\
R** = same as **r** but using "double" in C**\
a** = angle in degrees (shows a real number filtered text box and a dial [if **IupControlsOpen** were called], use "float" in C)\
**A** = same as **a** but using "double" in C**\
s** = string (shows a text box, use "char*" in C, it must have room enough for your string)\
**m** = multiline string (shows a multiline text box, use "char*" in C, it must have room enough for your string)\
**l** = list (shows a dropdown list box, use "int" in C for the zero based item index selected)\
**o** = list (shows a list of toggles inside a radio, use "int" in C for the zero based item index selected)\
**t** = separator (shows a horizontal line separator label, in this case text can be an empty string, not included in parameter count)\
**d** = string, but the interface uses a [IupDatePick](iup_datepick.md) element to select a date\
**f** = string (same as **s**, but also show a button to open a file selection dialog box)\
**c** = string (same as **s**, but also show a color button to open a color selection dialog box)**\
n** = string (same as **s**, but also show a font button to open a font selection dialog box)**\
h** = Ihandle* (a control handle that will be managed by the application, it will be placed after the parameters and before the buttons.)**\
x** = attributes for the **IupParamBox** in the extra options.\
**u** = buttons titles (allow to redefine the default button titles (OK and Cancel), and to add a third button, use [button1,button2,button3] as extra data, can omit one of them, it will use the default name, not included in parameter count)

***extra*** is one or more additional options for the given type

**[min,max,step]** are optional limits for **integer** and **real** types.
The **max** and **step** values can be omitted.
When **min** and **max** are specified a valuator will also be added to change the value.
To specify **step**, **max** must be also specified. **step** is the size of the increment.\
**[false,true]** are optional strings for **boolean** types to be displayed after the toggle.
The strings cannot have commas '**,**', nor brackets '**[**' or '**]**'.**\
mask** is an optional mask for the **string** and **multiline** types.
The dialog uses the [MASK](../attrib/iup_mask.md) attribute internally.
In this case we do no use the brackets '**[**' and '**]**' to avoid conflict with the specified mask.\
**\|item0\|item1\|item2,...\|** are the items of the **list**. At least one item must exist.
Again the brackets are not used to increase the possibilities for the strings, instead you must use '**\|**'.
Items index are zero based start.\
**[dialogtype\|filter\|directory\|nochangedir\|nooverwriteprompt]** are the respective attribute values passed to the [IupFileDlg](../dlg/iup_filedlg.md) control when activated.
All '**\|**' must exist, but you can let empty values to use the default values. No mask can be set.

**tip** is a string that is displayed in a TIP for the main control of the parameter. 

Since the **tip** string cannot contain a '\n' because of the param terminator, the '\r' character can be used to break lines in the TIP.
It will be internally converted to '\n' before actually setting the TIP.

A integer parameter always has a spin attached to the text to increment and decrement the value.
A real parameter only has a spin if a full interval is defined (min and max), in this case, the default step is (max-min)/100.
When the callback is called because a spin was activated, then the attribute "**SPINNING**" of the element will be defined to a non-NULL and non-zero value.

The default precision for real value display is given by the global attribute [DEFAULTPRECISION](../attrib/iup_globals.md).
But inside the callback, the application can set the param attribute "**PRECISION**" to use another value.
It will work only during interactive changes.
The decimal symbol will use the [DEFAULTDECIMALSYMBOL](../attrib/iup_globals.md) global attribute.

There are no extra parameters for the color string.
The mask is automatically set to capture 3 or 4 unsigned integers from 0 to 255 (R G B) or (R G B A) (alpha is optional).

The date extra parameters are simply [IupDatePick](iup_datepick.md) attributes in a single string for **IupSetAttributes** usage.

When the "s" type is used, the size can be controlled using the VISIBLECOLUMNS attribute at the param element.

### See Also

[IupGetParam](../dlg/iup_getparam.md), [IupParamBox](iup_parambox.md)
