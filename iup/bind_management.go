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

// Map creates (maps) the native interface objects corresponding to the given IUP interface elements.
//
// It will also called recursively to create the native element of all the children in the element's tree.
//
// The element must be already attached to a mapped container, except the dialog.
// A child can only be mapped if its parent is already mapped.
//
// This function is automatically called before the dialog is shown in Show, ShowXY or Popup.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_map.md
func Map(ih Ihandle) int {
	return int(C.IupMap(ih.ptr()))
}

// Unmap unmap the element from the native system. It will also unmap all its children.
// It will NOT detach the element from its parent, and it will NOT destroy the IUP element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_unmap.md
func Unmap(ih Ihandle) {
	C.IupUnmap(ih.ptr())
}

// Create creates an interface element given its class name and parameters.
// This function is called from all constructors like Dialog(...), Label(...), and so on.
//
// After creation the element still needs to be attached to a container and mapped to the native system so it can be visible.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_create.md
func Create(className string) Ihandle {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	return mkih(C.IupCreate(cClassName))
}

// Destroy destroys an interface element and all its children.
// Only dialogs, timers, popup menus and images should be normally destroyed, but detached controls can also be destroyed.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_destroy.md
func Destroy(ih Ihandle) {
	C.IupDestroy(ih.ptr())
}

// GetAllClasses returns the names of all registered classes.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getallclasses.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getclassname.md
func GetClassName(ih Ihandle) string {
	return C.GoString(C.IupGetClassName(ih.ptr()))
}

// GetClassType returns the name of the native type of an interface element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getclasstype.md
func GetClassType(ih Ihandle) string {
	return C.GoString(C.IupGetClassType(ih.ptr()))
}

// ClassMatch checks if the give class name matches the class name of the given interface element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_classmatch.md
func ClassMatch(ih Ihandle, className string) bool {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	return C.IupClassMatch(ih.ptr(), cClassName) != C.int(0)
}

// GetClassAttributes returns the names of all registered attributes of a class.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getclassattributes.md
func GetClassAttributes(className string) (names []string) {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	// The probing call counts both attributes and callbacks (shared table).
	// The filling call returns the real attribute count.
	max := int(C.IupGetClassAttributes(cClassName, nil, 0))
	if max <= 0 {
		return
	}
	pNames := make([]*C.char, max)
	n := int(C.IupGetClassAttributes(cClassName, (**C.char)(unsafe.Pointer(&pNames[0])), C.int(max)))
	names = make([]string, n)
	for i := 0; i < n; i++ {
		names[i] = C.GoString(pNames[i])
	}
	return
}

// GetClassCallbacks returns the names of all registered callbacks of a class.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getclasscallbacks.md
func GetClassCallbacks(className string) (names []string) {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	max := int(C.IupGetClassCallbacks(cClassName, nil, 0))
	if max <= 0 {
		return
	}
	pNames := make([]*C.char, max)
	n := int(C.IupGetClassCallbacks(cClassName, (**C.char)(unsafe.Pointer(&pNames[0])), C.int(max)))
	names = make([]string, n)
	for i := 0; i < n; i++ {
		names[i] = C.GoString(pNames[i])
	}
	return
}

// Attribute flag bits returned by [GetClassAttributeInfo].
const (
	AttribDefault        = 0
	AttribNoInherit      = 1
	AttribNoDefaultValue = 2
	AttribNoString       = 4
	AttribNotMapped      = 8
	AttribHasID          = 16
	AttribReadOnly       = 32
	AttribWriteOnly      = 64
	AttribHasID2         = 128
	AttribCallback       = 256
	AttribNoSave         = 512
	AttribNotSupported   = 1024
	AttribIhandleName    = 2048
	AttribIhandle        = 4096
)

// ClassAttributeInfo is the metadata returned by [GetClassAttributeInfo].
type ClassAttributeInfo struct {
	DefaultValue  string
	SystemDefault string
	Flags         int
}

// GetClassAttributeInfo returns the default value, system default and flag bits
// of a registered attribute. ok is false when the class or attribute is not
// registered.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getclassattributeinfo.md
func GetClassAttributeInfo(className, name string) (info ClassAttributeInfo, ok bool) {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	var def, sysDef *C.char
	var flags C.int

	if C.IupGetClassAttributeInfo(cClassName, cName, &def, &sysDef, &flags) != 1 {
		return
	}

	info.DefaultValue = C.GoString(def)
	info.SystemDefault = C.GoString(sysDef)
	info.Flags = int(flags)
	ok = true
	return
}

// GetClassCallbackFormat returns the parameter format of a registered callback,
// or "" if the class or callback is not registered.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getclasscallbackformat.md
func GetClassCallbackFormat(className, name string) string {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return C.GoString(C.IupGetClassCallbackFormat(cClassName, cName))
}

// ClassInfo is the metadata returned by [GetClassInfo].
type ClassInfo struct {
	Parent        string
	NativeType    string
	ChildType     int
	IsInteractive bool
	HasAttribID   int
}

// GetClassInfo returns class-level metadata: parent class, native type, child
// constraints, focus interactivity and id-attribute support. ok is false when
// the class is not registered.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getclassinfo.md
func GetClassInfo(className string) (info ClassInfo, ok bool) {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	var parent, nativeType *C.char
	var childType, isInteractive, hasAttribID C.int

	if C.IupGetClassInfo(cClassName, &parent, &nativeType, &childType, &isInteractive, &hasAttribID) != 0 {
		return
	}

	info.Parent = C.GoString(parent)
	info.NativeType = C.GoString(nativeType)
	info.ChildType = int(childType)
	info.IsInteractive = isInteractive != 0
	info.HasAttribID = int(hasAttribID)
	ok = true
	return
}

// ClassConstructor is the metadata returned by [GetClassConstructor].
type ClassConstructor struct {
	Format     string // creation parameter format ("h", "g", "s", "i" + "c", ...) or "" if NULL
	FormatAttr string // attribute name receiving the first parameter when Format starts with "s" or "a"
}

// GetClassConstructor returns the parameter format that IupCreate accepts for
// the class. ok is false when the class is not registered.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getclassconstructor.md
func GetClassConstructor(className string) (info ClassConstructor, ok bool) {
	cClassName := C.CString(className)
	defer C.free(unsafe.Pointer(cClassName))

	var format, formatAttr *C.char
	rc := C.IupGetClassConstructor(cClassName, &format, &formatAttr)
	if rc < 0 {
		return
	}
	info.Format = C.GoString(format)
	info.FormatAttr = C.GoString(formatAttr)
	ok = true
	return
}

// GetAllFunctions returns the well-known function-namespace names IUP
// dispatches itself (ENTRY_POINT, IDLE_ACTION,
// GLOBAL{KEYPRESS,BUTTON,MOTION,WHEEL}_CB), plus any extras the application
// has bound. Use [GetFunction] to test whether a given name is currently bound.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getallfunctions.md
func GetAllFunctions() (names []string) {
	max := int(C.IupGetAllFunctions(nil, 0))
	if max <= 0 {
		return
	}
	pNames := make([]*C.char, max)
	n := int(C.IupGetAllFunctions((**C.char)(unsafe.Pointer(&pNames[0])), C.int(max)))
	names = make([]string, n)
	for i := 0; i < n; i++ {
		names[i] = C.GoString(pNames[i])
	}
	return
}

// Flag bits returned by [GetGlobalInfo].
const (
	GlobalNormal   = 0
	GlobalReadOnly = 1
	GlobalPointer  = 2
)

// Driver bits returned by [GetGlobalInfo].
const (
	DriverWin        = 1
	DriverMotif      = 2
	DriverGTK        = 4
	DriverCocoa      = 8
	DriverQt         = 16
	DriverGTK4       = 32
	DriverEFL        = 64
	DriverWinUI      = 128
	DriverFLTK       = 256
	DriverAndroid    = 512
	DriverCocoaTouch = 1024
	DriverHaiku      = 2048
	DriverWasm       = 4096
)

// GlobalInfo is the metadata returned by [GetGlobalInfo].
type GlobalInfo struct {
	Flags   int // OR of Global* flag bits
	Drivers int // OR of Driver* driver bits
}

// GetAllGlobals returns the union of registered globals and any user-set
// globals not already in the registry.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getallglobals.md
func GetAllGlobals() (names []string) {
	max := int(C.IupGetAllGlobals(nil, 0))
	if max <= 0 {
		return
	}
	pNames := make([]*C.char, max)
	n := int(C.IupGetAllGlobals((**C.char)(unsafe.Pointer(&pNames[0])), C.int(max)))
	names = make([]string, n)
	for i := 0; i < n; i++ {
		names[i] = C.GoString(pNames[i])
	}
	return
}

// GetGlobalInfo returns the flag bits and driver-support mask for a registered
// global. ok is false when the name is not in the registry (e.g. an ad-hoc
// user-set global); flags and drivers are then zero.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getglobalinfo.md
func GetGlobalInfo(name string) (info GlobalInfo, ok bool) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	var flags, drivers C.int
	if C.IupGetGlobalInfo(cName, &flags, &drivers) != 1 {
		return
	}
	info.Flags = int(flags)
	info.Drivers = int(drivers)
	ok = true
	return
}

// SaveClassAttributes saves all registered attributes on the internal hash table.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_saveclassattributes.md
func SaveClassAttributes(ih Ihandle) {
	C.IupSaveClassAttributes(ih.ptr())
}

// CopyClassAttributes copies all registered attributes from one element to another.
// Both elements must be of the same class.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_copyclassattributes.md
func CopyClassAttributes(srcIh, dstIh Ihandle) {
	C.IupCopyClassAttributes(srcIh.ptr(), dstIh.ptr())
}

// SetClassDefaultAttribute changes the default value of an attribute for a class.
// It can be any attribute, i.e. registered attributes or user custom attributes.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setclassdefaultattribute.md
func SetClassDefaultAttribute(className, name, value string) {
	cClassName, cName, cValue := C.CString(className), C.CString(name), cStrOrNull(value)
	defer C.free(unsafe.Pointer(cClassName))
	defer C.free(unsafe.Pointer(cName))
	defer cStrFree(cValue)

	C.IupSetClassDefaultAttribute(cClassName, cName, cValue)
}

// Update mark the element or its children to be redraw when the control returns to the system.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_update.md
func Update(ih Ihandle) {
	C.IupUpdate(ih.ptr())
}

// UpdateChildren mark the element or its children to be redraw when the control returns to the system.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_update.md
func UpdateChildren(ih Ihandle) {
	C.IupUpdateChildren(ih.ptr())
}

// Redraw force the element and its children to be redraw immediately.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_redraw.md
func Redraw(ih Ihandle, children int) {
	C.IupRedraw(ih.ptr(), C.int(children))
}

// ConvertXYToPos converts a (x,y) coordinate in an item position.
//
// It can be used for Text (returns a position in the string), List (returns an item),
// Tree (returns a node identifier) or Matrix (returns a cell position, where pos=lin*numcol + col).
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_convertxytopos.md
func ConvertXYToPos(ih Ihandle, x, y int) int {
	return int(C.IupConvertXYToPos(ih.ptr(), C.int(x), C.int(y)))
}

// Refresh updates the size and layout of all controls in the same dialog.
// To be used after changing size attributes, or attributes that affect the size of the control.
// Can be used for any element inside a dialog, but the layout of the dialog and all controls will be updated.
// It can change the layout of all the controls inside the dialog because of the dynamic layout positioning.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_refresh.md
func Refresh(ih Ihandle) {
	C.IupRefresh(ih.ptr())
}

// RefreshChildren updates the size and layout of controls after changing size attributes,
// or attributes that affect the size of the control. Can be used for any element inside a dialog,
// only its children will be updated. It can change the layout of all the controls inside
// the given element because of the dynamic layout positioning.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_refreshchildren.md
func RefreshChildren(ih Ihandle) {
	C.IupRefreshChildren(ih.ptr())
}
