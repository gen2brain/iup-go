//go:build windows && !gtk && !gtk4

package iup

/*
#include "external/src/win/iupwin_thread.c"
*/
import "C"
