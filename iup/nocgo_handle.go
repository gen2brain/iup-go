//go:build !cgo && !js

package iup

import (
	"fmt"
	"sort"
	"strings"
)

type Ihandle uintptr

func mkih(p uintptr) Ihandle {
	return Ihandle(p)
}

func (ih Ihandle) Destroy() {
	Destroy(ih)
}

func (ih Ihandle) ResetAttribute(name string) Ihandle {
	ResetAttribute(ih, name)
	return ih
}

func (ih Ihandle) GetAllAttributes() []string {
	return GetAllAttributes(ih)
}

func (ih Ihandle) SetAttributes(params ...interface{}) Ihandle {
	for _, param := range params {
		switch param := param.(type) {
		case string:
			SetAttributes(ih, param)
		case map[string]string:
			keys := make([]string, 0, len(param))
			for key := range param {
				keys = append(keys, key)
			}
			sort.Slice(keys, func(i, j int) bool {
				return compareAttributeKeys(keys[i], keys[j])
			})
			for _, key := range keys {
				SetAttribute(ih, key, param[key])
			}
		case map[string]interface{}:
			keys := make([]string, 0, len(param))
			for key := range param {
				keys = append(keys, key)
			}
			sort.Slice(keys, func(i, j int) bool {
				return compareAttributeKeys(keys[i], keys[j])
			})
			for _, key := range keys {
				SetAttribute(ih, key, param[key])
			}
		}
	}
	return ih
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
		prefix := key[:i+1]
		var num int
		fmt.Sscanf(key[i+1:], "%d", &num)
		return prefix, num
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

func (ih Ihandle) GetAttribute(name string, ids ...interface{}) string {
	switch len(ids) {
	case 0:
		return GetAttribute(ih, name)
	case 1:
		return GetAttributeId(ih, name, ids[0].(int))
	case 2:
		return GetAttributeId2(ih, name, ids[0].(int), ids[1].(int))
	default:
		panic("bad arguments passed to GetAttribute")
	}
}

func (ih Ihandle) GetInt(name string, ids ...interface{}) int {
	switch len(ids) {
	case 0:
		return GetInt(ih, name)
	case 1:
		return GetIntId(ih, name, ids[0].(int))
	case 2:
		return GetIntId2(ih, name, ids[0].(int), ids[1].(int))
	default:
		panic("bad arguments passed to GetInt")
	}
}

func (ih Ihandle) GetFloat(name string, ids ...interface{}) float32 {
	switch len(ids) {
	case 0:
		return GetFloat(ih, name)
	case 1:
		return GetFloatId(ih, name, ids[0].(int))
	case 2:
		return GetFloatId2(ih, name, ids[0].(int), ids[1].(int))
	default:
		panic("bad arguments passed to GetFloat")
	}
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

func GetPtr(ih Ihandle, name string) uintptr {
	return iupGetAttributeRaw(uintptr(ih), name)
}

func GetPtrId(ih Ihandle, name string, id int) uintptr {
	return iupGetAttributeIdRaw(uintptr(ih), name, int32(id))
}

func GetPtrId2(ih Ihandle, name string, lin, col int) uintptr {
	return iupGetAttributeId2Raw(uintptr(ih), name, int32(lin), int32(col))
}

func (ih Ihandle) SetCallback(name string, fn interface{}) Ihandle {
	SetCallback(ih, name, fn)
	return ih
}

func (ih Ihandle) GetCallback(name string) uintptr {
	return GetCallback(ih, name)
}

func (ih Ihandle) SetHandle(name string) Ihandle {
	SetHandle(name, ih)
	return ih
}
