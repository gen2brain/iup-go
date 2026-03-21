## VALUE (non-inheritable)

Affects several elements differently - that is, its behavior is element-dependent.
It is often used to change the control's main value, such as the text of a [IupText](../elem/iup_text.md).

For the [IupRadio](../elem/iup_radio.md) and [IupZbox](../elem/iup_zbox.md), elements, which are categorized as composition elements, this attribute represents the element "selected" among the others in the designed composition.
To change this attribute in such cases, different mechanisms are necessary, according to the programming environment used.
When the elements taking part in a composition were created in C, this attribute's contents is a name that must be defined by the [IupSetHandle](../func/iup_sethandle.md) function.
