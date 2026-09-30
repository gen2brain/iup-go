//go:build windows && !gtk && !gtk4 && !qt && !qml && !efl

package iup

/*
#include "external/src/win/iupwin_thread.c"
*/
import "C"
