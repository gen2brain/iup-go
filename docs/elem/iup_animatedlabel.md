## IupAnimatedLabel

Creates an animated label interface element, which displays an image that is changed periodically.

It uses an animation that is simply an **IupUser** with several **IupImage** as children.

It inherits from [IupLabel](iup_label.md).

### Creation

    Ihandle* IupAnimatedLabel(Ihandle* animation);

**animation**: element that contains the list of images. It can be NULL.

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

All **IupLabel** attributes. The IMAGE attribute is periodically changed by a timer.

Additionally, it defines the following non-inheritable attributes.

> 
>
> ------------------------------------------------------------------------

**START** (write-only): starts the animation. The value is ignored.
By default, the animation is stopped.

**STOP** (write-only): stops the animation. The value is ignored.

**STOPWHENHIDDEN**: automatically stops the animation when the label is hidden.
Default: Yes.

**RUNNING** (read-only): return YES if the animation is running.

**FRAMETIME**: The time between each frame.
If the **IupUser** element has a FRAMETIME attribute, it will be used to set the **IupAnimatedLabel** FRAMETIME attribute, but it can be overwritten later on.

**FRAMECOUNT** (read-only): number of frames in the animation.
It is simply **IupGetChildCount** of the given **IupUser** element.

**ANIMATION**: the name of the element that contains the list of images.
The value passed must be the name of an **IupUser** element with several **IupImage** as children.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate a child to a name.

**ANIMATION_HANDLE**: same as ANIMATION but directly using the Ihandle* of the element.

### Callbacks

All **IupLabel** callbacks. No label callbacks are used internally.

### Examples

     label = IupAnimatedLabel(NULL);
    IupSetAttribute(label, "ANIMATION", "IUP_CircleProgressAnimation");
    IupSetAttribute(label, "START", "Yes");

[Browse for Example Files](../../examples/)

### See Also

[IupLabel](iup_label.md), [IupUser](iup_user.md), [IupImage](iup_image.md).
