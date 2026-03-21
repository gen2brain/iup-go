## ACTION

Action generated when the element is activated. Affects each element differently.

### Callback

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

In some elements, this callback may receive more parameters, apart from **ih**.
Please refer to each element's documentation.

### Affects

[IupButton](../elem/iup_button.md), [IupItem](../elem/iup_item.md), [IupList](../elem/iup_list.md), [IupText](../elem/iup_text.md), [IupCanvas](../elem/iup_canvas.md), [IupMultiline](../elem/iup_multiline.md), [IupToggle](../elem/iup_toggle.md)
