## IupShow

Displays a dialog in the current position, or changes a control VISIBLE attribute.

### Parameters/Return

    int IupShow(Ihandle *ih);

**ih**: identifier of the interface element.

**Returns:** IUP_NOERROR if successful. If there was an error returns IUP_ERROR.

### Notes

For dialogs, it is equivalent to call **IupShowXY** using IUP_CURRENT.
See [IupShowXY](iup_showxy.md) for more details.

For other controls, to call **IupShow** is the same as setting VISIBLE=YES.

### See Also

[IupShowXY](iup_showxy.md), [IupHide](iup_hide.md), [IupPopup](iup_popup.md), [IupMap](iup_map.md)
