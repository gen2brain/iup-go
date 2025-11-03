//go:build windows && !gtk && !qt

package iup

/*
#include "external/src/win/iupwin_loop.c"
*/
import "C"