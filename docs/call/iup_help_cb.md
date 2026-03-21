## HELP_CB

Action generated when the user press F1 at a control.
In Motif is also activated by the Help button in some workstations keyboard.

### Callback

    void function(Ihandle *ih);

**ih**: identifier of the element that activated the event.

**Returns**: IUP_CLOSE will be processed.

### Affects

All elements with user interaction.
