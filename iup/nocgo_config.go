//go:build !cgo && !js

package iup

import "github.com/ebitengine/purego"

func Config() Ihandle {
	return mkih(iupConfig())
}

func ConfigLoad(ih Ihandle) int {
	return int(iupConfigLoad(uintptr(ih)))
}

func ConfigSave(ih Ihandle) int {
	return int(iupConfigSave(uintptr(ih)))
}

func ConfigSetVariableStr(ih Ihandle, group, key string, value string) {
	iupConfigSetVariableStr(uintptr(ih), group, key, optCStr(value))
}

func ConfigSetVariableStrId(ih Ihandle, group, key string, id int, value string) {
	iupConfigSetVariableStrId(uintptr(ih), group, key, int32(id), optCStr(value))
}

func ConfigSetVariableInt(ih Ihandle, group, key string, value int) {
	iupConfigSetVariableInt(uintptr(ih), group, key, int32(value))
}

func ConfigSetVariableIntId(ih Ihandle, group, key string, id int, value int) {
	iupConfigSetVariableIntId(uintptr(ih), group, key, int32(id), int32(value))
}

func ConfigSetVariableDouble(ih Ihandle, group, key string, value float64) {
	iupConfigSetVariableDouble(uintptr(ih), group, key, value)
}

func ConfigSetVariableDoubleId(ih Ihandle, group, key string, id int, value float64) {
	iupConfigSetVariableDoubleId(uintptr(ih), group, key, int32(id), value)
}

func ConfigGetVariableStr(ih Ihandle, group, key string) string {
	return iupConfigGetVariableStr(uintptr(ih), group, key)
}

func ConfigGetVariableStrId(ih Ihandle, group, key string, id int) string {
	return iupConfigGetVariableStrId(uintptr(ih), group, key, int32(id))
}

func ConfigGetVariableInt(ih Ihandle, group, key string) int {
	return int(iupConfigGetVariableInt(uintptr(ih), group, key))
}

func ConfigGetVariableIntId(ih Ihandle, group, key string, id int) int {
	return int(iupConfigGetVariableIntId(uintptr(ih), group, key, int32(id)))
}

func ConfigGetVariableDouble(ih Ihandle, group, key string) float64 {
	return iupConfigGetVariableDouble(uintptr(ih), group, key)
}

func ConfigGetVariableDoubleId(ih Ihandle, group, key string, id int) float64 {
	return iupConfigGetVariableDoubleId(uintptr(ih), group, key, int32(id))
}

func ConfigGetVariableStrDef(ih Ihandle, group, key string, def string) string {
	return iupConfigGetVariableStrDef(uintptr(ih), group, key, def)
}

func ConfigGetVariableStrIdDef(ih Ihandle, group, key string, id int, def string) string {
	return iupConfigGetVariableStrIdDef(uintptr(ih), group, key, int32(id), def)
}

func ConfigGetVariableIntDef(ih Ihandle, group, key string, def int) int {
	return int(iupConfigGetVariableIntDef(uintptr(ih), group, key, int32(def)))
}

func ConfigGetVariableIntIdDef(ih Ihandle, group, key string, id int, def int) int {
	return int(iupConfigGetVariableIntIdDef(uintptr(ih), group, key, int32(id), int32(def)))
}

func ConfigGetVariableDoubleDef(ih Ihandle, group, key string, def float64) float64 {
	return iupConfigGetVariableDoubleDef(uintptr(ih), group, key, def)
}

func ConfigGetVariableDoubleIdDef(ih Ihandle, group, key string, id int, def float64) float64 {
	return iupConfigGetVariableDoubleIdDef(uintptr(ih), group, key, int32(id), def)
}

func ConfigSetListVariable(ih Ihandle, group, key, value string, add int) {
	iupConfigSetListVariable(uintptr(ih), group, key, optCStr(value), int32(add))
}

func ConfigDialogShow(ih, dialog Ihandle, name string) {
	iupConfigDialogShow(uintptr(ih), uintptr(dialog), name)
}

func ConfigDialogClosed(ih, dialog Ihandle, name string) {
	iupConfigDialogClosed(uintptr(ih), uintptr(dialog), name)
}

var recentCB = purego.NewCallback(func(ih uintptr) int {
	if f, ok := loadCallback(Ihandle(ih), "_IUPGO_RECENT_CB").(ActionFunc); ok {
		return f(Ihandle(ih))
	}
	return DEFAULT
})

func ConfigRecentInit(ih, menuOrList Ihandle, recentCb ActionFunc, maxRecent int) {
	storeCallback(ih, "_IUPGO_RECENT_CB", recentCb)
	iupConfigRecentInit(uintptr(ih), uintptr(menuOrList), recentCB, int32(maxRecent))
}

func ConfigRecentUpdate(ih Ihandle, filename string) {
	iupConfigRecentUpdate(uintptr(ih), filename)
}
