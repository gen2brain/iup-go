package iup

import (
	"unsafe"

	"github.com/google/uuid"
)

/*
#include <stdlib.h>
#include "iup.h"
*/
import "C"

// AnimatedLabel creates an animated label interface element, which displays an image that is changed periodically.
//
// It uses an animation that is simply an User with several Image as children.
//
// It inherits from Label.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupanimatedlabel.html
func AnimatedLabel(animation Ihandle) Ihandle {
	h := mkih(C.IupAnimatedLabel(animation.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Button creates an interface element that is a button.
// When selected, this element activates a function in the application.
// Its visual presentation can contain a text and/or an image.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupbutton.html
func Button(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupButton(cTitle, nil))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatButton creates an interface element that is a button, but it does not have native decorations.
// When selected, this element activates a function in the application.
// Its visual presentation can contain a text and/or an image.
//
// It behaves just like an Button, but since it is not a native control it has more flexibility for additional options.
// It can also behave like an Toggle (without the checkmark).
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupflatbutton.html
func FlatButton(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupFlatButton(cTitle))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// DropButton creates an interface element that is a button with a drop down arrow.
// It can function as a button and as a dropdown. Its visual presentation can contain a text and/or an image.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupdropbutton.html
func DropButton(dropchild Ihandle) Ihandle {
	h := mkih(C.IupDropButton(dropchild.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Calendar creates a month calendar interface element, where the user can select a date.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupcalendar.html
func Calendar() Ihandle {
	h := mkih(C.IupCalendar())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Canvas creates an interface element that is a canvas - a working area for your application.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupcanvas.html
func Canvas() Ihandle {
	h := mkih(C.IupCanvas(nil))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ColorBar creates a color palette to enable a color selection from several samples. It can select one or two colors.
// The primary color is selected with the left mouse button, and the secondary color is selected with the right mouse button.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupcolorbar.html
func ColorBar() Ihandle {
	h := mkih(C.IupColorbar())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ColorBrowser creates an element for selecting a color. The selection is done using a cylindrical projection of the RGB cube.
// The transformation defines a coordinate color system called HSI, that is still the RGB color space but using cylindrical coordinates.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupcolorbrowser.html
func ColorBrowser() Ihandle {
	h := mkih(C.IupColorBrowser())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// DatePick creates a date editing interface element, which can displays a calendar for selecting a date.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupdatepick.html
func DatePick() Ihandle {
	h := mkih(C.IupDatePick())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Dial creates a dial for regulating a given angular variable.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupdial.html
func Dial(orientation string) Ihandle {
	cOrientation := cStrOrNull(orientation)
	defer cStrFree(cOrientation)

	h := mkih(C.IupDial(cOrientation))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Gauge creates a Gauge control. Shows a percent value that can be updated to simulate a progression.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupgauge.html
func Gauge() Ihandle {
	h := mkih(C.IupGauge())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Label creates a label interface element, which displays a separator, a text or an image.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iuplabel.html
func Label(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupLabel(cTitle))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatLabel creates an interface element that is a label, but it does not have native decorations.
// Its visual presentation can contain a text and/or an image.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupflatlabel.html
func FlatLabel(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupFlatLabel(cTitle))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatSeparator creates an interface element that is a Separator, but it does not have native decorations.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupflatseparator.html
func FlatSeparator() Ihandle {
	h := mkih(C.IupFlatSeparator())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Link creates a label that displays an underlined clickable text. It inherits from Label.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iuplink.html
func Link(url, title string) Ihandle {
	cUrl := cStrOrNull(title)
	cTitle := cStrOrNull(title)
	defer cStrFree(cUrl)
	defer cStrFree(cTitle)

	h := mkih(C.IupLink(cUrl, cTitle))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// List Creates an interface element that displays a list of items.
// The list can be visible or can be dropped down. It also can have an edit box for text input. So it is a 4 in 1 element.
// In native systems the dropped down case is called Combo Box.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iuplist.html
func List() Ihandle {
	h := mkih(C.IupList(nil))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatList creates an interface element that displays a list of items, but it does not have native decorations.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupflatlist.html
func FlatList() Ihandle {
	h := mkih(C.IupFlatList())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ProgressBar creates a progress bar control. Shows a percent value that can be updated to simulate a progression.
//
// It is similar of Gauge, but uses native controls internally. Also does not have support for text inside the bar.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupprogressbar.html
func ProgressBar() Ihandle {
	h := mkih(C.IupProgressBar())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Spin creates a control set with a vertical box containing two buttons, one with an up arrow and the other with a down arrow,
// to be used to increment and decrement values.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupspin.html
func Spin() Ihandle {
	h := mkih(C.IupSpin())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// SpinBox creates a horizontal container that already contains a Spin.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupspin.html
func SpinBox(child Ihandle) Ihandle {
	h := mkih(C.IupSpinbox(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Text creates an editable text field.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iuptext.html
func Text() Ihandle {
	h := mkih(C.IupText(nil))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// TextConvertLinColToPos converts a (lin, col) character positioning into an absolute position.
// lin and col starts at 1, pos starts at 0. For single line controls pos is always "col-1".
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iuptext.html
func TextConvertLinColToPos(ih Ihandle, lin, col int) (pos int) {
	C.IupTextConvertLinColToPos(ih.ptr(), C.int(lin), C.int(col), (*C.int)(unsafe.Pointer(&pos)))
	return
}

// TextConvertPosToLinCol Converts an absolute position into a (lin, col) character positioning.
// lin and col starts at 1, pos starts at 0. For single line controls lin is always 1, and col is always "pos+1".
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iuptext.html
func TextConvertPosToLinCol(ih Ihandle, pos int) (lin, col int) {
	C.IupTextConvertPosToLinCol(ih.ptr(), C.int(pos), (*C.int)(unsafe.Pointer(&lin)), (*C.int)(unsafe.Pointer(&col)))
	return
}

// MultiLine creates an editable field with one or more lines.
//
// Text has support for multiple lines when the MULTILINE attribute is set to YES.
// Now when a Multiline element is created in fact a Text element with MULTILINE=YES is created.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupmultiline.html
func MultiLine() Ihandle {
	h := mkih(C.IupMultiLine(nil))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Toggle creates the toggle interface element.
// It is a two-state (on/off) button that, when selected, generates an action that activates a function in the associated application.
// Its visual representation can contain a text or an image.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iuptoggle.html
func Toggle(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupToggle(cTitle, nil))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatToggle creates an interface element that is a toggle, but it does not have native decorations.
// When selected, this element activates a function in the application. Its visual presentation can contain a text and/or an image.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupflattoggle.html
func FlatToggle(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupFlatToggle(cTitle))
	h.SetAttribute("UUID", uuid.NewString())
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
// https://www.tecgraf.puc-rio.br/iup/en/elem/iuptree.html
func Tree() Ihandle {
	h := mkih(C.IupTree())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatTree creates a tree containing nodes of branches or leaves. Both branches and leaves can have an associated text and image.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupflattree.html
func FlatTree() Ihandle {
	h := mkih(C.IupFlatTree())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// TreeSetAttributeHandle .
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iuptree.html
func TreeSetAttributeHandle(ih Ihandle, name string, id int, ihNamed Ihandle) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupTreeSetAttributeHandle(ih.ptr(), cName, C.int(id), ihNamed.ptr())
}

// Val creates a Valuator control. Selects a value in a limited interval.
// Also known as Scale or Trackbar in native systems.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupval.html
func Val(_type string) Ihandle {
	cType := cStrOrNull(_type)
	defer cStrFree(cType)

	h := mkih(C.IupVal(cType))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatVal creates a Valuator control, but it does not have native decorations.
// Selects a value in a limited interval. Also known as Scale or Trackbar in native systems.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupflatval.html
func FlatVal(orientation string) Ihandle {
	cOrientation := cStrOrNull(orientation)
	defer cStrFree(cOrientation)

	h := mkih(C.IupFlatVal(cOrientation))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}
