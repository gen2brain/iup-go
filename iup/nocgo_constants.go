//go:build !cgo && !js

package iup

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
	IGNORE   = -1
	DEFAULT  = -2
	CLOSE    = -3
	CONTINUE = -4
)

const (
	CENTER       = 0xFFFF
	LEFT         = 0xFFFE
	RIGHT        = 0xFFFD
	MOUSEPOS     = 0xFFFC
	CURRENT      = 0xFFFB
	CENTERPARENT = 0xFFFA
	LEFTPARENT   = 0xFFF9
	RIGHTPARENT  = 0xFFF8
	TOP          = LEFT
	BOTTOM       = RIGHT
	TOPPARENT    = LEFTPARENT
	BOTTOMPARENT = RIGHTPARENT
)

const (
	SHOW = iota
	RESTORE
	MINIMIZE
	MAXIMIZE
	HIDE
)

const (
	SBUP = iota
	SBDN
	SBPGUP
	SBPGDN
	SBPOSV
	SBDRAGV
	SBLEFT
	SBRIGHT
	SBPGLEFT
	SBPGRIGHT
	SBPOSH
	SBDRAGH
)

const (
	BUTTON1 = '1'
	BUTTON2 = '2'
	BUTTON3 = '3'
	BUTTON4 = '4'
	BUTTON5 = '5'
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

const (
	PRIMARY   = -1
	SECONDARY = -2
)

const (
	RECBINARY = iota
	RECTEXT
)

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

const (
	GESTURE_PINCH = iota
	GESTURE_ROTATE
	GESTURE_PAN
	GESTURE_SWIPE
	GESTURE_TAP
	GESTURE_LONGPRESS
)

const (
	GESTURE_BEGIN = iota
	GESTURE_CHANGED
	GESTURE_END
	GESTURE_CANCEL
)

const (
	GESTURE_SWIPE_RIGHT = iota
	GESTURE_SWIPE_LEFT
	GESTURE_SWIPE_UP
	GESTURE_SWIPE_DOWN
)

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

const (
	GlobalNormal   = 0
	GlobalReadOnly = 1
	GlobalPointer  = 2
)

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
