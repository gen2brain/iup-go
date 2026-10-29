//go:build windows && !gtk3 && !gtk4 && !qt6 && !qt5 && !qml && !efl

package iup

/*
#include "external/src/win/iupwin_thread.c"
*/
import "C"
