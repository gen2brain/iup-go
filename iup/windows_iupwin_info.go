//go:build windows && !gtk && !gtk4 && !qt && !winui && !efl && !fltk

package iup

/*
#include "external/src/win/iupwin_info.c"
*/
import "C"