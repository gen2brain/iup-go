## HELP_CB

Action generated when the user presses F1 at a control.
In Motif, it is also activated by the Help key in some workstation keyboards.

### Callback

    int function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

**Returns**: IUP_CLOSE will be processed.

### Affects

All elements with user interaction.
