//go:build !js

package iup

import (
	"unsafe"
)

/*
#include <stdlib.h>
#include "iup.h"
#include "iup_config.h"
#include "bind_callbacks.h"
*/
import "C"

// Config creates a new configuration database. To destroy it use the Destroy function.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func Config() Ihandle {
	h := mkih(C.IupConfig())
	return h
}

// ConfigLoad loads the configuration file.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigLoad(ih Ihandle) int {
	return int(C.IupConfigLoad(ih.ptr()))
}

// ConfigSave saves the configuration file.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigSave(ih Ihandle) int {
	return int(C.IupConfigSave(ih.ptr()))
}

// ConfigSetVariableStr stores a string variable in a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigSetVariableStr(ih Ihandle, group, key string, value string) {
	cGroup, cKey, cValue := C.CString(group), C.CString(key), cStrOrNull(value)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))
	defer cStrFree(cValue)

	C.IupConfigSetVariableStr(ih.ptr(), cGroup, cKey, cValue)
}

// ConfigSetVariableStrId stores a string variable with an id in a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigSetVariableStrId(ih Ihandle, group, key string, id int, value string) {
	cGroup, cKey, cValue := C.CString(group), C.CString(key), cStrOrNull(value)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))
	defer cStrFree(cValue)

	C.IupConfigSetVariableStrId(ih.ptr(), cGroup, cKey, C.int(id), cValue)
}

// ConfigSetVariableInt stores an integer variable in a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigSetVariableInt(ih Ihandle, group, key string, value int) {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	C.IupConfigSetVariableInt(ih.ptr(), cGroup, cKey, C.int(value))
}

// ConfigSetVariableIntId stores an integer variable with an id in a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigSetVariableIntId(ih Ihandle, group, key string, id int, value int) {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	C.IupConfigSetVariableIntId(ih.ptr(), cGroup, cKey, C.int(id), C.int(value))
}

// ConfigSetVariableDouble stores a floating point variable in a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigSetVariableDouble(ih Ihandle, group, key string, value float64) {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	C.IupConfigSetVariableDouble(ih.ptr(), cGroup, cKey, C.double(value))
}

// ConfigSetVariableDoubleId stores a floating point variable with an id in a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigSetVariableDoubleId(ih Ihandle, group, key string, id int, value float64) {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	C.IupConfigSetVariableDoubleId(ih.ptr(), cGroup, cKey, C.int(id), C.double(value))
}

// ConfigGetVariableStr returns a string variable of a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableStr(ih Ihandle, group, key string) string {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return C.GoString(C.IupConfigGetVariableStr(ih.ptr(), cGroup, cKey))
}

// ConfigGetVariableStrId returns a string variable with an id of a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableStrId(ih Ihandle, group, key string, id int) string {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return C.GoString(C.IupConfigGetVariableStrId(ih.ptr(), cGroup, cKey, C.int(id)))
}

// ConfigGetVariableInt returns an integer variable of a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableInt(ih Ihandle, group, key string) int {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return int(C.IupConfigGetVariableInt(ih.ptr(), cGroup, cKey))
}

// ConfigGetVariableIntId returns an integer variable with an id of a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableIntId(ih Ihandle, group, key string, id int) int {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return int(C.IupConfigGetVariableIntId(ih.ptr(), cGroup, cKey, C.int(id)))
}

// ConfigGetVariableDouble returns a floating point variable of a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableDouble(ih Ihandle, group, key string) float64 {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return float64(C.IupConfigGetVariableDouble(ih.ptr(), cGroup, cKey))
}

// ConfigGetVariableDoubleId returns a floating point variable with an id of a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableDoubleId(ih Ihandle, group, key string, id int) float64 {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return float64(C.IupConfigGetVariableDoubleId(ih.ptr(), cGroup, cKey, C.int(id)))
}

// ConfigGetVariableStrDef returns a string variable of a group, or def when it is not set.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableStrDef(ih Ihandle, group, key string, def string) string {
	cGroup, cKey, cDef := C.CString(group), C.CString(key), C.CString(def)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))
	defer C.free(unsafe.Pointer(cDef))

	return C.GoString(C.IupConfigGetVariableStrDef(ih.ptr(), cGroup, cKey, cDef))
}

// ConfigGetVariableStrIdDef returns a string variable with an id of a group, or def when it is not set.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableStrIdDef(ih Ihandle, group, key string, id int, def string) string {
	cGroup, cKey, cDef := C.CString(group), C.CString(key), C.CString(def)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))
	defer C.free(unsafe.Pointer(cDef))

	return C.GoString(C.IupConfigGetVariableStrIdDef(ih.ptr(), cGroup, cKey, C.int(id), cDef))
}

// ConfigGetVariableIntDef returns an integer variable of a group, or def when it is not set.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableIntDef(ih Ihandle, group, key string, def int) int {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return int(C.IupConfigGetVariableIntDef(ih.ptr(), cGroup, cKey, C.int(def)))
}

// ConfigGetVariableIntIdDef returns an integer variable with an id of a group, or def when it is not set.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableIntIdDef(ih Ihandle, group, key string, id int, def int) int {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return int(C.IupConfigGetVariableIntIdDef(ih.ptr(), cGroup, cKey, C.int(id), C.int(def)))
}

// ConfigGetVariableDoubleDef returns a floating point variable of a group, or def when it is not set.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableDoubleDef(ih Ihandle, group, key string, def float64) float64 {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return float64(C.IupConfigGetVariableDoubleDef(ih.ptr(), cGroup, cKey, C.double(def)))
}

// ConfigGetVariableDoubleIdDef returns a floating point variable with an id of a group, or def when it is not set.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigGetVariableDoubleIdDef(ih Ihandle, group, key string, id int, def float64) float64 {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return float64(C.IupConfigGetVariableDoubleIdDef(ih.ptr(), cGroup, cKey, C.int(id), C.double(def)))
}

// ConfigSetListVariable adds a value to, or removes it from, a list variable of a group.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigSetListVariable(ih Ihandle, group, key, value string, add int) {
	cGroup, cKey, cValue := C.CString(group), C.CString(key), cStrOrNull(value)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))
	defer cStrFree(cValue)

	C.IupConfigSetListVariable(ih.ptr(), cGroup, cKey, cValue, C.int(add))
}

// ConfigDialogShow show the dialog adjusting its size and position..
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigDialogShow(ih, dialog Ihandle, name string) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupConfigDialogShow(ih.ptr(), dialog.ptr(), cName)
}

// ConfigDialogClosed save the last dialog position and size when the dialog is about to be closed,
// usually inside the dialog CLOSE_CB callback..
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigDialogClosed(ih, dialog Ihandle, name string) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupConfigDialogClosed(ih.ptr(), dialog.ptr(), cName)
}

// ConfigRecentInit initializes the recent files feature for a config handle.
// menuOrList can be either a Menu or a List/FlatList control.
// maxRecent is the maximum number of recent files to track.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigRecentInit(ih, menuOrList Ihandle, recentCb ActionFunc, maxRecent int) {
	setRecentFunc(ih, recentCb)
	C.IupConfigRecentInit(ih.ptr(), menuOrList.ptr(), (C.Icallback)(C.goIupRecentCB), C.int(maxRecent))
}

// ConfigRecentUpdate adds or moves a filename to the top of the recent files list.
// Call ConfigSave after this to persist the change.
//
// https://gen2brain.github.io/iup-go/func/iup_config.html
func ConfigRecentUpdate(ih Ihandle, filename string) {
	cFilename := C.CString(filename)
	defer C.free(unsafe.Pointer(cFilename))
	C.IupConfigRecentUpdate(ih.ptr(), cFilename)
}
