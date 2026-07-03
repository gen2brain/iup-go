//go:build !cgo && !js

package iup

import (
	"bytes"
	"fmt"
	"image/color"
	"reflect"
	"runtime"
	"strings"
	"sync"
	"sync/atomic"
)

var attrMu reentrantRWMutex

type reentrantRWMutex struct {
	mu      sync.RWMutex
	owner   atomic.Int64
	recurse int32
}

func goroutineID() int64 {
	var buf [64]byte
	n := runtime.Stack(buf[:], false)
	var id int64
	for i := len("goroutine "); i < n; i++ {
		if buf[i] < '0' || buf[i] > '9' {
			break
		}
		id = id*10 + int64(buf[i]-'0')
	}
	return id
}

func (m *reentrantRWMutex) Lock() {
	gid := goroutineID()
	if m.owner.Load() == gid {
		m.recurse++
		return
	}
	m.mu.Lock()
	m.owner.Store(gid)
	m.recurse = 1
}

func (m *reentrantRWMutex) Unlock() {
	m.recurse--
	if m.recurse == 0 {
		m.owner.Store(0)
		m.mu.Unlock()
	}
}

func (m *reentrantRWMutex) RLock() {
	gid := goroutineID()
	if m.owner.Load() == gid {
		m.recurse++
		return
	}
	m.mu.RLock()
}

func (m *reentrantRWMutex) RUnlock() {
	gid := goroutineID()
	if m.owner.Load() == gid {
		m.recurse--
		if m.recurse == 0 {
			m.owner.Store(0)
			m.mu.Unlock()
		}
		return
	}
	m.mu.RUnlock()
}

func SetAttribute(ih Ihandle, name string, value interface{}) {
	attrMu.Lock()
	defer attrMu.Unlock()

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
	attrMu.Lock()
	defer attrMu.Unlock()

	return mkih(iupSetAttributes(uintptr(ih), str))
}

func ResetAttribute(ih Ihandle, name string) {
	attrMu.Lock()
	defer attrMu.Unlock()

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
	attrMu.Lock()
	defer attrMu.Unlock()

	iupSetAttributeHandle(uintptr(ih), name, uintptr(ihNamed))
}

func GetAttribute(ih Ihandle, name string) string {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetAttribute(uintptr(ih), name)
}

func GetAllAttributes(ih Ihandle) (ret []string) {
	attrMu.RLock()
	defer attrMu.RUnlock()

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
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetAttributes(uintptr(ih))
}

func GetAttributeHandle(ih Ihandle, name string) Ihandle {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return mkih(iupGetAttributeHandle(uintptr(ih), name))
}

func SetAttributeHandleId(ih Ihandle, name string, id int, ihNamed Ihandle) {
	attrMu.Lock()
	defer attrMu.Unlock()

	iupSetAttributeHandleId(uintptr(ih), name, int32(id), uintptr(ihNamed))
}

func GetAttributeHandleId(ih Ihandle, name string, id int) Ihandle {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return mkih(iupGetAttributeHandleId(uintptr(ih), name, int32(id)))
}

func SetAttributeHandleId2(ih Ihandle, name string, lin, col int, ihNamed Ihandle) {
	attrMu.Lock()
	defer attrMu.Unlock()

	iupSetAttributeHandleI2(uintptr(ih), name, int32(lin), int32(col), uintptr(ihNamed))
}

func GetAttributeHandleId2(ih Ihandle, name string, lin, col int) Ihandle {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return mkih(iupGetAttributeHandleI2(uintptr(ih), name, int32(lin), int32(col)))
}

func SetAttributeId(ih Ihandle, name string, id int, value interface{}) {
	attrMu.Lock()
	defer attrMu.Unlock()

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
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetAttributeId(uintptr(ih), name, int32(id))
}

func SetAttributeId2(ih Ihandle, name string, lin, col int, value interface{}) {
	attrMu.Lock()
	defer attrMu.Unlock()

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
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetAttributeId2(uintptr(ih), name, int32(lin), int32(col))
}

func SetGlobal(name string, value interface{}) {
	attrMu.Lock()
	defer attrMu.Unlock()

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
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetGlobal(name)
}

func GetGlobalPtr(name string) uintptr {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetGlobalRaw(name)
}

func GetGlobalIh(name string) Ihandle {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return Ihandle(iupGetGlobalRaw(name))
}

func StringCompare(str1, str2 string, caseSensitive, lexicographic bool) int {
	return int(iupStringCompare(str1, str2, int32(bool2int(caseSensitive)), int32(bool2int(lexicographic))))
}

func SetRGB(ih Ihandle, name string, r, g, b uint8) {
	attrMu.Lock()
	defer attrMu.Unlock()

	iupSetRGB(uintptr(ih), name, r, g, b)
}

func SetRGBA(ih Ihandle, name string, r, g, b, a uint8) {
	attrMu.Lock()
	defer attrMu.Unlock()

	iupSetRGBA(uintptr(ih), name, r, g, b, a)
}

func SetRGBId(ih Ihandle, name string, id int, r, g, b uint8) {
	attrMu.Lock()
	defer attrMu.Unlock()

	iupSetRGBId(uintptr(ih), name, int32(id), r, g, b)
}

func SetRGBId2(ih Ihandle, name string, lin, col int, r, g, b uint8) {
	attrMu.Lock()
	defer attrMu.Unlock()

	iupSetRGBId2(uintptr(ih), name, int32(lin), int32(col), r, g, b)
}

func GetInt(ih Ihandle, name string) int {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return int(iupGetInt(uintptr(ih), name))
}

func GetInt2(ih Ihandle, name string) (count, i1, i2 int) {
	attrMu.RLock()
	defer attrMu.RUnlock()

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
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetFloat(uintptr(ih), name)
}

func GetDouble(ih Ihandle, name string) float64 {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetDouble(uintptr(ih), name)
}

func GetRGB(ih Ihandle, name string) (r, g, b uint8) {
	attrMu.RLock()
	defer attrMu.RUnlock()

	iupGetRGB(uintptr(ih), name, &r, &g, &b)
	return
}

func GetRGBA(ih Ihandle, name string) (r, g, b, a uint8) {
	attrMu.RLock()
	defer attrMu.RUnlock()

	iupGetRGBA(uintptr(ih), name, &r, &g, &b, &a)
	return
}

func GetIntId(ih Ihandle, name string, id int) int {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return int(iupGetIntId(uintptr(ih), name, int32(id)))
}

func GetFloatId(ih Ihandle, name string, id int) float32 {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetFloatId(uintptr(ih), name, int32(id))
}

func GetDoubleId(ih Ihandle, name string, id int) float64 {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetDoubleId(uintptr(ih), name, int32(id))
}

func GetRGBId(ih Ihandle, name string, id int) (r, g, b uint8) {
	attrMu.RLock()
	defer attrMu.RUnlock()

	iupGetRGBId(uintptr(ih), name, int32(id), &r, &g, &b)
	return
}

func GetBoolId(ih Ihandle, name string, id int) bool {
	val := strings.ToUpper(GetAttributeId(ih, name, id))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}

func GetIntId2(ih Ihandle, name string, lin, col int) int {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return int(iupGetIntId2(uintptr(ih), name, int32(lin), int32(col)))
}

func GetFloatId2(ih Ihandle, name string, lin, col int) float32 {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetFloatId2(uintptr(ih), name, int32(lin), int32(col))
}

func GetDoubleId2(ih Ihandle, name string, lin, col int) float64 {
	attrMu.RLock()
	defer attrMu.RUnlock()

	return iupGetDoubleId2(uintptr(ih), name, int32(lin), int32(col))
}

func GetRGBId2(ih Ihandle, name string, lin, col int) (r, g, b uint8) {
	attrMu.RLock()
	defer attrMu.RUnlock()

	iupGetRGBId2(uintptr(ih), name, int32(lin), int32(col), &r, &g, &b)
	return
}

func GetBoolId2(ih Ihandle, name string, lin, col int) bool {
	val := strings.ToUpper(GetAttributeId2(ih, name, lin, col))
	return val == "YES" || val == "ON" || val == "TRUE" || val == "1"
}
