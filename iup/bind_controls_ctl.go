//go:build ctl

package iup

import (
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
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupcontrols.html
func ControlsOpen() int {
	return int(C.IupControlsOpen())
}

// Cells create a grid widget with custom drawing callbacks.
// It is a canvas-based control that displays a scrollable grid of cells.
// Each cell can be individually drawn via the DRAW_CB callback.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupcells.html
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
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix.html
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
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrixlist.html
func MatrixList() Ihandle {
	h := mkih(C.IupMatrixList())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// MatrixEx creates an extended matrix control with additional features.
// It provides extra functionality on top of the standard Matrix control,
// including undo/redo, copy/paste, sorting, searching, units conversion, etc.
//
// https://www.tecgraf.puc-rio.br/iup/en/ctrl/iupmatrix_extra.html
func MatrixEx() Ihandle {
	h := mkih(C.IupMatrixEx())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}
