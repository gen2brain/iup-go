//go:build darwin && !gtk && !gtk4 && !qt && !motif && !efl && !fltk

package iup

/*
#include "external/src/cocoa/iupcocoa_key.m"
*/
import "C"
