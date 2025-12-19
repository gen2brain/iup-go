//go:build windows && !gtk && !gtk4 && !qt

package iup

/*
#include "external/src/win/iupwin_frame.c"
*/
import "C"