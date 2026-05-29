package iup

/*
#include "iup.h"
*/
import "C"

// Constants
const (
	NAME           = C.IUP_NAME
	DESCRIPTION    = C.IUP_DESCRIPTION
	COPYRIGHT      = C.IUP_COPYRIGHT
	VERSION        = C.IUP_VERSION
	VERSION_NUMBER = C.IUP_VERSION_NUMBER
	VERSION_DATE   = C.IUP_VERSION_DATE
)

// Common flags and return values
const (
	ERROR      = 1
	NOERROR    = 0
	OPENED     = -1
	INVALID    = -1
	INVALID_ID = -10
)

// Callback return values
const (
	IGNORE   = -1
	DEFAULT  = -2
	CLOSE    = -3
	CONTINUE = -4
)

// Popup and ShowXY parameter values
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

// SHOW_CB callback values
const (
	SHOW = iota
	RESTORE
	MINIMIZE
	MAXIMIZE
	HIDE
)

// SCROLL_CB callback values
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

// Mouse button values and macros
const (
	BUTTON1 = '1'
	BUTTON2 = '2'
	BUTTON3 = '3'
	BUTTON4 = '4'
	BUTTON5 = '5'
)

// Pre-defined masks
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
	GESTURE_PINCH     = C.IUP_GESTURE_PINCH
	GESTURE_ROTATE    = C.IUP_GESTURE_ROTATE
	GESTURE_PAN       = C.IUP_GESTURE_PAN
	GESTURE_SWIPE     = C.IUP_GESTURE_SWIPE
	GESTURE_TAP       = C.IUP_GESTURE_TAP
	GESTURE_LONGPRESS = C.IUP_GESTURE_LONGPRESS
)

// GESTURE_CB states
const (
	GESTURE_BEGIN   = C.IUP_GESTURE_BEGIN
	GESTURE_CHANGED = C.IUP_GESTURE_CHANGED
	GESTURE_END     = C.IUP_GESTURE_END
	GESTURE_CANCEL  = C.IUP_GESTURE_CANCEL
)

// GESTURE_CB swipe directions
const (
	GESTURE_SWIPE_RIGHT = C.IUP_GESTURE_SWIPE_RIGHT
	GESTURE_SWIPE_LEFT  = C.IUP_GESTURE_SWIPE_LEFT
	GESTURE_SWIPE_UP    = C.IUP_GESTURE_SWIPE_UP
	GESTURE_SWIPE_DOWN  = C.IUP_GESTURE_SWIPE_DOWN
)
