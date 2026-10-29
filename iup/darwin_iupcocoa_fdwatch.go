//go:build darwin && !ios && !gtk3 && !gtk4 && !qt6 && !qt5 && !qml && !motif && !efl && !fltk

package iup

/*
#include "external/src/cocoa/iupcocoa_fdwatch.m"
*/
import "C"
