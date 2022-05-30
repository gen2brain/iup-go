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

// Fill creates void element, which dynamically occupies empty spaces always trying to expand itself.
// Its parent should be an Hbox, an Vbox or a GridBox, or else this type of expansion will not work.
// If an EXPAND is set on at least one of the other children of the box, then the fill expansion is ignored.
//
// It does not have a native representation.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupfill.html
func Fill() Ihandle {
	h := mkih(C.IupFill())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Space creates void element, which occupies an empty space.
// It will simply occupy a space in the layout. It does not have a natural size, it is 0x0 by default.
// It can be expandable or not, EXPAND will work as a regular element.
//
// It does not have a native representation.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupspace.html
func Space() Ihandle {
	h := mkih(C.IupSpace())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Cbox creates a void container for position elements in absolute coordinates. It is a concrete layout container.
//
// It does not have a native representation.
//
// The Cbox is equivalent of a Vbox or Hbox where all the children have the FLOATING attribute set to YES,
// but children must use CX and CY attributes instead of the POSITION attribute.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupcbox.html
func Cbox(children ...Ihandle) Ihandle {
	children = append(children, Ihandle(0))

	h := mkih(C.IupCboxv((**C.Ihandle)(unsafe.Pointer(&(children[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// GridBox creates a void container for composing elements in a regular grid.
// It is a box that arranges the elements it contains from top to bottom and from left to right,
// but can distribute the elements in lines or in columns.
//
// It does not have a native representation.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupgridbox.html
func GridBox(children ...Ihandle) Ihandle {
	children = append(children, Ihandle(0))

	h := mkih(C.IupGridBoxv((**C.Ihandle)(unsafe.Pointer(&(children[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// MultiBox creates a void container for composing elements in a irregular grid.
// It is a box that arranges the elements it contains from top to bottom and from left to right,
// by distributing the elements in lines or in columns. But its EXPAND attribute does not behave as a regular container,
// instead it behaves as a regular element expanding into the available space.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupmultibox.html
func MultiBox(children ...Ihandle) Ihandle {
	children = append(children, Ihandle(0))

	h := mkih(C.IupMultiBoxv((**C.Ihandle)(unsafe.Pointer(&(children[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Hbox creates a void container for composing elements horizontally.
// It is a box that arranges the elements it contains from left to right.
//
// It does not have a native representation.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iuphbox.html
func Hbox(children ...Ihandle) Ihandle {
	children = append(children, Ihandle(0))

	h := mkih(C.IupHboxv((**C.Ihandle)(unsafe.Pointer(&(children[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Vbox creates a void container for composing elements vertically.
// It is a box that arranges the elements it contains from top to bottom.
//
// It does not have a native representation.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupvbox.html
func Vbox(children ...Ihandle) Ihandle {
	children = append(children, Ihandle(0))

	h := mkih(C.IupVboxv((**C.Ihandle)(unsafe.Pointer(&(children[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Zbox Creates a void container for composing elements in hidden layers with only one layer visible.
// It is a box that piles up the children it contains, only the one child is visible.
//
// It does not have a native representation.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupzbox.html
func Zbox(children ...Ihandle) Ihandle {
	children = append(children, Ihandle(0))

	h := mkih(C.IupZboxv((**C.Ihandle)(unsafe.Pointer(&(children[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Radio creates a void container for grouping mutual exclusive toggles.
// Only one of its descendent toggles will be active at a time.
// The toggles can be at any composition.
//
// It does not have a native representation.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupradio.html
func Radio(child Ihandle) Ihandle {
	h := mkih(C.IupRadio(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Normalizer creates a void container that does not affect the dialog layout.
// It acts by normalizing all the controls in a list so their natural size becomes the biggest natural size amongst them.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupnormalizer.html
func Normalizer(ihList ...Ihandle) Ihandle {
	ihList = append(ihList, Ihandle(0))

	h := mkih(C.IupNormalizerv((**C.Ihandle)(unsafe.Pointer(&(ihList[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Frame creates a native container, which draws a frame with a title around its child.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupframe.html
func Frame(child Ihandle) Ihandle {
	h := mkih(C.IupFrame(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatFrame creates a native container, which draws a frame with a title around its child.
// The decorations are manually drawn. The control inherits from BackgroundBox.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupflatframe.html
func FlatFrame(child Ihandle) Ihandle {
	h := mkih(C.IupFlatFrame(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Tabs creates a native container for composing elements in hidden layers with only one layer visible (just like Zbox),
// but its visibility can be interactively controlled.
// The interaction is done in a line of tabs with titles and arranged according to the tab type.
// Also known as Notebook in native systems.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iuptabs.html
func Tabs(children ...Ihandle) Ihandle {
	children = append(children, Ihandle(0))

	h := mkih(C.IupTabsv((**C.Ihandle)(unsafe.Pointer(&(children[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatTabs Creates a native container for composing elements in hidden layers with only one layer visible (just like Zbox),
// but its visibility can be interactively controlled.
// The interaction is done in a line of tabs with titles and arranged according to the tab type.
// Also known as Notebook in native systems.
// Identical to the Tabs control but the decorations and buttons are manually drawn.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupflattabs.html
func FlatTabs(children ...Ihandle) Ihandle {
	children = append(children, Ihandle(0))

	h := mkih(C.IupFlatTabsv((**C.Ihandle)(unsafe.Pointer(&(children[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// BackgroundBox creates a simple native container with no decorations.
// Useful for controlling children visibility for Zbox or Expander. It inherits from Canvas.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupbackgroundbox.html
func BackgroundBox(child Ihandle) Ihandle {
	h := mkih(C.IupBackgroundBox(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ScrollBox creates a native container that allows its child to be scrolled. It inherits from Canvas.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupscrollbox.html
func ScrollBox(child Ihandle) Ihandle {
	h := mkih(C.IupScrollBox(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// FlatScrollBox Creates a native container that allows its child to be scrolled. It inherits from IupCanvas.
// The difference from ScrollBox is that its scrollbars are drawn.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupflatscrollbox.html
func FlatScrollBox(child Ihandle) Ihandle {
	h := mkih(C.IupFlatScrollBox(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// DetachBox creates a detachable void container.
//
// Dragging and dropping this element, it creates a new dialog composed by its child
// or elements arranged in it (for example, a child like Vbox or Hbox).
// During the drag, the ESC key can be pressed to cancel the action.
//
// It does not have a native representation, but it contains also a Canvas to implement the bar handler.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupdetachbox.html
func DetachBox(child Ihandle) Ihandle {
	h := mkih(C.IupDetachBox(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Expander creates a void container that can interactively show or hide its child.
//
// It does not have a native representation, but it contains also several elements to implement the bar handler.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupexpander.html
func Expander(child Ihandle) Ihandle {
	h := mkih(C.IupExpander(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Sbox creates a void container that allows its child to be resized.
// Allows expanding and contracting the child size in one direction.
//
// It does not have a native representation but it contains also a Canvas to implement the bar handler.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupsbox.html
func Sbox(child Ihandle) Ihandle {
	h := mkih(C.IupSbox(child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Split creates a void container that split its client area in two.
// Allows the provided controls to be enclosed in a box that allows expanding
// and contracting the element size in one direction, but when one is expanded the other is contracted.
//
// It does not have a native representation, but it contains also a Canvas to implement the bar handler.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/elem/iupsplit.html
func Split(child1, child2 Ihandle) Ihandle {
	h := mkih(C.IupSplit(child1.ptr(), child2.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}
