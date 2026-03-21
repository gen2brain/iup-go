package iup

import (
	"unsafe"
)

/*
#include <stdlib.h>
#include "iup.h"
*/
import "C"

// Append inserts an interface element at the end of the container, after the last element of the container.
// Valid for any element that contains other elements like dialog, frame, hbox, vbox, zbox or menu.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_append.md
func Append(ih, child Ihandle) Ihandle {
	return mkih(C.IupAppend(ih.ptr(), child.ptr()))
}

// Detach detaches an interface element from its parent.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_detach.md
func Detach(child Ihandle) {
	C.IupDetach(child.ptr())
}

// Insert Inserts an interface element before another child of the container.
// Valid for any element that contains other elements like dialog, frame, hbox, vbox, zbox, menu, etc.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_insert.md
func Insert(ih, refChild, child Ihandle) Ihandle {
	return mkih(C.IupInsert(ih.ptr(), refChild.ptr(), child.ptr()))
}

// Reparent moves an interface element from one position in the hierarchy tree to another.
// Both new_parent and child must be mapped or unmapped at the same time.
// If ref_child is NULL, then it will append the child to the new_parent.
// If ref_child is NOT NULL then it will insert child before ref_child inside the new_parent.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_reparent.md
func Reparent(ih, newParent, refChild Ihandle) int {
	return int(C.IupReparent(ih.ptr(), newParent.ptr(), refChild.ptr()))
}

// GetParent returns the parent of a control.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getparent.md
func GetParent(ih Ihandle) Ihandle {
	return mkih(C.IupGetParent(ih.ptr()))
}

// GetChild returns the a child of the control given its position.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getchild.md
func GetChild(ih Ihandle, pos int) Ihandle {
	return mkih(C.IupGetChild(ih.ptr(), C.int(pos)))
}

// GetChildPos returns the position of a child of the given control.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getchildpos.md
func GetChildPos(ih, child Ihandle) int {
	return int(C.IupGetChildPos(ih.ptr(), child.ptr()))
}

// GetChildCount returns the number of children of the given control.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getchildcount.md
func GetChildCount(ih Ihandle) int {
	return int(C.IupGetChildCount(ih.ptr()))
}

// GetNextChild returns the a child of the given control given its brother.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getnextchild.md
func GetNextChild(ih, child Ihandle) Ihandle {
	return mkih(C.IupGetNextChild(ih.ptr(), child.ptr()))
}

// GetBrother returns the brother of an element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getbrother.md
func GetBrother(ih Ihandle) Ihandle {
	return mkih(C.IupGetBrother(ih.ptr()))
}

// GetDialog returns the handle of the dialog that contains that interface element.
// Works also for children of a menu that is associated with a dialog.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getdialog.md
func GetDialog(ih Ihandle) Ihandle {
	return mkih(C.IupGetDialog(ih.ptr()))
}

// GetDialogChild returns the identifier of the child element that has the NAME attribute
// equals to the given value on the same dialog hierarchy.
// Works also for children of a menu that is associated with a dialog.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getdialogchild.md
func GetDialogChild(ih Ihandle, name string) Ihandle {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return mkih(C.IupGetDialogChild(ih.ptr(), cName))
}
