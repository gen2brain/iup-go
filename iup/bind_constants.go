package iup

/*
#include <iup.h>
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
