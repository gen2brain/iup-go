package iup

import (
	"bytes"
	"fmt"
	"image/color"
	"reflect"
	"strings"
	"unsafe"
)

/*
#include <stdlib.h>
#include "iup.h"
*/
import "C"

// SetAttribute sets an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattribute.html
func SetAttribute(ih Ihandle, name string, value interface{}) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	switch val := value.(type) {
	case nil:
		C.IupSetAttribute(ih.ptr(), cName, nil)
	case Ihandle:
		C.IupSetAttribute(ih.ptr(), cName, cih(val))
	case uintptr:
		C.IupSetAttribute(ih.ptr(), cName, cih(value.(Ihandle)))
	case string:
		cValue := cStrOrNull(val)
		defer cStrFree(cValue)
		C.IupSetStrAttribute(ih.ptr(), cName, cValue)
	case int, int8, int16, int32, int64, uint, uint8, uint16, uint32, uint64:
		C.IupSetInt(ih.ptr(), cName, C.int(reflect.ValueOf(value).Int()))
	case float32:
		C.IupSetFloat(ih.ptr(), cName, C.float(val))
	case float64:
		C.IupSetDouble(ih.ptr(), cName, C.double(val))
	case [3]uint8:
		C.IupSetRGB(ih.ptr(), cName, C.uchar(val[0]), C.uchar(val[1]), C.uchar(val[2]))
	case [4]uint8:
		C.IupSetRGBA(ih.ptr(), cName, C.uchar(val[0]), C.uchar(val[1]), C.uchar(val[2]), C.uchar(val[3]))
	case color.RGBA:
		C.IupSetRGBA(ih.ptr(), cName, C.uchar(val.R), C.uchar(val.G), C.uchar(val.B), C.uchar(val.A))
	case color.NRGBA:
		C.IupSetRGBA(ih.ptr(), cName, C.uchar(val.R), C.uchar(val.G), C.uchar(val.B), C.uchar(val.A))
	default:
		panic("bad argument passed to SetAttribute")
	}
}

// SetAttributes sets several attributes of an interface element.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattributes.html
func SetAttributes(ih Ihandle, str string) Ihandle {
	cStr := C.CString(str)
	defer C.free(unsafe.Pointer(cStr))

	return mkih(C.IupSetAttributes(ih.ptr(), cStr))
}

// ResetAttribute removes an attribute from the hash table of the element, and its children if the attribute is inheritable.
// It is useful to reset the state of inheritable attributes in a tree of elements.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupresetattribute.html
func ResetAttribute(ih Ihandle, name string) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupResetAttribute(ih.ptr(), cName)
}

// SetAtt sets several attributes of an interface element and optionally sets its name.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetatt.html
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

// SetAttrs method does not exist in C Iup. It has been provided as a convenience function to allow code such as:
//
//     box := iup.Hbox(button1, button2).SetAttrs("GAP", "5", "MARGIN", "8x8")
//
// C Iup provides SetAtt for this purpose but in Go Iup SetAttrs is an easier method to
// accomplish this task due to no necessity of handle_name.
func SetAttrs(ih Ihandle, args ...string) Ihandle {
	return SetAtt(ih, "", args...)
}

// SetAttributeHandle instead of using SetHandle and SetAttribute with a new creative name,
// this function automatically creates a non conflict name and associates the name with the attribute.
//
// It is very useful for associating images and menus.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattributehandle.html
func SetAttributeHandle(ih Ihandle, name string, ihNamed Ihandle) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupSetAttributeHandle(ih.ptr(), cName, ihNamed.ptr())
}

// GetAttribute returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetAttribute(ih Ihandle, name string) string {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return C.GoString(C.IupGetAttribute(ih.ptr(), cName))
}

// GetAllAttributes returns the names of all attributes of an element that are set in its internal hash table only.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetallattributes.html
func GetAllAttributes(ih Ihandle) (ret []string) {
	n := int(C.IupGetAllAttributes(ih.ptr(), nil, 0))
	if n > 0 {
		ret = make([]string, n)
		pRets := make([]*C.char, n)
		C.IupGetAllAttributes(ih.ptr(), (**C.char)(unsafe.Pointer(&pRets[0])), C.int(n))
		for i := 0; i < n; i++ {
			ret[i] = C.GoString(pRets[i])
		}
	}
	return
}

// GetAttributes returns all attributes of a given element that are set in the internal hash table.
// The known attributes that are pointers (not strings) are returned as integers.
//
// The internal attributes are not returned (attributes prefixed with "_IUP").
//
// Before calling this function the application must ensure that there is no pointer attributes
// set for that element, although all known pointers attributes are handled.
//
// This function should be avoided. Use iup.GetAllAttributes instead.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattributes.html
func GetAttributes(ih Ihandle) string {
	return C.GoString(C.IupGetAttributes(ih.ptr()))
}

// GetAttributeHandle instead of using GetAttribute and GetHandle, this function directly returns the associated handle.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattributehandle.html
func GetAttributeHandle(ih Ihandle, name string) Ihandle {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return mkih(C.IupGetAttributeHandle(ih.ptr(), cName))
}

// SetAttributeHandleId sets an attribute handle with an id.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattributehandle.html
func SetAttributeHandleId(ih Ihandle, name string, id int, ihNamed Ihandle) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupSetAttributeHandleId(ih.ptr(), cName, C.int(id), ihNamed.ptr())
}

// GetAttributeHandleId returns the handle attribute with an id.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattributehandle.html
func GetAttributeHandleId(ih Ihandle, name string, id int) Ihandle {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return mkih(C.IupGetAttributeHandleId(ih.ptr(), cName, C.int(id)))
}

// SetAttributeHandleId2 sets an attribute handle with lin and col.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattributehandle.html
func SetAttributeHandleId2(ih Ihandle, name string, lin, col int, ihNamed Ihandle) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupSetAttributeHandleId2(ih.ptr(), cName, C.int(lin), C.int(col), ihNamed.ptr())
}

// GetAttributeHandleId2 returns the handle attribute with lin and col.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattributehandle.html
func GetAttributeHandleId2(ih Ihandle, name string, lin, col int) Ihandle {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return mkih(C.IupGetAttributeHandleId2(ih.ptr(), cName, C.int(lin), C.int(col)))
}

// SetAttributeId sets an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattribute.html
func SetAttributeId(ih Ihandle, name string, id int, value interface{}) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	switch val := value.(type) {
	case nil:
		C.IupSetAttributeId(ih.ptr(), cName, C.int(id), nil)
	case Ihandle:
		C.IupSetAttributeId(ih.ptr(), cName, C.int(id), cih(value.(Ihandle)))
	case uintptr:
		C.IupSetAttributeId(ih.ptr(), cName, C.int(id), cih(value.(Ihandle)))
	case string:
		cValue := cStrOrNull(val)
		defer cStrFree(cValue)
		C.IupSetStrAttributeId(ih.ptr(), cName, C.int(id), cValue)
	case int, int8, int16, int32, int64, uint, uint8, uint16, uint32, uint64:
		C.IupSetIntId(ih.ptr(), cName, C.int(id), C.int(reflect.ValueOf(value).Int()))
	case float32:
		C.IupSetFloatId(ih.ptr(), cName, C.int(id), C.float(val))
	case float64:
		C.IupSetDoubleId(ih.ptr(), cName, C.int(id), C.double(val))
	case [3]uint8:
		C.IupSetRGBId(ih.ptr(), cName, C.int(id), C.uchar(val[0]), C.uchar(val[1]), C.uchar(val[2]))
	default:
		panic("bad argument passed to SetAttributeId")
	}
}

// GetAttributeId returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetAttributeId(ih Ihandle, name string, id int) string {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return C.GoString(C.IupGetAttributeId(ih.ptr(), cName, C.int(id)))
}

// SetAttributeId2 sets an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattribute.html
func SetAttributeId2(ih Ihandle, name string, lin, col int, value interface{}) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	switch val := value.(type) {
	case nil:
		C.IupSetAttributeId2(ih.ptr(), cName, C.int(lin), C.int(col), nil)
	case Ihandle:
		C.IupSetAttributeId2(ih.ptr(), cName, C.int(lin), C.int(col), cih(value.(Ihandle)))
	case uintptr:
		C.IupSetAttributeId2(ih.ptr(), cName, C.int(lin), C.int(col), cih(value.(Ihandle)))
	case string:
		cValue := cStrOrNull(val)
		defer cStrFree(cValue)
		C.IupSetStrAttributeId2(ih.ptr(), cName, C.int(lin), C.int(col), cValue)
	case int, int8, int16, int32, int64, uint, uint8, uint16, uint32, uint64:
		C.IupSetIntId2(ih.ptr(), cName, C.int(lin), C.int(col), C.int(reflect.ValueOf(value).Int()))
	case float32:
		C.IupSetFloatId2(ih.ptr(), cName, C.int(lin), C.int(col), C.float(val))
	case float64:
		C.IupSetDoubleId2(ih.ptr(), cName, C.int(lin), C.int(col), C.double(val))
	case [3]uint8:
		C.IupSetRGBId2(ih.ptr(), cName, C.int(lin), C.int(col), C.uchar(val[0]), C.uchar(val[1]), C.uchar(val[2]))
	default:
		panic("bad argument passed to SetAttributeId2")
	}
}

// SetRGBId2 sets an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattribute.html
func SetRGBId2(ih Ihandle, name string, lin, col int, r, g, b uint8) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupSetRGBId2(ih.ptr(), cName, C.int(lin), C.int(col), C.uchar(r), C.uchar(g), C.uchar(b))
}

// GetAttributeId2 returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetAttributeId2(ih Ihandle, name string, lin, col int) string {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return C.GoString(C.IupGetAttributeId2(ih.ptr(), cName, C.int(lin), C.int(col)))
}

// SetGlobal sets an attribute in the global environment.
// If the driver process the attribute then it will not be stored internally.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetglobal.html
func SetGlobal(name string, value interface{}) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	switch val := value.(type) {
	case nil:
		C.IupSetStrGlobal(cName, nil)
	case string:
		cValue := cStrOrNull(val)
		defer cStrFree(cValue)
		C.IupSetStrGlobal(cName, cValue)
	case Ihandle:
		C.IupSetGlobal(cName, cih(val))
	case uintptr:
		C.IupSetGlobal(cName, cih(Ihandle(val)))
	default:
		panic("bad argument passed to SetGlobal")
	}
}

// GetGlobal returns an attribute value from the global environment.
// The value can be returned from the driver or from the internal storage.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetglobal.html
func GetGlobal(name string) string {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return C.GoString(C.IupGetGlobal(cName))
}

// GetGlobalPtr returns an attribute value from the global environment.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetglobal.html
func GetGlobalPtr(name string) uintptr {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return uintptr(unsafe.Pointer(C.IupGetGlobal(cName)))
}

// GetGlobalIh returns an attribute value from the global environment.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetglobal.html
func GetGlobalIh(name string) Ihandle {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return Ihandle(unsafe.Pointer(C.IupGetGlobal(cName)))

}

// StringCompare is an utility function to compare strings lexicographically.
// Used internally in MatrixEx when sorting, but available in the main library.
//
// This means that numbers and text in the string are sorted separately (for ex: A1 A2 A11 A30 B1).
// Also natural alphabetic order is used: 123...aAáÁ...bBcC...
// The comparison will work only for Latin-1 characters, even if UTF8MODE is Yes.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupstringcompare.html
func StringCompare(str1, str2 string, caseSensitive, lexicographic bool) int {
	cStr1, cStr2 := C.CString(str1), C.CString(str2)
	defer C.free(unsafe.Pointer(cStr1))
	defer C.free(unsafe.Pointer(cStr2))

	return int(C.IupStringCompare(cStr1, cStr2, C.int(bool2int(caseSensitive)), C.int(bool2int(lexicographic))))
}

// SetRGB sets an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattribute.html
func SetRGB(ih Ihandle, name string, r, g, b uint8) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupSetRGB(ih.ptr(), cName, C.uchar(r), C.uchar(g), C.uchar(b))
}

// SetRGBA sets an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattribute.html
func SetRGBA(ih Ihandle, name string, r, g, b, a uint8) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupSetRGBA(ih.ptr(), cName, C.uchar(r), C.uchar(g), C.uchar(b), C.uchar(a))
}

// SetRGBId sets an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattribute.html
func SetRGBId(ih Ihandle, name string, id int, r, g, b uint8) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupSetRGBId(ih.ptr(), cName, C.int(id), C.uchar(r), C.uchar(g), C.uchar(b))
}

// GetInt returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetInt(ih Ihandle, name string) int {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return int(C.IupGetInt(ih.ptr(), cName))
}

// GetInt2 returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetInt2(ih Ihandle, name string) (count, i1, i2 int) { // count = 0, 1 or 2
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	count = int(C.IupGetIntInt(ih.ptr(), cName, (*C.int)(unsafe.Pointer(&i1)), (*C.int)(unsafe.Pointer(&i2))))
	return
}

// GetBool returns a boolean attribute value.
// Returns true for "YES", "ON", "TRUE", "1" (case insensitive), false otherwise.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetBool(ih Ihandle, name string) bool {
	val := strings.ToUpper(GetAttribute(ih, name))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}

// GetFloat returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetFloat(ih Ihandle, name string) float32 {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return float32(C.IupGetFloat(ih.ptr(), cName))
}

// GetDouble returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetDouble(ih Ihandle, name string) float64 {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return float64(C.IupGetDouble(ih.ptr(), cName))
}

// GetRGB returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetRGB(ih Ihandle, name string) (r, g, b uint8) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupGetRGB(ih.ptr(), cName, (*C.uchar)(unsafe.Pointer(&r)), (*C.uchar)(unsafe.Pointer(&g)), (*C.uchar)(unsafe.Pointer(&b)))
	return
}

// GetRGBA returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetRGBA(ih Ihandle, name string) (r, g, b, a uint8) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupGetRGBA(ih.ptr(), cName, (*C.uchar)(unsafe.Pointer(&r)), (*C.uchar)(unsafe.Pointer(&g)), (*C.uchar)(unsafe.Pointer(&b)), (*C.uchar)(unsafe.Pointer(&a)))
	return
}

// GetIntId returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetIntId(ih Ihandle, name string, id int) int {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return int(C.IupGetIntId(ih.ptr(), cName, C.int(id)))
}

// GetFloatId returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetFloatId(ih Ihandle, name string, id int) float32 {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return float32(C.IupGetFloatId(ih.ptr(), cName, C.int(id)))
}

// GetDoubleId returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetDoubleId(ih Ihandle, name string, id int) float64 {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return float64(C.IupGetDoubleId(ih.ptr(), cName, C.int(id)))
}

// GetRGBId returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetRGBId(ih Ihandle, name string, id int) (r, g, b uint8) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupGetRGBId(ih.ptr(), cName, C.int(id), (*C.uchar)(unsafe.Pointer(&r)), (*C.uchar)(unsafe.Pointer(&g)), (*C.uchar)(unsafe.Pointer(&b)))
	return
}

// GetBoolId returns a boolean attribute value with an id.
// Returns true for "YES", "ON", "TRUE", "1" (case insensitive), false otherwise.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetBoolId(ih Ihandle, name string, id int) bool {
	val := strings.ToUpper(GetAttributeId(ih, name, id))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}

// GetIntId2 returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetIntId2(ih Ihandle, name string, lin, col int) int {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return int(C.IupGetIntId2(ih.ptr(), cName, C.int(lin), C.int(col)))
}

// GetFloatId2 returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetFloatId2(ih Ihandle, name string, lin, col int) float32 {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return float32(C.IupGetFloatId2(ih.ptr(), cName, C.int(lin), C.int(col)))
}

// GetDoubleId2 returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetDoubleId2(ih Ihandle, name string, lin, col int) float64 {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return float64(C.IupGetDoubleId2(ih.ptr(), cName, C.int(lin), C.int(col)))
}

// GetRGBId2 returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetRGBId2(ih Ihandle, name string, lin, col int) (r, g, b uint8) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	C.IupGetRGBId2(ih.ptr(), cName, C.int(lin), C.int(col), (*C.uchar)(unsafe.Pointer(&r)), (*C.uchar)(unsafe.Pointer(&g)), (*C.uchar)(unsafe.Pointer(&b)))
	return
}

// GetBoolId2 returns a boolean attribute value with lin and col.
// Returns true for "YES", "ON", "TRUE", "1" (case insensitive), false otherwise.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
func GetBoolId2(ih Ihandle, name string, lin, col int) bool {
	val := strings.ToUpper(GetAttributeId2(ih, name, lin, col))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}
