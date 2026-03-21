## EXPAND (non-inheritable)

Allows the element to expand, fulfilling empty spaces inside its container.

It is a non-inheritable attribute, but a container inherits its parents EXPAND attribute.
In other words, although EXPAND is non-inheritable, it is inheritable for containers.
So if you set it at a container it will not affect its children, except for those who are containers.

The expansion is done equally for all expandable elements in the same container.

For a container, the actual EXPAND value will always be a **combination** of its own value and the value of its **children**.
In the sense that a container can only expand if its children can expand too in the same direction.

The HORIZONTALFREE and VERTICALFREE values will not behave as normal expansion.
These values will NOT affect the expansion of the container when set at its children, the children will simply expand to the available free space at the container.

See the [Layout Guide](../layout.md) for more details on sizes.

### Value

"YES" (both directions), "HORIZONTAL", "VERTICAL", "HORIZONTALFREE", "VERTICALFREE" or "NO".

Default: "NO". For containers, the default is "YES".

### Affects

All elements, except menus.
