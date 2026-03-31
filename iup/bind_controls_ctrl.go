//go:build ctrl

package iup

import (
	"unsafe"

	"github.com/google/uuid"
)

/*
#include <stdlib.h>
#include "iup.h"
#include "iupcontrols.h"
*/
import "C"

// ControlsOpen initializes the additional controls library (Cells, Matrix, MatrixList, MatrixEx).
// Must be called after IupOpen.
//
// Returns: IUP_NOERROR on success, IUP_OPENED if already opened, IUP_ERROR on failure.
func ControlsOpen() int {
	return int(C.IupControlsOpen())
}

// Cells create a grid widget with custom drawing callbacks.
// It is a canvas-based control that displays a scrollable grid of cells.
// Each cell can be individually drawn via the DRAW_CB callback.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_cells.md
func Cells() Ihandle {
	h := mkih(C.IupCells())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Matrix creates a matrix control for displaying and editing tabular data.
// The matrix is a grid of cells that can contain text, dropdowns, toggles, colors, images, and fill indicators.
//
// The action parameter is optional and can be "" or a callback name.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_matrix.md
func Matrix(action string) Ihandle {
	cAction := cStrOrNull(action)
	defer cStrFree(cAction)

	h := mkih(C.IupMatrix(cAction))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// MatrixList creates a matrix-based list control.
// It is a specialized version of the matrix control designed for displaying lists with columns.
// Supports label, color, and image columns.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_matrixlist.md
func MatrixList() Ihandle {
	h := mkih(C.IupMatrixList())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// MatrixEx creates an extended matrix control with additional features.
// It provides extra functionality on top of the standard Matrix control,
// including undo/redo, copy/paste, sorting, searching, units conversion, etc.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_matrixex.md
func MatrixEx() Ihandle {
	h := mkih(C.IupMatrixEx())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatButton creates an interface element that is a button, but it does not have native decorations.
// When selected, this element activates a function in the application.
// Its visual presentation can contain a text and/or an image.
//
// It behaves just like a Button, but since it is not a native control, it has more flexibility for additional options.
// It can also behave like a Toggle (without the checkmark).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_flatbutton.md
func FlatButton(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupFlatButton(cTitle))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatLabel creates an interface element that is a label, but it does not have native decorations.
// Its visual presentation can contain a text and/or an image.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_flatlabel.md
func FlatLabel(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupFlatLabel(cTitle))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatToggle creates an interface element that is a toggle, but it does not have native decorations.
// When selected, this element activates a function in the application. Its visual presentation can contain a text and/or an image.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_flattoggle.md
func FlatToggle(title string) Ihandle {
	cTitle := cStrOrNull(title)
	defer cStrFree(cTitle)

	h := mkih(C.IupFlatToggle(cTitle))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatList creates an interface element that displays a list of items, but it does not have native decorations.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_flatlist.md
func FlatList() Ihandle {
	h := mkih(C.IupFlatList())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatTree creates a tree containing nodes of branches or leaves. Both branches and leaves can have an associated text and image.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_flattree.md
func FlatTree() Ihandle {
	h := mkih(C.IupFlatTree())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatVal creates a Valuator control, but it does not have native decorations.
// Selects a value in a limited interval. Also known as Scale or Trackbar in native systems.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_flatval.md
func FlatVal(orientation string) Ihandle {
	cOrientation := cStrOrNull(orientation)
	defer cStrFree(cOrientation)

	h := mkih(C.IupFlatVal(cOrientation))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatFrame creates a native container, which draws a frame with a title around its child.
// The decorations are manually drawn. The control inherits from BackgroundBox.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_flatframe.md
func FlatFrame(child Ihandle) Ihandle {
	h := mkih(C.IupFlatFrame(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatTabs creates a native container for composing elements in hidden layers with only one layer visible (just like Zbox),
// but its visibility can be interactively controlled.
// The interaction is done in a line of tabs with titles and arranged according to the tab type.
// Also known as Notebook in native systems.
// Identical to the Tabs control but the decorations and buttons are manually drawn.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_flattabs.md
func FlatTabs(children ...Ihandle) Ihandle {
	children = append(children, Ihandle(0))

	h := mkih(C.IupFlatTabsv((**C.Ihandle)(unsafe.Pointer(&(children[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatScrollBox creates a native container that allows its child to be scrolled. It inherits from IupCanvas.
// The difference from ScrollBox is that its scrollbars are drawn.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/ctrl/iup_flatscrollbox.md
func FlatScrollBox(child Ihandle) Ihandle {
	h := mkih(C.IupFlatScrollBox(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}
