//go:build (darwin && !ios && !gtk && !gtk4 && !qt && !motif && !efl && !fltk) || gnustep

package iup

/*
#include "external/src/cocoa/iupcocoa_dialog.m"
*/
import "C"