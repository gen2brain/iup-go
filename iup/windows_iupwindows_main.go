//go:build windows && !qt6 && !qt5 && !qml && !winui && !efl

package iup

/*
#include "external/src/win/iupwindows_main.c"
*/
import "C"
