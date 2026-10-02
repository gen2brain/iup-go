//go:build (darwin && !ios && !gtk3 && !gtk4 && !qt && !qml && !motif && !efl && !fltk) || gnustep

package iup

/*
#include "external/src/cocoa/iupcocoa_key.m"
*/
import "C"
