//go:build js && wasm

package iup

import (
	"bytes"
	"fmt"
	"strings"
)

func GetAllAttributes(ih Ihandle) []string {
	return wasmNames("IupGetAllAttributes", []interface{}{"number"}, []interface{}{int(ih)})
}

func GetAttributeHandle(ih Ihandle, name string) Ihandle {
	return ccallHandle("IupGetAttributeHandle", []interface{}{"number", "string"}, []interface{}{int(ih), name})
}

func GetAttributeHandleId(ih Ihandle, name string, id int) Ihandle {
	return ccallHandle("IupGetAttributeHandleId", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, id})
}

func GetAttributeHandleId2(ih Ihandle, name string, lin, col int) Ihandle {
	return ccallHandle("IupGetAttributeHandleId2", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, lin, col})
}

func GetAttributes(ih Ihandle) string {
	return ccall("IupGetAttributes", "string", []interface{}{"number"}, []interface{}{int(ih)}).String()
}

func GetBool(ih Ihandle, name string) bool {
	val := strings.ToUpper(GetAttribute(ih, name))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}

func GetBoolId(ih Ihandle, name string, id int) bool {
	val := strings.ToUpper(GetAttributeId(ih, name, id))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}

func GetBoolId2(ih Ihandle, name string, lin, col int) bool {
	val := strings.ToUpper(GetAttributeId2(ih, name, lin, col))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}

func GetDouble(ih Ihandle, name string) float64 {
	return ccall("IupGetDouble", "number", []interface{}{"number", "string"}, []interface{}{int(ih), name}).Float()
}

func GetDoubleId(ih Ihandle, name string, id int) float64 {
	return ccall("IupGetDoubleId", "number", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, id}).Float()
}

func GetDoubleId2(ih Ihandle, name string, lin, col int) float64 {
	return ccall("IupGetDoubleId2", "number", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, lin, col}).Float()
}

func GetFloatId(ih Ihandle, name string, id int) float32 {
	return float32(ccall("IupGetFloatId", "number", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, id}).Float())
}

func GetFloatId2(ih Ihandle, name string, lin, col int) float32 {
	return float32(ccall("IupGetFloatId2", "number", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, lin, col}).Float())
}

func GetGlobalIh(name string) Ihandle {
	return ccallHandle("IupGetGlobal", []interface{}{"string"}, []interface{}{name})
}

func GetGlobalPtr(name string) uintptr {
	return uintptr(ccall("IupGetGlobal", "number", []interface{}{"string"}, []interface{}{name}).Int())
}

func GetInt2(ih Ihandle, name string) (count, i1, i2 int) {
	p1, p2 := wasmMalloc(4), wasmMalloc(4)
	defer wasmFree(p1)
	defer wasmFree(p2)
	count = ccall("IupGetIntInt", "number", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, p1, p2}).Int()
	return count, wasmGetI32(p1), wasmGetI32(p2)
}

func GetIntId(ih Ihandle, name string, id int) int {
	return ccall("IupGetIntId", "number", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, id}).Int()
}

func GetIntId2(ih Ihandle, name string, lin, col int) int {
	return ccall("IupGetIntId2", "number", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, lin, col}).Int()
}

func GetRGB(ih Ihandle, name string) (r, g, b uint8) {
	pr, pg, pb := wasmMalloc(1), wasmMalloc(1), wasmMalloc(1)
	defer wasmFree(pr)
	defer wasmFree(pg)
	defer wasmFree(pb)
	ccall("IupGetRGB", "", []interface{}{"number", "string", "number", "number", "number"}, []interface{}{int(ih), name, pr, pg, pb})
	return wasmGetU8(pr), wasmGetU8(pg), wasmGetU8(pb)
}

func GetRGBA(ih Ihandle, name string) (r, g, b, a uint8) {
	pr, pg, pb, pa := wasmMalloc(1), wasmMalloc(1), wasmMalloc(1), wasmMalloc(1)
	defer wasmFree(pr)
	defer wasmFree(pg)
	defer wasmFree(pb)
	defer wasmFree(pa)
	ccall("IupGetRGBA", "", []interface{}{"number", "string", "number", "number", "number", "number"}, []interface{}{int(ih), name, pr, pg, pb, pa})
	return wasmGetU8(pr), wasmGetU8(pg), wasmGetU8(pb), wasmGetU8(pa)
}

func GetRGBId(ih Ihandle, name string, id int) (r, g, b uint8) {
	pr, pg, pb := wasmMalloc(1), wasmMalloc(1), wasmMalloc(1)
	defer wasmFree(pr)
	defer wasmFree(pg)
	defer wasmFree(pb)
	ccall("IupGetRGBId", "", []interface{}{"number", "string", "number", "number", "number", "number"}, []interface{}{int(ih), name, id, pr, pg, pb})
	return wasmGetU8(pr), wasmGetU8(pg), wasmGetU8(pb)
}

func GetRGBId2(ih Ihandle, name string, lin, col int) (r, g, b uint8) {
	pr, pg, pb := wasmMalloc(1), wasmMalloc(1), wasmMalloc(1)
	defer wasmFree(pr)
	defer wasmFree(pg)
	defer wasmFree(pb)
	ccall("IupGetRGBId2", "", []interface{}{"number", "string", "number", "number", "number", "number", "number"}, []interface{}{int(ih), name, lin, col, pr, pg, pb})
	return wasmGetU8(pr), wasmGetU8(pg), wasmGetU8(pb)
}

func ResetAttribute(ih Ihandle, name string) {
	ccall("IupResetAttribute", "", []interface{}{"number", "string"}, []interface{}{int(ih), name})
}

func SetAtt(ih Ihandle, handle_name string, args ...string) Ihandle {
	attrs := bytes.NewBufferString("")
	for i := 0; i < len(args); i += 2 {
		if i > 0 {
			attrs.WriteString(",")
		}
		attrs.WriteString(fmt.Sprintf("%s=\"%s\"", args[i], args[i+1]))
	}
	SetAttributes(ih, attrs.String())
	return ih
}

func SetAttributeHandleId(ih Ihandle, name string, id int, ihNamed Ihandle) {
	ccall("IupSetAttributeHandleId", "", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, id, int(ihNamed)})
}

func SetAttributeHandleId2(ih Ihandle, name string, lin, col int, ihNamed Ihandle) {
	ccall("IupSetAttributeHandleId2", "", []interface{}{"number", "string", "number", "number", "number"}, []interface{}{int(ih), name, lin, col, int(ihNamed)})
}

func SetAttrs(ih Ihandle, args ...string) Ihandle { return SetAtt(ih, "", args...) }

func SetBool(ih Ihandle, name string, value bool) { SetAttribute(ih, name, value) }

func SetBoolId(ih Ihandle, name string, id int, value bool) { SetAttributeId(ih, name, id, value) }

func SetBoolId2(ih Ihandle, name string, lin, col int, v bool) {
	SetAttributeId2(ih, name, lin, col, v)
}

func SetRGB(ih Ihandle, name string, r, g, b uint8) {
	ccall("IupSetRGB", "", []interface{}{"number", "string", "number", "number", "number"}, []interface{}{int(ih), name, int(r), int(g), int(b)})
}

func SetRGBA(ih Ihandle, name string, r, g, b, a uint8) {
	ccall("IupSetRGBA", "", []interface{}{"number", "string", "number", "number", "number", "number"}, []interface{}{int(ih), name, int(r), int(g), int(b), int(a)})
}

func SetRGBId(ih Ihandle, name string, id int, r, g, b uint8) {
	ccall("IupSetRGBId", "", []interface{}{"number", "string", "number", "number", "number", "number"}, []interface{}{int(ih), name, id, int(r), int(g), int(b)})
}

func SetRGBId2(ih Ihandle, name string, lin, col int, r, g, b uint8) {
	ccall("IupSetRGBId2", "", []interface{}{"number", "string", "number", "number", "number", "number", "number"}, []interface{}{int(ih), name, lin, col, int(r), int(g), int(b)})
}

func StringCompare(str1, str2 string, caseSensitive, lexicographic bool) int {
	return ccall("IupStringCompare", "number",
		[]interface{}{"string", "string", "number", "number"},
		[]interface{}{str1, str2, b2i(caseSensitive), b2i(lexicographic)}).Int()
}
