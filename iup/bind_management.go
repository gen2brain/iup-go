package iup

import (
	"unsafe"
)

/*
#include <stdlib.h>
#include <iup.h>
*/
import "C"

// Map creates (maps) the native interface objects corresponding to the given IUP interface elements.
//
// It will also called recursively to create the native element of all the children in the element's tree.
//
// The element must be already attached to a mapped container, except the dialog.
// A child can only be mapped if its parent is already mapped.
//
// This function is automatically called before the dialog is shown in Show, ShowXY or Popup.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupmap.html
func Map(ih Ihandle) int {
	return int(C.IupMap(ih.ptr()))
}

// Unmap unmap the element from the native system. It will also unmap all its children.
// It will NOT detach the element from its parent, and it will NOT destroy the IUP element.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupunmap.html
func Unmap(ih Ihandle) {
	C.IupUnmap(ih.ptr())
}

// Create creates an interface element given its class name and parameters.
// This function is called from all constructors like Dialog(...), Label(...), and so on.
//
// After creation the element still needs to be attached to a container and mapped to the native system so it can be visible.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupcreate.html
func Create(className string) Ihandle {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	return mkih(C.IupCreate(cClassName))
}

// Destroy destroys an interface element and all its children.
// Only dialogs, timers, popup menus and images should be normally destroyed, but detached controls can also be destroyed.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupdestroy.html
func Destroy(ih Ihandle) {
	C.IupDestroy(ih.ptr())
}

// GetAllClasses returns the names of all registered classes.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupgetallclasses.html
func GetAllClasses() (names []string) {
	n := int(C.IupGetAllClasses(nil, 0))
	if n > 0 {
		names = make([]string, n)
		pNames := make([]*C.char, n)
		C.IupGetAllClasses((**C.char)(unsafe.Pointer(&pNames[0])), C.int(n))
		for i := 0; i < n; i++ {
			names[i] = C.GoString(pNames[i])
		}
	}
	return
}

// GetClassName returns the name of the class of an interface element.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupgetclassname.html
func GetClassName(ih Ihandle) string {
	return C.GoString(C.IupGetClassName(ih.ptr()))
}

// GetClassType returns the name of the native type of an interface element.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupgetclasstype.html
func GetClassType(ih Ihandle) string {
	return C.GoString(C.IupGetClassType(ih.ptr()))
}

// ClassMatch checks if the give class name matches the class name of the given interface element.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupclassmatch.html
func ClassMatch(ih Ihandle, className string) bool {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	return C.IupClassMatch(ih.ptr(), cClassName) != C.int(0)
}

// GetClassAttributes returns the names of all registered attributes of a class.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupgetclassattributes.html
func GetClassAttributes(className string) (names []string) {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	n := int(C.IupGetClassAttributes(cClassName, nil, 0))
	if n > 0 {
		names = make([]string, n)
		pNames := make([]*C.char, n)
		C.IupGetClassAttributes(cClassName, (**C.char)(unsafe.Pointer(&pNames[0])), C.int(n))
		for i := 0; i < n; i++ {
			names[i] = C.GoString(pNames[i])
		}
	}
	return
}

// GetClassCallbacks returns the names of all registered callbacks of a class.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupgetclasscallbacks.html
func GetClassCallbacks(className string) (names []string) {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	n := int(C.IupGetClassCallbacks(cClassName, nil, 0))
	if n > 0 {
		names = make([]string, n)
		pNames := make([]*C.char, n)
		C.IupGetClassCallbacks(cClassName, (**C.char)(unsafe.Pointer(&pNames[0])), C.int(n))
		for i := 0; i < n; i++ {
			names[i] = C.GoString(pNames[i])
		}
	}
	return
}

// SaveClassAttributes saves all registered attributes on the internal hash table.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupsaveclasscallbacks.html
func SaveClassAttributes(ih Ihandle) {
	C.IupSaveClassAttributes(ih.ptr())
}

// CopyClassAttributes copies all registered attributes from one element to another.
// Both elements must be of the same class.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupcopyclassattributes.html
func CopyClassAttributes(srcIh, dstIh Ihandle) {
	C.IupCopyClassAttributes(srcIh.ptr(), dstIh.ptr())
}

// SetClassDefaultAttribute changes the default value of an attribute for a class.
// It can be any attribute, i.e. registered attributes or user custom attributes.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupsetclassdefaultattribute.html
func SetClassDefaultAttribute(className, name, value string) {
	cClassName, cName, cValue := C.CString(className), C.CString(name), C.CString(value)
	defer C.free(unsafe.Pointer(cClassName))
	defer C.free(unsafe.Pointer(cName))
	defer C.free(unsafe.Pointer(cValue))

	C.IupSetClassDefaultAttribute(cClassName, cName, cValue)
}

// Update mark the element or its children to be redraw when the control returns to the system.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupupdate.html
func Update(ih Ihandle) {
	C.IupUpdate(ih.ptr())
}

// UpdateChildren mark the element or its children to be redraw when the control returns to the system.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupupdate.html
func UpdateChildren(ih Ihandle) {
	C.IupUpdateChildren(ih.ptr())
}

// Redraw force the element and its children to be redraw immediately.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupredraw.html
func Redraw(ih Ihandle, children int) {
	C.IupRedraw(ih.ptr(), C.int(children))
}

// ConvertXYToPos converts a (x,y) coordinate in an item position.
//
// It can be used for Text and Scintilla (returns a position in the string), List (returns an item),
// Tree (returns a node identifier) or Matrix (returns a cell position, where pos=lin*numcol + col).
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iupconvertxytopos.html
func ConvertXYToPos(ih Ihandle, x, y int) int {
	return int(C.IupConvertXYToPos(ih.ptr(), C.int(x), C.int(y)))
}

// Refresh updates the size and layout of all controls in the same dialog.
// To be used after changing size attributes, or attributes that affect the size of the control.
// Can be used for any element inside a dialog, but the layout of the dialog and all controls will be updated.
// It can change the layout of all the controls inside the dialog because of the dynamic layout positioning.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iuprefresh.html
func Refresh(ih Ihandle) {
	C.IupRefresh(ih.ptr())
}

// RefreshChildren updates the size and layout of controls after changing size attributes,
// or attributes that affect the size of the control. Can be used for any element inside a dialog,
// only its children will be updated. It can change the layout of all the controls inside
// the given element because of the dynamic layout positioning.
//
// http://webserver2.tecgraf.puc-rio.br/iup/en/func/iuprefreshchildren.html
func RefreshChildren(ih Ihandle) {
	C.IupRefreshChildren(ih.ptr())
}
