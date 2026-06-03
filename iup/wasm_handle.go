//go:build js && wasm

package iup

import (
	"fmt"
	"sort"
	"strconv"
	"strings"
)

// SetAttributes sets several attributes of an interface element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattributes.md
func (ih Ihandle) SetAttributes(params ...interface{}) Ihandle {
	for _, param := range params {
		switch p := param.(type) {
		case string:
			SetAttributes(ih, p)
		case map[string]string:
			for _, k := range sortAttributeKeys(p) {
				SetAttribute(ih, k, p[k])
			}
		case map[string]interface{}:
			m := make(map[string]string, len(p))
			for k, v := range p {
				m[k] = fmt.Sprintf("%v", v)
			}
			for _, k := range sortAttributeKeys(m) {
				SetAttribute(ih, k, m[k])
			}
		}
	}
	return ih
}

// sortAttributeKeys orders keys so order-dependent attributes (TITLEn, ADDLEAFn) apply in
// sequence; Go map iteration is randomized, which otherwise builds a different tree per run.
func sortAttributeKeys(m map[string]string) []string {
	keys := make([]string, 0, len(m))
	for k := range m {
		keys = append(keys, k)
	}
	sort.Slice(keys, func(i, j int) bool { return compareAttributeKeys(keys[i], keys[j]) })
	return keys
}

func compareAttributeKeys(a, b string) bool {
	aPrefix, aNum := splitAttributeKey(a)
	bPrefix, bNum := splitAttributeKey(b)
	if aNum >= 0 && bNum >= 0 {
		aPrio := getAttributePriority(aPrefix)
		bPrio := getAttributePriority(bPrefix)
		if aPrio != bPrio {
			return aPrio < bPrio
		}
		if aNum != bNum {
			return aNum < bNum
		}
	}
	return a < b
}

func splitAttributeKey(key string) (string, int) {
	i := len(key) - 1
	for i >= 0 && key[i] >= '0' && key[i] <= '9' {
		i--
	}
	if i < len(key)-1 {
		num, _ := strconv.Atoi(key[i+1:])
		return key[:i+1], num
	}
	return key, -1
}

func getAttributePriority(prefix string) int {
	switch strings.ToUpper(prefix) {
	case "NAME":
		return 0
	case "TITLE":
		return 1
	case "ADDBRANCH", "ADDLEAF", "INSERTBRANCH", "INSERTLEAF":
		return 2
	default:
		return 3
	}
}

// SetAttribute sets an interface element attribute.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattribute.md
func (ih Ihandle) SetAttribute(name string, value ...interface{}) Ihandle {
	switch len(value) {
	case 1:
		SetAttribute(ih, name, value[0])
	case 2:
		SetAttribute(ih, name, fmt.Sprintf("%vx%v", value[0], value[1]))
	case 3:
		SetAttribute(ih, name, [3]byte{value[0].(byte), value[1].(byte), value[2].(byte)})
	default:
		panic("bad argument passed to SetAttribute")
	}
	return ih
}

// GetAttribute returns an interface element attribute value.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getattribute.md
func (ih Ihandle) GetAttribute(name string) string {
	return GetAttribute(ih, name)
}

// GetInt returns an attribute value as an integer.
func (ih Ihandle) GetInt(name string) int {
	return GetInt(ih, name)
}

// Destroy destroys the element and its children.
func (ih Ihandle) Destroy() {
	Destroy(ih)
}

// GetFloat returns an attribute value as a float32.
func (ih Ihandle) GetFloat(name string) float32 {
	return GetFloat(ih, name)
}

// SetHandle associates a name with an interface element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_sethandle.md
func (ih Ihandle) SetHandle(name string) Ihandle {
	return SetHandle(name, ih)
}

// SetCallback associates a callback with an event.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setcallback.md
func (ih Ihandle) SetCallback(name string, fn interface{}) Ihandle {
	SetCallback(ih, name, fn)
	return ih
}

func GetPtr(ih Ihandle, name string) uintptr {
	return uintptr(ccall("IupGetAttribute", "number", []interface{}{"number", "string"}, []interface{}{int(ih), name}).Int())
}

func GetPtrId(ih Ihandle, name string, id int) uintptr {
	return uintptr(ccall("IupGetAttributeId", "number", []interface{}{"number", "string", "number"}, []interface{}{int(ih), name, id}).Int())
}

func GetPtrId2(ih Ihandle, name string, lin, col int) uintptr {
	return uintptr(ccall("IupGetAttributeId2", "number", []interface{}{"number", "string", "number", "number"}, []interface{}{int(ih), name, lin, col}).Int())
}

func (ih Ihandle) ResetAttribute(name string) Ihandle {
	ResetAttribute(ih, name)
	return ih
}

func (ih Ihandle) GetAllAttributes() []string {
	return GetAllAttributes(ih)
}

func (ih Ihandle) GetDouble(name string, ids ...interface{}) float64 {
	switch len(ids) {
	case 0:
		return GetDouble(ih, name)
	case 1:
		return GetDoubleId(ih, name, ids[0].(int))
	case 2:
		return GetDoubleId2(ih, name, ids[0].(int), ids[1].(int))
	default:
		panic("bad arguments passed to GetDouble")
	}
}

func (ih Ihandle) GetRGB(name string, ids ...interface{}) (r, g, b uint8) {
	switch len(ids) {
	case 0:
		return GetRGB(ih, name)
	case 1:
		return GetRGBId(ih, name, ids[0].(int))
	case 2:
		return GetRGBId2(ih, name, ids[0].(int), ids[1].(int))
	default:
		panic("bad arguments passed to GetRGB")
	}
}

func (ih Ihandle) GetBool(name string, ids ...interface{}) bool {
	switch len(ids) {
	case 0:
		return GetBool(ih, name)
	case 1:
		return GetBoolId(ih, name, ids[0].(int))
	case 2:
		return GetBoolId2(ih, name, ids[0].(int), ids[1].(int))
	default:
		panic("bad arguments passed to GetBool")
	}
}

func (ih Ihandle) SetBool(name string, value bool, ids ...interface{}) Ihandle {
	switch len(ids) {
	case 0:
		SetBool(ih, name, value)
	case 1:
		SetBoolId(ih, name, ids[0].(int), value)
	case 2:
		SetBoolId2(ih, name, ids[0].(int), ids[1].(int), value)
	default:
		panic("bad arguments passed to SetBool")
	}
	return ih
}

func (ih Ihandle) GetPtr(name string, ids ...interface{}) uintptr {
	switch len(ids) {
	case 0:
		return GetPtr(ih, name)
	case 1:
		return GetPtrId(ih, name, ids[0].(int))
	case 2:
		return GetPtrId2(ih, name, ids[0].(int), ids[1].(int))
	default:
		panic(fmt.Errorf("bad arguments passed to iup.Ihandle.GetPtr"))
	}
}

func (ih Ihandle) GetCallback(name string) uintptr {
	return GetCallback(ih, name)
}
