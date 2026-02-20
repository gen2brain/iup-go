//go:build windows && !gtk && !gtk4 && !qt && !winui

package iup

/*
#include "external/src/win/iupwin_popover.c"
*/
import "C"
