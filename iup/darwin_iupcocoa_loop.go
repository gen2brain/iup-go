//go:build (darwin && !ios && !gtk && !gtk4 && !qt && !qml && !motif && !efl && !fltk) || gnustep

package iup

/*
#include "external/src/cocoa/iupcocoa_loop.m"
*/
import "C"