### IupFlatTree Callbacks

**SELECTION_CB**: Action generated when a node is selected or deselected.
This action occurs when the user clicks with the mouse or uses the keyboard with the appropriate combination of keys.
It may be called more than once for the same node with the same status.

    int function(Ihandle *ih, int id, int status)

**ih**: identifier of the element that activated the event.\
**id**: Node identifier.\
**status**: 1=node selected, 0=node unselected.

**MULTISELECTION_CB**: Action generated after a continuous range of nodes is selected in one single operation.
If not defined the SELECTION_CB with status=1 will be called for all nodes in the range.
The range is always completely included, independent if some nodes were already marked.
That single operation also guaranties that all other nodes outside the range are already not selected.
Called only if MARKMODE=MULTIPLE.

    int function(Ihandle *ih, int* ids, int n)

**ih**: identifier of the element that activated the event.\
**ids**: Array of node identifiers. This array is kept for backward compatibility, the range is simply defined by ids[0] to ids[n-1], where `ids[i+1]=ids[i]+1`.\
**n**: Number of nodes in the array.

**MULTIUNSELECTION_CB**: Action generated before multiple nodes are unselected in one single operation.
If not defined the SELECTION_CB with status=0 will be called for all nodes in the range.
The range is not necessarily continuous. Called only if MARKMODE=MULTIPLE.

    int function(Ihandle *ih, int* ids, int n)

**ih**: identifier of the element that activated the event.\
**ids**: Array of node identifiers.\
**n**: Number of nodes in the array.

------------------------------------------------------------------------

**BRANCHOPEN_CB**: Action generated when a branch is expanded.
This action occurs when the user clicks the "+" sign on the left of the branch, or when double clicks the branch, or hits Enter on a collapsed branch.

    int function(Ihandle *ih, int id) 

**ih**: identifier of the element that activated the event.\
**id**: node identifier.

**Returns:** IUP_IGNORE for the branch not to be opened, or IUP_DEFAULT for the branch to be opened.

**BRANCHCLOSE_CB**: Action generated when a branch is collapsed.
This action occurs when the user clicks the "-" sign on the left of the branch, or when double clicks the branch, or hits Enter on an expanded branch.

    int function(Ihandle *ih, int id);

**ih**: identifier of the element that activated the event.\
**id**: node identifier.

**Returns:** IUP_IGNORE for the branch not to be closed, or IUP_DEFAULT for the branch to be closed.

**EXECUTELEAF_CB**: Action generated when a leaf is executed.
This action occurs when the user double clicks a leaf, or hits Enter on a leaf.

    int function(Ihandle *ih, int id); 

**ih**: identifier of the element that activated the event.\
**id**: node identifier. \

**EXECUTEBRANCH_CB**: Action generated when a branch is executed.
This action occurs when the user double clicks a branch, or hits Enter on a branch.
Is is called before the BRANCH*_CB callbacks.

    int function(Ihandle *ih, int id); 

**ih**: identifier of the element that activated the event.\
**id**: node identifier. 

------------------------------------------------------------------------

**SHOWRENAME_CB**: Action generated when a node is about to be renamed.
It occurs when the user clicks twice the node or press **F2**. Called only if SHOWRENAME=YES.

    int function(Ihandle *ih, int id);

**ih**: identifier of the element that activated the event.\
**id**: node identifier. 

**Returns:** if IUP_IGNORE is returned, the rename is canceled.\

**RENAME_CB**: Action generated after a node was renamed in place.
It occurs when the user press **Enter** after editing the name, or when the text box loses it focus.
Called only if SHOWRENAME=YES.

    int function(Ihandle *ih, int id, char *title);

**ih**: identifier of the element that activated the event.\
**id**: node identifier.\
**title**: new node title.

**Returns:** The new title is accepted only if the callback returns IUP_DEFAULT.
If the callback does not exists the new title is always accepted.
If the user pressed **Enter** and the callback returns IUP_IGNORE the editing continues.
If the text box loses its focus the editing stops always.\

------------------------------------------------------------------------

**DRAGDROP_CB**: Action generated when an internal drag & drop is executed.
Only active if **SHOWDRAGDROP=YES.**

    int function(Ihandle *ih, int drag_id, int drop_id, int isshift, int iscontrol); 

**ih**: identifier of the element that activated the event.\
**drag_id**: Identifier of the clicked node where the drag start.\
**drop_id**: Identifier of the clicked node where the drop were executed. -1 indicates a drop in a blank area.\
**isshift**: flag indicating the shift key state.\
**iscontrol**: flag indicating the control key state.

**Returns:** if returns IUP_CONTINUE, or if the callback is not defined and **SHOWDRAGDROP=YES**, then the node is moved to the new position.
If Ctrl is pressed then the node is copied instead of moved.
If the drop node is a branch and it is expanded, then the drag node is inserted as the first child of the node.
If the branch is not expanded or the node is a leaf, then the drag node is inserted as the next brother of the drop node.\

**NODEREMOVED_CB**: Action generated when a node is going to be removed.
It is only a notification, the action can not be aborted.
No node dependent attribute can be consulted during the callback.
It is useful to remove memory allocated for the userdata.

    int function(Ihandle *ih, void* userdata); 

**ih**: identifier of the element that activated the event.\
**userdata/userid**: USERDATA attribute.

**RIGHTCLICK_CB**: Action generated when the right mouse button is pressed over a node.

    int function(Ihandle *ih, int id); 

**ih**: identifier of the element that activated the event.\
**id**: node identifier.

------------------------------------------------------------------------

**TOGGLEVALUE_CB**: Action generated when the toggle's state was changed.
The callback also receives the new toggle's state.

    int function(Ihandle *ih, int id, int state);

**ih**: identifier of the element that activated the event.\
**id**: node identifier.\
**state**: 1 if the toggle's state was shifted to ON; 0 if it was shifted to OFF.
If SHOW3STATE=YES, −1 if it was shifted to NOTDEF.

------------------------------------------------------------------------

FLAT_[BUTTON_CB](../call/iup_button_cb.md): Action generated when any mouse button is pressed or released inside the element.
Use [IupConvertXYToPos](../func/iup_convertxytopos.md) to convert (x,y) coordinates in the node identifier.

FLAT_[MOTION_CB](../call/iup_motion_cb.md): Action generated when the mouse is moved over the element.
Use [IupConvertXYToPos](../func/iup_convertxytopos.md) to convert (x,y) coordinates in the node identifier.

[DROPFILES_CB](../call/iup_dropfiles_cb.md): Action generated when one or more files are dropped in the element.

------------------------------------------------------------------------

[MAP_CB](../call/iup_map_cb.md), [UNMAP_CB](../call/iup_unmap_cb.md), [DESTROY_CB](../call/iup_destroy_cb.md), [GETFOCUS_CB](../call/iup_getfocus_cb.md), [KILLFOCUS_CB](../call/iup_killfocus_cb.md), [ENTERWINDOW_CB](../call/iup_enterwindow_cb.md), [LEAVEWINDOW_CB](../call/iup_leavewindow_cb.md), [K_ANY](../call/iup_k_any.md), [HELP_CB](../call/iup_help_cb.md): All common callbacks are supported.

[Drag & Drop](../attrib/iup_dragdrop.md) callbacks are supported, but SHOWDRAGDROP must be set to NO. 
