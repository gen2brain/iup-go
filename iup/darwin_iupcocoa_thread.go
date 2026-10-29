//go:build (darwin && !ios && !gtk3 && !gtk4 && !qt6 && !qt5 && !qml && !efl) || gnustep

package iup

/*
#include "external/src/cocoa/iupcocoa_thread.m"
*/
import "C"
