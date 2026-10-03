package iup

import (
	"fmt"
	"image/color"
	"strconv"
	"strings"
)

// GetGlobalInt returns a global attribute as an integer, 0 when it is not a number.
//
// https://gen2brain.github.io/iup-go/func/iup_getglobal.html
func GetGlobalInt(name string) int {
	return strToInt(GetGlobal(name))
}

// GetGlobalBool returns a global attribute as a boolean.
//
// https://gen2brain.github.io/iup-go/func/iup_getglobal.html
func GetGlobalBool(name string) bool {
	return strToBool(GetGlobal(name))
}

// GetGlobalIntInt returns a global attribute holding two integers and how many were found.
//
// https://gen2brain.github.io/iup-go/func/iup_getglobal.html
func GetGlobalIntInt(name string) (count, i1, i2 int) {
	return strToIntInt(GetGlobal(name))
}

// GetGlobalRGB returns a global attribute holding a color in the "r g b" or "#rrggbb" format.
//
// https://gen2brain.github.io/iup-go/func/iup_getglobal.html
func GetGlobalRGB(name string) (r, g, b uint8) {
	return strToRGB(GetGlobal(name))
}

// SetAttributeId sets an interface element attribute for an id.
//
// https://gen2brain.github.io/iup-go/func/iup_setattribute.html
func (ih Ihandle) SetAttributeId(name string, id int, value any) Ihandle {
	SetAttributeId(ih, name, id, value)
	return ih
}

// SetAttributeId2 sets an interface element attribute for a (lin, col) position.
//
// https://gen2brain.github.io/iup-go/func/iup_setattribute.html
func (ih Ihandle) SetAttributeId2(name string, lin, col int, value any) Ihandle {
	SetAttributeId2(ih, name, lin, col, value)
	return ih
}

// SetAttributeHandle sets an attribute to the name of another interface element.
//
// https://gen2brain.github.io/iup-go/func/iup_setattributehandle.html
func (ih Ihandle) SetAttributeHandle(name string, ihNamed Ihandle, ids ...int) Ihandle {
	switch len(ids) {
	case 0:
		SetAttributeHandle(ih, name, ihNamed)
	case 1:
		SetAttributeHandleId(ih, name, ids[0], ihNamed)
	case 2:
		SetAttributeHandleId2(ih, name, ids[0], ids[1], ihNamed)
	default:
		panic("bad arguments passed to SetAttributeHandle")
	}
	return ih
}

// GetAttributeHandle returns the interface element named by an attribute.
//
// https://gen2brain.github.io/iup-go/func/iup_getattributehandle.html
func (ih Ihandle) GetAttributeHandle(name string, ids ...int) Ihandle {
	switch len(ids) {
	case 0:
		return GetAttributeHandle(ih, name)
	case 1:
		return GetAttributeHandleId(ih, name, ids[0])
	case 2:
		return GetAttributeHandleId2(ih, name, ids[0], ids[1])
	default:
		panic("bad arguments passed to GetAttributeHandle")
	}
}

// SetRGB sets a color attribute. Optional ids select an id or a (lin, col) position.
//
// https://gen2brain.github.io/iup-go/func/iup_setattribute.html
func (ih Ihandle) SetRGB(name string, r, g, b uint8, ids ...int) Ihandle {
	switch len(ids) {
	case 0:
		SetRGB(ih, name, r, g, b)
	case 1:
		SetRGBId(ih, name, ids[0], r, g, b)
	case 2:
		SetRGBId2(ih, name, ids[0], ids[1], r, g, b)
	default:
		panic("bad arguments passed to SetRGB")
	}
	return ih
}

// GetIntInt returns an attribute holding two integers and how many were found.
//
// https://gen2brain.github.io/iup-go/func/iup_getattribute.html
func (ih Ihandle) GetIntInt(name string) (count, i1, i2 int) {
	return GetInt2(ih, name)
}

func strToInt(s string) int {
	s = strings.TrimSpace(s)
	end := 0
	if end < len(s) && (s[end] == '-' || s[end] == '+') {
		end++
	}
	for end < len(s) && s[end] >= '0' && s[end] <= '9' {
		end++
	}
	if n, err := strconv.Atoi(s[:end]); err == nil {
		return n
	}
	if strToBool(s) {
		return 1
	}
	return 0
}

func strToBool(s string) bool {
	switch strings.ToUpper(strings.TrimSpace(s)) {
	case "YES", "ON", "TRUE", "1":
		return true
	}
	return false
}

func strToIntInt(s string) (count, i1, i2 int) {
	sep := "x"
	if strings.Contains(s, ":") {
		sep = ":"
	} else if strings.Contains(s, ",") {
		sep = ","
	}
	parts := strings.SplitN(strings.ToLower(s), sep, 2)
	if v, err := strconv.Atoi(strings.TrimSpace(parts[0])); err == nil {
		i1 = v
		count++
	}
	if len(parts) == 2 {
		if v, err := strconv.Atoi(strings.TrimSpace(parts[1])); err == nil {
			i2 = v
			count++
		}
	}
	return
}

func strToRGB(s string) (r, g, b uint8) {
	s = strings.TrimSpace(s)
	if strings.HasPrefix(s, "#") && len(s) >= 7 {
		if v, err := strconv.ParseUint(s[1:7], 16, 32); err == nil {
			return uint8(v >> 16), uint8(v >> 8), uint8(v)
		}
		return 0, 0, 0
	}
	var c [3]uint8
	for i, f := range strings.Fields(s) {
		if i == 3 {
			break
		}
		v, err := strconv.ParseUint(f, 10, 8)
		if err != nil {
			return 0, 0, 0
		}
		c[i] = uint8(v)
	}
	return c[0], c[1], c[2]
}

func attribValueString(value any) (string, bool) {
	switch v := value.(type) {
	case bool:
		if v {
			return "YES", true
		}
		return "NO", true
	case int, int8, int16, int32, int64, uint, uint8, uint16, uint32, uint64:
		return fmt.Sprintf("%d", v), true
	case float32:
		return strconv.FormatFloat(float64(v), 'g', -1, 32), true
	case float64:
		return strconv.FormatFloat(v, 'g', -1, 64), true
	case [3]uint8:
		return fmt.Sprintf("%d %d %d", v[0], v[1], v[2]), true
	case [4]uint8:
		return fmt.Sprintf("%d %d %d %d", v[0], v[1], v[2], v[3]), true
	case color.RGBA:
		return fmt.Sprintf("%d %d %d %d", v.R, v.G, v.B, v.A), true
	case color.NRGBA:
		return fmt.Sprintf("%d %d %d %d", v.R, v.G, v.B, v.A), true
	}
	return "", false
}

func rgbBytes(value []any) ([3]uint8, bool) {
	var c [3]uint8
	for i, v := range value {
		switch n := v.(type) {
		case uint8:
			c[i] = n
		case int:
			c[i] = uint8(n)
		case int32:
			c[i] = uint8(n)
		case int64:
			c[i] = uint8(n)
		case uint:
			c[i] = uint8(n)
		case uint32:
			c[i] = uint8(n)
		default:
			return c, false
		}
	}
	return c, true
}

// GetBytes returns a copy of a clipboard data attribute (FORMATDATA, NATIVEVECTORIMAGE), FORMATDATASIZE bytes long.
//
// https://gen2brain.github.io/iup-go/elem/iup_clipboard.html
func (ih Ihandle) GetBytes(name string) []byte {
	return GetBytes(ih, name)
}

// SetBytes sets FORMATDATASIZE to the data length and stores data in a clipboard data attribute (FORMATDATA, NATIVEVECTORIMAGE).
//
// https://gen2brain.github.io/iup-go/elem/iup_clipboard.html
func (ih Ihandle) SetBytes(name string, data []byte) Ihandle {
	SetBytes(ih, name, data)
	return ih
}

func setTypedFunction(name string, fn any, unset bool) {
	if unset {
		SetFunction(name, nil)
		return
	}
	SetFunction(name, fn)
}

// SetIdleFunc sets the IDLE_ACTION global callback. A nil fn removes it.
//
// https://gen2brain.github.io/iup-go/call/iup_idle_action.html
func SetIdleFunc(fn IdleFunc) { setTypedFunction("IDLE_ACTION", fn, fn == nil) }

// SetExitFunc sets the EXIT_CB global callback. A nil fn removes it.
//
// https://gen2brain.github.io/iup-go/call/iup_exit_cb.html
func SetExitFunc(fn ExitFunc) { setTypedFunction("EXIT_CB", fn, fn == nil) }

// SetGlobalKeyPressFunc sets the GLOBALKEYPRESS_CB global callback. A nil fn removes it.
// The callback is called only when the INPUTCALLBACKS global attribute is YES.
//
// https://gen2brain.github.io/iup-go/attrib/iup_globals.html#inputcallbacks
func SetGlobalKeyPressFunc(fn GlobalKeyPressFunc) {
	setTypedFunction("GLOBALKEYPRESS_CB", fn, fn == nil)
}

// SetGlobalButtonFunc sets the GLOBALBUTTON_CB global callback. A nil fn removes it.
// The callback is called only when the INPUTCALLBACKS global attribute is YES.
//
// https://gen2brain.github.io/iup-go/attrib/iup_globals.html#inputcallbacks
func SetGlobalButtonFunc(fn GlobalButtonFunc) {
	setTypedFunction("GLOBALBUTTON_CB", fn, fn == nil)
}

// SetGlobalMotionFunc sets the GLOBALMOTION_CB global callback. A nil fn removes it.
// The callback is called only when the INPUTCALLBACKS global attribute is YES.
//
// https://gen2brain.github.io/iup-go/attrib/iup_globals.html#inputcallbacks
func SetGlobalMotionFunc(fn GlobalMotionFunc) {
	setTypedFunction("GLOBALMOTION_CB", fn, fn == nil)
}

// SetGlobalWheelFunc sets the GLOBALWHEEL_CB global callback. A nil fn removes it.
// The callback is called only when the INPUTCALLBACKS global attribute is YES.
//
// https://gen2brain.github.io/iup-go/attrib/iup_globals.html#inputcallbacks
func SetGlobalWheelFunc(fn GlobalWheelFunc) {
	setTypedFunction("GLOBALWHEEL_CB", fn, fn == nil)
}

// SetGlobalEnterModalFunc sets the GLOBALENTERMODAL_CB global callback. A nil fn removes it.
func SetGlobalEnterModalFunc(fn GlobalEnterModalFunc) {
	setTypedFunction("GLOBALENTERMODAL_CB", fn, fn == nil)
}

// SetGlobalLeaveModalFunc sets the GLOBALLEAVEMODAL_CB global callback. A nil fn removes it.
func SetGlobalLeaveModalFunc(fn GlobalLeaveModalFunc) {
	setTypedFunction("GLOBALLEAVEMODAL_CB", fn, fn == nil)
}
