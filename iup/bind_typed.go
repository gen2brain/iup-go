package iup

import (
	"fmt"
	"image/color"
	"strconv"
	"strings"
)

// GetGlobalInt returns a global attribute as an integer, 0 when it is not a number.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getglobal.md
func GetGlobalInt(name string) int {
	return strToInt(GetGlobal(name))
}

// GetGlobalBool returns a global attribute as a boolean.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getglobal.md
func GetGlobalBool(name string) bool {
	return strToBool(GetGlobal(name))
}

// GetGlobalIntInt returns a global attribute holding two integers and how many were found.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getglobal.md
func GetGlobalIntInt(name string) (count, i1, i2 int) {
	return strToIntInt(GetGlobal(name))
}

// GetGlobalRGB returns a global attribute holding a color in the "r g b" or "#rrggbb" format.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getglobal.md
func GetGlobalRGB(name string) (r, g, b uint8) {
	return strToRGB(GetGlobal(name))
}

// SetAttributeId sets an interface element attribute for an id.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattribute.md
func (ih Ihandle) SetAttributeId(name string, id int, value interface{}) Ihandle {
	SetAttributeId(ih, name, id, value)
	return ih
}

// SetAttributeId2 sets an interface element attribute for a (lin, col) position.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattribute.md
func (ih Ihandle) SetAttributeId2(name string, lin, col int, value interface{}) Ihandle {
	SetAttributeId2(ih, name, lin, col, value)
	return ih
}

// SetAttributeHandle sets an attribute to the name of another interface element.
//
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattributehandle.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getattributehandle.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_setattribute.md
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
// https://github.com/gen2brain/iup-go/blob/main/docs/func/iup_getattribute.md
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

func attribValueString(value interface{}) (string, bool) {
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

func rgbBytes(value []interface{}) ([3]uint8, bool) {
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
