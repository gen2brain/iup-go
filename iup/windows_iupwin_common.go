//go:build windows && !gtk && !gtk4 && !qt && !qml && !winui && !efl && !fltk

package iup

/*
#include "external/src/win/iupwin_common.c"
*/
import "C"