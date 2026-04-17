//go:build (darwin && !gtk && !gtk4 && !qt && !efl && !fltk) || gnustep

package iup

/*
#include "external/src/cocoa/iupcocoa_popover.m"
*/
import "C"
