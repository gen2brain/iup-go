//go:build (darwin && !ios && !gtk3 && !gtk4 && !qt && !qml && !efl && !fltk) || gnustep

package iup

/*
#include "external/src/cocoa/iupcocoa_popover.m"
*/
import "C"
