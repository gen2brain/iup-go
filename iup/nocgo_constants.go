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
	AF_DEFAULT         = 0
	AF_NO_INHERIT      = 1
	AF_NO_DEFAULTVALUE = 2
	AF_NO_STRING       = 4
	AF_NOT_MAPPED      = 8
	AF_HAS_ID          = 16
	AF_READONLY        = 32
	AF_WRITEONLY       = 64
	AF_HAS_ID2         = 128
	AF_CALLBACK        = 256
	AF_NO_SAVE         = 512
	AF_NOT_SUPPORTED   = 1024
	AF_IHANDLENAME     = 2048
	AF_IHANDLE         = 4096
)

const (
	GF_NORMAL    = 0
	GF_READONLY  = 1
	GF_POINTER   = 2
	GF_WRITEONLY = 4
)

const (
	DRV_WIN        = 1
	DRV_MOTIF      = 2
	DRV_GTK        = 4
	DRV_COCOA      = 8
	DRV_QT         = 16
	DRV_GTK4       = 32
	DRV_EFL        = 64
	DRV_WINUI      = 128
	DRV_FLTK       = 256
	DRV_ANDROID    = 512
	DRV_COCOATOUCH = 1024
	DRV_HAIKU      = 2048
	DRV_WASM       = 4096
	DRV_QML        = 8192
)

// DrawPathFill fill rules
const (
	DRAW_RULE_WINDING = 0
	DRAW_RULE_EVENODD = 1
)
