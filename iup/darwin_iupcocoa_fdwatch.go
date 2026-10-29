//go:build darwin && !ios && !gtk3 && !gtk4 && !qt && !qml && !motif && !efl && !fltk

package iup

/*
#include "external/src/cocoa/iupcocoa_fdwatch.m"
*/
import "C"
