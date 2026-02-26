//go:build windows && !gtk && !gtk4 && !qt && !winui

package iup

/*
#include "external/src/win/iupwin_scrollbar.c"
*/
import "C"
