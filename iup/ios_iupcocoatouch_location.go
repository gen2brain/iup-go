//go:build ios && !gtk && !gtk4 && !qt && !motif && !efl && !fltk

package iup

/*
#include "external/src/cocoa/iupcocoa_location.m"
*/
import "C"
