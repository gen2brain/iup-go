//go:build js && wasm

package iup

func Config() Ihandle {
	return ccallHandle("IupConfig", nil, nil)
}

func ConfigDialogClosed(ih, dialog Ihandle, name string) {
	ccall("IupConfigDialogClosed", "", []interface{}{"number", "number", "string"}, []interface{}{int(ih), int(dialog), name})
}

func ConfigDialogShow(ih, dialog Ihandle, name string) {
	ccall("IupConfigDialogShow", "", []interface{}{"number", "number", "string"}, []interface{}{int(ih), int(dialog), name})
}

func ConfigGetVariableDouble(ih Ihandle, group, key string) float64 {
	return ccall("IupConfigGetVariableDouble", "number", []interface{}{"number", "string", "string"}, []interface{}{int(ih), group, key}).Float()
}

func ConfigGetVariableDoubleDef(ih Ihandle, group, key string, def float64) float64 {
	return ccall("IupConfigGetVariableDoubleDef", "number", []interface{}{"number", "string", "string", "number"}, []interface{}{int(ih), group, key, float64(def)}).Float()
}

func ConfigGetVariableDoubleId(ih Ihandle, group, key string, id int) float64 {
	return ccall("IupConfigGetVariableDoubleId", "number", []interface{}{"number", "string", "string", "number"}, []interface{}{int(ih), group, key, id}).Float()
}

func ConfigGetVariableDoubleIdDef(ih Ihandle, group, key string, id int, def float64) float64 {
	return ccall("IupConfigGetVariableDoubleIdDef", "number", []interface{}{"number", "string", "string", "number", "number"}, []interface{}{int(ih), group, key, id, float64(def)}).Float()
}

func ConfigGetVariableInt(ih Ihandle, group, key string) int {
	return ccall("IupConfigGetVariableInt", "number", []interface{}{"number", "string", "string"}, []interface{}{int(ih), group, key}).Int()
}

func ConfigGetVariableIntDef(ih Ihandle, group, key string, def int) int {
	return ccall("IupConfigGetVariableIntDef", "number", []interface{}{"number", "string", "string", "number"}, []interface{}{int(ih), group, key, def}).Int()
}

func ConfigGetVariableIntId(ih Ihandle, group, key string, id int) int {
	return ccall("IupConfigGetVariableIntId", "number", []interface{}{"number", "string", "string", "number"}, []interface{}{int(ih), group, key, id}).Int()
}

func ConfigGetVariableIntIdDef(ih Ihandle, group, key string, id int, def int) int {
	return ccall("IupConfigGetVariableIntIdDef", "number", []interface{}{"number", "string", "string", "number", "number"}, []interface{}{int(ih), group, key, id, def}).Int()
}

func ConfigGetVariableStr(ih Ihandle, group, key string) string {
	return ccall("IupConfigGetVariableStr", "string", []interface{}{"number", "string", "string"}, []interface{}{int(ih), group, key}).String()
}

func ConfigGetVariableStrDef(ih Ihandle, group, key string, def string) string {
	return ccall("IupConfigGetVariableStrDef", "string", []interface{}{"number", "string", "string", "string"}, []interface{}{int(ih), group, key, def}).String()
}

func ConfigGetVariableStrId(ih Ihandle, group, key string, id int) string {
	return ccall("IupConfigGetVariableStrId", "string", []interface{}{"number", "string", "string", "number"}, []interface{}{int(ih), group, key, id}).String()
}

func ConfigGetVariableStrIdDef(ih Ihandle, group, key string, id int, def string) string {
	return ccall("IupConfigGetVariableStrIdDef", "string", []interface{}{"number", "string", "string", "number", "string"}, []interface{}{int(ih), group, key, id, def}).String()
}

func ConfigLoad(ih Ihandle) int {
	return ccall("IupConfigLoad", "number", []interface{}{"number"}, []interface{}{int(ih)}).Int()
}

func ConfigRecentInit(ih, menuOrList Ihandle, recentCb ActionFunc, maxRecent int) {
	if recentCb != nil {
		SetCallback(ih, "RECENT_CB", recentCb)
	}
	ccall("IupConfigRecentInit", "", []interface{}{"number", "number", "number", "number"}, []interface{}{int(ih), int(menuOrList), 0, maxRecent})
}

func ConfigRecentUpdate(ih Ihandle, filename string) {
	ccall("IupConfigRecentUpdate", "", []interface{}{"number", "string"}, []interface{}{int(ih), filename})
}

func ConfigSave(ih Ihandle) int {
	return ccall("IupConfigSave", "number", []interface{}{"number"}, []interface{}{int(ih)}).Int()
}

func ConfigSetListVariable(ih Ihandle, group, key, value string, add int) {
	ccall("IupConfigSetListVariable", "", []interface{}{"number", "string", "string", "string", "number"}, []interface{}{int(ih), group, key, value, add})
}

func ConfigSetVariableDouble(ih Ihandle, group, key string, value float64) {
	ccall("IupConfigSetVariableDouble", "", []interface{}{"number", "string", "string", "number"}, []interface{}{int(ih), group, key, float64(value)})
}

func ConfigSetVariableDoubleId(ih Ihandle, group, key string, id int, value float64) {
	ccall("IupConfigSetVariableDoubleId", "", []interface{}{"number", "string", "string", "number", "number"}, []interface{}{int(ih), group, key, id, float64(value)})
}

func ConfigSetVariableInt(ih Ihandle, group, key string, value int) {
	ccall("IupConfigSetVariableInt", "", []interface{}{"number", "string", "string", "number"}, []interface{}{int(ih), group, key, value})
}

func ConfigSetVariableIntId(ih Ihandle, group, key string, id int, value int) {
	ccall("IupConfigSetVariableIntId", "", []interface{}{"number", "string", "string", "number", "number"}, []interface{}{int(ih), group, key, id, value})
}

func ConfigSetVariableStr(ih Ihandle, group, key string, value string) {
	ccall("IupConfigSetVariableStr", "", []interface{}{"number", "string", "string", "string"}, []interface{}{int(ih), group, key, value})
}

func ConfigSetVariableStrId(ih Ihandle, group, key string, id int, value string) {
	ccall("IupConfigSetVariableStrId", "", []interface{}{"number", "string", "string", "number", "string"}, []interface{}{int(ih), group, key, id, value})
}
