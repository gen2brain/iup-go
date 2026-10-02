//go:build darwin && !ios && !gtk3 && !gtk4 && !efl

package iup

/*
#include "external/src/cocoa/iupcocoa_tray.m"
*/
import "C"
