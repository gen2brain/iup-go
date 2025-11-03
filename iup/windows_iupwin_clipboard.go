//go:build windows && !gtk && !qt

package iup

/*
#include "external/src/win/iupwin_clipboard.c"
*/
import "C"