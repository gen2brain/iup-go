package iup

import (
	"fmt"
	"sort"
	"strings"
	"unsafe"
)

/*
#include <stdint.h>
#include <stdlib.h>
#include "iup.h"

static uintptr_t ih(Ihandle* i) {
        return (uintptr_t)(i);
}

static Ihandle* pih(uintptr_t p) {
	return (Ihandle*)(p);
}

static char* cih(uintptr_t p) {
	return (char*)(p);
}
*/
import "C"

// Ihandle type.
type Ihandle uintptr

func mkih(p *C.Ihandle) Ihandle {
	return Ihandle(C.ih(p))
}

func cih(ih Ihandle) *C.char {
	return C.cih(C.uintptr_t(ih))
}

func (ih Ihandle) ptr() *C.Ihandle {
	return C.pih(C.uintptr_t(ih))
}

// Destroy destroys an interface element and all its children.
// Only dialogs, timers, popup menus and images should be normally destroyed, but detached controls can also be destroyed.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupdestroy.html
func (ih Ihandle) Destroy() {
	Destroy(ih)
}

// ResetAttribute removes an attribute from the hash table of the element, and its children if the attribute is inheritable.
// It is useful to reset the state of inheritable attributes in a tree of elements.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupresetattribute.html
func (ih Ihandle) ResetAttribute(name string) Ihandle {
	ResetAttribute(ih, name)
	return ih
}

// GetAllAttributes returns the names of all attributes of an element that are set in its internal hash table only.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetallattributes.html
func (ih Ihandle) GetAllAttributes() []string {
	return GetAllAttributes(ih)
}

// SetAttributes sets several attributes of an interface element.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattributes.html
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

// compareAttributeKeys compares attribute keys for sorting.
// Tree attributes (TITLE, ADDLEAF, ADDBRANCH, etc.) are sorted numerically by their ID suffix.
// Other attributes are sorted alphabetically.
func compareAttributeKeys(a, b string) bool {
	// Extract prefix and numeric suffix for both keys
	aPrefix, aNum := splitAttributeKey(a)
	bPrefix, bNum := splitAttributeKey(b)

	// If both are tree-related attributes with numeric suffixes
	if aNum >= 0 && bNum >= 0 {
		// First sort by priority: TITLE < ADDLEAF/ADDBRANCH
		aPrio := getAttributePriority(aPrefix)
		bPrio := getAttributePriority(bPrefix)
		if aPrio != bPrio {
			return aPrio < bPrio
		}
		// Then sort by numeric ID
		if aNum != bNum {
			return aNum < bNum
		}
	}

	// Default: alphabetical sort
	return a < b
}

// splitAttributeKey splits an attribute key into prefix and numeric suffix.
// Returns (prefix, -1) if no numeric suffix is found.
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

// getAttributePriority returns priority for tree attribute ordering.
// Lower numbers are processed first.
func getAttributePriority(prefix string) int {
	switch strings.ToUpper(prefix) {
	case "NAME":
		return 0 // NAME attributes first
	case "TITLE":
		return 1 // TITLE attributes second
	case "ADDBRANCH", "ADDLEAF", "INSERTBRANCH", "INSERTLEAF":
		return 2 // ADD/INSERT attributes third
	default:
		return 3 // Everything else last
	}
}

// SetAttribute sets an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetattribute.html
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

// GetAttribute returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
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

// GetInt returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
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

// GetFloat returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
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

// GetDouble returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
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

// GetRGB returns the name of an interface element attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
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

// GetBool returns a boolean attribute value.
// Returns true for "YES", "ON", "TRUE", "1" (case insensitive), false otherwise.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetattribute.html
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

// GetPtr .
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

// GetPtr .
func GetPtr(ih Ihandle, name string) uintptr {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return uintptr(unsafe.Pointer(C.IupGetAttribute(ih.ptr(), cName)))
}

// GetPtrId .
func GetPtrId(ih Ihandle, name string, id int) uintptr {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return uintptr(unsafe.Pointer(C.IupGetAttributeId(ih.ptr(), cName, C.int(id))))
}

// GetPtrId2 .
func GetPtrId2(ih Ihandle, name string, lin, col int) uintptr {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return uintptr(unsafe.Pointer(C.IupGetAttributeId2(ih.ptr(), cName, C.int(lin), C.int(col))))
}

// SetCallback associates a callback to an event.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetcallback.html
func (ih Ihandle) SetCallback(name string, fn interface{}) Ihandle {
	SetCallback(ih, name, fn)
	return ih
}

// GetCallback returns the callback associated to an event.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetcallback.html
func (ih Ihandle) GetCallback(name string) uintptr {
	return GetCallback(ih, name)
}

// SetHandle associates a name with an interface element.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsethandle.html
func (ih Ihandle) SetHandle(name string) Ihandle {
	SetHandle(name, ih)
	return ih
}
