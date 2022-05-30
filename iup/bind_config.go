package iup

import (
	"unsafe"

	"github.com/google/uuid"
)

/*
#include <stdlib.h>
#include "iup.h"
#include "iup_config.h"
*/
import "C"

// Config creates a new configuration database. To destroy it use the Destroy function.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func Config() Ihandle {
	h := mkih(C.IupConfig())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ConfigLoad loads the configuration file.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigLoad(ih Ihandle) int {
	return int(C.IupConfigLoad(ih.ptr()))
}

// ConfigSave saves the configuration file.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigSave(ih Ihandle) int {
	return int(C.IupConfigSave(ih.ptr()))
}

// ConfigSetVariableStr .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigSetVariableStr(ih Ihandle, group, key string, value string) {
	cGroup, cKey, cValue := C.CString(group), C.CString(key), C.CString(value)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))
	defer C.free(unsafe.Pointer(cValue))

	C.IupConfigSetVariableStr(ih.ptr(), cGroup, cKey, cValue)
}

// ConfigSetVariableStrId .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigSetVariableStrId(ih Ihandle, group, key string, id int, value string) {
	cGroup, cKey, cValue := C.CString(group), C.CString(key), C.CString(value)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))
	defer C.free(unsafe.Pointer(cValue))

	C.IupConfigSetVariableStrId(ih.ptr(), cGroup, cKey, C.int(id), cValue)
}

// ConfigSetVariableInt .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigSetVariableInt(ih Ihandle, group, key string, value int) {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	C.IupConfigSetVariableInt(ih.ptr(), cGroup, cKey, C.int(value))
}

// ConfigSetVariableIntId .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigSetVariableIntId(ih Ihandle, group, key string, id int, value int) {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	C.IupConfigSetVariableIntId(ih.ptr(), cGroup, cKey, C.int(id), C.int(value))
}

// ConfigSetVariableDouble .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigSetVariableDouble(ih Ihandle, group, key string, value float64) {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	C.IupConfigSetVariableDouble(ih.ptr(), cGroup, cKey, C.double(value))
}

// ConfigSetVariableDoubleId .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigSetVariableDoubleId(ih Ihandle, group, key string, id int, value float64) {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	C.IupConfigSetVariableDoubleId(ih.ptr(), cGroup, cKey, C.int(id), C.double(value))
}

// ConfigGetVariableStr .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableStr(ih Ihandle, group, key string) string {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return C.GoString(C.IupConfigGetVariableStr(ih.ptr(), cGroup, cKey))
}

// ConfigGetVariableStrId .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableStrId(ih Ihandle, group, key string, id int) string {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return C.GoString(C.IupConfigGetVariableStrId(ih.ptr(), cGroup, cKey, C.int(id)))
}

// ConfigGetVariableInt .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableInt(ih Ihandle, group, key string) int {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return int(C.IupConfigGetVariableInt(ih.ptr(), cGroup, cKey))
}

// ConfigGetVariableIntId .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableIntId(ih Ihandle, group, key string, id int) int {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return int(C.IupConfigGetVariableIntId(ih.ptr(), cGroup, cKey, C.int(id)))
}

// ConfigGetVariableDouble .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableDouble(ih Ihandle, group, key string) float64 {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return float64(C.IupConfigGetVariableDouble(ih.ptr(), cGroup, cKey))
}

// ConfigGetVariableDoubleId .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableDoubleId(ih Ihandle, group, key string, id int) float64 {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return float64(C.IupConfigGetVariableDoubleId(ih.ptr(), cGroup, cKey, C.int(id)))
}

// ConfigGetVariableStrDef .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableStrDef(ih Ihandle, group, key string, def string) string {
	cGroup, cKey, cDef := C.CString(group), C.CString(key), C.CString(def)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))
	defer C.free(unsafe.Pointer(cDef))

	return C.GoString(C.IupConfigGetVariableStrDef(ih.ptr(), cGroup, cKey, cDef))
}

// ConfigGetVariableStrIdDef .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableStrIdDef(ih Ihandle, group, key string, id int, def string) string {
	cGroup, cKey, cDef := C.CString(group), C.CString(key), C.CString(def)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))
	defer C.free(unsafe.Pointer(cDef))

	return C.GoString(C.IupConfigGetVariableStrIdDef(ih.ptr(), cGroup, cKey, C.int(id), cDef))
}

// ConfigGetVariableIntDef .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableIntDef(ih Ihandle, group, key string, def int) int {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return int(C.IupConfigGetVariableIntDef(ih.ptr(), cGroup, cKey, C.int(def)))
}

// ConfigGetVariableIntIdDef .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableIntIdDef(ih Ihandle, group, key string, id int, def int) int {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return int(C.IupConfigGetVariableIntIdDef(ih.ptr(), cGroup, cKey, C.int(id), C.int(def)))
}

// ConfigGetVariableDoubleDef .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableDoubleDef(ih Ihandle, group, key string, def float64) float64 {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return float64(C.IupConfigGetVariableDoubleDef(ih.ptr(), cGroup, cKey, C.double(def)))
}

// ConfigGetVariableDoubleIdDef .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigGetVariableDoubleIdDef(ih Ihandle, group, key string, id int, def float64) float64 {
	cGroup, cKey := C.CString(group), C.CString(key)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))

	return float64(C.IupConfigGetVariableDoubleIdDef(ih.ptr(), cGroup, cKey, C.int(id), C.double(def)))
}

// ConfigSetListVariable .
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigSetListVariable(ih Ihandle, group, key, value string, add int) {
	cGroup, cKey, cValue := C.CString(group), C.CString(key), C.CString(value)
	defer C.free(unsafe.Pointer(cGroup))
	defer C.free(unsafe.Pointer(cKey))
	defer C.free(unsafe.Pointer(cValue))

	C.IupConfigSetListVariable(ih.ptr(), cGroup, cKey, cValue, C.int(add))
}

// ConfigDialogShow show the dialog adjusting its size and position..
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigDialogShow(ih, dialog Ihandle, name string) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupConfigDialogShow(ih.ptr(), dialog.ptr(), cName)
}

// ConfigDialogClosed save the last dialog position and size when the dialog is about to be closed,
// usually inside the dialog CLOSE_CB callback..
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupconfig.html
func ConfigDialogClosed(ih, dialog Ihandle, name string) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupConfigDialogClosed(ih.ptr(), dialog.ptr(), cName)
}
