//go:build darwin && !gtk && !gtk4 && !qt && !efl

package iup

/*
#include "external/src/cocoa/iupcocoa_thread.m"
*/
import "C"
