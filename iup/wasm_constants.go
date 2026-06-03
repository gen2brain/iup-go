//go:build js && wasm

package iup

// Constants mirroring bind_constants.go (cgo macros resolved to literals).

const (
	NAME           = "IUP - Portable User Interface"
	DESCRIPTION    = "Multi-platform Toolkit for Building Graphical User Interfaces"
	COPYRIGHT      = "Copyright (C) 1994-2025 Tecgraf/PUC-Rio"
	VERSION        = "3.32"
	VERSION_NUMBER = 332000
	VERSION_DATE   = "2025/01/06"
)

const (
	ERROR      = 1
	NOERROR    = 0
	OPENED     = -1
	INVALID    = -1
	INVALID_ID = -10
)

const (
	LEFTPARENT   = 0xFFF9
	RIGHTPARENT  = 0xFFF8
	TOPPARENT    = LEFTPARENT
	BOTTOMPARENT = RIGHTPARENT
)

// SHOW_CB callback values
const (
	SHOW = iota
	RESTORE
	MINIMIZE
	MAXIMIZE
	HIDE
)

const (
	MASK_FLOAT       = "[+/-]?(/d+/.?/d*|/./d+)"
	MASK_UFLOAT      = "(/d+/.?/d*|/./d+)"
	MASK_EFLOAT      = "[+/-]?(/d+/.?/d*|/./d+)([eE][+/-]?/d+)?"
	MASK_FLOATCOMMA  = "[+/-]?(/d+/,?/d*|/,/d+)"
	MASK_UFLOATCOMMA = "(/d+/,?/d*|/,/d+)"
	MASK_INT         = "[+/-]?/d+"
	MASK_UINT        = "/d+"
)

// Used by Colorbar
const (
	PRIMARY   = -1
	SECONDARY = -2
)

// Record input modes
const (
	RECBINARY = iota
	RECTEXT
)

// GetParam callback values
const (
	GETPARAM_BUTTON1 = -1
	GETPARAM_INIT    = -2
	GETPARAM_BUTTON2 = -3
	GETPARAM_BUTTON3 = -4
	GETPARAM_CLOSE   = -5
	GETPARAM_MAP     = -6
	GETPARAM_OK      = GETPARAM_BUTTON1
	GETPARAM_CANCEL  = GETPARAM_BUTTON2
	GETPARAM_HELP    = GETPARAM_BUTTON3
)

// GESTURE_CB gesture types
const (
	GESTURE_PINCH = iota
	GESTURE_ROTATE
	GESTURE_PAN
	GESTURE_SWIPE
	GESTURE_TAP
	GESTURE_LONGPRESS
)

// GESTURE_CB states
const (
	GESTURE_BEGIN = iota
	GESTURE_CHANGED
	GESTURE_END
	GESTURE_CANCEL
)

// GESTURE_CB swipe directions
const (
	GESTURE_SWIPE_RIGHT = iota
	GESTURE_SWIPE_LEFT
	GESTURE_SWIPE_UP
	GESTURE_SWIPE_DOWN
)

// Attribute flag bits returned by GetClassAttributeInfo.
const (
	AttribDefault        = 0
	AttribNoInherit      = 1
	AttribNoDefaultValue = 2
	AttribNoString       = 4
	AttribNotMapped      = 8
	AttribHasID          = 16
	AttribReadOnly       = 32
	AttribWriteOnly      = 64
	AttribHasID2         = 128
	AttribCallback       = 256
	AttribNoSave         = 512
	AttribNotSupported   = 1024
	AttribIhandleName    = 2048
	AttribIhandle        = 4096
)

// Flag bits returned by GetGlobalInfo.
const (
	GlobalNormal   = 0
	GlobalReadOnly = 1
	GlobalPointer  = 2
)

// Driver bits returned by GetGlobalInfo.
const (
	DriverWin        = 1
	DriverMotif      = 2
	DriverGTK        = 4
	DriverCocoa      = 8
	DriverQt         = 16
	DriverGTK4       = 32
	DriverEFL        = 64
	DriverWinUI      = 128
	DriverFLTK       = 256
	DriverAndroid    = 512
	DriverCocoaTouch = 1024
	DriverHaiku      = 2048
	DriverWasm       = 4096
)
