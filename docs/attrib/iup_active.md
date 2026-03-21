## ACTIVE

Activates or inhibits user interaction.

### Value

"YES" (active), "NO" (inactive).

Default: "YES"

### Notes

An interface element is only active if its native parent is also active.

ACTIVE can also be set for controls that do not have user interaction because they may have a visual feedback to indicate the inactive state.

In GTK and Motif, the inactive dialogs will still be able to move, resize and change their Z-order.
Although their contents will be inactive, keyboard will be disabled, and they can not be closed from the close box.

### Affects

All controls that have visual representation.
