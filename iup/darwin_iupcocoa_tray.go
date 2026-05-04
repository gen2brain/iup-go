//go:build darwin && !ios && !gtk && !gtk4 && !efl

package iup

/*
#include "external/src/cocoa/iupcocoa_tray.m"
*/
import "C"
