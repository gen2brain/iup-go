//go:build windows && !gtk3 && !gtk4 && !qt6 && !qt5 && !qml && !winui && !efl && !fltk

package iup

/*
#include "external/src/win/iupwin_progressbar.c"
*/
import "C"