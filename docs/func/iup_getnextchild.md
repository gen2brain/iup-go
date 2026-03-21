## IupGetNextChild

Returns a child of the given control given its brother.

### Parameters/Return

    Ihandle *IupGetNextChild(Ihandle* ih, Ihandle* child);

**ih**: identifier of the interface element. Can be NULL if child not NULL.\
**child**: Identifier of the child brother to be used as reference. To get the first child use NULL.

**Returns:** the handle of the child or NULL.

### Notes

This function will return the children of the control in the exact same order in which they were assigned.
If child in not NULL then it returns exactly the same result as [IupGetBrother](iup_getbrother.md).

### Example

    /* Lists all children of a IupVbox */

    #include <stdio.h>
    #include "iup.h"

    int main(int argc, char* argv[])
    {
      Ihandle *dialog, *bt, *lb, *vbox, *child;

      IupOpen(&argc, &argv);

      bt = IupButton("Button", NULL);
      lb = IupLabel("Label");

      vbox = IupVbox(bt, lb, NULL);

      dialog = IupDialog(vbox);
      IupShow(dialog);

      child = IupGetNextChild(vbox, NULL);

      while(child)
      {
        printf("vbox has a child of type %s\n", IupGetClassName(child));
        child = IupGetNextChild(NULL, child);
      }

      IupMainLoop();
      IupClose();

      return 0;
    }

### See Also

[IupGetBrother](iup_getbrother.md), [IupGetParent](iup_getparent.md), [IupGetChild](iup_getchild.md)
