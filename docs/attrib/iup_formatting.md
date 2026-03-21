## FORMATTING (non-inheritable)

When enabled, allows the use of text formatting attributes.
In GTK is always enabled, but only when MULTILINE=YES.
Default: NO.

Supported in Windows (Win32 and WinUI), GTK 3, GTK 4, Qt, macOS and EFL.
Not supported in Motif.

### Value

Can be: YES or NO. Default: NO.

### Affects

[IupText](../elem/iup_text.md)

### Auxiliary Attributes

#### ADDFORMATTAG [write-only] (non-inheritable)

Name of a format tag element to be added to the **IupText**.
The name is associated in C using [IupSetHandle](../func/iup_sethandle.md).
The name association must be done before setting the attribute.
It will set the ADDFORMATTAG_HANDLE with the associated handle.

#### ADDFORMATTAG_HANDLE [write-only] (non-inheritable)

Handle of a format tag element to be added to the **IupText**.
The tag element will be automatically destroyed when the **IupText** is mapped.
If the **IupText** is already mapped, the format tag is immediately destroyed when the attribute is set.
The format tag cannot be reused.

#### REMOVEFORMATTING [write-only] (non-inheritable)

Removes the formatting of the current selection if Yes or NULL, and from all text if ALL is used.

### Format Tag

The format tag element is a simple [IupUser](../elem/iup_user.md) element with some known attributes that will be interpreted when the tag is updated in the native system.

The formatting depends on the existing text, so if VALUE attribute is set, all formatting is lost.
You must set it again for the new text.

If the FONT attribute of the **IupText** is set, then it will affect the format of all characters in the text.

The default values cannot be dynamically changed.

#### General Format Tag Attributes

**BULK**: flag that means this tag is composed by several tags as its children.
The bulk tag itself does not have formatting. Used to optimize format tag modifications.
Default: NO.

**CLEANOUT**: when BULK=Yes is used to clear all the formatting at start. Default: NO.

**SELECTION/SELECTIONPOS**: same as the **IupText** [SELECTION/SELECTIONPOS](../elem/iup_text.md) attributes.
If not defined the **IupText** attribute will be used.
If the **IupText** attribute is also not defined then the current position will receive the format, so new text inserted or typed will be formatted with the tag.
Different tags that use the same selection interval are combined.
Setting these attributes here will not change the current setting in **IupText**.

**UNITS** [Windows Only]: By default, all distance units are integers in pixels, but in Windows you can also specify integer units in TWIPs (one twip is 1/1440 of an inch).
Can be TWIP or PIXELS. Default: PIXELS.

#### Paragraph Format Tag Attributes

**ALIGNMENT**: Can be JUSTIFY, RIGHT, CENTER and LEFT. Default: LEFT.

**INDENT**: paragraph indentation, the distance between the margin and the paragraph.
In Windows the right indentation, and the indentation of the second and subsequent lines (relative to the indentation of the first line) can be independently set using the **INDENTRIGHT** and **INDENTOFFSET** attributes, but only when **INDENT** is set.

**LINESPACING**: the distance between lines of the same paragraph.
In Windows, the values SINGLE, ONEHALF and DOUBLE can be used.

**NUMBERING**: Can be BULLET (bullet symbol), ARABIC (arabic numbers - 1,2,3...), LCLETTER (lower case letters - a,b,c...), UCLETTER (upper case letters - A,B,C...), LCROMAN (lower case Roman numerals - i,ii,iii...), UCROMAN (upper case Roman numerals - I,II,III...) and NONE.
Default: NONE.

**NUMBERINGSTYLE**: Can be RIGHTPARENTHESIS "a)", PARENTHESES "(a)", PERIOD "a.", NONUMBER (it will skip the numbering or bullet for the item) and NONE "".
Default: NONE.

**NUMBERINGTAB**: Minimum distance from a paragraph numbering or bullet to the paragraph text.

**SPACEAFTER**: distance left empty above the paragraph.

**SPACEBEFORE**: distance left empty below the paragraph.

**TABSARRAY**: a sequence of tab positions and alignment up to 32 tabs.
It uses the format:"pos align pos align pos align...".
Position is the distance relative to the left margin and alignment can be LEFT, CENTER, RIGHT and DECIMAL.
When DECIMAL alignment is used, the text is aligned according to a decimal point or period in the text, it is normally used to align numbers.

#### Character Format Tag Attributes

**BGCOLOR**: string containing a color in the format "rrr ggg bbb" for the background of the text.

**DISABLED** [Windows and macOS Only]: Can be YES or NO. Default NO. Set the visual appearance to disabled.

**FGCOLOR**: string containing a color in the format "rrr ggg bbb" for the text.

**FONTSCALE**: a size scale relative to the selected or current size. Values greater than 1 will increase the font.
Values smaller than 1 will shrink the font. Default: 1.0.
The following values are also accepted: "XX-SMALL" (0.58), "X-SMALL" (0.64), "SMALL" (0.83), "MEDIUM" (1.0), "LARGE" (1.2), "X-LARGE" (1.44), "XX-LARGE" (1.73).

**FONTFACE**: the face name of the font.

**FONTSIZE**: the size of the font in pixels or points. Pixel size uses negative values.

**ITALIC**: Can be YES or NO. Default NO.

**LANGUAGE** [GTK and EFL Only]: A text with a description of the text language.
The same value can be used in the "SYSTEMLANGUAGE" global attribute.

**RISE**: the distance, positive or negative from the base line.
Can also use the values SUPERSCRIPT and SUBSCRIPT, but this values will also reduce the size of the font.

**SMALLCAPS**: Can be YES or NO. Default NO. (Does not work always, depends on the font)

**PROTECTED**: Can be YES or NO. Default NO. When set to YES the selected text cannot be edited.

**STRETCH** [GTK and EFL Only]: Can be EXTRA_CONDENSED, CONDENSED, SEMI_CONDENSED, NORMAL, SEMI_EXPANDED, EXPANDED and EXTRA_EXPANDED.
Default NORMAL. (Does not work always, depends on the font)

**STRIKEOUT**: Can be YES or NO. Default NO.

**UNDERLINE**: Can be SINGLE, DOUBLE, DOTTED or NONE. Default NONE.

**WEIGHT**: Can be EXTRALIGHT, LIGHT, NORMAL, SEMIBOLD, BOLD, EXTRABOLD and HEAVY. Default: NORMAL.

### Examples

    Ihandle* formattag;
    IupSetAttribute(text, "FORMATTING", "YES");

    formattag = IupUser();
    IupSetAttribute(formattag, "ALIGNMENT", "CENTER");
    IupSetAttribute(formattag, "SPACEAFTER", "10");
    IupSetAttribute(formattag, "FONTSIZE", "24");
    IupSetAttribute(formattag, "SELECTION", "3,1:3,50");
    IupSetAttribute(text, "ADDFORMATTAG_HANDLE", (char*)formattag);

    formattag = IupUser();
    IupSetAttribute(formattag, "BGCOLOR", "255 128 64");
    IupSetAttribute(formattag, "UNDERLINE", "SINGLE");
    IupSetAttribute(formattag, "WEIGHT", "BOLD");
    IupSetAttribute(formattag, "SELECTION", "3,7:3,11");
    IupSetAttribute(text, "ADDFORMATTAG_HANDLE", (char*)formattag);

    formattag = IupUser();
    IupSetAttribute(formattag, "ITALIC", "YES");
    IupSetAttribute(formattag, "STRIKEOUT", "YES");
    IupSetAttribute(formattag, "SELECTION", "2,1:2,12");
    IupSetAttribute(text, "ADDFORMATTAG_HANDLE", (char*)formattag);
