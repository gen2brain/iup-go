//go:build (darwin && !ios && !gtk && !gtk4 && !qt && !motif && !efl && !fltk) || gnustep

package iup

/*
#include "external/src/cocoa/iupcocoa_progressbar.m"
*/
import "C"