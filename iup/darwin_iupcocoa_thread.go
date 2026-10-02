//go:build (darwin && !ios && !gtk3 && !gtk4 && !qt && !qml && !efl) || gnustep

package iup

/*
#include "external/src/cocoa/iupcocoa_thread.m"
*/
import "C"
