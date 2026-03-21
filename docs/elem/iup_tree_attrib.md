## IupTree Attributes

### General

**AUTOREDRAW** [Windows] (non-inheritable): automatically redraws the tree when something has changed.
Set to NO to change many nodes without updating the display. Default: "YES".

[BGCOLOR](../attrib/iup_bgcolor.md): Background color of the tree.
Default: the global attribute TXTBGCOLOR.

[EXPAND](../attrib/iup_expand.md) (non-inheritable): The default value is "YES".

**FGCOLOR**: default text foreground color.
Once each node is created it will not change its color when FGCOLOR is changed.
Default: the global attribute TXTFGCOLOR.

**HIDEBUTTONS** (creation-only): hide the expand and collapse buttons.
In GTK, branches will be only expanded programmatically.

**HIDELINES** (creation-only): hide the lines that connect the nodes in the hierarchy. (GTK 2.10)

**HLCOLOR** [Windows and Motif Only] (non-inheritable): the background color of the selected node.
Default: TXTHLCOLOR global attribute.

**INDENTATION**: sets the indentation level in pixels.
The visual effect of changing the indentation is highly system-dependent.
In GTK it acts as an additional indent value, and the lines do not follow the extra indent.
In Windows is limited to a minimum of 5 pixels.  (GTK 2.12)

**INFOTIP** [Windows Only]: the TIP is shown every time a node is highlighted.
This is the default behavior for TIPs in native tree controls in Windows, if set to No then it will use the regular TIP behavior.
Default: Yes.

[RASTERSIZE](../attrib/iup_rastersize.md) (non-inheritable): the initial size is "400x200".
Set to NULL to allow the automatic layout use smaller values.

**SCROLLVISIBLE** (read-only) [Windows Only]: Returns which scrollbars are visible at the moment.
Can be: YES (both), VERTICAL, HORIZONTAL, NO.

**SPACING**: vertical internal padding for each node.
Notice that the distance between each node will be actually 2x the spacing.

**CSPACING**: same as SPACING but using the units of the vertical part of the **SIZE** attribute.
It will actually set the SPACING attribute.

**TOPITEM** (write-only): position the given node identifier at the top of the tree or near to make it visible.
If any parent node is collapsed then they are automatically expanded.

> 
>
> ------------------------------------------------------------------------

[ACTIVE](../attrib/iup_active.md), [EXPAND](../attrib/iup_expand.md), [FONT](../attrib/iup_font.md), [SCREENPOSITION](../attrib/iup_screenposition.md), [POSITION](../attrib/iup_position.md), [MINSIZE](../attrib/iup_minsize.md), [MAXSIZE](../attrib/iup_maxsize.md), [WID](../attrib/iup_wid.md), [TIP](../attrib/iup_tip.md), [SIZE](../attrib/iup_size.md), [RASTERSIZE](../attrib/iup_rastersize.md), [ZORDER](../attrib/iup_zorder.md), [VISIBLE](../attrib/iup_visible.md), [THEME](../attrib/iup_theme.md): also accepted. 

### Expanders  (non-inheritable)

**HIDEBUTTONS** (creation-only): hide the expand and collapse buttons.
In GTK, branches will be only expanded programmatically.

**HIDELINES** (creation-only): hide the lines that connect the nodes in the hierarchy. (GTK 2.10)

### Nodes  (non-inheritable)

For these attributes, "id" is the specified node identifier.
If "id" is empty or invalid, then the focus node is used as the specified node.

**COUNT** (read-only) (non-inheritable): returns the total number of nodes in the tree.

**CHILDCOUNT*id*** (read-only): returns the immediate children count of the specified branch.
It does not count children of child that are branches.

**TOTALCHILDCOUNT*id*** (read-only): returns the total children count of the specified branch.
It counts all grandchildren too.

**ROOTCOUNT** (read-only): returns the number of root nodes.

**COLORid**: text foreground color of the specified node.
The value should be a string in the format "R G B" where R, G, B are numbers from 0 to 255.

**DEPTHid** (read-only): returns the depth of the specified node.
The first node has depth=0, its immediate children have depth=1, their children have depth=2 and so on.

**KINDid** (read-only): returns the kind of the specified node. Possible values:

> - "LEAF": The node is a leaf
> - "BRANCH": The node is a branch

**PARENT*id*** (read-only): returns the parent of the specified node.

**NEXT*id*** (read-only): returns the next brother (same depth) of the specified node.
Returns NULLs if at last child node of the parent (at the same depth).

**PREVIOUS*id*** (read-only): returns the previous brother (same depth) of the specified node.
Returns NULLs if at first child node of the parent (at the same depth). 

**LAST*id*** (read-only): returns the last brother (same depth) of the specified node.

**FIRST*id*** (read-only): returns the first brother (same depth) of the specified node.
This is the same as getting the first child of the parent of the given node.
If the specified node is the first child returns the specified node. 

**STATEid**: the state of the specified branch. Returns NULL for a LEAF.
In Windows, it will be effective only if the branch has children.
In GTK, it will be effective only if the parent is expanded. Possible values:

> - "EXPANDED": Expanded branch state (shows its children)
> - "COLLAPSED": Collapsed branch state (hides its children)

**TITLEid**: the text label of the specified node.

**TITLEFONTid**: the text font of the specified node.
The format is the same as the [FONT](../attrib/iup_font.md) attribute.

**TITLEFONTSTYLEid**: changes the font style of the specified node.
Actually changes the **TITLEFONTid** attribute.

**TITLEFONTSIZEid**: changes the font size of the specified node.
Actually changes the **TITLEFONTid** attribute.

**USERDATAid**: the user data associated with the specified node.

### Toggle (non-inheritable)

**SHOWTOGGLE** (creation-only) (non-inheritable): enables the use of toggles for all nodes of the tree.
Can be "YES", "3STATE" or NO". Default: "NO".

**EMPTYAS3STATE** (non-inheritable) [Windows Only]: when SHOWTOGGLE=Yes, the empty space left in nodes that TOGGLEVISIBLEid=NO is filled with the image of the 3state toggle.
Can be Yes or NO. Default: No.

**TOGGLEVALUEid** (non-inheritable): defines the toggle state. Values can be "ON" or "OFF".
If SHOW3STATE=YES then can also be "NOTDEF". Default: "OFF".

**TOGGLEVISIBLEid** (non-inheritable): defines the toggle visible state. Values can be "Yes" or "No".
Default: "Yes".

### Images (non-inheritable)

**IMAGEid** (write-only): image name to be used in the specified node, where id is the specified node identifier.
Use [IupSetHandle](../func/iup_sethandle.md) or [IupSetAttributeHandle](../func/iup_setattributehandle.md) to associate an image to a name.
See also [IupImage](iup_image.md). In Windows and Motif set the BGCOLOR attribute before setting the image.
If node is a branch it is used when collapsed. In Windows all images must have the same size.
In other systems only expanded and collpased images must have the same size.

**IMAGEEXPANDEDid** (write-only): same as the IMAGE attribute but used for expanded branches.

**IMAGELEAF**: the image name that will be shown for all leaves. Default: "IMGLEAF" (a bullet).
Internal values "IMGBLANK" (blank sheet of paper) and "IMGPAPER" (written sheet of paper) are also available.
If BGCOLOR is set the image is automatically updated.
This image defines the available space for the image in all nodes. "IMGEMPTY" can be used as a totally transparent image.

**IMAGEBRANCHCOLLAPSED**: the image name that will be shown for all collapsed branches.
Default: "IMGCOLLAPSED" (a closed folder). If BGCOLOR is set the image is automatically updated.

**IMAGEBRANCHEXPANDED**: the image name that will be shown for all expanded branches.
Default: "IMGEXPANDED" (an open folder). If BGCOLOR is set the image is automatically updated.

### Focus

**VALUE** (non-inheritable): The focus node identifier.
When retrieved but there isn't a node with focus it returns 0 if there are any nodes, and returns -1 if there are no nodes.
When changed and MARKMODE=SINGLE the node is also selected. The tree is always scrolled so the node becomes visible.
In Motif the tree will also receive the focus. Additionally, accepts the values:

> **"ROOT" or "FIRST": the first node (which is always expanded)**\
> "LAST": the last **expanded** node\
> "NEXT": the next **expanded** node, one node after the focus node. If at the last does nothing\
> "PREVIOUS": the previous **expanded** node, one node before the focus node. If at the first does nothing\
> "PGDN": the next **expanded** node, ten nodes node after the focus node. If at the last does nothing\
> "PGUP": the previous **expanded** node, ten nodes before the focus node. If at the first does nothing\
> "CLEAR": clears the selection of the focus node.

**CANFOCUS** (creation-only) (non-inheritable): enables the focus traversal of the control.
In Windows the control will still get the focus when clicked. Default: YES.

**PROPAGATEFOCUS**(non-inheritable): enables the focus callback forwarding to the next native parent with FOCUS_CB defined.
Default: NO.

### Marks

**MARK** (write-only) (non-inheritable): Selects a range of nodes in the format "start-end" (%d-%d).
Allowed only when MARKMODE=MULTIPLE. Also accepts the values:

> **"INVERTid": Inverts the specified node selected state, where id is the specified node identifier. If id is empty or invalid, then the focus node is used as reference node. **\
> "BLOCK": Selects all nodes between the focus node and the initial block-marking node defined by MARKSTART\
> "CLEARALL": Clear the selection of all nodes\
> "MARKALL": Selects all nodes\
> "INVERTALL": Inverts the selection of all nodes

**MARKEDid** (non-inheritable): The selection state of the specified node, where id is the specified node identifier.
If id is empty or invalid, then the focus node is used as reference node. Can be: YES or NO.
Default: NO

**MARKEDNODES** (non-inheritable): The selection state of all nodes.
It is/accepts a sequence of '+' and '-' symbols indicating the state of each node ('+'=selected, '-'=unselected.
When setting this value, if the number of specified symbols is smaller than the total count, then the remaining nodes will not be changed.
Can be set only when MARKMODE=MULTIPLE, can also be get when MARKMODE=SINGLE.

**MARKMODE**: defines how the nodes can be selected. Can be: SINGLE or MULTIPLE. Default: SINGLE.

**MARKSTART** (non-inheritable): Defines the initial node for the block marking, used when MARK=BLOCK.
The value must be the node identifier. Default: 0 (first node).

**MARKWHENTOGGLE** (non-inheritable) [GTK and Windows Only]: selects or clears the selection of a node when its toggle is changed.
Works only if the node has a toggle. Default: No.

### Hierarchy  (non-inheritable)

For these attributes "id" is the specified node identifier.
If "id" is empty or invalid, then the focus node is used as the specified node.

**ADDEXPANDED** (non-inheritable): Defines if branches will be expanded when created.
The branch will be actually expanded when it receives the first child.
Possible values: "YES" = The branches will be created expanded; "NO" = The branches will be created collapsed.
Default: "YES".

**ADDROOT** (non-inheritable): automatically adds an empty branch as the first node when the tree is mapped.
But this does not prevent the tree to have more nodes at the first depth.
Default: "YES".

**ADDLEAFid** (write-only): Adds a new leaf after the reference node, where id is the reference node identifier.
Use id=-1 to add before the first node. The value is used as the text label of the new node.
The id of the new node will be the id of the reference node + 1. The attribute **LASTADDNODE** is set to the new id.
The reference node is marked and all others unmarked. The reference node position remains the same.
If the reference node does not exist, nothing happens.
If the reference node is a branch, then the depth of the new node is one depth increment from the depth of the reference node, if the reference node is a leaf then the new node has the same depth.
If you need to add a node after a specified node but at a different depth use **INSERTLEAF**.
Ignored if set before map.

**ADDBRANCHid** (write-only): Same as **ADDLEAF** for branches.
Branches can be created expanded or collapsed depending on **ADDEXPANDED**.
Ignored if set before map.

**COPYNODEid** (write-only): Copies a node and its children, where id is the specified node identifier.
The value is the destination node identifier.
If the destination node is a branch, and it is expanded, then the specified node is inserted as the first child of the destination node.
If the branch is not expanded or the destination node is a leaf, then it is inserted as the next brother of the leaf.
The specified node is not changed. All node attributes are copied, except user data.
Ignored if set before map.

**DELNODEid** (write-only): Removes a node and/or its children, where id is the specified node identifier.
Ignored if set before map. Possible values:

> - "ALL": deletes all nodes, id is ignored. Notice that this will also deletes the root node.
> - "SELECTED": deletes the specified node and its children
> - "CHILDREN": deletes only the children of the specified node
> - "MARKED": deletes all the selected nodes (and all their children), id is ignored

**EXPANDALL** (write-only): expand or contracts all nodes.
Can be YES (expand all), or NO (contract all).

**INSERTLEAFid**, **INSERTBRANCHid** (write-only): Same as **ADDLEAF** and **ADDBRANCH** but the depth of the new node is always the same of the reference node.
If the reference node is a leaf, then the id of the new node will be the id of the reference node + 1.
If the reference node is a branch the id of the new node will be the id of the reference node + 1 + the total number of child nodes of the reference node.

**MOVENODEid** (write-only): Moves a node and its children, where id is the specified node identifier.
The value is the destination node identifier.
If the destination node is a branch, and it is expanded, then the specified node is inserted as the first child of the destination node.
If the branch is not expanded or the destination node is a leaf, then it is inserted as the next brother of the leaf.
The specified node is removed. User data and all node attributes are preserved.
Ignored if set before map.

### Editing

**RENAME** (write-only): Forces a rename action to take place. Valid only when SHOWRENAME=YES.

RENAMECARET (write-only): the caret’s position of the text box when in-place renaming.
Same as the CARET attribute for [IupText](iup_text.md), but here is used only once after SHOWRENAME_CB is called and before the text box is shown.

RENAMESELECTION (write-only): the selection interval of the text box when in-place renaming.
Same as the SELECTION attribute for [IupText](iup_text.md), but here is used only once after SHOWRENAME_CB is called and before the text box is shown.

**SHOWRENAME** (creation in Windows) (non-inheritable): Allows the in place rename of a node.
Default: "NO". F2 and clicking twice only starts to rename a node if SHOWRENAME=Yes.
In Windows must be set to YES before map, but can be changed later.

### Drag&Drop

**SHOWDRAGDROP** (creation-only) (non-inheritable): Enables the internal drag and drop of nodes, and enables the **DRAGDROP_CB** callback.
Default: "NO". Works only if MARKMODE=SINGLE.
[Drag & Drop](../attrib/iup_dragdrop.md) attributes are NOT used.

**DRAGDROPTREE** (non-inheritable): prepare the [Drag & Drop](../attrib/iup_dragdrop.md) callbacks to support drag and drop of nodes between trees (IupTree only), in the same IUP application.
[Drag & Drop](../attrib/iup_dragdrop.md) attributes still need to be set in order to activate the drag & drop support, so the application can control if this tree is a source and/or target.
Default: NO.

**DROPFILESTARGET** [Windows and GTK Only] (non-inheritable): Enable or disable the drop of files.
Default: NO, but if DROPFILES_CB is defined when the element is mapped then it will be automatically enabled.
This is NOT related to the drag&drop of nodes inside the tree.

**DROPEQUALDRAG** (non-inheritable): if enabled will allow a drop node to be equal to the drag node.
Used only if SHOWDRAGDROP =Yes. In the case the nodes are equal the callback return value is ignored, and nothing is done after.

[Drag & Drop](../attrib/iup_dragdrop.md) attributes are supported, but SHOWDRAGDROP must be set to No.  
