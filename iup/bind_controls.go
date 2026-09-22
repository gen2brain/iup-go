//go:build !js

package iup

import (
	"unsafe"
)

/*
#include <stdlib.h>
#include "iup.h"
*/
import "C"

// AnimatedLabel creates an animated label interface element, which displays an image that is changed periodically.
//
// It uses an animation that is simply a User with several Image objects as children.
//
// It inherits from Label.
//
// https://gen2brain.github.io/iup-go/elem/iup_animatedlabel.html
func AnimatedLabel(animation Ihandle) Ihandle {
	h := mkih(C.IupAnimatedLabel(animation.ptr()))
	return h
}

// Button creates an interface element that is a button.
// When selected, this element activates a function in the application.
// Its visual presentation can contain a text and/or an image.
//
// https://gen2brain.github.io/iup-go/elem/iup_button.html
func Button(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupButton(cTitle))
	return h
}

// Calendar creates a month calendar interface element, where the user can select a date.
//
// https://gen2brain.github.io/iup-go/elem/iup_calendar.html
func Calendar() Ihandle {
	h := mkih(C.IupCalendar())
	return h
}

// Canvas creates an interface element that is a canvas - a working area for your application.
//
// https://gen2brain.github.io/iup-go/elem/iup_canvas.html
func Canvas() Ihandle {
	h := mkih(C.IupCanvas())
	return h
}

// ColorBar creates a color palette to enable a color selection from several samples. It can select one or two colors.
// The primary color is selected with the left mouse button, and the secondary color is selected with the right mouse button.
//
// https://gen2brain.github.io/iup-go/elem/iup_colorbar.html
func ColorBar() Ihandle {
	h := mkih(C.IupColorbar())
	return h
}

// ColorBrowser creates an element for selecting a color. The selection is done using a cylindrical projection of the RGB cube.
// The transformation defines a coordinate color system called HSI, that is still the RGB color space but using cylindrical coordinates.
//
// https://gen2brain.github.io/iup-go/elem/iup_colorbrowser.html
func ColorBrowser() Ihandle {
	h := mkih(C.IupColorBrowser())
	return h
}

// DatePick creates a date editing interface element, which can displays a calendar for selecting a date.
//
// https://gen2brain.github.io/iup-go/elem/iup_datepick.html
func DatePick() Ihandle {
	h := mkih(C.IupDatePick())
	return h
}

// Terminal creates a terminal emulator control.
//
// https://gen2brain.github.io/iup-go/elem/iup_terminal.html
func Terminal() Ihandle {
	return mkih(C.IupTerminal())
}

// Dial creates a dial for regulating a given angular variable.
//
// https://gen2brain.github.io/iup-go/elem/iup_dial.html
func Dial(orientation string) Ihandle {
	cOrientation := cStrOrNull(orientation)
	defer cStrFree(cOrientation)

	h := mkih(C.IupDial(cOrientation))
	return h
}

// Label creates a label interface element, which displays a separator, a text or an image.
//
// https://gen2brain.github.io/iup-go/elem/iup_label.html
func Label(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupLabel(cTitle))
	return h
}

// Separator creates a separator interface element. It does not have native decorations.
//
// https://gen2brain.github.io/iup-go/elem/iup_separator.html
func Separator() Ihandle {
	h := mkih(C.IupSeparator())
	return h
}

// Link creates a label that displays an underlined clickable text. It inherits from Label.
//
// https://gen2brain.github.io/iup-go/elem/iup_link.html
func Link(url, title string) Ihandle {
	cUrl := cStrOrNull(url)
	cTitle := cStrOrNull(title)
	defer cStrFree(cUrl)
	defer cStrFree(cTitle)

	h := mkih(C.IupLink(cUrl, cTitle))
	return h
}

// List Creates an interface element that displays a list of items.
// The list can be visible or can be dropped down. It also can have an edit box for text input. So it is a 4 in 1 element.
// In native systems, the dropped-down case is called Combo Box.
//
// https://gen2brain.github.io/iup-go/elem/iup_list.html
func List() Ihandle {
	h := mkih(C.IupList())
	return h
}

// ProgressBar creates a progress bar control. Shows a percent value that can be updated to simulate a progression.
//
// It is similar of Gauge, but uses native controls internally. Also does not have support for text inside the bar.
//
// https://gen2brain.github.io/iup-go/elem/iup_progressbar.html
func ProgressBar() Ihandle {
	h := mkih(C.IupProgressBar())
	return h
}

// Spin creates a control set with a vertical box containing two buttons, one with an up arrow and the other with a down arrow,
// to be used to increment and decrement values.
//
// https://gen2brain.github.io/iup-go/elem/iup_spin.html
func Spin() Ihandle {
	h := mkih(C.IupSpin())
	return h
}

// SpinBox creates a horizontal container that already contains a Spin.
//
// https://gen2brain.github.io/iup-go/elem/iup_spin.html
func SpinBox(child Ihandle) Ihandle {
	h := mkih(C.IupSpinbox(child.ptr()))
	return h
}

// Text creates an editable text field.
//
// https://gen2brain.github.io/iup-go/elem/iup_text.html
func Text() Ihandle {
	h := mkih(C.IupText())
	return h
}

// TextConvertLinColToPos converts a (lin, col) character positioning into an absolute position.
// lin and col start at 1, pos starts at 0. For single line controls pos is always "col-1".
//
// https://gen2brain.github.io/iup-go/elem/iup_text.html
func TextConvertLinColToPos(ih Ihandle, lin, col int) (pos int) {
	C.IupTextConvertLinColToPos(ih.ptr(), C.int(lin), C.int(col), (*C.int)(unsafe.Pointer(&pos)))
	return
}

// TextConvertPosToLinCol Converts an absolute position into a (lin, col) character positioning.
// lin and col start at 1, pos starts at 0. For single line controls lin is always 1, and col is always "pos+1".
//
// https://gen2brain.github.io/iup-go/elem/iup_text.html
func TextConvertPosToLinCol(ih Ihandle, pos int) (lin, col int) {
	C.IupTextConvertPosToLinCol(ih.ptr(), C.int(pos), (*C.int)(unsafe.Pointer(&lin)), (*C.int)(unsafe.Pointer(&col)))
	return
}

// MultiLine creates an editable field with one or more lines.
//
// Text has support for multiple lines when the MULTILINE attribute is set to YES.
// Now when a Multiline element is created, in fact, a Text element with MULTILINE=YES is created.
//
// https://gen2brain.github.io/iup-go/elem/iup_multiline.html
func MultiLine() Ihandle {
	h := mkih(C.IupMultiLine())
	return h
}

// Toggle creates the toggle interface element.
// It is a two-state (on/off) button that, when selected, generates an action that activates a function in the associated application.
// Its visual representation can contain a text or an image.
//
// https://gen2brain.github.io/iup-go/elem/iup_toggle.html
func Toggle(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupToggle(cTitle))
	return h
}

// Tree creates a tree containing nodes of branches or leaves. Both branches and leaves can have an associated text and image.
//
// The branches can be expanded or collapsed. When a branch is expanded,
// its immediate children are visible, and when it is collapsed they are hidden.
//
// The leaves can generate an "executed" or "renamed" actions, branches can only generate a "renamed" action.
//
// The focus node is the node with the focus rectangle, marked nodes have their background inverted.
//
// https://gen2brain.github.io/iup-go/elem/iup_tree.html
func Tree() Ihandle {
	h := mkih(C.IupTree())
	return h
}

// Table creates a table with multiple columns and rows for displaying tabular data.
// Uses native table widgets on each platform.
//
// https://gen2brain.github.io/iup-go/elem/iup_table.html
func Table() Ihandle {
	h := mkih(C.IupTable())
	return h
}

// TreeSetAttributeHandle .
//
// https://gen2brain.github.io/iup-go/elem/iup_tree.html
func TreeSetAttributeHandle(ih Ihandle, name string, id int, ihNamed Ihandle) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupTreeSetAttributeHandle(ih.ptr(), cName, C.int(id), ihNamed.ptr())
}

// TreeSetUserId associates a user data pointer with a given tree node id.
// The userid is typically an Ihandle value (e.g. from User()), or a
// cgo.NewHandle value for associating arbitrary Go data with a node.
//
// Returns 1 on success, 0 on failure (invalid id).
//
// https://gen2brain.github.io/iup-go/elem/iup_tree.html
func TreeSetUserId(ih Ihandle, id int, userid uintptr) int {
	return int(C.IupTreeSetUserId(ih.ptr(), C.int(id), unsafe.Pointer(cih(Ihandle(userid)))))
}

// TreeGetUserId returns the user data pointer associated with a given tree node id.
// Returns 0 if the id is invalid or no user data is set.
//
// The returned value can be cast back to an Ihandle or cgo.Handle depending
// on what was stored with TreeSetUserId.
//
// https://gen2brain.github.io/iup-go/elem/iup_tree.html
func TreeGetUserId(ih Ihandle, id int) uintptr {
	return uintptr(unsafe.Pointer(C.IupTreeGetUserId(ih.ptr(), C.int(id))))
}

// TreeGetId returns the node id associated with a given user data pointer.
// Returns -1 if the user data is not found.
//
// https://gen2brain.github.io/iup-go/elem/iup_tree.html
func TreeGetId(ih Ihandle, userid uintptr) int {
	return int(C.IupTreeGetId(ih.ptr(), unsafe.Pointer(cih(Ihandle(userid)))))
}

// Val creates a Valuator control. Selects a value in a limited interval.
// Also known as Scale or Trackbar in native systems.
//
// https://gen2brain.github.io/iup-go/elem/iup_val.html
func Val(_type string) Ihandle {
	cType := cStrOrNull(_type)
	defer cStrFree(cType)

	h := mkih(C.IupVal(cType))
	return h
}

// Scrollbar creates a standalone Scrollbar control with a proportional thumb.
//
// https://gen2brain.github.io/iup-go/elem/iup_scrollbar.html
func Scrollbar(orientation string) Ihandle {
	cOrientation := cStrOrNull(orientation)
	defer cStrFree(cOrientation)

	h := mkih(C.IupScrollbar(cOrientation))
	return h
}

// Popover creates a floating container anchored to another element.
//
// https://gen2brain.github.io/iup-go/elem/iup_popover.html
func Popover(child Ihandle) Ihandle {
	h := mkih(C.IupPopover(child.ptr()))
	return h
}
