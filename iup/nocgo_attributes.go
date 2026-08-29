//go:build !cgo && !js

package iup

import (
	"bytes"
	"fmt"
	"image/color"
	"reflect"
	"strings"
)

func SetAttribute(ih Ihandle, name string, value interface{}) {
	switch val := value.(type) {
	case nil:
		iupSetAttribute(uintptr(ih), name, 0)
	case Ihandle:
		iupSetAttribute(uintptr(ih), name, uintptr(val))
	case uintptr:
		iupSetAttribute(uintptr(ih), name, val)
	case string:
		if val == "" {
			iupSetAttribute(uintptr(ih), name, 0)
		} else {
			iupSetStrAttribute(uintptr(ih), name, val)
		}
	case bool:
		s := "NO"
		if val {
			s = "YES"
		}
		iupSetStrAttribute(uintptr(ih), name, s)
	case int, int8, int16, int32, int64, uint, uint8, uint16, uint32, uint64:
		iupSetInt(uintptr(ih), name, int32(reflect.ValueOf(value).Int()))
	case float32:
		iupSetFloat(uintptr(ih), name, val)
	case float64:
		iupSetDouble(uintptr(ih), name, val)
	case [3]uint8:
		iupSetRGB(uintptr(ih), name, val[0], val[1], val[2])
	case [4]uint8:
		iupSetRGBA(uintptr(ih), name, val[0], val[1], val[2], val[3])
	case color.RGBA:
		iupSetRGBA(uintptr(ih), name, val.R, val.G, val.B, val.A)
	case color.NRGBA:
		iupSetRGBA(uintptr(ih), name, val.R, val.G, val.B, val.A)
	default:
		panic("bad argument passed to SetAttribute")
	}
}

func SetAttributes(ih Ihandle, str string) Ihandle {
	return mkih(iupSetAttributes(uintptr(ih), str))
}

func ResetAttribute(ih Ihandle, name string) {
	iupResetAttribute(uintptr(ih), name)
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

func SetAttrs(ih Ihandle, args ...string) Ihandle {
	return SetAtt(ih, "", args...)
}

func SetAttributeHandle(ih Ihandle, name string, ihNamed Ihandle) {
	iupSetAttributeHandle(uintptr(ih), name, uintptr(ihNamed))
}

func GetAttribute(ih Ihandle, name string) string {
	if attribIsNotString(ih, name) {
		ptr := iupGetAttributePtr(uintptr(ih), name)
		if ptr == 0 {
			return ""
		}
		return fmt.Sprintf("%#x", ptr)
	}

	return iupGetAttribute(uintptr(ih), name)
}

// attribIsNotString reports whether name is registered IUPAF_NO_STRING for the
// class of ih, meaning its value is a native handle and not a C string.
func attribIsNotString(ih Ihandle, name string) bool {
	className := iupGetClassName(uintptr(ih))
	if className == "" {
		return false
	}

	var flags int32
	if iupGetClassAttributeInfo(className, name, nil, nil, &flags) < 0 {
		return false
	}

	return flags&AttribNoString != 0
}

func GetAllAttributes(ih Ihandle) (ret []string) {
	n := int(iupGetAllAttributes(uintptr(ih), nil, 0))
	if n > 0 {
		buf := make([]uintptr, n)
		iupGetAllAttributes(uintptr(ih), &buf[0], int32(n))
		ret = make([]string, n)
		for i := 0; i < n; i++ {
			ret[i] = goString(buf[i])
		}
	}
	return
}

func GetAttributes(ih Ihandle) string {
	return iupGetAttributes(uintptr(ih))
}

func GetAttributeHandle(ih Ihandle, name string) Ihandle {
	return mkih(iupGetAttributeHandle(uintptr(ih), name))
}

func SetAttributeHandleId(ih Ihandle, name string, id int, ihNamed Ihandle) {
	iupSetAttributeHandleId(uintptr(ih), name, int32(id), uintptr(ihNamed))
}

func GetAttributeHandleId(ih Ihandle, name string, id int) Ihandle {
	return mkih(iupGetAttributeHandleId(uintptr(ih), name, int32(id)))
}

func SetAttributeHandleId2(ih Ihandle, name string, lin, col int, ihNamed Ihandle) {
	iupSetAttributeHandleI2(uintptr(ih), name, int32(lin), int32(col), uintptr(ihNamed))
}

func GetAttributeHandleId2(ih Ihandle, name string, lin, col int) Ihandle {
	return mkih(iupGetAttributeHandleI2(uintptr(ih), name, int32(lin), int32(col)))
}

func SetAttributeId(ih Ihandle, name string, id int, value interface{}) {
	switch val := value.(type) {
	case nil:
		iupSetAttributeId(uintptr(ih), name, int32(id), 0)
	case Ihandle:
		iupSetAttributeId(uintptr(ih), name, int32(id), uintptr(val))
	case uintptr:
		iupSetAttributeId(uintptr(ih), name, int32(id), val)
	case string:
		if val == "" {
			iupSetAttributeId(uintptr(ih), name, int32(id), 0)
		} else {
			iupSetStrAttributeId(uintptr(ih), name, int32(id), val)
		}
	case bool:
		s := "NO"
		if val {
			s = "YES"
		}
		iupSetStrAttributeId(uintptr(ih), name, int32(id), s)
	case int, int8, int16, int32, int64, uint, uint8, uint16, uint32, uint64:
		iupSetIntId(uintptr(ih), name, int32(id), int32(reflect.ValueOf(value).Int()))
	case float32:
		iupSetFloatId(uintptr(ih), name, int32(id), val)
	case float64:
		iupSetDoubleId(uintptr(ih), name, int32(id), val)
	case [3]uint8:
		iupSetRGBId(uintptr(ih), name, int32(id), val[0], val[1], val[2])
	default:
		panic("bad argument passed to SetAttributeId")
	}
}

func GetAttributeId(ih Ihandle, name string, id int) string {
	return iupGetAttributeId(uintptr(ih), name, int32(id))
}

func SetAttributeId2(ih Ihandle, name string, lin, col int, value interface{}) {
	switch val := value.(type) {
	case nil:
		iupSetAttributeId2(uintptr(ih), name, int32(lin), int32(col), 0)
	case Ihandle:
		iupSetAttributeId2(uintptr(ih), name, int32(lin), int32(col), uintptr(val))
	case uintptr:
		iupSetAttributeId2(uintptr(ih), name, int32(lin), int32(col), val)
	case string:
		if val == "" {
			iupSetAttributeId2(uintptr(ih), name, int32(lin), int32(col), 0)
		} else {
			iupSetStrAttributeId2(uintptr(ih), name, int32(lin), int32(col), val)
		}
	case bool:
		s := "NO"
		if val {
			s = "YES"
		}
		iupSetStrAttributeId2(uintptr(ih), name, int32(lin), int32(col), s)
	case int, int8, int16, int32, int64, uint, uint8, uint16, uint32, uint64:
		iupSetIntId2(uintptr(ih), name, int32(lin), int32(col), int32(reflect.ValueOf(value).Int()))
	case float32:
		iupSetFloatId2(uintptr(ih), name, int32(lin), int32(col), val)
	case float64:
		iupSetDoubleId2(uintptr(ih), name, int32(lin), int32(col), val)
	case [3]uint8:
		iupSetRGBId2(uintptr(ih), name, int32(lin), int32(col), val[0], val[1], val[2])
	default:
		panic("bad argument passed to SetAttributeId2")
	}
}

func GetAttributeId2(ih Ihandle, name string, lin, col int) string {
	return iupGetAttributeId2(uintptr(ih), name, int32(lin), int32(col))
}

func SetGlobal(name string, value interface{}) {
	switch val := value.(type) {
	case nil:
		iupSetGlobal(name, 0)
	case string:
		if val == "" {
			iupSetGlobal(name, 0)
		} else {
			iupSetStrGlobal(name, val)
		}
	case Ihandle:
		iupSetGlobal(name, uintptr(val))
	case uintptr:
		iupSetGlobal(name, val)
	default:
		panic("bad argument passed to SetGlobal")
	}
}

func GetGlobal(name string) string {
	return iupGetGlobal(name)
}

func GetGlobalPtr(name string) uintptr {
	return iupGetGlobalRaw(name)
}

func GetGlobalIh(name string) Ihandle {
	return Ihandle(iupGetGlobalRaw(name))
}

func StringCompare(str1, str2 string, caseSensitive, lexicographic bool) int {
	return int(iupStringCompare(str1, str2, int32(bool2int(caseSensitive)), int32(bool2int(lexicographic))))
}

func SetRGB(ih Ihandle, name string, r, g, b uint8) {
	iupSetRGB(uintptr(ih), name, r, g, b)
}

func SetRGBA(ih Ihandle, name string, r, g, b, a uint8) {
	iupSetRGBA(uintptr(ih), name, r, g, b, a)
}

func SetRGBId(ih Ihandle, name string, id int, r, g, b uint8) {
	iupSetRGBId(uintptr(ih), name, int32(id), r, g, b)
}

func SetRGBId2(ih Ihandle, name string, lin, col int, r, g, b uint8) {
	iupSetRGBId2(uintptr(ih), name, int32(lin), int32(col), r, g, b)
}

func GetInt(ih Ihandle, name string) int {
	return int(iupGetInt(uintptr(ih), name))
}

func GetInt2(ih Ihandle, name string) (count, i1, i2 int) {
	var c1, c2 int32
	count = int(iupGetIntInt(uintptr(ih), name, &c1, &c2))
	return count, int(c1), int(c2)
}

func GetBool(ih Ihandle, name string) bool {
	val := strings.ToUpper(GetAttribute(ih, name))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}

func SetBool(ih Ihandle, name string, value bool) {
	SetAttribute(ih, name, value)
}

func SetBoolId(ih Ihandle, name string, id int, value bool) {
	SetAttributeId(ih, name, id, value)
}

func SetBoolId2(ih Ihandle, name string, lin, col int, value bool) {
	SetAttributeId2(ih, name, lin, col, value)
}

func GetFloat(ih Ihandle, name string) float32 {
	return iupGetFloat(uintptr(ih), name)
}

func GetDouble(ih Ihandle, name string) float64 {
	return iupGetDouble(uintptr(ih), name)
}

func GetRGB(ih Ihandle, name string) (r, g, b uint8) {
	iupGetRGB(uintptr(ih), name, &r, &g, &b)
	return
}

func GetRGBA(ih Ihandle, name string) (r, g, b, a uint8) {
	iupGetRGBA(uintptr(ih), name, &r, &g, &b, &a)
	return
}

func GetIntId(ih Ihandle, name string, id int) int {
	return int(iupGetIntId(uintptr(ih), name, int32(id)))
}

func GetFloatId(ih Ihandle, name string, id int) float32 {
	return iupGetFloatId(uintptr(ih), name, int32(id))
}

func GetDoubleId(ih Ihandle, name string, id int) float64 {
	return iupGetDoubleId(uintptr(ih), name, int32(id))
}

func GetRGBId(ih Ihandle, name string, id int) (r, g, b uint8) {
	iupGetRGBId(uintptr(ih), name, int32(id), &r, &g, &b)
	return
}

func GetBoolId(ih Ihandle, name string, id int) bool {
	val := strings.ToUpper(GetAttributeId(ih, name, id))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}

func GetIntId2(ih Ihandle, name string, lin, col int) int {
	return int(iupGetIntId2(uintptr(ih), name, int32(lin), int32(col)))
}

func GetFloatId2(ih Ihandle, name string, lin, col int) float32 {
	return iupGetFloatId2(uintptr(ih), name, int32(lin), int32(col))
}

func GetDoubleId2(ih Ihandle, name string, lin, col int) float64 {
	return iupGetDoubleId2(uintptr(ih), name, int32(lin), int32(col))
}

func GetRGBId2(ih Ihandle, name string, lin, col int) (r, g, b uint8) {
	iupGetRGBId2(uintptr(ih), name, int32(lin), int32(col), &r, &g, &b)
	return
}

func GetBoolId2(ih Ihandle, name string, lin, col int) bool {
	val := strings.ToUpper(GetAttributeId2(ih, name, lin, col))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}
