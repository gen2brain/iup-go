//go:build darwin && !gtk && !gtk4 && !qt

package iup

/*
#include "external/src/cocoa/iupcocoa_popover.m"
*/
import "C"
