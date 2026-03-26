//go:build windows && !gtk && !gtk4 && !winui

package iup

/*
#include "external/src/win/iupwin_tray.c"
*/
import "C"
